/*
 * Copyright 2012-2017 Intel Corporation.
 *
 * This file is subject to the Intel Sample Source Code License. A copy
 * of the Intel Sample Source Code License is included.
 */

/* scif_accept.c : basic implementation of the connection APIs */


#include <scif.h>
#include "scif_tutorial.h"

#define LOCAL_PORT 2050
#define MSG_SIZE   1024

int main(int argc, char *argv[])
{
	scif_epd_t epd;
	scif_epd_t newepd;
	struct scif_portID portID;

	int i, no_bytes, backlog = 5;
	int msg_size, curr_size, block;
	int conn_port, req_port;
	char *send_buf = NULL, *recv_buf = NULL, *curr_addr, *end;

	/* initializing with default values, which can be overridden with
	 * user specified values
	 */
	req_port = LOCAL_PORT;
	msg_size = MSG_SIZE;
	block = 1;

	if (argc != 7) {
		printf("usage: ./scif_accept -l <local_port> -s <msg_size> "
						"-b <block/non-block 1/0>\n");
		exit(1);
	}
	req_port = atoi(argv[2]);
	msg_size = (int)strtol(argv[4], &end, 10);
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

	/* scif_listen : marks an end pt as listening end and returns, when successful
	 * returns 0. backlog is length of request queue: maximum no: of requests
	 * that can be pending
	 */
	if (scif_listen(epd, backlog) != 0) {
		printf("scif_listen failed with error %d\n", get_curr_status());
		exit(1);
	}

	/* scif_accept : accepts connection requests on listening end pt and creates a
	 * new end pt which connects to the peer end pt that initiated connection,
	 * when successful returns 0.
	 * SCIF_ACCEPT_SYNC blocks the call untill a connection is present
	 */
	if (scif_accept(epd, &portID, &newepd, SCIF_ACCEPT_SYNC) != 0) {
		printf("scif_accept failed with error %d\n", get_curr_status());
		exit(1);
	}
	printf("accepted connection request from node:%d port:%d\n", portID.node, portID.port);

	/* send & recv small data to verify the established connection
	 * scif_recv : receives data from a peer end pt. In blocking state, waits for the
	 * entire msg to be received. In non-blocking state, it receives only bytes which are
	 * available without waiting
	 */
	recv_buf = (char *)malloc(msg_size);
	if (recv_buf == NULL) {
		printf("mem allocation failed with errno : %d\n", get_curr_status());
		goto __end;
	}
	memset(recv_buf, 0x0, msg_size);
	curr_addr = recv_buf;
	curr_size = msg_size;
	while ((no_bytes = scif_recv(newepd, curr_addr, curr_size, block)) >= 0) {
		curr_addr = curr_addr + no_bytes;
		curr_size = curr_size - no_bytes;
		if(curr_size == 0)
		break;
	}
	if (no_bytes < 0) {
		printf("scif_recv failed with error %d\n", get_curr_status());
		goto __end;
	}

	/* scif_send : send messages between connected end pts. In blocking state, the call
	 * returns after sending entire msg unless interupted. In non-blocking state, it sends
	 * only those bytes that can be sent without waiting
	 */
	send_buf = (char *)malloc(msg_size);
	if (send_buf == NULL) {
		printf("mem allocation failed with error : %d\n", get_curr_status());
		goto __end;
	}
	memset(send_buf, 0xbc, msg_size);
	curr_addr = send_buf;
	curr_size = msg_size;
	while ((no_bytes = scif_send(newepd, curr_addr, curr_size, block)) >= 0) {
		curr_addr = curr_addr + no_bytes;
		curr_size = curr_size - no_bytes;
		if (curr_size == 0)
			break;
	}
	if (no_bytes < 0) {
		printf("scif_send failed with error %d\n", get_curr_status());
		goto __end;
	}

	/* Verify sent, received data. Data verification is optional/usage dependent */
	for (i = 0; i < (msg_size/sizeof(int)); i++) {
		if (((int*)send_buf)[i] != ((int*)recv_buf)[i]) {
			printf("data mismatch send_buf[%d] 0x%x recv_buf[%d] 0x%x\n",
				i, ((int*)send_buf)[i], i, ((int*)recv_buf)[i]);
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
	printf("scif_close success\n");

	if (errno == 0)
		printf("======== Program Success ========\n");
	else
		printf("======== Program Failed ========\n");

	return errno;
}

