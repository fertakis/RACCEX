#include <stdint.h>
#include <stdlib.h>

#define DEFAULT_SERVER_IP "localhost"
#define DEFAULT_SERVER_PORT "8888"

typedef enum var_type_enum {
    INT,
    UINT,
    STRING,
    BYTES
} var_type;

typedef struct var_struct {
    var_type type;
    size_t length;
    void *data;
} var;

enum {
	PHI_CMD = 0,
	PHI_CMD_RESULT,
	TEST,
	INIT,
	VERSION,
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

int get_server_connection_config(char *server_ip, char *server_port);


