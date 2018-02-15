#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <scif.h>
#include <pthread.h>

#include "common.h"

static int local_port = -1, rem_port = -1, is_card = 0, msg_size = -1,
		   blk_flag = 0;
static scif_epd_t epd_s, epd_c;

void *server(void *buf)
{
	scif_epd_t epd_acc;
	char *recv_buf = NULL;

	epd_acc = bscif_endp_accept(epd_s, is_card, rem_port);

	// recv
	recv_buf = (char *)malloc(msg_size);
	if (recv_buf != NULL) {
		memset(recv_buf, 0x0, msg_size);
		bscif_recv_all(epd_acc, recv_buf, msg_size, blk_flag);
	} else {
		perror("Memory allocation failed");
	}	
	*(char **) buf = recv_buf;

	bscif_endp_close(epd_s);
	bscif_endp_close(epd_acc);

	return NULL;
}

void *client(void *buf)
{
	char *msg = NULL;

	if (bscif_endp_connect(epd_c, is_card, rem_port) < 0)
		exit(EXIT_FAILURE);

	// send
	msg = (char *)malloc(msg_size);
	if (msg != NULL) {
		memset(msg, 0xbc, msg_size);
		bscif_send_all(epd_c, msg, msg_size, blk_flag);
	} else {
		perror("Memory allocation failed");
	}
	*(char **) buf = msg;

	bscif_endp_close(epd_c);

	return NULL;
}

int main(int argc, char* argv[])
{
	int o;
	char *msg = NULL, *recv_buf = NULL;
	pthread_t client_thread, server_thread;

	while ((o = getopt (argc, argv, "l:n:r:s:b:")) != -1) {
		switch (o) {
			case 'l':
				local_port = atoi(optarg);
				break;
			case 'n':
				is_card = atoi(optarg);
				break;
			case 'r':
				rem_port = atoi(optarg);
				break;
			case 's':
				msg_size = atoi(optarg);
				if (msg_size <= 0 || msg_size > INT_MAX) {
					fprintf(stderr, "Invalid message size.");
					exit(EXIT_FAILURE);
				}
				break;
			case 'b':
				if (atoi(optarg) != 0)
					blk_flag = SCIF_SEND_BLOCK;
				break;
			case '?':
				if (strchr("lnrsb", optopt) != NULL)
					fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint(optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
					fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
			default:	
				printf("Usage: ./scif-bench -l <local_port> "
					"-n <peer_node[host/card 0/1]> -r <remote_port> "
					"-s <msg_size> -b <block/nonblock 1/0>\n");
				exit(EXIT_FAILURE);
				break;
		}
	}

	if (local_port < 0 || rem_port < 0 || msg_size < 0 || optind == 1) {
		printf("Usage: ./scif-bench -l <local_port> "
			"-n <peer_node[host/card 0/1]> -r <remote_port> "
			"-s <msg_size> -b <block/nonblock 1/0>\n");
		exit(EXIT_FAILURE);
	}

	epd_s = bscif_endp_init(rem_port, 1);
	epd_c = bscif_endp_init(local_port);
	
	bscif_timers_init();
	
	if (pthread_create(&server_thread, NULL, server, &recv_buf) < 0) {
		fprintf(stderr, "Could not create thread\n");
		goto __end;
	}

	if (pthread_create(&client_thread, NULL, client, &msg) < 0) {
		fprintf(stderr, "Could not create thread\n");
		goto __end;
	}
	
	pthread_join(server_thread, NULL);
	pthread_join(client_thread, NULL);

	if (!bscif_verify_data(msg, recv_buf, msg_size))
		fprintf(stderr, "Sent/Received data verification failed");

	bscif_timers_print();

__end:
	if (msg != NULL)
		free(msg);
	else
		exit(EXIT_FAILURE);

	if (recv_buf != NULL)
		free(recv_buf);
	else
		exit(EXIT_FAILURE);

	return 0;
}

