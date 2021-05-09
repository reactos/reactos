/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxUserObjectApi.cpp

Abstract:

    This modules implements the C API's for the FxUserObject.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#include "fxobjectpch.hpp"

#include "fxuserobject.hpp"

// Tracing support
extern "C" {
#if defined(EVENT_TRACING)
#include "FxUserObjectApi.tmh"
#endif
}


extern "C" {

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfObjectCreate)(
   __in
   PWDF_DRIVER_GLOBALS DriverGlobals,
   __in_opt
   PWDF_OBJECT_ATTRIBUTES Attributes,
   __out
   WDFOBJECT* Object
   )

/*++

Routine Description:

    This creates a general WDF object for use by the device driver.

    It participates in general framework object contracts in that it:

    - Has a handle and a reference count
    - Has Cleanup and Destroy callbacks
    - Supports driver context memory and type
    - Can have child objects
    - Can optionally have a parent object and automatically delete with it

    It is intended to allow a WDF device driver to use this object to
    create its own structures that can participate in frameworks lifetime
    management.

    The device driver can use the objects context memory and type to
    represent its own internal data structures, and can further assign
    device driver specific resources and release them by registering
    for EvtObjectCleanup, and EvtObjectDestroy callbacks.

    The object may be deleted by using the WdfObjectDelete API.

    Since the object is represented by a frameworks handle, it can be
    reference counted, and validated.

    Class drivers may use this object to define framework object handles
    for their types.

Arguments:

    Attributes - WDF_OBJECT_ATTRIBUTES to define a parent object, context memory,
                 Cleanup and Destroy handlers.

Return Value:

    NTSTATUS

--*/

{
    DDI_ENTRY();

    NTSTATUS status;
    WDFOBJECT handle;
    FxUserObject* pUserObject;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pUserObject = NULL;
    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);

    //
    // Get the parent's globals if it is present
    //
    if (NT_SUCCESS(FxValidateObjectAttributesForParentHandle(pFxDriverGlobals,
                                                             Attributes))) {
        FxObject* pParent;

        FxObjectHandleGetPtrAndGlobals(pFxDriverGlobals,
                                       Attributes->ParentObject,
                                       FX_TYPE_OBJECT,
                                       (PVOID*)&pParent,
                                       &pFxDriverGlobals);
    }

    FxPointerNotNull(pFxDriverGlobals, Object);

    status = FxValidateObjectAttributes(pFxDriverGlobals,
                                        Attributes,
                                        FX_VALIDATE_OPTION_EXECUTION_LEVEL_ALLOWED
                                        );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Create the FxObject.
    //
    status = FxUserObject::_Create(pFxDriverGlobals, Attributes, &pUserObject);
    if (NT_SUCCESS(status)) {
        handle = pUserObject->GetHandle();
        *Object = handle;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE,
                            TRACINGUSEROBJECT,
                            "Created UserObject Handle 0x%p",
                            handle);
    }

    return status;
}

} // extern "C" the entire file
