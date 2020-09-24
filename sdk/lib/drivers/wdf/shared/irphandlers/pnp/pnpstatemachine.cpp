/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    PnpStateMachine.cpp

Abstract:

    This module implements the PnP state machine for the driver framework.
    This code was split out from FxPkgPnp.cpp.

Author:




Environment:

    Both kernel and user mode

Revision History:



--*/

#include "pnppriv.hpp"
#include <wdmguid.h>

#include<ntstrsafe.h>

extern "C" {
#if defined(EVENT_TRACING)
#include "PnpStateMachine.tmh"
#endif
}


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

// @@SMVERIFY_SPLIT_BEGIN

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
        { PnpEventPwrPolRemoved, WdfDevStatePnpFailedPowerPolicyRemoved DEBUGGED_EVENT },
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

// @@SMVERIFY_SPLIT_END

VOID
FxPkgPnp::PnpCheckAssumptions(
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
    WDFCASSERT(sizeof(FxPnpStateInfo) == sizeof(ULONG));

    WDFCASSERT((sizeof(m_WdfPnpStates)/sizeof(m_WdfPnpStates[0]))
               ==
               (WdfDevStatePnpNull - WdfDevStatePnpObjectCreated));

    // we assume these are the same length when we update the history index
    WDFCASSERT((sizeof(m_PnpMachine.m_Queue)/sizeof(m_PnpMachine.m_Queue[0]))
               ==
               (sizeof(m_PnpMachine.m_States.History)/
                sizeof(m_PnpMachine.m_States.History[0])));
}

