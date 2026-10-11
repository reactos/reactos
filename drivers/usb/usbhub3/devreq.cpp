/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Control transfers the device machine sends to a child device
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"
#include "devreq.h"

#define NDEBUG
#include <debug.h>

/* bmRequestType bytes used on the default pipe */
#define XFER_STD_TO_DEVICE          0x00
#define XFER_STD_TO_INTERFACE       0x01
#define XFER_STD_TO_ENDPOINT        0x02
#define XFER_STD_FROM_DEVICE        0x80
#define XFER_VENDOR_TO_DEVICE       0x40
#define XFER_VENDOR_FROM_DEVICE     0xC0
#define XFER_VENDOR_FROM_INTERFACE  0xC1
#define XFER_CLASS_FROM_PORT        0xA3

/* bmRequestType bits 6:5 */
#define XFER_TYPE(RequestType)      (((RequestType) >> 5) & 3)
#define XFER_TYPE_STANDARD          0
#define XFER_TYPE_VENDOR            2

/* Default size of a descriptor header read; the length is read back from it */
#define XFER_HEADER_READ            255

/* MS OS 1.0 extended properties header */
#define XFER_EXT_PROPERTIES_HEADER  10

/* The serial number is always asked in this language */
#define XFER_SERIAL_LANGUAGE        0x0409

/* Function suspend options in the high byte of wIndex */
#define XFER_FUNCTION_SUSPEND_WAKE  0x0300

/* Hub max port power that selects the low power charging policy */
#define XFER_LOW_POWER_PORT         100

C_ASSERT(sizeof(((HubChild*)0)->m_Sel) == 6);
C_ASSERT(sizeof(((HubChild*)0)->m_Qualifier) == 10);

static EVT_WDF_REQUEST_COMPLETION_ROUTINE HubDeviceControlComplete;
static EVT_WDF_REQUEST_COMPLETION_ROUTINE HubFdoDescriptorComplete;
static EVT_WDF_REQUEST_COMPLETION_ROUTINE HubBootStatusComplete;

/* Failure classification */

/* QUIRK: billboard string failures reuse the product string reason */
static
VOID
NTAPI
HubDeviceStringFailed(
    _In_ HubChild* Child,
    _In_ UCHAR Index)
{
    const USB_DEVICE_DESCRIPTOR* Device = &Child->m_DeviceDescriptor;
    HubBillboardInfo* Billboard = Child->m_Billboard;
    PUSB_DEVICE_CAPABILITY_BILLBOARD_DESCRIPTOR Caps;

    if (Device->iSerialNumber != 0 && Index == Device->iSerialNumber)
    {
        Child->m_EnumMessageId = HUB_ENUM_SERIAL_NUMBER_FAILED;
        return;
    }

    if (Index == 0)
    {
        Child->m_EnumMessageId = HUB_ENUM_LANGUAGE_IDS_FAILED;
        return;
    }

    if (Device->iProduct != 0 && Index == Device->iProduct)
    {
        Child->m_EnumMessageId = HUB_ENUM_PRODUCT_STRING_FAILED;
        return;
    }

    if (Billboard == NULL)
        return;

    Caps = Billboard->Descriptor;
    if (Caps->iAddtionalInfoURL != 0 && Index == Caps->iAddtionalInfoURL)
    {
        Child->m_EnumMessageId = HUB_ENUM_PRODUCT_STRING_FAILED;
        return;
    }

    if (Billboard->CurrentMode != HUB_BILLBOARD_NO_MODE &&
        Caps->AlternateMode[Billboard->CurrentMode].iAlternateModeSetting != 0 &&
        Index == Caps->AlternateMode[Billboard->CurrentMode].iAlternateModeSetting)
    {
        Child->m_EnumMessageId = HUB_ENUM_PRODUCT_STRING_FAILED;
    }
}

/** Requests a device may refuse without anything being wrong with it. */
static
BOOLEAN
NTAPI
HubDeviceFailureExpected(
    _In_ UCHAR Type,
    _In_ UCHAR Request,
    _In_ USHORT Value,
    _In_ USHORT Index)
{
    if (Type != XFER_TYPE_STANDARD)
        return FALSE;

    if (Request == USB_REQUEST_CLEAR_FEATURE && Value == USB_FEATURE_REMOTE_WAKEUP)
        return TRUE;

    if (Request == USB_REQUEST_SET_FEATURE && Value == USB_FEATURE_FUNCTION_SUSPEND && Index == 0)
        return TRUE;

    if (Request != USB_REQUEST_GET_DESCRIPTOR)
        return FALSE;

    return Value == ((USB_STRING_DESCRIPTOR_TYPE << 8) | HUB_MSOS_STRING_INDEX) ||
           (Value >> 8) == USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE;
}

