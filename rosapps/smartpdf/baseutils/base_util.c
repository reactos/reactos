/* Written by Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   The author disclaims copyright to this source code. */
#include "base_util.h"

void swap_int(int *one, int *two)
{
    int tmp = *one;
    *one = *two;
    *two = tmp;
}

void swap_double(double *one, double *two)
{
    double tmp = *one;
    *one = *two;
    *two = tmp;
}

int MinInt(int one, int two)
{
    if (one < two)
        return one;
    else
        return two;
}

void memzero(void *data, size_t len)
{
    memset(data, 0, len);
}

void *zmalloc(size_t len)
{
    void *data = malloc(len);
    if (data)
        memzero(data, len);
    return data;
}

/* TODO: probably should move to some other file and change name to
   sleep_milliseconds */
void sleep_milliseconds(int milliseconds)
{
#ifdef WIN32
    Sleep((DWORD)milliseconds);
#else
    struct timespec tv;
    int             secs, nanosecs;
    secs = milliseconds / 1000;
    nanosecs = (milliseconds - (secs * 1000)) * 1000;
    tv.tv_sec = (time_t) secs;
    tv.tv_nsec = (long) nanosecs;
    while (1)
    {
        int rval = nanosleep(&tv, &tv);
        if (rval == 0)
            /* Completed the entire sleep time; all done. */
            return;
        else if (errno == EINTR)
            /* Interrupted by a signal. Try again. */
            continue;
        else
            /* Some other error; bail out. */
            return;
    }
    return;
#endif
}

/* milli-second timer */
#ifdef _WIN32
void ms_timer_start(ms_timer *timer)
{
    assert(timer);
    if (!timer)
        return;
    QueryPerformanceCounter(&timer->start);
}
void ms_timer_stop(ms_timer *timer)
{
    assert(timer);
    if (!timer)
        return;
    QueryPerformanceCounter(&timer->end);
}

double ms_timer_time_in_ms(ms_timer *timer)
{
    LARGE_INTEGER   freq;
    double          time_in_secs;
    QueryPerformanceFrequency(&freq);
    time_in_secs = (double)(timer->end.QuadPart-timer->start.QuadPart)/(double)freq.QuadPart;
    return time_in_secs * 1000.0;
}
#else
typedef struct ms_timer {
    struct timeval    start;
    struct timeval    end;
} ms_timer;

void ms_timer_start(ms_timer *timer)
{
    assert(timer);
    if (!timer)
        return;
    gettimeofday(&timer->start, NULL);
}

void ms_timer_stop(ms_timer *timer)
{
    assert(timer);
    if (!timer)
        return;
    gettimeofday(&timer->end, NULL);
}

double ms_timer_time_in_ms(ms_timer *timer)
{
    double timeInMs;
    time_t seconds;
    int    usecs;

    assert(timer);
    if (!timer)
        return 0.0;
    /* TODO: this logic needs to be verified */
    seconds = timer->end.tv_sec - timer->start.tv_sec;
    usecs = timer->end.tv_usec - timer->start.tv_usec;
    if (usecs < 0) {
        --seconds;
        usecs += 1000000;
    }
    timeInMs = (double)seconds*(double)1000.0 + (double)usecs/(double)1000.0;
    return timeInMs;
}
#endif


