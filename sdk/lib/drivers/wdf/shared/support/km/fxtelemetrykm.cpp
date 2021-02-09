/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxTelemetryKm.cpp

Abstract:

    This module implements a telemetry methods.

Author:



Environment:

    Kernel mode only

Revision History:

Notes:

--*/

#include "fxsupportpch.hpp"
#include "fxldr.h"
#include <ntstrsafe.h>
#include <winmeta.h>
#include <telemetry\microsofttelemetry.h>

extern "C" {
#if defined(EVENT_TRACING)
#include "FxTelemetryKm.tmh"
#endif
}

/* ec044b58-3d13-3d13-936f-7b67dfb3e */
TRACELOGGING_DEFINE_PROVIDER(g_TelemetryProvider,
    KMDF_FX_TRACE_LOGGING_PROVIDER_NAME,
    (0xec044b58, 0x3d13, 0x4880, 0x93, 0x6f, 0x7b, 0x67, 0xdf, 0xb3, 0xe0, 0x56),
    TraceLoggingOptionMicrosoftTelemetry());

VOID
AllocAndInitializeTelemetryContext(
    _In_ PFX_TELEMETRY_CONTEXT* TelemetryContext
    )
{
    PFX_TELEMETRY_CONTEXT context = NULL;
    NTSTATUS status;

    context = (PFX_TELEMETRY_CONTEXT)MxMemory::MxAllocatePoolWithTag(
                                                    NonPagedPool,
                                                    sizeof(FX_TELEMETRY_CONTEXT),
                                                    FX_TAG);
    if (NULL == context) {
        goto exit;
    }

    status = ExUuidCreate(&(context->DriverSessionGUID));
    if (!NT_SUCCESS(status)) {
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
    TraceLoggingRegister(g_TelemetryProvider);
}

VOID
UnregisterTelemetryProvider(
    VOID
    )
{
    TraceLoggingUnregister(g_TelemetryProvider);
}

VOID
LogDeviceStartTelemetryEvent(
    _In_ PFX_DRIVER_GLOBALS DriverGlobals,
    _In_opt_ FxDevice* Fdo
    )
{
    //
    // See if telemetry registered and all the criteria to log is met.
    if (IsLoggingEnabledAndNeeded(DriverGlobals) == FALSE) {
        return;
    }

    //
    // Log driver info stream
    //
    LogDriverInfoStream(DriverGlobals, Fdo);
}

BOOLEAN
IsLoggingEnabledAndNeeded(
    _In_ PFX_DRIVER_GLOBALS DriverGlobals
    )
{
    LARGE_INTEGER lastLoggedTime;
    LARGE_INTEGER currentTime;
    LONGLONG delta;

    // If provider is not enabled exit.
    if (FALSE == FX_TELEMETRY_ENABLED(g_TelemetryProvider, DriverGlobals)) {
        return FALSE;
    }

    ASSERT(DriverGlobals->TelemetryContext);

    //
    // If we already fired an event during PnP start we are done. This avoids
    // repeatedly firing events during PnP rebalance.
    //
    if (InterlockedBitTestAndSet(
            &DriverGlobals->TelemetryContext->DoOnceFlagsBitmap,
            DeviceStartEventBit) != 0) {
        return FALSE;
    }

    //
    // log only if it has been MIN_HOURS_BEFORE_NEXT_LOG time since last log.
    // We don't log every time driver loads to avoid sending too much data
    // too many times in  case of a buggy driver going through load/unload cycle
    // or when a device is plugged in two many times.
    //
    lastLoggedTime.QuadPart = 0;
    RegistryReadLastLoggedTime(DriverGlobals, &lastLoggedTime);

    if (lastLoggedTime.QuadPart == 0) {
        //
        // driver is loading for first time ater install so need to log
        // event
        //
        return TRUE;
    }

    Mx::MxQuerySystemTime(&currentTime);

    delta = (currentTime.QuadPart - lastLoggedTime.QuadPart);

    DoTraceLevelMessage(DriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDRIVER,
        "lastlogged %I64x, current %I64x, delta %I64x",
        lastLoggedTime.QuadPart, currentTime.QuadPart, delta);

    //
    // KeQuerySystemTime returns time in 100-ns. We convert MIN_HOURS_BEFORE_NEXT_LOG
    // to 100-nano sec unit and then compare.
    //
    if (delta < WDF_ABS_TIMEOUT_IN_SEC(MIN_HOURS_BEFORE_NEXT_LOG * 60 * 60)) {
        return FALSE;
    }

    return TRUE;
}

VOID
LogDriverInfoStream(
    _In_ PFX_DRIVER_GLOBALS DriverGlobals,
    _In_opt_ FxDevice* Fdo
    )
{
    FxTelemetryDriverInfo driverInfo = {0};
    FxAutoString hardwareIDs, setupClass, busEnum, manufacturer;

    //
    // Log driver and device info
    //
    GetDriverInfo(DriverGlobals, Fdo, &driverInfo);

    if (Fdo != NULL) {
        //
        // Get Setup class
        //
        FxGetDevicePropertyString(Fdo,
                                  DevicePropertyClassName,
                                  &setupClass.m_UnicodeString);
        //
        // Get Bus enumerator
        //
        FxGetDevicePropertyString(Fdo,
                                  DevicePropertyEnumeratorName,
                                  &busEnum.m_UnicodeString);
        //
        // Get hardware id multi-string
        //
        FxGetDevicePropertyString(Fdo,
                                  DevicePropertyHardwareID,
                                  &hardwareIDs.m_UnicodeString);

        GetFirstHardwareId(&hardwareIDs.m_UnicodeString);

        //
        // Get manufacturer
        //
        FxGetDevicePropertyString(Fdo,
                                  DevicePropertyManufacturer,
                                  &manufacturer.m_UnicodeString);
    }

    KMDF_CENSUS_EVT_WRITE_DEVICE_START(g_TelemetryProvider,
        DriverGlobals,
        driverInfo,
        setupClass,
        busEnum,
        hardwareIDs,
        manufacturer);

    //
    // write current time to registry
    //
    RegistryWriteCurrentTime(DriverGlobals);
}

VOID
GetFirstHardwareId(
    _Inout_ PUNICODE_STRING HardwareIds
    )
/*++

    Routine Description:

        This routine returns the first hardware ID present in the multi-string.
        If the first string is longer than max allowed by Telemetry, a null value is
        returned instead of retruning a partial ID.

    Arguments:

        HardwareIds - a multi-string terminated by two unicode_nulls.

--*/

{
    PWCHAR curr;
    USHORT lengthCch;

    ASSERT(HardwareIds != NULL);

    curr = (PWCHAR) HardwareIds->Buffer;
    lengthCch = (HardwareIds->Length)/sizeof(WCHAR);

    //
    // if the caller supplied NULL buffer, then nothing to do.
    //
    if (curr == NULL) {
        RtlInitUnicodeString(HardwareIds, NULL);
        return;
    }

    //
    // if the first element is NULL then update the length
    //
    if (*curr == UNICODE_NULL) {
        HardwareIds->Length = 0;
        HardwareIds->MaximumLength = HardwareIds->Length + sizeof(UNICODE_NULL);
        return;
    }

    for (int i = 0; i < lengthCch; i++, curr++) {

        if (*curr == UNICODE_NULL) {
            //
            // We found the first string. Update size. We only want to keep the
            // first string.
            //
            HardwareIds->Length = (USHORT)(i * sizeof(WCHAR));
            HardwareIds->MaximumLength = HardwareIds->Length + sizeof(UNICODE_NULL);
            return;
        }
    }
}

VOID
GetDriverInfo(
    _In_ PFX_DRIVER_GLOBALS Globals,
    _In_opt_ FxDevice* Fdo,
    _Out_ FxTelemetryDriverInfo* DriverInfo
    )
{
    FxPkgPnp* pnpPkg;
    USHORT devInfo = 0;

    DriverInfo->bitmap.IsVerifierOn = Globals->FxVerifierOn;
    DriverInfo->bitmap.IsEnhancedVerifierOn = FLAG_TO_BOOL(Globals->FxEnhancedVerifierOptions, FxEnhancedVerifierFunctionTableHookMask);

    if (Fdo == NULL) {
        //
        // this is for non-pnp or noDispatchOverride.
        //
        DriverInfo->bitmap.IsNonPnpDriver = FLAG_TO_BOOL(Globals->Public.DriverFlags, WdfDriverInitNonPnpDriver);
        DriverInfo->bitmap.IsNoDispatchOverride = FLAG_TO_BOOL(Globals->Public.DriverFlags, WdfDriverInitNoDispatchOverride);
    }
    else {
        pnpPkg = Fdo->m_PkgPnp;
        devInfo = Fdo->GetDeviceTelemetryInfoFlags();

        DriverInfo->bitmap.IsFilter = Fdo->GetFdoPkg()->IsFilter();
        DriverInfo->bitmap.IsUsingRemoveLockOption = Fdo->IsRemoveLockEnabledForIo();
        DriverInfo->bitmap.IsUsingNonDefaultHardwareReleaseOrder = pnpPkg->IsDefaultReleaseHardwareOrder();
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
        DriverInfo->bitmap.IsUsingPassiveLevelInterrupt = IsDeviceInfoFlagSet(devInfo, DeviceInfoPassiveLevelInterrupt);
        DriverInfo->bitmap.IsUsingBusMasterDma = IsDeviceInfoFlagSet(devInfo, DeviceInfoDmaBusMaster);
        DriverInfo->bitmap.IsUsingSystemDma = IsDeviceInfoFlagSet(devInfo, DeviceInfoDmaSystem);
        DriverInfo->bitmap.IsUsingSystemDmaDuplex = IsDeviceInfoFlagSet(devInfo, DeviceInfoDmaSystemDuplex);
        DriverInfo->bitmap.IsUsingStaticBusEnumration = IsDeviceInfoFlagSet(devInfo, DeviceInfoHasStaticChildren);
        DriverInfo->bitmap.IsUsingDynamicBusEnumeration = IsDeviceInfoFlagSet(devInfo, DeviceInfoHasDynamicChildren);
    }
}

VOID
RegistryReadLastLoggedTime(
    _In_ PFX_DRIVER_GLOBALS DriverGlobals,
    _Out_ PLARGE_INTEGER LastLoggedTime
    )
{
    FxAutoRegKey hKey, hWdf;
    DECLARE_CONST_UNICODE_STRING(parametersPath, L"Parameters\\Wdf");
    DECLARE_CONST_UNICODE_STRING(valueName, WDF_LAST_TELEMETRY_LOG_TIME_VALUE);
    LARGE_INTEGER value;
    NTSTATUS status;

    ASSERT(LastLoggedTime != NULL);
    LastLoggedTime->QuadPart = 0;

    status = FxRegKey::_OpenKey(NULL,
                                DriverGlobals->Driver->GetRegistryPathUnicodeString(),
                                &hWdf.m_Key,
                                KEY_READ);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(DriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
            "Unable to open driver's service key, status %!STATUS!", status);
        return;
    }

    status = FxRegKey::_OpenKey(hWdf.m_Key,
                                &parametersPath,
                                &hKey.m_Key,
                                KEY_READ);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(DriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
            "Unable to open driver's service parameters key, status %!STATUS!",
            status);
        return;
    }

    value.QuadPart = 0;
    status = FxRegKey::_QueryQuadWord(
        hKey.m_Key, &valueName, &value);

    //
    // Set value only on success.
    //
    if (NT_SUCCESS(status)) {
        LastLoggedTime->QuadPart = value.QuadPart;
    }
}

