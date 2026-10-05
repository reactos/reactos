/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Port state machine: called frames for port status reading,
 *              hub power up, resume, suspended ports and D3cold
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include "usbhub3.h"

/* GLOBALS ********************************************************************/

#define PSM_YIELDS  SM_STATE_YIELDS_TO_CALLER
#define PSM_CRIT    SM_STATE_CRITICAL_ONLY
#define PSM_PASSIVE SM_STATE_NEEDS_PASSIVE

static const PortEvent FrameGoneOnly[] =
{
    PortEvent::DeviceGone, PortEvent::Count
};

static const PortEvent FrameRemovedOnly[] =
{
    PortEvent::HubRemoved, PortEvent::Count
};

static const PortEvent DrainDrops[] =
{
    PortEvent::CycleRequest, PortEvent::SetU1Timeout, PortEvent::SetU2Timeout, PortEvent::Count
};

static const PortEvent DrainEmptyDrops20[] =
{
    PortEvent::SuspendRequest, PortEvent::ResetRequest, PortEvent::ResumeRequest,
    PortEvent::DisableRequest, PortEvent::DeviceGone, PortEvent::CycleRequest,
    PortEvent::HibernateRequest, PortEvent::Count
};

static const PortEvent DrainEmptyDrops30[] =
{
    PortEvent::ResumeRequest, PortEvent::CycleRequest, PortEvent::SetU2Timeout,
    PortEvent::ResetRequest, PortEvent::DisableRequest, PortEvent::SuspendRequest,
    PortEvent::SetU1Timeout, PortEvent::DeviceGone, PortEvent::WarmResetRequest,
    PortEvent::DisableSuperSpeedRequest, PortEvent::Count
};

/* 2.0 port change: read status, ack every change bit, report the change */
const PortMachine::State PortMachine::ChangeIdle20 =
    { NULL, "ChangeIdle20", PSM_YIELDS, &PortMachine::OnChangeIdle20, NULL, NULL };

const PortMachine::State PortMachine::ChangeReading20 =
    { NULL, "ChangeReading20", PSM_CRIT,
      &PortMachine::OnChangeReading20, &PortMachine::EnterReadingStatus, NULL };

const PortMachine::State PortMachine::ChangeAcking20 =
    { NULL, "ChangeAcking20", PSM_CRIT,
      &PortMachine::OnChangeAcking20, &PortMachine::EnterAckingChange, NULL };

const PortMachine::State PortMachine::ChangeReporting20 =
    { NULL, "ChangeReporting20", PSM_PASSIVE, NULL, &PortMachine::EnterChangeReporting20, NULL };

/* Interrupt transfer given back, status changes are acked until the hub resets */
const PortMachine::State PortMachine::ChangeParked20 =
    { NULL, "ChangeParked20", PSM_YIELDS | PSM_PASSIVE,
      &PortMachine::OnChangeParked20, &PortMachine::EnterChangeParked20, NULL };

/* 3.0 port change and its reset and resume flavors */
const PortMachine::State PortMachine::ChangeIdle30 =
    { NULL, "ChangeIdle30", PSM_YIELDS, &PortMachine::OnChangeIdle30, NULL, NULL };

const PortMachine::State PortMachine::ChangeChecking30 =
    { NULL, "ChangeChecking30", PSM_YIELDS,
      &PortMachine::OnChangeChecking30, &PortMachine::EnterPlainStatusRead, NULL };

const PortMachine::State PortMachine::ResetChangeIdle =
    { NULL, "ResetChangeIdle", PSM_YIELDS, &PortMachine::OnResetChangeIdle, NULL, NULL };

const PortMachine::State PortMachine::ResetChangeChecking =
    { NULL, "ResetChangeChecking", PSM_YIELDS,
      &PortMachine::OnResetChangeChecking, &PortMachine::EnterPlainStatusRead, NULL };

const PortMachine::State PortMachine::ResetPollWait =
    { NULL, "ResetPollWait", PSM_CRIT,
      &PortMachine::OnResetPollWait, &PortMachine::EnterResetPollWait, NULL };

const PortMachine::State PortMachine::ResetPollChecking =
    { NULL, "ResetPollChecking", PSM_YIELDS,
      &PortMachine::OnResetPollChecking, &PortMachine::EnterResetPollChecking, NULL };

const PortMachine::State PortMachine::ResumeChangeIdle =
    { NULL, "ResumeChangeIdle", PSM_YIELDS, &PortMachine::OnResumeChangeIdle, NULL, NULL };

const PortMachine::State PortMachine::ResumeChangeChecking =
    { NULL, "ResumeChangeChecking", PSM_YIELDS,
      &PortMachine::OnResumeChangeChecking, &PortMachine::EnterPlainStatusRead, NULL };

/* 3.0 status reading, ends by returning the change, m_ReadKind picks the ending */
const PortMachine::State PortMachine::StatusReading =
    { NULL, "StatusReading", PSM_CRIT,
      &PortMachine::OnStatusReading, &PortMachine::EnterStatusReading, NULL };

const PortMachine::State PortMachine::StatusAcking =
    { NULL, "StatusAcking", PSM_CRIT,
      &PortMachine::OnStatusAcking, &PortMachine::EnterAckingChange, NULL };

const PortMachine::State PortMachine::StatusReporting =
    { NULL, "StatusReporting", PSM_PASSIVE, NULL, &PortMachine::EnterStatusReporting, NULL };

const PortMachine::State PortMachine::StatusFailing =
    { NULL, "StatusFailing", PSM_PASSIVE, NULL, &PortMachine::EnterStatusFailing, NULL };

/* Waiting for hub stop or suspend: fail device requests, drain status changes */
const PortMachine::State PortMachine::DrainIdle =
    { NULL, "DrainIdle", PSM_YIELDS, &PortMachine::OnDrainIdle, NULL, DrainDrops };

