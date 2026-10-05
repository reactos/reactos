/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Driver wide state, global policy and boot device support
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

/* Global policy bits, from the registry and the kernel shim engine */
enum class HubGlobal : ULONG
{
    DisableSelectiveSuspendUI          = 0x00000002,
    EnableDiagnosticMode               = 0x00000008,
    DisableUxdSupport                  = 0x00000010,
    EnableExtendedValidation           = 0x00000020,
    WakeOnConnectUI                    = 0x00000040,
    DisableOnSoftRemove                = 0x00000080,
    UxdDeleteOnShutdown                = 0x00000100,
    UxdDeleteOnReload                  = 0x00000200,
    UxdDeleteOnDisconnect              = 0x00000400,
    UxdEnable                          = 0x00000800,
    EtwEnabled                         = 0x00001000,
    CheckReservedFields           = 0x00002000,
    StrictDescriptorChecks          = 0x00004000,
    EnableUsb20HardwareLpm             = 0x00008000,
    PreventSuperSpeedDebounce          = 0x00010000,
    AlwaysSecondReset             = 0x00020000,
    EnableUsbLtm                       = 0x00040000
};

/* How MS OS descriptors are queried */
enum class HubMsOsMode : ULONG
{
    QueryOnce = 0,
    ForceQuery = 1,
    NeverQuery = 2
};

typedef
NTSTATUS
(NTAPI *PFN_HUB_KSE_QUERY_DEVICE_FLAGS)(
    _In_ PCWSTR DeviceKey,
    _In_ PCWSTR DeviceClass,
    _Out_ PULONG64 DeviceFlags);

typedef
NTSTATUS
(NTAPI *PFN_HUB_REGISTER_BOOT_DEVICE)(
    _In_ PVOID Registration,
    _Out_ PVOID* Handle);

typedef
VOID
(NTAPI *PFN_HUB_NOTIFY_BOOT_DEVICE_REMOVAL)(
    _In_ PVOID Handle);

typedef
NTSTATUS
(NTAPI *PFN_HUB_REGISTER_POWER_SETTING)(
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_ LPCGUID SettingGuid,
    _In_ PPOWER_SETTING_CALLBACK Callback,
    _In_opt_ PVOID Context,
    _Outptr_opt_ PVOID* Handle);

typedef
NTSTATUS
(NTAPI *PFN_HUB_UNREGISTER_POWER_SETTING)(
    _Inout_ PVOID Handle);

typedef
_Must_inspect_result_
NTSTATUS
(NTAPI *PFN_HUB_GET_INTERFACE_PROPERTY)(
    _In_ PUNICODE_STRING SymbolicLinkName,
    _In_ CONST DEVPROPKEY* PropertyKey,
    _In_ LCID Lcid,
    _Reserved_ ULONG Flags,
    _In_ ULONG Size,
    _Out_writes_bytes_to_(Size, *RequiredSize) PVOID Data,
    _Out_ PULONG RequiredSize,
    _Out_ PDEVPROPTYPE Type);

/** One instance, set up in DriverEntry. */
struct HubDriverState
{
    WDFDRIVER Driver;
    PDRIVER_OBJECT DriverObject;

    volatile LONG GlobalFlags;
    HubMsOsMode MsOsMode;

    /* Hub FDOs between prepare and release hardware */
    LIST_ENTRY HubList;
    WDFWAITLOCK HubListLock;

    /* Owned by the connector module */
    LIST_ENTRY ConnectorMap;
    WDFWAITLOCK CompanionPortLock;

    /* Copied into every USB 2 LPM device update */
    UCHAR Usb20LpmTimeout;

    WDFWMIINSTANCE SurpriseRemovalWmi;

    /* SuperSpeed devices whose last SET_ADDRESS failed */
    volatile LONG SuperSpeedDebounceVotes;

    /* Optional kernel routines; NULL when the kernel lacks them */
    PFN_HUB_KSE_QUERY_DEVICE_FLAGS KseQueryDeviceFlags;
    PFN_HUB_REGISTER_BOOT_DEVICE RegisterBootDevice;
    PFN_HUB_NOTIFY_BOOT_DEVICE_REMOVAL NotifyBootDeviceRemoval;
    PFN_HUB_REGISTER_POWER_SETTING RegisterPowerSetting;
    PFN_HUB_UNREGISTER_POWER_SETTING UnregisterPowerSetting;
    PFN_HUB_GET_INTERFACE_PROPERTY GetInterfaceProperty;

    BOOLEAN
    HasFlag(
        _In_ HubGlobal Flag) const
    {
        return (GlobalFlags & (LONG)Flag) != 0;
    }

    VOID
    SetFlag(
        _In_ HubGlobal Flag)
    {
        InterlockedOr(&GlobalFlags, (LONG)Flag);
    }

    VOID
    ClearFlag(
        _In_ HubGlobal Flag)
    {
        InterlockedAnd(&GlobalFlags, ~(LONG)Flag);
    }
};

extern HubDriverState HubDriver;

/* Live dump report types */
enum class HubReport : ULONG
{
    HubResetSucceeded = 0,
    HubResetFailed = 1,
    DeviceEnumerationFailed = 2
};

/* Live dump bugcheck parameter 1 values */
#define HUB_LIVEDUMP_HUB_RESET_OK          0x3000
#define HUB_LIVEDUMP_HUB_RESET_FAILED      0x3001
#define HUB_LIVEDUMP_SUPERSPEED_DISABLED   0x3002
#define HUB_LIVEDUMP_ENUMERATION_FAILED    0x3003

/* BUGCODE_USB3_DRIVER and what the hub raises with it */
#define HUB_BUGCHECK_USB3                  0x144
#define HUB_USB3_BOOT_DEVICE_FAILED        2
#define HUB_BOOT_DEVICE_IS_STORAGE         1
#define HUB_BOOT_DEVICE_IS_HUB             2

/* errreport.cpp */

NTSTATUS
NTAPI
HubCreateReport(
    _In_ WDFDEVICE HubFdo,
    _In_ HubReport Type,
    _In_ ULONG SubReason);

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubSubmitLiveDump(
    _In_ WDFDEVICE Device,
    _In_ ULONG Code,
    _In_ ULONG SubReason);

/* entry.cpp */

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubReadUxdPolicy(VOID);

/** Kernel boot device removal notice; nothing when the kernel lacks it or Handle is NULL. */
VOID
NTAPI
HubNotifyBootDeviceRemoval(
    _In_opt_ PVOID Handle);

_IRQL_requires_(PASSIVE_LEVEL)
BOOLEAN
NTAPI
HubIsWinPe(VOID);
