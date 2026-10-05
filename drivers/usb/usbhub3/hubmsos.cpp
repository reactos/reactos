/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     MS OS 1.0 and 2.0 descriptor decisions and their registry installs
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"
#include "hubregistry.h"

#define NDEBUG
#include <debug.h>

/* MS OS string descriptor bFlags: the container ID feature descriptor exists */
#define MSOS_FLAG_CONTAINER_ID          0x02

/* Extended properties feature descriptor */
#define MSOS_EXT_PROPERTIES_INDEX       5
#define MSOS_EXT_PROPERTIES_VERSION     0x0100
#define MSOS_EXT_PROPERTIES_MAX         4096

/* MS OS 2.0 registry property descriptor type */
#define MSOS20_TYPE_REGISTRY_PROPERTY   4

#include <pshpack1.h>

/* Extended properties header, followed by Count custom properties */
struct HubMsOsExtPropHeader
{
    ULONG dwLength;
    USHORT bcdVersion;
    USHORT wIndex;
    USHORT wCount;
};

/* One custom property; the name, a ULONG data length and the data follow */
struct HubMsOsCustomProperty
{
    ULONG dwSize;
    ULONG dwPropertyDataType;
    USHORT wPropertyNameLength;
};

#include <poppack.h>

C_ASSERT(sizeof(HubMsOsExtPropHeader) == 10);
C_ASSERT(sizeof(HubMsOsCustomProperty) == 10);

/* Size, type, name length and data length */
#define MSOS_CUSTOM_PROPERTY_FIXED      (sizeof(HubMsOsCustomProperty) + sizeof(ULONG))

/* Version gate shared by the MS OS 1.0 queries: USB 2.0 and later, or no version at all */
static
BOOLEAN
NTAPI
HubMsOsVersionAllowed(
    _In_ HubChild* Child)
{
    USHORT BcdUsb = Child->m_DeviceDescriptor.bcdUSB;

    return BcdUsb >= 0x0200 || BcdUsb < 0x0100;
}

static
BOOLEAN
NTAPI
HubHasMsOs20SetInfo(
    _In_ HubChild* Child)
{
    return (Child->m_MsOs20.Flags & HUB_MSOS20_SET_INFO) != 0;
}

static
const HubMsOsExtPropHeader*
NTAPI
HubExtPropHeader(
    _In_ HubChild* Child)
{
    return (const HubMsOsExtPropHeader*)Child->m_ScratchBuffer;
}

/* MS OS 2.0 */

/* Every errata, property and state bit except the alternate enumeration ones is dropped; the registry query runs again afterwards */
BOOLEAN
DeviceMachine::AltEnumNeededInEnum()
{
    const LONG Keep = (LONG)ChildState::AltEnumCompleted | (LONG)ChildState::AltEnumCommandSent;

    if (!m_Device->m_AltEnumCached || m_Device->HasState(ChildState::AltEnumCommandSent))
        return FALSE;

    m_Device->SetState(ChildState::AltEnumCompleted);
    m_Device->m_OriginalDeviceDescriptor = m_Device->m_DeviceDescriptor;

    m_Device->m_Hacks = 0;
    InterlockedExchange(&m_Device->m_Properties, 0);
    InterlockedAnd(&m_Device->m_StateFlags, Keep);

    return TRUE;
}

BOOLEAN
DeviceMachine::AltEnumNeededAfterBos()
{
    return (m_Device->m_MsOs20.Flags & HUB_MSOS20_ALT_ENUM) &&
           !m_Device->HasState(ChildState::AltEnumCommandSent);
}

BOOLEAN
DeviceMachine::AltEnumNeededOnReenum()
{
    if (!AltEnumNeededAfterBos())
        return FALSE;

    m_Device->SetState(ChildState::AltEnumCompleted);
    return TRUE;
}

BOOLEAN
DeviceMachine::MsOs20Supported()
{
    return HubHasMsOs20SetInfo(m_Device) && m_Device->m_MsOs20SetInfo.wLength != 0;
}

BOOLEAN
DeviceMachine::MsOs20ValuesWanted()
{
    if (!(m_Device->m_MsOs20.Flags & (HUB_MSOS20_REGISTRY | HUB_MSOS20_MODEL_ID)))
        return FALSE;

    return !m_Device->HasProperty(ChildProperty::ExtPropertiesInstalled);
}

