/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Endpoints, static streams and the endpoint machine actions
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include "xhcidrv.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

#define XHCI_TAG_ENDPOINT               'eHCX'

/* Facts kept in XhciEndpoint::m_Flags until the next mapping start */
#define XHCI_EP_CANCEL_REPORTED         0x00000001
#define XHCI_EP_FAILURE_REPORTED        0x00000002
#define XHCI_EP_STREAM_HALT_PENDING     0x00000004
#define XHCI_EP_STOPPED_HANDSHAKE       0x00000008
#define XHCI_EP_STOPPED_RECEIVED        0x00000010
#define XHCI_EP_HALT_RECEIVED           0x00000020
#define XHCI_EP_IGNORE_STOPPED          0x00000040
#define XHCI_EP_RESET_ACK_HANDSHAKE     0x00000080
#define XHCI_EP_CLIENT_RESET_HANDSHAKE  0x00000100

/* Hardware verifier conditions raised here */
#define XHCI_VERIFY_CONTROL_DEQUEUE     0x00000080
#define XHCI_VERIFY_RESET_ENDPOINT      0x00000100
#define XHCI_VERIFY_CANCEL_DEQUEUE      0x00000200
#define XHCI_VERIFY_STOP_EXHAUSTED      0x00000400
#define XHCI_VERIFY_EVENT_CODE          0x00000800
#define XHCI_VERIFY_STOP_ENDPOINT       0x00800000
#define XHCI_VERIFY_DUPLICATE_EVENT     0x02000000

/* Recovery reasons reported with a controller reset */
#define XHCI_REASON_NO_LIVE_DUMP        0x0000
#define XHCI_REASON_STOP_ERROR          0x1003
#define XHCI_REASON_STOP_CONTEXT        0x1004
#define XHCI_REASON_CONTROL_DEQUEUE     0x1005
#define XHCI_REASON_CONTROL_RESET       0x1006
#define XHCI_REASON_CANCEL_DEQUEUE      0x100A
#define XHCI_REASON_EVENT_CODE          0x1010
#define XHCI_REASON_DROP_FAILED         0x1017
#define XHCI_REASON_CLIENT_DEQUEUE      0x101A
#define XHCI_REASON_CLIENT_RESET        0x101B
#define XHCI_REASON_STOP_ADD_FAILED     0x101D
#define XHCI_REASON_EVENT_DATA_POINTER  0x101E
#define XHCI_REASON_EVENT_POINTER       0x101F
#define XHCI_REASON_DUPLICATE_POINTER   0x1020
#define XHCI_REASON_STREAM_RECONFIGURE  0x1021
#define XHCI_REASON_RECONFIGURE         0x1022
#define XHCI_REASON_NOT_STOPPED         0x1023

#define XHCI_RECLAIM_TAG                reinterpret_cast<PVOID>(static_cast<ULONG_PTR>('lcRX'))
#define XHCI_STOP_ATTEMPTS_MAX          20
#define XHCI_CLEAR_STALL_TIMEOUT_MS     5000
#define XHCI_CLEAR_STALL_STACK_SIZE     4

/* Reuse flag our 1.17 headers lack; the framework accepts it */
#define XHCI_REQUEST_REUSE_MUST_COMPLETE 0x00000002

/** One stream of a streams record: its ring and the block for its Set TR Dequeue Pointer. */
struct XhciStreamEntry
{
    XhciTransferRing* Ring;
    XhciCommand Command;
};

/**
 * Streams of an endpoint, stream id s at Entry[s - 1]. The client record lives in the
 * UCXSSTREAMS context, the default one (one stream, the main ring) in pool.
 */
struct XhciStreams
{
    XhciEndpoint* Endpoint;
    ULONG Count;
    ULONG MaxPrimary;
    volatile LONG StreamsLeftToFlush;
    volatile LONG UpdateCount;
    ULONG HaltedCode;
    XhciDmaBuffer* ContextArray;
    XhciStreamEntry Entry[1];
};

/** Pool block used to send CLEAR_FEATURE(ENDPOINT_HALT) through the default endpoint. */
struct XhciEndpoint::HaltClearBlock
{
    PIRP Irp;
    WDFREQUEST Request;
    struct _URB_CONTROL_TRANSFER_EX Urb;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(XhciEndpoint, XhciGetEndpoint);
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(XhciStreams, XhciGetStreams);

/* FUNCTIONS ******************************************************************/

static
ULONG
NTAPI
XhciStreamsSize(
    _In_ ULONG Count)
{
    return FIELD_OFFSET(XhciStreams, Entry) + Count * sizeof(XhciStreamEntry);
}

/** MaxPStreams for Count streams; the array then holds 2^(MaxPStreams + 1) entries. */
static
ULONG
NTAPI
XhciMaxPrimaryStreams(
    _In_ ULONG Count)
{
    ULONG Value = 1;

    while ((Count >> (Value + 1)) != 0)
        Value++;

    return Value;
}

static
ULONG
NTAPI
XhciStreamArrayBytes(
    _In_ ULONG MaxPrimary)
{
    return sizeof(XHCI_STREAM_CONTEXT) << (MaxPrimary + 1);
}

static
BOOLEAN
NTAPI
XhciIsHaltedCode(
    _In_ ULONG Code)
{
    switch (static_cast<XhciCompletionCode>(Code))
    {
        case XhciCompletionCode::DataBufferError:
        case XhciCompletionCode::BabbleDetected:
        case XhciCompletionCode::UsbTransactionError:
        case XhciCompletionCode::StallError:
        case XhciCompletionCode::InvalidStreamType:
        case XhciCompletionCode::EventLost:
        case XhciCompletionCode::InvalidStreamId:
        case XhciCompletionCode::SplitTransactionError:
            return TRUE;

        default:
            return FALSE;
    }
}

static
BOOLEAN
NTAPI
XhciIsStoppedCode(
    _In_ ULONG Code)
{
    return (Code == static_cast<ULONG>(XhciCompletionCode::Stopped)) ||
           (Code == static_cast<ULONG>(XhciCompletionCode::StoppedLengthInvalid)) ||
           (Code == static_cast<ULONG>(XhciCompletionCode::StoppedShortPacket));
}

/** Dword 3 of an endpoint scoped command TRB. */
static
ULONG
NTAPI
XhciEndpointCommandControl(
    _In_ enum XhciTrbType Type,
    _In_ ULONG Dci,
    _In_ ULONG SlotId)
{
    return (static_cast<ULONG>(Type) << XHCI_TRB_TYPE_SHIFT) |
           (Dci << XHCI_TRB_ENDPOINT_SHIFT) |
           (SlotId << XHCI_TRB_SLOT_SHIFT);
}

static
PURB
NTAPI
XhciRequestUrb(
    _In_ WDFREQUEST Request)
{
    WDF_REQUEST_PARAMETERS Parameters;

    WDF_REQUEST_PARAMETERS_INIT(&Parameters);
    WdfRequestGetParameters(Request, &Parameters);
    return static_cast<PURB>(Parameters.Parameters.Others.Arg1);
}

/** Default stream record whose single stream is the main ring. NULL when out of memory. */
static
XhciStreams*
NTAPI
XhciNewDefaultStreams(
    _In_ XhciEndpoint* Endpoint,
    _In_ XhciTransferRing* MainRing,
    _In_ ULONG ArrayMaxPrimary)
{
    XhciStreams* Streams;

    Streams = static_cast<XhciStreams*>(ExAllocatePoolWithTag(NonPagedPool,
                                                              XhciStreamsSize(1),
                                                              XHCI_TAG_ENDPOINT));
    if (Streams == NULL)
        return NULL;

    RtlZeroMemory(Streams, XhciStreamsSize(1));
    Streams->Endpoint = Endpoint;
    Streams->Count = 1;
    Streams->MaxPrimary = 1;
    Streams->Entry[0].Ring = MainRing;

    Streams->ContextArray =
        Endpoint->Controller()->m_Buffers.Acquire(XhciStreamArrayBytes(ArrayMaxPrimary));
    if (Streams->ContextArray == NULL)
    {
        ExFreePoolWithTag(Streams, XHCI_TAG_ENDPOINT);
        return NULL;
    }

    return Streams;
}

/* Accessors ******************************************************************/

XhciEndpoint*
XhciEndpoint::FromUcx(
    _In_ UCXENDPOINT Endpoint)
{
    return XhciGetEndpoint(Endpoint);
}

XhciController*
XhciEndpoint::Controller() const
{
    return m_Controller;
}

XhciUsbDevice*
XhciEndpoint::Device() const
{
    return m_Device;
}

UCXENDPOINT
XhciEndpoint::Handle() const
{
    return m_Ucx;
}

ULONG
XhciEndpoint::Dci() const
{
    return m_Dci;
}

ULONG
XhciEndpoint::TransferType() const
{
    return m_Descriptor.bmAttributes & USB_ENDPOINT_TYPE_MASK;
}

const USB_ENDPOINT_DESCRIPTOR*
XhciEndpoint::Descriptor() const
{
    return &m_Descriptor;
}

USHORT
XhciEndpoint::MaxPacketSize() const
{
    return m_Descriptor.wMaxPacketSize & 0x7FF;
}

ULONG
XhciEndpoint::MaxBurst() const
{
    /* The bulk ring asks at enable, before the first context was built */
    if (m_Device->Speed() >= UsbSuperSpeed)
        return m_Companion.bMaxBurst;

    return m_MaxBurst;
}

ULONG
XhciEndpoint::MaxEsitPayload() const
{
    return m_MaxPayload;
}

BOOLEAN
XhciEndpoint::IsStreamsCapable() const
{
    return m_StreamsCapable;
}

/* Creation and cleanup *******************************************************/

NTSTATUS
XhciEndpoint::Create(
    _In_ UCXCONTROLLER UcxController,
    _In_ UCXUSBDEVICE UcxUsbDevice,
    _In_ PUCXENDPOINT_INIT EndpointInit,
    _In_ const USB_ENDPOINT_DESCRIPTOR* Descriptor,
    _In_opt_ const USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR* Companion,
    _In_opt_ const USB_SUPERSPEEDPLUS_ISOCH_ENDPOINT_COMPANION_DESCRIPTOR* IsochCompanion)
{
    WDF_OBJECT_ATTRIBUTES Attributes;
    UCXENDPOINT Endpoint;
    XhciEndpoint* Self;
    NTSTATUS Status;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, XhciEndpoint);
    Attributes.EvtCleanupCallback = EvtCleanup;

    Status = UcxEndpointCreate(UcxUsbDevice, &EndpointInit, &Attributes, &Endpoint);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("UcxEndpointCreate for endpoint 0x%02x failed 0x%lx\n",
                Descriptor->bEndpointAddress, Status);
        return Status;
    }

    Self = new (XhciGetEndpoint(Endpoint)) XhciEndpoint;
    Self->m_Machine.Initialize(Self);

    /* UCX deletes the endpoint on failure; the cleanup callback frees what was made */
    return Self->Initialize(XhciController::FromUcx(UcxController),
                            UcxUsbDevice,
                            Endpoint,
                            Descriptor,
                            Companion,
                            IsochCompanion);
}

NTSTATUS
XhciEndpoint::Initialize(
    _In_ XhciController* Controller,
    _In_ UCXUSBDEVICE UcxUsbDevice,
    _In_ UCXENDPOINT Endpoint,
    _In_ const USB_ENDPOINT_DESCRIPTOR* Descriptor,
    _In_opt_ const USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR* Companion,
    _In_opt_ const USB_SUPERSPEEDPLUS_ISOCH_ENDPOINT_COMPANION_DESCRIPTOR* IsochCompanion)
{
    WDF_OBJECT_ATTRIBUTES Attributes;
    WDF_TIMER_CONFIG TimerConfig;
    ULONG Type;
    ULONG Number;
    NTSTATUS Status;

    m_Controller = Controller;
    m_UcxDevice = UcxUsbDevice;
    m_Device = XhciUsbDevice::FromUcx(UcxUsbDevice);
    m_Ucx = Endpoint;
    m_ResetStatus = STATUS_PENDING;
    KeInitializeEvent(&m_ResetAck, NotificationEvent, FALSE);

    WDF_TIMER_CONFIG_INIT(&TimerConfig, EvtTimer);
    TimerConfig.AutomaticSerialization = FALSE;
    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Endpoint;

    Status = WdfTimerCreate(&TimerConfig, &Attributes, &m_Timer);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Endpoint %p state timer creation failed 0x%lx\n", this, Status);
        return Status;
    }

    RtlCopyMemory(&m_Descriptor, Descriptor, sizeof(m_Descriptor));
    if (Companion != NULL)
        RtlCopyMemory(&m_Companion, Companion, sizeof(m_Companion));
    if (IsochCompanion != NULL)
        RtlCopyMemory(&m_IsochCompanion, IsochCompanion, sizeof(m_IsochCompanion));

    /* EP Type (xHCI 6.2.3): OUT types are 1..3, IN adds 4, control is 4 either way */
    Type = TransferType();
    m_XhciType = Type;
    if ((Type == USB_ENDPOINT_TYPE_CONTROL) || USB_ENDPOINT_DIRECTION_IN(m_Descriptor.bEndpointAddress))
        m_XhciType += 4;

    /* DCI (xHCI 4.5.1): control and IN endpoints are odd */
    Number = m_Descriptor.bEndpointAddress & USB_ENDPOINT_ADDRESS_MASK;
    m_Dci = Number * 2;
    if (m_XhciType >= XHCI_ENDPOINT_TYPE_CONTROL)
        m_Dci++;

    /* Streams also need controller support */
    m_StreamsCapable = (m_Device->Speed() >= UsbSuperSpeed) &&
                       (Type == USB_ENDPOINT_TYPE_BULK) &&
                       (m_Companion.bmAttributes.Bulk.MaxStreams != 0) &&
                       (m_Controller->m_Registers.SupportedStreams() != 0);

    if (m_StreamsCapable && m_Controller->HasErrata(XhciErrata::StrmReconfigOnStop))
    {
        Status = CreateClearStall();
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Endpoint %p clear stall context creation failed 0x%lx\n", this, Status);
            return Status;
        }
    }

    /* The main ring of a streams capable endpoint is stream 1 of the default record */
    Status = XhciTransferRing::Create(this, Endpoint, m_StreamsCapable ? 1 : 0, &m_MainRing);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Endpoint %p DCI %lu main ring creation failed 0x%lx\n", this, m_Dci, Status);
        return Status;
    }

    UcxEndpointSetWdfIoQueue(Endpoint, m_MainRing->Queue());

    m_Device->AddEndpoint(this);
    m_Listed = TRUE;

    DPRINT("Endpoint %p created, address 0x%02x DCI %lu type %lu\n",
           this, m_Descriptor.bEndpointAddress, m_Dci, m_XhciType);
    return STATUS_SUCCESS;
}

