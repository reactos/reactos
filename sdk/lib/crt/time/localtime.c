/*
 * COPYRIGHT:   LGPL, See LGPL.txt in the top level directory
 * PROJECT:     ReactOS CRT library
 * FILE:        lib/sdk/crt/time/localtime.c
 * PURPOSE:     Implementation of localtime, localtime_s
 * PROGRAMERS:  Timo Kreuzer
 *              Samuel Serapión
 */
#include <precomp.h>
#include <time.h>
#include "bitsfixup.h"

//fix me: header?
#define _MAX__TIME64_T     0x793406fffLL     /* number of seconds from
                                                 00:00:00, 01/01/1970 UTC to
                                                 23:59:59. 12/31/3000 UTC */

errno_t
localtime_s(struct tm* _tm, const time_t *ptime)
{
    /* check for NULL */
    if (!_tm || !ptime )
    {
        if(_tm) memset(_tm, 0xFF, sizeof(struct tm));
        _invalid_parameter(NULL,
                           0,//__FUNCTION__,
                           _CRT_WIDE(__FILE__),
                           __LINE__,
                           0);
        _set_errno(EINVAL);
        return EINVAL;
    }

    /* Validate input */
    if (*ptime < 0 || *ptime > _MAX__TIME64_T)
    {
        memset(_tm, 0xFF, sizeof(struct tm));
        _set_errno(EINVAL);
        return EINVAL;
    }

    _tm = localtime(ptime);
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

