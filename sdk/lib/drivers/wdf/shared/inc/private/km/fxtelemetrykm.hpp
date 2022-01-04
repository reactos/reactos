/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxTelemetryKm.hpp

Abstract:

    This is header file for telemetry methods.

Author:



Environment:

    Kernel mode only

Revision History:

Notes:

--*/

#pragma once

#include "fxldr.h"

#if defined(__cplusplus)
extern "C" {
#endif

//
// Event name:  KmdfCensusEvtDeviceStart
//
// Source:      KMDF only
//
// Description: Written when a FDO completes start successfully.
//
// Frequency:   If FX_TELEMETRY_ENABLED, once per driver session. This is tracked using the
//              DoOnceFlag in the telemetry context. Also not to exceed
//              once in 24 hours (last write time stored in registry).
//
//
#define KMDF_CENSUS_EVT_WRITE_DEVICE_START(TraceHandle , Globals, DriverConfig, SetupClass, BusEnum, HwID, Manafacturer)    \
            TraceLoggingWrite(TraceHandle,                                                                                  \
                "KmdfCensusEvtDeviceStart",                                                                                 \
                WDF_TELEMETRY_EVT_KEYWORDS,                                                                                 \
                WDF_CENSUS_EVT_DATA_COMMON(Globals),                                                                        \
                TraceLoggingString((Globals)->Public.DriverName,            "DriverServiceName"),                           \
                TraceLoggingKmdfDriverConfigInfo(DriverConfig,              "DriverConfigInfo"),                            \
                TraceLoggingWideString(SetupClass.m_UnicodeString.Buffer,   "SetupClass"),                                  \
                TraceLoggingWideString(BusEnum.m_UnicodeString.Buffer,      "BusEnumerator"),                               \
                TraceLoggingWideString(HwID.m_UnicodeString.Buffer,         "HardwareId"),                                  \
                TraceLoggingWideString(Manafacturer.m_UnicodeString.Buffer, "ManufacturerString")                           \
            );

//
// This is part of the data for KmdfCensusEvtDeviceStart event.
//
#define TraceLoggingKmdfDriverConfigInfo(info, fieldName)   \
    \
    TraceLoggingStruct(23, fieldName),  \
    \
    TraceLoggingUInt8(info.bitmap.IsNonPnpDriver,                           "IsNonPnpDriver"                        ), \
    TraceLoggingUInt8(info.bitmap.IsNoDispatchOverride,                     "IsNoDispatchOverride"                  ), \
    TraceLoggingUInt8(info.bitmap.IsVerifierOn,                             "IsVerifierOn"                          ), \
    TraceLoggingUInt8(info.bitmap.IsEnhancedVerifierOn,                     "IsEnhancedVerifierOn"                  ), \
    TraceLoggingUInt8(info.bitmap.IsFilter,                                 "IsFilter"                              ), \
    \
    TraceLoggingUInt8(info.bitmap.IsUsingRemoveLockOption,                  "IsUsingRemoveLockOption"               ), \
    TraceLoggingUInt8(info.bitmap.IsUsingNonDefaultHardwareReleaseOrder,    "IsUsingNonDefaultHardwareReleaseOrder" ), \
    TraceLoggingUInt8(info.bitmap.IsPowerPolicyOwner,                       "IsPowerPolicyOwner"                    ), \
    TraceLoggingUInt8(info.bitmap.IsS0IdleWakeFromS0Enabled,                "IsS0IdleWakeFromS0Enabled"             ), \
    TraceLoggingUInt8(info.bitmap.IsS0IdleUsbSSEnabled,                     "IsS0IdleUsbSSEnabled"                  ), \
    \
    TraceLoggingUInt8(info.bitmap.IsS0IdleSystemManaged,                    "IsS0IdleSystemManaged"                 ), \
    TraceLoggingUInt8(info.bitmap.IsSxWakeEnabled,                          "IsSxWakeEnabled"                       ), \
    TraceLoggingUInt8(info.bitmap.IsUsingLevelTriggeredLineInterrupt,       "IsUsingLevelTriggeredLineInterrupt"    ), \
    TraceLoggingUInt8(info.bitmap.IsUsingEdgeTriggeredLineInterrupt,        "IsUsingEdgeTriggeredLineInterrupt"     ), \
    TraceLoggingUInt8(info.bitmap.IsUsingMsiXOrSingleMsi22Interrupt,        "IsUsingMsiXOrSingleMsi22Interrupt"     ), \
    \
    TraceLoggingUInt8(info.bitmap.IsUsingMsi22MultiMessageInterrupt,        "IsUsingMsi22MultiMessageInterrupt"     ), \
    TraceLoggingUInt8(info.bitmap.IsUsingMultipleInterrupt,                 "IsUsingMultipleInterrupt"              ), \
    TraceLoggingUInt8(info.bitmap.IsUsingPassiveLevelInterrupt,             "IsUsingPassiveLevelInterrupt"          ), \
    TraceLoggingUInt8(info.bitmap.IsUsingBusMasterDma,                      "IsUsingBusMasterDma"                   ), \
    TraceLoggingUInt8(info.bitmap.IsUsingSystemDma,                         "IsUsingSystemDma"                      ), \
    \
    TraceLoggingUInt8(info.bitmap.IsUsingSystemDmaDuplex,                   "IsUsingSystemDmaDuplex"                ), \
    TraceLoggingUInt8(info.bitmap.IsUsingStaticBusEnumration,               "IsUsingStaticBusEnumration"            ), \
    TraceLoggingUInt8(info.bitmap.IsUsingDynamicBusEnumeration,             "IsUsingDynamicBusEnumeration"          )  \

//
// When changing the structure, do update TraceLoggingKmdfDriverConfigInfo
// for fields name and TraceLoggingStruct(count) as well. It is good to keep
// fields order the same but it is not required.
//
union FxTelemetryDriverInfo {
    struct {
        DWORD IsNonPnpDriver                        : 1;
        DWORD IsNoDispatchOverride                  : 1;
        DWORD IsVerifierOn                          : 1;
        DWORD IsEnhancedVerifierOn                  : 1;
        DWORD IsFilter                              : 1;
        DWORD IsUsingRemoveLockOption               : 1;
        DWORD IsUsingNonDefaultHardwareReleaseOrder : 1;
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
        DWORD IsUsingPassiveLevelInterrupt          : 1;
        DWORD IsUsingBusMasterDma                   : 1;
        DWORD IsUsingSystemDma                      : 1;
        DWORD IsUsingSystemDmaDuplex                : 1;
        DWORD IsUsingStaticBusEnumration            : 1;
        DWORD IsUsingDynamicBusEnumeration          : 1;
    } bitmap;
    DWORD Dword;
};


VOID
RegistryWriteCurrentTime(
    _In_ PFX_DRIVER_GLOBALS DriverGlobals
    );

VOID
RegistryReadLastLoggedTime(
    _In_ PFX_DRIVER_GLOBALS DriverGlobals,
    _Out_ PLARGE_INTEGER CurrentTime
    );

NTSTATUS
GetHardwareIdAndSetupclassFromRegistry(
    _In_ FxDevice* Fdo,
    _Out_ PUNICODE_STRING HwIds,
    _Out_ PUNICODE_STRING SetupClass
    );

BOOLEAN
IsLoggingEnabledAndNeeded(
    _In_ PFX_DRIVER_GLOBALS DriverGlobals
    );

_Must_inspect_result_
NTSTATUS
FxQueryData(
    _In_ PFX_DRIVER_GLOBALS DriverGlobals,
    _In_ HANDLE Key,
    _In_ PCUNICODE_STRING ValueName,
    _In_ ULONG Tag,
    _Out_ PKEY_VALUE_PARTIAL_INFORMATION* Info
    );

VOID
GetDriverInfo(
    _In_ PFX_DRIVER_GLOBALS Globals,
    _In_opt_ FxDevice* Fdo,
    _Out_ FxTelemetryDriverInfo* DriverInfo
    );

VOID
FxGetDevicePropertyString(
    _In_ FxDevice* Fdo,
    _In_ DEVICE_REGISTRY_PROPERTY  DeviceProperty,
    _Out_ PUNICODE_STRING PropertyString
    );

VOID
GetFirstHardwareId(
    _Inout_ PUNICODE_STRING HardwareIds
    );

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
QueryAndAllocString(
    _In_  HANDLE Key,
    _In_  PFX_DRIVER_GLOBALS Globals,
    _In_  PCUNICODE_STRING ValueName,
    _Out_ PKEY_VALUE_PARTIAL_INFORMATION* Info
    );


#if defined(__cplusplus)
}
#endif

