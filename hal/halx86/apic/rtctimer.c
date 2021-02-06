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

#ifndef _M_AMD64
#include "apic.h"
#endif

/* GLOBALS ********************************************************************/

const UCHAR HalpClockVector = 0xD1;
BOOLEAN HalpClockSetMSRate;
UCHAR HalpNextMSRate;
UCHAR HalpCurrentRate = 9;  /* Initial rate  9: 128 Hz / 7.8 ms */
ULONG HalpCurrentTimeIncrement;
static UCHAR RtcMinimumClockRate = 8;  /* Minimum rate  8: 256 Hz / 3.9 ms */
static UCHAR RtcMaximumClockRate = 12; /* Maximum rate 12: 16 Hz / 62.5 ms */

#ifndef _M_AMD64

typedef struct _HAL_RTC_TIME_INCREMENT
{
    ULONG RTCRegisterA;
    ULONG ClockRateIn100ns;
    ULONG Reserved;
    ULONG ClockRateAdjustment;
    ULONG IpiRate;
} HAL_RTC_TIME_INCREMENT, *PHAL_RTC_TIME_INCREMENT;

/*
CMOS 0Ah - RTC - STATUS REGISTER A (read/write) (usu 26h)

Bit(s)  Description     (Table C001)
 7      =1 time update cycle in progress, data ouputs undefined (bit 7 is read only)
 6-4    22 stage divider
        010 = 32768 Hz time base (default)
 3-0    rate selection bits for interrupt

        0000 none
 1      0001 256  30.517 microseconds
 2      0010 128  61.035 us
 3      0011 8192 122 us (minimum) 122.070
 4      0100 4096 244.140 us
 5      0101 2048 488.281 us
 6      0110 1024 976.562 us (default 1024 Hz) 976.562.5
 7      0111 512  1,953.125 milliseconds
 8      1000 256  3,906.25 ms
 9      1001 128  7,812.5 ms
 A      1010 64   15,625 ms
 B      1011 32   31,25 ms
 C      1100 16   62,5 ms
 D      1101 8    125 ms
 E      1110 4    250 ms
 F      1111 2    500 ms

*/
HAL_RTC_TIME_INCREMENT HalpRtcTimeIncrements[5] =
{
    {0x26, 0x02626, 0x26, 0x60, 0x010}, // 0 010 0110, ((default 1024 Hz) 976.562 microseconds )
    {0x27, 0x04C4C, 0x4B, 0xC0, 0x020}, // 0 010 0111, (512 Hz)
    {0x28, 0x09897, 0x32, 0x80, 0x040}, // 0 010 1000, (256 Hz)
    {0x29, 0x1312D, 0,    0,    0x080}, // 0 010 1001, (128 Hz)
    {0x2A, 0x2625A, 0,    0,    0x100}  // 0 010 1010, (64 Hz)
};

ULONG HalpInitialClockRateIndex = (5 - 1);

ULONG HalpCurrentRTCRegisterA;
ULONG HalpCurrentClockRateIn100ns;
ULONG HalpCurrentClockRateAdjustment;
ULONG HalpCurrentIpiRate;

ULONG HalpIpiClock = 0;
ULONG HalpIpiRateCounter = 0;
UCHAR HalpRateAdjustment = 0;

UCHAR HalpClockMcaQueueDpc;

BOOLEAN IsFirstCallClockInt = TRUE;
BOOLEAN IsFirstCallClockIntStub = TRUE;
BOOLEAN HalpTimerWatchdogEnabled = FALSE;
UCHAR ClockIntCalls = 0;

extern ULONG HalpWAETDeviceFlags;
extern BOOLEAN HalpUse8254;

#endif

/* FUNCTIONS ******************************************************************/

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

#ifdef _M_AMD64
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
    RosKeUpdateSystemTime(TrapFrame, LastIncrement, 0xFF, Irql);
}
#else
CODE_SEG("INIT")
VOID
NTAPI
HalpSetInitialClockRate()
{
    HalpClockSetMSRate = FALSE;
    HalpClockMcaQueueDpc = 0;

    HalpNextMSRate = HalpInitialClockRateIndex;

    HalpCurrentRTCRegisterA = HalpRtcTimeIncrements[HalpNextMSRate].RTCRegisterA;
    HalpCurrentClockRateIn100ns = HalpRtcTimeIncrements[HalpNextMSRate].ClockRateIn100ns;
    HalpCurrentClockRateAdjustment = HalpRtcTimeIncrements[HalpNextMSRate].ClockRateAdjustment;
    HalpCurrentIpiRate = HalpRtcTimeIncrements[HalpNextMSRate].IpiRate;

    DPRINT("HalpCurrentRTCRegisterA        - %X\n", HalpCurrentRTCRegisterA);
    DPRINT("HalpCurrentClockRateIn100ns    - %X\n", HalpCurrentClockRateIn100ns);
    DPRINT("HalpCurrentClockRateAdjustment - %X\n", HalpCurrentClockRateAdjustment);
    DPRINT("HalpCurrentIpiRate             - %X\n", HalpCurrentIpiRate);

    KeSetTimeIncrement(HalpRtcTimeIncrements[HalpNextMSRate].ClockRateIn100ns,
                       HalpRtcTimeIncrements[0].ClockRateIn100ns);
}

CODE_SEG("INIT")
VOID
NTAPI
HalpInitializeClock(VOID)
{
    DPRINT1("HalpInitializeClock: FIXME. DbgBreakPoint()\n");
    DbgBreakPoint();
}

#ifndef _MINIHAL_
VOID
FASTCALL
HaliClockInterrupt(_In_ PKTRAP_FRAME TrapFrame,
                   _In_ BOOLEAN IsAcpi)
{
    DPRINT1("HaliClockInterrupt: DbgBreakPoint()\n");
    DbgBreakPoint();
}
#endif // ifndef _MINIHAL_

VOID
FASTCALL
HalpClockInterruptStubHandler(_In_ PKTRAP_FRAME TrapFrame)
{
    DPRINT1("HalpClockInterruptStubHandler: TrapFrame %X. DbgBreakPoint()\n", TrapFrame);
    DbgBreakPoint();
}
#endif

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

