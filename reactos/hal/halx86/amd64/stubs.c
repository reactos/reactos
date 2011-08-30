/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING.ARM in the top level directory
 * FILE:            hal/halx86/amd64/stubs.c
 * PURPOSE:         HAL stubs
 * PROGRAMMERS:
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

LARGE_INTEGER HalpPerformanceFrequency;


/* FUNCTIONS ******************************************************************/

VOID
FASTCALL
HalClearSoftwareInterrupt(
    IN KIRQL Irql)
{
    UNIMPLEMENTED;
}

VOID
FASTCALL
HalRequestSoftwareInterrupt(
    IN KIRQL Irql)
{
    UNIMPLEMENTED;
}

BOOLEAN
NTAPI
HalBeginSystemInterrupt(
    IN KIRQL Irql,
    IN UCHAR Vector,
    OUT PKIRQL OldIrql)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOLEAN
NTAPI
HalEnableSystemInterrupt(
    IN UCHAR Vector,
    IN KIRQL Irql,
    IN KINTERRUPT_MODE InterruptMode)
{
    UNIMPLEMENTED;
    return FALSE;
}

VOID
NTAPI
HalDisableSystemInterrupt(
    IN UCHAR Vector,
    IN KIRQL Irql)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
HalEndSystemInterrupt(
    IN KIRQL OldIrql,
    IN PKTRAP_FRAME TrapFrame)
{
    UNIMPLEMENTED;
}

LARGE_INTEGER
NTAPI
KeQueryPerformanceCounter(
    OUT PLARGE_INTEGER PerformanceFrequency OPTIONAL)
{
    LARGE_INTEGER Result;

//    ASSERT(HalpPerformanceFrequency.QuadPart != 0);

    /* Does the caller want the frequency? */
    if (PerformanceFrequency)
    {
        /* Return value */
        *PerformanceFrequency = HalpPerformanceFrequency;
    }

    Result.QuadPart = __rdtsc();
    return Result;
}

VOID
NTAPI
HalCalibratePerformanceCounter(IN volatile PLONG Count,
                               IN ULONGLONG NewCount)
{
    UNIMPLEMENTED;
}

ULONG
NTAPI
HalSetTimeIncrement(IN ULONG Increment)
{
    UNIMPLEMENTED;
    return 0;
}
