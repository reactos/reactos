/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDeviceApi.cpp

Abstract:

    This module exposes the "C" interface to the FxDevice object.

Author:



Environment:

    Both kernel and user mode

Revision History:

--*/

#include "coreprivshared.hpp"
#include "fxiotarget.hpp"

extern "C" {
#include "FxDeviceApi.tmh"
}

struct FxOffsetAndName {
    __nullterminated PCHAR Name;
    UCHAR Offset;
};

#define OFFSET_AND_NAME(type, offset) { #offset, FIELD_OFFSET(type, offset) }

//
// extern "C" the entire file
//
extern "C" {

__drv_maxIRQL(DISPATCH_LEVEL)
WDFDRIVER
WDFEXPORT(WdfDeviceGetDriver)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
/*++

Routine Description:

    Given a Device Handle, return a Handle to the Driver object
    containing this device.

Arguments:
    Device - WDF Device handle.

Returns:
    WDF Driver handle.

--*/

{
    DDI_ENTRY();

    FxDevice* pDevice;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Device,
                         FX_TYPE_DEVICE,
                         (PVOID*) &pDevice);

    //
    // No need to ref count the driver handle b/c it will be around as long
    // as the device handle is.
    //
    return pDevice->GetDriver()->GetHandle();
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFIOTARGET
WDFEXPORT(WdfDeviceGetIoTarget)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
{
    DDI_ENTRY();

    FxIoTarget* pTarget;
    FxDeviceBase *pDeviceBase;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Device,
                         FX_TYPE_DEVICE_BASE,
                         (PVOID*) &pDeviceBase);

    pTarget = pDeviceBase->GetDefaultIoTarget();

    if (pTarget != NULL) {
        return pTarget->GetHandle();
    }

    return NULL;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
WDFIOTARGET
WDFEXPORT(WdfDeviceGetSelfIoTarget)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device
    )

/*++

Routine Description:

    Return the handle to the Self IO target for the device. A Self
    IO target is only created if the client opted for it by calling
    WdfDeviceInitAllowSelfTarget

Arguments:

    Device - Handle to the Device Object for which Self IO target is needed

Returns:

    WDFIOTARGET handle to the Self IO Target. NULL is returned in case the
    device does not have an Self IO target.

--*/

{
    DDI_ENTRY();

    FxIoTargetSelf* pTarget;
    FxDevice *pDevice;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Device,
                         FX_TYPE_DEVICE,
                         (PVOID*) &pDevice);

    pTarget = pDevice->GetSelfIoTarget();

    if (pTarget != NULL) {
        return pTarget->GetHandle();
    }

    return NULL;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceRetrieveDeviceName)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    WDFSTRING String
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDevice *pDevice;
    FxString* pString;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         String,
                         FX_TYPE_STRING,
                         (PVOID*) &pString);

    if (pDevice->m_DeviceName.Buffer != NULL) {
        status = pString->Assign(&pDevice->m_DeviceName);
    }
    else {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "Device name for WDFDEVICE 0x%p is NULL. Possibly incorrect "
            "device handle was passed, %!STATUS!", Device, status);
    }

    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceSetCharacteristics)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    ULONG DeviceCharacteristics
    )
{
    DDI_ENTRY();

    FxDevice *pDevice;
    MxDeviceObject deviceObject;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Device,
                         FX_TYPE_DEVICE,
                         (PVOID*) &pDevice);

    deviceObject.SetObject(pDevice->GetDeviceObject());
    deviceObject.SetCharacteristics(DeviceCharacteristics |
                                                  FILE_DEVICE_SECURE_OPEN);
}

__drv_maxIRQL(DISPATCH_LEVEL)
ULONG
WDFEXPORT(WdfDeviceGetCharacteristics)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
{
    DDI_ENTRY();

    FxDevice *pDevice;
    MxDeviceObject deviceObject;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Device,
                         FX_TYPE_DEVICE,
                         (PVOID*) &pDevice);

    deviceObject.SetObject(pDevice->GetDeviceObject());

    return deviceObject.GetCharacteristics();
}

