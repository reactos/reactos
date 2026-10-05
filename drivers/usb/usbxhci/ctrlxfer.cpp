/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Control transfer ring: setup, data and status TDs on endpoint 0 style pipes
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "xhcidrv.h"

#define NDEBUG
#include <debug.h>

/* Event Data value layout: endpoint type in bits 1:0, status stage flag in bit 2 */
static const ULONG_PTR XhciCtlStatusStageFlag = 0x4;
static const ULONG_PTR XhciCtlValueFlagBits = 0x7;

static const ULONG XhciCtlTagSignature = 0x6C744358;

/* Reason codes handed to the controller with a fatal error */
static const ULONG XhciCtlReasonBadTrbPointer = 0x101F;
static const ULONG XhciCtlReasonDuplicateEvent = 0x1020;

static const ULONG XhciCtlMaxTransfer = 0x10000;
static const ULONG XhciCtlMaxTrbLength = 0x10000;
static const ULONG XhciCtlShortDataStageTrb = 512;
static const ULONG XhciCtlImmediateBytes = 8;
static const ULONG XhciCtlSetupBytes = 8;

/* Setup, 17 data, a status TRB and two Event Data TRBs */
static const ULONG XhciCtlWorstCaseTd = 21;

enum class XhciCtlStep : ULONG
{
    Setup,
    FirstData,
    MoreData,
    DataEventData,
    Status,
    StatusEventData,
    Done
};

static
XhciControlRing*
NTAPI
XhciCtlOf(
    _In_ XhciTransferRing* Ring)
{
    return &Ring->m_Type.Control;
}

static
XhciMapState
NTAPI
XhciCtlMapState(
    _In_ const XhciTransferRing* Ring)
{
    return static_cast<XhciMapState>(Ring->m_MapState);
}

static
VOID
NTAPI
XhciCtlSetMapState(
    _Inout_ XhciTransferRing* Ring,
    _In_ XhciMapState State)
{
    InterlockedExchange(&Ring->m_MapState, static_cast<LONG>(State));
}

static
ULONG
NTAPI
XhciCtlTrbType(
    _In_ enum XhciTrbType Type)
{
    return static_cast<ULONG>(Type) << XHCI_TRB_TYPE_SHIFT;
}

/** Codes after which the controller halts the endpoint. */
static
BOOLEAN
NTAPI
XhciCtlIsHaltCode(
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
XhciCtlIsStopCode(
    _In_ ULONG Code)
{
    return Code == static_cast<ULONG>(XhciCompletionCode::Stopped) ||
           Code == static_cast<ULONG>(XhciCompletionCode::StoppedLengthInvalid) ||
           Code == static_cast<ULONG>(XhciCompletionCode::StoppedShortPacket);
}

/** URB status for a request that finished without a hardware completion code. */
static
USBD_STATUS
NTAPI
XhciCtlUsbdStatusFromNt(
    _In_ NTSTATUS Status)
{
    switch (Status)
    {
        case STATUS_CANCELLED:
            return USBD_STATUS_CANCELED;
        case STATUS_INSUFFICIENT_RESOURCES:
            return USBD_STATUS_INSUFFICIENT_RESOURCES;
        case STATUS_NOT_SUPPORTED:
            return USBD_STATUS_NOT_SUPPORTED;
        default:
            return USBD_STATUS_INVALID_PARAMETER;
    }
}

/** Writes a Link TRB back to the segment start at the enqueue index and wraps. */
static
VOID
NTAPI
XhciCtlWrap(
    _Inout_ XhciTransferRing* Ring,
    _In_ BOOLEAN FirstOfTd,
    _In_ BOOLEAN InDataStage)
{
    XHCI_TRB Link;
    ULONG64 Base = Ring->SegmentBase();

    Ring->BuildLinkTrb(&Link, FirstOfTd, InDataStage);
    Link.Dword[0] = static_cast<ULONG>(Base);
    Link.Dword[1] = static_cast<ULONG>(Base >> 32);
    Link.Dword[3] |= XHCI_LINK_TOGGLE_CYCLE;

    Ring->m_Trbs[Ring->m_EnqueueIndex] = Link;
    Ring->m_Cycle ^= XHCI_TRB_CYCLE;
    Ring->m_EnqueueIndex = 0;
}

/** Stores a finished TRB; the first TRB of a TD stays invalid until the doorbell step. */
static
VOID
NTAPI
XhciCtlPut(
    _Inout_ XhciTransferRing* Ring,
    _Inout_ PXHCI_TRB Trb,
    _In_ BOOLEAN FirstOfTd)
{
    ULONG Cycle = Ring->m_Cycle & XHCI_TRB_CYCLE;

    if (FirstOfTd)
        Cycle ^= XHCI_TRB_CYCLE;

    Trb->Dword[3] = (Trb->Dword[3] & ~XHCI_TRB_CYCLE) | Cycle;
    Ring->m_Trbs[Ring->m_EnqueueIndex] = *Trb;
    Ring->m_EnqueueIndex++;
}

/* Ring type entry points ****************************************************/

NTSTATUS
XhciControlRing::Initialize(
    _In_ XhciTransferRing* Ring)
{
    XhciControlRing* Self = XhciCtlOf(Ring);
    WDF_TIMER_CONFIG TimerConfig;
    WDF_DPC_CONFIG DpcConfig;
    WDF_OBJECT_ATTRIBUTES Attributes;
    NTSTATUS Status;
    ULONG Index;

    PAGED_CODE();

    RtlZeroMemory(Self, sizeof(*Self));
    Self->m_Ring = Ring;
    InitializeListHead(&Self->m_CanceledOnQueue);

    for (Index = 0; Index < TagCount; Index++)
    {
        Self->m_Tags[Index].Signature = XhciCtlTagSignature;
        Self->m_Tags[Index].Index = Index;
        Self->m_Tags[Index].Ring = Ring;
    }

    /* xHCI 6.4.1.1: immediate data needs a max packet size of at least 8 */
    Self->m_ImmediateAllowed = !Ring->m_Controller->HasErrata(XhciErrata::XferNoInlineData) &&
                               Ring->m_Endpoint->MaxPacketSize() >= XhciCtlImmediateBytes;

    WDF_TIMER_CONFIG_INIT(&TimerConfig, EvtTimeout);
    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Ring->m_Queue;
    Status = WdfTimerCreate(&TimerConfig, &Attributes, &Self->m_Timer);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Control ring %p: no timeout timer, status 0x%lx\n", Ring, Status);
        return Status;
    }

    WDF_DPC_CONFIG_INIT(&DpcConfig, EvtCanceledOnQueueDpc);
    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Ring->m_Queue;
    Status = WdfDpcCreate(&DpcConfig, &Attributes, &Self->m_CanceledOnQueueDpc);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Control ring %p: no DPC for requests canceled in the queue, status 0x%lx\n",
                Ring, Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

VOID
XhciControlRing::Enable(
    _In_ XhciTransferRing* Ring)
{
    UNREFERENCED_PARAMETER(Ring);
}

VOID
XhciControlRing::Disable(
    _In_ XhciTransferRing* Ring)
{
    XhciControlRing* Self = XhciCtlOf(Ring);
    XhciMapState State;
    KIRQL OldIrql;

    /* The common ring already waited, with a limit, for the last mapping context to unwind */
    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);

    State = XhciCtlMapState(Ring);
    if (State == XhciMapState::PausedFill)
        XhciCtlSetMapState(Ring, XhciMapState::Halted);
    else if (State == XhciMapState::Filling || State == XhciMapState::Halting)
        DPRINT1("Control ring %p disabled in map state %ld\n", Ring, static_cast<LONG>(State));

    if (Self->m_Current != NULL)
        DPRINT1("Control ring %p disabled while it holds request %p\n", Ring, Self->m_Current->Request);

    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
}

