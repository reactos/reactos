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
   CurrentThread->Tcb.State = THREAD_STATE_TERMINATED;
   KeDispatcherObjectWake(&CurrentThread->Tcb.DispatcherHeader);
   ZwYieldExecution();
   for(;;);
}

VOID PsTerminateOtherThread(PETHREAD Thread, NTSTATUS ExitStatus)
/*
 * FUNCTION: Terminate a thread when calling from that thread's context
 */
{
   KIRQL oldlvl;
   
   PiNrThreads--;
   KeRaiseIrql(DISPATCH_LEVEL, &oldlvl);
   Thread->Tcb.State = THREAD_STATE_TERMINATED;
   KeDispatcherObjectWake(&Thread->Tcb.DispatcherHeader);
   ObDereferenceObject(Thread->ThreadsProcess);
   Thread->ThreadsProcess = NULL;
   KeLowerIrql(oldlvl);
}


NTSTATUS
STDCALL
NtTerminateProcess (
	IN	HANDLE		ProcessHandle,
	IN	NTSTATUS	ExitStatus
	)
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


NTSTATUS
STDCALL
NtTerminateThread (
	IN	HANDLE		ThreadHandle,
	IN	NTSTATUS	ExitStatus
	)
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

VOID PsReleaseThread(PETHREAD Thread)
/*
 * FUNCTION: Called internally to release a thread's resources from another
 * thread's context
 * ARGUMENTS:
 *        Thread = Thread to release
 */
{
   DPRINT("PsReleaseThread(Thread %x)\n",Thread);
   
   RemoveEntryList(&Thread->Tcb.Entry);
   HalReleaseTask(Thread);
   ObDereferenceObject(Thread);
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


NTSTATUS
STDCALL
NtRegisterThreadTerminatePort (
	HANDLE	TerminationPort
	)
{
	UNIMPLEMENTED;
}
