/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Simple transactioned list
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */


/*
Abstract:

    This module implements a simple transactioned list which allows the caller
    to lock the list and then iterate over it without worrying about deletes
    and adds.
*/

#include "common/fxtransactionedlist.h"
#include "common/fxobject.h"


FxTransactionedList::FxTransactionedList()
{
    m_ListLockedRecursionCount = 0;
    m_DeleteOnRemove = FALSE;
    m_Deleting = FALSE;
    m_Retries = 0;
    m_DeletingDoneEvent = NULL;

    InitializeListHead(&m_ListHead);
    InitializeListHead(&m_TransactionHead);
}

FxTransactionedList::~FxTransactionedList()
{
    FxTransactionedEntry* pEntry;
    PLIST_ENTRY ple;

    //
    // If m_DeleteOnRemove is FALSE, there is no need to iterate over any of the
    // lists to free anything.
    //
    if (m_DeleteOnRemove == FALSE)
    {
        ASSERT(IsListEmpty(&m_ListHead));
        ASSERT(IsListEmpty(&m_TransactionHead));
        return;
    }

    ASSERT(m_ListLockedRecursionCount == 0);

    while (!IsListEmpty(&m_ListHead))
    {
        ple = RemoveHeadList(&m_ListHead);
        InitializeListHead(ple);

        pEntry = FxTransactionedEntry::_FromEntry(ple);

        switch (pEntry->m_Transaction) {
        case FxTransactionActionNothing:
            //
            // Nothing to do, no pending transaction
            //
            break;

        case FxTransactionActionAdd:
            //
            // Should not have an add transaction and be on the main list at the
            // same time!
            //
            ASSERT(FALSE);
            break;

        case FxTransactionActionRemove:
            //
            // Make sure it is not on the transaction list
            //
            RemoveEntryList(&pEntry->m_TransactionLink);
            InitializeListHead(&pEntry->m_TransactionLink);

            //
            // When inserted as a remove transaction, we add this reference in
            // RemoveLocked
            //
            pEntry->GetTransactionedObject()->RELEASE(pEntry);
            break;

        }

        pEntry->GetTransactionedObject()->DeleteObject();
    }

    while (!IsListEmpty(&m_TransactionHead))
    {
        ple = RemoveHeadList(&m_TransactionHead);
        InitializeListHead(ple);

        pEntry = CONTAINING_RECORD(ple, FxTransactionedEntry, m_TransactionLink);

        //
        // We yanked out all of the removes in the previous loop
        //
        ASSERT(pEntry->m_Transaction == FxTransactionActionAdd);

        //
        // Delete the object since this list owns it.
        //
        pEntry->GetTransactionedObject()->DeleteObject();
    }
}

_Must_inspect_result_
NTSTATUS
FxTransactionedList::Add(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in FxTransactionedEntry* Entry
    )
{
    NTSTATUS status;
    KIRQL irql;

    AcquireLock(FxDriverGlobals, &irql);

    if (m_Deleting)
    {
        status = STATUS_INVALID_DEVICE_STATE;
    }
    else
    {
        status = ProcessAdd(Entry);
    }

    if (NT_SUCCESS(status))
    {
        if (m_ListLockedRecursionCount == 0)
        {
            //
            // We can insert the entry now, do so
            //
            InsertTailList(&m_ListHead, &Entry->m_ListLink);

            EntryAdded(Entry);
        }
        else
        {
            //
            // List is locked, queue a transaction
            //
            Entry->m_Transaction = FxTransactionActionAdd;
            InsertTailList(&m_TransactionHead, &Entry->m_TransactionLink);
        }
    }

    ReleaseLock(FxDriverGlobals, irql);

    return status;
}

_Acquires_lock_(_Global_critical_region_)
VOID
FxWaitLockTransactionedList::AcquireLock(
    __in  PFX_DRIVER_GLOBALS FxDriverGlobals,
    __out PKIRQL Irql
    )
{
    UNREFERENCED_PARAMETER(Irql);
    m_StateChangeListLock.AcquireLock(FxDriverGlobals);
}

_Releases_lock_(_Global_critical_region_)
VOID
FxWaitLockTransactionedList::ReleaseLock(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in KIRQL Irql
    )
{
    UNREFERENCED_PARAMETER(Irql);
    m_StateChangeListLock.ReleaseLock(FxDriverGlobals);
}

VOID
FxTransactionedList::LockForEnum(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    KIRQL irql;

    AcquireLock(FxDriverGlobals, &irql);
    m_ListLockedRecursionCount++;
    ReleaseLock(FxDriverGlobals, irql);
}

VOID
FxTransactionedList::UnlockFromEnum(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    LIST_ENTRY releaseHead;
    KIRQL irql;
    MxEvent* event;

    InitializeListHead(&releaseHead);
    event = NULL;
    AcquireLock(FxDriverGlobals, &irql);
    m_ListLockedRecursionCount--;
    ProcessTransactionList(&releaseHead);

    if (m_ListLockedRecursionCount == 0 && m_Deleting)
    {
        event = m_DeletingDoneEvent;
        m_DeletingDoneEvent = NULL;
    }
    ReleaseLock(FxDriverGlobals, irql);

    ProcessObjectsToRelease(&releaseHead);

    if (event != NULL)
    {
        event->Set();
    }
}

