/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Idle notification state machine of a hub child device
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include <drivers/usb3/smengine.h>

class HubPdo;

enum class IdleEvent : UCHAR
{
    /* Child device created, the machine reads the selective suspend policy */
    Start,

    /* Idle notification IOCTL from the client driver, carries the IRP */
    RequestSubmitted,

    /* Cancel routine of the held IRP ran */
    RequestCanceled,

    /* Work item queued for the held IRP ran at passive level */
    WorkItemRan,

    /* Client idle callback returned */
    ClientCallbackFinished,

    /* Child device power changes and removal */
    PoweredUp,
    PoweringDown,
    Cleanup,

    Count
};

/**
 * @brief
 * Posts run to completion under m_PostLock; IRP completion and callbacks run after it is dropped.
 */
class IdleMachine : public SmMachine<IdleMachine, IdleEvent>
{
    friend class SmMachine<IdleMachine, IdleEvent>;

public:
    VOID
    Initialize(
        _In_ HubPdo* Pdo);

    NTSTATUS
    Post(
        _In_ IdleEvent Event,
        _In_opt_ PIRP Irp);

    HubPdo*
    Pdo() const
    {
        return m_Pdo;
    }

    /* The held idle IRP, NULL when there is none */
    PIRP
    HeldIrp() const
    {
        return m_HeldIrp;
    }

    static const SM_EVENT_INFO EventInfo[];

private:
    enum IDLE_FOLLOW_UP
    {
        FollowUpNone,
        FollowUpCompleteHeld,
        FollowUpRejectNew,
        FollowUpCallback,
        FollowUpWorkItem
    };

    enum IDLE_POWER_CHANGE
    {
        PowerUnchanged,
        PowerUp,
        PowerDown
    };

    /* States */
    static const State Starting;
    static const State Present;
    static const State NoRequest;
    static const State Ready;
    static const State SuspendUnsupported;
    static const State Parked;
    static const State Holding;
    static const State Pending;
    static const State AwaitingWorkItem;
    static const State CallbackDone;
    static const State CallbackRunning;
    static const State AwaitingCancel;
    static const State Removed;
    static const State Gone;
    static const State CallbackRemoved;
    static const State AwaitingCancelRemoved;

    /* Handlers */
    SM_RESULT
    OnStarting(
        _In_ IdleEvent Event);
    SM_RESULT
    OnPresent(
        _In_ IdleEvent Event);
    SM_RESULT
    OnNoRequest(
        _In_ IdleEvent Event);
    SM_RESULT
    OnReady(
        _In_ IdleEvent Event);
    SM_RESULT
    OnSuspendUnsupported(
        _In_ IdleEvent Event);
    SM_RESULT
    OnParked(
        _In_ IdleEvent Event);
    SM_RESULT
    OnHolding(
        _In_ IdleEvent Event);
    SM_RESULT
    OnPending(
        _In_ IdleEvent Event);
    SM_RESULT
    OnAwaitingWorkItem(
        _In_ IdleEvent Event);
    SM_RESULT
    OnCallbackRunning(
        _In_ IdleEvent Event);
    SM_RESULT
    OnAwaitingCancel(
        _In_ IdleEvent Event);
    SM_RESULT
    OnRemoved(
        _In_ IdleEvent Event);
    SM_RESULT
    OnCallbackRemoved(
        _In_ IdleEvent Event);
    SM_RESULT
    OnAwaitingCancelRemoved(
        _In_ IdleEvent Event);

    /* Helpers */
    VOID
    FinishHeld(
        _In_ NTSTATUS Status);

    VOID
    RejectNew(
        _In_ NTSTATUS Status);

    VOID
    HoldNew();

    SM_RESULT
    Release(
        _In_ NTSTATUS Status);

    SM_RESULT
    ReleaseForRemoval();

    SM_RESULT
    EndCallback();

    /* Actions and queries, implemented by the child device object */
    BOOLEAN SelectiveSuspendSupported();
    VOID
    MarkRequestPending(
        _In_ PIRP Irp);
    BOOLEAN
    ArmCancel(
        _In_ PIRP Irp);
    BOOLEAN
    DisarmCancel(
        _In_ PIRP Irp);
    VOID QueueIdleWorkItem();
    VOID
    InvokeIdleCallback(
        _In_ PIRP Irp);
    VOID
    CompleteIdleRequest(
        _In_ PIRP Irp,
        _In_ NTSTATUS Status);

    KSPIN_LOCK m_PostLock;
    HubPdo* m_Pdo;
    PIRP m_HeldIrp;
    PIRP m_NewIrp;
    NTSTATUS m_FollowUpStatus;
    IDLE_FOLLOW_UP m_FollowUp;
    IDLE_POWER_CHANGE m_PowerChange;
    BOOLEAN m_ReturnPending;
    BOOLEAN m_SuspendSupported;
    BOOLEAN m_CancelSeen;
};
