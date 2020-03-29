#ifndef _FXRELATEDDEVICELIST_H_
#define _FXRELATEDDEVICELIST_H_

#include "common/fxtransactionedlist.h"
#include "common/fxrelateddevice.h"


class FxRelatedDeviceList : protected FxSpinLockTransactionedList {

public:

    ULONG m_NeedReportMissing;

    FxRelatedDeviceList(
        VOID
        )
    {
        m_DeleteOnRemove = TRUE;
        m_NeedReportMissing = 0;
    }

    _Must_inspect_result_
    FxRelatedDevice*
    GetNextEntry(
        __in_opt FxRelatedDevice* Entry
        )
    {
        FxTransactionedEntry *pReturn, *pEntry;

        if (Entry == NULL)
        {
            pEntry = NULL;
        }
        else
        {
            pEntry = &Entry->m_TransactionedEntry;
        }

        pReturn = FxSpinLockTransactionedList::GetNextEntry(pEntry);

        if (pReturn != NULL)
        {
            return CONTAINING_RECORD(pReturn, FxRelatedDevice, m_TransactionedEntry);
        }
        else
        {
            return NULL;
        }
    }

    VOID
    LockForEnum(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        )
    {
        __super::LockForEnum(FxDriverGlobals);
    }

    VOID
    UnlockFromEnum(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        )
    {
        __super::UnlockFromEnum(FxDriverGlobals);
    }

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
    
};

#endif //_FXRELATEDDEVICELIST_H_
