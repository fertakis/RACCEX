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
#include <scif_ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "common.h"
#include "process.h"
#include "phi_errors.h"
#include "common.pb-c.h"

#define DEVICE_NODE "/dev/mic/scif"

	static int
scif_get_driver_version(void)
{
	int scif_version;
	scif_epd_t fd;

	if ((fd = open(DEVICE_NODE, O_RDWR)) < 0)
		return -1;
	scif_version = ioctl(fd, SCIF_GET_VERSION);
	if (scif_version < 0) {
		close(fd);
		return -1;
	}
	close(fd);
	return scif_version;
}

int exec_scif_get_driver_version(int *version)
{
	int ret;
	if((*version = scif_get_driver_version()) < 0)
	{
		perror("scif_get_driver_version");
		ret = SCIF_GET_DRIVER_VERSION_FAIL;
	}
	else
		ret = SCIF_SUCCESS;
	return ret;
}
int exec_scif_open(scif_epd_t *endp, client_node **cur_client)
{
	scif_epd_t t_endp;
	int ret;

	if((t_endp = scif_open()) < 0)
	{
		perror("scif_open");
		ret = SCIF_OPEN_FAIL; 
	}
	else 
	{
		ret = SCIF_SUCCESS;
		*endp = t_endp;
		*cur_client = (client_node *)malloc_safe(sizeof(client_node));
		(*cur_client)->id = *endp;
		printf("current client initialised with endp = %d\n", (*cur_client)->id);
	}
	return ret; 
}

int exec_scif_close(scif_epd_t endp, client_node **cur_client)
{
	int ret;

	if(scif_close(endp)<0)
	{
		perror("scif_close");
		ret  = SCIF_CLOSE_FAIL;
	} else 
	{
		ret = SCIF_SUCCESS;
		free(*cur_client);
		*cur_client = NULL;
	}
	return ret; 
}

int exec_scif_bind(scif_epd_t endp, uint16_t pn, int *portno)
{
	int ret;
	printf("executing scif_bind(%d, %d)\n",endp, pn);
	if((*portno = scif_bind(endp , pn)) < 0)
	{
		perror("scif_bind");
		ret = SCIF_BIND_FAIL;
	}
	else 
		ret = SCIF_SUCCESS;
	printf("ret=%d\n", *portno);
	return ret; 
}

int exec_scif_listen(scif_epd_t endp, int backlog)
{
	int ret;
	if(scif_listen(endp, backlog) < 0)
	{
		perror("scif_listen");
		ret = SCIF_LISTEN_FAIL;
	}
	else 
		ret = SCIF_SUCCESS;
	return ret;
}

int exec_scif_connect(scif_epd_t endp, struct scif_portID *dst, int *portID)
{
	int ret; 
	if((*portID = scif_connect(endp, dst)) < 0)
	{
		perror("scif_connect");
		ret = SCIF_CONNECT_FAIL;
	}
	else 	
		ret = SCIF_SUCCESS;

	return ret;
}

int exec_scif_accept(scif_epd_t endp, struct scif_portID *peer, 
		scif_epd_t *newepd, int flags)
{
	int ret;
	if(scif_accept(endp, peer, newepd, flags) < 0)
	{
		perror("scif_accept");
		ret = SCIF_ACCEPT_FAIL;
	}
	else
		ret = SCIF_SUCCESS;
	return ret; 
}

int exec_scif_send(scif_epd_t endp, void **msg, int len, int flags, 
		int *send_count)
{
	int ret;
	if((*send_count = scif_send(endp, *msg, len, flags)) < 0)
	{
		perror("scif_send");
		ret = SCIF_SEND_FAIL;
	}
	else
		ret = SCIF_SUCCESS;
	return ret; 
}

int exec_scif_recv(scif_epd_t endp, void *msg, int len, int flags, 
		int *read_count)
{
	int ret; 

	if((*read_count = scif_recv(endp, msg, len, flags)) < 0)
	{
		perror("scif_recv");
		ret = SCIF_RECV_FAIL;
	}
	else
		ret = SCIF_SUCCESS;
	return ret;
}