/** Records why the transfer failed and picks the event to post. */
static
DsmEvent
NTAPI
HubDeviceTransferFailed(
    _In_ HubChild* Child,
    _In_ NTSTATUS Status)
{
    const UCHAR* Setup = Child->m_Control.Urb.SetupPacket;
    USBD_STATUS UrbStatus = Child->m_Control.Urb.Hdr.Status;
    UCHAR Type = XFER_TYPE(Setup[0]);
    UCHAR Request = Setup[1];
    USHORT Value = Setup[2] | (Setup[3] << 8);
    USHORT Index = Setup[4] | (Setup[5] << 8);

    if (HubDeviceFailureExpected(Type, Request, Value, Index))
    {
        DPRINT("Device %p port %u request 0x%02x/0x%02x value 0x%04x failed 0x%lx usbd 0x%lx\n",
               Child, Child->m_Port->Number(), Setup[0], Request, Value, Status, UrbStatus);
    }
    else
    {
        DPRINT1("Device %p port %u request 0x%02x/0x%02x value 0x%04x index 0x%04x failed 0x%lx usbd 0x%lx\n",
                Child, Child->m_Port->Number(), Setup[0], Request, Value, Index, Status, UrbStatus);
    }

    /* Every failed nonzero SET_CONFIGURATION reports the same codes */
    if (Request == USB_REQUEST_SET_CONFIGURATION && Value != 0)
    {
        Child->m_LastNtStatus = STATUS_UNSUCCESSFUL;
        Child->m_LastUsbdStatus = USBD_STATUS_SET_CONFIG_FAILED;
    }
    else
    {
        Child->m_LastNtStatus = Status;
        Child->m_LastUsbdStatus = UrbStatus;
    }

    if (Request == USB_REQUEST_SET_SEL && Type == XFER_TYPE_STANDARD)
    {
        Child->m_EnumMessageId = HUB_ENUM_SET_SEL_FAILED;
        return (UrbStatus == USBD_STATUS_STALL_PID) ? DsmEvent::TransferStalled : DsmEvent::TransferFailed;
    }

    if (Request == USB_REQUEST_SET_FEATURE && (Value == USB_FEATURE_U1_ENABLE || Value == USB_FEATURE_U2_ENABLE))
        return DsmEvent::TransferFailed;

    if (Request == USB_REQUEST_GET_DESCRIPTOR && Type == XFER_TYPE_STANDARD)
    {
        switch (Setup[3])
        {
            case USB_DEVICE_DESCRIPTOR_TYPE:
                Child->m_EnumMessageId = HUB_ENUM_DEVICE_DESCRIPTOR_FAILED;
                break;

            case USB_CONFIGURATION_DESCRIPTOR_TYPE:
                Child->m_EnumMessageId = HUB_ENUM_CONFIG_DESCRIPTOR_FAILED;
                break;

            case USB_BOS_DESCRIPTOR_TYPE:
                Child->m_EnumMessageId = HUB_ENUM_BOS_FAILED;
                break;

            case USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE:
                Child->m_EnumMessageId = HUB_ENUM_QUALIFIER_FAILED;
                break;

            case USB_STRING_DESCRIPTOR_TYPE:
                HubDeviceStringFailed(Child, Setup[2]);
                break;

            default:
                break;
        }
        return DsmEvent::TransferFailed;
    }

    if (Type == XFER_TYPE_VENDOR && Request == Child->m_MsOsVendorCode)
    {
        switch (Index)
        {
            case HUB_MSOS_INDEX_EXT_CONFIG:
                Child->m_EnumMessageId = HUB_ENUM_MSOS_EXT_CONFIG_FAILED;
                break;

            case HUB_MSOS_INDEX_CONTAINER_ID:
                Child->m_EnumMessageId = HUB_ENUM_MSOS_CONTAINER_ID_FAILED;
                break;

            case HUB_MSOS20_INDEX_SET:
                Child->m_EnumMessageId = HUB_ENUM_MSOS20_SET_FAILED;
                break;

            case HUB_MSOS20_INDEX_ALT_ENUM:
                Child->m_EnumMessageId = HUB_ENUM_ALT_ENUM_FAILED;
                break;

            default:
                break;
        }
        return DsmEvent::TransferFailed;
    }

    return DsmEvent::TransferFailed;
}

static
VOID
NTAPI
HubDeviceControlComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context)
{
    HubChild* Child = (HubChild*)Context;
    NTSTATUS Status = Params->IoStatus.Status;
    DsmEvent Event = DsmEvent::TransferDone;

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(Target);

    Child->m_Control.UsbdFlags = 0;

    if (!NT_SUCCESS(Status))
        Event = HubDeviceTransferFailed(Child, Status);

    Child->m_Control.Reuse();
    Child->Post(Event);
}

/* Sending */

BOOLEAN
NTAPI
HubDeviceSend(
    _In_ HubChild* Child,
    _In_reads_bytes_opt_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_ BOOLEAN ShortTransferOk)
{
    NTSTATUS Status;

    Status = Child->m_Control.Send(Child->m_Hub->m_RootHubTarget,
                                   Child->m_UsbDevice,
                                   HubDeviceControlComplete,
                                   Child,
                                   Buffer,
                                   Length,
                                   ShortTransferOk,
                                   Child->m_NeedsForwardProgress);
    if (NT_SUCCESS(Status))
        return TRUE;

    Child->m_Control.UsbdFlags = 0;
    DPRINT1("Device %p transfer refused 0x%lx\n", Child, Status);
    return FALSE;
}