VOID
FxTransactionedList::ProcessTransactionList(
    __in PLIST_ENTRY ReleaseHead
    )
{
    LIST_ENTRY *ple;
    FxTransactionedEntry* pEntry;

    //
    // If there are other iterators, do not process transactions until they are
    // done.
    //
    if (m_ListLockedRecursionCount != 0)
    {
        return;
    }

    while (!IsListEmpty(&m_TransactionHead))
    {
        ple = RemoveHeadList(&m_TransactionHead);
        InitializeListHead(ple);

        pEntry = CONTAINING_RECORD(ple, FxTransactionedEntry, m_TransactionLink);

        ASSERT(pEntry->m_Transaction != FxTransactionActionNothing);

        if (pEntry->m_Transaction == FxTransactionActionAdd)
        {
            //
            // Add to the main list
            //
            InsertTailList(&m_ListHead, &pEntry->m_ListLink);

            //
            // Virtual notification of addition
            //
            EntryAdded(pEntry);
        }
        else if (pEntry->m_Transaction == FxTransactionActionRemove)
        {
            //
            // Remove it from the main list and move it to a free list
            //
            RemoveEntryList(&pEntry->m_ListLink);
            InsertTailList(ReleaseHead, &pEntry->m_TransactionLink);

            //
            // Virtual notification of removal
            //
            EntryRemoved(pEntry);
        }

        pEntry->m_Transaction = FxTransactionActionNothing;
    }
}

VOID
FxTransactionedList::ProcessObjectsToRelease(
    __in PLIST_ENTRY ReleaseHead
    )
{
    LIST_ENTRY *ple;
    FxTransactionedEntry* pEntry;

    while (!IsListEmpty(ReleaseHead))
    {
        ple = RemoveHeadList(ReleaseHead);
        InitializeListHead(ple);

        pEntry = CONTAINING_RECORD(ple, FxTransactionedEntry, m_TransactionLink);

        //
        // We always release our reference we took when we post the change
        // to the list
        //
        pEntry->GetTransactionedObject()->RELEASE(pEntry);

        //
        // 2ndary release if the list is set to do this
        //
        if (m_DeleteOnRemove)
        {
            pEntry->GetTransactionedObject()->DeleteObject();
        }
    }
}

_Must_inspect_result_
FxTransactionedEntry*
FxTransactionedList::GetNextEntry(
    __in_opt FxTransactionedEntry* Entry
    )
/*++

Routine Description:
    Gets the next entry.  Assumes the caller has called LockedForEnum

Arguments:
    Entry the current entry in the iteratation, NULL for the first

Return Value:
    next entry in the iteration, NULL if there are no more entries

  --*/
{
    //
    // The caller should have locked the list for enumeration
    //
    ASSERT(m_ListLockedRecursionCount > 0 || m_Deleting);

    return GetNextEntryLocked(Entry);
}

_Must_inspect_result_
FxTransactionedEntry*
FxTransactionedList::GetNextEntryLocked(
    __in_opt FxTransactionedEntry* Entry
    )
/*++

Routine Description:
    Returns the next entry.  Assumes that the caller has the list locked through
    a call to AcquireLock() or through LockForEnum()

Arguments:
    Entry the current entry in the iteratation, NULL for the first

Return Value:
    next entry in the iteration, NULL if there are no more entries

  --*/
{
    PLIST_ENTRY ple;

    if (Entry == NULL)
    {
        ple = m_ListHead.Flink;
    }
    else
    {
        ple = Entry->m_ListLink.Flink;
    }

    //
    // Find the next entry which does not have a pending transaction on it
    //
    for ( ; ple != &m_ListHead; ple = ple->Flink)
    {
        FxTransactionedEntry* pNext;

        pNext = FxTransactionedEntry::_FromEntry(ple);
        if (pNext->m_Transaction == FxTransactionActionNothing)
        {
            return pNext;
        }
    }

    //
    // Reached the end of the list
    //
    return NULL;
}

FxSpinLockTransactionedList::FxSpinLockTransactionedList() :
    FxTransactionedList()
{
}

__drv_raisesIRQL(DISPATCH_LEVEL)
__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FxSpinLockTransactionedList::AcquireLock(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __out PKIRQL Irql
    )
{
    UNREFERENCED_PARAMETER(FxDriverGlobals);
    m_ListLock.Acquire(Irql);
}

__drv_requiresIRQL(DISPATCH_LEVEL)
VOID
FxSpinLockTransactionedList::ReleaseLock(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in __drv_restoresIRQL KIRQL Irql
    )
{
    UNREFERENCED_PARAMETER(FxDriverGlobals);
    m_ListLock.Release(Irql);
}
