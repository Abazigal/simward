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

#include "forward.h"


static void free_forward(forward * f)
{
    free(f->caddr);
    free(f->buffer);
    free(f);
}

void free_tcp_forward(forward * f)
{
    free_forward(f);
}

void free_udp_forward(forward * f)
{
    free_forward(f);
}

static forward *new_forward(int rsock, struct sockaddr_in *caddr)
{
    forward *f = (forward *) malloc(sizeof(forward));
    if (f == NULL) {
	perror("malloc");
	return NULL;
    }

    f->buffer = (char *) malloc(BUFFER_SIZE);
    if (f->buffer == NULL) {
	perror("malloc");
	free(f);
	return NULL;
    }

    f->caddr = (struct sockaddr_in *)
	malloc(sizeof(struct sockaddr_in));
    if (f->caddr == NULL) {
	perror("malloc");
	free(f->buffer);
	free(f);
	return NULL;
    }

    memcpy(f->caddr, caddr, sizeof(struct sockaddr_in));


    f->bufflen = 0;
    f->csock = -1;
    f->rsock = rsock;
    f->status = WAIT_READ_BOTH;
    return f;
}


forward *new_tcp_forward(int csock, struct sockaddr_in * caddr, int rsock)
{
    forward *f = new_forward(rsock, caddr);
    if (f == NULL)
	return NULL;

    f->csock = csock;
    return f;
}


forward *new_udp_forward(struct sockaddr_in * caddr, int rsock)
{
    return new_forward(rsock, caddr);
}