__drv_maxIRQL(DISPATCH_LEVEL)
ULONG
WDFEXPORT(WdfDeviceGetAlignmentRequirement)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
{
    DDI_ENTRY();

    FxDeviceBase* pDeviceBase;
    MxDeviceObject deviceObject;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Device,
                         FX_TYPE_DEVICE_BASE,
                         (PVOID*) &pDeviceBase);

    deviceObject.SetObject(pDeviceBase->GetDeviceObject());

    return deviceObject.GetAlignmentRequirement();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceSetAlignmentRequirement)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    ULONG AlignmentRequirement
    )
{
    DDI_ENTRY();

    FxDeviceBase* pDeviceBase;
    MxDeviceObject deviceObject;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Device,
                         FX_TYPE_DEVICE_BASE,
                         (PVOID*) &pDeviceBase);

    deviceObject.SetObject(pDeviceBase->GetDeviceObject());

    deviceObject.SetAlignmentRequirement(AlignmentRequirement);
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDF_DEVICE_PNP_STATE
WDFEXPORT(WdfDeviceGetDevicePnpState)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
{
    DDI_ENTRY();

    FxDevice *pDevice;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Device,
                         FX_TYPE_DEVICE,
                         (PVOID*) &pDevice);

    return pDevice->GetDevicePnpState();
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDF_DEVICE_POWER_STATE
WDFEXPORT(WdfDeviceGetDevicePowerState)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
{
    DDI_ENTRY();

    FxDevice *pDevice;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Device,
                         FX_TYPE_DEVICE,
                         (PVOID*) &pDevice);

    return pDevice->GetDevicePowerState();
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDF_DEVICE_POWER_POLICY_STATE
WDFEXPORT(WdfDeviceGetDevicePowerPolicyState)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
{
    DDI_ENTRY();

    FxDevice *pDevice;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Device,
                         FX_TYPE_DEVICE,
                         (PVOID*) &pDevice);

    return pDevice->GetDevicePowerPolicyState();
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceAssignS0IdleSettings)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PWDF_DEVICE_POWER_POLICY_IDLE_SETTINGS Settings
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxDevice* pDevice;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Settings);

    if (pDevice->m_PkgPnp->IsPowerPolicyOwner() == FALSE) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "Device 0x%p is not the power policy owner, caller cannot set S0"
            " idle settings %!STATUS!", Device, status);

        return status;
    }

    //
    // Validate the Settings parameter
    //
    if (Settings->Size != sizeof(WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS) &&
        Settings->Size != sizeof(WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V1_9) &&
        Settings->Size != sizeof(WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V1_7)) {
        status = STATUS_INFO_LENGTH_MISMATCH;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Expected Settings Size %d, got %d, %!STATUS!",
            sizeof(WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS), Settings->Size,
            status);
        return status;
    }
    else if (Settings->DxState < PowerDeviceD1 ||
             Settings->DxState > PowerDeviceMaximum ||
             Settings->IdleCaps < IdleCannotWakeFromS0 ||
             Settings->IdleCaps > IdleUsbSelectiveSuspend ||
             Settings->UserControlOfIdleSettings < IdleDoNotAllowUserControl ||
             Settings->UserControlOfIdleSettings > IdleAllowUserControl ||
             Settings->Enabled < WdfFalse ||
             Settings->Enabled > WdfUseDefault) {

        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "a field (DxState, IdleCaps, Enabled, or UserControlOfIdleSettings)"
            " is out range, %!STATUS!", status);
        return status;
    }

    //
    // PowerUpIdleDeviceOnSystemWake is available only on > 1.7 and can be set to true
    // only when IdleCaps is IdleCannotWakeFromS0
    //
    if (Settings->Size > sizeof(WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V1_7)) {
        if (Settings->PowerUpIdleDeviceOnSystemWake < WdfFalse ||
            Settings->PowerUpIdleDeviceOnSystemWake > WdfUseDefault) {

            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "value of field PowerUpIdleDeviceOnSystemWake is out of range,"
                " %!STATUS!", status);
            return status;
        }
        else if (Settings->IdleCaps != IdleCannotWakeFromS0 &&
            Settings->PowerUpIdleDeviceOnSystemWake != WdfUseDefault) {

            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "value of field PowerUpIdleDeviceOnSystemWake should be set only when"
                " IdleCaps is IdleCannotWakeFromS0, %!STATUS!", status);
            return status;
        }
    }

    //
    // IdleTimeoutType is available only on > 1.9
    //
    if (Settings->Size > sizeof(WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_V1_9)) {
        if (Settings->IdleTimeoutType > SystemManagedIdleTimeoutWithHint) {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "WDFDEVICE %p, value of field IdleTimeoutType is out of range,"
                " %!STATUS!", Device, status);
            return status;
        }
    }

    return pDevice->m_PkgPnp->PowerPolicySetS0IdleSettings(Settings);
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceAssignSxWakeSettings)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PWDF_DEVICE_POWER_POLICY_WAKE_SETTINGS Settings
    )
{
    DDI_ENTRY();

    NTSTATUS status;
    FxDevice* pDevice;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    BOOLEAN armForWakeIfChildrenAreArmedForWake;
    BOOLEAN indicateChildWakeOnParentWake;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Settings);

    if (pDevice->m_PkgPnp->IsPowerPolicyOwner() == FALSE) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "Device 0x%p is not the power policy owner, caller cannot set Sx"
            " wake settings %!STATUS!", Device, status);

        return status;
    }

    //
    // Validate the Settings parameter
    //
    if (Settings->Size != sizeof(WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS) &&
        Settings->Size != sizeof(WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_V1_5)) {
        status = STATUS_INFO_LENGTH_MISMATCH;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Expected Settings Size %x, got %x, %!STATUS!",
                            sizeof(WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS),
                            Settings->Size,
                            status);
        return status;
    }
    else if (Settings->DxState < PowerDeviceD1 ||
             Settings->DxState > PowerDeviceMaximum ||
             Settings->UserControlOfWakeSettings < WakeDoNotAllowUserControl ||
             Settings->UserControlOfWakeSettings > WakeAllowUserControl ||
             Settings->Enabled < WdfFalse ||
             Settings->Enabled > WdfUseDefault) {

        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "a field (DxState, Enabled, or UserControlOfIdleSettings) is out "
            "range, %!STATUS!", status);
        return status;
    }

    //
    // Extract the values from the structure for all the parameters that were
    // added after v1.5.  Since the size of the structure can only grow, get
    // the values only if we are using a version of the struct after v1.5
    //

    //
    // ArmForWakeIfChildrenAreArmedForWake added after v1.5
    //
    armForWakeIfChildrenAreArmedForWake =
        (Settings->Size > sizeof(WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_V1_5)) ?
            Settings->ArmForWakeIfChildrenAreArmedForWake :
            FALSE;

    //
    // IndicateChildWakeOnParentWake added after v1.5
    //
    indicateChildWakeOnParentWake =
        (Settings->Size > sizeof(WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_V1_5)) ?
            Settings->IndicateChildWakeOnParentWake :
            FALSE;

    return pDevice->m_PkgPnp->PowerPolicySetSxWakeSettings(
        Settings,
        armForWakeIfChildrenAreArmedForWake,
        indicateChildWakeOnParentWake);
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceOpenRegistryKey)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
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

    NTSTATUS status;
    FxDevice* pDevice;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Key);

    *Key = NULL;

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return status;
    }

    status = FxValidateObjectAttributes(pFxDriverGlobals, KeyAttributes,
                                        FX_VALIDATE_OPTION_PARENT_NOT_ALLOWED);
    if (!NT_SUCCESS(status)) {

        return status;
    }

    return FxDevice::_OpenKey(pDevice->GetDriverGlobals(),
                              NULL,
                              pDevice,
                              DeviceInstanceKeyType,
                              DesiredAccess,
                              KeyAttributes,
                              Key);
}

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceOpenDevicemapKey) (
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PCUNICODE_STRING KeyName,
    _In_
    ACCESS_MASK DesiredAccess,
    _In_opt_
    PWDF_OBJECT_ATTRIBUTES KeyAttributes,
    _Out_
    WDFKEY* Key
    )
{
    DDI_ENTRY();

    NTSTATUS status;
    FxDevice* pDevice;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxRegKey* pKey = NULL;
    WDFKEY keyHandle;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Key);

    *Key = NULL;

    status = FxValidateUnicodeString(pFxDriverGlobals, KeyName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (KeyName->Length == 0) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "The subkey cannot be of length zero, %!STATUS!", status);
        return status;
    }

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return status;
    }

    status = FxValidateObjectAttributes(pFxDriverGlobals, KeyAttributes,
                                        FX_VALIDATE_OPTION_PARENT_NOT_ALLOWED);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    pKey = new(pFxDriverGlobals, KeyAttributes) FxRegKey(pFxDriverGlobals);

    if (pKey == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Unable to allocate memory for WDFKEY object, %!STATUS!", status);
            return status;
    }

    pKey->SetDeviceBase(pDevice);

    status = pKey->Commit(KeyAttributes, (WDFOBJECT*)&keyHandle);
    if (NT_SUCCESS(status)) {

        status = pDevice->OpenDevicemapKeyWorker(pFxDriverGlobals,
                                                 KeyName,
                                                 DesiredAccess,
                                                 pKey);
        if (NT_SUCCESS(status)) {
            *Key = keyHandle;
        }
    }

    if (!NT_SUCCESS(status)) {
        pKey->DeleteFromFailedCreate();
        pKey = NULL;
    }

    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceGetDeviceState)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __out
    PWDF_DEVICE_STATE DeviceState
    )
{
    DDI_ENTRY();

    FxDevice *pDevice;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, DeviceState);

    if (DeviceState->Size != sizeof(WDF_DEVICE_STATE)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFDEVICE 0x%p DeviceState Size %d, expected %d",
            Device, DeviceState->Size, sizeof(WDF_DEVICE_STATE));

        FxVerifierDbgBreakPoint(pFxDriverGlobals);

        return; // STATUS_INFO_LENGTH_MISMATCH;
    }

    pDevice->m_PkgPnp->GetPnpState(DeviceState);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceSetDeviceState)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PWDF_DEVICE_STATE DeviceState
    )
{
    DDI_ENTRY();

    FxDevice *pDevice;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    ULONG i;

    const static FxOffsetAndName offsets[] = {
        OFFSET_AND_NAME(WDF_DEVICE_STATE, Disabled),
        OFFSET_AND_NAME(WDF_DEVICE_STATE, DontDisplayInUI),
        OFFSET_AND_NAME(WDF_DEVICE_STATE, Failed),
        OFFSET_AND_NAME(WDF_DEVICE_STATE, NotDisableable),
        OFFSET_AND_NAME(WDF_DEVICE_STATE, Removed),
        OFFSET_AND_NAME(WDF_DEVICE_STATE, ResourcesChanged),
    };

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, DeviceState);

    if (DeviceState->Size != sizeof(WDF_DEVICE_STATE)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFDEVICE 0x%p, DeviceState Size %d, expected %d",
            Device, DeviceState->Size, sizeof(WDF_DEVICE_STATE));

        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return; // STATUS_INFO_LENGTH_MISMATCH;
    }

    for (i = 0; i < ARRAY_SIZE(offsets); i++) {
        WDF_TRI_STATE value;

        //
        // This check makes prefast happy
        //
        if (offsets[i].Offset + sizeof(WDF_TRI_STATE) > sizeof(*DeviceState)) {
            return;
        }

        value = *(WDF_TRI_STATE*) WDF_PTR_ADD_OFFSET(DeviceState,
                                                     offsets[i].Offset);

        switch (value) {
        case WdfFalse:
        case WdfTrue:
        case WdfUseDefault:
            break;

        default:
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "WDFDEVICE 0x%p DeviceState WDF_TRI_STATE %s value out of range, "
                "value is %d", Device, offsets[i].Name, value);
            FxVerifierDbgBreakPoint(pFxDriverGlobals);

            return; // STATUS_INVALID_PARAMETER
        }
    }

    pDevice->m_PkgPnp->SetPnpState(DeviceState);

    pDevice->InvalidateDeviceState();
}


