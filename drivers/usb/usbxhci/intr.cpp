/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Interrupt resources, event rings and event dispatch
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "xhcidrv.h"

#define NDEBUG
#include <debug.h>

typedef XhciInterrupters::Interrupter XHCI_INTERRUPTER, *PXHCI_INTERRUPTER;

typedef struct _XHCI_INTERRUPT_CONTEXT
{
    PXHCI_INTERRUPTER Entry;
} XHCI_INTERRUPT_CONTEXT, *PXHCI_INTERRUPT_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(XHCI_INTERRUPT_CONTEXT, XhciInterruptContext)

/* Hard limits */
#define XHCI_SECONDARY_MAX              16
#define XHCI_SEGMENTS_DEFAULT           4
#define XHCI_SEGMENTS_RING_FULL         8
#define XHCI_SEGMENTS_LIMITED           2
#define XHCI_BUDGET_CHECK_EVENTS        64
#define XHCI_HANDBACK_EVENTS            128
#define XHCI_REASON_EVENT_RING_FULL     0x1011
#define XHCI_DRAIN_WAIT_MS              60000

/* Group aware processor routines are not exported by every kernel; resolved once */
typedef ULONG (NTAPI *PFN_XHCI_ACTIVE_COUNT)(_In_ USHORT GroupNumber);
typedef ULONG (NTAPI *PFN_XHCI_INDEX_FROM_NUMBER)(_In_ PPROCESSOR_NUMBER Number);
typedef ULONG (NTAPI *PFN_XHCI_CURRENT_NUMBER)(_Out_opt_ PPROCESSOR_NUMBER Number);
typedef NTSTATUS (NTAPI *PFN_XHCI_DPC_WATCHDOG)(_Out_ PKDPC_WATCHDOG_INFORMATION Information);

static PFN_XHCI_ACTIVE_COUNT XhciActiveCountRoutine;
static PFN_XHCI_INDEX_FROM_NUMBER XhciIndexFromNumberRoutine;
static PFN_XHCI_CURRENT_NUMBER XhciCurrentNumberRoutine;
static PFN_XHCI_DPC_WATCHDOG XhciDpcWatchdogRoutine;

static EVT_WDF_INTERRUPT_ISR XhciEvtInterruptIsr;
static EVT_WDF_INTERRUPT_DPC XhciEvtInterruptDpc;
static EVT_WDF_INTERRUPT_ENABLE XhciEvtInterruptEnable;
static EVT_WDF_INTERRUPT_DISABLE XhciEvtInterruptDisable;
static EVT_WDF_WORKITEM XhciEvtRequeueWorkItem;

/* Processor helpers **********************************************************/

_IRQL_requires_(PASSIVE_LEVEL)
static
PVOID
NTAPI
XhciFindKernelRoutine(
    _In_ PCWSTR Name)
{
    UNICODE_STRING RoutineName;

    RtlInitUnicodeString(&RoutineName, Name);
    return MmGetSystemRoutineAddress(&RoutineName);
}

static
ULONG
NTAPI
XhciActiveProcessors(
    _In_ USHORT Group)
{
    if (XhciActiveCountRoutine != NULL)
        return XhciActiveCountRoutine(Group);

    return KeQueryActiveProcessorCount(NULL);
}

static
ULONG
NTAPI
XhciProcessorIndex(
    _In_ USHORT Group,
    _In_ ULONG Number)
{
    PROCESSOR_NUMBER Processor;

    if (XhciIndexFromNumberRoutine != NULL)
    {
        RtlZeroMemory(&Processor, sizeof(Processor));
        Processor.Group = Group;
        Processor.Number = (UCHAR)Number;
        return XhciIndexFromNumberRoutine(&Processor);
    }

    /* Without group support only group 0 exists and the index is the number */
    return (Group == 0) ? Number : MAXULONG;
}

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4996)
#endif