/* One registry property descriptor of the set into the hardware key */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubInstallMsOs20Property(
    _In_ HubChild* Child,
    _In_ const HubMsOs20Registry* Property)
{
    const UCHAR* Name = (const UCHAR*)(Property + 1);
    const UCHAR* Tail = Name + Property->wPropertyNameLength;
    UNICODE_STRING ValueName;
    USHORT DataLength;

    RtlCopyMemory(&DataLength, Tail, sizeof(DataLength));

    /* The descriptor counts the NUL; the value name must not */
    ValueName.Buffer = (PWCHAR)Name;
    ValueName.Length = 0;
    while (ValueName.Length < Property->wPropertyNameLength &&
           ValueName.Buffer[ValueName.Length / sizeof(WCHAR)] != UNICODE_NULL)
    {
        ValueName.Length += sizeof(WCHAR);
    }
    ValueName.MaximumLength = ValueName.Length;

    return HubWriteDeviceValue(Child,
                               &ValueName,
                               Property->wPropertyDataType,
                               DataLength,
                               (PVOID)(Tail + sizeof(DataLength)));
}

/* Only top level properties are installed; a failed write stops the walk but still answers TRUE */
BOOLEAN
DeviceMachine::InstallMsOs20Values()
{
    HubChild* Child = m_Device;
    const HubMsOs20SetHeader* Header = (const HubMsOs20SetHeader*)Child->m_MsOs20Set;
    HubMsOs20Common* Current;
    BOOLEAN Result = TRUE;
    PUCHAR End;
    NTSTATUS Status;

    PAGED_CODE();

    if (Header == NULL)
    {
        DPRINT1("Device %p has no MS OS 2.0 descriptor set to install\n", Child);
        return FALSE;
    }

    End = (PUCHAR)Header + Header->wTotalLength;
    Current = (HubMsOs20Common*)Header;

    while (Current != NULL)
    {
        if (Current->wDescriptorType == MSOS20_TYPE_REGISTRY_PROPERTY)
        {
            /* The registry write reports its own failure */
            Status = HubInstallMsOs20Property(Child, (const HubMsOs20Registry*)Current);
            if (!NT_SUCCESS(Status))
                break;
        }

        if (!HubDescNextMsOs20(End, &Current))
        {
            DPRINT1("Device %p MS OS 2.0 descriptor set walk failed\n", Child);
            Result = FALSE;
            break;
        }
    }

    /* The model id goes out whatever the property walk did */
    if ((Child->m_MsOs20.Flags & HUB_MSOS20_MODEL_ID) && Child->m_MsOs20.ModelId != NULL && Child->m_Pdo != NULL)
    {
        Status = IoSetDevicePropertyData(WdfDeviceWdmGetDeviceObject(Child->m_Pdo->m_Device),
                                         &DEVPKEY_Device_ModelId,
                                         LOCALE_NEUTRAL,
                                         0,
                                         DEVPROP_TYPE_GUID,
                                         sizeof(Child->m_MsOs20.ModelId->ModelId),
                                         Child->m_MsOs20.ModelId->ModelId);
        if (!NT_SUCCESS(Status))
            DPRINT1("Setting the model id failed 0x%lx\n", Status);
    }

    return Result;
}

/* MS OS 1.0 query gates */

BOOLEAN
DeviceMachine::MsOsQueryWanted()
{
    HubChild* Child = m_Device;

    if (HubHasMsOs20SetInfo(Child))
        return FALSE;

    switch (HubDriver.MsOsMode)
    {
        case HubMsOsMode::ForceQuery:
            Child->ClearProperty(ChildProperty::MsOsNotSupported);
            return TRUE;

        case HubMsOsMode::NeverQuery:
            Child->SetProperty(ChildProperty::MsOsNotSupported);
            return FALSE;

        default:
            break;
    }

    if (HubMsOsVersionAllowed(Child) &&
        !Child->HasProperty(ChildProperty::MsOsNotSupported) &&
        Child->m_MsOsVendorCode == 0)
    {
        return TRUE;
    }

    return Child->HasHack(ChildHack::AlwaysQueryMsOs);
}

BOOLEAN
DeviceMachine::MsOsExtendedConfigSupported()
{
    HubChild* Child = m_Device;

    if (HubHasMsOs20SetInfo(Child))
        return FALSE;

    if (HubMsOsVersionAllowed(Child) &&
        !Child->HasProperty(ChildProperty::MsOsNotSupported) &&
        !Child->HasProperty(ChildProperty::IsComposite))
    {
        return TRUE;
    }

    return Child->HasHack(ChildHack::AlwaysQueryMsOs);
}

BOOLEAN
DeviceMachine::ContainerIdWanted()
{
    HubChild* Child = m_Device;

    if (HubHasMsOs20SetInfo(Child))
        return FALSE;

    if (HubMsOsVersionAllowed(Child) &&
        !Child->HasProperty(ChildProperty::MsOsNotSupported) &&
        !Child->HasHack(ChildHack::SkipContainerIdQuery))
    {
        return TRUE;
    }

    return Child->HasHack(ChildHack::AlwaysQueryMsOs);
}

