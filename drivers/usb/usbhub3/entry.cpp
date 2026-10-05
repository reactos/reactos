/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Driver entry, cleanup and the global registry policy
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"

#define NDEBUG
#include <debug.h>

HubDriverState HubDriver;

/* USB stack loaders check this to learn which USBD contract the hub speaks */
extern "C" const ULONG Microsoft_USBD_Compat_Version = 0x110;

extern "C" DRIVER_INITIALIZE DriverEntry;
static EVT_WDF_OBJECT_CONTEXT_CLEANUP HubEvtDriverCleanup;

/** One 4 byte value; the registry type is not checked. */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubQueryRegistryULong(
    _In_ WDFKEY Key,
    _In_ PCWSTR Name,
    _Out_ PULONG Value)
{
    UNICODE_STRING ValueName;

    *Value = 0;
    RtlInitUnicodeString(&ValueName, Name);

    return WdfRegistryQueryValue(Key, &ValueName, sizeof(*Value), Value, NULL, NULL);
}

_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubOpenRegistryKey(
    _In_ PCWSTR Path,
    _In_ ACCESS_MASK Access,
    _Out_ WDFKEY* Key)
{
    UNICODE_STRING KeyName;

    RtlInitUnicodeString(&KeyName, Path);
    return WdfRegistryOpenKey(NULL, &KeyName, Access, WDF_NO_OBJECT_ATTRIBUTES, Key);
}

/* One policy value and what a nonzero read does with it */
struct HubPolicyValue
{
    PCWSTR Name;
    VOID (NTAPI *Apply)(_In_ ULONG Value);
};

static
VOID
NTAPI
HubApplyDisableSelectiveSuspendUI(
    _In_ ULONG Value)
{
    if (Value != 0)
        HubDriver.SetFlag(HubGlobal::DisableSelectiveSuspendUI);
}

static
VOID
NTAPI
HubApplyMsOsDescriptorMode(
    _In_ ULONG Value)
{
    if (Value <= (ULONG)HubMsOsMode::NeverQuery)
        HubDriver.MsOsMode = (HubMsOsMode)Value;
    else
        DPRINT1("Ignoring MsOsDescriptorMode %lu\n", Value);
}

static
VOID
NTAPI
HubApplyEnableDiagnosticMode(
    _In_ ULONG Value)
{
    if (Value != 0)
        HubDriver.SetFlag(HubGlobal::EnableDiagnosticMode);
}

/* The default is on; only an explicit zero turns it off */
static
VOID
NTAPI
HubApplyDisableOnSoftRemove(
    _In_ ULONG Value)
{
    if (Value == 0)
        HubDriver.ClearFlag(HubGlobal::DisableOnSoftRemove);
}

static
VOID
NTAPI
HubApplyDisableUxdSupport(
    _In_ ULONG Value)
{
    if (Value != 0)
        HubDriver.SetFlag(HubGlobal::DisableUxdSupport);
}

/* Bit 3 adds reserved field checks, bit 2 the strictest parsing */
static
VOID
NTAPI
HubApplyEnableExtendedValidation(
    _In_ ULONG Value)
{
    if (Value == 0)
        return;

    HubDriver.SetFlag(HubGlobal::EnableExtendedValidation);
    if (Value & 0x8)
        HubDriver.SetFlag(HubGlobal::CheckReservedFields);
    if (Value & 0x4)
        HubDriver.SetFlag(HubGlobal::StrictDescriptorChecks);
}

static
VOID
NTAPI
HubApplyWakeOnConnectUI(
    _In_ ULONG Value)
{
    if (Value != 0)
        HubDriver.SetFlag(HubGlobal::WakeOnConnectUI);
}

static
VOID
NTAPI
HubApplyPreventSuperSpeedDebounce(
    _In_ ULONG Value)
{
    if (Value != 0)
        HubDriver.SetFlag(HubGlobal::PreventSuperSpeedDebounce);
}

static const HubPolicyValue HubGlobalPolicy[] =
{
    { L"DisableSelectiveSuspendUI", HubApplyDisableSelectiveSuspendUI },
    { L"MsOsDescriptorMode", HubApplyMsOsDescriptorMode },
    { L"EnableDiagnosticMode", HubApplyEnableDiagnosticMode },
    { L"DisableOnSoftRemove", HubApplyDisableOnSoftRemove },
    { L"DisableUxdSupport", HubApplyDisableUxdSupport },
    { L"EnableExtendedValidation", HubApplyEnableExtendedValidation },
    { L"WakeOnConnectUI", HubApplyWakeOnConnectUI },
    { L"PreventDebounceTimeForSuperSpeedDevices", HubApplyPreventSuperSpeedDebounce }
};

