#include <stdint.h>
#include <stdlib.h>

#define DEFAULT_SERVER_IP "localhost"
#define DEFAULT_SERVER_PORT "8888"

typedef var_type_enum {
    INT,
    UINT,
    STRING,
    BYTES
} var_type;

typedef cmd_type_enum {
    INIT=0,
} cmds;

typedef struct var_struct {
    var_type type;
    void *data;
} var;
