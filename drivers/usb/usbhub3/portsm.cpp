/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Port state machine: event table, engine hooks, shared steps
 *              and the states used while the hub is not running
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include "usbhub3.h"

/* GLOBALS ********************************************************************/

const SM_EVENT_INFO PortMachine::EventInfo[] =
{
    { "TimerExpired",               SmEventCompletion, FALSE, FALSE },
    { "ResetPollExpired",           SmEventCritical,   FALSE, FALSE },
    { "TransferDone",               SmEventCritical,   FALSE, FALSE },
    { "TransferFailed",             SmEventCritical,   FALSE, FALSE },
    { "PortCleanup",                SmEventCompletion, FALSE, FALSE },
    { "HubStarted",                 SmEventCompletion, FALSE, FALSE },
    { "HubResumed",                 SmEventCompletion, FALSE, FALSE },
    { "HubResumedInS0",             SmEventCompletion, FALSE, FALSE },
    { "HubResumedWithReset",        SmEventCompletion, FALSE, FALSE },
    { "HubStopping",                SmEventCompletion, FALSE, FALSE },
    { "HubSuspending",              SmEventCompletion, FALSE, FALSE },
    { "HubResetting",               SmEventCompletion, FALSE, FALSE },
    { "StatusChanged",              SmEventCompletion, FALSE, FALSE },
    { "HubRemoved",                 SmEventCompletion, FALSE, FALSE },
    { "UserOverCurrentReset",       SmEventCompletion, FALSE, FALSE },
    { "DeviceGone",                 SmEventCompletion, FALSE, FALSE },
    { "HibernateRequest",           SmEventRequest,    FALSE, FALSE },
    { "CycleRequest",               SmEventRequest,    FALSE, FALSE },
    { "DisableRequest",             SmEventRequest,    FALSE, FALSE },
    { "DisableSuperSpeedRequest",   SmEventRequest,    FALSE, FALSE },
    { "ResetRequest",               SmEventRequest,    FALSE, FALSE },
    { "WarmResetRequest",           SmEventRequest,    FALSE, FALSE },
    { "ResumeRequest",              SmEventRequest,    FALSE, FALSE },
    { "SuspendRequest",             SmEventRequest,    FALSE, FALSE },
    { "SetU1Timeout",               SmEventRequest,    FALSE, TRUE  },
    { "SetU2Timeout",               SmEventRequest,    FALSE, TRUE  },
    { "ChangeConnect",              SmEventCompletion, FALSE, FALSE },
    { "ChangeOverCurrent",          SmEventCompletion, FALSE, FALSE },
    { "ChangeOverCurrentCleared",   SmEventCompletion, FALSE, FALSE },
    { "ChangeResetDone",            SmEventCompletion, FALSE, FALSE },
    { "ChangeResumed",              SmEventCompletion, FALSE, FALSE },
    { "ChangeDisabled",             SmEventCompletion, FALSE, FALSE },
    { "ChangeError",                SmEventCompletion, FALSE, FALSE },
    { "ChangeLinkError",            SmEventCompletion, FALSE, FALSE },
    { "ChangeNone",                 SmEventCompletion, FALSE, FALSE },
    { "ChangeResetBusy",            SmEventCompletion, FALSE, FALSE },
    { "ChangeNeedsHubReset",        SmEventCompletion, FALSE, FALSE },
    { "ChangeNeedsReset",           SmEventCompletion, FALSE, FALSE },
    { "ErrorIgnore",                SmEventCompletion, FALSE, FALSE },
    { "ErrorCycle",                 SmEventCompletion, FALSE, FALSE },
    { "ErrorDisableAndCycle",       SmEventCompletion, FALSE, FALSE },
    { "ErrorEnabledOnConnect",      SmEventCompletion, FALSE, FALSE },
    { "ErrorEnabledWhileReset",     SmEventCompletion, FALSE, FALSE },
    { "SubDone",                    SmEventCompletion, FALSE, FALSE },
};

static_assert(RTL_NUMBER_OF(PortMachine::EventInfo) == static_cast<ULONG>(PortEvent::Count),
              "EventInfo must have one entry per PortEvent");

