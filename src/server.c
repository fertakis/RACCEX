/*
 * server.c
 * Server Deamon for Remote Intel PHI Execution
 *
 * Konstantinos Fertakis <kfertak@cslab.ece.ntua.gr>
 */

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "include/common.h"
#include "include/protocol.h"
#include "include/process.h"
#include "include/common.pb-c.h"

int init_server_net(const char *port, struct sockaddr_in *sa) 
{
	int socket_fd;

	ddprintf("Initialising server...\n");
	/* Create TCP/IP socket, used as main chat channel */
	if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}
	ddprintf("Created TCP socket\n");

	/* Bind to a well-known port */
	memset(sa, 0, sizeof(sa));
	sa->sin_family = AF_INET;
	sa->sin_port = htons(atoi(port));
	//sa->sin_port = htons(port);
	sa->sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(socket_fd, (struct sockaddr *)sa, sizeof(*sa)) < 0) {
		perror("bind");
		exit(1);
	}
	ddprintf("Bound TCP socket to port %s\n", port);


	/* Listen for incoming connections */
	if (listen(socket_fd, TCP_BACKLOG) < 0) {
		perror("listen");
		exit(1);
	}
	return socket_fd;
}

void *serve_client(void *arg)
{
	int  msg_type, resp_type = -1, arg_cnt;
	void *msg=NULL;
	Cookie *cookie = NULL;
	PhiCmd *result = NULL, *cmd = NULL;
	uint32_t msg_length;
 	int type;

	thr_mng *client = arg; 
	
	for(;;) {
#ifdef BREAKDOWN
		TIMER_RESET(&s_des);
		TIMER_RESET(&s_unpack);
		TIMER_RESET(&s_dur);
		TIMER_RESET(&s_ser);
		TIMER_RESET(&s_send);
		TIMER_RESET(&s_free);
#endif
		msg_length = receive_message(&msg, client->sockfd);
#ifdef BREAKDOWN
		TIMER_START(&s_des);
#endif

		if (msg_length > 0)
			msg_type = deserialise_message(&cookie, &cmd, msg, msg_length);
		else {
			printf("\n--------------\nClient finished.\n\n");
			break;
		}
#ifdef BREAKDOWN
		TIMER_STOP(&s_des);
#endif

		type = cmd->type;

		ddprintf("Processing message\n");
		switch (msg_type) {
			case PHI_CMD:
				arg_cnt = process_phi_cmd(&result, cmd);
				resp_type = PHI_CMD_RESULT;
				break;
			default:
				printf("--------INVALID MESSAGE TYPE--------\n");
				break;
		}

		ddprintf("free msg\n");
#ifdef BREAKDOWN
		TIMER_START(&s_free);
#endif	
		if (msg != NULL) {
			ddprintf("msg not null\n");
			free(msg);
			msg = NULL;
		}

		ddprintf("free cookie\n");
		if (cookie != NULL) {
			free_deserialised_message(cookie);
			cookie = NULL;
			// cmd should be invalid now
			cmd = NULL;
		}
#ifdef BREAKDOWN
		TIMER_STOP(&s_free);
#endif
		if (resp_type != -1) {
			ddprintf("Packing and Sending result\n");
#ifdef BREAKDOWN
			TIMER_START(&s_ser);
#endif
			msg_length = serialise_message(&msg, resp_type, result);
#ifdef BREAKDOWN
			TIMER_STOP(&s_ser);
			TIMER_START(&s_send);
#endif 
			send_message(client->sockfd, msg, msg_length);
#ifdef BREAKDOWN
			TIMER_STOP(&s_send);
#endif
			
			//breakdown
#ifdef BREAKDOWN
			if(type == WRITE_TO) { 
				fprintf(out_fd, "TIME DESERIALIZE: %llu us %lf sec\n", TIMER_TOTAL(&s_des), TIMER_TOTAL(&s_des)/1000000.0);
				fprintf(out_fd, "TIME UNPACK: %llu us %lf sec\n", TIMER_TOTAL(&s_unpack), TIMER_TOTAL(&s_unpack)/1000000.0);
				fprintf(out_fd, "TIME DURING: %llu us %lf sec\n", TIMER_TOTAL(&s_dur), TIMER_TOTAL(&s_dur)/1000000.0);
				fprintf(out_fd, "TIME FREEING: %llu us %lf sec\n", TIMER_TOTAL(&s_free), TIMER_TOTAL(&s_free)/1000000.0);
				fprintf(out_fd, "TIME SERIALIZE: %llu us %lf sec\n", TIMER_TOTAL(&s_ser), TIMER_TOTAL(&s_ser)/1000000.0);
				fprintf(out_fd, "TIME SEND CALL: %llu us %lf sec\n", TIMER_TOTAL(&s_send), TIMER_TOTAL(&s_send)/1000000.0);
			}
#endif
			ddprintf("about to free phicmd\n");
			if (result != NULL) {
				// should be more freeing here...
				if(result->int_args != NULL)
					free(result->int_args);
				if(result->uint_args != NULL)
					free(result->uint_args);
				if(result->u64int_args != NULL)
					free(result->u64int_args);
				if(result->extra_args != NULL) {
					int j;
					for(j=0; j< result->n_extra_args; j++)
						if(type != READ_FROM && type != POLL)
							free(result->extra_args[j].data);
					free(result->extra_args);
				}
				result = NULL;
			}
		}
		ddprintf(">>\nMessage processed, cleaning up...\n<<\n");
		if (msg != NULL) {
			free(msg);
			msg = NULL;
		}

	}
	close(client->sockfd);
	free(client);
	return NULL;
}

