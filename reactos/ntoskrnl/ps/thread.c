/* $Id: thread.c,v 1.62 2000/12/23 02:37:40 dwelch Exp $
 *
 * COPYRIGHT:              See COPYING in the top level directory
 * PROJECT:                ReactOS kernel
 * FILE:                   ntoskrnl/ps/thread.c
 * PURPOSE:                Thread managment
 * PROGRAMMER:             David Welch (welch@mcmail.com)
 * REVISION HISTORY: 
 *               23/06/98: Created
 *               12/10/99: Phillip Susi:  Thread priorities, and APC work
 */

/*
 * NOTE:
 * 
 * All of the routines that manipulate the thread queue synchronize on
 * a single spinlock
 * 
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/ob.h>
#include <internal/hal.h>
#include <internal/ps.h>
#include <internal/ob.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES *******************************************************************/

/* GLOBALS ******************************************************************/

POBJECT_TYPE EXPORTED PsThreadType = NULL;

KSPIN_LOCK PiThreadListLock;

/*
 * PURPOSE: List of threads associated with each priority level
 */
LIST_ENTRY PiThreadListHead;
static LIST_ENTRY PriorityListHead[MAXIMUM_PRIORITY];
static BOOLEAN DoneInitYet = FALSE;
ULONG PiNrThreads = 0;
ULONG PiNrRunnableThreads = 0;

static PETHREAD CurrentThread = NULL;

/* FUNCTIONS ***************************************************************/

PKTHREAD STDCALL KeGetCurrentThread(VOID)
{
   return(&(CurrentThread->Tcb));
}

PETHREAD STDCALL PsGetCurrentThread(VOID)
{
   return(CurrentThread);
}

HANDLE STDCALL PsGetCurrentThreadId(VOID)
{
   return(PsGetCurrentThread()->Cid.UniqueThread);
}

static VOID PsInsertIntoThreadList(KPRIORITY Priority, PETHREAD Thread)
{
//   DPRINT("PsInsertIntoThreadList(Priority %x, Thread %x)\n",Priority,
//	  Thread);
//   DPRINT("Offset %x\n", THREAD_PRIORITY_MAX + Priority);
   
   if (PiThreadListLock.Lock == 0)
     {
	KeBugCheck(0);
     }
   if (Priority >= MAXIMUM_PRIORITY || Priority < 0)
     {
	DPRINT1("Invalid thread priority\n");
	KeBugCheck(0);
     }
   InsertTailList(&PriorityListHead[Priority], &Thread->Tcb.QueueListEntry);
   PiNrRunnableThreads++;
}

VOID PsDumpThreads(VOID)
{
   PLIST_ENTRY current_entry;
   PETHREAD current;
   ULONG t;
   
//   return;
   
   current_entry = PiThreadListHead.Flink;
   t = 0;
   
   while (current_entry != &PiThreadListHead)
     {
	current = CONTAINING_RECORD(current_entry, ETHREAD, 
				    Tcb.ThreadListEntry);
	t++;
	if (t > PiNrThreads)
	  {
	     DbgPrint("Too many threads on list\n");
	     return;
	  }
	DbgPrint("current %x current->Tcb.State %d eip %x/%x ",
		current, current->Tcb.State,
		0, current->Tcb.LastEip);
//	KeDumpStackFrames((PVOID)current->Tcb.Context.esp0, 
//			  16);
	DbgPrint("PID %d ", current->ThreadsProcess->UniqueProcessId);
	DbgPrint("\n");
	
	current_entry = current_entry->Flink;
     }
}

static PETHREAD PsScanThreadList (KPRIORITY Priority)
{
   PLIST_ENTRY current_entry;
   PETHREAD current;
   
//   DPRINT("PsScanThreadList(Priority %d)\n",Priority);
   if (PiThreadListLock.Lock == 0)
     {
	KeBugCheck(0);
     }
   current_entry = RemoveHeadList(&PriorityListHead[Priority]);
   if (current_entry != &PriorityListHead[Priority])
     {	
	current = CONTAINING_RECORD(current_entry, ETHREAD, 
				    Tcb.QueueListEntry);
     }
   else
     {
	current = NULL;
     }
   
   return(current);
}


