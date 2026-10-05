/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Per device and per hub registry state: errata, hardware keys, UXD and telemetry
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"
#include "hubregistry.h"

#define NDEBUG
#include <debug.h>

static const WCHAR HubUsbflagsPath[] =
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\usbflags";
static const WCHAR HubVerifierPath[] =
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\usb\\HardwareVerifier";
static const WCHAR HubUxdDevicesPath[] =
    L"\\registry\\machine\\system\\currentcontrolset\\services\\usbhub\\uxd_control\\devices";
static const WCHAR HubUxdPnpPath[] =
    L"\\registry\\machine\\system\\currentcontrolset\\services\\usbhub\\uxd_control\\pnp";
static const WCHAR HubGlobalCeipPath[] =
    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Usb\\Ceip";
static const WCHAR HubUsbControlPath[] =
    L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\USB";
static const WCHAR HubUsbFnPath[] =
    L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\USBFN";
static const WCHAR HubUsbFnDefaultPath[] =
    L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\USBFN\\Default";
static const WCHAR HubUsbFnConfigPath[] =
    L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\USBFN\\Configurations\\";

/* Function drivers opt in to D3cold under this subkey of the hardware key */
static const WCHAR HubD3ColdSubkey[] = L"e5b3b5ac-9725-4f78-963f-03dfb1d828c7";

/* Port D3cold reconnect wait unless the hardware key says otherwise, in ms */
#define HUB_D3COLD_RECONNECT_DEFAULT    1000

/* Bits in the shim engine's per device flag words */
#define KSE_SKIP_MS_OS                  0
#define KSE_DONT_SKIP_MS_OS             1
#define KSE_KEEP_ON_SOFT_REMOVE         3
#define KSE_LPM_OFF_FOR_CHAINED_HUBS    16
#define KSE_LPM_OFF_DOWNSTREAM          26
#define KSE_POLLING_TO_COMPLIANCE       35
#define KSE_NO_U2_FOR_TUNNELED_DEVICES  48

/* Dual role feature bits this machine may offer */
#define HUB_DUAL_ROLE_MTP               0x00000002
#define HUB_DUAL_ROLE_IP_OVER_USB       0x00000004
#define HUB_DUAL_ROLE_VIDEO             0x00000008
#define HUB_DUAL_ROLE_FUNCTIONS         0x0000000E
#define HUB_DUAL_ROLE_UCM_PRESENT       0x80000000

/* Shared primitives */

VOID
NTAPI
HubFormatIdStrings(
    _In_ const USB_DEVICE_DESCRIPTOR* Descriptor,
    _Out_ HubIdStrings* Ids)
{
    RtlStringCbPrintfA(Ids->Vendor, sizeof(Ids->Vendor), "%04X", Descriptor->idVendor);
    RtlStringCbPrintfA(Ids->Product, sizeof(Ids->Product), "%04X", Descriptor->idProduct);
    RtlStringCbPrintfA(Ids->Revision, sizeof(Ids->Revision), "%04X", Descriptor->bcdDevice);
}

