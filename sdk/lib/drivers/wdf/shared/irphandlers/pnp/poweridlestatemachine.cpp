/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    PowerIdleStateMachine.cpp

Abstract:

    This module implements the Power Policy idle state machine for the driver
    framework.

Author:



Environment:

    Both kernel and user mode

Revision History:



--*/

#include "pnppriv.hpp"

extern "C" {
#if defined(EVENT_TRACING)
#include "PowerIdleStateMachine.tmh"
#endif
}

const FxPowerIdleTargetState FxPowerIdleMachine::m_StoppedStates[] =
{
    { PowerIdleEventStart, FxIdleStarted DEBUGGED_EVENT },
};

const FxPowerIdleTargetState FxPowerIdleMachine::m_StartedStates[] =
{
    { PowerIdleEventPowerUpComplete, FxIdleStartedPowerUp DEBUGGED_EVENT },
    { PowerIdleEventPowerUpFailed, FxIdleStartedPowerFailed DEBUGGED_EVENT },
    { PowerIdleEventStop, FxIdleStopped DEBUGGED_EVENT },
};

const FxPowerIdleTargetState FxPowerIdleMachine::m_DisabledStates[] =
{
    { PowerIdleEventEnabled, FxIdleCheckIoCount DEBUGGED_EVENT },
    { PowerIdleEventDisabled, FxIdleDisabled DEBUGGED_EVENT },
    { PowerIdleEventPowerDown, FxIdleGoingToDx DEBUGGED_EVENT },
    { PowerIdleEventPowerDownFailed, FxIdlePowerFailed DEBUGGED_EVENT },
    { PowerIdleEventPowerUpFailed, FxIdlePowerFailed DEBUGGED_EVENT },
    { PowerIdleEventStop, FxIdleStopped DEBUGGED_EVENT },
};

const FxPowerIdleTargetState FxPowerIdleMachine::m_BusyStates[] =
{
    { PowerIdleEventIoDecrement, FxIdleDecrementIo DEBUGGED_EVENT },
    { PowerIdleEventDisabled, FxIdleDisabled DEBUGGED_EVENT },
    { PowerIdleEventPowerDown, FxIdleGoingToDx TRAP_ON_EVENT },
    { PowerIdleEventPowerDownFailed, FxIdlePowerFailed TRAP_ON_EVENT },
};

const FxPowerIdleTargetState FxPowerIdleMachine::m_TimerRunningStates[] =
{
    { PowerIdleEventDisabled, FxIdleDisabling DEBUGGED_EVENT },
    { PowerIdleEventIoIncrement, FxIdleCancelTimer DEBUGGED_EVENT },
    { PowerIdleEventEnabled, FxIdleCancelTimer DEBUGGED_EVENT },
    { PowerIdleEventTimerExpired, FxIdleTimingOut DEBUGGED_EVENT },
};

const FxPowerIdleTargetState FxPowerIdleMachine::m_TimedOutStates[] =
{
    { PowerIdleEventPowerDown, FxIdleTimedOutPowerDown DEBUGGED_EVENT },
    { PowerIdleEventPowerDownFailed, FxIdleTimedOutPowerDownFailed DEBUGGED_EVENT },
    { PowerIdleEventDisabled, FxIdleTimedOutDisabled DEBUGGED_EVENT },
    { PowerIdleEventEnabled, FxIdleTimedOutEnabled DEBUGGED_EVENT },
    { PowerIdleEventIoIncrement, FxIdleTimedOutIoIncrement DEBUGGED_EVENT },
};

const FxPowerIdleTargetState FxPowerIdleMachine::m_InDxStates[] =
{
    { PowerIdleEventPowerUpComplete, FxIdlePowerUp DEBUGGED_EVENT },
    { PowerIdleEventPowerUpFailed, FxIdleInDxPowerUpFailure DEBUGGED_EVENT },
    { PowerIdleEventPowerDownFailed, FxIdleInDxPowerUpFailure TRAP_ON_EVENT },
    { PowerIdleEventStop, FxIdleInDxStopped DEBUGGED_EVENT },
    { PowerIdleEventDisabled, FxIdleInDxDisabled DEBUGGED_EVENT },
    { PowerIdleEventIoIncrement, FxIdleInDxIoIncrement DEBUGGED_EVENT },
    { PowerIdleEventIoDecrement, FxIdleInDx DEBUGGED_EVENT },
    { PowerIdleEventPowerDown, FxIdleInDx DEBUGGED_EVENT },
    { PowerIdleEventEnabled, FxIdleInDxEnabled DEBUGGED_EVENT },
};

const FxPowerIdleTargetState FxPowerIdleMachine::m_WaitForTimeoutStates[] =
{
    { PowerIdleEventTimerExpired, FxIdleTimerExpired DEBUGGED_EVENT },
    { PowerIdleEventPowerDownFailed, FxIdlePowerFailedWaitForTimeout TRAP_ON_EVENT },
    { PowerIdleEventDisabled, FxIdleDisablingWaitForTimeout DEBUGGED_EVENT },
};

const FxPowerIdleTargetState FxPowerIdleMachine::m_DisablingWaitForTimeoutStates[] =
{
    { PowerIdleEventTimerExpired, FxIdleDisablingTimerExpired DEBUGGED_EVENT },
};

const FxPowerIdleTargetState FxPowerIdleMachine::m_PowerFailedWaitForTimeoutStates[] =
{
    { PowerIdleEventTimerExpired, FxIdlePowerFailed TRAP_ON_EVENT },
};

