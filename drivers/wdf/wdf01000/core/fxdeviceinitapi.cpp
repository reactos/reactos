/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Device init api functions
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */


#include "common/fxglobals.h"
#include "common/fxdevice.h"
#include "common/fxdeviceinit.h"
#include "common/fxvalidatefunctions.h"


extern "C" {

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
NTAPI
WDFEXPORT(WdfDeviceInitSetPnpPowerEventCallbacks)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PWDF_PNPPOWER_EVENT_CALLBACKS PnpPowerEventCallbacks
    )
{
    DDI_ENTRY();
        
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    FxPointerNotNull(pFxDriverGlobals, PnpPowerEventCallbacks);

    if (PnpPowerEventCallbacks->Size != sizeof(WDF_PNPPOWER_EVENT_CALLBACKS) &&
        PnpPowerEventCallbacks->Size != sizeof(WDF_PNPPOWER_EVENT_CALLBACKS_V1_9))
    {

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "PnpPowerEventCallbacks size %d is invalid, exptected %d",
            PnpPowerEventCallbacks->Size, sizeof(WDF_PNPPOWER_EVENT_CALLBACKS)
            );

        FxVerifierDbgBreakPoint(pFxDriverGlobals);

        return;
    }

    //
    // Make sure only one of the callbacks EvtDeviceUsageNotification or 
    // EvtDeviceUsageNotificationEx is provided by driver for >V1.9.
    //
    /*if (PnpPowerEventCallbacks->Size > sizeof(WDF_PNPPOWER_EVENT_CALLBACKS_V1_9) &&
        PnpPowerEventCallbacks->EvtDeviceUsageNotification != NULL &&
        PnpPowerEventCallbacks->EvtDeviceUsageNotificationEx != NULL)
    {

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Driver can provide either EvtDeviceUsageNotification or "
            "EvtDeviceUsageNotificationEx callback but not both");

        FxVerifierDbgBreakPoint(pFxDriverGlobals);

        return;
    }*/

    //
    // Driver's PnpPowerEventCallbacks structure may be from a previous 
    // version and therefore may be different in size than the current version 
    // that framework is using. Therefore, copy only PnpPowerEventCallbacks->Size
    // bytes and not sizeof(PnpPowerEventCallbacks) bytes.
    //
    RtlCopyMemory(&DeviceInit->PnpPower.PnpPowerEventCallbacks,
                  PnpPowerEventCallbacks,
                  PnpPowerEventCallbacks->Size);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
NTAPI
WDFEXPORT(WdfDeviceInitSetRequestAttributes)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PWDF_OBJECT_ATTRIBUTES RequestAttributes
    )
{
    DDI_ENTRY();
        
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    FxPointerNotNull(pFxDriverGlobals, RequestAttributes);

    //
    // Parent of all requests created from WDFDEVICE are parented by the
    // WDFDEVICE.
    //
    status = FxValidateObjectAttributes(pFxDriverGlobals,
                                        RequestAttributes,
                                        FX_VALIDATE_OPTION_PARENT_NOT_ALLOWED);

    if (!NT_SUCCESS(status))
    {
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    RtlCopyMemory(&DeviceInit->RequestAttributes,
                  RequestAttributes,
                  sizeof(WDF_OBJECT_ATTRIBUTES));
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
PWDFDEVICE_INIT
WDFEXPORT(WdfControlDeviceInitAllocate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDRIVER Driver,
    __in
    CONST UNICODE_STRING* SDDLString
    )
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
WDFEXPORT(WdfControlDeviceInitSetShutdownNotification)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PFN_WDF_DEVICE_SHUTDOWN_NOTIFICATION Notification,
    __in
    UCHAR Flags
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceInitFree)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceInitSetPowerPolicyEventCallbacks)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PWDF_POWER_POLICY_EVENT_CALLBACKS PowerPolicyEventCallbacks
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceInitSetPowerPolicyOwnership)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    BOOLEAN IsPowerPolicyOwner
    )
{
    WDFNOTIMPLEMENTED();
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceInitRegisterPnpStateChangeCallback)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    WDF_DEVICE_PNP_STATE PnpState,
    __in
    PFN_WDF_DEVICE_PNP_STATE_CHANGE_NOTIFICATION EvtDevicePnpStateChange,
    __in
    ULONG CallbackTypes
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceInitRegisterPowerStateChangeCallback)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    WDF_DEVICE_POWER_STATE PowerState,
    __in
    PFN_WDF_DEVICE_POWER_STATE_CHANGE_NOTIFICATION EvtDevicePowerStateChange,
    __in
    ULONG CallbackTypes
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceInitRegisterPowerPolicyStateChangeCallback)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    WDF_DEVICE_POWER_POLICY_STATE PowerPolicyState,
    __in
    PFN_WDF_DEVICE_POWER_POLICY_STATE_CHANGE_NOTIFICATION EvtDevicePowerPolicyStateChange,
    __in
    ULONG CallbackTypes
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceInitSetExclusive)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    BOOLEAN Exclusive
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceInitSetIoType)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    WDF_DEVICE_IO_TYPE IoType
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceInitSetPowerNotPageable)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceInitSetPowerPageable)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceInitSetPowerInrush)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceInitSetDeviceType)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    DEVICE_TYPE DeviceType
    )
{
    WDFNOTIMPLEMENTED();
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceInitAssignName)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in_opt
    PCUNICODE_STRING DeviceName
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceInitAssignSDDLString)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in_opt
    PCUNICODE_STRING SDDLString
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceInitSetDeviceClass)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    CONST GUID* DeviceClassGuid
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceInitSetCharacteristics)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    ULONG DeviceCharacteristics,
    __in
    BOOLEAN OrInValues
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceInitSetFileObjectConfig)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PWDF_FILEOBJECT_CONFIG FileObjectConfig,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES FileObjectAttributes
    )

