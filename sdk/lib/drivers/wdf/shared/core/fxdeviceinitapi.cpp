/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDeviceInitApi.cpp

Abstract:

    This module exposes the "C" interface to the FxDevice object.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#include "coreprivshared.hpp"

extern "C" {
// #include "FxDeviceInitApi.tmh"
}

typedef struct _WDF_PNPPOWER_EVENT_CALLBACKS_V1_9 {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    PFN_WDF_DEVICE_D0_ENTRY                 EvtDeviceD0Entry;

    PFN_WDF_DEVICE_D0_ENTRY_POST_INTERRUPTS_ENABLED EvtDeviceD0EntryPostInterruptsEnabled;

    PFN_WDF_DEVICE_D0_EXIT                  EvtDeviceD0Exit;

    PFN_WDF_DEVICE_D0_EXIT_PRE_INTERRUPTS_DISABLED EvtDeviceD0ExitPreInterruptsDisabled;

    PFN_WDF_DEVICE_PREPARE_HARDWARE         EvtDevicePrepareHardware;

    PFN_WDF_DEVICE_RELEASE_HARDWARE         EvtDeviceReleaseHardware;

    PFN_WDF_DEVICE_SELF_MANAGED_IO_CLEANUP  EvtDeviceSelfManagedIoCleanup;

    PFN_WDF_DEVICE_SELF_MANAGED_IO_FLUSH    EvtDeviceSelfManagedIoFlush;

    PFN_WDF_DEVICE_SELF_MANAGED_IO_INIT     EvtDeviceSelfManagedIoInit;

    PFN_WDF_DEVICE_SELF_MANAGED_IO_SUSPEND  EvtDeviceSelfManagedIoSuspend;

    PFN_WDF_DEVICE_SELF_MANAGED_IO_RESTART  EvtDeviceSelfManagedIoRestart;

    PFN_WDF_DEVICE_SURPRISE_REMOVAL         EvtDeviceSurpriseRemoval;

    PFN_WDF_DEVICE_QUERY_REMOVE             EvtDeviceQueryRemove;

    PFN_WDF_DEVICE_QUERY_STOP               EvtDeviceQueryStop;

    PFN_WDF_DEVICE_USAGE_NOTIFICATION       EvtDeviceUsageNotification;

    PFN_WDF_DEVICE_RELATIONS_QUERY          EvtDeviceRelationsQuery;

} WDF_PNPPOWER_EVENT_CALLBACKS_V1_9, *PWDF_PNPPOWER_EVENT_CALLBACKS_V1_9;


typedef struct _WDF_POWER_POLICY_EVENT_CALLBACKS_V1_5 {
    //
    // Size of this structure in bytes
    //
    ULONG Size;

    PFN_WDF_DEVICE_ARM_WAKE_FROM_S0         EvtDeviceArmWakeFromS0;

    PFN_WDF_DEVICE_DISARM_WAKE_FROM_S0      EvtDeviceDisarmWakeFromS0;

    PFN_WDF_DEVICE_WAKE_FROM_S0_TRIGGERED   EvtDeviceWakeFromS0Triggered;

    PFN_WDF_DEVICE_ARM_WAKE_FROM_SX         EvtDeviceArmWakeFromSx;

    PFN_WDF_DEVICE_DISARM_WAKE_FROM_SX      EvtDeviceDisarmWakeFromSx;

    PFN_WDF_DEVICE_WAKE_FROM_SX_TRIGGERED   EvtDeviceWakeFromSxTriggered;

} WDF_POWER_POLICY_EVENT_CALLBACKS_V1_5, *PWDF_POWER_POLICY_EVENT_CALLBACKS_V1_5;

typedef struct _WDF_PDO_EVENT_CALLBACKS_V1_9 {
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

} WDF_PDO_EVENT_CALLBACKS_V1_9, *PWDF_PDO_EVENT_CALLBACKS_V1_9;


//
// Extern "C" the entire file
//
extern "C" {

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfDeviceInitFree)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit
    )
{
    DDI_ENTRY();

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);

    if (DeviceInit->CreatedOnStack == FALSE) {
        delete DeviceInit;
    }
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfDeviceInitSetIoType)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    WDF_DEVICE_IO_TYPE IoType
    )
{
    DDI_ENTRY();

    WDF_IO_TYPE_CONFIG ioTypeConfig;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);

    WDF_IO_TYPE_CONFIG_INIT(&ioTypeConfig);
    ioTypeConfig.ReadWriteIoType = IoType;

    DeviceInit->AssignIoType(&ioTypeConfig);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
