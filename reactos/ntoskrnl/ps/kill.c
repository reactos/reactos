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

/* FUNCTIONS *****************************************************************/

VOID PsTerminateCurrentThread(NTSTATUS ExitStatus)
/*
 * FUNCTION: Terminates the current thread
 */
{
   KIRQL oldlvl;
   PETHREAD CurrentThread;
   
   PiNrThreads--;
   
   CurrentThread = PsGetCurrentThread();
   
   CurrentThread->ExitStatus = ExitStatus;
   
   DPRINT("terminating %x\n",CurrentThread);
   ObDereferenceObject(CurrentThread->ThreadsProcess);
   CurrentThread->ThreadsProcess = NULL;
   KeRaiseIrql(DISPATCH_LEVEL,&oldlvl);
   
   ObDereferenceObject(CurrentThread);
   DPRINT("ObGetReferenceCount(CurrentThread) %d\n",
	  ObGetReferenceCount(CurrentThread));
   
   CurrentThread->Tcb.DispatcherHeader.SignalState = TRUE;
   KeDispatcherObjectWake(&CurrentThread->Tcb.DispatcherHeader);
   
   PsDispatchThread(THREAD_STATE_TERMINATED);
   KeBugCheck(0);
}

VOID PsTerminateOtherThread(PETHREAD Thread, NTSTATUS ExitStatus)
/*
 * FUNCTION: Terminate a thread when calling from that thread's context
 */
{
   KIRQL oldIrql;
   
   KeAcquireSpinLock(&PiThreadListLock, &oldIrql);
   PiNrThreads--;
   if (Thread->Tcb.State == THREAD_STATE_RUNNABLE)
     {
	PiNrRunnableThreads--;
	RemoveEntryList(&Thread->Tcb.QueueListEntry);
     }
   Thread->Tcb.State = THREAD_STATE_TERMINATED;
   Thread->Tcb.DispatcherHeader.SignalState = TRUE;
   KeDispatcherObjectWake(&Thread->Tcb.DispatcherHeader);
   ObDereferenceObject(Thread->ThreadsProcess);
   ObDereferenceObject(Thread);
   Thread->ThreadsProcess = NULL;
   KeReleaseSpinLock(&PiThreadListLock, oldIrql);
}


NTSTATUS STDCALL NtTerminateProcess(IN	HANDLE		ProcessHandle,
				    IN	NTSTATUS	ExitStatus)
{
   NTSTATUS Status;
   PEPROCESS Process;
   KIRQL oldlvl;
   
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
   
   PiTerminateProcessThreads(Process, ExitStatus);
   KeRaiseIrql(DISPATCH_LEVEL, &oldlvl);
   Process->Pcb.ProcessState = PROCESS_STATE_TERMINATED;
   Process->Pcb.DispatcherHeader.SignalState = TRUE;
   KeDispatcherObjectWake(&Process->Pcb.DispatcherHeader);
   if (PsGetCurrentThread()->ThreadsProcess == Process)
   {
      KeLowerIrql(oldlvl);
      ObDereferenceObject(Process);
      PsTerminateCurrentThread(ExitStatus);
   }
   KeLowerIrql(oldlvl);
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
