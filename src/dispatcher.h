#ifndef DISPATCHER_H
#define DISPATCHER_H

#define P_IDX_C(x) ((x+1)*2)
#define P_IDX_R(x) ((x+1)*2+1)
#define F_IDX(x) ((x/2)-1)

int dispatcher(int lsock, struct sockaddr_in *dest, int type, int max);
#endif
