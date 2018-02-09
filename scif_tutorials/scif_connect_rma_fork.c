/*
 * Copyright 2012-2017 Intel Corporation.
 *
 * This file is subject to the Intel Sample Source Code License. A copy
 * of the Intel Sample Source Code License is included.
 */

/* scif_connect_rma_fork : demonstrates RMA across fork() with MADV_DONTFORK flag.
 * When VAS is not marked as MADV_DONTFORK, the copy-on-write semantics apply. As a
 * result, we can see that after fork(), the data written by child process is not
 * visible to subsequent read/writes. When VAS is marked as MADV_DONTFORK, the
 * virtual address range is visible only to parent and copy-on-write semantics
 * donot apply to that region
 */

#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <scif.h>

#define VA_GEN_MIN 0x4000000000000000
#define START_OFFSET 0x80000
#define PAGE_SIZE 0x1000
#define PEER_PORT 2050

#define BARRIER(epd, string) { \
	printf("%s\n", string); \
	if ((err = scif_send(epd, &control_msg, sizeof(control_msg), 1)) <= 0) { \
		printf("scif_send failed with err %d\n", errno); \
		fflush(stdout);	\
		goto __end; \
	} \
	if ((err = scif_recv(epd, &control_msg, sizeof(control_msg), 1)) <= 0) { \
		printf("scif_recv failed with err %d\n", errno); \
		fflush(stdout);	\
		goto __end; \
	} \
	printf("==============================================================\n"); \
}

typedef struct window_info {
	void *self_addr;
	off_t offset;
	void *peer_addr;
}win_t;

void segfault_sigaction(int signal, siginfo_t *si, void *arg)
{
	printf("Caught segfault at address %p\n", si->si_addr);
	exit(0);
}

int main(int argc, char *argv[])
{
	scif_epd_t epd;
	struct scif_portID portID;
	off_t suggested_offset;
	win_t buffer;

	pid_t pid;
	void *vaddr = NULL;
	int j, err, conn_port, control_msg, tries = 20;
	int msg_size, map_fixed, use_cpu, status, fork_flag;

	/* to catch seg fault in child process */
	struct sigaction *sa;
	sa = (struct sigaction *)malloc(sizeof(struct sigaction));
	if (sa == NULL) {
		printf("allocating sigaction struct failed with err : %d\n", errno);
		exit(1);
	}
	memset(sa, 0, sizeof(struct sigaction));
	sigemptyset(&sa->sa_mask);
	sa->sa_sigaction = segfault_sigaction;
	sa->sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, sa, NULL);

	if (argc != 11) {
		printf("usage: ./scif_connect_rma_fork -n <no 4K pages> -f <fork 1/0>"
				" -m <map_fixed> -c <cpu/dma 0/1> -r <remote_node>\n");
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
	portID.node = atoi(argv[10]);
	portID.port = PEER_PORT;

	/* open end point */
	if ((epd = scif_open()) == SCIF_OPEN_FAILED) {
		printf("scif_open failed with error %d\n", errno);
		exit(1);
	}

	/* bind end point to available port, generated dynamically */
	if ((conn_port = scif_bind(epd, 0)) < 0) {
		printf("scif_bind failed with error %d\n", errno);
		exit(1);
	}
	printf("bind success to port %d\n", conn_port);

	/* initiate a connection to remote node, when successful returns the
	 * peer portID. Re-tries for 20 seconds and exits with error message
	 */
__retry:
	if (scif_connect(epd, &portID) < 0) {
		if ((errno == ECONNREFUSED) && (tries > 0)) {
			printf("connection to node %d failed : trial %d\n", portID.node, tries);
			tries--;
			sleep(1);
			goto __retry;
		}
		printf("scif_connect failed with error %d\n", errno);
		exit(1);
	}
	printf("connect success\n");

	/* addresses in VAS & RAS must be multiple of page size */
	err = posix_memalign(&buffer.self_addr, 0x1000, msg_size);
	if (err != 0) {
		printf("posix_memalign failed with error : %d\n", err);
		goto __end;
	}
	memset(buffer.self_addr, 0xad, msg_size);
	printf("Allocated buffer at 0x%lx \n", (unsigned long)buffer.self_addr);

	err = posix_memalign(&vaddr, 0x1000, msg_size);
	if (err != 0) {
		printf("posix_memalign failed with error : %d\n", err);
		goto __end;
	}
	memset(vaddr, 0x0, msg_size);

	/* Marking VAS as DONTFORK prevents the range from being seen by
	 * the child and copy-on-write semantics donot apply.
	 */
	if (fork_flag) {
		printf("VAS marked as MADV_DONTFORK\n");
		madvise(buffer.self_addr, msg_size, MADV_DONTFORK);
	}

	if (map_fixed)
		suggested_offset = START_OFFSET;
	else
		suggested_offset = VA_GEN_MIN;

	/* scif_register : marks a memory region for remote access in RAS
	 * Each page reperesents the physical memory which backs the
	 * corresponding pages in VAS.
	 */
	if ((buffer.offset = scif_register(epd,
					buffer.self_addr,
					msg_size,
					suggested_offset,
					SCIF_PROT_READ | SCIF_PROT_WRITE,
					map_fixed? SCIF_MAP_FIXED : 0)) < 0) {
		perror("scif_register failed with error");
		goto __end;
	}
	printf("registered buffer at offset 0x%lx \n", (unsigned long)buffer.offset);
	BARRIER(epd, "scif_register done");

	pid = fork();
	if (pid == 0) {
		printf("In child process\n");

		/* In the case of fork_flag = 1, child doesn't have access to VAS,
		 * and any attempt to access it should generate segmentation fault
		 */

		memset(buffer.self_addr, 0xbc, msg_size);
		/* scif_writeto : copies msg_size bytes from local registered address
		 * space to remote registered address space
		 */
		if ((err = scif_writeto(epd,
				buffer.offset,
				msg_size,
				buffer.offset,
				use_cpu ? SCIF_RMA_USECPU : 0))) {
			perror("scif_writeto failed with error");
			goto __end;
		}
		printf("Writing to remote window : buffer.self_addr[[0] : 0x%x\n",
							((int *)buffer.self_addr)[0]);
		BARRIER(epd, "child write data to remote window done");

		/* Reading remote window data into a new local buffer to
		 * verify if the data written by child is visible on peer
		 */
		if ((err = scif_vreadfrom(epd,
					vaddr,
					msg_size,
					buffer.offset,
					use_cpu ? SCIF_RMA_USECPU : 0))) {
			perror("scif_vreadfrom failed with error");
			goto __end;
		}
		for (j = 0; j < (msg_size/sizeof(int)); j++) {
			printf("Reading from remote window : vaddr[0] : 0x%x\n",
								((int*)vaddr)[j]);
			if (((int*)vaddr)[j] != 0xbcbcbcbc) {
				printf("Data written by child process not visible\n");
				exit(0);
			}
		}
	}
	else {
		wait(&status);
		BARRIER(epd, "waiting on peer readfrom");
		/* scif_unregister : closes the registered window */
		if ((err = scif_unregister(epd, buffer.offset, msg_size)) < 0) {
			perror("scif_unregister failed with error");
			goto __end;
		}
		BARRIER(epd, "unregister window done");
		errno = 0;
	} /* end of parent */
__end:
	if (sa != NULL)
		free(sa);
	if (vaddr != NULL)
		free(vaddr);
	if (buffer.self_addr != NULL)
		free(buffer.self_addr);
	scif_close(epd);

	if (errno == 0)
		printf("======== Program Success ========\n");
	else
		printf("======== Program Failed ========\n");

	return errno;
}
