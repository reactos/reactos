/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/event.c
 * PURPOSE:         Implements event
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <internal/kernel.h>
#include <internal/wait.h>

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

VOID KeClearEvent(PKEVENT Event)
{
   Event->Header.SignalState=FALSE;   // (??) Is this atomic
}

VOID KeInitializeEvent(PKEVENT Event, EVENT_TYPE Type, BOOLEAN State)
{
   Event->Header.Type = Type;
   Event->Header.Absolute = 0;
   Event->Header.Inserted = 0;
   Event->Header.Size = sizeof(KEVENT) / sizeof(ULONG);
   Event->Header.SignalState = State;
   InitializeListHead(&(Event->Header.WaitListHead));
}

LONG KeReadStateEvent(PKEVENT Event)
{
   return(Event->Header.SignalState);
}

LONG KeResetEvent(PKEVENT Event)
{
   return(InterlockedExchange(&(Event->Header.SignalState),0));
}

LONG KeSetEvent(PKEVENT Event, KPRIORITY Increment, BOOLEAN Wait)
{
   int ret;
   KIRQL oldlvl;
   
   KeAcquireSpinLock(&DispatcherDatabaseLock,&oldlvl);
   ret = InterlockedExchange(&(Event->Header.SignalState),1);
   KeDispatcherObjectWake((DISPATCHER_HEADER *)Event);
   KeReleaseSpinLock(&DispatcherDatabaseLock,oldlvl);
}
