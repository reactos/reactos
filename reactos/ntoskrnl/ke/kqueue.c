/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PURPOSE:              ReactOS kernel
 * FILE:                 ntoskrnl/ke/device.c
 * PURPOSE:              Implement device queues
 * PROGRAMMER:           David Welch (welch@mcmail.com)
 * REVISION HISTORY:
 *               08/07/98: Created
 */

/* INCLUDES ****************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID InsertBeforeEntryInList(PLIST_ENTRY Head, PLIST_ENTRY After, 
			     PLIST_ENTRY Entry)
{
   if (After->Blink!=NULL)
     {
	After->Blink->Flink = Entry;
     }
   else
     {
	Head->Flink = Entry;
     }
   Entry->Blink = After->Blink;
   Entry->Flink = After;
   After->Blink = Entry;
}

BOOLEAN KeInsertByKeyDeviceQueue(PKDEVICE_QUEUE DeviceQueue,
				 PKDEVICE_QUEUE_ENTRY DeviceQueueEntry,
				 ULONG SortKey)
{
   KIRQL oldlvl;
   PLIST_ENTRY current;
   PKDEVICE_QUEUE_ENTRY entry;
   
   DeviceQueueEntry->Key=SortKey;

   KeAcquireSpinLock(&DeviceQueue->Lock,&oldlvl);
   
   if (!DeviceQueue->Busy)
     {
	DeviceQueue->Busy=TRUE;
	KeReleaseSpinLock(&DeviceQueue->Lock,oldlvl);
	return(FALSE);
     }
      
   current=DeviceQueue->ListHead.Flink;
   while (current!=(&DeviceQueue->ListHead))
     {
	entry = CONTAINING_RECORD(current,KDEVICE_QUEUE_ENTRY,Entry);
	if (entry->Key < SortKey)
	  {
	     InsertBeforeEntryInList(&DeviceQueue->ListHead,
				     &DeviceQueueEntry->Entry,current);
	     KeReleaseSpinLock(&DeviceQueue->Lock,oldlvl);
	     return(TRUE);
	  }
	current = current->Flink;
     }   
   InsertTailList(&DeviceQueue->ListHead,&DeviceQueueEntry->Entry);
   
   KeReleaseSpinLock(&DeviceQueue->Lock,oldlvl);
   return(TRUE);
}

PKDEVICE_QUEUE_ENTRY KeRemoveByKeyDeviceQueue(PKDEVICE_QUEUE DeviceQueue,
					      ULONG SortKey)
{
   KIRQL oldlvl;
   PLIST_ENTRY current;
   PKDEVICE_QUEUE_ENTRY entry;   
   
   assert_irql(DISPATCH_LEVEL);
   assert(DeviceQueue!=NULL);
   assert(DeviceQueue->Busy);
   
   KeAcquireSpinLock(&DeviceQueue->Lock,&oldlvl);
   
   if (IsListEmpty(&DeviceQueue->ListHead))
     {
	DeviceQueue->Busy=FALSE;
	KeReleaseSpinLock(&DeviceQueue->Lock,oldlvl);
	return(NULL);
     }
   
   current = DeviceQueue->ListHead.Flink;
   for(;;)
     {
	entry = CONTAINING_RECORD(current,KDEVICE_QUEUE_ENTRY,Entry);
	if (entry->Key < SortKey || current->Flink == NULL)
	  {
	     RemoveEntryList(current);
	     KeReleaseSpinLock(&DeviceQueue->Lock,oldlvl);
	     return(entry);
	  }
	current = current->Flink;
     }
}

PKDEVICE_QUEUE_ENTRY KeRemoveDeviceQueue(PKDEVICE_QUEUE DeviceQueue)
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
   
   list_entry = RemoveHeadList(&DeviceQueue->ListHead);
   if (list_entry==(&DeviceQueue->ListHead))
     {
	DeviceQueue->Busy=FALSE;
	KeReleaseSpinLock(&DeviceQueue->Lock,oldlvl);
	return(NULL);
     }
   KeReleaseSpinLock(&DeviceQueue->Lock,oldlvl);
   
   entry = CONTAINING_RECORD(list_entry,KDEVICE_QUEUE_ENTRY,Entry);
   return(entry);
}

VOID KeInitializeDeviceQueue(PKDEVICE_QUEUE DeviceQueue)
/*
 * FUNCTION: Intializes a device queue
 * ARGUMENTS:
 *       DeviceQueue = Device queue to initialize
 */
{
   assert(DeviceQueue!=NULL);
   InitializeListHead(&DeviceQueue->ListHead);
   DeviceQueue->Busy=FALSE;
   KeInitializeSpinLock(&DeviceQueue->Lock);
}

BOOLEAN KeInsertDeviceQueue(PKDEVICE_QUEUE DeviceQueue,
			    PKDEVICE_QUEUE_ENTRY DeviceQueueEntry)
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
   
   InsertTailList(&DeviceQueue->ListHead,&DeviceQueueEntry->Entry);
   DeviceQueueEntry->Key=0;
   
   KeReleaseSpinLock(&DeviceQueue->Lock,oldlvl);
   return(TRUE);
}
