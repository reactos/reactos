/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Checks and caching of the descriptors read from a child device
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"
#include "devreq.h"

#define NDEBUG
#include <debug.h>

/* Bytes of the device descriptor the first read keeps */
#define DESC_FIRST_READ_KEEP        8

/* A flash drive that must not use fast enumeration */
#define DESC_SLOW_ENUM_VENDOR       0x13FE
#define DESC_SLOW_ENUM_PRODUCT      0x5200

/* Interface association class codes of a composite device */
#define DESC_CLASS_MISCELLANEOUS    0xEF
#define DESC_SUBCLASS_COMMON        0x02
#define DESC_PROTOCOL_IAD           0x01

/* USB Attached SCSI interface */
#define DESC_SUBCLASS_SCSI          0x06
#define DESC_PROTOCOL_UAS           0x62

/* Two configured bits per alternate mode in bmConfigured; 3 means configured */
#define DESC_MODE_CONFIGURED        3

VOID
NTAPI
HubDeviceDescContext(
    _In_ HubChild* Child,
    _Out_ HubDescContext* Context)
{
    HubDescInitDeviceContext(Context,
                             Child->m_DeviceDescriptor.bcdUSB,
                             Child->Speed(),
                             Child->HasHack(ChildHack::UseWin8DescriptorValidation),
                             Child->m_Port->m_Info.SspIsochBurstCount,
                             Child->m_ValidationBits);

    /* Only names the device in the validator's debug prints */
    Context->LogContext = Child;
}

static
VOID
NTAPI
HubSetSqmFlag(
    _In_ HubChild* Child,
    _In_ ChildSqm Flag)
{
    InterlockedOr(&Child->m_SqmFlags, (LONG)Flag);
}

/* Device descriptor */

/*
 * VID and PID stay from the last full read. QUIRK: the VID/PID errata match
 * reads the scratch buffer even when fewer than 12 bytes came back.
 */
BOOLEAN
DeviceMachine::FirstDescriptorValid()
{
    HubChild* Child = m_Device;
    PUSB_DEVICE_DESCRIPTOR Descriptor = (PUSB_DEVICE_DESCRIPTOR)Child->m_ScratchBuffer;
    ULONG Bytes = HubDeviceBytesReturned(Child);
    HubDescContext Context;

    if (Bytes < DESC_FIRST_READ_KEEP)
    {
        DPRINT1("Device %p first device descriptor read returned %lu bytes\n", Child, Bytes);
        Child->m_EnumMessageId = (Bytes == 0) ? HUB_ENUM_DEVICE_DESCRIPTOR_FAILED : HUB_ENUM_BAD_DEVICE_DESCRIPTOR;
        return FALSE;
    }

    HubDeviceDescContext(Child, &Context);
    if (!HubDescCheckDevice(&Context, Descriptor, Bytes, NULL))
    {
        Child->m_EnumMessageId = HUB_ENUM_BAD_DEVICE_DESCRIPTOR;
        return FALSE;
    }

    Child->m_MaxPacketSize0 = Descriptor->bMaxPacketSize0;
    RtlCopyMemory(&Child->m_DeviceDescriptor, Descriptor, DESC_FIRST_READ_KEEP);

    if (Descriptor->idVendor == DESC_SLOW_ENUM_VENDOR && Descriptor->idProduct == DESC_SLOW_ENUM_PRODUCT)
        Child->m_Hacks |= (ULONG)ChildHack::AlwaysSecondReset;

    return TRUE;
}

BOOLEAN
DeviceMachine::DeviceDescriptorValid()
{
    HubChild* Child = m_Device;
    PUSB_DEVICE_DESCRIPTOR Descriptor = (PUSB_DEVICE_DESCRIPTOR)Child->m_ScratchBuffer;
    ULONG Bytes = HubDeviceBytesReturned(Child);
    BOOLEAN IsBillboard = FALSE;
    HubDescContext Context;

    if (Bytes != sizeof(*Descriptor))
    {
        DPRINT1("Device %p device descriptor read returned %lu bytes\n", Child, Bytes);
        Child->m_EnumMessageId = (Bytes == 0) ? HUB_ENUM_DEVICE_DESCRIPTOR_FAILED : HUB_ENUM_BAD_DEVICE_DESCRIPTOR;
        return FALSE;
    }

    HubDeviceDescContext(Child, &Context);
    if (!HubDescCheckDevice(&Context, Descriptor, Bytes, &IsBillboard))
    {
        Child->m_EnumMessageId = HUB_ENUM_BAD_DEVICE_DESCRIPTOR;
        return FALSE;
    }

    RtlCopyMemory(&Child->m_DeviceDescriptor, Descriptor, sizeof(*Descriptor));

    if (IsBillboard)
        Child->SetProperty(ChildProperty::HasBillboard);

    return TRUE;
}

