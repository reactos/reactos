/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         ReactOS kernel
 * FILE:            ntoskrnl/ke/kqueue.c
 * PURPOSE:         Implement device queues
 * 
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net) - Several optimizations and implement
 *                                                    usage of Inserted flag + reformat and
 *                                                    add debug output.
 *                  David Welch (welch@mcmail.com)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 *
 * FUNCTION: Intializes a device queue
 * ARGUMENTS:
 *       DeviceQueue = Device queue to initialize
 */
VOID
STDCALL
KeInitializeDeviceQueue(IN PKDEVICE_QUEUE DeviceQueue)
{

    /* Initialize the Header */
    DeviceQueue->Type = DeviceQueueObject;
    DeviceQueue->Size = sizeof(KDEVICE_QUEUE);
    
    /* Initialize the Listhead and Spinlock */
    InitializeListHead(&DeviceQueue->DeviceListHead);
    KeInitializeSpinLock(&DeviceQueue->Lock);
    
    /* Set it as busy */
    DeviceQueue->Busy=FALSE;
}

/*
 * @implemented
 *
 * FUNCTION: Inserts an entry in a device queue
 * ARGUMENTS:
 *        DeviceQueue = Queue to insert the entry in
 *        DeviceQueueEntry = Entry to insert
 * RETURNS: False is the device queue wasn't busy
 *          True otherwise
 */
BOOLEAN
STDCALL
KeInsertDeviceQueue(IN PKDEVICE_QUEUE DeviceQueue,
                    IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry)
{
    BOOLEAN Inserted;
    
    DPRINT("KeInsertDeviceQueue(DeviceQueue %x)\n", DeviceQueue);
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    /* Lock the Queue */
    KeAcquireSpinLockAtDpcLevel(&DeviceQueue->Lock);
   
    if (!DeviceQueue->Busy) {
        
        /* Set it as busy */
        Inserted = FALSE;
        DeviceQueue->Busy = TRUE;
    
    } else {

        /* Insert it into the list */
        Inserted = TRUE;        
        InsertTailList(&DeviceQueue->DeviceListHead, 
                       &DeviceQueueEntry->DeviceListEntry);
    
    
    }
   
    /* Sert the Insert state into the entry */
    DeviceQueueEntry->Inserted = Inserted;
   
    /* Release lock and return */
    KeReleaseSpinLockFromDpcLevel(&DeviceQueue->Lock);
    return Inserted;
}

/*
 * @implemented
 */
BOOLEAN 
STDCALL
KeInsertByKeyDeviceQueue(IN PKDEVICE_QUEUE DeviceQueue,
                         IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry,
                         IN ULONG SortKey)
{
    BOOLEAN Inserted;
    
    DPRINT("KeInsertByKeyDeviceQueue(DeviceQueue %x)\n", DeviceQueue);
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    /* Acquire the Lock */
    KeAcquireSpinLockAtDpcLevel(&DeviceQueue->Lock);
    
    /* Set the Sort Key */
    DeviceQueueEntry->SortKey=SortKey;
   
    if (!DeviceQueue->Busy) {
        
        DeviceQueue->Busy=TRUE;
        Inserted = FALSE;
  
    } else {
  
        /* Insert new entry after the last entry with SortKey less or equal to passed-in SortKey */
        InsertAscendingListFIFO(&DeviceQueue->DeviceListHead,
                                KDEVICE_QUEUE_ENTRY,
                                DeviceListEntry,
                                DeviceQueueEntry,
                                SortKey);
        Inserted = TRUE;
    }
   
    /* Reset the Inserted State */
    DeviceQueueEntry->Inserted = Inserted;
  
    /* Release Lock and Return */
    KeReleaseSpinLockFromDpcLevel(&DeviceQueue->Lock);
    return Inserted;
}

/*
 * @implemented
 *
 * FUNCTION: Removes an entry from a device queue
 * ARGUMENTS:
 *        DeviceQueue = Queue to remove the entry
 * RETURNS: The removed entry
 */
