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
    PKPRCB Prcb = KeGetCurrentPrcb();
    ULONG Increment;
    USHORT RollOver;
    ULONG_PTR Flags;
    TIMER_CONTROL_PORT_REGISTER TimerControl;

    /* Check the CPU Type */
    if (Prcb->CpuType <= 4)
    {
        /* 486's or equal can't go higher then 10ms */
        HalpLargestClockMS = 10;
        HalpNextMSRate = 9;
    }

    /* Get increment and rollover for the largest time clock ms possible */
    Increment = HalpRolloverTable[HalpLargestClockMS - 1].HighPart;
    RollOver = (USHORT)HalpRolloverTable[HalpLargestClockMS - 1].LowPart;

    /* Set the maximum and minimum increment with the kernel */
    HalpCurrentTimeIncrement = Increment;
    KeSetTimeIncrement(Increment, HalpRolloverTable[0].HighPart);

    /* Disable interrupts */
    Flags = __readeflags();
    _disable();
    
    //
    // Program the PIT for binary mode
    //
    TimerControl.BcdMode = FALSE;

    //
    // Program the PIT to generate a normal rate wave (Mode 3) on channel 0.
    // Channel 0 is used for the IRQ0 clock interval timer, and channel
    // 1 is used for DRAM refresh.
    //
    // Mode 2 gives much better accuracy than Mode 3.
    //
    TimerControl.OperatingMode = PitOperatingMode2;
    TimerControl.Channel = PitChannel0;
    
    //
    // Set the access mode that we'll use to program the reload value.
    //
    TimerControl.AccessMode = PitAccessModeLowHigh;
    
    //
    // Now write the programming bits
    //
    __outbyte(TIMER_CONTROL_PORT, TimerControl.Bits);
    
    //
    // Next we write the reload value for channel 0
    //
    __outbyte(TIMER_CHANNEL0_DATA_PORT, RollOver & 0xFF);
    __outbyte(TIMER_CHANNEL0_DATA_PORT, RollOver >> 8);

    /* Restore interrupts if they were previously enabled */
    __writeeflags(Flags);

    /* Save rollover and return */
    HalpCurrentRollOver = RollOver;
}

VOID
FASTCALL
HalpClockInterruptHandler(IN PKTRAP_FRAME TrapFrame)
{
    KIRQL Irql;
    
    /* Enter trap */
    KiEnterInterruptTrap(TrapFrame);
    
    /* Start the interrupt */
    if (HalBeginSystemInterrupt(CLOCK2_LEVEL, PRIMARY_VECTOR_BASE, &Irql))
    {
        /* Update the performance counter */
        HalpPerfCounter.QuadPart += HalpCurrentRollOver;
        
        /* Check if someone changed the time rate */
        if (HalpClockSetMSRate)
        {
            /* Not yet supported */
            UNIMPLEMENTED;
            while (TRUE);
        }
        
        /* Update the system time -- the kernel will exit this trap  */
        KeUpdateSystemTime(TrapFrame, HalpCurrentTimeIncrement, Irql);
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
        /* Profiling isn't yet enabled */
        UNIMPLEMENTED;
        while (TRUE);
    }
    
    /* Spurious, just end the interrupt */
    KiEoiHelper(TrapFrame);
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
    return HalpRolloverTable[Increment - 1].HighPart;
}

/* EOF */
