/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     PDO api functions
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */


#include "wdf.h"



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
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
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
    WDFNOTIMPLEMENTED();
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
    WDFNOTIMPLEMENTED();
    return NULL;
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
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
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
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
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
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
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
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
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
    WDFNOTIMPLEMENTED();
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
    WDFNOTIMPLEMENTED();
}

} // extern "C"