NTSTATUS
XhciControlRing::EnableForwardProgress(
    _In_ XhciTransferRing* Ring,
    _In_ ULONG MaxTransferSize)
{
    XhciControlRing* Self = XhciCtlOf(Ring);
    ULONG Pages;
    ULONG Trbs;
    KIRQL OldIrql;

    PAGED_CODE();

    /* Worst case page span of the buffer, then setup, two Event Data, status and a link */
    Pages = (PAGE_SIZE - 1 + MaxTransferSize + PAGE_SIZE - 1) / PAGE_SIZE;
    Trbs = Pages + 5;
    if (Trbs > Ring->m_LastIndex + 1)
        DPRINT1("Control ring %p: %lu TRBs for a forward progress transfer exceed the segment\n", Ring, Trbs);

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    Ring->m_DoubleBufferSize = 0;
    Self->m_ImmediateAllowed = FALSE;
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    return STATUS_SUCCESS;
}

VOID
XhciControlRing::Cleanup(
    _In_ XhciTransferRing* Ring)
{
    XhciControlRing* Self = XhciCtlOf(Ring);
    ULONG Index;

    if (Self->m_Current != NULL)
        DPRINT1("Control ring %p cleaned up while it holds request %p\n", Ring, Self->m_Current->Request);

    /* A late fast path event must not take freed ring memory for a live tag */
    for (Index = 0; Index < TagCount; Index++)
    {
        Self->m_Tags[Index].Signature = 0;
        Self->m_Tags[Index].Transfer = NULL;
    }
}

/* Request intake ************************************************************/

VOID
XhciControlRing::InitTransfer(
    _Out_ XhciControlTransfer* Transfer,
    _In_ WDFREQUEST Request)
{
    WDF_REQUEST_PARAMETERS Parameters;
    PURB Urb;

    WDF_REQUEST_PARAMETERS_INIT(&Parameters);
    WdfRequestGetParameters(Request, &Parameters);
    Urb = static_cast<PURB>(Parameters.Parameters.Others.Arg1);

    RtlZeroMemory(Transfer, sizeof(*Transfer));
    Transfer->Request = Request;
    Transfer->Urb = Urb;
    Transfer->Ring = m_Ring;
    Transfer->Mechanism = XhciControlMechanism::NoData;
    Transfer->BytesTotal = Urb->UrbControlTransfer.TransferBufferLength;
    Transfer->DataIn = (Urb->UrbControlTransfer.TransferFlags & USBD_TRANSFER_DIRECTION_IN) != 0;
    Transfer->CancelState = XhciControlCancel::Idle;
    Transfer->TimeoutState = XhciControlTimeout::Idle;
    Transfer->Status = STATUS_PENDING;
    Transfer->CompletionCode = static_cast<ULONG>(XhciCompletionCode::Invalid);
    Transfer->FirstIndex = m_Ring->m_EnqueueIndex;
    Transfer->EndIndex = m_Ring->m_EnqueueIndex;
    Transfer->Initialized = TRUE;

    /* QUIRK: the length reads 0 until completion, as it did on the USB 2.0 stack */
    Urb->UrbControlTransfer.TransferBufferLength = 0;
}

VOID
XhciControlRing::EvtIoDefault(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request)
{
    XhciTransferRing* Ring = XhciTransferRing::FromQueue(Queue);
    XhciControlRing* Self = XhciCtlOf(Ring);
    XhciRequestData* Data = XhciGetRequestData(Request);
    XhciControlTransfer* Transfer;
    BOOLEAN Map = FALSE;
    NTSTATUS Status;
    KIRQL OldIrql;

    if (Data == NULL)
    {
        DPRINT1("Control request %p has no driver context\n", Request);
        WdfRequestComplete(Request, STATUS_INVALID_DEVICE_STATE);
        return;
    }

    Transfer = &Data->Control;
    Self->InitTransfer(Transfer, Request);

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);

    ASSERT(Self->m_Current == NULL);
    Self->m_Current = Transfer;

    /* Cancelable before mapping, so a later reclaim can always finish it */
    Transfer->CancelState = XhciControlCancel::Armed;
    Status = WdfRequestMarkCancelableEx(Request, EvtRequestCancel);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Control request %p was canceled on arrival, status 0x%lx\n", Request, Status);
        Transfer->CancelState = XhciControlCancel::Done;
        Self->CompleteHeld();
    }
    else if (XhciCtlMapState(Ring) == XhciMapState::PausedFill)
    {
        XhciCtlSetMapState(Ring, XhciMapState::Filling);
        Map = TRUE;
    }

    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    if (Map)
        Self->BuildControlTd(Transfer);
}

/* Mapping *******************************************************************/

VOID
XhciControlRing::ReleaseResources(
    _Inout_ XhciControlTransfer* Transfer)
{
    XhciTransferRing* Ring = m_Ring;
    PDMA_ADAPTER Adapter;
    KIRQL OldIrql;

    if (Transfer->DoubleBuffer != NULL)
    {
        Ring->ReturnBounceBuffer(Transfer->DoubleBuffer);
        Transfer->DoubleBuffer = NULL;
    }

    if (Transfer->SgList != NULL)
    {
        Adapter = Ring->m_Controller->m_Buffers.DmaAdapter();
        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
        Adapter->DmaOperations->PutScatterGatherList(Adapter, Transfer->SgList, !Transfer->DataIn);
        KeLowerIrql(OldIrql);
        Transfer->SgList = NULL;
    }

    if (Transfer->Mdl != NULL && Transfer->Mdl == Ring->m_ReservedMdl)
    {
        InterlockedExchange(&m_ReservedMdlBusy, 0);
    }
    else if (Transfer->Mdl != NULL && Transfer->Mdl != Transfer->Urb->UrbControlTransfer.TransferBufferMDL)
    {
        IoFreeMdl(Transfer->Mdl);
    }

    Transfer->Mdl = NULL;
    Transfer->SystemBuffer = NULL;
    Transfer->Mechanism = XhciControlMechanism::NoData;
}