/*++

The locking model for the PnP state machine requires that events be enqueued
possibly at DISPATCH_LEVEL.  It also requires that the PnP state machine be
runnable at PASSIVE_LEVEL.  Consequently, we have two locks, one DISPATCH_LEVEL
lock that guards the event queue and one PASSIVE_LEVEL lock that guards the
state machine itself.

Algorithm:

1)  Acquire the PnP queue lock.
2)  Enqueue the request.  Requests are put at the end of the queue, except if
    they are PowerUp, or PowerDown, in which case they are put at the head of
    the queue.
3)  Drop the PnP queue lock.
4)  If the thread is running at PASSIVE_LEVEL, skip to step 6.
5)  Queue a work item onto any work queue.
6)  Attempt to acquire the state machine lock, with a near-zero-length timeout.
7)  If successful, skip to step 10.
8)  Queue a work item onto any work queue.
9)  Acquire the state machine lock.
10) Acquire the PnP queue lock.
11) Attempt to dequeue an event.
12) Drop the PnP queue lock.
13) If there was no event to dequeue, drop the state machine lock and exit.
14) Execute the state handler.  This may involve taking one of the other state
    machine queue locks, briefly, to deliver an event.
15) Go to Step 10.

Implementing this algorithm requires three functions.

PnpProcessEvent         -- Implements steps 1-8.
_PnpProcessEventInner   -- Implements step 9.
PnpProcessEventInner    -- Implements steps 10-15.

--*/

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

    if (m_PnpMachine.IsFull()) {
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

    if (m_PnpMachine.IsClosedLocked()) {
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
    if (Event & PnpPriorityEventsMask) {
        //
        // Stick it on the front of the queue, making it the next
        // event that will be processed.
        //
        m_PnpMachine.m_Queue[m_PnpMachine.InsertAtHead()] = Event;
    }
    else {
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
                    )) {

        LONGLONG timeout = 0;

        status = m_PnpMachine.m_StateMachineLock.AcquireLock(GetDriverGlobals(),
                                                             &timeout);

        if (FxWaitLockInternal::IsLockAcquired(status)) {
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
    for ( ; ; ) {
        entry = GetPnpTableEntry(m_Device->GetDevicePnpState());

        //
        // Get an event from the queue.
        //
        m_PnpMachine.Lock(&oldIrql);

        if (m_PnpMachine.IsEmpty()) {
            m_PnpMachine.GetFinishedState(Info);

            if (m_PnpMachine.m_FireAndForget) {
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
        if (event & PnpPriorityEventsMask) {
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

        if (entry->FirstTargetState.PnpEvent == event) {
            newState = entry->FirstTargetState.TargetState;

            DO_EVENT_TRAP(&entry->FirstTargetState);
        }
        else if (entry->OtherTargetStates != NULL) {
            ULONG i = 0;

            for (i = 0;
                 entry->OtherTargetStates[i].PnpEvent != PnpEventNull;
                 i++) {
                if (entry->OtherTargetStates[i].PnpEvent == event) {
                    newState = entry->OtherTargetStates[i].TargetState;
                    DO_EVENT_TRAP(&entry->OtherTargetStates[i]);
                    break;
                }
            }
        }

        if (newState == WdfDevStatePnpNull) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                "WDFDEVICE 0x%p !devobj 0x%p current pnp state "
                "%!WDF_DEVICE_PNP_STATE! dropping event %!FxPnpEvent!",
                m_Device->GetHandle(),
                m_Device->GetDeviceObject(),
                m_Device->GetDevicePnpState(),
                event);

            if ((entry->StateInfo.Bits.KnownDroppedEvents & event) == 0) {
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
            if (event & PnpEventPending) {
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
            else {
                DO_NOTHING();
            }
        }
        else {
            //
            // Now enter the new state.
            //
            PnpEnterNewState(newState);
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

    while (newState != WdfDevStatePnpNull) {
        DoTraceLevelMessage(
             GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNPPOWERSTATES,
             "WDFDEVICE 0x%p !devobj 0x%p entering PnP State "
             "%!WDF_DEVICE_PNP_STATE! from %!WDF_DEVICE_PNP_STATE!",
             m_Device->GetHandle(),
             m_Device->GetDeviceObject(),
             newState,
             currentState);

        if (m_PnpStateCallbacks != NULL) {
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

        if (m_PnpStateCallbacks != NULL) {
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
        if (entry->StateFunc != NULL) {
            newState = entry->StateFunc(this);

            //
            // Validate the return value if FX_STATE_MACHINE_VERIFY is enabled
            //
            VALIDATE_PNP_STATE(currentState, newState);
        }
        else {
            newState = WdfDevStatePnpNull;
        }

        if (m_PnpStateCallbacks != NULL) {
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
    if (This->PnpSendStartDeviceDownTheStackOverload() == FALSE) {
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
    NTSTATUS status;
    BOOLEAN matched;

    status = STATUS_SUCCESS;
    matched = FALSE;

    This->QueryForReenumerationInterface();

    status = This->CreatePowerThreadIfNeeded();

    if (NT_SUCCESS(status)) {
        status = This->PnpPrepareHardware(&matched);
    }

    if (!NT_SUCCESS(status)) {
        if (matched == FALSE) {
            //
            // NOTE:  consider going to WdfDevStatePnpFailed instead of yet
            //        another failed state out of start device handling.
            //

            //
            // We can handle remove out of the init state, revert back to that
            // state.
            //
            return WdfDevStatePnpFailedInit;
        }
        else {
            //
            // EvtDevicePrepareHardware is what failed, goto a state where we
            // undo that call.
            //
            return WdfDevStatePnpFailedOwnHardware;
        }
    }

    //
    // We only query for the capabilities for the power policy owner because
    // we use the capabilities to determine the right Dx state when we want to
    // wake from S0 or Sx.  Since only the power policy owner can enable wake
    // behavior, only the owner needs to query for the information.
    //
    // ALSO, if we are a filter, there are issues in stacks wrt pnp reentrancy.







    //
    if (This->IsPowerPolicyOwner()) {
        //
        // Query the stack for capabilities before telling the stack hw is
        // available
        //
        status = This->QueryForCapabilities();

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "could not query caps for stack, %!STATUS!", status);

            This->SetPendingPnpIrpStatus(status);
            return WdfDevStatePnpFailedOwnHardware;
        }

        This->m_CapsQueried = TRUE;
    }

    This->PnpPowerPolicyStart();

    return WdfDevStatePnpNull;
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
    NTSTATUS status;

    status = This->PnpEnableInterfacesAndRegisterWmi();

    if (!NT_SUCCESS(status)) {
        //
        // Upon failure, PnpEnableInterfacesAndRegisterWmi already marked the
        // irp as failed and recorded an internal error.
        //
        // FailedPowerDown will gracefully tear down the stack and bring it out
        // of D0.
        //
        return WdfDevStatePnpFailedPowerDown;
    }

    return WdfDevStatePnpStarted;
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
    This->SetPendingPnpIrpStatus(STATUS_DEVICE_POWER_FAILURE);

    return WdfDevStatePnpFailedOwnHardware;
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
    WDF_DEVICE_PNP_STATE state;
    NTSTATUS    status;

    //
    // First, call the driver.  If it succeeds, look at whether
    // it managed to stop its stuff.
    //
    status = This->m_DeviceQueryRemove.Invoke(This->m_Device->GetHandle());

    if (NT_SUCCESS(status)) {
        //
        // The driver has stopped all of its self managed io.  Proceed to
        // stop everything before passing the request down the stack.
        //
        state = WdfDevStatePnpQueryRemoveEnsureDeviceAwake;
    }
    else {
        //
        // The callback didn't manage to stop.  Go back to the Started state
        // where we will complete the pended pnp irp
        //
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "EvtDeviceQueryRemove failed, %!STATUS!", status);

        if (status == STATUS_NOT_SUPPORTED) {
            DoTraceLevelMessage(
                This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "EvtDeviceQueryRemove returned an invalid status "
                "STATUS_NOT_SUPPORTED");

            if (This->GetDriverGlobals()->IsVerificationEnabled(1, 11, OkForDownLevel)) {
                FxVerifierDbgBreakPoint(This->GetDriverGlobals());
            }
        }

        state = WdfDevStatePnpStarted;
    }

    This->SetPendingPnpIrpStatus(status);

    return state;
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
    NTSTATUS status;
    WDF_DEVICE_PNP_STATE state;

    //
    // Make sure that the device is powered on before we send the query remove
    // on its way.  If we do this after we send the query remove, we could race
    // with the remove which removes the reference and we want the device
    // powered on when processing remove.
    //
    status = This->PnpPowerReferenceDuringQueryPnp();
    if (STATUS_PENDING == status) {
        //
        // Device is transitioning to D0. The Pnp state machine will wait in
        // the current state until the transition is complete
        //
        state = WdfDevStatePnpNull;
    }
    else if (NT_SUCCESS(status)) {
        //
        // Already in D0
        //
        state = WdfDevStatePnpQueryRemovePending;
    }
    else {
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "StopIdle on WDFDEVICE %p failed, %!STATUS!, failing query remove",
            This->m_Device->GetHandle(), status);

        This->SetPendingPnpIrpStatus(status);

        //
        // The Started state will complete the irp when it sees the failure
        // status set on the irp.
        //
        state = WdfDevStatePnpStarted;
    }

    return state;
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
    FxIrp irp;

    irp.SetIrp(This->ClearPendingPnpIrp());
    (void) This->FireAndForgetIrp(&irp);
    return WdfDevStatePnpNull;
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
    NTSTATUS status;
    BOOLEAN completeQuery;

    completeQuery = TRUE;

    if (This->m_DeviceStopCount != 0) {
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
            "Failing QueryRemoveDevice because the driver "
            "has indicated that it cannot be stopped, count %d",
            This->m_DeviceStopCount);

        status = STATUS_INVALID_DEVICE_STATE;
    }
    else if (This->IsInSpecialUse()) {
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
            "Failing QueryRemoveDevice due to open special file counts "
            "(paging %d, hiber %d, dump %d, boot %d)",
            This->GetUsageCount(WdfSpecialFilePaging),
            This->GetUsageCount(WdfSpecialFileHibernation),
            This->GetUsageCount(WdfSpecialFileDump),
            This->GetUsageCount(WdfSpecialFileBoot));

        status = STATUS_DEVICE_NOT_READY;
    }
    else {
        //
        // Go on to next state in the "remove" progression.
        //
        completeQuery = FALSE;
        status = STATUS_SUCCESS;
    }

    if (completeQuery) {
        //
        // Store the status which Started will complete
        //
        This->SetPendingPnpIrpStatus(status);

        //
        // Revert to started
        //
        return WdfDevStatePnpStarted;
    }
    else {
        //
        // Wait for the other state machines to stop
        //
        return WdfDevStatePnpQueryRemoveAskDriver;
    }
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
    //
    // It is important to stop power policy before releasing the reference.
    // If the reference was released first, we could get into a situation where
    // we immediately go idle and then we must send a D0 irp when in the remove.
    // If there are devices on top of this device and we send a D0 irp during
    // remove processing, the upper devices will be sent an irp after getting a
    // pnp remove (and either crash or fail the power irp upon receiving it).
    //
    This->PnpPowerPolicyStop();
    This->PnpPowerDereferenceSelf();

    return WdfDevStatePnpNull;
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
    WDF_DEVICE_PNP_STATE state;
    NTSTATUS    status;

    //
    // First, call the driver.  If it succeeds, look at whether
    // it managed to stop its stuff.
    //
    status = This->m_DeviceQueryStop.Invoke(This->m_Device->GetHandle());

    if (NT_SUCCESS(status)) {
        //
        // Tell the other state machines to stop
        //
        state = WdfDevStatePnpQueryStopEnsureDeviceAwake;
    }
    else {
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "EvtDeviceQueryStop failed, %!STATUS!", status);

        if (status == STATUS_NOT_SUPPORTED) {
            DoTraceLevelMessage(
                This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "EvtDeviceQueryStop returned an invalid status "
                "STATUS_NOT_SUPPORTED");

            if (This->GetDriverGlobals()->IsVerificationEnabled(1, 11, OkForDownLevel)) {
                FxVerifierDbgBreakPoint(This->GetDriverGlobals());
            }
        }

        //
        // The callback didn't manage to stop.  Go back to the Started state.
        //
        state = WdfDevStatePnpStarted;
    }

    This->SetPendingPnpIrpStatus(status);

    return state;
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
    NTSTATUS status;
    WDF_DEVICE_PNP_STATE state;

    //
    // Make sure that the device is powered on before we send the query stop
    // on its way.  If we do this after we send the query stop, we could race
    // with the stop and we want the device powered on when processing stop.
    //
    status = This->PnpPowerReferenceDuringQueryPnp();
    if (STATUS_PENDING == status) {
        //
        // Device is transitioning to D0. The Pnp state machine will wait in
        // the current state until the transition is complete
        //
        state = WdfDevStatePnpNull;
    }
    else if (NT_SUCCESS(status)) {
        //
        // Already in D0
        //
        state = WdfDevStatePnpQueryStopPending;
    }
    else {
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "StopIdle on WDFDEVICE %p failed, %!STATUS!, failing query stop",
            This->m_Device->GetHandle(), status);

        This->SetPendingPnpIrpStatus(status);

        //
        // The Started state will complete the irp when it sees the failure
        // status set on the irp.
        //
        state = WdfDevStatePnpStarted;
    }

    return state;
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
    FxIrp irp;

    irp.SetIrp(This->ClearPendingPnpIrp());
    (void) This->FireAndForgetIrp(&irp);
    return WdfDevStatePnpNull;
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
    NTSTATUS status;
    BOOLEAN completeQuery = TRUE;

    if (This->m_DeviceStopCount != 0) {
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
            "Failing QueryStopDevice because the driver "
            "has indicated that it cannot be stopped, count %d",
            This->m_DeviceStopCount);

        status = STATUS_INVALID_DEVICE_STATE;
    }
    else if (This->IsInSpecialUse()) {
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
            "Failing QueryStopDevice due to open special file counts (paging %d,"
            " hiber %d, dump %d, boot %d)",
            This->GetUsageCount(WdfSpecialFilePaging),
            This->GetUsageCount(WdfSpecialFileHibernation),
            This->GetUsageCount(WdfSpecialFileDump),
            This->GetUsageCount(WdfSpecialFileBoot));

        status = STATUS_DEVICE_NOT_READY;
    }
    else {
        //
        // Go on to next state in the "stop" progression.
        //
        status = STATUS_SUCCESS;
        completeQuery = FALSE;
    }

    if (completeQuery) {
        //
        // Set the state which started will complete the request with
        //
        This->SetPendingPnpIrpStatus(status);

        //
        // Revert to started
        //
        return WdfDevStatePnpStarted;
    }
    else {
        //
        // Go ask power what the self managed io state is
        //
        return WdfDevStatePnpQueryStopAskDriver;
    }
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
    //
    // Purge non power managed queues now
    //
    m_Device->m_PkgIo->StopProcessingForPower(
        FxIoStopProcessingForPowerPurgeNonManaged
        );

    if (m_SelfManagedIoMachine != NULL) {
        m_SelfManagedIoMachine->Cleanup();
    }

    //
    // Cleanup WMI *after* EvtDeviceSelfManagedIoCleanup b/c we want to cleanup
    // after a well known and documented time.  The WMI docs state that you can
    // register providers and instances all the way through
    // EvtDeviceSelfManagedIoCleanup, so we mark WMI as cleaned up after that
    // call.
    //
    m_Device->WmiPkgCleanup();

    //
    // Mark the device as removed.
    //
    m_PnpStateAndCaps.Value &= ~FxPnpStateRemovedMask;
    m_PnpStateAndCaps.Value |= FxPnpStateRemovedTrue;

    //
    // Now call the driver and tell it to cleanup all its software state.
    //
    // We do the dispose early here before deleting the object
    // since the PNP remove event is the main trigger for
    // the Dispose chain in the framework.
    //
    // (Almost everything in the driver is rooted on the device object)
    //

    m_Device->EarlyDispose();

    //
    // All the children are in the disposed state, destroy them all.  m_Device
    // is not destroyed in this call.
    //

    m_Device->DestroyChildren();

    //
    // Wait for all children to drain out and cleanup.
    //

    m_Device->m_DisposeList->WaitForEmpty();

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
    This->PnpPowerDereferenceSelf();

    return WdfDevStatePnpStarted;
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
    //
    // Remove any child PDOs which may still be lingering around.  We do
    // the cleanup here so that we do it only once for the PDO which is being
    // removed (but may stick around) b/c it was not reported as missing.
    //
    //
    // Iterate over all of the reported children
    //
    This->ChildListNotifyRemove(&This->m_PendingChildCount);

    //
    // Decrement our bias from when the device was (re)started.  If all of the
    // children removed themselves synchronously, we just move to the cleanup
    // state, otherwise wait for all the children to fully process the remove
    // before remove the parent so that ordering of removal between parent and
    // child is guaranteed (where the order is that all children are cleaned
    // up before the parent is cleaned up).
    //
    if (InterlockedDecrement(&This->m_PendingChildCount) > 0) {
        return WdfDevStatePnpRemovedWaitForChildren;
    }
    else {
        return WdfDevStatePnpRemovedChildrenRemoved;
    }
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
    if (This->m_DeviceRemoveProcessed != NULL) {
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
    //
    // Invoke EvtDeviceSurpriseRemove
    //
    This->m_DeviceSurpriseRemoval.Invoke(This->m_Device->GetHandle());

    //
    // Call the overloaded surprise remove handler since
    // PnpEventSurpriseRemovePendingOverload will not get called
    //
    This->PnpEventSurpriseRemovePendingOverload();

    //
    // The surprise remove irp was pended, complete it now
    //
    This->PnpFinishProcessingIrp();

    return WdfDevStatePnpRemovedPdoWait;
}

VOID
FxPkgPnp::PnpCleanupForRemove(
    __in BOOLEAN GracefulRemove
    )
/*++

Routine Description:
    This is a common worker function between surprise remove and the graceful
    remove path to do common cleanup. This involves deregistering from WMI,
    device interfaces, symbolic links and stopping power managed i/o.

Arguments:
    GracefulRemove - if TRUE, we are in the graceful remove path, otherwise we
                     are in the surprise remove path.

Return Value:
    None

  --*/
{
    //
    // Disable WMI.
    //
    m_Device->WmiPkgDeregister();

    //
    // Disable any device interfaces.
    //
    PnpDisableInterfaces();

    DeleteSymbolicLinkOverload(GracefulRemove);








    // Flush/purge top-edge queues
    m_Device->m_PkgIo->StopProcessingForPower(
        FxIoStopProcessingForPowerPurgeManaged
        );

    //
    // Invoke EvtDeviceSelfManagedIoFlush
    //
    if (m_SelfManagedIoMachine != NULL) {
        m_SelfManagedIoMachine->Flush();
    }

    //
    // Tell all the resource objects that they no longer own anything.
    //
    NotifyResourceobjectsToReleaseResources();

    //
    // Flush persistent state to permanent storage.  We do this in the failed
    // state for the surprise removed case.  By storing the state when we are
    // surprise removed, if the stack is reenumerated, it will pick up the saved
    // state that was just committed.  If the state was saved during remove
    // device it can be too late because the new instance of the same device
    // could already be up and running and not pick up the saved state.
    //
    // It is important to save the state before completing the (potentially)
    // pended pnp irp.  Completing the pnp irp will allow a new instance of the
    // device to be enumerated and we want to save state before that happens.
    //
    SaveState(FALSE);

    if (m_SharedPower.m_WaitWakeOwner) {
        //
        // Don't care about the return code, just blindly try to complete the
        // wake request.  The function can handle the case where there is no
        // irp to complete.
        //
        (void) PowerIndicateWaitWakeStatus(STATUS_NO_SUCH_DEVICE);
    }
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
    NTSTATUS status;

    //
    // Surprise remove path releases hardware first then disables the interfaces,
    // so do the same in the graceful remove path.
    //

    //
    // Call the driver and tell it to unmap resources.
    //
    status = This->PnpReleaseHardware();
    if (!NT_SUCCESS(status)) {
        //
        // The driver failed to unmap resources.  Presumably this means that
        // there are now some leaked PTEs.  Just log the failure.
        //
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "EvtDeviceReleaseHardware %p failed, %!STATUS!",
            This->m_Device->GetHandle(), status);
    }

    This->PnpCleanupForRemove(TRUE);

    //
    // Tell the power policy state machine to prepare for device removal
    //
    This->PnpPowerPolicyRemove();

    return WdfDevStatePnpNull;
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

    This->m_AchievedStart = TRUE;

    //
    // Log Telemetry event for the FDO
    //
    if (This->m_Device->IsPdo() == FALSE) {
        This->m_Device->FxLogDeviceStartTelemetryEvent();
    }

    This->PnpFinishProcessingIrp();

    return WdfDevStatePnpNull;
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
    This->PnpPowerPolicyStop();

    return WdfDevStatePnpNull;
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
    //
    // Send an event to the Power Policy State Machine
    // telling it to "Start."
    //
    This->PnpPowerPolicyStart();

    return WdfDevStatePnpNull;
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
    NTSTATUS status;
    BOOLEAN matched;

    status = This->PnpPrepareHardware(&matched);

    if (!NT_SUCCESS(status)) {
        //
        // We can handle remove out of the init state, revert back to that state
        //
        if (matched == FALSE) {
            //
            // Wait for the remove irp to come in
            //
            COVERAGE_TRAP();
            return WdfDevStatePnpFailed;
        }
        else {
            //
            // EvtDevicePrepareHardware is what failed, goto a state where we
            // undo that call.
            //
            COVERAGE_TRAP();
            return WdfDevStatePnpFailedOwnHardware;
        }
    }

    return WdfDevStatePnpRestarting;
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
    WDF_DEVICE_PNP_STATE state;
    NTSTATUS status;

    status = This->PnpReleaseHardware();
    if (NT_SUCCESS(status)) {
        //
        // Tell all the resource objects that they no longer own anything.
        //
        This->NotifyResourceobjectsToReleaseResources();

        state = WdfDevStatePnpNull;
    }
    else {
        DoTraceLevelMessage(This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                            "EvtDeviceReleaseHardware failed - %!STATUS!",
                            status);
        COVERAGE_TRAP();

        This->SetInternalFailure();
        state = WdfDevStatePnpFailed;
    }

    //
    // Send the irp on its merry way.  This irp (stop device) cannot be failed
    // and must always be sent down the stack.
    //
    This->PnpFinishProcessingIrp();

    return state;
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
    if (This->PnpSendStartDeviceDownTheStackOverload() == FALSE) {
        //
        // The start irp's completion routine will move the state machine into
        // the new state.
        //
        return WdfDevStatePnpNull;
    }

    return WdfDevStatePnpStartingFromStopped;
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
    //
    // It is important to stop power policy before releasing the reference.
    // If the reference was released first, we could get into a situation where
    // we immediately go idle and then we must send a D0 irp when in the remove.
    // If there are devices on top of this device and we send a D0 irp during
    // remove processing, the upper devices will be sent an irp after getting a
    // pnp remove (and either crash or fail the power irp upon receiving it).
    //
    This->PnpPowerPolicyStop();
    This->PnpPowerDereferenceSelf();

    return WdfDevStatePnpNull;
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
    //
    // Invoke EvtDeviceSurpriseRemove
    //
    This->m_DeviceSurpriseRemoval.Invoke(This->m_Device->GetHandle());

    //
    // Notify the virtual override of the surprise remove.
    //
    This->PnpEventSurpriseRemovePendingOverload();

    return WdfDevStatePnpFailed;
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
    FxIrp irp(This->ClearPendingPnpIrp());

    irp.SetStatus(STATUS_SUCCESS);
    This->FireAndForgetIrp(&irp);

    return WdfDevStatePnpNull;
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
    FxIrp irp(This->ClearPendingPnpIrp());

    This->FireAndForgetIrp(&irp);

    return WdfDevStatePnpInit;
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
    COVERAGE_TRAP();

    This->PnpPowerDereferenceSelf();

    return WdfDevStatePnpSurpriseRemoveIoStarted;
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
    //
    // Invoke EvtDeviceSurpriseRemove
    //

    This->m_DeviceSurpriseRemoval.Invoke(This->m_Device->GetHandle());

    //
    // Notify the virtual override of the surprise remove.
    //
    This->PnpEventSurpriseRemovePendingOverload();

    return WdfDevStatePnpFailedIoStarting;
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
    //
    // Normal stop so that the power state machine will go through the power off
    // path and not skip directly to off like it would if we sent it a surprise
    // remove notification.
    //
    This->PnpPowerPolicyStop();

    return WdfDevStatePnpNull;
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
    This->PnpPowerPolicySurpriseRemove();

    return WdfDevStatePnpNull;
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
    //
    // Invoke EvtDeviceReleaseHardware
    //
    (void) This->PnpReleaseHardware();

    return WdfDevStatePnpFailed;
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
    This->PnpCleanupForRemove(FALSE);

    //
    // Tell the power policy state machine to prepare for device removal
    //
    This->PnpPowerPolicyRemove();

    return WdfDevStatePnpNull;
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
    //
    // Finish processing any pended PnP IRP.  Since we can reach this state from
    // states where a pnp irp was *not* pended, we do not require a pnp irp to
    // have been pended when trying to complete it.
    //
    This->PnpFinishProcessingIrp(FALSE);

    //
    // Request reenumeration if the client driver asked for it or if there was
    // an internal failure *and* if the client driver didn't specify failure...
    // AND if we have not yet exceeded our restart count within a period of time.
    //
    if ((This->m_FailedAction == WdfDeviceFailedAttemptRestart ||
         (This->m_FailedAction == WdfDeviceFailedUndefined && This->m_InternalFailure))
        &&
        This->PnpCheckAndIncrementRestartCount()) {
        //
        // No need to invalidate state because we are in a state waiting for
        // a remove device anyways so failure is imminent.
        //
        This->AskParentToRemoveAndReenumerate();
    }

    if (This->m_FailedAction != WdfDeviceFailedUndefined || This->m_InternalFailure) {
        //
        // If the failure occurred in this device, then tear down the stack if
        // we are in a state in which pnp thinks we are started.  If we are
        // already in a stopped state, this invalidation will do no harm.
        //
        MxDeviceObject physicalDeviceObject(
                                This->m_Device->GetPhysicalDevice()
                                );

        //
        // We need to pass FDO as a parameter as UMDF currently doesn't have
        // PDOs and instead needs FDO to invalidate device state.
        //
        physicalDeviceObject.InvalidateDeviceState(
            This->m_Device->GetDeviceObject() //FDO
            );
    }

    return WdfDevStatePnpFailedWaitForRemove;
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
    //
    // Invoke EvtDeviceSurpriseRemove
    //
    This->m_DeviceSurpriseRemoval.Invoke(This->m_Device->GetHandle());

    //
    // Call the overloaded surprise remove handler
    //
    This->PnpEventSurpriseRemovePendingOverload();

    //
    // The surprise remove irp was pended, complete it now.  The irp need not
    // be present.  If we failed before the surprise irp was sent and the irp
    // arrived in the middle of processing the failure, we could have completed
    // the s.r. irp in FailedWaitForRemove, which is OK.
    //
    This->PnpFinishProcessingIrp(FALSE);

    //
    // Return back to the failed state where will wait for remove
    //
    return WdfDevStatePnpFailedWaitForRemove;
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
    //
    // Complete the pended start irp with error
    //
    This->SetPendingPnpIrpStatus(STATUS_INVALID_DEVICE_STATE);
    This->PnpFinishProcessingIrp();

    return WdfDevStatePnpFailedWaitForRemove;
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
    //
    // Release the power thread that we may have previously acquired in
    // HardwareAvailable.
    //
    This->ReleasePowerThread();

    //
    // Deref the reenumeration interface
    //
    This->ReleaseReenumerationInterface();

    This->PnpFinishProcessingIrp();

    return WdfDevStatePnpInit;
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
    COVERAGE_TRAP();


    This->m_Device->EarlyDispose();

    //
    // All the children are in the disposed state, destroy them all.  m_Device
    // is not destroyed in this call.
    //

    This->m_Device->DestroyChildren();

    return WdfDevStatePnpFinal;
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
    //
    // Stop the power policy machine so that we simulate stopping first
    //
    This->PnpPowerPolicyStop();

    return WdfDevStatePnpNull;
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
    NTSTATUS status;

    status = This->PnpReleaseHardware();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "EvtDeviceReleaseHardware failed with %!STATUS!", status);

        COVERAGE_TRAP();

        This->SetInternalFailure();
        This->SetPendingPnpIrpStatus(status);

        return WdfDevStatePnpFailed;
    }

    if (This->PnpSendStartDeviceDownTheStackOverload() == FALSE) {
        //
        // The start irp's completion routine will move the state machine into
        // the new state.
        //
        return WdfDevStatePnpNull;
    }

    //
    // Start happened synchronously.  Transition to the new state now.
    //
    return WdfDevStatePnpRestartHardwareAvailable;
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
    NTSTATUS status;
    BOOLEAN matched;

    status = This->PnpPrepareHardware(&matched);

    if (!NT_SUCCESS(status)) {
        if (matched == FALSE) {
            //
            // Wait for the remove irp to come in
            //
            COVERAGE_TRAP();
            return WdfDevStatePnpFailed;
        }
        else {
            //
            // EvtDevicePrepareHardware is what failed, goto a state where we
            // undo that call.
            //
            COVERAGE_TRAP();
            return WdfDevStatePnpFailedOwnHardware;
        }
    }

    This->PnpPowerPolicyStart();

    return WdfDevStatePnpNull;
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
    //
    // Since this is a PDO and it is being restarted, it could have had an internal
    // failure during its previous start.  Since we use m_InternalFailure to set
    // PNP_DEVICE_FAILED when handling IRP_MN_QUERY_PNP_DEVICE_STATE, it should
    // be set to FALSE so we don't immediately fail the device after the start
    // has been succeeded.
    //
    This->m_InternalFailure = FALSE;
    This->m_Failed = FALSE;

    //
    // The PDO is being restarted and could have previous had a power thread
    // running.  If so, the reference count goes to zero when removed from a
    // started state.  Reset back to 1.
    //
    This->m_PowerThreadInterfaceReferenceCount = 1;

    //
    // The count is decremented on the initial started->removed transition (and
    // not subsequent removed -> removed transitiosn).  On removed -> restarted,
    // we need to set the count back to a bias of 1 so when we process the remove
    // again we can know if there are any pending child (of this PDO) removals
    // that we must wait for.
    //
    This->m_PendingChildCount = 1;

    //
    // Reset WMI state
    //
    This->m_Device->m_PkgWmi->ResetStateForPdoRestart();


    This->m_Device->m_PkgIo->ResetStateForRestart();

    if (This->IsPowerPolicyOwner()) {
        This->m_PowerPolicyMachine.m_Owner->m_PowerIdleMachine.Reset();
    }

    //
    // Set STATUS_SUCCESS in the irp so that the stack will start smoothly after
    // we have powered up.
    //
    This->SetPendingPnpIrpStatus(STATUS_SUCCESS);

    //
    // This flag is set on the wake-enabled device powering down path,
    // but if the device power down failed it may not have been cleared.
    //
    This->m_WakeInterruptsKeepConnected = FALSE;


    //
    // This flag is cleared so we can reacquire the start time and state
    //
    This->m_AchievedStart = FALSE;

    return WdfDevStatePnpHardwareAvailable;
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
    NTSTATUS status;

    //
    // We may not have a pnp irp at this stage (esp for PDO which are in the
    // removed state and whose parent is being removed) so we use the function
    // pointer as the unique tag.
    //
    // IoReleaseRemoveLockAndWait requires an outstanding reference to release,
    // so acquire it before calling it if we are in the case where the PDO is
    // being removed with no outstanding PNP remove irp b/c the parent is being
    // removed.
    //
    if (This->m_DeviceRemoveProcessed == NULL) {
        status = Mx::MxAcquireRemoveLock(
            This->m_Device->GetRemoveLock(),
            &FxPkgPnp::PnpEventFinal);

        ASSERT(NT_SUCCESS(status));
        UNREFERENCED_PARAMETER(status);
    }

    //
    // Indicate to the parent device that we are removed now (vs the destructor
    // of the object where we would never reach because in the case of the PDO
    // being removed b/c the parent is going away, the parent has a reference
    // on the PDO).







    if (This->m_Device->m_ParentWaitingOnChild) {
        (This->m_Device->m_ParentDevice->m_PkgPnp)->ChildRemoved();
    }


    if (This->m_DeviceRemoveProcessed == NULL) {
        //
        // We can get into this state w/out an event to set when a PDO (this
        // device) is in the removed state and then the parent is removed.
        //

        //
        // After this is called, any irp dispatched to FxDevice::DispatchWithLock
        // will fail with STATUS_INVALID_DEVICE_REQUEST.
        //
        Mx::MxReleaseRemoveLockAndWait(
            This->m_Device->GetRemoveLock(),
            &FxPkgPnp::PnpEventFinal);

        //
        // Delete the object when we exit the state machine.  Dispose was run
        // early in a previous state.
        //
        This->m_PnpMachine.SetDelayedDeletion();
    }
    else {
        //
        // The thread which received the pnp remove irp will delete the device
        //
        This->m_SetDeviceRemoveProcessed = TRUE;
    }

    return WdfDevStatePnpNull;
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::PnpEnableInterfacesAndRegisterWmi(
    VOID
    )
/*++

Routine Description:
    Enables all of the device interfaces and then registers wmi.

Arguments:
    None

Return Value:
    NT_SUCCESS if all goes well, !NT_SUCCESS otherwise

  --*/
{
    PSINGLE_LIST_ENTRY ple;
    NTSTATUS status;

    status = STATUS_SUCCESS;

    //
    // Enable any device interfaces.
    //
    m_DeviceInterfaceLock.AcquireLock(GetDriverGlobals());

    m_DeviceInterfacesCanBeEnabled = TRUE;

    for (ple = m_DeviceInterfaceHead.Next; ple != NULL; ple = ple->Next) {
        FxDeviceInterface *pDeviceInterface;

        pDeviceInterface = FxDeviceInterface::_FromEntry(ple);






#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
        //
        // By this time, the device interface, no matter what the WDFDEVICE role
        // will have been registered.
        //
        ASSERT(pDeviceInterface->m_SymbolicLinkName.Buffer != NULL);
#endif
        pDeviceInterface->SetState(TRUE);

        status = STATUS_SUCCESS;
    }

    m_DeviceInterfaceLock.ReleaseLock(GetDriverGlobals());

    if (NT_SUCCESS(status)) {
        status = m_Device->WmiPkgRegister();
    }

    if (!NT_SUCCESS(status)) {
        SetInternalFailure();
        SetPendingPnpIrpStatus(status);
    }

    return status;
}

__drv_when(!NT_SUCCESS(return), __drv_arg(ResourcesMatched, _Must_inspect_result_))
NTSTATUS
FxPkgPnp::PnpPrepareHardware(
    __inout PBOOLEAN ResourcesMatched
    )
/*++

Routine Description:
    Matches the PNP resources with the WDFINTERRUPT objects registered and then
    calls EvtDevicePrepareHardware.  All start paths call this function

Arguments:
    ResourcesMatched - indicates to the caller what stage failed if !NT_SUCCESS
                        is returned

Return Value:
    NT_SUCCESS if all goes well, !NT_SUCCESS if failure occurrs

  --*/
{
    NTSTATUS status;
    *ResourcesMatched = FALSE;

    //
    // FxPnpStateRemoved:
    // Mark the device a not removed.  This is just so that anybody sending
    // a PnP IRP_MN_QUERY_DEVICE_STATE gets a reasonable answer.
    //
    // FxPnpStateFailed, FxPnpStateResourcesChanged:
    // Both of these values can be set to true and cause another start to
    // be sent down the stack.  Reset these values back to false.  If there is
    // a need to set these values, the driver can set them in
    // EvtDevicePrepareHardware.
    //
    m_PnpStateAndCaps.Value &= ~(FxPnpStateRemovedMask |
                                 FxPnpStateFailedMask |
                                 FxPnpStateResourcesChangedMask);
    m_PnpStateAndCaps.Value |= (FxPnpStateRemovedUseDefault |
                                FxPnpStateFailedUseDefault |
                                FxPnpStateResourcesChangedUseDefault);

    //
    // This will parse the resources and setup all the WDFINTERRUPT handles
    //
    status = PnpMatchResources();

    if (!NT_SUCCESS(status)) {
        *ResourcesMatched = FALSE;
        SetInternalFailure();
        SetPendingPnpIrpStatus(status);
        return status;
    }

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    //
    // Build register resource table
    //
    status = m_Resources->BuildRegisterResourceTable();
    if (!NT_SUCCESS(status)) {
        SetInternalFailure();
        SetPendingPnpIrpStatus(status);
        goto exit;
    }

    //
    // Build Port resource table
    //
    status = m_Resources->BuildPortResourceTable();
    if (!NT_SUCCESS(status)) {
        SetInternalFailure();
        SetPendingPnpIrpStatus(status);
        goto exit;
    }

    //
    // We keep track if the device has any connection resources,
    // in which case we allow unrestricted access to interrupts
    // regardless of the UmdfDirectHardwareAccess directive.
    //
    status = m_Resources->CheckForConnectionResources();
    if (!NT_SUCCESS(status)) {
        SetInternalFailure();
        SetPendingPnpIrpStatus(status);
        goto exit;
    }
#endif

    *ResourcesMatched = TRUE;

    m_Device->SetCallbackFlags(
                        FXDEVICE_CALLBACK_IN_PREPARE_HARDWARE
                        );

    status = m_DevicePrepareHardware.Invoke(m_Device->GetHandle(),
                                            m_ResourcesRaw->GetHandle(),
                                            m_Resources->GetHandle());

    m_Device->ClearCallbackFlags(
                        FXDEVICE_CALLBACK_IN_PREPARE_HARDWARE
                        );

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                            "EvtDevicePrepareHardware failed %!STATUS!", status);

        if (status == STATUS_NOT_SUPPORTED) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                "EvtDevicePrepareHardware returned an invalid status "
                "STATUS_NOT_SUPPORTED");

            if (GetDriverGlobals()->IsVerificationEnabled(1, 11, OkForDownLevel)) {
                FxVerifierDbgBreakPoint(GetDriverGlobals());
            }
        }

        SetInternalFailure();
        SetPendingPnpIrpStatus(status);
        goto exit;
    }

    //
    // Now that we have assigned the resources to all the interrupts, figure out
    // the highest synch irql for each interrupt set which shares a spinlock.
    //
    PnpAssignInterruptsSyncIrql();

    //
    // Do mode-specific work. For KMDF, there is nothing additional to do.
    //
    status = PnpPrepareHardwareInternal();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
                            "PrepareHardware failed %!STATUS!", status);

        SetInternalFailure();
        SetPendingPnpIrpStatus(status);
    }

