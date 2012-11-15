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
#include <time.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include "dispatcher.h"
#include "dispatcher_tcp.h"
#include "dispatcher_udp.h"


/*
 * Function: rearrange_forward_list
 *
 * Reorganize the list of forwards so that all the NULL box
 * are "packed" together at the end of the list.
 */
void rearrange_forward_list(tracking_infos * infos)
{
    int start, end = F_IDX(infos->maxconnection - 1);

    for (start = 0; start < end; ++start) {
	if (infos->flist[start] != NULL)
	    continue;

	for (; end > start && infos->flist[end] == NULL; --end);
	if (infos->flist[end] != NULL) {
	    infos->flist[start] = infos->flist[end];
	    infos->flist[end] = NULL;

	    memcpy((char *) (infos->plist + P_IDX_C(start)),
		   (char *) (infos->plist + P_IDX_C(end)),
		   sizeof(struct pollfd) * 2);

	    infos->plist[P_IDX_C(end)].fd = -1;
	    infos->plist[P_IDX_R(end)].fd = -1;
	}
    }

/*	printf("-----\n");
    for(start = 0; start<infos->maxconnection;++start) {
	printf("%p\n",infos->flist[start]);
	printf("%d\n",infos->plist[P_IDX_C(start)].fd);
	printf("%d\n",infos->plist[P_IDX_R(start)].fd);
	printf("\n");
    }
*/
}


/*
 * Function: kill_forward
 *
 * Closes the sockets of the forward identified by the 
 * given forward ID.
 * (The ID of a forward is its Remote' socket file descriptor)
 */
static void kill_forward(char *fid, tracking_infos * infos)
{
    int i, id;

    if (sscanf(fid, "%d", &id) != 1) {
	printf("Invalid ID.\n");
	return;
    }

    for (i = 0; i < infos->maxconnection; ++i) {
	if (infos->flist[i] != NULL) {
	    if (infos->flist[i]->rsock == id) {
		if (infos->type == SOCK_STREAM)
		    handle_tcp_disconnect(P_IDX_R(i), infos);
		else
		    handle_udp_disconnect(P_IDX_R(i), infos);

		printf("Forward %d stopped.", id);
		return;
	    }
	}
    }

    printf("There is no forward with this ID.\n");
}


/*
 * Function: list_active_forwards
 *
 * Display informations about each active forwards.
 * (The ID of a forward is its Remote' socket file descriptor)
 */
static void list_active_forwards(tracking_infos * infos)
{
    int i;
    long tmp;

    printf("%s forwards (%d/%d max):\n\n",
	   (infos->type == SOCK_STREAM) ? "Active TCP" : "UDP",
	   infos->nbconnection, infos->maxconnection);

    for (i = 0; i < infos->maxconnection; ++i) {
	if (infos->flist[i] != NULL) {
	    printf("= Forward ID %d =\n", infos->flist[i]->rsock);
	    printf("Members: %s:%hu <-> ",
		   inet_ntoa(infos->flist[i]->caddr->sin_addr),
		   ntohs(infos->flist[i]->caddr->sin_port));
	    printf("%s:%hu\n",
		   inet_ntoa(infos->raddr->sin_addr),
		   ntohs(infos->raddr->sin_port));

	    printf("State: ");
	    switch (infos->flist[i]->status) {
	    case WAIT_WRITE_C:
		printf("Forwarding %d bytes to client\n",
		       infos->flist[i]->bufflen);
		break;
	    case WAIT_WRITE_R:
		printf("Forwarding %d bytes to remote host\n",
		       infos->flist[i]->bufflen);
		break;
	    default:
		printf("Waiting for data\n");
	    }

	    printf("Forwarded: ");
	    if (infos->flist[i]->totalbytes < 1024) {
		printf("%lld bytes\n", infos->flist[i]->totalbytes);
	    } else if (infos->flist[i]->totalbytes < (1 << 20)) {
		printf("%.2lf Kbytes\n",
		       infos->flist[i]->totalbytes / 1024.0);
	    } else {
		printf("%.2lf Mbytes\n",
		       infos->flist[i]->totalbytes / ((double) (1 << 20)));
	    }

	    printf("Last activity: ");
	    tmp = time(NULL) - infos->flist[i]->lastactivity;
	    if (tmp == 0) {
		printf("< 1 sec\n");
	    } else if (tmp < 60) {
		printf("%ld sec\n", tmp);
	    } else {
		printf("%ld min\n", tmp / 60);
	    }

	    printf("\n");
	}
    }
}


/*
 * Function: display_available_commands
 *
 * Display the list of known commands
 */
static void display_available_commands()
{
    printf("Available commands:\n");
    printf("\tlist\t\tShow the list of managed forwards\n");
    printf("\tkill <id>\tStop the specified forward\n");
    printf("\thelp\t\tDisplay this help\n");
    printf("\tquit\t\t(or 'exit'/Ctrl+D) Exit from program\n");
}


/*
 * Function: handle_user_command
 *
 * This function is called when the user has typed something
 * on the standard input. It reads the command and call the
 * corresponding function.
 */
int handle_user_command(tracking_infos * infos)
{
    char cmd[10];
    if (fgets(cmd, 10, stdin) == NULL) {
	printf("Closing ...\n");
	return 1;
    }

    if (strncasecmp(cmd, "list", 4) == 0) {
	list_active_forwards(infos);
    } else if (strncasecmp(cmd, "help", 4) == 0) {
	display_available_commands();
    } else if (strncasecmp(cmd, "kill ", 5) == 0) {
	kill_forward(cmd + 4, infos);
    } else if (strncasecmp(cmd, "quit", 4) == 0
	       || strncasecmp(cmd, "exit", 4) == 0) {
	printf("Closing ...\n");
	return 1;
    } else {
	printf("Unknown command.\n");
	printf("Type 'help' for the list of valid command.\n");
    }

    printf("\n> ");
    fflush(stdout);
    return 0;
}


/*
 * Function: dispatcher
 *
 * This function allocate the structure that will be used to track
 * forwards and various informations.
 * Then it calls the dispatcher loop corresponding to the given type.
 */
int dispatcher(int lsock, struct sockaddr_in *dest, int type, int max)
{
    int ret, plist_size;
    tracking_infos infos;

    /* Some init stuff */
    infos.nbconnection = 0;
    infos.type = type;
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
    plist_size = max * 2 + 2;

    infos.plist =
	(struct pollfd *) malloc(plist_size * sizeof(struct pollfd));

    if (infos.plist == NULL) {
	perror("malloc");
	return 0;
    }

    for (ret = 0; ret < plist_size; ++ret) {
	infos.plist[ret].fd = -1;
	infos.plist[ret].events = 0;
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
