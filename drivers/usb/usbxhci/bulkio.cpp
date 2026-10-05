/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Bulk and interrupt transfer rings: staging, TD building, events and reclaim
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "xhcidrv.h"

#define NDEBUG
#include <debug.h>

static const ULONG XhciBulkStageTag = 'gtSB';
static const ULONG XhciBulkTransferTag = 'rTkB';

static const ULONG XhciBulkPendingStagesMax = 5;
static const ULONG XhciBulkSegmentWaitLimit = 3;
static const ULONG XhciBulkTrbBytesMax = 0x10000;
static const ULONG XhciBulkImmediateBytes = 8;
static const ULONG XhciBulkMinStageSize = 0x10000;
static const ULONG XhciBulkNoOpCount = 8;
static const USBD_STATUS XhciBulkStatusNotSet = (USBD_STATUS)0xFFFFFFFF;

/* Reason codes handed to the controller with a fatal error */
static const ULONG XhciBulkReasonEd0Invalid = 0x101F;
static const ULONG XhciBulkReasonEd0Duplicate = 0x1020;

/* m_Flags bits, all cleared when mapping starts */
static const ULONG XhciBulkAckExpected = 0x01;
static const ULONG XhciBulkHalted = 0x02;
static const ULONG XhciBulkStoppedSeen = 0x04;
static const ULONG XhciBulkCanceled = 0x08;
static const ULONG XhciBulkOkToReclaim = 0x10;
static const ULONG XhciBulkReclaimAck = 0x20;

/* m_SegmentWait values */
static const LONG XhciBulkWaitIdle = 0;
static const LONG XhciBulkWaitParked = 1;
static const LONG XhciBulkWaitArrived = 2;
static const LONG XhciBulkWaitAbandoned = 3;

enum class XhciBulkStep : ULONG
{
    Normal,
    Pad,
    EventData,
    Done
};

C_ASSERT((sizeof(XhciBulkStage) % 8) == 0);

/* Small helpers that only use public state *********************************/

static
BOOLEAN
NTAPI
XhciBulkIsHaltedCode(
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
XhciBulkIsStoppedCode(
    _In_ ULONG Code)
{
    return Code == static_cast<ULONG>(XhciCompletionCode::Stopped) ||
           Code == static_cast<ULONG>(XhciCompletionCode::StoppedLengthInvalid) ||
           Code == static_cast<ULONG>(XhciCompletionCode::StoppedShortPacket);
}

static
BOOLEAN
NTAPI
XhciBulkEndpointIn(
    _In_ const XhciTransferRing* Ring)
{
    return (Ring->m_Endpoint->Descriptor()->bEndpointAddress & USB_ENDPOINT_DIRECTION_MASK) != 0;
}

static
BOOLEAN
NTAPI
XhciBulkTransferIn(
    _In_ const XhciBulkTransfer* Transfer)
{
    return (Transfer->Urb->TransferFlags & USBD_TRANSFER_DIRECTION_IN) != 0;
}

static
BOOLEAN
NTAPI
XhciBulkChainedMdl(
    _In_ const XhciBulkTransfer* Transfer)
{
    return Transfer->Urb->TransferBufferMDL != NULL &&
           Transfer->Urb->TransferBufferMDL->Next != NULL;
}

/** Worst case number of pages Bytes can touch at any alignment. */
static
ULONG
NTAPI
XhciBulkWorstPages(
    _In_ ULONG Bytes)
{
    return (Bytes + 2 * (PAGE_SIZE - 1)) / PAGE_SIZE;
}

static
BOOLEAN
NTAPI
XhciBulkSegmentHas(
    _In_ const XhciDmaBuffer* Segment,
    _In_ ULONG Start,
    _In_ ULONG OnePastEnd,
    _In_ ULONG64 Address)
{
    ULONG64 Base = (ULONG64)Segment->LogicalAddress.QuadPart;
    ULONG Slots = Segment->Size / sizeof(XHCI_TRB);
    ULONG64 Limit;

    Limit = (OnePastEnd >= Slots) ? Segment->Size : (ULONG64)OnePastEnd * sizeof(XHCI_TRB);
    return Address >= Base + (ULONG64)Start * sizeof(XHCI_TRB) && Address < Base + Limit;
}

static
BOOLEAN
NTAPI
XhciBulkNeedsNoOps(
    _In_ const XhciTransferRing* Ring)
{
    return Ring->m_Controller->HasErrata(XhciErrata::XferPadSsBulkInTail) &&
           Ring->m_Device->Speed() == UsbSuperSpeed &&
           Ring->m_TransferType == USB_ENDPOINT_TYPE_BULK &&
           XhciBulkEndpointIn(Ring);
}

/** Bytes of the split IN rounding pad for a stage of Size bytes, 0 when none applies. */
static
ULONG
NTAPI
XhciBulkPadSize(
    _In_ const XhciTransferRing* Ring,
    _In_ ULONG Size)
{
    const XHCI_SLOT_CONTEXT* Slot;
    USB_DEVICE_SPEED Speed = Ring->m_Device->Speed();
    ULONG MaxPacket = Ring->m_Endpoint->MaxPacketSize();

    if (!Ring->m_Controller->HasErrata(XhciErrata::XferPacketAlignChunks) ||
        !XhciBulkEndpointIn(Ring) ||
        (Speed != UsbLowSpeed && Speed != UsbFullSpeed) ||
        MaxPacket == 0 ||
        (Size % MaxPacket) == 0)
    {
        return 0;
    }

    Slot = static_cast<const XHCI_SLOT_CONTEXT*>(Ring->m_Device->OutputContext(0));
    if (Slot == NULL || Slot->TTHubSlotId == 0)
        return 0;

    return MaxPacket - (Size % MaxPacket);
}

static
VOID
NTAPI
XhciBulkCopyIn(
    _In_ const XhciBulkTransfer* Transfer,
    _In_ const XhciBulkStage* Stage,
    _In_ ULONG Bytes)
{
    if (Bytes != 0 &&
        Transfer->Mechanism == XhciBulkMechanism::DoubleBuffer &&
        XhciBulkTransferIn(Transfer))
    {
        RtlCopyMemory(Stage->Buffer, Stage->DoubleBuffer->VirtualAddress, Bytes);
    }
}

static
VOID
NTAPI
XhciBulkMoveList(
    _Inout_ PLIST_ENTRY Destination,
    _Inout_ PLIST_ENTRY Source)
{
    while (!IsListEmpty(Source))
        InsertTailList(Destination, RemoveHeadList(Source));
}

/**
 * Scans one segment of a stage from Start for the TRB the event names, adding the bytes
 * the stage moved before it. Returns TRUE when the TRB was found.
 */
static
BOOLEAN
NTAPI
XhciBulkScanSegment(
    _In_ const XhciDmaBuffer* Segment,
    _In_ ULONG Start,
    _In_ const XHCI_TRB* Event,
    _Inout_ PULONG Bytes)
{
    const XHCI_TRB* Trbs = static_cast<const XHCI_TRB*>(Segment->VirtualAddress);
    ULONG Slots = Segment->Size / sizeof(XHCI_TRB);
    ULONG64 Pointer = XhciTrbPointer(Event);
    ULONG Residual = Event->Dword[2] & XHCI_TRANSFER_EVENT_LENGTH_MASK;
    ULONG Code = XhciTrbCompletionCode(Event);
    ULONG64 Address = (ULONG64)Segment->LogicalAddress.QuadPart + (ULONG64)Start * sizeof(XHCI_TRB);
    ULONG Index;

    for (Index = Start; Index < Slots; Index++, Address += sizeof(XHCI_TRB))
    {
        BOOLEAN Match = (Address == Pointer);
        ULONG Length = Trbs[Index].Dword[2] & XHCI_TRB_LENGTH_MASK;

        switch (XhciTrbType(&Trbs[Index]))
        {
            case static_cast<ULONG>(XhciTrbType::Normal):
                if (!Match)
                {
                    *Bytes += Length;
                    break;
                }

                switch (static_cast<XhciCompletionCode>(Code))
                {
                    case XhciCompletionCode::Stopped:
                    case XhciCompletionCode::DataBufferError:
                    case XhciCompletionCode::BabbleDetected:
                    case XhciCompletionCode::StallError:
                    case XhciCompletionCode::UsbTransactionError:
                    case XhciCompletionCode::SplitTransactionError:
                        if (Residual <= Length)
                            *Bytes += Length - Residual;
                        break;
                    case XhciCompletionCode::StoppedShortPacket:
                        *Bytes = Residual;
                        break;
                    default:
                        break;
                }
                break;

            case static_cast<ULONG>(XhciTrbType::EventData):
                if (!Match)
                    *Bytes = 0;
                break;

            case static_cast<ULONG>(XhciTrbType::Link):
                return Match;

            default:
                break;
        }

        if (Match)
            return TRUE;
    }

    return FALSE;
}

static
ULONG
NTAPI
XhciBulkStageBytes(
    _In_ const XhciBulkStage* Stage,
    _In_ const XHCI_TRB* Event)
{
    const LIST_ENTRY* Entry;
    ULONG Bytes = 0;
    ULONG Start = Stage->FirstIndex;

    for (Entry = Stage->UsedSegments.Flink; Entry != &Stage->UsedSegments; Entry = Entry->Flink)
    {
        if (XhciBulkScanSegment(CONTAINING_RECORD(Entry, XhciDmaBuffer, Link), Start, Event, &Bytes))
            return Bytes;
        Start = 0;
    }

    if (!XhciBulkScanSegment(Stage->LastSegment, Start, Event, &Bytes))
        DPRINT1("Event TRB 0x%I64x not found while sizing its stage\n", XhciTrbPointer(Event));

    return Bytes;
}

static
BOOLEAN
NTAPI
XhciBulkStageContains(
    _In_ const XhciBulkStage* Stage,
    _In_ ULONG64 Address)
{
    const LIST_ENTRY* Entry;
    ULONG Start = Stage->FirstIndex;

    for (Entry = Stage->UsedSegments.Flink; Entry != &Stage->UsedSegments; Entry = Entry->Flink)
    {
        if (XhciBulkSegmentHas(CONTAINING_RECORD(Entry, XhciDmaBuffer, Link), Start, MAXULONG, Address))
            return TRUE;
        Start = 0;
    }

    return XhciBulkSegmentHas(Stage->LastSegment, Start, Stage->EndIndex, Address);
}

/** Event Data value of a stage: its address, the endpoint type, and on 32 bit its generation. */
static
ULONG64
NTAPI
XhciBulkEventData(
    _In_ const XhciTransferRing* Ring,
    _In_ const XhciBulkStage* Stage)
{
    ULONG64 Value = (ULONG_PTR)Stage | (Ring->m_TransferType & XHCI_EVENT_DATA_TYPE_MASK);

#ifndef _WIN64
    Value |= (ULONG64)Stage->Generation << 32;
#endif
    return Value;
}

static
XhciBulkStage*
NTAPI
XhciBulkStageFromEventData(
    _In_ ULONG64 Value)
{
    return reinterpret_cast<XhciBulkStage*>((ULONG_PTR)Value & ~(ULONG_PTR)XHCI_EVENT_DATA_TYPE_MASK);
}

/* Ring type entry points ****************************************************/

NTSTATUS
XhciBulkRing::Initialize(
    _In_ XhciTransferRing* Ring)
{
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    XhciController* Controller = Ring->m_Controller;
    ULONG Index;

    RtlZeroMemory(Bulk, sizeof(*Bulk));
    InitializeListHead(&Bulk->m_Pending);
    InitializeListHead(&Bulk->m_WaitingForCancel);
    InitializeListHead(&Bulk->m_Completion);

    for (Index = 0; Index < StageSlots; Index++)
    {
        XhciBulkStage* Stage = &Bulk->m_Stages[Index];

        Stage->Signature = XhciBulkStageTag;
        Stage->Ring = Ring;
        InitializeListHead(&Stage->FreeSegments);
        InitializeListHead(&Stage->UsedSegments);
    }

    Bulk->m_ImmediateData = !Controller->HasErrata(XhciErrata::XferNoInlineData) &&
                            Ring->m_Endpoint->MaxPacketSize() >= XhciBulkImmediateBytes &&
                            !XhciBulkEndpointIn(Ring);

    if (Controller->HasErrata(XhciErrata::XferOneAsyncTdAtATime) ||
        Controller->HasErrata(XhciErrata::RingKeepTdInOneSegment))
    {
        Bulk->m_MaxPendingStages = 1;
    }
    else
    {
        Bulk->m_MaxPendingStages = XhciBulkPendingStagesMax;
    }

    /* A TD has to fit one segment, less the Link, Event Data and pad slots */
    if (Controller->HasErrata(XhciErrata::RingKeepTdInOneSegment))
        Ring->m_MaxStageSize = (Ring->m_SegmentSize / sizeof(XHCI_TRB) - 4) * PAGE_SIZE;

    return STATUS_SUCCESS;
}

VOID
XhciBulkRing::Enable(
    _In_ XhciTransferRing* Ring)
{
    ULONG Burst;

    NT_ASSERT(Ring->m_MaxStageSize >= XhciBulkMinStageSize);

    if (Ring->m_Device->Speed() != UsbSuperSpeed)
        return;

    Burst = Ring->m_Endpoint->MaxPacketSize() * (Ring->m_Endpoint->MaxBurst() + 1);
    if (Burst == 0 || (Ring->m_MaxStageSize % Burst) == 0)
        return;

    Ring->m_MaxStageSize -= Ring->m_MaxStageSize % Burst;
    Ring->m_MapRegisterCount = (Ring->m_MaxStageSize + PAGE_SIZE - 1) / PAGE_SIZE;
    DPRINT("Bulk DCI %lu stream %lu stage size cut to %lu\n",
           Ring->m_Endpoint->Dci(), Ring->m_StreamId, Ring->m_MaxStageSize);
}

VOID
XhciBulkRing::Disable(
    _In_ XhciTransferRing* Ring)
{
    KIRQL OldIrql;

    ReleaseParkedMapping(Ring);

    InterlockedCompareExchange(&Ring->m_MapState,
                               static_cast<LONG>(XhciMapState::Halted),
                               static_cast<LONG>(XhciMapState::PausedFill));

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    if (OwnsRequests(Ring))
    {
        DPRINT1("Bulk DCI %lu stream %lu disabled while it still owns requests\n",
                Ring->m_Endpoint->Dci(), Ring->m_StreamId);
    }
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
}

VOID
XhciBulkRing::ReleaseParkedMapping(
    _In_ XhciTransferRing* Ring)
{
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;

    /* A loop parked on a segment wait only resumes from a PASSIVE work item; let it go */
    if (InterlockedCompareExchange(&Bulk->m_SegmentWait, XhciBulkWaitAbandoned, XhciBulkWaitParked) ==
        XhciBulkWaitParked)
    {
        DPRINT1("Bulk DCI %lu stream %lu disabled while waiting for segments\n",
                Ring->m_Endpoint->Dci(), Ring->m_StreamId);

        if (Ring->ChangeMapState(FALSE, XhciMapState::Halting, XhciMapState::Halted) !=
            XhciMapState::Halting)
        {
            InterlockedCompareExchange(&Ring->m_MapState,
                                       static_cast<LONG>(XhciMapState::PausedFill),
                                       static_cast<LONG>(XhciMapState::Filling));
        }
    }
}

NTSTATUS
XhciBulkRing::EnableForwardProgress(
    _In_ XhciTransferRing* Ring,
    _In_ ULONG MaxTransferSize)
{
    ULONG Needed = XhciBulkWorstPages(MaxTransferSize) + 1;
    ULONG PerBurst = XhciBulkWorstPages(Ring->m_Endpoint->MaxEsitPayload()) + 1;
    ULONG PerSegment = Ring->m_SegmentSize / sizeof(XHCI_TRB) - 1;
    ULONG Usable = PerSegment - (PerSegment % PerBurst);
    ULONG Segments;
    NTSTATUS Status;

    if (Usable == 0)
    {
        DPRINT1("Bulk DCI %lu: a burst of %lu TRBs does not fit a segment\n",
                Ring->m_Endpoint->Dci(), PerBurst);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Segments = (Needed + Usable - 1) / Usable;
    if (Segments == 0)
        Segments = 1;

    Status = Ring->EnsureFreeSegments(Segments, FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Bulk DCI %lu: %lu forward progress segments not available, 0x%08lx\n",
                Ring->m_Endpoint->Dci(), Segments, Status);
        return Status;
    }

    Ring->m_DoubleBufferSize = 0;
    Ring->m_Type.Bulk.m_ImmediateData = FALSE;
    return STATUS_SUCCESS;
}

VOID
XhciBulkRing::Cleanup(
    _In_ XhciTransferRing* Ring)
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    if (OwnsRequests(Ring))
    {
        DPRINT1("Bulk DCI %lu stream %lu cleaned up with requests still owned\n",
                Ring->m_Endpoint->Dci(), Ring->m_StreamId);
        NT_ASSERT(FALSE);
    }
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
}

