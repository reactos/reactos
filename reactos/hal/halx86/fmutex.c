/* $Id: fmutex.c,v 1.1 2001/08/21 20:18:27 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/fmutex.c
 * PURPOSE:         Implements fast mutexes
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 *                  Eric Kohl (ekohl@rz-online.de)
 * UPDATE HISTORY:
 *                  Created 09/06/2000
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID FASTCALL
ExAcquireFastMutex (PFAST_MUTEX	FastMutex)
{
   KeEnterCriticalRegion();
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


VOID FASTCALL
ExReleaseFastMutex (PFAST_MUTEX	FastMutex)
{
   assert(FastMutex->Owner == KeGetCurrentThread());
   FastMutex->Owner=NULL;
   if (InterlockedIncrement(&(FastMutex->Count))<=0)
     {
	return;
     }
   KeSetEvent(&(FastMutex->Event),0,FALSE);

   KeLeaveCriticalRegion();
}


BOOLEAN FASTCALL
ExTryToAcquireFastMutex (PFAST_MUTEX	FastMutex)
{
   UNIMPLEMENTED;
}

/* EOF */
