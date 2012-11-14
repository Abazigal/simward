/*
 * Simward is a command line program for easy TCP / UDP port 
 * forwarding with (optional) automatic data replacement.
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. 
 *
 *
 * Copyright 2012 Sylvain Rodon
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>


#define IN_DISPATCHER_C

#include "dispatcher.h"
#include "dispatcher_tcp.h"
#include "dispatcher_udp.h"

void handle_user_command(int *again, tracking_infos * infos)
{
    char cmd[10];
    int i;
    fgets(cmd, 10, stdin);

    if (strncasecmp(cmd, "list", 4) == 0) {
	printf("Active forwards (%d/%d max):\n",
	       infos->nbconnection, infos->maxconnection);

	for (i = 0; i < infos->maxconnection; ++i) {
	    if (infos->flist[i] != NULL) {
		printf("\t%s:%hu <-> ",
		       inet_ntoa(infos->flist[i]->caddr->sin_addr),
		       ntohs(infos->flist[i]->caddr->sin_port));
		printf("%s:%hu ",
		       inet_ntoa(infos->raddr->sin_addr),
		       ntohs(infos->raddr->sin_port));

		switch (infos->flist[i]->status) {
		case WAIT_WRITE_C:
		    printf("(<-DATA)");
		    break;
		case WAIT_WRITE_R:
		    printf("(DATA->)");
		    break;
		default:
		    printf("(->DATA<-)");
		}
		printf("\n");
	    }
	}
	printf("\n> ");
	fflush(stdout);
    } else if (strncasecmp(cmd, "quit", 4) == 0
	       || strncasecmp(cmd, "exit", 4) == 0) {
	printf("Closing ...\n");
	*again = 0;
    } else {
	printf("Unknown command\n");
	printf("\n> ");
	fflush(stdout);
    }

}

int dispatcher(int lsock, struct sockaddr_in *dest, int type, int max)
{
    int ret, plist_size;
    tracking_infos infos;

    /* Some init stuff */
    infos.nbconnection = 0;
    infos.maxconnection = max;
    infos.raddr = dest;

    /* Forward list */
    infos.flist = (forward **) malloc(sizeof(forward *) * max);
    if (infos.flist == NULL) {
	perror("malloc");
	return 0;
    }
    for (ret = 0; ret < max; ++ret) {
	infos.flist[ret] = NULL;
    }


    /* Polld list */
    if (type == SOCK_STREAM)
	plist_size = max * 2 + 2;
    else
	plist_size = max + 2;

    infos.plist =
	(struct pollfd *) malloc(plist_size * sizeof(struct pollfd));

    if (infos.plist == NULL) {
	perror("malloc");
	return 0;
    }

    for (ret = 0; ret < plist_size; ++ret) {
	infos.plist[ret].fd = -1;
    }


    /* Watch for user command */
    infos.plist[0].fd = STDIN_FILENO;
    infos.plist[0].events = POLLIN;

    /* Watch for new client */
    infos.plist[1].fd = lsock;
    infos.plist[1].events = POLLIN;


    /* Main dispatcher loop */
    if (type == SOCK_STREAM)
	ret = dispatch_tcp_loop(lsock, &infos);
    else
	ret = dispatch_udp_loop(lsock, &infos);


    /* Cleaning memory */
    free(infos.plist);
    free(infos.flist);

    return ret;
}
