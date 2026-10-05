/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Transfers the hub sends to its own hub device
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"

#define NDEBUG
#include <debug.h>

/* Public in newer usb.h: the transfer must not wait on paging */
#ifndef USB3_URB_RESERVED_RESOURCES
#define USB3_URB_RESERVED_RESOURCES 0x00000010
#endif

/* Control request blocks */

_Must_inspect_result_
NTSTATUS
HubControlRequest::Create(
    _In_ WDFOBJECT Parent,
    _In_ WDFIOTARGET SizingTarget)
{
    WDF_OBJECT_ATTRIBUTES Attributes;
    NTSTATUS Status;

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Parent;

    Status = WdfRequestCreate(&Attributes, SizingTarget, &Request);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Control request create for %p failed 0x%lx\n", Parent, Status);
        return Status;
    }

    Irp = WdfRequestWdmGetIrp(Request);
    UsbdFlags = 0;
    return STATUS_SUCCESS;
}

VOID
HubControlRequest::Reuse()
{
    WDF_REQUEST_REUSE_PARAMS Params;
    NTSTATUS Status;

    WDF_REQUEST_REUSE_PARAMS_INIT(&Params, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_SUCCESS);

    Status = WdfRequestReuse(Request, &Params);
    if (!NT_SUCCESS(Status))
        DPRINT1("Reusing request %p failed 0x%lx\n", Request, Status);
}

/*
 * The setup packet is already filled. A refused send is reported through the
 * request's status, and the request is made ready for the next use.
 */
NTSTATUS
HubControlRequest::Send(
    _In_ WDFIOTARGET Target,
    _In_opt_ UCXUSBDEVICE Device,
    _In_ PFN_WDF_REQUEST_COMPLETION_ROUTINE Completion,
    _In_ WDFCONTEXT Context,
    _In_reads_bytes_opt_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_ BOOLEAN ShortTransferOk,
    _In_ BOOLEAN ForwardProgress)
{
    IO_STACK_LOCATION Stack;
    NTSTATUS Status;

    Urb.Hdr.Length = sizeof(Urb);
    Urb.Hdr.Function = URB_FUNCTION_CONTROL_TRANSFER_EX;
    Urb.Hdr.UsbdDeviceHandle = Device;
    Urb.Hdr.UsbdFlags = UsbdFlags;

    Urb.TransferFlags = USBD_DEFAULT_PIPE_TRANSFER;
    if (ShortTransferOk)
        Urb.TransferFlags |= USBD_SHORT_TRANSFER_OK;
    if (Urb.SetupPacket[0] & 0x80)
        Urb.TransferFlags |= USBD_TRANSFER_DIRECTION_IN;
    if (ForwardProgress)
        Urb.TransferFlags |= USB3_URB_RESERVED_RESOURCES;

    Urb.TransferBufferLength = Length;
    Urb.TransferBuffer = Buffer;
    Urb.TransferBufferMDL = NULL;
    Urb.Timeout = HUB_SETUP_REQUEST_TIMEOUT_MS;

    RtlZeroMemory(&Stack, sizeof(Stack));
    Stack.MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    Stack.Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
    Stack.Parameters.Others.Argument1 = &Urb;
    WdfRequestWdmFormatUsingStackLocation(Request, &Stack);

    WdfRequestSetCompletionRoutine(Request, Completion, Context);

    if (WdfRequestSend(Request, Target, WDF_NO_SEND_OPTIONS))
        return STATUS_SUCCESS;

    Status = WdfRequestGetStatus(Request);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Control request 0x%x to device %p not sent 0x%lx\n", Urb.SetupPacket[1], Device, Status);
        Reuse();
    }

    return Status;
}

/* TT clear on behalf of ucx01000 */

static const char HubTagClearTt[] = "hub TT clear";

/* One TT clear in flight; several can be outstanding at once */
struct HubTtClear
{
    HubControlRequest Control;
    HubFdo* Hub;
    UCXENDPOINT Endpoint;
};

