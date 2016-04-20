#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#ifndef NETDATA_POPEN_H
#define NETDATA_POPEN_H 1

#define PIPE_READ 0
#define PIPE_WRITE 1

extern FILE *mypopen(const char *command, pid_t *pidptr);
extern int mypclose(FILE *fp, pid_t pid);

#endif /* NETDATA_POPEN_H */
