/* $Id: mutex.c,v 1.5 2000/06/04 19:50:12 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
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

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID
STDCALL
KeInitializeMutex (
	PKMUTEX	Mutex,
	ULONG	Level
	)
{
   KeInitializeDispatcherHeader(&Mutex->Header,
				InternalMutexType,
				sizeof(KMUTEX) / sizeof(ULONG),
				TRUE);
}

LONG
STDCALL
KeReadStateMutex (
	PKMUTEX	Mutex
	)
{
   return(Mutex->Header.SignalState);
}

LONG
STDCALL
KeReleaseMutex (
	PKMUTEX	Mutex,
	BOOLEAN	Wait
	)
{
   KeAcquireDispatcherDatabaseLock(Wait);
   Mutex->Header.SignalState--;
   assert(Mutex->Header.SignalState >= 0);
   if (Mutex->Header.SignalState == 0)
     {
	KeDispatcherObjectWake(&Mutex->Header);
     }
   KeReleaseDispatcherDatabaseLock(Wait);
   return(0);
}

NTSTATUS
STDCALL
KeWaitForMutexObject (
	PKMUTEX		Mutex,
	KWAIT_REASON	WaitReason,
	KPROCESSOR_MODE	WaitMode,
	BOOLEAN		Alertable,
	PLARGE_INTEGER	Timeout
	)
{
   return(KeWaitForSingleObject(Mutex,WaitReason,WaitMode,Alertable,Timeout));
}

/* EOF */
