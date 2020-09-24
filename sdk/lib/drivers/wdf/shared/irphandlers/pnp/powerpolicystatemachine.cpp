/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    PowerPolicyStateMachine.cpp

Abstract:

    This module implements the Power Policy state machine for the driver
    framework.  This code was split out from FxPkgPnp.cpp.

Author:




Environment:

    Both kernel and user mode

Revision History:



--*/

#include "pnppriv.hpp"

#if FX_IS_KERNEL_MODE
#include <usbdrivr.h>
#endif

#include "FxUsbIdleInfo.hpp"

extern "C" {
#if defined(EVENT_TRACING)
#include "PowerPolicyStateMachine.tmh"
#endif
}

//
// The Power Policy State Machine
//
// This state machine responds to the following events:
//
// PowerUp
// PowerDown
// PowerPolicyStart
// PowerPolicyStop
// IRP_MN_SET_POWER -- System State S0
// IRP_MN_SET_POWER -- System State Sx
// PowerTimeoutExpired
// IoPresent
// IRP_MN_WAIT_WAKE Complete
// IRP_MN_WAIT_WAKE Failed
//

#if FX_SUPER_DBG
  #define ASSERT_PWR_POL_STATE(_This, _State) \
      ASSERT((_This)->m_Device->GetDevicePowerPolicyState() == (_State))
#else
  #define ASSERT_PWR_POL_STATE(_This, _State) (0)
#endif

#if FX_STATE_MACHINE_VERIFY
    #define VALIDATE_PWR_POL_STATE(_CurrentState, _NewState)                      \
        ValidatePwrPolStateEntryFunctionReturnValue((_CurrentState), (_NewState))
#else
    #define VALIDATE_PWR_POL_STATE(_CurrentState, _NewState)   (0)
#endif  //FX_STATE_MACHINE_VERIFY

