#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#include "common.h"
#include "common.pb-c.h"
#include "protocol.h"
#include "client.h"
#include <scif.h>

int main(int argc, char *argv[]) {
	int client_sock_fd;
	char *server_ip, *server_port;
	size_t buf_size;
	void *buffer = NULL, *result = NULL;
	PhiCmd cmd1 = PHI_CMD__INIT,
	       cmd2 = PHI_CMD__INIT, cmd3 = PHI_CMD__INIT, cmd4 = PHI_CMD__INIT,
	       cmd5 = PHI_CMD__INIT;

	scif_epd_t endPoint;
	int scif_port_no, remote_portID;
	struct scif_portID *dst;

	if(argc > 3) {
		printf("Usage: client <server_ip> <server_port>\n");
		exit(EXIT_FAILURE);
	}

	if (argc == 1) {
		printf("No server ip or port defined, using defaults %s:%s\n", DEFAULT_SERVER_IP, DEFAULT_SERVER_PORT);
		server_ip = (char *) DEFAULT_SERVER_IP;
		server_port = (char *) DEFAULT_SERVER_PORT;
	} else if (argc == 2) {
		printf("No server port defined, using default %s\n", DEFAULT_SERVER_PORT);
		server_ip = argv[1];
		server_port = (char *) DEFAULT_SERVER_PORT;
	} else {
		server_ip = argv[1];
		server_port = argv[2];
	}

	client_sock_fd = init_client_connection(server_ip, server_port);
	printf("Connected to server %s on port %s...\n", server_ip, server_port);

	/**
	 * cmd1: scif_open()
	 **/	
	printf("\n* SCIF_OPEN()\n");
	cmd1.type = OPEN;
	cmd1.arg_count = 0 ;

	buf_size = serialise_message(&buffer, PHI_CMD, &cmd1);
	send_message(client_sock_fd, buffer, buf_size);

	free(buffer);
	get_phi_cmd_result(&result, client_sock_fd);
	endPoint = *(int *)result;	
	printf("endpoint is %d\nexiting..\n", endPoint);

	//close connection
	close(client_sock_fd);
	printf("Connection terminated...\n");
	//initialise new connection
	client_sock_fd = init_client_connection(server_ip, server_port);
	printf("Connection established...\n");

	/**
	 * cmd2: scif_bind()
	 **/
	printf("\n* SCIF_BIND()\n");
	cmd2.type = BIND;
	cmd2.arg_count = 2;
	cmd2.n_int_args = 2;
	cmd2.int_args = malloc_safe(sizeof(int)*cmd2.n_int_args);
	cmd2.int_args[0] = endPoint;
	cmd2.int_args[1] = 0; // In scif_bind(epd, pn), if pn is zero, a port number >= SCIF_PORT_RSVD is assigned and returned.

	buf_size = serialise_message(&buffer, PHI_CMD, &cmd2);
	send_message(client_sock_fd, buffer, buf_size);

	free(cmd2.int_args);
	get_phi_cmd_result(&result, client_sock_fd);
	scif_port_no = *(int *)result;	

	//close connection
	close(client_sock_fd);
	printf("Connection terminated...\n");
	//initialise new connection
	client_sock_fd = init_client_connection(server_ip, server_port);
	printf("Connection established...\n");

	/**
	 * cmd3: scif_listen()
	 **/
	printf("\n* SCIF_LISTEN()\n");
	cmd3.type = LISTEN;
	cmd3.arg_count = 2;
	cmd3.n_int_args = 2;
	cmd3.int_args = malloc_safe(sizeof(int)*cmd3.n_int_args);
	cmd3.int_args[0] = endPoint;
	cmd3.int_args[1] = 10; 

	buf_size = serialise_message(&buffer, PHI_CMD, &cmd3);
	send_message(client_sock_fd, buffer, buf_size);

	free(cmd3.int_args);
	get_phi_cmd_result(&result, client_sock_fd);
	if( *(int *)result == 0 )
		printf("Endpoint marked as listening endpoint\n");
	else
		printf("Problem while trying binding endpoint as a listenting endp\n");
	
	//close connection
	close(client_sock_fd);
	printf("Connection terminated...\n");
	
	//initialise new connection
	client_sock_fd = init_client_connection(server_ip, server_port);
	printf("Connection established...\n");
	
	/**
	 * cmd5: scif_connect()
	 **/
	printf("\n* SCIF_CONNECT()\n");
	cmd5.type = CONNECT;
	cmd5.arg_count = 2;
	
	cmd5.n_int_args = 1;
	cmd5.int_args = malloc_safe(sizeof(int)*cmd5.n_int_args);
	cmd5.int_args[0] = endPoint;
	
	cmd5.n_extra_args = 1; 
	cmd5.extra_args = malloc_safe(sizeof(*(cmd5.extra_args)) * cmd5.n_extra_args);
	
	dst = malloc_safe(sizeof(struct scif_portID));
	dst->node = 6;	
	dst->port = 1022;
	
	cmd5.extra_args[0].data = dst;
	cmd5.extra_args[0].len = sizeof(struct scif_portID);

	buf_size = serialise_message(&buffer, PHI_CMD, &cmd5);
	send_message(client_sock_fd, buffer, buf_size);

	free(cmd5.int_args);
	free(dst);
	free(cmd5.extra_args);
	free(buffer);

	get_phi_cmd_result(&result, client_sock_fd);
	remote_portID = *(int *)result;

	if( remote_portID >  0 )
		printf("connection established...\n");
	else
		printf("Problem while trying to connect to endpoint\n");

	//close connection
	close(client_sock_fd);
	printf("Connection terminated...\n");

	//initialise new connection
	client_sock_fd = init_client_connection(server_ip, server_port);
	printf("Connection established...\n");
	

	/**
	 * cmd4: scif_close()
	 **/
	printf("\n* SCIF_CLOSE()\n");
	cmd4.type = CLOSE;
	cmd4.arg_count = 1;
	cmd4.n_int_args = 1;
	cmd4.int_args = malloc_safe(sizeof(int)*cmd4.n_int_args);
	cmd4.int_args[0] = endPoint;

	buf_size = serialise_message(&buffer, PHI_CMD, &cmd4);
	send_message(client_sock_fd, buffer, buf_size);

	free(cmd4.int_args);
	get_phi_cmd_result(&result, client_sock_fd);
	if( *(int *)result == 0 )
		printf("Endpoint closed succesfully\n");
	else
		printf("Problem while trying to close endpoint\n");

	//close connection
	close(client_sock_fd);
	printf("Connection terminated...\n");

	return 0;
}
