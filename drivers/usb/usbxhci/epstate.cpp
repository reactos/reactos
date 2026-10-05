/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Endpoint state machine
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include "xhcidrv.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

#define XEP_STOP_TIMEOUT_MS         2000
#define XEP_STOP_RETRY_SHORT_MS     200
#define XEP_STOP_RETRY_LONG_MS      500

typedef XhciEndpointMachine Xep;

const SM_EVENT_INFO XhciEndpointMachine::EventInfo[] =
{
    { "CommandSucceeded",        SmEventCritical,   FALSE, FALSE },
    { "CommandFailed",           SmEventCritical,   FALSE, FALSE },
    { "StreamMapStopped",          SmEventCritical,   FALSE, FALSE },
    { "CancelAllowed",           SmEventCritical,   FALSE, FALSE },
    { "StopFailedHalted",        SmEventCritical,   FALSE, FALSE },
    { "StopFailedRunning",       SmEventCritical,   FALSE, FALSE },
    { "StopFailedStopped",       SmEventCritical,   FALSE, FALSE },
    { "BufferAllocFailed",       SmEventCompletion, FALSE, FALSE },
    { "ExpectedEventsDone",      SmEventCompletion, FALSE, FALSE },
    { "StoppedEventSeen",        SmEventCompletion, FALSE, FALSE },
    { "HaltReported",            SmEventCompletion, FALSE, FALSE },
    { "StreamsOff",         SmEventCompletion, FALSE, FALSE },
    { "StreamsOn",          SmEventCompletion, FALSE, FALSE },
    { "TimerFired",              SmEventCompletion, FALSE, FALSE },
    { "TransferAborted",        SmEventCompletion, FALSE, FALSE },
    { "TransfersRecovered",      SmEventCompletion, FALSE, FALSE },
    { "ControllerResetStarting", SmEventRequest,    FALSE, FALSE },
    { "ControllerResetDone",     SmEventRequest,    FALSE, FALSE },
    { "ControllerRemoved",       SmEventRequest,    FALSE, FALSE },
    { "Disable",                 SmEventRequest,    FALSE, FALSE },
    { "Enable",                  SmEventRequest,    FALSE, FALSE },
    { "ClientReset",             SmEventRequest,    FALSE, FALSE },
};

static_assert(RTL_NUMBER_OF(XhciEndpointMachine::EventInfo) == static_cast<ULONG>(XepEvent::Count),
              "EventInfo must have one entry per XepEvent");

static const XepEvent DisabledDiscards[] =
    { XepEvent::ControllerResetStarting, XepEvent::ControllerRemoved, XepEvent::Disable, XepEvent::Count };

static const XepEvent AwaitingDisableDiscards[] =
    { XepEvent::ControllerRemoved, XepEvent::Count };

static const XepEvent CancelDiscards[] =
    { XepEvent::TransferAborted, XepEvent::Count };

static const XepEvent AwaitingRecoveryDiscards[] =
    { XepEvent::HaltReported, XepEvent::TransferAborted, XepEvent::Count };

static const XepEvent FencedDiscards[] =
    { XepEvent::ClientReset, XepEvent::HaltReported, XepEvent::TransferAborted, XepEvent::Count };

/* Not programmed on the controller. Initial state */
const Xep::State Xep::Disabled =
    { NULL, "Disabled", SM_STATE_TAKES_REQUESTS, &Xep::OnDisabled, NULL, DisabledDiscards };

/* Everything but Disabled; Disable is only ever taken by the request states below */
const Xep::State Xep::Active =
    { NULL, "Active", 0, &Xep::OnActive, NULL };

/* Transfer rings are mapping, the controller reset done path acks and parks */
const Xep::State Xep::Running =
    { &Active, "Running", SM_STATE_TAKES_REQUESTS, &Xep::OnRunning, NULL };

const Xep::State Xep::Mapping =
    { &Running, "Mapping", 0, &Xep::OnMapping, NULL };

const Xep::State Xep::ResetWhileMapping =
    { &Running, "ResetWhileMapping", 0, NULL, NULL };

