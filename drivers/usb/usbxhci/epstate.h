/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Endpoint state machine
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include <drivers/usb3/smengine.h>

class XhciEndpoint;

enum class XepEvent : UCHAR
{
    /* Command and mapping completions, taken even while the endpoint is quiet */
    CommandSucceeded,
    CommandFailed,
    StreamMapStopped,
    CancelAllowed,
    StopFailedHalted,
    StopFailedRunning,
    StopFailedStopped,

    /* Transfer ring, hardware and timer notifications */
    BufferAllocFailed,
    ExpectedEventsDone,
    StoppedEventSeen,
    HaltReported,
    StreamsOff,
    StreamsOn,
    TimerFired,
    TransferAborted,
    TransfersRecovered,

    /* Requests from the controller, the endpoint owner and the client driver */
    ControllerResetStarting,
    ControllerResetDone,
    ControllerRemoved,
    Disable,
    Enable,
    ClientReset,

    Count
};

/** Which missing completion a timer expiry stands in for. */
typedef enum _XEP_TIMEOUT
{
    XepTimeoutHalt,
    XepTimeoutStoppedEvent,
    XepTimeoutDrainAfterStop,
    XepTimeoutDrainAfterHalt
} XEP_TIMEOUT;

class XhciEndpointMachine : public SmMachine<XhciEndpointMachine, XepEvent>
{
    friend class SmMachine<XhciEndpointMachine, XepEvent>;
    friend class XhciEndpointProbe;

public:
    VOID
    Initialize(
        _In_ XhciEndpoint* Endpoint);

    XhciEndpoint*
    Endpoint() const
    {
        return m_Endpoint;
    }

    static const SM_EVENT_INFO EventInfo[];

private:
    enum MAPPING_STOP_REASON
    {
        MappingStopForCancel,
        MappingStopForHalt,
        MappingStopForReset,
        MappingStopForRemoval
    };

    enum RECLAIM_REASON
    {
        ReclaimAfterStop,
        ReclaimAfterAutoReset,
        ReclaimAfterHaltClear,
        ReclaimAfterControllerReset
    };

    enum TIMER_EXIT
    {
        TimerExitHalt,
        TimerExitStoppedEvent,
        TimerExitStopDrained,
        TimerExitHaltDrained
    };

    /* States */
    static const State Disabled;
    static const State Active;
    static const State Running;
    static const State Mapping;
    static const State ResetWhileMapping;
    static const State AwaitingDisable;
    static const State Parked;
    static const State AwaitingClientReset;
    static const State AwaitingRecovery;
    static const State Fenced;
    static const State AwaitingControllerReset;
    static const State Removed;
    static const State HaltingRingFill;
    static const State AwaitingCancelOk;
    static const State Commanding;
    static const State StoppingPipe;
    static const State RebuildingContextAfterStop;
    static const State UpdatingDequeue;
    static const State ResettingDefaultPipe;
    static const State ClientResetCommand;
    static const State ResettingPipe;
    static const State RebuildingContextForReset;
    static const State Reclaiming;
    static const State Timed;
    static const State AwaitingStopRetry;
    static const State AwaitingHalt;
    static const State AwaitingStoppedEvent;
    static const State Draining;
    static const State LeavingTimer;

    /* Engine hooks */
    static const BOOLEAN SmCompletionsFirst = FALSE;
    static const USHORT SmTimerEventId = static_cast<USHORT>(XepEvent::TimerFired);

    VOID
    SmReference();

    VOID
    SmDereference();

    BOOLEAN
    SmCancelTimer();

    /* Handlers */
    SM_RESULT
    OnDisabled(
        _In_ XepEvent Event);
    SM_RESULT
    OnActive(
        _In_ XepEvent Event);
    SM_RESULT
    OnRunning(
        _In_ XepEvent Event);
    SM_RESULT
    OnMapping(
        _In_ XepEvent Event);
    SM_RESULT
    OnParked(
        _In_ XepEvent Event);
    SM_RESULT
    OnAwaitingClientReset(
        _In_ XepEvent Event);
    SM_RESULT
    OnAwaitingControllerReset(
        _In_ XepEvent Event);
    SM_RESULT
    OnHaltingRingFill(
        _In_ XepEvent Event);
    SM_RESULT
    OnAwaitingCancelOk(
        _In_ XepEvent Event);
    SM_RESULT
    OnCommanding(
        _In_ XepEvent Event);
    SM_RESULT
    OnStoppingPipe(
        _In_ XepEvent Event);
    SM_RESULT
    OnRebuildingContextAfterStop(
        _In_ XepEvent Event);
    SM_RESULT
    OnUpdatingDequeue(
        _In_ XepEvent Event);
    SM_RESULT
    OnResettingDefaultPipe(
        _In_ XepEvent Event);
    SM_RESULT
    OnClientResetCommand(
        _In_ XepEvent Event);
    SM_RESULT
    OnResettingPipe(
        _In_ XepEvent Event);
    SM_RESULT
    OnRebuildingContextForReset(
        _In_ XepEvent Event);
    SM_RESULT
    OnReclaiming(
        _In_ XepEvent Event);
    SM_RESULT
    OnAwaitingStopRetry(
        _In_ XepEvent Event);
    SM_RESULT
    OnAwaitingHalt(
        _In_ XepEvent Event);
    SM_RESULT
    OnAwaitingStoppedEvent(
        _In_ XepEvent Event);
    SM_RESULT
    OnDraining(
        _In_ XepEvent Event);

