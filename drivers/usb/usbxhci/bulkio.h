/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Bulk and interrupt transfer ring type
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

class XhciController;
class XhciTransferRing;
struct XhciBulkTransfer;

/** How a bulk or interrupt request reaches the hardware. */
enum class XhciBulkMechanism : UCHAR
{
    NoData,
    Immediate,
    DoubleBuffer,
    Dma
};

/** Cancel handshake state of a request owned by a bulk ring. */
enum class XhciBulkCancel : UCHAR
{
    Idle,
    Armed,
    CancelRoutinePending,
    Done
};

/**
 * One TD on a bulk ring, kept in ring memory. Its address with the endpoint type
 * in bits 1:0 is the Event Data value.
 */
struct DECLSPEC_ALIGN(8) XhciBulkStage
{
    ULONG Signature;
    ULONG Generation;           /**< Bumped per use; upper Event Data dword on 32 bit */
    BOOLEAN InUse;
    BOOLEAN OnHardware;
    BOOLEAN FreeMdl;
    XhciTransferRing* Ring;
    XhciBulkTransfer* Transfer;
    LIST_ENTRY FreeSegments;    /**< Reserved for Link TRBs, not linked yet */
    LIST_ENTRY UsedSegments;    /**< Filled and left through a Link TRB */
    ULONG Size;
    PMDL Mdl;
    PSCATTER_GATHER_LIST SgList;
    PUCHAR Buffer;
    XhciDmaBuffer* DoubleBuffer;
    ULONG BurstTrbBudget;
    ULONG TrbsReserved;
    ULONG TrbsWritten;
    XhciDmaBuffer* FirstSegment;
    ULONG FirstIndex;
    XhciDmaBuffer* LastSegment;
    ULONG EndIndex;             /**< One past the last TRB in LastSegment */
};

/** Per request record of a bulk or interrupt transfer, kept in the request context. */
struct XhciBulkTransfer
{
    LIST_ENTRY Link;
    ULONG Initialized;          /**< Live marker while the ring owns the record */
    BOOLEAN MappedOnce;
    BOOLEAN UsesReservedMdl;
    XhciBulkMechanism Mechanism;
    XhciBulkCancel CancelState;
    WDFREQUEST Request;
    struct _URB_BULK_OR_INTERRUPT_TRANSFER* Urb;
    XhciTransferRing* Ring;
    ULONG CompletionCode;
    NTSTATUS Status;
    PMDL Mdl;
    PUCHAR Buffer;
    XhciDmaBuffer* DoubleBuffer;
    ULONG BytesTotal;
    ULONG BytesTransferred;
    ULONG BytesQueued;
    ULONG TdsRetired;
    ULONG TdsQueued;
    XhciBulkStage* Stage;       /**< At most one stage per request */
};

/** Bulk and interrupt ring state and the entry points the transfer ring calls for it. */
class XhciBulkRing
{
public:
    static NTSTATUS
    Initialize(
        _In_ XhciTransferRing* Ring);
    static VOID
    Enable(
        _In_ XhciTransferRing* Ring);
    static VOID
    Disable(
        _In_ XhciTransferRing* Ring);

    /** Lets a mapping loop parked on a segment wait go, so a disable never waits on it. */
    static VOID
    ReleaseParkedMapping(
        _In_ XhciTransferRing* Ring);

    static NTSTATUS
    EnableForwardProgress(
        _In_ XhciTransferRing* Ring,
        _In_ ULONG MaxTransferSize);
    static VOID
    Cleanup(
        _In_ XhciTransferRing* Ring);

    static EVT_WDF_IO_QUEUE_STATE ReadyNotification;
    static EVT_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE EvtIoCanceledOnQueue;
    static EVT_WDF_DPC EvtCompletionDpc;

    static VOID
    ResumeRingFill(
        _In_ XhciTransferRing* Ring);
    static VOID
    SuspendRingFill(
        _In_ XhciTransferRing* Ring);
    static VOID
    OnPipeHalted(
        _In_ XhciTransferRing* Ring);
    static VOID
    StoppedEventReceived(
        _In_ XhciTransferRing* Ring);
    static VOID
    AllowReclaimOnCancel(
        _In_ XhciTransferRing* Ring);
    static VOID
    ConsumePendingEvents(
        _In_ XhciTransferRing* Ring);
    static VOID
    RecoverTransfers(
        _In_ XhciTransferRing* Ring);
    static BOOLEAN
    DoorbellRungSinceFill(
        _In_ const XhciTransferRing* Ring);
    static BOOLEAN
    HasQueuedWork(
        _In_ const XhciTransferRing* Ring);
    static BOOLEAN
    IsLikelyDuplicate(
        _In_ const XhciTransferRing* Ring,
        _In_ const XHCI_TRB* Event);

    /** The ring is always known; the NULL ring form is kept only for callers without one. */
    static BOOLEAN
    OnTransferEvent(
        _In_opt_ XhciTransferRing* Ring,
        _In_ XhciController* Controller,
        _In_ const XHCI_TRB* Event);

    /** Common buffer callback for asynchronous segment growth. */
    static VOID
    SegmentsArrived(
        _In_ XhciTransferRing* Ring,
        _In_ NTSTATUS Status);

private:
    /** Five stages on the hardware plus the one being prepared. */
    static const ULONG StageSlots = 6;

