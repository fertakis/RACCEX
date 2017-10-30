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
	sa->sin_port = htons(TCP_PORT);
	sa->sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(socket_fd, (struct sockaddr *)sa, sizeof(*sa)) < 0) {
		perror("bind");
		exit(1);
	}
	fprintf(stderr, "Bound TCP socket to port %d\n", TCP_PORT);


	/* Listen for incoming connections */
	if (listen(socket_fd, TCP_BACKLOG) < 0) {
		perror("listen");
		exit(1);
	}
	return socket_fd;
}



int main(int argc, char *argv[]) {
	int server_sfd, client_sfd, msg_type, resp_type, arg_cnt;
	struct sockaddr_in sa;
	char server_ip[16] /* IPv4 */, server_port[6], *local_port,client_host[NI_MAXHOST], client_serv[NI_MAXSERV], addrstr[INET_ADDRSTRLEN];
	socklen_t len;
	void *msg=NULL, *payload=NULL, *result=NULL, *des_msg=NULL ;
	uint32_t msg_length;

	if (argc > 2) {
		printf("Usage: server <local_port>\n");
		exit(EXIT_FAILURE);
	}

	if (argc == 1) {
		printf("No port defined, trying env vars\n");
		if (get_server_connection_config(server_ip, server_port) < 2)
			local_port = server_port;
		else {
			printf("Could not get env vars, using default %s\n", DEFAULT_SERVER_PORT);
			local_port = (char *) DEFAULT_SERVER_PORT;
		}
	} else {
		local_port = argv[1];
	}

	server_sfd = init_server_net(local_port, &sa);
	printf("\nServer listening on port %s for incoming connections...\n", local_port);

	for (;;) {
		resp_type = -1;
		
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

		for(;;) {
			msg_length = receive_message(&msg, client_sfd);
			if (msg_length > 0)
				msg_type = deserialise_message(&des_msg, &payload, msg, msg_length);

			printf("Processing message\n");
			switch (msg_type) {
				case PHI_CMD:
					arg_cnt = process_phi_cmd(&result, payload);
					resp_type = PHI_CMD_RESULT;
					break;
				/*case PHI_DEVICE_QUERY:
					process_cuda_device_query(&result, free_list, busy_list);
					resp_type = PHI_DEVICE_LIST;
					break;*/
			}

			//print_clients(client_list);
			//print_cuda_devices(free_list, busy_list);

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
				send_message(client_sfd, msg, msg_length);

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

			/*CHECK TO SEE IF CLIENT IS FINISHED
			if (get_client_status(client_handle) == 0) {
				// TODO: freeing
				printf("\n--------------\nClient finished.\n\n");
				break;
			}*/
		}
	}
	close(client_sfd);

	/*if (client_list != NULL)
		free_cdn_list(client_list);*/

	return EXIT_FAILURE;
}
