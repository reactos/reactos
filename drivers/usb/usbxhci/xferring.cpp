/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Transfer ring object shared by the control, bulk and isoch transfer types
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "xhcidrv.h"

#define NDEBUG
#include <debug.h>

/* Segment and buffer sizes of the two configurations */
static const ULONG XhciRingSegmentBytes = 0x200;
static const ULONG XhciRingWideSegmentBytes = 0x1000;
static const ULONG XhciRingDoubleBufferBytes = 0x200;

/** Largest bulk stage when a TD may not cross a Link TRB: one segment less a few TRBs, one page each. */
static const ULONG XhciRingSingleSegmentStage = (XhciRingSegmentBytes / sizeof(XHCI_TRB) - 4) * PAGE_SIZE;

/* How long Disable waits for a mapping context to unwind */
static const ULONG XhciRingIdleWaitMs = 5000;
static const ULONG XhciRingIdleWaitUs = 100000;

/** USBD status meaning "no hardware result", for the caller to replace with its default. */
static const USBD_STATUS XhciRingUsbdNotSet = (USBD_STATUS)0xFFFFFFFF;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(XhciTransferRing, XhciRingContext);

static EVT_WDF_OBJECT_CONTEXT_CLEANUP XhciRingQueueCleanup;
static EVT_WDF_WORKITEM XhciRingGrowthWorkItem;
static EVT_WDF_IO_WDM_IRP_FOR_FORWARD_PROGRESS XhciRingExamineForwardProgress;

/*
 * Every ring that can receive transfer events, across all controllers. The interrupter
 * fast path resolves its ring here from the event's slot and endpoint ids.
 */
static KSPIN_LOCK XhciLiveRingLock;
static LIST_ENTRY XhciLiveRings = { &XhciLiveRings, &XhciLiveRings };

static
ULONG
NTAPI
XhciRingCountList(
    _In_ const LIST_ENTRY* Head)
{
    const LIST_ENTRY* Entry;
    ULONG Count = 0;

    for (Entry = Head->Flink; Entry != Head; Entry = Entry->Flink)
        Count++;

    return Count;
}

/** Moves every entry of From to the tail of To, keeping their order. */
static
VOID
NTAPI
XhciRingAppendList(
    _Inout_ PLIST_ENTRY To,
    _Inout_ PLIST_ENTRY From)
{
    while (!IsListEmpty(From))
        InsertTailList(To, RemoveHeadList(From));
}

/** Moves every entry of From to the head of To, keeping their order. */
static
VOID
NTAPI
XhciRingPrependList(
    _Inout_ PLIST_ENTRY To,
    _Inout_ PLIST_ENTRY From)
{
    while (!IsListEmpty(From))
        InsertHeadList(To, RemoveTailList(From));
}

static
VOID
NTAPI
XhciRingQueueCleanup(
    _In_ WDFOBJECT Object)
{
    XhciRingContext(Object)->Cleanup();
}

static
VOID
NTAPI
XhciRingGrowthWorkItem(
    _In_ WDFWORKITEM WorkItem)
{
    WDFQUEUE Queue = (WDFQUEUE)WdfWorkItemGetParentObject(WorkItem);

    XhciTransferRing::FromQueue(Queue)->GrowSegments();
}

