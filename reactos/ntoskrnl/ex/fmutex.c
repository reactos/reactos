/* $Id: fmutex.c,v 1.5 2000/05/01 14:15:02 ea Exp $
 *
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

/* FIXME: in HAL */
VOID FASTCALL EXPORTED ExAcquireFastMutex(PFAST_MUTEX FastMutex)
{
   KeEnterCriticalRegion();
   ExAcquireFastMutexUnsafe(FastMutex);
}

VOID FASTCALL EXPORTED ExAcquireFastMutexUnsafe(PFAST_MUTEX FastMutex)
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

/* FIXME: convert it into a macro */
VOID FASTCALL ExInitializeFastMutex(PFAST_MUTEX FastMutex)
{
   FastMutex->Count=1;
   FastMutex->Owner=NULL;
   FastMutex->Contention=0;
   KeInitializeEvent(&(FastMutex->Event),
		     SynchronizationEvent,
		     FALSE);
}

VOID FASTCALL EXPORTED ExReleaseFastMutexUnsafe(PFAST_MUTEX FastMutex)
{
   assert(FastMutex->Owner == KeGetCurrentThread());
   FastMutex->Owner=NULL;
   if (InterlockedIncrement(&(FastMutex->Count))<=0)
     {
	return;
     }      
   KeSetEvent(&(FastMutex->Event),0,FALSE);
}

/* FIXME: in HAL */
VOID FASTCALL EXPORTED ExReleaseFastMutex(PFAST_MUTEX FastMutex)
{
   ExReleaseFastMutexUnsafe(FastMutex);
   KeLeaveCriticalRegion();
}


/* FIXME: in HAL */
BOOLEAN FASTCALL EXPORTED ExTryToAcquireFastMutex(PFAST_MUTEX FastMutex)
{
   UNIMPLEMENTED;
}


/* EOF */
