#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <scif.h>

#include "common.h"
#include "timer.h"

static bs_timer tm_recv, tm_send;

scif_epd_t bscif_endp_init_(int port, bool listen)
{
	scif_epd_t epd;
	int conn_port;

	// Create endpoint
	epd = scif_open();
	if (epd == SCIF_OPEN_FAILED) {
		perror("scif_open failed");
		exit(EXIT_FAILURE);
	}

	// Bind endpoint to port
	conn_port = scif_bind(epd, port);
	if (conn_port < 0) {
		perror("scif_bind failed");
		exit(EXIT_FAILURE);
	}
	printf("scif_bind to port %d success\n", conn_port);
	
	if (listen == true) {
		if (scif_listen(epd, LISTEN_QUEUE_LEN) != 0) {
			perror("scif_listen failed");
			exit(EXIT_FAILURE);
		}
	}

	return epd;
}

void bscif_endp_close(scif_epd_t epd)
{
	if (scif_close(epd) != 0) {
		perror("scif_close failed");
		exit(EXIT_FAILURE);
	}
}

void bscif_timers_init()
{
	TIMER_RESET(&tm_recv);
	TIMER_RESET(&tm_send);
}

void bscif_timers_print()
{
	printf("recv time (usec): %lf\nsend time (usec): %lf\n",
			TIMER_TO_USEC(TIMER_AVG(&tm_recv)),
			TIMER_TO_USEC(TIMER_AVG(&tm_send)));
}

int bscif_endp_connect(scif_epd_t epd, bool is_card, int rem_port)
{
	int ret, tries = 10;
	struct scif_portID port_id;

	port_id.node = is_card;
	port_id.port = rem_port;

	while ((ret = scif_connect(epd, &port_id)) < 0) {
		if ((errno == ECONNREFUSED) && (tries > 0)) {
			printf("connection to node %d failed: trial %d\n", port_id.node, tries);
			tries--;
			
			sleep(1);
		} else {
			perror("scif_connect failed");
			break;
		}
	}

	if (ret >= 0)
		printf("conect to node %d success\n", port_id.node);

	return ret;
}

int bscif_endp_accept(scif_epd_t epd, bool is_card, int rem_port)
{
	scif_epd_t new_epd;
	struct scif_portID port_id;

	port_id.node = is_card;
	port_id.port = rem_port;
	
	if (scif_accept(epd, &port_id, &new_epd, SCIF_ACCEPT_SYNC) != 0) {
		perror("scif_accept failed");
		exit(EXIT_FAILURE);
	}

	return new_epd;
}

int bscif_send(scif_epd_t epd, void *msg, int length, int flags)
{
	int bytes; 
	
	TIMER_START(&tm_send);
	bytes = scif_send(epd, msg, length, flags);	
	TIMER_STOP(&tm_send);

	if (bytes < 0) {
		perror("scif_send failed");
	}

	return bytes;
}

int bscif_send_all(scif_epd_t epd, void *msg, int length, int flags)
{
	int bytes, tbytes = 0;

	TIMER_START(&tm_send);
	do {	
		bytes = scif_send(epd, msg+tbytes, length-tbytes, flags);

		if (bytes < 0) {
			perror("scif_send failed");
			tbytes = bytes;
			break;
		}

		tbytes += bytes;
	} while (tbytes < length);
	TIMER_STOP(&tm_send);

	return tbytes;
}

int bscif_recv(scif_epd_t epd, void *msg, int length, int flags)
{
	int bytes; 
	
	TIMER_START(&tm_recv);
	bytes = scif_recv(epd, msg, length, flags);	
	TIMER_STOP(&tm_recv);

	if (bytes < 0) {
		perror("scif_recv failed");
	}

	return bytes;
}

int bscif_recv_all(scif_epd_t epd, void *msg, int length, int flags)
{
	int bytes, tbytes = 0;
	
	TIMER_START(&tm_recv);
	do {	
		bytes = scif_recv(epd, msg+tbytes, length-tbytes, flags);
		if (bytes < 0) {
			perror("scif_send failed");
			tbytes = bytes;
			break;
		}

		tbytes += bytes;
	} while (tbytes < length);
	TIMER_STOP(&tm_recv);

	return tbytes;
}

bool bscif_verify_data(void *sent, void *recvd, int length)
{
	int i;
	
	for (i = 0; i < length; i++) {
		if (((char *)sent)[i] != ((char *)recvd)[i]) {
			fprintf(stderr, "Data mismatch @%d byte\n", i);
			return false;
		}
	}

	return true;
}
