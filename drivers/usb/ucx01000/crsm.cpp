/*
 * PROJECT:     ReactOS USB Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Controller reset state machine
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include "ucx01000.h"

/* GLOBALS ********************************************************************/

const SM_EVENT_INFO ControllerResetMachine::EventInfo[] =
{
    { "ControllerLost",       SmEventRequest,    FALSE },
    { "HubRequestsReset",       SmEventRequest,    FALSE },
    { "RootHubPoweredUp",       SmEventRequest,    FALSE },
    { "RootHubPoweringDown",    SmEventRequest,    FALSE },
    { "ControllerNeedsReset",   SmEventCompletion, TRUE },
    { "ControllerResetDone",    SmEventCompletion, TRUE },
    { "ResetBlocksCleared", SmEventCompletion, FALSE },
    { "DevicesReady",           SmEventCompletion, FALSE },
};

static_assert(RTL_NUMBER_OF(ControllerResetMachine::EventInfo) == static_cast<ULONG>(CrEvent::Count),
              "EventInfo must have one entry per CrEvent");

/* Root hub PDO in D0, controller running */
const ControllerResetMachine::State ControllerResetMachine::RootHubOn =
    { NULL, "RootHubOn", SM_STATE_TAKES_REQUESTS, &ControllerResetMachine::OnRootHubOn, NULL };

/* Root hub PDO out of D0. Initial state */
const ControllerResetMachine::State ControllerResetMachine::RootHubOff =
    { NULL, "RootHubOff", SM_STATE_TAKES_REQUESTS, &ControllerResetMachine::OnRootHubOff, NULL };

/* The controller reset itself while the root hub was off, devices catch up */
const ControllerResetMachine::State ControllerResetMachine::PreparingDevicesOff =
    { NULL, "PreparingDevicesOff", 0,
      &ControllerResetMachine::OnPreparingDevicesOff, &ControllerResetMachine::EnterPreparingDevices };

/* A reset UCX drives, the reset in progress flag is set throughout */
const ControllerResetMachine::State ControllerResetMachine::Resetting =
    { NULL, "Resetting", 0, &ControllerResetMachine::OnResetting, NULL };

const ControllerResetMachine::State ControllerResetMachine::DrainingReferences =
    { &Resetting, "DrainingReferences", 0,
      &ControllerResetMachine::OnDrainingReferences, &ControllerResetMachine::EnterDrainingReferences };

const ControllerResetMachine::State ControllerResetMachine::PreparingDevices =
    { &Resetting, "PreparingDevices", 0,
      &ControllerResetMachine::OnPreparingDevices, &ControllerResetMachine::EnterPreparingDevices };

const ControllerResetMachine::State ControllerResetMachine::ResettingController =
    { &Resetting, "ResettingController", SM_STATE_NEEDS_PASSIVE,
      &ControllerResetMachine::OnResettingController, &ControllerResetMachine::EnterResettingController };

/* Another reset was asked for while one was running, call the driver again when it ends */
const ControllerResetMachine::State ControllerResetMachine::AwaitingResetDone =
    { &Resetting, "AwaitingResetDone", 0, &ControllerResetMachine::OnAwaitingResetDone, NULL };

/* Reset done on the controller's own request, the hub has to reset the root hub ports */
const ControllerResetMachine::State ControllerResetMachine::AwaitingHubReset =
    { &Resetting, "AwaitingHubReset", SM_STATE_TAKES_REQUESTS,
      &ControllerResetMachine::OnAwaitingHubReset, NULL };

/* The controller driver reported the hardware dead, nothing leaves this group */
const ControllerResetMachine::State ControllerResetMachine::Failed =
    { NULL, "Failed", SM_STATE_TAKES_REQUESTS, &ControllerResetMachine::OnFailed, NULL };

const ControllerResetMachine::State ControllerResetMachine::FailedOn =
    { &Failed, "FailedOn", 0, &ControllerResetMachine::OnFailedOn, NULL };

const ControllerResetMachine::State ControllerResetMachine::FailedOff =
    { &Failed, "FailedOff", 0, &ControllerResetMachine::OnFailedOff, NULL };

/* FUNCTIONS ******************************************************************/

VOID
ControllerResetMachine::Initialize(
    _In_ UcxController* Controller)
{
    m_Controller = Controller;
    m_ForHub = FALSE;
    m_ResetPending = FALSE;
    m_ResetSeen = FALSE;

    SmInitialize(&RootHubOff);
}

VOID
ControllerResetMachine::SmReference()
{
    ReferenceController();
}

VOID
ControllerResetMachine::SmDereference()
{
    DereferenceController();
}

VOID
ControllerResetMachine::SmQueuePassive()
{
    QueuePassiveWork();
}

SM_RESULT
ControllerResetMachine::OnRootHubOn(
    _In_ CrEvent Event)
{
    switch (Event)
    {
        case CrEvent::ControllerLost:
            FailRootHubIo();
            return SmTransition(&FailedOn);

        case CrEvent::ControllerNeedsReset:
            return StartReset(FALSE);

        case CrEvent::HubRequestsReset:
            return StartReset(TRUE);

        case CrEvent::RootHubPoweringDown:
            return PowerDown(FALSE);

        default:
            return SmUnhandled();
    }
}

/**
 * @brief
 * m_ResetSeen: a reset done while off was caught up. m_ResetPending: a reset is owed.
 */
