/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Isochronous transfer rings: frame scheduling, staging, TD writing, events and reclaim
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "xhcidrv.h"

#define NDEBUG
#include <debug.h>

static const ULONG XhciIsochTransferTag = 'hcsI';

static const ULONG XhciIsochMaxInFlight = 5;
static const ULONG XhciIsochBufferRetryLimit = 3;
static const ULONG XhciIsochAsapDelay = 5;
static const LONG XhciIsochFrameWindow = 1024;
static const ULONG XhciIsochRestartFast = 1024;
static const ULONG XhciIsochRestartSlow = 255;
static const ULONG XhciIsochInterruptEvery = 0x7F;
static const ULONG XhciIsochWatchdogMs = 10000;
static const ULONG XhciIsochTrbBytesMax = 0x10000;
static const ULONG XhciIsochMaxPackets = 0xFFFF;
static const USBD_STATUS XhciIsochNotSet = (USBD_STATUS)0xFFFFFFFF;

/* Microframes added to MFINDEX before it becomes a frame number */
static const ULONG XhciIsochBiasNow = 1;
static const ULONG XhciIsochBiasLate = 2;

/* Reason code handed to the controller with a fatal error */
static const ULONG XhciIsochReasonBadEventData = 0x101E;

/* m_Flags bits, all cleared when mapping starts */
static const LONG XhciIsochDoorbellRung = 0x01;
static const LONG XhciIsochOkToReclaim = 0x02;
static const LONG XhciIsochCanceled = 0x04;
static const LONG XhciIsochReclaimAck = 0x08;
static const LONG XhciIsochEmptyExpected = 0x10;
static const LONG XhciIsochLastFrameValid = 0x20;
static const LONG XhciIsochStoppedSeen = 0x40;

/* m_SegmentWait values */
static const LONG XhciIsochWaitIdle = 0;
static const LONG XhciIsochWaitParked = 1;
static const LONG XhciIsochWaitArrived = 2;
static const LONG XhciIsochWaitAbandoned = 3;

/* Small helpers that only use public state *********************************/

static
BOOLEAN
NTAPI
XhciIsochStoppedCode(
    _In_ ULONG Code)
{
    return Code == static_cast<ULONG>(XhciCompletionCode::Stopped) ||
           Code == static_cast<ULONG>(XhciCompletionCode::StoppedLengthInvalid) ||
           Code == static_cast<ULONG>(XhciCompletionCode::StoppedShortPacket);
}

static
BOOLEAN
NTAPI
XhciIsochEndpointIn(
    _In_ const XhciTransferRing* Ring)
{
    return (Ring->m_Endpoint->Descriptor()->bEndpointAddress & USB_ENDPOINT_DIRECTION_MASK) != 0;
}

static
ULONG
NTAPI
XhciIsochFrames(
    _In_ ULONG Packets,
    _In_ ULONG PerFrame)
{
    return (Packets + PerFrame - 1) / PerFrame;
}

static
ULONG
NTAPI
XhciIsochPacketLength(
    _In_ const XhciIsochTransfer* Transfer,
    _In_ ULONG Packet)
{
    const struct _URB_ISOCH_TRANSFER* Urb = Transfer->Urb;
    ULONG End;

    End = (Packet + 1 < Transfer->PacketsInUrb) ? Urb->IsoPacket[Packet + 1].Offset : Transfer->BytesTotal;
    return End - Urb->IsoPacket[Packet].Offset;
}

static
XhciIsochStage*
NTAPI
XhciIsochStageAt(
    _In_ XhciIsochTransfer* Transfer,
    _In_ ULONG Position)
{
    return &Transfer->Stages[(Transfer->StageHead + Position) % XhciIsochTransfer::StageSlots];
}

/** Max ESIT payload; a TD never carries more. */
static
ULONG
NTAPI
XhciIsochEsitPayload(
    _In_ const XhciTransferRing* Ring)
{
    ULONG Payload = Ring->m_Endpoint->MaxEsitPayload();

    if (Payload == 0)
        Payload = Ring->m_Endpoint->MaxPacketSize();

    return Payload;
}

/** Full speed device behind a TT on a controller that must not start a TD with a Link TRB. */
static
BOOLEAN
NTAPI
XhciIsochNeedsLinkGuard(
    _In_ const XhciTransferRing* Ring)
{
    const XHCI_SLOT_CONTEXT* Slot;

    if (!Ring->m_Controller->HasErrata(XhciErrata::IsochFsNoBareLinkTd) ||
        Ring->m_Device->Speed() != UsbFullSpeed)
    {
        return FALSE;
    }

    Slot = static_cast<const XHCI_SLOT_CONTEXT*>(Ring->m_Device->OutputContext(0));
    return Slot != NULL && Slot->TTHubSlotId != 0;
}

static
struct _URB_ISOCH_TRANSFER*
NTAPI
XhciIsochUrbOf(
    _In_ WDFREQUEST Request)
{
    WDF_REQUEST_PARAMETERS Parameters;

    WDF_REQUEST_PARAMETERS_INIT(&Parameters);
    WdfRequestGetParameters(Request, &Parameters);
    return static_cast<struct _URB_ISOCH_TRANSFER*>(Parameters.Parameters.Others.Arg1);
}

static
ULONG
NTAPI
XhciIsochCountList(
    _In_ const LIST_ENTRY* Head)
{
    const LIST_ENTRY* Entry;
    ULONG Count = 0;

    for (Entry = Head->Flink; Entry != Head; Entry = Entry->Flink)
        Count++;

    return Count;
}

static
VOID
NTAPI
XhciIsochMoveList(
    _Inout_ PLIST_ENTRY Destination,
    _Inout_ PLIST_ENTRY Source)
{
    while (!IsListEmpty(Source))
        InsertTailList(Destination, RemoveHeadList(Source));
}

/** TRUE when Transfer, with this intake number, is still on the active list. */
static
BOOLEAN
NTAPI
XhciIsochIsActive(
    _In_ const LIST_ENTRY* Pending,
    _In_ const XhciIsochTransfer* Transfer,
    _In_ ULONG Sequence)
{
    const LIST_ENTRY* Entry;

    for (Entry = Pending->Flink; Entry != Pending; Entry = Entry->Flink)
    {
        if (CONTAINING_RECORD(Entry, XhciIsochTransfer, Link) == Transfer)
            return Transfer->Sequence == Sequence;
    }

    return FALSE;
}

/* Scatter gather walking */

static
VOID
NTAPI
XhciIsochSkipEmpty(
    _In_ const SCATTER_GATHER_LIST* SgList,
    _Inout_ PULONG Element,
    _Inout_ PULONG Offset)
{
    while (*Element < SgList->NumberOfElements && *Offset >= SgList->Elements[*Element].Length)
    {
        (*Element)++;
        *Offset = 0;
    }
}

/**
 * Takes the next piece of at most Bytes, ending at an element end or 64 KB boundary (xHCI 6.4.1.1).
 * An exhausted list yields an empty piece at address 0.
 */
static
VOID
NTAPI
XhciIsochTakePiece(
    _In_ const SCATTER_GATHER_LIST* SgList,
    _Inout_ PULONG Element,
    _Inout_ PULONG Offset,
    _In_ ULONG Bytes,
    _Out_ PULONG64 Address,
    _Out_ PULONG Chunk)
{
    ULONG64 Start;
    ULONG Left;
    ULONG Boundary;

    XhciIsochSkipEmpty(SgList, Element, Offset);

    if (*Element >= SgList->NumberOfElements)
    {
        *Address = 0;
        *Chunk = 0;
        return;
    }

    Start = (ULONG64)SgList->Elements[*Element].Address.QuadPart + *Offset;
    Left = SgList->Elements[*Element].Length - *Offset;
    Boundary = XhciIsochTrbBytesMax - (ULONG)(Start & (XhciIsochTrbBytesMax - 1));

    *Address = Start;
    *Chunk = min(min(Bytes, Left), Boundary);
    *Offset += *Chunk;
}

static
ULONG
NTAPI
XhciIsochCountPieces(
    _In_ const SCATTER_GATHER_LIST* SgList,
    _In_ ULONG Element,
    _In_ ULONG Offset,
    _In_ ULONG Bytes)
{
    ULONG64 Address;
    ULONG Chunk;
    ULONG Pieces = 0;

    while (Bytes != 0)
    {
        XhciIsochTakePiece(SgList, &Element, &Offset, Bytes, &Address, &Chunk);
        if (Chunk == 0)
            break;

        Bytes -= Chunk;
        Pieces++;
    }

    return (Pieces == 0) ? 1 : Pieces;
}

static
VOID
NTAPI
XhciIsochAdvance(
    _In_ const SCATTER_GATHER_LIST* SgList,
    _Inout_ PULONG Element,
    _Inout_ PULONG Offset,
    _In_ ULONG Bytes)
{
    while (Bytes != 0)
    {
        ULONG Left;

        XhciIsochSkipEmpty(SgList, Element, Offset);
        if (*Element >= SgList->NumberOfElements)
            return;

        Left = SgList->Elements[*Element].Length - *Offset;
        if (Left > Bytes)
            Left = Bytes;

        *Offset += Left;
        Bytes -= Left;
    }
}

/* Stage TRB ranges */

static
BOOLEAN
NTAPI
XhciIsochSegmentHas(
    _In_ const XhciDmaBuffer* Segment,
    _In_ ULONG Start,
    _In_ ULONG OnePastEnd,
    _In_ ULONG64 Address)
{
    ULONG64 Base = (ULONG64)Segment->LogicalAddress.QuadPart;

    return Address >= Base + (ULONG64)Start * sizeof(XHCI_TRB) &&
           Address < Base + (ULONG64)OnePastEnd * sizeof(XHCI_TRB);
}

static
BOOLEAN
NTAPI
XhciIsochStageHas(
    _In_ const XhciIsochStage* Stage,
    _In_ ULONG64 Address)
{
    const LIST_ENTRY* Entry;
    ULONG Start = Stage->FirstIndex;

    for (Entry = Stage->UsedSegments.Flink; Entry != &Stage->UsedSegments; Entry = Entry->Flink)
    {
        const XhciDmaBuffer* Segment = CONTAINING_RECORD(Entry, XhciDmaBuffer, Link);

        if (XhciIsochSegmentHas(Segment, Start, Segment->Size / sizeof(XHCI_TRB), Address))
            return TRUE;
        Start = 0;
    }

    return XhciIsochSegmentHas(Stage->LastSegment, Start, Stage->EndIndex, Address);
}

/** The segment the stage continues in after Segment, NULL after the last one. */
static
XhciDmaBuffer*
NTAPI
XhciIsochNextSegment(
    _In_ const XhciIsochStage* Stage,
    _In_ const XhciDmaBuffer* Segment)
{
    if (Segment == Stage->LastSegment)
        return NULL;

    if (Segment->Link.Flink == &Stage->UsedSegments)
        return Stage->LastSegment;

    return CONTAINING_RECORD(Segment->Link.Flink, XhciDmaBuffer, Link);
}

static
ULONG
NTAPI
XhciIsochSegmentLimit(
    _In_ const XhciIsochStage* Stage,
    _In_ const XhciDmaBuffer* Segment)
{
    return (Segment == Stage->LastSegment) ? Stage->EndIndex : Segment->Size / sizeof(XHCI_TRB);
}

/** Bytes a TD moved, given the running sum up to the TRB an ED = 0 event names. */
static
ULONG
NTAPI
XhciIsochMatchBytes(
    _In_ const XhciTransferRing* Ring,
    _In_ ULONG Code,
    _In_ ULONG Length,
    _In_ ULONG Residual,
    _In_ ULONG Sum)
{
    switch (static_cast<XhciCompletionCode>(Code))
    {
        case XhciCompletionCode::MissedService:
            if (!Ring->m_Controller->m_Registers.MseLengthUsable())
                return Sum;
            break;

        case XhciCompletionCode::Stopped:
        case XhciCompletionCode::DataBufferError:
        case XhciCompletionCode::BabbleDetected:
        case XhciCompletionCode::IsochBufferOverrun:
        case XhciCompletionCode::StallError:
        case XhciCompletionCode::UsbTransactionError:
        case XhciCompletionCode::SplitTransactionError:
            break;

        case XhciCompletionCode::StoppedShortPacket:
            return Residual;

        case XhciCompletionCode::StoppedLengthInvalid:
        case XhciCompletionCode::NoPingResponse:
            return Sum;

        default:
            DPRINT1("Isoch DCI %lu: code %lu on a data TRB\n", Ring->m_Endpoint->Dci(), Code);
            return Sum;
    }

    /* QUIRK: a residual above the TRB length wraps the count */
    if (Residual > Length)
    {
        DPRINT1("Isoch DCI %lu: residual %lu above TRB length %lu, code %lu\n",
                Ring->m_Endpoint->Dci(), Residual, Length, Code);
    }

    return Sum + Length - Residual;
}