exit:
    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::PnpReleaseHardware(
    VOID
    )
/*++

Routine Description:
    Invokes the driver's release hardware callback if present.
    Releases any interrupt resources allocated during the prepare hardware callback.

Arguments:
    None

Return Value:
    Driver's release hardware callback return status or
    STATUS_SUCCESS if callback is not present.

  --*/
{
    NTSTATUS        status;
    FxInterrupt*    interrupt;
    PLIST_ENTRY     le;

    //
    // Invoke the device's release hardware callback.
    //
    status = m_DeviceReleaseHardware.Invoke(
                        m_Device->GetHandle(),
                        m_Resources->GetHandle());

    if (status == STATUS_NOT_SUPPORTED) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "EvtDeviceReleaseHardware returned an invalid status "
            "STATUS_NOT_SUPPORTED");

        if (GetDriverGlobals()->IsVerificationEnabled(1, 11, OkForDownLevel)) {
            FxVerifierDbgBreakPoint(GetDriverGlobals());
        }
    }

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    if (NT_SUCCESS(status)) {
        //
        // make sure driver has unmapped its resources
        //
        m_Resources->ValidateResourceUnmap();
    }

    //
    // delete the register and port resource tables
    //
    m_Resources->DeleteRegisterResourceTable();
    m_Resources->DeletePortResourceTable();
