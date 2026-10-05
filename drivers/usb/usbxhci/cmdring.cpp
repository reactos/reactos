/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Command ring: submission, completion, timeout and abort
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "xhcidrv.h"

#define NDEBUG
#include <debug.h>

/* One segment of 32 TRBs; the last one is the Link TRB back to the start */
static const ULONG XhciCmdTrbCount = 32;
static const ULONG XhciCmdUsable = XhciCmdTrbCount - 1;

/* Timer ticks (1 s each) a command gets; it is aborted when the count hits these values */
static const LONG XhciCmdTicksInitial = 10;
static const LONG XhciCmdTicksFirstAbort = 5;
static const LONG XhciCmdTicksAbortWait = 5;
static const ULONG XhciCmdTickMs = 1000;
static const ULONG XhciCmdTickSlackMs = 500;

/* Reason codes handed to the controller with a fatal error */
static const ULONG XhciCmdReasonAbortFailed = 0x1009;
static const ULONG XhciCmdReasonOutOfOrder = 0x1012;
static const ULONG XhciCmdReasonDequeueMismatch = 0x1013;

/* Address Device TRB Block Set Address Request (xHCI 6.4.3.4) */
static const ULONG XhciCmdBlockSetAddress = 0x00000200;

typedef struct _XHCI_COMMAND_TIMER_CONTEXT
{
    XhciCommandRing* Ring;
} XHCI_COMMAND_TIMER_CONTEXT, *PXHCI_COMMAND_TIMER_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(XHCI_COMMAND_TIMER_CONTEXT, XhciGetCommandTimerContext);

static
NTSTATUS
NTAPI
XhciCmdStatusFromCode(
    _In_ XhciCompletionCode Code)
{
    if (Code == XhciCompletionCode::Success)
        return STATUS_SUCCESS;

    if (Code == XhciCompletionCode::CommandAborted)
        return STATUS_IO_TIMEOUT;

    return STATUS_UNSUCCESSFUL;
}

static
XhciCommand*
NTAPI
XhciCmdFromLink(
    _In_ PLIST_ENTRY Entry)
{
    return CONTAINING_RECORD(Entry, XhciCommand, Link);
}

/** Calls Done for every command on the list, in order. No ring lock may be held. */
static
VOID
NTAPI
XhciCmdCompleteList(
    _Inout_ PLIST_ENTRY Finished)
{
    XhciCommand* Command;

    while (!IsListEmpty(Finished))
    {
        Command = XhciCmdFromLink(RemoveHeadList(Finished));
        XhciClearListEntry(&Command->Link);
        Command->Done(Command);
    }
}

static
VOID
NTAPI
XhciCmdWaiterDone(
    _In_ XhciCommand* Command)
{
    PKEVENT Event = Command->WaiterEvent;

    if (Command->WaiterDone)
        Command->WaiterDone(Command);

    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);
}

NTSTATUS
XhciCommandRing::Create(
    _In_ XhciController* Controller)
{
    WDF_TIMER_CONFIG Config;
    WDF_OBJECT_ATTRIBUTES Attributes;
    NTSTATUS Status;

    PAGED_CODE();

    m_Controller = Controller;
    m_Timer = NULL;
    KeInitializeSpinLock(&m_Lock);
    m_State = RingState::Uninitialized;
    m_Reset = ResetPhase::Idle;
    m_Segment = NULL;
    m_Trbs = NULL;
    m_RingAddress = 0;
    m_LinkAddress = 0;
    m_Enqueue = 0;
    m_Dequeue = 0;
    m_Cycle = XHCI_TRB_CYCLE;
    InitializeListHead(&m_Pending);
    InitializeListHead(&m_Waiting);
    m_OutOfOrderSeen = FALSE;
    m_Aborting = FALSE;
    m_AbortTicks = 0;
    m_DrainFromTimer = FALSE;

    WDF_TIMER_CONFIG_INIT_PERIODIC(&Config, XhciCommandRing::EvtTimer, XhciCmdTickMs);
    Config.TolerableDelay = XhciCmdTickSlackMs;
    Config.AutomaticSerialization = FALSE;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, XHCI_COMMAND_TIMER_CONTEXT);
    Attributes.ParentObject = Controller->m_Device;
    Attributes.ExecutionLevel = WdfExecutionLevelDispatch;

    Status = WdfTimerCreate(&Config, &Attributes, &m_Timer);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Command timer creation failed 0x%lx\n", Status);
        m_Timer = NULL;
        return Status;
    }

    XhciGetCommandTimerContext(m_Timer)->Ring = this;
    return STATUS_SUCCESS;
}

