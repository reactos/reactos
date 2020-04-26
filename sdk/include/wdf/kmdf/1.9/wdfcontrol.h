/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

_WdfVersionBuild_

Module Name:

    wdfcontrol.h

Abstract:

    Defines functions for controller and creating a "controller" NT4 style
    WDFDEVICE handle.

Environment:

    kernel mode only

Revision History:

--*/

#ifndef _WDFCONTROL_H_
#define _WDFCONTROL_H_



#if (NTDDI_VERSION >= NTDDI_WIN2K)

typedef
__drv_functionClass(EVT_WDF_DEVICE_SHUTDOWN_NOTIFICATION)
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
VOID
EVT_WDF_DEVICE_SHUTDOWN_NOTIFICATION(
    __in
    WDFDEVICE Device
    );

typedef EVT_WDF_DEVICE_SHUTDOWN_NOTIFICATION *PFN_WDF_DEVICE_SHUTDOWN_NOTIFICATION;

typedef enum _WDF_DEVICE_SHUTDOWN_FLAGS {
    WdfDeviceShutdown = 0x01,
    WdfDeviceLastChanceShutdown = 0x02,
} WDF_DEVICE_SHUTDOWN_FLAGS;



//
// WDF Function: WdfControlDeviceInitAllocate
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
PWDFDEVICE_INIT
(*PFN_WDFCONTROLDEVICEINITALLOCATE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDRIVER Driver,
    __in
    CONST UNICODE_STRING* SDDLString
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
PWDFDEVICE_INIT
FORCEINLINE
WdfControlDeviceInitAllocate(
    __in
    WDFDRIVER Driver,
    __in
    CONST UNICODE_STRING* SDDLString
    )
{
    return ((PFN_WDFCONTROLDEVICEINITALLOCATE) WdfFunctions[WdfControlDeviceInitAllocateTableIndex])(WdfDriverGlobals, Driver, SDDLString);
}

//
// WDF Function: WdfControlDeviceInitSetShutdownNotification
//
typedef
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
VOID
(*PFN_WDFCONTROLDEVICEINITSETSHUTDOWNNOTIFICATION)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PFN_WDF_DEVICE_SHUTDOWN_NOTIFICATION Notification,
    __in
    UCHAR Flags
    );

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
FORCEINLINE
WdfControlDeviceInitSetShutdownNotification(
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PFN_WDF_DEVICE_SHUTDOWN_NOTIFICATION Notification,
    __in
    UCHAR Flags
    )
{
    ((PFN_WDFCONTROLDEVICEINITSETSHUTDOWNNOTIFICATION) WdfFunctions[WdfControlDeviceInitSetShutdownNotificationTableIndex])(WdfDriverGlobals, DeviceInit, Notification, Flags);
}

//
// WDF Function: WdfControlFinishInitializing
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFCONTROLFINISHINITIALIZING)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfControlFinishInitializing(
    __in
    WDFDEVICE Device
    )
{
    ((PFN_WDFCONTROLFINISHINITIALIZING) WdfFunctions[WdfControlFinishInitializingTableIndex])(WdfDriverGlobals, Device);
}



#endif // (NTDDI_VERSION >= NTDDI_WIN2K)


#endif // _WDFCONTROL_H_