/*++

Routine Description:

    Registers callbacks for file object support.

    Defaults to WdfDeviceFileObjectNoFsContext.

Arguments:

Returns:

--*/

{
    WDFNOTIMPLEMENTED();
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceInitAssignWdmIrpPreprocessCallback)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PFN_WDFDEVICE_WDM_IRP_PREPROCESS EvtDeviceWdmIrpPreprocess,
    __in
    UCHAR MajorFunction,
    __drv_when(NumMinorFunctions > 0, __in_bcount(NumMinorFunctions))
    __drv_when(NumMinorFunctions == 0, __in_opt)
    PUCHAR MinorFunctions,
    __in
    ULONG NumMinorFunctions
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceInitSetIoInCallerContextCallback)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PFN_WDF_IO_IN_CALLER_CONTEXT EvtIoInCallerContext
    )

/*++

Routine Description:

    Registers an I/O pre-processing callback for the device.

    If registered, any I/O for the device is first presented to this
    callback function before being placed in any I/O Queue's.

    The callback is invoked in the thread and/or DPC context of the
    original WDM caller as presented to the I/O package. No framework
    threading, locking, synchronization, or queuing occurs, and
    responsibility for synchronization is up to the device driver.

    This API is intended to support METHOD_NEITHER IRP_MJ_DEVICE_CONTROL's
    which must access the user buffer in the original callers context. The
    driver would probe and lock the buffer pages from within this event
    handler using the functions supplied on the WDFREQUEST object, storing
    any required mapped buffers and/or pointers on the WDFREQUEST context
    whose size is set by the RequestContextSize of the WDF_DRIVER_CONFIG structure.

    It is the responsibility of this routine to either complete the request, or
    pass it on to the I/O package through WdfDeviceEnqueueRequest(Device, Request).

Arguments:
    DeviceInit - Device initialization structure
    
    EvtIoInCallerContext - Pointer to driver supplied callback function

Return Value:

--*/
{
    WDFNOTIMPLEMENTED();
}


//
// BEGIN FDO specific functions
//

__drv_maxIRQL(DISPATCH_LEVEL)
PDEVICE_OBJECT
WDFEXPORT(WdfFdoInitWdmGetPhysicalDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit
    )
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfFdoInitOpenRegistryKey)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    ULONG DeviceInstanceKeyType,
    __in
    ACCESS_MASK DesiredAccess,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES KeyAttributes,
    __out
    WDFKEY* Key
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfFdoInitQueryProperty)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    DEVICE_REGISTRY_PROPERTY  DeviceProperty,
    __in
    ULONG BufferLength,
    __out_bcount_full_opt(BufferLength)
    PVOID PropertyBuffer,
    __out
    PULONG ResultLength
    )
/*++

Routine Description:
    Retrieves the requested device property for the given device

Arguments:
    DeviceInit - Device initialization structure

    DeviceProperty - the property being queried

    BufferLength - length of PropertyBuffer in bytes

    PropertyBuffer - Buffer which will receive the property being queried

    ResultLength - if STATUS_BUFFER_TOO_SMALL is returned, then this will contain
                   the required length

Return Value:
    NTSTATUS

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfFdoInitAllocAndQueryProperty)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    DEVICE_REGISTRY_PROPERTY DeviceProperty,
    __in
    __drv_strictTypeMatch(__drv_typeExpr)
    POOL_TYPE PoolType,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES PropertyMemoryAttributes,
    __out
    WDFMEMORY* PropertyMemory
    )
/*++

Routine Description:
    Allocates and retrieves the requested device property for the given device init

Arguments:
    DeviceInit - Device initialization structure

    DeviceProperty - the property being queried

    PoolType - what type of pool to allocate

    PropertyMemoryAttributes - attributes to associate with PropertyMemory

    PropertyMemory - handle which will receive the property buffer

Return Value:
    NTSTATUS

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
WDFEXPORT(WdfFdoInitSetEventCallbacks)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PWDF_FDO_EVENT_CALLBACKS FdoEventCallbacks
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
WDFEXPORT(WdfFdoInitSetFilter)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
WDFEXPORT(WdfFdoInitSetDefaultChildListConfig)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __inout
    PWDFDEVICE_INIT DeviceInit,
    __in
    PWDF_CHILD_LIST_CONFIG Config,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES DefaultDeviceListAttributes
    )
{
    WDFNOTIMPLEMENTED();
}

//
// END FDO specific functions
//

//
// BEGIN PDO specific functions
//
_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
PWDFDEVICE_INIT
WDFEXPORT(WdfPdoInitAllocate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE ParentDevice
    )
{
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
WDFEXPORT(WdfPdoInitSetEventCallbacks)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PWDF_PDO_EVENT_CALLBACKS DispatchTable
    )
{
    WDFNOTIMPLEMENTED();
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfPdoInitAssignDeviceID)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PCUNICODE_STRING DeviceID
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfPdoInitAssignInstanceID)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PCUNICODE_STRING InstanceID
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfPdoInitAddHardwareID)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PCUNICODE_STRING HardwareID
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfPdoInitAddCompatibleID)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PCUNICODE_STRING CompatibleID
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfPdoInitAssignContainerID)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PCUNICODE_STRING ContainerID
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfPdoInitAddDeviceText)(
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
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
WDFEXPORT(WdfPdoInitSetDefaultLocale)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    LCID LocaleId
    )
{
    WDFNOTIMPLEMENTED();
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfPdoInitAssignRawDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    CONST GUID* DeviceClassGuid
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
WDFEXPORT(WdfPdoInitAllowForwardingRequestToParent)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit
    )
{
    WDFNOTIMPLEMENTED();
}



//
// END PDO specific functions
//

} // extern "C"