_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,


    __inout

    PWDFDEVICE_INIT* DeviceInit,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES DeviceAttributes,
    __out
    WDFDEVICE* Device
    )
{
    DDI_ENTRY();

    FxDevice* pDevice;
    NTSTATUS status;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, DeviceInit);
    FxPointerNotNull(pFxDriverGlobals, *DeviceInit);
    FxPointerNotNull(pFxDriverGlobals, Device);

    //
    // Use the object's globals, not the caller's globals
    //
    pFxDriverGlobals = (*DeviceInit)->DriverGlobals;

    *Device = NULL;

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Make sure the device attributes is initialized properly if passed in
    //
    status = FxValidateObjectAttributes(pFxDriverGlobals,
                                        DeviceAttributes,
                                        (FX_VALIDATE_OPTION_PARENT_NOT_ALLOWED |
                                        FX_VALIDATE_OPTION_EXECUTION_LEVEL_ALLOWED |
                                        FX_VALIDATE_OPTION_SYNCHRONIZATION_SCOPE_ALLOWED));

    if (!NT_SUCCESS(status)) {
        return status;
    }

    if ((*DeviceInit)->CreatedDevice != NULL) {
        //
        // Already created the device!
        //
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "WDFDEVICE 0x%p   already created"
                            "STATUS_INVALID_DEVICE_STATE",
                            Device);

        return STATUS_INVALID_DEVICE_STATE;
    }

    //
    // If security is specified, then the device being created *must* have a
    // name to apply that security too.
    //
    if ((*DeviceInit)->Security.Sddl != NULL || (*DeviceInit)->Security.DeviceClassSet) {
        if ((*DeviceInit)->HasName()) {
            //
            // Driver writer specified a name, all is good
            //
            DO_NOTHING();
        }
        else {
            status = STATUS_INVALID_SECURITY_DESCR;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "Device init: has device class or SDDL set, but does not have "
                "a name, %!STATUS!", status);

            return status;
        }
    }

    if ((*DeviceInit)->RequiresSelfIoTarget) {
        if ((*DeviceInit)->InitType != FxDeviceInitTypeFdo) {
            status = STATUS_INVALID_DEVICE_REQUEST;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "Client called WdfDeviceInitAllowSelfTarget. Self "
                "IO Targets are supported only for FDOs, %!STATUS!", status);
            return status;
        }
    }

    status = FxDevice::_Create(pFxDriverGlobals,
                               DeviceInit,
                               DeviceAttributes,
                               &pDevice);
    if (NT_SUCCESS(status)) {
        *Device = pDevice->GetHandle();
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceCreateSymbolicLink)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PCUNICODE_STRING SymbolicLinkName
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    PUNICODE_STRING pName;
    FxAutoString pdoName;
    FxDevice* pDevice;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);
    pName = NULL;

    FxPointerNotNull(pFxDriverGlobals, SymbolicLinkName);

    if (SymbolicLinkName->Length == 0) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "WDFDEVICE %p, SymbolicLinkName has no length, %!STATUS!",
                            Device, status);

        return status;
    }

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateUnicodeString(pFxDriverGlobals, SymbolicLinkName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (pDevice->m_SymbolicLinkName.Buffer != NULL) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFDEVICE %p already has a symbolic link associated with it, %!STATUS!",
            Device, status);

        return status;
    }

    status = pDevice->CreateSymbolicLink(pFxDriverGlobals, SymbolicLinkName);

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceQueryProperty)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    DEVICE_REGISTRY_PROPERTY  DeviceProperty,
    __in
    ULONG BufferLength,
     __out_bcount_full(BufferLength)
    PVOID PropertyBuffer,
    __out
    PULONG ResultLength
    )