const PortMachine::State PortMachine::DrainEmptyIdleUsb2 =
    { NULL, "DrainEmptyIdleUsb2", PSM_YIELDS, &PortMachine::OnDrainEmptyIdle, NULL, DrainEmptyDrops20 };

const PortMachine::State PortMachine::DrainEmptyIdleUsb3 =
    { NULL, "DrainEmptyIdleUsb3", PSM_YIELDS, &PortMachine::OnDrainEmptyIdle, NULL, DrainEmptyDrops30 };

const PortMachine::State PortMachine::DrainReading =
    { NULL, "DrainReading", PSM_CRIT,
      &PortMachine::OnDrainReading, &PortMachine::EnterReadingStatus, NULL };

const PortMachine::State PortMachine::DrainAcking =
    { NULL, "DrainAcking", PSM_CRIT,
      &PortMachine::OnDrainAcking, &PortMachine::EnterAckingChange, NULL };

const PortMachine::State PortMachine::DrainRearming =
    { NULL, "DrainRearming", PSM_PASSIVE, NULL, &PortMachine::EnterDrainRearming, NULL };

const PortMachine::State PortMachine::DrainChecking =
    { NULL, "DrainChecking", PSM_YIELDS,
      &PortMachine::OnDrainChecking, &PortMachine::EnterPlainStatusRead, NULL };

/* Hub start: power the port, wait the power on time, read lost changes */
const PortMachine::State PortMachine::PowerOnSending =
    { NULL, "PowerOnSending", PSM_CRIT,
      &PortMachine::OnPowerOnSending, &PortMachine::EnterPoweringPort, NULL };

const PortMachine::State PortMachine::PowerOnSettling =
    { NULL, "PowerOnSettling", 0,
      &PortMachine::OnPowerOnSettling, &PortMachine::EnterPowerOnSettling, FrameGoneOnly };

const PortMachine::State PortMachine::StartReading20 =
    { NULL, "StartReading20", PSM_CRIT,
      &PortMachine::OnStartReading20, &PortMachine::EnterReadingStatus, NULL };

const PortMachine::State PortMachine::StartReadingFirst =
    { NULL, "StartReadingFirst", PSM_CRIT,
      &PortMachine::OnStartReadingFirst, &PortMachine::EnterReadingStatus, NULL };

const PortMachine::State PortMachine::StartReading30 =
    { NULL, "StartReading30", PSM_YIELDS,
      &PortMachine::OnStartReading30, &PortMachine::EnterStartResumeStatusRead, NULL };

const PortMachine::State PortMachine::PowerUpResetHub =
    { NULL, "PowerUpResetHub", PSM_YIELDS,
      &PortMachine::OnPowerUpParked, &PortMachine::EnterPowerUpResetHub, FrameGoneOnly };

const PortMachine::State PortMachine::PowerUpRearming =
    { NULL, "PowerUpRearming", PSM_YIELDS | PSM_PASSIVE,
      &PortMachine::OnPowerUpParked, &PortMachine::EnterPowerUpRearming, FrameGoneOnly };

/* 2.0 hub resume */
const PortMachine::State PortMachine::ResumePowering =
    { NULL, "ResumePowering", PSM_CRIT,
      &PortMachine::OnResumePowering, &PortMachine::EnterPoweringPort, NULL };

const PortMachine::State PortMachine::ResumeSettling =
    { NULL, "ResumeSettling", 0,
      &PortMachine::OnResumeSettling, &PortMachine::EnterPowerOnSettling, NULL };

const PortMachine::State PortMachine::ResumeReading =
    { NULL, "ResumeReading", PSM_CRIT,
      &PortMachine::OnResumeReading, &PortMachine::EnterReadingStatus, NULL };

const PortMachine::State PortMachine::ResumeAckingConnect =
    { NULL, "ResumeAckingConnect", PSM_CRIT,
      &PortMachine::OnResumeAckingConnect, &PortMachine::EnterAckingChange, NULL };

const PortMachine::State PortMachine::ReconnectWait =
    { NULL, "ReconnectWait", 0, &PortMachine::OnReconnectWait, NULL, NULL };

const PortMachine::State PortMachine::ReconnectReading =
    { NULL, "ReconnectReading", PSM_CRIT,
      &PortMachine::OnReconnectReading, &PortMachine::EnterReadingStatus, NULL };

const PortMachine::State PortMachine::ReconnectAcking =
    { NULL, "ReconnectAcking", PSM_CRIT,
      &PortMachine::OnReconnectAcking, &PortMachine::EnterAckingChange, NULL };

const PortMachine::State PortMachine::ReconnectRearming =
    { NULL, "ReconnectRearming", PSM_PASSIVE, NULL, &PortMachine::EnterReconnectRearming, NULL };

const PortMachine::State PortMachine::ReconnectFlush =
    { NULL, "ReconnectFlush", 0, &PortMachine::OnReconnectFlush, NULL, NULL };

const PortMachine::State PortMachine::ResumeRereading =
    { NULL, "ResumeRereading", PSM_CRIT,
      &PortMachine::OnResumeRereading, &PortMachine::EnterReadingStatus, NULL };

/* 3.0 system resume */
const PortMachine::State PortMachine::SystemResumeReading =
    { NULL, "SystemResumeReading", PSM_YIELDS,
      &PortMachine::OnSystemResumeReading, &PortMachine::EnterStartResumeStatusRead, NULL };

const PortMachine::State PortMachine::SystemReconnectWait =
    { NULL, "SystemReconnectWait", 0, &PortMachine::OnSystemReconnectWait, NULL, NULL };

const PortMachine::State PortMachine::SystemResumeRereading =
    { NULL, "SystemResumeRereading", PSM_YIELDS,
      &PortMachine::OnSystemResumeRereading, &PortMachine::EnterPlainStatusRead, NULL };

/* 2.0 suspended port change */
const PortMachine::State PortMachine::SuspendIdle =
    { NULL, "SuspendIdle", PSM_YIELDS, &PortMachine::OnSuspendIdle, NULL, NULL };