#endif

    //
    // Delete all interrupt objects that were created in the prepare hardware
    // callback that the driver did not explicitly delete (in reverse order).
    //
    le = m_InterruptListHead.Blink;

    while(le != &m_InterruptListHead) {

        // Find the interrupt object pointer.
        interrupt = CONTAINING_RECORD(le, FxInterrupt, m_PnpList);

        // Advance the entry before it becomes invalid.
        le = le->Blink;

        // Let the interrupt know that 'release hardware' was called.
        interrupt->OnPostReleaseHardware();
    }

    return status;
}

VOID
FxPkgPnp::PnpPowerPolicyStart(
    VOID
    )
/*++

Routine Description:
    Informs the power policy state machine that it should start.

Arguments:
    None

Return Value:
    None

  --*/
{
    PowerPolicyProcessEvent(PwrPolStart);
}

VOID
FxPkgPnp::PnpPowerPolicyStop(
    VOID
    )
/*++

Routine Description:
    Informs the power and power policy state machines that they should stop.

Arguments:
    None

Return Value:
    None

  --*/
{
    PowerPolicyProcessEvent(PwrPolStop);
}

VOID
FxPkgPnp::PnpPowerPolicySurpriseRemove(
    VOID
    )
/*++

Routine Description:
    Informs the policy state machines that it should stop due to the hardware
    being surprise removed.

Arguments:
    None

Return Value:
    None

  --*/
{
    PowerPolicyProcessEvent(PwrPolSurpriseRemove);
}

