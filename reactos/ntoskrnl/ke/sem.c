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
long initState=Semaphore->Header.SignalState;
  if(Semaphore->Limit < initState+Adjustment
      || initState > initState+Adjustment)
     ExRaiseStatus(STATUS_SEMAPHORE_LIMIT_EXCEEDED);
  Semaphore->Header.SignalState+=Adjustment;
  if((initState == 0)
     && (Semaphore->Header.WaitListHead.Flink != &Semaphore->Header.WaitListHead))
  {
    //  wake up SignalState waiters
    while(Semaphore->Header.SignalState > 0
           &&  KeDispatcherObjectWakeOne(Semaphore->Header) ) ;
  }
  if (Wait)
  {
    // FIXME(2) : in this case, we must store somewhere that we have been here
    // and the functions KeWaitxxx must take care of this
  }
  return initState;
}

