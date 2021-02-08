/*
 * PROJECT:         ReactOS HAL
 * COPYRIGHT:       GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:            hal/halx86/pic/legacy.c
 * PURPOSE:         Legacy part PIC HALs code
 * PROGRAMMERS:     Copyright 2021 Vadim Galyant <vgal@rambler.ru>
 */

/* INCLUDES *******************************************************************/

#include <hal.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

#ifndef _MINIHAL_
VOID
FASTCALL
HalpClockInterruptHandler(IN PKTRAP_FRAME TrapFrame)
{
    HaliClockInterrupt(TrapFrame, FALSE, FALSE);
}
#endif

/* PUBLIC TIMER FUNCTIONS *****************************************************/

#if 0 // generic/systimer.S
VOID
NTAPI
KeStallExecutionProcessor(_In_ ULONG MicroSeconds)
{
    ;
}
#endif

VOID
NTAPI
HalCalibratePerformanceCounter(_In_ volatile PLONG Count,
                               _In_ ULONGLONG NewCount)
{
    ULONG_PTR Flags;

    /* Disable interrupts */
    Flags = __readeflags();
    _disable();

    /* Do a decrement for this CPU */
    _InterlockedDecrement(Count);

    /* Wait for other CPUs */
    while (*Count);

    /* Restore interrupts if they were previously enabled */
    __writeeflags(Flags);
}

LARGE_INTEGER
NTAPI
KeQueryPerformanceCounter(_Out_opt_ LARGE_INTEGER * OutPerformanceFrequency)
{
    return HalpTimerQueryPerfCount(OutPerformanceFrequency);
}

ULONG
NTAPI
HalSetTimeIncrement(_In_ ULONG Increment)
{
    return HaliTimerSetTimeIncrement(Increment);
}


/* EOF */
