/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Device state machine, detach and removal
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include "usbhub3.h"

/* GLOBALS ********************************************************************/

static const DsmEvent DetachedEventDiscards[] =
{
    DsmEvent::PortResetAbortedForSuspend, DsmEvent::PortDisableDone, DsmEvent::PortSuspendDone,
    DsmEvent::PortResetDone, DsmEvent::Count
};

/* Reported missing to PnP and waiting for the PDO removal; the configured flavor disables its endpoints first */
const DeviceMachine::State DeviceMachine::DetachedReported =
    { NULL, "DetachedReported", SM_STATE_TAKES_REQUESTS | SM_STATE_NEEDS_PASSIVE,
      &DeviceMachine::OnDetachedReported, &DeviceMachine::EnterDetachedReported };

const DeviceMachine::State DeviceMachine::DetachedReportedConfigured =
    { NULL, "DetachedReportedConfigured", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnDetachedReported, &DeviceMachine::EnterDetachedReportedConfigured };

/* PnP saw the device missing; the child is off the hub's list */
const DeviceMachine::State DeviceMachine::DetachedMissing =
    { NULL, "DetachedMissing", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnDetachedMissing, &DeviceMachine::EnterDetachedMissing };

/* Device disabled in the controller, waiting for PnP to remove the PDO */
const DeviceMachine::State DeviceMachine::AwaitingRemove =
    { NULL, "AwaitingRemove", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnAwaitingRemove, &DeviceMachine::EnterWithPortOff };

/* Port cycled after a failure, device still in the controller; waiting for detach or remove */
const DeviceMachine::State DeviceMachine::AwaitingDetachOrRemove =
    { NULL, "AwaitingDetachOrRemove", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnAwaitingDetachOrRemove, &DeviceMachine::EnterDetachedEvents };

/* Device disabled in the controller and the port is off */
const DeviceMachine::State DeviceMachine::ParkedPortOff =
    { NULL, "ParkedPortOff", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnParkedPortOff, &DeviceMachine::EnterWithPortOff };

const DeviceMachine::State DeviceMachine::ParkedPortOn =
    { NULL, "ParkedPortOn", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnParkedPortOn, &DeviceMachine::EnterWithPortOn };

/* PDO removed and the device deleted, waiting for the detach */
const DeviceMachine::State DeviceMachine::RemovedPortOff =
    { NULL, "RemovedPortOff", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnRemovedPortOff, &DeviceMachine::EnterWithPortOff };

const DeviceMachine::State DeviceMachine::RemovedPortOn =
    { NULL, "RemovedPortOn", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnRemovedPortOn, &DeviceMachine::EnterWithPortOn };

/* Sub machine: the device is detached or about to be, answer events without touching it */
const DeviceMachine::State DeviceMachine::DetachedEvents =
    { NULL, "DetachedEvents", SM_STATE_YIELDS_TO_CALLER,
      &DeviceMachine::OnDetachedEvents, NULL, DetachedEventDiscards };

/* Sub machine: the device is gone from the controller, its port still on */
const DeviceMachine::State DeviceMachine::PortOnEvents =
    { NULL, "PortOnEvents", SM_STATE_YIELDS_TO_CALLER, &DeviceMachine::OnPortOnEvents, NULL };

/* FUNCTIONS ******************************************************************/

SM_RESULT
DeviceMachine::EnterWithPortOff()
{
    return SmCall(&PortOffEvents);
}

SM_RESULT
DeviceMachine::EnterWithPortOn()
{
    return SmCall(&PortOnEvents);
}

SM_RESULT
DeviceMachine::EnterDetachedEvents()
{
    return SmCall(&DetachedEvents);
}

/* DETACHED WITH A PDO ********************************************************/

/* Device left without its configuration programmed in the controller */
SM_RESULT
DeviceMachine::DetachUnconfigured()
{
    m_EndpointsConfigured = FALSE;
    return SmTransition(&DetachedReported);
}

/* Device left with its configuration programmed in the controller */
SM_RESULT
DeviceMachine::DetachConfigured()
{
    m_EndpointsConfigured = TRUE;
    return SmTransition(&DetachedReportedConfigured);
}

SM_RESULT
DeviceMachine::EnterDetachedReported()
{
    m_ChildForgotten = FALSE;
    ReportDeviceMissing();
    MarkDisconnected();
    NotifyDisconnected();
    NotifyPortDetached();
    return SmCall(&DetachedEvents);
}

SM_RESULT
DeviceMachine::EnterDetachedReportedConfigured()
{
    m_ChildForgotten = FALSE;
    ReportDeviceMissing();
    MarkDisconnected();
    NotifyPortDetached();
    return SmCall(&DetachedEvents);
}

