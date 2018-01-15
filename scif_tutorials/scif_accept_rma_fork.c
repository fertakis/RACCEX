/*
 * Copyright 2012-2017 Intel Corporation.
 *
 * This file is subject to the Intel Sample Source Code License. A copy
 * of the Intel Sample Source Code License is included.
 */

/* scif_accept_rma_fork : demonstrates RMA across fork() with MADV_DONTFORK flag.
 * When VAS is not marked as MADV_DONTFORK, the copy-on-write semantics apply. As a
 * result, we can see that after fork(), the data written by child process is not
 * visible to subsequent read/writes. When VAS is marked as MADV_DONTFORK, the
 * virtual address range is visible only to parent and copy-on-write semantics
 * donot apply to that region
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <scif.h>

#define VA_GEN_MIN 0x4000000000000000
#define START_OFFSET 0x80000
#define PAGE_SIZE 0x1000
#define PORT_NO 2050
#define BACKLOG 5

#define BARRIER(newepd, string) { \
	printf("%s\n", string); \
	if ((err = scif_send(newepd, &control_msg, sizeof(control_msg), 1)) <= 0) { \
		printf("scif_send failed with err %d\n", errno); \
		fflush(stdout); \
		goto __end; \
	} \
	if ((err = scif_recv(newepd, &control_msg, sizeof(control_msg), 1)) <= 0) { \
		printf("scif_recv failed with err %d\n", errno); \
		fflush(stdout); \
		goto __end; \
	} \
	printf("==============================================================\n"); \
}

typedef struct window_info {
	void *self_addr;
	off_t offset;
	void *peer_addr;
}win_t;

int main(int argc, char *argv[])
{
	scif_epd_t epd, newepd;
	struct scif_portID portID;
	off_t suggested_offset;
	win_t buffer;

	int j, err, conn_port, control_msg;
	int msg_size, map_fixed, use_cpu, fork_flag;

	if (argc != 9) {
		printf("usage: ./scif_accept_rma_fork -n <no 4K pages> "
			"-f <fork 1/0> -m <map_fixed> -c <cpu/dma 1/0>\n");
		exit(1);
	}
	msg_size = atoi(argv[2]) * PAGE_SIZE;
	if (msg_size <= 0 || msg_size > INT_MAX) {
                printf("not valid msg size");
                exit(1);
        }
	fork_flag = atoi(argv[4]);
	map_fixed = atoi(argv[6]);
	use_cpu = atoi(argv[8]);

	/* open end point */
	if ((epd = scif_open()) == SCIF_OPEN_FAILED) {
		printf("scif_open failed with error %d\n", errno);
		exit(1);
	}

	/* bind end point to available port, generated dynamically */
	if ((conn_port = scif_bind(epd, PORT_NO)) < 0) {
		printf("scif_bind failed with error %d\n", errno);
		exit(1);
	}
	printf("bind success to port %d\n", conn_port);

	/* marks an end pt as listening end pt and queues up a maximum of BACKLOG
	 * no: of incoming connection requests
	 */
	if (scif_listen(epd, BACKLOG) != 0) {
		printf("scif_listen failed with error %d\n", errno);
		exit(1);
	}

	/* accepts a conn request by creating a new end pt that connects to peer */
	if (scif_accept(epd, &portID, &newepd, SCIF_ACCEPT_SYNC) != 0) {
		printf("scif_accept failed with error %d\n", errno);
		exit(1);
	}
	printf("accept success\n");

	err = posix_memalign(&buffer.self_addr, 0x1000, msg_size);
	if (err != 0) {
		printf("posix_memalign failed with error : %d\n", err);
		goto __end;
	}
	memset(buffer.self_addr, 0xef, msg_size);

	if (map_fixed)
		suggested_offset = START_OFFSET;
	else
		suggested_offset = VA_GEN_MIN;

	/* scif_register:  marks a memory region for remote access in RAS.
	 * Each page reperesents the physical memory which backs the
	 * corresponding pages in VAS.
	 */
	if ((buffer.offset = scif_register(newepd,
					buffer.self_addr,
					msg_size,
					suggested_offset,
					SCIF_PROT_READ | SCIF_PROT_WRITE,
					map_fixed? SCIF_MAP_FIXED : 0)) < 0) {
		err = errno;
		perror("scif_register failed with error");
		fflush(stdout);
		goto __end;
	}
	BARRIER(newepd, "scif_register done");

	if (!fork_flag) {
		printf("VAS not marked as MADV_DONTFORK\n");
		BARRIER(newepd, "waiting on peer child writeto");
	}

	/* Reading remote window data to verify if the memset done by child
	 * actually updated the backed VAS pages
	 */
	if ((err = scif_readfrom(newepd,
				buffer.offset,
				msg_size,
				buffer.offset,
				use_cpu ? SCIF_RMA_USECPU : 0))) {
		perror("scif_readfrom failed with error");
		goto __end;
	}
	BARRIER(newepd, "scif_readfrom done");

	for (j = 0; j < (msg_size/sizeof(int)); j++) {
		printf("buffer.self_addr[0] : 0x%x\n", ((int*)buffer.self_addr)[j]);
		if (((int*)buffer.self_addr)[j] != 0xbcbcbcbc) {
			printf("Data written by child process not visible!\n");
			break;
		}
	}

	/* scif_unregister : closes the registered window */
	if ((err = scif_unregister(newepd, buffer.offset, msg_size)) < 0) {
		perror("scif_unregister failed with error");
		goto __end;
	}
	BARRIER(newepd, "unregister window done");
	errno = 0;

__end:
	if (buffer.self_addr != NULL)
		free(buffer.self_addr);
	scif_close(newepd);
	scif_close(epd);

	if (errno == 0)
		printf("======== Program Success ========\n");
	else
		printf("======== Program Failed ========\n");

	return errno;
}
