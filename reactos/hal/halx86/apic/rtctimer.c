/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GNU GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/generic/apic.c
 * PURPOSE:         HAL APIC Management and Control Code
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 * REFERENCES:
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>


/* GLOBALS ********************************************************************/

const UCHAR HalpClockVector = 0xD1;
BOOLEAN HalpClockSetMSRate;
UCHAR HalpNextMSRate;
UCHAR HalpCurrentRate = 9;  /* Initial rate  9: 128 Hz / 7,8 ms */
ULONG HalpCurrentTimeIncrement;
static UCHAR RtcMinimumClockRate = 6;  /* Minimum rate  6:  16 Hz / 62,5 ms */
static UCHAR RtcMaximumClockRate = 10; /* Maximum rate 10: 256 Hz / 3,9 ms */


ULONG
FORCEINLINE
RtcClockRateToIncrement(UCHAR Rate)
{
    ULONG Freqency = ((32768 << 1) >> Rate);
    return (1000000 + (Freqency/2)) / Freqency;
}

VOID
RtcSetClockRate(UCHAR ClockRate)
{
    ULONG_PTR EFlags;
    UCHAR RegisterA;

    /* Disable interrupts */
    EFlags = __readeflags();
    _disable();

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

    /* Restore interrupts if they were previously enabled */
    __writeeflags(EFlags);
}


VOID
NTAPI
INIT_FUNCTION
HalpInitializeClock(VOID)
{
    UCHAR RegisterB;
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

    /* Start the interrupt */
    if (HalBeginSystemInterrupt(CLOCK_LEVEL, PRIMARY_VECTOR_BASE, &Irql))
    {
        /* Read register C, so that the next interrupt can happen */
        HalpReadCmos(RTC_REGISTER_C);;

        /* Save increment */
        LastIncrement = HalpCurrentTimeIncrement;

        /* Check if someone changed the time rate */
        if (HalpClockSetMSRate)
        {
            /* Set new clock rate */
            RtcSetClockRate(HalpCurrentRate);

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
    __debugbreak();
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
