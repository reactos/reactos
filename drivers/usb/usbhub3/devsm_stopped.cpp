/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Device state machine, enumerated device whose PDO is not started
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include "usbhub3.h"

/* GLOBALS ********************************************************************/

static const DsmEvent StoppedLpmDiscards[] = { DsmEvent::LpmSettingChanged, DsmEvent::Count };

/* PDO reported but not started; the device may be disabled if no driver loads in time */
const DeviceMachine::State DeviceMachine::StoppedAddressed =
    { NULL, "StoppedAddressed", SM_STATE_TAKES_REQUESTS | SM_STATE_STOP_TIMER_ON_EXIT,
      &DeviceMachine::OnStoppedAddressed, &DeviceMachine::EnterStoppedAddressed,
      StoppedLpmDiscards };

/* No driver came; the device stays enabled and is reset on the next start */
const DeviceMachine::State DeviceMachine::StoppedReady =
    { NULL, "StoppedReady", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnStoppedReady, &DeviceMachine::EnterWithMsOsInstall };

/* PDO stopped with the device disabled in the controller and at its port */
const DeviceMachine::State DeviceMachine::Stopped =
    { NULL, "Stopped", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnStopped, NULL, StoppedLpmDiscards };

/* Stopped while the hub is suspended */
const DeviceMachine::State DeviceMachine::StoppedHubSuspended =
    { NULL, "StoppedHubSuspended", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnStoppedHubSuspended, &DeviceMachine::EnterParkedForHub };

/* Enumerated but not started while the hub is suspended */
const DeviceMachine::State DeviceMachine::StoppedEnumeratedHubSuspended =
    { NULL, "StoppedEnumeratedHubSuspended", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnStoppedEnumeratedHubSuspended,
      &DeviceMachine::EnterHubSuspendedReleasingPower, StoppedLpmDiscards };

/* Reads the product name for a device text query; m_Resume says where to go after */
const DeviceMachine::State DeviceMachine::ReadingDeviceText =
    { NULL, "ReadingDeviceText", 0,
      &DeviceMachine::OnReadingDeviceText, &DeviceMachine::EnterReadingDeviceText };

/* Sub machine: waits for PnP to ask for the Microsoft OS extension install */
const DeviceMachine::State DeviceMachine::AwaitingMsOsInstall =
    { NULL, "AwaitingMsOsInstall", SM_STATE_YIELDS_TO_CALLER,
      &DeviceMachine::OnAwaitingMsOsInstall, NULL };

/* Sets the U2 timeout of an enumerated SuperSpeed device; m_Resume says where to go after */
const DeviceMachine::State DeviceMachine::SettingU2Timeout =
    { NULL, "SettingU2Timeout", 0,
      &DeviceMachine::OnSettingU2Timeout, &DeviceMachine::EnterSettingU2Timeout };

/* FUNCTIONS ******************************************************************/

/* Run Then from a state entered after the current one is left */
SM_RESULT
DeviceMachine::Then(
    _In_ DSM_THEN Next)
{
    m_Then = Next;
    return SmTransition(&Starting);
}

SM_RESULT
DeviceMachine::EnterWithMsOsInstall()
{
    return SmCall(&AwaitingMsOsInstall);
}

SM_RESULT
DeviceMachine::ToAwaitingDetachOrRemove(
    _In_ BOOLEAN Configured)
{
    m_EndpointsConfigured = Configured;
    return SmTransition(&AwaitingDetachOrRemove);
}

SM_RESULT
DeviceMachine::GoStoppedEnumerated()
{
    return SmTransition(&StoppedAddressed);
}

/* STOPPED ENUMERATED *********************************************************/

SM_RESULT
DeviceMachine::EnterStoppedAddressed()
{
    StartDriverWaitTimer();
    return SmCall(&AwaitingMsOsInstall);
}

