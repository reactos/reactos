/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/fmutex.c
 * PURPOSE:         Implements fast mutexes
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID ExAcquireFastMutex(PFAST_MUTEX FastMutex)
{
   KeEnterCriticalRegion();
   ExAcquireFastMutexUnsafe(FastMutex);
}

VOID ExAcquireFastMutexUnsafe(PFAST_MUTEX FastMutex)
{
   if (InterlockedDecrement(&(FastMutex->Count))==0)
     {
	return;
     }
   FastMutex->Contention++;
   KeWaitForSingleObject(&(FastMutex->Event),
			 Executive,
			 KernelMode,
			 FALSE,
			 NULL);
   FastMutex->Owner=KeGetCurrentThread();
}

VOID ExInitializeFastMutex(PFAST_MUTEX FastMutex)
{
   FastMutex->Count=1;
   FastMutex->Owner=NULL;
   FastMutex->Contention=0;
   KeInitializeEvent(&(FastMutex->Event),
		     SynchronizationEvent,
		     FALSE);
}

VOID ExReleaseFastMutexUnsafe(PFAST_MUTEX FastMutex)
{
   assert(FastMutex->Owner == KeGetCurrentThread());
   FastMutex->Owner=NULL;
   if (InterlockedIncrement(&(FastMutex->Count))<=0)
     {
	return;
     }      
   KeSetEvent(&(FastMutex->Event),0,FALSE);
}

VOID ExReleaseFastMutex(PFAST_MUTEX FastMutex)
{
   ExReleaseFastMutexUnsafe(FastMutex);
   KeLeaveCriticalRegion();
}

BOOLEAN ExTryToAcquireFastMutex(PFAST_MUTEX FastMutex)
{
   UNIMPLEMENTED;
}



