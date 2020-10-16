/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxIoTargetAPIKm.cpp

Abstract:

    This module implements the IO Target APIs

Author:

Environment:

    kernel mode only

Revision History:

--*/

#include "../../fxtargetsshared.hpp"

extern "C" {
// #include "FxIoTargetAPIKm.tmh"
}

//
// Extern the entire file
//
extern "C" {

__drv_maxIRQL(DISPATCH_LEVEL)
PDEVICE_OBJECT
STDCALL
WDFEXPORT(WdfIoTargetWdmGetTargetDeviceObject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget
    )
/*++

Routine Description:
    Returns the PDEVICE_OBJECT.  This is the device which PIRPs are sent to.
    This is not necessarily the PDEVICE_OBJECT that WDFDEVICE is attached to.

Arguments:
    IoTarget - target whose WDM device object is being returned

Return Value:
    valid PDEVICE_OBJECT or NULL on failure

  --*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxIoTarget* pTarget;
    PDEVICE_OBJECT pDevice;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   IoTarget,
                                   FX_TYPE_IO_TARGET,
                                   (PVOID*) &pTarget,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "enter WDFIOTARGET 0x%p", IoTarget);

    pDevice = pTarget->GetTargetDevice();

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "exit WDFIOTARGET 0x%p, WDM DevObj 0x%p", IoTarget, pDevice);

    return pDevice;
}

__drv_maxIRQL(DISPATCH_LEVEL)
PDEVICE_OBJECT
STDCALL
WDFEXPORT(WdfIoTargetWdmGetTargetPhysicalDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget
    )
/*++

Routine Description:
    Returns the PDO for the target itself.  This is not necessarily the same
    PDO as the WDFDEVICE that owns the target.  Not all targets have a PDO since
    you can open a legacy non pnp PDEVICE_OBJECT which does not have one.

Arguments:
    IoTarget - target whose PDO is being returned

Return Value:
    A valid PDEVICE_OBJECT or NULL upon success

  --*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxIoTarget* pTarget;
    PDEVICE_OBJECT pPdo;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   IoTarget,
                                   FX_TYPE_IO_TARGET,
                                   (PVOID*) &pTarget,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "enter WDFIOTARGET 0x%p", IoTarget);

    pPdo = pTarget->GetTargetPDO();

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "exit WDFIOTARGET 0x%p, WDM PDO 0x%p", IoTarget, pPdo);

    return pPdo;
}


__drv_maxIRQL(DISPATCH_LEVEL)
PFILE_OBJECT
STDCALL
WDFEXPORT(WdfIoTargetWdmGetTargetFileObject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget
    )
/*++

Routine Description:
    Returns the PFILE_OBJECT associated with the target.  Not all targets have
    an underlying file object so NULL is a valid and successful return value.

Arguments:
    IoTarget - the target whose fileobject is being returned

Return Value:
    a valid PFILE_OBJECT or NULL upon success

  --*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxIoTarget* pTarget;
    MdFileObject pFile;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   IoTarget,
                                   FX_TYPE_IO_TARGET,
                                   (PVOID*) &pTarget,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "enter WDFIOTARGET 0x%p", IoTarget);

    pFile = pTarget->GetTargetFileObject();

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "exit WDFIOTARGET 0x%p, WDM FileObj 0x%p", IoTarget, pFile);

    return pFile;
}

__drv_maxIRQL(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
STDCALL
WDFEXPORT(WdfIoTargetQueryForInterface)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
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
    Sends a query interface pnp request to the top of the target's stack.

Arguments:
    IoTarget - the target which is being queried

    InterfaceType - interface type specifier

    Interface - Interface block which will be filled in by the component which
                responds to the query interface

    Size - size in bytes of Interface

    Version - version of InterfaceType being requested

    InterfaceSpecificData - Additional data associated with Interface

Return Value:
    NTSTATUS

  --*/
{
    FxIoTarget* pTarget;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    PDEVICE_OBJECT pTopOfStack;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   IoTarget,
                                   FX_TYPE_IO_TARGET,
                                   (PVOID*) &pTarget,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, InterfaceType);
    FxPointerNotNull(pFxDriverGlobals, Interface);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    pTopOfStack = IoGetAttachedDeviceReference(pTarget->GetTargetDevice());
    ASSERT(pTopOfStack != NULL);

    status = FxQueryInterface::_QueryForInterface(pTopOfStack,
                                                  InterfaceType,
                                                  Interface,
                                                  Size,
                                                  Version,
                                                  InterfaceSpecificData);

    ObDereferenceObject(pTopOfStack);

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfIoTargetQueryTargetProperty)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in
    DEVICE_REGISTRY_PROPERTY  DeviceProperty,
    __in
    ULONG BufferLength,
    __drv_when(BufferLength != 0, __out_bcount_part_opt(BufferLength, *ResultLength))
    __drv_when(BufferLength == 0, __out_opt)
    PVOID PropertyBuffer,
    __deref_out_range(<=,BufferLength)
    PULONG ResultLength
    )
