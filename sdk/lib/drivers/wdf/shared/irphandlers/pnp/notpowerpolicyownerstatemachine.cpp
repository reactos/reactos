/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    NotPowerPolicyOwnerStateMachine.cpp

Abstract:

    This module implements the Not Power Policy Owner state machine for the driver
    framework.  This code was split out from PowerPolicyStateMachine.cpp

Author:




Environment:

    Both kernel and user mode

Revision History:



--*/

#include "pnppriv.hpp"

extern "C" {
#if defined(EVENT_TRACING)
#include "NotPowerPolicyOwnerStateMachine.tmh"
#endif
}

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_NotPowerPolOwnerObjectCreatedStates[] =
{
    { PwrPolStart, WdfDevStatePwrPolStarting DEBUGGED_EVENT },
    { PwrPolRemove, WdfDevStatePwrPolRemoved DEBUGGED_EVENT },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_NotPowerPolOwnerStartingStates[] =
{
    { PwrPolPowerUp, WdfDevStatePwrPolStarted DEBUGGED_EVENT },
    { PwrPolPowerUpFailed, WdfDevStatePwrPolStartingFailed DEBUGGED_EVENT },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_NotPowerPolOwnerStartingSucceededStates[] =
{
    { PwrPolPowerDownIoStopped, WdfDevStatePwrPolGotoDx DEBUGGED_EVENT },
    { PwrPolStop, WdfDevStatePwrPolStopping DEBUGGED_EVENT },
    { PwrPolSurpriseRemove, WdfDevStatePwrPolStopping DEBUGGED_EVENT },
    { PwrPolPowerUpHwStarted, WdfDevStatePwrPolGotoD0InD0 TRAP_ON_EVENT },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_NotPowerPolOwnerStartingFailedStates[] =
{
    { PwrPolRemove, WdfDevStatePwrPolRemoved DEBUGGED_EVENT },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_NotPowerPolOwnerGotoDxStates[] =
{
    { PwrPolPowerDown, WdfDevStatePwrPolDx DEBUGGED_EVENT },
    { PwrPolPowerDownFailed, WdfDevStatePwrPolStartingSucceeded DEBUGGED_EVENT },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_NotPowerPolOwnerGotoDxInDxStates[] =
{
    { PwrPolPowerDown, WdfDevStatePwrPolDx TRAP_ON_EVENT },
    { PwrPolPowerDownFailed, WdfDevStatePwrPolDx TRAP_ON_EVENT },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_NotPowerPolOwnerDxStates[] =
{
    { PwrPolPowerUpHwStarted, WdfDevStatePwrPolGotoD0 DEBUGGED_EVENT },
    { PwrPolStop, WdfDevStatePwrPolStopping DEBUGGED_EVENT },
    { PwrPolSurpriseRemove, WdfDevStatePwrPolStopping DEBUGGED_EVENT },
    { PwrPolPowerDownIoStopped, WdfDevStatePwrPolGotoDxInDx TRAP_ON_EVENT },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_NotPowerPolOwnerGotoD0States[] =
{
    { PwrPolPowerUp, WdfDevStatePwrPolStartingSucceeded DEBUGGED_EVENT },
    { PwrPolPowerUpFailed, WdfDevStatePwrPolStartingSucceeded TRAP_ON_EVENT },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_NotPowerPolOwnerGotoD0InD0States[] =
{
    { PwrPolPowerUp, WdfDevStatePwrPolStartingSucceeded TRAP_ON_EVENT },
    { PwrPolPowerUpFailed, WdfDevStatePwrPolStartingSucceeded TRAP_ON_EVENT },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_NotPowerPolOwnerStoppedStates[] =
{
    { PwrPolStart, WdfDevStatePwrPolStarting DEBUGGED_EVENT },
    { PwrPolRemove, WdfDevStatePwrPolRemoved DEBUGGED_EVENT },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_NotPowerPolOwnerStoppingWaitForImplicitPowerDownStates[] =
{
    { PwrPolImplicitPowerDown, WdfDevStatePwrPolStoppingSendStatus DEBUGGED_EVENT },
    { PwrPolImplicitPowerDownFailed, WdfDevStatePwrPolStoppingFailed DEBUGGED_EVENT },
    { PwrPolPowerDownIoStopped, WdfDevStatePwrPolStoppingPoweringDown DEBUGGED_EVENT },
    { PwrPolPowerUpHwStarted, WdfDevStatePwrPolStoppingPoweringUp DEBUGGED_EVENT },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_NotPowerPolOwnerStoppingPoweringUpStates[] =
{
    { PwrPolPowerUp, WdfDevStatePwrPolStoppingWaitingForImplicitPowerDown DEBUGGED_EVENT },
    { PwrPolPowerUpFailed, WdfDevStatePwrPolStoppingWaitingForImplicitPowerDown TRAP_ON_EVENT },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_NotPowerPolOwnerStoppingPoweringDownStates[] =
{
    { PwrPolPowerDown, WdfDevStatePwrPolStoppingWaitingForImplicitPowerDown DEBUGGED_EVENT },
    { PwrPolPowerDownFailed, WdfDevStatePwrPolStoppingWaitingForImplicitPowerDown DEBUGGED_EVENT },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_NotPowerPolOwnerRemovedStates[] =
{
    { PwrPolStart, WdfDevStatePwrPolStarting DEBUGGED_EVENT },
    { PwrPolRemove, WdfDevStatePwrPolRemoved DEBUGGED_EVENT },
};

const NOT_POWER_POLICY_OWNER_STATE_TABLE FxPkgPnp::m_WdfNotPowerPolicyOwnerStates[] =
{
    // current state
    // transition function,
    // target states
    // count of target states
    // Queue state
    //
    { WdfDevStatePwrPolObjectCreated,
      NULL,
      FxPkgPnp::m_NotPowerPolOwnerObjectCreatedStates,
      ARRAY_SIZE(FxPkgPnp::m_NotPowerPolOwnerObjectCreatedStates),
      TRUE,
    },

    { WdfDevStatePwrPolStarting,
      FxPkgPnp::NotPowerPolOwnerStarting,
      FxPkgPnp::m_NotPowerPolOwnerStartingStates,
      ARRAY_SIZE(FxPkgPnp::m_NotPowerPolOwnerStartingStates),
      FALSE,
    },

    { WdfDevStatePwrPolStarted,
      FxPkgPnp::NotPowerPolOwnerStarted,
      NULL,
      0,
      FALSE,
    },

    { WdfDevStatePwrPolStartingSucceeded,
      NULL,
      FxPkgPnp::m_NotPowerPolOwnerStartingSucceededStates,
      ARRAY_SIZE(FxPkgPnp::m_NotPowerPolOwnerStartingSucceededStates),
      TRUE,
    },

    { WdfDevStatePwrPolGotoDx,
      FxPkgPnp::NotPowerPolOwnerGotoDx,
      FxPkgPnp::m_NotPowerPolOwnerGotoDxStates,
      ARRAY_SIZE(FxPkgPnp::m_NotPowerPolOwnerGotoDxStates),
      FALSE,
    },

    { WdfDevStatePwrPolGotoDxInDx,
      FxPkgPnp::NotPowerPolOwnerGotoDxInDx,
      FxPkgPnp::m_NotPowerPolOwnerGotoDxInDxStates,
      ARRAY_SIZE(FxPkgPnp::m_NotPowerPolOwnerGotoDxInDxStates),
      FALSE,
    },

    { WdfDevStatePwrPolDx,
      NULL,
      FxPkgPnp::m_NotPowerPolOwnerDxStates,
      ARRAY_SIZE(FxPkgPnp::m_NotPowerPolOwnerDxStates),
      TRUE,
    },

    { WdfDevStatePwrPolGotoD0,
      FxPkgPnp::NotPowerPolOwnerGotoD0,
      FxPkgPnp::m_NotPowerPolOwnerGotoD0States,
      ARRAY_SIZE(FxPkgPnp::m_NotPowerPolOwnerGotoD0States),
      FALSE,
    },

    { WdfDevStatePwrPolGotoD0InD0,
      FxPkgPnp::NotPowerPolOwnerGotoD0InD0,
      FxPkgPnp::m_NotPowerPolOwnerGotoD0InD0States,
      ARRAY_SIZE(FxPkgPnp::m_NotPowerPolOwnerGotoD0InD0States),
      FALSE,
    },

    { WdfDevStatePwrPolStopping,
      FxPkgPnp::NotPowerPolOwnerStopping,
      NULL,
      0,
      FALSE,
    },

    { WdfDevStatePwrPolStoppingSendStatus,
      FxPkgPnp::NotPowerPolOwnerStoppingSendStatus,
      NULL,
      0,
      FALSE,
    },

    { WdfDevStatePwrPolStartingFailed,
      FxPkgPnp::NotPowerPolOwnerStartingFailed,
      FxPkgPnp::m_NotPowerPolOwnerStartingFailedStates,
      ARRAY_SIZE(FxPkgPnp::m_NotPowerPolOwnerStartingFailedStates),
      TRUE,
    },

    { WdfDevStatePwrPolStoppingFailed,
      FxPkgPnp::NotPowerPolOwnerStoppingFailed,
      NULL,
      0,
      FALSE,
    },

    { WdfDevStatePwrPolStopped,
      NULL,
      FxPkgPnp::m_NotPowerPolOwnerStoppedStates,
      ARRAY_SIZE(FxPkgPnp::m_NotPowerPolOwnerStoppedStates),
      TRUE,
    },

    { WdfDevStatePwrPolStoppingWaitingForImplicitPowerDown,
      NULL,
      FxPkgPnp::m_NotPowerPolOwnerStoppingWaitForImplicitPowerDownStates,
      ARRAY_SIZE(FxPkgPnp::m_NotPowerPolOwnerStoppingWaitForImplicitPowerDownStates),
      TRUE,
    },

    { WdfDevStatePwrPolStoppingPoweringUp,
      FxPkgPnp::NotPowerPolOwnerStoppingPoweringUp,
      FxPkgPnp::m_NotPowerPolOwnerStoppingPoweringUpStates,
      ARRAY_SIZE(FxPkgPnp::m_NotPowerPolOwnerStoppingPoweringUpStates),
      FALSE,
    },

    { WdfDevStatePwrPolStoppingPoweringDown,
      FxPkgPnp::NotPowerPolOwnerStoppingPoweringDown,
      FxPkgPnp::m_NotPowerPolOwnerStoppingPoweringDownStates,
      ARRAY_SIZE(FxPkgPnp::m_NotPowerPolOwnerStoppingPoweringDownStates),
      FALSE,
    },

    { WdfDevStatePwrPolRemoved,
      FxPkgPnp::NotPowerPolOwnerRemoved,
      FxPkgPnp::m_NotPowerPolOwnerRemovedStates,
      ARRAY_SIZE(FxPkgPnp::m_NotPowerPolOwnerRemovedStates),
      TRUE,
    },

    // the last entry must have WdfDevStatePwrPolNull as the current state
    { WdfDevStatePwrPolNull,
      NULL,
      NULL,
      0,
      FALSE,
    },
};

VOID
FxPkgPnp::NotPowerPolicyOwnerEnterNewState(
    __in WDF_DEVICE_POWER_POLICY_STATE NewState
    )
{
    CPNOT_POWER_POLICY_OWNER_STATE_TABLE entry;
    WDF_DEVICE_POWER_POLICY_STATE currentState, newState;
    WDF_DEVICE_POWER_POLICY_NOTIFICATION_DATA data;

    currentState = m_Device->GetDevicePowerPolicyState();
    newState = NewState;

    while (newState != WdfDevStatePwrPolNull) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNPPOWERSTATES,
            "WDFDEVICE 0x%p !devobj 0x%p entering not power policy owner state "
            "%!WDF_DEVICE_POWER_POLICY_STATE! from "
            "%!WDF_DEVICE_POWER_POLICY_STATE!", m_Device->GetHandle(),
            m_Device->GetDeviceObject(), newState, currentState);

        if (m_PowerPolicyStateCallbacks != NULL) {
            //
            // Callback for leaving the old state
            //
            RtlZeroMemory(&data, sizeof(data));

            data.Type = StateNotificationLeaveState;
            data.Data.LeaveState.CurrentState = currentState;
            data.Data.LeaveState.NewState = newState;

            m_PowerPolicyStateCallbacks->Invoke(currentState,
                                                StateNotificationLeaveState,
                                                m_Device->GetHandle(),
                                                &data);
        }

        m_PowerPolicyMachine.m_States.History[
            m_PowerPolicyMachine.IncrementHistoryIndex()] = (USHORT) newState;

        if (m_PowerPolicyStateCallbacks != NULL) {
            //
            // Callback for entering the new state
            //
            RtlZeroMemory(&data, sizeof(data));

            data.Type = StateNotificationEnterState;
            data.Data.EnterState.CurrentState = currentState;
            data.Data.EnterState.NewState = newState;

            m_PowerPolicyStateCallbacks->Invoke(newState,
                                                StateNotificationEnterState,
                                                m_Device->GetHandle(),
                                                &data);
        }

        m_Device->SetDevicePowerPolicyState(newState);
        currentState = newState;

        entry = GetNotPowerPolicyOwnerTableEntry(currentState);

        //
        // Call the state handler, if there is one.
        //
        if (entry->StateFunc != NULL) {
            newState = entry->StateFunc(this);
        }
        else {
            newState = WdfDevStatePwrPolNull;
        }

        if (m_PowerPolicyStateCallbacks != NULL) {
            //
            // Callback for post processing the new state
            //
            RtlZeroMemory(&data, sizeof(data));

            data.Type = StateNotificationPostProcessState;
            data.Data.PostProcessState.CurrentState = currentState;

            m_PowerPolicyStateCallbacks->Invoke(currentState,
                                                StateNotificationPostProcessState,
                                                m_Device->GetHandle(),
                                                &data);
        }
    }
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::NotPowerPolOwnerStarting(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Starting the power policy state machine for a device which is not the power
    policy owner.  Tell the power state machine to start up.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    This->PowerProcessEvent(PowerImplicitD0);
    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::NotPowerPolOwnerStarted(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Starting the power policy state machine for a device which is not the power
    policy owner has succeeded.  Indicate status to the pnp state machine.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    This->PnpProcessEvent(PnpEventPwrPolStarted);
    return WdfDevStatePwrPolStartingSucceeded;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::NotPowerPolOwnerGotoDx(
    __inout FxPkgPnp* This
    )
{
    This->PowerProcessEvent(PowerCompleteDx);
    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::NotPowerPolOwnerGotoDxInDx(
    __inout FxPkgPnp* This
    )
{
    This->PowerProcessEvent(PowerCompleteDx);
    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::NotPowerPolOwnerGotoD0(
    __inout FxPkgPnp* This
    )
{
    This->PowerProcessEvent(PowerCompleteD0);
    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::NotPowerPolOwnerGotoD0InD0(
    __inout FxPkgPnp* This
    )
{
    This->PowerProcessEvent(PowerCompleteD0);
    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::NotPowerPolOwnerStopping(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Stopping the power policy state machine for a device which is not the power
    policy owner.  Tell the power state machine to power down.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolStoppingWaitingForImplicitPowerDown

  --*/
{
    This->PowerProcessEvent(PowerImplicitD3);
    return WdfDevStatePwrPolStoppingWaitingForImplicitPowerDown;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::NotPowerPolOwnerStoppingSendStatus(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Stopping the power policy state machine for a device which is not the power
    policy owner has succeeded.  Inidcate status to the pnp state machine.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolStopped

  --*/
{
    This->PnpProcessEvent(PnpEventPwrPolStopped);
    return WdfDevStatePwrPolStopped;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::NotPowerPolOwnerStartingFailed(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Starting the power policy state machine for a device which is not the power
    policy owner has failed.  Inidcate status to the pnp state machine.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    This->PnpProcessEvent(PnpEventPwrPolStartFailed);
    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::NotPowerPolOwnerStoppingFailed(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Stopping the power policy state machine for a device which is not the power
    policy owner has failed.  Inidcate status to the pnp state machine.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolStopped

  --*/
{
    This->PnpProcessEvent(PnpEventPwrPolStopFailed);
    return WdfDevStatePwrPolStopped;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::NotPowerPolOwnerStoppingPoweringUp(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Right as  we attempted to implicitly power down, a real D0 irp came into the
    stack.  Complete the D0 irp in the power state machine so that the power
    state machine can process the implicit power irp

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    This->PowerProcessEvent(PowerCompleteD0);
    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::NotPowerPolOwnerStoppingPoweringDown(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Right as  we attempted to implicitly power down, a real Dx irp came into the
    stack.  Complete the Dx irp in the power state machine so that the power
    state machine can process the implicit power irp

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    This->PowerProcessEvent(PowerCompleteDx);
    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::NotPowerPolOwnerRemoved(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device is being removed, so prepare for removal.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    //
    // Since we are not the power policy owner, there is nothing to be done.
    // Indicate to the PNP state machine that we are ready for removal.
    //
    This->PnpProcessEvent(PnpEventPwrPolRemoved);
    return WdfDevStatePwrPolNull;
}
