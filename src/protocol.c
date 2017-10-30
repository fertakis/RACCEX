/*
 * protocol.c
 * Communication Protocol Infrastructure for Remote Intel PHI Execution
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

/* Insist untill all of the data has been read */
ssize_t insist_read(int fd, void *buf, size_t cnt)
{
    ssize_t ret;
    size_t orig_cnt = cnt;

    while (cnt > 0) {
        ret = read(fd, buf, cnt);
        if(ret <=0)
            return ret;
        buf += ret;
        cnt -= ret;
    }
    return orig_cnt;
}
/* Insist until all of the data has been written */
ssize_t insist_write(int fd, const void *buf, size_t cnt)
{
	ssize_t ret;
	size_t orig_cnt = cnt;
	
	while (cnt > 0) {
	        ret = write(fd, buf, cnt);
	        if (ret < 0)
	                return ret;
	        buf += ret;
	        cnt -= ret;
	}

	return orig_cnt;
}

size_t serialise_message(void **result, int msg_type, void *payload) {
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

int deserialise_message(void **result, void **payload, void *serialised_msg, uint32_t ser_msg_len)
{
	Cookie *msg;

	printf("Deserialising message data...\n");
	msg = cookie__unpack(NULL, ser_msg_len, (uint8_t *)serialised_msg);
	if (msg == NULL) {
		fprintf(stderr, "message unpacking failed\n");
		return -1;
	}
	
	switch (msg->type) {
		case PHI_CMD:
			printf("--------------\nIs PHI_CMD\n");
			*payload = msg->phi_cmd;
			break;
		case PHI_CMD_RESULT:
			printf("--------------\nIs PHI_CMD_RESULT\n");
			*payload = msg->phi_cmd;
			break;
	}
	
	// We can't call this here unless we make a *deep* copy of the
	// message payload...
	//cookie__free_unpacked(msg, NULL);
	*result = msg; 

	return msg->type;

}

void free_deserialised_message(void *msg) {
	printf("Freeing allocated memory for message...\n");
	cookie__free_unpacked((Cookie *)msg, NULL);
}

ssize_t send_message (int socket_fd, void *buffer, size_t len)
{
    printf("Sending %zu bytes...\n",len);
    return insist_write(socket_fd, buffer, len);
}

uint32_t receive_message(void **serialised_msg, int socket_fd) {
	void *buf;
	uint32_t msg_len;
	int ret = 0;

	buf = malloc_safe(sizeof(uint32_t));
	
	// read message length
	insist_read(socket_fd, buf, sizeof(uint32_t));

	msg_len = ntohl(*(uint32_t *)buf);
	printf("Going to read a message of %u bytes...\n", msg_len);
	
	buf = realloc(buf, msg_len);
	
	// read message
	insist_read(socket_fd, buf, msg_len);
	
	*serialised_msg = buf;

	return msg_len;
}