/* Device requests a port drops while it has nothing to do with them */
static const PortEvent IdleRequests[] =
{
    PortEvent::HubRemoved, PortEvent::HubStopping, PortEvent::DeviceGone,
    PortEvent::HibernateRequest, PortEvent::CycleRequest, PortEvent::DisableRequest,
    PortEvent::DisableSuperSpeedRequest, PortEvent::ResetRequest, PortEvent::WarmResetRequest,
    PortEvent::ResumeRequest, PortEvent::SuspendRequest, PortEvent::SetU1Timeout,
    PortEvent::SetU2Timeout, PortEvent::Count
};

static const PortEvent OffAttachedDrops[] =
{
    PortEvent::HubRemoved, PortEvent::StatusChanged, PortEvent::HubStopping,
    PortEvent::HibernateRequest, PortEvent::DisableRequest, PortEvent::DisableSuperSpeedRequest,
    PortEvent::ResetRequest, PortEvent::WarmResetRequest, PortEvent::ResumeRequest,
    PortEvent::SuspendRequest, PortEvent::SetU1Timeout, PortEvent::SetU2Timeout, PortEvent::Count
};

static const PortEvent RemovedAndStopped[] =
{
    PortEvent::HubRemoved, PortEvent::HubStopping, PortEvent::Count
};

static const PortEvent StatusAndRemoved[] =
{
    PortEvent::StatusChanged, PortEvent::HubRemoved, PortEvent::Count
};

static const PortEvent GoneOnly[] =
{
    PortEvent::DeviceGone, PortEvent::Count
};

/* Hub is stopped or suspended, the port only waits for the hub to come back */
const PortMachine::State PortMachine::Off =
    { NULL, "Off", SM_STATE_TAKES_REQUESTS | PSM_STATE_IDLE, &PortMachine::OnOff, NULL, NULL };

/* Where a new port starts: OffEmpty without its entry having run */
const PortMachine::State PortMachine::OffInitial =
    { &Off, "OffInitial", 0, &PortMachine::OnOffEmpty, NULL, IdleRequests };

const PortMachine::State PortMachine::OffEmpty =
    { &Off, "OffEmpty", 0, &PortMachine::OnOffEmpty, &PortMachine::EnterOffEmpty, IdleRequests };

const PortMachine::State PortMachine::OffAttached =
    { &Off, "OffAttached", 0, &PortMachine::OnOffAttached, NULL, OffAttachedDrops };

/* 3.0 port whose device went away while the port still needs a reset */
const PortMachine::State PortMachine::OffResetNeeded =
    { &Off, "OffResetNeeded", 0, &PortMachine::OnOffResetNeeded, NULL, IdleRequests };

const PortMachine::State PortMachine::OffSuperSpeedDisabled =
    { &Off, "OffSuperSpeedDisabled", 0,
      &PortMachine::OnOffSuperSpeedDisabled, NULL, RemovedAndStopped };

const PortMachine::State PortMachine::OffSuspendedUsb3 =
    { &Off, "OffSuspendedUsb3", 0,
      &PortMachine::OnOffSuspended, &PortMachine::EnterOffSuspended, StatusAndRemoved };

/* A suspended 2.0 port behind a suspended hub leaves device requests queued */
const PortMachine::State PortMachine::OffSuspendedUsb2 =
    { NULL, "OffSuspendedUsb2", PSM_STATE_IDLE,
      &PortMachine::OnOffSuspended, &PortMachine::EnterOffSuspended, StatusAndRemoved };

const PortMachine::State PortMachine::CleaningUp =
    { NULL, "CleaningUp", 0, &PortMachine::OnCleaningUp, &PortMachine::EnterCleaningUp, NULL };

const PortMachine::State PortMachine::Deleted =
    { NULL, "Deleted", PSM_STATE_IDLE_USB2, NULL, NULL, GoneOnly };

/* Transient, hands m_ReturnEvent to the frame that called this one */
const PortMachine::State PortMachine::Returning =
    { NULL, "Returning", 0, NULL, &PortMachine::EnterReturning, NULL };

/* FUNCTIONS ******************************************************************/

