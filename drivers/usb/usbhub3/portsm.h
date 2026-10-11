/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Port state machine for USB 2 and USB 3 hub ports
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include <drivers/usb3/smengine.h>

class HubPort;

enum class PortEvent : UCHAR
{
    /* Timers and port control transfers */
    TimerExpired,
    ResetPollExpired,
    TransferDone,
    TransferFailed,

    /* Hub machine fan out and port object lifetime */
    PortCleanup,
    HubStarted,
    HubResumed,
    HubResumedInS0,
    HubResumedWithReset,
    HubStopping,
    HubSuspending,
    HubResetting,
    StatusChanged,
    HubRemoved,
    UserOverCurrentReset,

    /* From the device machine */
    DeviceGone,
    HibernateRequest,
    CycleRequest,
    DisableRequest,
    DisableSuperSpeedRequest,
    ResetRequest,
    WarmResetRequest,
    ResumeRequest,
    SuspendRequest,
    SetU1Timeout,
    SetU2Timeout,

    /* Results handed back by called states and change queries, never posted */
    ChangeConnect,
    ChangeOverCurrent,
    ChangeOverCurrentCleared,
    ChangeResetDone,
    ChangeResumed,
    ChangeDisabled,
    ChangeError,
    ChangeLinkError,
    ChangeNone,
    ChangeResetBusy,
    ChangeNeedsHubReset,
    ChangeNeedsReset,
    ErrorIgnore,
    ErrorCycle,
    ErrorDisableAndCycle,
    ErrorEnabledOnConnect,
    ErrorEnabledWhileReset,
    SubDone,

    Count
};

/** Notices the port sends to the device machine. */
typedef enum _PSM_DEVICE_NOTICE
{
    PsmNoticeFailed,
    PsmNoticeDisabled,
    PsmNoticeResetRefused,
    PsmNoticeResumed,
    PsmNoticeSuspended,
    PsmNoticeResumeRefused,
    PsmNoticeResumeTimedOut,
    PsmNoticeStateDisabled,
    PsmNoticeStateEnabled,
    PsmNoticeStateSuspended,
    PsmNoticeResetTimedOut,
    PsmNoticeResetComplete,
    PsmNoticeTimeoutUpdated,
    PsmNoticeEnabledOnReconnect
} PSM_DEVICE_NOTICE;

/** What the single port timer is armed for. */
typedef enum _PSM_TIMER
{
    PsmTimerDebounce,
    PsmTimerOverCurrent,
    PsmTimerResetCompletion,
    PsmTimerResumeCompletion,
    PsmTimerResumeRecovery,
    PsmTimerPowerOn,
    PsmTimerReconnect,
    PsmTimerD3ColdReconnect,
    PsmTimerSuperSpeedDisable,
    PsmTimerResetPoll
} PSM_TIMER;

/** Answer of the "is a change bit still set" query. */
typedef enum _PSM_PENDING
{
    PsmPendingChange,
    PsmPendingNone,
    PsmPendingError
} PSM_PENDING;

/** Saved 2.0 port status bits the two status decisions look at. */
typedef struct _PSM_PORT_BITS
{
    BOOLEAN Connected;
    BOOLEAN Enabled;
    BOOLEAN OverCurrent;
    BOOLEAN OverCurrentChanged;
} PSM_PORT_BITS, *PPSM_PORT_BITS;

/* Port specific state flags, inherited from parents like the engine flags */
#define PSM_STATE_IDLE_USB2     0x00000100  /**< A 2.0 port resting here lets the hub suspend */
#define PSM_STATE_IDLE_USB3     0x00000200  /**< Same for a 3.0 port */
#define PSM_STATE_IDLE          (PSM_STATE_IDLE_USB2 | PSM_STATE_IDLE_USB3)

class PortMachine : public SmMachine<PortMachine, PortEvent>
{
    friend class SmMachine<PortMachine, PortEvent>;

public:
    VOID
    Initialize(
        _In_ HubPort* Port,
        _In_ BOOLEAN SuperSpeed);

    HubPort*
    Port() const
    {
        return m_Port;
    }

    BOOLEAN
    IsSuperSpeed() const
    {
        return m_Usb3;
    }

    BOOLEAN
    HoldsHubPower() const
    {
        return m_HubPowerHeld;
    }

