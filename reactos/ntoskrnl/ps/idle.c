/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/idle.c
 * PURPOSE:         Using idle time
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* GLOBALS *******************************************************************/

HANDLE PsIdleThreadHandle = NULL;
extern ULONG DpcQueueSize;
PETHREAD PiIdleThread;

/* FUNCTIONS *****************************************************************/

VOID STDCALL
PsIdleThreadMain(PVOID Context)
{
   KIRQL oldlvl;
   
   for(;;)
     {
       if (DpcQueueSize > 0)
	 {
	   KeRaiseIrql(DISPATCH_LEVEL,&oldlvl);
	   KiDispatchInterrupt();
	   KeLowerIrql(oldlvl);
	 }
       NtYieldExecution();
       __asm__( "hlt" );
     }
}

VOID PsInitIdleThread(VOID)
{
   KPRIORITY Priority;
   ULONG Affinity;
   NTSTATUS Status;

   PsCreateSystemThread(&PsIdleThreadHandle,
			THREAD_ALL_ACCESS,
			NULL,
			NULL,
			NULL,
			PsIdleThreadMain,
			NULL);

   Priority = LOW_PRIORITY;
   Status = NtSetInformationThread(PsIdleThreadHandle,
			  ThreadPriority,
			  &Priority,
			  sizeof(Priority));
   assertmsg(NT_SUCCESS(Status), ("NtSetInformationThread() failed with "
	"status 0x%.08x for ThreadPriority", Status));

   Affinity = 1 << 0;
   Status = NtSetInformationThread(PsIdleThreadHandle,
			  ThreadAffinityMask,
			  &Affinity,
			  sizeof(Affinity));
   assertmsg(NT_SUCCESS(Status), ("NtSetInformationThread() failed with "
	"status 0x%.08x for ThreadAffinityMask", Status));
}