static
WDF_IO_FORWARD_PROGRESS_ACTION
NTAPI
XhciRingExamineForwardProgress(
    _In_ WDFQUEUE Queue,
    _In_ PIRP Irp)
{
    PURB Urb;

    UNREFERENCED_PARAMETER(Queue);

    /* Control and bulk URBs keep TransferFlags at the same offset */
    Urb = (PURB)IoGetCurrentIrpStackLocation(Irp)->Parameters.Others.Argument1;
    if (Urb != NULL &&
        (Urb->UrbBulkOrInterruptTransfer.TransferFlags & USB3_URB_RESERVED_RESOURCES))
    {
        return WdfIoForwardProgressActionUseReservedRequest;
    }

    return WdfIoForwardProgressActionFailRequest;
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
XhciTransferRing::Create(
    _In_ XhciEndpoint* Endpoint,
    _In_ WDFOBJECT Parent,
    _In_ ULONG StreamId,
    _Out_ XhciTransferRing** Ring)
{
    XhciController* Controller = Endpoint->Controller();
    WDF_IO_QUEUE_CONFIG QueueConfig;
    WDF_OBJECT_ATTRIBUTES Attributes;
    WDF_DPC_CONFIG DpcConfig;
    PFN_WDF_DPC CompletionDpc;
    XhciTransferRing* NewRing;
    WDFQUEUE Queue;
    ULONG TransferType;
    ULONG SegmentSize;
    ULONG MaxStageSize;
    BOOLEAN NoLinkInTd;
    NTSTATUS Status;

    *Ring = NULL;

    TransferType = Endpoint->TransferType();
    MaxStageSize = Controller->m_Buffers.MaxDmaSize() - PAGE_SIZE;
    NoLinkInTd = Controller->HasErrata(XhciErrata::RingKeepTdInOneSegment);

    if (TransferType == USB_ENDPOINT_TYPE_CONTROL)
    {
        WDF_IO_QUEUE_CONFIG_INIT(&QueueConfig, WdfIoQueueDispatchSequential);
        QueueConfig.EvtIoDefault = XhciControlRing::EvtIoDefault;
        QueueConfig.EvtIoCanceledOnQueue = XhciControlRing::EvtIoCanceledOnQueue;
        CompletionDpc = XhciControlRing::EvtCompletionDpc;

        /* A whole control transfer must fit one segment when no Link TRB may split it */
        SegmentSize = NoLinkInTd ? XhciRingWideSegmentBytes : XhciRingSegmentBytes;
    }
    else if (TransferType == USB_ENDPOINT_TYPE_BULK || TransferType == USB_ENDPOINT_TYPE_INTERRUPT)
    {
        WDF_IO_QUEUE_CONFIG_INIT(&QueueConfig, WdfIoQueueDispatchManual);
        QueueConfig.EvtIoCanceledOnQueue = XhciBulkRing::EvtIoCanceledOnQueue;
        CompletionDpc = XhciBulkRing::EvtCompletionDpc;
        SegmentSize = XhciRingSegmentBytes;

        if (NoLinkInTd)
            MaxStageSize = XhciRingSingleSegmentStage;
    }
    else if (TransferType == USB_ENDPOINT_TYPE_ISOCHRONOUS)
    {
        WDF_IO_QUEUE_CONFIG_INIT(&QueueConfig, WdfIoQueueDispatchManual);
        QueueConfig.EvtIoCanceledOnQueue = XhciIsochRing::EvtIoCanceledOnQueue;
        CompletionDpc = XhciIsochRing::EvtCompletionDpc;
        SegmentSize = XhciRingSegmentBytes;
    }
    else
    {
        DPRINT1("Endpoint DCI %lu has transfer type %lu, no transfer ring for it\n",
                Endpoint->Dci(), TransferType);
        return STATUS_UNSUCCESSFUL;
    }

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, XhciTransferRing);
    Attributes.ParentObject = Parent;
    Attributes.EvtCleanupCallback = XhciRingQueueCleanup;

    Status = WdfIoQueueCreate(Controller->m_Device, &QueueConfig, &Attributes, &Queue);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Queue for DCI %lu stream %lu not created, 0x%08lx\n",
                Endpoint->Dci(), StreamId, Status);
        return Status;
    }

    /* The context is zeroed by WDF, so cleanup copes with any step below failing */
    NewRing = new (XhciRingContext(Queue)) XhciTransferRing;
    KeInitializeSpinLock(&NewRing->m_Lock);
    InitializeListHead(&NewRing->m_FreeSegments);
    InitializeListHead(&NewRing->m_DoubleBuffers);
    XhciClearListEntry(&NewRing->m_LiveLink);
    NewRing->m_Device = Endpoint->Device();
    NewRing->m_Endpoint = Endpoint;
    NewRing->m_StreamId = StreamId;
    NewRing->m_TransferType = TransferType;
    NewRing->m_Queue = Queue;
    NewRing->m_MapState = static_cast<LONG>(XhciMapState::Halted);
    NewRing->m_SegmentSize = SegmentSize;
    NewRing->m_DoubleBufferSize = XhciRingDoubleBufferBytes;
    NewRing->m_MaxStageSize = MaxStageSize;
    NewRing->m_MapRegisterCount = Controller->m_Buffers.MapRegisterCount();
    NewRing->m_LastIndex = SegmentSize / sizeof(XHCI_TRB) - 1;
    NewRing->m_Cycle = 1;
    NewRing->m_Controller = Controller;

    if (!NewRing->IsControl())
    {
        WDF_WORKITEM_CONFIG WorkConfig;

        Status = WdfIoQueueReadyNotify(Queue,
                                       NewRing->IsIsoch() ? XhciIsochRing::ReadyNotification :
                                                            XhciBulkRing::ReadyNotification,
                                       NewRing);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Ready notification for DCI %lu stream %lu not registered, 0x%08lx\n",
                    Endpoint->Dci(), StreamId, Status);
            return Status;
        }

        WDF_WORKITEM_CONFIG_INIT(&WorkConfig, XhciRingGrowthWorkItem);
        WorkConfig.AutomaticSerialization = FALSE;
        WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
        Attributes.ParentObject = Queue;

        Status = WdfWorkItemCreate(&WorkConfig, &Attributes, &NewRing->m_GrowthItem);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Segment growth work item for DCI %lu stream %lu not created, 0x%08lx\n",
                    Endpoint->Dci(), StreamId, Status);
            return Status;
        }
    }

    WDF_DPC_CONFIG_INIT(&DpcConfig, CompletionDpc);
    DpcConfig.AutomaticSerialization = TRUE;
    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Queue;

    Status = WdfDpcCreate(&DpcConfig, &Attributes, &NewRing->m_CompletionDpc);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Completion DPC for DCI %lu stream %lu not created, 0x%08lx\n",
                Endpoint->Dci(), StreamId, Status);
        return Status;
    }

    /* Take the first segment now so a later enable at DISPATCH_LEVEL never allocates */
    Status = NewRing->EnsureFreeSegments(1, FALSE);
    if (NT_SUCCESS(Status))
    {
        XhciSpinLockGuard Guard(&NewRing->m_Lock);

        if (IsListEmpty(&NewRing->m_FreeSegments))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            NewRing->m_Segment = CONTAINING_RECORD(RemoveHeadList(&NewRing->m_FreeSegments),
                                                   XhciDmaBuffer,
                                                   Link);
            XhciClearListEntry(&NewRing->m_Segment->Link);
            NewRing->m_Trbs = (PXHCI_TRB)NewRing->m_Segment->VirtualAddress;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("No initial segment for DCI %lu stream %lu, 0x%08lx\n",
                Endpoint->Dci(), StreamId, Status);
        return Status;
    }

    if (NewRing->IsControl())
        Status = XhciControlRing::Initialize(NewRing);
    else if (NewRing->IsIsoch())
        Status = XhciIsochRing::Initialize(NewRing);
    else
        Status = XhciBulkRing::Initialize(NewRing);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Type setup for DCI %lu stream %lu failed, 0x%08lx\n",
                Endpoint->Dci(), StreamId, Status);
        return Status;
    }

    NewRing->m_TypeReady = TRUE;
    NewRing->MakeLive();

    DPRINT("Ring %p for DCI %lu stream %lu type %lu, segment 0x%lx, stage max 0x%lx\n",
           NewRing, Endpoint->Dci(), StreamId, TransferType, SegmentSize, MaxStageSize);

    *Ring = NewRing;
    return STATUS_SUCCESS;
}

