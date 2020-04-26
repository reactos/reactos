/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

_WdfVersionBuild_

Module Name:

    WdfPdo.h

Abstract:

    This is the interface to the PDO WDFDEVICE handle.

Environment:

    kernel mode only

Revision History:

--*/

#ifndef _WDFPDO_H_
#define _WDFPDO_H_



#if (NTDDI_VERSION >= NTDDI_WIN2K)



typedef
__drv_functionClass(EVT_WDF_DEVICE_RESOURCES_QUERY)
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
EVT_WDF_DEVICE_RESOURCES_QUERY(
    __in
    WDFDEVICE Device,
    __in
    WDFCMRESLIST Resources
    );

typedef EVT_WDF_DEVICE_RESOURCES_QUERY *PFN_WDF_DEVICE_RESOURCES_QUERY;

typedef
__drv_functionClass(EVT_WDF_DEVICE_RESOURCE_REQUIREMENTS_QUERY)
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
EVT_WDF_DEVICE_RESOURCE_REQUIREMENTS_QUERY(
    __in
    WDFDEVICE Device,
    __in
    WDFIORESREQLIST IoResourceRequirementsList
    );

typedef EVT_WDF_DEVICE_RESOURCE_REQUIREMENTS_QUERY *PFN_WDF_DEVICE_RESOURCE_REQUIREMENTS_QUERY;

typedef
__drv_functionClass(EVT_WDF_DEVICE_EJECT)
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
EVT_WDF_DEVICE_EJECT(
    __in
    WDFDEVICE Device
    );

typedef EVT_WDF_DEVICE_EJECT *PFN_WDF_DEVICE_EJECT;

typedef
__drv_functionClass(EVT_WDF_DEVICE_SET_LOCK)
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
EVT_WDF_DEVICE_SET_LOCK(
    __in
    WDFDEVICE Device,
    __in
    BOOLEAN IsLocked
    );

typedef EVT_WDF_DEVICE_SET_LOCK *PFN_WDF_DEVICE_SET_LOCK;

typedef
__drv_functionClass(EVT_WDF_DEVICE_ENABLE_WAKE_AT_BUS)
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
EVT_WDF_DEVICE_ENABLE_WAKE_AT_BUS(
    __in
    WDFDEVICE Device,
    __in
    SYSTEM_POWER_STATE PowerState
    );

typedef EVT_WDF_DEVICE_ENABLE_WAKE_AT_BUS *PFN_WDF_DEVICE_ENABLE_WAKE_AT_BUS;

typedef
__drv_functionClass(EVT_WDF_DEVICE_DISABLE_WAKE_AT_BUS)
__drv_sameIRQL
__drv_maxIRQL(PASSIVE_LEVEL)
VOID
EVT_WDF_DEVICE_DISABLE_WAKE_AT_BUS(
    __in
    WDFDEVICE Device
    );

typedef EVT_WDF_DEVICE_DISABLE_WAKE_AT_BUS *PFN_WDF_DEVICE_DISABLE_WAKE_AT_BUS;

typedef struct _WDF_PDO_EVENT_CALLBACKS {
    //
    // The size of this structure in bytes
    //
    ULONG Size;

    //
    // Called in response to IRP_MN_QUERY_RESOURCES
    //
    PFN_WDF_DEVICE_RESOURCES_QUERY EvtDeviceResourcesQuery;

    //
    // Called in response to IRP_MN_QUERY_RESOURCE_REQUIREMENTS
    //
    PFN_WDF_DEVICE_RESOURCE_REQUIREMENTS_QUERY EvtDeviceResourceRequirementsQuery;

    //
    // Called in response to IRP_MN_EJECT
    //
    PFN_WDF_DEVICE_EJECT EvtDeviceEject;

    //
    // Called in response to IRP_MN_SET_LOCK
    //
    PFN_WDF_DEVICE_SET_LOCK EvtDeviceSetLock;

    //
    // Called in response to the power policy owner sending a wait wake to the
    // PDO.  Bus generic arming shoulding occur here.
    //
    PFN_WDF_DEVICE_ENABLE_WAKE_AT_BUS       EvtDeviceEnableWakeAtBus;

    //
    // Called in response to the power policy owner sending a wait wake to the
    // PDO.  Bus generic disarming shoulding occur here.
    //
    PFN_WDF_DEVICE_DISABLE_WAKE_AT_BUS      EvtDeviceDisableWakeAtBus;

} WDF_PDO_EVENT_CALLBACKS, *PWDF_PDO_EVENT_CALLBACKS;