    /* Entry actions */
    SM_RESULT EnterAwaitingControllerReset();
    SM_RESULT EnterRemoved();
    SM_RESULT EnterHaltingRingFill();
    SM_RESULT EnterAwaitingCancelOk();
    SM_RESULT EnterStoppingPipe();
    SM_RESULT EnterRebuildingContextAfterStop();
    SM_RESULT EnterUpdatingDequeue();
    SM_RESULT EnterResettingDefaultPipe();
    SM_RESULT EnterResettingPipe();
    SM_RESULT EnterRebuildingContextForReset();
    SM_RESULT EnterReclaiming();
    SM_RESULT EnterAwaitingStopRetry();
    SM_RESULT EnterAwaitingHalt();
    SM_RESULT EnterAwaitingStoppedEvent();
    SM_RESULT EnterDraining();
    SM_RESULT EnterLeavingTimer();

    /* Helpers */
    SM_RESULT
    Remap();

    SM_RESULT
    SuspendRingFillFor(
        _In_ MAPPING_STOP_REASON Reason);

    SM_RESULT
    ReclaimFor(
        _In_ RECLAIM_REASON Reason);

    SM_RESULT
    RetryStopAfter(
        _In_ ULONG Milliseconds);

    SM_RESULT
    DrainFor(
        _In_ BOOLEAN AfterHalt);

    SM_RESULT
    LeaveTimerFor(
        _In_ TIMER_EXIT Exit);

    SM_RESULT
    RunTimerExit(
        _In_ TIMER_EXIT Exit);

    SM_RESULT BeginStop();
    SM_RESULT AfterStopCommand();
    SM_RESULT AfterStopDrained();
    SM_RESULT AfterHalt();
    SM_RESULT AfterStoppedEvent();
    SM_RESULT AfterHaltDrained();

    BOOLEAN
    ReclaimIgnoresCancel() const
    {
        return (m_AfterReclaim == ReclaimAfterAutoReset) ||
               (m_AfterReclaim == ReclaimAfterControllerReset);
    }

    /* Actions, implemented by the endpoint object */
    VOID ReferenceEndpoint();
    VOID DereferenceEndpoint();
    BOOLEAN StopTimer();
    VOID
    StartTimer(
        _In_ ULONG Milliseconds);
    VOID
    ReportTimeout(
        _In_ XEP_TIMEOUT Timeout);
    VOID ResumeRingFill();
    VOID SuspendRingFill();
    VOID AckControllerReset();
    VOID CompleteResetRequest();
    VOID CompleteResetRequestIfAllowed();
    VOID ReportStoppedEvent();
    VOID DropStoppedEvent();
    VOID ResetStopAttempts();
    VOID CountStopAttempt();
    VOID SendStopCommand();
    VOID NotifyRingsHalted();
    VOID NotifyRingsClientReset();
    VOID NotifyRingsStoppedEvent();
    VOID NotifyRingsReclaimOnCancel();
    VOID NotifyRingsDrainEvents();
    VOID NotifyRingsReclaim();
    VOID AskUcxToCancel();
    VOID PurgeStreamQueues();
    VOID RestartStreamQueues();
    VOID ReconfigureForReset();
    VOID ReconfigureAfterStop();
    VOID UpdateDequeuePointers();
    VOID ResetDefaultPipe();
    VOID ResetEndpoint();
    VOID RequestControllerReset();

    /* Queries, implemented by the endpoint object */
    BOOLEAN IsControlEndpoint();
    BOOLEAN OnPrimaryInterrupterOnly();
    BOOLEAN DelaysFirstStop();
    BOOLEAN IgnoresStopContextError();
    BOOLEAN StopAttemptsExhausted();
    BOOLEAN DoorbellRungSinceFill();
    BOOLEAN NeedsContextRebuildAfterStop();
    BOOLEAN ShouldReconfigureForReset();
    BOOLEAN CanResetAfterHalt();

    XhciEndpoint* m_Endpoint;
    MAPPING_STOP_REASON m_AfterMapping;
    RECLAIM_REASON m_AfterReclaim;
    TIMER_EXIT m_AfterTimer;
    ULONG m_RetryDelay;
    BOOLEAN m_DrainAfterHalt;
};