NTSTATUS
XhciControlRing::PrepareTransfer(
    _Inout_ XhciControlTransfer* Transfer)
{
    XhciTransferRing* Ring = m_Ring;
    struct _URB_CONTROL_TRANSFER* Urb = &Transfer->Urb->UrbControlTransfer;
    PMDL UrbMdl = Urb->TransferBufferMDL;
    ULONG Bytes = Transfer->BytesTotal;
    XhciControlMechanism Mechanism;
    PMDL Mdl;

    /* A transfer mapped again after a stop starts over */
    ReleaseResources(Transfer);

    if (Bytes == 0)
        return STATUS_SUCCESS;

    if (m_ImmediateAllowed && Bytes <= XhciCtlImmediateBytes && !Transfer->DataIn)
    {
        Mechanism = XhciControlMechanism::Immediate;
    }
    else
    {
        Mechanism = XhciControlMechanism::Dma;

        if ((UrbMdl == NULL || UrbMdl->Next == NULL) && Bytes <= Ring->m_DoubleBufferSize)
        {
            Transfer->DoubleBuffer = Ring->BorrowBounceBuffer();
            if (Transfer->DoubleBuffer != NULL)
                Mechanism = XhciControlMechanism::DoubleBuffer;
        }
    }

    Transfer->Mechanism = Mechanism;

    if (Mechanism != XhciControlMechanism::Dma)
    {
        Transfer->SystemBuffer = Urb->TransferBuffer;
        if (Transfer->SystemBuffer == NULL && UrbMdl != NULL)
            Transfer->SystemBuffer = MmGetSystemAddressForMdlSafe(UrbMdl, NormalPagePriority);

        if (Transfer->SystemBuffer == NULL)
        {
            DPRINT1("Control request %p: no system address for its buffer\n", Transfer->Request);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else if (UrbMdl != NULL)
    {
        Transfer->Mdl = UrbMdl;
    }
    else
    {
        Mdl = IoAllocateMdl(Urb->TransferBuffer, Bytes, FALSE, FALSE, NULL);
        if (Mdl == NULL)
        {
            /* The reserve MDL serves one request at a time */
            if (!(Urb->TransferFlags & USB3_URB_RESERVED_RESOURCES) ||
                Ring->m_ReservedMdl == NULL ||
                InterlockedCompareExchange(&m_ReservedMdlBusy, 1, 0) != 0)
            {
                DPRINT1("Control request %p: no MDL for %lu bytes\n", Transfer->Request, Bytes);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            Mdl = Ring->m_ReservedMdl;
            MmInitializeMdl(Mdl, Urb->TransferBuffer, Bytes);
        }

        MmBuildMdlForNonPagedPool(Mdl);
        Transfer->Mdl = Mdl;
    }

    /* QUIRK: one TD per request, never split */
    if (Bytes > XhciCtlMaxTransfer)
    {
        DPRINT1("Control request %p: %lu bytes is more than 64 KB\n", Transfer->Request, Bytes);
        return STATUS_NOT_SUPPORTED;
    }

    if (Bytes > Ring->m_MaxStageSize)
    {
        DPRINT1("Control request %p: %lu bytes is more than the DMA stage size %lu\n",
                Transfer->Request, Bytes, Ring->m_MaxStageSize);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* QUIRK: the system buffer is NULL for Dma, so this never fires there */
    if (Bytes == Ring->m_MaxStageSize &&
        (reinterpret_cast<ULONG_PTR>(Transfer->SystemBuffer) & (PAGE_SIZE - 1)) != 0)
    {
        DPRINT1("Control request %p: stage sized buffer %p is not page aligned\n",
                Transfer->Request, Transfer->SystemBuffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (Mechanism == XhciControlMechanism::Dma && Transfer->Mdl->Next != NULL)
    {
        DPRINT1("Control request %p: chained MDLs are not supported\n", Transfer->Request);
        return STATUS_NOT_SUPPORTED;
    }

    return STATUS_SUCCESS;
}

VOID
XhciControlRing::BuildControlTd(
    _Inout_ XhciControlTransfer* Transfer)
{
    XhciTransferRing* Ring = m_Ring;
    PURB Urb = Transfer->Urb;
    PDMA_ADAPTER Adapter;
    BOOLEAN Acknowledge = FALSE;
    NTSTATUS Status;
    KIRQL OldIrql;

    Status = PrepareTransfer(Transfer);
    if (NT_SUCCESS(Status))
    {
        if (Urb->UrbHeader.Function == URB_FUNCTION_CONTROL_TRANSFER_EX &&
            Urb->UrbControlTransferEx.Timeout != 0)
        {
            KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
            if (Transfer->TimeoutState == XhciControlTimeout::Idle ||
                Transfer->TimeoutState == XhciControlTimeout::Armed)
            {
                Transfer->TimeoutState = XhciControlTimeout::Armed;
                WdfTimerStart(m_Timer, WDF_REL_TIMEOUT_IN_MS(Urb->UrbControlTransferEx.Timeout));
            }
            KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
        }

        Ring->UpdateInterrupterTarget(XhciTransferRing::UrbProcessorIndex(Urb));

        if (Transfer->Mechanism != XhciControlMechanism::Dma)
        {
            WriteTd(Transfer);
            return;
        }

        Adapter = Ring->m_Controller->m_Buffers.DmaAdapter();
        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
        Status = Adapter->DmaOperations->GetScatterGatherList(Adapter,
                                                              Ring->m_Controller->m_WdmDevice,
                                                              Transfer->Mdl,
                                                              MmGetMdlVirtualAddress(Transfer->Mdl),
                                                              Transfer->BytesTotal,
                                                              EvtDmaReady,
                                                              Transfer,
                                                              !Transfer->DataIn);
        KeLowerIrql(OldIrql);

        if (NT_SUCCESS(Status))
            return;

        DPRINT1("Control request %p: scatter gather list not built, status 0x%lx\n",
                Transfer->Request, Status);
    }

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);

    Transfer->Status = Status;

    if (XhciCtlMapState(Ring) == XhciMapState::Filling)
    {
        XhciCtlSetMapState(Ring, XhciMapState::PausedFill);
        if (m_Current == Transfer)
            CompleteHeld();
    }
    else if (XhciCtlMapState(Ring) == XhciMapState::Halting)
    {
        /* The request stays held; reclaim completes it with the stored status */
        XhciCtlSetMapState(Ring, XhciMapState::Halted);
        Acknowledge = TRUE;
    }

    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    if (Acknowledge)
        Ring->m_Endpoint->OnMappingStopped();
}

VOID
NTAPI
XhciControlRing::EvtDmaReady(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PSCATTER_GATHER_LIST ScatterGather,
    _In_ PVOID Context)
{
    XhciControlTransfer* Transfer = static_cast<XhciControlTransfer*>(Context);

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);

    Transfer->SgList = ScatterGather;
    XhciCtlOf(Transfer->Ring)->WriteTd(Transfer);
}

VOID
XhciControlRing::WriteTd(
    _Inout_ XhciControlTransfer* Transfer)
{
    XhciTransferRing* Ring = m_Ring;
    XhciController* Controller = Ring->m_Controller;
    struct _URB_CONTROL_TRANSFER* Urb = &Transfer->Urb->UrbControlTransfer;
    PSCATTER_GATHER_LIST SgList = Transfer->SgList;
    BOOLEAN Short512 = Controller->HasErrata(XhciErrata::CtlSplitDataAt512);
    XhciCtlStep Next = XhciCtlStep::Setup;
    XhciCtlStep Step;
    EventTag* Tag;
    ULONG_PTR TagValue;
    ULONG64 Address;
    XHCI_TRB Trb;
    ULONG Target;
    ULONG Packets;
    ULONG Mapped = 0;
    ULONG Element = 0;
    ULONG Offset = 0;
    ULONG Chunk;
    ULONG Limit;
    ULONG Remaining;
    ULONG TdCount = 0;
    ULONG FirstSlot;
    ULONG Trt;
    BOOLEAN First = TRUE;
    BOOLEAN DirOnNormal = FALSE;
    BOOLEAN LastData;
    BOOLEAN Acknowledge = FALSE;
    volatile UCHAR* CycleByte;
    KIRQL OldIrql;

    Packets = Ring->PacketCount(Transfer->BytesTotal);
    Target = (Ring->m_InterrupterTarget & XHCI_TRB_INTERRUPTER_MASK) << XHCI_TRB_INTERRUPTER_SHIFT;

    Tag = &m_Tags[m_NextTag % TagCount];
    m_NextTag++;
    ASSERT((reinterpret_cast<ULONG_PTR>(Tag) & XhciCtlValueFlagBits) == 0);
    TagValue = reinterpret_cast<ULONG_PTR>(Tag) | USB_ENDPOINT_TYPE_CONTROL;

    FirstSlot = Ring->m_EnqueueIndex;

    /* Keep the whole TD inside the segment when Link TRBs inside a TD are not allowed */
    if (Controller->HasErrata(XhciErrata::RingKeepTdInOneSegment) &&
        Ring->m_EnqueueIndex + XhciCtlWorstCaseTd > Ring->m_LastIndex)
    {
        XhciCtlWrap(Ring, TRUE, FALSE);
        First = FALSE;
    }

    while (Next != XhciCtlStep::Done)
    {
        Step = Next;

        if (Ring->m_EnqueueIndex >= Ring->m_LastIndex)
        {
            XhciCtlWrap(Ring,
                        First,
                        Step == XhciCtlStep::MoreData ||
                        Step == XhciCtlStep::DataEventData ||
                        Step == XhciCtlStep::StatusEventData);
            First = FALSE;
            continue;
        }

        RtlZeroMemory(&Trb, sizeof(Trb));
        Trb.Dword[2] = Target;

        switch (Step)
        {
            case XhciCtlStep::Setup:
            {
                if (Transfer->BytesTotal == 0)
                    Trt = XHCI_TRB_TRT_NO_DATA;
                else
                    Trt = Transfer->DataIn ? XHCI_TRB_TRT_IN : XHCI_TRB_TRT_OUT;

                RtlCopyMemory(&Trb.Dword[0], Urb->SetupPacket, XhciCtlSetupBytes);
                Trb.Dword[2] |= XhciCtlSetupBytes;
                Trb.Dword[3] = XhciCtlTrbType(XhciTrbType::SetupStage) | XHCI_TRB_IDT |
                               (Trt << XHCI_TRB_TRT_SHIFT);
                Next = Transfer->BytesTotal ? XhciCtlStep::FirstData : XhciCtlStep::Status;
                break;
            }

            case XhciCtlStep::FirstData:
            case XhciCtlStep::MoreData:
            {
                if (Step == XhciCtlStep::FirstData)
                {
                    Trb.Dword[3] = XhciCtlTrbType(XhciTrbType::DataStage);
                    if (Transfer->DataIn)
                        Trb.Dword[3] |= XHCI_TRB_DIR_IN;
                }
                else
                {
                    Trb.Dword[3] = XhciCtlTrbType(XhciTrbType::Normal);
                    if (DirOnNormal)
                        Trb.Dword[3] |= XHCI_TRB_DIR_IN;
                }
                Trb.Dword[3] |= XHCI_TRB_CHAIN;

                if (Transfer->Mechanism == XhciControlMechanism::Immediate)
                {
                    RtlCopyMemory(&Trb.Dword[0], Transfer->SystemBuffer, Transfer->BytesTotal);
                    Trb.Dword[3] |= XHCI_TRB_IDT;
                    Chunk = Transfer->BytesTotal;
                    LastData = TRUE;
                }
                else if (Transfer->Mechanism == XhciControlMechanism::DoubleBuffer)
                {
                    if (!Transfer->DataIn)
                    {
                        RtlCopyMemory(Transfer->DoubleBuffer->VirtualAddress,
                                      Transfer->SystemBuffer,
                                      Transfer->BytesTotal);
                    }

                    Address = Transfer->DoubleBuffer->LogicalAddress.QuadPart;
                    Trb.Dword[0] = static_cast<ULONG>(Address);
                    Trb.Dword[1] = static_cast<ULONG>(Address >> 32);
                    Chunk = Transfer->BytesTotal;
                    LastData = TRUE;
                }
                else
                {
                    Remaining = SgList->Elements[Element].Length - Offset;
                    Limit = XhciCtlMaxTrbLength;

                    /* These parts want a short first Data Stage TRB; IN marks what follows */
                    if (Short512 && Step == XhciCtlStep::FirstData && Remaining > XhciCtlShortDataStageTrb)
                    {
                        Limit = XhciCtlShortDataStageTrb;
                        DirOnNormal = Transfer->DataIn;
                    }

                    Address = SgList->Elements[Element].Address.QuadPart + Offset;

                    /* A TRB buffer may not cross a 64 KB boundary (xHCI 6.4.1.1) */
                    Limit = min(Limit, 0x10000 - static_cast<ULONG>(Address & 0xFFFF));

                    Chunk = min(Remaining, Limit);
                    Trb.Dword[0] = static_cast<ULONG>(Address);
                    Trb.Dword[1] = static_cast<ULONG>(Address >> 32);

                    Offset += Chunk;
                    if (Offset >= SgList->Elements[Element].Length)
                    {
                        Element++;
                        Offset = 0;
                    }
                    LastData = (Element >= SgList->NumberOfElements);
                }

                Mapped += Chunk;
                Trb.Dword[2] |= (Chunk & XHCI_TRB_LENGTH_MASK) |
                                (Ring->TdSize(Packets, Mapped, LastData) << XHCI_TRB_TD_SIZE_SHIFT);

                if (LastData)
                {
                    Trb.Dword[3] |= XHCI_TRB_ENT;
                    Next = XhciCtlStep::DataEventData;
                }
                else
                {
                    Next = XhciCtlStep::MoreData;
                }
                break;
            }

            case XhciCtlStep::DataEventData:
            case XhciCtlStep::StatusEventData:
            {
                Address = TagValue;
                if (Step == XhciCtlStep::StatusEventData)
                    Address |= XhciCtlStatusStageFlag;

                Trb.Dword[0] = static_cast<ULONG>(Address);
                Trb.Dword[1] = static_cast<ULONG>(Address >> 32);
                Trb.Dword[3] = XhciCtlTrbType(XhciTrbType::EventData) | XHCI_TRB_IOC;
                TdCount++;
                Next = (Step == XhciCtlStep::DataEventData) ? XhciCtlStep::Status : XhciCtlStep::Done;
                break;
            }

            case XhciCtlStep::Status:
            {
                /* QUIRK: the direction comes from the flags alone, even with no data stage */
                Trb.Dword[3] = XhciCtlTrbType(XhciTrbType::StatusStage) | XHCI_TRB_CHAIN | XHCI_TRB_ENT;
                if (!Transfer->DataIn)
                    Trb.Dword[3] |= XHCI_TRB_DIR_IN;
                Next = XhciCtlStep::StatusEventData;
                break;
            }

            default:
            {
                Next = XhciCtlStep::Done;
                continue;
            }
        }

        XhciCtlPut(Ring, &Trb, First);
        First = FALSE;
    }

    Transfer->FirstIndex = FirstSlot;
    Transfer->EndIndex = Ring->m_EnqueueIndex;

    /* Invalid TRB after the TD so the controller stops there */
    RtlZeroMemory(&Trb, sizeof(Trb));
    Trb.Dword[3] = (Ring->m_Cycle & XHCI_TRB_CYCLE) ^ XHCI_TRB_CYCLE;
    Ring->m_Trbs[Ring->m_EnqueueIndex] = Trb;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);

    if (Transfer->TagArmed)
        m_Tags[Transfer->TagIndex].Transfer = NULL;
    Tag->Transfer = Transfer;
    Transfer->TagIndex = Tag->Index;
    Transfer->TagArmed = TRUE;

    Transfer->TdCount = TdCount;
    Ring->m_DoorbellRung = TRUE;

    if (XhciCtlMapState(Ring) == XhciMapState::Filling)
    {
        XhciCtlSetMapState(Ring, XhciMapState::PausedFill);
    }
    else if (XhciCtlMapState(Ring) == XhciMapState::Halting)
    {
        XhciCtlSetMapState(Ring, XhciMapState::Halted);
        Acknowledge = TRUE;
    }

    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    /* The byte write is the barrier that publishes the TD before its first TRB turns valid */
    KeMemoryBarrier();
    CycleByte = reinterpret_cast<volatile UCHAR*>(&Ring->m_Trbs[FirstSlot]) + 12;
    WRITE_REGISTER_UCHAR(const_cast<PUCHAR>(CycleByte), *CycleByte ^ XHCI_TRB_CYCLE);
    KeMemoryBarrier();

    Ring->RingDoorbell();

    if (Acknowledge)
        Ring->m_Endpoint->OnMappingStopped();
}

/* Completion ****************************************************************/

VOID
XhciControlRing::QueueCompletion()
{
    WdfDpcEnqueue(m_Ring->m_CompletionDpc);
}

_Requires_lock_held_(m_Ring->m_Lock)
VOID
XhciControlRing::CopyIn(
    _Inout_ XhciControlTransfer* Transfer,
    _In_ ULONG Bytes)
{
    if (Transfer->Mechanism == XhciControlMechanism::DoubleBuffer && Transfer->DataIn && Bytes != 0)
        RtlCopyMemory(Transfer->SystemBuffer, Transfer->DoubleBuffer->VirtualAddress, Bytes);
}

/** Completes a request; the ring lock is dropped around the WDF call. */
_Requires_lock_held_(m_Ring->m_Lock)
VOID
XhciControlRing::Finish(
    _Inout_ XhciControlTransfer* Transfer)
{
    XhciTransferRing* Ring = m_Ring;
    WDFREQUEST Request = Transfer->Request;
    PURB Urb = Transfer->Urb;
    USBD_STATUS UsbdStatus;
    NTSTATUS Status;

    if (Transfer->CompletionCode == static_cast<ULONG>(XhciCompletionCode::Invalid))
    {
        if (Transfer->CancelState == XhciControlCancel::Done ||
            Transfer->TimeoutState == XhciControlTimeout::Expired)
        {
            Transfer->Status = STATUS_CANCELLED;
        }
        else if (Transfer->Status == STATUS_PENDING)
        {
            /* A request still marked pending completes as canceled */
            Transfer->Status = STATUS_CANCELLED;
        }

        UsbdStatus = XhciCtlUsbdStatusFromNt(Transfer->Status);
    }
    else
    {
        UsbdStatus = XhciTransferRing::UsbdStatusFromCompletionCode(Transfer->CompletionCode);
        Transfer->Status = XhciTransferRing::NtStatusFromUsbdStatus(UsbdStatus);
    }

    Status = Transfer->Status;
    Urb->UrbHeader.Status = UsbdStatus;
    Urb->UrbControlTransfer.TransferBufferLength = Transfer->BytesTransferred;

    DPRINT("Control request %p done, status 0x%lx, URB status 0x%lx, %lu of %lu bytes\n",
           Request, Status, UsbdStatus, Transfer->BytesTransferred, Transfer->BytesTotal);

    KeReleaseSpinLockFromDpcLevel(&Ring->m_Lock);

    ReleaseResources(Transfer);
    Transfer->Initialized = FALSE;
    WdfRequestComplete(Request, Status);

    KeAcquireSpinLockAtDpcLevel(&Ring->m_Lock);
}

/** Completes the held request unless a cancel or timer callback still has to run. */
_Requires_lock_held_(m_Ring->m_Lock)
NTSTATUS
XhciControlRing::CompleteHeld()
{
    XhciControlTransfer* Transfer = m_Current;
    NTSTATUS Status;

    if (Transfer->CancelState == XhciControlCancel::Armed)
    {
        Status = WdfRequestUnmarkCancelable(Transfer->Request);
        if (!NT_SUCCESS(Status))
        {
            Transfer->CancelState = XhciControlCancel::CancelRoutinePending;
            return STATUS_CANCELLED;
        }
        Transfer->CancelState = XhciControlCancel::Idle;
    }

    if (Transfer->TimeoutState == XhciControlTimeout::Armed)
    {
        if (!WdfTimerStop(m_Timer, FALSE))
        {
            Transfer->TimeoutState = XhciControlTimeout::TimerRoutinePending;
            return STATUS_CANCELLED;
        }
        Transfer->TimeoutState = XhciControlTimeout::Idle;
    }

    if (Transfer->TagArmed)
    {
        m_Tags[Transfer->TagIndex].Transfer = NULL;
        Transfer->TagArmed = FALSE;
    }

    m_Current = NULL;
    Finish(Transfer);
    return STATUS_SUCCESS;
}

/* Cancel and timeout ********************************************************/

/** Shared by the cancel callback and the timeout timer; both act like a cancel. */
VOID
XhciControlRing::Abort(
    _Inout_ XhciControlTransfer* Transfer,
    _In_ BOOLEAN FromTimer)
{
    XhciTransferRing* Ring = m_Ring;
    BOOLEAN Reported = FALSE;
    BOOLEAN Queue = FALSE;
    BOOLEAN Pending;
    BOOLEAN Lost;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    if (FromTimer)
        Pending = (Transfer->TimeoutState == XhciControlTimeout::Armed);
    else
        Pending = (Transfer->CancelState == XhciControlCancel::Armed);

    if (Pending)
    {
        if (m_ReclaimOnCancelAllowed)
            Queue = TRUE;
        else
            Reported = TRUE;
    }
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    if (Reported)
        Ring->m_Endpoint->OnTransferCanceled();

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    if (FromTimer)
        Lost = (Transfer->TimeoutState == XhciControlTimeout::TimerRoutinePending);
    else
        Lost = (Transfer->CancelState == XhciControlCancel::CancelRoutinePending);

    /* A completion left this request to us. After a reported cancel it waits for reclaim, or that permission would complete the next request */
    if (Lost && (!Reported || m_ReclaimAndAcknowledge || m_ReclaimOnCancelAllowed))
        Queue = TRUE;

    /* Last, so nothing completes the request while this callback still uses it */
    if (FromTimer)
        Transfer->TimeoutState = XhciControlTimeout::Expired;
    else
        Transfer->CancelState = XhciControlCancel::Done;
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    if (Queue)
        QueueCompletion();
}

VOID
XhciControlRing::EvtRequestCancel(
    _In_ WDFREQUEST Request)
{
    XhciControlTransfer* Transfer = &XhciGetRequestData(Request)->Control;

    DPRINT("Control request %p cancel\n", Request);
    XhciCtlOf(Transfer->Ring)->Abort(Transfer, FALSE);
}

VOID
XhciControlRing::EvtTimeout(
    _In_ WDFTIMER Timer)
{
    XhciTransferRing* Ring = XhciTransferRing::FromQueue(static_cast<WDFQUEUE>(WdfTimerGetParentObject(Timer)));
    XhciControlRing* Self = XhciCtlOf(Ring);
    XhciControlTransfer* Transfer;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    Transfer = Self->m_Current;
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    if (Transfer == NULL)
        return;

    DPRINT("Control request %p timed out\n", Transfer->Request);
    Self->Abort(Transfer, TRUE);
}

VOID
XhciControlRing::EvtIoCanceledOnQueue(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request)
{
    XhciTransferRing* Ring = XhciTransferRing::FromQueue(Queue);
    XhciControlRing* Self = XhciCtlOf(Ring);
    XhciControlTransfer* Transfer;
    KIRQL OldIrql;

    if (XhciGetRequestData(Request) == NULL)
    {
        DPRINT1("Control request %p canceled on the queue has no driver context\n", Request);
        WdfRequestComplete(Request, STATUS_CANCELLED);
        return;
    }

    Transfer = &XhciGetRequestData(Request)->Control;
    Self->InitTransfer(Transfer, Request);
    Transfer->CancelState = XhciControlCancel::Done;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    InsertTailList(&Self->m_CanceledOnQueue, &Transfer->Link);
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    /* Requests canceled in the queue are completed from a DPC */
    WdfDpcEnqueue(Self->m_CanceledOnQueueDpc);
}

VOID
XhciControlRing::EvtCanceledOnQueueDpc(
    _In_ WDFDPC Dpc)
{
    XhciTransferRing* Ring = XhciTransferRing::FromQueue(static_cast<WDFQUEUE>(WdfDpcGetParentObject(Dpc)));
    XhciControlRing* Self = XhciCtlOf(Ring);
    XhciControlTransfer* Transfer;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);

    if (!Self->m_CanceledDpcRunning)
    {
        Self->m_CanceledDpcRunning = TRUE;

        while (!IsListEmpty(&Self->m_CanceledOnQueue))
        {
            Transfer = CONTAINING_RECORD(RemoveHeadList(&Self->m_CanceledOnQueue), XhciControlTransfer, Link);
            XhciClearListEntry(&Transfer->Link);
            Self->Finish(Transfer);
        }

        Self->m_CanceledDpcRunning = FALSE;
    }

    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
}

VOID
XhciControlRing::EvtCompletionDpc(
    _In_ WDFDPC Dpc)
{
    XhciTransferRing* Ring = XhciTransferRing::FromQueue(static_cast<WDFQUEUE>(WdfDpcGetParentObject(Dpc)));
    XhciControlRing* Self = XhciCtlOf(Ring);
    BOOLEAN Reclaimed = FALSE;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);

    /* A failed completion is retried when the pending cancel or timer callback queues us again */
    if (Self->m_Current != NULL && NT_SUCCESS(Self->CompleteHeld()))
        Reclaimed = Self->m_ReclaimAndAcknowledge;

    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    if (Reclaimed)
        Ring->m_Endpoint->OnTransfersReclaimed();
}

/* Endpoint machine directives ***********************************************/

VOID
XhciControlRing::ResumeRingFill(
    _In_ XhciTransferRing* Ring)
{
    XhciControlRing* Self = XhciCtlOf(Ring);
    XhciControlTransfer* Transfer;
    BOOLEAN Queue = FALSE;
    BOOLEAN Map = FALSE;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);

    Ring->m_DoorbellRung = FALSE;
    Self->m_ReclaimAndAcknowledge = FALSE;
    Self->m_AcknowledgeExpectedEvents = FALSE;
    Self->m_ReclaimOnCancelAllowed = FALSE;
    Self->m_Outstanding = 0;

    if (XhciCtlMapState(Ring) != XhciMapState::Halted)
        DPRINT1("Control ring %p: mapping started in map state %ld\n", Ring, Ring->m_MapState);

    Transfer = Self->m_Current;
    if (Transfer == NULL)
    {
        XhciCtlSetMapState(Ring, XhciMapState::PausedFill);
    }
    else if (Transfer->CancelState == XhciControlCancel::Done ||
             Transfer->TimeoutState == XhciControlTimeout::Expired)
    {
        /* Its cancel or timeout was reported before this start and the endpoint dropped that report */
        XhciCtlSetMapState(Ring, XhciMapState::PausedFill);
        Queue = TRUE;
    }
    else
    {
        XhciCtlSetMapState(Ring, XhciMapState::Filling);
        Map = TRUE;
    }

    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    if (Queue)
        Self->QueueCompletion();

    /* QUIRK: a held request is mapped from scratch, which only happens if it never reached the hardware */
    if (Map)
        Self->BuildControlTd(Transfer);
}