STDCALL
WDFEXPORT(WdfDeviceInitSetIoTypeEx)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PWDF_IO_TYPE_CONFIG IoTypeConfig
    )
{
    DDI_ENTRY();

    NTSTATUS status;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, DeviceInit);

    if (IoTypeConfig->Size != sizeof(WDF_IO_TYPE_CONFIG)) {
        status = STATUS_INFO_LENGTH_MISMATCH;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                     "IoTypeConfig size (%d) incorrect, expected %d, %!STATUS!",
                     IoTypeConfig->Size,
                     sizeof(WDF_IO_TYPE_CONFIG), status);
        return;
    }

    DeviceInit->AssignIoType(IoTypeConfig);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfDeviceInitSetExclusive)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    BOOLEAN Exclusive
    )
{
    DDI_ENTRY();

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);

    DeviceInit->Exclusive = Exclusive;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfDeviceInitSetDeviceType)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    DEVICE_TYPE DeviceType
    )
{
    DDI_ENTRY();

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);

    DeviceInit->DeviceType = DeviceType;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfDeviceInitSetPowerNotPageable)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit
    )
{
    DDI_ENTRY();

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);

    DeviceInit->PowerPageable = FALSE;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfDeviceInitSetPowerPageable)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit
    )
{
    DDI_ENTRY();

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);

    DeviceInit->PowerPageable = TRUE;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfDeviceInitSetPowerInrush)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit
    )
{
    DDI_ENTRY();

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);

    //
    // If you are inrush, there is no way  you can be power pageable
    //
    DeviceInit->Inrush = TRUE;
    DeviceInit->PowerPageable = FALSE;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfDeviceInitAssignName)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in_opt
    PCUNICODE_STRING DeviceName
    )
{
    DDI_ENTRY();

    NTSTATUS status;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);

    status = FxVerifierCheckIrqlLevel(DeviceInit->DriverGlobals,
                                      PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (DeviceName != NULL) {
        status = FxValidateUnicodeString(DeviceInit->DriverGlobals,
                                         DeviceName);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    if (DeviceName == NULL) {
        if (DeviceInit->DeviceName != NULL) {
            DeviceInit->DeviceName->RELEASE(NULL);
            DeviceInit->DeviceName = NULL;
        }

        if (DeviceInit->IsPdoInit()) {
            //
            // Make sure a PDO has a name
            //
            DeviceInit->Characteristics |= FILE_AUTOGENERATED_DEVICE_NAME;
        }

        return STATUS_SUCCESS;
    }

    return DeviceInit->AssignName(DeviceInit->DriverGlobals, DeviceName);

}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
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
    DDI_ENTRY();

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);

    if (OrInValues) {
        DeviceInit->Characteristics |= DeviceCharacteristics | FILE_DEVICE_SECURE_OPEN;
    }
    else {
        DeviceInit->Characteristics = DeviceCharacteristics | FILE_DEVICE_SECURE_OPEN;
    }

    //
    // If the autogenerate flag is on, clear out the device name
    //
    if ((DeviceCharacteristics & FILE_AUTOGENERATED_DEVICE_NAME) &&
        DeviceInit->DeviceName != NULL) {
        DeviceInit->DeviceName->RELEASE(NULL);
        DeviceInit->DeviceName = NULL;
    }
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
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
    DDI_ENTRY();

    NTSTATUS status;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    WDF_FILEOBJECT_CLASS normalizedFileClass;
    WDF_FILEOBJECT_CLASS fileClass;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    FxPointerNotNull(pFxDriverGlobals, FileObjectConfig);

    if (FileObjectConfig->Size != sizeof(WDF_FILEOBJECT_CONFIG)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Invalid FileObjectConfig Size %d, expected %d",
            FileObjectConfig->Size, sizeof(WDF_FILEOBJECT_CONFIG));

        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    status = FxValidateObjectAttributes(
        pFxDriverGlobals,
        FileObjectAttributes,
        FX_VALIDATE_OPTION_PARENT_NOT_ALLOWED |
            FX_VALIDATE_OPTION_EXECUTION_LEVEL_ALLOWED |
            FX_VALIDATE_OPTION_SYNCHRONIZATION_SCOPE_ALLOWED
        );

    if (!NT_SUCCESS(status)) {
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    //
    // Validate AutoForwardCleanupClose
    //
    switch (FileObjectConfig->AutoForwardCleanupClose) {
    case WdfTrue:
    case WdfFalse:
    case WdfUseDefault:
        break;

    default:
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Invalid FileObjectConfig->AutoForwardCleanupClose value 0x%x, "
            "expected WDF_TRI_STATE value", FileObjectConfig->AutoForwardCleanupClose);

        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    DeviceInit->FileObject.Set = TRUE;

    DeviceInit->FileObject.AutoForwardCleanupClose =
        FileObjectConfig->AutoForwardCleanupClose;

    fileClass = FileObjectConfig->FileObjectClass;

    //
    // Remove bit flags and validate file object class value.
    //
    normalizedFileClass = FxFileObjectClassNormalize(fileClass);

    if (normalizedFileClass == WdfFileObjectInvalid ||
        normalizedFileClass > WdfFileObjectWdfCannotUseFsContexts)  {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Out of range FileObjectConfig->FileObjectClass %d",
            fileClass);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    //
    // UMDF doesn't support storing object handle at FsContext or FxContxt2.
    // Update the class to WdfFileObjectWdfCannotUseFsContexts for UMDF.
    //
    if (pFxDriverGlobals->IsUserModeDriver &&
        (normalizedFileClass == WdfFileObjectWdfCanUseFsContext ||
         normalizedFileClass == WdfFileObjectWdfCanUseFsContext2)) {

        //
        // update the FileObjectClass value
        //
        BOOLEAN canBeOptional =
            (fileClass & WdfFileObjectCanBeOptional) ? TRUE : FALSE;

        fileClass = WdfFileObjectWdfCannotUseFsContexts;
        if (canBeOptional) {
            fileClass = (WDF_FILEOBJECT_CLASS)
                (fileClass | WdfFileObjectCanBeOptional);
        }

        DoTraceLevelMessage(
             pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDEVICE,
             "FileObjectConfig->FileObjectClass value (%d) has been updated"
             " to a UMDF-supported value %d", normalizedFileClass,
             WdfFileObjectWdfCannotUseFsContexts);

        //
        // re-obtain the normalized class
        //
        normalizedFileClass = FxFileObjectClassNormalize(fileClass);
    }

    //
    // The optional flag can only be combined with a subset of values.
    //
    if (FxIsFileObjectOptional(fileClass)) {
        switch(normalizedFileClass) {
        case WdfFileObjectWdfCanUseFsContext:
        case WdfFileObjectWdfCanUseFsContext2:
        case WdfFileObjectWdfCannotUseFsContexts:
            break;

        default:
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "Invalid FileObjectConfig->FileObjectClass %d",
                fileClass);
            FxVerifierDbgBreakPoint(pFxDriverGlobals);
            return;
            break; // just in case static verification tools complain.
        }
    }

    DeviceInit->FileObject.Class = fileClass;

    RtlCopyMemory(&DeviceInit->FileObject.Callbacks,
                  FileObjectConfig,
                  sizeof(DeviceInit->FileObject.Callbacks));

    if (FileObjectAttributes != NULL) {
        RtlCopyMemory(&DeviceInit->FileObject.Attributes,
                      FileObjectAttributes,
                      sizeof(DeviceInit->FileObject.Attributes));
    }
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
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

    if (!NT_SUCCESS(status)) {
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    RtlCopyMemory(&DeviceInit->RequestAttributes,
                  RequestAttributes,
                  sizeof(WDF_OBJECT_ATTRIBUTES));
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfDeviceInitAssignSDDLString)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in_opt
    PCUNICODE_STRING SDDLString
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (SDDLString == NULL) {
        //
        // Since we require the SDDL on control device creation, you can't
        // clear it!
        //
        if (DeviceInit->IsControlDeviceInit()) {
            return STATUS_INVALID_DEVICE_REQUEST;
        }

        if (DeviceInit->Security.Sddl != NULL) {
            DeviceInit->Security.Sddl->RELEASE(NULL);
            DeviceInit->Security.Sddl = NULL;
        }

        return STATUS_SUCCESS;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, SDDLString);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (DeviceInit->Security.Sddl == NULL) {
        DeviceInit->Security.Sddl = new(pFxDriverGlobals,
                                        WDF_NO_OBJECT_ATTRIBUTES)
            FxString(pFxDriverGlobals);

        if (DeviceInit->Security.Sddl == NULL) {
             DoTraceLevelMessage(
                 pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                 "Couldn't create Security descriptor"
                 " STATUS_INSUFFICIENT_RESOURCES ");

             return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return DeviceInit->Security.Sddl->Assign(SDDLString);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfDeviceInitSetDeviceClass)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    CONST GUID* DeviceClassGuid
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    FxPointerNotNull(pFxDriverGlobals, DeviceClassGuid);

    DeviceInit->Security.DeviceClassSet = TRUE;
    RtlCopyMemory(&DeviceInit->Security.DeviceClass,
                  DeviceClassGuid,
                  sizeof(GUID));
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
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
        PnpPowerEventCallbacks->Size != sizeof(_WDF_PNPPOWER_EVENT_CALLBACKS_V1_9)) {

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
    if (PnpPowerEventCallbacks->Size > sizeof(WDF_PNPPOWER_EVENT_CALLBACKS_V1_9) &&
        PnpPowerEventCallbacks->EvtDeviceUsageNotification != NULL &&
        PnpPowerEventCallbacks->EvtDeviceUsageNotificationEx != NULL) {

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Driver can provide either EvtDeviceUsageNotification or "
            "EvtDeviceUsageNotificationEx callback but not both");

        FxVerifierDbgBreakPoint(pFxDriverGlobals);

        return;
    }

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
STDCALL
WDFEXPORT(WdfDeviceInitSetPowerPolicyEventCallbacks)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PWDF_POWER_POLICY_EVENT_CALLBACKS PowerPolicyEventCallbacks
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    FxPointerNotNull(pFxDriverGlobals, PowerPolicyEventCallbacks);

    //
    // The WDF_POWER_POLICY_EVENT_CALLBACKS structure size increased after v1.5.
    // Since this api is backwards compatible, it can accept either the current
    // structure or the older version. Validate the size of the structure here
    // so that it is one of the two supported sizes.
    //
    if (PowerPolicyEventCallbacks->Size != sizeof(WDF_POWER_POLICY_EVENT_CALLBACKS) &&
        PowerPolicyEventCallbacks->Size != sizeof(WDF_POWER_POLICY_EVENT_CALLBACKS_V1_5)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "PowerPolicyEventCallbacks size %d is invalid, expected %d",
            PowerPolicyEventCallbacks->Size, sizeof(WDF_POWER_POLICY_EVENT_CALLBACKS)
            );

        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    //
    // Only one of EvtDeviceArmWakeFromSx and EvtDeviceArmWakeFromSxWithReason
    // callbacks should be specified if the given power policy callbacks structure
    // is from a version after v1.5
    //
    if (PowerPolicyEventCallbacks->Size > sizeof(WDF_POWER_POLICY_EVENT_CALLBACKS_V1_5) &&
        PowerPolicyEventCallbacks->EvtDeviceArmWakeFromSx != NULL &&
        PowerPolicyEventCallbacks->EvtDeviceArmWakeFromSxWithReason != NULL) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "PowerPolicyEventCallbacks can have either EvtDeviceArmWakeFromSx "
            "or EvtDeviceArmWakeFromSxWithReason callback pointer, but not both"
            );

        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    RtlCopyMemory(&DeviceInit->PnpPower.PolicyEventCallbacks,
                  PowerPolicyEventCallbacks,
                  PowerPolicyEventCallbacks->Size);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfDeviceInitSetPowerPolicyOwnership)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    BOOLEAN IsPowerPolicyOwner
    )
{
    DDI_ENTRY();

     FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);

     DeviceInit->PnpPower.PowerPolicyOwner = IsPowerPolicyOwner ? WdfTrue : WdfFalse;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
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
    DDI_ENTRY();

    FxPnpStateCallbackInfo* pCallback;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    ULONG normalizedState;
    NTSTATUS status;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    FxPointerNotNull(pFxDriverGlobals, EvtDevicePnpStateChange);

    normalizedState = WdfDevStateNormalize(PnpState);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (normalizedState < WdfDevStatePnpObjectCreated || normalizedState > WdfDevStatePnpNull) {
        status = STATUS_INVALID_PARAMETER;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Pnp State is invalid %!STATUS!", status);

        return status;
    }

    if ((CallbackTypes & ~StateNotificationAllStates) != 0 ||
        CallbackTypes == 0x0) {
        status = STATUS_INVALID_PARAMETER;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "CallbackTypes is invalid %!STATUS!", status);

        return status;
    }

    if (DeviceInit->PnpPower.PnpStateCallbacks == NULL) {

        DeviceInit->PnpPower.PnpStateCallbacks =
            new (pFxDriverGlobals) FxPnpStateCallback();

        if (DeviceInit->PnpPower.PnpStateCallbacks == NULL) {
            status = STATUS_INVALID_PARAMETER;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "Couldn't create object PnpStateCallbacks %!STATUS!", status);

            return status;
        }
    }

    pCallback = &DeviceInit->PnpPower.PnpStateCallbacks->m_Methods[
        normalizedState - WdfDevStatePnpObjectCreated];

    pCallback->Callback = EvtDevicePnpStateChange;
    pCallback->Types = CallbackTypes;

    return STATUS_SUCCESS;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
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
    DDI_ENTRY();

    FxPowerStateCallbackInfo* pCallback;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    ULONG normalizedState;
    NTSTATUS status;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    FxPointerNotNull(pFxDriverGlobals, EvtDevicePowerStateChange);

    normalizedState = WdfDevStateNormalize(PowerState);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (normalizedState < WdfDevStatePowerObjectCreated ||
        normalizedState >= WdfDevStatePowerNull) {
        status = STATUS_INVALID_PARAMETER;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "PowerState State is invalid %!STATUS!", status);

        return status;
    }

    if ((CallbackTypes & ~StateNotificationAllStates) != 0 ||
        CallbackTypes == 0x0) {
        status = STATUS_INVALID_PARAMETER;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "CallbackTypes is invalid %!STATUS!"
                            "STATUS_INVALID_PARAMETER", status);

        return status;
    }

    if (DeviceInit->PnpPower.PowerStateCallbacks == NULL) {
        DeviceInit->PnpPower.PowerStateCallbacks =
            new (pFxDriverGlobals) FxPowerStateCallback();

        if (DeviceInit->PnpPower.PowerStateCallbacks == NULL) {
            status = STATUS_INVALID_PARAMETER;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "Couldn't create object PowerStateCallbacks %!STATUS!", status);

            return status;
        }
    }

    pCallback = &DeviceInit->PnpPower.PowerStateCallbacks->m_Methods[
        normalizedState - WdfDevStatePowerObjectCreated];

    pCallback->Callback = EvtDevicePowerStateChange;
    pCallback->Types = CallbackTypes;

    return STATUS_SUCCESS;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
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
    DDI_ENTRY();

    FxPowerPolicyStateCallbackInfo* pCallback;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    ULONG normalizedState;
    NTSTATUS status;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    FxPointerNotNull(pFxDriverGlobals, EvtDevicePowerPolicyStateChange);

    normalizedState = WdfDevStateNormalize(PowerPolicyState);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (normalizedState < WdfDevStatePwrPolObjectCreated ||
        normalizedState >= WdfDevStatePwrPolNull) {
        status = STATUS_INVALID_PARAMETER;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "PowerPolicyState State is invalid %!STATUS!", status);

        return status;
    }

    if ((CallbackTypes & ~StateNotificationAllStates) != 0 ||
        CallbackTypes == 0x0) {
        status = STATUS_INVALID_PARAMETER;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "CallbackTypes is invalid %!STATUS!", status);

        return status;
    }

    if (DeviceInit->PnpPower.PowerPolicyStateCallbacks == NULL) {
        DeviceInit->PnpPower.PowerPolicyStateCallbacks =
            new (pFxDriverGlobals) FxPowerPolicyStateCallback();

        if (DeviceInit->PnpPower.PowerPolicyStateCallbacks == NULL) {
            status = STATUS_INVALID_PARAMETER;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "Couldn't create object PowerPolicyStateCallbacks %!STATUS!", status);

            return status;
        }
    }

    pCallback = &DeviceInit->PnpPower.PowerPolicyStateCallbacks->m_Methods[
         normalizedState - WdfDevStatePwrPolObjectCreated];

    pCallback->Callback = EvtDevicePowerPolicyStateChange;
    pCallback->Types = CallbackTypes;

    return STATUS_SUCCESS;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
