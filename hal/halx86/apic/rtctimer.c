/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GNU GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/apic/rtctimer.c
 * PURPOSE:         HAL APIC Management and Control Code
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 * REFERENCES:      https://wiki.osdev.org/RTC
 *                  https://forum.osdev.org/viewtopic.php?f=13&t=20825&start=0
 *                  http://www.bioscentral.com/misc/cmosmap.htm
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#include "apicp.h"
#include <smp.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

static const UCHAR RtcMinimumClockRate = 6;  /* Minimum rate  6: 1024 Hz / 0.97 ms */
static const UCHAR RtcMaximumClockRate = 10; /* Maximum rate 10: 64 Hz / 15.6 ms */
static UCHAR HalpCurrentClockRate = 10;  /* Initial rate  10: 64 Hz / 15.6 ms */
static ULONG HalpCurrentTimeIncrement;
static ULONG HalpMinimumTimeIncrement;
static ULONG HalpMaximumTimeIncrement;
static ULONG HalpCurrentFractionalIncrement;
static ULONG HalpRunningFraction;
static BOOLEAN HalpSetClockRate;
static UCHAR HalpNextClockRate;

/*!
    \brief Converts the CMOS RTC rate into the time increment in 0.1ns intervals.

    Rate Frequency Interval (ms) Precise increment (0.1ns)
    ------------------------------------------------------
     0   disabled
     1   32768      0.03052            305,175
     2   16384      0.06103            610,351
     3    8192      0.12207          1,220,703
     4    4096      0.24414          2,441,406
     5    2048      0.48828          4,882,812
     6    1024      0.97656          9,765,625 <- minimum
     7     512      1.95313         19,531,250
     8     256      3.90625         39,062,500
     9     128      7.8125          78,125,000
    10      64     15.6250         156,250,000 <- maximum / default
    11      32     31.25           312,500,000
    12      16     62.5            625,000,000
    13       8    125            1,250,000,000
    14       4    250            2,500,000,000
    15       2    500            5,000,000,000

*/
FORCEINLINE
ULONG
RtcClockRateToPreciseIncrement(UCHAR Rate)
{
    /* Calculate frequency */
    ULONG Frequency = 32768 >> (Rate - 1);

    /* Calculate interval in 0.1ns interval: Interval = (1 / Frequency) * 10,000,000,000 */
    return 10000000000ULL / Frequency;
}

VOID
RtcSetClockRate(UCHAR ClockRate)
{
    UCHAR RegisterA;
    ULONG PreciseIncrement;

    /* Update the global values */
    HalpCurrentClockRate = ClockRate;
    PreciseIncrement = RtcClockRateToPreciseIncrement(ClockRate);
    HalpCurrentTimeIncrement = PreciseIncrement / 1000;
    HalpCurrentFractionalIncrement = PreciseIncrement % 1000;

    /* Acquire CMOS lock */
    HalpAcquireCmosSpinLock();

    // TODO: disable NMI

    /* Read value of register A */
    RegisterA = HalpReadCmos(RTC_REGISTER_A);

    /* Change lower 4 bits to new rate */
    RegisterA &= 0xF0;
    RegisterA |= ClockRate;

    /* Write the new value */
    HalpWriteCmos(RTC_REGISTER_A, RegisterA);

    /* Release CMOS lock */
    HalpReleaseCmosSpinLock();
}

CODE_SEG("INIT")
VOID
NTAPI
HalpInitializeClock(VOID)
{
    ULONG_PTR EFlags;
    UCHAR RegisterB;

    /* Save EFlags and disable interrupts */
    EFlags = __readeflags();
    _disable();

    // TODO: disable NMI

    /* Acquire CMOS lock */
    HalpAcquireCmosSpinLock();

    /* Enable the periodic interrupt in the CMOS */
    RegisterB = HalpReadCmos(RTC_REGISTER_B);
    HalpWriteCmos(RTC_REGISTER_B, RegisterB | RTC_REG_B_PI);

    /* Release CMOS lock */
    HalpReleaseCmosSpinLock();

    /* Set initial rate */
    RtcSetClockRate(HalpCurrentClockRate);

    /* Restore interrupt state */
    __writeeflags(EFlags);

    /* Calculate minumum and maximum increment */
    HalpMinimumTimeIncrement = RtcClockRateToPreciseIncrement(RtcMinimumClockRate) / 1000;
    HalpMaximumTimeIncrement = RtcClockRateToPreciseIncrement(RtcMaximumClockRate) / 1000;

    /* Notify the kernel about the maximum and minimum increment */
    KeSetTimeIncrement(HalpMaximumTimeIncrement, HalpMinimumTimeIncrement);

    /* Enable the timer interrupt */
    HalEnableSystemInterrupt(APIC_CLOCK_VECTOR, CLOCK_LEVEL, Latched);

    DPRINT1("Clock initialized\n");
}

