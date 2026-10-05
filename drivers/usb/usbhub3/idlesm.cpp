/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Idle notification state machine of a hub child device
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include "usbhub3.h"

/* GLOBALS ********************************************************************/

/* Posts are serialized by m_PostLock, so nothing is ever left queued */
const SM_EVENT_INFO IdleMachine::EventInfo[] =
{
    { "Start",              SmEventCompletion, FALSE },
    { "RequestSubmitted",   SmEventCompletion, FALSE },
    { "RequestCanceled",   SmEventCompletion, FALSE },
    { "WorkItemRan",        SmEventCompletion, FALSE },
    { "ClientCallbackFinished",   SmEventCompletion, FALSE },
    { "PoweredUp",          SmEventCompletion, FALSE },
    { "PoweringDown",       SmEventCompletion, FALSE },
    { "Cleanup",            SmEventCompletion, FALSE },
};

static_assert(RTL_NUMBER_OF(IdleMachine::EventInfo) == static_cast<ULONG>(IdleEvent::Count),
              "EventInfo must have one entry per IdleEvent");

const IdleMachine::State IdleMachine::Starting =
    { NULL, "Starting", 0, &IdleMachine::OnStarting, NULL };

/* Child device exists, power changes that need no work are dropped */
const IdleMachine::State IdleMachine::Present =
    { NULL, "Present", 0, &IdleMachine::OnPresent, NULL };

const IdleMachine::State IdleMachine::NoRequest =
    { &Present, "NoRequest", 0, &IdleMachine::OnNoRequest, NULL };

const IdleMachine::State IdleMachine::Ready =
    { &NoRequest, "Ready", 0, &IdleMachine::OnReady, NULL };

const IdleMachine::State IdleMachine::SuspendUnsupported =
    { &NoRequest, "SuspendUnsupported", 0, &IdleMachine::OnSuspendUnsupported, NULL };

/* Request held without a callback, selective suspend is unsupported */
const IdleMachine::State IdleMachine::Parked =
    { &Present, "Parked", 0, &IdleMachine::OnParked, NULL };

/* Request held on the selective suspend path */
const IdleMachine::State IdleMachine::Holding =
    { &Present, "Holding", 0, &IdleMachine::OnHolding, NULL };

/* Held request is cancelable and no callback is running */
const IdleMachine::State IdleMachine::Pending =
    { &Holding, "Pending", 0, &IdleMachine::OnPending, NULL };

const IdleMachine::State IdleMachine::AwaitingWorkItem =
    { &Pending, "AwaitingWorkItem", 0, &IdleMachine::OnAwaitingWorkItem, NULL };

const IdleMachine::State IdleMachine::CallbackDone =
    { &Pending, "CallbackDone", 0, NULL, NULL };

const IdleMachine::State IdleMachine::CallbackRunning =
    { &Holding, "CallbackRunning", 0, &IdleMachine::OnCallbackRunning, NULL };

/* Cancel routine owns the request, wait for it */
const IdleMachine::State IdleMachine::AwaitingCancel =
    { &Holding, "AwaitingCancel", 0, &IdleMachine::OnAwaitingCancel, NULL };

/* Child device cleaned up, new requests fail */
const IdleMachine::State IdleMachine::Removed =
    { NULL, "Removed", 0, &IdleMachine::OnRemoved, NULL };

const IdleMachine::State IdleMachine::Gone =
    { &Removed, "Gone", 0, NULL, NULL };

const IdleMachine::State IdleMachine::CallbackRemoved =
    { &Removed, "CallbackRemoved", 0, &IdleMachine::OnCallbackRemoved, NULL };

const IdleMachine::State IdleMachine::AwaitingCancelRemoved =
    { &Removed, "AwaitingCancelRemoved", 0, &IdleMachine::OnAwaitingCancelRemoved, NULL };

/* FUNCTIONS ******************************************************************/

VOID
IdleMachine::Initialize(
    _In_ HubPdo* Pdo)
{
    KeInitializeSpinLock(&m_PostLock);
    m_Pdo = Pdo;
    m_HeldIrp = NULL;
    m_NewIrp = NULL;
    m_FollowUpStatus = STATUS_PENDING;
    m_FollowUp = FollowUpNone;
    m_PowerChange = PowerUnchanged;
    m_ReturnPending = FALSE;
    m_SuspendSupported = FALSE;
    m_CancelSeen = FALSE;

    SmInitialize(&Starting);
}

/**
 * @brief
 * Irp is the new idle IRP for RequestSubmitted, else NULL.
 *
 * @return
 * STATUS_PENDING, or the status the IRP of this post was completed with.
 */