NTSTATUS
XhciEndpoint::CreateClearStall()
{
    WDF_OBJECT_ATTRIBUTES Attributes;
    NTSTATUS Status;

    m_ClearStall = static_cast<HaltClearBlock*>(ExAllocatePoolWithTag(NonPagedPool,
                                                                         sizeof(*m_ClearStall),
                                                                         XHCI_TAG_ENDPOINT));
    if (m_ClearStall == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(m_ClearStall, sizeof(*m_ClearStall));

    m_ClearStall->Irp = IoAllocateIrp(XHCI_CLEAR_STALL_STACK_SIZE, FALSE);
    if (m_ClearStall->Irp == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, XhciRequestData);
    Status = WdfRequestCreate(&Attributes, NULL, &m_ClearStall->Request);
    if (!NT_SUCCESS(Status))
        m_ClearStall->Request = NULL;

    return Status;
}

VOID
NTAPI
XhciEndpoint::EvtCleanup(
    _In_ WDFOBJECT Object)
{
    XhciEndpoint* Self = XhciGetEndpoint(Object);

    if (Self->m_Ucx != Object)
        return;

    Self->Cleanup();
}

/* Also frees what a failed create left behind */
VOID
XhciEndpoint::Cleanup()
{
    if (m_ClearStall != NULL)
    {
        if (m_ClearStall->Request != NULL)
            WdfObjectDelete(m_ClearStall->Request);
        if (m_ClearStall->Irp != NULL)
            IoFreeIrp(m_ClearStall->Irp);

        ExFreePoolWithTag(m_ClearStall, XHCI_TAG_ENDPOINT);
        m_ClearStall = NULL;
    }

    if (m_CurrentStreams == m_DefaultStreams)
        m_CurrentStreams = NULL;
    FreeDefaultStreams(m_DefaultStreams);
    m_DefaultStreams = NULL;

    if (m_StopInput != NULL)
    {
        m_Controller->m_Buffers.Free(m_StopInput);
        m_StopInput = NULL;
    }

    /* Last: dropping the list reference may free this object */
    if (m_Listed)
    {
        m_Listed = FALSE;
        m_Device->RemoveEndpoint(this);
    }
}

VOID
XhciEndpoint::FreeDefaultStreams(
    _In_opt_ XhciStreams* Streams)
{
    if (Streams == NULL)
        return;

    if (Streams->ContextArray != NULL)
        m_Controller->m_Buffers.Free(Streams->ContextArray);

    ExFreePoolWithTag(Streams, XHCI_TAG_ENDPOINT);
}

VOID
NTAPI
XhciEndpoint::EvtTimer(
    _In_ WDFTIMER Timer)
{
    XhciEndpoint* Self = XhciGetEndpoint(WdfTimerGetParentObject(Timer));

    Self->PostEvent(XepEvent::TimerFired);
}

/* UCX endpoint add callbacks *************************************************/

NTSTATUS
NTAPI
XhciEvtUsbDeviceDefaultEndpointAdd(
    _In_ UCXCONTROLLER UcxController,
    _In_ UCXUSBDEVICE UcxUsbDevice,
    _In_ ULONG MaxPacketSize,
    _In_ PUCXENDPOINT_INIT UcxEndpointInit)
{
    UCX_DEFAULT_ENDPOINT_EVENT_CALLBACKS Callbacks;
    USB_ENDPOINT_DESCRIPTOR Descriptor;
    NTSTATUS Status;

    RtlZeroMemory(&Descriptor, sizeof(Descriptor));
    Descriptor.bLength = sizeof(Descriptor);
    Descriptor.bDescriptorType = USB_ENDPOINT_DESCRIPTOR_TYPE;
    Descriptor.wMaxPacketSize = static_cast<USHORT>(MaxPacketSize);

    UCX_DEFAULT_ENDPOINT_EVENT_CALLBACKS_INIT(&Callbacks,
                                              XhciEndpoint::EvtPurge,
                                              XhciEndpoint::EvtStart,
                                              XhciEndpoint::EvtAbort,
                                              XhciEndpoint::EvtOkToCancel,
                                              XhciEndpoint::EvtDefaultUpdate);

    /* The forward progress hook travels in the reserved slot */
    Callbacks.Reserved1 = reinterpret_cast<HANDLE>(XhciEndpoint::EvtEnableForwardProgress);
    UcxDefaultEndpointInitSetEventCallbacks(UcxEndpointInit, &Callbacks);

    Status = XhciEndpoint::Create(UcxController, UcxUsbDevice, UcxEndpointInit, &Descriptor, NULL, NULL);
    if (!NT_SUCCESS(Status))
        DPRINT1("Default endpoint creation failed 0x%lx\n", Status);

    return Status;
}

NTSTATUS
NTAPI
XhciEvtUsbDeviceEndpointAdd(
    _In_ UCXCONTROLLER UcxController,
    _In_ UCXUSBDEVICE UcxUsbDevice,
    _In_reads_bytes_(UsbEndpointDescriptorBufferLength) PUSB_ENDPOINT_DESCRIPTOR UsbEndpointDescriptor,
    _In_ ULONG UsbEndpointDescriptorBufferLength,
    _In_opt_ PUSB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR SuperSpeedEndpointCompanionDescriptor,
    _In_ PUCXENDPOINT_INIT UcxEndpointInit)
{
    PUSB_SUPERSPEEDPLUS_ISOCH_ENDPOINT_COMPANION_DESCRIPTOR IsochCompanion = NULL;
    UCX_ENDPOINT_EVENT_CALLBACKS Callbacks;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(UsbEndpointDescriptorBufferLength);

    UCX_ENDPOINT_EVENT_CALLBACKS_INIT(&Callbacks,
                                      XhciEndpoint::EvtPurge,
                                      XhciEndpoint::EvtStart,
                                      XhciEndpoint::EvtAbort,
                                      XhciEndpoint::EvtReset,
                                      XhciEndpoint::EvtOkToCancel,
                                      XhciEndpoint::EvtStreamsAdd,
                                      XhciEndpoint::EvtStreamsEnable,
                                      XhciEndpoint::EvtStreamsDisable);
    Callbacks.Reserved1 = reinterpret_cast<HANDLE>(XhciEndpoint::EvtEnableForwardProgress);
    UcxEndpointInitSetEventCallbacks(UcxEndpointInit, &Callbacks);

    /* QUIRK: the SSP isoch companion is assumed to follow the companion, unchecked against the length */
    if ((SuperSpeedEndpointCompanionDescriptor != NULL) &&
        ((UsbEndpointDescriptor->bmAttributes & USB_ENDPOINT_TYPE_MASK) == USB_ENDPOINT_TYPE_ISOCHRONOUS) &&
        SuperSpeedEndpointCompanionDescriptor->bmAttributes.Isochronous.SspCompanion)
    {
        IsochCompanion = reinterpret_cast<PUSB_SUPERSPEEDPLUS_ISOCH_ENDPOINT_COMPANION_DESCRIPTOR>(
            reinterpret_cast<PUCHAR>(SuperSpeedEndpointCompanionDescriptor) +
            SuperSpeedEndpointCompanionDescriptor->bLength);
    }

    Status = XhciEndpoint::Create(UcxController,
                                  UcxUsbDevice,
                                  UcxEndpointInit,
                                  UsbEndpointDescriptor,
                                  SuperSpeedEndpointCompanionDescriptor,
                                  IsochCompanion);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Endpoint 0x%02x creation failed 0x%lx\n",
                UsbEndpointDescriptor->bEndpointAddress, Status);
    }

    return Status;
}

/* Enable, disable and context construction ***********************************/

