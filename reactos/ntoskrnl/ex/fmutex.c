/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: fmutex.c,v 1.16 2002/09/07 15:12:50 chorns Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/fmutex.c
 * PURPOSE:         Implements fast mutexes
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * PORTABILITY:     Checked.
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* FUNCTIONS *****************************************************************/

VOID FASTCALL
ExAcquireFastMutexUnsafe(PFAST_MUTEX FastMutex)
{
  assert(FastMutex->Owner != KeGetCurrentThread());
  InterlockedIncrement(&FastMutex->Contention);
  while (InterlockedExchange(&FastMutex->Count, 0) == 0)
     {       
       KeWaitForSingleObject(&FastMutex->Event,
			     Executive,
			     KernelMode,
			     FALSE,
			     NULL);      
     }
  InterlockedDecrement(&FastMutex->Contention);
  FastMutex->Owner = KeGetCurrentThread();
}

VOID FASTCALL
ExReleaseFastMutexUnsafe(IN PFAST_MUTEX  FastMutex)
{
  assert(FastMutex->Owner == KeGetCurrentThread());
  FastMutex->Owner = NULL;
  if (FastMutex->Contention > 0)
    {
      KeSetEvent(&FastMutex->Event, 0, FALSE);
    }
  InterlockedExchange(&FastMutex->Count, 1);
}

/* EOF */
