/* $Id: fmutex.c,v 1.2 2001/12/20 03:56:08 dwelch Exp $
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
ExTryToAcquireFastMutex (PFAST_MUTEX	FastMutex)
{
   UNIMPLEMENTED;
}

/* EOF */
