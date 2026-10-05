/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     PnP ids, container id, device text and the cached serial number
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"
#include "hubid.h"

#define NDEBUG
#include <debug.h>

/* Every formatted id fits in this many characters */
#define HUB_ID_MAX_CHARS            128

/* Room for a decimal ULONG port number */
#define HUB_INSTANCE_ID_CHARS       28

/* sizeof(L"Port_#nnnn.Hub_#nnnn") */
#define HUB_LOCATION_TEXT_BYTES     42

#define HUB_DEFAULT_LANGUAGE        0x0409

/* Name space of generated container ids {4B06FD46-C84E-4664-9C65-0C86D9047A0C} */
static const GUID HubContainerIdNamespace =
    { 0x4B06FD46, 0xC84E, 0x4664, { 0x9C, 0x65, 0x0C, 0x86, 0xD9, 0x04, 0x7A, 0x0C } };

/* Ids for a device that failed enumeration; the NUL stays in the counted length, which PnP never sees */
struct HubFailureId
{
    ULONG Reason;
    USHORT ProductId;
    PCWSTR Id;
    USHORT IdBytes;
};

#define HUB_FAILURE_ID(Reason, Pid, Text) { (Reason), (Pid), Text, sizeof(Text) }

static const HubFailureId HubFailureIds[] =
{
    HUB_FAILURE_ID(HUB_ENUM_DEVICE_DESCRIPTOR_FAILED, 0x0002, L"USB\\DEVICE_DESCRIPTOR_FAILURE"),
    HUB_FAILURE_ID(HUB_ENUM_SET_ADDRESS_FAILED,       0x0004, L"USB\\SET_ADDRESS_FAILURE"),
    HUB_FAILURE_ID(HUB_ENUM_PORT_RESET_FAILED,        0x0001, L"USB\\RESET_FAILURE"),
    HUB_FAILURE_ID(HUB_ENUM_BAD_CONFIG_DESCRIPTOR,    0x0006, L"USB\\CONFIGURATION_DESCRIPTOR_VALIDATION_FAILURE"),
    HUB_FAILURE_ID(HUB_ENUM_BAD_DEVICE_DESCRIPTOR,    0x0005, L"USB\\DEVICE_DESCRIPTOR_VALIDATION_FAILURE"),
    HUB_FAILURE_ID(HUB_ENUM_CONFIG_DESCRIPTOR_FAILED, 0x0003, L"USB\\CONFIG_DESCRIPTOR_FAILURE"),
    HUB_FAILURE_ID(HUB_ENUM_LINK_SS_INACTIVE,         0x0007, L"USB\\PORT_LINK_SSINACTIVE"),
    HUB_FAILURE_ID(HUB_ENUM_LINK_COMPLIANCE,          0x0008, L"USB\\PORT_LINK_COMPLIANCE_MODE"),
};

static const HubFailureId HubUnknownDeviceId =
    HUB_FAILURE_ID(0, 0x0000, L"USB\\UNKNOWN_DEVICE");

/* A billboard gets one class id with a counted NUL, like the failure ids */
static const WCHAR HubBillboardId[] = L"USB\\Class_11&SubClass_00&Prot_00";

static
const HubFailureId*
NTAPI
HubFailureIdFor(
    _In_ HubChild* Child)
{
    ULONG Index;

    for (Index = 0; Index < RTL_NUMBER_OF(HubFailureIds); Index++)
    {
        if (HubFailureIds[Index].Reason == Child->m_EnumMessageId)
            return &HubFailureIds[Index];
    }

    return &HubUnknownDeviceId;
}

/* ID LISTS ******************************************************************/

/* Where a builder sends each id it makes */
enum class HubIdKind
{
    Device,
    Hardware,
    Compatible,
    Container
};

struct HubIdSink
{
    HubIdKind Kind;
    PWDFDEVICE_INIT Init;
    PUSB_ID_STRING List;
};

/** One id being put together in a fixed buffer; the first failure sticks. */
struct HubIdText
{
    WCHAR Buffer[HUB_ID_MAX_CHARS];
    NTSTATUS Status;

    VOID
    Format(
        _In_z_ _Printf_format_string_ PCWSTR Format,
        ...)
    {
        va_list Args;

        va_start(Args, Format);
        Status = RtlStringCbVPrintfW(Buffer, sizeof(Buffer), Format, Args);
        va_end(Args);
    }

    VOID
    Set(
        _In_z_ PCWSTR Text)
    {
        Status = RtlStringCbCopyW(Buffer, sizeof(Buffer), Text);
    }

    VOID
    Append(
        _In_z_ PCWSTR Text)
    {
        if (NT_SUCCESS(Status))
            Status = RtlStringCbCatW(Buffer, sizeof(Buffer), Text);
    }
};

