#include <stdio.h>
#include <stdlib.h>
//#include "include/common.h"
#include "include/timer.h"



#include <scif.h>

#define PEER_NODE 1

#define LOCAL_PORT 2150
#define REMOTE_PORT 2050

int main(int argc, char *argv[])
{
	scif_epd_t epd;
	struct scif_portID portID;

	int no_bytes, err;
	int msg_size;
	int local_port, remote_port;
	void *send_buf = NULL;
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

        //send_buf = (char *)malloc(msg_size);
        err = posix_memalign(&send_buf, page_size, msg_size);
        if (err) {
                printf("send mem allocation failed with errno : %d\n", errno);
                goto __end;
        }
        memset(send_buf, 0xcd, msg_size);

	err = scif_connect(epd, &portID);
	if (err < 0) {
		printf("connection to node %d failed\n", portID.node);
		goto __end;
	}
	printf("conect to node %d success\n", portID.node);

//        recv_buf = (char *)malloc(msg_size);
//        if (recv_buf == NULL) {
//                printf("recv mem allocation failed with errno : %d\n", errno);
//                goto __end;
//        }
//        memset(recv_buf, 0x0, msg_size);

	printf("About to send %d bytes\n", msg_size);	
	TIMER_START(&timer);                                                                                                                  
	no_bytes = scif_send(epd, send_buf, msg_size, SCIF_SEND_BLOCK);
	TIMER_STOP(&timer);                                                                                                                  
	printf("Just sent %d bytes\n", no_bytes);	

        if (no_bytes != msg_size) {
                printf("scif_send failed with error %d\n", errno);

		errno = -1;
                goto __end;
        }

//	/* Verify sent, received data. Data verification is optional/usage dependent */
//	for (i = 0; i < (msg_size/sizeof(int)); i++) {
//		if (((int*)send_buf)[i] != ((int*)recv_buf)[i]) {
//			printf("data mismatch send_buf[%d] 0x%x recv_buf[%d] 0x%x\n",
//				i, ((int*)send_buf)[i], i, ((int*)recv_buf)[i]);
//			goto __end;
//		}
//	}


	//printf("TIME: %llu ms %lf sec\n", TIMER_TOTAL(&timer), TIMER_TOTAL(&timer)/1000000000.0);
	//printf("TIME: %lf us %lf sec\n", TIMER_TO_USEC(&timer), TIMER_TO_SEC(&timer)/1000000000.0);
	printf("TIME: %llu us %lf sec\n", TIMER_TOTAL(&timer), TIMER_TOTAL(&timer)/1000000.0);
	errno = 0;


__end:
	if (send_buf != NULL)
		free(send_buf);
//	if (recv_buf != NULL)
//		free(recv_buf);

	/* scif_close : closes the end pt, when successful returns 0 */
	//if ((scif_close(epd) != 0) || (scif_close(newepd) != 0)) {
	if ((scif_close(epd) != 0)) {
		printf("scif_close failed with error %d\n", errno);
		exit(1);
	}
	printf("scif_close success\n");

	if (errno == 0)
		printf("======== Program Success ========\n");
	else
		printf("======== Program Failed ========\n");

	return errno;
}