VOID
FxPkgPnp::PnpPowerPolicyRemove(
    VOID
    )
/*++

Routine Description:
    Informs the policy state machine that it should prepare for device removal.

Arguments:
    None

Return Value:
    None

  --*/
{
    PowerPolicyProcessEvent(PwrPolRemove);
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
    if (IsPresentPendingPnpIrp()) {

        irp.SetIrp(GetPendingPnpIrp());
        if (irp.GetMinorFunction() == IRP_MN_START_DEVICE
            ||
            !NT_SUCCESS(irp.GetStatus())) {

            irp.SetIrp(ClearPendingPnpIrp());
            CompletePnpRequest(&irp, irp.GetStatus());
        }
        else {
            m_PnpMachine.m_FireAndForget = TRUE;
        }
    }
}

VOID
FxPkgPnp::PnpDisableInterfaces(
    VOID
    )
/*++

Routine Description:
    Disables all of the registerd interfaces on the device.

Arguments:
    None

Return Value:
    None

  --*/
{
    PSINGLE_LIST_ENTRY ple;

    m_DeviceInterfaceLock.AcquireLock(GetDriverGlobals());

    m_DeviceInterfacesCanBeEnabled = FALSE;

    for (ple = m_DeviceInterfaceHead.Next; ple != NULL; ple = ple->Next) {

        FxDeviceInterface *pDeviceInterface;
        pDeviceInterface = FxDeviceInterface::_FromEntry(ple);
        pDeviceInterface->SetState(FALSE);
    }

    m_DeviceInterfaceLock.ReleaseLock(GetDriverGlobals());
}

