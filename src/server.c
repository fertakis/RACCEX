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

int init_server_net(const char *port, struct addrinfo *addr) 
{
	int socket_fd,
	struct sockaddr_in sa;
	/* Create TCP/IP socket, used as main chat channel */
	if ((socket_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}
	fprintf(stderr, "Created TCP socket\n");

	/* Bind to a well-known port */
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(TCP_PORT);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(socket_fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
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


int init_server(char *port, struct addrinfo *addr, void **free_list, void **busy_list) {
	int socket_fd;

	printf("Initializing server...\n");
	socket_fd = init_server_net(port, addr);
	discover_cuda_devices(free_list, busy_list);

	return socket_fd;
}

int main(int argc, char *argv[]) {
	int server_socket_fd, client_socket_fd, msg_type, resp_type, arg_cnt;
	struct sockaddr_in client_addr;
	struct addrinfo local_addr;
	char server_ip[16] /* IPv4 */, server_port[6], *local_port,
		 client_host[NI_MAXHOST], client_serv[NI_MAXSERV];
	socklen_t s;
	void *msg=NULL, *payload=NULL, *result=NULL, *dec_msg=NULL,
		 *free_list=NULL, *busy_list=NULL, *client_list=NULL, *client_handle=NULL;
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

	server_socket_fd = init_server(local_port, &local_addr, &free_list, &busy_list);
	printf("\nServer listening on port %s for incoming connections...\n", local_port);

	for (;;) {
		resp_type = -1;
		s = sizeof(client_addr);
		client_socket_fd = accept(server_socket_fd, (struct sockaddr*)&client_addr, &s);
		if (client_socket_fd < 0) {
			perror("accept failed");
			exit(EXIT_FAILURE);
		}

		printf("\nConnection accepted ");
		if (getnameinfo((struct sockaddr*)&client_addr, s,
					client_host, sizeof(client_host), client_serv,
					sizeof(client_serv), NI_NUMERICHOST | NI_NUMERICSERV) == 0)
			printf("from client @%s:%s\n", client_host, client_serv);
		else
			printf("from unidentified client");

		for(;;) {
			msg_length = receive_message(&msg, client_sock_fd);
			if (msg_length > 0)
				msg_type = decode_message(&dec_msg, &payload, msg, msg_length);

			printf("Processing message\n");
			switch (msg_type) {
				case PHI_CMD:
					arg_cnt = process_cuda_cmd(&result, payload, free_list, busy_list, &client_list, &client_handle);
					resp_type = PHI_CMD_RESULT;
					break;
				case PHI_DEVICE_QUERY:
					process_cuda_device_query(&result, free_list, busy_list);
					resp_type = PHI_DEVICE_LIST;
					break;
			}

			print_clients(client_list);
			print_cuda_devices(free_list, busy_list);

			if (msg != NULL) {
				free(msg);
				msg = NULL;
			}
			if (dec_msg != NULL) {
				free_decoded_message(dec_msg);
				dec_msg = NULL;
				// payload should be invalid now
				payload = NULL;
			}

			if (resp_type != -1) {
				gdprintf("Sending result\n");
				pack_cuda_cmd(&payload, result, arg_cnt, CUDA_CMD_RESULT);
				msg_length = encode_message(&msg, resp_type, payload);
				send_message(client_sock_fd, msg, msg_length);

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

			if (get_client_status(client_handle) == 0) {
				// TODO: freeing
				printf("\n--------------\nClient finished.\n\n");
				break;
			}
		}
	}
	close(client_socket_fd);

	if (free_list != NULL)
		free_cdn_list(free_list);

	if (busy_list != NULL)
		free_cdn_list(busy_list);

	if (client_list != NULL)
		free_cdn_list(client_list);

	return EXIT_FAILURE;
}