/* CLEAR_TT_BUFFER wValue; only control and bulk endpoints of low and full speed devices qualify */
static
NTSTATUS
NTAPI
HubTtClearValue(
    _In_ HubChild* Child,
    _In_ UCXENDPOINT Endpoint,
    _In_ ULONG EndpointAddress,
    _Out_ PUSHORT Value)
{
    HubConfiguration* Config = Child->m_CurrentConfig;
    PLIST_ENTRY Entry;
    HubInterface* Interface;
    HubPipe* Pipe;
    ULONG Index;
    USHORT Type;

    *Value = 0;

    if (Child->Speed() != UsbLowSpeed && Child->Speed() != UsbFullSpeed)
        return STATUS_UNSUCCESSFUL;

    if (Endpoint == Child->m_DefaultEndpoint)
    {
        *Value = (USHORT)((EndpointAddress & 0x0F) | ((Child->m_Address & 0x7F) << 4));
        return STATUS_SUCCESS;
    }

    if (Config == NULL)
        return STATUS_UNSUCCESSFUL;

    for (Entry = Config->Interfaces.Flink; Entry != &Config->Interfaces; Entry = Entry->Flink)
    {
        Interface = CONTAINING_RECORD(Entry, HubInterface, Link);

        for (Index = 0; Index < Interface->PipeCount; Index++)
        {
            Pipe = &Interface->Pipes[Index];
            if (Pipe->Endpoint != Endpoint)
                continue;

            switch (Pipe->Descriptor->bmAttributes & USB_ENDPOINT_TYPE_MASK)
            {
                case USB_ENDPOINT_TYPE_BULK:
                    Type = 2;
                    break;

                case USB_ENDPOINT_TYPE_CONTROL:
                    Type = 0;
                    break;

                default:
                    return STATUS_UNSUCCESSFUL;
            }

            *Value = (USHORT)((EndpointAddress & 0x0F) | ((Child->m_Address & 0x7F) << 4) | (Type << 11));
            if (EndpointAddress & USB_ENDPOINT_DIRECTION_MASK)
                *Value |= 0x8000;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_UNSUCCESSFUL;
}

static
VOID
NTAPI
HubTtClearComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context)
{
    HubTtClear* Clear = (HubTtClear*)Context;
    HubFdo* Hub = Clear->Hub;

    UNREFERENCED_PARAMETER(Target);

    if (!NT_SUCCESS(Params->IoStatus.Status))
    {
        DPRINT1("Hub %p TT clear for endpoint %p failed 0x%lx, URB status 0x%lx\n",
                Hub,
                Clear->Endpoint,
                Params->IoStatus.Status,
                Clear->Control.Urb.Hdr.Status);
    }

    Hub->m_Stack.ClearTtBufferComplete(Hub->m_Stack.Hub, Clear->Endpoint);

    WdfObjectDelete(Request);
    ExFreePoolWithTag(Clear, HUB_TAG_HUB);
}

/* Every path tells UCX the clear is over exactly once; until then the transfer cannot be canceled */
VOID
NTAPI
HubClearTtBuffer(
    _In_ UCXHUB_HUB_CONTEXT HubContext,
    _In_ UCXHUB_DEVICE_CONTEXT DeviceContext,
    _In_ UCXENDPOINT Endpoint,
    _In_ ULONG EndpointNumber,
    _In_ ULONG TtPortNumber)
{
    HubFdo* Hub = HubFdo::FromContext(HubContext);
    HubChild* Child = (HubChild*)DeviceContext;
    HubTtClear* Clear = NULL;
    WDFREQUEST Request = NULL;
    BOOLEAN ResetTt = Hub->HasFlag(HubFlag::ResetTtOnCancel);
    USHORT Value;
    NTSTATUS Status;

    if (!Hub->HasFlag(HubFlag::TtHub) || Child == NULL || Hub->HasFlag(HubFlag::NoClearTtOnCancel))
        goto Done;

    /* Computed even for RESET_TT, which does not use it */
    WdfObjectReferenceWithTag(Child->m_Object, (PVOID)HubTagClearTt);
    Status = HubTtClearValue(Child, Endpoint, EndpointNumber, &Value);
    WdfObjectDereferenceWithTag(Child->m_Object, (PVOID)HubTagClearTt);
    if (!NT_SUCCESS(Status))
        goto Done;

    Clear = (HubTtClear*)ExAllocatePoolWithTag(NonPagedPool, sizeof(*Clear), HUB_TAG_HUB);
    if (Clear == NULL)
    {
        DPRINT1("Hub %p no memory for a TT clear\n", Hub);
        goto Done;
    }

    RtlZeroMemory(Clear, sizeof(*Clear));
    Clear->Hub = Hub;
    Clear->Endpoint = Endpoint;

    if (!NT_SUCCESS(Clear->Control.Create(Hub->m_Device, WdfDeviceGetIoTarget(Hub->m_Device))))
        goto Done;
    Request = Clear->Control.Request;

    Clear->Control.SetSetup(0x23,
                            ResetTt ? HUB_REQUEST_RESET_TT : HUB_REQUEST_CLEAR_TT_BUFFER,
                            ResetTt ? 0 : Value,
                            Hub->HasFlag(HubFlag::MultiTtHub) ? (USHORT)TtPortNumber : 1,
                            0);

    Status = Clear->Control.Send(Hub->m_RootHubTarget,
                                 Hub->UsbDevice(),
                                 HubTtClearComplete,
                                 Clear,
                                 NULL,
                                 0,
                                 FALSE,
                                 Hub->NeedsForwardProgress());
    if (NT_SUCCESS(Status))
        return;

Done:
    if (Request != NULL)
        WdfObjectDelete(Request);
    if (Clear != NULL)
        ExFreePoolWithTag(Clear, HUB_TAG_HUB);

    Hub->m_Stack.ClearTtBufferComplete(Hub->m_Stack.Hub, Endpoint);
}

/* Output of IOCTL_USB_GET_PORT_STATUS */
#define HUB_USER_PORT_STATUS_OFFSET     4
#define HUB_USER_PORT_STATUS_SIZE       8

static EVT_WDF_REQUEST_COMPLETION_ROUTINE HubControlComplete;
static EVT_WDF_REQUEST_COMPLETION_ROUTINE HubInterruptComplete;

/* Hub level transfers */

static
VOID
NTAPI
HubControlComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context)
{
    HubFdo* Hub = (HubFdo*)Context;
    NTSTATUS Status = Params->IoStatus.Status;

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(Target);

    Hub->m_Control.Reuse();

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p control request 0x%x failed 0x%lx, URB status 0x%lx\n",
                Hub,
                Hub->m_Control.Urb.SetupPacket[1],
                Status,
                Hub->m_Control.Urb.Hdr.Status);
        Hub->m_FailureMessageId = HUB_MSG_CONTROL_TRANSFER_FAILED;
        Hub->Post(HubEvent::TransferFailed);
        return;
    }

    Hub->Post(HubEvent::TransferDone);
}