/* wLength is truncated to 16 bits while the buffer keeps the full length */
static
BOOLEAN
NTAPI
HubDeviceTrySend(
    _In_ HubChild* Child,
    _In_ UCHAR RequestType,
    _In_ UCHAR Request,
    _In_ USHORT Value,
    _In_ USHORT Index,
    _In_reads_bytes_opt_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_ BOOLEAN ShortTransferOk)
{
    Child->m_Control.SetSetup(RequestType, Request, Value, Index, (USHORT)Length);
    return HubDeviceSend(Child, Buffer, Length, ShortTransferOk);
}

static
BOOLEAN
NTAPI
HubDeviceRequest(
    _In_ HubChild* Child,
    _In_ UCHAR RequestType,
    _In_ UCHAR Request,
    _In_ USHORT Value,
    _In_ USHORT Index,
    _In_reads_bytes_opt_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_ BOOLEAN ShortTransferOk)
{
    if (HubDeviceTrySend(Child, RequestType, Request, Value, Index, Buffer, Length, ShortTransferOk))
        return TRUE;

    Child->Post(DsmEvent::TransferFailed);
    return FALSE;
}

static
VOID
NTAPI
HubDeviceGetDescriptor(
    _In_ HubChild* Child,
    _In_ UCHAR Type,
    _In_ UCHAR Index,
    _In_ USHORT LanguageId,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length)
{
    HubDeviceRequest(Child,
                     XFER_STD_FROM_DEVICE,
                     USB_REQUEST_GET_DESCRIPTOR,
                     (USHORT)((Type << 8) | Index),
                     LanguageId,
                     Buffer,
                     Length,
                     TRUE);
}

static
VOID
NTAPI
HubDeviceGetString(
    _In_ HubChild* Child,
    _In_ UCHAR Index,
    _In_ USHORT LanguageId)
{
    HubDeviceGetDescriptor(Child,
                           USB_STRING_DESCRIPTOR_TYPE,
                           Index,
                           LanguageId,
                           Child->m_ScratchBuffer,
                           MAXIMUM_USB_STRING_LENGTH);
}

static
VOID
NTAPI
HubDeviceMsOsRead(
    _In_ HubChild* Child,
    _In_ UCHAR RequestType,
    _In_ USHORT Index,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Length)
{
    HubDeviceRequest(Child, RequestType, Child->m_MsOsVendorCode, 0, Index, Buffer, Length, TRUE);
}

static
VOID
NTAPI
HubDeviceFeature(
    _In_ HubChild* Child,
    _In_ BOOLEAN Set,
    _In_ USHORT Feature)
{
    HubDeviceRequest(Child,
                     XFER_STD_TO_DEVICE,
                     Set ? USB_REQUEST_SET_FEATURE : USB_REQUEST_CLEAR_FEATURE,
                     Feature,
                     0,
                     NULL,
                     0,
                     TRUE);
}

static
VOID
NTAPI
HubFreeDevicePool(
    _Inout_ PVOID* Buffer)
{
    if (*Buffer == NULL)
        return;

    ExFreePoolWithTag(*Buffer, HUB_TAG_DEVICE);
    *Buffer = NULL;
}

/* Device and configuration descriptors */

/* Some devices misbehave unless the first read asks for 64 bytes */
VOID
DeviceMachine::ReadFirstDescriptor()
{
    HubDeviceGetDescriptor(m_Device, USB_DEVICE_DESCRIPTOR_TYPE, 0, 0, m_Device->m_ScratchBuffer, 0x40);
}

VOID
DeviceMachine::ReadDeviceDescriptor()
{
    HubDeviceGetDescriptor(m_Device,
                           USB_DEVICE_DESCRIPTOR_TYPE,
                           0,
                           0,
                           m_Device->m_ScratchBuffer,
                           sizeof(USB_DEVICE_DESCRIPTOR));
}

VOID
DeviceMachine::CancelTransfer()
{
    if (!WdfRequestCancelSentRequest(m_Device->m_Control.Request))
        DPRINT1("Device %p transfer not canceled, already completed\n", m_Device);
}

VOID
DeviceMachine::ReadConfigDescriptorHeader()
{
    HubChild* Child = m_Device;

    HubFreeDevicePool((PVOID*)&Child->m_ConfigDescriptor);
    Child->m_CompatIdInterface = NULL;

    HubDeviceGetDescriptor(Child,
                           USB_CONFIGURATION_DESCRIPTOR_TYPE,
                           0,
                           0,
                           Child->m_ScratchBuffer,
                           XFER_HEADER_READ);
}

/* The header stays in the scratch buffer for the check */
VOID
DeviceMachine::ReadFullConfigDescriptor()
{
    HubChild* Child = m_Device;
    USHORT Total = ((PUSB_CONFIGURATION_DESCRIPTOR)Child->m_ScratchBuffer)->wTotalLength;

    Child->m_ConfigDescriptor = (PUSB_CONFIGURATION_DESCRIPTOR)ExAllocatePoolWithTag(NonPagedPool,
                                                                                     Total,
                                                                                     HUB_TAG_DEVICE);
    if (Child->m_ConfigDescriptor == NULL)
    {
        DPRINT1("Device %p no memory for a %u byte configuration\n", Child, Total);
        Child->Post(DsmEvent::TransferFailed);
        return;
    }

    HubDeviceGetDescriptor(Child, USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, Child->m_ConfigDescriptor, Total);
}