// @@SMVERIFY_SPLIT_BEGIN

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolObjectCreatedOtherStates[] =
{
    { PwrPolRemove, WdfDevStatePwrPolRemoved DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolStartingOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolStartingFailed DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolStartedIdleCapableOtherStates[] =
{
    { PwrPolSx, WdfDevStatePwrPolStartedIdleCapableCancelTimerForSleep DEBUGGED_EVENT },
    { PwrPolStop, WdfDevStatePwrPolStoppingCancelTimer DEBUGGED_EVENT },
    { PwrPolSurpriseRemove,WdfDevStatePwrPolStoppingCancelTimer DEBUGGED_EVENT },
    { PwrPolS0IdlePolicyChanged, WdfDevStatePwrPolStartedCancelTimer DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolIdleCapableDeviceIdleOtherStates[] =
{
    { PwrPolSx, WdfDevStatePwrPolDeviceIdleSleeping DEBUGGED_EVENT },
    { PwrPolStop, WdfDevStatePwrPolDeviceIdleStopping DEBUGGED_EVENT },
    { PwrPolSurpriseRemove,WdfDevStatePwrPolDeviceIdleStopping DEBUGGED_EVENT },
    { PwrPolS0IdlePolicyChanged, WdfDevStatePwrPolDeviceIdleReturnToActive TRAP_ON_EVENT },
    { PwrPolIoPresent, WdfDevStatePwrPolDeviceIdleReturnToActive DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolTimerExpiredNoWakeOtherStates[] =
{
    { PwrPolPowerDown, WdfDevStatePwrPolTimerExpiredNoWakePowerDownNotProcessed TRAP_ON_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolTimerExpiredNoWakeCompletePowerDownOtherStates[] =
{
    { PwrPolPowerDownFailed, WdfDevStatePwrPolDevicePowerRequestFailed DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolWaitingUnarmedOtherStates[] =
{
    { PwrPolSx, WdfDevStatePwrPolSystemSleepFromDeviceWaitingUnarmed DEBUGGED_EVENT },
    { PwrPolStop, WdfDevStatePwrPolStoppingResetDevice DEBUGGED_EVENT },
    { PwrPolSurpriseRemove, WdfDevStatePwrPolStoppingCancelTimer DEBUGGED_EVENT },
    { PwrPolS0IdlePolicyChanged, WdfDevStatePwrPolS0NoWakePowerUp DEBUGGED_EVENT },
    { PwrPolDevicePowerRequired, WdfDevStatePwrPolS0NoWakePowerUp DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolS0NoWakePowerUpOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolDevicePowerRequestFailed DEBUGGED_EVENT },
    { PwrPolPowerUpNotSeen, WdfDevStatePwrPolDeviceD0PowerRequestFailed TRAP_ON_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolS0NoWakeCompletePowerUpOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolDevicePowerRequestFailed DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemSleepNeedWakeOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolSystemSleepPowerRequestFailed DEBUGGED_EVENT },
    { PwrPolPowerUpNotSeen, WdfDevStatePwrPolPowerUpForSystemSleepNotSeen TRAP_ON_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemSleepNeedWakeCompletePowerUpOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolSystemSleepPowerRequestFailed DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemAsleepWakeArmedOtherStates[] =
{
    { PwrPolWakeSuccess, WdfDevStatePwrPolSystemWakeDeviceWakeTriggered DEBUGGED_EVENT },
    { PwrPolWakeInterruptFired, WdfDevStatePwrPolSystemWakeDeviceWakeInterruptFired DEBUGGED_EVENT },
    { PwrPolNull,        WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemAsleepWakeArmedNPOtherStates[] =
{
    { PwrPolWakeSuccess, WdfDevStatePwrPolSystemWakeDeviceWakeTriggeredNP DEBUGGED_EVENT },
    { PwrPolWakeInterruptFired, WdfDevStatePwrPolSystemWakeDeviceWakeInterruptFiredNP TRAP_ON_EVENT },
    { PwrPolNull,        WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemWakeDeviceToD0OtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolDevicePowerRequestFailed DEBUGGED_EVENT },
    { PwrPolPowerUpNotSeen, WdfDevStatePwrPolDeviceD0PowerRequestFailed TRAP_ON_EVENT },
    { PwrPolNull,        WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemWakeDeviceToD0CompletePowerUpOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolDevicePowerRequestFailed DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolStartedWakeCapableOtherStates[] =
{
    { PwrPolSx, WdfDevStatePwrPolStartedWakeCapableCancelTimerForSleep DEBUGGED_EVENT },
    { PwrPolStop, WdfDevStatePwrPolStoppingCancelTimer DEBUGGED_EVENT },
    { PwrPolSurpriseRemove, WdfDevStatePwrPolStoppingCancelTimer DEBUGGED_EVENT },
    { PwrPolS0IdlePolicyChanged, WdfDevStatePwrPolStartedCancelTimer DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolWakeCapableDeviceIdleOtherStates[] =
{
    { PwrPolSx, WdfDevStatePwrPolDeviceIdleSleeping DEBUGGED_EVENT },
    { PwrPolStop, WdfDevStatePwrPolDeviceIdleStopping TRAP_ON_EVENT },
    { PwrPolSurpriseRemove,WdfDevStatePwrPolDeviceIdleStopping DEBUGGED_EVENT },
    { PwrPolS0IdlePolicyChanged, WdfDevStatePwrPolDeviceIdleReturnToActive TRAP_ON_EVENT },
    { PwrPolIoPresent, WdfDevStatePwrPolDeviceIdleReturnToActive DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSleepingWakePowerDownOtherStates[] =
{
    { PwrPolPowerDown, WdfDevStatePwrPolSleepingPowerDownNotProcessed TRAP_ON_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolTimerExpiredWakeCapablePowerDownOtherStates[] =
{
    { PwrPolPowerDown, WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownNotProcessed TRAP_ON_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolTimerExpiredWakeCapableSendWakeOtherStates[] =
{
    { PwrPolWakeFailed, WdfDevStatePwrPolTimerExpiredWakeCompletedDisarm DEBUGGED_EVENT },
    { PwrPolWakeSuccess, WdfDevStatePwrPolTimerExpiredWakeSucceeded DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolTimerExpiredWakeCapableUsbSSOtherStates[] =
{
    { PwrPolSx, WdfDevStatePwrPolStartedWakeCapableSleepingUsbSS DEBUGGED_EVENT },
    { PwrPolStop, WdfDevStatePwrPolStoppingCancelUsbSS TRAP_ON_EVENT },
    { PwrPolSurpriseRemove, WdfDevStatePwrPolStoppingCancelUsbSS DEBUGGED_EVENT },
    { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolWakeCapableUsbSSCompleted DEBUGGED_EVENT },
    { PwrPolS0IdlePolicyChanged, WdfDevStatePwrPolCancelUsbSS DEBUGGED_EVENT },
    { PwrPolIoPresent, WdfDevStatePwrPolCancelUsbSS DEBUGGED_EVENT },
    { PwrPolDevicePowerRequired, WdfDevStatePwrPolCancelUsbSS DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolWaitingArmedOtherStates[] =
{
    { PwrPolSx, WdfDevStatePwrPolCancelingUsbSSForSystemSleep DEBUGGED_EVENT },
    { PwrPolWakeSuccess, WdfDevStatePwrPolWaitingArmedWakeSucceededCancelUsbSS DEBUGGED_EVENT },
    { PwrPolWakeFailed, WdfDevStatePwrPolWaitingArmedWakeFailedCancelUsbSS DEBUGGED_EVENT },
    { PwrPolStop, WdfDevStatePwrPolStoppingD0CancelUsbSS DEBUGGED_EVENT },
    { PwrPolSurpriseRemove, WdfDevStatePwrPolWaitingArmedStoppingCancelUsbSS DEBUGGED_EVENT },
    { PwrPolS0IdlePolicyChanged, WdfDevStatePwrPolWaitingArmedIoPresentCancelUsbSS  DEBUGGED_EVENT },
    { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolIoPresentArmed TRAP_ON_EVENT },
    { PwrPolDevicePowerRequired, WdfDevStatePwrPolWaitingArmedIoPresentCancelUsbSS DEBUGGED_EVENT },
    { PwrPolWakeInterruptFired, WdfDevStatePwrPolWaitingArmedWakeInterruptFired DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolDisarmingWakeForSystemSleepCompletePowerUpOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolPowerUpForSystemSleepFailed DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolCancelingWakeForSystemSleepWakeCanceledOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolPowerUpForSystemSleepFailed DEBUGGED_EVENT },
    { PwrPolPowerUpNotSeen, WdfDevStatePwrPolPowerUpForSystemSleepNotSeen TRAP_ON_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolWokeFromS0OtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolDevicePowerRequestFailed DEBUGGED_EVENT },
    { PwrPolPowerUpNotSeen, WdfDevStatePwrPolDeviceD0PowerRequestFailed TRAP_ON_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolStoppingResetDeviceOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolStoppingResetDeviceFailed DEBUGGED_EVENT },
    { PwrPolPowerUpNotSeen, WdfDevStatePwrPolDeviceD0PowerRequestFailed TRAP_ON_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolStoppingResetDeviceCompletePowerUpOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolStoppingResetDeviceFailed DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolStoppingD0OtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolStoppingD0Failed DEBUGGED_EVENT },
    { PwrPolPowerUpNotSeen, WdfDevStatePwrPolDeviceD0PowerRequestFailed TRAP_ON_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolStoppingDisarmWakeOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolStoppingD0Failed DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolStoppingDisarmWakeCancelWakeOtherStates[] =
{
    { PwrPolWakeSuccess,     WdfDevStatePwrPolStoppingDisarmWakeWakeCanceled DEBUGGED_EVENT },
    { PwrPolNull,            WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolStartedOtherStates[] =
{
    { PwrPolStop, WdfDevStatePwrPolStopping DEBUGGED_EVENT },
    { PwrPolSurpriseRemove, WdfDevStatePwrPolStopping DEBUGGED_EVENT },
    { PwrPolS0IdlePolicyChanged, WdfDevStatePwrPolStartingDecideS0Wake DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolStartedWaitForIdleTimeoutOtherStates[] =
{
    { PwrPolStop, WdfDevStatePwrPolStoppingWaitForIdleTimeout TRAP_ON_EVENT },
    { PwrPolSurpriseRemove, WdfDevStatePwrPolStoppingWaitForIdleTimeout TRAP_ON_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolIoPresentArmedOtherStates[] =
{
    { PwrPolWakeSuccess, WdfDevStatePwrPolIoPresentArmedWakeCanceled DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolIoPresentArmedWakeCanceledOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolDevicePowerRequestFailed DEBUGGED_EVENT },
    { PwrPolPowerUpNotSeen, WdfDevStatePwrPolDeviceD0PowerRequestFailed TRAP_ON_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolS0WakeCompletePowerUpOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolDevicePowerRequestFailed DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolTimerExpiredWakeCapableWakeSucceededOtherStates[] =
{
    { PwrPolPowerDownFailed, WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownFailedUsbSS DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolTimerExpiredWakeCapableWakeFailedOtherStates[] =
{
    { PwrPolPowerDownFailed, WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownFailedUsbSS DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolTimerExpiredWakeCapablePowerDownFailedCancelWakeOtherStates[] =
{
    { PwrPolWakeFailed, WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownFailedWakeCanceled DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolCancelingWakeForSystemSleepOtherStates[] =
{
    { PwrPolWakeSuccess,     WdfDevStatePwrPolCancelingWakeForSystemSleepWakeCanceled DEBUGGED_EVENT },
    { PwrPolNull,            WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolTimerExpiredWakeCapableWakeArrivedOtherStates[] =
{
    { PwrPolWakeSuccess,    WdfDevStatePwrPolTimerExpiredWakeCapableWakeSucceeded DEBUGGED_EVENT },
    { PwrPolWakeFailed,     WdfDevStatePwrPolTimerExpiredWakeCapableWakeFailed DEBUGGED_EVENT },
    { PwrPolPowerDownFailed, WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownFailedCancelWake DEBUGGED_EVENT },
    { PwrPolWakeInterruptFired, WdfDevStatePwrPolTimerExpiredWakeCapableWakeInterruptArrived },
    { PwrPolNull,           WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolTimerExpiredWakeCapableCancelWakeOtherStates[] =
{
    { PwrPolWakeFailed, WdfDevStatePwrPolTimerExpiredWakeCapableWakeCanceled DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolTimerExpiredWakeCompletedPowerDownOtherStates[] =
{
    { PwrPolPowerDownFailed, WdfDevStatePwrPolDevicePowerRequestFailed DEBUGGED_EVENT },
    { PwrPolNull,           WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolTimerExpiredWakeCompletedPowerUpOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolDevicePowerRequestFailed DEBUGGED_EVENT },
    { PwrPolPowerUpNotSeen, WdfDevStatePwrPolDeviceD0PowerRequestFailed TRAP_ON_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSleepingWakeWakeArrivedOtherStates[] =
{
    { PwrPolPowerDownFailed, WdfDevStatePwrPolSleepingWakePowerDownFailed DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemWakeDeviceWakeTriggeredS0OtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolDevicePowerRequestFailed DEBUGGED_EVENT },
    { PwrPolPowerUpNotSeen, WdfDevStatePwrPolDeviceD0PowerRequestFailed TRAP_ON_EVENT },
    { PwrPolNull,           WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemWakeDeviceWakeTriggeredS0NPOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolDevicePowerRequestFailed DEBUGGED_EVENT },
    { PwrPolPowerUpNotSeen, WdfDevStatePwrPolDeviceD0PowerRequestFailed TRAP_ON_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemWakeDeviceWakeCompletePowerUpOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolDevicePowerRequestFailed DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSleepingNoWakePowerDownOtherStates[] =
{
    { PwrPolPowerDown, WdfDevStatePwrPolSleepingPowerDownNotProcessed TRAP_ON_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSleepingNoWakeCompletePowerDownOtherStates[] =
{
    { PwrPolPowerDownFailed, WdfDevStatePwrPolSystemSleepPowerRequestFailed DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSleepingSendWakeOtherStates[] =
{
    { PwrPolWakeSuccess, WdfDevStatePwrPolSleepingNoWakeCompletePowerDown DEBUGGED_EVENT },
    { PwrPolWakeFailed, WdfDevStatePwrPolSleepingNoWakeCompletePowerDown DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSleepingWakeWakeArrivedNPOtherStates[] =
{
    { PwrPolPowerDownFailed, WdfDevStatePwrPolSleepingWakePowerDownFailed DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSleepingWakePowerDownFailedOtherStates[] =
{
    { PwrPolWakeFailed, WdfDevStatePwrPolSleepingWakePowerDownFailedWakeCanceled DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemWakeDeviceWakeEnabledOtherStates[] =
{
    { PwrPolWakeSuccess,    WdfDevStatePwrPolSystemWakeDeviceWakeEnabledWakeCanceled DEBUGGED_EVENT },
    { PwrPolNull,           WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemWakeDeviceWakeEnabledWakeCanceledOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolDevicePowerRequestFailed DEBUGGED_EVENT },
    { PwrPolPowerUpNotSeen, WdfDevStatePwrPolDeviceD0PowerRequestFailed TRAP_ON_EVENT },
    { PwrPolNull,           WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemWakeDeviceWakeEnabledNPOtherStates[] =
{
    { PwrPolWakeSuccess,    WdfDevStatePwrPolSystemWakeDeviceWakeEnabledWakeCanceledNP DEBUGGED_EVENT },
    { PwrPolNull,           WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemWakeDeviceWakeEnabledWakeCanceledNPOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolDevicePowerRequestFailed DEBUGGED_EVENT },
    { PwrPolPowerUpNotSeen, WdfDevStatePwrPolDeviceD0PowerRequestFailed TRAP_ON_EVENT },
    { PwrPolNull,          WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolDevicePowerRequestFailedOtherStates[] =
{
    { PwrPolSurpriseRemove, WdfDevStatePwrPolStopping DEBUGGED_EVENT },
    { PwrPolNull,           WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolStoppingOtherStates[] =
{
    { PwrPolPowerDownFailed, WdfDevStatePwrPolStoppingFailed DEBUGGED_EVENT },
    { PwrPolNull,WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolStoppedOtherStates[] =
{
    { PwrPolRemove,         WdfDevStatePwrPolStoppedRemoving DEBUGGED_EVENT },
    { PwrPolNull,           WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolRestartingOtherStates[] =
{
    { PwrPolPowerUpFailed,  WdfDevStatePwrPolRestartingFailed DEBUGGED_EVENT },
    { PwrPolNull,           WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolStoppingCancelWakeOtherStates[] =
{
    { PwrPolWakeSuccess, WdfDevStatePwrPolStopping DEBUGGED_EVENT },
    { PwrPolNull,        WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolCancelUsbSSOtherStates[] =
{
    { PwrPolStop, WdfDevStatePwrPolStoppingWaitForUsbSSCompletion TRAP_ON_EVENT },
    { PwrPolSurpriseRemove, WdfDevStatePwrPolStoppingWaitForUsbSSCompletion TRAP_ON_EVENT },
    { PwrPolNull,        WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSleepingWakeRevertArmWakeOtherStates[] =
{
    { PwrPolWakeSuccess,    WdfDevStatePwrPolSleepingNoWakeCompletePowerDown DEBUGGED_EVENT },
    { PwrPolNull,           WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSleepingWakeRevertArmWakeNPOtherStates[] =
{
    { PwrPolWakeSuccess,    WdfDevStatePwrPolSleepingNoWakeCompletePowerDown DEBUGGED_EVENT },
    { PwrPolNull,           WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolRemovedOtherStates[] =
{
    { PwrPolRemove, WdfDevStatePwrPolRemoved DEBUGGED_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolWaitingArmedWakeInterruptFiredOtherStates[] =
{
    { PwrPolWakeSuccess, WdfDevStatePwrPolWaitingArmedWakeSucceededCancelUsbSS TRAP_ON_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemWakeDeviceWakeInterruptFiredOtherStates[] =
{
    { PwrPolWakeSuccess, WdfDevStatePwrPolSystemWakeDeviceWakeTriggered TRAP_ON_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemWakeDeviceWakeInterruptFiredNPOtherStates[] =
{
    { PwrPolWakeSuccess, WdfDevStatePwrPolSystemWakeDeviceWakeTriggeredNP TRAP_ON_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolTimerExpiredWakeCapableWakeInterruptArrivedOtherStates[] =
{
    { PwrPolWakeSuccess, WdfDevStatePwrPolTimerExpiredWakeCapableWakeSucceeded TRAP_ON_EVENT },
    { PwrPolPowerDownFailed, WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownFailedWakeInterruptArrived TRAP_ON_EVENT },
    { PwrPolPowerDown, WdfDevStatePwrPolWaitingArmedWakeInterruptFiredDuringPowerDown },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolTimerExpiredWakeCapablePowerDownFailedWakeInterruptArrivedOtherStates[] =
{
    { PwrPolWakeSuccess, WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownFailedWakeCanceled TRAP_ON_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolWaitingArmedWakeInterruptFiredDuringPowerDownOtherStates[] =
{
    { PwrPolWakeSuccess, WdfDevStatePwrPolWaitingArmedWakeSucceededCancelUsbSS TRAP_ON_EVENT },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_STATE_TABLE FxPkgPnp::m_WdfPowerPolicyStates[] =
{
    // transition function,
    // { first target state },
    // other target states
    // queue open,

    // WdfDevStatePwrPolObjectCreated
    { NULL,
      { PwrPolStart, WdfDevStatePwrPolStarting DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolObjectCreatedOtherStates,
      { TRUE,
        PwrPolS0 |  // Sx -> S0 transition on a PDO which was enumerated and
                    // in the disabled state

        PwrPolSx |  // Sx transition right after enumeration
        PwrPolS0IdlePolicyChanged },
    },

    // WdfDevStatePwrPolStarting
    { FxPkgPnp::PowerPolStarting,
      { PwrPolPowerUp, WdfDevStatePwrPolStartingPoweredUp DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolStartingOtherStates,
      { FALSE,
        PwrPolPowerUpFailed   // If the power state machine fails D0 entry upon
                              // initial start, we will get this event here.  The
                              // pnp s.m. will not rely on the pwr pol s.m. to
                              // power down the stack in this case.
      },
    },

    // WdfDevStatePwrPolStartingSucceeded
    { FxPkgPnp::PowerPolStartingSucceeded,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolStartingFailed
    { FxPkgPnp::PowerPolStartingFailed,
      { PwrPolRemove, WdfDevStatePwrPolRemoved DEBUGGED_EVENT },
      NULL,
      { TRUE,
        0 },
    },

    // WdfDevStatePwrPolStartingDecideS0Wake
    { FxPkgPnp::PowerPolStartingDecideS0Wake,
      { PwrPolNull, WdfDevStatePwrPolNull },  // transition out based on wake from S0 enabled
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolStartedIdleCapable
    { FxPkgPnp::PowerPolStartedIdleCapable,
      { PwrPolPowerTimeoutExpired, WdfDevStatePwrPolIdleCapableDeviceIdle DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolStartedIdleCapableOtherStates,
      { TRUE,
        PwrPolS0 | // If the machine send a query Sx and it fails, it will send
                   // an S0 while in the running state (w/out ever sending a true set Sx irp)
        PwrPolWakeArrived | // If the wake request is failed by the bus in between WakeArrived
                            // being posted in TimerExpiredWakeCapapble and being
                            // processed, this event will show up in this state if the idle
                            // setting changed at this exact moment as well
        PwrPolPowerUp |     // posted by power when we are powering up the first time
        PwrPolIoPresent |   // posted by the idle state machine.  If we return
                            // to idle this can happen
        PwrPolDevicePowerNotRequired | // The device-power-not-required event arrived just after an
                                       // I/O request or or an S0-idle policy change caused us to
                                       // become active again. The event is ignored in this case.
        PwrPolDevicePowerRequired // The device-power-required event arrived, but we had already
                                  // powered-up the device proactively because we detected that
                                  // power was needed. The event is ignored in this case.
        },
    },

    // WdfDevStatePwrPolTimerExpiredNoWake
    { FxPkgPnp::PowerPolTimerExpiredNoWake,
      { PwrPolPowerDownIoStopped, WdfDevStatePwrPolTimerExpiredNoWakeCompletePowerDown DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolTimerExpiredNoWakeOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolTimerExpiredNoWakeCompletePowerDown
    { FxPkgPnp::PowerPolTimerExpiredNoWakeCompletePowerDown,
      // NOTE: see the comments PowerPolWaitingArmedUsbSS() about why we query the
      // idle state instead of going directly to WdfDevStatePwrPolWaitingUnarmed
      { PwrPolPowerDown, WdfDevStatePwrPolTimerExpiredNoWakePoweredDownDisableIdleTimer DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolTimerExpiredNoWakeCompletePowerDownOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolWaitingUnarmed
    { NULL,
      { PwrPolIoPresent, WdfDevStatePwrPolWaitingUnarmedQueryIdle DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolWaitingUnarmedOtherStates,
      { TRUE,
        PwrPolS0 | // If the machine send a query Sx and it fails, it will send
                   // an S0 while in the running state (w/out ever sending a true set Sx irp)
        PwrPolDevicePowerNotRequired   // When moving from Sx -> S0, we do not power up the
                                       // device if:
                                       // (UsingSystemManagedIdleTimeout == TRUE) and
                                       // (IdleEnabled == TRUE) and
                                       // (WakeFromS0Capable == FALSE) and
                                       // (PowerUpIdleDeviceOnSystemWake == FALSE).
                                       // In this situation, we declare to the active/idle
                                       // state machine that we are idle, but ignore the
                                       // device-power-not-required event, because we are
                                       // already in Dx.
      },
    },

    // WdfDevStatePwrPolWaitingUnarmedQueryIdle
    { FxPkgPnp::PowerPolWaitingUnarmedQueryIdle,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolS0NoWakePowerUp
    { FxPkgPnp::PowerPolS0NoWakePowerUp,
      { PwrPolPowerUpHwStarted, WdfDevStatePwrPolS0NoWakeCompletePowerUp DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolS0NoWakePowerUpOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolS0NoWakeCompletePowerUp
    { FxPkgPnp::PowerPolS0NoWakeCompletePowerUp,
      { PwrPolPowerUp, WdfDevStatePwrPolStartingDecideS0Wake DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolS0NoWakeCompletePowerUpOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolSystemSleepFromDeviceWaitingUnarmed
    { FxPkgPnp::PowerPolSystemSleepFromDeviceWaitingUnarmed,
      { PwrPolNull, WdfDevStatePwrPolNull }, // transition out based on wake from Sx enabled
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolSystemSleepNeedWake
    { FxPkgPnp::PowerPolSystemSleepNeedWake,
      { PwrPolPowerUpHwStarted, WdfDevStatePwrPolSystemSleepNeedWakeCompletePowerUp DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolSystemSleepNeedWakeOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolSystemSleepNeedWakeCompletePowerUp
    { FxPkgPnp::PowerPolSystemSleepNeedWakeCompletePowerUp,
      { PwrPolPowerUp, WdfDevStatePwrPolSleeping DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolSystemSleepNeedWakeCompletePowerUpOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolSystemSleepPowerRequestFailed
    { FxPkgPnp::PowerPolSystemSleepPowerRequestFailed,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0, },
    },

    // WdfDevStatePwrPolCheckPowerPageable
    { FxPkgPnp::PowerPolCheckPowerPageable,
      { PwrPolNull, WdfDevStatePwrPolNull }, // transition out based on which DO_POWER_Xxx flags set on DO
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolSleepingWakeWakeArrived
    { FxPkgPnp::PowerPolSleepingWakeWakeArrived,
      { PwrPolPowerDown, WdfDevStatePwrPolSystemAsleepWakeArmed DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolSleepingWakeWakeArrivedOtherStates,
      { FALSE,
        PwrPolWakeFailed |  // wake completed before we could cancel
                            // the request, so the event may end up here
        PwrPolWakeSuccess|  // -do-
        PwrPolWakeInterruptFired // Wake interrupt fired when during power
                                 // down as part of system sleep transition

      },
    },

    // WdfDevStatePwrPolSleepingWakeRevertArmWake
    { FxPkgPnp::PowerPolSleepingWakeRevertArmWake,
      { PwrPolWakeFailed, WdfDevStatePwrPolSleepingNoWakeCompletePowerDown DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolSleepingWakeRevertArmWakeOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolSystemAsleepWakeArmed
    { FxPkgPnp::PowerPolSystemAsleepWakeArmed,
      { PwrPolS0, WdfDevStatePwrPolSystemWakeDeviceWakeEnabled DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolSystemAsleepWakeArmedOtherStates,
      { TRUE,
        PwrPolWakeFailed |        // Wake failed while in Sx
        PwrPolIoPresent |         // IO arrived when the machine was going to Sx
        PwrPolPowerTimeoutExpired | // we don't cancel the power timer when we goto
                                    // sleep from an idleable state
        PwrPolDevicePowerNotRequired // Upon receiving Sx, we simulated a device-power-
                                     // not-required, so the device-power-requirement
                                     // state machine sent us this event in response.
                                     // We can drop it because we already powered down.
        },
    },

    // WdfDevStatePwrPolSystemWakeDeviceWakeEnabled
    { FxPkgPnp::PowerPolSystemWakeDeviceWakeEnabled,
      { PwrPolWakeFailed, WdfDevStatePwrPolSystemWakeDeviceWakeEnabledWakeCanceled DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolSystemWakeDeviceWakeEnabledOtherStates,
      { FALSE,
        PwrPolWakeInterruptFired // wake interrupt fired as we were waking the
                                 // device on receiving S0 due to other reasons.
                                 // This event can fire until the wake interrupt
                                 // machine is notified of the power up.
      },
    },

    // WdfDevStatePwrPolSystemWakeDeviceWakeEnabledWakeCanceled
    { FxPkgPnp::PowerPolSystemWakeDeviceWakeEnabledWakeCanceled,
      { PwrPolPowerUpHwStarted, WdfDevStatePwrPolSystemWakeDeviceWakeDisarm DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolSystemWakeDeviceWakeEnabledWakeCanceledOtherStates,
      { FALSE,
        PwrPolWakeInterruptFired // wake interrupt fired as we were waking the
                                 // device on receiving S0 due to other reasons.
                                 // This event can fire until the wake interrupt
                                 // machine is notified of the power up.
      },
    },

    // WdfDevStatePwrPolSystemWakeDeviceWakeDisarm
    { FxPkgPnp::PowerPolSystemWakeDeviceWakeDisarm,
      { PwrPolNull, WdfDevStatePwrPolNull }, // transition out is hardcoded in func
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolSystemWakeDeviceWakeTriggered
    { NULL,
      { PwrPolS0, WdfDevStatePwrPolSystemWakeDeviceWakeTriggeredS0 DEBUGGED_EVENT },
      NULL,
      { TRUE,
        0 },
    },

    // WdfDevStatePwrPolSystemWakeDeviceWakeTriggeredS0
    { FxPkgPnp::PowerPolSystemWakeDeviceWakeTriggeredS0,
      { PwrPolPowerUpHwStarted, WdfDevStatePwrPolSystemWakeDeviceWokeDisarm DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolSystemWakeDeviceWakeTriggeredS0OtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolSystemWakeDeviceWokeDisarm
    { FxPkgPnp::PowerPolSystemWakeDeviceWokeDisarm,
      { PwrPolNull, WdfDevStatePwrPolNull }, // transition out is hardcoded in func
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolSleepingWakeWakeArrivedNP,
    { FxPkgPnp::PowerPolSleepingWakeWakeArrivedNP,
      { PwrPolPowerDown, WdfDevStatePwrPolSystemAsleepWakeArmedNP DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolSleepingWakeWakeArrivedNPOtherStates,
      { FALSE,
        PwrPolWakeFailed |  // wake completed before we could cancel
                            // the request, so the event may end up here
        PwrPolWakeSuccess|  // -do-
        PwrPolWakeInterruptFired // Wake interrupt fired when during power
                                 // down as part of system sleep transition

      },
    },

    // WdfDevStatePwrPolSleepingWakeRevertArmWakeNP,
    { FxPkgPnp::PowerPolSleepingWakeRevertArmWakeNP,
      { PwrPolWakeFailed, WdfDevStatePwrPolSleepingNoWakeCompletePowerDown DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolSleepingWakeRevertArmWakeNPOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolSleepingWakePowerDownFailed
    { FxPkgPnp::PowerPolSleepingWakePowerDownFailed,
      { PwrPolWakeSuccess, WdfDevStatePwrPolSleepingWakePowerDownFailedWakeCanceled TRAP_ON_EVENT },
      FxPkgPnp::m_PowerPolSleepingWakePowerDownFailedOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolSleepingWakePowerDownFailedWakeCanceled
    { FxPkgPnp::PowerPolSleepingWakePowerDownFailedWakeCanceled,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolSystemAsleepWakeArmedNP
    { FxPkgPnp::PowerPolSystemAsleepWakeArmedNP,
      { PwrPolS0, WdfDevStatePwrPolSystemWakeDeviceWakeEnabledNP DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolSystemAsleepWakeArmedNPOtherStates,
      { TRUE,
        PwrPolWakeFailed |        // Wake failed while in Sx
        PwrPolIoPresent |         // IO arrived when the machine was going to Sx
        PwrPolPowerTimeoutExpired // we don't cancel the power timer when we goto
                                  // sleep from an idleable state
        },
    },

    // WdfDevStatePwrPolSystemWakeDeviceWakeEnabledNP
    { FxPkgPnp::PowerPolSystemWakeDeviceWakeEnabledNP,
      { PwrPolWakeFailed, WdfDevStatePwrPolSystemWakeDeviceWakeEnabledWakeCanceledNP DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolSystemWakeDeviceWakeEnabledNPOtherStates,
      { FALSE,
        PwrPolWakeInterruptFired // wake interrupt fired as we were waking the
                                 // device on receiving S0 due to other reasons.
                                 // This event can fire until the wake interrupt
                                 // machine is notified of the power up.
      },
    },

    // WdfDevStatePwrPolSystemWakeDeviceWakeEnabledWakeCanceledNP
    { FxPkgPnp::PowerPolSystemWakeDeviceWakeEnabledWakeCanceledNP,
      { PwrPolPowerUpHwStarted, WdfDevStatePwrPolSystemWakeDeviceWakeDisarmNP DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolSystemWakeDeviceWakeEnabledWakeCanceledNPOtherStates,
      { FALSE,
        PwrPolWakeSuccess | // wake succeeded and completed before we could cancel
                            // the request, so the event ends up here

        PwrPolWakeInterruptFired // wake interrupt fired as we were waking the
                                 // device on receiving S0 due to other reasons.
                                 // This event can fire until the wake interrupt
                                 // machine is notified of the power up.
      },
    },

    // WdfDevStatePwrPolSystemWakeDeviceWakeDisarmNP
    { FxPkgPnp::PowerPolSystemWakeDeviceWakeDisarmNP,
      { PwrPolNull, WdfDevStatePwrPolNull }, // transition out is hardcoded in func
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolSystemWakeDeviceWakeTriggeredNP
    { NULL,
      { PwrPolS0, WdfDevStatePwrPolSystemWakeDeviceWakeTriggeredS0NP DEBUGGED_EVENT },
      NULL,
      { TRUE,
        PwrPolIoPresent     // I/O arrived before S0 arrival
        },
    },

    // WdfDevStatePwrPolSystemWakeDeviceWakeTriggeredS0NP
    { FxPkgPnp::PowerPolSystemWakeDeviceWakeTriggeredS0NP,
      { PwrPolPowerUpHwStarted, WdfDevStatePwrPolSystemWakeDeviceWokeDisarmNP DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolSystemWakeDeviceWakeTriggeredS0NPOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolSystemWakeDeviceWokeDisarmNP
    { FxPkgPnp::PowerPolSystemWakeDeviceWokeDisarmNP,
      { PwrPolNull, WdfDevStatePwrPolNull }, // transition out is hardcoded in func
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolSystemWakeDeviceWakeCompletePowerUp
    { FxPkgPnp::PowerPolSystemWakeDeviceWakeCompletePowerUp,
      { PwrPolPowerUp, WdfDevStatePwrPolStartingDecideS0Wake DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolSystemWakeDeviceWakeCompletePowerUpOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolSleeping
    { FxPkgPnp::PowerPolSleeping,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolSleepingNoWakePowerDown
    { FxPkgPnp::PowerPolSleepingNoWakePowerDown,
      { PwrPolPowerDownIoStopped, WdfDevStatePwrPolSleepingNoWakeCompletePowerDown DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolSleepingNoWakePowerDownOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolSleepingNoWakeCompletePowerDown
    { FxPkgPnp::PowerPolSleepingNoWakeCompletePowerDown,
      { PwrPolPowerDown, WdfDevStatePwrPolSystemAsleepNoWake DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolSleepingNoWakeCompletePowerDownOtherStates,
      { FALSE,
        PwrPolWakeArrived   // wake arrived event posted after the ww irp
                            // completed from SleepingSendWake state. Ignore this
                            // event since wake is already trigged.
      },
    },

    // WdfDevStatePwrPolSleepingNoWakeDxRequestFailed
    { FxPkgPnp::PowerPolSleepingNoWakeDxRequestFailed,
      { PwrPolNull, WdfDevStatePwrPolNull }, // transition out is hardcoded in func
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolSleepingWakePowerDown
    { FxPkgPnp::PowerPolSleepingWakePowerDown,
      { PwrPolPowerDownIoStopped, WdfDevStatePwrPolSleepingSendWake DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolSleepingWakePowerDownOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolSleepingSendWake
    { FxPkgPnp::PowerPolSleepingSendWake,
      { PwrPolWakeArrived, WdfDevStatePwrPolCheckPowerPageable DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolSleepingSendWakeOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolSystemAsleepNoWake
    { FxPkgPnp::PowerPolSystemAsleepNoWake,
      { PwrPolS0, WdfDevStatePwrPolSystemWakeDeviceWakeDisabled DEBUGGED_EVENT },
      NULL,
      { TRUE,
        PwrPolS0IdlePolicyChanged | // Policy changed while the device is in Dx
                                    // because of Sx, we will reevaluate the idle
                                    // settings when we return to S0 anyways
        PwrPolWakeArrived | // If arming for wake from sx failed, the WakeArrived
                            // event that was a part of that arming is dequeued here
        PwrPolIoPresent   | // I/O showed up when going into Sx
        PwrPolPowerTimeoutExpired |
        PwrPolDevicePowerNotRequired // Upon receiving Sx, we simulated a device-power-
                                     // not-required, so the device-power-requirement
                                     // state machine sent us this event in response.
                                     // We can drop it because we already powered down.
       },
    },

    // WdfDevStatePwrPolSystemWakeDeviceWakeDisabled
    { FxPkgPnp::PowerPolSystemWakeDeviceWakeDisabled,
      { PwrPolNull, WdfDevStatePwrPolNull }, // transition out based on wake from S0 enabled
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolSystemWakeDeviceToD0
    { FxPkgPnp::PowerPolSystemWakeDeviceToD0,
      { PwrPolPowerUpHwStarted, WdfDevStatePwrPolSystemWakeDeviceToD0CompletePowerUp DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolSystemWakeDeviceToD0OtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolSystemWakeDeviceToD0CompletePowerUp
    { FxPkgPnp::PowerPolSystemWakeDeviceToD0CompletePowerUp,
      { PwrPolPowerUp, WdfDevStatePwrPolStartingDecideS0Wake DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolSystemWakeDeviceToD0CompletePowerUpOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolSystemWakeQueryIdle
    { FxPkgPnp::PowerPolSystemWakeQueryIdle,
      { PwrPolNull, WdfDevStatePwrPolNull}, // transition out based on timer expiration state
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolStartedWakeCapable
    { FxPkgPnp::PowerPolStartedWakeCapable,
      { PwrPolPowerTimeoutExpired, WdfDevStatePwrPolWakeCapableDeviceIdle DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolStartedWakeCapableOtherStates,
      { TRUE,
        PwrPolS0 | // If the machine send a query Sx and it fails, it will send
                   // an S0 while in the running state (w/out ever sending a true set Sx irp)
        PwrPolPowerUp |
        PwrPolWakeArrived | // If the wake request is failed by the bus in between WakeArrived
                            // being posted in TimerExpiredWakeCapapble and being
                            // processed, this event will show up in this state

        PwrPolWakeSuccess | // wake succeeded while we were trying to cancel it
                            // while coming out of WaitingArmed b/c of io present

        PwrPolWakeFailed |  // wake request failed while we were trying to cancel it
                            // while coming out of WaitingArmed b/c of io present

        PwrPolUsbSelectiveSuspendCallback |
        PwrPolUsbSelectiveSuspendCompleted | // When returning from a success resume
                                             // from USB SS, the completion of the irp will
                                             // occur in the started state

        PwrPolIoPresent |   // posted by the idle state machine.  If we return
                            // to idle this can happen
        PwrPolDevicePowerNotRequired | // The device-power-not-required event arrived just after an
                                       // I/O request or or an S0-idle policy change caused us to
                                       // become active again. The event is ignored in this case.
        PwrPolDevicePowerRequired // The device-power-required event arrived, but we had already
                                  // powered-up the device proactively because we detected that
                                  // power was needed. The event is ignored in this case.
        },
    },

    // WdfDevStatePwrPolTimerExpiredDecideUsbSS
    { FxPkgPnp::PowerPolTimerExpiredDecideUsbSS,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolTimerExpiredWakeCapablePowerDown
    { FxPkgPnp::PowerPolTimerExpiredWakeCapablePowerDown,
      { PwrPolPowerDownIoStopped, WdfDevStatePwrPolTimerExpiredWakeCapableSendWake DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolTimerExpiredWakeCapablePowerDownOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolTimerExpiredWakeCapableSendWake
    { FxPkgPnp::PowerPolTimerExpiredWakeCapableSendWake,
      { PwrPolWakeArrived, WdfDevStatePwrPolTimerExpiredWakeCapableWakeArrived DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolTimerExpiredWakeCapableSendWakeOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolTimerExpiredWakeCapableUsbSS
    { FxPkgPnp::PowerPolTimerExpiredWakeCapableUsbSS,
      { PwrPolUsbSelectiveSuspendCallback, WdfDevStatePwrPolTimerExpiredWakeCapablePowerDown DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolTimerExpiredWakeCapableUsbSSOtherStates,
      { TRUE,
        0 },
    },

    // WdfDevStatePwrPolTimerExpiredWakeCapableWakeArrived
    { FxPkgPnp::PowerPolTimerExpiredWakeCapableWakeArrived,
      { PwrPolPowerDown, WdfDevStatePwrPolWaitingArmedUsbSS DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolTimerExpiredWakeCapableWakeArrivedOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolTimerExpiredWakeCapableCancelWake
    { FxPkgPnp::PowerPolTimerExpiredWakeCapableCancelWake,
      { PwrPolWakeSuccess, WdfDevStatePwrPolTimerExpiredWakeCapableWakeCanceled TRAP_ON_EVENT },
      FxPkgPnp::m_PowerPolTimerExpiredWakeCapableCancelWakeOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolTimerExpiredWakeCapableWakeCanceled
    { FxPkgPnp::PowerPolTimerExpiredWakeCapableWakeCanceled,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolTimerExpiredWakeCapableCleanup
    { FxPkgPnp::PowerPolTimerExpiredWakeCapableCleanup,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolTimerExpiredWakeCompletedPowerDown TRAP_ON_EVENT },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolTimerExpiredWakeCapableDxAllocFailed
    { FxPkgPnp::PowerPolTimerExpiredWakeCapableDxAllocFailed,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolTimerExpiredWakeCapableUndoPowerDown TRAP_ON_EVENT },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolTimerExpiredWakeCompletedPowerDown
    { FxPkgPnp::PowerPolTimerExpiredWakeCompletedPowerDown,
      { PwrPolPowerDown, WdfDevStatePwrPolTimerExpiredWakeCompletedPowerUp DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolTimerExpiredWakeCompletedPowerDownOtherStates,
      { FALSE,
        PwrPolWakeSuccess | // arming callback failed while going into Dx armed for wake from S0
                            // but bus completed wake request with success

        PwrPolWakeArrived   // if the wake request completes before PwrPolWakeArrived
                            // can be processed in the PwrPolTimerExpiredWakeCapableSendWake
                            // state, it will show up here
      },
    },

    // WdfDevStatePwrPolTimerExpiredWakeCompletedPowerUp
    { FxPkgPnp::PowerPolTimerExpiredWakeCompletedPowerUp,
      { PwrPolPowerUpHwStarted, WdfDevStatePwrPolTimerExpiredWakeCompletedHardwareStarted DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolTimerExpiredWakeCompletedPowerUpOtherStates,
      { FALSE,
        PwrPolWakeSuccess | // arming callback failed while going into Dx armed for wake from S0
                            // but bus completed wake request with success

        PwrPolWakeArrived   // if the wake request completes before PwrPolWakeArrived
                            // can be processed in the WdfDevStatePwrPolTimerExpiredWakeCapableSendWake
                            // state, it will show up here
      },
    },

    // WdfDevStatePwrPolWaitingArmedUsbSS
    { FxPkgPnp::PowerPolWaitingArmedUsbSS,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolWaitingArmed
    { NULL,
      { PwrPolIoPresent, WdfDevStatePwrPolWaitingArmedQueryIdle DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolWaitingArmedOtherStates,
      { TRUE,
        PwrPolS0 // If the machine send a query Sx and it fails, it will send
                 // an S0 while in the running state (w/out ever sending a true set Sx irp)
      },
    },

    // WdfDevStatePwrPolWaitingArmedQueryIdle
    { FxPkgPnp::PowerPolWaitingArmedQueryIdle,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolIoPresentArmed
    { FxPkgPnp::PowerPolIoPresentArmed,
      { PwrPolWakeFailed, WdfDevStatePwrPolIoPresentArmedWakeCanceled DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolIoPresentArmedOtherStates,
      { FALSE,
        PwrPolWakeInterruptFired // wake interrupt fired as we were waking the
                                 // device on receiving IO.This event can fire
                                 // until the wake interrupt machine is notified
                                 // of the power up.
      },
    },

    // WdfDevStatePwrPolIoPresentArmedWakeCanceled
    { FxPkgPnp::PowerPolIoPresentArmedWakeCanceled,
      { PwrPolPowerUpHwStarted, WdfDevStatePwrPolS0WakeDisarm DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolIoPresentArmedWakeCanceledOtherStates,
      { FALSE,
        PwrPolWakeSuccess | // The wake status was already processed before entering
                            // this state - indicates that the client driver is
                            // probably propagating a duplicate wake status using
                            // the WdfDeviceIndicateWakeStatus ddi.

        PwrPolWakeFailed  |  // The wake status was already processed before entering
                             // this state - indicates that the client driver is
                             // probably propagating a duplicate wake status using
                             // the WdfDeviceIndicateWakeStatus ddi.

        PwrPolWakeInterruptFired // wake interrupt fired as we were waking the
                                 // device on receiving IO.This event can fire
                                 // until the wake interrupt machine is notified
                                 // of the power up.

      },
    },

    // WdfDevStatePwrPolS0WakeDisarm,
    { FxPkgPnp::PowerPolS0WakeDisarm,
      { PwrPolNull, WdfDevStatePwrPolNull }, // transition out is hardcoded in func
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolS0WakeCompletePowerUp
    { FxPkgPnp::PowerPolS0WakeCompletePowerUp,
      { PwrPolPowerUp, WdfDevStatePwrPolStartingDecideS0Wake DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolS0WakeCompletePowerUpOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolTimerExpiredWakeSucceeded
    { FxPkgPnp::PowerPolTimerExpiredWakeSucceeded,
      { PwrPolNull, WdfDevStatePwrPolNull }, // transition out is hardcoded in func
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolTimerExpiredWakeCompletedDisarm
    { FxPkgPnp::PowerPolTimerExpiredWakeCompletedDisarm,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolTimerExpiredWakeCapableWakeSucceeded
    { NULL,
      { PwrPolPowerDown, WdfDevStatePwrPolWokeFromS0UsbSS DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolTimerExpiredWakeCapableWakeSucceededOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolTimerExpiredWakeCapableWakeFailed
    { NULL,
      { PwrPolPowerDown, WdfDevStatePwrPolWakeFailedUsbSS DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolTimerExpiredWakeCapableWakeFailedOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolWakeFailedUsbSS
    { FxPkgPnp::PowerPolWakeFailedUsbSS,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolIoPresentArmedWakeCanceled TRAP_ON_EVENT },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownFailedCancelWake
    { FxPkgPnp::PowerPolTimerExpiredWakeCapablePowerDownFailedCancelWake,
      { PwrPolWakeSuccess, WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownFailedWakeCanceled DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolTimerExpiredWakeCapablePowerDownFailedCancelWakeOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownFailedWakeCanceled
    { FxPkgPnp::PowerPolTimerExpiredWakeCapablePowerDownFailedWakeCanceled,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownFailedUsbSS
    { FxPkgPnp::PowerPolTimerExpiredWakeCapablePowerDownFailedUsbSS,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolDevicePowerRequestFailed TRAP_ON_EVENT },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolCancelingWakeForSystemSleep
    { FxPkgPnp::PowerPolCancelingWakeForSystemSleep,
      { PwrPolWakeFailed, WdfDevStatePwrPolCancelingWakeForSystemSleepWakeCanceled DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolCancelingWakeForSystemSleepOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolCancelingWakeForSystemSleepWakeCanceled
    { FxPkgPnp::PowerPolCancelingWakeForSystemSleepWakeCanceled,
      { PwrPolPowerUpHwStarted, WdfDevStatePwrPolDisarmingWakeForSystemSleepCompletePowerUp DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolCancelingWakeForSystemSleepWakeCanceledOtherStates,
      { FALSE,
        PwrPolWakeSuccess   // Wake completed successfully right as the transition
                            // from WaitingArmed to goto Sx occurred
      },
    },

    // WdfDevStatePwrPolDisarmingWakeForSystemSleepCompletePowerUp
    { FxPkgPnp::PowerPolDisarmingWakeForSystemSleepCompletePowerUp,
      { PwrPolPowerUp, WdfDevStatePwrPolStartedWakeCapableCancelTimerForSleep DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolDisarmingWakeForSystemSleepCompletePowerUpOtherStates,
      { FALSE,
        0, },
    },

    // WdfDevStatePwrPolPowerUpForSystemSleepFailed
    { FxPkgPnp::PowerPolPowerUpForSystemSleepFailed,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolWokeFromS0UsbSS
    { FxPkgPnp::PowerPolWokeFromS0UsbSS,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolWokeFromS0 DEBUGGED_EVENT },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolWokeFromS0
    { FxPkgPnp::PowerPolWokeFromS0,
      { PwrPolPowerUpHwStarted, WdfDevStatePwrPolWokeFromS0NotifyDriver DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolWokeFromS0OtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolWokeFromS0NotifyDriver
    { FxPkgPnp::PowerPolWokeFromS0NotifyDriver,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolStoppingResetDevice
    { FxPkgPnp::PowerPolStoppingResetDevice,
      { PwrPolPowerUpHwStarted, WdfDevStatePwrPolStoppingResetDeviceCompletePowerUp DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolStoppingResetDeviceOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolStoppingResetDeviceCompletePowerUp
    { FxPkgPnp::PowerPolStoppingResetDeviceCompletePowerUp,
      { PwrPolPowerUp, WdfDevStatePwrPolStopping DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolStoppingResetDeviceCompletePowerUpOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolStoppingResetDeviceFailed
    { FxPkgPnp::PowerPolStoppingResetDeviceFailed,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolStoppingD0
    { FxPkgPnp::PowerPolStoppingD0,
      { PwrPolPowerUpHwStarted, WdfDevStatePwrPolStoppingDisarmWake DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolStoppingD0OtherStates,
      { FALSE,
        PwrPolWakeSuccess | // In the waiting armed state, the wake completed
                            // right after PwrPolStop arrived
        PwrPolWakeFailed    // wake completed before we could cancel
                            // the request, so the event may end up here
      },
    },

    // WdfDevStatePwrPolStoppingD0Failed
    { FxPkgPnp::PowerPolStoppingD0Failed,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolStoppingDisarmWake
    { FxPkgPnp::PowerPolStoppingDisarmWake,
      { PwrPolPowerUp, WdfDevStatePwrPolStoppingDisarmWakeCancelWake DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolStoppingDisarmWakeOtherStates,
      { FALSE,
        PwrPolWakeFailed |  // wake completed before we could cancel
                            // the request, so the event may end up here
        PwrPolWakeSuccess   // -do-
      },
    },

    // WdfDevStatePwrPolStoppingDisarmWakeCancelWake
    { FxPkgPnp::PowerPolStoppingDisarmWakeCancelWake,
      { PwrPolWakeFailed, WdfDevStatePwrPolStoppingDisarmWakeWakeCanceled DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolStoppingDisarmWakeCancelWakeOtherStates,
      { FALSE,
        PwrPolUsbSelectiveSuspendCompleted // pwr pol stopped from a usb SS device
                                           // in Dx and the irp completed when the D0
                                           // irp was sent
      },
    },

    // WdfDevStatePwrPolStoppingDisarmWakeWakeCanceled
    { FxPkgPnp::PowerPolStoppingDisarmWakeWakeCanceled,
      { PwrPolNull, WdfDevStatePwrPolNull}, // transition to Stopping occurs in function
      NULL,
      { FALSE,
        PwrPolUsbSelectiveSuspendCompleted // pwr pol stopped from a usb SS device
                                           // in Dx and the irp completed when the D0
                                           // irp was sent
      },
    },

    // WdfDevStatePwrPolStopping
    { FxPkgPnp::PowerPolStopping,
      { PwrPolPowerDown, WdfDevStatePwrPolStoppingSendStatus DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolStoppingOtherStates,
      { FALSE,
        PwrPolUsbSelectiveSuspendCompleted // pwr pol stopped from a usb SS device
                                           // in Dx and the irp completed when the D0
                                           // irp was sent
      },
    },

    // WdfDevStatePwrPolStoppingFailed
    { FxPkgPnp::PowerPolStoppingFailed,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolStoppingSendStatus
    { FxPkgPnp::PowerPolStoppingSendStatus,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolStoppingCancelTimer
    { FxPkgPnp::PowerPolStoppingCancelTimer,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolStoppingWaitForIdleTimeout
    { NULL,
      { PwrPolPowerTimeoutExpired, WdfDevStatePwrPolStopping DEBUGGED_EVENT },
      NULL,
      { TRUE,
        0 },
    },

    // WdfDevStatePwrPolStoppingCancelUsbSS
    { FxPkgPnp::PowerPolStoppingCancelUsbSS,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolStoppingWaitForUsbSSCompletion
    { NULL,
     { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolStoppingCancelTimer DEBUGGED_EVENT },
     NULL,
     { TRUE,
       0 },
    },

    // WdfDevStatePwrPolStoppingCancelWake
    { FxPkgPnp::PowerPolStoppingCancelWake,
      { PwrPolWakeFailed, WdfDevStatePwrPolStopping DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolStoppingCancelWakeOtherStates,
      { TRUE,
        0 },
    },

    // WdfDevStatePwrPolStopped
    { NULL,
      { PwrPolStart, WdfDevStatePwrPolRestarting DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolStoppedOtherStates,
      { TRUE,
        PwrPolPowerTimeoutExpired | // idle timer fired right before stopping
                                    // pwr policy and before pwr pol could process
                                    // the timeout
        PwrPolIoPresent |         // I/O arrived while transitioning to the
                                  // stopped state
        PwrPolDevicePowerRequired // Due to a power-related failure, we declared our device state
                                  // as failed and stopped the power policy state machine. But
                                  // before stopping the power policy machine, we would have
                                  // declared ourselves as powered-on (maybe fake) in order to
                                  // move the power framework to a consistent state. Since we've
                                  // already declared ourselves as powered-on, we can drop this.
      },
    },

    // WdfDevStatePwrPolCancelUsbSS
    { FxPkgPnp::PowerPolCancelUsbSS,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolStartedCancelTimer DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolCancelUsbSSOtherStates,
      { TRUE,
        PwrPolIoPresent // I/O arrived while we were waiting for the USB idle notification IOCTL
                        // to be completed after we had canceled it. It is okay to drop this
                        // event because we are already in the process to returning to the powered-
                        // up state.
      },
    },

    // WdfDevStatePwrPolStarted
    { FxPkgPnp::PowerPolStarted,
      { PwrPolSx, WdfDevStatePwrPolSleeping DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolStartedOtherStates,
      { TRUE,
        PwrPolS0 | // If the machine send a query Sx and it fails, it will send
                   // an S0 while in the running state (w/out ever sending a true set Sx irp)
        PwrPolWakeArrived | // If the wake request is failed by the bus in between WakeArrived
                            // being posted in TimerExpiredWakeCapapble and being
                            // processed, this event will show up in this state if the idle
                            // setting changed at this exact moment as well
        PwrPolWakeSuccess | // returning from Dx armed for wake from Sx, wake
                            // is completed w/success after S0 irp arrives
        PwrPolPowerUp |
        PwrPolUsbSelectiveSuspendCompleted  | // sent when we move out of the armed
                                              // & idle enabled state into the
                                              // idle disabled state
        PwrPolIoPresent  |// This just indicates that I/O arrived, which is fine here
        PwrPolPowerTimeoutExpired | // this can happen when idle timer is disabled
                                    // due to policy change while powering up.
        PwrPolDevicePowerRequired // idle policy changed when device was powered down
                                  // due to S0-idle. The policy change caused us to power
                                  // up. As part of powering up, the device-power-required
                                  // event arrived. Can drop because we already powered-up.
      },
    },

    // WdfDevStatePwrPolStartedCancelTimer
    { FxPkgPnp::PowerPolStartedCancelTimer,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolStartedWaitForIdleTimeout
    { NULL,
      { PwrPolPowerTimeoutExpired, WdfDevStatePwrPolStartingDecideS0Wake TRAP_ON_EVENT },
      FxPkgPnp::m_PowerPolStartedWaitForIdleTimeoutOtherStates,
      { TRUE,
        0 },
    },

    // WdfDevStatePwrPolStartedWakeCapableCancelTimerForSleep
    { FxPkgPnp::PowerPolStartedWakeCapableCancelTimerForSleep,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolStartedWakeCapableWaitForIdleTimeout
    { NULL,
      { PwrPolPowerTimeoutExpired, WdfDevStatePwrPolSleeping TRAP_ON_EVENT },
      NULL,
      { TRUE,
        0 },
    },

    // WdfDevStatePwrPolStartedWakeCapableSleepingUsbSS
    { FxPkgPnp::PowerPolStartedWakeCapableSleepingUsbSS,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolSleeping DEBUGGED_EVENT },
      NULL,
      { TRUE,
        0 },
    },

    // WdfDevStatePwrPolStartedIdleCapableCancelTimerForSleep
    { FxPkgPnp::PowerPolStartedIdleCapableCancelTimerForSleep,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolStartedIdleCapableWaitForIdleTimeout
    { NULL,
      { PwrPolPowerTimeoutExpired, WdfDevStatePwrPolSleeping TRAP_ON_EVENT },
      NULL,
      { TRUE,
        0 },
    },

    // WdfDevStatePwrPolDeviceD0PowerRequestFailed
    { FxPkgPnp::PowerPolDeviceD0PowerRequestFailed,
      { PwrPolNull, WdfDevStatePwrPolNull},
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolDevicePowerRequestFailed
    { FxPkgPnp::PowerPolDevicePowerRequestFailed,
      { PwrPolStop, WdfDevStatePwrPolStoppingCancelTimer DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolDevicePowerRequestFailedOtherStates,
      { TRUE,
        PwrPolUsbSelectiveSuspendCompleted | // Device was suspened and surprise
                                             // removed while in Dx
        PwrPolS0 | // If the device failed D0Exit while the machine was going into
                   // Sx, then we will get this event when the machine comes back up

        PwrPolWakeArrived | // wake was completed before PwrPolWakeArrived was
                            // sent.  On immediate power down or up, the power
                            // operation failed

        PwrPolWakeFailed | // If the device failed exit d0 after being armed, the
                           // this event will be processed in the failed state
        PwrPolDevicePowerRequired | // We can drop because we already declared ourselves
                                    // as being powered on (fake power-on in order to
                                    // move the power framework to consistent state).
        PwrPolIoPresent // We're being notified that we need to be powered-on because
                        // there is I/O to process, but the device is already in failed
                        // state and is about to be removed.
      },
    },

    // State exists only for the non power policy owner state machine
    // WdfDevStatePwrPolGotoDx
    { NULL,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { TRUE,
        0 },
    },

    // WdfDevStatePwrPolGotoDxInDx
    { NULL,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { TRUE,
        0 },
    },

    // State exists only for the non power policy owner state machine
    // WdfDevStatePwrPolDx
    { NULL,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { TRUE,
        0 },
    },

    // State exists only for the non power policy owner state machine
    // WdfDevStatePwrPolGotoD0
    { NULL,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { TRUE,
        0 },
    },

    // WdfDevStatePwrPolGotoD0InD0
    { NULL,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { TRUE,
        0 },
    },

    // WdfDevStatePwrPolFinal
    { NULL,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { TRUE,
        0 },
    },

    // WdfDevStatePwrPolSleepingPowerDownNotProcessed
    { FxPkgPnp::PowerPolSleepingPowerDownNotProcessed,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownNotProcessed
    { FxPkgPnp::PowerPolTimerExpiredWakeCapablePowerDownNotProcessed,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolTimerExpiredNoWakePowerDownNotProcessed
    { FxPkgPnp::PowerPolTimerExpiredNoWakePowerDownNotProcessed,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolTimerExpiredNoWakePoweredDownDisableIdleTimer
    { FxPkgPnp::PowerPolTimerExpiredNoWakePoweredDownDisableIdleTimer,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolStoppingWaitingForImplicitPowerDown
    { NULL,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolStoppingPoweringUp
    { NULL,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolStoppingPoweringDown
    { NULL,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolPowerUpForSystemSleepNotSeen
    { FxPkgPnp::PowerPolPowerUpForSystemSleepNotSeen,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolWaitingArmedStoppingCancelUsbSS
    { FxPkgPnp::PowerPolWaitingArmedStoppingCancelUsbSS,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolStoppingCancelWake DEBUGGED_EVENT },
      NULL,
      { FALSE,
        PwrPolWakeFailed |  // wake completed before we could cancel
                            // the request, so the event may end up here
        PwrPolWakeSuccess   // -do-
      },
    },

    // WdfDevStatePwrPolWaitingArmedWakeFailedCancelUsbSS
    { FxPkgPnp::PowerPolWaitingArmedWakeFailedCancelUsbSS,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolIoPresentArmedWakeCanceled TRAP_ON_EVENT },
      NULL,
      { FALSE,
          0 },
    },

    // WdfDevStatePwrPolWaitingArmedIoPresentCancelUsbSS
    { FxPkgPnp::PowerPolWaitingArmedIoPresentCancelUsbSS,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolIoPresentArmed DEBUGGED_EVENT },
      NULL,
      { FALSE,
        PwrPolWakeFailed  |  // wake completed before we could cancel
                             // the request, so the event may end up here
        PwrPolWakeSuccess |  // -do-
        PwrPolWakeInterruptFired // wake interrupt fired as we were waking the
                                 // device on receiving IO.This event can fire
                                 // until the wake interrupt machine is notified
                                 // of the power up.
      },
    },

    // WdfDevStatePwrPolWaitingArmedWakeSucceededCancelUsbSS
    { FxPkgPnp::PowerPolWaitingArmedWakeSucceededCancelUsbSS,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolWokeFromS0 DEBUGGED_EVENT },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolCancelingUsbSSForSystemSleep
    { FxPkgPnp::PowerPolCancelingUsbSSForSystemSleep,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolCancelingWakeForSystemSleep DEBUGGED_EVENT },
      NULL,
      { FALSE,
        PwrPolWakeFailed |  // wake completed before we could cancel
                            // the request, so the event may end up here
        PwrPolWakeSuccess   // -do-
      },
    },

    // WdfDevStatePwrPolStoppingD0CancelUsbSS
    { FxPkgPnp::PowerPolStoppingD0CancelUsbSS,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolStoppingD0 TRAP_ON_EVENT },
      NULL,
      { FALSE,
        PwrPolWakeFailed |  // wake completed before we could cancel
                            // the request, so the event may end up here
        PwrPolWakeSuccess   // -do-
      },
    },

    // WdfDevStatePwrPolStartingPoweredUp
    { FxPkgPnp::PowerPolStartingPoweredUp,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolIdleCapableDeviceIdle
    { FxPkgPnp::PowerPolIdleCapableDeviceIdle,
      { PwrPolDevicePowerNotRequired, WdfDevStatePwrPolTimerExpiredNoWake DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolIdleCapableDeviceIdleOtherStates,
      { TRUE,
        0 },
    },

    // WdfDevStatePwrPolDeviceIdleReturnToActive
    { FxPkgPnp::PowerPolDeviceIdleReturnToActive,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolDeviceIdleSleeping
    { FxPkgPnp::PowerPolDeviceIdleSleeping,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolDeviceIdleStopping
    { FxPkgPnp::PowerPolDeviceIdleStopping,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolTimerExpiredNoWakeUndoPowerDown
    { FxPkgPnp::PowerPolTimerExpiredNoWakeUndoPowerDown,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolWakeCapableDeviceIdle
    { FxPkgPnp::PowerPolWakeCapableDeviceIdle,
      { PwrPolDevicePowerNotRequired, WdfDevStatePwrPolTimerExpiredDecideUsbSS DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolWakeCapableDeviceIdleOtherStates,
      { TRUE,
        PwrPolDevicePowerRequired // The device-power-required event arrived, but we had already
                                  // powered-up the device proactively because we detected that
                                  // power was needed. And after we powered up, we become idle
                                  // again and arrived in our current state before the device-
                                  // power-required event was received. The event is ignored in
                                  // this case.
      },
    },

    // WdfDevStatePwrPolWakeCapableUsbSSCompleted
    { FxPkgPnp::PowerPolWakeCapableUsbSSCompleted,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolTimerExpiredWakeCapableUndoPowerDown
    { FxPkgPnp::PowerPolTimerExpiredWakeCapableUndoPowerDown,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolTimerExpiredWakeCompletedHardwareStarted
    { FxPkgPnp::PowerPolTimerExpiredWakeCompletedHardwareStarted,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolStoppedRemoving
    { FxPkgPnp::PowerPolStoppedRemoving,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolRemoved
    { FxPkgPnp::PowerPolRemoved,
      { PwrPolStart, WdfDevStatePwrPolStarting DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolRemovedOtherStates,
      { TRUE,
        PwrPolSx | // device is disabled (must be a PDO) and then the machine
                   // moves into an Sx state
        PwrPolS0 |  // driver failed power up when moving out of S0 Dx idle
                    // state and system resumed after failure
        PwrPolS0IdlePolicyChanged // driver changes S0 idle settings while being
                                  // removed
      },
    },

    // WdfDevStatePwrPolRestarting
    { FxPkgPnp::PowerPolRestarting,
      { PwrPolPowerUp, WdfDevStatePwrPolStartingSucceeded DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolRestartingOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolRestartingFailed
    { FxPkgPnp::PowerPolRestartingFailed,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolStartingPoweredUpFailed
    { FxPkgPnp::PowerPolStartingPoweredUpFailed,
      { PwrPolPowerDown, WdfDevStatePwrPolStartingFailed DEBUGGED_EVENT },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolTimerExpiredNoWakeReturnToActive
    { FxPkgPnp::PowerPolTimerExpiredNoWakeReturnToActive,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolWaitingArmedWakeInterruptFired
    { FxPkgPnp::PowerPolWaitingArmedWakeInterruptFired,
      { PwrPolWakeFailed, WdfDevStatePwrPolWaitingArmedWakeSucceededCancelUsbSS DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolWaitingArmedWakeInterruptFiredOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolSystemWakeDeviceWakeInterruptFired
    { FxPkgPnp::PowerPolSystemWakeDeviceWakeInterruptFired,
      { PwrPolWakeFailed, WdfDevStatePwrPolSystemWakeDeviceWakeTriggered TRAP_ON_EVENT },
      FxPkgPnp::m_PowerPolSystemWakeDeviceWakeInterruptFiredOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolSystemWakeDeviceWakeInterruptFiredNP
    { FxPkgPnp::PowerPolSystemWakeDeviceWakeInterruptFiredNP,
      { PwrPolWakeFailed, WdfDevStatePwrPolSystemWakeDeviceWakeTriggeredNP TRAP_ON_EVENT },
      FxPkgPnp::m_PowerPolSystemWakeDeviceWakeInterruptFiredNPOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolTimerExpiredWakeCapableWakeInterruptArrived
    { FxPkgPnp::PowerPolTimerExpiredWakeCapableWakeInterruptArrived,
      { PwrPolWakeFailed, WdfDevStatePwrPolTimerExpiredWakeCapableWakeSucceeded DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolTimerExpiredWakeCapableWakeInterruptArrivedOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownFailedWakeInterruptArrived
    { NULL,
      { PwrPolWakeFailed, WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownFailedWakeCanceled DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolTimerExpiredWakeCapablePowerDownFailedWakeInterruptArrivedOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolWaitingArmedWakeInterruptFiredDuringPowerDown
    { NULL,
      { PwrPolWakeFailed, WdfDevStatePwrPolWaitingArmedWakeSucceededCancelUsbSS DEBUGGED_EVENT },
      FxPkgPnp::m_PowerPolWaitingArmedWakeInterruptFiredDuringPowerDownOtherStates,
      { FALSE,
        0 },
    },

    // WdfDevStatePwrPolNull
    // *** no entry for this state ***
};

// @@SMVERIFY_SPLIT_END

VOID
FxPkgPnp::PowerPolicyCheckAssumptions(
    VOID
    )
/*++

Routine Description:
    This routine is never actually called by running code, it just has
    WDFCASSERTs who upon failure, would not allow this file to be compiled.

    DO NOT REMOVE THIS FUNCTION just because it is not callec by any running
    code.

Arguments:
    None

Return Value:
    None

  --*/
{
    WDFCASSERT(sizeof(FxPwrPolStateInfo) == sizeof(ULONG));

    WDFCASSERT((sizeof(m_WdfPowerPolicyStates)/sizeof(m_WdfPowerPolicyStates[0]))
                 ==
               (WdfDevStatePwrPolNull - WdfDevStatePwrPolObjectCreated));

    // we assume these are the same length when we update the history index
    WDFCASSERT((sizeof(m_PowerPolicyMachine.m_Queue)/
                sizeof(m_PowerPolicyMachine.m_Queue[0]))
               ==
               (sizeof(m_PowerPolicyMachine.m_States.History)/
                sizeof(m_PowerPolicyMachine.m_States.History[0])));
}

CfxDevice *
IdleTimeoutManagement::GetDevice(
    VOID
    )
{
    IdlePolicySettings * idlePolicySettings = NULL;
    FxPowerPolicyOwnerSettings * ppoSettings = NULL;
    FxPkgPnp * pPkgPnp = NULL;

    idlePolicySettings = (IdlePolicySettings *) CONTAINING_RECORD(
                                                            this,
                                                            IdlePolicySettings,
                                                            m_TimeoutMgmt
                                                            );
    ppoSettings = (FxPowerPolicyOwnerSettings *) CONTAINING_RECORD(
                                                    idlePolicySettings,
                                                    FxPowerPolicyOwnerSettings,
                                                    m_IdleSettings
                                                    );
    pPkgPnp = ppoSettings->m_PkgPnp;

    return pPkgPnp->GetDevice();
}

IdleTimeoutManagement::IdleTimeoutStatusUpdateResult
IdleTimeoutManagement::UpdateIdleTimeoutStatus(
    __in IdleTimeoutManagement::IdleTimeoutStatusFlag Flag
    )
{
    LONG idleTimeoutStatusSnapshot;
    LONG updatedIdleTimeoutStatus;
    LONG preInterlockedIdleTimeoutStatus;

    //
    // Take a snapshot of the idle timeout management status
    //
    idleTimeoutStatusSnapshot = m_IdleTimeoutStatus;

    //
    // Verify that the flag we're trying to set is not already set
    //
    if (0 != (idleTimeoutStatusSnapshot & Flag)) {
        return IdleTimeoutStatusFlagAlreadySet;
    }

    //
    // Verify that the idle timeout management status is not already
    // "frozen"
    //
    if (0 != (idleTimeoutStatusSnapshot & IdleTimeoutStatusFrozen)) {
        //
        // It was too late to set the flag. The flag value was already
        // "frozen".
        //
        return IdleTimeoutStatusFlagsAlreadyFrozen;
    }

    //
    // Try to set the flag
    //
    updatedIdleTimeoutStatus = idleTimeoutStatusSnapshot | Flag;

    preInterlockedIdleTimeoutStatus = InterlockedCompareExchange(
                                                &m_IdleTimeoutStatus,
                                                updatedIdleTimeoutStatus,
                                                idleTimeoutStatusSnapshot
                                                );
    if (preInterlockedIdleTimeoutStatus != idleTimeoutStatusSnapshot) {

        if (0 != (preInterlockedIdleTimeoutStatus &
                                IdleTimeoutStatusFrozen)) {
            //
            // It was too late to set the flag. The flag value was already
            // "frozen".
            //
            return IdleTimeoutStatusFlagsAlreadyFrozen;
        }
        else {
            //
            // Idle timeout management status is being changed by multiple
            // threads in parallel. This is not supported.
            //
            return IdleTimeoutStatusFlagsUnexpected;
        }
    }

    //
    // We have successfully set the flag
    //
    return IdleTimeoutStatusFlagsUpdated;
}

NTSTATUS
IdleTimeoutManagement::UseSystemManagedIdleTimeout(
    __in PFX_DRIVER_GLOBALS DriverGlobals
    )
{
    NTSTATUS status;
    IdleTimeoutStatusUpdateResult statusUpdateResult;
    CfxDevice * device;

    //
    // First check if we are running on Windows 8 or above
    //
    if (_SystemManagedIdleTimeoutAvailable()) {

        //
        // Get the device object so we can use it for logging
        //
        device = GetDevice();

        //
        // Try to update the flag that specifies that the power framework should
        // determine the idle timeout.
        //
        statusUpdateResult = UpdateIdleTimeoutStatus(IdleTimeoutSystemManaged);

        switch (statusUpdateResult) {
            case IdleTimeoutStatusFlagsAlreadyFrozen:
            {
                //
                // Status is already frozen. Too late to update it.
                //
                status = STATUS_INVALID_DEVICE_REQUEST;
                DoTraceLevelMessage(
                    DriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                    "WDFDEVICE %p !devobj %p If the power framework is made "
                    "responsible for determining the idle timeout, then the "
                    "first call to assign S0-idle policy must occur before the "
                    "first start IRP is completed. However, in this case, it "
                    "occurred after the first start IRP was completed. "
                    "%!STATUS!.",
                    device->GetHandle(),
                    device->GetDeviceObject(),
                    status
                    );
                FxVerifierDbgBreakPoint(DriverGlobals);
            }
            break;

            case IdleTimeoutStatusFlagsUnexpected:
            {
                //
                // Status being updated from multiple threads in parallel. Not
                // supported.
                //
                status = STATUS_INVALID_DEVICE_REQUEST;
                DoTraceLevelMessage(
                    DriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                    "WDFDEVICE %p !devobj %p Calls to assign S0-idle settings "
                    "and to specify power framework settings are happening in "
                    "parallel. The driver needs to serialize these calls with "
                    "respect to each other. %!STATUS!.",
                    device->GetHandle(),
                    device->GetDeviceObject(),
                    status
                    );
                FxVerifierDbgBreakPoint(DriverGlobals);
            }
            break;

            case IdleTimeoutStatusFlagAlreadySet:
            {
                //
                // Status flag was already set. This should never happen for the
                // IdleTimeoutSystemManaged flag. The caller ensures this.
                //
                ASSERTMSG(
                    "IdleTimeoutManagement::UseSystemManagedIdleTimeout was "
                    "called more than once\n", FALSE);
            }

            //
            //     Fall through
            //
            //      ||  ||  ||
            //      \/  \/  \/
            //
            case IdleTimeoutStatusFlagsUpdated:
            {
                status = STATUS_SUCCESS;
            }
            break;

            default:
            {
                ASSERTMSG("Unexpected IdleTimeoutStatusUpdateResult value\n",
                          FALSE);
                status = STATUS_INTERNAL_ERROR;
            }
        }

    } else {
        //
        // If we're not running on Windows 8 or above, then there is nothing to
        // do.
        //
        status = STATUS_SUCCESS;
    }

    return status;
}

VOID
IdleTimeoutManagement::FreezeIdleTimeoutManagementStatus(
    __in PFX_DRIVER_GLOBALS DriverGlobals
    )
{
    LONG idleTimeoutSnapshot;
    LONG idleTimeoutStatus;
    LONG idleTimeoutPreviousStatus;
    CfxDevice * device;

    //
    // Get the device object so we can use it for logging
    //
    device = GetDevice();

    //
    // Take a snapshot of the idle timeout management status
    //
    idleTimeoutSnapshot = m_IdleTimeoutStatus;

    //
    // Set the bit that freezes the status
    //
    idleTimeoutStatus = idleTimeoutSnapshot | IdleTimeoutStatusFrozen;

    //
    // Update the status
    //
    idleTimeoutPreviousStatus = InterlockedExchange(&m_IdleTimeoutStatus,
                                                    idleTimeoutStatus);

    if (idleTimeoutPreviousStatus != idleTimeoutSnapshot) {
        //
        // An update of idle timeout status is racing with the freezing of idle
        // timeout status
        //
        DoTraceLevelMessage(
            DriverGlobals, TRACE_LEVEL_WARNING, TRACINGPNP,
            "WDFDEVICE %p !devobj %p The driver's S0-idle settings and/or power"
            " framework settings did not take effect because they were supplied"
            " too late. The driver must ensure that the settings are provided "
            "before the first start IRP is completed.",
            device->GetHandle(),
            device->GetDeviceObject()
            );
        FxVerifierDbgBreakPoint(DriverGlobals);
    }

    //
    // If the driver has specified power framework settings and system managed
    // idle timeout is available on this OS, then the driver must have opted for
    // system managed idle timeout.
    //
    if ((0 != (idleTimeoutStatus & IdleTimeoutPoxSettingsSpecified)) &&
        (_SystemManagedIdleTimeoutAvailable()) &&
        (0 == (idleTimeoutStatus & IdleTimeoutSystemManaged))) {
        DoTraceLevelMessage(
            DriverGlobals, TRACE_LEVEL_WARNING, TRACINGPNP,
            "WDFDEVICE %p !devobj %p The driver specified power framework "
            "settings, but did not opt for system-managed idle timeout.",
            device->GetHandle(),
            device->GetDeviceObject()
            );
        FxVerifierDbgBreakPoint(DriverGlobals);
    }
}

BOOLEAN
IdleTimeoutManagement::UsingSystemManagedIdleTimeout(
    VOID
    )
{
    //
    // If the value of this constant is changed, the debugger extension needs
    // to be fixed as well.
    //
    C_ASSERT(0x2 == IdleTimeoutSystemManaged);

    return (0 != (m_IdleTimeoutStatus & IdleTimeoutSystemManaged));
}

BOOLEAN
IdleTimeoutManagement::DriverSpecifiedPowerFrameworkSettings(
    VOID
    )
{
    return (0 != (m_IdleTimeoutStatus & IdleTimeoutPoxSettingsSpecified));
}

NTSTATUS
IdleTimeoutManagement::CommitPowerFrameworkSettings(
    __in PFX_DRIVER_GLOBALS DriverGlobals,
    __in PPOX_SETTINGS PoxSettings
    )
{
    NTSTATUS status;
    IdleTimeoutStatusUpdateResult statusUpdateResult;
    PVOID oldPoxSettings = NULL;
    BOOLEAN settingsSuccessfullySaved = FALSE;
    CfxDevice * device;

    //
    // We should never get here if system-managed idle timeout is not available
    //
    ASSERT(_SystemManagedIdleTimeoutAvailable());

    //
    // Get the device object so we can use it for logging
    //
    device = GetDevice();

    //
    // Try to save the driver's power framework settings
    //
    oldPoxSettings = InterlockedCompareExchangePointer((PVOID*) &m_PoxSettings,
                                                       PoxSettings,
                                                       NULL // Comparand
                                                       );
    if (NULL != oldPoxSettings) {
        //
        // The driver's power framework settings have already been specified
        // earlier. The driver should not be attempting to specify them more
        // than once.
        //
        status = STATUS_INVALID_DEVICE_REQUEST;
        DoTraceLevelMessage(
            DriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDFDEVICE %p !devobj %p The driver attempted to specify power "
            "framework settings more than once. %!STATUS!.",
            device->GetHandle(),
            device->GetDeviceObject(),
            status
            );
        FxVerifierDbgBreakPoint(DriverGlobals);
        goto exit;
    }
    settingsSuccessfullySaved = TRUE;

    //
    // Try to update the flag that indicates that the client driver has
    // specified settings that are to be used when we register with the power
    // framework.
    //
    statusUpdateResult = UpdateIdleTimeoutStatus(
                                IdleTimeoutPoxSettingsSpecified
                                );
    switch (statusUpdateResult) {
        case IdleTimeoutStatusFlagsAlreadyFrozen:
        {
            //
            // Status is already frozen. Too late to update it.
            //
            status = STATUS_INVALID_DEVICE_REQUEST;
            DoTraceLevelMessage(
                DriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "WDFDEVICE %p !devobj %p Power framework settings must be "
                "specified before the first start IRP is completed. %!STATUS!.",
                device->GetHandle(),
                device->GetDeviceObject(),
                status
                );
            FxVerifierDbgBreakPoint(DriverGlobals);
            goto exit;
        }
        break;

        case IdleTimeoutStatusFlagsUnexpected:
        {
            //
            // Status being updated from multiple threads in parallel. Not
            // supported.
            //
            status = STATUS_INVALID_DEVICE_REQUEST;
            DoTraceLevelMessage(
                DriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "WDFDEVICE %p !devobj %p Calls to assign S0-idle settings and "
                "to specify power framework settings are happening in parallel."
                " The driver needs to serialize these calls with respect to "
                "each other. %!STATUS!.",
                device->GetHandle(),
                device->GetDeviceObject(),
                status
                );
            FxVerifierDbgBreakPoint(DriverGlobals);
            goto exit;
        }
        break;

        case IdleTimeoutStatusFlagAlreadySet:
        {
            //
            // Status flag was already set. This should never happen for the
            // IdleTimeoutPoxSettingsSpecified flag because we have logic in the
            // beginning of this function to ensure that only the first caller
            // attempts to set this flag.
            //
            ASSERTMSG(
                "Attempt to set the IdleTimeoutPoxSettingsSpecified flag more "
                "than once\n", FALSE);
            status = STATUS_INTERNAL_ERROR;
            goto exit;
        }

        case IdleTimeoutStatusFlagsUpdated:
        {
            status = STATUS_SUCCESS;
        }
        break;

        default:
        {
            ASSERTMSG("Unexpected IdleTimeoutStatusUpdateResult value\n",
                      FALSE);
            status = STATUS_INTERNAL_ERROR;
            goto exit;
        }
    }

    //
    // If we get here, we must have a successful status
    //
    ASSERT(STATUS_SUCCESS == status);

exit:
    if (FALSE == NT_SUCCESS(status)) {
        if (settingsSuccessfullySaved) {
            //
            // Since a failure has occurred, we must reset the pointer to the
            // power framework settings.
            //
            m_PoxSettings = NULL;
        }
    }
    return status;
}

PolicySettings::~PolicySettings()
{







}

FxPowerPolicyMachine::FxPowerPolicyMachine(
    VOID
    ) : FxThreadedEventQueue(FxPowerPolicyEventQueueDepth)
{
    m_Owner = NULL;

    RtlZeroMemory(&m_Queue[0], sizeof(m_Queue));
    RtlZeroMemory(&m_States, sizeof(m_States));

    m_States.History[IncrementHistoryIndex()] = WdfDevStatePwrPolObjectCreated;

    m_SingularEventsPresent = 0x0;
}

FxPowerPolicyMachine::~FxPowerPolicyMachine(
    VOID
    )
{
    if (m_Owner != NULL) {
        delete m_Owner;
        m_Owner = NULL;
    }
}

_Must_inspect_result_
NTSTATUS
FxPowerPolicyMachine::InitUsbSS(
    VOID
    )
{
    FxUsbIdleInfo* pInfo;
    NTSTATUS status;

    //
    // The field is already set, we are good to go
    //
    if (m_Owner->m_UsbIdle != NULL) {
        return STATUS_SUCCESS;
    }

    pInfo = new (m_PkgPnp->GetDriverGlobals()) FxUsbIdleInfo(m_PkgPnp);

    if (pInfo == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = pInfo->Initialize();
    if (!NT_SUCCESS(status)) {
        delete pInfo;
        return status;
    }

    if (InterlockedCompareExchangePointer((PVOID*) &m_Owner->m_UsbIdle,
                                          pInfo,
                                          NULL) == NULL) {
        //
        // This thread was the one that set the field value.
        //
        DO_NOTHING();
    }
    else {
        //
        // Another thread raced in and beat this thread in setting the field,
        // just delete our allocation and use the other allocated FxUsbIdleInfo.
        //
        delete pInfo;
    }

    return STATUS_SUCCESS;
}

FxPowerPolicyOwnerSettings::FxPowerPolicyOwnerSettings(
    __in FxPkgPnp* PkgPnp
    ) : m_PoxInterface(PkgPnp)
{
    ULONG i;

    m_UsbIdle = NULL;

    m_PkgPnp = PkgPnp;

    //
    // Default every state to D3 except for system working which is D0 by
    // default.
    //
    m_SystemToDeviceStateMap = 0x0;

    for (i = 0; i < PowerSystemMaximum; i++) {
        FxPkgPnp::_SetPowerCapState(i,
                                    i == PowerSystemWorking ? PowerDeviceD0
                                                            : PowerDeviceD3,
                                    &m_SystemToDeviceStateMap);
    }

    m_IdealDxStateForSx = PowerDeviceD3;

    m_RequestedPowerUpIrp = FALSE;
    m_RequestedPowerDownIrp = FALSE;
    m_RequestedWaitWakeIrp = FALSE;
    m_WakeCompletionEventDropped = FALSE;
    m_PowerFailed = FALSE;
    m_CanSaveState = TRUE;

    m_ChildrenCanPowerUp = FALSE;
    m_ChildrenPoweredOnCount = 0;
    m_ChildrenArmedCount = 0;

    m_WaitWakeStatus = STATUS_NOT_SUPPORTED;
    m_SystemWakeSource = FALSE;
    m_WaitWakeCancelCompletionOwnership = CancelOwnershipUnclaimed;

    m_PowerCallbackObject = NULL;
    m_PowerCallbackRegistration = NULL;
}

FxPowerPolicyOwnerSettings::~FxPowerPolicyOwnerSettings(
    VOID
    )
{
    //
    // There are paths which cleanup this object which  do not go through the
    // pnp state machine, so make sure the power object callback fields are
    // freed.  In these paths, we are guaranteed PASSIVE_LEVEL b/c there will
    // be no dangling references which can cause deletion at a greater IRQL.
    //
    CleanupPowerCallback();

    if (m_UsbIdle != NULL) {
        delete m_UsbIdle;
        m_UsbIdle = NULL;
    }
}

VOID
FxPowerPolicyOwnerSettings::CleanupPowerCallback(
    VOID
    )
/*++

Routine Description:
    Cleans up the power state callback registration for this object.

Arguments:
    None

Return Value:
    None

  --*/
{
    if (m_PowerCallbackRegistration != NULL) {
        Mx::UnregisterCallback(m_PowerCallbackRegistration);
        m_PowerCallbackRegistration = NULL;

    }

    if (m_PowerCallbackObject != NULL) {
        Mx::MxDereferenceObject(m_PowerCallbackObject);
        m_PowerCallbackObject = NULL;
    }
}

_Must_inspect_result_
NTSTATUS
FxPowerPolicyOwnerSettings::Init(
    VOID
    )
/*++

Routine Description:
    Initialize the object.  We will try to register a power state callback so
    that we can be informed of when the machine is changing S states.  We are
    interested in system state changes because we need to know when NOT to write
    to the registry (to save wake settings) so that we don't cause a deadlock
    in a non power pageable device while moving into Sx.

Arguments:
    None

Return Value:
    NTSTATUS

  --*/
{
    OBJECT_ATTRIBUTES oa;
    UNICODE_STRING string;
    NTSTATUS status;

    RtlInitUnicodeString(&string, L"\\Callback\\PowerState");
    InitializeObjectAttributes(
        &oa,
        &string,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL
        );

    //
    // Create a callback object, but we do not want to be the first ones
    // to create it (it should be created way before we load in NTOS
    // anyways)
    //
    status = Mx::CreateCallback(&m_PowerCallbackObject, &oa, FALSE, TRUE);

    if (NT_SUCCESS(status)) {
        m_PowerCallbackRegistration = Mx::RegisterCallback(
            m_PowerCallbackObject,
            _PowerStateCallback,
            this
            );

        if (m_PowerCallbackRegistration == NULL) {
            //
            // Non-critical failure, so we'll free the callback object and keep
            // going
            //
            Mx::MxDereferenceObject(m_PowerCallbackObject);
            m_PowerCallbackObject = NULL;
        }
    }

    //
    // Initialize FxPowerIdleMachine
    //
    status = m_PowerIdleMachine.Init();
    if (!NT_SUCCESS(status)) {
        return status;
    }

    return status;
}

VOID
FxPowerPolicyOwnerSettings::_PowerStateCallback(
    __in     PVOID Context,
    __in_opt PVOID Argument1,
    __in_opt PVOID Argument2
    )
/*++

Routine Description:
    Callback invoked by the power subsystem when the system is changing S states

Arguments:
    Context -  FxPowerPolicyOwnerSettings pointer (the "this" pointer)

    Argument1 - Reason why the callback was invoked, we only care about
                PO_CB_SYSTEM_STATE_LOCK

    Argument2 - State transition that is occurring

Return Value:
    None

  --*/
{
    FxPowerPolicyOwnerSettings* pThis;

    pThis = (FxPowerPolicyOwnerSettings*) Context;

    if (Argument1 != (PVOID) PO_CB_SYSTEM_STATE_LOCK) {
        return;
    }

    pThis->m_PkgPnp->m_PowerPolicyMachine.m_StateMachineLock.AcquireLock(
        pThis->m_PkgPnp->GetDriverGlobals(),
        NULL
        );

    if (Argument2 == (PVOID) 0) {
        //
        // Write out the state if necessary before we turn off the paging path.
        //
        pThis->m_PkgPnp->SaveState(TRUE);

        //
        // Exiting S0
        //
        pThis->m_CanSaveState = FALSE;
    }
    else if (Argument2 == (PVOID) 1) {
        //
        // We have reentered S0
        //
        pThis->m_CanSaveState = TRUE;

        //
        // Write out the state if necessary now that the paging path is back
        //
        pThis->m_PkgPnp->SaveState(TRUE);
    }

    pThis->m_PkgPnp->m_PowerPolicyMachine.m_StateMachineLock.ReleaseLock(
        pThis->m_PkgPnp->GetDriverGlobals()
        );
}

/*++

The locking model for the Power policy state machine requires that events be enqueued
possibly at DISPATCH_LEVEL.  It also requires that the state machine be
runnable at PASSIVE_LEVEL.  Consequently, we have two locks, one DISPATCH_LEVEL
lock that guards the event queue and one PASSIVE_LEVEL lock that guards the
state machine itself.

The Power policy state machine has a few constraints that the PnP state machine
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

1)  Acquire the Power policy queue lock.
2)  Enqueue the event.  events are put at the end of the queue.
3)  Drop the Power policy queue lock.
4)  If the thread is running at PASSIVE_LEVEL, skip to step 6.
5)  Queue a work item onto the special power thread.
6)  Attempt to acquire the state machine lock, with a zero-length timeout (*).
7)  If successful, skip to step 9.
8)  Queue a work item onto the special power thread.
9)  Acquire the state machine lock.
10) Acquire the Power policy queue lock.
11) Attempt to dequeue an event.
12) Drop the Power  policyqueue lock.
13) If there was no event to dequeue, drop the state machine lock and exit.
14) Execute the state handler.  This may involve taking one of the other state
    machine queue locks, briefly, to deliver an event.
15) Go to Step 10.

(*) zero length is different then NULL (infinite) being passed for the timeout

Implementing this algorithm requires three functions.

PowerPolicyProcessEvent         -- Implements steps 1-8.
_PowerPolicyProcessEventInner   -- Implements step 9.
PowerPolicyProcessEventInner    -- Implements steps 10-15.

--*/

VOID
FxPkgPnp::PowerPolicyProcessEvent(
    __in FxPowerPolicyEvent Event,
    __in BOOLEAN ProcessOnDifferentThread
    )
/*++

Routine Description:
    This function implements steps 1-8 of the algorithm described above.

Arguments:
    Event - Current Power event

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
    ULONG mask;
    KIRQL irql;

    //
    // Take the lock, raising to DISPATCH_LEVEL.
    //
    m_PowerPolicyMachine.Lock(&irql);

    //
    // If the input Event is any of the events described by PowerSingularEventMask,
    // then check whether it is already queued up. If so, then dont enqueue this
    // Event.
    //
    if (Event & PowerPolSingularEventMask) {
        if ((m_PowerPolicyMachine.m_SingularEventsPresent & Event) == 0x00) {
            m_PowerPolicyMachine.m_SingularEventsPresent |= Event;
        }
        else {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
                "WDFDEVICE 0x%p !devobj 0x%p current pwr pol state "
                "%!WDF_DEVICE_POWER_POLICY_STATE! dropping event "
                "%!FxPowerPolicyEvent! because the Event is already enqueued.",
                m_Device->GetHandle(),
                m_Device->GetDeviceObject(),
                m_Device->GetDevicePowerPolicyState(), Event);

            m_PowerPolicyMachine.Unlock(irql);
            return;
        }
    }

    if (m_PowerPolicyMachine.IsFull()) {
        //
        // The queue is full.  Bail.
        //
        m_PowerPolicyMachine.Unlock(irql);

        ASSERT(!"The Power queue is full.  This shouldn't be able to happen.");
        return;
    }

    if (m_PowerPolicyMachine.IsClosedLocked()) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
            "WDFDEVICE 0x%p !devobj 0x%p current pwr pol state "
            "%!WDF_DEVICE_POWER_POLICY_STATE! dropping event "
            "%!FxPowerPolicyEvent! because of a closed queue",
            m_Device->GetHandle(),
            m_Device->GetDeviceObject(),
            m_Device->GetDevicePowerPolicyState(), Event);

        //
        // The queue is closed.  Bail
        //
        m_PowerPolicyMachine.Unlock(irql);

        return;
    }

    //
    // Enqueue the event.  Whether the event goes on the front
    // or the end of the queue depends on which event it is and if we are the
    // PPO or not.
    //
    // Yes, mask could be a member variable of m_PowerPolicyMachine, but why
    // waste 4 bytes when it is very easy to figure out?
    //
    mask = IsPowerPolicyOwner() ? PwrPolPriorityEventsMask
                                : PwrPolNotOwnerPriorityEventsMask;

    if (Event & mask) {
        //
        // Stick it on the front of the queue, making it the next event that
        // will be processed if, otherwise let these events go by.
        //
        m_PowerPolicyMachine.m_Queue[m_PowerPolicyMachine.InsertAtHead()] = Event;
    }
    else {
        //
        // Stick it on the end of the queue.
        //
        m_PowerPolicyMachine.m_Queue[m_PowerPolicyMachine.InsertAtTail()] = Event;
    }

    //
    // Drop the lock.
    //
    m_PowerPolicyMachine.Unlock(irql);

    //
    // Now, if we are running at PASSIVE_LEVEL, attempt to run the state
    // machine on this thread.  If we can't do that, then queue a work item.
    //
    if (FALSE == ShouldProcessPowerPolicyEventOnDifferentThread(
                    irql,
                    ProcessOnDifferentThread
                    )) {

        LONGLONG timeout = 0;

        status = m_PowerPolicyMachine.m_StateMachineLock.AcquireLock(
            GetDriverGlobals(), &timeout);

        if (FxWaitLockInternal::IsLockAcquired(status)) {
            FxPostProcessInfo info;

            //
            // We now hold the state machine lock.  So call the function that
            // dispatches the next state.
            //
            PowerPolicyProcessEventInner(&info);

            //
            // The pnp state machine should be the only one deleting the object
            //
            ASSERT(info.m_DeleteObject == FALSE);

            m_PowerPolicyMachine.m_StateMachineLock.ReleaseLock(
                GetDriverGlobals());

            info.Evaluate(this);

            return;
        }
    }

    //
    // The tag added above will be released when the work item runs
    //

    //
    // For one reason or another, we couldn't run the state machine on this
    // thread.  So queue a work item to do it.  If m_PnPWorkItemEnqueuing
    // is non-zero, that means that the work item is already being enqueued
    // on another thread.  This is significant, since it means that we can't do
    // anything with the work item on this thread, but it's okay, since the
    // work item will pick up our work and do it.
    //
    m_PowerPolicyMachine.QueueToThread();
}

VOID
FxPkgPnp::_PowerPolicyProcessEventInner(
    __inout FxPkgPnp* This,
    __inout FxPostProcessInfo* Info,
    __in    PVOID Context
    )
{

    UNREFERENCED_PARAMETER(Context);

    //
    // Take the state machine lock.
    //
    This->m_PowerPolicyMachine.m_StateMachineLock.AcquireLock(
        This->GetDriverGlobals()
        );

    //
    // Call the function that will actually run the state machine.
    //
    This->PowerPolicyProcessEventInner(Info);

    //
    // We are being called from the work item and m_WorkItemRunning is > 0, so
    // we cannot be deleted yet.
    //
    ASSERT(Info->SomethingToDo() == FALSE);

    //
    // Now release the lock
    //
    This->m_PowerPolicyMachine.m_StateMachineLock.ReleaseLock(
        This->GetDriverGlobals()
        );
}

VOID
FxPkgPnp::PowerPolicyProcessEventInner(
    __inout FxPostProcessInfo* Info
    )
{
    WDF_DEVICE_POWER_POLICY_STATE newState;
    FxPowerPolicyEvent event;
    ULONG i;
    KIRQL irql;

    if (IsPowerPolicyOwner()) {
        CPPOWER_POLICY_STATE_TABLE entry;

        //
        // Process as many events as we can.
        //
        for ( ; ; ) {
            entry = GetPowerPolicyTableEntry(m_Device->GetDevicePowerPolicyState());

            //
            // Get an event from the queue.
            //
            m_PowerPolicyMachine.Lock(&irql);

            if (m_PowerPolicyMachine.IsEmpty()) {
                m_PowerPolicyMachine.GetFinishedState(Info);

                //
                // The queue is empty.
                //
                m_PowerPolicyMachine.Unlock(irql);
                return;
            }

            event = m_PowerPolicyMachine.m_Queue[m_PowerPolicyMachine.GetHead()];

            //
            // At this point, we need to determine whether we can process this
            // event.
            //
            if (event & PwrPolPriorityEventsMask) {
                //
                // These are always possible to handle.
                //
                DO_NOTHING();
            }
            else {
                //
                // Check to see if this state can handle new events, ie if this
                // is a green dot (queue open) or red dot (queue *not* open) state.
                //
                if (entry->StateInfo.Bits.QueueOpen == FALSE) {
                    //
                    // This state can't handle new events.
                    //
                    m_PowerPolicyMachine.Unlock(irql);
                    return;
                }
            }

            //
            // If the event obtained from the queue was a singular event, then
            // clear the flag to allow other similar events to be put into this
            // queue for processing.
            //
            if (m_PowerPolicyMachine.m_SingularEventsPresent & event) {
               m_PowerPolicyMachine.m_SingularEventsPresent &= ~event;
            }

            m_PowerPolicyMachine.IncrementHead();
            m_PowerPolicyMachine.Unlock(irql);

            //
            // Find the entry in the power policy state table that corresponds
            // to this event.
            //
            newState = WdfDevStatePwrPolNull;

            if (entry->FirstTargetState.PowerPolicyEvent == event) {
                newState = entry->FirstTargetState.TargetState;

                DO_EVENT_TRAP(&entry->FirstTargetState);
            }
            else if (entry->OtherTargetStates != NULL) {
                for (i = 0;
                     entry->OtherTargetStates[i].PowerPolicyEvent != PwrPolNull;
                     i++) {
                    if (entry->OtherTargetStates[i].PowerPolicyEvent == event) {
                        newState = entry->OtherTargetStates[i].TargetState;
                        DO_EVENT_TRAP(&entry->OtherTargetStates[i]);
                        break;
                    }
                }
            }

            if (newState == WdfDevStatePwrPolNull) {
                //
                // This state doesn't respond to the event.  Just throw the event
                // away.
                //
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                    "WDFDEVICE 0x%p !devobj 0x%p current pwr pol state "
                    "%!WDF_DEVICE_POWER_POLICY_STATE! dropping event "
                    "%!FxPowerPolicyEvent!", m_Device->GetHandle(),
                    m_Device->GetDeviceObject(),
                    m_Device->GetDevicePowerPolicyState(), event);

                if ((entry->StateInfo.Bits.KnownDroppedEvents & event) == 0) {
                    COVERAGE_TRAP();

                    DoTraceLevelMessage(
                        GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                        "WDFDEVICE 0x%p !devobj 0x%p current state "
                        "%!WDF_DEVICE_POWER_POLICY_STATE!, policy event "
                        "%!FxPowerPolicyEvent! is not a known dropped "
                        "event, known dropped events are %!FxPowerPolicyEvent!",
                        m_Device->GetHandle(), m_Device->GetDeviceObject(),
                        m_Device->GetDevicePowerPolicyState(),
                        event, entry->StateInfo.Bits.KnownDroppedEvents);


                }

                //
                // Failsafes for events which have required processing in them.
                //
                switch (event) {
                case PwrPolSx:
                    //
                    // The Sx handling code expects that the state machine
                    // complete the Sx irp.  (S0 irps are never pended).  Since
                    // we don't have a state to transition to that will complete
                    // the request, do so now.
                    //
                    // (This can legitimately happen if a PDO is disabled and
                    //  the machines moves into an Sx state.)
                    //
                    PowerPolicyCompleteSystemPowerIrp();
                    break;

                case PwrPolUsbSelectiveSuspendCompleted:
                    //
                    // This state did not handle the event and event got
                    // dropped. However some state is definitely going to wait
                    // for this event. That's why we need m_EventDropped flag.
                    // If we didn't have this flag there will be no way to know
                    // if the event got dropped and some state will end up
                    // waiting for it indefinitely.
                    //
                    m_PowerPolicyMachine.m_Owner->m_UsbIdle->m_EventDropped = TRUE;
                    break;

                case PwrPolUsbSelectiveSuspendCallback:
                    m_PowerPolicyMachine.UsbSSCallbackProcessingComplete();
                    break;

                case PwrPolWakeSuccess:
                case PwrPolWakeFailed:
                    //
                    // This state did not handle the event and event got
                    // dropped. However some state is definitely going to wait
                    // for this event. That's why we need
                    // m_WakeCompletionEventDropped flag. If we didn't have this
                    // flag there will be no way to know if the event got
                    // dropped and some state will end up waiting for it
                    // indefinitely.
                    //
                    m_PowerPolicyMachine.m_Owner->m_WakeCompletionEventDropped = TRUE;
                    break;

                default:
                    DO_NOTHING();
                    break;
                }
            }
            else {
                //
                // Now enter the new state.
                //
                PowerPolicyEnterNewState(newState);
            }
        }
    }
    else {
        //
        // Process as many events as we can.
        //
        for ( ; ; ) {
            CPNOT_POWER_POLICY_OWNER_STATE_TABLE entry;

#pragma prefast(suppress:__WARNING_DEREF_NULL_PTR, "The current power policy state will always be in the table so entry will never be NULL")
            entry = GetNotPowerPolicyOwnerTableEntry(
                m_Device->GetDevicePowerPolicyState()
                );

            //
            // Get an event from the queue.
            //
            m_PowerPolicyMachine.Lock(&irql);

            if (m_PowerPolicyMachine.IsEmpty()) {
                //
                // The queue is empty.
                //
                m_PowerPolicyMachine.Unlock(irql);
                return;
            }

            event = m_PowerPolicyMachine.m_Queue[m_PowerPolicyMachine.GetHead()];

            //
            // At this point, we need to determine whether we can process this
            // event.
            //
            if (event & PwrPolNotOwnerPriorityEventsMask) {
                //
                // These are always possible to handle.
                //
                DO_NOTHING();
            }
            else {
                //
                // Check to see if this state can handle new events, ie if this
                // is a green dot (queue open) or red dot (queue *not* open) state.
                //
                if (entry->QueueOpen == FALSE) {
                    //
                    // This state can't handle new events.
                    //
                    m_PowerPolicyMachine.Unlock(irql);
                    return;
                }
            }

            //
            // If the event obtained from the queue was a singular event, then
            // clear the flag to allow other similar events to be put into this
            // queue for processing.
            //
            if (m_PowerPolicyMachine.m_SingularEventsPresent & event) {
               m_PowerPolicyMachine.m_SingularEventsPresent &= ~event;
            }

            m_PowerPolicyMachine.IncrementHead();
            m_PowerPolicyMachine.Unlock(irql);

            if (entry != NULL && entry->TargetStatesCount > 0) {
                for (i = 0; i < entry->TargetStatesCount; i++) {
                    if (event == entry->TargetStates[i].PowerPolicyEvent) {
                        DO_EVENT_TRAP(&entry->TargetStates[i]);

                        //
                        // Now enter the new state.
                        //
                        NotPowerPolicyOwnerEnterNewState(
                            entry->TargetStates[i].TargetState);
                        break;
                    }
                }
            }
        }
    }
}

VOID
FxPkgPnp::PowerPolicyEnterNewState(
    __in WDF_DEVICE_POWER_POLICY_STATE NewState
    )
/*++

Routine Description:
    This function looks up the handler for a state and then calls it.

Arguments:
    Event - Current power plicy event

Return Value:

    NTSTATUS

--*/
{
    CPPOWER_POLICY_STATE_TABLE entry;
    WDF_DEVICE_POWER_POLICY_STATE currentState, newState;
    WDF_DEVICE_POWER_POLICY_NOTIFICATION_DATA data;
    FxWatchdog watchdog(this);

    currentState = m_Device->GetDevicePowerPolicyState();
    newState = NewState;

    while (newState != WdfDevStatePwrPolNull) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNPPOWERSTATES,
            "WDFDEVICE 0x%p !devobj 0x%p entering power policy state "
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

        entry = GetPowerPolicyTableEntry(currentState);

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
            VALIDATE_PWR_POL_STATE(currentState, newState);

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


/*++

One of the goals of the Driver Framework is to make it really easy to write
a driver for a device which keeps the device in the lowest useful power state
at all times.  This could (and often does) mean that the device remains in the
D0 state whenever the machine is running.  Or it could mean that the device is
only in D0 when there are outstanding IRPs in its queues.  Or it could mean
that the device is in D0 whenever the driver explicitly says that it has to be.

Consequently, the Power Policy state machine has a bunch of states that relate
only to managing the state of the device while the system is running, possibly
allowing the device to "idle-out" to low power states while it isn't being
heavily used.  Once that idle-out process has begun, there needs to be some
way for the Framework I/O Package and the driver itself to tell the Power Policy
engine that the device must, for some time at least, be in the D0 (high-power,
working) state.  The problem is made much harder by the fact that the driver
(or the Framework itself) probably has to stall some operation while the device
is brought back into the D0 state.

So we've created two operations, PowerReference and PowerDereference, which
tell the Power Policy state machine when a device needs to be in the D0 state.
The I/O Package uses these internally, and the driver may as well.  We want
these operations to be as light-weight as possible, so that a driver never
experiences meaningful degradation in performance simply because it chose to
be a good electricity consumer and enabled idle-time power management.
Fortunately, the Framework I/O package can significantly reduce the number of
times that it needs to call these functions by only calling them when the
queue state transitions from empty to non-empty or back again.  A caller within
the driver itself will need to be aware that their usage can be somewhat
expensive.

Furthermore, these functions need to be callable at both PASSIVE_LEVEL and
DISPATCH_LEVEL.  This really necessitates two versions, as a PASSIVE_LEVEL
user within the driver probably would like us to block while the device is
moved into D0, while a DISPATCH_LEVEL user (or the Framework I/O package) would
prefer a much more asynchronous mode of use.

Dealing with these multiple modes involves a fairly complex locking scheme. Here
is a statement of algorithm.

Locks:

A)  Idle Transition Spinlock - DISPATCH_LEVEL
B)  Device in D0 Notification Event - PASSIVE_LEVEL

Device-wide variables:

  I) Power Reference Count - number of outstanding reasons to be in D0.
 II) Idle Timout -- value set by the driver which governs the idle timer
III) Transitioning -- boolean indicating whether we're in the process of
     moving the device from Dx to D0.

PowerDecrement:

1)  Take Idle Transition lock.
3)  Decrement of the device-wide power reference count.
3)  If that's not zero, drop the lock and exit.
4)  Set the driver's Idle Timer.
5)  Drop the Idle Transition lock.

PowerIncrement:

 1) Take Idle Transition lock.
 2) Increment of the device-wide power reference count.
 3) If that's 1, then we're transitioning out of idle.  Goto Step 6.
 4) If Transitioning == TRUE, we need to wait, Goto Step 13.
 5) Drop the Idle Transition lock.  Exit.
 6) Cancel the Idle Timer.  If that was unsuccessful, then the timer was not
    set, which means that the device has either moved out of D0 or it is moving
    out of D0.  Goto Step 8.
 7) Drop the Idle Transition lock.  Exit.
 8) The timer was not succefully cancelled.  This means that we have timed out
    in the past and we need to put the device back in D0.
    Set Transitioning = TRUE.
 9) Drop Idle Transition lock.
10) Reset the D0 Notification Event.
11) Send IoPresent event to the Power Policy state machine.
12) If IRQL == DISPATCH_LEVEL goto Step 15.
13) Wait for the D0 Notification Event.
14) Exit.
15) Note that I/O needs to be restarted later.
16) Exit STATUS_PENDING.

--*/
WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStarting(
    __inout FxPkgPnp* This
    )
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolStarting);

    This->m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.Start();

    This->PowerProcessEvent(PowerImplicitD0);

    //
    // Wait for the successful power up before starting any idle timers.  If
    // the power up fails, we will not send a power policy stop from the pnp
    // engine.
    //
    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStartingPoweredUp(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The power state policy state machine has powered up. Tell the active/idle
    state machine to initialize.

    The device needs to be in D0 before the active/idle state machine registers
    with the power framework. Therefore, we wait until the power state machine
    has brought the device into D0 before we tell the active/idle state machine
    to start. Moving the device into D0 allows us to touch hardware if needed
    (for example, to determine the number of components in the device) before
    registering with the power framework.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

--*/
{
    NTSTATUS status;
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolStartingPoweredUp);

    //
    // The decision regarding whether or not the power framework determines the
    // idle timeout is now "frozen" and cannot be changed unless the device is
    // stopped and restarted.
    //
    This->m_PowerPolicyMachine.m_Owner->
            m_IdleSettings.m_TimeoutMgmt.FreezeIdleTimeoutManagementStatus(
                                                This->GetDriverGlobals()
                                                );

    status = This->m_PowerPolicyMachine.m_Owner->
                        m_PoxInterface.InitializeComponents();
    if (FALSE == NT_SUCCESS(status)) {
        return WdfDevStatePwrPolStartingPoweredUpFailed;
    }

    return WdfDevStatePwrPolStartingSucceeded;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStartingPoweredUpFailed(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    We failed to initialize the device's components. We have already started the
    power state machine, so ask it to run down before we tell the PNP state
    machine to fail device start.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

--*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolStartingPoweredUpFailed);

    This->PowerProcessEvent(PowerImplicitD3);

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStartingSucceeded(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The power policy state machine has successfully started.  Notify the pnp
    state machine that this has occurred and then tell the active/idle state
    machine to move to an active state.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolStartingDecideS0Wake

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolStartingSucceeded);

    This->PnpProcessEvent(PnpEventPwrPolStarted);

    return WdfDevStatePwrPolStartingDecideS0Wake;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStartingFailed(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Attempting to bring the device into the D0 state failed.  Report the status
    to pnp.

Arguments:
    This - instance of the state machine

Return Value
    WdfDevStatePwrPolNull

  --*/
{
    KIRQL irql;

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolStartingFailed);

    This->m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.Stop();

    //
    // We raise IRQL to dispatch level so that pnp is forced onto its own thread
    // to process the PwrPolStartFailed event.  If pnp is on the power thread when
    // it processes the event and it tries to delete the dedicated thread, it
    // will deadlock waiting for the thread its on to exit.
    //
    Mx::MxRaiseIrql(DISPATCH_LEVEL, &irql);
    This->PnpProcessEvent(PnpEventPwrPolStartFailed);
    Mx::MxLowerIrql(irql);

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStartingDecideS0Wake(
    __inout FxPkgPnp* This
    )
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolStartingDecideS0Wake);

    This->PowerPolicyChildrenCanPowerUp();

    //
    // Save idle state if it is dirty.  We check when deciding the S0 state
    // because any change in the S0 idle settings will go through this state.
    //
    This->SaveState(TRUE);

    //
    // If necessary update the idle timeout hint to the power framework
    //
    This->m_PowerPolicyMachine.m_Owner->m_PoxInterface.UpdateIdleTimeoutHint();

    if (This->m_PowerPolicyMachine.m_Owner->m_IdleSettings.Enabled) {
        if (This->m_PowerPolicyMachine.m_Owner->m_IdleSettings.WakeFromS0Capable) {
            //
            // We can idle out and wake from S0
            //
            return WdfDevStatePwrPolStartedWakeCapable;
        }
        else {
            //
            // We can idle out, but not wake from the idle state
            //
            return WdfDevStatePwrPolStartedIdleCapable;
        }
    }

    return WdfDevStatePwrPolStarted;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStartedIdleCapable(
    __inout FxPkgPnp* This
    )
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolStartedIdleCapable);

    //
    // Enable the idle state machine.  This will release any threads who are
    // waiting for the device to return to D0.
    //
    This->m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.EnableTimer();

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolIdleCapableDeviceIdle(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    We are now idle. Tell the active/idle state machine to move us to an idle
    state.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    BOOLEAN canPowerDown;
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolIdleCapableDeviceIdle);

    canPowerDown = This->m_PowerPolicyMachine.m_Owner->
                        m_PoxInterface.DeclareComponentIdle();

    //
    // If we are using driver-managed idle timeout we can power down immediately
    // and so we jump to the next state (that initiates power down) immediately.
    // If we are using system-managed idle timeout, we wait in the current state
    // for device-power-not-required notification.
    //
    return (canPowerDown ?
                WdfDevStatePwrPolTimerExpiredNoWake :
                WdfDevStatePwrPolNull);
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolDeviceIdleReturnToActive(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    We are idle, but still in D0. We need to return to an active state.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolDeviceIdleReturnToActive);

    This->m_PowerPolicyMachine.m_Owner->
            m_PoxInterface.RequestComponentActive();

    return WdfDevStatePwrPolStartedCancelTimer;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolDeviceIdleSleeping(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    We are idle, but still in D0. System is going to a low power state.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolDeviceIdleSleeping);

    //
    // Normally we'd make the component active when we get the device-power-
    // required notification. But we've not yet processed the device-power-not-
    // required notification, so we will not be processing the device-power-
    // required notification either. So let's activate the component before we
    // process the Sx IRP.
    //
    This->m_PowerPolicyMachine.m_Owner->
            m_PoxInterface.RequestComponentActive();

    return WdfDevStatePwrPolStartedIdleCapableCancelTimerForSleep;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolDeviceIdleStopping(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    We are idle, but still in D0. Power policy state machine is being stopped.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolDeviceIdleStopping);

    //
    // Normally we'd make the component active when we get the device-power-
    // required notification. But we've not yet processed the device-power-not-
    // required notification, so we will not be processing the device-power-
    // required notification either. So let's activate the component before we
    // process the stop/remove request.
    //
    This->m_PowerPolicyMachine.m_Owner->
            m_PoxInterface.RequestComponentActive();

    return WdfDevStatePwrPolStoppingCancelTimer;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredNoWake(
    __inout FxPkgPnp* This
    )
{
    BOOLEAN poweredDown;
    NTSTATUS notifyPowerDownStatus;

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolTimerExpiredNoWake);

    //
    // Notify the device power requirement state machine that we are about to
    // power down.
    //
    notifyPowerDownStatus = This->m_PowerPolicyMachine.m_Owner->
                              m_PoxInterface.NotifyDevicePowerDown();
    if (FALSE == NT_SUCCESS(notifyPowerDownStatus)) {
        //
        // We couldn't notify the device power requirement state machine that
        // we are about to power down, because the "device-power-required"
        // notification has already arrived. So we should not power down at this
        // time. Revert back to the started state.
        //
        return WdfDevStatePwrPolTimerExpiredNoWakeReturnToActive;
    }

    poweredDown = This->PowerPolicyCanIdlePowerDown(
        This->m_PowerPolicyMachine.m_Owner->m_IdleSettings.DxState
        );

    if (poweredDown == FALSE) {
        //
        // Upon failure, revert back to the started state.
        //
        return WdfDevStatePwrPolTimerExpiredNoWakeUndoPowerDown;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredNoWakeCompletePowerDown(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device idled out and we sent the Dx request. The power state machine has
    gone as far as stopping I/O is waiting to be notified to complete the Dx
    process.  Send the PowerCompleteDx event to move the device fully into Dx.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    ASSERT_PWR_POL_STATE(This,
                         WdfDevStatePwrPolTimerExpiredNoWakeCompletePowerDown);

    This->PowerProcessEvent(PowerCompleteDx);

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolWaitingUnarmedQueryIdle(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device was in the WaitingUnarmed state and received a PwrPolIoPresent
    event.  Before committing the device to move back to D0, check to see if
    the device has returned to an idle state.  This can easily happen if the
    driver causes a power reference in D0Exit accidentally and the power
    reference is removed before exiting the function.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolWaitingUnarmedQueryIdle);

    //
    // If QueryReturnToIdle returns TRUE, return to the waiting state
    //
    if (This->m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.QueryReturnToIdle()) {
        return WdfDevStatePwrPolWaitingUnarmed;
    }
    else {
        return WdfDevStatePwrPolS0NoWakePowerUp;
    }
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolS0NoWakePowerUp(
    __inout FxPkgPnp* This
    )
{
    NTSTATUS status;

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolS0NoWakePowerUp);

    //
    // Attempt to get back to the D0 state
    //
    This->m_PowerPolicyMachine.m_Owner->
            m_PoxInterface.RequestComponentActive();
    status = This->PowerPolicySendDevicePowerRequest(PowerDeviceD0, Retry);

    if (!NT_SUCCESS(status)) {
        COVERAGE_TRAP();
        return WdfDevStatePwrPolDeviceD0PowerRequestFailed;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolS0NoWakeCompletePowerUp(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device was in a Dx unarmed for wake while in S0 and is now being brought
    into the  D0 state.  The device is currently in a partial D0 state (HW
    started), move it into the full D0 state by sending PowerCompleteD0 to the
    power state machine.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolS0NoWakeCompletePowerUp);

    This->m_PowerPolicyMachine.m_Owner->
                m_PoxInterface.DeviceIsPoweredOn();

    This->PowerProcessEvent(PowerCompleteD0);

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemSleepFromDeviceWaitingUnarmed(
    __inout FxPkgPnp* This
    )
{
    SYSTEM_POWER_STATE systemState;

    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolSystemSleepFromDeviceWaitingUnarmed);

    systemState = This->PowerPolicyGetPendingSystemState();

    if (This->PowerPolicyIsWakeEnabled() &&
        This->PowerPolicyCanWakeFromSystemState(systemState)) {
        return WdfDevStatePwrPolSystemSleepNeedWake;
    }
    else {
        return WdfDevStatePwrPolSystemAsleepNoWake;
    }
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemSleepNeedWake(
    __inout FxPkgPnp* This
    )
{
    NTSTATUS status;
    BOOLEAN result;

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolSystemSleepNeedWake);

    result = This->m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.DisableTimer();
    ASSERT(result);
    UNREFERENCED_PARAMETER(result);

    status = This->PowerPolicySendDevicePowerRequest(PowerDeviceD0, Retry);

    //
    // We are currently in Dx and not armed for wake.  While the current Dx
    // state may not be the same Dx state we would be for Sx, we can't get to
    // D0 to arm ourselves for Sx wake so just leave ourselves as is.
    //
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGPNP,
            "Failed to allocate D0 request to disarm from wake from S0 to allow "
            "arm for wake from Sx, %!STATUS!", status);

        COVERAGE_TRAP();

        //
        // If D0 IRP allocation fails, we don't treat that as an error. Instead,
        // we just let the device remain in Dx without arming it for
        // wake-from-Sx, even though the driver had enabled wake-from-Sx.
        //
        return WdfDevStatePwrPolSystemAsleepNoWake;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemSleepNeedWakeCompletePowerUp(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The machine is going into Sx while the device was in Dx.  We have started
    the D0 process.  The power state machine has moved the device into the HW
    working state and is waiting to be notified to complete the D0 process.
    Send the PowerCompleteD0 event to complete it.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    ASSERT_PWR_POL_STATE(This,
                         WdfDevStatePwrPolSystemSleepNeedWakeCompletePowerUp);

    This->PowerProcessEvent(PowerCompleteD0);

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemSleepPowerRequestFailed(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Power up failed in the power state machine.  Complete the pending system
    power irp with success (system ignores the results) even if the Dx irp
    failed.

Arguments:
    This - instance of the state machine

Return Value:
    new state WdfDevStatePwrPolDevicePowerRequestFailed

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolSystemSleepPowerRequestFailed);

    This->PowerPolicyCompleteSystemPowerIrp();

    return WdfDevStatePwrPolDevicePowerRequestFailed;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolCheckPowerPageable(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Checks to see if the device should move down the power pagable wake path
    or the NP path.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    ULONG flags;
    MxDeviceObject deviceObject;

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolCheckPowerPageable);

    deviceObject.SetObject(This->m_Device->GetDeviceObject());
    flags = deviceObject.GetFlags();

    if (flags & DO_POWER_PAGABLE) {
        ASSERT((flags & DO_POWER_INRUSH) == 0);

        return WdfDevStatePwrPolSleepingWakeWakeArrived;
    }
    else {
        //
        // DO_POWER_INRUSH also gets us to this state, but since it is mutually
        // exclusive with DO_POWER_PAGABLE, we don't need to check for it
        //
        return WdfDevStatePwrPolSleepingWakeWakeArrivedNP;
    }
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSleepingWakeWakeArrived(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device is in partial Dx (I/O has stopped) and will now be armed for wake.
    Complete the going Dx transition by sending a PowerCompleteDx irp to the

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    NTSTATUS status;
    ULONG wakeReason;

    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolSleepingWakeWakeArrived);

    ASSERT(This->PowerPolicyCanWakeFromSystemState(
                This->PowerPolicyGetPendingSystemState()
                ));

    wakeReason = This->PowerPolicyGetCurrentWakeReason();

    status = This->m_PowerPolicyMachine.m_Owner->m_DeviceArmWakeFromSx.Invoke(
        This->m_Device->GetHandle(),
        FLAG_TO_BOOL(wakeReason, FxPowerPolicySxWakeDeviceEnabledFlag),
        FLAG_TO_BOOL(wakeReason, FxPowerPolicySxWakeChildrenArmedFlag)
        );

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDFDEVICE %p Failed to arm for wake from Sx, %!STATUS!",
            This->m_Device->GetHandle(), status);

        return WdfDevStatePwrPolSleepingWakeRevertArmWake;
    }

    //
    // If the PDO is the Power Policy owner, then enable wake at bus, otherwise
    // the power state machine will enable wake at bus.
    //
    if (This->m_Device->IsPdo()) {
        status = This->PowerEnableWakeAtBusOverload();
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "WDFDEVICE %p Failed to Enable Wake at Bus, %!STATUS!",
                This->m_Device->GetHandle(), status);
            return WdfDevStatePwrPolSleepingWakeRevertArmWake;
        }
    }

    This->PowerProcessEvent(PowerCompleteDx);

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSleepingWakeRevertArmWake(
    __inout FxPkgPnp* This
    )
{
    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolSleepingWakeRevertArmWake);

    DoTraceLevelMessage(
        This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
        "reverting arm for wake from Sx due to failure to allocate wait wake "
        "request or wait wake request completed immeidately.  Device will *NOT* "
        "be armed for wake from Sx");

    //
    // Enable calls should be matched with Disable calls even in the failure
    // cases. However, for the Enable wake at bus failure, we do not call the
    // disable wake at bus method as we try to keep the failure behavior
    // consistent with the Power State machine. Only the Device Disarm wake
    // callback will be invoked here.
    //
    This->PowerPolicyDisarmWakeFromSx();

    //
    // attempt to cancel ww
    //
    if (This->PowerPolicyCancelWaitWake() == FALSE &&
        This->m_PowerPolicyMachine.m_Owner->m_WakeCompletionEventDropped) {
        return WdfDevStatePwrPolSleepingNoWakeCompletePowerDown;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemAsleepWakeArmed(
    __inout FxPkgPnp* This
    )
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolSystemAsleepWakeArmed);
    This->PowerPolicyCompleteSystemPowerIrp();

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeDeviceWakeEnabled(
    __inout FxPkgPnp* This
    )
{
    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolSystemWakeDeviceWakeEnabled);

    if (This->PowerPolicyCancelWaitWake() == FALSE &&
        This->m_PowerPolicyMachine.m_Owner->m_WakeCompletionEventDropped) {
        return WdfDevStatePwrPolSystemWakeDeviceWakeEnabledWakeCanceled;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeDeviceWakeInterruptFired(
    __inout FxPkgPnp* This
    )
{
    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolSystemWakeDeviceWakeInterruptFired);

    //
    // Make a note of the fact that system was woken by
    // a wake interrupt of this device
    //
    This->m_SystemWokenByWakeInterrupt = TRUE;

    if (This->PowerPolicyCancelWaitWake() == FALSE &&
        This->m_PowerPolicyMachine.m_Owner->m_WakeCompletionEventDropped) {

        return WdfDevStatePwrPolSystemWakeDeviceWakeTriggered;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeDeviceWakeEnabledWakeCanceled(
    __inout FxPkgPnp* This
    )
{
    NTSTATUS status;

    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolSystemWakeDeviceWakeEnabledWakeCanceled);

    status = This->PowerPolicySendDevicePowerRequest(PowerDeviceD0, Retry);

    if (!NT_SUCCESS(status)) {
        COVERAGE_TRAP();
        return WdfDevStatePwrPolDeviceD0PowerRequestFailed;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeDeviceWakeDisarm(
    __inout FxPkgPnp* This
    )
{
    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolSystemWakeDeviceWakeDisarm);

    //
    // If the PDO is the Power Policy owner, then disable wake at bus, otherwise
    // the power state machine will disable wake at bus.
    //
    if (This->m_Device->IsPdo()) {
        This->PowerDisableWakeAtBusOverload();
    }

    This->PowerPolicyDisarmWakeFromSx();

    return WdfDevStatePwrPolSystemWakeDeviceWakeCompletePowerUp;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeDeviceWakeTriggeredS0(
    __inout FxPkgPnp* This
    )
{
    NTSTATUS status;

    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolSystemWakeDeviceWakeTriggeredS0);

    status = This->PowerPolicySendDevicePowerRequest(PowerDeviceD0, Retry);

    if (!NT_SUCCESS(status)) {
        COVERAGE_TRAP();
        return WdfDevStatePwrPolDeviceD0PowerRequestFailed;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeDeviceWokeDisarm(
    __inout FxPkgPnp* This
    )
{
    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolSystemWakeDeviceWokeDisarm);

    //
    // If the PDO is the Power Policy owner, then disable wake at bus, otherwise
    // the power state machine will enable wake at bus.
    //
    if (This->m_Device->IsPdo()) {
        This->PowerDisableWakeAtBusOverload();
    }

    This->m_PowerPolicyMachine.m_Owner->m_DeviceWakeFromSxTriggered.Invoke(
        This->m_Device->GetHandle()
        );

    This->PowerPolicyDisarmWakeFromSx();

    return WdfDevStatePwrPolSystemWakeDeviceWakeCompletePowerUp;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSleepingWakeWakeArrivedNP(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device is in partial Dx (I/O has stopped) and will now be armed for wake.
    Complete the going Dx transition by sending a PowerCompleteDx irp to the
    power state machine.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    NTSTATUS status;
    ULONG wakeReason;

    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolSleepingWakeWakeArrivedNP);

    ASSERT(This->PowerPolicyCanWakeFromSystemState(
                This->PowerPolicyGetPendingSystemState()
                ));

    wakeReason = This->PowerPolicyGetCurrentWakeReason();

    status = This->m_PowerPolicyMachine.m_Owner->m_DeviceArmWakeFromSx.Invoke(
        This->m_Device->GetHandle(),
        FLAG_TO_BOOL(wakeReason, FxPowerPolicySxWakeDeviceEnabledFlag),
        FLAG_TO_BOOL(wakeReason, FxPowerPolicySxWakeChildrenArmedFlag)
        );

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDFDEVICE %p Failed to arm for wake from Sx, %!STATUS!",
            This->m_Device->GetHandle(), status);

        return WdfDevStatePwrPolSleepingWakeRevertArmWakeNP;
    }

    //
    // If the PDO is the Power Policy owner, then enable wake at bus, otherwise
    // the power state machine will enable wake at bus.
    //
    if (This->m_Device->IsPdo()) {
        status = This->PowerEnableWakeAtBusOverload();
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "WDFDEVICE %p Failed to Enable Wake at Bus, %!STATUS!",
                This->m_Device->GetHandle(), status);
            return WdfDevStatePwrPolSleepingWakeRevertArmWakeNP;
        }
    }

    This->PowerProcessEvent(PowerCompleteDx);

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSleepingWakeRevertArmWakeNP(
    __inout FxPkgPnp* This
    )
{
    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolSleepingWakeRevertArmWakeNP);

    DoTraceLevelMessage(
        This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
        "reverting arm for wake from Sx due to failure to allocate wait wake "
        "request or wait wake request completed immeidately.  Device will *NOT* "
        "be armed for wake from Sx");

    //
    // Enable calls should be matched with Disable calls even in the failure
    // cases. However, for the Enable wake at bus failure, we do not call the
    // disable wake at bus method as we try to keep the failure behavior
    // consistent with the Power State machine. Only the Device Disarm wake
    // callback will be invoked here.
    //
    This->PowerPolicyDisarmWakeFromSx();

    //
    // attempt to cancel ww
    //
    if (This->PowerPolicyCancelWaitWake() == FALSE &&
        This->m_PowerPolicyMachine.m_Owner->m_WakeCompletionEventDropped) {
        return WdfDevStatePwrPolSleepingNoWakeCompletePowerDown;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSleepingWakePowerDownFailed(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Power down failed in the power state machine.  Cancel the wait wake irp
    that was just sent down and revert the arming before moving to the failed
    state.

Arguments:
    This - instance of the state machine.

Return Value:
    new state

  --*/
{
    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolSleepingWakePowerDownFailed);

    if (This->PowerPolicyCancelWaitWake() == FALSE &&
        This->m_PowerPolicyMachine.m_Owner->m_WakeCompletionEventDropped) {
        return WdfDevStatePwrPolSleepingWakePowerDownFailedWakeCanceled;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSleepingWakePowerDownFailedWakeCanceled(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Wait wake irp has been cancelled.  Complete the Sx irp and goto the failed
    state.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolSleepingWakePowerDownFailedWakeCanceled);

    This->PowerPolicyCompleteSystemPowerIrp();

    return WdfDevStatePwrPolDevicePowerRequestFailed;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemAsleepWakeArmedNP(
    __inout FxPkgPnp* This
    )
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolSystemAsleepWakeArmedNP);

    This->PowerPolicyCompleteSystemPowerIrp();

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeDeviceWakeEnabledNP(
    __inout FxPkgPnp* This
    )
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolSystemWakeDeviceWakeEnabledNP);

    if (This->PowerPolicyCancelWaitWake() == FALSE &&
        This->m_PowerPolicyMachine.m_Owner->m_WakeCompletionEventDropped) {
        return WdfDevStatePwrPolSystemWakeDeviceWakeEnabledWakeCanceledNP;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeDeviceWakeInterruptFiredNP(
    __inout FxPkgPnp* This
    )
{
    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolSystemWakeDeviceWakeInterruptFiredNP);

    //
    // Make a notee of the fact that system was woken by
    // a wake interrupt of this device
    //
    This->m_SystemWokenByWakeInterrupt = TRUE;

    if (This->PowerPolicyCancelWaitWake() == FALSE &&
        This->m_PowerPolicyMachine.m_Owner->m_WakeCompletionEventDropped) {
        return WdfDevStatePwrPolSystemWakeDeviceWakeTriggeredNP;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeDeviceWakeEnabledWakeCanceledNP(
    __inout FxPkgPnp* This
    )
{
    NTSTATUS status;

    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolSystemWakeDeviceWakeEnabledWakeCanceledNP);

    status = This->PowerPolicySendDevicePowerRequest(PowerDeviceD0, Retry);

    if (!NT_SUCCESS(status)) {
        COVERAGE_TRAP();
        return WdfDevStatePwrPolDeviceD0PowerRequestFailed;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeDeviceWakeDisarmNP(
    __inout FxPkgPnp* This
    )
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolSystemWakeDeviceWakeDisarmNP);

    //
    // If the PDO is the Power Policy owner, then disable wake at bus, otherwise
    // the power state machine will disable wake at bus.
    //
    if (This->m_Device->IsPdo()) {
        This->PowerDisableWakeAtBusOverload();
    }

    This->PowerPolicyDisarmWakeFromSx();

    return WdfDevStatePwrPolSystemWakeDeviceWakeCompletePowerUp;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeDeviceWakeTriggeredS0NP(
    __inout FxPkgPnp* This
    )
{
    NTSTATUS status;

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolSystemWakeDeviceWakeTriggeredS0NP);

    status = This->PowerPolicySendDevicePowerRequest(PowerDeviceD0, Retry);

    if (!NT_SUCCESS(status)) {
        COVERAGE_TRAP();
        return WdfDevStatePwrPolDeviceD0PowerRequestFailed;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeDeviceWokeDisarmNP(
    __inout FxPkgPnp* This
    )
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolSystemWakeDeviceWokeDisarmNP);

    //
    // If the PDO is the Power Policy owner, then disable wake at bus, otherwise
    // the power state machine will disable wake at bus.
    //
    if (This->m_Device->IsPdo()) {
        This->PowerDisableWakeAtBusOverload();
    }

    This->m_PowerPolicyMachine.m_Owner->m_DeviceWakeFromSxTriggered.Invoke(
        This->m_Device->GetHandle()
        );

    This->PowerPolicyDisarmWakeFromSx();

    return WdfDevStatePwrPolSystemWakeDeviceWakeCompletePowerUp;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeDeviceWakeCompletePowerUp(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The system went into Sx and the device was armed for wake.  The system has
    now returned to S0, the device has started the D0 transition, and the device
    has been disarmed for wake.  We must now complete the D0 transition by sending
    PowerCompleteD0 to the power state machine.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    ASSERT_PWR_POL_STATE(This,
                         WdfDevStatePwrPolSystemWakeDeviceWakeCompletePowerUp);

    //
    // Simulate a device-power-required notification from the power framework.
    // An S0-IRP is essentially equivalent to a device-power-required
    // notification.
    //
    This->m_PowerPolicyMachine.m_Owner->
        m_PoxInterface.SimulateDevicePowerRequired();

    //
    // Notify the device-power-requirement state machine that we are powered on
    //
    This->m_PowerPolicyMachine.m_Owner->
                m_PoxInterface.DeviceIsPoweredOn();

    This->PowerProcessEvent(PowerCompleteD0);

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSleeping(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The machine is going into Sx.  Send a Dx irp to the stack.  The "x" depends
    on if the target system state is one which we can wake the system and
    the device is enabled to wake from Sx.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    NTSTATUS notifyPowerDownStatus;
    SYSTEM_POWER_STATE systemState;

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolSleeping);

    //
    // If the bus/PDO is not in the hibernate path, then verify that all the
    // children have powered down by now.
    //
    if (This->GetUsageCount(WdfSpecialFileHibernation) == 0 &&
        This->m_PowerPolicyMachine.m_Owner->m_ChildrenPoweredOnCount > 0) {
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDFDEVICE %p powering down before child devices have powered down. "
            "This usually indicates a faulty child device that completed the Sx "
            "irp before sending the Dx irp",
            This->m_Device->GetHandle());

        FxVerifierBreakOnDeviceStateError(
            This->m_Device->GetDriverGlobals());
    }

    //
    // Simulate a device-power-not-required notification from the power
    // framework. An Sx-IRP is essentially equivalent to a device-power-not-
    // required notification.
    //
    This->m_PowerPolicyMachine.m_Owner->
        m_PoxInterface.SimulateDevicePowerNotRequired();

    //
    // Notify the device-power-requirement state machine that we are about to
    // power down
    //
    notifyPowerDownStatus = This->m_PowerPolicyMachine.m_Owner->
                              m_PoxInterface.NotifyDevicePowerDown();

    //
    // We simulated a device-power-not-required notification before we notified
    // the device-power-requirement state machine that we are powering down.
    // Therefore, our notification should have succeeded.
    //
    ASSERT(NT_SUCCESS(notifyPowerDownStatus));
    UNREFERENCED_PARAMETER(notifyPowerDownStatus);

    systemState = This->PowerPolicyGetPendingSystemState();

    if (This->PowerPolicyIsWakeEnabled() &&
        This->PowerPolicyCanWakeFromSystemState(systemState)) {
        return WdfDevStatePwrPolSleepingWakePowerDown;
    }
    else {
        return WdfDevStatePwrPolSleepingNoWakePowerDown;
    }
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSleepingNoWakePowerDown(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Machine is going into Sx and the device is not enabled to wake from Sx.
    Request a D3 irp to put the device into a low power state.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    NTSTATUS status;
    DEVICE_POWER_STATE dxState;

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolSleepingNoWakePowerDown);

    dxState = (DEVICE_POWER_STATE)
        This->m_PowerPolicyMachine.m_Owner->m_IdealDxStateForSx;

    if (dxState != PowerDeviceD3) {
        DEVICE_POWER_STATE dxMappedState;

        //
        // Get the lightest Dx state for this Sx state as reported by the
        // device capabilities of the stack.
        //
        dxMappedState = _GetPowerCapState(
            This->PowerPolicyGetPendingSystemState(),
            This->m_PowerPolicyMachine.m_Owner->m_SystemToDeviceStateMap
            );

        //
        // If the ideal desired state is lighter than what the S->D mapping says
        // is the lightest supported D state, use the mapping value instead.
        //
        if (dxState < dxMappedState) {
            dxState = dxMappedState;
        }
    }

    ASSERT(dxState >= PowerDeviceD1 && dxState <= PowerDeviceD3);

    status = This->PowerPolicyPowerDownForSx(dxState, Retry);

    if (!NT_SUCCESS(status)) {
        COVERAGE_TRAP();
        return WdfDevStatePwrPolSleepingNoWakeDxRequestFailed;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSleepingNoWakeCompletePowerDown(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The system has moved into Sx and the device is not armed for wake.  The
    device in partial Dx (I/O has stopped), transition the device into full
    Dx by sending the PowerCompleteDx event to the power state machine.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    ASSERT_PWR_POL_STATE(This,
                         WdfDevStatePwrPolSleepingNoWakeCompletePowerDown);

    This->PowerProcessEvent(PowerCompleteDx);

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSleepingNoWakeDxRequestFailed(
    __inout FxPkgPnp* This
    )
{
    COVERAGE_TRAP();

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolSleepingNoWakeDxRequestFailed);

    This->SetInternalFailure();
    This->PowerPolicyCompleteSystemPowerIrp();

    if (FALSE == This->m_ReleaseHardwareAfterDescendantsOnFailure) {
        This->PnpProcessEvent(PnpEventPowerDownFailed);
    }

    return WdfDevStatePwrPolDevicePowerRequestFailed;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSleepingWakePowerDown(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The system is going into Sx and the device is set to arm itself for wake from
    Sx. Start the power down process by requesting the Dx irp to stop I/O in
    the power state machine.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    NTSTATUS status;

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolSleepingWakePowerDown);

    status = This->PowerPolicyPowerDownForSx(
        This->m_PowerPolicyMachine.m_Owner->m_WakeSettings.DxState, NoRetry
        );

    if (!NT_SUCCESS(status)) {
        COVERAGE_TRAP();
        return WdfDevStatePwrPolSleepingNoWakePowerDown;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSleepingSendWake(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Machine is moving into an Sx state and the device is in a partial Dx (I/O
    stopped) state.  Send a wait wake request so that the device can be armed
    for wake from Sx.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    SYSTEM_POWER_STATE systemState;
    NTSTATUS status;

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolSleepingSendWake);

    //
    // We are in a wake-enabled path, keep wake interrupts connected.
    //
    This->m_WakeInterruptsKeepConnected = TRUE;

    //
    // We use the deepest possible Sx state instead of the current Sx state so
    // that we can handle the FastS4 case where PowerPolicyGetPendingSystemState
    // would return, but we could possible goto S4.  By using the deepest
    // possible state, we can arm for any possible Sx state that we are capable
    // of waking from.
    //
    systemState = This->PowerPolicyGetDeviceDeepestSystemWakeState();

    status = This->PowerPolicySendWaitWakeRequest(systemState);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "Attempting to send wait wake request for EvtDeviceArmWakeFromSx() "
            "failed, %!STATUS!", status);

        COVERAGE_TRAP();

        return WdfDevStatePwrPolSleepingNoWakeCompletePowerDown;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemAsleepNoWake(
    __inout FxPkgPnp* This
    )
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolSystemAsleepNoWake);

    This->PowerPolicyCompleteSystemPowerIrp();

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeDeviceWakeDisabled(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The machine is moving from Sx->S0 and the device was in Dx and not armed for
    wake from Sx.  Determine if the device should remain in Dx after the machine
    has moved into S0.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolSystemWakeDeviceWakeDisabled);

    //
    // We do not attempt to let the device remain in Dx if we are using system-
    // managed idle timeout. This is because an S0 IRP is equivalent to a
    // device-power-required notification from the power framework. In response
    // to it, we need to power up the device and notify the power framework that
    // the device is powered on.
    //
    if ((This->m_PowerPolicyMachine.m_Owner->
          m_IdleSettings.m_TimeoutMgmt.UsingSystemManagedIdleTimeout() == FALSE)
            &&
        (This->m_PowerPolicyMachine.m_Owner->m_IdleSettings.Enabled)
            &&
        (This->m_PowerPolicyMachine.m_Owner->
          m_IdleSettings.WakeFromS0Capable == FALSE)
            &&
        (This->m_PowerPolicyMachine.m_Owner->
          m_IdleSettings.PowerUpIdleDeviceOnSystemWake == FALSE)) {

        return WdfDevStatePwrPolSystemWakeQueryIdle;
    }
    else {
        return WdfDevStatePwrPolSystemWakeDeviceToD0;
    }
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeDeviceToD0(
    __inout FxPkgPnp* This
    )
{
    NTSTATUS status;

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolSystemWakeDeviceToD0);

    status = This->PowerPolicySendDevicePowerRequest(PowerDeviceD0, Retry);
    if (!NT_SUCCESS(status)) {
        COVERAGE_TRAP();
        return WdfDevStatePwrPolDeviceD0PowerRequestFailed;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeDeviceToD0CompletePowerUp(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The machine was in Sx and the device was not armed for wake from Sx.  The
    machine is moving back into S0 and the device is in partial D0 (HW has
    started).  Move the device into full D0 by sending the PowerCompleteD0
    event to the power state machine.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolSystemWakeDeviceToD0CompletePowerUp);

    //
    // Simulate a device-power-not-required notification from the power
    // framework. An S0-IRP is essentially equivalent to a device-power-required
    // notification.
    //
    This->m_PowerPolicyMachine.m_Owner->
        m_PoxInterface.SimulateDevicePowerRequired();

    //
    // Notify the device-power-requirement state machine that we are powered on
    //
    This->m_PowerPolicyMachine.m_Owner->
                m_PoxInterface.DeviceIsPoweredOn();

    This->PowerProcessEvent(PowerCompleteD0);

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeQueryIdle(
    __inout FxPkgPnp* This
    )
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolSystemWakeQueryIdle);

    //
    // This state can be reached only if we are using driver-managed idle
    // timeout.
    //
    ASSERT(
      This->m_PowerPolicyMachine.m_Owner->
          m_IdleSettings.m_TimeoutMgmt.UsingSystemManagedIdleTimeout() == FALSE
      );

    if (This->m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.QueryReturnToIdle()) {
        return WdfDevStatePwrPolWaitingUnarmed;
    }
    else {
        return WdfDevStatePwrPolSystemWakeDeviceToD0;
    }
}


WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStartedWakeCapable(
    __inout FxPkgPnp* This
    )
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolStartedWakeCapable);

    //
    // Enable the idle state machine.  This will release any threads who are
    // waiting for the device to return to D0.
    //
    This->m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.EnableTimer();

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolWakeCapableDeviceIdle(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    We are now idle. Tell the active/idle state machine to move us to an idle
    state.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    BOOLEAN canPowerDown;

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolWakeCapableDeviceIdle);

    canPowerDown = This->m_PowerPolicyMachine.m_Owner->
                            m_PoxInterface.DeclareComponentIdle();

    //
    // If we are using driver-managed idle timeout we can power down immediately
    // and so we jump to the next state (that initiates power down) immediately.
    // If we are using system-managed idle timeout, we wait in the current state
    // for device-power-not-required notification.
    //
    return (canPowerDown ?
                WdfDevStatePwrPolTimerExpiredDecideUsbSS :
                WdfDevStatePwrPolNull);
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredDecideUsbSS(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Decides how to handle the idle timer firing.  If the device is capable of
    USB selective suspend, it will move the machine into that path.  Otherwise,
    the machine will be moved into the normal arm for wake while in D0 path.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolTimerExpiredWakeCapableUsbSS
    or
    WdfDevStatePwrPolTimerExpiredWakeCapable

  --*/
{
    NTSTATUS notifyPowerDownStatus;

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolTimerExpiredDecideUsbSS);

    //
    // Notify the device power requirement state machine that we are about to
    // power down.
    //
    notifyPowerDownStatus = This->m_PowerPolicyMachine.m_Owner->
                              m_PoxInterface.NotifyDevicePowerDown();
    if (FALSE == NT_SUCCESS(notifyPowerDownStatus)) {
        //
        // We couldn't notify the device power requirement state machine that
        // we are about to power down, because the "device-power-required"
        // notification has already arrived. So we should not power down at this
        // time. Revert back to the started state.
        //
        return WdfDevStatePwrPolDeviceIdleReturnToActive;
    }

    if (This->m_PowerPolicyMachine.m_Owner->m_IdleSettings.UsbSSCapable) {
        return WdfDevStatePwrPolTimerExpiredWakeCapableUsbSS;
    }
    else {
        return WdfDevStatePwrPolTimerExpiredWakeCapablePowerDown;
    }
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredWakeCapablePowerDown(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device is enabled to be armed for wake from S0.  The device just idled
    out and is now about to transition into this state. The first step is to
    move into a partial Dx state (I/O stopped), send the wake request, arm the
    device, and then move into full Dx.  This function starts the transition
    by requesting a Dx irp.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/

{
    BOOLEAN poweredDown;

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolTimerExpiredWakeCapablePowerDown);

    poweredDown = This->PowerPolicyCanIdlePowerDown(
        This->m_PowerPolicyMachine.m_Owner->m_IdleSettings.DxState
        );

    if (poweredDown == FALSE) {
        //
        // Upon failure, revert back to the started state.
        //
        return WdfDevStatePwrPolTimerExpiredWakeCapableDxAllocFailed;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredWakeCapableSendWake(
    __inout FxPkgPnp* This
    )
{
    NTSTATUS status;

    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolTimerExpiredWakeCapableSendWake);

    //
    // We are in a wake-enabled path, keep wake interrupts connected.
    //
    This->m_WakeInterruptsKeepConnected = TRUE;

    status = This->PowerPolicySendWaitWakeRequest(PowerSystemWorking);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "Could not allocate wake request for wake from S0, revert arming,"
            " %!STATUS!", status);

        COVERAGE_TRAP();

        return WdfDevStatePwrPolTimerExpiredWakeCapableCleanup;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredWakeCapableUsbSS(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Sends the selective suspend ready irp down to the USB parent

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolTimerExpiredWakeCapableUsbSS);

    This->PowerPolicySubmitUsbIdleNotification();

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolWakeCapableUsbSSCompleted(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    We've sent the USB idle notification, but the bus driver completed it even
    before sending the USB idle callback.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolWakeCapableUsbSSCompleted);

    //
    // We notified the device power requirement state machine that we are about
    // to power down, but eventually we didn't power down. So notify the device
    // power requirement state machine that the device should be considered
    // powered on.
    //
    This->m_PowerPolicyMachine.m_Owner->
                m_PoxInterface.DeviceIsPoweredOn();

    This->m_PowerPolicyMachine.m_Owner->
            m_PoxInterface.RequestComponentActive();

    return WdfDevStatePwrPolStartedWakeCapable;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredWakeCapableWakeArrived(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device is has been armed for wake from S0.  It is in a partial Dx state
    (I/O stopped) and needs to transition to the total Dx state by sending
    a PowerCompleteDx event to the power state machine.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    NTSTATUS status;

    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolTimerExpiredWakeCapableWakeArrived);

    status = This->m_PowerPolicyMachine.m_Owner->m_DeviceArmWakeFromS0.Invoke(
        This->m_Device->GetHandle()
        );

    if (!NT_SUCCESS(status)) {
        return WdfDevStatePwrPolTimerExpiredWakeCapableCancelWake;
    }

    //
    // If the PDO is the Power Policy owner, then enable wake at bus, otherwise
    // the power state machine will enable wake at bus.
    //
    if (This->m_Device->IsPdo()) {
        status = This->PowerEnableWakeAtBusOverload();
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "WDFDEVICE %p Failed to Enable Wake at Bus, %!STATUS!",
                This->m_Device->GetHandle(), status);

            return WdfDevStatePwrPolTimerExpiredWakeCapableCancelWake;
        }
    }

    This->PowerProcessEvent(PowerCompleteDx);

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredWakeCapableCancelWake(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Device was attempting to enter Dx armed for wake from S0, but
    EvtDeviceArmForWakeFromS0 returned failure.  Cancel the wait wake irp and
    move into Dx.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolTimerExpiredWakeCapableCancelWake);

    if (This->PowerPolicyCancelWaitWake() == FALSE) {
        return WdfDevStatePwrPolTimerExpiredWakeCapableWakeCanceled;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredWakeCapableWakeCanceled(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device was put into a wake from S0 state and the arm failed.  The wait
    wake irp has been canceled, now complete the power down.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolTimerExpiredWakeCapableCleanup

  --*/
{
    UNREFERENCED_PARAMETER(This);

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolTimerExpiredWakeCapableWakeCanceled);

    return WdfDevStatePwrPolTimerExpiredWakeCapableCleanup;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredWakeCapableCleanup(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    An attempt to arm the device for wake from S0 has failed.  Decide if we need
    to complete the USB SS callback or not before completely powering down so
    that we can power back up again.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolTimerExpiredWakeCapableCleanup);

    if (This->m_PowerPolicyMachine.m_Owner->m_IdleSettings.UsbSSCapable) {
        COVERAGE_TRAP();
        This->m_PowerPolicyMachine.UsbSSCallbackProcessingComplete();
    }

    //
    // Cancel UsbSS if present
    //
    if (This->PowerPolicyCancelUsbSSIfCapable()) {
        //
        // wait for Usbss completion event to move us from this state.
        //
        return WdfDevStatePwrPolNull;
    }

    return WdfDevStatePwrPolTimerExpiredWakeCompletedPowerDown;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredWakeCapableDxAllocFailed(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device idled out and we attempted to allocate a Dx irp so that we could
    be armed for wake from S0.  The Dx irp allocation failed.  Complete the
    USB SS callback and move back into the working state.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolTimerExpiredWakeCapableUndoPowerDown

  --*/
{
    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolTimerExpiredWakeCapableDxAllocFailed);

    if (This->m_PowerPolicyMachine.m_Owner->m_IdleSettings.UsbSSCapable) {
        COVERAGE_TRAP();
        This->m_PowerPolicyMachine.UsbSSCallbackProcessingComplete();
    }

    //
    // cancel USB SS irp if capable
    //
    if (This->PowerPolicyCancelUsbSSIfCapable()) {
        //
        // wait for Usbss completion event to move us from this state.
        //
        return WdfDevStatePwrPolNull;
    }

    return WdfDevStatePwrPolTimerExpiredWakeCapableUndoPowerDown;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredWakeCapableUndoPowerDown(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device idled out, but a failure occurred when we attempted to power
    down. So we need to return to the active state now.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolDeviceIdleReturnToActive

  --*/
{
    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolTimerExpiredWakeCapableUndoPowerDown);

    //
    // We notified the device power requirement state machine that we are about
    // to power down, but eventually we didn't power down. So notify the device
    // power requirement state machine that the device should be considered
    // powered on.
    //
    This->m_PowerPolicyMachine.m_Owner->
                m_PoxInterface.DeviceIsPoweredOn();
    return WdfDevStatePwrPolDeviceIdleReturnToActive;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredWakeCompletedPowerDown(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Complete the Dx transition from Dx armed for S0 wake.  Upon going into Dx,
    we will move back into D0 since the wake completed already (or there was
    an error in arming).

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolTimerExpiredWakeCompletedPowerDown);

    This->PowerProcessEvent(PowerCompleteDx);

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredWakeCompletedPowerUp(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device was armed for wake from S0 and the wake completed immediately
    (or there was a problem).  We put the device into Dx and we are now trying
    to bring it back into D0.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    NTSTATUS status;
    BOOLEAN result;

    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolTimerExpiredWakeCompletedPowerUp);

    This->m_PowerPolicyMachine.m_Owner->
            m_PoxInterface.RequestComponentActive();

    //
    // Disable the timer so that when we move back into D0, the timer is not
    // automatically started.  Since the timer has already fired, we should never
    // get FALSE back (which indicates we should wait for the timer to fire).
    //
    result = This->m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.DisableTimer();
    ASSERT(result);
    UNREFERENCED_PARAMETER(result);

    status = This->PowerPolicySendDevicePowerRequest(PowerDeviceD0, Retry);

    if (!NT_SUCCESS(status)) {
        //
        // Couldn't allocate power irp, goto the failed state
        //
        COVERAGE_TRAP();
        return WdfDevStatePwrPolDeviceD0PowerRequestFailed;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredWakeCompletedHardwareStarted(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    We went idle and were in the process of powering down, but the wait-wake IRP
    completed even before we could arm the device for wake. We finished powering
    down the device and we're now powering it back up due to the wait-wake
    completion.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    ASSERT_PWR_POL_STATE(This,
        WdfDevStatePwrPolTimerExpiredWakeCompletedHardwareStarted);

    This->m_PowerPolicyMachine.m_Owner->
                m_PoxInterface.DeviceIsPoweredOn();

    return WdfDevStatePwrPolS0WakeCompletePowerUp;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolWaitingArmedUsbSS(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Complete the USB SS callback now tha the device is armed and in Dx

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolWaitingArmed

  --*/
{
    BOOLEAN result;

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolWaitingArmedUsbSS);

    if (This->m_PowerPolicyMachine.m_Owner->m_IdleSettings.UsbSSCapable) {
        This->m_PowerPolicyMachine.UsbSSCallbackProcessingComplete();
    }

    //
    // Disable the timer so that when we move back into D0, the timer is not
    // automatically started.  Since the timer has already fired, we should never
    // get FALSE back (which indicates we should wait for the timer to fire).
    //
    result = This->m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.DisableTimer();
    ASSERT(result);
    UNREFERENCED_PARAMETER(result);

    //
    // PwrPolIoPresent can be sent before PwrPolPowerTimeoutExpired in the idle
    // state machine if the idle s.m. attempts to cancel the timer after it has
    // started running.  That means the PwrPolIoPresent meant to wake up the
    // device and resume from idle is lost.  By first querying the idle s.m.
    // after moving into Dx we can recover from the lost event.
    //
    return WdfDevStatePwrPolWaitingArmedQueryIdle;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolWaitingArmedQueryIdle(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device was in the WaitingArmed state and received a PwrPolIoPresent
    event.  Before committing the device to move back to D0, check to see if
    the device has returned to an idle state.  This can easily happen if the
    driver causes a power reference in D0Exit accidentally and the power
    reference is removed before exitting the function.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/

{
    //
    // If QueryReturnToIdle returns TRUE, return to the waiting state
    //
    if (This->m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.QueryReturnToIdle()) {
        return WdfDevStatePwrPolWaitingArmed;
    }
    else {
        return WdfDevStatePwrPolWaitingArmedIoPresentCancelUsbSS;
    }
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolIoPresentArmed(
    __inout FxPkgPnp* This
    )
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolIoPresentArmed);

    if (This->PowerPolicyCancelWaitWake() == FALSE &&
        This->m_PowerPolicyMachine.m_Owner->m_WakeCompletionEventDropped) {
        COVERAGE_TRAP();
        return WdfDevStatePwrPolIoPresentArmedWakeCanceled;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolWaitingArmedWakeInterruptFired(
    __inout FxPkgPnp* This
    )
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolWaitingArmedWakeInterruptFired);

    if (This->PowerPolicyCancelWaitWake() == FALSE &&
        This->m_PowerPolicyMachine.m_Owner->m_WakeCompletionEventDropped) {
        return WdfDevStatePwrPolWaitingArmedWakeSucceededCancelUsbSS;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolIoPresentArmedWakeCanceled(
    __inout FxPkgPnp* This
    )
{
    NTSTATUS status;

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolIoPresentArmedWakeCanceled);

    This->m_PowerPolicyMachine.m_Owner->
            m_PoxInterface.RequestComponentActive();

    status = This->PowerPolicySendDevicePowerRequest(PowerDeviceD0, Retry);

    if (!NT_SUCCESS(status)) {
        COVERAGE_TRAP();
        return WdfDevStatePwrPolDeviceD0PowerRequestFailed;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolS0WakeDisarm(
    __inout FxPkgPnp* This
    )
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolS0WakeDisarm);

    This->m_PowerPolicyMachine.m_Owner->
                m_PoxInterface.DeviceIsPoweredOn();

    //
    // If the PDO is the Power Policy owner, then disable wake at bus, otherwise
    // the power state machine will disable wake at bus.
    //
    if (This->m_Device->IsPdo()) {
        This->PowerDisableWakeAtBusOverload();
    }

    This->m_PowerPolicyMachine.m_Owner->m_DeviceDisarmWakeFromS0.Invoke(
        This->m_Device->GetHandle()
        );

    return WdfDevStatePwrPolS0WakeCompletePowerUp;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolS0WakeCompletePowerUp(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    We are moving back into D0 from being armed for wake from S0.  The device
    is in partial D0 (hw started) already, so complete the transition to D0
    by posting the PowerCompleteD0 event to the power state machine.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolS0WakeCompletePowerUp);

    This->PowerProcessEvent(PowerCompleteD0);

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredWakeSucceeded(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The wake irp succeeded synchronously when we sent it down the stack.  Notify
    the driver and move immediately back into the working state.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolStartingDecideS0Wake

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolTimerExpiredWakeSucceeded);

    This->m_PowerPolicyMachine.m_Owner->m_DeviceWakeFromS0Triggered.Invoke(
        This->m_Device->GetHandle()
        );

    return WdfDevStatePwrPolTimerExpiredWakeCompletedDisarm;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredWakeCompletedDisarm(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device was armed for wake from Dx in S0.  The wake irp completed
    before the PwrPolPowerDown was processed.  Disarm the device, move to Dx,
    and then immediately back to D0.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolTimerExpiredWakeCapableCleanup

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolTimerExpiredWakeCompletedDisarm);

    //
    // If the PDO is the Power Policy owner, then disable wake at bus, otherwise
    // the power state machine will disable wake at bus.
    //
    if (This->m_Device->IsPdo()) {
        This->PowerDisableWakeAtBusOverload();
    }

    This->m_PowerPolicyMachine.m_Owner->m_DeviceDisarmWakeFromS0.Invoke(
        This->m_Device->GetHandle()
        );

    return WdfDevStatePwrPolTimerExpiredWakeCapableCleanup;

}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolWakeFailedUsbSS(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Wake failed before we got the Dx irp.  Complete the SS calback and then
    move to the WakeFailed state.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolIoPresentArmedWakeCanceled

  --*/
{
    BOOLEAN result;

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolWakeFailedUsbSS);

    if (This->m_PowerPolicyMachine.m_Owner->m_IdleSettings.UsbSSCapable) {
        This->m_PowerPolicyMachine.UsbSSCallbackProcessingComplete();
    }

    //
    // Disable the timer so that when we move back into D0, the timer is not
    // automatically started.  Since the timer has already fired, we should never
    // get FALSE back (which indicates we should wait for the timer to fire).
    //
    result = This->m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.DisableTimer();
    ASSERT(result);
    UNREFERENCED_PARAMETER(result);

    //
    // cancel USB SS irp if capable
    //
    if (This->PowerPolicyCancelUsbSSIfCapable()) {
        //
        // wait for Usbss completion event to move us from this state.
        //
        return WdfDevStatePwrPolNull;
    }

    return WdfDevStatePwrPolIoPresentArmedWakeCanceled;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredWakeCapablePowerDownFailedCancelWake(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    We sent the wake request, but the power state machine failed.  Cancel the
    wake request and then move to the failed state.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    ASSERT_PWR_POL_STATE(
        This,
        WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownFailedCancelWake);

    if (This->PowerPolicyCancelWaitWake() == FALSE) {
        return WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownFailedWakeCanceled;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredWakeCapablePowerDownFailedWakeCanceled(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The wake request has been canceled. Move to the state where we will decide
    if we need to complete the USB SS callback.

    There is no need to disarm for wake from S0 because the failure here is for
    the device to power down.  We must assume the device is now in Dx and we
    cannot touch hw in this state.

Arguments:
    This - instance of the state machine

Return Value:
    new state, WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownFailedDecideUsbSS

  --*/
{
    UNREFERENCED_PARAMETER(This);

    ASSERT_PWR_POL_STATE(
        This,
        WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownFailedWakeCanceled);

    return WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownFailedUsbSS;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredWakeCapablePowerDownFailedUsbSS(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Complete the USB SS callback if enabled and the move to the failed state

Arguments:
    This - instance of the state machine

Return Value:
    new state, WdfDevStatePwrPolDevicePowerRequestFailed

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownFailedUsbSS);

    if (This->m_PowerPolicyMachine.m_Owner->m_IdleSettings.UsbSSCapable) {
        This->m_PowerPolicyMachine.UsbSSCallbackProcessingComplete();
    }

    if (This->PowerPolicyCancelUsbSSIfCapable()) {
        //
        // wait for Usbss completion event to move us from this state.
        //
        return WdfDevStatePwrPolNull;
    }

    return WdfDevStatePwrPolDevicePowerRequestFailed;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolCancelingWakeForSystemSleep(
    __inout FxPkgPnp* This
    )
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolCancelingWakeForSystemSleep);

    if (This->PowerPolicyCancelWaitWake() == FALSE &&
        This->m_PowerPolicyMachine.m_Owner->m_WakeCompletionEventDropped) {
        return WdfDevStatePwrPolCancelingWakeForSystemSleepWakeCanceled;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolCancelingWakeForSystemSleepWakeCanceled(
    __inout FxPkgPnp* This
    )
{
    NTSTATUS status;

    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolCancelingWakeForSystemSleepWakeCanceled);

    status = This->PowerPolicySendDevicePowerRequest(PowerDeviceD0, NoRetry);

    if (!NT_SUCCESS(status)) {
        COVERAGE_TRAP();
        return WdfDevStatePwrPolPowerUpForSystemSleepNotSeen;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolDisarmingWakeForSystemSleepCompletePowerUp(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device was armed for wake from S0 and in Dx.  The machine is now moving
    into Sx.  The device is currently in partial D0 (HW started).  Disarm wake
    from S0 and then move the device fully into D0 (I/0 started).

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolDisarmingWakeForSystemSleepCompletePowerUp);

    //
    // If the PDO is the Power Policy owner, then disable wake at bus, otherwise
    // the power state machine will disable wake at bus.
    //
    if (This->m_Device->IsPdo()) {
        This->PowerDisableWakeAtBusOverload();
    }

    This->m_PowerPolicyMachine.m_Owner->m_DeviceDisarmWakeFromS0.Invoke(
        This->m_Device->GetHandle()
        );

    This->PowerProcessEvent(PowerCompleteD0);

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolPowerUpForSystemSleepFailed(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The system is moving into an Sx state and the device was in a Dx armed for
    wake from S0 state.  Power up has failed (could not allocate the request,
    actual power up path failed, etc).  Complete the Sx irp and move into a
    state where we can be removed when we return to S0.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolDevicePowerRequestFailed

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolPowerUpForSystemSleepFailed);

    This->PowerPolicyCompleteSystemPowerIrp();

    return WdfDevStatePwrPolDevicePowerRequestFailed;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolWokeFromS0UsbSS(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device successfully woke up before we got to the WaitingArmed state.
    Complete the USB SS callback while we are in Dx.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolWokeFromS0

  --*/
{
    BOOLEAN result;

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolWokeFromS0UsbSS);

    if (This->m_PowerPolicyMachine.m_Owner->m_IdleSettings.UsbSSCapable) {
        This->m_PowerPolicyMachine.UsbSSCallbackProcessingComplete();
    }

    //
    // Disable the timer so that when we move back into D0, the timer is not
    // automatically started.  Since the timer has already fired, we should never
    // get FALSE back (which indicates we should wait for the timer to fire).
    //
    result = This->m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.DisableTimer();
    ASSERT(result);
    UNREFERENCED_PARAMETER(result);

    //
    // Cancel USBSS irp if capable
    //
    if (This->PowerPolicyCancelUsbSSIfCapable()) {
        //
        // Usbss completion event will move us from this state.
        //
        return WdfDevStatePwrPolNull;
    }

    return WdfDevStatePwrPolWokeFromS0;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolWokeFromS0(
    __inout FxPkgPnp* This
    )
{
    NTSTATUS status;

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolWokeFromS0);

    This->m_PowerPolicyMachine.m_Owner->
            m_PoxInterface.RequestComponentActive();

    status = This->PowerPolicySendDevicePowerRequest(PowerDeviceD0, Retry);
    if (!NT_SUCCESS(status)) {
        COVERAGE_TRAP();
        return WdfDevStatePwrPolDeviceD0PowerRequestFailed;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolWokeFromS0NotifyDriver(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device was armed for wake from S0 and it successfully woke up and triggered
    wake.  Notify the driver of this on the way back up to D0.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolS0WakeDisarm

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolWokeFromS0NotifyDriver);

    This->m_PowerPolicyMachine.m_Owner->m_DeviceWakeFromS0Triggered.Invoke(
        This->m_Device->GetHandle()
        );

    return WdfDevStatePwrPolS0WakeDisarm;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStoppingResetDevice(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device was in a Dx state, unarmed for wake.  The device is not being
    removed, so we must disable the idle timer and power up the device so that
    we can implicitly power it down.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    NTSTATUS status;
    BOOLEAN result;

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolStoppingResetDevice);

    This->m_PowerPolicyMachine.m_Owner->
            m_PoxInterface.RequestComponentActive();

    result = This->m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.DisableTimer();
    ASSERT(result);
    UNREFERENCED_PARAMETER(result);

    status = This->PowerPolicySendDevicePowerRequest(PowerDeviceD0, Retry);

    if (!NT_SUCCESS(status)) {
        COVERAGE_TRAP();
        return WdfDevStatePwrPolDeviceD0PowerRequestFailed;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStoppingResetDeviceCompletePowerUp(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
   The device was idled out and in Dx (not armed for wake from S0).  The power
   policy state machine is being stopped and the device has been brought into
   partial D0 (HW started).  Complete the D0 transition by sending PowerCompleteD0
   to the power state machine.

Arguments:
    This - Instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolStoppingResetDeviceCompletePowerUp);

    This->m_PowerPolicyMachine.m_Owner->
                m_PoxInterface.DeviceIsPoweredOn();

    This->PowerProcessEvent(PowerCompleteD0);

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStoppingResetDeviceFailed(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The attempt to power on a device in the idled out state so that we can stop
    the power policy state machine failed.  Record the error and continue down
    the stop path.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolStopping

  --*/

{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolStoppingResetDeviceFailed);

    This->m_PowerPolicyMachine.m_Owner->m_PowerFailed = TRUE;

    //
    // Notify the device power requirement state machine that the device should
    // be considered powered on, although it is not. Doing this will ensure that
    // component activation before device removal will succeed.
    //
    This->m_PowerPolicyMachine.m_Owner->
                m_PoxInterface.DeviceIsPoweredOn();

    return WdfDevStatePwrPolStopping;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStoppingD0(
    __inout FxPkgPnp* This
    )
{
    NTSTATUS status;

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolStoppingD0);

    This->m_PowerPolicyMachine.m_Owner->
            m_PoxInterface.RequestComponentActive();

    This->m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.DisableTimer();

    status = This->PowerPolicySendDevicePowerRequest(PowerDeviceD0, Retry);
    if (!NT_SUCCESS(status)) {
        COVERAGE_TRAP();
        return WdfDevStatePwrPolDeviceD0PowerRequestFailed;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStoppingD0Failed(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    While attempting to stop the device and bring it into D0, power up failed.
    Record the error and continue.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolStoppingDisarmWakeCancelWake

  --*/

{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolStoppingD0Failed);

    This->m_PowerPolicyMachine.m_Owner->m_PowerFailed = TRUE;

    //
    // Notify the device power requirement state machine that the device should
    // be considered powered on, although it is not. Doing this will ensure that
    // component activation before device removal will succeed.
    //
    This->m_PowerPolicyMachine.m_Owner->
                m_PoxInterface.DeviceIsPoweredOn();

    return WdfDevStatePwrPolStoppingDisarmWakeCancelWake;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStoppingDisarmWake(
    __inout FxPkgPnp* This
    )
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolStoppingDisarmWake);

    This->m_PowerPolicyMachine.m_Owner->
                m_PoxInterface.DeviceIsPoweredOn();

    //
    // If the PDO is the Power Policy owner, then disable wake at bus, otherwise
    // the power state machine will disable wake at bus.
    //
    if (This->m_Device->IsPdo()) {
        This->PowerDisableWakeAtBusOverload();
    }

    This->m_PowerPolicyMachine.m_Owner->m_DeviceDisarmWakeFromS0.Invoke(
        This->m_Device->GetHandle()
        );

    This->PowerProcessEvent(PowerCompleteD0);

    return WdfDevStatePwrPolNull;

}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStoppingDisarmWakeCancelWake(
    __inout FxPkgPnp* This
    )
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolStoppingDisarmWakeCancelWake);

    if (This->PowerPolicyCancelWaitWake() == FALSE &&
        This->m_PowerPolicyMachine.m_Owner->m_WakeCompletionEventDropped) {
        return WdfDevStatePwrPolStoppingDisarmWakeWakeCanceled;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStoppingDisarmWakeWakeCanceled(
    __inout FxPkgPnp* This
    )
{
    UNREFERENCED_PARAMETER(This);

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolStoppingDisarmWakeWakeCanceled);

    return WdfDevStatePwrPolStopping;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStopping(
    __inout FxPkgPnp* This
    )
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolStopping);

    //
    // Prevent any children from powering up.  Technically this is not necessary
    // because if we are getting to this point, all of our children have already
    // been implicitly stopped as well, but this keeps the child power up state
    // consistent with the power policy state.
    //
    This->PowerPolicyBlockChildrenPowerUp();

    //
    // This power change event does not need to be synchronous
    //
    This->PowerProcessEvent(PowerImplicitD3);

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStoppingFailed(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The attempt to move the power state machine into a Dx state failed.  Report
    the failure to pnp and move to the stopped state.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolStopped

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolStoppingFailed);

    This->m_PowerPolicyMachine.m_Owner->m_PowerFailed = TRUE;

    return WdfDevStatePwrPolStoppingSendStatus;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStoppingSendStatus(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The power policy machine is about to finish stopping.  The final act of
    stopping is notify the Pnp state machine of the final outcome.  Do so, reset
    state, and move to the stopped state.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolStopped

  --*/
{
    KIRQL irql;

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolStoppingSendStatus);

    This->m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.Stop();

    //
    // We raise IRQL to dispatch level so that pnp is forced onto its own thread
    // to process the PwrPolStopped event.  If pnp is on the power thread when
    // it processes the event and it tries to delete the dedicated thread, it
    // will deadlock waiting for the thread its on to exit.
    //
    Mx::MxRaiseIrql(DISPATCH_LEVEL, &irql);
    This->PnpProcessEvent(
        This->m_PowerPolicyMachine.m_Owner->m_PowerFailed
                          ? PnpEventPwrPolStopFailed
                          : PnpEventPwrPolStopped
        );
    Mx::MxLowerIrql(irql);

    //
    // Reset back to a non failed state
    //
    This->m_PowerPolicyMachine.m_Owner->m_PowerFailed = FALSE;

    return WdfDevStatePwrPolStopped;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStoppedRemoving(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device is being removed. Tell the active/idle state machine to
    unregister with the power framework.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolStoppedRemoving);

    This->m_PowerPolicyMachine.m_Owner->
            m_PoxInterface.UninitializeComponents();

    return WdfDevStatePwrPolRemoved;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolRemoved(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The active/idle state machine has unregistered with the power framework.
    Tell the PNP state machine that device removal can proceed.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    KIRQL irql;

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolRemoved);

    //
    // We raise IRQL to dispatch level so that pnp is forced onto its own thread
    // to process the PwrPolRemoved event.  If pnp is on the power thread when
    // it processes the event and it tries to delete the dedicated thread, it
    // will deadlock waiting for the thread it is on to exit.
    //
    Mx::MxRaiseIrql(DISPATCH_LEVEL, &irql);
    This->PnpProcessEvent(PnpEventPwrPolRemoved);
    Mx::MxLowerIrql(irql);

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolRestarting(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device is restarting after being stopped

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolRestarting);

    This->m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.Start();

    This->PowerProcessEvent(PowerImplicitD0);

    //
    // Wait for the successful power up before starting any idle timers.  If
    // the power up fails, we will not send a power policy stop from the pnp
    // engine.
    //
    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolRestartingFailed(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Power up failed when restarting the device

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolStopped

  --*/
{
    KIRQL irql;

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolRestartingFailed);

    This->m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.Stop();

    //
    // We raise IRQL to dispatch level so that pnp is forced onto its own thread
    // to process the PwrPolStartFailed event.  If pnp is on the power thread
    // when it processes the event and it tries to delete the dedicated thread,
    // it will deadlock waiting for the thread it is on to exit.
    //
    Mx::MxRaiseIrql(DISPATCH_LEVEL, &irql);
    This->PnpProcessEvent(PnpEventPwrPolStartFailed);
    Mx::MxLowerIrql(irql);

    return WdfDevStatePwrPolStopped;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStoppingCancelTimer(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Moving to the stopping state.  Need to cancel the idle timeout timer.

Arguments:
    This - instance of the state machine

Return Value:
    new power policy state

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolStoppingCancelTimer);

    if (This->m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.DisableTimer()) {
        return WdfDevStatePwrPolStopping;
    }
    else {
        //
        // Timer was not canceled, move to the state where we wait for the
        // timeout event to be posted.
        //
        return WdfDevStatePwrPolStoppingWaitForIdleTimeout;
    }
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStoppingCancelUsbSS(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Cancel the USB SS request because we are being stopped or surprise removed

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolStoppingWaitForUsbSSCompletion

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolStoppingCancelUsbSS);

    //
    // We notified the device power requirement state machine that we are about
    // to power down, but eventually we didn't power down. So notify the device
    // power requirement state machine that the device should be considered
    // powered on.
    //
    This->m_PowerPolicyMachine.m_Owner->
                m_PoxInterface.DeviceIsPoweredOn();

    This->m_PowerPolicyMachine.m_Owner->
            m_PoxInterface.RequestComponentActive();

    if (This->PowerPolicyCancelUsbSSIfCapable() == FALSE) {
        //
        // UsbSS already canceled/completed
        //
        return WdfDevStatePwrPolStoppingCancelTimer;
    }

    return WdfDevStatePwrPolStoppingWaitForUsbSSCompletion;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStoppingCancelWake(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Cancels the pended wait wake in the case where we are surprise removed and
    in a Dx state and armed for wake.

Arguments:
    This - instance of the state machine

Return Value:
    If there was no request to cancel, WdfDevStatePwrPolStopping. If there was,
    wait for the wait completion event to be posted before transitioning.

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolStoppingCancelWake);

    if (This->PowerPolicyCancelWaitWake() == FALSE &&
        This->m_PowerPolicyMachine.m_Owner->m_WakeCompletionEventDropped) {
        COVERAGE_TRAP();
        return WdfDevStatePwrPolStopping;
    }
    else {
        return WdfDevStatePwrPolNull;
    }
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolCancelUsbSS(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The S0 idle policy has changed.  Cancel the idle notification so we can move
    to another S0 started state

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolCancelUsbSS);

    //
    // We notified the device power requirement state machine that we are about
    // to power down, but eventually we didn't power down. So notify the device
    // power requirement state machine that the device should be considered
    // powered on.
    //
    This->m_PowerPolicyMachine.m_Owner->
                m_PoxInterface.DeviceIsPoweredOn();

    This->m_PowerPolicyMachine.m_Owner->
            m_PoxInterface.RequestComponentActive();

    if (This->PowerPolicyCancelUsbSSIfCapable() == FALSE) {
        //
        // UsbSS has already been canceled/completed
        //
        return WdfDevStatePwrPolStartedCancelTimer;
    }

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStarted(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device is now in the started state (w/regard to power policy).

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolStarted);

    //
    // We signal we are in D0 even though there is idling out in this state
    // because we could have just come from a state which *was* idled out.
    // For instance
    // 1) We are in WaitingArmed and an io present event moved us out of this state
    // 2) Immediately afterward, the s0 idle policy was changed
    // 3) When we enter StartingDecideS0Wake, we will move into this state without
    //    first entering StartedWakeCapable.
    // 4) The source of the io present event, if waiting synchronously needs to
    //    be unblocked.
    //
    This->m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.DisableTimer();

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStartedCancelTimer(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Device is started and we need to cancel the idle timer

Arguments:
    This - instance of the state machine

Return Value:
    new power policy state

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolStartedCancelTimer);

    if (This->m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.DisableTimer()) {
        return WdfDevStatePwrPolStartingDecideS0Wake;
    }
    else {
        COVERAGE_TRAP();
        return WdfDevStatePwrPolStartedWaitForIdleTimeout;
    }
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStartedWakeCapableCancelTimerForSleep(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Device is wake capable and enabled for idling out.  Try to disable the
    power idle state machine.

Arguments:
    This - instance of the state machine

Return Value:
    new power policy state

  --*/
{
    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolStartedWakeCapableCancelTimerForSleep);

    if (This->m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.DisableTimer()) {
        return WdfDevStatePwrPolSleeping;
    }
    else {
        COVERAGE_TRAP();
        return WdfDevStatePwrPolStartedWakeCapableWaitForIdleTimeout;
    }
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStartedWakeCapableSleepingUsbSS(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The machine is going to sleep after we have submitted the USB SS request
    but before USB has called us back on the go to idle callback.  Cancel the
    request and wait for its completion

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPol

  --*/
{
    BOOLEAN result;

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolStartedWakeCapableSleepingUsbSS);

    result = This->m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.DisableTimer();

    //
    // Since the power timeout expired, the disable should not return FALSE
    //
    ASSERT(result);
    UNREFERENCED_PARAMETER(result);

    //
    // Cancel USBSS irp
    //
    This->PowerPolicyCancelUsbSS();

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStartedIdleCapableCancelTimerForSleep(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Device is idle capable and enabled for idling out.  Try to disable the
    power idle state machine.

Arguments:
    This - instance of the state machine

Return Value:
    new power policy state

  --*/
{
    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolStartedIdleCapableCancelTimerForSleep);

    if (This->m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.DisableTimer()) {
        return WdfDevStatePwrPolSleeping;
    }
    else {
        COVERAGE_TRAP();
        return WdfDevStatePwrPolStartedIdleCapableWaitForIdleTimeout;
    }
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolDeviceD0PowerRequestFailed(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    A D0 request failed, notify pnp.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolDevicePowerRequestFailed

  --*/
{
    COVERAGE_TRAP();

    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolDeviceD0PowerRequestFailed);

    This->SetInternalFailure();

    if (FALSE == This->m_ReleaseHardwareAfterDescendantsOnFailure) {
        This->PnpProcessEvent(PnpEventPowerUpFailed);
    }

    return WdfDevStatePwrPolDevicePowerRequestFailed;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolDevicePowerRequestFailed(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Either we could not allocate a device power irp or the power state machine
    reported failure.  Mark the failure and then wait for pnp to send a stop
    event.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolDevicePowerRequestFailed);

    //
    // In the failure path we still need to notify the children that they can
    // power up so that they will unblock.
    //
    This->PowerPolicyChildrenCanPowerUp();

    This->m_PowerPolicyMachine.m_Owner->m_PowerFailed = TRUE;

    //
    // Notify the device power requirement state machine that the device should
    // be considered powered on, although it may not be in reality. Doing this
    // will ensure that component activation before device removal will succeed.
    //
    This->m_PowerPolicyMachine.m_Owner->
                m_PoxInterface.DeviceIsPoweredOn();

    return WdfDevStatePwrPolNull;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSleepingPowerDownNotProcessed(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    We requested a Dx irp but a driver layered above our device failed the
    request without sending it down the stack.  Queue a Tear down the stack and
    then move into a state where we will complete the Sx irp.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolSystemSleepPowerRequestFailed

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolSleepingPowerDownNotProcessed);

    This->SetInternalFailure();

    return WdfDevStatePwrPolSystemSleepPowerRequestFailed;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredWakeCapablePowerDownNotProcessed(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    We requested a Dx irp after idling out, but a driver layered above our device
    failed the request without sending it down the stack.  Queue a tear down the
    stack and then move into a state where we can handle tear down.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolTimerExpiredWakeCapableDxAllocFailed

  --*/
{
    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownNotProcessed);

    This->SetInternalFailure();

    return WdfDevStatePwrPolTimerExpiredWakeCapableDxAllocFailed;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredNoWakePowerDownNotProcessed(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    We requested a Dx irp after idling out, but a driver layered above our device
    failed the request without sending it down the stack.  Queue a tear down the
    stack and then move into a state where we can handle tear down.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolStartedIdleCapable

  --*/
{
    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolTimerExpiredNoWakePowerDownNotProcessed);

    This->SetInternalFailure();

    return WdfDevStatePwrPolTimerExpiredNoWakeUndoPowerDown;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredNoWakeUndoPowerDown(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device was idle and attempting to go to Dx (without wake-from-S0), but
    a power down failure occurred.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolTimerExpiredNoWakeUndoPowerDown);

    //
    // We notified the device power requirement state machine that we are about
    // to power down, but eventually we didn't power down. So notify the device
    // power requirement state machine that the device should be considered
    // powered on.
    //
    This->m_PowerPolicyMachine.m_Owner->
                m_PoxInterface.DeviceIsPoweredOn();

    return WdfDevStatePwrPolTimerExpiredNoWakeReturnToActive;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredNoWakeReturnToActive(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device was idle and attempting to go to Dx (without wake-from-S0), but
    eventually it did not go to Dx. This might have been because a power down
    failure occurred or because we received the device power required
    notification from the power framework before we could go to Dx.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolNull

  --*/
{
    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolTimerExpiredNoWakeReturnToActive);

    This->m_PowerPolicyMachine.m_Owner->
            m_PoxInterface.RequestComponentActive();

    return WdfDevStatePwrPolStartedIdleCapable;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredNoWakePoweredDownDisableIdleTimer(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device has fully powered down.  Disable the idle timer so that when we
    we power up the idle timer will not start running immediately.  Rather,
    we should power up and only start the idle timer after being explicitly
    enabled again.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolWaitingUnarmedQueryIdle

  --*/
{
    BOOLEAN result;

    ASSERT_PWR_POL_STATE(
        This, WdfDevStatePwrPolTimerExpiredNoWakePoweredDownDisableIdleTimer);

    //
    // Disable the timer so that when we move back into D0, the timer is not
    // automatically started.  Since the timer has already fired, we should never
    // get FALSE back (which indicates we should wait for the timer to fire).
    //
    result = This->m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.DisableTimer();
    ASSERT(result);
    UNREFERENCED_PARAMETER(result);

    //
    // Check to see if we should immediately power up
    //
    return WdfDevStatePwrPolWaitingUnarmedQueryIdle;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolPowerUpForSystemSleepNotSeen(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    PowerDeviceD0 request was not forwarded to this driver by upper driver even
    though this driver requested it. Power completion routine detects this
    condition, and causes transition to this state. Ensure that any pending
    S-IRP is completed.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolDeviceD0PowerRequestFailed

  --*/
{
    if (This->m_PendingSystemPowerIrp != NULL) {
        This->PowerPolicyCompleteSystemPowerIrp();
    }

    return WdfDevStatePwrPolDeviceD0PowerRequestFailed;
}

__drv_sameIRQL
VOID
FxPkgPnp::_PowerPolDeviceWaitWakeComplete(
    __in MdDeviceObject DeviceObject,
    __in UCHAR MinorFunction,
    __in POWER_STATE PowerState,
    __in_opt PVOID Context,
    __in PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:
    Completion routine for a requested wait wake irp.  Called once the wait wake
    irp has traveled through the entire stack or when some driver in the stack
    completes the wait wake irp.  We feed the result of the wait wake operation
    back into the power policy state machine through an event.

Arguments:
    Context - instance of the state machine
    IoStatus - pointer to the IO_STATUS_BLOCK structure for the completed IRP
    All others ignored

Return Value:
    None

  --*/
{
    FxPkgPnp* pThis;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(MinorFunction);
    UNREFERENCED_PARAMETER(PowerState);

    pThis = (FxPkgPnp*) Context;
    pThis->m_PowerPolicyMachine.m_Owner->m_WaitWakeStatus = IoStatus->Status;

    if (NT_SUCCESS(IoStatus->Status)) {
        pThis->PowerPolicyProcessEvent(PwrPolWakeSuccess);
    }
    else {
        pThis->PowerPolicyProcessEvent(PwrPolWakeFailed);
    }
}

__drv_sameIRQL
VOID
FxPkgPnp::_PowerPolDevicePowerDownComplete(
    __in MdDeviceObject DeviceObject,
    __in UCHAR MinorFunction,
    __in POWER_STATE PowerState,
    __in_opt PVOID Context,
    __in PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:
    Completion routine for a requested power irp.  Called once the power irp
    has traveled through the entire stack.  We feed the result of the power
    operation back into the power policy state machine through an event.

Arguments:
    Context - instance of the state machine
    All others ignored

Return Value:
    None

  --*/

{
    FxPkgPnp* pThis;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(MinorFunction);
    UNREFERENCED_PARAMETER(PowerState);
    UNREFERENCED_PARAMETER(IoStatus);

    pThis = (FxPkgPnp*) Context;

    //
    // Note that we are ignoring IoStatus.Status intentionally.  If we failed
    // power down in this device, we have tracked that state via
    // m_PowerDownFailure.  If some other device failed power down, we are still
    // in the state where we succeeded power down and we don't want to alter
    // that yet.
    //
    // Note that the state machine also handles an upper filter completing the
    // Dx irp before our device gets to process it (which would be a bug in the
    // filter driver) by handling the PwrPolPowerDown from the appropriate
    // state, as an example...
    //
    // a successful power down state transition looks like this
    // 1)  power policy state where we request a power irp
    // 2)  power sends a partial power down message (PwrPolPowerDownIoStopped)
    //      to power policy, moves to the partial power down state
    // 3)  power policy tells power to complete the power down
    // 4)  power completes the irp and we send the power down complete message
    //      (PwrPolPowerDown) moving from the partial power down state to the
    //      final power down state
    //
    // In the case where the top filter driver completes the PIRP without our
    // driver seeing it, we will be in the state where we requested a power irp
    // and process a power down complete event instead of the partial power down
    // event and in this transition, we detect the error.
    //
    if (pThis->m_PowerMachine.m_PowerDownFailure) {
        //
        // Clear the faliure condition if we ever attempt to power off this
        // device again (highly possible if it is a PDO).
        //
        pThis->m_PowerMachine.m_PowerDownFailure = FALSE;

        //
        // Power down failed, send ourself an even indicating that.
        //
        pThis->PowerPolicyProcessEvent(PwrPolPowerDownFailed);

        //
        // Inform pnp last so that all the other state machines are in the failed
        // state by the time we transition the pnp state of the device.
        //
        if (FALSE == pThis->m_ReleaseHardwareAfterDescendantsOnFailure) {
            pThis->PnpProcessEvent(PnpEventPowerDownFailed);
        }
    }
    else {
        //
        // Power down succeeded, send ourself an even indicating that.
        //
        pThis->PowerPolicyProcessEvent(PwrPolPowerDown);
    }
}

__drv_sameIRQL
VOID
FxPkgPnp::_PowerPolDevicePowerUpComplete(
    __in MdDeviceObject DeviceObject,
    __in UCHAR MinorFunction,
    __in POWER_STATE PowerState,
    __in_opt PVOID Context,
    __in PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:
    Completion routine for a requested power irp.  Called once the power irp
    has traveled through the entire stack.  We feed the result of the power
    operation back into the power policy state machine through an event.

Arguments:
    Context - instance of the state machine
    All others ignored

Return Value:
    None

  --*/

{
    FxPkgPnp* pThis;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(MinorFunction);
    UNREFERENCED_PARAMETER(PowerState);
    UNREFERENCED_PARAMETER(IoStatus);

    pThis = (FxPkgPnp*) Context;

    //
    // The state machine handles an upper filter completing the
    // D0 irp before our device gets to process it (which would be a bug in the
    // filter driver).
    //
    if (pThis->m_PowerPolicyMachine.m_Owner->m_RequestedPowerUpIrp) {
        //
        // We requested a Power Irp but that never arrived in dispatch routine.
        // We know this because m_RequestedPowerUpIrp is still TRUE at the end of
        // the power irp completion (it is set to false when it arrives in
        // dispatch routine).
        //
        DoTraceLevelMessage(
            pThis->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "PowerDeviceD0 requested by WDFDEVICE 0x%p !devobj 0x%p, "
            "is being completed by upper driver without sending it to "
            "driver that requested it",
            pThis->m_Device->GetHandle(),
            pThis->m_Device->GetDeviceObject());

        //
        // Power-up request not seen, send ourself an event indicating that.
        //
        pThis->PowerPolicyProcessEvent(PwrPolPowerUpNotSeen);
    }
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::PowerPolicySendDevicePowerRequest(
    __in DEVICE_POWER_STATE DeviceState,
    __in SendDeviceRequestAction Action
    )
/*++

Routine Description:
    Attempts to send a D irp to the stack.  The caller can specify if the
    request allocation is retried in case of failure.

Design Notes:
    The timeout and number of retries are somewhat magical numbers.  They were
    picked so that the total amount of time spent attempting to retry would be
    under a minute.  Any memory pressure the machine would be under after the
    first failure should be over by the end of the minute; if not, there are
    bigger problems.

    If you look at each of the callers who specify Retry for the Action, nearly
    each one transitions to the WdfDevStatePwrPolDevicePowerRequestFailed state
    if the request cannot be allocated.  This transition could be placed in this
    function, but it is not for 2 reasons:

    1)  Keep this function simple

    2)  It makes the flow of the state transition function easier to understand;
        all transitions out of the current state happen within top level
        transition function

Arguments:
    DeviceState - The new D state being request

    Action - Whether to retry upon failure to allocate the request

Return Value:
    NT_SUCCESS if the request was allocated, !NT_SUCCESS otherwise

  --*/
{
    MdRequestPowerComplete pCompletionRoutine;
    LARGE_INTEGER interval;
    NTSTATUS status;
    POWER_STATE state;
    ULONG i;

    status = STATUS_UNSUCCESSFUL;
    interval.QuadPart = WDF_REL_TIMEOUT_IN_MS(500);
    state.DeviceState = DeviceState;

    if (DeviceState == PowerDeviceD0) {
        //
        // We are powering up, we do not synchronize the completion of the D0 irp
        // with a potential S0 irp. However we need to ensure that if an upper filter
        // driver fails the power irp, we handle it gracefully rather than keep waiting
        // for the power irp to arrive.
        //
        pCompletionRoutine = _PowerPolDevicePowerUpComplete;
    }
    else {
        //
        // We are powering down, we synchronize the completion of the Dx irp
        // with a potential Sx irp.  If there is no pending Sx irp, the state
        // machine takes care of it.
        //
        pCompletionRoutine = _PowerPolDevicePowerDownComplete;
    }

    //
    // We track when we request power irps to catch someone other then ourselves
    // sending power irps to our own stack.
    //
    if (DeviceState == PowerDeviceD0) {
        m_PowerPolicyMachine.m_Owner->m_RequestedPowerUpIrp = TRUE;
    }
    else {
        m_PowerPolicyMachine.m_Owner->m_RequestedPowerDownIrp = TRUE;
    }

    for (i = 0; i < 100; i++) {
        status = FxIrp::RequestPowerIrp(m_Device->GetDeviceObject(),
                                   IRP_MN_SET_POWER,
                                   state,
                                   pCompletionRoutine,
                                   this);

        //
        // If we are not retrying, we always break out
        //
        if (NT_SUCCESS(status) || Action == NoRetry) {
            break;
        }

        Mx::MxDelayExecutionThread(KernelMode, FALSE, &interval);
    }

    if (!NT_SUCCESS(status)) {
        //
        // We are no longer requesting a power irp
        //
        if (DeviceState == PowerDeviceD0) {
            m_PowerPolicyMachine.m_Owner->m_RequestedPowerUpIrp = FALSE;
        }
        else {
            m_PowerPolicyMachine.m_Owner->m_RequestedPowerDownIrp = FALSE;
        }

        if (Action == Retry) {
            COVERAGE_TRAP();

            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "Could not request D%d irp for device %p (WDFDEVICE %p), "
                "%!STATUS!", DeviceState-1,
                m_Device->GetDeviceObject(),
                m_Device->GetHandle(), status);
        }
    }

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Requesting D%d irp, %!STATUS!",
                        DeviceState-1, status);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::PowerPolicySendWaitWakeRequest(
    __in SYSTEM_POWER_STATE SystemState
    )
{
    NTSTATUS status;
    POWER_STATE state;

    state.SystemState = SystemState;

    //
    // We track when we request power irps to catch someone other then ourselves
    // sending power irps to our own stack.
    //
    m_PowerPolicyMachine.m_Owner->m_RequestedWaitWakeIrp = TRUE;

    //
    // Since we are sending a fresh wake, clear any state that was meant
    // for the last wake IRP
    //
    m_SystemWokenByWakeInterrupt = FALSE;

    //
    // we are requesting new ww irp so re-initialize dropped event tracker.
    //
    m_PowerPolicyMachine.m_Owner->m_WakeCompletionEventDropped = FALSE;

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Requesting wait wake irp for S%d", SystemState-1);

    status = FxIrp::RequestPowerIrp(m_Device->GetDeviceObject(),
                               IRP_MN_WAIT_WAKE,
                               state,
                               _PowerPolDeviceWaitWakeComplete,
                               this);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                            "Requesting wait wake irp for S%d failed, %!STATUS!",
                            SystemState-1, status);

        //
        // We are no longer requesting a power irp
        //
        m_PowerPolicyMachine.m_Owner->m_RequestedWaitWakeIrp = FALSE;
    }

    return status;
}

VOID
FxPkgPnp::PowerPolicyCompleteSystemPowerIrp(
    VOID
    )
{
    FxIrp irp(m_PendingSystemPowerIrp);
    NTSTATUS status;

    ASSERT(m_PendingSystemPowerIrp != NULL);

    status = STATUS_SUCCESS;

    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
        "Completing system power irp %p (S%d), %!STATUS!",
        m_PendingSystemPowerIrp,
        irp.GetParameterPowerStateSystemState()-1,
        status);

    m_PendingSystemPowerIrp = NULL;

    CompletePowerRequest(&irp, STATUS_SUCCESS);
}

BOOLEAN
FxPkgPnp::PowerPolicyCancelWaitWake(
    VOID
    )
/*++

Routine Description:
    Completes or cancels a pending wait wake irp depending on if the irp is
    present and if the device is the owner of the wait wake irp.

Arguments:

Return Value:


  --*/
{
    MdIrp wwIrp;
    BOOLEAN cancelled, result;

    if (m_SharedPower.m_WaitWakeOwner) {
        //
        // This will complete the irp and then post the appropriate events to
        // both the power and power policy state machines.
        //
        cancelled = PowerIndicateWaitWakeStatus(STATUS_CANCELLED);
    }
    else {
        wwIrp = (MdIrp) InterlockedExchangePointer(
            (PVOID*) &m_SharedPower.m_WaitWakeIrp, NULL);

        if (wwIrp != NULL) {
            FxIrp irp(wwIrp);

            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                                "Successfully got WaitWake irp %p for cancelling", wwIrp);

            result = irp.Cancel();

            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                                "Cancel of irp %p returned %d", wwIrp, result);


            if (m_PowerPolicyMachine.CanCompleteWaitWakeIrp()) {
                CompletePowerRequest(&irp, irp.GetStatus());
            }
            else {
                //
                // Irp has been completed by the lower bus driver, that's OK
                // because the completion of the request will trigger the
                // same transition
                //
                DO_NOTHING();
            }

            cancelled = TRUE;
        }
        else {
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                                "No WaitWake irp to cancel");

            cancelled = FALSE;
        }
    }

    return cancelled;
}

__drv_sameIRQL
NTSTATUS
FxPkgPnp::_PowerPolicyWaitWakeCompletionRoutine(
    __in MdDeviceObject DeviceObject,
    __in MdIrp OriginalIrp,
    __in_xcount_opt("varies") PVOID Context
    )
{
    FxIrp originalIrp(OriginalIrp);
    MdIrp pOldIrp;
    FxPkgPnp *pThis;
    NTSTATUS status;

    pThis = (FxPkgPnp*) Context;

    if (originalIrp.PendingReturned()) {
        originalIrp.MarkIrpPending();
    }

    DoTraceLevelMessage(pThis->GetDriverGlobals(),
                        TRACE_LEVEL_INFORMATION, TRACINGPNP,
                        "WDFDEVICE %p !devobj %p Completion of WaitWake irp %p,"
                        " %!STATUS!", pThis->m_Device->GetHandle(),
                        DeviceObject, originalIrp.GetIrp(),
                        originalIrp.GetStatus());

    if (NT_SUCCESS(originalIrp.GetStatus())) {

        //
        // Check to see if this device caused the machine to wake up
        //
        pThis->PowerPolicyUpdateSystemWakeSource(&originalIrp);
    }

    if (pThis->m_SystemWokenByWakeInterrupt) {
        //
        // If the system was woken by a wake interrupt, we need to mark this
        // device as the wake source. Since we typically cancel the wait
        // wake IRP in this path, it is expected that the completion
        // status will not be success. We need to change this status code to
        // success because power manager ignores the wake source information
        // reported by a wait wake IRP that is not completed successfully.
        //
        pThis->_PowerSetSystemWakeSource(&originalIrp);
        originalIrp.SetStatus(STATUS_SUCCESS);
    }

    //
    // Attempt to gain exclusive ownership of the irp from the cancel call site.
    // If we do (indicated by the exchange returning a non-NULL pointer), we can
    // complete the request w/out worrying about the cancel call site.
    //
    // If we can't, we must check to see if we can complete the wait wake irp
    // (the cancel call site at least has the irp pointer value and has called
    // IoCancelIrp on it).
    //
    pOldIrp = (MdIrp) InterlockedExchangePointer(
        (PVOID*)&pThis->m_SharedPower.m_WaitWakeIrp, NULL);

    ASSERT(pOldIrp == NULL || pOldIrp == originalIrp.GetIrp());

    if (pOldIrp != NULL ||
        pThis->m_PowerPolicyMachine.CanCompleteWaitWakeIrp()) {
        DoTraceLevelMessage(pThis->GetDriverGlobals(),
                            TRACE_LEVEL_VERBOSE, TRACINGPNP,
                            "Completion of WaitWake irp %p",
                            originalIrp.GetIrp());

        originalIrp.StartNextPowerIrp();

        status = STATUS_CONTINUE_COMPLETION;

        Mx::MxReleaseRemoveLock(
            &FxDevice::_GetFxWdmExtension(
                DeviceObject)->IoRemoveLock,
            originalIrp.GetIrp()
            );
    }
    else {
        //
        // ************ WARNING *************
        // By this time the IRP may have got completed by cancel call site, so
        // don't touch irp *members* in this path (it is ok to use the IRP
        // address though as in the log below).
        //
        DoTraceLevelMessage(
            pThis->GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
            "Not completing WaitWake irp %p in completion routine",
            originalIrp.GetIrp());

        status = STATUS_MORE_PROCESSING_REQUIRED;
    }

    return status;
}

__drv_sameIRQL
NTSTATUS
FxPkgPnp::_PowerPolicyUsbSelectiveSuspendCompletionRoutine(
    __in MdDeviceObject DeviceObject,
    __in MdIrp Irp,
    __in_xcount_opt("varies") PVOID Context
    )
{
    FxPkgPnp* This;
    FxIrp irp(Irp);

    UNREFERENCED_PARAMETER(DeviceObject);

    This = (FxPkgPnp*) Context;

    //
    // Parameters DeviceObejct and Irp are always set to NULL in UMDF, so
    // don't touch these in UMDF trace
    //
#if FX_IS_KERNEL_MODE
    DoTraceLevelMessage(This->GetDriverGlobals(),
        TRACE_LEVEL_INFORMATION, TRACINGPNP,
        "WDFDEVICE %p, !devobj %p Completion of UsbSS irp %p, %!STATUS!",
        This->m_Device, This->m_Device->GetDeviceObject(),
        irp.GetIrp(), irp.GetStatus());

#elif FX_IS_USER_MODE
    UNREFERENCED_PARAMETER(irp);

    DoTraceLevelMessage(This->GetDriverGlobals(),
        TRACE_LEVEL_INFORMATION, TRACINGPNP,
        "WDFDEVICE %p, !devobj %p Completion of UsbSS irp",
        This->m_Device, This->m_Device->GetDeviceObject());
#endif

    //
    // Post an event indicating that the irp has been completed
    //
    This->PowerPolicyProcessEvent(
        PwrPolUsbSelectiveSuspendCompleted
        );

    return STATUS_MORE_PROCESSING_REQUIRED;
}

BOOLEAN
FxPkgPnp::PowerPolicyCanIdlePowerDown(
    __in DEVICE_POWER_STATE DxState
    )
/*++

Routine Description:
    Attempts to send a Dx irp down the stack after this device has idled out.
    Before the Dx can be set, we must check that no child devices have attempted
    to power up in between the idle timer firing and the state machine
    processing the event.  After we have determined that no children are in
    D0, we setup a guard so that any child which attempts to power up after this
    point will pend the power up until this device has either powered all the
    way down and back up or could not allocate a device power irp

Arguments:
    DxState - the destination device state

Return Value:
    TRUE if the device power irp was sent, FALSE otherwise

  --*/
{
    BOOLEAN powerDown;

    //
    // If we potentially have children, make sure that they are all in Dx before
    // the parent powers down.
    //
    if (m_EnumInfo != NULL) {
        m_EnumInfo->AcquireParentPowerStateLock(GetDriverGlobals());
        if (m_PowerPolicyMachine.m_Owner->m_ChildrenPoweredOnCount == 0) {
            //
            // Setup a guard so that no children power up until we either fail
            // this power down or power all the way down and back up.
            //
            m_PowerPolicyMachine.m_Owner->m_ChildrenCanPowerUp = FALSE;
            powerDown = TRUE;
        }
        else {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
                "WDFDEVICE %p !devobj 0x%p not idling out because there are %d "
                "children who are powered up", m_Device->GetHandle(),
                m_Device->GetDeviceObject(),
                m_PowerPolicyMachine.m_Owner->m_ChildrenPoweredOnCount);
            powerDown = FALSE;
        }
        m_EnumInfo->ReleaseParentPowerStateLock(GetDriverGlobals());
    }
    else {
        powerDown = TRUE;
    }

    if (powerDown) {
        NTSTATUS status;

        status = PowerPolicySendDevicePowerRequest(DxState, NoRetry);

        if (!NT_SUCCESS(status)) {
            //
            // This will set m_ChildrenCanPowerUp to TRUE and send a
            // ParentMovesToD0 in case any children powered down in between
            // determining if the parent can go idle and failing to do so.
            //
            PowerPolicyChildrenCanPowerUp();

            powerDown = FALSE;
        }
    }

    return powerDown;
}

VOID
FxPkgPnp::PowerPolicyPostParentToD0ToChildren(
    VOID
    )
/*++

Routine Description:
    Indicates to all of the children that the parent is in D0

Arguments:
    None

Return Value:
    None.

  --*/
{
    FxTransactionedEntry* ple;

    ASSERT(IsPowerPolicyOwner());

    if (m_EnumInfo != NULL) {
        m_EnumInfo->m_ChildListList.LockForEnum(GetDriverGlobals());

        ple = NULL;
        while ((ple = m_EnumInfo->m_ChildListList.GetNextEntry(ple)) != NULL) {
            ((FxChildList*) ple->GetTransactionedObject())->PostParentToD0();
        }

        m_EnumInfo->m_ChildListList.UnlockFromEnum(GetDriverGlobals());
    }
}

VOID
FxPkgPnp::PowerPolicyChildrenCanPowerUp(
    VOID
    )
/*++

Routine Description:
    After this function returns, any child devices rooted off of this parent
    device can now move into D0.

Arguments:
    None

Return Value:
    None

  --*/
{
    //
    // This can be called for any PPO so we must check first if we have any
    // possibility of children.
    //
    if (m_EnumInfo == NULL) {
        return;
    }

    if (IsPowerPolicyOwner()) {
        m_EnumInfo->AcquireParentPowerStateLock(GetDriverGlobals());

        //
        // When the child attempts to power up, m_ChildrenPoweredOnCount can
        // be incremented while the parent is in Dx. The important value
        // here is the guard value, m_ChildrenCanPowerUp which must be
        // FALSE.
        //
        // ASSERT (m_PowerPolicyMachine.m_Owner->m_ChildrenPoweredOnCount == 0);

        //
        // In the USB SS case, we can return to StartingDecideS0Wake without
        // ever attempting to go into a Dx state state, so m_ChildrenCanPowerUp
        // can be TRUE.
        //
        // ASSERT(m_PowerPolicyMachine.m_Owner->m_ChildrenCanPowerUp == FALSE);

        m_PowerPolicyMachine.m_Owner->m_ChildrenCanPowerUp = TRUE;
        m_EnumInfo->ReleaseParentPowerStateLock(GetDriverGlobals());

        //
        // Now that we have set the state of the parent, any child which checks
        // the power state after we have released the lock will be able to
        // power up immediately.  We now need to unblock all children which
        // checked the parent state before this function was called by posting
        // a PowerParentToD0 event to each child (regardless of the PDO device
        // power state).
        //
        PowerPolicyPostParentToD0ToChildren();
    }
    else {
        //
        // Since we are not the power policy owner for the parent device,
        // we cannot make any guarantees about the child being in D0 while
        // the parent is in D0 so we do not call PowerPostParentToD0ToChildren().
        // Additionally in PowerPolicyCanChildPowerUp(), if the parent (this)
        // device is not the PPO, we don't even check the parent D state before
        // powering up the child.
        //
        DO_NOTHING();
    }
}

VOID
__inline
FxPkgPnp::PowerPolicyDisarmWakeFromSx(
    VOID
    )
/*++

Routine Description:
    Calls into the client driver to disarm itself from wake and the clears the
    flag which indicates that the device is a source of system wake.  The disarm
    callback is the last callback available to the driver to indicate the wake
    status of its children and have the wake status propagate upwards to the
    PDO's stack.

  --*/
{
    NTSTATUS wwStatus;
    FxTransactionedEntry* ple;

    m_PowerPolicyMachine.m_Owner->m_DeviceDisarmWakeFromSx.Invoke(
        m_Device->GetHandle()
        );

    wwStatus = m_PowerPolicyMachine.m_Owner->m_WaitWakeStatus;

    if (wwStatus != STATUS_CANCELLED &&
        m_EnumInfo != NULL &&
        PowerPolicyShouldPropagateWakeStatusToChildren()) {
        m_EnumInfo->m_ChildListList.LockForEnum(GetDriverGlobals());

        ple = NULL;
        while ((ple = m_EnumInfo->m_ChildListList.GetNextEntry(ple)) != NULL) {
            ((FxChildList*) ple->GetTransactionedObject())->
                IndicateWakeStatus(wwStatus);
        }

        m_EnumInfo->m_ChildListList.UnlockFromEnum(GetDriverGlobals());
    }

    m_PowerPolicyMachine.m_Owner->m_WaitWakeStatus = STATUS_NOT_SUPPORTED;

    //
    // Always set the wake source back to FALSE regardless of it previous value.
    //
    m_PowerPolicyMachine.m_Owner->m_SystemWakeSource = FALSE;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolWaitingArmedStoppingCancelUsbSS(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device is in Dx and surprise removed. Cancel the idle notification so
    we can move to another state

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolStoppingCancelWake

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolWaitingArmedStoppingCancelUsbSS);

    This->m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.DisableTimer();

    //
    // Since we are in Dx and surprise removed, cancel the usb SS irp if it is
    // there.  Otherwise, the USB PDO will call us back after we have processed
    // remove.
    //
    if (This->PowerPolicyCancelUsbSSIfCapable()) {
        //
        // Usbss completion event will move us from this state.
        //
        return WdfDevStatePwrPolNull;
    }

    //
    // If usbss irp was there it has already been canceled, so march on.
    //
    return WdfDevStatePwrPolStoppingCancelWake;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolWaitingArmedWakeFailedCancelUsbSS(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device is in Dx armed for wake and wait wake irp got failed.
    Cancel the idle notification so we can move to another state

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolIoPresentArmedWakeCanceled

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolWaitingArmedWakeFailedCancelUsbSS);

    if (This->PowerPolicyCancelUsbSSIfCapable()) {
        //
        // Usbss completion event will move us from this state.
        //
        return WdfDevStatePwrPolNull;
    }

    //
    // If usbss irp was there it has already been canceled, so march on.
    //
    return WdfDevStatePwrPolIoPresentArmedWakeCanceled;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolWaitingArmedIoPresentCancelUsbSS(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device is in Dx armed for wake.
    Cancel the idle notification so we can move to another state

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolIoPresentArmed

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolWaitingArmedIoPresentCancelUsbSS);

    if (This->PowerPolicyCancelUsbSSIfCapable()) {
        //
        // Usbss completion event will move us from this state.
        //
        return WdfDevStatePwrPolNull;
    }

    //
    // If usbss irp was there it has already been canceled, so march on.
    //
    return WdfDevStatePwrPolIoPresentArmed;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolWaitingArmedWakeSucceededCancelUsbSS(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device woke from S0.
    Cancel the idle notification so we can move to another state

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolWokeFromS0

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolWaitingArmedWakeSucceededCancelUsbSS);

    if (This->PowerPolicyCancelUsbSSIfCapable()) {
        //
        // Usbss completion event will move us from this state.
        //
        return WdfDevStatePwrPolNull;
    }

    //
    // If usbss irp was there it has already been canceled, so march on.
    //
    return WdfDevStatePwrPolWokeFromS0;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolCancelingUsbSSForSystemSleep(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The system is going to sleep..
    Cancel the idle notification so we can move to another state

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolCancelingWakeForSystemSleep

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolCancelingUsbSSForSystemSleep);

    if (This->PowerPolicyCancelUsbSSIfCapable()) {
        //
        // Usbss completion event will move us from this state.
        //
        return WdfDevStatePwrPolNull;
    }

    //
    // If usbss irp was there it has already been canceled, so march on.
    //
    return WdfDevStatePwrPolCancelingWakeForSystemSleep;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStoppingD0CancelUsbSS(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Cancel the idle notification so we can move to another state

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePwrPolStoppingD0

  --*/
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolStoppingD0CancelUsbSS);

    if (This->PowerPolicyCancelUsbSSIfCapable()) {
        //
        // Usbss completion event will move us from this state.
        //
        return WdfDevStatePwrPolNull;
    }

    //
    // If usbss irp was there it has already been canceled, so march on.
    //
    return WdfDevStatePwrPolStoppingD0;
}

BOOLEAN
FxPkgPnp::PowerPolicyCancelUsbSSIfCapable(
    VOID
    )
/*++

Routine Description:
    Cancel the idle notification irp if capable

Arguments:
    none

Return Value:
    TRUE if irp was canceled
    FALSE if not capable of USBSS or if Irp was already completed.

  --*/
{
    if (m_PowerPolicyMachine.m_Owner->m_UsbIdle == NULL ||
        m_PowerPolicyMachine.m_Owner->m_UsbIdle->m_EventDropped) {
        return FALSE;
    }
    else {
        PowerPolicyCancelUsbSS();
        return TRUE;
    }
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredWakeCapableWakeInterruptArrived(
    __inout FxPkgPnp* This
    )
{
    ASSERT_PWR_POL_STATE(This, WdfDevStatePwrPolTimerExpiredWakeCapableWakeInterruptArrived);

    if (This->PowerPolicyCancelWaitWake() == FALSE &&
        This->m_PowerPolicyMachine.m_Owner->m_WakeCompletionEventDropped) {
        return WdfDevStatePwrPolTimerExpiredWakeCapableWakeSucceeded;
    }

    return WdfDevStatePwrPolNull;
}


