#ifndef CLIENT_H
#define CLIENT_H

#include "common.pb-c.h"

typedef struct _unitofwork {
	int endp;
 	int socket_fd;
	int ref_count;
} unitofwork ;


int init_client_connection(const char *s_ip, const char *s_port);

void establish_connection(unitofwork *uow);

int send_phi_cmd(int socket_fd, var **args, size_t arg_cnt, int cmd_type);

int get_phi_cmd_result(PhiCmd **result, void **des_msg, int socket_fd);

#endif /* CLIENT_H */
