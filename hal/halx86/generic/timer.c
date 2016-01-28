/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/generic/timer.c
 * PURPOSE:         HAL Timer Routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

#if defined(ALLOC_PRAGMA) && !defined(_MINIHAL_)
#pragma alloc_text(INIT, HalpInitializeClock)
#endif

/* GLOBALS *******************************************************************/

#define PIT_LATCH  0x00

LARGE_INTEGER HalpLastPerfCounter;
LARGE_INTEGER HalpPerfCounter;
ULONG HalpPerfCounterCutoff;
BOOLEAN HalpClockSetMSRate;
ULONG HalpCurrentTimeIncrement;
ULONG HalpCurrentRollOver;
ULONG HalpNextMSRate = 14;
ULONG HalpLargestClockMS = 15;

static struct _HALP_ROLLOVER
{
    ULONG RollOver;
    ULONG Increment;
} HalpRolloverTable[15] =
{
    {1197, 10032},
    {2394, 20064},
    {3591, 30096},
    {4767, 39952},
    {5964, 49984},
    {7161, 60016},
    {8358, 70048},
    {9555, 80080},
    {10731, 89936},
    {11949, 100144},
    {13125, 110000},
    {14322, 120032},
    {15519, 130064},
    {16695, 139920},
    {17892, 149952}
};

/* PRIVATE FUNCTIONS *********************************************************/

FORCEINLINE
ULONG
HalpRead8254Value(void)
{
    ULONG TimerValue;

    /* Send counter latch command for channel 0 */
    __outbyte(TIMER_CONTROL_PORT, PIT_LATCH);
    __nop();

    /* Read the value, LSB first */
    TimerValue = __inbyte(TIMER_CHANNEL0_DATA_PORT);
    __nop();
    TimerValue |= __inbyte(TIMER_CHANNEL0_DATA_PORT) << 8;

    return TimerValue;
}

VOID
NTAPI
HalpSetTimerRollOver(USHORT RollOver)
{
    ULONG_PTR Flags;
    TIMER_CONTROL_PORT_REGISTER TimerControl;

    /* Disable interrupts */
    Flags = __readeflags();
    _disable();

    /* Program the PIT for binary mode */
    TimerControl.BcdMode = FALSE;

    /*
     * Program the PIT to generate a normal rate wave (Mode 3) on channel 0.
     * Channel 0 is used for the IRQ0 clock interval timer, and channel
     * 1 is used for DRAM refresh.
     *
     * Mode 2 gives much better accuracy than Mode 3.
     */
    TimerControl.OperatingMode = PitOperatingMode2;
    TimerControl.Channel = PitChannel0;

    /* Set the access mode that we'll use to program the reload value */
    TimerControl.AccessMode = PitAccessModeLowHigh;

    /* Now write the programming bits */
    __outbyte(TIMER_CONTROL_PORT, TimerControl.Bits);

    /* Next we write the reload value for channel 0 */
    __outbyte(TIMER_CHANNEL0_DATA_PORT, RollOver & 0xFF);
    __outbyte(TIMER_CHANNEL0_DATA_PORT, RollOver >> 8);

    /* Restore interrupts if they were previously enabled */
    __writeeflags(Flags);
}

INIT_SECTION
VOID
NTAPI
HalpInitializeClock(VOID)
{
    ULONG Increment;
    USHORT RollOver;

    DPRINT("HalpInitializeClock()\n");

    /* Get increment and rollover for the largest time clock ms possible */
    Increment = HalpRolloverTable[HalpLargestClockMS - 1].Increment;
    RollOver = (USHORT)HalpRolloverTable[HalpLargestClockMS - 1].RollOver;

    /* Set the maximum and minimum increment with the kernel */
    KeSetTimeIncrement(Increment, HalpRolloverTable[0].Increment);

    /* Set the rollover value for the timer */
    HalpSetTimerRollOver(RollOver);

    /* Save rollover and increment */
    HalpCurrentRollOver = RollOver;
    HalpCurrentTimeIncrement = Increment;
}

#ifdef _M_IX86
#ifndef _MINIHAL_
VOID
FASTCALL
HalpClockInterruptHandler(IN PKTRAP_FRAME TrapFrame)
{
    ULONG LastIncrement;
    KIRQL Irql;

    /* Enter trap */
    KiEnterInterruptTrap(TrapFrame);

    /* Start the interrupt */
    if (HalBeginSystemInterrupt(CLOCK2_LEVEL, PRIMARY_VECTOR_BASE, &Irql))
    {
        /* Update the performance counter */
        HalpPerfCounter.QuadPart += HalpCurrentRollOver;
        HalpPerfCounterCutoff = KiEnableTimerWatchdog;

        /* Save increment */
        LastIncrement = HalpCurrentTimeIncrement;

        /* Check if someone changed the time rate */
        if (HalpClockSetMSRate)
        {
            /* Update the global values */
            HalpCurrentTimeIncrement = HalpRolloverTable[HalpNextMSRate - 1].Increment;
            HalpCurrentRollOver = HalpRolloverTable[HalpNextMSRate - 1].RollOver;

            /* Set new timer rollover */
            HalpSetTimerRollOver((USHORT)HalpCurrentRollOver);

            /* We're done */
            HalpClockSetMSRate = FALSE;
        }

        /* Update the system time -- the kernel will exit this trap  */
        KeUpdateSystemTime(TrapFrame, LastIncrement, Irql);
    }

    /* Spurious, just end the interrupt */
    KiEoiHelper(TrapFrame);
}

