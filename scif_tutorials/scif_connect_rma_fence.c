/*
 * Copyright 2012-2017 Intel Corporation.
 *
 * This file is subject to the Intel Sample Source Code License. A copy
 * of the Intel Sample Source Code License is included.
 */

/* scif_connect_rma_fence.c : This program demonstrates the need for synchronization
 * and how it is achieved through scif_fence_signal(). In this program, the node
 * registers a window and uses scif_writeto to write PAGE_SIZE data chunks to remote
 * RAS thus initiating multiple RMAs. The peer performs a data reversal in terms of
 * PAGE_SIZE data chunks which allows the node to get updated data through scif_readfrom.
 */

#include <scif.h>
#include "scif_tutorial.h"

#define VA_GEN_MIN 0x4000000000000000
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

typedef struct window_info {
	void *self_addr;
	off_t offset;
}win_t;

int main(int argc, char *argv[])
{
	scif_epd_t epd;
	struct scif_portID portID;
	off_t suggested_offset, curr_offset, signal_offset;
	win_t buffer;

	void *curr_addr;
	char ack_msg[32];
	int j, err, data, no_buf, control_msg, tries = 20;
	int conn_port, msg_size, map_fixed, use_cpu;

	if (argc != 9) {
		printf("usage: ./scif_connect_rma_fence -n <no 4K pages> "
			"-m <map_fixed 1/0> -c <cpu/dma 1/0> "
			"-r <remote_node>\n");
		exit(1);
	}
	no_buf = atoi(argv[2]);
	map_fixed = atoi(argv[4]);
	use_cpu = atoi(argv[6]);
	portID.node = atoi(argv[8]);
	msg_size = (no_buf + 1) * PAGE_SIZE;
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

	if (map_fixed)
		suggested_offset = START_OFFSET;
	else
		suggested_offset = VA_GEN_MIN;

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
		printf("_aligned_malloc failed with error : %d\n", get_curr_status());
		goto __end;
	}
#endif

	/* scif_register : marks a memory region for remote access */
	if ((buffer.offset = scif_register (epd,
					buffer.self_addr,
					msg_size,
					suggested_offset,
					SCIF_PROT_READ | SCIF_PROT_WRITE,
					map_fixed? SCIF_MAP_FIXED : 0)) < 0) {
		printf("scif_register failed with error : %d\n", get_curr_status());
		goto __end;
	}
	printf("registered buffer at offset 0x%lx\n", (unsigned long)buffer.offset);
	BARRIER(epd, "window reg done");

	data = 0x1;
	curr_offset = buffer.offset;
	curr_addr = buffer.self_addr;
	for (j = 0; j < no_buf; j++) {
		memset(curr_addr, data, PAGE_SIZE);

		if ((err = scif_writeto(epd,
					curr_offset, /* local RAS offset */
					PAGE_SIZE,
					curr_offset, /* remote RAS offset */
					use_cpu ? SCIF_RMA_USECPU : 0))) {
			printf("scif_writeto failed with error : %d\n", get_curr_status());
			goto __end;
		}
		/* prints the starting value of PAGE_SIZE chunk to verify data */
		printf("curr_addr[0] : %lx\n", (unsigned long)((int *)curr_addr)[0]);

		data = data + 1;
		curr_addr = (char *)curr_addr + PAGE_SIZE;
		curr_offset = curr_offset + PAGE_SIZE;
	}

	/* Send a msg to peer about RMA initiation */
#ifndef _WIN32
	strcpy(ack_msg, "RMA_INIT");
#else
	strcpy_s(ack_msg, sizeof(ack_msg), "RMA_INIT");
#endif
	scif_send(epd, ack_msg, sizeof(ack_msg), 1);

	/* scif_fence_signal : Returns after marking the current set of uncompleted RMAs
	 * initiated from local endpt or remote endpt as specified by the flag
	 * SCIF_FENCE_INIT_SELF/ SCIF_FENCE_INIT_PEER. It signals the RMA completion
	 * by writing the given value at local or remote offset as specified by the
	 * flag SCIF_SIGNAL_LOCAL/ SCIF_SIGNAL_REMOTE. The specified signal_offset must
	 * be in a registered window.
	 */

	/* Last chunk of RAS window is reserved for value update by scif_fence_signal */
	signal_offset = buffer.offset + (no_buf * PAGE_SIZE);

	if ((err = scif_fence_signal(epd,
			signal_offset,	/* local offset */
			0xabcd,		/* value to be written to local offset */
			signal_offset,	/* remote offset */
			0xabcd,		/* value to be written to remote offset */
			SCIF_FENCE_INIT_SELF | SCIF_SIGNAL_LOCAL)) != 0) {
		printf("scif_fence_signal failed with error : %d\n", get_curr_status());
		goto __end;
	}

	BARRIER(epd, "waiting on peer reverse data");
	curr_offset = buffer.offset;
	for (j = 0; j < no_buf; j++) {
		if ((err = scif_readfrom(epd,
					curr_offset, /* local RAS offset */
					PAGE_SIZE,
					curr_offset, /* remote RAS offset */
					use_cpu ? SCIF_RMA_USECPU : 0))) {
			printf("scif_readfrom failed with error : %d\n", get_curr_status());
			goto __end;
		}
		curr_offset = curr_offset + PAGE_SIZE;
	}

	/* Send a msg to peer about RMA initiation */
	scif_send(epd, ack_msg, sizeof(ack_msg), 1);

	/* Fencing against scif_readfrom operations initiated from local endpt*/
	if ((err = scif_fence_signal(epd,
			signal_offset,	/* local offset */
			0xabcd,		/* value to be written to local offset */
			signal_offset,	/* remote offset */
			0xabcd,		/* value to be written to remote offset */
			SCIF_FENCE_INIT_SELF | SCIF_SIGNAL_LOCAL)) != 0) {
		printf("scif_fence_signal failed with error : %d\n", get_curr_status());
		goto __end;
	}

	/* prints the starting value of PAGE_SIZE chunk to verify data */
	curr_addr = buffer.self_addr;
	for (j = 0; j < no_buf; j++) {
		printf("curr_addr[0] : %lx\n", (unsigned long)((int *)curr_addr)[0]);
		curr_addr = (char *)curr_addr + PAGE_SIZE;
	}

	/* scif_unregister : closes the registered window */
	if ((err = scif_unregister(epd, buffer.offset, msg_size)) < 0) {
		printf("scif_unregister failed with error : %d\n", get_curr_status());
		goto __end;
	}
	BARRIER(epd, "unregister window done");
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
