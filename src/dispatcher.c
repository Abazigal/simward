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

#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>

#include "dispatcher.h"
#include "forward.h"


static void rearrange(forward ** flist, struct pollfd *plist)
{
    return;
}

static int dispatch_tcp_loop(int lsock, struct sockaddr_in *dest,
			     int max, forward ** flist,
			     struct pollfd *plist)
{
    int nbconnection = 0, plist_size = max * 2 + 1, again = 1;
    int i, tmp, csock, rsock, events;
    struct sockaddr_in caddr;


    while (again) {
	events = poll(plist, plist_size, -1);
	for (i = 0; i < plist_size && events > 0; ++i) {
	    if (plist[i].revents == 0)
		continue;

	    --events;

	    if (i == 0 && (plist[i].revents & POLLIN)) {
		/* New user command */

	    } else if (i == 1) {
		/* New client for us */

		/* Establish connection with client */
		tmp = sizeof(caddr);
		csock =
		    accept(lsock, (struct sockaddr *) &caddr,
			   (socklen_t *) & tmp);
		if (csock == -1) {
		    perror("accept");
		    continue;
		}

		/* Check connection limit */
		if (nbconnection >= max) {
		    fprintf(stderr,
			    "Can't accept more connection; closing new one ...\n");
		    close(csock);
		    continue;
		}

		/* Connect to remote host */
		rsock = socket(AF_INET, SOCK_STREAM, 0);
		if (rsock == -1) {
		    perror("socket");
		    close(csock);
		    continue;
		}

		if (connect
		    (rsock, (const struct sockaddr *) dest,
		     sizeof(struct sockaddr_in)) == -1) {
		    perror("connect");
		    close(csock);
		    continue;
		}


		/* Register forward */
		flist[nbconnection] = new_tcp_forward(csock, rsock);
		if (flist[nbconnection] == NULL) {
		    close(rsock);
		    close(csock);
		    continue;
		}

		plist[P_IDX_C(nbconnection)].fd = csock;
		plist[P_IDX_R(nbconnection)].fd = rsock;


		++nbconnection;
	    } else {
	    }
	}

	/* Check flist */
	if (nbconnection < max && flist[nbconnection] != NULL)
	    rearrange(flist, plist);

    }

    return 1;
}

static int dispatch_udp_loop(int lsock, struct sockaddr_in *dest,
			     int max, forward ** flist,
			     struct pollfd *plist)
{
    return 1;
}

int dispatcher(int lsock, struct sockaddr_in *dest, int type, int max)
{
    int ret, plist_size;

    /* Forward list */
    forward **flist = (forward **) malloc(sizeof(forward *) * max);
    if (flist == NULL) {
	perror("malloc");
	return 0;
    }
    for (ret = 0; ret < max; ++ret) {
	flist[ret] = NULL;
    }

    /* Polld list */
    if (type == SOCK_STREAM)
	plist_size = max * 2 + 1;
    else
	plist_size = max + 1;

    struct pollfd *plist =
	(struct pollfd *) malloc(plist_size * sizeof(struct pollfd));

    if (plist == NULL) {
	perror("malloc");
	return 0;
    }

    for (ret = 0; ret < plist_size; ++ret) {
	plist[ret].fd = -1;
    }

    /* Watch for user command */
    plist[0].fd = STDIN_FILENO;
    plist[0].events = POLLIN;

    /* Watch for new client */
    plist[1].fd = lsock;
    plist[1].events = POLLIN;


    /* Main dispatcher loop */
    if (type == SOCK_STREAM)
	ret = dispatch_tcp_loop(lsock, dest, max, flist, plist);
    else
	ret = dispatch_udp_loop(lsock, dest, max, flist, plist);



    free(plist);
    free(flist);
    return ret;
}
