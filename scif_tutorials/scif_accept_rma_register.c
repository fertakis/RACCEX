/*
 * Copyright 2012-2017 Intel Corporation.
 *
 * This file is subject to the Intel Sample Source Code License. A copy
 * of the Intel Sample Source Code License is included.
 */

/* scif_accept_rma_register.c : This program demonstrates 2 processes P1 & P2 where each
 * establish a connection with peer end pt and has a corresponding registered window. Both
 * windows on peer back same physical address pages. P1 writes to peer at corresponding
 * window offset & P2 reads from its corresponding offset. P2 should be able to read the
 * data written by P1 and vice-versa
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
	void *self_addr;
	off_t offset;
}win_t;

int main(int argc, char *argv[])
{
	scif_epd_t epd, newepd[CONNECTIONS]={0};
	struct scif_portID portID;
	off_t suggested_offset;
	win_t buffer[CONNECTIONS];

	int i, err, conn_port;
	int control_msg, msg_size, map_fixed;

	if (argc != 5) {
		printf("usage: ./scif_accept_rma_register -n <no 4K pages>"
						" -m <map_fixed 0/1>\n");
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
	err = posix_memalign(&buffer[0].self_addr, 0x1000, msg_size);
	if (err != 0) {
		printf("posix_memalign failed with error : %d\n", err);
		goto __end;
	}
#else
	buffer[0].self_addr = _aligned_malloc(msg_size, 0x1000);
	if (buffer[0].self_addr == NULL) {
		printf("_aligned_malloc failed with error : %d\n", get_curr_status());
		goto __end;
	}
#endif
	memset(buffer[0].self_addr, 0x0, msg_size);

	/* using same Virtual Address for both windows to back
	 * same physical address pages */
	buffer[1].self_addr = buffer[0].self_addr;

	for (i = 0; i < CONNECTIONS; i++) {
		/* accepts a conn request by creating a new end pt
		 * that connects to peer
		 */
		if (scif_accept(epd, &portID, &newepd[i], SCIF_ACCEPT_SYNC) != 0) {
			printf("scif_accept failed with error %d\n", get_curr_status());
			exit(1);
		}
		printf("accept success\n");

		/* scif_register : marks a memory region for remote access starting at
		 * offset po, a function of suggested_offset & msg_size which backs the
		 * VAS starting at buffer.self_addr. Successful registration returns po,
		 * offset where mapping is placed
		 */
		if ((buffer[i].offset = scif_register (newepd[i],
					buffer[i].self_addr,
					msg_size,
					suggested_offset,
					SCIF_PROT_READ | SCIF_PROT_WRITE,
					map_fixed ? SCIF_MAP_FIXED : 0)) < 0) {
			printf("scif_register failed with error : %d\n", get_curr_status());
			goto __end;
		}
		printf("registered buffer at offset 0x%lx\n",
					(unsigned long)buffer[i].offset);
		BARRIER(newepd[i], "register window done");

		/* send registered window offset to remote node */
		scif_send(newepd[i], &buffer[i].offset, sizeof(buffer[i].offset), 1);
	}

	/* Following BARRIERs are used for synchronization between P1, P2 rread/writes */
	for (i = 0; i < CONNECTIONS; i++)
		BARRIER(newepd[i], "waiting on peer vwriteto");

	for (i = CONNECTIONS; i > 0; i--)
		BARRIER(newepd[i-1], "sync processes P1 & P2");

	for (i = 0; i < CONNECTIONS; i++)
		BARRIER(newepd[i], "waiting on peer vreadfrom");

	for (i = 0; i < CONNECTIONS; i++) {
		 /* scif_unregister : closes the registered window */
		if ((err = scif_unregister(newepd[i], buffer[i].offset, msg_size)) < 0) {
			printf("scif_unregister failed with error : %d\n", get_curr_status());
			goto __end;
		}
		BARRIER(newepd[i], "unregister done");
	}
	errno = 0;

__end:
	if (buffer[0].self_addr != NULL) {
#ifndef _WIN32
		free(buffer[0].self_addr);
#else
		_aligned_free(buffer[0].self_addr);
#endif
	}
	for (i = 0; i < CONNECTIONS; i++)
		scif_close(newepd[i]);
	scif_close(epd);

	if (errno == 0)
		printf("======== Program Success ========\n");
	else
		printf("======== Program Failed ========\n");

	return errno;
}
