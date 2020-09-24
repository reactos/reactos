/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDevicePdoApi.cpp

Abstract:

    This module exposes the "C" interface to the FxDevice object.

Author:



Environment:

    Kernel mode only

Revision History:

--*/

#include "fxcorepch.hpp"

extern "C" {
#include "FxDevicePdoApi.tmh"
}

NTSTATUS
GetPdoPackageFromDeviceHandle(
    __in
    IN  PFX_DRIVER_GLOBALS CallersGlobals,
    __in
    WDFDEVICE Device,
    __in
    PCHAR FunctionName,
    __out
    FxPkgPdo **Package,
    __out
    PFX_DRIVER_GLOBALS* ObjectGlobals,
    __out_opt
    FxDevice **OutDevice = NULL
    )
{
    NTSTATUS status;
    FxDevice *pDevice;

    FxObjectHandleGetPtrAndGlobals(CallersGlobals,
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID*) &pDevice,
                                   ObjectGlobals);

    //
    // If the optional "OutDevice" argument is present, return the pointer
    // to the device.
    //
    if (OutDevice != NULL) {
        *OutDevice = pDevice;
    }

    //
    // Check to see if a PDO package is installed on the device.
    //
    if (pDevice->IsPdo()) {
        *Package = (FxPkgPdo *) pDevice->GetPdoPkg();
        status = STATUS_SUCCESS;
    }
    else {
        DoTraceLevelMessage((*ObjectGlobals), TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "%s: Incorrect device handle supplied (0x%p).  "
                            "Device is not a PDO.", FunctionName, Device);

        status = STATUS_INVALID_PARAMETER;
    }

    return status;
}

//
// Extern "C" the rest of the file file
//
extern "C" {

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfPdoMarkMissing)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxPkgPdo *pPkgPdo;
    FxDevice *pDevice;

    status = GetPdoPackageFromDeviceHandle(GetFxDriverGlobals(DriverGlobals),
                                           Device,
                                           __FUNCTION__,
                                           &pPkgPdo,
                                           &pFxDriverGlobals,
                                           &pDevice);

    if (NT_SUCCESS(status)) {
        //
        // Check to see if the device is enumerated off of a child list.  If so,
        // have the child list object perform any necessary actions.
        //
        status = pPkgPdo->m_OwningChildList->UpdateDeviceAsMissing(pDevice);
    }

    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfPdoRequestEject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
{
    NTSTATUS status;
    FxPkgPdo *pPkgPdo;
    FxDevice *pDevice;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    status = GetPdoPackageFromDeviceHandle(GetFxDriverGlobals(DriverGlobals),
                                           Device,
                                           __FUNCTION__,
                                           &pPkgPdo,
                                           &pFxDriverGlobals,
                                           &pDevice);

    if (NT_SUCCESS(status)) {
        PDEVICE_OBJECT pdo;

        pdo = pDevice->GetSafePhysicalDevice();

        if (pdo != NULL) {
            IoRequestDeviceEject(pdo);
        }
        else {
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                                "PDO WDFDEVICE %p not reported yet to pnp, cannot eject!",
                                Device);
            FxVerifierDbgBreakPoint(pFxDriverGlobals);
        }
    }
    else {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Can only eject PDOs, %!STATUS!", status);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
    }
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFDEVICE
WDFEXPORT(WdfPdoGetParent)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxPkgPdo *pPkgPdo;
    FxDevice *pDevice;

    status = GetPdoPackageFromDeviceHandle(GetFxDriverGlobals(DriverGlobals),
                                           Device,
                                           __FUNCTION__,
                                           &pPkgPdo,
                                           &pFxDriverGlobals,
                                           &pDevice);

    if (NT_SUCCESS(status)) {
        return pDevice->m_ParentDevice->GetHandle();
    }
    else {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Can only eject PDOs, %!STATUS!", status);

        return NULL;
    }
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfPdoRetrieveIdentificationDescription)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __inout
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxPkgPdo *pPkgPdo;

    status = GetPdoPackageFromDeviceHandle(GetFxDriverGlobals(DriverGlobals),
                                           Device,
                                           __FUNCTION__,
                                           &pPkgPdo,
                                           &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, IdentificationDescription);

    if (NT_SUCCESS(status)) {
        FxChildList* pList;

        if (pPkgPdo->m_Description == NULL) {
            return STATUS_INVALID_DEVICE_REQUEST;
        }

        pList = pPkgPdo->m_Description->GetParentList();

        //
        // The fact that a statically enumerated PDO is enumerated using an
        // FxChildList should not be exposed to the driver.  Besides, the driver
        // does not know the definition of the identificaiton descirption anyways.
        //
        if (pList->IsStaticList() ||
            pList->GetIdentificationDescriptionSize() !=
                    IdentificationDescription->IdentificationDescriptionSize) {
            return STATUS_INVALID_DEVICE_REQUEST;
        }

        pList->CopyId(IdentificationDescription,
                      pPkgPdo->m_Description->GetId());

        status = STATUS_SUCCESS;
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfPdoRetrieveAddressDescription)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __inout
    PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxPkgPdo *pPkgPdo;

    status = GetPdoPackageFromDeviceHandle(GetFxDriverGlobals(DriverGlobals),
                                           Device,
                                           __FUNCTION__,
                                           &pPkgPdo,
                                           &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, AddressDescription);

    if (NT_SUCCESS(status)) {
        FxChildList* pList;

        if (pPkgPdo->m_Description == NULL) {
            return STATUS_INVALID_DEVICE_REQUEST;
        }

        pList = pPkgPdo->m_Description->GetParentList();

        //
        // A call to pList->IsStaticList() in the if below is not needed because
        // a static list does not have address descriptions.   Make sure this
        // assumption is not violated through the ASSERT().
        //
        ASSERT(pList->IsStaticList() == FALSE);

        if (pList->HasAddressDescriptions() == FALSE ||
            pList->GetAddressDescriptionSize() !=
                                AddressDescription->AddressDescriptionSize) {
            return STATUS_INVALID_DEVICE_REQUEST;
        }

        pList->GetAddressDescriptionFromEntry(pPkgPdo->m_Description,
                                              AddressDescription);

        status = STATUS_SUCCESS;
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfPdoUpdateAddressDescription)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __inout
    PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxPkgPdo *pPkgPdo;

    status = GetPdoPackageFromDeviceHandle(GetFxDriverGlobals(DriverGlobals),
                                           Device,
                                           __FUNCTION__,
                                           &pPkgPdo,
                                           &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, AddressDescription);

    if (NT_SUCCESS(status)) {
        FxChildList* pList;

        if (pPkgPdo->m_Description == NULL) {
            return STATUS_INVALID_DEVICE_REQUEST;
        }

        pList = pPkgPdo->m_Description->GetParentList();

        if (pList->GetAddressDescriptionSize() !=
                                AddressDescription->AddressDescriptionSize) {
            return STATUS_INVALID_DEVICE_REQUEST;
        }

        pList->UpdateAddressDescriptionFromEntry(pPkgPdo->m_Description,
                                                 AddressDescription);

        status = STATUS_SUCCESS;
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfPdoAddEjectionRelationsPhysicalDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PDEVICE_OBJECT PhysicalDevice
    )
