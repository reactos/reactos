/*
 * PROJECT:     ReactOS USB Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Endpoint state machine
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include "ucx01000.h"

/* GLOBALS ********************************************************************/

const SM_EVENT_INFO EndpointMachine::EventInfo[] =
{
    { "HubStartRequest",               SmEventRequest,    FALSE },
    { "HubPurge",               SmEventRequest,    FALSE },
    { "HubTreePurge",           SmEventRequest,    FALSE },
    { "HubAbort",               SmEventRequest,    FALSE },
    { "HubDisconnect",          SmEventRequest,    FALSE },
    { "HubDisable",      SmEventRequest,    FALSE },
    { "HubDeviceReset",      SmEventRequest,    FALSE },
    { "DeviceResetDone",     SmEventRequest,    FALSE },
    { "ConfigureDone",             SmEventRequest,    FALSE },
    { "HubEndpointReset",       SmEventRequest,    FALSE },
    { "Delete",                     SmEventRequest,    FALSE },
    { "ClientAbortUrb",         SmEventRequest,    FALSE },
    { "ClientStreamsEnable",    SmEventRequest,    FALSE },
    { "ClientStreamsDisable",   SmEventRequest,    FALSE },
    { "AbortUrbParked",             SmEventCompletion, FALSE },
    { "AbortDone",              SmEventCompletion, FALSE },
    { "PurgeDone",              SmEventCompletion, FALSE },
    { "EndpointResetDone",      SmEventCompletion, FALSE },
    { "StreamsOpened",      SmEventCompletion, FALSE },
    { "StreamsClosed",     SmEventCompletion, FALSE },
    { "ControllerResetStarting",  SmEventCompletion, FALSE },
    { "StaleReplaced",                SmEventCompletion, FALSE },
    { "ControllerResetDone",    SmEventResume,     FALSE },
};

static_assert(RTL_NUMBER_OF(EndpointMachine::EventInfo) == static_cast<ULONG>(EpEvent::Count),
              "EventInfo must have one entry per EpEvent");

/* Endpoint is not running on the controller, hub requests are acked as is */
const EndpointMachine::State EndpointMachine::Inactive =
    { NULL, "Inactive", SM_STATE_TAKES_REQUESTS, &EndpointMachine::OnInactive, NULL };

const EndpointMachine::State EndpointMachine::Created =
    { &Inactive, "Created", 0, &EndpointMachine::OnCreated, NULL };

const EndpointMachine::State EndpointMachine::Disabled =
    { &Inactive, "Disabled", 0, &EndpointMachine::OnDisabled, NULL };

/* Off the controller's endpoint list, so a controller reset never resumes it */
const EndpointMachine::State EndpointMachine::Retired =
    { &Inactive, "Retired", 0, &EndpointMachine::OnRetired, NULL };

const EndpointMachine::State EndpointMachine::Stale =
    { &Retired, "Stale", 0, &EndpointMachine::OnStale, NULL };

const EndpointMachine::State EndpointMachine::AwaitingDestroy =
    { &Retired, "AwaitingDestroy", 0, NULL, NULL };

const EndpointMachine::State EndpointMachine::Enabled =
    { NULL, "Enabled", SM_STATE_TAKES_REQUESTS, &EndpointMachine::OnEnabled, NULL };

/* Waiting on a completion, a controller reset pauses the machine in place */
const EndpointMachine::State EndpointMachine::Busy =
    { NULL, "Busy", 0, &EndpointMachine::OnBusy, NULL };

const EndpointMachine::State EndpointMachine::Purging =
    { &Busy, "Purging", 0, &EndpointMachine::OnPurging, &EndpointMachine::EnterPurging };

/* One request handed to the client driver, returns to m_ReturnTo when done */
const EndpointMachine::State EndpointMachine::Operation =
    { &Busy, "Operation", 0, &EndpointMachine::OnOperation, NULL };

const EndpointMachine::State EndpointMachine::AwaitingAbortUrb =
    { &Operation, "AwaitingAbortUrb", 0, &EndpointMachine::OnAwaitingAbortUrb, NULL };

const EndpointMachine::State EndpointMachine::Aborting =
    { &Operation, "Aborting", 0, &EndpointMachine::OnAborting, &EndpointMachine::EnterAborting };

