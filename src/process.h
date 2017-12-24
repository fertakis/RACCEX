#ifndef PROCESS_H
#define PROCESS_H

typedef struct client_node_s {
	int id;
} client_node;

int exec_scif_open(scif_epd_t *endp, client_node **cur_client);
int exec_scif_close(scif_epd_t *endp, client_node **cur_client);
int exec_scif_bind(scif_epd_t endp, uint16_t pn, int *portno);
int exec_scif_listen(scif_epd_t endp, int backlog);
int exec_scif_connect(scif_epd_t endp, struct scif_portID *dst, int *portID);
int exec_scif_accept(scif_epd_t endp, struct scif_portID *peer, scif_epd_t *newepd, int flags);


int process_phi_cmd(void **result, void *cmd_ptr,client_node **cur_client);

#endif /* PROCESS_H */