VOID
PortMachine::Initialize(
    _In_ HubPort* Port,
    _In_ BOOLEAN SuperSpeed)
{
    m_Port = Port;
    m_Usb3 = SuperSpeed;
    m_HubPowerHeld = FALSE;
    m_AfterTimer = FollowConnect;
    m_OldDevice = OldDevicePlain;
    m_ReadKind = ReadPlain;
    m_D3Pending = D3PendingStop;
    m_SuspendReport = PortEvent::ChangeError;
    m_D3FlushReturn = PortEvent::HubStopping;
    m_ReturnEvent = PortEvent::SubDone;
    m_ReadFresh = FALSE;
    m_WarmResume = FALSE;
    m_AttachForcedReset = FALSE;
    m_LpmU2 = FALSE;
    m_ArmForEmpty = FALSE;
    m_ParkResetsHub = FALSE;
    m_D3AfterResume = FALSE;
    m_D3PowerUpWarm = FALSE;
    m_ReconnectFlushOnStop = FALSE;
    m_DrainReturnsEmpty = FALSE;

    SmInitialize(&OffInitial);
}

/* ENGINE HOOKS ***************************************************************/

VOID
PortMachine::SmReference()
{
    ReferencePort();
}

VOID
PortMachine::SmDereference()
{
    DereferencePort();
}

VOID
PortMachine::SmQueuePassive()
{
    QueuePassiveWork();
}

VOID
PortMachine::SmOnDequeue(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::DeviceGone)
        ForgetDetachedDevice();
}

static
BOOLEAN
NTAPI
PsmHasFlag(
    _In_ const PortMachine::State* Target,
    _In_ ULONG Flag)
{
    for (; Target != NULL; Target = Target->Parent)
    {
        if (Target->Flags & Flag)
            return TRUE;
    }

    return FALSE;
}

BOOLEAN
PortMachine::AllowsHubSuspend(
    _In_ const State* Target) const
{
    return PsmHasFlag(Target, m_Usb3 ? PSM_STATE_IDLE_USB3 : PSM_STATE_IDLE_USB2);
}

/**
 * @brief
 * The innermost frame that does not yield to its caller decides whether the port holds the hub awake.
 */
VOID
PortMachine::SmStateChanged()
{
    ULONG frame = SmDepth();

    while ((frame > 0) && PsmHasFlag(SmFrame(frame), SM_STATE_YIELDS_TO_CALLER))
        frame -= 1;

    UpdateHubPower(AllowsHubSuspend(SmFrame(frame)));
}

VOID
PortMachine::UpdateHubPower(
    _In_ BOOLEAN Idle)
{
    if (Idle)
    {
        if (m_HubPowerHeld)
        {
            HubPowerDrop();
            m_HubPowerHeld = FALSE;
        }
    }
    else if (!m_HubPowerHeld && HubPowerTake())
    {
        m_HubPowerHeld = TRUE;
    }
}

/* Work done in code that used to pass through a busy state */
VOID
PortMachine::KeepHubAwake()
{
    UpdateHubPower(FALSE);
}

/* SHARED STEPS ***************************************************************/

SM_RESULT
PortMachine::PowerDown(
    _In_ const State* Target)
{
    DropPortPowerReference();
    return SmTransition(Target);
}

SM_RESULT
PortMachine::PowerUp(
    _In_ const State* Target)
{
    TakePortPowerReference();
    return SmTransition(Target);
}

SM_RESULT
PortMachine::ResetHubThen(
    _In_ const State* Target)
{
    RequestHubReset();
    return SmTransition(Target);
}

SM_RESULT
PortMachine::ReturnFromFrame(
    _In_ PortEvent Event)
{
    m_ReturnEvent = Event;
    return SmTransition(&Returning);
}

SM_RESULT
PortMachine::EnterReturning()
{
    return SmReturn(m_ReturnEvent);
}

/* Something may be on the empty port, look before debouncing */
SM_RESULT
PortMachine::CheckConnection()
{
    if (!m_Usb3)
        return SmTransition(DeviceConnected() ? &DebouncingUsb2 : &Disconnected);

    if (!DevicePresent())
        return SmTransition(&Disconnected);

    return CheckOldDevice(LinkInU0() ? OldDevicePlain : OldDeviceResetNeeded);
}

SM_RESULT
PortMachine::CheckOldDevice(
    _In_ OLD_DEVICE_KIND Kind)
{
    if (OldDevicePresent())
    {
        m_OldDevice = Kind;
        return SmTransition(&OldDeviceWait);
    }

    return DeviceArrived(Kind);
}

/**
 * @brief
 * Drops requests the previous device left behind, then reattaches or creates a device.
 */
