/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/generic/timer.c
 * PURPOSE:         HAL Timer Routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

BOOLEAN HalpClockSetMSRate;
ULONG HalpCurrentTimeIncrement;
ULONG HalpCurrentRollOver;
ULONG HalpNextMSRate = 14;
ULONG HalpLargestClockMS = 15;

LARGE_INTEGER HalpRolloverTable[15] =
{
    {{1197, 10032}},
    {{2394, 20064}},
    {{3591, 30096}},
    {{4767, 39952}},
    {{5964, 49984}},
    {{7161, 60016}},
    {{8358, 70048}},
    {{9555, 80080}},
    {{10731, 89936}},
    {{11949, 100144}},
    {{13125, 110000}},
    {{14322, 120032}},
    {{15519, 130064}},
    {{16695, 139920}},
    {{17892, 149952}}
};

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
HalpInitializeClock(VOID)
{
    //PKPRCB Prcb = KeGetCurrentPrcb();
    ULONG Increment;
    USHORT RollOver;
    ULONG Flags = 0;

    /* Get increment and rollover for the largest time clock ms possible */
    Increment = HalpRolloverTable[HalpLargestClockMS - 1].HighPart;
    RollOver = (USHORT)HalpRolloverTable[HalpLargestClockMS - 1].LowPart;

    /* Set the maximum and minimum increment with the kernel */
    HalpCurrentTimeIncrement = Increment;
    KeSetTimeIncrement(Increment, HalpRolloverTable[0].HighPart);

    /* Disable interrupts */
    Flags = __readmsr();
    _disable();

    /* Set the rollover */
    __outbyte(TIMER_CONTROL_PORT, TIMER_SC0 | TIMER_BOTH | TIMER_MD2);
    __outbyte(TIMER_DATA_PORT0, RollOver & 0xFF);
    __outbyte(TIMER_DATA_PORT0, RollOver >> 8);

    /* Restore interrupts if they were previously enabled */
    __writemsr(Flags);

    /* Save rollover and return */
    HalpCurrentRollOver = RollOver;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
VOID
NTAPI
HalCalibratePerformanceCounter(IN volatile PLONG Count,
                               IN ULONGLONG NewCount)
{
    ULONG Flags = 0;

    /* Disable interrupts */
    Flags = __readmsr();
    _disable();

    /* Do a decrement for this CPU */
    _InterlockedDecrement(Count);

    /* Wait for other CPUs */
    while (*Count);

    /* Restore interrupts if they were previously enabled */
    __writemsr(Flags);
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
    return HalpRolloverTable[Increment - 1].HighPart;
}

VOID
NTHALAPI
KeStallExecutionProcessor(ULONG USec)
{
    LARGE_INTEGER Freq, Start = KeQueryPerformanceCounter(&Freq), End;
    LARGE_INTEGER Timebase, Remainder;
    Timebase.QuadPart = 1000000;
    Freq.QuadPart *= USec;
    End = RtlLargeIntegerDivide(Freq, Timebase, &Remainder);
    End.QuadPart += Start.QuadPart;
    while(End.QuadPart > __rdtsc());
}

/* EOF */
