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
#include <internal/string.h>
#include <internal/hal.h>
#include <internal/ps.h>

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

static PETHREAD CurrentThread = NULL;

static ULONG NextThreadUniqueId = 0;

/* FUNCTIONS ***************************************************************/

PKTHREAD KeGetCurrentThread(VOID)
{
   return(&(CurrentThread->Tcb));
}

PETHREAD PsGetCurrentThread(VOID)
{
   return((PETHREAD)KeGetCurrentThread());
}

static VOID PsInsertIntoThreadList(KPRIORITY Priority, PETHREAD Thread)
{
   KIRQL oldlvl;
   
   DPRINT("PsInsertIntoThreadList(Priority %d, Thread %x)\n",Priority,Thread);
   
   KeAcquireSpinLock(&ThreadListLock,&oldlvl);
   InsertTailList(&PriorityListHead[THREAD_PRIORITY_MAX+Priority],
		  &Thread->Tcb.Entry);
   KeReleaseSpinLock(&ThreadListLock,oldlvl);
}

VOID PsBeginThread(PKSTART_ROUTINE StartRoutine, PVOID StartContext)
{
   NTSTATUS Ret;
   
   KeReleaseSpinLock(&ThreadListLock,PASSIVE_LEVEL);
   Ret = StartRoutine(StartContext);
   PsTerminateSystemThread(Ret);
   for(;;);
}

static PETHREAD PsScanThreadList(KPRIORITY Priority)
{
   PLIST_ENTRY current_entry;
   PETHREAD current;
   PETHREAD oldest = NULL;
   ULONG oldest_time = 0;
   
   DPRINT("PsScanThreadList(Priority %d)\n",Priority);
   
   current_entry = PriorityListHead[THREAD_PRIORITY_MAX+Priority].Flink;
   while (current_entry != &PriorityListHead[THREAD_PRIORITY_MAX+Priority])
     {
	current = CONTAINING_RECORD(current_entry,ETHREAD,Tcb.Entry);
	if (current->Tcb.ThreadState == THREAD_STATE_RUNNABLE)
	  {
	     if (oldest == NULL || oldest_time > current->Tcb.LastTick)
	       {
		  oldest = current;
		  oldest_time = current->Tcb.LastTick;
	       }
	  }
	current_entry = current_entry->Flink;
     }
   DPRINT("PsScanThreadList() = %x\n",oldest);
   return(oldest);
}

VOID PsDispatchThread(VOID)
{
   KPRIORITY CurrentPriority;
   PETHREAD Candidate;
   KIRQL irql;
   LARGE_INTEGER TickCount;
   
   KeAcquireSpinLock(&ThreadListLock,&irql);
   
   if (!DoneInitYet)
     {
	return;
     }
   
   DPRINT("PsDispatchThread() Current %x\n",CurrentThread);
      
   if (CurrentThread->Tcb.ThreadState==THREAD_STATE_RUNNING)     
     {
	CurrentThread->Tcb.ThreadState=THREAD_STATE_RUNNABLE;
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
	     CurrentThread->Tcb.LastTick = GET_LARGE_INTEGER_LOW_PART(TickCount);
	     CurrentThread->Tcb.ThreadState = THREAD_STATE_RUNNING;
	     KeReleaseSpinLock(&ThreadListLock,irql);
	     return;
	  }
	if (Candidate != NULL)
	  {	
	     DPRINT("Scheduling %x\n",Candidate);
	     
	     Candidate->Tcb.ThreadState = THREAD_STATE_RUNNING;
	     
	     KeQueryTickCount(&TickCount);
	     CurrentThread->Tcb.LastTick = GET_LARGE_INTEGER_LOW_PART(TickCount);
	     
	     CurrentThread = Candidate;
	     
	     HalTaskSwitch(&CurrentThread->Tcb);
	     KeReleaseSpinLock(&ThreadListLock,irql);
	     return;
	  }
     }
}

