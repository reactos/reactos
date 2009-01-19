/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS CRT library
 * FILE:        lib/sdk/crt/time/ftime.c
 * PURPOSE:     Deprecated BSD library call
 * PROGRAMERS:  Timo Kreuzer
 */
#include <precomp.h>
#include <sec_api/time_s.h>
#include <sys/timeb.h>
#include "bitsfixup.h"

/******************************************************************************
 * \name _ftime_s
 * \brief Get the current time.
 * \param [out] ptimeb Pointer to a structure of type struct _timeb that 
 *        recieves the current time.
 * \sa http://msdn.microsoft.com/en-us/library/95e68951.aspx
 */
errno_t
_ftime_s(struct _timeb *ptimeb)
{
    DWORD ret;
    TIME_ZONE_INFORMATION TimeZoneInformation;
    FILETIME SystemTime;

    /* Validate parameters */
    if (!ptimeb)
    {
        _invalid_parameter(0,
                           0,//__FUNCTION__,
                           _CRT_WIDE(__FILE__),
                           __LINE__,
                           0);
        return EINVAL;
    }

    ret = GetTimeZoneInformation(&TimeZoneInformation);
    ptimeb->dstflag = (ret == TIME_ZONE_ID_DAYLIGHT) ? 1 : 0;
    ptimeb->timezone = TimeZoneInformation.Bias;

    GetSystemTimeAsFileTime(&SystemTime);
    ptimeb->time = FileTimeToUnixTime(&SystemTime, &ptimeb->millitm);

    return 0;
}

/******************************************************************************
 * \name _ftime
 * \brief Get the current time.
 * \param [out] ptimeb Pointer to a structure of type struct _timeb that 
 *        recieves the current time.
 * \note This function is for compatability and simply calls the secure 
 *       version _ftime_s().
 * \sa http://msdn.microsoft.com/en-us/library/z54t9z5f.aspx
 */
void
_ftime(struct _timeb *ptimeb)
{
    _ftime_s(ptimeb);
}