VOID
FORCEINLINE
WDF_PDO_EVENT_CALLBACKS_INIT(
    __out PWDF_PDO_EVENT_CALLBACKS Callbacks
    )
{
    RtlZeroMemory(Callbacks, sizeof(WDF_PDO_EVENT_CALLBACKS));
    Callbacks->Size = sizeof(WDF_PDO_EVENT_CALLBACKS);
}

//
// WDF Function: WdfPdoInitAllocate
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
PWDFDEVICE_INIT
(*PFN_WDFPDOINITALLOCATE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE ParentDevice
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
PWDFDEVICE_INIT
FORCEINLINE
WdfPdoInitAllocate(
    __in
    WDFDEVICE ParentDevice
    )
{
    return ((PFN_WDFPDOINITALLOCATE) WdfFunctions[WdfPdoInitAllocateTableIndex])(WdfDriverGlobals, ParentDevice);
}

//
// WDF Function: WdfPdoInitSetEventCallbacks
//
typedef
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
VOID
(*PFN_WDFPDOINITSETEVENTCALLBACKS)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PWDF_PDO_EVENT_CALLBACKS DispatchTable
    );

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
FORCEINLINE
WdfPdoInitSetEventCallbacks(
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PWDF_PDO_EVENT_CALLBACKS DispatchTable
    )
{
    ((PFN_WDFPDOINITSETEVENTCALLBACKS) WdfFunctions[WdfPdoInitSetEventCallbacksTableIndex])(WdfDriverGlobals, DeviceInit, DispatchTable);
}

//
// WDF Function: WdfPdoInitAssignDeviceID
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFPDOINITASSIGNDEVICEID)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PCUNICODE_STRING DeviceID
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfPdoInitAssignDeviceID(
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PCUNICODE_STRING DeviceID
    )
{
    return ((PFN_WDFPDOINITASSIGNDEVICEID) WdfFunctions[WdfPdoInitAssignDeviceIDTableIndex])(WdfDriverGlobals, DeviceInit, DeviceID);
}

//
// WDF Function: WdfPdoInitAssignInstanceID
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFPDOINITASSIGNINSTANCEID)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PCUNICODE_STRING InstanceID
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfPdoInitAssignInstanceID(
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PCUNICODE_STRING InstanceID
    )
{
    return ((PFN_WDFPDOINITASSIGNINSTANCEID) WdfFunctions[WdfPdoInitAssignInstanceIDTableIndex])(WdfDriverGlobals, DeviceInit, InstanceID);
}

//
// WDF Function: WdfPdoInitAddHardwareID
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFPDOINITADDHARDWAREID)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PCUNICODE_STRING HardwareID
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfPdoInitAddHardwareID(
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PCUNICODE_STRING HardwareID
    )
{
    return ((PFN_WDFPDOINITADDHARDWAREID) WdfFunctions[WdfPdoInitAddHardwareIDTableIndex])(WdfDriverGlobals, DeviceInit, HardwareID);
}

//
// WDF Function: WdfPdoInitAddCompatibleID
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFPDOINITADDCOMPATIBLEID)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PCUNICODE_STRING CompatibleID
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfPdoInitAddCompatibleID(
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PCUNICODE_STRING CompatibleID
    )
{
    return ((PFN_WDFPDOINITADDCOMPATIBLEID) WdfFunctions[WdfPdoInitAddCompatibleIDTableIndex])(WdfDriverGlobals, DeviceInit, CompatibleID);
}

