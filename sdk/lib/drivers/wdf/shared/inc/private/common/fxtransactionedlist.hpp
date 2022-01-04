/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxTransactionedList.hpp

Abstract:

    This module defines the abstract FxTransactionedList class.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#ifndef _FXTRANSACTIONEDLIST_H_
#define _FXTRANSACTIONEDLIST_H_

enum FxListTransactionAction {
    FxTransactionActionNothing = 1,
    FxTransactionActionAdd,
    FxTransactionActionRemove,
};

struct FxTransactionedEntry {
    friend FxTransactionedList;

    FxTransactionedEntry(
        __in_opt FxObject* Object = NULL
        )
    {
        m_Transaction = FxTransactionActionNothing;
        m_TransactionedObject = Object;
        InitializeListHead(&m_ListLink);
        InitializeListHead(&m_TransactionLink);
    }

    VOID
    SetTransactionedObject(
        __in FxObject* Object
        )
    {
        m_TransactionedObject = Object;
    }

    FxObject*
    GetTransactionedObject(
        VOID
        )
    {
        return m_TransactionedObject;
    }

    static
    FxTransactionedEntry*
    _FromEntry(
        __in PLIST_ENTRY Entry
        )
    {
        return CONTAINING_RECORD(Entry, FxTransactionedEntry, m_ListLink);
    }

    FxListTransactionAction
    GetTransactionAction(
        VOID
        )
    {
        return m_Transaction;
    }

private:
    LIST_ENTRY m_ListLink;

    LIST_ENTRY m_TransactionLink;

    FxListTransactionAction m_Transaction;

    FxObject* m_TransactionedObject;
};

class FxTransactionedList : public FxStump {
public:
    FxTransactionedList();

    ~FxTransactionedList();

    VOID
    LockForEnum(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    VOID
    UnlockFromEnum(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    NTSTATUS
    Add(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in FxTransactionedEntry* Entry
        );

    VOID
    Remove(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in FxTransactionedEntry* Entry
        );

    _Must_inspect_result_
    FxTransactionedEntry*
    GetNextEntry(
        __in_opt FxTransactionedEntry* Entry
        );

    BOOLEAN
    Deleting(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in_opt MxEvent* DeleteDoneEvent
        );

protected:
    virtual
    VOID
    AcquireLock(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __out PKIRQL Irql
        ) =0;

    virtual
    VOID
    ReleaseLock(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in KIRQL Irql
        ) =0;

    virtual
    _Must_inspect_result_
    NTSTATUS
    ProcessAdd(
        __in FxTransactionedEntry* Entry
        )
    {
        UNREFERENCED_PARAMETER(Entry);

        return STATUS_SUCCESS;
    }

    virtual
    VOID
    EntryAdded(
        __in FxTransactionedEntry* Entry
        )
    {
        UNREFERENCED_PARAMETER(Entry);
    }

    virtual
    VOID
    EntryRemoved(
        __in FxTransactionedEntry* Entry
        )
    {
        UNREFERENCED_PARAMETER(Entry);
    }

    virtual
    BOOLEAN
    Compare(
        __in FxTransactionedEntry* Entry,
        __in PVOID Data
        )
    {
        UNREFERENCED_PARAMETER(Entry);
        UNREFERENCED_PARAMETER(Data);

        return TRUE;
    }

    VOID
    SearchForAndRemove(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in PVOID EntryData
        );

    _Must_inspect_result_
    FxTransactionedEntry*
    GetNextEntryLocked(
        __in_opt FxTransactionedEntry* Entry
        );

    BOOLEAN
    RemoveLocked(
        __in FxTransactionedEntry* Entry
        );

    VOID
    ProcessTransactionList(
        __in PLIST_ENTRY ReleaseHead
        );

    VOID
    ProcessObjectsToRelease(
        __in PLIST_ENTRY ReleaseHead
        );

protected:
    LIST_ENTRY m_ListHead;

    LIST_ENTRY m_TransactionHead;

    MxEvent* m_DeletingDoneEvent;

    ULONG m_ListLockedRecursionCount;

    BOOLEAN m_DeleteOnRemove;

    BOOLEAN m_Deleting;

    //
    // The base class does not use this field, but to save space in the size of
    // the derived structures, we place it here after the BOOLEANs and there is
    // minimal structure byte packing.
    //
    UCHAR m_Retries;
};


class FxSpinLockTransactionedList : public FxTransactionedList {

public:
    FxSpinLockTransactionedList();

protected:

    __drv_raisesIRQL(DISPATCH_LEVEL)
    __drv_maxIRQL(DISPATCH_LEVEL)
    virtual
    VOID
    AcquireLock(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __out PKIRQL Irql
        );

    __drv_requiresIRQL(DISPATCH_LEVEL)
    virtual
    VOID
    ReleaseLock(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in __drv_restoresIRQL KIRQL Irql
        );

    MxLock m_ListLock;
};

class FxWaitLockTransactionedList : public FxTransactionedList {

public:

    FxWaitLockTransactionedList(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        )
    {
        UNREFERENCED_PARAMETER(FxDriverGlobals);
    }

    __inline
    NTSTATUS
#ifdef _MSC_VER
#pragma prefast(suppress:__WARNING_UNMATCHED_DEFN, "_Must_inspect_result_ not needed in kernel mode as the function always succeeds");
#endif
    Initialize(
        VOID
        )
    {
        return m_StateChangeListLock.Initialize();
    }

protected:
    virtual
    _Acquires_lock_(_Global_critical_region_)
    VOID
    AcquireLock(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __out PKIRQL Irql
        );

    virtual
    _Releases_lock_(_Global_critical_region_)
    VOID
    ReleaseLock(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in KIRQL Irql
        );

    FxWaitLockInternal m_StateChangeListLock;
};

#endif // _FXTRANSACTIONEDLIST_H_