VOID
NTAPI
XhciBulkRing::ReadyNotification(
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

    /* A loop that is about to pause sees this and goes round once more */
    KickFill(Ring);
}

VOID
NTAPI
XhciBulkRing::EvtIoCanceledOnQueue(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request)
{
    XhciTransferRing* Ring = XhciTransferRing::FromQueue(Queue);
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    XhciBulkTransfer* Transfer = &XhciGetRequestData(Request)->Bulk;
    KIRQL OldIrql;

    if (Transfer->Initialized != XhciBulkTransferTag)
        InitTransfer(Ring, Transfer, Request);

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    Transfer->CancelState = XhciBulkCancel::Done;
    InsertTailList(&Bulk->m_Completion, &Transfer->Link);
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    QueueCompletionDpc(Ring);
}

VOID
NTAPI
XhciBulkRing::EvtCompletionDpc(
    _In_ WDFDPC Dpc)
{
    XhciTransferRing* Ring = XhciTransferRing::FromQueue(static_cast<WDFQUEUE>(WdfDpcGetParentObject(Dpc)));
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    LIST_ENTRY Batch;
    BOOLEAN Acknowledge = FALSE;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    if (Bulk->m_DpcRunning)
    {
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
        return;
    }
    Bulk->m_DpcRunning = TRUE;

    do
    {
        InitializeListHead(&Batch);
        XhciBulkMoveList(&Batch, &Bulk->m_Completion);
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

        CompleteBatch(Ring, &Batch);

        KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    } while (!IsListEmpty(&Bulk->m_Completion));

    if ((Bulk->m_Flags & XhciBulkReclaimAck) && !OwnsRequests(Ring))
    {
        Bulk->m_Flags &= ~XhciBulkReclaimAck;
        Acknowledge = TRUE;
    }

    Bulk->m_DpcRunning = FALSE;
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    if (Acknowledge)
        Ring->m_Endpoint->OnTransfersReclaimed();
}

VOID
XhciBulkRing::ResumeRingFill(
    _In_ XhciTransferRing* Ring)
{
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    XhciMapState Previous;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    Bulk->m_Flags = 0;
    Bulk->m_OutstandingEvents = 0;
    Ring->m_DoorbellRung = FALSE;
    NT_ASSERT(Bulk->m_PendingStages == 0);
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    Previous = Ring->ChangeMapState(TRUE, XhciMapState::Halted, XhciMapState::Filling);
    if (Previous == XhciMapState::Halted)
    {
        BuildTdsFromQueue(Ring);
        return;
    }

    DPRINT1("Bulk DCI %lu stream %lu started mapping from map state %ld\n",
            Ring->m_Endpoint->Dci(), Ring->m_StreamId, static_cast<LONG>(Previous));
}

VOID
XhciBulkRing::SuspendRingFill(
    _In_ XhciTransferRing* Ring)
{
    XhciMapState Previous;

    Previous = Ring->ChangeMapState(TRUE, XhciMapState::Halted, XhciMapState::Halting);

    /* QUIRK: a loop parked on a segment wait acknowledges only when the segments arrive */
    if (Previous == XhciMapState::PausedFill)
        Ring->ChangeMapState(FALSE, XhciMapState::Halting, XhciMapState::Halted);
}

VOID
XhciBulkRing::OnPipeHalted(
    _In_ XhciTransferRing* Ring)
{
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    KIRQL OldIrql;

    if (Ring->m_StreamId == 0)
        return;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    if (!(Bulk->m_Flags & XhciBulkHalted))
    {
        StreamStopOrHalt(Ring);
        Bulk->m_Flags |= XhciBulkHalted;
    }
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
}

VOID
XhciBulkRing::StoppedEventReceived(
    _In_ XhciTransferRing* Ring)
{
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    KIRQL OldIrql;

    if (Ring->m_StreamId == 0)
        return;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    if (!(Bulk->m_Flags & XhciBulkStoppedSeen))
    {
        StreamStopOrHalt(Ring);
        Bulk->m_Flags |= XhciBulkStoppedSeen;
    }
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
}

VOID
XhciBulkRing::AllowReclaimOnCancel(
    _In_ XhciTransferRing* Ring)
{
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    BOOLEAN Queue = FALSE;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    if (Bulk->m_Flags & XhciBulkCanceled)
    {
        /* QUIRK: the "OK to reclaim" flag stays clear when a cancel already happened */
        XhciBulkMoveList(&Bulk->m_Completion, &Bulk->m_Pending);
        Queue = TRUE;
    }
    else
    {
        Bulk->m_Flags |= XhciBulkOkToReclaim;
    }
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    if (Queue)
        QueueCompletionDpc(Ring);
}

VOID
XhciBulkRing::ConsumePendingEvents(
    _In_ XhciTransferRing* Ring)
{
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    BOOLEAN Acknowledge = FALSE;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    if (Bulk->m_OutstandingEvents == 0)
        Acknowledge = TRUE;
    else
        Bulk->m_Flags |= XhciBulkAckExpected;
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    if (Acknowledge)
        Ring->m_Endpoint->OnExpectedEventsProcessed();
}