/* A refused send is reported to the machine right away */
static
VOID
NTAPI
HubSendControl(
    _In_ HubFdo* Hub,
    _In_reads_bytes_opt_(Length) PVOID Buffer,
    _In_ ULONG Length,
    _In_ BOOLEAN ShortOk)
{
    NTSTATUS Status;

    Status = Hub->m_Control.Send(Hub->m_RootHubTarget,
                                 Hub->UsbDevice(),
                                 HubControlComplete,
                                 Hub,
                                 Buffer,
                                 Length,
                                 ShortOk,
                                 Hub->NeedsForwardProgress());
    if (!NT_SUCCESS(Status))
        Hub->Post(HubEvent::TransferFailed);
}

VOID
HubMachine::AckHubChange()
{
    m_Hub->m_Control.SetSetup(0x20, HUB_REQUEST_CLEAR_FEATURE, m_Hub->m_SelectedFeature, 0, 0);
    DPRINT("Hub %p acking hub change %u\n", m_Hub, m_Hub->m_SelectedFeature);
    HubSendControl(m_Hub, NULL, 0, FALSE);
}

/* Only full, high and SuperSpeed hubs have a hub descriptor */
VOID
HubMachine::GetHubDescriptor()
{
    switch (m_Hub->m_Parent.HubSpeed)
    {
        case UsbFullSpeed:
        case UsbHighSpeed:
            m_Hub->m_Control.SetSetup(0xA0,
                                      HUB_REQUEST_GET_DESCRIPTOR,
                                      USB_20_HUB_DESCRIPTOR_TYPE << 8,
                                      0,
                                      sizeof(m_Hub->m_HubDescriptor.Usb20));
            HubSendControl(m_Hub, &m_Hub->m_HubDescriptor, sizeof(m_Hub->m_HubDescriptor.Usb20), TRUE);
            break;

        case UsbSuperSpeed:
            m_Hub->m_Control.SetSetup(0xA0,
                                      HUB_REQUEST_GET_DESCRIPTOR,
                                      USB_30_HUB_DESCRIPTOR_TYPE << 8,
                                      0,
                                      sizeof(m_Hub->m_HubDescriptor.Usb30));
            HubSendControl(m_Hub, &m_Hub->m_HubDescriptor, sizeof(m_Hub->m_HubDescriptor.Usb30), TRUE);
            break;

        default:
            DPRINT1("Hub %p of speed %lu has no hub descriptor\n", m_Hub, (ULONG)m_Hub->m_Parent.HubSpeed);
            m_Hub->Post(HubEvent::TransferFailed);
            break;
    }
}

