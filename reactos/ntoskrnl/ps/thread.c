/*
 * COPYRIGHT:              See COPYING in the top level directory
 * PROJECT:                ReactOS kernel
 * FILE:                   ntoskrnl/ps/thread.c
 * PURPOSE:                Thread managment
 * PROGRAMMER:             David Welch (welch@mcmail.com)
 * REVISION HISTORY: 
 *               23/06/98: Created
 */

/*
 * NOTE:
 * 
 * All of the routines that manipulate the thread queue synchronize on
 * a single spinlock
 * 
 */

/* INCLUDES ****************************************************************/

#include <windows.h>
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

#define NR_THREAD_PRIORITY_LEVELS (31)
#define THREAD_PRIORITY_MAX (15)

static KSPIN_LOCK ThreadListLock = {0,};

/*
 * PURPOSE: List of threads associated with each priority level
 */
static LIST_ENTRY PriorityListHead[NR_THREAD_PRIORITY_LEVELS]={{NULL,NULL},};
static BOOLEAN DoneInitYet = FALSE;
ULONG PiNrThreads = 0;
ULONG PiNrRunnableThreads = 0;

static PETHREAD CurrentThread = NULL;

static ULONG PiNextThreadUniqueId = 0;

/* FUNCTIONS ***************************************************************/

PKTHREAD KeGetCurrentThread(VOID)
{
   return(&(CurrentThread->Tcb));
}

PETHREAD PsGetCurrentThread(VOID)
{
   return(CurrentThread);
}

VOID PiTerminateProcessThreads(PEPROCESS Process, NTSTATUS ExitStatus)
{
   KIRQL oldlvl;
   PLIST_ENTRY current_entry;
   PETHREAD current;
   ULONG i;

   KeAcquireSpinLock(&ThreadListLock, &oldlvl);

   for (i=0; i<NR_THREAD_PRIORITY_LEVELS; i++)
   {
        current_entry = PriorityListHead[i].Flink;
        while (current_entry != &PriorityListHead[i])
        {
             current = CONTAINING_RECORD(current_entry,ETHREAD,Tcb.Entry);
             if (current->ThreadsProcess == Process &&
                 current != PsGetCurrentThread())
             {
                  PsTerminateOtherThread(current, ExitStatus);
             }
             current_entry = current_entry->Flink;
        }
   }

   KeReleaseSpinLock(&ThreadListLock, oldlvl);
}

static VOID PsInsertIntoThreadList(KPRIORITY Priority, PETHREAD Thread)
{
   KIRQL oldlvl;
   
   DPRINT("PsInsertIntoThreadList(Priority %x, Thread %x)\n",Priority,
	  Thread);
   DPRINT("Offset %x\n", THREAD_PRIORITY_MAX + Priority);
   
   KeAcquireSpinLock(&ThreadListLock,&oldlvl);
   InsertTailList(&PriorityListHead[THREAD_PRIORITY_MAX+Priority],
		  &Thread->Tcb.Entry);
   KeReleaseSpinLock(&ThreadListLock,oldlvl);
}

VOID PsBeginThread(PKSTART_ROUTINE StartRoutine, PVOID StartContext)
{
   NTSTATUS Ret;
   
//   KeReleaseSpinLock(&ThreadListLock,PASSIVE_LEVEL);
   Ret = StartRoutine(StartContext);
   PsTerminateSystemThread(Ret);
   KeBugCheck(0);
}

static PETHREAD PsScanThreadList(KPRIORITY Priority)
{
   PLIST_ENTRY current_entry;
   PETHREAD current;
   PETHREAD oldest = NULL;
   ULONG oldest_time = 0;
   
//   DPRINT("PsScanThreadList(Priority %d)\n",Priority);
   
   current_entry = PriorityListHead[THREAD_PRIORITY_MAX+Priority].Flink;
   while (current_entry != &PriorityListHead[THREAD_PRIORITY_MAX+Priority])
     {
	current = CONTAINING_RECORD(current_entry,ETHREAD,Tcb.Entry);

	if (current->Tcb.State == THREAD_STATE_TERMINATED &&
	    current != CurrentThread)
	  {
	     PsReleaseThread(current);
	  }

	if (current->Tcb.State == THREAD_STATE_RUNNABLE)
	  {
	     if (oldest == NULL || oldest_time > current->Tcb.LastTick)
	       {
		  oldest = current;
		  oldest_time = current->Tcb.LastTick;
	       }
	  }
	current_entry = current_entry->Flink;
     }
//   DPRINT("PsScanThreadList() = %x\n",oldest);
   return(oldest);
}