/* Acked a controller reset, only a disable gets it going again */
const Xep::State Xep::AwaitingDisable =
    { &Active, "AwaitingDisable", SM_STATE_TAKES_REQUESTS, NULL, NULL, AwaitingDisableDiscards };

/* Stopped and waiting for outside help */
const Xep::State Xep::Parked =
    { &Active, "Parked", SM_STATE_TAKES_REQUESTS, &Xep::OnParked, NULL };

const Xep::State Xep::AwaitingClientReset =
    { &Parked, "AwaitingClientReset", 0, &Xep::OnAwaitingClientReset, NULL, CancelDiscards };

/* A command failed, the controller has to be reset or removed */
const Xep::State Xep::AwaitingRecovery =
    { &Parked, "AwaitingRecovery", 0, NULL, NULL, AwaitingRecoveryDiscards };

/* The controller is resetting or gone, client resets and halts no longer matter */
const Xep::State Xep::Fenced =
    { &Active, "Fenced", SM_STATE_TAKES_REQUESTS, NULL, NULL, FencedDiscards };

const Xep::State Xep::AwaitingControllerReset =
    { &Fenced, "AwaitingControllerReset", 0,
      &Xep::OnAwaitingControllerReset, &Xep::EnterAwaitingControllerReset };

const Xep::State Xep::Removed =
    { &Fenced, "Removed", 0, NULL, &Xep::EnterRemoved };

/* Transfer rings stop mapping, m_AfterMapping says why */
const Xep::State Xep::HaltingRingFill =
    { &Active, "HaltingRingFill", SM_STATE_CRITICAL_ONLY,
      &Xep::OnHaltingRingFill, &Xep::EnterHaltingRingFill };

/* UCX was asked to cancel the transfers */
const Xep::State Xep::AwaitingCancelOk =
    { &Active, "AwaitingCancelOk", SM_STATE_CRITICAL_ONLY,
      &Xep::OnAwaitingCancelOk, &Xep::EnterAwaitingCancelOk };

/* A command is on the command ring */
const Xep::State Xep::Commanding =
    { &Active, "Commanding", 0, &Xep::OnCommanding, NULL };

const Xep::State Xep::StoppingPipe =
    { &Commanding, "StoppingPipe", SM_STATE_CRITICAL_ONLY,
      &Xep::OnStoppingPipe, &Xep::EnterStoppingPipe };

const Xep::State Xep::RebuildingContextAfterStop =
    { &Commanding, "RebuildingContextAfterStop", SM_STATE_CRITICAL_ONLY,
      &Xep::OnRebuildingContextAfterStop, &Xep::EnterRebuildingContextAfterStop };

const Xep::State Xep::UpdatingDequeue =
    { &Commanding, "UpdatingDequeue", SM_STATE_CRITICAL_ONLY,
      &Xep::OnUpdatingDequeue, &Xep::EnterUpdatingDequeue };

const Xep::State Xep::ResettingDefaultPipe =
    { &Commanding, "ResettingDefaultPipe", 0,
      &Xep::OnResettingDefaultPipe, &Xep::EnterResettingDefaultPipe, CancelDiscards };

/* Commands run for a client reset; a failure completes the client's request */
const Xep::State Xep::ClientResetCommand =
    { &Commanding, "ClientResetCommand", 0, &Xep::OnClientResetCommand, NULL };

const Xep::State Xep::ResettingPipe =
    { &ClientResetCommand, "ResettingPipe", 0,
      &Xep::OnResettingPipe, &Xep::EnterResettingPipe };

const Xep::State Xep::RebuildingContextForReset =
    { &ClientResetCommand, "RebuildingContextForReset", 0,
      &Xep::OnRebuildingContextForReset, &Xep::EnterRebuildingContextForReset };

/* Transfer rings hand back their transfers, m_AfterReclaim says what follows */
const Xep::State Xep::Reclaiming =
    { &Active, "Reclaiming", 0, &Xep::OnReclaiming, &Xep::EnterReclaiming };

/* The state timer runs; leaving for anything but its expiry stops it */
const Xep::State Xep::Timed =
    { &Active, "Timed", SM_STATE_STOP_TIMER_ON_EXIT, NULL, NULL };

