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
/* $Id: sem.c,v 1.11 2002/09/08 10:23:29 chorns Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/sem.c
 * PURPOSE:         Implements kernel semaphores
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/id.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID STDCALL 
KeInitializeSemaphore (PKSEMAPHORE	Semaphore,
		       LONG		Count,
		       LONG		Limit)
{
   KeInitializeDispatcherHeader(&Semaphore->Header,
				InternalSemaphoreType,
				sizeof(KSEMAPHORE)/sizeof(ULONG),
				Count);
   Semaphore->Limit=Limit;
}

LONG STDCALL 
KeReadStateSemaphore (PKSEMAPHORE	Semaphore)
{
   return(Semaphore->Header.SignalState);
}

LONG STDCALL 
KeReleaseSemaphore (PKSEMAPHORE	Semaphore,
		    KPRIORITY	Increment,
		    LONG		Adjustment,
		    BOOLEAN		Wait)
/*
 * FUNCTION: KeReleaseSemaphore releases a given semaphore object. This
 * routine supplies a runtime priority boost for waiting threads. If this
 * call sets the semaphore to the Signaled state, the semaphore count is
 * augmented by the given value. The caller can also specify whether it
 * will call one of the KeWaitXXX routines as soon as KeReleaseSemaphore
 * returns control.
 * ARGUMENTS:
 *       Semaphore = Points to an initialized semaphore object for which the
 *                   caller provides the storage.
 *       Increment = Specifies the priority increment to be applied if
 *                   releasing the semaphore causes a wait to be 
 *                   satisfied.
 *       Adjustment = Specifies a value to be added to the current semaphore
 *                    count. This value must be positive
 *       Wait = Specifies whether the call to KeReleaseSemaphore is to be
 *              followed immediately by a call to one of the KeWaitXXX.
 * RETURNS: If the return value is zero, the previous state of the semaphore
 *          object is Not-Signaled.
 */
{
   ULONG InitialState;
  
   DPRINT("KeReleaseSemaphore(Semaphore %x, Increment %d, Adjustment %d, "
	  "Wait %d)\n", Semaphore, Increment, Adjustment, Wait);
   
   KeAcquireDispatcherDatabaseLock(Wait);
   
   InitialState = Semaphore->Header.SignalState;
   if (Semaphore->Limit < InitialState + Adjustment ||
       InitialState > InitialState + Adjustment)
     {
	ExRaiseStatus(STATUS_SEMAPHORE_LIMIT_EXCEEDED);
     }
   
   Semaphore->Header.SignalState += Adjustment;
   if (InitialState == 0)
     {
       KeDispatcherObjectWake(&Semaphore->Header);
     }
   
  KeReleaseDispatcherDatabaseLock(Wait);
  return(InitialState);
}

/* EOF */