NTSTATUS
XhciCommandRing::Prepare()
{
    PAGED_CODE();

    m_Segment = m_Controller->m_Buffers.Acquire(XhciCmdTrbCount * sizeof(*m_Trbs));
    if (!m_Segment)
    {
        DPRINT1("No common buffer for the command ring, 0x%lx\n", STATUS_INSUFFICIENT_RESOURCES);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    m_Trbs = static_cast<PXHCI_TRB>(m_Segment->VirtualAddress);
    m_RingAddress = m_Segment->LogicalAddress.QuadPart;
    m_LinkAddress = TrbAddress(XhciCmdUsable);
    m_Enqueue = 0;
    m_Dequeue = 0;
    m_Cycle = XHCI_TRB_CYCLE;
    InitializeListHead(&m_Pending);
    InitializeListHead(&m_Waiting);

    /* A restart after a failed controller must accept commands again */
    m_Reset = ResetPhase::Idle;
    m_Aborting = FALSE;
    m_DrainFromTimer = FALSE;

    m_State = RingState::Prepared;
    return STATUS_SUCCESS;
}

VOID
XhciCommandRing::Release()
{
    LIST_ENTRY Finished;
    KIRQL OldIrql;

    PAGED_CODE();

    InitializeListHead(&Finished);

    if (m_State == RingState::Prepared)
    {
        if (m_Timer)
            WdfTimerStop(m_Timer, TRUE);

        if (m_Controller->IsAccessible())
            m_Controller->m_Registers.WriteOperational64(XHCI_OP_CRCR, 0);

        KeAcquireSpinLock(&m_Lock, &OldIrql);
        if (!IsListEmpty(&m_Pending) || !IsListEmpty(&m_Waiting))
        {
            DPRINT1("Command ring released with commands outstanding\n");
            DrainAllLocked(&Finished);
        }
        m_State = RingState::Released;
        KeReleaseSpinLock(&m_Lock, OldIrql);

        /* Complete these commands so no caller waits forever */
        XhciCmdCompleteList(&Finished);

        m_Controller->m_Buffers.Free(m_Segment);
    }

    m_Segment = NULL;
    m_Trbs = NULL;
    m_RingAddress = 0;
    m_LinkAddress = 0;
    m_State = RingState::Released;
}

VOID
XhciCommandRing::InitializeRing()
{
    PXHCI_TRB Link;
    KIRQL OldIrql;

    if (m_State != RingState::Prepared || !m_Controller->IsAccessible())
        return;

    KeAcquireSpinLock(&m_Lock, &OldIrql);

    ASSERT(IsListEmpty(&m_Pending));
    ASSERT(IsListEmpty(&m_Waiting));

    m_Enqueue = 0;
    m_Dequeue = 0;
    m_Cycle = XHCI_TRB_CYCLE;
    RtlZeroMemory(m_Trbs, XhciCmdTrbCount * sizeof(*m_Trbs));

    /* The Link TRB starts with cycle 0, so the xHC does not follow it until we wrap */
    Link = &m_Trbs[XhciCmdUsable];
    Link->Dword[0] = static_cast<ULONG>(m_RingAddress);
    Link->Dword[1] = static_cast<ULONG>(m_RingAddress >> 32);
    Link->Dword[2] = 0;
    Link->Dword[3] = (static_cast<ULONG>(XhciTrbType::Link) << XHCI_TRB_TYPE_SHIFT) |
                     XHCI_LINK_TOGGLE_CYCLE;

    KeMemoryBarrier();
    m_Controller->m_Registers.WriteOperational64(XHCI_OP_CRCR, m_RingAddress | m_Cycle);

    KeReleaseSpinLock(&m_Lock, OldIrql);
}

NTSTATUS
XhciCommandRing::D0Entry()
{
    InitializeRing();
    return STATUS_SUCCESS;
}

VOID
XhciCommandRing::PostInterruptsEntry()
{
    /* The controller module issues the vendor firmware queries through SubmitAndWait */
}

VOID
XhciCommandRing::PutOnRingLocked(
    _Inout_ XhciCommand* Command)
{
    volatile ULONG* Control;
    PXHCI_TRB Slot;
    BOOLEAN WasIdle;

    Slot = &m_Trbs[m_Enqueue];

    Command->Countdown = XhciCmdTicksInitial;
    Command->Matched = FALSE;
    Command->RingIndex = m_Enqueue;

    /* Dword 3 holds the cycle bit, so the xHC may only see it after the rest */
    Slot->Dword[0] = Command->Trb.Dword[0];
    Slot->Dword[1] = Command->Trb.Dword[1];
    Slot->Dword[2] = Command->Trb.Dword[2];
    KeMemoryBarrier();
    Control = &Slot->Dword[3];
    *Control = (Command->Trb.Dword[3] & ~XHCI_TRB_CYCLE) | m_Cycle;

    WasIdle = IsListEmpty(&m_Pending);
    InsertTailList(&m_Pending, &Command->Link);

    m_Enqueue++;
    if (m_Enqueue == XhciCmdUsable)
    {
        /* Hand the Link TRB over with the cycle of the pass that just ended */
        KeMemoryBarrier();
        Control = &m_Trbs[XhciCmdUsable].Dword[3];
        *Control = *Control ^ XHCI_TRB_CYCLE;
        m_Cycle ^= XHCI_TRB_CYCLE;
        m_Enqueue = 0;
    }

    KeMemoryBarrier();
    m_Controller->m_Registers.RingDoorbell(0, 0);

    if (WasIdle)
        WdfTimerStart(m_Timer, WDF_REL_TIMEOUT_IN_MS(XhciCmdTickMs));
}

VOID
XhciCommandRing::PlaceLocked(
    _Inout_ XhciCommand* Command,
    _In_ BOOLEAN FrontOfLine,
    _Inout_ PLIST_ENTRY Finished)
{
    BOOLEAN Park;

    if (m_Reset == ResetPhase::AfterReset || m_Reset == ResetPhase::Failed)
    {
        /* Fail at once; Done runs after the ring lock is dropped */
        Command->Status = STATUS_NO_SUCH_DEVICE;
        InsertTailList(Finished, &Command->Link);
        return;
    }

    Park = FALSE;
    if (m_Reset == ResetPhase::Requested || m_Reset == ResetPhase::Resetting)
    {
        Park = TRUE;
    }
    else if (m_Aborting)
    {
        Park = TRUE;
    }
    else if (!IsListEmpty(&m_Pending) && m_Controller->HasErrata(XhciErrata::CmdOneInFlight))
    {
        Park = TRUE;
    }
    else if ((m_Enqueue + 1) % XhciCmdUsable == m_Dequeue)
    {
        Command->Countdown = XhciCmdTicksInitial;
        Park = TRUE;
    }

    if (!Park)
    {
        PutOnRingLocked(Command);
        return;
    }

    if (FrontOfLine)
        InsertHeadList(&m_Waiting, &Command->Link);
    else
        InsertTailList(&m_Waiting, &Command->Link);
}

VOID
XhciCommandRing::DrainAllLocked(
    _Inout_ PLIST_ENTRY Finished)
{
    XhciCommand* Command;

    /* QUIRK: Code is left as it was */
    while (!IsListEmpty(&m_Pending))
    {
        Command = XhciCmdFromLink(RemoveHeadList(&m_Pending));
        Command->Status = STATUS_NO_SUCH_DEVICE;
        InsertTailList(Finished, &Command->Link);
    }

    while (!IsListEmpty(&m_Waiting))
    {
        Command = XhciCmdFromLink(RemoveHeadList(&m_Waiting));
        Command->Status = STATUS_NO_SUCH_DEVICE;
        InsertTailList(Finished, &Command->Link);
    }
}

VOID
XhciCommandRing::Submit(
    _Inout_ XhciCommand* Command)
{
    LIST_ENTRY Finished;
    KIRQL OldIrql;

    ASSERT(Command->Done != NULL);

    XhciClearListEntry(&Command->Link);

    if (!m_Controller->IsAccessible())
    {
        Command->Code = XhciCompletionCode::Invalid;
        Command->Status = STATUS_NO_SUCH_DEVICE;
        Command->Done(Command);
        return;
    }

    InitializeListHead(&Finished);

    KeAcquireSpinLock(&m_Lock, &OldIrql);

    if (m_State != RingState::Prepared)
    {
        DPRINT1("Command type %lu submitted with no command ring\n", XhciTrbType(&Command->Trb));
        Command->Code = XhciCompletionCode::Invalid;
        Command->Status = STATUS_NO_SUCH_DEVICE;
        InsertTailList(&Finished, &Command->Link);
    }
    else
    {
        PlaceLocked(Command, FALSE, &Finished);
    }

    KeReleaseSpinLock(&m_Lock, OldIrql);

    XhciCmdCompleteList(&Finished);
}

NTSTATUS
XhciCommandRing::SubmitAndWait(
    _Inout_ XhciCommand* Command)
{
    KEVENT Event;

    PAGED_CODE();

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Command->WaiterDone = Command->Done;
    Command->WaiterEvent = &Event;
    Command->Done = XhciCmdWaiterDone;

    Submit(Command);

    /* QUIRK: no timeout; the abort and reset paths always complete the command */
    KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);

    Command->Done = Command->WaiterDone;
    Command->WaiterDone = NULL;
    Command->WaiterEvent = NULL;

    return Command->Status;
}