VOID
FASTCALL
HalpClockInterruptHandler(IN PKTRAP_FRAME TrapFrame)
{
    ULONG LastIncrement;
    KIRQL Irql;

    /* Enter trap */
    KiEnterInterruptTrap(TrapFrame);
#ifdef _M_AMD64
    /* This is for debugging */
    TrapFrame->ErrorCode = 0xc10c4;
#endif

    /* Start the interrupt */
    if (!HalBeginSystemInterrupt(CLOCK_LEVEL, APIC_CLOCK_VECTOR, &Irql))
    {
        /* Spurious, just end the interrupt */
#ifdef _M_IX86
        KiEoiHelper(TrapFrame);
#endif
        return;
    }

    /* Read register C, so that the next interrupt can happen */
    HalpReadCmos(RTC_REGISTER_C);

    /* Save increment */
    LastIncrement = HalpCurrentTimeIncrement;

    /* Check if the running fraction has accounted for 100 ns */
    HalpRunningFraction += HalpCurrentFractionalIncrement;
    if (HalpRunningFraction >= 1000)
    {
        LastIncrement++;
        HalpRunningFraction -= 1000;
    }

    /* Check if someone changed the time rate */
    if (HalpSetClockRate)
    {
        /* Set new clock rate */
        RtcSetClockRate(HalpNextClockRate);

        /* We're done */
        HalpSetClockRate = FALSE;
    }

    /* Send the clock IPI to all other CPUs */
    HalpBroadcastClockIpi(CLOCK_IPI_VECTOR);

    /* Update the system time -- on x86 the kernel will exit this trap  */
    KeUpdateSystemTime(TrapFrame, LastIncrement, Irql);

    /* End the interrupt */
    KiEndInterrupt(Irql, TrapFrame);
}

VOID
FASTCALL
HalpClockIpiHandler(IN PKTRAP_FRAME TrapFrame)
{
    KIRQL Irql;

    /* Enter trap */
    KiEnterInterruptTrap(TrapFrame);
#ifdef _M_AMD64
    /* This is for debugging */
    TrapFrame->ErrorCode = 0xc10c4;
#endif

    /* Start the interrupt */
    if (!HalBeginSystemInterrupt(CLOCK_LEVEL, CLOCK_IPI_VECTOR, &Irql))
    {
        /* Spurious, just end the interrupt */
#ifdef _M_IX86
        KiEoiHelper(TrapFrame);
#endif
        return;
    }

    /* Call the kernel to update runtimes */
    KeUpdateRunTime(TrapFrame, Irql);

    /* End the interrupt */
    KiEndInterrupt(Irql, TrapFrame);
}

ULONG
NTAPI
HalSetTimeIncrement(IN ULONG Increment)
{
    UCHAR Rate;
    ULONG NextIncrement;

    /* Lookup largest value below given Increment */
    for (Rate = RtcMinimumClockRate; Rate < RtcMaximumClockRate; Rate++)
    {
        /* Check if this is the largest rate possible */
        NextIncrement = RtcClockRateToPreciseIncrement(Rate + 1) / 1000;
        if (NextIncrement > Increment)
            break;
    }

    /* Set the rate and tell HAL we want to change it */
    HalpNextClockRate = Rate;
    HalpSetClockRate = TRUE;

    /* Return the real increment */
    return RtcClockRateToPreciseIncrement(Rate) / 1000;
}
