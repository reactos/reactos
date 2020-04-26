/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

_WdfVersionBuild_

Module Name:

    wdfminiport.h

Abstract:

    Interfaces for WDF usage in a miniport environment

Environment:

    kernel mode only

Revision History:

--*/

#ifndef _WDFMINIPORT_H_
#define _WDFMINIPORT_H_



#if (NTDDI_VERSION >= NTDDI_WIN2K)



//
// WDF Function: WdfDeviceMiniportCreate
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFDEVICEMINIPORTCREATE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDRIVER Driver,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __in
    PDEVICE_OBJECT DeviceObject,
    __in_opt
    PDEVICE_OBJECT AttachedDeviceObject,
    __in_opt
    PDEVICE_OBJECT Pdo,
    __out
    WDFDEVICE* Device
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfDeviceMiniportCreate(
    __in
    WDFDRIVER Driver,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __in
    PDEVICE_OBJECT DeviceObject,
    __in_opt
    PDEVICE_OBJECT AttachedDeviceObject,
    __in_opt
    PDEVICE_OBJECT Pdo,
    __out
    WDFDEVICE* Device
    )
{
    return ((PFN_WDFDEVICEMINIPORTCREATE) WdfFunctions[WdfDeviceMiniportCreateTableIndex])(WdfDriverGlobals, Driver, Attributes, DeviceObject, AttachedDeviceObject, Pdo, Device);
}

//
// WDF Function: WdfDriverMiniportUnload
//
typedef
WDFAPI
VOID
(*PFN_WDFDRIVERMINIPORTUNLOAD)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDRIVER Driver
    );

VOID
FORCEINLINE
WdfDriverMiniportUnload(
    __in
    WDFDRIVER Driver
    )
{
    ((PFN_WDFDRIVERMINIPORTUNLOAD) WdfFunctions[WdfDriverMiniportUnloadTableIndex])(WdfDriverGlobals, Driver);
}



#endif // (NTDDI_VERSION >= NTDDI_WIN2K)


#endif // _WDFMINIPORT_H_

