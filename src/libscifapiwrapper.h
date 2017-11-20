#ifndef LIBSCIFAPIWRAPPER_H
#define LIBSCIFAPIWRAPPER_H


int scif_get_driver_version(void);
scif_epd_t scif_open(void);
int scif_close(scif_epd_t epd);
int scif_bind(scif_epd_t epd, uint16_t pn);
int scif_listen(scif_epd_t epd, int backlog);


#endif /* LIBSCIFAPIWRAPPER_H */
