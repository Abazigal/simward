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
#include "dispatcher_udp.h"

/*
static void rearrange_udp_forward_list(tracking_infos *infos)
{
	int start, end=F_IDX(infos->maxconnection - 1);
	for(start=0;start<end;++start) {
		if(infos->flist[start] != NULL)
			continue;

		for(;end>start && infos->flist[end]==NULL;--end);
		if(infos->flist[end] != NULL) {
			infos->flist[start] = infos->flist[end];
			infos->flist[end] = NULL;

			memcpy((void*)(infos->plist)+P_IDX_C(start),
					(void*)(infos->plist)+P_IDX_C(end),
					sizeof(struct pollfd)*2);

			infos->plist[P_IDX_C(end)].fd = -1;
			infos->plist[P_IDX_R(end)].fd = -1;
		}
	}
}


static void handle_new_udp_client (int lsock, tracking_infos *infos) {
	
	++(infos->nbconnection);
}


static void handle_udp_disconnect(int plidx, tracking_infos *infos) {

	--(infos->nbconnection);
}


static void handle_udp_read_data (int plidx, tracking_infos *infos) {
}
*/

int dispatch_udp_loop(int lsock, tracking_infos * infos)
{
    return 1;
}