VOID PsDispatchThreadNoLock (ULONG NewThreadStatus)
{
   KPRIORITY CurrentPriority;
   PETHREAD Candidate;
   
   CurrentThread->Tcb.State = NewThreadStatus;
   if (CurrentThread->Tcb.State == THREAD_STATE_RUNNABLE)
     {
	PiNrRunnableThreads--;
	PsInsertIntoThreadList(CurrentThread->Tcb.Priority,
			       CurrentThread);
     }
   
   for (CurrentPriority = HIGH_PRIORITY;
	CurrentPriority >= LOW_PRIORITY;
	CurrentPriority--)
     {
	Candidate = PsScanThreadList(CurrentPriority);
	if (Candidate == CurrentThread)
	  {
	     KeReleaseSpinLockFromDpcLevel(&PiThreadListLock);
	     return;
	  }
	if (Candidate != NULL)
	  {	
	    PETHREAD OldThread;

	     DPRINT("Scheduling %x(%d)\n",Candidate, CurrentPriority);
	     
	     Candidate->Tcb.State = THREAD_STATE_RUNNING;
	    	     
	     OldThread = CurrentThread;
	     CurrentThread = Candidate;
	     
	     KeReleaseSpinLockFromDpcLevel(&PiThreadListLock);
	     //	     HalTaskSwitch(&CurrentThread->Tcb);
	     Ki386ContextSwitch(&CurrentThread->Tcb, &OldThread->Tcb);
	     PsReapThreads();
	     return;
	  }
     }
   DbgPrint("CRITICAL: No threads are runnable\n");
   KeBugCheck(0);
}

VOID PsDispatchThread(ULONG NewThreadStatus)
{
   KIRQL oldIrql;
   
   if (!DoneInitYet)
     {
	return;
     }   
   
   KeAcquireSpinLock(&PiThreadListLock, &oldIrql);  
   /*
    * Save wait IrqL
    */
   CurrentThread->Tcb.WaitIrql = oldIrql;
   PsDispatchThreadNoLock(NewThreadStatus);
   KeLowerIrql(oldIrql);
}

/*
 * Suspend and resume may only be called to suspend the current thread, except
 * by apc.c
 */

ULONG PsUnfreezeThread(PETHREAD Thread, PNTSTATUS WaitStatus)
{
   KIRQL oldIrql;
   ULONG r;
   
   DPRINT("PsUnfreezeThread(Thread %x, WaitStatus %x)\n", Thread, WaitStatus);
   
   KeAcquireSpinLock(&PiThreadListLock, &oldIrql);
   
   Thread->Tcb.FreezeCount--;
   r = Thread->Tcb.FreezeCount;
   
   if (WaitStatus != NULL)
     {
	Thread->Tcb.WaitStatus = *WaitStatus;
     }
   
   if (r <= 0)
     {
	DPRINT("Resuming thread\n");
	Thread->Tcb.State = THREAD_STATE_RUNNABLE;
	PsInsertIntoThreadList(Thread->Tcb.Priority, Thread);
     }
   
   KeReleaseSpinLock(&PiThreadListLock, oldIrql);
   return(r);
}

VOID PsUnfreezeOtherThread(PETHREAD Thread)
{
   ULONG r;
   
   Thread->Tcb.FreezeCount--;
   r = Thread->Tcb.FreezeCount;
   
   if (r <= 0)
     {
	Thread->Tcb.State = THREAD_STATE_RUNNABLE;
	PsInsertIntoThreadList(Thread->Tcb.Priority, Thread);
     }
}

ULONG PsFreezeThread(PETHREAD Thread, 
		     PNTSTATUS Status, 
		     UCHAR Alertable,
		     ULONG WaitMode)
{
   KIRQL oldIrql;
   ULONG r;
   
   KeAcquireSpinLock(&PiThreadListLock, &oldIrql);
   
   Thread->Tcb.FreezeCount++;
   r = Thread->Tcb.FreezeCount;
   
   DPRINT("r %d\n", r);
   
   if (r == 0)
     {
	DPRINT("Not suspending thread\n");
	KeReleaseSpinLock(&PiThreadListLock, oldIrql);
	return(r);
     }
   
   Thread->Tcb.Alertable = Alertable;
   Thread->Tcb.WaitMode = WaitMode;
   
   if (PsGetCurrentThread() != Thread)
     {
	DbgPrint("Suspending other\n");
	KeBugCheck(0);
     }
   else
     {
	DPRINT("Suspending self\n");
	Thread->Tcb.WaitIrql = oldIrql;
	PsDispatchThreadNoLock(THREAD_STATE_FROZEN);
	if (Status != NULL)
	  {
	     *Status = Thread->Tcb.WaitStatus;
	  }
	KeLowerIrql(oldIrql);
     }
   return(r);
}

