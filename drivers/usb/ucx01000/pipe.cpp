/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     UCXENDPOINT and UCXSSTREAMS objects and the endpoint machine actions
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ucx01000.h"

#define NDEBUG
#include <debug.h>

/* Derived values */

#define UCX_KB(x) ((x) * 1024UL)
#define UCX_MB(x) ((x) * 1024UL * 1024UL)

/* Rows are Low, Full, High, Super; columns are UcxTransferType, isoch is computed instead */
static const ULONG UcxMaxTransferTable[4][5] =
{
    /* Unknown  Control      Bulk        Isoch  Interrupt */
    {  0,       UCX_KB(4),   UCX_MB(4),  0,     UCX_MB(4) },
    {  0,       UCX_KB(4),   UCX_MB(4),  0,     UCX_MB(4) },
    {  0,       UCX_KB(64),  UCX_MB(4),  0,     UCX_MB(4) },
    {  0,       UCX_KB(64),  UCX_MB(32), 0,     UCX_MB(4) }
};

/* Anything faster than SuperSpeed is sized as SuperSpeed */
static
ULONG
NTAPI
UcxSpeedRow(
    _In_ USB_DEVICE_SPEED Speed)
{
    return ((ULONG)Speed > (ULONG)UsbSuperSpeed) ? (ULONG)UsbSuperSpeed : (ULONG)Speed;
}

static
BOOLEAN
NTAPI
UcxIsSlowSpeed(
    _In_ USB_DEVICE_SPEED Speed)
{
    return Speed == UsbLowSpeed || Speed == UsbFullSpeed;
}

ULONG
NTAPI
UcxComputeMaxTransferSize(
    _In_ USB_DEVICE_SPEED Speed,
    _In_ USHORT BcdUsb,
    _In_ const UcxEndpointInit* Init)
{
    ULONG Row = UcxSpeedRow(Speed);
    USHORT MaxPacket;
    ULONG PacketBytes;

    if (Init->TransferType != UcxTransferType::Isochronous)
    {
        if (Init->TransferType == UcxTransferType::Control &&
            UcxIsSlowSpeed(Speed) &&
            UcxDriver.Allow64KLowOrFullSpeedControl)
        {
            return UCX_KB(64);
        }

        return UcxMaxTransferTable[Row][(ULONG)Init->TransferType];
    }

    if (Row == (ULONG)UsbSuperSpeed)
    {
        if (Init->Companion.bmAttributes.Isochronous.SspCompanion)
            return 1024 * Init->IsochCompanion.dwBytesPerInterval;

        return 1024 * Init->Companion.wBytesPerInterval;
    }

    /* Before USB 2.5 the top bits carry the additional transactions per microframe */
    MaxPacket = Init->Descriptor.wMaxPacketSize;
    if (BcdUsb < 0x0250)
        PacketBytes = (MaxPacket & 0x7FF) * (((MaxPacket >> 11) & 0x3) + 1);
    else
        PacketBytes = MaxPacket;

    return ((Row == (ULONG)UsbHighSpeed) ? 1024 : 256) * PacketBytes;
}

/* Exports */

UcxEndpoint*
UcxEndpoint::FromHandle(
    _In_ UCXENDPOINT Handle)
{
    return UcxGetEndpointContext(Handle);
}

VOID
UcxEndpoint::InitSetEventCallbacks(
    _Inout_ PUCXENDPOINT_INIT Init,
    _In_ PUCX_ENDPOINT_EVENT_CALLBACKS Callbacks)
{
    UcxEndpointCallbacks* Target = &Init->Callbacks;

    NT_ASSERT(Init->Kind != UcxEndpointKind::Default);

    Target->Purge = Callbacks->EvtEndpointPurge;
    Target->Start = Callbacks->EvtEndpointStart;
    Target->Abort = Callbacks->EvtEndpointAbort;
    Target->Reset = Callbacks->EvtEndpointReset;
    Target->OkToCancelTransfers = Callbacks->EvtEndpointOkToCancelTransfers;
    Target->StaticStreamsAdd = Callbacks->EvtEndpointStaticStreamsAdd;
    Target->StaticStreamsEnable = Callbacks->EvtEndpointStaticStreamsEnable;
    Target->StaticStreamsDisable = Callbacks->EvtEndpointStaticStreamsDisable;
    Target->EnableForwardProgress = (PFN_UCX_HCD_ENDPOINT_ENABLE_FORWARD_PROGRESS)Callbacks->Reserved1;

    /* Appended by newer controller drivers, taken only from the full structure */
    if (Callbacks->Size == sizeof(*Callbacks))
    {
        Target->GetIsochTransferPathDelays = Callbacks->EvtEndpointGetIsochTransferPathDelays;
        Target->SetCharacteristic = Callbacks->EvtEndpointSetCharacteristic;
    }
}

