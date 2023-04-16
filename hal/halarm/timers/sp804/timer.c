/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halarm/generic/timer.c
 * PURPOSE:         Timer Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

VOID
FASTCALL
KeUpdateSystemTime(
    IN PKTRAP_FRAME TrapFrame,
    IN ULONG Increment,
    IN KIRQL OldIrql
);

/* GLOBALS ********************************************************************/

ULONG HalpCurrentTimeIncrement, HalpNextTimeIncrement, HalpNextIntervalCount;

/* PRIVATE FUNCTIONS **********************************************************/

VOID
HalpClockInterrupt(VOID)
{
    /* Clear the interrupt */
    ASSERT(KeGetCurrentIrql() == CLOCK2_LEVEL);
    WRITE_REGISTER_ULONG(TIMER0_INT_CLEAR, 1);

    /* FIXME: Update HAL Perf counters */

    /* FIXME: Check if someone changed the clockrate */

    /* Call the kernel */
    KeUpdateSystemTime(KeGetCurrentThread()->TrapFrame,
                       HalpCurrentTimeIncrement,
                       CLOCK2_LEVEL);
}

VOID
HalpStallInterrupt(VOID)
{
    /* Clear the interrupt */
    WRITE_REGISTER_ULONG(TIMER0_INT_CLEAR, 1);
}

VOID
HalpInitializeClock(VOID)
{
    PKPCR Pcr = KeGetPcr();
    ULONG ClockInterval;
    SP804_CONTROL_REGISTER ControlRegister;

    /* Setup the clock and profile interrupt */
    Pcr->InterruptRoutine[CLOCK2_LEVEL] = HalpStallInterrupt;

    /*
     * Configure the interval to 10ms
     * (INTERVAL (10ms) * TIMCLKfreq (1MHz))
     * --------------------------------------- == 10^4
     *  (TIMCLKENXdiv (1) * PRESCALEdiv (1))
     */
    ClockInterval = 0x2710;

    /* Configure the timer */
    ControlRegister.AsUlong = 0;
    ControlRegister.Wide = TRUE;
    ControlRegister.Periodic = TRUE;
    ControlRegister.Interrupt = TRUE;
    ControlRegister.Enabled = TRUE;

    /* Enable the timer */
    WRITE_REGISTER_ULONG(TIMER0_LOAD, ClockInterval);
    WRITE_REGISTER_ULONG(TIMER0_CONTROL, ControlRegister.AsUlong);
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
    UNIMPLEMENTED;
    while (TRUE);
}

/*
 * @implemented
 */
ULONG
NTAPI
HalSetTimeIncrement(IN ULONG Increment)
{
    UNIMPLEMENTED;
    while (TRUE);
    return Increment;
}

/*
 * @implemented
 */
VOID
NTAPI
KeStallExecutionProcessor(IN ULONG Microseconds)
{
    SP804_CONTROL_REGISTER ControlRegister;

    /* Enable the timer */
    WRITE_REGISTER_ULONG(TIMER1_LOAD, Microseconds);

    /* Configure the timer */
    ControlRegister.AsUlong = 0;
    ControlRegister.OneShot = TRUE;
    ControlRegister.Wide = TRUE;
    ControlRegister.Periodic = TRUE;
    ControlRegister.Enabled = TRUE;
    WRITE_REGISTER_ULONG(TIMER1_CONTROL, ControlRegister.AsUlong);

    /* Now we will loop until the timer reached 0 */
    while (READ_REGISTER_ULONG(TIMER1_VALUE));
}

/*
 * @implemented
 */
LARGE_INTEGER
NTAPI
KeQueryPerformanceCounter(IN PLARGE_INTEGER PerformanceFreq)
{
    LARGE_INTEGER Value;

    UNIMPLEMENTED;
    while (TRUE);

    Value.QuadPart = 0;
    return Value;
}

/* EOF */