SM_RESULT
DeviceMachine::OnDetachedReported(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PdoReportedMissing:
            return SmTransition(&DetachedMissing);

        case DsmEvent::PdoCleanup:
            ForgetChild();
            DereferencePort();
            return DisableForRemoval(&DeviceMachine::RemovalDeleteAfterLeaving);

        case DsmEvent::PdoPowerDownFinal:
            return DisableForRemoval(&DeviceMachine::RemovalPowerDownDone);

        case DsmEvent::HubStoppingAfterSuspend:
            return DisableForRemoval(&DeviceMachine::RemovalAckHubStop);

        case DsmEvent::HubStopping:
            return DisableForRemoval(&DeviceMachine::RemovalDropPowerForHubStop);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnterDetachedMissing()
{
    m_ChildForgotten = TRUE;
    ForgetChild();
    DereferencePort();
    return SmCall(&DetachedEvents);
}

SM_RESULT
DeviceMachine::OnDetachedMissing(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::HubStopping:
        case DsmEvent::HubStoppingAfterSuspend:
            return DisableForRemoval(&DeviceMachine::RemovalLeaveHub);

        case DsmEvent::PdoCleanup:
            return DisableForRemoval(&DeviceMachine::RemovalDeleteAfterLeaving);

        case DsmEvent::PdoPowerDownFinal:
            return DisableForRemoval(&DeviceMachine::RemovalPowerDownDone);

        default:
            return SmUnhandled();
    }
}

/* Disable the endpoints if they are programmed, then the device, then run After */
SM_RESULT
DeviceMachine::DisableForRemoval(
    _In_ DSM_THEN After)
{
    m_AfterDisable = After;
    if (m_EndpointsConfigured)
        return Wait(&DisablingEndpoints, &DeviceMachine::RemovalEndpointsDisabled);

    return Wait(&DisablingDevice, &DeviceMachine::RemovalDeviceDisabled);
}

SM_RESULT
DeviceMachine::RemovalEndpointsDisabled(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return Wait(&DisablingDevice, &DeviceMachine::RemovalDeviceDisabled);
}

SM_RESULT
DeviceMachine::RemovalDeviceDisabled(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return (this->*m_AfterDisable)();
}

SM_RESULT
DeviceMachine::RemovalDeleteAfterLeaving()
{
    UnregisterFromHub();
    DiscardHubEvents();
    return AtPassive(&DeviceMachine::RemovalDeleteAndFinish);
}

/* Free everything the controller holds for the device and wait to be deleted */
SM_RESULT
DeviceMachine::RemovalDeleteAndFinish()
{
    DeleteConfigEndpoints();
    DeleteDefaultEndpoint();
    DeleteDeviceInController();
    SignalPnpWaiter();
    return SmTransition(&AwaitingDelete);
}

SM_RESULT
DeviceMachine::RemovalPowerDownDone()
{
    SignalPnpWaiter();
    return RemovalLeaveHub();
}

SM_RESULT
DeviceMachine::RemovalLeaveHub()
{
    UnregisterFromHub();
    DiscardHubEvents();
    m_Unregistered = TRUE;
    return SmTransition(&AwaitingRemove);
}

SM_RESULT
DeviceMachine::RemovalAckHubStop()
{
    ConfirmStopWhileSuspended();
    return RemovalHubStopped();
}

SM_RESULT
DeviceMachine::RemovalDropPowerForHubStop()
{
    DereferenceDevicePower();
    return RemovalHubStopped();
}

SM_RESULT
DeviceMachine::RemovalHubStopped()
{
    m_Unregistered = FALSE;
    m_ChildForgotten = FALSE;
    return SmTransition(&AwaitingRemove);
}

/*
 * PDO still to be removed. m_Unregistered and m_ChildForgotten say how much
 * of leaving the hub is already done.
 */
SM_RESULT
DeviceMachine::OnAwaitingRemove(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PdoReportedMissing:
            if (m_ChildForgotten)
                return SmUnhandled();

            ForgetChild();
            DereferencePort();
            m_ChildForgotten = TRUE;
            if (!m_Unregistered)
            {
                UnregisterFromHub();
                DiscardHubEvents();
                m_Unregistered = TRUE;
            }

            return SmTransition(&AwaitingRemove);

        case DsmEvent::PdoCleanup:
            if (!m_ChildForgotten)
            {
                ForgetChild();
                DereferencePort();
            }

            if (!m_Unregistered)
            {
                UnregisterFromHub();
                DiscardHubEvents();
            }

            return AtPassive(&DeviceMachine::RemovalDeleteAndFinish);

        default:
            return SmUnhandled();
    }
}

