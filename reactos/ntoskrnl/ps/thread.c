/* $Id: thread.c,v 1.126 2004/07/02 00:47:57 royce Exp $
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
#include <internal/ps.h>
#include <internal/ob.h>
#include <internal/pool.h>
#include <ntos/minmax.h>
#include <internal/ldr.h>
#include <rosrtl/string.h>

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
static ULONG PriorityListMask = 0;
static BOOLEAN DoneInitYet = FALSE;
static PETHREAD IdleThreads[MAXIMUM_PROCESSORS];
ULONG PiNrThreads = 0;
ULONG PiNrReadyThreads = 0;
static HANDLE PiReaperThreadHandle;
static KEVENT PiReaperThreadEvent;
static BOOLEAN PiReaperThreadShouldTerminate = FALSE;
ULONG PiNrThreadsAwaitingReaping = 0;

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
   return(((PIKPCR) KeGetCurrentKPCR())->CurrentThread);
}

/*
 * @implemented
 */
HANDLE STDCALL PsGetCurrentThreadId(VOID)
{
   return(PsGetCurrentThread()->Cid.UniqueThread);
}

static VOID
PsInsertIntoThreadList(KPRIORITY Priority, PETHREAD Thread)
{
   assert(THREAD_STATE_READY == Thread->Tcb.State);
   if (Priority >= MAXIMUM_PRIORITY || Priority < LOW_PRIORITY)
     {
	DPRINT1("Invalid thread priority (%d)\n", Priority);
	KEBUGCHECK(0);
     }
   InsertTailList(&PriorityListHead[Priority], &Thread->Tcb.QueueListEntry);
   PriorityListMask |= (1 << Priority);
   PiNrReadyThreads++;
}

static VOID PsRemoveFromThreadList(PETHREAD Thread)
{
   assert(THREAD_STATE_READY == Thread->Tcb.State);
   RemoveEntryList(&Thread->Tcb.QueueListEntry);
   if (IsListEmpty(&PriorityListHead[(ULONG)Thread->Tcb.Priority]))
     {
        PriorityListMask &= ~(1 << Thread->Tcb.Priority);
     }
   PiNrReadyThreads--;
}


VOID PsDumpThreads(BOOLEAN IncludeSystem)
{
   PLIST_ENTRY current_entry;
   PETHREAD current;
   ULONG t;
   ULONG i;

   current_entry = PiThreadListHead.Flink;
   t = 0;

   while (current_entry != &PiThreadListHead)
     {
       PULONG Ebp;
       PULONG Esp;

       current = CONTAINING_RECORD(current_entry, ETHREAD,
				   Tcb.ThreadListEntry);
       t++;
       if (t > PiNrThreads)
	 {
	   DbgPrint("Too many threads on list\n");
	   return;
	 }
       if (IncludeSystem || current->ThreadsProcess->UniqueProcessId >= 6)
	 {
	   DbgPrint("current->Tcb.State %d PID.TID %d.%d Name %.8s Stack: \n",
		    current->Tcb.State,
		    current->ThreadsProcess->UniqueProcessId,
		    current->Cid.UniqueThread,
		    current->ThreadsProcess->ImageFileName);
	   if (current->Tcb.State == THREAD_STATE_READY ||
	       current->Tcb.State == THREAD_STATE_SUSPENDED ||
	       current->Tcb.State == THREAD_STATE_BLOCKED)
	     {
	       Esp = (PULONG)current->Tcb.KernelStack;
	       Ebp = (PULONG)Esp[3];
	       DbgPrint("Ebp 0x%.8X\n", Ebp);
	       i = 0;
	       while (Ebp != 0 && Ebp >= (PULONG)current->Tcb.StackLimit)
		 {
		   DbgPrint("%.8X %.8X%s", Ebp[0], Ebp[1],
			    (i % 8) == 7 ? "\n" : "  ");
		   Ebp = (PULONG)Ebp[0];
		   i++;
		 }
	       if ((i % 8) != 7)
		 {
		   DbgPrint("\n");
		 }
	     }
	 }
       current_entry = current_entry->Flink;
     }
}