STDCALL
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
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    FxPointerNotNull(pFxDriverGlobals, EvtDeviceWdmIrpPreprocess);

    if (NumMinorFunctions > 0) {
        FxPointerNotNull(pFxDriverGlobals, MinorFunctions);
    }

    //
    // ARRAY_SIZE(DeviceInit->PreprocessInfo->Dispatch) just returns a constant
    // size, it does not actually deref PreprocessInfo (which could be NULL)
    //
    if (MajorFunction >= ARRAY_SIZE(DeviceInit->PreprocessInfo->Dispatch)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "MajorFunction is invalid"
                            "STATUS_INVALID_PARAMETER"
                            );

        return STATUS_INVALID_PARAMETER;
    }

    if (DeviceInit->PreprocessInfo == NULL) {
        DeviceInit->PreprocessInfo = new(pFxDriverGlobals) FxIrpPreprocessInfo();

        if (DeviceInit->PreprocessInfo == NULL) {
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                                "Couldn't create object PreprocessInfo"
                                "STATUS_INSUFFICIENT_RESOURCES"
                                );


            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (NumMinorFunctions > 0) {
        if (DeviceInit->PreprocessInfo->Dispatch[MajorFunction].NumMinorFunctions != 0) {

            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                                "Already assigned Minorfunctions"
                                "STATUS_INVALID_DEVICE_REQUEST"
                                );
            return STATUS_INVALID_DEVICE_REQUEST;
        }

        DeviceInit->PreprocessInfo->Dispatch[MajorFunction].MinorFunctions =
            (PUCHAR) FxPoolAllocate(pFxDriverGlobals,
                                    NonPagedPool,
                                    sizeof(UCHAR) * NumMinorFunctions);

        if (DeviceInit->PreprocessInfo->Dispatch[MajorFunction].MinorFunctions == NULL) {

            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                                "Couldn't create object MinorFunctions"
                                "STATUS_INSUFFICIENT_RESOURCES"
                                );

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(
            &DeviceInit->PreprocessInfo->Dispatch[MajorFunction].MinorFunctions[0],
            &MinorFunctions[0],
            NumMinorFunctions
            );

        DeviceInit->PreprocessInfo->Dispatch[MajorFunction].NumMinorFunctions =
            NumMinorFunctions;
    }

    DeviceInit->PreprocessInfo->Dispatch[MajorFunction].EvtDevicePreprocess =
        EvtDeviceWdmIrpPreprocess;

    return STATUS_SUCCESS;
}


