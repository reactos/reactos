/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/kill.c
 * PURPOSE:         Terminating a thread
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ps.h>
#include <internal/ke.h>
#include <internal/mm.h>
#include <internal/ob.h>
#include <internal/port.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

extern ULONG PiNrThreads;
extern ULONG PiNrRunnableThreads;
extern KSPIN_LOCK PiThreadListLock;
extern LIST_ENTRY PiThreadListHead;
extern KSPIN_LOCK PiApcLock;

VOID PsTerminateCurrentThread(NTSTATUS ExitStatus);

/* FUNCTIONS *****************************************************************/

VOID 
PiTerminateProcessThreads(PEPROCESS Process, NTSTATUS ExitStatus)
{
   KIRQL oldlvl;
   PLIST_ENTRY current_entry;
   PETHREAD current;
   
   DPRINT("PiTerminateProcessThreads(Process %x, ExitStatus %x)\n",
	  Process, ExitStatus);
   
   KeAcquireSpinLock(&PiThreadListLock, &oldlvl);

   current_entry = Process->ThreadListHead.Flink;
   while (current_entry != &Process->ThreadListHead)
     {
	current = CONTAINING_RECORD(current_entry, ETHREAD,
				    Tcb.ProcessThreadListEntry);
	if (current != PsGetCurrentThread() &&
	    current->DeadThread == 0)
	  {
	     DPRINT("Terminating %x, current thread: %x, "
		    "thread's process: %x\n", current, PsGetCurrentThread(), 
		    current->ThreadsProcess);
	     KeReleaseSpinLock(&PiThreadListLock, oldlvl);
	     PsTerminateOtherThread(current, ExitStatus);
	     KeAcquireSpinLock(&PiThreadListLock, &oldlvl);
	     current_entry = Process->ThreadListHead.Flink;
	  }
	else
	  {
	     current_entry = current_entry->Flink;
	  }
     }
   KeReleaseSpinLock(&PiThreadListLock, oldlvl);
   DPRINT("Finished PiTerminateProcessThreads()\n");
}

VOID 
PsReapThreads(VOID)
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
	     PEPROCESS Process = current->ThreadsProcess; 
	     NTSTATUS Status = current->ExitStatus;
	     
	     DPRINT("PsProcessType %x\n", PsProcessType);
	     DPRINT("Reaping thread %x\n", current);
	     DPRINT("Ref count %d\n", ObGetReferenceCount(Process));
	     current->Tcb.State = THREAD_STATE_TERMINATED_2;
	     RemoveEntryList(&current->Tcb.ProcessThreadListEntry);
	     if (IsListEmpty(&Process->ThreadListHead))
	       {
		  DPRINT("Last thread terminated, terminating process\n");
		  KeReleaseSpinLock( &PiThreadListLock, oldIrql );
		  PiTerminateProcess(Process, Status);
		  KeAcquireSpinLock( &PiThreadListLock, &oldIrql );
	       }
	     DPRINT("Ref count %d\n", ObGetReferenceCount(Process));
	     KeReleaseSpinLock(&PiThreadListLock, oldIrql);
	     ObDereferenceObject(current);
	     KeAcquireSpinLock(&PiThreadListLock, &oldIrql);
	     current_entry = PiThreadListHead.Flink;
	  }
     }
   KeReleaseSpinLock(&PiThreadListLock, oldIrql);
}

VOID PsTerminateCurrentThread(NTSTATUS ExitStatus)
/*
 * FUNCTION: Terminates the current thread
 */
{
   KIRQL oldIrql;
   PETHREAD CurrentThread;
   
   CurrentThread = PsGetCurrentThread();
   
   DPRINT("terminating %x\n",CurrentThread);
   KeAcquireSpinLock(&PiThreadListLock, &oldIrql);
   
   CurrentThread->ExitStatus = ExitStatus;
   KeAcquireDispatcherDatabaseLock(FALSE);
   CurrentThread->Tcb.DispatcherHeader.SignalState = TRUE;
   KeDispatcherObjectWake(&CurrentThread->Tcb.DispatcherHeader);
   KeReleaseDispatcherDatabaseLock(FALSE);
   
   PsDispatchThreadNoLock(THREAD_STATE_TERMINATED_1);
   KeBugCheck(0);
}

VOID
PiTerminateThreadRundownRoutine(PKAPC Apc)
{
  ExFreePool(Apc);
}

VOID
PiTerminateThreadKernelRoutine(PKAPC Apc,
			       PKNORMAL_ROUTINE* NormalRoutine,
			       PVOID* NormalContext,
			       PVOID* SystemArgument1,
			       PVOID* SystemArguemnt2)
{
  ExFreePool(Apc);
}

VOID
PiTerminateThreadNormalRoutine(PVOID NormalContext,
			     PVOID SystemArgument1,
			     PVOID SystemArgument2)
{
  PsTerminateCurrentThread(PsGetCurrentThread()->ExitStatus);
}