XhciTransferRing*
XhciTransferRing::FromQueue(
    _In_ WDFQUEUE Queue)
{
    return XhciRingContext(Queue);
}

WDFQUEUE
XhciTransferRing::Queue() const
{
    return m_Queue;
}

BOOLEAN
XhciTransferRing::IsControl() const
{
    return m_TransferType == USB_ENDPOINT_TYPE_CONTROL;
}

BOOLEAN
XhciTransferRing::IsBulkOrInterrupt() const
{
    return m_TransferType == USB_ENDPOINT_TYPE_BULK ||
           m_TransferType == USB_ENDPOINT_TYPE_INTERRUPT;
}

BOOLEAN
XhciTransferRing::IsIsoch() const
{
    return m_TransferType == USB_ENDPOINT_TYPE_ISOCHRONOUS;
}

VOID
XhciTransferRing::Cleanup()
{
    LIST_ENTRY Leftover;

    /* The queue failed before the ring was filled in */
    if (m_Controller == NULL)
        return;

    RetireLive();

    InitializeListHead(&Leftover);
    {
        XhciSpinLockGuard Guard(&m_Lock);

        XhciRingAppendList(&Leftover, &m_FreeSegments);
        XhciRingAppendList(&Leftover, &m_DoubleBuffers);
    }

    if (!IsListEmpty(&Leftover))
    {
        DPRINT1("Ring for DCI %lu stream %lu still pooled %lu buffers at cleanup\n",
                m_Endpoint->Dci(), m_StreamId, XhciRingCountList(&Leftover));
        m_Controller->m_Buffers.FreeList(&Leftover);
    }

    if (m_Segment != NULL)
    {
        m_Controller->m_Buffers.Free(m_Segment);
        m_Segment = NULL;
        m_Trbs = NULL;
    }

    if (m_ReservedMdl != NULL)
    {
        IoFreeMdl(m_ReservedMdl);
        m_ReservedMdl = NULL;
    }

    if (!m_TypeReady)
        return;

    m_TypeReady = FALSE;
    if (IsControl())
        XhciControlRing::Cleanup(this);
    else if (IsIsoch())
        XhciIsochRing::Cleanup(this);
    else
        XhciBulkRing::Cleanup(this);
}

VOID
XhciTransferRing::MakeLive()
{
    XhciSpinLockGuard Guard(&XhciLiveRingLock);

    InsertTailList(&XhciLiveRings, &m_LiveLink);
    m_Live = TRUE;
}

VOID
XhciTransferRing::RetireLive()
{
    {
        XhciSpinLockGuard Guard(&XhciLiveRingLock);

        if (!m_Live)
            return;

        RemoveEntryList(&m_LiveLink);
        XhciClearListEntry(&m_LiveLink);
        m_Live = FALSE;
    }

    /* Nobody can find the ring any more; let an event handler already inside it finish */
    while (InterlockedCompareExchange(&m_EventUsers, 0, 0) != 0)
        YieldProcessor();
}

XhciTransferRing*
XhciTransferRing::ReferenceLiveRing(
    _In_ XhciController* Controller,
    _In_ const XhciUsbDevice* Device,
    _In_ ULONG Dci)
{
    XhciSpinLockGuard Guard(&XhciLiveRingLock);
    XhciTransferRing* Found = NULL;
    PLIST_ENTRY Entry;

    for (Entry = XhciLiveRings.Flink; Entry != &XhciLiveRings; Entry = Entry->Flink)
    {
        XhciTransferRing* Ring = CONTAINING_RECORD(Entry, XhciTransferRing, m_LiveLink);

        if (Ring->m_Controller != Controller ||
            Ring->m_Device != Device ||
            Ring->m_Endpoint->Dci() != Dci)
        {
            continue;
        }

        /* The event does not say which stream it belongs to; the endpoint sorts that out */
        if (Ring->m_StreamId != 0)
            return NULL;

        Found = Ring;
    }

    if (Found != NULL)
        InterlockedIncrement(&Found->m_EventUsers);

    return Found;
}

