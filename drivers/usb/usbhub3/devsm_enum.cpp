/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Device state machine, attach and first enumeration
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include "usbhub3.h"

/* GLOBALS ********************************************************************/

static const DsmEvent DeleteDiscards[] =
{
    DsmEvent::PdoReportedMissing, DsmEvent::LpmSettingChanged, DsmEvent::NoPingResponse,
    DsmEvent::Count
};

static const DsmEvent LpmDiscards[] = { DsmEvent::LpmSettingChanged, DsmEvent::Count };

/* Nothing attached yet. Initial state */
const DeviceMachine::State DeviceMachine::AwaitingAttach =
    { NULL, "AwaitingAttach", 0, &DeviceMachine::OnAwaitingAttach, NULL };

/* Done with the device, waiting for the port to delete the machine */
const DeviceMachine::State DeviceMachine::AwaitingDelete =
    { NULL, "AwaitingDelete", 0, NULL, NULL, DeleteDiscards };

/* Runs the enumeration sub machine with a power reference on the hub */
const DeviceMachine::State DeviceMachine::Enumerating =
    { NULL, "Enumerating", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnEnumerating, &DeviceMachine::EnterEnumerating, LpmDiscards };

/* Enumeration sub machine */
const DeviceMachine::State DeviceMachine::ClaimingAddressZero =
    { NULL, "ClaimingAddressZero", 0,
      &DeviceMachine::OnClaimingAddressZero, &DeviceMachine::EnterClaimingAddressZero };

const DeviceMachine::State DeviceMachine::CancelingClaimForHub =
    { NULL, "CancelingClaimForHub", 0,
      &DeviceMachine::OnCancelingClaimForHub, &DeviceMachine::EnterCancelingClaim };

const DeviceMachine::State DeviceMachine::AtAddressZero =
    { NULL, "AtAddressZero", 0, &DeviceMachine::OnAtAddressZero, &DeviceMachine::EnterAtAddressZero };

const DeviceMachine::State DeviceMachine::AddressingInEnum =
    { NULL, "AddressingInEnum", 0,
      &DeviceMachine::OnAddressingInEnum, &DeviceMachine::EnterAddressing };

const DeviceMachine::State DeviceMachine::ReadingDescriptors =
    { NULL, "ReadingDescriptors", 0,
      &DeviceMachine::OnReadingDescriptors, &DeviceMachine::EnterReadingDescriptors };

const DeviceMachine::State DeviceMachine::LinkPowerInEnum =
    { NULL, "LinkPowerInEnum", 0,
      &DeviceMachine::OnLinkPowerInEnum, &DeviceMachine::EnterLinkPowerInEnum };

const DeviceMachine::State DeviceMachine::UpdatingDevice =
    { NULL, "UpdatingDevice", SM_STATE_CRITICAL_ONLY,
      &DeviceMachine::OnUpdatingDevice, &DeviceMachine::EnterUpdatingDevice };

const DeviceMachine::State DeviceMachine::ParkedForHubInEnum =
    { NULL, "ParkedForHubInEnum", SM_STATE_YIELDS_TO_CALLER,
      &DeviceMachine::OnParkedForHubInEnum, &DeviceMachine::EnterParkedForHub };

/* FUNCTIONS ******************************************************************/

SM_RESULT
DeviceMachine::OnAwaitingAttach(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::PortAttached)
        return SmUnhandled();

    if (!RegisterWithHub())
    {
        MarkAttachFailed();
        return SmTransition(&AwaitingDelete);
    }

    ReferencePort();
    MarkAttachSucceeded();
    (VOID)ReferenceHubPower();
    return SmTransition(&Enumerating);
}

SM_RESULT
DeviceMachine::EnterEnumerating()
{
    StartEnumRetries();
    return SmCall(&ClaimingAddressZero);
}