VOID
XhciCommandRing::OnCompletionEvent(
    _In_ const XHCI_TRB* Event)
{
    LIST_ENTRY Finished;
    PLIST_ENTRY Entry;
    XhciCommand* Command;
    XhciCompletionCode Code;
    ULONG64 Pointer;
    BOOLEAN Retired;
    BOOLEAN OutOfOrder;
    KIRQL OldIrql;

    Code = static_cast<XhciCompletionCode>(XhciTrbCompletionCode(Event));
    if (Code == XhciCompletionCode::CommandRingStopped)
    {
        HandleRingStopped(Event);
        return;
    }

    InitializeListHead(&Finished);
    Pointer = XhciTrbPointer(Event) & ~0xFULL;
    Retired = FALSE;
    OutOfOrder = FALSE;

    KeAcquireSpinLock(&m_Lock, &OldIrql);

    for (Entry = m_Pending.Flink; Entry != &m_Pending; Entry = Entry->Flink)
    {
        Command = XhciCmdFromLink(Entry);
        if (!Command->Matched && TrbAddress(Command->RingIndex) == Pointer)
        {
            Command->Matched = TRUE;
            Command->Code = Code;
            Command->Completion = *Event;
            break;
        }
    }

    /* Retire in ring order so the dequeue index stays in step with the xHC */
    while (!IsListEmpty(&m_Pending))
    {
        Command = XhciCmdFromLink(m_Pending.Flink);
        if (!Command->Matched)
            break;

        RemoveHeadList(&m_Pending);
        ASSERT(Command->RingIndex == m_Dequeue);
        m_Dequeue = (m_Dequeue + 1) % XhciCmdUsable;
        Command->Status = XhciCmdStatusFromCode(Command->Code);
        InsertTailList(&Finished, &Command->Link);
        Retired = TRUE;

        if (!IsListEmpty(&m_Waiting))
            PlaceLocked(XhciCmdFromLink(RemoveHeadList(&m_Waiting)), TRUE, &Finished);
    }

    /* QUIRK: an event that matches nothing counts as out of order too */
    if (!Retired && !m_OutOfOrderSeen)
    {
        DPRINT1("Command completion out of order: TRB 0x%I64x code %lu\n",
                Pointer,
                static_cast<ULONG>(Code));
        m_OutOfOrderSeen = TRUE;
        OutOfOrder = TRUE;
    }

    if (IsListEmpty(&m_Pending) && !m_Aborting)
        WdfTimerStop(m_Timer, FALSE);

    KeReleaseSpinLock(&m_Lock, OldIrql);

    if (OutOfOrder)
        m_Controller->RaiseControllerFault(XhciRecovery::Ignore, XhciCmdReasonOutOfOrder);

    XhciCmdCompleteList(&Finished);
}