const EndpointMachine::State EndpointMachine::EnablingStreams =
    { &Operation, "EnablingStreams", 0,
      &EndpointMachine::OnEnablingStreams, &EndpointMachine::EnterEnablingStreams };

const EndpointMachine::State EndpointMachine::DisablingStreams =
    { &Operation, "DisablingStreams", 0,
      &EndpointMachine::OnDisablingStreams, &EndpointMachine::EnterDisablingStreams };

const EndpointMachine::State EndpointMachine::ResettingPipe =
    { &Operation, "ResettingPipe", 0,
      &EndpointMachine::OnResettingPipe, &EndpointMachine::EnterResettingPipe };

/* Controller reset hit an operation started from Enabled */
const EndpointMachine::State EndpointMachine::OperationHeld =
    { NULL, "OperationHeld", 0,
      &EndpointMachine::OnOperationHeld, &EndpointMachine::EnterOperationHeld };

const EndpointMachine::State EndpointMachine::OperationUnwinding =
    { NULL, "OperationUnwinding", 0, &EndpointMachine::OnOperationUnwinding, NULL };

/* FUNCTIONS ******************************************************************/

VOID
EndpointMachine::Initialize(
    _In_ UcxEndpoint* Endpoint)
{
    m_Endpoint = Endpoint;
    m_ReturnTo = NULL;
    m_HeldOperation = NULL;
    m_AfterPurge = PurgeFollowUpNone;
    m_Enabled = FALSE;
    m_AbortForHub = FALSE;
    m_PauseAfterPurge = FALSE;

    SmInitialize(&Created);
}

/**
 * @brief
 * Abort URB is only taken while running; the caller fails the URB on FALSE.
 */
BOOLEAN
EndpointMachine::SmAccepts(
    _In_ EpEvent Event)
{
    if (Event == EpEvent::ClientAbortUrb)
        return SmCurrent() == &Enabled;

    return TRUE;
}

VOID
EndpointMachine::SmReference()
{
    ReferenceEndpoint();
}

VOID
EndpointMachine::SmDereference()
{
    DereferenceEndpoint();
}

