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
    void *data;
} var;

enum  cmd_type_enum {
	PHI_CMD=0,
	PHI_CMD_RESULT,
	TEST,
	RESULT,
    INIT
};

int get_server_connection_config(char *server_ip, char *server_port);
