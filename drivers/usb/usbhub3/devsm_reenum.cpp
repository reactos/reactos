/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Device state machine, enumerating a known device again after a
 *              reset, resume or restart
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include "usbhub3.h"

/* GLOBALS ********************************************************************/

static const DsmEvent ReenumLpmDiscards[] = { DsmEvent::LpmSettingChanged, DsmEvent::Count };

static const DsmEvent ReenumQuietDiscards[] =
{
    DsmEvent::LpmSettingChanged, DsmEvent::NoPingResponse, DsmEvent::Count
};

/* The flavors differ only in which requests wait and which are dropped */
const DeviceMachine::State DeviceMachine::Reenumerating =
    { NULL, "Reenumerating", 0, &DeviceMachine::OnReenumerating, &DeviceMachine::EnterReenumerating };

const DeviceMachine::State DeviceMachine::ReenumeratingForHubStart =
    { NULL, "ReenumeratingForHubStart", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnReenumerating, &DeviceMachine::EnterReenumerating, ReenumLpmDiscards };

const DeviceMachine::State DeviceMachine::ReenumeratingForPower =
    { NULL, "ReenumeratingForPower", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnReenumerating, &DeviceMachine::EnterReenumerating, ReenumQuietDiscards };

/* Address zero work in the re-enumeration sub machine */
const DeviceMachine::State DeviceMachine::AtAddressZeroAgain =
    { NULL, "AtAddressZeroAgain", 0,
      &DeviceMachine::OnAtAddressZeroAgain, &DeviceMachine::EnterAtAddressZeroAgain };

const DeviceMachine::State DeviceMachine::AddressingAgain =
    { NULL, "AddressingAgain", 0, &DeviceMachine::OnAddressingAgain, &DeviceMachine::EnterAddressing };

const DeviceMachine::State DeviceMachine::LinkPowerAgain =
    { NULL, "LinkPowerAgain", 0, &DeviceMachine::OnLinkPowerAgain, &DeviceMachine::EnterLinkPowerAgain };

const DeviceMachine::State DeviceMachine::ParkedForHubAgain =
    { NULL, "ParkedForHubAgain", SM_STATE_YIELDS_TO_CALLER,
      &DeviceMachine::OnParkedForHubAgain, &DeviceMachine::EnterParkedForHub };

/* Claims address zero for the re-enumeration */
const DeviceMachine::State DeviceMachine::ClaimingAddressZeroAgain =
    { NULL, "ClaimingAddressZeroAgain", 0,
      &DeviceMachine::OnClaimingAddressZeroAgain, &DeviceMachine::EnterClaimingAddressZero };

const DeviceMachine::State DeviceMachine::CancelingClaimAgain =
    { NULL, "CancelingClaimAgain", SM_STATE_CRITICAL_ONLY,
      &DeviceMachine::OnCancelingClaimAgain, &DeviceMachine::EnterCancelingClaim };

/* The boot device failed to enumerate again; the system cannot go on */
const DeviceMachine::State DeviceMachine::BugChecking =
    { NULL, "BugChecking", 0, NULL, &DeviceMachine::EnterBugChecking };

/* FUNCTIONS ******************************************************************/

/* Start a re-enumeration from Target with the given start and result handling */
SM_RESULT
DeviceMachine::Reenumerate(
    _In_ const State* Target,
    _In_ DSM_REENUM Kind,
    _In_ BOOLEAN ForceReset,
    _In_ DSM_NEXT After)
{
    m_ReenumKind = Kind;
    m_ReenumForceReset = ForceReset;
    m_AfterReenum = After;
    return SmTransition(Target);
}

SM_RESULT
DeviceMachine::EnterReenumerating()
{
    if (m_ReenumForceReset)
        ForceResetOnNextStart();

    StartEnumRetries();
    LogReenumeration();

    switch (m_ReenumKind)
    {
        case DsmReenumConfigured:
            return CallRequest(&RequestingCritical, &DeviceMachine::PurgeDeviceTreeIo,
                               &DeviceMachine::ReenumTreePurgedConfigured);

        case DsmReenumUnconfigured:
            return CallRequest(&RequestingCritical, &DeviceMachine::PurgeDeviceTreeIo,
                               &DeviceMachine::ReenumTreePurgedUnconfigured);

        default:
            ClearReprogramNeeded();
            return CallWait(&EnablingDevice, &DeviceMachine::ReenumEnabled);
    }
}

SM_RESULT
DeviceMachine::OnReenumerating(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
        case DsmEvent::Failed:
        case DsmEvent::FailedDeviceEnabled:
        case DsmEvent::DetachedDeviceEnabled:
        case DsmEvent::PortDetached:
        case DsmEvent::PdoCleanup:
            return (this->*m_AfterReenum)(Event);

        default:
            return SmUnhandled();
    }
}

