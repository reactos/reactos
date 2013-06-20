/*
 * COPYRIGHT:   LGPL, See LGPL.txt in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/time/time.c
 * PURPOSE:     Implementation of _time (_time32, _time64)
 * PROGRAMER:   Timo Kreuzer
 */
#include <precomp.h>
#include <time.h>
#include "bitsfixup.h"

time_t _time(time_t* ptime)
{
    FILETIME SystemTime;
    time_t time = 0;

    GetSystemTimeAsFileTime(&SystemTime);
    time = (time_t)FileTimeToUnixTime(&SystemTime, NULL);

    if (ptime)
    {
        *ptime = time;
    }
    return time;
}