SM_RESULT
PortMachine::DeviceArrived(
    _In_ OLD_DEVICE_KIND Kind)
{
    static const PortEvent Stale20[] =
    {
        PortEvent::ResumeRequest, PortEvent::DisableRequest, PortEvent::CycleRequest,
        PortEvent::SuspendRequest, PortEvent::ResetRequest, PortEvent::HibernateRequest
    };
    static const PortEvent Stale30[] =
    {
        PortEvent::WarmResetRequest, PortEvent::HibernateRequest, PortEvent::ResetRequest,
        PortEvent::CycleRequest, PortEvent::ResumeRequest, PortEvent::DeviceGone,
        PortEvent::SetU1Timeout, PortEvent::SuspendRequest, PortEvent::DisableRequest,
        PortEvent::SetU2Timeout, PortEvent::DisableSuperSpeedRequest
    };
    ULONG index;

    if (!m_Usb3)
    {
        for (index = 0; index < RTL_NUMBER_OF(Stale20); index++)
            SmDiscardQueued(Stale20[index]);

        if (BootDeviceReturning())
        {
            ReattachBootDevice();
            return SmTransition(&AttachedDisabled);
        }

        if (!CreateDevice())
            return SmTransition(&DebouncingUsb2);

        MarkUsb2Device();
        if (!ConnectDevice())
            return SmTransition(&AttachFailedWait);

        return SmTransition(&AttachedDisabled);
    }

    for (index = 0; index < RTL_NUMBER_OF(Stale30); index++)
        SmDiscardQueued(Stale30[index]);

    if ((Kind == OldDevicePlain) && NeedsDebounce())
        return SmTransition(&DebouncingUsb3);

    return ReattachOrCreate(Kind);
}

SM_RESULT
PortMachine::ReattachOrCreate(
    _In_ OLD_DEVICE_KIND Kind)
{
    if (BootDeviceReturning())
    {
        ReattachBootDevice();
        return SmTransition((Kind == OldDeviceResetInProgress) ? &ResetRequestWait : &Enabled);
    }

    if (Kind == OldDeviceResetInProgress)
    {
        /* The previous table retried here until creation worked */
        while (!CreateDevice())
        {
        }
    }
    else if (!CreateDevice())
    {
        return CheckOldDevice(Kind);
    }

    if (Kind != OldDevicePlain)
        ForceResetOnEnumeration();

    MarkUsb3Device();
    if (!ConnectDevice())
    {
        m_AttachForcedReset = (Kind != OldDevicePlain);
        return SmTransition(&AttachFailedWait);
    }

    if (Kind == OldDevicePlain)
        return SmTransition(&Enabled);

    return SmTransition((Kind == OldDeviceResetNeeded) ? &AttachedDisabled : &ResetRequestWait);
}

SM_RESULT
PortMachine::StartOverCurrentWait()
{
    StartTimer(PsmTimerOverCurrent);
    return SmTransition(&OverCurrentWait);
}

/* A port change on an attached port without a running timer */
SM_RESULT
PortMachine::DetachThen(
    _In_ PortEvent Change)
{
    DisconnectDevice();

    if (Change == PortEvent::ChangeOverCurrent)
        return StartOverCurrentWait();

    if (Change == PortEvent::ChangeOverCurrentCleared)
        return SmTransition(&PoweringPort);

    return CheckConnection();
}

/* Same, while the port holds an interrupt reference and runs a timer */
SM_RESULT
PortMachine::DetachTimed(
    _In_ PortEvent Change)
{
    if ((Change != PortEvent::ChangeOverCurrentCleared) && !m_Usb3)
    {
        DropInterruptReference();
        DisconnectDevice();
    }
    else
    {
        DisconnectDevice();
        DropInterruptReference();
    }

    if (Change == PortEvent::ChangeOverCurrent)
        return StopTimerThen(FollowOverCurrent);

    if (Change == PortEvent::ChangeOverCurrentCleared)
        return StopTimerThen(FollowOverCurrentCleared);

    return StopTimerThen(FollowConnect);
}

SM_RESULT
PortMachine::CycleDetach()
{
    DisconnectDevice();
    m_OldDevice = m_Usb3 ? OldDeviceResetNeeded : OldDevicePlain;
    return SmTransition(&OldDeviceWait);
}

