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
/* $Id: fmutex.c,v 1.9 2001/03/16 18:11:21 dwelch Exp $
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