VOID
XhciControlRing::SuspendRingFill(
    _In_ XhciTransferRing* Ring)
{
    BOOLEAN Acknowledge = FALSE;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);

    if (XhciCtlMapState(Ring) == XhciMapState::Filling)
    {
        /* The mapping context acknowledges once it is done */
        XhciCtlSetMapState(Ring, XhciMapState::Halting);
    }
    else
    {
        ASSERT(XhciCtlMapState(Ring) == XhciMapState::PausedFill);
        XhciCtlSetMapState(Ring, XhciMapState::Halted);
        Acknowledge = TRUE;
    }

    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    if (Acknowledge)
        Ring->m_Endpoint->OnMappingStopped();
}

VOID
XhciControlRing::AllowReclaimOnCancel(
    _In_ XhciTransferRing* Ring)
{
    XhciControlRing* Self = XhciCtlOf(Ring);
    BOOLEAN Queue;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    Queue = (Self->m_Current != NULL &&
             (Self->m_Current->CancelState == XhciControlCancel::Done ||
              Self->m_Current->TimeoutState == XhciControlTimeout::Expired));
    Self->m_ReclaimOnCancelAllowed = TRUE;
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    if (Queue)
        Self->QueueCompletion();
}

VOID
XhciControlRing::ConsumePendingEvents(
    _In_ XhciTransferRing* Ring)
{
    XhciControlRing* Self = XhciCtlOf(Ring);
    BOOLEAN Drained;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    Drained = (Self->m_Outstanding == 0);
    if (!Drained)
        Self->m_AcknowledgeExpectedEvents = TRUE;
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    if (Drained)
        Ring->m_Endpoint->OnExpectedEventsProcessed();
}

