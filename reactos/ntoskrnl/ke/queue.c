/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002 ReactOS Team
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
/* $Id: queue.c,v 1.6 2003/06/07 11:34:36 chorns Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/queue.c
 * PURPOSE:         Implements kernel queues
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 * UPDATE HISTORY:
 *                  Created 04/01/2002
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/id.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID STDCALL
KeInitializeQueue(IN PKQUEUE Queue,
		  IN ULONG Count OPTIONAL)
{
  KeInitializeDispatcherHeader(&Queue->Header,
			       InternalQueueType,
			       sizeof(KQUEUE)/sizeof(ULONG),
			       0);
  InitializeListHead(&Queue->EntryListHead);
  InitializeListHead(&Queue->ThreadListHead);
  Queue->CurrentCount = 0;
  Queue->MaximumCount = (Count == 0) ? (ULONG) KeNumberProcessors : Count;
}


LONG STDCALL
KeReadStateQueue(IN PKQUEUE Queue)
{
  return(Queue->Header.SignalState);
}


LONG STDCALL
KiInsertQueue(
   IN PKQUEUE Queue,
   IN PLIST_ENTRY Entry,
   BOOLEAN Head
   )
{
   ULONG InitialState;
  
   DPRINT("KiInsertQueue(Queue %x, Entry %x)\n", Queue, Entry);
   
   KeAcquireDispatcherDatabaseLock(FALSE);
   
   InitialState = Queue->Header.SignalState;
   Queue->Header.SignalState++;
   
   if (Head)
   {
      InsertHeadList(&Queue->EntryListHead, Entry);
   }
   else
   {
      InsertTailList(&Queue->EntryListHead, Entry);
   }

   if (Queue->CurrentCount < Queue->MaximumCount && InitialState == 0)
   {
      KeDispatcherObjectWake(&Queue->Header);
   }

   KeReleaseDispatcherDatabaseLock(FALSE);
   return InitialState;
}



LONG STDCALL
KeInsertHeadQueue(IN PKQUEUE Queue,
		  IN PLIST_ENTRY Entry)
{
   return KiInsertQueue(Queue,Entry,TRUE);
}


LONG STDCALL
KeInsertQueue(IN PKQUEUE Queue,
	      IN PLIST_ENTRY Entry)
{
   return KiInsertQueue(Queue,Entry,FALSE);
}


PLIST_ENTRY STDCALL
KeRemoveQueue(IN PKQUEUE Queue,
	      IN KPROCESSOR_MODE WaitMode,
	      IN PLARGE_INTEGER Timeout OPTIONAL)
{
   PLIST_ENTRY ListEntry;
   NTSTATUS Status;
   PKTHREAD Thread = KeGetCurrentThread();

   KeAcquireDispatcherDatabaseLock(FALSE);

   //assiciate new thread with queue?
   if (Thread->Queue != Queue)
   {
      //remove association from other queue
      if (!IsListEmpty(&Thread->QueueListEntry))
      {
         RemoveEntryList(&Thread->QueueListEntry);
      }

      //associate with this queue
      InsertHeadList(&Queue->ThreadListHead, &Thread->QueueListEntry);
      Queue->CurrentCount++;
      Thread->Queue = Queue;
   }
   
   if (Queue->CurrentCount <= Queue->MaximumCount && !IsListEmpty(&Queue->EntryListHead))
   {
      ListEntry = RemoveHeadList(&Queue->EntryListHead);
      Queue->Header.SignalState--;
      KeReleaseDispatcherDatabaseLock(FALSE);
      return ListEntry;
   }

   //need to wait for it...
   KeReleaseDispatcherDatabaseLock(FALSE);

   Status = KeWaitForSingleObject(Queue,
                                  WrQueue,
                                  WaitMode,
                                  TRUE,//Alertable,
                                  Timeout);

   if (Status == STATUS_TIMEOUT || Status == STATUS_USER_APC)
   {
      return (PVOID)Status;
   }
   else
   {
      KeAcquireDispatcherDatabaseLock(FALSE);
      ListEntry = RemoveHeadList(&Queue->EntryListHead);
      KeReleaseDispatcherDatabaseLock(FALSE);
      return ListEntry;
   }

}


PLIST_ENTRY STDCALL
KeRundownQueue(IN PKQUEUE Queue)
{
   PLIST_ENTRY EnumEntry;
   PKTHREAD Thread;

   DPRINT("KeRundownQueue(Queue %x)\n", Queue);

   //FIXME: should we wake thread waiting on a queue? 

   KeAcquireDispatcherDatabaseLock(FALSE);

   // Clear Queue and QueueListEntry members of all threads associated with this queue
   while (!IsListEmpty(&Queue->ThreadListHead))
   {
      EnumEntry = RemoveHeadList(&Queue->ThreadListHead);
      InitializeListHead(EnumEntry);
      Thread = CONTAINING_RECORD(EnumEntry, KTHREAD, QueueListEntry);
      Thread->Queue = NULL;
   }

   if (!IsListEmpty(&Queue->EntryListHead))
      EnumEntry = Queue->EntryListHead.Flink;
   else
      EnumEntry = NULL;

   KeReleaseDispatcherDatabaseLock(FALSE);

   return EnumEntry;
}

/* EOF */