    static const SM_EVENT_INFO EventInfo[];

private:
    /** What runs once the port timer is stopped or has fired. */
    enum TIMER_FOLLOW_UP
    {
        FollowConnect,
        FollowCycle,
        FollowOverCurrent,
        FollowOverCurrentCleared
    };

    /** Which kind of reconnect a 3.0 port is waiting on the old device for. */
    enum OLD_DEVICE_KIND
    {
        OldDevicePlain,
        OldDeviceResetNeeded,
        OldDeviceResetInProgress
    };

    /** How the 3.0 status reading frame ends. */
    enum READ_KIND
    {
        ReadPlain,
        ReadNoChange,
        ReadStartResume
    };

    /** What a D3cold reconnect timer flush hands back when it fires. */
    enum D3_PENDING
    {
        D3PendingStop,
        D3PendingOverCurrent,
        D3PendingOverCurrentCleared,
        D3PendingReattach,
        D3PendingHubReset
    };

public:
    /* States, defined in portsm.cpp, portsm_port.cpp and portsm_frames.cpp */
    static const State Off;
    static const State OffInitial;
    static const State OffEmpty;
    static const State OffAttached;
    static const State OffResetNeeded;
    static const State OffSuperSpeedDisabled;
    static const State OffSuspendedUsb3;
    static const State OffSuspendedUsb2;
    static const State CleaningUp;
    static const State Deleted;

    static const State Empty;
    static const State Disconnected;
    static const State StartingEmpty;
    static const State Debouncing;
    static const State DebouncingUsb2;
    static const State DebouncingUsb3;
    static const State OldDeviceWait;
    static const State OverCurrentWait;
    static const State NotifyingOverCurrent;
    static const State UserResetWait;
    static const State TimerFlush;
    static const State FlushBeforeOffEmpty;
    static const State FlushBeforeOffAttached;
    static const State FlushBeforeSuperSpeedOff;

    static const State Attached;
    static const State Enabled;
    static const State AttachedDisabled;
    static const State Suspended;
    static const State DisabledInSuspend;
    static const State SuspendedD3Cold;
    static const State ResumeAckWait;
    static const State ResetRequestWait;
    static const State StartingAttached;
    static const State ResumingAttached;
    static const State ResumingDisabled;
    static const State SystemResuming;
    static const State ErrorRestarting;
    static const State ResumeAckWaitStopped;

    static const State Timed;
    static const State ResetWait;
    static const State ResetEnabledWait;
    static const State ResetFlush;
    static const State ResumeWait;
    static const State ResumeRecovery;
    static const State ResumeFlush;

    static const State HubResetIssued;
    static const State HubResetWait;
    static const State HubResetTimerWait;
    static const State EmptyHubResetWait;
    static const State EmptyHubResetTimerWait;
    static const State AttachFailedWait;
    static const State SuperSpeedOffWait;
    static const State SsLinkDisabled;

    static const State Transfer;
    static const State PoweringPort;
    static const State QueryingOverCurrent;
    static const State SendingReset;
    static const State SendingWarmReset;
    static const State SendingResume;
    static const State SendingSuspend;
    static const State DisablingOnRequest;
    static const State DisablingForCycle;
    static const State DisablingBeforeConnect;
    static const State QuiescingForHubSuspend;
    static const State DisablingForCycleTimed;
    static const State DisablingOnSuspendTimed;
    static const State DisablingOnResetTimeout;
    static const State ReadingForResume;
    static const State ReadingAfterResumeTimeout;
    static const State SettingLpmTimeout;
    static const State DisablingSuperSpeed;
    static const State DisablingSuperSpeedOnRequest;
    static const State EnablingSuperSpeedOnStop;
    static const State EnablingSuperSpeedOnTimer;
    static const State ResetTimeoutStatus;
    static const State ArmingWake;

    /* Called frames */
    static const State ChangeIdle20;
    static const State ChangeReading20;
    static const State ChangeAcking20;
    static const State ChangeReporting20;
    static const State ChangeParked20;

    static const State ChangeIdle30;
    static const State ChangeChecking30;
    static const State ResetChangeIdle;
    static const State ResetChangeChecking;
    static const State ResetPollWait;
    static const State ResetPollChecking;
    static const State ResumeChangeIdle;
    static const State ResumeChangeChecking;

    static const State StatusReading;
    static const State StatusAcking;
    static const State StatusReporting;
    static const State StatusFailing;