/*++

Routine Description:
    Retrieves the requested device property for the given device

Arguments:
    Device - the device whose PDO whose will be queried

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
    FxDevice* pDevice;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, ResultLength);
    if (BufferLength > 0) {
        FxPointerNotNull(pFxDriverGlobals, PropertyBuffer);
    }

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (pDevice->IsLegacy()) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "WDFDEVICE 0x%p is not a PnP device %!STATUS!",
                            Device, status);

        return status;
    }

    status = FxDevice::_QueryProperty(pFxDriverGlobals,
                                      NULL,
                                      pDevice,
                                      NULL,
                                      DeviceProperty,
                                      BufferLength,
                                      PropertyBuffer,
                                      ResultLength);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDEVICE,
                        "exit WDFDEVICE %p, Property %d, %!STATUS!",
                        Device, DeviceProperty, status);

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceAllocAndQueryProperty)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    DEVICE_REGISTRY_PROPERTY  DeviceProperty,
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
    Allocates and retrieves the requested device property for the given device

Arguments:
    Device - the pnp device whose PDO whose will be queried

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
    FxDevice* pDevice;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
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

    if (pDevice->IsLegacy()) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "WDFDEVICE %p is not a PnP device, %!STATUS!",
                            Device, status);

        return status;
    }

    status = FxDevice::_AllocAndQueryProperty(pFxDriverGlobals,
                                              NULL,
                                              pDevice,
                                              NULL,
                                              DeviceProperty,
                                              PoolType,
                                              PropertyMemoryAttributes,
                                              PropertyMemory);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDEVICE,
                        "exit WDFDEVICE %p, Property %d, %!STATUS!",
                        Device, DeviceProperty, status);

    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceSetStaticStopRemove)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    BOOLEAN Stoppable
    )
{
    DDI_ENTRY();

    FxDevice *pDevice;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Device,
                         FX_TYPE_DEVICE,
                         (PVOID*) &pDevice);

    if (Stoppable) {
        //
        // Stoppable means that query stop / query remove can succeed.
        // This means m_DeviceStopCount == 0 (eventually if there are nested
        // calls to this function).
        //
        ASSERT(pDevice->m_PkgPnp->m_DeviceStopCount > 0);
        InterlockedDecrement((PLONG) &pDevice->m_PkgPnp->m_DeviceStopCount);
    }
    else {
        InterlockedIncrement((PLONG) &pDevice->m_PkgPnp->m_DeviceStopCount);
    }

    return;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceSetFailed)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    WDF_DEVICE_FAILED_ACTION FailedAction
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDevice *pDevice;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    if (FailedAction < WdfDeviceFailedAttemptRestart ||
        FailedAction > WdfDeviceFailedNoRestart) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Invalid FailedAction %d", FailedAction);
        FxVerifierDbgBreakPoint(pDevice->GetDriverGlobals());
        return;
    }

    DoTraceLevelMessage(
        pFxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGDEVICE,
        "WDFDEVICE %p, !devobj %p SetFailed %!WDF_DEVICE_FAILED_ACTION!",
        Device, pDevice->GetDeviceObject(), FailedAction);

    pDevice->m_PkgPnp->SetDeviceFailed(FailedAction);
}

__inline
NTSTATUS
StopIdleWorker(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    BOOLEAN WaitForD0,
    __in
    PVOID Tag,
    __in
    LONG Line,
    __in
    PSTR File
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxDevice *pDevice;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    if (WaitForD0) {
        status = FxVerifierCheckIrqlLevel(pFxDriverGlobals,
                                          PASSIVE_LEVEL);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }


    if (pDevice->m_PkgPnp->IsPowerPolicyOwner() == FALSE) {
        status = STATUS_INVALID_DEVICE_STATE;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFDEVICE %p WdfDeviceStopIdle does nothing if you are not the power "
            "policy owner for the stack, %!STATUS!", Device, status);
        return status;
    }

    status = pDevice->m_PkgPnp->PowerReference(WaitForD0, Tag, Line, File);

    DoTraceLevelMessage(
        pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDEVICE,
        "WDFDEVICE %p WdfDeviceStopIdle, WaitForD0 %d %!STATUS!",
        Device, WaitForD0, status);

    return status;
}

__inline
VOID
ResumeIdleWorker(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PVOID Tag,
    __in
    LONG Line,
    __in
    PSTR File
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDevice *pDevice;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    if (pDevice->m_PkgPnp->IsPowerPolicyOwner() == FALSE) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WdfDeviceResumeIdle does nothing if you are not the power "
            "policy owner for the stack");
        return;
    }

    pDevice->m_PkgPnp->PowerDereference(Tag, Line, File);
}

_Must_inspect_result_
__drv_when(WaitForD0 == 0, __drv_maxIRQL(DISPATCH_LEVEL))
__drv_when(WaitForD0 != 0, __drv_maxIRQL(PASSIVE_LEVEL))
NTSTATUS
WDFEXPORT(WdfDeviceStopIdleNoTrack)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    BOOLEAN WaitForD0
    )
{
    DDI_ENTRY();

    NTSTATUS status;

    status = StopIdleWorker(DriverGlobals,
                            Device,
                            WaitForD0,
                            NULL,
                            0,
                            NULL);

    return status;
}

_Must_inspect_result_
__drv_when(WaitForD0 == 0, __drv_maxIRQL(DISPATCH_LEVEL))
__drv_when(WaitForD0 != 0, __drv_maxIRQL(PASSIVE_LEVEL))
NTSTATUS
WDFEXPORT(WdfDeviceStopIdleActual)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    BOOLEAN WaitForD0,
    __in_opt
    PVOID Tag,
    __in
    LONG Line,
    __in
    PSTR File
    )
{
    DDI_ENTRY();

    NTSTATUS status;

    status = StopIdleWorker(DriverGlobals,
                            Device,
                            WaitForD0,
                            Tag,
                            Line,
                            File);

    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceResumeIdleNoTrack)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )
{
    DDI_ENTRY();

    ResumeIdleWorker(DriverGlobals,
                     Device,
                     NULL,
                     0,
                     NULL);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceResumeIdleActual)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in_opt
    PVOID Tag,
    __in
    LONG Line,
    __in
    PSTR File
    )
{
    DDI_ENTRY();

    ResumeIdleWorker(DriverGlobals,
                     Device,
                     Tag,
                     Line,
                     File);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceSetPnpCapabilities)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PWDF_DEVICE_PNP_CAPABILITIES PnpCapabilities
    )
/*++

Routine Description:
    Sets the pnp capabilities of the device.  This function is usually called
    during AddDevice or EvtDevicePrepareHardware since these values are queried
    by the stack and pnp during start device processing or immediately afterwards.

Arguments:
    Device - Device being set

    PnpCapabilities - Caps being set

Return Value:
    None

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDevice* pDevice;
    ULONG i;

    const static FxOffsetAndName offsets[] = {
        OFFSET_AND_NAME(WDF_DEVICE_PNP_CAPABILITIES, LockSupported),
        OFFSET_AND_NAME(WDF_DEVICE_PNP_CAPABILITIES, EjectSupported),
        OFFSET_AND_NAME(WDF_DEVICE_PNP_CAPABILITIES, Removable),
        OFFSET_AND_NAME(WDF_DEVICE_PNP_CAPABILITIES, DockDevice),
        OFFSET_AND_NAME(WDF_DEVICE_PNP_CAPABILITIES, UniqueID),
        OFFSET_AND_NAME(WDF_DEVICE_PNP_CAPABILITIES, SilentInstall),
        OFFSET_AND_NAME(WDF_DEVICE_PNP_CAPABILITIES, SurpriseRemovalOK),
        OFFSET_AND_NAME(WDF_DEVICE_PNP_CAPABILITIES, HardwareDisabled),
        OFFSET_AND_NAME(WDF_DEVICE_PNP_CAPABILITIES, NoDisplayInUI),
    };

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, PnpCapabilities);

    if (PnpCapabilities->Size != sizeof(WDF_DEVICE_PNP_CAPABILITIES)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFDEVICE 0x%p PnpCapabilities Size %d, expected %d",
            Device, PnpCapabilities->Size, sizeof(WDF_DEVICE_PNP_CAPABILITIES));
        FxVerifierDbgBreakPoint(pFxDriverGlobals);

        return; // STATUS_INFO_LENGTH_MISMATCH;
    }
    else {
        for (i = 0; i < ARRAY_SIZE(offsets); i++) {
            WDF_TRI_STATE value;

            //
            // This check makes prefast happy
            //
            if (offsets[i].Offset + sizeof(WDF_TRI_STATE) >
                                                    sizeof(*PnpCapabilities)) {
                return;
            }

            value = *(WDF_TRI_STATE*) WDF_PTR_ADD_OFFSET(PnpCapabilities,
                                                         offsets[i].Offset);
            switch (value) {
            case WdfFalse:
            case WdfTrue:
            case WdfUseDefault:
                break;

            default:
                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                    "WDFDEVICE 0x%p PnpCapabilities WDF_TRI_STATE %s value out "
                    "of range, value is %d", Device, offsets[i].Name, value);
                FxVerifierDbgBreakPoint(pFxDriverGlobals);

                return; // STATUS_INVALID_PARAMETER
            }
        }
    }

    pDevice->m_PkgPnp->SetPnpCaps(PnpCapabilities);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfDeviceSetPowerCapabilities)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    PWDF_DEVICE_POWER_CAPABILITIES PowerCapabilities
    )
/*++

Routine Description:
    Sets the power capabilities of the device.  This function is usually called
    during AddDevice or EvtDevicePrepareHardware since these values are queried
    by the stack and power during start device processing or immediately afterwards.

Arguments:
    Device - Device being set

    PowerCapabilities - Caps being set

Return Value:
    None

  --*/
{
    DDI_ENTRY();

    FxDevice* pDevice;
    ULONG i;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    const static FxOffsetAndName offsets[] = {
        OFFSET_AND_NAME(WDF_DEVICE_POWER_CAPABILITIES, DeviceD1),
        OFFSET_AND_NAME(WDF_DEVICE_POWER_CAPABILITIES, DeviceD2),
        OFFSET_AND_NAME(WDF_DEVICE_POWER_CAPABILITIES, WakeFromD0),
        OFFSET_AND_NAME(WDF_DEVICE_POWER_CAPABILITIES, WakeFromD1),
        OFFSET_AND_NAME(WDF_DEVICE_POWER_CAPABILITIES, WakeFromD2),
        OFFSET_AND_NAME(WDF_DEVICE_POWER_CAPABILITIES, WakeFromD3),
    };

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, PowerCapabilities);

    if (PowerCapabilities->Size != sizeof(WDF_DEVICE_POWER_CAPABILITIES)) {

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFDEVICE 0x%p PowerCapabilities Size %d, expected %d",
            Device, PowerCapabilities->Size,
            sizeof(WDF_DEVICE_POWER_CAPABILITIES));

        FxVerifierDbgBreakPoint(pFxDriverGlobals);

        return; // STATUS_INFO_LENGTH_MISMATCH;
    }
    else {
        for (i = 0; i < ARRAY_SIZE(offsets); i++) {
            WDF_TRI_STATE value;

            //
            // This check makes prefast happy
            //
            if (offsets[i].Offset + sizeof(WDF_TRI_STATE) >
                                                sizeof(*PowerCapabilities)) {
                return;
            }

            value = *(WDF_TRI_STATE*) WDF_PTR_ADD_OFFSET(PowerCapabilities,
                                                         offsets[i].Offset);
            switch (value) {
            case WdfFalse:
            case WdfTrue:
            case WdfUseDefault:
                break;

            default:
                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                    "WDFDEVICE 0x%p PowerCapabilities WDF_TRI_STATE %s value out "
                    "of range, value is %d",
                    Device, offsets[i].Name, value);
                FxVerifierDbgBreakPoint(pFxDriverGlobals);

                return; // STATUS_INVALID_PARAMETER
            }
        }

        for (i = 0; i < ARRAY_SIZE(PowerCapabilities->DeviceState); i++) {
            if (PowerCapabilities->DeviceState[i] < PowerDeviceUnspecified ||
                PowerCapabilities->DeviceState[i] > PowerDeviceMaximum) {

                DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                                    "WDFDEVICE 0x%p PowerCapabilities DeviceState is invalid",
                                    Device);

                FxVerifierDbgBreakPoint(pFxDriverGlobals);

                return; // STATUS_INVALID_PARAMETER;
            }
        }

        if (PowerCapabilities->DeviceWake < PowerDeviceUnspecified ||
            PowerCapabilities->DeviceWake > PowerDeviceMaximum) {

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "WDFDEVICE 0x%p PowerCapabilities DeviceWake %d is out of range",
                Device, PowerCapabilities->DeviceWake);

            FxVerifierDbgBreakPoint(pFxDriverGlobals);

            return; // STATUS_INVALID_PARAMETER;
        }

        if (PowerCapabilities->SystemWake < PowerSystemUnspecified ||
            PowerCapabilities->SystemWake > PowerSystemMaximum) {

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "WDFDEVICE 0x%p PowerCapabilities SystemWake %d is out of range",
                Device, PowerCapabilities->SystemWake);

            FxVerifierDbgBreakPoint(pFxDriverGlobals);

            return; // STATUS_INVALID_PARAMETER;
        }

        if (pDevice->m_PkgPnp->IsPowerPolicyOwner() == FALSE &&
            PowerCapabilities->IdealDxStateForSx != PowerDeviceMaximum) {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "WDFDEVICE 0x%p PowerCapabilities IdealDxStateForSx %d can only"
                " be set by the power policy owner",
                Device, PowerCapabilities->IdealDxStateForSx);

            FxVerifierDbgBreakPoint(pFxDriverGlobals);

            return; // STATUS_INVALID_PARAMETER;
        }

        //
        // D0 is not allowed as an ideal Dx state
        //
        if (PowerCapabilities->IdealDxStateForSx < PowerDeviceD1 ||
            PowerCapabilities->IdealDxStateForSx > PowerDeviceMaximum) {

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "WDFDEVICE 0x%p PowerCapabilities IdealDxStateForSx %d is out "
                "of range", Device, PowerCapabilities->IdealDxStateForSx);

            FxVerifierDbgBreakPoint(pFxDriverGlobals);

            return; // STATUS_INVALID_PARAMETER;
        }
    }


    pDevice->m_PkgPnp->SetPowerCaps(PowerCapabilities);
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceConfigureRequestDispatching)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in
    WDFQUEUE Queue,
    __in
    __drv_strictTypeMatch(__drv_typeCond)
    WDF_REQUEST_TYPE RequestType
    )

