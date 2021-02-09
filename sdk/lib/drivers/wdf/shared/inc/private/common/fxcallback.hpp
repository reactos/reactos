/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxCallback.hpp

Abstract:

    This represents a delegate object base class for
    calling back into the driver.

Author:




Revision History:

--*/

#ifndef _FXCALLBACK_H_
#define _FXCALLBACK_H_

extern "C" {
#if defined(EVENT_TRACING)
#include "FxCallback.hpp.tmh"
#endif
}

class FxCallback  {

public:

    FxCallback(
        __in_opt PFX_DRIVER_GLOBALS FxDriverGlobals = NULL
        )
    {
        UNREFERENCED_PARAMETER(FxDriverGlobals);
    }

    PVOID
    operator new(
        __in size_t Size,
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in POOL_TYPE PoolType = NonPagedPool
        )
    {
        return FxPoolAllocate(FxDriverGlobals, PoolType, Size);
    }

    VOID
    operator delete(
        __in PVOID pointer
        )
    {
        FxPoolFree(pointer);
    }

protected:
    void
    __inline
    CallbackStart(
        VOID
        )
    {
        // intentionally does nothing, visual place marker only for now
        DO_NOTHING();
    }

    void
    __inline
    CallbackEnd(
        VOID
        )
    {
        // intentionally does nothing, visual place marker only for now
        DO_NOTHING();
    }
};

class FxLockedCallback {

private:
    FxCallbackLock* m_CallbackLock;

public:
    FxLockedCallback(
        VOID
        )
    {
        m_CallbackLock = NULL;
    }

    FxCallbackLock*
    GetCallbackLockPtr(
        VOID
        )
    {
        return m_CallbackLock;
    }

    void
    SetCallbackLockPtr(
        FxCallbackLock* Lock
        )
    {
        m_CallbackLock = Lock;
    }

protected:
    __inline
    void
    CallbackStart(
        __out PKIRQL PreviousIrql
        )
    {
        if (m_CallbackLock != NULL) {
            m_CallbackLock->Lock(PreviousIrql);
        }
    }

    __inline
    void
    CallbackEnd(
        __in KIRQL PreviousIrql
        )
    {
        if (m_CallbackLock != NULL) {
            m_CallbackLock->Unlock(PreviousIrql);
        }
    }
};

#endif // _FXCALLBACK_H_