/* Report the device missing from a state where the port is off */
SM_RESULT
DeviceMachine::ReportMissingAndLeave()
{
    ReportDeviceMissing();
    MarkDisconnected();
    NotifyDisconnected();
    NotifyPortDetached();
    UnregisterFromHub();
    DiscardHubEvents();
    m_Unregistered = TRUE;
    m_ChildForgotten = FALSE;
    return SmTransition(&AwaitingRemove);
}

/* PORT CYCLED, DEVICE STILL PROGRAMMED ***************************************/

SM_RESULT
DeviceMachine::OnAwaitingDetachOrRemove(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PdoCleanup:
            return DisableForRemoval(&DeviceMachine::RemovalDeleteForgetAndPark);

        case DsmEvent::PortDetached:
            if (m_EndpointsConfigured)
                return AtPassive(&DeviceMachine::RemovalNotifyThenReport);

            return DetachUnconfigured();

        case DsmEvent::HubStopping:
            return DisableForRemoval(&DeviceMachine::RemovalDropPowerAndPark);

        case DsmEvent::PdoPowerDownFinal:
            SignalPnpWaiter();
            return SmTransition(&AwaitingDetachOrRemove);

        case DsmEvent::HubStoppingAfterSuspend:
            return DisableForRemoval(&DeviceMachine::RemovalAckAndPark);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::RemovalNotifyThenReport()
{
    NotifyDisconnected();
    return DetachConfigured();
}

SM_RESULT
DeviceMachine::RemovalDeleteForgetAndPark()
{
    return AtPassive(&DeviceMachine::RemovalDeleteForget);
}

/* Free the controller's copy of the device, forget the child, tell PnP; port stays off */
SM_RESULT
DeviceMachine::RemovalDeleteForget()
{
    DeleteConfigEndpoints();
    DeleteDefaultEndpoint();
    DeleteDeviceInController();
    ForgetChild();
    SignalPnpWaiter();
    return SmTransition(&RemovedPortOff);
}

SM_RESULT
DeviceMachine::RemovalDropPowerAndPark()
{
    DereferenceDevicePower();
    m_PortMayResume = TRUE;
    return SmTransition(&ParkedPortOff);
}

SM_RESULT
DeviceMachine::RemovalAckAndPark()
{
    ConfirmStopWhileSuspended();
    m_PortMayResume = TRUE;
    return SmTransition(&ParkedPortOff);
}

/* PARKED AND REMOVED *********************************************************/

SM_RESULT
DeviceMachine::OnParkedPortOff(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            return AtPassive(&DeviceMachine::ReportMissingAndLeave);

        case DsmEvent::PdoCleanup:
            return AtPassive(&DeviceMachine::RemovalDeleteForget);

        case DsmEvent::PortResumeDone:
        case DsmEvent::PortNowEnabledOnReconnect:
            if (!m_PortMayResume)
                return SmUnhandled();

            return SmTransition(&ParkedPortOn);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::OnParkedPortOn(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortOffForHubSuspend:
            m_PortMayResume = TRUE;
            return SmTransition(&ParkedPortOff);

        case DsmEvent::PdoCleanup:
            return AtPassive(&DeviceMachine::RemovalDeleteForgetPortOn);

        case DsmEvent::PortDetached:
            return AtPassive(&DeviceMachine::ReportMissingAndLeave);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::RemovalDeleteForgetPortOn()
{
    DeleteConfigEndpoints();
    DeleteDefaultEndpoint();
    DeleteDeviceInController();
    ForgetChild();
    SignalPnpWaiter();
    return SmTransition(&RemovedPortOn);
}

SM_RESULT
DeviceMachine::OnRemovedPortOff(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            return EndAfterDetach();

        case DsmEvent::PortNowEnabledOnReconnect:
        case DsmEvent::PortResumeDone:
            return SmTransition(&RemovedPortOn);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::OnRemovedPortOn(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortOffForHubSuspend:
            return SmTransition(&RemovedPortOff);

        case DsmEvent::PortDetached:
            return EndAfterDetach();

        default:
            return SmUnhandled();
    }
}

/* PDO cleanup while the port is being disabled */
SM_RESULT
DeviceMachine::CleanupPortDisabled(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            m_AfterDelete = &DeviceMachine::EndAfterDetach;
            return Wait(&DisablingDevice, &DeviceMachine::CleanupDeviceDisabled);

        case DsmEvent::PortDisableDone:
        case DsmEvent::PortFault:
            m_AfterDelete = &DeviceMachine::CleanupDone;
            return Wait(&DisablingDevice, &DeviceMachine::CleanupDeviceDisabled);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::CleanupDeviceDisabled(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return AtPassive(&DeviceMachine::CleanupDeleteAll);
}

SM_RESULT
DeviceMachine::CleanupDeleteAll()
{
    DeleteConfigEndpoints();
    DeleteDefaultEndpoint();
    DeleteDeviceInController();
    ForgetChild();
    SignalPnpWaiter();
    return (this->*m_AfterDelete)();
}

SM_RESULT
DeviceMachine::CleanupDone()
{
    return SmTransition(&RemovedPortOn);
}

/* SUB MACHINES ***************************************************************/

SM_RESULT
DeviceMachine::OnDetachedEvents(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ClientCyclePort:
        case DsmEvent::NoPingResponse:
        case DsmEvent::PortNowSuspended:
        case DsmEvent::PortNowEnabled:
        case DsmEvent::PortResetTimeout:
        case DsmEvent::LpmSettingChanged:
        case DsmEvent::PortFault:
        case DsmEvent::PortNowDisabled:
            return SmHandled();

        case DsmEvent::ClientSelectConfig:
        case DsmEvent::ClientResetDevice:
        case DsmEvent::ClientClearStall:
        case DsmEvent::ClientSyncResetPipe:
        case DsmEvent::ClientResetPipe:
        case DsmEvent::ClientSetInterface:
        case DsmEvent::ClientUnconfigure:
            FailClientRequest();
            return SmHandled();

        case DsmEvent::HubGetDescriptor:
            FailHubDescriptorRequest();
            return SmHandled();

        case DsmEvent::PdoPreStart:
            AckPreStart();
            return SmHandled();

        case DsmEvent::HubResumed:
        case DsmEvent::HubResumedWithReset:
        case DsmEvent::HubStarted:
        case DsmEvent::HubResumedInS0:
            ReferenceDevicePower();
            return SmTransition(&DetachedEvents);

        case DsmEvent::PdoPowerDown:
        case DsmEvent::PdoInstallMsOsExt:
        case DsmEvent::PdoPowerUp:
            SignalPnpWaiter();
            return SmTransition(&DetachedEvents);

        case DsmEvent::HubSuspending:
            DereferenceDevicePower();
            return SmTransition(&DetachedEvents);

        case DsmEvent::ClientStreams:
            return ForwardStreams(&DetachedEvents);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::OnPortOnEvents(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortNowDisabled:
        case DsmEvent::PortNowEnabled:
        case DsmEvent::PortResetTimeout:
        case DsmEvent::LpmSettingChanged:
        case DsmEvent::ClientCyclePort:
        case DsmEvent::PortDisableDone:
        case DsmEvent::PortFault:
        case DsmEvent::NoPingResponse:
        case DsmEvent::PortSuspendDone:
        case DsmEvent::PortResetAbortedForSuspend:
        case DsmEvent::PortNowEnabledOnReconnect:
        case DsmEvent::PortNowSuspended:
        case DsmEvent::PortResumeDone:
            return SmHandled();

        case DsmEvent::ClientUnconfigure:
        case DsmEvent::ClientSyncResetPipe:
        case DsmEvent::ClientSetInterface:
        case DsmEvent::ClientResetDevice:
        case DsmEvent::ClientResetPipe:
        case DsmEvent::ClientClearStall:
        case DsmEvent::ClientSelectConfig:
            FailClientRequest();
            return SmHandled();

        case DsmEvent::HubGetDescriptor:
            FailHubDescriptorRequest();
            return SmHandled();

        case DsmEvent::PdoPreStart:
            AckPreStart();
            return SmHandled();

        case DsmEvent::HubResumed:
        case DsmEvent::HubStarted:
        case DsmEvent::HubResumedWithReset:
        case DsmEvent::HubResumedInS0:
            ReferenceDevicePower();
            return SmTransition(&PortOnEvents);

        case DsmEvent::HubStoppingAfterSuspend:
            ConfirmStopWhileSuspended();
            return SmTransition(&PortOnEvents);

        case DsmEvent::PdoPowerDownFinal:
        case DsmEvent::PdoPowerUp:
        case DsmEvent::PdoInstallMsOsExt:
        case DsmEvent::PdoPowerDown:
            SignalPnpPowerFailure();
            return SmTransition(&PortOnEvents);

        case DsmEvent::HubSuspending:
            return Wait(&DisablingPort, &DeviceMachine::PortOnOffForHubSuspend);

        case DsmEvent::ClientStreams:
            return ForwardStreams(&PortOnEvents);

        case DsmEvent::HubStopping:
            DereferenceDevicePower();
            return SmTransition(&PortOnEvents);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::PortOnOffForHubSuspend(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::PortFault:
        case DsmEvent::PortDisableDone:
            DereferenceDevicePower();
            return EndWith(DsmEvent::PortOffForHubSuspend);

        default:
            return SmUnhandled();
    }
}
