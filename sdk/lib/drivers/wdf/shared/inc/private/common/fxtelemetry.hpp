/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxTelemetry.hpp

Abstract:

    This is the header file for core framework (Wdf0100 and Wudfx02000)
    related telemetry.

Author:



Environment:

    Both kernel and user mode

Revision History:

Notes:

--*/

#pragma once

#include <strsafe.h>
#include "FxTelemetryCommon.hpp"

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
#include "FxTelemetryKm.hpp"
#else
#include "FxTelemetryUm.hpp"
#endif

#if defined(__cplusplus)
extern "C" {
#endif

TRACELOGGING_DECLARE_PROVIDER(g_TelemetryProvider);

#define FX_TELEMETRY_ENABLED(TraceHandle, Globals)                \
    (TraceHandle && IsDriverTelemetryContextInitialized(Globals))     \

#define WDF_CENSUS_EVT_DATA_COMMON(FxGlobals)                     \
    TraceLoggingStruct(1, "CensusCommonV1"),                      \
        TraceLoggingGuid((FxGlobals)->TelemetryContext->DriverSessionGUID, "SessionGUID")

//
// For events that want to fire once per
// session define a bit for use in DoOnceFlagsBitmap
//
typedef enum _FX_TELEMETRY_DO_ONCE_BITS {
    DeviceStartEventBit = 0
} FX_TELEMETRY_DO_ONCE_BITS;

//
// Event name:  WdfCensusEvtDrvLoad
//
// Source:      Mode agnostic (UMDF and KMDF)
//
// Description: Written when a WDF client or cx calls WdfDriverCreate.
//              The event contains information about the driver version,
//              verifier options, service name and driver configuration
//              to track non-pnp drivers or WDF miniports.
//
// Frequency:   If FX_TELEMETRY_ENABLED then everytime a driver calls WdfDriverCreate.
//
#define WDF_CENSUS_EVT_WRITE_DRIVER_LOAD(TraceHandle, Globals, DrvImage, WdfVersion)   \
            TraceLoggingWrite(TraceHandle,                                             \
                "WdfCensusEvtDrvLoad",                                                 \
                WDF_TELEMETRY_EVT_KEYWORDS,                                            \
                WDF_CENSUS_EVT_DATA_COMMON(Globals),                                   \
                TraceLoggingStruct(9, "DriverInfo"                                                                                       ), \
                    TraceLoggingString((Globals)->Public.DriverName,                                         "DriverService"             ), \
                    TraceLoggingWideString(DrvImage,                                                         "DriverImage"               ), \
                    TraceLoggingWideString(WdfVersion,                                                       "WdfVersion"                ), \
                    TraceLoggingUInt32((Globals)->WdfBindInfo->Version.Major,                                "DriverVersionMajor"        ), \
                    TraceLoggingUInt32((Globals)->WdfBindInfo->Version.Minor,                                "DriverVersionMinor"        ), \
                    TraceLoggingBoolean((Globals)->FxVerifierOn,                                             "FxVerifierOn"              ), \
                    TraceLoggingBoolean(!!((Globals)->Public.DriverFlags & WdfDriverInitNonPnpDriver),       "DriverNonPnP"              ), \
                    TraceLoggingBoolean(!!((Globals)->Public.DriverFlags & WdfDriverInitNoDispatchOverride), "DriverNoDispatchOverride"  ), \
                    TraceLoggingUInt32((Globals)->FxEnhancedVerifierOptions,                                 "FxEnhancedVeriferOptions"  )  \
                );

#define MIN_HOURS_BEFORE_NEXT_LOG  24
#define BASE_10 (10)

#define WDF_LAST_TELEMETRY_LOG_TIME_VALUE L"TimeOfLastTelemetryLog"
#define WDF_DRIVER_IMAGE_NAME_VALUE L"ImagePath"

//
// bit-flags for tracking hardware info for device start telemetry event
//
enum FxDeviceInfoFlags : USHORT {
    DeviceInfoLineBasedLevelTriggeredInterrupt =   0x1,
    DeviceInfoLineBasedEdgeTriggeredInterrupt  =   0x2,
    DeviceInfoMsiXOrSingleMsi22Interrupt       =   0x4,
    DeviceInfoMsi22MultiMessageInterrupt       =   0x8,
    DeviceInfoPassiveLevelInterrupt            =  0x10,
    DeviceInfoDmaBusMaster                     =  0x20,
    DeviceInfoDmaSystem                        =  0x40,
    DeviceInfoDmaSystemDuplex                  =  0x80,
    DeviceInfoHasStaticChildren                = 0x100,
    DeviceInfoHasDynamicChildren               = 0x200,
    DeviceInfoIsUsingDriverWppRecorder         = 0x400
};

//
// wdf version strig example "01.011"
//
#define WDF_VERSION_STRING_SIZE_INCLUDING_SEPARATOR_CCH   10

BOOLEAN
__inline
IsDeviceInfoFlagSet(
    _In_ USHORT DeviceInfo,
    _In_ FxDeviceInfoFlags Flag
    )
{
    return FLAG_TO_BOOL(DeviceInfo, Flag);
}

VOID
AllocAndInitializeTelemetryContext(
    _In_ PFX_TELEMETRY_CONTEXT* TelemetryContext
    );

__inline
BOOLEAN
IsDriverTelemetryContextInitialized(
_In_ PFX_DRIVER_GLOBALS FxDrvGlobals
)
{
    ASSERT(FxDrvGlobals);
    return (NULL != FxDrvGlobals->TelemetryContext);
}

VOID
RegisterTelemetryProvider(
    VOID
    );

VOID
UnregisterTelemetryProvider(
    VOID
    );

VOID
LogDeviceStartTelemetryEvent(
    _In_ PFX_DRIVER_GLOBALS Globals,
    _In_opt_ FxDevice* Fdo
    );

VOID
LogDriverInfoStream(
    _In_ PFX_DRIVER_GLOBALS Globals,
    _In_opt_ FxDevice* Fdo
    );

_Must_inspect_result_
NTSTATUS
GetImageName(
    _In_ PFX_DRIVER_GLOBALS DriverGlobals,
    _Out_ PUNICODE_STRING ImageName
    );

VOID
__inline
BuildStringFromPartialInfo(
    _In_ PKEY_VALUE_PARTIAL_INFORMATION Info,
    _Out_ PUNICODE_STRING String
    )
{
    String->Buffer = (PWCHAR) &Info->Data[0];
    String->MaximumLength = (USHORT) Info->DataLength;
    String->Length = String->MaximumLength - sizeof(UNICODE_NULL);

    //
    // ensure string is null terminated
    //
    String->Buffer[String->Length/sizeof(WCHAR)] = UNICODE_NULL;
}

VOID
GetNameFromPath(
    _In_ PCUNICODE_STRING Path,
    _Out_ PUNICODE_STRING Name
    );

#if defined(__cplusplus)
}
#endif

