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

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID KeInitializeMutex(PKMUTEX Mutex, ULONG Level)
{
   Mutex->Header.Type=2;
   Mutex->Header.SignalState=TRUE;
   Mutex->Header.Size = 8;
   Mutex->OwnerThread = NULL;
   Mutex->ApcDisable = 0;
   InitializeListHead(&Mutex->Header.WaitListHead);
}

LONG KeReadStateMutex(PKMUTEX Mutex)
{
   return(Mutex->Header.SignalState);
}

LONG KeReleaseMutex(PKMUTEX Mutex, BOOLEAN Wait)
{
   UNIMPLEMENTED;
}

NTSTATUS KeWaitForMutexObject(PKMUTEX Mutex,
			      KWAIT_REASON WaitReason,
			      KPROCESSOR_MODE WaitMode,
			      BOOLEAN Alertable,
			      PLARGE_INTEGER Timeout)
{
   return(KeWaitForSingleObject(Mutex,WaitReason,WaitMode,Alertable,Timeout));
}