/* Root hubs go by their controller; anything unknown gets the 7FFF placeholder */
static
VOID
NTAPI
HubFormatControllerIds(
    _In_ const UCXHUB_CONTROLLER_INFO_V2* Info,
    _Out_ HubIdStrings* Ids)
{
    switch (Info->Type)
    {
        case UcxControllerParentBusTypePci:
            RtlStringCbPrintfA(Ids->Vendor, sizeof(Ids->Vendor), "%04X", Info->Pci.VendorId);
            RtlStringCbPrintfA(Ids->Product, sizeof(Ids->Product), "%04X", Info->Pci.DeviceId);
            RtlStringCbPrintfA(Ids->Revision, sizeof(Ids->Revision), "%04X", Info->Pci.RevisionId);
            break;

        case UcxControllerParentBusTypeAcpi:
            RtlCopyMemory(Ids->Vendor, Info->Acpi.VendorId, sizeof(Ids->Vendor));
            RtlCopyMemory(Ids->Product, Info->Acpi.DeviceId, sizeof(Ids->Product));
            RtlCopyMemory(Ids->Revision, Info->Acpi.RevisionId, sizeof(Ids->Revision));
            Ids->Vendor[sizeof(Ids->Vendor) - 1] = ANSI_NULL;
            Ids->Product[sizeof(Ids->Product) - 1] = ANSI_NULL;
            Ids->Revision[sizeof(Ids->Revision) - 1] = ANSI_NULL;
            break;

        default:
            RtlStringCbPrintfA(Ids->Vendor, sizeof(Ids->Vendor), "%04X", MAXLONG);
            RtlStringCbPrintfA(Ids->Product, sizeof(Ids->Product), "%04X", MAXLONG);
            RtlStringCbPrintfA(Ids->Revision, sizeof(Ids->Revision), "%04X", 0);
            break;
    }
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubRegReadValue(
    _In_ WDFKEY Key,
    _In_ PCWSTR Name,
    _In_ ULONG Size,
    _Out_writes_bytes_(Size) PVOID Data)
{
    UNICODE_STRING ValueName;

    RtlZeroMemory(Data, Size);
    RtlInitUnicodeString(&ValueName, Name);

    return WdfRegistryQueryValue(Key, &ValueName, Size, Data, NULL, NULL);
}

/** Reads one value of a walk. A missing value only clears Present; FALSE means stop. */
_IRQL_requires_(PASSIVE_LEVEL)
static
BOOLEAN
NTAPI
HubRegWalkDword(
    _In_ WDFKEY Key,
    _In_ PCWSTR Name,
    _Out_ PULONG Value,
    _Out_ PBOOLEAN Present,
    _Inout_ NTSTATUS* Status)
{
    *Status = HubRegReadValue(Key, Name, sizeof(*Value), Value);
    *Present = NT_SUCCESS(*Status);

    if (*Status == STATUS_OBJECT_NAME_NOT_FOUND)
        return TRUE;

    return *Present;
}

_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubRegOpen(
    _In_opt_ WDFKEY Parent,
    _In_ PCUNICODE_STRING Name,
    _In_ ACCESS_MASK Access,
    _Out_ WDFKEY* Key)
{
    return WdfRegistryOpenKey(Parent, Name, Access, WDF_NO_OBJECT_ATTRIBUTES, Key);
}

_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubRegOpenPath(
    _In_ PCWSTR Path,
    _In_ ACCESS_MASK Access,
    _Out_ WDFKEY* Key)
{
    UNICODE_STRING Name;

    RtlInitUnicodeString(&Name, Path);
    return HubRegOpen(NULL, &Name, Access, Key);
}

_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubRegOpenOrCreate(
    _In_opt_ WDFKEY Parent,
    _In_ PCUNICODE_STRING Name,
    _In_ ACCESS_MASK OpenAccess,
    _In_ ACCESS_MASK CreateAccess,
    _Out_ WDFKEY* Key)
{
    NTSTATUS Status;

    Status = HubRegOpen(Parent, Name, OpenAccess, Key);
    if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
        return Status;

    return WdfRegistryCreateKey(Parent, Name, CreateAccess, REG_OPTION_NON_VOLATILE, NULL, WDF_NO_OBJECT_ATTRIBUTES, Key);
}

_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubOpenDeviceKey(
    _In_ HubChild* Child,
    _In_ ACCESS_MASK Access,
    _Out_ WDFKEY* Key)
{
    *Key = NULL;

    if (Child->m_Pdo == NULL || Child->m_Pdo->m_Device == NULL)
        return STATUS_INVALID_DEVICE_STATE;

    return WdfDeviceOpenRegistryKey(Child->m_Pdo->m_Device, PLUGPLAY_REGKEY_DEVICE, Access, WDF_NO_OBJECT_ATTRIBUTES, Key);
}

_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubOpenHubKey(
    _In_ HubFdo* Hub,
    _In_ ACCESS_MASK Access,
    _Out_ WDFKEY* Key)
{
    return WdfDeviceOpenRegistryKey(Hub->m_Device, PLUGPLAY_REGKEY_DEVICE, Access, WDF_NO_OBJECT_ATTRIBUTES, Key);
}

/* Opens usbflags and its VVVVPPPPRRRR subkey, creating both even for a read since user mode tools expect them */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubOpenUsbflagsKeys(
    _In_ const HubIdStrings* Ids,
    _In_ ACCESS_MASK Access,
    _Out_ WDFKEY* RootKey,
    _Out_ WDFKEY* DeviceKey)
{
    WCHAR Buffer[26];
    UNICODE_STRING Name;
    WDFKEY Root;
    NTSTATUS Status;

    *RootKey = NULL;
    *DeviceKey = NULL;

    RtlInitUnicodeString(&Name, HubUsbflagsPath);
    Status = HubRegOpenOrCreate(NULL, &Name, Access, Access, &Root);
    if (!NT_SUCCESS(Status))
        return Status;

    RtlInitEmptyUnicodeString(&Name, Buffer, sizeof(Buffer));
    Status = RtlUnicodeStringPrintf(&Name, L"%hs%hs%hs", Ids->Vendor, Ids->Product, Ids->Revision);
    if (NT_SUCCESS(Status))
        Status = HubRegOpenOrCreate(Root, &Name, Access, KEY_ALL_ACCESS, DeviceKey);

    if (!NT_SUCCESS(Status))
    {
        WdfRegistryClose(Root);
        *DeviceKey = NULL;
        return Status;
    }

    *RootKey = Root;
    return STATUS_SUCCESS;
}

/*
 * Two shim engine lookups, by VID+PID+REV and by VID+PID, merged into one
 * word. Without the kernel routine, which is the ReactOS case, it stays 0.
 */
_IRQL_requires_(PASSIVE_LEVEL)
static
ULONG64
NTAPI
HubQueryKseDeviceFlags(
    _In_ const HubIdStrings* Ids,
    _In_ BOOLEAN RootHub)
{
    PCWSTR Bus = RootHub ? L"ROOT_HUB" : L"USB";
    WCHAR Key[50];
    ULONG64 Flags = 0;
    ULONG64 Word;

    if (HubDriver.KseQueryDeviceFlags == NULL)
        return 0;

    if (!NT_SUCCESS(RtlStringCchPrintfW(Key,
                                        RTL_NUMBER_OF(Key),
                                        L"USB:%ls\\VID_%hs&PID_%hs&REV_%hs",
                                        Bus,
                                        Ids->Vendor,
                                        Ids->Product,
                                        Ids->Revision)))
    {
        return 0;
    }

    Word = 0;
    HubDriver.KseQueryDeviceFlags(Key, L"USB", &Word);
    Flags |= Word;

    if (!NT_SUCCESS(RtlStringCchPrintfW(Key,
                                        RTL_NUMBER_OF(Key),
                                        L"USB:%ls\\VID_%hs&PID_%hs",
                                        Bus,
                                        Ids->Vendor,
                                        Ids->Product)))
    {
        return Flags;
    }

    Word = 0;
    HubDriver.KseQueryDeviceFlags(Key, L"USB", &Word);
    return Flags | Word;
}

FORCEINLINE
BOOLEAN
NTAPI
HubKseBit(
    _In_ ULONG64 Flags,
    _In_ ULONG Bit)
{
    return (Flags & (1ULL << Bit)) != 0;
}

/* VIA VL810 hubs before revision 0x89 cannot run SuperSpeed; only the low revision byte counts */
static
BOOLEAN
NTAPI
HubIsBrokenVl810(
    _In_ const USB_DEVICE_DESCRIPTOR* Descriptor)
{
    return Descriptor->idVendor == 0x2109 &&
           Descriptor->idProduct == 0x0810 &&
           (UCHAR)Descriptor->bcdDevice < 0x89;
}

/* Per device errata */

/* Extra condition on a shim engine bit */
enum class HubErrataGate : UCHAR
{
    None,
    BehindExternalHub,
    NotHub
};

/*
 * One errata source. With a value name, a present value decides alone
 * (nonzero sets) and the shim bit only counts when the value is absent.
 */
struct HubDeviceErrata
{
    PCWSTR Value;
    UCHAR KseBit;
    HubErrataGate Gate;
    ChildHack Hack;
};

static const HubDeviceErrata HubDeviceErrataBeforeSoftRemove[] =
{
    { L"IgnoreHWSerNum", 6, HubErrataGate::None, ChildHack::DisableSerialNumber },
    { L"UseWin8DescriptorValidation", 31, HubErrataGate::None, ChildHack::UseWin8DescriptorValidation },
    { L"ResetOnResume", 2, HubErrataGate::None, ChildHack::ResetAfterSystemResume }
};

static const HubDeviceErrata HubDeviceErrataAfterSoftRemove[] =
{
    { L"RequestConfigDescOnReset", 4, HubErrataGate::None, ChildHack::RequestConfigDescOnReset },
    { L"SkipContainerIdQuery", 5, HubErrataGate::None, ChildHack::SkipContainerIdQuery },
    { L"DisableLPM", 12, HubErrataGate::None, ChildHack::DisableLpm },
    { NULL, 10, HubErrataGate::None, ChildHack::IgnoreBosValidationFailure },
    { NULL, 14, HubErrataGate::None, ChildHack::NoExitLatencyRequest },
    { NULL, 16, HubErrataGate::BehindExternalHub, ChildHack::DisableLpm },
    { NULL, 19, HubErrataGate::None, ChildHack::TolerateStalePipes },
    { NULL, 21, HubErrataGate::None, ChildHack::DisableUasp },
    { NULL, 23, HubErrataGate::None, ChildHack::NoIsochDelayRequest },
    { NULL, 24, HubErrataGate::None, ChildHack::ResetAfterIdleResume },
    { L"SkipBOSDescriptorQuery", 27, HubErrataGate::None, ChildHack::SkipBosQuery },
    { NULL, 13, HubErrataGate::None, ChildHack::DisableUsb20Lpm },
    { NULL, 17, HubErrataGate::None, ChildHack::DisableRemoteWakeForUsb20Lpm },
    { NULL, 30, HubErrataGate::None, ChildHack::BlockedDevice },
    { NULL, 22, HubErrataGate::NotHub, ChildHack::DisableSuperSpeed },
    { NULL, 32, HubErrataGate::None, ChildHack::AlwaysSecondReset }
};

_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubApplyDeviceErrata(
    _In_ HubChild* Child,
    _In_ WDFKEY Key,
    _In_ ULONG64 Kse,
    _In_reads_(Count) const HubDeviceErrata* Table,
    _In_ ULONG Count)
{
    ULONG Index;
    ULONG Value;
    BOOLEAN Apply;
    NTSTATUS Status;

    for (Index = 0; Index < Count; Index++)
    {
        const HubDeviceErrata* Entry = &Table[Index];

        Apply = HubKseBit(Kse, Entry->KseBit);

        if (Entry->Value != NULL)
        {
            Status = HubRegReadValue(Key, Entry->Value, sizeof(Value), &Value);
            if (NT_SUCCESS(Status))
                Apply = (Value != 0);
            else if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
                return Status;
        }
        else if (Entry->Gate == HubErrataGate::BehindExternalHub)
        {
            Apply = Apply && !Child->m_Hub->IsRootHub();
        }
        else if (Entry->Gate == HubErrataGate::NotHub)
        {
            Apply = Apply && !Child->HasProperty(ChildProperty::IsHub);
        }

        if (Apply)
            Child->m_Hacks |= (ULONG)Entry->Hack;
    }

    return STATUS_SUCCESS;
}

/* Pairs of (interface, alternate setting). An older filter is freed once a new one exists. */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubReadAlternateSettingFilter(
    _In_ HubChild* Child,
    _In_ WDFKEY Key)
{
    DECLARE_CONST_UNICODE_STRING(Name, L"AlternateSettingFilter");
    HubAltSettingFilter* Filter;
    ULONG Size = 0;
    ULONG Type = REG_NONE;

    if (WdfRegistryQueryValue(Key, &Name, 0, NULL, &Size, &Type) != STATUS_BUFFER_OVERFLOW)
        return;

    if (Size == 0 || (Size & 1) || Type != REG_BINARY)
        return;

    Filter = (HubAltSettingFilter*)ExAllocatePoolWithTag(NonPagedPool,
                                                         FIELD_OFFSET(HubAltSettingFilter, Entries) + Size,
                                                         HUB_TAG_DEVICE);
    if (Filter == NULL)
        return;

    RtlZeroMemory(Filter, FIELD_OFFSET(HubAltSettingFilter, Entries) + Size);

    if (Child->m_AlternateSettingFilter != NULL)
        ExFreePoolWithTag(Child->m_AlternateSettingFilter, HUB_TAG_DEVICE);
    Child->m_AlternateSettingFilter = Filter;

    if (!NT_SUCCESS(WdfRegistryQueryValue(Key, &Name, Size, Filter->Entries, NULL, NULL)))
    {
        ExFreePoolWithTag(Filter, HUB_TAG_DEVICE);
        Child->m_AlternateSettingFilter = NULL;
        return;
    }

    Filter->Count = Size / sizeof(Filter->Entries[0]);
}

/*
 * Bits only accumulate, except DisableOnSoftRemove, which is forced on and
 * reread. A read error other than a missing value fails the attempt.
 */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubQueryDeviceErrata(
    _In_ HubChild* Child,
    _In_ const HubIdStrings* Ids)
{
    WCHAR NameBuffer[32];
    UNICODE_STRING Name;
    WDFKEY Root;
    WDFKEY Key;
    ULONG64 Kse;
    ULONG Value;
    HubMsOs20SetInfo SetInfo;
    NTSTATUS Status;

    Status = HubOpenUsbflagsKeys(Ids, KEY_READ, &Root, &Key);
    if (!NT_SUCCESS(Status))
        return Status;

    Kse = HubQueryKseDeviceFlags(Ids, FALSE);

    /* Legacy per VID and PID serial number override in the usbflags root */
    RtlInitEmptyUnicodeString(&Name, NameBuffer, sizeof(NameBuffer));
    Status = RtlUnicodeStringPrintf(&Name, L"IgnoreHWSerNum%hs%hs", Ids->Vendor, Ids->Product);
    if (!NT_SUCCESS(Status))
        goto Done;

    Value = 0;
    Status = WdfRegistryQueryValue(Root, &Name, sizeof(Value), &Value, NULL, NULL);
    if (NT_SUCCESS(Status))
    {
        if (Value != 0)
            Child->m_Hacks |= (ULONG)ChildHack::DisableSerialNumber;
    }
    else if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
    {
        goto Done;
    }

    /* osvc: byte 1 is the vendor code; two zero bytes mean no MS OS descriptor */
    Status = HubRegReadValue(Key, L"osvc", sizeof(USHORT), &Value);
    if (NT_SUCCESS(Status))
    {
        if ((USHORT)Value != 0)
            Child->m_MsOsVendorCode = (UCHAR)(Value >> 8);
        else
            Child->SetProperty(ChildProperty::MsOsNotSupported);
    }
    else if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        if (HubKseBit(Kse, KSE_SKIP_MS_OS))
            Child->SetProperty(ChildProperty::MsOsNotSupported);
        else if (HubKseBit(Kse, KSE_DONT_SKIP_MS_OS))
            Child->m_Hacks |= (ULONG)ChildHack::AlwaysQueryMsOs;
    }
    else
    {
        goto Done;
    }

    Status = HubApplyDeviceErrata(Child,
                                  Key,
                                  Kse,
                                  HubDeviceErrataBeforeSoftRemove,
                                  RTL_NUMBER_OF(HubDeviceErrataBeforeSoftRemove));
    if (!NT_SUCCESS(Status))
        goto Done;

    /* On by default for every device; only an explicit zero or the shim turns it off */
    Child->m_Hacks |= (ULONG)ChildHack::DisableOnSoftRemove;

    Status = HubRegReadValue(Key, L"DisableOnSoftRemove", sizeof(Value), &Value);
    if (NT_SUCCESS(Status))
    {
        if (Value == 0)
            Child->m_Hacks &= ~(ULONG)ChildHack::DisableOnSoftRemove;
    }
    else if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        if (HubKseBit(Kse, KSE_KEEP_ON_SOFT_REMOVE))
            Child->m_Hacks &= ~(ULONG)ChildHack::DisableOnSoftRemove;
    }
    else
    {
        goto Done;
    }

    Status = HubApplyDeviceErrata(Child,
                                  Key,
                                  Kse,
                                  HubDeviceErrataAfterSoftRemove,
                                  RTL_NUMBER_OF(HubDeviceErrataAfterSoftRemove));
    if (!NT_SUCCESS(Status))
        goto Done;

    /* Set info cached by an earlier start: the alternate enumeration command can go out before the descriptors */
    Status = HubRegReadValue(Key, L"MsOs20DescriptorSetInfo", sizeof(SetInfo), &SetInfo);
    if (NT_SUCCESS(Status))
    {
        Child->m_AltEnumCached = TRUE;
        Child->m_MsOs20.Flags |= HUB_MSOS20_ALT_ENUM;
        Child->m_MsOsVendorCode = SetInfo.bVendorCode;
        Child->m_MsOs20SetInfo = SetInfo;
    }
    else if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
    {
        goto Done;
    }

    if (HubIsBrokenVl810(&Child->m_DeviceDescriptor))
        Child->m_Hacks |= (ULONG)ChildHack::NonFunctional;

    HubReadAlternateSettingFilter(Child, Key);
    Status = STATUS_SUCCESS;

