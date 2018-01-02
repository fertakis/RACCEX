/*
 * client.c
 * Client application for Remote Intel PHI Execution
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
#include "client.h"
#include "common.pb-c.h"
#include "protocol.h"

int init_client_connection(const char *s_ip, const char *s_port)
{
    int sd, port;
    struct hostent *hp;
    struct sockaddr_in sa;

    /* Create TCP/IP socket, used as main chat channel */
    if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }
    fprintf(stderr, "Created TCP socket\n");

    /* Look up remote hostname on DNS */
    if ( !(hp = gethostbyname(s_ip))) {
        printf("DNS lookup failed for host %s\n", s_ip);
        exit(1);
    }

    /* Connect to remote TCP port */
    sa.sin_family = AF_INET;
    sa.sin_port = htons(atoi(s_port));
    memcpy(&sa.sin_addr.s_addr, hp->h_addr, sizeof(struct in_addr));
    fprintf(stderr, "Connecting to remote host...\n"); fflush(stderr);
    if (connect(sd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
        perror("connect");
        exit(1);
    }
    fprintf(stderr, "Connected.\n");

    return sd;
}

void init_params(unitofwork *uow) {
    uow->endp = -1;
}

void establish_connection(unitofwork *uow) {
    char *server, *server_port;

    if(uow->endp < 0)
    {
        if(get_server_connection_config(&server, &server_port) !=0) {
            sprintf(server, DEFAULT_SERVER_IP);
            sprintf(server_port, DEFAULT_SERVER_PORT);
            printf("Could not get env vars, using defaults: %s:%s\n", server, server_port);

        }
        uow->socket_fd = init_client_connection(server, server_port);
        printf("Connected to server %s on port %s...\n", server, server_port);
    }
}

int send_phi_cmd(int socket_fd, var ** args, size_t arg_cnt, int cmd_type)
{
    void *buf = NULL, *payload = NULL;
    size_t len;

    //printf("Preparing and sending Phi cmd..\n);"
    pack_phi_cmd(&payload, args, arg_cnt, cmd_type);
    
    len = serialise_message(&buf, PHI_CMD, payload);
    if(buf == NULL)
        return -1;
    send_message(socket_fd, buf, len);

    free(buf);

    return 0;
}

int get_phi_cmd_result(PhiCmd **result, int socket_fd)
{
	size_t len;
	void *buf = NULL, *payload = NULL, *deserialised_message=NULL;
	int res_code;
	
	printf("Waiting result from PHI Server...\n");
	len = receive_message(&buf, socket_fd);
	if(len > 0)
		deserialise_message(&deserialised_message, &payload, buf, len);
	else {
		fprintf(stderr, "Problem receiving server response.\n");
		exit(EXIT_FAILURE);
	}
	
	if(payload == NULL) {
		fprintf(stderr, "Problem deserialising message.\n");
		exit(EXIT_FAILURE);
	} else {
		*result = payload;
		res_code = *result->phi_result_code;
		printf("Server responded: \n| result code: %d\n", res_code);
		
		free_deserialised_message(deserialised_message);
	}
	
	if(buf != NULL)
		free(buf);
	return res_code;
}
