/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/apic/apictimer.c
 * PURPOSE:         System Profiling
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#include "apicp.h"
#define NDEBUG
#include <debug.h>

extern LARGE_INTEGER HalpCpuClockFrequency;

/* HAL profiling variables */
BOOLEAN HalIsProfiling = FALSE;
ULONGLONG HalCurProfileInterval = 10000000;
ULONGLONG HalMinProfileInterval = 1000;
ULONGLONG HalMaxProfileInterval = 10000000;

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

VOID
FASTCALL
HalpProfileInterruptHandler(_In_ PKTRAP_FRAME TrapFrame)
{
    KeProfileInterruptWithSource(TrapFrame, ProfileTime);
}


/* PUBLIC FUNCTIONS ***********************************************************/

VOID
NTAPI
HalInitializeProfiling(VOID)
{
    KeGetPcr()->HalReserved[HAL_PROFILING_INTERVAL] = HalCurProfileInterval;
    KeGetPcr()->HalReserved[HAL_PROFILING_MULTIPLIER] = 1; /* TODO: HACK */
}

VOID
NTAPI
HalStartProfileInterrupt(IN KPROFILE_SOURCE ProfileSource)
{
    LVT_REGISTER LvtEntry;

    /* Only handle ProfileTime */
    if (ProfileSource == ProfileTime)
    {
        /* OK, we are profiling now */
        HalIsProfiling = TRUE;

        /* Set interrupt interval */
        ApicWrite(APIC_TICR, KeGetPcr()->HalReserved[HAL_PROFILING_INTERVAL]);

        /* Unmask it */
        LvtEntry.Long = 0;
        LvtEntry.TimerMode = 1;
        LvtEntry.Vector = APIC_PROFILE_VECTOR;
        LvtEntry.Mask = 0;
        ApicWrite(APIC_TMRLVTR, LvtEntry.Long);
    }
}

VOID
NTAPI
HalStopProfileInterrupt(IN KPROFILE_SOURCE ProfileSource)
{
    LVT_REGISTER LvtEntry;

    /* Only handle ProfileTime */
    if (ProfileSource == ProfileTime)
    {
        /* We are not profiling */
        HalIsProfiling = FALSE;

        /* Mask interrupt */
        LvtEntry.Long = 0;
        LvtEntry.TimerMode = 1;
        LvtEntry.Vector = APIC_PROFILE_VECTOR;
        LvtEntry.Mask = 1;
        ApicWrite(APIC_TMRLVTR, LvtEntry.Long);
    }
}

ULONG_PTR
NTAPI
HalSetProfileInterval(IN ULONG_PTR Interval)
{
    ULONGLONG TimerInterval;
    ULONGLONG FixedInterval;

    FixedInterval = (ULONGLONG)Interval;

    /* Check bounds */
    if (FixedInterval < HalMinProfileInterval)
    {
        FixedInterval = HalMinProfileInterval;
    }
    else if (FixedInterval > HalMaxProfileInterval)
    {
        FixedInterval = HalMaxProfileInterval;
    }

    /* Remember interval */
    HalCurProfileInterval = FixedInterval;

    /* Recalculate interval for APIC */
    TimerInterval = FixedInterval * KeGetPcr()->HalReserved[HAL_PROFILING_MULTIPLIER] / HalMaxProfileInterval;

    /* Remember recalculated interval in PCR */
    KeGetPcr()->HalReserved[HAL_PROFILING_INTERVAL] = (ULONG)TimerInterval;

    /* And set it */
    ApicWrite(APIC_TICR, (ULONG)TimerInterval);

    return Interval;
}
