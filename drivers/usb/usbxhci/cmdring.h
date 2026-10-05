/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Command ring
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

class XhciController;
struct XhciCommand;

typedef VOID (NTAPI *PFN_XHCI_COMMAND_DONE)(_In_ XhciCommand* Command);

/**
 * Submitter fills Trb (minus the cycle bit) and Done. The ring fills Code and
 * Completion, then calls Done at DISPATCH_LEVEL.
 */
struct XhciCommand
{
    XHCI_TRB Trb;
    PFN_XHCI_COMMAND_DONE Done;
    PVOID Context;
    XhciCompletionCode Code;
    XHCI_TRB Completion;
    NTSTATUS Status;            /**< STATUS_SUCCESS, or why the command never ran */
    BOOLEAN Internal;           /**< Issued by the driver during D0 entry; a stuck one asks for an internal reset */
    LIST_ENTRY Link;            /**< Owned by the ring while the command is pending */

    /*
     * Status values: STATUS_SUCCESS (code Success), STATUS_UNSUCCESSFUL (any other
     * code), STATUS_IO_TIMEOUT (aborted), STATUS_NO_SUCH_DEVICE (controller gone or reset).
     */
    ULONG RingIndex;            /**< TRB slot while on the ring */
    LONG Countdown;             /**< Timer ticks left before the next abort */
    BOOLEAN Matched;            /**< Completion event seen, waiting for in order retirement */
    PFN_XHCI_COMMAND_DONE WaiterDone;
    PKEVENT WaiterEvent;
};

/** The command ring. */
class XhciCommandRing
{
public:
    NTSTATUS
    Create(
        _In_ XhciController* Controller);
    NTSTATUS Prepare();
    VOID Release();
    NTSTATUS D0Entry();
    VOID PostInterruptsEntry();
    VOID PreReset();
    VOID PostReset();
    VOID PostResetSuccess();
    VOID FailAll();
    VOID OnHostLost();

    /** Queues Command; Done always runs exactly once, possibly before this returns. */
    _IRQL_requires_max_(DISPATCH_LEVEL)
    VOID
    Submit(
        _Inout_ XhciCommand* Command);

    /** PASSIVE_LEVEL helper: submits and waits for Done. */
    _IRQL_requires_(PASSIVE_LEVEL)
    NTSTATUS
    SubmitAndWait(
        _Inout_ XhciCommand* Command);

    /* From the primary interrupter DPC */
    VOID
    OnCompletionEvent(
        _In_ const XHCI_TRB* Event);


    static EVT_WDF_TIMER EvtTimer;

private:
    enum class RingState : ULONG
    {
        Uninitialized,
        Prepared,
        Released
    };

    enum class ResetPhase : ULONG
    {
        Idle,
        Requested,
        Resetting,
        AfterReset,
        Failed
    };

    VOID InitializeRing();
    VOID OnTimer();

    VOID
    PlaceLocked(
        _Inout_ XhciCommand* Command,
        _In_ BOOLEAN FrontOfLine,
        _Inout_ PLIST_ENTRY Finished);

    VOID
    PutOnRingLocked(
        _Inout_ XhciCommand* Command);

    VOID
    RestartStoppedRingLocked(
        _Inout_ PLIST_ENTRY Finished);

    VOID
    HandleRingStopped(
        _In_ const XHCI_TRB* Event);

    VOID
    DrainAllLocked(
        _Inout_ PLIST_ENTRY Finished);

    VOID
    FailEverything();

    ULONG64
    TrbAddress(
        _In_ ULONG Index) const
    {
        return m_RingAddress + Index * sizeof(XHCI_TRB);
    }

    XhciController* m_Controller;
    WDFTIMER m_Timer;
    KSPIN_LOCK m_Lock;
    RingState m_State;
    ResetPhase m_Reset;
    XhciDmaBuffer* m_Segment;
    PXHCI_TRB m_Trbs;
    ULONG64 m_RingAddress;
    ULONG64 m_LinkAddress;
    ULONG m_Enqueue;
    ULONG m_Dequeue;
    ULONG m_Cycle;
    LIST_ENTRY m_Pending;
    LIST_ENTRY m_Waiting;
    BOOLEAN m_OutOfOrderSeen;
    BOOLEAN m_Aborting;
    LONG m_AbortTicks;
    BOOLEAN m_DrainFromTimer;
};
