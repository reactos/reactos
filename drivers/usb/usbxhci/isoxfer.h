/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Isochronous transfer ring type
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

class XhciController;
class XhciTransferRing;
struct XhciIsochTransfer;

/** Cancel handshake state of a request owned by an isoch ring. */
enum class XhciIsochCancel : UCHAR
{
    Idle,
    Armed,
    CancelRoutinePending,
    Done
};

/** A run of whole frames of one URB, mapped with one scatter gather list. */
struct XhciIsochStage
{
    BOOLEAN InUse;
    BOOLEAN OnHardware;         /**< Counted in the ring's in flight stages */
    BOOLEAN FreeMdl;
    BOOLEAN Truncated;          /**< Ended early because the rest did not fit the reserved segments */
    XhciIsochTransfer* Transfer;
    ULONG FirstPacket;
    ULONG LastPacket;
    ULONG NextPacket;           /**< One past the last packet an event named */
    ULONG Size;
    PMDL Mdl;
    PSCATTER_GATHER_LIST SgList;
    LIST_ENTRY FreeSegments;    /**< Reserved for Link TRBs, not linked yet */
    LIST_ENTRY UsedSegments;    /**< Left through a Link TRB, oldest first */
    XhciDmaBuffer* FirstSegment;
    ULONG FirstIndex;
    XhciDmaBuffer* LastSegment;
    ULONG EndIndex;             /**< One past the last TRB in LastSegment */
    XhciDmaBuffer* ScanSegment; /**< Where the next byte count scan starts */
    ULONG ScanIndex;
};

/** Per request record of an isoch transfer, kept in the request context. */
struct XhciIsochTransfer
{
    static const ULONG StageSlots = 3;

    LIST_ENTRY Link;
    ULONG Initialized;          /**< Live marker while the ring owns the record */
    ULONG Sequence;             /**< Ring wide intake number, tells a reused record apart */
    XhciIsochCancel CancelState;
    BOOLEAN OwnMdl;
    WDFREQUEST Request;
    struct _URB_ISOCH_TRANSFER* Urb;
    XhciTransferRing* Ring;
    NTSTATUS Status;            /**< STATUS_PENDING until the URB is finalized */
    PMDL Mdl;
    ULONG StartFrame;
    ULONG BytesTotal;
    ULONG BytesTransferred;
    ULONG BytesQueued;
    ULONG BytesDropped;
    ULONG PacketsInUrb;
    ULONG PacketsRetired;
    ULONG PacketsQueued;
    ULONG PacketsDropped;
    ULONG TdsRetired;
    ULONG TdsQueued;
    ULONG StageHead;            /**< Oldest used slot of the circular stage queue */
    ULONG StageCount;
    XhciIsochStage Stages[StageSlots];
};

