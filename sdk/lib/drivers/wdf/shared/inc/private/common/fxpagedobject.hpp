/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxPagedObject.hpp

Abstract:

    This module defines the abstract FxPagedObject class.

Author:



--*/

#ifndef _FXPAGEDOBJECT_H_
#define _FXPAGEDOBJECT_H_

class FxPagedObject : public FxObject
{
private:
    MxPagedLock* m_Lock;

public:

    FxPagedObject(
        __in WDFTYPE Type,
        __in USHORT Size,
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        ) :
        FxObject(Type, Size, FxDriverGlobals)
    {
        m_Lock = NULL;

        // no need to hold the lock while the object is being constructed
        MarkPassiveCallbacks(ObjectDoNotLock);
    }

    virtual
    ~FxPagedObject(
        VOID
        )
    {
        if (m_Lock != NULL) {
            FxPoolFree(m_Lock);
            m_Lock = NULL;
        }
    }

    VOID
    Lock(
        VOID
        )
    {
        m_Lock->Acquire();
    }

    VOID
    Unlock(
        VOID
        )
    {
        m_Lock->Release();
    }

    _Must_inspect_result_
    NTSTATUS
    Initialize(
        VOID
        )
    {
        PFX_DRIVER_GLOBALS fxDriverGlobals;

        fxDriverGlobals = GetDriverGlobals();
        m_Lock = (MxPagedLock*) FxPoolAllocate(fxDriverGlobals,
                                               NonPagedPool,
                                               sizeof(MxPagedLock));
        if (m_Lock != NULL) {
            return m_Lock->Initialize();
        }
        else {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
};

#endif //  _FXPAGEDOBJECT_H_
