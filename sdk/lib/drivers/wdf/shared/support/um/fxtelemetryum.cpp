/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxTelemetryUm.cpp

Abstract:

    This module implements a telemetry methods.

Author:



Environment:

    User mode only

Revision History:

Notes:

--*/

#include "fxsupportpch.hpp"
#include "DriverFrameworks-UserMode-UmEvents.h"
#include "FxldrUm.h"
#include <winmeta.h>
#include <TraceLoggingProvider.h>
#include <telemetry\MicrosoftTelemetry.h>
#include <rpc.h>
#include <rpcndr.h>

extern "C" {
#if defined(EVENT_TRACING)
#include "FxTelemetryUm.tmh"
#endif
}

/* 8ad60765-a021-4494-8594-9346970cf50f */
TRACELOGGING_DEFINE_PROVIDER(g_TelemetryProvider,
    UMDF_FX_TRACE_LOGGING_PROVIDER_NAME,
    (0x8ad60765, 0xa021, 0x4494, 0x85, 0x94, 0x93, 0x46, 0x97, 0x0c, 0xf5, 0x0f),
    TraceLoggingOptionMicrosoftTelemetry());

VOID
AllocAndInitializeTelemetryContext(
    _In_ PFX_TELEMETRY_CONTEXT* TelemetryContext
    )
{
    PFX_TELEMETRY_CONTEXT context = NULL;
    RPC_STATUS status;

    context = (PFX_TELEMETRY_CONTEXT)MxMemory::MxAllocatePoolWithTag(NonPagedPool,
                                            sizeof(FX_TELEMETRY_CONTEXT),
                                            FX_TAG);
    if (NULL == context) {
        goto exit;
    }

    status = UuidCreate(&(context->DriverSessionGUID));
    if ((status != RPC_S_OK) && (status != RPC_S_UUID_LOCAL_ONLY)) {
        MxMemory::MxFreePool(context);
        context = NULL;
        goto exit;
    }

    context->DoOnceFlagsBitmap = 0;
exit:
    *TelemetryContext = context;
}

VOID
RegisterTelemetryProvider(
    VOID
    )
{
    EventRegisterMicrosoft_Windows_DriverFrameworks_UserMode();

    TraceLoggingRegister(g_TelemetryProvider);
}

VOID
UnregisterTelemetryProvider(
    VOID
    )
{
    EventUnregisterMicrosoft_Windows_DriverFrameworks_UserMode();

    TraceLoggingUnregister(g_TelemetryProvider);
}

VOID
LogDeviceStartTelemetryEvent(
    _In_ PFX_DRIVER_GLOBALS Globals,
    _In_opt_ FxDevice* Fdo
    )
{
    // If provider is not enabled we're done.
    if (FALSE == FX_TELEMETRY_ENABLED(g_TelemetryProvider, Globals)) {
        return;
    }

    //
    // If we already fired an event during PnP start we are done. This avoids
    // repeatedly firing events during PnP rebalance.
    //
    if (InterlockedBitTestAndSet(
            &Globals->TelemetryContext->DoOnceFlagsBitmap,
            DeviceStartEventBit) == 1) {
        return;
    }

    //
    // log the DriverInfo stream.
    //
    LogDriverInfoStream(Globals, Fdo);
}

VOID
LogDriverInfoStream(
    _In_ PFX_DRIVER_GLOBALS DriverGlobals,
    _In_ FxDevice* Fdo
    )
{
    LONG error = ERROR_SUCCESS;
    PWCHAR str = NULL;
    UFxTelemetryDriverInfo driverInfo = {0};
    LPCWSTR hardwareIds = NULL;
    LPCWSTR setupClass = NULL;
    LPCWSTR busEnum = NULL;
    LPCWSTR manufacturer = NULL;
    UMDF_DRIVER_REGSITRY_INFO devRegInfo = {0};
    PCWSTR groupId = NULL;
    IWudfDeviceStack* devStack = NULL;

    if (Fdo == NULL) {
        //
        // Telemetry events are logged during DriverEntry as well to capture non-pnp
        // and class extension (which are non-pnp drivers) driver info. Although
        // current UMDF datapoint doesn't have a separate flag for non-pnp driver,
        // we still want to log the driver name and its properies if available.
        //
        devStack = DriverGlobals->Driver->GetDriverObject()->WudfDevStack;
        if (devStack != NULL) {
            devStack->GetPdoProperties(&hardwareIds,
                                       &setupClass,
                                       &busEnum,
                                       &manufacturer);
        }
    }
    else {
        devStack = Fdo->GetDeviceStack();
        devStack->GetPdoProperties(&hardwareIds,
                                   &setupClass,
                                   &busEnum,
                                   &manufacturer);

        Fdo->RetrieveDeviceInfoRegistrySettings(&groupId, &devRegInfo);
    }

    //
    // Log Driver info
    //
    if (Fdo != NULL) {
        GetDriverInfo(Fdo, &devRegInfo, &driverInfo);
    }

    UMDF_CENSUS_EVT_WRITE_DEVICE_START(g_TelemetryProvider,
                                    DriverGlobals,
                                    driverInfo,
                                    setupClass,
                                    busEnum,
                                    hardwareIds,
                                    manufacturer);

    if (groupId != NULL) {
        delete [] groupId;
        groupId = NULL;
    }
}

