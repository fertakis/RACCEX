/*
 * Copyright 2012-2017 Intel Corporation.
 *
 * This file is subject to the Intel Sample Source Code License. A copy
 * of the Intel Sample Source Code License is included.
 */

/* scif_connect_readwrite_p2.c : this program demonstrates scif_readfrom & scif_writeto APIs
 * As these transfers are between Registered Address Spaces, it emphasizes the need for having
 * registered windows on both the nodes.
 * Two processes P1, P2 write data onto remote node, which then swaps the data & allows P1, P2
 * to read updated data
 */

#include <scif.h>
#include "scif_tutorial.h"

#define VA_GEN_MIN 0x4000000000000000
#define START_OFFSET 0x80000
#define PAGE_SIZE 0x1000
#define PEER_PORT 2050

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
	void *self_addr;
	off_t offset;
}win_t;

int write_data(scif_epd_t, win_t *, int, int, char *);

int main(int argc, char *argv[])
{
	scif_epd_t epd;
	struct scif_portID portID;
	off_t suggested_offset;
	win_t buffer;

	char msg[32];
	int j, err, control_msg, tries = 20;
	int conn_port, msg_size, map_fixed, use_cpu;

	if (argc != 9) {
		printf("usage: ./scif_connect_readwrite -n <no 4K pages> "
			"-m <map_fixed 1/0> -c <cpu/dma 1/0> -r <remote_node>\n");
		exit(1);
	}
	msg_size = atoi(argv[2]) * PAGE_SIZE;
	if (msg_size <= 0 || msg_size > INT_MAX) {
		printf("not valid msg size");
		exit(1);
	}
	map_fixed = atoi(argv[4]);
	use_cpu = atoi(argv[6]);
	portID.node = atoi(argv[8]);
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
		printf("_aligned_malloc failed with error : %d\n", errno);
		goto __end;
	}
#endif
	memset(buffer.self_addr, 0xbc, msg_size);

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

	err = write_data(epd, &buffer, msg_size, use_cpu, msg);
	/* if scif_writeto fails with any other error, terminate the process */
	if (err != 0 && err != ENXIO)
		goto __end;

	scif_send(epd, msg, sizeof(msg), 1);
	if (err == ENXIO) {
		BARRIER(epd, "waiting on peer window register");
		/* upon successful peer window register, the process re-tries to
		 * write the data
		 */
		if ((err = write_data(epd, &buffer, msg_size, use_cpu, msg)) != 0)
			goto __end;
	}
	BARRIER(epd, "writeto done");

	/* scif_readfrom : copies msg_size bytes from remote registered address space
	 * to local registered address space. When SCIF_RMA_USECPU is set, data is
	 * copied using programmed read/writes. Otherwise, data is copied using DMA
	 * if SCIF_RMA_SYNC flag is included, scif_readfrom returns after the
	 * transfer is complete. Otherwise, the transfer maybe performed
	 * asynchronously.
	 */
	BARRIER(epd, "waiting on peer data swap");
	if ((err = scif_readfrom(epd,
				buffer.offset,
				msg_size,
				buffer.offset,
				(use_cpu ? SCIF_RMA_USECPU : 0) | SCIF_RMA_SYNC))) {
		printf("scif_readfrom failed with error : %d\n", get_curr_status());
		goto __end;
	}
	BARRIER(epd, "readfrom done");

	/* verify if the data read is the swapped data */
	for (j = 0; j < (msg_size/sizeof(int)); j++) {
		if (((int*)buffer.self_addr)[j] != 0xadadadad) {
			printf("data mismatch: self_addr[%d] 0x%x\n",
					j, ((int*)buffer.self_addr)[j]);
			errno = -1;
			goto __end;
		}
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

int write_data(scif_epd_t epd, win_t *buffer, int msg_size, int use_cpu, char *msg)
{
	int err;

	/* scif_writeto : copies msg_size bytes from local registered address space
	 * to remote registered address space. When SCIF_RMA_USECPU is set, data is
	 * copied using programmed read/writes. Otherwise, data is copied using DMA
	 * if SCIF_RMA_SYNC flag is included, scif_writeto returns after the
	 * transfer is complete. Otherwise, the transfer maybe performed
	 * asynchronously.
	 */
	if ((err = scif_writeto(epd,
				buffer->offset, /* local RAS offset */
				msg_size,
				buffer->offset, /* remote RAS offset */
				(use_cpu ? SCIF_RMA_USECPU : 0) | SCIF_RMA_SYNC))) {
		/* errno ENXIO : No such device or address*/
		if (get_curr_status() == ENXIO)
#ifndef _WIN32
			strcpy(msg, "WINREQ");
#else
			strcpy_s(msg, sizeof(msg), "WINREQ");
#endif
		else {
			printf("scif_writeto failed with error : %d\n", get_curr_status());
			return errno;
		}
	}
	else {
#ifndef _WIN32
		strcpy(msg, "SUCCESS");
#else
		strcpy_s(msg, sizeof(msg), "SUCCESS");
#endif
		errno = 0;
	}

	return errno;
}