SM_RESULT
DeviceMachine::OnEnumerating(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
            DereferenceHubPower();
            return SmTransition(&ReportingToPnp);

        case DsmEvent::Failed:
            DereferenceHubPower();
            return SmTransition(&ReportingUnknown);

        case DsmEvent::PortDetached:
            DereferenceHubPower();
            return EndAfterDetach();

        default:
            return SmUnhandled();
    }
}

/* Tell the port, leave the hub and wait to be deleted */
SM_RESULT
DeviceMachine::EndAfterDetach()
{
    NotifyPortDetached();
    UnregisterFromHub();
    DereferencePort();
    DiscardHubEvents();
    return SmTransition(&AwaitingDelete);
}

/* Hub events that arrive after the device left the hub mean nothing to it */
VOID
DeviceMachine::DiscardHubEvents()
{
    SmDiscardQueued(DsmEvent::HubResumed);
    SmDiscardQueued(DsmEvent::HubStarted);
    SmDiscardQueued(DsmEvent::HubSuspending);
    SmDiscardQueued(DsmEvent::HubStopping);
    SmDiscardQueued(DsmEvent::HubResumedWithReset);
    SmDiscardQueued(DsmEvent::HubStoppingAfterSuspend);
    SmDiscardQueued(DsmEvent::HubResumedInS0);
}

SM_RESULT
DeviceMachine::EnterClaimingAddressZero()
{
    ClaimAddressZero();
    return SmHandled();
}

SM_RESULT
DeviceMachine::OnClaimingAddressZero(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            CancelControllerIoctl();
            return Wait(&IoctlAfterDetach, &DeviceMachine::EnumClaimEndedOnDetach);

        case DsmEvent::HubStopping:
        case DsmEvent::HubSuspending:
            return SmTransition(&CancelingClaimForHub);

        case DsmEvent::ControllerIoctlDone:
            return SmTransition(&AtAddressZero);

        case DsmEvent::ControllerIoctlFailed:
            return EnumRetryOrFail();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnumClaimEndedOnDetach(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ControllerIoctlDone:
            ReleaseAddressZero();
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::ControllerIoctlFailed:
            return EndWith(DsmEvent::PortDetached);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnterCancelingClaim()
{
    CancelControllerIoctl();
    return SmHandled();
}

SM_RESULT
DeviceMachine::OnCancelingClaimForHub(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ControllerIoctlDone:
            RequestPortDisable();
            return Wait(&AwaitingPort, &DeviceMachine::EnumPortOffHoldingAddressZero);

        case DsmEvent::PortDetached:
            return Wait(&IoctlAfterDetach, &DeviceMachine::EnumClaimEndedOnDetach);

        case DsmEvent::ControllerIoctlFailed:
            return Wait(&DisablingPort, &DeviceMachine::EnumPortOffForHub);

        default:
            return SmUnhandled();
    }
}

/* Port disable for a hub stop or suspend while the device holds address zero */
SM_RESULT
DeviceMachine::EnumPortOffHoldingAddressZero(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            ReleaseAddressZero();
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::PortDisableDone:
        case DsmEvent::PortFault:
            ReleaseAddressZero();
            return EnumParkForHub();

        default:
            return SmUnhandled();
    }
}

