/* $Id: thread.c,v 1.95 2002/06/15 11:27:28 jfilby Exp $
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
static PETHREAD IdleThreads[MAXIMUM_PROCESSORS];
ULONG PiNrThreads = 0;
ULONG PiNrRunnableThreads = 0;

static GENERIC_MAPPING PiThreadMapping = {THREAD_READ,
					  THREAD_WRITE,
					  THREAD_EXECUTE,
					  THREAD_ALL_ACCESS};

/* FUNCTIONS ***************************************************************/

PKTHREAD STDCALL KeGetCurrentThread(VOID)
{
   return(KeGetCurrentKPCR()->CurrentThread);
}

PETHREAD STDCALL PsGetCurrentThread(VOID)
{
  PKTHREAD CurrentThread = KeGetCurrentKPCR()->CurrentThread;
  return(CONTAINING_RECORD(CurrentThread, ETHREAD, Tcb));
}

HANDLE STDCALL PsGetCurrentThreadId(VOID)
{
   return(PsGetCurrentThread()->Cid.UniqueThread);
}

VOID 
PsInsertIntoThreadList(KPRIORITY Priority, PETHREAD Thread)
{
   if (Priority >= MAXIMUM_PRIORITY || Priority < 0)
     {
	DPRINT1("Invalid thread priority\n");
	KeBugCheck(0);
     }
   InsertTailList(&PriorityListHead[Priority], &Thread->Tcb.QueueListEntry);
   PiNrRunnableThreads++;
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
	   if (current->Tcb.State == THREAD_STATE_RUNNABLE ||
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
#if 0
   PLIST_ENTRY current_entry;
   PETHREAD current;
   
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
#else
   PLIST_ENTRY current_entry;
   PETHREAD current;

   current_entry = PriorityListHead[Priority].Flink;
   while (current_entry != &PriorityListHead[Priority])
     {
       current = CONTAINING_RECORD(current_entry, ETHREAD,
				   Tcb.QueueListEntry);
       assert(current->Tcb.State == THREAD_STATE_RUNNABLE);
       DPRINT("current->Tcb.UserAffinity %x Affinity %x PID %d %d\n",
	       current->Tcb.UserAffinity, Affinity, current->Cid.UniqueThread,
	       Priority);
       if (current->Tcb.UserAffinity & Affinity)
	 {
	   RemoveEntryList(&current->Tcb.QueueListEntry);
	   return(current);
	 }
       current_entry = current_entry->Flink;
     }
   return(NULL);
#endif
}


VOID PsDispatchThreadNoLock (ULONG NewThreadStatus)
{
   KPRIORITY CurrentPriority;
   PETHREAD Candidate;
   ULONG Affinity;
   PKTHREAD KCurrentThread = KeGetCurrentKPCR()->CurrentThread;
   PETHREAD CurrentThread = CONTAINING_RECORD(KCurrentThread, ETHREAD, Tcb);

   DPRINT("PsDispatchThread() %d/%d\n", KeGetCurrentProcessorNumber(),
	   CurrentThread->Cid.UniqueThread);
   
   CurrentThread->Tcb.State = NewThreadStatus;
   if (CurrentThread->Tcb.State == THREAD_STATE_RUNNABLE)
     {
	PiNrRunnableThreads++;
	PsInsertIntoThreadList(CurrentThread->Tcb.Priority,
			       CurrentThread);
     }
   
   Affinity = 1 << KeGetCurrentProcessorNumber();
   for (CurrentPriority = HIGH_PRIORITY;
	CurrentPriority >= LOW_PRIORITY;
	CurrentPriority--)
     {
	Candidate = PsScanThreadList(CurrentPriority, Affinity);
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
	    Ki386ContextSwitch(&CurrentThread->Tcb, &OldThread->Tcb);
	    PsReapThreads();
	    return;
	  }
     }
   CPRINT("CRITICAL: No threads are runnable\n");
   KeBugCheck(0);
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
   KeGetCurrentKPCR()->CurrentThread->WaitIrql = oldIrql;
   PsDispatchThreadNoLock(NewThreadStatus);
   KeLowerIrql(oldIrql);
}

VOID
PsUnblockThread(PETHREAD Thread, PNTSTATUS WaitStatus)
{
  KIRQL oldIrql;

  KeAcquireSpinLock(&PiThreadListLock, &oldIrql);
  if (WaitStatus != NULL)
    {
      Thread->Tcb.WaitStatus = *WaitStatus;
    }
  Thread->Tcb.State = THREAD_STATE_RUNNABLE;
  PsInsertIntoThreadList(Thread->Tcb.Priority, Thread);
  KeReleaseSpinLock(&PiThreadListLock, oldIrql);
}