__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
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
    DDI_ENTRY();

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), EvtIoInCallerContext);

    DeviceInit->IoInCallerContextCallback = EvtIoInCallerContext;

    return;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfDeviceInitSetRemoveLockOptions)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PWDF_REMOVE_LOCK_OPTIONS RemoveLockOptions
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS fxDriverGlobals = GetFxDriverGlobals(DriverGlobals);
    ULONG validFlags = WDF_REMOVE_LOCK_OPTION_ACQUIRE_FOR_IO;

    FxPointerNotNull(fxDriverGlobals, DeviceInit);

    FxPointerNotNull(fxDriverGlobals, RemoveLockOptions);

    if (RemoveLockOptions->Size != sizeof(WDF_REMOVE_LOCK_OPTIONS)) {
        //
        // Size is wrong, bail out
        //
        DoTraceLevelMessage(fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                            "RemoveLockOptions %p Size incorrect, expected %d, "
                            "got %d",
                            RemoveLockOptions, sizeof(WDF_REMOVE_LOCK_OPTIONS),
                            RemoveLockOptions->Size);

        FxVerifierDbgBreakPoint(fxDriverGlobals);
        goto Done;
    }

    if ((RemoveLockOptions->Flags & ~validFlags) != 0) {
        //
        // Invalid flag
        //
        DoTraceLevelMessage(fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                            "RemoveLockOptions %p Flags 0x%x invalid, "
                            "valid mask is 0x%x",
                            RemoveLockOptions, RemoveLockOptions->Flags,
                            validFlags);

        FxVerifierDbgBreakPoint(fxDriverGlobals);
        goto Done;
    }

    if (FxDeviceInitTypeControlDevice == DeviceInit->InitType) {
        //
        // At this time this feature is not supported on control-devices.
        //
        DoTraceLevelMessage(fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                            "WdfDeviceInitSetRemoveLockOptions is not "
                            "supported on control devices");

        FxVerifierDbgBreakPoint(fxDriverGlobals);
        goto Done;
    }

    DeviceInit->RemoveLockOptionFlags = RemoveLockOptions->Flags;
