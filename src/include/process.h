#ifndef PROCESS_H
#define PROCESS_H

#include <scif.h>
#include "common.pb-c.h"

typedef struct client_node_s {
	int id;
} client_node;

int exec_scif_open(scif_epd_t *endp);
int exec_scif_close(scif_epd_t endp);
int exec_scif_bind(scif_epd_t endp, uint16_t pn, int *portno);
int exec_scif_listen(scif_epd_t endp, int backlog);
int exec_scif_connect(scif_epd_t endp, struct scif_portID *dst, int *portID);
int exec_scif_accept(scif_epd_t endp, struct scif_portID *peer, scif_epd_t *newepd, int flags);
int exec_scif_send(scif_epd_t epd, void *msg, int len, int flags, int *send_count);
int exec_scif_recv(scif_epd_t epd, void *msg, int len, int flags, int *read_count);
int exec_scif_register(scif_epd_t endp, void *addr, size_t len, off_t offset, int prot_flags, int map_flags, off_t *resulted_off_t);
int exec_scif_unregister(scif_epd_t endp, off_t offset, size_t len, int *result);
int exec_scif_mmap(void *addr, size_t len, int prot_flags, int map_flags, scif_epd_t epd, off_t offset, void *result);
int exec_scif_munmap(void *addr, size_t len);
int exec_scif_readfrom(scif_epd_t epd, off_t loffset, size_t len, off_t roffset, int rma_flags, int *result);
int exec_scif_writeto(scif_epd_t epd, off_t loffset, size_t len, off_t roffset, int rma_flags, int *result);
int exec_scif_vreadfrom(scif_epd_t epd, void *addr, size_t len, off_t offset, int rma_flags, int *result);
int exec_scif_vwriteto(scif_epd_t epd, void *addr, size_t len, off_t offset, int rma_flags, int *result);
int exec_scif_fence_mark(scif_epd_t epd, int flags, int *mark, int *result);
int exec_scif_fence_wait(scif_epd_t epd, int mark, int *result);
int exec_scif_fence_signal(scif_epd_t epd, off_t loff, uint64_t lval, off_t roff, uint64_t rval, int flags, int *result);
int exec_scif_get_nodeIDs(uint16_t *nodes, int len, uint16_t *self, int *online_nodes);
int exec_scif_poll(struct scif_pollepd *epds, unsigned int nepds, long timeout, int *poll_res);

int process_phi_cmd(PhiCmd **result, PhiCmd *cmd);

#endif /* PROCESS_H */
