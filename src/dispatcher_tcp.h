#ifndef DISPATCHER_TCP_H
#define DISPATCHER_TCP_H

void handle_tcp_disconnect(int plidx, tracking_infos * infos);
int dispatch_tcp_loop(int lsock, tracking_infos * infos);
#endif