const PortMachine::State PortMachine::SuspendReading =
    { NULL, "SuspendReading", PSM_CRIT,
      &PortMachine::OnSuspendReading, &PortMachine::EnterReadingStatus, NULL };

const PortMachine::State PortMachine::SuspendAcking =
    { NULL, "SuspendAcking", PSM_CRIT,
      &PortMachine::OnSuspendAcking, &PortMachine::EnterAckingChange, NULL };

const PortMachine::State PortMachine::SuspendRecovery =
    { NULL, "SuspendRecovery", 0,
      &PortMachine::OnSuspendRecovery, &PortMachine::EnterSuspendRecovery, NULL };

const PortMachine::State PortMachine::SuspendReporting =
    { NULL, "SuspendReporting", PSM_PASSIVE, NULL, &PortMachine::EnterSuspendReporting, NULL };

/* D3cold: the suspended device lost power, wait for it to come back */
const PortMachine::State PortMachine::D3Idle =
    { NULL, "D3Idle", PSM_YIELDS, &PortMachine::OnD3Idle, &PortMachine::EnterWithPortChange, NULL };

const PortMachine::State PortMachine::D3PoweredOff =
    { NULL, "D3PoweredOff", PSM_YIELDS, &PortMachine::OnD3PoweredOff, NULL, NULL };

const PortMachine::State PortMachine::D3PoweringUp =
    { NULL, "D3PoweringUp", 0,
      &PortMachine::OnD3PoweringUp, &PortMachine::EnterD3PoweringUp, NULL };

const PortMachine::State PortMachine::D3Debouncing =
    { NULL, "D3Debouncing", 0,
      &PortMachine::OnD3Debouncing, &PortMachine::EnterWithPortChange, FrameRemovedOnly };

const PortMachine::State PortMachine::D3DebounceFlush =
    { NULL, "D3DebounceFlush", 0, &PortMachine::OnD3DebounceFlush, NULL, FrameRemovedOnly };

const PortMachine::State PortMachine::D3FlushThenReturn =
    { NULL, "D3FlushThenReturn", 0, &PortMachine::OnD3FlushThenReturn, NULL, FrameRemovedOnly };

const PortMachine::State PortMachine::D3ResumeAckWait =
    { NULL, "D3ResumeAckWait", PSM_YIELDS,
      &PortMachine::OnD3ResumeAckWait, &PortMachine::EnterWithPortChange, NULL };

const PortMachine::State PortMachine::D3ReconnectWait =
    { NULL, "D3ReconnectWait", 0,
      &PortMachine::OnD3ReconnectWait, &PortMachine::EnterWithPortChange, NULL };

const PortMachine::State PortMachine::D3ReconnectFlush =
    { NULL, "D3ReconnectFlush", 0, &PortMachine::OnD3ReconnectFlush, NULL, NULL };

/* 3.0 remote wake programming before the hub suspends */
const PortMachine::State PortMachine::WakeArming =
    { NULL, "WakeArming", PSM_CRIT, &PortMachine::OnWakeArming, &PortMachine::EnterWakeArming, NULL };

/* FUNCTIONS ******************************************************************/

SM_RESULT
PortMachine::EnterAckingChange()
{
    SendAckChange();
    return SmHandled();
}

/* 2.0 PORT CHANGE ************************************************************/

SM_RESULT
PortMachine::OnChangeIdle20(
    _In_ PortEvent Event)
{
    if (Event != PortEvent::StatusChanged)
        return SmUnhandled();

    ResetChangeAccumulator();
    return SmTransition(&ChangeReading20);
}

SM_RESULT
PortMachine::CheckPending20(
    _In_ const State* Acking,
    _In_ const State* Quiet)
{
    switch (PendingChange())
    {
        case PsmPendingChange:
            return SmTransition(Acking);

        case PsmPendingError:
            return ParkChange20(TRUE);

        default:
            return SmTransition(Quiet);
    }
}

SM_RESULT
PortMachine::ParkChange20(
    _In_ BOOLEAN ResetHub)
{
    m_ParkResetsHub = ResetHub;
    return SmTransition(&ChangeParked20);
}

SM_RESULT
PortMachine::OnChangeReading20(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferFailed)
        return ParkChange20(TRUE);

    if (Event == PortEvent::TransferDone)
        return CheckPending20(&ChangeAcking20, &ChangeReporting20);

    return SmUnhandled();
}

SM_RESULT
PortMachine::OnChangeAcking20(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferFailed)
        return ParkChange20(TRUE);

    if (Event == PortEvent::TransferDone)
        return CheckPending20(&ChangeAcking20, &ChangeReading20);

    return SmUnhandled();
}

SM_RESULT
PortMachine::EnterChangeReporting20()
{
    PortEvent change = NextChange();

    ResumeInterruptTransfer();
    return SmReturn(change);
}

SM_RESULT
PortMachine::EnterChangeParked20()
{
    ResumeInterruptTransfer();

    if (m_ParkResetsHub)
    {
        m_ParkResetsHub = FALSE;
        RequestHubReset();
    }

    return SmHandled();
}

SM_RESULT
PortMachine::OnChangeParked20(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::StatusChanged)
        return ParkChange20(FALSE);

    return SmUnhandled();
}

/* 3.0 PORT CHANGE ************************************************************/

SM_RESULT
PortMachine::CallStatusRead(
    _In_ READ_KIND Kind)
{
    m_ReadKind = Kind;
    m_ReadFresh = TRUE;
    return SmCall(&StatusReading);
}

SM_RESULT
PortMachine::EnterPlainStatusRead()
{
    return CallStatusRead(ReadPlain);
}

SM_RESULT
PortMachine::EnterStartResumeStatusRead()
{
    return CallStatusRead(ReadStartResume);
}

SM_RESULT
PortMachine::OnChangeIdle30(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::StatusChanged)
        return SmTransition(&ChangeChecking30);

    return SmUnhandled();
}

