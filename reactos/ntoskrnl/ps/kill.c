/* $Id: kill.c,v 1.83 2004/11/13 22:27:15 hbirr Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/kill.c
 * PURPOSE:         Terminating a thread
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

extern KSPIN_LOCK PiThreadLock;

VOID PsTerminateCurrentThread(NTSTATUS ExitStatus);

#define TAG_TERMINATE_APC   TAG('T', 'A', 'P', 'C')

LIST_ENTRY ThreadsToReapHead;

/* FUNCTIONS *****************************************************************/

VOID
PsInitializeThreadReaper(VOID)
{
  InitializeListHead(&ThreadsToReapHead);
}

VOID
PsReapThreads(VOID)
{
  KIRQL oldlvl;
  PETHREAD Thread;
  PLIST_ENTRY ListEntry;

  KeAcquireSpinLock(&PiThreadLock, &oldlvl);
  while((ListEntry = RemoveHeadList(&ThreadsToReapHead)) != &ThreadsToReapHead)
  {
    PiNrThreadsAwaitingReaping--;
    KeReleaseSpinLock(&PiThreadLock, oldlvl);
    
    Thread = CONTAINING_RECORD(ListEntry, ETHREAD, TerminationPortList);

    ObDereferenceObject(Thread);

    KeAcquireSpinLock(&PiThreadLock, &oldlvl);
  }
  KeReleaseSpinLock(&PiThreadLock, oldlvl);
}

VOID
PsQueueThreadReap(PETHREAD Thread)
{
  InsertTailList(&ThreadsToReapHead, &Thread->TerminationPortList);
  PiNrThreadsAwaitingReaping++;
}

VOID
PiTerminateProcessThreads(PEPROCESS Process,
			  NTSTATUS ExitStatus)
{
   KIRQL oldlvl;
   PLIST_ENTRY current_entry;
   PETHREAD current, CurrentThread = PsGetCurrentThread();
   
   DPRINT("PiTerminateProcessThreads(Process %x, ExitStatus %x)\n",
	  Process, ExitStatus);
   
   KeAcquireSpinLock(&PiThreadLock, &oldlvl);

   current_entry = Process->ThreadListHead.Flink;
   while (current_entry != &Process->ThreadListHead)
     {
	current = CONTAINING_RECORD(current_entry, ETHREAD,
				    ThreadListEntry);
	if (current != CurrentThread && current->HasTerminated == 0)
	  {
	     DPRINT("Terminating %x, current thread: %x, "
		    "thread's process: %x\n", current, PsGetCurrentThread(), 
		    current->ThreadsProcess);
	     KeReleaseSpinLock(&PiThreadLock, oldlvl);
	     PsTerminateOtherThread(current, ExitStatus);
	     KeAcquireSpinLock(&PiThreadLock, &oldlvl);
	     current_entry = Process->ThreadListHead.Flink;
	  }
	else
	  {
	     current_entry = current_entry->Flink;
	  }
     }
   KeReleaseSpinLock(&PiThreadLock, oldlvl);
   DPRINT("Finished PiTerminateProcessThreads()\n");
}