VOID 
PsTerminateOtherThread(PETHREAD Thread, NTSTATUS ExitStatus)
/*
 * FUNCTION: Terminate a thread when calling from another thread's context
 * NOTES: This function must be called with PiThreadListLock held
 */
{
  PKAPC Apc;

  DPRINT("PsTerminateOtherThread(Thread %x, ExitStatus %x)\n",
	 Thread, ExitStatus);
  
  Thread->DeadThread = 1;
  Thread->ExitStatus = ExitStatus;
  Apc = ExAllocatePool(NonPagedPool, sizeof(KAPC));
  KeInitializeApc(Apc,
		  &Thread->Tcb,
		  0,
		  PiTerminateThreadKernelRoutine,
		  PiTerminateThreadRundownRoutine,
		  PiTerminateThreadNormalRoutine,
		  KernelMode,
		  NULL);
  KeInsertQueueApc(Apc,
		   NULL,
		   NULL,
		   KernelMode);
}

NTSTATUS STDCALL 
PiTerminateProcess(PEPROCESS Process, NTSTATUS ExitStatus)
{
   DPRINT("PiTerminateProcess(Process %x, ExitStatus %x) RC %d HC %d\n",
	   Process, ExitStatus, ObGetReferenceCount(Process),
	   ObGetHandleCount(Process));
   
   if (Process->Pcb.ProcessState == PROCESS_STATE_TERMINATED)
     {
	return(STATUS_SUCCESS);
     }
   
   ObCloseAllHandles(Process);
   KeAcquireDispatcherDatabaseLock(FALSE);
   Process->Pcb.ProcessState = PROCESS_STATE_TERMINATED;
   Process->Pcb.DispatcherHeader.SignalState = TRUE;
   KeDispatcherObjectWake(&Process->Pcb.DispatcherHeader);
   KeReleaseDispatcherDatabaseLock(FALSE);
   DPRINT("RC %d\n", ObGetReferenceCount(Process));
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL NtTerminateProcess(IN	HANDLE		ProcessHandle,
				    IN	NTSTATUS	ExitStatus)
{
   NTSTATUS Status;
   PEPROCESS Process;
   
   DPRINT("NtTerminateProcess(ProcessHandle %x, ExitStatus %x)\n",
	   ProcessHandle, ExitStatus);
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
                                      PROCESS_TERMINATE,
                                      PsProcessType,
				      UserMode,
                                      (PVOID*)&Process,
				      NULL);
   if (!NT_SUCCESS(Status))
   {
        return(Status);
   }
   
   PiTerminateProcessThreads(Process, ExitStatus);
   if (PsGetCurrentThread()->ThreadsProcess == Process)
     {
       ObDereferenceObject(Process);
       PsTerminateCurrentThread(ExitStatus);
     }
   ObDereferenceObject(Process);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtTerminateThread(IN	HANDLE		ThreadHandle,
				   IN	NTSTATUS	ExitStatus)
{
   PETHREAD Thread;
   NTSTATUS Status;
   
   Status = ObReferenceObjectByHandle(ThreadHandle,
				      THREAD_TERMINATE,
				      PsThreadType,
				      UserMode,
				      (PVOID*)&Thread,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   ObDereferenceObject(Thread);
   
   if (Thread == PsGetCurrentThread())
     {
	PsTerminateCurrentThread(ExitStatus);
     }
   else
     {
	PsTerminateOtherThread(Thread, ExitStatus);
     }
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL PsTerminateSystemThread(NTSTATUS ExitStatus)
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
	KeAcquireSpinLock(&Thread->ActiveTimerListLock, &oldIrql);
     }
   KeReleaseSpinLock(&Thread->ActiveTimerListLock, oldIrql);
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL 
NtRegisterThreadTerminatePort(HANDLE TerminationPortHandle)
{
   NTSTATUS Status;
   PEPORT_TERMINATION_REQUEST Request;
   PEPORT TerminationPort;
   KIRQL oldIrql;
   PETHREAD Thread;
   
   Status = ObReferenceObjectByHandle(TerminationPortHandle,
				      PORT_ALL_ACCESS,
				      ExPortType,
				      UserMode,
				      (PVOID*)&TerminationPort,
				      NULL);   
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   Request = ExAllocatePool(NonPagedPool, sizeof(Request));
   Request->Port = TerminationPort;
   Thread = PsGetCurrentThread();
   KeAcquireSpinLock(&Thread->ActiveTimerListLock, &oldIrql);
   InsertTailList(&Thread->TerminationPortList, &Request->ThreadListEntry);
   KeReleaseSpinLock(&Thread->ActiveTimerListLock, oldIrql);
   
   return(STATUS_SUCCESS);
}
