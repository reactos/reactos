/* $Id: thread.c,v 1.42 1999/12/16 22:59:03 phreak Exp $
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
#include <string.h>
#include <internal/string.h>
#include <internal/hal.h>
#include <internal/ps.h>
#include <internal/ob.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES *******************************************************************/

/* GLOBALS ******************************************************************/

POBJECT_TYPE PsThreadType = NULL;

#define NR_THREAD_PRIORITY_LEVELS (32)
#define THREAD_PRIORITY_MAX (15)

KSPIN_LOCK PiThreadListLock;

/*
 * PURPOSE: List of threads associated with each priority level
 */
LIST_ENTRY PiThreadListHead;
static LIST_ENTRY PriorityListHead[NR_THREAD_PRIORITY_LEVELS];
static BOOLEAN DoneInitYet = FALSE;
ULONG PiNrThreads = 0;
ULONG PiNrRunnableThreads = 0;

static PETHREAD CurrentThread = NULL;

/* FUNCTIONS ***************************************************************/

PKTHREAD KeGetCurrentThread(VOID)
{
   return(&(CurrentThread->Tcb));
}

PETHREAD PsGetCurrentThread(VOID)
{
   return(CurrentThread);
}

HANDLE PsGetCurrentThreadId(VOID)
{
   return(CurrentThread->Cid.UniqueThread);
}

VOID PiTerminateProcessThreads(PEPROCESS Process, NTSTATUS ExitStatus)
{
   KIRQL oldlvl;
   PLIST_ENTRY current_entry;
   PETHREAD current;

   KeAcquireSpinLock(&PiThreadListLock, &oldlvl);

   current_entry = PiThreadListHead.Flink;
   while (current_entry != &PiThreadListHead)
     {
	current = CONTAINING_RECORD(current_entry,ETHREAD,Tcb.QueueListEntry);
	if (current->ThreadsProcess == Process &&
	    current != PsGetCurrentThread())
	  {
	     KeReleaseSpinLock(&PiThreadListLock, oldlvl);
	     PsTerminateOtherThread(current, ExitStatus);
	     KeAcquireSpinLock(&PiThreadListLock, &oldlvl);
	     current_entry = PiThreadListHead.Flink;
	  }
	current_entry = current_entry->Flink;
     }

   KeReleaseSpinLock(&PiThreadListLock, oldlvl);
}

static VOID PsInsertIntoThreadList(KPRIORITY Priority, PETHREAD Thread)
{
//   DPRINT("PsInsertIntoThreadList(Priority %x, Thread %x)\n",Priority,
//	  Thread);
//   DPRINT("Offset %x\n", THREAD_PRIORITY_MAX + Priority);
   
   InsertTailList(&PriorityListHead[THREAD_PRIORITY_MAX+Priority],
		  &Thread->Tcb.QueueListEntry);
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
	if (t >= PiNrThreads)
	  {
	     DbgPrint("Too many threads on list\n");
	     return;
	  }
	DPRINT1("current %x current->Tcb.State %d eip %x ",
		current, current->Tcb.State,
		current->Tcb.Context.eip);
//	KeDumpStackFrames(0, 16);
	DPRINT1("PID %d ", current->ThreadsProcess->UniqueProcessId);
	DPRINT1("\n");
	
	current_entry = current_entry->Flink;
     }
}

VOID PsReapThreads(VOID)
{
   PLIST_ENTRY current_entry;
   PETHREAD current;
   KIRQL oldIrql;
   
//   DPRINT1("PsReapThreads()\n");
   
   KeAcquireSpinLock(&PiThreadListLock, &oldIrql);
   
   current_entry = PiThreadListHead.Flink;
   
   while (current_entry != &PiThreadListHead)
     {
	current = CONTAINING_RECORD(current_entry, ETHREAD, 
				    Tcb.ThreadListEntry);
	
	current_entry = current_entry->Flink;
	
	if (current->Tcb.State == THREAD_STATE_TERMINATED_1)
	  {
	     DPRINT("Reaping thread %x\n", current);
	     current->Tcb.State = THREAD_STATE_TERMINATED_2;
	     KeReleaseSpinLock(&PiThreadListLock, oldIrql);
	     ObDereferenceObject(current);
	     KeAcquireSpinLock(&PiThreadListLock, &oldIrql);
	     current_entry = PiThreadListHead.Flink;
	  }
     }
   KeReleaseSpinLock(&PiThreadListLock, oldIrql);
}

