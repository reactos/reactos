/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/event.c
 * PURPOSE:         Implements events
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

VOID KeClearEvent(PKEVENT Event)
{
   Event->Header.SignalState=FALSE;   
}

VOID KeInitializeEvent(PKEVENT Event, EVENT_TYPE Type, BOOLEAN State)
{
   KeInitializeDispatcherHeader(&(Event->Header),Type,
				sizeof(Event)/sizeof(ULONG),State);
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

   DPRINT("KeSetEvent(Event %x, Wait %x)\n",Event,Wait);
   KeAcquireDispatcherDatabaseLock(Wait);
//   ret = InterlockedExchange(&(Event->Header.SignalState),1);
   Event->Header.SignalState=1;
   KeDispatcherObjectWake((DISPATCHER_HEADER *)Event);
   KeReleaseDispatcherDatabaseLock(Wait);
}
