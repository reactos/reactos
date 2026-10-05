/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Device state machine, unconfigured device in D0, selective
 *              suspend and resume
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include "usbhub3.h"

/* GLOBALS ********************************************************************/

static const DsmEvent PowerLpmDiscards[] = { DsmEvent::LpmSettingChanged, DsmEvent::Count };

/* PDO started, device in D0 with no configuration selected */
const DeviceMachine::State DeviceMachine::UnconfiguredOn =
    { NULL, "UnconfiguredOn", SM_STATE_TAKES_REQUESTS, &DeviceMachine::OnUnconfiguredOn, NULL };

/* PDO leaving D0, running the suspend sub machine */
const DeviceMachine::State DeviceMachine::SuspendingUnconfigured =
    { NULL, "SuspendingUnconfigured", 0,
      &DeviceMachine::OnSuspendingUnconfigured, &DeviceMachine::EnterSuspendingUnconfigured };

/* PDO in a low power state, device suspended at its port */
const DeviceMachine::State DeviceMachine::SuspendedUnconfigured =
    { NULL, "SuspendedUnconfigured", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnSuspendedUnconfigured, &DeviceMachine::EnterSuspended, PowerLpmDiscards };

/* Device suspended because its hub suspended */
const DeviceMachine::State DeviceMachine::UnconfiguredHubSuspended =
    { NULL, "UnconfiguredHubSuspended", 0,
      &DeviceMachine::OnUnconfiguredHubSuspended, &DeviceMachine::EnterHubSuspendedReleasingPower };

/* Suspend sub machine: the device suspended at its port */
const DeviceMachine::State DeviceMachine::DeviceSuspended =
    { NULL, "DeviceSuspended", SM_STATE_YIELDS_TO_CALLER, &DeviceMachine::OnDeviceSuspended, NULL };

/* Suspended while the hub is suspended too; m_D3Cold once the port lost power */
const DeviceMachine::State DeviceMachine::SuspendedHubSuspended =
    { NULL, "SuspendedHubSuspended", SM_STATE_YIELDS_TO_CALLER,
      &DeviceMachine::OnSuspendedHubSuspended, &DeviceMachine::EnterHubSuspendedReleasingPower };

/* The device must be enumerated again once PnP powers it up */
const DeviceMachine::State DeviceMachine::AwaitingPowerUpForReset =
    { NULL, "AwaitingPowerUpForReset", SM_STATE_YIELDS_TO_CALLER,
      &DeviceMachine::OnAwaitingPowerUpForReset, NULL };

/* The device woke the system; waiting for PnP to power it up */
const DeviceMachine::State DeviceMachine::AwaitingPowerUpAfterWake =
    { NULL, "AwaitingPowerUpAfterWake", SM_STATE_YIELDS_TO_CALLER,
      &DeviceMachine::OnAwaitingPowerUpAfterWake, &DeviceMachine::EnterAwaitingPowerUpAfterWake };

/* The port lost power while the device was suspended */
const DeviceMachine::State DeviceMachine::OffAfterD3Cold =
    { NULL, "OffAfterD3Cold", SM_STATE_YIELDS_TO_CALLER,
      &DeviceMachine::OnOffAfterD3Cold, NULL };

/* FUNCTIONS ******************************************************************/

SM_RESULT
DeviceMachine::GoUnconfiguredOn()
{
    return SmTransition(&UnconfiguredOn);
}

/* UNCONFIGURED IN D0 *********************************************************/

