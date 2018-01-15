/*
 * Copyright 2012-2017 Intel Corporation.
 *
 * This file is subject to the Intel Sample Source Code License. A copy
 * of the Intel Sample Source Code License is included.
 */

/* scif_connect_rma_mmap.c : In this program node tries to mmap into peer's registered
 * window which failes because of a smaller sized window. The node then sends a msg
 * requesting window of sufficient size and the peer registers new window allowing
 * node to mmap. This program demonstrates mmaping over multiple contiguous windows
 */

#include <scif.h>
#include "scif_tutorial.h"

#define START_OFFSET 0x80000
#define PAGE_SIZE 0x1000
#define PEER_PORT 2050
#define BACKLOG 5

#define BARRIER(epd, string) { \
	printf("%s\n", string); \
	if ((err = scif_send(epd, &control_msg, sizeof(control_msg), 1)) < 0) { \
		printf("scif_send failed with err %d\n", get_curr_status()); \
		fflush(stdout); \
		goto __end; \
	} \
	if ((err = scif_recv(epd, &control_msg, sizeof(control_msg), 1)) < 0) { \
		printf("scif_recv failed with err %d\n", get_curr_status()); \
		fflush(stdout); \
		goto __end; \
	} \
	printf("==============================================================\n"); \
}

typedef struct window_info {
	void *mmap_addr;
	void *self_addr;
	off_t offset;
}win_t;

static int realloc_flag = 0;

void mmap_helper(scif_epd_t, win_t *, int, int, char *);

int main(int argc, char *argv[])
{
	scif_epd_t epd;
	struct scif_portID portID;
	win_t buffer;

	char msg[32];
	size_t ret;
	int err, pages, control_msg, tries = 20;
	int j, conn_port, msg_size;

	if (argc != 5) {
		printf("usage: ./scif_connect_rma_mmap -n <no 4K pages> "
							"-r <remote_node>\n");
		exit(1);
	}
	pages = atoi(argv[2]);
	portID.node = atoi(argv[4]);
	msg_size = pages * PAGE_SIZE;
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
	err = posix_memalign(&buffer.self_addr, 0x1000, msg_size);
	if (err != 0) {
		printf("posix_memalign failed with error : %d\n", (int)err);
		goto __end;
	}
	memset(buffer.self_addr, 0xbc, msg_size);
#else
	buffer.self_addr = NULL;
#endif

	BARRIER(epd, "waiting on peer register window done");

	/* Receive remote registered window offset */
	scif_recv(epd, &buffer.offset, sizeof(buffer.offset), 1);

	mmap_helper(epd, &buffer, msg_size, pages, msg);
	printf("msg : %s\n", msg);
	/* if mmap fails due to other errors, terminate the process */
	if (strcmp(msg, "FAIL") == 0)
		goto __end;
	else {
		scif_send(epd, msg, sizeof(msg), 1);
		printf("realloc_flag : %d\n", realloc_flag);
		if (realloc_flag) {
			/* waiting for peer to register new window with
			 * sufficient size for mmap
			 */
			BARRIER(epd, "waiting on peer new window register");
		}
		/* receives Acknowledgement from peer */
		scif_recv(epd, msg, (int)strlen(msg), 1);
		if ((ret = strspn(msg, "REALLOC")) > 0) {
			/* re-mmap */
			mmap_helper(epd, &buffer, msg_size, pages, msg);
			printf("msg : %s\n", msg);
			if (strcmp(msg, "FAIL") == 0)
				goto __end;
		}
	}

	/* verify window registration & mapping */
	for (j = 0; j < (msg_size/sizeof(int)); j++) {
		if (((int*)buffer.mmap_addr)[j] != 0xbcbcbcbc) {
			printf("data mismatch: self_addr[%d] %x\n",
					j, ((int*)buffer.mmap_addr)[j]);
			errno = -1;
			goto __end;
		}
	}
	BARRIER(epd, "multi window mmap done");

	/* scif_unmap :  removes mapping to a remote window */
	if ((err = scif_munmap(buffer.mmap_addr, msg_size)) < 0) {
		printf("scif_munmap failed with error : %d\n", get_curr_status());
		goto __end;
	}
	BARRIER(epd, "munmap done");
	BARRIER(epd, "remote window unregister done");
	errno = 0;

__end:
#ifndef _WIN32
	if (buffer.self_addr != NULL)
		free(buffer.self_addr);
#endif
	scif_close(epd);

	if (errno == 0)
		printf("======== Program Success ========\n");
	else
		printf("======== Program Failed ========\n");

	return errno;
}

void mmap_helper(scif_epd_t epd, win_t *buffer, int msg_size, int pages, char *msg)
{
	/* scif_mmap : maps pages in VAS starting at pa to remote window starting
	 * at buffer.offset where pa is a function of buffer.self_addr & msg_size.
	 * successful mapping returns pa, the address where mapping is placed
	 */
	if ((buffer->mmap_addr = scif_mmap(buffer->self_addr,
					msg_size,
					SCIF_PROT_READ | SCIF_PROT_WRITE,
#ifndef _WIN32
					SCIF_MAP_FIXED,
#else
					0, /* mapping a malloc returned address is not
					    * supported on windows. so, not setting
					    * SCIF_MAP_FIXED flag, picks up implementation
					    * defined address
					    */
#endif
					epd,
					buffer->offset)) == SCIF_MMAP_FAILED) {

		printf("scif_mmap failed with error : %d\n", get_curr_status());

		if (get_curr_status() != ENXIO)
#ifndef _WIN32
			strcpy(msg, "FAIL");
#else
			strcpy_s(msg, sizeof(msg), "FAIL");
#endif
		else {
			/* errno ENXIO implies Bad address or No Device which could be
			 * due to bad window offset. So node sends to peer the
			 * required window size to mmap
			 */
			realloc_flag = 1;
#ifndef _WIN32
			sprintf(msg, "MEMREQ %d", pages);
#else
			sprintf_s(msg, sizeof(msg), "MEMREQ %d", pages);
#endif
		}
	}
	else {
		printf("mapped buffers at address 0x%lx\n",
					(unsigned long)buffer->mmap_addr);
#ifndef _WIN32
		strcpy(msg, "SUCCESS");
#else
		strcpy_s(msg, sizeof(msg), "SUCCESS");
#endif
	}
}
