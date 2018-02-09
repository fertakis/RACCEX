/*
 * Copyright 2012-2017 Intel Corporation.
 *
 * This file is subject to the Intel Sample Source Code License. A copy
 * of the Intel Sample Source Code License is included.
 */

/* scif_accept_launch.c : This host program demonstrates dynamic allocation of an available port no:
 * for scif_bind to bind to an end pt (when a specific port no: is not specified). This port no:
 * needs to be communicated to card side program for it to initiate connection request.
 * Host programlaunches a remote process (through ssh) and passes the port_no to peer.
 * Also, demonstrates that when a connected end pt closes, the corresponding peer end pt is
 * also closed.
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

int main(int argc, char *argv[])
{
	scif_epd_t epd, newepd;
	struct scif_portID portID;

	pid_t pid;
	int i, err, no_bytes, curr_size, backlog = 5;
	int conn_port, msg_size, block , terminate, peer_node;
	char *send_buf = NULL, *recv_buf = NULL, *curr_addr;
	char node[128], param[128];

	if (argc != 9) {
		printf("usage ./scif_accept_launch_host -n <peer_node> -s <msg_size> -b <block/nonblock 1/0>"
			" -t <terminate_check 1/0>\n**setting -t option demonstrates the effect of"
			" closing a connected end pt**\n");
		exit(1);
	}
	peer_node = atoi(argv[2]);
	msg_size = atoi(argv[4]);
	if (msg_size <= 0 || msg_size > INT_MAX) {
		printf("not valid msg size");
		exit(1);
	}
	block = atoi(argv[6]);
	terminate = atoi(argv[8]);

	/* scif_open : creates an end point, when successful returns end pt descriptor */
	if ((epd = scif_open()) == SCIF_OPEN_FAILED) {
		printf("host: scif_open failed with error %d\n", (int)epd);
		exit(1);
	}

	/* scif_bind : when port_no is not specified, it dynamically allocates a port_no
	 * from available ports and binds it to the end pt
	 */
	if ((conn_port = scif_bind(epd, 0)) < 0) {
		printf("host: scif_bind failed with error %d\n", conn_port);
		exit(1);
	}
	printf("host: scif_bind to port %d success\n", conn_port);

	/* scif_listen : marks an end pt as listening end pt and listens to incoming
	 * connection requests, when successful returns 0
	 */
	if (scif_listen(epd, backlog) != 0) {
		printf("host: scif_listen failed with error %d\n", errno);
		exit(1);
	}

	/* child process launches the remote program on MIC and passes local port_no and
	 * other arguments to initiate a conn request on MIC. Parent process is blocked
	 * on accept() for incoming connection requests
	 */
	pid = fork();
	if(pid == 0) {
		sprintf(param, "-r %d -s %d -b %d -t %d", conn_port, msg_size, block, terminate);
		sprintf(node, "mic%d", (peer_node)-1);
		char *eargs[] = {"ssh", node,
				 "sh", "/tmp/launch.sh", param,
				 NULL};
		execve("/usr/bin/ssh", eargs, NULL);
		_exit(0);
	}
	else {
		/* scif_accept : accepts connection on listening end pt and creates a new
		 * end pt which connects to the peer end pt that initiated connection,
		 * when successful returns 0
		 */
		if (scif_accept(epd, &portID, &newepd, SCIF_ACCEPT_SYNC) != 0) {
			printf("host: scif_accept failed with error %d\n", errno);
			exit(1);
		}

		/* send & recv to verify the connection
		 * scif_recv : receives data from a peer end pt. In blocking state,
		 * waits for the entire msg to be received. In non-blocking state,
		 * it receives only bytes which are available without waiting
		 */
		recv_buf = (char *)malloc(msg_size);
		if (recv_buf == NULL) {
			printf("mem allocation failed with error : %d\n", errno);
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
			printf("host: recv failed with error %d\n", errno);
			goto __end;
		}
		printf("host: scif_recv success\n");

		/* send & recv data to verify the connection
		 * scif_send : send messages between connected end pts. In blocking state,
		 * it returns after sending entire msg unless interupted. In non-blocking
		 * state, it sends only those bytes that can be sent without waiting
		 */
		send_buf = (char *)malloc(msg_size);
		if (send_buf == NULL) {
			printf("mem allocation failed with error : %d\n", errno);
			goto __end;
		}
		memset(send_buf, 0xbc, msg_size);
		curr_addr = send_buf;
		curr_size = msg_size;
		while ((no_bytes = scif_send(newepd, curr_addr, curr_size, block)) >= 0) {
			curr_addr = curr_addr + no_bytes;
			curr_size = curr_size - no_bytes;
			if(curr_size == 0)
				break;
		}
		if (no_bytes < 0) {
			perror("host: send failed with error");
			/* check for a valid FD on newepd, if terminate = 1,
			 * i.e; closing remote connected end pt, scif_get_fd
			 * should fail because corresponding newepd is also
			 * closed.
			 * scif_get_fd gets the file descriptor for end pt.
			 */
			err = scif_get_fd(newepd);
			printf("host: scif_get_fd failed with error : %d returned %d\n", errno, err);
			goto __end;
		}
		printf("host: scif_send success\n");

		/* verify sent & received data : this is optional/usage dependent */
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
			printf("host: scif_close failed with error %d\n", errno);
			exit(1);
		}
		printf("host: scif_close success\n");
	} //parent end

	/* terminate == 1 is a forced error case */
	if ((errno == 0) || (terminate == 1))
		printf("======== host: Program Success ========\n");
	else
		printf("======== host: Program Failed ========\n");

	return errno;
}

