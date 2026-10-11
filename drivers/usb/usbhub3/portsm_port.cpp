/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Port state machine: states of a running hub port
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include "usbhub3.h"

/* GLOBALS ********************************************************************/

static const PortEvent DisconnectedDrops[] =
{
    PortEvent::HubRemoved, PortEvent::DeviceGone, PortEvent::HibernateRequest,
    PortEvent::CycleRequest, PortEvent::DisableRequest, PortEvent::DisableSuperSpeedRequest,
    PortEvent::ResetRequest, PortEvent::WarmResetRequest, PortEvent::ResumeRequest,
    PortEvent::SuspendRequest, PortEvent::SetU1Timeout, PortEvent::SetU2Timeout, PortEvent::Count
};

static const PortEvent RemovedOnly[] =
{
    PortEvent::HubRemoved, PortEvent::Count
};

static const PortEvent RemovedAndGone[] =
{
    PortEvent::HubRemoved, PortEvent::DeviceGone, PortEvent::Count
};

static const PortEvent RemovedAndLpm[] =
{
    PortEvent::HubRemoved, PortEvent::SetU1Timeout, PortEvent::SetU2Timeout, PortEvent::Count
};

static const PortEvent PortStatusAndRemoved[] =
{
    PortEvent::StatusChanged, PortEvent::HubRemoved, PortEvent::Count
};

/* Port has no device, or is bringing one up */
const PortMachine::State PortMachine::Empty =
    { NULL, "Empty", 0, &PortMachine::OnEmpty, NULL, NULL };

const PortMachine::State PortMachine::Disconnected =
    { &Empty, "Disconnected", SM_STATE_TAKES_REQUESTS | PSM_STATE_IDLE,
      NULL, &PortMachine::EnterDisconnected, DisconnectedDrops };

const PortMachine::State PortMachine::StartingEmpty =
    { &Empty, "StartingEmpty", 0,
      &PortMachine::OnStartingEmpty, &PortMachine::EnterStartingEmpty, RemovedOnly };

const PortMachine::State PortMachine::Debouncing =
    { NULL, "Debouncing", 0, &PortMachine::OnDebouncing, NULL, NULL };

const PortMachine::State PortMachine::DebouncingUsb2 =
    { &Debouncing, "DebouncingUsb2", 0, NULL, &PortMachine::EnterDebouncing, RemovedAndGone };

const PortMachine::State PortMachine::DebouncingUsb3 =
    { &Debouncing, "DebouncingUsb3", 0, NULL, &PortMachine::EnterDebouncing, RemovedOnly };

const PortMachine::State PortMachine::OldDeviceWait =
    { NULL, "OldDeviceWait", 0,
      &PortMachine::OnOldDeviceWait, &PortMachine::EnterWithPortChange, RemovedOnly };

const PortMachine::State PortMachine::OverCurrentWait =
    { NULL, "OverCurrentWait", 0,
      &PortMachine::OnOverCurrentWait, &PortMachine::EnterWithPortChange, RemovedAndGone };

const PortMachine::State PortMachine::NotifyingOverCurrent =
    { NULL, "NotifyingOverCurrent", SM_STATE_NEEDS_PASSIVE,
      NULL, &PortMachine::EnterNotifyingOverCurrent, NULL };

const PortMachine::State PortMachine::UserResetWait =
    { NULL, "UserResetWait", 0,
      &PortMachine::OnUserResetWait, &PortMachine::EnterWithPortChange, RemovedAndGone };

/* Timer could not be stopped, m_AfterTimer runs once it fires */
const PortMachine::State PortMachine::TimerFlush =
    { NULL, "TimerFlush", 0,
      &PortMachine::OnTimerFlush, &PortMachine::EnterWithPortChange, RemovedAndGone };

const PortMachine::State PortMachine::FlushBeforeOffEmpty =
    { NULL, "FlushBeforeOffEmpty", 0, &PortMachine::OnFlushBeforeOff, NULL, RemovedAndGone };

const PortMachine::State PortMachine::FlushBeforeOffAttached =
    { NULL, "FlushBeforeOffAttached", 0, &PortMachine::OnFlushBeforeOff, NULL, PortStatusAndRemoved };

const PortMachine::State PortMachine::FlushBeforeSuperSpeedOff =
    { NULL, "FlushBeforeSuperSpeedOff", 0, &PortMachine::OnFlushBeforeOff, NULL, RemovedOnly };

/* A device is on the port */
const PortMachine::State PortMachine::Attached =
    { NULL, "Attached", 0, &PortMachine::OnAttached, NULL, NULL };

const PortMachine::State PortMachine::Enabled =
    { &Attached, "Enabled", SM_STATE_TAKES_REQUESTS,
      &PortMachine::OnEnabled, &PortMachine::EnterWithPortChange, RemovedOnly };

const PortMachine::State PortMachine::AttachedDisabled =
    { &Attached, "AttachedDisabled", SM_STATE_TAKES_REQUESTS | PSM_STATE_IDLE,
      &PortMachine::OnAttachedDisabled, &PortMachine::EnterWithPortChange, RemovedOnly };

const PortMachine::State PortMachine::Suspended =
    { &Attached, "Suspended", SM_STATE_TAKES_REQUESTS | PSM_STATE_IDLE,
      &PortMachine::OnSuspended, &PortMachine::EnterSuspended, RemovedOnly };

const PortMachine::State PortMachine::DisabledInSuspend =
    { &Attached, "DisabledInSuspend", SM_STATE_TAKES_REQUESTS | PSM_STATE_IDLE,
      &PortMachine::OnDisabledInSuspend, &PortMachine::EnterWithPortChange, RemovedOnly };

const PortMachine::State PortMachine::SuspendedD3Cold =
    { &Attached, "SuspendedD3Cold", SM_STATE_TAKES_REQUESTS | PSM_STATE_IDLE,
      &PortMachine::OnDisabledInSuspend, &PortMachine::EnterSuspendedD3Cold, RemovedOnly };

const PortMachine::State PortMachine::ResumeAckWait =
    { &Attached, "ResumeAckWait", SM_STATE_TAKES_REQUESTS,
      &PortMachine::OnResumeAckWait, &PortMachine::EnterWithPortChange, RemovedOnly };

const PortMachine::State PortMachine::ResetRequestWait =
    { &Attached, "ResetRequestWait", SM_STATE_TAKES_REQUESTS,
      &PortMachine::OnResetRequestWait, &PortMachine::EnterResetWait, RemovedAndLpm };

const PortMachine::State PortMachine::StartingAttached =
    { &Attached, "StartingAttached", 0,
      &PortMachine::OnStartingAttached, &PortMachine::EnterStartingAttached, RemovedOnly };

const PortMachine::State PortMachine::ResumingAttached =
    { &Attached, "ResumingAttached", 0,
      &PortMachine::OnResumingAttached, &PortMachine::EnterResumingAttached, RemovedOnly };