/* Ring type entry points ****************************************************/

NTSTATUS
XhciIsochRing::Initialize(
    _In_ XhciTransferRing* Ring)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    USB_DEVICE_SPEED Speed = Ring->m_Device->Speed();
    ULONG Interval = Ring->m_Endpoint->Descriptor()->bInterval;
    WDF_TIMER_CONFIG TimerConfig;
    WDF_OBJECT_ATTRIBUTES Attributes;
    ULONG Period;
    NTSTATUS Status;

    RtlZeroMemory(Self, sizeof(*Self));
    InitializeListHead(&Self->m_Pending);
    InitializeListHead(&Self->m_WaitingForCancel);
    InitializeListHead(&Self->m_Completion);

    if (Speed >= UsbHighSpeed)
    {
        /* QUIRK: the raw bInterval is used, and 0 behaves as an 8 microframe period */
        Period = (Interval == 0 || Interval > 4) ? 8 : (1UL << (Interval - 1));
        Self->m_PacketsPerFrame = 8 / Period;
    }
    else
    {
        NT_ASSERT(Speed == UsbFullSpeed);
        Self->m_PacketsPerFrame = 1;
    }

    WDF_TIMER_CONFIG_INIT(&TimerConfig, EvtWatchdog);
    TimerConfig.TolerableDelay = 0;
    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Ring->m_Queue;

    Status = WdfTimerCreate(&TimerConfig, &Attributes, &Self->m_Watchdog);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Isoch DCI %lu: ring empty watchdog not created, 0x%08lx\n",
                Ring->m_Endpoint->Dci(), Status);
        return Status;
    }

    DPRINT("Isoch DCI %lu: %lu packets per frame\n", Ring->m_Endpoint->Dci(), Self->m_PacketsPerFrame);
    return STATUS_SUCCESS;
}

VOID
XhciIsochRing::Enable(
    _In_ XhciTransferRing* Ring)
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    Ring->m_Type.Isoch.m_FreshStream = TRUE;
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
}

VOID
XhciIsochRing::Disable(
    _In_ XhciTransferRing* Ring)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    KIRQL OldIrql;

    ReleaseParkedMapping(Ring);
    WdfTimerStop(Self->m_Watchdog, FALSE);

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    if (OwnsRequests(Ring))
        DPRINT1("Isoch DCI %lu disabled while it still owns requests\n", Ring->m_Endpoint->Dci());
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
}

VOID
XhciIsochRing::ReleaseParkedMapping(
    _In_ XhciTransferRing* Ring)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;

    /* A pass parked on a segment wait only resumes from a PASSIVE work item; let it go */
    if (InterlockedCompareExchange(&Self->m_SegmentWait, XhciIsochWaitAbandoned, XhciIsochWaitParked) !=
        XhciIsochWaitParked)
    {
        return;
    }

    DPRINT1("Isoch DCI %lu disabled while waiting for segments\n", Ring->m_Endpoint->Dci());

    if (Ring->ChangeMapState(FALSE, XhciMapState::Halting, XhciMapState::Halted) != XhciMapState::Halting)
    {
        InterlockedCompareExchange(&Ring->m_MapState,
                                   static_cast<LONG>(XhciMapState::PausedFill),
                                   static_cast<LONG>(XhciMapState::Filling));
    }
}

VOID
XhciIsochRing::Cleanup(
    _In_ XhciTransferRing* Ring)
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    if (OwnsRequests(Ring))
    {
        DPRINT1("Isoch DCI %lu cleaned up with requests still owned\n", Ring->m_Endpoint->Dci());
        NT_ASSERT(FALSE);
    }
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
}

VOID
NTAPI
XhciIsochRing::ReadyNotification(
    _In_ WDFQUEUE Queue,
    _In_ WDFCONTEXT Context)
{
    XhciTransferRing* Ring = XhciTransferRing::FromQueue(Queue);

    UNREFERENCED_PARAMETER(Context);

    if (Ring->ChangeMapState(FALSE, XhciMapState::PausedFill, XhciMapState::Filling) == XhciMapState::PausedFill)
    {
        BuildTdsFromQueue(Ring);
        return;
    }

    /* A pass that is about to pause sees this and goes round once more */
    InterlockedExchange(&Ring->m_Type.Isoch.m_FillWanted, 1);
    if (Ring->ChangeMapState(FALSE, XhciMapState::PausedFill, XhciMapState::Filling) == XhciMapState::PausedFill)
        BuildTdsFromQueue(Ring);
}

VOID
NTAPI
XhciIsochRing::EvtIoCanceledOnQueue(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request)
{
    XhciTransferRing* Ring = XhciTransferRing::FromQueue(Queue);
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    XhciIsochTransfer* Transfer = &XhciGetRequestData(Request)->Isoch;
    struct _URB_ISOCH_TRANSFER* Urb = XhciIsochUrbOf(Request);
    ULONG Index;
    KIRQL OldIrql;

    /* QUIRK: the packet statuses are left at the private "not set" value */
    Urb->Hdr.Status = USBD_STATUS_CANCELED;
    Urb->TransferBufferLength = 0;
    Urb->ErrorCount = 0;
    for (Index = 0; Index < Urb->NumberOfPackets && Index < XhciIsochMaxPackets; Index++)
        Urb->IsoPacket[Index].Status = XhciIsochNotSet;

    RtlZeroMemory(Transfer, sizeof(*Transfer));
    Transfer->Initialized = XhciIsochTransferTag;
    Transfer->Request = Request;
    Transfer->Urb = Urb;
    Transfer->Ring = Ring;
    Transfer->CancelState = XhciIsochCancel::Done;
    Transfer->Status = STATUS_CANCELLED;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    InsertTailList(&Self->m_Completion, &Transfer->Link);
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    WdfDpcEnqueue(Ring->m_CompletionDpc);
}

VOID
NTAPI
XhciIsochRing::EvtCompletionDpc(
    _In_ WDFDPC Dpc)
{
    XhciTransferRing* Ring = XhciTransferRing::FromQueue(static_cast<WDFQUEUE>(WdfDpcGetParentObject(Dpc)));
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    LIST_ENTRY Batch;
    BOOLEAN Acknowledge = FALSE;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    if (Self->m_DpcRunning)
    {
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
        return;
    }
    Self->m_DpcRunning = TRUE;

    while (!IsListEmpty(&Self->m_Completion))
    {
        InitializeListHead(&Batch);
        XhciIsochMoveList(&Batch, &Self->m_Completion);
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

        CompleteBatch(Ring, &Batch);

        KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    }

    if ((Self->m_Flags & XhciIsochReclaimAck) && !OwnsRequests(Ring))
    {
        InterlockedAnd(&Self->m_Flags, ~XhciIsochReclaimAck);
        Acknowledge = TRUE;
    }

    Self->m_DpcRunning = FALSE;
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    if (Acknowledge)
        Ring->m_Endpoint->OnTransfersReclaimed();
}

VOID
XhciIsochRing::ResumeRingFill(
    _In_ XhciTransferRing* Ring)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    XhciMapState Previous;
    KIRQL OldIrql;

    InterlockedExchange(&Self->m_Flags, 0);

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    Self->m_PendingStages = 0;
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    Previous = Ring->ChangeMapState(TRUE, XhciMapState::Halted, XhciMapState::Filling);
    if (Previous == XhciMapState::Halted)
    {
        BuildTdsFromQueue(Ring);
        return;
    }

    DPRINT1("Isoch DCI %lu started mapping from map state %ld\n",
            Ring->m_Endpoint->Dci(), static_cast<LONG>(Previous));
}

VOID
XhciIsochRing::SuspendRingFill(
    _In_ XhciTransferRing* Ring)
{
    XhciMapState Previous;

    Previous = Ring->ChangeMapState(TRUE, XhciMapState::Halted, XhciMapState::Halting);

    switch (Previous)
    {
        case XhciMapState::AwaitingStarve:
            WdfTimerStop(Ring->m_Type.Isoch.m_Watchdog, FALSE);
            Ring->ChangeMapState(FALSE, XhciMapState::Halting, XhciMapState::Halted);
            break;

        case XhciMapState::PausedFill:
            Ring->ChangeMapState(FALSE, XhciMapState::Halting, XhciMapState::Halted);
            break;

        case XhciMapState::Filling:
            /* The context that owns mapping acknowledges the stop */
            break;

        default:
            DPRINT1("Isoch DCI %lu stopped mapping from map state %ld\n",
                    Ring->m_Endpoint->Dci(), static_cast<LONG>(Previous));
            break;
    }
}

VOID
XhciIsochRing::OnClientPipeReset(
    _In_ XhciTransferRing* Ring)
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    Ring->m_Type.Isoch.m_FreshStream = TRUE;
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
}

VOID
XhciIsochRing::AllowReclaimOnCancel(
    _In_ XhciTransferRing* Ring)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    BOOLEAN Queue = FALSE;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    if (Self->m_Flags & XhciIsochCanceled)
    {
        XhciIsochMoveList(&Self->m_Completion, &Self->m_Pending);
        Queue = TRUE;
    }
    else
    {
        InterlockedOr(&Self->m_Flags, XhciIsochOkToReclaim);
    }
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    if (Queue)
        WdfDpcEnqueue(Ring->m_CompletionDpc);
}

VOID
XhciIsochRing::ConsumePendingEvents(
    _In_ XhciTransferRing* Ring)
{
    /* Lost Missed Service events mean there is nothing reliable to wait for; late ones are dropped */
    Ring->m_Endpoint->OnExpectedEventsProcessed();
}

VOID
XhciIsochRing::RecoverTransfers(
    _In_ XhciTransferRing* Ring)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    InterlockedOr(&Self->m_Flags, XhciIsochReclaimAck);
    XhciIsochMoveList(&Self->m_Completion, &Self->m_Pending);
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    /* Only the DPC can tell that nothing is owned any more */
    WdfDpcEnqueue(Ring->m_CompletionDpc);
}

BOOLEAN
XhciIsochRing::DoorbellRungSinceFill(
    _In_ const XhciTransferRing* Ring)
{
    return (Ring->m_Type.Isoch.m_Flags & XhciIsochDoorbellRung) != 0;
}

BOOLEAN
XhciIsochRing::HasQueuedWork(
    _In_ const XhciTransferRing* Ring)
{
    PKSPIN_LOCK Lock = const_cast<PKSPIN_LOCK>(&Ring->m_Lock);
    BOOLEAN Pending;
    KIRQL OldIrql;

    KeAcquireSpinLock(Lock, &OldIrql);
    Pending = !IsListEmpty(&Ring->m_Type.Isoch.m_Pending);
    KeReleaseSpinLock(Lock, OldIrql);

    return Pending;
}

BOOLEAN
XhciIsochRing::OnTransferEvent(
    _In_ XhciTransferRing* Ring,
    _In_ const XHCI_TRB* Event)
{
    if (Event->Dword[3] & XHCI_TRANSFER_EVENT_ED)
        Ed1Event(Ring, Event);
    else
        Ed0Event(Ring, Event);

    return TRUE;
}

VOID
XhciIsochRing::SegmentsArrived(
    _In_ XhciTransferRing* Ring,
    _In_ NTSTATUS Status)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;

    if (!NT_SUCCESS(Status))
        DPRINT1("Isoch DCI %lu segment growth failed, 0x%08lx\n", Ring->m_Endpoint->Dci(), Status);

    Self->m_BufferRetries = 0;

    for (;;)
    {
        /* QUIRK: the pass runs even when a stop came in meanwhile, so one more stage can go out */
        if (InterlockedCompareExchange(&Self->m_SegmentWait, XhciIsochWaitIdle, XhciIsochWaitParked) ==
            XhciIsochWaitParked)
        {
            BuildTdsFromQueue(Ring);
            return;
        }

        if (InterlockedCompareExchange(&Self->m_SegmentWait, XhciIsochWaitIdle, XhciIsochWaitAbandoned) ==
            XhciIsochWaitAbandoned)
        {
            return;
        }

        /* The pass has not left yet; it sees this and retries the stage itself */
        if (InterlockedCompareExchange(&Self->m_SegmentWait, XhciIsochWaitArrived, XhciIsochWaitIdle) ==
            XhciIsochWaitIdle)
        {
            return;
        }
    }
}

/* Mapping pass **************************************************************/

VOID
XhciIsochRing::BuildTdsFromQueue(
    _In_ XhciTransferRing* Ring)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    LONG State;
    KIRQL OldIrql;

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    for (;;)
    {
        FillLoop(Ring);

        State = InterlockedCompareExchange(&Ring->m_MapState, 0, 0);

        if (State == static_cast<LONG>(XhciMapState::PausedFill) && Self->m_FillWanted != 0)
        {
            if (Ring->ChangeMapState(FALSE, XhciMapState::PausedFill, XhciMapState::Filling) == XhciMapState::PausedFill)
                continue;
        }
        else if (State == static_cast<LONG>(XhciMapState::AwaitingStarve) && Self->m_RingEmptySeen != 0)
        {
            if (LeaveRingEmptyWait(Ring))
                continue;
        }

        break;
    }

    KeLowerIrql(OldIrql);
}

