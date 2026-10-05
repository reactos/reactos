/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Endpoint object, its state machine owner and the UCX endpoint callbacks
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

class XhciController;
class XhciUsbDevice;
class XhciTransferRing;
struct XhciStreams;

/** One endpoint. Lives in the UCXENDPOINT context and owns the endpoint machine. */
class XhciEndpoint
{
public:
    static XhciEndpoint*
    FromUcx(
        _In_ UCXENDPOINT Endpoint);

    XhciController* Controller() const;
    XhciUsbDevice* Device() const;
    UCXENDPOINT Handle() const;
    ULONG Dci() const;

    /** USB transfer type, bmAttributes bits 1:0. */
    ULONG TransferType() const;
    const USB_ENDPOINT_DESCRIPTOR* Descriptor() const;
    USHORT MaxPacketSize() const;
    ULONG MaxBurst() const;
    ULONG MaxEsitPayload() const;
    BOOLEAN IsStreamsCapable() const;

    /* Used by the USB device module during Configure Endpoint and Address Device */
    _IRQL_requires_max_(DISPATCH_LEVEL)
    NTSTATUS Enable();
    _IRQL_requires_max_(DISPATCH_LEVEL)
    VOID Disable();

    /** Initializes this endpoint's input endpoint context. */
    VOID
    BuildContext(
        _Out_writes_bytes_(ContextSize) PVOID EndpointContext,
        _In_ ULONG ContextSize) const;

    /* From the USB device */
    VOID
    OnTransferEvent(
        _In_ const XHCI_TRB* Event);
    VOID ControllerResetStarting();
    _IRQL_requires_(PASSIVE_LEVEL)
    VOID ControllerResetDone();
    VOID ControllerRemoved();

    /* Hooks the transfer rings call, at up to DISPATCH_LEVEL */
    VOID OnTransferCanceled();
    VOID
    OnHaltedCompletionCode(
        _In_ ULONG CompletionCode,
        _In_ BOOLEAN TransferFound);
    BOOLEAN ShouldDropStoppedEvent() const;
    VOID OnStoppedEvent();
    VOID OnExpectedEventsProcessed();
    VOID OnMappingStopped();
    VOID OnTransfersReclaimed();

    XhciEndpointMachine m_Machine;

    /** Endpoint disable; SlotDisabling FALSE keeps forward progress resources. */
    _IRQL_requires_max_(DISPATCH_LEVEL)
    VOID
    Disable(
        _In_ BOOLEAN SlotDisabling);

    /* Stream context access for the bulk ring after a stop */
    ULONG64
    StreamDequeuePointer(
        _In_ ULONG StreamId) const;
    ULONG
    StreamTransferLength(
        _In_ ULONG StreamId) const;

    /** Last halted code that matched no stream transfer, 0 when none. */
    ULONG StreamHaltedCode() const;

    /** Endpoint create for both add callbacks. PASSIVE_LEVEL. */
    static NTSTATUS
    Create(
        _In_ UCXCONTROLLER UcxController,
        _In_ UCXUSBDEVICE UcxUsbDevice,
        _In_ PUCXENDPOINT_INIT EndpointInit,
        _In_ const USB_ENDPOINT_DESCRIPTOR* Descriptor,
        _In_opt_ const USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR* Companion,
        _In_opt_ const USB_SUPERSPEEDPLUS_ISOCH_ENDPOINT_COMPANION_DESCRIPTOR* IsochCompanion);

    /* UCX endpoint callbacks */
    static EVT_UCX_ENDPOINT_PURGE EvtPurge;
    static EVT_UCX_ENDPOINT_START EvtStart;
    static EVT_UCX_ENDPOINT_ABORT EvtAbort;
    static EVT_UCX_ENDPOINT_RESET EvtReset;
    static EVT_UCX_ENDPOINT_OK_TO_CANCEL_TRANSFERS EvtOkToCancel;
    static EVT_UCX_DEFAULT_ENDPOINT_UPDATE EvtDefaultUpdate;
    static EVT_UCX_ENDPOINT_STATIC_STREAMS_ADD EvtStreamsAdd;
    static EVT_UCX_ENDPOINT_STATIC_STREAMS_ENABLE EvtStreamsEnable;
    static EVT_UCX_ENDPOINT_STATIC_STREAMS_DISABLE EvtStreamsDisable;

    static NTSTATUS
    NTAPI
    EvtEnableForwardProgress(
        _In_ UCXENDPOINT Endpoint,
        _In_ ULONG MaxTransferSize);

private:
    friend class XhciEndpointMachine;

    struct HaltClearBlock;

    static EVT_WDF_OBJECT_CONTEXT_CLEANUP EvtCleanup;
    static EVT_WDF_OBJECT_CONTEXT_CLEANUP EvtStreamsCleanup;
    static EVT_WDF_TIMER EvtTimer;
    static EVT_WDF_IO_QUEUE_STATE EvtPurgeDone;
    static EVT_WDF_IO_QUEUE_STATE EvtAbortDone;
    static IO_COMPLETION_ROUTINE ClearStallDone;