VOID
UcxEndpoint::InitSetDefaultEventCallbacks(
    _Inout_ PUCXENDPOINT_INIT Init,
    _In_ PUCX_DEFAULT_ENDPOINT_EVENT_CALLBACKS Callbacks)
{
    UcxEndpointCallbacks* Target = &Init->Callbacks;

    NT_ASSERT(Callbacks->Size == sizeof(*Callbacks));
    NT_ASSERT(Init->Kind == UcxEndpointKind::Default);

    Target->DefaultEndpointUpdate = Callbacks->EvtDefaultEndpointUpdate;
    Target->Purge = Callbacks->EvtEndpointPurge;
    Target->Start = Callbacks->EvtEndpointStart;
    Target->Abort = Callbacks->EvtEndpointAbort;
    Target->OkToCancelTransfers = Callbacks->EvtEndpointOkToCancelTransfers;
    Target->EnableForwardProgress = (PFN_UCX_HCD_ENDPOINT_ENABLE_FORWARD_PROGRESS)Callbacks->Reserved1;
}

VOID
UcxEndpoint::Initialize(
    _In_ UcxUsbDevice* Device,
    _In_ const UcxEndpointInit* Init)
{
    USB_DEVICE_SPEED Speed = Device->Speed();

    m_Controller = Device->m_Controller;
    m_Device = Device;
    m_Callbacks = Init->Callbacks;
    m_Descriptor = Init->Descriptor;

    m_Pipe.Endpoint = this;
    m_Pipe.Kind = Init->Kind;
    m_Pipe.TransferType = Init->TransferType;
    m_Pipe.Self = &m_Pipe;
    m_Pipe.MaximumTransferSize = UcxComputeMaxTransferSize(Speed, Device->m_BcdUsb, Init);

    if (Init->TransferType != UcxTransferType::Control)
        m_Pipe.DirectionIn = USB_ENDPOINT_DIRECTION_IN(m_Descriptor.bEndpointAddress) ? TRUE : FALSE;

    if (Init->TransferType == UcxTransferType::Isochronous)
    {
        if (UcxIsSlowSpeed(Speed))
        {
            m_Pipe.IsochPacketLimit = 255;
            m_Pipe.IsochPeriodMicroframes = 8;
        }
        else
        {
            /* bInterval 0 wraps to the top bit, as the x86 shift does */
            m_Pipe.IsochPacketLimit = 1024;
            m_Pipe.IsochPeriodMicroframes = 1UL << ((m_Descriptor.bInterval - 1) & 31);
        }
    }

    /* Only async transfers of slow devices behind a real TT need the TT cleared */
    m_ClearTtOnCancel = m_Controller->m_ClearTtBufferOnAsyncCancel &&
                        Init->TransferType != UcxTransferType::Interrupt &&
                        Init->TransferType != UcxTransferType::Isochronous &&
                        UcxIsSlowSpeed(Speed) &&
                        Device->m_Info.TtHub != NULL;

    UcxClearListEntry(&m_DeviceLink);
    UcxClearListEntry(&m_TrackingLink);
    UcxClearListEntry(&m_DisconnectLink);
    UcxClearListEntry(&m_ControllerResetLink);
    UcxClearListEntry(&m_TreePurgeLink);
    UcxClearListEntry(&m_OperationLink);
}

