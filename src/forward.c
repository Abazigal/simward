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
#include <time.h>

#include "forward.h"

/*
 * Function: free_forward
 *
 * Clean all the allocated memory space 
 * concerning the given forward.
 */
void free_forward(forward * f)
{
    free(f->caddr);
    free(f->buffer);
    free(f);
}


/*
 * Function: new_forward
 *
 * Allocate a new forward 'object' and fill it with
 * the given informations.
 */
forward *new_forward(int csock, struct sockaddr_in *caddr, int rsock)
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
    f->totalbytes = 0;
    f->lastactivity = (long) time(NULL);
    f->csock = csock;
    f->rsock = rsock;
    f->status = WAIT_READ_BOTH;
    return f;
}