NTSTATUS
XhciEndpoint::Enable()
{
    XhciTransferRing* Ring;
    XhciStreams* Streams;
    UCXSSTREAMS UcxStreams;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Index;

    /* UCX decides whether client streams are open; they may close while we are disabled */
    UcxStreams = UcxEndpointGetStaticStreamsReferenced(m_Ucx, this);

    if (!m_StreamsCapable)
    {
        Status = m_MainRing->Enable();
    }
    else if (UcxStreams == NULL)
    {
        /* A default record kept across a disable for forward progress is reused as is */
        if (m_DefaultStreams == NULL)
        {
            m_DefaultStreams = XhciNewDefaultStreams(this, m_MainRing, 1);
            m_CurrentStreams = m_DefaultStreams;
            if (m_DefaultStreams == NULL)
                Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else if (m_CurrentStreams == NULL)
        {
            m_CurrentStreams = m_DefaultStreams;
        }

        if (NT_SUCCESS(Status))
            Status = m_MainRing->Enable();

        if (NT_SUCCESS(Status))
            StreamContext(1)->DequeuePointer = m_MainRing->DequeuePointerValue();
    }
    else
    {
        Streams = XhciGetStreams(UcxStreams);
        NT_ASSERT(m_CurrentStreams == NULL);
        m_CurrentStreams = Streams;

        if (Streams->ContextArray == NULL)
        {
            Streams->ContextArray =
                m_Controller->m_Buffers.Acquire(XhciStreamArrayBytes(Streams->MaxPrimary));
            if (Streams->ContextArray == NULL)
                Status = STATUS_INSUFFICIENT_RESOURCES;
        }

        for (Index = 0; NT_SUCCESS(Status) && (Index < Streams->Count); Index++)
        {
            Ring = Streams->Entry[Index].Ring;
            Status = Ring->Enable();
            if (NT_SUCCESS(Status))
                StreamContext(Index + 1)->DequeuePointer = Ring->DequeuePointerValue();
        }
    }

    if (NT_SUCCESS(Status))
    {
        PostEvent(XepEvent::Enable);
    }
    else
    {
        DPRINT1("Endpoint %p DCI %lu enable failed 0x%lx\n", this, m_Dci, Status);
        DisableRings(TRUE);
    }

    if (UcxStreams != NULL)
        WdfObjectDereferenceWithTag(UcxStreams, this);

    return Status;
}

VOID
XhciEndpoint::Disable()
{
    Disable(FALSE);
}

VOID
XhciEndpoint::Disable(
    _In_ BOOLEAN SlotDisabling)
{
    WDFREQUEST Pending;

    /* Complete a client reset that raced a controller reset so it does not stay pending */
    Pending = static_cast<WDFREQUEST>(
        InterlockedExchangePointer(reinterpret_cast<PVOID volatile*>(&m_ResetRequest), NULL));
    if (Pending != NULL)
    {
        DPRINT1("Endpoint %p DCI %lu disabled with a client reset pending; completing it\n", this, m_Dci);
        WdfRequestComplete(Pending, STATUS_SUCCESS);
    }

    DisableRings(SlotDisabling);
    PostEvent(XepEvent::Disable);
}

VOID
XhciEndpoint::DisableRings(
    _In_ BOOLEAN SlotDisabling)
{
    BOOLEAN FreeResources;
    ULONG Index;

    /* Forward progress keeps the ring resources unless the whole device goes away */
    FreeResources = !m_ForwardProgress || SlotDisabling;

    if (!m_StreamsCapable)
    {
        m_MainRing->Disable(FreeResources);
        return;
    }

    if (m_CurrentStreams == NULL)
        return;

    for (Index = 0; Index < m_CurrentStreams->Count; Index++)
        m_CurrentStreams->Entry[Index].Ring->Disable(FreeResources);

    if (!FreeResources)
        return;

    if (m_CurrentStreams == m_DefaultStreams)
    {
        FreeDefaultStreams(m_DefaultStreams);
        m_DefaultStreams = NULL;
    }
    else if (m_CurrentStreams->ContextArray != NULL)
    {
        m_Controller->m_Buffers.Free(m_CurrentStreams->ContextArray);
        m_CurrentStreams->ContextArray = NULL;
    }

    m_CurrentStreams = NULL;
}

VOID
XhciEndpoint::BuildContext(
    _Out_writes_bytes_(ContextSize) PVOID EndpointContext,
    _In_ ULONG ContextSize) const
{
    PXHCI_ENDPOINT_CONTEXT Context = static_cast<PXHCI_ENDPOINT_CONTEXT>(EndpointContext);
    USB_DEVICE_SPEED Speed = m_Device->Speed();
    ULONG Type = TransferType();
    ULONG Packet = MaxPacketSize();
    ULONG Period = m_Descriptor.bInterval;
    BOOLEAN Periodic;
    ULONG Interval = 0;
    ULONG Burst = 0;
    ULONG Mult = 0;
    ULONG Esit = 0;

    NT_ASSERT(ContextSize >= sizeof(*Context));
    UNREFERENCED_PARAMETER(ContextSize);

    Periodic = (Type == USB_ENDPOINT_TYPE_INTERRUPT) || (Type == USB_ENDPOINT_TYPE_ISOCHRONOUS);

    Context->EndpointType = m_XhciType;
    Context->MaxPacketSize = Packet;

    /*
     * Isochronous endpoints never retry. The interrupt IN of one scanner behind an external
     * hub also gets unlimited retries on flagged controllers.
     */
    if (Type == USB_ENDPOINT_TYPE_ISOCHRONOUS)
    {
        Context->ErrorCount = 0;
    }
    else if (Type == USB_ENDPOINT_TYPE_INTERRUPT &&
             USB_ENDPOINT_DIRECTION_IN(m_Descriptor.bEndpointAddress) &&
             m_Controller->HasErrata(XhciErrata::EpInfiniteCerr) &&
             m_Device->DeviceDescriptor()->idVendor == 0x05F9 &&
             m_Device->DeviceDescriptor()->idProduct == 0x1101 &&
             m_Device->PortPath()->PortPathDepth > 1)
    {
        Context->ErrorCount = 0;
    }
    else
    {
        Context->ErrorCount = 3;
    }

    if (Speed == UsbLowSpeed)
    {
        if (Type == USB_ENDPOINT_TYPE_INTERRUPT)
        {
            Interval = (Period < 16) ? 6 : ((Period < 32) ? 7 : 8);
            Esit = Packet;
        }

        m_MaxPayload = Packet;
    }
    else if (Speed == UsbFullSpeed)
    {
        if (Type == USB_ENDPOINT_TYPE_INTERRUPT)
        {
            /* Frames to 125 us units as a power of two, 1 ms (3) at the least */
            Interval = 3;
            while ((Interval < 8) && (Period >= (2U << (Interval - 3))))
                Interval++;
            Esit = Packet;
        }
        else if (Type == USB_ENDPOINT_TYPE_ISOCHRONOUS)
        {
            /* QUIRK: full speed isoch is always serviced every frame */
            Interval = 3;
            Esit = Packet;
        }

        m_MaxPayload = Packet;
    }
    else if (Speed == UsbHighSpeed)
    {
        if (Periodic)
        {
            /* QUIRK: bInterval 6 and up is clamped to 32 microframes */
            Interval = (Period <= 1) ? 0 : ((Period <= 5) ? (Period - 1) : 5);
            Burst = (m_Descriptor.wMaxPacketSize >> 11) & 3;
            Esit = Packet * (Burst + 1);
        }

        m_MaxPayload = Packet * (Burst + 1);
    }
    else
    {
        Burst = m_Companion.bMaxBurst;

        if (Periodic)
        {
            Interval = (Period <= 1) ? 0 : ((Period <= 5) ? (Period - 1) : 5);

            if (m_Companion.bmAttributes.Isochronous.SspCompanion)
            {
                Esit = m_IsochCompanion.dwBytesPerInterval;
            }
            else
            {
                Esit = m_Companion.wBytesPerInterval;
                Mult = m_Companion.bmAttributes.Isochronous.Mult;
            }
        }

        /* QUIRK: everything but isoch ignores the burst here */
        m_MaxPayload = (Type == USB_ENDPOINT_TYPE_ISOCHRONOUS) ? Esit : Packet;
    }

    if (m_Controller->HasErrata(XhciErrata::EpClampIntervalTo7) && (Interval > 7))
        Interval = 7;

    m_MaxBurst = Burst;

    Context->Interval = Interval;
    Context->MaxBurstSize = Burst;
    Context->Mult = Mult;
    Context->MaxEsitPayloadLo = Esit & 0xFFFF;
    Context->MaxEsitPayloadHi = (Esit >> 16) & 0xFF;

    /* QUIRK: control and bulk get an Average TRB Length of 0 */
    Context->AverageTrbLength = Esit / 2;

    if (m_StreamsCapable && (m_CurrentStreams != NULL))
        Context->MaxPStreams = m_CurrentStreams->MaxPrimary;

    Context->TRDequeuePointer = EndpointDequeuePointer();
    Context->LinearStreamArray = (Context->MaxPStreams != 0) ? 1 : 0;
}

ULONG64
XhciEndpoint::EndpointDequeuePointer() const
{
    if (!m_StreamsCapable)
        return m_MainRing->DequeuePointerValue();

    if ((m_CurrentStreams == NULL) || (m_CurrentStreams->ContextArray == NULL))
        return 0;

    return m_CurrentStreams->ContextArray->LogicalAddress.QuadPart;
}

/* Rings and stream contexts **************************************************/

BOOLEAN
XhciEndpoint::UsesStreams() const
{
    return m_StreamsCapable && (m_CurrentStreams != NULL);
}

ULONG
XhciEndpoint::RingCount() const
{
    return UsesStreams() ? m_CurrentStreams->Count : 1;
}

XhciTransferRing*
XhciEndpoint::RingAt(
    _In_ ULONG Index) const
{
    return UsesStreams() ? m_CurrentStreams->Entry[Index].Ring : m_MainRing;
}

VOID
XhciEndpoint::ForEachRing(
    _In_ VOID (XhciTransferRing::*Action)())
{
    ULONG Count = RingCount();
    ULONG Index;

    if (UsesStreams())
        InterlockedExchange(&m_CurrentStreams->UpdateCount, 0);

    for (Index = 0; Index < Count; Index++)
        (RingAt(Index)->*Action)();
}

/** Stream endpoints report once every stream ring has reported. TRUE when posted. */
BOOLEAN
XhciEndpoint::PostWhenAllRings(
    _In_ XepEvent Event)
{
    if (UsesStreams() &&
        (static_cast<ULONG>(InterlockedIncrement(&m_CurrentStreams->UpdateCount)) != m_CurrentStreams->Count))
    {
        return FALSE;
    }

    PostEvent(Event);
    return TRUE;
}

BOOLEAN
XhciEndpoint::AnyTransfersPending() const
{
    ULONG Count = RingCount();
    ULONG Index;

    for (Index = 0; Index < Count; Index++)
    {
        if (RingAt(Index)->HasQueuedWork())
            return TRUE;
    }

    return FALSE;
}

BOOLEAN
XhciEndpoint::StreamCommandDone()
{
    return static_cast<ULONG>(InterlockedIncrement(&m_CurrentStreams->UpdateCount)) ==
           m_CurrentStreams->Count;
}

PXHCI_STREAM_CONTEXT
XhciEndpoint::StreamContext(
    _In_ ULONG StreamId) const
{
    return static_cast<PXHCI_STREAM_CONTEXT>(m_CurrentStreams->ContextArray->VirtualAddress) + StreamId;
}

ULONG64
XhciEndpoint::StreamDequeuePointer(
    _In_ ULONG StreamId) const
{
    if (!UsesStreams() || (m_CurrentStreams->ContextArray == NULL))
        return 0;

    return StreamContext(StreamId)->DequeuePointer;
}

/** Bytes a stopped stream moved, from wherever this controller keeps its EDTLA. */
ULONG
XhciEndpoint::StreamTransferLength(
    _In_ ULONG StreamId) const
{
    PXHCI_STREAM_CONTEXT Context;
    BOOLEAN First;
    BOOLEAN Second;
    ULONG Length = 0;

    if (!UsesStreams() || (m_CurrentStreams->ContextArray == NULL))
        return 0;

    Context = StreamContext(StreamId);
    First = m_Controller->HasErrata(XhciErrata::StrmByteCountLayoutA);
    Second = m_Controller->HasErrata(XhciErrata::StrmByteCountLayoutB);

    if (!First && !Second)
    {
        Length = Context->Dword2 & XHCI_STREAM_EDTLA_MASK;
    }
    else if (First && !Second)
    {
        Length = Context->Dword2 >> 8;
        if (m_Controller->HasErrata(XhciErrata::StrmByteCountHasValidFlag) && !(Context->Dword2 & 0x80))
            Length = 0;
    }
    else if (!First && Second)
    {
        Length = Context->Dword3 & XHCI_STREAM_EDTLA_MASK;
    }

    if (m_Controller->HasErrata(XhciErrata::StrmByteCountTruncated))
    {
        NT_ASSERT(!Second);
        Length += Context->Dword3 & 0x1FFFF;
    }

    return Length;
}

VOID
XhciEndpoint::ResetStreamTransferLength(
    _In_ ULONG StreamId)
{
    PXHCI_STREAM_CONTEXT Context;
    BOOLEAN First;
    BOOLEAN Second;

    if (!m_Controller->HasErrata(XhciErrata::StrmByteCountRearm))
        return;

    Context = StreamContext(StreamId);
    First = m_Controller->HasErrata(XhciErrata::StrmByteCountLayoutA);
    Second = m_Controller->HasErrata(XhciErrata::StrmByteCountLayoutB);

    if (!First && !Second)
        Context->Dword2 &= ~XHCI_STREAM_EDTLA_MASK;
    else if (First && !Second)
        Context->Dword2 &= 0xFF;
    else if (!First && Second)
        Context->Dword3 &= ~XHCI_STREAM_EDTLA_MASK;
}

/** Only a ring nobody has queued to since its last reset counts as empty. */
BOOLEAN
XhciEndpoint::StreamRingEmpty(
    _In_ ULONG StreamId) const
{
    XhciTransferRing* Ring = m_CurrentStreams->Entry[StreamId - 1].Ring;
    ULONG64 Pointer = StreamContext(StreamId)->DequeuePointer & ~XHCI_DEQUEUE_FLAGS_MASK;

    return (Pointer == Ring->SegmentBase()) && (Pointer == Ring->EnqueuePointer());
}

ULONG
XhciEndpoint::StreamHaltedCode() const
{
    return UsesStreams() ? m_CurrentStreams->HaltedCode : 0;
}

/* Events and hooks ***********************************************************/

VOID
XhciEndpoint::PostEvent(
    _In_ XepEvent Event)
{
    m_Machine.SmPost(Event);
}

VOID
XhciEndpoint::PostCommandFailure()
{
    if (!(InterlockedOr(&m_Flags, XHCI_EP_FAILURE_REPORTED) & XHCI_EP_FAILURE_REPORTED))
        PostEvent(XepEvent::CommandFailed);
}

VOID
XhciEndpoint::FatalError(
    _In_ ULONG Reason)
{
    m_Controller->RaiseControllerFault(XhciRecovery::ResetHost, Reason);
}

ULONG
XhciEndpoint::OutputEndpointState() const
{
    return static_cast<PXHCI_ENDPOINT_CONTEXT>(m_Device->OutputContext(m_Dci))->EndpointState;
}

PENDPOINT_RESET
XhciEndpoint::ResetPayload() const
{
    return reinterpret_cast<PENDPOINT_RESET>(XhciRequestUrb(m_ResetRequest));
}

VOID
XhciEndpoint::OnTransferEvent(
    _In_ const XHCI_TRB* Event)
{
    ULONG Code = XhciTrbCompletionCode(Event);
    ULONG64 Pointer;
    ULONG64 ArrayStart;
    ULONG Index;

    if ((Code == static_cast<ULONG>(XhciCompletionCode::TrbError)) ||
        (Code == static_cast<ULONG>(XhciCompletionCode::UndefinedError)) ||
        ((Code >= 192) && (Code <= 223)))
    {
        DPRINT1("Endpoint %p DCI %lu transfer event with completion code %lu\n", this, m_Dci, Code);
        m_Controller->VerifierCheck(XHCI_VERIFY_EVENT_CODE);
        FatalError(XHCI_REASON_EVENT_CODE);
        return;
    }

    if (!UsesStreams())
    {
        m_MainRing->OnTransferEvent(Event);
        return;
    }

    Pointer = XhciTrbPointer(Event);
    ArrayStart = m_CurrentStreams->ContextArray->LogicalAddress.QuadPart;

    if ((Pointer != 0) &&
        ((Pointer < ArrayStart) || (Pointer >= ArrayStart + m_CurrentStreams->ContextArray->Size)))
    {
        for (Index = 0; Index < m_CurrentStreams->Count; Index++)
        {
            if (m_CurrentStreams->Entry[Index].Ring->OnTransferEvent(Event))
                return;
        }

        if (Event->Dword[3] & XHCI_TRANSFER_EVENT_ED)
        {
            DPRINT1("Endpoint %p DCI %lu event data 0x%I64x matched no stream\n", this, m_Dci, Pointer);
            FatalError(XHCI_REASON_EVENT_DATA_POINTER);
            return;
        }

        if (!XhciIsStoppedCode(Code))
        {
            for (Index = 0; Index < m_CurrentStreams->Count; Index++)
            {
                if (!m_CurrentStreams->Entry[Index].Ring->IsLikelyDuplicate(Event))
                    continue;

                if (m_Controller->HasErrata(XhciErrata::EvtDiscardRepeatedEd0))
                {
                    DPRINT1("Endpoint %p DCI %lu dropping a likely duplicate event at 0x%I64x\n",
                            this, m_Dci, Pointer);
                    return;
                }

                DPRINT1("Endpoint %p DCI %lu likely duplicate event at 0x%I64x\n", this, m_Dci, Pointer);
                FatalError(XHCI_REASON_DUPLICATE_POINTER);
                return;
            }

            DPRINT1("Endpoint %p DCI %lu event at 0x%I64x matched no stream\n", this, m_Dci, Pointer);
            FatalError(XHCI_REASON_EVENT_POINTER);
            return;
        }

        /* Hardware compliance: a stopped event should point into some stream ring */
        DPRINT1("Endpoint %p DCI %lu stopped event at 0x%I64x matched no stream\n", this, m_Dci, Pointer);
    }

    /* An event for the endpoint as a whole */
    if (XhciIsHaltedCode(Code))
        OnHaltedCompletionCode(Code, FALSE);
    else if (XhciIsStoppedCode(Code))
        OnStoppedEvent();
    else
        DPRINT1("Endpoint %p DCI %lu ignoring endpoint event code %lu\n", this, m_Dci, Code);
}

VOID
XhciEndpoint::OnTransferCanceled()
{
    if (!(InterlockedOr(&m_Flags, XHCI_EP_CANCEL_REPORTED) & XHCI_EP_CANCEL_REPORTED))
        PostEvent(XepEvent::TransferAborted);
}

VOID
XhciEndpoint::OnHaltedCompletionCode(
    _In_ ULONG CompletionCode,
    _In_ BOOLEAN TransferFound)
{
    if (InterlockedOr(&m_Flags, XHCI_EP_HALT_RECEIVED) & XHCI_EP_HALT_RECEIVED)
    {
        m_Controller->VerifierCheck(XHCI_VERIFY_DUPLICATE_EVENT);
        return;
    }

    if (!TransferFound)
    {
        if (UsesStreams())
            m_CurrentStreams->HaltedCode = CompletionCode;
        InterlockedOr(&m_Flags, XHCI_EP_STREAM_HALT_PENDING);
    }

    PostEvent(XepEvent::HaltReported);
}

BOOLEAN
XhciEndpoint::ShouldDropStoppedEvent() const
{
    return (m_Flags & XHCI_EP_IGNORE_STOPPED) != 0;
}

VOID
XhciEndpoint::OnStoppedEvent()
{
    /* QUIRK: a second stopped event reports the duplicate halt verifier condition */
    if (InterlockedOr(&m_Flags, XHCI_EP_STOPPED_RECEIVED) & XHCI_EP_STOPPED_RECEIVED)
    {
        m_Controller->VerifierCheck(XHCI_VERIFY_DUPLICATE_EVENT);
        return;
    }

    if (InterlockedXor(&m_Flags, XHCI_EP_STOPPED_HANDSHAKE) & XHCI_EP_STOPPED_HANDSHAKE)
        PostEvent(XepEvent::StoppedEventSeen);
}

VOID
XhciEndpoint::OnExpectedEventsProcessed()
{
    PostWhenAllRings(XepEvent::ExpectedEventsDone);
}

VOID
XhciEndpoint::OnMappingStopped()
{
    PostWhenAllRings(XepEvent::StreamMapStopped);
}

VOID
XhciEndpoint::OnTransfersReclaimed()
{
    /* Drops the reference NotifyRingsReclaim took, once the machine has the event */
    if (PostWhenAllRings(XepEvent::TransfersRecovered))
        WdfObjectDereferenceWithTag(m_Ucx, XHCI_RECLAIM_TAG);
}

/* Controller reset and removal ***********************************************/

VOID
XhciEndpoint::ControllerResetStarting()
{
    PostEvent(XepEvent::ControllerResetStarting);
}

VOID
XhciEndpoint::ControllerResetDone()
{
    KeClearEvent(&m_ResetAck);
    PostEvent(XepEvent::ControllerResetDone);

    /* Whoever toggles second finishes the handshake; wait only if the machine has not acked */
    if (!(InterlockedXor(&m_Flags, XHCI_EP_RESET_ACK_HANDSHAKE) & XHCI_EP_RESET_ACK_HANDSHAKE))
        KeWaitForSingleObject(&m_ResetAck, Executive, KernelMode, FALSE, NULL);
}

VOID
XhciEndpoint::ControllerRemoved()
{
    PostEvent(XepEvent::ControllerRemoved);
}

/* UCX endpoint callbacks *****************************************************/

VOID
NTAPI
XhciEndpoint::EvtPurge(
    _In_ UCXCONTROLLER UcxController,
    _In_ UCXENDPOINT UcxEndpoint)
{
    XhciEndpoint* Self = FromUcx(UcxEndpoint);
    ULONG Index;

    UNREFERENCED_PARAMETER(UcxController);

    if (!Self->UsesStreams())
    {
        WdfIoQueuePurge(Self->m_MainRing->Queue(), EvtPurgeDone, UcxEndpoint);
        return;
    }

    /* QUIRK: purge and abort share this counter */
    Self->m_CurrentStreams->StreamsLeftToFlush = Self->m_CurrentStreams->Count;
    for (Index = 0; Index < Self->m_CurrentStreams->Count; Index++)
        WdfIoQueuePurge(Self->m_CurrentStreams->Entry[Index].Ring->Queue(), EvtPurgeDone, UcxEndpoint);
}

VOID
NTAPI
XhciEndpoint::EvtPurgeDone(
    _In_ WDFQUEUE Queue,
    _In_ WDFCONTEXT Context)
{
    UCXENDPOINT UcxEndpoint = static_cast<UCXENDPOINT>(Context);
    XhciEndpoint* Self = FromUcx(UcxEndpoint);

    UNREFERENCED_PARAMETER(Queue);

    if (Self->UsesStreams() && (InterlockedDecrement(&Self->m_CurrentStreams->StreamsLeftToFlush) != 0))
        return;

    UcxEndpointPurgeComplete(UcxEndpoint);
}

/* Start and abort use the stream rings only while a streams record is current */
VOID
NTAPI
XhciEndpoint::EvtStart(
    _In_ UCXCONTROLLER UcxController,
    _In_ UCXENDPOINT UcxEndpoint)
{
    XhciEndpoint* Self = FromUcx(UcxEndpoint);
    ULONG Index;

    UNREFERENCED_PARAMETER(UcxController);

    if (!Self->UsesStreams())
    {
        WdfIoQueueStart(Self->m_MainRing->Queue());
        return;
    }

    for (Index = 0; Index < Self->m_CurrentStreams->Count; Index++)
        WdfIoQueueStart(Self->m_CurrentStreams->Entry[Index].Ring->Queue());
}

VOID
NTAPI
XhciEndpoint::EvtAbort(
    _In_ UCXCONTROLLER UcxController,
    _In_ UCXENDPOINT UcxEndpoint)
{
    XhciEndpoint* Self = FromUcx(UcxEndpoint);
    ULONG Index;

    UNREFERENCED_PARAMETER(UcxController);

    if (!Self->UsesStreams())
    {
        WdfIoQueueStopAndPurge(Self->m_MainRing->Queue(), EvtAbortDone, UcxEndpoint);
        return;
    }

    Self->m_CurrentStreams->StreamsLeftToFlush = Self->m_CurrentStreams->Count;
    for (Index = 0; Index < Self->m_CurrentStreams->Count; Index++)
        WdfIoQueueStopAndPurge(Self->m_CurrentStreams->Entry[Index].Ring->Queue(), EvtAbortDone, UcxEndpoint);
}

VOID
NTAPI
XhciEndpoint::EvtAbortDone(
    _In_ WDFQUEUE Queue,
    _In_ WDFCONTEXT Context)
{
    UCXENDPOINT UcxEndpoint = static_cast<UCXENDPOINT>(Context);
    XhciEndpoint* Self = FromUcx(UcxEndpoint);

    UNREFERENCED_PARAMETER(Queue);

    if (Self->UsesStreams() && (InterlockedDecrement(&Self->m_CurrentStreams->StreamsLeftToFlush) != 0))
        return;

    UcxEndpointAbortComplete(UcxEndpoint);
}

VOID
NTAPI
XhciEndpoint::EvtReset(
    _In_ UCXCONTROLLER UcxController,
    _In_ UCXENDPOINT UcxEndpoint,
    _In_ WDFREQUEST Request)
{
    XhciEndpoint* Self = FromUcx(UcxEndpoint);
    XhciEndpointReset* Data = &XhciGetRequestData(Request)->EndpointReset;
    WDFREQUEST Taken;

    UNREFERENCED_PARAMETER(UcxController);

    RtlZeroMemory(Data, sizeof(*Data));
    Data->Endpoint = Self;

    /* Publish the request before the handshake so the machine can always find it */
    Self->m_ResetStatus = STATUS_PENDING;
    InterlockedExchangePointer(reinterpret_cast<PVOID volatile*>(&Self->m_ResetRequest), Request);

    if (!(InterlockedXor(&Self->m_Flags, XHCI_EP_CLIENT_RESET_HANDSHAKE) & XHCI_EP_CLIENT_RESET_HANDSHAKE))
    {
        Self->PostEvent(XepEvent::ClientReset);
        return;
    }

    /* The machine already allowed completion: the endpoint is fenced or removed */
    Taken = static_cast<WDFREQUEST>(
        InterlockedExchangePointer(reinterpret_cast<PVOID volatile*>(&Self->m_ResetRequest), NULL));
    if (Taken != NULL)
    {
        Self->m_ResetStatus = STATUS_PENDING;
        WdfRequestComplete(Taken, STATUS_SUCCESS);
    }
}

VOID
NTAPI
XhciEndpoint::EvtOkToCancel(
    _In_ UCXENDPOINT UcxEndpoint)
{
    FromUcx(UcxEndpoint)->PostEvent(XepEvent::CancelAllowed);
}

NTSTATUS
NTAPI
XhciEndpoint::EvtEnableForwardProgress(
    _In_ UCXENDPOINT Endpoint,
    _In_ ULONG MaxTransferSize)
{
    XhciEndpoint* Self = FromUcx(Endpoint);
    XhciTransferRing* Ring;
    NTSTATUS Status;

    /* QUIRK: UCX may ask again with a larger size; that is not supported */
    NT_ASSERT(!Self->m_ForwardProgress);

    /* The main ring serves until a streams record is current */
    Ring = Self->UsesStreams() ? Self->m_CurrentStreams->Entry[0].Ring : Self->m_MainRing;

    Status = Ring->EnableForwardProgress(MaxTransferSize);
    if (NT_SUCCESS(Status))
        Self->m_ForwardProgress = TRUE;
    else
        DPRINT1("Endpoint %p forward progress enable failed 0x%lx\n", Self, Status);

    return Status;
}

/* Default endpoint update ****************************************************/

VOID
NTAPI
XhciEndpoint::EvtDefaultUpdate(
    _In_ UCXCONTROLLER UcxController,
    _In_ WDFREQUEST Request)
{
    XhciDefaultEndpointUpdate* Data = &XhciGetRequestData(Request)->DefaultEndpointUpdate;
    PDEFAULT_ENDPOINT_UPDATE Payload;
    PXHCI_INPUT_CONTROL_CONTEXT Control;
    PXHCI_ENDPOINT_CONTEXT Context;
    XhciController* Controller;
    XhciEndpoint* Self;
    ULONG64 InputAddress;
    ULONG Size;
    PUCHAR Base;

    UNREFERENCED_PARAMETER(UcxController);

    /* QUIRK: the endpoint comes from the payload, not from the device UCX dispatched on */
    Payload = reinterpret_cast<PDEFAULT_ENDPOINT_UPDATE>(XhciRequestUrb(Request));
    Self = FromUcx(Payload->DefaultEndpoint);
    Controller = Self->m_Controller;

    RtlZeroMemory(Data, sizeof(*Data));
    Data->Endpoint = Self;
    Data->MaxPacketSize = Payload->MaxPacketSize;

    Size = Controller->m_Registers.ContextSize();
    Data->InputContext = Controller->m_Buffers.Acquire(Size * XHCI_INPUT_CONTEXT_COUNT);
    if (Data->InputContext == NULL)
    {
        DPRINT1("Endpoint %p no input context for the max packet size update\n", Self);
        WdfRequestComplete(Request, STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    Base = static_cast<PUCHAR>(Data->InputContext->VirtualAddress);
    InputAddress = Data->InputContext->LogicalAddress.QuadPart;

    /* Evaluate Context looks only at the EP0 Max Packet Size (xHCI 4.6.7) */
    Control = reinterpret_cast<PXHCI_INPUT_CONTROL_CONTEXT>(Base);
    Control->AddFlags = 1 << 1;

    Context = reinterpret_cast<PXHCI_ENDPOINT_CONTEXT>(Base + 2 * Size);
    Context->EndpointType = XHCI_ENDPOINT_TYPE_CONTROL;
    Context->MaxPacketSize = Data->MaxPacketSize;
    Context->ErrorCount = 3;
    Context->TRDequeuePointer = Self->m_MainRing->DequeuePointerValue();

    Self->m_MaxPayload = Data->MaxPacketSize;

    /* QUIRK: the controller keeps the old dequeue pointer until the next Address Device */
    Self->m_MainRing->InitializeRing();

    Data->Command.Trb.Dword[0] = static_cast<ULONG>(InputAddress);
    Data->Command.Trb.Dword[1] = static_cast<ULONG>(InputAddress >> 32);
    Data->Command.Trb.Dword[3] = XhciEndpointCommandControl(XhciTrbType::EvaluateContext,
                                                            0,
                                                            Self->m_Device->SlotId());
    Data->Command.Done = DefaultUpdateDone;
    Data->Command.Context = Request;

    Controller->m_Commands.Submit(&Data->Command);
}

VOID
NTAPI
XhciEndpoint::DefaultUpdateDone(
    _In_ XhciCommand* Command)
{
    WDFREQUEST Request = static_cast<WDFREQUEST>(Command->Context);
    XhciDefaultEndpointUpdate* Data = &XhciGetRequestData(Request)->DefaultEndpointUpdate;
    XhciEndpoint* Self = Data->Endpoint;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    Self->m_Controller->m_Buffers.Free(Data->InputContext);
    Data->InputContext = NULL;

    if (Command->Status == STATUS_NO_SUCH_DEVICE)
    {
        DPRINT1("Endpoint %p max packet size update lost with the controller\n", Self);
    }
    else if (Command->Code != XhciCompletionCode::Success)
    {
        DPRINT1("Endpoint %p Evaluate Context failed with code %lu\n",
                Self, static_cast<ULONG>(Command->Code));
    }
    else
    {
        Self->m_Descriptor.wMaxPacketSize = static_cast<USHORT>(Data->MaxPacketSize);
        Status = STATUS_SUCCESS;
    }

    WdfRequestComplete(Request, Status);
}

/* Static streams *************************************************************/

NTSTATUS
NTAPI
XhciEndpoint::EvtStreamsAdd(
    _In_ UCXENDPOINT UcxEndpoint,
    _In_ ULONG NumberOfStreams,
    _In_ PUCXSSTREAMS_INIT UcxStaticStreamsInit)
{
    XhciEndpoint* Self = FromUcx(UcxEndpoint);
    WDF_OBJECT_ATTRIBUTES Attributes;
    STREAM_INFO Info;
    XhciStreams* Streams;
    UCXSSTREAMS UcxStreams;
    XhciTransferRing* Ring;
    NTSTATUS Status;
    ULONG StreamId;

    if (!Self->m_StreamsCapable)
    {
        DPRINT1("Endpoint %p is not streams capable\n", Self);
        return STATUS_INVALID_PARAMETER;
    }

    if ((NumberOfStreams == 0) || (NumberOfStreams > Self->m_Controller->m_Registers.SupportedStreams()))
    {
        DPRINT1("Endpoint %p bad stream count %lu\n", Self, NumberOfStreams);
        return STATUS_INVALID_PARAMETER;
    }

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, XhciStreams);
    Attributes.ContextSizeOverride = XhciStreamsSize(NumberOfStreams);
    Attributes.EvtCleanupCallback = EvtStreamsCleanup;

    Status = UcxStaticStreamsCreate(UcxEndpoint, &UcxStaticStreamsInit, &Attributes, &UcxStreams);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Endpoint %p UcxStaticStreamsCreate failed 0x%lx\n", Self, Status);
        return Status;
    }

    Streams = XhciGetStreams(UcxStreams);
    Streams->Endpoint = Self;
    Streams->Count = NumberOfStreams;
    Streams->MaxPrimary = XhciMaxPrimaryStreams(NumberOfStreams);

    for (StreamId = 1; StreamId <= NumberOfStreams; StreamId++)
    {
        Status = XhciTransferRing::Create(Self, UcxStreams, StreamId, &Ring);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Endpoint %p stream %lu ring creation failed 0x%lx\n", Self, StreamId, Status);
            return Status;
        }

        Streams->Entry[StreamId - 1].Ring = Ring;

        STREAM_INFO_INIT(&Info, Ring->Queue(), StreamId);
        UcxStaticStreamsSetStreamInfo(UcxStreams, &Info);
    }

    return STATUS_SUCCESS;
}

