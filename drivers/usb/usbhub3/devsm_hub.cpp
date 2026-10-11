/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Device state machine, the hub suspended sub machine
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include "usbhub3.h"

/* GLOBALS ********************************************************************/

/* The hub is suspended with the device's power reference dropped */
const DeviceMachine::State DeviceMachine::HubSuspended =
    { NULL, "HubSuspended", SM_STATE_YIELDS_TO_CALLER,
      &DeviceMachine::OnHubSuspended, NULL };

/* Same, but the port was found disabled or failed while the hub suspended */
const DeviceMachine::State DeviceMachine::HubSuspendedPortOff =
    { NULL, "HubSuspendedPortOff", SM_STATE_YIELDS_TO_CALLER,
      &DeviceMachine::OnHubSuspendedPortOff, NULL };

/* FUNCTIONS ******************************************************************/

SM_RESULT
DeviceMachine::OnHubSuspended(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::HubResumed:
            ReferenceDevicePower();
            return Wait(&AwaitingPort, &DeviceMachine::HubPortSynced);

        case DsmEvent::HubResumedWithReset:
            ReferenceDevicePower();
            return Wait(&AwaitingPort, &DeviceMachine::HubPortSyncedForReset);

        case DsmEvent::HubResumedInS0:
            ReferenceDevicePower();
            return EndWith(DsmEvent::SuspendedAfterHubResume);

        case DsmEvent::PortNowDisabled:
        case DsmEvent::PortNowEnabledOnReconnect:
            return Wait(&AwaitingPort, &DeviceMachine::HubResumeForReenumeration);

        case DsmEvent::PortNowSuspended:
            return Wait(&AwaitingPort, &DeviceMachine::HubResumeWithPortSuspended);

        case DsmEvent::PortNowEnabled:
            return Wait(&AwaitingPort, &DeviceMachine::HubResumeWithPortEnabled);

        case DsmEvent::PdoPowerDown:
            SignalPnpWaiter();
            return SmTransition(&HubSuspended);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::OnHubSuspendedPortOff(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::HubResumed:
        case DsmEvent::HubResumedWithReset:
            ReferenceDevicePower();
            return Wait(&AwaitingPort, &DeviceMachine::HubPortSyncedForReset);

        case DsmEvent::HubResumedInS0:
            ReferenceDevicePower();
            return EndWith(DsmEvent::NeedsReenumeration);

        case DsmEvent::PortNowEnabled:
        case DsmEvent::PortNowDisabled:
        case DsmEvent::PortNowSuspended:
        case DsmEvent::PortNowEnabledOnReconnect:
            return Wait(&AwaitingPort, &DeviceMachine::HubResumeForReenumeration);

        default:
            return SmUnhandled();
    }
}