SM_RESULT
DeviceMachine::OnUnconfiguredOn(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ClientResetPipe:
        case DsmEvent::ClientClearStall:
        case DsmEvent::ClientSetInterface:
        case DsmEvent::ClientSyncResetPipe:
            FailClientRequest();
            return SmHandled();

        case DsmEvent::DeviceTextQuery:
            SignalQueryText();
            return SmHandled();

        case DsmEvent::ClientCyclePort:
            CyclePortForClient();
            return SmHandled();

        case DsmEvent::NoPingResponse:
            return SmHandled();

        case DsmEvent::HubSuspending:
            return Wait(&SuspendingPort, &DeviceMachine::UnconfiguredSuspendedForHub);

        case DsmEvent::HubGetDescriptor:
            m_Resume = &DeviceMachine::GoUnconfiguredOn;
            return Request(&RequestingCritical, &DeviceMachine::GetDescriptorForHub,
                           &DeviceMachine::HubDescriptorDone);

        case DsmEvent::ClientUnconfigure:
            return Request(&RequestingCritical, &DeviceMachine::SetNullConfiguration,
                           &DeviceMachine::UnconfiguredByClient);

        case DsmEvent::ClientResetDevice:
            return ReenumForClientUnconfigured();

        case DsmEvent::ClientSelectConfig:
            m_FromConfigured = FALSE;
            return AtPassive(&DeviceMachine::SelectConfigValidate);

        case DsmEvent::PdoPowerDown:
            return SmTransition(&SuspendingUnconfigured);

        case DsmEvent::PortDetached:
            return DetachUnconfigured();

        case DsmEvent::LpmSettingChanged:
            m_Resume = &DeviceMachine::GoUnconfiguredOn;
            return SmTransition(&SettingU2Timeout);

        case DsmEvent::PdoPowerDownFinal:
            if (DisableWhenUnused())
                return Wait(&DisablingDevice, &DeviceMachine::UnconfiguredDisabledForStop);

            SignalPnpWaiter();
            ForceResetOnNextStart();
            return SmTransition(&StoppedReady);

        case DsmEvent::ClientStreams:
            return ForwardStreams(&UnconfiguredOn);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::UnconfiguredSuspendedForHub(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortFault:
            DereferenceDevicePower();
            return ToAwaitingDetachOrRemove(FALSE);

        case DsmEvent::PortSuspendDone:
            return SmTransition(&UnconfiguredHubSuspended);

        case DsmEvent::PortDetached:
            return DetachUnconfigured();

        default:
            return SmUnhandled();
    }
}

/* PDO stopped: the device is disabled and its port turned off */
SM_RESULT
DeviceMachine::UnconfiguredDisabledForStop(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return Wait(&DisablingPort, &DeviceMachine::UnconfiguredPortOffForStop);
}