VOID
NTAPI
XhciEndpoint::EvtStreamsCleanup(
    _In_ WDFOBJECT Object)
{
    XhciStreams* Streams = XhciGetStreams(Object);

    if ((Streams->ContextArray != NULL) && (Streams->Endpoint != NULL))
    {
        Streams->Endpoint->m_Controller->m_Buffers.Free(Streams->ContextArray);
        Streams->ContextArray = NULL;
    }
}

VOID
XhciEndpoint::DisableStreamRings(
    _In_ XhciStreams* Streams)
{
    ULONG Index;

    for (Index = 0; Index < Streams->Count; Index++)
        Streams->Entry[Index].Ring->Disable(TRUE);
}

VOID
NTAPI
XhciEndpoint::EvtStreamsEnable(
    _In_ UCXENDPOINT UcxEndpoint,
    _In_ UCXSSTREAMS UcxStaticStreams,
    _In_ WDFREQUEST Request)
{
    NTSTATUS Status;

    Status = FromUcx(UcxEndpoint)->OpenStreams(XhciGetStreams(UcxStaticStreams), Request);
    if (Status != STATUS_PENDING)
        WdfRequestComplete(Request, Status);
}

/** Switches from the default record to the client streams; STATUS_PENDING once the configure is out. */
NTSTATUS
XhciEndpoint::OpenStreams(
    _In_ XhciStreams* Streams,
    _In_ WDFREQUEST Request)
{
    PURB Urb = XhciRequestUrb(Request);
    WDFQUEUE MainQueue = m_MainRing->Queue();
    XhciTransferRing* Ring;
    ULONG Queued;
    ULONG Owned;
    NTSTATUS Status;
    ULONG Index;

    /* The main queue stays drained for as long as client streams are open */
    WdfIoQueueDrain(MainQueue, NULL, NULL);
    WdfIoQueueGetState(MainQueue, &Queued, &Owned);
    if ((Queued != 0) || (Owned != 0))
    {
        DPRINT1("Endpoint %p streams open with requests still on the main queue\n", this);
        Urb->UrbHeader.Status = USBD_STATUS_INVALID_PARAMETER;
        Status = STATUS_INVALID_PARAMETER;
        goto Failure;
    }

    if ((Streams->Count == 0) || (Streams->Count > m_Controller->m_Registers.SupportedStreams()))
    {
        DPRINT1("Endpoint %p streams open with bad stream count %lu\n", this, Streams->Count);
        Urb->UrbHeader.Status = USBD_STATUS_INVALID_PARAMETER;
        Status = STATUS_INVALID_PARAMETER;
        goto Failure;
    }

    XhciGetRequestData(Request)->StreamsConfigure.Streams = Streams;

    Streams->ContextArray = m_Controller->m_Buffers.Acquire(XhciStreamArrayBytes(Streams->MaxPrimary));
    if (Streams->ContextArray == NULL)
    {
        DPRINT1("Endpoint %p no stream context array for %lu streams\n", this, Streams->Count);
        Urb->UrbHeader.Status = USBD_STATUS_INSUFFICIENT_RESOURCES;
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Failure;
    }

    m_CurrentStreams = Streams;

    for (Index = 0; Index < Streams->Count; Index++)
    {
        Ring = Streams->Entry[Index].Ring;
        Status = Ring->Enable();
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Endpoint %p stream %lu ring enable failed 0x%lx\n", this, Index + 1, Status);
            Urb->UrbHeader.Status = USBD_STATUS_INSUFFICIENT_RESOURCES;
            goto Failure;
        }

        StreamContext(Index + 1)->DequeuePointer = Ring->DequeuePointerValue();
    }

    m_Device->ReconfigureEndpoint(this, StreamsEnableDone, Request);
    return STATUS_PENDING;

