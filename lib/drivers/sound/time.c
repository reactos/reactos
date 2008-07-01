/*
    ReactOS Sound System
    Timing helper

    Author:
        Andrew Greenwood (andrew.greenwood@silverblade.co.uk)

    History:
        31 May 2008 - Created

    Notes:
        Timing may require testing!
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

