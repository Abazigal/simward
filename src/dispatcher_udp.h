#ifndef DISPATCHER_UDP_H
#define DISPATCHER_UDP_H

#ifndef IN_DISPATCHER_C
#define P_IDX_C(x) 1
#define P_IDX_R(x) (x+2)
#define F_IDX(x) (x-2)
#endif

int dispatch_udp_loop(int lsock, tracking_infos * infos);
#endif
