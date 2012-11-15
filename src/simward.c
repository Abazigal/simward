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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "simward.h"
#include "dispatcher.h"

int main(int argc, char **argv)
{
    /* Treat command line */
    parameters params;
    memset(&params, 0, sizeof(parameters));
    if (!parse_cmdline(argc, argv, &params))
	return EXIT_FAILURE;


    /* Create the sockaddr of the remote host */
    struct sockaddr_in raddr;
    struct hostent *hresolved;
    memset(&raddr, 0, sizeof(struct sockaddr_in));
    raddr.sin_family = AF_INET;
    raddr.sin_port = htons(params.rport);
    if ((hresolved = gethostbyname(params.rhost)) == NULL) {
	fprintf(stderr, "Invalid destination host\n");
	return EXIT_FAILURE;
    }
    memcpy(&(raddr.sin_addr), hresolved->h_addr, sizeof(char *));


    /* Create our listening socket, on which clients will connect */
    int lsock;
    lsock = socket(AF_INET,
		   (params.various & tFLAG) ? SOCK_STREAM : SOCK_DGRAM, 0);
    if (lsock == -1) {
	perror("socket");
	return EXIT_FAILURE;
    }

    /* Bind it to the given local port and listen for incoming connections */
    struct sockaddr_in laddr;
    memset(&laddr, 0, sizeof(struct sockaddr_in));
    laddr.sin_family = AF_INET;
    laddr.sin_addr.s_addr = INADDR_ANY;
    laddr.sin_port = htons(params.lport);
    if (bind(lsock, (const struct sockaddr *) &laddr, sizeof(laddr)) == -1) {
	perror("bind");
	return EXIT_FAILURE;
    }

    if (params.various & tFLAG) {
	if (listen(lsock, BACKLOG_SZ) == -1) {
	    perror("listen");
	    return EXIT_FAILURE;
	}
    }


    /* Display prompt */
    printf("> ");
    fflush(stdout);

    /* Do the stuff ... */
    dispatcher(lsock, &raddr,
	       (params.various & tFLAG) ? SOCK_STREAM : SOCK_DGRAM,
	       params.max);


    close(lsock);
    return EXIT_SUCCESS;
}


/*
 * Function: parse_cmdline
 *
 * Parses arguments on the command line, checks coherency,
 * and fills the 'parameters' structure.
 */
int parse_cmdline(int argc, char **argv, parameters * params)
{
    int i;

    while ((i = getopt(argc, argv, "tum:")) > 0) {
	switch (i) {
	case 't':
	    if (params->various & uFLAG) {
		fprintf(stderr, "Can't specify -t with -u\n");
		return 0;
	    }
	    params->various |= tFLAG;
	    break;
	case 'u':
	    if (params->various & tFLAG) {
		fprintf(stderr, "Can't specify -u with -t\n");
		return 0;
	    }
	    params->various |= uFLAG;
	    break;
	case 'm':
	    params->various |= mFLAG;
	    if (sscanf(optarg, "%d", &(params->max)) != 1
		|| params->max < 1) {
		fprintf(stderr, "Invalid maximum: %s\n", optarg);
		return 0;
	    }
	    break;
	case ':':
	    fprintf(stderr, "Missing argument for option: %c\n\n", optopt);
	    usage();
	    return 0;
	default:
	    fprintf(stderr, "Unknown option: %c\n\n", optopt);
	    usage();
	    return 0;
	}
    }

    if (optind + 3 != argc) {
	fprintf(stderr, "Too few arguments\n\n");
	usage();
	return 0;
    }

    if (sscanf(argv[optind], "%d", &(params->lport)) != 1
	|| params->lport < 1) {
	fprintf(stderr, "Invalid port: %s\n", argv[optind]);
	return 0;
    }

    params->rhost = argv[optind + 1];

    if (sscanf(argv[optind + 2], "%d", &(params->rport)) != 1
	|| params->rport < 1) {
	fprintf(stderr, "Invalid port: %s\n", argv[optind + 2]);
	return 0;
    }

    if (!(params->various & (tFLAG | uFLAG))) {
	params->various |= tFLAG;
    }

    if (!(params->various & (mFLAG))) {
	params->max = DEFAULT_MAX;
    }

    return 1;
}


/*
 * Function: usage
 *
 * Print usage.
 */
void usage()
{
    fprintf(stderr, "Usage: simward [-t|-u] [-m <max>]\n");
    fprintf(stderr, "\t<local_port> <remote_host> <remote_port>\n");

    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "\t-t Use TCP ports (default)\n");
    fprintf(stderr, "\t-u Use UDP ports\n");
    fprintf(stderr, "\t-m Maximum number of simultaneous connections\n");
}