static PETHREAD PsScanThreadList (KPRIORITY Priority, ULONG Affinity)
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
           assert(current->Tcb.State == THREAD_STATE_READY);
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
  while (1)
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
   PKTHREAD KCurrentThread = ((PIKPCR) KeGetCurrentKPCR())->CurrentThread;
   PETHREAD CurrentThread = CONTAINING_RECORD(KCurrentThread, ETHREAD, Tcb);

   DPRINT("PsDispatchThread() %d/%d/%d/%d\n", KeGetCurrentProcessorNumber(),
	   CurrentThread->Cid.UniqueThread, NewThreadStatus, CurrentThread->Tcb.State);

   CurrentThread->Tcb.State = (UCHAR)NewThreadStatus;
   if (CurrentThread->Tcb.State == THREAD_STATE_READY)
     {
	PsInsertIntoThreadList(CurrentThread->Tcb.Priority,
			       CurrentThread);
     }
   if (CurrentThread->Tcb.State == THREAD_STATE_TERMINATED_1)
     {
       PiNrThreadsAwaitingReaping++;
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
#if 0
            /*
	     * This code is moved to the end of KiArchContextSwitch.
	     * It should be execute after the context switch.
	     */
	    KeReleaseSpinLockFromDpcLevel(&PiThreadListLock);
	    if (PiNrThreadsAwaitingReaping > 0)
	      {
		PiWakeupReaperThread();
	      }
#endif
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

   if (!DoneInitYet)
     {
	return;
     }

   KeAcquireSpinLock(&PiThreadListLock, &oldIrql);
   /*
    * Save wait IRQL
    */
   ((PIKPCR) KeGetCurrentKPCR())->CurrentThread->WaitIrql = oldIrql;
   PsDispatchThreadNoLock(NewThreadStatus);
   KeLowerIrql(oldIrql);
}

VOID
PsUnblockThread(PETHREAD Thread, PNTSTATUS WaitStatus)
{
  KIRQL oldIrql;

  KeAcquireSpinLock(&PiThreadListLock, &oldIrql);
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
  KeReleaseSpinLock(&PiThreadListLock, oldIrql);
}