SM_RESULT
EndpointMachine::OnInactive(
    _In_ EpEvent Event)
{
    switch (Event)
    {
        case EpEvent::HubStartRequest:
        case EpEvent::HubPurge:
        case EpEvent::HubAbort:
        case EpEvent::HubDeviceReset:
            CompleteHubOperation();
            return SmHandled();

        case EpEvent::HubTreePurge:
            CompleteTreePurge();
            return SmHandled();

        case EpEvent::HubDisconnect:
            return SmHandled();

        case EpEvent::ControllerResetStarting:
            return PauseForControllerReset();

        case EpEvent::ControllerResetDone:
            return SmHandled();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
EndpointMachine::OnCreated(
    _In_ EpEvent Event)
{
    switch (Event)
    {
        case EpEvent::Delete:
            return Retire();

        case EpEvent::ConfigureDone:
            m_Enabled = TRUE;
            if (!CanStart())
                return StartPurge(PurgeFollowUpHubOperation);

            CompleteHubOperation();
            return SmTransition(&Enabled);

        case EpEvent::DeviceResetDone:
            CompleteHubOperation();
            return SmHandled();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
EndpointMachine::OnDisabled(
    _In_ EpEvent Event)
{
    switch (Event)
    {
        case EpEvent::Delete:
            return Retire();

        case EpEvent::HubDisable:
        case EpEvent::HubDeviceReset:
            m_Enabled = FALSE;
            CompleteHubOperation();
            return SmHandled();

        case EpEvent::ConfigureDone:
        case EpEvent::DeviceResetDone:
            m_Enabled = TRUE;
            return TryStart();

        case EpEvent::HubStartRequest:
            return TryStart();

        case EpEvent::ClientStreamsEnable:
            return BeginOperation(&EnablingStreams, &Disabled);

        case EpEvent::ClientStreamsDisable:
            return BeginOperation(&DisablingStreams, &Disabled);

        case EpEvent::HubEndpointReset:
            return BeginOperation(&ResettingPipe, &Disabled);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
EndpointMachine::OnRetired(
    _In_ EpEvent Event)
{
    if (Event == EpEvent::ControllerResetStarting)
    {
        AckControllerReset();
        return SmHandled();
    }

    return SmUnhandled();
}

SM_RESULT
EndpointMachine::OnStale(
    _In_ EpEvent Event)
{
    if (Event == EpEvent::StaleReplaced)
    {
        DeleteStaleEndpoint();
        return SmTransition(&AwaitingDestroy);
    }

    return SmUnhandled();
}

SM_RESULT
EndpointMachine::OnEnabled(
    _In_ EpEvent Event)
{
    switch (Event)
    {
        case EpEvent::ClientStreamsEnable:
            return BeginOperation(&EnablingStreams, &Enabled);

        case EpEvent::ClientStreamsDisable:
            return BeginOperation(&DisablingStreams, &Enabled);

        case EpEvent::HubEndpointReset:
            return BeginOperation(&ResettingPipe, &Enabled);

        case EpEvent::ClientAbortUrb:
            m_AbortForHub = FALSE;
            return BeginOperation(&AwaitingAbortUrb, &Enabled);

        case EpEvent::HubAbort:
            m_AbortForHub = TRUE;
            return BeginOperation(&Aborting, &Enabled);

        case EpEvent::HubDisable:
        case EpEvent::HubDeviceReset:
            m_Enabled = FALSE;
            return StartPurge(PurgeFollowUpHubOperation);

        case EpEvent::HubPurge:
            return StartPurge(PurgeFollowUpHubOperation);

        case EpEvent::HubTreePurge:
            return StartPurge(PurgeFollowUpTreePurge);

        case EpEvent::HubDisconnect:
            return StartPurge(PurgeFollowUpNone);

        case EpEvent::ControllerResetStarting:
            m_PauseAfterPurge = TRUE;
            return StartPurge(PurgeFollowUpNone);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
EndpointMachine::OnBusy(
    _In_ EpEvent Event)
{
    switch (Event)
    {
        case EpEvent::ControllerResetStarting:
            return PauseForControllerReset();

        case EpEvent::ControllerResetDone:
            return SmHandled();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
EndpointMachine::EnterPurging()
{
    PurgeEndpoint();

    if (m_PauseAfterPurge)
    {
        m_PauseAfterPurge = FALSE;
        return PauseForControllerReset();
    }

    return SmHandled();
}

SM_RESULT
EndpointMachine::OnPurging(
    _In_ EpEvent Event)
{
    if (Event != EpEvent::PurgeDone)
        return SmUnhandled();

    if (m_AfterPurge == PurgeFollowUpHubOperation)
        CompleteHubOperation();
    else if (m_AfterPurge == PurgeFollowUpTreePurge)
        CompleteTreePurge();

    return SmTransition(&Disabled);
}

SM_RESULT
EndpointMachine::OnOperation(
    _In_ EpEvent Event)
{
    if (Event != EpEvent::ControllerResetStarting)
        return SmUnhandled();

    /* Endpoint is already purged, Busy pauses in place */
    if (m_ReturnTo == &Disabled)
        return SmUnhandled();

    m_HeldOperation = SmCurrent();
    return SmTransition(&OperationHeld);
}

SM_RESULT
EndpointMachine::OnAwaitingAbortUrb(
    _In_ EpEvent Event)
{
    if (Event == EpEvent::AbortUrbParked)
        return SmTransition(&Aborting);

    return SmUnhandled();
}

SM_RESULT
EndpointMachine::EnterAborting()
{
    AbortEndpoint();
    return SmHandled();
}

SM_RESULT
EndpointMachine::OnAborting(
    _In_ EpEvent Event)
{
    if (Event != EpEvent::AbortDone)
        return SmUnhandled();

    if (m_AbortForHub)
    {
        StartEndpoint();
        CompleteHubOperation();
    }
    else
    {
        CompleteAbortUrb();
        StartEndpoint();
    }

    return SmTransition(&Enabled);
}

SM_RESULT
EndpointMachine::EnterEnablingStreams()
{
    if (m_ReturnTo == &Enabled)
        ForwardStreamsEnableRequest();
    else
        RejectStreamsEnableRequest();

    return SmHandled();
}

SM_RESULT
EndpointMachine::OnEnablingStreams(
    _In_ EpEvent Event)
{
    if (Event != EpEvent::StreamsOpened)
        return SmUnhandled();

    FinishStreamsOpenRequest(FALSE);
    return SmTransition(m_ReturnTo);
}

SM_RESULT
EndpointMachine::EnterDisablingStreams()
{
    /* A deprogrammed device has no streams on the controller to tear down */
    if ((m_ReturnTo == &Disabled) && DeviceDeprogrammed())
        ParkStreamsDisableRequest();
    else
        ForwardStreamsDisableRequest();

    return SmHandled();
}

SM_RESULT
EndpointMachine::OnDisablingStreams(
    _In_ EpEvent Event)
{
    if (Event != EpEvent::StreamsClosed)
        return SmUnhandled();

    FinishStreamsCloseRequest();
    return SmTransition(m_ReturnTo);
}

SM_RESULT
EndpointMachine::EnterResettingPipe()
{
    if ((m_ReturnTo == &Disabled) && (DeviceDeprogrammed() || DeviceDisconnected()))
        FailResetRequest();
    else
        ForwardResetRequest();

    return SmHandled();
}

SM_RESULT
EndpointMachine::OnResettingPipe(
    _In_ EpEvent Event)
{
    if (Event != EpEvent::EndpointResetDone)
        return SmUnhandled();

    FinishEndpointResetRequest();
    return SmTransition(m_ReturnTo);
}

SM_RESULT
EndpointMachine::EnterOperationHeld()
{
    return PauseForControllerReset();
}

SM_RESULT
EndpointMachine::OnOperationHeld(
    _In_ EpEvent Event)
{
    if (Event == EpEvent::ControllerResetDone)
        return SmTransition(&OperationUnwinding);

    return SmUnhandled();
}

/**
 * @brief
 * Reset under an outstanding operation: finish it, then purge.
 */
SM_RESULT
EndpointMachine::OnOperationUnwinding(
    _In_ EpEvent Event)
{
    if (Event == EpEvent::ControllerResetStarting)
        return SmTransition(&OperationHeld);

    /* A hub abort only ever has the abort itself outstanding */
    if ((m_HeldOperation == &Aborting) && m_AbortForHub)
    {
        if (Event != EpEvent::AbortDone)
            return SmUnhandled();

        CompleteHubOperation();
        return StartPurge(PurgeFollowUpNone);
    }

    switch (Event)
    {
        case EpEvent::AbortUrbParked:
        case EpEvent::AbortDone:
            CompleteAbortUrb();
            break;

        case EpEvent::StreamsOpened:
            FinishStreamsOpenRequest(TRUE);
            break;

        case EpEvent::StreamsClosed:
            FinishStreamsCloseRequest();
            break;

        case EpEvent::EndpointResetDone:
            FinishEndpointResetRequest();
            break;

        default:
            return SmUnhandled();
    }

    return StartPurge(PurgeFollowUpNone);
}

/**
 * @brief
 * Pause before the ack; nothing may touch the machine after it.
 */
SM_RESULT
EndpointMachine::PauseForControllerReset()
{
    SmBeginPause();
    AckControllerReset();
    return SmPaused();
}

SM_RESULT
EndpointMachine::StartPurge(
    _In_ PURGE_FOLLOW_UP FollowUp)
{
    m_AfterPurge = FollowUp;
    return SmTransition(&Purging);
}

SM_RESULT
EndpointMachine::BeginOperation(
    _In_ const State* Target,
    _In_ const State* ReturnTo)
{
    m_ReturnTo = ReturnTo;
    return SmTransition(Target);
}

SM_RESULT
EndpointMachine::TryStart()
{
    if (!CanStart())
    {
        CompleteHubOperation();
        return SmHandled();
    }

    StartEndpoint();
    CompleteHubOperation();
    return SmTransition(&Enabled);
}

/**
 * @brief
 * An endpoint the client still has a handle to stays around as stale.
 */
SM_RESULT
EndpointMachine::Retire()
{
    if (!ClientHoldsHandle())
    {
        DeleteEndpoint();
        return SmTransition(&AwaitingDestroy);
    }

    if (ParkAsStale())
        return SmTransition(&Stale);

    DeleteStaleEndpoint();
    return SmTransition(&AwaitingDestroy);
}

BOOLEAN
EndpointMachine::CanStart()
{
    return m_Enabled && DeviceAllowsStart();
}