/* Configuration descriptor */

BOOLEAN
DeviceMachine::ConfigLongerThanRead()
{
    PUSB_CONFIGURATION_DESCRIPTOR Header = (PUSB_CONFIGURATION_DESCRIPTOR)m_Device->m_ScratchBuffer;

    return Header->bDescriptorType == USB_CONFIGURATION_DESCRIPTOR_TYPE &&
           HubDeviceBytesReturned(m_Device) < Header->wTotalLength;
}

static
BOOLEAN
NTAPI
HubIsCompositeLayout(
    _In_ const USB_DEVICE_DESCRIPTOR* Device,
    _In_ const USB_CONFIGURATION_DESCRIPTOR* Config)
{
    if (Device->bNumConfigurations != 1 || Config->bNumInterfaces <= 1)
        return FALSE;

    if (Device->bDeviceClass == 0)
        return TRUE;

    return Device->bDeviceClass == DESC_CLASS_MISCELLANEOUS &&
           Device->bDeviceSubClass == DESC_SUBCLASS_COMMON &&
           Device->bDeviceProtocol == DESC_PROTOCOL_IAD;
}

/*
 * Picks the interface for the compatible ids. A hub or a fast enough UAS device
 * also gets a decorated serial number to keep its instance ids distinct.
 */
static
VOID
NTAPI
HubPickCompatIdInterface(
    _In_ HubChild* Child,
    _In_ PUSB_CONFIGURATION_DESCRIPTOR Config)
{
    PUSB_INTERFACE_DESCRIPTOR Interface;
    PUSB_INTERFACE_DESCRIPTOR Uas;
    USB_DEVICE_SPEED Speed = Child->Speed();
    BOOLEAN FastEnough;

    Interface = HubDescFindInterface(Config, Config, -1, -1, -1, -1, -1, NULL);
    Child->m_CompatIdInterface = Interface;
    if (Interface == NULL)
        return;

    if (Interface->bInterfaceClass == USB_DEVICE_CLASS_HUB)
    {
        Child->SetProperty(ChildProperty::IsHub);
        Child->SetState(ChildState::PrefixedSerial);
        return;
    }

    /* QUIRK: only UsbSuperSpeed qualifies, not a SuperSpeedPlus speed value */
    FastEnough = (Speed == UsbSuperSpeed && Child->m_Hub->m_StreamsSupported) || Speed == UsbHighSpeed;

    if (Child->HasProperty(ChildProperty::IsComposite) ||
        Child->m_DeviceDescriptor.bcdUSB <= 0x0200 ||
        Child->HasHack(ChildHack::DisableUasp) ||
        !FastEnough)
    {
        return;
    }

    Uas = HubDescFindInterface(Config,
                               Config,
                               Interface->bInterfaceNumber,
                               -1,
                               USB_DEVICE_CLASS_STORAGE,
                               DESC_SUBCLASS_SCSI,
                               DESC_PROTOCOL_UAS,
                               NULL);
    if (Uas != NULL)
    {
        Child->SetState(ChildState::PrefixedSerial);
        Child->m_CompatIdInterface = Uas;
    }
}