/* BOS */

VOID
DeviceMachine::ReadBosHeader()
{
    HubChild* Child = m_Device;

    HubFreeDevicePool((PVOID*)&Child->m_Bos);
    HubFreeBillboardInfo(Child);

    HubDeviceGetDescriptor(Child, USB_BOS_DESCRIPTOR_TYPE, 0, 0, Child->m_ScratchBuffer, XFER_HEADER_READ);
}

VOID
DeviceMachine::ReadBos()
{
    HubChild* Child = m_Device;
    USHORT Total = ((PUSB_BOS_DESCRIPTOR)Child->m_ScratchBuffer)->wTotalLength;

    Child->m_Bos = (PUSB_BOS_DESCRIPTOR)ExAllocatePoolWithTag(NonPagedPool, Total, HUB_TAG_DEVICE);
    if (Child->m_Bos == NULL)
    {
        DPRINT1("Device %p no memory for a %u byte BOS\n", Child, Total);
        Child->Post(DsmEvent::TransferFailed);
        return;
    }

    HubDeviceGetDescriptor(Child, USB_BOS_DESCRIPTOR_TYPE, 0, 0, Child->m_Bos, Total);
}

/* The vendor command and the features come from the BOS walk */
VOID
DeviceMachine::SendUsbFeatures()
{
    HubChild* Child = m_Device;
    ULONG Features = Child->m_DualRoleLocalFeatures;

    if (HubDeviceRequest(Child,
                         XFER_VENDOR_TO_DEVICE,
                         Child->m_DualRoleRequestCode,
                         (USHORT)(Features & 0xFFFF),
                         (USHORT)(Features >> 16),
                         NULL,
                         0,
                         TRUE))
    {
        Child->SetState(ChildState::DualRoleCommandSent);
    }
}

/* Billboard strings */

VOID
DeviceMachine::ReadBillboardString()
{
    HubChild* Child = m_Device;

    if (Child->m_Billboard == NULL)
    {
        DPRINT1("Device %p has no billboard to read the URL string of\n", Child);
        Child->Post(DsmEvent::TransferFailed);
        return;
    }

    HubDeviceGetString(Child, Child->m_Billboard->Descriptor->iAddtionalInfoURL, 0);
}

VOID
DeviceMachine::ReadAltModeString()
{
    HubChild* Child = m_Device;
    HubBillboardInfo* Billboard = Child->m_Billboard;
    UCHAR Index;

    if (Billboard == NULL || Billboard->CurrentMode == HUB_BILLBOARD_NO_MODE)
    {
        DPRINT1("Device %p has no alternate mode string to read\n", Child);
        Child->Post(DsmEvent::TransferFailed);
        return;
    }

    Index = Billboard->Descriptor->AlternateMode[Billboard->CurrentMode].iAlternateModeSetting;

    Child->m_Control.SetSetup(XFER_STD_FROM_DEVICE,
                              USB_REQUEST_GET_DESCRIPTOR,
                              (USHORT)((USB_STRING_DESCRIPTOR_TYPE << 8) | Index),
                              0,
                              MAXIMUM_USB_STRING_LENGTH);

    if (!HubDeviceSend(Child, Child->m_ScratchBuffer, MAXIMUM_USB_STRING_LENGTH, TRUE))
    {
        Billboard->CurrentMode = HUB_BILLBOARD_NO_MODE;
        Child->Post(DsmEvent::TransferFailed);
    }
}

/* Strings */

VOID
DeviceMachine::ReadLanguageIds()
{
    HubFreeDevicePool((PVOID*)&m_Device->m_LanguageIds);
    HubDeviceGetString(m_Device, 0, 0);
}

/* QUIRK: without the errata a zero iProduct reads the language table as the name */
VOID
DeviceMachine::ReadProductName()
{
    HubChild* Child = m_Device;

    if (Child->m_DeviceDescriptor.iProduct == 0 && Child->HasHack(ChildHack::DisableSerialNumber))
    {
        DPRINT("Device %p has no product string\n", Child);
        Child->Post(DsmEvent::TransferFailed);
        return;
    }

    HubFreeDevicePool((PVOID*)&Child->m_ProductString);
    HubDeviceGetString(Child, Child->m_DeviceDescriptor.iProduct, Child->m_LanguageId);
}

/* Always in US English, whatever the device lists */
VOID
DeviceMachine::ReadSerialNumber()
{
    HubChild* Child = m_Device;

    if (Child->m_DeviceDescriptor.iSerialNumber == 0)
    {
        DPRINT("Device %p has no serial number string\n", Child);
        Child->Post(DsmEvent::TransferFailed);
        return;
    }

    HubDeviceGetString(Child, Child->m_DeviceDescriptor.iSerialNumber, XFER_SERIAL_LANGUAGE);
}