VOID
XhciControlRing::RecoverTransfers(
    _In_ XhciTransferRing* Ring)
{
    XhciControlRing* Self = XhciCtlOf(Ring);
    BOOLEAN Held;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);
    Held = (Self->m_Current != NULL);
    if (Held)
        Self->m_ReclaimAndAcknowledge = TRUE;
    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    if (Held)
        Self->QueueCompletion();
    else
        Ring->m_Endpoint->OnTransfersReclaimed();
}

BOOLEAN
XhciControlRing::DoorbellRungSinceFill(
    _In_ const XhciTransferRing* Ring)
{
    XhciTransferRing* Writable = const_cast<XhciTransferRing*>(Ring);
    BOOLEAN Rung;
    KIRQL OldIrql;

    ASSERT(XhciCtlMapState(Ring) == XhciMapState::Halted);

    KeAcquireSpinLock(&Writable->m_Lock, &OldIrql);
    Rung = Writable->m_DoorbellRung;
    KeReleaseSpinLock(&Writable->m_Lock, OldIrql);

    return Rung;
}

BOOLEAN
XhciControlRing::HasQueuedWork(
    _In_ const XhciTransferRing* Ring)
{
    return Ring->m_Type.Control.m_Current != NULL;
}

/* Transfer events ***********************************************************/

