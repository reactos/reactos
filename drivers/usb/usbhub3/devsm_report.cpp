/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Device state machine, reporting the device to PnP and the
 *              device that failed enumeration
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include "usbhub3.h"

/* GLOBALS ********************************************************************/

static const DsmEvent ReportLpmDiscards[] = { DsmEvent::LpmSettingChanged, DsmEvent::Count };
static const DsmEvent MissingDiscards[] = { DsmEvent::PdoReportedMissing, DsmEvent::Count };

static const DsmEvent PortOffDiscards[] =
{
    DsmEvent::PortResetDone, DsmEvent::PortNowDisabled, DsmEvent::NoPingResponse,
    DsmEvent::Count
};

/* Adds the device to the hub's child list and creates its PDO */
const DeviceMachine::State DeviceMachine::ReportingToPnp =
    { NULL, "ReportingToPnp", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnReportingToPnp, &DeviceMachine::EnterReportingToPnp, ReportLpmDiscards };

/* Creates a PDO for a device that failed enumeration so PnP shows it as failed */
const DeviceMachine::State DeviceMachine::ReportingUnknown =
    { NULL, "ReportingUnknown", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnReportingUnknown, &DeviceMachine::EnterReportingUnknown, ReportLpmDiscards };

/* The failed device's PDO exists; m_FailedPoweredUp once PnP powered it up */
const DeviceMachine::State DeviceMachine::FailedDevice =
    { NULL, "FailedDevice", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnFailedDevice, &DeviceMachine::EnterFailedDevice };

/* The failed device detached, waiting for PnP to clean up its PDO */
const DeviceMachine::State DeviceMachine::FailedDeviceGone =
    { NULL, "FailedDeviceGone", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnFailedDeviceGone, &DeviceMachine::EnterFailedDeviceGone, MissingDiscards };

/* Sub machine of FailedDevice: keeps hub and PnP events balanced */
const DeviceMachine::State DeviceMachine::FailedDeviceEvents =
    { NULL, "FailedDeviceEvents", SM_STATE_YIELDS_TO_CALLER,
      &DeviceMachine::OnFailedDeviceEvents, NULL, ReportLpmDiscards };

const DeviceMachine::State DeviceMachine::FailedHubSuspended =
    { NULL, "FailedHubSuspended", SM_STATE_YIELDS_TO_CALLER,
      &DeviceMachine::OnFailedHubSuspended, &DeviceMachine::EnterFailedHubSuspended };

/* Sub machine for a device whose port is off: answers everything without touching the device */
const DeviceMachine::State DeviceMachine::PortOffEvents =
    { NULL, "PortOffEvents", SM_STATE_YIELDS_TO_CALLER,
      &DeviceMachine::OnPortOffEvents, NULL, PortOffDiscards };

/* FUNCTIONS ******************************************************************/

/* REPORTING TO PNP ***********************************************************/

SM_RESULT
DeviceMachine::EnterReportingToPnp()
{
    StartDuplicateRetries();
    return CallAtPassive(&DeviceMachine::ReportAddChild);
}