SM_RESULT
DeviceMachine::OnStoppedAddressed(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TimerFired:
            if (DisableWhenUnused())
                return Wait(&DisablingDevice, &DeviceMachine::StoppedDisabledForNoDriver);

            ForceResetOnNextStart();
            return SmTransition(&StoppedReady);

        case DsmEvent::HubStopping:
            return Wait(&DisablingPort, &DeviceMachine::StoppedPortOffForHubStop);

        case DsmEvent::PortDetached:
            return DetachUnconfigured();

        case DsmEvent::ClientUnconfigure:
            return Request(&RequestingCritical, &DeviceMachine::SetNullConfiguration,
                           &DeviceMachine::UnconfiguredByClient);

        case DsmEvent::ClientResetDevice:
            return ReenumForClientUnconfigured();

        case DsmEvent::ClientSelectConfig:
            m_FromConfigured = FALSE;
            return AtPassive(&DeviceMachine::SelectConfigValidate);

        case DsmEvent::PdoPreStart:
            return Then(&DeviceMachine::StoppedAckPreStart);

        case DsmEvent::PdoCleanup:
            return Wait(&DisablingPort, &DeviceMachine::CleanupPortDisabled);

        case DsmEvent::HubSuspending:
            return Wait(&SuspendingPort, &DeviceMachine::StoppedPortSuspendedForHub);

        case DsmEvent::PdoPowerUp:
            return Then(&DeviceMachine::StoppedPoweredUp);

        case DsmEvent::DeviceTextQuery:
            m_Resume = &DeviceMachine::GoStoppedEnumerated;
            return SmTransition(&ReadingDeviceText);

        case DsmEvent::HubGetDescriptor:
            m_Resume = &DeviceMachine::GoStoppedEnumerated;
            return Request(&RequestingCritical, &DeviceMachine::GetDescriptorForHub,
                           &DeviceMachine::HubDescriptorDone);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::StoppedAckPreStart()
{
    CompletePreStart();
    AllowIo();
    return SmTransition(&StoppedAddressed);
}

SM_RESULT
DeviceMachine::StoppedPoweredUp()
{
    SignalPnpWaiter();
    return SmTransition(&UnconfiguredOn);
}

/* No driver loaded in time: disable the device and its port */
SM_RESULT
DeviceMachine::StoppedDisabledForNoDriver(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return Wait(&DisablingPort, &DeviceMachine::StoppedPortOffForNoDriver);
}

SM_RESULT
DeviceMachine::StoppedPortOffForNoDriver(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            return AtPassive(&DeviceMachine::ReportMissingAndLeave);

        case DsmEvent::PortFault:
        case DsmEvent::PortDisableDone:
            return SmTransition(&Stopped);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::StoppedPortOffForHubStop(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            DereferenceDevicePower();
            return DetachUnconfigured();

        case DsmEvent::PortFault:
        case DsmEvent::PortDisableDone:
            return Wait(&DisablingDevice, &DeviceMachine::StoppedDisabledForHub);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::StoppedDisabledForHub(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    DereferenceDevicePower();
    return SmTransition(&StoppedHubSuspended);
}

SM_RESULT
DeviceMachine::StoppedPortSuspendedForHub(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortSuspendDone:
            return SmTransition(&StoppedEnumeratedHubSuspended);

        case DsmEvent::PortFault:
            DereferenceDevicePower();
            return ToAwaitingDetachOrRemove(FALSE);

        case DsmEvent::PortDetached:
            DereferenceDevicePower();
            return DetachUnconfigured();

        default:
            return SmUnhandled();
    }
}

/* Client asked for no configuration; m_Resume says where the flow goes after */
SM_RESULT
DeviceMachine::UnconfiguredByClient(
    _In_ DsmEvent Event)
{
    if ((Event != DsmEvent::TransferDone) && (Event != DsmEvent::TransferFailed))
        return SmUnhandled();

    CompleteClientRequest();
    return BackToUnconfigured();
}

/* Started PDOs go to the unconfigured D0 state, others back to waiting for a start */
SM_RESULT
DeviceMachine::BackToUnconfigured()
{
    if (PdoStarted())
        return SmTransition(&UnconfiguredOn);

    return SmTransition(&StoppedAddressed);
}

SM_RESULT
DeviceMachine::HubDescriptorDone(
    _In_ DsmEvent Event)
{
    if ((Event != DsmEvent::TransferDone) && (Event != DsmEvent::TransferFailed))
        return SmUnhandled();

    return (this->*m_Resume)();
}

/* DEVICE TEXT ****************************************************************/

SM_RESULT
DeviceMachine::EnterReadingDeviceText()
{
    return CallRequest(&RequestingCritical, &DeviceMachine::ReadProductName,
                       &DeviceMachine::DeviceTextRead);
}

SM_RESULT
DeviceMachine::DeviceTextRead(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferFailed:
            break;

        case DsmEvent::TransferDone:
            (VOID)ProductNameValid();
            break;

        default:
            return SmUnhandled();
    }

    CompleteQueryText();
    return EndWith(DsmEvent::Succeeded);
}

SM_RESULT
DeviceMachine::OnReadingDeviceText(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::Succeeded)
        return SmUnhandled();

    return (this->*m_Resume)();
}