Done:
    WdfRegistryClose(Key);
    WdfRegistryClose(Root);
    return Status;
}

/*
 * HardwareVerifier\<id>\<version>\<value>, trying VID+PID+REV, VID+PID and
 * global in turn. Only a missing key or value moves on to the next try.
 */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubQueryVerifierFlags(
    _In_ const USB_DEVICE_DESCRIPTOR* Descriptor,
    _In_ const HubIdStrings* Ids,
    _In_ PCWSTR ValueName,
    _Out_ PULONG Flags)
{
    PCWSTR Version;
    WCHAR Buffer[48];
    UNICODE_STRING Path;
    WDFKEY Root;
    WDFKEY Key;
    ULONG Try;
    NTSTATUS Status;

    *Flags = 0;

    /* bcdUSB 0 is treated as USB 3 */
    if (Descriptor->bcdUSB == 0 || Descriptor->bcdUSB >= 0x0300)
        Version = L"usb30";
    else if (Descriptor->bcdUSB <= 0x0200)
        Version = L"usbUpto20";
    else
        Version = L"usb2X";

    Status = HubRegOpenPath(HubVerifierPath, KEY_READ, &Root);
    if (!NT_SUCCESS(Status))
    {
        if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
            DPRINT1("Opening the hardware verifier key failed 0x%lx\n", Status);

        return Status;
    }

    for (Try = 0; Try < 3; Try++)
    {
        RtlInitEmptyUnicodeString(&Path, Buffer, sizeof(Buffer));

        if (Try == 0)
            Status = RtlUnicodeStringPrintf(&Path, L"%hs%hs%hs\\%ls", Ids->Vendor, Ids->Product, Ids->Revision, Version);
        else if (Try == 1)
            Status = RtlUnicodeStringPrintf(&Path, L"%hs%hs\\%ls", Ids->Vendor, Ids->Product, Version);
        else
            Status = RtlUnicodeStringPrintf(&Path, L"global\\%ls", Version);

        if (!NT_SUCCESS(Status))
            break;

        Status = HubRegOpen(Root, &Path, KEY_READ, &Key);
        if (NT_SUCCESS(Status))
        {
            Status = HubRegReadValue(Key, ValueName, sizeof(*Flags), Flags);
            WdfRegistryClose(Key);
        }

        if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
            break;

        *Flags = 0;
    }

    if (!NT_SUCCESS(Status))
    {
        if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
            DPRINT1("Hardware verifier %S flags unreadable 0x%lx\n", ValueName, Status);

        *Flags = 0;
    }

    WdfRegistryClose(Root);
    return Status;
}

BOOLEAN
DeviceMachine::QueryRegistryValues()
{
    HubIdStrings Ids;
    NTSTATUS Status;

    HubFormatIdStrings(&m_Device->m_DeviceDescriptor, &Ids);

    Status = HubQueryDeviceErrata(m_Device, &Ids);
    HubQueryVerifierFlags(&m_Device->m_DeviceDescriptor, &Ids, L"device", &m_Device->m_VerifierFlags);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Device %p usbflags query failed 0x%lx\n", m_Device, Status);
        m_Device->m_EnumMessageId = HUB_MSG_REGISTRY_FAILURE;
        return FALSE;
    }

    return TRUE;
}

/* Hub errata */

/* With a value name, a present value decides alone; the shim bit counts when it is absent */
struct HubHubErrata
{
    PCWSTR Value;
    UCHAR KseBit;
    HubFlag Set;
    ULONG Clear;
};

static const HubHubErrata HubHubErrataFirst[] =
{
    { L"ResetTTOnCancel", 8, HubFlag::ResetTtOnCancel, 0 },
    { L"NoClearTTBufferOnCancel", 9, HubFlag::NoClearTtOnCancel, (ULONG)HubFlag::ResetTtOnCancel },
    { NULL, 11, HubFlag::PowerOnPortsOnStart, 0 },
    { L"DisableLPM", 12, HubFlag::DisableLpm, 0 },
    { NULL, 13, HubFlag::DisableUsb20Lpm, 0 },
    { NULL, 15, HubFlag::DelayAfterResetComplete, 0 },
    { NULL, 18, HubFlag::ToleratesU0WhileDetached, 0 },
    { NULL, 20, HubFlag::IgnoreEnabledInSsInactive, 0 },
    { NULL, 22, HubFlag::DisableSuperSpeed, 0 },
    { NULL, 25, HubFlag::DiscardEnableDuringReset, 0 }
};

static const HubHubErrata HubHubErrataSecond[] =
{
    { NULL, 28, HubFlag::NoSelectiveSuspendIntegrated, 0 },
    { NULL, 29, HubFlag::DisallowU2AcceptOnly, 0 }
};

_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubApplyHubErrata(
    _In_ HubFdo* Hub,
    _In_ WDFKEY Key,
    _In_ ULONG64 Kse,
    _In_reads_(Count) const HubHubErrata* Table,
    _In_ ULONG Count)
{
    ULONG Index;
    ULONG Value;
    BOOLEAN Apply;
    NTSTATUS Status;

    for (Index = 0; Index < Count; Index++)
    {
        Apply = HubKseBit(Kse, Table[Index].KseBit);

        if (Table[Index].Value != NULL)
        {
            Status = HubRegReadValue(Key, Table[Index].Value, sizeof(Value), &Value);
            if (NT_SUCCESS(Status))
                Apply = (Value != 0);
            else if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
                return Status;
        }

        if (!Apply)
            continue;

        if (Table[Index].Clear != 0)
            InterlockedAnd(&Hub->m_Flags, ~(LONG)Table[Index].Clear);
        Hub->SetFlag(Table[Index].Set);
    }

    return STATUS_SUCCESS;
}