VOID
XhciBulkRing::RecoverTransfers(
    _In_ XhciTransferRing* Ring)
{
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    XhciBulkMoveList(&Bulk->m_Completion, &Bulk->m_Pending);
    Bulk->m_Flags |= XhciBulkReclaimAck;
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    /* Only the DPC can tell that nothing is owned any more */
    QueueCompletionDpc(Ring);
}

BOOLEAN
XhciBulkRing::DoorbellRungSinceFill(
    _In_ const XhciTransferRing* Ring)
{
    return Ring->m_DoorbellRung;
}

BOOLEAN
XhciBulkRing::HasQueuedWork(
    _In_ const XhciTransferRing* Ring)
{
    PKSPIN_LOCK Lock = const_cast<PKSPIN_LOCK>(&Ring->m_Lock);
    BOOLEAN Pending;
    KIRQL OldIrql;

    KeAcquireSpinLock(Lock, &OldIrql);
    Pending = !IsListEmpty(&Ring->m_Type.Bulk.m_Pending);
    KeReleaseSpinLock(Lock, OldIrql);

    return Pending;
}

BOOLEAN
XhciBulkRing::IsLikelyDuplicate(
    _In_ const XhciTransferRing* Ring,
    _In_ const XHCI_TRB* Event)
{
    PKSPIN_LOCK Lock = const_cast<PKSPIN_LOCK>(&Ring->m_Lock);
    BOOLEAN NoOp;
    BOOLEAN Likely;
    KIRQL OldIrql;

    KeAcquireSpinLock(Lock, &OldIrql);
    Likely = LikelyDuplicate(Ring, XhciTrbPointer(Event), &NoOp);
    KeReleaseSpinLock(Lock, OldIrql);

    return Likely;
}

BOOLEAN
XhciBulkRing::OnTransferEvent(
    _In_opt_ XhciTransferRing* Ring,
    _In_ XhciController* Controller,
    _In_ const XHCI_TRB* Event)
{
    UNREFERENCED_PARAMETER(Controller);

    if (Ring == NULL)
    {
        DPRINT1("Bulk transfer event 0x%I64x without a ring, dropped\n", XhciTrbPointer(Event));
        return FALSE;
    }

    if (Event->Dword[3] & XHCI_TRANSFER_EVENT_ED)
        return Ed1Event(Ring, Event);

    return Ed0Event(Ring, Event);
}

VOID
XhciBulkRing::SegmentsArrived(
    _In_ XhciTransferRing* Ring,
    _In_ NTSTATUS Status)
{
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Bulk DCI %lu stream %lu segment growth failed, 0x%08lx\n",
                Ring->m_Endpoint->Dci(), Ring->m_StreamId, Status);
    }

    for (;;)
    {
        /* The loop already left: resume it here, whatever the map state */
        if (InterlockedCompareExchange(&Bulk->m_SegmentWait, XhciBulkWaitIdle, XhciBulkWaitParked) ==
            XhciBulkWaitParked)
        {
            Bulk->m_SegmentWaits++;
            BuildTdsFromQueue(Ring);
            return;
        }

        /* The ring was disabled while waiting */
        if (InterlockedCompareExchange(&Bulk->m_SegmentWait, XhciBulkWaitIdle, XhciBulkWaitAbandoned) ==
            XhciBulkWaitAbandoned)
        {
            return;
        }

        /* The loop has not left yet; it sees this and retries the stage itself */
        if (InterlockedCompareExchange(&Bulk->m_SegmentWait, XhciBulkWaitArrived, XhciBulkWaitIdle) ==
            XhciBulkWaitIdle)
        {
            Bulk->m_SegmentWaits++;
            return;
        }
    }
}

/* Mapping loop **************************************************************/

VOID
XhciBulkRing::BuildTdsFromQueue(
    _In_ XhciTransferRing* Ring)
{
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    KIRQL OldIrql;

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    for (;;)
    {
        FillLoop(Ring);

        if (Ring->m_MapState != static_cast<LONG>(XhciMapState::PausedFill) ||
            Bulk->m_FillWanted == 0)
        {
            break;
        }

        if (Ring->ChangeMapState(FALSE, XhciMapState::PausedFill, XhciMapState::Filling) != XhciMapState::PausedFill)
            break;
    }

    KeLowerIrql(OldIrql);
}

VOID
XhciBulkRing::FillLoop(
    _In_ XhciTransferRing* Ring)
{
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    XhciBulkStage* Stage;
    Outcome Result;
    BOOLEAN Pause = FALSE;

    for (;;)
    {
        InterlockedExchange(&Bulk->m_FillWanted, 0);

        Stage = NextTdSlot(Ring);
        if (Stage == NULL)
        {
            Pause = TRUE;
            break;
        }

        Result = PrimeStage(Ring, Stage);
        if (Result == Outcome::Completed)
            continue;
        if (Result == Outcome::WaitSegments)
            break;
        if (Result == Outcome::Failed)
        {
            Pause = TRUE;
            break;
        }

        Result = EncodeStage(Ring, Stage);
        if (Result == Outcome::Completed)
            continue;
        if (Result == Outcome::WaitDma)
            break;
        if (Result == Outcome::Failed)
        {
            Pause = TRUE;
            break;
        }

        /* Also completes a stop that raced in */
        if (Ring->ChangeMapState(FALSE, XhciMapState::Filling, XhciMapState::Filling) != XhciMapState::Filling)
            break;
    }

    if (Pause)
        Ring->ChangeMapState(FALSE, XhciMapState::Filling, XhciMapState::PausedFill);
}

VOID
XhciBulkRing::KickFill(
    _In_ XhciTransferRing* Ring)
{
    InterlockedExchange(&Ring->m_Type.Bulk.m_FillWanted, 1);

    if (Ring->ChangeMapState(FALSE, XhciMapState::PausedFill, XhciMapState::Filling) == XhciMapState::PausedFill)
        BuildTdsFromQueue(Ring);
}

XhciBulkStage*
XhciBulkRing::NextTdSlot(
    _In_ XhciTransferRing* Ring)
{
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    XhciBulkTransfer* Transfer;
    XhciBulkStage* Stage;
    WDFREQUEST Request;
    BOOLEAN Idle;
    NTSTATUS Status;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);

    if (Bulk->m_PendingStages >= Bulk->m_MaxPendingStages)
    {
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
        return NULL;
    }

    if (!IsListEmpty(&Bulk->m_Pending))
    {
        Transfer = CONTAINING_RECORD(Bulk->m_Pending.Blink, XhciBulkTransfer, Link);

        /* A zero length request has not mapped yet even though its byte count is met */
        if (Transfer->BytesQueued < Transfer->BytesTotal || !Transfer->MappedOnce)
        {
            if (Bulk->m_SegmentWaits != XhciBulkSegmentWaitLimit)
            {
                Stage = AcquireStage(Ring, Transfer);
                KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
                return Stage;
            }

            if (Transfer->TdsQueued != Transfer->TdsRetired)
            {
                /* QUIRK: completes later with the partial length and the hardware status */
                Transfer->BytesQueued = Transfer->BytesTotal;
                KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
                return NULL;
            }

            DPRINT1("Bulk DCI %lu stream %lu gave up waiting for segments\n",
                    Ring->m_Endpoint->Dci(), Ring->m_StreamId);
            CompleteAfterUncancel(Ring, Transfer, USBD_STATUS_INSUFFICIENT_RESOURCES, TRUE);

            /* Reset the count so the next split request waits again */
            Bulk->m_SegmentWaits = 0;
        }
    }

    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    for (;;)
    {
        Status = WdfIoQueueRetrieveNextRequest(Ring->m_Queue, &Request);
        if (!NT_SUCCESS(Status))
            return NULL;

        if (XhciGetRequestData(Request) == NULL)
        {
            DPRINT1("Bulk request %p has no driver context\n", Request);
            WdfRequestComplete(Request, STATUS_INVALID_DEVICE_STATE);
            continue;
        }

        Transfer = &XhciGetRequestData(Request)->Bulk;
        InitTransfer(Ring, Transfer, Request);
        Transfer->MappedOnce = FALSE;
        ChooseMechanism(Ring, Transfer);

        Status = ArrangeBuffer(Ring, Transfer);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Bulk DCI %lu stream %lu request %p has no usable buffer, 0x%08lx\n",
                    Ring->m_Endpoint->Dci(), Ring->m_StreamId, Request, Status);
            Complete(Ring, Transfer, USBD_STATUS_INSUFFICIENT_RESOURCES);
            continue;
        }

        Transfer->CancelState = XhciBulkCancel::Armed;
        Status = WdfRequestMarkCancelableEx(Request, EvtRequestCancel);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Bulk DCI %lu stream %lu request %p canceled before mapping, 0x%08lx\n",
                    Ring->m_Endpoint->Dci(), Ring->m_StreamId, Request, Status);
            Transfer->CancelState = XhciBulkCancel::Done;
            Complete(Ring, Transfer, USBD_STATUS_CANCELED);
            continue;
        }

        break;
    }

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    Idle = (Bulk->m_PendingStages == 0);
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    /* Every stage on the ring shares one event ring so its events stay in order */
    if (Idle)
        Ring->UpdateInterrupterTarget();

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    Stage = AcquireStage(Ring, Transfer);
    InsertTailList(&Bulk->m_Pending, &Transfer->Link);
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    return Stage;
}

_Requires_lock_held_(Ring->m_Lock)
XhciBulkStage*
XhciBulkRing::AcquireStage(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciBulkTransfer* Transfer)
{
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    XhciBulkStage* Stage;
    ULONG Index;

    if (Transfer->Stage != NULL)
        return NULL;

    for (Index = 0; Index < StageSlots; Index++)
    {
        Stage = &Bulk->m_Stages[Index];
        if (Stage->InUse)
            continue;

        Stage->InUse = TRUE;
        Stage->OnHardware = FALSE;
        Stage->FreeMdl = FALSE;
        Stage->Generation++;
        Stage->Transfer = Transfer;
        InitializeListHead(&Stage->FreeSegments);
        InitializeListHead(&Stage->UsedSegments);
        Stage->Size = 0;
        Stage->Mdl = NULL;
        Stage->SgList = NULL;
        Stage->Buffer = NULL;
        Stage->DoubleBuffer = NULL;
        Stage->BurstTrbBudget = 0;
        Stage->TrbsReserved = 0;
        Stage->TrbsWritten = 0;
        Stage->FirstSegment = Ring->m_Segment;
        Stage->FirstIndex = Ring->m_EnqueueIndex;
        Stage->LastSegment = Ring->m_Segment;
        Stage->EndIndex = Ring->m_EnqueueIndex;

        Transfer->Stage = Stage;
        return Stage;
    }

    DPRINT1("Bulk DCI %lu stream %lu ran out of stage records\n",
            Ring->m_Endpoint->Dci(), Ring->m_StreamId);
    return NULL;
}

