/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxTelemetryUm.hpp

Abstract:

    This is header file for telemetry methods.

Author:



Environment:

    User mode only

Revision History:

Notes:

--*/

#pragma once

#include "fxldrum.h"

//
// Event name:  UmdfCensusEvtDeviceStart
//
// Source:      Mode agnostic (UMDF and KMDF)
//
// Description: Written when a FDO completes start successfully.
//
// Frequency:   If FX_TELEMETRY_ENABLED, once per driver session. This is tracked using the
//              DoOnceFlag in the telemetry context.
//
#define UMDF_CENSUS_EVT_WRITE_DEVICE_START(TraceHandle , Globals, DriverConfig, SetupClass, BusEnum, HwID, Manafacturer)    \
            TraceLoggingWrite(TraceHandle,                                                                                  \
                "UmdfCensusEvtDeviceStart",                                                                                 \
                WDF_TELEMETRY_EVT_KEYWORDS,                                                                                 \
                WDF_CENSUS_EVT_DATA_COMMON(Globals),                                                                        \
                TraceLoggingString((Globals)->Public.DriverName,    "DriverServiceName"),                                   \
                TraceLoggingUmdfDriverConfigInfo(DriverConfig,      "DriverConfigInfo"),                                    \
                TraceLoggingWideString(SetupClass,                  "SetupClass"),                                          \
                TraceLoggingWideString(BusEnum,                     "BusEnumerator"),                                       \
                TraceLoggingWideString(HwID,                        "HardwareId"),                                          \
                TraceLoggingWideString(Manafacturer,                "ManufacturerString")                                   \
            );

//
// This is part of the data for UmdfCensusEvtDeviceStart event.
//
#define TraceLoggingUmdfDriverConfigInfo(info, fieldName)   \
    \
    TraceLoggingStruct(20, fieldName),  \
    \
    TraceLoggingUInt8(info.bitmap.IsFilter,                                 "IsFilter"                              ), \
    TraceLoggingUInt8(info.bitmap.IsPowerPolicyOwner,                       "IsPowerPolicyOwner"                    ), \
    TraceLoggingUInt8(info.bitmap.IsS0IdleWakeFromS0Enabled,                "IsS0IdleWakeFromS0Enabled"             ), \
    TraceLoggingUInt8(info.bitmap.IsS0IdleUsbSSEnabled,                     "IsS0IdleUsbSSEnabled"                  ), \
    TraceLoggingUInt8(info.bitmap.IsS0IdleSystemManaged,                    "IsS0IdleSystemManaged"                 ), \
    \
    TraceLoggingUInt8(info.bitmap.IsSxWakeEnabled,                          "IsSxWakeEnabled"                       ), \
    TraceLoggingUInt8(info.bitmap.IsUsingLevelTriggeredLineInterrupt,       "IsUsingLevelTriggeredLineInterrupt"    ), \
    TraceLoggingUInt8(info.bitmap.IsUsingEdgeTriggeredLineInterrupt,        "IsUsingEdgeTriggeredLineInterrupt"     ), \
    TraceLoggingUInt8(info.bitmap.IsUsingMsiXOrSingleMsi22Interrupt,        "IsUsingMsiXOrSingleMsi22Interrupt"     ), \
    TraceLoggingUInt8(info.bitmap.IsUsingMsi22MultiMessageInterrupt,        "IsUsingMsi22MultiMessageInterrupt"     ), \
    \
    TraceLoggingUInt8(info.bitmap.IsUsingMultipleInterrupt,                 "IsUsingMultipleInterrupt"              ), \
    TraceLoggingUInt8(info.bitmap.IsDirectHardwareAccessAllowed,            "IsDirectHardwareAccessAllowed"         ), \
    TraceLoggingUInt8(info.bitmap.IsUsingUserModemappingAccessMode,         "IsUsingUserModemappingAccessMode"      ), \
    TraceLoggingUInt8(info.bitmap.IsKernelModeClientAllowed,                "IsKernelModeClientAllowed"             ), \
    TraceLoggingUInt8(info.bitmap.IsNullFileObjectAllowed,                  "IsNullFileObjectAllowed"               ), \
    \
    TraceLoggingUInt8(info.bitmap.IsPoolingDisabled,                        "IsPoolingDisabled"                     ), \
    TraceLoggingUInt8(info.bitmap.IsMethodNeitherActionCopy,                "IsMethodNeitherActionCopy"             ), \
    TraceLoggingUInt8(info.bitmap.IsUsingDirectIoForReadWrite,              "IsUsingDirectIoForReadWrite"           ), \
    TraceLoggingUInt8(info.bitmap.IsUsingDirectIoForIoctl,                  "IsUsingDirectIoForIoctl"               ), \
    TraceLoggingUInt8(info.bitmap.IsUsingDriverWppRecorder,                 "IsUsingDriverWppRecorder"              )  \

//
// bit-map for driver info stream
//
// When changing the structure, do update TraceLoggingUmdfDriverConfigInfo
// for fields name and TraceLoggingStruct(count) as well. It is good to keep
// fields order the same but it is not required.
//
union UFxTelemetryDriverInfo {
    struct {
        DWORD IsFilter                              : 1;
        DWORD IsPowerPolicyOwner                    : 1;
        DWORD IsS0IdleWakeFromS0Enabled             : 1;
        DWORD IsS0IdleUsbSSEnabled                  : 1;
        DWORD IsS0IdleSystemManaged                 : 1;
        DWORD IsSxWakeEnabled                       : 1;
        DWORD IsUsingLevelTriggeredLineInterrupt    : 1;
        DWORD IsUsingEdgeTriggeredLineInterrupt     : 1;
        DWORD IsUsingMsiXOrSingleMsi22Interrupt     : 1;
        DWORD IsUsingMsi22MultiMessageInterrupt     : 1;
        DWORD IsUsingMultipleInterrupt              : 1;
        DWORD IsDirectHardwareAccessAllowed         : 1;
        DWORD IsUsingUserModemappingAccessMode      : 1;
        DWORD IsKernelModeClientAllowed             : 1;
        DWORD IsNullFileObjectAllowed               : 1;
        DWORD IsPoolingDisabled                     : 1;
        DWORD IsMethodNeitherActionCopy             : 1;
        DWORD IsUsingDirectIoForReadWrite           : 1;
        DWORD IsUsingDirectIoForIoctl               : 1;
        DWORD IsUsingDriverWppRecorder              : 1;
    } bitmap;
    DWORD Dword;
};

typedef struct _UMDF_DRIVER_REGSITRY_INFO {
    BOOLEAN IsKernelModeClientAllowed;
    BOOLEAN IsNullFileObjectAllowed;
    BOOLEAN IsMethodNeitherActionCopy;
    BOOLEAN IsHostProcessSharingDisabled;
} UMDF_DRIVER_REGSITRY_INFO, *PUMDF_DRIVER_REGSITRY_INFO;

VOID
GetDriverInfo(
    _In_ FxDevice* Fdo,
    _In_ PUMDF_DRIVER_REGSITRY_INFO RegInfo,
    _Out_ UFxTelemetryDriverInfo* DriverInfo
    );