VOID
XhciIsochRing::FillLoop(
    _In_ XhciTransferRing* Ring)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    XhciIsochStage* Stage;
    Outcome Result;

    for (;;)
    {
        InterlockedExchange(&Self->m_FillWanted, 0);
        InterlockedExchange(&Self->m_RingEmptySeen, 0);

        Stage = NextTdSlot(Ring);
        if (Stage == NULL)
        {
            Ring->ChangeMapState(FALSE, XhciMapState::Filling, XhciMapState::PausedFill);
            return;
        }

        Result = PrimeStage(Ring, Stage);
        if (Result == Outcome::Completed)
            continue;
        if (Result == Outcome::WaitSegments)
            return;
        if (Result == Outcome::Gap)
        {
            AwaitRingEmpty(Ring);
            return;
        }
        if (Result == Outcome::Failed)
        {
            Ring->ChangeMapState(FALSE, XhciMapState::Filling, XhciMapState::PausedFill);
            return;
        }

        Result = EncodeStage(Ring, Stage);
        if (Result == Outcome::WaitDma)
            return;
        if (Result == Outcome::Late)
        {
            AwaitRingEmpty(Ring);
            return;
        }

        /* Also completes a stop that raced in */
        if (Ring->ChangeMapState(FALSE, XhciMapState::Filling, XhciMapState::Filling) != XhciMapState::Filling)
            return;
    }
}

VOID
XhciIsochRing::AwaitRingEmpty(
    _In_ XhciTransferRing* Ring)
{
    if (Ring->ChangeMapState(FALSE, XhciMapState::Filling, XhciMapState::AwaitingStarve) == XhciMapState::Filling)
        WdfTimerStart(Ring->m_Type.Isoch.m_Watchdog, WDF_REL_TIMEOUT_IN_MS(XhciIsochWatchdogMs));
}

BOOLEAN
XhciIsochRing::LeaveRingEmptyWait(
    _In_ XhciTransferRing* Ring)
{
    if (Ring->ChangeMapState(FALSE, XhciMapState::AwaitingStarve, XhciMapState::Filling) !=
        XhciMapState::AwaitingStarve)
    {
        return FALSE;
    }

    WdfTimerStop(Ring->m_Type.Isoch.m_Watchdog, FALSE);
    return TRUE;
}

/** A stage left the ring: map more, or leave the ring empty wait once nothing is in flight. */
VOID
XhciIsochRing::KickAfterStage(
    _In_ XhciTransferRing* Ring,
    _In_ ULONG InFlight)
{
    InterlockedExchange(&Ring->m_Type.Isoch.m_FillWanted, 1);

    if (Ring->ChangeMapState(FALSE, XhciMapState::PausedFill, XhciMapState::Filling) == XhciMapState::PausedFill)
    {
        BuildTdsFromQueue(Ring);
        return;
    }

    if (InFlight == 0 && LeaveRingEmptyWait(Ring))
        BuildTdsFromQueue(Ring);
}

BOOLEAN
XhciIsochRing::ParkOnSegments(
    _In_ XhciTransferRing* Ring)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;

    if (InterlockedCompareExchange(&Self->m_SegmentWait, XhciIsochWaitParked, XhciIsochWaitIdle) ==
        XhciIsochWaitIdle)
    {
        return TRUE;
    }

    InterlockedExchange(&Self->m_SegmentWait, XhciIsochWaitIdle);
    return FALSE;
}

/* Intake and stage selection ************************************************/

XhciIsochStage*
XhciIsochRing::NextTdSlot(
    _In_ XhciTransferRing* Ring)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    XhciIsochTransfer* Transfer;
    XhciIsochStage* Stage = NULL;
    WDFREQUEST Request;
    BOOLEAN Idle;
    NTSTATUS Status;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    Self->m_CurrentStage = NULL;

    if (Self->m_PendingStages >= XhciIsochMaxInFlight)
    {
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
        return NULL;
    }

    if (!IsListEmpty(&Self->m_Pending))
    {
        Transfer = CONTAINING_RECORD(Self->m_Pending.Blink, XhciIsochTransfer, Link);

        if (Self->m_BufferRetries == XhciIsochBufferRetryLimit)
        {
            /* QUIRK: nothing counts the retries up, so this is never reached */
            DPRINT1("Isoch DCI %lu gave up waiting for segments\n", Ring->m_Endpoint->Dci());

            if (Transfer->TdsRetired != Transfer->TdsQueued)
            {
                SkipUnmapped(Transfer);
                KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
                return NULL;
            }

            CompleteAfterUncancel(Ring, Transfer, XhciIsochNotSet, USBD_STATUS_INSUFFICIENT_RESOURCES, TRUE, FALSE);
        }
        else if (Transfer->PacketsQueued + Transfer->PacketsDropped < Transfer->PacketsInUrb)
        {
            FlagMissedPackets(Ring, Transfer);

            if (Transfer->Urb->IsoPacket[Transfer->PacketsInUrb - 1].Status != USBD_STATUS_ISO_NOT_ACCESSED_LATE)
            {
                Stage = AcquireStage(Ring, Transfer);
                Self->m_CurrentStage = Stage;
                KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
                return Stage;
            }

            if (Transfer->TdsRetired != Transfer->TdsQueued)
            {
                SkipUnmapped(Transfer);
                KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
                return NULL;
            }

            /* QUIRK: like the USB 2.0 stack, a URB that ran late reports success, even when canceled */
            CompleteAfterUncancel(Ring, Transfer, USBD_STATUS_SUCCESS, USBD_STATUS_SUCCESS, TRUE, TRUE);
        }
    }

    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    for (;;)
    {
        Status = WdfIoQueueRetrieveNextRequest(Ring->m_Queue, &Request);
        if (!NT_SUCCESS(Status))
        {
            if (Self->m_PendingStages == 0)
                DPRINT("Isoch DCI %lu has nothing queued, the stream may starve\n", Ring->m_Endpoint->Dci());
            return NULL;
        }

        if (TakeRequest(Ring, Request, &Transfer))
            break;
    }

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    Idle = (Self->m_PendingStages == 0);
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    /* Every TD on the ring reports to one event ring so its events stay in order */
    if (Idle)
        Ring->UpdateInterrupterTarget(XhciTransferRing::UrbProcessorIndex((PURB)Transfer->Urb));

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    Stage = AcquireStage(Ring, Transfer);
    InsertTailList(&Self->m_Pending, &Transfer->Link);
    Self->m_CurrentStage = Stage;
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    return Stage;
}

/** Sets up a request taken from the queue; FALSE when it was completed instead. */
BOOLEAN
XhciIsochRing::TakeRequest(
    _In_ XhciTransferRing* Ring,
    _In_ WDFREQUEST Request,
    _Out_ XhciIsochTransfer** Taken)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    XhciRequestData* Data = XhciGetRequestData(Request);
    struct _URB_ISOCH_TRANSFER* Urb;
    XhciIsochTransfer* Transfer;
    NTSTATUS Status;

    *Taken = NULL;

    if (Data == NULL)
    {
        DPRINT1("Isoch request %p has no driver context\n", Request);
        WdfRequestComplete(Request, STATUS_INVALID_DEVICE_STATE);
        return FALSE;
    }

    Urb = XhciIsochUrbOf(Request);
    Transfer = &Data->Isoch;

    /* UCX reuses requests, so the record starts from zero every time */
    RtlZeroMemory(Transfer, sizeof(*Transfer));
    XhciClearListEntry(&Transfer->Link);
    Transfer->Initialized = XhciIsochTransferTag;
    Transfer->Sequence = ++Self->m_NextSequence;
    Transfer->Request = Request;
    Transfer->Urb = Urb;
    Transfer->Ring = Ring;
    Transfer->CancelState = XhciIsochCancel::Idle;
    Transfer->Status = STATUS_PENDING;
    Transfer->BytesTotal = Urb->TransferBufferLength;
    Transfer->PacketsInUrb = (Urb->NumberOfPackets <= XhciIsochMaxPackets) ? Urb->NumberOfPackets : 0;

    if (Urb->TransferBufferMDL != NULL)
    {
        Transfer->Mdl = Urb->TransferBufferMDL;
    }
    else if (Transfer->BytesTotal != 0)
    {
        if (Urb->TransferBuffer != NULL)
            Transfer->Mdl = IoAllocateMdl(Urb->TransferBuffer, Transfer->BytesTotal, FALSE, FALSE, NULL);

        if (Transfer->Mdl == NULL)
        {
            DPRINT1("Isoch DCI %lu request %p: no MDL for a %lu byte buffer\n",
                    Ring->m_Endpoint->Dci(), Request, Transfer->BytesTotal);

            /* QUIRK: the caller's packet statuses and error count are kept */
            CompletePlain(Ring, Transfer, XhciIsochNotSet, USBD_STATUS_INSUFFICIENT_RESOURCES, FALSE);
            return FALSE;
        }

        Transfer->OwnMdl = TRUE;
        MmBuildMdlForNonPagedPool(Transfer->Mdl);
    }

    /* Packet offsets must lie inside the buffer before the MDL chain is walked */
    if (!PacketsValid(Transfer))
    {
        DPRINT1("Isoch DCI %lu request %p: %lu packets with bad offsets for %lu bytes\n",
                Ring->m_Endpoint->Dci(), Request, Urb->NumberOfPackets, Transfer->BytesTotal);
        Urb->ErrorCount = 0;
        CompletePlain(Ring, Transfer, USBD_STATUS_ISO_NOT_ACCESSED_BY_HW, USBD_STATUS_INVALID_PARAMETER, FALSE);
        return FALSE;
    }

    Status = ScheduleStartFrame(Ring, Transfer);
    if (!NT_SUCCESS(Status))
    {
        CompletePlain(Ring, Transfer, USBD_STATUS_ISO_NOT_ACCESSED_LATE, USBD_STATUS_BAD_START_FRAME, FALSE);
        return FALSE;
    }

    FlagMissedPackets(Ring, Transfer);
    if (Urb->IsoPacket[Transfer->PacketsInUrb - 1].Status == USBD_STATUS_ISO_NOT_ACCESSED_LATE)
    {
        DPRINT("Isoch DCI %lu request %p is late in full\n", Ring->m_Endpoint->Dci(), Request);
        CompletePlain(Ring, Transfer, USBD_STATUS_SUCCESS, USBD_STATUS_SUCCESS, TRUE);
        return FALSE;
    }

    if (Transfer->BytesTotal == 0)
    {
        DPRINT("Isoch DCI %lu request %p has no data\n", Ring->m_Endpoint->Dci(), Request);
        CompletePlain(Ring, Transfer, XhciIsochNotSet, XhciIsochNotSet, TRUE);
        return FALSE;
    }

    Transfer->CancelState = XhciIsochCancel::Armed;
    Status = WdfRequestMarkCancelableEx(Request, EvtRequestCancel);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Isoch DCI %lu request %p canceled before mapping, 0x%08lx\n",
                Ring->m_Endpoint->Dci(), Request, Status);
        Transfer->CancelState = XhciIsochCancel::Done;
        CompletePlain(Ring, Transfer, XhciIsochNotSet, USBD_STATUS_CANCELED, FALSE);
        return FALSE;
    }

    *Taken = Transfer;
    return TRUE;
}

BOOLEAN
XhciIsochRing::PacketsValid(
    _In_ const XhciIsochTransfer* Transfer)
{
    const struct _URB_ISOCH_TRANSFER* Urb = Transfer->Urb;
    ULONG Index;

    if (Transfer->PacketsInUrb == 0)
        return FALSE;

    for (Index = 0; Index + 1 < Transfer->PacketsInUrb; Index++)
    {
        if (Urb->IsoPacket[Index].Offset > Urb->IsoPacket[Index + 1].Offset)
            return FALSE;
    }

    return Urb->IsoPacket[Transfer->PacketsInUrb - 1].Offset <= Transfer->BytesTotal;
}

