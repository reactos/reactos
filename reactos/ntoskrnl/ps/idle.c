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

#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

HANDLE PsIdleThreadHandle = NULL;
extern ULONG DpcQueueSize;
PETHREAD PiIdleThread;
extern CHAR KiTimerSystemAuditing;

/* FUNCTIONS *****************************************************************/

/** System idle thread procedure
 *
 */
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
/*
 *	Tell ke/timer.c it's okay to run.
 */
       KiTimerSystemAuditing = 1;

       NtYieldExecution();

       Ke386HaltProcessor();
     }
}


/** Initialization of system idle thread
 *
 */ 
VOID INIT_FUNCTION
PsInitIdleThread(VOID)
{
   KPRIORITY Priority;
   ULONG Affinity;
   NTSTATUS Status;
   
   Status = PsCreateSystemThread(&PsIdleThreadHandle,
			THREAD_ALL_ACCESS,
			NULL,
			NULL,
			NULL,
			PsIdleThreadMain,
			NULL);
   if(!NT_SUCCESS(Status)) {
	DPRINT("Couldn't create Idle System Thread!");
	KEBUGCHECK(0);
	return;
   }   

   Priority = LOW_PRIORITY;
   Status = NtSetInformationThread(PsIdleThreadHandle,
			  ThreadPriority,
			  &Priority,
			  sizeof(Priority));
   if(!NT_SUCCESS(Status)) {
	DPRINT("Couldn't set Priority to Idle System Thread!");
	return;
   }
   
   Affinity = 1 << 0;
   Status = NtSetInformationThread(PsIdleThreadHandle,
			  ThreadAffinityMask,
			  &Affinity,
			  sizeof(Affinity));
   if(!NT_SUCCESS(Status)) {
	DPRINT("Couldn't set Affinity Mask to Idle System Thread!");
   }   
}