//
// WDF Function: WdfPdoInitAssignContainerID
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFPDOINITASSIGNCONTAINERID)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PCUNICODE_STRING ContainerID
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfPdoInitAssignContainerID(
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PCUNICODE_STRING ContainerID
    )
{
    return ((PFN_WDFPDOINITASSIGNCONTAINERID) WdfFunctions[WdfPdoInitAssignContainerIDTableIndex])(WdfDriverGlobals, DeviceInit, ContainerID);
}

//
// WDF Function: WdfPdoInitAddDeviceText
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFPDOINITADDDEVICETEXT)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PCUNICODE_STRING DeviceDescription,
    __in
    PCUNICODE_STRING DeviceLocation,
    __in
    LCID LocaleId
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfPdoInitAddDeviceText(
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PCUNICODE_STRING DeviceDescription,
    __in
    PCUNICODE_STRING DeviceLocation,
    __in
    LCID LocaleId
    )
{
    return ((PFN_WDFPDOINITADDDEVICETEXT) WdfFunctions[WdfPdoInitAddDeviceTextTableIndex])(WdfDriverGlobals, DeviceInit, DeviceDescription, DeviceLocation, LocaleId);
}

//
// WDF Function: WdfPdoInitSetDefaultLocale
//
typedef
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
VOID
(*PFN_WDFPDOINITSETDEFAULTLOCALE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    LCID LocaleId
    );

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
FORCEINLINE
WdfPdoInitSetDefaultLocale(
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    LCID LocaleId
    )
{
    ((PFN_WDFPDOINITSETDEFAULTLOCALE) WdfFunctions[WdfPdoInitSetDefaultLocaleTableIndex])(WdfDriverGlobals, DeviceInit, LocaleId);
}

//
// WDF Function: WdfPdoInitAssignRawDevice
//
typedef
__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFPDOINITASSIGNRAWDEVICE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    CONST GUID* DeviceClassGuid
    );

__checkReturn
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
FORCEINLINE
WdfPdoInitAssignRawDevice(
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    CONST GUID* DeviceClassGuid
    )
{
    return ((PFN_WDFPDOINITASSIGNRAWDEVICE) WdfFunctions[WdfPdoInitAssignRawDeviceTableIndex])(WdfDriverGlobals, DeviceInit, DeviceClassGuid);
}

//
// WDF Function: WdfPdoInitAllowForwardingRequestToParent
//
typedef
__drv_maxIRQL(PASSIVE_LEVEL)
WDFAPI
VOID
(*PFN_WDFPDOINITALLOWFORWARDINGREQUESTTOPARENT)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit
    );

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
FORCEINLINE
WdfPdoInitAllowForwardingRequestToParent(
    __in
    PWDFDEVICE_INIT DeviceInit
    )
{
    ((PFN_WDFPDOINITALLOWFORWARDINGREQUESTTOPARENT) WdfFunctions[WdfPdoInitAllowForwardingRequestToParentTableIndex])(WdfDriverGlobals, DeviceInit);
}

//
// WDF Function: WdfPdoMarkMissing
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFPDOMARKMISSING)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfPdoMarkMissing(
    __in
    WDFDEVICE Device
    )
{
    return ((PFN_WDFPDOMARKMISSING) WdfFunctions[WdfPdoMarkMissingTableIndex])(WdfDriverGlobals, Device);
}

//
// WDF Function: WdfPdoRequestEject
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFPDOREQUESTEJECT)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfPdoRequestEject(
    __in
    WDFDEVICE Device
    )
{
    ((PFN_WDFPDOREQUESTEJECT) WdfFunctions[WdfPdoRequestEjectTableIndex])(WdfDriverGlobals, Device);
}

//
// WDF Function: WdfPdoGetParent
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
WDFDEVICE
(*PFN_WDFPDOGETPARENT)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    );

__drv_maxIRQL(DISPATCH_LEVEL)
WDFDEVICE
FORCEINLINE
WdfPdoGetParent(
    __in
    WDFDEVICE Device
    )
{
    return ((PFN_WDFPDOGETPARENT) WdfFunctions[WdfPdoGetParentTableIndex])(WdfDriverGlobals, Device);
}