_Must_inspect_result_
NTSTATUS
UcxEndpoint::Create(
    _In_ UCXUSBDEVICE UsbDevice,
    _Inout_ PUCXENDPOINT_INIT* EndpointInit,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXENDPOINT* Endpoint)
{
    UcxEndpointInit* Init = *EndpointInit;
    UcxUsbDevice* Device = UcxUsbDevice::FromHandle(UsbDevice);
    WDF_OBJECT_ATTRIBUTES Primary;
    UcxEndpoint* Context;
    WDFOBJECT Object;
    NTSTATUS Status = STATUS_SUCCESS;

    *Endpoint = NULL;

    /* Parented to the init block's device, referenced through the parameter */
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Primary, UcxEndpoint);
    Primary.ParentObject = Init->Device;
    Primary.EvtDestroyCallback = EvtDestroy;

    Status = UcxCreateObjectWithTwoContexts(&Primary, Attributes, &Object);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Endpoint object create failed for UsbDevice %p 0x%lx\n", UsbDevice, Status);
        return Status;
    }

    WdfObjectReference(UsbDevice);

    Context = new (UcxGetEndpointContext(Object)) UcxEndpoint();
    Context->m_Handle = (UCXENDPOINT)Object;
    Context->Initialize(Device, Init);

    if (Init->CreateMachine)
    {
        Context->m_Machine.Initialize(Context);
        Context->m_HasMachine = TRUE;

        /* Creation reference, dropped by the machine once it deletes the object */
        WdfObjectReference(Object);
    }

    /* Recorded before the disconnect check, so a failure leaves them behind */
    if (Init->Kind == UcxEndpointKind::Default)
    {
        NT_ASSERT(Device->m_DefaultEndpoint == NULL);
        Device->m_DefaultEndpoint = Context;
        Device->m_DefaultPipe = &Context->m_Pipe;
    }

    {
        SpinLockGuard Guard(&Context->m_Controller->m_TopologyLock);

        if (Device->m_Disconnected)
        {
            Status = STATUS_DEVICE_NOT_CONNECTED;
        }
        else
        {
            InsertHeadList(&Device->m_EndpointList, &Context->m_DeviceLink);
            Context->m_Controller->m_ChildEndpointCount++;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Endpoint %p not created, UsbDevice %p is disconnected\n", Object, UsbDevice);

        if (Context->m_HasMachine)
            WdfObjectDereference(Object);

        WdfObjectDelete(Object);
        return Status;
    }

    /* Tracking may have been turned on after the insert above and taken it already */
    if (Device->m_StalePipeIoAllowed)
    {
        SpinLockGuard Guard(&Device->m_TrackingLock);

        if (!UcxIsListEntryLinked(&Context->m_TrackingLink))
            InsertTailList(&Device->m_TrackingList, &Context->m_TrackingLink);
    }

    DPRINT("Endpoint %p address 0x%x created on UsbDevice %p\n",
           Context->m_Handle,
           Context->m_Descriptor.bEndpointAddress,
           UsbDevice);

    *Endpoint = Context->m_Handle;
    Init->Created = Context->m_Handle;
    *EndpointInit = NULL;

    return STATUS_SUCCESS;
}

/* A context that was never filled has a NULL device and no queue */
VOID
NTAPI
UcxEndpoint::EvtDestroy(
    _In_ WDFOBJECT Object)
{
    UcxEndpoint* Context = UcxGetEndpointContext(Object);

    Context->m_HasMachine = FALSE;

    if (Context->m_Pipe.Queue != NULL)
    {
        WdfObjectDereference(Context->m_Pipe.Queue);
        Context->m_Pipe.Queue = NULL;
    }

    Context->m_Pipe.Self = NULL;

    if (Context->m_Device != NULL)
        WdfObjectDereference(Context->m_Device->m_Handle);
}

/* The first queue wins; later calls are a controller driver bug and ignored */
VOID
UcxEndpoint::SetWdfIoQueue(
    _In_ WDFQUEUE Queue)
{
    if (m_Pipe.Queue != NULL)
    {
        DPRINT1("Endpoint %p already has queue %p\n", m_Handle, m_Pipe.Queue);
        return;
    }

    WdfObjectReference(Queue);
    m_Pipe.Queue = Queue;
}