VOID
XhciCommandRing::RestartStoppedRingLocked(
    _Inout_ PLIST_ENTRY Finished)
{
    LIST_ENTRY Requeue;
    XhciCommand* Head;
    ULONG RingCycle;

    if (!IsListEmpty(&m_Pending))
    {
        Head = XhciCmdFromLink(m_Pending.Flink);

        if (Head->Countdown <= 0)
        {
            DPRINT1("Command type %lu aborted twice, failing it\n", XhciTrbType(&Head->Trb));

            RemoveHeadList(&m_Pending);
            m_Dequeue = (m_Dequeue + 1) % XhciCmdUsable;

            /* TRBs behind a wrapped enqueue index still carry the previous cycle */
            RingCycle = m_Cycle;
            if (m_Dequeue > m_Enqueue)
                RingCycle ^= XHCI_TRB_CYCLE;

            m_Controller->m_Registers.WriteOperational64(XHCI_OP_CRCR,
                                                         TrbAddress(m_Dequeue) | RingCycle);

            Head->Code = XhciCompletionCode::CommandAborted;
            Head->Status = STATUS_IO_TIMEOUT;
            InsertTailList(Finished, &Head->Link);
        }
        else if (Head->Countdown == XhciCmdTicksFirstAbort)
        {
            DPRINT1("Command type %lu still not done after the first abort, retrying\n",
                    XhciTrbType(&Head->Trb));
        }
        else if (Head->Countdown != XhciCmdTicksInitial)
        {
            DPRINT1("Unexpected command countdown %ld after the ring stopped\n", Head->Countdown);
        }
    }

    if (!IsListEmpty(&m_Pending))
    {
        KeMemoryBarrier();
        m_Controller->m_Registers.RingDoorbell(0, 0);
    }

    InitializeListHead(&Requeue);
    while (!IsListEmpty(&m_Waiting))
        InsertTailList(&Requeue, RemoveHeadList(&m_Waiting));

    while (!IsListEmpty(&Requeue))
        PlaceLocked(XhciCmdFromLink(RemoveHeadList(&Requeue)), FALSE, Finished);
}

