/*
    ReactOS Sound System
    Timing helper

    Author:
        Andrew Greenwood (silverblade@reactos.org)

    History:
        31 May 2008 - Created

    Notes:
        Have checked timing in DebugView. A 10,000ms delay covered a period
        of 124.305 sec to 134.308 sec. Not 100% accurate but likely down to
        the delays in submitting the debug strings?
*/

/*
    Nanoseconds are fun! You must try some!
    1 ns        = .000000001 seconds    = .0000001 ms
    100 ns      = .0000001 seconds      = .00001 ms
    10000 ns    = .00001 seconds        = .001 ms
    1000000 ns  = .001 seconds          = 1 ms
*/

#include <ntddk.h>

VOID
SleepMs(ULONG Milliseconds)
{
    LARGE_INTEGER Period;

    Period.QuadPart = -Milliseconds;
    Period.QuadPart *= 10000;

    KeDelayExecutionThread(KernelMode, FALSE, &Period);
}

ULONG
QuerySystemTimeMs()
{
    LARGE_INTEGER Time;

    KeQuerySystemTime(&Time);

    Time.QuadPart /= 10000;

    return (ULONG) Time.QuadPart;
}