SM_RESULT
PortMachine::OnChangeChecking30(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeError:
        case PortEvent::ChangeResetBusy:
        case PortEvent::ChangeResetDone:
            return ReturnFromFrame(PortEvent::ChangeNeedsHubReset);

        case PortEvent::ChangeLinkError:
            return ReturnFromFrame(PortEvent::ChangeConnect);

        case PortEvent::ChangeNone:
        case PortEvent::ChangeResumed:
            return SmTransition(&ChangeIdle30);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnResetChangeIdle(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::StatusChanged)
        return SmTransition(&ResetChangeChecking);

    return SmUnhandled();
}

SM_RESULT
PortMachine::OnResetChangeChecking(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeNone:
        case PortEvent::ChangeLinkError:
            return SmTransition(&ResetChangeIdle);

        case PortEvent::ChangeResumed:
        case PortEvent::ChangeError:
            return ReturnFromFrame(PortEvent::ChangeNeedsHubReset);

        case PortEvent::ChangeResetBusy:
            if (PollResetCompletion())
                return SmTransition(&ResetPollWait);
            return SmTransition(&ResetChangeIdle);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::EnterResetPollWait()
{
    StartTimer(PsmTimerResetPoll);
    return SmHandled();
}

SM_RESULT
PortMachine::OnResetPollWait(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::ResetPollExpired)
        return SmTransition(&ResetPollChecking);

    return SmUnhandled();
}

SM_RESULT
PortMachine::EnterResetPollChecking()
{
    return CallStatusRead(ReadNoChange);
}

SM_RESULT
PortMachine::OnResetPollChecking(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeLinkError:
        case PortEvent::ChangeResumed:
            return ReturnFromFrame(PortEvent::ChangeNeedsHubReset);

        case PortEvent::ChangeNone:
            if (LinkInU0())
                return ReturnFromFrame(PortEvent::ChangeResetDone);
            return ReturnFromFrame(PortEvent::ChangeNeedsHubReset);

        case PortEvent::ChangeResetBusy:
            return SmTransition(&ResetChangeChecking);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnResumeChangeIdle(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::StatusChanged)
        return SmTransition(&ResumeChangeChecking);

    return SmUnhandled();
}

SM_RESULT
PortMachine::OnResumeChangeChecking(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeNone:
            return SmTransition(&ResumeChangeIdle);

        case PortEvent::ChangeLinkError:
            return ReturnFromFrame(PortEvent::ChangeConnect);

        case PortEvent::ChangeError:
        case PortEvent::ChangeResetBusy:
        case PortEvent::ChangeResetDone:
            return ReturnFromFrame(PortEvent::ChangeNeedsHubReset);

        default:
            return SmUnhandled();
    }
}

/* 3.0 STATUS READING *********************************************************/

SM_RESULT
PortMachine::EnterStatusReading()
{
    if (m_ReadFresh)
    {
        m_ReadFresh = FALSE;
        ResetChangeAccumulator();
        KeepHubAwake();
    }

    SendGetStatus();
    return SmHandled();
}

SM_RESULT
PortMachine::StatusFailed()
{
    if (m_ReadKind == ReadPlain)
        return SmTransition(&StatusFailing);

    if (m_ReadKind == ReadStartResume)
        TakePortPowerReference();

    return ReturnFromFrame(PortEvent::ChangeNeedsHubReset);
}

SM_RESULT
PortMachine::StatusQuiet()
{
    if (m_ReadKind == ReadPlain)
        return SmTransition(&StatusReporting);

    if (m_ReadKind == ReadStartResume)
    {
        TakePortPowerReference();
        FixPortStateAfterPowerUp();
    }

    return ReturnFromFrame(NextChange());
}

SM_RESULT
PortMachine::OnStatusReading(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferFailed)
        return StatusFailed();

    if (Event != PortEvent::TransferDone)
        return SmUnhandled();

    switch (PendingChange())
    {
        case PsmPendingChange:
            return SmTransition(&StatusAcking);

        case PsmPendingError:
            return StatusFailed();

        default:
            return StatusQuiet();
    }
}

SM_RESULT
PortMachine::OnStatusAcking(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferFailed)
        return StatusFailed();

    if (Event != PortEvent::TransferDone)
        return SmUnhandled();

    switch (PendingChange())
    {
        case PsmPendingChange:
            return SmTransition(&StatusAcking);

        case PsmPendingError:
            return StatusFailed();

        default:
            return SmTransition(&StatusReading);
    }
}

SM_RESULT
PortMachine::EnterStatusReporting()
{
    ResumeInterruptTransfer();
    return SmReturn(NextChange());
}

SM_RESULT
PortMachine::EnterStatusFailing()
{
    ResumeInterruptTransfer();
    return SmReturn(PortEvent::ChangeNeedsHubReset);
}

/* DRAINING *******************************************************************/

SM_RESULT
PortMachine::FailDeviceRequest()
{
    TellDevice(PsmNoticeFailed);
    return SmTransition(&DrainIdle);
}

SM_RESULT
PortMachine::DrainDone()
{
    if (!m_DrainReturnsEmpty)
        return SmTransition(&DrainIdle);

    return SmTransition(m_Usb3 ? &DrainEmptyIdleUsb3 : &DrainEmptyIdleUsb2);
}

SM_RESULT
PortMachine::OnDrainIdle(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::DisableRequest:
        case PortEvent::SuspendRequest:
        case PortEvent::ResetRequest:
        case PortEvent::ResumeRequest:
            return FailDeviceRequest();

        case PortEvent::HibernateRequest:
            if (m_Usb3)
                return SmUnhandled();
            return FailDeviceRequest();

        case PortEvent::WarmResetRequest:
            if (!m_Usb3)
                return SmUnhandled();
            return FailDeviceRequest();

        case PortEvent::StatusChanged:
            return SmTransition(m_Usb3 ? &DrainChecking : &DrainReading);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnDrainEmptyIdle(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::StatusChanged)
        return SmTransition(m_Usb3 ? &DrainChecking : &DrainReading);

    return SmUnhandled();
}

