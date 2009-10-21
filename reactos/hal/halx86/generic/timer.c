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

/* time to wait */
#define MICROSECOND_TO_WAIT 1000
/* the tick count for 1 ms is 1193.182 (1193182 Hz) round it up */
#define TICKCOUNT_TO_WAIT  1194

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

    /* Check the CPU Type */
    if (Prcb->CpuType <= 4)
    {
        /* 486's or equal can't go higher then 10ms */
        HalpLargestClockMS = 10;
        HalpNextMSRate = 9;
    }

    /* Get increment and rollover for the largest time clock ms possible */
    Increment= HalpRolloverTable[HalpLargestClockMS - 1].HighPart;
    RollOver = (USHORT)HalpRolloverTable[HalpLargestClockMS - 1].LowPart;

    /* Set the maximum and minimum increment with the kernel */
    HalpCurrentTimeIncrement = Increment;
    KeSetTimeIncrement(Increment, HalpRolloverTable[0].HighPart);

    /* Disable interrupts */
    Flags = __readeflags();
    _disable();

    /* Set the rollover */
    __outbyte(TIMER_CONTROL_PORT, TIMER_SC0 | TIMER_BOTH | TIMER_MD2);
    __outbyte(TIMER_DATA_PORT0, RollOver & 0xFF);
    __outbyte(TIMER_DATA_PORT0, RollOver >> 8);

    /* Restore interrupts if they were previously enabled */
    __writeeflags(Flags);

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

ULONG
WaitFor8254Wraparound(VOID)
{
    ULONG StartTicks;
    ULONG PrevTicks;
    LONG Delta;

    StartTicks = HalpQuery8254Counter();

    do
    {
        PrevTicks = StartTicks;
        StartTicks = HalpQuery8254Counter();
        Delta = StartTicks - PrevTicks;

        /*
         * This limit for delta seems arbitrary, but it isn't, it's
         * slightly above the level of error a buggy Mercury/Neptune
         * chipset timer can cause.
         */

    }
    while (Delta < 300);

    return StartTicks;
}

VOID
NTAPI
HalpCalibrateStallExecution(VOID)
{
    ULONG CalibrationBit;
    ULONG EndTicks;
    ULONG StartTicks;
    ULONG OverheadTicks;
    PKIPCR Pcr;

    Pcr = (PKIPCR)KeGetPcr();

    /* Measure the delay for the minimum call overhead in ticks */
    Pcr->StallScaleFactor = 1;
    StartTicks = WaitFor8254Wraparound();
    KeStallExecutionProcessor(1);
    EndTicks = HalpQuery8254Counter();
    OverheadTicks = (StartTicks - EndTicks);

    do
    {
        /* Increase the StallScaleFactor */
        Pcr->StallScaleFactor = Pcr->StallScaleFactor * 2;

        if (Pcr->StallScaleFactor == 0)
        {
           /* Nothing found */
           break;
        }

        /* Get the start ticks */
        StartTicks = WaitFor8254Wraparound();

        /* Wait for a defined time */
        KeStallExecutionProcessor(MICROSECOND_TO_WAIT);

        /* Get the end ticks */
        EndTicks = HalpQuery8254Counter();

        DPRINT("Pcr->StallScaleFactor: %d\n", Pcr->StallScaleFactor);
        DPRINT("Time1 : StartTicks %i - EndTicks %i = %i\n",
            StartTicks, EndTicks, StartTicks - EndTicks);
    } while ((StartTicks - EndTicks) <= (TICKCOUNT_TO_WAIT + OverheadTicks));

    /* A StallScaleFactor lesser than INITIAL_STALL_COUNT makes no sense */
    if (Pcr->StallScaleFactor >= (INITIAL_STALL_COUNT * 2))
    {
        /* Adjust the StallScaleFactor */
        Pcr->StallScaleFactor  = Pcr->StallScaleFactor / 2;

        /* Setup the CalibrationBit */
        CalibrationBit = Pcr->StallScaleFactor;

        for (;;)
        {
            /* Lower the CalibrationBit */
            CalibrationBit = CalibrationBit / 2;
            if (CalibrationBit == 0)
            {
                break;
            }

            /* Add the CalibrationBit */
            Pcr->StallScaleFactor = Pcr->StallScaleFactor + CalibrationBit;

            /* Get the start ticks */
            StartTicks = WaitFor8254Wraparound();

            /* Wait for a defined time */
            KeStallExecutionProcessor(MICROSECOND_TO_WAIT);

            /* Get the end ticks */
            EndTicks = HalpQuery8254Counter();

            DPRINT("Pcr->StallScaleFactor: %d\n", Pcr->StallScaleFactor);
            DPRINT("Time2 : StartTicks %i - EndTicks %i = %i\n",
                StartTicks, EndTicks, StartTicks - EndTicks);

            if ((StartTicks-EndTicks) > (TICKCOUNT_TO_WAIT+OverheadTicks))
            {
                /* Too big so subtract the CalibrationBit */
                Pcr->StallScaleFactor = Pcr->StallScaleFactor - CalibrationBit;
            }
        }
        DPRINT("New StallScaleFactor: %d\n", Pcr->StallScaleFactor);
    }
    else
    {
       /* Set StallScaleFactor to the default */
       Pcr->StallScaleFactor = INITIAL_STALL_COUNT;
    }

#if 0
    /* For debugging */
    ULONG i;

    DPRINT1("About to start delay loop test\n");
    DPRINT1("Waiting for a minute...");
    for (i = 0; i < (60*1000*20); i++)
    {
        KeStallExecutionProcessor(50);
    }
    DPRINT1("finished\n");


    DPRINT1("About to start delay loop test\n");
    DPRINT1("Waiting for a minute...");
    for (i = 0; i < (60*1000); i++)
    {
        KeStallExecutionProcessor(1000);
    }
    DPRINT1("finished\n");


    DPRINT1("About to start delay loop test\n");
    DPRINT1("Waiting for a minute...");
    for (i = 0; i < (60*1000*1000); i++)
    {
        KeStallExecutionProcessor(1);
    }
    DPRINT1("finished\n");

    DPRINT1("About to start delay loop test\n");
    DPRINT1("Waiting for a minute...");
    KeStallExecutionProcessor(60*1000000);
    DPRINT1("finished\n");
#endif
}


/* EOF */