VOID
DeviceMachine::ReadQualifier()
{
    HubDeviceGetDescriptor(m_Device,
                           USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE,
                           0,
                           0,
                           &m_Device->m_Qualifier,
                           sizeof(m_Device->m_Qualifier));
}

/* MS OS 1.0 */

VOID
DeviceMachine::ReadMsOsDescriptor()
{
    HubDeviceGetDescriptor(m_Device,
                           USB_STRING_DESCRIPTOR_TYPE,
                           HUB_MSOS_STRING_INDEX,
                           0,
                           &m_Device->m_MsOsString,
                           sizeof(m_Device->m_MsOsString));
}

VOID
DeviceMachine::ReadContainerIdHeader()
{
    HubDeviceMsOsRead(m_Device,
                      XFER_VENDOR_FROM_DEVICE,
                      HUB_MSOS_INDEX_CONTAINER_ID,
                      m_Device->m_ScratchBuffer,
                      sizeof(HubMsOsHeader));
}

VOID
DeviceMachine::ReadContainerId()
{
    HubDeviceMsOsRead(m_Device,
                      XFER_VENDOR_FROM_DEVICE,
                      HUB_MSOS_INDEX_CONTAINER_ID,
                      m_Device->m_ScratchBuffer,
                      sizeof(HubMsOsContainerId));
}

VOID
DeviceMachine::ReadExtendedConfigHeader()
{
    HubFreeDevicePool((PVOID*)&m_Device->m_MsOsExtConfig);

    HubDeviceMsOsRead(m_Device,
                      XFER_VENDOR_FROM_DEVICE,
                      HUB_MSOS_INDEX_EXT_CONFIG,
                      m_Device->m_ScratchBuffer,
                      sizeof(HubMsOsExtConfigHeader));
}

/* UCX refuses a buffer over 64 KB, so a huge dwLength just fails the transfer */
VOID
DeviceMachine::ReadExtendedConfig()
{
    HubChild* Child = m_Device;
    ULONG Length = ((HubMsOsExtConfigHeader*)Child->m_ScratchBuffer)->dwLength;

    Child->m_MsOsExtConfig = (HubMsOsExtConfig*)ExAllocatePoolWithTag(NonPagedPool, Length, HUB_TAG_DEVICE);
    if (Child->m_MsOsExtConfig == NULL)
    {
        DPRINT1("Device %p no memory for a %lu byte extended configuration\n", Child, Length);
        Child->Post(DsmEvent::TransferFailed);
        return;
    }

    HubDeviceMsOsRead(Child, XFER_VENDOR_FROM_DEVICE, HUB_MSOS_INDEX_EXT_CONFIG, Child->m_MsOsExtConfig, Length);
}

VOID
DeviceMachine::ReadExtPropertiesHeader()
{
    HubDeviceMsOsRead(m_Device,
                      XFER_VENDOR_FROM_INTERFACE,
                      HUB_MSOS_INDEX_EXT_PROPERTIES,
                      m_Device->m_ScratchBuffer,
                      XFER_EXT_PROPERTIES_HEADER);
}

/* The buffer was sized from the header by AllocateExtPropertiesBuffer */
VOID
DeviceMachine::ReadExtProperties()
{
    HubChild* Child = m_Device;
    ULONG Length = *(PULONG)Child->m_ScratchBuffer;

    if (Child->m_ExtProperties == NULL)
    {
        DPRINT1("Device %p has no extended properties buffer\n", Child);
        Child->Post(DsmEvent::TransferFailed);
        return;
    }

    HubDeviceMsOsRead(Child, XFER_VENDOR_FROM_INTERFACE, HUB_MSOS_INDEX_EXT_PROPERTIES, Child->m_ExtProperties, Length);
}

/* MS OS 2.0 */

/* Large transfers must be allowed or UCX caps the read at the default pipe maximum */
VOID
DeviceMachine::ReadMsOs20Set()
{
    HubChild* Child = m_Device;
    USHORT Length = Child->m_MsOs20SetInfo.wLength;

    HubFreeDevicePool(&Child->m_MsOs20Set);
    Child->m_MsOs20SetLength = 0;

    Child->m_MsOs20Set = ExAllocatePoolWithTag(NonPagedPool, Length, HUB_TAG_DEVICE);
    if (Child->m_MsOs20Set == NULL)
    {
        DPRINT1("Device %p no memory for a %u byte MS OS 2.0 set\n", Child, Length);
        Child->Post(DsmEvent::TransferFailed);
        return;
    }

    Child->m_MsOsVendorCode = Child->m_MsOs20SetInfo.bVendorCode;
    Child->m_Control.UsbdFlags = UCXHUB_URB_FLAG_ALLOW_LARGE_TRANSFER;

    HubDeviceMsOsRead(Child, XFER_VENDOR_FROM_DEVICE, HUB_MSOS20_INDEX_SET, Child->m_MsOs20Set, Length);
}