SM_RESULT
DeviceMachine::OnReportingToPnp(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
            return SmTransition(&StoppedAddressed);

        case DsmEvent::PortDetached:
            return DisableAndDelete(&DeviceMachine::EndAfterDetach);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ReportAddChild()
{
    switch (AddChildToHub())
    {
        case DsmAddChildDone:
            return ReportCreatePdo();

        case DsmAddChildDuplicate:
            return Delay(DsmDelayDuplicate, &DeviceMachine::ReportDuplicateWaitEnded);

        default:
            if (!DuplicateRetriesUsedUp())
                return Delay(DsmDelayDuplicate, &DeviceMachine::ReportDuplicateWaitEnded);

            DropSerialNumber();
            return ReportCreatePdo();
    }
}

SM_RESULT
DeviceMachine::ReportCreatePdo()
{
    if (CreatePdo())
        return EndWith(DsmEvent::Succeeded);

    AskPortCycle();
    ForgetChild();
    return Wait(&IgnoringHub, &DeviceMachine::EndOnDetach);
}

/* Another device with the same serial number is still in the list */
SM_RESULT
DeviceMachine::ReportDuplicateWaitEnded(
    _In_ DsmEvent Event)
{
    BOOLEAN stopped;

    switch (Event)
    {
        case DsmEvent::HubStopping:
        case DsmEvent::HubSuspending:
            stopped = StopTimer();
            AskPortCycle();
            if (stopped)
                return Wait(&AwaitingPort, &DeviceMachine::EndOnDetach);

            return Wait(&AwaitingPort, &DeviceMachine::ReportCycledWithTimer);

        case DsmEvent::TimerFired:
            return AtPassive(&DeviceMachine::ReportAddChild);

        case DsmEvent::PortDetached:
            if (StopTimer())
                return EndWith(DsmEvent::PortDetached);

            return Wait(&TimerAfterDetach, &DeviceMachine::EndDetachedOnTimer);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ReportCycledWithTimer(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            return Wait(&TimerAfterDetach, &DeviceMachine::EndDetachedOnTimer);

        case DsmEvent::TimerFired:
            return Wait(&AwaitingPort, &DeviceMachine::EndOnDetach);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EndOnDetach(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::PortDetached)
        return SmUnhandled();

    return EndWith(DsmEvent::PortDetached);
}

/* FAILED DEVICE **************************************************************/

SM_RESULT
DeviceMachine::EnterReportingUnknown()
{
    return CallAtPassive(&DeviceMachine::ReportUnknownPdo);
}

SM_RESULT
DeviceMachine::ReportUnknownPdo()
{
    if (CreatePlaceholderPdo())
    {
        ReportEnumFailure();
        return EndWith(DsmEvent::Succeeded);
    }

    AskPortCycle();
    return Wait(&IgnoringHub, &DeviceMachine::EndOnDetach);
}

SM_RESULT
DeviceMachine::OnReportingUnknown(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
            m_FailedPoweredUp = FALSE;
            return SmTransition(&FailedDevice);

        case DsmEvent::PortDetached:
            return EndAfterDetach();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnterFailedDevice()
{
    return SmCall(&FailedDeviceEvents);
}

SM_RESULT
DeviceMachine::OnFailedDevice(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PdoCleanup:
            SignalPnpWaiter();
            return SmTransition(&RemovedPortOn);

        case DsmEvent::PdoPowerUp:
            if (m_FailedPoweredUp)
                AskPortCycle();

            SignalPnpWaiter();
            m_FailedPoweredUp = TRUE;
            return SmTransition(&FailedDevice);

        case DsmEvent::PortDetached:
            return SmTransition(&FailedDeviceGone);

        case DsmEvent::PortNowEnabledOnReconnect:
            return Wait(&DisablingPort, &DeviceMachine::FailedPortDisabledAgain);

        default:
            return SmUnhandled();
    }
}

/* A failed device came back on reconnect; keep its port off */
SM_RESULT
DeviceMachine::FailedPortDisabledAgain(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDisableDone:
        case DsmEvent::PortFault:
            return SmTransition(&FailedDevice);

        case DsmEvent::PortDetached:
            if (m_FailedPoweredUp)
                return SmUnhandled();

            return SmTransition(&FailedDeviceGone);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnterFailedDeviceGone()
{
    ReportDeviceMissing();
    NotifyPortDetached();
    return SmCall(&PortOffEvents);
}

SM_RESULT
DeviceMachine::OnFailedDeviceGone(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::PdoCleanup)
        return SmUnhandled();

    SignalPnpWaiter();
    return LeaveHub();
}

/* Unregister from the hub and wait to be deleted */
SM_RESULT
DeviceMachine::LeaveHub()
{
    UnregisterFromHub();
    DereferencePort();
    DiscardHubEvents();
    return SmTransition(&AwaitingDelete);
}

SM_RESULT
DeviceMachine::OnFailedDeviceEvents(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PdoPreStart:
            AckPreStart();
            return SmHandled();

        case DsmEvent::HubGetDescriptor:
            FailHubDescriptorRequest();
            return SmHandled();

        case DsmEvent::PortNowEnabled:
        case DsmEvent::PortNowDisabled:
        case DsmEvent::PortNowSuspended:
            return SmHandled();

        case DsmEvent::PdoPowerDownFinal:
        case DsmEvent::PdoInstallMsOsExt:
        case DsmEvent::PdoPowerDown:
            SignalPnpWaiter();
            return SmTransition(&FailedDeviceEvents);

        case DsmEvent::HubResumedInS0:
        case DsmEvent::HubStarted:
            ReferenceDevicePower();
            return SmTransition(&FailedDeviceEvents);

        case DsmEvent::HubSuspending:
            return Wait(&DisablingPort, &DeviceMachine::FailedPortOffForHub);

        case DsmEvent::HubStoppingAfterSuspend:
            ConfirmStopWhileSuspended();
            return SmTransition(&FailedDeviceEvents);

        case DsmEvent::HubStopping:
            DereferenceDevicePower();
            return SmTransition(&FailedDeviceEvents);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::FailedPortOffForHub(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            DereferenceDevicePower();
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::PortDisableDone:
        case DsmEvent::PortFault:
            return SmTransition(&FailedHubSuspended);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnterFailedHubSuspended()
{
    DereferenceDevicePower();
    return SmCall(&HubSuspended);
}

SM_RESULT
DeviceMachine::OnFailedHubSuspended(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::HubStoppedHoldingReference:
            DereferenceDevicePower();
            return SmTransition(&FailedDeviceEvents);

        case DsmEvent::NeedsReenumeration:
        case DsmEvent::SuspendedAfterHubResume:
        case DsmEvent::ResumedWithHub:
            return SmTransition(&FailedDeviceEvents);

        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::HubStoppingAfterSuspend:
            ConfirmStopWhileSuspended();
            return SmTransition(&FailedDeviceEvents);

        default:
            return SmUnhandled();
    }
}

/* PORT OFF EVENTS ************************************************************/

SM_RESULT
DeviceMachine::OnPortOffEvents(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ClientCyclePort:
        case DsmEvent::PortDisableDone:
        case DsmEvent::PortResetTimeout:
        case DsmEvent::PortResetAbortedForSuspend:
        case DsmEvent::PortNowEnabled:
        case DsmEvent::PortSuspendDone:
        case DsmEvent::LpmSettingChanged:
        case DsmEvent::PortFault:
        case DsmEvent::PortNowSuspended:
            return SmHandled();

        case DsmEvent::ClientResetPipe:
        case DsmEvent::ClientSetInterface:
        case DsmEvent::ClientSyncResetPipe:
        case DsmEvent::ClientResetDevice:
        case DsmEvent::ClientSelectConfig:
        case DsmEvent::ClientClearStall:
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
        case DsmEvent::HubStarted:
        case DsmEvent::HubResumedWithReset:
        case DsmEvent::HubResumedInS0:
            ReferenceDevicePower();
            return SmTransition(&PortOffEvents);

        case DsmEvent::PdoPowerDownFinal:
        case DsmEvent::PdoPowerUp:
        case DsmEvent::PdoInstallMsOsExt:
        case DsmEvent::PdoPowerDown:
            SignalPnpPowerFailure();
            return SmTransition(&PortOffEvents);

        case DsmEvent::HubStopping:
        case DsmEvent::HubSuspending:
            DereferenceDevicePower();
            return SmTransition(&PortOffEvents);

        case DsmEvent::HubStoppingAfterSuspend:
            ConfirmStopWhileSuspended();
            return SmTransition(&PortOffEvents);

        case DsmEvent::ClientStreams:
            return ForwardStreams(&PortOffEvents);

        default:
            return SmUnhandled();
    }
}
