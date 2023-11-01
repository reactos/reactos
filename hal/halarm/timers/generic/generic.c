/*
 * PROJECT:     ARM Generic Timer Implementation
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Does cool things like Memory Management
 * COPYRIGHT:   Copyright 2023 Justin Miller <justin.miller@reactos.org>
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

/* GLOBALS ********************************************************************/

ULONG HalpCurrentTimeIncrement, HalpNextTimeIncrement, HalpNextIntervalCount;

/* PRIVATE FUNCTIONS **********************************************************/

/*
 * @ UNIMPLEMENTED
 */
VOID
HalpClockInterrupt(VOID)
{
    UNIMPLEMENTED;
    while (TRUE);
}

/*
 * @ UNIMPLEMENTED
 */
VOID
HalpStallInterrupt(VOID)
{
   UNIMPLEMENTED;
    while (TRUE);
}

/*
 * @ UNIMPLEMENTED
 */
VOID
HalpInitializeClock(VOID)
{
    UNIMPLEMENTED;
    while (TRUE);
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @ UNIMPLEMENTED
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
 * @ UNIMPLEMENTED
 */
ULONG
NTAPI
HalSetTimeIncrement(IN ULONG Increment)
{
    UNIMPLEMENTED;
    while (TRUE);
    return Increment;
}

/*
 * @ UNIMPLEMENTED
 */
VOID
NTAPI
KeStallExecutionProcessor(IN ULONG Microseconds)
{

}

/*
 * @ UNIMPLEMENTED
 */
LARGE_INTEGER
NTAPI
KeQueryPerformanceCounter(IN PLARGE_INTEGER PerformanceFreq)
{
    LARGE_INTEGER Value;

    UNIMPLEMENTED;
    while (TRUE);

    Value.QuadPart = 0;
    return Value;
}