/* $Id: timer.c,v 1.33 2000/07/19 14:23:37 dwelch Exp $
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
#include <string.h>
#include <internal/string.h>
#include <stdio.h>
#include <internal/ke.h>
#include <internal/id.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES *****************************************************************/

#define TIMER_IRQ 0

/* GLOBALS ****************************************************************/

#define IDMAP_BASE         (0xd0000000)

/*
 * Return a linear address which can be used to access the physical memory
 * starting at x 
 */
extern inline unsigned int physical_to_linear(unsigned int x)
{
        return(x+IDMAP_BASE);
}

extern inline unsigned int linear_to_physical(unsigned int x)
{
        return(x-IDMAP_BASE);
}

/*
 * Current time
 */
static unsigned long long boot_time = 0;
static unsigned long long system_time = 0;

/*
 * Number of timer interrupts since initialisation
 */
volatile ULONGLONG KiTimerTicks;

/*
 * The increment in the system clock every timer tick (in system time units)
 * 
 * = (1/18.2)*10^9 
 * 
 * RJJ was 54945055
 */
#define CLOCK_INCREMENT (549255)

/*
 * PURPOSE: List of timers
 */
static LIST_ENTRY TimerListHead;
static KSPIN_LOCK TimerListLock;

/* must raise IRQL to HIGH_LEVEL and grab spin lock there, to sync with ISR */

extern ULONG PiNrRunnableThreads;

#define MICROSECONDS_PER_TICK (54945)
#define TICKS_TO_CALIBRATE (1)
#define CALIBRATE_PERIOD (MICROSECONDS_PER_TICK * TICKS_TO_CALIBRATE)
#define SYSTEM_TIME_UNITS_PER_MSEC (10000)

static BOOLEAN TimerInitDone = FALSE;

/* FUNCTIONS **************************************************************/


NTSTATUS STDCALL NtQueryTimerResolution(OUT	PULONG	MinimumResolution,
					OUT	PULONG	MaximumResolution, 
					OUT	PULONG	ActualResolution)
{
	UNIMPLEMENTED;
}


NTSTATUS STDCALL NtSetTimerResolution(IN	ULONG	RequestedResolution,
				      IN	BOOL	SetOrUnset,
				      OUT	PULONG	ActualResolution)
{
	UNIMPLEMENTED;
}


NTSTATUS STDCALL NtQueryPerformanceCounter (IN	PLARGE_INTEGER	Counter,
					    IN	PLARGE_INTEGER	Frequency)
{
	UNIMPLEMENTED;
}


NTSTATUS KeAddThreadTimeout(PKTHREAD Thread, PLARGE_INTEGER Interval)
{
   assert(Thread != NULL);
   assert(Interval != NULL);

   DPRINT("KeAddThreadTimeout(Thread %x, Interval %x)\n",Thread,Interval);
   
   KeInitializeTimer(&(Thread->Timer));
   KeSetTimer( &(Thread->Timer), *Interval, &Thread->TimerDpc );

   DPRINT("Thread->Timer.entry.Flink %x\n",
	    Thread->Timer.TimerListEntry.Flink);
   
   return STATUS_SUCCESS;
}


NTSTATUS STDCALL NtDelayExecution(IN ULONG Alertable,
				  IN TIME* Interval)
{
   NTSTATUS Status;
   LARGE_INTEGER Timeout;
   
   Timeout = *((PLARGE_INTEGER)Interval);
   Timeout.QuadPart = -Timeout.QuadPart;
   DPRINT("NtDelayExecution(Alertable %d, Internal %x) IntervalP %x\n",
	  Alertable, Internal, Timeout);
   
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
   PKTHREAD CurrentThread = KeGetCurrentThread();
   KeAddThreadTimeout(CurrentThread, Interval);
   return (KeWaitForSingleObject(&(CurrentThread->Timer),
				 Executive,
				 KernelMode,
				 Alertable,
				 NULL));
}

ULONG STDCALL
KeQueryTimeIncrement (VOID)
/*
 * FUNCTION: Gets the increment (in 100-nanosecond units) that is added to 
 * the system clock every time the clock interrupts
 * RETURNS: The increment
 */
{
   return(CLOCK_INCREMENT);
}