const PortMachine::State PortMachine::ResumingDisabled =
    { &Attached, "ResumingDisabled", 0,
      &PortMachine::OnResumingDisabled, &PortMachine::EnterResumingAttached, RemovedOnly };

const PortMachine::State PortMachine::SystemResuming =
    { &Attached, "SystemResuming", 0,
      &PortMachine::OnSystemResuming, &PortMachine::EnterSystemResuming, RemovedOnly };

/* Not under Attached: hub stop is not expected while it reads status */
const PortMachine::State PortMachine::ErrorRestarting =
    { NULL, "ErrorRestarting", 0,
      &PortMachine::OnErrorRestarting, &PortMachine::EnterErrorRestarting, NULL };

const PortMachine::State PortMachine::ResumeAckWaitStopped =
    { NULL, "ResumeAckWaitStopped", SM_STATE_TAKES_REQUESTS,
      &PortMachine::OnResumeAckWaitStopped, NULL, RemovedOnly };

/* Attached, holding an interrupt reference with the port timer running */
const PortMachine::State PortMachine::Timed =
    { NULL, "Timed", 0, &PortMachine::OnTimed, NULL, NULL };

const PortMachine::State PortMachine::ResetWait =
    { &Timed, "ResetWait", 0, &PortMachine::OnResetWait, &PortMachine::EnterResetWait, NULL };

const PortMachine::State PortMachine::ResetEnabledWait =
    { &Timed, "ResetEnabledWait", 0,
      &PortMachine::OnResetEnabledWait, &PortMachine::EnterWithPortChange, NULL };

const PortMachine::State PortMachine::ResetFlush =
    { &Timed, "ResetFlush", 0,
      &PortMachine::OnResetFlush, &PortMachine::EnterWithPortChange, RemovedOnly };

const PortMachine::State PortMachine::ResumeWait =
    { &Timed, "ResumeWait", 0, &PortMachine::OnResumeWait, &PortMachine::EnterResumeWait, NULL };

const PortMachine::State PortMachine::ResumeRecovery =
    { &Timed, "ResumeRecovery", 0,
      &PortMachine::OnResumeRecovery, &PortMachine::EnterWithPortChange, RemovedOnly };

const PortMachine::State PortMachine::ResumeFlush =
    { &Timed, "ResumeFlush", 0,
      &PortMachine::OnResumeFlush, &PortMachine::EnterWithPortChange, RemovedOnly };

/* Waiting for the hub to stop or suspend after a hub reset was asked for */
const PortMachine::State PortMachine::HubResetIssued =
    { NULL, "HubResetIssued", SM_STATE_TAKES_REQUESTS,
      &PortMachine::OnHubResetIssued, &PortMachine::EnterHubResetIssued, RemovedOnly };

const PortMachine::State PortMachine::HubResetWait =
    { NULL, "HubResetWait", SM_STATE_TAKES_REQUESTS,
      &PortMachine::OnHubResetWait, &PortMachine::EnterDraining, RemovedOnly };

const PortMachine::State PortMachine::HubResetTimerWait =
    { NULL, "HubResetTimerWait", SM_STATE_TAKES_REQUESTS,
      &PortMachine::OnHubResetTimerWait, &PortMachine::EnterDraining, RemovedOnly };

const PortMachine::State PortMachine::EmptyHubResetWait =
    { NULL, "EmptyHubResetWait", SM_STATE_TAKES_REQUESTS,
      &PortMachine::OnEmptyHubResetWait, &PortMachine::EnterDrainingEmpty, RemovedOnly };

const PortMachine::State PortMachine::EmptyHubResetTimerWait =
    { NULL, "EmptyHubResetTimerWait", SM_STATE_TAKES_REQUESTS,
      &PortMachine::OnEmptyHubResetTimerWait, &PortMachine::EnterDrainingEmpty, RemovedOnly };

const PortMachine::State PortMachine::AttachFailedWait =
    { NULL, "AttachFailedWait", 0,
      &PortMachine::OnAttachFailedWait, &PortMachine::EnterDrainingEmpty, RemovedOnly };

const PortMachine::State PortMachine::SuperSpeedOffWait =
    { NULL, "SuperSpeedOffWait", 0,
      &PortMachine::OnSuperSpeedOffWait, &PortMachine::EnterDrainingEmpty, RemovedOnly };

const PortMachine::State PortMachine::SsLinkDisabled =
    { NULL, "SsLinkDisabled", SM_STATE_TAKES_REQUESTS | PSM_STATE_IDLE,
      &PortMachine::OnSsLinkDisabled, &PortMachine::EnterDrainingEmpty, RemovedOnly };

/* A port control transfer is in flight, only its completion may run */
const PortMachine::State PortMachine::Transfer =
    { NULL, "Transfer", SM_STATE_CRITICAL_ONLY, NULL, NULL, NULL };

const PortMachine::State PortMachine::PoweringPort =
    { &Transfer, "PoweringPort", 0,
      &PortMachine::OnPoweringPort, &PortMachine::EnterPoweringPort, RemovedOnly };

const PortMachine::State PortMachine::QueryingOverCurrent =
    { &Transfer, "QueryingOverCurrent", 0,
      &PortMachine::OnQueryingOverCurrent, &PortMachine::EnterReadingStatus, RemovedOnly };

const PortMachine::State PortMachine::SendingReset =
    { &Transfer, "SendingReset", 0,
      &PortMachine::OnSendingReset, &PortMachine::EnterSendingReset, NULL };

const PortMachine::State PortMachine::SendingWarmReset =
    { &Transfer, "SendingWarmReset", 0,
      &PortMachine::OnSendingReset, &PortMachine::EnterSendingWarmReset, NULL };

const PortMachine::State PortMachine::SendingResume =
    { &Transfer, "SendingResume", 0,
      &PortMachine::OnSendingResume, &PortMachine::EnterSendingResume, RemovedOnly };

const PortMachine::State PortMachine::SendingSuspend =
    { &Transfer, "SendingSuspend", 0,
      &PortMachine::OnSendingSuspend, &PortMachine::EnterSendingSuspend, RemovedOnly };

const PortMachine::State PortMachine::DisablingOnRequest =
    { &Transfer, "DisablingOnRequest", 0,
      &PortMachine::OnDisablingOnRequest, &PortMachine::EnterDisablingOnRequest, RemovedOnly };

const PortMachine::State PortMachine::DisablingForCycle =
    { &Transfer, "DisablingForCycle", 0,
      &PortMachine::OnDisablingForCycle, &PortMachine::EnterDisablingPort, RemovedOnly };

const PortMachine::State PortMachine::DisablingBeforeConnect =
    { &Transfer, "DisablingBeforeConnect", 0,
      &PortMachine::OnDisablingBeforeConnect, &PortMachine::EnterDisablingPort, RemovedOnly };

const PortMachine::State PortMachine::QuiescingForHubSuspend =
    { &Transfer, "QuiescingForHubSuspend", 0,
      &PortMachine::OnQuiescingForHubSuspend, &PortMachine::EnterQuiescingForHubSuspend,
      RemovedOnly };