static PETHREAD PsScanThreadList (KPRIORITY Priority)
{
   PLIST_ENTRY current_entry;
   PETHREAD current;
   
//   DPRINT("PsScanThreadList(Priority %d)\n",Priority);
   
   current_entry = RemoveHeadList(
			    &PriorityListHead[THREAD_PRIORITY_MAX + Priority]);
   if (current_entry != &PriorityListHead[THREAD_PRIORITY_MAX + Priority])
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
   PiNrRunnableThreads--;
   if (CurrentThread->Tcb.State == THREAD_STATE_RUNNABLE)
     {
	PsInsertIntoThreadList(CurrentThread->Tcb.Priority,
			       CurrentThread);
     }
   
   for (CurrentPriority = THREAD_PRIORITY_TIME_CRITICAL; 
	(CurrentPriority >= THREAD_PRIORITY_IDLE);
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
	     DPRINT("Scheduling %x(%d)\n",Candidate, CurrentPriority);
	     
	     Candidate->Tcb.State = THREAD_STATE_RUNNING;
	    	     
	     CurrentThread = Candidate;
	     
	     KeReleaseSpinLockFromDpcLevel( &PiThreadListLock );
	     HalTaskSwitch(&CurrentThread->Tcb);
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
   CurrentThread->Tcb.WaitIrql = oldIrql;		// save wait Irql
   PsDispatchThreadNoLock(NewThreadStatus);
   KeLowerIrql(oldIrql);
//   DPRINT("oldIrql %d\n",oldIrql);
}

/*
 * Suspend and resume may only be called to suspend the current thread, except by apc.c
 */

ULONG PsUnfreezeThread(PETHREAD Thread, PNTSTATUS WaitStatus)
{
   KIRQL oldIrql;
   ULONG r;
   
   DPRINT("PsUnfreezeThread(Thread %x, WaitStatus %x)\n", Thread, WaitStatus);
   
   KeAcquireSpinLock(&PiThreadListLock, &oldIrql);
   
   Thread->Tcb.FreezeCount--;
   r = Thread->Tcb.FreezeCount;
   
   if (r <= 0)
     {
	DPRINT("Resuming thread\n");
	Thread->Tcb.State = THREAD_STATE_RUNNABLE;
	if (WaitStatus != NULL)
	  {
	     Thread->Tcb.WaitStatus = *WaitStatus;
	  }
	PsInsertIntoThreadList(Thread->Tcb.Priority, Thread);
     }
   
   KeReleaseSpinLock(&PiThreadListLock, oldIrql);
   return(r);
}

ULONG PsFreezeThread(PETHREAD Thread, PNTSTATUS Status, UCHAR Alertable,
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
	DPRINT("Suspending other\n");
	if (Thread->Tcb.State == THREAD_STATE_RUNNABLE)
	  {
	     RemoveEntryList(&Thread->Tcb.QueueListEntry);
	  }
	Thread->Tcb.State = THREAD_STATE_FROZEN;
	KeReleaseSpinLock(&PiThreadListLock, oldIrql);
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

VOID PsInitThreadManagment(VOID)
/*
 * FUNCTION: Initialize thread managment
 */
{
   PETHREAD FirstThread;
   ULONG i;
   ANSI_STRING AnsiString;
   HANDLE FirstThreadHandle;
   
   KeInitializeSpinLock(&PiThreadListLock);
   for (i=0; i<NR_THREAD_PRIORITY_LEVELS; i++)
     {
	InitializeListHead(&PriorityListHead[i]);
     }
   InitializeListHead(&PiThreadListHead);
   
   PsThreadType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
   
   RtlInitAnsiString(&AnsiString,"Thread");
   RtlAnsiStringToUnicodeString(&PsThreadType->TypeName,&AnsiString,TRUE);
   
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
		      THREAD_ALL_ACCESS,NULL);
   HalInitFirstTask(FirstThread);
   FirstThread->Tcb.State = THREAD_STATE_RUNNING;
   FirstThread->Tcb.FreezeCount = 0;
   ZwClose(FirstThreadHandle);
   
   DPRINT("FirstThread %x\n",FirstThread);
   
   CurrentThread = FirstThread;
   
   DoneInitYet = TRUE;
}


/*
 * Sets thread's base priority relative to the process' base priority
 * Should only be passed in THREAD_PRIORITY_ constants in pstypes.h
 */
LONG KeSetBasePriorityThread(PKTHREAD Thread, LONG Increment)
{
   Thread->BasePriority = ((PETHREAD)Thread)->ThreadsProcess->Pcb.BasePriority + Increment;
   if( Thread->BasePriority < 0 )
	   Thread->BasePriority = 0;
   else if( Thread->BasePriority >= NR_THREAD_PRIORITY_LEVELS )
	   Thread->BasePriority = NR_THREAD_PRIORITY_LEVELS - 1;
   Thread->Priority = Thread->BasePriority;
   return 1;
}


KPRIORITY KeSetPriorityThread(PKTHREAD Thread, KPRIORITY Priority)
{
   KPRIORITY OldPriority;
   KIRQL oldIrql;
   
   OldPriority = Thread->Priority;
   Thread->Priority = Priority;

   KeAcquireSpinLock(&PiThreadListLock, &oldIrql);
   RemoveEntryList(&Thread->QueueListEntry);
   PsInsertIntoThreadList(Thread->BasePriority,
			  CONTAINING_RECORD(Thread,ETHREAD,Tcb));
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


NTSTATUS STDCALL NtGetContextThread (IN	HANDLE		ThreadHandle, 
				     OUT	PCONTEXT	Context)
{
	UNIMPLEMENTED;
}


NTSTATUS STDCALL NtOpenThread(OUT	PHANDLE ThreadHandle,
			      IN	ACCESS_MASK DesiredAccess,
			      IN	POBJECT_ATTRIBUTES ObjectAttributes,
			      IN	PCLIENT_ID ClientId)
{
	UNIMPLEMENTED;
}


NTSTATUS STDCALL NtResumeThread (IN	HANDLE	ThreadHandle,
				 IN	PULONG	SuspendCount)
/*
 * FUNCTION: Decrements a thread's resume count
 * ARGUMENTS: 
 *        ThreadHandle = Handle to the thread that should be resumed
 *        ResumeCount =  The resulting resume count.
 * REMARK:
 *	  A thread is resumed if its suspend count is 0. This procedure maps to
 *        the win32 ResumeThread function. ( documentation about the the suspend count can be found here aswell )
 * RETURNS: Status
 */
{
   UNIMPLEMENTED;
   return(STATUS_UNSUCCESSFUL);
}


NTSTATUS STDCALL NtSetContextThread (IN	HANDLE		ThreadHandle,
				     IN	PCONTEXT	Context)
{
   UNIMPLEMENTED;
}


NTSTATUS STDCALL NtSuspendThread (IN HANDLE ThreadHandle,
				  IN PULONG PreviousSuspendCount)
/*
 * FUNCTION: Increments a thread's suspend count
 * ARGUMENTS: 
 *        ThreadHandle = Handle to the thread that should be resumed
 *        PreviousSuspendCount =  The resulting/previous suspend count.
 * REMARK:
 *	  A thread will be suspended if its suspend count is greater than 0. 
 *        This procedure maps to the win32 SuspendThread function. ( 
 *        documentation about the the suspend count can be found here aswell )
 *        The suspend count is not increased if it is greater than 
 *        MAXIMUM_SUSPEND_COUNT.
 * RETURNS: Status
 */ 
{
   UNIMPLEMENTED;
   return(STATUS_UNSUCCESSFUL);
}


NTSTATUS STDCALL NtContinue(IN PCONTEXT	Context,
			    IN CINT IrqLevel)
{
   UNIMPLEMENTED;
}


NTSTATUS STDCALL NtYieldExecution(VOID)
{ 
   PsDispatchThread(THREAD_STATE_RUNNABLE);
   return(STATUS_SUCCESS);
}


/* EOF */
