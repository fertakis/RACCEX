/*
 * Copyright 2012-2017 Intel Corporation.
 *
 * This file is subject to the Intel Sample Source Code License. A copy
 * of the Intel Sample Source Code License is included.
 */

/* scif_connect_vreadwrite_p2.c : this program demonstrates scif_vreadfrom & scif_vwriteto APIs
 * As these transfers are between local Virtual Address space and remote Registered Address
 * Space, it emphasizes the need for having registered windows on peer. It reduces the
 * overhead of window register/mmap on the node
 * Two processes P1, P2 write data onto remote node, which then swaps the data & allows P1, P2
 * to read updated data
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
	off_t offset;
}win_t;

int main(int argc, char *argv[])
{
	scif_epd_t epd;
	struct scif_portID portID;
	off_t remote_offset;
	win_t buffer;

	int j, err, control_msg, tries = 20;
	int conn_port, msg_size, use_cpu;

	if (argc != 7) {
		printf("usage: ./scif_connect_vreadwrite -n <no 4K pages> "
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
	memset(buffer.self_addr, 0xbc, msg_size);

	BARRIER(epd, "peer register window done");

	/* receive registered window offset from remote node */
	scif_recv(epd, &remote_offset, sizeof(remote_offset), 1);

	/* scif_vwriteto : copies msg_size bytes from local Virtual Addr Space to
	 * remote Registered Addr Space. When SCIF_RMA_USECPU is set, data is copied
	 * using programmed read/writes. Otherwise, data is copied using DMA
	 * if SCIF_RMA_SYNC flag is included, scif_vwriteto returns after the
	 * transfer is complete. Otherwise the transfer maybe performed
	 * asynchronously.
	 */
	if ((err = scif_vwriteto(epd,
				buffer.self_addr,
				msg_size,
				remote_offset,
				(use_cpu ? SCIF_RMA_USECPU : 0) | SCIF_RMA_SYNC))) {
		printf("scif_vwriteto failed with error : %d\n", get_curr_status());
		goto __end;
	}
	BARRIER(epd, "vwriteto done");

	/* scif_vreadfrom : copies msg_size bytes from remote Registered Addr Space
	 * to local Virtual Addr Space. When SCIF_RMA_USECPU is set, data is copied
	 * using programmed read/writes. Otherwise, data is copied using DMA
	 * if SCIF_RMA_SYNC flag is included scif_vreadfrom returns after the
	 * transfer is complete. Otherwise the tansfer maybe performed
	 * asynchronously.
	 */
	BARRIER(epd, "waiting on peer data swap");
	if ((err = scif_vreadfrom(epd,
				buffer.self_addr,
				msg_size,
				remote_offset,
				(use_cpu ? SCIF_RMA_USECPU : 0) | SCIF_RMA_SYNC))) {
		printf("scif_vreadfrom failed with error : %d\n", get_curr_status());
		goto __end;
	}
	BARRIER(epd, "readfrom done");

	/* verify if the data read is the swapped data */
	for (j = 0; j < (msg_size/sizeof(int)); j++) {
		if (((int*)buffer.self_addr)[j] != 0xadadadad) {
			printf("data mismatch: buffer.self_addr[%d] %x\n",
					j, ((int*)buffer.self_addr)[j]);
			errno = -1;
			goto __end;
		}
	}

	BARRIER(epd, "waiting on peer win unreg");
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