VOID
PsTerminateCurrentThread(NTSTATUS ExitStatus)
/*
 * FUNCTION: Terminates the current thread
 */
{
   KIRQL oldIrql;
   PETHREAD CurrentThread;
   PLIST_ENTRY current_entry;
   PKMUTANT Mutant;
   BOOLEAN Last;
   PEPROCESS CurrentProcess;
   SIZE_T Length = PAGE_SIZE;
   PVOID TebBlock;

   KeLowerIrql(PASSIVE_LEVEL);

   CurrentThread = PsGetCurrentThread();
   CurrentProcess = CurrentThread->ThreadsProcess;

   /* Can't terminate a thread if it attached another process */
   if (AttachedApcEnvironment == CurrentThread->Tcb.ApcStateIndex)
     {
        KEBUGCHECKEX(INVALID_PROCESS_ATTACH_ATTEMPT, (ULONG) CurrentProcess,
                     (ULONG) CurrentThread->Tcb.ApcState.Process,
                     (ULONG) CurrentThread->Tcb.ApcStateIndex,
                     (ULONG) CurrentThread);
     }

   KeAcquireSpinLock(&PiThreadLock, &oldIrql);

   DPRINT("terminating %x\n",CurrentThread);

   CurrentThread->ExitStatus = ExitStatus;
   KeQuerySystemTime((PLARGE_INTEGER)&CurrentThread->ExitTime);
   KeCancelTimer(&CurrentThread->Tcb.Timer);
 
   /* Remove the thread from the thread list of its process */
   RemoveEntryList(&CurrentThread->ThreadListEntry);
   Last = IsListEmpty(&CurrentProcess->ThreadListHead);

   KeReleaseSpinLock(&PiThreadLock, oldIrql);

   /* Notify subsystems of the thread termination */
   PspRunCreateThreadNotifyRoutines(CurrentThread, FALSE);
   PsTerminateWin32Thread(CurrentThread);

   /* Free the TEB */
   if(CurrentThread->Tcb.Teb)
   {
     DPRINT("Decommit teb at %p\n", CurrentThread->Tcb.Teb);
     ExAcquireFastMutex(&CurrentProcess->TebLock);
     TebBlock = MM_ROUND_DOWN(CurrentThread->Tcb.Teb, MM_VIRTMEM_GRANULARITY);
     ZwFreeVirtualMemory(NtCurrentProcess(),
                         (PVOID *)&CurrentThread->Tcb.Teb,
                         &Length,
                         MEM_DECOMMIT);
     DPRINT("teb %p, TebBlock %p\n", CurrentThread->Tcb.Teb, TebBlock);
     if (TebBlock != CurrentProcess->TebBlock ||
         CurrentProcess->TebBlock == CurrentProcess->TebLastAllocated)
       {
         MmLockAddressSpace(&CurrentProcess->AddressSpace);
         MmReleaseMemoryAreaIfDecommitted(CurrentProcess, &CurrentProcess->AddressSpace, TebBlock);
         MmUnlockAddressSpace(&CurrentProcess->AddressSpace);
       }
     ExReleaseFastMutex(&CurrentProcess->TebLock);
   }

   /* abandon all owned mutants */
   current_entry = CurrentThread->Tcb.MutantListHead.Flink;
   while (current_entry != &CurrentThread->Tcb.MutantListHead)
     {
	Mutant = CONTAINING_RECORD(current_entry, KMUTANT,
				   MutantListEntry);
	KeReleaseMutant(Mutant,
			MUTANT_INCREMENT,
			TRUE,
			FALSE);
	current_entry = CurrentThread->Tcb.MutantListHead.Flink;
     }

   oldIrql = KeAcquireDispatcherDatabaseLock();
   CurrentThread->Tcb.DispatcherHeader.SignalState = TRUE;
   KeDispatcherObjectWake(&CurrentThread->Tcb.DispatcherHeader);
   KeReleaseDispatcherDatabaseLock (oldIrql);

   /* The last thread shall close the door on exit */
   if(Last)
   {
    PspRunCreateProcessNotifyRoutines(CurrentProcess, FALSE);
    PsTerminateWin32Process(CurrentProcess);
    PiTerminateProcess(CurrentProcess, ExitStatus);
   }

   KeAcquireSpinLock(&PiThreadLock, &oldIrql);

#ifdef _ENABLE_THRDEVTPAIR
   ExpSwapThreadEventPair(CurrentThread, NULL); /* Release the associated eventpair object, if there was one */
#endif /* _ENABLE_THRDEVTPAIR */
   KeRemoveAllWaitsThread (CurrentThread, STATUS_UNSUCCESSFUL, FALSE);

   PsDispatchThreadNoLock(THREAD_STATE_TERMINATED_1);
   DPRINT1("Unexpected return, CurrentThread %x PsGetCurrentThread() %x\n", CurrentThread, PsGetCurrentThread());
   KEBUGCHECK(0);
}