    static const State DrainIdle;
    static const State DrainEmptyIdleUsb2;
    static const State DrainEmptyIdleUsb3;
    static const State DrainReading;
    static const State DrainAcking;
    static const State DrainRearming;
    static const State DrainChecking;

    static const State PowerOnSending;
    static const State PowerOnSettling;
    static const State StartReading20;
    static const State StartReadingFirst;
    static const State StartReading30;
    static const State PowerUpResetHub;
    static const State PowerUpRearming;

    static const State ResumePowering;
    static const State ResumeSettling;
    static const State ResumeReading;
    static const State ResumeAckingConnect;
    static const State ReconnectWait;
    static const State ReconnectReading;
    static const State ReconnectAcking;
    static const State ReconnectRearming;
    static const State ReconnectFlush;
    static const State ResumeRereading;
    static const State SystemResumeReading;
    static const State SystemReconnectWait;
    static const State SystemResumeRereading;

    static const State SuspendIdle;
    static const State SuspendReading;
    static const State SuspendAcking;
    static const State SuspendRecovery;
    static const State SuspendReporting;
    static const State D3Idle;
    static const State D3PoweredOff;
    static const State D3PoweringUp;
    static const State D3Debouncing;
    static const State D3DebounceFlush;
    static const State D3FlushThenReturn;
    static const State D3ResumeAckWait;
    static const State D3ReconnectWait;
    static const State D3ReconnectFlush;

    static const State WakeArming;
    static const State Returning;

private:
    /* Engine hooks */
    static const BOOLEAN SmCompletionsFirst = FALSE;

    VOID SmReference();
    VOID SmDereference();
    VOID SmQueuePassive();
    VOID SmStateChanged();

    VOID
    SmOnDequeue(
        _In_ PortEvent Event);

