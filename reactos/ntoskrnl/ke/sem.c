/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/sem.c
 * PURPOSE:         Implements kernel semaphores
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID KeInitializeSemaphore(PKSEMAPHORE Semaphore,
			   LONG Count,
			   LONG Limit)
{
   KeInitializeDispatcherHeader(&Semaphore->Header,
				InternalSemaphoreType,
				sizeof(KSEMAPHORE)/sizeof(ULONG),
				Count);
   Semaphore->Limit=Limit;
}

LONG KeReadStateSemaphore(PKSEMAPHORE Semaphore)
{
   return(Semaphore->Header.SignalState);
}

LONG KeReleaseSemaphore(PKSEMAPHORE Semaphore,
			KPRIORITY Increment,
			LONG Adjustment,
			BOOLEAN Wait)
/*
 * FUNCTION: KeReleaseSemaphore releases a given semaphore object. This
 * routine supplies a runtime priority boost for waiting threads. If this
 * call sets the semaphore to the Signaled state, the semaphore count is
 * augmented by the given value. The caller can also specify whether it
 * will call one of the KeWaitXXX routines as soon as KeReleaseSemaphore
 * returns control.
 * ARGUMENTS:
 *       Semaphore = Points to an initialized semaphore object for which the
 *                   caller provides the storage.
 *       Increment = Specifies the priority increment to be applied if
 *                   releasing the semaphore causes a wait to be 
 *                   satisfied.
 *       Adjustment = Specifies a value to be added to the current semaphore
 *                    count. This value must be positive
 *       Wait = Specifies whether the call to KeReleaseSemaphore is to be
 *              followed immediately by a call to one of the KeWaitXXX.
 * RETURNS: If the return value is zero, the previous state of the semaphore
 *          object is Not-Signaled.
 */
{
   ULONG initState = Semaphore->Header.SignalState;
  
   DPRINT("KeReleaseSemaphore(Semaphore %x, Increment %d, Adjustment %d, "
	  "Wait %d)\n", Semaphore, Increment, Adjustment, Wait);
   
   KeAcquireDispatcherDatabaseLock(Wait);
   
   if (Semaphore->Limit < initState+Adjustment
       || initState > initState+Adjustment)
     {
	ExRaiseStatus(STATUS_SEMAPHORE_LIMIT_EXCEEDED);
     }
   
   Semaphore->Header.SignalState+=Adjustment;
   DPRINT("initState %d\n", initState);
   if(initState == 0)
     {
	//  wake up SignalState waiters
	DPRINT("Waking waiters\n");
	KeDispatcherObjectWake(&Semaphore->Header);
     }
   
  KeReleaseDispatcherDatabaseLock(Wait);
  return initState;
}

