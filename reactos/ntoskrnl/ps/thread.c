/* $Id: thread.c,v 1.30 1999/11/12 12:01:17 dwelch Exp $
 *
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

KSPIN_LOCK PiThreadListLock;

/*
 * PURPOSE: List of threads associated with each priority level
 */
static LIST_ENTRY PiThreadListHead;
static LIST_ENTRY PriorityListHead[NR_THREAD_PRIORITY_LEVELS];
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

   KeAcquireSpinLock(&PiThreadListLock, &oldlvl);

   current_entry = PiThreadListHead.Flink;
   while (current_entry != &PiThreadListHead)
     {
	current = CONTAINING_RECORD(current_entry,ETHREAD,Tcb.QueueListEntry);
	if (current->ThreadsProcess == Process &&
	    current != PsGetCurrentThread())
	  {
	     PsTerminateOtherThread(current, ExitStatus);
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
}

VOID PsBeginThread(PKSTART_ROUTINE StartRoutine, PVOID StartContext)
{
   NTSTATUS Ret;
   
//   KeReleaseSpinLock(&PiThreadListLock,PASSIVE_LEVEL);
   KeLowerIrql(PASSIVE_LEVEL);
   Ret = StartRoutine(StartContext);
   PsTerminateSystemThread(Ret);
   KeBugCheck(0);
}

#if 0
VOID PsDumpThreads(VOID)
{
   KPRIORITY CurrentPriority;
   PLIST_ENTRY current_entry;
   PETHREAD current;
   
   for (CurrentPriority = THREAD_PRIORITY_TIME_CRITICAL; 
	(CurrentPriority >= THREAD_PRIORITY_IDLE);
	CurrentPriority--)
     {
	current_entry = PriorityListHead[THREAD_PRIORITY_MAX + CurrentPriority].Flink;
	
	while (current_entry != &PriorityListHead[THREAD_PRIORITY_MAX+CurrentPriority])
	  {
	     current = CONTAINING_RECORD(current_entry, ETHREAD, Tcb.Entry);
	     
	     DPRINT("%d: current %x current->Tcb.State %d\n",
		    CurrentPriority, current, current->Tcb.State);
	     
	     current_entry = current_entry->Flink;
	  }
     }
}
#endif

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


static VOID PsDispatchThreadNoLock (ULONG NewThreadStatus)
{
   KPRIORITY CurrentPriority;
   PETHREAD Candidate;
   
   if (!DoneInitYet)
     {
	return;
     }
   
   CurrentThread->Tcb.State = NewThreadStatus;
   if (CurrentThread->Tcb.State == THREAD_STATE_RUNNABLE)
     {
	PsInsertIntoThreadList(CurrentThread->Tcb.BasePriority,
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
	     
	     KeReleaseSpinLockFromDpcLevel(&PiThreadListLock);
	     HalTaskSwitch(&CurrentThread->Tcb);
	     return;
	  }
     }
   DbgPrint("CRITICAL: No threads are runnable\n");
   KeBugCheck(0);
}

VOID PiBeforeBeginThread(VOID)
{
   DPRINT("PiBeforeBeginThread()\n");
   //KeReleaseSpinLock(&PiThreadListLock, PASSIVE_LEVEL);
   KeLowerIrql(PASSIVE_LEVEL);
   DPRINT("KeGetCurrentIrql() %d\n", KeGetCurrentIrql());
}

VOID PsDispatchThread(ULONG NewThreadStatus)
{
   KIRQL oldIrql;
   
   KeAcquireSpinLock(&PiThreadListLock, &oldIrql);
   PsDispatchThreadNoLock(NewThreadStatus);
//   KeReleaseSpinLock(&PiThreadListLock, oldIrql);
   KeLowerIrql(oldIrql);
//   DPRINT("oldIrql %d\n",oldIrql);
}


NTSTATUS PsInitializeThread(HANDLE			ProcessHandle, 
			    PETHREAD		* ThreadPtr,
			    PHANDLE			ThreadHandle,
			    ACCESS_MASK		DesiredAccess,
			    POBJECT_ATTRIBUTES	ThreadAttributes)
{
   PETHREAD Thread;
   NTSTATUS Status;
   KIRQL oldIrql;
   
   PiNrThreads++;
   
   Thread = ObCreateObject(ThreadHandle,
			   DesiredAccess,
			   ThreadAttributes,
			   PsThreadType);
   DPRINT("Thread = %x\n",Thread);
   Thread->Tcb.State = THREAD_STATE_SUSPENDED;
   Thread->Tcb.BasePriority = THREAD_PRIORITY_NORMAL;
   Thread->Tcb.SuspendCount = 1;
   InitializeListHead(&Thread->Tcb.ApcState.ApcListHead[0]);
   InitializeListHead(&Thread->Tcb.ApcState.ApcListHead[1]);
   Thread->Tcb.KernelApcDisable = 1;

   KeInitializeDispatcherHeader(&Thread->Tcb.DispatcherHeader,
                                InternalThreadType,
                                sizeof(ETHREAD),
                                FALSE);

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
   InitializeListHead(&Thread->IrpList);
   Thread->Cid.UniqueThread = (HANDLE)InterlockedIncrement(
					      &PiNextThreadUniqueId);
   Thread->Cid.UniqueProcess = (HANDLE)Thread->ThreadsProcess->UniqueProcessId;
   DPRINT("Thread->Cid.UniqueThread %d\n",Thread->Cid.UniqueThread);
   ObReferenceObjectByPointer(Thread,
			      THREAD_ALL_ACCESS,
			      PsThreadType,
			      UserMode);
   
   *ThreadPtr = Thread;
   
   InsertTailList(&PiThreadListHead, &Thread->Tcb.ThreadListEntry);
   
   ObDereferenceObject(Thread->ThreadsProcess);
   return(STATUS_SUCCESS);
}


ULONG PsResumeThread(PETHREAD Thread)
{
   ULONG r;
   KIRQL oldIrql;
   
   DPRINT("PsResumeThread(Thread %x) CurrentThread %x \n",Thread,
	  PsGetCurrentThread());
   DPRINT("Thread->Tcb.SuspendCount %d\n",Thread->Tcb.SuspendCount);
   KeAcquireSpinLock(&PiThreadListLock, &oldIrql);
   r = InterlockedDecrement(&Thread->Tcb.SuspendCount);
   DPRINT("r %d Thread->Tcb.SuspendCount %d\n",r,Thread->Tcb.SuspendCount);
   if (r <= 0)
     {
//	DPRINT("Marking thread %x as runnable\n",Thread);
	Thread->Tcb.State = THREAD_STATE_RUNNABLE;
	PsInsertIntoThreadList(Thread->Tcb.BasePriority, Thread);
	PiNrRunnableThreads++;	
     }
   DPRINT("About release ThreadListLock = %x\n", &PiThreadListLock);
   KeReleaseSpinLock(&PiThreadListLock, oldIrql);
   DPRINT("Finished PsResumeThread()\n");
   return(r);
}


ULONG PsSuspendThread(PETHREAD Thread)
{
   ULONG r;
   KIRQL oldIrql;
   
   DPRINT("PsSuspendThread(Thread %x)\n",Thread);
   DPRINT("Thread->Tcb.BasePriority %d\n", Thread->Tcb.BasePriority);
   DPRINT("Thread->Tcb.SuspendCount %d\n",Thread->Tcb.SuspendCount);
   KeAcquireSpinLock(&PiThreadListLock, &oldIrql);
   r = InterlockedIncrement(&Thread->Tcb.SuspendCount);
   DPRINT("r %d Thread->Tcb.SuspendCount %d\n",r,Thread->Tcb.SuspendCount);
   
   if (r > 0)
     {
	if (Thread != CurrentThread)
	  {
	     if (Thread->Tcb.State == THREAD_STATE_RUNNABLE)
	       {
		  RemoveEntryList(&Thread->Tcb.QueueListEntry);
	       }
	     Thread->Tcb.State = THREAD_STATE_SUSPENDED;
	     PiNrRunnableThreads--;
	     KeReleaseSpinLock(&PiThreadListLock, oldIrql);
	  }
	else
	  {
	     DPRINT("Suspending current thread\n");
	     PiNrRunnableThreads--;
	     PsDispatchThreadNoLock(THREAD_STATE_SUSPENDED);
	     KeLowerIrql(oldIrql);
	  }
     }
   else
     {       
	DPRINT("About to release ThreadListLock = %x\n", &PiThreadListLock);
	KeReleaseSpinLock(&PiThreadListLock, oldIrql);
	DPRINT("PsSuspendThread() finished\n");
     }
   return(r);
}


VOID PiDeleteThread(PVOID ObjectBody)
{
   DbgPrint("PiDeleteThread(ObjectBody %x)\n",ObjectBody);
   
   HalReleaseTask((PETHREAD)ObjectBody);
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


static NTSTATUS
PsCreateTeb (HANDLE ProcessHandle,
             PNT_TEB *TebPtr,
             PETHREAD Thread,
             PINITIAL_TEB InitialTeb)
{
   MEMORY_BASIC_INFORMATION Info;
   NTSTATUS Status;
   ULONG ByteCount;
   ULONG RegionSize;
   ULONG TebSize;
   PVOID TebBase;
   NT_TEB Teb;

   TebBase = (PVOID)0x7FFDE000;
   TebSize = PAGESIZE;

   while (TRUE)
     {
        /* The TEB must reside in user space */
        Status = NtAllocateVirtualMemory(ProcessHandle,
                                         &TebBase,
                                         0,
                                         &TebSize,
                                         MEM_COMMIT,
                                         PAGE_READWRITE);
        if (NT_SUCCESS(Status))
          {
             DPRINT ("TEB allocated at %x\n", TebBase);
             break;
          }
        else
          {
             DPRINT ("TEB allocation failed! Status %x\n",Status);
          }

        TebBase = Info.BaseAddress - TebSize;
   }

   DPRINT ("TebBase %p TebSize %lu\n", TebBase, TebSize);

   /* set all pointers to and from the TEB */
   Teb.Tib.Self = TebBase;
   if (Thread->ThreadsProcess)
     {
        Teb.Peb = Thread->ThreadsProcess->Peb; /* No PEB yet!! */
     }

   /* store stack information from InitialTeb */
   if (InitialTeb != NULL)
     {
        Teb.Tib.StackBase = InitialTeb->StackBase;
        Teb.Tib.StackLimit = InitialTeb->StackLimit;

        /*
         * I don't know if this is really stored in a WNT-TEB,
         * but it's needed to free the thread stack. (Eric Kohl)
         */
        Teb.StackCommit = InitialTeb->StackCommit;
        Teb.StackCommitMax = InitialTeb->StackCommitMax;
        Teb.StackReserved = InitialTeb->StackReserved;
     }


   /* more initialization */
   Teb.Cid.UniqueThread = Thread->Cid.UniqueThread;
   Teb.Cid.UniqueProcess = Thread->Cid.UniqueProcess;

   /* write TEB data into teb page */
   Status = NtWriteVirtualMemory(ProcessHandle,
                                 TebBase,
                                 &Teb,
                                 sizeof(NT_TEB),
                                 &ByteCount);

   if (!NT_SUCCESS(Status))
     {
        /* free TEB */
        DPRINT ("Writing TEB failed!\n");

        RegionSize = 0;
        NtFreeVirtualMemory(ProcessHandle,
                            TebBase,
                            &RegionSize,
                            MEM_RELEASE);

        return Status;
     }

   /* FIXME: fs:[0] = TEB */

   if (TebPtr != NULL)
     {
//        *TebPtr = (PNT_TEB)TebBase;
     }

   DPRINT ("TEB allocated at %p\n", TebBase);

   return Status;
}


NTSTATUS
STDCALL
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
   PNT_TEB  TebBase;
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

   Status = PsCreateTeb (ProcessHandle,
                         &TebBase,
                         Thread,
                         InitialTeb);
   if (!NT_SUCCESS(Status))
     {
        return(Status);
     }

   /* Attention: TebBase is in user memory space */
//   Thread->Tcb.Teb = TebBase;

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
   KIRQL oldIrql;
   
   OldPriority = Thread->BasePriority;
   Thread->BasePriority = Priority;

   KeAcquireSpinLock(&PiThreadListLock, &oldIrql);
   RemoveEntryList(&Thread->QueueListEntry);
   PsInsertIntoThreadList(Thread->BasePriority,
			  CONTAINING_RECORD(Thread,ETHREAD,Tcb));
   KeReleaseSpinLock(&PiThreadListLock, oldIrql);
   return(OldPriority);
}


NTSTATUS STDCALL NtAlertResumeThread(IN	HANDLE	ThreadHandle,
				     OUT	PULONG	SuspendCount)
{
   UNIMPLEMENTED;
}


NTSTATUS STDCALL NtAlertThread (IN	HANDLE	ThreadHandle)
{
	UNIMPLEMENTED;
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
   
   (*SuspendCount) = PsResumeThread(Thread);
   
   ObDereferenceObject(Thread);
   return(STATUS_SUCCESS);
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
   
   (*PreviousSuspendCount) = PsSuspendThread(Thread);
   
   ObDereferenceObject(Thread);
   return(STATUS_SUCCESS);
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
