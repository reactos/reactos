/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Hub state machine
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include <drivers/usb3/smengine.h>

class HubFdo;

enum class HubEvent : UCHAR
{
    /* PnP and power requests, wait until the current state takes requests */
    DeviceAdded,
    PrepareHardware,
    ReleaseHardware,
    DeviceCleanup,
    D0Entry,
    D0EntryFromSx,
    D0Exit,
    D0ExitFinal,
    PortStatusRequest,
    ResetRequested,

    /* Completions of work the machine started */
    TimerFired,
    ConfigurationSet,
    ConfigurationFailed,
    IoctlDone,
    IoctlFailed,
    InterruptDone,
    InterruptFailed,
    ResetDone,
    ResetFailed,
    ResetFailedOnRemoval,

    /* From the port and device machines */
    PortsAcquired,
    PortsReleased,
    PortsResetReleased,
    PortsInterruptReleased,
    PortsEnableInterrupt,
    DevicesAcquired,
    DevicesReleased,
    DevicesStoppedAfterSuspend,

    /* Control transfer and pipe reset completions, taken even in critical only states */
    TransferDone,
    TransferFailed,
    PipeResetDone,
    PipeResetFailed,

    /* Handed to the caller by SmReturn, never posted */
    CallSucceeded,
    CallFailed,

    Count
};

enum class HubKind : UCHAR
{
    Root,
    Usb20,
    Usb30
};

/** Answer of the hub change bit queries */
enum class HubCheck : UCHAR
{
    No,
    Yes,
    Error
};

/** Hub level change reported by the hub status */
enum class HubChange : UCHAR
{
    None,
    PowerGood,
    PowerLost,
    OverCurrent,
    OverCurrentCleared,
    Error
};

class HubMachine : public SmMachine<HubMachine, HubEvent>
{
    friend class SmMachine<HubMachine, HubEvent>;

public:
    VOID
    Initialize(
        _In_ HubFdo* Hub,
        _In_ HubKind Kind);

    HubFdo*
    Hub() const
    {
        return m_Hub;
    }

    static const SM_EVENT_INFO EventInfo[];

private:
    /** Why the hub is being taken down */
    enum class Teardown : UCHAR
    {
        Recover,
        Fatal,
        Stop,
        StopAfterReset,
        Suspend,
        ResumeFailed
    };

    /** Why the port and device references are being taken */
    enum class PowerUp : UCHAR
    {
        Start,
        PostReset,
        Resume,
        ResumeInS0,
        ResumeWithReset,
        ResumeResetFailed
    };

    /** Which part of the router DROM the mailbox read is fetching */
    enum class DromPhase : UCHAR
    {
        Header,
        Full
    };

    /** What a hub status read is for */
    enum class StatusRead : UCHAR
    {
        Change,
        LostChanges,
        OverCurrent
    };

    static const BOOLEAN SmCompletionsFirst = FALSE;

    /* States */
    static const State AwaitingAddDevice;
    static const State Unconfigured;
    static const State QueryingParentInfo;
    static const State Configuring;
    static const State Reconfiguring;
    static const State DromRead;
    static const State DromReadingFirstStatus;
    static const State DromWritingMetadata;
    static const State DromSendingCommand;
    static const State DromReadingStatus;
    static const State DromReadingWord;
    static const State QueryingErrata;
    static const State AwaitingD0;
    static const State Down;
    static const State Stopped;
    static const State AwaitingReleaseHardware;
    static const State AwaitingReleaseAfterFailure;
    static const State AwaitingStop;
    static const State SurpriseRemoved;
    static const State Deleted;
    static const State PortsAcquiring;
    static const State DevicesAcquiring;
    static const State Listening;
    static const State NotListening;
    static const State ReadingPortStatus;
    static const State ReadingHubStatus;
    static const State AckingHubChange;
    static const State AwaitingOverCurrentTimer;
    static const State ResettingPipe;
    static const State DrainingInterruptRefs;
    static const State CancelingInterrupt;
    static const State AwaitingInterruptRefs;
    static const State AwaitingInterruptEnable;
    static const State DevicesReleasing;
    static const State PortsReleasing;
    static const State PortsResetting;
    static const State CheckingRecoveryLimit;
    static const State ReportingFailure;
    static const State Halted;
    static const State HubResetting;
    static const State LoggingReset;
    static const State AwaitingResetRetry;
    static const State ReconfiguringAfterReset;
    static const State ResettingOnResume;
    static const State ReconfiguringAfterResume;
    static const State CheckingPowerLoss;
    static const State Asleep;
    static const State Suspended;
    static const State RecycleSuspended;
    static const State SuspendedResetPending;
    static const State AwaitingStopAfterSuspend;

