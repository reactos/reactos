/* $Id: timer.c,v 1.51 2002/08/14 20:58:35 dwelch Exp $
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

#define NDEBUG
#include <internal/debug.h>

/* TYPES *****************************************************************/

#define TIMER_IRQ 0

/* GLOBALS ****************************************************************/

/*
 * Current time
 */
static unsigned long long boot_time = 0;
static unsigned long long system_time = 0;

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
static KDPC ExpireTimerDpc;

/* must raise IRQL to HIGH_LEVEL and grab spin lock there, to sync with ISR */

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
  UNIMPLEMENTED;
}


NTSTATUS STDCALL
NtDelayExecution(IN ULONG Alertable,
		 IN TIME* Interval)
{
   NTSTATUS Status;
   LARGE_INTEGER Timeout;
   
   Timeout = *((PLARGE_INTEGER)Interval);
   DPRINT("NtDelayExecution(Alertable %d, Internal %x) IntervalP %x\n",
	  Alertable, Internal, Timeout);
   
   DPRINT("Execution delay is %d/%d\n", 
	  Timeout.u.HighPart, Timeout.u.LowPart);
   Status = KeDelayExecutionThread(UserMode, Alertable, &Timeout);
   return(Status);
}


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

   KeInitializeTimer(&Thread->Timer);
   KeSetTimer(&Thread->Timer, *Interval, NULL);
   return (KeWaitForSingleObject(&Thread->Timer,
				 Executive,
				 UserMode,
				 Alertable,
				 NULL));
}


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
  CurrentTime->QuadPart = system_time;
}


NTSTATUS STDCALL
NtGetTickCount (PULONG	UpTime)
{
  LARGE_INTEGER TickCount;
  if (UpTime == NULL)
    return(STATUS_INVALID_PARAMETER);
  KeQueryTickCount(&TickCount);
  *UpTime = TickCount.u.LowPart;
  return (STATUS_SUCCESS);
}


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
   
   DPRINT("KeSetTimerEx(Timer %x), DueTime: \n",Timer);
   KeAcquireSpinLock( &TimerListLock, &oldlvl );
   
   Timer->Dpc = Dpc;
   if (DueTime.QuadPart < 0)
     {
	Timer->DueTime.QuadPart = system_time - DueTime.QuadPart;
     }
   else
     {
	Timer->DueTime.QuadPart = DueTime.QuadPart;
     }
   Timer->Period = Period;
   Timer->Header.SignalState = FALSE;
   if (Timer->TimerListEntry.Flink != NULL)
     {
	KeReleaseSpinLock(&TimerListLock, oldlvl);
	return(TRUE);
     }
   InsertTailList(&TimerListHead,&Timer->TimerListEntry);
   KeReleaseSpinLock(&TimerListLock, oldlvl);
   
   return(FALSE);
}

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
   
   KeRaiseIrql(HIGH_LEVEL, &oldlvl);
   KeAcquireSpinLockAtDpcLevel( &TimerListLock );
		     
   if (Timer->TimerListEntry.Flink == NULL)
     {
	KeReleaseSpinLock(&TimerListLock, oldlvl);
	return(FALSE);
     }
   RemoveEntryList(&Timer->TimerListEntry);
   Timer->TimerListEntry.Flink = Timer->TimerListEntry.Blink = NULL;
   KeReleaseSpinLock(&TimerListLock, oldlvl);

   return(TRUE);
}

BOOLEAN STDCALL
KeReadStateTimer (PKTIMER	Timer)
{
   return(Timer->Header.SignalState);
}

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

STATIC VOID 
HandleExpiredTimer(PKTIMER current)
{
   DPRINT("HandleExpiredTime(current %x)\n",current);
   if (current->Dpc != NULL)
     {
	DPRINT("current->Dpc %x current->Dpc->DeferredRoutine %x\n",
	       current->Dpc, current->Dpc->DeferredRoutine);
	KeInsertQueueDpc(current->Dpc,
			 NULL,
			 NULL);
	DPRINT("Finished dpc routine\n");
     }
   KeAcquireDispatcherDatabaseLock(FALSE);
   current->Header.SignalState = TRUE;
   KeDispatcherObjectWake(&current->Header);
   KeReleaseDispatcherDatabaseLock(FALSE);
   if (current->Period != 0)
     {
	current->DueTime.QuadPart += 
	  current->Period * SYSTEM_TIME_UNITS_PER_MSEC;
     }
   else
     {
	RemoveEntryList(&current->TimerListEntry);
	current->TimerListEntry.Flink = current->TimerListEntry.Blink = NULL;
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

   DPRINT("KeExpireTimers()\n");
   
   current_entry = TimerListHead.Flink;
   
   KeAcquireSpinLockAtDpcLevel(&TimerListLock);
   
   while (current_entry != &TimerListHead)
     {
       current = CONTAINING_RECORD(current_entry, KTIMER, TimerListEntry);
	
       current_entry = current_entry->Flink;
       
       if (system_time >= current->DueTime.QuadPart)
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
   system_time = system_time + CLOCK_INCREMENT;
   
   /*
    * Queue a DPC that will expire timers
    */
   KeInsertQueueDpc(&ExpireTimerDpc, (PVOID)Eip, 0);
}


VOID
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
   KeInitializeDpc(&ExpireTimerDpc, KeExpireTimers, 0);
   TimerInitDone = TRUE;
   /*
    * Calculate the starting time for the system clock
    */
   HalQueryRealTimeClock(&TimeFields);
   RtlTimeFieldsToTime(&TimeFields, &SystemBootTime);
   boot_time=SystemBootTime.QuadPart;
   system_time=boot_time;

   DPRINT("Finished KeInitializeTimerImpl()\n");
}
