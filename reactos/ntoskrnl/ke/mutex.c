/*
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

VOID KeInitializeMutex(PKMUTEX Mutex, ULONG Level)
{
   KeInitializeDispatcherHeader(&Mutex->Header,
				InternalMutexType,
				sizeof(KMUTEX) / sizeof(ULONG),
				TRUE);
}

LONG KeReadStateMutex(PKMUTEX Mutex)
{
   return(Mutex->Header.SignalState);
}

LONG KeReleaseMutex(PKMUTEX Mutex, BOOLEAN Wait)
{
   KeAcquireDispatcherDatabaseLock(Wait);
   KeDispatcherObjectWake(&Mutex->Header);
   KeReleaseDispatcherDatabaseLock(Wait);
   return(0);
}

NTSTATUS KeWaitForMutexObject(PKMUTEX Mutex,
			      KWAIT_REASON WaitReason,
			      KPROCESSOR_MODE WaitMode,
			      BOOLEAN Alertable,
			      PLARGE_INTEGER Timeout)
{
   return(KeWaitForSingleObject(Mutex,WaitReason,WaitMode,Alertable,Timeout));
}