/* RE-ENUMERATION SUB MACHINE *************************************************/

SM_RESULT
DeviceMachine::ReenumTreePurgedConfigured(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    if (DeviceProgrammingLost())
        return Wait(&EnablingDevice, &DeviceMachine::ReenumEnabled);

    return Wait(&DisablingEndpoints, &DeviceMachine::ReenumEndpointsDisabled);
}

SM_RESULT
DeviceMachine::ReenumEndpointsDisabled(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return SmTransition(&ClaimingAddressZeroAgain);
}

SM_RESULT
DeviceMachine::ReenumTreePurgedUnconfigured(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    if (DeviceProgrammingLost())
        return Wait(&EnablingDevice, &DeviceMachine::ReenumEnabled);

    return SmTransition(&ClaimingAddressZeroAgain);
}

SM_RESULT
DeviceMachine::ReenumEnabled(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ControllerIoctlFailed:
            if (!EnumRetriesUsedUp())
                return Wait(&EnablingDevice, &DeviceMachine::ReenumEnabled);

            return Request(&Requesting, &DeviceMachine::FailIoAndDisablePort,
                           &DeviceMachine::ReenumPortOffDeviceDisabled);

        case DsmEvent::ControllerIoctlDone:
            return SmTransition(&ClaimingAddressZeroAgain);

        default:
            return SmUnhandled();
    }
}

VOID
DeviceMachine::FailIoAndDisablePort()
{
    SetFailIo();
    RequestPortDisable();
}

