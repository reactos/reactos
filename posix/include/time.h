/*
 * time.h
 *
 * time types. Based on the Single UNIX(r) Specification, Version 2
 *
 * This file is part of the ReactOS Operating System.
 *
 * Contributors:
 *  Created by KJK::Hyperion <noog@libero.it>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#ifndef __TIME_H_INCLUDED__
#define __TIME_H_INCLUDED__

/* INCLUDES */
#include <signal.h>
#include <sys/types.h>

/* OBJECTS */
//extern static int getdate_err; /* FIXME */
extern int       daylight;
extern long int  timezone;
extern char     *tzname[];

/* TYPES */
/* pre-declaration of signal.h types to suppress warnings caused by circular
   dependencies */
struct sigevent;

struct tm
{
 int    tm_sec;   /* seconds [0,61] */
 int    tm_min;   /* minutes [0,59] */
 int    tm_hour;  /* hour [0,23] */
 int    tm_mday;  /* day of month [1,31] */
 int    tm_mon;   /* month of year [0,11] */
 int    tm_year;  /* years since 1900 */
 int    tm_wday;  /* day of week [0,6] (Sunday = 0) */
 int    tm_yday;  /* day of year [0,365] */
 int    tm_isdst; /* daylight savings flag */
};

struct timespec
{
 time_t  tv_sec;    /* seconds */
 long    tv_nsec;   /* nanoseconds */
};

struct itimerspec
{
 struct timespec  it_interval;  /* timer period */
 struct timespec  it_value;     /* timer expiration */
};

/* CONSTANTS */
/* FIXME: all the constants are wrong */
/* Number of clock ticks per second returned by the times() function (LEGACY). */
#define CLK_TCK (1)
/* A number used to convert the value returned by the clock() function into
   seconds. */
#define CLOCKS_PER_SEC (1)
/* The identifier of the systemwide realtime clock. */
#define CLOCK_REALTIME (0)
/* Flag indicating time is absolute with respect to the clock associated with a
   timer. */
#define TIMER_ABSTIME (1)

/* PROTOTYPES */
char      *asctime(const struct tm *);
char      *asctime_r(const struct tm *, char *);
clock_t    clock(void);
int        clock_getres(clockid_t, struct timespec *);
int        clock_gettime(clockid_t, struct timespec *);
int        clock_settime(clockid_t, const struct timespec *);
char      *ctime(const time_t *);
char      *ctime_r(const time_t *, char *);
double     difftime(time_t, time_t);
struct tm *getdate(const char *);
struct tm *gmtime(const time_t *);
struct tm *gmtime_r(const time_t *, struct tm *);
struct tm *localtime(const time_t *);
struct tm *localtime_r(const time_t *, struct tm *);
time_t     mktime(struct tm *);
int        nanosleep(const struct timespec *, struct timespec *);
size_t     strftime(char *, size_t, const char *, const struct tm *);
char      *strptime(const char *, const char *, struct tm *);
time_t     time(time_t *);
int        timer_create(clockid_t, struct sigevent *, timer_t *);
int        timer_delete(timer_t);
int        timer_gettime(timer_t, struct itimerspec *);
int        timer_getoverrun(timer_t);
int        timer_settime(timer_t, int, const struct itimerspec *,
               struct itimerspec *);
void       tzset(void);

/* MACROS */

#endif /* __TIME_H_INCLUDED__ */ /* replace with the appropriate tag */

/* EOF */

