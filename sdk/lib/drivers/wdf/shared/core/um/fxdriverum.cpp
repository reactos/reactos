/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxDriverUm.cpp

Abstract:

    This is the main driver framework.

Author:



Environment:

    User mode only

Revision History:



--*/

#include "coreprivshared.hpp"
#include "fxiotarget.hpp"
#include "fxldrum.h"

// Tracing support
extern "C" {
#include "FxDriverUm.tmh"
}


_Must_inspect_result_
NTSTATUS
FxDriver::AddDevice (
    _In_  PDRIVER_OBJECT_UM         DriverObject,
    _In_  PVOID                     Context,
    _In_  IWudfDeviceStack *        DevStack,
    _In_  LPCWSTR                   KernelDeviceName,
    _In_opt_ HKEY                   PdoKey,
    _In_  LPCWSTR                   ServiceName,
    _In_  LPCWSTR                   DevInstanceID,
    _In_  ULONG                     DriverID
    )
{
    FxDriver *pDriver;

    //
    // Context parameter is CWudfDriverGlobals in legacy UMDF. Not used in
    // UMDF 2.0
    //
    UNREFERENCED_PARAMETER(Context);

    pDriver = FxDriver::GetFxDriver(DriverObject);



    if (pDriver != NULL) {
        return pDriver->AddDevice(DevStack,
                                  KernelDeviceName,
                                  PdoKey,
                                  ServiceName,
                                  DevInstanceID,
                                  DriverID
                                  );
    }

    return STATUS_UNSUCCESSFUL;
}

_Must_inspect_result_
NTSTATUS
FxDriver::AddDevice(
    _In_  IWudfDeviceStack *        DevStack,
    _In_  LPCWSTR                   KernelDeviceName,
    _In_opt_ HKEY                   PdoKey,
    _In_  LPCWSTR                   ServiceName,
    _In_  LPCWSTR                   DevInstanceID,
    _In_  ULONG                     DriverID
    )
{
    WDFDEVICE_INIT init(this);
    FxDevice* pDevice;
    NTSTATUS status;
    HRESULT hr = S_OK;
    LONG lRetVal = -1;

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Enter AddDevice DevStack %p", DevStack);

    //FX_VERIFY(INTERNAL, CHECK_NOT_NULL(DevStack));
    //FX_VERIFY(INTERNAL, CHECK_NOT_NULL(KernelDeviceName));
    //FX_VERIFY(INTERNAL, CHECK_HANDLE(PdoKey));
    //FX_VERIFY(INTERNAL, CHECK_NOT_NULL(ServiceName));
    //FX_VERIFY(INTERNAL, CHECK_NOT_NULL(DevInstanceID));

    pDevice = NULL;
    init.CreatedOnStack = TRUE;
    init.InitType = FxDeviceInitTypeFdo;
    init.Fdo.PhysicalDevice = NULL;

    //
    // Capture the input parameters
    //
    init.DevStack = DevStack;
    init.DriverID = DriverID;

    lRetVal = RegOpenKeyEx(
        PdoKey,
        NULL,
        0,
        KEY_READ,
        &init.PdoKey
        );

    if (ERROR_SUCCESS != lRetVal) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                            "Registry key open failed for the PDO key, "
                            "winerror %!WINERROR!", lRetVal);

        hr = HRESULT_FROM_WIN32(lRetVal);
        return FxDevice::NtStatusFromHr(DevStack, hr);
    }

    size_t len = 0;
    hr = StringCchLengthW(ServiceName, STRSAFE_MAX_CCH, &len);
    if (FAILED(hr)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                            "Registry path string too long or badly formed "
                            "path. Invalid configuration HRESULT %!hresult!",
                            hr);
        return FxDevice::NtStatusFromHr(DevStack, hr);
    }

    len += 1;    // Add one for the string termination character
    init.ConfigRegistryPath = new WCHAR[len];
    if (NULL == init.ConfigRegistryPath) {
        hr = E_OUTOFMEMORY;
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                            "Failed to allocate memory for Config path"
                            " HRESULT %!hresult!", hr);

        return FxDevice::NtStatusFromHr(DevStack, hr);
    }

    hr = StringCchCopyW(init.ConfigRegistryPath, len, ServiceName);
    if (FAILED(hr)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                            "Failed to copy the configuration path status "
                            "%!hresult!", hr);
        return FxDevice::NtStatusFromHr(DevStack, hr);
    }

    //
    // Capture the PDO device instance ID.
    //
    len = 0;
    hr = StringCchLengthW(DevInstanceID, STRSAFE_MAX_CCH, &len);
    if (FAILED(hr)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                            "Device Instance ID string too long or badly formed"
                            " path. Invalid configuration %!hresult!", hr);
        return FxDevice::NtStatusFromHr(DevStack, hr);
    }

    len += 1; // Add one for the string termination character
    init.DevInstanceID = new WCHAR[len];
    if (NULL == init.DevInstanceID) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                            "Failed to allocate memory for DevInstanceID "
                            "%!hresult!", hr);
        hr = E_OUTOFMEMORY;
        return FxDevice::NtStatusFromHr(DevStack, hr);
    }

    hr = StringCchCopyW(init.DevInstanceID, len, DevInstanceID);
    if (FAILED(hr)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                            "Unable to copy DevInstanceID %!hresult!", hr);
        return FxDevice::NtStatusFromHr(DevStack, hr);
    }

    //
    // Capture Kernel device name.
    //
    len = 0;
    hr = StringCchLengthW(KernelDeviceName, STRSAFE_MAX_CCH, &len);
    if (FAILED(hr)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                            "Unable to determine KernelDeviceName length"
                            "%!hresult!", hr);
        return FxDevice::NtStatusFromHr(DevStack, hr);
    }

    len += 1; // Add one for string termination character.
    init.KernelDeviceName = new WCHAR[len];
    if (init.KernelDeviceName == NULL) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                            "Failed to allocate memory for KernelDeviceName "
                            "%!hresult!", hr);
        hr = E_OUTOFMEMORY;
        return FxDevice::NtStatusFromHr(DevStack, hr);
    }

    hr = StringCchCopyW(init.KernelDeviceName, len, KernelDeviceName);
    if (FAILED(hr)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                            "Unable to copy kernel device name KernelDeviceName"
                            " %!hresult!", hr);
        return FxDevice::NtStatusFromHr(DevStack, hr);
    }

    //
    // Invoke driver's AddDevice callback
    //
    status = m_DriverDeviceAdd.Invoke(GetHandle(), &init);

    //
    // Caller returned w/out creating a device, we are done.  Returning
    // STATUS_SUCCESS w/out creating a device and attaching to the stack is OK,
    // especially for filter drivers which selectively attach to devices.
    //
    if (init.CreatedDevice == NULL) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGPNP,
                            "Driver did not create a device in "
                            "EvtDriverAddDevice, status %!STATUS!", status);

        //
        // We do not let filters affect the building of the rest of the stack.
        // If they return error, we convert it to STATUS_SUCCESS.
        //
        if (init.Fdo.Filter && !NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
                "Filter returned %!STATUS! without creating a WDFDEVICE, "
                "converting to STATUS_SUCCESS", status);
            status = STATUS_SUCCESS;
        }

        return status;
    }

    pDevice = init.CreatedDevice;

    if (NT_SUCCESS(status)) {
        //
        // Make sure that DO_DEVICE_INITIALIZING is cleared.
        // FxDevice::FdoInitialize does not do this b/c the driver writer may
        // want the bit set until sometime after WdfDeviceCreate returns
        //
        pDevice->FinishInitializing();
    }
    else {
        //
        // Created a device, but returned error.
        //
        ASSERT(pDevice->IsPnp());
        ASSERT(pDevice->m_CurrentPnpState == WdfDevStatePnpInit);

        status = pDevice->DeleteDeviceFromFailedCreate(status, TRUE);
        pDevice = NULL;
    }

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Exit, status %!STATUS!", status);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxDriver::AllocateDriverObjectExtensionAndStoreFxDriver(
    VOID
    )
{
    //
    // No allocation needed for user-mode, just store FxDriver in driver object.
    //
    m_DriverObject.GetObject()->FxDriver = this;

    return STATUS_SUCCESS;
}

