#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <netinet/in.h>
#include <poll.h>
#include "forward.h"

typedef struct {
    forward **flist;
    struct pollfd *plist;

    struct sockaddr_in *raddr;

    int nbconnection;
    int maxconnection;
} tracking_infos;

void handle_user_command(int *again, tracking_infos * infos);
int dispatcher(int lsock, struct sockaddr_in *dest, int type, int max);
#endif
