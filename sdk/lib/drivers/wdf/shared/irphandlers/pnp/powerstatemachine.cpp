/*++
Copyright (c) Microsoft. All rights reserved.

Module Name:

    PowerStateMachine.cpp

Abstract:

    This module implements the Power state machine for the driver framework.
    This code was split out from FxPkgPnp.cpp.

Author:




Environment:

    Both kernel and user mode

Revision History:



--*/

#include "pnppriv.hpp"

extern "C" {
#if defined(EVENT_TRACING)
#include "PowerStateMachine.tmh"
#endif
}

#if FX_STATE_MACHINE_VERIFY
    #define VALIDATE_POWER_STATE(_CurrentState, _NewState)                        \
        ValidatePowerStateEntryFunctionReturnValue((_CurrentState), (_NewState))
#else
    #define VALIDATE_POWER_STATE(_CurrentState, _NewState)   (0)
#endif  //FX_STATE_MACHINE_VERIFY


// @@SMVERIFY_SPLIT_BEGIN

//
// The Power State Machine
//
// This state machine responds to power IRPs that relate
// to devices.  It is a subordinate, responding either to IRPs
// that are sent by other drivers, or by IRPs that are
// sent by the Power Policy State Machine.
//
// It responds to:
//
// IRP_MN_SET_POWER -- device power IRPs only
// IRP_MN_WAIT_WAKE
// IRP_MN_WAIT_WAKE Complete
// PowerImplicitD0
// PowerImplicitD3
// ParentMovesToD0
// PowerPolicyStop
// PowerMarkPageable
// PowerMarkNonpageable
//






const POWER_EVENT_TARGET_STATE FxPkgPnp::m_PowerD0OtherStates[] =
{
    { PowerImplicitD3,              WdfDevStatePowerGotoD3Stopped DEBUGGED_EVENT },
    { PowerMarkNonpageable,         WdfDevStatePowerDecideD0State DEBUGGED_EVENT },
    { PowerEventMaximum,            WdfDevStatePowerNull },
};

const POWER_EVENT_TARGET_STATE FxPkgPnp::m_PowerD0NPOtherStates[] =
{
    { PowerImplicitD3,              WdfDevStatePowerGotoD3Stopped DEBUGGED_EVENT },
    { PowerMarkPageable,            WdfDevStatePowerDecideD0State DEBUGGED_EVENT },
    { PowerEventMaximum,            WdfDevStatePowerNull },
};

const POWER_EVENT_TARGET_STATE FxPkgPnp::m_PowerD0BusWakeOwnerOtherStates[] =
{
    { PowerWakeArrival,             WdfDevStatePowerEnablingWakeAtBus DEBUGGED_EVENT },
    { PowerImplicitD3,              WdfDevStatePowerGotoD3Stopped DEBUGGED_EVENT },
    { PowerMarkNonpageable,         WdfDevStatePowerDecideD0State DEBUGGED_EVENT },
    { PowerEventMaximum,            WdfDevStatePowerNull },
};

const POWER_EVENT_TARGET_STATE FxPkgPnp::m_PowerD0BusWakeOwnerNPOtherStates[] =
{
    { PowerWakeArrival,             WdfDevStatePowerEnablingWakeAtBusNP DEBUGGED_EVENT },
    { PowerImplicitD3,              WdfDevStatePowerGotoD3Stopped DEBUGGED_EVENT },
    { PowerMarkPageable,            WdfDevStatePowerD0BusWakeOwner DEBUGGED_EVENT },
    { PowerEventMaximum,            WdfDevStatePowerNull },
};

const POWER_EVENT_TARGET_STATE FxPkgPnp::m_PowerD0ArmedForWakeOtherStates[] =
{
    { PowerWakeSucceeded,           WdfDevStatePowerD0DisarmingWakeAtBus DEBUGGED_EVENT },
    { PowerWakeFailed,              WdfDevStatePowerD0DisarmingWakeAtBus DEBUGGED_EVENT },
    { PowerWakeCanceled,            WdfDevStatePowerD0DisarmingWakeAtBus DEBUGGED_EVENT },
    { PowerMarkNonpageable,         WdfDevStatePowerD0ArmedForWakeNP TRAP_ON_EVENT },
    { PowerImplicitD3,              WdfDevStatePowerGotoImplicitD3DisarmWakeAtBus DEBUGGED_EVENT },
    { PowerEventMaximum,            WdfDevStatePowerNull },
};

const POWER_EVENT_TARGET_STATE FxPkgPnp::m_PowerD0ArmedForWakeNPOtherStates[] =
{
    { PowerWakeSucceeded,           WdfDevStatePowerD0DisarmingWakeAtBusNP DEBUGGED_EVENT },
    { PowerWakeFailed,              WdfDevStatePowerD0DisarmingWakeAtBusNP TRAP_ON_EVENT },
    { PowerWakeCanceled,            WdfDevStatePowerD0DisarmingWakeAtBusNP DEBUGGED_EVENT },
    { PowerMarkPageable,            WdfDevStatePowerD0ArmedForWake TRAP_ON_EVENT },
    { PowerImplicitD3,              WdfDevStatePowerGotoImplicitD3DisarmWakeAtBus TRAP_ON_EVENT },
    { PowerEventMaximum,            WdfDevStatePowerNull },
};

const POWER_EVENT_TARGET_STATE FxPkgPnp::m_PowerDNotZeroOtherStates[] =
{
    { PowerImplicitD3,              WdfDevStatePowerGotoDxStoppedDisableInterrupt DEBUGGED_EVENT },
    { PowerEventMaximum,            WdfDevStatePowerNull },
};

const POWER_EVENT_TARGET_STATE FxPkgPnp::m_PowerDNotZeroNPOtherStates[] =
{
    { PowerImplicitD3, WdfDevStatePowerGotoDxStoppedDisableInterruptNP DEBUGGED_EVENT },
    { PowerEventMaximum, WdfDevStatePowerNull },
};

const POWER_EVENT_TARGET_STATE FxPkgPnp::m_DxArmedForWakeOtherStates[] =
{
    { PowerWakeSucceeded,           WdfDevStatePowerWakePending DEBUGGED_EVENT },
    { PowerWakeCanceled,            WdfDevStatePowerWakePending DEBUGGED_EVENT },
    { PowerWakeFailed,              WdfDevStatePowerWakePending DEBUGGED_EVENT },
    { PowerImplicitD3,              WdfDevStatePowerDxStoppedDisarmWake TRAP_ON_EVENT },
    { PowerEventMaximum,            WdfDevStatePowerNull },
};

const POWER_EVENT_TARGET_STATE FxPkgPnp::m_DxArmedForWakeNPOtherStates[] =
{
    { PowerWakeSucceeded,           WdfDevStatePowerWakePendingNP DEBUGGED_EVENT },
    { PowerWakeFailed,              WdfDevStatePowerWakePendingNP DEBUGGED_EVENT },
    { PowerWakeCanceled,            WdfDevStatePowerWakePendingNP DEBUGGED_EVENT },
    { PowerImplicitD3,              WdfDevStatePowerDxStoppedDisarmWakeNP TRAP_ON_EVENT },
    { PowerEventMaximum,            WdfDevStatePowerNull },
};

const POWER_EVENT_TARGET_STATE FxPkgPnp::m_WakePendingOtherStates[] =
{
    { PowerImplicitD0,              WdfDevStatePowerCheckParentStateArmedForWake TRAP_ON_EVENT },
    { PowerImplicitD3,              WdfDevStatePowerDxStoppedDisarmWake DEBUGGED_EVENT },
    { PowerEventMaximum,            WdfDevStatePowerNull },
};

const POWER_EVENT_TARGET_STATE FxPkgPnp::m_WakePendingNPOtherStates[] =
{
    { PowerImplicitD0,              WdfDevStatePowerCheckParentStateArmedForWakeNP TRAP_ON_EVENT },
    { PowerImplicitD3,              WdfDevStatePowerDxStoppedDisarmWakeNP DEBUGGED_EVENT },
    { PowerEventMaximum,            WdfDevStatePowerNull },
};

const POWER_EVENT_TARGET_STATE FxPkgPnp::m_PowerStoppedOtherStates[] =
{
    { PowerDx,                      WdfDevStatePowerStoppedCompleteDx DEBUGGED_EVENT },
    { PowerEventMaximum,            WdfDevStatePowerNull },
};

const POWER_EVENT_TARGET_STATE FxPkgPnp::m_PowerDxStoppedOtherStates[] =
{
    { PowerD0,                      WdfDevStatePowerGotoStopped DEBUGGED_EVENT },
    { PowerEventMaximum,            WdfDevStatePowerNull },
};

