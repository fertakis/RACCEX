/*
 * Copyright 2012-2017 Intel Corporation.
 *
 * This file is subject to the Intel Sample Source Code License. A copy
 * of the Intel Sample Source Code License is included.
 */

/* scif_accept_ringbuffer.c : This program demonstrates a uni-directional messaging
 * mechanism. The connecting process allocates a window which is mapped by the remote
 * node. The remote node's ring buffer acts as receive queue to which messages are
 * written through mapped region.
 */

#include <scif.h>
#include "scif_tutorial.h"

#define START_OFFSET 0x80000
#define PAGE_SIZE 0x1000
#define PORT_NO 2050
#define BACKLOG 5

#define BARRIER(newepd, string) { \
	printf("%s\n", string); \
	if ((err = scif_send(newepd, &control_msg, sizeof(control_msg), 1)) <= 0) { \
		printf("scif_send failed with err %d\n", get_curr_status()); \
		fflush(stdout); \
		goto __end; \
	} \
	if ((err = scif_recv(newepd, &control_msg, sizeof(control_msg), 1)) <= 0) { \
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

int pop_data(ring_t *);

int main(int argc, char *argv[])
{
	scif_epd_t epd, newepd;
	struct scif_portID portID;
	off_t remote_offset;
	ring_t rbuf;

	void *mmap_addr;
	int i, err, conn_port, capacity;
	int control_msg, msg_size;

	if (argc != 3) {
		printf("usage: ./scif_accept_ringbuffer -n <no 4K pages>\n");
		exit(1);
	}
	capacity = atoi(argv[2]);
	msg_size = capacity * PAGE_SIZE;
	if (msg_size <= 0 || msg_size > INT_MAX) {
		printf("not valid msg size");
		exit(1);
	}

	/* open end pt */
	if ((epd = scif_open()) == SCIF_OPEN_FAILED) {
		printf("scif_open failed with err %d\n", get_curr_status());
		exit(1);
	}

	/* bind end pt to specified port */
	if ((conn_port = scif_bind(epd, PORT_NO)) < 0) {
		printf("scif_bind failed with error %d\n", get_curr_status());
		exit(2);
	}
	printf("bind success to port %d\n", conn_port);

	/* marks an end pt as listening end pt and queues up a maximum of BACKLOG
	 * no: of incoming connection requests
	 */
	if (scif_listen(epd, BACKLOG) != 0) {
		printf("scif_listen failed with error %d\n", get_curr_status());
		exit(1);
	}

	/* accepts a conn request by creating a new end pt that connects to peer */
	if (scif_accept(epd, &portID, &newepd, SCIF_ACCEPT_SYNC) != 0) {
		printf("scif_accept failed with error %d\n", get_curr_status());
		exit(1);
	}

	/* Receive Remote Registered window offset */
	scif_recv(newepd, &remote_offset, sizeof(remote_offset), 1);
	printf("remote window offset : 0x%lx\n", (unsigned long)remote_offset);

	/* mmap to remote registered window : not setting SCIF_MAP_FIXED allows to choose
	 * an implementation defined address in local VAS
	 */
	rbuf.buf = NULL;
	if ((mmap_addr = scif_mmap(rbuf.buf,
				msg_size,
				SCIF_PROT_READ | SCIF_PROT_WRITE,
				0,
				newepd,
				remote_offset)) == SCIF_MMAP_FAILED) {
		printf("scif_mmap failed with error : %d\n", get_curr_status());
		goto __end;
	}
	rbuf.buf = (char *)mmap_addr;
	rbuf.buf_end = (char *)rbuf.buf + msg_size;
	rbuf.local_head = rbuf.buf;
	rbuf.local_tail = rbuf.buf;
	rbuf.capacity = capacity;
	rbuf.buf_size = PAGE_SIZE;
	rbuf.curr_count = 0;

	/* Send Ring Buffer address to peer */
	scif_send(newepd, &rbuf.buf, sizeof(rbuf.buf), 1);

	/* Receive Remote Ring buffer addr */
	scif_recv(newepd, &rbuf.remote_head, sizeof(rbuf.remote_head), 1);
	rbuf.remote_tail = rbuf.remote_head;
	printf("remote ring buffer addr : %lx\n", (unsigned long)rbuf.remote_head);

	i = 0;
	while (i < rbuf.capacity) {
		BARRIER(newepd, "waiting on peer push");

		/* receive data msg from peer */
		err = pop_data(&rbuf);
		if (err == -1)
			printf("buffer empty, no msgs to pull\n");
		else {
			/* update remote pointers */
			rbuf.remote_head = (char *)rbuf.remote_head + rbuf.buf_size;
		}

		i++;
	}

	/* scif_unmap :  Removes the mapping to a remote window */
	if ((err = scif_munmap(mmap_addr, msg_size)) < 0) {
		printf("scif_munmap failed with error : %d\n", get_curr_status());
		goto __end;
	}
	BARRIER(newepd, "munmap done");

	BARRIER(newepd, "waiting on peer unregister");
	errno = 0;

__end:
	scif_close(newepd);
	scif_close(epd);

	if (errno == 0)
		printf("======== Program Success ========\n");
	else
		printf("======== Program Failed ========\n");

	return errno;
}

int pop_data(ring_t *rbuf)
{
	/* buffer empty */
	if (rbuf->curr_count == rbuf->capacity)
		return -1;

	/* pulling data msg from start of buffer */
	printf("poped data : 0x%x\n", ((int *)rbuf->local_tail)[0]);
	rbuf->local_tail = (char *)rbuf->local_tail + rbuf->buf_size;

	/* buffer end, reset pointer */
	if (rbuf->local_tail == rbuf->buf_end)
		rbuf->local_tail = rbuf->buf;
	rbuf->curr_count++;
	printf("current buffer count = %d\n", rbuf->curr_count);

	return 0;
}

