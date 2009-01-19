/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS CRT library
 * FILE:        lib/sdk/crt/time/localtime.c
 * PURPOSE:     Implementation of localtime, localtime_s
 * PROGRAMERS:  Timo Kreuzer
 */
#include <precomp.h>
#include <time.h>
#include "bitsfixup.h"

errno_t
localtime_s(struct tm* _tm, const time_t *ptime)
{

    /* Validate parameters */
    if (!_tm || !ptime)
    {
        _invalid_parameter(NULL,
                           0,//__FUNCTION__, 
                           _CRT_WIDE(__FILE__), 
                           __LINE__, 
                           0);
        return EINVAL;
    }


    return 0;
}

extern char _tz_is_set;

struct tm *
localtime(const time_t *ptime)
{
    time_t time = *ptime;
    struct tm * ptm;

    /* Check for invalid time value */
    if (time < 0)
    {
        return 0;
    }

    /* Never without */
    if (!_tz_is_set)
        _tzset();

    /* Check for overflow */

    /* Correct for timezone */
    time -= _timezone;
#if 0
    /* Correct for daylight saving */
    if (_isdstime(time))
    {
        ptm->tm_isdst = 1;
        time -= _dstbias;
    }
#endif
    ptm = gmtime(&time);

    return ptm;
}