NTSTATUS
XhciIsochRing::ScheduleStartFrame(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciIsochTransfer* Transfer)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    struct _URB_ISOCH_TRANSFER* Urb = Transfer->Urb;
    ULONG Threshold;
    ULONG StartFrame;
    ULONG Now = 0;
    ULONG Index;
    LONG Distance;
    KIRQL OldIrql;

    Threshold = (Ring->m_Device->Speed() >= UsbHighSpeed) ? XhciIsochRestartFast : XhciIsochRestartSlow;

    if (Urb->TransferFlags & USBD_START_ISO_TRANSFER_ASAP)
        Now = Ring->m_Controller->GetFrameNumber(XhciIsochBiasNow);

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);

    if (Urb->TransferFlags & USBD_START_ISO_TRANSFER_ASAP)
    {
        StartFrame = Self->m_NextStartFrame;

        /* QUIRK: unsigned and without wrap handling, and the threshold is really a packet count */
        if (Self->m_FreshStream || (Now > StartFrame && Now - StartFrame > Threshold))
        {
            StartFrame = Now + XhciIsochAsapDelay;
            Self->m_DoorbellAfterFirstTd = TRUE;
        }

        Urb->StartFrame = StartFrame;
    }
    else
    {
        StartFrame = Urb->StartFrame;
    }

    Self->m_NextStartFrame = StartFrame + XhciIsochFrames(Transfer->PacketsInUrb, Self->m_PacketsPerFrame);
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    Transfer->StartFrame = StartFrame;
    Urb->TransferBufferLength = 0;
    Urb->ErrorCount = 0;
    for (Index = 0; Index < Transfer->PacketsInUrb; Index++)
        Urb->IsoPacket[Index].Status = XhciIsochNotSet;

    Distance = (LONG)(StartFrame - Ring->m_Controller->GetFrameNumber(XhciIsochBiasLate));
    if (Distance > XhciIsochFrameWindow || Distance < -XhciIsochFrameWindow)
    {
        DPRINT1("Isoch DCI %lu: start frame 0x%lx is %ld frames away\n",
                Ring->m_Endpoint->Dci(), StartFrame, Distance);
        return STATUS_UNSUCCESSFUL;
    }

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    Self->m_FreshStream = FALSE;
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    return STATUS_SUCCESS;
}

/** Marks the unmapped frames that are already due as late. */
VOID
XhciIsochRing::FlagMissedPackets(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciIsochTransfer* Transfer)
{
    ULONG PerFrame = Ring->m_Type.Isoch.m_PacketsPerFrame;
    ULONG First = Transfer->PacketsQueued + Transfer->PacketsDropped;
    ULONG Frame = Transfer->StartFrame + XhciIsochFrames(First, PerFrame);
    ULONG End = Frame + XhciIsochFrames(Transfer->PacketsInUrb - First, PerFrame);
    ULONG Now = Ring->m_Controller->GetFrameNumber(XhciIsochBiasLate);
    ULONG Packet;
    ULONG Index;

    /* QUIRK: unsigned compares without wrap handling */
    while (Frame < End && Frame <= Now)
    {
        Packet = (Frame - Transfer->StartFrame) * PerFrame;

        for (Index = 0; Index < PerFrame && Packet + Index < Transfer->PacketsInUrb; Index++)
        {
            Transfer->Urb->IsoPacket[Packet + Index].Status = USBD_STATUS_ISO_NOT_ACCESSED_LATE;
            Transfer->BytesDropped += XhciIsochPacketLength(Transfer, Packet + Index);
            Transfer->PacketsDropped++;
        }

        Frame++;
    }
}

VOID
XhciIsochRing::SkipUnmapped(
    _Inout_ XhciIsochTransfer* Transfer)
{
    Transfer->PacketsDropped = Transfer->PacketsInUrb - Transfer->PacketsQueued;
    Transfer->BytesDropped = (Transfer->BytesTotal > Transfer->BytesQueued) ?
                             Transfer->BytesTotal - Transfer->BytesQueued : 0;
}

_Requires_lock_held_(Ring->m_Lock)
XhciIsochStage*
XhciIsochRing::AcquireStage(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciIsochTransfer* Transfer)
{
    XhciIsochStage* Stage;

    if (Transfer->StageCount == XhciIsochTransfer::StageSlots)
        return NULL;

    Stage = XhciIsochStageAt(Transfer, Transfer->StageCount);
    Transfer->StageCount++;

    RtlZeroMemory(Stage, sizeof(*Stage));
    Stage->InUse = TRUE;
    Stage->Transfer = Transfer;
    Stage->FirstPacket = 0xFFFFFFFE;
    Stage->LastPacket = 0xFFFFFFFE;
    Stage->NextPacket = 1;
    InitializeListHead(&Stage->FreeSegments);
    InitializeListHead(&Stage->UsedSegments);
    Stage->FirstSegment = Ring->m_Segment;
    Stage->LastSegment = Ring->m_Segment;
    Stage->ScanSegment = Ring->m_Segment;
    Stage->FirstIndex = Ring->m_EnqueueIndex;
    Stage->EndIndex = Ring->m_EnqueueIndex;
    Stage->ScanIndex = Ring->m_EnqueueIndex;

    return Stage;
}

/** Gives back a stage's MDL and segments and frees its slot. Its list was returned already. */
_Requires_lock_held_(Ring->m_Lock)
VOID
XhciIsochRing::FreeStage(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciIsochStage* Stage)
{
    XhciIsochTransfer* Transfer = Stage->Transfer;

    NT_ASSERT(Stage->SgList == NULL);

    if (Stage->FreeMdl)
        IoFreeMdl(Stage->Mdl);
    Stage->Mdl = NULL;
    Stage->FreeMdl = FALSE;

    Ring->ReturnSegments(&Stage->FreeSegments, TRUE);
    Ring->ReturnSegments(&Stage->UsedSegments, FALSE);
    InitializeListHead(&Stage->FreeSegments);
    InitializeListHead(&Stage->UsedSegments);

    if (Ring->m_Type.Isoch.m_CurrentStage == Stage)
        Ring->m_Type.Isoch.m_CurrentStage = NULL;

    /* Only the oldest or the newest stage of a transfer ever goes */
    if (Stage == XhciIsochStageAt(Transfer, 0))
    {
        Transfer->StageHead = (Transfer->StageHead + 1) % XhciIsochTransfer::StageSlots;
    }
    else
    {
        NT_ASSERT(Stage == XhciIsochStageAt(Transfer, Transfer->StageCount - 1));
    }
    Transfer->StageCount--;

    Stage->InUse = FALSE;
    Stage->OnHardware = FALSE;
}

_Requires_lock_not_held_(Ring->m_Lock)
VOID
XhciIsochRing::PutScatterGather(
    _In_ XhciTransferRing* Ring,
    _In_opt_ PSCATTER_GATHER_LIST SgList)
{
    PDMA_ADAPTER Adapter;
    KIRQL OldIrql;

    if (SgList == NULL)
        return;

    /* Never under the ring lock: the HAL may run another DMA callback of this ring */
    Adapter = Ring->m_Controller->m_Buffers.DmaAdapter();
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    Adapter->DmaOperations->PutScatterGatherList(Adapter, SgList, !XhciIsochEndpointIn(Ring));
    KeLowerIrql(OldIrql);
}

/** Every stage is back and every packet is mapped or skipped. */
BOOLEAN
XhciIsochRing::TransferDone(
    _In_ const XhciIsochTransfer* Transfer)
{
    return Transfer->StageCount == 0 &&
           Transfer->TdsRetired == Transfer->TdsQueued &&
           Transfer->PacketsQueued + Transfer->PacketsDropped >= Transfer->PacketsInUrb;
}

/* Stage preparation *********************************************************/

XhciIsochRing::Outcome
XhciIsochRing::PrimeStage(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciIsochStage* Stage)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    XhciIsochTransfer* Transfer = Stage->Transfer;
    ULONG Frame;
    ULONG Segments;
    BOOLEAN Gap = FALSE;
    NTSTATUS Status;
    KIRQL OldIrql;

    Stage->FirstPacket = Transfer->PacketsQueued + Transfer->PacketsDropped;
    Frame = Transfer->StartFrame + XhciIsochFrames(Stage->FirstPacket, Self->m_PacketsPerFrame);

    /* Without contiguous Frame IDs the controller must run dry before a jump (xHCI 4.11.2.5) */
    if (!Ring->m_Controller->m_Registers.ContiguousFrameId())
    {
        KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
        if ((Self->m_Flags & XhciIsochLastFrameValid) &&
            Frame != Self->m_LastScheduledFrame + 1 &&
            ((Self->m_Flags & XhciIsochEmptyExpected) || Self->m_PendingStages != 0))
        {
            FreeStage(Ring, Stage);
            Gap = TRUE;
        }
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

        if (Gap)
        {
            DPRINT("Isoch DCI %lu: frame 0x%lx does not follow 0x%lx, waiting for the ring to drain\n",
                   Ring->m_Endpoint->Dci(), Frame, Self->m_LastScheduledFrame);
            return Outcome::Gap;
        }
    }

    if (!NT_SUCCESS(AcquireStageMdl(Ring, Stage)))
        return FailStage(Ring, Stage);

    if (!SizeStage(Ring, Stage))
        return FailStage(Ring, Stage);

    Segments = EstimateSegments(Ring, Stage);
    if (Segments == 0)
        return Outcome::Ready;

    Status = Ring->TakeSegments(Segments, &Stage->FreeSegments);
    if (Status == STATUS_PENDING)
    {
        KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
        FreeStage(Ring, Stage);
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

        if (ParkOnSegments(Ring))
            return Outcome::WaitSegments;

        /* The segments already arrived; take the stage again from the top */
        return Outcome::Completed;
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Isoch DCI %lu: %lu segments not available, 0x%08lx\n",
                Ring->m_Endpoint->Dci(), Segments, Status);
        return FailStage(Ring, Stage);
    }

    Self->m_BufferRetries = 0;
    return Outcome::Ready;
}