VOID
HubMachine::GetStandardStatus()
{
    m_Hub->m_Control.SetSetup(0x80, USB_REQUEST_GET_STATUS, 0, 0, sizeof(m_Hub->m_DeviceStatus));
    DPRINT("Hub %p reading device status\n", m_Hub);
    HubSendControl(m_Hub, &m_Hub->m_DeviceStatus, sizeof(m_Hub->m_DeviceStatus), FALSE);
}

/* The previous copy is kept even when the send then fails */
VOID
HubMachine::GetHubStatus()
{
    m_Hub->m_PreviousHubStatus = m_Hub->m_HubStatus;

    m_Hub->m_Control.SetSetup(0xA0, HUB_REQUEST_GET_STATUS, 0, 0, sizeof(m_Hub->m_HubStatus));
    DPRINT("Hub %p reading hub status\n", m_Hub);
    HubSendControl(m_Hub, &m_Hub->m_HubStatus, sizeof(m_Hub->m_HubStatus), FALSE);
}

/* The parent reports a 1 based depth; the hub wants it 0 based */
VOID
HubMachine::SetHubDepth()
{
    m_Hub->m_Control.SetSetup(0x20, HUB_REQUEST_SET_HUB_DEPTH, (USHORT)(m_Hub->m_Parent.HubDepth - 1), 0, 0);
    DPRINT("Hub %p setting hub depth %lu\n", m_Hub, (ULONG)m_Hub->m_Parent.HubDepth - 1);
    HubSendControl(m_Hub, NULL, 0, FALSE);
}

/* The first read takes 255 bytes, a second one the full length; the buffer is zeroed so a short read leaves no stale wTotalLength */
VOID
HubMachine::GetConfigDescriptor(
    _In_ BOOLEAN FullLength)
{
    PUSB_CONFIGURATION_DESCRIPTOR Previous = m_Hub->m_ConfigDescriptor;
    PUSB_CONFIGURATION_DESCRIPTOR Buffer;
    ULONG Length;
    NTSTATUS Status;

    if (!FullLength)
    {
        if (Previous != NULL)
            ExFreePoolWithTag(Previous, HUB_TAG_HUB);
        Previous = NULL;
        Length = MAXUCHAR;
    }
    else
    {
        Length = Previous->wTotalLength;
    }

    m_Hub->m_ConfigDescriptor = NULL;

    Buffer = (PUSB_CONFIGURATION_DESCRIPTOR)ExAllocatePoolWithTag(NonPagedPool, Length, HUB_TAG_HUB);
    if (Buffer == NULL)
    {
        DPRINT1("Hub %p no memory for a %lu byte configuration descriptor\n", m_Hub, Length);
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        DPRINT("Hub %p reading %lu bytes of configuration descriptor\n", m_Hub, Length);
        RtlZeroMemory(Buffer, Length);
        m_Hub->m_ConfigDescriptor = Buffer;

        m_Hub->m_Control.SetSetup(0x80, USB_REQUEST_GET_DESCRIPTOR, USB_CONFIGURATION_DESCRIPTOR_TYPE << 8, 0, (USHORT)Length);
        Status = m_Hub->m_Control.Send(m_Hub->m_RootHubTarget,
                                       m_Hub->UsbDevice(),
                                       HubControlComplete,
                                       m_Hub,
                                       Buffer,
                                       Length,
                                       TRUE,
                                       m_Hub->NeedsForwardProgress());
    }

    if (Previous != NULL)
        ExFreePoolWithTag(Previous, HUB_TAG_HUB);

    if (NT_SUCCESS(Status))
        return;

    if (m_Hub->m_ConfigDescriptor != NULL)
    {
        ExFreePoolWithTag(m_Hub->m_ConfigDescriptor, HUB_TAG_HUB);
        m_Hub->m_ConfigDescriptor = NULL;
    }

    m_Hub->Post(HubEvent::TransferFailed);
}