    /* Called: hub descriptor and configuration for external hubs */
    static const State ReadingHubDescriptor;
    static const State AwaitingDescriptorRetry;
    static const State ReadingStandardStatus;
    static const State ReadingConfigDescriptor;
    static const State UpdatingController;
    static const State SettingConfiguration;
    static const State FinishingConfiguration;

    /* Called: root hub information from the controller */
    static const State ReadingRootHubInfo;
    static const State ReadingRootHub20Ports;
    static const State ReadingRootHub30Ports;

    /* Called: hub depth for USB 3 hubs */
    static const State ProgrammingHubDepth;

    static const HubEvent ResetDiscards[];

    /* Engine hooks */
    VOID
    SmReference();

    VOID
    SmDereference();

    VOID
    SmQueuePassive();

    /* Handlers */
    SM_RESULT
    OnAwaitingAddDevice(
        _In_ HubEvent Event);
    SM_RESULT
    OnUnconfigured(
        _In_ HubEvent Event);
    SM_RESULT
    OnQueryingParentInfo(
        _In_ HubEvent Event);
    SM_RESULT
    OnConfiguring(
        _In_ HubEvent Event);
    SM_RESULT
    OnReconfiguring(
        _In_ HubEvent Event);
    SM_RESULT
    OnDromRead(
        _In_ HubEvent Event);
    SM_RESULT
    OnDromReadingFirstStatus(
        _In_ HubEvent Event);
    SM_RESULT
    OnDromWritingMetadata(
        _In_ HubEvent Event);
    SM_RESULT
    OnDromSendingCommand(
        _In_ HubEvent Event);
    SM_RESULT
    OnDromReadingStatus(
        _In_ HubEvent Event);
    SM_RESULT
    OnDromReadingWord(
        _In_ HubEvent Event);
    SM_RESULT
    OnAwaitingD0(
        _In_ HubEvent Event);
    SM_RESULT
    OnDown(
        _In_ HubEvent Event);
    SM_RESULT
    OnStopped(
        _In_ HubEvent Event);
    SM_RESULT
    OnAwaitingReleaseHardware(
        _In_ HubEvent Event);
    SM_RESULT
    OnAwaitingReleaseAfterFailure(
        _In_ HubEvent Event);
    SM_RESULT
    OnAwaitingStop(
        _In_ HubEvent Event);
    SM_RESULT
    OnSurpriseRemoved(
        _In_ HubEvent Event);
    SM_RESULT
    OnPortsAcquiring(
        _In_ HubEvent Event);
    SM_RESULT
    OnDevicesAcquiring(
        _In_ HubEvent Event);
    SM_RESULT
    OnListening(
        _In_ HubEvent Event);
    SM_RESULT
    OnNotListening(
        _In_ HubEvent Event);
    SM_RESULT
    OnReadingPortStatus(
        _In_ HubEvent Event);
    SM_RESULT
    OnReadingHubStatus(
        _In_ HubEvent Event);
    SM_RESULT
    OnAckingHubChange(
        _In_ HubEvent Event);
    SM_RESULT
    OnAwaitingOverCurrentTimer(
        _In_ HubEvent Event);
    SM_RESULT
    OnResettingPipe(
        _In_ HubEvent Event);
    SM_RESULT
    OnDrainingInterruptRefs(
        _In_ HubEvent Event);
    SM_RESULT
    OnCancelingInterrupt(
        _In_ HubEvent Event);
    SM_RESULT
    OnAwaitingInterruptRefs(
        _In_ HubEvent Event);
    SM_RESULT
    OnAwaitingInterruptEnable(
        _In_ HubEvent Event);
    SM_RESULT
    OnDevicesReleasing(
        _In_ HubEvent Event);
    SM_RESULT
    OnPortsReleasing(
        _In_ HubEvent Event);
    SM_RESULT
    OnPortsResetting(
        _In_ HubEvent Event);
    SM_RESULT
    OnHubResetting(
        _In_ HubEvent Event);
    SM_RESULT
    OnAwaitingResetRetry(
        _In_ HubEvent Event);
    SM_RESULT
    OnReconfiguringAfterReset(
        _In_ HubEvent Event);
    SM_RESULT
    OnResettingOnResume(
        _In_ HubEvent Event);
    SM_RESULT
    OnReconfiguringAfterResume(
        _In_ HubEvent Event);
    SM_RESULT
    OnCheckingPowerLoss(
        _In_ HubEvent Event);
    SM_RESULT
    OnAsleep(
        _In_ HubEvent Event);
    SM_RESULT
    OnSuspended(
        _In_ HubEvent Event);
    SM_RESULT
    OnRecycleSuspended(
        _In_ HubEvent Event);
    SM_RESULT
    OnSuspendedResetPending(
        _In_ HubEvent Event);
    SM_RESULT
    OnAwaitingStopAfterSuspend(
        _In_ HubEvent Event);
    SM_RESULT
    OnReadingHubDescriptor(
        _In_ HubEvent Event);
    SM_RESULT
    OnAwaitingDescriptorRetry(
        _In_ HubEvent Event);
    SM_RESULT
    OnReadingStandardStatus(
        _In_ HubEvent Event);
    SM_RESULT
    OnReadingConfigDescriptor(
        _In_ HubEvent Event);
    SM_RESULT
    OnUpdatingController(
        _In_ HubEvent Event);
    SM_RESULT
    OnSettingConfiguration(
        _In_ HubEvent Event);
    SM_RESULT
    OnFinishingConfiguration(
        _In_ HubEvent Event);
    SM_RESULT
    OnReadingRootHubInfo(
        _In_ HubEvent Event);
    SM_RESULT
    OnReadingRootHub20Ports(
        _In_ HubEvent Event);
    SM_RESULT
    OnReadingRootHub30Ports(
        _In_ HubEvent Event);
    SM_RESULT
    OnProgrammingHubDepth(
        _In_ HubEvent Event);