BOOLEAN
DeviceMachine::MsOsContainerIdSupported()
{
    return (m_Device->m_MsOsString.bPad & MSOS_FLAG_CONTAINER_ID) != 0;
}

/* Reads the semaphore first, so the answer reflects the hardware key now */
BOOLEAN
DeviceMachine::ExtPropertiesWanted()
{
    HubChild* Child = m_Device;
    USHORT BcdUsb = Child->m_DeviceDescriptor.bcdUSB;
    BOOLEAN Installed;

    PAGED_CODE();

    HubRefreshExtPropertiesInstalled(Child);
    Installed = Child->HasProperty(ChildProperty::ExtPropertiesInstalled);

    if (Child->HasProperty(ChildProperty::IsComposite) || HubHasMsOs20SetInfo(Child))
        return FALSE;

    if (Child->HasHack(ChildHack::AlwaysQueryMsOs) && !Installed)
        return TRUE;

    if ((BcdUsb >= 0x0100 && BcdUsb <= 0x01FF) ||
        Installed ||
        Child->HasProperty(ChildProperty::MsOsNotSupported))
    {
        return FALSE;
    }

    return TRUE;
}

/* MS OS 1.0 usbflags records; write failures are ignored */

VOID
DeviceMachine::MarkMsOsUnsupported()
{
    UCHAR Record[2] = { 0, 0 };

    PAGED_CODE();

    HubWriteUsbflagsValue(m_Device, L"osvc", Record, sizeof(Record));
    m_Device->SetProperty(ChildProperty::MsOsNotSupported);
}

VOID
DeviceMachine::StoreMsOsVendorCode()
{
    UCHAR Record[2];

    PAGED_CODE();

    Record[0] = 1;
    Record[1] = m_Device->m_MsOsVendorCode;
    HubWriteUsbflagsValue(m_Device, L"osvc", Record, sizeof(Record));
}

/* Read back as the 2 byte value 1 by the next errata query */
VOID
DeviceMachine::MarkContainerIdUnsupported()
{
    USHORT Record = 1;

    HubWriteUsbflagsValue(m_Device, L"SkipContainerIdQuery", &Record, sizeof(Record));
    m_Device->m_Hacks |= (ULONG)ChildHack::SkipContainerIdQuery;
}

/* MS OS 1.0 extended properties */

VOID
DeviceMachine::MarkMsOsInstallHandled()
{
    m_Device->SetState(ChildState::MsOsInstallHandled);
}

BOOLEAN
DeviceMachine::SetExtPropertiesSemaphore()
{
    DECLARE_CONST_UNICODE_STRING(Name, L"ExtPropDescSemaphore");
    ULONG Value = 1;

    PAGED_CODE();

    return NT_SUCCESS(HubWriteDeviceValue(m_Device, &Name, REG_DWORD, sizeof(Value), &Value));
}

/* The header stays in the scratch buffer for the full read and its check */
BOOLEAN
DeviceMachine::ExtPropertiesHeaderValid()
{
    const HubMsOsExtPropHeader* Header = HubExtPropHeader(m_Device);

    if (m_Device->m_Control.Urb.TransferBufferLength != sizeof(*Header))
    {
        DPRINT1("Device %p extended properties header is %lu bytes\n",
                m_Device, m_Device->m_Control.Urb.TransferBufferLength);
        return FALSE;
    }

    if (Header->dwLength >= sizeof(*Header) &&
        Header->bcdVersion == MSOS_EXT_PROPERTIES_VERSION &&
        Header->wIndex == MSOS_EXT_PROPERTIES_INDEX &&
        Header->wCount != 0)
    {
        return TRUE;
    }

    DPRINT1("Device %p extended properties header is invalid\n", m_Device);
    return FALSE;
}

/* Sized from the header before the 4 KB cap is checked */
BOOLEAN
DeviceMachine::AllocateExtPropertiesBuffer()
{
    HubChild* Child = m_Device;

    if (Child->m_ExtProperties != NULL)
        ExFreePoolWithTag(Child->m_ExtProperties, HUB_TAG_DEVICE);

    Child->m_ExtProperties = ExAllocatePoolWithTag(NonPagedPool, HubExtPropHeader(Child)->dwLength, HUB_TAG_DEVICE);
    if (Child->m_ExtProperties == NULL)
    {
        DPRINT1("Device %p extended properties of %lu bytes could not be allocated\n",
                Child, HubExtPropHeader(Child)->dwLength);
        return FALSE;
    }

    return TRUE;
}

