/*
 * common.h
 *
 * Remote Intel PHI Execution
 *
 * Konstantinos Fertakis <kfertak@cslab.ece.ntua.gr>
 */

#ifndef _COMMON_H
#define _COMMON_H

/* Compile-time options */
#define TCP_PORT    35001
#define TCP_BACKLOG 5

#define HELLO_THERE "Hello there!"
#define MESSAGE_LENGTH 1000

struct phi_cmd {
    int type;
    char *arg;
    int out;
};

#endif /* _COMMON_H */

