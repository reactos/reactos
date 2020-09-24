/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxUserObject.cpp

Abstract:

    This module implements the user object that device
    driver writers can use to take advantage of the
    driver frameworks infrastructure.

Author:




Environment:

    Both kernel and user mode

Revision History:






--*/

#include "fxobjectpch.hpp"

#include "FxUserObject.hpp"

// Tracing support
extern "C" {
#if defined(EVENT_TRACING)
#include "FxUserObject.tmh"
#endif
}

_Must_inspect_result_
NTSTATUS
FxUserObject::_Create(
    __in     PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in_opt PWDF_OBJECT_ATTRIBUTES Attributes,
    __out    FxUserObject** pUserObject
    )
{
    FxUserObject* pObject = NULL;
    NTSTATUS status;
    USHORT wrapperSize = 0;
    WDFOBJECT handle;

#ifdef INLINE_WRAPPER_ALLOCATION
#if FX_CORE_MODE==FX_CORE_USER_MODE
    wrapperSize = FxUserObject::GetWrapperSize();
#endif
#endif

    pObject = new(FxDriverGlobals, Attributes, wrapperSize)
        FxUserObject(FxDriverGlobals);


    if (pObject == NULL) {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGOBJECT,
                            "Memory allocation failed");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = pObject->Commit(Attributes, &handle);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR,
                            TRACINGOBJECT,
                            "FxObject::Commit failed %!STATUS!", status);
    }

    if (NT_SUCCESS(status)) {
        *pUserObject = pObject;
    }
    else {
        pObject->DeleteFromFailedCreate();
    }

    return status;
}

//
// Public constructors
//

FxUserObject::FxUserObject(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    ) :
#ifdef INLINE_WRAPPER_ALLOCATION
    FxNonPagedObject(FX_TYPE_USEROBJECT, sizeof(FxUserObject) + GetWrapperSize(), FxDriverGlobals)
#else
    FxNonPagedObject(FX_TYPE_USEROBJECT, sizeof(FxUserObject), FxDriverGlobals)
#endif
{
    return;
}

