/*
 * Copyright 2012-2017 Intel Corporation.
 *
 * This file is subject to the Intel Sample Source Code License. A copy
 * of the Intel Sample Source Code License is included.
 */

/* scif_accept_rma_fence.c : This program demonstrates the need for synchronization and how
 * it is achieved through scif_fence_mark() & scif_fence_wait(). In this program,
 * the peer node registers a window and uses scif_writeto to write PAGE_SIZE data chunks
 * to local RAS thus initiating multiple RMAs. The node performs a data reversal in terms
 * of PAGE_SIZE data chunks which allows the peer to get updated data through scif_readfrom.
 */

#include <scif.h>
#include "scif_tutorial.h"

#define VA_GEN_MIN 0x4000000000000000
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

#define FENCE(newepd, self) { \
	if ((err = scif_fence_mark(newepd, \
			self? SCIF_FENCE_INIT_SELF : SCIF_FENCE_INIT_PEER, \
			&mark))) { \
		printf("scif_fence_mark failed with err %d\n", get_curr_status()); \
		goto __end; \
	} \
	printf("Value of mark 0x%x\n", mark); \
	if ((err = scif_fence_wait(newepd, mark))) { \
		printf("scif_fence_wait failed with err %d\n", get_curr_status()); \
		goto __end; \
	} \
}

typedef struct window_info {
	void *self_addr;
	off_t offset;
}win_t;

int main(int argc, char *argv[])
{
	scif_epd_t epd, newepd;
	struct scif_portID portID;
	off_t suggested_offset;
	win_t buffer;

	char ack_msg[32]={0};
	void *temp = NULL, *curr_addr, *end_addr;
	int j, err, conn_port, no_buf, self=0;
	int control_msg, msg_size, map_fixed, mark;

	if (argc != 5) {
		printf("usage: ./scif_accept_rma_fence -n <no 4K pages>"
						" -m <map_fixed 0/1>\n");
		exit(1);
	}
	no_buf = atoi(argv[2]);
	map_fixed = atoi(argv[4]);
	msg_size = (no_buf + 1) * PAGE_SIZE;
	if ((no_buf <= 0 || no_buf > INT_MAX) || (msg_size <= 0 || msg_size > INT_MAX)) {
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
	memset(buffer.self_addr, 0x0, msg_size);

#ifndef _WIN32
	err = posix_memalign(&temp, 0x1000, PAGE_SIZE);
	if (err != 0) {
		printf("posix_memalign failed with error : %d\n", err);
		goto __end;
	}
#else
	temp = _aligned_malloc(PAGE_SIZE, 0x1000);
	if (temp == NULL) {
		printf("_aligned_malloc failed with error : %d\n", get_curr_status());
		goto __end;
	}
#endif
	memset(temp, 0x0, PAGE_SIZE);

	/* scif_register : marks a memory region for remote access */
	if ((buffer.offset = scif_register (newepd,
					buffer.self_addr,
					msg_size,
					suggested_offset,
					SCIF_PROT_READ | SCIF_PROT_WRITE,
					map_fixed? SCIF_MAP_FIXED : 0)) < 0) {
		printf("scif_register failed with error : %d\n", get_curr_status());
		goto __end;
	}
	printf("registered buffer at offset 0x%lx\n", (unsigned long)buffer.offset);
	BARRIER(newepd, "window reg done");

	/* Receive ACK msg from peer when it initiates RMA, to set fence */
	scif_recv(newepd, ack_msg, sizeof(ack_msg), 1);
	if (strcmp(ack_msg, "RMA_INIT") == 0) {
		/* scif_fence_mark : Marks the current set of uncompleted RMAs initiated
		 * through endpt or corresponding peer endpt which uses flags
		 * SCIF_FENCE_INIT_SELF and SCIF_FENCE_INIT_PEER respectively.
		 * The RMAs are marked with a value in mark.
		 * scif_fence_wait : Waits for the completion of RMAs marked with value
		 * in mark. As we dont have any RMAs intiatedfrom newepd, self = 0.
		 * We are fencing against scif_writeto operations initiatedy peer end point
		 */
		self = 0;
		FENCE(newepd, self);
	}

	/* Reversing the written data in PAGE_SIZE chunks */
	curr_addr = buffer.self_addr;
	/* Last chunk of RAS window is reserved for value update by scif_fence_signal */
	end_addr = (char *)buffer.self_addr + msg_size - (2 * PAGE_SIZE);

	for (j = 0; j < (int)no_buf/2; j++) {
		memcpy(temp, curr_addr, PAGE_SIZE);
		memcpy(curr_addr, end_addr, PAGE_SIZE);
		memcpy(end_addr, temp, PAGE_SIZE);

		curr_addr = (char *)curr_addr + PAGE_SIZE;
		end_addr = (char *)end_addr - PAGE_SIZE;
	}
	BARRIER(newepd, "data reverse done");

	/* Receive ACK msg from peer when it initiates RMA, to set fence */
	scif_recv(newepd, ack_msg, sizeof(ack_msg), 1);
	if (strcmp(ack_msg, "RMA_INIT") == 0) {
		/* Fencing against scif_readfrom operations initiated by peer end pt */
		FENCE(newepd, self);
	}

	/* scif_unregister : closes the registered window */
	if ((err = scif_unregister(newepd, buffer.offset, msg_size)) < 0) {
		printf("scif_unregister failed with error : %d\n", get_curr_status());
		goto __end;
	}
	BARRIER(newepd, "unregister window done");
	errno = 0;

__end:
	if (buffer.self_addr != NULL) {
#ifndef _WIN32
		free(buffer.self_addr);
#else
		_aligned_free(buffer.self_addr);
#endif
	}
	if (temp != NULL) {
#ifndef _WIN32
		free(temp);
#else
		_aligned_free(temp);
#endif
	}

	scif_close(newepd);
	scif_close(epd);
	if (errno == 0)
		printf("======== Program Success ========\n");
	else
		printf("======== Program Failed ========\n");

	return errno;
}

