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
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

const UCHAR HalpClockVector = 0xD1;
BOOLEAN HalpClockSetMSRate;
UCHAR HalpNextMSRate;
UCHAR HalpCurrentRate = 9;  /* Initial rate  9: 128 Hz / 7.8 ms */
ULONG HalpCurrentTimeIncrement;
static UCHAR RtcMinimumClockRate = 8;  /* Minimum rate  8: 256 Hz / 3.9 ms */
static UCHAR RtcMaximumClockRate = 12; /* Maximum rate 12: 16 Hz / 62.5 ms */

/*!
    \brief Converts the CMOS RTC rate into the time increment in 100ns intervals.

    Rate Freqency Interval (ms)  Result
    -------------------------------------
     0   disabled
     1   32768      0.03052          305
     2   16384      0.06103          610
     3    8192      0.12207         1221
     4    4096      0.24414         2441
     5    2048      0.48828         4883
     6    1024      0.97656         9766
     7     512      1.95313        19531
     8     256      3.90625        39063
     9     128      7.8125         78125
    10      64     15.6250        156250
    11      32     31.25          312500
    12      16     62.5           625000
    13       8    125            1250000
    14       4    250            2500000
    15       2    500            5000000

*/
FORCEINLINE
ULONG
RtcClockRateToIncrement(UCHAR Rate)
{
    /* Calculate frequency */
    ULONG Freqency = 32768 >> (Rate - 1);

    /* Calculate interval in 100ns interval: Interval = (1 / Frequency) * 10000000
       This formula will round properly, instead of truncating. */
    return (10000000 + (Freqency/2)) / Freqency;
}

VOID
RtcSetClockRate(UCHAR ClockRate)
{
    UCHAR RegisterA;

    /* Update the global values */
    HalpCurrentRate = ClockRate;
    HalpCurrentTimeIncrement = RtcClockRateToIncrement(ClockRate);

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
    RtcSetClockRate(HalpCurrentRate);

    /* Restore interrupt state */
    __writeeflags(EFlags);

    /* Notify the kernel about the maximum and minimum increment */
    KeSetTimeIncrement(RtcClockRateToIncrement(RtcMaximumClockRate),
                       RtcClockRateToIncrement(RtcMinimumClockRate));


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
    if (!HalBeginSystemInterrupt(CLOCK_LEVEL, HalpClockVector, &Irql))
    {
        /* Spurious, just end the interrupt */
        KiEoiHelper(TrapFrame);
    }

    /* Read register C, so that the next interrupt can happen */
    HalpReadCmos(RTC_REGISTER_C);

    /* Save increment */
    LastIncrement = HalpCurrentTimeIncrement;

    /* Check if someone changed the time rate */
    if (HalpClockSetMSRate)
    {
        /* Set new clock rate */
        RtcSetClockRate(HalpNextMSRate);

        /* We're done */
        HalpClockSetMSRate = FALSE;
    }

    /* Update the system time -- on x86 the kernel will exit this trap  */
    KeUpdateSystemTime(TrapFrame, LastIncrement, Irql);
}

ULONG
NTAPI
HalSetTimeIncrement(IN ULONG Increment)
{
    UCHAR Rate;

    /* Lookup largest value below given Increment */
    for (Rate = RtcMinimumClockRate; Rate <= RtcMaximumClockRate; Rate++)
    {
        /* Check if this is the largest rate possible */
        if (RtcClockRateToIncrement(Rate + 1) > Increment) break;
    }

    /* Set the rate and tell HAL we want to change it */
    HalpNextMSRate = Rate;
    HalpClockSetMSRate = TRUE;

    /* Return the real increment */
    return RtcClockRateToIncrement(Rate);
}