VOID
XhciTransferRing::DereferenceLive()
{
    InterlockedDecrement(&m_EventUsers);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
XhciTransferRing::Enable()
{
    ASSERT(m_MapState == static_cast<LONG>(XhciMapState::Halted));

    if (m_Segment == NULL)
    {
        DPRINT1("Ring for DCI %lu stream %lu has no segment to enable\n",
                m_Endpoint->Dci(), m_StreamId);
        return STATUS_INVALID_DEVICE_STATE;
    }

    InitializeRing();

    if (IsControl())
        XhciControlRing::Enable(this);
    else if (IsIsoch())
        XhciIsochRing::Enable(this);
    else
        XhciBulkRing::Enable(this);

    DPRINT("Ring for DCI %lu stream %lu enabled\n", m_Endpoint->Dci(), m_StreamId);
    return STATUS_SUCCESS;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
XhciTransferRing::WaitForMappingIdle()
{
    BOOLEAN Passive = (KeGetCurrentIrql() == PASSIVE_LEVEL);
    ULONGLONG Deadline = KeQueryInterruptTime() +
                         10000ULL * (Passive ? XhciRingIdleWaitMs : XhciRingIdleWaitUs / 1000);

    for (;;)
    {
        LONG State = InterlockedCompareExchange(&m_MapState, 0, 0);

        if (State != static_cast<LONG>(XhciMapState::Filling) &&
            State != static_cast<LONG>(XhciMapState::Halting))
        {
            return;
        }

        if (KeQueryInterruptTime() >= Deadline)
        {
            DPRINT1("Ring for DCI %lu stream %lu still in map state %ld, disabling anyway\n",
                    m_Endpoint->Dci(), m_StreamId, State);

            /* The late mapping context can no longer acknowledge the stop, so do it here once */
            if (InterlockedCompareExchange(&m_MapState,
                                           static_cast<LONG>(XhciMapState::Halted),
                                           static_cast<LONG>(XhciMapState::Halting)) ==
                static_cast<LONG>(XhciMapState::Halting))
            {
                m_Endpoint->OnMappingStopped();
            }
            return;
        }

        if (Passive)
        {
            LARGE_INTEGER Interval = XhciRelativeMs(1);

            KeDelayExecutionThread(KernelMode, FALSE, &Interval);
        }
        else
        {
            KeStallExecutionProcessor(1);
        }

    }
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
XhciTransferRing::Disable(
    _In_ BOOLEAN FreeResources)
{
    LIST_ENTRY Segments;
    LIST_ENTRY DoubleBuffers;
    LONG Previous;

    /* The idle wait is bounded so it cannot hang a single processor at DISPATCH_LEVEL */
    if (IsIsoch())
        XhciIsochRing::ReleaseParkedMapping(this);
    else if (!IsControl())
        XhciBulkRing::ReleaseParkedMapping(this);
    WaitForMappingIdle();

    if (IsControl())
        XhciControlRing::Disable(this);
    else if (IsIsoch())
        XhciIsochRing::Disable(this);
    else
        XhciBulkRing::Disable(this);

    Previous = InterlockedExchange(&m_MapState, static_cast<LONG>(XhciMapState::Halted));
    ASSERT(Previous == static_cast<LONG>(XhciMapState::Halted) ||
           Previous == static_cast<LONG>(XhciMapState::PausedFill));
    UNREFERENCED_PARAMETER(Previous);

    if (!FreeResources)
        return;

    InitializeListHead(&Segments);
    InitializeListHead(&DoubleBuffers);
    {
        XhciSpinLockGuard Guard(&m_Lock);

        XhciRingAppendList(&Segments, &m_FreeSegments);
        XhciRingAppendList(&DoubleBuffers, &m_DoubleBuffers);
    }

    if (!IsListEmpty(&Segments))
        m_Controller->m_Buffers.FreeList(&Segments);

    if (!IsListEmpty(&DoubleBuffers))
        m_Controller->m_Buffers.FreeList(&DoubleBuffers);

    if (m_ReservedMdl != NULL)
    {
        IoFreeMdl(m_ReservedMdl);
        m_ReservedMdl = NULL;
    }
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
XhciTransferRing::EnableForwardProgress(
    _In_ ULONG MaxTransferSize)
{
    WDF_IO_QUEUE_FORWARD_PROGRESS_POLICY Policy;
    PMDL Mdl;
    PMDL Old;
    NTSTATUS Status;

    if (IsIsoch())
    {
        DPRINT1("Forward progress asked for isoch DCI %lu, not supported\n", m_Endpoint->Dci());
        return STATUS_NOT_SUPPORTED;
    }

    if (IsControl())
        Status = XhciControlRing::EnableForwardProgress(this, MaxTransferSize);
    else
        Status = XhciBulkRing::EnableForwardProgress(this, MaxTransferSize);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Forward progress setup for DCI %lu stream %lu failed, 0x%08lx\n",
                m_Endpoint->Dci(), m_StreamId, Status);
        return Status;
    }

    Mdl = IoAllocateMdl(NULL, MaxTransferSize, FALSE, FALSE, NULL);
    if (Mdl == NULL)
    {
        DPRINT1("No forward progress MDL of %lu bytes for DCI %lu stream %lu\n",
                MaxTransferSize, m_Endpoint->Dci(), m_StreamId);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* KMDF refuses a second policy on the same queue, and the policy never changes */
    if (!m_ForwardProgressQueue)
    {
        WDF_IO_QUEUE_FORWARD_PROGRESS_POLICY_EXAMINE_INIT(&Policy, 1, XhciRingExamineForwardProgress);

        Status = WdfIoQueueAssignForwardProgressPolicy(m_Queue, &Policy);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Forward progress policy for DCI %lu stream %lu refused, 0x%08lx\n",
                    m_Endpoint->Dci(), m_StreamId, Status);
            IoFreeMdl(Mdl);
            return Status;
        }

        m_ForwardProgressQueue = TRUE;
    }

    /* A changed maximum size replaces the previous MDL */
    Old = m_ReservedMdl;
    m_ReservedMdl = Mdl;
    if (Old != NULL)
        IoFreeMdl(Old);

    DPRINT("Forward progress on DCI %lu stream %lu for %lu bytes\n",
           m_Endpoint->Dci(), m_StreamId, MaxTransferSize);
    return STATUS_SUCCESS;
}

VOID
XhciTransferRing::InitializeRing()
{
    RtlZeroMemory(m_Segment->VirtualAddress, m_Segment->Size);
    m_Trbs = (PXHCI_TRB)m_Segment->VirtualAddress;
    m_EnqueueIndex = 0;
    m_LastIndex = m_SegmentSize / sizeof(XHCI_TRB) - 1;
    m_Cycle = 1;
}

ULONG64
XhciTransferRing::SegmentBase() const
{
    return (ULONG64)m_Segment->LogicalAddress.QuadPart;
}

ULONG64
XhciTransferRing::EnqueuePointer() const
{
    return SegmentBase() + (ULONG64)m_EnqueueIndex * sizeof(XHCI_TRB);
}

ULONG64
XhciTransferRing::DequeuePointerValue() const
{
    ULONG64 Value = EnqueuePointer();

    if (m_Cycle & 1)
        Value |= XHCI_DEQUEUE_CYCLE;

    if (m_StreamId != 0)
        Value |= (ULONG64)XHCI_SCT_PRIMARY_RING << XHCI_DEQUEUE_SCT_SHIFT;

    return Value;
}

VOID
XhciTransferRing::ResumeRingFill()
{
    if (IsControl())
        XhciControlRing::ResumeRingFill(this);
    else if (IsIsoch())
        XhciIsochRing::ResumeRingFill(this);
    else
        XhciBulkRing::ResumeRingFill(this);
}

VOID
XhciTransferRing::SuspendRingFill()
{
    if (IsControl())
        XhciControlRing::SuspendRingFill(this);
    else if (IsIsoch())
        XhciIsochRing::SuspendRingFill(this);
    else
        XhciBulkRing::SuspendRingFill(this);
}

VOID
XhciTransferRing::OnPipeHalted()
{
    /* A halted control ring waits for its reset and an isoch ring never reports a halt */
    if (IsBulkOrInterrupt())
        XhciBulkRing::OnPipeHalted(this);
}

VOID
XhciTransferRing::OnClientPipeReset()
{
    /* Only an isoch ring cares: its next ASAP URB starts a new stream */
    if (IsIsoch())
        XhciIsochRing::OnClientPipeReset(this);
}

VOID
XhciTransferRing::StoppedEventReceived()
{
    /* An isoch ring already tracks the stop through its own events */
    if (IsBulkOrInterrupt())
        XhciBulkRing::StoppedEventReceived(this);
}

VOID
XhciTransferRing::AllowReclaimOnCancel()
{
    if (IsControl())
        XhciControlRing::AllowReclaimOnCancel(this);
    else if (IsIsoch())
        XhciIsochRing::AllowReclaimOnCancel(this);
    else
        XhciBulkRing::AllowReclaimOnCancel(this);
}

VOID
XhciTransferRing::ConsumePendingEvents()
{
    if (IsControl())
        XhciControlRing::ConsumePendingEvents(this);
    else if (IsIsoch())
        XhciIsochRing::ConsumePendingEvents(this);
    else
        XhciBulkRing::ConsumePendingEvents(this);
}

VOID
XhciTransferRing::RecoverTransfers()
{
    if (IsControl())
        XhciControlRing::RecoverTransfers(this);
    else if (IsIsoch())
        XhciIsochRing::RecoverTransfers(this);
    else
        XhciBulkRing::RecoverTransfers(this);
}

BOOLEAN
XhciTransferRing::DoorbellRungSinceFill() const
{
    if (IsControl())
        return XhciControlRing::DoorbellRungSinceFill(this);

    if (IsIsoch())
        return XhciIsochRing::DoorbellRungSinceFill(this);

    return XhciBulkRing::DoorbellRungSinceFill(this);
}

BOOLEAN
XhciTransferRing::HasQueuedWork() const
{
    if (IsControl())
        return XhciControlRing::HasQueuedWork(this);

    if (IsIsoch())
        return XhciIsochRing::HasQueuedWork(this);

    return XhciBulkRing::HasQueuedWork(this);
}

BOOLEAN
XhciTransferRing::OnTransferEvent(
    _In_ const XHCI_TRB* Event)
{
    if (IsControl())
        return XhciControlRing::OnTransferEvent(this, m_Controller, Event);

    if (IsBulkOrInterrupt())
        return XhciBulkRing::OnTransferEvent(this, m_Controller, Event);

    if (IsIsoch())
        return XhciIsochRing::OnTransferEvent(this, Event);

    return FALSE;
}

BOOLEAN
XhciTransferRing::OnUntargetedTransferEvent(
    _In_ XhciController* Controller,
    _In_ const XHCI_TRB* Event)
{
    ULONG EndpointType = Event->Dword[0] & XHCI_EVENT_DATA_TYPE_MASK;
    ULONG SlotId = XhciTrbSlotId(Event);
    ULONG Dci = (Event->Dword[3] & XHCI_TRB_ENDPOINT_MASK) >> XHCI_TRB_ENDPOINT_SHIFT;
    XhciTransferRing* Ring;
    BOOLEAN SameType;

    if (EndpointType == USB_ENDPOINT_TYPE_ISOCHRONOUS)
        return FALSE;

    /* Find the ring by slot and endpoint id so the type handler can check the pointer before using it */
    Ring = ReferenceLiveRing(Controller,
                             static_cast<const XhciUsbDevice*>(Controller->m_Slots.LookupSlot(SlotId)),
                             Dci);
    if (Ring == NULL)
        return FALSE;

    if (EndpointType == USB_ENDPOINT_TYPE_CONTROL)
        SameType = Ring->IsControl();
    else
        SameType = Ring->IsBulkOrInterrupt();

    if (SameType)
    {
        Ring->OnTransferEvent(Event);
    }
    else
    {
        DPRINT1("Stale Event Data 0x%I64x for slot %lu DCI %lu type %lu, dropped\n",
                XhciTrbPointer(Event), SlotId, Dci, Ring->m_TransferType);
    }

    Ring->DereferenceLive();
    return TRUE;
}

BOOLEAN
XhciTransferRing::IsLikelyDuplicate(
    _In_ const XHCI_TRB* Event) const
{
    if (IsBulkOrInterrupt())
        return XhciBulkRing::IsLikelyDuplicate(this, Event);

    return FALSE;
}

NTSTATUS
XhciTransferRing::EnsureFreeSegments(
    _In_ ULONG Count,
    _In_ BOOLEAN Async)
{
    LIST_ENTRY Taken;
    ULONG Have;
    ULONG Missing;
    BOOLEAN Enqueue = FALSE;
    NTSTATUS Status;

    {
        XhciSpinLockGuard Guard(&m_Lock);

        Have = XhciRingCountList(&m_FreeSegments);
    }

    if (Have >= Count)
        return STATUS_SUCCESS;

    Missing = Count - Have;
    InitializeListHead(&Taken);

    Status = m_Controller->m_Buffers.AcquireList(m_SegmentSize, Missing, &Taken);
    if (NT_SUCCESS(Status))
    {
        XhciSpinLockGuard Guard(&m_Lock);

        XhciRingAppendList(&m_FreeSegments, &Taken);
        return STATUS_SUCCESS;
    }

    /* At PASSIVE_LEVEL the pool already grew synchronously, so the failure is final */
    if (Async && m_GrowthItem != NULL && KeGetCurrentIrql() > PASSIVE_LEVEL)
    {
        {
            XhciSpinLockGuard Guard(&m_Lock);

            if (m_GrowthWanted < Missing)
                m_GrowthWanted = Missing;

            if (!m_GrowthQueued)
            {
                m_GrowthQueued = TRUE;
                Enqueue = TRUE;
            }
        }

        if (Enqueue)
            WdfWorkItemEnqueue(m_GrowthItem);

        return STATUS_PENDING;
    }

    DPRINT1("Ring for DCI %lu stream %lu could not get %lu segments, 0x%08lx\n",
            m_Endpoint->Dci(), m_StreamId, Missing, Status);
    return Status;
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
XhciTransferRing::GrowSegments()
{
    LIST_ENTRY Taken;
    ULONG Count;
    NTSTATUS Status = STATUS_SUCCESS;

    {
        XhciSpinLockGuard Guard(&m_Lock);

        Count = m_GrowthWanted;
        m_GrowthWanted = 0;
        m_GrowthQueued = FALSE;
    }

    InitializeListHead(&Taken);
    if (Count != 0)
        Status = m_Controller->m_Buffers.AcquireList(m_SegmentSize, Count, &Taken);

    if (NT_SUCCESS(Status))
    {
        XhciSpinLockGuard Guard(&m_Lock);

        XhciRingPrependList(&m_FreeSegments, &Taken);
    }
    else
    {
        DPRINT1("Ring for DCI %lu stream %lu could not grow by %lu segments, 0x%08lx\n",
                m_Endpoint->Dci(), m_StreamId, Count, Status);
    }

    if (IsIsoch())
        XhciIsochRing::SegmentsArrived(this, Status);
    else
        XhciBulkRing::SegmentsArrived(this, Status);
}

NTSTATUS
XhciTransferRing::TakeSegments(
    _In_ ULONG Count,
    _Inout_ PLIST_ENTRY List)
{
    LIST_ENTRY Moved;
    ULONG Index;
    NTSTATUS Status;

    Status = EnsureFreeSegments(Count, TRUE);
    if (Status != STATUS_SUCCESS)
        return Status;

    InitializeListHead(&Moved);
    {
        XhciSpinLockGuard Guard(&m_Lock);

        if (XhciRingCountList(&m_FreeSegments) < Count)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            for (Index = 0; Index < Count; Index++)
                InsertTailList(&Moved, RemoveHeadList(&m_FreeSegments));

            XhciRingPrependList(List, &Moved);
        }
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Ring for DCI %lu stream %lu lost its reserved segments\n",
                m_Endpoint->Dci(), m_StreamId);
    }

    return Status;
}

