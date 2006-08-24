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
static KDPC KiExpireTimerDpc;
BOOLEAN KiClockSetupComplete = FALSE;

extern ULONG KiMaximumDpcQueueDepth;
extern ULONG KiMinimumDpcRate;
extern ULONG KiAdjustDpcThreshold;
extern ULONG KiIdealDpcRate;

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

    DPRINT("KiInitializeSystemClock()\n");
    InitializeListHead(&KiTimerListHead);
    KeInitializeDpc(&KiExpireTimerDpc, (PKDEFERRED_ROUTINE)KiExpireTimers, 0);

    /* Calculate the starting time for the system clock */
    HalQueryRealTimeClock(&TimeFields);
    RtlTimeFieldsToTime(&TimeFields, &SystemBootTime);

    /* Set up the Used Shared Data */
    SharedUserData->TickCountLowDeprecated = 0;
    SharedUserData->TickCountMultiplier = 167783691; // 2^24 * 1193182 / 119310
    SharedUserData->InterruptTime.High2Time = 0;
    SharedUserData->InterruptTime.LowPart = 0;
    SharedUserData->InterruptTime.High1Time = 0;
    SharedUserData->SystemTime.High2Time = SystemBootTime.u.HighPart;
    SharedUserData->SystemTime.LowPart = SystemBootTime.u.LowPart;
    SharedUserData->SystemTime.High1Time = SystemBootTime.u.HighPart;

    KiClockSetupComplete = TRUE;
    DPRINT("Finished KiInitializeSystemClock()\n");
}

VOID
NTAPI
KiSetSystemTime(PLARGE_INTEGER NewSystemTime)
{
  LARGE_INTEGER OldSystemTime;
  LARGE_INTEGER DeltaTime;
  KIRQL OldIrql;

  ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

  OldIrql = KeAcquireDispatcherDatabaseLock();

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

  KeReleaseDispatcherDatabaseLock(OldIrql);

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
 * @unimplemented
 */
VOID
FASTCALL
KeSetTimeUpdateNotifyRoutine(
    IN PTIME_UPDATE_NOTIFY_ROUTINE NotifyRoutine
    )
{
    UNIMPLEMENTED;
}

/*
 * NOTE: On Windows this function takes exactly one parameter and EBP is
 *       guaranteed to point to KTRAP_FRAME. The function is used only
 *       by HAL, so there's no point in keeping that prototype.
 *
 * @implemented
 */
VOID
STDCALL
KeUpdateRunTime(IN PKTRAP_FRAME  TrapFrame,
                IN KIRQL  Irql)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    PKTHREAD CurrentThread;
    PKPROCESS CurrentProcess;

    /* Make sure we don't go further if we're in early boot phase. */
    if (!(Prcb) || !(Prcb->CurrentThread)) return;

    /* Get the current thread and process */
    CurrentThread = Prcb->CurrentThread;
    CurrentProcess = CurrentThread->ApcState.Process;

    /* Check if we came from user mode */
    if (TrapFrame->PreviousPreviousMode != KernelMode)
    {
        /* Update user times */
        CurrentThread->UserTime++;
        InterlockedIncrement((PLONG)&CurrentProcess->UserTime);
        Prcb->UserTime++;
    }
    else
    {
        /* Check IRQ */
        if (Irql > DISPATCH_LEVEL)
        {
            /* This was an interrupt */
            Prcb->InterruptTime++;
        }
        else if ((Irql < DISPATCH_LEVEL) || !(Prcb->DpcRoutineActive))
        {
            /* This was normal kernel time */
            CurrentThread->KernelTime++;
            InterlockedIncrement((PLONG)&CurrentProcess->KernelTime);
        }
        else if (Irql == DISPATCH_LEVEL)
        {
            /* This was DPC time */
            Prcb->DpcTime++;
        }

        /* Update CPU kernel time in all cases */
        Prcb->KernelTime++;
   }

    /* Set the last DPC Count and request rate */
    Prcb->DpcLastCount = Prcb->DpcData[0].DpcCount;
    Prcb->DpcRequestRate = ((Prcb->DpcData[0].DpcCount - Prcb->DpcLastCount) +
                             Prcb->DpcRequestRate) / 2;

    /* Check if we should request a DPC */
    if ((Prcb->DpcData[0].DpcQueueDepth) && !(Prcb->DpcRoutineActive))
    {
        /* Request one */
        HalRequestSoftwareInterrupt(DISPATCH_LEVEL);

        /* Update the depth if needed */
        if ((Prcb->DpcRequestRate < KiIdealDpcRate) &&
            (Prcb->MaximumDpcQueueDepth > 1))
        {
            /* Decrease the maximum depth by one */
            Prcb->MaximumDpcQueueDepth--;
        }
    }
    else
    {
        /* Decrease the adjustment threshold */
        if (!(--Prcb->AdjustDpcThreshold))
        {
            /* We've hit 0, reset it */
            Prcb->AdjustDpcThreshold = KiAdjustDpcThreshold;

            /* Check if we've hit queue maximum */
            if (KiMaximumDpcQueueDepth != Prcb->MaximumDpcQueueDepth)
            {
                /* Increase maximum by one */
                Prcb->MaximumDpcQueueDepth++;
            }
        }
    }

   /*
    * If we're at end of quantum request software interrupt. The rest
    * is handled in KiDispatchInterrupt.
    *
    * NOTE: If one stays at DISPATCH_LEVEL for a long time the DPC routine
    * which checks for quantum end will not be executed and decrementing
    * the quantum here can result in overflow. This is not a problem since
    * we don't care about the quantum value anymore after the QuantumEnd
    * flag is set.
    */
    if ((CurrentThread->Quantum -= 3) <= 0)
    {
        Prcb->QuantumEnd = TRUE;
        HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
    }
}


