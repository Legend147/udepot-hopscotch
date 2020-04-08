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

stopwatch *sw_create() {
	stopwatch *sw = (stopwatch *)calloc(1, sizeof(stopwatch));
	return sw;
}

int sw_destroy(stopwatch *sw) {
	free(sw);
	return 0;
}

int sw_start(stopwatch *sw) {
	gettimeofday(&sw->start, NULL);
	return 0;
}

int sw_end(stopwatch *sw) {
	gettimeofday(&sw->end, NULL);
	return 0;
}

int sw_print(stopwatch *sw) {
	time_t usec = (sw->end.tv_sec - sw->start.tv_sec) * 1000000;
	usec += sw->end.tv_usec - sw->start.tv_usec;

	printf("%ld:%ld:%ld (sec:msec:usec)\n",
		usec/1000000, (usec/1000)%1000, usec%1000);

	return 0;
}

time_t sw_get_usec(stopwatch *sw) {
	time_t usec = (sw->end.tv_sec - sw->start.tv_sec) * 1000000;
	usec += sw->end.tv_usec - sw->start.tv_usec;

	if (usec > 1000000) {
		fprintf(stderr, "Stopwatch: usec overflow!");
		return -1;
	}
	return usec;
}

#endif
