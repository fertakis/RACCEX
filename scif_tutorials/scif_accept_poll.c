/*
 * Copyright 2012-2017 Intel Corporation.
 *
 * This file is subject to the Intel Sample Source Code License. A copy
 * of the Intel Sample Source Code License is included.
 */

/* scif_accept_poll.c : uses poll on incoming request queue, send & recv queues
 * for transfering large data in non-blocking mode
 */

#include <scif.h>
#include "scif_tutorial.h"

#define PAGE_SIZE 0x1000

int main(int argc, char *argv[])
{
	scif_epd_t epd;
	scif_epd_t newepd = 0;
	struct scif_portID portID;
	struct scif_pollepd spollfd;

	int i, err, no_bytes, backlog = 5;
	int msg_size, curr_size, block;
	int conn_port, req_port;
	char *send_buf = NULL, *recv_buf = NULL, *curr_buf;

	if (argc != 7) {
		printf("usage: ./scif_accept_poll -l <local_port> -s <no 4k pages>"
						" -b <block/non-block 1/0>\n");
		exit(1);
	}
	req_port = atoi(argv[2]);
	msg_size = atoi(argv[4]) * PAGE_SIZE;
	if (msg_size <= 0 || msg_size > INT_MAX) {
		printf("not valid msg size");
		exit(1);
	}
	block = atoi(argv[6]);

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
	printf("scif_bind to port %d success\n", conn_port);

	/* scif_listen : marks an end pt as listening end pt and listens to incoming
	 * connection requests, when successful returns 0
	 */
	if (scif_listen(epd, backlog) != 0) {
		printf("scif_listen failed with error %d\n", get_curr_status());
		exit(1);
	}

	/* scif_accept : accepts connection on listening end pt and creates a new
	 * end pt which connects to the peer end pt that initiated connection,
	 * when successful returns 0. poll() is used to determine when there is a
	 * connection request in the queue
	 */
	err = 1;
	while (err) {
		/* scif_get_fd : gets file descriptor for endpoint descriptor */
		spollfd.epd = epd;
		spollfd.events = SCIF_POLLIN;
		spollfd.revents = SCIF_POLLIN;
		if ((err = scif_poll(&spollfd, 1, 20)) < 0) {
			printf("poll failed with err %d\n", get_curr_status());
			exit(1);
		}
		if (((err = scif_accept(epd, &portID, &newepd, 0)) < 0) && (get_curr_status() != EAGAIN)) {
			printf("scif_accept failed with error %d\n", get_curr_status());
			exit(1);
		}
	}
	printf("scif_accept poll success\n");

	/* send & recv to verify the connection
	 * scif_recv : receives data from a peer end pt. Transfering large data in
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

	spollfd.epd = newepd;
	spollfd.events = SCIF_POLLIN;
	spollfd.revents = 0;
	while ((no_bytes = scif_recv(newepd, curr_buf, curr_size, block)) < curr_size) {
		if(no_bytes < 0) {
			printf("scif_recv failed with error %d\n", get_curr_status());
			goto __end;
		}
		curr_buf = curr_buf + no_bytes;
		curr_size = curr_size - no_bytes;
		printf("polling on recv queue: bytes remaining = %d\n", curr_size);
		scif_poll(&spollfd, 1, 20);
	}
	printf("scif_recv success\n");

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
	spollfd.events = SCIF_POLLOUT;
	spollfd.revents = SCIF_POLLOUT;
	no_bytes = 0;
	while ((no_bytes = scif_send(newepd, curr_buf, curr_size, block)) < curr_size) {
		if (no_bytes < 0) {
			printf("scif_send failed with error %d\n", get_curr_status());
			goto __end;
		}
		curr_size = curr_size - no_bytes;
		curr_buf = curr_buf + no_bytes;
		printf("polling on send queue: bytes remaining = %d\n", curr_size);
		scif_poll(&spollfd, 1, 20);
	}
	printf("scif_send success\n");
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

	/* scif_close : closes the end pt, when successful returns 0 */
	if ((scif_close(epd) != 0) || (scif_close(newepd) != 0)) {
		printf("scif_close failed with error %d\n", get_curr_status());
		exit(1);
	}
	if (errno == 0)
		printf("======== Program Success ========\n");
	else
		printf("======== Program Failed ========\n");

	return errno;
}