/* Despite the name, only wTotalLength is looked at */
BOOLEAN
HubMachine::ConfigDescriptorTruncated()
{
    m_Hub->m_ConfigTotalLength = m_Hub->m_ConfigDescriptor->wTotalLength;
    return m_Hub->m_ConfigTotalLength > MAXUCHAR;
}

/*
 * The length check runs after the validator so both get logged. A read
 * shorter than a configuration descriptor is refused before any copy.
 */
BOOLEAN
HubMachine::CacheConfigDescriptor()
{
    PUSB_CONFIGURATION_DESCRIPTOR Held = m_Hub->m_ConfigDescriptor;
    PUSB_CONFIGURATION_DESCRIPTOR Copy = NULL;
    USHORT TotalLength = Held->wTotalLength;
    BOOLEAN Valid = FALSE;

    if (m_Hub->m_Control.Urb.TransferBufferLength < sizeof(*Held) || TotalLength < sizeof(*Held))
    {
        DPRINT1("Hub %p configuration descriptor too short, %lu bytes, wTotalLength %u\n",
                m_Hub,
                m_Hub->m_Control.Urb.TransferBufferLength,
                TotalLength);
        goto Exit;
    }

    /* The buffer was sized from the first read, so a larger second wTotalLength is rejected */
    if (TotalLength > m_Hub->m_ConfigTotalLength)
    {
        HubLogConfigTotalLengthMismatch(m_Hub);
        goto Exit;
    }

    if (TotalLength < MAXUCHAR)
    {
        Copy = (PUSB_CONFIGURATION_DESCRIPTOR)ExAllocatePoolWithTag(NonPagedPool, TotalLength, HUB_TAG_HUB);
        if (Copy == NULL)
        {
            DPRINT1("Hub %p no memory to cache the configuration descriptor\n", m_Hub);
            goto Exit;
        }

        RtlCopyMemory(Copy, Held, TotalLength);
        m_Hub->m_ConfigDescriptor = Copy;
    }

    if (!HubValidateHubConfiguration(m_Hub, m_Hub->m_ConfigDescriptor, TotalLength))
        goto Exit;

    if (TotalLength != m_Hub->m_ConfigTotalLength)
    {
        HubLogConfigTotalLengthMismatch(m_Hub);
        goto Exit;
    }

    Valid = TRUE;

Exit:
    if (Copy != NULL)
        ExFreePoolWithTag(Held, HUB_TAG_HUB);

    if (!Valid && m_Hub->m_ConfigDescriptor != NULL)
    {
        ExFreePoolWithTag(m_Hub->m_ConfigDescriptor, HUB_TAG_HUB);
        m_Hub->m_ConfigDescriptor = NULL;
    }

    return Valid;
}

/*
 * Flags are only ever set, so a re-read after a hub reset cannot clear one.
 * The validator gets the full buffer size, not the returned length.
 */
BOOLEAN
HubMachine::ParseHubDescriptor()
{
    USHORT Characteristics;
    ULONG OverCurrentMode;

    switch (m_Hub->m_Parent.HubSpeed)
    {
        case UsbFullSpeed:
        case UsbHighSpeed:
            if (!HubCheckUsb2HubDescriptor(m_Hub, &m_Hub->m_HubDescriptor.Usb20, sizeof(m_Hub->m_HubDescriptor.Usb20)))
                return FALSE;

            m_Hub->m_PortCount = m_Hub->m_HubDescriptor.Usb20.bNumberOfPorts;
            Characteristics = m_Hub->m_HubDescriptor.Usb20.wHubCharacteristics;

            if (Characteristics & 0x0001)
                m_Hub->SetFlag(HubFlag::PerPortPower);
            if (Characteristics & 0x0008)
                m_Hub->SetFlag(HubFlag::PerPortOverCurrent);
            return TRUE;

        case UsbSuperSpeed:
            if (!HubCheckUsb3HubDescriptor(m_Hub, &m_Hub->m_HubDescriptor.Usb30, sizeof(m_Hub->m_HubDescriptor.Usb30)))
                return FALSE;

            m_Hub->m_PortCount = m_Hub->m_HubDescriptor.Usb30.bNumberOfPorts;
            Characteristics = m_Hub->m_HubDescriptor.Usb30.wHubCharacteristics;
            OverCurrentMode = (Characteristics >> 3) & 0x3;

            if (Characteristics & 0x0001)
                m_Hub->SetFlag(HubFlag::PerPortPower);
            if (OverCurrentMode == 1)
                m_Hub->SetFlag(HubFlag::PerPortOverCurrent);
            else if (OverCurrentMode >= 2)
                m_Hub->SetFlag(HubFlag::NoOverCurrentProtection);
            return TRUE;

        default:
            DPRINT1("Hub %p of speed %lu has no hub descriptor to parse\n", m_Hub, (ULONG)m_Hub->m_Parent.HubSpeed);
            return FALSE;
    }
}