const PortMachine::State PortMachine::DisablingForCycleTimed =
    { &Transfer, "DisablingForCycleTimed", 0,
      &PortMachine::OnDisablingForCycleTimed, &PortMachine::EnterDisablingPort, RemovedOnly };

const PortMachine::State PortMachine::DisablingOnSuspendTimed =
    { &Transfer, "DisablingOnSuspendTimed", 0,
      &PortMachine::OnDisablingOnSuspendTimed, &PortMachine::EnterDisablingPort, RemovedOnly };

const PortMachine::State PortMachine::DisablingOnResetTimeout =
    { &Transfer, "DisablingOnResetTimeout", 0,
      &PortMachine::OnDisablingOnResetTimeout, &PortMachine::EnterDisablingPort, RemovedOnly };

const PortMachine::State PortMachine::ReadingForResume =
    { &Transfer, "ReadingForResume", 0,
      &PortMachine::OnReadingForResume, &PortMachine::EnterReadingStatus, NULL };

const PortMachine::State PortMachine::ReadingAfterResumeTimeout =
    { &Transfer, "ReadingAfterResumeTimeout", 0,
      &PortMachine::OnReadingAfterResumeTimeout, &PortMachine::EnterReadingStatus, NULL };

const PortMachine::State PortMachine::SettingLpmTimeout =
    { &Transfer, "SettingLpmTimeout", 0,
      &PortMachine::OnSettingLpmTimeout, &PortMachine::EnterSettingLpmTimeout, NULL };

const PortMachine::State PortMachine::DisablingSuperSpeed =
    { &Transfer, "DisablingSuperSpeed", 0,
      &PortMachine::OnDisablingSuperSpeed, &PortMachine::EnterDisablingSuperSpeed, NULL };

const PortMachine::State PortMachine::DisablingSuperSpeedOnRequest =
    { &Transfer, "DisablingSuperSpeedOnRequest", 0,
      &PortMachine::OnDisablingSuperSpeedOnRequest,
      &PortMachine::EnterDisablingSuperSpeedOnRequest, NULL };

const PortMachine::State PortMachine::EnablingSuperSpeedOnStop =
    { &Transfer, "EnablingSuperSpeedOnStop", 0,
      &PortMachine::OnEnablingSuperSpeedOnStop, &PortMachine::EnterEnablingSuperSpeed, NULL };

const PortMachine::State PortMachine::EnablingSuperSpeedOnTimer =
    { &Transfer, "EnablingSuperSpeedOnTimer", 0,
      &PortMachine::OnEnablingSuperSpeedOnTimer, &PortMachine::EnterEnablingSuperSpeed, NULL };

const PortMachine::State PortMachine::ResetTimeoutStatus =
    { &Transfer, "ResetTimeoutStatus", 0,
      &PortMachine::OnResetTimeoutStatus, &PortMachine::EnterResetTimeoutStatus, NULL };

const PortMachine::State PortMachine::ArmingWake =
    { &Transfer, "ArmingWake", 0, &PortMachine::OnArmingWake, &PortMachine::EnterArmingWake, NULL };

/* FUNCTIONS ******************************************************************/

SM_RESULT
PortMachine::EnterWithPortChange()
{
    return SmCall(m_Usb3 ? &ChangeIdle30 : &ChangeIdle20);
}

/* EMPTY PORT *****************************************************************/

