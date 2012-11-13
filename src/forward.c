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
#include <string.h>

#include <netinet/in.h>

#include "forward.h"


static forward *new_forward(int rsock)
{
    forward *f = (forward *) malloc(sizeof(forward));
    if (f == NULL) {
	perror("malloc");
	return NULL;
    }

    f->rsock = rsock;
    f->status = WAIT_READ_BOTH;
    return f;
}

static void free_forward(forward * f)
{
    free(f);
}


forward *new_tcp_forward(int csock, int rsock)
{
    forward *f = new_forward(rsock);
    if (f == NULL)
	return NULL;

    f->client.csock = csock;
    return f;
}


forward *new_udp_forward(struct sockaddr_in * caddr, int rsock)
{
    forward *f = new_forward(rsock);
    if (f == NULL)
	return NULL;

    f->client.caddr = (struct sockaddr_in *)
	malloc(sizeof(struct sockaddr_in));
    if (f->client.caddr == NULL) {
	perror("malloc");
	free_forward(f);
	return NULL;
    }

    memcpy(f->client.caddr, caddr, sizeof(struct sockaddr_in));
    return f;
}