/*
 * A read error other than a missing value ends the query at once, skipping
 * the DisableOnSoftRemove default and the report.
 */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubQueryHubErrata(
    _In_ HubFdo* Hub,
    _In_ const HubIdStrings* Ids)
{
    WDFKEY Root;
    WDFKEY Key;
    ULONG64 Kse;
    NTSTATUS Status;

    Status = HubOpenUsbflagsKeys(Ids, KEY_READ, &Root, &Key);
    if (!NT_SUCCESS(Status))
        return Status;

    Kse = HubQueryKseDeviceFlags(Ids, Hub->IsRootHub());

    Status = HubApplyHubErrata(Hub, Key, Kse, HubHubErrataFirst, RTL_NUMBER_OF(HubHubErrataFirst));
    if (!NT_SUCCESS(Status))
        goto Done;

    /* Either bit turns LPM off for the whole subtree below this hub */
    if (HubKseBit(Kse, KSE_LPM_OFF_DOWNSTREAM))
    {
        InterlockedOr((volatile LONG*)&Hub->m_ParentInfo.Flags, HUB_PARENT_DISABLE_LPM);
        Hub->SetFlag(HubFlag::DisableLpm);
    }

    if (Hub->IsRootHub() && HubKseBit(Kse, KSE_LPM_OFF_FOR_CHAINED_HUBS))
        Hub->m_ParentInfo.Flags |= HUB_PARENT_DISABLE_LPM;

    HubApplyHubErrata(Hub, Key, Kse, HubHubErrataSecond, RTL_NUMBER_OF(HubHubErrataSecond));

    Hub->SetFlag(HubFlag::DisableOnSoftRemove);
    if (HubKseBit(Kse, KSE_KEEP_ON_SOFT_REMOVE))
        Hub->ClearFlag(HubFlag::DisableOnSoftRemove);

    if (HubKseBit(Kse, KSE_POLLING_TO_COMPLIANCE))
        Hub->SetFlag(HubFlag::PollingToComplianceOnStart);

    if (HubKseBit(Kse, KSE_NO_U2_FOR_TUNNELED_DEVICES))
        Hub->m_TunneledDevicesSkipU2 = TRUE;

    if (HubIsBrokenVl810(&Hub->m_ParentInfo.DeviceDescriptor))
        Hub->SetFlag(HubFlag::DisableSuperSpeed);

    if (Hub->HasFlag(HubFlag::DisableSuperSpeed))
        HubSubmitLiveDump(Hub->m_Device, HUB_LIVEDUMP_SUPERSPEED_DISABLED, 0);

Done:
    WdfRegistryClose(Key);
    WdfRegistryClose(Root);
    return Status;
}

/* The hub machine ignores the outcome */
VOID
HubMachine::QueryErrataFlags()
{
    HubIdStrings Ids;
    NTSTATUS Status;

    if (m_Hub->IsRootHub())
        HubFormatControllerIds(&m_Hub->m_ControllerInfo, &Ids);
    else
        HubFormatIdStrings(&m_Hub->m_ParentInfo.DeviceDescriptor, &Ids);

    HubQueryVerifierFlags(&m_Hub->m_ParentInfo.DeviceDescriptor, &Ids, L"hub", &m_Hub->m_VerifierFlags);

    Status = HubQueryHubErrata(m_Hub, &Ids);
    if (!NT_SUCCESS(Status))
        DPRINT1("Hub %p usbflags query failed 0x%lx\n", m_Hub, Status);
}

/* Hub hardware key */

/* Missing values are skipped; any other read error ends the reads */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubReadHubRegistryValues(
    _In_ HubFdo* Hub)
{
    WDFKEY Key;
    ULONG Value;
    BOOLEAN Present;
    NTSTATUS Status;

    PAGED_CODE();

    Status = HubOpenHubKey(Hub, KEY_READ, &Key);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p hardware key could not be opened 0x%lx\n", Hub, Status);
        return;
    }

    if (!HubRegWalkDword(Key, L"WakeSystemOnConnect", &Value, &Present, &Status))
        goto Done;
    if (Present && Value != 0)
        Hub->SetFlag(HubFlag::WakeOnConnect);

    /* Lifetime count survives devnode restarts; it is a 16 bit count */
    if (!HubRegWalkDword(Key, L"HardResetCount", &Value, &Present, &Status))
        goto Done;
    if (Present)
        Hub->m_LifetimeRecoveryCount = (USHORT)Value;

    if (!HubRegWalkDword(Key, L"OvercurrentDetected", &Value, &Present, &Status))
        goto Done;
    if (Present)
    {
        if (Value & 1)
            Hub->SetFlag(HubFlag::OverCurrentDetected);
        else
            Hub->ClearFlag(HubFlag::OverCurrentDetected);
    }

    /* 1 asks for the USB4 router DROM read; a missing value leaves the last one */
    if (!HubRegWalkDword(Key, L"HubFWUpdateProtocol", &Value, &Present, &Status))
        goto Done;
    if (Present)
        Hub->m_FwUpdateProtocol = Value;

Done:
    if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_NOT_FOUND)
        DPRINT1("Hub %p hardware key read failed 0x%lx\n", Hub, Status);

    WdfRegistryClose(Key);
}

/**
 * @brief
 * Usb4HostName overrides the _DSD USB4 host router name of every USB 3 ACPI
 * port. A string from an earlier start is kept since ports still use it.
 */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubReadUsb4HostName(
    _In_ HubFdo* Hub)
{
    DECLARE_CONST_UNICODE_STRING(ValueName, L"Usb4HostName");
    WDF_OBJECT_ATTRIBUTES Attributes;
    WDFSTRING Name;
    WDFKEY Key;
    NTSTATUS Status;

    PAGED_CODE();

    Status = HubOpenHubKey(Hub, KEY_READ, &Key);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p hardware key not opened for Usb4HostName 0x%lx\n", Hub, Status);
        return;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Hub->m_Device;

    Status = WdfStringCreate(NULL, &Attributes, &Name);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p Usb4HostName string not created 0x%lx\n", Hub, Status);
        goto Done;
    }

    Status = WdfRegistryQueryString(Key, &ValueName, Name);
    if (!NT_SUCCESS(Status))
    {
        if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
            DPRINT1("Hub %p Usb4HostName read failed 0x%lx\n", Hub, Status);
        WdfObjectDelete(Name);
        goto Done;
    }

    Hub->m_Usb4HostName = Name;
    DPRINT("Hub %p USB4 host name override present\n", Hub);

Done:
    WdfRegistryClose(Key);
}

static
VOID
NTAPI
HubEvtOverCurrentWorkItem(
    _In_ WDFWORKITEM WorkItem)
{
    DECLARE_CONST_UNICODE_STRING(Name, L"OvercurrentDetected");
    HubFdo* Hub = HubFdo::FromDevice((WDFDEVICE)WdfWorkItemGetParentObject(WorkItem));
    ULONG Value;
    WDFKEY Key;
    NTSTATUS Status;

    Value = Hub->HasFlag(HubFlag::OverCurrentDetected) ? 1 : 0;

    Status = HubOpenHubKey(Hub, KEY_WRITE, &Key);
    if (NT_SUCCESS(Status))
    {
        Status = WdfRegistryAssignValue(Key, &Name, REG_DWORD, sizeof(Value), &Value);
        WdfRegistryClose(Key);
    }

    if (!NT_SUCCESS(Status))
        DPRINT1("Hub %p over current record not written 0x%lx\n", Hub, Status);

    WdfObjectDelete(WorkItem);
}

/*
 * The flag is reloaded at every start and never cleared, so the value is
 * written once per devnode. A failed work item is not retried.
 */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
HubNoteHubOverCurrent(
    _In_ HubFdo* Hub)
{
    WDF_WORKITEM_CONFIG Config;
    WDF_OBJECT_ATTRIBUTES Attributes;
    WDFWORKITEM WorkItem;
    NTSTATUS Status;

    if (InterlockedOr(&Hub->m_Flags, (LONG)HubFlag::OverCurrentDetected) & (LONG)HubFlag::OverCurrentDetected)
        return;

    WDF_WORKITEM_CONFIG_INIT(&Config, HubEvtOverCurrentWorkItem);
    Config.AutomaticSerialization = TRUE;

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Hub->m_Device;

    Status = WdfWorkItemCreate(&Config, &Attributes, &WorkItem);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Over current work item failed 0x%lx\n", Status);
        return;
    }

    WdfWorkItemEnqueue(WorkItem);
}