VOID
XhciTransferRing::ReturnSegments(
    _Inout_ PLIST_ENTRY List,
    _In_ BOOLEAN ToHead)
{
    if (IsListEmpty(List))
        return;

    if (m_Controller->HasErrata(XhciErrata::RingClearNewSegments))
    {
        PLIST_ENTRY Entry;

        for (Entry = List->Flink; Entry != List; Entry = Entry->Flink)
        {
            XhciDmaBuffer* Segment = CONTAINING_RECORD(Entry, XhciDmaBuffer, Link);

            RtlZeroMemory(Segment->VirtualAddress, Segment->Size);
        }
    }

    if (ToHead)
        XhciRingPrependList(&m_FreeSegments, List);
    else
        XhciRingAppendList(&m_FreeSegments, List);
}

XhciDmaBuffer*
XhciTransferRing::BorrowBounceBuffer()
{
    XhciDmaBuffer* Buffer = NULL;

    /* Forward progress rings never double buffer */
    if (m_DoubleBufferSize == 0)
        return NULL;

    {
        XhciSpinLockGuard Guard(&m_Lock);

        if (!IsListEmpty(&m_DoubleBuffers))
        {
            Buffer = CONTAINING_RECORD(RemoveHeadList(&m_DoubleBuffers), XhciDmaBuffer, Link);
            XhciClearListEntry(&Buffer->Link);
        }
    }

    if (Buffer == NULL)
        Buffer = m_Controller->m_Buffers.Acquire(m_DoubleBufferSize);

    return Buffer;
}