    /* Command completions */
    static VOID
    NTAPI
    StopDone(
        _In_ XhciCommand* Command);
    static VOID
    NTAPI
    CancelDequeueDone(
        _In_ XhciCommand* Command);
    static VOID
    NTAPI
    ControlResetDone(
        _In_ XhciCommand* Command);
    static VOID
    NTAPI
    ControlDequeueDone(
        _In_ XhciCommand* Command);
    static VOID
    NTAPI
    ClientResetDone(
        _In_ XhciCommand* Command);
    static VOID
    NTAPI
    ClientDequeueDone(
        _In_ XhciCommand* Command);
    static VOID
    NTAPI
    ResetDropDone(
        _In_ XhciCommand* Command);
    static VOID
    NTAPI
    ResetConfigureDone(
        _In_ XhciCommand* Command);
    static VOID
    NTAPI
    StopDropDone(
        _In_ XhciCommand* Command);
    static VOID
    NTAPI
    StopAddDone(
        _In_ XhciCommand* Command);
    static VOID
    NTAPI
    DefaultUpdateDone(
        _In_ XhciCommand* Command);

    /* Single endpoint reconfigure completions of the streams open and close */
    static VOID
    NTAPI
    StreamsEnableDone(
        _In_ PVOID Context,
        _In_ NTSTATUS Status);
    static VOID
    NTAPI
    StreamsDisableDone(
        _In_ PVOID Context,
        _In_ NTSTATUS Status);

    NTSTATUS
    Initialize(
        _In_ XhciController* Controller,
        _In_ UCXUSBDEVICE UcxUsbDevice,
        _In_ UCXENDPOINT Endpoint,
        _In_ const USB_ENDPOINT_DESCRIPTOR* Descriptor,
        _In_opt_ const USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR* Companion,
        _In_opt_ const USB_SUPERSPEEDPLUS_ISOCH_ENDPOINT_COMPANION_DESCRIPTOR* IsochCompanion);
    NTSTATUS CreateClearStall();
    VOID Cleanup();

    VOID
    DisableRings(
        _In_ BOOLEAN SlotDisabling);

    /* The rings an action fans out to: each stream ring, or the main ring */
    BOOLEAN UsesStreams() const;
    ULONG RingCount() const;
    XhciTransferRing*
    RingAt(
        _In_ ULONG Index) const;
    VOID
    ForEachRing(
        _In_ VOID (XhciTransferRing::*Action)());
    BOOLEAN
    PostWhenAllRings(
        _In_ XepEvent Event);
    BOOLEAN AnyTransfersPending() const;

    ULONG64 EndpointDequeuePointer() const;
    PXHCI_STREAM_CONTEXT
    StreamContext(
        _In_ ULONG StreamId) const;
    VOID
    ResetStreamTransferLength(
        _In_ ULONG StreamId);
    BOOLEAN
    StreamRingEmpty(
        _In_ ULONG StreamId) const;
    BOOLEAN StreamCommandDone();

    VOID
    PostEvent(
        _In_ XepEvent Event);
    VOID PostCommandFailure();
    VOID
    FatalError(
        _In_ ULONG Reason);
    ULONG OutputEndpointState() const;
    PENDPOINT_RESET ResetPayload() const;

    VOID
    BuildSetDequeue(
        _Out_ XhciCommand* Command,
        _In_ ULONG64 DequeuePointer,
        _In_ ULONG StreamId,
        _In_ PFN_XHCI_COMMAND_DONE Done);
    VOID
    BuildEndpointConfigure(
        _In_ XhciDmaBuffer* Input,
        _In_opt_ const VOID* EndpointContext,
        _In_ BOOLEAN Drop,
        _In_ BOOLEAN Add,
        _Out_ XhciCommand* Command,
        _In_ PFN_XHCI_COMMAND_DONE Done);
    PXHCI_ENDPOINT_CONTEXT
    InputEndpointContext(
        _In_ XhciDmaBuffer* Input) const;

    VOID
    DequeueAfterStop(
        _In_ ULONG StreamId);
    VOID
    DequeueAfterReset(
        _In_ ULONG StreamId);
    VOID SendClearStall();

    NTSTATUS
    OpenStreams(
        _In_ XhciStreams* Streams,
        _In_ WDFREQUEST Request);
    NTSTATUS
    CloseStreams(
        _In_ XhciStreams* Streams,
        _In_ WDFREQUEST Request);
    VOID
    DisableStreamRings(
        _In_ XhciStreams* Streams);
    /** Frees a pool allocated default stream record and its array. */
    VOID
    FreeDefaultStreams(
        _In_opt_ XhciStreams* Streams);

    XhciController* m_Controller;
    XhciUsbDevice* m_Device;
    UCXUSBDEVICE m_UcxDevice;
    UCXENDPOINT m_Ucx;
    BOOLEAN m_Listed;
    BOOLEAN m_ForwardProgress;
    BOOLEAN m_StreamsCapable;
    volatile LONG m_Flags;
    KEVENT m_ResetAck;
    WDFTIMER m_Timer;
    XhciTransferRing* m_MainRing;

    USB_ENDPOINT_DESCRIPTOR m_Descriptor;
    USB_SUPERSPEED_ENDPOINT_COMPANION_DESCRIPTOR m_Companion;
    USB_SUPERSPEEDPLUS_ISOCH_ENDPOINT_COMPANION_DESCRIPTOR m_IsochCompanion;
    ULONG m_XhciType;
    ULONG m_Dci;

    /* Side outputs of BuildContext, read by the transfer rings */
    mutable ULONG m_MaxBurst;
    mutable ULONG m_MaxPayload;

    XhciStreams* m_DefaultStreams;
    XhciStreams* m_CurrentStreams;

    ULONG m_StopAttempts;
    XhciCommand m_Command;
    XhciDmaBuffer* m_StopInput;
    HaltClearBlock* m_ClearStall;

    WDFREQUEST volatile m_ResetRequest;
    NTSTATUS m_ResetStatus;
};