NTSTATUS
XhciIsochRing::AcquireStageMdl(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciIsochStage* Stage)
{
    XhciIsochTransfer* Transfer = Stage->Transfer;
    PMDL Mdl = Transfer->Mdl;
    ULONG Offset = Transfer->BytesQueued + Transfer->BytesDropped;
    PUCHAR Start;
    ULONG Length;
    PMDL Partial;

    while (Mdl != NULL && Offset >= MmGetMdlByteCount(Mdl))
    {
        Offset -= MmGetMdlByteCount(Mdl);
        Mdl = Mdl->Next;
    }

    if (Mdl == NULL)
    {
        DPRINT1("Isoch DCI %lu: MDL chain shorter than the transfer\n", Ring->m_Endpoint->Dci());
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (Offset == 0)
    {
        Stage->Mdl = Mdl;
        return STATUS_SUCCESS;
    }

    Start = static_cast<PUCHAR>(MmGetMdlVirtualAddress(Mdl)) + Offset;
    Length = MmGetMdlByteCount(Mdl) - Offset;

    Partial = IoAllocateMdl(Start, Length, FALSE, FALSE, NULL);
    if (Partial == NULL)
    {
        DPRINT1("Isoch DCI %lu: partial MDL allocation failed\n", Ring->m_Endpoint->Dci());
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IoBuildPartialMdl(Mdl, Partial, Start, Length);
    Partial->Next = Mdl->Next;
    Stage->Mdl = Partial;
    Stage->FreeMdl = TRUE;
    return STATUS_SUCCESS;
}

/** Picks the last packet and byte count of a stage; FALSE when not even one frame fits. */
BOOLEAN
XhciIsochRing::SizeStage(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciIsochStage* Stage)
{
    XhciIsochTransfer* Transfer = Stage->Transfer;
    ULONG PerFrame = Ring->m_Type.Isoch.m_PacketsPerFrame;
    ULONG Used = Transfer->BytesQueued + Transfer->BytesDropped;
    ULONG Remaining = (Transfer->BytesTotal > Used) ? Transfer->BytesTotal - Used : 0;
    ULONG Limit = Ring->m_MaxStageSize;
    ULONG Registers = 0;
    PDMA_ADAPTER Adapter;
    DMA_TRANSFER_INFO Info;
    ULONG Packet;
    ULONG Total = 0;
    PMDL Mdl;
    ULONG Left;

    Adapter = Ring->m_Controller->m_Buffers.DmaAdapter();
    if (Adapter->Version >= DEVICE_DESCRIPTION_VERSION3 &&
        Adapter->DmaOperations->Size > FIELD_OFFSET(DMA_OPERATIONS, GetDmaTransferInfo) &&
        Adapter->DmaOperations->GetDmaTransferInfo != NULL)
    {
        RtlZeroMemory(&Info, sizeof(Info));
        Info.Version = DMA_TRANSFER_INFO_VERSION1;
        if (NT_SUCCESS(Adapter->DmaOperations->GetDmaTransferInfo(Adapter,
                                                                  Stage->Mdl,
                                                                  0,
                                                                  Remaining,
                                                                  !XhciIsochEndpointIn(Ring),
                                                                  &Info)))
        {
            Registers = Info.V1.MapRegisterCount;
        }
    }

    if (Registers == 0)
    {
        /* Without the HAL query count the pages the chain spans */
        for (Mdl = Stage->Mdl, Left = Remaining; Mdl != NULL && Left != 0; Mdl = Mdl->Next)
        {
            ULONG Chunk = min(Left, MmGetMdlByteCount(Mdl));

            Registers += ADDRESS_AND_SIZE_TO_SPAN_PAGES(MmGetMdlVirtualAddress(Mdl), Chunk);
            Left -= Chunk;
        }
    }

    if (Registers > Ring->m_MapRegisterCount && Ring->m_MapRegisterCount > 1)
        Limit = min(Limit, (Ring->m_MapRegisterCount - 1) * PAGE_SIZE);

    if (Remaining <= Limit)
    {
        Stage->LastPacket = Transfer->PacketsInUrb - 1;
        Stage->Size = Remaining;
        return TRUE;
    }

    /* Whole frames only */
    Packet = Stage->FirstPacket;
    while (Packet < Transfer->PacketsInUrb)
    {
        ULONG FrameBytes = 0;
        ULONG Count;

        for (Count = 0; Count < PerFrame && Packet + Count < Transfer->PacketsInUrb; Count++)
            FrameBytes += XhciIsochPacketLength(Transfer, Packet + Count);

        if (Total + FrameBytes > Limit)
            break;

        Total += FrameBytes;
        Packet += Count;
    }

    /* Fail instead of building an empty stage when one frame exceeds the limit */
    if (Packet == Stage->FirstPacket)
    {
        DPRINT1("Isoch DCI %lu: one frame needs more than %lu bytes of mapping\n",
                Ring->m_Endpoint->Dci(), Limit);
        return FALSE;
    }

    Stage->LastPacket = Packet - 1;
    Stage->Size = Total;
    return TRUE;
}

ULONG
XhciIsochRing::EstimateSegments(
    _In_ XhciTransferRing* Ring,
    _In_ const XhciIsochStage* Stage)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    BOOLEAN Guard = XhciIsochNeedsLinkGuard(Ring);
    ULONG PerTd;
    ULONG Needed;
    ULONG Available;
    ULONG Usable;
    ULONG Segments = 0;

    PerTd = 1 + ((XhciIsochEsitPayload(Ring) + 2 * PAGE_SIZE - 2) >> PAGE_SHIFT);
    if (Guard)
        PerTd += 1;
    if (MmGetMdlByteCount(Stage->Mdl) < Stage->Size)
        PerTd += 2;

    /* A TD never needs more than one segment */
    if (PerTd > Ring->m_LastIndex)
        PerTd = Ring->m_LastIndex;

    Self->m_TrbsPerTd = PerTd;

    Needed = PerTd * (Stage->LastPacket - Stage->FirstPacket + 1);
    Available = Ring->m_LastIndex - Ring->m_EnqueueIndex;
    Available -= Available % PerTd;

    if (Needed > Available)
    {
        Usable = Ring->m_LastIndex - (Ring->m_LastIndex % PerTd);
        Segments = (Needed - Available + Usable - 1) / Usable;
    }

    if (Guard)
        Segments++;

    return Segments;
}

/** Drops a stage that cannot be mapped; completes its request when nothing of it is on the ring. */
XhciIsochRing::Outcome
XhciIsochRing::FailStage(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciIsochStage* Stage)
{
    XhciIsochTransfer* Transfer = Stage->Transfer;
    PSCATTER_GATHER_LIST SgList = Stage->SgList;
    Outcome Result;
    KIRQL OldIrql;

    Stage->SgList = NULL;
    PutScatterGather(Ring, SgList);

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    FreeStage(Ring, Stage);

    if (Transfer->TdsRetired == Transfer->TdsQueued)
    {
        CompleteAfterUncancel(Ring, Transfer, XhciIsochNotSet, USBD_STATUS_INSUFFICIENT_RESOURCES, TRUE, FALSE);
        Result = Outcome::Completed;
    }
    else
    {
        /* The request completes when its last mapped stage comes back */
        SkipUnmapped(Transfer);
        Result = Outcome::Failed;
    }

    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
    return Result;
}

XhciIsochRing::Outcome
XhciIsochRing::EncodeStage(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciIsochStage* Stage)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    PDMA_ADAPTER Adapter = Ring->m_Controller->m_Buffers.DmaAdapter();
    NTSTATUS Status;
    KIRQL OldIrql;

    InterlockedExchange(&Self->m_LateStage, 0);

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    Status = Adapter->DmaOperations->GetScatterGatherList(Adapter,
                                                          WdfDeviceWdmGetDeviceObject(Ring->m_Controller->m_Device),
                                                          Stage->Mdl,
                                                          MmGetMdlVirtualAddress(Stage->Mdl),
                                                          Stage->Size,
                                                          DmaCallback,
                                                          Stage,
                                                          !XhciIsochEndpointIn(Ring));
    KeLowerIrql(OldIrql);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Isoch DCI %lu: GetScatterGatherList for %lu bytes failed, 0x%08lx\n",
                Ring->m_Endpoint->Dci(), Stage->Size, Status);
        (VOID)FailStage(Ring, Stage);
        return Outcome::Ready;
    }

    /* Whichever of this context and the callback gets here second keeps mapping */
    if (InterlockedXor(&Self->m_FillAgain, 1) == 1)
        return (Self->m_LateStage != 0) ? Outcome::Late : Outcome::Ready;

    return Outcome::WaitDma;
}

VOID
NTAPI
XhciIsochRing::DmaCallback(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PSCATTER_GATHER_LIST ScatterGather,
    _In_ PVOID Context)
{
    XhciIsochStage* Stage = static_cast<XhciIsochStage*>(Context);
    XhciTransferRing* Ring = Stage->Transfer->Ring;
    XhciIsochRing* Self = &Ring->m_Type.Isoch;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    Stage->SgList = ScatterGather;
    WriteStage(Ring, Stage);

    if (InterlockedXor(&Self->m_FillAgain, 1) != 1)
        return;

    if (Self->m_LateStage != 0)
    {
        AwaitRingEmpty(Ring);
        return;
    }

    if (Ring->ChangeMapState(FALSE, XhciMapState::Filling, XhciMapState::Filling) == XhciMapState::Filling)
        BuildTdsFromQueue(Ring);
}

/* Ring writing **************************************************************/

/** Writes a mapped stage into the ring, or drops it when it is late or nothing fits. */
VOID
XhciIsochRing::WriteStage(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciIsochStage* Stage)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    XhciIsochTransfer* Transfer = Stage->Transfer;
    PSCATTER_GATHER_LIST SgList;
    ULONG Frame;
    ULONG Now;
    ULONG Fit;
    ULONG Packet;
    ULONG Size = 0;
    KIRQL OldIrql;

    if (Ring->m_Controller->HasErrata(XhciErrata::IsochSkipPastFrames))
    {
        Frame = Transfer->StartFrame + Stage->FirstPacket / Self->m_PacketsPerFrame;
        Now = Ring->m_Controller->GetFrameNumber(XhciIsochBiasLate);

        if ((LONG)(Now - Frame) >= 0)
        {
            DPRINT1("Isoch DCI %lu: stage for frame 0x%lx is late at 0x%lx\n",
                    Ring->m_Endpoint->Dci(), Frame, Now);

            SgList = Stage->SgList;
            Stage->SgList = NULL;
            PutScatterGather(Ring, SgList);

            KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
            FreeStage(Ring, Stage);
            InterlockedOr(&Self->m_Flags, XhciIsochEmptyExpected);
            KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

            /* The controller runs off the end and reports the ring empty */
            Ring->RingDoorbell();
            InterlockedExchange(&Self->m_LateStage, 1);
            return;
        }
    }

    Fit = LayoutStage(Ring, Stage, FALSE);
    if (Fit == 0)
    {
        DPRINT1("Isoch DCI %lu: packet %lu does not fit the reserved segments\n",
                Ring->m_Endpoint->Dci(), Stage->FirstPacket);
        (VOID)FailStage(Ring, Stage);
        return;
    }

    /* End the stage at the last TD that fits in the segment */
    if (Stage->FirstPacket + Fit - 1 != Stage->LastPacket)
    {
        DPRINT1("Isoch DCI %lu: stage cut after %lu of %lu packets\n",
                Ring->m_Endpoint->Dci(), Fit, Stage->LastPacket - Stage->FirstPacket + 1);

        for (Packet = Stage->FirstPacket; Packet < Stage->FirstPacket + Fit; Packet++)
            Size += XhciIsochPacketLength(Transfer, Packet);

        KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
        Stage->LastPacket = Stage->FirstPacket + Fit - 1;
        Stage->Size = Size;
        Stage->Truncated = TRUE;
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
    }

    (VOID)LayoutStage(Ring, Stage, TRUE);

    /* The stage may be gone already; only the ring is used from here */
    Ring->RingDoorbell();
}

/**
 * Lays the stage's TDs out on the ring. Without Commit it only counts how many TDs fit
 * the current segment and the reserved ones, the last of them still able to end the stage.
 */