SM_RESULT
PortMachine::OnDrainReading(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferFailed)
        return SmTransition(&DrainRearming);

    if (Event != PortEvent::TransferDone)
        return SmUnhandled();

    if (PendingChange() == PsmPendingChange)
        return SmTransition(&DrainAcking);

    return SmTransition(&DrainRearming);
}

SM_RESULT
PortMachine::OnDrainAcking(
    _In_ PortEvent Event)
{
    if ((Event == PortEvent::TransferDone) || (Event == PortEvent::TransferFailed))
        return SmTransition(&DrainRearming);

    return SmUnhandled();
}

SM_RESULT
PortMachine::EnterDrainRearming()
{
    ResumeInterruptTransfer();
    return DrainDone();
}

SM_RESULT
PortMachine::OnDrainChecking(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeNone:
        case PortEvent::ChangeResetDone:
        case PortEvent::ChangeNeedsHubReset:
        case PortEvent::ChangeLinkError:
        case PortEvent::ChangeOverCurrentCleared:
        case PortEvent::ChangeResetBusy:
        case PortEvent::ChangeError:
        case PortEvent::ChangeOverCurrent:
        case PortEvent::ChangeResumed:
        case PortEvent::ChangeConnect:
            return DrainDone();

        default:
            return SmUnhandled();
    }
}

/* HUB START ******************************************************************/

SM_RESULT
PortMachine::OnPowerOnSending(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferDone)
        return SmTransition(&PowerOnSettling);

    if (Event != PortEvent::TransferFailed)
        return SmUnhandled();

    TakePortPowerReference();
    if (m_Usb3)
        return ReturnFromFrame(PortEvent::ChangeNeedsHubReset);

    return SmTransition(&PowerUpResetHub);
}

SM_RESULT
PortMachine::EnterPowerOnSettling()
{
    StartTimer(PsmTimerPowerOn);
    return SmHandled();
}

SM_RESULT
PortMachine::OnPowerOnSettling(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TimerExpired)
        return SmTransition(m_Usb3 ? &StartReading30 : &StartReading20);

    return SmUnhandled();
}

SM_RESULT
PortMachine::OnStartReading20(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferFailed)
    {
        TakePortPowerReference();
        return SmTransition(&PowerUpResetHub);
    }

    if (Event != PortEvent::TransferDone)
        return SmUnhandled();

    TakePortPowerReference();
    return ReturnFromFrame(LostChange());
}

SM_RESULT
PortMachine::OnStartReadingFirst(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferFailed)
    {
        TakePortPowerReference();
        return ReturnFromFrame(PortEvent::ChangeNeedsHubReset);
    }

    if (Event != PortEvent::TransferDone)
        return SmUnhandled();

    return SmTransition(PortPowered() ? &StartReading30 : &PowerOnSending);
}

SM_RESULT
PortMachine::OnStartReading30(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeError:
            return ReturnFromFrame(PortEvent::ChangeNeedsHubReset);

        case PortEvent::ChangeResumed:
        case PortEvent::ChangeResetDone:
        case PortEvent::ChangeLinkError:
            return ReturnFromFrame(PortEvent::ChangeConnect);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::EnterPowerUpResetHub()
{
    RequestHubReset();
    return SmHandled();
}

SM_RESULT
PortMachine::EnterPowerUpRearming()
{
    ResumeInterruptTransfer();
    return SmHandled();
}

SM_RESULT
PortMachine::OnPowerUpParked(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::StatusChanged)
        return SmTransition(&PowerUpRearming);

    return SmUnhandled();
}

/* 2.0 HUB RESUME *************************************************************/

SM_RESULT
PortMachine::ResumeStateDisabled()
{
    TellDevice(PsmNoticeStateDisabled);
    return SmTransition(&PowerUpResetHub);
}

SM_RESULT
PortMachine::ResumeReadDone()
{
    return ReturnFromFrame(LostChange());
}

SM_RESULT
PortMachine::ResumeConnectCheck()
{
    if (ConnectChangedOnResume())
        return SmTransition(&ResumeAckingConnect);

    return ResumeReadDone();
}

SM_RESULT
PortMachine::OnResumePowering(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferDone)
        return SmTransition(&ResumeSettling);

    if (Event != PortEvent::TransferFailed)
        return SmUnhandled();

    TakePortPowerReference();
    return ResumeStateDisabled();
}

SM_RESULT
PortMachine::OnResumeSettling(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TimerExpired)
        return SmTransition(&ResumeReading);

    return SmUnhandled();
}

/**
 * @brief
 * An over current seen now, or one that came and went while suspended, is reported.
 */
SM_RESULT
PortMachine::OnResumeReading(
    _In_ PortEvent Event)
{
    PSM_PORT_BITS bits;

    if (Event == PortEvent::TransferFailed)
    {
        TakePortPowerReference();
        return ResumeStateDisabled();
    }

    if (Event != PortEvent::TransferDone)
        return SmUnhandled();

    TakePortPowerReference();

    ReadPortBits(&bits);
    if (bits.OverCurrent || bits.OverCurrentChanged)
    {
        MarkOverCurrentCause();
        return ReturnFromFrame(bits.OverCurrent ? PortEvent::ChangeOverCurrent :
                                                  PortEvent::ChangeOverCurrentCleared);
    }

    if (DeviceConnected())
        return ResumeConnectCheck();

    StartTimer(PsmTimerReconnect);
    return SmTransition(&ReconnectWait);
}

SM_RESULT
PortMachine::OnResumeAckingConnect(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferFailed)
        return ResumeStateDisabled();

    if (Event == PortEvent::TransferDone)
        return ResumeReadDone();

    return SmUnhandled();
}

