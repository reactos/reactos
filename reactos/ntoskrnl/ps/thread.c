/* $Id: thread.c,v 1.59 2000/10/22 16:36:53 ekohl Exp $
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
	DbgPrint("current %x current->Tcb.State %d eip %x ",
		current, current->Tcb.State,
		current->Tcb.Context.eip);
	KeDumpStackFrames((PVOID)current->Tcb.Context.esp0, 
			  16);
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
	     DPRINT("Scheduling %x(%d)\n",Candidate, CurrentPriority);
	     
	     Candidate->Tcb.State = THREAD_STATE_RUNNING;
	    	     
	     CurrentThread = Candidate;
	     
	     KeReleaseSpinLockFromDpcLevel(&PiThreadListLock);
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
   /*
    * Save wait IrqL
    */
   CurrentThread->Tcb.WaitIrql = oldIrql;
   PsDispatchThreadNoLock(NewThreadStatus);
   KeLowerIrql(oldIrql);
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

VOID PsInitThreadManagment(VOID)
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
		      THREAD_ALL_ACCESS,NULL);
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

NTSTATUS STDCALL NtOpenThread(OUT	PHANDLE ThreadHandle,
			      IN	ACCESS_MASK DesiredAccess,
			      IN	POBJECT_ATTRIBUTES ObjectAttributes,
			      IN	PCLIENT_ID ClientId)
{
	UNIMPLEMENTED;
}

VOID KeContextToTrapFrame(PCONTEXT Context,
			  PKTRAP_FRAME TrapFrame)
{
      if (Context->ContextFlags & CONTEXT_CONTROL)
     {
	TrapFrame->Esp = Context->Esp;
	TrapFrame->Ss = Context->SegSs;
	TrapFrame->Cs = Context->SegCs;
	TrapFrame->Eip = Context->Eip;
	TrapFrame->Eflags = Context->EFlags;	
	TrapFrame->Ebp = Context->Ebp;
     }
   if (Context->ContextFlags & CONTEXT_INTEGER)
     {
	TrapFrame->Eax = Context->Eax;
	TrapFrame->Ebx = Context->Ebx;
	TrapFrame->Ecx = Context->Ecx;
	/*
	 * Edx is used in the TrapFrame to hold the old trap frame pointer
	 * so we don't want to overwrite it here
	 */
/*	TrapFrame->Edx = Context->Edx; */
	TrapFrame->Esi = Context->Esi;
	TrapFrame->Edx = Context->Edi;
     }
   if (Context->ContextFlags & CONTEXT_SEGMENTS)
     {
	TrapFrame->Ds = Context->SegDs;
	TrapFrame->Es = Context->SegEs;
	TrapFrame->Fs = Context->SegFs;
	TrapFrame->Gs = Context->SegGs;
     }
   if (Context->ContextFlags & CONTEXT_FLOATING_POINT)
     {
	/*
	 * Not handled
	 */
     }
   if (Context->ContextFlags & CONTEXT_DEBUG_REGISTERS)
     {
	/*
	 * Not handled
	 */
     }
}

VOID KeTrapFrameToContext(PKTRAP_FRAME TrapFrame,
			  PCONTEXT Context)
{
   if (Context->ContextFlags & CONTEXT_CONTROL)
     {
	Context->SegSs = TrapFrame->Ss;
	Context->Esp = TrapFrame->Esp;
	Context->SegCs = TrapFrame->Cs;
	Context->Eip = TrapFrame->Eip;
	Context->EFlags = TrapFrame->Eflags;
	Context->Ebp = TrapFrame->Ebp;
     }
   if (Context->ContextFlags & CONTEXT_INTEGER)
     {
	Context->Eax = TrapFrame->Eax;
	Context->Ebx = TrapFrame->Ebx;
	Context->Ecx = TrapFrame->Ecx;
	/*
	 * NOTE: In the trap frame which is built on entry to a system
	 * call TrapFrame->Edx will actually hold the address of the
	 * previous TrapFrame. I don't believe leaking this information
	 * has security implications
	 */
	Context->Edx = TrapFrame->Edx;
	Context->Esi = TrapFrame->Esi;
	Context->Edi = TrapFrame->Edi;
     }
   if (Context->ContextFlags & CONTEXT_SEGMENTS)
     {
	Context->SegDs = TrapFrame->Ds;
	Context->SegEs = TrapFrame->Es;
	Context->SegFs = TrapFrame->Fs;
	Context->SegGs = TrapFrame->Gs;
     }
   if (Context->ContextFlags & CONTEXT_DEBUG_REGISTERS)
     {
	/*
	 * FIXME: Implement this case
	 */	
     }
   if (Context->ContextFlags & CONTEXT_FLOATING_POINT)
     {
	/*
	 * FIXME: Implement this case
	 */
     }
#if 0   
   if (Context->ContextFlags & CONTEXT_EXTENDED_REGISTERS)
     {
	/*
	 * FIXME: Investigate this
	 */
     }
#endif
}

