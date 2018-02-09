/*
 * Copyright 2012-2017 Intel Corporation.
 *
 * This file is subject to the Intel Sample Source Code License. A copy
 * of the Intel Sample Source Code License is included.
 */

/* scif_connect_rma_register_p1.c : This program demonstrates 2 processes P1 & P2 where each
 * establish a connection with peer end pt and has a corresponding registered window. Both
 * windows on peer back same physical address pages. P1 writes to peer at corresponding
 * window offset & P2 reads from its corresponding offset. P2 should be able to read the
 * data written by P1 and vice-versa
 */

#include <scif.h>
#include "scif_tutorial.h"

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

typedef struct window_info {
	void *self_addr;
}win_t;

int main(int argc, char *argv[])
{
	scif_epd_t epd;
	struct scif_portID portID;
	win_t buffer;
	off_t suggested_offset;

	int err, control_msg, tries = 20;
	int conn_port, msg_size, use_cpu;

	if (argc != 7) {
		printf("usage: ./scif_connect_rma_register -n <no 4K pages> "
				"-c <cpu/dma 1/0> -r <remote_node>\n");
		exit(1);
	}
	msg_size = atoi(argv[2]) * PAGE_SIZE;
	if (msg_size <= 0 || msg_size > INT_MAX) {
		printf("not valid msg size");
		exit(1);
	}
	use_cpu = atoi(argv[4]);
	portID.node = atoi(argv[6]);
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
	err = posix_memalign(&buffer.self_addr, 0x1000, msg_size);
	if (err != 0) {
		printf("posix_memalign failed with error : %d\n", err);
		goto __end;
	}
#else
	buffer.self_addr = _aligned_malloc(msg_size, 0x1000);
	if (buffer.self_addr == NULL) {
		printf("_aligned_malloc failed with error : %d\n", errno);
		goto __end;
	}
#endif
	memset(buffer.self_addr, 0xad, msg_size);

	BARRIER(epd, "waiting on peer win register");
	/* Receive registered window offset from remote node */
	scif_recv(epd, &suggested_offset, sizeof(suggested_offset), 1);

	/* scif_vwriteto : copies msg_size bytes from local virtual address space
	 * to remote registered address space. When SCIF_RMA_USECPU is set, data is
	 * copied using programmed read/writes. Otherwise, data is copied using DMA
	 */
	if ((err = scif_vwriteto(epd,
				buffer.self_addr,
				msg_size,
				suggested_offset,
				(use_cpu ? SCIF_RMA_USECPU : 0) | SCIF_RMA_SYNC))) {
		printf("scif_vwriteto failed with error : %d\n", get_curr_status());
		goto __end;
	}
	printf("Writing: buffer.self_addr[0] : %x\n",
				((int *)buffer.self_addr)[0]);

	BARRIER(epd, "vwriteto done");
	BARRIER(epd, "waiting on P2 vwriteto");
	/* scif_vreadfrom : copies msg_size bytes from remote registered address space
	 * to local virtual address space. When SCIF_RMA_USECPU is set, data is copied
	 * using programmed read/writes. Otherwise, data is copied using DMA
	 */
	if ((err = scif_vreadfrom(epd,
				buffer.self_addr,
				msg_size,
				suggested_offset,
				(use_cpu ? SCIF_RMA_USECPU : 0) | SCIF_RMA_SYNC))) {
		printf("scif_vreadfrom failed with error : %d\n", get_curr_status());
		goto __end;
	}
	printf("Reading: buffer.self_addr[0] : %x\n",
				((int *)buffer.self_addr)[0]);

	BARRIER(epd, "vreadfrom done");
	BARRIER(epd, "waiting on peer unregister window");
	errno = 0;

__end:
	if (buffer.self_addr != NULL) {
#ifndef _WIN32
		free(buffer.self_addr);
#else
		_aligned_free(buffer.self_addr);
#endif
	}
	scif_close(epd);

	if (errno == 0)
		printf("======== Program Success ========\n");
	else
		printf("======== Program Failed ========\n");

	return errno;
}