/*++

Routine Description:
    Registers a PDO from another non descendant (not verifiable though) pnp
    stack to be reported as also requiring eject when this PDO is ejected.

    The PDO could be another device enumerated by this driver.

Arguments:
    Device - the PDO for this driver

    PhysicalDevice - PDO for the other stack

Return Value:
    NTSTATUS

  --*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxPkgPdo* pPkgPdo;
    NTSTATUS status;

    status = GetPdoPackageFromDeviceHandle(GetFxDriverGlobals(DriverGlobals),
                                           Device,
                                           __FUNCTION__,
                                           &pPkgPdo,
                                           &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, PhysicalDevice);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = pPkgPdo->AddEjectionDevice(PhysicalDevice);

    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfPdoRemoveEjectionRelationsPhysicalDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PDEVICE_OBJECT PhysicalDevice
    )
/*++

Routine Description:
    Deregisters a PDO from another non descendant (not verifiable though) pnp
    stack so that it will not be reported as also requiring eject when this PDO
    is ejected.

Arguments:
    Device - the PDO for this driver

    PhysicalDevice - PDO for the other stack

Return Value:
    None

  --*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxPkgPdo* pPkgPdo;
    NTSTATUS status;

    status = GetPdoPackageFromDeviceHandle(GetFxDriverGlobals(DriverGlobals),
                                           Device,
                                           __FUNCTION__,
                                           &pPkgPdo,
                                           &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, PhysicalDevice);

    if (!NT_SUCCESS(status)) {
        return; // status;
    }

    pPkgPdo->RemoveEjectionDevice(PhysicalDevice);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfPdoClearEjectionRelationsDevices)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
/*++

Routine Description:
    Deregisters all PDOs so that they will not be reported as also requiring
    eject when this PDO is ejected.

Arguments:
    Device - this driver's PDO

Return Value:
    None

  --*/

{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxPkgPdo* pPkgPdo;
    NTSTATUS status;

    status = GetPdoPackageFromDeviceHandle(GetFxDriverGlobals(DriverGlobals),
                                           Device,
                                           __FUNCTION__,
                                           &pPkgPdo,
                                           &pFxDriverGlobals);

    if (!NT_SUCCESS(status)) {
        return; // status;
    }

    pPkgPdo->ClearEjectionDevicesList();
}

} // extern "C"

