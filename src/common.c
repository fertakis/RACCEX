#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

char *server_ip;
char *server_port;

int get_server_connection_config(char *server_ip, char *server_port)
{
    int ret = 0;
    const char *remote_server = getenv("REMOTE_PHI_SERVER"),
          *remote_port = getenv("REMOTE_PHI_PORT");
    if(remote_server == NULL)
    {
        remote_server = DEFAULT_SERVER_IP;
        printf("Enviromental Variable 'REMOTE_PHI_SERVER' not defined, using default server ip: %s\n", remote_server);
        ret = 1 ;
    }
    if(remote_port == NULL)
    {
        remote_server = DEFAULT_SERVER_IP;
        printf("Enviromental Variable 'REMOTE_PHI_SERVER' not defined, using default server ip: %s\n", remote_server);
        ret = 1;
    }

    strcpy(server_ip, remote_server);
    strcpy(server_port, remote_port);

    return ret;
}