/* The port changed while the hub was suspended, the device needs enumerating again */
SM_RESULT
DeviceMachine::HubResumeForReenumeration(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::HubResumed:
        case DsmEvent::HubResumedWithReset:
            ReferenceDevicePower();
            return EndWith(DsmEvent::NeedsReenumeration);

        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::HubResumeWithPortSuspended(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::HubResumed:
            ReferenceDevicePower();
            if (ResetOnResumeFromSx())
                return Wait(&ResumingPort, &DeviceMachine::HubPortResumedForReset);

            return EndWith(DsmEvent::SuspendedAfterHubResume);

        case DsmEvent::PortResumeDone:
            RequestPortResume();
            return Wait(&AwaitingPort, &DeviceMachine::HubResumeWithPortEnabled);

        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::HubResumedWithReset:
            ReferenceDevicePower();
            return Wait(&ResumingPort, &DeviceMachine::HubPortResumedForReset);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::HubResumeWithPortEnabled(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::HubResumed:
            ReferenceDevicePower();
            return HubEndResumedOrReenumerate();

        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::HubResumedWithReset:
            ReferenceDevicePower();
            return EndWith(DsmEvent::NeedsReenumeration);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::HubEndResumedOrReenumerate()
{
    if (ResetOnResumeFromSx())
        return EndWith(DsmEvent::NeedsReenumeration);

    return EndWith(DsmEvent::ResumedWithHub);
}

/* Hub resumed; the port machine reports where the port ended up */
SM_RESULT
DeviceMachine::HubPortSynced(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortNowDisabled:
        case DsmEvent::PortNowEnabledOnReconnect:
            return EndWith(DsmEvent::NeedsReenumeration);

        case DsmEvent::PortNowEnabled:
            return HubEndResumedOrReenumerate();

        case DsmEvent::PortNowSuspended:
            return EndWith(DsmEvent::SuspendedAfterHubResume);

        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::HubSuspending:
            return Wait(&AwaitingPort, &DeviceMachine::HubPortSyncedForSuspend);

        case DsmEvent::HubStopping:
            return Wait(&AwaitingPort, &DeviceMachine::HubPortSyncedForStop);

        default:
            return SmUnhandled();
    }
}

/* Hub resumed with a reset; the device is enumerated again unless the port is suspended */
SM_RESULT
DeviceMachine::HubPortSyncedForReset(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortNowEnabled:
        case DsmEvent::PortNowDisabled:
        case DsmEvent::PortNowEnabledOnReconnect:
            return EndWith(DsmEvent::NeedsReenumeration);

        case DsmEvent::PortNowSuspended:
            return Wait(&ResumingPort, &DeviceMachine::HubPortResumedForReset);

        case DsmEvent::HubSuspending:
            return Wait(&AwaitingPort, &DeviceMachine::HubPortSyncedForSuspend);

        case DsmEvent::HubStopping:
            return Wait(&AwaitingPort, &DeviceMachine::HubPortSyncedForStop);

        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        default:
            return SmUnhandled();
    }
}

/* The hub suspends again before the port machine caught up */
SM_RESULT
DeviceMachine::HubPortSyncedForSuspend(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortNowSuspended:
            DereferenceDevicePower();
            return SmTransition(&HubSuspended);

        case DsmEvent::PortNowDisabled:
            DereferenceDevicePower();
            return SmTransition(&HubSuspendedPortOff);

        case DsmEvent::PortNowEnabled:
        case DsmEvent::PortNowEnabledOnReconnect:
            return Wait(&SuspendingPort, &DeviceMachine::HubPortSuspendedAgain);

        case DsmEvent::PortDetached:
            DereferenceDevicePower();
            return EndWith(DsmEvent::PortDetached);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::HubPortSuspendedAgain(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortSuspendDone:
            DereferenceDevicePower();
            return SmTransition(&HubSuspended);

        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::PortFault:
            DereferenceDevicePower();
            return SmTransition(&HubSuspendedPortOff);

        default:
            return SmUnhandled();
    }
}

/* The hub stops before the port machine caught up; the power reference stays */
SM_RESULT
DeviceMachine::HubPortSyncedForStop(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortNowEnabled:
        case DsmEvent::PortNowEnabledOnReconnect:
            return Wait(&DisablingPort, &DeviceMachine::HubPortOffForStop);

        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::PortNowSuspended:
        case DsmEvent::PortNowDisabled:
            return EndWith(DsmEvent::HubStoppedHoldingReference);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::HubPortOffForStop(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::PortFault:
        case DsmEvent::PortDisableDone:
            return EndWith(DsmEvent::HubStoppedHoldingReference);

        default:
            return SmUnhandled();
    }
}

/* Resuming the port only so the device can be reset and enumerated again */
SM_RESULT
DeviceMachine::HubPortResumedForReset(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortResumeDone:
        case DsmEvent::PortNowEnabledOnReconnect:
        case DsmEvent::PortResumeTimeout:
        case DsmEvent::PortFault:
        case DsmEvent::PortNowDisabled:
            return EndWith(DsmEvent::NeedsReenumeration);

        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::HubStopping:
            return Wait(&AwaitingPort, &DeviceMachine::HubResumeEndedForStop);

        case DsmEvent::HubSuspending:
            return Wait(&AwaitingPort, &DeviceMachine::HubResumeEndedForSuspend);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::HubResumeEndedForStop(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortResumeDone:
            return Wait(&DisablingPort, &DeviceMachine::HubPortOffForStop);

        case DsmEvent::PortNowEnabledOnReconnect:
        case DsmEvent::PortResumeTimeout:
        case DsmEvent::PortNowDisabled:
        case DsmEvent::PortFault:
            return EndWith(DsmEvent::HubStoppedHoldingReference);

        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::HubResumeEndedForSuspend(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortResumeDone:
        case DsmEvent::PortNowEnabledOnReconnect:
            return Wait(&SuspendingPort, &DeviceMachine::HubPortSuspendedAgain);

        case DsmEvent::PortFault:
        case DsmEvent::PortNowDisabled:
        case DsmEvent::PortResumeTimeout:
            DereferenceDevicePower();
            return SmTransition(&HubSuspendedPortOff);

        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        default:
            return SmUnhandled();
    }
}