VOID
FxPkgPnp::PnpEventSurpriseRemovePendingOverload(
    VOID
    )
{

    //
    // Mark all of the children as missing because the parent has just been
    // removed.   Note that this will happen after all of the children have
    // already received the surprise remove event.  This is OK because the
    // reported status is inspected during the remove device event which will
    // happen after the parent finishes processing the surprise event.
    //
    if (m_EnumInfo != NULL) {
        m_EnumInfo->m_ChildListList.LockForEnum(GetDriverGlobals());
        FxTransactionedEntry* ple;

        ple = NULL;
        while ((ple = m_EnumInfo->m_ChildListList.GetNextEntry(ple)) != NULL) {
            FxChildList::_FromEntry(ple)->NotifyDeviceSurpriseRemove();
        }

        m_EnumInfo->m_ChildListList.UnlockFromEnum(GetDriverGlobals());
    }
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::PnpMatchResources(
    VOID
    )
/*++

Routine Description:

    This method is called in response to a PnP StartDevice IRP
    coming up the stack.  It:

    - Captures the device's resources
    - Calls out to interested resource objects
    - Sends an event to the PnP state machine

Arguemnts:

    Irp - a pointer to the FxIrp

Returns:

    NTSTATUS

--*/
{
    PCM_RESOURCE_LIST pResourcesRaw;
    PCM_RESOURCE_LIST pResourcesTranslated;
    FxResourceCm* resCmRaw;
    FxResourceCm* resCmTrans;
    FxInterrupt*  interrupt;
    PLIST_ENTRY ple;
    NTSTATUS status;
    FxCollectionEntry *curRaw, *curTrans, *endTrans;
    ULONG messageCount;
    FxIrp irp;

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Entering PnpMatchResources");

    //
    // We must clear these flags before calling into the event handler because
    // it might set these states back (which is OK).  If we don't clear these
    // states and the start succeeds, we would endlessly report that our
    // resources have changed and be restarted over and over.
    //
    m_PnpStateAndCaps.Value &= ~(FxPnpStateFailedMask |
                                 FxPnpStateResourcesChangedMask);
    m_PnpStateAndCaps.Value |= (FxPnpStateFailedUseDefault |
                                FxPnpStateResourcesChangedUseDefault);

    irp.SetIrp(m_PendingPnPIrp);
    pResourcesRaw = irp.GetParameterAllocatedResources();
    pResourcesTranslated = irp.GetParameterAllocatedResourcesTranslated();

    status = m_ResourcesRaw->BuildFromWdmList(pResourcesRaw, FxResourceNoAccess);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "Could not allocate raw resource list for WDFDEVICE 0x%p, %!STATUS!",
            m_Device->GetHandle(), status);
        goto Done;
    }

    status = m_Resources->BuildFromWdmList(pResourcesTranslated, FxResourceNoAccess);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "Could not allocate translated resource list for WDFDEVICE 0x%p, %!STATUS!",
            m_Device->GetHandle(), status);
        goto Done;
    }

    //
    // reset the stored information in all interrupts in the rebalance case
    //
    for (ple = m_InterruptListHead.Flink;
         ple != &m_InterruptListHead;
         ple = ple->Flink) {
        interrupt = CONTAINING_RECORD(ple, FxInterrupt, m_PnpList);
        interrupt->Reset();
    }

    //
    // Now iterate across the resources, looking for ones that correspond
    // to objects that we are managing.  Tell those objects about the resources
    // that were assigned.
    //
    ple = &m_InterruptListHead;

    endTrans = m_Resources->End();

    for (curTrans = m_Resources->Start(), curRaw = m_ResourcesRaw->Start();
         curTrans != endTrans;
         curTrans = curTrans->Next(), curRaw = curRaw->Next()) {

        ASSERT(curTrans->m_Object->GetType() == FX_TYPE_RESOURCE_CM);
        ASSERT(curRaw->m_Object->GetType() == FX_TYPE_RESOURCE_CM);

        resCmRaw = (FxResourceCm*) curRaw->m_Object;

        if (resCmRaw->m_Descriptor.Type == CmResourceTypeInterrupt) {
            //
            // We're looking at an interrupt resource.
            //
            if (ple->Flink == &m_InterruptListHead) {
                //
                // Oops, there are no more interrupt objects.
                //
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGPNP,
                    "Not enough interrupt objects created by WDFDEVICE 0x%p",
                    m_Device->GetHandle());
                break;
            }

            resCmTrans = (FxResourceCm*) curTrans->m_Object;
            ASSERT(resCmTrans->m_Descriptor.Type == CmResourceTypeInterrupt);

            messageCount = resCmRaw->m_Descriptor.u.MessageInterrupt.Raw.MessageCount;

            if (FxInterrupt::_IsMessageInterrupt(resCmTrans->m_Descriptor.Flags)
                &&
                (messageCount > 1)) {
                ULONG i;
                //
                // Multi-message MSI 2.2 needs to be handled differently
                //
                for (i = 0, ple = ple->Flink;
                     i < messageCount && ple != &m_InterruptListHead;
                     i++, ple = ple->Flink) {

                    //
                    // Get the next interrupt object.
                    //
                    interrupt = CONTAINING_RECORD(ple, FxInterrupt, m_PnpList);

                    //
                    // Tell the interrupt object what its resources are.
                    //
                    interrupt->AssignResources(&resCmRaw->m_Descriptor,
                                               &resCmTrans->m_Descriptor);
                }
            }
            else {
                //
                // This is either MSI2.2 with 1 message, MSI-X or Line based.
                //
                ple = ple->Flink;
                interrupt = CONTAINING_RECORD(ple, FxInterrupt, m_PnpList);

                //
                // Tell the interrupt object what its resources are.
                //
                interrupt->AssignResources(&resCmRaw->m_Descriptor,
                                           &resCmTrans->m_Descriptor);
            }
        }
    }

#if FX_IS_KERNEL_MODE









    //
    // If there are any pended I/Os that were sent to the target
    // that were pended in the transition to stop, then this will
    // resend them.
    //
    // ISSUE:  This has the potential of I/O completing
    // before the driver's start callback has been called...but,
    // this is the same as the PDO pending a sent irp and completing
    // it when the PDO is restarted before the FDO has a change to
    // process the start irp which was still pended below.
    //
    if (m_Device->IsFilter()) {
        //
        // If this is a filter device, then copy the FILE_REMOVABLE_MEDIA
        // characteristic from the lower device.
        //
        if (m_Device->GetAttachedDevice()->Characteristics & FILE_REMOVABLE_MEDIA) {
            ULONG characteristics;

            characteristics =
                m_Device->GetDeviceObject()->Characteristics | FILE_REMOVABLE_MEDIA;

            m_Device->GetDeviceObject()->Characteristics = characteristics;
        }










        m_Device->SetFilterIoType();
    }

#endif // FX_IS_KERNEL_MODE

Done:
    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "Exiting PnpMatchResources %!STATUS!", status);

    return status;
}

VOID
FxPkgPnp::PnpAssignInterruptsSyncIrql(
    VOID
    )
