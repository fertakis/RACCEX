/*
 * Copyright 2012-2017 Intel Corporation.
 *
 * This file is subject to the Intel Sample Source Code License. A copy
 * of the Intel Sample Source Code License is included.
 */

/* scif_accept_multiple.c : accept multiple connection requests on an end point
 * The requests are handled synchronously. It also demonstrates that closing a
 * listening end pt, rejects further connection requests
 */

#include <scif.h>
#include "scif_tutorial.h"

#define PORT_NO 2050
#define BACKLOG 5

int verify_connection(scif_epd_t, int, int);

int main(int argc, char *argv[])
{
	scif_epd_t epd;
	scif_epd_t newepd[BACKLOG];
	struct scif_portID portID;

	int i, j, err, close = 0;
	int msg_size, last = 0;
	int conn_port, req_port;

	if (argc != 5) {
		printf("usage: ./scif_accept_multiple -s <msg_size> -e <close epd 1/0>\n"
					" **setting -e option demonstrates the effect"
					" of closing listening epd**\n"
					" **need minimum 4 connection requests from peer"
					" to demonstrate this scenario**\n");
		exit(1);
	}
	msg_size = atoi(argv[2]);
	if (msg_size <= 0 || msg_size > INT_MAX) {
		printf("not valid msg size");
		exit(1);
	}
	close = atoi(argv[4]);
	req_port = PORT_NO;

	if ((epd = scif_open()) == SCIF_OPEN_FAILED) {
		printf("scif_open failed with error %d\n", (int)epd);
		exit(1);
	}

	if ((conn_port = scif_bind(epd, req_port)) < 0) {
		printf("scif_bind failed with error %d\n", conn_port);
		exit(1);
	}
	printf("scif_bind to port %d success\n", conn_port);

	j = 0;
	last = 0;
	err = scif_listen(epd, BACKLOG);
	while (1) {
		if (last == 1)
			break;

		/* Demonstrates the effect of closing listening end point.
		 * Chose to close the endpt at 3rd connection request, this value
		 * is demonstration specific. scif_accept should fail when
		 * listening end pt is closed.
		 */
		if (close == 1 && j == 2) {
			printf("closing listening epd\n");
			scif_close(epd);
		}

		printf("--connection %d--\n", j);

		if ((err = scif_accept(epd, &portID, &newepd[j], SCIF_ACCEPT_SYNC)) < 0) {
			printf("scif_accept failed with error : %d\n", get_curr_status());
			break;
		}
		printf("scif_accept success\n");

		last = verify_connection(newepd[j], msg_size, last);
		if (last == -1) {
			printf("data verification failed on newepd[%d]\n", j);
			last = 0;
		}
		j++;
	}

	/* scif_close : closes the end pt, when successful returns 0 */
	for (i = 0; i < j; i++) {
		if ((scif_close(newepd[i])) != 0) {
			printf("scif_close of newepd[%d] failed with error : %d\n", i, get_curr_status());
			goto __end;
		}
	}
	errno = 0;

__end:
	/* setting 'e' is a demonstration case, in this scenario,
	 * following scif_close will fail, ignore the error since
	 * eps is already closed above
	 */
	if (scif_close(epd) != 0)
		printf("scif_close failed with error : %d\n", errno);

	/* j == 2 is a forced error case */
	if ((errno == 0) || (j == 2))
		printf("======== Program Success ========\n");
	else
		printf("======== Program Failed ========\n");

	return errno;
}

int verify_connection(scif_epd_t newepd, int msg_size, int last)
{
	int i, no_bytes, block;
	char *send_buf = NULL, *recv_buf = NULL;

	/* Setting default transfer to be BLOCKING mode */
	block = 1;

	/* send & recv to verify the connection
	 * scif_recv : receives data from a peer end pt. In blocking state, waits
	 * for the entire msg to be received.
	 */
	recv_buf = (char *)malloc(msg_size);
	if (recv_buf == NULL) {
		printf("mem allocation failed with error : %d\n", get_curr_status());
		last = -1;
		goto __lend;
	}
	memset(recv_buf, 0x0, msg_size);
	while ((no_bytes = scif_recv(newepd, recv_buf, msg_size, block)) <= 0) {
		if (no_bytes < 0) {
			printf("scif_recv failed with error %d\n", get_curr_status());
			last = -1;
			goto __lend;
		}
	}
	printf("scif_recv success\n");

	/* send & recv data to verify the connection
	 * scif_send : send messages between connected end pts.
	 * The last conn request received is identified by data 0xad, which allows
	 * to skip indefinite blocking on accept() for incoming requests
	 */
	send_buf = (char *)malloc(msg_size);
	if (send_buf == NULL) {
		printf("mem allocation failed with error : %d\n", get_curr_status());
		last = -1;
		goto __lend;
	}
	if (memchr(recv_buf, 0xad, msg_size) != NULL) {
		memset(send_buf, 0xad, msg_size);
		/* set last = 1 for last connection request */
		last = 1;
	}
	else
		memset(send_buf, 0xbc, msg_size);

	while ((no_bytes = scif_send(newepd, send_buf, msg_size, block)) <= 0) {
		if (no_bytes < 0) {
			printf("scif_send failed with error %d\n", get_curr_status());
			last = -1;
			goto __lend;
		}
	}
	printf("scif_send success\n");

	/* verify sent & received data : this is optional or usage dependent */
	for (i = 0; i < (msg_size/sizeof(int)); i++) {
		if (((int*)send_buf)[i] != ((int*)recv_buf)[i]) {
			printf("data mismatch send_buf[%d] 0x%x recv_buf[%d] 0x%x\n",
					i, ((int*)send_buf)[i], i, ((int*)recv_buf)[i]);
			last = -1;
			goto __lend;
		}
	}

__lend:
	if (send_buf != NULL)
		free(send_buf);
	if (recv_buf != NULL)
		free(recv_buf);

	return last;
}