VOID STDCALL
KeQuerySystemTime (PLARGE_INTEGER	CurrentTime)
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
	if ( UpTime == NULL )
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
   KeRaiseIrql( HIGH_LEVEL, &oldlvl );
   KeAcquireSpinLockAtDpcLevel(&TimerListLock);
   
   Timer->Dpc = Dpc;
   if (DueTime.QuadPart < 0)
     {
	Timer->DueTime.QuadPart = system_time + 100000000;
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
   
   return FALSE;
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
   
   KeRaiseIrql( HIGH_LEVEL, &oldlvl );
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
   KeInitializeTimerEx(Timer,NotificationTimer);
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
KeQueryTickCount (PLARGE_INTEGER	TickCount)
/*
 * FUNCTION: Returns the number of ticks since the system was booted
 * ARGUMENTS:
 *         TickCount (OUT) = Points to storage for the number of ticks
 */
{
  TickCount->QuadPart = KiTimerTicks;
}

static void HandleExpiredTimer(PKTIMER current)
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
   current->Header.SignalState = TRUE;
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

VOID KeExpireTimers(VOID)
{
   PLIST_ENTRY current_entry = NULL;
   PKTIMER current = NULL;
   KIRQL oldlvl;

   DPRINT("KeExpireTimers()\n");
   
   current_entry = TimerListHead.Flink;
   
//   DPRINT("&TimerListHead %x\n",&TimerListHead);
//   DPRINT("current_entry %x\n",current_entry);
//   DPRINT("current_entry->Flink %x\n",current_entry->Flink);
//   DPRINT("current_entry->Flink->Flink %x\n",current_entry->Flink->Flink);
       
   KeRaiseIrql(HIGH_LEVEL, &oldlvl);
   KeAcquireSpinLockAtDpcLevel(&TimerListLock);
   
   while (current_entry!=(&TimerListHead))
     {
	current = CONTAINING_RECORD(current_entry, KTIMER, TimerListEntry);
	
	current_entry = current_entry->Flink;
	
	if (system_time >= current->DueTime.QuadPart)
	  {
	     HandleExpiredTimer(current);
	  }      
     }
   
   KeReleaseSpinLock(&TimerListLock, oldlvl);
//   DPRINT("Finished KeExpireTimers()\n");
}


VOID KiUpdateSystemTime (VOID)
/*
 * FUNCTION: Handles a timer interrupt
 */
{
   char str[36];
   char* vidmem=(char *)physical_to_linear(0xb8000 + 160 - 36);
   int i;
   int x,y;
//   extern ULONG EiNrUsedBlocks;
   extern unsigned int EiFreeNonPagedPool;
   extern unsigned int EiUsedNonPagedPool;
   //   extern ULONG PiNrThreads;
//   extern ULONG MiNrFreePages;
   
   if (TimerInitDone == FALSE)
     {
	return;
     }
   /*
    * Increment the number of timers ticks 
    */
   KiTimerTicks++;
   system_time = system_time + CLOCK_INCREMENT;
   
   /*
    * Display the tick count in the top left of the screen as a debugging
    * aid
    */
//   sprintf(str,"%.8u %.8u",nr_used_blocks,ticks);
   if ((EiFreeNonPagedPool + EiUsedNonPagedPool) == 0)
     {
	x = y = 0;
     }
   else
     {
	x = (EiFreeNonPagedPool * 100) / 
	  (EiFreeNonPagedPool + EiUsedNonPagedPool);
	y = (EiUsedNonPagedPool * 100) / 
	  (EiFreeNonPagedPool + EiUsedNonPagedPool);
     }
   memset(str, 0, sizeof(str));
//   sprintf(str,"%.8u %.8u",(unsigned int)EiNrUsedBlocks,
//	   (unsigned int)EiFreeNonPagedPool);
//   sprintf(str,"%.8u %.8u",EiFreeNonPagedPool,EiUsedNonPagedPool);
//   sprintf(str,"%.8u %.8u",(unsigned int)PiNrRunnableThreads,
//	   (unsigned int)PiNrThreads);
//   sprintf(str,"%.8u %.8u", (unsigned int)PiNrRunnableThreads,
//	   (unsigned int)MiNrFreePages);
   sprintf(str,"%.8u %.8u",EiFreeNonPagedPool,(unsigned int)KiTimerTicks);

   for (i=0;i<17;i++)
     {
	*vidmem=str[i];
	vidmem++;
	*vidmem=0x7;
	vidmem++;
     }
   KeExpireTimers();
}


VOID KeInitializeTimerImpl(VOID)
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