VOID
RegistryWriteCurrentTime(
    _In_ PFX_DRIVER_GLOBALS DriverGlobals
    )
{
    FxAutoRegKey hDriver, hParameters, hWdf;
    DECLARE_CONST_UNICODE_STRING(parametersPart, L"Parameters");
    DECLARE_CONST_UNICODE_STRING(wdfPart, L"Wdf");
    LARGE_INTEGER currentTime;

    //
    // Not defined with the macro because ZwSetValue doesn't use
    // PCUNICODE_STRING
    //
    UNICODE_STRING wdfTimeOfLastTelemetryLog;
    NTSTATUS status;

    RtlInitUnicodeString(&wdfTimeOfLastTelemetryLog, WDF_LAST_TELEMETRY_LOG_TIME_VALUE);

    status = FxRegKey::_OpenKey(NULL,
                                DriverGlobals->Driver->GetRegistryPathUnicodeString(),
                                &hDriver.m_Key,
                                KEY_WRITE | KEY_READ
                                );
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(DriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
            "Unable to open driver's service key, status %!STATUS!", status);
        return;
    }
    //
    //  Key creation, unlike user mode, must happen one level at a time, since
    //  create will also open take both steps instead of trying open first
    //
    status = FxRegKey::_Create(hDriver.m_Key,
                               &parametersPart,
                               &hParameters.m_Key,
                               KEY_WRITE | KEY_READ
                               );
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(DriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
            "Unable to write Parameters key, status %!STATUS!", status);
        return;
    }

    status = FxRegKey::_Create(hParameters.m_Key,
                               &wdfPart,
                               &hWdf.m_Key,
                               KEY_WRITE | KEY_READ
                               );
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(DriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
            "Unable to write Parameters key, status %!STATUS!", status);
        return;
    }

    //
    //  Using ZwSetValueKey here to avoid having to change the implementation
    //  in FxRegKey of SetValue to a static / thiscall pair
    //
    currentTime.QuadPart = 0;
    Mx::MxQuerySystemTime(&currentTime);

    status = Mx::MxSetValueKey(hWdf.m_Key,
                           &wdfTimeOfLastTelemetryLog,
                           0,
                           REG_QWORD,
                           &currentTime.QuadPart,
                           sizeof(currentTime)
                           );

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(DriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
            "Failed to record current time for Telemetry log, status %!STATUS!",
            status);
    }
}

