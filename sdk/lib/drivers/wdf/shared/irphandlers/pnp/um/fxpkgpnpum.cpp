//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include "..\pnppriv.hpp"

#include <initguid.h>
#include <wdmguid.h>

extern "C" {
#if defined(EVENT_TRACING)
#include "FxPkgPnpUM.tmh"
#endif
}

NTSTATUS
FxPkgPnp::FilterResourceRequirements(
    __in IO_RESOURCE_REQUIREMENTS_LIST **IoList
    )
{
    UNREFERENCED_PARAMETER(IoList);
    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::AllocateDmaEnablerList(
    VOID
    )
{
    ASSERTMSG("Not implemented for UMDF\n", FALSE);

    return STATUS_NOT_IMPLEMENTED;
}

VOID
FxPkgPnp::AddDmaEnabler(
    __in FxDmaEnabler* Enabler
    )
{
    UNREFERENCED_PARAMETER(Enabler);
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}

VOID
FxPkgPnp::RemoveDmaEnabler(
    __in FxDmaEnabler* Enabler
    )
{
    UNREFERENCED_PARAMETER(Enabler);
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}

NTSTATUS
FxPkgPnp::UpdateWmiInstance(
    _In_ FxWmiInstanceAction Action,
    _In_ BOOLEAN ForS0Idle
    )
{
    HRESULT hr;
    NTSTATUS status;
    IWudfDeviceStack* devStack;
    WmiIdleWakeInstanceUpdate updateType;

    devStack = m_Device->GetDeviceStack();

    ASSERT(Action != InstanceActionInvalid);

    if (Action == AddInstance) {
        updateType = ForS0Idle ? AddS0IdleInstance : AddSxWakeInstance;
    } else {
        updateType = ForS0Idle ? RemoveS0IdleInstance : RemoveSxWakeInstance;
    }

    hr = devStack->UpdateIdleWakeWmiInstance(updateType);
    if (S_OK == hr) {
        status = STATUS_SUCCESS;
    }
    else {
        PUMDF_VERSION_DATA driverVersion = devStack->GetMinDriverVersion();
        BOOL preserveCompat =
             devStack->ShouldPreserveIrpCompletionStatusCompatibility();

        status = CHostFxUtil::NtStatusFromHr(hr,
                                             driverVersion->MajorNumber,
                                             driverVersion->MinorNumber,
                                             preserveCompat);
    }

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(),
                            TRACE_LEVEL_ERROR,
                            TRACINGPNP,
                            "failed to send ioctl to update %s WMI instance "
                            "%!STATUS!",
                            ForS0Idle ? "S0Idle" : "SxWake",
                            status);
    }

    return status;
}

NTSTATUS
FxPkgPnp::ReadStateFromRegistry(
    _In_ PCUNICODE_STRING ValueName,
    _Out_ PULONG Value
    )
{
    DWORD err;
    NTSTATUS status;
    DWORD data;
    DWORD dataSize;
    HKEY pwrPolKey = NULL;
    IWudfDeviceStack* devStack;

    ASSERT(NULL != Value);
    ASSERT(ValueName != NULL &&
           ValueName->Length != 0 &&
           ValueName->Buffer != NULL);

    *Value = 0;
    devStack = m_Device->GetDeviceStack();

    err = RegOpenKeyEx(devStack->GetDeviceRegistryKey(),
                       WUDF_POWER_POLICY_SETTINGS,
                       0,
                       KEY_READ,
                       &pwrPolKey);
    if (ERROR_SUCCESS != err) {
        DoTraceLevelMessage(GetDriverGlobals(),
                            TRACE_LEVEL_ERROR,
                            TRACINGPNP,
                            "RegOpenKeyEx returned error %d",
                            err);
        goto Clean;
    }

    dataSize = sizeof(data);
    err = RegQueryValueEx(pwrPolKey,
                          ValueName->Buffer,
                          NULL,
                          NULL,
                          (BYTE*) &data,
                          &dataSize);
    if (ERROR_SUCCESS != err) {
        DoTraceLevelMessage(GetDriverGlobals(),
                            TRACE_LEVEL_ERROR,
                            TRACINGPNP,
                            "failed to read registry, "
                            "RegQueryValueEx returned error %d",
                            err);
        goto Clean;
    }

    *Value = data;
    err = ERROR_SUCCESS;

Clean:
    if (NULL != pwrPolKey) {
        RegCloseKey(pwrPolKey);
    }

    if (ERROR_SUCCESS == err) {
        status = STATUS_SUCCESS;
    }
    else {
        PUMDF_VERSION_DATA driverVersion = devStack->GetMinDriverVersion();
        BOOL preserveCompat =
             devStack->ShouldPreserveIrpCompletionStatusCompatibility();

        status = CHostFxUtil::NtStatusFromHr(HRESULT_FROM_WIN32(err),
                                             driverVersion->MajorNumber,
                                             driverVersion->MinorNumber,
                                             preserveCompat);
    }

    return status;
}