static
ULONG
NTAPI
XhciCurrentProcessorIndex(VOID)
{
    if (XhciCurrentNumberRoutine != NULL)
        return XhciCurrentNumberRoutine(NULL);

    /* Single group kernels: the number is the index */
    return KeGetCurrentProcessorNumber();
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

/* Interrupter register access ************************************************/

static
ULONG
NTAPI
XhciReadInterrupter(
    _In_ PXHCI_INTERRUPTER Entry,
    _In_ ULONG Offset)
{
    return Entry->Controller->m_Registers.ReadRuntime32(Entry->RegisterOffset + Offset);
}

static
VOID
NTAPI
XhciWriteInterrupter(
    _In_ PXHCI_INTERRUPTER Entry,
    _In_ ULONG Offset,
    _In_ ULONG Value)
{
    Entry->Controller->m_Registers.WriteRuntime32(Entry->RegisterOffset + Offset, Value);
}

static
VOID
NTAPI
XhciWriteInterrupter64(
    _In_ PXHCI_INTERRUPTER Entry,
    _In_ ULONG Offset,
    _In_ ULONG64 Value)
{
    Entry->Controller->m_Registers.WriteRuntime64(Entry->RegisterOffset + Offset, Value);
}

/** Writes ERDP from the software dequeue position; ClearBusy also writes EHB to clear it. */
static
VOID
NTAPI
XhciWriteDequeue(
    _In_ PXHCI_INTERRUPTER Entry,
    _In_ BOOLEAN ClearBusy)
{
    ULONG64 Value;

    Value = (ULONG64)Entry->Segment->LogicalAddress.QuadPart +
            (ULONG64)Entry->DequeueIndex * sizeof(XHCI_TRB);
    Value |= Entry->DequeueSegment & XHCI_ERDP_SEGMENT_MASK;

    if (ClearBusy)
        Value |= XHCI_ERDP_BUSY;

    XhciWriteInterrupter64(Entry, XHCI_ERDP, Value);
}

/** Zeroes the event ring and puts the consumer back at the start (xHC halted). */
static
VOID
NTAPI
XhciResetEventRing(
    _Inout_ PXHCI_INTERRUPTER Entry)
{
    PLIST_ENTRY Link;
    XhciDmaBuffer* Buffer;

    for (Link = Entry->SegmentList.Flink; Link != &Entry->SegmentList; Link = Link->Flink)
    {
        Buffer = CONTAINING_RECORD(Link, XhciDmaBuffer, Link);
        RtlZeroMemory(Buffer->VirtualAddress, Entry->SegmentSize);
    }

    Entry->Segment = CONTAINING_RECORD(Entry->SegmentList.Flink, XhciDmaBuffer, Link);
    Entry->CycleState = XHCI_TRB_CYCLE;
    Entry->DequeueIndex = 0;
    Entry->DequeueSegment = 0;

    /* A requeue skipped at D0 exit can leave this set */
    Entry->DpcRunning = FALSE;
}

/** ERSTSZ, ERDP, then ERSTBA, which arms the event ring (xHCI 4.9.4). */
static
VOID
NTAPI
XhciProgramEventRing(
    _In_ PXHCI_INTERRUPTER Entry)
{
    XhciWriteInterrupter(Entry, XHCI_ERSTSZ, Entry->SegmentCount);
    XhciWriteDequeue(Entry, TRUE);
    XhciWriteInterrupter64(Entry, XHCI_ERSTBA, (ULONG64)Entry->Erst->LogicalAddress.QuadPart);
}

static
VOID
NTAPI
XhciUnmaskInterrupter(
    _In_ PXHCI_INTERRUPTER Entry)
{
    ULONG Iman;

    XhciWriteInterrupter(Entry, XHCI_IMOD, XHCI_IMOD_INTERVAL_50US);

    /* IP is write one to clear, so writing back what was read also drops a stale IP */
    Iman = XhciReadInterrupter(Entry, XHCI_IMAN);
    XhciWriteInterrupter(Entry, XHCI_IMAN, Iman | XHCI_IMAN_ENABLE);
}

/* Interrupt callbacks ********************************************************/

static
BOOLEAN
NTAPI
XhciEvtInterruptIsr(
    _In_ WDFINTERRUPT Interrupt,
    _In_ ULONG MessageID)
{
    PXHCI_INTERRUPTER Entry;
    ULONG Iman;

    Entry = XhciInterruptContext(Interrupt)->Entry;
    if (Entry == NULL)
        return FALSE;

    /* A shared line needs IMAN to tell whether this controller raised it */
    if (MessageID == 0 && !Entry->MessageSignaled)
    {
        Iman = XhciReadInterrupter(Entry, XHCI_IMAN);
        if ((Iman & XHCI_IMAN_PENDING) == 0)
            return FALSE;

        XhciWriteInterrupter(Entry, XHCI_IMAN, Iman);
    }

    WdfInterruptQueueDpcForIsr(Interrupt);
    return TRUE;
}

static
NTSTATUS
NTAPI
XhciEvtInterruptEnable(
    _In_ WDFINTERRUPT Interrupt,
    _In_ WDFDEVICE AssociatedDevice)
{
    PXHCI_INTERRUPTER Entry;

    UNREFERENCED_PARAMETER(AssociatedDevice);

    Entry = XhciInterruptContext(Interrupt)->Entry;
    if (Entry == NULL)
        return STATUS_SUCCESS;

    Entry->Enabled = TRUE;
    Entry->PendingDisable = FALSE;

    /* Drop a stale DpcRunning left by a requeue skipped at D0 exit */
    Entry->DpcRunning = FALSE;

    XhciUnmaskInterrupter(Entry);
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
XhciEvtInterruptDisable(
    _In_ WDFINTERRUPT Interrupt,
    _In_ WDFDEVICE AssociatedDevice)
{
    PXHCI_INTERRUPTER Entry;
    ULONG Iman;

    UNREFERENCED_PARAMETER(AssociatedDevice);

    Entry = XhciInterruptContext(Interrupt)->Entry;
    if (Entry == NULL)
        return STATUS_SUCCESS;

    Entry->Enabled = FALSE;

    Iman = XhciReadInterrupter(Entry, XHCI_IMAN);
    XhciWriteInterrupter(Entry, XHCI_IMAN, Iman & ~XHCI_IMAN_ENABLE);
    return STATUS_SUCCESS;
}

/* Event processing ***********************************************************/

/** Remaining DPC budget check. Limits are computed on the first successful query. */
static
BOOLEAN
NTAPI
XhciDpcBudgetSpent(
    _Inout_ PBOOLEAN LimitsKnown,
    _Inout_ PULONG TimeLimit,
    _Inout_ PULONG WatchdogLimit)
{
    KDPC_WATCHDOG_INFORMATION Watchdog;

    /* ReactOS may not export it; the DPC then never yields */
    if (XhciDpcWatchdogRoutine == NULL)
        return FALSE;

    if (!NT_SUCCESS(XhciDpcWatchdogRoutine(&Watchdog)))
        return FALSE;

    if (!*LimitsKnown)
    {
        *TimeLimit = (Watchdog.DpcTimeLimit * 95) / 100;
        *WatchdogLimit = (Watchdog.DpcWatchdogLimit * 25) / 100;
        *LimitsKnown = TRUE;
    }

    /* The counts are ticks left, so this yields early on purpose */
    if (Watchdog.DpcTimeCount < *TimeLimit)
        return TRUE;

    return (Watchdog.DpcWatchdogCount < *WatchdogLimit);
}

static
PXHCI_TRB
NTAPI
XhciDequeueTrb(
    _In_ PXHCI_INTERRUPTER Entry)
{
    return (PXHCI_TRB)Entry->Segment->VirtualAddress + Entry->DequeueIndex;
}

static
BOOLEAN
NTAPI
XhciEventPending(
    _In_ PXHCI_INTERRUPTER Entry,
    _In_ PXHCI_TRB Trb)
{
    ULONG Control;

    Control = *(volatile ULONG*)&Trb->Dword[3];
    return (Control & XHCI_TRB_CYCLE) == Entry->CycleState;
}

static
VOID
NTAPI
XhciAdvanceDequeue(
    _Inout_ PXHCI_INTERRUPTER Entry)
{
    Entry->DequeueIndex++;
    if (Entry->DequeueIndex < Entry->TrbsPerSegment)
        return;

    Entry->DequeueIndex = 0;
    Entry->DequeueSegment++;

    if (Entry->DequeueSegment < Entry->SegmentCount)
    {
        Entry->Segment = CONTAINING_RECORD(Entry->Segment->Link.Flink, XhciDmaBuffer, Link);
        return;
    }

    /* Wrapped past the last ERST entry: the producer toggled its cycle state too */
    Entry->DequeueSegment = 0;
    Entry->CycleState ^= XHCI_TRB_CYCLE;
    Entry->Segment = CONTAINING_RECORD(Entry->SegmentList.Flink, XhciDmaBuffer, Link);
}

static
VOID
NTAPI
XhciRouteTransferEvent(
    _In_ PXHCI_INTERRUPTER Entry,
    _Inout_ PXHCI_TRB Event)
{
    XhciController* Controller = Entry->Controller;
    ULONG Code = XhciTrbCompletionCode(Event);
    BOOLEAN Isoch;
    BOOLEAN Direct;

    if ((Event->Dword[3] & XHCI_TRANSFER_EVENT_ED) &&
        Controller->HasErrata(XhciErrata::IsochStarveEventNoEd) &&
        (Code == static_cast<ULONG>(XhciCompletionCode::RingUnderrun) ||
         Code == static_cast<ULONG>(XhciCompletionCode::RingOverrun)))
    {
        /* These parts report ring empty with ED set and a zero pointer */
        Event->Dword[3] &= ~XHCI_TRANSFER_EVENT_ED;
    }

    Isoch = ((Event->Dword[0] & XHCI_EVENT_DATA_TYPE_MASK) == USB_ENDPOINT_TYPE_ISOCHRONOUS);
    Direct = (Event->Dword[3] & XHCI_TRANSFER_EVENT_ED) &&
             !Isoch &&
             !Controller->HasErrata(XhciErrata::EvtCheckTrbAddress);

    /* The transfer ring takes it straight away unless it has to go through the endpoint */
    if (Direct && XhciTransferRing::OnUntargetedTransferEvent(Controller, Event))
        return;

    if (Controller->m_Slots.LookupSlot(XhciTrbSlotId(Event)) == NULL)
    {
        if (!Isoch)
        {
            DPRINT1("Transfer event for slot %lu endpoint %lu has no device, dropped\n",
                    XhciTrbSlotId(Event),
                    (Event->Dword[3] & XHCI_TRB_ENDPOINT_MASK) >> XHCI_TRB_ENDPOINT_SHIFT);
        }
        return;
    }

    Controller->m_Slots.OnTransferEvent(Event);
}

static
VOID
NTAPI
XhciDispatchEvent(
    _In_ PXHCI_INTERRUPTER Entry,
    _Inout_ PXHCI_TRB Event)
{
    XhciController* Controller = Entry->Controller;
    ULONG Type = XhciTrbType(Event);
    ULONG Code = XhciTrbCompletionCode(Event);

    if (Code == static_cast<ULONG>(XhciCompletionCode::VendorQuirk199) &&
        Controller->HasErrata(XhciErrata::EvtDropVendorCode199))
    {
        DPRINT1("Dropping event type %lu with vendor completion code 199\n", Type);
        return;
    }

    if (Type == static_cast<ULONG>(XhciTrbType::TransferEvent))
    {
        XhciRouteTransferEvent(Entry, Event);
        return;
    }

    if (Type == static_cast<ULONG>(XhciTrbType::HostControllerEvent) &&
        Code == static_cast<ULONG>(XhciCompletionCode::EventRingFull))
    {
        Entry->RingFullCount++;
        DPRINT1("Interrupter %lu event ring full (%lu times)\n", Entry->Index, Entry->RingFullCount);

        if (Controller->HasErrata(XhciErrata::EvtResetOnRingOverflow))
            Controller->RaiseControllerFault(XhciRecovery::ResetHost, XHCI_REASON_EVENT_RING_FULL);
        return;
    }

    /* Only transfer events are targeted at secondary interrupters */
    if (Entry->Index != 0)
    {
        DPRINT1("Event type %lu on secondary interrupter %lu, dropped\n", Type, Entry->Index);
        return;
    }

    switch (Type)
    {
        case static_cast<ULONG>(XhciTrbType::CommandCompletionEvent):
        case static_cast<ULONG>(XhciTrbType::VendorCommandCompletionEvent):
            Controller->m_Commands.OnCompletionEvent(Event);
            break;

        case static_cast<ULONG>(XhciTrbType::PortStatusChangeEvent):
            Controller->m_RootHub.OnPortStatusChangeEvent(Event);
            break;

        case static_cast<ULONG>(XhciTrbType::DeviceNotificationEvent):
            if (Controller->m_Slots.LookupSlot(XhciTrbSlotId(Event)) != NULL)
                Controller->m_Slots.OnDeviceNotificationEvent(Event);
            break;

        case static_cast<ULONG>(XhciTrbType::HostControllerEvent):
            DPRINT1("Host controller event with completion code %lu\n", Code);
            break;

        case static_cast<ULONG>(XhciTrbType::MfindexWrapEvent):
            /* USBCMD.EWE is never set, so the controller should not send these */
            DPRINT1("Unexpected MFINDEX wrap event\n");
            break;

        default:
            DPRINT1("Unknown event type %lu (code %lu), dropped\n", Type, Code);
            break;
    }
}

/** Hands the rest of the ring to a later DPC. EHB stays set, so no interrupt arrives meanwhile. */
static
VOID
NTAPI
XhciRequeueEvents(
    _Inout_ PXHCI_INTERRUPTER Entry)
{
    KeAcquireSpinLockAtDpcLevel(&Entry->Lock);

    /* D0 exit is draining: leave the rest for the next D0 entry */
    if (Entry->PendingDisable)
    {
        KeReleaseSpinLockFromDpcLevel(&Entry->Lock);
        DPRINT("Interrupter %lu requeue skipped, disable pending\n", Entry->Index);
        return;
    }

    KeClearEvent(&Entry->RequeueIdle);
    KeReleaseSpinLockFromDpcLevel(&Entry->Lock);

    /* Requeue is NULL if creation failed or it was already torn down */
    if (Entry->Requeue != NULL)
    {
        WdfWorkItemEnqueue(Entry->Requeue);
        return;
    }

    KeAcquireSpinLockAtDpcLevel(&Entry->Lock);
    Entry->DpcRunning = FALSE;
    KeSetEvent(&Entry->RequeueIdle, IO_NO_INCREMENT, FALSE);
    KeReleaseSpinLockFromDpcLevel(&Entry->Lock);

    WdfInterruptQueueDpcForIsr(Entry->Interrupt);
}

static
VOID
NTAPI
XhciEvtRequeueWorkItem(
    _In_ WDFWORKITEM WorkItem)
{
    WDFINTERRUPT Interrupt;
    PXHCI_INTERRUPTER Entry;
    KIRQL OldIrql;

    Interrupt = (WDFINTERRUPT)WdfWorkItemGetParentObject(WorkItem);
    Entry = XhciInterruptContext(Interrupt)->Entry;

    KeAcquireSpinLock(&Entry->Lock, &OldIrql);
    Entry->DpcRunning = FALSE;
    KeSetEvent(&Entry->RequeueIdle, IO_NO_INCREMENT, FALSE);
    KeReleaseSpinLock(&Entry->Lock, OldIrql);

    WdfInterruptQueueDpcForIsr(Interrupt);
}

static
VOID
NTAPI
XhciEvtInterruptDpc(
    _In_ WDFINTERRUPT Interrupt,
    _In_ WDFOBJECT AssociatedObject)
{
    PXHCI_INTERRUPTER Entry;
    PXHCI_TRB Trb;
    XHCI_TRB Event;
    ULONG Processed;
    BOOLEAN LimitsKnown = FALSE;
    ULONG TimeLimit = 0;
    ULONG WatchdogLimit = 0;

    UNREFERENCED_PARAMETER(AssociatedObject);

    Entry = XhciInterruptContext(Interrupt)->Entry;
    if (Entry == NULL)
        return;

    /* A second DPC or a pending requeue already owns the ring */
    KeAcquireSpinLockAtDpcLevel(&Entry->Lock);
    if (Entry->DpcRunning)
    {
        KeReleaseSpinLockFromDpcLevel(&Entry->Lock);
        return;
    }
    Entry->DpcRunning = TRUE;
    KeReleaseSpinLockFromDpcLevel(&Entry->Lock);

    Trb = XhciDequeueTrb(Entry);
    for (Processed = 0; XhciEventPending(Entry, Trb); Processed++)
    {
        if ((Processed % XHCI_BUDGET_CHECK_EVENTS) == 0 &&
            XhciDpcBudgetSpent(&LimitsKnown, &TimeLimit, &WatchdogLimit))
        {
            XhciWriteDequeue(Entry, FALSE);
            XhciRequeueEvents(Entry);
            return;
        }

        /* Copy only after the cycle bit said the TRB is complete */
        RtlCopyMemory(&Event, Trb, sizeof(Event));
        XhciAdvanceDequeue(Entry);
        XhciDispatchEvent(Entry, &Event);

        Trb = XhciDequeueTrb(Entry);

        /* Give consumed TRBs back early on long bursts; EHB stays set */
        if (Processed != 0 &&
            (Processed % XHCI_HANDBACK_EVENTS) == 0 &&
            XhciEventPending(Entry, Trb))
        {
            XhciWriteDequeue(Entry, FALSE);
        }
    }

    /* Written even when nothing was consumed, so a spurious interrupt still clears EHB */
    KeAcquireSpinLockAtDpcLevel(&Entry->Lock);
    Entry->DpcRunning = FALSE;
    XhciWriteDequeue(Entry, TRUE);
    KeReleaseSpinLockFromDpcLevel(&Entry->Lock);
}

/* Resource requirements ******************************************************/

static
BOOLEAN
NTAPI
XhciIsMessageRequirement(
    _In_ PIO_RESOURCE_DESCRIPTOR Descriptor)
{
    const USHORT MessageFlags = CM_RESOURCE_INTERRUPT_LATCHED | CM_RESOURCE_INTERRUPT_MESSAGE;

    return Descriptor->Type == CmResourceTypeInterrupt &&
           (Descriptor->Flags & MessageFlags) == MessageFlags;
}

static
BOOLEAN
NTAPI
XhciIsMessageResource(
    _In_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor)
{
    const USHORT MessageFlags = CM_RESOURCE_INTERRUPT_LATCHED | CM_RESOURCE_INTERRUPT_MESSAGE;

    return Descriptor->Type == CmResourceTypeInterrupt &&
           (Descriptor->Flags & MessageFlags) == MessageFlags;
}

/** Drops every MSI descriptor and keeps only lists that still offer a line interrupt. */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
XhciStripMessageRequirements(
    _In_ WDFIORESREQLIST Requirements)
{
    WDFIORESLIST List;
    PIO_RESOURCE_DESCRIPTOR Descriptor;
    IO_RESOURCE_DESCRIPTOR Preferred;
    ULONG ListIndex;
    ULONG Index;
    ULONG LineCount;
    BOOLEAN AnyLine = FALSE;

    ListIndex = WdfIoResourceRequirementsListGetCount(Requirements);
    while (ListIndex > 0)
    {
        ListIndex -= 1;
        List = WdfIoResourceRequirementsListGetIoResList(Requirements, ListIndex);
        LineCount = 0;

        Index = WdfIoResourceListGetCount(List);
        while (Index > 0)
        {
            Index -= 1;
            Descriptor = WdfIoResourceListGetDescriptor(List, Index);
            if (Descriptor->Type != CmResourceTypeInterrupt)
                continue;

            if (XhciIsMessageRequirement(Descriptor))
            {
                WdfIoResourceListRemove(List, Index);
                continue;
            }

            /* The line interrupt was offered as a fallback; make it the preferred choice */
            Preferred = *Descriptor;
            Preferred.Option = 0;
            WdfIoResourceListUpdateDescriptor(List, &Preferred, Index);
            LineCount++;
        }

        if (LineCount == 0)
        {
            DPRINT("Removing requirements list %lu, it has no line interrupt\n", ListIndex);
            WdfIoResourceRequirementsListRemove(Requirements, ListIndex);
        }
        else
        {
            AnyLine = TRUE;
        }
    }

    if (!AnyLine)
    {
        DPRINT1("Line interrupts are required but no requirements list offers one\n");
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

/** Picks the list with the best MSI offer: MSI-X first, then the most messages. */
_IRQL_requires_(PASSIVE_LEVEL)
static
BOOLEAN
NTAPI
XhciFindMessageList(
    _In_ WDFIORESREQLIST Requirements,
    _Out_ PULONG ListIndex,
    _Out_ PULONG Messages)
{
    WDFIORESLIST List;
    PIO_RESOURCE_DESCRIPTOR Descriptor;
    ULONG Lists;
    ULONG Current;
    ULONG Index;
    ULONG Count;
    ULONG Descriptors;
    BOOLEAN MsiX;
    BOOLEAN BestMsiX = FALSE;

    *ListIndex = 0;
    *Messages = 0;

    Lists = WdfIoResourceRequirementsListGetCount(Requirements);
    for (Current = 0; Current < Lists; Current++)
    {
        List = WdfIoResourceRequirementsListGetIoResList(Requirements, Current);
        Count = 0;
        Descriptors = 0;

        for (Index = 0; Index < WdfIoResourceListGetCount(List); Index++)
        {
            Descriptor = WdfIoResourceListGetDescriptor(List, Index);
            if (!XhciIsMessageRequirement(Descriptor) || (Descriptor->Option & IO_RESOURCE_ALTERNATIVE))
                continue;

            Descriptors++;
            Count += Descriptor->u.Interrupt.MaximumVector - Descriptor->u.Interrupt.MinimumVector + 1;
        }

        /* MSI-X lists carry one descriptor per message, MSI a single ranged one */
        MsiX = (Descriptors > 1);

        if (MsiX && (!BestMsiX || Count > *Messages))
        {
            BestMsiX = TRUE;
            *ListIndex = Current;
            *Messages = Count;
        }
        else if (!MsiX && !BestMsiX && Count > *Messages)
        {
            *ListIndex = Current;
            *Messages = Count;
        }
    }

    return BestMsiX;
}

/** Keeps message descriptors covering Wanted messages and removes the others. */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
XhciTrimMessageRequirements(
    _In_ WDFIORESLIST List,
    _In_ ULONG Wanted)
{
    PIO_RESOURCE_DESCRIPTOR Descriptor;
    IO_RESOURCE_DESCRIPTOR Trimmed;
    ULONG Index = 0;
    ULONG Offered;
    ULONG Claim;

    while (Index < WdfIoResourceListGetCount(List))
    {
        Descriptor = WdfIoResourceListGetDescriptor(List, Index);
        if (!XhciIsMessageRequirement(Descriptor))
        {
            Index++;
            continue;
        }

        if (Wanted == 0)
        {
            WdfIoResourceListRemove(List, Index);
            continue;
        }

        Offered = Descriptor->u.Interrupt.MaximumVector - Descriptor->u.Interrupt.MinimumVector + 1;
        if (Offered <= Wanted)
        {
            Wanted -= Offered;
            Index++;
            continue;
        }

        /* Multi message MSI grants powers of two only */
        Claim = 1;
        while (Claim < Wanted)
            Claim <<= 1;

        Trimmed = *Descriptor;
        Trimmed.u.Interrupt.MaximumVector = CM_RESOURCE_INTERRUPT_MESSAGE_TOKEN;
        Trimmed.u.Interrupt.MinimumVector = CM_RESOURCE_INTERRUPT_MESSAGE_TOKEN - Claim + 1;
        WdfIoResourceListUpdateDescriptor(List, &Trimmed, Index);

        DPRINT("Message descriptor %lu trimmed to %lu messages\n", Index, Claim);
        Wanted = 0;
        Index++;
    }
}

/* XhciInterrupters ***********************************************************/

NTSTATUS
XhciInterrupters::Create(
    _In_ XhciController* Controller)
{
    NTSTATUS Status;

    PAGED_CODE();

    m_Controller = Controller;
    m_PrimaryInterrupt = NULL;
    m_Mechanism = Mechanism::Line;
    m_SecondaryCount = 0;
    m_Count = 0;
    m_Capacity = 0;
    m_Table = NULL;
    m_Lookup = NULL;
    m_LookupSize = 0;

    XhciActiveCountRoutine =
        (PFN_XHCI_ACTIVE_COUNT)XhciFindKernelRoutine(L"KeQueryActiveProcessorCountEx");
    XhciIndexFromNumberRoutine =
        (PFN_XHCI_INDEX_FROM_NUMBER)XhciFindKernelRoutine(L"KeGetProcessorIndexFromNumber");
    XhciCurrentNumberRoutine =
        (PFN_XHCI_CURRENT_NUMBER)XhciFindKernelRoutine(L"KeGetCurrentProcessorNumberEx");
    XhciDpcWatchdogRoutine =
        (PFN_XHCI_DPC_WATCHDOG)XhciFindKernelRoutine(L"KeQueryDpcWatchdogInformation");

    /* The primary takes the first interrupt resource KMDF finds at start */
    Status = CreateInterrupt(NULL, NULL, &m_PrimaryInterrupt);
    if (!NT_SUCCESS(Status))
        DPRINT1("Primary interrupt creation failed 0x%lx\n", Status);

    return Status;
}

NTSTATUS
XhciInterrupters::CreateInterrupt(
    _In_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Raw,
    _In_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Translated,
    _Out_ WDFINTERRUPT* Interrupt)
{
    WDF_INTERRUPT_CONFIG Config;
    WDF_OBJECT_ATTRIBUTES Attributes;
    NTSTATUS Status;

    WDF_INTERRUPT_CONFIG_INIT(&Config, XhciEvtInterruptIsr, XhciEvtInterruptDpc);
    Config.EvtInterruptEnable = XhciEvtInterruptEnable;
    Config.EvtInterruptDisable = XhciEvtInterruptDisable;
    Config.InterruptRaw = Raw;
    Config.InterruptTranslated = Translated;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, XHCI_INTERRUPT_CONTEXT);

    Status = WdfInterruptCreate(m_Controller->m_Device, &Config, &Attributes, Interrupt);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("WdfInterruptCreate failed 0x%lx\n", Status);
        *Interrupt = NULL;
    }

    return Status;
}

ULONG
XhciInterrupters::SecondaryCountFor(
    _In_ ULONG Messages) const
{
    ULONG Limit;
    ULONG Processors;
    ULONG Count;

    if (m_Controller->HasErrata(XhciErrata::IntPrimaryOnly) || Messages <= 1)
        return 0;

    /* One message always goes to the primary */
    Limit = min(Messages - 1, (ULONG)XHCI_SECONDARY_MAX);

    /* A single processor still gets its own secondary */
    Processors = XhciActiveProcessors(0);
    if (Processors <= Limit)
        return Processors;

    /* Halve until the processors split evenly over what is available */
    Count = Processors / 2;
    while (Count > Limit)
        Count /= 2;

    return Count;
}

NTSTATUS
XhciInterrupters::FilterResourceRequirements(
    _In_ WDFIORESREQLIST Requirements)
{
    ULONG ListIndex;
    ULONG Messages;
    BOOLEAN MsiX;

    PAGED_CODE();

    if (m_Controller->HasErrata(XhciErrata::IntLegacyLineOnly))
    {
        m_SecondaryCount = 0;
        return XhciStripMessageRequirements(Requirements);
    }

    MsiX = XhciFindMessageList(Requirements, &ListIndex, &Messages);
    m_SecondaryCount = MsiX ? SecondaryCountFor(Messages) : 0;

    DPRINT("Requirements list %lu offers %lu messages (MSI-X %u), %lu secondary interrupters\n",
           ListIndex, Messages, MsiX, m_SecondaryCount);

    if (WdfIoResourceRequirementsListGetCount(Requirements) != 0)
    {
        XhciTrimMessageRequirements(WdfIoResourceRequirementsListGetIoResList(Requirements, ListIndex),
                                    1 + m_SecondaryCount);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
XhciInterrupters::AffinitizeResourceRequirements(
    _In_ WDFIORESREQLIST Requirements)
{
    WDFIORESLIST List;
    PIO_RESOURCE_DESCRIPTOR Descriptor;
    IO_RESOURCE_DESCRIPTOR Pinned;
    ULONG ListIndex;
    ULONG Messages;
    ULONG PerInterrupter;
    ULONG NextProcessor = 0;
    ULONG Index;
    ULONG Step;
    BOOLEAN Primary = TRUE;

    PAGED_CODE();

    if (m_SecondaryCount == 0)
        return STATUS_SUCCESS;

    if (!XhciFindMessageList(Requirements, &ListIndex, &Messages))
    {
        DPRINT1("Secondary interrupters planned but no MSI-X list found\n");
        return STATUS_SUCCESS;
    }

    PerInterrupter = XhciActiveProcessors(0) / m_SecondaryCount;
    List = WdfIoResourceRequirementsListGetIoResList(Requirements, ListIndex);

    for (Index = 0; Index < WdfIoResourceListGetCount(List); Index++)
    {
        Descriptor = WdfIoResourceListGetDescriptor(List, Index);
        if (!XhciIsMessageRequirement(Descriptor))
            continue;

        Pinned = *Descriptor;
        Pinned.Flags |= CM_RESOURCE_INTERRUPT_POLICY_INCLUDED;
        Pinned.u.Interrupt.AffinityPolicy = IrqPolicySpecifiedProcessors;
        Pinned.u.Interrupt.TargetedProcessors = 0;

        /* The primary gets an empty processor set */
        if (!Primary)
        {
            for (Step = 0; Step < PerInterrupter && NextProcessor < sizeof(KAFFINITY) * 8; Step++)
            {
                Pinned.u.Interrupt.TargetedProcessors |= (KAFFINITY)1 << NextProcessor;
                NextProcessor++;
            }
        }

        Primary = FALSE;
        WdfIoResourceListUpdateDescriptor(List, &Pinned, Index);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
XhciInterrupters::PrepareOne(
    _Inout_ Interrupter* Entry,
    _In_ WDFINTERRUPT Interrupt,
    _In_ ULONG Index,
    _In_ BOOLEAN MessageSignaled)
{
    XhciController* Controller = m_Controller;
    WDF_WORKITEM_CONFIG WorkConfig;
    WDF_OBJECT_ATTRIBUTES Attributes;
    PXHCI_ERST_ENTRY Table;
    PLIST_ENTRY Link;
    XhciDmaBuffer* Buffer;
    ULONG Segments;
    ULONG Hardware;
    ULONG Slot;
    NTSTATUS Status;

    Entry->Controller = Controller;
    Entry->Interrupt = Interrupt;
    Entry->Index = Index;
    Entry->RegisterOffset = XHCI_RUNTIME_INTERRUPTER_BASE + Index * XHCI_INTERRUPTER_STRIDE;
    Entry->MessageSignaled = MessageSignaled;
    InitializeListHead(&Entry->SegmentList);
    KeInitializeSpinLock(&Entry->Lock);
    KeInitializeEvent(&Entry->RequeueIdle, NotificationEvent, TRUE);

    /* Segment count: software default capped by ERST Max, then by the errata */
    Segments = Controller->HasErrata(XhciErrata::EvtResetOnRingOverflow) ?
               XHCI_SEGMENTS_RING_FULL : XHCI_SEGMENTS_DEFAULT;
    Hardware = max(Controller->m_Registers.EventRingSegmentTableMax(), 1UL);
    Segments = min(Segments, Hardware);
    if (Segments > XHCI_SEGMENTS_LIMITED && Controller->HasErrata(XhciErrata::EvtTwoSegmentRingCap))
        Segments = XHCI_SEGMENTS_LIMITED;

    Entry->SegmentCount = Segments;
    Entry->SegmentSize = PAGE_SIZE;
    Entry->TrbsPerSegment = PAGE_SIZE / sizeof(XHCI_TRB);

    Entry->Erst = Controller->m_Buffers.Acquire(Segments * sizeof(XHCI_ERST_ENTRY));
    if (Entry->Erst == NULL)
    {
        DPRINT1("Interrupter %lu: no buffer for the segment table\n", Index);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = Controller->m_Buffers.AcquireList(Entry->SegmentSize, Segments, &Entry->SegmentList);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Interrupter %lu: no buffers for %lu event ring segments 0x%lx\n",
                Index, Segments, Status);
        goto Fail;
    }

    XhciWriteInterrupter(Entry, XHCI_ERSTSZ, 0);

    /* The table is written once here and survives power transitions */
    Table = (PXHCI_ERST_ENTRY)Entry->Erst->VirtualAddress;
    Slot = 0;
    for (Link = Entry->SegmentList.Flink; Link != &Entry->SegmentList; Link = Link->Flink)
    {
        Buffer = CONTAINING_RECORD(Link, XhciDmaBuffer, Link);
        Table[Slot].SegmentAddress = (ULONG64)Buffer->LogicalAddress.QuadPart;
        Table[Slot].SegmentSize = Entry->TrbsPerSegment;
        Table[Slot].Reserved = 0;
        Slot++;
    }

    WDF_WORKITEM_CONFIG_INIT(&WorkConfig, XhciEvtRequeueWorkItem);
    WorkConfig.AutomaticSerialization = FALSE;
    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Interrupt;

    Status = WdfWorkItemCreate(&WorkConfig, &Attributes, &Entry->Requeue);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Interrupter %lu: requeue work item creation failed 0x%lx\n", Index, Status);
        Entry->Requeue = NULL;
        goto Fail;
    }

    Entry->Segment = CONTAINING_RECORD(Entry->SegmentList.Flink, XhciDmaBuffer, Link);
    Entry->CycleState = XHCI_TRB_CYCLE;
    Entry->Prepared = TRUE;
    XhciInterruptContext(Interrupt)->Entry = Entry;
    return STATUS_SUCCESS;

Fail:
    if (!IsListEmpty(&Entry->SegmentList))
        Controller->m_Buffers.FreeList(&Entry->SegmentList);
    Controller->m_Buffers.Free(Entry->Erst);
    Entry->Erst = NULL;
    return Status;
}

NTSTATUS
XhciInterrupters::Prepare(
    _In_ WDFCMRESLIST Raw,
    _In_ WDFCMRESLIST Translated)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    WDFINTERRUPT Interrupt;
    ULONG Assigned = 0;
    ULONG Index;
    ULONG Message;
    ULONG Limit;
    BOOLEAN FirstSkipped;
    NTSTATUS Status;

    PAGED_CODE();

    m_Mechanism = Mechanism::Line;
    for (Index = 0; Index < WdfCmResourceListGetCount(Raw); Index++)
    {
        Descriptor = WdfCmResourceListGetDescriptor(Raw, Index);
        if (Descriptor->Type != CmResourceTypeInterrupt)
            continue;

        if (XhciIsMessageResource(Descriptor))
        {
            m_Mechanism = (Assigned != 0) ? Mechanism::MsiX : Mechanism::Msi;
            Assigned += Descriptor->u.MessageInterrupt.Raw.MessageCount;
        }
        else
        {
            Assigned++;
        }
    }

    if (Assigned == 0)
    {
        DPRINT1("No interrupt resources were assigned\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    m_Table = (Interrupter*)ExAllocatePoolWithTag(NonPagedPool,
                                                  Assigned * sizeof(*m_Table),
                                                  XHCI_TAG_INTERRUPTER);
    if (m_Table == NULL)
    {
        DPRINT1("Interrupter table allocation for %lu entries failed\n", Assigned);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(m_Table, Assigned * sizeof(*m_Table));
    m_Capacity = Assigned;

    Status = PrepareOne(&m_Table[0], m_PrimaryInterrupt, 0, m_Mechanism != Mechanism::Line);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Primary interrupter prepare failed 0x%lx\n", Status);
        return Status;
    }

    m_Count = 1;

    /* Register parsing may have asked for a single interrupter after filtering */
    if (m_Controller->HasErrata(XhciErrata::IntPrimaryOnly))
        m_SecondaryCount = 0;

    /* The MSI-X table alone may allow more than MaxIntrs */
    Limit = m_Controller->m_Registers.MaxInterrupters();
    if (Limit != 0 && m_SecondaryCount > Limit - 1)
        m_SecondaryCount = Limit - 1;

    if (m_SecondaryCount != 0 && Assigned > 1 && m_Mechanism == Mechanism::MsiX)
    {
        FirstSkipped = FALSE;
        Message = 0;

        for (Index = 0; Index < WdfCmResourceListGetCount(Translated); Index++)
        {
            Descriptor = WdfCmResourceListGetDescriptor(Translated, Index);
            if (Descriptor->Type != CmResourceTypeInterrupt ||
                (Descriptor->Flags & CM_RESOURCE_INTERRUPT_MESSAGE) == 0)
            {
                continue;
            }

            /* The first message belongs to the primary */
            if (!FirstSkipped)
            {
                FirstSkipped = TRUE;
                continue;
            }

            Message++;
            if (Message > m_SecondaryCount || Message >= m_Capacity)
                break;

            Status = CreateInterrupt(WdfCmResourceListGetDescriptor(Raw, Index), Descriptor, &Interrupt);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Secondary interrupter %lu creation failed 0x%lx\n", Message, Status);
                return Status;
            }

            Status = PrepareOne(&m_Table[Message], Interrupt, Message, TRUE);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Secondary interrupter %lu prepare failed 0x%lx\n", Message, Status);
                return Status;
            }

#if defined(NT_PROCESSOR_GROUPS)
            m_Table[Message].Group = Descriptor->u.MessageInterrupt.Translated.Group;
#endif
            m_Table[Message].Affinity = Descriptor->u.MessageInterrupt.Translated.Affinity;

            /* Count only the interrupters actually set up */
            m_Count = Message + 1;
        }
    }

    DPRINT("%lu interrupt messages assigned, mechanism %lu, %lu interrupters in use\n",
           Assigned, static_cast<ULONG>(m_Mechanism), m_Count);

    return BuildLookup();
}

NTSTATUS
XhciInterrupters::BuildLookup()
{
    Interrupter* Entry;
    ULONG Size;
    ULONG Number;
    ULONG Target;
    ULONG Index;

    Size = XhciActiveProcessors(ALL_PROCESSOR_GROUPS);
    m_Lookup = (PUSHORT)ExAllocatePoolWithTag(NonPagedPool,
                                              max(Size, 1UL) * sizeof(*m_Lookup),
                                              XHCI_TAG_INTERRUPTER);
    if (m_Lookup == NULL)
    {
        DPRINT1("Processor lookup table allocation for %lu processors failed\n", Size);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(m_Lookup, max(Size, 1UL) * sizeof(*m_Lookup));
    m_LookupSize = Size;

    /* Zero entries mean the primary; group 0 masks only */
    for (Target = 1; Target < m_Count; Target++)
    {
        Entry = &m_Table[Target];
        for (Number = 0; Number < sizeof(KAFFINITY) * 8; Number++)
        {
            if ((Entry->Affinity & ((KAFFINITY)1 << Number)) == 0)
                continue;

            Index = XhciProcessorIndex(Entry->Group, Number);
            if (Index < m_LookupSize)
                m_Lookup[Index] = (USHORT)Target;
        }
    }

    return STATUS_SUCCESS;
}

VOID
XhciInterrupters::ReleaseOne(
    _Inout_ Interrupter* Entry)
{
    XhciController* Controller = m_Controller;

    if (Controller->IsAccessible())
    {
        XhciWriteInterrupter(Entry, XHCI_ERSTSZ, 0);
        XhciWriteInterrupter64(Entry, XHCI_ERSTBA, 0);
        XhciWriteInterrupter64(Entry, XHCI_ERDP, 0);
    }

    Controller->m_Buffers.FreeList(&Entry->SegmentList);
    Controller->m_Buffers.Free(Entry->Erst);
    Entry->Erst = NULL;
    Entry->Segment = NULL;

    if (Entry->Requeue != NULL)
    {
        WdfWorkItemFlush(Entry->Requeue);
        WdfObjectDelete(Entry->Requeue);
        Entry->Requeue = NULL;
    }

    XhciInterruptContext(Entry->Interrupt)->Entry = NULL;
    Entry->Prepared = FALSE;
}

VOID
XhciInterrupters::Release()
{
    ULONG Index;

    PAGED_CODE();

    if (m_Table != NULL)
    {
        for (Index = 0; Index < m_Capacity; Index++)
        {
            if (m_Table[Index].Prepared)
                ReleaseOne(&m_Table[Index]);
        }

        ExFreePoolWithTag(m_Table, XHCI_TAG_INTERRUPTER);
        m_Table = NULL;
    }

    if (m_Lookup != NULL)
    {
        ExFreePoolWithTag(m_Lookup, XHCI_TAG_INTERRUPTER);
        m_Lookup = NULL;
    }

    m_Count = 0;
    m_Capacity = 0;
    m_LookupSize = 0;
}

NTSTATUS
XhciInterrupters::D0Entry(
    _In_ BOOLEAN StatePreserved)
{
    ULONG Index;

    if (!m_Controller->IsAccessible())
        return STATUS_SUCCESS;

    /* On a restore the ring contents and positions from before D0 exit stay valid */
    for (Index = 0; Index < m_Count; Index++)
    {
        if (!StatePreserved)
            XhciResetEventRing(&m_Table[Index]);

        XhciProgramEventRing(&m_Table[Index]);
    }

    return STATUS_SUCCESS;
}

VOID
XhciInterrupters::D0ExitPreInterruptsDisabled()
{
    Interrupter* Entry;
    LARGE_INTEGER Timeout;
    KIRQL OldIrql;
    ULONG Index;

    PAGED_CODE();

    for (Index = 0; Index < m_Count; Index++)
    {
        Entry = &m_Table[Index];
        if (Entry->Requeue == NULL)
            continue;

        KeAcquireSpinLock(&Entry->Lock, &OldIrql);
        Entry->PendingDisable = TRUE;
        KeReleaseSpinLock(&Entry->Lock, OldIrql);

        /* Freeing state under a running work item is worse than waiting */
        Timeout = XhciRelativeMs(XHCI_DRAIN_WAIT_MS);
        while (KeWaitForSingleObject(&Entry->RequeueIdle,
                                     Executive,
                                     KernelMode,
                                     FALSE,
                                     &Timeout) == STATUS_TIMEOUT)
        {
            DPRINT1("Interrupter %lu requeue work item still running after 60 s\n", Index);
        }
    }
}

VOID
XhciInterrupters::D0Exit()
{
    DPRINT("Interrupters stopped, %lu in use\n", m_Count);
}

VOID
XhciInterrupters::PostReset()
{
    ULONG Index;

    if (!m_Controller->IsAccessible())
        return;

    for (Index = 0; Index < m_Count; Index++)
    {
        XhciResetEventRing(&m_Table[Index]);
        XhciProgramEventRing(&m_Table[Index]);
    }

    for (Index = 0; Index < m_Count; Index++)
    {
        if (m_Table[Index].Enabled)
            XhciUnmaskInterrupter(&m_Table[Index]);
    }
}

ULONG
XhciInterrupters::Count() const
{
    return m_Count;
}

ULONG
XhciInterrupters::TargetForCurrentProcessor() const
{
    ULONG Index;

    if (m_Lookup == NULL)
        return 0;

    Index = XhciCurrentProcessorIndex();

    /* Processors added after start are not in the table and use the primary */
    return (Index < m_LookupSize) ? m_Lookup[Index] : 0;
}
