/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PURPOSE:              ReactOS kernel
 * FILE:                 ntoskrnl/ke/kqueue.c
 * PURPOSE:              Implement device queues
 * PROGRAMMER:           David Welch (welch@mcmail.com)
 * REVISION HISTORY:
 *               08/07/98: Created
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/


/*
 * @implemented
 */
BOOLEAN STDCALL
KeInsertByKeyDeviceQueue (
  IN PKDEVICE_QUEUE		DeviceQueue,
  IN PKDEVICE_QUEUE_ENTRY	DeviceQueueEntry,
  IN ULONG			SortKey)
{
  DPRINT("KeInsertByKeyDeviceQueue()\n");
  assert(KeGetCurrentIrql() == DISPATCH_LEVEL);   
   
  DeviceQueueEntry->SortKey=SortKey;

  KeAcquireSpinLockAtDpcLevel(&DeviceQueue->Lock);
   
  if (!DeviceQueue->Busy)
  {
    DeviceQueue->Busy=TRUE;
    KeReleaseSpinLockFromDpcLevel(&DeviceQueue->Lock);
    return(FALSE);
  }
  
  /* Insert new entry after the last entry with SortKey less or equal to passed-in SortKey */
  InsertAscendingListFIFO(&DeviceQueue->DeviceListHead,
                          KDEVICE_QUEUE_ENTRY,
                          DeviceListEntry,
                          DeviceQueueEntry,
                          SortKey);
   
  KeReleaseSpinLockFromDpcLevel(&DeviceQueue->Lock);
  return(TRUE);
}

/*
 * @implemented
 */
PKDEVICE_QUEUE_ENTRY
STDCALL
KeRemoveByKeyDeviceQueue (
	IN PKDEVICE_QUEUE	DeviceQueue,
	IN ULONG		SortKey
	)
{
  PLIST_ENTRY current;
  PKDEVICE_QUEUE_ENTRY entry;   
   
  assert(KeGetCurrentIrql() == DISPATCH_LEVEL);
  assert(DeviceQueue!=NULL);
  assert(DeviceQueue->Busy);
   
  KeAcquireSpinLockAtDpcLevel(&DeviceQueue->Lock);
   
  /* an attempt to remove an entry from an empty (and busy) queue sets the queue to idle */
  if (IsListEmpty(&DeviceQueue->DeviceListHead))
  {
    DeviceQueue->Busy = FALSE;
    KeReleaseSpinLockFromDpcLevel(&DeviceQueue->Lock);
    return NULL;
  }
   
  /* find entry with SortKey greater than or equal to the passed-in SortKey */
  current = DeviceQueue->DeviceListHead.Flink;
  while (current != &DeviceQueue->DeviceListHead)
  {
    entry = CONTAINING_RECORD(current,KDEVICE_QUEUE_ENTRY,DeviceListEntry);
    if (entry->SortKey >= SortKey)
    {
       RemoveEntryList(current);
       KeReleaseSpinLockFromDpcLevel(&DeviceQueue->Lock);
       return entry;
    }
    current = current->Flink;
  }
  
  /* if we didn't find a match, return the first entry */
  current = RemoveHeadList(&DeviceQueue->DeviceListHead);
  KeReleaseSpinLockFromDpcLevel(&DeviceQueue->Lock);

  return CONTAINING_RECORD(current,KDEVICE_QUEUE_ENTRY,DeviceListEntry);
}

/*
 * @implemented
 */
PKDEVICE_QUEUE_ENTRY
STDCALL
KeRemoveDeviceQueue (
  IN PKDEVICE_QUEUE	DeviceQueue)
