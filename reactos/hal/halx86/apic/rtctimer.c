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
UCHAR HalpCurrentRate = 9;
ULONG HalpCurrentTimeIncrement;
static UCHAR RtcLargestClockRate = 10;


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

   // RtcSetClockRate(HalpCurrentRate);
}

VOID
FASTCALL
HalpClockInterruptHandler(IN PKTRAP_FRAME TrapFrame)
{
    ULONG LastIncrement;
    KIRQL Irql;

    /* Enter trap */
    KiEnterInterruptTrap(TrapFrame);
__debugbreak();

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
            /* Update the global values */
            HalpCurrentRate = HalpNextMSRate;
            HalpCurrentTimeIncrement = RtcClockRateToIncrement(HalpCurrentRate);

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
    for (Rate = 2; Rate < RtcLargestClockRate; Rate++)
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