static
NTSTATUS
NTAPI
HubIdListAppend(
    _Inout_ PUSB_ID_STRING List,
    _In_ PCUNICODE_STRING Id)
{
    ULONG Kept = (List->Buffer != NULL) ? List->LengthInBytes - sizeof(WCHAR) : 0;
    ULONG Total = Kept + Id->Length + 2 * sizeof(WCHAR);
    PWCHAR Buffer;

    Buffer = (PWCHAR)ExAllocatePoolWithTag(NonPagedPool, Total, HUB_TAG_DEVICE);
    if (Buffer == NULL)
    {
        DPRINT1("Id list of %lu bytes could not be allocated\n", Total);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Buffer, Total);
    if (Kept != 0)
        RtlCopyMemory(Buffer, List->Buffer, Kept);
    RtlCopyMemory((PUCHAR)Buffer + Kept, Id->Buffer, Id->Length);

    if (List->Buffer != NULL)
        ExFreePoolWithTag(List->Buffer, HUB_TAG_DEVICE);

    List->Buffer = Buffer;
    List->LengthInBytes = Total;
    return STATUS_SUCCESS;
}

VOID
NTAPI
HubIdListFree(
    _Inout_ PUSB_ID_STRING Ids)
{
    if (Ids->Buffer == NULL)
        return;

    ExFreePoolWithTag(Ids->Buffer, HUB_TAG_DEVICE);
    Ids->Buffer = NULL;
    Ids->LengthInBytes = 0;
}

/* On allocation failure the copy keeps the source's language and length with no buffer */
NTSTATUS
NTAPI
HubIdListCopy(
    _Out_ PUSB_ID_STRING Destination,
    _In_ const USB_ID_STRING* Source)
{
    RtlZeroMemory(Destination, sizeof(*Destination));

    if (Source->Buffer == NULL || Source->LengthInBytes == 0)
        return STATUS_SUCCESS;

    *Destination = *Source;
    Destination->Buffer = (PWCHAR)ExAllocatePoolWithTag(NonPagedPool, Source->LengthInBytes, HUB_TAG_DEVICE);
    if (Destination->Buffer == NULL)
    {
        DPRINT1("Id list copy of %lu bytes could not be allocated\n", Source->LengthInBytes);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(Destination->Buffer, Source->Buffer, Source->LengthInBytes);
    return STATUS_SUCCESS;
}

/* The PDO init gets the id first; the list is only touched when that worked */
static
NTSTATUS
NTAPI
HubIdEmit(
    _In_ const HubIdSink* Sink,
    _In_ PCUNICODE_STRING Id)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (Sink->Init != NULL)
    {
        switch (Sink->Kind)
        {
            case HubIdKind::Device:
                Status = WdfPdoInitAssignDeviceID(Sink->Init, Id);
                break;

            case HubIdKind::Hardware:
                Status = WdfPdoInitAddHardwareID(Sink->Init, Id);
                break;

            case HubIdKind::Compatible:
                Status = WdfPdoInitAddCompatibleID(Sink->Init, Id);
                break;

            case HubIdKind::Container:
                Status = WdfPdoInitAssignContainerID(Sink->Init, Id);
                break;
        }

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Assigning id %wZ (kind %d) failed 0x%lx\n", Id, (int)Sink->Kind, Status);
            return Status;
        }
    }

    if (Sink->List != NULL)
        Status = HubIdListAppend(Sink->List, Id);

    return Status;
}

static
NTSTATUS
NTAPI
HubIdEmitText(
    _In_ const HubIdSink* Sink,
    _In_ const HubIdText* Text)
{
    UNICODE_STRING Id;

    if (!NT_SUCCESS(Text->Status))
    {
        DPRINT1("Formatting an id failed 0x%lx\n", Text->Status);
        return Text->Status;
    }

    RtlInitUnicodeString(&Id, Text->Buffer);
    return HubIdEmit(Sink, &Id);
}

/** Emits a fixed id whose byte count is given as is, counted NUL included. */
static
NTSTATUS
NTAPI
HubIdEmitCounted(
    _In_ const HubIdSink* Sink,
    _In_ PCWSTR Id,
    _In_ USHORT IdBytes)
{
    UNICODE_STRING String;

    String.Buffer = (PWCH)Id;
    String.Length = IdBytes;
    String.MaximumLength = IdBytes;
    return HubIdEmit(Sink, &String);
}

/* DEVICE PROPERTIES THE IDS DEPEND ON ****************************************/

static
BOOLEAN
NTAPI
HubIdIsKnown(
    _In_ HubChild* Child)
{
    return Child->HasState(ChildState::WorkingDevice);
}

/* Neither a working device nor one with a usable configuration */
static
BOOLEAN
NTAPI
HubIdUsesFailureId(
    _In_ HubChild* Child)
{
    return !HubIdIsKnown(Child) && !Child->HasState(ChildState::ConfigurationValid);
}

static
BOOLEAN
NTAPI
HubIdIsVmReserved(
    _In_ HubChild* Child)
{
    return Child->HasProperty(ChildProperty::UxdReserved) &&
           !Child->HasProperty(ChildProperty::IsHub);
}

/* bcdDevice as four characters; up to USB 2.0 each nibble is added to '0', so A to F become ':' to '?' */
static
VOID
NTAPI
HubIdRevisionText(
    _In_ USHORT BcdUsb,
    _In_ USHORT BcdDevice,
    _Out_writes_(5) PWCHAR Text)
{
    static const WCHAR HexDigits[] = L"0123456789ABCDEF";
    ULONG Index;
    ULONG Nibble;

    for (Index = 0; Index < 4; Index++)
    {
        Nibble = (BcdDevice >> (12 - Index * 4)) & 0xF;
        Text[Index] = (BcdUsb > 0x0200) ? HexDigits[Nibble] : (WCHAR)(L'0' + Nibble);
    }

    Text[4] = UNICODE_NULL;
}