VOID KeGetContextRundownRoutine(PKAPC Apc)
{
   PKEVENT Event;
   PNTSTATUS Status;
   
   Event = (PKEVENT)Apc->SystemArgument1;
   Status = (PNTSTATUS)Apc->SystemArgument2;
   (*Status) = STATUS_THREAD_IS_TERMINATING;
   KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
}

VOID KeGetContextKernelRoutine(PKAPC Apc,
			       PKNORMAL_ROUTINE* NormalRoutine,
			       PVOID* NormalContext,
			       PVOID* SystemArgument1,
			       PVOID* SystemArgument2)
{
   PKEVENT Event;
   PCONTEXT Context;
   PNTSTATUS Status;
   
   Context = (PCONTEXT)(*NormalContext);
   Event = (PKEVENT)(*SystemArgument1);
   Status = (PNTSTATUS)(*SystemArgument2);
   
   KeTrapFrameToContext(KeGetCurrentThread()->TrapFrame, Context);
   
   *Status = STATUS_SUCCESS;
   KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
}

NTSTATUS STDCALL NtGetContextThread (IN	HANDLE ThreadHandle, 
				     OUT PCONTEXT Context)
{
   PETHREAD Thread;
   NTSTATUS Status;
   
   Status = ObReferenceObjectByHandle(ThreadHandle,
				      THREAD_GET_CONTEXT,
				      PsThreadType,
				      UserMode,
				      (PVOID*)&Thread,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   if (Thread == PsGetCurrentThread())
     {
	/*
	 * I don't know if trying to get your own context makes much
	 * sense but we can handle it more efficently.
	 */
	
	KeTrapFrameToContext(Thread->Tcb.TrapFrame, Context);
	ObDereferenceObject(Thread);
	return(STATUS_SUCCESS);
     }
   else
     {
	KAPC Apc;
	KEVENT Event;
	NTSTATUS AStatus;
	CONTEXT KContext;
	
	KContext.ContextFlags = Context->ContextFlags;
	KeInitializeEvent(&Event,
			  NotificationEvent,
			  FALSE);	
	AStatus = STATUS_SUCCESS;
	
	KeInitializeApc(&Apc,
			&Thread->Tcb,
			0,
			KeGetContextKernelRoutine,
			KeGetContextRundownRoutine,
			NULL,
			KernelMode,
			(PVOID)&KContext);
	KeInsertQueueApc(&Apc,
			 (PVOID)&Event,
			 (PVOID)&AStatus,
			 0);
	Status = KeWaitForSingleObject(&Event,
				       0,
				       UserMode,
				       FALSE,
				       NULL);
	if (!NT_SUCCESS(Status))
	  {
	     return(Status);
	  }
	if (!NT_SUCCESS(AStatus))
	  {
	     return(AStatus);
	  }
	memcpy(Context, &KContext, sizeof(CONTEXT));
	return(STATUS_SUCCESS);
     }
}

NTSTATUS STDCALL NtSetContextThread (IN	HANDLE		ThreadHandle,
				     IN	PCONTEXT	Context)
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
			    IN BOOLEAN TestAlert)
{
   PKTRAP_FRAME TrapFrame;
   
   TrapFrame = KeGetCurrentThread()->TrapFrame;
   if (TrapFrame == NULL)
     {
	DbgPrint("NtContinue called but TrapFrame was NULL\n");
	KeBugCheck(0);
     }   
   KeContextToTrapFrame(Context, TrapFrame);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtYieldExecution(VOID)
{ 
   PsDispatchThread(THREAD_STATE_RUNNABLE);
   return(STATUS_SUCCESS);
}


/* EOF */
