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
/* $Id: mutex.c,v 1.10 2001/11/07 02:14:10 ekohl Exp $
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
#include <internal/ps.h>
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
  Mutex->MutantListEntry.Flink = NULL;
  Mutex->MutantListEntry.Blink = NULL;
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
  if (Mutex->OwnerThread != KeGetCurrentThread())
    {
      DbgPrint("THREAD_NOT_MUTEX_OWNER: Mutex %p\n", Mutex);
      KeBugCheck(0); /* THREAD_NOT_MUTEX_OWNER */
    }
  Mutex->Header.SignalState++;
  assert(Mutex->Header.SignalState <= 1);
  if (Mutex->Header.SignalState == 1)
    {
      Mutex->OwnerThread = NULL;
      if (Mutex->MutantListEntry.Flink && Mutex->MutantListEntry.Blink)
	RemoveEntryList(&Mutex->MutantListEntry);
      KeDispatcherObjectWake(&Mutex->Header);
    }
  KeReleaseDispatcherDatabaseLock(Wait);
  return(0);
}

NTSTATUS STDCALL
KeWaitForMutexObject(IN PKMUTEX Mutex,
		     IN KWAIT_REASON WaitReason,
		     IN KPROCESSOR_MODE WaitMode,
		     IN BOOLEAN Alertable,
		     IN PLARGE_INTEGER Timeout)
{
  return(KeWaitForSingleObject(Mutex,WaitReason,WaitMode,Alertable,Timeout));
}


VOID STDCALL
KeInitializeMutant(IN PKMUTANT Mutant,
		   IN BOOLEAN InitialOwner)
{
  if (InitialOwner == TRUE)
    {
      KeInitializeDispatcherHeader(&Mutant->Header,
				   InternalMutexType,
				   sizeof(KMUTANT) / sizeof(ULONG),
				   0);
      InsertTailList(&KeGetCurrentThread()->MutantListHead,
		     &Mutant->MutantListEntry);
      Mutant->OwnerThread = KeGetCurrentThread();
    }
  else
    {
      KeInitializeDispatcherHeader(&Mutant->Header,
				   InternalMutexType,
				   sizeof(KMUTANT) / sizeof(ULONG),
				   1);
      Mutant->MutantListEntry.Flink = NULL;
      Mutant->MutantListEntry.Blink = NULL;
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
		IN KPRIORITY Increment,
		IN BOOLEAN Abandon,
		IN BOOLEAN Wait)
{
  KeAcquireDispatcherDatabaseLock(Wait);
  if (Abandon == FALSE)
    {
      if (Mutant->OwnerThread != NULL && Mutant->OwnerThread != KeGetCurrentThread())
	{
	  DbgPrint("THREAD_NOT_MUTEX_OWNER: Mutant->OwnerThread %p CurrentThread %p\n",
		   Mutant->OwnerThread,
		   KeGetCurrentThread());
	  KeBugCheck(0); /* THREAD_NOT_MUTEX_OWNER */
	}
      Mutant->Header.SignalState++;
      assert(Mutant->Header.SignalState <= 1);
    }
  else
    {
      if (Mutant->OwnerThread != NULL)
	{
	  Mutant->Header.SignalState = 1;
	  Mutant->Abandoned = TRUE;
	}
    }

  if (Mutant->Header.SignalState == 1)
    {
      Mutant->OwnerThread = NULL;
      if (Mutant->MutantListEntry.Flink && Mutant->MutantListEntry.Blink)
	RemoveEntryList(&Mutant->MutantListEntry);
      KeDispatcherObjectWake(&Mutant->Header);
    }

  KeReleaseDispatcherDatabaseLock(Wait);
  return(0);
}

/* EOF */
