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
#include <sys/mman.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "include/common.h"
#include "include/process.h"
#include "include/phi_errors.h"
#include "include/common.pb-c.h"


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
int exec_scif_open(scif_epd_t *endp)
{
	scif_epd_t t_endp;
	int ret;

	if((t_endp = scif_open()) < 0) {
		perror("scif_open");
		ret = SCIF_OPEN_FAIL; 
	}
	else {
		ret = SCIF_SUCCESS;
		*endp = t_endp;
	}
	return ret; 
}

int exec_scif_close(scif_epd_t endp)
{
	int ret;

	if(scif_close(endp)<0) {
		perror("scif_close");
		ret  = SCIF_CLOSE_FAIL;
	} else {
		ret = SCIF_SUCCESS;
	}
	return ret; 
}

int exec_scif_bind(scif_epd_t endp, uint16_t pn, int *portno)
{
	int ret;
	if((*portno = scif_bind(endp , pn)) < 0) {
		perror("scif_bind");
		ret = SCIF_BIND_FAIL;
	}
	else 
		ret = SCIF_SUCCESS;
	return ret; 
}

int exec_scif_listen(scif_epd_t endp, int backlog)
{
	int ret;
	if(scif_listen(endp, backlog) < 0) {
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
	if((*portID = scif_connect(endp, dst)) < 0) {
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
	if(scif_accept(endp, peer, newepd, flags) < 0) {
		perror("scif_accept");
		ret = SCIF_ACCEPT_FAIL;
	}
	else
		ret = SCIF_SUCCESS;
	return ret; 
}

int exec_scif_send(scif_epd_t endp, void *msg, int len, int flags, 
		int *send_count)
{
	int ret;
	if((*send_count = scif_send(endp, msg, len, flags)) < 0) {
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

	if((*read_count = scif_recv(endp, msg, len, flags)) < 0) {
		perror("scif_recv");
		ret = SCIF_RECV_FAIL;
	}
	else
		ret = SCIF_SUCCESS;
	return ret;
}

int exec_scif_register(scif_epd_t endp, void *addr, size_t len, 
		off_t offset, int prot_flags, int map_flags, 
		off_t *resulted_off_t)
{
	int ret; 

	if((*resulted_off_t = scif_register(endp, addr, len, offset, prot_flags, map_flags)) < 0) {
		perror("scif_register");
		ret = SCIF_REGISTER_FAIL;
	}
	else
		ret = SCIF_SUCCESS;
	return ret;
}

int exec_scif_unregister(scif_epd_t endp, off_t offset, size_t len, int *result)
{
	int ret; 

	if((*result = scif_unregister(endp, offset, len)) < 0) {
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

int exec_scif_readfrom(scif_epd_t epd, off_t loffset, size_t len, off_t roffset, int rma_flags, int *result)
{
	int ret; 

	if((*result = scif_readfrom(epd, loffset, len, roffset, rma_flags)) < 0) {
		perror("scif_readfrom");
		ret = SCIF_READ_FROM_FAIL;
	}
	else
		ret = SCIF_SUCCESS;

	return ret;
}

int exec_scif_writeto(scif_epd_t epd, off_t loffset, size_t len, off_t roffset, int rma_flags, int *result)
{
	int ret; 

	if((*result = scif_writeto(epd, loffset, len, roffset, rma_flags)) < 0) {
		perror("scif_writeto");
		ret = SCIF_WRITE_TO_FAIL;
	}
	else
		ret = SCIF_SUCCESS;

	return ret;
}

int exec_scif_vreadfrom(scif_epd_t epd, void *addr, size_t len, off_t offset, int rma_flags, int *result)
{
	int ret; 

	if((*result = scif_vreadfrom(epd, addr, len, offset, rma_flags)) < 0) {
		perror("scif_vreadfrom");
		ret = SCIF_VREAD_FROM_FAIL;
	}
	else
		ret = SCIF_SUCCESS;

	return ret;
}

int exec_scif_vwriteto(scif_epd_t epd, void *addr, size_t len, off_t offset, int rma_flags, int *result)
{
	int ret; 

	if((*result = scif_vwriteto(epd, addr, len, offset, rma_flags)) < 0) {
		perror("scif_vwriteto");
		ret = SCIF_VWRITE_TO_FAIL;
	}
	else
		ret = SCIF_SUCCESS;

	return ret;
}

int exec_scif_fence_mark(scif_epd_t epd, int flags, int *mark, int *result)
{
	int ret; 

	if((*result = scif_fence_mark(epd, flags, mark)) < 0) {
		perror("scif_fence_mark");
		ret = SCIF_FENCE_MARK_FAIL;
	}
	else
		ret = SCIF_SUCCESS;

	return ret;
}

int exec_scif_fence_wait(scif_epd_t epd, int mark, int *result)
{
	int ret; 

	if((*result = scif_fence_wait(epd, mark)) < 0) {
		perror("scif_fence_wait");
		ret = SCIF_FENCE_WAIT_FAIL;
	}
	else
		ret = SCIF_SUCCESS;

	return ret;
}

int exec_scif_fence_signal(scif_epd_t epd, off_t loff, uint64_t lval, off_t roff, uint64_t rval,
		int flags, int *result)
{
	int ret; 

	if(( *result = scif_fence_signal(epd, loff, lval, roff, rval, flags)) < 0) {
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

	if((*online_nodes = scif_get_nodeIDs(nodes, len, self)) < 0) {
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

	if((*poll_res = scif_poll(epds, nepds, timeout)) < 0 ) {
		perror("scif_poll");
		ret = SCIF_POLL_FAIL;
	}
	else
		ret = SCIF_SUCCESS;
	return ret;
}

int process_phi_cmd(PhiCmd **result, PhiCmd *cmd) {
	PhiCmd *res;

	ddprintf("Processing PHI_CMD %d\n", cmd->type);

	res = malloc_safe(sizeof(PhiCmd));
	phi_cmd__init(res);

	res->type = PHI_CMD_RESULT;

	switch(cmd->type) {
		case GET_VERSION:
			res->arg_count = 2;

			res->n_int_args = 1;
			res->int_args = malloc_safe(sizeof(int));

			res->phi_result_code = exec_scif_get_driver_version(&(res->int_args[0]));

			break;
		case OPEN:
			res->arg_count = 2;

			res->n_int_args = 1;
			res->int_args = malloc_safe(sizeof(int));

			res->phi_result_code = exec_scif_open(&(res->int_args[0]));

			break;
		case CLOSE:
			//TODO: scif_close call goes here...
			res->arg_count = 1;

			res->phi_result_code = exec_scif_close((scif_epd_t)cmd->int_args[0]);

			break;
		case BIND:
			//TODO: scif_bind call goes here...
			res->arg_count = 2;

			res->n_int_args = 1;
			res->int_args = malloc_safe(sizeof(int));

			res->phi_result_code = exec_scif_bind((scif_epd_t)cmd->int_args[0], 
					(uint16_t)cmd->uint_args[0], &(res->int_args[0]));

			break;
		case LISTEN:
			//TODO: scif_liste call goes here...
			res->arg_count = 1;
			
			res->phi_result_code = exec_scif_listen((scif_epd_t)cmd->int_args[0], 
					cmd->int_args[1]);

			break;
		case CONNECT:
			//TODO: scif_connect call goes here...
			res->arg_count = 2;

			res->n_int_args = 1;
			res->int_args = malloc_safe(sizeof(int));

			res->phi_result_code = exec_scif_connect((scif_epd_t)cmd->int_args[0], 
					(struct scif_portID *)cmd->extra_args[0].data, &(res->int_args[0]));
			break;
		case ACCEPT:
			//TODO: scif_accept call goes here...

			res->arg_count = 2;

			res->n_int_args = 1;
			res->int_args = malloc_safe(sizeof(int));

			cmd->phi_result_code = exec_scif_accept((scif_epd_t)cmd->int_args[0], 
					(struct scif_portID *)cmd->extra_args[0].data, 
					&(res->int_args[0]), cmd->int_args[1]); 

			break;
		case SEND:
			//TODO: scif_send call goes here...
			ddprintf("scif_send..\n");
			res->arg_count = 2;

			res->n_int_args = 1;
			res->int_args = malloc_safe(sizeof(int));

			res->phi_result_code = exec_scif_send((scif_epd_t)cmd->int_args[0], 
					(void *)cmd->extra_args[0].data, (int)cmd->int_args[1], 
					(int)cmd->int_args[2], &(res->int_args[0]));	

			break;
		case RECV:
			//TODO: scif_recv call goes here...
			ddprintf("recv\n");
			res->arg_count = 3;

			res->n_int_args = 1;
			res->int_args = malloc_safe(sizeof(int));

			res->n_extra_args = 1;
			res->extra_args = malloc_safe(sizeof(ProtobufCBinaryData));

			res->extra_args[0].len = cmd->int_args[1];
			ddprintf("to read %d\n", cmd->int_args[1]);
			res->extra_args[0].data = malloc_safe(res->extra_args[0].len);
			ddprintf("about to call recv\n");
			res->phi_result_code = exec_scif_recv((scif_epd_t)cmd->int_args[0], 
					res->extra_args[0].data, cmd->int_args[1], cmd->int_args[2],
					&(res->int_args[0]));
			ddprintf("finished rcv while read %d\n", res->int_args[0]);
			break;
		case REGISTER: 
			//TODO: scif_register call goes here...
			res->arg_count = 2;

			res->n_extra_args = 1;
			res->extra_args = malloc_safe(sizeof(ProtobufCBinaryData));

			res->extra_args[0].len = sizeof(off_t);
			res->extra_args[0].data = malloc_safe(res->extra_args[0].len);


			off_t resulted_offset, client_offset;							
			void *addr, *client_addr;
			pid_t client_pid;
			addr_map *map_slot = NULL;

			memcpy(&client_addr, cmd->extra_args[0].data, sizeof(void *));
			memcpy(&client_offset, cmd->extra_args[1].data, sizeof(off_t));
			memcpy(&client_pid, cmd->extra_args[2].data, sizeof(pid_t));

			//addr = mmap(NULL, (size_t)cmd->uint_args[0],  PROT_READ | PROT_WRITE,
			//MAP_ANON | MAP_SHARED, -1, 0);
			if(posix_memalign(&addr, 0x1000, (size_t)cmd->uint_args[0]) != 0 ){
				perror("posix_memalign");
				res->int_args[0] = -1;
				break;
			}	

			res->phi_result_code = exec_scif_register((scif_epd_t)cmd->int_args[0],
					addr, (size_t)cmd->uint_args[0],
					client_offset,
					cmd->int_args[1], cmd->int_args[2],
					res->extra_args[0].data);

			map_slot = identify_map(client_pid, client_addr, addr, (size_t)cmd->uint_args[0], *((off_t *)res->extra_args[0].data));
			if(map_slot == NULL) {
				printf("error creating map for scif_register()\n");
			}
			break;

		case UNREGISTER: { 
				//TODO: scif_unregister call goes here...
				pid_t pid;
				off_t offset; 
			
				res->arg_count = 2;

				res->n_int_args = 1;
				res->int_args = malloc_safe(sizeof(int));

				memcpy(&offset, cmd->extra_args[0].data, sizeof(off_t));
				memcpy(&pid, cmd->extra_args[1].data, sizeof(pid_t));

				res->phi_result_code = exec_scif_unregister((scif_epd_t)cmd->int_args[0],
						offset,
						cmd->uint_args[0], &(res->int_args[0]));		
				if(res->phi_result_code == SCIF_SUCCESS || errno == 104 ) {
					addr_map *mp = get_map(pid, offset);
					free(mp->server_addr);
					if(remove_mapping(pid, offset) < 0)
						printf("error freeing mapping\n");
				}
				break;
		}
		case MMAP:
			//TODO: scif_mmap call goes here...
			break;
		case MUNMAP:
			//TODO: scif_munmap call goes here...
			break;
		case READ_FROM: { 
				//TODO: scif_read_from call goes here...

				res->arg_count = 2;

				res->n_int_args = 1;
				res->int_args = malloc_safe(sizeof(int));

				off_t loffset, roffset;
				pid_t pid;
				pthread_t tid;

				memcpy(&loffset, cmd->extra_args[0].data, sizeof(off_t));
				memcpy(&roffset, cmd->extra_args[1].data, sizeof(off_t));
				memcpy(&pid, cmd->extra_args[2].data, sizeof(pid_t));

				res->phi_result_code = exec_scif_readfrom((scif_epd_t)cmd->int_args[0], 
						loffset, (size_t)cmd->uint_args[0],
						roffset, cmd->int_args[1], &(res->int_args[0]));
				if(res->phi_result_code == SCIF_SUCCESS) {
					//addr_map *mp = identify_map(pid, NULL, NULL, loffset);
					addr_map *mp = get_map(pid, loffset);
	
					res->arg_count++;
	
					res->n_extra_args = 1;
					res->extra_args = malloc_safe(sizeof(ProtobufCBinaryData));
					res->extra_args[0].len = (size_t)cmd->uint_args[0];
	
					void *copy_from = mp->server_addr + (loffset - mp->offset);				
					res->extra_args[0].data = copy_from;
				}
	
				break;
		}
		case WRITE_TO: {
				//TODO: scif_write_to call goes here...
				res->arg_count = 2;
	
				res->n_int_args = 1;
				res->int_args = malloc_safe(sizeof(int));
	
				off_t loffset, roffset;
				pid_t pid;
				pthread_t tid;
				size_t len = (size_t)cmd->uint_args[0];
	
				memcpy(&loffset, cmd->extra_args[0].data, sizeof(off_t));
				memcpy(&roffset, cmd->extra_args[1].data, sizeof(off_t));
				memcpy(&pid, cmd->extra_args[2].data, sizeof(pid_t));			
	
				addr_map *mp = get_map(pid, loffset);
				if(mp == NULL) {
					printf("scif_writeto: error while trying to obtain mapped srvr_addr\n");
				}
				void *copy_to = mp->server_addr + (loffset - mp->offset);
	
				//copy to server registered address space len bytes for dma
				memcpy(copy_to, cmd->extra_args[3].data, len);

				res->phi_result_code = exec_scif_writeto((scif_epd_t)cmd->int_args[0],
						loffset, len,
						roffset, cmd->int_args[1], &(res->int_args[0]));
				break;
		}
		case VREAD_FROM: {
				//TODO: scif_vread_from call goes here...
	
				size_t len = (size_t)cmd->uint_args[0];
				res->arg_count = 3;
	
				res->n_int_args = 1;
				res->int_args = malloc_safe(sizeof(int));
	
				res->n_extra_args = 1;
				res->extra_args = malloc_safe(sizeof(ProtobufCBinaryData));
				res->extra_args[0].len = len;
				res->extra_args[0].data = malloc_safe(len);
	
				res->phi_result_code = exec_scif_vreadfrom((scif_epd_t)cmd->int_args[0],
							res->extra_args[0].data, len, *(off_t *)cmd->extra_args[0].data, 
							cmd->int_args[1], &(res->int_args[0]));
				break;
		}
		case VWRITE_TO: { 
				//TODO: scif_vwrite_to call goes here...
				res->arg_count = 2;

				res->n_int_args = 1;
				res->int_args = malloc_safe(sizeof(int));

				off_t offset;
				memcpy(&offset, cmd->extra_args[0].data, sizeof(off_t));
	
				res->phi_result_code = exec_scif_vwriteto((scif_epd_t)cmd->int_args[0],
							(void *)cmd->extra_args[1].data, (size_t)cmd->uint_args[0], 
							offset, cmd->int_args[1], &(res->int_args[0]));
				break;
		}
		case FENCE_MARK: { 
				//TODO: scif_fence_mark call goes here...
				res->arg_count = 2;
		
				res->n_int_args = 2;
				res->int_args = malloc_safe(sizeof(int)*2);
	
				res->phi_result_code = exec_scif_fence_mark((scif_epd_t)cmd->int_args[0],
						cmd->int_args[1], &(res->int_args[0]), &(res->int_args[1]));
			break;
		}
		case FENCE_WAIT: {
				//TODO: scif_fence_wait call goes here...
				res->arg_count = 2;

				res->n_int_args = 1;
				res->int_args = malloc_safe(sizeof(int));

				res->phi_result_code = exec_scif_fence_wait((scif_epd_t)cmd->int_args[0],
						cmd->int_args[1], &(res->int_args[0]));
				break;
		}
		case FENCE_SIGNAL: {
				//TODO: scif_fence_signal call goes here...
				res->arg_count = 2;

				res->n_int_args = 1;
				res->int_args = malloc_safe(sizeof(int));
	
				off_t loff, roff;
				memcpy(&loff, cmd->extra_args[0].data, sizeof(off_t));
				memcpy(&roff, cmd->extra_args[1].data, sizeof(off_t));

				res->phi_result_code = exec_scif_fence_signal((scif_epd_t)cmd->int_args[0],
						loff, cmd->u64int_args[0],
						roff, cmd->u64int_args[1],
						cmd->int_args[1], &(res->int_args[0]));
				break;
		}
		case GET_NODE_IDS: {
				//TODO: scif_get_node_ids call goes here...
				res->arg_count = 3;

				res->n_int_args = 1;
				res->int_args = malloc_safe(sizeof(int));

				int len = cmd->int_args[0];
				uint16_t *nodes, *self;

				nodes = malloc_safe(sizeof(uint16_t) * len);
				self = malloc_safe(sizeof(uint16_t));
	
				res->phi_result_code = exec_scif_get_nodeIDs(nodes, len, self, &(res->int_args[0]));
				res->n_extra_args = 2;
				res->extra_args = malloc_safe(sizeof(ProtobufCBinaryData)*2);
				
				res->extra_args[0].len = sizeof(uint16_t)*(res->int_args[0]);
				res->extra_args[0].data = nodes;
				res->extra_args[1].len = sizeof(uint16_t);
				res->extra_args[1].data = self;
	
				break;
		}
		case POLL: {

				//TODO: scif_poll call goes here...
				res->arg_count = 3;

				res->n_int_args = 1;
				res->int_args = malloc_safe(sizeof(int));
	
				struct scif_pollepd *epds = (struct scif_pollepd *)cmd->extra_args[0].data;

				res->phi_result_code = exec_scif_poll( epds,
						cmd->uint_args[0], (long)cmd->uint_args[1],
						&(res->int_args[0]));

				res->n_extra_args = 1;
				res->extra_args = malloc_safe(sizeof(ProtobufCBinaryData));
				res->extra_args[0].len = sizeof(struct scif_pollepd) * cmd->uint_args[0];
				res->extra_args[0].data = malloc_safe(res->extra_args[0].len);
				memcpy(res->extra_args[0].data, epds, res->extra_args[0].len);
	
				break;
		}
		case LIB_INIT:
			//TODO: scif_lib_init call goes here...
			break;	
	}

	if(res->phi_result_code != PHI_SUCCESS)
	{
		res->arg_count++;
		res->has_phi_errorno = 1;
		res->phi_errorno = errno;
	}

	*result = res;	
	ddprintf("process finished\n");
	return res->arg_count;
}
