#include <stdio.h>
#include <stdlib.h>
//#include "include/common.h"
#include "include/timer.h"



#include <scif.h>

#define DEFAULT_OFFSET 0x4000000000000000
#define PEER_NODE 0

#define LOCAL_PORT 2150
#define REMOTE_PORT 2050

int main(int argc, char *argv[])
{
	scif_epd_t epd;
	struct scif_portID portID;

	int err;
	int msg_size;
	int local_port, remote_port;
	void *rbuf = NULL;
	char *end;

	int page_size = sysconf(_SC_PAGESIZE);
	scif_timers_t timer;
	//struct timeval time[2];
	TIMER_RESET(&timer);                                                                                                                  
	//struct timeval ctime,start;

	local_port = LOCAL_PORT;
	remote_port = REMOTE_PORT;

	if (argc != 5) {
		printf("usage: ./scif_send -r port -s <msg_size> \n");
						
		exit(1);
	}
	remote_port = atoi(argv[2]);
	local_port = remote_port + 100;
        portID.node = PEER_NODE;                                                                                                              
        portID.port = remote_port;
	msg_size = (int)strtol(argv[4], &end, 10);
	if (msg_size <= 0 || msg_size > INT_MAX) {                                                                                            
		printf("not valid msg size");
		exit(1);
	}

	/* scif_open : creates an end point, when successful returns end pt descriptor */
	if ((epd = scif_open()) == SCIF_OPEN_FAILED) {
		printf("scif_open failed with error %d\n", (int)epd);
		exit(1);
	}

	/* scif_bind : binds an end pt to a port_no, when successful returns the port_no
	 * to which the end pt is bound
	 */
	if ((err = scif_bind(epd, local_port)) < 0) {
		printf("scif_bind failed with error %d\n", err);
		exit(1);
	}
	printf("scif_bind to port %d success\n", local_port);

//        err = posix_memalign(&lbuf, page_size, msg_size);
//        if (err) {
//                printf("local mem allocation failed with errno : %d\n", errno);
//                goto __end;
//        }
//        memset(lbuf, 0xcd, msg_size);

        err = posix_memalign(&rbuf, page_size, msg_size);
        if (err) {
                printf("remote mem allocation failed with errno : %d\n", errno);
                goto __end;
        }
        memset(rbuf, 0xcd, msg_size);

	err = scif_connect(epd, &portID);
	if (err < 0) {
		printf("connection to node %d failed\n", portID.node);
		goto __end;
	}
	printf("conect to node %d success\n", portID.node);

sleep(1);
 off_t       offset = scif_register(epd,
                                rbuf,
                                msg_size,
                                0,
                                SCIF_PROT_READ | SCIF_PROT_WRITE,
                                0);
printf("OFFSET: 0x%lx\n", offset);

	printf("About to wrote %d bytes\n", msg_size);	
	TIMER_START(&timer);
	//err = scif_vreadfrom(epd, rbuf, msg_size, DEFAULT_OFFSET, SCIF_RMA_SYNC);
	err = scif_writeto(epd, offset, msg_size, DEFAULT_OFFSET, SCIF_RMA_SYNC);
	TIMER_STOP(&timer);
	if (err) {
                printf("scif_writeto failed with error %d\n", errno);

		errno = -1;
                goto __end;
	}
	printf("Just wrote %d bytes\n", msg_size);	

//	/* Verify sent, received data. Data verification is optional/usage dependent */
//	for (i = 0; i < (msg_size/sizeof(int)); i++) {
//		if (((int*)lbuf)[i] != ((int*)rbuf)[i]) {
//			printf("data mismatch lbuf[%d] 0x%x rbuf[%d] 0x%x\n",
//				i, ((int*)lbuf)[i], i, ((int*)rbuf)[i]);
//			goto __end;
//		}
//	}


	//printf("TIME: %llu ms %lf sec\n", TIMER_TOTAL(&timer), TIMER_TOTAL(&timer)/1000000000.0);
	//printf("TIME: %lf us %lf sec\n", TIMER_TO_USEC(&timer), TIMER_TO_SEC(&timer)/1000000000.0);
	printf("TIME: %llu us %lf sec\n", TIMER_TOTAL(&timer), TIMER_TOTAL(&timer)/1000000.0);
	errno = 0;


__end:
//	if (lbuf != NULL)
//		free(lbuf);
	if (rbuf != NULL)
		free(rbuf);

	if ((scif_close(epd) != 0)) {
		printf("scif_close failed with error %d\n", errno);
		exit(1);
	}

	if (errno == 0)
		printf("======== Program Success ========\n");
	else
		printf("======== Program Failed ========\n");

	return errno;
}