/*++

Routine Description:

    Configure which IRP_MJ_* requests are automatically
    forwarded by the I/O package to the specified Queue.

    By default the I/O package sends all requests to the
    devices default queue. This API allows certain requests
    to be specified to their own queues under driver control.

Arguments:

    Device    - The device which is handling the IO.

    Queue     - I/O Queue object to forward specified request to.

    RequestType - WDF Request type to be forwarded to the queue

Returns:

    NTSTATUS

--*/

{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS  status;
    FxDevice*  pDevice;
    FxIoQueue* pFxIoQueue;

    pDevice = NULL;
    pFxIoQueue = NULL;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    if (RequestType != WdfRequestTypeCreate &&
        RequestType != WdfRequestTypeRead &&
        RequestType != WdfRequestTypeWrite &&
        RequestType != WdfRequestTypeDeviceControl &&
        RequestType != WdfRequestTypeDeviceControlInternal) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                            "Invalid RequestType %!WDF_REQUEST_TYPE!, %!STATUS!",
                            RequestType, status);
        return status;
    }

    //
    // Validate the Queue handle
    //
    FxObjectHandleGetPtr(pFxDriverGlobals,
                         Queue,
                         FX_TYPE_QUEUE,
                         (PVOID*)&pFxIoQueue);

    if (pDevice != pFxIoQueue->GetDevice()) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
            "Input WDFQUEUE 0x%p doesn't belong to the WDFDEVICE 0x%p, %!STATUS!",
            Queue, Device, status);

        return status;
    }

    if (pDevice->IsLegacy()) {
        //
        // This is a controldevice. Make sure the create is called after the device
        // is initialized and ready to accept I/O.
        //
        MxDeviceObject deviceObject(pDevice->GetDeviceObject());
        if ((deviceObject.GetFlags() & DO_DEVICE_INITIALIZING) == 0x0) {

            status = STATUS_INVALID_DEVICE_STATE;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "Queue cannot be configured for automatic dispatching"
                " after WdfControlDeviceFinishInitializing"
                "is called on the WDFDEVICE %p is called %!STATUS!",
                Device,
                status);
            return status;
        }
    }
    else {
        //
        // This is either FDO or PDO. Make sure it's not started yet.
        //
        if (pDevice->GetDevicePnpState() != WdfDevStatePnpInit) {
            status = STATUS_INVALID_DEVICE_STATE;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "Queue cannot be configured for automatic dispatching"
                "after the WDFDEVICE %p is started, %!STATUS!",
                Device, status);
            return status;
        }
    }

    if (RequestType == WdfRequestTypeCreate) {
        status = pDevice->m_PkgGeneral->ConfigureForwarding(pFxIoQueue);
    }
    else {
        status = pDevice->m_PkgIo->ConfigureForwarding(pFxIoQueue, RequestType);
    }

    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFQUEUE
