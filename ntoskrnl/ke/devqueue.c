/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/devqueue.c
 * PURPOSE:         Implement device queues
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID
NTAPI
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
 */
BOOLEAN
NTAPI
KeInsertDeviceQueue(IN PKDEVICE_QUEUE DeviceQueue,
                    IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry)
{
    KLOCK_QUEUE_HANDLE DeviceLock;
    BOOLEAN Inserted;
    ASSERT_DEVICE_QUEUE(DeviceQueue);

    DPRINT("KeInsertDeviceQueue() DevQueue %p, Entry %p\n", DeviceQueue, DeviceQueueEntry);

    /* Lock the queue */
    KiAcquireDeviceQueueLock(DeviceQueue, &DeviceLock);

    /* Check if it's not busy */
    if (!DeviceQueue->Busy)
    {
        /* Set it as busy */
        Inserted = FALSE;
        DeviceQueue->Busy = TRUE;
    }
    else
    {
        /* Insert it into the list */
        Inserted = TRUE;
        InsertTailList(&DeviceQueue->DeviceListHead,
                       &DeviceQueueEntry->DeviceListEntry);
    }

    /* Sert the Insert state into the entry */
    DeviceQueueEntry->Inserted = Inserted;

    /* Release the lock */
    KiReleaseDeviceQueueLock(&DeviceLock);

    /* Return the state */
    return Inserted;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeInsertByKeyDeviceQueue(IN PKDEVICE_QUEUE DeviceQueue,
                         IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry,
                         IN ULONG SortKey)
{
    KLOCK_QUEUE_HANDLE DeviceLock;
    BOOLEAN Inserted;
    ASSERT_DEVICE_QUEUE(DeviceQueue);

    DPRINT("KeInsertByKeyDeviceQueue() DevQueue %p, Entry %p, SortKey 0x%x\n", DeviceQueue, DeviceQueueEntry, SortKey);

    /* Lock the queue */
    KiAcquireDeviceQueueLock(DeviceQueue, &DeviceLock);

    /* Set the Sort Key */
    DeviceQueueEntry->SortKey = SortKey;

    /* Check if it's not busy */
    if (!DeviceQueue->Busy)
    {
        /* Set it as busy */
        Inserted = FALSE;
        DeviceQueue->Busy = TRUE;
    }
    else
    {
        /* Insert new entry after the last entry with SortKey less or equal to passed-in SortKey */
        InsertAscendingListFIFO(&DeviceQueue->DeviceListHead,
                                DeviceQueueEntry,
                                KDEVICE_QUEUE_ENTRY,
                                DeviceListEntry,
                                SortKey);
        Inserted = TRUE;
    }

    /* Release the lock */
    KiReleaseDeviceQueueLock(&DeviceLock);

    /* Return the state */
    return Inserted;
}

/*
 * @implemented
 */
PKDEVICE_QUEUE_ENTRY
NTAPI
KeRemoveDeviceQueue(IN PKDEVICE_QUEUE	DeviceQueue)
{
    PLIST_ENTRY ListEntry;
    PKDEVICE_QUEUE_ENTRY ReturnEntry;
    KLOCK_QUEUE_HANDLE DeviceLock;
    ASSERT_DEVICE_QUEUE(DeviceQueue);

    DPRINT("KeRemoveDeviceQueue() DevQueue %p\n", DeviceQueue);

    /* Lock the queue */
    KiAcquireDeviceQueueLock(DeviceQueue, &DeviceLock);
    ASSERT(DeviceQueue->Busy);

    /* Check if this is an empty queue */
    if (IsListEmpty(&DeviceQueue->DeviceListHead))
    {
        /* Set it to idle and return nothing*/
        DeviceQueue->Busy = FALSE;
        ReturnEntry = NULL;
    }
    else
    {
        /* Remove the Entry from the List */
        ListEntry = RemoveHeadList(&DeviceQueue->DeviceListHead);
        ReturnEntry = CONTAINING_RECORD(ListEntry,
                                        KDEVICE_QUEUE_ENTRY,
                                        DeviceListEntry);

        /* Set it as non-inserted */
        ReturnEntry->Inserted = FALSE;
    }

    /* Release the lock */
    KiReleaseDeviceQueueLock(&DeviceLock);

    /* Return the entry */
    return ReturnEntry;
}

/*
 * @implemented
 */
PKDEVICE_QUEUE_ENTRY
NTAPI
KeRemoveByKeyDeviceQueue(IN PKDEVICE_QUEUE DeviceQueue,
                         IN ULONG SortKey)
{
    PLIST_ENTRY ListEntry;
    PKDEVICE_QUEUE_ENTRY ReturnEntry;
    KLOCK_QUEUE_HANDLE DeviceLock;
    ASSERT_DEVICE_QUEUE(DeviceQueue);

    DPRINT("KeRemoveByKeyDeviceQueue() DevQueue %p, SortKey 0x%x\n", DeviceQueue, SortKey);

    /* Lock the queue */
    KiAcquireDeviceQueueLock(DeviceQueue, &DeviceLock);
    ASSERT(DeviceQueue->Busy);

    /* Check if this is an empty queue */
    if (IsListEmpty(&DeviceQueue->DeviceListHead))
    {
        /* Set it to idle and return nothing*/
        DeviceQueue->Busy = FALSE;
        ReturnEntry = NULL;
    }
    else
    {
        /* If SortKey is greater than the last key, then return the first entry right away */
        ListEntry = &DeviceQueue->DeviceListHead;
        ReturnEntry = CONTAINING_RECORD(ListEntry->Blink,
                                        KDEVICE_QUEUE_ENTRY,
                                        DeviceListEntry);

        if (SortKey >= ReturnEntry->SortKey)
        {
            ReturnEntry = CONTAINING_RECORD(ListEntry->Flink,
                                            KDEVICE_QUEUE_ENTRY,
                                            DeviceListEntry);

            /* Remove it from the list */
            RemoveEntryList(&ReturnEntry->DeviceListEntry);
        }
        else
        {
            /* Find entry with SortKey greater than or equal to the passed-in SortKey */
            LIST_FOR_EACH(ReturnEntry, &DeviceQueue->DeviceListHead, KDEVICE_QUEUE_ENTRY, DeviceListEntry) 
            {
                /* Check if keys match */
                if (ReturnEntry->SortKey >= SortKey)
                {
                    /* We found it, so just remove it */
                    RemoveEntryList(&ReturnEntry->DeviceListEntry);
                    break;
                }
            }

            /* Check if we found something */
            if (!ReturnEntry)
            {
                /*  Not found, return the first entry */
                ListEntry = RemoveHeadList(&DeviceQueue->DeviceListHead);
                ReturnEntry = CONTAINING_RECORD(ListEntry,
                                                KDEVICE_QUEUE_ENTRY,
                                                DeviceListEntry);
            }
        }

        /* Set it as non-inserted */
        ReturnEntry->Inserted = FALSE;
    }

    /* Release the lock */
    KiReleaseDeviceQueueLock(&DeviceLock);

    /* Return the entry */
    return ReturnEntry;
}

/*
 * @implemented
 */
PKDEVICE_QUEUE_ENTRY
NTAPI
KeRemoveByKeyDeviceQueueIfBusy(IN PKDEVICE_QUEUE DeviceQueue,
                               IN ULONG SortKey)
{
    PLIST_ENTRY ListEntry;
    PKDEVICE_QUEUE_ENTRY ReturnEntry;
    KLOCK_QUEUE_HANDLE DeviceLock;
    ASSERT_DEVICE_QUEUE(DeviceQueue);

    DPRINT("KeRemoveByKeyDeviceQueueIfBusy() DevQueue %p, SortKey 0x%x\n", DeviceQueue, SortKey);

    /* Lock the queue */
    KiAcquireDeviceQueueLock(DeviceQueue, &DeviceLock);

    /* Check if this is an empty or idle queue */
    if (!(DeviceQueue->Busy) || (IsListEmpty(&DeviceQueue->DeviceListHead)))
    {
        /* Set it to idle and return nothing*/
        DeviceQueue->Busy = FALSE;
        ReturnEntry = NULL;
    }
    else
    {
        /* Find entry with SortKey greater than or equal to the passed-in SortKey */
        LIST_FOR_EACH(ReturnEntry, &DeviceQueue->DeviceListHead, KDEVICE_QUEUE_ENTRY, DeviceListEntry) 
        {
            /* Check if keys match */
            if (ReturnEntry->SortKey >= SortKey)
            {
               /* We found it, so just remove it */
               RemoveEntryList(&ReturnEntry->DeviceListEntry);
               break;
            }
        }

        /* Check if we found something */
        if (!ReturnEntry)
        {
            /*  Not found, return the first entry */
            ListEntry = RemoveHeadList(&DeviceQueue->DeviceListHead);
            ReturnEntry = CONTAINING_RECORD(ListEntry,
                                            KDEVICE_QUEUE_ENTRY,
                                            DeviceListEntry);
        }

        /* Set it as non-inserted */
        ReturnEntry->Inserted = FALSE;
    }

    /* Release the lock */
    KiReleaseDeviceQueueLock(&DeviceLock);

    /* Return the entry */
    return ReturnEntry;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeRemoveEntryDeviceQueue(IN PKDEVICE_QUEUE DeviceQueue,
                         IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry)
{
    BOOLEAN OldState;
    KLOCK_QUEUE_HANDLE DeviceLock;
    ASSERT_DEVICE_QUEUE(DeviceQueue);

    DPRINT("KeRemoveEntryDeviceQueue() DevQueue %p, Entry %p\n", DeviceQueue, DeviceQueueEntry);

    /* Lock the queue */
    KiAcquireDeviceQueueLock(DeviceQueue, &DeviceLock);
    ASSERT(DeviceQueue->Busy);

    /* Check the insertion state */
    OldState = DeviceQueueEntry->Inserted;
    if (OldState)
    {
        /* Remove it */
        DeviceQueueEntry->Inserted = FALSE;
        RemoveEntryList(&DeviceQueueEntry->DeviceListEntry);
    }

    /* Unlock and return old state */
    KiReleaseDeviceQueueLock(&DeviceLock);
    return OldState;
}