    /* Frame 0 handlers */
    SM_RESULT
    OnOff(
        _In_ PortEvent Event);
    SM_RESULT
    OnOffEmpty(
        _In_ PortEvent Event);
    SM_RESULT
    OnOffAttached(
        _In_ PortEvent Event);
    SM_RESULT
    OnOffResetNeeded(
        _In_ PortEvent Event);
    SM_RESULT
    OnOffSuperSpeedDisabled(
        _In_ PortEvent Event);
    SM_RESULT
    OnOffSuspended(
        _In_ PortEvent Event);
    SM_RESULT
    OnCleaningUp(
        _In_ PortEvent Event);
    SM_RESULT
    OnEmpty(
        _In_ PortEvent Event);
    SM_RESULT
    OnStartingEmpty(
        _In_ PortEvent Event);
    SM_RESULT
    OnDebouncing(
        _In_ PortEvent Event);
    SM_RESULT
    OnOldDeviceWait(
        _In_ PortEvent Event);
    SM_RESULT
    OnOverCurrentWait(
        _In_ PortEvent Event);
    SM_RESULT
    OnUserResetWait(
        _In_ PortEvent Event);
    SM_RESULT
    OnTimerFlush(
        _In_ PortEvent Event);
    SM_RESULT
    OnFlushBeforeOff(
        _In_ PortEvent Event);
    SM_RESULT
    OnAttached(
        _In_ PortEvent Event);
    SM_RESULT
    OnEnabled(
        _In_ PortEvent Event);
    SM_RESULT
    OnAttachedDisabled(
        _In_ PortEvent Event);
    SM_RESULT
    OnSuspended(
        _In_ PortEvent Event);
    SM_RESULT
    OnDisabledInSuspend(
        _In_ PortEvent Event);
    SM_RESULT
    OnResumeAckWait(
        _In_ PortEvent Event);
    SM_RESULT
    OnResetRequestWait(
        _In_ PortEvent Event);
    SM_RESULT
    OnStartingAttached(
        _In_ PortEvent Event);
    SM_RESULT
    OnResumingAttached(
        _In_ PortEvent Event);
    SM_RESULT
    OnResumingDisabled(
        _In_ PortEvent Event);
    SM_RESULT
    OnSystemResuming(
        _In_ PortEvent Event);
    SM_RESULT
    OnErrorRestarting(
        _In_ PortEvent Event);
    SM_RESULT
    OnResumeAckWaitStopped(
        _In_ PortEvent Event);
    SM_RESULT
    OnTimed(
        _In_ PortEvent Event);
    SM_RESULT
    OnResetWait(
        _In_ PortEvent Event);
    SM_RESULT
    OnResetEnabledWait(
        _In_ PortEvent Event);
    SM_RESULT
    OnResetFlush(
        _In_ PortEvent Event);
    SM_RESULT
    OnResumeWait(
        _In_ PortEvent Event);
    SM_RESULT
    OnResumeRecovery(
        _In_ PortEvent Event);
    SM_RESULT
    OnResumeFlush(
        _In_ PortEvent Event);
    SM_RESULT
    OnHubResetIssued(
        _In_ PortEvent Event);
    SM_RESULT
    OnHubResetWait(
        _In_ PortEvent Event);
    SM_RESULT
    OnHubResetTimerWait(
        _In_ PortEvent Event);
    SM_RESULT
    OnEmptyHubResetWait(
        _In_ PortEvent Event);
    SM_RESULT
    OnEmptyHubResetTimerWait(
        _In_ PortEvent Event);
    SM_RESULT
    OnAttachFailedWait(
        _In_ PortEvent Event);
    SM_RESULT
    OnSuperSpeedOffWait(
        _In_ PortEvent Event);
    SM_RESULT
    OnSsLinkDisabled(
        _In_ PortEvent Event);
    SM_RESULT
    OnPoweringPort(
        _In_ PortEvent Event);
    SM_RESULT
    OnQueryingOverCurrent(
        _In_ PortEvent Event);
    SM_RESULT
    OnSendingReset(
        _In_ PortEvent Event);
    SM_RESULT
    OnSendingResume(
        _In_ PortEvent Event);
    SM_RESULT
    OnSendingSuspend(
        _In_ PortEvent Event);
    SM_RESULT
    OnDisablingOnRequest(
        _In_ PortEvent Event);
    SM_RESULT
    OnDisablingForCycle(
        _In_ PortEvent Event);
    SM_RESULT
    OnDisablingBeforeConnect(
        _In_ PortEvent Event);
    SM_RESULT
    OnQuiescingForHubSuspend(
        _In_ PortEvent Event);
    SM_RESULT
    OnDisablingForCycleTimed(
        _In_ PortEvent Event);
    SM_RESULT
    OnDisablingOnSuspendTimed(
        _In_ PortEvent Event);
    SM_RESULT
    OnDisablingOnResetTimeout(
        _In_ PortEvent Event);
    SM_RESULT
    OnReadingForResume(
        _In_ PortEvent Event);
    SM_RESULT
    OnReadingAfterResumeTimeout(
        _In_ PortEvent Event);
    SM_RESULT
    OnSettingLpmTimeout(
        _In_ PortEvent Event);
    SM_RESULT
    OnDisablingSuperSpeed(
        _In_ PortEvent Event);
    SM_RESULT
    OnDisablingSuperSpeedOnRequest(
        _In_ PortEvent Event);
    SM_RESULT
    OnEnablingSuperSpeedOnStop(
        _In_ PortEvent Event);
    SM_RESULT
    OnEnablingSuperSpeedOnTimer(
        _In_ PortEvent Event);
    SM_RESULT
    OnResetTimeoutStatus(
        _In_ PortEvent Event);
    SM_RESULT
    OnArmingWake(
        _In_ PortEvent Event);

    /* Frame 0 entries */
    SM_RESULT EnterOffEmpty();
    SM_RESULT EnterOffSuspended();
    SM_RESULT EnterCleaningUp();
    SM_RESULT EnterDisconnected();
    SM_RESULT EnterWithPortChange();
    SM_RESULT EnterStartingEmpty();
    SM_RESULT EnterDebouncing();
    SM_RESULT EnterNotifyingOverCurrent();
    SM_RESULT EnterSuspended();
    SM_RESULT EnterSuspendedD3Cold();
    SM_RESULT EnterStartingAttached();
    SM_RESULT EnterResumingAttached();
    SM_RESULT EnterSystemResuming();
    SM_RESULT EnterErrorRestarting();
    SM_RESULT EnterResetWait();
    SM_RESULT EnterResumeWait();
    SM_RESULT EnterHubResetIssued();
    SM_RESULT EnterDraining();
    SM_RESULT EnterDrainingEmpty();
    SM_RESULT EnterPoweringPort();
    SM_RESULT EnterSendingReset();
    SM_RESULT EnterSendingWarmReset();
    SM_RESULT EnterSendingResume();
    SM_RESULT EnterSendingSuspend();
    SM_RESULT EnterDisablingOnRequest();
    SM_RESULT EnterDisablingPort();
    SM_RESULT EnterQuiescingForHubSuspend();
    SM_RESULT EnterReadingStatus();
    SM_RESULT EnterSettingLpmTimeout();
    SM_RESULT EnterDisablingSuperSpeed();
    SM_RESULT EnterDisablingSuperSpeedOnRequest();
    SM_RESULT EnterEnablingSuperSpeed();
    SM_RESULT EnterResetTimeoutStatus();
    SM_RESULT EnterArmingWake();

