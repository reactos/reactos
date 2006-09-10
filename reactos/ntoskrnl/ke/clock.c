/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/clock.c
 * PURPOSE:         Handle System Clock
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net) - Created
 *                  David Welch & Phillip Susi - Implementation (?)
 */

 /* NOTES ******************************************************************/
/*
 * System time units are 100-nanosecond intervals
 */

/* INCLUDES ***************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, KiInitializeSystemClock)
#endif

/* GLOBALS ****************************************************************/

/*
 * Current time
 */
#if defined(__GNUC__)
LARGE_INTEGER SystemBootTime = (LARGE_INTEGER)0LL;
#else
LARGE_INTEGER SystemBootTime = { 0 };
#endif

CHAR KiTimerSystemAuditing = 0;
KDPC KiExpireTimerDpc;
BOOLEAN KiClockSetupComplete = FALSE;


/*
 * Number of timer interrupts since initialisation
 */
volatile KSYSTEM_TIME KeTickCount = {0};
volatile ULONG KiRawTicks = 0;
LONG KiTickOffset = 0;
extern LIST_ENTRY KiTimerListHead;

/*
 * The increment in the system clock every timer tick (in system time units)
 *
 * = (1/18.2)*10^9
 *
 * RJJ was 54945055
 */
#define CLOCK_INCREMENT (100000)

ULONG KeMaximumIncrement = 100000;
ULONG KeMinimumIncrement = 100000;
ULONG KeTimeAdjustment   = 100000;

#define MICROSECONDS_PER_TICK (10000)
#define TICKS_TO_CALIBRATE (1)
#define CALIBRATE_PERIOD (MICROSECONDS_PER_TICK * TICKS_TO_CALIBRATE)

/* FUNCTIONS **************************************************************/

/*
 * FUNCTION: Initializes timer irq handling
 * NOTE: This is only called once from main()
 */
VOID
INIT_FUNCTION
NTAPI
KiInitializeSystemClock(VOID)
{
    TIME_FIELDS TimeFields;

    /* Calculate the starting time for the system clock */
    HalQueryRealTimeClock(&TimeFields);
    RtlTimeFieldsToTime(&TimeFields, &SystemBootTime);

    /* Set up the Used Shared Data */
    SharedUserData->SystemTime.High2Time = SystemBootTime.u.HighPart;
    SharedUserData->SystemTime.LowPart = SystemBootTime.u.LowPart;
    SharedUserData->SystemTime.High1Time = SystemBootTime.u.HighPart;
    KiClockSetupComplete = TRUE;
}

VOID
NTAPI
KiSetSystemTime(PLARGE_INTEGER NewSystemTime)
{
  LARGE_INTEGER OldSystemTime;
  LARGE_INTEGER DeltaTime;
  KIRQL OldIrql;

  ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

  OldIrql = KiAcquireDispatcherLock();

  do
    {
      OldSystemTime.u.HighPart = SharedUserData->SystemTime.High1Time;
      OldSystemTime.u.LowPart = SharedUserData->SystemTime.LowPart;
    }
  while (OldSystemTime.u.HighPart != SharedUserData->SystemTime.High2Time);

  /* Set the new system time */
  SharedUserData->SystemTime.LowPart = NewSystemTime->u.LowPart;
  SharedUserData->SystemTime.High1Time = NewSystemTime->u.HighPart;
  SharedUserData->SystemTime.High2Time = NewSystemTime->u.HighPart;

  /* Calculate the difference between the new and the old time */
  DeltaTime.QuadPart = NewSystemTime->QuadPart - OldSystemTime.QuadPart;

  /* Update system boot time */
  SystemBootTime.QuadPart += DeltaTime.QuadPart;

  /* Update absolute timers */
  DPRINT1("FIXME: TIMER UPDATE NOT DONE!!!\n");

  KiReleaseDispatcherLock(OldIrql);

  /*
   * NOTE: Expired timers will be processed at the next clock tick!
   */
}

/*
 * @implemented
 */
ULONG
STDCALL
KeQueryTimeIncrement(VOID)
/*
 * FUNCTION: Gets the increment (in 100-nanosecond units) that is added to
 * the system clock every time the clock interrupts
 * RETURNS: The increment
 */
{
    return KeMaximumIncrement;
}

/*
 * @implemented
 */
#undef KeQueryTickCount
VOID
STDCALL
KeQueryTickCount(PLARGE_INTEGER TickCount)
/*
 * FUNCTION: Returns the number of ticks since the system was booted
 * ARGUMENTS:
 *         TickCount (OUT) = Points to storage for the number of ticks
 */
{
    TickCount->QuadPart = *(PULONGLONG)&KeTickCount;
}

/*
 * FUNCTION: Gets the current system time
 * ARGUMENTS:
 *          CurrentTime (OUT) = The routine stores the current time here
 * NOTE: The time is the number of 100-nanosecond intervals since the
 * 1st of January, 1601.
 *
 * @implemented
 */
VOID
STDCALL
KeQuerySystemTime(PLARGE_INTEGER CurrentTime)
{
    do {
        CurrentTime->u.HighPart = SharedUserData->SystemTime.High1Time;
        CurrentTime->u.LowPart = SharedUserData->SystemTime.LowPart;
    } while (CurrentTime->u.HighPart != SharedUserData->SystemTime.High2Time);
}

ULONGLONG
STDCALL
KeQueryInterruptTime(VOID)
{
    LARGE_INTEGER CurrentTime;

    do {
        CurrentTime.u.HighPart = SharedUserData->InterruptTime.High1Time;
        CurrentTime.u.LowPart = SharedUserData->InterruptTime.LowPart;
    } while (CurrentTime.u.HighPart != SharedUserData->InterruptTime.High2Time);

    return CurrentTime.QuadPart;
}

/*
 * @implemented
 */
VOID
STDCALL
KeSetTimeIncrement(
    IN ULONG MaxIncrement,
    IN ULONG MinIncrement)
{
    /* Set some Internal Variables */
    /* FIXME: We use a harcoded CLOCK_INCREMENT. That *must* be changed */
    KeMaximumIncrement = MaxIncrement;
    KeMinimumIncrement = MinIncrement;
}

/*
 * @implemented
 */
ULONG
STDCALL
NtGetTickCount(VOID)
{
    LARGE_INTEGER TickCount;

    KeQueryTickCount(&TickCount);
    return TickCount.u.LowPart;
}

NTSTATUS
STDCALL
NtQueryTimerResolution(OUT PULONG MinimumResolution,
                       OUT PULONG MaximumResolution,
                       OUT PULONG ActualResolution)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
STDCALL
NtSetTimerResolution(IN ULONG DesiredResolution,
                     IN BOOLEAN SetResolution,
                     OUT PULONG CurrentResolution)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