VOID PsDispatchThread(VOID)
{
   KPRIORITY CurrentPriority;
   PETHREAD Candidate;
   KIRQL irql;
   LARGE_INTEGER TickCount;
   
   if (!DoneInitYet)
     {
	return;
     }
   
   KeAcquireSpinLock(&ThreadListLock, &irql);
   
   DPRINT("PsDispatchThread() Current %x\n",CurrentThread);
      
   if (CurrentThread->Tcb.State==THREAD_STATE_RUNNING)     
     {
	CurrentThread->Tcb.State=THREAD_STATE_RUNNABLE;
     }
   
   for (CurrentPriority=THREAD_PRIORITY_TIME_CRITICAL; 
	CurrentPriority>=THREAD_PRIORITY_IDLE;
	CurrentPriority--)
     {
	Candidate = PsScanThreadList(CurrentPriority);
	if (Candidate == CurrentThread)
	  {
             DPRINT("Scheduling current thread\n");
             KeQueryTickCount(&TickCount);
             CurrentThread->Tcb.LastTick = TickCount.u.LowPart;
	     CurrentThread->Tcb.State = THREAD_STATE_RUNNING;
	     KeReleaseSpinLock(&ThreadListLock,irql);
	     return;
	  }
	if (Candidate != NULL)
	  {	
             DPRINT("Scheduling %x\n",Candidate);
	     
	     Candidate->Tcb.State = THREAD_STATE_RUNNING;
	     
	     KeQueryTickCount(&TickCount);
             CurrentThread->Tcb.LastTick = TickCount.u.LowPart;
	     
	     CurrentThread = Candidate;
	     
	     HalTaskSwitch(&CurrentThread->Tcb);
	     KeReleaseSpinLock(&ThreadListLock, irql);
	     return;
	  }
     }
   DbgPrint("CRITICAL: No threads are runnable\n");
   KeBugCheck(0);
}

NTSTATUS PsInitializeThread(HANDLE ProcessHandle, 
			    PETHREAD* ThreadPtr,
			    PHANDLE ThreadHandle,
			    ACCESS_MASK DesiredAccess,
			    POBJECT_ATTRIBUTES ThreadAttributes)
{
   PETHREAD Thread;
   NTSTATUS Status;
   
   PiNrThreads++;
   
   Thread = ObCreateObject(ThreadHandle,
			   DesiredAccess,
			   ThreadAttributes,
			   PsThreadType);
   DPRINT("Thread = %x\n",Thread);
   Thread->Tcb.LastTick = 0;
   Thread->Tcb.State = THREAD_STATE_SUSPENDED;
   Thread->Tcb.BasePriority = THREAD_PRIORITY_NORMAL;
   Thread->Tcb.SuspendCount = 1;
   InitializeListHead(&Thread->Tcb.ApcState.ApcListHead[0]);
   InitializeListHead(&Thread->Tcb.ApcState.ApcListHead[1]);
   Thread->Tcb.KernelApcDisable = 1;
   
   if (ProcessHandle != NULL)
     {
	Status = ObReferenceObjectByHandle(ProcessHandle,
					   PROCESS_CREATE_THREAD,
					   PsProcessType,
					   UserMode,
					   (PVOID*)&Thread->ThreadsProcess,
					   NULL);
	if (Status != STATUS_SUCCESS)
	  {
	     DPRINT("Failed at %s:%d\n",__FILE__,__LINE__);
	     return(Status);
	  }
     }
   else
     {
	Thread->ThreadsProcess = SystemProcess;
	ObReferenceObjectByPointer(Thread->ThreadsProcess,
				   PROCESS_CREATE_THREAD,
				   PsProcessType,
				   UserMode);
     }
   ObReferenceObjectByPointer(Thread->ThreadsProcess,
			      PROCESS_CREATE_THREAD,
			      PsProcessType,
			      UserMode);
   InitializeListHead(&(Thread->IrpList));
   Thread->Cid.UniqueThread = (HANDLE)InterlockedIncrement(
					      &PiNextThreadUniqueId);
   Thread->Cid.UniqueProcess = (HANDLE)Thread->ThreadsProcess->UniqueProcessId;
   DPRINT("Thread->Cid.UniqueThread %d\n",Thread->Cid.UniqueThread);
   ObReferenceObjectByPointer(Thread,
			      THREAD_ALL_ACCESS,
			      PsThreadType,
			      UserMode);
   PsInsertIntoThreadList(Thread->Tcb.BasePriority,Thread);
   
   *ThreadPtr = Thread;
   
   ObDereferenceObject(Thread->ThreadsProcess);
   return(STATUS_SUCCESS);
}