const FxIdleStateTable FxPowerIdleMachine::m_StateTable[] =
{
    // FxIdleStopped
    {   FxPowerIdleMachine::Stopped,
        FxPowerIdleMachine::m_StoppedStates,
        ARRAY_SIZE(FxPowerIdleMachine::m_StoppedStates),
    },

    // FxIdleStarted
    {   FxPowerIdleMachine::Started,
        FxPowerIdleMachine::m_StartedStates,
        ARRAY_SIZE(FxPowerIdleMachine::m_StartedStates),
    },

    // FxIdleStartedPowerUp
    {   FxPowerIdleMachine::StartedPowerUp,
        NULL,
        0,
    },

    // FxIdleStartedPowerFailed
    {   FxPowerIdleMachine::StartedPowerFailed,
        NULL,
        0,
    },

    // FxIdleDisabled
    {   FxPowerIdleMachine::Disabled,
        FxPowerIdleMachine::m_DisabledStates,
        ARRAY_SIZE(FxPowerIdleMachine::m_DisabledStates),
    },

    // FxIdleCheckIoCount
    {   FxPowerIdleMachine::CheckIoCount,
        NULL,
        0,
    },

    // FxIdleBusy
    {   NULL,
        FxPowerIdleMachine::m_BusyStates,
        ARRAY_SIZE(FxPowerIdleMachine::m_BusyStates),
    },

    // FxIdleDecrementIo
    {   FxPowerIdleMachine::DecrementIo,
        NULL,
        0,
    },

    // FxIdleStartTimer
    {   FxPowerIdleMachine::StartTimer,
        NULL,
        0,
    },

    // FxIdleTimerRunning
    {   NULL,
        FxPowerIdleMachine::m_TimerRunningStates,
        ARRAY_SIZE(FxPowerIdleMachine::m_TimerRunningStates)
    },

    // FxIdleTimingOut
    {   FxPowerIdleMachine::TimingOut,
        NULL,
        0,
    },

    // FxIdleTimedOut
    {   NULL,
        FxPowerIdleMachine::m_TimedOutStates,
        ARRAY_SIZE(FxPowerIdleMachine::m_TimedOutStates),
    },

    // FxIdleTimedOutIoIncrement
    {   FxPowerIdleMachine::TimedOutIoIncrement,
        NULL,
        0,
    },

    // FxIdleTimedOutPowerDown
    {   FxPowerIdleMachine::TimedOutPowerDown,
        NULL,
        0,
    },

    // FxIdleTimedOutPowerDownFailed
    {   FxPowerIdleMachine::TimedOutPowerDownFailed,
        NULL,
        0,
    },

    // FxIdleGoingToDx,
    {   FxPowerIdleMachine::GoingToDx,
        NULL,
        0,
    },

    // FxIdleInDx,
    {   FxPowerIdleMachine::InDx,
        FxPowerIdleMachine::m_InDxStates,
        ARRAY_SIZE(FxPowerIdleMachine::m_InDxStates),
    },

    // FxIdleInDxIoIncrement
    {   FxPowerIdleMachine::InDxIoIncrement,
        NULL,
        0,
    },

    // FxIdleInDxPowerUpFailure
    {   FxPowerIdleMachine::InDxPowerUpFailure,
        NULL,
        0,
    },

    // FxIdleInDxStopped
    {   FxPowerIdleMachine::InDxStopped,
        NULL,
        0,
    },

    // FxIdleInDxDisabled
    {   FxPowerIdleMachine::InDxDisabled,
        NULL,
        0,
    },

    // FxIdleInDxEnabled
    {   FxPowerIdleMachine::InDxEnabled,
        NULL,
        0,
    },

    // FxIdlePowerUp
    {   FxPowerIdleMachine::PowerUp,
        NULL,
        0,
    },

    // FxIdlePowerUpComplete
    {   FxPowerIdleMachine::PowerUpComplete,
        NULL,
        0,
    },

    // FxIdleTimedOutDisabled
    {   FxPowerIdleMachine::TimedOutDisabled,
        NULL,
        0,
    },

    // FxIdleTimedOutEnabled
    {   FxPowerIdleMachine::TimedOutEnabled,
        NULL,
        0,
    },

    // FxIdleCancelTimer
    {   FxPowerIdleMachine::CancelTimer,
        NULL,
        0,
    },

    // FxIdleWaitForTimeout
    {   NULL,
        FxPowerIdleMachine::m_WaitForTimeoutStates,
        ARRAY_SIZE(FxPowerIdleMachine::m_WaitForTimeoutStates),
    },

    // FxIdleTimerExpired
    {   FxPowerIdleMachine::TimerExpired,
        NULL,
        0,
    },

    // FxIdleDisabling
    {   FxPowerIdleMachine::Disabling,
        NULL,
        0,
    },

    // FxIdleDisablingWaitForTimeout
    {   NULL,
        FxPowerIdleMachine::m_DisablingWaitForTimeoutStates,
        ARRAY_SIZE(FxPowerIdleMachine::m_DisablingWaitForTimeoutStates),
    },

    // FxIdleDisablingTimerExpired
    {   FxPowerIdleMachine::DisablingTimerExpired,
        NULL,
        0,
    },

    // FxIdlePowerFailedWaitForTimeout
    {   NULL,
        FxPowerIdleMachine::m_PowerFailedWaitForTimeoutStates,
        ARRAY_SIZE(FxPowerIdleMachine::m_PowerFailedWaitForTimeoutStates),
    },

    // FxIdlePowerFailed
    {   FxPowerIdleMachine::PowerFailed,
        NULL,
        0,
    },
};

__inline
FxPkgPnp*
GetPnpPkg(
    __inout FxPowerIdleMachine* This
    )
{
    return CONTAINING_RECORD(This,
                             FxPowerPolicyOwnerSettings,
                             m_PowerIdleMachine)->m_PkgPnp;
}


FxPowerIdleMachine::FxPowerIdleMachine(
    VOID
    )
/*++

Routine Description:
    Constructs the power idle state machine

Arguments:
    None

Return Value:
    None

  --*/
{
    //
    // m_Lock and m_PowerTimeoutTimer are now being initialized in Init method
    // since they may fail for UM.
    //

    m_PowerTimeout.QuadPart = 0;
    m_CurrentIdleState = FxIdleStopped;

    m_EventHistoryIndex = 0;
    m_StateHistoryIndex = 0;

    RtlZeroMemory(&m_EventHistory[0], sizeof(m_EventHistory));
    RtlZeroMemory(&m_StateHistory[0], sizeof(m_StateHistory));

    m_TagTracker = NULL;
}

FxPowerIdleMachine::~FxPowerIdleMachine(
    VOID
    )
{
    if (m_TagTracker != NULL) {
        delete m_TagTracker;
        m_TagTracker = NULL;
    }
}

