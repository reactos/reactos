/*
 * PROJECT:     ARM Generic Timer Implementation
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Does cool things like Memory Management
 * COPYRIGHT:   Copyright 2023 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <hal.h>
#include "timer.h"
#include "generictimer.h"
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
/*
 * This works for now however some
 * TODO::
 *  -> improve stall accuracy
 *  -> Set up clock rate changes
 *  -> Fix rest of clock Interrupts
 */
/* PRIVATE FUNCTIONS **********************************************************/

static
UINT32
HalpGetTimerFreq(VOID)
{
  return ArmGenericTimerGetTimerFreq ();
}

/*
 * @ UNIMPLEMENTED
 */
VOID
HalpClockInterrupt(VOID)
{
    /* Clear the interrupt */
     ASSERT(KeGetCurrentIrql() == CLOCK2_LEVEL);
    //TODO: Clear Interrupt

    /* FIXME: Update HAL Perf counters */

    /* FIXME: Check if someone changed the clockrate */

    /* Call the kernel */
    KeUpdateSystemTime(KeGetCurrentThread()->TrapFrame,
                       HalpCurrentTimeIncrement,
                       CLOCK2_LEVEL);
}

/*
 * @ SEMI-IMPLEMENTED
 */
VOID
HalpInitializeClock(VOID)
{
    UINT32  TimerCtrlReg;

    /* Enable timer */
    TimerCtrlReg  = ArmReadCntpCtl ();
    TimerCtrlReg |= ARM_ARCH_TIMER_ENABLE;
    ArmWriteCntpCtl (TimerCtrlReg);
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @ IMPLEMENTED
 */
VOID
NTAPI
HalCalibratePerformanceCounter(IN volatile PLONG Count,
                               IN ULONGLONG NewCount)
{
    /* nothing should happen here */
    UNIMPLEMENTED;
    while (TRUE);
}

/*
 * @ UNIMPLEMENTED
 */
ULONG
NTAPI
HalSetTimeIncrement(IN ULONG Increment)
{
    /* no need to do anything here yet */
    UNIMPLEMENTED;
    return Increment;
}

/*
 * @ SEMI-IMPLEMENTED
 * This works i just will make it better later
 */
VOID
NTAPI
KeStallExecutionProcessor(IN ULONG Microseconds)
{
    UINT32  TimerTicks64;
    UINT32  SystemCounterVal;
    TimerTicks64 = Microseconds * HalpGetTimerFreq ();
    SystemCounterVal = ArmGenericTimerGetSystemCount ();
    TimerTicks64 += SystemCounterVal;

     // Wait until delay count expires.
    while (SystemCounterVal < TimerTicks64) {
         SystemCounterVal = ArmGenericTimerGetSystemCount ();
    }
}

/*
 * @ IMPLEMENTED
 */
LARGE_INTEGER
NTAPI
KeQueryPerformanceCounter(IN PLARGE_INTEGER PerformanceFreq)
{
    LARGE_INTEGER Value;

    if (PerformanceFreq) PerformanceFreq->QuadPart = ArmGenericTimerGetTimerFreq ();
    Value.QuadPart = ArmGenericTimerGetSystemCount();

    return Value;
}