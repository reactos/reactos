/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/sdk/crt/time/time.c
 * PURPOSE:     Implementation of _time (_tim32, _tim64)
 * PROGRAMER:   Timo Kreuzer
 */
#include <precomp.h>
#include <time.h>
#include "bitsfixup.h"

time_t _time(time_t* ptime)
{
	FILETIME SystemTime;
	time_t time = 0;

    if (ptime)
    {
    	GetSystemTimeAsFileTime(&SystemTime);
	    time = FileTimeToUnixTime(&SystemTime, NULL);
		*ptime = time;
    }
	return time;
}