int exec_scif_register(scif_epd_t endp, void **addr, size_t len, 
		off_t offset, int prot_flags, int map_flags, 
		off_t *resulted_off_t)
{
	int ret; 

	if((*resulted_off_t = scif_register(endp, *addr, len, offset, prot_flags, map_flags)) < 0)
	{
		perror("scif_register");
		ret = SCIF_REGISTER_FAIL;
	}
	else
		ret = SCIF_SUCCESS;
	return ret;
}

int exec_scif_unregister(scif_epd_t endp, off_t offset, size_t len)
{
	int ret; 

	if(scif_unregister(endp, offset, len) < 0)
	{
		perror("scif_unregister");
		ret = SCIF_UNREGISTER_FAIL;
	}
	else
		ret = SCIF_SUCCESS;
	return ret;
}


int exec_scif_mmap(void *addr, size_t len, int prot_flags, int map_flags, scif_epd_t epd, 
		off_t offset, void *result)
{
	int ret = -2;

	//TODO: To be implemented...

	return ret;
}

int exec_scif_munmap(void *addr, size_t len)
{
	int ret = -2;

	//TODO: To be implemented...

	return ret;
}

int exec_scif_readfrom(scif_epd_t epd, off_t loffset, size_t len, off_t roffset, int rma_flags)
{
	int ret; 

	if(scif_readfrom(epd, loffset, len, roffset, rma_flags) < 0)
	{
		perror("scif_readfrom");
		ret = SCIF_READ_FROM_FAIL;
	}
	else
		ret = SCIF_SUCCESS;

	return ret;
}

int exec_scif_writeto(scif_epd_t epd, off_t loffset, size_t len, off_t roffset, int rma_flags)
{
	int ret; 

	if(scif_writeto(epd, loffset, len, roffset, rma_flags) < 0)
	{
		perror("scif_writeto");
		ret = SCIF_WRITE_TO_FAIL;
	}
	else
		ret = SCIF_SUCCESS;

	return ret;
}

int exec_scif_vreadfrom(scif_epd_t epd, void *addr, size_t len, off_t offset, int rma_flags)
{
	int ret; 

	if(scif_vreadfrom(epd, addr, len, offset, rma_flags) < 0)
	{
		perror("scif_vreadfrom");
		ret = SCIF_VREAD_FROM_FAIL;
	}
	else
		ret = SCIF_SUCCESS;

	return ret;
}

int exec_scif_vwriteto(scif_epd_t epd, void *addr, size_t len, off_t offset, int rma_flags)
{
	int ret; 

	if(scif_vwriteto(epd, addr, len, offset, rma_flags) < 0)
	{
		perror("scif_vwriteto");
		ret = SCIF_VWRITE_TO_FAIL;
	}
	else
		ret = SCIF_SUCCESS;

	return ret;
}

int exec_scif_fence_mark(scif_epd_t epd, int flags, int *mark)
{
	int ret; 

	if(scif_fence_mark(epd, flags, mark) < 0)
	{
		perror("scif_fence_mark");
		ret = SCIF_FENCE_MARK_FAIL;
	}
	else
		ret = SCIF_SUCCESS;

	return ret;
}

int exec_scif_fence_wait(scif_epd_t epd, int mark)
{
	int ret; 

	if(scif_fence_wait(epd, mark) < 0)
	{
		perror("scif_fence_wait");
		ret = SCIF_FENCE_WAIT_FAIL;
	}
	else
		ret = SCIF_SUCCESS;

	return ret;
}

int exec_scif_fence_signal(scif_epd_t epd, off_t loff, uint64_t lval, off_t roff, uint64_t rval,
		int flags)
{
	int ret; 

	if(scif_fence_signal(epd, loff, lval, roff, rval, flags) < 0)
	{
		perror("scif_fence_signal");
		ret = SCIF_FENCE_SIGNAL_FAIL;
	}
	else
		ret = SCIF_SUCCESS;

	return ret;
}