/** Clears the TT buffer first for a low or full speed device behind a TT. */
VOID
UcxEndpoint::NeedToCancelTransfers()
{
    const USB_DEVICE_PORT_PATH* Path = &m_Device->m_Info.PortPath;
    UcxUsbDevice* TtHub;

    if (!m_ClearTtOnCancel)
    {
        m_Callbacks.OkToCancelTransfers(m_Handle);
        return;
    }

    /* A second call during a clear never gets its OK */
    if (InterlockedCompareExchange(&m_CancelSync, 2, 0) != 0)
    {
        DPRINT1("Endpoint %p HCD asked to cancel transfers again during a TT clear\n", m_Handle);
        NT_ASSERT(FALSE);
        return;
    }

    WdfObjectReference(m_Handle);

    TtHub = UcxUsbDevice::FromHandle(m_Device->m_Info.TtHub);
    TtHub->m_HubClearTtBuffer(TtHub->m_HubContext,
                              m_Device->m_HubDeviceContext,
                              m_Handle,
                              m_Descriptor.bEndpointAddress,
                              Path->PortPath[Path->TTHubDepth]);

    ReleaseCancelSync();
}

VOID
UcxEndpoint::ReleaseCancelSync()
{
    if (InterlockedDecrement(&m_CancelSync) != 0)
        return;

    m_Callbacks.OkToCancelTransfers(m_Handle);
    WdfObjectDereference(m_Handle);
}

UCXSSTREAMS
UcxEndpoint::GetStaticStreamsReferenced(
    _In_ PVOID Tag)
{
    UCXSSTREAMS Streams = NULL;

    if (m_Streams == NULL)
        return NULL;

    SpinLockGuard Guard(&m_Controller->m_TopologyLock);

    if (m_Streams != NULL)
    {
        Streams = m_Streams->m_Handle;
        WdfObjectReferenceWithTag(Streams, Tag);
    }

    return Streams;
}

/* Hub interface */

NTSTATUS
UcxEndpoint::CreateThroughHcd(
    _In_ UcxUsbDevice* Device,
    _Inout_ UcxEndpointInit* Init,
    _In_ ULONG MaxPacketSize,
    _In_opt_ PUSB_ENDPOINT_DESCRIPTOR Descriptor,
    _In_ ULONG DescriptorBufferLength,
    _In_opt_ PUSB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR Companion,
    _Out_ UCXENDPOINT* Endpoint)
{
    const UCX_USBDEVICE_EVENT_CALLBACKS* Hcd = &Device->m_Callbacks.Public;
    UCXCONTROLLER Controller = Device->m_Controller->m_Handle;
    NTSTATUS Status;

    PAGED_CODE();

    Init->Device = Device->m_Handle;
    Init->CreateMachine = TRUE;

    if (Init->Kind == UcxEndpointKind::Default)
    {
        Status = Hcd->EvtUsbDeviceDefaultEndpointAdd(Controller, Device->m_Handle, MaxPacketSize, Init);
    }
    else
    {
        Status = Hcd->EvtUsbDeviceEndpointAdd(Controller,
                                              Device->m_Handle,
                                              Descriptor,
                                              DescriptorBufferLength,
                                              Companion,
                                              Init);
    }

    if (!NT_SUCCESS(Status))
        DPRINT1("UsbDevice %p HCD endpoint add failed 0x%lx\n", Device->m_Handle, Status);

    /* Success without an endpoint or a queue leaves nothing to send transfers to */
    if (NT_SUCCESS(Status) &&
        (Init->Created == NULL || FromHandle(Init->Created)->m_Pipe.Queue == NULL))
    {
        DPRINT1("UsbDevice %p HCD endpoint add set no endpoint %p or no queue\n", Device->m_Handle, Init->Created);
        UcxVerifierBreak(Device->m_Controller->m_DriverVerifierEnabled);
        Status = STATUS_INTERNAL_ERROR;
    }

    if (NT_SUCCESS(Status))
    {
        *Endpoint = Init->Created;
        return Status;
    }

    *Endpoint = NULL;
    if (Init->Created != NULL)
        FromHandle(Init->Created)->DeleteFromHub();

    return Status;
}

_Must_inspect_result_
NTSTATUS
UcxEndpoint::CreateDefaultFromHub(
    _In_ UCXUSBDEVICE UsbDevice,
    _In_ ULONG MaxPacketSize,
    _Out_ UCXENDPOINT* Endpoint)
{
    UcxEndpointInit Init;

    RtlZeroMemory(&Init, sizeof(Init));
    Init.Kind = UcxEndpointKind::Default;
    Init.TransferType = UcxTransferType::Control;

    return CreateThroughHcd(UcxUsbDevice::FromHandle(UsbDevice), &Init, MaxPacketSize, NULL, 0, NULL, Endpoint);
}

