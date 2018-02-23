#ifndef SCIFBENCH_COMMON_H
#define SCIFBENCH_COMMON_H

#include <scif.h>
#include <stdbool.h>

void bscif_timers_init();
void bscif_timers_print();

#define VA_bscif_endp_init(X, Y, ...) bscif_endp_init_(X, Y)
#define bscif_endp_init(...) VA_bscif_endp_init(__VA_ARGS__, 0)
scif_epd_t bscif_endp_init_(int port, bool listen);

int bscif_endp_connect(scif_epd_t epd, bool is_card, int rem_port);
int bscif_endp_accept(scif_epd_t epd, bool is_card, int rem_port);
int bscif_send(scif_epd_t epd, void *msg, int length, int flags);
int bscif_send_all(scif_epd_t epd, void *msg, int length, int flags);
int bscif_recv(scif_epd_t epd, void *msg, int length, int flags);
int bscif_recv_all(scif_epd_t epd, void *msg, int length, int flags);
bool bscif_verify_data(void *sent, void *recvd, int length);
void bscif_endp_close(scif_epd_t epd);


#define LISTEN_QUEUE_LEN 5

#endif  /* SCIFBENCH_COMMON_H */