VOID
FASTCALL
HalpProfileInterruptHandler(IN PKTRAP_FRAME TrapFrame)
{
    KIRQL Irql;

    /* Enter trap */
    KiEnterInterruptTrap(TrapFrame);

    /* Start the interrupt */
    if (HalBeginSystemInterrupt(PROFILE_LEVEL, PRIMARY_VECTOR_BASE + 8, &Irql))
    {
        /* Spin until the interrupt pending bit is clear */
        HalpAcquireCmosSpinLock();
        while (HalpReadCmos(RTC_REGISTER_C) & RTC_REG_C_IRQ)
            ;
        HalpReleaseCmosSpinLock();

        /* If profiling is enabled, call the kernel function */
        if (!HalpProfilingStopped)
        {
            KeProfileInterrupt(TrapFrame);
        }

        /* Finish the interrupt */
        _disable();
        HalEndSystemInterrupt(Irql, TrapFrame);
    }

    /* Spurious, just end the interrupt */
    KiEoiHelper(TrapFrame);
}
#endif

#endif

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
VOID
NTAPI
HalCalibratePerformanceCounter(IN volatile PLONG Count,
                               IN ULONGLONG NewCount)
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

/*
 * @implemented
 */
ULONG
NTAPI
HalSetTimeIncrement(IN ULONG Increment)
{
    /* Round increment to ms */
    Increment /= 10000;

    /* Normalize between our minimum (1 ms) and maximum (variable) setting */
    if (Increment > HalpLargestClockMS) Increment = HalpLargestClockMS;
    if (Increment <= 0) Increment = 1;

    /* Set the rate and tell HAL we want to change it */
    HalpNextMSRate = Increment;
    HalpClockSetMSRate = TRUE;

    /* Return the increment */
    return HalpRolloverTable[Increment - 1].Increment;
}

LARGE_INTEGER
NTAPI
KeQueryPerformanceCounter(PLARGE_INTEGER PerformanceFrequency)
{
    LARGE_INTEGER CurrentPerfCounter;
    ULONG CounterValue, ClockDelta;
    KIRQL OldIrql;

    /* If caller wants performance frequency, return hardcoded value */
    if (PerformanceFrequency) PerformanceFrequency->QuadPart = PIT_FREQUENCY;

    /* Check if we were called too early */
    if (HalpCurrentRollOver == 0) return HalpPerfCounter;

    /* Check if interrupts are disabled */
    if(!(__readeflags() & EFLAGS_INTERRUPT_MASK)) return HalpPerfCounter;

    /* Raise irql to DISPATCH_LEVEL */
    OldIrql = KeGetCurrentIrql();
    if (OldIrql < DISPATCH_LEVEL) KfRaiseIrql(DISPATCH_LEVEL);

    do
    {
        /* Get the current performance counter value */
        CurrentPerfCounter = HalpPerfCounter;

        /* Read the 8254 counter value */
        CounterValue = HalpRead8254Value();

    /* Repeat if the value has changed (a clock interrupt happened) */
    } while (CurrentPerfCounter.QuadPart != HalpPerfCounter.QuadPart);

    /* After someone changed the clock rate, during the first clock cycle we
       might see a counter value larger than the rollover. In this case we
       pretend it already has the new rollover value. */
    if (CounterValue > HalpCurrentRollOver) CounterValue = HalpCurrentRollOver;

    /* The interrupt is issued on the falling edge of the OUT line, when the
       counter changes from 1 to max. Calculate a clock delta, so that directly
       after the interrupt it is 0, going up to (HalpCurrentRollOver - 1). */
    ClockDelta = HalpCurrentRollOver - CounterValue;

    /* Add the clock delta */
    CurrentPerfCounter.QuadPart += ClockDelta;

    /* Check if the value is smaller then before, this means, we somehow
       missed an interrupt. This is a sign that the timer interrupt
       is very inaccurate. Probably a virtual machine. */
    if (CurrentPerfCounter.QuadPart < HalpLastPerfCounter.QuadPart)
    {
        /* We missed an interrupt. Assume we will receive it later */
        CurrentPerfCounter.QuadPart += HalpCurrentRollOver;
    }

    /* Update the last counter value */
    HalpLastPerfCounter = CurrentPerfCounter;

    /* Restore previous irql */
    if (OldIrql < DISPATCH_LEVEL) KfLowerIrql(OldIrql);

    /* Return the result */
    return CurrentPerfCounter;
}

/* EOF */