const POWER_STATE_TABLE FxPkgPnp::m_WdfPowerStates[] =
{
    // WdfDevStatePowerObjectCreated
    {   NULL,
        { PowerImplicitD0, WdfDevStatePowerStartingCheckDeviceType DEBUGGED_EVENT },
        NULL,
        { TRUE,
          PowerMarkPageable | // parent sends usage notifications before the PDO is
          PowerMarkNonpageable | // started
          PowerParentToD0 | // parent powered up upon enumeration of child
          PowerDx | // If we are on top of a power policy owner who sends a Dx
                    // during start (or after AddDevice, etc)
          PowerD0   // If we are on top of a power policy owner who sends a D0
                    // to the stack in start device, we can get this event early
        },
    },

    // WdfDevStatePowerCheckDeviceType
    {   FxPkgPnp::PowerCheckDeviceType,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerCheckDeviceTypeNP
    {   FxPkgPnp::PowerCheckDeviceTypeNP,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerCheckParentState
    {   FxPkgPnp::PowerCheckParentState,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerCheckParentStateNP
    {   FxPkgPnp::PowerCheckParentStateNP,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerEnablingWakeAtBus
    {   FxPkgPnp::PowerEnablingWakeAtBus,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerEnablingWakeAtBusNP
    {   FxPkgPnp::PowerEnablingWakeAtBusNP,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerD0
    {   FxPkgPnp::PowerDZero,
        { PowerDx, WdfDevStatePowerGotoDx DEBUGGED_EVENT },
        FxPkgPnp::m_PowerD0OtherStates,
        { TRUE,
          PowerImplicitD3 |
          PowerD0               // A non WDF power policy owner might send a D0 irp
                                // while we are in D0
        },
    },

    // WdfDevStatePowerD0NP
    {   FxPkgPnp::PowerD0NP,
        { PowerDx, WdfDevStatePowerGotoDxNP DEBUGGED_EVENT },
        FxPkgPnp::m_PowerD0NPOtherStates,
        { TRUE,
          PowerD0               // A non WDF power policy owner might send a D0 irp
                                // while we are in D0
        },
    },

    // WdfDevStatePowerD0BusWakeOwner
    {   FxPkgPnp::PowerD0BusWakeOwner,
        { PowerDx, WdfDevStatePowerGotoDx DEBUGGED_EVENT },
        FxPkgPnp::m_PowerD0BusWakeOwnerOtherStates,
        { TRUE,
          PowerWakeSucceeded |  // During surprise remove, the pnp state machine
                                // could complete the ww request and result in
                                // this event before the pwr pol machine is stopped
          PowerWakeCanceled |   // while powering up, the wait wake owner canceled
                                // the ww irp
          PowerWakeFailed   |   // while powering up, the wait wake owner failed
                                // the ww irp
          PowerParentToD0 |
          PowerD0               // A non WDF power policy owner might send a D0 irp
                                // while we are in D0
        },
    },

    // WdfDevStatePowerD0BusWakeOwnerNP
    {   FxPkgPnp::PowerD0BusWakeOwnerNP,
        { PowerDx, WdfDevStatePowerGotoDxNP DEBUGGED_EVENT },
        FxPkgPnp::m_PowerD0BusWakeOwnerNPOtherStates,
        { TRUE,
          PowerWakeSucceeded |  // During surprise remove, the pnp state machine
                                // could complete the ww request and result in
                                // this event before the pwr pol machine is stopped
          PowerWakeCanceled |   // while powering up, the wait wake owner canceled
                                // the ww irp
          PowerParentToD0 |
          PowerD0               // A non WDF power policy owner might send a D0 irp
                                // while we are in D0
        },
    },

    // WdfDevStatePowerD0ArmedForWake
    {   FxPkgPnp::PowerD0ArmedForWake,
        { PowerDx, WdfDevStatePowerGotoDxArmedForWake DEBUGGED_EVENT },
        FxPkgPnp::m_PowerD0ArmedForWakeOtherStates,
        { TRUE,
          PowerParentToD0 |
          PowerWakeArrival  |   // PowerIsWakeRequestPresent() returned true in
                                // WdfDevStatePowerD0BusWakeOwner and raced with
                                // this event being processed
          PowerWakeCanceled |
          PowerD0               // A non WDF power policy owner might send a D0 irp
                                // while we are in D0
        },
    },

    // WdfDevStatePowerD0ArmedForWakeNP
    {   FxPkgPnp::PowerD0ArmedForWakeNP,
        { PowerDx, WdfDevStatePowerGotoDxArmedForWakeNP DEBUGGED_EVENT },
        FxPkgPnp::m_PowerD0ArmedForWakeNPOtherStates,
        { TRUE,
          PowerParentToD0 |
          PowerWakeArrival  |   // PowerIsWakeRequestPresent() returned true in
                                // WdfDevStatePowerD0BusWakeOwnerNP and raced with
                                // this event being processed
          PowerWakeCanceled |
          PowerD0               // A non WDF power policy owner might send a D0 irp
                                // while we are in D0
        },
    },

    // WdfDevStatePowerD0DisarmingWakeAtBus
    {   FxPkgPnp::PowerD0DisarmingWakeAtBus,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerD0DisarmingWakeAtBusNP
    {   FxPkgPnp::PowerD0DisarmingWakeAtBusNP,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerD0Starting
    {   FxPkgPnp::PowerD0Starting,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerD0StartingConnectInterrupt
    {   FxPkgPnp::PowerD0StartingConnectInterrupt,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerD0StartingDmaEnable
    {   FxPkgPnp::PowerD0StartingDmaEnable,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerD0StartingStartSelfManagedIo
    {   FxPkgPnp::PowerD0StartingStartSelfManagedIo,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerDecideD0State
    {   FxPkgPnp::PowerDecideD0State,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerGotoD3Stopped
    {   FxPkgPnp::PowerGotoD3Stopped,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerStopped
    {   NULL,
        { PowerImplicitD0, WdfDevStatePowerStartingCheckDeviceType DEBUGGED_EVENT },
        FxPkgPnp::m_PowerStoppedOtherStates,
        { TRUE,
          PowerD0 |             // as a filter above the PPO and the PPO powers on the stack
                                // before seeing a surprise remove or remove irp
          PowerWakeFailed |     // power policy owner canceled the wake request while
                                // we were transitioning to stop (or after the
                                // transition succeeded)
          PowerParentToD0 },
    },

    // WdfDevStatePowerStartingCheckDeviceType
    {   FxPkgPnp::PowerStartingCheckDeviceType,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerStartingChild
    {   FxPkgPnp::PowerStartingChild,
        { PowerParentToD0, WdfDevStatePowerD0Starting DEBUGGED_EVENT },
        NULL,
        { TRUE,
          0 },
    },

    // WdfDevStatePowerDxDisablingWakeAtBus
    {   FxPkgPnp::PowerDxDisablingWakeAtBus,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerDxDisablingWakeAtBusNP
    {   FxPkgPnp::PowerDxDisablingWakeAtBusNP,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerGotoDx
    {   FxPkgPnp::PowerGotoDNotZero,
        { PowerCompleteDx, WdfDevStatePowerNotifyingD0ExitToWakeInterrupts DEBUGGED_EVENT },
        NULL,
        { FALSE,
          PowerWakeArrival |    // on a PDO which is the PPO, it will send a wake
                                // request in this state.


          PowerParentToD0       // Parent is powering up while this device is powering
                                // down
        },
    },

    // WdfDevStatePowerGotoDxNP
    {   FxPkgPnp::PowerGotoDNotZeroNP,
        { PowerCompleteDx, WdfDevStatePowerNotifyingD0ExitToWakeInterruptsNP DEBUGGED_EVENT },
        NULL,
        { FALSE,
          PowerWakeArrival |    // on a PDO which is the PPO, it will send a wake
                                // request in this state.


          PowerParentToD0       // Parent is powering up while this device is powering
                                // down
        },
    },

    // WdfDevStatePowerGotoDxIoStopped
    {   FxPkgPnp::PowerGotoDNotZeroIoStopped,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerGotoDxIoStoppedNP
    {   FxPkgPnp::PowerGotoDNotZeroIoStoppedNP,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerGotoDxNPFailed
    {   FxPkgPnp::PowerGotoDxNPFailed,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // NOTE:  can't use PowerDx as a func name since it's an enum value
    // WdfDevStatePowerDx
    {   NULL,
        { PowerD0, WdfDevStatePowerCheckDeviceType DEBUGGED_EVENT },
        FxPkgPnp::m_PowerDNotZeroOtherStates,
        { TRUE,
          PowerWakeArrival |    // on a PDO which is the PPO, it will send a wake
                                // request in this state.



          PowerWakeCanceled |   // on a PDO which is the PPO, it will cancel the
                                // the wake request in this state.  Since we didn't
                                // handle PowerWakeArrival in WdfDevStatePowerGotoDx
                                // for this scenario, we must also handle the canel
                                // here.  Even if we handle wake arrived in that
                                // state, the PPO (non KMDF) could send a wake
                                // request while in Dx and then cancel it in Dx,
                                // so we must still ignore this event here.

          PowerWakeSucceeded |  // on a PDO which is the PPO, a completion of the
                                // wait wake can arrive in this state.  This event
                                // can be ignored and the pending wait wake will
                                // be completed.

          PowerImplicitD3 |
          PowerParentToD0 |     // parent went to D0 first while the PDO was still
                                // in Dx

          PowerDx               // power policy sent a Dx to Dx transition
        },
    },

    // WdfDevStatePowerDxNP
    {   NULL,
        { PowerD0, WdfDevStatePowerCheckDeviceTypeNP DEBUGGED_EVENT },
        FxPkgPnp::m_PowerDNotZeroNPOtherStates,
        { TRUE,
          PowerWakeArrival |    // on a PDO which is the PPO, it will send a wake
                                // request in this state.



          PowerWakeCanceled |   // on a PDO which is the PPO, it will cancel the
                                // the wake request in this state.  Since we didn't
                                // handle PowerWakeArrival in WdfDevStatePowerGotoDx
                                // for this scenario, we must also handle the canel
                                // here.  Even if we handle wake arrived in that
                                // state, the PPO (non KMDF) could send a wake
                                // request while in Dx and then cancel it in Dx,
                                // so we must still ignore this event here.

          PowerWakeSucceeded |  // on a PDO which is the PPO, a completion of the
                                // wait wake can arrive in this state.  This event
                                // can be ignored and the pending wait wake will
                                // be completed.

          PowerParentToD0 |     // parent went to D0 first while the PDO was still
                                // in Dx

          PowerDx               // power policy sent a Dx to Dx transition
        },
    },

    // WdfDevStatePowerGotoDxArmedForWake
    {   FxPkgPnp::PowerGotoDxArmedForWake,
        { PowerCompleteDx, WdfDevStatePowerGotoDxIoStoppedArmedForWake DEBUGGED_EVENT },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerGotoDxArmedForWakeNP
    {   FxPkgPnp::PowerGotoDxArmedForWakeNP,
        { PowerCompleteDx, WdfDevStatePowerGotoDxIoStoppedArmedForWakeNP DEBUGGED_EVENT },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerGotoDxIoStoppedArmedForWake
    {   FxPkgPnp::PowerGotoDxIoStoppedArmedForWake,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerGotoDxIoStoppedArmedForWakeNP
    {   FxPkgPnp::PowerGotoDxIoStoppedArmedForWakeNP,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerDxArmedForWake
    {   NULL,
        { PowerD0, WdfDevStatePowerCheckParentStateArmedForWake DEBUGGED_EVENT },
        FxPkgPnp::m_DxArmedForWakeOtherStates,
        { TRUE,
          PowerParentToD0      // can occur on a PDO when a Dx transition completes
                               // on the parent and it wakes up before the child
        },
    },

    // WdfDevStatePowerDxArmedForWakeNP
    {   NULL,
        { PowerD0, WdfDevStatePowerCheckParentStateArmedForWakeNP DEBUGGED_EVENT },
        FxPkgPnp::m_DxArmedForWakeNPOtherStates,
        { TRUE,
          PowerParentToD0      // can occur on a PDO when a Dx transition completes
                               // on the parent and it wakes up before the child
        },
    },

    // WdfDevStatePowerCheckParentStateArmedForWake
    {   FxPkgPnp::PowerCheckParentStateArmedForWake,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerCheckParentStateArmedForWakeNP
    {   FxPkgPnp::PowerCheckParentStateArmedForWakeNP,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerWaitForParentArmedForWake
    {   NULL,
        { PowerParentToD0, WdfDevStatePowerDxDisablingWakeAtBus DEBUGGED_EVENT },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerWaitForParentArmedForWakeNP
    {   NULL,
        { PowerParentToD0, WdfDevStatePowerDxDisablingWakeAtBusNP DEBUGGED_EVENT },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerStartSelfManagedIo
    {   FxPkgPnp::PowerStartSelfManagedIo,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerStartSelfManagedIoNP
    {   FxPkgPnp::PowerStartSelfManagedIoNP,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerStartSelfManagedIoFailed
    {   FxPkgPnp::PowerStartSelfManagedIoFailed,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerStartSelfManagedIoFailedNP
    {   FxPkgPnp::PowerStartSelfManagedIoFailedNP,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerWaitForParent
    {   NULL,
        { PowerParentToD0, WdfDevStatePowerWaking DEBUGGED_EVENT },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerWaitForParentNP
    {   NULL,
        { PowerParentToD0, WdfDevStatePowerWakingNP DEBUGGED_EVENT },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerWakePending
    {   FxPkgPnp::PowerWakePending,
        { PowerD0, WdfDevStatePowerCheckParentStateArmedForWake DEBUGGED_EVENT },
        FxPkgPnp::m_WakePendingOtherStates,
        { TRUE,
          PowerParentToD0 // parent moved to D0 while the child was moving to
                          // D0 from Dx armed for wake
        },
    },

    // WdfDevStatePowerWakePendingNP
    {   FxPkgPnp::PowerWakePendingNP,
        { PowerD0, WdfDevStatePowerCheckParentStateArmedForWakeNP DEBUGGED_EVENT },
        FxPkgPnp::m_WakePendingNPOtherStates,
        { TRUE,
          PowerParentToD0 // parent moved to D0 while the child was moving to
                          // D0 from Dx armed for wake
        },
    },

    // WdfDevStatePowerWaking
    {   FxPkgPnp::PowerWaking,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerWakingNP
    {   FxPkgPnp::PowerWakingNP,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerWakingConnectInterrupt
    {   FxPkgPnp::PowerWakingConnectInterrupt,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerWakingConnectInterruptNP
    {   FxPkgPnp::PowerWakingConnectInterruptNP,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerWakingConnectInterruptFailed
    {   FxPkgPnp::PowerWakingConnectInterruptFailed,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerWakingConnectInterruptFailedNP
    {   FxPkgPnp::PowerWakingConnectInterruptFailedNP,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerWakingDmaEnable
    {   FxPkgPnp::PowerWakingDmaEnable,
        { PowerCompleteD0, WdfDevStatePowerStartSelfManagedIo DEBUGGED_EVENT },
        NULL,
        { FALSE,
          PowerParentToD0 // parent moved to D0 while the child was moving to
                          // D0 from Dx armed for wake
        },
    },

    // WdfDevStatePowerWakingDmaEnableNP
    {   FxPkgPnp::PowerWakingDmaEnableNP,
        { PowerCompleteD0, WdfDevStatePowerStartSelfManagedIoNP DEBUGGED_EVENT },
        NULL,
        { FALSE,
          PowerParentToD0 // parent moved to D0 while the child was moving to
                          // D0 from Dx armed for wake
        },
    },

    // WdfDevStatePowerWakingDmaEnableFailed
    {   FxPkgPnp::PowerWakingDmaEnableFailed,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerWakingDmaEnableFailedNP
    {   FxPkgPnp::PowerWakingDmaEnableFailedNP,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerReportPowerUpFailedDerefParent
    {   FxPkgPnp::PowerReportPowerUpFailedDerefParent,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0
        },
    },

    // WdfDevStatePowerReportPowerUpFailed
    {   FxPkgPnp::PowerReportPowerUpFailed,
        { PowerImplicitD3, WdfDevStatePowerPowerFailedPowerDown DEBUGGED_EVENT },
        NULL,
        { TRUE,
          0
        },
    },

    // WdfDevStatePowerPowerFailedPowerDown
    {   FxPkgPnp::PowerPowerFailedPowerDown,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerReportPowerDownFailed
    {   FxPkgPnp::PowerReportPowerDownFailed,
        { PowerImplicitD3, WdfDevStatePowerPowerFailedPowerDown DEBUGGED_EVENT },
        NULL,
        { TRUE,
          0 },
    },

    // WdfDevStatePowerInitialConnectInterruptFailed
    {   FxPkgPnp::PowerInitialConnectInterruptFailed,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerInitialDmaEnableFailed
    {   FxPkgPnp::PowerInitialDmaEnableFailed,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerInitialSelfManagedIoFailed
    {   FxPkgPnp::PowerInitialSelfManagedIoFailed,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerInitialPowerUpFailedDerefParent
    {   FxPkgPnp::PowerInitialPowerUpFailedDerefParent,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerInitialPowerUpFailed
    {   FxPkgPnp::PowerInitialPowerUpFailed,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerDxStoppedDisarmWake
    {   FxPkgPnp::PowerDxStoppedDisarmWake,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerDxStoppedDisarmWakeNP
    {   FxPkgPnp::PowerDxStoppedDisarmWakeNP,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerGotoDxStoppedDisableInterruptNP
    {   FxPkgPnp::PowerGotoDxStoppedDisableInterruptNP,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerGotoDxStopped
    {   FxPkgPnp::PowerGotoDxStopped,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerDxStopped
    {   NULL,
        { PowerImplicitD0, WdfDevStatePowerDxStoppedDecideDxState TRAP_ON_EVENT },
        FxPkgPnp::m_PowerDxStoppedOtherStates,
        { TRUE,
          0 },
    },

    // WdfDevStatePowerGotoStopped
    {   FxPkgPnp::PowerGotoStopped,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerStoppedCompleteDx
    {   FxPkgPnp::PowerStoppedCompleteDx,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerDxStoppedDecideDxState
    {   FxPkgPnp::PowerDxStoppedDecideDxState,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerDxStoppedArmForWake
    {   FxPkgPnp::PowerDxStoppedArmForWake,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerDxStoppedArmForWakeNP
    {   FxPkgPnp::PowerDxStoppedArmForWakeNP,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerFinalPowerDownFailed
    {   FxPkgPnp::PowerFinalPowerDownFailed,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerFinal
    {   NULL,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerGotoImplicitD3DisarmWakeAtBus
    {   FxPkgPnp::PowerGotoImplicitD3DisarmWakeAtBus,
        { PowerEventMaximum, WdfDevStatePowerNull DEBUGGED_EVENT },
        NULL,
        { FALSE,
            0 },
    },

    // WdfDevStatePowerUpFailed
    {   FxPkgPnp::PowerUpFailed,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerUpFailedDerefParent
    {   FxPkgPnp::PowerUpFailedDerefParent,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerGotoDxFailed
    {   FxPkgPnp::PowerGotoDxFailed,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerGotoDxStoppedDisableInterrupt
    {   FxPkgPnp::PowerGotoDxStoppedDisableInterrupt,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerUpFailedNP
    {   FxPkgPnp::PowerUpFailedNP,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerUpFailedDerefParentNP
    {   FxPkgPnp::PowerUpFailedDerefParentNP,
        { PowerEventMaximum, WdfDevStatePowerNull },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerNotifyingD0ExitToWakeInterrupts
    {   FxPkgPnp::PowerNotifyingD0ExitToWakeInterrupts,
        { PowerWakeInterruptCompleteTransition, WdfDevStatePowerGotoDxIoStopped DEBUGGED_EVENT },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerNotifyingD0EntryToWakeInterrupts
    {   FxPkgPnp::PowerNotifyingD0EntryToWakeInterrupts,
        { PowerWakeInterruptCompleteTransition, WdfDevStatePowerWakingConnectInterrupt DEBUGGED_EVENT },
        NULL,
        { FALSE,
          0 },
    },
    // WdfDevStatePowerNotifyingD0ExitToWakeInterruptsNP
    {   FxPkgPnp::PowerNotifyingD0ExitToWakeInterruptsNP,
        { PowerWakeInterruptCompleteTransition, WdfDevStatePowerGotoDxIoStoppedNP TRAP_ON_EVENT },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerNotifyingD0EntryToWakeInterrupts
    {   FxPkgPnp::PowerNotifyingD0EntryToWakeInterruptsNP,
        { PowerWakeInterruptCompleteTransition, WdfDevStatePowerWakingConnectInterruptNP TRAP_ON_EVENT },
        NULL,
        { FALSE,
          0 },
    },

    // WdfDevStatePowerNull
    // *** no entry for this state ***
};

// @@SMVERIFY_SPLIT_END

_Must_inspect_result_
NTSTATUS
FxPowerMachine::Init(
    __inout FxPkgPnp* Pnp,
    __in PFN_PNP_EVENT_WORKER WorkerRoutine
    )
{
    NTSTATUS status;

    status = FxThreadedEventQueue::Init(Pnp, WorkerRoutine);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    return STATUS_SUCCESS;
}

VOID
FxPkgPnp::PowerCheckAssumptions(
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
    WDFCASSERT(sizeof(FxPowerStateInfo) == sizeof(ULONG));

    WDFCASSERT((sizeof(m_WdfPowerStates)/sizeof(m_WdfPowerStates[0]))
               ==
               (WdfDevStatePowerNull - WdfDevStatePowerObjectCreated));

    // we assume these are the same length when we update the history index
    WDFCASSERT((sizeof(m_PowerMachine.m_Queue.Events)/
                sizeof(m_PowerMachine.m_Queue.Events[0]))
               ==
               (sizeof(m_PowerMachine.m_States.History)/
                sizeof(m_PowerMachine.m_States.History[0])));
}


/*++

The locking model for the Power state machine requires that events be enqueued
possibly at DISPATCH_LEVEL.  It also requires that the state machine be
runnable at PASSIVE_LEVEL.  Consequently, we have two locks, one DISPATCH_LEVEL
lock that guards the event queue and one PASSIVE_LEVEL lock that guards the
state machine itself.

The Power state machine has a few constraints that the PnP state machine
doesn't.  Sometimes it has to call some driver functions at PASSIVE_LEVEL, but
with the disks turned off.  This means that these functions absolutely must not
page fault.  You might think that this means that we should call the driver at
DISPATCH_LEVEL, and you'd be right if your only concern were for perfectly
safe code.  The problem with that approach, though is that it will force much
of the rest of the driver to DISPATCH_LEVEL, which will only push the driver
writer into using lots of asynchronous work items, which will complicate their
code and make it unsafe in a new variety of ways.  So we're going to go with
PASSIVE_LEVEL here and setting a timeout of 20 seconds.  If the driver faults,
the timer will fire and log the failure.  This also means that the driver must
complete these callbacks within 20 seconds.  Even beyond that, it means that
the work items must be queued onto a special thread, one that once the machine
has started to go to sleep, never handles any work items that may fault.

Algorithm:

1)  Acquire the Power queue lock.
2)  Enqueue the request.  Requests are put at the end of the queue.
3)  Drop the Power queue lock.
4)  If the thread is running at PASSIVE_LEVEL, skip to step 6.
5)  Queue a work item onto the special power thread.
6)  Attempt to acquire the state machine lock, with a near-zero-length timeout.
7)  If successful, skip to step 9.
8)  Queue a work item onto the special power thread.
9)  Acquire the state machine lock.
10) Acquire the Power queue lock.
11) Attempt to dequeue an event.
12) Drop the Power queue lock.
13) If there was no event to dequeue, drop the state machine lock and exit.
14) Execute the state handler.  This may involve taking one of the other state
    machine queue locks, briefly, to deliver an event.
15) Go to Step 10.

Implementing this algorithm requires three functions.

PowerProcessEvent         -- Implements steps 1-8.
_PowerProcessEventInner   -- Implements step 9.
PowerProcessEventInner    -- Implements steps 10-15.

--*/

VOID
FxPkgPnp::PowerProcessEvent(
    __in FxPowerEvent Event,
    __in BOOLEAN ProcessOnDifferentThread
    )
/*++

Routine Description:
    This function implements steps 1-8 of the algorithm described above.

Arguments:
    Event - Current Power event

    ProcessOnDifferentThread - Process the event on a different thread
        regardless of IRQL. By default this is FALSE as per the declaration.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
    KIRQL irql;

    //
    // Take the lock, raising to DISPATCH_LEVEL.
    //
    m_PowerMachine.Lock(&irql);

    //
    // If the input Event is any of the events described by PowerSingularEventMask,
    // then check whether it is already queued up. If so, then dont enqueue this
    // Event.
    //
    if (Event & PowerSingularEventMask) {
        if ((m_PowerMachine.m_SingularEventsPresent & Event) == 0x00) {
            m_PowerMachine.m_SingularEventsPresent |= Event;
        }
        else {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
                "WDFDEVICE 0x%p !devobj 0x%p current pwr pol state "
                "%!WDF_DEVICE_POWER_STATE! dropping event %!FxPowerEvent! because "
                "the Event is already enqueued.", m_Device->GetHandle(),
                m_Device->GetDeviceObject(),
                m_Device->GetDevicePowerState(),
                Event);

            m_PowerMachine.Unlock(irql);
            return;
        }
    }

    if (m_PowerMachine.IsFull()) {
        //
        // The queue is full.  Bail.
        //
        m_PowerMachine.Unlock(irql);

        ASSERT(!"The Power queue is full.  This shouldn't be able to happen.");
        return;
    }

    if (m_PowerMachine.IsClosedLocked()) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
            "WDFDEVICE 0x%p !devobj 0x%p current pwr pol state "
            "%!WDF_DEVICE_POWER_STATE! dropping event %!FxPowerEvent! because "
            "of a closed queue", m_Device->GetHandle(),
            m_Device->GetDeviceObject(),
            m_Device->GetDevicePowerState(),
            Event);

        //
        // The queue is closed.  Bail
        //
        m_PowerMachine.Unlock(irql);

        return;
    }

    //
    // Enqueue the event.  Whether the event goes on the front
    // or the end of the queue depends on which event it is.
    //
    if (Event & PowerPriorityEventsMask) {
        //
        // Stick it on the front of the queue, making it the next
        // event that will be processed.
        //
        m_PowerMachine.m_Queue.Events[m_PowerMachine.InsertAtHead()] = (USHORT) Event;
    }
    else {
        //
        // Stick it on the end of the queue.
        //
        m_PowerMachine.m_Queue.Events[m_PowerMachine.InsertAtTail()] = (USHORT) Event;
    }

    //
    // Drop the lock.
    //
    m_PowerMachine.Unlock(irql);

    //
    // Now, if we are running at PASSIVE_LEVEL, attempt to run the state
    // machine on this thread.  If we can't do that, then queue a work item.
    //

    if (irql == PASSIVE_LEVEL &&
        FALSE == ProcessOnDifferentThread) {
        LONGLONG timeout = 0;

        status = m_PowerMachine.m_StateMachineLock.AcquireLock(
            GetDriverGlobals(), &timeout);

        if (FxWaitLockInternal::IsLockAcquired(status)) {
            FxPostProcessInfo info;

            //
            // We now hold the state machine lock.  So call the function that
            // dispatches the next state.
            //
            PowerProcessEventInner(&info);

            //
            // The pnp state machine should be the only one deleting the object
            //
            ASSERT(info.m_DeleteObject == FALSE);

            m_PowerMachine.m_StateMachineLock.ReleaseLock(GetDriverGlobals());

            info.Evaluate(this);

            return;
        }
    }

    //
    // The tag added above will be released when the work item runs
    //

    // For one reason or another, we couldn't run the state machine on this
    // thread.  So queue a work item to do it.  If m_PnPWorkItemEnqueuing
    // is non-zero, that means that the work item is already being enqueued
    // on another thread.  This is significant, since it means that we can't do
    // anything with the work item on this thread, but it's okay, since the
    // work item will pick up our work and do it.
    //
    m_PowerMachine.QueueToThread();
}

VOID
FxPkgPnp::_PowerProcessEventInner(
    __in FxPkgPnp* This,
    __in FxPostProcessInfo* Info,
    __in PVOID WorkerContext
    )
{

    UNREFERENCED_PARAMETER(WorkerContext);

    //
    // Take the state machine lock.
    //
    This->m_PowerMachine.m_StateMachineLock.AcquireLock(
        This->GetDriverGlobals()
        );

    //
    // Call the function that will actually run the state machine.
    //
    This->PowerProcessEventInner(Info);

    //
    // We are being called from the work item and m_WorkItemRunning is > 0, so
    // we cannot be deleted yet.
    //
    ASSERT(Info->SomethingToDo() == FALSE);

    //
    // Now release the lock
    //
    This->m_PowerMachine.m_StateMachineLock.ReleaseLock(
        This->GetDriverGlobals()
        );
}

VOID
FxPkgPnp::PowerProcessEventInner(
    __inout FxPostProcessInfo* Info
    )
/*++

Routine Description:
    This routine runs the state machine.  It implements steps 10-15 of the
    algorithm described above.

--*/
{
    WDF_DEVICE_POWER_STATE currentPowerState, newState;
    CPPOWER_STATE_TABLE entry;
    FxPowerEvent event;
    KIRQL       oldIrql;

    //
    // Process as many events as we can.
    //
    for (;;) {

        newState = WdfDevStatePowerNull;
        currentPowerState = m_Device->GetDevicePowerState();
        entry = GetPowerTableEntry(currentPowerState);

        //
        // Get an event from the queue.
        //
        m_PowerMachine.Lock(&oldIrql);

        if (m_PowerMachine.IsEmpty()) {
            m_PowerMachine.GetFinishedState(Info);

            //
            // The queue is empty.
            //
            m_PowerMachine.Unlock(oldIrql);
            return;
        }

        event = (FxPowerEvent) m_PowerMachine.m_Queue.Events[m_PowerMachine.GetHead()];

        //
        // At this point, we need to determine whether we can process this
        // event.
        //
        if (event & PowerPriorityEventsMask) {
            //
            // These are always possible to handle.
            //
            DO_NOTHING();
        }
        else {
            //
            // Check to see if this state can handle new events.
            //
            if (entry->StateInfo.Bits.QueueOpen == FALSE) {
                //
                // This state can't handle new events.
                //
                m_PowerMachine.Unlock(oldIrql);
                return;
            }
        }

        //
        // If the event obtained from the queue was a singular event, then
        // clear the flag to allow other similar events to be put into this
        // queue for processing.
        //
        if (m_PowerMachine.m_SingularEventsPresent & event) {
           m_PowerMachine.m_SingularEventsPresent &= ~event;
        }

        m_PowerMachine.IncrementHead();
        m_PowerMachine.Unlock(oldIrql);

        //
        // Find the entry in the power state table that corresponds to this event
        //
        if (entry->FirstTargetState.PowerEvent == event) {
            newState = entry->FirstTargetState.TargetState;

            DO_EVENT_TRAP(&entry->FirstTargetState);
        }
        else if (entry->OtherTargetStates != NULL) {
            ULONG i = 0;

            for (i = 0;
                 entry->OtherTargetStates[i].PowerEvent != PowerEventMaximum;
                 i++) {
                if (entry->OtherTargetStates[i].PowerEvent == event) {
                    newState = entry->OtherTargetStates[i].TargetState;
                    DO_EVENT_TRAP(&entry->OtherTargetStates[i]);
                    break;
                }
            }
        }

        if (newState == WdfDevStatePowerNull) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                "WDFDEVICE 0x%p !devobj 0x%p current power state "
                "%!WDF_DEVICE_POWER_STATE! dropping event %!FxPowerEvent!",
                m_Device->GetHandle(),
                m_Device->GetDeviceObject(),
                m_Device->GetDevicePowerState(), event);

            //
            // This state doesn't respond to the Event.  Potentially throw
            // the event away.
            //
            if ((entry->StateInfo.Bits.KnownDroppedEvents & event) == 0) {
                COVERAGE_TRAP();

                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGPNP,
                    "WDFDEVICE %p !devobj 0x%p current state "
                    "%!WDF_DEVICE_POWER_STATE! event %!FxPowerEvent! is not a "
                    "known dropped event, known dropped events are "
                    "%!FxPowerEvent!", m_Device->GetHandle(),
                    m_Device->GetDeviceObject(),
                    m_Device->GetDevicePowerState(),
                    event, entry->StateInfo.Bits.KnownDroppedEvents);


            }

            switch (event) {
            case PowerWakeSucceeded:
            case PowerWakeFailed:
            case PowerWakeCanceled:
                //
                // There are states where we don't care if the wake completed.
                // Since the completion/cancellation of the wake request posts
                // an event which it assumes will complete the request, we must
                // catch these events here and complete the request.
                //
                PowerCompletePendedWakeIrp();
                break;

            case PowerD0:
            case PowerDx:
                //
                // There are some (non WDF) power policy owner implementations
                // which send Dx to Dx or D0 to D0 transitions to the stack.
                //
                // We don't explicitly handle them in the state machine.
                //
                // Instead, we complete the pended irp if are about to drop it
                // on the floor.
                //
                PowerReleasePendingDeviceIrp();
                break;
            }
        }
        else {
            //
            // Now enter the new state.
            //
            PowerEnterNewState(newState);
        }
    }
}

VOID
FxPkgPnp::PowerEnterNewState(
    __in WDF_DEVICE_POWER_STATE State
    )
/*++

Routine Description:
    This function looks up the handler for a state and
    then calls it.

Arguments:
    Event - Current PnP event

Return Value:

    NTSTATUS

--*/
{
    CPPOWER_STATE_TABLE    entry;
    WDF_DEVICE_POWER_STATE currentState, newState;
    WDF_DEVICE_POWER_NOTIFICATION_DATA data;
    FxWatchdog watchdog(this);

    currentState = m_Device->GetDevicePowerState();
    newState = State;

    while (newState != WdfDevStatePowerNull) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNPPOWERSTATES,
            "WDFDEVICE 0x%p !devobj 0x%p entering Power State "
            "%!WDF_DEVICE_POWER_STATE! from %!WDF_DEVICE_POWER_STATE!",
            m_Device->GetHandle(),
            m_Device->GetDeviceObject(),
            newState, currentState);

        if (m_PowerStateCallbacks != NULL) {
            //
            // Callback for leaving the old state
            //
            RtlZeroMemory(&data, sizeof(data));

            data.Type = StateNotificationLeaveState;
            data.Data.LeaveState.CurrentState = currentState;
            data.Data.LeaveState.NewState = newState;

            m_PowerStateCallbacks->Invoke(currentState,
                                          StateNotificationLeaveState,
                                          m_Device->GetHandle(),
                                          &data);
        }

        m_PowerMachine.m_States.History[m_PowerMachine.IncrementHistoryIndex()] =
            (USHORT) newState;

        if (m_PowerStateCallbacks != NULL) {
            //
            // Callback for entering the new state
            //
            RtlZeroMemory(&data, sizeof(data));

            data.Type = StateNotificationEnterState;
            data.Data.EnterState.CurrentState = currentState;
            data.Data.EnterState.NewState = newState;

            m_PowerStateCallbacks->Invoke(newState,
                                          StateNotificationEnterState,
                                          m_Device->GetHandle(),
                                          &data);
        }

        m_Device->SetDevicePowerState(newState);
        currentState = newState;

        entry = GetPowerTableEntry(currentState);

        //
        // And call the state handler, if there is one.
        //
        if (entry->StateFunc != NULL) {
            watchdog.StartTimer(currentState);
            newState = entry->StateFunc(this);
            watchdog.CancelTimer(currentState);

            //
            // Validate the return value if FX_STATE_MACHINE_VERIFY is enabled
            //
            VALIDATE_POWER_STATE(currentState, newState);

        }
        else {
            newState = WdfDevStatePowerNull;
        }

        if (m_PowerStateCallbacks != NULL) {
            //
            // Callback for post processing the new state
            //
            RtlZeroMemory(&data, sizeof(data));

            data.Type = StateNotificationPostProcessState;
            data.Data.PostProcessState.CurrentState = currentState;

            m_PowerStateCallbacks->Invoke(currentState,
                                          StateNotificationPostProcessState,
                                          m_Device->GetHandle(),
                                          &data);
        }
    }
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerCheckParentState(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Check Parent state.  Its only task
    is to dispatch to the FDO and PDO logic and handle error.

Arguments:
    none

Return Value:

    new power state

--*/
{
    NTSTATUS status;
    BOOLEAN parentOn;

    status = This->PowerCheckParentOverload(&parentOn);

    if (!NT_SUCCESS(status)) {
        return WdfDevStatePowerUpFailed;
    }
    else if (parentOn) {
        return WdfDevStatePowerWaking;
    }
    else {
        return WdfDevStatePowerWaitForParent;
    }
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerCheckParentStateNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Check Parent (NP) state.  Its only task
    is to dispatch to the FDO and PDO logic.

Arguments:
    none

Return Value:

    new power state

--*/
{
    NTSTATUS status;
    BOOLEAN parentOn;

    status = This->PowerCheckParentOverload(&parentOn);

    if (!NT_SUCCESS(status)) {
        return WdfDevStatePowerUpFailedNP;
    }
    else if (parentOn) {
        return WdfDevStatePowerWakingNP;
    }
    else {
        return WdfDevStatePowerWaitForParentNP;
    }
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerCheckDeviceType(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Check Type state.  Its only task
    is to dispatch to the FDO and PDO logic.

Arguments:
    none

Return Value:

    new power state

--*/
{
    return This->PowerCheckDeviceTypeOverload();
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerCheckDeviceTypeNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Check Type (NP) state.  Its only task
    is to dispatch to the FDO and PDO logic.

Arguments:
    none

Return Value:

    new power state

--*/
{
    return This->PowerCheckDeviceTypeNPOverload();
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerEnablingWakeAtBus(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function requests the driver to arm the device in a bus generic fashion.

Arguments:
    The package which contains this instance of the state machine

Return Value:
    new power state

  --*/
{
    NTSTATUS status;

    //
    // We should only get into this state when this devobj is not a PDO and a
    // power policy owner.
    //
    ASSERT((This->m_Device->IsPdo() && This->IsPowerPolicyOwner()) == FALSE);

    status = This->PowerEnableWakeAtBusOverload();

    ASSERT(status != STATUS_CANCELLED);

    if (NT_SUCCESS(status)) {
        //
        // No matter of the irp status (canceled, pending, completed), we always
        // transition to the D0ArmedForWake state because that is where we
        // we handle the change in the irp's status.
        //
        return WdfDevStatePowerD0ArmedForWake;
    }
    else {
        This->PowerCompleteWakeRequestFromWithinMachine(status);

        return WdfDevStatePowerD0BusWakeOwner;
    }
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerEnablingWakeAtBusNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function requests the driver to arm the device in a bus generic fashion.

Arguments:
    The package which contains this instance of the state machine

Return Value:
    new power state

  --*/
{
    NTSTATUS status;

    //
    // We should only get into this state when this devobj is not a PDO and a
    // power policy owner.
    //
    ASSERT((This->m_Device->IsPdo() &&
        This->IsPowerPolicyOwner()) == FALSE);

    status = This->PowerEnableWakeAtBusOverload();

    ASSERT(status != STATUS_CANCELLED);

    if (NT_SUCCESS(status)) {
        //
        // No matter of the irp status (canceled, pending, completed), we always
        // transition to the D0ArmedForWake state because that is where we
        // we handle the change in the irp's status.
        //
        return WdfDevStatePowerD0ArmedForWakeNP;
    }
    else {
        //
        // Complete the irp with the error that callback indicated
        //
        COVERAGE_TRAP();

        This->PowerCompleteWakeRequestFromWithinMachine(status);

        return WdfDevStatePowerD0BusWakeOwnerNP;
    }
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerDZero(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    D0 state where we cannot wake the machine

Arguments:
    This - Instance of this state machine

Return Value:
    new power state machine state

  --*/
{
    if ((This->m_Device->GetDeviceObjectFlags() & DO_POWER_PAGABLE) == 0) {
        //
        // We are non pageable, go to that state now
        //
        COVERAGE_TRAP();

        return WdfDevStatePowerDecideD0State;
    }

    return WdfDevStatePowerNull;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerD0NP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    D0 state where we can't cause paging IO and we cannot wake the machine

Arguments:
    This - Instance of this state machine

Return Value:
    new power state machine state

  --*/
{
    if (This->m_Device->GetDeviceObjectFlags() & DO_POWER_PAGABLE) {
        //
        // We are pageable, go to that state now
        //
        COVERAGE_TRAP();
        return WdfDevStatePowerDecideD0State;
    }

    return WdfDevStatePowerNull;
}


WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerD0BusWakeOwner(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the D0 state.  It's job is to figure out whether
    we need to swtich to the D0NP state, and to wait around for an event
    of some kind or another.

Arguments:
    none

Return Value:

    new power state

--*/
{
    if ((This->m_Device->GetDeviceObjectFlags() & DO_POWER_PAGABLE) == 0) {
        //
        // We are non pageable, go to that state now
        //
        COVERAGE_TRAP();

        return WdfDevStatePowerDecideD0State;
    }
    else if (This->PowerIsWakeRequestPresent()) {
        return WdfDevStatePowerEnablingWakeAtBus;
    }
    else {
        return WdfDevStatePowerNull;
    }
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerD0BusWakeOwnerNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the D0NP state.  It's job is to figure out whether
    we need to swtich to the D0 state, and to wait around for an event
    of some kind or another.

Arguments:
    none

Return Value:

    new power state

--*/
{
    if (This->m_Device->GetDeviceObjectFlags() & DO_POWER_PAGABLE) {
        //
        // Pageable.
        //
        COVERAGE_TRAP();

        return WdfDevStatePowerDecideD0State;
    }
    else if (This->PowerIsWakeRequestPresent()) {
        return WdfDevStatePowerEnablingWakeAtBusNP;
    }
    else {
        return WdfDevStatePowerNull;
    }
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerD0ArmedForWake(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Device is in D0 and armed for wake.  Complete any pended D0 irp if the power
    policy owner make a D0 to D0 transition.  Transition the NP version of
    this state if we are no longer pageable.

Arguments:
    This - instance of the state machine

Return Value:
    new state machine state

  --*/
{
    if ((This->m_Device->GetDeviceObjectFlags() & DO_POWER_PAGABLE) == 0) {
        //
        // We are non pageable, go to that state now
        //
        COVERAGE_TRAP();

        return WdfDevStatePowerDecideD0State;
    }

    return WdfDevStatePowerNull;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerD0ArmedForWakeNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Device is in D0 and armed for wake.  Complete any pended D0 irp if the power
    policy owner make a D0 to D0 transition.  Transition the pageable version of
    this state if we are no longer NP.

Arguments:
    This - instance of the state machine

Return Value:
    new state machine state

  --*/
{
    if (This->m_Device->GetDeviceObjectFlags() & DO_POWER_PAGABLE) {
        //
        // We are pageable, go to that state now
        //
        COVERAGE_TRAP();

        return WdfDevStatePowerDecideD0State;
    }

    return WdfDevStatePowerNull;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerGotoImplicitD3DisarmWakeAtBus(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Disarms the bus after we have armed it.  The device is going to implicit D3
    and it has not yet been powered down.

Arguments:
    This - instance of the state machine

Return Value:
    new state machine state

  --*/
{
    //
    // We should only get into this state when this devobj is a PDO and not a
    // power policy owner.
    //
    ASSERT((This->m_Device->IsPdo() &&
        This->IsPowerPolicyOwner()) == FALSE);

    //
    // Disarm
    // No need to complete the pended ww irp. State machine will complete it
    // in PnpFailed handler, or upper driver will cancel it.
    //
    This->PowerDisableWakeAtBusOverload();

    return WdfDevStatePowerGotoD3Stopped;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerD0DisarmingWakeAtBus(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Disarms the bus after we have armed it.  The device is still in D0 so it has
    not yet powered down.

Arguments:
    This - This instance of the state machine

Return Value:
    None.

  --*/
{
    //
    // We should only get into this state when this devobj is not a PDO and a
    // power policy owner.
    //
    ASSERT((This->m_Device->IsPdo() &&
        This->IsPowerPolicyOwner()) == FALSE);

    //
    // Disarm
    //
    This->PowerDisableWakeAtBusOverload();
    This->PowerCompletePendedWakeIrp();

    //
    // Go back to normal unarmed D0 with bus wake ownership
    //
    return WdfDevStatePowerD0BusWakeOwner;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerD0DisarmingWakeAtBusNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Disarms the bus after we have armed it.  The device is still in D0 so it has
    not yet powered down.

Arguments:
    This - This instance of the state machine

Return Value:
    None.

  --*/
{
    //
    // We should only get into this state when this devobj is not a PDO and a
    // power policy owner.
    //
    ASSERT((This->m_Device->IsPdo() && This->IsPowerPolicyOwner()) == FALSE);

    //
    // Disarm
    //
    This->PowerDisableWakeAtBusOverload();
    This->PowerCompletePendedWakeIrp();

    //
    // Go back to normal unarmed D0 with bus wake ownership
    //
    return WdfDevStatePowerD0BusWakeOwnerNP;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerD0Starting(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function calls D0Entry and the moves to the next state based on the
    result.

Arguments:
    This - instance of the state machine

Return Value:
    new power state

--*/
{
    NTSTATUS    status;

    //
    // Call the driver to tell it to put the hardware into the working
    // state.
    //
    // m_DevicePowerState is the "old" state because we update it after the
    // D0Entry callback.
    //
    status = This->m_DeviceD0Entry.Invoke(
        This->m_Device->GetHandle(),
        (WDF_POWER_DEVICE_STATE) This->m_DevicePowerState);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "EvtDeviceD0Entry WDFDEVICE 0x%p !devobj 0x%p,  old state "
            "%!WDF_POWER_DEVICE_STATE! failed, %!STATUS!",
            This->m_Device->GetHandle(),
            This->m_Device->GetDeviceObject(),
            This->m_DevicePowerState, status);

        return WdfDevStatePowerInitialPowerUpFailedDerefParent;
    }

    return WdfDevStatePowerD0StartingConnectInterrupt;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerD0StartingConnectInterrupt(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Continues bringing the device into D0 from the D3 final state.  This routine
    connects and enables the interrupts.  If successful it will open up the power
    managed i/o queues.

Arguments:
    This - instance of the state machine

Return Value:
    new state machine state

  --*/
{
    NTSTATUS status;

    //
    // Connect the interrupt and enable it
    //
    status = This->NotifyResourceObjectsD0(NotifyResourcesNoFlags);
    if (!NT_SUCCESS(status)) {
        //
        // NotifyResourceObjectsD0 has already logged the error, no need to
        // repeat any error messsages here
        //
        return WdfDevStatePowerInitialConnectInterruptFailed;
    }

    status = This->m_DeviceD0EntryPostInterruptsEnabled.Invoke(
        This->m_Device->GetHandle(),
        (WDF_POWER_DEVICE_STATE) This->m_DevicePowerState);

    if (!NT_SUCCESS(status)) {

        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "EvtDeviceD0EntryPostInterruptsEnabed WDFDEVICE 0x%p !devobj 0x%p, "
            "old state %!WDF_POWER_DEVICE_STATE! failed, %!STATUS!",
            This->m_Device->GetHandle(),
            This->m_Device->GetDeviceObject(),
            This->m_DevicePowerState, status);
        return WdfDevStatePowerInitialConnectInterruptFailed;
    }

    //
    // Last, figure out which state to drop into.  This is the juncture
    // where we figure out if we're doing power in a pageable fashion.
    //
    return WdfDevStatePowerD0StartingDmaEnable;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerD0StartingDmaEnable(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    The device is movig to D0 for the first time.  Enable any DMA enablers
    attached to the device.

Arguments:
    This - instance of the state machine

Return Value:
    new machine state

  --*/
{
    if (This->PowerDmaEnableAndScan(TRUE) == FALSE) {
        return WdfDevStatePowerInitialDmaEnableFailed;
    }

    return WdfDevStatePowerD0StartingStartSelfManagedIo;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerD0StartingStartSelfManagedIo(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    The device is entering D0 from the final Dx state (either start or restart
    perhaps).  Send a start event to self managed io and then release

Arguments:
    This - instance of the state machine

Return Value:
    new state machine state

  --*/
{




    This->m_Device->m_PkgIo->ResumeProcessingForPower();

    if (This->m_SelfManagedIoMachine != NULL) {
        NTSTATUS status;

        status = This->m_SelfManagedIoMachine->Start();

        if (!NT_SUCCESS(status)) {
            // return WdfDevStatePowerInitialSelfManagedIoFailed; __REACTOS__ : allow to fail
        }
    }

    This->PowerSetDevicePowerState(WdfPowerDeviceD0);

    //
    // Send the PowerUp event to both the PnP and the Power Policy state machines.
    //
    This->PowerSendPowerUpEvents();

    return WdfDevStatePowerDecideD0State;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerDecideD0State(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Decide which D0 state we should transition to given the wait wake ownership
    of this device and if DO_POWER_PAGABLE is set or not.

Arguments:
    This - instance of the state machine

Return Value:
    new power state

  --*/
{
    if (This->m_Device->GetDeviceObjectFlags() & DO_POWER_PAGABLE) {
        //
        // Pageable.
        //
        if (This->m_SharedPower.m_WaitWakeOwner) {
            return WdfDevStatePowerD0BusWakeOwner;
        }
        else {
            return WdfDevStatePowerD0;
        }
    }
    else {
        //
        // Non-pageable.
        //
        if (This->m_SharedPower.m_WaitWakeOwner) {
            return WdfDevStatePowerD0BusWakeOwnerNP;
        }
        else {
            return WdfDevStatePowerD0NP;
        }
    }
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerGotoD3Stopped(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the D3 Stopped state.

Arguments:
    none

Return Value:

    new power state

--*/
{
    NTSTATUS status;
    BOOLEAN failed;

    failed = FALSE;

    //
    // We *must* call suspend here even though the pnp state machine called self
    // managed io stop.  Consider the following:
    // 1 this device is a filter
    // 2 the power policy owner has idle enabled and the device stack is
    //   currently idled out (in Dx)
    // 3 the query remove comes, this driver processes it and succeeds
    //   self managed io stop
    // 4 before the PwrPolStop event is processed in this driver, the pwr policy
    //   owner moves the stack into D0.
    // 5 now this driver processed the PwrPolStop and moves into this state.  We
    //   now need to make sure self managed i/o is in the stopped state before
    //   doing anything else.
    //

    //
    // The self managed io state machine can handle a suspend event when it is
    // already in the stopped state.
    //
    // Tell the driver to stop its self-managed I/O.
    //
    if (This->m_SelfManagedIoMachine != NULL) {
        status = This->m_SelfManagedIoMachine->Suspend();

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "EvtDeviceSelfManagedIoStop failed %!STATUS!", status);
            failed = TRUE;
        }
    }





    // Top-edge queue hold.
    This->m_Device->m_PkgIo->StopProcessingForPower(FxIoStopProcessingForPowerHold);

    if (This->PowerDmaPowerDown() == FALSE) {
        failed = TRUE;
    }

    status = This->m_DeviceD0ExitPreInterruptsDisabled.Invoke(
        This->m_Device->GetHandle(),
        WdfPowerDeviceD3Final
        );

    if (!NT_SUCCESS(status)) {
        failed = TRUE;

        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "EvtDeviceD0ExitPreInterruptsDisabled WDFDEVICE 0x%p !devobj 0x%p, "
            "new state %!WDF_POWER_DEVICE_STATE! failed, %!STATUS!",
            This->m_Device->GetHandle(),
            This->m_Device->GetDeviceObject(),
            WdfPowerDeviceD3Final, status);
    }

    //
    // Disconnect the interrupt.
    //
    status = This->NotifyResourceObjectsDx(NotifyResourcesForceDisconnect);
    if (!NT_SUCCESS(status)) {
        //
        // NotifyResourceObjectsDx already traced the error
        //
        failed = TRUE;
    }

    //
    // Call the driver to tell it to put the hardware into a sleeping
    // state.
    //
    status = This->m_DeviceD0Exit.Invoke(This->m_Device->GetHandle(),
                                         WdfPowerDeviceD3Final);
    if (!NT_SUCCESS(status)) {
        failed = TRUE;

        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "EvtDeviceD0Exit WDFDEVICE 0x%p !devobj 0x%p, new state "
            "%!WDF_POWER_DEVICE_STATE! failed, %!STATUS!",
            This->m_Device->GetHandle(),
            This->m_Device->GetDeviceObject(),
            WdfPowerDeviceD3Final, status);
    }

    This->PowerSetDevicePowerState(WdfPowerDeviceD3Final);

    //
    // If this is a child, release the power reference on the parent
    //
    This->PowerParentPowerDereference();

    if (failed) {
        return WdfDevStatePowerFinalPowerDownFailed;
    }

    This->PowerSendPowerDownEvents(FxPowerDownTypeImplicit);

    //
    // If we are not the PPO for the stack we could receive a power irp
    // during the middle of an implicit power down so we cannot assume
    // that there will be no pended power irp during an implicit power down.
    //
    ASSERT(This->IsPowerPolicyOwner() ? This->m_PendingDevicePowerIrp == NULL  : TRUE);

    return WdfDevStatePowerStopped;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerStartingCheckDeviceType(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    The device is implicitly powering up from the created or stopped state.
    Determine if this is a PDO or not to determine if we must bring the parent
    back into D0.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerStartingChild or WdfDevStatePowerD0Starting

  --*/
{
    if (This->m_Device->IsPdo()) {
        return WdfDevStatePowerStartingChild;
    }
    else {
        return WdfDevStatePowerD0Starting;
    }
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerStartingChild(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Get the parent into a D0 state

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerNull or WdfDevStatePowerD0Starting

  --*/
{
    NTSTATUS status;
    BOOLEAN parentOn;

    status = This->PowerCheckParentOverload(&parentOn);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "PowerReference on parent WDFDEVICE %p for child WDFDEVICE %p "
            "failed, %!STATUS!", This->m_Device->m_ParentDevice->m_Device->GetHandle(),
                This->m_Device->GetHandle(),
            status);

        return WdfDevStatePowerInitialPowerUpFailed;
    }
    else if (parentOn) {
        //
        // Parent is powered on, start the power up sequence
        //
        return WdfDevStatePowerD0Starting;
    }
    else {
        //
        // The call to PowerReference will bring the parent into D0 and
        // move us out of this state after we return.
        //
        return WdfDevStatePowerNull;
    }
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerDxDisablingWakeAtBus(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Disable Wake at Bus state.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerWaking

--*/
{
    //
    // We should only get into this state when this devobj is not a PDO and a
    // power policy owner.
    //
    ASSERT((This->m_Device->IsPdo() && This->IsPowerPolicyOwner()) == FALSE);

    This->PowerDisableWakeAtBusOverload();

    return WdfDevStatePowerWaking;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerDxDisablingWakeAtBusNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Disable Wake at Bus state.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerWakingNP

--*/
{
    //
    // We should only get into this state when this devobj is not a PDO and a
    // power policy owner.
    //
    ASSERT((This->m_Device->IsPdo() && This->IsPowerPolicyOwner()) == FALSE);

    This->PowerDisableWakeAtBusOverload();

    return WdfDevStatePowerWakingNP;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerGotoDNotZero(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    State where we go into Dx in the pageable path.

Arguments:
    The instance of this state machine

Return Value:
    new power state

  --*/
{
    This->PowerGotoDx();

    return WdfDevStatePowerNull;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerGotoDNotZeroNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    State where we go into Dx in the NP path.

Arguments:
    The instance of this state machine

Return Value:
    new power state

  --*/
{
    This->PowerGotoDx();

    return WdfDevStatePowerNull;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerGotoDNotZeroIoStopped(
    __inout FxPkgPnp*   This
    )
{
    if (This->PowerGotoDxIoStopped() == FALSE) {
        return WdfDevStatePowerGotoDxFailed;
    }

    return WdfDevStatePowerDx;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerGotoDNotZeroIoStoppedNP(
    __inout FxPkgPnp*   This
    )
{
    if (This->PowerGotoDxIoStoppedNP() == FALSE) {
        return WdfDevStatePowerGotoDxNPFailed;
    }

    return WdfDevStatePowerDxNP;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerGotoDxNPFailed(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Going to Dx in the NP path failed.  Disconnect all the interrupts.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerReportPowerDownFailed

  --*/
{
    This->DisconnectInterruptNP();

    return WdfDevStatePowerReportPowerDownFailed;
}

VOID
FxPkgPnp::PowerGotoDx(
    VOID
    )
/*++

Routine Description:
    Implements the going into Dx logic for the pageable path.

Arguments:
    None

Return Value:
    None

  --*/
{
    if (m_SelfManagedIoMachine != NULL) {
        NTSTATUS    status;

        //
        // Tell the driver to stop its self-managed I/O
        //
        status = m_SelfManagedIoMachine->Suspend();

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "EvtDeviceSelfManagedIoStop failed %!STATUS!", status);

            m_PowerMachine.m_IoCallbackFailure = TRUE;
        }
    }





    // Top-edge queue hold
    m_Device->m_PkgIo->StopProcessingForPower(FxIoStopProcessingForPowerHold);

    PowerPolicyProcessEvent(PwrPolPowerDownIoStopped);
}

BOOLEAN
FxPkgPnp::PowerGotoDxIoStopped(
    VOID
    )
/*++

Routine Description:
    Implements the going into Dx logic for the pageable path.



Arguments:
    None

Return Value:
    TRUE if the power down succeeded, FALSE otherwise

  --*/
{
    WDF_POWER_DEVICE_STATE state;
    NTSTATUS    status;
    BOOLEAN     failed;
    FxIrp   irp;
    ULONG   notifyFlags;

    failed = FALSE;

    //
    // First determine the state that will be indicated to the driver
    //
    irp.SetIrp(m_PendingDevicePowerIrp);

    switch (irp.GetParameterPowerShutdownType()) {
    case PowerActionShutdown:
    case PowerActionShutdownReset:
    case PowerActionShutdownOff:
        state = WdfPowerDeviceD3Final;
        break;

    default:
        state = (WDF_POWER_DEVICE_STATE) irp.GetParameterPowerStateDeviceState();
        break;
    }

    //
    // Can we even be a power pageable device and be in hibernation path?
    //
    if (m_SystemPowerState == PowerSystemHibernate &&
        GetUsageCount(WdfSpecialFileHibernation) != 0) {
        COVERAGE_TRAP();

        //
        // This device is in the hibernation path and the target system state is
        // S4.  Tell the driver that it should do special handling.
        //
        state = WdfPowerDevicePrepareForHibernation;
    }

    if (PowerDmaPowerDown() == FALSE) {
        failed = TRUE;
    }

    status = m_DeviceD0ExitPreInterruptsDisabled.Invoke(
        m_Device->GetHandle(),
        state
        );

    if (!NT_SUCCESS(status)) {
        failed = TRUE;

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "EvtDeviceD0ExitPreInterruptsDisabled WDFDEVICE 0x%p !devobj 0x%p, "
            "new state %!WDF_POWER_DEVICE_STATE! failed, %!STATUS!",
            m_Device->GetHandle(),
            m_Device->GetDeviceObject(), state, status);
    }

    //
    // interrupt disable & disconnect
    //

    notifyFlags = NotifyResourcesExplicitPowerDown;

    //
    // In general, m_WaitWakeIrp is accessed through guarded InterlockedExchange
    // operations. However, following is a special case where we just want to know
    // the current value. It is possible that the value of m_WaitWakeIrp can
    // change right after we query it. Users of NotifyResourcesArmedForWake will
    // need to be aware of this fact.
    //
    // Note that relying on m_WaitWakeIrp to decide whether to disconnect the wake
    // interrupts or not is unreliable and may result in a race condition between
    // the device powering down and a wake interrupt firing:
    //
    // Thread A: Device is powering down and is going to disconnect wake interrupts
    //           unless m_WaitWakeIrp is not NULL.
    // Thread B: Wake interrupt fires (holding the OS interrupt lock) which results
    //           in completing the IRP_MN_WAIT_WAKE and setting m_WaitWakeIrp to NULL.
    //           Thread then blocks waiting for the device to power up.
    // Thread A: m_WaitWakeIrp is NULL so we disconnect the wake interrupt, but are
    //           blocked waiting to acquire the lock held by the ISR. The deadlock
    //           results in bugcheck 0x9F since the Dx IRP is being blocked.
    //
    // The m_WakeInterruptsKeepConnected flag is set when we request a IRP_MN_WAIT_WAKE
    // in the device powering down path, and is cleared below once it is used.
    //
    if (m_SharedPower.m_WaitWakeIrp != NULL || m_WakeInterruptsKeepConnected == TRUE) {
        notifyFlags |= NotifyResourcesArmedForWake;
        m_WakeInterruptsKeepConnected = FALSE;
    }

    status = NotifyResourceObjectsDx(notifyFlags);
    if (!NT_SUCCESS(status)) {
        //
        // NotifyResourceObjectsDx already traced the error
        //
        failed = TRUE;
    }

    //
    // Call the driver to tell it to put the hardware into a sleeping
    // state.
    //

    status = m_DeviceD0Exit.Invoke(m_Device->GetHandle(), state);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "EvtDeviceD0Exit WDFEVICE 0x%p !devobj 0x%p, new state "
            "%!WDF_POWER_DEVICE_STATE! failed, %!STATUS!",
            m_Device->GetHandle(),
            m_Device->GetDeviceObject(), state, status);

        failed = TRUE;
    }

    //
    // If this is a child, release the power reference on the parent
    //
    PowerParentPowerDereference();

    //
    // Set our state no matter if power down failed or not
    //
    PowerSetDevicePowerState(state);

    //
    // Stopping self managed io previously failed, convert that failure into
    // a local failure here.
    //
    if (m_PowerMachine.m_IoCallbackFailure) {
        m_PowerMachine.m_IoCallbackFailure = FALSE;
        failed = TRUE;
    }

    if (failed) {
        //
        // Power policy will use this property when it is processing the
        // completion of the Dx irp.
        //
        m_PowerMachine.m_PowerDownFailure = TRUE;

        //
        // This state will record that we encountered an internal error.
        //
        return FALSE;
    }

    PowerSendPowerDownEvents(FxPowerDownTypeExplicit);

    PowerReleasePendingDeviceIrp();

    return TRUE;
}

BOOLEAN
FxPkgPnp::PowerGotoDxIoStoppedNP(
    VOID
    )
/*++

Routine Description:
    This function implements going into the Dx state in the NP path.

Arguments:
    None

Return Value:
    TRUE if the power down succeeded, FALSE otherwise

  --*/
{
    WDF_POWER_DEVICE_STATE state;
    NTSTATUS    status;
    BOOLEAN     failed;
    FxIrp   irp;

    failed = FALSE;

    //
    // First determine the state that will be indicated to the driver
    //
    irp.SetIrp(m_PendingDevicePowerIrp);

    switch (irp.GetParameterPowerShutdownType()) {
    case PowerActionShutdown:
    case PowerActionShutdownReset:
    case PowerActionShutdownOff:
        state = WdfPowerDeviceD3Final;
        break;

    default:
        state = (WDF_POWER_DEVICE_STATE) irp.GetParameterPowerStateDeviceState();
        break;
    }

    if (m_SystemPowerState == PowerSystemHibernate &&
        GetUsageCount(WdfSpecialFileHibernation) != 0) {
        //
        // This device is in the hibernation path and the target system state is
        // S4.  Tell the driver that it should do special handling.
        //
        state = WdfPowerDevicePrepareForHibernation;
    }

    if (PowerDmaPowerDown()  == FALSE) {
        failed = TRUE;
    }

    status = m_DeviceD0ExitPreInterruptsDisabled.Invoke(
        m_Device->GetHandle(),
        state
        );

    if (!NT_SUCCESS(status)) {
        failed = TRUE;

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "EvtDeviceD0ExitPreInterruptsDisabled WDFDEVICE 0x%p !devobj 0x%p, "
            "new state %!WDF_POWER_DEVICE_STATE! failed, %!STATUS!",
            m_Device->GetHandle(),
            m_Device->GetDeviceObject(), state, status);
    }

    //
    // Interrupt disable (and NO disconnect)
    //
    status = NotifyResourceObjectsDx(NotifyResourcesNP);

    if (!NT_SUCCESS(status)) {
        //
        // NotifyResourceObjectsDx already traced the error
        //
        failed = TRUE;
    }

    //
    // Call the driver to tell it to put the hardware into a sleeping
    // state.
    //

    status = m_DeviceD0Exit.Invoke(m_Device->GetHandle(), state);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "EvtDeviceD0Exit WDFDEVICE 0x%p !devobj 0x%p, new state "
            "%!WDF_POWER_DEVICE_STATE! failed, %!STATUS!",
            m_Device->GetHandle(),
            m_Device->GetDeviceObject(), state, status);

        failed = TRUE;
    }

    //
    // If this is a child, release the power reference on the parent
    //
    PowerParentPowerDereference();

    //
    // Set our state no matter if power down failed or not
    //
    PowerSetDevicePowerState(state);

    //
    // Stopping self managed io previously failed, convert that failure into
    // a local failure here.
    //
    if (m_PowerMachine.m_IoCallbackFailure) {
        m_PowerMachine.m_IoCallbackFailure = FALSE;
        failed = TRUE;
    }

    if (failed) {
        //
        // Power policy will use this property when it is processing the
        // completion of the Dx irp.
        //
        m_PowerMachine.m_PowerDownFailure = TRUE;

        //
        // This state will record that we encountered an internal error.
        //
        return FALSE;
    }

    PowerSendPowerDownEvents(FxPowerDownTypeExplicit);

    PowerReleasePendingDeviceIrp();

    return TRUE;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerGotoDxArmedForWake(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Dx state when we are armed for wake.

Arguments:
    This - The instance of the state machine

Return Value:

    new power state

--*/
{
    This->PowerGotoDx();

    return WdfDevStatePowerNull;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerGotoDxArmedForWakeNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Dx state when we are armed for wake in the NP
    path.

Arguments:
    This - The instance of the state machine

Return Value:

    new power state

--*/
{
    This->PowerGotoDx();

    return WdfDevStatePowerNull;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerGotoDxIoStoppedArmedForWake(
    __inout FxPkgPnp*   This
    )
{
    if (This->PowerGotoDxIoStopped() == FALSE) {
        return WdfDevStatePowerGotoDxFailed;
    }

    return WdfDevStatePowerDxArmedForWake;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerGotoDxIoStoppedArmedForWakeNP(
    __inout FxPkgPnp*   This
    )
{
    if (This->PowerGotoDxIoStoppedNP() == FALSE) {
        return WdfDevStatePowerGotoDxNPFailed;
    }

    return WdfDevStatePowerDxArmedForWakeNP;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerCheckParentStateArmedForWake(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    The PDO was armed for wake in Dx and needs to be disarmed at the bus level.
    The child can only be disarmed while the parent is in D0, so check the state
    of the parent.  If in D0, move directly to the disarm state, otherwise move
    into a wait state and disarm once the parent is in D0.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/

{
    NTSTATUS status;
    BOOLEAN parentOn;

    status = This->PowerCheckParentOverload(&parentOn);

    if (!NT_SUCCESS(status)) {
        return WdfDevStatePowerUpFailed;
    }
    else if (parentOn) {
        return WdfDevStatePowerDxDisablingWakeAtBus;
    }
    else {
        return WdfDevStatePowerWaitForParentArmedForWake;
    }
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerCheckParentStateArmedForWakeNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Same as PowerCheckParentStateArmedForWake, but we are in the NP path

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    NTSTATUS status;
    BOOLEAN parentOn;

    status = This->PowerCheckParentOverload(&parentOn);

    if (!NT_SUCCESS(status)) {
        return WdfDevStatePowerUpFailedNP;
    }
    else if (parentOn) {
        return WdfDevStatePowerDxDisablingWakeAtBusNP;
    }
    else {
        return WdfDevStatePowerWaitForParentArmedForWakeNP;
    }
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerStartSelfManagedIo(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Start Self-Managed I/O state.  It tells the
    driver that it can resume operations that were not interlocked with
    the PnP and Power state machines.

Arguments:
    This - The instance of the state machine

Return Value:

    new power state

--*/
{





    // Top-edge queue release
    This->m_Device->m_PkgIo->ResumeProcessingForPower();

    if (This->m_SelfManagedIoMachine != NULL) {
        NTSTATUS    status;

        status = This->m_SelfManagedIoMachine->Start();

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "EvtDeviceSelfManagedIoRestart failed - %!STATUS!", status);

            return WdfDevStatePowerStartSelfManagedIoFailed;
        }
    }

    This->PowerSetDevicePowerState(WdfPowerDeviceD0);

    //
    // Send the PowerUp event to both the PnP and the Power Policy state
    // machines.
    //
    This->PowerSendPowerUpEvents();

    This->PowerReleasePendingDeviceIrp();

    if (This->m_SharedPower.m_WaitWakeOwner) {
        return WdfDevStatePowerD0BusWakeOwner;
    }
    else {
        return WdfDevStatePowerD0;
    }
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerStartSelfManagedIoNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Start Self-Managed I/O state.  It tells the
    driver that it can resume operations that were not interlocked with
    the PnP and Power state machines.

Arguments:
    This - The instance of the state machine

Return Value:

    new power state

--*/
{





    // Top-edge queue release
    This->m_Device->m_PkgIo->ResumeProcessingForPower();

    if (This->m_SelfManagedIoMachine != NULL) {
        NTSTATUS    status;

        status = This->m_SelfManagedIoMachine->Start();

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "EvtDeviceSelfManagedIoRestart failed - %!STATUS!", status);

            return WdfDevStatePowerStartSelfManagedIoFailedNP;
        }
    }

    This->PowerSetDevicePowerState(WdfPowerDeviceD0);

    //
    // Send the PowerUp event to both the PnP and the Power Policy state machines.
    //
    This->PowerSendPowerUpEvents();

    This->PowerReleasePendingDeviceIrp();

    if (This->m_SharedPower.m_WaitWakeOwner) {
        return WdfDevStatePowerD0BusWakeOwnerNP;
    }
    else {
        return WdfDevStatePowerD0NP;
    }
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerStartSelfManagedIoFailed(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Starting self managed io back up from an explicit Dx to D0 transition failed.
    Hold the power managed queues and proceed down the power up failure path.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerWakingDmaEnableFailed

  --*/
{




    This->m_Device->m_PkgIo->StopProcessingForPower(FxIoStopProcessingForPowerHold);

    return WdfDevStatePowerWakingDmaEnableFailed;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerStartSelfManagedIoFailedNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Starting self managed io back up from an explicit Dx to D0 transition failed.
    Hold the power managed queues and proceed down the power up failure path.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerWakingDmaEnableFailedNP

  --*/
{




    This->m_Device->m_PkgIo->StopProcessingForPower(FxIoStopProcessingForPowerHold);

    return WdfDevStatePowerWakingDmaEnableFailedNP;
}


WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerWakePending(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    State that indicates a successful wake from Dx.  Primarily exists so that
    the driver writer can register to know about the entrance into this state.
    It also completes the pended wait wake request (which posts the appopriate
    events to the power policy state machine if it's listening).

Arguments:
    This - The instance of the state machine

Return Value:
    return WdfDevStatePowerNull

  --*/
{
    This->PowerCompletePendedWakeIrp();
    return WdfDevStatePowerNull;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerWakePendingNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    State that indicates a successful wake from Dx.  Primarily exists so that
    the driver writer can register to know about the entrance into this state.
    It also completes the pended wait wake request (which posts the appopriate
    events to the power policy state machine if it's listening).

Arguments:
    This - The instance of the state machine

Return Value:
    return WdfDevStatePowerNull

  --*/
{
    This->PowerCompletePendedWakeIrp();
    return WdfDevStatePowerNull;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerWaking(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Waking state.  Its job is to call into the
    driver to tell it to restore its hardware, and then to connect interrupts
    and release the queues.

Arguments:
    This - The instance of the state machine

Return Value:

    new power state

--*/
{
    NTSTATUS status;

    //
    // m_DevicePowerState is the "old" state because we update it after the
    // D0Entry callback in SelfManagedIo or PowerPolicyStopped
    //
    status = This->m_DeviceD0Entry.Invoke(
        This->m_Device->GetHandle(),
        (WDF_POWER_DEVICE_STATE) This->m_DevicePowerState);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "EvtDeviceD0Entry WDFDEVICE 0x%p !devobj 0x%p, old state "
            "%!WDF_POWER_DEVICE_STATE! failed, %!STATUS!",
            This->m_Device->GetHandle(),
            This->m_Device->GetDeviceObject(),
            This->m_DevicePowerState, status);

        return WdfDevStatePowerUpFailedDerefParent;
    }

    return WdfDevStatePowerNotifyingD0EntryToWakeInterrupts;
}


WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerWakingNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the WakingNP state.  Its job is to call into the
    driver to tell it to restore its hardware and release the queues.

Arguments:
    This - The instance of the state machine

Return Value:

    new power state

--*/
{
    NTSTATUS status;

    //
    // m_DevicePowerState is the "old" state because we update it after the
    // D0Entry callback in SelfManagedIoNP or PowerPolicyStopped
    //
    status = This->m_DeviceD0Entry.Invoke(
        This->m_Device->GetHandle(),
        (WDF_POWER_DEVICE_STATE) This->m_DevicePowerState);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "EvtDeviceD0Entry WDFDEVICE 0x%p !devobj 0x%p, old state "
            "%!WDF_POWER_DEVICE_STATE! failed, %!STATUS!",
            This->m_Device->GetHandle(),
            This->m_Device->GetDeviceObject(),
            This->m_DevicePowerState, status);

        return WdfDevStatePowerUpFailedDerefParentNP;
    }

    return WdfDevStatePowerNotifyingD0EntryToWakeInterruptsNP;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerWakingConnectInterrupt(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    The device is returning to D0 in the power pageable path.  Connect and
    enable the interrupts.  If that succeeds, scan for children and then
    open the power managed I/O queues.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    NTSTATUS status;

    //
    // interrupt connect and enable
    //
    status = This->NotifyResourceObjectsD0(NotifyResourcesExplicitPowerup);

    if (!NT_SUCCESS(status)) {
        return WdfDevStatePowerWakingConnectInterruptFailed;
    }

    status = This->m_DeviceD0EntryPostInterruptsEnabled.Invoke(
        This->m_Device->GetHandle(),
        (WDF_POWER_DEVICE_STATE) This->m_DevicePowerState);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "EvtDeviceD0EntryPostInterruptsEnabed WDFDEVICE 0x%p !devobj 0x%p, "
            "old state %!WDF_POWER_DEVICE_STATE! failed, %!STATUS!",
            This->m_Device->GetHandle(),
            This->m_Device->GetDeviceObject(),
            This->m_DevicePowerState, status);
        return WdfDevStatePowerWakingConnectInterruptFailed;
    }

    return WdfDevStatePowerWakingDmaEnable;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerWakingConnectInterruptNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    The device is returning to D0 in the power pageable path.
    Enable the interrupts.  If that succeeds, scan for children and then
    open the power managed I/O queues.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    NTSTATUS status;

    //
    // interrupt enable (already connected b/c they were never disconnected
    // during the Dx transition).
    //
    status = This->NotifyResourceObjectsD0(NotifyResourcesNP);

    if (!NT_SUCCESS(status)) {
        return WdfDevStatePowerWakingConnectInterruptFailedNP;
    }

    status = This->m_DeviceD0EntryPostInterruptsEnabled.Invoke(
        This->m_Device->GetHandle(),
        (WDF_POWER_DEVICE_STATE) This->m_DevicePowerState);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "EvtDeviceD0EntryPostInterruptsEnabed WDFDEVICE 0x%p !devobj 0x%p, "
            "old state %!WDF_POWER_DEVICE_STATE! failed, %!STATUS!",
            This->m_Device->GetHandle(),
            This->m_Device->GetDeviceObject(),
            This->m_DevicePowerState, status);
        return WdfDevStatePowerWakingConnectInterruptFailedNP;
    }

    return WdfDevStatePowerWakingDmaEnableNP;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerWakingConnectInterruptFailed(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Connecting or enabling the interrupts failed.  Disable and disconnect any
    interrupts which have been connected and maybe enabled.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerReportPowerUpFailed

  --*/
{
    This->PowerConnectInterruptFailed();

    return WdfDevStatePowerReportPowerUpFailedDerefParent;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerWakingConnectInterruptFailedNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Enabling the interrupts failed.  Disable and disconnect any
    interrupts which have been connected and maybe enabled.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerReportPowerUpFailed

  --*/
{
    //
    // PowerConnectInterruptFailed will call IoDisconnectInterrupt.  Since we
    // are in the NP path, this may cause a deadlock between this thread and
    // paging I/O.  Log something to the IFR so that if the watchdog timer kicks
    // in, at least we have context as to why we died.
    //
    DoTraceLevelMessage(
        This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
        "Force disconnecting interupts on !devobj %p, WDFDEVICE %p",
        This->m_Device->GetDeviceObject(),
        This->m_Device->GetHandle());

    This->PowerConnectInterruptFailed();

    return WdfDevStatePowerReportPowerUpFailedDerefParent;
}

BOOLEAN
FxPkgPnp::PowerDmaEnableAndScan(
    __in BOOLEAN ImplicitPowerUp
    )
{
    FxTransactionedEntry* ple;

    if (PowerDmaPowerUp() == FALSE) {
        return FALSE;
    }

    if (m_EnumInfo != NULL) {
        //
        // Scan for children
        //
        m_EnumInfo->m_ChildListList.LockForEnum(GetDriverGlobals());

        ple = NULL;
        while ((ple = m_EnumInfo->m_ChildListList.GetNextEntry(ple)) != NULL) {
            ((FxChildList*) ple->GetTransactionedObject())->ScanForChildren();
        }

        m_EnumInfo->m_ChildListList.UnlockFromEnum(GetDriverGlobals());
    }

    if (ImplicitPowerUp == FALSE) {
        PowerPolicyProcessEvent(PwrPolPowerUpHwStarted);
    }

    return TRUE;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerWakingDmaEnable(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    The device is returning to D0 from Dx.  Power up all DMA enablers and scan
    for children.

Arguments:
    This - instance of the state machine

Return Value:
    new machine state

  --*/
{
    if (This->PowerDmaEnableAndScan(FALSE) == FALSE) {
        return WdfDevStatePowerWakingDmaEnableFailed;
    }

    //
    // Return the state that we should drop into next.
    //
    return WdfDevStatePowerNull;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerWakingDmaEnableNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    The device is returning to D0 from Dx in the NP path.  Power up all DMA
    enablers and scan for children.

Arguments:
    This - instance of the state machine

Return Value:
    new machine state

  --*/
{
    if (This->PowerDmaEnableAndScan(FALSE) == FALSE) {
        return WdfDevStatePowerWakingDmaEnableFailedNP;
    }

    //
    // Return the state that we should drop into next.
    //
    return WdfDevStatePowerNull;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerWakingDmaEnableFailed(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Powering up a DMA enabler failed.  Power down all DMA enablers and progress
    down the failed power up path.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerWakingConnectInterruptFailed

  --*/
{
    NTSTATUS status;

    (void) This->PowerDmaPowerDown();

    status = This->m_DeviceD0ExitPreInterruptsDisabled.Invoke(
        This->m_Device->GetHandle(),
        WdfPowerDeviceD3Final
        );

    if (!NT_SUCCESS(status)) {
        //
        // Report the error, but continue forward
        //
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "EvtDeviceD0ExitPreInterruptsDisabled WDFDEVICE 0x%p !devobj 0x%p "
            "new state %!WDF_POWER_DEVICE_STATE! failed, %!STATUS!",
            This->m_Device->GetHandle(),
            This->m_Device->GetDeviceObject(),
            WdfPowerDeviceD3Final, status);
    }

    return WdfDevStatePowerWakingConnectInterruptFailed;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerWakingDmaEnableFailedNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Powering up a DMA enabler failed in the NP path.  Power down all DMA
    enablers and progress down the failed power up path.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerWakingConnectInterruptFailedNP

  --*/
{
    NTSTATUS status;

    COVERAGE_TRAP();

    (void) This->PowerDmaPowerDown();

    status = This->m_DeviceD0ExitPreInterruptsDisabled.Invoke(
        This->m_Device->GetHandle(),
        WdfPowerDeviceD3Final
        );

    if (!NT_SUCCESS(status)) {
        //
        // Report the error, but continue forward
        //
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "EvtDeviceD0ExitPreInterruptsDisabled WDFDEVICE 0x%p !devobj 0x%p "
            "new state %!WDF_POWER_DEVICE_STATE! failed, %!STATUS!",
            This->m_Device->GetHandle(),
            This->m_Device->GetDeviceObject(),
            WdfPowerDeviceD3Final, status);
    }

    return WdfDevStatePowerWakingConnectInterruptFailedNP;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerReportPowerUpFailedDerefParent(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Power up failed.  Release the reference on the parent that was taken at the
    start of power up.

Arguments:
    This - instance of the state machine.

Return Value:
    WdfDevStatePowerInitialPowerUpFailed

  --*/
{
    //
    // If this is a child, release the power reference on the parent
    //
    This->PowerParentPowerDereference();

    return WdfDevStatePowerReportPowerUpFailed;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerReportPowerUpFailed(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Posts PowerUpFailed event to the PnP state machine and then waits for death.
    The pended Power IRP is also completed.

Arguments:
    This - The instance of the state machine

Return Value:
    new power state

  --*/
{
    This->m_SystemPowerAction = PowerActionNone;

    This->PowerReleasePendingDeviceIrp();
    This->PowerSendPowerUpFailureEvent();

    return WdfDevStatePowerNull;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerPowerFailedPowerDown(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    We failed to power up/down properly and now the power policy state machine wants
    to power down the device.  Since power policy and pnp rely on power posting
    power down events to make forward progress, we must do that here

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerStopped

  --*/
{
    //
    // Even though we failed power up and never really powered down, record our
    // state as powered down, so that if we are restarted (can easily happen for
    // a PDO), we have the correct previous state.
    //
    This->PowerSetDevicePowerState(WdfPowerDeviceD3Final);

    This->PowerSendPowerDownEvents(FxPowerDownTypeImplicit);

    return WdfDevStatePowerStopped;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerReportPowerDownFailed(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Posts PowerDownFailed event to the PnP state machine and then waits for death.
    The pended Power IRP is also completed.

Arguments:
    This - The instance of the state machine

Return Value:
    WdfDevStatePowerNull

  --*/
{




    This->PowerReleasePendingDeviceIrp();
    This->PowerSendPowerDownFailureEvent(FxPowerDownTypeExplicit);

    return WdfDevStatePowerNull;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerInitialConnectInterruptFailed(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    When bringing the device out of the D3 final state, connecting or enabling
    the interrupts failed.  Disconnect and disable any interrupts which are
    connected and possibly  enabled.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerInitialPowerUpFailedDerefParent

  --*/
{
    This->PowerConnectInterruptFailed();

    return WdfDevStatePowerInitialPowerUpFailedDerefParent;
}


WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerInitialDmaEnableFailed(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Initial power up of the device failed while enabling DMA.  Disable any
    started DMA enablers and proceed down the initial power up failure path.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerInitialConnectInterruptFailed

  --*/
{
    NTSTATUS status;

    (void) This->PowerDmaPowerDown();

    status = This->m_DeviceD0ExitPreInterruptsDisabled.Invoke(
        This->m_Device->GetHandle(),
        WdfPowerDeviceD3Final
        );

    if (!NT_SUCCESS(status)) {
        //
        // Report the error, but continue forward
        //
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "EvtDeviceD0ExitPreInterruptsDisabled WDFDEVICE 0x%p !devobj 0x%p "
            "new state %!WDF_POWER_DEVICE_STATE! failed, %!STATUS!",
            This->m_Device->GetHandle(),
            This->m_Device->GetDeviceObject(),
            WdfPowerDeviceD3Final, status);
    }

    return WdfDevStatePowerInitialConnectInterruptFailed;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerInitialSelfManagedIoFailed(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    The self managed io start failed when bringing the device out of Dx with an
    implicit D0 transition.  Hold the power queues that were previous open can
    continue to cleanup

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerInitialDmaEnableFailed

  --*/
{




    This->m_Device->m_PkgIo->StopProcessingForPower(FxIoStopProcessingForPowerHold);

    return WdfDevStatePowerInitialDmaEnableFailed;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerInitialPowerUpFailedDerefParent(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Dereferences the parent's power ref count we took during start up

Arguments:
    This - The instance of the state machine

Return Value:
    WdfDevStatePowerInitialPowerUpFailed

  --*/
{

    //
    // If this is a child, release the power reference on the parent
    //
    This->PowerParentPowerDereference();

    return WdfDevStatePowerInitialPowerUpFailed;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerInitialPowerUpFailed(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Posts PowerUpFailed event to the PnP state machine and then waits for death.

Arguments:
    This - The instance of the state machine

Return Value:
    WdfDevStatePowerStopped

  --*/
{
    This->PowerSendPowerUpFailureEvent();

    return WdfDevStatePowerStopped;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerDxStoppedDisarmWake(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Disarms the device from wake because the device is being stopped/removed
    while in Dx.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerDxStopped

  --*/
{
    //
    // We should only get into this state when this devobj is not a PDO and a
    // power policy owner.
    //
    ASSERT((This->m_Device->IsPdo() && This->IsPowerPolicyOwner()) == FALSE);

    This->PowerDisableWakeAtBusOverload();

    return WdfDevStatePowerGotoDxStoppedDisableInterrupt;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerDxStoppedDisarmWakeNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Disarms the device from wake because the device is being stopped/removed
    while in Dx.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerGotoDxStoppedDisableInterruptNP

  --*/
{
    //
    // We should only get into this state when this devobj is not a PDO and a
    // power policy owner.
    //
    ASSERT((This->m_Device->IsPdo() && This->IsPowerPolicyOwner()) == FALSE);

    This->PowerDisableWakeAtBusOverload();

    return WdfDevStatePowerGotoDxStoppedDisableInterruptNP;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerGotoDxStoppedDisableInterruptNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Device is in Dx and was implicitly powered down. Disable the interrupt since
    the interrupt was not disconnected in the NP power down path.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerGotoDxStopped

  --*/
{
    This->NotifyResourceObjectsDx(NotifyResourcesSurpriseRemoved);

    return WdfDevStatePowerGotoDxStopped;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerGotoDxStopped(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Device is in Dx and being removed/stopped.  Inform the other state machines
    that the device is powered down so that they may continue.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerStopped

  --*/
{
    This->PowerSendPowerDownEvents(FxPowerDownTypeImplicit);

    return WdfDevStatePowerStopped;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerGotoStopped(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    We are in the Dx state and the power policy owner sent an explicit D0 irp
    to the stack.  Transition to the stopped state which will transition to the
    D0 state when receiving an implicit D0.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerStopped

  --*/
{
    //
    // We should only get into this state when this devobj is not the power
    // policy owner.
    //
    ASSERT(This->IsPowerPolicyOwner() == FALSE);

    This->PowerReleasePendingDeviceIrp();
    return WdfDevStatePowerStopped;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerStoppedCompleteDx(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    We were in the Stopped state and the power policy owner sent an explicit
    Dx request to the stack.  We will move to the DxStopped state where we will
    transition to a Dx state when an implicit D0 is received.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerDxStopped

  --*/
{
    //
    // We should only get into this state when this devobj is not the power
    // policy owner.
    //
    ASSERT(This->IsPowerPolicyOwner() == FALSE);

    This->PowerReleasePendingDeviceIrp();
    return WdfDevStatePowerDxStopped;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerDxStoppedDecideDxState(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    We are implicitly powering up the stack while in the Dx state.  Decide
    which Dx "holding" state to transition to.

Arguments:
    This - instance of the state machine

Return Value:
    new power state

  --*/
{
    //
    // We should only get into this state when this devobj is not the power
    // policy owner.
    //
    ASSERT(This->IsPowerPolicyOwner() == FALSE);

    //
    // Move power policy back into a working state
    //
    // While it seems odd to send a power up in a Dx state, the power up is
    // really an indication to the power policy state machine that the implicit
    // D0 has been successfully processed, basically, it is a status event more
    // then an indication of true power state.
    //
    This->PowerSendPowerUpEvents();

    if (This->m_Device->GetDeviceObjectFlags() & DO_POWER_PAGABLE) {
        if (This->PowerIsWakeRequestPresent()) {
            COVERAGE_TRAP();
            return WdfDevStatePowerDxStoppedArmForWake;
        }
        else {
            return WdfDevStatePowerDx;
        }
    }
    else {
        if (This->PowerIsWakeRequestPresent()) {
            COVERAGE_TRAP();
            return WdfDevStatePowerDxStoppedArmForWakeNP;
        }
        else {
            COVERAGE_TRAP();
            return WdfDevStatePowerDxNP;
        }
    }
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerDxStoppedArmForWake(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    We are implicitly powering the stack back up while the device is in Dx
    and there is a wait wake request presnt on this device.  Enable wake at
    the bus level and then move to the appropriate state based on the enabling
    status.

Arguments:
    This - instance of the state machine

Return Value:
    new power state

  --*/
{
    NTSTATUS status;

    COVERAGE_TRAP();

    //
    // We should only get into this state when this devobj is not a PDO and a
    // power policy owner.
    //
    ASSERT((This->m_Device->IsPdo() && This->IsPowerPolicyOwner()) == FALSE);

    status = This->PowerEnableWakeAtBusOverload();

    if (NT_SUCCESS(status)) {
        //
        // No matter of the irp status (canceled, pending, completed), we always
        // transition to the D0ArmedForWake state because that is where we
        // we handle the change in the irp's status.
        //
        COVERAGE_TRAP();
        return WdfDevStatePowerDxArmedForWake;
    }
    else {
        COVERAGE_TRAP();
        This->PowerCompleteWakeRequestFromWithinMachine(status);
        return WdfDevStatePowerDx;
    }
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerDxStoppedArmForWakeNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    We are implicitly powering the stack back up while the device is in Dx
    and there is a wait wake request presnt on this device.  Enable wake at
    the bus level and then move to the appropriate state based on the enabling
    status.

Arguments:
    This - instance of the state machine

Return Value:
    new power state

  --*/
{
    NTSTATUS status;

    COVERAGE_TRAP();

    //
    // We should only get into this state when this devobj is not a PDO and a
    // power policy owner.
    //
    ASSERT((This->m_Device->IsPdo() && This->IsPowerPolicyOwner()) == FALSE);

    status = This->PowerEnableWakeAtBusOverload();

    if (NT_SUCCESS(status)) {
        //
        // No matter of the irp status (canceled, pending, completed), we always
        // transition to the D0ArmedForWake state because that is where we
        // we handle the change in the irp's status.
        //
        COVERAGE_TRAP();
        return WdfDevStatePowerDxArmedForWakeNP;
    }
    else {
        COVERAGE_TRAP();
        This->PowerCompleteWakeRequestFromWithinMachine(status);
        return WdfDevStatePowerDxNP;
    }
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerFinalPowerDownFailed(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Posts PowerDownFailed event to the PnP state machine and then waits for death.

Arguments:
    This - The instance of the state machine

Return Value:
    WdfDevStatePowerStopped

  --*/
{
    This->PowerSendPowerDownFailureEvent(FxPowerDownTypeImplicit);

    //
    // If we are not the PPO for the stack we could receive a power irp
    // during the middle of an implicit power down so we cannot assume
    // that there will be no pended power irp during an implicit power down.
    //
    ASSERT(This->IsPowerPolicyOwner() ? This->m_PendingDevicePowerIrp == NULL  : TRUE);

    return WdfDevStatePowerStopped;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerUpFailedDerefParent(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Disconnects interrupts

Arguments:
    This - The instance of the state machine

Return Value:
    WdfDevStatePowerReportPowerUpFailed

  --*/
{
    //
    // Notify the interrupt state machines that the power up failed
    //
    This->SendEventToAllWakeInterrupts(WakeInterruptEventD0EntryFailed);
    //
    // NotifyResourceObjectsDx will log any errors
    //
    (void) This->NotifyResourceObjectsDx(NotifyResourcesForceDisconnect |
                                     NotifyResourcesDisconnectInactive);

    return WdfDevStatePowerReportPowerUpFailedDerefParent;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerUpFailed(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Disconnects interrupts

Arguments:
    This - The instance of the state machine

Return Value:
    WdfDevStatePowerReportPowerUpFailed

  --*/
{
    COVERAGE_TRAP();
    //
    // Notify the interrupt state machines that the power up failed
    //
    This->SendEventToAllWakeInterrupts(WakeInterruptEventD0EntryFailed);
    //
    // NotifyResourceObjectsDx will log any errors
    //
    (void) This->NotifyResourceObjectsDx(NotifyResourcesForceDisconnect |
                                     NotifyResourcesDisconnectInactive);

    return WdfDevStatePowerReportPowerUpFailed;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerGotoDxFailed(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Disconnects interrupts

Arguments:
    This - The instance of the state machine

Return Value:
    WdfDevStatePowerReportPowerDownFailed

  --*/
{
    //
    // NotifyResourceObjectsDx will log any errors
    //
    (void) This->NotifyResourceObjectsDx(NotifyResourcesForceDisconnect |
                                     NotifyResourcesDisconnectInactive);

    return WdfDevStatePowerReportPowerDownFailed;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerGotoDxStoppedDisableInterrupt(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Disconnects interrupts

Arguments:
    This - The instance of the state machine

Return Value:
    WdfDevStatePowerGotoDxStopped

  --*/
{
    //
    // NotifyResourceObjectsDx will log any errors
    //
    (void) This->NotifyResourceObjectsDx(NotifyResourcesForceDisconnect |
                                     NotifyResourcesDisconnectInactive);

    return WdfDevStatePowerGotoDxStopped;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerUpFailedDerefParentNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Disconnects interrupts

Arguments:
    This - The instance of the state machine

Return Value:
    WdfDevStatePowerReportPowerUpFailed

  --*/
{
    //
    // Notify the interrupt state machines that the power up failed
    //
    This->SendEventToAllWakeInterrupts(WakeInterruptEventD0EntryFailed);
    This->DisconnectInterruptNP();

    return WdfDevStatePowerReportPowerUpFailedDerefParent;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerUpFailedNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Disconnects interrupts

Arguments:
    This - The instance of the state machine

Return Value:
    WdfDevStatePowerReportPowerUpFailed

  --*/
{
    //
    // Notify the interrupt state machines that the power up failed
    //
    This->SendEventToAllWakeInterrupts(WakeInterruptEventD0EntryFailed);
    This->DisconnectInterruptNP();

    return WdfDevStatePowerReportPowerUpFailed;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerNotifyingD0EntryToWakeInterrupts(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Notifies the wake interrupt state machines that the device has entered
    D0

Arguments:
    This - The instance of the state machine

Return Value:
    WdfDevStatePowerWakingConnectInterrupt if there are no wake interrupts
    or WdfDevStatePowerNull otherwise

  --*/
{
    if (This->m_WakeInterruptCount == 0) {
        return WdfDevStatePowerWakingConnectInterrupt;
    }

    This->SendEventToAllWakeInterrupts(WakeInterruptEventEnteringD0);

    return WdfDevStatePowerNull;;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerNotifyingD0ExitToWakeInterrupts(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Notifies the wake interrupt state machines that the device is about
    to exit D0

Arguments:
    This - The instance of the state machine

Return Value:
    WdfDevStatePowerDx if there are no wake interrupts or WdfDevStatePowerNull
    otherwise

  --*/
{
    if (This->m_WakeInterruptCount == 0) {
        return WdfDevStatePowerGotoDxIoStopped;
    }

    //
    // Indiciate to the wake interrupt state machine that the device is
    // leaving D0 and also whether the device is armed for wake. The wake
    // interrupt machine treats these differently as described below.
    //
    if (This->m_WakeInterruptsKeepConnected == TRUE ||
        This->m_SharedPower.m_WaitWakeIrp != NULL) {
        This->SendEventToAllWakeInterrupts(WakeInterruptEventLeavingD0);
    }
    else {
        //
        // When a wake interrupt is not armed for wake it will be disconnected
        // by the power state machine once the wake interrupt state machine
        // acknowledges the transition. If the interrupt fires between
        // the time this event is posted and it is disconnected, it needs to be
        // delivered to the driver or a deadlock could occur between PO state machine
        // trying to disconnect the interrupt and the wake interrupt machine
        // holding on to the ISR waiting for the device to return to D0 before
        // delivering the interrupt.
        //
        This->SendEventToAllWakeInterrupts(WakeInterruptEventLeavingD0NotArmedForWake);
    }

    return WdfDevStatePowerNull;;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerNotifyingD0EntryToWakeInterruptsNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Notifies the wake interrupt state machines that the device has entered
    D0

Arguments:
    This - The instance of the state machine

Return Value:
    WdfDevStatePowerWakingConnectInterruptNP if there are no wake interrupts
    or WdfDevStatePowerNull otherwise

  --*/
{
    if (This->m_WakeInterruptCount == 0) {
        return WdfDevStatePowerWakingConnectInterruptNP;
    }

    This->SendEventToAllWakeInterrupts(WakeInterruptEventEnteringD0);

    return WdfDevStatePowerNull;;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerNotifyingD0ExitToWakeInterruptsNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Notifies the wake interrupt state machines that the device is about
    to exit D0

Arguments:
    This - The instance of the state machine

Return Value:
    WdfDevStatePowerDxNP if there are no wake interrupts or WdfDevStatePowerNull
    otherwise

  --*/
{
    if (This->m_WakeInterruptCount == 0) {
        return WdfDevStatePowerGotoDxIoStoppedNP;
    }

    //
    // Indiciate to the wake interrupt state machine that the device is
    // leaving D0 and also whether the device is armed for wake. The wake
    // interrupt machine treats these differently as described below
    //
    if (This->m_WakeInterruptsKeepConnected == TRUE ||
        This->m_SharedPower.m_WaitWakeIrp != NULL) {
        This->SendEventToAllWakeInterrupts(WakeInterruptEventLeavingD0);
    }
    else {
        //
        // When a wake interrupt is not armed for wake it will be disconnected by
        // the power state machine once the wake interrupt state machine
        // acknowledges the transition. If the interrupt fires between
        // the time this event is posted and it is disconnected, it needs to be
        // delivered to the driver or a deadlock could occur between PO state machine
        // trying to disconnect the interrupt and the wake interrupt machine holding on
        // to the ISR waiting for the device to return to D0 before delivering the
        // interrupt.
        //
        This->SendEventToAllWakeInterrupts(WakeInterruptEventLeavingD0NotArmedForWake);
    }

    return WdfDevStatePowerNull;;
}

VOID
FxPkgPnp::DisconnectInterruptNP(
    VOID
    )
{
    //
    // NotifyResourceObjectsDx will call IoDisconnectInterrupt.  Since we
    // are in the NP path, this may cause a deadlock between this thread and
    // paging I/O.  Log something to the IFR so that if the watchdog timer kicks
    // in, at least we have context as to why we died.
    //
    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
        "Force disconnecting interrupts on !devobj 0x%p, WDFDEVICE %p",
        m_Device->GetDeviceObject(),
        m_Device->GetHandle());

    //
    // NotifyResourceObjectsDx will log any errors
    //
    (void) NotifyResourceObjectsDx(NotifyResourcesForceDisconnect);
}

BOOLEAN
FxPkgPnp::PowerIndicateWaitWakeStatus(
    __in NTSTATUS WaitWakeStatus
    )
/*++

Routine Description:
    If there is a pended wait wake request, this routine will remove the cancel
    routine and post the appropriate event to the power state machine.  The
    consumer of this event will do the actual completion of the wait wake
    request.

    The difference between this routine and
    PowerCompleteWakeRequestFromWithinMachine is that
    PowerCompleteWakeRequestFromWithinMachine is a private API to the state
    machine and will attempt to complete the request immediately instead of
    deferring the completion through the posting of a power state machine event.

    PowerCompletePendedWakeIrp is used within the state machine to complete the
    IRP deferred by this routine.

Arguments:
    WaitWakeStatus - The final status of the wait wake request

Return Value:
    TRUE if there there was a request to cancel

  --*/
{
    if (PowerMakeWakeRequestNonCancelable(WaitWakeStatus)) {
        //
        // The power machine will eventually call PowerCompletePendedWakeIrp
        // to complete the request.
        //
        if (WaitWakeStatus == STATUS_CANCELLED) {
            PowerProcessEvent(PowerWakeCanceled);
        }
        else if (NT_SUCCESS(WaitWakeStatus)) {
            PowerProcessEvent(PowerWakeSucceeded);
        }
        else {
            PowerProcessEvent(PowerWakeFailed);
        }

        return TRUE;
    }
    else {
        return FALSE;
    }
}

VOID
FxPkgPnp::PowerCompletePendedWakeIrp(
    VOID
    )
/*++

Routine Description:
    Completes the wait wake request that was pended by the power state machine.
    It is valid if there is no request to complete (the only time there will be
    a request to complete is when this power state machine is the wait wake owner

Arguments:
    None

Return Value:
    None

  --*/
{
    PLIST_ENTRY ple;
    KIRQL irql;

    if (m_SharedPower.m_WaitWakeOwner == FALSE) {
        COVERAGE_TRAP();
        return;
    }

    //
    // Pop an irp off of the list
    //
    m_PowerMachine.m_WaitWakeLock.Acquire(&irql);
    ASSERT(IsListEmpty(&m_PowerMachine.m_WaitWakeIrpToBeProcessedList) == FALSE);
    ple = RemoveHeadList(&m_PowerMachine.m_WaitWakeIrpToBeProcessedList);
    m_PowerMachine.m_WaitWakeLock.Release(irql);

    InitializeListHead(ple);

    FxIrp irp(FxIrp::GetIrpFromListEntry(ple));

    CompletePowerRequest(&irp, irp.GetStatus());
}

VOID
FxPkgPnp::PowerCompleteWakeRequestFromWithinMachine(
    __in NTSTATUS WaitWakeStatus
    )
/*++

Routine Description:
    Completes a wait wake from within the power state machine.  Contrary to
    PowerIndicateWaitWakeStatus which posts an event to the state machine to
    process the irp's status change, this routine attempts to complete the
    irp immediately.

Arguments:
    Status - Final status of the wake irp

Return Value:
    None

  --*/
{
    if (PowerMakeWakeRequestNonCancelable(WaitWakeStatus)) {
        PowerCompletePendedWakeIrp();
    }
}

BOOLEAN
FxPkgPnp::PowerMakeWakeRequestNonCancelable(
    __in NTSTATUS WaitWakeStatus
    )
/*++

Routine Description:
    Attempts to clear the cancel routine from the pended wake request.  If
    successful, it will put the wake request on a pending wake list to be
    completed later by PowerCompletePendedWakeIrp .

Arguments:
    WaitWakeStatus - final status for the wake request

Return Value:
    TRUE if there was a request and we cleared the cancel routine, FALSE
    otherwise

  --*/
{
    KIRQL irql;
    BOOLEAN result;

    //
    // Currently we assume that if we are the bus wait wake owner and that only
    // PDOs can be bus wake owners, so we must have a parent device.
    //
    ASSERT(m_SharedPower.m_WaitWakeOwner &&
        (m_Device->m_ParentDevice != NULL));

    result = FALSE;

    //
    // Attempt to retrieve the wait wake irp.  We can safely dereference the
    // PIRP in while holding the lock as long as it is not NULL.
    //
    m_PowerMachine.m_WaitWakeLock.Acquire(&irql);

    if (m_SharedPower.m_WaitWakeIrp != NULL) {
        MdCancelRoutine pOldCancelRoutine;
        FxIrp wwIrp;

        wwIrp.SetIrp(m_SharedPower.m_WaitWakeIrp);

        pOldCancelRoutine = wwIrp.SetCancelRoutine(NULL);

        if (pOldCancelRoutine != NULL) {
            FxPkgPnp* pParentPkg;

            pParentPkg = m_Device->m_ParentDevice->m_PkgPnp;

            //
            // Propagate the successful wake status from the parent to this
            // child's WW IRP if the parent is the PPO for its stack.
            //
            if (NT_SUCCESS(WaitWakeStatus) &&
                pParentPkg->IsPowerPolicyOwner() &&
                pParentPkg->m_PowerPolicyMachine.m_Owner->m_SystemWakeSource) {
                //
                // The only way that m_SystemWakeSource can be TRUE is if
                // FxLibraryGlobals.PoGetSystemWake != NULL and if it is not
                // NULL, then FxLibraryGlobals.PoSetSystemWake cannot be NULL
                // either.





                FxPkgPnp::_PowerSetSystemWakeSource(&wwIrp);

                //
                // If this PDO is the PPO for its stack, then we must mark this
                // device as the system wake source if we have any
                // enumerated PDOs off of this PDO so that we can propagate the
                // system wake source attribute to our children stacks.
                // (For a FDO which is the PPO, we do this in the WW completion
                // routine.)
                //
                if (IsPowerPolicyOwner()) {
                    DoTraceLevelMessage(
                        GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
                        "WDFDEVICE 0x%p !devobj 0x%p WW !irp 0x%p is a source "
                        "of wake", m_Device->GetHandle(),
                        m_Device->GetDeviceObject(),
                        m_SharedPower.m_WaitWakeIrp);

                    m_PowerPolicyMachine.m_Owner->m_SystemWakeSource = TRUE;
                }
            }

            //
            // Set the status for the irp when it is completed later
            //
            wwIrp.SetStatus(WaitWakeStatus);

            //
            // Queue the irp for completion later
            //
            InsertTailList(&m_PowerMachine.m_WaitWakeIrpToBeProcessedList,
                           wwIrp.ListEntry());

            wwIrp.SetIrp(NULL);
            m_SharedPower.m_WaitWakeIrp = NULL;
            result = TRUE;
        }
        else {
            //
            // The irp is being canceled as we run here.  As soon as the spin
            // lock is dropped, the cancel routine will run (or continue if it
            // is blocked on this lock).  Do nothing here and let the cancel
            // routine run its course.
            //
            ASSERT(wwIrp.IsCanceled());
            DO_NOTHING();
        }
    }
    m_PowerMachine.m_WaitWakeLock.Release(irql);

    return result;
}

VOID
FxPkgPnp::PowerSendIdlePowerEvent(
    __in FxPowerIdleEvents Event
    )
{
    if (IsPowerPolicyOwner()) {
        m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.ProcessPowerEvent(Event);
    }
}

VOID
FxPkgPnp::PowerSendPowerDownEvents(
    __in FxPowerDownType Type
    )
/*++

Routine Description:
    The device has powered down, inform the power policy state machine

Arguments:
    Type - the type of power down being performed

Return Value:
    None

  --*/
{
    //
    // If this is an implicit power type, there is completion routine on the
    // power irp and we must send these events in the power state machine
    // regardless if we are the PPO or not.
    //
    if (Type == FxPowerDownTypeImplicit) {
        PowerSendIdlePowerEvent(PowerIdleEventPowerDown);

        //
        // If we are the power policy owner, there is no need to distinguish
        // between an implicit power down or an explicit power down since the
        // PPO controls all power irps in the stack and a power policy stop
        // (e.g. an implicit power down) will not be racing with a real power
        // irp.
        //
        // The non PPO state machine needs to distinguish between the 2 types
        // of power downs because both of them may occur simultaneously and we
        // don't want to interpret the power down event for the real Dx irp with
        // the power down event for the (final) implicit power down.
        //
        PowerPolicyProcessEvent(IsPowerPolicyOwner() ? PwrPolPowerDown
                                                     : PwrPolImplicitPowerDown);
        return;
    }

    ASSERT(Type == FxPowerDownTypeExplicit);

    //
    // If we are the PPO, then we will send PwrPolPowerDown in the completion
    // routine passed to PoRequestPowerIrp.  If we are not the PPO, we must send
    // the event now because we have no such completion routine.
    //
    if (IsPowerPolicyOwner()) {
        //
        // Transition the idle state machine to off immediately.
        //
        m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.ProcessPowerEvent(
            PowerIdleEventPowerDown
            );
    }
    else {
        //
        // If we are not the PPO, there is no idle state machine to send an
        // event to and there is no Po request completion routine to send this
        // event, so we send it now.
        //
        PowerPolicyProcessEvent(PwrPolPowerDown);
    }
}

VOID
FxPkgPnp::PowerSendPowerUpEvents(
    VOID
    )
/*++

Routine Description:
    Sends power up events to the pnp and power policy state machines

Arguments:
    None

Return Value:
    None

  --*/
{
    PowerSendIdlePowerEvent(PowerIdleEventPowerUpComplete);

    //
    // This must be called *before* PowerPostParentToD0ToChildren so that we
    // clear the child power up guard variable in the power policy state machine
    // and then post an event which will unblock the child.
    //
    PowerPolicyProcessEvent(PwrPolPowerUp);
}

VOID
FxPkgPnp::PowerSendPowerDownFailureEvent(
    __in FxPowerDownType Type
    )
/*++

Routine Description:
    Sends a power down failure event to the pnp state machine marks an internal
    error.

Arguments:
    None

Return Value:
    None

  --*/
{
    SetInternalFailure();

    if (IsPowerPolicyOwner()) {
        //
        // If we are the PPO and this is an explicit power operation, then we
        // will send PwrPolPowerDownFailed in the completion routine passed to
        // PoRequestPowerIrp.   Otherwise, if this is an implicit power operation
        // that failed, we need to send the event now because there is no
        // Po completion routine to send the event later.
        //
        if (Type == FxPowerDownTypeImplicit) {
            //
            // Since there is no power irp to complete, we must send the event
            // now.
            //
            // We only send PwrPolImplicitPowerDownFailed if we are not the
            // PPO, so we can't share code with the !PPO path.
            //
            PowerPolicyProcessEvent(PwrPolPowerDownFailed);
        }
        else {
            ASSERT(Type == FxPowerDownTypeExplicit);

            //
            // Process the state change immediately in the idle state machine.
            // We should do this only on an explicit power down since the PPO
            // will have disabled the idle state machine before attempting an
            // implicit power down.
            //
            m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.ProcessPowerEvent(
                PowerIdleEventPowerDownFailed
                );
        }
    }
    else {
        //
        // We are not the PPO, so we must send the event now because we have no
        // such completion routine.  Decide which event to send to the
        // !PPO state machine.
        //
        PowerPolicyProcessEvent(
            Type == FxPowerDownTypeImplicit ? PwrPolImplicitPowerDownFailed
                                            : PwrPolPowerDownFailed
            );

        //
        // Send the pnp event last becuase if we are in the middle of a Dx
        // transition during S0, there is no S irp to guard a pnp state change
        // from occurring immediately after sending this event.  If we sent it
        // before sending the power policy message, power policy might not
        // transition to the failed state by the time pnp does creating a
        // mismatch.
        //
        if (FALSE == m_ReleaseHardwareAfterDescendantsOnFailure) {
            PnpProcessEvent(PnpEventPowerDownFailed);
        }
    }
}

VOID
FxPkgPnp::PowerSendPowerUpFailureEvent(
    VOID
    )
/*++

Routine Description:
    Sends a power up failure event to the pnp state machine marks an internal
    error.

Arguments:
    None

Return Value:
    None

  --*/
{
    SetInternalFailure();
    PowerSendIdlePowerEvent(PowerIdleEventPowerUpFailed);

    PowerPolicyProcessEvent(PwrPolPowerUpFailed);

    if (FALSE == m_ReleaseHardwareAfterDescendantsOnFailure) {
        PnpProcessEvent(PnpEventPowerUpFailed);
    }
}

VOID
FxPkgPnp::PowerSetDevicePowerState(
    __in WDF_POWER_DEVICE_STATE State
    )
/*++

Routine Description:
    Stores the state in the object and notifies the system of the change.

Arguments:
    State - new device state

Return Value:
    VOID

  --*/
{
    POWER_STATE powerState;

    //
    // Remember our previous state
    //
    m_DevicePowerStateOld = m_DevicePowerState;

    //
    // Set our new state
    //
    ASSERT(State <= 0xFF);
    m_DevicePowerState = (BYTE) State;

    //
    // Notify the system of the new power state.
    //
    switch (State) {
    case WdfPowerDeviceD3Final:
    case WdfPowerDevicePrepareForHibernation:
        powerState.DeviceState = PowerDeviceD3;
        break;

    case WdfPowerDeviceD0:
         m_SystemPowerAction = PowerActionNone;
         __fallthrough;

    default:
        powerState.DeviceState = (DEVICE_POWER_STATE) State;
        break;
    }

    MxDeviceObject deviceObject(m_Device->GetDeviceObject());
    deviceObject.SetPowerState(
                    DevicePowerState,
                    powerState);
}

VOID
FxPkgPnp::PowerConnectInterruptFailed(
    VOID
    )
/*++

Routine Description:
    Worker routine for all the paths where we are trying to bring the device into
    D0 and failed while trying to connect or enable an interrupt.  This routine
    disables and disconnects all interrupts, calls D0Exit, and sets the device
    state.

Arguments:
    This - instance of the state machine

Return Value:
    None

  --*/

{
    NTSTATUS status;

    status = NotifyResourceObjectsDx(NotifyResourcesForceDisconnect);

    if (!NT_SUCCESS(status)) {
        //
        // Report the error, but continue forward
        //
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "Interrupt(s) disconnect on WDFDEVICE %p failed, %!STATUS!",
            m_Device->GetHandle(), status);
    }

    status = m_DeviceD0Exit.Invoke(m_Device->GetHandle(),
        WdfPowerDeviceD3Final);

    if (!NT_SUCCESS(status)) {
        //
        // Report the error, but continue forward
        //
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "EvtDeviceD0Exit WDFDEVICE 0x%p !devobj 0x%p failed, %!STATUS!",
            m_Device->GetHandle(),
            m_Device->GetDeviceObject(), status);
    }

    PowerSetDevicePowerState(WdfPowerDeviceD3Final);
}








VOID
FxPkgPnp::_PowerWaitWakeCancelRoutine(
    __in MdDeviceObject DeviceObject,
    __in MdIrp Irp
    )
/*++

Routine Description:
    Cancel routine for the pended wait wake irp.  This will post an event to
    the power state machine to process the cancellation under the machine's
    lock and complete the irp later.

Arguments:
    DeviceObject - Instance of this state machine
    Irp - The wait wake request being canceled

Return Value:
    None

  --*/
{
    CfxDevice* pDevice;
    FxPkgPdo* pThis;
    FxIrp irp(Irp);
    KIRQL irql;

    //
    // Release the IO cancel spinlock because we use our own
    //
    Mx::ReleaseCancelSpinLock(irp.GetCancelIrql());

    pDevice = FxDevice::GetFxDevice(DeviceObject);
    pThis = pDevice->GetPdoPkg();

    ASSERT(pThis->m_SharedPower.m_WaitWakeOwner);

    //
    // Clear out the IRQL and set our state to disarming
    //
    pThis->m_PowerMachine.m_WaitWakeLock.Acquire(&irql);

    ASSERT(pThis->m_SharedPower.m_WaitWakeIrp == Irp &&
              pThis->m_SharedPower.m_WaitWakeIrp != NULL);

    InsertTailList(&pThis->m_PowerMachine.m_WaitWakeIrpToBeProcessedList,
                   irp.ListEntry());

    //
    // Set the status for the irp when it is completed later
    //
    irp.SetStatus(STATUS_CANCELLED);

    pThis->m_SharedPower.m_WaitWakeIrp = NULL;

    pThis->m_PowerMachine.m_WaitWakeLock.Release(irql);

    pThis->PowerProcessEvent(PowerWakeCanceled);
}