/*++

Routine Description:
    Retrieves the requested device property for the given target

Arguments:
    IoTarget - the target whose PDO whose will be queried

    DeviceProperty - the property being queried

    BufferLength - length of PropertyBuffer in bytes

    PropertyBuffer - Buffer which will receive the property being queried

    ResultLength - if STATUS_BUFFER_TOO_SMALL is returned, then this will contain
                   the required length

Return Value:
    NTSTATUS

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pGlobals;
    NTSTATUS status;
    FxIoTarget* pTarget;
    MdDeviceObject pPdo;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   IoTarget,
                                   FX_TYPE_IO_TARGET,
                                   (PVOID*) &pTarget,
                                   &pGlobals);

    FxPointerNotNull(pGlobals, ResultLength);
    if (BufferLength > 0) {
        FxPointerNotNull(pGlobals, PropertyBuffer);
    }

    status = FxVerifierCheckIrqlLevel(pGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    pPdo = pTarget->GetTargetPDO();

    if (pPdo == NULL) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(pGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "WDFIOTARGET 0x%p has no PDO (not opened yet?), %!STATUS!",
                            IoTarget, status);

        return status;
    }

    status = FxDevice::_GetDeviceProperty(pPdo,
                                          DeviceProperty,
                                          BufferLength,
                                          PropertyBuffer,
                                          ResultLength);

    DoTraceLevelMessage(pGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "exit WDFIOTARGET 0x%p, Property %d, %!STATUS!",
                        IoTarget, DeviceProperty, status);



    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
STDCALL
WDFEXPORT(WdfIoTargetAllocAndQueryTargetProperty)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in
    DEVICE_REGISTRY_PROPERTY DeviceProperty,
    __in
    __drv_strictTypeMatch(1)
    POOL_TYPE PoolType,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES PropertyMemoryAttributes,
    __out
    WDFMEMORY*  PropertyMemory
    )
/*++

Routine Description:
    Allocates and retrieves the requested device property for the given target

Arguments:
    IoTarget - the target whose PDO whose will be queried

    DeviceProperty - the property being queried

    PoolType - what type of pool to allocate

    PropertyMemoryAttributes - attributes to associate with PropertyMemory

    PropertyMemory - handle which will receive the property buffer

Return Value:
    NTSTATUS

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxIoTarget* pTarget;
    MdDeviceObject pPdo;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   IoTarget,
                                   FX_TYPE_IO_TARGET,
                                   (PVOID*) &pTarget,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, PropertyMemory);

    *PropertyMemory = NULL;

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    FxVerifierCheckNxPoolType(pFxDriverGlobals, PoolType, pFxDriverGlobals->Tag);

    status = FxValidateObjectAttributes(pFxDriverGlobals, PropertyMemoryAttributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    pPdo = pTarget->GetTargetPDO();

    if (pPdo == NULL) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "WDFIOTARGET %p has no PDO (not opened yet?), %!STATUS!",
                            IoTarget, status);

        return status;
    }

    //
    // Worker function which does the 2 passes.  First pass to query the size,
    // the second pass w/the correctly sized buffer.
    //
    status = FxDevice::_AllocAndQueryProperty(pFxDriverGlobals,
                                              NULL,
                                              NULL,
                                              pPdo,
                                              DeviceProperty,
                                              PoolType,
                                              PropertyMemoryAttributes,
                                              PropertyMemory);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "exit WDFIOTARGET 0x%p, Property %d, %!STATUS!",
                        IoTarget, DeviceProperty, status);

    return status;
}

}