/* An 8 byte ANSI field of an MS OS descriptor, up to its first zero byte */
static
VOID
NTAPI
HubIdWidenMsOsField(
    _In_reads_(8) const UCHAR* Field,
    _Out_writes_(9) PWCHAR Text)
{
    ULONG Index;

    for (Index = 0; Index < 8 && Field[Index] != 0; Index++)
        Text[Index] = Field[Index];

    Text[Index] = UNICODE_NULL;
}

/* DEVICE ID *****************************************************************/

static
NTSTATUS
NTAPI
HubIdBuildDeviceId(
    _In_ HubChild* Child,
    _In_ PWDFDEVICE_INIT DeviceInit)
{
    HubIdSink Sink = { HubIdKind::Device, DeviceInit, NULL };
    HubIdText Text;
    USHORT VendorId = Child->m_DeviceDescriptor.idVendor;
    USHORT ProductId = Child->m_DeviceDescriptor.idProduct;

    if (HubIdIsKnown(Child) && HubIdIsVmReserved(Child))
        return HubIdEmit(&Sink, &Child->m_VmReservedId);

    if (HubIdUsesFailureId(Child))
    {
        VendorId = 0;
        ProductId = HubFailureIdFor(Child)->ProductId;
    }

    Text.Format(L"USB\\VID_%04X&PID_%04X", VendorId, ProductId);
    if (Child->HasHack(ChildHack::NonFunctional))
        Text.Append(L"_NON_FUNCTIONAL");

    return HubIdEmitText(&Sink, &Text);
}

/* HARDWARE IDS **************************************************************/

/*
 * The VM reserved id only replaces the hardware ids of a PDO being created;
 * a list built for GET_DEVICE_CONFIG_INFO keeps the VID and PID ids.
 */
NTSTATUS
NTAPI
HubIdBuildHardwareIds(
    _In_ HubChild* Child,
    _In_opt_ PWDFDEVICE_INIT DeviceInit,
    _Out_opt_ PUSB_ID_STRING Ids)
{
    HubIdSink Sink = { HubIdKind::Hardware, DeviceInit, Ids };
    const USB_DEVICE_DESCRIPTOR* Descriptor = &Child->m_DeviceDescriptor;
    HubIdText Text;
    WCHAR Revision[5];
    NTSTATUS Status;

    if (Ids != NULL)
        RtlZeroMemory(Ids, sizeof(*Ids));

    if (HubIdUsesFailureId(Child))
    {
        const HubFailureId* Failure = HubFailureIdFor(Child);
        return HubIdEmitCounted(&Sink, Failure->Id, Failure->IdBytes);
    }

    if (DeviceInit != NULL && HubIdIsVmReserved(Child))
        return HubIdEmit(&Sink, &Child->m_VmReservedId);

    HubIdRevisionText(Descriptor->bcdUSB, Descriptor->bcdDevice, Revision);

    Text.Format(L"USB\\VID_%04X&PID_%04X&REV_", Descriptor->idVendor, Descriptor->idProduct);
    Text.Append(Revision);
    Status = HubIdEmitText(&Sink, &Text);
    if (!NT_SUCCESS(Status))
        return Status;

    Text.Format(L"USB\\VID_%04X&PID_%04X", Descriptor->idVendor, Descriptor->idProduct);
    return HubIdEmitText(&Sink, &Text);
}

/* COMPATIBLE IDS ************************************************************/