BOOLEAN
HubMachine::ParsePowerStatus()
{
    m_Hub->m_MaxPortPower = m_Hub->m_DeviceStatus.SelfPowered ? 500 : 100;
    return TRUE;
}

HubCheck
HubMachine::SelectHubChange()
{
    USHORT Status = m_Hub->m_HubStatus.HubStatus.AsUshort16;
    USHORT Change = m_Hub->m_HubStatus.HubChange.AsUshort16;

    if (Change & HS_OVER_CURRENT)
    {
        DPRINT1("Hub %p over current change, hub status 0x%x\n", m_Hub, Status);
        HubNoteHubOverCurrent(m_Hub);
        m_Hub->m_SelectedChange = (Status & HS_OVER_CURRENT) ? HubChange::OverCurrent : HubChange::OverCurrentCleared;
        m_Hub->m_SelectedFeature = HUB_C_OVER_CURRENT;
        return HubCheck::Yes;
    }

    if (Change & HS_LOCAL_POWER_LOST)
    {
        DPRINT("Hub %p local power change, hub status 0x%x\n", m_Hub, Status);
        m_Hub->m_SelectedChange = (Status & HS_LOCAL_POWER_LOST) ? HubChange::PowerLost : HubChange::PowerGood;
        m_Hub->m_SelectedFeature = HUB_C_LOCAL_POWER;
        return HubCheck::Yes;
    }

    if (Change != 0)
        DPRINT1("Hub %p reports unknown hub change 0x%x\n", m_Hub, Change);

    return (Change != 0) ? HubCheck::Error : HubCheck::No;
}

HubChange
HubMachine::PendingHubChange()
{
    return m_Hub->m_SelectedChange;
}

/* A lost change is never reported */
HubChange
HubMachine::LostHubChange()
{
    return HubChange::None;
}

BOOLEAN
HubMachine::OverCurrentCleared()
{
    return (m_Hub->m_HubStatus.HubStatus.AsUshort16 & HS_OVER_CURRENT) == 0;
}

BOOLEAN
HubMachine::IsErrorFatal()
{
    return TRUE;
}

/* Status change interrupt transfer */

static
VOID
NTAPI
HubInterruptComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context)
{
    HubFdo* Hub = (HubFdo*)Context;
    HubInterruptRequest* Interrupt = &Hub->m_Interrupt;
    NTSTATUS Status = Params->IoStatus.Status;

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(Target);

    /* Built by hand; the bitmap helper that initializes it is pageable */
    Interrupt->BitmapBytes = (USHORT)Interrupt->Urb.TransferBufferLength;
    Interrupt->Bitmap.SizeOfBitMap = Interrupt->BitmapBytes * 8;
    Interrupt->Bitmap.Buffer = (PULONG)Interrupt->BitmapBuffer;

    if (!NT_SUCCESS(Status))
    {
        /* The hub cancels its own read when it powers down */
        if (Status == STATUS_CANCELLED)
        {
            DPRINT("Hub %p status change read canceled\n", Hub);
        }
        else
        {
            DPRINT1("Hub %p status change read failed 0x%lx, URB status 0x%lx\n",
                    Hub,
                    Status,
                    Interrupt->Urb.Hdr.Status);
            Hub->m_FailureMessageId = HUB_MSG_INTERRUPT_FAILED;
        }

        Hub->Post(HubEvent::InterruptFailed);
        return;
    }

    Hub->Post(HubEvent::InterruptDone);
}

