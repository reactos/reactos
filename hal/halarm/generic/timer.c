/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halarm/generic/timer.c
 * PURPOSE:         Timer Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

VOID
FASTCALL
KeUpdateSystemTime(
    IN PKTRAP_FRAME TrapFrame,
    IN ULONG Increment,
    IN KIRQL OldIrql
);


/*
 * @unimplemented
 */
VOID
NTAPI
HalCalibratePerformanceCounter(IN volatile PLONG Count,
                               IN ULONGLONG NewCount)
{
    UNIMPLEMENTED;
    while (TRUE);
}

/*
 * @unimplemented
 */
ULONG
NTAPI
HalSetTimeIncrement(IN ULONG Increment)
{
    UNIMPLEMENTED;
    while (TRUE);
    return 0;
}

/*
 * @unimplemented
 */
VOID
NTAPI
KeStallExecutionProcessor(IN ULONG Microseconds)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
LARGE_INTEGER
NTAPI
KeQueryPerformanceCounter(IN PLARGE_INTEGER PerformanceFreq)
{
    UNIMPLEMENTED;
    while (TRUE);
    return PerformanceFreq[0];
}

/* EOF */