/** Isochronous ring state and the entry points the transfer ring module calls. */
class XhciIsochRing
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

    /** Lets a mapping pass parked on a segment wait go, so a disable never waits on it. */
    static VOID
    ReleaseParkedMapping(
        _In_ XhciTransferRing* Ring);

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
    OnClientPipeReset(
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
    OnTransferEvent(
        _In_ XhciTransferRing* Ring,
        _In_ const XHCI_TRB* Event);

    /** Common buffer callback for asynchronous segment growth. */
    static VOID
    SegmentsArrived(
        _In_ XhciTransferRing* Ring,
        _In_ NTSTATUS Status);

private:
    enum class Outcome : ULONG
    {
        Ready,
        Completed,
        WaitSegments,
        WaitDma,
        Gap,
        Late,
        Failed
    };

    static EVT_WDF_REQUEST_CANCEL EvtRequestCancel;
    static EVT_WDF_TIMER EvtWatchdog;
    static DRIVER_LIST_CONTROL DmaCallback;

    /* Mapping */
    static VOID
    BuildTdsFromQueue(
        _In_ XhciTransferRing* Ring);
    static VOID
    FillLoop(
        _In_ XhciTransferRing* Ring);
    static VOID
    AwaitRingEmpty(
        _In_ XhciTransferRing* Ring);
    static BOOLEAN
    LeaveRingEmptyWait(
        _In_ XhciTransferRing* Ring);
    static VOID
    KickAfterStage(
        _In_ XhciTransferRing* Ring,
        _In_ ULONG InFlight);
    static BOOLEAN
    ParkOnSegments(
        _In_ XhciTransferRing* Ring);

    /* Intake and stage selection */
    static XhciIsochStage*
    NextTdSlot(
        _In_ XhciTransferRing* Ring);
    static BOOLEAN
    TakeRequest(
        _In_ XhciTransferRing* Ring,
        _In_ WDFREQUEST Request,
        _Out_ XhciIsochTransfer** Taken);
    static BOOLEAN
    PacketsValid(
        _In_ const XhciIsochTransfer* Transfer);
    static NTSTATUS
    ScheduleStartFrame(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciIsochTransfer* Transfer);
    static VOID
    FlagMissedPackets(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciIsochTransfer* Transfer);
    static VOID
    SkipUnmapped(
        _Inout_ XhciIsochTransfer* Transfer);
    static XhciIsochStage*
    AcquireStage(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciIsochTransfer* Transfer);
    static VOID
    FreeStage(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciIsochStage* Stage);
    static VOID
    PutScatterGather(
        _In_ XhciTransferRing* Ring,
        _In_opt_ PSCATTER_GATHER_LIST SgList);
    static BOOLEAN
    TransferDone(
        _In_ const XhciIsochTransfer* Transfer);

    /* Stage preparation and ring writing */
    static Outcome
    PrimeStage(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciIsochStage* Stage);
    static NTSTATUS
    AcquireStageMdl(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciIsochStage* Stage);
    static BOOLEAN
    SizeStage(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciIsochStage* Stage);
    static ULONG
    EstimateSegments(
        _In_ XhciTransferRing* Ring,
        _In_ const XhciIsochStage* Stage);
    static Outcome
    FailStage(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciIsochStage* Stage);
    static Outcome
    EncodeStage(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciIsochStage* Stage);
    static VOID
    WriteStage(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciIsochStage* Stage);
    static ULONG
    LayoutStage(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciIsochStage* Stage,
        _In_ BOOLEAN Commit);
    static VOID
    InsertLink(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciIsochStage* Stage,
        _In_ BOOLEAN FirstOfTd);

    /* Completion */
    static VOID
    CompleteTd(
        _In_ XhciTransferRing* Ring,
        _In_ XhciIsochTransfer* Transfer,
        _In_ XhciIsochStage* Stage,
        _In_ ULONG Sequence,
        _In_ ULONG Code,
        _In_ ULONG Bytes,
        _In_ BOOLEAN CloseStage);
    static VOID
    FinishExpiredIsoch(
        _In_ XhciTransferRing* Ring,
        _In_ const XhciIsochTransfer* Newest,
        _In_ ULONG Sequence);
    static VOID
    RetireExpiredStages(
        _In_ XhciTransferRing* Ring,
        _In_ XhciIsochTransfer* Transfer,
        _In_ XhciIsochStage* Stage,
        _In_ ULONG Sequence);
    static ULONG
    ReleaseAllStages(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciIsochTransfer* Transfer);
    static VOID
    Finalize(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciIsochTransfer* Transfer,
        _In_ USBD_STATUS PacketStatus,
        _In_ USBD_STATUS HeaderStatus);
    static VOID
    CompletePlain(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciIsochTransfer* Transfer,
        _In_ USBD_STATUS PacketStatus,
        _In_ USBD_STATUS HeaderStatus,
        _In_ BOOLEAN Async);
    static VOID
    CompleteAfterUncancel(
        _In_ XhciTransferRing* Ring,
        _Inout_ XhciIsochTransfer* Transfer,
        _In_ USBD_STATUS PacketStatus,
        _In_ USBD_STATUS HeaderStatus,
        _In_ BOOLEAN FinishCanceled,
        _In_ BOOLEAN Async);
    static VOID
    CompleteBatch(
        _In_ XhciTransferRing* Ring,
        _Inout_ PLIST_ENTRY Batch);
    static BOOLEAN
    OwnsRequests(
        _In_ const XhciTransferRing* Ring);

    /* Transfer events */
    static VOID
    Ed1Event(
        _In_ XhciTransferRing* Ring,
        _In_ const XHCI_TRB* Event);
    static VOID
    Ed0Event(
        _In_ XhciTransferRing* Ring,
        _In_ const XHCI_TRB* Event);
    static BOOLEAN
    FindTrb(
        _In_ XhciTransferRing* Ring,
        _In_ const XHCI_TRB* Event,
        _Outptr_result_maybenull_ XhciIsochStage** Found,
        _Out_ PULONG Bytes);
    static VOID
    OnRingStarved(
        _In_ XhciTransferRing* Ring);
    static BOOLEAN
    FinishLostStages(
        _In_ XhciTransferRing* Ring);

    /* Guarded by the ring lock unless noted */
    BOOLEAN m_FreshStream;
    BOOLEAN m_DoorbellAfterFirstTd;     /**< Mapping context only */
    BOOLEAN m_DpcRunning;
    ULONG m_NextStartFrame;
    ULONG m_LastScheduledFrame;
    ULONG m_PacketsPerFrame;
    ULONG m_PendingStages;
    ULONG m_TrbsPerTd;
    ULONG m_BufferRetries;
    ULONG m_NextSequence;
    volatile LONG m_Flags;              /**< Interlocked */
    volatile LONG m_FillWanted;     /**< Interlocked */
    volatile LONG m_RingEmptySeen;      /**< Interlocked */
    volatile LONG m_FillAgain;    /**< Interlocked DMA callback handoff */
    volatile LONG m_LateStage;          /**< Interlocked */
    volatile LONG m_SegmentWait;        /**< Interlocked segment callback handoff */
    XhciIsochStage* m_CurrentStage;
    LIST_ENTRY m_Pending;
    LIST_ENTRY m_WaitingForCancel;
    LIST_ENTRY m_Completion;
    WDFTIMER m_Watchdog;

    /* Counters */
    ULONG m_Transfers;
    ULONG m_FailedTransfers;
    ULONG64 m_Bytes;
    ULONG m_Tds;
    ULONG m_TdFailures;
    ULONG m_MissedServiceErrors;
    ULONG m_RingEmptyEvents;
};