_Requires_lock_held_(Ring->m_Lock)
VOID
XhciBulkRing::ReleaseStage(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciBulkStage* Stage)
{
    NT_ASSERT(Stage->SgList == NULL);

    if (Stage->FreeMdl)
    {
        IoFreeMdl(Stage->Mdl);
        Stage->FreeMdl = FALSE;
    }
    Stage->Mdl = NULL;

    Ring->ReturnSegments(&Stage->FreeSegments, TRUE);
    Ring->ReturnSegments(&Stage->UsedSegments, FALSE);
    InitializeListHead(&Stage->FreeSegments);
    InitializeListHead(&Stage->UsedSegments);

    if (Stage->Transfer != NULL)
        Stage->Transfer->Stage = NULL;

    Stage->Transfer = NULL;
    Stage->OnHardware = FALSE;
    Stage->InUse = FALSE;
}

_Requires_lock_not_held_(Ring->m_Lock)
VOID
XhciBulkRing::PutScatterGather(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciBulkStage* Stage)
{
    PDMA_ADAPTER Adapter;
    KIRQL OldIrql;

    if (Stage->SgList == NULL)
        return;

    /* Never under the ring lock: the HAL may run another DMA callback of this ring */
    Adapter = Ring->m_Controller->m_Buffers.DmaAdapter();
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    Adapter->DmaOperations->PutScatterGatherList(Adapter,
                                                 Stage->SgList,
                                                 !XhciBulkTransferIn(Stage->Transfer));
    KeLowerIrql(OldIrql);
    Stage->SgList = NULL;
}

/* Request records ***********************************************************/

VOID
XhciBulkRing::InitTransfer(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciBulkTransfer* Transfer,
    _In_ WDFREQUEST Request)
{
    WDF_REQUEST_PARAMETERS Parameters;
    struct _URB_BULK_OR_INTERRUPT_TRANSFER* Urb;

    /* A requeued request keeps where it got to */
    if (Transfer->Initialized == XhciBulkTransferTag)
    {
        NT_ASSERT(Transfer->CancelState == XhciBulkCancel::Idle);
        NT_ASSERT(Transfer->Status == STATUS_PENDING);
        NT_ASSERT(!XhciIsListEntryLinked(&Transfer->Link));
        NT_ASSERT(Transfer->TdsQueued == Transfer->TdsRetired);
        return;
    }

    WDF_REQUEST_PARAMETERS_INIT(&Parameters);
    WdfRequestGetParameters(Request, &Parameters);
    Urb = static_cast<struct _URB_BULK_OR_INTERRUPT_TRANSFER*>(Parameters.Parameters.Others.Arg1);

    RtlZeroMemory(Transfer, sizeof(*Transfer));
    XhciClearListEntry(&Transfer->Link);
    Transfer->Initialized = XhciBulkTransferTag;
    Transfer->Request = Request;
    Transfer->Urb = Urb;
    Transfer->Ring = Ring;
    Transfer->CancelState = XhciBulkCancel::Idle;
    Transfer->Status = STATUS_PENDING;
    Transfer->Mechanism = XhciBulkMechanism::NoData;
    Transfer->BytesTotal = Urb->TransferBufferLength;

    /* QUIRK: the length reads 0 until completion, as the USB 2.0 stack does */
    Urb->TransferBufferLength = 0;
}

VOID
XhciBulkRing::ChooseMechanism(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciBulkTransfer* Transfer)
{
    if (Transfer->BytesTotal == 0)
    {
        Transfer->Mechanism = XhciBulkMechanism::NoData;
        return;
    }

    if (Ring->m_Type.Bulk.m_ImmediateData && Transfer->BytesTotal <= XhciBulkImmediateBytes)
    {
        Transfer->Mechanism = XhciBulkMechanism::Immediate;
        return;
    }

    if (!XhciBulkChainedMdl(Transfer) && Transfer->BytesTotal <= Ring->m_DoubleBufferSize)
    {
        Transfer->DoubleBuffer = Ring->BorrowBounceBuffer();
        if (Transfer->DoubleBuffer != NULL)
        {
            Transfer->Mechanism = XhciBulkMechanism::DoubleBuffer;
            return;
        }
    }

    Transfer->Mechanism = XhciBulkMechanism::Dma;
}

NTSTATUS
XhciBulkRing::ArrangeBuffer(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciBulkTransfer* Transfer)
{
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    struct _URB_BULK_OR_INTERRUPT_TRANSFER* Urb = Transfer->Urb;

    switch (Transfer->Mechanism)
    {
        case XhciBulkMechanism::Immediate:
        case XhciBulkMechanism::DoubleBuffer:
            if (Urb->TransferBuffer != NULL)
            {
                Transfer->Buffer = static_cast<PUCHAR>(Urb->TransferBuffer);
            }
            else if (Urb->TransferBufferMDL != NULL)
            {
                Transfer->Buffer = static_cast<PUCHAR>(
                    MmGetSystemAddressForMdlSafe(Urb->TransferBufferMDL, NormalPagePriority));
            }

            if (Transfer->Buffer == NULL)
            {
                DPRINT1("Bulk DCI %lu stream %lu: no system address for the buffer\n",
                        Ring->m_Endpoint->Dci(), Ring->m_StreamId);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            return STATUS_SUCCESS;

        case XhciBulkMechanism::Dma:
            if (Urb->TransferBufferMDL != NULL)
            {
                Transfer->Mdl = Urb->TransferBufferMDL;
                return STATUS_SUCCESS;
            }

            Transfer->Mdl = IoAllocateMdl(Urb->TransferBuffer, Transfer->BytesTotal, FALSE, FALSE, NULL);
            if (Transfer->Mdl == NULL)
            {
                if (!(Urb->TransferFlags & USB3_URB_RESERVED_RESOURCES) ||
                    Ring->m_ReservedMdl == NULL)
                {
                    DPRINT1("Bulk DCI %lu stream %lu: MDL allocation failed\n",
                            Ring->m_Endpoint->Dci(), Ring->m_StreamId);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                /* The reserved MDL serves one request at a time */
                if (InterlockedCompareExchange(&Bulk->m_ForwardProgressBusy, 1, 0) != 0)
                {
                    DPRINT1("Bulk DCI %lu stream %lu: forward progress MDL already in use\n",
                            Ring->m_Endpoint->Dci(), Ring->m_StreamId);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                Transfer->Mdl = Ring->m_ReservedMdl;
                Transfer->UsesReservedMdl = TRUE;
                MmInitializeMdl(Transfer->Mdl, Urb->TransferBuffer, Transfer->BytesTotal);
            }

            MmBuildMdlForNonPagedPool(Transfer->Mdl);
            return STATUS_SUCCESS;

        default:
            return STATUS_SUCCESS;
    }
}

VOID
XhciBulkRing::DropMdl(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciBulkTransfer* Transfer)
{
    if (Transfer->Mdl == NULL)
        return;

    if (Transfer->UsesReservedMdl)
    {
        Transfer->UsesReservedMdl = FALSE;
        InterlockedExchange(&Ring->m_Type.Bulk.m_ForwardProgressBusy, 0);
    }
    else if (Transfer->Mdl != Transfer->Urb->TransferBufferMDL)
    {
        IoFreeMdl(Transfer->Mdl);
    }

    Transfer->Mdl = NULL;
}

VOID
XhciBulkRing::FreeTransferResources(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciBulkTransfer* Transfer)
{
    NT_ASSERT(Transfer->Stage == NULL);

    if (Transfer->DoubleBuffer != NULL)
    {
        Ring->ReturnBounceBuffer(Transfer->DoubleBuffer);
        Transfer->DoubleBuffer = NULL;
    }

    DropMdl(Ring, Transfer);
    Transfer->Buffer = NULL;
    Transfer->Initialized = 0;
}

/* Stage preparation *********************************************************/

XhciBulkRing::Outcome
XhciBulkRing::PrimeStage(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciBulkStage* Stage)
{
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    XhciBulkTransfer* Transfer = Stage->Transfer;
    ULONG Segments;
    NTSTATUS Status;
    KIRQL OldIrql;

    switch (Transfer->Mechanism)
    {
        case XhciBulkMechanism::Immediate:
            Stage->Buffer = Transfer->Buffer + Transfer->BytesQueued;
            break;

        case XhciBulkMechanism::DoubleBuffer:
            Stage->Buffer = Transfer->Buffer + Transfer->BytesQueued;
            Stage->DoubleBuffer = Transfer->DoubleBuffer;
            break;

        case XhciBulkMechanism::Dma:
            if (!NT_SUCCESS(AcquireStageMdl(Ring, Stage)))
                return FailStage(Ring, Stage);
            break;

        default:
            break;
    }

    SizeStage(Ring, Stage);
    EstimateTrbs(Ring, Stage);

    if (!EstimateSegments(Ring, Stage, &Segments))
        return FailStage(Ring, Stage);

    if (Segments == 0)
        return Outcome::Ready;

    Status = Ring->TakeSegments(Segments, &Stage->FreeSegments);
    if (Status == STATUS_PENDING)
    {
        KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
        ReleaseStage(Ring, Stage);
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

        if (ParkOnSegments(Ring))
            return Outcome::WaitSegments;

        /* The segments already arrived; map the request again from the top */
        return Outcome::Completed;
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Bulk DCI %lu stream %lu: %lu segments not available, 0x%08lx\n",
                Ring->m_Endpoint->Dci(), Ring->m_StreamId, Segments, Status);
        return FailStage(Ring, Stage);
    }

    Bulk->m_SegmentWaits = 0;
    return Outcome::Ready;
}

BOOLEAN
XhciBulkRing::ParkOnSegments(
    _In_ XhciTransferRing* Ring)
{
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    LONG Previous;

    Previous = InterlockedCompareExchange(&Bulk->m_SegmentWait, XhciBulkWaitParked, XhciBulkWaitIdle);
    if (Previous == XhciBulkWaitIdle)
        return TRUE;

    InterlockedExchange(&Bulk->m_SegmentWait, XhciBulkWaitIdle);
    return FALSE;
}

NTSTATUS
XhciBulkRing::AcquireStageMdl(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciBulkStage* Stage)
{
    XhciBulkTransfer* Transfer = Stage->Transfer;
    PMDL Mdl = Transfer->Mdl;
    ULONG Offset = Transfer->BytesQueued;
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
        DPRINT1("Bulk DCI %lu stream %lu: MDL chain shorter than the transfer\n",
                Ring->m_Endpoint->Dci(), Ring->m_StreamId);
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
        DPRINT1("Bulk DCI %lu stream %lu: partial MDL allocation failed\n",
                Ring->m_Endpoint->Dci(), Ring->m_StreamId);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IoBuildPartialMdl(Mdl, Partial, Start, Length);
    Partial->Next = Mdl->Next;
    Stage->Mdl = Partial;
    Stage->FreeMdl = TRUE;
    return STATUS_SUCCESS;
}

VOID
XhciBulkRing::SizeStage(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciBulkStage* Stage)
{
    XhciBulkTransfer* Transfer = Stage->Transfer;
    ULONG Remaining = Transfer->BytesTotal - Transfer->BytesQueued;
    ULONG Limit = Ring->m_MaxStageSize;
    ULONG Registers = 0;
    PDMA_ADAPTER Adapter;
    DMA_TRANSFER_INFO Info;
    PMDL Mdl;
    ULONG Left;

    if (Transfer->Mechanism != XhciBulkMechanism::Dma)
    {
        Stage->Size = Remaining;
        return;
    }

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
                                                                  !XhciBulkTransferIn(Transfer),
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

    Stage->Size = min(Remaining, Limit);
}

VOID
XhciBulkRing::EstimateTrbs(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciBulkStage* Stage)
{
    XhciBulkTransfer* Transfer = Stage->Transfer;
    ULONG Burst;
    ULONG Carry = 0;
    ULONG CarryTrbs = 0;
    ULONG Left;
    PMDL Mdl;

    if (Transfer->Mechanism != XhciBulkMechanism::Dma)
    {
        /* One Normal and one Event Data TRB */
        Stage->TrbsReserved = 2;
        Stage->BurstTrbBudget = 2;
    }
    else
    {
        Burst = min(Ring->m_Endpoint->MaxEsitPayload(), Stage->Size);
        if (Burst == 0)
            Burst = 1;

        Stage->BurstTrbBudget = XhciBulkWorstPages(Burst) + 1;
        Stage->TrbsReserved = 0;

        if (!XhciBulkChainedMdl(Transfer))
        {
            Stage->TrbsReserved = ADDRESS_AND_SIZE_TO_SPAN_PAGES(MmGetMdlVirtualAddress(Stage->Mdl),
                                                                 Stage->Size);
        }
        else
        {
            /* QUIRK: per burst sized fragment estimate; a carried fragment is counted twice */
            for (Mdl = Stage->Mdl, Left = Stage->Size; Mdl != NULL && Left != 0; Mdl = Mdl->Next)
            {
                ULONG Chunk = min(Left, MmGetMdlByteCount(Mdl));
                ULONG Offset = 0;

                Left -= Chunk;
                while (Offset < Chunk)
                {
                    PUCHAR Va = static_cast<PUCHAR>(MmGetMdlVirtualAddress(Mdl)) + Offset;
                    ULONG Piece = Chunk - Offset;
                    ULONG Span;

                    if (Piece + Carry >= Burst)
                    {
                        Span = ADDRESS_AND_SIZE_TO_SPAN_PAGES(Va, Burst - Carry) + CarryTrbs;
                        Offset += Burst - Carry;
                        Carry = 0;
                        CarryTrbs = 0;
                    }
                    else
                    {
                        Span = ADDRESS_AND_SIZE_TO_SPAN_PAGES(Va, Piece);
                        Carry = Piece;
                        CarryTrbs = Span;
                        Offset = Chunk;
                    }

                    Stage->TrbsReserved += Span;

                    if (Span >= Ring->m_LastIndex)
                    {
                        DPRINT1("Bulk DCI %lu stream %lu: a %lu TRB fragment needs a whole segment\n",
                                Ring->m_Endpoint->Dci(), Ring->m_StreamId, Span);
                    }

                    if (Span > Stage->BurstTrbBudget)
                        Stage->BurstTrbBudget = Span;
                }
            }
        }

        Stage->TrbsReserved += 1;
    }

    if (XhciBulkPadSize(Ring, Stage->Size) != 0)
    {
        Stage->BurstTrbBudget += 1;
        Stage->TrbsReserved += 1;
    }
}

BOOLEAN
XhciBulkRing::EstimateSegments(
    _In_ XhciTransferRing* Ring,
    _In_ const XhciBulkStage* Stage,
    _Out_ PULONG Segments)
{
    ULONG Available;
    ULONG Usable;

    *Segments = 0;

    if (Ring->m_Controller->HasErrata(XhciErrata::RingKeepTdInOneSegment))
    {
        /* The TD always starts in a fresh segment */
        *Segments = 1;
    }
    else
    {
        Available = Ring->m_LastIndex - Ring->m_EnqueueIndex;
        Available -= Available % Stage->BurstTrbBudget;

        if (Stage->TrbsReserved > Available)
        {
            Usable = Ring->m_LastIndex - (Ring->m_LastIndex % Stage->BurstTrbBudget);

            /* One burst can need more TRBs than a segment holds */
            if (Usable == 0)
            {
                DPRINT1("Bulk DCI %lu stream %lu: a %lu TRB burst does not fit a segment\n",
                        Ring->m_Endpoint->Dci(), Ring->m_StreamId, Stage->BurstTrbBudget);
                return FALSE;
            }

            *Segments = (Stage->TrbsReserved - Available + Usable - 1) / Usable;
        }
    }

    if (XhciBulkNeedsNoOps(Ring))
        *Segments += 1;

    return TRUE;
}

/** Drops a stage that cannot be mapped; completes its request when nothing of it is on the ring. */
XhciBulkRing::Outcome
XhciBulkRing::FailStage(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciBulkStage* Stage)
{
    XhciBulkTransfer* Transfer = Stage->Transfer;
    Outcome Result;
    KIRQL OldIrql;

    PutScatterGather(Ring, Stage);

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    ReleaseStage(Ring, Stage);

    if (Transfer->TdsQueued == Transfer->TdsRetired)
    {
        CompleteAfterUncancel(Ring, Transfer, USBD_STATUS_INSUFFICIENT_RESOURCES, TRUE);
        Result = Outcome::Completed;
    }
    else
    {
        Transfer->BytesQueued = Transfer->BytesTotal;
        Result = Outcome::Failed;
    }

    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
    return Result;
}

XhciBulkRing::Outcome
XhciBulkRing::EncodeStage(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciBulkStage* Stage)
{
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    PDMA_ADAPTER Adapter;
    NTSTATUS Status;
    KIRQL OldIrql;

    if (Stage->Transfer->Mechanism != XhciBulkMechanism::Dma)
    {
        if (!BuildTd(Ring, Stage))
            return FailStage(Ring, Stage);
        return Outcome::Ready;
    }

    Adapter = Ring->m_Controller->m_Buffers.DmaAdapter();

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    Status = Adapter->DmaOperations->GetScatterGatherList(Adapter,
                                                          WdfDeviceWdmGetDeviceObject(Ring->m_Controller->m_Device),
                                                          Stage->Mdl,
                                                          MmGetMdlVirtualAddress(Stage->Mdl),
                                                          Stage->Size,
                                                          DmaCallback,
                                                          Stage,
                                                          !XhciBulkTransferIn(Stage->Transfer));
    KeLowerIrql(OldIrql);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Bulk DCI %lu stream %lu: GetScatterGatherList failed, 0x%08lx\n",
                Ring->m_Endpoint->Dci(), Ring->m_StreamId, Status);
        return FailStage(Ring, Stage);
    }

    /* Whichever of this context and the callback gets here second keeps mapping */
    if (InterlockedXor(&Bulk->m_FillAgain, 1) == 1)
        return Outcome::Ready;

    return Outcome::WaitDma;
}