VOID STDCALL
PiTerminateThreadRundownRoutine(PKAPC Apc)
{
  ExFreePool(Apc);
}

VOID STDCALL
PiTerminateThreadKernelRoutine(PKAPC Apc,
			       PKNORMAL_ROUTINE* NormalRoutine,
			       PVOID* NormalContext,
			       PVOID* SystemArgument1,
			       PVOID* SystemArguemnt2)
{
  ExFreePool(Apc);
}

VOID STDCALL
PiTerminateThreadNormalRoutine(PVOID NormalContext,
			     PVOID SystemArgument1,
			     PVOID SystemArgument2)
{
  PsTerminateCurrentThread(PsGetCurrentThread()->ExitStatus);
}

VOID
PsTerminateOtherThread(PETHREAD Thread,
		       NTSTATUS ExitStatus)
/*
 * FUNCTION: Terminate a thread when calling from another thread's context
 * NOTES: This function must be called with PiThreadLock held
 */
{
  PKAPC Apc;
  NTSTATUS Status;

  DPRINT("PsTerminateOtherThread(Thread %x, ExitStatus %x)\n",
	 Thread, ExitStatus);
  
  Thread->HasTerminated = TRUE;
  Thread->ExitStatus = ExitStatus;
  Apc = ExAllocatePoolWithTag(NonPagedPool, sizeof(KAPC), TAG_TERMINATE_APC);
  KeInitializeApc(Apc,
		  &Thread->Tcb,
		  OriginalApcEnvironment,
		  PiTerminateThreadKernelRoutine,
		  PiTerminateThreadRundownRoutine,
		  PiTerminateThreadNormalRoutine,
		  KernelMode,
		  NULL);
  KeInsertQueueApc(Apc,
		   NULL,
		   NULL,
		   IO_NO_INCREMENT);
  if (THREAD_STATE_BLOCKED == Thread->Tcb.State && UserMode == Thread->Tcb.WaitMode)
    {
      DPRINT("Unblocking thread\n");
      Status = STATUS_THREAD_IS_TERMINATING;
      KeRemoveAllWaitsThread(Thread, Status, TRUE);
    }
}