ULONG
XhciIsochRing::LayoutStage(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciIsochStage* Stage,
    _In_ BOOLEAN Commit)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    XhciIsochTransfer* Transfer = Stage->Transfer;
    const SCATTER_GATHER_LIST* SgList = Stage->SgList;
    ULONG LastSlot = Ring->m_LastIndex;
    ULONG LastPacket = Stage->LastPacket;
    ULONG PerFrame = Self->m_PacketsPerFrame;
    BOOLEAN Guard = XhciIsochNeedsLinkGuard(Ring);
    BOOLEAN AllowBei = !Ring->m_Controller->HasErrata(XhciErrata::IsochInterruptEveryTd);
    ULONG Hcc2 = Ring->m_Controller->m_Registers.Hccparams2();
    BOOLEAN ExtendedTbc = (Hcc2 & XHCI_HCC2_LEC) && (Hcc2 & XHCI_HCC2_ETC);
    ULONG Esit = XhciIsochEsitPayload(Ring);
    ULONG MaxPacket = Ring->m_Endpoint->MaxPacketSize();
    ULONG BurstSize = Ring->m_Endpoint->MaxBurst() + 1;
    ULONG Target = (Ring->m_InterrupterTarget & XHCI_TRB_INTERRUPTER_MASK) << XHCI_TRB_INTERRUPTER_SHIFT;
    ULONG Spares = XhciIsochCountList(&Stage->FreeSegments);
    ULONG Index = Ring->m_EnqueueIndex;
    ULONG Element = 0;
    ULONG Offset = 0;
    ULONG Placed = 0;
    ULONG Fit = 0;
    ULONG Packet;
    KIRQL OldIrql;

    if (Commit)
    {
        KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
        Stage->FirstSegment = Ring->m_Segment;
        Stage->LastSegment = Ring->m_Segment;
        Stage->ScanSegment = Ring->m_Segment;
        Stage->FirstIndex = Ring->m_EnqueueIndex;
        Stage->EndIndex = Ring->m_EnqueueIndex;
        Stage->ScanIndex = Ring->m_EnqueueIndex;
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
    }

    for (Packet = Stage->FirstPacket; Packet <= LastPacket; Packet++)
    {
        ULONG Length = XhciIsochPacketLength(Transfer, Packet);
        ULONG Carried = min(Length, Esit);
        ULONG Pieces = XhciIsochCountPieces(SgList, Element, Offset, Carried);
        BOOLEAN IsLast = (Packet == LastPacket);
        ULONG Frame = Transfer->StartFrame + Packet / PerFrame;
        PXHCI_TRB First = NULL;
        volatile UCHAR* CycleByte;
        BOOLEAN Tail;
        BOOLEAN Bei;
        XHCI_TRB Trb;
        ULONG Tdpc;
        ULONG Tbc;
        ULONG Tlbpc;
        ULONG Mapped = 0;
        ULONG Piece;
        ULONG64 Value;

        if (Commit)
            Index = Ring->m_EnqueueIndex;

        /* The data TRBs and the Event Data TRB of a TD stay in front of the Link slot */
        if (Index + Pieces + 1 > LastSlot)
        {
            if (!Commit)
            {
                if (Spares == 0 || Pieces + 1 > LastSlot)
                    break;
                Spares--;
            }
            else
            {
                First = &Ring->m_Trbs[Ring->m_EnqueueIndex];
                InsertLink(Ring, Stage, TRUE);
            }
            Index = 0;
        }

        /* A full speed TD behind a TT ends in a new segment instead of meeting a Link first */
        Tail = Guard && (IsLast || LastSlot - (Index + Pieces) < Self->m_TrbsPerTd);

        if (!Commit)
        {
            if (!Guard || Spares != 0)
                Fit = Placed + 1;

            if (Tail)
            {
                if (Spares == 0)
                    break;
                Spares--;
                Index = 1;
            }
            else
            {
                Index += Pieces + 1;
            }

            Placed++;
            XhciIsochAdvance(SgList, &Element, &Offset, Length);
            continue;
        }

        /* Burst fields (xHCI 4.11.2.3) */
        Tdpc = (MaxPacket != 0) ? (Carried + MaxPacket - 1) / MaxPacket : 1;
        if (Tdpc == 0)
            Tdpc = 1;
        Tbc = (Tdpc + BurstSize - 1) / BurstSize - 1;
        Tlbpc = (Tdpc % BurstSize) ? (Tdpc % BurstSize) - 1 : BurstSize - 1;

        if (First == NULL)
            First = &Ring->m_Trbs[Ring->m_EnqueueIndex];

        for (Piece = 0; Piece < Pieces; Piece++)
        {
            BOOLEAN LastData = (Piece + 1 == Pieces);
            PXHCI_TRB Slot = &Ring->m_Trbs[Ring->m_EnqueueIndex];
            ULONG64 Address;
            ULONG Chunk;
            ULONG SizeField;

            /* QUIRK: bytes beyond the max ESIT payload of a packet are never sent */
            XhciIsochTakePiece(SgList, &Element, &Offset, Carried - Mapped, &Address, &Chunk);
            Mapped += Chunk;

            RtlZeroMemory(&Trb, sizeof(Trb));
            Trb.Dword[0] = (ULONG)Address;
            Trb.Dword[1] = (ULONG)(Address >> 32);

            if (Piece == 0 && ExtendedTbc)
                SizeField = Tbc;
            else
                SizeField = Ring->TdSize(Ring->PacketCount(Carried), Mapped, LastData);

            Trb.Dword[2] = Chunk | (SizeField << XHCI_TRB_TD_SIZE_SHIFT) | Target;
            Trb.Dword[3] = XHCI_TRB_CHAIN | (LastData ? XHCI_TRB_ENT : 0);

            if (Piece == 0)
            {
                /* QUIRK: every TD carries its Frame ID, SIA is never used */
                Trb.Dword[3] |= (static_cast<ULONG>(XhciTrbType::Isoch) << XHCI_TRB_TYPE_SHIFT) |
                                (Tlbpc << XHCI_ISOCH_TLBPC_SHIFT) |
                                ((Frame & XHCI_ISOCH_FRAME_ID_MASK) << XHCI_ISOCH_FRAME_ID_SHIFT);
                if (!ExtendedTbc)
                    Trb.Dword[3] |= Tbc << XHCI_ISOCH_TBC_SHIFT;
            }
            else
            {
                Trb.Dword[3] |= static_cast<ULONG>(XhciTrbType::Normal) << XHCI_TRB_TYPE_SHIFT;
            }

            /* The first slot of the TD stays invalid until the whole TD is written */
            Trb.Dword[3] |= (Slot == First) ? (Ring->m_Cycle ^ XHCI_TRB_CYCLE) : Ring->m_Cycle;

            *Slot = Trb;
            Ring->m_EnqueueIndex++;
        }

        XhciIsochAdvance(SgList, &Element, &Offset, Length - Mapped);

        if (Tail)
        {
            RtlZeroMemory(&Trb, sizeof(Trb));
            Trb.Dword[3] = (static_cast<ULONG>(XhciTrbType::Normal) << XHCI_TRB_TYPE_SHIFT) |
                           XHCI_TRB_CHAIN | XHCI_TRB_ENT | Ring->m_Cycle;
            Ring->m_Trbs[Ring->m_EnqueueIndex] = Trb;
            Ring->m_EnqueueIndex++;

            InsertLink(Ring, Stage, FALSE);
        }

        /* An interrupt at least every 128 packets and at every stage end */
        Bei = AllowBei &&
              !IsLast &&
              Packet + 1 != Transfer->PacketsInUrb &&
              ((Packet + 1) & XhciIsochInterruptEvery) != 0;

        Value = USB_ENDPOINT_TYPE_ISOCHRONOUS | ((ULONG64)Packet << 16) | ((ULONG64)Transfer->StartFrame << 32);
        RtlZeroMemory(&Trb, sizeof(Trb));
        Trb.Dword[0] = (ULONG)Value;
        Trb.Dword[1] = (ULONG)(Value >> 32);
        Trb.Dword[2] = Target;
        Trb.Dword[3] = (static_cast<ULONG>(XhciTrbType::EventData) << XHCI_TRB_TYPE_SHIFT) |
                       XHCI_TRB_IOC | (Bei ? XHCI_TRB_BEI : 0) | Ring->m_Cycle;
        Ring->m_Trbs[Ring->m_EnqueueIndex] = Trb;
        Ring->m_EnqueueIndex++;

        /* Recycled segments are not cleared, and a stale TRB could look valid */
        Stage->EndIndex = Ring->m_EnqueueIndex;
        RtlZeroMemory(&Ring->m_Trbs[Ring->m_EnqueueIndex], sizeof(XHCI_TRB));

        if (IsLast || Self->m_DoorbellAfterFirstTd)
        {
            KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
            if (IsLast)
            {
                Transfer->PacketsQueued = Packet + 1 - Transfer->PacketsDropped;
                Transfer->TdsQueued++;
                Transfer->BytesQueued += Stage->Size;
                Stage->OnHardware = TRUE;
                Self->m_PendingStages++;
                Self->m_LastScheduledFrame = Frame;
                InterlockedOr(&Self->m_Flags, XhciIsochLastFrameValid);

                /* A cut stage leaves the rest of the URB to skip */
                if (Stage->Truncated)
                    SkipUnmapped(Transfer);
            }
            InterlockedOr(&Self->m_Flags, XhciIsochEmptyExpected | XhciIsochDoorbellRung);
            KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
        }

        /* A one byte register write publishes the TD after every store above */
        KeMemoryBarrier();
        CycleByte = reinterpret_cast<volatile UCHAR*>(&First->Dword[3]);
        WRITE_REGISTER_UCHAR(const_cast<PUCHAR>(CycleByte), (UCHAR)(*CycleByte ^ XHCI_TRB_CYCLE));
        KeMemoryBarrier();

        if (Self->m_DoorbellAfterFirstTd)
        {
            Self->m_DoorbellAfterFirstTd = FALSE;
            Ring->RingDoorbell();
        }

        Placed++;
    }

    return Commit ? Placed : Fit;
}

/** Ends the current segment with a Link TRB into the stage's next reserved segment. */
VOID
XhciIsochRing::InsertLink(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciIsochStage* Stage,
    _In_ BOOLEAN FirstOfTd)
{
    XhciDmaBuffer* Next;
    XHCI_TRB Trb;
    KIRQL OldIrql;

    /* The layout pass made sure a reserved segment is there */
    NT_ASSERT(!IsListEmpty(&Stage->FreeSegments));

    Next = CONTAINING_RECORD(RemoveHeadList(&Stage->FreeSegments), XhciDmaBuffer, Link);
    XhciClearListEntry(&Next->Link);

    Ring->BuildLinkTrb(&Trb, FirstOfTd, !FirstOfTd);
    Trb.Dword[0] = Next->LogicalAddress.LowPart;
    Trb.Dword[1] = (ULONG)Next->LogicalAddress.HighPart;
    Ring->m_Trbs[Ring->m_EnqueueIndex] = Trb;

    /* Never Toggle Cycle: the ring is an endless forward chain of fresh segments */
    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    InsertTailList(&Stage->UsedSegments, &Ring->m_Segment->Link);
    Ring->m_Segment = Next;
    Ring->m_Trbs = static_cast<PXHCI_TRB>(Next->VirtualAddress);
    Ring->m_EnqueueIndex = 0;
    Stage->LastSegment = Next;
    Stage->EndIndex = 0;
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
}

/* Completion ****************************************************************/

/** Accounts one TD event; closing the stage's last TD frees the stage. */
VOID
XhciIsochRing::CompleteTd(
    _In_ XhciTransferRing* Ring,
    _In_ XhciIsochTransfer* Transfer,
    _In_ XhciIsochStage* Stage,
    _In_ ULONG Sequence,
    _In_ ULONG Code,
    _In_ ULONG Bytes,
    _In_ BOOLEAN CloseStage)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    struct _URB_ISOCH_TRANSFER* Urb;
    PSCATTER_GATHER_LIST SgList;
    ULONG Packet;
    ULONG Index;
    ULONG InFlight;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);

    /* A reclaim or a stale pass may have taken the stage meanwhile */
    if (!XhciIsochIsActive(&Self->m_Pending, Transfer, Sequence) ||
        !Stage->InUse ||
        Stage->Transfer != Transfer)
    {
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
        return;
    }

    Urb = Transfer->Urb;
    Packet = Stage->NextPacket - 1;
    if (Packet >= Transfer->PacketsInUrb)
    {
        DPRINT1("Isoch DCI %lu: event for packet %lu of %lu\n",
                Ring->m_Endpoint->Dci(), Packet, Transfer->PacketsInUrb);
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
        return;
    }

    /* TDs the controller passed without a Missed Service event */
    for (Index = Packet; Index > 0 && Urb->IsoPacket[Index - 1].Status == XhciIsochNotSet; Index--)
    {
        DPRINT1("Isoch DCI %lu: packet %lu was skipped by the controller\n",
                Ring->m_Endpoint->Dci(), Index - 1);
        Urb->IsoPacket[Index - 1].Status = USBD_STATUS_ISO_TD_ERROR;
    }

    if (Urb->IsoPacket[Packet].Status == XhciIsochNotSet && (!XhciIsochStoppedCode(Code) || Bytes != 0))
    {
        Urb->IsoPacket[Packet].Status = XhciTransferRing::UsbdStatusFromCompletionCode(Code);
        Transfer->BytesTransferred += Bytes;
        if (XhciIsochEndpointIn(Ring))
            Urb->IsoPacket[Packet].Length = Bytes;
    }

    if (Packet != Stage->LastPacket || !CloseStage)
    {
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
        return;
    }

    if (!Stage->OnHardware)
    {
        DPRINT1("Isoch DCI %lu: end of a stage still being written was reported\n", Ring->m_Endpoint->Dci());
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
        return;
    }

    SgList = Stage->SgList;
    Stage->SgList = NULL;
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    PutScatterGather(Ring, SgList);

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    if (!XhciIsochIsActive(&Self->m_Pending, Transfer, Sequence) || !Stage->InUse)
    {
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
        return;
    }

    FreeStage(Ring, Stage);
    Transfer->TdsRetired++;
    if (Self->m_PendingStages != 0)
        Self->m_PendingStages--;
    InFlight = Self->m_PendingStages;

    /* Complete only once no stage is in use and every packet was queued or dropped */
    if (TransferDone(Transfer))
        CompleteAfterUncancel(Ring, Transfer, XhciIsochNotSet, XhciIsochNotSet, FALSE, FALSE);

    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    KickAfterStage(Ring, InFlight);
}

/** Completes every active transfer older than Newest; their events will not come. */
VOID
XhciIsochRing::FinishExpiredIsoch(
    _In_ XhciTransferRing* Ring,
    _In_ const XhciIsochTransfer* Newest,
    _In_ ULONG Sequence)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    XhciIsochTransfer* Transfer;
    LIST_ENTRY Stale;
    KIRQL OldIrql;

    InitializeListHead(&Stale);

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    if (XhciIsochIsActive(&Self->m_Pending, Newest, Sequence))
    {
        while (Self->m_Pending.Flink != &Newest->Link)
            InsertTailList(&Stale, RemoveHeadList(&Self->m_Pending));
    }
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    while (!IsListEmpty(&Stale))
    {
        Transfer = CONTAINING_RECORD(RemoveHeadList(&Stale), XhciIsochTransfer, Link);
        XhciClearListEntry(&Transfer->Link);

        DPRINT("Isoch DCI %lu: completing stale request %p\n", Ring->m_Endpoint->Dci(), Transfer->Request);
        ReleaseAllStages(Ring, Transfer);

        KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
        CompleteAfterUncancel(Ring, Transfer, XhciIsochNotSet, XhciIsochNotSet, TRUE, FALSE);
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
    }
}

/** Closes the stages of Transfer older than Stage as missed. */
VOID
XhciIsochRing::RetireExpiredStages(
    _In_ XhciTransferRing* Ring,
    _In_ XhciIsochTransfer* Transfer,
    _In_ XhciIsochStage* Stage,
    _In_ ULONG Sequence)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    XhciIsochStage* Oldest;
    BOOLEAN Progress;
    KIRQL OldIrql;

    for (;;)
    {
        KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
        if (!XhciIsochIsActive(&Self->m_Pending, Transfer, Sequence) ||
            !Stage->InUse ||
            Transfer->StageCount == 0)
        {
            KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
            return;
        }

        Oldest = XhciIsochStageAt(Transfer, 0);
        if (Oldest == Stage)
        {
            KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
            return;
        }

        Oldest->NextPacket = Oldest->LastPacket + 1;
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

        CompleteTd(Ring, Transfer, Oldest, Sequence, static_cast<ULONG>(XhciCompletionCode::MissedService), 0, TRUE);

        KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
        Progress = !Oldest->InUse || XhciIsochStageAt(Transfer, 0) != Oldest;
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

        if (!Progress)
            return;
    }
}