/* usbflags writes */

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubWriteUsbflagsValue(
    _In_ HubChild* Child,
    _In_ PCWSTR Name,
    _In_reads_bytes_(Size) PVOID Data,
    _In_ ULONG Size)
{
    UNICODE_STRING ValueName;
    HubIdStrings Ids;
    WDFKEY Root;
    WDFKEY Key;
    NTSTATUS Status;

    HubFormatIdStrings(&Child->m_DeviceDescriptor, &Ids);

    Status = HubOpenUsbflagsKeys(&Ids, KEY_WRITE, &Root, &Key);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Device %p usbflags value %S: key not opened 0x%lx\n", Child, Name, Status);
        return Status;
    }

    RtlInitUnicodeString(&ValueName, Name);
    Status = WdfRegistryAssignValue(Key, &ValueName, REG_BINARY, Size, Data);
    if (!NT_SUCCESS(Status))
        DPRINT1("Device %p usbflags value %S not written 0x%lx\n", Child, Name, Status);

    WdfRegistryClose(Key);
    WdfRegistryClose(Root);
    return Status;
}

/* Read back as row MsOs20DescriptorSetInfo of the errata query on the next enumeration */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubCacheMsOs20SetInfo(
    _In_ HubChild* Child)
{
    if (!(Child->m_MsOs20.Flags & HUB_MSOS20_ALT_ENUM) || Child->m_AltEnumCached)
        return;

    HubWriteUsbflagsValue(Child, L"MsOs20DescriptorSetInfo", &Child->m_MsOs20SetInfo, sizeof(Child->m_MsOs20SetInfo));
    Child->m_AltEnumCached = TRUE;
}

/* Device hardware key */

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubWriteDeviceValue(
    _In_ HubChild* Child,
    _In_ PCUNICODE_STRING Name,
    _In_ ULONG Type,
    _In_ ULONG Length,
    _In_reads_bytes_(Length) PVOID Data)
{
    WDFKEY Key;
    NTSTATUS Status;

    Status = HubOpenDeviceKey(Child, KEY_WRITE, &Key);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Device %p value %wZ: hardware key not opened 0x%lx\n", Child, Name, Status);
        return Status;
    }

    Status = WdfRegistryAssignValue(Key, Name, Type, Length, Data);
    if (!NT_SUCCESS(Status))
        DPRINT1("Device %p value %wZ not written 0x%lx\n", Child, Name, Status);

    WdfRegistryClose(Key);
    return Status;
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubWriteDeviceString(
    _In_ HubChild* Child,
    _In_ PCUNICODE_STRING Name,
    _In_ WDFSTRING Value)
{
    WDFKEY Key;
    NTSTATUS Status;

    Status = HubOpenDeviceKey(Child, KEY_WRITE, &Key);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Device %p string %wZ: hardware key not opened 0x%lx\n", Child, Name, Status);
        return Status;
    }

    Status = WdfRegistryAssignString(Key, Name, Value);
    if (!NT_SUCCESS(Status))
        DPRINT1("Device %p string %wZ not written 0x%lx\n", Child, Name, Status);

    WdfRegistryClose(Key);
    return Status;
}

_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubWriteDeviceDword(
    _In_ HubChild* Child,
    _In_ PCWSTR Name,
    _In_ ULONG Value)
{
    UNICODE_STRING ValueName;

    RtlInitUnicodeString(&ValueName, Name);
    HubWriteDeviceValue(Child, &ValueName, REG_DWORD, sizeof(Value), &Value);
}

/* Presence counts, not the value */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubRefreshExtPropertiesInstalled(
    _In_ HubChild* Child)
{
    WDFKEY Key;
    ULONG Value;

    if (!NT_SUCCESS(HubOpenDeviceKey(Child, KEY_READ, &Key)))
        return;

    Child->ClearProperty(ChildProperty::ExtPropertiesInstalled);

    if (NT_SUCCESS(HubRegReadValue(Key, L"ExtPropDescSemaphore", sizeof(Value), &Value)))
        Child->SetProperty(ChildProperty::ExtPropertiesInstalled);

    WdfRegistryClose(Key);
}

/* Counted copy without a NUL */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubReadFriendlyName(
    _In_ HubChild* Child,
    _In_ WDFKEY Key,
    _In_ WDFSTRING String,
    _Out_ PBOOLEAN Stop)
{
    DECLARE_CONST_UNICODE_STRING(Name, L"FriendlyName");
    UNICODE_STRING Text;
    PWCHAR Copy;
    NTSTATUS Status;

    *Stop = FALSE;

    Status = WdfRegistryQueryString(Key, &Name, String);
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
        return Status;

    if (!NT_SUCCESS(Status))
    {
        *Stop = TRUE;
        return Status;
    }

    WdfStringGetUnicodeString(String, &Text);

    if (Child->m_FriendlyName != NULL)
    {
        ExFreePoolWithTag(Child->m_FriendlyName, HUB_TAG_DEVICE);
        Child->m_FriendlyName = NULL;
        Child->m_FriendlyNameLength = 0;
    }

    if (Text.Length == 0)
        return STATUS_SUCCESS;

    Copy = (PWCHAR)ExAllocatePoolWithTag(NonPagedPool, Text.Length, HUB_TAG_DEVICE);
    if (Copy == NULL)
    {
        /* The remaining reads are skipped, yet the caller sees success */
        DPRINT1("Device %p friendly name could not be allocated\n", Child);
        *Stop = TRUE;
        return STATUS_SUCCESS;
    }

    RtlCopyMemory(Copy, Text.Buffer, Text.Length);
    Child->m_FriendlyName = Copy;
    Child->m_FriendlyNameLength = Text.Length;

    return STATUS_SUCCESS;
}

/*
 * A read error other than a missing value ends the reads, which also skips
 * the D3cold reconnect default, so the port keeps its previous timeout.
 */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubReadDeviceHardwareValues(
    _In_ HubChild* Child)
{
    UNICODE_STRING SubkeyName;
    WDFSTRING String = NULL;
    WDFKEY Key;
    WDFKEY Subkey = NULL;
    ULONG Value;
    BOOLEAN Present;
    BOOLEAN Stop;
    NTSTATUS Status;

    PAGED_CODE();

    Status = HubOpenDeviceKey(Child, KEY_READ, &Key);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Device %p hardware key could not be opened 0x%lx\n", Child, Status);
        return Status;
    }

    if (!HubRegWalkDword(Key, L"DeviceSelectiveSuspended", &Value, &Present, &Status))
        goto Done;
    if (Present && Value != 0)
        Child->SetProperty(ChildProperty::SupportsSelectiveSuspend);

    Status = WdfStringCreate(NULL, WDF_NO_OBJECT_ATTRIBUTES, &String);
    if (!NT_SUCCESS(Status))
        goto Done;

    Status = HubReadFriendlyName(Child, Key, String, &Stop);
    if (Stop)
        goto Done;

    RtlInitUnicodeString(&SubkeyName, HubD3ColdSubkey);
    Status = HubRegOpen(Key, &SubkeyName, KEY_READ, &Subkey);
    if (!NT_SUCCESS(Status))
        Subkey = NULL;

    if (NT_SUCCESS(Status))
    {
        if (!HubRegWalkDword(Subkey, L"D3ColdSupported", &Value, &Present, &Status))
            goto Done;
        if (Present && Value != 0)
            Child->SetState(ChildState::D3ColdEnabledByDriver);
    }
    else if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
    {
        goto Done;
    }

    if (!HubRegWalkDword(Key, L"AllowIdleIrpInD3", &Value, &Present, &Status))
        goto Done;
    if (Present && Value != 0)
        Child->SetProperty(ChildProperty::AllowIdleIrpInD3);

    /* No range check; 0 is accepted */
    Child->m_Port->m_D3ColdReconnectTimeout = HUB_D3COLD_RECONNECT_DEFAULT;

    if (!HubRegWalkDword(Key, L"D3ColdReconnectTimeout", &Value, &Present, &Status))
        goto Done;
    if (Present)
        Child->m_Port->m_D3ColdReconnectTimeout = Value;

    Status = STATUS_SUCCESS;

Done:
    if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_NOT_FOUND)
        DPRINT1("Device %p hardware key read failed 0x%lx\n", Child, Status);

    if (Subkey != NULL)
        WdfRegistryClose(Subkey);
    WdfRegistryClose(Key);
    if (String != NULL)
        WdfObjectDelete(String);

    return Status;
}