VOID
XhciTransferRing::ReturnBounceBuffer(
    _In_ XhciDmaBuffer* Buffer)
{
    XhciSpinLockGuard Guard(&m_Lock);

    InsertTailList(&m_DoubleBuffers, &Buffer->Link);
}

XhciMapState
XhciTransferRing::ChangeMapState(
    _In_ BOOLEAN FromAny,
    _In_ XhciMapState From,
    _In_ XhciMapState To)
{
    LONG Old;
    BOOLEAN Changed;

    if (FromAny)
    {
        Old = InterlockedExchange(&m_MapState, static_cast<LONG>(To));
        Changed = TRUE;
    }
    else
    {
        Old = InterlockedCompareExchange(&m_MapState, static_cast<LONG>(To), static_cast<LONG>(From));
        Changed = (Old == static_cast<LONG>(From));
    }

    if (Changed)
    {
        if (To == XhciMapState::Halted)
            m_Endpoint->OnMappingStopped();
    }
    else if (From == XhciMapState::Filling)
    {
        /* A stop arrived while this context was mapping; it acknowledges the stop itself */
        ASSERT(Old == static_cast<LONG>(XhciMapState::Halting));

        if (InterlockedCompareExchange(&m_MapState,
                                       static_cast<LONG>(XhciMapState::Halted),
                                       static_cast<LONG>(XhciMapState::Halting)) ==
            static_cast<LONG>(XhciMapState::Halting))
        {
            m_Endpoint->OnMappingStopped();
        }
    }

    return static_cast<XhciMapState>(Old);
}

