#ifndef FORWARD_H
#define FORWARD_H

#define WAIT_READ_BOTH 1
#define WAIT_WRITE_C 2
#define WAIT_WRITE_R 3

typedef struct {
    union {
	int csock;		/* Used in TCP mode */
	struct sockaddr_in *caddr;	/* Used in UDP mode */
    } client;
    int rsock;

    char status;
} forward;


forward *new_tcp_forward(int csock, int rsock);
forward *new_udp_forward(struct sockaddr_in *caddr, int rsock);
#endif