/*
 * NOTE: On Windows this function takes exactly zero parameters and EBP is
 *       guaranteed to point to KTRAP_FRAME. Also [esp+0] contains an IRQL.
 *       The function is used only by HAL, so there's no point in keeping
 *       that prototype.
 *
 * @implemented
 */
VOID
STDCALL
KeUpdateSystemTime(IN PKTRAP_FRAME TrapFrame,
                   IN KIRQL Irql,
                   IN ULONG Increment)
{
    LONG OldOffset;
    LARGE_INTEGER Time;
    ASSERT(KeGetCurrentIrql() == PROFILE_LEVEL);

    /* Update interrupt time */
    Time.LowPart = SharedUserData->InterruptTime.LowPart;
    Time.HighPart = SharedUserData->InterruptTime.High1Time;
    Time.QuadPart += Increment;
    SharedUserData->InterruptTime.High2Time = Time.u.HighPart;
    SharedUserData->InterruptTime.LowPart = Time.u.LowPart;
    SharedUserData->InterruptTime.High1Time = Time.u.HighPart;

    /* Increase the tick offset */
    KiTickOffset -= Increment;
    OldOffset = KiTickOffset;

    /* Check if this isn't a tick yet */
    if (KiTickOffset > 0)
    {
        /* Expire timers */
        KeInsertQueueDpc(&KiExpireTimerDpc, (PVOID)TrapFrame->Eip, 0);
    }
    else
    {
        /* Setup time structure for system time */
        Time.LowPart = SharedUserData->SystemTime.LowPart;
        Time.HighPart = SharedUserData->SystemTime.High1Time;
        Time.QuadPart += KeTimeAdjustment;
        SharedUserData->SystemTime.High2Time = Time.HighPart;
        SharedUserData->SystemTime.LowPart = Time.LowPart;
        SharedUserData->SystemTime.High1Time = Time.HighPart;

        /* Setup time structure for tick time */
        Time.LowPart = KeTickCount.LowPart;
        Time.HighPart = KeTickCount.High1Time;
        Time.QuadPart += 1;
        KeTickCount.High2Time = Time.HighPart;
        KeTickCount.LowPart = Time.LowPart;
        KeTickCount.High1Time = Time.HighPart;
        SharedUserData->TickCount.High2Time = Time.HighPart;
        SharedUserData->TickCount.LowPart = Time.LowPart;
        SharedUserData->TickCount.High1Time = Time.HighPart;

        /* Update tick count in shared user data as well */
        SharedUserData->TickCountLowDeprecated++;

        /* Queue a DPC that will expire timers */
        KeInsertQueueDpc(&KiExpireTimerDpc, (PVOID)TrapFrame->Eip, 0);
    }

    /* Update process and thread times */
    if (OldOffset <= 0)
    {
        /* This was a tick, calculate the next one */
        KiTickOffset += KeMaximumIncrement;
        KeUpdateRunTime(TrapFrame, Irql);
    }
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
