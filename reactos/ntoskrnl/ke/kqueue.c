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

VOID 
InsertBeforeEntryInList(PLIST_ENTRY Head, PLIST_ENTRY After, PLIST_ENTRY Entry)
{
   InsertHeadList(After, Entry);
}

/*
 * @implemented
 */
BOOLEAN STDCALL
KeInsertByKeyDeviceQueue (PKDEVICE_QUEUE		DeviceQueue,
			  PKDEVICE_QUEUE_ENTRY	DeviceQueueEntry,
			  ULONG			SortKey)
{
   KIRQL oldlvl;
   PLIST_ENTRY current;
   PKDEVICE_QUEUE_ENTRY entry;
   
   DPRINT("KeInsertByKeyDeviceQueue()\n");
   
   DeviceQueueEntry->SortKey=SortKey;

   KeAcquireSpinLock(&DeviceQueue->Lock,&oldlvl);
   
   if (!DeviceQueue->Busy)
     {
	DeviceQueue->Busy=TRUE;
	KeReleaseSpinLock(&DeviceQueue->Lock,oldlvl);
	return(FALSE);
     }
      
   current=DeviceQueue->DeviceListHead.Flink;
   while (current!=(&DeviceQueue->DeviceListHead))
     {
	entry = CONTAINING_RECORD(current,KDEVICE_QUEUE_ENTRY,DeviceListEntry);
	if (entry->SortKey < SortKey)
	  {
	     InsertBeforeEntryInList(&DeviceQueue->DeviceListHead,
				     &DeviceQueueEntry->DeviceListEntry,
				     current);
	     KeReleaseSpinLock(&DeviceQueue->Lock,oldlvl);
	     return(TRUE);
	  }
	current = current->Flink;
     }   
   InsertTailList(&DeviceQueue->DeviceListHead,&DeviceQueueEntry->DeviceListEntry);
   
   KeReleaseSpinLock(&DeviceQueue->Lock,oldlvl);
   return(TRUE);
}

/*
 * @implemented
 */
PKDEVICE_QUEUE_ENTRY
STDCALL
KeRemoveByKeyDeviceQueue (
	PKDEVICE_QUEUE	DeviceQueue,
	ULONG		SortKey
	)
{
   KIRQL oldlvl;
   PLIST_ENTRY current;
   PKDEVICE_QUEUE_ENTRY entry;   
   
   assert_irql(DISPATCH_LEVEL);
   assert(DeviceQueue!=NULL);
   assert(DeviceQueue->Busy);
   
   KeAcquireSpinLock(&DeviceQueue->Lock,&oldlvl);
   
   current = DeviceQueue->DeviceListHead.Flink;
   while (current != &DeviceQueue->DeviceListHead)
     {
	entry = CONTAINING_RECORD(current,KDEVICE_QUEUE_ENTRY,DeviceListEntry);
	if (entry->SortKey < SortKey ||
	    current->Flink == &DeviceQueue->DeviceListHead)
	  {
	     RemoveEntryList(current);
	     KeReleaseSpinLock(&DeviceQueue->Lock,oldlvl);
	     return(entry);
	  }
	current = current->Flink;
     }
   DeviceQueue->Busy = FALSE;
   KeReleaseSpinLock(&DeviceQueue->Lock, oldlvl);
   return(NULL);
}

/*
 * @implemented
 */
PKDEVICE_QUEUE_ENTRY
STDCALL
KeRemoveDeviceQueue (
	PKDEVICE_QUEUE	DeviceQueue
	)
/*
 * FUNCTION: Removes an entry from a device queue
 * ARGUMENTS:
 *        DeviceQueue = Queue to remove the entry
 * RETURNS: The removed entry
 */
{
   KIRQL oldlvl;
   PLIST_ENTRY list_entry;
   PKDEVICE_QUEUE_ENTRY entry;
   
   DPRINT("KeRemoveDeviceQueue(DeviceQueue %x)\n",DeviceQueue);
   
   assert_irql(DISPATCH_LEVEL);
   assert(DeviceQueue!=NULL);
   assert(DeviceQueue->Busy);
   
   KeAcquireSpinLock(&DeviceQueue->Lock,&oldlvl);
   
   list_entry = RemoveHeadList(&DeviceQueue->DeviceListHead);
   if (list_entry==(&DeviceQueue->DeviceListHead))
     {
	DeviceQueue->Busy=FALSE;
	KeReleaseSpinLock(&DeviceQueue->Lock,oldlvl);
	return(NULL);
     }
   KeReleaseSpinLock(&DeviceQueue->Lock,oldlvl);
   
   entry = CONTAINING_RECORD(list_entry,KDEVICE_QUEUE_ENTRY,DeviceListEntry);
   return(entry);
}

/*
 * @implemented
 */
VOID
STDCALL
KeInitializeDeviceQueue (
	PKDEVICE_QUEUE	DeviceQueue
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
	PKDEVICE_QUEUE		DeviceQueue,
	PKDEVICE_QUEUE_ENTRY	DeviceQueueEntry
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
   KIRQL oldlvl;
   
   KeAcquireSpinLock(&DeviceQueue->Lock,&oldlvl);
   
   if (!DeviceQueue->Busy)
     {
	DeviceQueue->Busy=TRUE;
	KeReleaseSpinLock(&DeviceQueue->Lock,oldlvl);
	return(FALSE);
     }
   
   InsertTailList(&DeviceQueue->DeviceListHead,
		  &DeviceQueueEntry->DeviceListEntry);
   DeviceQueueEntry->SortKey=0;
   
   KeReleaseSpinLock(&DeviceQueue->Lock,oldlvl);
   return(TRUE);
}


/*
 * @unimplemented
 */
BOOLEAN STDCALL
KeRemoveEntryDeviceQueue(PKDEVICE_QUEUE DeviceQueue,
			 PKDEVICE_QUEUE_ENTRY DeviceQueueEntry)
{
  UNIMPLEMENTED;
  return(FALSE);
}