XhciControlRing::EventTag*
XhciControlRing::TagFromEvent(
    _In_opt_ XhciTransferRing* Ring,
    _In_ const XHCI_TRB* Event)
{
    ULONG_PTR Address = static_cast<ULONG_PTR>(XhciTrbPointer(Event)) & ~XhciCtlValueFlagBits;
    EventTag* Tag = reinterpret_cast<EventTag*>(Address);
    XhciControlRing* Owner;
    ULONG Index;

    if (Address == 0)
        return NULL;

    /* With the ring known, only compare addresses; nothing is read through the value */
    if (Ring != NULL)
    {
        Owner = XhciCtlOf(Ring);
        for (Index = 0; Index < TagCount; Index++)
        {
            if (Tag == &Owner->m_Tags[Index])
                return Tag;
        }
        return NULL;
    }

    if (Tag->Signature != XhciCtlTagSignature || Tag->Index >= TagCount || Tag->Ring == NULL)
        return NULL;

    if (&XhciCtlOf(Tag->Ring)->m_Tags[Tag->Index] != Tag)
        return NULL;

    return Tag;
}

BOOLEAN
XhciControlRing::OnTransferEvent(
    _In_opt_ XhciTransferRing* Ring,
    _In_ XhciController* Controller,
    _In_ const XHCI_TRB* Event)
{
    EventTag* Tag;

    UNREFERENCED_PARAMETER(Controller);

    if (!(Event->Dword[3] & XHCI_TRANSFER_EVENT_ED))
    {
        /* Events without Event Data only come through the endpoint, with the ring known */
        if (Ring == NULL)
            return FALSE;

        return XhciCtlOf(Ring)->OnTrbEvent(Event);
    }

    Tag = TagFromEvent(Ring, Event);
    if (Tag == NULL)
    {
        if (Ring == NULL)
        {
            DPRINT1("Control Event Data value 0x%I64x names no live transfer, dropped\n",
                    XhciTrbPointer(Event));
            return FALSE;
        }

        DPRINT1("Control ring %p: Event Data value 0x%I64x is not one of its own, dropped\n",
                Ring, XhciTrbPointer(Event));
        return FALSE;
    }

    return XhciCtlOf(Tag->Ring)->OnEventDataEvent(Tag, Event);
}

