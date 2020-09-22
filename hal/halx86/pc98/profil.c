/*
 * PROJECT:     NEC PC-98 series HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     System Profiling
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

BOOLEAN HalpProfilingStopped = TRUE;
UCHAR HalpProfileRate = 3;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
HalStopProfileInterrupt(
    _In_ KPROFILE_SOURCE ProfileSource)
{
    UNREFERENCED_PARAMETER(ProfileSource);

    HalpAcquireCmosSpinLock();

    /* Clear the interrupt flag */
    (VOID)__inbyte(RTC_IO_i_INTERRUPT_RESET);

    HalpProfilingStopped = TRUE;

    HalpReleaseCmosSpinLock();
}

VOID
NTAPI
HalStartProfileInterrupt(
    _In_ KPROFILE_SOURCE ProfileSource)
{
    UNREFERENCED_PARAMETER(ProfileSource);

    HalpProfilingStopped = FALSE;

    HalpAcquireCmosSpinLock();

    /* Configure the clock divisor for generating periodic interrupts */
    __outbyte(RTC_IO_o_INT_CLOCK_DIVISOR, HalpProfileRate | 0x80);

    HalpReleaseCmosSpinLock();
}

ULONG_PTR
NTAPI
HalSetProfileInterval(
    _In_ ULONG_PTR Interval)
{
    /*
     * FIXME:
     * 1) What is the maximum and minimum interrupt frequency for the RTC?
     * 2) Find the maximum possible clock divisor value.
     */
    UNIMPLEMENTED;

    /* Update interval */
    if (!HalpProfilingStopped)
       HalStartProfileInterrupt(0);

    /* For now... */
    return Interval;
}