    /* Entry actions */
    SM_RESULT EnterQueryingParentInfo();
    SM_RESULT EnterConfiguring();
    SM_RESULT EnterReconfiguring();
    SM_RESULT EnterDromReadingFirstStatus();
    SM_RESULT EnterDromWritingMetadata();
    SM_RESULT EnterDromSendingCommand();
    SM_RESULT EnterDromReadingStatus();
    SM_RESULT EnterDromReadingWord();
    SM_RESULT EnterQueryingErrata();
    SM_RESULT EnterSignalPnp();
    SM_RESULT EnterCheckingRecoveryLimit();
    SM_RESULT EnterReportingFailure();
    SM_RESULT EnterHalted();
    SM_RESULT EnterHubResetting();
    SM_RESULT EnterLoggingReset();
    SM_RESULT EnterReconfiguringAfterReset();
    SM_RESULT EnterReconfiguringAfterResume();
    SM_RESULT EnterSuspendedResetPending();
    SM_RESULT EnterReadingHubDescriptor();
    SM_RESULT EnterAwaitingDescriptorRetry();
    SM_RESULT EnterReadingStandardStatus();
    SM_RESULT EnterReadingConfigDescriptor();
    SM_RESULT EnterUpdatingController();
    SM_RESULT EnterSettingConfiguration();
    SM_RESULT EnterFinishingConfiguration();
    SM_RESULT EnterReadingRootHubInfo();
    SM_RESULT EnterReadingRootHub20Ports();
    SM_RESULT EnterReadingRootHub30Ports();
    SM_RESULT EnterProgrammingHubDepth();

    /* Helpers */
    SM_RESULT StartHubConfiguration();
    SM_RESULT
    FailStart(
        _In_ BOOLEAN FirstStart);
    SM_RESULT
    StartDromOrSkip(
        _In_ const State* Next);
    SM_RESULT CheckDromStatus();
    SM_RESULT StartDromWords();
    SM_RESULT StoreDromWordAndContinue();
    SM_RESULT FinishDrom();
    SM_RESULT
    StartPowerUp(
        _In_ PowerUp Kind);
    SM_RESULT
    ReadStatus(
        _In_ StatusRead Purpose);
    SM_RESULT
    HandleHubChange(
        _In_ HubChange Change);
    SM_RESULT HandleHubStatusFault();
    SM_RESULT OverCurrentError();
    SM_RESULT
    SendInterrupt(
        _In_ BOOLEAN ClearPipeResets);
    SM_RESULT
    QuiesceWithInterrupt(
        _In_ Teardown Reason);
    SM_RESULT
    QuiesceWithoutInterrupt(
        _In_ Teardown Reason);
    SM_RESULT AfterInterruptQuiet();
    SM_RESULT
    ReadPortStatus(
        _In_ const State* ReturnTo);
    SM_RESULT ResumeResetFailed();
    SM_RESULT CheckPowerLoss();
    SM_RESULT
    CallHubDepthOr(
        _In_ PowerUp Kind);

