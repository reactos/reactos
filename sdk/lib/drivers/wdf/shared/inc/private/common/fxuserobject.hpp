
/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    FxUserObject.hpp

Abstract:

    This module implements the user object that device
    driver writers can use to take advantage of the
    driver frameworks infrastructure.

Author:




Environment:

    Both kernel and user mode

Revision History:


        Made mode agnostic

--*/

#ifndef _FXUSEROBJECT_H_
#define _FXUSEROBJECT_H_

class FxUserObject : public FxNonPagedObject {

private:

public:

    static
    _Must_inspect_result_
    NTSTATUS
    _Create(
        __in     PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in_opt PWDF_OBJECT_ATTRIBUTES Attributes,
        __out    FxUserObject** Object
        );

    FxUserObject(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    virtual
    NTSTATUS
    QueryInterface(
        __in FxQueryInterfaceParams* Params
        )
    {
        switch (Params->Type) {
        case FX_TYPE_USEROBJECT:
             *Params->Object = (FxUserObject*) this;
             break;

        default:
            return __super::QueryInterface(Params);
        }

        return STATUS_SUCCESS;
    }

    __inline
    WDFOBJECT
    GetHandle(
        VOID
        )
    {
        return (WDFOBJECT) GetObjectHandle();
    }

private:

#ifdef INLINE_WRAPPER_ALLOCATION
#if (FX_CORE_MODE==FX_CORE_USER_MODE)
    static
    USHORT
    GetWrapperSize(
        VOID
        );

public:
    FORCEINLINE
    PVOID
    GetCOMWrapper(
        VOID
        )
    {
        PBYTE ptr = (PBYTE) this;
        return (ptr + (USHORT) WDF_ALIGN_SIZE_UP(sizeof(*this), MEMORY_ALLOCATION_ALIGNMENT));
    }
#endif
#endif
};

#endif // _FXUSEROBJECT_H_

