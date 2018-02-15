#include <stdio.h>
#include <stdlib.h>
#include "include/common.h"



#include <scif.h>


#define LOCAL_PORT 2050

int main(int argc, char *argv[])
{
	scif_epd_t epd;
	scif_epd_t newepd;
	struct scif_portID portID;

	int i, err, backlog = 16;
	int msg_size;
	int conn_port, req_port;
	void *reg_buf = NULL, *test_buf = NULL;
	char *end;
	int page_size = sysconf(_SC_PAGESIZE);
	off_t offset;

	req_port = LOCAL_PORT;

	if (argc != 5) {
		printf("usage: ./scif_sendrecv -l port -s <msg_size> \n");
						
		exit(1);
	}
	req_port = atoi(argv[2]);
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
	if ((conn_port = scif_bind(epd, req_port)) < 0) {
		printf("scif_bind failed with error %d\n", conn_port);
		exit(1);
	}
	printf("scif_bind to port %d success\n", conn_port);

	/* scif_listen : marks an end pt as listening end and returns, when successful
	 * returns 0. backlog is length of request queue: maximum no: of requests
	 * that can be pending
	 */
	if (scif_listen(epd, backlog) != 0) {
		printf("scif_listen failed with error %d\n", errno);
		exit(1);
	}

	/* scif_accept : accepts connection requests on listening end pt and creates a
	 * new end pt which connects to the peer end pt that initiated connection,
	 * when successful returns 0.
	 * SCIF_ACCEPT_SYNC blocks the call untill a connection is present
	 */
	if (scif_accept(epd, &portID, &newepd, SCIF_ACCEPT_SYNC) != 0) {
		printf("scif_accept failed with error %d\n", errno);
		exit(1);
	}
	printf("accepted connection request from node:%d port:%d\n", portID.node, portID.port);

	err = posix_memalign(&test_buf, page_size, msg_size);
        if (err) {
                printf("test mem allocation failed with errno : %d\n", errno);
                goto __end;
        }
        memset(test_buf, 0xcd, msg_size);

	err = posix_memalign(&reg_buf, page_size, msg_size);
        if (err) {
                printf("reg mem allocation failed with errno : %d\n", errno);
                goto __end;
        }
        memset(reg_buf, 0x0, msg_size);

	offset = scif_register(newepd,
				reg_buf,
				msg_size,
				0,
				SCIF_PROT_READ | SCIF_PROT_WRITE,
				0);
        if (offset < 0) {
                printf("register failed with errno : %d\n", errno);
                goto __end;
        }
	printf("Registerd at 0x%lx offset\n", offset);	

sleep(2);

	/* Verify sent, received data. Data verification is optional/usage dependent */
	for (i = 0; i < (msg_size/sizeof(int)); i++) {
		if (((int*)test_buf)[i] != ((int*)reg_buf)[i]) {
			printf("data mismatch test_buf[%d] 0x%x reg_buf[%d] 0x%x\n",
				i, ((int*)test_buf)[i], i, ((int*)reg_buf)[i]);
			
			errno = -1;
			goto __end;
		}
	}
	errno = 0;



__end:
	if (test_buf != NULL)
		free(test_buf);


	err = scif_unregister(newepd, offset, msg_size);
	if (err < 0)
		printf("scif_unregister failed with error %d\n", errno);

	if (reg_buf != NULL)
		free(reg_buf);

	/* scif_close : closes the end pt, when successful returns 0 */
	if ((scif_close(epd) != 0) || (scif_close(newepd) != 0)) {
		printf("scif_close failed with error %d\n", errno);
		exit(1);
	}

	if (errno == 0)
		printf("======== Program Success ========\n");
	else
		printf("======== Program Failed ======== %d\n", errno);

	return errno;
}
