/* $Id: thread.c,v 1.140 2004/12/10 16:50:37 navaraf Exp $
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

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* TYPES *******************************************************************/

/* GLOBALS ******************************************************************/

extern LIST_ENTRY PsProcessListHead;

POBJECT_TYPE EXPORTED PsThreadType = NULL;

KSPIN_LOCK PiThreadLock;
LONG PiNrThreadsAwaitingReaping = 0;

/*
 * PURPOSE: List of threads associated with each priority level
 */
static LIST_ENTRY PriorityListHead[MAXIMUM_PRIORITY];
static ULONG PriorityListMask = 0;
static BOOLEAN DoneInitYet = FALSE;
static KEVENT PiReaperThreadEvent;
static BOOLEAN PiReaperThreadShouldTerminate = FALSE;

static GENERIC_MAPPING PiThreadMapping = {THREAD_READ,
					  THREAD_WRITE,
					  THREAD_EXECUTE,
					  THREAD_ALL_ACCESS};

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
PKTHREAD STDCALL KeGetCurrentThread(VOID)
{
#ifdef MP
   ULONG Flags;
   PKTHREAD Thread;
   Ke386SaveFlags(Flags);
   Ke386DisableInterrupts();
   Thread = KeGetCurrentKPCR()->PrcbData.CurrentThread;
   Ke386RestoreFlags(Flags);
   return Thread;
#else
   return(KeGetCurrentKPCR()->PrcbData.CurrentThread);
#endif
}

/*
 * @implemented
 */
HANDLE STDCALL PsGetCurrentThreadId(VOID)
{
   return(PsGetCurrentThread()->Cid.UniqueThread);
}

/*
 * @implemented
 */