BOOLEAN
XhciControlRing::OnEventDataEvent(
    _In_ EventTag* Tag,
    _In_ const XHCI_TRB* Event)
{
    XhciTransferRing* Ring = m_Ring;
    XhciControlTransfer* Transfer;
    ULONG Code = XhciTrbCompletionCode(Event);
    ULONG Bytes = Event->Dword[2] & XHCI_TRANSFER_EVENT_LENGTH_MASK;
    ULONG EndpointId = (Event->Dword[3] & XHCI_TRB_ENDPOINT_MASK) >> XHCI_TRB_ENDPOINT_SHIFT;
    BOOLEAN StatusStage = (XhciTrbPointer(Event) & XhciCtlStatusStageFlag) != 0;
    BOOLEAN Halted = FALSE;
    BOOLEAN Stopped = FALSE;
    BOOLEAN Drained = FALSE;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);

    Transfer = Tag->Transfer;
    if (Transfer == NULL || Transfer != m_Current)
    {
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

        DPRINT1("Control ring %p: stale Event Data event, code %lu, dropped\n", Ring, Code);
        return TRUE;
    }

    if (EndpointId != Ring->m_Endpoint->Dci() || XhciTrbSlotId(Event) != Ring->m_Device->SlotId())
    {
        DPRINT1("Control ring %p: event for slot %lu endpoint %lu, ring is slot %lu endpoint %lu\n",
                Ring, XhciTrbSlotId(Event), EndpointId, Ring->m_Device->SlotId(), Ring->m_Endpoint->Dci());
    }

    if (Bytes > Transfer->BytesTotal)
    {
        DPRINT1("Control ring %p: %lu bytes reported for a %lu byte request\n",
                Ring, Bytes, Transfer->BytesTotal);
        Bytes = 0;
    }

    if (!StatusStage)
    {
        Transfer->BytesTransferred = Bytes;
        CopyIn(Transfer, Bytes);
    }

    if (XhciCtlIsHaltCode(Code))
    {
        /* The endpoint resets the pipe, then reclaim completes the request */
        Transfer->CompletionCode = Code;
        Halted = TRUE;
    }
    else if (XhciCtlIsStopCode(Code))
    {
        Transfer->EventsReceived++;

        if (StatusStage)
        {
            if (Transfer->BytesTransferred == Transfer->BytesTotal)
                Transfer->CompletionCode = static_cast<ULONG>(XhciCompletionCode::Success);
            else if (Code == static_cast<ULONG>(XhciCompletionCode::StoppedShortPacket))
                Transfer->CompletionCode = Code;

            m_Outstanding = max(static_cast<LONG>(Transfer->TdCount) - static_cast<LONG>(Transfer->EventsReceived), 0L);
        }
        else
        {
            /* QUIRK: a stop on the data stage expects no further events */
            m_Outstanding = 0;
        }

        Stopped = TRUE;
    }
    else
    {
        Transfer->EventsReceived++;

        if (StatusStage)
        {
            Transfer->CompletionCode = Code;
            if (Transfer->CancelState != XhciControlCancel::Done &&
                Transfer->TimeoutState != XhciControlTimeout::Expired)
            {
                CompleteHeld();
            }
        }

        if (m_Outstanding > 0)
        {
            m_Outstanding--;
            Drained = (m_Outstanding == 0 && m_AcknowledgeExpectedEvents);
        }
    }

    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    if (Halted)
        Ring->m_Endpoint->OnHaltedCompletionCode(Code, TRUE);

    /* QUIRK: the drop stopped event flag is only consulted for events without Event Data */
    if (Stopped)
        Ring->m_Endpoint->OnStoppedEvent();

    if (Drained)
        Ring->m_Endpoint->OnExpectedEventsProcessed();

    return TRUE;
}