Failure:
    DisableStreamRings(Streams);
    m_CurrentStreams = m_DefaultStreams;
    if (Streams->ContextArray != NULL)
    {
        m_Controller->m_Buffers.Free(Streams->ContextArray);
        Streams->ContextArray = NULL;
    }

    WdfIoQueueStart(MainQueue);
    return Status;
}

VOID
NTAPI
XhciEndpoint::StreamsEnableDone(
    _In_ PVOID Context,
    _In_ NTSTATUS Status)
{
    WDFREQUEST Request = static_cast<WDFREQUEST>(Context);
    XhciStreams* Streams = XhciGetRequestData(Request)->StreamsConfigure.Streams;
    XhciEndpoint* Self = Streams->Endpoint;
    PURB Urb = XhciRequestUrb(Request);

    if (NT_SUCCESS(Status))
    {
        Self->m_MainRing->Disable(TRUE);
        Self->FreeDefaultStreams(Self->m_DefaultStreams);
        Self->m_DefaultStreams = NULL;
        Urb->UrbHeader.Status = USBD_STATUS_SUCCESS;
    }
    else
    {
        DPRINT1("Endpoint %p streams open reconfigure failed 0x%lx\n", Self, Status);
        Self->DisableStreamRings(Streams);
        if (Streams->ContextArray != NULL)
        {
            Self->m_Controller->m_Buffers.Free(Streams->ContextArray);
            Streams->ContextArray = NULL;
        }

        Self->m_CurrentStreams = Self->m_DefaultStreams;
        Urb->UrbHeader.Status = USBD_STATUS_INTERNAL_HC_ERROR;
        WdfIoQueueStart(Self->m_MainRing->Queue());
    }

    if (NT_SUCCESS(Status))
        Self->PostEvent(XepEvent::StreamsOn);

    WdfRequestComplete(Request, Status);
}

VOID
NTAPI
XhciEndpoint::EvtStreamsDisable(
    _In_ UCXENDPOINT UcxEndpoint,
    _In_ UCXSSTREAMS UcxStaticStreams,
    _In_ WDFREQUEST Request)
{
    NTSTATUS Status;

    Status = FromUcx(UcxEndpoint)->CloseStreams(XhciGetStreams(UcxStaticStreams), Request);
    if (Status != STATUS_PENDING)
        WdfRequestComplete(Request, Status);
}

/** Switches back to a new default record; STATUS_PENDING once the configure is out. */
NTSTATUS
XhciEndpoint::CloseStreams(
    _In_ XhciStreams* Streams,
    _In_ WDFREQUEST Request)
{
    PURB Urb = XhciRequestUrb(Request);
    XhciStreams* Default;
    NTSTATUS Status;

    XhciGetRequestData(Request)->StreamsConfigure.Streams = Streams;

    /* QUIRK: the array is sized for the client stream count, larger than one stream needs */
    Default = XhciNewDefaultStreams(this, m_MainRing, Streams->MaxPrimary);
    if (Default == NULL)
    {
        DPRINT1("Endpoint %p no default stream record for the streams close\n", this);
        Urb->UrbHeader.Status = USBD_STATUS_INSUFFICIENT_RESOURCES;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    m_DefaultStreams = Default;
    m_CurrentStreams = Default;

    Status = m_MainRing->Enable();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Endpoint %p main ring enable for the streams close failed 0x%lx\n", this, Status);
        Urb->UrbHeader.Status = USBD_STATUS_INSUFFICIENT_RESOURCES;

        /* Restore the client record so the freed default record is not left current */
        m_CurrentStreams = Streams;
        FreeDefaultStreams(Default);
        m_DefaultStreams = NULL;
        return Status;
    }

    StreamContext(1)->DequeuePointer = m_MainRing->DequeuePointerValue();

    m_Device->ReconfigureEndpoint(this, StreamsDisableDone, Request);
    return STATUS_PENDING;
}

VOID
NTAPI
XhciEndpoint::StreamsDisableDone(
    _In_ PVOID Context,
    _In_ NTSTATUS Status)
{
    WDFREQUEST Request = static_cast<WDFREQUEST>(Context);
    XhciStreams* Streams = XhciGetRequestData(Request)->StreamsConfigure.Streams;
    XhciEndpoint* Self = Streams->Endpoint;
    PURB Urb = XhciRequestUrb(Request);

    if (NT_SUCCESS(Status))
    {
        Self->DisableStreamRings(Streams);
        if (Streams->ContextArray != NULL)
        {
            Self->m_Controller->m_Buffers.Free(Streams->ContextArray);
            Streams->ContextArray = NULL;
        }

        WdfIoQueueStart(Self->m_MainRing->Queue());
        Urb->UrbHeader.Status = USBD_STATUS_SUCCESS;
    }
    else
    {
        /* QUIRK: the main ring enabled for the close stays enabled */
        DPRINT1("Endpoint %p streams close reconfigure failed 0x%lx\n", Self, Status);
        Self->m_CurrentStreams = Streams;
        Self->FreeDefaultStreams(Self->m_DefaultStreams);
        Self->m_DefaultStreams = NULL;
        Urb->UrbHeader.Status = USBD_STATUS_INTERNAL_HC_ERROR;
    }

    if (NT_SUCCESS(Status))
        Self->PostEvent(XepEvent::StreamsOff);

    WdfRequestComplete(Request, Status);
}

