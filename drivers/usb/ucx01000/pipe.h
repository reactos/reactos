/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     UCXENDPOINT and UCXSSTREAMS objects
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

class UcxStaticStreams;

enum class UcxEndpointKind : ULONG
{
    Generic = 0,
    ZeroBandwidth = 1,
    Default = 2
};

enum class UcxTransferType : ULONG
{
    Unknown = 0,
    Control = 1,
    Bulk = 2,
    Isochronous = 3,
    Interrupt = 4
};

/* What a USBD_PIPE_HANDLE points at; Self is cleared once the handle goes stale */
struct UcxPipe
{
    WDFQUEUE Queue;
    UcxEndpoint* Endpoint;
    UcxTransferType TransferType;
    UcxEndpointKind Kind;
    BOOLEAN BelongsToStream;
    BOOLEAN DirectionIn;
    ULONG MaximumTransferSize;
    ULONG IsochPacketLimit;
    ULONG IsochPeriodMicroframes;
    UcxPipe* Self;
    BOOLEAN ReservedIoActive;
    BOOLEAN ForwardProgressAllocatedByHcd;
    ULONG ReservedIoMaxBytes;

    BOOLEAN
    IsValid() const
    {
        return Self == this;
    }

    static
    UcxPipe*
    FromHandle(
        _In_ USBD_PIPE_HANDLE Handle)
    {
        return (UcxPipe*)Handle;
    }

    USBD_PIPE_HANDLE
    Handle()
    {
        return (USBD_PIPE_HANDLE)this;
    }
};

/* The Reserved1 slot of both endpoint callback structures */
typedef
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
(NTAPI *PFN_UCX_HCD_ENDPOINT_ENABLE_FORWARD_PROGRESS)(
    _In_ UCXENDPOINT Endpoint,
    _In_ ULONG MaxTransferSize);

/** Union of the regular and default endpoint callbacks, as kept per endpoint. */
struct UcxEndpointCallbacks
{
    PFN_UCX_ENDPOINT_PURGE Purge;
    PFN_UCX_ENDPOINT_START Start;
    PFN_UCX_ENDPOINT_ABORT Abort;
    PFN_UCX_ENDPOINT_RESET Reset;
    PFN_UCX_ENDPOINT_OK_TO_CANCEL_TRANSFERS OkToCancelTransfers;
    PFN_UCX_ENDPOINT_STATIC_STREAMS_ADD StaticStreamsAdd;
    PFN_UCX_ENDPOINT_STATIC_STREAMS_ENABLE StaticStreamsEnable;
    PFN_UCX_ENDPOINT_STATIC_STREAMS_DISABLE StaticStreamsDisable;
    PFN_UCX_DEFAULT_ENDPOINT_UPDATE DefaultEndpointUpdate;
    PFN_UCX_HCD_ENDPOINT_ENABLE_FORWARD_PROGRESS EnableForwardProgress;
    PFN_UCX_ENDPOINT_GET_ISOCH_TRANSFER_PATH_DELAYS GetIsochTransferPathDelays;
    PFN_UCX_ENDPOINT_SET_CHARACTERISTIC SetCharacteristic;
};

/* Opaque to the HCD; lives on the UCX stack while the HCD's endpoint add runs */
struct _UCXENDPOINT_INIT
{
    UcxEndpointKind Kind;
    UCXUSBDEVICE Device;
    UcxTransferType TransferType;
    USB_ENDPOINT_DESCRIPTOR Descriptor;
    USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR Companion;
    USB_SUPERSPEEDPLUS_ISOCH_ENDPOINT_COMPANION_DESCRIPTOR IsochCompanion;
    BOOLEAN HasCompanion;
    BOOLEAN HasIsochCompanion;
    BOOLEAN CreateMachine;
    UCXENDPOINT Created;
    UcxEndpointCallbacks Callbacks;
};

typedef struct _UCXENDPOINT_INIT UcxEndpointInit;