    /* Called frame handlers and entries, portsm_frames.cpp */
    SM_RESULT
    OnChangeIdle20(
        _In_ PortEvent Event);
    SM_RESULT
    OnChangeReading20(
        _In_ PortEvent Event);
    SM_RESULT
    OnChangeAcking20(
        _In_ PortEvent Event);
    SM_RESULT
    OnChangeParked20(
        _In_ PortEvent Event);
    SM_RESULT EnterChangeReporting20();
    SM_RESULT EnterChangeParked20();

    SM_RESULT
    OnChangeIdle30(
        _In_ PortEvent Event);
    SM_RESULT
    OnChangeChecking30(
        _In_ PortEvent Event);
    SM_RESULT
    OnResetChangeIdle(
        _In_ PortEvent Event);
    SM_RESULT
    OnResetChangeChecking(
        _In_ PortEvent Event);
    SM_RESULT
    OnResetPollWait(
        _In_ PortEvent Event);
    SM_RESULT
    OnResetPollChecking(
        _In_ PortEvent Event);
    SM_RESULT
    OnResumeChangeIdle(
        _In_ PortEvent Event);
    SM_RESULT
    OnResumeChangeChecking(
        _In_ PortEvent Event);
    SM_RESULT EnterPlainStatusRead();
    SM_RESULT EnterResetPollWait();
    SM_RESULT EnterResetPollChecking();

    SM_RESULT
    OnStatusReading(
        _In_ PortEvent Event);
    SM_RESULT
    OnStatusAcking(
        _In_ PortEvent Event);
    SM_RESULT EnterStatusReading();
    SM_RESULT EnterStatusReporting();
    SM_RESULT EnterStatusFailing();

    SM_RESULT
    OnDrainIdle(
        _In_ PortEvent Event);
    SM_RESULT
    OnDrainEmptyIdle(
        _In_ PortEvent Event);
    SM_RESULT
    OnDrainReading(
        _In_ PortEvent Event);
    SM_RESULT
    OnDrainAcking(
        _In_ PortEvent Event);
    SM_RESULT
    OnDrainChecking(
        _In_ PortEvent Event);
    SM_RESULT EnterDrainRearming();

    SM_RESULT
    OnPowerOnSending(
        _In_ PortEvent Event);
    SM_RESULT
    OnPowerOnSettling(
        _In_ PortEvent Event);
    SM_RESULT
    OnStartReading20(
        _In_ PortEvent Event);
    SM_RESULT
    OnStartReadingFirst(
        _In_ PortEvent Event);
    SM_RESULT
    OnStartReading30(
        _In_ PortEvent Event);
    SM_RESULT
    OnPowerUpParked(
        _In_ PortEvent Event);
    SM_RESULT EnterPowerOnSettling();
    SM_RESULT EnterPowerUpResetHub();
    SM_RESULT EnterPowerUpRearming();

    SM_RESULT
    OnResumePowering(
        _In_ PortEvent Event);
    SM_RESULT
    OnResumeSettling(
        _In_ PortEvent Event);
    SM_RESULT
    OnResumeReading(
        _In_ PortEvent Event);
    SM_RESULT
    OnResumeAckingConnect(
        _In_ PortEvent Event);
    SM_RESULT
    OnReconnectWait(
        _In_ PortEvent Event);
    SM_RESULT
    OnReconnectReading(
        _In_ PortEvent Event);
    SM_RESULT
    OnReconnectAcking(
        _In_ PortEvent Event);
    SM_RESULT
    OnReconnectFlush(
        _In_ PortEvent Event);
    SM_RESULT
    OnResumeRereading(
        _In_ PortEvent Event);
    SM_RESULT
    OnSystemResumeReading(
        _In_ PortEvent Event);
    SM_RESULT
    OnSystemReconnectWait(
        _In_ PortEvent Event);
    SM_RESULT
    OnSystemResumeRereading(
        _In_ PortEvent Event);
    SM_RESULT EnterAckingChange();
    SM_RESULT EnterReconnectRearming();
    SM_RESULT EnterStartResumeStatusRead();

