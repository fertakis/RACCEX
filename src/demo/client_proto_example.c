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
#include <signal.h>
#include <unistd.h>
#include <netdb.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include "common.h"
#include "amessage.pb-c.h"

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
ssize_t insist_write(int fd, void *buf, size_t cnt)
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

int main(int argc, char *argv[])
{
    int sd, port;
    ssize_t n;
    AMessage msg = AMESSAGE__INIT;
    void *buf;
    unsigned len;
    char *hostname;
    struct hostent *hp;
    struct sockaddr_in sa;

    if (argc != 4 && argc !=5) {
        fprintf(stderr, "Usage: %s hostname port a [b]\n", argv[0]);
        exit(1);
    }
    hostname = argv[1];
    port = atoi(argv[2]); /* Needs better error checking */

    /* Create TCP/IP socket, used as main chat channel */
    if ((sd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }
    fprintf(stderr, "Created TCP socket\n");

    /* Look up remote hostname on DNS */
    if ( !(hp = gethostbyname(hostname))) {
        printf("DNS lookup failed for host %s\n", hostname);
        exit(1);
    }

    /* Connect to remote TCP port */
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    memcpy(&sa.sin_addr.s_addr, hp->h_addr, sizeof(struct in_addr));
    fprintf(stderr, "Connecting to remote host... "); 
    fflush(stderr);
    if (connect(sd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
        perror("connect");
        exit(1);
    }
    fprintf(stderr, "Connected.\n");

    /* Be careful with buffer overruns, ensure NUL-termination 
       strncpy(buf, HELLO_THERE, sizeof(buf));
       buf[sizeof(buf) - 1] = '\0';*/

    //Initialise command
    msg.a = atoi(argv[3]);
    if (argc == 5) { msg.has_b = 1; msg.b = atoi(argv[4]); }
    len = amessage__get_packed_size(&msg);

    buf = malloc(len);
    amessage__pack(&msg,buf);

    /* Send command */
    if (insist_write(sd, buf, sizeof(buf)) != sizeof(buf)) {
        perror("write");
        exit(1);
    }

    fprintf(stderr,"command send sucesfully\n");

    fflush(stdout);

    /*
     * Let the remote know we're not going to write anything else.
     * Try removing the shutdown() call and see what happens.
     */
    if (shutdown(sd, SHUT_WR) < 0) {
        perror("shutdown");
        exit(1);
    }

    fprintf(stderr, "\nDone.\n");
    return 0;
}