SM_RESULT
PortMachine::OnReconnectWait(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::StatusChanged:
            ResetChangeAccumulator();
            return SmTransition(&ReconnectReading);

        case PortEvent::HubStopping:
            if (StopTimer())
                return ReturnFromFrame(PortEvent::HubStopping);
            m_ReconnectFlushOnStop = TRUE;
            return SmTransition(&ReconnectFlush);

        case PortEvent::TimerExpired:
            return SmTransition(&ResumeRereading);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::ReconnectStatusFailed()
{
    TellDevice(PsmNoticeStateDisabled);
    RequestHubReset();
    return SmTransition(&PowerUpRearming);
}

SM_RESULT
PortMachine::ReconnectCheck()
{
    switch (PendingChange())
    {
        case PsmPendingChange:
            return SmTransition(&ReconnectAcking);

        case PsmPendingError:
            return ReconnectStatusFailed();

        default:
            return SmTransition(&ReconnectRearming);
    }
}

SM_RESULT
PortMachine::OnReconnectReading(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferDone)
        return ReconnectCheck();

    if (Event == PortEvent::TransferFailed)
        return ReconnectStatusFailed();

    return SmUnhandled();
}

SM_RESULT
PortMachine::OnReconnectAcking(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferDone)
        return ReconnectCheck();

    if (Event == PortEvent::TransferFailed)
        return ReconnectStatusFailed();

    return SmUnhandled();
}

SM_RESULT
PortMachine::EnterReconnectRearming()
{
    ResumeInterruptTransfer();

    if (DeviceConnected() && StopTimer())
        return ResumeReadDone();

    return SmTransition(&ReconnectWait);
}

SM_RESULT
PortMachine::OnReconnectFlush(
    _In_ PortEvent Event)
{
    if (Event != PortEvent::TimerExpired)
        return SmUnhandled();

    if (m_ReconnectFlushOnStop)
        return ReturnFromFrame(PortEvent::HubStopping);

    return SmTransition(&SystemResumeRereading);
}

SM_RESULT
PortMachine::OnResumeRereading(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferFailed)
        return ResumeStateDisabled();

    if (Event != PortEvent::TransferDone)
        return SmUnhandled();

    if (DeviceConnected())
        return ResumeConnectCheck();

    return ResumeReadDone();
}

/* 3.0 SYSTEM RESUME **********************************************************/

SM_RESULT
PortMachine::SystemResumeFailed()
{
    ForceResetOnEnumeration();
    TellDevice(PsmNoticeStateDisabled);
    return ReturnFromFrame(PortEvent::ChangeNeedsReset);
}

SM_RESULT
PortMachine::OnSystemResumeReading(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeNone:
            TellDevice(PsmNoticeStateSuspended);
            return ReturnFromFrame(PortEvent::SubDone);

        case PortEvent::ChangeResumed:
            TellDevice(PsmNoticeStateEnabled);
            return ReturnFromFrame(PortEvent::ChangeResumed);

        case PortEvent::ChangeConnect:
            if (DeviceConnected())
            {
                TellDevice(PsmNoticeEnabledOnReconnect);
                return ReturnFromFrame(PortEvent::ChangeResumed);
            }
            StartTimer(PsmTimerReconnect);
            return SmTransition(&SystemReconnectWait);

        case PortEvent::ChangeResetBusy:
            TellDevice(PsmNoticeStateDisabled);
            ForceResetOnEnumeration();
            return ReturnFromFrame(PortEvent::ChangeResetBusy);

        case PortEvent::ChangeLinkError:
        case PortEvent::ChangeResetDone:
            return SystemResumeFailed();

        case PortEvent::ChangeError:
            return ReturnFromFrame(PortEvent::ChangeNeedsHubReset);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnSystemReconnectWait(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::HubStopping:
            if (StopTimer())
                return ReturnFromFrame(PortEvent::HubStopping);
            m_ReconnectFlushOnStop = TRUE;
            return SmTransition(&ReconnectFlush);

        case PortEvent::StatusChanged:
            if (StopTimer())
                return SmTransition(&SystemResumeRereading);
            m_ReconnectFlushOnStop = FALSE;
            return SmTransition(&ReconnectFlush);

        case PortEvent::TimerExpired:
            return ReturnFromFrame(PortEvent::ChangeConnect);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnSystemResumeRereading(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeLinkError:
            return SystemResumeFailed();

        case PortEvent::ChangeResumed:
        case PortEvent::ChangeError:
        case PortEvent::ChangeResetDone:
        case PortEvent::ChangeResetBusy:
            return ReturnFromFrame(PortEvent::ChangeNeedsHubReset);

        case PortEvent::ChangeNone:
            return ReturnFromFrame(PortEvent::ChangeConnect);

        case PortEvent::ChangeConnect:
            TellDevice(PsmNoticeEnabledOnReconnect);
            return ReturnFromFrame(PortEvent::ChangeResumed);

        default:
            return SmUnhandled();
    }
}

/* 2.0 SUSPENDED PORT CHANGE **************************************************/

SM_RESULT
PortMachine::OnSuspendIdle(
    _In_ PortEvent Event)
{
    if (Event != PortEvent::StatusChanged)
        return SmUnhandled();

    ResetChangeAccumulator();
    return SmTransition(&SuspendReading);
}

SM_RESULT
PortMachine::ReportSuspendChange(
    _In_ PortEvent Report)
{
    m_SuspendReport = Report;
    return SmTransition(&SuspendReporting);
}

/**
 * @brief
 * An error on a port that is connected but no longer enabled counts as a connect.
 */
SM_RESULT
PortMachine::OnSuspendReading(
    _In_ PortEvent Event)
{
    PSM_PORT_BITS bits;
    PortEvent change;

    if (Event == PortEvent::TransferFailed)
        return ParkChange20(TRUE);

    if (Event != PortEvent::TransferDone)
        return SmUnhandled();

    switch (PendingChange())
    {
        case PsmPendingChange:
            return SmTransition(&SuspendAcking);

        case PsmPendingError:
            return ParkChange20(TRUE);

        default:
            break;
    }

    change = NextChange();
    switch (change)
    {
        case PortEvent::ChangeResetDone:
        case PortEvent::ChangeDisabled:
            return ReportSuspendChange(PortEvent::ChangeError);

        case PortEvent::ChangeError:
            ReadPortBits(&bits);
            if (bits.Connected && !bits.Enabled)
                return ReportSuspendChange(PortEvent::ChangeConnect);
            return ReportSuspendChange(PortEvent::ChangeError);

        case PortEvent::ChangeResumed:
            return SmTransition(&SuspendRecovery);

        case PortEvent::ChangeOverCurrentCleared:
        case PortEvent::ChangeConnect:
        case PortEvent::ChangeOverCurrent:
            return ReportSuspendChange(change);

        default:
            SM_BREAK("Port change not valid on a suspended port");
            return SmHandled();
    }
}

SM_RESULT
PortMachine::OnSuspendAcking(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferFailed)
        return ParkChange20(TRUE);

    if (Event == PortEvent::TransferDone)
        return CheckPending20(&SuspendAcking, &SuspendReading);

    return SmUnhandled();
}