int main(int argc, char *argv[]) {
	int server_sfd, client_sfd;
	struct sockaddr_in sa;
	char *server, *server_port, *local_port, addrstr[INET_ADDRSTRLEN];
	socklen_t len;
	thr_mng *client;
#ifndef BREAKDOWN
	if (argc > 2) {
		printf("Usage: server <local_port>\n");
		exit(EXIT_FAILURE);
	}
#endif

//	if (argc == 1) {
		ddprintf("No port defined, trying env vars\n");
		get_server_connection_config(&server, &server_port);
		local_port = server_port;
//	} else {
//		local_port = argv[1];
//	}
#ifdef BREAKDOWN
	char path[200];
	strcpy(path,"/home/users/kfertak/racex_benchmarks/read_write/breakdown_results/run_server_racex_");
	strcat(path, argv[1]);
	strcat(path,"_");
	strcat(path, argv[2]);
	strcat(path, ".out");
	out_fd = fopen(path, "r+");
#endif
	initialise_addr_map_list(&maps);

	server_sfd = init_server_net(local_port, &sa);
	printf("\nServer listening on port %s for incoming connections...\n", local_port);

	for (;;) {
		fprintf(stderr, "Waiting for an incoming connection...\n");

		/* Accept an incoming connection */
		len = sizeof(struct sockaddr_in);
		if ((client_sfd = accept(server_sfd, (struct sockaddr *)&sa, &len)) < 0) {
			perror("accept");
			exit(1);
		}
		if (!inet_ntop(AF_INET, &sa.sin_addr, addrstr, sizeof(addrstr))) {
			perror("could not format IP address");
			exit(1);
		}
		fprintf(stderr, "Incoming connection from %s:%d\n",
				addrstr, ntohs(sa.sin_port));	

		/*Allocate memory for thread management struct */
		client = malloc_safe(sizeof(thr_mng));	
		if(client == NULL) { 
			printf("Problem allocating memory for new client\n");
			close(client_sfd);
			continue;
		}
		client->sockfd = client_sfd;

		if(pthread_create(&client->thread_id, NULL, serve_client, client) != 0 ) {
			perror("pthread_create");
			printf("error spawning thread for new client\n");
			close(client_sfd);
			continue;
		}

	}

	//should never be here
	return EXIT_FAILURE;
}