ULONG PsResumeThread(PETHREAD Thread)
{
   KIRQL oldIrql;
   ULONG SuspendCount;

   KeAcquireSpinLock(&PiThreadListLock, &oldIrql);

   if (Thread->Tcb.SuspendCount > 0)
     {
	Thread->Tcb.SuspendCount--;
	SuspendCount = Thread->Tcb.SuspendCount;
	Thread->Tcb.State = THREAD_STATE_RUNNABLE;
	PsInsertIntoThreadList(Thread->Tcb.Priority, Thread);
     }

   KeReleaseSpinLock(&PiThreadListLock, oldIrql);

   return SuspendCount;
}

ULONG PsSuspendThread(PETHREAD Thread)
{
   KIRQL oldIrql;
   ULONG PreviousSuspendCount;

   KeAcquireSpinLock(&PiThreadListLock, &oldIrql);

   PreviousSuspendCount = Thread->Tcb.SuspendCount;
   if (Thread->Tcb.SuspendCount < MAXIMUM_SUSPEND_COUNT)
     {
	Thread->Tcb.SuspendCount++;
     }

   if (PsGetCurrentThread() == Thread)
     {
	DbgPrint("Cannot suspend self\n");
	KeBugCheck(0);
     }

   Thread->Tcb.State = THREAD_STATE_SUSPENDED;

   KeReleaseSpinLock(&PiThreadListLock, oldIrql);

   return PreviousSuspendCount;
}

VOID 
PsInitThreadManagment(VOID)
/*
 * FUNCTION: Initialize thread managment
 */
{
   PETHREAD FirstThread;
   ULONG i;
   HANDLE FirstThreadHandle;
   
   KeInitializeSpinLock(&PiThreadListLock);
   for (i=0; i < MAXIMUM_PRIORITY; i++)
     {
	InitializeListHead(&PriorityListHead[i]);
     }
   InitializeListHead(&PiThreadListHead);
   
   PsThreadType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
   
   RtlInitUnicodeString(&PsThreadType->TypeName, L"Thread");
   
   PsThreadType->TotalObjects = 0;
   PsThreadType->TotalHandles = 0;
   PsThreadType->MaxObjects = 0;
   PsThreadType->MaxHandles = 0;
   PsThreadType->PagedPoolCharge = 0;
   PsThreadType->NonpagedPoolCharge = sizeof(ETHREAD);
   PsThreadType->Dump = NULL;
   PsThreadType->Open = NULL;
   PsThreadType->Close = PiCloseThread;
   PsThreadType->Delete = PiDeleteThread;
   PsThreadType->Parse = NULL;
   PsThreadType->Security = NULL;
   PsThreadType->QueryName = NULL;
   PsThreadType->OkayToClose = NULL;
   PsThreadType->Create = NULL;
   
   PsInitializeThread(NULL,&FirstThread,&FirstThreadHandle,
		      THREAD_ALL_ACCESS,NULL, TRUE);
   HalInitFirstTask(FirstThread);
   FirstThread->Tcb.State = THREAD_STATE_RUNNING;
   FirstThread->Tcb.FreezeCount = 0;
   CurrentThread = FirstThread;
   CURRENT_KPCR->CurrentThread = (PVOID)FirstThread;
   NtClose(FirstThreadHandle);
   
   DPRINT("FirstThread %x\n",FirstThread);
      
   DoneInitYet = TRUE;
}


/*
 * Sets thread's base priority relative to the process' base priority
 * Should only be passed in THREAD_PRIORITY_ constants in pstypes.h
 */
LONG STDCALL
KeSetBasePriorityThread (PKTHREAD	Thread,
			 LONG		Increment)
{
   Thread->BasePriority = 
     ((PETHREAD)Thread)->ThreadsProcess->Pcb.BasePriority + Increment;
   if (Thread->BasePriority < LOW_PRIORITY)
     Thread->BasePriority = LOW_PRIORITY;
   else if (Thread->BasePriority >= MAXIMUM_PRIORITY)
	   Thread->BasePriority = HIGH_PRIORITY;
   Thread->Priority = Thread->BasePriority;
   return 1;
}


KPRIORITY STDCALL
KeSetPriorityThread (PKTHREAD Thread, KPRIORITY Priority)
{
   KPRIORITY OldPriority;
   KIRQL oldIrql;
   
   if (Priority < 0 || Priority >= MAXIMUM_PRIORITY)
     {
	KeBugCheck(0);
     }
   
   OldPriority = Thread->Priority;
   Thread->Priority = (CHAR)Priority;

   KeAcquireSpinLock(&PiThreadListLock, &oldIrql);
   if (Thread->State == THREAD_STATE_RUNNABLE)
    {
	RemoveEntryList(&Thread->QueueListEntry);
	PsInsertIntoThreadList(Thread->BasePriority, 
			       CONTAINING_RECORD(Thread,ETHREAD,Tcb));
     }
   KeReleaseSpinLock(&PiThreadListLock, oldIrql);
   return(OldPriority);
}