Done:
    return;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfDeviceInitSetReleaseHardwareOrderOnFailure)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    WDF_RELEASE_HARDWARE_ORDER_ON_FAILURE ReleaseHardwareOrderOnFailure
    )
{
    DDI_ENTRY();

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);

    if ((ReleaseHardwareOrderOnFailure ==
            WdfReleaseHardwareOrderOnFailureInvalid) ||
        (ReleaseHardwareOrderOnFailure >
            WdfReleaseHardwareOrderOnFailureAfterDescendants)) {
        DoTraceLevelMessage(
            DeviceInit->DriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Out of range WDF_RELEASE_HARDWARE_ORDER_ON_FAILURE %d",
            ReleaseHardwareOrderOnFailure);
        FxVerifierDbgBreakPoint(DeviceInit->DriverGlobals);
        return;
    }

    DeviceInit->ReleaseHardwareOrderOnFailure = ReleaseHardwareOrderOnFailure;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfDeviceInitAllowSelfIoTarget)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit
    )
/*++

Routine Description:

    A client or Cx calls this API to indicate that it would like to leverage an
    Self IO target.

    For now the Self targets are not supported for PDOs, Miniport Device,
    or non pnp Devices.

Arguments:

    DeviceInit - Pointer to the WDFDEVICE_INIT structure

Returns:

    VOID

--*/
{
    DDI_ENTRY();

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);

    DeviceInit->RequiresSelfIoTarget = TRUE;
}



//
// BEGIN FDO specific functions
//

__drv_maxIRQL(DISPATCH_LEVEL)
PDEVICE_OBJECT
STDCALL
WDFEXPORT(WdfFdoInitWdmGetPhysicalDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit
    )
{
    DDI_ENTRY();

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);

    if (DeviceInit->IsNotFdoInit()) {
        DoTraceLevelMessage(
            DeviceInit->DriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Not a PWDFDEVICE_INIT for an FDO"
            );

        return NULL;
    }

    return reinterpret_cast<PDEVICE_OBJECT>(DeviceInit->Fdo.PhysicalDevice);
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
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
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    FxPointerNotNull(pFxDriverGlobals, Key);

    *Key = NULL;

    if (DeviceInit->IsNotFdoInit()) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Not a PWDFDEVICE_INIT for an FDO, %!STATUS!",
                            status);
        return status;
    }

    status = FxValidateObjectAttributes(pFxDriverGlobals, KeyAttributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    return FxDevice::_OpenKey(pFxDriverGlobals,
                              DeviceInit,
                              NULL,
                              DeviceInstanceKeyType,
                              DesiredAccess,
                              KeyAttributes,
                              Key);
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfFdoInitSetFilter)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    if (!NT_SUCCESS(FxVerifierCheckIrqlLevel(pFxDriverGlobals,
                                             PASSIVE_LEVEL))) {
        return;
    }
    if (DeviceInit->IsNotFdoInit()) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Not a PWDFDEVICE_INIT for an FDO");

        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    DeviceInit->Fdo.Filter = TRUE;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
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
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    FxPointerNotNull(pFxDriverGlobals, ResultLength);
    if (BufferLength > 0) {
        FxPointerNotNull(pFxDriverGlobals, PropertyBuffer);
    }

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (DeviceInit->IsNotFdoInit()) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Not a PWDFDEVICE_INIT for an FDO, %!STATUS!",
                            status);
        return status;
    }

    status = FxDevice::_QueryProperty(pFxDriverGlobals,
                                      DeviceInit,
                                      NULL,
                                      NULL,
                                      DeviceProperty,
                                      BufferLength,
                                      PropertyBuffer,
                                      ResultLength);
    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
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
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

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

    if (DeviceInit->IsNotFdoInit()) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Not a PWDFDEVICE_INIT for an FDO, %!STATUS!",
                            status);

        return status;
    }

    status = FxDevice::_AllocAndQueryProperty(pFxDriverGlobals,
                                              DeviceInit,
                                              NULL,
                                              NULL,
                                              DeviceProperty,
                                              PoolType,
                                              PropertyMemoryAttributes,
                                              PropertyMemory);
    return status;
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfFdoInitSetEventCallbacks)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PWDF_FDO_EVENT_CALLBACKS FdoEventCallbacks
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    FxPointerNotNull(pFxDriverGlobals, FdoEventCallbacks);

    if (!NT_SUCCESS(FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL))) {
        return;
    }

    if (DeviceInit->IsNotFdoInit()) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Not a PWDFDEVICE_INIT for an FDO");
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    if (FdoEventCallbacks->Size != sizeof(WDF_FDO_EVENT_CALLBACKS)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "FdoEventCallbacks size %d is invalid, expected %d",
            FdoEventCallbacks->Size, sizeof(WDF_FDO_EVENT_CALLBACKS)
            );
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    if (FdoEventCallbacks->EvtDeviceFilterAddResourceRequirements != NULL &&
        FdoEventCallbacks->EvtDeviceRemoveAddedResources == NULL) {
        //
        // Not allowed to add resources without filtering them out later
        //
        DoTraceLevelMessage(
            GetFxDriverGlobals(DriverGlobals), TRACE_LEVEL_ERROR, TRACINGPNP,
            "Must set EvtDeviceRemoveAddedResources if "
            "EvtDeviceFilterAddResourceRequirements (%p) is set",
            FdoEventCallbacks->EvtDeviceFilterAddResourceRequirements);

        FxVerifierDbgBreakPoint(GetFxDriverGlobals(DriverGlobals));
        return;
    }

    RtlCopyMemory(&DeviceInit->Fdo.EventCallbacks,
                  FdoEventCallbacks,
                  sizeof(DeviceInit->Fdo.EventCallbacks));
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
STDCALL
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
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    size_t totalDescriptionSize;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;
    totalDescriptionSize = 0;

    FxPointerNotNull(pFxDriverGlobals, Config);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return;
    }

    if (DeviceInit->IsNotFdoInit()) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Not a PWDFDEVICE_INIT for an FDO");
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    status = FxChildList::_ValidateConfig(pFxDriverGlobals,
                                          Config,
                                          &totalDescriptionSize);
    if (!NT_SUCCESS(status)) {
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    if (DefaultDeviceListAttributes != NULL) {
        status = FxValidateObjectAttributes(pFxDriverGlobals,
                                            DefaultDeviceListAttributes,
                                            FX_VALIDATE_OPTION_PARENT_NOT_ALLOWED);
        if (!NT_SUCCESS(status)) {
            FxVerifierDbgBreakPoint(pFxDriverGlobals);
            return;
        }

        RtlCopyMemory(&DeviceInit->Fdo.ListConfigAttributes,
                      DefaultDeviceListAttributes,
                      sizeof(DeviceInit->Fdo.ListConfigAttributes));
    }

    RtlCopyMemory(&DeviceInit->Fdo.ListConfig,
                  Config,
                  sizeof(WDF_CHILD_LIST_CONFIG));
}

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
STDCALL
WDFEXPORT(WdfFdoInitQueryPropertyEx)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PWDF_DEVICE_PROPERTY_DATA DeviceProperty,
    _In_
    ULONG BufferLength,
    _Out_
    PVOID PropertyBuffer,
    _Out_
    PULONG ResultLength,
    _Out_
    PDEVPROPTYPE Type
    )
