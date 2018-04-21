#ifndef __SCIFBENCH_TIMER_H__
#define __SCIFBENCH_TIMER_H__

#include <scif.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include<time.h>

typedef struct timers {
        unsigned long long total;
        unsigned long long val;
        unsigned long cnt;
} scif_timers_t;

unsigned long long get_cycles(void)
{
        struct timeval tv;
        gettimeofday(&tv,NULL);
        return tv.tv_sec * 1000000 + tv.tv_usec;
}

static inline unsigned long long get_time()
{
	struct timespec ts;
	unsigned long long time;

	if(clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
		perror("clock gettime");
		exit(EXIT_FAILURE);
	}
	time = ts.tv_sec * 1000000000 + ts.tv_nsec;
	//printf("time: %llu ns\n", time);

	return time;
}

#define TIMER_START(tm) do { (tm)->val = get_time(); } while (0)
#define TIMER_STOP(tm)  do { (tm)->total += get_time() - (tm)->val; ++(tm)->cnt; } while (0)
#define TIMER_GET(tm)   do { (tm)->total += get_time() - (tm)->val; (tm)->val = get_time(); } while (0)
#define TIMER_RESET(tm) do { (tm)->total = (tm)->val = 0; (tm)->cnt = 0; } while (0)
#define TIMER_TOTAL(tm) ((tm)->total)
#define TIMER_COUNT(tm) ((tm)->cnt)
#define TIMER_AVG(tm)   ((tm)->cnt ? ((tm)->total / (tm)->cnt) : 0)

#define TIMER_TO_SEC(t) (t / 1000000000.0)                                                                                                    
#define TIMER_TO_USEC(t) (t / 1000.0)

#endif
