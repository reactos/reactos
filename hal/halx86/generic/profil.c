/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/generic/profil.c
 * PURPOSE:         System Profiling
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

BOOLEAN HalpProfilingStopped = TRUE;
UCHAR HalpProfileRate = 8;

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID
NTAPI
HalStopProfileInterrupt(IN KPROFILE_SOURCE ProfileSource)
{
    UCHAR StatusB;

    UNREFERENCED_PARAMETER(ProfileSource);

    /* Acquire the CMOS lock */
    HalpAcquireCmosSpinLock();

    /* Read Status Register B */
    StatusB = HalpReadCmos(RTC_REGISTER_B);

    /* Disable periodic interrupts */
    StatusB = StatusB & ~RTC_REG_B_PI;

    /* Write new value into Status Register B */
    HalpWriteCmos(RTC_REGISTER_B, StatusB);

    HalpProfilingStopped = TRUE;

    /* Release the CMOS lock */
    HalpReleaseCmosSpinLock();
}

/*
 * @unimplemented
 */
VOID
NTAPI
HalStartProfileInterrupt(IN KPROFILE_SOURCE ProfileSource)
{
    UCHAR StatusA, StatusB;

    UNREFERENCED_PARAMETER(ProfileSource);

    HalpProfilingStopped = FALSE;

    /* Acquire the CMOS lock */
    HalpAcquireCmosSpinLock();

    /* Set the interval in Status Register A */
    StatusA = HalpReadCmos(RTC_REGISTER_A);
    StatusA = (StatusA & 0xF0) | HalpProfileRate;
    HalpWriteCmos(RTC_REGISTER_A, StatusA);

    /* Enable periodic interrupts in Status Register B */
    StatusB = HalpReadCmos(RTC_REGISTER_B);
    StatusB = StatusB | RTC_REG_B_PI;
    HalpWriteCmos(RTC_REGISTER_B, StatusB);

    /* Release the CMOS lock */
    HalpReleaseCmosSpinLock();
}

/*
 * @unimplemented
 */
ULONG_PTR
NTAPI
HalSetProfileInterval(IN ULONG_PTR Interval)
{
    ULONG_PTR CurrentValue, NextValue;
    UCHAR i;

    /* Normalize interval. 122100 ns is the smallest supported */
    Interval &= ~(1 << 31);
    if (Interval < 1221)
        Interval = 1221;

    /* Highest rate value of 15 means 500 ms */
    CurrentValue = 5000000;
    for (i = 15; ; i--)
    {
        NextValue = (CurrentValue + 1) / 2;
        if (Interval > CurrentValue - NextValue / 2)
            break;
        CurrentValue = NextValue;
    }

    /* Interval as needed by RTC */
    HalpProfileRate = i;

    /* Reset the  */
    if (!HalpProfilingStopped)
    {
       HalStartProfileInterrupt(0);
    }

    return CurrentValue;
}