/* Custom property at Offset fits and is well formed; Size gets its dwSize */
static
BOOLEAN
NTAPI
HubCustomPropertyValid(
    _In_reads_bytes_(Total) const UCHAR* Buffer,
    _In_ ULONG Total,
    _In_ ULONG Offset,
    _Out_ PULONG Size)
{
    const HubMsOsCustomProperty* Property = (const HubMsOsCustomProperty*)(Buffer + Offset);
    ULONG Remaining = Total - Offset;
    ULONG NameLength;
    ULONG DataLength;
    const WCHAR* Name;

    if (Remaining < MSOS_CUSTOM_PROPERTY_FIXED)
        return FALSE;

    *Size = Property->dwSize;
    if (*Size > Remaining || *Size < MSOS_CUSTOM_PROPERTY_FIXED)
        return FALSE;

    if (Property->dwPropertyDataType < REG_SZ || Property->dwPropertyDataType > REG_MULTI_SZ)
        return FALSE;

    NameLength = Property->wPropertyNameLength;
    if (NameLength == 0 || (NameLength & 1) || NameLength > *Size - MSOS_CUSTOM_PROPERTY_FIXED)
        return FALSE;

    Name = (const WCHAR*)(Property + 1);
    if (Name[0] == UNICODE_NULL || Name[NameLength / sizeof(WCHAR) - 1] != UNICODE_NULL)
        return FALSE;

    RtlCopyMemory(&DataLength, (const UCHAR*)Name + NameLength, sizeof(DataLength));

    return DataLength <= *Size - MSOS_CUSTOM_PROPERTY_FIXED - NameLength;
}

/* Bytes after the last counted property are allowed */
BOOLEAN
DeviceMachine::ExtPropertiesValid()
{
    HubChild* Child = m_Device;
    const HubMsOsExtPropHeader* Header = HubExtPropHeader(Child);
    const UCHAR* Buffer = (const UCHAR*)Child->m_ExtProperties;
    ULONG Total = Header->dwLength;
    ULONG Offset;
    ULONG Size;
    ULONG Found = 0;

    if (Buffer == NULL || Child->m_Control.Urb.TransferBufferLength != Total)
    {
        DPRINT1("Device %p extended properties: %lu of %lu bytes read\n",
                Child, Child->m_Control.Urb.TransferBufferLength, Total);
        return FALSE;
    }

    if (RtlCompareMemory(Buffer, Header, sizeof(*Header)) != sizeof(*Header))
    {
        DPRINT1("Device %p extended properties header changed between reads\n", Child);
        return FALSE;
    }

    if (Total > MSOS_EXT_PROPERTIES_MAX)
    {
        DPRINT1("Device %p extended properties too large (%lu bytes)\n", Child, Total);
        return FALSE;
    }

    for (Offset = sizeof(*Header); Offset < Total; Offset += Size)
    {
        if (!HubCustomPropertyValid(Buffer, Total, Offset, &Size))
        {
            DPRINT1("Device %p custom property at offset %lu is invalid\n", Child, Offset);
            return FALSE;
        }

        if (++Found == Header->wCount)
            break;
    }

    if (Found != Header->wCount)
    {
        DPRINT1("Device %p has %lu custom properties, header says %u\n", Child, Found, Header->wCount);
        return FALSE;
    }

    return TRUE;
}

/* The first failed write ends the install */
BOOLEAN
DeviceMachine::WriteCustomProperties()
{
    HubChild* Child = m_Device;
    const UCHAR* Cursor = (const UCHAR*)Child->m_ExtProperties + sizeof(HubMsOsExtPropHeader);
    USHORT Count = ((const HubMsOsExtPropHeader*)Child->m_ExtProperties)->wCount;
    const HubMsOsCustomProperty* Property;
    UNICODE_STRING Name;
    const UCHAR* Tail;
    ULONG DataLength;
    USHORT Index;

    PAGED_CODE();

    for (Index = 0; Index < Count; Index++)
    {
        Property = (const HubMsOsCustomProperty*)Cursor;

        RtlInitUnicodeString(&Name, (PCWSTR)(Property + 1));
        Tail = (const UCHAR*)(Property + 1) + Property->wPropertyNameLength;
        RtlCopyMemory(&DataLength, Tail, sizeof(DataLength));

        if (!NT_SUCCESS(HubWriteDeviceValue(Child,
                                            &Name,
                                            Property->dwPropertyDataType,
                                            DataLength,
                                            (PVOID)(Tail + sizeof(DataLength)))))
        {
            return FALSE;
        }

        Cursor += Property->dwSize;
    }

    return TRUE;
}

VOID
DeviceMachine::FreeExtPropertiesBuffer()
{
    NT_ASSERT(m_Device->m_ExtProperties != NULL);

    if (m_Device->m_ExtProperties != NULL)
        ExFreePoolWithTag(m_Device->m_ExtProperties, HUB_TAG_DEVICE);
    m_Device->m_ExtProperties = NULL;
}