VOID
NTAPI
XhciBulkRing::DmaCallback(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PSCATTER_GATHER_LIST ScatterGather,
    _In_ PVOID Context)
{
    XhciBulkStage* Stage = static_cast<XhciBulkStage*>(Context);
    XhciTransferRing* Ring = Stage->Ring;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    Stage->SgList = ScatterGather;
    if (!BuildTd(Ring, Stage))
        (VOID)FailStage(Ring, Stage);

    if (InterlockedXor(&Ring->m_Type.Bulk.m_FillAgain, 1) == 1)
    {
        if (Ring->ChangeMapState(FALSE, XhciMapState::Filling, XhciMapState::Filling) == XhciMapState::Filling)
            BuildTdsFromQueue(Ring);
    }
}

/* TD building ***************************************************************/

/**
 * Writes the TD of a stage and hands it to the controller. On failure the ring is put
 * back as it was and the caller fails the stage.
 */
BOOLEAN
XhciBulkRing::BuildTd(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciBulkStage* Stage)
{
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    XhciBulkTransfer* Transfer = Stage->Transfer;
    XhciDmaBuffer* PadBuffer = Ring->m_Controller->m_SplitPadBuffer;
    PSCATTER_GATHER_LIST SgList = Stage->SgList;
    ULONG PadBytes = XhciBulkPadSize(Ring, Stage->Size);
    BOOLEAN NoOps = XhciBulkNeedsNoOps(Ring);
    ULONG Packets = Ring->PacketCount(Stage->Size);
    ULONG Pending = Stage->BurstTrbBudget;
    ULONG Mapped = 0;
    ULONG Element = 0;
    ULONG64 ElementAddress = 0;
    ULONG ElementLeft = 0;
    XhciBulkStep Step = XhciBulkStep::Normal;
    PXHCI_TRB First = &Ring->m_Trbs[Ring->m_EnqueueIndex];
    volatile UCHAR* CycleByte;
    XHCI_TRB Trb;
    ULONG64 Value;
    ULONG Chunk;
    ULONG Index;
    BOOLEAN IsFirst;
    KIRQL OldIrql;

    Stage->TrbsWritten = 0;

    if (Ring->m_Controller->HasErrata(XhciErrata::RingKeepTdInOneSegment) && !InsertLink(Ring, Stage, TRUE))
    {
        UndoTd(Ring, Stage);
        return FALSE;
    }

    while (Step != XhciBulkStep::Done)
    {
        IsFirst = (&Ring->m_Trbs[Ring->m_EnqueueIndex] == First);

        /* Keep each burst sized piece of the TD inside one segment (xHCI 4.11.7.1) */
        if (Pending + Ring->m_EnqueueIndex > Ring->m_LastIndex ||
            (NoOps && Step == XhciBulkStep::EventData &&
             Ring->m_EnqueueIndex + XhciBulkNoOpCount + 1 > Ring->m_LastIndex))
        {
            /* A failed insert fails the TD instead of retrying */
            if (!InsertLink(Ring, Stage, IsFirst))
            {
                UndoTd(Ring, Stage);
                return FALSE;
            }
            continue;
        }

        RtlZeroMemory(&Trb, sizeof(Trb));

        if (Step == XhciBulkStep::Normal)
        {
            Trb.Dword[3] = static_cast<ULONG>(XhciTrbType::Normal) << XHCI_TRB_TYPE_SHIFT;

            switch (Transfer->Mechanism)
            {
                case XhciBulkMechanism::Immediate:
                    RtlCopyMemory(&Trb.Dword[0], Stage->Buffer, Stage->Size);
                    Trb.Dword[3] |= XHCI_TRB_IDT;
                    Chunk = Stage->Size;
                    Step = XhciBulkStep::EventData;
                    break;

                case XhciBulkMechanism::DoubleBuffer:
                    if (!XhciBulkTransferIn(Transfer))
                        RtlCopyMemory(Stage->DoubleBuffer->VirtualAddress, Stage->Buffer, Stage->Size);
                    Trb.Dword[0] = Stage->DoubleBuffer->LogicalAddress.LowPart;
                    Trb.Dword[1] = (ULONG)Stage->DoubleBuffer->LogicalAddress.HighPart;
                    Chunk = Stage->Size;
                    Step = XhciBulkStep::EventData;
                    break;

                case XhciBulkMechanism::Dma:
                    if (ElementLeft == 0)
                    {
                        ElementAddress = (ULONG64)SgList->Elements[Element].Address.QuadPart;
                        ElementLeft = SgList->Elements[Element].Length;
                        Element++;
                    }

                    /* A TRB buffer may not cross a 64 KB boundary (xHCI 6.4.1.1) */
                    Chunk = min(ElementLeft, XhciBulkTrbBytesMax - (ULONG)(ElementAddress & 0xFFFF));
                    Trb.Dword[0] = (ULONG)ElementAddress;
                    Trb.Dword[1] = (ULONG)(ElementAddress >> 32);
                    ElementAddress += Chunk;
                    ElementLeft -= Chunk;

                    if (ElementLeft == 0 && Element >= SgList->NumberOfElements)
                        Step = XhciBulkStep::EventData;
                    break;

                default:
                    Chunk = 0;
                    Step = XhciBulkStep::EventData;
                    break;
            }

            Mapped += Chunk;
            Trb.Dword[2] = Chunk |
                           (Ring->TdSize(Packets, Mapped, Step == XhciBulkStep::EventData) << XHCI_TRB_TD_SIZE_SHIFT);
            Trb.Dword[3] |= XHCI_TRB_CHAIN;
            Pending--;
        }
        else if (Step == XhciBulkStep::Pad)
        {
            Trb.Dword[0] = PadBuffer->LogicalAddress.LowPart;
            Trb.Dword[1] = (ULONG)PadBuffer->LogicalAddress.HighPart;
            Trb.Dword[2] = PadBytes;
            Trb.Dword[3] = (static_cast<ULONG>(XhciTrbType::Normal) << XHCI_TRB_TYPE_SHIFT) | XHCI_TRB_CHAIN;
            Pending--;
            PadBytes = 0;
            Step = XhciBulkStep::EventData;
        }
        else
        {
            Value = XhciBulkEventData(Ring, Stage);
            Trb.Dword[0] = (ULONG)Value;
            Trb.Dword[1] = (ULONG)(Value >> 32);
            Trb.Dword[3] = (static_cast<ULONG>(XhciTrbType::EventData) << XHCI_TRB_TYPE_SHIFT) | XHCI_TRB_IOC;
            Step = XhciBulkStep::Done;
        }

        Stage->TrbsWritten++;

        if (Step == XhciBulkStep::EventData)
        {
            if (PadBytes != 0 && PadBuffer != NULL)
            {
                Trb.Dword[2] += 1 << XHCI_TRB_TD_SIZE_SHIFT;
                Step = XhciBulkStep::Pad;

                /* The pad still needs a slot when the burst is used up */
                if (Pending == 0)
                    Pending = 1;
            }
            else
            {
                /* xHCI 4.12.3: let a flow controlled ring reach the Event Data TRB */
                Trb.Dword[3] |= XHCI_TRB_ENT;
                Pending = 1;
            }
        }
        else if (Step == XhciBulkStep::Normal && Pending == 0)
        {
            Pending = (Stage->TrbsReserved > Stage->TrbsWritten) ?
                      min(Stage->BurstTrbBudget, Stage->TrbsReserved - Stage->TrbsWritten) :
                      Stage->BurstTrbBudget;
        }

        /* The first slot stays invalid until the whole TD is written */
        Trb.Dword[3] |= IsFirst ? (Ring->m_Cycle ^ XHCI_TRB_CYCLE) : Ring->m_Cycle;
        Trb.Dword[2] |= Ring->m_InterrupterTarget << XHCI_TRB_INTERRUPTER_SHIFT;

        Ring->m_Trbs[Ring->m_EnqueueIndex] = Trb;
        Ring->m_EnqueueIndex++;

        if (Step == XhciBulkStep::Done && NoOps)
        {
            for (Index = 0; Index < XhciBulkNoOpCount; Index++)
            {
                RtlZeroMemory(&Trb, sizeof(Trb));
                Trb.Dword[3] = (static_cast<ULONG>(XhciTrbType::NoOp) << XHCI_TRB_TYPE_SHIFT) | Ring->m_Cycle;
                Ring->m_Trbs[Ring->m_EnqueueIndex] = Trb;
                Ring->m_EnqueueIndex++;
            }
        }
    }

    NT_ASSERT(Stage->TrbsWritten <= Stage->TrbsReserved);

    /* Recycled segments are not cleared, so the slot after the TD must be */
    Stage->LastSegment = Ring->m_Segment;
    Stage->EndIndex = Ring->m_EnqueueIndex;
    RtlZeroMemory(&Ring->m_Trbs[Ring->m_EnqueueIndex], sizeof(XHCI_TRB));

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    Transfer->TdsQueued++;
    Transfer->BytesQueued += Stage->Size;
    Transfer->MappedOnce = TRUE;
    Bulk->m_PendingStages++;
    Stage->OnHardware = TRUE;
    Ring->m_DoorbellRung = TRUE;
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    /* A one byte register write publishes the TD after every store above */
    KeMemoryBarrier();
    CycleByte = reinterpret_cast<volatile UCHAR*>(&First->Dword[3]);
    WRITE_REGISTER_UCHAR(const_cast<PUCHAR>(CycleByte), (UCHAR)(*CycleByte ^ XHCI_TRB_CYCLE));
    KeMemoryBarrier();

    Ring->RingDoorbell();
    return TRUE;
}

