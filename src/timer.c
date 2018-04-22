#include <stdio.h>
#include <sys/time.h>

unsigned long long get_cycles(void)
{
        struct timeval tv;
        gettimeofday(&tv,NULL);
        return tv.tv_sec * 1000000 + tv.tv_usec;
}