int exec_scif_get_nodeIDs(uint16_t *nodes, int len, uint16_t *self, int *online_nodes)
{
	int ret; 

	if((*online_nodes = scif_get_nodeIDs(nodes, len, self)) < 0)
	{
		perror("scif_get_nodeIDs");
		ret = SCIF_GET_NODE_IDS_FAIL;
	}
	else
		ret = SCIF_SUCCESS;
	return ret; 
}

int exec_scif_poll(struct scif_pollepd *epds, unsigned int nepds, long timeout, int *poll_res)
{
	int ret;

	if((*poll_res = scif_poll(epds, nepds, timeout)) < 0 )
	{
		perror("scif_poll");
		ret = SCIF_POLL_FAIL;
	}
	else
		ret = SCIF_SUCCESS;
	return ret;
}

int process_phi_cmd(void **result, void *cmd_ptr, client_node  **cur_client) {
	int phi_result = 0, *int_res = NULL, int_res_count = 0, 
	    uint_res_count = 0 , arg_count = 1, *errorno = NULL;
	PhiCmd *cmd = cmd_ptr;
	uint64_t *uint_res = NULL; 
	void *extra_args = NULL;
	size_t extra_args_size = 0;
	var **res = NULL;

	printf("Processing PHI_CMD\n");
	switch(cmd->type) {
		case GET_VERSION:
			printf("Executing get_driver_version...\n");

			arg_count++;
			int_res = malloc_safe(sizeof(int));
			int_res_count = 1;

			phi_result = exec_scif_get_driver_version(int_res);

			break;
		case OPEN:
			printf("Executing scif_open() ... \n");
			arg_count++;
			int_res = malloc_safe(sizeof(int));
			int_res_count = 1;

			phi_result = exec_scif_open(int_res, cur_client);

			break;
		case CLOSE:
			printf("Executing scif_close() ... \n");
			//TODO: scif_close call goes here...

			phi_result = exec_scif_close((scif_epd_t)cmd->int_args[0], cur_client);

			break;
		case BIND:
			printf("Executing scif_bind() ... \n");
			//TODO: scif_bind call goes here...

			arg_count++;
			int_res = malloc_safe(sizeof(int));
			int_res_count = 1;

			phi_result = exec_scif_bind((scif_epd_t)cmd->int_args[0], 
					(uint16_t)cmd->uint_args[0], int_res);

			break;
		case LISTEN:
			printf("Executing scif_listen() ... \n");
			//TODO: scif_liste call goes here...

			phi_result = exec_scif_listen((scif_epd_t)cmd->int_args[0], 
					cmd->int_args[1]);

			break;
		case CONNECT:
			printf("Executing scif_connect() ... \n");
			//TODO: scif_connect call goes here...

			arg_count++;
			int_res = malloc_safe(sizeof(int));
			int_res_count = 1;

			phi_result = exec_scif_connect((scif_epd_t)cmd->int_args[0], 
					(struct scif_portID *)cmd->extra_args[0].data, int_res);

			break;
		case ACCEPT:
			printf("Executing scif_accept() ... \n");
			//TODO: scif_accept call goes here...

			arg_count++;
			int_res = malloc_safe(sizeof(int));
			int_res_count = 1;

			phi_result = exec_scif_accept((scif_epd_t)cmd->int_args[0], 
					(struct scif_portID *)cmd->extra_args[0].data, 
					int_res, cmd->int_args[1]); 

			break;
		case SEND:
			printf("Executing scif_send() ... \n");
			//TODO: scif_send call goes here...

			arg_count++;
			int_res = malloc_safe(sizeof(int));
			int_res_count = 1; 

			phi_result = exec_scif_send((scif_epd_t)cmd->int_args[0], 
					(void *)cmd->extra_args[0].data, cmd->int_args[1], 
					cmd->int_args[2], int_res);	

			break;
		case RECV:
			printf("Executing scif_recv() ... \n");
			//TODO: scif_recv call goes here...
			void *data;

			arg_count += 2;
			int_res = malloc_safe(sizeof(int));
			int_res_count = 1;

			extra_args = malloc_safe((size_t)cmd->int_args[1]);
			extra_args_size = (size_t)cmd->int_args[1];
			phi_result = exec_scif_recv((scif_epd_t)cmd->int_args[0], 
					&extra_args, cmd->int_args[1], cmd->int_args[2],
					int_res);
			break;
		case REGISTER:
			{
				printf("Executing scif_register() ... \n");
				//TODO: scif_register call goes here...
				arg_count++;

				extra_args_size = sizeof(void *) + sizeof(off_t);
				extra_args = malloc_safe(extra_args_size);

				void *addr;
				off_t resulted_offset;							

				phi_result = exec_scif_register((scif_epd_t)cmd->int_args[0],
						&addr, (size_t)cmd->int_args[1],
						(off_t)cmd->extra_args[0].data,
						cmd->int_args[2], cmd->int_args[3],
						&resulted_offset);	 								
				memcpy(extra_args, &addr, sizeof(void *));
				memcpy(extra_args + sizeof(void *), &resulted_offset, sizeof(off_t));
				break;
			}
		case UNREGISTER:
			printf("Executing scif_unregister() ... \n");
			//TODO: scif_unregister call goes here...

			phi_result = exec_scif_unregister((scif_epd_t)cmd->int_args[0],
					(off_t)cmd->extra_args[0].data,
					cmd->int_args[1]);		
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
			phi_result = exec_scif_readfrom((scif_epd_t)cmd->int_args[0], 
					(off_t)cmd->extra_args[0].data, (size_t)cmd->int_args[1],
					(off_t)cmd->extra_args[1].data, cmd->int_args[2]);
			break;
		case WRITE_TO:
			printf("Executing scif_write_to() ... \n");
			//TODO: scif_write_to call goes here...
			phi_result = exec_scif_writeto((scif_epd_t)cmd->int_args[0],
					(off_t)cmd->extra_args[0].data, (size_t)cmd->int_args[1],
					(off_t)cmd->extra_args[1].data, cmd->int_args[2]);
			break;
		case VREAD_FROM:
			{
				printf("Executing scif_vread_from() ... \n");
				//TODO: scif_vread_from call goes here...
				arg_count++;
				void *addr;
				size_t len = (size_t)cmd->int_args[1];
				addr = malloc_safe(len);
				phi_result = exec_scif_vreadfrom((scif_epd_t)cmd->int_args[0],
						addr, len, (off_t)cmd->extra_args[0].data,
						cmd->int_args[2]);
				extra_args = addr;
				extra_args_size = len;
				break;
			}
		case VWRITE_TO:
			printf("Executing scif_vwrite_to) ... \n");
			//TODO: scif_vwrite_to call goes here...
			phi_result = exec_scif_vwriteto((scif_epd_t)cmd->int_args[0],
					&cmd->extra_args[0].data, (size_t)cmd->int_args[1], 
					(off_t)cmd->extra_args[1].data,	cmd->int_args[2]);
			break;
		case FENCE_MARK:
			{
				printf("Executing scif_fence_mark() ... \n");
				//TODO: scif_fence_mark call goes here...
				arg_count++;

				int_res = malloc_safe(sizeof(int));
				int_res_count = 1;

				phi_result = exec_scif_fence_mark((scif_epd_t)cmd->int_args[0],
						cmd->int_args[1], int_res);
				break;
			}
		case FENCE_WAIT:
			printf("Executing scif_fence_wait() ... \n");
			//TODO: scif_fence_wait call goes here...

			phi_result = exec_scif_fence_wait((scif_epd_t)cmd->int_args[0],
					cmd->int_args[1]);
			break;
		case FENCE_SIGNAL:
			printf("Executing scif_fence_signal) ... \n");
			//TODO: scif_fence_signal call goes here...

			phi_result = exec_scif_fence_signal((scif_epd_t)cmd->int_args[0],
					(off_t)cmd->extra_args[0].data, cmd->uint_args[0],
					(off_t)cmd->extra_args[1].data, cmd->uint_args[1],
					cmd->int_args[1]);
			break;
		case GET_NODE_IDS:
			{
				printf("Executing scif_get_node_ids) ... \n");
				//TODO: scif_get_node_ids call goes here...
				arg_count += 2 ;

				int len = cmd->int_args[0];
				uint16_t *nodes, *self;

				nodes = malloc_safe(sizeof(uint16_t) * len);
				self = malloc_safe(sizeof(uint16_t));

				int_res = malloc_safe(sizeof(int));
				int_res_count = 1;

				phi_result = exec_scif_get_nodeIDs(nodes, len, self, int_res);

				extra_args_size = sizeof(uint16_t)*((*int_res)+1);
				extra_args = malloc_safe(extra_args_size);

				memcpy(extra_args, nodes, sizeof(uint16_t)*(*int_res));
				memcpy(extra_args+sizeof(uint16_t)*(*int_res), self, sizeof(uint16_t));

				break;
			}
		case POLL:
			{
				printf("Executing scif_poll() ... \n");
				//TODO: scif_poll call goes here...

				arg_count++;

				int_res = malloc_safe(sizeof(int));
				int_res_count = 1;

				phi_result = exec_scif_poll((struct scif_pollepd *)cmd->extra_args[0].data,
						cmd->uint_args[0], (long)cmd->uint_args[1],
						int_res);
				break;
			}
		case LIB_INIT:
			printf("Executing scif_lib_init() ... \n");
			//TODO: scif_lib_init call goes here...
			break;	
	}

	if(phi_result != PHI_SUCCESS)
	{
		errorno = malloc_safe(sizeof(int));
		errorno = errno;
		arg_count++;			
	}

	res = malloc_safe(sizeof(var *) * arg_count);

	res[0] = malloc_safe(sizeof(var));
	res[0]->type = PHI_RESULT_CODE;
	res[0]->elements = 1;
	res[0]->length = sizeof(int);
	res[0]->data = malloc_safe(res[0]->length);
	memcpy(res[0]->data, &phi_result, res[0]->length);

	if(arg_count > 1 ) 
	{
		int it = 1;	

		//int arguments
		if(int_res_count > 0)
		{
			res[it] = malloc_safe(sizeof(var));
			res[it]->type = INT;
			res[it]->elements = int_res_count;
			res[it]->length = sizeof(int)*int_res_count;
			res[it]->data = malloc_safe(res[it]->length);
			memcpy(res[it]->data, int_res, res[it]->length);
			it++;
		}

		//uint arguments
		if(uint_res_count > 0)
		{
			res[it] = malloc_safe(sizeof(var));
			res[it]->type = UINT;
			res[it]->elements = uint_res_count;
			res[it]->length = sizeof(uint64_t)*uint_res_count;
			res[it]->data = malloc_safe(res[it]->length);
			memcpy(res[it]->data, uint_res, res[it]->length);	
			it++;
		}

		//byte arguments
		if(extra_args_size > 0)
		{
			res[it] = malloc_safe(sizeof(var));
			res[it]->type = BYTES;
			res[it]->elements = 1;
			res[it]->length = extra_args_size;
			res[it]->data = malloc_safe(extra_args_size);
			memcpy(res[it]->data, extra_args, extra_args_size);
			it++;
		}
		
		//error number
		if(errno != NULL)
		{
			res[it] = malloc_safe(sizeof(var));
			res[it]->type = ERRORNO;
			res[it]-> elements = 1;
			res[it]->length = sizeof(int);
			res[it]->data = malloc_safe(sizeof(int));
			memcpy(res[it]->data, errorno, sizeof(int));
		}
	}

	*result = res;	

	if(int_res != NULL)
		free(int_res);
	if(uint_res != NULL)
		free(uint_res);
	if (extra_args != NULL)
		free(extra_args);
	if(errorno != NULL)
		free(errorno);

	return arg_count;
}