VOID
FxPkgPnp::WriteStateToRegistry(
    __in HANDLE RegKey,
    __in PUNICODE_STRING ValueName,
    __in ULONG Value
    )
{
    DWORD err;
    HRESULT hr;
    HKEY hKey = NULL;
    IWudfDeviceStack* devStack;
    UMINT::WDF_PROPERTY_STORE_ROOT propertyStore;

    UNREFERENCED_PARAMETER(RegKey);

    ASSERT(ValueName != NULL &&
           ValueName->Length != 0 &&
           ValueName->Buffer != NULL);

    devStack = m_Device->GetDeviceStack();

    propertyStore.LengthCb = sizeof(UMINT::WDF_PROPERTY_STORE_ROOT);
    propertyStore.RootClass = UMINT::WdfPropertyStoreRootClassHardwareKey;
    propertyStore.Qualifier.HardwareKey.ServiceName = WUDF_POWER_POLICY_SETTINGS;

    hr = devStack->CreateRegistryEntry(&propertyStore,
                                       UMINT::WdfPropertyStoreCreateIfMissing,
                                       KEY_QUERY_VALUE | KEY_SET_VALUE,
                                       NULL,
                                       &hKey,
                                       NULL);
    if (FAILED(hr)) {
        goto Clean;
    }

    //
    // Failure to save the user's idle/wake settings is not critical and we
    // will continue on regardless. Hence we ignore the return value.
    //
    err = RegSetValueEx(hKey,
                        ValueName->Buffer,
                        0,
                        REG_DWORD,
                        (BYTE *) &Value,
                        sizeof(Value));
    if (err != ERROR_SUCCESS) {
        DoTraceLevelMessage(GetDriverGlobals(),
                            TRACE_LEVEL_ERROR,
                            TRACINGPNP,
                            "Failed to set Registry value "
                            "for S0Idle/SxWake error %d",
                            err);
        goto Clean;
    }

Clean:
    if (NULL != hKey) {
        RegCloseKey(hKey);
    }
}

NTSTATUS
FxPkgPnp::UpdateWmiInstanceForS0Idle(
    __in FxWmiInstanceAction Action
    )
{
    NTSTATUS status;

    //
    // Send an IOCTL to redirector
    // to add/remove S0Idle WMI instance.
    //
    status = UpdateWmiInstance(Action, TRUE);

    return status;
}

VOID
FxPkgPnp::ReadRegistryS0Idle(
    __in PCUNICODE_STRING ValueName,
    __out BOOLEAN *Enabled
    )
{
    NTSTATUS status;
    ULONG value;

    status = ReadStateFromRegistry(ValueName, &value);

    //
    // Modify value of Enabled only if success
    //
    if (NT_SUCCESS(status)) {
        //
        // Normalize the ULONG value into a BOOLEAN
        //
        *Enabled = (value == FALSE) ? FALSE : TRUE;
    }
}

NTSTATUS
FxPkgPnp::UpdateWmiInstanceForSxWake(
    __in FxWmiInstanceAction Action
    )
{
    NTSTATUS status;

    //
    // Send an IOCTL to redirector
    // to add/remove SxWake WMI instance.
    //
    status = UpdateWmiInstance(Action, FALSE);

    return status;
}

VOID
FxPkgPnp::ReadRegistrySxWake(
    __in PCUNICODE_STRING ValueName,
    __out BOOLEAN *Enabled
    )
{
    NTSTATUS status;
    ULONG value;

    status = ReadStateFromRegistry(ValueName, &value);

    //
    // Modify value of Enabled only if success
    //
    if (NT_SUCCESS(status)) {
        //
        // Normalize the ULONG value into a BOOLEAN
        //
        *Enabled = (value == FALSE) ? FALSE : TRUE;
    }
}

VOID
PnpPassThroughQIWorker(
    __in    MxDeviceObject* Device,
    __inout FxIrp* Irp,
    __inout FxIrp* ForwardIrp
    )
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(ForwardIrp);
    ASSERTMSG("Not implemented for UMDF\n", FALSE);
}


VOID
FxPkgPnp::RevokeDmaEnablerResources(
    __in FxDmaEnabler * /* DmaEnabler */
    )
{
    // Do nothing
}