_Must_inspect_result_
NTSTATUS
UcxEndpoint::CreateFromHub(
    _In_ UCXUSBDEVICE UsbDevice,
    _In_ PUSB_ENDPOINT_DESCRIPTOR Descriptor,
    _In_ ULONG DescriptorBufferLength,
    _In_opt_ PUSB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR Companion,
    _Out_ UCXENDPOINT* Endpoint)
{
    static const UcxTransferType TypeFromAttributes[4] =
    {
        UcxTransferType::Control,
        UcxTransferType::Isochronous,
        UcxTransferType::Bulk,
        UcxTransferType::Interrupt
    };
    UcxEndpointInit Init;

    RtlZeroMemory(&Init, sizeof(Init));
    Init.TransferType = TypeFromAttributes[Descriptor->bmAttributes & USB_ENDPOINT_TYPE_MASK];
    Init.Kind = (Descriptor->wMaxPacketSize == 0) ? UcxEndpointKind::ZeroBandwidth : UcxEndpointKind::Generic;
    RtlCopyMemory(&Init.Descriptor, Descriptor, sizeof(Init.Descriptor));

    if (Companion != NULL)
    {
        RtlCopyMemory(&Init.Companion, Companion, sizeof(Init.Companion));
        Init.HasCompanion = TRUE;

        /* The SuperSpeedPlus companion follows right after; the buffer length is not checked */
        if (Init.TransferType == UcxTransferType::Isochronous &&
            Companion->bmAttributes.Isochronous.SspCompanion)
        {
            RtlCopyMemory(&Init.IsochCompanion,
                          (PUCHAR)Companion + Companion->bLength,
                          sizeof(Init.IsochCompanion));
            Init.HasIsochCompanion = TRUE;
        }
    }

    return CreateThroughHcd(UcxUsbDevice::FromHandle(UsbDevice),
                            &Init,
                            0,
                            Descriptor,
                            DescriptorBufferLength,
                            Companion,
                            Endpoint);
}

/* The machine decides whether it goes now or stays around as stale */
VOID
UcxEndpoint::DeleteFromHub()
{
    Post(EpEvent::Delete);
}

/* Static streams */

UcxStaticStreams*
UcxStaticStreams::FromHandle(
    _In_ UCXSSTREAMS Handle)
{
    return UcxGetStaticStreamsContext(Handle);
}

_Must_inspect_result_
NTSTATUS
UcxStaticStreams::Create(
    _In_ UCXENDPOINT Endpoint,
    _Inout_ PUCXSSTREAMS_INIT* Init,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXSSTREAMS* StaticStreams)
{
    UcxStaticStreamsInit* StreamsInit = *Init;
    WDF_OBJECT_ATTRIBUTES Primary;
    UcxStaticStreams* Context;
    WDFOBJECT Object;
    NTSTATUS Status;

    *StaticStreams = NULL;

    /* The stream entries follow the context in the same allocation */
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Primary, UcxStaticStreams);
    Primary.ContextSizeOverride = sizeof(UcxStaticStreams) + StreamsInit->StreamCount * sizeof(UcxStream);
    Primary.ParentObject = StreamsInit->Endpoint;
    Primary.EvtDestroyCallback = EvtDestroy;

    Status = UcxCreateObjectWithTwoContexts(&Primary, Attributes, &Object);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Streams object create failed for endpoint %p 0x%lx\n", Endpoint, Status);
        return Status;
    }

    Context = new (UcxGetStaticStreamsContext(Object)) UcxStaticStreams();
    Context->m_Handle = (UCXSSTREAMS)Object;
    Context->m_StreamCount = StreamsInit->StreamCount;
    Context->m_Endpoint = UcxEndpoint::FromHandle(Endpoint);
    Context->m_Init = StreamsInit;

    StreamsInit->Created = Context->m_Handle;
    *StaticStreams = Context->m_Handle;
    *Init = NULL;

    return STATUS_SUCCESS;
}