/* QUIRK: the command counts as sent once the send is accepted, even if the device fails it */
VOID
DeviceMachine::SendAltEnumCommand()
{
    HubChild* Child = m_Device;

    if (HubDeviceRequest(Child,
                         XFER_VENDOR_TO_DEVICE,
                         Child->m_MsOsVendorCode,
                         (USHORT)(Child->m_MsOs20SetInfo.bAltEnumCode << 8),
                         HUB_MSOS20_INDEX_ALT_ENUM,
                         NULL,
                         0,
                         TRUE))
    {
        Child->SetState(ChildState::AltEnumCommandSent);
    }
}

/* Configuration and pipes */

VOID
DeviceMachine::SetConfiguration()
{
    HubChild* Child = m_Device;

    if (Child->m_CurrentConfig == NULL)
    {
        DPRINT1("Device %p has no configuration to select\n", Child);
        Child->Post(DsmEvent::TransferFailed);
        return;
    }

    HubDeviceRequest(Child,
                     XFER_STD_TO_DEVICE,
                     USB_REQUEST_SET_CONFIGURATION,
                     Child->m_CurrentConfig->Descriptor.bConfigurationValue,
                     0,
                     NULL,
                     0,
                     FALSE);
}

VOID
DeviceMachine::SetNullConfiguration()
{
    HubDeviceRequest(m_Device, XFER_STD_TO_DEVICE, USB_REQUEST_SET_CONFIGURATION, 0, 0, NULL, 0, FALSE);
}

BOOLEAN
DeviceMachine::AlternateSettingLeft()
{
    HubConfiguration* Config = m_Device->m_CurrentConfig;
    HubInterface* Interface;
    PLIST_ENTRY Entry;

    if (Config == NULL)
        return FALSE;

    for (Entry = Config->Interfaces.Flink; Entry != &Config->Interfaces; Entry = Entry->Flink)
    {
        Interface = CONTAINING_RECORD(Entry, HubInterface, Link);
        if (Interface->NeedsSetInterface)
        {
            m_Device->m_NextInterface = Interface;
            return TRUE;
        }
    }

    return FALSE;
}

VOID
DeviceMachine::SetInterface()
{
    HubChild* Child = m_Device;
    HubInterface* Interface = Child->m_NextInterface;

    if (Interface == NULL)
    {
        DPRINT("Device %p has no alternate setting left to select\n", Child);
        Child->Post(DsmEvent::TransferDone);
        return;
    }

    Child->m_Control.SetSetup(XFER_STD_TO_INTERFACE,
                              USB_REQUEST_SET_INTERFACE,
                              Interface->Descriptor->bAlternateSetting,
                              Interface->Descriptor->bInterfaceNumber,
                              0);

    Interface->NeedsSetInterface = FALSE;
    Child->m_NextInterface = NULL;

    if (!HubDeviceSend(Child, NULL, 0, FALSE))
        Child->Post(DsmEvent::TransferFailed);
}

VOID
DeviceMachine::ClearEndpointHalt()
{
    HubChild* Child = m_Device;
    HubPipe* Pipe = Child->m_TargetPipe;

    if (Pipe == NULL)
    {
        DPRINT1("Device %p clear halt on an unknown pipe\n", Child);
        Child->m_LastUsbdStatus = USBD_STATUS_INVALID_PIPE_HANDLE;
        Child->m_LastNtStatus = STATUS_INVALID_PARAMETER;
        Child->Post(DsmEvent::TransferFailed);
        return;
    }

    HubDeviceRequest(Child,
                     XFER_STD_TO_ENDPOINT,
                     USB_REQUEST_CLEAR_FEATURE,
                     USB_FEATURE_ENDPOINT_STALL,
                     Pipe->Descriptor->bEndpointAddress,
                     NULL,
                     0,
                     FALSE);
}

HubPipe*
NTAPI
HubFindPipeByHandle(
    _In_opt_ HubConfiguration* Config,
    _In_ USBD_PIPE_HANDLE Handle)
{
    HubInterface* Interface;
    PLIST_ENTRY Entry;
    ULONG Index;

    if (Config == NULL)
        return NULL;

    for (Entry = Config->Interfaces.Flink; Entry != &Config->Interfaces; Entry = Entry->Flink)
    {
        Interface = CONTAINING_RECORD(Entry, HubInterface, Link);

        for (Index = 0; Index < Interface->PipeCount; Index++)
        {
            if (Interface->Pipes[Index].PipeHandle == Handle)
                return &Interface->Pipes[Index];
        }
    }

    return NULL;
}

/* Remote wake */

/* QUIRK: the PDO is marked armed before the request and stays so on failure */
VOID
DeviceMachine::ArmForWake()
{
    HubChild* Child = m_Device;

    if (Child->m_Pdo != NULL)
        InterlockedOr(&Child->m_Pdo->m_Flags, (LONG)PdoFlag::ArmedForWake);

    if (Child->m_Kind & DSM_KIND_SUPER_SPEED)
    {
        HubDeviceRequest(Child,
                         XFER_STD_TO_INTERFACE,
                         USB_REQUEST_SET_FEATURE,
                         USB_FEATURE_FUNCTION_SUSPEND,
                         XFER_FUNCTION_SUSPEND_WAKE,
                         NULL,
                         0,
                         TRUE);
    }
    else
    {
        HubDeviceFeature(Child, TRUE, USB_FEATURE_REMOTE_WAKEUP);
    }
}

