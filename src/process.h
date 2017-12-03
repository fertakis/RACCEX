#ifndef PROCESS_H
#define PROCESS_H

typedef struct client_node_s {
	int id;
} client_node;

int process_phi_cmd(void **result, void *cmd_ptr, client_node *cur_client);

#endif /* PROCESS_H */
