/*
 * Copyright 2012-2017 Intel Corporation.
 *
 * This file is subject to the Intel Sample Source Code License. A copy
 * of the Intel Sample Source Code License is included.
 */

/* scif_accept_register.c : demonstrates basic implementation fo registering windows
 * and mmap calls
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
	void *peer_addr;
	void *self_addr;
	off_t offset;
}win_t;

int main(int argc, char *argv[])
{
	scif_epd_t epd, newepd;
	struct scif_portID portID;
	off_t suggested_offset = 0, peer_offset;
	win_t buffer;

	int j, err, conn_port, unreg;
	int control_msg, msg_size, map_manager;

	if (argc != 7) {
		printf("usage: ./scif_accept_register -n <no 4K pages> "
				"-m <map_manager 0/1/2> -u <unreg_check>\n");
		exit(1);
	}
	msg_size = atoi(argv[2]) * PAGE_SIZE;
	if (msg_size <= 0 || msg_size > INT_MAX) {
		printf("not valid msg size");
		exit(1);
	}
	map_manager = atoi(argv[4]);
	unreg = atoi(argv[6]);

	/* open end pt */
	if ((epd = scif_open()) == SCIF_OPEN_FAILED) {
		printf("scif_open failed with err %d\n", errno);
		exit(1);
	}

	/* bind end pt to specified port */
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

	/* addresses in VAS & RAS must be multiple of page size */
	err = posix_memalign(&buffer.self_addr, 0x1000, msg_size);
	if (err != 0) {
		printf("posix_memalign failed with error : %d\n", err);
		goto __end;
	}
	memset(buffer.self_addr, 0xbc, msg_size);
	printf("Buffer allocated at 0x%lx\n", (unsigned long)buffer.self_addr);

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
	if ((buffer.offset = scif_register (newepd,
					buffer.self_addr,
					msg_size,
					suggested_offset,
					SCIF_PROT_READ | SCIF_PROT_WRITE,
					map_manager? SCIF_MAP_FIXED : 0)) < 0) {
		perror("scif_register failed with error");
		goto __end;
	}
	printf("registered buffer at offset 0x%lx\n", (unsigned long)buffer.offset);
	BARRIER(newepd, "register window done");

	if ((map_manager == 0) || (map_manager == 2)){
		/* receive peer's window offset to mmap */
		if ((err = scif_recv(newepd, &peer_offset, sizeof(peer_offset), 1)) < 0) {
			printf("scif_recv failed with error : %d\n", errno);
			goto __end;
		}
		/* send registered window offset to peer to allow mmaping */
		if ((err = scif_send(newepd, &buffer.offset, sizeof(buffer.offset), 1)) < 0) {
			printf("scif_send failed with error : %d\n", errno);
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
					newepd,
					peer_offset)) == MAP_FAILED) {
		perror("scif_mmap failed with error");
		goto __end;
	}
	printf("mapped buffers at address 0x%lx\n", (unsigned long)buffer.peer_addr);

	/* verify window registration & mapping */
	for (j = 0; j < (msg_size/sizeof(int)); j++) {
		if (((int*)buffer.peer_addr)[j] != 0xadadadad) {
			printf("data mismatch: peer_addr[%d] %x\n",
					j, ((int*)buffer.peer_addr)[j]);
			errno = -1;
			goto __end;
		}
	}

	if (unreg)
		BARRIER(newepd, "waiting on peer win unregister");

	/* scif_unmap :  removes mapping to a remote window */
	if ((err = scif_munmap(buffer.peer_addr, msg_size)) < 0) {
		perror("scif_munmap failed with error");
		goto __end;
	}

	/* scif_unregister : closes the registered window. window is not
	 * deleted until all references to it are removed
	 */
	if ((err = scif_unregister(newepd, buffer.offset, msg_size)) < 0) {
		perror("scif_unregister failed with error");
		goto __end;
	}

	if (unreg)
		BARRIER(newepd, "munmap done");

	BARRIER(newepd, "munmap & unregister done");
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