WDFEXPORT(WdfDeviceGetDefaultQueue)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )

/*++

Routine Description:

    Return the handle to the default queue for the device.

Arguments:

    Device - Handle to the Device Object

Returns:

    WDFQUEUE

--*/

{
    DDI_ENTRY();

    FxPkgIo*   pPkgIo;;
    FxIoQueue* pFxIoQueue;;
    FxDevice * pFxDevice;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pPkgIo = NULL;
    pFxIoQueue = NULL;

    //
    // Validate the I/O Package handle, and get the FxPkgIo*
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pFxDevice,
                                   &pFxDriverGlobals);

    pPkgIo = (FxPkgIo *) pFxDevice->m_PkgIo;
    pFxIoQueue = pPkgIo->GetDefaultQueue();

    //
    // A default queue is optional
    //
    if (pFxIoQueue == NULL) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_WARNING, TRACINGIO,
                            "No default Queue configured "
                            "for Device 0x%p", Device);
        return NULL;
    }

    return (WDFQUEUE)pFxIoQueue->GetObjectHandle();;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfDeviceEnqueueRequest)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE  Device,
    __in
    WDFREQUEST Request
    )

/*++

Routine Description:

    Inserts a request into the I/O Package processing pipeline.

    The given request is processed by the I/O package, forwarding
    it to its configured destination or the default queue.

    If an EvtIoInCallerContext is callback is registered, it is not called.

Arguments:

    Device  - Handle to the Device Object

    Request - Request to insert in the processing pipeline

Return Value:

    STATUS_SUCCESS - Request has been inserted into the I/O processing pipeline

    !NT_SUCCESS(status)
    STATUS_WDF_BUSY - Device is not accepting requests, the driver still owns the
                     the request and must complete it, or correct the conditions.

--*/

{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDevice *pDevice;
    FxRequest* pRequest;

    //
    // Validate the device object handle
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);
    //
    // Validate Device object handle
    //
    FxObjectHandleGetPtr(pFxDriverGlobals,
                         Device,
                         FX_TYPE_DEVICE,
                         (PVOID *) &pDevice);

    //
    // Validate the request object handle
    //
    FxObjectHandleGetPtr(pFxDriverGlobals,
                         Request,
                         FX_TYPE_REQUEST,
                         (PVOID *) &pRequest);

    //
    // Dispatch it to the Io Package object
    //
    return pDevice->m_PkgIo->EnqueueRequest(pDevice, pRequest);
}

__drv_maxIRQL(DISPATCH_LEVEL)
POWER_ACTION
WDFEXPORT(WdfDeviceGetSystemPowerAction)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device
    )

/*++

Routine Description:

    This DDI returns the System power action.

    If the DDI is called in the power down path of a device, it will return the
    current system power transition type because of which the device is powering
    down.  If the DDI is called in the power up path of a device, it will return
    the system power transition type which initially put the device into a lower
    power state.  If the DDI is called during the normal functioning of a device
    that is not undergoing a power state transition, or if the device is powering
    up as part of the system start up, the DDI will return the PowerActionNone
    value.

Arguments:

    Device  - Handle to the Device Object

Return Value:

    Returns a POWER_ACTION enum value that represents the current system power
    action with regards to any system power transition underway.

--*/

{
    DDI_ENTRY();

    FxDevice *pDevice;

    //
    // Validate Device object handle
    //
    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Device,
                         FX_TYPE_DEVICE,
                         (PVOID*) &pDevice);

    return pDevice->m_PkgPnp->GetSystemPowerAction();
}

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceQueryPropertyEx)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_PROPERTY_DATA DeviceProperty,
    _In_
    ULONG BufferLength,
    _Out_
    PVOID PropertyBuffer,
    _Out_
    PULONG RequiredSize,
    _Out_
    PDEVPROPTYPE Type
    )
