/*
 * Copyright 2012-2017 Intel Corporation.
 *
 * This file is subject to the Intel Sample Source Code License. A copy
 * of the Intel Sample Source Code License is included.
 */

/* scif_connect_poll.c : uses poll on send and recv queues for transfering large data
 * in non-blocking mode
 */

#include <scif.h>
#include "scif_tutorial.h"

#define PAGE_SIZE 0x1000

int main(int argc, char *argv[])
{
	scif_epd_t epd;
	struct scif_portID portID;
	struct scif_pollepd *spollfd = NULL;

	int i, tries = 20;
	int block, msg_size, curr_size, no_bytes;
	int conn_port, req_port;
	char *send_buf = NULL, *recv_buf = NULL, *curr_buf;

	if (argc != 11) {
		printf("usage: ./scif_connect_poll -l <local_port> -n <peer_node[host/card 0/1]>"
			" -r <remote_port> -s <no 4k pages> -b <block/non-block 1/0>\n");
		exit(1);
	}
	req_port = atoi(argv[2]);
	portID.node = atoi(argv[4]);
	portID.port = atoi(argv[6]);
	msg_size = atoi(argv[8]) * PAGE_SIZE;
	if (msg_size <= 0 || msg_size > INT_MAX) {
		printf("not valid msg size");
		exit(1);
	}
	block = atoi(argv[10]);

	/* scif_open : creates an end point, when successful returns end pt descriptor */
	if ((epd = scif_open()) == SCIF_OPEN_FAILED) {
		printf("scif_open failed with error %d\n", (int)epd);
		exit(1);
	}

	/* scif_bind : binds an end pt to a port_no, when successful returns the port_no
	 * to which the end pt is bound
	 */
	if ((conn_port = scif_bind(epd, req_port)) < 0) {
		printf("scif_bind failed with error %d\n", conn_port);
		exit(1);
	}
	printf("scif_bind success to port %d\n", conn_port);

	/* scif_connect : initiate a connection to remote node, when successful returns
	 * the peer portID. Re-tries for 20 seconds and exits with error message
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

	/* send & recv data to verify the connection
	 * scif_send : send messages between connected end pts. Transfering large data in
	 * Non-blocking mode by using poll on send queue. Sends only bytes that are
	 * available without waiting
	 */
	send_buf = (char *)malloc(msg_size);
	if (send_buf == NULL) {
		printf("mem allocation failed with error : %d\n", get_curr_status());
		goto __end;
	}
	memset(send_buf, 0xbc, msg_size);
	curr_buf = send_buf;
	curr_size = msg_size;

	spollfd = (struct scif_pollepd *)malloc(sizeof(struct scif_pollepd));
	if (spollfd == NULL) {
		printf("allocating mem to pollfd struct failed with Err: %d\n", get_curr_status());
		goto __end;
	}
	spollfd->epd = epd;
	spollfd->events = SCIF_POLLOUT;
	spollfd->revents = SCIF_POLLOUT;
	while ((no_bytes = scif_send(epd, curr_buf, curr_size, block)) < curr_size) {
		if (no_bytes < 0) {
			printf("scif_send failed with error %d\n", get_curr_status());
			goto __end;
		}
		curr_size = curr_size - no_bytes;
		curr_buf = curr_buf + no_bytes;
		printf("polling on send queue: bytes remaining = %d\n", curr_size);
		scif_poll(spollfd, 1, 20);
	}
	printf("scif_send success\n");

	/* scif_recv : receives data from a peer end pt. Transfering large data in
	 * non-blocking state, by using poll on recv queue. It receives only bytes
	 * which are available without waiting
	 */
	recv_buf = (char *)malloc(msg_size);
	if (recv_buf == NULL) {
		printf("mem allocation failed with error : %d\n", get_curr_status());
		goto __end;
	}
	memset(recv_buf, 0x0, msg_size);
	curr_buf = recv_buf;
	curr_size = msg_size;

	spollfd->events = SCIF_POLLIN;
	spollfd->revents = SCIF_POLLIN;
	no_bytes = 0;
	while ((no_bytes = scif_recv(epd, curr_buf, curr_size, block)) < curr_size) {
		if (no_bytes < 0) {
			printf("scif_recv failed with error %d\n",get_curr_status());
			goto __end;
		}
		curr_buf = curr_buf + no_bytes;
		curr_size = curr_size - no_bytes;
		printf("polling on recv queue: bytes_remaining = %d\n", curr_size);
		scif_poll(spollfd, 1, 20);
	}
	printf("scif_recv success\n");

	/* verify sent & received data */
	for (i = 0; i < (msg_size/sizeof(int)); i++) {
		if (((int*)send_buf)[i] != ((int*)recv_buf)[i]) {
			printf("data mismatch send_buf[%d] 0x%x recv_buf[%d] 0x%x\n",
				i, ((int*)send_buf)[i], i, ((int*)recv_buf)[i]);
			errno = -1;
			goto __end;
		}
	}
	errno = 0;

__end:
	if (send_buf != NULL)
		free(send_buf);
	if (recv_buf != NULL)
		free(recv_buf);
	if (spollfd != NULL)
		free(spollfd);

	/* scif_close : closes the end pt, when successful returns 0 */
	if (scif_close(epd) != 0) {
		printf("scif_close failed with error %d\n", get_curr_status());
		exit(1);
	}
	if (errno == 0)
		printf("======== Program Success ========\n");
	else
		printf("======== Program Failed ========\n");

	return errno;
}