VOID
GetDriverInfo(
    _In_ FxDevice* Fdo,
    _In_ PUMDF_DRIVER_REGSITRY_INFO RegInfo,
    _Out_ UFxTelemetryDriverInfo* DriverInfo
    )
{
    FxPkgPnp* pnpPkg;
    USHORT devInfo = 0;
    WDF_DEVICE_IO_TYPE readWritePreference;
    WDF_DEVICE_IO_TYPE ioControlPreference;
    UMDF_DRIVER_REGSITRY_INFO devRegInfo = {0};
    PWSTR groupId = NULL;

    pnpPkg = (FxPkgPnp*)Fdo->GetFdoPkg();
    devInfo = Fdo->GetDeviceTelemetryInfoFlags();
    Fdo->GetDeviceStackIoType(&readWritePreference, &ioControlPreference);

    DriverInfo->bitmap.IsFilter = Fdo->IsFilter();
    DriverInfo->bitmap.IsPowerPolicyOwner = pnpPkg->IsPowerPolicyOwner();
    DriverInfo->bitmap.IsS0IdleWakeFromS0Enabled =  pnpPkg->IsS0IdleWakeFromS0Enabled();
    DriverInfo->bitmap.IsS0IdleUsbSSEnabled = pnpPkg->IsS0IdleUsbSSEnabled();
    DriverInfo->bitmap.IsS0IdleSystemManaged = pnpPkg->IsS0IdleSystemManaged();
    DriverInfo->bitmap.IsSxWakeEnabled = pnpPkg->IsSxWakeEnabled();
    DriverInfo->bitmap.IsUsingLevelTriggeredLineInterrupt = IsDeviceInfoFlagSet(devInfo, DeviceInfoLineBasedLevelTriggeredInterrupt);
    DriverInfo->bitmap.IsUsingEdgeTriggeredLineInterrupt = IsDeviceInfoFlagSet(devInfo, DeviceInfoLineBasedEdgeTriggeredInterrupt);
    DriverInfo->bitmap.IsUsingMsiXOrSingleMsi22Interrupt = IsDeviceInfoFlagSet(devInfo, DeviceInfoMsiXOrSingleMsi22Interrupt);
    DriverInfo->bitmap.IsUsingMsi22MultiMessageInterrupt = IsDeviceInfoFlagSet(devInfo, DeviceInfoMsi22MultiMessageInterrupt);
    DriverInfo->bitmap.IsUsingMultipleInterrupt = pnpPkg->HasMultipleInterrupts();
    DriverInfo->bitmap.IsDirectHardwareAccessAllowed = Fdo->IsDirectHardwareAccessAllowed();
    DriverInfo->bitmap.IsUsingUserModemappingAccessMode = Fdo->AreRegistersMappedToUsermode();
    DriverInfo->bitmap.IsKernelModeClientAllowed = RegInfo->IsKernelModeClientAllowed;
    DriverInfo->bitmap.IsNullFileObjectAllowed = RegInfo->IsNullFileObjectAllowed;
    DriverInfo->bitmap.IsPoolingDisabled = RegInfo->IsHostProcessSharingDisabled;
    DriverInfo->bitmap.IsMethodNeitherActionCopy = RegInfo->IsMethodNeitherActionCopy;
    DriverInfo->bitmap.IsUsingDirectIoForReadWrite = (readWritePreference == WdfDeviceIoDirect);
    DriverInfo->bitmap.IsUsingDirectIoForIoctl = (ioControlPreference == WdfDeviceIoDirect);
    DriverInfo->bitmap.IsUsingDriverWppRecorder = Fdo->GetDriver()->IsDriverObjectFlagSet(DriverObjectUmFlagsLoggingEnabled);

    return;
}

_Must_inspect_result_
NTSTATUS
GetImageName(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _Out_ PUNICODE_STRING ImageName
    )