/* Enumeration data written at PDO start; failures are ignored */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubWriteEnumerationData(
    _In_ HubChild* Child)
{
    PAGED_CODE();

    if (Child->m_MsOs20.Flags & HUB_MSOS20_SET_INFO)
        HubWriteDeviceDword(Child, L"MsOs20Flags", Child->m_MsOs20.Flags);

    HubWriteDeviceDword(Child, L"EnumerationRetryCount", Child->m_EnumRetryCount);
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubWriteEnumerationFailureCode(
    _In_ HubChild* Child)
{
    PAGED_CODE();

    HubWriteDeviceDword(Child, L"EnumerationFailureCode", Child->m_EnumMessageId);
}

/* UXD (VM redirection) settings */

/* uxd_port_NNN, at least three digits */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubFormatUxdPortName(
    _In_ USHORT PortNumber,
    _Out_writes_(Count) PWCHAR Buffer,
    _In_ ULONG Count,
    _Out_ PUNICODE_STRING Name)
{
    RtlInitEmptyUnicodeString(Name, Buffer, (USHORT)(Count * sizeof(WCHAR)));
    return RtlUnicodeStringPrintf(Name, L"uxd_port_%03u", PortNumber);
}

_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubFormatUxdDeviceName(
    _In_ HubChild* Child,
    _Out_writes_(Count) PWCHAR Buffer,
    _In_ ULONG Count,
    _Out_ PUNICODE_STRING Name)
{
    const USB_DEVICE_DESCRIPTOR* Descriptor = &Child->m_DeviceDescriptor;

    RtlInitEmptyUnicodeString(Name, Buffer, (USHORT)(Count * sizeof(WCHAR)));
    return RtlUnicodeStringPrintf(Name,
                                  L"%04X%04X%04X",
                                  Descriptor->idVendor,
                                  Descriptor->idProduct,
                                  Descriptor->bcdDevice);
}

/* Shorter data leaves the tail zero; longer data fails */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubReadUxdRecord(
    _In_ WDFKEY Key,
    _In_ PCUNICODE_STRING Name,
    _Out_ HubUxdSettings* Record)
{
    RtlZeroMemory(Record, sizeof(*Record));
    return WdfRegistryQueryValue(Key, Name, sizeof(*Record), Record, NULL, NULL);
}

/*
 * Creating the uxd subkey reports success with an all zero record, which
 * reserves the device with a zero PnP GUID.
 */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubReadUxdPortRecord(
    _In_ HubFdo* Hub,
    _In_ USHORT PortNumber,
    _Out_ HubUxdSettings* Record)
{
    DECLARE_CONST_UNICODE_STRING(UxdName, L"uxd");
    WCHAR Buffer[24];
    UNICODE_STRING Name;
    WDFKEY HubKey;
    WDFKEY UxdKey;
    NTSTATUS Status;

    RtlZeroMemory(Record, sizeof(*Record));

    Status = HubOpenHubKey(Hub, KEY_READ | KEY_WRITE, &HubKey);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = HubRegOpen(HubKey, &UxdName, KEY_READ, &UxdKey);
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        Status = WdfRegistryCreateKey(HubKey, &UxdName, KEY_READ, REG_OPTION_NON_VOLATILE, NULL, WDF_NO_OBJECT_ATTRIBUTES, &UxdKey);
        if (NT_SUCCESS(Status))
            WdfRegistryClose(UxdKey);

        WdfRegistryClose(HubKey);
        return Status;
    }

    if (NT_SUCCESS(Status))
    {
        Status = HubFormatUxdPortName(PortNumber, Buffer, RTL_NUMBER_OF(Buffer), &Name);
        if (NT_SUCCESS(Status))
            Status = HubReadUxdRecord(UxdKey, &Name, Record);

        WdfRegistryClose(UxdKey);
    }

    WdfRegistryClose(HubKey);
    return Status;
}

_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubReadUxdDeviceRecord(
    _In_ HubChild* Child,
    _Out_ HubUxdSettings* Record)
{
    WCHAR Buffer[16];
    UNICODE_STRING Name;
    WDFKEY Key;
    NTSTATUS Status;

    RtlZeroMemory(Record, sizeof(*Record));

    Status = HubRegOpenPath(HubUxdDevicesPath, KEY_READ, &Key);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = HubFormatUxdDeviceName(Child, Buffer, RTL_NUMBER_OF(Buffer), &Name);
    if (NT_SUCCESS(Status))
        Status = HubReadUxdRecord(Key, &Name, Record);

    WdfRegistryClose(Key);
    return Status;
}

/*
 * The port record wins over the VID/PID/REV record. Without the policy key
 * the reserved flag is dropped but the old record stays.
 */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubSaveUxdState(
    _In_ HubFdo* Hub,
    _In_ HubChild* Child,
    _Out_opt_ PBOOLEAN UpdateRequired)
{
    HubUxdSettings Previous;
    HubUxdSettings Current;
    BOOLEAN Found;

    PAGED_CODE();

    if (UpdateRequired != NULL)
        *UpdateRequired = FALSE;

    Child->ClearProperty(ChildProperty::UxdReserved);
    Previous = Child->m_Uxd;

    if (!NT_SUCCESS(HubReadUxdPolicy()))
        return STATUS_SUCCESS;

    if (!HubDriver.HasFlag(HubGlobal::UxdEnable) || HubDriver.HasFlag(HubGlobal::DisableUxdSupport))
        return STATUS_SUCCESS;

    Found = NT_SUCCESS(HubReadUxdPortRecord(Hub, Child->m_Port->Number(), &Current));
    if (!Found)
        Found = NT_SUCCESS(HubReadUxdDeviceRecord(Child, &Current));

    if (Found)
    {
        DPRINT("Device %p on port %u is reserved for a virtual machine\n", Child, Child->m_Port->Number());
        Child->SetProperty(ChildProperty::UxdReserved);
        InterlockedOr(&Child->m_SqmFlags, (LONG)ChildSqm::Virtualized);
    }
    else
    {
        RtlZeroMemory(&Current, sizeof(Current));
    }

    Child->m_Uxd = Current;

    if (UpdateRequired != NULL &&
        RtlCompareMemory(&Current, &Previous, sizeof(Current)) != sizeof(Current) &&
        !Child->HasProperty(ChildProperty::IsHub))
    {
        *UpdateRequired = TRUE;
    }

    return STATUS_SUCCESS;
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubPurgeUxdState(
    _In_ HubChild* Child,
    _In_ HubUxdEvent Event)
{
    DECLARE_CONST_UNICODE_STRING(UxdName, L"uxd");
    WCHAR Buffer[24];
    UNICODE_STRING Name;
    WDFKEY HubKey;
    WDFKEY Key;
    BOOLEAN Delete;
    NTSTATUS Status;

    PAGED_CODE();

    HubReadUxdPolicy();

    /* DeleteOnReload must be exactly 1; DeleteOnDisconnect any nonzero value */
    if (Event == HubUxdEvent::Disable)
        Delete = Child->m_Uxd.DeleteOnReload == 1 || HubDriver.HasFlag(HubGlobal::UxdDeleteOnReload);
    else if (Event == HubUxdEvent::Disconnect)
        Delete = Child->m_Uxd.DeleteOnDisconnect != 0 || HubDriver.HasFlag(HubGlobal::UxdDeleteOnDisconnect);
    else
        Delete = FALSE;

    if (!Delete)
        return STATUS_SUCCESS;

    DPRINT("Device %p UXD records deleted on event %d\n", Child, (int)Event);

    if (NT_SUCCESS(HubOpenHubKey(Child->m_Hub, KEY_ALL_ACCESS, &HubKey)))
    {
        Status = HubRegOpen(HubKey, &UxdName, KEY_ALL_ACCESS, &Key);
        if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            if (NT_SUCCESS(WdfRegistryCreateKey(HubKey, &UxdName, KEY_ALL_ACCESS, REG_OPTION_NON_VOLATILE, NULL, WDF_NO_OBJECT_ATTRIBUTES, &Key)))
                WdfRegistryClose(Key);
        }
        else if (NT_SUCCESS(Status))
        {
            if (NT_SUCCESS(HubFormatUxdPortName(Child->m_Port->Number(), Buffer, RTL_NUMBER_OF(Buffer), &Name)))
                WdfRegistryRemoveValue(Key, &Name);
            WdfRegistryClose(Key);
        }

        WdfRegistryClose(HubKey);
    }

    if (NT_SUCCESS(HubRegOpenPath(HubUxdDevicesPath, KEY_READ | KEY_SET_VALUE, &Key)))
    {
        if (NT_SUCCESS(HubFormatUxdDeviceName(Child, Buffer, RTL_NUMBER_OF(Buffer), &Name)))
            WdfRegistryRemoveValue(Key, &Name);
        WdfRegistryClose(Key);
    }

    return STATUS_SUCCESS;
}

/* A missing key or value leaves String empty and still succeeds */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubQueryUxdDeviceValue(
    _In_ HubChild* Child,
    _In_ WDFSTRING String)
{
    UNICODE_STRING GuidText;
    WDFKEY Key;
    NTSTATUS Status;

    PAGED_CODE();

    Status = RtlStringFromGUID(Child->m_Uxd.PnpGuid, &GuidText);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("UXD GUID conversion failed 0x%lx\n", Status);
        return STATUS_SUCCESS;
    }

    Status = HubRegOpenPath(HubUxdPnpPath, KEY_READ, &Key);
    if (NT_SUCCESS(Status))
    {
        Status = WdfRegistryQueryString(Key, &GuidText, String);
        if (!NT_SUCCESS(Status))
            DPRINT1("UXD PnP value missing 0x%lx\n", Status);
        WdfRegistryClose(Key);
    }

    RtlFreeUnicodeString(&GuidText);
    return STATUS_SUCCESS;
}