FxDriver*
FxDriver::GetFxDriver(
    __in MdDriverObject DriverObject
    )
{
    return DriverObject->FxDriver;
}

VOID
FxDriver::ClearDriverObjectFxDriver(
    VOID
    )
{
    PDRIVER_OBJECT_UM pDriverObj = m_DriverObject.GetObject();

    if (pDriverObj != NULL) {
        pDriverObj->FxDriver = NULL;
    }
}

NTSTATUS
FxDriver::OpenParametersKey(
    VOID
    )
{
    HRESULT hr;
    NTSTATUS status;

    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();
    PDRIVER_OBJECT_UM pDrvObj = GetDriverObject();
    IWudfDeviceStack* pDevStack = (IWudfDeviceStack*)pDrvObj->WudfDevStack;

    UMINT::WDF_PROPERTY_STORE_ROOT rootSpecifier;
    UMINT::WDF_PROPERTY_STORE_RETRIEVE_FLAGS flags;
    CANSI_STRING serviceNameA;
    DECLARE_UNICODE_STRING_SIZE(serviceNameW, WDF_DRIVER_GLOBALS_NAME_LEN);
    HKEY hKey;

    RtlInitAnsiString(&serviceNameA, FxDriverGlobals->Public.DriverName);
    status = RtlAnsiStringToUnicodeString(&serviceNameW,
                                          &serviceNameA,
                                          FALSE);
    if (NT_SUCCESS(status)) {
        rootSpecifier.LengthCb = sizeof(UMINT::WDF_PROPERTY_STORE_ROOT);
        rootSpecifier.RootClass = UMINT::WdfPropertyStoreRootDriverParametersKey;
        rootSpecifier.Qualifier.ParametersKey.ServiceName = serviceNameW.Buffer;

        flags = UMINT::WdfPropertyStoreCreateIfMissing;

        hr = pDevStack->CreateRegistryEntry(&rootSpecifier,
                                            flags,
                                            GENERIC_ALL & ~(GENERIC_WRITE | KEY_CREATE_SUB_KEY | WRITE_DAC),
                                            NULL,
                                            &hKey,
                                            NULL);
        status = FxDevice::NtStatusFromHr(pDevStack, hr);
        if (NT_SUCCESS(status)) {
            m_DriverParametersKey = hKey;
        }
    }

    return status;
}


