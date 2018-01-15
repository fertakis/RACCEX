/*
 * Copyright 2012-2017 Intel Corporation.
 *
 * This file is subject to the Intel Sample Source Code License. A copy
 * of the Intel Sample Source Code License is included.
 */

/* scif_connect_multiple.c : request multiple connections to remote node */

#include <scif.h>
#include "scif_tutorial.h"

#define MAX_EPD 5 /* usage dependent */
#define PEER_PORT 2050

static int count;
int verify_connection(scif_epd_t, int, int);

int main(int argc, char* argv[])
{
	scif_epd_t epd[MAX_EPD];
	struct scif_portID portID;

	int j, i, err, tries, no_conn;
	int msg_size, start_port, conn_port;

	if (argc != 9) {
		printf("usage: ./scif_connect_multiple -n <peer_node> -l <starting_port_no> "
				"-c <no_connections> -s <msg_size>\n");
		exit(1);
	}
	portID.node = atoi(argv[2]);
	start_port = atoi(argv[4]);
	no_conn = atoi(argv[6]);
	msg_size = atoi(argv[8]);
	if (msg_size <= 0 || msg_size > INT_MAX) {
		printf("not valid msg_size\n");
		exit(1);
	}
	portID.port = PEER_PORT;

	if (no_conn <= 0 || no_conn > MAX_EPD) {
		printf("Min: 1 connection, Max allowed connection requests : %d\n", MAX_EPD);
		exit(1);
	}

	count = 0;
	for (i = 0; i < no_conn; i++) {
		tries = 20;
		printf("--connection %d--\n", i);
		if ((epd[i] = scif_open()) == SCIF_OPEN_FAILED) {
			printf("scif_open failed with error %d\n", (int)epd[i]);
			exit(1);
		}

		if ((conn_port = scif_bind(epd[i], start_port + i)) < 0) {
			printf("scif_bind failed with error %d\n", conn_port);
			exit(1);
		}
		printf("scif_bind to port %d success\n", conn_port);

__retry:
		if (scif_connect(epd[i], &portID) < 0) {
			if ((get_curr_status() == ECONNREFUSED) && (tries > 0)) {
				printf("connection[%d] to node %d failed : trial %d\n",
								i, portID.node, tries);
				tries--;
#ifndef _WIN32
				sleep(1);
#else
				Sleep(1000);
#endif
				goto __retry;
			}
			printf("scif_connect failed with error : %d\n", get_curr_status());
			break;
		}
		printf("scif_connect success\n");

		if ((err = verify_connection(epd[i], msg_size, no_conn)) != 0)
			printf("data verification failed on epd[%d]\n", i);
	}

	/* scif_close : closes the end pt, when successful returns 0
	 * closes end points for established connections
	 */
	for (j = 0; j < i; j++) {
		if (scif_close(epd[j]) != 0) {
			printf("scif_close[%d] failed with error %d\n", j, get_curr_status());
			goto __end;
		}
	}
	printf("scif_close success\n");
	errno = 0;

__end:
	/* scif_connect failure when i == 2 is a forced error case
	 * due to closing of listening end pt on accepting side
	 */
	if ((errno == 0) || (i == 2))
		printf("========= Program Success ========\n");
	else
		printf("======== Program Failed ========\n");

	return errno;
}

int verify_connection(scif_epd_t epd, int msg_size, int no_conn)
{
	int i, no_bytes, block, ret = 0;
	char *send_buf = NULL, *recv_buf = NULL;

	/* setting default transfer to be BLOCKING mode */
	block = 1;

	/* send & recv data to verify the connection
	 * scif_send : send messages between connected end pts.
	 * Last conn request is identified by data 0xad, which helps to skip indefinite
	 * blocking on accept() for further requests
	 */
	send_buf = (char *)malloc(msg_size);
	if (send_buf == NULL) {
		printf("mem allocation failed with error : %d\n", get_curr_status());
		ret = -1;
		goto __lend;
	}
	if (count++ == no_conn-1)
		memset(send_buf, 0xad, msg_size);
	else
		memset(send_buf, 0xbc, msg_size);

	while ((no_bytes = scif_send(epd, send_buf, msg_size, block)) <= 0) {
		if (no_bytes < 0) {
			printf("scif_send failed with error %d\n", get_curr_status());
			ret = -1;
			goto __lend;
		}
	}
	printf("scif_send success\n");

	/* scif_recv : receives data from a peer end pt. In blocking state, waits for the
	 * entire msg to be received.
	 */
	recv_buf = (char *)malloc(msg_size);
	if (recv_buf == NULL) {
		printf("mem allocation failed with error : %d\n", get_curr_status());
		ret = -1;
		goto __lend;
	}
	memset(recv_buf, 0x0, msg_size);
	while ((no_bytes = scif_recv(epd, recv_buf, msg_size, block)) <= 0) {
		if (no_bytes < 0) {
			printf("scif_recv failed with error %d\n", get_curr_status());
			ret = -1;
			goto __lend;
		}
	}
	printf("scif_recv success\n");

	/* verify sent & received data : this is optional/usage dependent */
	for (i = 0; i < (msg_size/sizeof(int)); i++) {
		if (((int*)send_buf)[i] != ((int*)recv_buf)[i]) {
			printf("data mismatch send_buf[%d] 0x%x recv_buf[%d] 0x%x\n",
					i, ((int*)send_buf)[i], i, ((int*)recv_buf)[i]);
			ret = -1;
			goto __lend;
		}
	}

__lend:
	if (send_buf != NULL)
		free(send_buf);
	if (recv_buf != NULL)
		free(recv_buf);
	if (ret == 0)
		return 0;
	else
		return -1;
}