NTSTATUS STDCALL NtAlertResumeThread(IN	HANDLE ThreadHandle,
				     OUT PULONG	SuspendCount)
{
   UNIMPLEMENTED;
}


NTSTATUS STDCALL NtAlertThread (IN HANDLE ThreadHandle)
{
   PETHREAD Thread;
   NTSTATUS Status;
   NTSTATUS ThreadStatus;
   
   Status = ObReferenceObjectByHandle(ThreadHandle,
				      THREAD_SUSPEND_RESUME,
				      PsThreadType,
				      UserMode,
				      (PVOID*)&Thread,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   ThreadStatus = STATUS_ALERTED;
   (VOID)PsUnfreezeThread(Thread, &ThreadStatus);
   
   ObDereferenceObject(Thread);
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL 
NtOpenThread(OUT PHANDLE ThreadHandle,
	     IN	ACCESS_MASK DesiredAccess,
	     IN	POBJECT_ATTRIBUTES ObjectAttributes,
	     IN	PCLIENT_ID ClientId)
{
	UNIMPLEMENTED;
}

NTSTATUS STDCALL 
NtResumeThread (IN	HANDLE	ThreadHandle,
		IN	PULONG	SuspendCount)
/*
 * FUNCTION: Decrements a thread's resume count
 * ARGUMENTS: 
 *        ThreadHandle = Handle to the thread that should be resumed
 *        ResumeCount =  The resulting resume count.
 * REMARK:
 *        A thread is resumed if its suspend count is 0. This procedure maps to
 *        the win32 ResumeThread function. ( documentation about the the suspend
 *        count can be found here aswell )
 * RETURNS: Status
 */
{
   PETHREAD Thread;
   NTSTATUS Status;
   ULONG Count;

   Status = ObReferenceObjectByHandle(ThreadHandle,
				      THREAD_SUSPEND_RESUME,
				      PsThreadType,
				      UserMode,
				      (PVOID*)&Thread,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   Count = PsResumeThread(Thread);
   if (SuspendCount != NULL)
     {
	*SuspendCount = Count;
     }

   ObDereferenceObject((PVOID)Thread);

   return STATUS_SUCCESS;
}


NTSTATUS STDCALL 
NtSuspendThread (IN HANDLE ThreadHandle,
		 IN PULONG PreviousSuspendCount)
/*
 * FUNCTION: Increments a thread's suspend count
 * ARGUMENTS: 
 *        ThreadHandle = Handle to the thread that should be resumed
 *        PreviousSuspendCount =  The resulting/previous suspend count.
 * REMARK:
 *        A thread will be suspended if its suspend count is greater than 0. 
 *        This procedure maps to the win32 SuspendThread function. ( 
 *        documentation about the the suspend count can be found here aswell )
 *        The suspend count is not increased if it is greater than 
 *        MAXIMUM_SUSPEND_COUNT.
 * RETURNS: Status
 */ 
{
   PETHREAD Thread;
   NTSTATUS Status;
   ULONG Count;

   Status = ObReferenceObjectByHandle(ThreadHandle,
				      THREAD_SUSPEND_RESUME,
				      PsThreadType,
				      UserMode,
				      (PVOID*)&Thread,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   Count = PsSuspendThread(Thread);
   if (PreviousSuspendCount != NULL)
     {
	*PreviousSuspendCount = Count;
     }

   ObDereferenceObject((PVOID)Thread);

   return STATUS_SUCCESS;
}

NTSTATUS STDCALL
NtCallbackReturn (PVOID		Result,
		  ULONG		ResultLength,
		  NTSTATUS	Status)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL
NtW32Call (IN ULONG RoutineIndex,
	   IN PVOID Argument,
	   IN ULONG ArgumentLength,
	   OUT PVOID* Result OPTIONAL,
	   OUT PULONG ResultLength OPTIONAL)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL 
NtContinue(IN PCONTEXT	Context,
	   IN BOOLEAN TestAlert)
{
   PKTRAP_FRAME TrapFrame;
   
   /*
    * Copy the supplied context over the register information that was saved
    * on entry to kernel mode, it will then be restored on exit
    * FIXME: Validate the context
    */
   TrapFrame = KeGetCurrentThread()->TrapFrame;
   if (TrapFrame == NULL)
     {
	DbgPrint("NtContinue called but TrapFrame was NULL\n");
	KeBugCheck(0);
     }
   KeContextToTrapFrame(Context, TrapFrame);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL 
NtYieldExecution(VOID)
   PsDispatchThread(THREAD_STATE_RUNNABLE);
   return(STATUS_SUCCESS);
}


/* EOF */