VOID
FxGetDevicePropertyString(
    _In_ FxDevice* Fdo,
    _In_ DEVICE_REGISTRY_PROPERTY  DeviceProperty,
    _Out_ PUNICODE_STRING PropertyString
    )
{
    MdDeviceObject pdo;
    NTSTATUS status;
    PFX_DRIVER_GLOBALS pFxDriverGlobals = Fdo->GetDriverGlobals();
    ULONG length = 0;
    PVOID buffer = NULL;

    ASSERT(PropertyString != NULL);
    RtlZeroMemory(PropertyString, sizeof(UNICODE_STRING));

    pdo = Fdo->GetSafePhysicalDevice();
    if (pdo  == NULL) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Could not get PDO from FDO WDFDEVICE 0x%p, %!STATUS!",
                            Fdo->GetHandle(), status);
        return;
    }

    status = FxDevice::_GetDeviceProperty(pdo, DeviceProperty, 0, NULL, &length);
    if (status != STATUS_BUFFER_TOO_SMALL) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Could not retrieve property %d length %d, %!STATUS!",
                            DeviceProperty, length, status);
        return;
    }

    buffer = FxPoolAllocate(pFxDriverGlobals, PagedPool, length);
    if (buffer == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "Could not allocate memory for property %d length %d, %!STATUS!",
                DeviceProperty, length, status);
        return;
    }

    status = FxDevice::_GetDeviceProperty(pdo, DeviceProperty, length, buffer, &length);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                            "Could not query for full buffer, size %d, for "
                            "property %d, %!STATUS!",
                            length, DeviceProperty, status);
        FxPoolFree(buffer);
        return;
    }

    PropertyString->Buffer = (PWCH)buffer;
    PropertyString->Length = (USHORT) length - sizeof(UNICODE_NULL);
    PropertyString->MaximumLength = (USHORT) length;

    //
    // ensure it's null terminated
    //
    PropertyString->Buffer[PropertyString->Length/sizeof(WCHAR)] = UNICODE_NULL;
}