/*
 * After a transfer with no change bits, the root hub is asked to hold the
 * next one in UCX instead of polling the controller driver.
 */
VOID
HubMachine::StartStatusChangeRead()
{
    HubInterruptRequest* Interrupt = &m_Hub->m_Interrupt;
    WDF_REQUEST_REUSE_PARAMS Reuse;
    IO_STACK_LOCATION Stack;
    NTSTATUS Status;

    WDF_REQUEST_REUSE_PARAMS_INIT(&Reuse, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_SUCCESS);
    WdfRequestReuse(Interrupt->Request, &Reuse);

    RtlZeroMemory(&Interrupt->Urb, sizeof(Interrupt->Urb));
    Interrupt->Urb.Hdr.Length = sizeof(Interrupt->Urb);
    Interrupt->Urb.Hdr.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
    Interrupt->Urb.Hdr.UsbdDeviceHandle = m_Hub->UsbDevice();
    Interrupt->Urb.TransferFlags = USBD_SHORT_TRANSFER_OK;
    if (m_Hub->NeedsForwardProgress())
        Interrupt->Urb.TransferFlags |= USB3_URB_RESERVED_RESOURCES;

    Interrupt->Urb.TransferBuffer = Interrupt->BitmapBuffer;
    Interrupt->Urb.TransferBufferLength = m_Hub->IsRootHub() ? Interrupt->BitmapMaxBytes : m_Hub->m_InterruptMaxPacket;
    Interrupt->Urb.PipeHandle = Interrupt->Pipe;

    if (Interrupt->LastInterruptWasEmpty)
    {
        Interrupt->Urb.Hdr.UsbdFlags |= UCXHUB_URB_FLAG_PEND_INTERRUPT_TX;
        Interrupt->LastInterruptWasEmpty = FALSE;
    }

    RtlZeroMemory(&Stack, sizeof(Stack));
    Stack.MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    Stack.Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
    Stack.Parameters.Others.Argument1 = &Interrupt->Urb;
    WdfRequestWdmFormatUsingStackLocation(Interrupt->Request, &Stack);

    WdfRequestSetCompletionRoutine(Interrupt->Request, HubInterruptComplete, m_Hub);

    if (WdfRequestSend(Interrupt->Request, WdfDeviceGetIoTarget(m_Hub->m_Device), WDF_NO_SEND_OPTIONS))
        return;

    /* A refused send with a success status posts nothing */
    Status = WdfRequestGetStatus(Interrupt->Request);
    WdfRequestReuse(Interrupt->Request, &Reuse);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p status change read not sent 0x%lx\n", m_Hub, Status);
        m_Hub->Post(HubEvent::InterruptFailed);
    }
}

/* The completion still posts exactly one event */
VOID
HubMachine::CancelStatusChangeRead()
{
    if (!WdfRequestCancelSentRequest(m_Hub->m_Interrupt.Request))
        DPRINT1("Hub %p status change read not canceled\n", m_Hub);
}

/* Bit 0 is read even after a zero length completion */
HubCheck
HubMachine::CheckHubChangeBit()
{
    HubInterruptRequest* Interrupt = &m_Hub->m_Interrupt;
    HubCheck Result = (Interrupt->BitmapBuffer[0] & 1) ? HubCheck::Yes : HubCheck::No;
    ULONG Bit;

    for (Bit = 1; Bit < (ULONG)Interrupt->BitmapBytes * 8; Bit++)
    {
        if (!(Interrupt->BitmapBuffer[Bit / 8] & (1 << (Bit % 8))))
            continue;

        if (m_Hub->FindPort(Bit) == NULL)
        {
            DPRINT1("Hub %p reports a change on missing port %lu\n", m_Hub, Bit);
            return HubCheck::Error;
        }
    }

    return Result;
}

/* Client and user mode port status */

/* Reads into the target port's own status buffer */
VOID
HubMachine::GetPortStatus()
{
    HubPort* Port = m_Hub->m_PortStatusTarget;
    BOOLEAN Extended = Port->HasProperty(PortProperty::EnhancedSuperSpeed);
    USHORT Length = Extended ? sizeof(Port->m_Current) : sizeof(Port->m_Current.StatusChange);

    m_Hub->m_Control.SetSetup(0xA3, HUB_REQUEST_GET_STATUS, Extended ? 2 : 0, Port->Number(), Length);
    HubSendControl(m_Hub, &Port->m_Current, Length, FALSE);
}