_Must_inspect_result_
NTSTATUS
FxPowerIdleMachine::Init(
    VOID
    )
{
    NTSTATUS status;

    //
    // For KM, event initialize always succeeds. For UM, it might fail.
    //
    status = m_D0NotificationEvent.Initialize(NotificationEvent, TRUE);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // For KM, timer initialize always succeeds. For UM, it might fail.
    //
    status = m_PowerTimeoutTimer.Initialize(this, _PowerTimeoutDpcRoutine, 0);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    Reset();

    return STATUS_SUCCESS;
}

VOID
FxPowerIdleMachine::CheckAssumptions(
    VOID
    )
/*++

Routine Description:
    This routine is never actually called by running code, it just has
    WDFCASSERTs who upon failure, would not allow this file to be compiled.

    DO NOT REMOVE THIS FUNCTION just because it is not called by any running
    code.

Arguments:
    None

Return Value:
    None

  --*/
{
    WDFCASSERT((sizeof(m_StateTable)/sizeof(m_StateTable[0]))
               ==
               (FxIdleMax - FxIdleStopped));
}

FxPowerIdleStates
FxPowerIdleMachine::Stopped(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    State machine has entered the stopped state, clear the started flag

Arguments:
    This - instance of the state machine

Return Value:
    FxIdleMax

  --*/
{
    This->m_Flags &= ~FxPowerIdleIsStarted;

    return FxIdleMax;
}

FxPowerIdleStates
FxPowerIdleMachine::Started(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    State machine has entered the started state, set the started flag

Arguments:
    This - instance of the state machine

Return Value:
    FxIdleMax

  --*/
{
    This->m_Flags |= FxPowerIdleIsStarted;

    //
    // We are in the started state, but we are not powered up.
    //
    This->m_D0NotificationEvent.Clear();

    return FxIdleMax;
}

FxPowerIdleStates
FxPowerIdleMachine::StartedPowerUp(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    We were in the started and powered off state.  We are powered up,
    so set the event now so that we can wake up any waiters.

Arguments:
    This - instance of the state machine

Return Value:
    FxIdleDisabled

  --*/
{
    //
    // Moving from the started state to the powered on state
    //
    This->SendD0Notification();

    return FxIdleDisabled;
}

FxPowerIdleStates
FxPowerIdleMachine::StartedPowerFailed(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    The state machine was started, but the initial power up failed.  Mark the
    failure.

Arguments:
    This - instance of the state machine

Return Value:
    FxIdleStarted

  --*/
{
    //
    // Failed to initially power up
    //
    This->m_Flags |= FxPowerIdlePowerFailed;

    //
    // We assume in the started state that the event is set
    //
    ASSERT(This->m_D0NotificationEvent.ReadState() == 0);

    return FxIdleStarted;
}

FxPowerIdleStates
FxPowerIdleMachine::Disabled(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    State machine has entered the disabled state, unblock all waiters

Arguments:
    This - instance of the state machine

Return Value:
    FxIdleMax

  --*/

{
    This->m_Flags &= ~FxPowerIdleTimerEnabled;

    return FxIdleMax;
}

FxPowerIdleStates
FxPowerIdleMachine::CheckIoCount(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    Checks the IO count and transitions the appropriate state.   This is the
    first state we are in after being disabled or after transitioning from Dx to
    D0.

Arguments:
    This - instance of the state machine

Return Value:
    new state machine state

  --*/
{
    This->m_Flags |= FxPowerIdleTimerEnabled;

    if (This->m_IoCount == 0) {
        return FxIdleStartTimer;
    }
    else {
        return FxIdleBusy;
    }
}

FxPowerIdleStates
FxPowerIdleMachine::DecrementIo(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    Checks the IO count and returns a new state

Arguments:
    This - instance of the state machine

Return Value:
    new state machine state

  --*/
{
    if (This->m_IoCount == 0) {
        return FxIdleStartTimer;
    }
    else {
        return FxIdleBusy;
    }
}

FxPowerIdleStates
FxPowerIdleMachine::StartTimer(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    The io count is now at zero.  Start the idle timer so that when it expires,
    the device will move into Dx.

Arguments:
    This - instance of the state machine

Return Value:
    FxIdleMax

  --*/
{
    ASSERT((This->m_Flags & FxPowerIdleTimerEnabled) && This->m_IoCount == 0);

    This->m_Flags |= FxPowerIdleTimerStarted;
    This->m_PowerTimeoutTimer.Start(This->m_PowerTimeout,
                                    m_IdleTimerTolerableDelayMS);

    return FxIdleTimerRunning;
}


FxPowerIdleStates
FxPowerIdleMachine::TimingOut(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    The idle timer has expired. Indicate to the power policy state machine
    that it should power down.

Arguments:
    This - instance of the state machine

Return Value:
    FxIdleTimedOut

  --*/
{
#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
    GetPnpPkg(This)->PowerPolicyProcessEvent(PwrPolPowerTimeoutExpired);
#else
    GetPnpPkg(This)->PowerPolicyProcessEvent(
        PwrPolPowerTimeoutExpired,
        TRUE // ProcessEventOnDifferentThread
        );
#endif

    //
    // Timer is no longer running.  Used when we disable the state machine and
    // need to know the timer's running state.
    //
    //
    This->m_Flags &= ~FxPowerIdleTimerStarted;

    //
    // While the device is still powered up, we are no longer in D0 in terms of
    // PowerReference returning immmediately if TRUE is specified to that call.
    //
    This->m_D0NotificationEvent.Clear();

    return FxIdleTimedOut;
}

FxPowerIdleStates
FxPowerIdleMachine::TimedOutIoIncrement(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    A power reference occurred after we notified the power policy machine of
    a power timeout, but before we timed out. Send an io present event to the
    power policy machine so that it can move into the D0 state/not timed out
    state again.

Arguments:
    This - instance of the state machine

Return Value:
    FxIdleTimedOut

  --*/
{
    FxPkgPnp* pPkgPnp;

    pPkgPnp = GetPnpPkg(This);

    if (This->m_Flags & FxPowerIdleIoPresentSent) {
        DoTraceLevelMessage(
            pPkgPnp->GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
            "WDFDEVICE %p idle (in D0) not sending io present event (already sent)",
            pPkgPnp->GetDevice()->GetHandle());
    }
    else {
#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
        pPkgPnp->PowerPolicyProcessEvent(PwrPolIoPresent);
#else
        pPkgPnp->PowerPolicyProcessEvent(
            PwrPolIoPresent,
            TRUE // ProcessEventOnDifferentThread
            );
#endif

        This->m_Flags |= FxPowerIdleIoPresentSent;
    }

    return FxIdleTimedOut;
}

FxPowerIdleStates
FxPowerIdleMachine::TimedOutPowerDown(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    The idle timer fired and we are now powering down.  Clear the flag that
    limits our sending of the io present event to one time while in the timed
    out state.

Arguments:
    This - instance of the state machine

Return Value:
    FxIdleGoingToDx

  --*/
{
    //
    // We can send the io present event again
    //
    This->m_Flags &= ~FxPowerIdleIoPresentSent;

    return FxIdleGoingToDx;
}

FxPowerIdleStates
FxPowerIdleMachine::TimedOutPowerDownFailed(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    The idle timer fired and we could no power down.  Clear the flag that
    limits our sending of the io present event to one time while in the timed
    out state.

Arguments:
    This - instance of the state machine

Return Value:
    FxIdlePowerFailed

  --*/
{
    //
    // We can send the io present event again
    //
    This->m_Flags &= ~FxPowerIdleIoPresentSent;

    return FxIdlePowerFailed;
}

FxPowerIdleStates
FxPowerIdleMachine::GoingToDx(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    The device is going into Dx.  It could be going into Dx because the idle
    timer expired or because the machine is moving into Sx, the reason doesn't
    matter though.  Clear the in D0 event.

Arguments:
    This - instance of the state machine

Return Value:
    FxIdleInDx

  --*/
{
    This->m_D0NotificationEvent.Clear();
    return FxIdleInDx;
}

FxPowerIdleStates
FxPowerIdleMachine::InDx(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    The device has moved into Dx.   If there is no pending io, mark the device
    as idle.  We can be in Dx with pending io if IoIncrement was called after
    the device moved into Dx.

Arguments:
    This - instance of the state machine

Return Value:
    FxIdleMax

  --*/
{
    This->m_Flags |= FxPowerIdleInDx;

    return FxIdleMax;
}

FxPowerIdleStates
FxPowerIdleMachine::InDxIoIncrement(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    In Dx and the io count went up.  Send the event to the power policy state
    machine indicating new io is present.

Arguments:
    This - instance of the state machine

Return Value:
    FxIdleInDx

  --*/
{
    FxPkgPnp* pPkgPnp;

    pPkgPnp = GetPnpPkg(This);

    if (This->m_Flags & FxPowerIdleIoPresentSent) {
        DoTraceLevelMessage(
            pPkgPnp->GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
            "WDFDEVICE %p idle (in Dx) not sending io present event (already sent)",
            pPkgPnp->GetDevice()->GetHandle());
    }
    else {
#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
        pPkgPnp->PowerPolicyProcessEvent(PwrPolIoPresent);
#else
        pPkgPnp->PowerPolicyProcessEvent(
            PwrPolIoPresent,
            TRUE // ProcessEventOnDifferentThread
            );
#endif
        This->m_Flags |= FxPowerIdleIoPresentSent;
    }

    return FxIdleInDx;
}

FxPowerIdleStates
FxPowerIdleMachine::InDxPowerUpFailure(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    Device is in Dx and there was a failure in the power up path.  The device
    is no longer idle, even though it is stil in Dx.

Arguments:
    This - instance of the state machine

Return Value:
    FxIdlePowerFailed

  --*/
{
    //
    // FxPowerIdleInDx - We are no longer in Dx
    // FxPowerIdleIoPresentSent = We can send the io present event again
    //
    This->m_Flags &= ~(FxPowerIdleInDx | FxPowerIdleIoPresentSent);

    return FxIdlePowerFailed;
}

FxPowerIdleStates
FxPowerIdleMachine::InDxStopped(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    The state machine was stopped while in the Dx state.  When the machine is in
    the stopped state, the notification event is in the signaled state.

Arguments:
    This - instance of the state machine

Return Value:
    FxIdleStopped

  --*/
{
    //
    // When the machine is in the stopped state, the notification event is in
    // the signaled state.
    //
    This->SendD0Notification();

    //
    // We are not longer idle since we are not in the Dx state anymore
    //
    This->m_Flags &= ~FxPowerIdleInDx;

    //
    // We are no longer enabled since we are stopped from the Dx state
    //
    This->m_Flags &= ~FxPowerIdleTimerEnabled;

    //
    // We can send the io present event again
    //
    This->m_Flags &= ~FxPowerIdleIoPresentSent;

    return FxIdleStopped;
}

FxPowerIdleStates
FxPowerIdleMachine::InDxDisabled(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    The device is in Dx and the state machine is being disabled (most likely due
    to a surprise remove).

Arguments:
    This - instance of the state machine

Return Value:
    FxIdleDisabled

  --*/
{
    //
    // Idle timer is being disabled while in Dx.  Remain in Dx.
    //
    This->m_Flags &= ~FxPowerIdleTimerEnabled;

    return FxIdleInDx;
}

FxPowerIdleStates
FxPowerIdleMachine::InDxEnabled(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    The device is in Dx and the state machine is being enabled (most like due
    to trying to remain in Dx after Sx->S0.

Arguments:
    This - instance of the state machine

Return Value:
    FxIdleInDx

  --*/
{
    //
    // Idle timer is being enabled while in Dx.  Remain in Dx.
    //
    This->m_Flags |= FxPowerIdleTimerEnabled;

    return FxIdleInDx;
}

FxPowerIdleStates
FxPowerIdleMachine::PowerUp(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    The device powered up enough to where we can let waiters go and start pounding
    on their hardware.

Arguments:
    This - instance of the state machine

Return Value:
    FxIdlePowerUpComplete

  --*/
{
    //
    // FxPowerIdleInDx - we are not longer idle since we are not in the Dx state
    //                   anymore
    // FxPowerIdleIoPresentSent - We can send the io present event again
    //
    This->m_Flags &= ~(FxPowerIdleInDx | FxPowerIdleIoPresentSent);

    This->SendD0Notification();

    return FxIdlePowerUpComplete;
}

FxPowerIdleStates
FxPowerIdleMachine::PowerUpComplete(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    The device is moving into D0, determine which D0 state to move into
    based on the enabled state and io count.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    if (This->m_Flags & FxPowerIdleTimerEnabled) {
        if (This->m_Flags & FxPowerIdleTimerStarted) {
            COVERAGE_TRAP();
            return FxIdleTimerRunning;
        }
        else {
            return FxIdleCheckIoCount;
        }
    }
    else {
        //
        // Not enabled, better not have a timer running
        //
        ASSERT((This->m_Flags & FxPowerIdleTimerStarted) == 0);

        return FxIdleDisabled;
    }
}

FxPowerIdleStates
FxPowerIdleMachine::TimedOutDisabled(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    The power idle state machine is moving into the disabled state.  Set the
    D0 event.

Arguments:
    This - instance of the state machine

Return Value:
    FxIdleDisabled

  --*/
{
    //
    // Notify any waiters that we are now in D0
    //
    This->SendD0Notification();

    //
    // We can send the io present event again
    //
    This->m_Flags &= ~FxPowerIdleIoPresentSent;

    return FxIdleDisabled;
}

FxPowerIdleStates
FxPowerIdleMachine::TimedOutEnabled(
    __inout FxPowerIdleMachine* This
    )
{
    //
    // Notify any waiters that we are now in D0
    //
    This->SendD0Notification();

    //
    // We can send the io present event again
    //
    This->m_Flags &= ~FxPowerIdleIoPresentSent;

    return FxIdleCheckIoCount;
}

FxPowerIdleStates
FxPowerIdleMachine::CancelTimer(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    The timer is running and we need to cancel it because of an io increment or
    the state machine being disabled.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    if (This->CancelIdleTimer()) {
        return FxIdleCheckIoCount;
    }
    else {
        return FxIdleWaitForTimeout;
    }
}

FxPowerIdleStates
FxPowerIdleMachine::TimerExpired(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    The timer was not canceled because it was running.  The timer has now
    fired, so we can move forward.

Arguments:
    This - instance of the state machine

Return Value:
    FxIdleCheckIoCount

  --*/
{
    This->m_Flags &= ~FxPowerIdleTimerStarted;

    return FxIdleCheckIoCount;
}

FxPowerIdleStates
FxPowerIdleMachine::Disabling(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    Timer is running and the state machine is being disabled.  Cancel the idle
    timer.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    if (This->CancelIdleTimer()) {
        return FxIdleDisabled;
    }
    else {
        return FxIdleDisablingWaitForTimeout;
    }
}

FxPowerIdleStates
FxPowerIdleMachine::DisablingTimerExpired(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    When disabling the state machine, the timer could not be canceled.  The
    timer has now expired and the state machine can move forward.

Arguments:
    This - instance of the state machine

Return Value:
    FxIdleDisabled

  --*/
{
    This->m_Flags &= ~FxPowerIdleTimerStarted;

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
    GetPnpPkg(This)->PowerPolicyProcessEvent(PwrPolPowerTimeoutExpired);
#else
    GetPnpPkg(This)->PowerPolicyProcessEvent(
        PwrPolPowerTimeoutExpired,
        TRUE // ProcessEventOnDifferentThread
        );
#endif

    return FxIdleDisabled;
}

FxPowerIdleStates
FxPowerIdleMachine::PowerFailed(
    __inout FxPowerIdleMachine* This
    )
/*++

Routine Description:
    A power operation (up or down) failed.  Mark the machine as failed so that
    PowerReference will fail properly.

Arguments:
    This - instance of the state machine

Return Value:
    FxIdleDisabled

  --*/
{
    //
    // By this time, the timer should be stopped
    //
    ASSERT((This->m_Flags & FxPowerIdleTimerStarted) == 0);

    This->m_Flags |= FxPowerIdlePowerFailed;

    //
    // We are no longer enabled to time out since we are in a failed state
    //
    This->m_Flags &= ~FxPowerIdleTimerEnabled;

    //
    // Wake up any waiters and indicate failure to them.
    //
    This->SendD0Notification();

    return FxIdleDisabled;
}

__drv_maxIRQL(DISPATCH_LEVEL)
__drv_minIRQL(DISPATCH_LEVEL)
__drv_requiresIRQL(DISPATCH_LEVEL)
__drv_sameIRQL
VOID
FxPowerIdleMachine::_PowerTimeoutDpcRoutine(
    __in     PKDPC Dpc,
    __in_opt PVOID Context,
    __in_opt PVOID SystemArgument1,
    __in_opt PVOID SystemArgument2
    )
/*++

Routine Description:
    Timer DPC which posts the timeout expired event to the power policy state
    machine

Arguments:
    Dpc - DPC
    Context, SysArg1, SysArg2 - Unused

Return Value:
    None

  --*/
{
    FxPowerIdleMachine* pThis;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    pThis = (FxPowerIdleMachine*) Context;

    pThis->m_Lock.AcquireAtDpcLevel();
    pThis->ProcessEventLocked(PowerIdleEventTimerExpired);

#if FX_IS_KERNEL_MODE
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    PFN_WDF_DRIVER_DEVICE_ADD pDriverDeviceAdd;

    pFxDriverGlobals = GetPnpPkg(pThis)->GetDriverGlobals();

    //
    // We need to provide XPerf with a symbol of the client to figure out
    // which component this idle timer is for. Since AddDevice is always there
    // we use that to pass the symbol along.
    //
    pDriverDeviceAdd = pFxDriverGlobals->Driver->GetDriverDeviceAddMethod();

    FxPerfTraceDpc(&pDriverDeviceAdd);
#endif

    pThis->m_Lock.ReleaseFromDpcLevel();
}

VOID
FxPowerIdleMachine::Reset(
    VOID
    )
/*++

Routine Description:
    Reset the state machine to a known state on a PDO restart.

Arguments:
    None

Return Value:
    None

  --*/
{
    FxPkgPnp* pPkgPnp;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    ASSERT(m_CurrentIdleState == FxIdleStopped);

    m_IoCount = 0;
    m_Flags = 0x0;

    pPkgPnp = GetPnpPkg(this);
    pFxDriverGlobals = pPkgPnp->GetDriverGlobals();

    if (pFxDriverGlobals->DebugExtension != NULL &&
        pFxDriverGlobals->DebugExtension->TrackPower != FxTrackPowerNone) {
        //
        // Ignore potential failure, power ref tracking is not an essential feature.
        //
        (void)FxTagTracker::CreateAndInitialize(&m_TagTracker,
                                                pFxDriverGlobals,
                                                FxTagTrackerTypePower,
                                                pFxDriverGlobals->DebugExtension->TrackPower == FxTrackPowerRefsAndStack,
                                                pPkgPnp->GetDevice());
    }

    SendD0Notification();
}

VOID
FxPowerIdleMachine::EnableTimer(
    VOID
    )
/*++

Routine Description:
    Public function that the power policy state machine uses to put this state
    machine in to an enabled state and potentially start the idle timer.

Arguments:
    None

Return Value:
    None

  --*/
{
    KIRQL irql;

    m_Lock.Acquire(&irql);
    ProcessEventLocked(PowerIdleEventEnabled);
    m_Lock.Release(irql);
}

BOOLEAN
FxPowerIdleMachine::DisableTimer(
    VOID
    )
/*++

Routine Description:
    Public function which the power policy state machine uses to put this state
    machine into a disabled state.  If necessary, the state machine will attempt
    to cancel the idle timer.

Arguments:
    None

Return Value:
    TRUE if the idle timer was cancelled and the caller may proceed directly to
    its new state

    FALSE if the idle timer was not cancelled and the caller must wait for the
    io timeout event to be posted before proceeding.

  --*/
{
    KIRQL irql;
    BOOLEAN disabledImmediately;

    m_Lock.Acquire(&irql);

    ProcessEventLocked(PowerIdleEventDisabled);

    //
    // If FxPowerIdleTimerStarted is still set after disabling the state machine,
    // then we could not cancel the timer and we must wait for the timer expired
    // event to be posted to this state machine.  This state machine will then
    // post a PwrPolIoPresent event to power policy.
    //
    if (m_Flags & FxPowerIdleTimerStarted) {
        disabledImmediately = FALSE;
    }
    else {
        disabledImmediately = TRUE;
    }

    m_Lock.Release(irql);

    return disabledImmediately;
}

VOID
FxPowerIdleMachine::Start(
    VOID
    )
/*++

Routine Description:
    Public function that the power policy state machine uses to put this state
    machine into a started state so that the caller can call PowerReference
    successfully.

Arguments:
    None

Return Value:
    None

  --*/
{
    KIRQL irql;
    m_Lock.Acquire(&irql);
    ProcessEventLocked(PowerIdleEventStart);
    m_Lock.Release(irql);
}

VOID
FxPowerIdleMachine::Stop(
    VOID
    )
/*++

Routine Description:
    Public function which the power policy state machine uses to put this state
    machine into a state where PowerReference will no longer work.

Arguments:
    None

Return Value:
    None

  --*/
{
    KIRQL irql;

    m_Lock.Acquire(&irql);
    ProcessEventLocked(PowerIdleEventStop);
    m_Lock.Release(irql);
}

_Must_inspect_result_
NTSTATUS
FxPowerIdleMachine::PowerReferenceWorker(
    __in BOOLEAN WaitForD0,
    __in FxPowerReferenceFlags Flags,
    __in_opt PVOID Tag,
    __in_opt LONG Line,
    __in_opt PSTR File
    )
/*++

Routine Description:
    Caller wants to move the device into D0 manually.  The caller may optionally
    wait synchronously for the transition to occur if the device is currently in
    Dx.

Arguments:
    WaitForD0 - TRUE if the caller wants to synchronously wait for the Dx to D0
                transition

    QueryPnpPending - TRUE if we are being called to bring the device back to
                working state when a QueryRemove or a QueryStop

Return Value:
    NTSTATUS

    STATUS_SUCCESS - success
    STATUS_PENDING - transition is occurring
    STATUS_POWER_STATE_INVALID - ower transition has failed

  --*/
{
    NTSTATUS status;
    KIRQL irql;
    ULONG count = 0;



















    //
    // Poke the state machine
    //
    status = IoIncrementWithFlags(Flags, &count);

    //
    // STATUS_PENDING indicates a Dx to D0 transition is occurring right now
    //
    if (status == STATUS_PENDING) {
        if (WaitForD0) {
            FxPkgPnp* pPkgPnp;

            ASSERT(Mx::MxGetCurrentIrql() <= APC_LEVEL);

            //
            // With the current usage, if WaitForD0 is TRUE, then the only
            // acceptable flag is FxPowerReferenceDefault.
            //
            // If the usage changes in the future such that it is acceptable to
            // have WaitForD0 set to TRUE and some flag(s) set, then the ASSERT
            // below should be updated accordingly (or removed altogether).
            //
            ASSERT(FxPowerReferenceDefault == Flags);

            pPkgPnp = GetPnpPkg(this);

            DoTraceLevelMessage(
                pPkgPnp->GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                "WDFDEVICE %p in thread %p waiting synchronously for Dx to D0 "
                "transition",
                pPkgPnp->GetDevice()->GetHandle(),
                Mx::MxGetCurrentThread());

            //
            // Returns success always
            //
            (void) FxPowerIdleMachine::WaitForD0();

            m_Lock.Acquire(&irql);

            //
            // If WaitForD0 is TRUE, then the FxPowerIdleSendPnpPowerUpEvent
            // flag can't be set. That flag is only used when the PnP state
            // machine waits asynchronously for the device to power up during
            // query-remove.
            //
            ASSERT(0 == (m_Flags & FxPowerIdleSendPnpPowerUpEvent));

            if ((m_Flags & FxPowerIdlePowerFailed) != 0x0 ||
                (m_Flags & FxPowerIdleIsStarted) == 0x0) {

                //
                // Event was set because a power up or down failure occurred
                //
                status = STATUS_POWER_STATE_INVALID;

                if (m_Flags & FxPowerIdlePowerFailed) {
                    DoTraceLevelMessage(
                        pPkgPnp->GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "WDFDEVICE %p waiting for D0 in thread %p failed because of "
                        "power failure, %!STATUS!",
                        pPkgPnp->GetDevice()->GetHandle(),
                        Mx::MxGetCurrentThread(),
                        status);
                }
                else {
                    COVERAGE_TRAP();
                    DoTraceLevelMessage(
                        pPkgPnp->GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "WDFDEVICE %p waiting for D0 in thread %p failed because of "
                        "invalid state , %!STATUS!",
                        pPkgPnp->GetDevice()->GetHandle(),
                        Mx::MxGetCurrentThread(), status);
                }

                //
                // Decrement the io count that was taken above
                //
                ASSERT(m_IoCount > 0);
                m_IoCount--;
                ProcessEventLocked(PowerIdleEventIoDecrement);
            }
            else {
                //
                // Successfully returned to D0
                //
                status = STATUS_SUCCESS;
            }
            m_Lock.Release(irql);
        }
    }

    if (m_TagTracker != NULL) {
        //
        // Only track the reference if the call was successful
        // and the counter was actually incremented.
        //
        if (status == STATUS_SUCCESS || status == STATUS_PENDING) {
            m_TagTracker->UpdateTagHistory(Tag, Line, File, TagAddRef, count);
        }
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPowerIdleMachine::IoIncrement(
    VOID
    )
/*++

Routine Description:
    Public function for any component to increment the io count.  The increment
    may cause the state machine to move out of the enabled state depending on
    the current state.

Arguments:
    None

Return Value:
    STATUS_PENDING if the state machine is transition from idle to non idle

    STATUS_SUCCESS otherwise

  --*/
{
    return IoIncrementWithFlags(FxPowerReferenceDefault);
}

_Must_inspect_result_
NTSTATUS
FxPowerIdleMachine::IoIncrementWithFlags(
    __in FxPowerReferenceFlags Flags,
    __out_opt PULONG Count
    )
/*++

Routine Description:
    An enchanced version of FxPowerIdleMachine::IoIncrement that has special
    behavior based on flags passed in by the caller. Please read the routine
    description of FxPowerIdleMachine::IoIncrement as well.

Arguments:
    Flags - The following flags are defined -
         FxPowerReferenceDefault - No special behavior
         FxPowerReferenceSendPnpPowerUpEvent - Set the
           FxPowerIdleSendPnpPowerUpEvent flag in the idle state machine flags.
           This will indicate to the idle state machine that when the device
           powers up, it needs to send the PnpEventDeviceInD0 event to the PnP
           state machine.

Return Value:
    STATUS_PENDING if the state machine is transition from idle to non idle

    STATUS_SUCCESS otherwise

  --*/
{
    NTSTATUS status;
    KIRQL irql;

    m_Lock.Acquire(&irql);

    if (m_Flags & FxPowerIdlePowerFailed) {
        //
        // fail without incrementing the count because we are in an
        // invalid power state
        //
        status = STATUS_POWER_STATE_INVALID;
        COVERAGE_TRAP();
    }
    else if ((m_Flags & FxPowerIdleIsStarted) == 0x0) {
        //
        // The state machine is not yet in a started state
        //
        status = STATUS_POWER_STATE_INVALID;
    }
    else {
        m_IoCount++;
        if (Count != NULL) {
            *Count = m_IoCount;
        }

        ProcessEventLocked(PowerIdleEventIoIncrement);

        if (InD0Locked()) {
            status = STATUS_SUCCESS;
        }
        else {
            status = STATUS_PENDING;
            if (Flags & FxPowerReferenceSendPnpPowerUpEvent) {
                m_Flags |= FxPowerIdleSendPnpPowerUpEvent;
            }
        }
    }
    m_Lock.Release(irql);

    return status;
}


VOID
FxPowerIdleMachine::IoDecrement(
    __in_opt PVOID Tag,
    __in_opt LONG Line,
    __in_opt PSTR File
    )
/*++

Routine Description:
    Public function which allows the caller decrement the pending io count on
    this state machine.  If the count goes to zero and idle is enabled, then
    the timer is started.

Arguments:
    None

Return Value:
    None

  --*/
{
    KIRQL irql;
    FxPkgPnp* pPkgPnp;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    ULONG count;

    pPkgPnp = GetPnpPkg(this);
    pFxDriverGlobals = pPkgPnp->GetDriverGlobals();

    m_Lock.Acquire(&irql);

    if (m_IoCount == 0) {
        //
        // We can get here for the following reasons:
        // 1. Driver called WdfDevicveStopIdle/WdfDeviceResumeIdle in a mismatched
        //    manner. This is a driver bug.
        // 2. Framework did power deref without a corresponding power ref.
        //    This would be a framework bug.
        //
        // We will break into debugger if verifier is turned on. This will allow
        // developers to catch this problem during develeopment.
        // We limit this break to version 1.11+ because otherwise older drivers
        // may hit this, and if they cannot be fixed for some reason, then
        // verifier would need to be turned off to avoid the break which is not
        // desirable.
        //
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDFDEVICE 0x%p !devobj 0x%p The device is being power-dereferenced"
            " without a matching power-reference. This could occur if driver"
            " incorrectly calls WdfDeviceResumeIdle without a matching call to"
            " WdfDeviceStopIdle.",
            pPkgPnp->GetDevice()->GetHandle(),
            pPkgPnp->GetDevice()->GetDeviceObject());

        if (pFxDriverGlobals->IsVerificationEnabled(1, 11, OkForDownLevel)) {
           FxVerifierDbgBreakPoint(pFxDriverGlobals);
        }
    }

    ASSERT(m_IoCount > 0);
    count = --m_IoCount;
    ProcessEventLocked(PowerIdleEventIoDecrement);
    m_Lock.Release(irql);

    if (m_TagTracker != NULL) {
        m_TagTracker->UpdateTagHistory(Tag, Line, File, TagRelease, count);
    }
}

BOOLEAN
FxPowerIdleMachine::QueryReturnToIdle(
    VOID
    )
/*++

Routine Description:
    Public function which allows the caller to query the current io count on
    this state machine.  If the count non zero, the device will be brought back
    to D0.  If zero, the device will remain in Dx.

Arguments:
    None

Return Value:
    if TRUE is returned, there is an outstanding IO.
    if FALSE is returned, the device is idle.

  --*/
{
    KIRQL irql;
    BOOLEAN result;

    //
    // To return to idle, the following must be true
    // 1)  the device must be in Dx (FxPowerIdleInDx)
    // 2)  the timer must *NOT*  be running
    // 3)  an IO count of zero
    //
    m_Lock.Acquire(&irql);

    //     1
    if ((m_Flags & FxPowerIdleInDx) == FxPowerIdleInDx &&
        // 2                                        3
        (m_Flags & FxPowerIdleTimerStarted) == 0 && m_IoCount == 0x0) {
        result = TRUE;
    }
    else {
        result = FALSE;
    }

    //
    // If the caller is querying about returning to idle, then they have
    // processed the io present event that we previously sent.  We must clear
    // the flag, otherwise the following can occur
    // 1)  new io arrives, send the io present message
    // 2)  return to idle (io count goes to zero)
    // 3)  get queried to return to idle, return TRUE
    // 4)  new io arrives, we do not send the new io present message because the
    //     FxPowerIdleIoPresentSent flag is set.
    //
    m_Flags &= ~FxPowerIdleIoPresentSent;

    m_Lock.Release(irql);

    return result;
}

VOID
FxPowerIdleMachine::SendD0Notification(
    VOID
    )
{
    m_D0NotificationEvent.Set();

    if (m_Flags & FxPowerIdleSendPnpPowerUpEvent) {

        m_Flags &= ~FxPowerIdleSendPnpPowerUpEvent;

        //
        // Send an event to the Pnp state machine indicating that the device is
        // now in D0.
        //
#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
        GetPnpPkg(this)->PnpProcessEvent(PnpEventDeviceInD0);
#else
        GetPnpPkg(this)->PnpProcessEvent(
            PnpEventDeviceInD0,
            TRUE // ProcessEventOnDifferentThread
            );
#endif

    }
    return;
}

VOID
FxPowerIdleMachine::ProcessPowerEvent(
    __in FxPowerIdleEvents Event
    )
/*++

Routine Description:
    Post a power related event to the state machine.

Arguments:
    Event - the event to post

Return Value:
    None

  --*/
{
    KIRQL irql;

    //
    // All other event types have specialized public functions
    //
    ASSERT(Event == PowerIdleEventPowerUpComplete ||
           Event == PowerIdleEventPowerUpFailed ||
           Event == PowerIdleEventPowerDown ||
           Event == PowerIdleEventPowerDownFailed);

    m_Lock.Acquire(&irql);
    ProcessEventLocked(Event);
    m_Lock.Release(irql);
}

VOID
FxPowerIdleMachine::ProcessEventLocked(
    __in FxPowerIdleEvents Event
    )
/*++

Routine Description:
    Processes an event and runs it through the state machine

Arguments:


Return Value:


  --*/

{
    const FxIdleStateTable* entry;
    FxPowerIdleStates newState;
    FxPkgPnp* pPkgPnp;

    pPkgPnp = GetPnpPkg(this);

    m_EventHistory[m_EventHistoryIndex] = Event;
    m_EventHistoryIndex = (m_EventHistoryIndex + 1) %
                          (sizeof(m_EventHistory)/sizeof(m_EventHistory[0]));

    entry = &m_StateTable[m_CurrentIdleState-FxIdleStopped];
    newState = FxIdleMax;

    for (ULONG i = 0; i < entry->TargetStatesCount; i++) {
        if (entry->TargetStates[i].PowerIdleEvent == Event) {
            DO_EVENT_TRAP(&entry->TargetStates[i]);
            newState = entry->TargetStates[i].PowerIdleState;
            break;
        }
    }

    if (newState == FxIdleMax) {
        switch (Event) {
        case PowerIdleEventIoIncrement:
        case PowerIdleEventIoDecrement:
            //
            // We always can handle io increment, io decrement, and query return
            // to idle from any state...
            //
            break;

        case PowerIdleEventEnabled:
            if (m_Flags & FxPowerIdleTimerEnabled) {
                //
                // Getting an enable event while enabled is OK
                //
                break;
            }
            //  ||   ||  Fall    ||  ||
            //  \/   \/  through \/  \/

        default:
            //
            // ...but we should not be dropping any other events from this state.
            //

            //
            DoTraceLevelMessage(
                pPkgPnp->GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
                "WDFDEVICE 0x%p !devobj 0x%p power idle state %!FxPowerIdleStates!"
                " dropping event %!FxPowerIdleEvents!",
                pPkgPnp->GetDevice()->GetHandle(),
                pPkgPnp->GetDevice()->GetDeviceObject(),
                m_CurrentIdleState, Event);

            COVERAGE_TRAP();
        }
    }

    while (newState != FxIdleMax) {

        DoTraceLevelMessage(
            pPkgPnp->GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNPPOWERSTATES,
            "WDFDEVICE 0x%p !devobj 0x%p entering power idle state "
            "%!FxPowerIdleStates! from %!FxPowerIdleStates!",
            pPkgPnp->GetDevice()->GetHandle(),
            pPkgPnp->GetDevice()->GetDeviceObject(),
            newState, m_CurrentIdleState);

        m_StateHistory[m_StateHistoryIndex] = newState;
        m_StateHistoryIndex = (m_StateHistoryIndex + 1) %
                              (sizeof(m_StateHistory)/sizeof(m_StateHistory[0]));

        m_CurrentIdleState = newState;
        entry = &m_StateTable[m_CurrentIdleState-FxIdleStopped];

        if (entry->StateFunc != NULL) {
            newState = entry->StateFunc(this);
        }
        else {
            newState = FxIdleMax;
        }
    }
}
