/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    devquobj.c

Abstract:

    This module implements the kernel device queue object. Functions are
    provided to initialize a device queue object and to insert and remove
    device queue entries in a device queue object.

Author:

    David N. Cutler (davec) 1-Apr-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// The following assert macro is used to check that an input device queue
// is really a kdevice_queue and not something else, like deallocated pool.
//

#define ASSERT_DEVICE_QUEUE(E) {            \
    ASSERT((E)->Type == DeviceQueueObject); \
}


VOID
KeInitializeDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue
    )

/*++

Routine Description:

    This function initializes a kernel device queue object.

Arguments:

    DeviceQueue - Supplies a pointer to a control object of type device
        queue.

    SpinLock - Supplies a pointer to an executive spin lock.

Return Value:

    None.

--*/

{

    //
    // Initialize standard control object header.
    //

    DeviceQueue->Type = DeviceQueueObject;
    DeviceQueue->Size = sizeof(KDEVICE_QUEUE);

    //
    // Initialize the device queue list head, spin lock, and busy indicator.
    //

    InitializeListHead(&DeviceQueue->DeviceListHead);
    KeInitializeSpinLock(&DeviceQueue->Lock);
    DeviceQueue->Busy = FALSE;
    return;
}

BOOLEAN
KeInsertDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry
    )

/*++

Routine Description:

    This function inserts a device queue entry at the tail of the specified
    device queue. If the device is not busy, then it is set busy and the entry
    is not placed in the device queue. Otherwise the specified entry is placed
    at the end of the device queue.

    N.B. This function can only be called from DISPATCH_LEVEL.

Arguments:

    DeviceQueue - Supplies a pointer to a control object of type device queue.

    DeviceQueueEntry - Supplies a pointer to a device queue entry.

Return Value:

    If the device is not busy, then a value of FALSE is returned. Otherwise a
    value of TRUE is returned.

--*/

{

    BOOLEAN Inserted;

    ASSERT_DEVICE_QUEUE(DeviceQueue);
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    //
    // Lock specified device queue.
    //

    KiAcquireSpinLock(&DeviceQueue->Lock);

    //
    // Insert the specified device queue entry at the end of the device queue
    // if the device queue is busy. Otherwise set the device queue busy and
    // don't insert the device queue entry.
    //

    if (DeviceQueue->Busy == TRUE) {
        Inserted = TRUE;
        InsertTailList(&DeviceQueue->DeviceListHead,
                       &DeviceQueueEntry->DeviceListEntry);
    } else {
        DeviceQueue->Busy = TRUE;
        Inserted = FALSE;
    }
    DeviceQueueEntry->Inserted = Inserted;
    KiReleaseSpinLock(&DeviceQueue->Lock);
    return Inserted;
}

BOOLEAN
KeInsertByKeyDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry,
    IN ULONG SortKey
    )

/*++

Routine Description:

    This function inserts a device queue entry into the specified device
    queue according to a sort key. If the device is not busy, then it is
    set busy and the entry is not placed in the device queue. Otherwise
    the specified entry is placed in the device queue at a position such
    that the specified sort key is greater than or equal to its predecessor
    and less than its successor.

    N.B. This function can only be called from DISPATCH_LEVEL.

Arguments:

    DeviceQueue - Supplies a pointer to a control object of type device queue.

    DeviceQueueEntry - Supplies a pointer to a device queue entry.

    SortKey - Supplies the sort key by which the position to insert the device
        queue entry is to be determined.

Return Value:

    If the device is not busy, then a value of FALSE is returned. Otherwise a
    value of TRUE is returned.

--*/

{

    BOOLEAN Inserted;
    PLIST_ENTRY NextEntry;
    PKDEVICE_QUEUE_ENTRY QueueEntry;

    ASSERT_DEVICE_QUEUE(DeviceQueue);
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    //
    // Lock specified device queue.
    //

    KiAcquireSpinLock(&DeviceQueue->Lock);

    //
    // Insert the specified device queue entry in the device queue at the
    // position specified by the sort key if the device queue is busy.
    // Otherwise set the device queue busy an don't insert the device queue
    // entry.
    //

    DeviceQueueEntry->SortKey = SortKey;
    if (DeviceQueue->Busy == TRUE) {
        Inserted = TRUE;
        NextEntry = DeviceQueue->DeviceListHead.Flink;
        while (NextEntry != &DeviceQueue->DeviceListHead) {
            QueueEntry = CONTAINING_RECORD(NextEntry, KDEVICE_QUEUE_ENTRY,
                                           DeviceListEntry);
            if (SortKey < QueueEntry->SortKey) {
                break;
            }
            NextEntry = NextEntry->Flink;
        }
        NextEntry = NextEntry->Blink;
        InsertHeadList(NextEntry, &DeviceQueueEntry->DeviceListEntry);
    } else {
        DeviceQueue->Busy = TRUE;
        Inserted = FALSE;
    }
    DeviceQueueEntry->Inserted = Inserted;
    KiReleaseSpinLock(&DeviceQueue->Lock);
    return Inserted;
}

PKDEVICE_QUEUE_ENTRY
KeRemoveDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue
    )

/*++

Routine Description:

    This function removes an entry from the head of the specified device
    queue. If the device queue is empty, then the device is set Not-Busy
    and a NULL pointer is returned. Otherwise the next entry is removed
    from the head of the device queue and the address of device queue entry
    is returned.

    N.B. This function can only be called from DISPATCH_LEVEL.

Arguments:

    DeviceQueue - Supplies a pointer to a control object of type device queue.

Return Value:

    A NULL pointer is returned if the device queue is empty. Otherwise a
    pointer to a device queue entry is returned.

--*/

