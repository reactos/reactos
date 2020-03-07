#include "common/fxpkgpnp.h"
#include "common/fxdevice.h"
#include "common/fxmacros.h"


//
// The PnP State Machine
//
// This state machine responds to several PnP events:
//
// AddDevice                    -- Always targets the same state
// IRP_MN_START_DEVICE
// IRP_MN_START_DEVICE Complete -- Handled on the way up the stack
// IRP_MN_QUERY_REMOVE_DEVICE
// IRP_MN_QUERY_STOP_DEVICE
// IRP_MN_CANCEL_REMOVE_DEVICE
// IRP_MN_CANCEL_STOP_DEVICE
// IRP_MN_STOP_DEVICE
// IRP_MN_REMOVE_DEVICE
// IRP_MN_SURPRISE_REMOVE_DEVICE -- Always targets the same state
// IRP_MN_EJECT
//
// Each state has an entry for each of these events, listing the
// target state for each of them.
//

#if FX_STATE_MACHINE_VERIFY
    #define VALIDATE_PNP_STATE(_CurrentState, _NewState)     \
        ValidatePnpStateEntryFunctionReturnValue((_CurrentState), (_NewState))
#else
    #define VALIDATE_PNP_STATE(_CurrentState, _NewState)   (0)
#endif  //FX_STATE_MACHINE_VERIFY


const PNP_EVENT_TARGET_STATE FxPkgPnp::m_PnpInitOtherStates[] =
{
    { PnpEventQueryRemove, WdfDevStatePnpInitQueryRemove DEBUGGED_EVENT },
    { PnpEventRemove, WdfDevStatePnpRemoved DEBUGGED_EVENT },
    { PnpEventParentRemoved, WdfDevStatePnpRemoved DEBUGGED_EVENT },
    { PnpEventSurpriseRemove, WdfDevStatePnpInitSurpriseRemoved DEBUGGED_EVENT },
    { PnpEventEject, WdfDevStatePnpEjectHardware DEBUGGED_EVENT },
    { PnpEventNull, WdfDevStatePnpNull },
};

const PNP_EVENT_TARGET_STATE FxPkgPnp::m_PnpInitStartingOtherStates[] =
{
    { PnpEventStartDeviceFailed, WdfDevStatePnpInit DEBUGGED_EVENT },
    { PnpEventNull, WdfDevStatePnpNull },
};

const PNP_EVENT_TARGET_STATE FxPkgPnp::m_PnpHardwareAvailableOtherStates[] =
{
    { PnpEventPwrPolStartFailed, WdfDevStatePnpHardwareAvailablePowerPolicyFailed DEBUGGED_EVENT },
    { PnpEventNull, WdfDevStatePnpNull },
};

const PNP_EVENT_TARGET_STATE FxPkgPnp::m_PnpQueryStopPendingOtherStates[] =
{
    { PnpEventCancelStop, WdfDevStatePnpQueryCanceled DEBUGGED_EVENT },
    { PnpEventSurpriseRemove, WdfDevStatePnpQueriedSurpriseRemove TRAP_ON_EVENT },
    { PnpEventNull, WdfDevStatePnpNull },
};

const PNP_EVENT_TARGET_STATE FxPkgPnp::m_PnpRemovedPdoWaitOtherStates[] =
{
    { PnpEventStartDevice, WdfDevStatePnpPdoRestart DEBUGGED_EVENT },
    { PnpEventRemove, WdfDevStatePnpCheckForDevicePresence DEBUGGED_EVENT },
    { PnpEventParentRemoved, WdfDevStatePnpPdoRemoved DEBUGGED_EVENT },
    { PnpEventSurpriseRemove, WdfDevStatePnpRemovedPdoSurpriseRemoved DEBUGGED_EVENT },
    { PnpEventNull, WdfDevStatePnpNull },
};

const PNP_EVENT_TARGET_STATE FxPkgPnp::m_PnpRestartingOtherStates[] =
{
    { PnpEventPwrPolStartFailed, WdfDevStatePnpFailedOwnHardware DEBUGGED_EVENT },
    { PnpEventNull, WdfDevStatePnpNull },
};

const PNP_EVENT_TARGET_STATE FxPkgPnp::m_PnpStartedOtherStates[] =
{
    { PnpEventQueryStop, WdfDevStatePnpQueryStopStaticCheck DEBUGGED_EVENT },
    { PnpEventCancelStop, WdfDevStatePnpStartedCancelStop DEBUGGED_EVENT },
    { PnpEventCancelRemove, WdfDevStatePnpStartedCancelRemove DEBUGGED_EVENT },
    { PnpEventRemove, WdfDevStatePnpStartedRemoving DEBUGGED_EVENT },
    { PnpEventSurpriseRemove, WdfDevStatePnpSurpriseRemoveIoStarted DEBUGGED_EVENT },
    { PnpEventPowerUpFailed,   WdfDevStatePnpFailedIoStarting DEBUGGED_EVENT },
    { PnpEventPowerDownFailed, WdfDevStatePnpFailedPowerDown DEBUGGED_EVENT },
    { PnpEventStartDevice, WdfDevStatePnpRestart DEBUGGED_EVENT },
    { PnpEventNull, WdfDevStatePnpNull },
};

const PNP_EVENT_TARGET_STATE FxPkgPnp::m_PnpQueryRemovePendingOtherStates[] =
{
    { PnpEventCancelRemove, WdfDevStatePnpQueryCanceled DEBUGGED_EVENT },
    { PnpEventSurpriseRemove, WdfDevStatePnpQueriedSurpriseRemove TRAP_ON_EVENT },
    { PnpEventNull, WdfDevStatePnpNull },
};

const PNP_EVENT_TARGET_STATE FxPkgPnp::m_PnpQueriedRemovingOtherStates[] =
{
    { PnpEventPwrPolStopFailed, WdfDevStatePnpRemovingDisableInterfaces DEBUGGED_EVENT },
    { PnpEventNull, WdfDevStatePnpNull },
};

const PNP_EVENT_TARGET_STATE FxPkgPnp::m_PnpInitQueryRemoveOtherStates[] =
{
    { PnpEventCancelRemove, WdfDevStatePnpInitQueryRemoveCanceled DEBUGGED_EVENT },
    { PnpEventNull, WdfDevStatePnpNull },
};

const PNP_EVENT_TARGET_STATE FxPkgPnp::m_PnpStoppedOtherStates[] =
{
    { PnpEventSurpriseRemove, WdfDevStatePnpSurpriseRemove DEBUGGED_EVENT },
    { PnpEventNull, WdfDevStatePnpNull },
};

