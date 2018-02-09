/*
 * Copyright 2012-2017 Intel Corporation.
 *
 * This file is subject to the Intel Sample Source Code License. A copy
 * of the Intel Sample Source Code License is included.
 */

/* scif_connect_launch.c : remote process on card launched by host program (through ssh),
 * receives port_no from host and intiates a connection request. Also, demonstrates that
 * closing a connected end pt closes the the corresponding peer end pt.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <scif.h>

#ifdef HOST
#define PEER_NODE 1
#else
#define PEER_NODE 0
#endif

int main(int argc, char *argv[])
{
	scif_epd_t epd;
	struct scif_portID portID;

	int i, terminate, tries = 20;
	int block, msg_size, no_bytes, curr_size;
	char *send_buf, *recv_buf, *curr_addr;

	if (argc != 9) {
		printf("usage: ./scif_connect_launch_mic -r <peer_port> -s <msg_size>"
				" -b <block/nonblock 1/0> -t <terminate_check>\n");
		exit(1);
	}
	portID.node = PEER_NODE;
	portID.port = atoi(argv[2]);
	msg_size = atoi(argv[4]);
	if (msg_size <= 0 || msg_size > INT_MAX) {
		printf("not valid msg size");
		exit(1);
	}
	block = atoi(argv[6]);
	terminate = atoi(argv[8]);
	printf("Card side program launched, Initiating connection request\n");

	/* scif_open : creates an end point, when successful returns end pt descriptor */
	if ((epd = scif_open()) == SCIF_OPEN_FAILED) {
		printf("card: scif_open failed with error %d\n", (int)epd);
		exit(1);
	}

	/* scif_connect : initiate a connection to remote node, when successful returns
	 * the peer portID. Re-tries for 20 seconds and exits with error message
	 * When called without scif_bind(), it allocates a port from available pool
	 * of ports and automatically binds to epd
	 */
__retry:
	if (scif_connect(epd, &portID) < 0) {
		if ((errno == ECONNREFUSED) && (tries > 0)) {
			printf("connection to node %d failed : trial %d\n", portID.node, tries);
			tries--;
			sleep(1);
			goto __retry;
		}
		printf("card: scif_connect failed with error %d\n", errno);
		exit(1);
	}

	/* send & recv data to verify the connection
	 * scif_send : send messages between connected end pts. In blocking state, the call
	 * returns after sending entire msg unless interupted. In non-blocking state, it sends
	 * only those bytes that can be sent without waiting
	 */
	if ((send_buf = (char *)malloc(msg_size)) == NULL) {
		printf("mem allocation failed with error : %d\n", errno);
		goto __end;
	}
	memset(send_buf, 0xbc, msg_size);
	curr_addr = send_buf;
	curr_size = msg_size;
	while ((no_bytes = scif_send(epd, curr_addr, curr_size, block)) >= 0) {
		curr_addr = curr_addr + no_bytes;
		curr_size = curr_size - no_bytes;
		if(curr_size == 0)
			break;
	}
	if (no_bytes < 0) {
		printf("card: scif_send failed with error %d\n", errno);
		free(send_buf);
		goto __end;
	}
	printf("card: scif_send success\n");

	/* demonstrates the effect of closing a connected end pt */
	if (terminate) {
		printf("card: closing connected epd\n");
		goto __end;
	}

	/* scif_recv : receives data from a peer end pt. In blocking state, waits for the
	 * entire msg to be received. In non-blocking state, it receives only bytes which are
	 * available without waiting
	 */
	if ((recv_buf = (char *)malloc(msg_size)) == NULL) {
		printf("mem allocation failed with error : %d\n", errno);
		goto __end;
	}
	memset(recv_buf, 0x0, msg_size);
	curr_addr = recv_buf;
	curr_size = msg_size;
	while ((no_bytes = scif_recv(epd, curr_addr, curr_size, block)) >= 0) {
		curr_addr = curr_addr + no_bytes;
		curr_size = curr_size - no_bytes;
		if(curr_size == 0)
			break;
	}
	if (no_bytes < 0) {
		printf("card: scif_recv failed with error %d\n", errno);
		free(recv_buf);
		goto __end;
	}
	printf("card: scif_recv success\n");

	/* verify sent & received data : this is optional/usage dependent */
	for (i = 0; i < (msg_size/sizeof(int)); i++) {
		if (((int*)send_buf)[i] != ((int*)recv_buf)[i]) {
			printf("data mismatch send_buf[%d] 0x%x recv_buf[%d] 0x%x\n",
				i, ((int*)send_buf)[i], i, ((int*)recv_buf)[i]);
		}
	}
	if (send_buf != NULL)
		free(send_buf);
	if (recv_buf != NULL)
		free(recv_buf);
	errno = 0;

__end:
	/* scif_close : closes the end pt, when successful returns 0 */
	if (scif_close(epd) != 0) {
		printf("scif_close failed with error %d\n", errno);
		exit(1);
	}
	if (errno == 0)
		printf("======== card: Program Success ========\n");
	else
		printf("======== card: Program Failed ========\n");

	return errno;
}
