/* $Id: suspend.c,v 1.1 2001/01/19 15:09:01 dwelch Exp $
 *
 * COPYRIGHT:              See COPYING in the top level directory
 * PROJECT:                ReactOS kernel
 * FILE:                   ntoskrnl/ps/suspend.c
 * PURPOSE:                Thread managment
 * PROGRAMMER:             David Welch (welch@mcmail.com)
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/ob.h>
#include <internal/hal.h>
#include <internal/ps.h>
#include <internal/ob.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

extern KSPIN_LOCK PiThreadListLock;
VOID PsInsertIntoThreadList(KPRIORITY Priority, PETHREAD Thread);

/* FUNCTIONS *****************************************************************/

#if 1
ULONG PsResumeThread(PETHREAD Thread)
{
   KIRQL oldIrql;
   ULONG SuspendCount;

   KeAcquireSpinLock(&PiThreadListLock, &oldIrql);

   if (Thread->Tcb.SuspendCount > 0)
     {
	Thread->Tcb.SuspendCount--;
	SuspendCount = Thread->Tcb.SuspendCount;
	Thread->Tcb.State = THREAD_STATE_RUNNABLE;
	PsInsertIntoThreadList(Thread->Tcb.Priority, Thread);
     }

   KeReleaseSpinLock(&PiThreadListLock, oldIrql);

   return SuspendCount;
}
#endif

#if 0
VOID
PiSuspendThreadRundownRoutine(PKAPC Apc)
{
   ExFreePool(Apc);
}

VOID
PiSuspendThreadKernelRoutine(PKAPC Apc,
			     PKNORMAL_ROUTINE* NormalRoutine,
			     PVOID* NormalContext,
			     PVOID* SystemArgument1,
			     PVOID* SystemArguemnt2)
{
   InterlockedIncrement(&PsGetCurrentThread()->Tcb.SuspendThread);
   KeWaitForSingleObject((PVOID)&PsGetCurrentThread()->SuspendSemaphore,
			 0,
			 UserMode,
			 TRUE,
			 NULL);
   ExFreePool(Apc);
}

NTSTATUS 
PsSuspendThread(PETHREAD Thread, PULONG SuspendCount)
{
   PKAPC Apc;
   
   Apc = ExAllocatePool(NonPagedPool, sizeof(KAPC));
   if (Apc == NULL)
     {
	return(STATUS_NO_MORE_MEMORY);
     }
   
   *SuspendCount = Thread->Tcb.SuspendCount;
   
   KeInitializeApc(Apc,
		   &Thread->Tcb,
		   NULL,
		   PiSuspendThreadKernelRoutine,
		   PiSuspendThreadRundownRoutine,
		   NULL,
		   KernelMode,
		   NULL);
   KeInsertQueueApc(Apc,
		    NULL,
		    NULL,
		    0);
   return(STATUS_SUCCESS);
}
#endif

#if 1
ULONG 
PsSuspendThread(PETHREAD Thread)
{
   KIRQL oldIrql;
   ULONG PreviousSuspendCount;

   KeAcquireSpinLock(&PiThreadListLock, &oldIrql);

   PreviousSuspendCount = Thread->Tcb.SuspendCount;
   if (Thread->Tcb.SuspendCount < MAXIMUM_SUSPEND_COUNT)
     {
	Thread->Tcb.SuspendCount++;
     }

   if (PsGetCurrentThread() == Thread)
     {
	DbgPrint("Cannot suspend self\n");
	KeBugCheck(0);
     }

   Thread->Tcb.State = THREAD_STATE_SUSPENDED;

   KeReleaseSpinLock(&PiThreadListLock, oldIrql);

   return PreviousSuspendCount;
}
#endif
