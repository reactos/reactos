/* $Id: timer.c,v 1.81 2004/10/17 15:39:29 hbirr Exp $
 *
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/ke/timer.c
 * PURPOSE:        Handle timers
 * PROGRAMMER:     David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                 28/05/98: Created
 *                 12/3/99:  Phillip Susi: enabled the timers, fixed spin lock
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

/*
 * Number of timer interrupts since initialisation
 */
volatile ULONGLONG KeTickCount = 0;
volatile ULONG KiRawTicks = 0;

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



/*
 * PURPOSE: List of timers
 */
static LIST_ENTRY AbsoluteTimerListHead;
static LIST_ENTRY RelativeTimerListHead;
static KSPIN_LOCK TimerListLock;
static KSPIN_LOCK TimerValueLock;
static KDPC ExpireTimerDpc;

/* must raise IRQL to PROFILE_LEVEL and grab spin lock there, to sync with ISR */

extern HANDLE PsIdleThreadHandle;

#define MICROSECONDS_PER_TICK (10000)
#define TICKS_TO_CALIBRATE (1)
#define CALIBRATE_PERIOD (MICROSECONDS_PER_TICK * TICKS_TO_CALIBRATE)
#define SYSTEM_TIME_UNITS_PER_MSEC (10000)

static BOOLEAN TimerInitDone = FALSE;

/* FUNCTIONS **************************************************************/