ULONG
STDCALL
PsGetThreadFreezeCount(
	PETHREAD Thread
	)
{
	return Thread->Tcb.FreezeCount;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
PsGetThreadHardErrorsAreDisabled(
    PETHREAD	Thread
	)
{
	return Thread->HardErrorsAreDisabled;
}

/*
 * @implemented
 */
HANDLE
STDCALL
PsGetThreadId(
    PETHREAD	Thread
	)
{
	return Thread->Cid.UniqueThread;
}

/*
 * @implemented
 */
PEPROCESS
STDCALL
PsGetThreadProcess(
    PETHREAD	Thread
	)
{
	return Thread->ThreadsProcess;
}

/*
 * @implemented
 */
HANDLE
STDCALL
PsGetThreadProcessId(
    PETHREAD	Thread
	)
{
	return Thread->Cid.UniqueProcess;
}

/*
 * @implemented
 */
HANDLE
STDCALL
PsGetThreadSessionId(
    PETHREAD	Thread
	)
{
	return (HANDLE)Thread->ThreadsProcess->SessionId;
}

/*
 * @implemented
 */
PTEB
STDCALL
PsGetThreadTeb(
    PETHREAD	Thread
	)
{
	return Thread->Tcb.Teb;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetThreadWin32Thread(
    PETHREAD	Thread
	)
{
	return Thread->Tcb.Win32Thread;
}

/*
 * @implemented
 */
KPROCESSOR_MODE
STDCALL
PsGetCurrentThreadPreviousMode (
    	VOID
	)
{
	return (KPROCESSOR_MODE)PsGetCurrentThread()->Tcb.PreviousMode;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetCurrentThreadStackBase (
    	VOID
	)
{
	return PsGetCurrentThread()->Tcb.StackBase;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetCurrentThreadStackLimit (
    	VOID
	)
{
	return (PVOID)PsGetCurrentThread()->Tcb.StackLimit;
}

/*
 * @implemented
 */
BOOLEAN STDCALL
PsIsThreadTerminating(IN PETHREAD Thread)
{
  return (Thread->HasTerminated ? TRUE : FALSE);
}

/*
 * @unimplemented
 */             
BOOLEAN
STDCALL
PsIsSystemThread(
    PETHREAD Thread
    )
{
	UNIMPLEMENTED;
	return FALSE;	
}

/*
 * @implemented
 */                       
BOOLEAN
STDCALL
PsIsThreadImpersonating(
    PETHREAD	Thread
	)
{
  return Thread->ActiveImpersonationInfo;
}

static VOID
PsInsertIntoThreadList(KPRIORITY Priority, PETHREAD Thread)
{
   ASSERT(THREAD_STATE_READY == Thread->Tcb.State);
   if (Priority >= MAXIMUM_PRIORITY || Priority < LOW_PRIORITY)
     {
	DPRINT1("Invalid thread priority (%d)\n", Priority);
	KEBUGCHECK(0);
     }
   InsertTailList(&PriorityListHead[Priority], &Thread->Tcb.QueueListEntry);
   PriorityListMask |= (1 << Priority);
}

static VOID PsRemoveFromThreadList(PETHREAD Thread)
{
   ASSERT(THREAD_STATE_READY == Thread->Tcb.State);
   RemoveEntryList(&Thread->Tcb.QueueListEntry);
   if (IsListEmpty(&PriorityListHead[(ULONG)Thread->Tcb.Priority]))
     {
        PriorityListMask &= ~(1 << Thread->Tcb.Priority);
     }
}


VOID PsDumpThreads(BOOLEAN IncludeSystem)
{
   PLIST_ENTRY AThread, AProcess;
   PEPROCESS Process;
   PETHREAD Thread;
   ULONG nThreads = 0;
   
   AProcess = PsProcessListHead.Flink;
   while(AProcess != &PsProcessListHead)
   {
     Process = CONTAINING_RECORD(AProcess, EPROCESS, ProcessListEntry);
     /* FIXME - skip suspended, ... processes? */
     if((Process != PsInitialSystemProcess) ||
        (Process == PsInitialSystemProcess && IncludeSystem))
     {
       AThread = Process->ThreadListHead.Flink;
       while(AThread != &Process->ThreadListHead)
       {
         Thread = CONTAINING_RECORD(AThread, ETHREAD, ThreadListEntry);

         nThreads++;
         DbgPrint("Thread->Tcb.State %d PID.TID %d.%d Name %.8s Stack: \n",
                  Thread->Tcb.State,
                  Thread->ThreadsProcess->UniqueProcessId,
                  Thread->Cid.UniqueThread,
                  Thread->ThreadsProcess->ImageFileName);
         if(Thread->Tcb.State == THREAD_STATE_READY ||
            Thread->Tcb.State == THREAD_STATE_SUSPENDED ||
            Thread->Tcb.State == THREAD_STATE_BLOCKED)
         {
           ULONG i = 0;
           PULONG Esp = (PULONG)Thread->Tcb.KernelStack;
           PULONG Ebp = (PULONG)Esp[3];
           DbgPrint("Ebp 0x%.8X\n", Ebp);
           while(Ebp != 0 && Ebp >= (PULONG)Thread->Tcb.StackLimit)
           {
             DbgPrint("%.8X %.8X%s", Ebp[0], Ebp[1], (i % 8) == 7 ? "\n" : "  ");
             Ebp = (PULONG)Ebp[0];
             i++;
           }
           if((i % 8) != 7)
           {
             DbgPrint("\n");
           }
         }
         AThread = AThread->Flink;
       }
     }
     AProcess = AProcess->Flink;
   }
}

static PETHREAD PsScanThreadList(KPRIORITY Priority, ULONG Affinity)
{
   PLIST_ENTRY current_entry;
   PETHREAD current;
   ULONG Mask;

   Mask = (1 << Priority);
   if (PriorityListMask & Mask)
     {
       current_entry = PriorityListHead[Priority].Flink;
       while (current_entry != &PriorityListHead[Priority])
         {
           current = CONTAINING_RECORD(current_entry, ETHREAD,
				       Tcb.QueueListEntry);
	   if (current->Tcb.State != THREAD_STATE_READY)
	     {
	       DPRINT1("%d/%d\n", current->Cid.UniqueThread, current->Tcb.State);
	     }
           ASSERT(current->Tcb.State == THREAD_STATE_READY);
           DPRINT("current->Tcb.UserAffinity %x Affinity %x PID %d %d\n",
	          current->Tcb.UserAffinity, Affinity, current->Cid.UniqueThread,
	          Priority);
           if (current->Tcb.UserAffinity & Affinity)
	     {
	       PsRemoveFromThreadList(current);
	       return(current);
	     }
           current_entry = current_entry->Flink;
	 }
     }
   return(NULL);
}

VOID STDCALL
PiWakeupReaperThread(VOID)
{
  KeSetEvent(&PiReaperThreadEvent, 0, FALSE);
}

VOID STDCALL
PiReaperThreadMain(PVOID Ignored)
{
  for(;;)
  {
    KeWaitForSingleObject(&PiReaperThreadEvent,
			  Executive,
			  KernelMode,
			  FALSE,
			  NULL);
    if (PiReaperThreadShouldTerminate)
	{
	  PsTerminateSystemThread(0);
	}
    PsReapThreads();
  }
}

VOID PsDispatchThreadNoLock (ULONG NewThreadStatus)
{
   KPRIORITY CurrentPriority;
   PETHREAD Candidate;
   ULONG Affinity;
   PKTHREAD KCurrentThread = KeGetCurrentThread();
   PETHREAD CurrentThread = CONTAINING_RECORD(KCurrentThread, ETHREAD, Tcb);

   DPRINT("PsDispatchThread() %d/%d/%d/%d\n", KeGetCurrentProcessorNumber(),
	   CurrentThread->Cid.UniqueThread, NewThreadStatus, CurrentThread->Tcb.State);

   CurrentThread->Tcb.State = (UCHAR)NewThreadStatus;
   switch(NewThreadStatus)
   {
     case THREAD_STATE_READY:
	PsInsertIntoThreadList(CurrentThread->Tcb.Priority,
			       CurrentThread);
	break;
     case THREAD_STATE_TERMINATED_1:
	PsQueueThreadReap(CurrentThread);
	break;
   }

   Affinity = 1 << KeGetCurrentProcessorNumber();
   for (CurrentPriority = HIGH_PRIORITY;
	CurrentPriority >= LOW_PRIORITY;
	CurrentPriority--)
     {
	Candidate = PsScanThreadList(CurrentPriority, Affinity);
	if (Candidate == CurrentThread)
	  {
	     Candidate->Tcb.State = THREAD_STATE_RUNNING;
	     KeReleaseSpinLockFromDpcLevel(&PiThreadLock);
	     return;
	  }
	if (Candidate != NULL)
	  {
	    PETHREAD OldThread;

	    DPRINT("Scheduling %x(%d)\n",Candidate, CurrentPriority);

	    Candidate->Tcb.State = THREAD_STATE_RUNNING;

	    OldThread = CurrentThread;
	    CurrentThread = Candidate;

	    MmUpdatePageDir(PsGetCurrentProcess(),(PVOID)CurrentThread->ThreadsProcess, sizeof(EPROCESS));

	    KiArchContextSwitch(&CurrentThread->Tcb, &OldThread->Tcb);
	    return;
	  }
     }
   CPRINT("CRITICAL: No threads are ready\n");
   KEBUGCHECK(0);
}

VOID STDCALL
PsDispatchThread(ULONG NewThreadStatus)
{
   KIRQL oldIrql;

   if (!DoneInitYet || KeGetCurrentKPCR()->PrcbData.IdleThread == NULL)
     {
	return;
     }

   KeAcquireSpinLock(&PiThreadLock, &oldIrql);
   /*
    * Save wait IRQL
    */
   KeGetCurrentThread()->WaitIrql = oldIrql;
   PsDispatchThreadNoLock(NewThreadStatus);
   KeLowerIrql(oldIrql);
}

VOID
PsUnblockThread(PETHREAD Thread, PNTSTATUS WaitStatus)
{
  KIRQL oldIrql;

  KeAcquireSpinLock(&PiThreadLock, &oldIrql);
  if (THREAD_STATE_TERMINATED_1 == Thread->Tcb.State ||
      THREAD_STATE_TERMINATED_2 == Thread->Tcb.State)
    {
       DPRINT("Can't unblock thread %d because it's terminating\n",
	       Thread->Cid.UniqueThread);
    }
  else if (THREAD_STATE_READY == Thread->Tcb.State ||
           THREAD_STATE_RUNNING == Thread->Tcb.State)
    {
       DPRINT("Can't unblock thread %d because it's ready or running\n",
	       Thread->Cid.UniqueThread);
    }
  else
    {
      if (WaitStatus != NULL)
	{
	  Thread->Tcb.WaitStatus = *WaitStatus;
	}
      Thread->Tcb.State = THREAD_STATE_READY;
      PsInsertIntoThreadList(Thread->Tcb.Priority, Thread);
    }
  KeReleaseSpinLock(&PiThreadLock, oldIrql);
}

VOID
PsBlockThread(PNTSTATUS Status, UCHAR Alertable, ULONG WaitMode,
	      BOOLEAN DispatcherLock, KIRQL WaitIrql, UCHAR WaitReason)
{
  KIRQL oldIrql;
  PKTHREAD KThread;
  PETHREAD Thread;
  PKWAIT_BLOCK WaitBlock;

  if (!DispatcherLock)
    {
      oldIrql = KeAcquireDispatcherDatabaseLock();
      KiAcquireSpinLock(&PiThreadLock);
    }
  else
    {
      KeAcquireSpinLock(&PiThreadLock, &oldIrql);
    }

  KThread = KeGetCurrentThread();
  Thread = CONTAINING_RECORD (KThread, ETHREAD, Tcb);
  if (KThread->ApcState.KernelApcPending)
  {
    WaitBlock = (PKWAIT_BLOCK)Thread->Tcb.WaitBlockList;
    while (WaitBlock)
      {
	RemoveEntryList (&WaitBlock->WaitListEntry);
	WaitBlock = WaitBlock->NextWaitBlock;
      }
    Thread->Tcb.WaitBlockList = NULL;
    KeReleaseDispatcherDatabaseLockFromDpcLevel();
    PsDispatchThreadNoLock (THREAD_STATE_READY);
    if (Status != NULL)
      {
	*Status = STATUS_KERNEL_APC;
      }
  }
  else
    {
      KeReleaseDispatcherDatabaseLockFromDpcLevel();
      Thread->Tcb.Alertable = Alertable;
      Thread->Tcb.WaitMode = (UCHAR)WaitMode;
      Thread->Tcb.WaitIrql = WaitIrql;
      Thread->Tcb.WaitReason = WaitReason;
      PsDispatchThreadNoLock(THREAD_STATE_BLOCKED);

      if (Status != NULL)
	{
	  *Status = Thread->Tcb.WaitStatus;
	}
    }
  KeLowerIrql(WaitIrql);
}

VOID
PsFreezeAllThreads(PEPROCESS Process)
     /*
      * Used by the debugging code to freeze all the process's threads
      * while the debugger is examining their state.
      */
{
  KIRQL oldIrql;
  PLIST_ENTRY current_entry;
  PETHREAD current;

  KeAcquireSpinLock(&PiThreadLock, &oldIrql);

  current_entry = Process->ThreadListHead.Flink;
  while (current_entry != &Process->ThreadListHead)
    {
      current = CONTAINING_RECORD(current_entry, ETHREAD,
				  ThreadListEntry);

      /*
       * We have to be careful here, we can't just set the freeze the
       * thread inside kernel mode since it may be holding a lock.
       */

      current_entry = current_entry->Flink;
    }

  KeReleaseSpinLock(&PiThreadLock, oldIrql);
}

ULONG
PsEnumThreadsByProcess(PEPROCESS Process)
{
  KIRQL oldIrql;
  PLIST_ENTRY current_entry;
  ULONG Count = 0;

  KeAcquireSpinLock(&PiThreadLock, &oldIrql);

  current_entry = Process->ThreadListHead.Flink;
  while (current_entry != &Process->ThreadListHead)
    {
      Count++;
      current_entry = current_entry->Flink;
    }
  
  KeReleaseSpinLock(&PiThreadLock, oldIrql);
  return Count;
}

/*
 * @unimplemented
 */                       
NTSTATUS
STDCALL
PsRemoveCreateThreadNotifyRoutine (
    IN PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;	
}

/*
 * @unimplemented
 */                       
ULONG
STDCALL
PsSetLegoNotifyRoutine(   	
	PVOID LegoNotifyRoutine  	 
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
 * @implemented
 */                       
VOID
STDCALL
PsSetThreadHardErrorsAreDisabled(
    PETHREAD	Thread,
    BOOLEAN	HardErrorsAreDisabled
	)
{
	Thread->HardErrorsAreDisabled = HardErrorsAreDisabled;
}

/*
 * @implemented
 */                       
VOID
STDCALL
PsSetThreadWin32Thread(
    PETHREAD	Thread,
    PVOID	Win32Thread
	)
{
	Thread->Tcb.Win32Thread = Win32Thread;
}

VOID
PsApplicationProcessorInit(VOID)
{
}

VOID INIT_FUNCTION
PsPrepareForApplicationProcessorInit(ULONG Id)
{
  PETHREAD IdleThread;
  HANDLE IdleThreadHandle;
  PKPCR Pcr = (PKPCR)((ULONG_PTR)KPCR_BASE + Id * PAGE_SIZE);

  PsInitializeThread(NULL,
		     &IdleThread,
		     &IdleThreadHandle,
		     THREAD_ALL_ACCESS,
		     NULL,
		     TRUE);
  IdleThread->Tcb.State = THREAD_STATE_RUNNING;
  IdleThread->Tcb.FreezeCount = 0;
  IdleThread->Tcb.UserAffinity = 1 << Id;
  IdleThread->Tcb.Priority = LOW_PRIORITY;
  Pcr->PrcbData.IdleThread = &IdleThread->Tcb;
  Pcr->PrcbData.CurrentThread = &IdleThread->Tcb;
  NtClose(IdleThreadHandle);
  DPRINT("IdleThread for Processor %d has PID %d\n",
	   Id, IdleThread->Cid.UniqueThread);
}

VOID INIT_FUNCTION
PsInitThreadManagment(VOID)
/*
 * FUNCTION: Initialize thread managment
 */
{
   HANDLE PiReaperThreadHandle;
   PETHREAD FirstThread;
   ULONG i;
   HANDLE FirstThreadHandle;
   NTSTATUS Status;

   KeInitializeSpinLock(&PiThreadLock);
   for (i=0; i < MAXIMUM_PRIORITY; i++)
     {
	InitializeListHead(&PriorityListHead[i]);
     }

   PsThreadType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));

   PsThreadType->Tag = TAG('T', 'H', 'R', 'T');
   PsThreadType->TotalObjects = 0;
   PsThreadType->TotalHandles = 0;
   PsThreadType->MaxObjects = 0;
   PsThreadType->MaxHandles = 0;
   PsThreadType->PagedPoolCharge = 0;
   PsThreadType->NonpagedPoolCharge = sizeof(ETHREAD);
   PsThreadType->Mapping = &PiThreadMapping;
   PsThreadType->Dump = NULL;
   PsThreadType->Open = NULL;
   PsThreadType->Close = NULL;
   PsThreadType->Delete = PiDeleteThread;
   PsThreadType->Parse = NULL;
   PsThreadType->Security = NULL;
   PsThreadType->QueryName = NULL;
   PsThreadType->OkayToClose = NULL;
   PsThreadType->Create = NULL;
   PsThreadType->DuplicationNotify = NULL;

   RtlRosInitUnicodeStringFromLiteral(&PsThreadType->TypeName, L"Thread");

   ObpCreateTypeObject(PsThreadType);

   PsInitializeThread(NULL,&FirstThread,&FirstThreadHandle,
		      THREAD_ALL_ACCESS,NULL, TRUE);
   FirstThread->Tcb.State = THREAD_STATE_RUNNING;
   FirstThread->Tcb.FreezeCount = 0;
   KeGetCurrentKPCR()->PrcbData.CurrentThread = (PVOID)FirstThread;
   NtClose(FirstThreadHandle);

   DPRINT("FirstThread %x\n",FirstThread);

   DoneInitYet = TRUE;

   /*
    * Create the reaper thread
    */
   PsInitializeThreadReaper();
   KeInitializeEvent(&PiReaperThreadEvent, SynchronizationEvent, FALSE);
   Status = PsCreateSystemThread(&PiReaperThreadHandle,
				 THREAD_ALL_ACCESS,
				 NULL,
				 NULL,
				 NULL,
				 PiReaperThreadMain,
				 NULL);
   if (!NT_SUCCESS(Status))
     {
       DPRINT1("PS: Failed to create reaper thread.\n");
       KEBUGCHECK(0);
     }

   NtClose(PiReaperThreadHandle);
}

/*
 * @implemented
 */
LONG STDCALL
KeSetBasePriorityThread (PKTHREAD	Thread,
			 LONG		Increment)
/*
 * Sets thread's base priority relative to the process' base priority
 * Should only be passed in THREAD_PRIORITY_ constants in pstypes.h
 */
{
   KPRIORITY Priority;
   if (Increment < -2)
     {
       Increment = -2;
     }
   else if (Increment > 2)
     {
       Increment = 2;
     }
   Priority = ((PETHREAD)Thread)->ThreadsProcess->Pcb.BasePriority + Increment;
   if (Priority < LOW_PRIORITY)
   {
     Priority = LOW_PRIORITY;
   }
   else if (Priority >= MAXIMUM_PRIORITY)
     {
       Thread->BasePriority = HIGH_PRIORITY;
     }
   KeSetPriorityThread(Thread, Priority);
   return 1;
}


/*
 * @implemented
 */
KPRIORITY STDCALL
KeSetPriorityThread (PKTHREAD Thread, KPRIORITY Priority)
{
   KPRIORITY OldPriority;
   KIRQL oldIrql;
   PKTHREAD CurrentThread;
   ULONG Mask;

   if (Priority < LOW_PRIORITY || Priority >= MAXIMUM_PRIORITY)
     {
	KEBUGCHECK(0);
     }

   KeAcquireSpinLock(&PiThreadLock, &oldIrql);

   OldPriority = Thread->Priority;
   Thread->BasePriority = Thread->Priority = (CHAR)Priority;

   if (OldPriority != Priority)
     {
       if (Thread->State == THREAD_STATE_READY)
         {
	   PsRemoveFromThreadList((PETHREAD)Thread);
	   PsInsertIntoThreadList(Priority, (PETHREAD)Thread);
	   CurrentThread = KeGetCurrentThread();
	   if (CurrentThread->Priority < Priority)
	     {
               PsDispatchThreadNoLock(THREAD_STATE_READY);
               KeLowerIrql(oldIrql);
	       return (OldPriority);
	     }
	 }
       else if (Thread->State == THREAD_STATE_RUNNING)
         {
	   if (Priority < OldPriority)
	     {
	       /* Check for threads with a higher priority */
	       Mask = ~((1 << (Priority + 1)) - 1);
	       if (PriorityListMask & Mask)
	         {
                   PsDispatchThreadNoLock(THREAD_STATE_READY);
                   KeLowerIrql(oldIrql);
	           return (OldPriority);
		 }
	     }
	 }
     }
   KeReleaseSpinLock(&PiThreadLock, oldIrql);
   return(OldPriority);
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
KeSetAffinityThread(PKTHREAD	Thread,
		    PVOID	AfMask)
/*
 * Sets thread's affinity
 */
{
	DPRINT1("KeSetAffinityThread() is a stub returning STATUS_SUCCESS");
	return STATUS_SUCCESS; // FIXME: Use function below
	//return ZwSetInformationThread(handle, ThreadAffinityMask,<pointer to affinity mask>,sizeof(KAFFINITY));
}


NTSTATUS STDCALL
NtAlertResumeThread(IN	HANDLE ThreadHandle,
		    OUT PULONG	SuspendCount)
{
   UNIMPLEMENTED;
   return(STATUS_NOT_IMPLEMENTED);

}


NTSTATUS STDCALL
NtAlertThread (IN HANDLE ThreadHandle)
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
   (VOID)PsUnblockThread(Thread, &ThreadStatus);

   ObDereferenceObject(Thread);
   return(STATUS_SUCCESS);
}

/**********************************************************************
 *	NtOpenThread/4
 *
 *	@implemented
 */
NTSTATUS STDCALL
NtOpenThread(OUT PHANDLE ThreadHandle,
	     IN	ACCESS_MASK DesiredAccess,
	     IN	POBJECT_ATTRIBUTES ObjectAttributes,
	     IN	PCLIENT_ID ClientId)
{
   NTSTATUS Status = STATUS_INVALID_PARAMETER;

   if((NULL != ThreadHandle)&&(NULL != ObjectAttributes))
   {
      PETHREAD EThread = NULL;

      if((ClientId)
	&& (ClientId->UniqueThread))
      {
         // It is an error to specify both
	 // ObjectAttributes.ObjectName
         // and ClientId.
         if((ObjectAttributes)
	   && (ObjectAttributes->ObjectName)
	   && (0 < ObjectAttributes->ObjectName->Length))
	 {
            return(STATUS_INVALID_PARAMETER_MIX);
	 }
	 // Parameters mix OK
         Status = PsLookupThreadByThreadId(ClientId->UniqueThread,
                     & EThread);
      }
      else if((ObjectAttributes)
	     && (ObjectAttributes->ObjectName)
	     && (0 < ObjectAttributes->ObjectName->Length))
      {
         // Three Ob attributes are forbidden
         if(!(ObjectAttributes->Attributes &
            (OBJ_PERMANENT | OBJ_EXCLUSIVE | OBJ_OPENIF)))
	 {
            Status = ObReferenceObjectByName(ObjectAttributes->ObjectName,
                        ObjectAttributes->Attributes,
                        NULL,
                        DesiredAccess,
                        PsThreadType,
                        UserMode,
                        NULL,
                        (PVOID*) & EThread);
	 }
      }
      // EThread may be OK...
      if(STATUS_SUCCESS == Status)
      {
         Status = ObCreateHandle(PsGetCurrentProcess(),
                     EThread,
                     DesiredAccess,
                     FALSE,
                     ThreadHandle);
         ObDereferenceObject(EThread);
      }
   }
   return(Status);
}

NTSTATUS STDCALL
NtYieldExecution(VOID)
{
  PsDispatchThread(THREAD_STATE_READY);
  return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
PsLookupProcessThreadByCid(IN PCLIENT_ID Cid,
			   OUT PEPROCESS *Process OPTIONAL,
			   OUT PETHREAD *Thread)
{
  PCID_OBJECT CidObject;
  PETHREAD FoundThread;

  CidObject = PsLockCidHandle((HANDLE)Cid->UniqueThread, PsThreadType);
  if(CidObject != NULL)
  {
    FoundThread = CidObject->Obj.Thread;
    ObReferenceObject(FoundThread);
    
    if(Process != NULL)
    {
      *Process = FoundThread->ThreadsProcess;
      ObReferenceObject(FoundThread->ThreadsProcess);
    }

    PsUnlockCidObject(CidObject);
    return STATUS_SUCCESS;
  }

  return STATUS_INVALID_PARAMETER;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
PsLookupThreadByThreadId(IN PVOID ThreadId,
			 OUT PETHREAD *Thread)
{
  PCID_OBJECT CidObject;
  
  CidObject = PsLockCidHandle((HANDLE)ThreadId, PsThreadType);
  if(CidObject != NULL)
  {
    *Thread = CidObject->Obj.Thread;
    ObReferenceObject(*Thread);
    
    PsUnlockCidObject(CidObject);
    return STATUS_SUCCESS;
  }

  return STATUS_INVALID_PARAMETER;
}

/* EOF */
