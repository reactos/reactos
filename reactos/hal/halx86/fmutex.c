/* $Id: fmutex.c,v 1.5 2002/09/08 10:22:24 chorns Exp $
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
   ExAcquireFastMutexUnsafe(FastMutex);
}


VOID FASTCALL
ExReleaseFastMutex (PFAST_MUTEX	FastMutex)
{
  ExReleaseFastMutexUnsafe(FastMutex);
  KeLeaveCriticalRegion();
}


BOOLEAN FASTCALL
ExTryToAcquireFastMutex (PFAST_MUTEX FastMutex)
{
  KeEnterCriticalRegion();
  if (InterlockedExchange(&FastMutex->Count, 0) == 1)
    {
      FastMutex->Owner = KeGetCurrentThread();
      return(TRUE);
    }
  else
    {
      KeLeaveCriticalRegion();
      return(FALSE);
    }
}

/* EOF */
