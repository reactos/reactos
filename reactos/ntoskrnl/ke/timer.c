/* $Id: timer.c,v 1.64 2003/12/30 18:52:04 fireball Exp $
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

#include <limits.h>
#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/id.h>
#include <internal/ps.h>
#include <internal/safe.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES *****************************************************************/

#define TIMER_IRQ 0

/* GLOBALS ****************************************************************/

/*
 * Current time
 */
static LONGLONG boot_time   = 0;
static LONGLONG system_time = 0;

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

/*
 * PURPOSE: List of timers
 */
static LIST_ENTRY TimerListHead;
static KSPIN_LOCK TimerListLock;
static KSPIN_LOCK TimerValueLock;
static KDPC ExpireTimerDpc;

/* must raise IRQL to PROFILE_LEVEL and grab spin lock there, to sync with ISR */

extern ULONG PiNrRunnableThreads;

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
}


NTSTATUS STDCALL
NtSetTimerResolution(IN ULONG RequestedResolution,
		     IN BOOL SetOrUnset,
		     OUT PULONG ActualResolution)
{
  UNIMPLEMENTED;
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
  KIRQL oldIrql;

  KeRaiseIrql(PROFILE_LEVEL, &oldIrql);
  KeAcquireSpinLockAtDpcLevel(&TimerValueLock);
  CurrentTime->QuadPart = system_time;
  KeReleaseSpinLockFromDpcLevel(&TimerValueLock);
  KeLowerIrql(oldIrql);
}


NTSTATUS STDCALL
NtGetTickCount (PULONG	UpTime)
{
  LARGE_INTEGER TickCount;
  if (UpTime == NULL)
  {
    return(STATUS_INVALID_PARAMETER);
  }

  KeQueryTickCount(&TickCount);
  return(MmCopyToCaller(UpTime, &TickCount.u.LowPart, sizeof(*UpTime)));
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
   LARGE_INTEGER SystemTime;
   BOOLEAN AlreadyInList;

   DPRINT("KeSetTimerEx(Timer %x), DueTime: \n",Timer);

   assert(KeGetCurrentIrql() <= DISPATCH_LEVEL);

   KeRaiseIrql(DISPATCH_LEVEL, &oldlvl);
   KeQuerySystemTime(&SystemTime);
   KeAcquireSpinLockAtDpcLevel(&TimerListLock);

   Timer->Dpc = Dpc;
   if (DueTime.QuadPart < 0)
     {
	Timer->DueTime.QuadPart = SystemTime.QuadPart - DueTime.QuadPart;
     }
   else
     {
	Timer->DueTime.QuadPart = DueTime.QuadPart;
     }
   Timer->Period = Period;
   Timer->Header.SignalState = FALSE;
   AlreadyInList = (Timer->TimerListEntry.Flink == NULL) ? FALSE : TRUE;
   assert((Timer->TimerListEntry.Flink == NULL && Timer->TimerListEntry.Blink == NULL) ||
          (Timer->TimerListEntry.Flink != NULL && Timer->TimerListEntry.Blink != NULL));
   if (!AlreadyInList)
     {
	assert(&TimerListHead != &Timer->TimerListEntry);
	InsertTailList(&TimerListHead, &Timer->TimerListEntry);
	assert(TimerListHead.Flink != &TimerListHead);
	assert(Timer->TimerListEntry.Flink != &Timer->TimerListEntry);
     }
   /*
    * TODO: Perhaps verify that the timer really is in
    * the TimerListHead list if AlreadyInList is TRUE?
    */
   KeReleaseSpinLockFromDpcLevel(&TimerListLock);
   KeLowerIrql(oldlvl);

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
   assert(&Timer->TimerListEntry != &TimerListHead);
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
 * We enter this function at IRQL DISPATCH_LEVEL, and with the
 * TimerListLock held.
 */
STATIC VOID 
HandleExpiredTimer(PKTIMER Timer)
{
   DPRINT("HandleExpiredTime(Timer %x) at IRQL: %d\n", Timer, KeGetCurrentIrql());
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
     }
   else
     {
	assert(&Timer->TimerListEntry != &TimerListHead);
	RemoveEntryList(&Timer->TimerListEntry);
	Timer->TimerListEntry.Flink = Timer->TimerListEntry.Blink = NULL;
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
   LARGE_INTEGER SystemTime;

   DPRINT("KeExpireTimers()\n");

   assert(KeGetCurrentIrql() == DISPATCH_LEVEL);

   KeQuerySystemTime(&SystemTime);

   if (KeGetCurrentIrql() > DISPATCH_LEVEL)
   {
       DPRINT1("-----------------------------\n");
       KEBUGCHECK(0);
   }

   KeAcquireSpinLockAtDpcLevel(&TimerListLock);

   current_entry = TimerListHead.Flink;

   assert(current_entry);

   while (current_entry != &TimerListHead)
     {
       current = CONTAINING_RECORD(current_entry, KTIMER, TimerListEntry);
       assert(current);
       assert(current_entry != &TimerListHead);
       assert(current_entry->Flink != current_entry);

       current_entry = current_entry->Flink;
       if ((ULONGLONG) SystemTime.QuadPart >= current->DueTime.QuadPart)
	 {
	   HandleExpiredTimer(current);
	 }
     }

   KiAddProfileEvent(ProfileTime, Eip);

   KeReleaseSpinLockFromDpcLevel(&TimerListLock);
}


VOID
KiUpdateSystemTime(KIRQL oldIrql,
		   ULONG Eip)
/*
 * FUNCTION: Handles a timer interrupt
 */
{

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

   KeAcquireSpinLockAtDpcLevel(&TimerValueLock);
   system_time += CLOCK_INCREMENT;
   KeReleaseSpinLockFromDpcLevel(&TimerValueLock);

   /*
    * Queue a DPC that will expire timers
    */
   KeInsertQueueDpc(&ExpireTimerDpc, (PVOID)Eip, 0);
}


VOID INIT_FUNCTION
KeInitializeTimerImpl(VOID)
/*
 * FUNCTION: Initializes timer irq handling
 * NOTE: This is only called once from main()
 */
{
   TIME_FIELDS TimeFields;
   LARGE_INTEGER SystemBootTime;

   DPRINT("KeInitializeTimerImpl()\n");
   InitializeListHead(&TimerListHead);
   KeInitializeSpinLock(&TimerListLock);
   KeInitializeSpinLock(&TimerValueLock);
   KeInitializeDpc(&ExpireTimerDpc, KeExpireTimers, 0);
   TimerInitDone = TRUE;
   /*
    * Calculate the starting time for the system clock
    */
   HalQueryRealTimeClock(&TimeFields);
   RtlTimeFieldsToTime(&TimeFields, &SystemBootTime);
   boot_time=SystemBootTime.QuadPart;
   system_time=boot_time;

   SharedUserData->TickCountLow = 0;
   SharedUserData->TickCountMultiplier = 167783691; // 2^24 * 1193182 / 119310

   DPRINT("Finished KeInitializeTimerImpl()\n");
}
