/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Device state machine, work at address zero and address assignment
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include "usbhub3.h"

/* GLOBALS ********************************************************************/

/* Sets up the 3.0 device in the controller and runs the port reset sub machine */
const DeviceMachine::State DeviceMachine::ResettingSuperSpeed =
    { NULL, "ResettingSuperSpeed", SM_STATE_YIELDS_TO_CALLER,
      &DeviceMachine::OnResettingSuperSpeed, &DeviceMachine::EnterResettingSuperSpeed };

/* Points the default endpoint at the max packet size from the first descriptor read */
const DeviceMachine::State DeviceMachine::UpdatingDefaultEndpoint =
    { NULL, "UpdatingDefaultEndpoint", SM_STATE_CRITICAL_ONLY,
      &DeviceMachine::OnShared, &DeviceMachine::EnterUpdatingDefaultEndpoint };

/* Gives the device its address */
const DeviceMachine::State DeviceMachine::SettingAddress =
    { NULL, "SettingAddress", SM_STATE_CRITICAL_ONLY,
      &DeviceMachine::OnSettingAddress, &DeviceMachine::EnterSettingAddress };

/* FUNCTIONS ******************************************************************/

/* 2.0 PORT AT ADDRESS ZERO, FIRST ENUMERATION ********************************/

SM_RESULT
DeviceMachine::Zero20FirstResetDone(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortResetDone:
            if (!SetSpeedFor20())
                return EndWith(DsmEvent::Failed);

            return AtPassive(&DeviceMachine::Zero20CreateDevice);

        case DsmEvent::PortResetAbortedForSuspend:
            return Wait(&AwaitingPort, &DeviceMachine::EndOnHubOrDetach);

        case DsmEvent::HubStopping:
        case DsmEvent::HubSuspending:
            m_HubEvent = Event;
            return Wait(&AwaitingPort, &DeviceMachine::Zero20FirstResetEndedForHub);

        case DsmEvent::PortResetTimeout:
            RecordResetTimeout();
            return EndWith(DsmEvent::Failed);

        case DsmEvent::PortFault:
            return EndWith(DsmEvent::Failed);

        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::Zero20FirstResetEndedForHub(
    _In_ DsmEvent Event)
{
    if (Event == DsmEvent::PortDisableDone)
        return EndWith(m_HubEvent);

    return EndOnResetForHub(Event);
}

SM_RESULT
DeviceMachine::Zero20CreateDevice()
{
    if (!CreateDeviceInController())
        return EndWith(DsmEvent::Failed);

    if (!CreateDefaultEndpoint())
    {
        DeleteDeviceInController();
        return EndWith(DsmEvent::Failed);
    }

    return Wait(&EnablingDevice, &DeviceMachine::Zero20Enabled);
}

SM_RESULT
DeviceMachine::Zero20Enabled(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ControllerIoctlDone:
            return Delay(IsFirstEnumTry() ? DsmDelayPostReset : DsmDelayPostResetLong,
                         &DeviceMachine::Zero20FirstDelayEnded);

        case DsmEvent::ControllerIoctlFailed:
            return DeleteThen(&DeviceMachine::EndFailed);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::Zero20FirstDelayEnded(
    _In_ DsmEvent Event)
{
    if (Event == DsmEvent::TimerFired)
        return Wait(&ReadingFirstDescriptor, &DeviceMachine::Zero20FirstDescriptorRead);

    return Zero20StopDelay(Event);
}

/* Hub stop, hub suspend or detach while the post reset timer runs */
SM_RESULT
DeviceMachine::Zero20StopDelay(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            if (StopTimer())
                return DisableAndDelete(&DeviceMachine::EndDetached);

            return Wait(&TimerAfterDetach, &DeviceMachine::Zero20TimerFlushedOnDetach);

        case DsmEvent::HubStopping:
        case DsmEvent::HubSuspending:
            m_HubEvent = Event;
            if (StopTimer())
                return DisableAndDelete(&DeviceMachine::EndWithHubEvent);

            return Wait(&AwaitingPort, &DeviceMachine::Zero20TimerFlushedForHub);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::Zero20TimerFlushedOnDetach(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::TimerFired)
        return SmUnhandled();

    return DisableAndDelete(&DeviceMachine::EndDetached);
}

SM_RESULT
DeviceMachine::Zero20TimerFlushedForHub(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            return Wait(&TimerAfterDetach, &DeviceMachine::Zero20TimerFlushedOnDetach);

        case DsmEvent::TimerFired:
            return DisableAndDelete(&DeviceMachine::EndWithHubEvent);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::Zero20FirstDescriptorRead(
    _In_ DsmEvent Event)
{
    if ((Event != DsmEvent::TransferDone) && (Event != DsmEvent::TransferFailed))
        return SmUnhandled();

    if (FirstDescriptorValid())
        return Wait(&UpdatingDefaultEndpoint, &DeviceMachine::Zero20EndpointUpdated);

    return DisableAndDelete(&DeviceMachine::EndFailed);
}

SM_RESULT
DeviceMachine::EnterUpdatingDefaultEndpoint()
{
    UpdateDefaultEndpoint();
    RecordDeviceVersion();
    return SmHandled();
}

SM_RESULT
DeviceMachine::Zero20EndpointUpdated(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ControllerIoctlDone:
            if (NeedsSecondReset())
                return Wait(&ResettingPort, &DeviceMachine::Zero20SecondResetDone);

            return Wait(&NotifyingReset, &DeviceMachine::Zero20ResetNotified);

        case DsmEvent::ControllerIoctlFailed:
            return DisableAndDelete(&DeviceMachine::EndFailed);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::Zero20ResetNotified(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ControllerIoctlDone:
            return EndWith(DsmEvent::Succeeded);

        case DsmEvent::ControllerIoctlFailed:
            return DisableAndDelete(&DeviceMachine::EndFailed);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::Zero20SecondResetDone(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortResetDone:
            return Delay(IsFirstEnumTry() ? DsmDelayPostReset : DsmDelayPostResetLong,
                         &DeviceMachine::Zero20SecondDelayEnded);

        case DsmEvent::PortDetached:
            return DisableAndDelete(&DeviceMachine::EndDetached);

        case DsmEvent::PortResetTimeout:
            RecordResetTimeout();
            return DisableAndDelete(&DeviceMachine::EndFailed);

        case DsmEvent::HubStopping:
        case DsmEvent::HubSuspending:
            m_HubEvent = Event;
            return Wait(&AwaitingPort, &DeviceMachine::Zero20SecondResetEndedForHub);

        case DsmEvent::PortFault:
            return DisableAndDelete(&DeviceMachine::EndFailed);

        case DsmEvent::PortResetAbortedForSuspend:
            return Wait(&AwaitingPort, &DeviceMachine::Zero20SecondResetAwaitHub);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::Zero20SecondResetEndedForHub(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            return DisableAndDelete(&DeviceMachine::EndDetached);

        case DsmEvent::PortResetTimeout:
        case DsmEvent::PortFault:
        case DsmEvent::PortResetDone:
            return DisableAndDelete(&DeviceMachine::EndWithHubEvent);

        case DsmEvent::PortResetAbortedForSuspend:
            return Wait(&DisablingPort, &DeviceMachine::Zero20PortOffForHub);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::Zero20SecondResetAwaitHub(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            return DisableAndDelete(&DeviceMachine::EndDetached);

        case DsmEvent::HubStopping:
        case DsmEvent::HubSuspending:
            m_HubEvent = Event;
            return Wait(&DisablingPort, &DeviceMachine::Zero20PortOffForHub);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::Zero20PortOffForHub(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortFault:
        case DsmEvent::PortDisableDone:
            return DisableAndDelete(&DeviceMachine::EndWithHubEvent);

        case DsmEvent::PortDetached:
            return DisableAndDelete(&DeviceMachine::EndDetached);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::Zero20SecondDelayEnded(
    _In_ DsmEvent Event)
{
    if (Event == DsmEvent::TimerFired)
        return Wait(&NotifyingReset, &DeviceMachine::Zero20ResetNotified);

    return Zero20StopDelay(Event);
}

/* 3.0 PORT AT ADDRESS ZERO, FIRST ENUMERATION ********************************/

SM_RESULT
DeviceMachine::Zero30CreateDevice()
{
    if (!CreateDeviceInController())
        return EndWith(DsmEvent::Failed);

    if (!CreateDefaultEndpoint())
    {
        DeleteDeviceInController();
        return EndWith(DsmEvent::Failed);
    }

    return SmTransition(&ResettingSuperSpeed);
}

SM_RESULT
DeviceMachine::EnterResettingSuperSpeed()
{
    return StartSuperSpeedReset();
}

SM_RESULT
DeviceMachine::OnResettingSuperSpeed(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
            if (!SetSpeedFor30())
                return DeleteThen(&DeviceMachine::EndFailed);

            return Wait(&EnablingDevice, &DeviceMachine::Zero30Enabled);

        case DsmEvent::Failed:
            return DeleteThen(&DeviceMachine::EndFailed);

        case DsmEvent::HubStopping:
        case DsmEvent::HubSuspending:
            m_HubEvent = Event;
            return DeleteThen(&DeviceMachine::EndWithHubEvent);

        case DsmEvent::PortDetached:
            return DeleteThen(&DeviceMachine::EndDetached);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::Zero30Enabled(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ControllerIoctlDone:
            return EndWith(DsmEvent::Succeeded);

        case DsmEvent::ControllerIoctlFailed:
            return DeleteThen(&DeviceMachine::EndFailed);

        default:
            return SmUnhandled();
    }
}

/* SUPERSPEED PORT RESET SUB MACHINE ******************************************/

/* Pushes the reset sub machine; a device that needs no reset only tells the controller */
SM_RESULT
DeviceMachine::StartSuperSpeedReset()
{
    switch (ResetKindNeeded())
    {
        case DsmResetHot:
            RequestPortReset();
            return CallWait(&AwaitingPort, &DeviceMachine::SuperSpeedResetDone);

        case DsmResetWarm:
            RequestPortWarmReset();
            return CallWait(&AwaitingPort, &DeviceMachine::SuperSpeedResetDone);

        default:
            return CallWait(&NotifyingReset, &DeviceMachine::SuperSpeedResetNotified);
    }
}

SM_RESULT
DeviceMachine::SuperSpeedResetNotified(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return EndWith(DsmEvent::Succeeded);
}

SM_RESULT
DeviceMachine::SuperSpeedResetDone(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::HubStopping:
        case DsmEvent::HubSuspending:
            m_HubEvent = Event;
            return Wait(&AwaitingPort, &DeviceMachine::EndOnResetForHub);

        case DsmEvent::PortResetDone:
            if (IsFirstEnumTry())
                return Wait(&NotifyingReset, &DeviceMachine::SuperSpeedResetNotified);

            return Delay(DsmDelayPostResetSuperSpeed, &DeviceMachine::SuperSpeedDelayEnded);

        case DsmEvent::PortResetAbortedForSuspend:
            return Wait(&AwaitingPort, &DeviceMachine::EndOnHubOrDetach);

        case DsmEvent::PortResetTimeout:
            RecordResetTimeout();
            return EndWith(DsmEvent::Failed);

        case DsmEvent::PortFault:
            return EndWith(DsmEvent::Failed);

        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::SuperSpeedDelayEnded(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TimerFired:
            return Wait(&NotifyingReset, &DeviceMachine::SuperSpeedResetNotified);

        case DsmEvent::HubStopping:
        case DsmEvent::HubSuspending:
        case DsmEvent::PortDetached:
            return EndAfterTimer(Event);

        default:
            return SmUnhandled();
    }
}

/* ADDRESS SUB MACHINE ********************************************************/

SM_RESULT
DeviceMachine::EnterSettingAddress()
{
    SetDeviceAddress();
    return SmHandled();
}

SM_RESULT
DeviceMachine::OnSettingAddress(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ControllerIoctlFailed:
            return EndWith(DsmEvent::Failed);

        case DsmEvent::ControllerIoctlDone:
            if (!WaitNeededAfterAddress())
                return EndWith(DsmEvent::Succeeded);

            return Delay(DsmDelayPostAddress, &DeviceMachine::AddressDelayEnded);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::AddressDelayEnded(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TimerFired:
            return EndWith(DsmEvent::Succeeded);

        case DsmEvent::HubStopping:
        case DsmEvent::HubSuspending:
        case DsmEvent::PortDetached:
            return EndAfterTimer(Event);

        default:
            return SmUnhandled();
    }
}
