/*
 * Copyright 2012-2017 Intel Corporation.
 *
 * This file is subject to the Intel Sample Source Code License. A copy
 * of the Intel Sample Source Code License is included.
 */

/* scif_connect.c : basic implementation of connection APIs */

#include <scif.h>
#include "scif_tutorial.h"

#ifdef HOST
#define PEER_NODE 1
#else
#define PEER_NODE 0
#endif

#define PEER_PORT  2050
#define LOCAL_PORT 2049
#define MSG_SIZE   1024

int main(int argc, char* argv[])
{
	scif_epd_t epd, epd1;
	struct scif_portID portID;

	int i, err, curr_size, tries = 20;
	int block, msg_size, no_bytes;
	int conn_port, req_port;
	char *send_buf = NULL, *recv_buf = NULL, *curr_addr;

	/* initializing with defualt values, which can be overridden with
	 * user specified values
	 */
	req_port = LOCAL_PORT;
	portID.node = PEER_NODE;
	portID.port = PEER_PORT;
	msg_size = MSG_SIZE;
	block = 1;

	if (argc != 11) {
		printf("usage: ./scif_connect -l <local_port> -n <peer_node[host/card 0/1]>"
			" -r <remote_port> -s <msg_size> -b <block/nonblock 1/0>\n");
		exit(1);
	}
	req_port = atoi(argv[2]);
	portID.node = atoi(argv[4]);
	portID.port = atoi(argv[6]);
	msg_size = atoi(argv[8]);
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
	printf("scif_bind to port %d success\n", conn_port);

	/* faulty case demonstration : bind multiple end pts to a port */
	if ((epd1 = scif_open()) == SCIF_OPEN_FAILED) {
		printf("scif_open failed with error %d\n", (int)epd1);
		exit(1);
	}
	if ((err = scif_bind(epd1, req_port)) < 0)
		printf("cannot bind multiple epd to a port : error %d\n", get_curr_status());

	/* faulty case demonstration : bind an end pt to multiple ports */
	if ((err = scif_bind(epd, req_port + 1)) < 0)
		printf("cannot bind epd to multiple ports : error %d\n", get_curr_status());

	/* scif_connect : initiate a connection to remote node, when successful returns
	 * the peer portID. Re-tries for 20 seconds and exits with error message
	 */
__retry:
	if ((err = scif_connect(epd, &portID)) < 0) {
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
	printf("conect to node %d success\n", portID.node);

	/* send & recv small data to verify the established connection
	 * scif_send : send messages between connected end pts. In blocking state, the call
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
	while ((no_bytes = scif_send(epd, curr_addr, curr_size, block)) >= 0) {
		curr_addr = curr_addr + no_bytes;
		curr_size = curr_size - no_bytes;
		if(curr_size == 0)
			break;
	}
	if (no_bytes < 0) {
		printf("scif_send failed with error %d\n", get_curr_status());
		goto __end;
	}

	/* scif_recv : receives data from a peer end pt. In blocking state, waits for the
	 * entire msg to be received. In non-blocking state, it receives only bytes which are
	 * available without waiting
	 */
	recv_buf = (char *)malloc(msg_size);
	if (recv_buf == NULL) {
		printf("mem allocation failed with error : %d\n", get_curr_status());
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
		printf("scif_recv failed with error %d\n", get_curr_status());
		goto __end;
	}

	/* verify sent, received data. Data verification is optional/usage dependent */
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
	if (scif_close(epd) != 0 || scif_close(epd1) != 0) {
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