/** Returns the lists and frees every stage of a transfer nobody else can reach any more. */
ULONG
XhciIsochRing::ReleaseAllStages(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciIsochTransfer* Transfer)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    XhciIsochStage* Stage;
    PSCATTER_GATHER_LIST SgList;
    ULONG Freed = 0;
    KIRQL OldIrql;

    for (;;)
    {
        KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
        if (Transfer->StageCount == 0)
        {
            KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
            break;
        }

        Stage = XhciIsochStageAt(Transfer, 0);
        SgList = Stage->SgList;
        Stage->SgList = NULL;
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

        PutScatterGather(Ring, SgList);

        KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
        if (Stage->OnHardware)
        {
            Freed++;
            Transfer->TdsRetired++;
            if (Self->m_PendingStages != 0)
                Self->m_PendingStages--;
        }
        FreeStage(Ring, Stage);
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
    }

    return Freed;
}

/** Fills in the URB from the transfer record and frees what intake built. */
VOID
XhciIsochRing::Finalize(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciIsochTransfer* Transfer,
    _In_ USBD_STATUS PacketStatus,
    _In_ USBD_STATUS HeaderStatus)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    struct _URB_ISOCH_TRANSFER* Urb = Transfer->Urb;
    USBD_STATUS Status;
    ULONG Index;

    NT_ASSERT(Transfer->StageCount == 0);

    Urb->TransferBufferLength = Transfer->BytesTransferred;

    for (Index = 0; Index < Transfer->PacketsInUrb; Index++)
    {
        if (PacketStatus != XhciIsochNotSet)
            Urb->IsoPacket[Index].Status = PacketStatus;
        else if (Urb->IsoPacket[Index].Status == XhciIsochNotSet)
            Urb->IsoPacket[Index].Status = USBD_STATUS_ISO_NOT_ACCESSED_BY_HW;

        /* QUIRK: added to whatever the URB held, as the USB 2.0 stack does */
        if (Urb->IsoPacket[Index].Status != USBD_STATUS_SUCCESS)
        {
            Urb->ErrorCount++;
            Self->m_TdFailures++;
        }
        Self->m_Tds++;
    }

    if (HeaderStatus != XhciIsochNotSet)
        Status = HeaderStatus;
    else if (Urb->ErrorCount == Transfer->PacketsInUrb)
        Status = USBD_STATUS_ISOCH_REQUEST_FAILED;
    else
        Status = USBD_STATUS_SUCCESS;

    Urb->Hdr.Status = Status;
    Transfer->Status = XhciTransferRing::NtStatusFromUsbdStatus(Status);

    Self->m_Transfers++;
    Self->m_Bytes += Transfer->BytesTransferred;
    if (!NT_SUCCESS(Transfer->Status))
    {
        Self->m_FailedTransfers++;

        if (Transfer->Status != STATUS_CANCELLED)
        {
            DPRINT1("Isoch DCI %lu request %p failed, 0x%08lx USBD 0x%08lx, %lu of %lu packets in error\n",
                    Ring->m_Endpoint->Dci(), Transfer->Request, Transfer->Status, Status,
                    Urb->ErrorCount, Transfer->PacketsInUrb);
        }
    }

    if (Transfer->OwnMdl)
    {
        IoFreeMdl(Transfer->Mdl);
        Transfer->OwnMdl = FALSE;
    }
    Transfer->Mdl = NULL;
    Transfer->Initialized = 0;
}

/** Completes a request that was never cancelable. */
_Requires_lock_not_held_(Ring->m_Lock)
VOID
XhciIsochRing::CompletePlain(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciIsochTransfer* Transfer,
    _In_ USBD_STATUS PacketStatus,
    _In_ USBD_STATUS HeaderStatus,
    _In_ BOOLEAN Async)
{
    KIRQL OldIrql;

    Finalize(Ring, Transfer, PacketStatus, HeaderStatus);

    if (!Async)
    {
        WdfRequestComplete(Transfer->Request, Transfer->Status);
        return;
    }

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    InsertTailList(&Ring->m_Type.Isoch.m_Completion, &Transfer->Link);
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    WdfDpcEnqueue(Ring->m_CompletionDpc);
}

_Requires_lock_held_(Ring->m_Lock)
VOID
XhciIsochRing::CompleteAfterUncancel(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciIsochTransfer* Transfer,
    _In_ USBD_STATUS PacketStatus,
    _In_ USBD_STATUS HeaderStatus,
    _In_ BOOLEAN FinishCanceled,
    _In_ BOOLEAN Async)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    NTSTATUS Status;

    switch (Transfer->CancelState)
    {
        case XhciIsochCancel::Idle:
            break;

        case XhciIsochCancel::Armed:
            if (XhciIsListEntryLinked(&Transfer->Link))
            {
                RemoveEntryList(&Transfer->Link);
                XhciClearListEntry(&Transfer->Link);
            }

            Status = WdfRequestUnmarkCancelable(Transfer->Request);
            if (!NT_SUCCESS(Status))
            {
                /* The cancel callback is about to run and finishes the job */
                Transfer->CancelState = XhciIsochCancel::CancelRoutinePending;
                InsertTailList(&Self->m_WaitingForCancel, &Transfer->Link);
                return;
            }
            Transfer->CancelState = XhciIsochCancel::Idle;
            break;

        case XhciIsochCancel::Done:
            if (!FinishCanceled)
                return;
            if (HeaderStatus == XhciIsochNotSet)
                HeaderStatus = USBD_STATUS_CANCELED;
            break;

        default:
            NT_ASSERT(FALSE);
            return;
    }

    if (XhciIsListEntryLinked(&Transfer->Link))
    {
        RemoveEntryList(&Transfer->Link);
        XhciClearListEntry(&Transfer->Link);
    }

    KeReleaseSpinLockFromDpcLevel(&Ring->m_Lock);

    Finalize(Ring, Transfer, PacketStatus, HeaderStatus);

    if (Async)
    {
        KeAcquireSpinLockAtDpcLevel(&Ring->m_Lock);
        InsertTailList(&Self->m_Completion, &Transfer->Link);
        KeReleaseSpinLockFromDpcLevel(&Ring->m_Lock);
        WdfDpcEnqueue(Ring->m_CompletionDpc);
    }
    else
    {
        WdfRequestComplete(Transfer->Request, Transfer->Status);
    }

    KeAcquireSpinLockAtDpcLevel(&Ring->m_Lock);
}

VOID
XhciIsochRing::CompleteBatch(
    _In_ XhciTransferRing* Ring,
    _Inout_ PLIST_ENTRY Batch)
{
    XhciIsochTransfer* Transfer;
    KIRQL OldIrql;

    while (!IsListEmpty(Batch))
    {
        Transfer = CONTAINING_RECORD(RemoveHeadList(Batch), XhciIsochTransfer, Link);
        XhciClearListEntry(&Transfer->Link);

        if (Transfer->Status != STATUS_PENDING)
        {
            WdfRequestComplete(Transfer->Request, Transfer->Status);
            continue;
        }

        ReleaseAllStages(Ring, Transfer);

        /* QUIRK: everything reclaimed reports canceled, canceled or not */
        KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
        CompleteAfterUncancel(Ring, Transfer, XhciIsochNotSet, USBD_STATUS_CANCELED, TRUE, FALSE);
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
    }
}

_Requires_lock_held_(Ring->m_Lock)
BOOLEAN
XhciIsochRing::OwnsRequests(
    _In_ const XhciTransferRing* Ring)
{
    const XhciIsochRing* Self = &Ring->m_Type.Isoch;

    return !IsListEmpty(&Self->m_Pending) ||
           !IsListEmpty(&Self->m_WaitingForCancel) ||
           !IsListEmpty(&Self->m_Completion);
}

VOID
NTAPI
XhciIsochRing::EvtRequestCancel(
    _In_ WDFREQUEST Request)
{
    XhciIsochTransfer* Transfer = &XhciGetRequestData(Request)->Isoch;
    XhciTransferRing* Ring = Transfer->Ring;
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    BOOLEAN Queue = FALSE;
    BOOLEAN Report = FALSE;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    if (Transfer->CancelState == XhciIsochCancel::Armed)
    {
        if (Self->m_Flags & XhciIsochOkToReclaim)
        {
            XhciIsochMoveList(&Self->m_Completion, &Self->m_Pending);
            Queue = TRUE;
        }
        else
        {
            InterlockedOr(&Self->m_Flags, XhciIsochCanceled);
            Report = TRUE;
        }
    }
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    if (Report)
        Ring->m_Endpoint->OnTransferCanceled();

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    if (Transfer->CancelState == XhciIsochCancel::CancelRoutinePending)
    {
        RemoveEntryList(&Transfer->Link);
        InsertTailList(&Self->m_Completion, &Transfer->Link);
        Queue = TRUE;
    }
    Transfer->CancelState = XhciIsochCancel::Done;
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    if (Queue)
        WdfDpcEnqueue(Ring->m_CompletionDpc);
}

/* Transfer events ***********************************************************/

VOID
XhciIsochRing::Ed1Event(
    _In_ XhciTransferRing* Ring,
    _In_ const XHCI_TRB* Event)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    ULONG64 Value = XhciTrbPointer(Event);
    ULONG Code = XhciTrbCompletionCode(Event);
    ULONG Bytes = Event->Dword[2] & XHCI_TRANSFER_EVENT_LENGTH_MASK;
    ULONG Packet = (ULONG)(Value >> 16) & 0xFFFF;
    ULONG Frame = (ULONG)(Value >> 32);
    XhciIsochTransfer* Transfer = NULL;
    XhciIsochStage* Stage = NULL;
    ULONG Sequence = 0;
    PLIST_ENTRY Entry;
    ULONG Index;
    KIRQL OldIrql;

    if ((Value & XHCI_EVENT_DATA_TYPE_MASK) != USB_ENDPOINT_TYPE_ISOCHRONOUS)
    {
        DPRINT1("Isoch DCI %lu: Event Data 0x%I64x is not an isoch tag, resetting\n",
                Ring->m_Endpoint->Dci(), Value);
        Ring->m_Controller->RaiseControllerFault(XhciRecovery::ResetHost, XhciIsochReasonBadEventData);
        return;
    }

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);

    if (Self->m_Flags & XhciIsochStoppedSeen)
    {
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
        return;
    }

    /* QUIRK: found by start frame, so of two URBs with one start frame the older one matches */
    for (Entry = Self->m_Pending.Flink; Entry != &Self->m_Pending; Entry = Entry->Flink)
    {
        XhciIsochTransfer* Candidate = CONTAINING_RECORD(Entry, XhciIsochTransfer, Link);

        if (Candidate->StartFrame == Frame)
        {
            Transfer = Candidate;
            break;
        }
    }

    if (Transfer != NULL)
    {
        Sequence = Transfer->Sequence;

        for (Index = 0; Index < Transfer->StageCount; Index++)
        {
            XhciIsochStage* Candidate = XhciIsochStageAt(Transfer, Index);

            if (Packet <= Candidate->LastPacket)
            {
                Stage = Candidate;
                break;
            }
        }

        if (Stage == NULL)
        {
            DPRINT1("Isoch DCI %lu: no stage holds packet %lu of request %p\n",
                    Ring->m_Endpoint->Dci(), Packet, Transfer->Request);
        }
        else if (Packet == 0 || Packet > Stage->NextPacket - 1)
        {
            Stage->NextPacket = Packet + 1;
            Transfer->PacketsRetired = Packet + 1;
        }
    }

    if (XhciIsochStoppedCode(Code))
    {
        /* Some controllers report the stop on the Event Data TRB */
        InterlockedOr(&Self->m_Flags, XhciIsochStoppedSeen);
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

        if (Stage != NULL)
            CompleteTd(Ring, Transfer, Stage, Sequence, Code, Bytes, FALSE);

        Ring->m_Endpoint->OnStoppedEvent();
        return;
    }

    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    /* A late event of a request that is gone already */
    if (Stage == NULL)
        return;

    FinishExpiredIsoch(Ring, Transfer, Sequence);
    RetireExpiredStages(Ring, Transfer, Stage, Sequence);
    CompleteTd(Ring, Transfer, Stage, Sequence, Code, Bytes, TRUE);
}