/** Called per stream with IDs counting up from 1; a bad call fails the whole set. */
VOID
UcxStaticStreams::SetStreamInfo(
    _In_ PSTREAM_INFO StreamInfo)
{
    BOOLEAN Verifying = m_Endpoint->m_Controller->m_DriverVerifierEnabled;
    UcxStaticStreamsInit* Init = m_Init;
    UcxPipe* EndpointPipe = &m_Endpoint->m_Pipe;
    UcxStream* Entry;
    ULONG Index;

    if (Init == NULL)
    {
        DPRINT1("Streams %p stream info set outside the add callback\n", m_Handle);
        UcxVerifierBreak(Verifying);
        return;
    }

    if (StreamInfo->Size != sizeof(*StreamInfo))
    {
        DPRINT1("Streams %p stream info has bad size %lu\n", m_Handle, StreamInfo->Size);
        Init->Failed = TRUE;
        UcxVerifierBreak(Verifying);
        return;
    }

    /* Counted even when this call then fails */
    Index = Init->SetInfoCalls++;

    if (Index >= m_StreamCount || StreamInfo->StreamId != Index + 1 || StreamInfo->WdfQueue == NULL)
    {
        DPRINT1("Streams %p bad stream info at call %lu, stream ID %lu, queue %p\n",
                m_Handle,
                Index,
                StreamInfo->StreamId,
                StreamInfo->WdfQueue);
        Init->Failed = TRUE;
        UcxVerifierBreak(Verifying);
        return;
    }

    WdfObjectReference(StreamInfo->WdfQueue);

    Entry = Stream(Index);
    Entry->Filled = TRUE;
    Entry->Index = Index;
    Entry->Streams = this;
    Entry->StreamId = StreamInfo->StreamId;
    Entry->Pipe.Queue = StreamInfo->WdfQueue;
    Entry->Pipe.Endpoint = m_Endpoint;
    Entry->Pipe.TransferType = UcxTransferType::Bulk;
    Entry->Pipe.Kind = UcxEndpointKind::Generic;
    Entry->Pipe.BelongsToStream = TRUE;
    Entry->Pipe.DirectionIn = EndpointPipe->DirectionIn;
    Entry->Pipe.MaximumTransferSize = EndpointPipe->MaximumTransferSize;
    Entry->Pipe.Self = &Entry->Pipe;
}

VOID
NTAPI
UcxStaticStreams::EvtDestroy(
    _In_ WDFOBJECT Object)
{
    UcxStaticStreams* Context = UcxGetStaticStreamsContext(Object);
    UcxStream* Entry;
    ULONG Index;

    for (Index = 0; Index < Context->m_StreamCount; Index++)
    {
        Entry = Context->Stream(Index);
        Entry->Pipe.Self = NULL;

        if (Entry->Pipe.Queue != NULL)
        {
            WdfObjectDereference(Entry->Pipe.Queue);
            Entry->Pipe.Queue = NULL;
        }
    }
}

/* Endpoint machine actions */

VOID
EndpointMachine::ReferenceEndpoint()
{
    WdfObjectReference(m_Endpoint->m_Handle);
}

VOID
EndpointMachine::DereferenceEndpoint()
{
    WdfObjectDereference(m_Endpoint->m_Handle);
}

VOID
EndpointMachine::StartEndpoint()
{
    m_Endpoint->m_Callbacks.Start(m_Endpoint->m_Controller->m_Handle, m_Endpoint->m_Handle);
}

/* PurgeDone may be posted from inside the callback */
VOID
EndpointMachine::PurgeEndpoint()
{
    m_Endpoint->m_Callbacks.Purge(m_Endpoint->m_Controller->m_Handle, m_Endpoint->m_Handle);
}

VOID
EndpointMachine::AbortEndpoint()
{
    m_Endpoint->m_Callbacks.Abort(m_Endpoint->m_Controller->m_Handle, m_Endpoint->m_Handle);
}

VOID
EndpointMachine::ForwardResetRequest()
{
    WDFREQUEST Request = (WDFREQUEST)m_Endpoint->TakePending();

    m_Endpoint->m_Callbacks.Reset(m_Endpoint->m_Controller->m_Handle, m_Endpoint->m_Handle, Request);
}

/* The IRP still comes back through the completion routine and EndpointResetDone */
VOID
EndpointMachine::FailResetRequest()
{
    DPRINT1("Endpoint %p reset failed, device is gone or reset\n", m_Endpoint->m_Handle);
    WdfRequestComplete((WDFREQUEST)m_Endpoint->TakePending(), STATUS_NO_SUCH_DEVICE);
}

