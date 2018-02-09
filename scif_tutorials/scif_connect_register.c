/*
 * Copyright 2012-2017 Intel Corporation.
 *
 * This file is subject to the Intel Sample Source Code License. A copy
 * of the Intel Sample Source Code License is included.
 */

/* scif_connect_register.c : demonstrates basic implementation of registering windows
 * and mmap calls. Also demonstrates unregistering of a window
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

#define START_OFFSET 0x80000
#define PAGE_SIZE 0x1000
#define PEER_PORT 2050

#define BARRIER(epd, string) { \
	printf("%s\n", string); \
	if ((err = scif_send(epd, &control_msg, sizeof(control_msg), 1)) <= 0) { \
		printf("scif_send failed with err %d\n", errno); \
		fflush(stdout); \
		goto __end; \
	} \
	if ((err = scif_recv(epd, &control_msg, sizeof(control_msg), 1)) <= 0) { \
		printf("scif_recv failed with err %d\n", errno); \
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

int main(int argc, char *argv[])
{
	scif_epd_t epd;
	struct scif_portID portID;
	off_t suggested_offset = 0, peer_offset;
	win_t buffer, newbuf;

	int j, err, control_msg, tries = 20;
	int conn_port, msg_size, map_manager, unreg;

	if (argc != 9) {
		printf("usage: ./scif_connect_register -n <no 4K pages> "
			"-m <map_manager 0/1/2> -r <remote_node> -u <unreg_check>\n");
		exit(1);
	}
	portID.port = PEER_PORT;
	msg_size = atoi(argv[2]) * PAGE_SIZE;
	if (msg_size <= 0 || msg_size > INT_MAX) {
		printf("not valid msg size");
		exit(1);
	}
	map_manager = atoi(argv[4]);
	portID.node = atoi(argv[6]);
	unreg = atoi(argv[8]);

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
	printf("Allocated buffer at 0x%lx\n", (unsigned long)buffer.self_addr);

	/* map_manager=0 : SCIF_MAP_FIXED not set, scif manages RAS; implementaion
	 * defined suitable offsets are chosen for mapping len bytes
	 * map_manager=1 : SCIF_MAP_FIXED set, user managed; specified fixed offset are
	 * used. For scif_register doesnt replace existing registrations & returns error.
	 * However, scif_mmap replaces existing mappings
	 * map_manager=2 : SCIF_MAP_FIXED set, OS managed; offset is same as virtual addr.
	 * This relieves the app from the need to track association between VA and RA as
	 * they are same
	 */
	if (map_manager == 0)
		suggested_offset = 0;
	else if (map_manager == 1)
		suggested_offset = START_OFFSET;
	else if (map_manager == 2)
		suggested_offset = (off_t) buffer.self_addr;

	/* scif_register : marks a memory region for remote access starting at offset po,
	 * a function of suggested_offset & msg_size which backs the VAS starting at
	 * buffer.self_addr. Successful registration returns po, offset where mapping
	 * is placed
	 */
	if ((buffer.offset = scif_register (epd,
					buffer.self_addr,
					msg_size,
					suggested_offset,
					SCIF_PROT_READ | SCIF_PROT_WRITE,
					map_manager? SCIF_MAP_FIXED : 0)) < 0) {
		perror("scif_register failed with error");
		goto __end;
	}
	printf("registered buffer at offset 0x%lx\n", (unsigned long)buffer.offset);
	BARRIER(epd, "register window done");

	if ((map_manager == 0) || (map_manager == 2)) {
		/* send registered window offset to peer to allow mmaping */
		if ((err = scif_send(epd, &buffer.offset, sizeof(buffer.offset), 1)) < 0) {
			printf("scif_send failed with error : %d\n", errno);
			goto __end;
		}
		/* receive peer's window offset to mmap */
		if ((err = scif_recv(epd, &peer_offset, sizeof(peer_offset), 1)) < 0) {
			printf("scif_recv failed with error : %d\n", errno);
			goto __end;
		}
	}
	else
		peer_offset = buffer.offset;

	/* scif_mmap : maps pages in VAS starting at peer_addr to remote window
	 * starting at buffer.offset where peer_addr is a function of buffer.self_addr
	 * & msg_size. successful mapping returns peer_addr, the address where
	 * mapping is placed.
	 */
	printf("peer window offset : 0x%lx\n", (unsigned long)peer_offset);
	if ((buffer.peer_addr = scif_mmap(buffer.self_addr,
					msg_size,
					SCIF_PROT_READ | SCIF_PROT_WRITE,
					map_manager? SCIF_MAP_FIXED : 0,
					epd,
					peer_offset)) == MAP_FAILED) {
		perror("scif_mmap failed with error");
		goto __end;
	}
	printf("mapped buffers at address 0x%lx\n", (unsigned long)buffer.peer_addr);

	/* verify window registration & mapping */
	for (j = 0; j < (msg_size/sizeof(int)); j++) {
		if (((int*)buffer.peer_addr)[j] != 0xbcbcbcbc) {
			printf("data mismatch: peer_addr[%d] %x\n",
					j, ((int*)buffer.peer_addr)[j]);
			errno = -1;
			goto __end;
		}
	}

	/* scif_munmap : removes mapping to a remote window */
	if ((err = scif_munmap(buffer.peer_addr, msg_size)) < 0) {
		perror("scif_munmap failed with error");
		goto __end;
	}

	/* scif_unregister : closes the registered window */
	/* unregister window without removing the peer mappings */
	if ((err = scif_unregister(epd, buffer.offset, msg_size)) < 0) {
		printf("scif_unregister failed with error");
		goto __end;
	}

	if (unreg) {
		BARRIER(epd, "win unregister done");

		/* Trying to register a new window over unregistered range
		 * This should fail because a window continues to exist until all
		 * the references to it are removed
		 */
		err = posix_memalign(&newbuf.self_addr, 0x1000, msg_size);
		if (err != 0) {
			printf("posix_memalign failed with error : %d\n", err);
			goto __end;
		}
		memset(newbuf.self_addr, 0x0, msg_size);
		if ((newbuf.offset = scif_register (epd,
						newbuf.self_addr,
						msg_size,
						buffer.offset,
						SCIF_PROT_READ | SCIF_PROT_WRITE,
						map_manager? SCIF_MAP_FIXED : 0)) < 0) {
			perror("scif_register failed with error");
		}

		/* remove any references to unregistered window which actually deletes
		 * the window
		 */
		BARRIER(epd, "waiting on peer munmap");

		/* this should work now as the window no longer exists over
		 * specified range
		 */
		if ((newbuf.offset = scif_register (epd,
						newbuf.self_addr,
						msg_size,
						buffer.offset,
						SCIF_PROT_READ | SCIF_PROT_WRITE,
						map_manager? SCIF_MAP_FIXED : 0)) < 0) {
			perror("scif_register failed with error");
		}
		else {
			printf("new window register success\n");
			printf("newbuf.offset : 0x%lx\n", (unsigned long)newbuf.offset);
		}

		free(newbuf.self_addr);
	}

	BARRIER(epd, "munmap & unregister done");
	errno = 0;

__end:
	if (buffer.self_addr != NULL)
		free(buffer.self_addr);
	scif_close(epd);

	if (errno == 0)
		printf("======== Program Success ========\n");
	else
		printf("======== Program Failed ========\n");

	return errno;
}