/* Out of retries with the device disabled in the controller */
SM_RESULT
DeviceMachine::ReenumPortOffDeviceDisabled(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::PortFault:
        case DsmEvent::PortDisableDone:
            return EndWith(DsmEvent::Failed);

        case DsmEvent::HubSuspending:
        case DsmEvent::HubStopping:
            return Wait(&AwaitingPort, &DeviceMachine::ReenumPortOffForHubDeviceDisabled);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ReenumPortOffForHubDeviceDisabled(
    _In_ DsmEvent Event)
{
    if ((Event != DsmEvent::PortDisableDone) && (Event != DsmEvent::PortFault))
        return SmUnhandled();

    return ReenumParkForHub();
}

SM_RESULT
DeviceMachine::ReenumParkForHub()
{
    DereferenceDevicePower();
    return SmTransition(&ParkedForHubAgain);
}

SM_RESULT
DeviceMachine::OnParkedForHubAgain(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::SuspendedAfterHubResume:
        case DsmEvent::ResumedWithHub:
        case DsmEvent::NeedsReenumeration:
            (VOID)DeviceProgrammingLost();
            return Wait(&EnablingDevice, &DeviceMachine::ReenumEnabled);

        case DsmEvent::HubStoppedHoldingReference:
            return ReenumParkForHub();

        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::HubStarted:
            ReferenceDevicePower();
            return Wait(&EnablingDevice, &DeviceMachine::ReenumEnabled);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::OnClaimingAddressZeroAgain(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            CancelControllerIoctl();
            return Wait(&IoctlAfterDetach, &DeviceMachine::ReenumClaimEndedOnDetach);

        case DsmEvent::HubStopping:
        case DsmEvent::HubSuspending:
            return SmTransition(&CancelingClaimAgain);

        case DsmEvent::ControllerIoctlDone:
            return SmTransition(&AtAddressZeroAgain);

        case DsmEvent::ControllerIoctlFailed:
            return ReenumRetryOrFail();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ReenumClaimEndedOnDetach(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ControllerIoctlFailed:
            return EndWith(DsmEvent::DetachedDeviceEnabled);

        case DsmEvent::ControllerIoctlDone:
            ReleaseAddressZero();
            return EndWith(DsmEvent::DetachedDeviceEnabled);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::OnCancelingClaimAgain(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ControllerIoctlFailed:
            return ReenumDisableForHub();

        case DsmEvent::ControllerIoctlDone:
            ReleaseAddressZero();
            return ReenumDisableForHub();

        default:
            return SmUnhandled();
    }
}

/* Hub stop or suspend: disable the device and the port, then park */
SM_RESULT
DeviceMachine::ReenumDisableForHub()
{
    return Wait(&DisablingDevice, &DeviceMachine::ReenumDisabledForHub);
}

SM_RESULT
DeviceMachine::ReenumDisabledForHub(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return Wait(&DisablingPort, &DeviceMachine::ReenumPortOffForHub);
}

SM_RESULT
DeviceMachine::ReenumPortOffForHub(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::PortFault:
        case DsmEvent::PortDisableDone:
            return ReenumParkForHub();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ReenumRetryOrFail()
{
    if (EnumRetriesUsedUp())
    {
        return Request(&RequestingCritical, &DeviceMachine::FailIoAndPurge,
                       &DeviceMachine::ReenumPurgedAfterFailure);
    }

    return Delay(DsmDelayEnumRetry, &DeviceMachine::ReenumRetryDelayEnded);
}

VOID
DeviceMachine::FailIoAndPurge()
{
    SetFailIo();
    PurgeDeviceIo();
}

SM_RESULT
DeviceMachine::ReenumPurgedAfterFailure(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return Wait(&DisablingPort, &DeviceMachine::ReenumPortOffAfterFailure);
}

/* Out of retries with the device still enabled in the controller */
SM_RESULT
DeviceMachine::ReenumPortOffAfterFailure(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::DetachedDeviceEnabled);

        case DsmEvent::PortFault:
        case DsmEvent::PortDisableDone:
            return EndWith(DsmEvent::FailedDeviceEnabled);

        case DsmEvent::HubSuspending:
        case DsmEvent::HubStopping:
            return Wait(&AwaitingPort, &DeviceMachine::ReenumPortOffForHubDeviceEnabled);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ReenumPortOffForHubDeviceEnabled(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::DetachedDeviceEnabled);

        case DsmEvent::PortFault:
        case DsmEvent::PortDisableDone:
            return Wait(&DisablingDevice, &DeviceMachine::ReenumDisabledThenPark);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ReenumDisabledThenPark(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return ReenumParkForHub();
}

SM_RESULT
DeviceMachine::ReenumRetryDelayEnded(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TimerFired:
            return SmTransition(&ClaimingAddressZeroAgain);

        case DsmEvent::PortDetached:
            return ReenumStopRetryOnDetach();

        case DsmEvent::HubSuspending:
        case DsmEvent::HubStopping:
            return ReenumStopRetryForHub();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ReenumStopRetryOnDetach()
{
    if (StopTimer())
        return EndWith(DsmEvent::DetachedDeviceEnabled);

    return Wait(&TimerAfterDetach, &DeviceMachine::ReenumTimerFlushedOnDetach);
}

SM_RESULT
DeviceMachine::ReenumTimerFlushedOnDetach(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::TimerFired)
        return SmUnhandled();

    return EndWith(DsmEvent::DetachedDeviceEnabled);
}

SM_RESULT
DeviceMachine::ReenumStopRetryForHub()
{
    if (StopTimer())
        return ReenumDisableForHub();

    return Wait(&AwaitingPort, &DeviceMachine::ReenumTimerFlushedForHub);
}

SM_RESULT
DeviceMachine::ReenumTimerFlushedForHub(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TimerFired:
            return ReenumDisableForHub();

        case DsmEvent::PortDetached:
            return Wait(&TimerAfterDetach, &DeviceMachine::ReenumTimerFlushedOnDetach);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnterAtAddressZeroAgain()
{
    if (KindWithin(DSM_KIND_PORT30 | DSM_KIND_ANY_VERSION | DSM_KIND_ANY_SPEED))
        return StartSuperSpeedReset();

    if (KindWithin(DSM_KIND_PORT20 | DSM_KIND_ANY_VERSION | DSM_KIND_ANY_SPEED))
        return CallWait(&ResettingPort, &DeviceMachine::Again20FirstResetDone);

    return SmUnhandled();
}

SM_RESULT
DeviceMachine::OnAtAddressZeroAgain(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
            return SmTransition(&AddressingAgain);

        case DsmEvent::Failed:
            return ReenumRetryOrFailHoldingAddressZero();

        case DsmEvent::PortDetached:
            ReleaseAddressZero();
            return EndWith(DsmEvent::DetachedDeviceEnabled);

        case DsmEvent::HubSuspending:
        case DsmEvent::HubStopping:
            ReleaseAddressZero();
            return ReenumDisableForHub();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ReenumRetryOrFailHoldingAddressZero()
{
    if (EnumRetriesUsedUp())
    {
        return Request(&RequestingCritical, &DeviceMachine::FailIoAndPurge,
                       &DeviceMachine::ReenumPurgedHoldingAddressZero);
    }

    return Delay(DsmDelayEnumRetry, &DeviceMachine::ReenumRetryDelayEndedHoldingAddressZero);
}

SM_RESULT
DeviceMachine::ReenumPurgedHoldingAddressZero(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return Wait(&DisablingPort, &DeviceMachine::ReenumPortOffHoldingAddressZero);
}

SM_RESULT
DeviceMachine::ReenumPortOffHoldingAddressZero(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            ReleaseAddressZero();
            return EndWith(DsmEvent::DetachedDeviceEnabled);

        case DsmEvent::HubSuspending:
        case DsmEvent::HubStopping:
            ReleaseAddressZero();
            return Wait(&AwaitingPort, &DeviceMachine::ReenumPortOffForHubDeviceEnabled);

        case DsmEvent::PortDisableDone:
        case DsmEvent::PortFault:
            ReleaseAddressZero();
            return EndWith(DsmEvent::FailedDeviceEnabled);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ReenumRetryDelayEndedHoldingAddressZero(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::HubSuspending:
        case DsmEvent::HubStopping:
            ReleaseAddressZero();
            return ReenumStopRetryForHub();

        case DsmEvent::PortDetached:
            ReleaseAddressZero();
            return ReenumStopRetryOnDetach();

        case DsmEvent::TimerFired:
            return SmTransition(&AtAddressZeroAgain);

        default:
            return SmUnhandled();
    }
}

/* 2.0 PORT AT ADDRESS ZERO, RE-ENUMERATION ***********************************/

SM_RESULT
DeviceMachine::Again20FirstResetDone(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortResetDone:
            return Wait(&NotifyingReset, &DeviceMachine::Again20FirstResetNotified);

        case DsmEvent::HubStopping:
        case DsmEvent::HubSuspending:
            m_HubEvent = Event;
            return Wait(&AwaitingPort, &DeviceMachine::EndOnResetForHub);

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
DeviceMachine::Again20FirstResetNotified(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    if (DeviceSpeedChanged())
        return EndWith(DsmEvent::Failed);

    return Delay(DsmDelayPostReset, &DeviceMachine::Again20FirstDelayEnded);
}

SM_RESULT
DeviceMachine::Again20FirstDelayEnded(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TimerFired:
            return Wait(&ReadingFirstDescriptor, &DeviceMachine::Again20FirstDescriptorRead);

        case DsmEvent::HubStopping:
        case DsmEvent::HubSuspending:
        case DsmEvent::PortDetached:
            return EndAfterTimer(Event);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::Again20FirstDescriptorRead(
    _In_ DsmEvent Event)
{
    if ((Event != DsmEvent::TransferDone) && (Event != DsmEvent::TransferFailed))
        return SmUnhandled();

    if (FirstDescriptorValid())
        return Wait(&ResettingPort, &DeviceMachine::Again20SecondResetDone);

    return EndWith(DsmEvent::Failed);
}

SM_RESULT
DeviceMachine::Again20SecondResetDone(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortResetDone:
            return Wait(&NotifyingReset, &DeviceMachine::Again20SecondResetNotified);

        case DsmEvent::PortResetTimeout:
            RecordResetTimeout();
            return EndWith(DsmEvent::Failed);

        case DsmEvent::PortFault:
            return EndWith(DsmEvent::Failed);

        case DsmEvent::PortResetAbortedForSuspend:
            return Wait(&AwaitingPort, &DeviceMachine::EndOnHubOrDetach);

        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::HubStopping:
        case DsmEvent::HubSuspending:
            m_HubEvent = Event;
            return Wait(&AwaitingPort, &DeviceMachine::EndOnResetForHub);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::Again20SecondResetNotified(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ControllerIoctlDone:
            return Delay(IsFirstEnumTry() ? DsmDelayPostReset : DsmDelayPostResetLong,
                         &DeviceMachine::Again20SecondDelayEnded);

        case DsmEvent::ControllerIoctlFailed:
            return EndWith(DsmEvent::Failed);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::Again20SecondDelayEnded(
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

/* ADDRESS, IDENTITY CHECK AND LINK POWER *************************************/

SM_RESULT
DeviceMachine::OnAddressingAgain(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Failed:
            return ReenumRetryOrFailHoldingAddressZero();

        case DsmEvent::Succeeded:
            ReleaseAddressZero();
            return Request(&RequestingCritical, &DeviceMachine::ReadDeviceDescriptor,
                           &DeviceMachine::ReenumDescriptorRead);

        case DsmEvent::PortDetached:
            ReleaseAddressZero();
            return EndWith(DsmEvent::DetachedDeviceEnabled);

        case DsmEvent::HubStopping:
        case DsmEvent::HubSuspending:
            ReleaseAddressZero();
            return ReenumDisableForHub();

        default:
            return SmUnhandled();
    }
}

/* Make sure the same device came back before reusing what is known about it */
SM_RESULT
DeviceMachine::ReenumDescriptorRead(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            if (!SameDeviceConnected())
                return ReenumWrongDevice();

            if (AltEnumNeededOnReenum())
            {
                return Request(&RequestingCritical, &DeviceMachine::SendAltEnumCommand,
                               &DeviceMachine::ReenumAltEnumSent);
            }

            return ReenumSerialNumber();

        case DsmEvent::TransferFailed:
            return ReenumRetryOrFail();

        default:
            return SmUnhandled();
    }
}

/* A different device on the boot device's port stops here until it leaves */
SM_RESULT
DeviceMachine::ReenumWrongDevice()
{
    if (!IsBootDevice())
        return ReenumRetryOrFail();

    return Request(&Requesting, &DeviceMachine::NotifyWrongDevice,
                   &DeviceMachine::ReenumWrongDeviceLeft);
}

SM_RESULT
DeviceMachine::ReenumWrongDeviceLeft(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::PortDetached)
        return SmUnhandled();

    return EndWith(DsmEvent::DetachedDeviceEnabled);
}

SM_RESULT
DeviceMachine::ReenumAltEnumSent(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferFailed:
            return ReenumRetryOrFail();

        case DsmEvent::TransferDone:
            return ReenumSerialNumber();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ReenumSerialNumber()
{
    if (!SerialNumberCompared())
        return ReenumConfigDescriptor();

    return Request(&RequestingCritical, &DeviceMachine::ReadSerialNumber,
                   &DeviceMachine::ReenumSerialRead);
}

SM_RESULT
DeviceMachine::ReenumSerialRead(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferFailed:
            return ReenumRetryOrFail();

        case DsmEvent::TransferDone:
            if (SameSerialNumber())
                return ReenumConfigDescriptor();

            return ReenumWrongDevice();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ReenumConfigDescriptor()
{
    if (!ConfigReadOnReset())
        return SmTransition(&LinkPowerAgain);

    return Request(&RequestingCritical, &DeviceMachine::ReadConfigDescriptorHeader,
                   &DeviceMachine::ReenumConfigRead);
}

SM_RESULT
DeviceMachine::ReenumConfigRead(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            return SmTransition(&LinkPowerAgain);

        case DsmEvent::TransferFailed:
            return ReenumRetryOrFail();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnterLinkPowerAgain()
{
    if (KindWithin(DSM_KIND_USB3X | DSM_KIND_ANY_SPEED | DSM_KIND_ANY_PORT))
        return CallStep(&DeviceMachine::LinkStart);

    if (KindWithin(DSM_KIND_USB2X | DSM_KIND_ANY_SPEED | DSM_KIND_ANY_PORT))
        return CallStep(&DeviceMachine::Link20Start);

    return ReenumPdCharging();
}

SM_RESULT
DeviceMachine::OnLinkPowerAgain(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Failed:
            return ReenumRetryOrFail();

        case DsmEvent::Succeeded:
            return ReenumPdCharging();

        default:
            return SmUnhandled();
    }
}

/* USB 2.1 LPM: the controller request completes on its own when it was sent */
SM_RESULT
DeviceMachine::Link20Start()
{
    if (!UpdateLpm20())
        return EndWith(DsmEvent::Succeeded);

    return Wait(&WaitingForController, &DeviceMachine::Link20Updated);
}

SM_RESULT
DeviceMachine::Link20Updated(
    _In_ DsmEvent Event)
{
    if ((Event != DsmEvent::ControllerIoctlDone) && (Event != DsmEvent::ControllerIoctlFailed))
        return SmUnhandled();

    return EndWith(DsmEvent::Succeeded);
}

SM_RESULT
DeviceMachine::ReenumPdCharging()
{
    if (SupportsPdCharging())
        return Wait(&SettingPdCharging, &DeviceMachine::ReenumPdChargingSet);

    return EndWith(DsmEvent::Succeeded);
}

SM_RESULT
DeviceMachine::ReenumPdChargingSet(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            return EndWith(DsmEvent::Succeeded);

        case DsmEvent::TransferFailed:
            return ReenumRetryOrFail();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnterBugChecking()
{
    BugCheckForBootDevice();
    return SmHandled();
}

/* CALLERS ********************************************************************/

/* Port cycled after a failure; the device's endpoints are still to be deleted */
SM_RESULT
DeviceMachine::CycleAndPark()
{
    AskPortCycle();
    m_PortMayResume = FALSE;
    return SmTransition(&ParkedPortOff);
}

/* Client reset of an unconfigured device */
SM_RESULT
DeviceMachine::ReenumDoneForClientUnconfigured(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
            CompleteClientRequest();
            return BackToUnconfigured();

        default:
            return ReenumFailedForClient(Event);
    }
}

SM_RESULT
DeviceMachine::ReenumFailedForClient(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Failed:
            CompleteClientRequestFailed();
            return CycleAndPark();

        case DsmEvent::FailedDeviceEnabled:
            CompleteClientRequestFailed();
            AskPortCycle();
            return ToAwaitingDetachOrRemove(FALSE);

        case DsmEvent::DetachedDeviceEnabled:
            CompleteClientRequestFailed();
            return DetachUnconfigured();

        case DsmEvent::PortDetached:
            CompleteClientRequestFailed();
            return AtPassive(&DeviceMachine::ReportMissingAndLeave);

        default:
            return SmUnhandled();
    }
}

/* Client reset of a configured device: put the configuration back afterwards */
SM_RESULT
DeviceMachine::ReenumDoneForClientConfigured(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::Succeeded)
        return ReenumFailedForClient(Event);

    if (!PrepareListsForReset())
    {
        CompleteClientRequestFailed();
        return BackToUnconfigured();
    }

    m_AfterConfigure = &DeviceMachine::ReconfiguredForClient;
    return SmTransition(&Configuring);
}

SM_RESULT
DeviceMachine::ReconfiguredForClient(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Failed:
            CompleteClientRequestFailed();
            return BackToUnconfigured();

        case DsmEvent::Succeeded:
            return ClientDoneOk();

        case DsmEvent::PortDetached:
            CompleteClientRequestFailed();
            return DetachConfigured();

        default:
            return SmUnhandled();
    }
}

/* Client reset of the boot device; m_DetachEnds when a detach ends the reset */
SM_RESULT
DeviceMachine::ReenumDoneForBootDevice(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
            if (!PrepareListsForReset())
                return SmTransition(&BugChecking);

            m_AfterConfigure = &DeviceMachine::ReconfiguredBootDevice;
            return SmTransition(&Configuring);

        case DsmEvent::DetachedDeviceEnabled:
            return BootDeviceDetachDuringReset(FALSE);

        case DsmEvent::Failed:
        case DsmEvent::FailedDeviceEnabled:
            return SmTransition(&BugChecking);

        case DsmEvent::PortDetached:
            if (!m_DetachEndsReset)
                return SmUnhandled();

            return BootDeviceDetachDuringReset(TRUE);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ReconfiguredBootDevice(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
            NotifyReconnected();
            CompleteClientRequest();
            return SmTransition(&ConfiguredOn);

        case DsmEvent::Failed:
            return SmTransition(&BugChecking);

        case DsmEvent::PortDetached:
            return Request(&RequestingCriticalPassive, &DeviceMachine::DisableEndpointsAndNotify,
                           &DeviceMachine::BootDeviceDisabledForReset);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::BootDeviceDisabledForReset(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    m_ContextGone = FALSE;
    return SmTransition(&BootDeviceResetPending);
}

/* Hub started again with the device enumerated but not started */
SM_RESULT
DeviceMachine::ReenumDoneForHubStart(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
            return SmTransition(&StoppedAddressed);

        case DsmEvent::Failed:
            return CycleAndPark();

        case DsmEvent::DetachedDeviceEnabled:
            return Wait(&DisablingDevice, &DeviceMachine::ReenumDisabledThenReport);

        case DsmEvent::FailedDeviceEnabled:
            return Wait(&DisablingDevice, &DeviceMachine::ReenumDisabledThenCycle);

        case DsmEvent::PdoCleanup:
            return AtPassive(&DeviceMachine::RemovalDeleteForgetPortOn);

        case DsmEvent::PortDetached:
            return AtPassive(&DeviceMachine::ReportMissingAndLeave);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ReenumDisabledThenReport(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return AtPassive(&DeviceMachine::ReportMissingAndLeave);
}

SM_RESULT
DeviceMachine::ReenumDisabledThenCycle(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return CycleAndPark();
}

/* PDO start of a device that stayed enabled or stopped */
SM_RESULT
DeviceMachine::ReenumDoneForStart(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
            return AtPassive(&DeviceMachine::ReenumCleanUpOldConfig);

        case DsmEvent::Failed:
            SignalPnpWaiter();
            return CycleAndPark();

        case DsmEvent::DetachedDeviceEnabled:
            SignalPnpWaiter();
            return DetachUnconfigured();

        case DsmEvent::FailedDeviceEnabled:
            return Wait(&DisablingDevice, &DeviceMachine::ReenumDisabledThenSignalAndCycle);

        case DsmEvent::PortDetached:
            SignalPnpWaiter();
            return AtPassive(&DeviceMachine::ReportMissingAndLeave);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ReenumCleanUpOldConfig()
{
    DeleteConfigEndpoints();
    SignalPnpWaiter();
    return SmTransition(&UnconfiguredOn);
}

SM_RESULT
DeviceMachine::ReenumDisabledThenSignalAndCycle(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    SignalPnpWaiter();
    return CycleAndPark();
}

/* PDO pre start of a device that stayed enabled */
SM_RESULT
DeviceMachine::ReenumDoneForPreStart(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
            return StoppedAckPreStart();

        case DsmEvent::FailedDeviceEnabled:
            return Wait(&DisablingDevice, &DeviceMachine::ReenumDisabledThenAckAndCycle);

        case DsmEvent::DetachedDeviceEnabled:
            CompletePreStart();
            return DetachUnconfigured();

        case DsmEvent::Failed:
            CompletePreStart();
            return CycleAndPark();

        case DsmEvent::PortDetached:
            CompletePreStart();
            return AtPassive(&DeviceMachine::ReportMissingAndLeave);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ReenumDisabledThenAckAndCycle(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    CompletePreStart();
    return CycleAndPark();
}

/* Hub resumed and asked for the device to be enumerated again */
SM_RESULT
DeviceMachine::ReenumDoneForHubResumeUnconfigured(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
            return SmTransition(&UnconfiguredOn);

        default:
            return ReenumFailedForHubResume(Event);
    }
}

SM_RESULT
DeviceMachine::ReenumFailedForHubResume(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::DetachedDeviceEnabled:
            return DetachUnconfigured();

        case DsmEvent::Failed:
            return CycleAndPark();

        case DsmEvent::FailedDeviceEnabled:
            AskPortCycle();
            return ToAwaitingDetachOrRemove(FALSE);

        case DsmEvent::PortDetached:
            return AtPassive(&DeviceMachine::ReportMissingAndLeave);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ReenumDoneForHubResumeConfigured(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::Succeeded)
        return ReenumFailedForHubResume(Event);

    if (!PrepareListsForReset())
        return Wait(&DisablingPort, &DeviceMachine::ReenumPortOffAfterConfigFailure);

    m_AfterConfigure = &DeviceMachine::ReconfiguredAfterHubResume;
    return SmTransition(&Configuring);
}

SM_RESULT
DeviceMachine::ReconfiguredAfterHubResume(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
            return SmTransition(&ConfiguredOn);

        case DsmEvent::Failed:
            return Wait(&DisablingPort, &DeviceMachine::ReenumPortOffAfterConfigFailure);

        case DsmEvent::PortDetached:
            return AtPassive(&DeviceMachine::RemovalNotifyThenReport);

        default:
            return SmUnhandled();
    }
}

/* Configuration could not be restored: the port goes off, the device waits */
SM_RESULT
DeviceMachine::ReenumPortOffAfterConfigFailure(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDisableDone:
        case DsmEvent::PortFault:
            return ToAwaitingDetachOrRemove(FALSE);

        case DsmEvent::PortDetached:
            return DetachUnconfigured();

        default:
            return SmUnhandled();
    }
}

/* PnP resume that needed a re-enumeration */
SM_RESULT
DeviceMachine::ReenumDoneForResumeUnconfigured(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
            SignalPnpWaiter();
            return SmTransition(&UnconfiguredOn);

        default:
            return ReenumFailedForResume(Event);
    }
}

SM_RESULT
DeviceMachine::ReenumFailedForResume(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::FailedDeviceEnabled:
            AskPortCycle();
            SignalPnpWaiter();
            return ToAwaitingDetachOrRemove(FALSE);

        case DsmEvent::Failed:
            SignalPnpWaiter();
            return CycleAndPark();

        case DsmEvent::DetachedDeviceEnabled:
            SignalPnpWaiter();
            return DetachUnconfigured();

        case DsmEvent::PortDetached:
            SignalPnpWaiter();
            return AtPassive(&DeviceMachine::ReportMissingAndLeave);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ReenumDoneForResumeConfigured(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::Succeeded)
        return ReenumFailedForResume(Event);

    if (!PrepareListsForReset())
        return ReenumCycleAfterResumeFailure();

    m_AfterConfigure = &DeviceMachine::ReconfiguredAfterResume;
    return SmTransition(&Configuring);
}

SM_RESULT
DeviceMachine::ReenumCycleAfterResumeFailure()
{
    AskPortCycle();
    SignalPnpWaiter();
    return Wait(&DisablingPort, &DeviceMachine::ReenumPortOffAfterConfigFailure);
}

SM_RESULT
DeviceMachine::ReconfiguredAfterResume(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
            SignalPnpWaiter();
            return SmTransition(&ConfiguredOn);

        case DsmEvent::Failed:
            return ReenumCycleAfterResumeFailure();

        case DsmEvent::PortDetached:
            SignalPnpWaiter();
            return DetachConfigured();

        default:
            return SmUnhandled();
    }
}

/* PnP resume of the boot device that needed a re-enumeration */
SM_RESULT
DeviceMachine::ReenumDoneForBootResume(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Failed:
        case DsmEvent::FailedDeviceEnabled:
            return SmTransition(&BugChecking);

        case DsmEvent::Succeeded:
            if (!PrepareListsForReset())
                return SmTransition(&BugChecking);

            m_AfterConfigure = &DeviceMachine::ReconfiguredBootAfterResume;
            return SmTransition(&Configuring);

        case DsmEvent::DetachedDeviceEnabled:
            SignalPnpWaiter();
            NotifyPortDetached();
            return SmTransition(&BootDeviceDetached);

        case DsmEvent::PortDetached:
            SignalPnpWaiter();
            return SmTransition(&BootDeviceDetachedContextGone);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ReconfiguredBootAfterResume(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Failed:
            return SmTransition(&BugChecking);

        case DsmEvent::Succeeded:
            SignalPnpWaiter();
            return SmTransition(&ConfiguredOn);

        case DsmEvent::PortDetached:
            SignalPnpWaiter();
            return AtPassive(&DeviceMachine::BootDeviceNotifyDetached);

        default:
            return SmUnhandled();
    }
}

/* STARTING A RE-ENUMERATION **************************************************/

SM_RESULT
DeviceMachine::ReenumForClientUnconfigured()
{
    return Reenumerate(&Reenumerating, DsmReenumUnconfigured, TRUE,
                       &DeviceMachine::ReenumDoneForClientUnconfigured);
}

SM_RESULT
DeviceMachine::ReenumForClientConfigured()
{
    return Reenumerate(&Reenumerating, DsmReenumConfigured, FALSE,
                       &DeviceMachine::ReenumDoneForClientConfigured);
}

/* Boot device reset by its client, either right away or once it is back */
SM_RESULT
DeviceMachine::ReenumForBootClient(
    _In_ BOOLEAN AfterReattach)
{
    DSM_REENUM kind = DsmReenumConfigured;

    if (AfterReattach)
        kind = m_ContextGone ? DsmReenumRestart : DsmReenumUnconfigured;

    m_DetachEndsReset = AfterReattach;
    return Reenumerate(&Reenumerating, kind, FALSE, &DeviceMachine::ReenumDoneForBootDevice);
}

SM_RESULT
DeviceMachine::ReenumForHubStart()
{
    return Reenumerate(&ReenumeratingForHubStart, DsmReenumUnconfigured, FALSE,
                       &DeviceMachine::ReenumDoneForHubStart);
}

SM_RESULT
DeviceMachine::ReenumForStart(
    _In_ DSM_REENUM Kind)
{
    return Reenumerate(&ReenumeratingForPower, Kind, FALSE, &DeviceMachine::ReenumDoneForStart);
}

SM_RESULT
DeviceMachine::ReenumForPreStart()
{
    return Reenumerate(&ReenumeratingForPower, DsmReenumRestart, TRUE,
                       &DeviceMachine::ReenumDoneForPreStart);
}

SM_RESULT
DeviceMachine::ReenumForHubResume(
    _In_ BOOLEAN Configured)
{
    if (Configured)
    {
        return Reenumerate(&ReenumeratingForPower, DsmReenumConfigured, FALSE,
                           &DeviceMachine::ReenumDoneForHubResumeConfigured);
    }

    return Reenumerate(&ReenumeratingForPower, DsmReenumUnconfigured, FALSE,
                       &DeviceMachine::ReenumDoneForHubResumeUnconfigured);
}

SM_RESULT
DeviceMachine::ReenumForResume(
    _In_ BOOLEAN Configured)
{
    if (Configured)
    {
        return Reenumerate(&ReenumeratingForPower, DsmReenumConfigured, FALSE,
                           &DeviceMachine::ReenumDoneForResumeConfigured);
    }

    return Reenumerate(&ReenumeratingForPower, DsmReenumUnconfigured, FALSE,
                       &DeviceMachine::ReenumDoneForResumeUnconfigured);
}

SM_RESULT
DeviceMachine::ReenumForBootResume()
{
    return Reenumerate(&ReenumeratingForPower, DsmReenumConfigured, FALSE,
                       &DeviceMachine::ReenumDoneForBootResume);
}
