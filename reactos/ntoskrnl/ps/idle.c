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

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

HANDLE IdleThreadHandle = NULL;
extern ULONG DpcQueueSize;

/* FUNCTIONS *****************************************************************/

static VOID PsIdleThreadMain(PVOID Context)
{
   KIRQL oldlvl;
   
   for(;;)
     {
        DPRINT("Idling.... DpcQueueSize %d\n",DpcQueueSize);
	if (DpcQueueSize > 0)
	  {
	     KeRaiseIrql(DISPATCH_LEVEL,&oldlvl);
	     KeDrainDpcQueue();
	     KeLowerIrql(oldlvl);
	  }
	ZwYieldExecution();
     }
}

VOID PsInitIdleThread(VOID)
{
   KPRIORITY Priority;
   
   PsCreateSystemThread(&IdleThreadHandle,
			THREAD_ALL_ACCESS,
			NULL,
			NULL,
			NULL,
			PsIdleThreadMain,
			NULL);
   
   Priority = THREAD_PRIORITY_IDLE;
   ZwSetInformationThread(IdleThreadHandle,
			  ThreadPriority,
			  &Priority,
			  sizeof(Priority));
}
