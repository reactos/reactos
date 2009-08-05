/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS CRT library
 * FILE:        lib/sdk/crt/time/clock.c
 * PURPOSE:     Implementation of clock()
 * PROGRAMER:   Timo Kreuzer
 */
#include <precomp.h>

ULARGE_INTEGER g_StartupTime;

void
initclock(void)
{
    GetSystemTimeAsFileTime((FILETIME*)&g_StartupTime);
}

/******************************************************************************
 * \name clock
 * \brief Returns the current process's elapsed time.
 */
clock_t
clock(void)
{
    ULARGE_INTEGER Time;

    GetSystemTimeAsFileTime((FILETIME*)&Time);
    Time.QuadPart -= g_StartupTime.QuadPart;
    return FileTimeToUnixTime((FILETIME*)&Time, NULL);
};
