#ifndef FORWARD_H
#define FORWARD_H

#include <netinet/in.h>

#define BUFFER_SIZE 3000
#define WAIT_READ_BOTH 1
#define WAIT_WRITE_C 2
#define WAIT_WRITE_R 3

typedef struct {
    int csock;
    struct sockaddr_in *caddr;
    int rsock;

    char status;
    int lastactivity;
    unsigned long long totalbytes;

    char *buffer;
    int bufflen;
} forward;


void free_forward(forward * f);
forward *new_forward(int csock, struct sockaddr_in *caddr, int rsock);
#endif