SM_RESULT
ControllerResetMachine::OnRootHubOff(
    _In_ CrEvent Event)
{
    switch (Event)
    {
        case CrEvent::ControllerResetDone:
            m_ResetPending = FALSE;
            if (!m_ResetSeen)
                return SmTransition(&PreparingDevicesOff);

            ReleaseResetCompleteWaiter();
            return SmHandled();

        case CrEvent::ControllerNeedsReset:
            SignalPortChange();
            m_ResetPending = TRUE;
            return SmHandled();

        case CrEvent::RootHubPoweredUp:
            LetResetCompletionProceed();
            if (m_ResetPending)
                return StartReset(FALSE);

            return SmTransition(&RootHubOn);

        case CrEvent::ControllerLost:
            FailRootHubIo();
            return SmTransition(&FailedOff);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
ControllerResetMachine::OnPreparingDevicesOff(
    _In_ CrEvent Event)
{
    if (Event != CrEvent::DevicesReady)
        return SmUnhandled();

    NotifyDevicesResetDone();
    ReleaseResetCompleteWaiter();
    m_ResetSeen = TRUE;
    return SmTransition(&RootHubOff);
}

SM_RESULT
ControllerResetMachine::OnResetting(
    _In_ CrEvent Event)
{
    /* A reset is already under way */
    if (Event == CrEvent::ControllerNeedsReset)
        return SmHandled();

    return SmUnhandled();
}

SM_RESULT
ControllerResetMachine::EnterDrainingReferences()
{
    HoldDeviceRequests();
    MarkResetInProgress();
    FailRootHubIo();
    DropResetBlock();
    return SmHandled();
}

SM_RESULT
ControllerResetMachine::OnDrainingReferences(
    _In_ CrEvent Event)
{
    if (Event == CrEvent::ResetBlocksCleared)
        return SmTransition(&PreparingDevices);

    return SmUnhandled();
}

SM_RESULT
ControllerResetMachine::EnterPreparingDevices()
{
    PrepareDevicesForReset();
    return SmHandled();
}

SM_RESULT
ControllerResetMachine::OnPreparingDevices(
    _In_ CrEvent Event)
{
    if (Event == CrEvent::DevicesReady)
        return SmTransition(&ResettingController);

    return SmUnhandled();
}

SM_RESULT
ControllerResetMachine::EnterResettingController()
{
    ResetController();
    return SmHandled();
}

SM_RESULT
ControllerResetMachine::OnResettingController(
    _In_ CrEvent Event)
{
    switch (Event)
    {
        case CrEvent::ControllerResetDone:
            NotifyDevicesResetDone();
            ReleaseDeviceRequests();
            if (m_ForHub)
                return EndResetForHub();

            return SmTransition(&AwaitingHubReset);

        case CrEvent::ControllerNeedsReset:
            return SmTransition(&AwaitingResetDone);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
ControllerResetMachine::OnAwaitingResetDone(
    _In_ CrEvent Event)
{
    if (Event == CrEvent::ControllerResetDone)
        return SmTransition(&ResettingController);

    return SmUnhandled();
}

SM_RESULT
ControllerResetMachine::OnAwaitingHubReset(
    _In_ CrEvent Event)
{
    switch (Event)
    {
        case CrEvent::HubRequestsReset:
            return EndResetForHub();

        case CrEvent::ControllerNeedsReset:
            HoldDeviceRequests();
            return SmTransition(&PreparingDevices);

        case CrEvent::RootHubPoweringDown:
            UnblockRootHubTraffic();
            MarkResetFinished();
            return PowerDown(TRUE);

        case CrEvent::ControllerLost:
            MarkResetFinished();
            FailRootHubIo();
            return SmTransition(&FailedOn);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
ControllerResetMachine::OnFailed(
    _In_ CrEvent Event)
{
    if (Event == CrEvent::ControllerNeedsReset)
        return SmHandled();

    return SmUnhandled();
}

SM_RESULT
ControllerResetMachine::OnFailedOn(
    _In_ CrEvent Event)
{
    switch (Event)
    {
        case CrEvent::HubRequestsReset:
            CompleteHubReset(FALSE);
            return SmHandled();

        case CrEvent::RootHubPoweringDown:
            AllowRootHubPowerDown();
            return SmTransition(&FailedOff);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
ControllerResetMachine::OnFailedOff(
    _In_ CrEvent Event)
{
    switch (Event)
    {
        case CrEvent::ControllerResetDone:
            ReleaseResetCompleteWaiter();
            return SmHandled();

        case CrEvent::RootHubPoweredUp:
            LetResetCompletionProceed();
            return SmTransition(&FailedOn);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
ControllerResetMachine::StartReset(
    _In_ BOOLEAN ForHub)
{
    m_ForHub = ForHub;
    return SmTransition(&DrainingReferences);
}

SM_RESULT
ControllerResetMachine::EndResetForHub()
{
    UnblockRootHubTraffic();
    MarkResetFinished();
    CompleteHubReset(TRUE);
    return SmTransition(&RootHubOn);
}

SM_RESULT
ControllerResetMachine::PowerDown(
    _In_ BOOLEAN ResetSeen)
{
    AllowRootHubPowerDown();
    m_ResetPending = FALSE;
    m_ResetSeen = ResetSeen;
    return SmTransition(&RootHubOff);
}
