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

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

extern ULONG PiNrThreads;
extern ULONG PiNrRunnableThreads;
extern KSPIN_LOCK PiThreadListLock;
extern LIST_ENTRY PiThreadListHead;

/* FUNCTIONS *****************************************************************/

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
	     PEPROCESS Process = current->ThreadsProcess; 
	     NTSTATUS Status = current->ExitStatus;
	     
	     ObReferenceObjectByPointer(Process, 
					0, 
					PsProcessType, 
					KernelMode );
	     DPRINT("Reaping thread %x\n", current);
	     current->Tcb.State = THREAD_STATE_TERMINATED_2;
	     RemoveEntryList(&current->Tcb.ProcessThreadListEntry);
	     KeReleaseSpinLock(&PiThreadListLock, oldIrql);
	     ObDereferenceObject(current);
	     KeAcquireSpinLock(&PiThreadListLock, &oldIrql);
	     if(IsListEmpty( &Process->Pcb.ThreadListHead))
	       {
		  /* 
		   * TODO: Optimize this so it doesnt jerk the IRQL around so 
		   * much :)
		   */
		  DPRINT("Last thread terminated, terminating process\n");
		  KeReleaseSpinLock(&PiThreadListLock, oldIrql);
		  PiTerminateProcess(Process, Status);
		  KeAcquireSpinLock(&PiThreadListLock, &oldIrql);
	       }
	     ObDereferenceObject(Process);
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
   
   DPRINT("ObGetReferenceCount(CurrentThread) %d\n",
	  ObGetReferenceCount(CurrentThread));
   DPRINT("ObGetHandleCount(CurrentThread) %x\n",
	  ObGetHandleCount(CurrentThread));
   
   CurrentThread->Tcb.DispatcherHeader.SignalState = TRUE;
   KeDispatcherObjectWake(&CurrentThread->Tcb.DispatcherHeader);
   
   PsDispatchThreadNoLock(THREAD_STATE_TERMINATED_1);
   KeBugCheck(0);
}

VOID PsTerminateOtherThread(PETHREAD Thread, NTSTATUS ExitStatus)
/*
 * FUNCTION: Terminate a thread when calling from that thread's context
 */
{
   KIRQL oldIrql;
   
   KeAcquireSpinLock(&PiThreadListLock, &oldIrql);
   if (Thread->Tcb.State == THREAD_STATE_RUNNABLE)
     {
	RemoveEntryList(&Thread->Tcb.QueueListEntry);
     }
   Thread->Tcb.State = THREAD_STATE_TERMINATED_2;
   Thread->Tcb.DispatcherHeader.SignalState = TRUE;
   KeDispatcherObjectWake(&Thread->Tcb.DispatcherHeader);
   KeReleaseSpinLock(&PiThreadListLock, oldIrql);
   ObDereferenceObject(Thread);
}

NTSTATUS STDCALL PiTerminateProcess(PEPROCESS Process,
				    NTSTATUS ExitStatus)
{
   KIRQL oldlvl;
   
   DPRINT("PsTerminateProcess(Process %x, ExitStatus %x)\n",
          Process, ExitStatus);
   
   PiTerminateProcessThreads(Process, ExitStatus);
   KeRaiseIrql(DISPATCH_LEVEL, &oldlvl);
   Process->Pcb.ProcessState = PROCESS_STATE_TERMINATED;
   Process->Pcb.DispatcherHeader.SignalState = TRUE;
   KeDispatcherObjectWake(&Process->Pcb.DispatcherHeader);
   KeLowerIrql(oldlvl);
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
   if (Status != STATUS_SUCCESS)
   {
        return(Status);
   }
   
   PiTerminateProcess(Process, ExitStatus);
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


NTSTATUS PsTerminateSystemThread(NTSTATUS ExitStatus)
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


NTSTATUS STDCALL NtRegisterThreadTerminatePort(HANDLE	TerminationPort)
{
   UNIMPLEMENTED;
}