/* Port disable for a hub stop or suspend */
SM_RESULT
DeviceMachine::EnumPortOffForHub(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::PortDisableDone:
        case DsmEvent::PortFault:
            return EnumParkForHub();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnumParkForHub()
{
    DereferenceDevicePower();
    return SmTransition(&ParkedForHubInEnum);
}

SM_RESULT
DeviceMachine::EnumRetryOrFail()
{
    if (!EnumRetriesUsedUp())
        return Delay(DsmDelayEnumRetry, &DeviceMachine::EnumRetryDelayEnded);

    return Wait(&DisablingPort, &DeviceMachine::EnumPortOffAfterFailure);
}

SM_RESULT
DeviceMachine::EnumRetryDelayEnded(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TimerFired:
            return SmTransition(&ClaimingAddressZero);

        case DsmEvent::PortDetached:
            return EnumStopRetryOnDetach();

        case DsmEvent::HubSuspending:
        case DsmEvent::HubStopping:
            return EnumStopRetryForHub();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnumStopRetryOnDetach()
{
    if (StopTimer())
        return EndWith(DsmEvent::PortDetached);

    return Wait(&TimerAfterDetach, &DeviceMachine::EnumTimerFlushedOnDetach);
}

SM_RESULT
DeviceMachine::EnumTimerFlushedOnDetach(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::TimerFired)
        return SmUnhandled();

    return EndWith(DsmEvent::PortDetached);
}

SM_RESULT
DeviceMachine::EnumStopRetryForHub()
{
    if (StopTimer())
        return Wait(&DisablingPort, &DeviceMachine::EnumPortOffForHub);

    return Wait(&AwaitingPort, &DeviceMachine::EnumTimerFlushedForHub);
}

SM_RESULT
DeviceMachine::EnumTimerFlushedForHub(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TimerFired:
            return Wait(&DisablingPort, &DeviceMachine::EnumPortOffForHub);

        case DsmEvent::PortDetached:
            return Wait(&TimerAfterDetach, &DeviceMachine::EnumTimerFlushedOnDetach);

        default:
            return SmUnhandled();
    }
}

