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

typedef struct timers {
        unsigned long long total;
        unsigned long long val;
        unsigned long cnt;
} scif_timers_t;

unsigned long long get_cycles(void);

#define TIMER_START(tp) do {( tp)->val = get_cycles(); } while (0)
#define TIMER_STOP(tp) do { (tp)->total += get_cycles() - (tp)->val; ++(tp)->cnt; } while (0)
#define TIMER_RESET(tp) do { (tp)->total = (tp)->val = 0; (tp)->cnt = 0; } while (0)
#define TIMER_TOTAL(tp) ((tp)->total)
#define TIMER_COUNT(tp) ((tp)->cnt)
#define TIMER_AVG(tp) ((tp)->cnt ? ((tp)->total / (tp)->cnt) : -1)
#define TICKS_TO_USEC(t) (1000 * t/CYCLES_PER_SEC)

#endif
