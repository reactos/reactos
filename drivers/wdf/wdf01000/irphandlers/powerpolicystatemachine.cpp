#include "common/fxpowerpolicystatemachine.h"
#include "common/fxpkgpnp.h"
#include "common/fxpoweridlestatemachine.h"
#include "common/fxdevice.h"
#include "common/fxwatchdog.h"


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



const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolStartingOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolStartingFailed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolStartedIdleCapableOtherStates[] =
{
    { PwrPolSx, WdfDevStatePwrPolStartedIdleCapableCancelTimerForSleep },
    { PwrPolStop, WdfDevStatePwrPolStoppingCancelTimer },
    { PwrPolSurpriseRemove, WdfDevStatePwrPolStoppingCancelTimer },
    { PwrPolS0IdlePolicyChanged, WdfDevStatePwrPolStartedCancelTimer },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolTimerExpiredNoWakeOtherStates[] =
{
    { PwrPolPowerDown, WdfDevStatePwrPolTimerExpiredNoWakePowerDownNotProcessed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolTimerExpiredNoWakeCompletePowerDownOtherStates[] =
{
    { PwrPolPowerDownFailed, WdfDevStatePwrPolDevicePowerRequestFailed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolWaitingUnarmedOtherStates[] =
{
    { PwrPolSx, WdfDevStatePwrPolSystemSleepFromDeviceWaitingUnarmed },
    { PwrPolStop, WdfDevStatePwrPolStoppingResetDevice },
    { PwrPolSurpriseRemove, WdfDevStatePwrPolStoppingCancelTimer },
    { PwrPolS0IdlePolicyChanged, WdfDevStatePwrPolS0NoWakePowerUp },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolS0NoWakePowerUpOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolDevicePowerRequestFailed },
    { PwrPolPowerUpNotSeen, WdfDevStatePwrPolDeviceD0PowerRequestFailed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolS0NoWakeCompletePowerUpOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolDevicePowerRequestFailed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemSleepNeedWakeOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolSystemSleepPowerRequestFailed },
    { PwrPolPowerUpNotSeen, WdfDevStatePwrPolPowerUpForSystemSleepNotSeen },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemSleepNeedWakeCompletePowerUpOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolSystemSleepPowerRequestFailed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemAsleepWakeArmedOtherStates[] =
{
    { PwrPolWakeSuccess, WdfDevStatePwrPolSystemWakeDeviceWakeTriggered },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemAsleepWakeArmedNPOtherStates[] =
{
    { PwrPolWakeSuccess, WdfDevStatePwrPolSystemWakeDeviceWakeTriggeredNP },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemWakeDeviceToD0OtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolDevicePowerRequestFailed },
    { PwrPolPowerUpNotSeen, WdfDevStatePwrPolDeviceD0PowerRequestFailed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemWakeDeviceToD0CompletePowerUpOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolDevicePowerRequestFailed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolStartedWakeCapableOtherStates[] =
{
    { PwrPolSx, WdfDevStatePwrPolStartedWakeCapableCancelTimerForSleep },
    { PwrPolStop, WdfDevStatePwrPolStoppingCancelTimer },
    { PwrPolSurpriseRemove, WdfDevStatePwrPolStoppingCancelTimer },
    { PwrPolS0IdlePolicyChanged, WdfDevStatePwrPolStartedCancelTimer },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSleepingWakePowerDownOtherStates[] =
{
    { PwrPolPowerDown, WdfDevStatePwrPolSleepingPowerDownNotProcessed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolTimerExpiredWakeCapablePowerDownOtherStates[] =
{
    { PwrPolPowerDown, WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownNotProcessed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolTimerExpiredWakeCapableSendWakeOtherStates[] =
{
    { PwrPolWakeFailed, WdfDevStatePwrPolTimerExpiredWakeCompletedDisarm },
    { PwrPolWakeSuccess, WdfDevStatePwrPolTimerExpiredWakeSucceeded },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolTimerExpiredWakeCapableUsbSSOtherStates[] =
{
    { PwrPolSx, WdfDevStatePwrPolStartedWakeCapableSleepingUsbSS },
    { PwrPolStop, WdfDevStatePwrPolStoppingCancelUsbSS },
    { PwrPolSurpriseRemove, WdfDevStatePwrPolStoppingCancelUsbSS },
    { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolStartedWakeCapable },
    { PwrPolS0IdlePolicyChanged, WdfDevStatePwrPolCancelUsbSS },
    { PwrPolIoPresent, WdfDevStatePwrPolCancelUsbSS },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolWaitingArmedOtherStates[] =
{
    { PwrPolSx, WdfDevStatePwrPolCancelingUsbSSForSystemSleep },
    { PwrPolWakeSuccess, WdfDevStatePwrPolWaitingArmedWakeSucceededCancelUsbSS },
    { PwrPolWakeFailed, WdfDevStatePwrPolWaitingArmedWakeFailedCancelUsbSS },
    { PwrPolStop, WdfDevStatePwrPolStoppingD0CancelUsbSS },
    { PwrPolSurpriseRemove, WdfDevStatePwrPolWaitingArmedStoppingCancelUsbSS },
    { PwrPolS0IdlePolicyChanged, WdfDevStatePwrPolWaitingArmedIoPresentCancelUsbSS },
    { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolIoPresentArmed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolDisarmingWakeForSystemSleepCompletePowerUpOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolPowerUpForSystemSleepFailed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolCancelingWakeForSystemSleepWakeCanceledOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolPowerUpForSystemSleepFailed },
    { PwrPolPowerUpNotSeen, WdfDevStatePwrPolPowerUpForSystemSleepNotSeen },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolWokeFromS0OtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolDevicePowerRequestFailed },
    { PwrPolPowerUpNotSeen, WdfDevStatePwrPolDeviceD0PowerRequestFailed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolStoppingResetDeviceOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolStoppingResetDeviceFailed },
    { PwrPolPowerUpNotSeen, WdfDevStatePwrPolDeviceD0PowerRequestFailed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolStoppingResetDeviceCompletePowerUpOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolStoppingResetDeviceFailed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolStoppingD0OtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolStoppingD0Failed },
    { PwrPolPowerUpNotSeen, WdfDevStatePwrPolDeviceD0PowerRequestFailed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolStoppingDisarmWakeOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolStoppingD0Failed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolStoppingDisarmWakeCancelWakeOtherStates[] =
{
    { PwrPolWakeSuccess, WdfDevStatePwrPolStoppingDisarmWakeWakeCanceled },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolStartedOtherStates[] =
{
    { PwrPolStop, WdfDevStatePwrPolStopping },
    { PwrPolSurpriseRemove, WdfDevStatePwrPolStopping },
    { PwrPolS0IdlePolicyChanged, WdfDevStatePwrPolStartingDecideS0Wake },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolStartedWaitForIdleTimeoutOtherStates[] =
{
    { PwrPolStop, WdfDevStatePwrPolStoppingWaitForIdleTimeout },
    { PwrPolSurpriseRemove, WdfDevStatePwrPolStoppingWaitForIdleTimeout },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolIoPresentArmedOtherStates[] =
{
    { PwrPolWakeSuccess, WdfDevStatePwrPolIoPresentArmedWakeCanceled },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolIoPresentArmedWakeCanceledOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolDevicePowerRequestFailed },
    { PwrPolPowerUpNotSeen, WdfDevStatePwrPolDeviceD0PowerRequestFailed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolS0WakeCompletePowerUpOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolDevicePowerRequestFailed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolTimerExpiredWakeCapableWakeSucceededOtherStates[] =
{
    { PwrPolPowerDownFailed, WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownFailedUsbSS },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolTimerExpiredWakeCapableWakeFailedOtherStates[] =
{
    { PwrPolPowerDownFailed, WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownFailedUsbSS },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolTimerExpiredWakeCapablePowerDownFailedCancelWakeOtherStates[] =
{
    { PwrPolWakeFailed, WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownFailedWakeCanceled },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolCancelingWakeForSystemSleepOtherStates[] =
{
    { PwrPolWakeSuccess, WdfDevStatePwrPolCancelingWakeForSystemSleepWakeCanceled },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolTimerExpiredWakeCapableWakeArrivedOtherStates[] =
{
    { PwrPolWakeSuccess, WdfDevStatePwrPolTimerExpiredWakeCapableWakeSucceeded },
    { PwrPolWakeFailed, WdfDevStatePwrPolTimerExpiredWakeCapableWakeFailed },
    { PwrPolPowerDownFailed, WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownFailedCancelWake },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolTimerExpiredWakeCapableCancelWakeOtherStates[] =
{
    { PwrPolWakeFailed, WdfDevStatePwrPolTimerExpiredWakeCapableWakeCanceled },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolTimerExpiredWakeCompletedPowerDownOtherStates[] =
{
    { PwrPolPowerDownFailed, WdfDevStatePwrPolDevicePowerRequestFailed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolTimerExpiredWakeCompletedPowerUpOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolDevicePowerRequestFailed },
    { PwrPolPowerUpNotSeen, WdfDevStatePwrPolDeviceD0PowerRequestFailed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSleepingWakeWakeArrivedOtherStates[] =
{
    { PwrPolPowerDownFailed, WdfDevStatePwrPolSleepingWakePowerDownFailed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemWakeDeviceWakeTriggeredS0OtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolDevicePowerRequestFailed },
    { PwrPolPowerUpNotSeen, WdfDevStatePwrPolDeviceD0PowerRequestFailed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemWakeDeviceWakeTriggeredS0NPOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolDevicePowerRequestFailed },
    { PwrPolPowerUpNotSeen, WdfDevStatePwrPolDeviceD0PowerRequestFailed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemWakeDeviceWakeCompletePowerUpOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolDevicePowerRequestFailed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSleepingNoWakePowerDownOtherStates[] =
{
    { PwrPolPowerDown, WdfDevStatePwrPolSleepingPowerDownNotProcessed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSleepingNoWakeCompletePowerDownOtherStates[] =
{
    { PwrPolPowerDownFailed, WdfDevStatePwrPolSystemSleepPowerRequestFailed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSleepingSendWakeOtherStates[] =
{
    { PwrPolWakeSuccess, WdfDevStatePwrPolSleepingNoWakeCompletePowerDown },
    { PwrPolWakeFailed, WdfDevStatePwrPolSleepingNoWakeCompletePowerDown },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSleepingWakeWakeArrivedNPOtherStates[] =
{
    { PwrPolPowerDownFailed, WdfDevStatePwrPolSleepingWakePowerDownFailed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSleepingWakePowerDownFailedOtherStates[] =
{
    { PwrPolWakeFailed, WdfDevStatePwrPolSleepingWakePowerDownFailedWakeCanceled },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemWakeDeviceWakeEnabledOtherStates[] =
{
    { PwrPolWakeSuccess, WdfDevStatePwrPolSystemWakeDeviceWakeEnabledWakeCanceled },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemWakeDeviceWakeEnabledWakeCanceledOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolDevicePowerRequestFailed },
    { PwrPolPowerUpNotSeen, WdfDevStatePwrPolDeviceD0PowerRequestFailed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemWakeDeviceWakeEnabledNPOtherStates[] =
{
    { PwrPolWakeSuccess, WdfDevStatePwrPolSystemWakeDeviceWakeEnabledWakeCanceledNP },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolSystemWakeDeviceWakeEnabledWakeCanceledNPOtherStates[] =
{
    { PwrPolPowerUpFailed, WdfDevStatePwrPolDevicePowerRequestFailed },
    { PwrPolPowerUpNotSeen, WdfDevStatePwrPolDeviceD0PowerRequestFailed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolDevicePowerRequestFailedOtherStates[] =
{
    { PwrPolSurpriseRemove, WdfDevStatePwrPolStopping },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolStoppingOtherStates[] =
{
    { PwrPolPowerDownFailed, WdfDevStatePwrPolStoppingFailed },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolStoppingCancelWakeOtherStates[] =
{
    { PwrPolWakeSuccess, WdfDevStatePwrPolStopping },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_EVENT_TARGET_STATE FxPkgPnp::m_PowerPolCancelUsbSSOtherStates[] =
{
    { PwrPolStop, WdfDevStatePwrPolStoppingWaitForUsbSSCompletion },
    { PwrPolSurpriseRemove, WdfDevStatePwrPolStoppingWaitForUsbSSCompletion },
    { PwrPolNull, WdfDevStatePwrPolNull },
};

const POWER_POLICY_STATE_TABLE FxPkgPnp::m_WdfPowerPolicyStates[] =
{
    { NULL,
      { PwrPolStart, WdfDevStatePwrPolStarting },
      NULL,
      { TRUE,
        PwrPolStart
        | PwrPolS0
        | PwrPolPowerDown
        | PwrPolSurpriseRemove
      },
    },

    { FxPkgPnp::PowerPolStarting,
      { PwrPolPowerUp, WdfDevStatePwrPolStartingSucceeded },
      FxPkgPnp::m_PowerPolStartingOtherStates,
      { FALSE,
        PwrPolImplicitPowerDown
      },
    },

    { FxPkgPnp::PowerPolStartingSucceeded,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolStartingFailed,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolStartingDecideS0Wake,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolStartedWakeCapable,
      { PwrPolPowerTimeoutExpired, WdfDevStatePwrPolTimerExpiredNoWake },
      FxPkgPnp::m_PowerPolStartedIdleCapableOtherStates,
      { TRUE,
        PwrPolStart
        | PwrPolPowerDown
        | PwrPolPowerDownIoStopped
        | PwrPolWakeSuccess
        | PwrPolPowerTimeoutExpired
      },
    },

    { FxPkgPnp::PowerPolTimerExpiredNoWake,
      { PwrPolPowerDownIoStopped, WdfDevStatePwrPolTimerExpiredNoWakeCompletePowerDown },
      FxPkgPnp::m_PowerPolTimerExpiredNoWakeOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolSleepingNoWakeCompletePowerDown,
      { PwrPolPowerDown, WdfDevStatePwrPolTimerExpiredNoWakePoweredDownDisableIdleTimer },
      FxPkgPnp::m_PowerPolTimerExpiredNoWakeCompletePowerDownOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { NULL,
      { PwrPolIoPresent, WdfDevStatePwrPolWaitingUnarmedQueryIdle },
      FxPkgPnp::m_PowerPolWaitingUnarmedOtherStates,
      { TRUE,
        PwrPolStart
        | PwrPolPowerDown
      },
    },

    { FxPkgPnp::PowerPolSystemWakeQueryIdle,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolSystemWakeDeviceWakeTriggeredS0NP,
      { PwrPolPowerUpHwStarted, WdfDevStatePwrPolS0NoWakeCompletePowerUp },
      FxPkgPnp::m_PowerPolS0NoWakePowerUpOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::NotPowerPolOwnerGotoD0,
      { PwrPolPowerUp, WdfDevStatePwrPolStartingDecideS0Wake },
      FxPkgPnp::m_PowerPolS0NoWakeCompletePowerUpOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolSystemSleepFromDeviceWaitingUnarmed,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolSystemSleepNeedWake,
      { PwrPolPowerUpHwStarted, WdfDevStatePwrPolSystemSleepNeedWakeCompletePowerUp },
      FxPkgPnp::m_PowerPolSystemSleepNeedWakeOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::NotPowerPolOwnerGotoD0,
      { PwrPolPowerUp, WdfDevStatePwrPolSleeping },
      FxPkgPnp::m_PowerPolSystemSleepNeedWakeCompletePowerUpOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolPowerUpForSystemSleepFailed,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolCheckPowerPageable,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolSleepingWakeWakeArrived,
      { PwrPolPowerDown, WdfDevStatePwrPolSystemAsleepWakeArmed },
      FxPkgPnp::m_PowerPolSleepingWakeWakeArrivedOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolSleepingWakeRevertArmWake,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolSystemAsleepWakeArmedNP,
      { PwrPolS0, WdfDevStatePwrPolSystemWakeDeviceWakeEnabled },
      FxPkgPnp::m_PowerPolSystemAsleepWakeArmedOtherStates,
      { TRUE,
        PwrPolStart
        | PwrPolIoPresent
        | PwrPolPowerTimeoutExpired
        | PwrPolS0IdlePolicyChanged
      },
    },

    { FxPkgPnp::PowerPolSystemWakeDeviceWakeEnabled,
      { PwrPolWakeFailed, WdfDevStatePwrPolSystemWakeDeviceWakeEnabledWakeCanceled },
      FxPkgPnp::m_PowerPolSystemWakeDeviceWakeEnabledOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolSystemWakeDeviceWakeTriggeredS0NP,
      { PwrPolPowerUpHwStarted, WdfDevStatePwrPolSystemWakeDeviceWakeDisarm },
      FxPkgPnp::m_PowerPolSystemWakeDeviceWakeEnabledWakeCanceledOtherStates,
      { FALSE,
        PwrPolWakeFailed
      },
    },

    { FxPkgPnp::PowerPolSystemWakeDeviceWakeDisarm,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { NULL,
      { PwrPolS0, WdfDevStatePwrPolSystemWakeDeviceWakeTriggeredS0 },
      NULL,
      { TRUE,
        PwrPolStart
      },
    },

    { FxPkgPnp::PowerPolSystemWakeDeviceWakeTriggeredS0NP,
      { PwrPolPowerUpHwStarted, WdfDevStatePwrPolSystemWakeDeviceWokeDisarm },
      FxPkgPnp::m_PowerPolSystemWakeDeviceWakeTriggeredS0OtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolSystemWakeDeviceWokeDisarm,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolSleepingWakeWakeArrivedNP,
      { PwrPolPowerDown, WdfDevStatePwrPolSystemAsleepWakeArmedNP },
      FxPkgPnp::m_PowerPolSleepingWakeWakeArrivedNPOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolSleepingWakeRevertArmWakeNP,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolSleepingWakePowerDownFailed,
      { PwrPolWakeSuccess, WdfDevStatePwrPolSleepingWakePowerDownFailedWakeCanceled },
      FxPkgPnp::m_PowerPolSleepingWakePowerDownFailedOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolPowerUpForSystemSleepFailed,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolSystemAsleepWakeArmedNP,
      { PwrPolS0, WdfDevStatePwrPolSystemWakeDeviceWakeEnabledNP },
      FxPkgPnp::m_PowerPolSystemAsleepWakeArmedNPOtherStates,
      { TRUE,
        PwrPolStart
        | PwrPolIoPresent
        | PwrPolPowerTimeoutExpired
        | PwrPolS0IdlePolicyChanged
      },
    },

    { FxPkgPnp::PowerPolSystemWakeDeviceWakeEnabledNP,
      { PwrPolWakeFailed, WdfDevStatePwrPolSystemWakeDeviceWakeEnabledWakeCanceledNP },
      FxPkgPnp::m_PowerPolSystemWakeDeviceWakeEnabledNPOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolSystemWakeDeviceWakeTriggeredS0NP,
      { PwrPolPowerUpHwStarted, WdfDevStatePwrPolSystemWakeDeviceWakeDisarmNP },
      FxPkgPnp::m_PowerPolSystemWakeDeviceWakeEnabledWakeCanceledNPOtherStates,
      { FALSE,
        PwrPolWakeFailed
      },
    },

    { FxPkgPnp::PowerPolSystemWakeDeviceWakeDisarm,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { NULL,
      { PwrPolS0, WdfDevStatePwrPolSystemWakeDeviceWakeTriggeredS0NP },
      NULL,
      { TRUE,
        PwrPolStart
        | PwrPolPowerTimeoutExpired
      },
    },

    { FxPkgPnp::PowerPolSystemWakeDeviceWakeTriggeredS0NP,
      { PwrPolPowerUpHwStarted, WdfDevStatePwrPolSystemWakeDeviceWokeDisarmNP },
      FxPkgPnp::m_PowerPolSystemWakeDeviceWakeTriggeredS0NPOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolSystemWakeDeviceWokeDisarm,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::NotPowerPolOwnerGotoD0,
      { PwrPolPowerUp, WdfDevStatePwrPolStartingDecideS0Wake },
      FxPkgPnp::m_PowerPolSystemWakeDeviceWakeCompletePowerUpOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolSleeping,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolSleepingNoWakePowerDown,
      { PwrPolPowerDownIoStopped, WdfDevStatePwrPolSleepingNoWakeCompletePowerDown },
      FxPkgPnp::m_PowerPolSleepingNoWakePowerDownOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolSleepingNoWakeCompletePowerDown,
      { PwrPolPowerDown, WdfDevStatePwrPolSystemAsleepNoWake },
      FxPkgPnp::m_PowerPolSleepingNoWakeCompletePowerDownOtherStates,
      { FALSE,
        PwrPolWakeSuccess
      },
    },

    { FxPkgPnp::PowerPolSleepingNoWakeDxRequestFailed,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolSleepingWakePowerDown,
      { PwrPolPowerDownIoStopped, WdfDevStatePwrPolSleepingSendWake },
      FxPkgPnp::m_PowerPolSleepingWakePowerDownOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolSleepingSendWake,
      { PwrPolWakeArrived, WdfDevStatePwrPolCheckPowerPageable },
      FxPkgPnp::m_PowerPolSleepingSendWakeOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolSystemAsleepWakeArmedNP,
      { PwrPolS0, WdfDevStatePwrPolSystemWakeDeviceWakeDisabled },
      NULL,
      { TRUE,
        PwrPolStart
        | PwrPolWakeSuccess
        | PwrPolPowerTimeoutExpired
        | PwrPolS0IdlePolicyChanged
        | PwrPolSurpriseRemove
      },
    },

    { FxPkgPnp::PowerPolSystemWakeDeviceWakeDisabled,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolSystemWakeDeviceWakeTriggeredS0NP,
      { PwrPolPowerUpHwStarted, WdfDevStatePwrPolSystemWakeDeviceToD0CompletePowerUp },
      FxPkgPnp::m_PowerPolSystemWakeDeviceToD0OtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::NotPowerPolOwnerGotoD0,
      { PwrPolPowerUp, WdfDevStatePwrPolStartingDecideS0Wake },
      FxPkgPnp::m_PowerPolSystemWakeDeviceToD0CompletePowerUpOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolSystemWakeQueryIdle,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolStartedWakeCapable,
      { PwrPolPowerTimeoutExpired, WdfDevStatePwrPolTimerExpiredDecideUsbSS },
      FxPkgPnp::m_PowerPolStartedWakeCapableOtherStates,
      { TRUE,
        PwrPolStart
        | PwrPolPowerDown
        | PwrPolPowerDownIoStopped
        | PwrPolWakeSuccess
        | PwrPolWakeFailed
        | PwrPolIoPresent
        | PwrPolPowerTimeoutExpired
        | PwrPolUsbSelectiveSuspendCompleted
        | PwrPolPowerDownFailed
      },
    },

    { FxPkgPnp::PowerPolTimerExpiredDecideUsbSS,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolTimerExpiredWakeCapablePowerDown,
      { PwrPolPowerDownIoStopped, WdfDevStatePwrPolTimerExpiredWakeCapableSendWake },
      FxPkgPnp::m_PowerPolTimerExpiredWakeCapablePowerDownOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolTimerExpiredWakeCapableSendWake,
      { PwrPolWakeArrived, WdfDevStatePwrPolTimerExpiredWakeCapableWakeArrived },
      FxPkgPnp::m_PowerPolTimerExpiredWakeCapableSendWakeOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolTimerExpiredWakeCapableUsbSS,
      { PwrPolUsbSelectiveSuspendCallback, WdfDevStatePwrPolTimerExpiredWakeCapablePowerDown },
      FxPkgPnp::m_PowerPolTimerExpiredWakeCapableUsbSSOtherStates,
      { TRUE,
        PwrPolStart
      },
    },

    { FxPkgPnp::PowerPolTimerExpiredWakeCapableWakeArrived,
      { PwrPolPowerDown, WdfDevStatePwrPolWaitingArmedUsbSS },
      FxPkgPnp::m_PowerPolTimerExpiredWakeCapableWakeArrivedOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolTimerExpiredWakeCapableCancelWake,
      { PwrPolWakeSuccess, WdfDevStatePwrPolTimerExpiredWakeCapableWakeCanceled },
      FxPkgPnp::m_PowerPolTimerExpiredWakeCapableCancelWakeOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolTimerExpiredWakeCapableWakeCanceled,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolTimerExpiredWakeCapableCleanup,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolTimerExpiredWakeCompletedPowerDown },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolTimerExpiredWakeCapableDxAllocFailed,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolStartedCancelTimer },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolSleepingNoWakeCompletePowerDown,
      { PwrPolPowerDown, WdfDevStatePwrPolTimerExpiredWakeCompletedPowerUp },
      FxPkgPnp::m_PowerPolTimerExpiredWakeCompletedPowerDownOtherStates,
      { FALSE,
        PwrPolWakeSuccess
        | PwrPolWakeFailed
      },
    },

    { FxPkgPnp::PowerPolTimerExpiredWakeCompletedPowerUp,
      { PwrPolPowerUpHwStarted, WdfDevStatePwrPolS0WakeCompletePowerUp },
      FxPkgPnp::m_PowerPolTimerExpiredWakeCompletedPowerUpOtherStates,
      { FALSE,
        PwrPolWakeSuccess
        | PwrPolWakeFailed
      },
    },

    { FxPkgPnp::PowerPolWaitingArmedUsbSS,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { NULL,
      { PwrPolIoPresent, WdfDevStatePwrPolWaitingArmedQueryIdle },
      FxPkgPnp::m_PowerPolWaitingArmedOtherStates,
      { TRUE,
        PwrPolStart
        | PwrPolPowerDown
      },
    },

    { FxPkgPnp::PowerPolWaitingArmedQueryIdle,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolIoPresentArmed,
      { PwrPolWakeFailed, WdfDevStatePwrPolIoPresentArmedWakeCanceled },
      FxPkgPnp::m_PowerPolIoPresentArmedOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolSystemWakeDeviceWakeTriggeredS0NP,
      { PwrPolPowerUpHwStarted, WdfDevStatePwrPolS0WakeDisarm },
      FxPkgPnp::m_PowerPolIoPresentArmedWakeCanceledOtherStates,
      { FALSE,
        PwrPolWakeFailed
        | PwrPolIoPresent
      },
    },

    { FxPkgPnp::PowerPolS0WakeDisarm,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::NotPowerPolOwnerGotoD0,
      { PwrPolPowerUp, WdfDevStatePwrPolStartingDecideS0Wake },
      FxPkgPnp::m_PowerPolS0WakeCompletePowerUpOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolTimerExpiredWakeSucceeded,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolTimerExpiredWakeCompletedDisarm,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { NULL,
      { PwrPolPowerDown, WdfDevStatePwrPolWokeFromS0UsbSS },
      FxPkgPnp::m_PowerPolTimerExpiredWakeCapableWakeSucceededOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { NULL,
      { PwrPolPowerDown, WdfDevStatePwrPolWakeFailedUsbSS },
      FxPkgPnp::m_PowerPolTimerExpiredWakeCapableWakeFailedOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolWakeFailedUsbSS,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolIoPresentArmedWakeCanceled },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolTimerExpiredWakeCapablePowerDownFailedCancelWake,
      { PwrPolWakeSuccess, WdfDevStatePwrPolTimerExpiredWakeCapablePowerDownFailedWakeCanceled },
      FxPkgPnp::m_PowerPolTimerExpiredWakeCapablePowerDownFailedCancelWakeOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolTimerExpiredWakeCapablePowerDownFailedWakeCanceled,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolTimerExpiredWakeCapablePowerDownFailedUsbSS,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolDevicePowerRequestFailed },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolCancelingWakeForSystemSleep,
      { PwrPolWakeFailed, WdfDevStatePwrPolCancelingWakeForSystemSleepWakeCanceled },
      FxPkgPnp::m_PowerPolCancelingWakeForSystemSleepOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolCancelingWakeForSystemSleepWakeCanceled,
      { PwrPolPowerUpHwStarted, WdfDevStatePwrPolDisarmingWakeForSystemSleepCompletePowerUp },
      FxPkgPnp::m_PowerPolCancelingWakeForSystemSleepWakeCanceledOtherStates,
      { FALSE,
        PwrPolWakeFailed
      },
    },

    { FxPkgPnp::PowerPolDisarmingWakeForSystemSleepCompletePowerUp,
      { PwrPolPowerUp, WdfDevStatePwrPolStartedWakeCapableCancelTimerForSleep },
      FxPkgPnp::m_PowerPolDisarmingWakeForSystemSleepCompletePowerUpOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolPowerUpForSystemSleepFailed,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolWokeFromS0UsbSS,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolWokeFromS0 },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolSystemWakeDeviceWakeTriggeredS0NP,
      { PwrPolPowerUpHwStarted, WdfDevStatePwrPolWokeFromS0NotifyDriver },
      FxPkgPnp::m_PowerPolWokeFromS0OtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolWokeFromS0NotifyDriver,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolStoppingResetDevice,
      { PwrPolPowerUpHwStarted, WdfDevStatePwrPolStoppingResetDeviceCompletePowerUp },
      FxPkgPnp::m_PowerPolStoppingResetDeviceOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::NotPowerPolOwnerGotoD0,
      { PwrPolPowerUp, WdfDevStatePwrPolStopping },
      FxPkgPnp::m_PowerPolStoppingResetDeviceCompletePowerUpOtherStates,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolStoppingResetDeviceFailed,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolStoppingResetDevice,
      { PwrPolPowerUpHwStarted, WdfDevStatePwrPolStoppingDisarmWake },
      FxPkgPnp::m_PowerPolStoppingD0OtherStates,
      { FALSE,
        PwrPolWakeFailed
      },
    },

    { FxPkgPnp::PowerPolStoppingD0Failed,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolDisarmingWakeForSystemSleepCompletePowerUp,
      { PwrPolPowerUp, WdfDevStatePwrPolStoppingDisarmWakeCancelWake },
      FxPkgPnp::m_PowerPolStoppingDisarmWakeOtherStates,
      { FALSE,
        PwrPolPowerDownFailed
      },
    },

    { FxPkgPnp::PowerPolStoppingDisarmWakeCancelWake,
      { PwrPolWakeFailed, WdfDevStatePwrPolStoppingDisarmWakeWakeCanceled },
      FxPkgPnp::m_PowerPolStoppingDisarmWakeCancelWakeOtherStates,
      { FALSE,
        PwrPolPowerDownFailed
      },
    },

    { FxPkgPnp::PowerPolStoppingDisarmWakeWakeCanceled,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolPowerDownFailed
      },
    },

    { FxPkgPnp::PowerPolStopping,
      { PwrPolPowerDown, WdfDevStatePwrPolStoppingSendStatus },
      FxPkgPnp::m_PowerPolStoppingOtherStates,
      { FALSE,
        PwrPolPowerDownFailed
      },
    },

    { FxPkgPnp::PowerPolStoppingFailed,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolStoppingSendStatus,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolStoppingCancelTimer,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { NULL,
      { PwrPolPowerTimeoutExpired, WdfDevStatePwrPolStopping },
      NULL,
      { TRUE,
        PwrPolStart
      },
    },

    { FxPkgPnp::PowerPolStoppingCancelUsbSS,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { NULL,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolStoppingCancelTimer },
      NULL,
      { TRUE,
        PwrPolStart
      },
    },

    { FxPkgPnp::PowerPolStoppingCancelWake,
      { PwrPolWakeFailed, WdfDevStatePwrPolStopping },
      FxPkgPnp::m_PowerPolStoppingCancelWakeOtherStates,
      { TRUE,
        PwrPolStart
      },
    },

    { NULL,
      { PwrPolStart, WdfDevStatePwrPolStarting },
      NULL,
      { TRUE,
        PwrPolStart
        | PwrPolS0
        | PwrPolPowerDown
        | PwrPolWakeFailed
        | PwrPolIoPresent
        | PwrPolPowerTimeoutExpired
        | PwrPolS0IdlePolicyChanged
        | PwrPolSurpriseRemove
        | PwrPolPowerDownFailed
      },
    },

    { FxPkgPnp::PowerPolCancelUsbSS,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolStartedCancelTimer },
      FxPkgPnp::m_PowerPolCancelUsbSSOtherStates,
      { TRUE,
        PwrPolStart
      },
    },

    { FxPkgPnp::PowerPolStarted,
      { PwrPolSx, WdfDevStatePwrPolSleeping },
      FxPkgPnp::m_PowerPolStartedOtherStates,
      { TRUE,
        PwrPolStart
        | PwrPolPowerDown
        | PwrPolPowerDownIoStopped
        | PwrPolWakeSuccess
        | PwrPolWakeFailed
        | PwrPolPowerTimeoutExpired
        | PwrPolS0IdlePolicyChanged
        | PwrPolPowerDownFailed
      },
    },

    { FxPkgPnp::PowerPolStartedCancelTimer,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { NULL,
      { PwrPolPowerTimeoutExpired, WdfDevStatePwrPolStartingDecideS0Wake },
      FxPkgPnp::m_PowerPolStartedWaitForIdleTimeoutOtherStates,
      { TRUE,
        PwrPolStart
      },
    },

    { FxPkgPnp::PowerPolStartedWakeCapableCancelTimerForSleep,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { NULL,
      { PwrPolPowerTimeoutExpired, WdfDevStatePwrPolSleeping },
      NULL,
      { TRUE,
        PwrPolStart
      },
    },

    { FxPkgPnp::PowerPolStartedWakeCapableSleepingUsbSS,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolSleeping },
      NULL,
      { TRUE,
        PwrPolStart
      },
    },

    { FxPkgPnp::PowerPolStartedIdleCapableCancelTimerForSleep,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { NULL,
      { PwrPolPowerTimeoutExpired, WdfDevStatePwrPolSleeping },
      NULL,
      { TRUE,
        PwrPolStart
      },
    },

    { FxPkgPnp::PowerPolDeviceD0PowerRequestFailed,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolDevicePowerRequestFailed,
      { PwrPolStop, WdfDevStatePwrPolStoppingCancelTimer },
      FxPkgPnp::m_PowerPolDevicePowerRequestFailedOtherStates,
      { TRUE,
        PwrPolStart
        | PwrPolPowerDown
        | PwrPolWakeSuccess
        | PwrPolIoPresent
        | PwrPolPowerDownFailed
      },
    },

    { NULL,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { TRUE,
        PwrPolStart
      },
    },

    { NULL,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { TRUE,
        PwrPolStart
      },
    },

    { NULL,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { TRUE,
        PwrPolStart
      },
    },

    { NULL,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { TRUE,
        PwrPolStart
      },
    },

    { NULL,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { TRUE,
        PwrPolStart
      },
    },

    { NULL,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { TRUE,
        PwrPolStart
      },
    },

    { FxPkgPnp::PowerPolSleepingPowerDownNotProcessed,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolTimerExpiredWakeCapablePowerDownNotProcessed,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolTimerExpiredNoWakePowerDownNotProcessed,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolTimerExpiredNoWakePoweredDownDisableIdleTimer,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { NULL,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { NULL,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { NULL,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolPowerUpForSystemSleepNotSeen,
      { PwrPolNull, WdfDevStatePwrPolNull },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolWaitingArmedStoppingCancelUsbSS,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolStoppingCancelWake },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolWaitingArmedWakeFailedCancelUsbSS,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolIoPresentArmedWakeCanceled },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolWaitingArmedIoPresentCancelUsbSS,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolIoPresentArmed },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolWaitingArmedWakeSucceededCancelUsbSS,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolWokeFromS0 },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolCancelingUsbSSForSystemSleep,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolCancelingWakeForSystemSleep },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

    { FxPkgPnp::PowerPolStoppingD0CancelUsbSS,
      { PwrPolUsbSelectiveSuspendCompleted, WdfDevStatePwrPolStoppingD0 },
      NULL,
      { FALSE,
        PwrPolInvalid },
    },

};



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

    for (i = 0; i < PowerSystemMaximum; i++)
    {
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

    if (m_UsbIdle != NULL)
    {
        delete m_UsbIdle;
        m_UsbIdle = NULL;
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

    if (NT_SUCCESS(status))
    {
        m_PowerCallbackRegistration = Mx::RegisterCallback(
            m_PowerCallbackObject,
            _PowerStateCallback,
            this
            );

        if (m_PowerCallbackRegistration == NULL)
        {
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
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return status;
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
    if (m_PowerCallbackRegistration != NULL)
    {
        Mx::UnregisterCallback(m_PowerCallbackRegistration);
        m_PowerCallbackRegistration = NULL;
    }

    if (m_PowerCallbackObject != NULL)
    {
        Mx::MxDereferenceObject(m_PowerCallbackObject);
        m_PowerCallbackObject = NULL;
    }
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

    if (Argument1 != (PVOID) PO_CB_SYSTEM_STATE_LOCK)
    {
        return;
    }

    pThis->m_PkgPnp->m_PowerPolicyMachine.m_StateMachineLock.AcquireLock(
        pThis->m_PkgPnp->GetDriverGlobals(),
        NULL
        );

    if (Argument2 == (PVOID) 0)
    {
        //
        // Write out the state if necessary before we turn off the paging path.
        //
        pThis->m_PkgPnp->SaveState(TRUE);

        //
        // Exiting S0
        //
        pThis->m_CanSaveState = FALSE;
    }
    else if (Argument2 == (PVOID) 1)
    {
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

VOID
FxPkgPnp::PowerPolicyProcessEventInner(
    __inout FxPostProcessInfo* Info
    )
{
    WDF_DEVICE_POWER_POLICY_STATE newState;
    FxPowerPolicyEvent event;
    ULONG i;
    KIRQL irql;

    if (IsPowerPolicyOwner())
    {
        CPPOWER_POLICY_STATE_TABLE entry;

        //
        // Process as many events as we can.
        //
        for ( ; ; )
        {
            entry = GetPowerPolicyTableEntry(m_Device->GetDevicePowerPolicyState());

            //
            // Get an event from the queue.
            //
            m_PowerPolicyMachine.Lock(&irql);

            if (m_PowerPolicyMachine.IsEmpty())
            {
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
            if (event & PwrPolPriorityEventsMask)
            {
                //
                // These are always possible to handle.
                //
                DO_NOTHING();
            }
            else
            {
                //
                // Check to see if this state can handle new events, ie if this
                // is a green dot (queue open) or red dot (queue *not* open) state.
                //
                if (entry->StateInfo.Bits.QueueOpen == FALSE)
                {
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
            if (m_PowerPolicyMachine.m_SingularEventsPresent & event)
            {
               m_PowerPolicyMachine.m_SingularEventsPresent &= ~event;
            }

            m_PowerPolicyMachine.IncrementHead();
            m_PowerPolicyMachine.Unlock(irql);

            //
            // Find the entry in the power policy state table that corresponds
            // to this event.
            //
            newState = WdfDevStatePwrPolNull;

            if (entry->FirstTargetState.PowerPolicyEvent == event)
            {
                newState = entry->FirstTargetState.TargetState;

                DO_EVENT_TRAP(&entry->FirstTargetState);
            }
            else if (entry->OtherTargetStates != NULL)
            {
                for (i = 0;
                     entry->OtherTargetStates[i].PowerPolicyEvent != PwrPolNull;
                     i++)
                {
                    if (entry->OtherTargetStates[i].PowerPolicyEvent == event)
                    {
                        newState = entry->OtherTargetStates[i].TargetState;
                        DO_EVENT_TRAP(&entry->OtherTargetStates[i]);
                        break;
                    }
                }
            }

            if (newState == WdfDevStatePwrPolNull)
            {
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

                if ((entry->StateInfo.Bits.KnownDroppedEvents & event) == 0)
                {
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

                    // TODO: Uncomment when implement USB
                    //m_PowerPolicyMachine.m_Owner->m_UsbIdle->m_EventDropped = TRUE;
                    __debugbreak();
                    break;

                case PwrPolUsbSelectiveSuspendCallback:
                    // TODO: Uncomment when implement USB
                    //m_PowerPolicyMachine.UsbSSCallbackProcessingComplete();
                    __debugbreak();
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
            else
            {
                //
                // Now enter the new state.
                //
                PowerPolicyEnterNewState(newState);
            }
        }
    }
    else
    {
        //
        // Process as many events as we can.
        //
        for ( ; ; )
        {
            CPNOT_POWER_POLICY_OWNER_STATE_TABLE entry;

#pragma prefast(suppress:__WARNING_DEREF_NULL_PTR, "The current power policy state will always be in the table so entry will never be NULL")
            entry = GetNotPowerPolicyOwnerTableEntry(
                m_Device->GetDevicePowerPolicyState()
                );

            //
            // Get an event from the queue.
            //
            m_PowerPolicyMachine.Lock(&irql);

            if (m_PowerPolicyMachine.IsEmpty())
            {
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
            if (event & PwrPolNotOwnerPriorityEventsMask)
            {
                //
                // These are always possible to handle.
                //
                DO_NOTHING();
            }
            else
            {
                //
                // Check to see if this state can handle new events, ie if this
                // is a green dot (queue open) or red dot (queue *not* open) state.
                //
                if (entry->QueueOpen == FALSE)
                {
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
            if (m_PowerPolicyMachine.m_SingularEventsPresent & event)
            {
               m_PowerPolicyMachine.m_SingularEventsPresent &= ~event;
            }

            m_PowerPolicyMachine.IncrementHead();
            m_PowerPolicyMachine.Unlock(irql);

            if (entry != NULL && entry->TargetStatesCount > 0)
            {
                for (i = 0; i < entry->TargetStatesCount; i++)
                {
                    if (event == entry->TargetStates[i].PowerPolicyEvent)
                    {
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

PolicySettings::~PolicySettings()
{
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

    while (newState != WdfDevStatePwrPolNull)
    {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNPPOWERSTATES,
            "WDFDEVICE 0x%p !devobj 0x%p entering power policy state "
            "%!WDF_DEVICE_POWER_POLICY_STATE! from "
            "%!WDF_DEVICE_POWER_POLICY_STATE!", m_Device->GetHandle(),
            m_Device->GetDeviceObject(), newState, currentState);

        if (m_PowerPolicyStateCallbacks != NULL)
        {
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

        if (m_PowerPolicyStateCallbacks != NULL)
        {
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
        if (entry->StateFunc != NULL)
        {
            watchdog.StartTimer(currentState);
            newState = entry->StateFunc(this);
            watchdog.CancelTimer(currentState);

            //
            // Validate the return value if FX_STATE_MACHINE_VERIFY is enabled
            //
            VALIDATE_PWR_POL_STATE(currentState, newState);

        }
        else
        {
            newState = WdfDevStatePwrPolNull;
        }

        if (m_PowerPolicyStateCallbacks != NULL)
        {
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
    if (Event & PowerPolSingularEventMask)
    {
        if ((m_PowerPolicyMachine.m_SingularEventsPresent & Event) == 0x00)
        {
            m_PowerPolicyMachine.m_SingularEventsPresent |= Event;
        }
        else
        {
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

    if (m_PowerPolicyMachine.IsFull())
    {
        //
        // The queue is full.  Bail.
        //
        m_PowerPolicyMachine.Unlock(irql);

        ASSERT(!"The Power queue is full.  This shouldn't be able to happen.");
        return;
    }

    if (m_PowerPolicyMachine.IsClosedLocked())
    {
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

    if (Event & mask)
    {
        //
        // Stick it on the front of the queue, making it the next event that
        // will be processed if, otherwise let these events go by.
        //
        m_PowerPolicyMachine.m_Queue[m_PowerPolicyMachine.InsertAtHead()] = Event;
    }
    else
    {
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
                    ))
    {
        LONGLONG timeout = 0;

        status = m_PowerPolicyMachine.m_StateMachineLock.AcquireLock(
            GetDriverGlobals(), &timeout);

        if (FxWaitLockInternal::IsLockAcquired(status))
        {
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

BOOLEAN
FxPkgPnp::ShouldProcessPowerPolicyEventOnDifferentThread(
    __in KIRQL CurrentIrql,
    __in BOOLEAN CallerSpecifiedProcessingOnDifferentThread
    )
/*++
Routine Description:

    This function returns whether the power policy state machine should process
    the current event on the same thread or on a different one.

Arguemnts:

    CurrentIrql - The current IRQL
    
    CallerSpecifiedProcessingOnDifferentThread - Whether or not caller of 
        PowerPolicyProcessEvent specified that the event be processed on a 
        different thread.

Returns:
    TRUE if the power policy state machine should process the event on a 
       different thread.
       
    FALSE if the power policy state machine should process the event on the 
       same thread

--*/
{
    //
    // For KMDF, we ignore what the caller of PowerPolicyProcessEvent specified
    // (which should always be FALSE, BTW) and base our decision on the current
    // IRQL. If we are running at PASSIVE_LEVEL, we process on the same thread
    // else we queue a work item.
    //
    UNREFERENCED_PARAMETER(CallerSpecifiedProcessingOnDifferentThread);

    ASSERT(FALSE == CallerSpecifiedProcessingOnDifferentThread);

    return (CurrentIrql == PASSIVE_LEVEL) ? FALSE : TRUE;
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
    if (m_Owner != NULL)
    {
        delete m_Owner;
        m_Owner = NULL;
    }
}


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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStartingDecideS0Wake(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStartedIdleCapable(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredNoWake(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolS0NoWakePowerUp(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemSleepFromDeviceWaitingUnarmed(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemSleepNeedWake(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSleepingWakeRevertArmWake(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemAsleepWakeArmed(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeDeviceWakeEnabled(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeDeviceWakeEnabledWakeCanceled(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeDeviceWakeDisarm(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeDeviceWakeTriggeredS0(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeDeviceWokeDisarm(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSleepingWakeRevertArmWakeNP(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemAsleepWakeArmedNP(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeDeviceWakeEnabledNP(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeDeviceWakeEnabledWakeCanceledNP(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeDeviceWakeTriggeredS0NP(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSleepingNoWakeDxRequestFailed(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeDeviceToD0(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolSystemWakeQueryIdle(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStartedWakeCapable(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolTimerExpiredWakeCapableSendWake(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolIoPresentArmed(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolIoPresentArmedWakeCanceled(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolS0WakeDisarm(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolCancelingWakeForSystemSleep(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolCancelingWakeForSystemSleepWakeCanceled(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolWokeFromS0(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStoppingD0(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStoppingDisarmWake(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStoppingDisarmWakeCancelWake(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStoppingDisarmWakeWakeCanceled(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::PowerPolStopping(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
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
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

WDF_DEVICE_POWER_POLICY_STATE
FxPkgPnp::NotPowerPolOwnerGotoD0(
    __inout FxPkgPnp* This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePwrPolInvalid;
}

//

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