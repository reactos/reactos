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

/* FUNCTIONS *****************************************************************/

static NTSTATUS PsIdleThreadMain(PVOID Context)
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
     }
}

VOID PsInitIdleThread(VOID)
{
   KPRIORITY Priority;
   
   PsCreateSystemThread(&PsIdleThreadHandle,
			THREAD_ALL_ACCESS,
			NULL,
			NULL,
			NULL,
			PsIdleThreadMain,
			NULL);
   
   Priority = LOW_PRIORITY;
   NtSetInformationThread(PsIdleThreadHandle,
			  ThreadPriority,
			  &Priority,
			  sizeof(Priority));
}
