/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/queue.c
 * PURPOSE:         Implements kernel queues
 * 
 * PROGRAMMERS:     Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/


/*
 * @implemented
 */
VOID STDCALL
KeInitializeQueue(IN PKQUEUE Queue,
		  IN ULONG Count OPTIONAL)
{
  KeInitializeDispatcherHeader(&Queue->Header,
			       QueueObject,
			       sizeof(KQUEUE)/sizeof(ULONG),
			       0);
  InitializeListHead(&Queue->EntryListHead);
  InitializeListHead(&Queue->ThreadListHead);
  Queue->CurrentCount = 0;
  Queue->MaximumCount = (Count == 0) ? (ULONG) KeNumberProcessors : Count;
}


/*
 * @implemented
 *
 * Returns number of entries in the queue
 */
LONG STDCALL
KeReadStateQueue(IN PKQUEUE Queue)
{
  return(Queue->Header.SignalState);
}

/*
 * Returns the previous number of entries in the queue
 */
LONG STDCALL
KiInsertQueue(
   IN PKQUEUE Queue,
   IN PLIST_ENTRY Entry,
   BOOLEAN Head
   )
{
   ULONG InitialState;
  
   DPRINT("KiInsertQueue(Queue %x, Entry %x)\n", Queue, Entry);
   
   InitialState = Queue->Header.SignalState;

   if (Head)
   {
      InsertHeadList(&Queue->EntryListHead, Entry);
   }
   else
   {
      InsertTailList(&Queue->EntryListHead, Entry);
   }
   
   //inc. num entries in queue
   Queue->Header.SignalState++;
   
   /* Why the KeGetCurrentThread()->Queue != Queue?
    * KiInsertQueue might be called from an APC for the current thread. 
    * -Gunnar
    */
   if (Queue->CurrentCount < Queue->MaximumCount &&
       !IsListEmpty(&Queue->Header.WaitListHead) &&
       KeGetCurrentThread()->Queue != Queue)
   {
      KiDispatcherObjectWake(&Queue->Header, IO_NO_INCREMENT);
   }

   return InitialState;
}



/*
 * @implemented
 */
LONG STDCALL
KeInsertHeadQueue(IN PKQUEUE Queue,
		  IN PLIST_ENTRY Entry)
{
   LONG Result;
   KIRQL OldIrql;
   
   OldIrql = KeAcquireDispatcherDatabaseLock();
   Result = KiInsertQueue(Queue,Entry,TRUE);
   KeReleaseDispatcherDatabaseLock(OldIrql);
   
   return Result;
}


/*
 * @implemented
 */
LONG STDCALL
KeInsertQueue(IN PKQUEUE Queue,
	      IN PLIST_ENTRY Entry)
{
   LONG Result;
   KIRQL OldIrql;
   
   OldIrql = KeAcquireDispatcherDatabaseLock();
   Result = KiInsertQueue(Queue,Entry,FALSE);
   KeReleaseDispatcherDatabaseLock(OldIrql);
   
   return Result;
}


/*
 * @implemented
 */
PLIST_ENTRY STDCALL
KeRemoveQueue(IN PKQUEUE Queue,
	      IN KPROCESSOR_MODE WaitMode,
	      IN PLARGE_INTEGER Timeout OPTIONAL)
{
   
   PLIST_ENTRY ListEntry;
   NTSTATUS Status;
   PKTHREAD Thread = KeGetCurrentThread();
   KIRQL OldIrql;

   OldIrql = KeAcquireDispatcherDatabaseLock ();

   if (Thread->Queue != Queue)
   {
      /*
       * INVESTIGATE: What is the Thread->QueueListEntry used for? It's linked it into the
       * Queue->ThreadListHead when the thread registers with the queue and unlinked when
       * the thread registers with a new queue. The Thread->Queue already tells us what
       * queue the thread is registered with.
       * -Gunnar
       */

      //unregister thread from previous queue (if any)
      if (Thread->Queue)
      {
         RemoveEntryList(&Thread->QueueListEntry);
         Thread->Queue->CurrentCount--;
         
         if (Thread->Queue->CurrentCount < Thread->Queue->MaximumCount && 
             !IsListEmpty(&Thread->Queue->EntryListHead))
         {
            KiDispatcherObjectWake(&Thread->Queue->Header, 0);
         }
      }

      // register thread with this queue
      InsertTailList(&Queue->ThreadListHead, &Thread->QueueListEntry);
      Thread->Queue = Queue;
   }
   else /* if (Thread->Queue == Queue) */
   {
      //dec. num running threads
      Queue->CurrentCount--;
   }
   
   
   
   
   while (TRUE)
   {
      if (Queue->CurrentCount < Queue->MaximumCount && !IsListEmpty(&Queue->EntryListHead))
      {
         ListEntry = RemoveHeadList(&Queue->EntryListHead);
         //dec. num entries in queue
         Queue->Header.SignalState--;
         //inc. num running threads
         Queue->CurrentCount++;
         
         KeReleaseDispatcherDatabaseLock(OldIrql);
         return ListEntry;
      }
      else
      {
         //inform KeWaitXxx that we are holding disp. lock
         Thread->WaitNext = TRUE;
         Thread->WaitIrql = OldIrql;

         Status = KeWaitForSingleObject(Queue,
                                        WrQueue,
                                        WaitMode,
                                        TRUE, //bAlertable
                                        Timeout);

         if (Status == STATUS_TIMEOUT || Status == STATUS_USER_APC)
         {
            return (PVOID)Status;
         }
         
         OldIrql = KeAcquireDispatcherDatabaseLock ();
      }
   }
}


/*
 * @implemented
 */
PLIST_ENTRY STDCALL
KeRundownQueue(IN PKQUEUE Queue)
{
   PLIST_ENTRY EnumEntry;
   PKTHREAD Thread;
   KIRQL OldIrql;

   DPRINT("KeRundownQueue(Queue %x)\n", Queue);

   /* I'm just guessing how this should work:-/
    * -Gunnar 
    */
     
   OldIrql = KeAcquireDispatcherDatabaseLock ();

   //no thread must wait on queue at rundown
   ASSERT(IsListEmpty(&Queue->Header.WaitListHead));
   
   // unlink threads and clear their Thread->Queue
   while (!IsListEmpty(&Queue->ThreadListHead))
   {
      EnumEntry = RemoveHeadList(&Queue->ThreadListHead);
      Thread = CONTAINING_RECORD(EnumEntry, KTHREAD, QueueListEntry);
      Thread->Queue = NULL;
   }

   if (IsListEmpty(&Queue->EntryListHead))
   {
      EnumEntry = NULL;
   }
   else
   {
      EnumEntry = Queue->EntryListHead.Flink;
   }

   KeReleaseDispatcherDatabaseLock (OldIrql);

   return EnumEntry;
}

/* EOF */
