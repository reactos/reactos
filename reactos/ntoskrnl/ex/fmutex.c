/* $Id: fmutex.c,v 1.8 2000/12/23 02:37:39 dwelch Exp $
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

VOID FASTCALL EXPORTED 
ExAcquireFastMutexUnsafe (PFAST_MUTEX	FastMutex)
{
   if (InterlockedDecrement(&FastMutex->Count) == 0)
     {
	return;
     }
   FastMutex->Contention++;
   KeWaitForSingleObject(&FastMutex->Event,
			 Executive,
			 KernelMode,
			 FALSE,
			 NULL);
   FastMutex->Owner = KeGetCurrentThread();
}

VOID FASTCALL EXPORTED 
ExReleaseFastMutexUnsafe (PFAST_MUTEX	FastMutex)
{
   assert(FastMutex->Owner == KeGetCurrentThread());
   if (InterlockedIncrement(&FastMutex->Count) <= 0)
     {
	return;
     }
   FastMutex->Owner = NULL;
   KeSetEvent(&FastMutex->Event, 0, FALSE);
}

/* EOF */