VOID
PsBlockThread(PNTSTATUS Status, UCHAR Alertable, ULONG WaitMode, 
	      BOOLEAN DispatcherLock, KIRQL WaitIrql)
{
  KIRQL oldIrql;
  PKTHREAD KThread = KeGetCurrentKPCR()->CurrentThread;
  PETHREAD Thread = CONTAINING_RECORD (KThread, ETHREAD, Tcb);
  PKWAIT_BLOCK WaitBlock;

  KeAcquireSpinLock(&PiThreadListLock, &oldIrql);

  if (KThread->ApcState.KernelApcPending)
  {
    if (!DispatcherLock)
    {
      KeAcquireDispatcherDatabaseLock(FALSE);
    }
    WaitBlock = (PKWAIT_BLOCK)Thread->Tcb.WaitBlockList;
    while (WaitBlock)
    {
      RemoveEntryList (&WaitBlock->WaitListEntry);
      WaitBlock = WaitBlock->NextWaitBlock;
    }
    Thread->Tcb.WaitBlockList = NULL;
    KeReleaseDispatcherDatabaseLockAtDpcLevel(FALSE);
    PsDispatchThreadNoLock (THREAD_STATE_RUNNABLE);
    if (Status != NULL)
    {
      *Status = STATUS_KERNEL_APC;
    }
  }
  else
  {
    if (DispatcherLock)
    {
      KeReleaseDispatcherDatabaseLockAtDpcLevel(FALSE);
    }
    Thread->Tcb.Alertable = Alertable;
    Thread->Tcb.WaitMode = WaitMode;
    Thread->Tcb.WaitIrql = WaitIrql;
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
  KeGetCurrentKPCR()->CurrentThread = 
    (PVOID)IdleThreads[KeGetCurrentProcessorNumber()];
}

VOID
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
   PsThreadType->Close = PiCloseThread;
   PsThreadType->Delete = PiDeleteThread;
   PsThreadType->Parse = NULL;
   PsThreadType->Security = NULL;
   PsThreadType->QueryName = NULL;
   PsThreadType->OkayToClose = NULL;
   PsThreadType->Create = NULL;
   PsThreadType->DuplicationNotify = NULL;
   
   PsInitializeThread(NULL,&FirstThread,&FirstThreadHandle,
		      THREAD_ALL_ACCESS,NULL, TRUE);
   FirstThread->Tcb.State = THREAD_STATE_RUNNING;
   FirstThread->Tcb.FreezeCount = 0;
   KeGetCurrentKPCR()->CurrentThread = (PVOID)FirstThread;
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


NTSTATUS STDCALL 
NtAlertResumeThread(IN	HANDLE ThreadHandle,
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
   (VOID)PsUnblockThread(Thread, &ThreadStatus);
   
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
NtCallbackReturn (PVOID		Result,
		  ULONG		ResultLength,
		  NTSTATUS	Status)
{
  PULONG OldStack;
  PETHREAD Thread;
  PNTSTATUS CallbackStatus;
  PULONG CallerResultLength;
  PVOID* CallerResult;
  PVOID InitialStack;
  PVOID StackBase;
  ULONG StackLimit;
  KIRQL oldIrql;

  Thread = PsGetCurrentThread();
  OldStack = (PULONG)Thread->Tcb.CallbackStack;
  Thread->Tcb.CallbackStack = NULL;

  CallbackStatus = (PNTSTATUS)OldStack[0];
  CallerResultLength = (PULONG)OldStack[1];
  CallerResult = (PVOID*)OldStack[2];
  InitialStack = (PVOID)OldStack[3];
  StackBase = (PVOID)OldStack[4];
  StackLimit = OldStack[5];

  *CallbackStatus = Status;
  if (CallerResult != NULL && CallerResultLength != NULL)
    {
      if (Result == NULL)
	{
	  *CallerResultLength = 0;
	}
      else
	{
	  *CallerResultLength = min(ResultLength, *CallerResultLength);
	  memcpy(*CallerResult, Result, *CallerResultLength);
	}
    }

  KeRaiseIrql(HIGH_LEVEL, &oldIrql);
  Thread->Tcb.InitialStack = InitialStack;
  Thread->Tcb.StackBase = StackBase;
  Thread->Tcb.StackLimit = StackLimit;
  KeStackSwitchAndRet((PVOID)(OldStack + 6));

  /* Should never return. */
  KeBugCheck(0);
  return(STATUS_UNSUCCESSFUL);
}

PVOID STATIC
PsAllocateCallbackStack(ULONG StackSize)
{
  PVOID KernelStack = NULL;
  NTSTATUS Status;
  PMEMORY_AREA StackArea;
  ULONG i;

  StackSize = PAGE_ROUND_UP(StackSize);
  MmLockAddressSpace(MmGetKernelAddressSpace());
  Status = MmCreateMemoryArea(NULL,
			      MmGetKernelAddressSpace(),
			      MEMORY_AREA_KERNEL_STACK,
			      &KernelStack,
			      StackSize,
			      0,
			      &StackArea,
			      FALSE);
  MmUnlockAddressSpace(MmGetKernelAddressSpace());  
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Failed to create thread stack\n");
      return(NULL);
    }
  for (i = 0; i < (StackSize / PAGESIZE); i++)
    {
      PHYSICAL_ADDRESS Page;
      Status = MmRequestPageMemoryConsumer(MC_NPPOOL, TRUE, &Page);
      if (!NT_SUCCESS(Status))
	{
	  return(NULL);
	}
      Status = MmCreateVirtualMapping(NULL,
				      KernelStack + (i * PAGESIZE),
				      PAGE_EXECUTE_READWRITE,
				      Page,
				      TRUE);
    }
  return(KernelStack);
}

NTSTATUS STDCALL
NtW32Call (IN ULONG RoutineIndex,
	   IN PVOID Argument,
	   IN ULONG ArgumentLength,
	   OUT PVOID* Result OPTIONAL,
	   OUT PULONG ResultLength OPTIONAL)
{
  PETHREAD Thread;
  PVOID NewStack;
  ULONG StackSize;
  PKTRAP_FRAME NewFrame;
  PULONG UserEsp;
  KIRQL oldIrql;
  ULONG SavedStackLimit;
  PVOID SavedStackBase;
  PVOID SavedInitialStack;
  NTSTATUS CallbackStatus;

  Thread = PsGetCurrentThread();
  if (Thread->Tcb.CallbackStack != NULL)
    {
      return(STATUS_UNSUCCESSFUL);
    }

  /* Set up the new kernel and user environment. */
  StackSize = (ULONG)(Thread->Tcb.StackBase - Thread->Tcb.StackLimit);  
  NewStack = PsAllocateCallbackStack(StackSize);
  memcpy(NewStack + StackSize - sizeof(KTRAP_FRAME), Thread->Tcb.TrapFrame,
	 sizeof(KTRAP_FRAME));
  NewFrame = (PKTRAP_FRAME)(NewStack + StackSize - sizeof(KTRAP_FRAME));
  NewFrame->Esp -= (ArgumentLength + (4 * sizeof(ULONG))); 
  NewFrame->Eip = (ULONG)LdrpGetSystemDllCallbackDispatcher();
  UserEsp = (PULONG)NewFrame->Esp;
  UserEsp[0] = 0;     /* Return address. */
  UserEsp[1] = RoutineIndex;
  UserEsp[2] = (ULONG)&UserEsp[4];
  UserEsp[3] = ArgumentLength;
  memcpy((PVOID)&UserEsp[4], Argument, ArgumentLength);

  /* Switch to the new environment and return to user-mode. */
  KeRaiseIrql(HIGH_LEVEL, &oldIrql);
  SavedStackLimit = Thread->Tcb.StackLimit;
  SavedStackBase = Thread->Tcb.StackBase;
  SavedInitialStack = Thread->Tcb.InitialStack;
  Thread->Tcb.InitialStack = Thread->Tcb.StackBase = NewStack + StackSize;
  Thread->Tcb.StackLimit = (ULONG)NewStack;
  Thread->Tcb.KernelStack = NewStack + StackSize - sizeof(KTRAP_FRAME);
  KePushAndStackSwitchAndSysRet(SavedStackLimit, 
				(ULONG)SavedStackBase, 
				(ULONG)SavedInitialStack, (ULONG)Result, 
				(ULONG)ResultLength, (ULONG)&CallbackStatus, 
				Thread->Tcb.KernelStack);

  /* 
   * The callback return will have already restored most of the state we 
   * modified.
   */
  KeLowerIrql(PASSIVE_LEVEL);
  ExFreePool(NewStack);
  return(CallbackStatus);
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
	CPRINT("NtContinue called but TrapFrame was NULL\n");
	KeBugCheck(0);
     }
   KeContextToTrapFrame(Context, TrapFrame);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtYieldExecution(VOID)
{
  PsDispatchThread(THREAD_STATE_RUNNABLE);
  return(STATUS_SUCCESS);
}


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
	    *Process = current->ThreadsProcess;
	  *Thread = current;
	  KeReleaseSpinLock(&PiThreadListLock, oldIrql);
	  return(STATUS_SUCCESS);
	}

      current_entry = current_entry->Flink;
    }

  KeReleaseSpinLock(&PiThreadListLock, oldIrql);

  return(STATUS_INVALID_PARAMETER);
}


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
	  *Thread = current;
	  KeReleaseSpinLock(&PiThreadListLock, oldIrql);
	  return(STATUS_SUCCESS);
	}

      current_entry = current_entry->Flink;
    }

  KeReleaseSpinLock(&PiThreadListLock, oldIrql);

  return(STATUS_INVALID_PARAMETER);
}

/* EOF */