VOID
EndpointMachine::FinishEndpointResetRequest()
{
    IoCompleteRequest((PIRP)m_Endpoint->TakePending(), IO_NO_INCREMENT);
}

VOID
EndpointMachine::ForwardStreamsEnableRequest()
{
    WDFREQUEST Request = (WDFREQUEST)m_Endpoint->TakePending();

    m_Endpoint->m_Callbacks.StaticStreamsEnable(m_Endpoint->m_Handle, m_Endpoint->m_Streams->m_Handle, Request);
}

/* The open completion deletes the streams and posts StreamsOpened */
VOID
EndpointMachine::RejectStreamsEnableRequest()
{
    DPRINT1("Endpoint %p static streams enable rejected in this state\n", m_Endpoint->m_Handle);
    WdfRequestComplete((WDFREQUEST)m_Endpoint->TakePending(), STATUS_INVALID_DEVICE_STATE);
}

VOID
EndpointMachine::FinishStreamsOpenRequest(
    _In_ BOOLEAN ForceFailure)
{
    PIRP Irp = (PIRP)m_Endpoint->TakePending();

    if (ForceFailure)
    {
        DPRINT1("Endpoint %p static streams open failed by endpoint state\n", m_Endpoint->m_Handle);
        InterlockedDecrement(&m_Endpoint->m_StreamsOpenCount);
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
        Irp->IoStatus.Information = 0;
    }
    else if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        InterlockedDecrement(&m_Endpoint->m_StreamsOpenCount);
    }

    UcxCompleteHeldUrbIrp(Irp);
}

VOID
EndpointMachine::ForwardStreamsDisableRequest()
{
    WDFREQUEST Request = (WDFREQUEST)m_Endpoint->TakePending();

    m_Endpoint->m_Callbacks.StaticStreamsDisable(m_Endpoint->m_Handle, m_Endpoint->m_Streams->m_Handle, Request);
}

/* Completed by the reset holding queue once the reset is over; the forward status is not checked */
VOID
EndpointMachine::ParkStreamsDisableRequest()
{
    WDFREQUEST Request = (WDFREQUEST)m_Endpoint->TakePending();
    NTSTATUS Status;

    Status = WdfRequestForwardToIoQueue(Request, m_Endpoint->m_Controller->m_PendDuringResetQueue);
    if (!NT_SUCCESS(Status))
        DPRINT1("Endpoint %p could not park streams close %p 0x%lx\n", m_Endpoint->m_Handle, Request, Status);
}

/* UCX drops its streams even when the controller driver failed the close */
VOID
EndpointMachine::FinishStreamsCloseRequest()
{
    PIRP Irp = (PIRP)m_Endpoint->TakePending();
    UcxStaticStreams* Streams;

    {
        SpinLockGuard Guard(&m_Endpoint->m_Controller->m_TopologyLock);

        Streams = m_Endpoint->m_Streams;
        m_Endpoint->m_Streams = NULL;
    }

    if (Streams != NULL)
        WdfObjectDelete(Streams->m_Handle);

    InterlockedDecrement(&m_Endpoint->m_StreamsOpenCount);
    UcxCompleteHeldUrbIrp(Irp);
}

/* NULL from the CSQ means the client canceled it meanwhile */
VOID
EndpointMachine::CompleteAbortUrb()
{
    PIRP Irp = IoCsqRemoveIrp(&m_Endpoint->m_Controller->m_AbortPipeCsq, &m_Endpoint->m_AbortCsqContext);
    PURB Urb;

    if (Irp == NULL)
        return;

    Urb = (PURB)IoGetCurrentIrpStackLocation(Irp)->Parameters.Others.Argument1;
    UcxCompleteUrbAtDispatch(Irp, Urb, STATUS_SUCCESS, USBD_STATUS_SUCCESS);
}

VOID
EndpointMachine::CompleteHubOperation()
{
    m_Endpoint->m_Device->CompleteHubOperation();
}

/* The request is completed before it is forgotten, and only then may the next one start */
VOID
EndpointMachine::CompleteTreePurge()
{
    UcxController* Controller = m_Endpoint->m_Controller;

    if (InterlockedDecrement(&Controller->m_PendingTreePurgeEndpoints) != 0)
        return;

    WdfRequestComplete(Controller->m_PendingTreePurge, STATUS_SUCCESS);
    Controller->m_PendingTreePurge = NULL;
    WdfIoQueueStart(Controller->m_TreePurgeQueue);
}