_Must_inspect_result_
NTSTATUS
GetImageName(
    _In_ PFX_DRIVER_GLOBALS DriverGlobals,
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

    ASSERT(ImageName != NULL);
    RtlZeroMemory(ImageName, sizeof(UNICODE_STRING));

    //
    // Open driver's Service base key
    //
    status = FxRegKey::_OpenKey(NULL,
                                DriverGlobals->Driver->GetRegistryPathUnicodeString(),
                                &hKey.m_Key,
                                KEY_READ);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(DriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
            "Unable to open driver's service key, status %!STATUS!", status);
        return status;
    }

    status = QueryAndAllocString(hKey.m_Key,
                                DriverGlobals,
                                &valueName,
                                &value);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(DriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
            "Failed to get Image name from service key, status %!STATUS!",
            status);
        return status;
    }

    BuildStringFromPartialInfo(value, &imagePath);

    //
    // Now read the "ImagePath" and extract just the driver filename as a new
    // unicode string.
    //
    GetNameFromPath(&imagePath, &imageName);

    if (imageName.Length == 0x0) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(DriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
            "ERROR: GetNameFromPath could not find a name, status 0x%x\n",
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
        DoTraceLevelMessage(DriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
            "ERROR: size computation failed with Status 0x%x\n", status);
        goto cleanUp;
    }

    //
    // allocate a buffer to hold Unicode string + null char.
    //
    ImageName->Buffer = (PWCH) FxPoolAllocate(DriverGlobals, PagedPool, size);

    if (ImageName->Buffer == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(DriverGlobals, TRACE_LEVEL_ERROR, TRACINGDRIVER,
            "ERROR: ExAllocatePoolWithTag failed with Status 0x%x\n", status);
        goto cleanUp;
    }

    RtlZeroMemory(ImageName->Buffer, size);
    ImageName->Length = 0x0;
    ImageName->MaximumLength = size;

    status = RtlUnicodeStringCopy(ImageName, &imageName);

    //
    // The copy cannot fail since we setup the buffer to hold enough space for
    // the contents of the ImagePath value.
    //
    ASSERT(NT_SUCCESS(status));