/* MICROSOFT OS EXTENSION INSTALL *********************************************/

SM_RESULT
DeviceMachine::OnAwaitingMsOsInstall(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::PdoInstallMsOsExt)
        return SmUnhandled();

    return AtPassive(&DeviceMachine::MsOsInstallStart);
}

SM_RESULT
DeviceMachine::MsOsInstallStart()
{
    BOOLEAN wanted = ExtPropertiesWanted();

    MarkMsOsInstallHandled();
    if (wanted)
    {
        if (!SetExtPropertiesSemaphore())
            return MsOsInstallDone();

        return Request(&RequestingCritical, &DeviceMachine::ReadExtPropertiesHeader,
                       &DeviceMachine::MsOsExtHeaderRead);
    }

    if (MsOs20ValuesWanted() && InstallMsOs20Values())
        (VOID)SetExtPropertiesSemaphore();

    return MsOsInstallDone();
}

SM_RESULT
DeviceMachine::MsOsInstallDone()
{
    SignalPnpWaiter();
    return SmTransition(&AwaitingMsOsInstall);
}

SM_RESULT
DeviceMachine::MsOsExtHeaderRead(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            if (ExtPropertiesHeaderValid() && AllocateExtPropertiesBuffer())
            {
                return Request(&RequestingCritical, &DeviceMachine::ReadExtProperties,
                               &DeviceMachine::MsOsExtRead);
            }

            return MsOsInstallDone();

        case DsmEvent::TransferFailed:
            return MsOsInstallDone();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::MsOsExtRead(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            if (ExtPropertiesValid())
                return AtPassive(&DeviceMachine::MsOsWriteProperties);

            return MsOsFreeAndDone();

        case DsmEvent::TransferFailed:
            return MsOsFreeAndDone();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::MsOsWriteProperties()
{
    (VOID)WriteCustomProperties();
    return MsOsFreeAndDone();
}

SM_RESULT
DeviceMachine::MsOsFreeAndDone()
{
    FreeExtPropertiesBuffer();
    return MsOsInstallDone();
}

/* STOPPED ENABLED AND STOPPED ************************************************/

SM_RESULT
DeviceMachine::OnStoppedReady(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ClientSelectConfig:
        case DsmEvent::ClientResetPipe:
        case DsmEvent::ClientClearStall:
        case DsmEvent::ClientResetDevice:
        case DsmEvent::ClientUnconfigure:
        case DsmEvent::ClientSyncResetPipe:
        case DsmEvent::ClientSetInterface:
            FailClientRequest();
            return SmHandled();

        case DsmEvent::ClientCyclePort:
            CyclePortForClient();
            return SmHandled();

        case DsmEvent::DeviceTextQuery:
            SignalQueryText();
            return SmHandled();

        case DsmEvent::HubGetDescriptor:
            FailHubDescriptorRequest();
            return SmHandled();

        case DsmEvent::ClientStreams:
            return ForwardStreams(&StoppedReady);

        case DsmEvent::PdoPowerUp:
            return ReenumForStart(DsmReenumUnconfigured);

        case DsmEvent::PdoPreStart:
            AllowIo();
            CompletePreStart();
            return SmTransition(&StoppedReady);

        case DsmEvent::PortDetached:
            return DetachUnconfigured();

        case DsmEvent::PdoCleanup:
            return Wait(&DisablingPort, &DeviceMachine::CleanupPortDisabled);

        case DsmEvent::LpmSettingChanged:
            m_Resume = &DeviceMachine::GoStoppedEnabled;
            return SmTransition(&SettingU2Timeout);

        case DsmEvent::HubStopping:
        case DsmEvent::HubSuspending:
            return Wait(&DisablingDevice, &DeviceMachine::StoppedEnabledDisabledForHub);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::GoStoppedEnabled()
{
    return SmTransition(&StoppedReady);
}

SM_RESULT
DeviceMachine::StoppedEnabledDisabledForHub(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    RequestPortDisable();
    return Wait(&AwaitingPort, &DeviceMachine::StoppedPortOffAfterDisable);
}

/* Port off after the device was disabled for a hub stop or suspend */
SM_RESULT
DeviceMachine::StoppedPortOffAfterDisable(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            return AtPassive(&DeviceMachine::ReportMissingAndLeave);

        case DsmEvent::PortFault:
        case DsmEvent::PortDisableDone:
            DereferenceDevicePower();
            return SmTransition(&StoppedHubSuspended);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::OnStopped(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ClientResetDevice:
        case DsmEvent::ClientSyncResetPipe:
        case DsmEvent::ClientSetInterface:
        case DsmEvent::ClientClearStall:
        case DsmEvent::ClientUnconfigure:
        case DsmEvent::ClientResetPipe:
        case DsmEvent::ClientSelectConfig:
            FailClientRequest();
            return SmHandled();

        case DsmEvent::DeviceTextQuery:
            SignalQueryText();
            return SmHandled();

        case DsmEvent::HubGetDescriptor:
            FailHubDescriptorRequest();
            return SmHandled();

        case DsmEvent::ClientCyclePort:
            CyclePortForClient();
            return SmHandled();

        case DsmEvent::PdoPowerUp:
            return ReenumForStart(DsmReenumRestart);

        case DsmEvent::HubStopping:
        case DsmEvent::HubSuspending:
            return Then(&DeviceMachine::StoppedDropPower);

        case DsmEvent::ClientStreams:
            return ForwardStreams(&Stopped);

        case DsmEvent::PortDetached:
            return AtPassive(&DeviceMachine::ReportMissingAndLeave);

        case DsmEvent::PdoPreStart:
            return ReenumForPreStart();

        case DsmEvent::PdoCleanup:
            return AtPassive(&DeviceMachine::RemovalDeleteForgetPortOn);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::StoppedDropPower()
{
    DereferenceDevicePower();
    return SmTransition(&StoppedHubSuspended);
}

/* STOPPED WITH THE HUB SUSPENDED *********************************************/

SM_RESULT
DeviceMachine::OnStoppedHubSuspended(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ClientUnconfigure:
        case DsmEvent::ClientResetPipe:
        case DsmEvent::ClientSetInterface:
        case DsmEvent::ClientResetDevice:
        case DsmEvent::ClientSelectConfig:
        case DsmEvent::ClientClearStall:
        case DsmEvent::ClientSyncResetPipe:
            FailClientRequest();
            return SmHandled();

        case DsmEvent::HubGetDescriptor:
            FailHubDescriptorRequest();
            return SmHandled();

        case DsmEvent::PdoPreStart:
            AckPreStart();
            return SmHandled();

        case DsmEvent::DeviceTextQuery:
            SignalQueryText();
            return SmHandled();

        case DsmEvent::ClientCyclePort:
            CyclePortForClient();
            return SmHandled();

        case DsmEvent::LpmSettingChanged:
            return SmHandled();

        case DsmEvent::PdoInstallMsOsExt:
            ReleasePnpWaiter();
            return SmHandled();

        case DsmEvent::ResumedWithHub:
        case DsmEvent::NeedsReenumeration:
        case DsmEvent::SuspendedAfterHubResume:
            return Wait(&DisablingPort, &DeviceMachine::StoppedPortOffAfterHubResume);

        case DsmEvent::HubStoppedHoldingReference:
            return Then(&DeviceMachine::StoppedDropPower);

        case DsmEvent::ClientStreams:
            return ForwardStreams(&StoppedHubSuspended);

        case DsmEvent::PortDetached:
            return AtPassive(&DeviceMachine::ReportMissingAndLeave);

        case DsmEvent::PdoCleanup:
            return AtPassive(&DeviceMachine::RemovalDeleteForgetPortOn);

        case DsmEvent::HubStoppingAfterSuspend:
            return Then(&DeviceMachine::StoppedAckHubStop);

        case DsmEvent::HubStarted:
            return Then(&DeviceMachine::StoppedHubStarted);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::StoppedAckHubStop()
{
    ConfirmStopWhileSuspended();
    return SmTransition(&StoppedHubSuspended);
}

SM_RESULT
DeviceMachine::StoppedHubStarted()
{
    ReferenceDevicePower();
    return SmTransition(&Stopped);
}

SM_RESULT
DeviceMachine::StoppedPortOffAfterHubResume(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDisableDone:
        case DsmEvent::PortFault:
            return SmTransition(&Stopped);

        case DsmEvent::HubStopping:
        case DsmEvent::HubSuspending:
            return Wait(&AwaitingPort, &DeviceMachine::StoppedPortOffAfterDisable);

        case DsmEvent::PortDetached:
            return AtPassive(&DeviceMachine::ReportMissingAndLeave);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnterHubSuspendedReleasingPower()
{
    DereferenceDevicePower();
    return SmCall(&HubSuspended);
}

SM_RESULT
DeviceMachine::OnStoppedEnumeratedHubSuspended(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::HubGetDescriptor:
            FailHubDescriptorRequest();
            return SmHandled();

        case DsmEvent::PdoPreStart:
            AckPreStart();
            return SmHandled();

        case DsmEvent::PdoInstallMsOsExt:
            ReleasePnpWaiter();
            return SmHandled();

        case DsmEvent::HubStoppedHoldingReference:
        case DsmEvent::HubStopping:
            return Wait(&DisablingDevice, &DeviceMachine::StoppedDisabledForHub);

        case DsmEvent::ResumedWithHub:
            return SmTransition(&StoppedAddressed);

        case DsmEvent::SuspendedAfterHubResume:
            return Wait(&ResumingPort, &DeviceMachine::StoppedPortResumed);

        case DsmEvent::NeedsReenumeration:
            return ReenumForHubStart();

        case DsmEvent::PortDetached:
            return DetachUnconfigured();

        case DsmEvent::PdoCleanup:
            return Wait(&DisablingDevice, &DeviceMachine::StoppedDisabledForCleanup);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::StoppedDisabledForCleanup(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return AtPassive(&DeviceMachine::RemovalDeleteForgetPortOn);
}

SM_RESULT
DeviceMachine::StoppedPortResumed(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortResumeDone:
        case DsmEvent::PortNowDisabled:
        case DsmEvent::PortNowEnabledOnReconnect:
            return SmTransition(&StoppedAddressed);

        case DsmEvent::PortResumeTimeout:
            return ReenumForHubStart();

        case DsmEvent::HubStopping:
            return Wait(&AwaitingPort, &DeviceMachine::StoppedResumeEndedForStop);

        case DsmEvent::PortDetached:
            return DetachUnconfigured();

        case DsmEvent::PortFault:
            return ToAwaitingDetachOrRemove(FALSE);

        case DsmEvent::HubSuspending:
            return Wait(&AwaitingPort, &DeviceMachine::StoppedResumeEndedForSuspend);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::StoppedResumeEndedForStop(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortResumeDone:
        case DsmEvent::PortNowDisabled:
        case DsmEvent::PortNowEnabledOnReconnect:
            return Wait(&DisablingPort, &DeviceMachine::StoppedPortOffForHubStop);

        case DsmEvent::PortFault:
        case DsmEvent::PortResumeTimeout:
            return ToAwaitingDetachOrRemove(FALSE);

        case DsmEvent::PortDetached:
            return DetachUnconfigured();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::StoppedResumeEndedForSuspend(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortFault:
        case DsmEvent::PortResumeTimeout:
            return ToAwaitingDetachOrRemove(FALSE);

        case DsmEvent::PortResumeDone:
            return Wait(&SuspendingPort, &DeviceMachine::StoppedPortSuspendedForHub);

        case DsmEvent::PortDetached:
            return DetachUnconfigured();

        default:
            return SmUnhandled();
    }
}

/* U2 TIMEOUT FOR AN ENUMERATED DEVICE ****************************************/

SM_RESULT
DeviceMachine::EnterSettingU2Timeout()
{
    if (KindWithin(DSM_KIND_USB3X | DSM_KIND_ANY_SPEED | DSM_KIND_ANY_PORT))
        return CallStep(&DeviceMachine::U2TimeoutStart);

    return (this->*m_Resume)();
}

SM_RESULT
DeviceMachine::OnSettingU2Timeout(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
        case DsmEvent::Failed:
            return (this->*m_Resume)();

        case DsmEvent::PortDetached:
            return DetachUnconfigured();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::U2TimeoutStart()
{
    if (!U2NeededForEnumerated())
        return EndWith(DsmEvent::Succeeded);

    RequestU2Timeout();
    return Wait(&AwaitingPortYielding, &DeviceMachine::U2TimeoutSet);
}

SM_RESULT
DeviceMachine::U2TimeoutSet(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortTimeoutSet:
            return EndWith(DsmEvent::Succeeded);

        case DsmEvent::PortFault:
            return EndWith(DsmEvent::Failed);

        default:
            return SmUnhandled();
    }
}
