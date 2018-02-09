/*
 * Copyright 2012-2017 Intel Corporation.
 *
 * This file is subject to the Intel Sample Source Code License. A copy
 * of the Intel Sample Source Code License is included.
 */

/* scif_connect_ringbuffer.c : This program demonstrates a uni-directional messaging
 * mechanism. The connecting process allocates a window which is mapped by the remote
 * node. The remote node's ring buffer acts as receive queue to which messages are
 * written through mapped region.
 */

#include <scif.h>
#include "scif_tutorial.h"

#define START_OFFSET 0x80000
#define PAGE_SIZE 0x1000
#define PEER_PORT 2050

#define BARRIER(epd, string) { \
	printf("%s\n", string); \
	if ((err = scif_send(epd, &control_msg, sizeof(control_msg), 1)) <= 0) { \
		printf("scif_send failed with err %d\n", get_curr_status()); \
		fflush(stdout); \
		goto __end; \
	} \
	if ((err = scif_recv(epd, &control_msg, sizeof(control_msg), 1)) <= 0) { \
		printf("scif_recv failed with err %d\n", get_curr_status()); \
		fflush(stdout); \
		goto __end; \
	} \
	printf("==============================================================\n"); \
}

typedef struct ringbuffer {
	void *buf;
	void *buf_end;
	void *local_head;
	void *local_tail;
	void *remote_head;
	void *remote_tail;
	int capacity;
	int buf_size;
	int curr_count;
}ring_t;

static int data = 0x0;

int push_data(ring_t *);

int main  (int argc, char *argv[])
{
	scif_epd_t epd;
	struct scif_portID portID;
	off_t suggested_offset, win_offset;
	ring_t rbuf;

	int i, err, control_msg, tries = 20;
	int conn_port, capacity, msg_size, map_fixed;

	if (argc != 7) {
		printf("usage: ./scif_connect_ringbuffer -n <no 4K pages> "
				"-m <map_fixed 0/1> -r <remote_node>\n");
		exit(1);
	}
	capacity = atoi(argv[2]);
	map_fixed = atoi(argv[4]);
	portID.node = atoi(argv[6]);
	msg_size = capacity * PAGE_SIZE;
	if (msg_size <= 0 || msg_size > INT_MAX) {
		printf("not valid msg size");
		exit(1);
	}
	portID.port = PEER_PORT;

	/* open end point */
	if ((epd = scif_open()) == SCIF_OPEN_FAILED) {
		printf("scif_open failed with error %d\n", get_curr_status());
		exit(1);
	}

	/* bind end point to available port, generated dynamically */
	if ((conn_port = scif_bind(epd, 0)) < 0) {
		printf("scif_bind failed with error %d\n", get_curr_status());
		exit(1);
	}
	printf("bind success to port %d\n", conn_port);

	/* initiate a connection to remote node, when successful returns the
	 * peer portID. Re-tries for 20 seconds and exits with error message
	 */
__retry:
	if (scif_connect(epd, &portID) < 0) {
		if ((get_curr_status() == ECONNREFUSED) && (tries > 0)) {
			printf("connection to node %d failed : trial %d\n", portID.node, tries);
			tries--;
#ifndef _WIN32
			sleep(1);
#else
			Sleep(1000);
#endif
			goto __retry;
		}
		printf("scif_connect failed with error %d\n", get_curr_status());
		exit(1);
	}
	printf("connect success\n");

	/* addresses in VAS & RAS must be multiple of page size */
#ifndef _WIN32
	err = posix_memalign(&rbuf.buf, 0x1000, msg_size);
	if (err != 0) {
		printf("posix_memalign failed with error : %d\n", err);
		goto __end;
	}
#else
	rbuf.buf = _aligned_malloc(msg_size, 0x1000);
	if (rbuf.buf == NULL) {
		printf("_aligned_malloc failed with error : %d\n", get_curr_status());
		goto __end;
	}
#endif
	memset(rbuf.buf, 0x0, msg_size);
	rbuf.buf_end = (char *)rbuf.buf + msg_size;
	rbuf.local_head = rbuf.buf;
	rbuf.local_tail = rbuf.buf;
	rbuf.capacity = capacity;
	rbuf.buf_size = PAGE_SIZE;
	rbuf.curr_count = 0;

	if (map_fixed)
		suggested_offset = START_OFFSET;
	else
		suggested_offset = 0;

	printf("registering size : %d\n", msg_size);
	/* scif_register :  marks a memory region for remote access */
	if ((win_offset = scif_register (epd,
					rbuf.buf,
					msg_size,
					suggested_offset,
					SCIF_PROT_READ | SCIF_PROT_WRITE,
					map_fixed? SCIF_MAP_FIXED : 0)) < 0) {
		printf("scif_register failed with error : %d\n", get_curr_status());
		goto __end;
	}
	printf("registered buffer at offset 0x%lx\n", (unsigned long)win_offset);

	/* Send Registered window offset to peer */
	scif_send(epd, &win_offset, sizeof(win_offset), 1);

	/* Receive remote node ring buffer address */
	scif_recv(epd, &rbuf.remote_head, sizeof(rbuf.remote_head), 1);
	rbuf.remote_tail = rbuf.remote_head;
	printf("remote node ring buffer addr : %lx\n", (unsigned long)rbuf.remote_head);

	/* Send Ring buffer addr to peer */
	scif_send(epd, &rbuf.buf, sizeof(rbuf.buf), 1);

	i = 0;
	while (i < rbuf.capacity) {
		/* send data msg to peer */
		err = push_data(&rbuf);
		if (err == -1)
			printf("buffer full, cannot push more msgs\n");
		else {
			/* update remote pointers */
			rbuf.remote_tail = (char *)rbuf.remote_tail + rbuf.buf_size;
		}

		BARRIER(epd, "waiting on peer pull");
		i++;
	}

	BARRIER(epd, "waiting on peer munmap");
	/* scif_unregister : closes the registered window */
	if ((err = scif_unregister(epd, win_offset, msg_size)) < 0) {
		printf("scif_unregister failed with error");
		goto __end;
	}
	BARRIER(epd, "unregister done");
	errno = 0;

__end:
	if (rbuf.buf != NULL) {
#ifndef _WIN32
		free(rbuf.buf);
#else
		_aligned_free(rbuf.buf);
#endif
	}
	scif_close(epd);

	if (errno == 0)
		printf("======== Program Success ========\n");
	else
		printf("======== Program Failed ========\n");

	return errno;
}

int push_data(ring_t *rbuf)
{
	/* buffer full */
	if (rbuf->curr_count == rbuf->capacity)
		return -1;

	/* pushing data msg at the end of buffer */
	data = data + 1;
	memset(rbuf->local_head, data, rbuf->buf_size);
	printf("pushed data : 0x%x\n", ((int *)rbuf->local_head)[0]);

	rbuf->local_head = (char *)rbuf->local_head + rbuf->buf_size;

	/* buffer end, reset pointer */
	if (rbuf->local_head == rbuf->buf_end)
		rbuf->local_head = rbuf->buf;
	rbuf->curr_count++;
	printf("current buffer count = %d\n", rbuf->curr_count);

	return 0;
}

