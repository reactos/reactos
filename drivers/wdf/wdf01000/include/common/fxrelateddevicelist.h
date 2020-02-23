#ifndef _FXRELATEDDEVICELIST_H_
#define _FXRELATEDDEVICELIST_H_

#include "common/fxtransactionedlist.h"

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