{

    PKDEVICE_QUEUE_ENTRY DeviceQueueEntry;
    PLIST_ENTRY NextEntry;

    ASSERT_DEVICE_QUEUE(DeviceQueue);
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    //
    // Lock specified device queue.
    //

    KiAcquireSpinLock(&DeviceQueue->Lock);

    ASSERT(DeviceQueue->Busy == TRUE);

    //
    // If the device queue is not empty, then remove the first entry from
    // the queue. Otherwise set the device queue not busy.
    //

    if (IsListEmpty(&DeviceQueue->DeviceListHead) == TRUE) {
        DeviceQueue->Busy = FALSE;
        DeviceQueueEntry = (PKDEVICE_QUEUE_ENTRY)NULL;
    } else {
        NextEntry = RemoveHeadList(&DeviceQueue->DeviceListHead);
        DeviceQueueEntry = CONTAINING_RECORD(NextEntry, KDEVICE_QUEUE_ENTRY,
                                             DeviceListEntry);
        DeviceQueueEntry->Inserted = FALSE;
    }

    //
    // Release device queue spin lock and return address of device queue
    // entry.
    //

    KiReleaseSpinLock(&DeviceQueue->Lock);
    return DeviceQueueEntry;
}

PKDEVICE_QUEUE_ENTRY
KeRemoveByKeyDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue,
    IN ULONG SortKey
    )

/*++

Routine Description:

    This function removes an entry from the specified device
    queue. If the device queue is empty, then the device is set Not-Busy
    and a NULL pointer is returned. Otherwise the an entry is removed
    from the device queue and the address of device queue entry
    is returned.  The queue is search for the first entry which has a value
    greater than or equal to the SortKey.  If no such entry is found then the
    first entry of the queue is returned.

    N.B. This function can only be called from DISPATCH_LEVEL.

Arguments:

    DeviceQueue - Supplies a pointer to a control object of type device queue.

    SortKey - Supplies the sort key by which the position to remove the device
        queue entry is to be determined.

Return Value:

    A NULL pointer is returned if the device queue is empty. Otherwise a
    pointer to a device queue entry is returned.

--*/

{

    PKDEVICE_QUEUE_ENTRY DeviceQueueEntry;
    PLIST_ENTRY NextEntry;

    ASSERT_DEVICE_QUEUE(DeviceQueue);
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    //
    // Lock specified device queue.
    //

    KiAcquireSpinLock(&DeviceQueue->Lock);

    ASSERT(DeviceQueue->Busy == TRUE);

    //
    // If the device queue is not empty, then remove the first entry from
    // the queue. Otherwise set the device queue not busy.
    //

    if (IsListEmpty(&DeviceQueue->DeviceListHead) == TRUE) {
        DeviceQueue->Busy = FALSE;
        DeviceQueueEntry = (PKDEVICE_QUEUE_ENTRY)NULL;
    } else {
        NextEntry = DeviceQueue->DeviceListHead.Flink;
        while (NextEntry != &DeviceQueue->DeviceListHead) {
            DeviceQueueEntry = CONTAINING_RECORD(NextEntry, KDEVICE_QUEUE_ENTRY,
                                           DeviceListEntry);
            if (SortKey <= DeviceQueueEntry->SortKey) {
                break;
            }
            NextEntry = NextEntry->Flink;
        }

        if (NextEntry != &DeviceQueue->DeviceListHead) {
            RemoveEntryList(&DeviceQueueEntry->DeviceListEntry);

        } else {
            NextEntry = RemoveHeadList(&DeviceQueue->DeviceListHead);
            DeviceQueueEntry = CONTAINING_RECORD(NextEntry, KDEVICE_QUEUE_ENTRY,
                                             DeviceListEntry);
        }

        DeviceQueueEntry->Inserted = FALSE;
    }

    //
    // Release device queue spin lock and return address of device queue
    // entry.
    //

    KiReleaseSpinLock(&DeviceQueue->Lock);
    return DeviceQueueEntry;
}

BOOLEAN
KeRemoveEntryDeviceQueue (
    IN PKDEVICE_QUEUE DeviceQueue,
    IN PKDEVICE_QUEUE_ENTRY DeviceQueueEntry
    )

/*++

Routine Description:

    This function removes a specified entry from the the specified device
    queue. If the device queue entry is not in the device queue, then no
    operation is performed. Otherwise the specified device queue entry is
    removed from the device queue and its inserted status is set to FALSE.

Arguments:

    DeviceQueue - Supplies a pointer to a control object of type device queue.

    DeviceQueueEntry - Supplies a pointer to a device queue entry which is to
        be removed from its device queue.

Return Value:

    A value of TRUE is returned if the device queue entry is removed from its
    device queue. Otherwise a value of FALSE is returned.

--*/

{

    KIRQL OldIrql;
    BOOLEAN Removed;

    ASSERT_DEVICE_QUEUE(DeviceQueue);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQL to dispatcher level and lock specified device queue.
    //

    ExAcquireSpinLock(&DeviceQueue->Lock, &OldIrql);

    //
    // If the device queue entry is not in a device queue, then no operation
    // is performed. Otherwise remove the specified device queue entry from its
    // device queue.
    //

    Removed = DeviceQueueEntry->Inserted;
    if (Removed == TRUE) {
        DeviceQueueEntry->Inserted = FALSE;
        RemoveEntryList(&DeviceQueueEntry->DeviceListEntry);
    }

    //
    // Unlock specified device queue, lower IRQL to its previous level, and
    // return whether the device queue entry was removed from its queue.
    //

    ExReleaseSpinLock(&DeviceQueue->Lock, OldIrql);
    return Removed;
}
