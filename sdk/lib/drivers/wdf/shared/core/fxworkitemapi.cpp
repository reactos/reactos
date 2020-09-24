/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxWorkItemApi.cpp

Abstract:

    This implements the WDFWORKITEM API's

Author:



Environment:

    Both kernel and user mode

Revision History:


--*/

#include "coreprivshared.hpp"

#include "FxWorkItem.hpp"

extern "C" {
#include "FxWorkItemApi.tmh"
}


//
// extern "C" the entire file
//
extern "C" {

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfWorkItemCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDF_WORKITEM_CONFIG Config,
    __in
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFWORKITEM* WorkItem
    )

/*++

Routine Description:

    Create a WorkItem object that will call the supplied function with
    context when it fires. It returns a handle to the WDFWORKITEM object.

Arguments:

    Config - Pointer to WDF_WORKITEM_CONFIG structure

    Attributes - WDF_OBJECT_ATTRIBUTES to set the parent object and to request
                 a context memory allocation,  and a DestroyCallback.

    WorkItem - Pointer to the created WDFWORKITEM handle.

Returns:

    STATUS_SUCCESS - A WDFWORKITEM handle has been created.

    The WDFWORKITEM will be automatically deleted when the object it is
    associated with is deleted.

Notes:

    The WDFWORKITEM object is deleted either when the DEVICE or QUEUE it is
    associated with is deleted, or WdfObjectDelete is called.

    If the WDFWORKITEM is used to access WDM objects, a Cleanup callback should
    be registered to allow references to be released.

--*/

{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxObject* pParent;
    NTSTATUS status;

    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);
    status = FxValidateObjectAttributesForParentHandle(pFxDriverGlobals,
                                                       Attributes,
                                                       FX_VALIDATE_OPTION_PARENT_REQUIRED);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    FxObjectHandleGetPtrAndGlobals(pFxDriverGlobals,
                                   Attributes->ParentObject,
                                   FX_TYPE_OBJECT,
                                   (PVOID*)&pParent,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Config);
    FxPointerNotNull(pFxDriverGlobals, WorkItem);

    if (Config->Size != sizeof(WDF_WORKITEM_CONFIG)) {
        status = STATUS_INFO_LENGTH_MISMATCH;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDF_WORKITEM_CONFIG Size got %d, expected %d, %!STATUS!",
            Config->Size, sizeof(WDF_WORKITEM_CONFIG), status);

        return status;
    }

    if (Config->EvtWorkItemFunc == NULL) {
        status = STATUS_INVALID_PARAMETER;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Supplied EvtWorkItemFunc == NULL, %!STATUS!",
                            status);

        return status;
    }

    //
    // The parent for FxWorkItem is explicitly part of the API, and ties
    // synchronization and lifetime.
    //
    status = FxValidateObjectAttributes(pFxDriverGlobals, Attributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    return FxWorkItem::_Create(
        pFxDriverGlobals, Config, Attributes, pParent, WorkItem);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfWorkItemEnqueue)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWORKITEM WorkItem
    )
/*++

Routine Description:

    Enqueue a WorkItem to execute.

Arguments:

    WorkItem - Handle to WDFWORKITEM

Returns:

    None

--*/
{
    DDI_ENTRY();

    FxWorkItem* pFxWorkItem;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         WorkItem,
                         FX_TYPE_WORKITEM,
                         (PVOID*)&pFxWorkItem);

    pFxWorkItem->Enqueue();
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFOBJECT
WDFEXPORT(WdfWorkItemGetParentObject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWORKITEM WorkItem
    )

/*++

Routine Description:

    Return the Object handle supplied to WdfWorkItemCreate

Arguments:

    WDFWORKITEM - Handle to WDFWORKITEM object created with WdfWorkItemCreate.

Returns:

    Handle to the framework object that is the specified work-item object's
    parent object.

--*/

{
    DDI_ENTRY();

    FxWorkItem* pFxWorkItem;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         WorkItem,
                         FX_TYPE_WORKITEM,
                         (PVOID*)&pFxWorkItem);

    return pFxWorkItem->GetAssociatedObject();
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
WDFEXPORT(WdfWorkItemFlush)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFWORKITEM WorkItem
    )
/*++

Routine Description:

    Wait until any outstanding workitems have completed

Arguments:

    WorkItem - Handle to WDFWORKITEM

Returns:

    None

--*/
{
    DDI_ENTRY();

    FxWorkItem* pFxWorkItem;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         WorkItem,
                         FX_TYPE_WORKITEM,
                         (PVOID*)&pFxWorkItem);

    //
    // Use the object's globals, not the caller's
    //
    if (!NT_SUCCESS(FxVerifierCheckIrqlLevel(pFxWorkItem->GetDriverGlobals(),
                                            PASSIVE_LEVEL))) {
        return;
    }

    pFxWorkItem->FlushAndWait();
}

} // extern "C"