BOOLEAN
XhciControlRing::LocateTrb(
    _In_ const XhciControlTransfer* Transfer,
    _In_ const XHCI_TRB* Event,
    _Out_ PULONG Bytes,
    _Out_ PULONG SkippedTds) const
{
    const XhciTransferRing* Ring = m_Ring;
    ULONG64 Pointer = XhciTrbPointer(Event);
    ULONG64 Base = Ring->SegmentBase();
    ULONG EventLength = Event->Dword[2] & XHCI_TRANSFER_EVENT_LENGTH_MASK;
    XhciCompletionCode Code = static_cast<XhciCompletionCode>(XhciTrbCompletionCode(Event));
    const XHCI_TRB* Trb;
    ULONG Index = Transfer->FirstIndex;
    ULONG Visited;
    ULONG Length;
    ULONG Type;
    BOOLEAN Match;

    *Bytes = 0;
    *SkippedTds = 0;

    if (Pointer == 0)
        return FALSE;

    /* Stopped on an empty ring: every TD of the request already ran */
    if (Pointer == Ring->EnqueuePointer())
    {
        *SkippedTds = Transfer->TdCount;
        return TRUE;
    }

    /* Walk the TD from its first slot to its end slot, wrapping at the Link slot */
    for (Visited = 0; Visited <= Ring->m_LastIndex; Visited++)
    {
        Trb = &Ring->m_Trbs[Index];
        Type = XhciTrbType(Trb);
        Length = Trb->Dword[2] & XHCI_TRB_LENGTH_MASK;
        Match = (Base + static_cast<ULONG64>(Index) * sizeof(*Trb) == Pointer);

        /* A link written in mid segment jumps to the start, skipping the stale slots after it */
        if (Type == static_cast<ULONG>(XhciTrbType::Link) && !Match && Index != Transfer->EndIndex)
        {
            Index = 0;
            continue;
        }

        if (Type == static_cast<ULONG>(XhciTrbType::Normal) || Type == static_cast<ULONG>(XhciTrbType::DataStage))
        {
            if (!Match)
            {
                *Bytes += Length;
            }
            else if (Code == XhciCompletionCode::StoppedShortPacket)
            {
                *Bytes = EventLength;
            }
            else if (Code == XhciCompletionCode::Stopped ||
                     Code == XhciCompletionCode::DataBufferError ||
                     Code == XhciCompletionCode::BabbleDetected ||
                     Code == XhciCompletionCode::StallError ||
                     Code == XhciCompletionCode::UsbTransactionError ||
                     Code == XhciCompletionCode::SplitTransactionError)
            {
                /* The event carries the residual of this TRB */
                if (EventLength <= Length)
                    *Bytes += Length - EventLength;
            }
        }
        else if (Type == static_cast<ULONG>(XhciTrbType::EventData) && !Match)
        {
            /* That TD finished and reported its own byte count */
            *Bytes = 0;
            (*SkippedTds)++;
        }

        if (Match)
            return TRUE;

        if (Index == Transfer->EndIndex)
            break;

        Index = (Index >= Ring->m_LastIndex) ? 0 : Index + 1;
    }

    return FALSE;
}

BOOLEAN
XhciControlRing::OnTrbEvent(
    _In_ const XHCI_TRB* Event)
{
    XhciTransferRing* Ring = m_Ring;
    XhciController* Controller = Ring->m_Controller;
    XhciControlTransfer* Transfer;
    ULONG Code = XhciTrbCompletionCode(Event);
    ULONG64 Pointer = XhciTrbPointer(Event);
    ULONG64 Base;
    ULONG Bytes;
    ULONG Skipped;
    LONG Outstanding;
    KIRQL OldIrql;

    if (XhciCtlIsStopCode(Code) && Ring->m_Endpoint->ShouldDropStoppedEvent())
        return TRUE;

    KeAcquireSpinLock(&Ring->m_Lock, &OldIrql);

    Transfer = m_Current;
    if (Transfer == NULL)
    {
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);
        DPRINT("Control ring %p: event code %lu with no request held\n", Ring, Code);
        return FALSE;
    }

    if (!LocateTrb(Transfer, Event, &Bytes, &Skipped))
    {
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

        Base = Ring->SegmentBase();
        if (Pointer >= Base && Pointer < Base + Ring->m_SegmentSize)
        {
            if (Controller->HasErrata(XhciErrata::EvtDiscardRepeatedEd0))
            {
                DPRINT1("Control ring %p: TRB 0x%I64x is outside the request, likely a duplicate, dropped\n",
                        Ring, Pointer);
            }
            else
            {
                DPRINT1("Control ring %p: TRB 0x%I64x is outside the request, likely a duplicate, resetting\n",
                        Ring, Pointer);
                Controller->RaiseControllerFault(XhciRecovery::ResetHost, XhciCtlReasonDuplicateEvent);
            }
        }
        else
        {
            DPRINT1("Control ring %p: TRB 0x%I64x is not on the ring, resetting\n", Ring, Pointer);
            Controller->RaiseControllerFault(XhciRecovery::ResetHost, XhciCtlReasonBadTrbPointer);
        }
        return FALSE;
    }

    if (Bytes > Transfer->BytesTotal)
    {
        DPRINT1("Control ring %p: %lu bytes found for a %lu byte request\n", Ring, Bytes, Transfer->BytesTotal);
        Bytes = 0;
    }

    /* Zero keeps a count an earlier data stage event already reported */
    if (Bytes != 0)
    {
        Transfer->BytesTransferred = Bytes;
        CopyIn(Transfer, Bytes);
    }

    if (XhciCtlIsHaltCode(Code))
    {
        Transfer->CompletionCode = Code;
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

        Ring->m_Endpoint->OnHaltedCompletionCode(Code, TRUE);
        return TRUE;
    }

    if (XhciCtlIsStopCode(Code))
    {
        if (Transfer->BytesTransferred == Transfer->BytesTotal)
            Transfer->CompletionCode = static_cast<ULONG>(XhciCompletionCode::Success);
        else if (Code == static_cast<ULONG>(XhciCompletionCode::StoppedShortPacket))
            Transfer->CompletionCode = Code;

        Transfer->EventsReceived++;
        Outstanding = static_cast<LONG>(Skipped + 1) - static_cast<LONG>(Transfer->EventsReceived);
        m_Outstanding = max(Outstanding, 0L);
        KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

        Ring->m_Endpoint->OnStoppedEvent();
        return TRUE;
    }

    KeReleaseSpinLock(&Ring->m_Lock, OldIrql);

    DPRINT1("Control ring %p: unexpected code %lu for TRB 0x%I64x\n", Ring, Code, Pointer);
    return FALSE;
}
