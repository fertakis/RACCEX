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

int init_client_connection(const char *s_ip, const char *s_port)
{
    int sd, port;
    char *hostname;
    struct hostent *hp;
    struct sockaddr_in sa;

    /* Create TCP/IP socket, used as main chat channel */
    if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }
    fprintf(stderr, "Created TCP socket\n");

    /* Look up remote hostname on DNS */
    if ( !(hp = gethostbyname(hostname))) {
        printf("DNS lookup failed for host %s\n", hostname);
        exit(1);
    }

    /* Connect to remote TCP port */
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    memcpy(&sa.sin_addr.s_addr, hp->h_addr, sizeof(struct in_addr));
    fprintf(stderr, "Connecting to remote host... "); fflush(stderr);
    if (connect(sd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
        perror("connect");
        exit(1);
    }
    fprintf(stderr, "Connected.\n");

    return sd;
}

void init_params(unitofwork *uow) {
    uow->id = -1;
}

void establish_connection(unitofwork *uow) {
    char server_ip[16], server_port[6];

    if(uow->id < 0)
    {
        if(get_server_connection_config(server_ip, server_port) !=0) {
            sprintf(server_ip, DEFAULT_SERVER_IP);
            sprintf(server_port, DEFAULT_SERVER_PORT);
            printf("Could not get env vars, using defaults: %s:%s\n", server_ip, server_port);

        }
        uow->socket_fd = init_client_connection(server_ip, server_port);
        printf("Connected to server %s on port %s...\n", server_ip, server_port);
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

void get_phi_cmd_result(void **result, int socket_fd)
{
	PhiCmd *cmd;
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
		cmd = payload;
		res_code = cmd->int_args[0];
		printf("Server responded: \n| result code: %d\n", res_code); 
		if (cmd->n_uint_args > 0) {
			*result = malloc_safe(sizeof(uint64_t));
			memcpy(*result, &cmd->uint_args[0], sizeof(uint64_t));
			printf("| result: 0x% \n", *(uint64_t *) *result);
		} else if (cmd->n_extra_args > 0) {
			*result = malloc_safe(cmd->extra_args[0].len);
			memcpy(*result, cmd->extra_args[0].data, cmd->extra_args[0].len);
			printf("| result: (bytes)\n");
		}
		free_deserialised_message(deserialised_message);
	}
	
	if(buf != NULL)
		free(buf);
	return res_code;
}