SM_RESULT
DeviceMachine::UnconfiguredPortOffForStop(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            SignalPnpWaiter();
            return AtPassive(&DeviceMachine::ReportMissingAndLeave);

        case DsmEvent::PortDisableDone:
        case DsmEvent::PortFault:
            SignalPnpWaiter();
            return SmTransition(&Stopped);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnterSuspendingUnconfigured()
{
    ClearResetAtResume();
    return CallStep(&DeviceMachine::SuspendStart);
}

SM_RESULT
DeviceMachine::OnSuspendingUnconfigured(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
        case DsmEvent::Failed:
            return SmTransition(&SuspendedUnconfigured);

        case DsmEvent::HubSuspending:
            SignalPnpWaiter();
            return SmTransition(&UnconfiguredHubSuspended);

        case DsmEvent::PortDetached:
            SignalPnpWaiter();
            return DetachUnconfigured();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnterSuspended()
{
    SignalPnpWaiter();
    return SmCall(&DeviceSuspended);
}

SM_RESULT
DeviceMachine::OnSuspendedUnconfigured(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ClientSetInterface:
        case DsmEvent::ClientResetPipe:
        case DsmEvent::ClientSyncResetPipe:
        case DsmEvent::ClientUnconfigure:
        case DsmEvent::ClientSelectConfig:
        case DsmEvent::ClientResetDevice:
        case DsmEvent::ClientClearStall:
            FailClientRequest();
            return SmHandled();

        case DsmEvent::NoPingResponse:
            return SmHandled();

        case DsmEvent::DeviceTextQuery:
            SignalQueryText();
            return SmHandled();

        case DsmEvent::ClientCyclePort:
            CyclePortForClient();
            return SmHandled();

        case DsmEvent::HubGetDescriptor:
            FailHubDescriptorRequest();
            return SmHandled();

        case DsmEvent::NeedsReenumeration:
            return ReenumForResume(FALSE);

        case DsmEvent::ResumeDone:
            SignalPnpWaiter();
            return SmTransition(&UnconfiguredOn);

        case DsmEvent::PdoCleanup:
            m_EndpointsConfigured = FALSE;
            return DisableForRemoval(&DeviceMachine::RemovalDeleteForgetAndPark);

        case DsmEvent::PortDetached:
            return DetachUnconfigured();

        case DsmEvent::HubStopping:
            m_EndpointsConfigured = FALSE;
            return DisableForRemoval(&DeviceMachine::RemovalDropPowerAndPark);

        case DsmEvent::ClientStreams:
            return ForwardStreams(&SuspendedUnconfigured);

        case DsmEvent::HubStoppingAfterSuspend:
            m_EndpointsConfigured = FALSE;
            return DisableForRemoval(&DeviceMachine::RemovalAckAndPark);

        case DsmEvent::PortFault:
            return ToAwaitingDetachOrRemove(FALSE);

        case DsmEvent::PdoPreStart:
            return Wait(&DisablingDevice, &DeviceMachine::UnconfiguredDisabledForRestart);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::UnconfiguredDisabledForRestart(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return ReenumForPreStart();
}

SM_RESULT
DeviceMachine::OnUnconfiguredHubSuspended(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::NeedsReenumeration:
            return ReenumForHubResume(FALSE);

        case DsmEvent::ResumedWithHub:
            return SmTransition(&UnconfiguredOn);

        case DsmEvent::SuspendedAfterHubResume:
            return Wait(&ResumingPort, &DeviceMachine::UnconfiguredResumedWithHub);

        case DsmEvent::PortDetached:
            return DetachUnconfigured();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::UnconfiguredResumedWithHub(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortResumeDone:
            return SmTransition(&UnconfiguredOn);

        case DsmEvent::PortFault:
        case DsmEvent::PortResumeTimeout:
        case DsmEvent::PortNowDisabled:
        case DsmEvent::PortNowEnabledOnReconnect:
            return ToAwaitingDetachOrRemove(FALSE);

        case DsmEvent::PortDetached:
            return DetachUnconfigured();

        default:
            return SmUnhandled();
    }
}

/* SUSPEND SUB MACHINE ********************************************************/

/* Arm the device for wake if needed, flush its IO and suspend its port */
SM_RESULT
DeviceMachine::SuspendStart()
{
    if (ArmForWakeWanted())
    {
        return Request(&RequestingCritical, &DeviceMachine::AbortDeviceIo,
                       &DeviceMachine::SuspendIoAborted);
    }

    m_AfterPurge = &DeviceMachine::SuspendPortPlain;
    return Request(&RequestingCritical, &DeviceMachine::PurgeIoForSuspend,
                   &DeviceMachine::SuspendIoPurged);
}

SM_RESULT
DeviceMachine::SuspendIoAborted(
    _In_ DsmEvent Event)
{
    if ((Event != DsmEvent::ControllerIoctlDone) && (Event != DsmEvent::ControllerIoctlFailed))
        return SmUnhandled();

    return Request(&RequestingCritical, &DeviceMachine::ArmForWake,
                   &DeviceMachine::SuspendArmed);
}

SM_RESULT
DeviceMachine::SuspendArmed(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            m_AfterPurge = &DeviceMachine::SuspendPortArmed;
            break;

        case DsmEvent::TransferFailed:
            m_AfterPurge = &DeviceMachine::SuspendPortArmFailed;
            break;

        default:
            return SmUnhandled();
    }

    return Request(&RequestingCritical, &DeviceMachine::PurgeIoForSuspend,
                   &DeviceMachine::SuspendIoPurged);
}

SM_RESULT
DeviceMachine::SuspendIoPurged(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return Wait(&SuspendingPort, m_AfterPurge);
}

SM_RESULT
DeviceMachine::SuspendPortPlain(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortSuspendDone:
            return EndWith(DsmEvent::Succeeded);

        case DsmEvent::HubSuspending:
            return Wait(&AwaitingPort, &DeviceMachine::SuspendEndedForHub);

        case DsmEvent::PortFault:
            return EndWith(DsmEvent::Failed);

        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::SuspendPortArmed(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortFault:
            FinishWaitWake();
            return EndWith(DsmEvent::Failed);

        case DsmEvent::HubSuspending:
            return Wait(&AwaitingPort, &DeviceMachine::SuspendEndedForHub);

        case DsmEvent::PortSuspendDone:
            return EndWith(DsmEvent::Succeeded);

        case DsmEvent::PortDetached:
            FinishWaitWake();
            return EndWith(DsmEvent::PortDetached);

        default:
            return SmUnhandled();
    }
}

/* Arming failed: the port is still suspended but the suspend reports failure */
SM_RESULT
DeviceMachine::SuspendPortArmFailed(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortFault:
        case DsmEvent::PortSuspendDone:
            FinishWaitWake();
            return EndWith(DsmEvent::Failed);

        case DsmEvent::HubSuspending:
            return Wait(&AwaitingPort, &DeviceMachine::SuspendEndedForHub);

        case DsmEvent::PortDetached:
            FinishWaitWake();
            return EndWith(DsmEvent::PortDetached);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::SuspendEndedForHub(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortSuspendDone:
        case DsmEvent::PortFault:
        case DsmEvent::PortDisableDone:
            return EndWith(DsmEvent::HubSuspending);

        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        default:
            return SmUnhandled();
    }
}

/* SUSPENDED SUB MACHINE ******************************************************/

SM_RESULT
DeviceMachine::OnDeviceSuspended(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortNowEnabledOnReconnect:
            MarkResetAtResume();
            RequestPortResume();
            return SuspendedResumedByPort();

        case DsmEvent::HubSuspending:
            m_D3Cold = FALSE;
            return SmTransition(&SuspendedHubSuspended);

        case DsmEvent::PdoPowerUp:
            return Wait(&ResumingPort, &DeviceMachine::SuspendedPortResumed);

        case DsmEvent::PortResumeDone:
            RequestPortResume();
            return SuspendedResumedByPort();

        case DsmEvent::PortNowDisabled:
            MarkResetAtResume();
            RequestPortResume();
            FinishWaitWake();
            return SmTransition(&OffAfterD3Cold);

        default:
            return SmUnhandled();
    }
}

/* The port resumed on its own: a wake from an armed device or a stray resume */
SM_RESULT
DeviceMachine::SuspendedResumedByPort()
{
    if (DeviceArmedForWake())
    {
        return Request(&RequestingCritical, &DeviceMachine::StartDeviceIo,
                       &DeviceMachine::SuspendedIoStartedOnWake);
    }

    return Wait(&SuspendingPort, &DeviceMachine::SuspendedBackDown);
}

SM_RESULT
DeviceMachine::SuspendedIoStartedOnWake(
    _In_ DsmEvent Event)
{
    if ((Event != DsmEvent::ControllerIoctlDone) && (Event != DsmEvent::ControllerIoctlFailed))
        return SmUnhandled();

    return SmTransition(&AwaitingPowerUpAfterWake);
}

/* Device not armed for wake goes back to suspend */
SM_RESULT
DeviceMachine::SuspendedBackDown(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortSuspendDone:
            return SmTransition(&DeviceSuspended);

        case DsmEvent::HubSuspending:
            return Wait(&AwaitingPort, &DeviceMachine::SuspendedPortDownForHub);

        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::PortFault:
            return EndWith(DsmEvent::PortFault);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::SuspendedPortDownForHub(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortSuspendDone:
            m_D3Cold = FALSE;
            return SmTransition(&SuspendedHubSuspended);

        case DsmEvent::PortDetached:
            DereferenceDevicePower();
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::PortFault:
            DereferenceDevicePower();
            return EndWith(DsmEvent::PortFault);

        default:
            return SmUnhandled();
    }
}

/* PnP powers the device up: resume the port, then restart its IO */
SM_RESULT
DeviceMachine::SuspendedPortResumed(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortFault:
        case DsmEvent::PortResumeTimeout:
        case DsmEvent::PortResumeAbortedForSuspend:
            SignalPnpWaiter();
            return EndWith(DsmEvent::PortFault);

        case DsmEvent::PortResumeDone:
            return Request(&RequestingCritical, &DeviceMachine::StartDeviceIo,
                           &DeviceMachine::SuspendedIoStarted);

        case DsmEvent::PortDetached:
            SignalPnpWaiter();
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::PortNowDisabled:
        case DsmEvent::PortNowEnabledOnReconnect:
            MarkResetAtResume();
            return EndWith(DsmEvent::NeedsReenumeration);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::SuspendedIoStarted(
    _In_ DsmEvent Event)
{
    if ((Event != DsmEvent::ControllerIoctlDone) && (Event != DsmEvent::ControllerIoctlFailed))
        return SmUnhandled();

    if (DisarmOnResume())
    {
        FinishWaitWake();
        return Request(&RequestingCritical, &DeviceMachine::DisarmWake,
                       &DeviceMachine::SuspendedDisarmed);
    }

    return SuspendedResumeDone();
}

SM_RESULT
DeviceMachine::SuspendedDisarmed(
    _In_ DsmEvent Event)
{
    if ((Event != DsmEvent::TransferDone) && (Event != DsmEvent::TransferFailed))
        return SmUnhandled();

    return SuspendedResumeDone();
}

/* Resume finished; a reset on resume turns it into a re-enumeration */
SM_RESULT
DeviceMachine::SuspendedResumeDone()
{
    if (ResetOnResumeFromS0())
        return SuspendedNeedsReset();

    if (ResetAtResumeMarked())
        return EndWith(DsmEvent::NeedsReenumeration);

    return EndWith(DsmEvent::ResumeDone);
}

SM_RESULT
DeviceMachine::SuspendedNeedsReset()
{
    ForceResetOnNextStart();
    MarkResetAtResume();
    return EndWith(DsmEvent::NeedsReenumeration);
}

SM_RESULT
DeviceMachine::EnterAwaitingPowerUpAfterWake()
{
    FinishWaitWake();
    MarkSystemWakeSource();
    return SmHandled();
}

SM_RESULT
DeviceMachine::OnAwaitingPowerUpAfterWake(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PdoPowerUp:
            return Request(&RequestingCritical, &DeviceMachine::DisarmWake,
                           &DeviceMachine::SuspendedDisarmed);

        case DsmEvent::HubSuspending:
            return Request(&RequestingCritical, &DeviceMachine::PurgeIoForSuspend,
                           &DeviceMachine::SuspendedPurgedAfterWake);

        case DsmEvent::HubStopping:
            return Wait(&DisablingPort, &DeviceMachine::SuspendedPortOffForStop);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::SuspendedPurgedAfterWake(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    RequestPortSuspend();
    return Wait(&AwaitingPort, &DeviceMachine::SuspendedPortDownForHub);
}

SM_RESULT
DeviceMachine::SuspendedPortOffForStop(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::PortFault:
        case DsmEvent::PortDisableDone:
            return EndWith(DsmEvent::HubStopping);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::OnSuspendedHubSuspended(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::NeedsReenumeration:
            if (m_D3Cold)
                return SmTransition(&OffAfterD3Cold);

            return SmTransition(&AwaitingPowerUpForReset);

        case DsmEvent::ResumedWithHub:
            if (m_D3Cold)
                return SmTransition(&OffAfterD3Cold);

            return SuspendedResumedByPort();

        case DsmEvent::SuspendedAfterHubResume:
            if (m_D3Cold)
                return SmTransition(&OffAfterD3Cold);

            if (ResetAtResumeMarked())
                return SmTransition(&AwaitingPowerUpForReset);

            return SmTransition(&DeviceSuspended);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::OnAwaitingPowerUpForReset(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PdoPowerUp:
            return SuspendedNeedsReset();

        case DsmEvent::HubSuspending:
            return Request(&RequestingCritical, &DeviceMachine::PurgeIoForSuspend,
                           &DeviceMachine::SuspendedPurgedForReset);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::SuspendedPurgedForReset(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return Request(&Requesting, &DeviceMachine::DisablePortMarkingReset,
                   &DeviceMachine::SuspendedPortOffForReset);
}

/* The port is turned off and the device will be reset on its next resume */
VOID
DeviceMachine::DisablePortMarkingReset()
{
    RequestPortDisable();
    MarkResetAtResume();
}

SM_RESULT
DeviceMachine::SuspendedPortOffForReset(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            DereferenceDevicePower();
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::PortFault:
            return EndWith(DsmEvent::PortFault);

        case DsmEvent::PortDisableDone:
            m_D3Cold = FALSE;
            return SmTransition(&SuspendedHubSuspended);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::OnOffAfterD3Cold(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PdoPowerUp:
            return EndWith(DsmEvent::NeedsReenumeration);

        case DsmEvent::HubSuspending:
            m_D3Cold = TRUE;
            return SmTransition(&SuspendedHubSuspended);

        default:
            return SmUnhandled();
    }
}