PKDEVICE_QUEUE_ENTRY
STDCALL
KeRemoveDeviceQueue(IN PKDEVICE_QUEUE	DeviceQueue)
{
    PLIST_ENTRY ListEntry;
    PKDEVICE_QUEUE_ENTRY ReturnEntry;
   
    DPRINT("KeRemoveDeviceQueue(DeviceQueue %x)\n", DeviceQueue);
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
   
    /* Acquire the Lock */
    KeAcquireSpinLockAtDpcLevel(&DeviceQueue->Lock);
    ASSERT(DeviceQueue->Busy);
   
    /* An attempt to remove an entry from an empty (and busy) queue sets the queue idle */
    if (IsListEmpty(&DeviceQueue->DeviceListHead)) {
        DeviceQueue->Busy = FALSE;
        ReturnEntry = NULL;
   
   } else {
   
        /* Remove the Entry from the List */
        ListEntry = RemoveHeadList(&DeviceQueue->DeviceListHead);
        ReturnEntry = CONTAINING_RECORD(ListEntry, 
                                        KDEVICE_QUEUE_ENTRY,
                                        DeviceListEntry);
        
        /* Set it as non-inserted */
        ReturnEntry->Inserted = FALSE;
    }

    /* Release lock and Return */
    KeReleaseSpinLockFromDpcLevel(&DeviceQueue->Lock);
    return ReturnEntry;
}

/*
 * @implemented
 */
PKDEVICE_QUEUE_ENTRY
STDCALL
KeRemoveByKeyDeviceQueue (IN PKDEVICE_QUEUE DeviceQueue,
                          IN ULONG SortKey)
{
    PLIST_ENTRY ListEntry;
    PKDEVICE_QUEUE_ENTRY ReturnEntry;

    DPRINT("KeRemoveByKeyDeviceQueue(DeviceQueue %x)\n", DeviceQueue);
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    
    /* Acquire the Lock */
    KeAcquireSpinLockAtDpcLevel(&DeviceQueue->Lock);
    ASSERT(DeviceQueue->Busy);

    /* An attempt to remove an entry from an empty (and busy) queue sets the queue idle */
    if (IsListEmpty(&DeviceQueue->DeviceListHead)) {
        
        DeviceQueue->Busy = FALSE;
        ReturnEntry = NULL;
   
   } else {
   
        /* Find entry with SortKey greater than or equal to the passed-in SortKey */
        ListEntry = DeviceQueue->DeviceListHead.Flink;
        while (ListEntry != &DeviceQueue->DeviceListHead) {

            /* Get Entry */
            ReturnEntry = CONTAINING_RECORD(ListEntry, 
                                            KDEVICE_QUEUE_ENTRY,
                                            DeviceListEntry);
            
            /* Check if keys match */
            if (ReturnEntry->SortKey >= SortKey) break;
            
            /* Move to next item */
            ListEntry = ListEntry->Flink;
        }
        
        /* Check if we found something */
        if (ListEntry == &DeviceQueue->DeviceListHead) {
        
            /*  Not found, return the first entry */
            ListEntry = RemoveHeadList(&DeviceQueue->DeviceListHead);
            ReturnEntry = CONTAINING_RECORD(ListEntry, 
                                            KDEVICE_QUEUE_ENTRY,
                                            DeviceListEntry);
        } else {
        
            /* We found it, so just remove it */
            RemoveEntryList(&ReturnEntry->DeviceListEntry);
        }
        
        /* Set it as non-inserted */
        ReturnEntry->Inserted = FALSE;
    }
   
    /* Release lock and Return */
    KeReleaseSpinLockFromDpcLevel(&DeviceQueue->Lock);
    return ReturnEntry;
}

/*
 * @unimplemented
 */
STDCALL
PKDEVICE_QUEUE_ENTRY
KeRemoveByKeyDeviceQueueIfBusy(IN PKDEVICE_QUEUE DeviceQueue,
                               IN ULONG SortKey)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @implemented
 */
BOOLEAN STDCALL
KeRemoveEntryDeviceQueue(IN PKDEVICE_QUEUE DeviceQueue,
                         IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry)
{
    KIRQL OldIrql;
    BOOLEAN OldState;
    
    DPRINT("KeRemoveEntryDeviceQueue(DeviceQueue %x)\n", DeviceQueue);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
  
    /* Acquire the Lock */
    KeAcquireSpinLock(&DeviceQueue->Lock, &OldIrql);
    
    /* Check/Set Old State */
    if ((OldState = DeviceQueueEntry->Inserted)) {
    
        /* Remove it */
        DeviceQueueEntry->Inserted = FALSE;
        RemoveEntryList(&DeviceQueueEntry->DeviceListEntry);
    }
  
    /* Unlock and return old state */
    KeReleaseSpinLock(&DeviceQueue->Lock, OldIrql);
    return OldState;
}
