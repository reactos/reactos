/* $Id: mutex.c,v 1.7 2000/07/06 14:34:50 dwelch Exp $
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
#include <internal/id.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID STDCALL KeInitializeMutex (PKMUTEX	Mutex,
				ULONG	Level)
{
   KeInitializeDispatcherHeader(&Mutex->Header,
				InternalMutexType,
				sizeof(KMUTEX) / sizeof(ULONG),
				1);
}

LONG STDCALL KeReadStateMutex (PKMUTEX	Mutex)
{
   return(Mutex->Header.SignalState);
}

LONG STDCALL KeReleaseMutex (PKMUTEX	Mutex,
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

NTSTATUS STDCALL KeWaitForMutexObject (PKMUTEX		Mutex,
				       KWAIT_REASON	WaitReason,
				       KPROCESSOR_MODE	WaitMode,
				       BOOLEAN		Alertable,
				       PLARGE_INTEGER	Timeout)
{
   return(KeWaitForSingleObject(Mutex,WaitReason,WaitMode,Alertable,Timeout));
}

/* EOF */
