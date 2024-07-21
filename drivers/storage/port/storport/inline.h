/*
 * PROJECT:     ReactOS Storport Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Storport driver - inlined helper utilities
 * COPYRIGHT:   Copyright 2024 Wu Haotian (rigoligo03@gmail.com)
 */

#pragma once

#include <wdm.h>

/*
  Convenient counted linked list manipulation. In ReactOS Storport there are linked lists whose
  item count is kept alongside the list. This function provides a cleaner implementation to
  reduce complexity in functional code.
*/

/**
 * @brief InsertHeadList and increment element counter with a corresponding spin lock held.
 * 
 * @param SpinLock Spin lock for interlocked exclusive access.
 * @param ListHead List head of the list.
 * @param Entry List entry to be inserted.
 * @param Counter Pointer to the list element counter ULONG value.
 * @return ULONG Number of items in the list after insertion.
 */
ULONG
FORCEINLINE
StorpInterlockedInsertHeadListCounted(
    _In_ PKSPIN_LOCK SpinLock,
    _In_ PLIST_ENTRY ListHead,
    _In_ PLIST_ENTRY Entry,
    _In_ PULONG Counter
)
{
    KLOCK_QUEUE_HANDLE LockHandle;

    KeAcquireInStackQueuedSpinLock(SpinLock, &LockHandle);
    InsertHeadList(ListHead, Entry);
    ++(*Counter);
    KeReleaseInStackQueuedSpinLock(&LockHandle);

    return (*Counter);
}

/**
 * @brief InsertTailList and increment element counter with a corresponding spin lock held.
 * 
 * @param SpinLock Spin lock for interlocked exclusive access.
 * @param ListHead List head of the list.
 * @param Entry List entry to be inserted.
 * @param Counter Pointer to the list element counter ULONG value.
 * @return ULONG Number of items in the list after insertion.
 */
ULONG
FORCEINLINE
StorpInterlockedInsertTailListCounted(
    _In_ PKSPIN_LOCK SpinLock,
    _In_ PLIST_ENTRY ListHead,
    _In_ PLIST_ENTRY Entry,
    _In_ PULONG Counter
)
{
    KLOCK_QUEUE_HANDLE LockHandle;

    KeAcquireInStackQueuedSpinLock(SpinLock, &LockHandle);
    InsertTailList(ListHead, Entry);
    ++(*Counter);
    KeReleaseInStackQueuedSpinLock(&LockHandle);

    return (*Counter);
}

/**
 * @brief RemoveEntryList and decrement element counter with a corresponding spin lock held.
 * 
 * @param SpinLock Spin lock for interlocked exclusive access.
 * @param Entry List entry to be removed.
 * @param Counter Pointer to the list element counter ULONG value.
 * @return BOOLEAN Whether the list is empty after the removal.
 */
BOOLEAN
FORCEINLINE
StorpInterlockedRemoveEntryListCounted(
    _In_ PKSPIN_LOCK SpinLock,
    _In_ PLIST_ENTRY Entry,
    _In_ PULONG Counter)
{
    KLOCK_QUEUE_HANDLE LockHandle;
    BOOLEAN Empty;

    KeAcquireInStackQueuedSpinLock(SpinLock, &LockHandle);
    RemoveEntryList(Entry);
    --(*Counter);
    KeReleaseInStackQueuedSpinLock(&LockHandle);

    return Empty;
}

