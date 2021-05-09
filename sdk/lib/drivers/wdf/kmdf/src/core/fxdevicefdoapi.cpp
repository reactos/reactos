/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDeviceFdoApi.cpp

Abstract:

    This module exposes the "C" interface to the FxDevice object.

Author:



Environment:

    Kernel mode only

Revision History:

--*/

#include "fxcorepch.hpp"

extern "C" {
// #include "FxDeviceFdoApi.tmh"
}

//
// Extern "C" the rest of the  file
//
extern "C" {

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfFdoAddStaticChild)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Fdo,
    __in
    WDFDEVICE Child
    )
/*++

Routine Description:
    Adds a statically enumerated child to the static device list.

Arguments:
    Fdo - the parent of the child

    Child - the child to add

Return Value:
    NTSTATUS

  --*/

{
    FxStaticChildDescription description;
    FxDevice* pFdo;
    FxDevice* pPdo;
    FxPkgFdo* pPkgFdo;
    NTSTATUS status;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Fdo,
                         FX_TYPE_DEVICE,
                         (PVOID*)&pFdo);

    //
    // Verify type
    //
    if (pFdo->IsLegacy() || pFdo->IsFdo() == FALSE) {
        status = STATUS_INVALID_PARAMETER;

        DoTraceLevelMessage(
            pFdo->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFDEVICE 0x%p is either legacy or is not a Fdo, %!STATUS!",
            Fdo, status);

        return status;
    }

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Child,
                         FX_TYPE_DEVICE,
                         (PVOID*)&pPdo);

    if (pPdo->IsLegacy() || pPdo->IsPdo() == FALSE) {
        status = STATUS_INVALID_PARAMETER;

        DoTraceLevelMessage(
            pFdo->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFDEVICE Child 0x%p is either legacy or is not a PDO, %!STATUS!",
            Child, status);

        return status;
    }

    pPkgFdo = pFdo->GetFdoPkg();

    //
    // Try to add the device to the list.  We use the FxDevice* of the PDO as the
    // unique ID in the list.
    //
    description.Header.IdentificationDescriptionSize = sizeof(description);
    description.Pdo = pPdo;

    status = pPkgFdo->m_StaticDeviceList->Add(&description.Header, NULL, NULL);

    if (NT_SUCCESS(status)) {
        pFdo->SetDeviceTelemetryInfoFlags(DeviceInfoHasStaticChildren);
    }

    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfFdoLockStaticChildListForIteration)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Fdo
    )
/*++

Routine Description:
    Locks the static child list for iteration.  Any adds or removes that occur
    while the list is locked are pended until it is unlocked.  Locking can
    be nested.  When the unlock count is zero, the changes are evaluated.

Arguments:
    Fdo - the parent who owns the list of static children

Return Value:
    None

  --*/

{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    WDF_CHILD_LIST_ITERATOR iterator;
    FxDevice* pDevice;
    FxPkgFdo* pPkgFdo;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Fdo,
                                   FX_TYPE_DEVICE,
                                   (PVOID*)&pDevice,
                                   &pFxDriverGlobals);

    //
    // Verify type
    //
    if (pDevice->IsLegacy() || (pDevice->IsFdo() == FALSE)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Invalid WDFDEVICE %p is not an FDO", Fdo);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    pPkgFdo = pDevice->GetFdoPkg();

    //
    // Create a fake iterator to begin iteration.  We will not need it in the
    // retrieve next static child call.
    //
    WDF_CHILD_LIST_ITERATOR_INIT(&iterator, WdfRetrieveAllChildren);

    pPkgFdo->m_StaticDeviceList->BeginIteration(&iterator);
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
WDFDEVICE
STDCALL
WDFEXPORT(WdfFdoRetrieveNextStaticChild)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Fdo,
    __in
    WDFDEVICE PreviousChild,
    __in
    ULONG Flags
    )
