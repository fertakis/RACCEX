/*
 * client.c
 * Client application for Remote Intel PHI Execution
 *
 * Konstantinos Fertakis <kfertak@cslab.ece.ntua.gr>
 */

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>


#include <arpa/inet.h>
#include <netinet/in.h>

#include "common.h"
#include "client.h"
#include "common.pb-c.h"


size_t encode_message(void **result, int msg_type, void *payload) {
	size_t buf_size;
	uint32_t msg_length, msg_len_n;
	Cookie message = COOKIE__INIT;
	void *buffer, *msg_buffer; 

	gdprintf("Encoding message data...\n");
	message.type = msg_type;

	switch (msg_type) {
		case PHI_CMD:
		case PHI_CMD_RESULT:
			message.phi_cmd = payload;
			break;
	}

	msg_length = cookie__get_packed_size(&message);
	msg_buffer = malloc_safe(msg_length);
	cookie__pack(&message, msg_buffer);
	msg_len_n = htonl(msg_length);

	buf_size = msg_length + sizeof(msg_len_n);
	buffer = malloc_safe(buf_size);
	memcpy(buffer, &msg_len_n, sizeof(msg_len_n));
	memcpy(buffer+sizeof(msg_len_n), msg_buffer, msg_length);

	*result = buffer;

	return buf_size;
}