const PNP_EVENT_TARGET_STATE FxPkgPnp::m_PnpStoppedWaitForStartCompletionOtherStates[] =
{
    { PnpEventStartDeviceFailed, WdfDevStatePnpFailed TRAP_ON_EVENT },
    { PnpEventNull, WdfDevStatePnpNull },
};

const PNP_EVENT_TARGET_STATE FxPkgPnp::m_PnpStartedStoppingOtherStates[] =
{
    { PnpEventPwrPolStopFailed, WdfDevStatePnpFailedOwnHardware DEBUGGED_EVENT },
    { PnpEventNull, WdfDevStatePnpNull },
};

const PNP_EVENT_TARGET_STATE FxPkgPnp::m_PnpStartedStoppingFailedOtherStates[] =
{
    { PnpEventPwrPolStopFailed, WdfDevStatePnpFailedOwnHardware TRAP_ON_EVENT },
    { PnpEventNull, WdfDevStatePnpNull },
};

const PNP_EVENT_TARGET_STATE FxPkgPnp::m_PnpEjectFailedOtherStates[] =
{
    { PnpEventSurpriseRemove, WdfDevStatePnpSurpriseRemove TRAP_ON_EVENT },
    { PnpEventNull, WdfDevStatePnpNull },
};

const PNP_EVENT_TARGET_STATE FxPkgPnp::m_PnpStartedRemovingOtherStates[] =
{
    { PnpEventPwrPolStopFailed, WdfDevStatePnpRemovingDisableInterfaces DEBUGGED_EVENT },
    { PnpEventNull, WdfDevStatePnpNull },
};

const PNP_EVENT_TARGET_STATE FxPkgPnp::m_PnpFailedPowerDownOtherStates[] =
{
    { PnpEventPwrPolStopFailed, WdfDevStatePnpFailedOwnHardware DEBUGGED_EVENT },
    { PnpEventNull, WdfDevStatePnpNull },
};

const PNP_EVENT_TARGET_STATE FxPkgPnp::m_PnpFailedIoStartingOtherStates[] =
{
    { PnpEventPwrPolStopFailed, WdfDevStatePnpFailedOwnHardware DEBUGGED_EVENT },
    { PnpEventNull, WdfDevStatePnpNull },
};

const PNP_EVENT_TARGET_STATE FxPkgPnp::m_PnpFailedWaitForRemoveOtherStates[] =
{
    { PnpEventSurpriseRemove, WdfDevStatePnpFailedSurpriseRemoved DEBUGGED_EVENT },
    { PnpEventStartDevice, WdfDevStatePnpFailedStarted TRAP_ON_EVENT },
    { PnpEventNull, WdfDevStatePnpNull },
};

const PNP_EVENT_TARGET_STATE FxPkgPnp::m_PnpRestartOtherStates[] =
{
    { PnpEventPwrPolStopFailed, WdfDevStatePnpHardwareAvailablePowerPolicyFailed DEBUGGED_EVENT },
    { PnpEventNull, WdfDevStatePnpNull },
};

const PNP_EVENT_TARGET_STATE FxPkgPnp::m_PnpRestartReleaseHardware[] =
{
    { PnpEventStartDeviceFailed, WdfDevStatePnpFailed TRAP_ON_EVENT },
    { PnpEventNull, WdfDevStatePnpNull },
};

const PNP_EVENT_TARGET_STATE FxPkgPnp::m_PnpRestartHardwareAvailableOtherStates[] =
{
    { PnpEventPwrPolStartFailed, WdfDevStatePnpHardwareAvailablePowerPolicyFailed DEBUGGED_EVENT },
    { PnpEventNull, WdfDevStatePnpNull },
};