NTSTATUS STDCALL
PiTerminateProcess(PEPROCESS Process,
		   NTSTATUS ExitStatus)
{
   KIRQL OldIrql;
   PEPROCESS CurrentProcess;

   DPRINT("PiTerminateProcess(Process %x, ExitStatus %x) PC %d HC %d\n",
	   Process, ExitStatus, ObGetObjectPointerCount(Process),
	   ObGetObjectHandleCount(Process));
   
   ObReferenceObject(Process);
   if (InterlockedExchange((PLONG)&Process->Pcb.State, 
			   PROCESS_STATE_TERMINATED) == 
       PROCESS_STATE_TERMINATED)
     {
        ObDereferenceObject(Process);
	return(STATUS_SUCCESS);
     }
   CurrentProcess = PsGetCurrentProcess();
   if (Process != CurrentProcess)
   {
      KeAttachProcess(&Process->Pcb);
   }
   ObCloseAllHandles(Process);
   if (Process != CurrentProcess)
   {
      KeDetachProcess();
   }
   OldIrql = KeAcquireDispatcherDatabaseLock ();
   Process->Pcb.DispatcherHeader.SignalState = TRUE;
   KeDispatcherObjectWake(&Process->Pcb.DispatcherHeader);
   KeReleaseDispatcherDatabaseLock (OldIrql);
   ObDereferenceObject(Process);
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL
NtTerminateProcess(IN	HANDLE		ProcessHandle  OPTIONAL,
		   IN	NTSTATUS	ExitStatus)
{
   NTSTATUS Status;
   PEPROCESS Process;
   
   DPRINT("NtTerminateProcess(ProcessHandle %x, ExitStatus %x)\n",
	   ProcessHandle, ExitStatus);
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
                                      PROCESS_TERMINATE,
                                      PsProcessType,
				      KeGetCurrentThread()->PreviousMode,
                                      (PVOID*)&Process,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
       return(Status);
     }
   Process->ExitStatus = ExitStatus;
   PiTerminateProcessThreads(Process, ExitStatus);
   if (PsGetCurrentThread()->ThreadsProcess == Process)
     {
       ObDereferenceObject(Process);
       PsTerminateCurrentThread(ExitStatus);
       /*
        * We should never get here!
        */
       return(STATUS_SUCCESS);
     }
   ObDereferenceObject(Process);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtTerminateThread(IN	HANDLE		ThreadHandle,
		  IN	NTSTATUS	ExitStatus)
{
   PETHREAD Thread;
   NTSTATUS Status;
   
   Status = ObReferenceObjectByHandle(ThreadHandle,
				      THREAD_TERMINATE,
				      PsThreadType,
				      KeGetCurrentThread()->PreviousMode,
				      (PVOID*)&Thread,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   if (Thread == PsGetCurrentThread())
     {
        /* dereference the thread object before we kill our thread */
        ObDereferenceObject(Thread);
        PsTerminateCurrentThread(ExitStatus);
        /*
         * We should never get here!
         */
     }
   else
     {
	PsTerminateOtherThread(Thread, ExitStatus);
	ObDereferenceObject(Thread);
     }
   return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
PsTerminateSystemThread(NTSTATUS ExitStatus)
/*
 * FUNCTION: Terminates the current thread
 * ARGUMENTS:
 *         ExitStatus = Status to pass to the creater
 * RETURNS: Doesn't
 */
{
   PsTerminateCurrentThread(ExitStatus);
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL
NtCallTerminatePorts(PETHREAD Thread)
{
   KIRQL oldIrql;
   PLIST_ENTRY current_entry;
   PEPORT_TERMINATION_REQUEST current;
   
   KeAcquireSpinLock(&Thread->ActiveTimerListLock, &oldIrql);
   while ((current_entry = RemoveHeadList(&Thread->TerminationPortList)) !=
	  &Thread->TerminationPortList);
     {
	current = CONTAINING_RECORD(current_entry,
				    EPORT_TERMINATION_REQUEST,
				    ThreadListEntry);
	KeReleaseSpinLock(&Thread->ActiveTimerListLock, oldIrql);
	LpcSendTerminationPort(current->Port, 
			       Thread->CreateTime);
	ExFreePool(current);
	KeAcquireSpinLock(&Thread->ActiveTimerListLock, &oldIrql);
     }
   KeReleaseSpinLock(&Thread->ActiveTimerListLock, oldIrql);
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL
NtRegisterThreadTerminatePort(HANDLE PortHandle)
{
   NTSTATUS Status;
   PEPORT_TERMINATION_REQUEST Request;
   PEPORT TerminationPort;
   KIRQL oldIrql;
   PETHREAD Thread;
   
   Status = ObReferenceObjectByHandle(PortHandle,
				      PORT_ALL_ACCESS,
				      ExPortType,
				      KeGetCurrentThread()->PreviousMode,
				      (PVOID*)&TerminationPort,
				      NULL);   
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   Request = ExAllocatePool(NonPagedPool, sizeof(EPORT_TERMINATION_REQUEST));
   if(Request != NULL)
   {
     Request->Port = TerminationPort;
     Thread = PsGetCurrentThread();
     KeAcquireSpinLock(&Thread->ActiveTimerListLock, &oldIrql);
     InsertTailList(&Thread->TerminationPortList, &Request->ThreadListEntry);
     KeReleaseSpinLock(&Thread->ActiveTimerListLock, oldIrql);

     return(STATUS_SUCCESS);
   }
   else
   {
     ObDereferenceObject(TerminationPort);
     return(STATUS_INSUFFICIENT_RESOURCES);
   }
}