NTSTATUS PsInitializeThread(HANDLE ProcessHandle, 
			    PETHREAD* ThreadPtr,
			    PHANDLE ThreadHandle,
			    ACCESS_MASK DesiredAccess,
			    POBJECT_ATTRIBUTES ThreadAttributes)
{
   ULONG ThreadId;
   ULONG ProcessId;
   PETHREAD Thread;
   NTSTATUS Status;
   
   Thread = ObGenericCreateObject(ThreadHandle,
				  DesiredAccess,
				  ThreadAttributes,
				  PsThreadType);
   DPRINT("Thread = %x\n",Thread);
   Thread->Tcb.LastTick = 0;
   Thread->Tcb.ThreadState=THREAD_STATE_SUSPENDED;
   Thread->Tcb.BasePriority=THREAD_PRIORITY_NORMAL;
   Thread->Tcb.CurrentPriority=THREAD_PRIORITY_NORMAL;
   Thread->Tcb.ApcList=ExAllocatePool(NonPagedPool,sizeof(LIST_ENTRY));
   Thread->Tcb.SuspendCount = 1;
   if (ProcessHandle!=NULL)
     {
	Status = ObReferenceObjectByHandle(ProcessHandle,
					   PROCESS_CREATE_THREAD,
					   PsProcessType,
					   UserMode,
					   (PVOID*)&Thread->ThreadsProcess,
					   NULL);
	if (Status != STATUS_SUCCESS)
	  {
	     return(Status);
	  }
     }
   else
     {
	Thread->ThreadsProcess=SystemProcess;
     }
   InitializeListHead(Thread->Tcb.ApcList);
   InitializeListHead(&(Thread->IrpList));
   Thread->Cid.UniqueThread=NextThreadUniqueId++;
//   thread->Cid.ThreadId=InterlockedIncrement(&NextThreadUniqueId);
   PsInsertIntoThreadList(Thread->Tcb.CurrentPriority,Thread);
   
   *ThreadPtr = Thread;
   
   return(STATUS_SUCCESS);
}

VOID PsResumeThread(PETHREAD Thread)
{
   DPRINT("PsResumeThread(Thread %x)\n",Thread);
   
   Thread->Tcb.SuspendCount--;
   DPRINT("Thread->Tcb.SuspendCount %d\n",Thread->Tcb.SuspendCount);
   DPRINT("Thread->Tcb.ThreadState %d THREAD_STATE_RUNNING %d\n",
	    Thread->Tcb.ThreadState,THREAD_STATE_RUNNING);
   if (Thread->Tcb.SuspendCount <= 0 && 
       Thread->Tcb.ThreadState != THREAD_STATE_RUNNING)
     {
        DPRINT("Setting thread to runnable\n");
	Thread->Tcb.ThreadState = THREAD_STATE_RUNNABLE;
     }
   DPRINT("Finished PsResumeThread()\n");
}

VOID PsSuspendThread(PETHREAD Thread)
{
   DPRINT("PsSuspendThread(Thread %x)\n",Thread);
   Thread->Tcb.SuspendCount++;
   if (Thread->Tcb.SuspendCount > 0)
     {
	Thread->Tcb.ThreadState = THREAD_STATE_SUSPENDED;
	if (Thread == CurrentThread)
	  {
	     PsDispatchThread();
	  }
     }
}

void PsInitThreadManagment(void)
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
   PsThreadType->Delete = NULL;
   PsThreadType->Parse = NULL;
   PsThreadType->Security = NULL;
   PsThreadType->QueryName = NULL;
   PsThreadType->OkayToClose = NULL;
   
   PsInitializeThread(NULL,&FirstThread,&FirstThreadHandle,
		      THREAD_ALL_ACCESS,NULL);
   HalInitFirstTask(FirstThread);
   FirstThread->Tcb.ThreadState = THREAD_STATE_RUNNING;
   FirstThread->Tcb.SuspendCount = 0;

   DPRINT("FirstThread %x\n",FirstThread);
   
   CurrentThread = FirstThread;
   
   DoneInitYet = TRUE;
}

NTSTATUS NtCreateThread(PHANDLE ThreadHandle,
			ACCESS_MASK DesiredAccess,
			POBJECT_ATTRIBUTES ObjectAttributes,
			HANDLE ProcessHandle,
			PCLIENT_ID Client,
			PCONTEXT ThreadContext,
			PINITIAL_TEB InitialTeb,
			BOOLEAN CreateSuspended)
{
   return(ZwCreateThread(ThreadHandle,
			 DesiredAccess,
			 ObjectAttributes,
			 ProcessHandle,
			 Client,
			 ThreadContext,
			 InitialTeb,
			 CreateSuspended));
}