SM_RESULT
PortMachine::OnEmpty(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeConnect:
            return CheckConnection();

        case PortEvent::ChangeOverCurrent:
            return StartOverCurrentWait();

        case PortEvent::ChangeOverCurrentCleared:
            return SmTransition(&PoweringPort);

        case PortEvent::ChangeResetDone:
        case PortEvent::ChangeDisabled:
        case PortEvent::ChangeResumed:
        case PortEvent::ChangeError:
            if (m_Usb3)
                return SmUnhandled();
            return SmTransition(&Disconnected);

        case PortEvent::ChangeNeedsHubReset:
            if (!m_Usb3)
                return SmUnhandled();
            return ResetHubThen(&EmptyHubResetWait);

        case PortEvent::HubStopping:
            return PowerDown(&OffEmpty);

        case PortEvent::HubSuspending:
            if (!m_Usb3)
                return PowerDown(&OffEmpty);
            m_ArmForEmpty = TRUE;
            return SmTransition(&ArmingWake);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::EnterDisconnected()
{
    NotifyBootDeviceRemoval();
    return EnterWithPortChange();
}

SM_RESULT
PortMachine::EnterStartingEmpty()
{
    return SmCall(m_Usb3 ? &StartReadingFirst : &PowerOnSending);
}

SM_RESULT
PortMachine::OnStartingEmpty(
    _In_ PortEvent Event)
{
    if (!m_Usb3)
    {
        if (Event == PortEvent::SubDone)
            return SmTransition(&Disconnected);

        return SmUnhandled();
    }

    if (Event == PortEvent::ChangeNone)
        return SmTransition(&Disconnected);

    if (Event == PortEvent::ChangeResetBusy)
        return CheckOldDevice(OldDeviceResetInProgress);

    return SmUnhandled();
}

SM_RESULT
PortMachine::EnterDebouncing()
{
    StartTimer(PsmTimerDebounce);
    return EnterWithPortChange();
}

SM_RESULT
PortMachine::OnDebouncing(
    _In_ PortEvent Event)
{
    PortEvent response;

    switch (Event)
    {
        case PortEvent::ChangeOverCurrentCleared:
            return StopTimerThen(FollowOverCurrentCleared);

        case PortEvent::ChangeOverCurrent:
            return StopTimerThen(FollowOverCurrent);

        case PortEvent::ChangeConnect:
            return StopTimerThen(FollowConnect);

        case PortEvent::ChangeNeedsHubReset:
            if (!m_Usb3)
                return SmUnhandled();
            RequestHubReset();
            return SmTransition(StopTimer() ? &EmptyHubResetWait : &EmptyHubResetTimerWait);

        case PortEvent::ChangeDisabled:
        case PortEvent::ChangeResumed:
        case PortEvent::ChangeResetDone:
        case PortEvent::ChangeError:
            if (m_Usb3)
                return SmUnhandled();

            response = ErrorResponse();
            switch (response)
            {
                case PortEvent::ChangeNeedsHubReset:
                    RequestHubReset();
                    return SmTransition(StopTimer() ? &EmptyHubResetWait : &EmptyHubResetTimerWait);

                case PortEvent::ErrorIgnore:
                case PortEvent::ChangeConnect:
                    return StopTimerThen(FollowConnect);

                case PortEvent::ErrorCycle:
                    return StopTimerThen(FollowCycle);

                case PortEvent::ErrorDisableAndCycle:
                    return SmTransition(&DisablingForCycleTimed);

                case PortEvent::ChangeOverCurrentCleared:
                    return StopTimerThen(FollowOverCurrentCleared);

                case PortEvent::ChangeOverCurrent:
                    return StopTimerThen(FollowOverCurrent);

                default:
                    SM_BREAK("Port error response not valid here");
                    return SmHandled();
            }

        case PortEvent::TimerExpired:
            if (!m_Usb3)
                return CheckOldDevice(OldDevicePlain);
            return ReattachOrCreate(OldDevicePlain);

        case PortEvent::HubSuspending:
            if (m_Usb3)
                return SmTransition(&FlushBeforeOffEmpty);
            return SmTransition(&DisablingOnSuspendTimed);

        case PortEvent::HubStopping:
            if (m_Usb3)
                return SmTransition(&FlushBeforeOffEmpty);
            if (StopTimer())
                return PowerDown(&OffEmpty);
            return SmTransition(&FlushBeforeOffEmpty);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnOldDeviceWait(
    _In_ PortEvent Event)
{
    PortEvent response;

    switch (Event)
    {
        case PortEvent::ChangeOverCurrentCleared:
            return SmTransition(&PoweringPort);

        case PortEvent::ChangeConnect:
            return CheckConnection();

        case PortEvent::ChangeOverCurrent:
            return StartOverCurrentWait();

        case PortEvent::ChangeNeedsHubReset:
            if (!m_Usb3)
                return SmUnhandled();
            return ResetHubThen(&EmptyHubResetWait);

        case PortEvent::ChangeDisabled:
        case PortEvent::ChangeResumed:
        case PortEvent::ChangeResetDone:
        case PortEvent::ChangeError:
            if (m_Usb3)
                return SmUnhandled();

            response = ErrorResponse();
            switch (response)
            {
                case PortEvent::ChangeNeedsHubReset:
                    return ResetHubThen(&EmptyHubResetWait);

                case PortEvent::ErrorIgnore:
                    return SmTransition(&OldDeviceWait);

                case PortEvent::ErrorCycle:
                    return SmTransition(&DebouncingUsb2);

                case PortEvent::ChangeOverCurrent:
                    return StartOverCurrentWait();

                case PortEvent::ErrorDisableAndCycle:
                    return SmTransition(&DisablingBeforeConnect);

                default:
                    SM_BREAK("Port error response not valid here");
                    return SmHandled();
            }

        case PortEvent::HubStopping:
            return PowerDown(m_Usb3 ? &OffResetNeeded : &OffEmpty);

        case PortEvent::HubSuspending:
            if (m_Usb3 && (m_OldDevice == OldDeviceResetNeeded))
                return PowerDown(&OffResetNeeded);
            return SmTransition(&QuiescingForHubSuspend);

        case PortEvent::DeviceGone:
            return DeviceArrived(m_Usb3 ? m_OldDevice : OldDevicePlain);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnOverCurrentWait(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeError:
        case PortEvent::ChangeResetDone:
        case PortEvent::ChangeDisabled:
        case PortEvent::ChangeResumed:
            if (m_Usb3)
                return SmUnhandled();
            return SmTransition(&OverCurrentWait);

        case PortEvent::ChangeConnect:
            return SmTransition(&OverCurrentWait);

        case PortEvent::ChangeNeedsHubReset:
            if (!m_Usb3)
                return SmUnhandled();
            RequestHubReset();
            return SmTransition(StopTimer() ? &EmptyHubResetWait : &EmptyHubResetTimerWait);

        case PortEvent::ChangeOverCurrentCleared:
            return StopTimerThen(FollowOverCurrentCleared);

        case PortEvent::ChangeOverCurrent:
            return StopTimerThen(FollowOverCurrent);

        case PortEvent::TimerExpired:
            if (OverCurrentPersists())
                return SmTransition(&NotifyingOverCurrent);
            return SmTransition(&PoweringPort);

        case PortEvent::HubStopping:
        case PortEvent::HubSuspending:
            return SmTransition(&FlushBeforeOffEmpty);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::EnterNotifyingOverCurrent()
{
    NotifyUserOfOverCurrent();
    return SmTransition(&UserResetWait);
}

SM_RESULT
PortMachine::OnUserResetWait(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeError:
        case PortEvent::ChangeResetDone:
        case PortEvent::ChangeDisabled:
        case PortEvent::ChangeResumed:
            if (m_Usb3)
                return SmUnhandled();
            return SmTransition(&UserResetWait);

        case PortEvent::ChangeConnect:
            return SmTransition(&UserResetWait);

        case PortEvent::ChangeNeedsHubReset:
            if (!m_Usb3)
                return SmUnhandled();
            CancelUserOverCurrentReset();
            RequestHubReset();
            SmDiscardQueued(PortEvent::UserOverCurrentReset);
            return SmTransition(&EmptyHubResetWait);

        case PortEvent::ChangeOverCurrent:
            return StartOverCurrentWait();

        case PortEvent::ChangeOverCurrentCleared:
            CancelUserOverCurrentReset();
            SmDiscardQueued(PortEvent::UserOverCurrentReset);
            return SmTransition(&PoweringPort);

        case PortEvent::UserOverCurrentReset:
            return SmTransition(&PoweringPort);

        case PortEvent::HubStopping:
        case PortEvent::HubSuspending:
            CancelUserOverCurrentReset();
            SmDiscardQueued(PortEvent::UserOverCurrentReset);
            return PowerDown(&OffEmpty);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnTimerFlush(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::TimerExpired:
            return FollowTimer(m_AfterTimer);

        case PortEvent::ChangeOverCurrent:
            m_AfterTimer = FollowOverCurrent;
            return SmTransition(&TimerFlush);

        case PortEvent::ChangeOverCurrentCleared:
            m_AfterTimer = FollowOverCurrentCleared;
            return SmTransition(&TimerFlush);

        case PortEvent::ChangeConnect:
            /* After an over current the connect change waits for the timer too */
            if ((m_AfterTimer == FollowConnect) || (m_AfterTimer == FollowCycle))
                m_AfterTimer = FollowConnect;
            return SmTransition(&TimerFlush);

        case PortEvent::ChangeError:
        case PortEvent::ChangeResetDone:
        case PortEvent::ChangeDisabled:
        case PortEvent::ChangeResumed:
            if (m_Usb3)
                return SmUnhandled();
            return SmTransition(&TimerFlush);

        case PortEvent::ChangeNeedsHubReset:
            if (!m_Usb3)
                return SmUnhandled();
            return ResetHubThen(&EmptyHubResetTimerWait);

        case PortEvent::HubStopping:
            return SmTransition(&FlushBeforeOffEmpty);

        case PortEvent::HubSuspending:
            if (!m_Usb3 && (m_AfterTimer == FollowCycle))
                return SmTransition(&DisablingOnSuspendTimed);
            return SmTransition(&FlushBeforeOffEmpty);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnFlushBeforeOff(
    _In_ PortEvent Event)
{
    if (Event != PortEvent::TimerExpired)
        return SmUnhandled();

    if (SmCurrent() == &FlushBeforeOffAttached)
        return PowerDown(&OffAttached);

    return PowerDown(&OffEmpty);
}

/* ATTACHED *******************************************************************/

SM_RESULT
PortMachine::OnAttached(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeConnect:
        case PortEvent::ChangeOverCurrent:
        case PortEvent::ChangeOverCurrentCleared:
            return DetachThen(Event);

        case PortEvent::ChangeNeedsHubReset:
            if (!m_Usb3)
                return SmUnhandled();
            return SmTransition(&HubResetIssued);

        case PortEvent::HubStopping:
            return PowerDown(&OffAttached);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnEnabled(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeError:
        case PortEvent::ChangeDisabled:
        case PortEvent::ChangeResetDone:
        case PortEvent::ChangeResumed:
            if (m_Usb3)
                return SmUnhandled();
            return HandleConnectedFault(&Enabled);

        case PortEvent::DisableRequest:
            return SmTransition(&DisablingOnRequest);

        case PortEvent::SuspendRequest:
            return SmTransition(&SendingSuspend);

        case PortEvent::HibernateRequest:
            TellDevice(PsmNoticeSuspended);
            if (!m_Usb3)
                RecordPortSuspended();
            return SmTransition(&Suspended);

        case PortEvent::ResetRequest:
            return TakeInterruptForReset(&SendingReset, &Enabled);

        case PortEvent::CycleRequest:
            if (m_Usb3)
                return CycleDetach();
            DisconnectDevice();
            return SmTransition(&DisablingForCycle);

        case PortEvent::WarmResetRequest:
            if (!m_Usb3)
                return SmUnhandled();
            return TakeInterruptForReset(&SendingWarmReset, &Enabled);

        case PortEvent::SetU1Timeout:
        case PortEvent::SetU2Timeout:
            if (!m_Usb3)
                return SmUnhandled();
            m_LpmU2 = (Event == PortEvent::SetU2Timeout);
            return SmTransition(&SettingLpmTimeout);

        case PortEvent::DisableSuperSpeedRequest:
            if (!m_Usb3)
                return SmUnhandled();
            return SmTransition(&DisablingSuperSpeedOnRequest);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnAttachedDisabled(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeError:
        case PortEvent::ChangeDisabled:
        case PortEvent::ChangeResetDone:
        case PortEvent::ChangeResumed:
            if (m_Usb3)
                return SmUnhandled();
            return HandleConnectedFault(&AttachedDisabled);

        case PortEvent::DisableRequest:
            if (m_Usb3)
                return SmTransition(&DisablingOnRequest);
            TellDevice(PsmNoticeDisabled);
            RecordPortDisabled();
            return SmTransition(&AttachedDisabled);

        case PortEvent::CycleRequest:
            return CycleDetach();

        case PortEvent::HubSuspending:
            return PowerDown(&OffAttached);

        case PortEvent::ResetRequest:
            if (!m_Usb3)
                return TakeInterruptForReset(&SendingReset, &AttachedDisabled);
            return TakeInterruptForReset(&SendingWarmReset, &AttachedDisabled);

        case PortEvent::WarmResetRequest:
            if (!m_Usb3)
                return SmUnhandled();
            return TakeInterruptForReset(&SendingWarmReset, &AttachedDisabled);

        case PortEvent::DisableSuperSpeedRequest:
            if (!m_Usb3)
                return SmUnhandled();
            return SmTransition(&DisablingSuperSpeedOnRequest);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::EnterSuspended()
{
    return SmCall(m_Usb3 ? &ChangeIdle30 : &SuspendIdle);
}

SM_RESULT
PortMachine::OnSuspended(
    _In_ PortEvent Event)
{
    if (m_Usb3)
    {
        switch (Event)
        {
            case PortEvent::ChangeConnect:
                if (D3ColdEnabled())
                    return SmTransition(&SuspendedD3Cold);
                return DetachThen(Event);

            case PortEvent::DisableRequest:
                TellDevice(PsmNoticeDisabled);
                ForceResetOnEnumeration();
                return SmTransition(&DisabledInSuspend);

            case PortEvent::ResumeRequest:
                if (TakeInterruptReference())
                    return SmTransition(&ReadingForResume);
                TellDevice(PsmNoticeResumeRefused);
                DropInterruptReference();
                return SmTransition(&Suspended);

            default:
                return OnDisabledInSuspend(Event);
        }
    }

    switch (Event)
    {
        case PortEvent::ChangeResumed:
            return SmTransition(&ResumeAckWait);

        case PortEvent::ChangeResetDone:
        case PortEvent::ChangeError:
            return HandleConnectedFault(&Suspended);

        case PortEvent::ChangeDisabled:
            return SmTransition(&AttachedDisabled);

        case PortEvent::HubSuspending:
            return SmTransition(&OffSuspendedUsb2);

        case PortEvent::ResumeRequest:
            if (TakeInterruptReference())
                return SmTransition(&SendingResume);
            TellDevice(PsmNoticeResumeRefused);
            return SmTransition(&Suspended);

        case PortEvent::ResetRequest:
            return TakeInterruptForReset(&SendingReset, &AttachedDisabled);

        case PortEvent::CycleRequest:
            DisconnectDevice();
            return SmTransition(&DisablingForCycle);

        default:
            return SmUnhandled();
    }
}

/* 3.0 suspended states: suspended, suspended after a disable, and D3cold */
SM_RESULT
PortMachine::OnDisabledInSuspend(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeResumed:
            if (SmFrame(0) != &SuspendedD3Cold)
                return SmUnhandled();
            return SmTransition(&Enabled);

        case PortEvent::DisableRequest:
            TellDevice(PsmNoticeDisabled);
            ForceResetOnEnumeration();
            return SmTransition(&DisabledInSuspend);

        case PortEvent::HubSuspending:
            m_ArmForEmpty = FALSE;
            return SmTransition(&ArmingWake);

        case PortEvent::ResetRequest:
        case PortEvent::WarmResetRequest:
            return TakeInterruptForReset(&SendingWarmReset, &AttachedDisabled);

        case PortEvent::CycleRequest:
            return CycleDetach();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::EnterSuspendedD3Cold()
{
    return SmCall(&D3Idle);
}

SM_RESULT
PortMachine::OnResumeAckWait(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeError:
        case PortEvent::ChangeDisabled:
        case PortEvent::ChangeResetDone:
        case PortEvent::ChangeResumed:
            return HandleConnectedFault(&ResumeAckWait);

        case PortEvent::ResumeRequest:
            return SmTransition(&Enabled);

        case PortEvent::HubStopping:
            return SmTransition(&ResumeAckWaitStopped);

        case PortEvent::CycleRequest:
            DisconnectDevice();
            return SmTransition(&DisablingForCycle);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnResumeAckWaitStopped(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::ResumeRequest)
        return PowerDown(&OffAttached);

    if (Event == PortEvent::CycleRequest)
    {
        DisconnectDevice();
        return PowerDown(&OffEmpty);
    }

    return SmUnhandled();
}

SM_RESULT
PortMachine::OnResetRequestWait(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeResetDone:
            return SmTransition(&Enabled);

        case PortEvent::CycleRequest:
            return CycleDetach();

        case PortEvent::ResetRequest:
        case PortEvent::WarmResetRequest:
            return TakeInterruptForReset(&ResetWait, &ResetRequestWait);

        case PortEvent::DisableRequest:
            return SmTransition(&DisablingOnRequest);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::EnterStartingAttached()
{
    return SmCall(&PowerOnSending);
}

SM_RESULT
PortMachine::OnStartingAttached(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::SubDone:
            return SmTransition(&AttachedDisabled);

        case PortEvent::ChangeResetDone:
        case PortEvent::ChangeResumed:
        case PortEvent::ChangeError:
        case PortEvent::ChangeDisabled:
            return HandleConnectedFault(&AttachedDisabled);

        case PortEvent::HubSuspending:
            return PowerDown(&OffAttached);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::EnterResumingAttached()
{
    if ((SmCurrent() == &ResumingAttached) && m_WarmResume)
        return SmCall(&ResumeReading);

    return SmCall(&ResumePowering);
}

SM_RESULT
PortMachine::OnResumingAttached(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeError:
        case PortEvent::ChangeResetDone:
        case PortEvent::SubDone:
            TellDevice(PsmNoticeStateSuspended);
            return SmTransition(&Suspended);

        case PortEvent::ChangeResumed:
            TellDevice(PsmNoticeStateEnabled);
            return SmTransition(&Enabled);

        case PortEvent::ChangeDisabled:
            TellDevice(PsmNoticeStateDisabled);
            return SmTransition(&AttachedDisabled);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnResumingDisabled(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeError:
        case PortEvent::ChangeDisabled:
        case PortEvent::ChangeResetDone:
        case PortEvent::ChangeResumed:
        case PortEvent::SubDone:
            TellDevice(PsmNoticeStateDisabled);
            return SmTransition(&AttachedDisabled);

        case PortEvent::HubSuspending:
            return PowerDown(&OffAttached);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::EnterSystemResuming()
{
    return SmCall(&SystemResumeReading);
}

SM_RESULT
PortMachine::OnSystemResuming(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeResumed:
            return SmTransition(&Enabled);

        case PortEvent::SubDone:
            return SmTransition(&Suspended);

        case PortEvent::ChangeResetBusy:
            return SmTransition(&ResetRequestWait);

        case PortEvent::ChangeNeedsReset:
            return SmTransition(&AttachedDisabled);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::EnterErrorRestarting()
{
    return CallStatusRead(ReadStartResume);
}

SM_RESULT
PortMachine::OnErrorRestarting(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeResetBusy:
            return SmTransition(&ResetRequestWait);

        case PortEvent::ChangeResumed:
        case PortEvent::ChangeLinkError:
        case PortEvent::ChangeError:
        case PortEvent::ChangeNone:
        case PortEvent::ChangeResetDone:
            return SmTransition(&AttachedDisabled);

        case PortEvent::HubStopping:
            return SmUnhandled();

        default:
            return OnAttached(Event);
    }
}

/* TIMED **********************************************************************/

SM_RESULT
PortMachine::OnTimed(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeConnect:
        case PortEvent::ChangeOverCurrent:
        case PortEvent::ChangeOverCurrentCleared:
            return DetachTimed(Event);

        case PortEvent::ChangeNeedsHubReset:
            if (!m_Usb3)
                return SmUnhandled();
            return TimedHubReset();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::EnterResetWait()
{
    if (!m_Usb3)
        return SmCall(&ChangeIdle20);

    if (SmCurrent() == &ResetWait)
        StartTimer(PsmTimerResetCompletion);

    return SmCall(&ResetChangeIdle);
}

SM_RESULT
PortMachine::OnResetWait(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeResetDone:
            if (StopTimer())
                return ResetSucceeded();
            return SmTransition(&ResetFlush);

        case PortEvent::HubRemoved:
            return ReleaseAndStopTimer();

        case PortEvent::ChangeError:
        case PortEvent::ChangeDisabled:
        case PortEvent::ChangeResumed:
        case PortEvent::ChangeConnect:
            if (m_Usb3)
                return SmUnhandled();
            return TimedError(&ResetWait, TRUE);

        case PortEvent::TimerExpired:
            if (m_Usb3)
                return SmTransition(&ResetTimeoutStatus);
            TellDevice(PsmNoticeResetTimedOut);
            DropInterruptReference();
            return SmTransition(&AttachedDisabled);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnResetEnabledWait(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeResetDone:
            if (StopTimer())
                return ResetSucceeded();
            return SmTransition(&ResetFlush);

        case PortEvent::HubRemoved:
            return ReleaseAndStopTimer();

        case PortEvent::ChangeResumed:
        case PortEvent::ChangeError:
        case PortEvent::ChangeDisabled:
            return TimedError(&ResetEnabledWait, TRUE);

        case PortEvent::TimerExpired:
            return SmTransition(&DisablingOnResetTimeout);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnResetFlush(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeError:
        case PortEvent::ChangeDisabled:
        case PortEvent::ChangeResumed:
        case PortEvent::ChangeResetDone:
            if (m_Usb3)
                return SmUnhandled();
            return TimedError(&ResetFlush, FALSE);

        case PortEvent::TimerExpired:
            return ResetSucceeded();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::EnterResumeWait()
{
    return SmCall(m_Usb3 ? &ResumeChangeIdle : &ChangeIdle20);
}

SM_RESULT
PortMachine::OnResumeWait(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeResumed:
            if (!StopTimer())
                return SmTransition(&ResumeFlush);
            if (m_Usb3)
                return ResumeSucceeded();
            StartTimer(PsmTimerResumeRecovery);
            return SmTransition(&ResumeRecovery);

        case PortEvent::ChangeDisabled:
        case PortEvent::ChangeError:
        case PortEvent::ChangeResetDone:
            if (m_Usb3)
                return SmUnhandled();
            return TimedError(&ResumeWait, FALSE);

        case PortEvent::HubRemoved:
            return ReleaseAndStopTimer();

        case PortEvent::TimerExpired:
            if (m_Usb3)
                return SmTransition(&ReadingAfterResumeTimeout);
            TellDevice(PsmNoticeResumeTimedOut);
            DropInterruptReference();
            return SmTransition(&AttachedDisabled);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnResumeRecovery(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeResumed:
        case PortEvent::ChangeResetDone:
        case PortEvent::ChangeError:
        case PortEvent::ChangeDisabled:
            return TimedError(&ResumeRecovery, FALSE);

        case PortEvent::TimerExpired:
            return ResumeSucceeded();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnResumeFlush(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeDisabled:
        case PortEvent::ChangeError:
        case PortEvent::ChangeResetDone:
        case PortEvent::ChangeResumed:
            if (m_Usb3)
                return SmUnhandled();
            return TimedError(&ResumeFlush, FALSE);

        case PortEvent::TimerExpired:
            if (m_Usb3)
                return ResumeSucceeded();
            StartTimer(PsmTimerResumeRecovery);
            return SmTransition(&ResumeRecovery);

        default:
            return SmUnhandled();
    }
}

/* HUB RESET PENDING **********************************************************/

SM_RESULT
PortMachine::EnterHubResetIssued()
{
    RequestHubReset();
    return EnterDraining();
}

SM_RESULT
PortMachine::EnterDraining()
{
    m_DrainReturnsEmpty = FALSE;
    return SmCall(&DrainIdle);
}

SM_RESULT
PortMachine::EnterDrainingEmpty()
{
    m_DrainReturnsEmpty = TRUE;
    return SmCall(m_Usb3 ? &DrainEmptyIdleUsb3 : &DrainEmptyIdleUsb2);
}

SM_RESULT
PortMachine::OnHubResetIssued(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::HubStopping)
        return PowerDown(&OffAttached);

    if (Event == PortEvent::HubSuspending)
        return SmTransition(m_Usb3 ? &OffSuspendedUsb3 : &OffSuspendedUsb2);

    return SmUnhandled();
}

SM_RESULT
PortMachine::OnHubResetWait(
    _In_ PortEvent Event)
{
    if ((Event == PortEvent::HubStopping) || (Event == PortEvent::HubSuspending))
        return PowerDown(&OffAttached);

    return SmUnhandled();
}

SM_RESULT
PortMachine::OnHubResetTimerWait(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TimerExpired)
        return SmTransition(&HubResetWait);

    if ((Event == PortEvent::HubStopping) || (Event == PortEvent::HubSuspending))
        return SmTransition(&FlushBeforeOffAttached);

    return SmUnhandled();
}

SM_RESULT
PortMachine::OnEmptyHubResetWait(
    _In_ PortEvent Event)
{
    if ((Event == PortEvent::HubStopping) || (Event == PortEvent::HubSuspending))
        return PowerDown(&OffEmpty);

    return SmUnhandled();
}

SM_RESULT
PortMachine::OnEmptyHubResetTimerWait(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TimerExpired)
        return SmTransition(&EmptyHubResetWait);

    if ((Event == PortEvent::HubStopping) || (Event == PortEvent::HubSuspending))
        return SmTransition(&FlushBeforeOffEmpty);

    return SmUnhandled();
}

SM_RESULT
PortMachine::OnAttachFailedWait(
    _In_ PortEvent Event)
{
    if ((Event != PortEvent::HubStopping) && (Event != PortEvent::HubSuspending))
        return SmUnhandled();

    if (!m_Usb3)
        return PowerDown(&OffEmpty);

    if ((Event == PortEvent::HubSuspending) && !m_AttachForcedReset)
        return SmTransition(&QuiescingForHubSuspend);

    return PowerDown(&OffResetNeeded);
}

SM_RESULT
PortMachine::OnSuperSpeedOffWait(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TimerExpired)
        return SmTransition(&EnablingSuperSpeedOnTimer);

    if ((Event == PortEvent::HubStopping) || (Event == PortEvent::HubSuspending))
        return SmTransition(&EnablingSuperSpeedOnStop);

    return SmUnhandled();
}

SM_RESULT
PortMachine::OnSsLinkDisabled(
    _In_ PortEvent Event)
{
    if ((Event == PortEvent::HubStopping) || (Event == PortEvent::HubSuspending))
        return PowerDown(&OffSuperSpeedDisabled);

    return SmUnhandled();
}

/* PORT CONTROL TRANSFERS *****************************************************/

SM_RESULT
PortMachine::EnterPoweringPort()
{
    SendPortPower();
    return SmHandled();
}

SM_RESULT
PortMachine::EnterReadingStatus()
{
    SendGetStatus();
    return SmHandled();
}

SM_RESULT
PortMachine::EnterSendingReset()
{
    SendReset();
    return SmHandled();
}

SM_RESULT
PortMachine::EnterSendingWarmReset()
{
    SendWarmReset();
    return SmHandled();
}

SM_RESULT
PortMachine::EnterSendingResume()
{
    SendResume();
    return SmHandled();
}

SM_RESULT
PortMachine::EnterSendingSuspend()
{
    SendSuspend();
    return SmHandled();
}

SM_RESULT
PortMachine::EnterDisablingPort()
{
    SendDisable();
    return SmHandled();
}

SM_RESULT
PortMachine::EnterDisablingOnRequest()
{
    if (!m_Usb3)
    {
        SendDisable();
        return SmHandled();
    }

    /* A 3.0 port has no disabled state, the link is suspended instead */
    SendSuspend();
    ForceResetOnEnumeration();
    return SmHandled();
}

SM_RESULT
PortMachine::EnterQuiescingForHubSuspend()
{
    if (m_Usb3)
        SendSuspend();
    else
        SendDisable();

    return SmHandled();
}

SM_RESULT
PortMachine::EnterSettingLpmTimeout()
{
    if (m_LpmU2)
        SendU2Timeout();
    else
        SendU1Timeout();

    return SmHandled();
}

SM_RESULT
PortMachine::EnterDisablingSuperSpeed()
{
    SendLinkDisable();
    return SmHandled();
}

SM_RESULT
PortMachine::EnterDisablingSuperSpeedOnRequest()
{
    DisconnectDevice();
    SendLinkDisable();
    return SmHandled();
}

SM_RESULT
PortMachine::EnterEnablingSuperSpeed()
{
    SendLinkRxDetect();
    return SmHandled();
}

SM_RESULT
PortMachine::EnterResetTimeoutStatus()
{
    return CallStatusRead(ReadNoChange);
}

SM_RESULT
PortMachine::EnterArmingWake()
{
    return SmCall(&WakeArming);
}

SM_RESULT
PortMachine::OnPoweringPort(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferDone)
        return SmTransition(&QueryingOverCurrent);

    if (Event == PortEvent::TransferFailed)
        return ResetHubThen(&EmptyHubResetWait);

    return SmUnhandled();
}

SM_RESULT
PortMachine::OnQueryingOverCurrent(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferFailed)
        return ResetHubThen(&EmptyHubResetWait);

    if (Event != PortEvent::TransferDone)
        return SmUnhandled();

    if (OverCurrentActive())
        return StartOverCurrentWait();

    if (m_Usb3)
        return SmTransition(&Disconnected);

    return CheckConnection();
}

SM_RESULT
PortMachine::OnSendingReset(
    _In_ PortEvent Event)
{
    if ((Event != PortEvent::TransferDone) && (Event != PortEvent::TransferFailed))
        return SmUnhandled();

    if (!m_Usb3)
    {
        StartTimer(PsmTimerResetCompletion);
        return SmTransition(&ResetWait);
    }

    if (Event == PortEvent::TransferDone)
        return SmTransition(&ResetWait);

    return ResetTransferFailed();
}

SM_RESULT
PortMachine::OnSendingResume(
    _In_ PortEvent Event)
{
    if ((Event != PortEvent::TransferDone) && (Event != PortEvent::TransferFailed))
        return SmUnhandled();

    if (m_Usb3 && (Event == PortEvent::TransferFailed))
    {
        DropInterruptReference();
        return FailToHubReset();
    }

    StartTimer(PsmTimerResumeCompletion);
    return SmTransition(&ResumeWait);
}

SM_RESULT
PortMachine::OnSendingSuspend(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferFailed)
        return FailToHubReset();

    if (Event != PortEvent::TransferDone)
        return SmUnhandled();

    TellDevice(PsmNoticeSuspended);
    if (!m_Usb3)
        RecordPortSuspended();

    return SmTransition(&Suspended);
}

SM_RESULT
PortMachine::OnDisablingOnRequest(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferFailed)
    {
        TellDevice(PsmNoticeFailed);
        return SmTransition(&AttachedDisabled);
    }

    if (Event != PortEvent::TransferDone)
        return SmUnhandled();

    TellDevice(PsmNoticeDisabled);
    if (m_Usb3)
        return SmTransition(&DisabledInSuspend);

    RecordPortDisabled();
    return SmTransition(&AttachedDisabled);
}

SM_RESULT
PortMachine::OnDisablingForCycle(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferFailed)
        return ResetHubThen(&EmptyHubResetWait);

    if (Event == PortEvent::TransferDone)
        return SmTransition(&OldDeviceWait);

    return SmUnhandled();
}

SM_RESULT
PortMachine::OnDisablingBeforeConnect(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferFailed)
        return ResetHubThen(&EmptyHubResetWait);

    if (Event == PortEvent::TransferDone)
        return SmTransition(&DebouncingUsb2);

    return SmUnhandled();
}

SM_RESULT
PortMachine::OnQuiescingForHubSuspend(
    _In_ PortEvent Event)
{
    const State* target = m_Usb3 ? &OffResetNeeded : &OffEmpty;

    if (Event == PortEvent::TransferFailed)
    {
        RequestHubReset();
        return PowerDown(target);
    }

    if (Event == PortEvent::TransferDone)
        return PowerDown(target);

    return SmUnhandled();
}

SM_RESULT
PortMachine::OnDisablingForCycleTimed(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferFailed)
    {
        RequestHubReset();
        return SmTransition(StopTimer() ? &EmptyHubResetWait : &EmptyHubResetTimerWait);
    }

    if (Event == PortEvent::TransferDone)
        return StopTimerThen(FollowCycle);

    return SmUnhandled();
}

SM_RESULT
PortMachine::OnDisablingOnSuspendTimed(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferFailed)
        return ResetHubThen(&FlushBeforeOffEmpty);

    if (Event == PortEvent::TransferDone)
        return SmTransition(&FlushBeforeOffEmpty);

    return SmUnhandled();
}

SM_RESULT
PortMachine::OnDisablingOnResetTimeout(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferFailed)
        return ResetTransferFailed();

    if (Event != PortEvent::TransferDone)
        return SmUnhandled();

    TellDevice(PsmNoticeResetTimedOut);
    DropInterruptReference();
    return SmTransition(&AttachedDisabled);
}

SM_RESULT
PortMachine::OnReadingForResume(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferFailed)
    {
        DropInterruptReference();
        return FailToHubReset();
    }

    if (Event != PortEvent::TransferDone)
        return SmUnhandled();

    /* The link may already be back in U0, then no resume is sent */
    if (LinkInU0())
        return ResumeSucceeded();

    return SmTransition(&SendingResume);
}

SM_RESULT
PortMachine::OnReadingAfterResumeTimeout(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferFailed)
        return FailToHubReset();

    if (Event != PortEvent::TransferDone)
        return SmUnhandled();

    if (LinkInU0())
        return ResumeSucceeded();

    DropInterruptReference();
    TellDevice(PsmNoticeResumeTimedOut);
    return SmTransition(&AttachedDisabled);
}

SM_RESULT
PortMachine::OnSettingLpmTimeout(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferFailed)
    {
        TellDevice(PsmNoticeFailed);
        return SmTransition(&Enabled);
    }

    if (Event == PortEvent::TransferDone)
    {
        TellDevice(PsmNoticeTimeoutUpdated);
        return SmTransition(&Enabled);
    }

    return SmUnhandled();
}

SM_RESULT
PortMachine::OnDisablingSuperSpeed(
    _In_ PortEvent Event)
{
    if ((Event == PortEvent::TransferDone) || (Event == PortEvent::TransferFailed))
        return PowerUp(&SsLinkDisabled);

    return SmUnhandled();
}

SM_RESULT
PortMachine::OnDisablingSuperSpeedOnRequest(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferFailed)
        return ResetHubThen(&EmptyHubResetWait);

    if (Event == PortEvent::TransferDone)
    {
        StartTimer(PsmTimerSuperSpeedDisable);
        return SmTransition(&SuperSpeedOffWait);
    }

    return SmUnhandled();
}

SM_RESULT
PortMachine::OnEnablingSuperSpeedOnStop(
    _In_ PortEvent Event)
{
    if ((Event != PortEvent::TransferDone) && (Event != PortEvent::TransferFailed))
        return SmUnhandled();

    if (StopTimer())
        return PowerDown(&OffEmpty);

    return SmTransition(&FlushBeforeSuperSpeedOff);
}

SM_RESULT
PortMachine::OnEnablingSuperSpeedOnTimer(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferFailed)
        return ResetHubThen(&EmptyHubResetWait);

    if (Event == PortEvent::TransferDone)
        return SmTransition(&Disconnected);

    return SmUnhandled();
}

/* Status read after the reset timer fired on a 3.0 port */
SM_RESULT
PortMachine::OnResetTimeoutStatus(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeNone:
            TellDevice(PsmNoticeResetTimedOut);
            DropInterruptReference();
            return SmTransition(&AttachedDisabled);

        case PortEvent::ChangeResetDone:
            return ResetSucceeded();

        case PortEvent::ChangeResetBusy:
        case PortEvent::ChangeNeedsHubReset:
        case PortEvent::ChangeResumed:
        case PortEvent::ChangeError:
            TellDevice(PsmNoticeFailed);
            RequestHubReset();
            DropInterruptReference();
            return SmTransition(&HubResetWait);

        case PortEvent::ChangeOverCurrent:
            DisconnectDevice();
            DropInterruptReference();
            return StartOverCurrentWait();

        case PortEvent::ChangeOverCurrentCleared:
            DisconnectDevice();
            DropInterruptReference();
            return SmTransition(&PoweringPort);

        case PortEvent::ChangeConnect:
            DisconnectDevice();
            DropInterruptReference();
            return CheckConnection();

        case PortEvent::ChangeLinkError:
            TellDevice(PsmNoticeFailed);
            LogLinkStateError();
            DropInterruptReference();
            return SmTransition(&AttachedDisabled);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnArmingWake(
    _In_ PortEvent Event)
{
    if (Event != PortEvent::SubDone)
        return SmUnhandled();

    if (m_ArmForEmpty)
        return PowerDown(&OffEmpty);

    return SmTransition(&OffSuspendedUsb3);
}
