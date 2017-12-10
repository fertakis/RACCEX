/*
 * common.c
 * Common methods and functionality for Remote Intel PHI Execution
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
#include "common.pb-c.h"

char *server_ip;
char *server_port;

inline void *malloc_safe_f(size_t size, const char *file, const int line) {
	void *ptr = NULL;

	ptr = malloc(size);
	if (ptr == NULL && size != 0) {
		fprintf(stderr, "[%s, %i] Memory allocation failed!\n", file, line);
		exit(EXIT_FAILURE);
	}

	return ptr;
}

int get_server_connection_config(char *server, char *server_port)
{
	int ret = 0;

	server = getenv("REMOTE_PHI_SERVER");
	server_port = getenv("REMOTE_PHI_PORT");

	if(server == NULL)
	{
		server = DEFAULT_SERVER_IP;
		printf("Enviromental Variable 'REMOTE_PHI_SERVER' not defined, using default server ip: %s\n", server);
		ret = 1 ;
	}
	if(server_port == NULL)
	{
		server_port = DEFAULT_SERVER_PORT;
		printf("Enviromental Variable 'REMOTE_PHI_PORT' not defined, using default server port: %s\n", server_port);
		ret = 1;
	}

	return ret;
}

int pack_phi_cmd(void **payload, var **args, size_t arg_count, int type) {
	PhiCmd *cmd;
	int i;

	printf("Packing PHI cmd...\n");

	cmd = malloc_safe(sizeof(PhiCmd));
	phi_cmd__init(cmd);

	cmd->type = type;
	cmd->arg_count = arg_count;

	for (i = 0; i < arg_count; i++) {	
		switch (args[i]->type) {
			case INT:
				cmd->n_int_args = args[i]->elements;
				cmd->int_args = args[i]->data;
				break;
			case UINT:
				cmd->n_uint_args = args[i]->elements;
				cmd->uint_args = args[i]->data;
				break;
			case STRING:
				cmd->n_str_args = args[i]->elements;
				cmd->str_args = args[i]->data;
				break;
			case BYTES:
				//cmd->n_extra_args = args[i]->elements;
				cmd->n_extra_args = 1;
				cmd->extra_args = malloc_safe(sizeof(*(cmd->extra_args)) * cmd->n_extra_args);
				cmd->extra_args[0].data = args[i]->data;
				cmd->extra_args[0].len = args[i]->length;
				break;
		}
	}

	*payload = cmd;
	return 0;
}
