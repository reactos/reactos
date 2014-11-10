/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/apic/apictimer.c
 * PURPOSE:         System Profiling
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

#include "apic.h"

extern LARGE_INTEGER HalpCpuClockFrequency;

/* TIMER FUNCTIONS ************************************************************/

VOID
NTAPI
ApicSetTimerInterval(ULONG MicroSeconds)
{
    LVT_REGISTER LvtEntry;
    ULONGLONG TimerInterval;

    /* Calculate the Timer interval */
    TimerInterval = HalpCpuClockFrequency.QuadPart * MicroSeconds / 1000000;

    /* Set the count interval */
    ApicWrite(APIC_TICR, (ULONG)TimerInterval);

    /* Set to periodic */
    LvtEntry.Long = 0;
    LvtEntry.TimerMode = 1;
    LvtEntry.Vector = APIC_PROFILE_VECTOR;
    LvtEntry.Mask = 0;
    ApicWrite(APIC_TMRLVTR, LvtEntry.Long);

}

VOID
NTAPI
ApicInitializeTimer(ULONG Cpu)
{

    /* Initialize the TSC */
    //HalpInitializeTsc();

    /* Set clock multiplier to 1 */
    ApicWrite(APIC_TDCR, TIMER_DV_DivideBy1);

    ApicSetTimerInterval(1000);

// KeSetTimeIncrement
}


/* PUBLIC FUNCTIONS ***********************************************************/

VOID
NTAPI
HalStartProfileInterrupt(IN KPROFILE_SOURCE ProfileSource)
{
    UNIMPLEMENTED;
    return;
}

VOID
NTAPI
HalStopProfileInterrupt(IN KPROFILE_SOURCE ProfileSource)
{
    UNIMPLEMENTED;
    return;
}

ULONG_PTR
NTAPI
HalSetProfileInterval(IN ULONG_PTR Interval)
{
    UNIMPLEMENTED;
    return Interval;
}