/*++

Routine Description:
    Figure out the highest synch irql for each interrupt set which shares a
    spinlock.

    foreach(interrupt assigned to this instance)
        determine the max sync irql in the set
        set all the associated interrupts to the sync irql
        set the sync irql on the first interrupt in the set

Arguments:
    None

Return Value:
    None

  --*/
{
    PLIST_ENTRY ple;
    FxInterrupt* pInterrupt;

    for (ple = m_InterruptListHead.Flink;
         ple != &m_InterruptListHead;
         ple = ple->Flink) {

        KIRQL syncIrql;

        pInterrupt = CONTAINING_RECORD(ple, FxInterrupt, m_PnpList);

        syncIrql = pInterrupt->GetResourceIrql();

        if (syncIrql == PASSIVE_LEVEL) {
            //
            // The irql associated with the resources assigned is passive,
            // this can happen in the following scenarios:
            //
            // (1)  no resources were assigned.  Skip this interrupt b/c it has
            //      no associated resources. Note: setting the SynchronizeIrql
            //      to PASSIVE_LEVEL is a no-op.
            //
            // (2) this interrupt is handled at passive-level.
            //      Set SynchronizeIrql to passive-level and continue.
            //
            pInterrupt->SetSyncIrql(PASSIVE_LEVEL);
            continue;
        }

        if (pInterrupt->IsSharedSpinLock() == FALSE) {
            //
            // If the interrupt spinlock is not shared, it's sync irql is the
            // irql assigned to it in the resources.
            //
            pInterrupt->SetSyncIrql(syncIrql);
        }
        else if (pInterrupt->IsSyncIrqlSet() == FALSE) {
            FxInterrupt* pFwdInterrupt;
            PLIST_ENTRY pleFwd;

            //
            // Find all of the other interrupts which share the lock and compute
            // the max sync irql.
            //
            for (pleFwd = ple->Flink;
                 pleFwd != &m_InterruptListHead;
                 pleFwd = pleFwd->Flink) {

                pFwdInterrupt = CONTAINING_RECORD(pleFwd, FxInterrupt, m_PnpList);

                //
                // If the 2 do not share the same lock, they are not in the same
                // set.
                //
                if (pFwdInterrupt->SharesLock(pInterrupt) == FALSE) {
                    continue;
                }

                if (pFwdInterrupt->GetResourceIrql() > syncIrql) {
                    syncIrql = pFwdInterrupt->GetResourceIrql();
                }
            }

            //
            // Now that we found the max sync irql, set it for all interrupts in
            // the set which share the lock
            //
            for (pleFwd = ple->Flink;
                 pleFwd != &m_InterruptListHead;
                 pleFwd = pleFwd->Flink) {

                pFwdInterrupt = CONTAINING_RECORD(pleFwd, FxInterrupt, m_PnpList);

                //
                // If the 2 do not share the same lock, they are not in the same
                // set.
                //
                if (pFwdInterrupt->SharesLock(pInterrupt) == FALSE) {
                    continue;
                }

                pFwdInterrupt->SetSyncIrql(syncIrql);
            }

            //
            // Set the sync irql for the first interrupt in the set.  We have set
            // the sync irql for all other interrupts in the set.
            //
            pInterrupt->SetSyncIrql(syncIrql);
        }
        else {
            //
            // If IsSyncIrqlSet is TRUE, we already covered this interrupt in a
            // previous pass of this loop when we computed the max sync irql for
            // an interrupt set.
            //
            ASSERT(pInterrupt->GetSyncIrql() > PASSIVE_LEVEL);
            DO_NOTHING();
        }
    }
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::ValidateCmResource(
    __inout PCM_PARTIAL_RESOURCE_DESCRIPTOR* CmResourceRaw,
    __inout PCM_PARTIAL_RESOURCE_DESCRIPTOR* CmResource
    )
/*++

Routine Description:
    Makes sure the specified resource is valid.

Arguments:
    CmResourceRaw - the raw resource to validate.
    CmResource - the translated resources to validate.

Return Value:
    STATUS_SUCCESS if resource is valid or
    NTSTATUS error.

  --*/
{
    NTSTATUS            status;
    FxCollectionEntry*  cur;
    FxCollectionEntry*  curRaw;
    FxResourceCm*       res;
    FxResourceCm*       resRaw;
    PFX_DRIVER_GLOBALS  fxDriverGlobals;

    ASSERT(m_ResourcesRaw != NULL);
    ASSERT(m_Resources != NULL);

    res = NULL;
    resRaw = NULL;

    fxDriverGlobals = GetDriverGlobals();

    //
    // Find the resource in our list.
    //
    for (cur = m_Resources->Start(), curRaw = m_ResourcesRaw->Start();
         cur != m_Resources->End();
         cur = cur->Next(), curRaw = curRaw->Next()) {

        res = (FxResourceCm*) cur->m_Object;
        resRaw = (FxResourceCm*) curRaw->m_Object;

        if (&res->m_DescriptorClone == *CmResource) {
            break;
        }
    }

    //
    // Error out if not found.
    //
    if (cur == m_Resources->End()) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "The translated PCM_PARTIAL_RESOURCE_DESCRIPTOR 0x%p is not valid, "
            "WDFDEVICE 0x%p, %!STATUS!",
            *CmResource, m_Device->GetHandle(), status);
        FxVerifierDbgBreakPoint(fxDriverGlobals);
        goto Done;
    }

    //
    // Error out if the associated raw resource is not the same.
    //
    if (&resRaw->m_DescriptorClone != *CmResourceRaw) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "The raw PCM_PARTIAL_RESOURCE_DESCRIPTOR 0x%p is not valid, "
            "WDFDEVICE 0x%p, %!STATUS!",
            *CmResourceRaw, m_Device->GetHandle(), status);
        FxVerifierDbgBreakPoint(fxDriverGlobals);
        goto Done;
    }

    //
    // Make sure driver didn't change any of the PnP settings.
    //
    if (sizeof(res->m_Descriptor) !=
            RtlCompareMemory(&res->m_DescriptorClone,
                             &res->m_Descriptor,
                             sizeof(res->m_Descriptor))) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "The translated PCM_PARTIAL_RESOURCE_DESCRIPTOR 0x%p is not valid, "
            "driver cannot change the assigned PnP resources, WDFDEVICE 0x%p, "
            "%!STATUS!",
            *CmResource, m_Device->GetHandle(), status);
        FxVerifierDbgBreakPoint(fxDriverGlobals);
        goto Done;
    }

    if (sizeof(resRaw->m_Descriptor) !=
            RtlCompareMemory(&resRaw->m_DescriptorClone,
                             &resRaw->m_Descriptor,
                             sizeof(resRaw->m_Descriptor))) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "The raw PCM_PARTIAL_RESOURCE_DESCRIPTOR 0x%p is not valid, "
            "driver cannot change the assigned PnP resources, WDFDEVICE 0x%p, "
            "%!STATUS!",
            *CmResourceRaw, m_Device->GetHandle(), status);
        FxVerifierDbgBreakPoint(fxDriverGlobals);
        goto Done;
    }

    //
    // Return the real descriptor.
    //
    ASSERT(res != NULL && resRaw != NULL);
    *CmResource = &res->m_Descriptor;
    *CmResourceRaw = &resRaw->m_Descriptor;

    status = STATUS_SUCCESS;

Done:
    return status;
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::ValidateInterruptResourceCm(
    __in PCM_PARTIAL_RESOURCE_DESCRIPTOR CmIntResourceRaw,
    __in PCM_PARTIAL_RESOURCE_DESCRIPTOR CmIntResource,
    __in PWDF_INTERRUPT_CONFIG Configuration
    )
/*++

Routine Description:
    Makes sure the specified resource is valid for an interrupt resource.

Arguments:
    CmIntResourceRaw - the raw interrupt resource to validate.
    CmIntResource - the translated interrupt resource to validate.

Return Value:
    STATUS_SUCCESS if resource is valid or
    NTSTATUS error.

  --*/
{
    NTSTATUS            status;
    PLIST_ENTRY         le;
    FxInterrupt*        interrupt;
    ULONG               messageCount;
    PFX_DRIVER_GLOBALS  fxDriverGlobals;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmIntResourceRaw;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmIntResource;

    cmIntResourceRaw = CmIntResourceRaw;
    cmIntResource = CmIntResource;
    fxDriverGlobals = GetDriverGlobals();

    //
    // Get the real descriptor not the copy.
    //
    status = ValidateCmResource(&cmIntResourceRaw, &cmIntResource);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    //
    // Make sure this is an interrupt resource.
    //
    if (cmIntResourceRaw->Type != CmResourceTypeInterrupt) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "The raw PCM_PARTIAL_RESOURCE_DESCRIPTOR 0x%p is not an "
            "interrupt resource, WDFDEVICE 0x%p, %!STATUS!",
            CmIntResourceRaw, m_Device->GetHandle(), status);
        FxVerifierDbgBreakPoint(fxDriverGlobals);
        goto Done;
    }

    if (cmIntResource->Type != CmResourceTypeInterrupt) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "The translated PCM_PARTIAL_RESOURCE_DESCRIPTOR 0x%p is not an "
            "interrupt resource, WDFDEVICE 0x%p, %!STATUS!",
            CmIntResource, m_Device->GetHandle(), status);
        FxVerifierDbgBreakPoint(fxDriverGlobals);
        goto Done;
    }

    //
    // Make sure resource was not claimed by another interrupt.
    // Multi-message MSI 2.2 interrupts: allowed to reuse claimed resources.
    //     Driver must create them sequentially.
    // Line-based interrupts: allowed to reuse claimed resources.
    //     Driver can create them out-of-order.
    // Other MSI: not allowed to reuse claimed resources.
    //
    messageCount = 0;

    for (le = m_InterruptListHead.Flink;
         le != &m_InterruptListHead;
         le = le->Flink) {

        interrupt = CONTAINING_RECORD(le, FxInterrupt, m_PnpList);

        if (cmIntResource != interrupt->GetResources()) {
            //
            // Multi-message MSI 2.2 interrupts must be sequential.
            //
            if (messageCount != 0) {
                status = STATUS_INVALID_PARAMETER;
                DoTraceLevelMessage(
                    fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                    "Multi-message MSI 2.2 interrupts must be created "
                    "sequentially, WDFDEVICE 0x%p, %!STATUS!",
                    m_Device->GetHandle(), status);
                FxVerifierDbgBreakPoint(fxDriverGlobals);
                goto Done;
            }

            continue;
        }

        if (interrupt->IsWakeCapable() &&
            Configuration->PassiveHandling) {
            DoTraceLevelMessage(
                fxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGPNP,
                "The PCM_PARTIAL_RESOURCE_DESCRIPTOR 0x%p was already used "
                "to create a wakable interrupt 0x%p, WDFDEVICE 0x%p and "
                "any functional interrupt being shared with wakable interrupt "
                "can not use passive level handling",
                CmIntResource, interrupt->GetHandle(),
                m_Device->GetHandle());
            status = STATUS_INVALID_PARAMETER;
            goto Done;
        }

        if (interrupt->IsPassiveHandling() &&
            Configuration->CanWakeDevice) {
            DoTraceLevelMessage(
                fxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGPNP,
                "The PCM_PARTIAL_RESOURCE_DESCRIPTOR 0x%p was already used "
                "to create a passive level interrupt 0x%p, WDFDEVICE 0x%p and "
                "is now being used to create a wakable interrupt. A functional "
                "passive level interrupt can not be shared with wakable interrupt",
                CmIntResource, interrupt->GetHandle(),
                m_Device->GetHandle());
            status = STATUS_INVALID_PARAMETER;
            goto Done;
        }

        //
        // Check for multi-message MSI 2.2 interrupts. These are allowed
        // to use the same resource.
        // We allow line based interrupts to reuse claimed resources.
        //
        if (FxInterrupt::_IsMessageInterrupt(cmIntResource->Flags) == FALSE) {
            DoTraceLevelMessage(
                fxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGPNP,
                "The PCM_PARTIAL_RESOURCE_DESCRIPTOR 0x%p was already used "
                "to create interrupt 0x%p, WDFDEVICE 0x%p",
                CmIntResource, interrupt->GetHandle(),
                m_Device->GetHandle());
            continue;
        }

        //
        // Only allow the correct # of messages.
        //
        messageCount++;
        if (messageCount >
            cmIntResourceRaw->u.MessageInterrupt.Raw.MessageCount) {

            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "All the MSI 2.2 interrupts for "
                "PCM_PARTIAL_RESOURCE_DESCRIPTOR 0x%p are already created, "
                "WDFDEVICE 0x%p, %!STATUS!",
                CmIntResource, m_Device->GetHandle(), status);
            FxVerifierDbgBreakPoint(fxDriverGlobals);
            goto Done;
        }
    }

    status = STATUS_SUCCESS;