/*++

Routine Description:
    Returns the next static child in the list based on the PreviousChild and
    Flags values.

Arguments:
    Fdo - the parent which owns the static child list

    PreviousChild - The child returned on the last call to this DDI.  If NULL,
        returns the first static child specified by the Flags criteria

    Flags - combination of values from WDF_RETRIEVE_CHILD_FLAGS.
        WdfRetrievePresentChildren - reports children who have been reported as
                                     present to pnp
        WdfRetrieveMissingChildren = reports children who have been reported as
                                     missing to pnp
        WdfRetrievePendingChildren = reports children who have been added to the
                                     list, but not yet reported to pnp

Return Value:
    next WDFDEVICE handle representing a static child or NULL

  --*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDevice* pDevice;
    FxPkgFdo* pPkgFdo;
    WDFDEVICE next;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Fdo,
                                   FX_TYPE_DEVICE,
                                   (PVOID*)&pDevice,
                                   &pFxDriverGlobals);

    //
    // validate Flags
    //
    if (Flags == 0 || (Flags & ~WdfRetrieveAllChildren) != 0) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Invalid Flags 0x%x", Flags);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return NULL;
    }

    //
    // Verify type
    //
    if (pDevice->IsLegacy() || (pDevice->IsFdo() == FALSE)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFDEVICE %p is not an FDO", Fdo);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return NULL;
    }

    pPkgFdo = pDevice->GetFdoPkg();

    next = pPkgFdo->m_StaticDeviceList->GetNextStaticDevice(PreviousChild, Flags);

    return next;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfFdoUnlockStaticChildListFromIteration)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Fdo
    )
/*++

Routine Description:
    Unlocks the static child list from iteration.  Upon the last unlock, any
    pended modifications to the list will be applied.

Arguments:
    Fdo - the parent who owns the static child list

Return Value:
    None

  --*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    WDF_CHILD_LIST_ITERATOR iterator;
    FxDevice* pDevice;
    FxPkgFdo* pPkgFdo;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Fdo,
                                   FX_TYPE_DEVICE,
                                   (PVOID*)&pDevice,
                                   &pFxDriverGlobals);

    //
    // Verify type
    //
    if (pDevice->IsLegacy() || (pDevice->IsFdo() == FALSE)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFDEVICE %p is not an FDO", Fdo);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    //
    // Local iterator used to end iteration.  WdfRetrieveAllChildren is an
    // arbitrary value.
    //
    WDF_CHILD_LIST_ITERATOR_INIT(&iterator, WdfRetrieveAllChildren);

    pPkgFdo = pDevice->GetFdoPkg();
    pPkgFdo->m_StaticDeviceList->EndIteration(&iterator);
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfFdoQueryForInterface)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Fdo,
    __in
    LPCGUID InterfaceType,
    __out
    PINTERFACE Interface,
    __in
    USHORT Size,
    __in
    USHORT Version,
    __in_opt
    PVOID InterfaceSpecificData
    )
/*++

Routine Description:
    Sends a query interface pnp request to the top of the stack (which means that
    the request will travel through this device before going down the stack).

Arguments:
    Fdo - the device stack which is being queried

    InterfaceType - interface type specifier

    Interface - Interface block which will be filled in by the component which
                responds to the query interface

    Size - size in bytes of Interfce

    Version - version of InterfaceType being requested

    InterfaceSpecificData - Additional data associated with Interface

Return Value:
    NTSTATUS

  --*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDeviceBase* pDeviceBase;
    FxDevice* pDevice;
    FxQueryInterfaceParams params = { (PVOID*) &pDevice, FX_TYPE_DEVICE, 0 };
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Fdo,
                                   FX_TYPE_DEVICE_BASE,
                                   (PVOID*)&pDeviceBase,
                                   &pFxDriverGlobals);

    pDevice = NULL;

    FxPointerNotNull(pFxDriverGlobals, InterfaceType);
    FxPointerNotNull(pFxDriverGlobals, Interface);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // See if we have a full fledged WDFDEVICE or a miniport WDFDEVICE
    //
    if (NT_SUCCESS(pDeviceBase->QueryInterface(&params))) {
        //
        // Verify type
        //
        if (pDevice->IsLegacy() || (pDevice->IsFdo() == FALSE)) {
            status = STATUS_INVALID_PARAMETER;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "WDFDEVICE 0x%p Device is either legacy or is not a Fdo %!STATUS!",
                Fdo, status);

            return status;
        }
    }
    else {
        //
        // miniport WDFDEVICE, nothing to check
        //
        DO_NOTHING();
    }

    return pDeviceBase->QueryForInterface(
        InterfaceType, Interface, Size, Version, InterfaceSpecificData);
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFCHILDLIST
STDCALL
WDFEXPORT(WdfFdoGetDefaultChildList)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Fdo
    )
/*++

Routine Description:
    Returns the default dynamic child list associated with the FDO.   For a
    valid handle to be returned, the driver must have configured the default
    list in its device init phase.

Arguments:
    Fdo - the FDO being asked to return the handle

Return Value:
    a valid handle value or NULL

  --*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDevice* pDevice;
    FxPkgFdo* pPkgFdo;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Fdo,
                                   FX_TYPE_DEVICE,
                                   (PVOID*)&pDevice,
                                   &pFxDriverGlobals);

    //
    // Verify type
    //
    if (pDevice->IsLegacy() || (pDevice->IsFdo() == FALSE)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFDEVICE %p is not an FDO", Fdo);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return NULL;
    }

    pPkgFdo = pDevice->GetFdoPkg();

    if (pPkgFdo->m_DefaultDeviceList != NULL) {
        return (WDFCHILDLIST) pPkgFdo->m_DefaultDeviceList->GetObjectHandle();
    }
    else {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Default child list for FDO %p not configured, call "
            "WdfFdoInitSetDefaultChildListConfig to do so", Fdo);
        return NULL;
    }
}

} // extern "C"