SM_RESULT
PortMachine::EnterSuspendRecovery()
{
    StartTimer(PsmTimerResumeRecovery);
    return SmHandled();
}

SM_RESULT
PortMachine::OnSuspendRecovery(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TimerExpired)
        return ReportSuspendChange(PortEvent::ChangeResumed);

    return SmUnhandled();
}

SM_RESULT
PortMachine::EnterSuspendReporting()
{
    if (m_SuspendReport == PortEvent::ChangeResumed)
        TellDevice(PsmNoticeResumed);

    ResumeInterruptTransfer();

    if (m_SuspendReport != PortEvent::ChangeConnect)
        return ReturnFromFrame(m_SuspendReport);

    if (!D3ColdEnabled())
        return ReturnFromFrame(PortEvent::ChangeConnect);

    /* The device lost power while suspended, give it time to come back */
    if (!DeviceConnected())
        return SmTransition(&D3Idle);

    StartTimer(PsmTimerDebounce);
    m_D3AfterResume = FALSE;
    return SmTransition(&D3Debouncing);
}

/* D3COLD *********************************************************************/

SM_RESULT
PortMachine::D3ConnectCheck(
    _In_ BOOLEAN AfterResume)
{
    if (DeviceConnected())
    {
        StartTimer(PsmTimerDebounce);
        m_D3AfterResume = AfterResume;
        return SmTransition(&D3Debouncing);
    }

    if (AfterResume)
        return ReturnFromFrame(PortEvent::ChangeConnect);

    return SmTransition(&D3Idle);
}

SM_RESULT
PortMachine::D3StopDebounceTimer()
{
    if (StopTimer())
        return D3ConnectCheck(m_D3AfterResume);

    return SmTransition(&D3DebounceFlush);
}

SM_RESULT
PortMachine::D3StopReconnectTimer(
    _In_ D3_PENDING Pending)
{
    if (StopTimer())
        return D3PendingDone(Pending);

    m_D3Pending = Pending;
    return SmTransition(&D3ReconnectFlush);
}

SM_RESULT
PortMachine::D3PendingDone(
    _In_ D3_PENDING Pending)
{
    switch (Pending)
    {
        case D3PendingOverCurrent:
            return ReturnFromFrame(PortEvent::ChangeOverCurrent);

        case D3PendingOverCurrentCleared:
            return ReturnFromFrame(PortEvent::ChangeOverCurrentCleared);

        case D3PendingHubReset:
            return ReturnFromFrame(PortEvent::ChangeNeedsHubReset);

        case D3PendingReattach:
            if (m_Usb3)
            {
                TellDevice(PsmNoticeEnabledOnReconnect);
                return ReturnFromFrame(PortEvent::ChangeResumed);
            }
            StartTimer(PsmTimerDebounce);
            m_D3AfterResume = TRUE;
            return SmTransition(&D3Debouncing);

        default:
            return ReturnFromFrame(PortEvent::HubStopping);
    }
}