    enum class Outcome : ULONG
    {
        Ready,
        Completed,
        WaitSegments,
        WaitDma,
        Failed
    };

    static EVT_WDF_REQUEST_CANCEL EvtRequestCancel;
    static DRIVER_LIST_CONTROL DmaCallback;

    static VOID
    BuildTdsFromQueue(
        _In_ XhciTransferRing* Ring);
    static VOID
    FillLoop(
        _In_ XhciTransferRing* Ring);
    static VOID
    KickFill(
        _In_ XhciTransferRing* Ring);
    static XhciBulkStage*
    NextTdSlot(
        _In_ XhciTransferRing* Ring);
    static XhciBulkStage*
    AcquireStage(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciBulkTransfer* Transfer);
    static VOID
    ReleaseStage(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciBulkStage* Stage);
    static VOID
    PutScatterGather(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciBulkStage* Stage);

    static VOID
    InitTransfer(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciBulkTransfer* Transfer,
        _In_ WDFREQUEST Request);
    static VOID
    ChooseMechanism(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciBulkTransfer* Transfer);
    static NTSTATUS
    ArrangeBuffer(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciBulkTransfer* Transfer);
    static VOID
    DropMdl(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciBulkTransfer* Transfer);
    static VOID
    FreeTransferResources(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciBulkTransfer* Transfer);

    static Outcome
    PrimeStage(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciBulkStage* Stage);
    static NTSTATUS
    AcquireStageMdl(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciBulkStage* Stage);
    static VOID
    SizeStage(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciBulkStage* Stage);
    static VOID
    EstimateTrbs(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciBulkStage* Stage);
    static BOOLEAN
    EstimateSegments(
        _In_ XhciTransferRing* Ring,
        _In_ const XhciBulkStage* Stage,
        _Out_ PULONG Segments);
    static BOOLEAN
    ParkOnSegments(
        _In_ XhciTransferRing* Ring);
    static Outcome
    FailStage(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciBulkStage* Stage);
    static Outcome
    EncodeStage(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciBulkStage* Stage);
    static BOOLEAN
    BuildTd(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciBulkStage* Stage);
    static BOOLEAN
    InsertLink(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciBulkStage* Stage,
        _In_ BOOLEAN FirstOfTd);
    static VOID
    UndoTd(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciBulkStage* Stage);

    static VOID
    CompleteAfterUncancel(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciBulkTransfer* Transfer,
        _In_ USBD_STATUS Default,
        _In_ BOOLEAN FinishCanceled);
    static VOID
    Complete(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciBulkTransfer* Transfer,
        _In_ USBD_STATUS Default);
    static VOID
    CompleteBatch(
        _In_ XhciTransferRing* Ring,
        _Inout_ PLIST_ENTRY Batch);
    static VOID
    QueueCompletionDpc(
        _In_ XhciTransferRing* Ring);
    static BOOLEAN
    OwnsRequests(
        _In_ const XhciTransferRing* Ring);

    static BOOLEAN
    Ed1Event(
        _In_ XhciTransferRing* Ring,
        _In_ const XHCI_TRB* Event);
    static BOOLEAN
    Ed0Event(
        _In_ XhciTransferRing* Ring,
        _In_ const XHCI_TRB* Event);
    static BOOLEAN
    IsLiveStage(
        _In_ const XhciTransferRing* Ring,
        _In_ const XhciBulkStage* Stage,
        _In_ ULONG64 EventData);
    static BOOLEAN
    LookupStage(
        _In_ const XhciTransferRing* Ring,
        _In_ ULONG64 Address,
        _Outptr_result_maybenull_ XhciBulkStage** Stage,
        _Out_ PULONG Skipped);
    static VOID
    HaltedEvent(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciBulkStage* Stage,
        _In_ ULONG Code,
        _In_ ULONG Bytes);
    static VOID
    StoppedEvent(
        _In_ XhciTransferRing* Ring,
        _Inout_opt_ XhciBulkStage* Stage,
        _In_ ULONG Code,
        _In_ ULONG Bytes,
        _In_ ULONG Skipped);
    static BOOLEAN
    LikelyDuplicate(
        _In_ const XhciTransferRing* Ring,
        _In_ ULONG64 Address,
        _Out_ PBOOLEAN PointsToNoOp);
    static VOID
    Ed0Mismatch(
        _In_ XhciTransferRing* Ring,
        _In_ const XHCI_TRB* Event);
    static VOID
    StreamStopOrHalt(
        _In_ XhciTransferRing* Ring);

    /* Guarded by the ring lock unless noted */
    BOOLEAN m_ImmediateData;            /**< Set at initialization and forward progress */
    BOOLEAN m_DpcRunning;
    ULONG m_Flags;
    ULONG m_MaxPendingStages;
    ULONG m_PendingStages;
    ULONG m_OutstandingEvents;
    ULONG m_SegmentWaits;               /**< Mapping context only */
    volatile LONG m_FillAgain;    /**< Interlocked DMA callback handoff */
    volatile LONG m_FillWanted;     /**< Interlocked */
    volatile LONG m_SegmentWait;        /**< Interlocked segment callback handoff */
    volatile LONG m_ForwardProgressBusy;
    LIST_ENTRY m_Pending;
    LIST_ENTRY m_WaitingForCancel;
    LIST_ENTRY m_Completion;
    XhciBulkStage m_Stages[StageSlots];
};
