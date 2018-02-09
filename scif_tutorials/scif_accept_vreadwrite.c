/*
 * Copyright 2012-2017 Intel Corporation.
 *
 * This file is subject to the Intel Sample Source Code License. A copy
 * of the Intel Sample Source Code License is included.
 */

/* scif_accept_vreadwrite.c : this program demonstrates scif_vreadfrom & scif_vwriteto APIs
 * As these transfers are between local Virtual Address space and remote Registered Address
 * Space, it emphasizes the need for having registered windows on peer. It reduces the
 * overhead of window register or mmap on the node
 * Two processes P1, P2 write data onto remote node, which then swaps the data & allows P1, P2
 * to read updated data
 */

#include <scif.h>
#include "scif_tutorial.h"

#define START_OFFSET 0x80000
#define PAGE_SIZE 0x1000
#define CONNECTIONS 2
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

typedef struct window_info {
	void *peer_addr;
	void *self_addr;
	off_t offset;
}win_t;

int main(int argc, char *argv[])
{
	scif_epd_t epd, newepd[CONNECTIONS]={0};
	struct scif_portID portID;
	off_t suggested_offset;
	win_t *buffers = NULL;

	void *temp;
	int i, err, conn_port;
	int control_msg, msg_size, map_fixed;

	if (argc != 5) {
		printf("usage: ./scif_accept_vreadwrite -n <no 4K pages> "
						"-m <map_fixed 0/1> \n");
		exit(1);
	}
	msg_size = atoi(argv[2]) * PAGE_SIZE;
	if (msg_size <= 0 || msg_size > INT_MAX) {
		printf("not valid msg size");
		exit(1);
	}
	map_fixed = atoi(argv[4]);

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

	if (map_fixed)
		suggested_offset = START_OFFSET;
	else
		suggested_offset = 0;

	/* addresses in VAS & RAS must be multiple of page size */
#ifndef _WIN32
	err = posix_memalign(&temp, 0x1000, msg_size);
	if (err != 0) {
		printf("posix_memalign failed with error : %d\n", err);
		goto __end;
	}
#else
	temp = _aligned_malloc(msg_size, 0x1000);
	if (temp  == NULL) {
		printf("_aligned_malloc failed with error : %d\n", get_curr_status());
		goto __end;
	}
#endif
	memset(temp, 0x0, msg_size);

	buffers = (win_t *)  malloc(sizeof(win_t) * CONNECTIONS);
	if (buffers == NULL) {
		printf("mem allocation failed with error : %d\n", get_curr_status());
		goto __end;
	}
	for (i = 0; i < CONNECTIONS; i++) {
		/* accepts a conn request by creating a new end pt
		 * that connects to peer
		 */
		if (scif_accept(epd, &portID, &newepd[i], SCIF_ACCEPT_SYNC) != 0) {
			printf("scif_accept failed with error %d\n", get_curr_status());
			exit(1);
		}
		printf("accept success\n");

#ifndef _WIN32
		err = posix_memalign(&buffers[i].self_addr, 0x1000, msg_size);
		if (err != 0) {
			printf("posix_memalign failed with error : %d\n", err);
			goto __end;
		}
#else
		buffers[i].self_addr = _aligned_malloc(msg_size, 0x1000);
		if (buffers[i].self_addr == NULL) {
			printf("_aligned_malloc failed with error : %d\n", errno);
			goto __end;
		}
#endif
		memset(buffers[i].self_addr, 0x0, msg_size);

		/* scif_register : marks a memory region for remote access */
		if ((buffers[i].offset = scif_register(newepd[i],
						buffers[i].self_addr,
						msg_size,
						suggested_offset,
						SCIF_PROT_READ | SCIF_PROT_WRITE,
						map_fixed ? SCIF_MAP_FIXED : 0)) < 0) {
			printf("scif_register failed with error : %d\n", get_curr_status());
			goto __end;
		}
		printf("registered buffer at offset 0x%lx\n", (unsigned long)buffers[i].offset);
		BARRIER(newepd[i], "register window done");

		/* send registered window offset to remote node */
		scif_send(newepd[i], &buffers[i].offset, sizeof(buffers[i].offset), 1);

		BARRIER(newepd[i], "waiting on peer vwriteto");
	}

	/* swap data from process p1 & p2 */
	printf("Before Swap: \n");
	printf("buffer[0].self_addr[0] : %x buffer[1].self_addr[0] : %x\n",
		((int*)buffers[0].self_addr)[0], ((int*)buffers[1].self_addr)[0]);

	memcpy(temp, buffers[0].self_addr, msg_size);
	memcpy(buffers[0].self_addr, buffers[1].self_addr, msg_size);
	memcpy(buffers[1].self_addr, temp, msg_size);

	printf("After Swap: \n");
	printf("buffer[0].self_addr[0] : %x buffer[1].self_addr[0] : %x\n",
		((int*)buffers[0].self_addr)[0], ((int*)buffers[1].self_addr)[0]);

	for (i = 0; i < CONNECTIONS; i++) {
		BARRIER(newepd[i], "data swap done");
		BARRIER(newepd[i], "waiting on peer readfrom");
	}

	for (i = 0; i < CONNECTIONS; i++) {
		/* scif_unregister : closes the registered window */
		if ((err = scif_unregister(newepd[i],
					buffers[i].offset,
					msg_size)) != 0)
			goto __end;
		BARRIER(newepd[i], "win unregister done");
	}
	errno = 0;

__end:
	if (buffers != NULL) {
		for (i = 0; i < CONNECTIONS; i++) {
#ifndef _WIN32
			free(buffers[i].self_addr);
#else
			_aligned_free(buffers[i].self_addr);
#endif
			scif_close(newepd[i]);
		}
		free(buffers);
	}
	if (temp != NULL) {
#ifndef _WIN32
		free(temp);
#else
		_aligned_free(temp);
#endif
	}

	scif_close(epd);
	if (errno == 0)
		printf("======== Program Success ========\n");
	else
		printf("======== Program Failed ========\n");

	return errno;
}
