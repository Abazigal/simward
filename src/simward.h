#ifndef SIMWARD_H
#define SIMWARD_H

#define tFLAG 1
#define uFLAG 2
#define mFLAG 4

#define BACKLOG_SZ 10
#define DEFAULT_MAX 10

typedef struct {
    int max;
    int lport;
    int rport;
    char *rhost;

    short various;
} parameters;

int parse_cmdline(int argc, char **argv, parameters * params);
void usage();

#endif