/* A missing value is skipped; any other read error ends the walk */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubReadGlobalPolicy(VOID)
{
    WDFKEY Key;
    ULONG Index;
    ULONG Value;
    NTSTATUS Status;

    HubDriver.SetFlag(HubGlobal::DisableOnSoftRemove);

    if (!NT_SUCCESS(HubOpenRegistryKey(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\USBHUB\\hubg",
                                       KEY_READ,
                                       &Key)))
    {
        return;
    }

    for (Index = 0; Index < RTL_NUMBER_OF(HubGlobalPolicy); Index++)
    {
        Status = HubQueryRegistryULong(Key, HubGlobalPolicy[Index].Name, &Value);
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
            continue;

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Reading %S failed 0x%lx\n", HubGlobalPolicy[Index].Name, Status);
            break;
        }

        HubGlobalPolicy[Index].Apply(Value);
    }

    WdfRegistryClose(Key);
}

/* On by default with an L1 timeout of 2; read errors are ignored */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubReadUsb20LpmPolicy(VOID)
{
    WDFKEY Key;
    ULONG Value;

    HubDriver.SetFlag(HubGlobal::EnableUsb20HardwareLpm);
    HubDriver.Usb20LpmTimeout = 2;

    if (!NT_SUCCESS(HubOpenRegistryKey(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\usb\\Usb20HardwareLpm",
                                       KEY_READ,
                                       &Key)))
    {
        return;
    }

    if (NT_SUCCESS(HubQueryRegistryULong(Key, L"Usb20HardwareLpmOverride", &Value)))
    {
        if (Value != 0)
            HubDriver.SetFlag(HubGlobal::EnableUsb20HardwareLpm);
        else
            HubDriver.ClearFlag(HubGlobal::EnableUsb20HardwareLpm);
    }

    if (NT_SUCCESS(HubQueryRegistryULong(Key, L"Usb20HardwareLpmTimeout", &Value)) && Value <= MAXUCHAR)
        HubDriver.Usb20LpmTimeout = (UCHAR)Value;

    WdfRegistryClose(Key);
}

_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubReadLtmPolicy(VOID)
{
    WDFKEY Key;
    ULONG Value;

    HubDriver.ClearFlag(HubGlobal::EnableUsbLtm);

    if (!NT_SUCCESS(HubOpenRegistryKey(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\usb\\UsbLtm",
                                       KEY_READ,
                                       &Key)))
    {
        return;
    }

    if (NT_SUCCESS(HubQueryRegistryULong(Key, L"UsbLtmEnable", &Value)))
    {
        if (Value != 0)
            HubDriver.SetFlag(HubGlobal::EnableUsbLtm);
        else
            HubDriver.ClearFlag(HubGlobal::EnableUsbLtm);
    }

    WdfRegistryClose(Key);
}

/* The kernel export is optional; without it no global flags apply */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubReadKseGlobalFlags(VOID)
{
    ULONG64 Flags = 0;

    if (HubDriver.KseQueryDeviceFlags == NULL)
        return;

    HubDriver.KseQueryDeviceFlags(L"USBHUB:GLOBAL_FLAGS", L"USBHUB", &Flags);

    if (Flags & 1)
        HubDriver.SetFlag(HubGlobal::AlwaysSecondReset);
}

/** Reread whenever user experience settings change; the open status is returned. */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubReadUxdPolicy(VOID)
{
    static const struct
    {
        PCWSTR Name;
        HubGlobal Flag;
    } UxdValues[] =
    {
        { L"UxdGlobalDeleteOnShutdown", HubGlobal::UxdDeleteOnShutdown },
        { L"UxdGlobalDeleteOnReload", HubGlobal::UxdDeleteOnReload },
        { L"UxdGlobalDeleteOnDisconnect", HubGlobal::UxdDeleteOnDisconnect },
        { L"UxdGlobalEnable", HubGlobal::UxdEnable }
    };
    WDFKEY Key;
    ULONG Index;
    ULONG Value;
    NTSTATUS Status;

    for (Index = 0; Index < RTL_NUMBER_OF(UxdValues); Index++)
        HubDriver.ClearFlag(UxdValues[Index].Flag);

    Status = HubOpenRegistryKey(L"\\registry\\machine\\system\\currentcontrolset\\services\\usbhub\\uxd_control\\policy",
                                KEY_READ,
                                &Key);
    if (!NT_SUCCESS(Status))
        return Status;

    for (Index = 0; Index < RTL_NUMBER_OF(UxdValues); Index++)
    {
        if (NT_SUCCESS(HubQueryRegistryULong(Key, UxdValues[Index].Name, &Value)) && Value != 0)
            HubDriver.SetFlag(UxdValues[Index].Flag);
    }

    WdfRegistryClose(Key);
    return STATUS_SUCCESS;
}