/*++

Routine Description:

    This routine queries device property.

Arguments:

    DriverGlobals - DriverGlobals pointer

    DeviceInit - WDF DeviceInit structure pointer.

    DeviceProperty - A pointer to WDF_DEVICE_PROPERTY_DATA structure.

    BufferLength - The size, in bytes, of the buffer that is pointed to by
                   PropertyBuffer.

    PropertyBuffer - A caller-supplied pointer to a caller-allocated buffer that
                  receives the requested information. The pointer can be NULL
                  if the BufferLength parameter is zero.

    ResultLength - A caller-supplied location that, on return, contains the
                  size, in bytes, of the information that the method stored in
                  PropertyBuffer. If the function's return value is
                  STATUS_BUFFER_TOO_SMALL, this location receives the required
                  buffer size.

    Type - A pointer to a DEVPROPTYPE variable. If method successfully retrieves
                  the property data, the routine writes the property type value
                  to this variable. This value indicates the type of property
                  data that is in the Data buffer.

Return Value:

    Method returns an NTSTATUS value. This routine might return one of the
    following values.

    STATUS_BUFFER_TOO_SMALL - The supplied buffer is too small to receive the
                            information. The ResultLength member receives the
                            size of buffer required.
    STATUS_SUCCESS  - The operation succeeded.
    STATUS_INVALID_PARAMETER - One of the parameters is incorrect.

    The method might return other NTSTATUS values.

--*/

{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    FxPointerNotNull(pFxDriverGlobals, DeviceProperty);

    if (DeviceProperty->Size != sizeof(WDF_DEVICE_PROPERTY_DATA)) {
        status = STATUS_INFO_LENGTH_MISMATCH;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                     "PropertyData size (%d) incorrect, expected %d, %!STATUS!",
                     DeviceProperty->Size,
                     sizeof(WDF_DEVICE_PROPERTY_DATA), status);
        return status;
    }

    FxPointerNotNull(pFxDriverGlobals, ResultLength);
    if (BufferLength > 0) {
        FxPointerNotNull(pFxDriverGlobals, PropertyBuffer);
    }

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (DeviceInit->IsNotFdoInit()) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Not a PWDFDEVICE_INIT for an FDO, %!STATUS!",
                            status);
        return status;
    }

    status = FxDevice::_QueryPropertyEx(pFxDriverGlobals,
                                        DeviceInit,
                                        NULL,
                                        DeviceProperty,
                                        FxDeviceProperty,
                                        BufferLength,
                                        PropertyBuffer,
                                        ResultLength,
                                        Type);
    return status;
}

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
STDCALL
WDFEXPORT(WdfFdoInitAllocAndQueryPropertyEx)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    PWDFDEVICE_INIT DeviceInit,
    _In_
    PWDF_DEVICE_PROPERTY_DATA DeviceProperty,
    _In_
    _Strict_type_match_
    POOL_TYPE PoolType,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES PropertyMemoryAttributes,
    _Out_
    WDFMEMORY* PropertyMemory,
    _Out_
    PDEVPROPTYPE Type
    )