/* Opaque to the HCD; lives on the UCX stack while the HCD's streams add runs */
struct _UCXSSTREAMS_INIT
{
    UCXENDPOINT Endpoint;
    ULONG StreamCount;
    ULONG SetInfoCalls;
    BOOLEAN Failed;
    UCXSSTREAMS Created;
};

typedef struct _UCXSSTREAMS_INIT UcxStaticStreamsInit;

class UcxEndpoint
{
public:
    /* Exports */

    _Must_inspect_result_
    static
    NTSTATUS
    Create(
        _In_ UCXUSBDEVICE UsbDevice,
        _Inout_ PUCXENDPOINT_INIT* EndpointInit,
        _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
        _Out_ UCXENDPOINT* Endpoint);

    static
    VOID
    InitSetEventCallbacks(
        _Inout_ PUCXENDPOINT_INIT Init,
        _In_ PUCX_ENDPOINT_EVENT_CALLBACKS Callbacks);

    static
    VOID
    InitSetDefaultEventCallbacks(
        _Inout_ PUCXENDPOINT_INIT Init,
        _In_ PUCX_DEFAULT_ENDPOINT_EVENT_CALLBACKS Callbacks);

    VOID
    SetWdfIoQueue(
        _In_ WDFQUEUE Queue);

    VOID
    NeedToCancelTransfers();

    UCXSSTREAMS
    GetStaticStreamsReferenced(
        _In_ PVOID Tag);

    /* Hub interface */

    _Must_inspect_result_
    static
    NTSTATUS
    CreateDefaultFromHub(
        _In_ UCXUSBDEVICE UsbDevice,
        _In_ ULONG MaxPacketSize,
        _Out_ UCXENDPOINT* Endpoint);

    _Must_inspect_result_
    static
    NTSTATUS
    CreateFromHub(
        _In_ UCXUSBDEVICE UsbDevice,
        _In_ PUSB_ENDPOINT_DESCRIPTOR Descriptor,
        _In_ ULONG DescriptorBufferLength,
        _In_opt_ PUSB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR Companion,
        _Out_ UCXENDPOINT* Endpoint);

    VOID
    DeleteFromHub();

    /** TT clear handshake: one decrement from the hub callback, one from the hub's ack. */
    VOID
    ReleaseCancelSync();

    /* Request paths, see pipeio.cpp */

    VOID
    HandleHubReset(
        _In_ WDFREQUEST Request);

    NTSTATUS
    OnHcdResetDone(
        _In_ PIRP Irp);

    NTSTATUS
    OpenStaticStreams(
        _In_ PIRP Irp,
        _In_ PURB Urb);

    NTSTATUS
    CloseStaticStreams(
        _In_ PIRP Irp,
        _In_ PURB Urb);

    VOID
    HandleClientStreamsEnable(
        _In_ WDFREQUEST Request);

    VOID
    HandleClientStreamsDisable(
        _In_ WDFREQUEST Request);

    NTSTATUS
    OpenStaticStreamsComplete(
        _In_ PIRP Irp,
        _In_ PURB Urb);

    NTSTATUS
    CloseStaticStreamsComplete(
        _In_ PIRP Irp);

    NTSTATUS
    AbortPipe(
        _In_ PIRP Irp,
        _In_ PURB Urb);

    NTSTATUS
    GetIsochPathDelays(
        _In_ PIRP Irp,
        _In_ PURB Urb);

    /* Lookups */

    static
    UcxEndpoint*
    FromHandle(
        _In_ UCXENDPOINT Handle);

    static
    UcxEndpoint*
    FromPipe(
        _In_ USBD_PIPE_HANDLE Handle)
    {
        return UcxPipe::FromHandle(Handle)->Endpoint;
    }

    BOOLEAN
    HasMachine() const
    {
        return m_HasMachine;
    }

    /** FALSE only when the abort URB gate turned the event away. */
    BOOLEAN
    Post(
        _In_ EpEvent Event)
    {
        return m_Machine.SmPost(Event);
    }

    /* Taking and putting the one pending request or IRP */