VOID
XhciIsochRing::Ed0Event(
    _In_ XhciTransferRing* Ring,
    _In_ const XHCI_TRB* Event)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    ULONG Code = XhciTrbCompletionCode(Event);
    ULONG64 Pointer = XhciTrbPointer(Event);
    XhciIsochTransfer* Transfer = NULL;
    XhciIsochStage* Stage;
    ULONG Sequence = 0;
    ULONG Bytes;
    BOOLEAN Matched;
    KIRQL OldIrql;

    switch (static_cast<XhciCompletionCode>(Code))
    {
        case XhciCompletionCode::RingUnderrun:
        case XhciCompletionCode::RingOverrun:
            OnRingStarved(Ring);
            return;

        case XhciCompletionCode::MissedService:
            Self->m_MissedServiceErrors++;

            /* A Missed Service event without a TRB pointer has nothing to complete */
            if (Pointer == 0)
                return;
            break;

        case XhciCompletionCode::NoPingResponse:
            UcxEndpointNoPingResponseError(Ring->m_Endpoint->Handle());

            /* The length is invalid without a TRB pointer */
            if (Pointer == 0)
                return;
            break;

        case XhciCompletionCode::Stopped:
        case XhciCompletionCode::StoppedLengthInvalid:
        case XhciCompletionCode::StoppedShortPacket:
        case XhciCompletionCode::DataBufferError:
        case XhciCompletionCode::BabbleDetected:
        case XhciCompletionCode::IsochBufferOverrun:
        case XhciCompletionCode::UsbTransactionError:
        case XhciCompletionCode::StallError:
        case XhciCompletionCode::EventLost:
        case XhciCompletionCode::SplitTransactionError:
            break;

        default:
            DPRINT1("Isoch DCI %lu: unexpected code %lu without Event Data\n", Ring->m_Endpoint->Dci(), Code);
            break;
    }

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);

    if (Self->m_Flags & XhciIsochStoppedSeen)
    {
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
        return;
    }

    Matched = FindTrb(Ring, Event, &Stage, &Bytes);
    if (Stage != NULL)
    {
        Transfer = Stage->Transfer;
        Sequence = Transfer->Sequence;
    }

    if (XhciIsochStoppedCode(Code))
    {
        /* Also covers a stop at the enqueue pointer or on a Link TRB after the last stage */
        InterlockedOr(&Self->m_Flags, XhciIsochStoppedSeen);
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

        if (Stage != NULL)
            CompleteTd(Ring, Transfer, Stage, Sequence, Code, Bytes, FALSE);

        Ring->m_Endpoint->OnStoppedEvent();
        return;
    }

    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    if (Stage == NULL)
    {
        if (!Matched)
        {
            DPRINT("Isoch DCI %lu: event at 0x%I64x code %lu matched no stage, dropped\n",
                   Ring->m_Endpoint->Dci(), Pointer, Code);
        }
        return;
    }

    /* The following Event Data event or a later stale pass closes the stage */
    FinishExpiredIsoch(Ring, Transfer, Sequence);
    RetireExpiredStages(Ring, Transfer, Stage, Sequence);
    CompleteTd(Ring, Transfer, Stage, Sequence, Code, Bytes, FALSE);
}

/**
 * Finds the stage holding the TRB an event names, works out the bytes its TD moved and
 * moves the stage to the TD's packet. TRUE with no stage means the ring was empty.
 */
_Requires_lock_held_(Ring->m_Lock)
BOOLEAN
XhciIsochRing::FindTrb(
    _In_ XhciTransferRing* Ring,
    _In_ const XHCI_TRB* Event,
    _Outptr_result_maybenull_ XhciIsochStage** Found,
    _Out_ PULONG Bytes)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    ULONG64 Pointer = XhciTrbPointer(Event);
    ULONG Residual = Event->Dword[2] & XHCI_TRANSFER_EVENT_LENGTH_MASK;
    ULONG Code = XhciTrbCompletionCode(Event);
    XhciIsochStage* Stage = NULL;
    XhciDmaBuffer* Segment;
    const XHCI_TRB* Trbs;
    PLIST_ENTRY Entry;
    ULONG Index;
    ULONG Limit;
    ULONG Sum = 0;
    ULONG Count;
    BOOLEAN Hit = FALSE;

    *Found = NULL;
    *Bytes = 0;

    if (Pointer == 0)
    {
        DPRINT1("Isoch DCI %lu: event code %lu without a TRB pointer\n", Ring->m_Endpoint->Dci(), Code);
        return FALSE;
    }

    if (Pointer == Ring->EnqueuePointer())
        return TRUE;

    for (Entry = Self->m_Pending.Flink; Entry != &Self->m_Pending && Stage == NULL; Entry = Entry->Flink)
    {
        XhciIsochTransfer* Transfer = CONTAINING_RECORD(Entry, XhciIsochTransfer, Link);

        for (Count = 0; Count < Transfer->StageCount; Count++)
        {
            XhciIsochStage* Candidate = XhciIsochStageAt(Transfer, Count);

            if (XhciIsochStageHas(Candidate, Pointer))
            {
                Stage = Candidate;
                break;
            }
        }
    }

    if (Stage == NULL)
        return FALSE;

    *Found = Stage;

    /* Sum the TD's data TRBs up to the one named, from where the last scan stopped */
    Segment = Stage->ScanSegment;
    Index = Stage->ScanIndex;

    while (Segment != NULL && !Hit)
    {
        ULONG64 Base = (ULONG64)Segment->LogicalAddress.QuadPart;

        Trbs = static_cast<const XHCI_TRB*>(Segment->VirtualAddress);
        Limit = XhciIsochSegmentLimit(Stage, Segment);

        for (; Index < Limit; Index++)
        {
            BOOLEAN Match = (Base + (ULONG64)Index * sizeof(XHCI_TRB) == Pointer);
            ULONG Type = XhciTrbType(&Trbs[Index]);
            BOOLEAN Data = (Type == static_cast<ULONG>(XhciTrbType::Normal) ||
                            Type == static_cast<ULONG>(XhciTrbType::Isoch));

            if (Type == static_cast<ULONG>(XhciTrbType::Link))
            {
                if (!Match)
                    break;

                /* QUIRK: a stop on a Link TRB leaves the packet where it was */
                Stage->ScanSegment = Segment;
                Stage->ScanIndex = Index;
                *Bytes = Sum;
                return TRUE;
            }

            if (!Data && Type != static_cast<ULONG>(XhciTrbType::EventData))
                DPRINT1("Isoch DCI %lu: TRB type %lu inside a stage\n", Ring->m_Endpoint->Dci(), Type);

            if (!Match)
            {
                if (Data)
                    Sum += Trbs[Index].Dword[2] & XHCI_TRB_LENGTH_MASK;
                else if (Type == static_cast<ULONG>(XhciTrbType::EventData))
                    Sum = 0;
                continue;
            }

            if (Data)
                Sum = XhciIsochMatchBytes(Ring, Code, Trbs[Index].Dword[2] & XHCI_TRB_LENGTH_MASK, Residual, Sum);

            Hit = TRUE;
            break;
        }

        if (!Hit)
        {
            Segment = XhciIsochNextSegment(Stage, Segment);
            Index = 0;
        }
    }

    if (!Hit)
    {
        DPRINT1("Isoch DCI %lu: TRB 0x%I64x not found past the scan position\n", Ring->m_Endpoint->Dci(), Pointer);
        return TRUE;
    }

    *Bytes = Sum;
    Stage->ScanSegment = Segment;
    Stage->ScanIndex = Index;

    /* The TD's Event Data TRB names its packet */
    while (Segment != NULL)
    {
        Trbs = static_cast<const XHCI_TRB*>(Segment->VirtualAddress);
        Limit = XhciIsochSegmentLimit(Stage, Segment);

        for (; Index < Limit; Index++)
        {
            ULONG Type = XhciTrbType(&Trbs[Index]);

            if (Type == static_cast<ULONG>(XhciTrbType::EventData))
            {
                ULONG Packet = (Trbs[Index].Dword[0] >> 16) & 0xFFFF;

                Stage->NextPacket = Packet + 1;
                Stage->Transfer->PacketsRetired = Packet + 1;
                return TRUE;
            }

            if (Type == static_cast<ULONG>(XhciTrbType::Link))
            {
                if (!XhciIsochNeedsLinkGuard(Ring))
                    DPRINT1("Isoch DCI %lu: Link TRB inside a TD\n", Ring->m_Endpoint->Dci());
                break;
            }

            if (Type != static_cast<ULONG>(XhciTrbType::Normal) && Type != static_cast<ULONG>(XhciTrbType::Isoch))
                DPRINT1("Isoch DCI %lu: TRB type %lu before the Event Data TRB\n", Ring->m_Endpoint->Dci(), Type);
        }

        Segment = XhciIsochNextSegment(Stage, Segment);
        Index = 0;
    }

    return TRUE;
}

VOID
XhciIsochRing::OnRingStarved(
    _In_ XhciTransferRing* Ring)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    ULONG Now = Ring->m_Controller->GetFrameNumber(XhciIsochBiasNow);
    BOOLEAN Abort;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    Self->m_RingEmptyEvents++;
    InterlockedAnd(&Self->m_Flags, ~XhciIsochEmptyExpected);
    if (Self->m_PendingStages == 0)
        InterlockedAnd(&Self->m_Flags, ~XhciIsochLastFrameValid);

    Abort = Ring->m_Controller->HasErrata(XhciErrata::IsochMissedServiceSilent) &&
            Self->m_PendingStages != 0 &&
            (Self->m_Flags & XhciIsochLastFrameValid) &&
            (LONG)(Now - Self->m_LastScheduledFrame) >= 0;
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    if (Abort)
    {
        /*
         * The controller ran past TDs without a word. Their events may still sit on a
         * secondary event ring, so the endpoint stops and reclaims instead.
         */
        DPRINT1("Isoch DCI %lu: ring empty with stages unreported, reclaiming\n", Ring->m_Endpoint->Dci());
        Ring->m_Endpoint->OnTransferCanceled();
        return;
    }

    InterlockedExchange(&Self->m_RingEmptySeen, 1);
    if (LeaveRingEmptyWait(Ring))
        BuildTdsFromQueue(Ring);
}

/** Closes in flight stages as missed once every frame they were scheduled for has passed. */
BOOLEAN
XhciIsochRing::FinishLostStages(
    _In_ XhciTransferRing* Ring)
{
    XhciIsochRing* Self = &Ring->m_Type.Isoch;
    ULONG Now = Ring->m_Controller->GetFrameNumber(XhciIsochBiasNow);
    XhciIsochTransfer* Transfer;
    XhciIsochStage* Stage;
    PLIST_ENTRY Entry;
    ULONG Sequence;
    ULONG Index;
    BOOLEAN Stuck;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    if ((Self->m_Flags & XhciIsochLastFrameValid) && (LONG)(Now - Self->m_LastScheduledFrame) <= 0)
    {
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
        return FALSE;
    }
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    for (;;)
    {
        Stage = NULL;
        Transfer = NULL;
        Sequence = 0;

        KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
        for (Entry = Self->m_Pending.Flink; Entry != &Self->m_Pending && Stage == NULL; Entry = Entry->Flink)
        {
            Transfer = CONTAINING_RECORD(Entry, XhciIsochTransfer, Link);

            for (Index = 0; Index < Transfer->StageCount; Index++)
            {
                if (XhciIsochStageAt(Transfer, Index)->OnHardware)
                {
                    Stage = XhciIsochStageAt(Transfer, Index);
                    Stage->NextPacket = Stage->LastPacket + 1;
                    Sequence = Transfer->Sequence;
                    break;
                }
            }
        }
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

        if (Stage == NULL)
            break;

        DPRINT1("Isoch DCI %lu: closing a stage whose events never came\n", Ring->m_Endpoint->Dci());
        CompleteTd(Ring, Transfer, Stage, Sequence, static_cast<ULONG>(XhciCompletionCode::MissedService), 0, TRUE);

        KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
        Stuck = Stage->InUse && Stage->OnHardware && Stage->Transfer == Transfer;
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

        if (Stuck)
            break;
    }

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    InterlockedAnd(&Self->m_Flags, ~XhciIsochEmptyExpected);
    if (Self->m_PendingStages == 0)
        InterlockedAnd(&Self->m_Flags, ~XhciIsochLastFrameValid);
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    return TRUE;
}

VOID
NTAPI
XhciIsochRing::EvtWatchdog(
    _In_ WDFTIMER Timer)
{
    XhciTransferRing* Ring = XhciTransferRing::FromQueue(static_cast<WDFQUEUE>(WdfTimerGetParentObject(Timer)));
    XhciIsochRing* Self = &Ring->m_Type.Isoch;

    if (InterlockedCompareExchange(&Ring->m_MapState, 0, 0) != static_cast<LONG>(XhciMapState::AwaitingStarve))
        return;

    DPRINT1("Isoch DCI %lu: no ring empty event for %lu ms\n", Ring->m_Endpoint->Dci(), XhciIsochWatchdogMs);

    /* Close stages whose frames have passed so the ring does not stay parked */
    if (!FinishLostStages(Ring))
    {
        WdfTimerStart(Self->m_Watchdog, WDF_REL_TIMEOUT_IN_MS(XhciIsochWatchdogMs));
        return;
    }

    InterlockedExchange(&Self->m_RingEmptySeen, 1);
    if (LeaveRingEmptyWait(Ring))
        BuildTdsFromQueue(Ring);
}