/* Endpoint commands **********************************************************/

VOID
XhciEndpoint::BuildSetDequeue(
    _Out_ XhciCommand* Command,
    _In_ ULONG64 DequeuePointer,
    _In_ ULONG StreamId,
    _In_ PFN_XHCI_COMMAND_DONE Done)
{
    RtlZeroMemory(Command, sizeof(*Command));
    Command->Trb.Dword[0] = static_cast<ULONG>(DequeuePointer);
    Command->Trb.Dword[1] = static_cast<ULONG>(DequeuePointer >> 32);
    Command->Trb.Dword[2] = StreamId << XHCI_CMD_STREAM_ID_SHIFT;
    Command->Trb.Dword[3] = XhciEndpointCommandControl(XhciTrbType::SetTrDequeuePointer,
                                                       m_Dci,
                                                       m_Device->SlotId());
    Command->Done = Done;
    Command->Context = this;
}

PXHCI_ENDPOINT_CONTEXT
XhciEndpoint::InputEndpointContext(
    _In_ XhciDmaBuffer* Input) const
{
    ULONG Size = m_Controller->m_Registers.ContextSize();

    return reinterpret_cast<PXHCI_ENDPOINT_CONTEXT>(static_cast<PUCHAR>(Input->VirtualAddress) +
                                                    (m_Dci + 1) * Size);
}

/** Configure Endpoint for this DCI only; EndpointContext, when given, replaces the input endpoint context. */
VOID
XhciEndpoint::BuildEndpointConfigure(
    _In_ XhciDmaBuffer* Input,
    _In_opt_ const VOID* EndpointContext,
    _In_ BOOLEAN Drop,
    _In_ BOOLEAN Add,
    _Out_ XhciCommand* Command,
    _In_ PFN_XHCI_COMMAND_DONE Done)
{
    ULONG Size = m_Controller->m_Registers.ContextSize();
    PUCHAR Base = static_cast<PUCHAR>(Input->VirtualAddress);
    PXHCI_INPUT_CONTROL_CONTEXT Control = reinterpret_cast<PXHCI_INPUT_CONTROL_CONTEXT>(Base);
    PXHCI_SLOT_CONTEXT Slot = reinterpret_cast<PXHCI_SLOT_CONTEXT>(Base + Size);
    ULONG64 InputAddress = Input->LogicalAddress.QuadPart;

    RtlZeroMemory(Control, Size);

    /* QUIRK: controller owned fields of the output contexts travel along; the xHC ignores them */
    RtlCopyMemory(Slot, m_Device->OutputContext(0), Size);
    if (EndpointContext != NULL)
        RtlCopyMemory(InputEndpointContext(Input), EndpointContext, Size);

    /* The cached count, since a temporary drop lowers the controller's own value */
    Slot->ContextEntries = m_Device->ContextEntries();

    Control->AddFlags = 1;
    if (Add)
        Control->AddFlags |= 1 << m_Dci;
    if (Drop)
        Control->DropFlags = 1 << m_Dci;

    RtlZeroMemory(Command, sizeof(*Command));
    Command->Trb.Dword[0] = static_cast<ULONG>(InputAddress);
    Command->Trb.Dword[1] = static_cast<ULONG>(InputAddress >> 32);
    Command->Trb.Dword[3] = XhciEndpointCommandControl(XhciTrbType::ConfigureEndpoint,
                                                       0,
                                                       m_Device->SlotId());
    Command->Done = Done;
    Command->Context = this;
}

VOID
NTAPI
XhciEndpoint::StopDone(
    _In_ XhciCommand* Command)
{
    XhciEndpoint* Self = static_cast<XhciEndpoint*>(Command->Context);
    ULONG State = Self->OutputEndpointState();

    if (Command->Status == STATUS_NO_SUCH_DEVICE)
    {
        Self->PostCommandFailure();
        return;
    }

    if (Command->Code == XhciCompletionCode::Success)
    {
        if (State == XHCI_ENDPOINT_STATE_STOPPED)
        {
            Self->PostEvent(XepEvent::CommandSucceeded);
            return;
        }

        DPRINT1("Endpoint %p DCI %lu in state %lu after a successful stop\n", Self, Self->m_Dci, State);
        Self->FatalError(XHCI_REASON_NOT_STOPPED);
        Self->PostCommandFailure();
        return;
    }

    if (Command->Code == XhciCompletionCode::ContextStateError)
    {
        switch (State)
        {
            case XHCI_ENDPOINT_STATE_RUNNING:
                /* A doorbell raced the stop */
                Self->PostEvent(XepEvent::StopFailedRunning);
                return;

            case XHCI_ENDPOINT_STATE_STOPPED:
                Self->PostEvent(XepEvent::StopFailedStopped);
                return;

            case XHCI_ENDPOINT_STATE_HALTED:
                if (Self->TransferType() != USB_ENDPOINT_TYPE_ISOCHRONOUS)
                {
                    Self->PostEvent(XepEvent::StopFailedHalted);
                    return;
                }

                DPRINT1("Endpoint %p DCI %lu isoch endpoint halted on stop\n", Self, Self->m_Dci);
                Self->FatalError(XHCI_REASON_STOP_ERROR);
                Self->PostCommandFailure();
                return;

            default:
                DPRINT1("Endpoint %p DCI %lu stop context error in state %lu\n", Self, Self->m_Dci, State);
                Self->FatalError(XHCI_REASON_STOP_CONTEXT);
                Self->PostCommandFailure();
                return;
        }
    }

    DPRINT1("Endpoint %p DCI %lu Stop Endpoint failed with code %lu\n",
            Self, Self->m_Dci, static_cast<ULONG>(Command->Code));
    Self->m_Controller->VerifierCheck(XHCI_VERIFY_STOP_ENDPOINT);
    Self->FatalError(XHCI_REASON_STOP_ERROR);
    Self->PostCommandFailure();
}

/** Moves one ring past the canceled transfers after a stop. StreamId 0 is the main ring. */
VOID
XhciEndpoint::DequeueAfterStop(
    _In_ ULONG StreamId)
{
    XhciTransferRing* Ring = m_MainRing;
    XhciCommand* Command = &m_Command;

    if (StreamId != 0)
    {
        Ring = m_CurrentStreams->Entry[StreamId - 1].Ring;
        Command = &m_CurrentStreams->Entry[StreamId - 1].Command;

        if (StreamRingEmpty(StreamId) &&
            !m_Controller->HasErrata(XhciErrata::StrmReconfigOnStop))
        {
            if (StreamCommandDone())
                PostEvent(XepEvent::CommandSucceeded);
            return;
        }

        ResetStreamTransferLength(StreamId);
    }

    Ring->InitializeRing();
    BuildSetDequeue(Command, Ring->DequeuePointerValue(), StreamId, CancelDequeueDone);
    m_Controller->m_Commands.Submit(Command);
}

VOID
NTAPI
XhciEndpoint::CancelDequeueDone(
    _In_ XhciCommand* Command)
{
    XhciEndpoint* Self = static_cast<XhciEndpoint*>(Command->Context);

    if (Command->Status == STATUS_NO_SUCH_DEVICE)
    {
        Self->PostCommandFailure();
        return;
    }

    if (Command->Code != XhciCompletionCode::Success)
    {
        DPRINT1("Endpoint %p DCI %lu Set TR Dequeue Pointer after stop failed with code %lu\n",
                Self, Self->m_Dci, static_cast<ULONG>(Command->Code));
        Self->m_Controller->VerifierCheck(XHCI_VERIFY_CANCEL_DEQUEUE);
        Self->FatalError(XHCI_REASON_CANCEL_DEQUEUE);
        Self->PostCommandFailure();
        return;
    }

    if ((Command != &Self->m_Command) && !Self->StreamCommandDone())
        return;

    Self->PostEvent(XepEvent::CommandSucceeded);
}

VOID
NTAPI
XhciEndpoint::ControlResetDone(
    _In_ XhciCommand* Command)
{
    XhciEndpoint* Self = static_cast<XhciEndpoint*>(Command->Context);

    if (Command->Status == STATUS_NO_SUCH_DEVICE)
    {
        Self->PostCommandFailure();
        return;
    }

    /* QUIRK: a Context State error is fatal too, although it could go on to the dequeue update */
    if (Command->Code != XhciCompletionCode::Success)
    {
        DPRINT1("Endpoint %p DCI %lu Reset Endpoint failed with code %lu\n",
                Self, Self->m_Dci, static_cast<ULONG>(Command->Code));
        Self->m_Controller->VerifierCheck(XHCI_VERIFY_RESET_ENDPOINT);
        Self->FatalError(XHCI_REASON_CONTROL_RESET);
        Self->PostCommandFailure();
        return;
    }

    Self->m_MainRing->InitializeRing();
    Self->BuildSetDequeue(&Self->m_Command, Self->m_MainRing->DequeuePointerValue(), 0, ControlDequeueDone);
    Self->m_Controller->m_Commands.Submit(&Self->m_Command);
}

VOID
NTAPI
XhciEndpoint::ControlDequeueDone(
    _In_ XhciCommand* Command)
{
    XhciEndpoint* Self = static_cast<XhciEndpoint*>(Command->Context);

    if (Command->Status == STATUS_NO_SUCH_DEVICE)
    {
        Self->PostCommandFailure();
        return;
    }

    if (Command->Code != XhciCompletionCode::Success)
    {
        DPRINT1("Endpoint %p DCI %lu Set TR Dequeue Pointer after reset failed with code %lu\n",
                Self, Self->m_Dci, static_cast<ULONG>(Command->Code));
        Self->m_Controller->VerifierCheck(XHCI_VERIFY_CONTROL_DEQUEUE);
        Self->FatalError(XHCI_REASON_CONTROL_DEQUEUE);
        Self->PostCommandFailure();
        return;
    }

    Self->m_ResetStatus = STATUS_SUCCESS;
    Self->PostEvent(XepEvent::CommandSucceeded);
}

VOID
NTAPI
XhciEndpoint::ClientResetDone(
    _In_ XhciCommand* Command)
{
    XhciEndpoint* Self = static_cast<XhciEndpoint*>(Command->Context);
    ULONG StreamId;

    if (Command->Status == STATUS_NO_SUCH_DEVICE)
    {
        Self->m_ResetStatus = STATUS_UNSUCCESSFUL;
        Self->PostCommandFailure();
        return;
    }

    if (Command->Code != XhciCompletionCode::Success)
    {
        DPRINT1("Endpoint %p DCI %lu client Reset Endpoint failed with code %lu\n",
                Self, Self->m_Dci, static_cast<ULONG>(Command->Code));
        Self->m_Controller->VerifierCheck(XHCI_VERIFY_RESET_ENDPOINT);
        Self->FatalError(XHCI_REASON_CLIENT_RESET);
        Self->m_ResetStatus = STATUS_UNSUCCESSFUL;
        Self->PostCommandFailure();
        return;
    }

    NT_ASSERT(Self->OutputEndpointState() == XHCI_ENDPOINT_STATE_STOPPED);

    if (!Self->UsesStreams())
    {
        Self->DequeueAfterReset(0);
        return;
    }

    InterlockedExchange(&Self->m_CurrentStreams->UpdateCount, 0);
    for (StreamId = 1; StreamId <= Self->m_CurrentStreams->Count; StreamId++)
        Self->DequeueAfterReset(StreamId);
}

/** Points the controller at a ring's current position; the pending transfers stay put. */
VOID
XhciEndpoint::DequeueAfterReset(
    _In_ ULONG StreamId)
{
    XhciTransferRing* Ring = m_MainRing;
    XhciCommand* Command = &m_Command;

    if (StreamId != 0)
    {
        Ring = m_CurrentStreams->Entry[StreamId - 1].Ring;
        Command = &m_CurrentStreams->Entry[StreamId - 1].Command;

        if (StreamRingEmpty(StreamId))
        {
            if (StreamCommandDone())
            {
                m_ResetStatus = STATUS_SUCCESS;
                PostEvent(XepEvent::CommandSucceeded);
            }
            return;
        }
    }

    BuildSetDequeue(Command, Ring->DequeuePointerValue(), StreamId, ClientDequeueDone);
    m_Controller->m_Commands.Submit(Command);
}

