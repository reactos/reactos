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
/* $Id: mutex.c,v 1.8 2001/04/09 02:45:04 dwelch Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/mutex.c
 * PURPOSE:         Implements mutex
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/id.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID STDCALL 
KeInitializeMutex (PKMUTEX	Mutex,
		   ULONG	Level)
{
   KeInitializeDispatcherHeader(&Mutex->Header,
				InternalMutexType,
				sizeof(KMUTEX) / sizeof(ULONG),
				1);
}

LONG STDCALL 
KeReadStateMutex (PKMUTEX	Mutex)
{
   return(Mutex->Header.SignalState);
}

LONG STDCALL 
KeReleaseMutex (PKMUTEX	Mutex,
		BOOLEAN	Wait)
{
   KeAcquireDispatcherDatabaseLock(Wait);
   Mutex->Header.SignalState++;
   assert(Mutex->Header.SignalState <= 1);
   if (Mutex->Header.SignalState == 1)
     {
	KeDispatcherObjectWake(&Mutex->Header);
     }
   KeReleaseDispatcherDatabaseLock(Wait);
   return(0);
}

NTSTATUS STDCALL 
KeWaitForMutexObject (PKMUTEX		Mutex,
		      KWAIT_REASON	WaitReason,
		      KPROCESSOR_MODE	WaitMode,
		      BOOLEAN		Alertable,
				       PLARGE_INTEGER	Timeout)
{
   return(KeWaitForSingleObject(Mutex,WaitReason,WaitMode,Alertable,Timeout));
}

/* EOF */