VOID
XhciTransferRing::BuildLinkTrb(
    _Out_ PXHCI_TRB Trb,
    _In_ BOOLEAN FirstOfTd,
    _In_ BOOLEAN InDataStage) const
{
    ULONG Control;

    RtlZeroMemory(Trb, sizeof(*Trb));

    /* A TD's first TRB stays invalid until the whole TD is written */
    Control = static_cast<ULONG>(XhciTrbType::Link) << XHCI_TRB_TYPE_SHIFT;
    Control |= FirstOfTd ? (m_Cycle ^ 1) & XHCI_TRB_CYCLE : m_Cycle & XHCI_TRB_CYCLE;

    if (m_Controller->HasErrata(XhciErrata::RingLinkTrbChain))
        Control |= XHCI_TRB_CHAIN;
    else if (!FirstOfTd && InDataStage)
        Control |= XHCI_TRB_CHAIN;

    Trb->Dword[2] = (m_InterrupterTarget & XHCI_TRB_INTERRUPTER_MASK) << XHCI_TRB_INTERRUPTER_SHIFT;
    Trb->Dword[3] = Control;
}

ULONG
XhciTransferRing::PacketCount(
    _In_ ULONG Bytes) const
{
    ULONG MaxPacket = m_Endpoint->MaxPacketSize();

    /* xHCI 0.96 parts take no TD Size, so the count is not needed */
    if (m_Controller->m_Registers.VersionMajor() < 1 || MaxPacket == 0)
        return 0;

    return (Bytes + MaxPacket - 1) / MaxPacket;
}