VOID PsResumeThread(PETHREAD Thread)
{
   DPRINT("PsResumeThread(Thread %x)\n",Thread);
   Thread->Tcb.SuspendCount--;
   if (Thread->Tcb.SuspendCount <= 0 &&
       Thread->Tcb.State != THREAD_STATE_RUNNING)
     {
        DPRINT("Setting thread to runnable\n");
	Thread->Tcb.State = THREAD_STATE_RUNNABLE;
     }
   DPRINT("Finished PsResumeThread()\n");
}

VOID PsSuspendThread(PETHREAD Thread)
{
   DPRINT("PsSuspendThread(Thread %x)\n",Thread);
   Thread->Tcb.SuspendCount++;
   if (Thread->Tcb.SuspendCount > 0)
     {
	Thread->Tcb.State = THREAD_STATE_SUSPENDED;
	if (Thread == CurrentThread)
	  {
	     PsDispatchThread();
	  }
     }
}

VOID PiDeleteThread(PVOID ObjectBody)
{
   DbgPrint("PiDeleteThread(ObjectBody %x)\n",ObjectBody);
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
   
   KeInitializeSpinLock(&ThreadListLock);
   for (i=0; i<NR_THREAD_PRIORITY_LEVELS; i++)
     {
	InitializeListHead(&PriorityListHead[i]);
     }
   
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
   PsThreadType->Close = NULL;
   PsThreadType->Delete = PiDeleteThread;
   PsThreadType->Parse = NULL;
   PsThreadType->Security = NULL;
   PsThreadType->QueryName = NULL;
   PsThreadType->OkayToClose = NULL;
   
   PsInitializeThread(NULL,&FirstThread,&FirstThreadHandle,
		      THREAD_ALL_ACCESS,NULL);
   HalInitFirstTask(FirstThread);
   FirstThread->Tcb.State = THREAD_STATE_RUNNING;
   FirstThread->Tcb.SuspendCount = 0;

   DPRINT("FirstThread %x\n",FirstThread);
   
   CurrentThread = FirstThread;
   
   DoneInitYet = TRUE;
}

NTSTATUS
NtCreateThread (
	PHANDLE			ThreadHandle,
	ACCESS_MASK		DesiredAccess,
	POBJECT_ATTRIBUTES	ObjectAttributes,
	HANDLE			ProcessHandle,
	PCLIENT_ID		Client,
	PCONTEXT		ThreadContext,
	PINITIAL_TEB		InitialTeb,
	BOOLEAN			CreateSuspended
	)
{
   PETHREAD Thread;
   NTSTATUS Status;
   
   DPRINT("NtCreateThread(ThreadHandle %x, PCONTEXT %x)\n",
	  ThreadHandle,ThreadContext);
   
   Status = PsInitializeThread(ProcessHandle,&Thread,ThreadHandle,
			       DesiredAccess,ObjectAttributes);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   Status = HalInitTaskWithContext(Thread,ThreadContext);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   Thread->StartAddress=NULL;

   if (Client!=NULL)
     {
	*Client=Thread->Cid;
     }  
   
   if (!CreateSuspended)
     {
        DPRINT("Not creating suspended\n");
	PsResumeThread(Thread);
     }
   DPRINT("Finished PsCreateThread()\n");
   return(STATUS_SUCCESS);
}

