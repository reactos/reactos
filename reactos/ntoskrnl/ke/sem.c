/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/sem.c
 * PURPOSE:         Implements kernel semaphores
 * 
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID STDCALL
KeInitializeSemaphore (PKSEMAPHORE	Semaphore,
		       LONG		Count,
		       LONG		Limit)
{
   KeInitializeDispatcherHeader(&Semaphore->Header,
				InternalSemaphoreType,
				sizeof(KSEMAPHORE)/sizeof(ULONG),
				Count);
   Semaphore->Limit=Limit;
}

/*
 * @implemented
 */
LONG STDCALL
KeReadStateSemaphore (PKSEMAPHORE	Semaphore)
{
   return(Semaphore->Header.SignalState);
}

/*
 * @implemented
 */
LONG STDCALL
KeReleaseSemaphore (PKSEMAPHORE	Semaphore,
		    KPRIORITY	Increment,
		    LONG		Adjustment,
		    BOOLEAN		Wait)
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
  ULONG InitialState;
  KIRQL OldIrql;

  DPRINT("KeReleaseSemaphore(Semaphore %x, Increment %d, Adjustment %d, "
	  "Wait %d)\n", Semaphore, Increment, Adjustment, Wait);

  OldIrql = KeAcquireDispatcherDatabaseLock();

  InitialState = Semaphore->Header.SignalState;
  if (Semaphore->Limit < (LONG) InitialState + Adjustment ||
      InitialState > InitialState + Adjustment)
    {
      ExRaiseStatus(STATUS_SEMAPHORE_LIMIT_EXCEEDED);
    }

  Semaphore->Header.SignalState += Adjustment;
  if (InitialState == 0)
    {
      KiDispatcherObjectWake(&Semaphore->Header, SEMAPHORE_INCREMENT);
    }

  if (Wait == FALSE)
    {
      KeReleaseDispatcherDatabaseLock(OldIrql);
    }
  else
    {
      KTHREAD *Thread = KeGetCurrentThread();
      Thread->WaitNext = TRUE;
      Thread->WaitIrql = OldIrql;
    }

  return(InitialState);
}

/* EOF */