VOID
NTAPI
XhciEndpoint::ClientDequeueDone(
    _In_ XhciCommand* Command)
{
    XhciEndpoint* Self = static_cast<XhciEndpoint*>(Command->Context);

    if (Command->Status == STATUS_NO_SUCH_DEVICE)
    {
        Self->m_ResetStatus = STATUS_UNSUCCESSFUL;
        Self->PostCommandFailure();
        return;
    }

    if (Command->Code != XhciCompletionCode::Success)
    {
        DPRINT1("Endpoint %p DCI %lu Set TR Dequeue Pointer for the client reset failed with code %lu\n",
                Self, Self->m_Dci, static_cast<ULONG>(Command->Code));
        Self->m_Controller->VerifierCheck(XHCI_VERIFY_CONTROL_DEQUEUE);
        Self->FatalError(XHCI_REASON_CLIENT_DEQUEUE);
        Self->m_ResetStatus = STATUS_UNSUCCESSFUL;
        Self->PostCommandFailure();
        return;
    }

    if ((Command != &Self->m_Command) && !Self->StreamCommandDone())
        return;

    Self->m_ResetStatus = STATUS_SUCCESS;
    Self->PostEvent(XepEvent::CommandSucceeded);
}

VOID
NTAPI
XhciEndpoint::ResetDropDone(
    _In_ XhciCommand* Command)
{
    XhciEndpoint* Self = static_cast<XhciEndpoint*>(Command->Context);
    XhciEndpointReset* Data = &XhciGetRequestData(Self->m_ResetRequest)->EndpointReset;
    XhciTransferRing* Ring;
    ULONG StreamId;

    if ((Command->Status == STATUS_NO_SUCH_DEVICE) || (Command->Code != XhciCompletionCode::Success))
    {
        if (Command->Status != STATUS_NO_SUCH_DEVICE)
        {
            DPRINT1("Endpoint %p DCI %lu drop for the client reset failed with code %lu\n",
                    Self, Self->m_Dci, static_cast<ULONG>(Command->Code));
            Self->FatalError(XHCI_REASON_DROP_FAILED);
        }

        Self->m_Controller->m_Buffers.Free(Data->InputContext);
        Data->InputContext = NULL;
        Self->m_ResetStatus = STATUS_SUCCESS;
        Self->PostCommandFailure();
        return;
    }

    for (StreamId = 1; StreamId <= Self->m_CurrentStreams->Count; StreamId++)
    {
        Ring = Self->m_CurrentStreams->Entry[StreamId - 1].Ring;
        Self->ResetStreamTransferLength(StreamId);
        Ring->InitializeRing();
        Self->StreamContext(StreamId)->DequeuePointer = Ring->DequeuePointerValue();
    }

    /* Add it back with the endpoint context stashed by the drop */
    Self->BuildEndpointConfigure(Data->InputContext, NULL, FALSE, TRUE, &Data->Command, ResetConfigureDone);
    Self->InputEndpointContext(Data->InputContext)->TRDequeuePointer = Self->EndpointDequeuePointer();
    Self->m_Controller->m_Commands.Submit(&Data->Command);
}

VOID
NTAPI
XhciEndpoint::ResetConfigureDone(
    _In_ XhciCommand* Command)
{
    XhciEndpoint* Self = static_cast<XhciEndpoint*>(Command->Context);
    XhciEndpointReset* Data = &XhciGetRequestData(Self->m_ResetRequest)->EndpointReset;

    Self->m_Controller->m_Buffers.Free(Data->InputContext);
    Data->InputContext = NULL;

    /* QUIRK: the client reset succeeds even when the configure fails */
    Self->m_ResetStatus = STATUS_SUCCESS;

    if (Command->Status == STATUS_NO_SUCH_DEVICE)
    {
        Self->PostCommandFailure();
        return;
    }

    if (Command->Code != XhciCompletionCode::Success)
    {
        DPRINT1("Endpoint %p DCI %lu add for the client reset failed with code %lu\n",
                Self, Self->m_Dci, static_cast<ULONG>(Command->Code));
        Self->FatalError(Self->UsesStreams() ? XHCI_REASON_STREAM_RECONFIGURE : XHCI_REASON_RECONFIGURE);
        Self->PostCommandFailure();
        return;
    }

    Self->PostEvent(XepEvent::CommandSucceeded);
}

VOID
NTAPI
XhciEndpoint::StopDropDone(
    _In_ XhciCommand* Command)
{
    XhciEndpoint* Self = static_cast<XhciEndpoint*>(Command->Context);

    if ((Command->Status == STATUS_NO_SUCH_DEVICE) || (Command->Code != XhciCompletionCode::Success))
    {
        if (Command->Status != STATUS_NO_SUCH_DEVICE)
        {
            DPRINT1("Endpoint %p DCI %lu drop after stop failed with code %lu\n",
                    Self, Self->m_Dci, static_cast<ULONG>(Command->Code));
            Self->FatalError(XHCI_REASON_DROP_FAILED);
        }

        Self->m_Controller->m_Buffers.Free(Self->m_StopInput);
        Self->m_StopInput = NULL;
        Self->PostCommandFailure();
        return;
    }

    /* Leave the stream context array alone; pending events are matched against the old ring contents */
    Self->BuildEndpointConfigure(Self->m_StopInput, NULL, FALSE, TRUE, &Self->m_Command, StopAddDone);
    Self->InputEndpointContext(Self->m_StopInput)->TRDequeuePointer = Self->EndpointDequeuePointer();
    Self->m_Controller->m_Commands.Submit(&Self->m_Command);
}

VOID
NTAPI
XhciEndpoint::StopAddDone(
    _In_ XhciCommand* Command)
{
    XhciEndpoint* Self = static_cast<XhciEndpoint*>(Command->Context);

    Self->m_Controller->m_Buffers.Free(Self->m_StopInput);
    Self->m_StopInput = NULL;

    if (Command->Status == STATUS_NO_SUCH_DEVICE)
    {
        Self->PostCommandFailure();
        return;
    }

    if (Command->Code != XhciCompletionCode::Success)
    {
        DPRINT1("Endpoint %p DCI %lu add after stop failed with code %lu\n",
                Self, Self->m_Dci, static_cast<ULONG>(Command->Code));
        Self->FatalError(XHCI_REASON_STOP_ADD_FAILED);
        Self->PostCommandFailure();
        return;
    }

    Self->SendClearStall();
}

/** Resets the device side sequence number with CLEAR_FEATURE(ENDPOINT_HALT) on our default endpoint queue. */
VOID
XhciEndpoint::SendClearStall()
{
    PUSB_DEFAULT_PIPE_SETUP_PACKET Setup;
    WDF_REQUEST_REUSE_PARAMS Reuse;
    struct _URB_CONTROL_TRANSFER_EX* Urb;
    PIO_STACK_LOCATION Stack;
    XhciEndpoint* Control;
    NTSTATUS Status;
    PIRP Irp;

    if (m_ClearStall == NULL)
    {
        PostEvent(XepEvent::CommandSucceeded);
        return;
    }

    Control = m_Device->EndpointAt(1);
    if (Control == NULL)
    {
        /* Post anyway so the endpoint machine does not wait forever */
        DPRINT1("Endpoint %p DCI %lu has no default endpoint for the clear stall\n", this, m_Dci);
        PostEvent(XepEvent::CommandSucceeded);
        return;
    }

    Irp = m_ClearStall->Irp;
    IoReuseIrp(Irp, STATUS_SUCCESS);

    Urb = &m_ClearStall->Urb;
    RtlZeroMemory(Urb, sizeof(*Urb));
    Urb->Hdr.Length = sizeof(*Urb);
    Urb->Hdr.Function = URB_FUNCTION_CONTROL_TRANSFER_EX;
    Urb->TransferFlags = USBD_DEFAULT_PIPE_TRANSFER | USB3_URB_RESERVED_RESOURCES;
    Urb->Timeout = XHCI_CLEAR_STALL_TIMEOUT_MS;

    Setup = reinterpret_cast<PUSB_DEFAULT_PIPE_SETUP_PACKET>(Urb->SetupPacket);
    Setup->bmRequestType.B = BMREQUEST_HOST_TO_DEVICE | (BMREQUEST_STANDARD << 5) | BMREQUEST_TO_ENDPOINT;
    Setup->bRequest = USB_REQUEST_CLEAR_FEATURE;
    Setup->wValue.W = USB_FEATURE_ENDPOINT_STALL;
    Setup->wIndex.W = m_Descriptor.bEndpointAddress;
    Setup->wLength = 0;

    /* Make the top location look like a dispatched internal IOCTL with our routine attached */
    Stack = IoGetNextIrpStackLocation(Irp);
    RtlZeroMemory(Stack, sizeof(*Stack));
    Stack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    Stack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
    Stack->Parameters.Others.Argument1 = Urb;

    Status = IoSetCompletionRoutineEx(m_Controller->m_WdmDevice, Irp, ClearStallDone, this, TRUE, TRUE, TRUE);
    if (!NT_SUCCESS(Status))
        IoSetCompletionRoutine(Irp, ClearStallDone, this, TRUE, TRUE, TRUE);

    IoSetNextIrpStackLocation(Irp);

    WDF_REQUEST_REUSE_PARAMS_INIT(&Reuse,
                                  WDF_REQUEST_REUSE_SET_NEW_IRP | XHCI_REQUEST_REUSE_MUST_COMPLETE,
                                  STATUS_SUCCESS);
    Reuse.NewIrp = Irp;
    WdfRequestReuse(m_ClearStall->Request, &Reuse);

    Status = WdfRequestForwardToIoQueue(m_ClearStall->Request, Control->m_MainRing->Queue());
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Endpoint %p clear stall forward failed 0x%lx\n", this, Status);
        WdfRequestComplete(m_ClearStall->Request, Status);
    }
}

