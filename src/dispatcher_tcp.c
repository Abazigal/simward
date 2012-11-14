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

#include "dispatcher.h"
#include "dispatcher_tcp.h"


static void rearrange_tcp_forward_list(tracking_infos * infos)
{
    int start, end = F_IDX(infos->maxconnection - 1);
    for (start = 0; start < end; ++start) {
	if (infos->flist[start] != NULL)
	    continue;

	for (; end > start && infos->flist[end] == NULL; --end);
	if (infos->flist[end] != NULL) {
	    infos->flist[start] = infos->flist[end];
	    infos->flist[end] = NULL;

	    memcpy((void *) (infos->plist) + P_IDX_C(start),
		   (void *) (infos->plist) + P_IDX_C(end),
		   sizeof(struct pollfd) * 2);

	    infos->plist[P_IDX_C(end)].fd = -1;
	    infos->plist[P_IDX_R(end)].fd = -1;
	}
    }
}


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


    /* Register forward */
    infos->flist[infos->nbconnection] =
	new_tcp_forward(csock, &caddr, rsock);
    if (infos->flist[infos->nbconnection] == NULL) {
	close(rsock);
	close(csock);
	return;
    }

    infos->plist[P_IDX_C(infos->nbconnection)].fd = csock;
    infos->plist[P_IDX_C(infos->nbconnection)].events = POLLIN;
    infos->plist[P_IDX_R(infos->nbconnection)].fd = rsock;
    infos->plist[P_IDX_R(infos->nbconnection)].events = POLLIN;


    ++(infos->nbconnection);
}


static void handle_tcp_disconnect(int plidx, tracking_infos * infos)
{
    forward *ftmp = infos->flist[F_IDX(plidx)];

    if (ftmp->rsock == infos->plist[plidx].fd) {
	close(ftmp->csock);
    } else {
	close(ftmp->rsock);
    }

    free_tcp_forward(ftmp);
    infos->flist[F_IDX(plidx)] = NULL;

    infos->plist[P_IDX_C(F_IDX(plidx))].fd = -1;
    infos->plist[P_IDX_R(F_IDX(plidx))].fd = -1;

    --(infos->nbconnection);
}


static void handle_tcp_read_data(int plidx, tracking_infos * infos)
{
    forward *ftmp = infos->flist[F_IDX(plidx)];
    ftmp->bufflen =
	read(infos->plist[plidx].fd, ftmp->buffer, BUFFER_SIZE);

    if (ftmp->bufflen <= 0) {
	if (ftmp->bufflen < 0)
	    perror("read");

	close(infos->plist[plidx].fd);
	handle_tcp_disconnect(plidx, infos);
	return;
    }

    if (ftmp->rsock == infos->plist[plidx].fd) {
	infos->plist[P_IDX_C(F_IDX(plidx))].events = POLLOUT;
	infos->plist[P_IDX_R(F_IDX(plidx))].events = 0;
	ftmp->status = WAIT_WRITE_C;
    } else {
	infos->plist[P_IDX_C(F_IDX(plidx))].events = 0;
	infos->plist[P_IDX_R(F_IDX(plidx))].events = POLLOUT;
	ftmp->status = WAIT_WRITE_R;
    }
}


static void handle_tcp_write_data(int plidx, tracking_infos * infos)
{
    int tmp;
    forward *ftmp = infos->flist[F_IDX(plidx)];

    if (ftmp->status == WAIT_WRITE_C) {
	tmp = write(ftmp->csock, ftmp->buffer, ftmp->bufflen);
    } else {
	tmp = write(ftmp->rsock, ftmp->buffer, ftmp->bufflen);
    }

    if (tmp <= 0) {
	if (tmp < 0)
	    perror("write");

	close(infos->plist[plidx].fd);
	handle_tcp_disconnect(plidx, infos);
	return;
    }

    if (tmp == ftmp->bufflen) {
	infos->plist[P_IDX_C(F_IDX(plidx))].events = POLLIN;
	infos->plist[P_IDX_R(F_IDX(plidx))].events = POLLIN;
	ftmp->status = WAIT_READ_BOTH;
    } else {
	memmove((char *) (ftmp->buffer), (char *) (ftmp->buffer + tmp),
		ftmp->bufflen - tmp);
	ftmp->bufflen -= tmp;
    }
}


int dispatch_tcp_loop(int lsock, tracking_infos * infos)
{
    int plist_size = infos->maxconnection * 2 + 2, again = 1;
    int i, events;


    while (again) {
	events = poll(infos->plist, plist_size, -1);
	for (i = 0; i < plist_size && events > 0; ++i) {
	    if (infos->plist[i].revents == 0)
		continue;

	    --events;

	    if (i == 0 && (infos->plist[i].revents & POLLIN)) {
		/* New user command */
		handle_user_command(&again, infos);
	    } else if (i == 1) {
		/* New client for us */
		handle_new_tcp_client(lsock, infos);
	    } else if (infos->plist[i].revents & POLLHUP) {
		/* Someone has closed the connection */
		handle_tcp_disconnect(i, infos);
	    } else if (infos->plist[i].revents & POLLIN) {
		/* We have data to receive */
		handle_tcp_read_data(i, infos);
	    } else if (infos->plist[i].revents & POLLOUT) {
		/* We have data to send */
		handle_tcp_write_data(i, infos);
	    }
	}

	/* Check flist */
	if (infos->nbconnection < infos->maxconnection
	    && infos->flist[infos->nbconnection] != NULL)
	    rearrange_tcp_forward_list(infos);
    }

    for (i = 0; i < infos->maxconnection; ++i) {
	if (infos->flist[i] != NULL) {
	    /* Close connections */
	    close(infos->flist[i]->rsock);
	    close(infos->flist[i]->csock);

	    /* Free memory space */
	    free_tcp_forward(infos->flist[i]);
	}
    }

    return 1;
}