SM_RESULT
PortMachine::StopTimerThen(
    _In_ TIMER_FOLLOW_UP FollowUp)
{
    if (StopTimer())
        return FollowTimer(FollowUp);

    m_AfterTimer = FollowUp;
    return SmTransition(&TimerFlush);
}

SM_RESULT
PortMachine::FollowTimer(
    _In_ TIMER_FOLLOW_UP FollowUp)
{
    if (FollowUp == FollowOverCurrent)
        return StartOverCurrentWait();

    if (FollowUp == FollowOverCurrentCleared)
        return SmTransition(&PoweringPort);

    return CheckConnection();
}

/**
 * @brief
 * Handles a 2.0 port status that makes no sense for an attached port.
 */
SM_RESULT
PortMachine::HandleConnectedFault(
    _In_ const State* Stay)
{
    PortEvent response = ErrorResponse();

    switch (response)
    {
        case PortEvent::ErrorIgnore:
            return SmTransition(Stay);

        case PortEvent::ChangeNeedsHubReset:
            if (Stay == &ResumeAckWait)
                return ResetHubThen(Stay);
            return SmTransition(&HubResetIssued);

        case PortEvent::ErrorCycle:
            return CycleDetach();

        case PortEvent::ErrorDisableAndCycle:
            DisconnectDevice();
            return SmTransition(&DisablingForCycle);

        case PortEvent::ChangeConnect:
        case PortEvent::ChangeOverCurrent:
        case PortEvent::ChangeOverCurrentCleared:
            return DetachThen(response);

        default:
            SM_BREAK("Port error response not valid here");
            return SmHandled();
    }
}

/* Same for a 2.0 port with a timer running, DuringReset picks the reset rules */
SM_RESULT
PortMachine::TimedError(
    _In_ const State* Stay,
    _In_ BOOLEAN DuringReset)
{
    PortEvent response = DuringReset ? ErrorResponseDuringReset() : ErrorResponse();

    switch (response)
    {
        case PortEvent::ErrorIgnore:
            return SmTransition(Stay);

        case PortEvent::ErrorEnabledWhileReset:
            return SmTransition(&ResetEnabledWait);

        case PortEvent::ChangeNeedsHubReset:
            return TimedHubReset();

        case PortEvent::ErrorCycle:
            DropInterruptReference();
            DisconnectDevice();
            return StopTimerThen(FollowConnect);

        case PortEvent::ErrorDisableAndCycle:
            DropInterruptReference();
            DisconnectDevice();
            return SmTransition(&DisablingForCycleTimed);

        case PortEvent::ChangeConnect:
        case PortEvent::ChangeOverCurrent:
        case PortEvent::ChangeOverCurrentCleared:
            return DetachTimed(response);

        default:
            SM_BREAK("Port error response not valid here");
            return SmHandled();
    }
}

SM_RESULT
PortMachine::TimedHubReset()
{
    RequestHubReset();
    return ReleaseAndStopTimer();
}

/* Give up on a timed operation: fail it to the device and wait for the hub */
SM_RESULT
PortMachine::ReleaseAndStopTimer()
{
    BOOLEAN stopped = StopTimer();

    TellDevice(PsmNoticeFailed);
    DropInterruptReference();
    return SmTransition(stopped ? &HubResetWait : &HubResetTimerWait);
}

SM_RESULT
PortMachine::TakeInterruptForReset(
    _In_ const State* Start,
    _In_ const State* Refused)
{
    if (TakeInterruptReference())
        return SmTransition(Start);

    TellDevice(PsmNoticeResetRefused);
    return SmTransition(Refused);
}

SM_RESULT
PortMachine::ResetSucceeded()
{
    TellDevice(PsmNoticeResetComplete);
    DropInterruptReference();
    return SmTransition(&Enabled);
}

SM_RESULT
PortMachine::ResetTransferFailed()
{
    if (m_Usb3)
    {
        DropInterruptReference();
        TellDevice(PsmNoticeFailed);
    }
    else
    {
        TellDevice(PsmNoticeFailed);
        DropInterruptReference();
    }

    return SmTransition(&HubResetIssued);
}

SM_RESULT
PortMachine::ResumeSucceeded()
{
    if (m_Usb3)
    {
        DropInterruptReference();
        TellDevice(PsmNoticeResumed);
    }
    else
    {
        TellDevice(PsmNoticeResumed);
        DropInterruptReference();
    }

    return SmTransition(&Enabled);
}

