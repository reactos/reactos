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

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID
NTAPI
HalStopProfileInterrupt(IN KPROFILE_SOURCE ProfileSource)
{
    UCHAR StatusB;

    /* Acquire the CMOS lock */
    HalpAcquireCmosSpinLock();

    /* Read Status Register B */
    StatusB = HalpReadCmos(RTC_REGISTER_B);

    /* Disable periodic interrupts */
    StatusB = StatusB & ~RTC_REG_B_PI;

    /* Write new value into Status Register B */
    HalpWriteCmos(RTC_REGISTER_B, StatusB);

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
    UNIMPLEMENTED;
    return;
}

/*
 * @unimplemented
 */
ULONG_PTR
NTAPI
HalSetProfileInterval(IN ULONG_PTR Interval)
{
    UNIMPLEMENTED;
    return Interval;
}
