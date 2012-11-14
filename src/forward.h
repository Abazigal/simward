#ifndef FORWARD_H
#define FORWARD_H

#include <netinet/in.h>

#define BUFFER_SIZE 3000
#define WAIT_READ_BOTH 1
#define WAIT_WRITE_C 2
#define WAIT_WRITE_R 3

typedef struct {
    int csock;			/* Only used in TCP mode */
    struct sockaddr_in *caddr;
    int rsock;

    char status;

    char *buffer;
    int bufflen;
} forward;


void free_tcp_forward(forward * f);
void free_udp_forward(forward * f);
forward *new_tcp_forward(int csock, struct sockaddr_in *caddr, int rsock);
forward *new_udp_forward(struct sockaddr_in *caddr, int rsock);
#endif
