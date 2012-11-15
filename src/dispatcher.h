#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <netinet/in.h>
#include <poll.h>
#include "forward.h"

#define P_IDX_C(x) ((x+1)*2)
#define P_IDX_R(x) ((x+1)*2+1)
#define F_IDX(x) ((x/2)-1)

typedef struct {
    forward **flist;
    struct pollfd *plist;

    struct sockaddr_in *raddr;
    int type;

    int nbconnection;
    int maxconnection;
} tracking_infos;

void rearrange_forward_list(tracking_infos * infos);
int handle_user_command(tracking_infos * infos);
int dispatcher(int lsock, struct sockaddr_in *dest, int type, int max);
#endif