NTSTATUS
IdleMachine::Post(
    _In_ IdleEvent Event,
    _In_opt_ PIRP Irp)
{
    IDLE_FOLLOW_UP followUp;
    PIRP finished = NULL;
    PIRP callbackIrp = NULL;
    NTSTATUS status;
    BOOLEAN MustPend;
    KIRQL irql;

    KeAcquireSpinLock(&m_PostLock, &irql);

    m_NewIrp = Irp;
    m_FollowUp = FollowUpNone;
    m_FollowUpStatus = STATUS_PENDING;
    m_ReturnPending = FALSE;

    SmPost(Event);

    followUp = m_FollowUp;
    status = m_FollowUpStatus;
    MustPend = m_ReturnPending;

    if (followUp == FollowUpCompleteHeld)
    {
        finished = m_HeldIrp;
        m_HeldIrp = NULL;
    }
    else if (followUp == FollowUpRejectNew)
    {
        finished = Irp;
    }
    else if (followUp == FollowUpCallback)
    {
        callbackIrp = m_HeldIrp;
    }

    m_NewIrp = NULL;
    KeReleaseSpinLock(&m_PostLock, irql);

    if (followUp == FollowUpCallback)
    {
        InvokeIdleCallback(callbackIrp);
        Post(IdleEvent::ClientCallbackFinished, NULL);
    }
    else if (followUp == FollowUpWorkItem)
    {
        QueueIdleWorkItem();
    }

    if (finished != NULL)
        CompleteIdleRequest(finished, status);

    return MustPend ? STATUS_PENDING : status;
}

/* Complete the held request once the lock is dropped */
VOID
IdleMachine::FinishHeld(
    _In_ NTSTATUS Status)
{
    m_FollowUp = FollowUpCompleteHeld;
    m_FollowUpStatus = Status;
}

/* Complete the request of this post once the lock is dropped */
VOID
IdleMachine::RejectNew(
    _In_ NTSTATUS Status)
{
    m_FollowUp = FollowUpRejectNew;
    m_FollowUpStatus = Status;
}

VOID
IdleMachine::HoldNew()
{
    NT_ASSERT(m_HeldIrp == NULL);

    m_HeldIrp = m_NewIrp;
    MarkRequestPending(m_HeldIrp);
}

/* Take the cancel routine back and complete, or wait for the cancel */
SM_RESULT
IdleMachine::Release(
    _In_ NTSTATUS Status)
{
    if (!DisarmCancel(m_HeldIrp))
        return SmTransition(&AwaitingCancel);

    FinishHeld(Status);
    return SmTransition(&Ready);
}

SM_RESULT
IdleMachine::ReleaseForRemoval()
{
    if (!DisarmCancel(m_HeldIrp))
        return SmTransition(&AwaitingCancelRemoved);

    FinishHeld(STATUS_NO_SUCH_DEVICE);
    return SmTransition(&Gone);
}

/* The first power change or cancel seen during the callback picks the outcome */
SM_RESULT
IdleMachine::EndCallback()
{
    NTSTATUS status;

    if (m_PowerChange == PowerUp)
        status = STATUS_SUCCESS;
    else if (m_PowerChange == PowerDown)
        status = STATUS_POWER_STATE_INVALID;
    else
        status = STATUS_CANCELLED;

    if (m_CancelSeen)
    {
        FinishHeld(status);
        return SmTransition(&Ready);
    }

    if (m_PowerChange == PowerUnchanged)
        return SmTransition(&CallbackDone);

    return Release(status);
}

SM_RESULT
IdleMachine::OnStarting(
    _In_ IdleEvent Event)
{
    if (Event != IdleEvent::Start)
        return SmUnhandled();

    m_SuspendSupported = SelectiveSuspendSupported();
    return SmTransition(m_SuspendSupported ? &Ready : &SuspendUnsupported);
}

SM_RESULT
IdleMachine::OnPresent(
    _In_ IdleEvent Event)
{
    if (Event == IdleEvent::PoweredUp || Event == IdleEvent::PoweringDown)
        return SmHandled();

    return SmUnhandled();
}

SM_RESULT
IdleMachine::OnNoRequest(
    _In_ IdleEvent Event)
{
    if (Event == IdleEvent::Cleanup)
        return SmTransition(&Gone);

    return SmUnhandled();
}