static
WDFREQUEST
NTAPI
HubTakePortStatusRequest(
    _In_ HubFdo* Hub,
    _Out_ PWDF_REQUEST_PARAMETERS Params)
{
    WDFREQUEST Request = Hub->m_PortStatusRequest;

    Hub->m_PortStatusRequest = NULL;

    WDF_REQUEST_PARAMETERS_INIT(Params);
    WdfRequestGetParameters(Request, Params);
    return Request;
}

/* A child sees a port that lost power as disabled once, so its driver resets it */
VOID
HubMachine::ReplyPortStatus()
{
    HubPort* Port = m_Hub->m_PortStatusTarget;
    USHORT Status = Port->m_Current.StatusChange.PortStatus.AsUshort16;
    USHORT Change = Port->m_Current.StatusChange.PortChange.AsUshort16;
    WDF_REQUEST_PARAMETERS Params;
    WDFREQUEST Request = HubTakePortStatusRequest(m_Hub, &Params);
    WDFMEMORY Memory;
    NTSTATUS Result;
    PULONG Bits;
    HubPdo* Pdo;

    switch (Params.Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_USB_GET_PORT_STATUS:
            Result = WdfRequestRetrieveOutputMemory(Request, &Memory);
            if (!NT_SUCCESS(Result))
            {
                DPRINT1("Hub %p port %u status output buffer missing 0x%lx\n", m_Hub, Port->Number(), Result);
                WdfRequestComplete(Request, Result);
                return;
            }

            *(PUSHORT)((PUCHAR)WdfMemoryGetBuffer(Memory, NULL) + HUB_USER_PORT_STATUS_OFFSET) = Status;
            WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, HUB_USER_PORT_STATUS_SIZE);
            return;

        case IOCTL_INTERNAL_USB_GET_PORT_STATUS:
            Bits = (PULONG)Params.Parameters.Others.Arg1;
            Pdo = (HubPdo*)Params.Parameters.Others.Arg2;
            Result = STATUS_SUCCESS;

            if (Status & PS_CONNECTED)
                *Bits |= USBD_PORT_CONNECTED;
            else
                Result = STATUS_NO_SUCH_DEVICE;

            if (Status & PS_ENABLED)
            {
                if (Pdo->m_Flags & (LONG)PdoFlag::ReportPortDisabled)
                    InterlockedAnd(&Pdo->m_Flags, ~(LONG)PdoFlag::ReportPortDisabled);
                else
                    *Bits |= USBD_PORT_ENABLED;
            }

            if (Change & PC_CONNECT)
                Result = STATUS_NO_SUCH_DEVICE;

            if (!NT_SUCCESS(Result))
            {
                DPRINT1("Hub %p port %u status 0x%x change 0x%x reported as no device\n",
                        m_Hub,
                        Port->Number(),
                        Status,
                        Change);
            }

            WdfRequestComplete(Request, Result);
            return;

        default:
            DPRINT1("Hub %p port status request with IOCTL 0x%lx\n",
                    m_Hub,
                    Params.Parameters.DeviceIoControl.IoControlCode);
            WdfRequestComplete(Request, STATUS_ADAPTER_HARDWARE_ERROR);
            return;
    }
}

VOID
HubMachine::FailPortStatus(
    _In_ BOOLEAN HardwareError)
{
    WDF_REQUEST_PARAMETERS Params;
    WDFREQUEST Request = HubTakePortStatusRequest(m_Hub, &Params);

    if (Params.Parameters.DeviceIoControl.IoControlCode == IOCTL_INTERNAL_USB_GET_PORT_STATUS)
        *(PULONG)Params.Parameters.Others.Arg1 = 0;

    DPRINT1("Hub %p port %u status request failed, hardware error %u\n",
            m_Hub,
            m_Hub->m_PortStatusTarget->Number(),
            HardwareError);

    WdfRequestComplete(Request, HardwareError ? STATUS_ADAPTER_HARDWARE_ERROR : STATUS_NO_SUCH_DEVICE);
}