SM_RESULT
PortMachine::OnD3Idle(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeDisabled:
        case PortEvent::ChangeResetDone:
        case PortEvent::ChangeError:
        case PortEvent::ChangeResumed:
            if (m_Usb3)
                return SmUnhandled();
            return D3ConnectCheck(FALSE);

        case PortEvent::ChangeConnect:
            if (!m_Usb3)
                return D3ConnectCheck(FALSE);
            if (!DevicePresent())
                return SmTransition(&D3Idle);
            TellDevice(PsmNoticeEnabledOnReconnect);
            return SmTransition(&D3ResumeAckWait);

        case PortEvent::ChangeOverCurrent:
        case PortEvent::ChangeOverCurrentCleared:
        case PortEvent::HubStopping:
            return ReturnFromFrame(Event);

        case PortEvent::ChangeNeedsHubReset:
            if (!m_Usb3)
                return SmUnhandled();
            return ReturnFromFrame(Event);

        case PortEvent::ResumeRequest:
        case PortEvent::DisableRequest:
            StartTimer(PsmTimerD3ColdReconnect);
            return SmTransition(&D3ReconnectWait);

        case PortEvent::HubSuspending:
            DropPortPowerReference();
            return SmTransition(&D3PoweredOff);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnD3PoweredOff(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::HubResumedInS0:
            return PowerUp(&D3Idle);

        case PortEvent::HubResumed:
        case PortEvent::HubResumedWithReset:
            if (m_Usb3)
            {
                TellDevice(PsmNoticeStateSuspended);
                return PowerUp(&D3Idle);
            }
            m_D3PowerUpWarm = (Event == PortEvent::HubResumed);
            return SmTransition(&D3PoweringUp);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::EnterD3PoweringUp()
{
    return SmCall(m_D3PowerUpWarm ? &ResumeReading : &ResumePowering);
}

SM_RESULT
PortMachine::OnD3PoweringUp(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeOverCurrent:
        case PortEvent::ChangeOverCurrentCleared:
            return ReturnFromFrame(Event);

        case PortEvent::SubDone:
        case PortEvent::ChangeConnect:
        case PortEvent::ChangeError:
        case PortEvent::ChangeResumed:
        case PortEvent::ChangeResetDone:
            if (!DeviceConnected())
                return ReturnFromFrame(PortEvent::ChangeConnect);
            TellDevice(PsmNoticeStateDisabled);
            return ReturnFromFrame(PortEvent::ChangeDisabled);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnD3Debouncing(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeError:
        case PortEvent::ChangeDisabled:
        case PortEvent::ChangeConnect:
        case PortEvent::ChangeResumed:
        case PortEvent::ChangeResetDone:
            return D3StopDebounceTimer();

        case PortEvent::ChangeOverCurrent:
            return D3StopReconnectTimer(D3PendingOverCurrent);

        case PortEvent::ChangeOverCurrentCleared:
            return D3StopReconnectTimer(D3PendingOverCurrentCleared);

        case PortEvent::TimerExpired:
            if (!m_D3AfterResume)
                return SmTransition(&D3ResumeAckWait);
            TellDevice(PsmNoticeStateDisabled);
            return ReturnFromFrame(PortEvent::ChangeDisabled);

        case PortEvent::HubStopping:
            m_D3FlushReturn = PortEvent::HubStopping;
            return SmTransition(&D3FlushThenReturn);

        case PortEvent::HubSuspending:
            if (m_D3AfterResume)
                return SmUnhandled();
            m_D3FlushReturn = PortEvent::HubSuspending;
            return SmTransition(&D3FlushThenReturn);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnD3DebounceFlush(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::TimerExpired:
            return D3ConnectCheck(m_D3AfterResume);

        case PortEvent::HubStopping:
            m_D3FlushReturn = PortEvent::HubStopping;
            return SmTransition(&D3FlushThenReturn);

        case PortEvent::HubSuspending:
            if (m_D3AfterResume)
                return SmUnhandled();
            m_D3FlushReturn = PortEvent::HubSuspending;
            return SmTransition(&D3FlushThenReturn);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnD3FlushThenReturn(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TimerExpired)
        return ReturnFromFrame(m_D3FlushReturn);

    return SmUnhandled();
}

SM_RESULT
PortMachine::OnD3ResumeAckWait(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeResetDone:
        case PortEvent::ChangeResumed:
        case PortEvent::ChangeDisabled:
        case PortEvent::ChangeError:
            if (m_Usb3)
                return SmUnhandled();
            return D3ConnectCheck(FALSE);

        case PortEvent::ChangeConnect:
            if (!m_Usb3)
                return D3ConnectCheck(FALSE);
            if (DevicePresent())
                return SmTransition(&D3ResumeAckWait);
            return ReturnFromFrame(PortEvent::ChangeConnect);

        case PortEvent::ChangeOverCurrent:
        case PortEvent::ChangeOverCurrentCleared:
        case PortEvent::HubStopping:
            return ReturnFromFrame(Event);

        case PortEvent::ChangeNeedsHubReset:
            if (!m_Usb3)
                return SmUnhandled();
            return ReturnFromFrame(Event);

        case PortEvent::ResumeRequest:
            if (m_Usb3)
                return ReturnFromFrame(PortEvent::ChangeResumed);
            TellDevice(PsmNoticeStateDisabled);
            return ReturnFromFrame(PortEvent::ChangeDisabled);

        case PortEvent::DisableRequest:
            if (m_Usb3)
                return SmUnhandled();
            TellDevice(PsmNoticeStateDisabled);
            return ReturnFromFrame(PortEvent::ChangeDisabled);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnD3ReconnectWait(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::ChangeResumed:
        case PortEvent::ChangeError:
        case PortEvent::ChangeResetDone:
            if (m_Usb3)
                return SmUnhandled();
            if (DeviceConnected())
                return D3StopReconnectTimer(D3PendingReattach);
            return SmTransition(&D3ReconnectWait);

        case PortEvent::ChangeConnect:
            if (m_Usb3 ? DevicePresent() : DeviceConnected())
                return D3StopReconnectTimer(D3PendingReattach);
            return SmTransition(&D3ReconnectWait);

        case PortEvent::ChangeDisabled:
            if (m_Usb3)
                return SmUnhandled();
            return D3StopReconnectTimer(D3PendingReattach);

        case PortEvent::ChangeOverCurrent:
            return D3StopReconnectTimer(D3PendingOverCurrent);

        case PortEvent::ChangeOverCurrentCleared:
            return D3StopReconnectTimer(D3PendingOverCurrentCleared);

        case PortEvent::ChangeNeedsHubReset:
            if (!m_Usb3)
                return SmUnhandled();
            return D3StopReconnectTimer(D3PendingHubReset);

        case PortEvent::HubStopping:
            return D3StopReconnectTimer(D3PendingStop);

        case PortEvent::TimerExpired:
            return ReturnFromFrame(PortEvent::ChangeConnect);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnD3ReconnectFlush(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TimerExpired)
        return D3PendingDone(m_D3Pending);

    /* Only the hub stop flush has nothing more to wait for */
    if ((Event == PortEvent::HubStopping) && (m_D3Pending != D3PendingStop))
    {
        m_D3Pending = D3PendingStop;
        return SmTransition(&D3ReconnectFlush);
    }

    return SmUnhandled();
}

/* REMOTE WAKE ****************************************************************/

SM_RESULT
PortMachine::EnterWakeArming()
{
    BOOLEAN enable = HubArmedForWake();

    if (!enable)
        enable = BootDeviceReturning();

    KeepHubAwake();
    SendRemoteWake(enable);
    return SmHandled();
}

SM_RESULT
PortMachine::OnWakeArming(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::TransferFailed)
        RequestHubReset();
    else if (Event != PortEvent::TransferDone)
        return SmUnhandled();

    return ReturnFromFrame(PortEvent::SubDone);
}
