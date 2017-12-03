/*
 * process.c
 * Server Infrastructure for Remote Intel PHI Execution
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
#include <scif.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "common.h"
#include "process.h"
#include "phi_errors.h"
#include "common.pb-c.h"

int process_phi_cmd(void **result, void *cmd_ptr, client_node *cur_client) {
	int phi_result = 0, int_res = 0,arg_count = 0;
	PhiCmd *cmd = cmd_ptr;
	uint64_t uint_res = 0; 
	void *extra_args = NULL, *res_data = NULL;
	size_t extra_args_size = 0, res_length = 0;
	var **res = NULL;
	var_type res_type;

	printf("Processing PHI_CMD\n");
	switch(cmd->type) {
		case GET_VERSION:
			printf("Executing get_driver_version...\n");
			//int_res = scif_get_driver_version();
			res_type = INT;
			break;
			/*get_client_handle(client_handle, client_list, cmd->int_args[0]);
			  uint_res = ((client_node *) *client_handle)->id;
			// cuInit() should have already been executed by the server 
			// by that point...
			//cuda_result = cuda_err_print(cuInit(cmd->uint_args[0]), 0);
			cuda_result = CUDA_SUCCESS;
			res_type = UINT;*/
		case OPEN:
			{
				printf("Executing scif_open() ... \n");
				scif_epd_t endp;
				if((endp = scif_open()) < 0)
				{
					perror("scif_open");
					phi_result = SCIF_OPEN_FAIL; 
				}
				else 
				{
					phi_result = SCIF_SUCCESS;
					res_type = INT;
					int_res = endp;
					cur_client = malloc_safe(sizeof(client_node));
					cur_client->id = endp;
					printf("current client initialised with endp = %d\n", cur_client->id);
				}
				break;
			}
		case CLOSE:
			printf("Executing scif_close() ... \n");
			//TODO: scif_close call goes here...
			if(scif_close((scif_epd_t)cmd->int_args[0])<0)
			{
				perror("scif_close");
				phi_result = SCIF_CLOSE_FAIL;
			} else 
			{
				phi_result = SCIF_SUCCESS;
				free(cur_client);
				cur_client = NULL;
			}
			break;
		case BIND:
			printf("Executing scif_bind() ... \n");
			//TODO: scif_bind call goes here...
			if((int_res = scif_bind((scif_epd_t)cmd->int_args[0], (uint16_t)cmd->int_args[1])) < 0)
			{
				perror("scif_bind");
				phi_result = SCIF_BIND_FAIL;
			}
			else 
			{
				phi_result = SCIF_SUCCESS;
				res_type = INT;
			}
			break;
		case LISTEN:
			printf("Executing scif_listen() ... \n");
			//TODO: scif_liste call goes here...
			if(scif_listen((scif_epd_t) cmd->int_args[0], cmd->int_args[1]) < 0)
			{
				perror("scif_listen");
				phi_result = SCIF_LISTEN_FAIL;
			}
			else 
			{
				phi_result = SCIF_SUCCESS;
			}
			break;
		case CONNECT:
			printf("Executing scif_connect() ... \n");
			//TODO: scif_connect call goes here...
			struct scif_portID *dst;
			dst = (struct scif_portID *)cmd->extra_args[0].data;
			printf("remote scif_portID: node=%d and port=%d \n", dst->node, dst->port);
			
			if((int_res = scif_connect((scif_epd_t)cmd->int_args[0], dst)) < 0)
			{
				perror("scif_connect");
				phi_result = SCIF_CONNECT_FAIL;
			}
			else 
			{	
				phi_result = SCIF_SUCCESS;
				res_type = INT;
			}
			//free allocated memory
			free(dst);
			break;
		case ACCEPT:
			printf("Executing scif_accept() ... \n");
			//TODO: scif_accept call goes here...
			struct scif_portID *peer;
			break;
		case SEND:
			printf("Executing scif_send() ... \n");
			//TODO: scif_send call goes here...
			break;
		case RECV:
			printf("Executing scif_recv() ... \n");
			//TODO: scif_recv call goes here...
			break;
		case REGISTER:
			printf("Executing scif_register() ... \n");
			//TODO: scif_register call goes here...
			break;
		case UNREGISTER:
			printf("Executing scif_unregister() ... \n");
			//TODO: scif_unregister call goes here...
			break;
		case MMAP:
			printf("Executing scif_mmap() ... \n");
			//TODO: scif_mmap call goes here...
			break;
		case MUNMAP:
			printf("Executing scif_munmap() ... \n");
			//TODO: scif_munmap call goes here...
			break;
		case READ_FROM:
			printf("Executing scif_read_from() ... \n");
			//TODO: scif_read_from call goes here...
			break;
		case WRITE_TO:
			printf("Executing scif_write_to() ... \n");
			//TODO: scif_write_to call goes here...
			break;
		case VREAD_FROM:
			printf("Executing scif_vread_from() ... \n");
			//TODO: scif_vread_from call goes here...
			break;
		case VWRITE_TO:
			printf("Executing scif_vwrite_to) ... \n");
			//TODO: scif_vwrite_to call goes here...
			break;
		case FENCE_MARK:
			printf("Executing scif_fence_mark() ... \n");
			//TODO: scif_fence_mark call goes here...
			break;
		case FENCE_WAIT:
			printf("Executing scif_fence_wait() ... \n");
			//TODO: scif_fence_wait call goes here...
			break;
		case FENCE_SIGNAL:
			printf("Executing scif_fence_signal) ... \n");
			//TODO: scif_fence_signal call goes here...
			break;
		case GET_NODE_IDS:
			printf("Executing scif_get_node_ids) ... \n");
			//TODO: scif_get_node_ids call goes here...
			break;
		case POLL:
			printf("Executing scif_poll() ... \n");
			//TODO: scif_poll call goes here...
			break;
		case LIB_INIT:
			printf("Executing scif_lib_init() ... \n");
			//TODO: scif_lib_init call goes here...
			break;	
	}

	if(res_type == INT)
	{
		res_length = sizeof(int);
		res_data = &int_res;
	} else if (extra_args_size != 0)
	{
		res_type = BYTES;
		res_length = extra_args_size;
		res_data = extra_args;
	}
	
	if(res_length > 0 ) {
		res = malloc_safe(sizeof(var *)*2);
		res[1] = malloc_safe(sizeof(var));
		res[1]->type = res_type;
		res[1]->elements = 1;
		res[1]->length = res_length;
		res[1]->data = malloc_safe(res_length);
		memcpy(res[1]->data, res_data, res_length);
		arg_count = 2 ;
	} else {
		res = malloc_safe(sizeof(var *));
		arg_count = 1;
	}
	
	res[0] = malloc_safe(sizeof(var));
	res[0]->type = INT;
	res[0]->elements = 1;
	res[0]->length = sizeof(int);
	res[0]->data = malloc_safe(res[0]->length);
	memcpy(res[0]->data, &phi_result, res[0]->length);

	*result = res;	

	if (extra_args != NULL)
		free(extra_args);

	return arg_count;
}