/*
 * Deletes every record marked for deletion at shutdown. Names are cut to 12
 * characters and a name over 15 ends the pass.
 */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubPurgeUxdRecords(
    _In_ WDFKEY Key)
{
    union
    {
        KEY_VALUE_BASIC_INFORMATION Info;
        UCHAR Raw[FIELD_OFFSET(KEY_VALUE_BASIC_INFORMATION, Name) + 15 * sizeof(WCHAR)];
    } Entry;
    HANDLE Handle = WdfRegistryWdmGetHandle(Key);
    HubUxdSettings Record;
    UNICODE_STRING Name;
    ULONG Index = 0;
    ULONG Returned;

    for (;;)
    {
        RtlZeroMemory(&Entry, sizeof(Entry));

        if (!NT_SUCCESS(ZwEnumerateValueKey(Handle, Index, KeyValueBasicInformation, &Entry, sizeof(Entry), &Returned)))
            break;

        Index++;

        if (Entry.Info.Type != REG_BINARY)
            continue;

        Name.Buffer = Entry.Info.Name;
        Name.Length = (USHORT)min(Entry.Info.NameLength, 12 * sizeof(WCHAR));
        Name.MaximumLength = Name.Length;

        if (!NT_SUCCESS(HubReadUxdRecord(Key, &Name, &Record)))
            break;

        if (Record.DeleteOnShutdown != 1 && !HubDriver.HasFlag(HubGlobal::UxdDeleteOnShutdown))
            continue;

        if (!NT_SUCCESS(WdfRegistryRemoveValue(Key, &Name)))
        {
            DPRINT1("UXD record %wZ could not be deleted at shutdown\n", &Name);
            break;
        }

        /* The next value moved into this slot */
        Index--;
    }
}

/* Each hub repeats the global pass; later passes find nothing left */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubUxdShutdownCleanup(
    _In_ HubFdo* Hub)
{
    DECLARE_CONST_UNICODE_STRING(UxdName, L"uxd");
    WDFKEY HubKey;
    WDFKEY Key;
    NTSTATUS Status;

    HubReadUxdPolicy();

    if (NT_SUCCESS(HubRegOpenPath(HubUxdDevicesPath, KEY_ALL_ACCESS, &Key)))
    {
        HubPurgeUxdRecords(Key);
        WdfRegistryClose(Key);
    }

    if (!NT_SUCCESS(HubOpenHubKey(Hub, KEY_ALL_ACCESS, &HubKey)))
        return;

    Status = HubRegOpen(HubKey, &UxdName, KEY_ALL_ACCESS, &Key);
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        if (NT_SUCCESS(WdfRegistryCreateKey(HubKey, &UxdName, KEY_ALL_ACCESS, REG_OPTION_NON_VOLATILE, NULL, WDF_NO_OBJECT_ATTRIBUTES, &Key)))
            WdfRegistryClose(Key);
    }
    else if (NT_SUCCESS(Status))
    {
        HubPurgeUxdRecords(Key);
        WdfRegistryClose(Key);
    }

    WdfRegistryClose(HubKey);
}

/* Telemetry */

/* DeviceInformation bits are only ever added. Validation words 0 to 6 are written; word 7 has no value name. */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubFlushSqmFlags(
    _In_ HubChild* Child)
{
    DECLARE_CONST_UNICODE_STRING(CeipName, L"Ceip");
    DECLARE_CONST_UNICODE_STRING(InformationName, L"DeviceInformation");
    DECLARE_CONST_UNICODE_STRING(InterconnectName, L"PortInterconnectType");
    WCHAR Buffer[32];
    UNICODE_STRING Name;
    WDFKEY Key;
    WDFKEY Ceip;
    ULONG Previous;
    ULONG Bits;
    ULONG Index;
    NTSTATUS Status;

    PAGED_CODE();

    Status = HubOpenDeviceKey(Child, KEY_READ | KEY_WRITE, &Key);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Device %p telemetry: hardware key not opened 0x%lx\n", Child, Status);
        return;
    }

    Status = WdfRegistryCreateKey(Key, &CeipName, KEY_READ | KEY_WRITE, REG_OPTION_NON_VOLATILE, NULL, WDF_NO_OBJECT_ATTRIBUTES, &Ceip);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Device %p telemetry: Ceip key not created 0x%lx\n", Child, Status);
        goto CloseKey;
    }

    Status = WdfRegistryQueryULong(Ceip, &InformationName, &Previous);
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
        Previous = 0;
    else if (!NT_SUCCESS(Status))
        goto CloseCeip;

    Status = WdfRegistryAssignULong(Ceip,
                                    &InformationName,
                                    (ULONG)Child->m_SqmFlags | (ULONG)ChildSqm::IsOnXhci | Previous);
    if (!NT_SUCCESS(Status))
        goto CloseCeip;

    Status = WdfRegistryAssignULong(Ceip, &InterconnectName, Child->m_Port->m_InterconnectType);
    if (!NT_SUCCESS(Status))
        goto CloseCeip;

    Bits = 0;
    for (Index = 0; Index < RTL_NUMBER_OF(Child->m_ValidationBits); Index++)
        Bits |= Child->m_ValidationBits[Index];

    if (Bits == 0)
        goto CloseCeip;

    for (Index = 0; Index < 7; Index++)
    {
        RtlInitEmptyUnicodeString(&Name, Buffer, sizeof(Buffer));
        if (!NT_SUCCESS(RtlUnicodeStringPrintf(&Name, L"DescriptorValidationInfo%lu", Index)))
            break;

        if (!NT_SUCCESS(WdfRegistryAssignValue(Ceip,
                                               &Name,
                                               REG_DWORD,
                                               sizeof(Child->m_ValidationBits[Index]),
                                               &Child->m_ValidationBits[Index])))
        {
            DPRINT1("Device %p telemetry: %wZ not written\n", Child, &Name);
            break;
        }
    }

CloseCeip:
    if (!NT_SUCCESS(Status))
        DPRINT1("Device %p telemetry not written 0x%lx\n", Child, Status);

    WdfRegistryClose(Ceip);
CloseKey:
    WdfRegistryClose(Key);
}

static
VOID
NTAPI
HubEvtSelectiveSuspendWorkItem(
    _In_ WDFWORKITEM WorkItem)
{
    HubPdo* Pdo = HubPdo::FromDevice((WDFDEVICE)WdfWorkItemGetParentObject(WorkItem));
    HubChild* Child = Pdo->m_Child;

    /* The work item is left to its parent here */
    if (Child == NULL)
    {
        DPRINT("PDO %p lost its device before the selective suspend record\n", Pdo);
        return;
    }

    HubWriteDeviceDword(Child,
                        L"DeviceSelectiveSuspended",
                        Child->HasProperty(ChildProperty::SupportsSelectiveSuspend) ? 1 : 0);

    WdfObjectDelete(WorkItem);
}

/* Once written, the device counts as selective suspend capable on every later start */
_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
NTAPI
HubRecordSelectiveSuspend(
    _In_ HubChild* Child)
{
    WDF_WORKITEM_CONFIG Config;
    WDF_OBJECT_ATTRIBUTES Attributes;
    WDFWORKITEM WorkItem;
    NTSTATUS Status;

    if (Child->HasProperty(ChildProperty::SupportsSelectiveSuspend) || Child->m_Pdo == NULL)
        return;

    Child->SetProperty(ChildProperty::SupportsSelectiveSuspend);

    WDF_WORKITEM_CONFIG_INIT(&Config, HubEvtSelectiveSuspendWorkItem);
    Config.AutomaticSerialization = TRUE;

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Child->m_Pdo->m_Device;

    Status = WdfWorkItemCreate(&Config, &Attributes, &WorkItem);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Selective suspend work item failed 0x%lx\n", Status);
        return;
    }

    WdfWorkItemEnqueue(WorkItem);
}

/* Two recoveries at once may lose an increment */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubBumpBootSurpriseRemovalCount(VOID)
{
    DECLARE_CONST_UNICODE_STRING(CountName, L"BootPathSurpriseRemovalCount");
    UNICODE_STRING Path;
    WDFKEY Key;
    ULONG Count;
    NTSTATUS Status;

    PAGED_CODE();

    RtlInitUnicodeString(&Path, HubGlobalCeipPath);
    Status = WdfRegistryCreateKey(NULL, &Path, KEY_READ | KEY_WRITE, REG_OPTION_NON_VOLATILE, NULL, WDF_NO_OBJECT_ATTRIBUTES, &Key);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Opening the USB Ceip key failed 0x%lx\n", Status);
        return;
    }

    Status = WdfRegistryQueryULong(Key, &CountName, &Count);
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        Count = 0;
        Status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(Status))
        Status = WdfRegistryAssignULong(Key, &CountName, Count + 1);

    if (!NT_SUCCESS(Status))
        DPRINT1("Updating the boot path surprise removal count failed 0x%lx\n", Status);

    WdfRegistryClose(Key);
}