/*++

Routine Description:

    This routine queries interface property.

Arguments:

    DriverGlobals - DriverGlobals pointer

    Device - WDF Device handle.

    DeviceProperty - A pointer to WDF_DEVICE_PROPERTY_ DATA structure.

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
    FxDevice* pDevice;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, DeviceProperty);

    //
    // Validate PropertyData
    //
    if (DeviceProperty->Size != sizeof(WDF_DEVICE_PROPERTY_DATA)) {
        status = STATUS_INFO_LENGTH_MISMATCH;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                     "PropertyData size (%d) incorrect, expected %d, %!STATUS!",
                     DeviceProperty->Size,
                     sizeof(WDF_DEVICE_PROPERTY_DATA), status);
        return status;
    }

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, APC_LEVEL);
    if (!NT_SUCCESS(status)) {
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return status;
    }

    FxPointerNotNull(pFxDriverGlobals, RequiredSize);
    FxPointerNotNull(pFxDriverGlobals, Type);

    if (BufferLength != 0 && PropertyBuffer == NULL) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                    "Property buffer size is non-zero, while the buffer is NULL"
                    ", %!STATUS!", status);
        return status;
    }

    if (BufferLength == 0 && PropertyBuffer != NULL) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                    "Property buffer size is zero, while the buffer is non-NULL"
                    ", %!STATUS!", status);
        return status;
    }

    status = FxDevice::_QueryPropertyEx(pFxDriverGlobals,
                                        NULL,
                                        pDevice,
                                        DeviceProperty,
                                        FxDeviceProperty,
                                        BufferLength,
                                        PropertyBuffer,
                                        RequiredSize,
                                        Type);
    return status;
}

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceAllocAndQueryPropertyEx)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
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

    Device - WDF Device handle.

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
    FxDevice* pDevice;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, DeviceProperty);

    //
    // Validate PropertyData
    //
    if (DeviceProperty->Size != sizeof(WDF_DEVICE_PROPERTY_DATA)) {
        status = STATUS_INFO_LENGTH_MISMATCH;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                     "PropertyData size (%d) incorrect, expected %d, %!STATUS!",
                     DeviceProperty->Size,
                     sizeof(WDF_DEVICE_PROPERTY_DATA), status);
        return status;
    }

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, APC_LEVEL);
    if (!NT_SUCCESS(status)) {
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return status;
    }

    FxVerifierCheckNxPoolType(pFxDriverGlobals, PoolType, pFxDriverGlobals->Tag);

    FxPointerNotNull(pFxDriverGlobals, PropertyMemory);
    FxPointerNotNull(pFxDriverGlobals, Type);

    *PropertyMemory = NULL;

    status = FxValidateObjectAttributes(pFxDriverGlobals, PropertyMemoryAttributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxDevice::_AllocAndQueryPropertyEx(pFxDriverGlobals,
                                                NULL,
                                                pDevice,
                                                DeviceProperty,
                                                FxDeviceProperty,
                                                PoolType,
                                                PropertyMemoryAttributes,
                                                PropertyMemory,
                                                Type);
    return status;
}

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceAssignProperty)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_
    PWDF_DEVICE_PROPERTY_DATA DeviceProperty,
    _In_
    DEVPROPTYPE Type,
    _In_
    ULONG BufferLength,
    _In_opt_
    PVOID PropertyBuffer
    )
/*++

Routine Description:

    This routine assigns interface property.

Arguments:

    DriverGlobals - DriverGlobals pointer

    Device - WDF Device handle.

    PropertyData - A pointer to WDF_DEVICE_PROPERTY_ DATA structure.

    Type - Set this parameter to the DEVPROPTYPE value that specifies the type
           of the data that is supplied in the Data buffer.

    BufferLength - Specifies the length, in bytes, of the buffer that
           PropertyBuffer points to.

    PropertyBuffer - optional, A pointer to the device interface property data.
           Set this parameter to NULL to delete the specified property.

Return Value:

    Mthod returns an NTSTATUS value. This routine might return one of the
    following values. It might return other NTSTATUS-codes as well.

    STATUS_SUCCESS - The operation succeeded.
    STATUS_INVALID_PARAMETER - One of the parameters is incorrect.

--*/

{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxDevice *pDevice;
    NTSTATUS status;

    //
    // Validate the Device object handle and get its FxDevice. Also get the
    // driver globals pointer.
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, DeviceProperty);

    //
    // Validate PropertyData
    //
    if (DeviceProperty->Size != sizeof(WDF_DEVICE_PROPERTY_DATA)) {
        status = STATUS_INFO_LENGTH_MISMATCH;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                     "PropertyData size (%d) incorrect, expected %d, %!STATUS!",
                     DeviceProperty->Size,
                     sizeof(WDF_DEVICE_PROPERTY_DATA), status);
        return status;
    }

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, APC_LEVEL);
    if (!NT_SUCCESS(status)) {
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return status;
    }

    if (BufferLength == 0 && PropertyBuffer != NULL) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                    "Property buffer size is zero, while the buffer is non-NULL"
                    ", %!STATUS!", status);
        return status;
    }

    status = pDevice->AssignProperty(DeviceProperty,
                                     FxDeviceProperty,
                                     Type,
                                     BufferLength,
                                     PropertyBuffer
                                     );
    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfDeviceConfigureWdmIrpDispatchCallback)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFDEVICE Device,
    _In_opt_
    WDFDRIVER Driver,
    _In_
    UCHAR MajorFunction,
    _In_
    PFN_WDFDEVICE_WDM_IRP_DISPATCH EvtDeviceWdmIrpDispatch,
    _In_opt_
    WDFCONTEXT DriverContext
    )