/* QUIRK: when the set fit in the first read the length check compares the header with itself */
BOOLEAN
DeviceMachine::ConfigDescriptorValid()
{
    HubChild* Child = m_Device;
    PUSB_CONFIGURATION_DESCRIPTOR Header = (PUSB_CONFIGURATION_DESCRIPTOR)Child->m_ScratchBuffer;
    PUSB_CONFIGURATION_DESCRIPTOR Config = Child->m_ConfigDescriptor ? Child->m_ConfigDescriptor : Header;
    ULONG Bytes = HubDeviceBytesReturned(Child);
    BOOLEAN Streams = FALSE;
    HubDescContext Context;

    HubDeviceDescContext(Child, &Context);

    if (!HubDescCheckConfiguration(&Context, Config, Bytes, &Streams))
    {
        Child->m_EnumMessageId = HUB_ENUM_BAD_CONFIG_DESCRIPTOR;
        return FALSE;
    }

    if (Config->wTotalLength != Header->wTotalLength)
    {
        HubDescLog(&Context, HubDescCode::ConfigTotalLengthTooLarge, FALSE);
        Child->m_EnumMessageId = HUB_ENUM_BAD_CONFIG_DESCRIPTOR;
        return FALSE;
    }

    Child->SetState(ChildState::ConfigurationValid);

    if (Streams)
        HubSetSqmFlag(Child, ChildSqm::SupportsStreams);
    if (Child->m_DeviceDescriptor.bNumConfigurations > 1)
        HubSetSqmFlag(Child, ChildSqm::MultipleConfigurations);

    if (HubIsCompositeLayout(&Child->m_DeviceDescriptor, Config))
        Child->SetProperty(ChildProperty::IsComposite);

    if (Child->m_ConfigDescriptor == NULL)
    {
        /* A lax USB 2.0 device may report a wTotalLength shorter than the header; allocate at least a header */
        ULONG Size = max((ULONG)Header->wTotalLength, (ULONG)sizeof(*Config));

        Config = (PUSB_CONFIGURATION_DESCRIPTOR)ExAllocatePoolWithTag(NonPagedPool, Size, HUB_TAG_DEVICE);
        if (Config == NULL)
        {
            DPRINT1("Device %p no memory for a %u byte configuration\n", Child, Header->wTotalLength);
            return FALSE;
        }

        RtlCopyMemory(Config, Header, min(Size, (ULONG)sizeof(Child->m_ScratchBuffer)));
        Child->m_ConfigDescriptor = Config;
    }

    HubPickCompatIdInterface(Child, Config);

    /* Composite hubs are not supported; enumeration goes on anyway */
    if (Child->HasProperty(ChildProperty::IsHub) && Child->HasProperty(ChildProperty::IsComposite))
        DPRINT1("Device %p is a composite hub\n", Child);

    if (Config->bmAttributes & USB_CONFIG_REMOTE_WAKEUP)
        Child->SetProperty(ChildProperty::RemoteWakeCapable);

    if (Child->HasHack(ChildHack::BlockedDevice))
    {
        DPRINT1("Device %p is marked incompatible, enumeration fails\n", Child);
        Child->m_EnumMessageId = HUB_ENUM_INCOMPATIBLE;
        return FALSE;
    }

    return TRUE;
}

/* BOS descriptor */

BOOLEAN
DeviceMachine::BosQuerySkipped()
{
    return m_Device->HasHack(ChildHack::SkipBosQuery) || m_Device->m_DeviceDescriptor.bcdUSB <= 0x0200;
}

/* Optional descriptors only fail the enumeration of a device newer than USB 2.0 */
BOOLEAN
DeviceMachine::IgnoreDescriptorError()
{
    return m_Device->m_DeviceDescriptor.bcdUSB <= 0x0200;
}