cleanUp:

    if (value != NULL) {
        FxPoolFree(value);
    }

    return status;
}


_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
QueryAndAllocString(
    _In_  HANDLE Key,
    _In_  PFX_DRIVER_GLOBALS Globals,
    _In_  PCUNICODE_STRING ValueName,
    _Out_ PKEY_VALUE_PARTIAL_INFORMATION* Info
    )
{
    PKEY_VALUE_PARTIAL_INFORMATION info;
    NTSTATUS status;
    ULONG length;

    status = STATUS_UNSUCCESSFUL;
    info = NULL;

    ASSERT(Info != NULL);
    *Info = NULL;

    status = Mx::MxQueryValueKey(Key,
                             (PUNICODE_STRING)ValueName,
                             KeyValuePartialInformation,
                             NULL,
                             0,
                             &length);

    if (!NT_SUCCESS(status) && status != STATUS_BUFFER_TOO_SMALL) {
        goto cleanup;
    }

    //
    // Pool can be paged b/c we are running at PASSIVE_LEVEL and we are going
    // to free it at the end of this function.
    //
    status = RtlULongAdd(length,
                         FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data),
                         &length);

    if (!NT_SUCCESS(status)) {
        goto cleanup;
    }

    info = (PKEY_VALUE_PARTIAL_INFORMATION) FxPoolAllocate(Globals,
                                                           PagedPool,
                                                           length);

    if (info == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    RtlZeroMemory(info, length);

    //
    // Query registry for the data under ValueName
    //
    status = Mx::MxQueryValueKey(Key,
                             (PUNICODE_STRING) ValueName,
                             KeyValuePartialInformation,
                             info,
                             length,
                             &length);


    if (NT_SUCCESS(status)) {
        if (info->Type != REG_SZ && info->Type != REG_EXPAND_SZ) {
            status = STATUS_OBJECT_TYPE_MISMATCH;
            goto cleanup;
        }

        if (info->DataLength == 0 ||
            (info->DataLength % 2) != 0 ||
            (info->DataLength >
            (length - FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data)))) {
            status = STATUS_INVALID_PARAMETER;
            goto cleanup;
        }

        *Info = info;
    }

cleanup:

    if (!NT_SUCCESS(status)) {
        if (info != NULL) {
            FxPoolFree(info);
        }
    }

    return status;
}