const PNP_STATE_TABLE FxPkgPnp::m_WdfPnpStates[] = {
    // State function
    // First transition event & state
    // Other transition events & states
    // state info

    //      WdfDevStatePnpObjectCreated
    {   NULL,
        { PnpEventAddDevice, WdfDevStatePnpInit DEBUGGED_EVENT },
        NULL,
        { TRUE,
          0 },
    },

    //      WdfDevStatePnpCheckForDevicePresence
    {   FxPkgPnp::PnpEventCheckForDevicePresence,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpEjectFailed
    {   NULL,
        { PnpEventStartDevice, WdfDevStatePnpPdoRestart DEBUGGED_EVENT },
        FxPkgPnp::m_PnpEjectFailedOtherStates,
        { TRUE,
          0 },
    },

    //      WdfDevStatePnpEjectHardware
    {   FxPkgPnp::PnpEventEjectHardware,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpEjectedWaitingForRemove
    {   NULL,
        { PnpEventRemove, WdfDevStatePnpPdoRemoved DEBUGGED_EVENT },
        NULL,
        { TRUE,
          PnpEventSurpriseRemove },  // can receive this if parent is surprise
                                     // removed while the ejected pdo is waiting
                                     // for remove.
    },

    //      WdfDevStatePnpInit
    {   NULL,
        { PnpEventStartDevice, WdfDevStatePnpInitStarting DEBUGGED_EVENT },
        FxPkgPnp::m_PnpInitOtherStates,
        { TRUE,
          PnpEventStartDevice },
    },

    //      WdfDevStatePnpInitStarting
    {   FxPkgPnp::PnpEventInitStarting,
        { PnpEventStartDeviceComplete, WdfDevStatePnpHardwareAvailable DEBUGGED_EVENT },
        FxPkgPnp::m_PnpInitStartingOtherStates,
        { TRUE,
          0 },
    },

    //      WdfDevStatePnpInitSurpriseRemoved
    {   FxPkgPnp::PnpEventInitSurpriseRemoved,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpHardwareAvailable
    {   FxPkgPnp::PnpEventHardwareAvailable,
        { PnpEventPwrPolStarted, WdfDevStatePnpEnableInterfaces DEBUGGED_EVENT },
        FxPkgPnp::m_PnpHardwareAvailableOtherStates,
        { FALSE,
          PnpEventPowerUpFailed
        },
    },

    //      WdfDevStatePnpEnableInterfaces
    {   FxPkgPnp::PnpEventEnableInterfaces,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpHardwareAvailablePowerPolicyFailed
    {   FxPkgPnp::PnpEventHardwareAvailablePowerPolicyFailed,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpQueryRemoveAskDriver
    {   FxPkgPnp::PnpEventQueryRemoveAskDriver,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpQueryRemovePending
    {   FxPkgPnp::PnpEventQueryRemovePending,
        { PnpEventRemove, WdfDevStatePnpQueriedRemoving DEBUGGED_EVENT },
        FxPkgPnp::m_PnpQueryRemovePendingOtherStates,
        { TRUE,
          0,
        },
    },

    //      WdfDevStatePnpQueryRemoveStaticCheck
    {   FxPkgPnp::PnpEventQueryRemoveStaticCheck,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpQueriedRemoving,
    {   FxPkgPnp::PnpEventQueriedRemoving,
        { PnpEventPwrPolStopped, WdfDevStatePnpRemovingDisableInterfaces DEBUGGED_EVENT },
        FxPkgPnp::m_PnpQueriedRemovingOtherStates,
        { FALSE,
          PnpEventPowerDownFailed | // We ignore these power failed events because
          PnpEventPowerUpFailed     // they will be translated into failed power
                                    // policy events.
        },
    },

    //      WdfDevStatePnpQueryStopAskDriver
    {   FxPkgPnp::PnpEventQueryStopAskDriver,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpQueryStopPending
    {   FxPkgPnp::PnpEventQueryStopPending,
        { PnpEventStop, WdfDevStatePnpStartedStopping DEBUGGED_EVENT },
        FxPkgPnp::m_PnpQueryStopPendingOtherStates,
        { TRUE,
          0,
        },
    },

    //      WdfDevStatePnpQueryStopStaticCheck
    {   FxPkgPnp::PnpEventQueryStopStaticCheck,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpQueryCanceled,
    {   FxPkgPnp::PnpEventQueryCanceled,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpRemoved
    {   FxPkgPnp::PnpEventRemoved,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpPdoRemoved
    {   FxPkgPnp::PnpEventPdoRemoved,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpRemovedPdoWait
    {   FxPkgPnp::PnpEventRemovedPdoWait,
        { PnpEventEject, WdfDevStatePnpEjectHardware DEBUGGED_EVENT },
        FxPkgPnp::m_PnpRemovedPdoWaitOtherStates,
        { TRUE,
          PnpEventCancelRemove | // Amazingly enough, you can get a cancel q.r.
                                 // on a PDO without seeing the query remove if
                                 // the stack is partially built
          PnpEventQueryRemove  | // Can get a query remove from the removed state
                                 // when installing a PDO that is disabled
          PnpEventPowerDownFailed // We may get this for a PDO if implicit power
                                  // down callbacks were failed. The failed power
                                  // policy stop event took care of rundown.
        },
    },

    //      WdfDevStatePnpRemovedPdoSurpriseRemoved
    {   FxPkgPnp::PnpEventRemovedPdoSurpriseRemoved,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpRemovingDisableInterfaces
    {   FxPkgPnp::PnpEventRemovingDisableInterfaces,
        { PnpEventPwrPolRemoved, WdfDevStatePnpRemoved DEBUGGED_EVENT },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpRestarting
    {   FxPkgPnp::PnpEventRestarting,
        { PnpEventPwrPolStarted, WdfDevStatePnpStarted DEBUGGED_EVENT },
        FxPkgPnp::m_PnpRestartingOtherStates,
        { FALSE,
          PnpEventPowerUpFailed
        },
    },

    //      WdfDevStatePnpStarted
    {   FxPkgPnp::PnpEventStarted,
        { PnpEventQueryRemove, WdfDevStatePnpQueryRemoveStaticCheck DEBUGGED_EVENT },
        FxPkgPnp::m_PnpStartedOtherStates,
        { TRUE,
          0,
        },
    },

    //      WdfDevStatePnpStartedCancelStop
    {   FxPkgPnp::PnpEventStartedCancelStop,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpStartedCancelRemove
    {   FxPkgPnp::PnpEventStartedCancelRemove,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpStartedRemoving
    {   FxPkgPnp::PnpEventStartedRemoving,
        { PnpEventPwrPolStopped, WdfDevStatePnpRemovingDisableInterfaces DEBUGGED_EVENT },
        FxPkgPnp::m_PnpStartedRemovingOtherStates,
        { TRUE,
          PnpEventPowerUpFailed | // device was idled out and in Dx when we got removed
                                  // and this event is due to the power up that occured
                                  // to move it into D0 so it could be disarmed
          PnpEventPowerDownFailed
        },
    },

    //      WdfDevStatePnpStartingFromStopped
    {   FxPkgPnp::PnpEventStartingFromStopped,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpStopped
    {   FxPkgPnp::PnpEventStopped,
        { PnpEventStartDevice, WdfDevStatePnpStoppedWaitForStartCompletion DEBUGGED_EVENT },
        FxPkgPnp::m_PnpStoppedOtherStates,
        { TRUE,
          0,
        },
    },

    //      WdfDevStatePnpStoppedWaitForStartCompletion
    {   FxPkgPnp::PnpEventStoppedWaitForStartCompletion,
        { PnpEventStartDeviceComplete, WdfDevStatePnpStartingFromStopped DEBUGGED_EVENT },
        FxPkgPnp::m_PnpStoppedWaitForStartCompletionOtherStates,
        { TRUE,
          0 },
    },

    //      WdfDevStatePnpStartedStopping
    {   FxPkgPnp::PnpEventStartedStopping,
        { PnpEventPwrPolStopped, WdfDevStatePnpStopped DEBUGGED_EVENT },
        FxPkgPnp::m_PnpStartedStoppingOtherStates,
        { TRUE,
          PnpEventPowerUpFailed | // device was idled out and in Dx when we got stopped
                                  // and this event is due to the power up that occured
                                  // to move it into D0 so it could be disarmed
          PnpEventPowerDownFailed
        },
    },

    // The function is named PnpEventSurpriseRemoved with a 'd' because
    // PnpEventSurpriseRemove (no 'd') is an event name

    //      WdfDevStatePnpSurpriseRemove
    {   FxPkgPnp::PnpEventSurpriseRemoved,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpInitQueryRemove
    {   FxPkgPnp::PnpEventInitQueryRemove,
        { PnpEventRemove, WdfDevStatePnpRemoved DEBUGGED_EVENT },
        FxPkgPnp::m_PnpInitQueryRemoveOtherStates,
        { TRUE,
          0 },
    },

    //      WdfDevStatePnpInitQueryRemoveCanceled
    {   FxPkgPnp::PnpEventInitQueryRemoveCanceled,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { TRUE,
          0 },
    },

    //      WdfDevStatePnpFdoRemoved
    {   FxPkgPnp::PnpEventFdoRemoved,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpRemovedWaitForChildren
    {   NULL,
        { PnpEventChildrenRemovalComplete, WdfDevStatePnpRemovedChildrenRemoved DEBUGGED_EVENT },
        NULL,
        { TRUE,
          PnpEventPowerDownFailed  // device power down even from processing remove
        },
    },

    //      WdfDevStatePnpQueriedSurpriseRemove
    {   FxPkgPnp::PnpEventQueriedSurpriseRemove,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpSurpriseRemoveIoStarted
    {   FxPkgPnp::PnpEventSurpriseRemoveIoStarted,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpFailedPowerDown
    {   FxPkgPnp::PnpEventFailedPowerDown,
        { PnpEventPwrPolStopped, WdfDevStatePnpFailedOwnHardware DEBUGGED_EVENT },
        FxPkgPnp::m_PnpFailedPowerDownOtherStates,
        { FALSE,
          PnpEventPowerDownFailed ,
        },
    },

    //      WdfDevStatePnpFailedIoStarting
    {   FxPkgPnp::PnpEventFailedIoStarting,
        { PnpEventPwrPolStopped, WdfDevStatePnpFailedOwnHardware DEBUGGED_EVENT },
        FxPkgPnp::m_PnpFailedIoStartingOtherStates,
        { FALSE,
          PnpEventPowerDownFailed |

          PnpEventPowerUpFailed   // if the device idled out and then failed
                                  // d0 entry, the power up failed can be passed
                                  // up by the IoInvalidateDeviceRelations and
                                  // subsequence surprise remove event.
        },
    },

    //      WdfDevStatePnpFailedOwnHardware
    {   FxPkgPnp::PnpEventFailedOwnHardware,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpFailed
    {   FxPkgPnp::PnpEventFailed,
        //
        // NOTE: WdfDevStatePnpFailedPowerPolicyRemoved not in WDF 1.9 ???
        //
        //{ PnpEventPwrPolRemoved, WdfDevStatePnpFailedPowerPolicyRemoved DEBUGGED_EVENT },
        { PnpEventPwrPolRemoved, WdfDevStatePnpNull DEBUGGED_EVENT },
        NULL,
        { FALSE,
          0,
        },
    },

    //      WdfDevStatePnpFailedSurpriseRemoved
    {   FxPkgPnp::PnpEventFailedSurpriseRemoved,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0, },
    },

    //      WdfDevStatePnpFailedStarted
    {   FxPkgPnp::PnpEventFailedStarted,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0, },
    },

    //      WdfDevStatePnpFailedWaitForRemove,
    {   NULL,
        { PnpEventRemove, WdfDevStatePnpRemoved DEBUGGED_EVENT },
        FxPkgPnp::m_PnpFailedWaitForRemoveOtherStates,
        { TRUE,
          PnpEventPowerUpFailed | // initial power up failed, power policy start
                                  // failed event moved the state machine to the
                                  // failed state first
          PnpEventPowerDownFailed | // implicitD3 power down failed
          PnpEventQueryRemove |  // start succeeded, but we still get a query in
                                 // the removed case
          PnpEventCancelStop |   // power down failure while processing query stop
                                 // and q.s. irp completed with error
          PnpEventCancelRemove   // power down failure while processing query remove
                                 // and q.r. irp completed with error
        },
    },

    //      WdfDevStatePnpFailedInit
    {   FxPkgPnp::PnpEventFailedInit,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpPdoInitFailed
    {   FxPkgPnp::PnpEventPdoInitFailed,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpRestart
    {   FxPkgPnp::PnpEventRestart,
        { PnpEventPwrPolStopped, WdfDevStatePnpRestartReleaseHardware DEBUGGED_EVENT },
        FxPkgPnp::m_PnpRestartOtherStates,
        { FALSE,
          PnpEventPowerUpFailed | // when stopping power policy, device was in
                                  // Dx and bringing it to D0 succeeded or failed
          PnpEventPowerDownFailed // same as power up
        },
    },

    //      WdfDevStatePnpRestartReleaseHardware
    {   FxPkgPnp::PnpEventRestartReleaseHardware,
        { PnpEventStartDeviceComplete, WdfDevStatePnpRestartHardwareAvailable DEBUGGED_EVENT },
        FxPkgPnp::m_PnpRestartReleaseHardware,
        { TRUE,
          PnpEventPowerDownFailed // the previous pwr policy stop
                                  // in WdfDevStaePnpRestart will
                                  // cause these events to show up here
        },
    },

    //      WdfDevStatePnpRestartHardwareAvailable
    {   FxPkgPnp::PnpEventRestartHardwareAvailable,
        { PnpEventPwrPolStarted, WdfDevStatePnpStarted DEBUGGED_EVENT },
        FxPkgPnp::m_PnpRestartHardwareAvailableOtherStates,
        { TRUE,
          PnpEventPowerUpFailed
        },
    },

    //      WdfDevStatePnpPdoRestart
    {   FxPkgPnp::PnpEventPdoRestart,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpFinal
    {   FxPkgPnp::PnpEventFinal,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { TRUE,
          PnpEventPowerDownFailed, // on the final implicit power down, a
                                   // callback returned !NT_SUCCESS
        },
    },

    //      WdfDevStatePnpRemovedChildrenRemoved
    {   FxPkgPnp::PnpEventRemovedChildrenRemoved,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { TRUE,
          0 } ,
    },

    //      WdfDevStatePnpQueryRemoveEnsureDeviceAwake
    {   FxPkgPnp::PnpEventQueryRemoveEnsureDeviceAwake,
        { PnpEventDeviceInD0, WdfDevStatePnpQueryRemovePending DEBUGGED_EVENT },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpQueryStopEnsureDeviceAwake
    {   FxPkgPnp::PnpEventQueryStopEnsureDeviceAwake,
        { PnpEventDeviceInD0, WdfDevStatePnpQueryStopPending DEBUGGED_EVENT },
        NULL,
        { FALSE,
          0 },
    },

    //      WdfDevStatePnpFailedPowerPolicyRemoved
    {   FxPkgPnp::PnpEventFailedPowerPolicyRemoved,
        { PnpEventNull, WdfDevStatePnpNull },
        NULL,
        { FALSE,
          0 } ,
    },
};


VOID
FxPkgPnp::PnpProcessEvent(
    __in FxPnpEvent   Event,
    __in BOOLEAN      ProcessOnDifferentThread
    )
/*++

Routine Description:
    This function implements steps 1-8 of the algorithm described above.

Arguments:
    Event - Current PnP event

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
    KIRQL oldIrql;

    //
    // Take the lock, raising to DISPATCH_LEVEL.
    //
    m_PnpMachine.Lock(&oldIrql);

    if (m_PnpMachine.IsFull())
    {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
            "WDFDEVICE 0x%p !devobj 0x%p current pnp state %!WDF_DEVICE_PNP_STATE! "
            "dropping event %!FxPnpEvent! because of a full queue",
            m_Device->GetHandle(),
            m_Device->GetDeviceObject(),
            m_Device->GetDevicePnpState(),
            Event);

        //
        // The queue is full.  Bail.
        //
        m_PnpMachine.Unlock(oldIrql);

        ASSERT(!"The PnP queue is full.  This shouldn't be able to happen.");
        return;
    }

    if (m_PnpMachine.IsClosedLocked())
    {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
            "WDFDEVICE 0x%p !devobj 0x%p current pnp state %!WDF_DEVICE_PNP_STATE! "
            "dropping event %!FxPnpEvent! because of a closed queue",
            m_Device->GetHandle(),
            m_Device->GetDeviceObject(),
            m_Device->GetDevicePnpState(),
            Event);

        //
        // The queue is closed.  Bail
        //
        m_PnpMachine.Unlock(oldIrql);

        return;
    }

    //
    // Enqueue the event.  Whether the event goes on the front
    // or the end of the queue depends on which event it is.
    //
    if (Event & PnpPriorityEventsMask)
    {
        //
        // Stick it on the front of the queue, making it the next
        // event that will be processed.
        //
        m_PnpMachine.m_Queue[m_PnpMachine.InsertAtHead()] = Event;
    }
    else
    {
        //
        // Stick it on the end of the queue.
        //
        m_PnpMachine.m_Queue[m_PnpMachine.InsertAtTail()] = Event;
    }

    //
    // Drop the lock.
    //
    m_PnpMachine.Unlock(oldIrql);

    //
    // Now, if we are running at PASSIVE_LEVEL, attempt to run the state
    // machine on this thread.  If we can't do that, then queue a work item.
    //

    if (FALSE == ShouldProcessPnpEventOnDifferentThread(
                    oldIrql,
                    ProcessOnDifferentThread
                    ))
    {

        LONGLONG timeout = 0;

        status = m_PnpMachine.m_StateMachineLock.AcquireLock(GetDriverGlobals(),
                                                             &timeout);

        if (FxWaitLockInternal::IsLockAcquired(status))
        {
            FxPostProcessInfo info;

            //
            // We now hold the state machine lock.  So call the function that
            // dispatches the next state.
            //
            PnpProcessEventInner(&info);

            m_PnpMachine.m_StateMachineLock.ReleaseLock(GetDriverGlobals());

            info.Evaluate(this);
            return;
        }
    }

    //
    // For one reason or another, we couldn't run the state machine on this
    // thread.  So queue a work item to do it.  If m_PnPWorkItemEnqueuing
    // is non-zero, that means that the work item is already being enqueued
    // on another thread.  This is significant, since it means that we can't do
    // anything with the work item on this thread, but it's okay, since the
    // work item will pick up our work and do it.
    //
    m_PnpMachine.QueueToThread();
}

BOOLEAN
FxPkgPnp::ShouldProcessPnpEventOnDifferentThread(
    __in KIRQL CurrentIrql,
    __in BOOLEAN CallerSpecifiedProcessingOnDifferentThread
    )
/*++
Routine Description:

    This function returns whether the PnP state machine should process the 
    current event on the same thread or on a different one.

Arguemnts:

    CurrentIrql - The current IRQL
    
    CallerSpecifiedProcessingOnDifferentThread - Whether or not caller of 
        PnpProcessEvent specified that the event be processed on a different 
        thread.

Returns:
    TRUE if the PnP state machine should process the event on a different
       thread.
       
    FALSE if the PnP state machine should process the event on the same thread

--*/
{
    //
    // For KMDF, we ignore what the caller of PnpProcessEvent specified (which
    // should always be FALSE, BTW) and base our decision on the current IRQL.
    // If we are running at PASSIVE_LEVEL, we process on the same thread else
    // we queue a work item.
    //
    UNREFERENCED_PARAMETER(CallerSpecifiedProcessingOnDifferentThread);

    ASSERT(FALSE == CallerSpecifiedProcessingOnDifferentThread);

    return (CurrentIrql == PASSIVE_LEVEL) ? FALSE : TRUE;
}

VOID
FxPkgPnp::_PnpProcessEventInner(
    __inout FxPkgPnp* This,
    __inout FxPostProcessInfo* Info,
    __in PVOID WorkerContext
    )
{
    UNREFERENCED_PARAMETER(WorkerContext);

    //
    // Take the state machine lock.
    //
    This->m_PnpMachine.m_StateMachineLock.AcquireLock(
        This->GetDriverGlobals()
        );

    //
    // Call the function that will actually run the state machine.
    //
    This->PnpProcessEventInner(Info);

    //
    // We are being called from the work item and m_WorkItemRunning is > 0, so
    // we cannot be deleted yet.
    //
    ASSERT(Info->SomethingToDo() == FALSE);

    //
    // Now release the lock
    //
    This->m_PnpMachine.m_StateMachineLock.ReleaseLock(
        This->GetDriverGlobals()
        );
}

VOID
FxPkgPnp::PnpProcessEventInner(
    __inout FxPostProcessInfo* Info
    )
/*++

Routine Description:
    This routine runs the state machine.  It implements steps 10-15 of the
    algorithm described above.

--*/
{
    WDF_DEVICE_PNP_STATE newState;
    CPPNP_STATE_TABLE    entry;
    FxPnpEvent           event;
    KIRQL                oldIrql;

    //
    // Process as many events as we can.
    //
    for ( ; ; )
    {
        entry = GetPnpTableEntry(m_Device->GetDevicePnpState());

        //
        // Get an event from the queue.
        //
        m_PnpMachine.Lock(&oldIrql);

        if (m_PnpMachine.IsEmpty())
        {
            m_PnpMachine.GetFinishedState(Info);

            if (m_PnpMachine.m_FireAndForget)
            {
                m_PnpMachine.m_FireAndForget = FALSE;
                Info->m_FireAndForgetIrp = ClearPendingPnpIrp();

                ASSERT(Info->m_FireAndForgetIrp != NULL);
            }

            Info->m_SetRemovedEvent = m_SetDeviceRemoveProcessed;
            m_SetDeviceRemoveProcessed = FALSE;

            //
            // The queue is empty.
            //
            m_PnpMachine.Unlock(oldIrql);

            return;
        }

        event = m_PnpMachine.m_Queue[m_PnpMachine.GetHead()];

        //
        // At this point, we need to determine whether we can process this
        // event.
        //
        if (event & PnpPriorityEventsMask)
        {
            //
            // These are always possible to handle.
            //
            DO_NOTHING();
        }
        else
        {
            //
            // Check to see if this state can handle new events.
            //
            if (entry->StateInfo.Bits.QueueOpen == FALSE)
            {
                //
                // This state can't handle new events.
                //
                m_PnpMachine.Unlock(oldIrql);
                return;
            }
        }

        m_PnpMachine.IncrementHead();
        m_PnpMachine.Unlock(oldIrql);

        //
        // Find the entry in the PnP state table that corresponds
        // to this event.
        //
        newState = WdfDevStatePnpNull;

        if (entry->FirstTargetState.PnpEvent == event)
        {
            newState = entry->FirstTargetState.TargetState;

            DO_EVENT_TRAP(&entry->FirstTargetState);
        }
        else if (entry->OtherTargetStates != NULL)
        {
            ULONG i = 0;

            for (i = 0;
                 entry->OtherTargetStates[i].PnpEvent != PnpEventNull;
                 i++)
            {
                if (entry->OtherTargetStates[i].PnpEvent == event)
                {
                    newState = entry->OtherTargetStates[i].TargetState;
                    DO_EVENT_TRAP(&entry->OtherTargetStates[i]);
                    break;
                }
            }
        }

        if (newState == WdfDevStatePnpNull)
        {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                "WDFDEVICE 0x%p !devobj 0x%p current pnp state "
                "%!WDF_DEVICE_PNP_STATE! dropping event %!FxPnpEvent!",
                m_Device->GetHandle(),
                m_Device->GetDeviceObject(),
                m_Device->GetDevicePnpState(),
                event);

            if ((entry->StateInfo.Bits.KnownDroppedEvents & event) == 0)
            {
                COVERAGE_TRAP();

                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGPNP,
                    "WDFDEVICE 0x%p !devobj %p current state "
                    "%!WDF_DEVICE_PNP_STATE!, policy event %!FxPnpEvent! is not"
                    " a known dropped event, known dropped events are "
                    "%!FxPnpEvent!",
                    m_Device->GetHandle(),
                    m_Device->GetDeviceObject(),
                    m_Device->GetDevicePnpState(),
                    event,
                    entry->StateInfo.Bits.KnownDroppedEvents);

                // DIAG:  add diag code here
            }

            //
            // This state doesn't respond to the Event.  Make sure we do not
            // drop an event which pends a pnp irp on the floor though.
            //
            if (event & PnpEventPending)
            {
                //
                // In the case of a previous power up/down failure, the following
                // can happen
                // 1  invalidate device relations
                // 2  failure event is posted to pnp state machine
                // 3  process power failure event first, but while processing,
                //    query device state is completed, and failed /removed is reported
                // 4  surprise remove comes, the irp is queued, the event is queued
                // 5  processing of power failure event continues, gets to
                //    Failed and completes the s.r irp
                // 6  the surprise remove event is processed (the current value
                //    of the local var event), but since we are already in the
                //    Failed state, we ignore this event and end up
                //    here.
                //
                // This means that if we are processing surprise remove, we cannot
                // 100% expect that an irp has been pended.
                //
                PnpFinishProcessingIrp(
                    (event == PnpEventSurpriseRemove) ? FALSE : TRUE);
            }
            else
            {
                DO_NOTHING();
            }
        }
        else
        {
            //
            // Now enter the new state.
            //
            PnpEnterNewState(newState);
        }
    }
}

VOID
FxPkgPnp::PnpFinishProcessingIrp(
    __in BOOLEAN IrpMustBePresent
    )
/*++

Routine Description:
    Finishes handling a pended pnp irp

Arguments:
    None

Return Value:
    None

  --*/
{
    FxIrp irp;

    UNREFERENCED_PARAMETER(IrpMustBePresent);
    ASSERT(IrpMustBePresent == FALSE || IsPresentPendingPnpIrp());

    //
    // Start device is the only request we handle on the way back up the stack.
    // Also, if we fail any pnp irps that we are allowed to fail, we just
    // complete them.
    //
    if (IsPresentPendingPnpIrp())
    {
        irp.SetIrp(GetPendingPnpIrp());
        if (irp.GetMinorFunction() == IRP_MN_START_DEVICE
            ||
            !NT_SUCCESS(irp.GetStatus()))
        {
            irp.SetIrp(ClearPendingPnpIrp());
            CompletePnpRequest(&irp, irp.GetStatus());
        }
        else
        {
            m_PnpMachine.m_FireAndForget = TRUE;
        }
    }
}

VOID
FxPkgPnp::PnpEnterNewState(
    __in WDF_DEVICE_PNP_STATE State
    )
/*++

Routine Description:
    This function looks up the handler for a state and
    then calls it.

Arguments:
    Event - Current PnP event

Return Value:
    None.

--*/
{
    CPPNP_STATE_TABLE    entry;
    WDF_DEVICE_PNP_STATE currentState, newState;
    WDF_DEVICE_PNP_NOTIFICATION_DATA data;

    currentState = m_Device->GetDevicePnpState();
    newState = State;

    while (newState != WdfDevStatePnpNull)
    {
        DoTraceLevelMessage(
             GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNPPOWERSTATES,
             "WDFDEVICE 0x%p !devobj 0x%p entering PnP State "
             "%!WDF_DEVICE_PNP_STATE! from %!WDF_DEVICE_PNP_STATE!",
             m_Device->GetHandle(),
             m_Device->GetDeviceObject(),
             newState,
             currentState);

        if (m_PnpStateCallbacks != NULL)
        {
            //
            // Callback for leaving the old state
            //
            RtlZeroMemory(&data, sizeof(data));

            data.Type = StateNotificationLeaveState;
            data.Data.LeaveState.CurrentState = currentState;
            data.Data.LeaveState.NewState = newState;

            m_PnpStateCallbacks->Invoke(currentState,
                                        StateNotificationLeaveState,
                                        m_Device->GetHandle(),
                                        &data);
        }

        m_PnpMachine.m_States.History[m_PnpMachine.IncrementHistoryIndex()] =
            (USHORT) newState;

        if (m_PnpStateCallbacks != NULL)
        {
            //
            // Callback for entering the new state
            //
            RtlZeroMemory(&data, sizeof(data));

            data.Type = StateNotificationEnterState;
            data.Data.EnterState.CurrentState = currentState;
            data.Data.EnterState.NewState = newState;

            m_PnpStateCallbacks->Invoke(newState,
                                        StateNotificationEnterState,
                                        m_Device->GetHandle(),
                                        &data);
        }

        m_Device->SetDevicePnpState(newState);
        currentState = newState;

        entry = GetPnpTableEntry(currentState);

        //
        // Call the state handler if one is present and record our new state
        //
        if (entry->StateFunc != NULL)
        {
            newState = entry->StateFunc(this);

            //
            // Validate the return value if FX_STATE_MACHINE_VERIFY is enabled
            //
            VALIDATE_PNP_STATE(currentState, newState);
        }
        else
        {
            newState = WdfDevStatePnpNull;
        }

        if (m_PnpStateCallbacks != NULL)
        {
            //
            // Callback for post processing the new state
            //
            RtlZeroMemory(&data, sizeof(data));

            data.Type = StateNotificationPostProcessState;
            data.Data.PostProcessState.CurrentState = currentState;

            m_PnpStateCallbacks->Invoke(currentState,
                                        StateNotificationPostProcessState,
                                        m_Device->GetHandle(),
                                        &data);
        }
    }
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

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventCheckForDevicePresence(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Check For Device
    Presence state.  This is a state that is specific
    to PDOs, so this function should be overloaded by
    the PDO class and never called.

Arguments:
    none

Return Value:

    VOID

--*/
{
    return This->PnpEventCheckForDevicePresenceOverload();
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventEjectHardware(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Eject Hardware state.
    This is a state that is specific to PDOs, so this
    function should be overloaded by the PDO class
    and never called.

Arguments:
    none

Return Value:

    VOID

--*/
{
    return This->PnpEventEjectHardwareOverload();
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventInitStarting(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device is recieving a start for the first time.  The start is on the way
    down the stack.

Arguments:
    This - instance of the state machine

Return Value:
    new machine state

  --*/
{
    if (This->PnpSendStartDeviceDownTheStackOverload() == FALSE)
    {
        //
        // Start was sent asynchronously down the stack, the irp's completion
        // routine will move the state machine to the new state.
        //
        return WdfDevStatePnpNull;
    }

    return WdfDevStatePnpHardwareAvailable;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventInitSurpriseRemoved(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This transition should only occur for a PDO.

    The device was initialized, but then it's parent bus was surprise removed.
    Complete the surprise remove and wait for the remove.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePnpInit

  --*/
{
    This->PnpFinishProcessingIrp(TRUE);

    return WdfDevStatePnpInit;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventHardwareAvailable(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Hardware Available state.

Arguments:
    none

Return Value:

    VOID

--*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventStarted(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Completes the pending request or sends it on its way.

Arguments:
    This - instance of the state machine for the device

Return Value:
    WdfDevStatePnpNull

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventEnableInterfaces(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device has powered up and fully started, now enable the device
    interfaces and WMI.  We wait until the last possible moment because on
    win2k WMI registration is not synchronized with the completion of start
    device, so if we register WMI early in start device processing, we could get
    wmi requests while we are initializing, which is a race condition we want
    to eliminate.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventHardwareAvailablePowerPolicyFailed(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Our previous state called PowerPolicyStart or PowerPolicyStopRemove and the
    power state machine could not perform the requested action.  We still have
    a start irp pending, so set its status and then proceed down the start failure
    path.

Arguments:
    This - instance of the state machien

Return Value:
    WdfDevStatePnpFailedOwnHardware

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventQueryRemoveAskDriver(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Query Remove Ask Driver
    state.  It's job is to invoke EvtDeviceQueryRemove and then complete the
    QueryRemove IRP if needed.

Arguments:
    This - instance of the state machine

Return Value:
    new state

--*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventQueryRemovePending(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    The device has fully stopped.  Let go of the query remove irp.

Arguments:
    This - instance of the state machine for this device

Return Value:
    WdfDevStatePnpNull

  --*/

{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventQueryRemoveStaticCheck(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Query Remove Static Check
    state.  It's job is to determine whether this device,
    in general, can stop.  If it can, then we proceed on
    to Query Remove Ask Driver.  If not, then we go to
    back to Started.

Arguments:
    none

Return Value:

    VOID

--*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventQueriedRemoving(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    The device was query removed and is now in the removed state.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePnpNull

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventQueryStopAskDriver(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Query Stop Ask Driver
    state.  It's job is to invoke the EvtDeviceQueryStop
    callback and then complete the QueryStop IRP if needed.

Arguments:
    This - instance of the state machine

Return Value:
    new state

--*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventQueryStopPending(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Everything in the device has stopped due to the stop device irp.  Complete
    it now

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePnpNull

  --*/

{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventQueryStopStaticCheck(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Query Stop Static Check state.  It's job is to
    determine whether this device, in general, can stop.  If it can, then we
    proceed on to Query Stop Ask Driver.  Otherwise we will return to the
    Started state.  If the driver has set that the state machine should ignore
    query stop/remove, the query will succeed, otherwise it will fail

Arguments:
    This - instance of the state machine

Return Value:
   new machine state

--*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventQueryCanceled(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    The device was in the queried state (remove or stop) and the query was
    canceled.  Remove the power reference taken and return to the started state.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePnpStarted

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventRemoved(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Removed state.  It tears down any remaining
    children and the moves into a role (FDO/PDO) specific state.

Arguments:
    This - instance of the state machine

Return Value:
    new machine state

--*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventPdoRemoved(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the PDO Removed state.  This function is called
    when the PDO is actually reported missing to the OS or the FDO is removed.

Arguments:
    none

Return Value:
    new state

--*/
{
    return This->PnpEventPdoRemovedOverload();
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventRemovedPdoWait(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Indicates to the remove path that remove processing is done and we will
    wait for additional PNP events to arrive at this state machine

Arguments:
    This - Instance of the state machine

Return Value:
    WdfDevStatePnpNull

  --*/
{
    if (This->m_DeviceRemoveProcessed != NULL)
    {
        This->m_SetDeviceRemoveProcessed = TRUE;
    }

    return WdfDevStatePnpNull;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventRemovedPdoSurpriseRemoved(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    PDO has been removed (could have been disabled in user mode) and is now
    surprise removed by the underlying bus.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePnpRemovedPdoWait

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventRemovingDisableInterfaces(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Removing Disable
    Interfaces state.  It disables any device interfaces
    and then tells the power policy state machine to prepare for device removal.

Arguments:
    none

Return Value:

    WdfDevStatePnpNull

--*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventRestarting(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Cancelling Stop state.

Arguments:
    none

Return Value:

    VOID

--*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventStartedStopping(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Received a stop irp.  Stop the power policy machine and then wait for it to
    complete.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePnpNull

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventStartedCancelStop(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Cancel stop received from the started state.  Just return to the started
    state where we will handle the pended irp.

Arguments:
    This - Instance of the state machine

Return Value:
    WdfDevStatePnpStarted

  --*/
{
    UNREFERENCED_PARAMETER(This);
    return WdfDevStatePnpStarted;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventStartedCancelRemove(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Cancel remove received from the started state.  Just return to the started
    state where we will handle the pended irp.

Arguments:
    This - Instance of the state machine

Return Value:
    WdfDevStatePnpStarted

  --*/
{
    UNREFERENCED_PARAMETER(This);

    return WdfDevStatePnpStarted;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventStartedRemoving(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Remove directly from started.  Power down the other state machines.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePnpNull

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventStartingFromStopped(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Restarting From Stopped state.  It's job
    is to map new resources and then proceed to the Restarting state.

Arguments:
    none

Return Value:

    VOID

--*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventStopped(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Stopped state.  It's job is to invoke
    EvtDeviceReleaseHardware.

Arguments:
    none

Return Value:

    VOID

--*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventStoppedWaitForStartCompletion(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The start irp is coming down from the stopped state.  Send it down the stack
    and transition to the new state if needed.

Arguments:
    This -  instance of the state machine

Return Value:
    new machine state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventSurpriseRemoved(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    We got IRP_MN_SURPRISE_REMOVE_DEVICE while the system was pretty far down
    the removal path, or never started.  Call EvtDeviceSurpriseRemoval, call the
    surprise remove virtual and drop into the Failed path.

Arguments:
    This - instance of the state machine

Return Value:
    new device pnp state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventInitQueryRemove(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Query remove from the init state.  Complete the pended request.

Arguments:
    This - instance of th state machine.

Return Value:
    WdfDevStatePnpNull

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventInitQueryRemoveCanceled(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Handle a query remove canceled from the init state.  Complete the pended
    request.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePnpInit

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventFdoRemoved(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    FDO is being removed, hand off to the derived pnp package

Arguments:
    This - instance of the state machine

Return Value:
    new device pnp state

  --*/
{
    return This->PnpEventFdoRemovedOverload();
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventQueriedSurpriseRemove(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device was in a queried (either stop or cancel) state and was surprise
    removed from it.

Arguments:
    This - instance of the state machine

Return Value:
    new state, WdfDevStatePnpSurpriseRemoveIoStarted

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventSurpriseRemoveIoStarted(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    We got IRP_MN_SURPRISE_REMOVE_DEVICE while the system was more or less
    running.  Start down the Surprise Remove/Device Failed path.  This state
    calls EvtDeviceSurpriseRemoval, calls the virtual surprise remove overload,
     and then drops into the Failed path.

Arguments:
    This - instance of the state machine

Return Value:
    new device pnp state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventFailedPowerDown(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device was in the started state and failed power down or could not
    enable its interfaces.  Gracefully power down the device and tear down
    the stack.

    The difference between this routine and PnpEventFailedIoStarting is that
    FailedIoStarting sends a surprise remove to the power state machine.  After
    surprise remove has been sent to the power state machine, it will not attempt
    to put the device into Dx because it assumes the device is no longer present.
    In this error case, we still want the device to be powered down, so we send
    a normal stop remove.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventFailedIoStarting(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device failed (or was yanked out of the machine) while it was more
    or less running.  Tell the driver to stop self-managed I/O and drop into
    the next state on the failure path.

Arguments:                                    j
    This - instance of the state machine

Return Value:
    new device pnp state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventFailedOwnHardware(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device failed (or was yanked out of the machine) while it owned access
    to the hardware.  Tell the driver to release resources and drop into
    the next state on the failure path.

Arguments:
    This - instance of the state machine

Return Value:
    new device pnp state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventFailed(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
        The device failed (or was yanked out of the machine).  Disable interfaces,
        flush queues and tell the driver to clean up random stuff.
        Also ask the power policy state machine to prepare for device removal.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePnpNull

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventFailedSurpriseRemoved(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The device has failed and then received a surprise remove.  This can easily
    happen on return from low power as following:

    1  device attempts to enter D0, D0Entry fails
    2  pnp state machine proceeds down failure path and stops at FailedWaitForRemove
    3  bus driver finds device missing, reports it as such and a s.r. irp
       arrives

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePnpFailedWaitForRemove

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventFailedStarted(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Device failed (probably somewhere in the start path) and got another start
    request.  Fail the start and return to the state where we will wait for a
    remove irp.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePnpFailedWaitForRemove

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventFailedInit(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Processing of the start irp's resources failed.  Complete the start irp.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePnpNull

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventPdoInitFailed(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The driver failed EvtDeviceSoftwareInit.  Run cleanup and die.

Arguments:
    This - instance of the state machine

Return Value:
    new device pnp state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventRestart(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    A start to start transition has occurred.  Go through the normal stop path
    first and then restart things.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStateNull or new machine state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventRestartReleaseHardware(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Release the hardware resources and send the start irp down the stack

Arguments:
    This - instance of the state machine

Return Value:
    new machine state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventRestartHardwareAvailable(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    Prepare the hardware and restart the power and power  policy state machines
    after a successful start -> start transition where the start irp is coming
    up the stack.

Arguments:
    This - instance of the state machine

Return Value:
    new machine state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventPdoRestart(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The PDO was in the removed state has received another start irp.  Reset state
    and then move into the start sequence.

Arguments:
    This -  instance of the state machine

Return Value:
    WdfDevStatePnpHardwareAvailable

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventFinal(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The final resting and dead state for the FxDevice that has been removed.  We
    release our final reference here and destroy the object.

Arguments:
    This - This instance of the state machine

Return Value:
    WdfDevStatePnpNull

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventRemovedChildrenRemoved(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    All of this device's previously enumerated children have been removed.  Move
    the state machine into its next state based on the device's role.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    return This->PnpGetPostRemoveState();
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventQueryRemoveEnsureDeviceAwake(
   __inout FxPkgPnp *This
   )
/*++
Routine Description:
    This function brings the device to a working state if it is idle. If the
    device is already in working state, it ensures that it does not idle out.

Arguments:
    This - instance of the state machine

Return Value:
    new state
--*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventQueryStopEnsureDeviceAwake(
   __inout FxPkgPnp *This
   )
/*++
Routine Description:
    This function brings the device to a working state if it is idle. If the
    device is already in working state, it ensures that it does not idle out.

Arguments:
    This - instance of the state machine

Return Value:
    new state
--*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

WDF_DEVICE_PNP_STATE
FxPkgPnp::PnpEventFailedPowerPolicyRemoved(
    __inout FxPkgPnp* This
    )
/*++

Routine Description:
    The power policy state machine has prepared for device removal. Invalidate
    the device state and wait for IRP_MN_REMOVE_DEVICE.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePnpFailedWaitForRemove

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePnpInvalid;
}

VOID
FxPkgPnp::PnpEventSurpriseRemovePendingOverload(
    VOID
    )
{
    WDFNOTIMPLEMENTED();    
}

VOID
FxPkgPnp::PnpEventRemovedCommonCode(
    VOID
    )
/*++

Routine Description:
    This function implements the Removed state.

Arguments:
    none

Return Value:

    VOID

--*/
{
    WDFNOTIMPLEMENTED();
}