NTSTATUS ZwCreateThread(PHANDLE ThreadHandle,
			ACCESS_MASK DesiredAccess,
			POBJECT_ATTRIBUTES ObjectAttributes,
			HANDLE ProcessHandle,
			PCLIENT_ID Client,
			PCONTEXT ThreadContext,
			PINITIAL_TEB InitialTeb,
			BOOLEAN CreateSuspended)
{
   PETHREAD Thread;
   NTSTATUS Status;
   
   Status = PsInitializeThread(ProcessHandle,&Thread,ThreadHandle,
			       DesiredAccess,ObjectAttributes);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   HalInitTaskWithContext(Thread,ThreadContext);
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
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   Thread->StartAddress=StartRoutine;
   HalInitTask(Thread,StartRoutine,StartContext);

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
   OldPriority = Thread->CurrentPriority;
   Thread->CurrentPriority = Priority;

   RemoveEntryList(&Thread->Entry);
   PsInsertIntoThreadList(Thread->CurrentPriority,
			  CONTAINING_RECORD(Thread,ETHREAD,Tcb));
   
   return(OldPriority);
}

NTSTATUS STDCALL NtAlertResumeThread(IN HANDLE ThreadHandle,
				     OUT PULONG SuspendCount)
{
   return(ZwAlertResumeThread(ThreadHandle,SuspendCount));
}

NTSTATUS STDCALL ZwAlertResumeThread(IN HANDLE ThreadHandle,
				     OUT PULONG SuspendCount)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtAlertThread(IN HANDLE ThreadHandle)
{
   return(ZwAlertThread(ThreadHandle));
}

NTSTATUS STDCALL ZwAlertThread(IN HANDLE ThreadHandle)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtGetContextThread(IN HANDLE ThreadHandle, 
				    OUT PCONTEXT Context)
{
   return(ZwGetContextThread(ThreadHandle,Context));
}

NTSTATUS STDCALL ZwGetContextThread(IN HANDLE ThreadHandle, 
				    OUT PCONTEXT Context)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtOpenThread(OUT PHANDLE ThreadHandle,
			      IN ACCESS_MASK DesiredAccess,
			      IN POBJECT_ATTRIBUTES ObjectAttributes,
			      IN PCLIENT_ID ClientId)
{
   return(ZwOpenThread(ThreadHandle,
		       DesiredAccess,
		       ObjectAttributes,
		       ClientId));
}

NTSTATUS STDCALL ZwOpenThread(OUT PHANDLE ThreadHandle,
			      IN ACCESS_MASK DesiredAccess,
			      IN POBJECT_ATTRIBUTES ObjectAttributes,
			      IN PCLIENT_ID ClientId)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtResumeThread(IN HANDLE ThreadHandle,
				IN PULONG SuspendCount)
{
   return(ZwResumeThread(ThreadHandle,SuspendCount));
}

NTSTATUS STDCALL ZwResumeThread(IN HANDLE ThreadHandle,
				IN PULONG SuspendCount)
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
	Thread->Tcb.ThreadState = THREAD_STATE_RUNNABLE;
     }
   
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL NtSetContextThread(IN HANDLE ThreadHandle,
				    IN PCONTEXT Context)
{
   return(ZwSetContextThread(ThreadHandle,Context));
}

NTSTATUS STDCALL ZwSetContextThread(IN HANDLE ThreadHandle,
				    IN PCONTEXT Context)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtSuspendThread(IN HANDLE ThreadHandle,
				 IN PULONG PreviousSuspendCount)
{
   return(ZwSuspendThread(ThreadHandle,PreviousSuspendCount));
}

NTSTATUS STDCALL ZwSuspendThread(IN HANDLE ThreadHandle,
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
	Thread->Tcb.ThreadState = THREAD_STATE_SUSPENDED;
	if (Thread == PsGetCurrentThread())
	  {
	     PsDispatchThread();
	  }
     }
   
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL NtContinue(IN PCONTEXT Context, IN CINT IrqLevel)
{
   return(ZwContinue(Context,IrqLevel));
}

NTSTATUS STDCALL ZwContinue(IN PCONTEXT Context, IN CINT IrqLevel)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtYieldExecution(VOID)
{
   return(ZwYieldExecution());
}

NTSTATUS STDCALL ZwYieldExecution(VOID)
{
   PsDispatchThread();
   return(STATUS_SUCCESS);
}
