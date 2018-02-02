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

#include "common.h"
#include "protocol.h"
#include "process.h"
#include "common.pb-c.h"

int init_server_net(const char *port, struct sockaddr_in *sa) 
{
	int socket_fd;

	printf("Initialising server...\n");
	/* Create TCP/IP socket, used as main chat channel */
	if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}
	fprintf(stderr, "Created TCP socket\n");

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
	fprintf(stderr, "Bound TCP socket to port %s\n", port);


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
	void *msg=NULL, *payload=NULL, *result=NULL, *des_msg=NULL;
	//client_node *cur_client = NULL;
	uint32_t msg_length;
	thr_mng *client = arg; 

	for(;;) {
		msg_length = receive_message(&msg, client->sockfd);
		if (msg_length > 0)
			msg_type = deserialise_message(&des_msg, &payload, msg, msg_length);
		else {
			printf("\n--------------\nClient finished.\n\n");
			break;
		}


		printf("Processing message\n");
		switch (msg_type) {
			case PHI_CMD:
				arg_cnt = process_phi_cmd(&result, payload);
				resp_type = PHI_CMD_RESULT;
				break;
			default:
				printf("--------INVALID MESSAGE TYPE--------\n");
				break;
		}


		if (msg != NULL) {
			free(msg);
			msg = NULL;
		}
		if (des_msg != NULL) {
			free_deserialised_message(des_msg);
			des_msg = NULL;
			// payload should be invalid now
			payload = NULL;
		}

		if (resp_type != -1) {
			printf("Packing and Sending result\n");
			pack_phi_cmd(&payload, result, arg_cnt, PHI_CMD_RESULT);
			msg_length = serialise_message(&msg, resp_type, payload);
			send_message(client->sockfd, msg, msg_length);

			if (result != NULL) {
				// should be more freeing here...
				free(result);
				result = NULL;
			}
		}
		printf(">>\nMessage processed, cleaning up...\n<<\n");
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
	//pid_t client;
	thr_mng *client;

	if (argc > 2) {
		printf("Usage: server <local_port>\n");
		exit(EXIT_FAILURE);
	}

	if (argc == 1) {
		printf("No port defined, trying env vars\n");
		get_server_connection_config(&server, &server_port);
		local_port = server_port;
	} else {
		local_port = argv[1];
	}

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
		
		/*if((client = fork()) < 0)
		{
			perror("fork");
			exit(1);
		}
		else if (client == 0)
		{
			//client
			serve_client(client_sfd);
		}*/		
	}

	//should never be here
	return EXIT_FAILURE;
}
