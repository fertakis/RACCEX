#ifndef CLIENT_H
#define CLIENT_H

typedef struct _unitofwork {
	int endp;
 	int socket_fd;
} unitofwork ;


int init_client_connection(const char *s_ip, const char *s_port);

void init_params(unitofwork *uow);

void establish_connection(unitofwork *uow);

int send_phi_cmd(int socket_fd, var **args, size_t arg_cnt, int cmd_type);

int get_phi_cmd_result(PhiCmd **result, int socket_fd);

#endif /* CLIENT_H */