Done:
    return status;
}

#define RESTART_START_ACHIEVED_NAME L"StartAchieved"
#define RESTART_START_TIME_NAME     L"StartTime"
#define RESTART_COUNT_NAME          L"Count"

const PWCHAR FxPkgPnp::m_RestartStartAchievedName = RESTART_START_ACHIEVED_NAME;
const PWCHAR FxPkgPnp::m_RestartStartTimeName = RESTART_START_TIME_NAME;
const PWCHAR FxPkgPnp::m_RestartCountName = RESTART_COUNT_NAME;

const ULONG FxPkgPnp::m_RestartTimePeriodMaximum = 60;
const ULONG FxPkgPnp::m_RestartCountMaximum = 5;

BOOLEAN
FxPkgPnp::PnpIncrementRestartCountLogic(
    _In_ HANDLE RestartKey,
    _In_ BOOLEAN CreatedNewKey
    )
/*++

Routine Description:
    This routine determines if this device should ask the bus driver to
    reenumerate the device.   This is determined by how many times the entire
    stack has asked for a restart within a given period.  This is stack wide
    because the settings are stored in a key in the device node itself (which all
    devices share).

    The period and number of times a restart are attempted are defined as constants
    (m_RestartTimePeriodMaximum, m_RestartCountMaximum)in this class.   They are
    current defined as a period of 60 seconds and a restart max count of 5.

    The settings are stored in a volatile key so that they do not persist across
    machine reboots.  Persisting across reboots makes no sense if we restrict the
    number of restarts w/in a period.

    The rules are as follows
    1)  if the key does not exist, treat this as the beginning of the period
        and ask for a reenumeration
    2)  if the key exists
        a)  if the beginning of the period and the restart count cannot be read
            do not ask for a reenumeration
        b)  if the beginning of the period is after the current time, either the
            current tick count has wrapped or the key has somehow survived a
            reboot.  Either way, treat this as a reset of the period and ask
            for a reenumeration
        c)  if the current time is after the period start time and within the
            restart period, increment the restart count.  if the count is <=
            the max restart count, ask for a reenumeration.  If it exceeds the
            max, do not ask for a reenumeration.
        d)  if the current time is after the period stat time and exceeds the
            maximum period, and if the device as reached the started state,
            reset the period, count, and started state, then ask for a
            reenumeration.

Considerations:
    There is a reenumeration loop that a device can get caught in. If a device
    takes more than m_RestartTimePeriodMaximum to fail m_RestartCountMaximum
    times then the device will be caught in this loop. If it is failing on the
    way to PnpEventStarted then the device will likely cause a 9F bugcheck.
    This is because they hold a power lock while in this loop. If the device
    fails after PnpEventStarted then pnp can progress and the device can loop
    here indefinitely. We have shipped with this behavior for several releases,
    so we are hesitant to completely change this behavior. The concern is that
    a device out there relies on this behavior.

Arguments:
    RestartKey - opened handle to the Restart registry key
    CreatedNewKey - TRUE if the Restart key was created just now

Return Value:
    TRUE if a restart should be requested.

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG count;
    LARGE_INTEGER currentTickCount, startTickCount;
    BOOLEAN started, writeTick, writeCount, writeStarted;

    DECLARE_CONST_UNICODE_STRING(valueNameStartTime, RESTART_START_TIME_NAME);
    DECLARE_CONST_UNICODE_STRING(valueNameCount, RESTART_COUNT_NAME);
    DECLARE_CONST_UNICODE_STRING(valueNameStartAchieved, RESTART_START_ACHIEVED_NAME);

    count = 0;
    started = FALSE;
    writeTick = FALSE;
    writeCount = FALSE;
    writeStarted = FALSE;

    Mx::MxQueryTickCount(&currentTickCount);

    started = m_AchievedStart;
    if (started) {
        //
        // Save the fact the driver started without failing
        //
        writeStarted = TRUE;
    }


    //
    // If the key was created right now, there is nothing to check, just write out
    // the data.
    //
    if (CreatedNewKey) {
        writeTick = TRUE;
        writeCount = TRUE;

        //
        // First restart
        //
        count = 1;
    }
    else {
        ULONG length, type;

        //
        // First try to get the start time of when we first attempted a restart
        //
        status = FxRegKey::_QueryValue(GetDriverGlobals(),
                                       RestartKey,
                                       &valueNameStartTime,
                                       sizeof(startTickCount.QuadPart),
                                       &startTickCount.QuadPart,
                                       &length,
                                       &type);

        if (NT_SUCCESS(status) &&
            length == sizeof(startTickCount.QuadPart) && type == REG_BINARY) {

            //
            // Now try to get the last restart count
            //
            status = FxRegKey::_QueryULong(RestartKey,
                                           &valueNameCount,
                                           &count);

            if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
                //
                // We read the start time, but not the count.  Assume there was
                // at least one previous restart.
                //
                count = 1;
                status = STATUS_SUCCESS;
            }
        }

        if (NT_SUCCESS(status)) {
            if (currentTickCount.QuadPart < startTickCount.QuadPart) {
                //
                // Somehow the key survived a reboot or the clock overflowed
                // and the current time is less then the last time we started
                // timing restarts.  Either way, just treat this as the first
                // time we are restarting.
                //
                writeTick = TRUE;
                writeCount = TRUE;
                count = 1;
            }
            else {
                LONGLONG delta;

                //
                // Compute the difference in time in 100 ns units
                //
                delta = (currentTickCount.QuadPart - startTickCount.QuadPart) *
                         Mx::MxQueryTimeIncrement();

                if (delta <= WDF_ABS_TIMEOUT_IN_SEC(m_RestartTimePeriodMaximum)) {
                    //
                    // We are within the time limit, see if we are within the
                    // count limit
                    count++;

                    //
                    // The count starts at one, so include the maximum in the
                    // compare.
                    //
                    if (count <= m_RestartCountMaximum) {
                        writeCount = TRUE;
                    }
                    else {
                        //
                        // Exceeded the restart count, do not attempt to restart
                        // the device.
                        //
                        status = STATUS_UNSUCCESSFUL;
                    }
                }
                else {
                    if (started == FALSE) {
                        ULONG length, type, value;
                        status = FxRegKey::_QueryValue(GetDriverGlobals(),
                                                       RestartKey,
                                                       &valueNameStartAchieved,
                                                       sizeof(value),
                                                       &value,
                                                       &length,
                                                       &type);
                        if (!NT_SUCCESS(status) || length != sizeof(value) ||
                            type != REG_DWORD) {
                            value = 0;
                        }
                        started = value != 0;
                        status = STATUS_SUCCESS;
                    }

                    if (started) {
                        //
                        // Exceeded the time limit.  This is treated as a reset of
                        // the time limit, so we will try to restart and reset the
                        // start time and restart count.
                        //
                        writeTick = TRUE;
                        writeCount = TRUE;
                        count = 1;

                        //
                        // Erase the fact the driver once started and
                        // make it do it again to get another 5 attempts to
                        // restart.
                        //
                        writeStarted = TRUE;
                        started = FALSE;
                    }
                    else {
                        //
                        // Device never started
                        //
                        status = STATUS_UNSUCCESSFUL;
                    }
                }
            }
        }
    }

    if (writeTick) {
        //
        // Write out the time and the count
        //
        NTSTATUS status2;
        status2 = FxRegKey::_SetValue(RestartKey,
                                     (PUNICODE_STRING)&valueNameStartTime,
                                     REG_BINARY,
                                     &currentTickCount.QuadPart,
                                     sizeof(currentTickCount.QuadPart));
        //
        // Don't let status report success if it was an error prior to _SetValue
        //
        if(NT_SUCCESS(status)) {
            status = status2;
        }
    }

    if (NT_SUCCESS(status) && writeCount) {
        status = FxRegKey::_SetValue(RestartKey,
                                     (PUNICODE_STRING)&valueNameCount,
                                     REG_DWORD,
                                     &count,
                                     sizeof(count));
    }

    if (writeStarted) {
        NTSTATUS status2;
        DWORD value = started;
        status2 = FxRegKey::_SetValue(RestartKey,
                                     (PUNICODE_STRING)&valueNameStartAchieved,
                                     REG_DWORD,
                                     &value,
                                     sizeof(value));
        //
        // Don't let status report success if it was an error prior to _SetValue
        //
        if(NT_SUCCESS(status)) {
            status = status2;
        }
    }

    return NT_SUCCESS(status) ? TRUE : FALSE;
}