SM_RESULT
IdleMachine::OnReady(
    _In_ IdleEvent Event)
{
    switch (Event)
    {
        case IdleEvent::RequestSubmitted:
            HoldNew();
            if (ArmCancel(m_HeldIrp))
            {
                m_FollowUp = FollowUpWorkItem;
                return SmTransition(&AwaitingWorkItem);
            }

            /* Already canceled; the IRP was marked pending, so return pending */
            FinishHeld(STATUS_CANCELLED);
            m_ReturnPending = TRUE;
            return SmHandled();

        case IdleEvent::RequestCanceled:
        case IdleEvent::WorkItemRan:
            return SmHandled();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
IdleMachine::OnSuspendUnsupported(
    _In_ IdleEvent Event)
{
    if (Event != IdleEvent::RequestSubmitted)
        return SmUnhandled();

    HoldNew();
    if (ArmCancel(m_HeldIrp))
        return SmTransition(&Parked);

    FinishHeld(STATUS_CANCELLED);
    return SmHandled();
}

SM_RESULT
IdleMachine::OnParked(
    _In_ IdleEvent Event)
{
    switch (Event)
    {
        case IdleEvent::RequestSubmitted:
            RejectNew(STATUS_DEVICE_BUSY);
            return SmHandled();

        case IdleEvent::RequestCanceled:
            FinishHeld(STATUS_CANCELLED);
            return SmTransition(&SuspendUnsupported);

        case IdleEvent::Cleanup:
            return ReleaseForRemoval();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
IdleMachine::OnHolding(
    _In_ IdleEvent Event)
{
    switch (Event)
    {
        case IdleEvent::RequestSubmitted:
            RejectNew(STATUS_DEVICE_BUSY);
            return SmHandled();

        case IdleEvent::WorkItemRan:
            return SmHandled();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
IdleMachine::OnPending(
    _In_ IdleEvent Event)
{
    switch (Event)
    {
        case IdleEvent::RequestCanceled:
            FinishHeld(STATUS_CANCELLED);
            return SmTransition(&Ready);

        case IdleEvent::PoweredUp:
            return Release(STATUS_SUCCESS);

        case IdleEvent::PoweringDown:
            return Release(STATUS_POWER_STATE_INVALID);

        case IdleEvent::Cleanup:
            return ReleaseForRemoval();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
IdleMachine::OnAwaitingWorkItem(
    _In_ IdleEvent Event)
{
    if (Event != IdleEvent::WorkItemRan)
        return SmUnhandled();

    m_PowerChange = PowerUnchanged;
    m_CancelSeen = FALSE;
    m_FollowUp = FollowUpCallback;
    return SmTransition(&CallbackRunning);
}

SM_RESULT
IdleMachine::OnCallbackRunning(
    _In_ IdleEvent Event)
{
    switch (Event)
    {
        case IdleEvent::ClientCallbackFinished:
            return EndCallback();

        case IdleEvent::PoweredUp:
        case IdleEvent::PoweringDown:
            if (m_PowerChange == PowerUnchanged && !m_CancelSeen)
                m_PowerChange = (Event == IdleEvent::PoweredUp) ? PowerUp : PowerDown;
            return SmHandled();

        case IdleEvent::RequestCanceled:
            /* A second cancel is not expected */
            if (m_CancelSeen)
                return SmUnhandled();

            m_CancelSeen = TRUE;
            return SmHandled();

        case IdleEvent::Cleanup:
            return SmTransition(&CallbackRemoved);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
IdleMachine::OnAwaitingCancel(
    _In_ IdleEvent Event)
{
    switch (Event)
    {
        case IdleEvent::RequestCanceled:
            FinishHeld(STATUS_CANCELLED);
            return SmTransition(&Ready);

        case IdleEvent::Cleanup:
            return SmTransition(&AwaitingCancelRemoved);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
IdleMachine::OnRemoved(
    _In_ IdleEvent Event)
{
    switch (Event)
    {
        case IdleEvent::RequestSubmitted:
            RejectNew(STATUS_NO_SUCH_DEVICE);
            return SmHandled();

        case IdleEvent::RequestCanceled:
        case IdleEvent::WorkItemRan:
            return SmHandled();

        default:
            return SmUnhandled();
    }
}

/* The held request is left as is, the same as the previous machine */
SM_RESULT
IdleMachine::OnCallbackRemoved(
    _In_ IdleEvent Event)
{
    if (Event == IdleEvent::ClientCallbackFinished)
        return SmTransition(&Gone);

    return SmUnhandled();
}

SM_RESULT
IdleMachine::OnAwaitingCancelRemoved(
    _In_ IdleEvent Event)
{
    if (Event != IdleEvent::RequestCanceled)
        return SmUnhandled();

    FinishHeld(STATUS_NO_SUCH_DEVICE);
    return SmTransition(&Gone);
}
