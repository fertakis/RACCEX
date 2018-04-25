#ifndef PROTOCOL_H
#define PROTOCOL_H
#include "common.pb-c.h"

size_t serialise_message(void **result, int msg_type, PhiCmd *cmd);

int deserialise_message(void **result, void **payload, void *serialised_msg, uint32_t ser_msg_len);

void free_deserialised_message(void *msg);

ssize_t send_message(int socket_fd, void *buffer, size_t len);

uint32_t receive_message(void **serialised_msg, int socket_fd);

ssize_t insist_read(int fd, void *buf, size_t cnt);

ssize_t insist_write(int fd, const void *buf, size_t cnt);

#endif /* PROTOCOL_H */