VOID
DeviceMachine::DisarmWake()
{
    HubChild* Child = m_Device;

    if (Child->m_Pdo != NULL)
        InterlockedAnd(&Child->m_Pdo->m_Flags, ~(LONG)PdoFlag::ArmedForWake);

    if (Child->m_Kind & DSM_KIND_SUPER_SPEED)
    {
        HubDeviceRequest(Child,
                         XFER_STD_TO_INTERFACE,
                         USB_REQUEST_SET_FEATURE,
                         USB_FEATURE_FUNCTION_SUSPEND,
                         0,
                         NULL,
                         0,
                         TRUE);
    }
    else
    {
        HubDeviceFeature(Child, FALSE, USB_FEATURE_REMOTE_WAKEUP);
    }
}

/* Link power */

VOID
DeviceMachine::EnableU1()
{
    HubDeviceFeature(m_Device, TRUE, USB_FEATURE_U1_ENABLE);
}

VOID
DeviceMachine::DisableU1()
{
    HubDeviceFeature(m_Device, FALSE, USB_FEATURE_U1_ENABLE);
}

VOID
DeviceMachine::EnableU2()
{
    HubDeviceFeature(m_Device, TRUE, USB_FEATURE_U2_ENABLE);
}

VOID
DeviceMachine::DisableU2()
{
    HubDeviceFeature(m_Device, FALSE, USB_FEATURE_U2_ENABLE);
}

VOID
DeviceMachine::EnableLtm()
{
    HubDeviceFeature(m_Device, TRUE, USB_FEATURE_LTM_ENABLE);
}

VOID
DeviceMachine::SetPdChargingPolicy()
{
    USHORT Policy = (m_Device->m_Hub->m_MaxPortPower == XFER_LOW_POWER_PORT) ? USB_CHARGING_POLICY_ICCLPF
                                                                              : USB_CHARGING_POLICY_ICCHPF;

    HubDeviceRequest(m_Device,
                     XFER_STD_TO_DEVICE,
                     USB_REQUEST_SET_FEATURE,
                     USB_FEATURE_CHARGING_POLICY,
                     Policy,
                     NULL,
                     0,
                     TRUE);
}

/** Path exit latency for one link state; Slowest is set when Link exceeds it. */
static
USHORT
NTAPI
HubSelPathLatency(
    _In_ USHORT DeviceLatency,
    _In_ USHORT UpstreamLatency,
    _In_ USHORT SlowestLatency,
    _In_ UCHAR SlowestDepth,
    _In_ LONG Depth,
    _Out_ PUSHORT Link,
    _Inout_ PBOOLEAN Slowest)
{
    USHORT Path;

    *Link = max(DeviceLatency, UpstreamLatency);
    Path = (USHORT)(SlowestLatency + ((Depth - SlowestDepth) * 1000 + 500) / 1000);

    if (*Link <= Path)
        return Path;

    *Slowest = TRUE;
    return *Link;
}

/* QUIRK: U1 PEL and SEL are 8 bit and wrap; at depth 0 the link delays (ns) are added as microseconds */
VOID
DeviceMachine::SetSel()
{
    HubChild* Child = m_Device;
    HubFdo* Hub = Child->m_Hub;
    const HubParentInfo* Parent = &Hub->m_ParentInfo;
    LONG Depth = Hub->m_Parent.HubDepth;
    ULONG DecodeLatency = (Hub->m_HubDescriptor.Usb30.bHubHdrDecLat + 5) / 10;
    ULONG LinkDelay = Child->m_RxTpDelay + Child->m_TxTpDelay;
    USHORT LinkU1;
    USHORT LinkU2;
    UCHAR PelU1;
    USHORT PelU2;
    ULONG Erdy;

    PelU1 = (UCHAR)HubSelPathLatency(Child->m_U1ExitLatency,
                                     Parent->U1ExitLatency,
                                     Parent->SlowestLinkU1ExitLatency,
                                     Parent->SlowestLinkU1Depth,
                                     Depth,
                                     &LinkU1,
                                     &Child->m_SlowestLinkU1);

    PelU2 = HubSelPathLatency(Child->m_U2ExitLatency,
                              Parent->U2ExitLatency,
                              Parent->SlowestLinkU2ExitLatency,
                              Parent->SlowestLinkU2Depth,
                              Depth,
                              &LinkU2,
                              &Child->m_SlowestLinkU2);

    Child->m_HostU1ExitLatency = (USHORT)(Parent->HostInitiatedU1ExitLatency + LinkU1 + DecodeLatency);
    Child->m_HostU2ExitLatency = (USHORT)(Parent->HostInitiatedU2ExitLatency + LinkU2 + DecodeLatency);

    /* 2100 ns forwarding, 400 ns per hub each way, plus the link delays, rounded to us */
    if (Depth == 0)
        Erdy = LinkDelay;
    else
        Erdy = ((Depth + 1) * LinkDelay + 800 * Depth + 2200) / 1000;

    /* 5 us host response time */
    Child->m_Sel.U1Pel = PelU1;
    Child->m_Sel.U1Sel = (UCHAR)(PelU1 + Erdy + 5);
    Child->m_Sel.U2Pel = PelU2;
    Child->m_Sel.U2Sel = (USHORT)(PelU2 + Erdy + 5);

    HubDeviceRequest(Child,
                     XFER_STD_TO_DEVICE,
                     USB_REQUEST_SET_SEL,
                     0,
                     0,
                     &Child->m_Sel,
                     sizeof(Child->m_Sel),
                     TRUE);
}