/* A failed clear feature only leaves the device sequence number behind, the client recovers */
NTSTATUS
NTAPI
XhciEndpoint::ClearStallDone(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context)
{
    XhciEndpoint* Self = static_cast<XhciEndpoint*>(Context);

    UNREFERENCED_PARAMETER(DeviceObject);

    if (!NT_SUCCESS(Irp->IoStatus.Status))
        DPRINT1("Endpoint %p clear stall finished with 0x%lx\n", Self, Irp->IoStatus.Status);

    Self->PostEvent(XepEvent::CommandSucceeded);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

/* Endpoint machine actions ***************************************************/

VOID
XhciEndpointMachine::ReferenceEndpoint()
{
    WdfObjectReference(m_Endpoint->m_Ucx);
}

VOID
XhciEndpointMachine::DereferenceEndpoint()
{
    WdfObjectDereference(m_Endpoint->m_Ucx);
}

BOOLEAN
XhciEndpointMachine::StopTimer()
{
    return WdfTimerStop(m_Endpoint->m_Timer, FALSE);
}

VOID
XhciEndpointMachine::StartTimer(
    _In_ ULONG Milliseconds)
{
    WdfTimerStart(m_Endpoint->m_Timer, WDF_REL_TIMEOUT_IN_MS(Milliseconds));
}

VOID
XhciEndpointMachine::ReportTimeout(
    _In_ XEP_TIMEOUT Timeout)
{
    XhciEndpoint* Ep = m_Endpoint;

    switch (Timeout)
    {
        case XepTimeoutHalt:
            DPRINT1("Endpoint %p DCI %lu stop failed as halted but no halt event came\n", Ep, Ep->m_Dci);
            break;

        case XepTimeoutStoppedEvent:
            DPRINT1("Endpoint %p DCI %lu got no stopped event after the stop\n", Ep, Ep->m_Dci);
            break;

        case XepTimeoutDrainAfterStop:
            DPRINT1("Endpoint %p DCI %lu expected events still missing after the stop\n", Ep, Ep->m_Dci);
            break;

        default:
            DPRINT1("Endpoint %p DCI %lu expected events still missing after the halt\n", Ep, Ep->m_Dci);
            break;
    }

}

VOID
XhciEndpointMachine::ResumeRingFill()
{
    XhciEndpoint* Ep = m_Endpoint;

    InterlockedExchange(&Ep->m_Flags, 0);
    if (Ep->UsesStreams())
        Ep->m_CurrentStreams->HaltedCode = 0;

    Ep->ForEachRing(&XhciTransferRing::ResumeRingFill);
}

VOID
XhciEndpointMachine::SuspendRingFill()
{
    m_Endpoint->ForEachRing(&XhciTransferRing::SuspendRingFill);
}

VOID
XhciEndpointMachine::AckControllerReset()
{
    XhciEndpoint* Ep = m_Endpoint;

    if (InterlockedXor(&Ep->m_Flags, XHCI_EP_RESET_ACK_HANDSHAKE) & XHCI_EP_RESET_ACK_HANDSHAKE)
        KeSetEvent(&Ep->m_ResetAck, IO_NO_INCREMENT, FALSE);
}

VOID
XhciEndpointMachine::CompleteResetRequest()
{
    XhciEndpoint* Ep = m_Endpoint;
    WDFREQUEST Request;
    NTSTATUS Status;

    InterlockedAnd(&Ep->m_Flags, ~XHCI_EP_CLIENT_RESET_HANDSHAKE);

    Status = Ep->m_ResetStatus;
    Ep->m_ResetStatus = STATUS_PENDING;
    Request = static_cast<WDFREQUEST>(
        InterlockedExchangePointer(reinterpret_cast<PVOID volatile*>(&Ep->m_ResetRequest), NULL));

    if (Request == NULL)
        return;

    /* A controller reset or removal got here first; complete with success, not STATUS_PENDING */
    if (Status == STATUS_PENDING)
        Status = STATUS_SUCCESS;

    DPRINT("Endpoint %p client reset completed with 0x%lx\n", Ep, Status);
    WdfRequestComplete(Request, Status);
}

VOID
XhciEndpointMachine::CompleteResetRequestIfAllowed()
{
    XhciEndpoint* Ep = m_Endpoint;

    if (InterlockedXor(&Ep->m_Flags, XHCI_EP_CLIENT_RESET_HANDSHAKE) & XHCI_EP_CLIENT_RESET_HANDSHAKE)
        CompleteResetRequest();
}

VOID
XhciEndpointMachine::ReportStoppedEvent()
{
    XhciEndpoint* Ep = m_Endpoint;

    if (InterlockedXor(&Ep->m_Flags, XHCI_EP_STOPPED_HANDSHAKE) & XHCI_EP_STOPPED_HANDSHAKE)
        Ep->PostEvent(XepEvent::StoppedEventSeen);
}

VOID
XhciEndpointMachine::DropStoppedEvent()
{
    InterlockedOr(&m_Endpoint->m_Flags, XHCI_EP_IGNORE_STOPPED);
}

VOID
XhciEndpointMachine::ResetStopAttempts()
{
    m_Endpoint->m_StopAttempts = 0;
}

VOID
XhciEndpointMachine::CountStopAttempt()
{
    m_Endpoint->m_StopAttempts++;
}

VOID
XhciEndpointMachine::SendStopCommand()
{
    XhciEndpoint* Ep = m_Endpoint;
    XhciCommand* Command = &Ep->m_Command;

    RtlZeroMemory(Command, sizeof(*Command));
    Command->Trb.Dword[3] = XhciEndpointCommandControl(XhciTrbType::StopEndpoint,
                                                       Ep->m_Dci,
                                                       Ep->m_Device->SlotId());
    Command->Done = XhciEndpoint::StopDone;
    Command->Context = Ep;

    Ep->m_Controller->m_Commands.Submit(Command);
}

VOID
XhciEndpointMachine::NotifyRingsHalted()
{
    m_Endpoint->ForEachRing(&XhciTransferRing::OnPipeHalted);
}

VOID
XhciEndpointMachine::NotifyRingsClientReset()
{
    m_Endpoint->ForEachRing(&XhciTransferRing::OnClientPipeReset);
}

VOID
XhciEndpointMachine::NotifyRingsStoppedEvent()
{
    m_Endpoint->ForEachRing(&XhciTransferRing::StoppedEventReceived);
}

VOID
XhciEndpointMachine::NotifyRingsReclaimOnCancel()
{
    m_Endpoint->ForEachRing(&XhciTransferRing::AllowReclaimOnCancel);
}

VOID
XhciEndpointMachine::NotifyRingsDrainEvents()
{
    m_Endpoint->ForEachRing(&XhciTransferRing::ConsumePendingEvents);
}

/* The endpoint stays referenced until every ring reported its transfers reclaimed */
VOID
XhciEndpointMachine::NotifyRingsReclaim()
{
    WdfObjectReferenceWithTag(m_Endpoint->m_Ucx, XHCI_RECLAIM_TAG);
    m_Endpoint->ForEachRing(&XhciTransferRing::RecoverTransfers);
}

VOID
XhciEndpointMachine::AskUcxToCancel()
{
    UcxEndpointNeedToCancelTransfers(m_Endpoint->m_Ucx);
}

/* Stream queues are purged only when a halt matched no stream transfer */
VOID
XhciEndpointMachine::PurgeStreamQueues()
{
    XhciEndpoint* Ep = m_Endpoint;
    ULONG Index;

    if (!(Ep->m_Flags & XHCI_EP_STREAM_HALT_PENDING) || !Ep->UsesStreams())
        return;

    for (Index = 0; Index < Ep->m_CurrentStreams->Count; Index++)
        WdfIoQueuePurge(Ep->m_CurrentStreams->Entry[Index].Ring->Queue(), NULL, NULL);
}

VOID
XhciEndpointMachine::RestartStreamQueues()
{
    XhciEndpoint* Ep = m_Endpoint;
    ULONG Index;

    if (!(Ep->m_Flags & XHCI_EP_STREAM_HALT_PENDING) || !Ep->UsesStreams())
        return;

    for (Index = 0; Index < Ep->m_CurrentStreams->Count; Index++)
        WdfIoQueueStart(Ep->m_CurrentStreams->Entry[Index].Ring->Queue());
}

/* Resets the data toggle or sequence number of an endpoint that is not halted */
VOID
XhciEndpointMachine::ReconfigureForReset()
{
    XhciEndpoint* Ep = m_Endpoint;
    XhciEndpointReset* Data = &XhciGetRequestData(Ep->m_ResetRequest)->EndpointReset;
    PVOID Original = Ep->m_Device->OutputContext(Ep->m_Dci);
    ULONG Size = Ep->m_Controller->m_Registers.ContextSize();

    Data->InputContext = Ep->m_Controller->m_Buffers.Acquire(Size * XHCI_INPUT_CONTEXT_COUNT);
    if (Data->InputContext == NULL)
    {
        DPRINT1("Endpoint %p DCI %lu no input context for the client reset\n", Ep, Ep->m_Dci);
        Ep->m_ResetStatus = STATUS_INSUFFICIENT_RESOURCES;
        Ep->PostEvent(XepEvent::BufferAllocFailed);
        return;
    }

    if (Ep->UsesStreams())
    {
        /* Drop first; the copied endpoint context waits in the buffer for the add */
        Ep->BuildEndpointConfigure(Data->InputContext, Original, TRUE, FALSE,
                                   &Data->Command, XhciEndpoint::ResetDropDone);
    }
    else
    {
        Ep->BuildEndpointConfigure(Data->InputContext, Original, TRUE, TRUE,
                                   &Data->Command, XhciEndpoint::ResetConfigureDone);
        Ep->m_MainRing->InitializeRing();
        Ep->InputEndpointContext(Data->InputContext)->TRDequeuePointer = Ep->m_MainRing->DequeuePointerValue();
    }

    Ep->m_Controller->m_Commands.Submit(&Data->Command);
}

/* Some controllers need a stream endpoint configured again after a stop */
VOID
XhciEndpointMachine::ReconfigureAfterStop()
{
    XhciEndpoint* Ep = m_Endpoint;
    PVOID Original = Ep->m_Device->OutputContext(Ep->m_Dci);
    ULONG Size = Ep->m_Controller->m_Registers.ContextSize();

    Ep->m_StopInput = Ep->m_Controller->m_Buffers.Acquire(Size * XHCI_INPUT_CONTEXT_COUNT);
    if (Ep->m_StopInput == NULL)
    {
        DPRINT1("Endpoint %p DCI %lu no input context for the reconfigure after stop\n", Ep, Ep->m_Dci);
        Ep->m_Controller->RaiseControllerFault(XhciRecovery::ResetHost, XHCI_REASON_NO_LIVE_DUMP);
        Ep->PostCommandFailure();
        return;
    }

    Ep->BuildEndpointConfigure(Ep->m_StopInput, Original, TRUE, FALSE,
                               &Ep->m_Command, XhciEndpoint::StopDropDone);
    Ep->m_Controller->m_Commands.Submit(&Ep->m_Command);
}

/* Moves every ring past the canceled transfers */
VOID
XhciEndpointMachine::UpdateDequeuePointers()
{
    XhciEndpoint* Ep = m_Endpoint;
    ULONG StreamId;

    if (!Ep->UsesStreams())
    {
        Ep->DequeueAfterStop(0);
        return;
    }

    InterlockedExchange(&Ep->m_CurrentStreams->UpdateCount, 0);
    for (StreamId = 1; StreamId <= Ep->m_CurrentStreams->Count; StreamId++)
        Ep->DequeueAfterStop(StreamId);
}

VOID
XhciEndpointMachine::ResetDefaultPipe()
{
    XhciEndpoint* Ep = m_Endpoint;
    XhciCommand* Command = &Ep->m_Command;

    RtlZeroMemory(Command, sizeof(*Command));
    Command->Trb.Dword[3] = XhciEndpointCommandControl(XhciTrbType::ResetEndpoint,
                                                       Ep->m_Dci,
                                                       Ep->m_Device->SlotId());
    Command->Done = XhciEndpoint::ControlResetDone;
    Command->Context = Ep;

    Ep->m_Controller->m_Commands.Submit(Command);
}

VOID
XhciEndpointMachine::ResetEndpoint()
{
    XhciEndpoint* Ep = m_Endpoint;
    XhciCommand* Command = &XhciGetRequestData(Ep->m_ResetRequest)->EndpointReset.Command;

    RtlZeroMemory(Command, sizeof(*Command));
    Command->Trb.Dword[3] = XhciEndpointCommandControl(XhciTrbType::ResetEndpoint,
                                                       Ep->m_Dci,
                                                       Ep->m_Device->SlotId());
    if (Ep->ResetPayload()->Flags & FlagEndpointResetPreserveTransferState)
        Command->Trb.Dword[3] |= XHCI_CMD_TRANSFER_STATE_PRESERVE;

    Command->Done = XhciEndpoint::ClientResetDone;
    Command->Context = Ep;

    Ep->m_Controller->m_Commands.Submit(Command);
}

VOID
XhciEndpointMachine::RequestControllerReset()
{
    XhciEndpoint* Ep = m_Endpoint;

    DPRINT1("Endpoint %p DCI %lu would not stop after %lu attempts\n", Ep, Ep->m_Dci, Ep->m_StopAttempts);
    Ep->m_Controller->VerifierCheck(XHCI_VERIFY_STOP_EXHAUSTED);
    Ep->FatalError(XHCI_REASON_STOP_CONTEXT);
}

/* Endpoint machine queries ***************************************************/

BOOLEAN
XhciEndpointMachine::IsControlEndpoint()
{
    return m_Endpoint->m_XhciType == XHCI_ENDPOINT_TYPE_CONTROL;
}

BOOLEAN
XhciEndpointMachine::OnPrimaryInterrupterOnly()
{
    return m_Endpoint->m_Controller->HasErrata(XhciErrata::IntPrimaryOnly);
}

BOOLEAN
XhciEndpointMachine::DelaysFirstStop()
{
    return m_Endpoint->m_Controller->HasErrata(XhciErrata::EpDeferFirstStop);
}

BOOLEAN
XhciEndpointMachine::IgnoresStopContextError()
{
    XhciController* Controller = m_Endpoint->m_Controller;

    if (!Controller->HasErrata(XhciErrata::EpStopContextErrorIsStopped))
        return FALSE;

    NT_ASSERT(Controller->HasErrata(XhciErrata::IntPrimaryOnly));
    return TRUE;
}

BOOLEAN
XhciEndpointMachine::StopAttemptsExhausted()
{
    return m_Endpoint->m_StopAttempts >= XHCI_STOP_ATTEMPTS_MAX;
}

BOOLEAN
XhciEndpointMachine::DoorbellRungSinceFill()
{
    XhciEndpoint* Ep = m_Endpoint;
    ULONG Index;

    if (!Ep->UsesStreams())
        return Ep->m_MainRing->DoorbellRungSinceFill();

    /* QUIRK: asking also resets the shared stream counter */
    InterlockedExchange(&Ep->m_CurrentStreams->UpdateCount, 0);
    for (Index = 0; Index < Ep->m_CurrentStreams->Count; Index++)
    {
        if (Ep->m_CurrentStreams->Entry[Index].Ring->DoorbellRungSinceFill())
            return TRUE;
    }

    return FALSE;
}

BOOLEAN
XhciEndpointMachine::NeedsContextRebuildAfterStop()
{
    return m_Endpoint->m_StreamsCapable &&
           m_Endpoint->m_Controller->HasErrata(XhciErrata::StrmReconfigOnStop);
}

BOOLEAN
XhciEndpointMachine::ShouldReconfigureForReset()
{
    XhciEndpoint* Ep = m_Endpoint;
    ULONG Type = Ep->TransferType();

    /* Like the USB 2.0 stack: nothing to do for a preserved state, control or isoch */
    if ((Ep->ResetPayload()->Flags & FlagEndpointResetPreserveTransferState) ||
        (Type == USB_ENDPOINT_TYPE_CONTROL) ||
        (Type == USB_ENDPOINT_TYPE_ISOCHRONOUS))
    {
        Ep->m_ResetStatus = STATUS_SUCCESS;
        return FALSE;
    }

    if (!Ep->AnyTransfersPending())
        return TRUE;

    /* QUIRK: with transfers pending a bulk or interrupt reset succeeds without a toggle reset */
    Ep->m_ResetStatus = Ep->m_StreamsCapable ? STATUS_UNSUCCESSFUL : STATUS_SUCCESS;
    return FALSE;
}

BOOLEAN
XhciEndpointMachine::CanResetAfterHalt()
{
    XhciEndpoint* Ep = m_Endpoint;

    if (Ep->m_StreamsCapable && Ep->AnyTransfersPending())
    {
        Ep->m_ResetStatus = STATUS_UNSUCCESSFUL;
        return FALSE;
    }

    return TRUE;
}