/* Out of retries: the port is being disabled */
SM_RESULT
DeviceMachine::EnumPortOffAfterFailure(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::HubStopping:
        case DsmEvent::HubSuspending:
            return Wait(&AwaitingPort, &DeviceMachine::EnumPortOffForHub);

        case DsmEvent::PortFault:
        case DsmEvent::PortDisableDone:
            return EndWith(DsmEvent::Failed);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnterAtAddressZero()
{
    if (KindWithin(DSM_KIND_PORT20 | DSM_KIND_ANY_VERSION | DSM_KIND_ANY_SPEED))
        return CallWait(&ResettingPort, &DeviceMachine::Zero20FirstResetDone);

    if (KindWithin(DSM_KIND_PORT30 | DSM_KIND_ANY_VERSION | DSM_KIND_ANY_SPEED))
        return CallAtPassive(&DeviceMachine::Zero30CreateDevice);

    return SmUnhandled();
}

SM_RESULT
DeviceMachine::OnAtAddressZero(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
            return SmTransition(&AddressingInEnum);

        case DsmEvent::Failed:
            return EnumRetryOrFailHoldingAddressZero();

        case DsmEvent::PortDetached:
            ReleaseAddressZero();
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::HubStopping:
        case DsmEvent::HubSuspending:
            RequestPortDisable();
            return Wait(&AwaitingPort, &DeviceMachine::EnumPortOffHoldingAddressZero);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnumRetryOrFailHoldingAddressZero()
{
    if (!EnumRetriesUsedUp())
        return Delay(DsmDelayEnumRetry, &DeviceMachine::EnumRetryDelayEndedHoldingAddressZero);

    return Wait(&DisablingPort, &DeviceMachine::EnumPortOffAfterFailureHoldingAddressZero);
}

SM_RESULT
DeviceMachine::EnumRetryDelayEndedHoldingAddressZero(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TimerFired:
            return SmTransition(&AtAddressZero);

        case DsmEvent::PortDetached:
            ReleaseAddressZero();
            return EnumStopRetryOnDetach();

        case DsmEvent::HubSuspending:
        case DsmEvent::HubStopping:
            ReleaseAddressZero();
            return EnumStopRetryForHub();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnumPortOffAfterFailureHoldingAddressZero(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::HubStopping:
        case DsmEvent::HubSuspending:
            return Wait(&AwaitingPort, &DeviceMachine::EnumPortOffHoldingAddressZero);

        case DsmEvent::PortDetached:
            ReleaseAddressZero();
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::PortFault:
        case DsmEvent::PortDisableDone:
            ReleaseAddressZero();
            return EndWith(DsmEvent::Failed);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnterAddressing()
{
    return SmCall(&SettingAddress);
}

SM_RESULT
DeviceMachine::OnAddressingInEnum(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Failed:
            return DisableAndDelete(&DeviceMachine::EnumRetryOrFailHoldingAddressZero);

        case DsmEvent::Succeeded:
            ReleaseAddressZero();
            return SmTransition(&ReadingDescriptors);

        case DsmEvent::HubStopping:
        case DsmEvent::HubSuspending:
            return DisableAndDelete(&DeviceMachine::EnumPortOffAfterDelete);

        case DsmEvent::PortDetached:
            return DisableAndDelete(&DeviceMachine::EnumReleaseAndEndDetached);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnumPortOffAfterDelete()
{
    RequestPortDisable();
    return Wait(&AwaitingPort, &DeviceMachine::EnumPortOffHoldingAddressZero);
}

SM_RESULT
DeviceMachine::EnumReleaseAndEndDetached()
{
    ReleaseAddressZero();
    return EndWith(DsmEvent::PortDetached);
}

SM_RESULT
DeviceMachine::OnReadingDescriptors(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
            return SmTransition(&LinkPowerInEnum);

        case DsmEvent::Failed:
            return DisableAndDelete(&DeviceMachine::EnumRetryOrFail);

        case DsmEvent::PortDetached:
            return DisableAndDelete(&DeviceMachine::EndDetached);

        case DsmEvent::HubSuspending:
        case DsmEvent::HubStopping:
            RequestPortDisable();
            return Wait(&AwaitingPort, &DeviceMachine::EnumPortOffWithDevice);

        default:
            return SmUnhandled();
    }
}

/* Port disable for a hub stop or suspend once the device has its address */
SM_RESULT
DeviceMachine::EnumPortOffWithDevice(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            return DisableAndDelete(&DeviceMachine::EndDetached);

        case DsmEvent::PortFault:
        case DsmEvent::PortDisableDone:
            return DisableAndDelete(&DeviceMachine::EnumParkForHub);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnterLinkPowerInEnum()
{
    if (KindWithin(DSM_KIND_USB3X | DSM_KIND_ANY_SPEED | DSM_KIND_ANY_PORT))
        return CallStep(&DeviceMachine::LinkStart);

    return SmTransition(&UpdatingDevice);
}

SM_RESULT
DeviceMachine::OnLinkPowerInEnum(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Failed:
            return DisableAndDelete(&DeviceMachine::EnumRetryOrFail);

        case DsmEvent::Succeeded:
            return SmTransition(&UpdatingDevice);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnterUpdatingDevice()
{
    UpdateDeviceInController();
    return SmHandled();
}

SM_RESULT
DeviceMachine::OnUpdatingDevice(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ControllerIoctlFailed:
        case DsmEvent::ControllerExitLatencyTooLarge:
            return DisableAndDelete(&DeviceMachine::EnumRetryOrFail);

        case DsmEvent::ControllerIoctlDone:
            if (SupportsPdCharging())
                return Wait(&SettingPdCharging, &DeviceMachine::EnumPdChargingSet);

            return EndWith(DsmEvent::Succeeded);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnumPdChargingSet(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            return EndWith(DsmEvent::Succeeded);

        case DsmEvent::TransferFailed:
            return DisableAndDelete(&DeviceMachine::EnumRetryOrFail);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnterParkedForHub()
{
    return SmCall(&HubSuspended);
}

SM_RESULT
DeviceMachine::OnParkedForHubInEnum(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ResumedWithHub:
        case DsmEvent::SuspendedAfterHubResume:
        case DsmEvent::NeedsReenumeration:
            (VOID)DeviceProgrammingLost();
            return SmTransition(&ClaimingAddressZero);

        case DsmEvent::HubStoppedHoldingReference:
            return EnumParkForHub();

        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::HubStarted:
            ReferenceDevicePower();
            return SmTransition(&ClaimingAddressZero);

        default:
            return SmUnhandled();
    }
}