/*++

Routine Description:

    This routine queries device property.

Arguments:

    DriverGlobals - DriverGlobals pointer

    DeviceInit - WDF DeviceInit pointer.

    PropertyData - A pointer to WDF_DEVICE_PROPERTY_ DATA structure.

    PoolType - A POOL_TYPE-typed enumerator that specifies the type of memory
               to be allocated.

    PropertyMemoryAttributes - optional, A pointer to a caller-allocated
               WDF_OBJECT_ATTRIBUTES structure that describes object attributes
               for the memory object that the function will allocate. This
               parameter is optional and can be WDF_NO_OBJECT_ATTRIBUTES.

    PropertyMemory - A pointer to a WDFMEMORY-typed location that receives a
               handle to a framework memory object.

    Type - A pointer to a DEVPROPTYPE variable. If method successfully retrieves
               the property data, the routine writes the property type value to
               this variable. This value indicates the type of property data
               that is in the Data buffer.

Return Value:

    Method returns an NTSTATUS value. This routine might return one of the
    following values. It might return other NTSTATUS-codes as well.

    STATUS_SUCCESS  The operation succeeded.
    STATUS_INVALID_PARAMETER    One of the parameters is incorrect.

--*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    FxPointerNotNull(pFxDriverGlobals, DeviceProperty);

    if (DeviceProperty->Size != sizeof(WDF_DEVICE_PROPERTY_DATA)) {
        status = STATUS_INFO_LENGTH_MISMATCH;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                     "PropertyData size (%d) incorrect, expected %d, %!STATUS!",
                     DeviceProperty->Size,
                     sizeof(WDF_DEVICE_PROPERTY_DATA), status);
        return status;
    }

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

    if (DeviceInit->IsNotFdoInit()) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Not a PWDFDEVICE_INIT for an FDO, %!STATUS!",
                            status);

        return status;
    }

    status = FxDevice::_AllocAndQueryPropertyEx(pFxDriverGlobals,
                                                DeviceInit,
                                                NULL,
                                                DeviceProperty,
                                                FxDeviceProperty,
                                                PoolType,
                                                PropertyMemoryAttributes,
                                                PropertyMemory,
                                                Type);
    return status;
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
STDCALL
WDFEXPORT(WdfPdoInitAllocate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE ParentDevice
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    PWDFDEVICE_INIT pInit;
    FxDevice* pDevice;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   ParentDevice,
                                   FX_TYPE_DEVICE,
                                   (PVOID*) &pDevice,
                                   &pFxDriverGlobals);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return NULL;
    }

    if (pDevice->IsFdo() == FALSE) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Parent device is not a FDO (must use WDFCHILDLIST to use a PDO as a parent)"
            );

        return NULL;
    }

    pInit = new(pFxDriverGlobals) WDFDEVICE_INIT(pDevice->GetDriver());
    if (pInit == NULL) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Couldn't create WDFDEVICE_INIT object");

        return NULL;
    }

    pInit->SetPdo(pDevice);

    //
    // Dynamically created PDOs are not created with this API, rather they go
    // through WDFCHILDLIST's callback.
    //
    pInit->Pdo.Static = TRUE;

    return pInit;
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfPdoInitSetEventCallbacks)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PWDF_PDO_EVENT_CALLBACKS DispatchTable
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    FxPointerNotNull(pFxDriverGlobals, DispatchTable);

    if (!NT_SUCCESS(FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL))) {
        return;
    }

    if (DeviceInit->IsNotPdoInit()) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Not a PWDFDEVICE_INIT for a PDO");
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    if (DispatchTable->Size != sizeof(WDF_PDO_EVENT_CALLBACKS) &&
        DispatchTable->Size != sizeof(WDF_PDO_EVENT_CALLBACKS_V1_9)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "DispatchTable size %d is invalid, expected %d",
                            DispatchTable->Size, sizeof(WDF_PDO_EVENT_CALLBACKS));
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return; // STATUS_INFO_LENGTH_MISMATCH;
    }

    RtlCopyMemory(&DeviceInit->Pdo.EventCallbacks,
                  DispatchTable,
                  sizeof(WDF_PDO_EVENT_CALLBACKS));
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfPdoInitAssignDeviceID)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PCUNICODE_STRING DeviceID
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    FxPointerNotNull(pFxDriverGlobals, DeviceID);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, DeviceID);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (DeviceInit->IsNotPdoInit()) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Not a PWDFDEVICE_INIT for a PDO, %!STATUS!",
                            status);
        return status;
    }

    if (DeviceInit->Pdo.DeviceID == NULL) {
        DeviceInit->Pdo.DeviceID = new(pFxDriverGlobals, WDF_NO_OBJECT_ATTRIBUTES)
            FxString(pFxDriverGlobals);

        if (DeviceInit->Pdo.DeviceID == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Couldn't allocate DeviceID object, %!STATUS!",
                            status);
            return status;
        }
    }

    status = DeviceInit->Pdo.DeviceID->Assign(DeviceID);

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfPdoInitAssignInstanceID)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PCUNICODE_STRING InstanceID
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    FxPointerNotNull(pFxDriverGlobals, InstanceID);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, InstanceID);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (DeviceInit->IsNotPdoInit()) {
        status = STATUS_INVALID_DEVICE_REQUEST ;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Not a PWDFDEVICE_INIT for a PDO, %!STATUS!",
                            status);
        return status;
    }

    if (DeviceInit->Pdo.InstanceID == NULL) {
        DeviceInit->Pdo.InstanceID = new(pFxDriverGlobals, WDF_NO_OBJECT_ATTRIBUTES)
            FxString(pFxDriverGlobals);

        if (DeviceInit->Pdo.InstanceID == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Couldn't allocate InstanceID object, %!STATUS!",
                            status);
            return status;
        }
    }

    status = DeviceInit->Pdo.InstanceID->Assign(InstanceID);

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfPdoInitAddHardwareID)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PCUNICODE_STRING HardwareID
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxString* pID;
    NTSTATUS status;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    FxPointerNotNull(pFxDriverGlobals, HardwareID);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, HardwareID);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (DeviceInit->IsNotPdoInit()) {
        status = STATUS_INVALID_DEVICE_REQUEST ;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Not a PWDFDEVICE_INIT for a PDO, %!STATUS!",
                            status);
        return status;
    }

    pID = new(pFxDriverGlobals, WDF_NO_OBJECT_ATTRIBUTES) FxString(pFxDriverGlobals);
    if (pID == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Couldn't allocate String object %!STATUS!", status);
        return status;
    }

    status = pID->Assign(HardwareID);
    if (NT_SUCCESS(status)) {
        status = (DeviceInit->Pdo.HardwareIDs.Add(pFxDriverGlobals, pID)) ?
            STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    }

    //
    // Upon success, the FxCollection will have taken a reference.
    // Upon failure, this is object cleanup.
    //
    pID->Release();

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfPdoInitAddCompatibleID)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PCUNICODE_STRING CompatibleID
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxString* pID;
    NTSTATUS status;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    FxPointerNotNull(pFxDriverGlobals, CompatibleID);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, CompatibleID);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (DeviceInit->IsNotPdoInit()) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Not a PWDFDEVICE_INIT for a PDO, %!STATUS!",
                            status);
        return status;
    }

    pID = new(pFxDriverGlobals, WDF_NO_OBJECT_ATTRIBUTES) FxString(pFxDriverGlobals);
    if (pID == NULL) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                        "Couldn't allocate String object"
                        "STATUS_INSUFFICIENT_RESOURCES "
                        );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = pID->Assign(CompatibleID);
    if (NT_SUCCESS(status)) {
        status = (DeviceInit->Pdo.CompatibleIDs.Add(pFxDriverGlobals, pID)) ?
            STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
    }

    //
    // Upon success, the FxCollection will have taken a reference. Upon failure,
    // this is object cleanup.
    //
    pID->Release();

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfPdoInitAssignContainerID)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    PCUNICODE_STRING ContainerID
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    FxPointerNotNull(pFxDriverGlobals, ContainerID);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, ContainerID);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (DeviceInit->IsNotPdoInit()) {
        status = STATUS_INVALID_DEVICE_REQUEST ;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Not a PWDFDEVICE_INIT for a PDO, %!STATUS!",
                            status);
        return status;
    }

    if (DeviceInit->Pdo.ContainerID == NULL) {
        DeviceInit->Pdo.ContainerID = new(pFxDriverGlobals,
                                          WDF_NO_OBJECT_ATTRIBUTES)
            FxString(pFxDriverGlobals);

        if (DeviceInit->Pdo.ContainerID == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Couldn't allocate ContainerID object, %!STATUS!",
                            status);

            return status;
        }
    }

    status = DeviceInit->Pdo.ContainerID->Assign(ContainerID);

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
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
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDeviceText *pDeviceText;
    NTSTATUS status;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    FxPointerNotNull(pFxDriverGlobals, DeviceDescription);
    FxPointerNotNull(pFxDriverGlobals, DeviceLocation);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, DeviceDescription);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, DeviceLocation);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (DeviceInit->IsNotPdoInit()) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Not a PWDFDEVICE_INIT for a PDO, %!STATUS!", status);
        return status;
    }

    pDeviceText = new(pFxDriverGlobals, PagedPool) FxDeviceText();

    if (pDeviceText == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Couldn't allocate DeviceText object, %!STATUS!", status);

        return status;
    }

    pDeviceText->m_LocaleId = LocaleId;

    pDeviceText->m_Description = FxDuplicateUnicodeStringToString(
        pFxDriverGlobals, DeviceDescription);

    if (pDeviceText->m_Description == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Couldn't allocate DeviceDescription string, %!STATUS!", status);

        goto Done;
    }

    pDeviceText->m_LocationInformation = FxDuplicateUnicodeStringToString(
        pFxDriverGlobals, DeviceLocation);

    if (pDeviceText->m_LocationInformation == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Couldn't allocate DeviceLocation string, %!STATUS!", status);

        goto Done;
    }

    //
    // Insert at the end of the list and update end of the list
    //
    *DeviceInit->Pdo.LastDeviceTextEntry = &pDeviceText->m_Entry;
    DeviceInit->Pdo.LastDeviceTextEntry = &pDeviceText->m_Entry.Next;

Done:
    if (!NT_SUCCESS(status)) {
        delete pDeviceText;
    }

    return status;
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfPdoInitSetDefaultLocale)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    LCID LocaleId
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    if (!NT_SUCCESS(FxVerifierCheckIrqlLevel(pFxDriverGlobals,
                                             PASSIVE_LEVEL))) {
        return;
    }

    if (DeviceInit->IsNotPdoInit()) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Not a PWDFDEVICE_INIT for a PDO");
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    DeviceInit->Pdo.DefaultLocale = LocaleId;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfPdoInitAssignRawDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit,
    __in
    CONST GUID* DeviceClassGuid
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    FxPointerNotNull(pFxDriverGlobals, DeviceClassGuid);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (DeviceInit->IsNotPdoInit()) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Not a PWDFDEVICE_INIT for a PDO, %!STATUS!",
                            status);
        return status;
    }

    DeviceInit->Pdo.Raw = TRUE;
    DeviceInit->Security.DeviceClassSet = TRUE;
    RtlCopyMemory(&DeviceInit->Security.DeviceClass,
                  DeviceClassGuid,
                  sizeof(GUID));

    return STATUS_SUCCESS;
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfPdoInitAllowForwardingRequestToParent)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    PWDFDEVICE_INIT DeviceInit
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return;
    }

    if (DeviceInit->IsNotPdoInit()) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Not a PWDFDEVICE_INIT for a PDO, %!STATUS!",
                            status);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    DeviceInit->Pdo.ForwardRequestToParent = TRUE;
    return ;
}



//
// END PDO specific functions
//

//
// BEGIN CONTROL DEVICE specific functions
//

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
PWDFDEVICE_INIT
STDCALL
WDFEXPORT(WdfControlDeviceInitAllocate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDRIVER Driver,
    __in
    CONST UNICODE_STRING* SDDLString
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDriver* pDriver;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Driver,
                                   FX_TYPE_DRIVER,
                                   (PVOID*) &pDriver,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, SDDLString);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return NULL;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, SDDLString);
    if (!NT_SUCCESS(status)) {
        return NULL;
    }

    return WDFDEVICE_INIT::_AllocateControlDeviceInit(pDriver, SDDLString);
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
STDCALL
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
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxPointerNotNull(GetFxDriverGlobals(DriverGlobals), DeviceInit);
    pFxDriverGlobals = DeviceInit->DriverGlobals;

    FxPointerNotNull(pFxDriverGlobals, Notification);

    if (DeviceInit->IsNotControlDeviceInit()) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Not a PWDFDEVICE_INIT for a control device");
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    if ((Flags & ~(WdfDeviceShutdown | WdfDeviceLastChanceShutdown)) != 0) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "WdfDeviceShutdown Flags 0x%x are invalid", Flags);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    DeviceInit->Control.ShutdownNotification = Notification;
    DeviceInit->Control.Flags |= Flags;
}

//
// END CONTROL DEVICE specific functions
//

} // extern "C"