//
// WDF Function: WdfPdoRetrieveIdentificationDescription
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFPDORETRIEVEIDENTIFICATIONDESCRIPTION)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __inout
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfPdoRetrieveIdentificationDescription(
    __in
    WDFDEVICE Device,
    __inout
    PWDF_CHILD_IDENTIFICATION_DESCRIPTION_HEADER IdentificationDescription
    )
{
    return ((PFN_WDFPDORETRIEVEIDENTIFICATIONDESCRIPTION) WdfFunctions[WdfPdoRetrieveIdentificationDescriptionTableIndex])(WdfDriverGlobals, Device, IdentificationDescription);
}

//
// WDF Function: WdfPdoRetrieveAddressDescription
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFPDORETRIEVEADDRESSDESCRIPTION)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __inout
    PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfPdoRetrieveAddressDescription(
    __in
    WDFDEVICE Device,
    __inout
    PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
    )
{
    return ((PFN_WDFPDORETRIEVEADDRESSDESCRIPTION) WdfFunctions[WdfPdoRetrieveAddressDescriptionTableIndex])(WdfDriverGlobals, Device, AddressDescription);
}

//
// WDF Function: WdfPdoUpdateAddressDescription
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFPDOUPDATEADDRESSDESCRIPTION)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __inout
    PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfPdoUpdateAddressDescription(
    __in
    WDFDEVICE Device,
    __inout
    PWDF_CHILD_ADDRESS_DESCRIPTION_HEADER AddressDescription
    )
{
    return ((PFN_WDFPDOUPDATEADDRESSDESCRIPTION) WdfFunctions[WdfPdoUpdateAddressDescriptionTableIndex])(WdfDriverGlobals, Device, AddressDescription);
}

//
// WDF Function: WdfPdoAddEjectionRelationsPhysicalDevice
//
typedef
__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
(*PFN_WDFPDOADDEJECTIONRELATIONSPHYSICALDEVICE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PDEVICE_OBJECT PhysicalDevice
    );

__checkReturn
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
FORCEINLINE
WdfPdoAddEjectionRelationsPhysicalDevice(
    __in
    WDFDEVICE Device,
    __in
    PDEVICE_OBJECT PhysicalDevice
    )
{
    return ((PFN_WDFPDOADDEJECTIONRELATIONSPHYSICALDEVICE) WdfFunctions[WdfPdoAddEjectionRelationsPhysicalDeviceTableIndex])(WdfDriverGlobals, Device, PhysicalDevice);
}

//
// WDF Function: WdfPdoRemoveEjectionRelationsPhysicalDevice
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFPDOREMOVEEJECTIONRELATIONSPHYSICALDEVICE)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PDEVICE_OBJECT PhysicalDevice
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfPdoRemoveEjectionRelationsPhysicalDevice(
    __in
    WDFDEVICE Device,
    __in
    PDEVICE_OBJECT PhysicalDevice
    )
{
    ((PFN_WDFPDOREMOVEEJECTIONRELATIONSPHYSICALDEVICE) WdfFunctions[WdfPdoRemoveEjectionRelationsPhysicalDeviceTableIndex])(WdfDriverGlobals, Device, PhysicalDevice);
}

//
// WDF Function: WdfPdoClearEjectionRelationsDevices
//
typedef
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
VOID
(*PFN_WDFPDOCLEAREJECTIONRELATIONSDEVICES)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    );

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
FORCEINLINE
WdfPdoClearEjectionRelationsDevices(
    __in
    WDFDEVICE Device
    )
{
    ((PFN_WDFPDOCLEAREJECTIONRELATIONSDEVICES) WdfFunctions[WdfPdoClearEjectionRelationsDevicesTableIndex])(WdfDriverGlobals, Device);
}



#endif // (NTDDI_VERSION >= NTDDI_WIN2K)


#endif // _WDFPDO_H_