/** Ends the current segment with a Link TRB into the next reserved one. */
BOOLEAN
XhciBulkRing::InsertLink(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciBulkStage* Stage,
    _In_ BOOLEAN FirstOfTd)
{
    XhciDmaBuffer* Next;
    XHCI_TRB Trb;
    KIRQL OldIrql;

    if (IsListEmpty(&Stage->FreeSegments))
    {
        DPRINT1("Bulk DCI %lu stream %lu: no reserved segment left for a Link TRB\n",
                Ring->m_Endpoint->Dci(), Ring->m_StreamId);
        return FALSE;
    }

    Next = CONTAINING_RECORD(RemoveHeadList(&Stage->FreeSegments), XhciDmaBuffer, Link);
    XhciClearListEntry(&Next->Link);

    /* Chained even ahead of the pad TRB */
    Ring->BuildLinkTrb(&Trb, FirstOfTd, TRUE);
    Trb.Dword[0] = Next->LogicalAddress.LowPart;
    Trb.Dword[1] = (ULONG)Next->LogicalAddress.HighPart;
    Ring->m_Trbs[Ring->m_EnqueueIndex] = Trb;

    /* The ring never toggles its cycle state: it is an endless forward chain of segments */
    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    InsertTailList(&Stage->UsedSegments, &Ring->m_Segment->Link);
    Ring->m_Segment = Next;
    Ring->m_Trbs = static_cast<PXHCI_TRB>(Next->VirtualAddress);
    Ring->m_EnqueueIndex = 0;
    Stage->LastSegment = Next;
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    return TRUE;
}

/** Rewinds the ring to where a TD that could not be finished started. */
VOID
XhciBulkRing::UndoTd(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciBulkStage* Stage)
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);

    if (Ring->m_Segment != Stage->FirstSegment)
    {
        RemoveEntryList(&Stage->FirstSegment->Link);
        XhciClearListEntry(&Stage->FirstSegment->Link);
        InsertTailList(&Stage->FreeSegments, &Ring->m_Segment->Link);
        Ring->m_Segment = Stage->FirstSegment;
        Ring->m_Trbs = static_cast<PXHCI_TRB>(Stage->FirstSegment->VirtualAddress);
    }

    /* The first slot never became valid, so the controller cannot be past it */
    Ring->m_EnqueueIndex = Stage->FirstIndex;
    RtlZeroMemory(&Ring->m_Trbs[Stage->FirstIndex], sizeof(XHCI_TRB));
    Stage->LastSegment = Stage->FirstSegment;
    Stage->EndIndex = Stage->FirstIndex;

    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
}

/* Completion ****************************************************************/

