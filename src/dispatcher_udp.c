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
#include <limits.h>
#include <time.h>

#include <sys/socket.h>

#include "dispatcher.h"
#include "dispatcher_udp.h"


/*
 * Function: handle_udp_disconnect
 *
 * This function is called when a part (client or remote) has 
 * closed its socket (connection is over). 
 * We close the other connection by closing the other socket, then we
 * clean memory space and forward list associated with this forward.
 * User command "Kill" also rely on this function.
 *
 */
void handle_udp_disconnect(int plidx, tracking_infos * infos)
{
    forward *ftmp = infos->flist[F_IDX(plidx)];

    close(ftmp->rsock);

    free_forward(ftmp);
    infos->flist[F_IDX(plidx)] = NULL;

    infos->plist[P_IDX_C(F_IDX(plidx))].fd = -1;
    infos->plist[P_IDX_R(F_IDX(plidx))].fd = -1;

    --(infos->nbconnection);
}


/*
 * Function: kill_inactive_forward
 * 
 * Find the forward with the oldest last activity
 * and kill it.
 */
static void kill_inactive_forward(tracking_infos * infos)
{
    long min = LONG_MAX;
    int i, idxmin = 0;

    for (i = 0; i < infos->maxconnection; ++i) {
	if (infos->flist[i] != NULL) {
	    if (infos->flist[i]->lastactivity < min) {
		min = infos->flist[i]->lastactivity;
		idxmin = i;
	    }
	}
    }

    handle_udp_disconnect(P_IDX_R(idxmin), infos);
}



/*
 * Function: handle_udp_read_cdata
 *
 * This function is called when we receive a datagram on our
 * listening socket. It can be either a datagram of an active
 * forward or a new "connection".
 *  - In the first case we store the data in the buffer of the
 *    corresponding forward.
 *  - In the second case, we create a new forward with the
 *    data in its buffer.
 *
 */
static void handle_udp_read_cdata(forward * ffoo, tracking_infos * infos)
{
    int i, tmp, rsock;


    /* Receive datagram (without consuming it) */
    tmp = sizeof(struct sockaddr_in);
    ffoo->bufflen = recvfrom(ffoo->csock, ffoo->buffer, BUFFER_SIZE,
			     MSG_PEEK, (struct sockaddr *) ffoo->caddr,
			     (socklen_t *) & tmp);

    /* Find the corresponding forward */
    for (i = 0; i < infos->maxconnection; ++i) {
	if (infos->flist[i] != NULL
	    && memcmp(ffoo->caddr, infos->flist[i]->caddr, tmp) == 0) {
	    break;
	}
    }

    if (i == infos->maxconnection) {
	/* It's an unknown client */

	/* Read (and consume) the message */
	ffoo->bufflen = recvfrom(ffoo->csock, ffoo->buffer, BUFFER_SIZE, 0,
				 (struct sockaddr *) ffoo->caddr,
				 (socklen_t *) & tmp);

	/* Check connection limit */
	/* In UDP mode, if the limit is reached, we close the 
	   connection with the oldest last activity instead of
	   refusing the new client */
	if (infos->nbconnection >= infos->maxconnection) {
	    kill_inactive_forward(infos);
	    rearrange_forward_list(infos);
	}

	/* Connect to remote host */
	rsock = socket(AF_INET, SOCK_DGRAM, 0);
	if (rsock == -1) {
	    perror("socket");
	    return;
	}

	if (connect(rsock, (const struct sockaddr *) infos->raddr,
		    sizeof(struct sockaddr_in)) == -1) {
	    perror("connect");
	    close(rsock);
	    return;
	}


	/* Register forward */
	infos->flist[infos->nbconnection] =
	    new_forward(ffoo->csock, ffoo->caddr, rsock);
	if (infos->flist[infos->nbconnection] == NULL) {
	    close(rsock);
	    return;
	}

	infos->plist[P_IDX_C(infos->nbconnection)].fd = ffoo->csock;
	infos->plist[P_IDX_C(infos->nbconnection)].events = 0;
	infos->plist[P_IDX_R(infos->nbconnection)].fd = rsock;

	i = infos->nbconnection;
	++(infos->nbconnection);

    } else {
	/* We found the corresponding forward */
	if (ffoo->bufflen <= 0) {
	    /* Probably never reached */
	    handle_udp_disconnect(P_IDX_C(i), infos);
	    return;
	}

	/* If we are not supposed to read new data,
	   return without having read (consume) the message */
	if (infos->flist[i]->status != WAIT_READ_BOTH)
	    return;

	/* Read (and consume) the message */
	ffoo->bufflen = recvfrom(ffoo->csock, ffoo->buffer, BUFFER_SIZE, 0,
				 (struct sockaddr *) ffoo->caddr,
				 (socklen_t *) & tmp);
    }

    /* Update stats */
    infos->flist[i]->totalbytes += ffoo->bufflen;
    infos->flist[i]->lastactivity = (long) time(NULL);


    memcpy(infos->flist[i]->buffer, ffoo->buffer, ffoo->bufflen);
    infos->flist[i]->bufflen = ffoo->bufflen;
    infos->flist[i]->status = WAIT_WRITE_R;
    infos->plist[P_IDX_R(i)].events = POLLOUT;
}