/* The COMPAT_VID forms go in front of the DevClass forms */
static
NTSTATUS
NTAPI
HubIdBuildCompositeIds(
    _In_ HubChild* Child,
    _In_ const HubIdSink* Sink)
{
    static PCWSTR const Plain[] =
    {
        L"USB\\DevClass_00&SubClass_00&Prot_00",
        L"USB\\DevClass_00&SubClass_00",
        L"USB\\DevClass_00",
        L"USB\\COMPOSITE"
    };
    USHORT VendorId = Child->m_DeviceDescriptor.idVendor;
    HubIdText Text;
    NTSTATUS Status;
    ULONG Index;

    /* "Prot00" without the underscore is intentional */
    Text.Format(L"USB\\COMPAT_VID_%04X&DevClass_00&SubClass_00&Prot00", VendorId);
    Status = HubIdEmitText(Sink, &Text);
    if (!NT_SUCCESS(Status))
        return Status;

    Text.Format(L"USB\\COMPAT_VID_%04X&DevClass_00&SubClass_00", VendorId);
    Status = HubIdEmitText(Sink, &Text);
    if (!NT_SUCCESS(Status))
        return Status;

    Text.Format(L"USB\\COMPAT_VID_%04X&DevClass_00", VendorId);
    Status = HubIdEmitText(Sink, &Text);
    if (!NT_SUCCESS(Status))
        return Status;

    for (Index = 0; Index < RTL_NUMBER_OF(Plain); Index++)
    {
        Text.Set(Plain[Index]);
        Status = HubIdEmitText(Sink, &Text);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
HubIdEmitMsComp(
    _In_ const HubIdSink* Sink,
    _In_reads_(8) const UCHAR* CompatibleId,
    _In_reads_opt_(8) const UCHAR* SubCompatibleId)
{
    HubIdText Text;
    WCHAR Field[9];

    HubIdWidenMsOsField(CompatibleId, Field);
    Text.Set(L"USB\\MS_COMP_");
    Text.Append(Field);

    if (SubCompatibleId != NULL)
    {
        HubIdWidenMsOsField(SubCompatibleId, Field);
        Text.Append(L"&MS_SUBCOMP_");
        Text.Append(Field);
    }

    return HubIdEmitText(Sink, &Text);
}

/* MS OS 2.0 wins over MS OS 1.0; only the first 1.0 function counts */
static
NTSTATUS
NTAPI
HubIdBuildMsOsIds(
    _In_ HubChild* Child,
    _In_ const HubIdSink* Sink)
{
    const UCHAR* CompatibleId;
    const UCHAR* SubCompatibleId;
    NTSTATUS Status;

    if ((Child->m_MsOs20.Flags & HUB_MSOS20_COMPATIBLE_ID) && Child->m_MsOs20.CompatibleId != NULL)
    {
        CompatibleId = Child->m_MsOs20.CompatibleId->CompatibleId;
        SubCompatibleId = Child->m_MsOs20.CompatibleId->SubCompatibleId;

        if (SubCompatibleId[0] != 0 && CompatibleId[0] != 0)
        {
            Status = HubIdEmitMsComp(Sink, CompatibleId, SubCompatibleId);
            if (!NT_SUCCESS(Status))
                return Status;
        }
    }
    else if (Child->m_MsOsExtConfig != NULL)
    {
        CompatibleId = Child->m_MsOsExtConfig->Function[0].CompatibleId;
        SubCompatibleId = Child->m_MsOsExtConfig->Function[0].SubCompatibleId;

        /* Only the sub compatible id is checked, so the compatible id can come out empty */
        if (SubCompatibleId[0] != 0)
        {
            Status = HubIdEmitMsComp(Sink, CompatibleId, SubCompatibleId);
            if (!NT_SUCCESS(Status))
                return Status;
        }
    }
    else
    {
        return STATUS_SUCCESS;
    }

    if (CompatibleId[0] != 0)
        return HubIdEmitMsComp(Sink, CompatibleId, NULL);

    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
HubIdBuildClassIds(
    _In_ HubChild* Child,
    _In_ const HubIdSink* Sink)
{
    const USB_INTERFACE_DESCRIPTOR* Interface = Child->m_CompatIdInterface;
    USHORT VendorId = Child->m_DeviceDescriptor.idVendor;
    UCHAR Class = 0;
    UCHAR SubClass = 0;
    UCHAR Protocol = 0;
    HubIdText Text;
    NTSTATUS Status;

    if (Interface != NULL)
    {
        Class = Interface->bInterfaceClass;
        SubClass = Interface->bInterfaceSubClass;
        Protocol = Interface->bInterfaceProtocol;
    }

    Status = HubIdBuildMsOsIds(Child, Sink);
    if (!NT_SUCCESS(Status))
        return Status;

    /* COMPAT_VID class ids, between the MS OS ids and the plain class ids */
    Text.Format(L"USB\\COMPAT_VID_%04X&Class_%02X&SubClass_%02X&Prot_%02X", VendorId, Class, SubClass, Protocol);
    Status = HubIdEmitText(Sink, &Text);
    if (!NT_SUCCESS(Status))
        return Status;

    Text.Format(L"USB\\COMPAT_VID_%04X&Class_%02X&SubClass_%02X", VendorId, Class, SubClass);
    Status = HubIdEmitText(Sink, &Text);
    if (!NT_SUCCESS(Status))
        return Status;

    Text.Format(L"USB\\COMPAT_VID_%04X&Class_%02X", VendorId, Class);
    Status = HubIdEmitText(Sink, &Text);
    if (!NT_SUCCESS(Status))
        return Status;

    Text.Format(L"USB\\Class_%02X&SubClass_%02X&Prot_%02X", Class, SubClass, Protocol);
    Status = HubIdEmitText(Sink, &Text);
    if (!NT_SUCCESS(Status))
        return Status;

    Text.Format(L"USB\\Class_%02X&SubClass_%02X", Class, SubClass);
    Status = HubIdEmitText(Sink, &Text);
    if (!NT_SUCCESS(Status))
        return Status;

    Text.Format(L"USB\\Class_%02X", Class);
    return HubIdEmitText(Sink, &Text);
}

/* A composite hub gets the composite ids, and a low speed hub gets none */
NTSTATUS
NTAPI
HubIdBuildCompatibleIds(
    _In_ HubChild* Child,
    _In_opt_ PWDFDEVICE_INIT DeviceInit,
    _Out_opt_ PUSB_ID_STRING Ids)
{
    HubIdSink Sink = { HubIdKind::Compatible, DeviceInit, Ids };
    HubIdText Text;

    if (Ids != NULL)
        RtlZeroMemory(Ids, sizeof(*Ids));

    if (HubIdUsesFailureId(Child))
    {
        const HubFailureId* Failure = HubFailureIdFor(Child);
        return HubIdEmitCounted(&Sink, Failure->Id, Failure->IdBytes);
    }

    if (Child->HasProperty(ChildProperty::IsComposite))
        return HubIdBuildCompositeIds(Child, &Sink);

    if (Child->HasProperty(ChildProperty::IsHub))
    {
        switch (Child->Speed())
        {
            case UsbHighSpeed:
            case UsbFullSpeed:
                Text.Set(L"USB\\USB20_HUB");
                return HubIdEmitText(&Sink, &Text);

            case UsbSuperSpeed:
                Text.Set(L"USB\\USB30_HUB");
                return HubIdEmitText(&Sink, &Text);

            default:
                return STATUS_SUCCESS;
        }
    }

    if (Child->HasProperty(ChildProperty::HasBillboard))
        return HubIdEmitCounted(&Sink, HubBillboardId, sizeof(HubBillboardId));

    return HubIdBuildClassIds(Child, &Sink);
}

/* CONTAINER ID **************************************************************/

/* Name based id from VID, PID, revision and serial; the raw digest bytes make it not an RFC 4122 v5 UUID */
static
VOID
NTAPI
HubIdGenerateContainerId(
    _In_ HubChild* Child)
{
    const USB_DEVICE_DESCRIPTOR* Descriptor = &Child->m_DeviceDescriptor;
    ULONG NameBytes = Child->m_SerialNumberLength + 28;
    UCHAR Digest[HUB_SHA1_DIGEST_BYTES];
    WCHAR Revision[5];
    HubSha1 Sha;
    PWCHAR Name;
    size_t NameLength;
    NTSTATUS Status;

    Name = (PWCHAR)ExAllocatePoolWithTag(NonPagedPool, NameBytes, HUB_TAG_DEVICE);
    if (Name == NULL)
    {
        DPRINT1("Container id name for device %p could not be allocated\n", Child);
        return;
    }

    if (Descriptor->bcdUSB > 0x0200)
    {
        /* bcdUSB, not bcdDevice, and space padded */
        Status = RtlStringCbPrintfW(Name, NameBytes, L"%04X%04X.%4X",
                                    Descriptor->idVendor, Descriptor->idProduct, Descriptor->bcdUSB);
    }
    else
    {
        HubIdRevisionText(Descriptor->bcdUSB, Descriptor->bcdDevice, Revision);
        Status = RtlStringCbPrintfW(Name, NameBytes, L"%04X%04X", Descriptor->idVendor, Descriptor->idProduct);
        if (NT_SUCCESS(Status))
            Status = RtlStringCbCatW(Name, NameBytes, Revision);
    }

    if (NT_SUCCESS(Status))
        Status = RtlStringCbCatW(Name, NameBytes, Child->m_SerialNumber + Child->m_SerialPrefixLength / sizeof(WCHAR));

    if (NT_SUCCESS(Status))
        Status = RtlStringCbLengthW(Name, NameBytes, &NameLength);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Container id name for device %p failed: 0x%lx\n", Child, Status);
        ExFreePoolWithTag(Name, HUB_TAG_DEVICE);
        return;
    }

    HubSha1Init(&Sha);
    HubSha1Update(&Sha, &HubContainerIdNamespace, sizeof(HubContainerIdNamespace));
    HubSha1Update(&Sha, Name, NameLength);
    HubSha1Final(&Sha, Digest);
    ExFreePoolWithTag(Name, HUB_TAG_DEVICE);

    RtlCopyMemory(&Child->m_ContainerId, Digest, sizeof(Child->m_ContainerId));
    Child->m_ContainerId.Data3 = (Child->m_ContainerId.Data3 & 0x0FFF) | 0x5000;
    Child->m_ContainerId.Data4[0] = (Child->m_ContainerId.Data4[0] & 0x3F) | 0x80;
    Child->SetProperty(ChildProperty::ContainerIdKnown);
}

static
BOOLEAN
NTAPI
HubIdHasContainerId(
    _In_ HubChild* Child)
{
    return Child->HasProperty(ChildProperty::ContainerIdKnown) ||
           Child->HasProperty(ChildProperty::ContainerIdFromBos);
}

/* Never fails the PDO; a fixed device takes its container from its parent */
static
VOID
NTAPI
HubIdBuildContainerId(
    _In_ HubChild* Child,
    _In_ PWDFDEVICE_INIT DeviceInit)
{
    HubIdSink Sink = { HubIdKind::Container, DeviceInit, NULL };
    const GUID* Id = &Child->m_ContainerId;
    HubIdText Text;
    NTSTATUS Status;

    if (Child->HasProperty(ChildProperty::NotRemovable))
        return;

    if (!HubIdHasContainerId(Child) && Child->m_SerialNumber != NULL)
        HubIdGenerateContainerId(Child);

    if (!HubIdHasContainerId(Child))
        return;

    Text.Format(L"{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                Id->Data1, Id->Data2, Id->Data3,
                Id->Data4[0], Id->Data4[1], Id->Data4[2], Id->Data4[3],
                Id->Data4[4], Id->Data4[5], Id->Data4[6], Id->Data4[7]);

    Status = HubIdEmitText(&Sink, &Text);
    if (!NT_SUCCESS(Status))
        DPRINT1("Container id for device %p not assigned: 0x%lx\n", Child, Status);
}

BOOLEAN
NTAPI
HubIdGetContainerId(
    _In_ HubChild* Child,
    _Out_ GUID* ContainerId)
{
    RtlZeroMemory(ContainerId, sizeof(*ContainerId));

    if (Child->HasProperty(ChildProperty::NotRemovable) || !HubIdHasContainerId(Child))
        return FALSE;

    *ContainerId = Child->m_ContainerId;
    return TRUE;
}

/* VM RESERVED ID ************************************************************/

/* The value's WDFSTRING is deleted here; if the copy fails the id stays empty and the PDO is not created */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubIdLoadVmReservedId(
    _In_ HubChild* Child)
{
    UNICODE_STRING Value;
    WDFSTRING String;
    PWCHAR Buffer;
    NTSTATUS Status;

    PAGED_CODE();

    if (Child->m_VmReservedId.Buffer != NULL)
        ExFreePoolWithTag(Child->m_VmReservedId.Buffer, HUB_TAG_DEVICE);
    RtlZeroMemory(&Child->m_VmReservedId, sizeof(Child->m_VmReservedId));

    Status = WdfStringCreate(NULL, WDF_NO_OBJECT_ATTRIBUTES, &String);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("VM reserved id string for device %p failed 0x%lx\n", Child, Status);
        return Status;
    }

    HubQueryUxdDeviceValue(Child, String);
    WdfStringGetUnicodeString(String, &Value);

    Buffer = (PWCHAR)ExAllocatePoolWithTag(NonPagedPool, Value.Length + sizeof(WCHAR), HUB_TAG_DEVICE);
    if (Buffer == NULL)
    {
        DPRINT1("VM reserved id for device %p could not be allocated\n", Child);
        WdfObjectDelete(String);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Buffer, Value.Length + sizeof(WCHAR));
    RtlCopyMemory(Buffer, Value.Buffer, Value.Length);

    Child->m_VmReservedId.Buffer = Buffer;
    Child->m_VmReservedId.Length = Value.Length + sizeof(WCHAR);
    Child->m_VmReservedId.MaximumLength = Child->m_VmReservedId.Length;

    WdfObjectDelete(String);
    return STATUS_SUCCESS;
}

VOID
NTAPI
HubIdFreeDeviceIds(
    _In_ HubChild* Child)
{
    if (Child->m_VmReservedId.Buffer != NULL)
        ExFreePoolWithTag(Child->m_VmReservedId.Buffer, HUB_TAG_DEVICE);

    RtlZeroMemory(&Child->m_VmReservedId, sizeof(Child->m_VmReservedId));
}

/* PDO IDS *******************************************************************/

/* The serial number is never truncated, whatever its length */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubIdAssignInstanceId(
    _In_ HubChild* Child,
    _In_ PWDFDEVICE_INIT DeviceInit)
{
    HubPort* Port = Child->m_Port;
    WCHAR Digits[HUB_INSTANCE_ID_CHARS];
    UNICODE_STRING Id;
    ULONG Number;
    NTSTATUS Status;

    if (Child->m_SerialNumber != NULL)
    {
        RtlInitUnicodeString(&Id, Child->m_SerialNumber);
        Status = WdfPdoInitAssignInstanceID(DeviceInit, &Id);
        if (!NT_SUCCESS(Status))
            DPRINT1("Instance id %wZ for device %p failed 0x%lx\n", &Id, Child, Status);

        return Status;
    }

    Number = Port->Number();
    if (Port->HasProperty(PortProperty::TypeCWithoutSwitch) &&
        Port->m_InstancePortNumber != 0 &&
        HubIdIsKnown(Child))
    {
        Number = Port->m_InstancePortNumber;
    }

    Id.Buffer = Digits;
    Id.Length = 0;
    Id.MaximumLength = sizeof(Digits);

    Status = RtlIntegerToUnicodeString(Number, 10, &Id);
    if (NT_SUCCESS(Status))
        Status = WdfPdoInitAssignInstanceID(DeviceInit, &Id);

    if (!NT_SUCCESS(Status))
        DPRINT1("Instance id from port %lu for device %p failed 0x%lx\n", Number, Child, Status);

    return Status;
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubIdAssignPdoIds(
    _In_ HubChild* Child,
    _In_ PWDFDEVICE_INIT DeviceInit)
{
    NTSTATUS Status;

    PAGED_CODE();

    HubSaveUxdState(Child->m_Hub, Child, NULL);

    if (Child->HasProperty(ChildProperty::UxdReserved))
    {
        Status = HubIdLoadVmReservedId(Child);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    Status = HubIdBuildDeviceId(Child, DeviceInit);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = HubIdBuildHardwareIds(Child, DeviceInit, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = HubIdBuildCompatibleIds(Child, DeviceInit, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    HubIdBuildContainerId(Child, DeviceInit);

    return HubIdAssignInstanceId(Child, DeviceInit);
}

/* DEVICE TEXT ***************************************************************/

/* A zero entry ends the device's language list */
BOOLEAN
NTAPI
HubIdLanguageSupported(
    _In_ HubChild* Child,
    _In_ USHORT Language)
{
    const USB_STRING_DESCRIPTOR* List = Child->m_LanguageIds;
    ULONG Count;
    ULONG Index;

    if (List == NULL || List->bLength <= 2)
        return FALSE;

    Count = (List->bLength - 2) / sizeof(WCHAR);
    for (Index = 0; Index < Count && List->bString[Index] != 0; Index++)
    {
        if (List->bString[Index] == Language)
            return TRUE;
    }

    return FALSE;
}

/* The device machine reads the product string in m_LanguageId and signals */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubIdReadProductString(
    _In_ HubChild* Child)
{
    KeClearEvent(&Child->m_QueryTextEvent);
    Child->Post(DsmEvent::DeviceTextQuery);
    HubWaitForPnpEvent(&Child->m_QueryTextEvent, "device text query", Child->m_Object);
}

_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubIdLocationText(
    _In_ HubChild* Child,
    _Inout_ PIRP Irp)
{
    PWCHAR Text;
    NTSTATUS Status;

    Text = (PWCHAR)ExAllocatePoolWithTag(NonPagedPool, HUB_LOCATION_TEXT_BYTES, HUB_TAG_DEVICE);
    if (Text == NULL)
    {
        DPRINT1("Location text for device %p could not be allocated\n", Child);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = RtlStringCbPrintfW(Text, HUB_LOCATION_TEXT_BYTES, L"Port_#%04d.Hub_#%04d",
                                Child->m_Port->Number(), Child->m_Hub->m_HubNumber);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Location text for device %p failed 0x%lx\n", Child, Status);
        ExFreePoolWithTag(Text, HUB_TAG_DEVICE);
        return Status;
    }

    Irp->IoStatus.Information = (ULONG_PTR)Text;
    return STATUS_SUCCESS;
}

/*
 * The "disable serial number" errata also hides the product string. A
 * language the device lacks, or a failed read, falls back to US English.
 */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubIdDescriptionText(
    _In_ HubChild* Child,
    _Inout_ PIRP Irp,
    _In_ LCID Locale)
{
    USHORT Language = LANGIDFROMLCID(Locale);
    PUSB_STRING_DESCRIPTOR Product;
    ULONG Characters;
    PWCHAR Text;

    if (Child->m_DeviceDescriptor.iProduct == 0 || Child->HasHack(ChildHack::DisableSerialNumber))
        return STATUS_NOT_SUPPORTED;

    Irp->IoStatus.Information = 0;

    if (Language == 0 || !HubIdLanguageSupported(Child, Language))
        Language = HUB_DEFAULT_LANGUAGE;

    if (Language != Child->m_LanguageId)
    {
        Child->m_LanguageId = Language;
        if (Child->m_ProductString != NULL)
        {
            ExFreePoolWithTag(Child->m_ProductString, HUB_TAG_DEVICE);
            Child->m_ProductString = NULL;
        }

        HubIdReadProductString(Child);
    }

    if (Child->m_ProductString == NULL)
    {
        if (Language == HUB_DEFAULT_LANGUAGE)
        {
            DPRINT("Device %p has no product string\n", Child);
            return STATUS_NOT_SUPPORTED;
        }

        Child->m_LanguageId = HUB_DEFAULT_LANGUAGE;
        HubIdReadProductString(Child);

        if (Child->m_ProductString == NULL)
        {
            DPRINT("Device %p has no product string\n", Child);
            return STATUS_NOT_SUPPORTED;
        }
    }

    Product = Child->m_ProductString;
    Characters = (Product->bLength - 2) / sizeof(WCHAR) + 1;

    Text = (PWCHAR)ExAllocatePoolWithTag(NonPagedPool, Characters * sizeof(WCHAR), HUB_TAG_DEVICE);
    if (Text == NULL)
    {
        DPRINT1("Description text for device %p could not be allocated\n", Child);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(Text, Product->bString, (Characters - 1) * sizeof(WCHAR));
    Text[Characters - 1] = UNICODE_NULL;

    Irp->IoStatus.Information = (ULONG_PTR)Text;
    return STATUS_SUCCESS;
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubIdQueryDeviceText(
    _In_ HubChild* Child,
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status;

    PAGED_CODE();

    switch (Stack->Parameters.QueryDeviceText.DeviceTextType)
    {
        case DeviceTextLocationInformation:
            Status = HubIdLocationText(Child, Irp);
            break;

        case DeviceTextDescription:
            Status = HubIdDescriptionText(Child, Irp, Stack->Parameters.QueryDeviceText.LocaleId);
            break;

        default:
            Status = STATUS_NOT_SUPPORTED;
            break;
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

/* SERIAL NUMBER *************************************************************/

/* Bytes the last control transfer of the device returned into the scratch buffer */
static
ULONG
NTAPI
HubIdBytesReturned(
    _In_ HubChild* Child)
{
    return min(Child->m_Control.Urb.TransferBufferLength, (ULONG)sizeof(Child->m_ScratchBuffer));
}

/* Keeps the two halves of a USB 3 hub, and UAS against BOT, on separate instance ids */
static
PCWSTR
NTAPI
HubIdSerialPrefix(
    _In_ HubChild* Child)
{
    if (Child->HasProperty(ChildProperty::IsHub))
        return (Child->m_DeviceDescriptor.bcdUSB >= 0x0300) ? L"MSFT30" : L"MSFT20";

    if (Child->HasState(ChildState::PrefixedSerial))
        return L"MSFT30";

    return NULL;
}

BOOLEAN
NTAPI
HubIdGetSerialNumberText(
    _In_ HubChild* Child,
    _Outptr_result_bytebuffer_(*Length) PCWSTR* Text,
    _Out_ PULONG Length)
{
    *Text = NULL;
    *Length = 0;

    if (Child->m_SerialNumber == NULL ||
        Child->m_SerialNumberLength < Child->m_SerialPrefixLength + sizeof(WCHAR))
    {
        return FALSE;
    }

    *Text = Child->m_SerialNumber + Child->m_SerialPrefixLength / sizeof(WCHAR);
    *Length = Child->m_SerialNumberLength - Child->m_SerialPrefixLength - sizeof(WCHAR);
    return TRUE;
}

BOOLEAN
DeviceMachine::IgnoreSerialNumber()
{
    return m_Device->HasHack(ChildHack::DisableSerialNumber);
}

BOOLEAN
DeviceMachine::SerialNumberIndexZero()
{
    return m_Device->m_DeviceDescriptor.iSerialNumber == 0;
}

/* Sized from the transfer length and the decoration written, not bLength, so a zeroed tail ends the string */
BOOLEAN
DeviceMachine::SerialNumberValid()
{
    HubChild* Child = m_Device;
    PUSB_STRING_DESCRIPTOR Descriptor = (PUSB_STRING_DESCRIPTOR)Child->m_ScratchBuffer;
    ULONG Returned = HubIdBytesReturned(Child);
    ULONG Length = Returned;
    HubDescContext Context;
    PCWSTR Prefix;
    ULONG PrefixBytes;
    PWCHAR Buffer;

    HubDescInitDeviceContext(&Context,
                             Child->m_DeviceDescriptor.bcdUSB,
                             Child->Speed(),
                             Child->HasHack(ChildHack::UseWin8DescriptorValidation),
                             Child->m_Port->m_Info.SspIsochBurstCount,
                             Child->m_ValidationBits);

    if (!HubDescCheckSerialNumber(&Context, Descriptor, &Length) || Length < sizeof(WCHAR))
    {
        DPRINT1("Device %p serial number string is invalid (%lu bytes)\n", Child, Returned);
        Child->m_EnumMessageId = HUB_ENUM_BAD_SERIAL_NUMBER;
        return FALSE;
    }

    if (Child->m_SerialNumber != NULL)
    {
        ExFreePoolWithTag(Child->m_SerialNumber, HUB_TAG_DEVICE);
        Child->m_SerialNumber = NULL;
    }

    Prefix = HubIdSerialPrefix(Child);
    PrefixBytes = (Prefix != NULL) ? HUB_SERIAL_PREFIX_BYTES : 0;

    Buffer = (PWCHAR)ExAllocatePoolWithTag(NonPagedPool, Returned + PrefixBytes, HUB_TAG_DEVICE);
    if (Buffer == NULL)
    {
        DPRINT1("Device %p serial number could not be allocated\n", Child);
        return FALSE;
    }

    RtlZeroMemory(Buffer, Returned + PrefixBytes);
    if (Prefix != NULL)
        RtlCopyMemory(Buffer, Prefix, PrefixBytes);

    /* Length is bLength now: the string bytes plus room for the NUL */
    RtlCopyMemory((PUCHAR)Buffer + PrefixBytes, Descriptor->bString, Length - sizeof(WCHAR));

    Child->m_SerialNumber = Buffer;
    Child->m_SerialNumberLength = Returned + PrefixBytes;
    Child->m_SerialPrefixLength = PrefixBytes;
    return TRUE;
}

VOID
DeviceMachine::DropSerialNumber()
{
    if (m_Device->m_SerialNumber == NULL)
        return;

    ExFreePoolWithTag(m_Device->m_SerialNumber, HUB_TAG_DEVICE);
    m_Device->m_SerialNumber = NULL;
}

BOOLEAN
DeviceMachine::SerialNumberCompared()
{
    if (m_Device->m_SerialNumber == NULL)
        return FALSE;

    return m_Device->m_DeviceDescriptor.bcdUSB > 0x0200 || IsBootDevice();
}

/* Lengths come from transfers, so different padding between reads or a short length does not match */
BOOLEAN
DeviceMachine::SameSerialNumber()
{
    HubChild* Child = m_Device;
    PUSB_STRING_DESCRIPTOR Descriptor = (PUSB_STRING_DESCRIPTOR)Child->m_ScratchBuffer;
    ULONG Returned = HubIdBytesReturned(Child);
    PCWSTR Stored;
    ULONG StoredLength;

    if (!HubIdGetSerialNumberText(Child, &Stored, &StoredLength) || Returned < 2)
        return FALSE;

    if (StoredLength == Returned - 2 &&
        RtlCompareMemory(Descriptor->bString, Stored, StoredLength) == StoredLength)
    {
        return TRUE;
    }

    DPRINT1("Device %p came back with a different serial number\n", Child);
    return FALSE;
}
