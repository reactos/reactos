/* $Id: fmutex.c,v 1.7 2000/07/06 14:34:49 dwelch Exp $
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

VOID FASTCALL EXPORTED ExAcquireFastMutexUnsafe (PFAST_MUTEX	FastMutex)
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

VOID FASTCALL EXPORTED ExReleaseFastMutexUnsafe (PFAST_MUTEX	FastMutex)
{
   assert(FastMutex->Owner == KeGetCurrentThread());
   FastMutex->Owner = NULL;
   if (InterlockedIncrement(&FastMutex->Count) <= 0)
     {
	return;
     }
   KeSetEvent(&FastMutex->Event, 0, FALSE);
}

/* EOF */
