/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS CRT library
 * FILE:        lib/sdk/crt/time/mktime.c
 * PURPOSE:     Implementation of mktime, _mkgmtime
 * PROGRAMERS:  Timo Kreuzer
 */
#include <precomp.h>
#include "bitsfixup.h"

#define MAX_32BIT_TIME 0xFFFFFFFFULL

static int g_monthdays[13] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};

__time64_t
mktime_worker(struct tm * ptm, int utc)
{
    struct tm *ptm2;
    __time64_t time;
    int mons, years, leapyears;

    /* Normalize year and month */
    if (ptm->tm_mon < 0)
    {
        mons = -ptm->tm_mon - 1;
        ptm->tm_year -= 1 + mons / 12;
        ptm->tm_mon = 11 - (mons % 12);
    }
    else if (ptm->tm_mon > 11)
    {
        mons = ptm->tm_mon;
        ptm->tm_year += (mons / 12);
        ptm->tm_mon = mons % 12;
    }

    /* Is it inside margins */
    if (ptm->tm_year < 70 || ptm->tm_year > 139) // FIXME: max year for 64 bits
    {
        return -1;
    }

    years = ptm->tm_year - 70;

    /* Number of leapyears passed since 1970 */
    leapyears = (years + 1) / 4;

    /* Calculate days up to 1st of Jan */
    time = years * 365 + leapyears;

    /* Calculate days up to 1st of month */
    time += g_monthdays[ptm->tm_mon];

    /* Check if we need to add a leap day */
    if (((years + 2) % 4) == 0)
    {
        if (ptm->tm_mon > 2)
        {
            time++;
        }
    }

    time += ptm->tm_mday - 1;

    time *= 24;
    time += ptm->tm_hour;

    time *= 60;
    time += ptm->tm_min;

    time *= 60;
    time += ptm->tm_sec;

    if (time < 0)
    {
        return -1;
    }

    /* Finally get normalized tm struct */
    ptm2 = _gmtime64(&time);
    if (!ptm2)
    {
        return -1;
    }
    *ptm = *ptm2;

    return time;
}

/*    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
*/

/** 
 * \name _mkgmtime
 * 
 */
time_t
_mkgmtime(struct tm *ptm)
{
    time_t time = mktime_worker(ptm, 1);
    return (time > MAX_32BIT_TIME) ? -1 : time;
}

time_t
mktime(struct tm *ptm)
{
    time_t time = mktime_worker(ptm, 0);
    return (time > MAX_32BIT_TIME) ? -1 : time;
}

__time32_t
_mkgmtime32(struct tm *ptm)
{
    time_t time = mktime_worker(ptm, 1);
    return (time > MAX_32BIT_TIME) ? -1 : time;
}

__time32_t
_mktime32(struct tm *ptm)
{
    time_t time = mktime_worker(ptm, 0);
    return (time > MAX_32BIT_TIME) ? -1 : time;
}

__time64_t
_mkgmtime64(struct tm *ptm)
{
    return mktime_worker(ptm, 1);
}

__time64_t
_mktime64(struct tm *ptm)
{
    return mktime_worker(ptm, 0);
}