const Xep::State Xep::AwaitingStopRetry =
    { &Timed, "AwaitingStopRetry", 0, &Xep::OnAwaitingStopRetry, &Xep::EnterAwaitingStopRetry };

const Xep::State Xep::AwaitingHalt =
    { &Timed, "AwaitingHalt", 0, &Xep::OnAwaitingHalt, &Xep::EnterAwaitingHalt };

const Xep::State Xep::AwaitingStoppedEvent =
    { &Timed, "AwaitingStoppedEvent", 0,
      &Xep::OnAwaitingStoppedEvent, &Xep::EnterAwaitingStoppedEvent };

const Xep::State Xep::Draining =
    { &Timed, "Draining", 0, &Xep::OnDraining, &Xep::EnterDraining };

/* Left a timed state early. Waits here when the timer could not be stopped */
const Xep::State Xep::LeavingTimer =
    { &Active, "LeavingTimer", 0, NULL, &Xep::EnterLeavingTimer };

/* FUNCTIONS ******************************************************************/

VOID
XhciEndpointMachine::Initialize(
    _In_ XhciEndpoint* Endpoint)
{
    m_Endpoint = Endpoint;
    m_AfterMapping = MappingStopForCancel;
    m_AfterReclaim = ReclaimAfterStop;
    m_AfterTimer = TimerExitHalt;
    m_RetryDelay = XEP_STOP_RETRY_SHORT_MS;
    m_DrainAfterHalt = FALSE;

    SmInitialize(&Disabled);
}

VOID
XhciEndpointMachine::SmReference()
{
    ReferenceEndpoint();
}

VOID
XhciEndpointMachine::SmDereference()
{
    DereferenceEndpoint();
}

BOOLEAN
XhciEndpointMachine::SmCancelTimer()
{
    return StopTimer();
}