VOID
XhciCommandRing::HandleRingStopped(
    _In_ const XHCI_TRB* Event)
{
    LIST_ENTRY Finished;
    ULONG64 Reported;
    ULONG64 Expected;
    KIRQL OldIrql;

    InitializeListHead(&Finished);
    Reported = XhciTrbPointer(Event) & ~0xFULL;

    KeAcquireSpinLock(&m_Lock, &OldIrql);

    m_Aborting = FALSE;
    Expected = TrbAddress(m_Dequeue);

    if (Reported != Expected && !(Reported == m_LinkAddress && m_Dequeue == 0))
    {
        DPRINT1("Command ring stopped at 0x%I64x, expected 0x%I64x; asking for a reset\n",
                Reported,
                Expected);
        m_Reset = ResetPhase::Requested;
        KeReleaseSpinLock(&m_Lock, OldIrql);

        m_Controller->RaiseControllerFault(XhciRecovery::ResetHost, XhciCmdReasonDequeueMismatch);
        return;
    }

    RestartStoppedRingLocked(&Finished);

    KeReleaseSpinLock(&m_Lock, OldIrql);

    XhciCmdCompleteList(&Finished);
}

VOID
NTAPI
XhciCommandRing::EvtTimer(
    _In_ WDFTIMER Timer)
{
    XhciGetCommandTimerContext(Timer)->Ring->OnTimer();
}

