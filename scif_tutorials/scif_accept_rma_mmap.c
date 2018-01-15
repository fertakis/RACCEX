/*
 * Copyright 2012-2017 Intel Corporation.
 *
 * This file is subject to the Intel Sample Source Code License. A copy
 * of the Intel Sample Source Code License is included.
 */

/* scif_accept_rma_mmap.c : In this program peer tries to mmap into node's registered
 * window which failes because of a smaller sized window. The peer then sends a msg
 * requesting window of sufficient size and the node registers new window allowing
 * peer to mmap. This program demonstrates mmaping over multiple contiguous windows
 */

#include <scif.h>
#include "scif_tutorial.h"

#define START_OFFSET 0x80000
#define PAGE_SIZE 0x1000
#define PORT_NO 2050
#define BACKLOG 5

#define BARRIER(newepd, string) { \
	printf("%s\n", string); \
	if ((err = scif_send(newepd, &control_msg, sizeof(control_msg), 1)) < 0) { \
		printf("scif_send failed with err %d\n", get_curr_status()); \
		fflush(stdout); \
		goto __end; \
	} \
	if ((err = scif_recv(newepd, &control_msg, sizeof(control_msg), 1)) < 0) { \
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

static int realloc_flag = 0;

int reg_win(scif_epd_t, win_t *, off_t, int);

int main(int argc, char *argv[])
{
	scif_epd_t epd, newepd;
	struct scif_portID portID;
	off_t suggested_offset;
	win_t buffer;
	win_t nbuffer={0};

	char msg[32]={0}, temp[32]={0};
	int err, conn_port, control_msg, orig_size;
	int i, j = 0, len, pages, msg_size, req_mem;

	if (argc != 3) {
		printf("usage: ./scif_accept_rma_mmap -n <no 4K pages>\n"
				"** This value should be less than or equal"
				" to value given on peer **\n");
		exit(1);
	}
	pages = atoi(argv[2]);
	msg_size = pages * PAGE_SIZE;
	if (msg_size <= 0 || msg_size > INT_MAX) {
		printf("not valid msg size");
		exit(1);
	}
	/* setting default to be SCIF_MAP_FIXED */
	suggested_offset = START_OFFSET;
	orig_size = msg_size;

	/* open end point */
	if ((epd = scif_open()) == SCIF_OPEN_FAILED) {
		printf("scif_open failed with error %d\n", get_curr_status());
		exit(1);
	}

	/* bind end point to available port, generated dynamically */
	if ((conn_port = scif_bind(epd, PORT_NO)) < 0) {
		printf("scif_bind failed with error %d\n", get_curr_status());
		exit(1);
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
	printf("accept success\n");

	/* helper function for scif_register */
	if ((err = reg_win(newepd, &buffer, suggested_offset, msg_size)))
		goto __end;

	scif_send(newepd, &buffer.offset, sizeof(buffer.offset), 1);

	printf("realloc_flag : %d\n", realloc_flag);
	/* receives ACK/NACK msg of mmap from peer */
	scif_recv(newepd, msg, sizeof(msg), 1);
	printf("msg received : %s\n", msg);

	/* MEMREQ indicates we need to register a bigger window
	 * with sufficient size
	 */
	if ((len = (int)strspn(msg, "MEMREQ ")) > 0) {
		for (i = len; i < strlen(msg); i++)
			temp[j++] = msg[i];
		temp[j] = '\0';
		req_mem = atoi(temp);

		/* registering a contiguous window */
		suggested_offset = START_OFFSET + msg_size;
		/* new window size = reminder of total size required
		 * by peer and old window size
		 */
		msg_size = (req_mem - pages) * PAGE_SIZE;
		if (msg_size <= 0 || msg_size > INT_MAX) {
			printf("not valid size\n");
			goto __end;
		}
		realloc_flag = 1;

		if ((err = reg_win(newepd, &nbuffer, suggested_offset, msg_size)))
			goto __end;
	}

	if (realloc_flag) {
#ifndef _WIN32
		strcpy(msg, "REALLOC_SUCCESS");
#else
		strcpy_s(msg, sizeof(msg), "REALLOC_SUCCESS");
#endif
	}
	else {
#ifndef _WIN32
		strcpy(msg, "SUCCESS");
#else
		strcpy_s(msg, sizeof(msg), "SUCCES");
#endif
	}
	/* send ACK to peer */
	scif_send(newepd, msg, (int)strlen(msg), 1);
	BARRIER(newepd, "waiting on peer mmap");

	/* scif_unregister : closes the registered window */
	if ((err = scif_unregister(newepd, buffer.offset, orig_size)) < 0) {
		printf("scif_unregister failed with error : %d\n", get_curr_status());
		goto __end;
	}
	if (realloc_flag) {
		if ((err = scif_unregister(newepd, nbuffer.offset, msg_size)) < 0) {
			printf("scif_unregister failed with error : %d\n", get_curr_status());
			goto __end;
		}
	}
	BARRIER(newepd, "waiting on peer munmap");
	BARRIER(newepd, "unregister done");
	errno = 0;

__end:
	if (buffer.self_addr != NULL) {
#ifndef _WIN32
		free(buffer.self_addr);
#else
		_aligned_free(buffer.self_addr);
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

int reg_win(scif_epd_t newepd, win_t *buffer, off_t suggested_offset, int msg_size)
{
	int err, control_msg;

	/* addresses in VAS & RAS must be multiple of page size */
#ifndef _WIN32
	err = posix_memalign(&buffer->self_addr, 0x1000, msg_size);
	if (err != 0) {
		printf("posix_memalign failed with error : %d\n", err);
		goto __end;
	}
#else
	buffer->self_addr = _aligned_malloc(msg_size, 0x1000);
	if (buffer->self_addr == NULL) {
		printf("_aligned_malloc failed with error : %d\n", get_curr_status());
		goto __end;
	}
#endif
	memset(buffer->self_addr, 0xbc, msg_size);

	/* scif_register : marks a memory region for remote access starting at offset po,
	 * a function of suggested_offset & msg_size which backs the VAS starting at
	 * buffer.self_addr. Successful registration returns po, offset where mapping
	 * is placed
	 */
	if ((buffer->offset = scif_register(newepd,
					buffer->self_addr,
					msg_size,
					suggested_offset,
					SCIF_PROT_READ | SCIF_PROT_WRITE,
					SCIF_MAP_FIXED)) < 0) {
		printf("scif_register failed with error : %d\n", get_curr_status());
		fflush(stdout);
		goto __end;
	}
	printf("registered buffers at address 0x%lx\n", (unsigned long)buffer->offset);
	BARRIER(newepd, "scif_register done");
	errno = 0;

__end:
	return errno;
}