SM_RESULT
PortMachine::FailToHubReset()
{
    TellDevice(PsmNoticeFailed);
    return SmTransition(&HubResetIssued);
}

/* HUB NOT RUNNING ************************************************************/

SM_RESULT
PortMachine::OnOff(
    _In_ PortEvent Event)
{
    UNREFERENCED_PARAMETER(Event);
    return SmUnhandled();
}

SM_RESULT
PortMachine::EnterOffEmpty()
{
    ClearPortStatus();
    return SmHandled();
}

SM_RESULT
PortMachine::OnOffEmpty(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::HubResumed:
        case PortEvent::HubResumedWithReset:
            return SmTransition(&StartingEmpty);

        case PortEvent::HubStarted:
            if (m_Usb3 && SuperSpeedBlocked())
                return SmTransition(&DisablingSuperSpeed);
            return SmTransition(&StartingEmpty);

        case PortEvent::HubResumedInS0:
            return PowerUp(&Disconnected);

        case PortEvent::HubResetting:
            DropHubResetReference();
            return SmTransition(&OffEmpty);

        case PortEvent::PortCleanup:
            return SmTransition(&Deleted);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnOffAttached(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::HubResumed:
        case PortEvent::HubResumedWithReset:
            if (!m_Usb3)
                return SmTransition(&ResumingDisabled);
            TellDevice(PsmNoticeStateDisabled);
            return SmTransition(&ErrorRestarting);

        case PortEvent::HubStarted:
            return SmTransition(m_Usb3 ? &ErrorRestarting : &StartingAttached);

        case PortEvent::HubResumedInS0:
            return PowerUp(&AttachedDisabled);

        case PortEvent::HubResetting:
            DisconnectDevice();
            DropHubResetReference();
            return SmTransition(&OffEmpty);

        case PortEvent::CycleRequest:
            DisconnectDevice();
            return SmTransition(m_Usb3 ? &OffResetNeeded : &OffEmpty);

        case PortEvent::PortCleanup:
            return SmTransition(&CleaningUp);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnOffResetNeeded(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::HubResumed:
        case PortEvent::HubResumedWithReset:
        case PortEvent::HubStarted:
        case PortEvent::HubResumedInS0:
            TakePortPowerReference();
            return CheckOldDevice(OldDeviceResetNeeded);

        case PortEvent::HubResetting:
            DropHubResetReference();
            return SmTransition(&OffEmpty);

        case PortEvent::PortCleanup:
            return SmTransition(&Deleted);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::OnOffSuperSpeedDisabled(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::HubResumed:
        case PortEvent::HubResumedInS0:
            return PowerUp(&SsLinkDisabled);

        case PortEvent::HubStarted:
        case PortEvent::HubResumedWithReset:
            return SmTransition(&DisablingSuperSpeed);

        case PortEvent::HubResetting:
            DropHubResetReference();
            return SmTransition(&OffSuperSpeedDisabled);

        case PortEvent::PortCleanup:
            return SmTransition(&Deleted);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::EnterOffSuspended()
{
    DropPortPowerReference();
    return SmHandled();
}

SM_RESULT
PortMachine::OnOffSuspended(
    _In_ PortEvent Event)
{
    switch (Event)
    {
        case PortEvent::HubResumedInS0:
            return PowerUp(&Suspended);

        case PortEvent::HubStopping:
            return SmTransition(&OffAttached);

        case PortEvent::HubResumed:
        case PortEvent::HubResumedWithReset:
            if (m_Usb3)
                return SmTransition(&SystemResuming);
            m_WarmResume = (Event == PortEvent::HubResumed);
            return SmTransition(&ResumingAttached);

        case PortEvent::CycleRequest:
            DisconnectDevice();
            return SmTransition(m_Usb3 ? &OffResetNeeded : &OffEmpty);

        case PortEvent::PortCleanup:
            return SmTransition(&CleaningUp);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
PortMachine::EnterCleaningUp()
{
    DisconnectDevice();
    return SmHandled();
}

SM_RESULT
PortMachine::OnCleaningUp(
    _In_ PortEvent Event)
{
    if (Event == PortEvent::DeviceGone)
        return SmTransition(&Deleted);

    return SmUnhandled();
}