VOID
PsBlockThread(PNTSTATUS Status, UCHAR Alertable, ULONG WaitMode,
	      BOOLEAN DispatcherLock, KIRQL WaitIrql, UCHAR WaitReason)
{
  KIRQL oldIrql;
  PKTHREAD KThread;
  PETHREAD Thread;
  PKWAIT_BLOCK WaitBlock;

  KeAcquireSpinLock(&PiThreadListLock, &oldIrql);

  KThread = ((PIKPCR) KeGetCurrentKPCR())->CurrentThread;
  Thread = CONTAINING_RECORD (KThread, ETHREAD, Tcb);
  if (KThread->ApcState.KernelApcPending)
  {
    if (!DispatcherLock)
      {
	KeAcquireDispatcherDatabaseLockAtDpcLevel();
      }
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
      if (DispatcherLock)
	{
	  KeReleaseDispatcherDatabaseLockFromDpcLevel();
	}
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

  KeAcquireSpinLock(&PiThreadListLock, &oldIrql);

  current_entry = Process->ThreadListHead.Flink;
  while (current_entry != &Process->ThreadListHead)
    {
      current = CONTAINING_RECORD(current_entry, ETHREAD,
				  Tcb.ProcessThreadListEntry);

      /*
       * We have to be careful here, we can't just set the freeze the
       * thread inside kernel mode since it may be holding a lock.
       */

      current_entry = current_entry->Flink;
    }

  KeReleaseSpinLock(&PiThreadListLock, oldIrql);
}

VOID
PsApplicationProcessorInit(VOID)
{
  ((PIKPCR) KeGetCurrentKPCR())->CurrentThread =
    (PVOID)IdleThreads[KeGetCurrentProcessorNumber()];
}

VOID INIT_FUNCTION
PsPrepareForApplicationProcessorInit(ULONG Id)
{
  PETHREAD IdleThread;
  HANDLE IdleThreadHandle;

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
  IdleThreads[Id] = IdleThread;

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
   PETHREAD FirstThread;
   ULONG i;
   HANDLE FirstThreadHandle;
   NTSTATUS Status;

   KeInitializeSpinLock(&PiThreadListLock);
   for (i=0; i < MAXIMUM_PRIORITY; i++)
     {
	InitializeListHead(&PriorityListHead[i]);
     }

   InitializeListHead(&PiThreadListHead);

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
   ((PIKPCR) KeGetCurrentKPCR())->CurrentThread = (PVOID)FirstThread;
   NtClose(FirstThreadHandle);

   DPRINT("FirstThread %x\n",FirstThread);

   DoneInitYet = TRUE;

   /*
    * Create the reaper thread
    */
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

   KeAcquireSpinLock(&PiThreadListLock, &oldIrql);

   OldPriority = Thread->Priority;
   Thread->BasePriority = Thread->Priority = (CHAR)Priority;

   if (OldPriority != Priority)
     {
       if (Thread->State == THREAD_STATE_READY)
         {
	   PsRemoveFromThreadList((PETHREAD)Thread);
	   PsInsertIntoThreadList(Priority, (PETHREAD)Thread);
	   CurrentThread = ((PIKPCR) KeGetCurrentKPCR())->CurrentThread;
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
   KeReleaseSpinLock(&PiThreadListLock, oldIrql);
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
	&& (ClientId->UniqueProcess)
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
	 Status = PsLookupProcessThreadByCid(ClientId,
                     NULL,
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
  KIRQL oldIrql;
  PLIST_ENTRY current_entry;
  PETHREAD current;

  KeAcquireSpinLock(&PiThreadListLock, &oldIrql);

  current_entry = PiThreadListHead.Flink;
  while (current_entry != &PiThreadListHead)
    {
      current = CONTAINING_RECORD(current_entry,
				  ETHREAD,
				  Tcb.ThreadListEntry);
      if (current->Cid.UniqueThread == Cid->UniqueThread &&
	  current->Cid.UniqueProcess == Cid->UniqueProcess)
	{
	  if (Process != NULL)
          {
	    *Process = current->ThreadsProcess;
            ObReferenceObject(current->ThreadsProcess);
          }

	  *Thread = current;
          ObReferenceObject(current);

	  KeReleaseSpinLock(&PiThreadListLock, oldIrql);
	  return(STATUS_SUCCESS);
	}

      current_entry = current_entry->Flink;
    }

  KeReleaseSpinLock(&PiThreadListLock, oldIrql);

  return(STATUS_INVALID_PARAMETER);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
PsLookupThreadByThreadId(IN PVOID ThreadId,
			 OUT PETHREAD *Thread)
{
  KIRQL oldIrql;
  PLIST_ENTRY current_entry;
  PETHREAD current;

  KeAcquireSpinLock(&PiThreadListLock, &oldIrql);

  current_entry = PiThreadListHead.Flink;
  while (current_entry != &PiThreadListHead)
    {
      current = CONTAINING_RECORD(current_entry,
				  ETHREAD,
				  Tcb.ThreadListEntry);
      if (current->Cid.UniqueThread == (HANDLE)ThreadId)
	{
	  KeReleaseSpinLock(&PiThreadListLock, oldIrql);
	  *Thread = current;
          ObReferenceObject(current);
	  return(STATUS_SUCCESS);
	}

      current_entry = current_entry->Flink;
    }

  KeReleaseSpinLock(&PiThreadListLock, oldIrql);

  return(STATUS_INVALID_PARAMETER);
}

/* EOF */