/* Dual role features (plain Zw access, outside KMDF) */

_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubZwOpenKey(
    _In_ PCUNICODE_STRING Path,
    _Out_ PHANDLE Handle)
{
    OBJECT_ATTRIBUTES Attributes;

    InitializeObjectAttributes(&Attributes,
                               (PUNICODE_STRING)Path,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    return ZwOpenKey(Handle, KEY_READ, &Attributes);
}

/* Succeeds only for exactly 4 bytes of data; the type is not checked */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubZwReadULong(
    _In_ PCWSTR Path,
    _In_ PCWSTR Name,
    _Out_ PULONG Value)
{
    union
    {
        KEY_VALUE_PARTIAL_INFORMATION Info;
        UCHAR Raw[FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data) + sizeof(ULONG)];
    } Data;
    UNICODE_STRING KeyPath;
    UNICODE_STRING ValueName;
    HANDLE Handle;
    ULONG Returned;
    NTSTATUS Status;

    RtlInitUnicodeString(&KeyPath, Path);
    Status = HubZwOpenKey(&KeyPath, &Handle);
    if (!NT_SUCCESS(Status))
        return Status;

    RtlInitUnicodeString(&ValueName, Name);
    Status = ZwQueryValueKey(Handle, &ValueName, KeyValuePartialInformation, &Data, sizeof(Data), &Returned);
    ZwClose(Handle);

    if (!NT_SUCCESS(Status) || Data.Info.DataLength != sizeof(ULONG))
        return STATUS_UNSUCCESSFUL;

    RtlCopyMemory(Value, Data.Info.Data, sizeof(ULONG));
    return STATUS_SUCCESS;
}

/* Whole value data in paged pool; the caller bounds every string walk by DataLength */
_IRQL_requires_(PASSIVE_LEVEL)
static
PKEY_VALUE_PARTIAL_INFORMATION
NTAPI
HubZwReadData(
    _In_ PCUNICODE_STRING Path,
    _In_ PCWSTR Name)
{
    PKEY_VALUE_PARTIAL_INFORMATION Info = NULL;
    UNICODE_STRING ValueName;
    HANDLE Handle;
    ULONG Size = 0;
    NTSTATUS Status;

    if (!NT_SUCCESS(HubZwOpenKey(Path, &Handle)))
        return NULL;

    RtlInitUnicodeString(&ValueName, Name);

    Status = ZwQueryValueKey(Handle, &ValueName, KeyValuePartialInformation, NULL, 0, &Size);
    if ((Status == STATUS_BUFFER_TOO_SMALL || Status == STATUS_BUFFER_OVERFLOW) && Size != 0)
    {
        Info = (PKEY_VALUE_PARTIAL_INFORMATION)ExAllocatePoolWithTag(PagedPool, Size, HUB_TAG_DEVICE);
        if (Info != NULL &&
            !NT_SUCCESS(ZwQueryValueKey(Handle, &ValueName, KeyValuePartialInformation, Info, Size, &Size)))
        {
            ExFreePoolWithTag(Info, HUB_TAG_DEVICE);
            Info = NULL;
        }
    }

    ZwClose(Handle);
    return Info;
}

/* Characters of the string at Text before its NUL, never past Limit */
static
ULONG
NTAPI
HubBoundedLength(
    _In_reads_(Limit) PCWSTR Text,
    _In_ ULONG Limit)
{
    ULONG Length = 0;

    while (Length < Limit && Text[Length] != UNICODE_NULL)
        Length++;

    return Length;
}

/* Function bits from the USB function configuration InterfaceList */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubAddConfiguredFunctions(
    _Inout_ PULONG Features)
{
    static const struct
    {
        PCWSTR Name;
        ULONG Bit;
    } Functions[] =
    {
        { L"MTP", HUB_DUAL_ROLE_MTP },
        { L"IpOverUsb", HUB_DUAL_ROLE_IP_OVER_USB },
        { L"VidStream", HUB_DUAL_ROLE_VIDEO }
    };
    PKEY_VALUE_PARTIAL_INFORMATION Current;
    PKEY_VALUE_PARTIAL_INFORMATION List;
    UNICODE_STRING Path;
    UNICODE_STRING Text;
    UNICODE_STRING Entry;
    UNICODE_STRING Match;
    PWCHAR PathBuffer;
    PCWSTR Chars;
    ULONG Count;
    ULONG Position;
    ULONG Length;
    ULONG Index;
    USHORT PathSize;

    RtlInitUnicodeString(&Path, HubUsbFnDefaultPath);
    Current = HubZwReadData(&Path, L"CurrentConfiguration");
    if (Current == NULL)
        return;

    Chars = (PCWSTR)Current->Data;
    Text.Length = (USHORT)(HubBoundedLength(Chars, Current->DataLength / sizeof(WCHAR)) * sizeof(WCHAR));
    Text.MaximumLength = Text.Length;
    Text.Buffer = (PWCHAR)Chars;

    PathSize = (USHORT)(sizeof(HubUsbFnConfigPath) + Text.Length);
    PathBuffer = (PWCHAR)ExAllocatePoolWithTag(PagedPool, PathSize, HUB_TAG_DEVICE);
    if (PathBuffer == NULL)
        goto FreeCurrent;

    RtlInitEmptyUnicodeString(&Path, PathBuffer, PathSize);
    RtlAppendUnicodeToString(&Path, HubUsbFnConfigPath);
    RtlAppendUnicodeStringToString(&Path, &Text);

    List = HubZwReadData(&Path, L"InterfaceList");
    if (List == NULL)
        goto FreePath;

    Chars = (PCWSTR)List->Data;
    Count = List->DataLength / sizeof(WCHAR);
    Position = 0;

    while (Position < Count && Chars[Position] != UNICODE_NULL)
    {
        Length = HubBoundedLength(&Chars[Position], Count - Position);

        Entry.Buffer = (PWCHAR)&Chars[Position];
        Entry.Length = (USHORT)(Length * sizeof(WCHAR));
        Entry.MaximumLength = Entry.Length;

        for (Index = 0; Index < RTL_NUMBER_OF(Functions); Index++)
        {
            RtlInitUnicodeString(&Match, Functions[Index].Name);
            if (RtlEqualUnicodeString(&Entry, &Match, TRUE))
                *Features |= Functions[Index].Bit;
        }

        if (*Features == HUB_DUAL_ROLE_FUNCTIONS)
            break;

        Position += Length + 1;
    }

    ExFreePoolWithTag(List, HUB_TAG_DEVICE);
FreePath:
    ExFreePoolWithTag(PathBuffer, HUB_TAG_DEVICE);
FreeCurrent:
    ExFreePoolWithTag(Current, HUB_TAG_DEVICE);
}

/*
 * A test override replaces all but the UCM bit. Otherwise the USB function
 * list sets the function bits, unless the default configuration is included.
 */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubQueryLocalDualRoleFeatures(
    _Out_ PULONG Features)
{
    ULONG IncludeDefault = 0;
    ULONG Value;

    PAGED_CODE();

    if (!NT_SUCCESS(HubZwReadULong(HubUsbControlPath, L"DualRoleFeaturesTestOverride", Features)))
    {
        *Features = 0;
        HubZwReadULong(HubUsbControlPath, L"DualRoleFeatures", Features);

        HubZwReadULong(HubUsbFnDefaultPath, L"IncludeDefaultCfg", &IncludeDefault);
        if (NT_SUCCESS(HubZwReadULong(HubUsbFnPath, L"IncludeDefaultCfg", &Value)))
            IncludeDefault = Value;

        if (IncludeDefault == 0)
        {
            *Features &= ~HUB_DUAL_ROLE_FUNCTIONS;
            HubAddConfiguredFunctions(Features);
        }
    }

    if (NT_SUCCESS(HubZwReadULong(HubUsbControlPath, L"UcmIsPresent", &Value)))
    {
        if (Value != 0)
            *Features |= HUB_DUAL_ROLE_UCM_PRESENT;
        else
            *Features &= ~HUB_DUAL_ROLE_UCM_PRESENT;
    }
}

/* Device teardown */

VOID
NTAPI
HubReleaseRegistryState(
    _In_ HubChild* Child)
{
    if (Child->m_FriendlyName != NULL)
    {
        ExFreePoolWithTag(Child->m_FriendlyName, HUB_TAG_DEVICE);
        Child->m_FriendlyName = NULL;
        Child->m_FriendlyNameLength = 0;
    }

    if (Child->m_MsOs20Set != NULL)
    {
        ExFreePoolWithTag(Child->m_MsOs20Set, HUB_TAG_DEVICE);
        Child->m_MsOs20Set = NULL;
    }

    if (Child->m_ExtProperties != NULL)
    {
        ExFreePoolWithTag(Child->m_ExtProperties, HUB_TAG_DEVICE);
        Child->m_ExtProperties = NULL;
    }
}