    SM_RESULT
    OnSuspendIdle(
        _In_ PortEvent Event);
    SM_RESULT
    OnSuspendReading(
        _In_ PortEvent Event);
    SM_RESULT
    OnSuspendAcking(
        _In_ PortEvent Event);
    SM_RESULT
    OnSuspendRecovery(
        _In_ PortEvent Event);
    SM_RESULT EnterSuspendRecovery();
    SM_RESULT EnterSuspendReporting();
    SM_RESULT
    OnD3Idle(
        _In_ PortEvent Event);
    SM_RESULT
    OnD3PoweredOff(
        _In_ PortEvent Event);
    SM_RESULT
    OnD3PoweringUp(
        _In_ PortEvent Event);
    SM_RESULT
    OnD3Debouncing(
        _In_ PortEvent Event);
    SM_RESULT
    OnD3DebounceFlush(
        _In_ PortEvent Event);
    SM_RESULT
    OnD3FlushThenReturn(
        _In_ PortEvent Event);
    SM_RESULT
    OnD3ResumeAckWait(
        _In_ PortEvent Event);
    SM_RESULT
    OnD3ReconnectWait(
        _In_ PortEvent Event);
    SM_RESULT
    OnD3ReconnectFlush(
        _In_ PortEvent Event);
    SM_RESULT EnterD3PoweringUp();

    SM_RESULT
    OnWakeArming(
        _In_ PortEvent Event);
    SM_RESULT EnterWakeArming();
    SM_RESULT EnterReturning();

    /* Shared code for the previous decision and one shot states */
    SM_RESULT
    PowerDown(
        _In_ const State* Target);
    SM_RESULT
    PowerUp(
        _In_ const State* Target);
    SM_RESULT
    ResetHubThen(
        _In_ const State* Target);
    SM_RESULT CheckConnection();
    SM_RESULT StartOverCurrentWait();
    SM_RESULT
    DetachThen(
        _In_ PortEvent Change);
    SM_RESULT
    DetachTimed(
        _In_ PortEvent Change);
    SM_RESULT
    StopTimerThen(
        _In_ TIMER_FOLLOW_UP FollowUp);
    SM_RESULT
    FollowTimer(
        _In_ TIMER_FOLLOW_UP FollowUp);
    SM_RESULT
    HandleConnectedFault(
        _In_ const State* Stay);
    SM_RESULT
    TimedError(
        _In_ const State* Stay,
        _In_ BOOLEAN DuringReset);
    SM_RESULT TimedHubReset();
    SM_RESULT ReleaseAndStopTimer();
    SM_RESULT
    ReattachOrCreate(
        _In_ OLD_DEVICE_KIND Kind);
    SM_RESULT
    CheckOldDevice(
        _In_ OLD_DEVICE_KIND Kind);
    SM_RESULT
    DeviceArrived(
        _In_ OLD_DEVICE_KIND Kind);
    SM_RESULT
    TakeInterruptForReset(
        _In_ const State* Start,
        _In_ const State* Refused);
    SM_RESULT ResumeSucceeded();
    SM_RESULT FailToHubReset();
    SM_RESULT ResetSucceeded();
    SM_RESULT
    CallStatusRead(
        _In_ READ_KIND Kind);
    SM_RESULT
    CheckPending20(
        _In_ const State* Acking,
        _In_ const State* Quiet);
    SM_RESULT
    ParkChange20(
        _In_ BOOLEAN ResetHub);
    SM_RESULT ResumeReadDone();
    SM_RESULT ResumeConnectCheck();
    SM_RESULT ResumeStateDisabled();
    SM_RESULT ReconnectCheck();
    SM_RESULT ReconnectStatusFailed();
    SM_RESULT
    D3ConnectCheck(
        _In_ BOOLEAN AfterResume);
    SM_RESULT
    D3StopReconnectTimer(
        _In_ D3_PENDING Pending);
    SM_RESULT
    D3PendingDone(
        _In_ D3_PENDING Pending);
    SM_RESULT D3StopDebounceTimer();
    SM_RESULT SystemResumeFailed();
    SM_RESULT
    ReportSuspendChange(
        _In_ PortEvent Report);
    SM_RESULT
    ReturnFromFrame(
        _In_ PortEvent Event);
    SM_RESULT CycleDetach();
    SM_RESULT ResetTransferFailed();
    SM_RESULT StatusFailed();
    SM_RESULT StatusQuiet();
    SM_RESULT DrainDone();
    SM_RESULT FailDeviceRequest();
    BOOLEAN
    AllowsHubSuspend(
        _In_ const State* Target) const;
    VOID
    UpdateHubPower(
        _In_ BOOLEAN Idle);
    VOID KeepHubAwake();

public:
    /* Actions, implemented by the port object */
    VOID ReferencePort();
    VOID DereferencePort();
    VOID QueuePassiveWork();
    VOID ForgetDetachedDevice();
    BOOLEAN HubPowerTake();
    VOID HubPowerDrop();

