/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/fmutex.c
 * PURPOSE:         Implements fast mutexes
 * 
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID FASTCALL
ExAcquireFastMutexUnsafe(PFAST_MUTEX FastMutex)
{
  ASSERT(FastMutex->Owner != KeGetCurrentThread());
  InterlockedIncrementUL(&FastMutex->Contention);
  while (InterlockedExchange(&FastMutex->Count, 0) == 0)
     {
       KeWaitForSingleObject(&FastMutex->Event,
			     Executive,
			     KernelMode,
			     FALSE,
			     NULL);
     }
  InterlockedDecrementUL(&FastMutex->Contention);
  FastMutex->Owner = KeGetCurrentThread();
}

/*
 * @implemented
 */
VOID FASTCALL
ExReleaseFastMutexUnsafe(PFAST_MUTEX FastMutex)
{
  ASSERT(FastMutex->Owner == KeGetCurrentThread());
  FastMutex->Owner = NULL;
  InterlockedExchange(&FastMutex->Count, 1);
  if (FastMutex->Contention > 0)
    {
      KeSetEvent(&FastMutex->Event, 0, FALSE);
    }
}

/* EOF */