/*
 * FUNCTION: Removes an entry from a device queue
 * ARGUMENTS:
 *        DeviceQueue = Queue to remove the entry
 * RETURNS: The removed entry
 */
{
   PLIST_ENTRY list_entry;
   
   DPRINT("KeRemoveDeviceQueue(DeviceQueue %x)\n",DeviceQueue);
   
   assert(KeGetCurrentIrql() == DISPATCH_LEVEL);
   assert(DeviceQueue!=NULL);
   assert(DeviceQueue->Busy);
   
   KeAcquireSpinLockAtDpcLevel(&DeviceQueue->Lock);
   
   /* an attempt to remove an entry from an empty (and busy) queue sets the queue idle */
   if (IsListEmpty(&DeviceQueue->DeviceListHead))
   {
     DeviceQueue->Busy = FALSE;
     KeReleaseSpinLockFromDpcLevel(&DeviceQueue->Lock);
     return NULL;
   }
   
   list_entry = RemoveHeadList(&DeviceQueue->DeviceListHead);
   KeReleaseSpinLockFromDpcLevel(&DeviceQueue->Lock);
   
   return CONTAINING_RECORD(list_entry,KDEVICE_QUEUE_ENTRY,DeviceListEntry);
}

/*
 * @implemented
 */
VOID
STDCALL
KeInitializeDeviceQueue (
  IN PKDEVICE_QUEUE	DeviceQueue
	)
/*
 * FUNCTION: Intializes a device queue
 * ARGUMENTS:
 *       DeviceQueue = Device queue to initialize
 */
{
   assert(DeviceQueue!=NULL);
   InitializeListHead(&DeviceQueue->DeviceListHead);
   DeviceQueue->Busy=FALSE;
   KeInitializeSpinLock(&DeviceQueue->Lock);
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
KeInsertDeviceQueue (
  IN PKDEVICE_QUEUE		DeviceQueue,
  IN PKDEVICE_QUEUE_ENTRY	DeviceQueueEntry
	)
/*
 * FUNCTION: Inserts an entry in a device queue
 * ARGUMENTS:
 *        DeviceQueue = Queue to insert the entry in
 *        DeviceQueueEntry = Entry to insert
 * RETURNS: False is the device queue wasn't busy
 *          True otherwise
 */
{
  assert(KeGetCurrentIrql() == DISPATCH_LEVEL);
   
  KeAcquireSpinLockAtDpcLevel(&DeviceQueue->Lock);
   
  if (!DeviceQueue->Busy)
  {
    DeviceQueue->Busy=TRUE;
    KeReleaseSpinLockFromDpcLevel(&DeviceQueue->Lock);
    return(FALSE);
  }
   
  DeviceQueueEntry->SortKey=0;
  InsertTailList(&DeviceQueue->DeviceListHead, &DeviceQueueEntry->DeviceListEntry);
   
  KeReleaseSpinLockFromDpcLevel(&DeviceQueue->Lock);
  return(TRUE);
}


/*
 * @implemented
 */
BOOLEAN STDCALL
KeRemoveEntryDeviceQueue(
  IN PKDEVICE_QUEUE DeviceQueue,
  IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry)
{
  PLIST_ENTRY current;
  KIRQL oldIrql;
  PKDEVICE_QUEUE_ENTRY entry;
  
  assert(KeGetCurrentIrql() <= DISPATCH_LEVEL);
  
  KeAcquireSpinLock(&DeviceQueue->Lock, &oldIrql);
  
  current = DeviceQueue->DeviceListHead.Flink;
  while (current != &DeviceQueue->DeviceListHead)
  {
    entry = CONTAINING_RECORD(current, KDEVICE_QUEUE_ENTRY, DeviceListEntry);
    if (DeviceQueueEntry == entry)
    {
       RemoveEntryList(current);
       KeReleaseSpinLock(&DeviceQueue->Lock, oldIrql);
       /* entry was in the queue (but not anymore) */
       return TRUE;
    }
    current = current->Flink;
  }
  
  KeReleaseSpinLock(&DeviceQueue->Lock, oldIrql);
  
  /* entry wasn't in the queue */
  return FALSE;
}