/*
 * Function: handle_udp_read_rdata
 *
 * This function is called when the remote host has sent data.
 * We read those data into the forward buffer, and tells that it must
 * be sent through the other part' socket.
 *
 */
static void handle_udp_read_rdata(int plidx, tracking_infos * infos)
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

	handle_udp_disconnect(plidx, infos);
	return;
    }

    /* Update stats */
    ftmp->totalbytes += ftmp->bufflen;
    ftmp->lastactivity = (long) time(NULL);

    infos->plist[P_IDX_C(F_IDX(plidx))].events = POLLOUT;
    infos->plist[P_IDX_R(F_IDX(plidx))].events = 0;
    ftmp->status = WAIT_WRITE_C;
}


/*
 * Function: handle_udp_write_data
 *
 * This function is called when a part (client or remote) has to send 
 * datas that are waiting in the buffer.
 * We write those datas to the concerned socket and if all the datas 
 * have been send, we go back in "Listening" state (WAIT_READ_BOTH).
 *
 */
static void handle_udp_write_data(int plidx, tracking_infos * infos)
{
    int tmp;
    forward *ftmp = infos->flist[F_IDX(plidx)];

    if (ftmp->status == WAIT_WRITE_C) {
	tmp = sendto(ftmp->csock, ftmp->buffer, ftmp->bufflen,
		     0, (const struct sockaddr *) (ftmp->caddr),
		     sizeof(struct sockaddr_in));
    } else {
	tmp = write(ftmp->rsock, ftmp->buffer, ftmp->bufflen);
    }

    /* Checks error or closed connection */
    if (tmp <= 0) {
	if (tmp < 0)
	    perror("write");

	handle_udp_disconnect(plidx, infos);
	return;
    }

    if (tmp == ftmp->bufflen) {
	/* All the datas have been sent */
	infos->plist[P_IDX_C(F_IDX(plidx))].events = 0;
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
 * Function: dispatch_udp_loop
 *
 * This function checks, for each forward, if the next action to 
 * perform (read datas, send to remote/client,...) is possible. 
 * If yes, it calls the corresponding handling function.
 *
 * Moreover, it keeps an eye on user commands.
 *
 */
int dispatch_udp_loop(int lsock, tracking_infos * infos)
{
    int plist_size = infos->maxconnection + 2, again = 1;
    int i, events;

    struct sockaddr_in foo_addr;
    forward *foo_forward = new_forward(lsock, &foo_addr, -1);


    while (again) {
	events = poll(infos->plist, plist_size, -1);
	for (i = 0; i < plist_size && events > 0; ++i) {
	    if (infos->plist[i].revents == 0)
		continue;

	    --events;

	    if (i == 0 && (infos->plist[i].revents & POLLIN)) {
		/* New user command */
		if (handle_user_command(infos))
		    again = 0;
	    } else if (i == 1) {
		/* We have data to read, maybe a new client */
		handle_udp_read_cdata(foo_forward, infos);
	    } else if (infos->plist[i].revents & POLLOUT) {
		/* We have data to send */
		handle_udp_write_data(i, infos);
	    } else if (infos->plist[i].revents & POLLIN) {
		/* We have data to read */
		handle_udp_read_rdata(i, infos);
	    }
	}

	/* Check flist */
	if (infos->nbconnection < infos->maxconnection
	    && infos->flist[infos->nbconnection] != NULL)
	    rearrange_forward_list(infos);
    }

    for (i = 0; i < infos->maxconnection; ++i) {
	if (infos->flist[i] != NULL) {
	    /* Close connection with remote */
	    close(infos->flist[i]->rsock);

	    /* Free memory space */
	    free_forward(infos->flist[i]);
	}
    }

    return 1;
}
