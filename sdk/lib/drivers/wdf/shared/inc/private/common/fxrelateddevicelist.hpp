//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef _FXRELATEDDEVICELIST_H_
#define _FXRELATEDDEVICELIST_H_

class FxRelatedDeviceList : protected FxSpinLockTransactionedList {
public:
    FxRelatedDeviceList(
        VOID
        )
    {
        m_DeleteOnRemove = TRUE;
        m_NeedReportMissing = 0;
    }

    VOID
    LockForEnum(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        )
    {
        FxSpinLockTransactionedList::LockForEnum(FxDriverGlobals); // __super call
    }

    VOID
    UnlockFromEnum(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        )
    {
        FxSpinLockTransactionedList::UnlockFromEnum(FxDriverGlobals); // __super call
    }

    _Must_inspect_result_
    NTSTATUS
    Add(
        __in PFX_DRIVER_GLOBALS Globals,
        __inout FxRelatedDevice* Entry
        );

    VOID
    Remove(
        __in PFX_DRIVER_GLOBALS Globals,
        __in MdDeviceObject Device
        );

    _Must_inspect_result_
    FxRelatedDevice*
    GetNextEntry(
        __in_opt FxRelatedDevice* Entry
        );

    UCHAR
    IncrementRetries(
        VOID
        )
    {
        m_Retries++;
        return m_Retries;
    }

    VOID
    ZeroRetries(
        VOID
        )
    {
        m_Retries = 0;
    }

    _Must_inspect_result_
    PVOID
    operator new(
        __in size_t Size,
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        )
    {
        return FxPoolAllocate(FxDriverGlobals, NonPagedPool, Size);
    }

    VOID
    operator delete(
        __in PVOID pointer
        )
    {
        FxPoolFree(pointer);
    }

protected:
    virtual
    _Must_inspect_result_
    NTSTATUS
    ProcessAdd(
        __in FxTransactionedEntry *Entry
        );

    virtual
    BOOLEAN
    Compare(
        __in FxTransactionedEntry* Entry,
        __in PVOID Data
        );

    virtual
    VOID
    EntryRemoved(
        __in FxTransactionedEntry* Entry
        );

public:
    ULONG m_NeedReportMissing;
};

#endif //  _FXRELATEDDEVICELIST_H_
