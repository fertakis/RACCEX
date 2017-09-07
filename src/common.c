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