/* All in ns: propagation to this hub, the hub's own delay and the device link */
VOID
DeviceMachine::SetIsochDelay()
{
    HubChild* Child = m_Device;
    HubFdo* Hub = Child->m_Hub;
    USHORT Delay;

    Delay = (USHORT)(Hub->m_ParentInfo.TotalTpPropagationDelay +
                     Hub->m_HubDescriptor.Usb30.wHubDelay +
                     Child->m_TxTpDelay);

    HubDeviceRequest(Child, XFER_STD_TO_DEVICE, USB_REQUEST_ISOCH_DELAY, Delay, 0, NULL, 0, TRUE);
}

/* User mode descriptor request on behalf of the hub FDO */

/* The event goes out before the user request is completed */
static
VOID
NTAPI
HubFdoDescriptorComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context)
{
    HubChild* Child = (HubChild*)Context;
    HubQueueContext* Queue = HubGetQueueContext(WdfRequestGetIoQueue(Request));
    ULONG Size = Queue->Urb.TransferBufferLength;

    UNREFERENCED_PARAMETER(Target);

    if (!NT_SUCCESS(Params->IoStatus.Status))
        DPRINT1("Device %p descriptor read for the hub failed 0x%lx\n", Child, Params->IoStatus.Status);

    Child->Post(NT_SUCCESS(Params->IoStatus.Status) ? DsmEvent::TransferDone : DsmEvent::TransferFailed);
    HubCompleteFdoDescriptorRequest(Child, Request, WdfRequestGetStatus(Request), Size);
}

/* The URB in the queue context was filled by the IOCTL handler */
VOID
DeviceMachine::GetDescriptorForHub()
{
    HubChild* Child = m_Device;
    WDFREQUEST Request = Child->m_FdoRequest;
    HubQueueContext* Queue = HubGetQueueContext(WdfRequestGetIoQueue(Request));
    IO_STACK_LOCATION Stack;

    RtlZeroMemory(&Stack, sizeof(Stack));
    Stack.MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    Stack.Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
    Stack.Parameters.Others.Argument1 = &Queue->Urb;
    WdfRequestWdmFormatUsingStackLocation(Request, &Stack);

    WdfRequestSetCompletionRoutine(Request, HubFdoDescriptorComplete, Child);

    if (WdfRequestSend(Request, Child->m_Hub->m_RootHubTarget, WDF_NO_SEND_OPTIONS))
        return;

    DPRINT1("Device %p descriptor read for the hub not sent 0x%lx\n", Child, WdfRequestGetStatus(Request));
    Child->Post(DsmEvent::TransferFailed);
    HubCompleteFdoDescriptorRequest(Child, Request, STATUS_UNSUCCESSFUL, 0);
}

/* Boot device presence */

static
VOID
NTAPI
HubBootStatusComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context)
{
    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Params);

    KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
}

/*
 * Always the 4 byte status, even on an enhanced SuperSpeed port. The request
 * is made the first time it is needed and reused on the next call.
 */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubReadPortStatusForBootDevice(
    _In_ HubChild* Child,
    _Out_ PULONG PortStatus)
{
    HubFdo* Hub = Child->m_Hub;
    HubControlRequest* Control = &Child->m_BootControl;
    KEVENT Done;
    NTSTATUS Status;

    PAGED_CODE();

    *PortStatus = 0;

    if (Control->Request == NULL)
    {
        Status = Control->Create(Child->m_Object, WdfDeviceGetIoTarget(Hub->m_Device));
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Device %p boot status request not created 0x%lx\n", Child, Status);
            return Status;
        }
    }
    else
    {
        Control->Reuse();
    }

    KeInitializeEvent(&Done, NotificationEvent, FALSE);

    Control->SetSetup(XFER_CLASS_FROM_PORT,
                      HUB_REQUEST_GET_STATUS,
                      0,
                      Child->m_Port->Number(),
                      sizeof(*PortStatus));

    Status = Control->Send(Hub->m_RootHubTarget,
                           Hub->UsbDevice(),
                           HubBootStatusComplete,
                           &Done,
                           PortStatus,
                           sizeof(*PortStatus),
                           FALSE,
                           TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Port %u boot status read not sent 0x%lx\n", Child->m_Port->Number(), Status);
        return Status;
    }

    HubWaitForPnpEvent(&Done, "boot device port status", Child->m_Object);

    Status = WdfRequestGetStatus(Control->Request);
    if (!NT_SUCCESS(Status))
        DPRINT1("Port %u boot status read failed 0x%lx\n", Child->m_Port->Number(), Status);

    return Status;
}
