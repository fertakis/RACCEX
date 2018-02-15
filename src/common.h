#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include "common.pb-c.h"

#define DEFAULT_SERVER_IP "localhost"
#define DEFAULT_SERVER_PORT "8888"

/* Compile-time options */
#define TCP_PORT    35001
#define TCP_BACKLOG 5

extern struct addr_map_list maps; 

typedef enum var_type_enum {
    INT,
    UINT,
    U64INT,
    STRING,
    BYTES,
    PHI_RESULT_CODE, 
    ERRORNO
} var_type;

typedef struct var_struct {
    var_type type;
    size_t length;
    size_t elements;
    void *data;
} var;

enum {
	PHI_CMD = 0,
	PHI_CMD_RESULT,
	TEST,
	INIT,
	GET_VERSION,
	OPEN,
	CLOSE,
	BIND,
	LISTEN,
	CONNECT,
	ACCEPT,
	SEND,
	RECV,
	REGISTER,
	UNREGISTER,
	MMAP,
	MUNMAP,
	READ_FROM,
	WRITE_TO,
	VREAD_FROM,
	VWRITE_TO,
	FENCE_MARK,
	FENCE_WAIT,
	FENCE_SIGNAL,
	GET_NODE_IDS,
	POLL,
	LIB_INIT
};

enum phi_result_code{
	PHI_SUCCESS=0,
	PHI_ERROR
};

enum scif_return_codes {
	SCIF_SUCCESS=0,
	SCIF_GET_DRIVER_VERSION_FAIL,
	SCIF_OPEN_FAIL,
	SCIF_BIND_FAIL,
	SCIF_LISTEN_FAIL,
	SCIF_CONNECT_FAIL,
	SCIF_ACCEPT_FAIL,
	SCIF_CLOSE_FAIL,
	SCIF_SEND_FAIL,
	SCIF_RECV_FAIL,
	SCIF_REGISTER_FAIL,
	SCIF_UNREGISTER_FAIL,
	SCIF_MMAP_FAIL,
	SCIF_MUNMAP_FAIL,
	SCIF_READ_FROM_FAIL,
	SCIF_WRITE_TO_FAIL,
	SCIF_VREAD_FROM_FAIL,
	SCIF_VWRITE_TO_FAIL,
	SCIF_FENCE_MARK_FAIL,
	SCIF_FENCE_WAIT_FAIL,
	SCIF_FENCE_SIGNAL_FAIL,
	SCIF_GET_NODE_IDS_FAIL,
	SCIF_POLL_FAIL,
	SCIF_LIB_INIT_FAIL
};
typedef struct address_mapping {
	pid_t proc_id;
	void *client_addr, *server_addr;
	off_t offset;
	size_t len;
	struct address_mapping *next;
} addr_map;

struct addr_map_list {
	addr_map *head;
	int num_maps;	
};

typedef struct thread_management {
	pthread_t thread_id;
	int sockfd;
	int ref_count;
	struct thread_management *next;
} thr_mng;

struct thread_mng_list {
	thr_mng *head;
	int num_threads;
};

addr_map * get_map(pid_t pid, off_t lofft);
int remove_mapping(pid_t pid, off_t offset);
addr_map * identify_map( pid_t pid, void *clnt_addr, void *srv_addr, size_t len, off_t offt);
void initialise_addr_map_list();
void get_server_connection_config(char **server, char **server_port);
int pack_phi_cmd(void ** payload, var **args, size_t arg_count, int type);
int unpack_phi_cmd(var ** args, PhiCmd *cmd);

void *malloc_safe_f(size_t size, const char *file, const int line);
#define malloc_safe(size) malloc_safe_f(size, __FILE__, __LINE__)

#ifdef RSCIF_DEBUG
#define rdprintf printf
#else
#define rdprintf  
#endif

#ifdef RSCIF_COMM_DEBUG
#define rcdprintf printf
#else
#define rcdprintf  
#endif


#endif /* CLIENT_H */