BOOLEAN
DeviceMachine::BosHeaderValid()
{
    HubChild* Child = m_Device;
    ULONG Bytes = HubDeviceBytesReturned(Child);
    HubDescContext Context;

    if (Bytes < sizeof(USB_BOS_DESCRIPTOR))
    {
        DPRINT1("Device %p BOS header read returned %lu bytes\n", Child, Bytes);
        Child->m_EnumMessageId = HUB_ENUM_BAD_BOS;
        return FALSE;
    }

    HubDeviceDescContext(Child, &Context);
    if (!HubDescCheckBosHeader(&Context, (PUSB_BOS_DESCRIPTOR)Child->m_ScratchBuffer, Bytes))
    {
        Child->m_EnumMessageId = HUB_ENUM_BAD_BOS;
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
DeviceMachine::BosComplete()
{
    PUSB_BOS_DESCRIPTOR Header = (PUSB_BOS_DESCRIPTOR)m_Device->m_ScratchBuffer;

    return HubDeviceBytesReturned(m_Device) >= Header->wTotalLength;
}

/* Alternate modes that fit in the capability, whatever bNumberOfAlternateModes claims */
static
UCHAR
NTAPI
HubBillboardModeCount(
    _In_ PUSB_DEVICE_CAPABILITY_BILLBOARD_DESCRIPTOR Caps)
{
    ULONG First = FIELD_OFFSET(USB_DEVICE_CAPABILITY_BILLBOARD_DESCRIPTOR, AlternateMode);
    ULONG Room;

    if (Caps->bLength <= First)
        return 0;

    Room = (Caps->bLength - First) / sizeof(Caps->AlternateMode[0]);
    return (UCHAR)min(Room, (ULONG)Caps->bNumberOfAlternateModes);
}

VOID
NTAPI
HubFreeBillboardInfo(
    _In_ HubChild* Child)
{
    HubBillboardInfo* Billboard = Child->m_Billboard;

    if (Billboard == NULL)
        return;

    if (Billboard->Descriptor != NULL)
        ExFreePoolWithTag(Billboard->Descriptor, HUB_TAG_HUB);

    ExFreePoolWithTag(Billboard, HUB_TAG_HUB);
    Child->m_Billboard = NULL;
}

/* An allocation failure leaves the device without billboard data; enumeration goes on */
static
VOID
NTAPI
HubKeepBillboardData(
    _In_ HubChild* Child,
    _In_ PUSB_DEVICE_CAPABILITY_BILLBOARD_DESCRIPTOR Caps)
{
    HubBillboardInfo* Billboard;
    ULONG Size;

    HubFreeBillboardInfo(Child);

    Billboard = (HubBillboardInfo*)ExAllocatePoolWithTag(NonPagedPool, sizeof(*Billboard), HUB_TAG_HUB);
    if (Billboard == NULL)
    {
        DPRINT1("Device %p no memory for the billboard record\n", Child);
        return;
    }

    RtlZeroMemory(Billboard, sizeof(*Billboard));
    Billboard->CurrentMode = HUB_BILLBOARD_NO_MODE;

    /* Readers walk every mode the descriptor claims, which may run past bLength */
    Size = FIELD_OFFSET(USB_DEVICE_CAPABILITY_BILLBOARD_DESCRIPTOR, AlternateMode) +
           Caps->bNumberOfAlternateModes * sizeof(Caps->AlternateMode[0]);
    Size = max(Size, (ULONG)Caps->bLength);

    Billboard->Descriptor = (PUSB_DEVICE_CAPABILITY_BILLBOARD_DESCRIPTOR)ExAllocatePoolWithTag(NonPagedPool,
                                                                                               Size,
                                                                                               HUB_TAG_HUB);
    if (Billboard->Descriptor == NULL)
    {
        DPRINT1("Device %p no memory for a %lu byte billboard copy\n", Child, Size);
        ExFreePoolWithTag(Billboard, HUB_TAG_HUB);
        return;
    }

    RtlZeroMemory(Billboard->Descriptor, Size);
    RtlCopyMemory(Billboard->Descriptor, Caps, Caps->bLength);
    Child->m_Billboard = Billboard;
}

/* Cached once per device and never refreshed. The count is bounded by the capability length. */
static
BOOLEAN
NTAPI
HubCacheSublinkSpeeds(
    _In_ HubChild* Child,
    _In_ PUSB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_USB_DESCRIPTOR Caps)
{
    ULONG First = FIELD_OFFSET(USB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_USB_DESCRIPTOR, bmSublinkSpeedAttr);
    ULONG Count = Caps->bmAttributes.SublinkSpeedAttrCount + 1;
    ULONG Room = (Caps->bLength > First) ? (Caps->bLength - First) / sizeof(Caps->bmSublinkSpeedAttr[0]) : 0;
    ULONG Size;

    Count = min(Count, Room);
    if (Count == 0)
        return TRUE;

    Size = Count * sizeof(Caps->bmSublinkSpeedAttr[0]);
    Child->m_SublinkSpeedAttr = (PUSB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_SPEED)ExAllocatePoolWithTag(NonPagedPool,
                                                                                                   Size,
                                                                                                   HUB_TAG_DEVICE);
    if (Child->m_SublinkSpeedAttr == NULL)
    {
        DPRINT1("Device %p no memory for %lu sublink speeds\n", Child, Count);
        return FALSE;
    }

    RtlCopyMemory(Child->m_SublinkSpeedAttr, Caps->bmSublinkSpeedAttr, Size);
    Child->m_SublinkSpeedAttrCount = Count;
    return TRUE;
}

/* There is no partner notification yet, so this only logs */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubPublishDualRolePartner(
    _In_ HubPort* Port,
    _In_ BOOLEAN Attached,
    _In_ ULONG Features)
{
    DPRINT("Port %p dual role partner %u features 0x%lx\n", Port, Attached, Features);
}

_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubCacheDualRole(
    _In_ HubChild* Child,
    _In_ const HubPlatformFeatures* Features)
{
    Child->SetProperty(ChildProperty::DualRole);
    Child->m_DualRoleRequestCode = Features->VendorCommand;
    HubQueryLocalDualRoleFeatures(&Child->m_DualRoleLocalFeatures);
    Child->m_DualRolePartnerFeatures = Features->Features;
    HubPublishDualRolePartner(Child->m_Port, TRUE, Child->m_DualRolePartnerFeatures);
}

/*
 * The set info is cached once and survives the BOS reread after the alternate
 * enumeration command. A command cached in usbflags is not requested again.
 */
static
VOID
NTAPI
HubCacheMsOs20SetInfoFromBos(
    _In_ HubChild* Child,
    _In_ const HubMsOs20SetInfo* SetInfo)
{
    RtlCopyMemory(&Child->m_MsOs20SetInfo, SetInfo, sizeof(Child->m_MsOs20SetInfo));
    Child->m_MsOs20.Flags |= HUB_MSOS20_SET_INFO;

    if (Child->m_MsOs20SetInfo.bAltEnumCode != 0 && !Child->m_AltEnumCached)
    {
        Child->m_MsOsVendorCode = Child->m_MsOs20SetInfo.bVendorCode;
        Child->m_MsOs20.Flags |= HUB_MSOS20_ALT_ENUM;
    }
}


/* TRUE when Length bytes at Start lie inside the bytes the device returned */
static
BOOLEAN
NTAPI
HubBosSpanFits(
    _In_ const VOID* Start,
    _In_ ULONG Length,
    _In_ const UCHAR* End)
{
    return (const UCHAR*)Start + Length <= End;
}

/* The cache routines read each capability in full, so drop any that run past the buffer */
static
VOID
NTAPI
HubDropCapsPastEnd(
    _Inout_ HubBosInfo* Info,
    _In_ const UCHAR* End)
{
    if (Info->SuperSpeedPlus != NULL &&
        !HubBosSpanFits(Info->SuperSpeedPlus,
                        max((ULONG)Info->SuperSpeedPlus->bLength,
                            (ULONG)FIELD_OFFSET(USB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_USB_DESCRIPTOR, bmSublinkSpeedAttr)),
                        End))
    {
        DPRINT1("SuperSpeedPlus capability runs past the BOS, ignored\n");
        Info->SuperSpeedPlus = NULL;
    }

    if (Info->Billboard != NULL &&
        !HubBosSpanFits(Info->Billboard,
                        max((ULONG)Info->Billboard->bLength,
                            (ULONG)FIELD_OFFSET(USB_DEVICE_CAPABILITY_BILLBOARD_DESCRIPTOR, AlternateMode)),
                        End))
    {
        DPRINT1("Billboard capability runs past the BOS, ignored\n");
        Info->Billboard = NULL;
    }

    if (Info->ContainerId != NULL && !HubBosSpanFits(Info->ContainerId, sizeof(*Info->ContainerId), End))
    {
        DPRINT1("Container id capability runs past the BOS, ignored\n");
        Info->ContainerId = NULL;
    }

    if (Info->PlatformFeatures != NULL && !HubBosSpanFits(Info->PlatformFeatures, sizeof(*Info->PlatformFeatures), End))
    {
        DPRINT1("Platform features capability runs past the BOS, ignored\n");
        Info->PlatformFeatures = NULL;
    }

    if (Info->MsOs20SetInfo != NULL && !HubBosSpanFits(Info->MsOs20SetInfo, sizeof(*Info->MsOs20SetInfo), End))
    {
        DPRINT1("MS OS 2.0 set info runs past the BOS, ignored\n");
        Info->MsOs20SetInfo = NULL;
    }
}

/* QUIRK: with the ignore errata a failed walk still feeds its partial results */
BOOLEAN
DeviceMachine::BosValid()
{
    HubChild* Child = m_Device;
    PUSB_BOS_DESCRIPTOR Header = (PUSB_BOS_DESCRIPTOR)Child->m_ScratchBuffer;
    PUSB_BOS_DESCRIPTOR Bos = Child->m_Bos ? Child->m_Bos : Header;
    BOOLEAN SetInfoKnown = (Child->m_MsOs20.Flags & HUB_MSOS20_SET_INFO) != 0;
    HubDescContext Context;
    HubBosInfo Info;
    HubLpmInfo Lpm;
    BOOLEAN Ltm = FALSE;
    ULONG Flags = 0;
    BOOLEAN Valid;

    PAGED_CODE();

    RtlZeroMemory(&Lpm, sizeof(Lpm));
    HubDeviceDescContext(Child, &Context);

    Valid = HubDescCheckBos(&Context,
                            Bos,
                            HubDeviceBytesReturned(Child),
                            &Info,
                            &Child->m_U1ExitLatency,
                            &Child->m_U2ExitLatency,
                            &Ltm,
                            &Lpm,
                            &Flags);
    if (!Valid && !Child->HasHack(ChildHack::IgnoreBosValidationFailure))
    {
        Child->m_EnumMessageId = HUB_ENUM_BAD_BOS;
        return FALSE;
    }

    if (!Valid)
        DPRINT1("Device %p BOS is invalid, kept because of its errata\n", Child);

    HubDropCapsPastEnd(&Info, (const UCHAR*)Bos + HubDeviceBytesReturned(Child));

    if (Ltm)
        Child->SetProperty(ChildProperty::LtmCapable);
    if (Lpm.Capable)
        Child->SetProperty(ChildProperty::Usb20LpmCapable);
    if (Lpm.BeslAndAltHird)
        Child->SetProperty(ChildProperty::BeslCapable);
    if (Flags & HUB_BOS_CHARGING_POLICY)
        Child->SetProperty(ChildProperty::ChargingPolicySupported);

    if (Info.SuperSpeedPlus != NULL && Child->m_SublinkSpeedAttr == NULL)
    {
        if (!HubCacheSublinkSpeeds(Child, Info.SuperSpeedPlus))
            return FALSE;
    }

    /* An MS OS container id wins over the BOS one */
    if (Info.ContainerId != NULL)
    {
        Child->SetProperty(ChildProperty::ContainerIdFromBos);
        if (!Child->HasProperty(ChildProperty::ContainerIdKnown))
            RtlCopyMemory(&Child->m_ContainerId, Info.ContainerId->ContainerID, sizeof(Child->m_ContainerId));
    }

    if (Info.Billboard != NULL)
        HubKeepBillboardData(Child, Info.Billboard);

    if (Info.PlatformFeatures != NULL)
        HubCacheDualRole(Child, Info.PlatformFeatures);

    if (!SetInfoKnown && Info.MsOs20SetInfo != NULL)
        HubCacheMsOs20SetInfoFromBos(Child, Info.MsOs20SetInfo);

    if (Child->m_Bos == NULL)
    {
        Bos = (PUSB_BOS_DESCRIPTOR)ExAllocatePoolWithTag(NonPagedPool, Header->wTotalLength, HUB_TAG_DEVICE);
        if (Bos == NULL)
        {
            DPRINT1("Device %p no memory for a %u byte BOS\n", Child, Header->wTotalLength);
            return FALSE;
        }

        RtlCopyMemory(Bos, Header, min(Header->wTotalLength, (ULONG)sizeof(Child->m_ScratchBuffer)));
        Child->m_Bos = Bos;
    }

    if (Flags & HUB_BOS_SUPERSPEED_CAPABLE)
        HubSetSqmFlag(Child, ChildSqm::SuperSpeedCapable);
    if (Valid)
        HubSetSqmFlag(Child, ChildSqm::ValidBos);

    return TRUE;
}

/* Billboard */

BOOLEAN
DeviceMachine::HasBillboard()
{
    return m_Device->m_Billboard != NULL;
}

BOOLEAN
DeviceMachine::BillboardStringWanted()
{
    HubBillboardInfo* Billboard = m_Device->m_Billboard;

    if (Billboard == NULL)
        return FALSE;

    if (Billboard->Descriptor->iAddtionalInfoURL != 0)
        return TRUE;

    DPRINT("Device %p billboard has no URL string\n", m_Device);
    return FALSE;
}

BOOLEAN
DeviceMachine::BillboardStringValid()
{
    HubChild* Child = m_Device;
    HubDescContext Context;

    HubDeviceDescContext(Child, &Context);
    if (!HubDescCheckString(&Context,
                            (PUSB_STRING_DESCRIPTOR)Child->m_ScratchBuffer,
                            HubDeviceBytesReturned(Child),
                            NULL))
    {
        Child->m_EnumMessageId = HUB_ENUM_BAD_BILLBOARD_URL;
        return FALSE;
    }

    DPRINT("Device %p billboard URL string read\n", Child);
    return TRUE;
}

/* The PDO publishes whether any mode failed or succeeded to configure */
static
VOID
NTAPI
HubNoteAltMode(
    _In_ HubBillboardInfo* Billboard)
{
    UCHAR Mode = Billboard->CurrentMode;
    UCHAR Configured = (Billboard->Descriptor->bmConfigured[Mode / 4] >> ((Mode % 4) * 2)) & 3;

    DPRINT("Billboard %p alternate mode %u configuration state %u\n", Billboard, Mode, Configured);

    if (Configured == DESC_MODE_CONFIGURED)
        Billboard->ModeSuccess = TRUE;
    else
        Billboard->ModeError = TRUE;
}

/* Modes without a string are noted and skipped */
BOOLEAN
DeviceMachine::AltModeStringWanted()
{
    HubBillboardInfo* Billboard = m_Device->m_Billboard;
    UCHAR Count;

    if (Billboard == NULL)
        return FALSE;

    Count = HubBillboardModeCount(Billboard->Descriptor);

    if (Billboard->CurrentMode == HUB_BILLBOARD_NO_MODE)
        Billboard->CurrentMode = 0;

    while (Billboard->CurrentMode < Count &&
           Billboard->Descriptor->AlternateMode[Billboard->CurrentMode].iAlternateModeSetting == 0)
    {
        HubNoteAltMode(Billboard);
        Billboard->CurrentMode++;
    }

    if (Billboard->CurrentMode >= Count)
    {
        Billboard->CurrentMode = HUB_BILLBOARD_NO_MODE;
        return FALSE;
    }

    return TRUE;
}

/* QUIRK: the walk moves to the next mode even when this string is bad */
BOOLEAN
DeviceMachine::AltModeStringValid()
{
    HubChild* Child = m_Device;
    HubBillboardInfo* Billboard = Child->m_Billboard;
    HubDescContext Context;
    BOOLEAN Valid;

    if (Billboard == NULL || Billboard->CurrentMode == HUB_BILLBOARD_NO_MODE)
    {
        DPRINT1("Device %p alternate mode string check without a current mode\n", Child);
        return FALSE;
    }

    HubDeviceDescContext(Child, &Context);
    Valid = HubDescCheckString(&Context,
                               (PUSB_STRING_DESCRIPTOR)Child->m_ScratchBuffer,
                               HubDeviceBytesReturned(Child),
                               NULL);
    if (Valid)
        HubNoteAltMode(Billboard);
    else
        Child->m_EnumMessageId = HUB_ENUM_BAD_ALT_MODE_STRING;

    Billboard->CurrentMode++;
    return Valid;
}

/* Strings */

/* Copies the validated string out of the scratch buffer; allocation failure sets no reason */
static
BOOLEAN
NTAPI
HubCacheString(
    _In_ HubChild* Child,
    _In_ ULONG BadReason,
    _Outptr_result_maybenull_ PUSB_STRING_DESCRIPTOR* Cache)
{
    PUSB_STRING_DESCRIPTOR String = (PUSB_STRING_DESCRIPTOR)Child->m_ScratchBuffer;
    ULONG Bytes = HubDeviceBytesReturned(Child);
    HubDescContext Context;
    PUSB_STRING_DESCRIPTOR Copy;

    *Cache = NULL;

    HubDeviceDescContext(Child, &Context);
    if (!HubDescCheckString(&Context, String, Bytes, NULL))
    {
        Child->m_EnumMessageId = BadReason;
        return FALSE;
    }

    Copy = (PUSB_STRING_DESCRIPTOR)ExAllocatePoolWithTag(NonPagedPool, String->bLength, HUB_TAG_DEVICE);
    if (Copy == NULL)
    {
        DPRINT1("Device %p no memory for a %u byte string\n", Child, String->bLength);
        return FALSE;
    }

    RtlZeroMemory(Copy, String->bLength);
    RtlCopyMemory(Copy, String, min((ULONG)String->bLength, Bytes));
    *Cache = Copy;
    return TRUE;
}

BOOLEAN
DeviceMachine::LanguageIdsValid()
{
    return HubCacheString(m_Device, HUB_ENUM_BAD_LANGUAGE_IDS, &m_Device->m_LanguageIds);
}

BOOLEAN
DeviceMachine::ProductNameValid()
{
    return HubCacheString(m_Device, HUB_ENUM_BAD_PRODUCT_STRING, &m_Device->m_ProductString);
}

/* The content is not checked; its presence alone marks a high speed capable device */
BOOLEAN
DeviceMachine::QualifierValid()
{
    if (HubDeviceBytesReturned(m_Device) < sizeof(m_Device->m_Qualifier))
    {
        DPRINT1("Device %p qualifier read returned %lu bytes\n", m_Device, HubDeviceBytesReturned(m_Device));
        m_Device->m_EnumMessageId = HUB_ENUM_BAD_QUALIFIER;
        return FALSE;
    }

    m_Device->SetProperty(ChildProperty::HighSpeedCapable);
    return TRUE;
}

/* MS OS 1.0 */

BOOLEAN
DeviceMachine::MsOsDescriptorValid()
{
    HubChild* Child = m_Device;
    HubDescContext Context;

    if (HubDeviceBytesReturned(Child) != sizeof(Child->m_MsOsString))
    {
        DPRINT1("Device %p MS OS string read returned %lu bytes\n", Child, HubDeviceBytesReturned(Child));
        return FALSE;
    }

    HubDeviceDescContext(Child, &Context);
    if (!HubDescCheckMsOsString(&Context, &Child->m_MsOsString))
        return FALSE;

    Child->m_MsOsVendorCode = Child->m_MsOsString.bVendorCode;
    return TRUE;
}

/* Nothing is cached from the header */
BOOLEAN
DeviceMachine::ContainerIdHeaderValid()
{
    HubChild* Child = m_Device;
    HubDescContext Context;

    HubDeviceDescContext(Child, &Context);
    if (!HubDescCheckMsOsContainerIdHeader(&Context,
                                           (const HubMsOsHeader*)Child->m_ScratchBuffer,
                                           HubDeviceBytesReturned(Child)))
    {
        Child->m_EnumMessageId = HUB_ENUM_BAD_MSOS_CONTAINER_ID;
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
DeviceMachine::ContainerIdKnown()
{
    HubChild* Child = m_Device;
    const HubMsOsContainerId* Descriptor = (const HubMsOsContainerId*)Child->m_ScratchBuffer;
    HubDescContext Context;

    HubDeviceDescContext(Child, &Context);
    if (!HubDescCheckMsOsContainerId(&Context, Descriptor, HubDeviceBytesReturned(Child)))
        return FALSE;

    RtlCopyMemory(&Child->m_ContainerId, Descriptor->ContainerId, sizeof(Child->m_ContainerId));
    Child->SetProperty(ChildProperty::ContainerIdKnown);
    return TRUE;
}

BOOLEAN
DeviceMachine::ExtendedConfigHeaderValid()
{
    HubChild* Child = m_Device;
    HubDescContext Context;

    HubDeviceDescContext(Child, &Context);
    if (!HubDescCheckMsOsExtConfigHeader(&Context,
                                         (const HubMsOsExtConfigHeader*)Child->m_ScratchBuffer,
                                         HubDeviceBytesReturned(Child)))
    {
        Child->m_EnumMessageId = HUB_ENUM_BAD_MSOS_EXT_CONFIG;
        return FALSE;
    }

    return TRUE;
}

/* The header read is still in the scratch buffer; a bad descriptor is dropped */
BOOLEAN
DeviceMachine::ExtendedConfigValid()
{
    HubChild* Child = m_Device;
    const HubMsOsExtConfigHeader* Header = (const HubMsOsExtConfigHeader*)Child->m_ScratchBuffer;
    ULONG Bytes = HubDeviceBytesReturned(Child);
    HubDescContext Context;

    if (Child->m_MsOsExtConfig == NULL)
    {
        DPRINT1("Device %p has no extended configuration buffer\n", Child);
        return FALSE;
    }

    HubDeviceDescContext(Child, &Context);
    if (Bytes == Header->dwLength && HubDescCheckMsOsExtConfig(&Context, Child->m_MsOsExtConfig, Bytes))
        return TRUE;

    if (Bytes != Header->dwLength)
        DPRINT1("Device %p extended configuration read %lu of %lu bytes\n", Child, Bytes, Header->dwLength);

    ExFreePoolWithTag(Child->m_MsOsExtConfig, HUB_TAG_DEVICE);
    Child->m_MsOsExtConfig = NULL;
    return FALSE;
}

/* MS OS 2.0 */

BOOLEAN
DeviceMachine::MsOs20SetValid()
{
    HubChild* Child = m_Device;
    HubDescContext Context;
    HubMsOs20ResumeTime* Resume;

    PAGED_CODE();

    if (Child->m_MsOs20Set == NULL)
    {
        DPRINT1("Device %p has no MS OS 2.0 set buffer\n", Child);
        return FALSE;
    }

    Child->m_MsOs20SetLength = HubDeviceBytesReturned(Child);

    HubDeviceDescContext(Child, &Context);
    if (!HubDescCheckMsOs20Set(&Context,
                               Child->m_MsOs20Set,
                               Child->m_MsOs20SetLength,
                               &Child->m_MsOs20SetInfo,
                               &Child->m_MsOs20))
    {
        Child->m_EnumMessageId = HUB_ENUM_BAD_MSOS20_SET;
        return FALSE;
    }

    if (Child->m_MsOs20.Flags & HUB_MSOS20_CCGP)
        Child->SetProperty(ChildProperty::IsComposite);

    Resume = Child->m_MsOs20.ResumeTime;
    if (Resume != NULL)
    {
        Child->m_HasResumeRecoveryTime = TRUE;
        Child->m_ResumeRecoveryTime = Resume->bResumeRecoveryTime;
        Child->m_ResumeSignalingTime = Resume->bResumeSignalingTime;
    }

    return TRUE;
}

/* Teardown */

VOID
NTAPI
HubReleaseTransferState(
    _In_ HubChild* Child)
{
    HubFreeBillboardInfo(Child);

    if (Child->m_SublinkSpeedAttr != NULL)
    {
        ExFreePoolWithTag(Child->m_SublinkSpeedAttr, HUB_TAG_DEVICE);
        Child->m_SublinkSpeedAttr = NULL;
        Child->m_SublinkSpeedAttrCount = 0;
    }

    if (Child->m_MsOsExtConfig != NULL)
    {
        ExFreePoolWithTag(Child->m_MsOsExtConfig, HUB_TAG_DEVICE);
        Child->m_MsOsExtConfig = NULL;
    }
}
