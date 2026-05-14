/*
 * PROJECT:     NEC PC-98 series HAL
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     TSC calibration for the busy-wait loop routine
 * COPYRIGHT:   Copyright 2011 Timo Kreuzer <timo.kreuzer@reactos.org>
 *              Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES ******************************************************************/

#include <hal.h>

#define NDEBUG
#include <debug.h>

#include "delay.h"

/* GLOBALS *******************************************************************/

#define SAMPLE_FREQUENCY   1024 // 0.977 ms

VOID
__cdecl
HalpTscCalibrationISR(VOID);

extern volatile ULONG TscCalibrationPhase;
extern ULONG64 TscCalibrationArray[NUM_SAMPLES];
extern UCHAR HalpStallExecutionSerialize;

/* FUNCTIONS *****************************************************************/

static
CODE_SEG("INIT")
VOID
HalpPrepareStallExecution(VOID)
{
    PUCHAR Instruction = &HalpStallExecutionSerialize;
    PKPRCB Prcb = KeGetCurrentPrcb();

    /* xor eax, eax; cpuid */
    ASSERT((Instruction[1] == 0xC0) && // The byte [0] has different encodings
           (Instruction[2] == 0x0F) &&
           (Instruction[3] == 0xA2));

    /*
     * Starting with the Pentium Pro processor it is necessary to force
     * the in-order execution of the RDTSC instruction using a serializing instruction.
     * For more details, please refer to Section 3.1 of
     * Intel "Using the RDTSC Instruction for Performance Monitoring".
     *
     * Patch the KeStallExecutionProcessor function to remove the serializing instruction
     * for the Pentium and Pentium MMX processors because the CPUID instruction is slow.
     */
    if ((Prcb->CpuType < 6) && !strcmp(Prcb->VendorString, "GenuineIntel"))
    {
        /* Replace "xor eax, eax; cpuid" with "lea esi, [esi+0]" */
        Instruction[0] = 0x8D;
        Instruction[1] = 0x74;
        Instruction[2] = 0x26;
        Instruction[3] = 0x00;

        KeSweepICache(Instruction, 4);
    }
}

static
CODE_SEG("INIT")
ULONG64
HalpDoLinearRegression(
    _In_ ULONG XMax,
    _In_reads_(XMax + 1) const ULONG64* ArrayY)
{
    ULONG X, SumXX;
    ULONG64 SumXY;

    /* Calculate the sum of the squares of X */
    SumXX = (XMax * (XMax + 1) * (2 * XMax + 1)) / 6;

    /* Calculate the sum of the differences to the first value weighted by X */
    for (SumXY = 0, X = 1; X <= XMax; X++)
    {
        SumXY += X * (ArrayY[X] - ArrayY[0]);
    }

    /* Account for sample frequency */
    SumXY *= SAMPLE_FREQUENCY;

    /* Return the quotient of the sums */
    return (SumXY + (SumXX / 2)) / SumXX;
}

CODE_SEG("INIT")
VOID
NTAPI
HalpCalibrateStallExecution(VOID)
{
    ULONG_PTR Flags;
    PVOID PreviousHandler;
    TIMER_CONTROL_PORT_REGISTER TimerControl;
    ULONG TimerFrequency;
    USHORT Period;
    ULONG64 CpuClockFrequency;

    /* Check if the CPU supports RDTSC */
    if (!(KeGetCurrentPrcb()->FeatureBits & KF_RDTSC))
    {
        KeBugCheck(HAL_INITIALIZATION_FAILED);
    }

    Flags = __readeflags();
    _disable();

    PreviousHandler = KeQueryInterruptHandler(PIC_TIMER_IRQ);
    KeRegisterInterruptHandler(PRIMARY_VECTOR_BASE + PIC_TIMER_IRQ, HalpTscCalibrationISR);

    /* Program the PIT for binary mode */
    TimerControl.BcdMode = FALSE;
    TimerControl.OperatingMode = PitOperatingMode2;
    TimerControl.Channel = PitChannel0;
    TimerControl.AccessMode = PitAccessModeLowHigh;

    if (__inbyte(0x42) & 0x20)
        TimerFrequency = TIMER_FREQUENCY_1;
    else
        TimerFrequency = TIMER_FREQUENCY_2;
    Period = (TimerFrequency + (SAMPLE_FREQUENCY / 2)) / SAMPLE_FREQUENCY;

    __outbyte(TIMER_CONTROL_PORT, TimerControl.Bits);
    __outbyte(TIMER_CHANNEL0_DATA_PORT, Period & 0xFF);
    __outbyte(TIMER_CHANNEL0_DATA_PORT, Period >> 8);

    HalEnableSystemInterrupt(PRIMARY_VECTOR_BASE + PIC_TIMER_IRQ, CLOCK2_LEVEL, Latched);

    /* Collect the sample data */
    _enable();
    while (TscCalibrationPhase != (NUM_SAMPLES + 1))
        NOTHING;
    _disable();

    HalDisableSystemInterrupt(PRIMARY_VECTOR_BASE + PIC_TIMER_IRQ, CLOCK2_LEVEL);
    KeRegisterInterruptHandler(PRIMARY_VECTOR_BASE + PIC_TIMER_IRQ, PreviousHandler);

    /* Calculate an average, using simplified linear regression */
    CpuClockFrequency = HalpDoLinearRegression(NUM_SAMPLES - 1, TscCalibrationArray);
    KeGetPcr()->StallScaleFactor = (ULONG)(CpuClockFrequency / 1000000);

    HalpPrepareStallExecution();

    __writeeflags(Flags);
}
