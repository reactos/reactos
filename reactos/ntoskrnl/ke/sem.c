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

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID KeInitializeSemaphore(PKSEMAPHORE Semaphore,
			   LONG Count,
			   LONG Limit)
{
   KeInitializeDispatcherHeader(&Semaphore->Header,SemaphoreType,
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
{
 ULONG initState = Semaphore->Header.SignalState;
  KeAcquireDispatcherDatabaseLock(Wait);
  if(Semaphore->Limit < initState+Adjustment
      || initState > initState+Adjustment)
     ExRaiseStatus(STATUS_SEMAPHORE_LIMIT_EXCEEDED);
  Semaphore->Header.SignalState+=Adjustment;
  if(initState == 0)
  {
    //  wake up SignalState waiters
    KeDispatcherObjectWake(&Semaphore->Header)  ;
  }
  KeReleaseDispatcherDatabaseLock(Wait);
  return initState;
}