_Requires_lock_held_(Ring->m_Lock)
VOID
XhciBulkRing::CompleteAfterUncancel(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciBulkTransfer* Transfer,
    _In_ USBD_STATUS Default,
    _In_ BOOLEAN FinishCanceled)
{
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    NTSTATUS Status;

    switch (Transfer->CancelState)
    {
        case XhciBulkCancel::Idle:
            break;

        case XhciBulkCancel::Armed:
            if (XhciIsListEntryLinked(&Transfer->Link))
            {
                RemoveEntryList(&Transfer->Link);
                XhciClearListEntry(&Transfer->Link);
            }

            Status = WdfRequestUnmarkCancelable(Transfer->Request);
            if (!NT_SUCCESS(Status))
            {
                /* The cancel callback is about to run and finishes the job */
                Transfer->CancelState = XhciBulkCancel::CancelRoutinePending;
                InsertTailList(&Bulk->m_WaitingForCancel, &Transfer->Link);
                return;
            }
            Transfer->CancelState = XhciBulkCancel::Idle;
            break;

        case XhciBulkCancel::Done:
            if (!FinishCanceled)
                return;
            if (Default == XhciBulkStatusNotSet)
                Default = USBD_STATUS_CANCELED;
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
    Complete(Ring, Transfer, Default);
    KeAcquireSpinLockAtDpcLevel(&Ring->m_Lock);
}

_Requires_lock_not_held_(Ring->m_Lock)
VOID
XhciBulkRing::Complete(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciBulkTransfer* Transfer,
    _In_ USBD_STATUS Default)
{
    struct _URB_BULK_OR_INTERRUPT_TRANSFER* Urb = Transfer->Urb;
    WDFREQUEST Request = Transfer->Request;
    USBD_STATUS UsbdStatus;
    NTSTATUS Status;

    NT_ASSERT(Transfer->CancelState == XhciBulkCancel::Idle ||
              Transfer->CancelState == XhciBulkCancel::Done);

    UsbdStatus = XhciTransferRing::UsbdStatusFromCompletionCode(Transfer->CompletionCode);
    if (Transfer->CompletionCode == 0 || UsbdStatus == XhciBulkStatusNotSet)
        UsbdStatus = Default;

    Urb->TransferBufferLength = Transfer->BytesTransferred;
    Urb->Hdr.Status = UsbdStatus;
    Status = XhciTransferRing::NtStatusFromUsbdStatus(UsbdStatus);
    Transfer->Status = Status;

    if (!NT_SUCCESS(Status) && Status != STATUS_CANCELLED)
    {
        DPRINT1("Bulk DCI %lu stream %lu request %p failed, 0x%08lx USBD 0x%08lx code %lu, %lu bytes\n",
                Ring->m_Endpoint->Dci(), Ring->m_StreamId, Request, Status, UsbdStatus,
                Transfer->CompletionCode, Transfer->BytesTransferred);
    }

    FreeTransferResources(Ring, Transfer);
    WdfRequestComplete(Request, Status);
}

VOID
XhciBulkRing::CompleteBatch(
    _In_ XhciTransferRing* Ring,
    _Inout_ PLIST_ENTRY Batch)
{
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    XhciBulkTransfer* Transfer;
    XhciBulkStage* Stage;
    LIST_ENTRY Canceled;
    LIST_ENTRY Finished;
    LIST_ENTRY Requeue;
    BOOLEAN OnHardware;
    NTSTATUS Status;
    KIRQL OldIrql;

    InitializeListHead(&Canceled);
    InitializeListHead(&Finished);
    InitializeListHead(&Requeue);

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);

    while (!IsListEmpty(Batch))
    {
        Transfer = CONTAINING_RECORD(RemoveHeadList(Batch), XhciBulkTransfer, Link);
        XhciClearListEntry(&Transfer->Link);

        Stage = Transfer->Stage;
        if (Stage != NULL)
        {
            KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
            PutScatterGather(Ring, Stage);
            KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);

            OnHardware = Stage->OnHardware;
            ReleaseStage(Ring, Stage);
            if (OnHardware)
            {
                Transfer->TdsRetired++;
                Bulk->m_PendingStages--;
            }
        }

        if (Transfer->CancelState == XhciBulkCancel::Done)
        {
            InsertTailList(&Canceled, &Transfer->Link);
            continue;
        }

        if (Transfer->CancelState == XhciBulkCancel::Armed)
        {
            Status = WdfRequestUnmarkCancelable(Transfer->Request);
            if (!NT_SUCCESS(Status))
            {
                Transfer->CancelState = XhciBulkCancel::CancelRoutinePending;
                InsertTailList(&Bulk->m_WaitingForCancel, &Transfer->Link);
                continue;
            }
            Transfer->CancelState = XhciBulkCancel::Idle;
        }

        if (Transfer->CompletionCode == static_cast<ULONG>(XhciCompletionCode::Success) ||
            Transfer->CompletionCode == static_cast<ULONG>(XhciCompletionCode::StoppedShortPacket))
        {
            InsertTailList(&Finished, &Transfer->Link);
        }
        else
        {
            InsertTailList(&Requeue, &Transfer->Link);
        }
    }

    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    while (!IsListEmpty(&Canceled))
    {
        Transfer = CONTAINING_RECORD(RemoveHeadList(&Canceled), XhciBulkTransfer, Link);
        XhciClearListEntry(&Transfer->Link);

        if (Ring->m_StreamId != 0 && Transfer->CompletionCode == 0)
            Transfer->CompletionCode = Ring->m_Endpoint->StreamHaltedCode();

        Complete(Ring, Transfer, USBD_STATUS_CANCELED);
    }

    while (!IsListEmpty(&Finished))
    {
        Transfer = CONTAINING_RECORD(RemoveHeadList(&Finished), XhciBulkTransfer, Link);
        XhciClearListEntry(&Transfer->Link);
        Complete(Ring, Transfer, XhciBulkStatusNotSet);
    }

    /* From the tail, so the oldest request ends up at the head of the queue */
    while (!IsListEmpty(&Requeue))
    {
        Transfer = CONTAINING_RECORD(RemoveTailList(&Requeue), XhciBulkTransfer, Link);
        XhciClearListEntry(&Transfer->Link);

        /* It resumes where the hardware stopped */
        Transfer->BytesQueued = Transfer->BytesTransferred;

        if (Transfer->DoubleBuffer != NULL)
        {
            Ring->ReturnBounceBuffer(Transfer->DoubleBuffer);
            Transfer->DoubleBuffer = NULL;
        }
        DropMdl(Ring, Transfer);

        Status = WdfRequestRequeue(Transfer->Request);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Bulk DCI %lu stream %lu could not requeue request %p, 0x%08lx\n",
                    Ring->m_Endpoint->Dci(), Ring->m_StreamId, Transfer->Request, Status);
            Complete(Ring, Transfer, USBD_STATUS_CANCELED);
        }
    }
}

VOID
XhciBulkRing::QueueCompletionDpc(
    _In_ XhciTransferRing* Ring)
{
    WdfDpcEnqueue(Ring->m_CompletionDpc);
}

_Requires_lock_held_(Ring->m_Lock)
BOOLEAN
XhciBulkRing::OwnsRequests(
    _In_ const XhciTransferRing* Ring)
{
    const XhciBulkRing* Bulk = &Ring->m_Type.Bulk;

    return !IsListEmpty(&Bulk->m_Pending) ||
           !IsListEmpty(&Bulk->m_WaitingForCancel) ||
           !IsListEmpty(&Bulk->m_Completion);
}

VOID
NTAPI
XhciBulkRing::EvtRequestCancel(
    _In_ WDFREQUEST Request)
{
    XhciBulkTransfer* Transfer = &XhciGetRequestData(Request)->Bulk;
    XhciTransferRing* Ring = Transfer->Ring;
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    BOOLEAN Queue = FALSE;
    BOOLEAN Report = FALSE;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    if (Transfer->CancelState == XhciBulkCancel::Armed)
    {
        if (Bulk->m_Flags & XhciBulkOkToReclaim)
        {
            /* QUIRK: everything pending goes; requests that were not canceled get requeued */
            XhciBulkMoveList(&Bulk->m_Completion, &Bulk->m_Pending);
            Queue = TRUE;
        }
        else
        {
            Bulk->m_Flags |= XhciBulkCanceled;
            Report = TRUE;
        }
    }
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    if (Report)
        Ring->m_Endpoint->OnTransferCanceled();

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    if (Transfer->CancelState == XhciBulkCancel::CancelRoutinePending)
    {
        RemoveEntryList(&Transfer->Link);
        InsertTailList(&Bulk->m_Completion, &Transfer->Link);
        Queue = TRUE;
    }
    Transfer->CancelState = XhciBulkCancel::Done;
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    if (Queue)
        QueueCompletionDpc(Ring);
}

/* Transfer events ***********************************************************/

_Requires_lock_held_(Ring->m_Lock)
BOOLEAN
XhciBulkRing::IsLiveStage(
    _In_ const XhciTransferRing* Ring,
    _In_ const XhciBulkStage* Stage,
    _In_ ULONG64 EventData)
{
    const XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    const LIST_ENTRY* Entry;
    ULONG Index;

    UNREFERENCED_PARAMETER(EventData);

    for (Index = 0; Index < StageSlots; Index++)
    {
        if (&Bulk->m_Stages[Index] == Stage)
            break;
    }

    if (Index == StageSlots || !Stage->InUse || !Stage->OnHardware || Stage->Transfer == NULL)
        return FALSE;

#ifndef _WIN64
    if ((ULONG)(EventData >> 32) != Stage->Generation)
        return FALSE;
#endif

    /* Any pending request's stage qualifies, not only the oldest */
    for (Entry = Bulk->m_Pending.Flink; Entry != &Bulk->m_Pending; Entry = Entry->Flink)
    {
        if (CONTAINING_RECORD(Entry, XhciBulkTransfer, Link)->Stage == Stage)
            return TRUE;
    }

    return FALSE;
}

BOOLEAN
XhciBulkRing::Ed1Event(
    _In_ XhciTransferRing* Ring,
    _In_ const XHCI_TRB* Event)
{
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    ULONG64 Value = XhciTrbPointer(Event);
    XhciBulkStage* Stage = XhciBulkStageFromEventData(Value);
    XhciBulkTransfer* Transfer;
    XhciBulkStage* Match;
    ULONG Code = XhciTrbCompletionCode(Event);
    ULONG Bytes = Event->Dword[2] & XHCI_TRANSFER_EVENT_LENGTH_MASK;
    ULONG EndpointId = (Event->Dword[3] & XHCI_TRB_ENDPOINT_MASK) >> XHCI_TRB_ENDPOINT_SHIFT;
    BOOLEAN Acknowledge = FALSE;
    BOOLEAN Live;
    ULONG64 Address;
    ULONG Skipped;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    Live = IsLiveStage(Ring, Stage, Value);
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    if (!Live)
    {
        /* A stale or unknown Event Data pointer is dropped, never dereferenced */
        if (Ring->m_StreamId == 0)
        {
            DPRINT1("Bulk DCI %lu: stale or unknown Event Data 0x%I64x code %lu, dropped\n",
                    Ring->m_Endpoint->Dci(), Value, Code);
        }

        KickFill(Ring);
        return (Ring->m_StreamId == 0);
    }

    Transfer = Stage->Transfer;

    if (EndpointId != Ring->m_Endpoint->Dci() || XhciTrbSlotId(Event) != Ring->m_Device->SlotId())
    {
        DPRINT1("Bulk event for slot %lu DCI %lu reached slot %lu DCI %lu\n",
                XhciTrbSlotId(Event), EndpointId, Ring->m_Device->SlotId(), Ring->m_Endpoint->Dci());
    }

    if (Bytes > Stage->Size)
    {
        DPRINT1("Bulk DCI %lu stream %lu: %lu bytes reported for a %lu byte stage\n",
                Ring->m_Endpoint->Dci(), Ring->m_StreamId, Bytes, Stage->Size);
        Bytes = 0;
    }

    if (XhciBulkIsHaltedCode(Code))
    {
        DPRINT1("Bulk DCI %lu stream %lu: halted code %lu reported through Event Data\n",
                Ring->m_Endpoint->Dci(), Ring->m_StreamId, Code);
        HaltedEvent(Ring, Stage, Code, Bytes);
    }
    else if (XhciBulkIsStoppedCode(Code))
    {
        /* QUIRK: the drop stopped event flag is not consulted for Event Data events */
        Address = (ULONG64)Stage->FirstSegment->LogicalAddress.QuadPart +
                  (ULONG64)Stage->FirstIndex * sizeof(XHCI_TRB);

        KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
        if (LookupStage(Ring, Address, &Match, &Skipped) && Match == Stage)
            StoppedEvent(Ring, Stage, Code, Bytes, Skipped);
        else
            NT_ASSERT(FALSE);
        Bulk->m_Flags |= XhciBulkStoppedSeen;
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

        Ring->m_Endpoint->OnStoppedEvent();
    }
    else
    {
        /* Only a short packet or a fully queued request records the code, so a reclaim cannot finish a partial one as Success */
        if (Code == static_cast<ULONG>(XhciCompletionCode::ShortPacket) ||
            Transfer->BytesQueued == Transfer->BytesTotal)
        {
            Transfer->CompletionCode = Code;
        }

        Transfer->BytesTransferred += Bytes;
        XhciBulkCopyIn(Transfer, Stage, Bytes);
        PutScatterGather(Ring, Stage);

        KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
        ReleaseStage(Ring, Stage);
        Transfer->TdsRetired++;

        if (Code == static_cast<ULONG>(XhciCompletionCode::ShortPacket) ||
            Transfer->BytesQueued == Transfer->BytesTotal)
        {
            CompleteAfterUncancel(Ring, Transfer, XhciBulkStatusNotSet, FALSE);
        }

        Bulk->m_PendingStages--;
        if (Bulk->m_OutstandingEvents != 0)
        {
            Bulk->m_OutstandingEvents--;
            if (Bulk->m_OutstandingEvents == 0 && (Bulk->m_Flags & XhciBulkAckExpected))
                Acknowledge = TRUE;
        }
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

        if (Acknowledge)
            Ring->m_Endpoint->OnExpectedEventsProcessed();
    }

    /* A slot on the ring freed up */
    KickFill(Ring);
    return TRUE;
}