    VOID
    TellDevice(
        _In_ PSM_DEVICE_NOTICE Notice);
    VOID
    StartTimer(
        _In_ PSM_TIMER Timer);
    BOOLEAN StopTimer();

    VOID ClearPortStatus();
    VOID TakePortPowerReference();
    VOID DropPortPowerReference();
    VOID DropHubResetReference();
    BOOLEAN TakeInterruptReference();
    VOID DropInterruptReference();
    VOID RequestHubReset();
    VOID ResumeInterruptTransfer();
    VOID ResetChangeAccumulator();
    VOID FixPortStateAfterPowerUp();

    VOID DisconnectDevice();
    VOID ReattachBootDevice();
    VOID NotifyBootDeviceRemoval();
    VOID MarkUsb2Device();
    VOID MarkUsb3Device();
    VOID ForceResetOnEnumeration();
    VOID RecordPortDisabled();
    VOID RecordPortSuspended();
    VOID CancelUserOverCurrentReset();
    VOID NotifyUserOfOverCurrent();
    VOID LogLinkStateError();
    VOID MarkOverCurrentCause();

    VOID SendGetStatus();
    VOID SendAckChange();
    VOID SendReset();
    VOID SendWarmReset();
    VOID SendDisable();
    VOID SendResume();
    VOID SendSuspend();
    VOID SendPortPower();
    VOID SendU1Timeout();
    VOID SendU2Timeout();
    VOID SendLinkDisable();
    VOID SendLinkRxDetect();
    VOID
    SendRemoteWake(
        _In_ BOOLEAN Enable);

    /* Queries, implemented by the port object */
    BOOLEAN DeviceConnected();
    BOOLEAN DevicePresent();
    BOOLEAN OverCurrentActive();
    BOOLEAN OverCurrentPersists();
    BOOLEAN CreateDevice();
    BOOLEAN ConnectDevice();
    BOOLEAN BootDeviceReturning();
    BOOLEAN OldDevicePresent();
    BOOLEAN ConnectChangedOnResume();
    BOOLEAN D3ColdEnabled();
    BOOLEAN NeedsDebounce();
    BOOLEAN LinkInU0();
    BOOLEAN SuperSpeedBlocked();
    BOOLEAN PortPowered();
    BOOLEAN PollResetCompletion();
    BOOLEAN HubArmedForWake();
    PSM_PENDING PendingChange();
    PortEvent NextChange();
    PortEvent LostChange();
    PortEvent ErrorResponse();
    PortEvent ErrorResponseDuringReset();
    VOID
    ReadPortBits(
        _Out_ PPSM_PORT_BITS Bits);

private:
    HubPort* m_Port;
    BOOLEAN m_Usb3;
    BOOLEAN m_HubPowerHeld;
    TIMER_FOLLOW_UP m_AfterTimer;
    OLD_DEVICE_KIND m_OldDevice;
    READ_KIND m_ReadKind;
    D3_PENDING m_D3Pending;
    PortEvent m_SuspendReport;
    PortEvent m_D3FlushReturn;
    PortEvent m_ReturnEvent;
    BOOLEAN m_ReadFresh;
    BOOLEAN m_WarmResume;
    BOOLEAN m_AttachForcedReset;
    BOOLEAN m_LpmU2;
    BOOLEAN m_ArmForEmpty;
    BOOLEAN m_ParkResetsHub;
    BOOLEAN m_D3AfterResume;
    BOOLEAN m_D3PowerUpWarm;
    BOOLEAN m_ReconnectFlushOnStop;
    BOOLEAN m_DrainReturnsEmpty;
};
