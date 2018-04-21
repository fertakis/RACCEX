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

#include "include/common.h"
#include "include/common.pb-c.h"

char *server_ip;
char *server_port;
FILE *out_fd;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
struct addr_map_list maps;

void initialise_addr_map_list() {
	maps.num_maps = 0;
	maps.head = NULL;
}

addr_map * identify_map( pid_t pid, void *clnt_addr, void *srv_addr, 
				size_t len, off_t offt) 
{
	addr_map *ret = NULL;
	
	pthread_mutex_lock(&lock);

	if(maps.head == NULL) {
		//first mapping
		maps.num_maps++;
		maps.head = malloc_safe(sizeof(addr_map));
		maps.head->next = NULL;
		maps.head->proc_id = pid;
		maps.head->client_addr = clnt_addr;
		maps.head->server_addr = srv_addr;
		maps.head->len = len;
		maps.head->offset = offt;
		ret = maps.head;
	}
	else {
		addr_map *it = maps.head;
		while (it->next != NULL) {
			if( it->client_addr == clnt_addr &&
					it->proc_id == pid) {
				//found it
				ret = it;
				break;
			}
			it = it->next;
		}
		if(ret == NULL) {
			//not found, check last entry
			if( it->client_addr == clnt_addr &&
					it->proc_id == pid)
				//found, was the last entry
				ret = it;
			else {
				//not found, create new entry
				ret = malloc_safe(sizeof(addr_map));
				ret->proc_id = pid;
				ret->client_addr = clnt_addr;
				ret->server_addr = srv_addr;
				ret->len = len;
				ret->offset = offt;
				ret->next = NULL;
				it->next = ret;
				maps.num_maps++;
			}
		}
	}

	pthread_mutex_unlock(&lock);

	return ret; 
}

addr_map * get_map(pid_t pid, off_t lofft) 
{
	addr_map *ret = NULL;
	

	pthread_mutex_lock(&lock);

	if(maps.head == NULL) {

		pthread_mutex_unlock(&lock);
		return ret;	
	}
	else {
		addr_map *it = maps.head;
		while (it->next != NULL) {
			if( (lofft >= it->offset) &&
					(lofft < (it->offset + it->len)) && 
					(it->proc_id == pid)) {
				//found it
				ret = it;
				break;
			}
			it = it->next;
		}
		if(ret == NULL) {
			//not found, check last entry
			if(( lofft >= it->offset) &&
					(lofft < (it->offset + it->len)) &&
					(it->proc_id == pid))
				//found, was the last entry
				ret = it;
			}
	}


	pthread_mutex_unlock(&lock);
	return ret; 
}

int remove_mapping(pid_t pid, off_t offset) {
	int ret = -1; 

	addr_map *curr, *prev;


	pthread_mutex_lock(&lock);
	
	curr = maps.head;
	prev = maps.head;

	if((maps.head->offset == offset) && (maps.head->proc_id == pid)) {
		void *temp = maps.head;
		maps.head = maps.head->next; 
		maps.num_maps--;
		free(temp);
		ret = 0;
	}
	else {
		while(curr != NULL) {
			if(curr->offset == offset && curr->proc_id == pid) {
				//found it 
				prev->next = curr->next;
				maps.num_maps--;
				free(curr);
				ret = 0;
				break;
			}
			prev = curr;
			curr = curr->next;
		}
	}


	pthread_mutex_unlock(&lock);
	return ret; 
}

inline void *malloc_safe_f(size_t size, const char *file, const int line) {
	void *ptr = NULL;

	ptr = malloc(size);
	if (ptr == NULL && size != 0) {
		fprintf(stderr, "[%s, %i] Memory allocation failed!\n", file, line);
		exit(EXIT_FAILURE);
	}

	return ptr;
}

void get_server_connection_config(char **server, char **server_port)
{
	*server = getenv("REMOTE_PHI_SERVER"),
		*server_port = getenv("REMOTE_PHI_PORT");

	ddprintf("server config..\n");
	if(*server == NULL)
	{
		*server = DEFAULT_SERVER_IP;
		ddprintf("Enviromental Variable 'REMOTE_PHI_SERVER' not defined, using default server ip: %s\n", *server);
	}
	if(*server_port == NULL)
	{
		*server_port = DEFAULT_SERVER_PORT;
		ddprintf("Enviromental Variable 'REMOTE_PHI_PORT' not defined, using default server port: %s\n", *server_port);
	}
	ddprintf("end server config\n");	
}

int pack_phi_cmd(void **payload, var **args, size_t arg_count, int type) {
	PhiCmd *cmd;
	int i;

	ddprintf("Packing PHI cmd...\n");

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
			case U64INT:
				cmd->n_u64int_args = args[i]->elements;
				cmd->u64int_args = args[i]->data;
			case BYTES:
				//cmd->n_extra_args = args[i]->elements;
				cmd->n_extra_args = 1;
				cmd->extra_args = malloc_safe(sizeof(*(cmd->extra_args)) * cmd->n_extra_args);
				cmd->extra_args[0].data = args[i]->data;
				cmd->extra_args[0].len = args[i]->length;
				break;
			case PHI_RESULT_CODE:
				cmd->phi_result_code = *(int *)args[i]->data;
				break;
			case ERRORNO:
				cmd->has_phi_errorno = 1;
				cmd->phi_errorno = *(int *)args[i]->data;
				break;
		}
	}

	*payload = cmd;
	return 0;
}

//TODO: Implement unpack function in order to be able to seperate transport layer
//	from application layer.(In libscifapiwrapper absense of pointer to Cookie structure)
int unpack_phi_cmd(var **args, PhiCmd *cmd ) 
{
	int ret = 0 ;

	ddprintf("Unpacking PHI cmd ...");

	return ret;
}

void print_bytes(const void *object, size_t size)
{
	  // This is for C++; in C just drop the static_cast<>() and assign.
	  const unsigned char * const bytes = (const unsigned char *)object;
	  size_t i;
	  
	  fprintf(out_fd, "[ ");
	  for(i = 0; i < size; i++)
	  {
	  	fprintf(out_fd, "%02x ", bytes[i]);
	  	//fprintf(out_fd, "%d ", bytes[i]);
	  }
	  fprintf(out_fd, "]\n");
}

/*void print_ints(const void *object, size_t size)
{
	  int *buff = object;
	  int i, len = size/sizeof(int);
	  
	  fprintf(out_fd, "[ ");
	  for(i = 0; i < len; i++) {
	  	fprintf(out_fd, "%d ", buff[i]);
	  	//fprintf(out_fd, "%d ", bytes[i]);
	  }
	  fprintf(out_fd, "]\n");
}*/
