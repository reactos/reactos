/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS CRT library
 * FILE:        lib/sdk/crt/time/gmtime.c
 * PURPOSE:     Implementation of gmtime, _gmtime32, _gmtime64
 * PROGRAMERS:  Timo Kreuzer
 */
#include <precomp.h>

unsigned int g_monthdays[13] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
unsigned int g_lpmonthdays[13] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366};

struct tm *
_gmtime_worker(struct tm *ptm, __time64_t time, int do_dst)
{
    unsigned int days, daystoyear, dayinyear, leapdays, leapyears, years, month;
    unsigned int secondinday, secondinhour;
    unsigned int *padays;

    if (time < 0)
    {
        return 0;
    }

    /* Divide into date and time */
    days = time / SECONDSPERDAY;
    secondinday = time % SECONDSPERDAY;

    /* Shift to days from 1.1.1601 */
    days += DIFFDAYS;

    /* Calculate leap days passed till today */
    leapdays = leapdays_passed(days);

    /* Calculate number of full leap years passed */
    leapyears = leapyears_passed(days);

    /* Are more leap days passed than leap years? */
    if (leapdays > leapyears)
    {
        /* Yes, we're in a leap year */
        padays = g_lpmonthdays;
    }
    else
    {
        /* No, normal year */
        padays = g_monthdays;
    }

    /* Calculate year */
    years = (days - leapdays) / 365;
    ptm->tm_year = years - 299;

    /* Calculate number of days till 1.1. of this year */
    daystoyear = years * 365 + leapyears;

    /* Calculate the day in this year */
    dayinyear = days - daystoyear;

    /* Shall we do DST corrections? */
    ptm->tm_isdst = 0;
    if (do_dst)
    {
        unsigned int yeartime = dayinyear * SECONDSPERDAY + secondinday ;
        if (yeartime >= dst_begin && yeartime <= dst_end) // FIXME! DST in winter
        {
            time -= _dstbias;
            days = time / SECONDSPERDAY + DIFFDAYS;
            dayinyear = days - daystoyear;
            ptm->tm_isdst = 1;
        }
    }

    ptm->tm_yday = dayinyear;

    /* dayinyear < 366 => terminates with i <= 11 */
    for (month = 0; dayinyear >= padays[month+1]; month++)
        ;

    /* Set month and day in month */
    ptm->tm_mon = month;
    ptm->tm_mday = 1 + dayinyear - padays[month];

    /* Get weekday */
    ptm->tm_wday = (days + 4) % 7;

    /* Calculate hour and second in hour */
    ptm->tm_hour = secondinday / SECONDSPERHOUR;
    secondinhour = secondinday % SECONDSPERHOUR;

    /* Calculate minute and second */
    ptm->tm_min = secondinhour / 60;
    ptm->tm_sec = secondinhour % 60;

    return ptm;
}

/******************************************************************************
 * \name _gmtime64
 * \brief 
 * \param ptime Pointer to a variable of type __time64_t containing the time.
 */
struct tm *
_gmtime64(const __time64_t * ptime)
{
    PTHREADDATA pThreadData;
    struct tm *ptm;
    __time64_t time = *ptime;

    /* Validate parameters */
    if (time < 0)
    {
        return 0;
    }

    /* Get pointer to TLS tm buffer */
    pThreadData = GetThreadData();
    ptm = &pThreadData->tmbuf;

    /* Use _gmtime_worker to do the ral work */
    return _gmtime_worker(ptm, time, 0);
}

/******************************************************************************
 * \name _gmtime32
 * \brief 
 * \param ptime Pointer to a variable of type __time32_t containing the time.
 */
struct tm *
_gmtime32(const __time32_t * ptime)
{
    __time64_t time64 = (__time64_t)*ptime;
    return _gmtime64(&time64);
}

/******************************************************************************
 * \name gmtime
 * \brief 
 * \param ptime Pointer to a variable of type time_t containing the time.
 */
struct tm *
gmtime(const time_t * ptime)
{
    __time64_t time64 = (__time64_t)*ptime;
    return _gmtime64(&time64);
}