NTSTATUS PsCreateSystemThread(PHANDLE ThreadHandle,
			      ACCESS_MASK DesiredAccess,
			      POBJECT_ATTRIBUTES ObjectAttributes,
			      HANDLE ProcessHandle,
			      PCLIENT_ID ClientId,
			      PKSTART_ROUTINE StartRoutine,
			      PVOID StartContext)
/*
 * FUNCTION: Creates a thread which executes in kernel mode
 * ARGUMENTS:
 *       ThreadHandle (OUT) = Caller supplied storage for the returned thread 
 *                            handle
 *       DesiredAccess = Requested access to the thread
 *       ObjectAttributes = Object attributes (optional)
 *       ProcessHandle = Handle of process thread will run in
 *                       NULL to use system process
 *       ClientId (OUT) = Caller supplied storage for the returned client id
 *                        of the thread (optional)
 *       StartRoutine = Entry point for the thread
 *       StartContext = Argument supplied to the thread when it begins
 *                     execution
 * RETURNS: Success or failure status
 */
{
   PETHREAD Thread;
   NTSTATUS Status;
   
   DPRINT("PsCreateSystemThread(ThreadHandle %x, ProcessHandle %x)\n",
	    ThreadHandle,ProcessHandle);
   
   Status = PsInitializeThread(ProcessHandle,&Thread,ThreadHandle,
			       DesiredAccess,ObjectAttributes);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   Thread->StartAddress=StartRoutine;
   Status = HalInitTask(Thread,StartRoutine,StartContext);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   if (ClientId!=NULL)
     {
	*ClientId=Thread->Cid;
     }  

   PsResumeThread(Thread);
   
   return(STATUS_SUCCESS);
}

LONG KeSetBasePriorityThread(PKTHREAD Thread, LONG Increment)
{
   UNIMPLEMENTED;
}

KPRIORITY KeSetPriorityThread(PKTHREAD Thread, KPRIORITY Priority)
{
   KPRIORITY OldPriority;
   OldPriority = Thread->BasePriority;
   Thread->BasePriority = Priority;

   RemoveEntryList(&Thread->Entry);
   PsInsertIntoThreadList(Thread->BasePriority,
			  CONTAINING_RECORD(Thread,ETHREAD,Tcb));
   
   return(OldPriority);
}


NTSTATUS
STDCALL
NtAlertResumeThread (
	IN	HANDLE	ThreadHandle,
	OUT	PULONG	SuspendCount
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtAlertThread (
	IN	HANDLE	ThreadHandle
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtGetContextThread (
	IN	HANDLE		ThreadHandle, 
	OUT	PCONTEXT	Context
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtOpenThread (
	OUT	PHANDLE			ThreadHandle,
	IN	ACCESS_MASK		DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes,
	IN	PCLIENT_ID		ClientId
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtResumeThread (
	IN	HANDLE	ThreadHandle,
	IN	PULONG	SuspendCount
	)
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
   PETHREAD Thread;
   NTSTATUS Status;
   
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
   
   (*SuspendCount) = InterlockedDecrement(&Thread->Tcb.SuspendCount);
   if (Thread->Tcb.SuspendCount <= 0)
     {
	Thread->Tcb.State = THREAD_STATE_RUNNABLE;
     }
   
   ObDereferenceObject(Thread);
   return(STATUS_SUCCESS);
}


NTSTATUS
STDCALL
NtSetContextThread (
	IN	HANDLE		ThreadHandle,
	IN	PCONTEXT	Context
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtSuspendThread (
	IN	HANDLE	ThreadHandle,
	IN	PULONG	PreviousSuspendCount
	)
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
   PETHREAD Thread;
   NTSTATUS Status;
   
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
   
   (*PreviousSuspendCount) = InterlockedIncrement(&Thread->Tcb.SuspendCount);
   if (Thread->Tcb.SuspendCount > 0)
     {
	Thread->Tcb.State = THREAD_STATE_SUSPENDED;
	if (Thread == PsGetCurrentThread())
	  {
	     PsDispatchThread();
	  }
     }
   
   ObDereferenceObject(Thread);
   return(STATUS_SUCCESS);
}


NTSTATUS
STDCALL
NtContinue (
	IN	PCONTEXT	Context,
	IN	CINT		IrqLevel
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtYieldExecution(VOID)
{
	PsDispatchThread();
	return(STATUS_SUCCESS);
}