VOID
XhciCommandRing::OnTimer()
{
    LIST_ENTRY Finished;
    XhciCommand* Head;
    ULONG64 Crcr;
    BOOLEAN Accessible;
    BOOLEAN NewlyGone;
    BOOLEAN WantReset;
    BOOLEAN WantInternal = FALSE;
    KIRQL OldIrql;

    InitializeListHead(&Finished);
    Accessible = m_Controller->IsAccessible();
    NewlyGone = FALSE;
    WantReset = FALSE;

    KeAcquireSpinLock(&m_Lock, &OldIrql);

    if (Accessible && m_Reset == ResetPhase::Idle && m_State == RingState::Prepared)
    {
        Head = IsListEmpty(&m_Pending) ? NULL : XhciCmdFromLink(m_Pending.Flink);

        if (!Head && !m_Aborting)
        {
            WdfTimerStop(m_Timer, FALSE);
        }
        else
        {
            Crcr = m_Controller->m_Registers.ReadOperational64(XHCI_OP_CRCR);

            if (Crcr == ~0ULL)
            {
                DPRINT1("Command ring control reads all ones, controller is gone\n");
                Accessible = FALSE;
                NewlyGone = TRUE;
            }
            else if (m_Aborting)
            {
                m_AbortTicks--;
                if (m_AbortTicks <= 0)
                {
                    m_Aborting = FALSE;

                    if (Crcr & XHCI_CRCR_RUNNING)
                    {
                        DPRINT1("Command abort timed out with the ring running; asking for a reset\n");
                        WdfTimerStop(m_Timer, FALSE);
                        m_Reset = ResetPhase::Requested;
                        WantReset = TRUE;
                        WantInternal = (Head != NULL && Head->Internal);
                    }
                    else
                    {
                        DPRINT1("Command abort timed out with the ring stopped; restarting it\n");
                        RestartStoppedRingLocked(&Finished);
                    }
                }
            }
            else
            {
                if (Head->Countdown > 0)
                    Head->Countdown--;

                if (Head->Countdown == XhciCmdTicksFirstAbort || Head->Countdown == 0)
                {
                    if (Crcr & XHCI_CRCR_RUNNING)
                    {
                        DPRINT1("Command type %lu timed out, aborting\n", XhciTrbType(&Head->Trb));

                        /* Address Device with BSR clear waits on the bus, so it is not a controller fault */
                        if (XhciTrbType(&Head->Trb) != static_cast<ULONG>(XhciTrbType::AddressDevice) ||
                            (Head->Trb.Dword[3] & XhciCmdBlockSetAddress))
                        {
                            DPRINT1("Controller did not finish a command within %ld s\n",
                                    XhciCmdTicksInitial - Head->Countdown);
                        }

                        m_Aborting = TRUE;
                        m_AbortTicks = XhciCmdTicksAbortWait;
                        m_Controller->m_Registers.WriteOperational64(XHCI_OP_CRCR,
                                                                     Crcr | XHCI_CRCR_ABORT);
                    }
                    else
                    {
                        DPRINT1("Command ring not running with a command pending; asking for a reset\n");
                        WdfTimerStop(m_Timer, FALSE);
                        m_Reset = ResetPhase::Requested;
                        WantReset = TRUE;
                        WantInternal = Head->Internal;
                    }
                }
            }
        }
    }

    if (!Accessible)
    {
        if (m_DrainFromTimer)
            DrainAllLocked(&Finished);

        WdfTimerStop(m_Timer, FALSE);
    }

    KeReleaseSpinLock(&m_Lock, OldIrql);

    if (NewlyGone)
        m_Controller->MarkGone();

    XhciCmdCompleteList(&Finished);

    if (WantReset)
        m_Controller->RaiseControllerFault(WantInternal ? XhciRecovery::InternalReset : XhciRecovery::ResetHost,
                                       XhciCmdReasonAbortFailed);
}

VOID
XhciCommandRing::PreReset()
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&m_Lock, &OldIrql);
    m_Reset = ResetPhase::Resetting;
    m_Aborting = FALSE;
    KeReleaseSpinLock(&m_Lock, OldIrql);
}

VOID
XhciCommandRing::PostReset()
{
    LIST_ENTRY Finished;
    KIRQL OldIrql;

    InitializeListHead(&Finished);

    KeAcquireSpinLock(&m_Lock, &OldIrql);
    DrainAllLocked(&Finished);
    WdfTimerStop(m_Timer, FALSE);
    m_Reset = ResetPhase::AfterReset;
    KeReleaseSpinLock(&m_Lock, OldIrql);

    XhciCmdCompleteList(&Finished);

    InitializeRing();
}

VOID
XhciCommandRing::PostResetSuccess()
{
    LIST_ENTRY Finished;
    LIST_ENTRY Requeue;
    KIRQL OldIrql;

    InitializeListHead(&Finished);
    InitializeListHead(&Requeue);

    KeAcquireSpinLock(&m_Lock, &OldIrql);

    m_Reset = ResetPhase::Idle;

    while (!IsListEmpty(&m_Waiting))
        InsertTailList(&Requeue, RemoveHeadList(&m_Waiting));

    while (!IsListEmpty(&Requeue))
        PlaceLocked(XhciCmdFromLink(RemoveHeadList(&Requeue)), FALSE, &Finished);

    KeReleaseSpinLock(&m_Lock, OldIrql);

    XhciCmdCompleteList(&Finished);
}

VOID
XhciCommandRing::FailEverything()
{
    LIST_ENTRY Finished;
    KIRQL OldIrql;

    InitializeListHead(&Finished);

    KeAcquireSpinLock(&m_Lock, &OldIrql);
    m_Reset = ResetPhase::Failed;
    m_Aborting = FALSE;
    m_DrainFromTimer = TRUE;
    DrainAllLocked(&Finished);
    if (m_Timer)
        WdfTimerStop(m_Timer, FALSE);
    KeReleaseSpinLock(&m_Lock, OldIrql);

    XhciCmdCompleteList(&Finished);
}

VOID
XhciCommandRing::FailAll()
{
    FailEverything();
}

VOID
XhciCommandRing::OnHostLost()
{
    FailEverything();
}