VOID
EndpointMachine::AckControllerReset()
{
    m_Endpoint->m_Controller->PrepareForResetOperationDone();
}

/** Takes the endpoint out of the strict pipe handle list, if it is on it. */
static
VOID
NTAPI
UcxUntrackEndpoint(
    _In_ UcxEndpoint* Endpoint)
{
    UcxUsbDevice* Device = Endpoint->m_Device;

    if (!Device->m_StalePipeIoAllowed)
        return;

    SpinLockGuard Guard(&Device->m_TrackingLock);

    if (UcxIsListEntryLinked(&Endpoint->m_TrackingLink))
    {
        RemoveEntryList(&Endpoint->m_TrackingLink);
        UcxClearListEntry(&Endpoint->m_TrackingLink);
    }
}

/* Delete first, then the creation reference: the parent may already have deleted it */
VOID
EndpointMachine::DeleteEndpoint()
{
    UcxController* Controller = m_Endpoint->m_Controller;
    UCXENDPOINT Handle = m_Endpoint->m_Handle;

    DPRINT("Endpoint %p deleted\n", Handle);

    UcxUntrackEndpoint(m_Endpoint);

    {
        SpinLockGuard Guard(&Controller->m_TopologyLock);

        Controller->m_ChildEndpointCount--;
        RemoveEntryList(&m_Endpoint->m_DeviceLink);
        UcxClearListEntry(&m_Endpoint->m_DeviceLink);
    }

    WdfObjectDelete(Handle);
    WdfObjectDereference(Handle);
}

/** Keeps one stale endpoint per address readable for clients still holding it. */
BOOLEAN
EndpointMachine::ParkAsStale()
{
    UcxUsbDevice* Device = m_Endpoint->m_Device;
    UcxController* Controller = m_Endpoint->m_Controller;
    UcxEndpoint* Replaced = NULL;
    UcxEndpoint* Candidate;
    PLIST_ENTRY Entry;
    BOOLEAN Parked;

    UcxUntrackEndpoint(m_Endpoint);

    {
        SpinLockGuard Guard(&Controller->m_TopologyLock);

        for (Entry = Device->m_StaleEndpointList.Flink; Entry != &Device->m_StaleEndpointList; Entry = Entry->Flink)
        {
            Candidate = CONTAINING_RECORD(Entry, UcxEndpoint, m_DeviceLink);

            if (Candidate->m_Descriptor.bEndpointAddress == m_Endpoint->m_Descriptor.bEndpointAddress)
            {
                RemoveEntryList(Entry);
                UcxClearListEntry(Entry);
                Replaced = Candidate;
                break;
            }
        }

        Controller->m_ChildEndpointCount--;
        RemoveEntryList(&m_Endpoint->m_DeviceLink);
        UcxClearListEntry(&m_Endpoint->m_DeviceLink);
        m_Endpoint->m_Pipe.Self = NULL;

        Parked = !Device->m_PendingDelete;
        if (Parked)
            InsertTailList(&Device->m_StaleEndpointList, &m_Endpoint->m_DeviceLink);
    }

    if (Replaced != NULL)
        Replaced->Post(EpEvent::StaleReplaced);

    DPRINT("Endpoint %p parked as stale %u, replaced %p\n", m_Endpoint->m_Handle, Parked, Replaced);

    return Parked;
}

/* Already off every list */
VOID
EndpointMachine::DeleteStaleEndpoint()
{
    UCXENDPOINT Handle = m_Endpoint->m_Handle;

    DPRINT("Stale endpoint %p deleted\n", Handle);

    WdfObjectDelete(Handle);
    WdfObjectDereference(Handle);
}

BOOLEAN
EndpointMachine::DeviceAllowsStart()
{
    return m_Endpoint->m_Device->AllowsEndpointStart();
}

BOOLEAN
EndpointMachine::DeviceDeprogrammed()
{
    return m_Endpoint->m_Device->m_DeprogrammedByControllerReset;
}

BOOLEAN
EndpointMachine::DeviceDisconnected()
{
    return m_Endpoint->m_Device->m_Disconnected;
}

BOOLEAN
EndpointMachine::ClientHoldsHandle()
{
    return m_Endpoint->m_ClientHoldsHandle;
}
