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

#include "dispatcher.h"
#include "dispatcher_tcp.h"


/*
 * Function: handle_new_tcp_client
 *
 * This function is called when data are readable on the listening socket,
 * which means that a new client is attempting to connect.
 * This functions "registers" that client by adding a new Forward object
 * to the list of managed forwards.
 *
 */
static void handle_new_tcp_client(int lsock, tracking_infos * infos)
{
    int tmp, csock, rsock;
    struct sockaddr_in caddr;

    /* Accept connection with client */
    tmp = sizeof(caddr);
    csock = accept(lsock, (struct sockaddr *) &caddr, (socklen_t *) & tmp);
    if (csock == -1) {
	perror("accept");
	return;
    }

    /* Check connection limit */
    if (infos->nbconnection >= infos->maxconnection) {
	fprintf(stderr,
		"Can't accept more connection; closing new one ...\n");
	close(csock);
	return;
    }

    /* Connect to remote host */
    rsock = socket(AF_INET, SOCK_STREAM, 0);
    if (rsock == -1) {
	perror("socket");
	close(csock);
	return;
    }

    if (connect(rsock, (const struct sockaddr *) infos->raddr,
		sizeof(struct sockaddr_in)) == -1) {
	perror("connect");
	close(csock);
	return;
    }


    /* Register a new forward */
    infos->flist[infos->nbconnection] = new_forward(csock, &caddr, rsock);
    if (infos->flist[infos->nbconnection] == NULL) {
	close(rsock);
	close(csock);
	return;
    }

    /* At the beginning, we are waiting for data from both part */
    infos->plist[P_IDX_C(infos->nbconnection)].fd = csock;
    infos->plist[P_IDX_C(infos->nbconnection)].events = POLLIN;
    infos->plist[P_IDX_R(infos->nbconnection)].fd = rsock;
    infos->plist[P_IDX_R(infos->nbconnection)].events = POLLIN;


    ++(infos->nbconnection);
}


/*
 * Function: handle_tcp_disconnect
 *
 * This function is called when a part (client or remote) has 
 * closed its socket (connection is over). 
 * We close the other connection by closing the other socket, then we
 * clean memory space and forward list associated with this forward.
 * User command "Kill" also rely on this function.
 *
 */
void handle_tcp_disconnect(int plidx, tracking_infos * infos)
{
    forward *ftmp = infos->flist[F_IDX(plidx)];

    close(ftmp->csock);
    close(ftmp->rsock);

    free_forward(ftmp);
    infos->flist[F_IDX(plidx)] = NULL;

    infos->plist[P_IDX_C(F_IDX(plidx))].fd = -1;
    infos->plist[P_IDX_R(F_IDX(plidx))].fd = -1;

    --(infos->nbconnection);
}


/*
 * Function: handle_tcp_read_data
 *
 * This function is called when a part (client or remote) has sent data.
 * We read those data into the forward buffer, and tells that it must
 * be sent through the other part' socket.
 *
 */
static void handle_tcp_read_data(int plidx, tracking_infos * infos)
{
    forward *ftmp = infos->flist[F_IDX(plidx)];

    /* If we are not supposed to read, we don't */
    if (ftmp->status != WAIT_READ_BOTH)
	return;

    ftmp->bufflen =
	read(infos->plist[plidx].fd, ftmp->buffer, BUFFER_SIZE);

    /* Checks error or closed connection */
    if (ftmp->bufflen <= 0) {
	if (ftmp->bufflen < 0)
	    perror("read");

	handle_tcp_disconnect(plidx, infos);
	return;
    }

    /* Update stats */
    ftmp->totalbytes += ftmp->bufflen;
    ftmp->lastactivity = (long) time(NULL);

    if (ftmp->rsock == infos->plist[plidx].fd) {
	/* We just read from the remote, so we have to send to the client */
	infos->plist[P_IDX_C(F_IDX(plidx))].events = POLLOUT;
	infos->plist[P_IDX_R(F_IDX(plidx))].events = 0;
	ftmp->status = WAIT_WRITE_C;
    } else {
	/* We just read from the client, so we have to send to the remote */
	infos->plist[P_IDX_C(F_IDX(plidx))].events = 0;
	infos->plist[P_IDX_R(F_IDX(plidx))].events = POLLOUT;
	ftmp->status = WAIT_WRITE_R;
    }
}


/*
 * Function: handle_tcp_write_data
 *
 * This function is called when a part (client or remote) has to send 
 * datas that are waiting in the buffer.
 * We write those datas to the concerned socket and if all the datas 
 * have been send, we go back in "Listening" state (WAIT_READ_BOTH).
 *
 */
static void handle_tcp_write_data(int plidx, tracking_infos * infos)
{
    int tmp;
    forward *ftmp = infos->flist[F_IDX(plidx)];

    if (ftmp->status == WAIT_WRITE_C) {
	tmp = write(ftmp->csock, ftmp->buffer, ftmp->bufflen);
    } else {
	tmp = write(ftmp->rsock, ftmp->buffer, ftmp->bufflen);
    }

    /* Checks error or closed connection */
    if (tmp <= 0) {
	if (tmp < 0)
	    perror("write");

	handle_tcp_disconnect(plidx, infos);
	return;
    }


    if (tmp == ftmp->bufflen) {
	/* All the datas have been sent */
	infos->plist[P_IDX_C(F_IDX(plidx))].events = POLLIN;
	infos->plist[P_IDX_R(F_IDX(plidx))].events = POLLIN;
	ftmp->status = WAIT_READ_BOTH;
    } else {
	/* There are remaining datas that we will send later */
	memmove((char *) (ftmp->buffer), (char *) (ftmp->buffer + tmp),
		ftmp->bufflen - tmp);
	ftmp->bufflen -= tmp;
    }
}


/*
 * Function: dispatch_tcp_loop
 *
 * This function checks, for each forward, if the next action to 
 * perform (read datas, send to remote/client,...) is possible. 
 * If yes, it calls the corresponding handling function.
 *
 * Moreover, it keeps an eye on new client connections 
 * and user commands.
 *
 */
int dispatch_tcp_loop(int lsock, tracking_infos * infos)
{
    int plist_size = infos->maxconnection * 2 + 2, again = 1;
    int i, events;


    while (again) {
	events = poll(infos->plist, plist_size, -1);
	for (i = 0; i < plist_size && events > 0; ++i) {
	    if (infos->plist[i].fd == -1 || infos->plist[i].revents == 0)
		continue;

	    --events;

	    if (i == 0) {
		/* New user command */
		if (handle_user_command(infos))
		    again = 0;
	    } else if (i == 1) {
		/* New client for us */
		handle_new_tcp_client(lsock, infos);
	    } else if (infos->plist[i].revents & POLLOUT) {
		/* We have data to send */
		handle_tcp_write_data(i, infos);
	    } else if (infos->plist[i].revents & POLLIN) {
		/* We have data to receive */
		handle_tcp_read_data(i, infos);
	    }
	}

	/* Check flist */
	if (infos->nbconnection < infos->maxconnection
	    && infos->flist[infos->nbconnection] != NULL)
	    rearrange_forward_list(infos);
    }


    /* User asked to quit; close/free everything */
    for (i = 0; i < infos->maxconnection; ++i) {
	if (infos->flist[i] != NULL) {
	    /* Close connections */
	    close(infos->flist[i]->rsock);
	    close(infos->flist[i]->csock);

	    /* Clean memory */
	    free_forward(infos->flist[i]);
	}
    }

    return 1;
}
