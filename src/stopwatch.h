#ifndef __STOPWATCH_H__
#define __STOPWATCH_H__

#include <sys/time.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <climits>

typedef struct {
	struct timeval start, end;
} stopwatch;

stopwatch *sw_create();
int sw_destroy(stopwatch *sw);
int sw_start(stopwatch *sw);
int sw_end(stopwatch *sw);
int sw_print(stopwatch *sw);
time_t sw_get_usec(stopwatch *sw);

#endif