NTSTATUS STDCALL
NtQueryTimerResolution(OUT PULONG MinimumResolution,
		       OUT PULONG MaximumResolution,
		       OUT PULONG ActualResolution)
{
  UNIMPLEMENTED;
  return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS STDCALL
NtSetTimerResolution(IN ULONG RequestedResolution,
		     IN BOOL SetOrUnset,
		     OUT PULONG ActualResolution)
{
  UNIMPLEMENTED;
  return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS STDCALL
NtQueryPerformanceCounter(IN PLARGE_INTEGER Counter,
			  IN PLARGE_INTEGER Frequency)
{
  LARGE_INTEGER PerfCounter;
  LARGE_INTEGER PerfFrequency;
  NTSTATUS      Status;

  PerfCounter = KeQueryPerformanceCounter(&PerfFrequency);

  if (Counter != NULL)
    {
      Status = MmCopyToCaller(&Counter->QuadPart, &PerfCounter.QuadPart, sizeof(PerfCounter.QuadPart));
      if (!NT_SUCCESS(Status))
        {
	  return(Status);
        }
    }

  if (Frequency != NULL)
  {
      Status = MmCopyToCaller(&Frequency->QuadPart, &PerfFrequency.QuadPart, sizeof(PerfFrequency.QuadPart));
      if (!NT_SUCCESS(Status))
        {
	  return(Status);
        }
  }

  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtDelayExecution(IN ULONG Alertable,
		 IN TIME* Interval)
{
   NTSTATUS Status;
   LARGE_INTEGER Timeout;

   Status = MmCopyFromCaller(&Timeout, Interval, sizeof(Timeout));
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   Timeout = *((PLARGE_INTEGER)Interval);
   DPRINT("NtDelayExecution(Alertable %d, Internal %x) IntervalP %x\n",
	  Alertable, Internal, Timeout);
   
   DPRINT("Execution delay is %d/%d\n", 
	  Timeout.u.HighPart, Timeout.u.LowPart);
   Status = KeDelayExecutionThread(UserMode, (BOOLEAN)Alertable, &Timeout);
   return(Status);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
KeDelayExecutionThread (KPROCESSOR_MODE	WaitMode,
			BOOLEAN		Alertable,
			PLARGE_INTEGER	Interval)
/*
 * FUNCTION: Puts the current thread into an alertable or nonalertable 
 * wait state for a given internal
 * ARGUMENTS:
 *          WaitMode = Processor mode in which the caller is waiting
 *          Altertable = Specifies if the wait is alertable
 *          Interval = Specifies the interval to wait
 * RETURNS: Status
 */
{
   PKTHREAD Thread = KeGetCurrentThread();

   KeSetTimer(&Thread->Timer, *Interval, NULL);
   return (KeWaitForSingleObject(&Thread->Timer,
				 (WaitMode == KernelMode) ? Executive : UserRequest, /* TMN: Was unconditionally Executive */
				 WaitMode, /* TMN: Was UserMode */
				 Alertable,
				 NULL));
}


/*
 * @implemented
 */
ULONG STDCALL
KeQueryTimeIncrement(VOID)
/*
 * FUNCTION: Gets the increment (in 100-nanosecond units) that is added to 
 * the system clock every time the clock interrupts
 * RETURNS: The increment
 */
{
  return(CLOCK_INCREMENT);
}


/*
 * @implemented
 */
VOID STDCALL
KeQuerySystemTime(PLARGE_INTEGER CurrentTime)
/*
 * FUNCTION: Gets the current system time
 * ARGUMENTS:
 *          CurrentTime (OUT) = The routine stores the current time here
 * NOTE: The time is the number of 100-nanosecond intervals since the
 * 1st of January, 1601.
 */
{
  do
    {
      CurrentTime->u.HighPart = SharedUserData->SystemTime.High1Part;
      CurrentTime->u.LowPart = SharedUserData->SystemTime.LowPart;
    }
  while (CurrentTime->u.HighPart != SharedUserData->SystemTime.High2Part);
}

ULONGLONG STDCALL
KeQueryInterruptTime(VOID)
{
  LARGE_INTEGER CurrentTime;

  do
    {
      CurrentTime.u.HighPart = SharedUserData->InterruptTime.High1Part;
      CurrentTime.u.LowPart = SharedUserData->InterruptTime.LowPart;
    }
  while (CurrentTime.u.HighPart != SharedUserData->InterruptTime.High2Part);

  return CurrentTime.QuadPart;
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

/*
 * @implemented
 */
BOOLEAN STDCALL
KeSetTimer (PKTIMER		Timer,
	    LARGE_INTEGER	DueTime,
	    PKDPC		Dpc)
/*
 * FUNCTION: Sets the absolute or relative interval at which a timer object
 * is to be set to the signaled state and optionally supplies a 
 * CustomTimerDpc to be executed when the timer expires.
 * ARGUMENTS:
 *          Timer = Points to a previously initialized timer object
 *          DueTimer = If positive then absolute time to expire at
 *                     If negative then the relative time to expire at
 *          Dpc = If non-NULL then a dpc to be called when the timer expires
 * RETURNS: True if the timer was already in the system timer queue
 *          False otherwise
 */
{
   return(KeSetTimerEx(Timer, DueTime, 0, Dpc));
}

/*
 * @implemented
 */
BOOLEAN STDCALL
KeSetTimerEx (PKTIMER		Timer,
	      LARGE_INTEGER	DueTime,
	      LONG		Period,
	      PKDPC		Dpc)
/*
 * FUNCTION: Sets the absolute or relative interval at which a timer object
 * is to be set to the signaled state and optionally supplies a 
 * CustomTimerDpc to be executed when the timer expires.
 * ARGUMENTS:
 *          Timer = Points to a previously initialized timer object
 *          DueTimer = If positive then absolute time to expire at
 *                     If negative then the relative time to expire at
 *          Dpc = If non-NULL then a dpc to be called when the timer expires
 * RETURNS: True if the timer was already in the system timer queue
 *          False otherwise
 */
{
   KIRQL oldlvl;
   LARGE_INTEGER Time;
   BOOLEAN AlreadyInList;

   DPRINT("KeSetTimerEx(Timer %x), DueTime: \n",Timer);

   assert(KeGetCurrentIrql() <= DISPATCH_LEVEL);

   KeAcquireSpinLock(&TimerListLock, &oldlvl);

   Timer->Dpc = Dpc;
   if (DueTime.QuadPart < 0)
     {
        Timer->Header.Absolute = 0;
	Timer->DueTime.QuadPart = KeQueryInterruptTime() - DueTime.QuadPart;
     }
   else
     {
        KeQuerySystemTime(&Time);
        Timer->Header.Absolute = 1;
	if (DueTime.QuadPart >= Time.QuadPart)
	  {
            Timer->DueTime.QuadPart = DueTime.QuadPart;
	  }
	else
	  {
	    Timer->DueTime.QuadPart = Time.QuadPart;
	  }
     }
   Timer->Period = Period;
   Timer->Header.SignalState = FALSE;
   AlreadyInList = (Timer->TimerListEntry.Flink == NULL) ? FALSE : TRUE;
   assert((Timer->TimerListEntry.Flink == NULL && Timer->TimerListEntry.Blink == NULL) ||
          (Timer->TimerListEntry.Flink != NULL && Timer->TimerListEntry.Blink != NULL));
   if (AlreadyInList)
     {
       RemoveEntryList(&Timer->TimerListEntry);
     }
   if (Timer->Header.Absolute)
     {
       InsertAscendingList(&AbsoluteTimerListHead, 
                           KTIMER,
                           TimerListEntry, 
                           Timer,
                           DueTime.QuadPart);

     }
   else
     {
       InsertAscendingList(&RelativeTimerListHead, 
                          KTIMER,
                          TimerListEntry, 
                          Timer, 
                          DueTime.QuadPart);

     }

   KeReleaseSpinLock(&TimerListLock, oldlvl);

   return AlreadyInList;
}

/*
 * @implemented
 */
BOOLEAN STDCALL
KeCancelTimer (PKTIMER	Timer)
/*
 * FUNCTION: Removes a timer from the system timer list
 * ARGUMENTS:
 *       Timer = timer to cancel
 * RETURNS: True if the timer was running
 *          False otherwise
 */
{
   KIRQL oldlvl;
   
   DPRINT("KeCancelTimer(Timer %x)\n",Timer);

   KeAcquireSpinLock(&TimerListLock, &oldlvl);

   if (Timer->TimerListEntry.Flink == NULL)
     {
	KeReleaseSpinLock(&TimerListLock, oldlvl);
	return(FALSE);
     }
   if (Timer->Header.Absolute)
     {
       assert(&Timer->TimerListEntry != &AbsoluteTimerListHead);
     }
   else
     {
       assert(&Timer->TimerListEntry != &RelativeTimerListHead);
     }
   assert(Timer->TimerListEntry.Flink != &Timer->TimerListEntry);
   RemoveEntryList(&Timer->TimerListEntry);
   Timer->TimerListEntry.Flink = Timer->TimerListEntry.Blink = NULL;
   KeReleaseSpinLock(&TimerListLock, oldlvl);

   return(TRUE);
}

/*
 * @implemented
 */
BOOLEAN STDCALL
KeReadStateTimer (PKTIMER	Timer)
{
   return (BOOLEAN)(Timer->Header.SignalState);
}

/*
 * @implemented
 */
VOID STDCALL
KeInitializeTimer (PKTIMER	Timer)
/*
 * FUNCTION: Initalizes a kernel timer object
 * ARGUMENTS:
 *          Timer = caller supplied storage for the timer
 * NOTE: This function initializes a notification timer
 */
{
   KeInitializeTimerEx(Timer, NotificationTimer);
}

/*
 * @implemented
 */
VOID STDCALL
KeInitializeTimerEx (PKTIMER		Timer,
		     TIMER_TYPE	Type)
/*
 * FUNCTION: Initializes a kernel timer object
 * ARGUMENTS:
 *          Timer = caller supplied storage for the timer
 *          Type = the type of timer (notification or synchronization)
 * NOTE: When a notification type expires all waiting threads are released
 * and the timer remains signalled until it is explicitly reset. When a 
 * syncrhonization timer expires its state is set to signalled until a
 * single waiting thread is released and then the timer is reset.
 */
{
   ULONG IType;

   if (Type == NotificationTimer)
     {
	IType = InternalNotificationTimer;
     }
   else if (Type == SynchronizationTimer)
     {
	IType = InternalSynchronizationTimer;
     }
   else
     {
	assert(FALSE);
	return;
     }

   KeInitializeDispatcherHeader(&Timer->Header,
				IType,
				sizeof(KTIMER) / sizeof(ULONG),
				FALSE);
   Timer->TimerListEntry.Flink = Timer->TimerListEntry.Blink = NULL;
}

/*
 * @implemented
 */
VOID STDCALL
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
 * @implemented
 */
ULONG
STDCALL
KeQueryRuntimeThread(
	IN PKTHREAD Thread,
	OUT PULONG UserTime
	)
{
	/* Return the User Time */
	*UserTime = Thread->UserTime;
	
	/* Return the Kernel Time */
	return Thread->KernelTime;
}

/*
 * @implemented
 */
VOID
STDCALL
KeSetTimeIncrement(
    IN ULONG MaxIncrement,
    IN ULONG MinIncrement
)
{
	/* Set some Internal Variables */
	/* FIXME: We use a harcoded CLOCK_INCREMENT. That *must* be changed */
	KeMaximumIncrement = MaxIncrement;
	KeMinimumIncrement = MinIncrement;
}

/*
 * We enter this function at IRQL DISPATCH_LEVEL, and with the
 * TimerListLock held.
 */
STATIC VOID 
HandleExpiredTimer(PKTIMER Timer)
{
   DPRINT("HandleExpiredTime(Timer %x)\n", Timer);
   if (Timer->Dpc != NULL)
     {
	DPRINT("Timer->Dpc %x Timer->Dpc->DeferredRoutine %x\n",
	       Timer->Dpc, Timer->Dpc->DeferredRoutine);
	KeInsertQueueDpc(Timer->Dpc,
			 NULL,
			 NULL);
	DPRINT("Finished dpc routine\n");
     }

   assert(KeGetCurrentIrql() == DISPATCH_LEVEL);

   KeAcquireDispatcherDatabaseLockAtDpcLevel();
   Timer->Header.SignalState = TRUE;
   KeDispatcherObjectWake(&Timer->Header);
   KeReleaseDispatcherDatabaseLockFromDpcLevel();

   if (Timer->Period != 0)
     {
       Timer->DueTime.QuadPart += 
	 Timer->Period * SYSTEM_TIME_UNITS_PER_MSEC;
       if (Timer->Header.Absolute)
         {
           InsertAscendingList(&AbsoluteTimerListHead, 
                               KTIMER,
                               TimerListEntry,
                               Timer, 
	                             DueTime.QuadPart);
         }
       else
         {
           InsertAscendingList(&RelativeTimerListHead, 
                               KTIMER,
                               TimerListEntry, 
                               Timer,
	                             DueTime.QuadPart);
         }
     }
}

VOID STDCALL
KeExpireTimers(PKDPC Dpc,
	       PVOID Context1,
	       PVOID Arg1,
	       PVOID Arg2)
{
   PLIST_ENTRY current_entry = NULL;
   PKTIMER current = NULL;
   ULONG Eip = (ULONG)Arg1;
   LARGE_INTEGER InterruptTime;
   LARGE_INTEGER SystemTime;
   LIST_ENTRY TimerList;

   DPRINT("KeExpireTimers()\n");

   assert(KeGetCurrentIrql() == DISPATCH_LEVEL);

   InitializeListHead(&TimerList);

   KeAcquireSpinLockAtDpcLevel(&TimerListLock);

   InterruptTime.QuadPart = KeQueryInterruptTime();
   KeQuerySystemTime(&SystemTime);

   current_entry = RelativeTimerListHead.Flink;
   assert(current_entry);
   while (current_entry != &RelativeTimerListHead)
     {
       current = CONTAINING_RECORD(current_entry, KTIMER, TimerListEntry);
       assert(current);
       assert(current_entry != &RelativeTimerListHead);
       assert(current_entry->Flink != current_entry);
       if ((ULONGLONG)InterruptTime.QuadPart < current->DueTime.QuadPart)
         {
	   break;
	 }
       current_entry = current_entry->Flink;
       RemoveEntryList(&current->TimerListEntry);
       InsertTailList(&TimerList, &current->TimerListEntry);
     }

   current_entry = AbsoluteTimerListHead.Flink;
   assert(current_entry);
   while (current_entry != &AbsoluteTimerListHead)
     {
       current = CONTAINING_RECORD(current_entry, KTIMER, TimerListEntry);
       assert(current);
       assert(current_entry != &AbsoluteTimerListHead);
       assert(current_entry->Flink != current_entry);
       if ((ULONGLONG)SystemTime.QuadPart < current->DueTime.QuadPart)
         {
	   break;
	 }
       current_entry = current_entry->Flink;
       RemoveEntryList(&current->TimerListEntry);
       InsertTailList(&TimerList, &current->TimerListEntry);
     }

   while (!IsListEmpty(&TimerList))
     {
       current_entry = RemoveHeadList(&TimerList);
       current = CONTAINING_RECORD(current_entry, KTIMER, TimerListEntry);
       current->TimerListEntry.Flink = current->TimerListEntry.Blink = NULL;
       HandleExpiredTimer(current);
     }

   KiAddProfileEvent(ProfileTime, Eip);

   KeReleaseSpinLockFromDpcLevel(&TimerListLock);
}


VOID
KiUpdateSystemTime(KIRQL oldIrql,
		   PKIRQ_TRAPFRAME Tf)
/*
 * FUNCTION: Handles a timer interrupt
 */
{
   LARGE_INTEGER Time;

   assert(KeGetCurrentIrql() == PROFILE_LEVEL);

   KiRawTicks++;
   
   if (TimerInitDone == FALSE)
     {
	return;
     }
   /*
    * Increment the number of timers ticks 
    */
   KeTickCount++;
   SharedUserData->TickCountLow++;
   KiAcquireSpinLock(&TimerValueLock);

   Time.u.LowPart = SharedUserData->InterruptTime.LowPart;
   Time.u.HighPart = SharedUserData->InterruptTime.High1Part;
   Time.QuadPart += CLOCK_INCREMENT;
   SharedUserData->InterruptTime.High2Part = Time.u.HighPart;
   SharedUserData->InterruptTime.LowPart = Time.u.LowPart;
   SharedUserData->InterruptTime.High1Part = Time.u.HighPart;

   Time.u.LowPart = SharedUserData->SystemTime.LowPart;
   Time.u.HighPart = SharedUserData->SystemTime.High1Part;
   Time.QuadPart += CLOCK_INCREMENT;
   SharedUserData->SystemTime.High2Part = Time.u.HighPart;
   SharedUserData->SystemTime.LowPart = Time.u.LowPart;
   SharedUserData->SystemTime.High1Part = Time.u.HighPart;

   KiReleaseSpinLock(&TimerValueLock);

   /* Update process and thread times */
   KiUpdateProcessThreadTime(oldIrql, Tf);

   /*
    * Queue a DPC that will expire timers
    */
   KeInsertQueueDpc(&ExpireTimerDpc, (PVOID)Tf->Eip, 0);
}


VOID INIT_FUNCTION
KeInitializeTimerImpl(VOID)
/*
 * FUNCTION: Initializes timer irq handling
 * NOTE: This is only called once from main()
 */
{
   TIME_FIELDS TimeFields;

   DPRINT("KeInitializeTimerImpl()\n");
   InitializeListHead(&AbsoluteTimerListHead);
   InitializeListHead(&RelativeTimerListHead);
   KeInitializeSpinLock(&TimerListLock);
   KeInitializeSpinLock(&TimerValueLock);
   KeInitializeDpc(&ExpireTimerDpc, KeExpireTimers, 0);
   /*
    * Calculate the starting time for the system clock
    */
   HalQueryRealTimeClock(&TimeFields);
   RtlTimeFieldsToTime(&TimeFields, &SystemBootTime);

   SharedUserData->TickCountLow = 0;
   SharedUserData->TickCountMultiplier = 167783691; // 2^24 * 1193182 / 119310
   SharedUserData->InterruptTime.High2Part = 0;
   SharedUserData->InterruptTime.LowPart = 0;
   SharedUserData->InterruptTime.High1Part = 0;
   SharedUserData->SystemTime.High2Part = SystemBootTime.u.HighPart;
   SharedUserData->SystemTime.LowPart = SystemBootTime.u.LowPart;
   SharedUserData->SystemTime.High1Part = SystemBootTime.u.HighPart;

   TimerInitDone = TRUE;
   DPRINT("Finished KeInitializeTimerImpl()\n");
}


VOID
KiUpdateProcessThreadTime(KIRQL oldIrql, PKIRQ_TRAPFRAME Tf)
{
   PKTHREAD CurrentThread;
   PEPROCESS ThreadsProcess;
   PKPCR Pcr;

   Pcr = KeGetCurrentKPCR();

/*
 *  Make sure no counting can take place until Processes and Threads are
 *  running!
 */
   if ((PsInitialSystemProcess == NULL) || (Pcr->PrcbData.IdleThread == NULL) || 
        (KiTimerSystemAuditing == 0))
     {
       return;
     }

   DPRINT("KernelTime  %u, UserTime %u \n", Kpcr->PrcbData.KernelTime, Kpcr->PrcbData.UserTime);

   if (oldIrql > DISPATCH_LEVEL)
   {
      Pcr->PrcbData.InterruptTime++;
   }
   else if (oldIrql == DISPATCH_LEVEL)
   {
      Pcr->PrcbData.DpcTime++;
   }
   else
   {
      CurrentThread = KeGetCurrentThread();
      ThreadsProcess = ((PETHREAD)CurrentThread)->ThreadsProcess;
      /* 
       * Cs bit 0 is always set for user mode if we are in protected mode.
       * V86 mode is counted as user time.
       */
      if (Tf->Cs & 0x1 ||
	  Tf->Eflags & X86_EFLAGS_VM)
      {
#ifdef MP
         InterlockedIncrement((PLONG)&ThreadsProcess->Pcb.UserTime);
#else
         ThreadsProcess->Pcb.UserTime++;
#endif
         CurrentThread->UserTime++;
         Pcr->PrcbData.UserTime++;
      }
      else
      {
#ifdef MP
         InterlockedIncrement((PLONG)&ThreadsProcess->Pcb.KernelTime);
#else
         ThreadsProcess->Pcb.KernelTime++;  
#endif
         CurrentThread->KernelTime++;
         Pcr->PrcbData.KernelTime++;
      }  	
   }
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
 * @unimplemented
 */
VOID
STDCALL
KeUpdateRunTime(
	IN PKTRAP_FRAME	TrapFrame
)
{
	KIRQL OldIrql = PASSIVE_LEVEL;
	
	/* These are equivalent... we should just remove the Ki and stick it here... */
	KiUpdateProcessThreadTime(OldIrql, (PKIRQ_TRAPFRAME)TrapFrame);
}

/*
 * @unimplemented
 */
VOID 
STDCALL
KeUpdateSystemTime(
	IN PKTRAP_FRAME TrapFrame,
	IN ULONG        Increment
)
{
	KIRQL OldIrql = PASSIVE_LEVEL;
	
	/* These are equivalent... we should just remove the Ki and stick it here... */
	KiUpdateSystemTime(OldIrql, (PKIRQ_TRAPFRAME)TrapFrame);
}

/*EOF*/

