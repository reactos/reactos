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
/* $Id: mutex.c,v 1.9 2001/11/04 00:17:24 ekohl Exp $
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
KeInitializeMutex(IN PKMUTEX Mutex,
		  IN ULONG Level)
{
  KeInitializeDispatcherHeader(&Mutex->Header,
			       InternalMutexType,
			       sizeof(KMUTEX) / sizeof(ULONG),
			       1);
  Mutex->OwnerThread = NULL;
  Mutex->Abandoned = FALSE;
  Mutex->ApcDisable = 1;
}

LONG STDCALL
KeReadStateMutex(IN PKMUTEX Mutex)
{
  return(Mutex->Header.SignalState);
}

LONG STDCALL
KeReleaseMutex(IN PKMUTEX Mutex,
	       IN BOOLEAN Wait)
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
KeWaitForMutexObject(PKMUTEX		Mutex,
		     KWAIT_REASON	WaitReason,
		     KPROCESSOR_MODE	WaitMode,
		     BOOLEAN		Alertable,
		     PLARGE_INTEGER	Timeout)
{
  return(KeWaitForSingleObject(Mutex,WaitReason,WaitMode,Alertable,Timeout));
}


VOID STDCALL
KeInitializeMutant(IN PKMUTANT Mutant,
		   IN BOOLEAN InitialOwner)
{
  KeInitializeDispatcherHeader(&Mutant->Header,
			       InternalMutexType,
			       sizeof(KMUTANT) / sizeof(ULONG),
			       1);
  if (InitialOwner == TRUE)
    {
      Mutant->OwnerThread = KeGetCurrentThread();
    }
  else
    {
      Mutant->OwnerThread = NULL;
    }
  Mutant->Abandoned = FALSE;
  Mutant->ApcDisable = 0;
}

LONG STDCALL
KeReadStateMutant(IN PKMUTANT Mutant)
{
  return(Mutant->Header.SignalState);
}

LONG STDCALL
KeReleaseMutant(IN PKMUTANT Mutant,
		ULONG Param2,
		ULONG Param3,
		IN BOOLEAN Wait)
{
  KeAcquireDispatcherDatabaseLock(Wait);
  Mutant->Header.SignalState++;
  assert(Mutant->Header.SignalState <= 1);
  if (Mutant->Header.SignalState == 1)
    {
      KeDispatcherObjectWake(&Mutant->Header);
    }
  KeReleaseDispatcherDatabaseLock(Wait);
  return(0);
}

/* EOF */