ULONG
XhciTransferRing::TdSize(
    _In_ ULONG PacketCount,
    _In_ ULONG BytesMappedSoFar,
    _In_ BOOLEAN LastTrbOfTd) const
{
    ULONG MaxPacket = m_Endpoint->MaxPacketSize();
    ULONG Sent;
    ULONG Remaining;

    if (LastTrbOfTd || MaxPacket == 0 || m_Controller->m_Registers.VersionMajor() < 1)
        return 0;

    Sent = BytesMappedSoFar / MaxPacket;
    if (Sent >= PacketCount)
        return 0;

    Remaining = PacketCount - Sent;
    return (Remaining > 31) ? 31 : Remaining;
}

VOID
XhciTransferRing::UpdateInterrupterTarget()
{
    m_InterrupterTarget = m_Controller->m_Interrupters.TargetForCurrentProcessor() &
                          XHCI_TRB_INTERRUPTER_MASK;
}

VOID
XhciTransferRing::UpdateInterrupterTarget(
    _In_ ULONG ProcessorIndex)
{
    if (ProcessorIndex == XHCI_URB_NO_PROCESSOR)
    {
        UpdateInterrupterTarget();
        return;
    }

    m_InterrupterTarget = m_Controller->m_Interrupters.TargetForProcessor(ProcessorIndex) &
                          XHCI_TRB_INTERRUPTER_MASK;
}

ULONG
XhciTransferRing::UrbProcessorIndex(
    _In_ PURB Urb)
{
    return *(PULONG)&Urb->UrbControlTransfer.hca;
}

VOID
XhciTransferRing::RingDoorbell()
{
    /* The cycle bit flip must be visible before the controller is told to look */
    KeMemoryBarrier();
    m_Device->RingDoorbell(m_Endpoint->Dci(), m_StreamId);
}

/** Code 0 (no hardware result) gives the "not set" value 0xFFFFFFFF for the caller to replace. */
USBD_STATUS
XhciTransferRing::UsbdStatusFromCompletionCode(
    _In_ ULONG CompletionCode)
{
    switch (static_cast<XhciCompletionCode>(CompletionCode))
    {
        case XhciCompletionCode::Invalid:
            return XhciRingUsbdNotSet;

        /* QUIRK: short packets always succeed, USBD_SHORT_TRANSFER_OK is not consulted */
        case XhciCompletionCode::Success:
        case XhciCompletionCode::ShortPacket:
        case XhciCompletionCode::Stopped:
        case XhciCompletionCode::StoppedShortPacket:
            return USBD_STATUS_SUCCESS;

        case XhciCompletionCode::DataBufferError:
            return USBD_STATUS_DATA_BUFFER_ERROR;

        case XhciCompletionCode::BabbleDetected:
        case XhciCompletionCode::IsochBufferOverrun:
            return USBD_STATUS_BABBLE_DETECTED;

        case XhciCompletionCode::StallError:
            return USBD_STATUS_STALL_PID;

        case XhciCompletionCode::UsbTransactionError:
        case XhciCompletionCode::SplitTransactionError:
            return USBD_STATUS_XACT_ERROR;

        case XhciCompletionCode::MissedService:
            return USBD_STATUS_ISO_TD_ERROR;

        case XhciCompletionCode::NoPingResponse:
            return USBD_STATUS_NO_PING_RESPONSE;

        case XhciCompletionCode::StoppedLengthInvalid:
            return USBD_STATUS_ISO_NOT_ACCESSED_BY_HW;

        case XhciCompletionCode::InvalidStreamType:
            return USBD_STATUS_INVALID_STREAM_TYPE;

        case XhciCompletionCode::InvalidStreamId:
            return USBD_STATUS_INVALID_STREAM_ID;

        default:
            /* QUIRK: any other code is reported as a transaction error */
            DPRINT1("Unexpected completion code %lu reported as a transaction error\n",
                    CompletionCode);
            return USBD_STATUS_XACT_ERROR;
    }
}

NTSTATUS
XhciTransferRing::NtStatusFromUsbdStatus(
    _In_ USBD_STATUS UsbdStatus)
{
    switch (UsbdStatus)
    {
        case USBD_STATUS_SUCCESS:
        case USBD_STATUS_PORT_OPERATION_PENDING:
            return STATUS_SUCCESS;

        case USBD_STATUS_INSUFFICIENT_RESOURCES:
            return STATUS_INSUFFICIENT_RESOURCES;

        case USBD_STATUS_INVALID_PARAMETER:
        case USBD_STATUS_INVALID_URB_FUNCTION:
        case USBD_STATUS_INVALID_PIPE_HANDLE:
        case USBD_STATUS_BAD_START_FRAME:
            return STATUS_INVALID_PARAMETER;

        case USBD_STATUS_NOT_SUPPORTED:
            return STATUS_NOT_SUPPORTED;

        case USBD_STATUS_DEVICE_GONE:
            return STATUS_NO_SUCH_DEVICE;

        case USBD_STATUS_CANCELED:
            return STATUS_CANCELLED;

        default:
            return STATUS_UNSUCCESSFUL;
    }
}