    /* Actions, implemented by the hub object */
    VOID ReferenceHub();
    VOID DereferenceHub();
    VOID QueuePassiveWork();
    VOID AcquirePowerReference();
    VOID ReleasePowerReference();
    VOID SignalPnpEvent();
    VOID SignalPnpFailure();
    VOID ReportFailureToPnp();
    VOID BugcheckBootHub();
    VOID LogResetRecovery();
    VOID LogResetOnResume();
    VOID QueryErrataFlags();
    VOID CancelStatusChangeRead();
    VOID StartStatusChangeRead();
    VOID GetHubStatus();
    VOID GetPortStatus();
    VOID ReplyPortStatus();
    VOID
    FailPortStatus(
        _In_ BOOLEAN HardwareError);
    VOID AckHubChange();
    VOID GetHubDescriptor();
    VOID GetStandardStatus();
    VOID
    GetConfigDescriptor(
        _In_ BOOLEAN FullLength);
    VOID SetHubDepth();
    VOID InitializeResetCount();
    VOID ClearPipeResetCount();
    VOID InitializeDescriptorRetries();
    VOID ArmResetRetryTimer();
    VOID ArmOverCurrentTimer();
    VOID StartDescriptorRetryTimer();
    VOID QueueSurpriseRemovalToPorts();
    VOID QueueStopToPorts();
    VOID QueueResetToPorts();
    VOID QueueStartToPorts();
    VOID QueueSuspendToPorts();
    VOID QueueResumeToPorts();
    VOID QueueResumeInS0ToPorts();
    VOID QueueResumeWithResetToPorts();
    VOID QueueFakeStatusChangeToPorts();
    VOID QueueStatusChangeToPorts();
    VOID AllowHubReset();
    VOID QueueStopToDevices();
    VOID QueueStopAfterSuspendToDevices();
    VOID QueueSuspendToDevices();
    VOID QueueStartToDevices();
    VOID QueueResumeToDevices();
    VOID QueueResumeInS0ToDevices();
    VOID QueueResumeWithResetToDevices();
    VOID ResetHub();
    VOID ResetStatusChangePipe();
    VOID SelectHubConfiguration();
    VOID GetParentInfo();
    VOID QueryControllerInfo();
    VOID GetRootHubInfo();
    VOID GetRootHub20Ports();
    VOID GetRootHub30Ports();
    VOID UpdateControllerHubInfo();
    VOID
    ReadDromRegister(
        _In_ USHORT Register);
    VOID
    WriteDromRegister(
        _In_ USHORT Register,
        _In_ ULONG Value);
    VOID UseDromHeaderBuffer();
    VOID ReleaseDromBuffer();
    VOID
    StoreDromWord(
        _In_ ULONG Index);
    VOID ParseDromIdentity();

    /* Queries, implemented by the hub object */
    BOOLEAN IsDepthZero();
    BOOLEAN IsInBootPath();
    BOOLEAN WasResetByParent();
    BOOLEAN RecoveryLimitReached();
    BOOLEAN ResetLimitReached();
    BOOLEAN PipeResetLimitReached();
    BOOLEAN DescriptorRetriesExhausted();
    BOOLEAN PortInterruptRefsReleased();
    BOOLEAN OverCurrentCleared();
    BOOLEAN CreateChildPorts();
    BOOLEAN IsErrorFatal();
    HubCheck CheckHubChangeBit();
    HubCheck SelectHubChange();
    HubChange PendingHubChange();
    HubChange LostHubChange();
    BOOLEAN ConfigDescriptorTruncated();
    BOOLEAN ParseHubDescriptor();
    BOOLEAN ParsePowerStatus();
    BOOLEAN CacheConfigDescriptor();
    BOOLEAN IsDromReadWanted();
    ULONG MailboxValue();
    ULONG DromDataLength();
    BOOLEAN
    AllocateDromBuffer(
        _In_ ULONG Length);

    HubFdo* m_Hub;
    HubKind m_Kind;
    Teardown m_Teardown;
    PowerUp m_PowerUp;
    StatusRead m_StatusRead;
    const State* m_PortStatusReturn;
    BOOLEAN m_ReleaseToUnconfigured;
    BOOLEAN m_FullConfigRead;
    ULONGLONG m_DromPollStart;
    ULONG m_DromIndex;
    ULONG m_DromWords;
    DromPhase m_DromPhase;
    const State* m_DromNext;
};
