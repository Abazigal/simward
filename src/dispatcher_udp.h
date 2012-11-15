#ifndef DISPATCHER_UDP_H
#define DISPATCHER_UDP_H

void handle_udp_disconnect(int plidx, tracking_infos * infos);
int dispatch_udp_loop(int lsock, tracking_infos * infos);
#endif