BOOLEAN
XhciBulkRing::Ed0Event(
    _In_ XhciTransferRing* Ring,
    _In_ const XHCI_TRB* Event)
{
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    ULONG Code = XhciTrbCompletionCode(Event);
    XhciBulkStage* Stage;
    ULONG Skipped;
    ULONG Bytes = 0;
    KIRQL OldIrql;

    if (XhciBulkIsStoppedCode(Code) && Ring->m_Endpoint->ShouldDropStoppedEvent())
        return TRUE;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);

    if (!LookupStage(Ring, XhciTrbPointer(Event), &Stage, &Skipped))
    {
        if (Ring->m_StreamId == 0)
            Ed0Mismatch(Ring, Event);
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
        return FALSE;
    }

    if (Stage != NULL)
    {
        Bytes = XhciBulkStageBytes(Stage, Event);
        if (Bytes > Stage->Size)
        {
            DPRINT1("Bulk DCI %lu stream %lu: %lu bytes found in a %lu byte stage\n",
                    Ring->m_Endpoint->Dci(), Ring->m_StreamId, Bytes, Stage->Size);
            Bytes = 0;
        }
    }

    if (XhciBulkIsHaltedCode(Code))
    {
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

        /* QUIRK: old controllers report a halt one past the last TRB; that event is dropped */
        if (Stage != NULL)
            HaltedEvent(Ring, Stage, Code, Bytes);
        else
            DPRINT("Bulk DCI %lu: halt reported at the enqueue pointer, dropped\n", Ring->m_Endpoint->Dci());

        return TRUE;
    }

    if (XhciBulkIsStoppedCode(Code))
    {
        StoppedEvent(Ring, Stage, Code, Bytes, Skipped);
        Bulk->m_Flags |= XhciBulkStoppedSeen;
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

        Ring->m_Endpoint->OnStoppedEvent();
        return TRUE;
    }

    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
    DPRINT1("Bulk DCI %lu stream %lu: unexpected code %lu without Event Data\n",
            Ring->m_Endpoint->Dci(), Ring->m_StreamId, Code);
    return FALSE;
}

/** Finds the stage holding a TRB address; Skipped counts the stages before it. */
_Requires_lock_held_(Ring->m_Lock)
BOOLEAN
XhciBulkRing::LookupStage(
    _In_ const XhciTransferRing* Ring,
    _In_ ULONG64 Address,
    _Outptr_result_maybenull_ XhciBulkStage** Stage,
    _Out_ PULONG Skipped)
{
    const XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    const LIST_ENTRY* Entry;
    ULONG Count = 0;

    *Stage = NULL;
    *Skipped = 0;

    if (Address == 0)
        return FALSE;

    if (Address == Ring->EnqueuePointer())
    {
        *Skipped = Bulk->m_PendingStages;
        return TRUE;
    }

    for (Entry = Bulk->m_Pending.Flink; Entry != &Bulk->m_Pending; Entry = Entry->Flink)
    {
        XhciBulkStage* Candidate = CONTAINING_RECORD(Entry, XhciBulkTransfer, Link)->Stage;

        if (Candidate == NULL || !Candidate->OnHardware)
            continue;

        if (XhciBulkStageContains(Candidate, Address))
        {
            *Stage = Candidate;
            *Skipped = Count;
            return TRUE;
        }

        Count++;
    }

    return FALSE;
}

VOID
XhciBulkRing::HaltedEvent(
    _In_ XhciTransferRing* Ring,
    _Inout_ XhciBulkStage* Stage,
    _In_ ULONG Code,
    _In_ ULONG Bytes)
{
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    XhciBulkTransfer* Transfer = Stage->Transfer;
    BOOLEAN Acknowledge = FALSE;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    Bulk->m_Flags |= XhciBulkHalted;
    Bulk->m_OutstandingEvents = 1;
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    /* The machine must see the halt before the client sees the failed request */
    Ring->m_Endpoint->OnHaltedCompletionCode(Code, TRUE);

    Transfer->CompletionCode = Code;
    Transfer->BytesTransferred += Bytes;
    XhciBulkCopyIn(Transfer, Stage, Bytes);
    PutScatterGather(Ring, Stage);

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    ReleaseStage(Ring, Stage);
    Transfer->TdsRetired++;
    CompleteAfterUncancel(Ring, Transfer, XhciBulkStatusNotSet, FALSE);
    Bulk->m_PendingStages--;
    if (Bulk->m_OutstandingEvents != 0)
        Bulk->m_OutstandingEvents--;
    if (Bulk->m_Flags & XhciBulkAckExpected)
        Acknowledge = TRUE;
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    if (Acknowledge)
        Ring->m_Endpoint->OnExpectedEventsProcessed();
}

_Requires_lock_held_(Ring->m_Lock)
VOID
XhciBulkRing::StoppedEvent(
    _In_ XhciTransferRing* Ring,
    _Inout_opt_ XhciBulkStage* Stage,
    _In_ ULONG Code,
    _In_ ULONG Bytes,
    _In_ ULONG Skipped)
{
    XhciBulkTransfer* Transfer;

    if (Stage != NULL)
    {
        Transfer = Stage->Transfer;
        Transfer->BytesTransferred += Bytes;
        XhciBulkCopyIn(Transfer, Stage, Bytes);

        if (Transfer->BytesTransferred == Transfer->BytesTotal)
            Transfer->CompletionCode = static_cast<ULONG>(XhciCompletionCode::Success);
        else if (Code == static_cast<ULONG>(XhciCompletionCode::StoppedShortPacket))
            Transfer->CompletionCode = Code;
    }

    /* Earlier stages finished but their events are still on the way */
    Ring->m_Type.Bulk.m_OutstandingEvents = Skipped;
}

_Requires_lock_held_(Ring->m_Lock)
BOOLEAN
XhciBulkRing::LikelyDuplicate(
    _In_ const XhciTransferRing* Ring,
    _In_ ULONG64 Address,
    _Out_ PBOOLEAN PointsToNoOp)
{
    const XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    const XhciDmaBuffer* Segment = NULL;
    const XhciDmaBuffer* Candidate;
    const LIST_ENTRY* Entry;
    const XHCI_TRB* Trb;

    *PointsToNoOp = FALSE;

    /* The segment most recently given back to the pool */
    if (!IsListEmpty(&Ring->m_FreeSegments))
    {
        Candidate = CONTAINING_RECORD(Ring->m_FreeSegments.Blink, XhciDmaBuffer, Link);
        if (XhciBulkSegmentHas(Candidate, 0, MAXULONG, Address))
            Segment = Candidate;
    }

    if (Segment == NULL)
    {
        Candidate = Ring->m_Segment;
        for (Entry = Bulk->m_Pending.Flink; Entry != &Bulk->m_Pending; Entry = Entry->Flink)
        {
            const XhciBulkStage* Oldest = CONTAINING_RECORD(Entry, XhciBulkTransfer, Link)->Stage;

            if (Oldest != NULL)
            {
                Candidate = Oldest->FirstSegment;
                break;
            }
        }

        if (XhciBulkSegmentHas(Candidate, 0, MAXULONG, Address))
            Segment = Candidate;
    }

    if (Segment == NULL)
        return FALSE;

    Trb = static_cast<const XHCI_TRB*>(Segment->VirtualAddress) +
          (ULONG)((Address - (ULONG64)Segment->LogicalAddress.QuadPart) / sizeof(XHCI_TRB));
    *PointsToNoOp = (XhciTrbType(Trb) == static_cast<ULONG>(XhciTrbType::NoOp));
    return TRUE;
}

_Requires_lock_held_(Ring->m_Lock)
VOID
XhciBulkRing::Ed0Mismatch(
    _In_ XhciTransferRing* Ring,
    _In_ const XHCI_TRB* Event)
{
    ULONG64 Pointer = XhciTrbPointer(Event);
    ULONG Code = XhciTrbCompletionCode(Event);
    BOOLEAN NoOp;

    if (!LikelyDuplicate(Ring, Pointer, &NoOp))
    {
        DPRINT1("Bulk DCI %lu: event TRB pointer 0x%I64x is not on the ring, resetting\n",
                Ring->m_Endpoint->Dci(), Pointer);
        Ring->m_Controller->RaiseControllerFault(XhciRecovery::ResetHost, XhciBulkReasonEd0Invalid);
        return;
    }

    if (Ring->m_Controller->HasErrata(XhciErrata::EvtDiscardRepeatedEd0))
    {
        DPRINT1("Bulk DCI %lu: likely duplicate event at 0x%I64x dropped\n",
                Ring->m_Endpoint->Dci(), Pointer);
        return;
    }

    if (NoOp && XhciBulkNeedsNoOps(Ring) && XhciBulkIsStoppedCode(Code))
    {
        DPRINT1("Bulk DCI %lu: stopped event on a No Op TRB at 0x%I64x dropped\n",
                Ring->m_Endpoint->Dci(), Pointer);
        return;
    }

    DPRINT1("Bulk DCI %lu: likely duplicate event at 0x%I64x code %lu, resetting\n",
            Ring->m_Endpoint->Dci(), Pointer, Code);
    Ring->m_Controller->RaiseControllerFault(XhciRecovery::ResetHost, XhciBulkReasonEd0Duplicate);
}

/** Stream ring accounting when another stream saw the halt or the stopped event. */
_Requires_lock_held_(Ring->m_Lock)
VOID
XhciBulkRing::StreamStopOrHalt(
    _In_ XhciTransferRing* Ring)
{
    XhciBulkRing* Bulk = &Ring->m_Type.Bulk;
    XhciBulkStage* Stage;
    ULONG64 Dequeue;
    ULONG Skipped;
    ULONG Bytes;

    NT_ASSERT(Ring->m_StreamId != 0);

    Dequeue = Ring->m_Endpoint->StreamDequeuePointer(Ring->m_StreamId) & ~XHCI_DEQUEUE_FLAGS_MASK;

    if (!LookupStage(Ring, Dequeue, &Stage, &Skipped))
    {
        DPRINT1("Bulk DCI %lu stream %lu: stream dequeue pointer 0x%I64x is not on the ring\n",
                Ring->m_Endpoint->Dci(), Ring->m_StreamId, Dequeue);
        Stage = NULL;
    }

    if (Stage == NULL)
    {
        Bulk->m_OutstandingEvents = Bulk->m_PendingStages;
        return;
    }

    Bytes = Ring->m_Endpoint->StreamTransferLength(Ring->m_StreamId);
    if (Bytes > Stage->Size)
    {
        DPRINT1("Bulk DCI %lu stream %lu: context reports %lu bytes for a %lu byte stage\n",
                Ring->m_Endpoint->Dci(), Ring->m_StreamId, Bytes, Stage->Size);
        Bytes = 0;
    }

    Stage->Transfer->BytesTransferred += Bytes;
    XhciBulkCopyIn(Stage->Transfer, Stage, Bytes);
    Bulk->m_OutstandingEvents = Skipped;
}
