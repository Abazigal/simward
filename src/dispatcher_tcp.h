#ifndef DISPATCHER_TCP_H
#define DISPATCHER_TCP_H

#ifndef IN_DISPATCHER_C
#define P_IDX_C(x) ((x+1)*2)
#define P_IDX_R(x) ((x+1)*2+1)
#define F_IDX(x) ((x/2)-1)
#endif

int dispatch_tcp_loop(int lsock, tracking_infos * infos);
#endif