/*++

Routine Description:
    Retrieve the ImageName value from the named Service registry key.

    Caller is responsible for freeing the buffer allocated in ImageName::Buffer.

Arguments:
    DriverGlobals - pointer to FX_DRIVER_GLOBALS

    ImageeName - Pointer to a UNICODE_STRING which will receive the image name
        upon a return value of NT_SUCCESS()

Return Value:
    NTSTATUS

--*/
{
    NTSTATUS status;
    FxAutoRegKey hKey;
    DECLARE_CONST_UNICODE_STRING(valueName, L"ImagePath");
    UNICODE_STRING imagePath = {0};
    UNICODE_STRING imageName = {0};
    PKEY_VALUE_PARTIAL_INFORMATION value = NULL;
    USHORT size;
    ULONG length, type;
    PVOID dataBuffer;

    type = REG_SZ;
    length = 0;

    ASSERT(ImageName != NULL);
    RtlZeroMemory(ImageName, sizeof(UNICODE_STRING));

    //
    // Open driver's Service base key
    //
    status = FxRegKey::_OpenKey(NULL,
                                FxDriverGlobals->Driver->GetRegistryPathUnicodeString(),
                                &hKey.m_Key,
                                KEY_READ);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
            "Unable to open driver's service key, %!STATUS!", status);
        return status;
    }

    //
    // Find out how big a buffer we need to allocate if the value is present
    //
    status = FxRegKey::_QueryValue(FxDriverGlobals,
                                   hKey.m_Key,
                                   &valueName,
                                   length,
                                   NULL,
                                   &length,
                                   &type);

    //
    // We expect the list to be bigger then a standard partial, so if it is
    // not, just bail now.
    //
    if (status != STATUS_BUFFER_OVERFLOW && status != STATUS_BUFFER_TOO_SMALL) {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Pool can be paged b/c we are running at PASSIVE_LEVEL and we are going
    // to free it at the end of this function.
    //
    dataBuffer = FxPoolAllocate(FxDriverGlobals, PagedPool, length);
    if (dataBuffer == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
            "Failed to allocate memory for image path string, %!STATUS!",
            status);
        return status;
    }

    //
    // Requery now that we have a big enough buffer
    //
    status = FxRegKey::_QueryValue(FxDriverGlobals,
                                   hKey.m_Key,
                                   &valueName,
                                   length,
                                   dataBuffer,
                                   &length,
                                   &type);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
            "Failed to get Image name from service key, %!STATUS!",
            status);
        goto cleanUp;
    }

    //
    // Verify that the data from the registry is a valid string.
    //
    if (type != REG_SZ && type != REG_EXPAND_SZ) {
        return STATUS_OBJECT_TYPE_MISMATCH;
    }

    if (length == 0 || length > USHORT_MAX) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // string must be NULL-terminated
    //
    PWCHAR str = (PWCHAR) dataBuffer;
    if (str[(length/sizeof(WCHAR)) - 1] != UNICODE_NULL) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
            "ERROR: string not terminated with NULL, %!status!\n",
                 status);
        goto cleanUp;
    }

    RtlInitUnicodeString(&imagePath, (PCWSTR) dataBuffer);

    //
    // Now read the "ImagePath" and extract just the driver filename as a new
    // unicode string.
    //
    GetNameFromPath(&imagePath, &imageName);

    if (imageName.Length == 0x0) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
            "ERROR: GetNameFromPath could not find a name, %!status!\n",
                 status);
        goto cleanUp;
    }

    //
    // Check for interger overflow for length before we allocate memory
    // size = path->Length + sizeof(UNICODE_NULL);
    // len is used below to compute the string size including the NULL, so
    // compute len to include the terminating NULL.
    //
    status = RtlUShortAdd(imageName.Length, sizeof(UNICODE_NULL), &size);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
            "ERROR: size computation failed, %!status!\n", status);
        goto cleanUp;
    }

    //
    // allocate a buffer to hold Unicode string + null char.
    //
    ImageName->Buffer = (PWCH) FxPoolAllocate(FxDriverGlobals, PagedPool, size);

    if (ImageName->Buffer == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
            "ERROR: ExAllocatePoolWithTag failed, %!status!\n", status);
        goto cleanUp;
    }

    RtlZeroMemory(ImageName->Buffer, size);
    ImageName->Length = 0x0;
    ImageName->MaximumLength = size;

    HRESULT hr = StringCbCopy(ImageName->Buffer, size, imageName.Buffer);
    if (FAILED(hr)) {
        status = STATUS_UNSUCCESSFUL;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
            "ERROR: failed to copy string buffer, HRESULT 0x%x, %!status!\n",
            hr, status);
        goto cleanUp;
    }

    //
    // The copy cannot fail since we setup the buffer to hold enough space for
    // the contents of the ImagePath value.
    //
    ASSERT(NT_SUCCESS(status));

cleanUp:

    if (!NT_SUCCESS(status)) {
        if (ImageName->Buffer != NULL) {
            FxPoolFree(ImageName->Buffer);
            RtlInitUnicodeString(ImageName, NULL);
        }
    }

    if (dataBuffer != NULL) {
        FxPoolFree(dataBuffer);
    }

    return status;
}