SM_RESULT
XhciEndpointMachine::OnDisabled(
    _In_ XepEvent Event)
{
    switch (Event)
    {
        case XepEvent::ControllerResetDone:
            /* Enter again so the discards are purged once more */
            AckControllerReset();
            return SmTransition(&Disabled);

        case XepEvent::Enable:
            return Remap();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
XhciEndpointMachine::OnActive(
    _In_ XepEvent Event)
{
    if (Event == XepEvent::Disable)
        return SmTransition(&Disabled);

    return SmUnhandled();
}

SM_RESULT
XhciEndpointMachine::OnRunning(
    _In_ XepEvent Event)
{
    if (Event != XepEvent::ControllerResetDone)
        return SmUnhandled();

    AckControllerReset();
    return SmTransition(&AwaitingDisable);
}

SM_RESULT
XhciEndpointMachine::OnMapping(
    _In_ XepEvent Event)
{
    switch (Event)
    {
        case XepEvent::ControllerResetStarting:
            return SmTransition(&ResetWhileMapping);

        case XepEvent::StreamsOn:
        case XepEvent::StreamsOff:
            return Remap();

        case XepEvent::TransferAborted:
            return SuspendRingFillFor(MappingStopForCancel);

        case XepEvent::HaltReported:
            return SuspendRingFillFor(MappingStopForHalt);

        case XepEvent::ControllerRemoved:
            return SuspendRingFillFor(MappingStopForRemoval);

        case XepEvent::ClientReset:
            NotifyRingsClientReset();
            if (ShouldReconfigureForReset())
                return SuspendRingFillFor(MappingStopForReset);

            CompleteResetRequest();
            return SmTransition(&Mapping);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
XhciEndpointMachine::OnParked(
    _In_ XepEvent Event)
{
    switch (Event)
    {
        case XepEvent::ControllerResetStarting:
            return SmTransition(&AwaitingControllerReset);

        case XepEvent::ControllerRemoved:
            return SmTransition(&Removed);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
XhciEndpointMachine::OnAwaitingClientReset(
    _In_ XepEvent Event)
{
    if (Event != XepEvent::ClientReset)
        return SmUnhandled();

    if (CanResetAfterHalt())
        return SmTransition(&ResettingPipe);

    CompleteResetRequest();
    return SmTransition(&AwaitingClientReset);
}

SM_RESULT
XhciEndpointMachine::EnterAwaitingControllerReset()
{
    CompleteResetRequestIfAllowed();
    return SmHandled();
}

SM_RESULT
XhciEndpointMachine::OnAwaitingControllerReset(
    _In_ XepEvent Event)
{
    if (Event == XepEvent::ControllerResetDone)
        return ReclaimFor(ReclaimAfterControllerReset);

    return SmUnhandled();
}

SM_RESULT
XhciEndpointMachine::EnterRemoved()
{
    NotifyRingsReclaimOnCancel();
    CompleteResetRequestIfAllowed();
    return SmHandled();
}

SM_RESULT
XhciEndpointMachine::EnterHaltingRingFill()
{
    SuspendRingFill();
    if (m_AfterMapping == MappingStopForHalt)
        SmDiscardQueued(XepEvent::TransferAborted);

    return SmHandled();
}

SM_RESULT
XhciEndpointMachine::OnHaltingRingFill(
    _In_ XepEvent Event)
{
    if (Event != XepEvent::StreamMapStopped)
        return SmUnhandled();

    switch (m_AfterMapping)
    {
        case MappingStopForCancel:
            if (DoorbellRungSinceFill())
                return BeginStop();

            return ReclaimFor(ReclaimAfterStop);

        case MappingStopForHalt:
            return AfterHalt();

        case MappingStopForReset:
            return SmTransition(&RebuildingContextForReset);

        default:
            return SmTransition(&Removed);
    }
}

SM_RESULT
XhciEndpointMachine::EnterAwaitingCancelOk()
{
    AskUcxToCancel();
    return SmHandled();
}

SM_RESULT
XhciEndpointMachine::OnAwaitingCancelOk(
    _In_ XepEvent Event)
{
    if (Event == XepEvent::CancelAllowed)
        return ReclaimFor(ReclaimAfterStop);

    return SmUnhandled();
}

SM_RESULT
XhciEndpointMachine::OnCommanding(
    _In_ XepEvent Event)
{
    if (Event == XepEvent::CommandFailed)
        return SmTransition(&AwaitingRecovery);

    return SmUnhandled();
}

SM_RESULT
XhciEndpointMachine::EnterStoppingPipe()
{
    CountStopAttempt();
    SendStopCommand();
    return SmHandled();
}

SM_RESULT
XhciEndpointMachine::OnStoppingPipe(
    _In_ XepEvent Event)
{
    switch (Event)
    {
        case XepEvent::CommandSucceeded:
            return AfterStopCommand();

        case XepEvent::StopFailedHalted:
            return SmTransition(&AwaitingHalt);

        case XepEvent::StopFailedRunning:
            return RetryStopAfter(XEP_STOP_RETRY_LONG_MS);

        case XepEvent::StopFailedStopped:
            return RetryStopAfter(XEP_STOP_RETRY_SHORT_MS);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
XhciEndpointMachine::EnterRebuildingContextAfterStop()
{
    ReconfigureAfterStop();
    return SmHandled();
}

SM_RESULT
XhciEndpointMachine::OnRebuildingContextAfterStop(
    _In_ XepEvent Event)
{
    if (Event == XepEvent::CommandSucceeded)
        return SmTransition(&UpdatingDequeue);

    return SmUnhandled();
}

SM_RESULT
XhciEndpointMachine::EnterUpdatingDequeue()
{
    UpdateDequeuePointers();
    return SmHandled();
}

SM_RESULT
XhciEndpointMachine::OnUpdatingDequeue(
    _In_ XepEvent Event)
{
    if (Event == XepEvent::CommandSucceeded)
        return SmTransition(&AwaitingCancelOk);

    return SmUnhandled();
}

SM_RESULT
XhciEndpointMachine::EnterResettingDefaultPipe()
{
    ResetDefaultPipe();
    return SmHandled();
}

SM_RESULT
XhciEndpointMachine::OnResettingDefaultPipe(
    _In_ XepEvent Event)
{
    if (Event == XepEvent::CommandSucceeded)
        return ReclaimFor(ReclaimAfterAutoReset);

    return SmUnhandled();
}

SM_RESULT
XhciEndpointMachine::OnClientResetCommand(
    _In_ XepEvent Event)
{
    if (Event != XepEvent::CommandFailed)
        return SmUnhandled();

    CompleteResetRequest();
    return SmTransition(&AwaitingRecovery);
}

SM_RESULT
XhciEndpointMachine::EnterResettingPipe()
{
    ResetEndpoint();
    return SmHandled();
}

SM_RESULT
XhciEndpointMachine::OnResettingPipe(
    _In_ XepEvent Event)
{
    if (Event == XepEvent::CommandSucceeded)
        return ReclaimFor(ReclaimAfterHaltClear);

    return SmUnhandled();
}

SM_RESULT
XhciEndpointMachine::EnterRebuildingContextForReset()
{
    ReconfigureForReset();
    return SmHandled();
}

SM_RESULT
XhciEndpointMachine::OnRebuildingContextForReset(
    _In_ XepEvent Event)
{
    if ((Event != XepEvent::CommandSucceeded) && (Event != XepEvent::BufferAllocFailed))
        return SmUnhandled();

    /* A buffer failure already set the request status, the request completes either way */
    CompleteResetRequest();
    return Remap();
}

SM_RESULT
XhciEndpointMachine::EnterReclaiming()
{
    NotifyRingsReclaim();
    if (ReclaimIgnoresCancel())
        SmDiscardQueued(XepEvent::TransferAborted);

    return SmHandled();
}

SM_RESULT
XhciEndpointMachine::OnReclaiming(
    _In_ XepEvent Event)
{
    if (Event == XepEvent::TransferAborted)
        return ReclaimIgnoresCancel() ? SmHandled() : SmUnhandled();

    if (Event != XepEvent::TransfersRecovered)
        return SmUnhandled();

    switch (m_AfterReclaim)
    {
        case ReclaimAfterHaltClear:
            RestartStreamQueues();
            CompleteResetRequest();
            return Remap();

        case ReclaimAfterControllerReset:
            AckControllerReset();
            return SmTransition(&AwaitingDisable);

        default:
            return Remap();
    }
}

SM_RESULT
XhciEndpointMachine::EnterAwaitingStopRetry()
{
    StartTimer(m_RetryDelay);
    return SmHandled();
}

SM_RESULT
XhciEndpointMachine::OnAwaitingStopRetry(
    _In_ XepEvent Event)
{
    switch (Event)
    {
        case XepEvent::TimerFired:
            if (!StopAttemptsExhausted())
                return SmTransition(&StoppingPipe);

            /* Controllers with this quirk report a context error for a stopped endpoint */
            if (IgnoresStopContextError())
                return AfterStopCommand();

            RequestControllerReset();
            return SmTransition(&AwaitingRecovery);

        case XepEvent::HaltReported:
            return LeaveTimerFor(TimerExitHalt);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
XhciEndpointMachine::EnterAwaitingHalt()
{
    StartTimer(XEP_STOP_TIMEOUT_MS);
    return SmHandled();
}

SM_RESULT
XhciEndpointMachine::OnAwaitingHalt(
    _In_ XepEvent Event)
{
    switch (Event)
    {
        case XepEvent::TimerFired:
            ReportTimeout(XepTimeoutHalt);
            return AfterHalt();

        case XepEvent::HaltReported:
            return LeaveTimerFor(TimerExitHalt);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
XhciEndpointMachine::EnterAwaitingStoppedEvent()
{
    ReportStoppedEvent();
    StartTimer(XEP_STOP_TIMEOUT_MS);
    return SmHandled();
}

SM_RESULT
XhciEndpointMachine::OnAwaitingStoppedEvent(
    _In_ XepEvent Event)
{
    switch (Event)
    {
        case XepEvent::TimerFired:
            ReportTimeout(XepTimeoutStoppedEvent);
            return AfterStoppedEvent();

        case XepEvent::StoppedEventSeen:
            return LeaveTimerFor(TimerExitStoppedEvent);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
XhciEndpointMachine::EnterDraining()
{
    NotifyRingsDrainEvents();
    StartTimer(XEP_STOP_TIMEOUT_MS);
    if (m_DrainAfterHalt)
        SmDiscardQueued(XepEvent::TransferAborted);

    return SmHandled();
}

SM_RESULT
XhciEndpointMachine::OnDraining(
    _In_ XepEvent Event)
{
    switch (Event)
    {
        case XepEvent::TimerFired:
            if (m_DrainAfterHalt)
            {
                ReportTimeout(XepTimeoutDrainAfterHalt);
                return AfterHaltDrained();
            }

            ReportTimeout(XepTimeoutDrainAfterStop);
            return AfterStopDrained();

        case XepEvent::ExpectedEventsDone:
            return LeaveTimerFor(m_DrainAfterHalt ? TimerExitHaltDrained : TimerExitStopDrained);

        case XepEvent::TransferAborted:
            return m_DrainAfterHalt ? SmHandled() : SmUnhandled();

        default:
            return SmUnhandled();
    }
}

/**
 * @brief
 * Runs the timer exit; the engine already stopped the timer, so it cannot fire after this.
 */
SM_RESULT
XhciEndpointMachine::EnterLeavingTimer()
{
    return RunTimerExit(m_AfterTimer);
}

SM_RESULT
XhciEndpointMachine::Remap()
{
    ResumeRingFill();
    return SmTransition(&Mapping);
}

SM_RESULT
XhciEndpointMachine::SuspendRingFillFor(
    _In_ MAPPING_STOP_REASON Reason)
{
    m_AfterMapping = Reason;
    return SmTransition(&HaltingRingFill);
}

SM_RESULT
XhciEndpointMachine::ReclaimFor(
    _In_ RECLAIM_REASON Reason)
{
    m_AfterReclaim = Reason;
    return SmTransition(&Reclaiming);
}

SM_RESULT
XhciEndpointMachine::RetryStopAfter(
    _In_ ULONG Milliseconds)
{
    m_RetryDelay = Milliseconds;
    return SmTransition(&AwaitingStopRetry);
}

SM_RESULT
XhciEndpointMachine::DrainFor(
    _In_ BOOLEAN AfterHalt)
{
    m_DrainAfterHalt = AfterHalt;
    return SmTransition(&Draining);
}

SM_RESULT
XhciEndpointMachine::LeaveTimerFor(
    _In_ TIMER_EXIT Exit)
{
    m_AfterTimer = Exit;
    return SmTransition(&LeavingTimer);
}

SM_RESULT
XhciEndpointMachine::RunTimerExit(
    _In_ TIMER_EXIT Exit)
{
    switch (Exit)
    {
        case TimerExitHalt:
            return AfterHalt();

        case TimerExitStoppedEvent:
            return AfterStoppedEvent();

        case TimerExitStopDrained:
            return AfterStopDrained();

        default:
            return AfterHaltDrained();
    }
}

SM_RESULT
XhciEndpointMachine::BeginStop()
{
    ResetStopAttempts();
    if (DelaysFirstStop())
        return RetryStopAfter(XEP_STOP_RETRY_LONG_MS);

    return SmTransition(&StoppingPipe);
}

/* Stop command done; wait for the force stopped event unless the controller cannot deliver it */
SM_RESULT
XhciEndpointMachine::AfterStopCommand()
{
    if (!OnPrimaryInterrupterOnly())
        return SmTransition(&AwaitingStoppedEvent);

    DropStoppedEvent();
    return AfterStopDrained();
}

SM_RESULT
XhciEndpointMachine::AfterStopDrained()
{
    if (NeedsContextRebuildAfterStop())
        return SmTransition(&RebuildingContextAfterStop);

    return SmTransition(&UpdatingDequeue);
}

SM_RESULT
XhciEndpointMachine::AfterHalt()
{
    NotifyRingsHalted();

    /* Control endpoints are reset here, others wait for the client */
    if (IsControlEndpoint())
        return SmTransition(&ResettingDefaultPipe);

    return DrainFor(TRUE);
}

SM_RESULT
XhciEndpointMachine::AfterStoppedEvent()
{
    NotifyRingsStoppedEvent();
    return DrainFor(FALSE);
}

SM_RESULT
XhciEndpointMachine::AfterHaltDrained()
{
    PurgeStreamQueues();
    NotifyRingsReclaimOnCancel();
    return SmTransition(&AwaitingClientReset);
}