_IRQL_requires_(PASSIVE_LEVEL)
BOOLEAN
NTAPI
HubIsWinPe(VOID)
{
    return NT_SUCCESS(RtlCheckRegistryKey(RTL_REGISTRY_CONTROL, (PWSTR)L"MiniNT"));
}

/* Internal boot devices are never registered, so their handle stays NULL */
VOID
NTAPI
HubNotifyBootDeviceRemoval(
    _In_opt_ PVOID Handle)
{
    if (Handle != NULL && HubDriver.NotifyBootDeviceRemoval != NULL)
        HubDriver.NotifyBootDeviceRemoval(Handle);
}

_IRQL_requires_(PASSIVE_LEVEL)
static
PVOID
NTAPI
HubResolveKernelRoutine(
    _In_ PCWSTR Name)
{
    UNICODE_STRING RoutineName;

    RtlInitUnicodeString(&RoutineName, Name);
    return MmGetSystemRoutineAddress(&RoutineName);
}

static
VOID
NTAPI
HubEvtDriverCleanup(
    _In_ WDFOBJECT Object)
{
    UNREFERENCED_PARAMETER(Object);

    PAGED_CODE();

    NT_ASSERT(IsListEmpty(&HubDriver.HubList));
    NT_ASSERT(IsListEmpty(&HubDriver.ConnectorMap));

    HubDriver.DriverObject = NULL;
}

extern "C"
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG Config;
    WDF_OBJECT_ATTRIBUTES Attributes;
    NTSTATUS Status;

    HubDriver.DriverObject = DriverObject;
    InitializeListHead(&HubDriver.HubList);
    InitializeListHead(&HubDriver.ConnectorMap);

    WDF_DRIVER_CONFIG_INIT(&Config, HubFdo::EvtDeviceAdd);
    Config.DriverPoolTag = HUB_TAG_DRIVER;

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.EvtCleanupCallback = HubEvtDriverCleanup;

    Status = WdfDriverCreate(DriverObject, RegistryPath, &Attributes, &Config, &HubDriver.Driver);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("WdfDriverCreate failed 0x%lx\n", Status);
        return Status;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = HubDriver.Driver;

    Status = WdfWaitLockCreate(&Attributes, &HubDriver.HubListLock);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub list lock create failed 0x%lx\n", Status);
        return Status;
    }

    Status = WdfWaitLockCreate(&Attributes, &HubDriver.CompanionPortLock);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Connector map lock create failed 0x%lx\n", Status);
        return Status;
    }

    HubReadGlobalPolicy();
    HubReadUsb20LpmPolicy();
    HubReadLtmPolicy();

    HubDriver.KseQueryDeviceFlags =
        (PFN_HUB_KSE_QUERY_DEVICE_FLAGS)HubResolveKernelRoutine(L"KseQueryDeviceFlags");
    HubDriver.RegisterPowerSetting =
        (PFN_HUB_REGISTER_POWER_SETTING)HubResolveKernelRoutine(L"PoRegisterPowerSettingCallback");
    HubDriver.UnregisterPowerSetting =
        (PFN_HUB_UNREGISTER_POWER_SETTING)HubResolveKernelRoutine(L"PoUnregisterPowerSettingCallback");
    HubDriver.GetInterfaceProperty =
        (PFN_HUB_GET_INTERFACE_PROPERTY)HubResolveKernelRoutine(L"IoGetDeviceInterfacePropertyData");

    HubReadKseGlobalFlags();

    DPRINT("Hub driver loaded, global flags 0x%lx\n", (ULONG)HubDriver.GlobalFlags);

    return STATUS_SUCCESS;
}
