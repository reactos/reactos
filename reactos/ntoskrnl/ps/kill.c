/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/kill.c
 * PURPOSE:         Terminating a thread
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* GLBOALS *******************************************************************/

extern ULONG PiNrThreads;

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL NtTerminateThread(IN HANDLE ThreadHandle,
				   IN NTSTATUS ExitStatus)
{
   return(ZwTerminateThread(ThreadHandle,ExitStatus));
}

NTSTATUS STDCALL ZwTerminateThread(IN HANDLE ThreadHandle, 
				   IN NTSTATUS ExitStatus)
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
	PsTerminateSystemThread(ExitStatus);
     }
   else
     {
	UNIMPLEMENTED;
     }
}

VOID PsReleaseThread(PETHREAD Thread)
{
   DPRINT("PsReleaseThread(Thread %x)\n",Thread);
   
   RemoveEntryList(&Thread->Tcb.Entry);
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
   KIRQL oldlvl;
   PETHREAD CurrentThread;
   
   PiNrThreads--;
   
   CurrentThread = PsGetCurrentThread();
   
   CurrentThread->ExitStatus = ExitStatus;
   
   DPRINT("terminating %x\n",CurrentThread);
   ObDereferenceObject(CurrentThread->ThreadsProcess);
   KeRaiseIrql(DISPATCH_LEVEL,&oldlvl);
   CurrentThread->Tcb.ThreadState = THREAD_STATE_TERMINATED;
   ZwYieldExecution();
   for(;;);
}

NTSTATUS STDCALL NtRegisterThreadTerminatePort(HANDLE TerminationPort)
{
   return(ZwRegisterThreadTerminatePort(TerminationPort));
}

NTSTATUS STDCALL ZwRegisterThreadTerminatePort(HANDLE TerminationPort)
{
   UNIMPLEMENTED;
}