/*++

    Routine Description:

        Configure callbacks for IRP_MJ_READ, IRP_MJ_WRITE, IRP_MJ_DEVICE_CONTROL, and
        IRP_MJ_INTERNAL_DEVICE_CONTROL (KMDF only). By default the I/O package sends all requests to
        a device's default queue, or to the queues configured with
        WdfDeviceConfigureRequestDisaptching. This DDI allows a driver specified
        callback to select a different queue dynamically during runtime.

    Arguments:

        Device - The device which is handling the I/O.

        Driver - An optional driver handle. Used to associate the
                 callback with a specific class extension.

        MajorFunction - IRP major function type to be forwarded to the callback

        EvtDeviceWdmIrpDispatch - Callback invoked when encountering the given major function.

        DriverContext - An optional untyped driver specified context.

    Returns:

        STATUS_SUCCESS on success
        STATUS_INVALID_PARAMETER if an incorrect MajorFunction was provided
        STATUS_INSUFFICIENT_RESOURCES if insufficient memory was available
        STATUS_INVALID_DEVICE_STATE if this DDI was called at an improper time

 --*/
{
    PFX_DRIVER_GLOBALS  pFxDriverGlobals;
    NTSTATUS            status;
    FxDevice*           pDevice;
    FxCxDeviceInfo*     pCxDeviceInfo;
    ULONG               deviceFlags;

    pDevice = NULL;
    pCxDeviceInfo = NULL;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE,
                                   (PVOID *) &pDevice,
                                   &pFxDriverGlobals);

    //
    // Validate the MajorFunction provided. Note that
    // IRP_MJ_INTERNAL_DEVICE_CONTROL is KMDF only.
    //
    switch (MajorFunction) {
        case IRP_MJ_WRITE:
        case IRP_MJ_READ:
        case IRP_MJ_DEVICE_CONTROL:
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
        case IRP_MJ_INTERNAL_DEVICE_CONTROL:
#endif
            break;
        default:
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "Invalid MajorFunction provided %!IRPMJ!, %!STATUS!",
                MajorFunction, status);
            goto exit;
    }

    //
    // Validate the driver handle and get (if present) the associated cx info.
    //
    if (Driver != NULL) {
        FxDriver*   pDriver;

        FxObjectHandleGetPtr(pFxDriverGlobals,
                             Driver,
                             FX_TYPE_DRIVER,
                             (PVOID*)&pDriver);

        //
        // Find the driver's cx info if it's not the main driver for this device.
        // cx struct info can be NULL if cx acts as client driver.
        //
        pCxDeviceInfo = pDevice->GetCxDeviceInfo(pDriver);
    }

    //
    // Make sure callback is not null.
    //
    FxPointerNotNull(pFxDriverGlobals, EvtDeviceWdmIrpDispatch);

    //
    // This callback can only be called during initialization.
    //
    if (pDevice->IsLegacy()) {

        //
        // Extract the device flags from the device object
        //
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
        deviceFlags = pDevice->GetDeviceObject()->Flags;
#else
        deviceFlags = pDevice->GetDeviceObject()->GetDeviceObjectWdmFlags();
#endif

        //
        // This is a controldevice. Make sure the create is called after the device
        // is initialized and ready to accept I/O.
        //
        if ((deviceFlags & DO_DEVICE_INITIALIZING) == 0x0) {
            status = STATUS_INVALID_DEVICE_STATE;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "Driver cannot set IRP dispatch callback "
                "after WdfControlDeviceFinishInitializing "
                "is called on the WDFDEVICE %p, %!STATUS!",
                pDevice, status);
            goto exit;
        }
    } else {
        //
        // This is either FDO or PDO. Make sure it's not started yet.
        //
        if (pDevice->GetDevicePnpState() != WdfDevStatePnpInit) {
            status = STATUS_INVALID_DEVICE_STATE;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "Driver cannot set IRP dispatch callback "
                "after the WDFDEVICE %p is started, %!STATUS!",
                pDevice, status);
            goto exit;
        }
    }

    //
    // Let I/O package do the rest.
    //
    status = pDevice->m_PkgIo->ConfigureDynamicDispatching(MajorFunction,
                                                           pCxDeviceInfo,
                                                           EvtDeviceWdmIrpDispatch,
                                                           DriverContext);
exit:
    return status;
}

_Must_inspect_result_
NTSTATUS
FX_VF_FUNCTION(VerifyWdfDeviceWdmDispatchIrpToIoQueue) (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ FxDevice* device,
    _In_ MdIrp Irp,
    _In_ FxIoQueue* queue,
    _In_ ULONG Flags
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    UCHAR majorFunction, minorFunction;
    FxIrp fxIrp(Irp);

    PAGED_CODE_LOCKED();

    majorFunction = fxIrp.GetMajorFunction();
    minorFunction = fxIrp.GetMinorFunction();

    DoTraceLevelMessage(
        FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIO,
        "WDFDEVICE 0x%p !devobj 0x%p %!IRPMJ!, IRP_MN %x, IRP 0x%p",
        device->GetHandle(), device->GetDeviceObject(),
        majorFunction, minorFunction, Irp);

    //
    // Validate Flags. For UMDF, this field is reserved and must be zero.
    //
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    if (Flags & ~FX_DISPATCH_IRP_TO_IO_QUEUE_FLAGS_MASK) {
#else
    if (Flags != WDF_DISPATCH_IRP_TO_IO_QUEUE_NO_FLAGS) {
#endif
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                "Flags 0x%x are invalid, %!STATUS!",
                Flags, status);
        FxVerifierDbgBreakPoint(FxDriverGlobals);
        goto Done;
    }

    //
    // Only read/writes/ctrls/internal_ctrls IRPs are allowed, i.e., the I/O request set.
    //
    if (device->GetDispatchPackage(majorFunction) != device->m_PkgIo) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                "Only Read/Write/Control/Internal-Control IRPs can be "
                "forwarded to I/O Queue 0x%p, Irp 0x%p, %!IRPMJ!, "
                "IRP_MN %x, Device 0x%p, %!STATUS!",
                 queue->GetHandle(), Irp, majorFunction, minorFunction,
                 device->GetObjectHandle(), status);
        FxVerifierDbgBreakPoint(FxDriverGlobals);
        goto Done;
    }

    //
    // Make sure queue can handle the request.
    //
    if (FALSE == queue->IsIoEventHandlerRegistered(
                            (WDF_REQUEST_TYPE)majorFunction)) {

        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                "I/O Queue 0x%p cannot handle Irp 0x%p, %!IRPMJ!, "
                "IRP_MN %x, Device 0x%p, %!STATUS!",
                 queue->GetHandle(), Irp, majorFunction, minorFunction,
                 device->GetObjectHandle(), status);
        FxVerifierDbgBreakPoint(FxDriverGlobals);
        goto Done;
    }

    if (device->m_ParentDevice == queue->GetDevice()) {
        //
        // Send to parent device's queue validation.
        //
        if (device->m_ParentDevice == NULL) {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                    FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                    "No parent device for Device 0x%p, %!STATUS!",
                     device->GetObjectHandle(), status);
            FxVerifierDbgBreakPoint(FxDriverGlobals);
            goto Done;
        }

        //
        // Make sure the child device is a PDO
        //
        ASSERT(device->IsPdo());

        //
        // Check if the WdfPdoInitSetForwardRequestToParent was called to
        // increase the StackSize of the child Device  to include the stack
        // size of the parent Device
        //
        if (device->IsPnp() &&
            device->GetPdoPkg()->m_AllowForwardRequestToParent == FALSE) {
            status = STATUS_INVALID_DEVICE_REQUEST;
            DoTraceLevelMessage(
                    FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                    "WdfPdoInitSetForwardRequestToParent not called on "
                    "Device 0x%p, %!STATUS!",
                    device->GetObjectHandle(), status);
            FxVerifierDbgBreakPoint(FxDriverGlobals);
            goto Done;
        }
    }
    else {
        //
        // Send to current device's queue validation.
        //
        if (device != queue->GetDevice()) {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                    FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                    "Cannot forward a request to "
                    "a different Device 0x%p, %!STATUS!",
                    queue->GetDevice()->GetObjectHandle(), status);
            FxVerifierDbgBreakPoint(FxDriverGlobals);
            goto Done;
        }
    }

Done:
    return status;
}

} // extern "C"