    PVOID
    TakePending()
    {
        PVOID Pending = m_Pending;

        m_Pending = NULL;
        return Pending;
    }

    VOID
    SetPending(
        _In_ PVOID Pending)
    {
        NT_ASSERT(m_Pending == NULL);
        m_Pending = Pending;
    }

public:
    UCXENDPOINT m_Handle;
    UcxController* m_Controller;
    UcxUsbDevice* m_Device;
    UcxPipe m_Pipe;

    UcxStaticStreams* m_Streams;
    LONG m_StreamsOpenCount;

    UcxEndpointCallbacks m_Callbacks;
    USB_ENDPOINT_DESCRIPTOR m_Descriptor;

    /* Device endpoint list, or the device stale list once stale */
    LIST_ENTRY m_DeviceLink;
    LIST_ENTRY m_TrackingLink;

    /* Temporary lists built under the topology lock */
    LIST_ENTRY m_DisconnectLink;
    LIST_ENTRY m_ControllerResetLink;
    LIST_ENTRY m_TreePurgeLink;
    LIST_ENTRY m_OperationLink;

    EndpointMachine m_Machine;
    BOOLEAN m_HasMachine;

    PVOID m_Pending;
    BOOLEAN m_OpenFailedOnReset;

    IO_CSQ_IRP_CONTEXT m_AbortCsqContext;

    /* 0 when idle, 2 while a TT clear is in flight */
    LONG m_CancelSync;

    BOOLEAN m_ClientHoldsHandle;
    BOOLEAN m_ClearTtOnCancel;

private:
    VOID
    Initialize(
        _In_ UcxUsbDevice* Device,
        _In_ const UcxEndpointInit* Init);

    static
    NTSTATUS
    CreateThroughHcd(
        _In_ UcxUsbDevice* Device,
        _Inout_ UcxEndpointInit* Init,
        _In_ ULONG MaxPacketSize,
        _In_opt_ PUSB_ENDPOINT_DESCRIPTOR Descriptor,
        _In_ ULONG DescriptorBufferLength,
        _In_opt_ PUSB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR Companion,
        _Out_ UCXENDPOINT* Endpoint);

    static
    EVT_WDF_OBJECT_CONTEXT_DESTROY EvtDestroy;

    friend class EndpointMachine;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(UcxEndpoint, UcxGetEndpointContext);

/* One static stream of a UCXSSTREAMS object */
struct UcxStream
{
    BOOLEAN Filled;
    ULONG Index;
    UcxStaticStreams* Streams;
    UcxPipe Pipe;
    ULONG StreamId;
};

class UcxStaticStreams
{
public:
    _Must_inspect_result_
    static
    NTSTATUS
    Create(
        _In_ UCXENDPOINT Endpoint,
        _Inout_ PUCXSSTREAMS_INIT* Init,
        _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
        _Out_ UCXSSTREAMS* StaticStreams);

    VOID
    SetStreamInfo(
        _In_ PSTREAM_INFO StreamInfo);

    static
    UcxStaticStreams*
    FromHandle(
        _In_ UCXSSTREAMS Handle);

    UcxStream*
    Stream(
        _In_ ULONG Index)
    {
        return &((UcxStream*)(this + 1))[Index];
    }

public:
    UCXSSTREAMS m_Handle;
    ULONG m_StreamCount;
    UcxEndpoint* m_Endpoint;

    /* Set only while EvtEndpointStaticStreamsAdd runs */
    UcxStaticStreamsInit* m_Init;

private:
    static
    EVT_WDF_OBJECT_CONTEXT_DESTROY EvtDestroy;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(UcxStaticStreams, UcxGetStaticStreamsContext);

/* Maximum transfer sizes and isoch parameters, see pipe.cpp */

ULONG
NTAPI
UcxComputeMaxTransferSize(
    _In_ USB_DEVICE_SPEED Speed,
    _In_ USHORT BcdUsb,
    _In_ const UcxEndpointInit* Init);
