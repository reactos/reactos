/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halamd64/generic/tsc.c
 * PURPOSE:         HAL Routines for TSC handling
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

#include "tsc.h"

LARGE_INTEGER HalpCpuClockFrequency = {{INITIAL_STALL_COUNT * 1000000}};

UCHAR TscCalibrationPhase;
ULONG64 TscCalibrationArray[NUM_SAMPLES];
UCHAR HalpRtcClockVector = 0xD1;

#define RTC_MODE 6 /* Mode 6 is 1024 Hz */
#define SAMPLE_FREQENCY ((32768 << 1) >> RTC_MODE)

/* PRIVATE FUNCTIONS *********************************************************/

static
ULONG64
DoLinearRegression(
    ULONG XMax,
    ULONG64 *ArrayY)
{
    ULONG X, SumXX;
    ULONG64 SumXY;

    /* Calculate the sum of the squares of X */
    SumXX = (XMax * (XMax + 1) * (2*XMax + 1)) / 6;

    /* Calculate the sum of the differences to the first value
       weighted by x */
    for (SumXY = 0, X = 1; X <= XMax; X++)
    {
         SumXY += X * (ArrayY[X] - ArrayY[0]);
    }

    /* Account for sample frequency */
    SumXY *= SAMPLE_FREQENCY;

    /* Return the quotient of the sums */
    return (SumXY + (SumXX/2)) / SumXX;
}

VOID
NTAPI
HalpInitializeTsc(VOID)
{
    ULONG_PTR Flags;
    KIDTENTRY OldIdtEntry, *IdtPointer;
    PKPCR Pcr = KeGetPcr();
    UCHAR RegisterA, RegisterB;

    /* Check if the CPU supports RDTSC */
    if (!(KeGetCurrentPrcb()->FeatureBits & KF_RDTSC))
    {
        KeBugCheck(HAL_INITIALIZATION_FAILED);
    }

     /* Save flags and disable interrupts */
    Flags = __readeflags();
    _disable();

    /* Enable the periodic interrupt in the CMOS */
    RegisterB = HalpReadCmos(RTC_REGISTER_B);
    HalpWriteCmos(RTC_REGISTER_B, RegisterB | RTC_REG_B_PI);

    /* Modify register A to RTC_MODE to get SAMPLE_FREQENCY */
    RegisterA = HalpReadCmos(RTC_REGISTER_A);
    RegisterA = (RegisterA & 0xF0) | RTC_MODE;
    HalpWriteCmos(RTC_REGISTER_A, RegisterA);

    /* Save old IDT entry */
    IdtPointer = KiGetIdtEntry(Pcr, HalpRtcClockVector);
    OldIdtEntry = *IdtPointer;

    /* Set the calibration ISR */
    KeRegisterInterruptHandler(HalpRtcClockVector, TscCalibrationISR);

    /* Reset TSC value to 0 */
    __writemsr(MSR_RDTSC, 0);

    /* Enable the timer interupt */
    HalEnableSystemInterrupt(HalpRtcClockVector, CLOCK_LEVEL, Latched);

    /* Read register C, so that the next interrupt can happen */
    HalpReadCmos(RTC_REGISTER_C);;

    /* Wait for completion */
    _enable();
    while (TscCalibrationPhase < NUM_SAMPLES) _ReadWriteBarrier();
    _disable();

    /* Disable the periodic interrupt in the CMOS */
    HalpWriteCmos(RTC_REGISTER_B, RegisterB & ~RTC_REG_B_PI);

    /* Disable the timer interupt */
    HalDisableSystemInterrupt(HalpRtcClockVector, CLOCK_LEVEL);

    /* Restore old IDT entry */
    *IdtPointer = OldIdtEntry;

    /* Calculate an average, using simplified linear regression */
    HalpCpuClockFrequency.QuadPart = DoLinearRegression(NUM_SAMPLES - 1,
                                                        TscCalibrationArray);

    /* Restore flags */
    __writeeflags(Flags);

}

VOID
NTAPI
HalpCalibrateStallExecution(VOID)
{
    // Timer interrupt is now active

    HalpInitializeTsc();

    KeGetPcr()->StallScaleFactor = (ULONG)(HalpCpuClockFrequency.QuadPart / 1000000);
}

/* PUBLIC FUNCTIONS ***********************************************************/

LARGE_INTEGER
NTAPI
KeQueryPerformanceCounter(
    OUT PLARGE_INTEGER PerformanceFrequency OPTIONAL)
{
    LARGE_INTEGER Result;

    /* Make sure it's calibrated */
    ASSERT(HalpCpuClockFrequency.QuadPart != 0);

    /* Does the caller want the frequency? */
    if (PerformanceFrequency)
    {
        /* Return tsc frequency */
        *PerformanceFrequency = HalpCpuClockFrequency;
    }

    /* Return the current value */
    Result.QuadPart = __rdtsc();
    return Result;
}

VOID
NTAPI
KeStallExecutionProcessor(ULONG MicroSeconds)
{
    ULONG64 StartTime, EndTime;

    /* Get the initial time */
    StartTime = __rdtsc();

    /* Calculate the ending time */
    EndTime = StartTime + KeGetPcr()->StallScaleFactor * MicroSeconds;

    /* Loop until time is elapsed */
    while (__rdtsc() < EndTime);
}

VOID
NTAPI
HalCalibratePerformanceCounter(
    IN volatile PLONG Count,
    IN ULONGLONG NewCount)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
}

