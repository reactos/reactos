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
static BOOLEAN KiClockSetupComplete = FALSE;

/*
 * Number of timer interrupts since initialisation
 */
volatile ULONGLONG KeTickCount = 0;
volatile ULONG KiRawTicks = 0;

extern LIST_ENTRY KiTimerListHead;

/*
 * The increment in the system clock every timer tick (in system time units)
 * 
 * = (1/18.2)*10^9 
 * 
 * RJJ was 54945055
 */
#define CLOCK_INCREMENT (100000)

#ifdef  __GNUC__
ULONG EXPORTED KeMaximumIncrement = 100000;
ULONG EXPORTED KeMinimumIncrement = 100000;
#else
/* Microsoft-style declarations */
EXPORTED ULONG KeMaximumIncrement = 100000;
EXPORTED ULONG KeMinimumIncrement = 100000;
#endif

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
KiInitializeSystemClock(VOID)
{
    TIME_FIELDS TimeFields;

    DPRINT1("KiInitializeSystemClock()\n");
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
    DPRINT1("Finished KiInitializeSystemClock()\n");
}

VOID
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
    TickCount->QuadPart = KeTickCount;
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
KeUpdateRunTime(
    IN PKTRAP_FRAME  TrapFrame,
    IN KIRQL  Irql
    )
{
   PKPCR Pcr;
   PKTHREAD CurrentThread;
   PKPROCESS CurrentProcess;
#if 0
   ULONG DpcLastCount;
#endif

   Pcr = KeGetCurrentKPCR();

   /* Make sure we don't go further if we're in early boot phase. */
   if (Pcr == NULL || Pcr->PrcbData.CurrentThread == NULL)
      return;

   DPRINT("KernelTime  %u, UserTime %u \n", Pcr->PrcbData.KernelTime, Pcr->PrcbData.UserTime);

   CurrentThread = Pcr->PrcbData.CurrentThread;
   CurrentProcess = CurrentThread->ApcState.Process;

   /* 
    * Cs bit 0 is always set for user mode if we are in protected mode.
    * V86 mode is counted as user time.
    */
   if (TrapFrame->Cs & 0x1 ||
       TrapFrame->Eflags & X86_EFLAGS_VM)
   {
      InterlockedIncrementUL(&CurrentThread->UserTime);
      InterlockedIncrementUL(&CurrentProcess->UserTime);
      Pcr->PrcbData.UserTime++;
   }
   else
   {
      if (Irql > DISPATCH_LEVEL)
      {
         Pcr->PrcbData.InterruptTime++;
      }
      else if (Irql == DISPATCH_LEVEL)
      {
         Pcr->PrcbData.DpcTime++;
      }
      else
      {
         InterlockedIncrementUL(&CurrentThread->KernelTime);
         InterlockedIncrementUL(&CurrentProcess->KernelTime);
	 Pcr->PrcbData.KernelTime++;
      }
   }

#if 0
   DpcLastCount = Pcr->PrcbData.DpcLastCount;
   Pcr->PrcbData.DpcLastCount = Pcr->PrcbData.DpcCount;
   Pcr->PrcbData.DpcRequestRate = ((Pcr->PrcbData.DpcCount - DpcLastCount) +
                                   Pcr->PrcbData.DpcRequestRate) / 2;
#endif

   if (Pcr->PrcbData.DpcData[0].DpcQueueDepth > 0 &&
       Pcr->PrcbData.DpcRoutineActive == FALSE &&
       Pcr->PrcbData.DpcInterruptRequested == FALSE)
   {
      HalRequestSoftwareInterrupt(DISPATCH_LEVEL);
   }

   /* FIXME: Do DPC rate adjustments */

   /*
    * If we're at end of quantum request software interrupt. The rest
    * is handled in KiDispatchInterrupt.
    */
   if ((CurrentThread->Quantum -= 3) <= 0)
   {
     Pcr->PrcbData.QuantumEnd = TRUE;
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
KeUpdateSystemTime(
    IN PKTRAP_FRAME  TrapFrame,
    IN KIRQL  Irql
    )
/*
 * FUNCTION: Handles a timer interrupt
 */
{
   LARGE_INTEGER Time;

   ASSERT(KeGetCurrentIrql() == PROFILE_LEVEL);

   KiRawTicks++;
   
   if (KiClockSetupComplete == FALSE) return;

   /*
    * Increment the number of timers ticks 
    */
   KeTickCount++;
   SharedUserData->TickCountLowDeprecated++;

   Time.u.LowPart = SharedUserData->InterruptTime.LowPart;
   Time.u.HighPart = SharedUserData->InterruptTime.High1Time;
   Time.QuadPart += CLOCK_INCREMENT;
   SharedUserData->InterruptTime.High2Time = Time.u.HighPart;
   SharedUserData->InterruptTime.LowPart = Time.u.LowPart;
   SharedUserData->InterruptTime.High1Time = Time.u.HighPart;

   Time.u.LowPart = SharedUserData->SystemTime.LowPart;
   Time.u.HighPart = SharedUserData->SystemTime.High1Time;
   Time.QuadPart += CLOCK_INCREMENT;
   SharedUserData->SystemTime.High2Time = Time.u.HighPart;
   SharedUserData->SystemTime.LowPart = Time.u.LowPart;
   SharedUserData->SystemTime.High1Time = Time.u.HighPart;

   /* FIXME: Here we should check for remote debugger break-ins */

   /* Update process and thread times */
   KeUpdateRunTime(TrapFrame, Irql);

   /*
    * Queue a DPC that will expire timers
    */
   KeInsertQueueDpc(&KiExpireTimerDpc, (PVOID)TrapFrame->Eip, 0);
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
