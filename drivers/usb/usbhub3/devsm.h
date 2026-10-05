/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Device state machine, one per device attached to a hub port
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include <drivers/usb3/smengine.h>

class HubChild;

enum class DsmEvent : UCHAR
{
    /* Port machine reports */
    PortAttached,
    PortReattached,
    PortDetached,
    PortDisableDone,
    PortFault,
    PortResetDone,
    PortResetAbortedForSuspend,
    PortResetTimeout,
    PortResumeDone,
    PortResumeAbortedForSuspend,
    PortResumeTimeout,
    PortSuspendDone,
    PortNowDisabled,
    PortNowEnabled,
    PortNowEnabledOnReconnect,
    PortNowSuspended,
    PortTimeoutSet,

    /* Hub machine reports */
    HubStarted,
    HubStopping,
    HubStoppingAfterSuspend,
    HubSuspending,
    HubResumed,
    HubResumedInS0,
    HubResumedWithReset,

    /* Timer the machine started */
    TimerFired,

    /* Completions of transfers and controller requests the machine started */
    TransferDone,
    TransferFailed,
    TransferStalled,
    ControllerIoctlDone,
    ControllerIoctlFailed,
    ControllerExitLatencyTooLarge,
    ControllerRequestDone,

    /* PnP and power requests on the device PDO, wait for a state that takes requests */
    PdoPreStart,
    PdoPowerUp,
    PdoPowerDown,
    PdoPowerDownFinal,
    PdoPrepareHibernate,
    PdoInstallMsOsExt,
    PdoReportedMissing,
    PdoCleanup,

    /* Client driver and hub FDO requests, wait for a state that takes requests */
    ClientCyclePort,
    ClientResetDevice,
    ClientResetPipe,
    ClientSyncResetPipe,
    ClientClearStall,
    ClientSelectConfig,
    ClientUnconfigure,
    ClientSetInterface,
    ClientStreams,
    DeviceTextQuery,
    HubGetDescriptor,
    LpmSettingChanged,
    NoPingResponse,

    /* Results a sub machine hands back to the state that called it */
    Succeeded,
    Failed,
    FailedDeviceEnabled,
    DetachedDeviceEnabled,
    PortOffForHubSuspend,
    HubStoppedHoldingReference,
    ResumeDone,
    ResumedWithHub,
    SuspendedAfterHubResume,
    NeedsReenumeration,
    FailEnumeration,

    Count
};

/** Result of the reset type query before a SuperSpeed re-enumeration. */
typedef enum _DSM_RESET_KIND
{
    DsmResetNone,
    DsmResetHot,
    DsmResetWarm
} DSM_RESET_KIND;

/** What a link power feature query wants done with the feature. */
typedef enum _DSM_FEATURE_CHANGE
{
    DsmFeatureKeep,
    DsmFeatureEnable,
    DsmFeatureDisable
} DSM_FEATURE_CHANGE;

/** Outcome of adding the device to the hub's list of children. */
typedef enum _DSM_ADD_CHILD
{
    DsmAddChildDone,
    DsmAddChildDuplicate,
    DsmAddChildFailed
} DSM_ADD_CHILD;

/** Which timer the shared delay state starts. */
typedef enum _DSM_DELAY
{
    DsmDelayEnumRetry,
    DsmDelayPostReset,
    DsmDelayPostResetLong,
    DsmDelayPostResetSuperSpeed,
    DsmDelayPostAddress,
    DsmDelayDuplicate
} DSM_DELAY;

/** How a re-enumeration starts. */
typedef enum _DSM_REENUM
{
    DsmReenumConfigured,    /**< Purge the device tree, deconfigure unless programming was lost */
    DsmReenumUnconfigured,  /**< Purge the device tree, re-enable if programming was lost */
    DsmReenumRestart        /**< The controller forgot the device, enable it again */
} DSM_REENUM;

/* Device kind bits, kept up to date by the device code and used to pick flows */
#define DSM_KIND_USB10          0x00000001
#define DSM_KIND_USB1X          0x00000002
#define DSM_KIND_USB20          0x00000004
#define DSM_KIND_USB2X          0x00000008
#define DSM_KIND_USB3X          0x00000010
#define DSM_KIND_ANY_VERSION    0x000000FF
#define DSM_KIND_HIGH_SPEED     0x00000100
#define DSM_KIND_FULL_SPEED     0x00000200
#define DSM_KIND_LOW_SPEED      0x00000400
#define DSM_KIND_SUPER_SPEED    0x00000800
#define DSM_KIND_ANY_SPEED      0x0000FF00
#define DSM_KIND_PORT20         0x00010000
#define DSM_KIND_PORT30         0x00020000
#define DSM_KIND_ANY_PORT       0x00FF0000

class DeviceMachine : public SmMachine<DeviceMachine, DsmEvent>
{
    friend class SmMachine<DeviceMachine, DsmEvent>;

public:
    VOID
    Initialize(
        _In_ HubChild* Device);

    HubChild*
    Device() const
    {
        return m_Device;
    }

    static const SM_EVENT_INFO EventInfo[];

private:
    /** Where a shared wait goes once its event arrives. */
    typedef SM_RESULT (DeviceMachine::*DSM_NEXT)(_In_ DsmEvent Event);

    /** Code that runs once a shared step state is entered. */
    typedef SM_RESULT (DeviceMachine::*DSM_THEN)();

    /** Starts the request a shared request state waits for. */
    typedef VOID (DeviceMachine::*DSM_ACTION)();

    /* Engine hooks */
    static const BOOLEAN SmCompletionsFirst = FALSE;
    static const USHORT SmTimerEventId = static_cast<USHORT>(DsmEvent::TimerFired);

    VOID SmReference();
    VOID SmDereference();
    VOID SmQueuePassive();
    BOOLEAN SmCancelTimer();

    /* Shared helpers, devsm.cpp */
    BOOLEAN
    KindWithin(
        _In_ ULONG Allowed);

    SM_RESULT
    Wait(
        _In_ const State* Target,
        _In_ DSM_NEXT Next);

    SM_RESULT
    AtPassive(
        _In_ DSM_THEN Then);

    SM_RESULT
    EndWith(
        _In_ DsmEvent Result);

    SM_RESULT
    OnShared(
        _In_ DsmEvent Event);
    SM_RESULT EnterPassiveStep();
    SM_RESULT EnterEnding();

    /* Shared states, devsm.cpp */
    static const State PassiveStep;
    static const State Ending;

    /* Shared waits and steps, devsm.cpp */
    static const State DisablingPort;
    static const State AwaitingPort;
    static const State Delaying;
    static const State EnablingDevice;
    static const State ResettingPort;
    static const State NotifyingReset;
    static const State ReadingFirstDescriptor;
    static const State Starting;
    static const State Requesting;
    static const State RequestingCritical;
    static const State RequestingYielding;
    static const State WaitingForController;
    static const State RequestingCriticalPassive;
    static const State CancelingTransfer;
    static const State SuspendingPort;
    static const State ResumingPort;
    static const State IgnoringHub;
    static const State ForwardingStreams;
    static const State DisablingEndpoints;
    static const State AwaitingPortYielding;
    static const State IoctlAfterDetach;
    static const State TimerAfterDetach;
    static const State DisablingDevice;
    static const State SettingPdCharging;

    SM_RESULT EnterDisablingPort();
    SM_RESULT EnterDelaying();
    SM_RESULT
    Delay(
        _In_ DSM_DELAY Kind,
        _In_ DSM_NEXT Next);
    SM_RESULT EnterEnablingDevice();
    SM_RESULT EnterResettingPort();
    SM_RESULT EnterNotifyingReset();
    SM_RESULT EnterReadingFirstDescriptor();
    SM_RESULT
    CallWait(
        _In_ const State* Target,
        _In_ DSM_NEXT Next);
    SM_RESULT
    CallAtPassive(
        _In_ DSM_THEN Then);
    SM_RESULT
    DeleteThen(
        _In_ DSM_THEN After);
    SM_RESULT EndFailed();
    SM_RESULT EndDetached();
    SM_RESULT EndWithHubEvent();
    SM_RESULT
    EndOnHubOrDetach(
        _In_ DsmEvent Event);
    SM_RESULT
    EndOnResetForHub(
        _In_ DsmEvent Event);
    SM_RESULT
    EndAfterTimer(
        _In_ DsmEvent Event);
    SM_RESULT
    EndDetachedOnTimer(
        _In_ DsmEvent Event);
    SM_RESULT
    EndForHubOnTimer(
        _In_ DsmEvent Event);
    SM_RESULT EnterStarting();
    SM_RESULT
    CallStep(
        _In_ DSM_THEN Then);
    SM_RESULT EnterRequesting();
    SM_RESULT
    Request(
        _In_ const State* Target,
        _In_ DSM_ACTION Start,
        _In_ DSM_NEXT Next);
    SM_RESULT
    CallRequest(
        _In_ const State* Target,
        _In_ DSM_ACTION Start,
        _In_ DSM_NEXT Next);
    SM_RESULT EnterCancelingTransfer();
    SM_RESULT EnterSuspendingPort();
    SM_RESULT EnterResumingPort();
    SM_RESULT
    ForwardStreams(
        _In_ const State* ReturnTo);
    SM_RESULT EnterForwardingStreams();
    SM_RESULT EnterDisablingEndpoints();
    SM_RESULT
    OnForwardingStreams(
        _In_ DsmEvent Event);
    SM_RESULT
    CancelTransferAndEnd(
        _In_ DsmEvent Event);
    SM_RESULT
    EndOnTransferCanceled(
        _In_ DsmEvent Event);
    SM_RESULT EnterDisablingDevice();
    SM_RESULT EnterSettingPdCharging();
    SM_RESULT
    DisableAndDelete(
        _In_ DSM_THEN After);
    SM_RESULT
    DeleteOnceDisabled(
        _In_ DsmEvent Event);
    SM_RESULT DeleteDeviceThen();

    /* Attach and first enumeration, devsm_enum.cpp */
    static const State AwaitingAttach;
    static const State AwaitingDelete;
    static const State Enumerating;
    static const State ClaimingAddressZero;
    static const State CancelingClaimForHub;
    static const State AtAddressZero;
    static const State AddressingInEnum;
    static const State ReadingDescriptors;
    static const State LinkPowerInEnum;
    static const State UpdatingDevice;
    static const State ParkedForHubInEnum;

    SM_RESULT
    OnAwaitingAttach(
        _In_ DsmEvent Event);
    SM_RESULT EnterEnumerating();
    SM_RESULT
    OnEnumerating(
        _In_ DsmEvent Event);
    SM_RESULT EndAfterDetach();
    VOID DiscardHubEvents();
    SM_RESULT EnterClaimingAddressZero();
    SM_RESULT
    OnClaimingAddressZero(
        _In_ DsmEvent Event);
    SM_RESULT
    EnumClaimEndedOnDetach(
        _In_ DsmEvent Event);
    SM_RESULT EnterCancelingClaim();
    SM_RESULT
    OnCancelingClaimForHub(
        _In_ DsmEvent Event);
    SM_RESULT
    EnumPortOffHoldingAddressZero(
        _In_ DsmEvent Event);
    SM_RESULT
    EnumPortOffForHub(
        _In_ DsmEvent Event);
    SM_RESULT EnumParkForHub();
    SM_RESULT EnumRetryOrFail();
    SM_RESULT
    EnumRetryDelayEnded(
        _In_ DsmEvent Event);
    SM_RESULT EnumStopRetryOnDetach();
    SM_RESULT
    EnumTimerFlushedOnDetach(
        _In_ DsmEvent Event);
    SM_RESULT EnumStopRetryForHub();
    SM_RESULT
    EnumTimerFlushedForHub(
        _In_ DsmEvent Event);
    SM_RESULT
    EnumPortOffAfterFailure(
        _In_ DsmEvent Event);
    SM_RESULT EnterAtAddressZero();
    SM_RESULT
    OnAtAddressZero(
        _In_ DsmEvent Event);
    SM_RESULT EnumRetryOrFailHoldingAddressZero();
    SM_RESULT
    EnumRetryDelayEndedHoldingAddressZero(
        _In_ DsmEvent Event);
    SM_RESULT
    EnumPortOffAfterFailureHoldingAddressZero(
        _In_ DsmEvent Event);
    SM_RESULT EnterAddressing();
    SM_RESULT
    OnAddressingInEnum(
        _In_ DsmEvent Event);
    SM_RESULT EnumPortOffAfterDelete();
    SM_RESULT EnumReleaseAndEndDetached();
    SM_RESULT EnterReadingDescriptors();
    SM_RESULT
    OnReadingDescriptors(
        _In_ DsmEvent Event);
    SM_RESULT
    EnumPortOffWithDevice(
        _In_ DsmEvent Event);
    SM_RESULT EnterLinkPowerInEnum();
    SM_RESULT
    OnLinkPowerInEnum(
        _In_ DsmEvent Event);
    SM_RESULT EnterUpdatingDevice();
    SM_RESULT
    OnUpdatingDevice(
        _In_ DsmEvent Event);
    SM_RESULT
    EnumPdChargingSet(
        _In_ DsmEvent Event);
    SM_RESULT EnterParkedForHub();
    SM_RESULT
    OnParkedForHubInEnum(
        _In_ DsmEvent Event);

    /* Hub suspended sub machine, devsm_hub.cpp */
    static const State HubSuspended;
    static const State HubSuspendedPortOff;

    SM_RESULT
    OnHubSuspended(
        _In_ DsmEvent Event);
    SM_RESULT
    OnHubSuspendedPortOff(
        _In_ DsmEvent Event);
    SM_RESULT
    HubResumeForReenumeration(
        _In_ DsmEvent Event);
    SM_RESULT
    HubResumeWithPortSuspended(
        _In_ DsmEvent Event);
    SM_RESULT
    HubResumeWithPortEnabled(
        _In_ DsmEvent Event);
    SM_RESULT HubEndResumedOrReenumerate();
    SM_RESULT
    HubPortSynced(
        _In_ DsmEvent Event);
    SM_RESULT
    HubPortSyncedForReset(
        _In_ DsmEvent Event);
    SM_RESULT
    HubPortSyncedForSuspend(
        _In_ DsmEvent Event);
    SM_RESULT
    HubPortSuspendedAgain(
        _In_ DsmEvent Event);
    SM_RESULT
    HubPortSyncedForStop(
        _In_ DsmEvent Event);
    SM_RESULT
    HubPortOffForStop(
        _In_ DsmEvent Event);
    SM_RESULT
    HubPortResumedForReset(
        _In_ DsmEvent Event);
    SM_RESULT
    HubResumeEndedForStop(
        _In_ DsmEvent Event);
    SM_RESULT
    HubResumeEndedForSuspend(
        _In_ DsmEvent Event);

    /* Address zero and address assignment, devsm_zero.cpp */
    static const State ResettingSuperSpeed;
    static const State UpdatingDefaultEndpoint;
    static const State SettingAddress;

    SM_RESULT
    Zero20FirstResetDone(
        _In_ DsmEvent Event);
    SM_RESULT
    Zero20FirstResetEndedForHub(
        _In_ DsmEvent Event);
    SM_RESULT Zero20CreateDevice();
    SM_RESULT
    Zero20Enabled(
        _In_ DsmEvent Event);
    SM_RESULT
    Zero20FirstDelayEnded(
        _In_ DsmEvent Event);
    SM_RESULT
    Zero20StopDelay(
        _In_ DsmEvent Event);
    SM_RESULT
    Zero20TimerFlushedOnDetach(
        _In_ DsmEvent Event);
    SM_RESULT
    Zero20TimerFlushedForHub(
        _In_ DsmEvent Event);
    SM_RESULT
    Zero20FirstDescriptorRead(
        _In_ DsmEvent Event);
    SM_RESULT EnterUpdatingDefaultEndpoint();
    SM_RESULT
    Zero20EndpointUpdated(
        _In_ DsmEvent Event);
    SM_RESULT
    Zero20ResetNotified(
        _In_ DsmEvent Event);
    SM_RESULT
    Zero20SecondResetDone(
        _In_ DsmEvent Event);
    SM_RESULT
    Zero20SecondResetEndedForHub(
        _In_ DsmEvent Event);
    SM_RESULT
    Zero20SecondResetAwaitHub(
        _In_ DsmEvent Event);
    SM_RESULT
    Zero20PortOffForHub(
        _In_ DsmEvent Event);
    SM_RESULT
    Zero20SecondDelayEnded(
        _In_ DsmEvent Event);
    SM_RESULT Zero30CreateDevice();
    SM_RESULT EnterResettingSuperSpeed();
    SM_RESULT
    OnResettingSuperSpeed(
        _In_ DsmEvent Event);
    SM_RESULT
    Zero30Enabled(
        _In_ DsmEvent Event);
    SM_RESULT StartSuperSpeedReset();
    SM_RESULT
    SuperSpeedResetNotified(
        _In_ DsmEvent Event);
    SM_RESULT
    SuperSpeedResetDone(
        _In_ DsmEvent Event);
    SM_RESULT
    SuperSpeedDelayEnded(
        _In_ DsmEvent Event);
    SM_RESULT EnterSettingAddress();
    SM_RESULT
    OnSettingAddress(
        _In_ DsmEvent Event);
    SM_RESULT
    AddressDelayEnded(
        _In_ DsmEvent Event);

    /* Descriptors after addressing and SuperSpeed link power, devsm_desc.cpp */
    static const State CollectingDescriptors;
    static const State ReadingDeviceDescriptor;
    static const State ReadingConfigDescriptor;
    static const State DisablingSuperSpeed;
    static const State RemainingDescriptors;
    static const State ReadingBos;
    static const State ReadingMsOs;
    static const State ReadingLanguages;
    static const State ReadingProductName;
    static const State ReadingQualifier;

    SM_RESULT
    OnCollectingDescriptors(
        _In_ DsmEvent Event);
    SM_RESULT EnterReadingDeviceDescriptor();
    SM_RESULT
    OnReadingDeviceDescriptor(
        _In_ DsmEvent Event);
    SM_RESULT DescriptorsQueryRegistry();
    SM_RESULT
    DescriptorsAltEnumSent(
        _In_ DsmEvent Event);
    SM_RESULT EnterDisablingSuperSpeed();
    SM_RESULT EnterReadingConfigDescriptor();
    SM_RESULT
    OnReadingConfigDescriptor(
        _In_ DsmEvent Event);
    SM_RESULT EnterRemainingDescriptors();
    SM_RESULT
    OnRemainingDescriptors(
        _In_ DsmEvent Event);
    SM_RESULT EnterReadingBos();
    SM_RESULT
    OnReadingBos(
        _In_ DsmEvent Event);
    SM_RESULT BosStart();
    SM_RESULT
    BosHeaderRead(
        _In_ DsmEvent Event);
    SM_RESULT BosError();
    SM_RESULT
    BosRead(
        _In_ DsmEvent Event);
    SM_RESULT BosValidate();
    SM_RESULT
    BosFeaturesSent(
        _In_ DsmEvent Event);
    SM_RESULT BosBillboard();
    SM_RESULT
    BosBillboardRead(
        _In_ DsmEvent Event);
    SM_RESULT BosAltModes();
    SM_RESULT
    BosAltModeRead(
        _In_ DsmEvent Event);
    SM_RESULT BosAltEnum();
    SM_RESULT BosMsOs20();
    SM_RESULT
    BosMsOs20Read(
        _In_ DsmEvent Event);
    SM_RESULT BosMsOs20Validate();
    SM_RESULT
    BosAltEnumSent(
        _In_ DsmEvent Event);
    SM_RESULT
    AltDeviceDescriptorRead(
        _In_ DsmEvent Event);
    SM_RESULT AltQueryRegistry();
    SM_RESULT
    AltConfigHeaderRead(
        _In_ DsmEvent Event);
    SM_RESULT
    AltConfigRead(
        _In_ DsmEvent Event);
    SM_RESULT AltValidateConfig();
    SM_RESULT
    AltBosHeaderRead(
        _In_ DsmEvent Event);
    SM_RESULT
    AltBosRead(
        _In_ DsmEvent Event);
    SM_RESULT AltBosValidate();
    SM_RESULT EnterReadingMsOs();
    SM_RESULT
    OnReadingMsOs(
        _In_ DsmEvent Event);
    SM_RESULT MsOsStart();
    SM_RESULT
    MsOsRead(
        _In_ DsmEvent Event);
    SM_RESULT MsOsMarkUnsupported();
    SM_RESULT MsOsStoreVendorCode();
    SM_RESULT MsOsSerial();
    SM_RESULT
    MsOsSerialRead(
        _In_ DsmEvent Event);
    SM_RESULT
    MsOsError(
        _In_ DSM_THEN GoOn);
    SM_RESULT MsOsExtendedConfig();
    SM_RESULT
    MsOsExtendedHeaderRead(
        _In_ DsmEvent Event);
    SM_RESULT
    MsOsExtendedRead(
        _In_ DsmEvent Event);
    SM_RESULT MsOsContainerId();
    SM_RESULT
    MsOsContainerHeaderRead(
        _In_ DsmEvent Event);
    SM_RESULT
    MsOsContainerRead(
        _In_ DsmEvent Event);
    SM_RESULT MsOsNoContainerId();
    SM_RESULT MsOsMarkNoContainerId();
    SM_RESULT EnterReadingLanguages();
    SM_RESULT
    OnReadingLanguages(
        _In_ DsmEvent Event);
    SM_RESULT StringError();
    SM_RESULT EnterReadingProductName();
    SM_RESULT
    OnReadingProductName(
        _In_ DsmEvent Event);
    SM_RESULT EnterReadingQualifier();
    SM_RESULT
    OnReadingQualifier(
        _In_ DsmEvent Event);
    SM_RESULT
    QualifierRead(
        _In_ DsmEvent Event);
    SM_RESULT LinkStart();
    SM_RESULT
    LinkIsochDelaySet(
        _In_ DsmEvent Event);
    SM_RESULT LinkExitLatency();
    SM_RESULT
    LinkSelSet(
        _In_ DsmEvent Event);
    SM_RESULT LinkLatencyTolerance();
    SM_RESULT
    LinkLtmSet(
        _In_ DsmEvent Event);

    /* Reporting to PnP and the failed device, devsm_report.cpp */
    static const State ReportingToPnp;
    static const State ReportingUnknown;
    static const State FailedDevice;
    static const State FailedDeviceGone;
    static const State FailedDeviceEvents;
    static const State FailedHubSuspended;
    static const State PortOffEvents;

    SM_RESULT EnterReportingToPnp();
    SM_RESULT
    OnReportingToPnp(
        _In_ DsmEvent Event);
    SM_RESULT ReportAddChild();
    SM_RESULT ReportCreatePdo();
    SM_RESULT
    ReportDuplicateWaitEnded(
        _In_ DsmEvent Event);
    SM_RESULT
    ReportCycledWithTimer(
        _In_ DsmEvent Event);
    SM_RESULT
    EndOnDetach(
        _In_ DsmEvent Event);
    SM_RESULT EnterReportingUnknown();
    SM_RESULT ReportUnknownPdo();
    SM_RESULT
    OnReportingUnknown(
        _In_ DsmEvent Event);
    SM_RESULT EnterFailedDevice();
    SM_RESULT
    OnFailedDevice(
        _In_ DsmEvent Event);
    SM_RESULT
    FailedPortDisabledAgain(
        _In_ DsmEvent Event);
    SM_RESULT EnterFailedDeviceGone();
    SM_RESULT
    OnFailedDeviceGone(
        _In_ DsmEvent Event);
    SM_RESULT LeaveHub();
    SM_RESULT
    OnFailedDeviceEvents(
        _In_ DsmEvent Event);
    SM_RESULT
    FailedPortOffForHub(
        _In_ DsmEvent Event);
    SM_RESULT EnterFailedHubSuspended();
    SM_RESULT
    OnFailedHubSuspended(
        _In_ DsmEvent Event);
    SM_RESULT
    OnPortOffEvents(
        _In_ DsmEvent Event);

    /* Detach and removal, devsm_remove.cpp */
    static const State DetachedReported;
    static const State DetachedReportedConfigured;
    static const State DetachedMissing;
    static const State AwaitingRemove;
    static const State AwaitingDetachOrRemove;
    static const State ParkedPortOff;
    static const State ParkedPortOn;
    static const State RemovedPortOff;
    static const State RemovedPortOn;
    static const State DetachedEvents;
    static const State PortOnEvents;

    SM_RESULT EnterWithPortOff();
    SM_RESULT EnterWithPortOn();
    SM_RESULT EnterDetachedEvents();
    SM_RESULT DetachUnconfigured();
    SM_RESULT DetachConfigured();
    SM_RESULT EnterDetachedReported();
    SM_RESULT EnterDetachedReportedConfigured();
    SM_RESULT
    OnDetachedReported(
        _In_ DsmEvent Event);
    SM_RESULT EnterDetachedMissing();
    SM_RESULT
    OnDetachedMissing(
        _In_ DsmEvent Event);
    SM_RESULT
    DisableForRemoval(
        _In_ DSM_THEN After);
    SM_RESULT
    RemovalEndpointsDisabled(
        _In_ DsmEvent Event);
    SM_RESULT
    RemovalDeviceDisabled(
        _In_ DsmEvent Event);
    SM_RESULT RemovalDeleteAfterLeaving();
    SM_RESULT RemovalDeleteAndFinish();
    SM_RESULT RemovalPowerDownDone();
    SM_RESULT RemovalLeaveHub();
    SM_RESULT RemovalAckHubStop();
    SM_RESULT RemovalDropPowerForHubStop();
    SM_RESULT RemovalHubStopped();
    SM_RESULT
    OnAwaitingRemove(
        _In_ DsmEvent Event);
    SM_RESULT ReportMissingAndLeave();
    SM_RESULT
    OnAwaitingDetachOrRemove(
        _In_ DsmEvent Event);
    SM_RESULT RemovalNotifyThenReport();
    SM_RESULT RemovalDeleteForgetAndPark();
    SM_RESULT RemovalDeleteForget();
    SM_RESULT RemovalDropPowerAndPark();
    SM_RESULT RemovalAckAndPark();
    SM_RESULT
    OnParkedPortOff(
        _In_ DsmEvent Event);
    SM_RESULT
    OnParkedPortOn(
        _In_ DsmEvent Event);
    SM_RESULT RemovalDeleteForgetPortOn();
    SM_RESULT
    OnRemovedPortOff(
        _In_ DsmEvent Event);
    SM_RESULT
    OnRemovedPortOn(
        _In_ DsmEvent Event);
    SM_RESULT
    CleanupPortDisabled(
        _In_ DsmEvent Event);
    SM_RESULT
    CleanupDeviceDisabled(
        _In_ DsmEvent Event);
    SM_RESULT CleanupDeleteAll();
    SM_RESULT CleanupDone();
    SM_RESULT
    OnDetachedEvents(
        _In_ DsmEvent Event);
    SM_RESULT
    OnPortOnEvents(
        _In_ DsmEvent Event);
    SM_RESULT
    PortOnOffForHubSuspend(
        _In_ DsmEvent Event);

    /* Enumerated, PDO not started, devsm_stopped.cpp */
    static const State StoppedAddressed;
    static const State StoppedReady;
    static const State Stopped;
    static const State StoppedHubSuspended;
    static const State StoppedEnumeratedHubSuspended;
    static const State ReadingDeviceText;
    static const State AwaitingMsOsInstall;
    static const State SettingU2Timeout;

    SM_RESULT
    Then(
        _In_ DSM_THEN Next);
    SM_RESULT EnterWithMsOsInstall();
    SM_RESULT
    ToAwaitingDetachOrRemove(
        _In_ BOOLEAN Configured);
    SM_RESULT GoStoppedEnumerated();
    SM_RESULT EnterStoppedAddressed();
    SM_RESULT
    OnStoppedAddressed(
        _In_ DsmEvent Event);
    SM_RESULT StoppedAckPreStart();
    SM_RESULT StoppedPoweredUp();
    SM_RESULT
    StoppedDisabledForNoDriver(
        _In_ DsmEvent Event);
    SM_RESULT
    StoppedPortOffForNoDriver(
        _In_ DsmEvent Event);
    SM_RESULT
    StoppedPortOffForHubStop(
        _In_ DsmEvent Event);
    SM_RESULT
    StoppedDisabledForHub(
        _In_ DsmEvent Event);
    SM_RESULT
    StoppedPortSuspendedForHub(
        _In_ DsmEvent Event);
    SM_RESULT
    UnconfiguredByClient(
        _In_ DsmEvent Event);
    SM_RESULT BackToUnconfigured();
    SM_RESULT
    HubDescriptorDone(
        _In_ DsmEvent Event);
    SM_RESULT EnterReadingDeviceText();
    SM_RESULT
    DeviceTextRead(
        _In_ DsmEvent Event);
    SM_RESULT
    OnReadingDeviceText(
        _In_ DsmEvent Event);
    SM_RESULT
    OnAwaitingMsOsInstall(
        _In_ DsmEvent Event);
    SM_RESULT MsOsInstallStart();
    SM_RESULT MsOsInstallDone();
    SM_RESULT
    MsOsExtHeaderRead(
        _In_ DsmEvent Event);
    SM_RESULT
    MsOsExtRead(
        _In_ DsmEvent Event);
    SM_RESULT MsOsWriteProperties();
    SM_RESULT MsOsFreeAndDone();
    SM_RESULT
    OnStoppedReady(
        _In_ DsmEvent Event);
    SM_RESULT GoStoppedEnabled();
    SM_RESULT
    StoppedEnabledDisabledForHub(
        _In_ DsmEvent Event);
    SM_RESULT
    StoppedPortOffAfterDisable(
        _In_ DsmEvent Event);
    SM_RESULT
    OnStopped(
        _In_ DsmEvent Event);
    SM_RESULT StoppedDropPower();
    SM_RESULT
    OnStoppedHubSuspended(
        _In_ DsmEvent Event);
    SM_RESULT StoppedAckHubStop();
    SM_RESULT StoppedHubStarted();
    SM_RESULT
    StoppedPortOffAfterHubResume(
        _In_ DsmEvent Event);
    SM_RESULT EnterHubSuspendedReleasingPower();
    SM_RESULT
    OnStoppedEnumeratedHubSuspended(
        _In_ DsmEvent Event);
    SM_RESULT
    StoppedDisabledForCleanup(
        _In_ DsmEvent Event);
    SM_RESULT
    StoppedPortResumed(
        _In_ DsmEvent Event);
    SM_RESULT
    StoppedResumeEndedForStop(
        _In_ DsmEvent Event);
    SM_RESULT
    StoppedResumeEndedForSuspend(
        _In_ DsmEvent Event);
    SM_RESULT EnterSettingU2Timeout();
    SM_RESULT
    OnSettingU2Timeout(
        _In_ DsmEvent Event);
    SM_RESULT U2TimeoutStart();
    SM_RESULT
    U2TimeoutSet(
        _In_ DsmEvent Event);

    /* Unconfigured in D0, suspend and resume, devsm_power.cpp */
    static const State UnconfiguredOn;
    static const State SuspendingUnconfigured;
    static const State SuspendedUnconfigured;
    static const State UnconfiguredHubSuspended;
    static const State DeviceSuspended;
    static const State SuspendedHubSuspended;
    static const State AwaitingPowerUpForReset;
    static const State AwaitingPowerUpAfterWake;
    static const State OffAfterD3Cold;

    SM_RESULT GoUnconfiguredOn();
    SM_RESULT
    OnUnconfiguredOn(
        _In_ DsmEvent Event);
    SM_RESULT
    UnconfiguredSuspendedForHub(
        _In_ DsmEvent Event);
    SM_RESULT
    UnconfiguredDisabledForStop(
        _In_ DsmEvent Event);
    SM_RESULT
    UnconfiguredPortOffForStop(
        _In_ DsmEvent Event);
    SM_RESULT EnterSuspendingUnconfigured();
    SM_RESULT
    OnSuspendingUnconfigured(
        _In_ DsmEvent Event);
    SM_RESULT EnterSuspended();
    SM_RESULT
    OnSuspendedUnconfigured(
        _In_ DsmEvent Event);
    SM_RESULT
    UnconfiguredDisabledForRestart(
        _In_ DsmEvent Event);
    SM_RESULT
    OnUnconfiguredHubSuspended(
        _In_ DsmEvent Event);
    SM_RESULT
    UnconfiguredResumedWithHub(
        _In_ DsmEvent Event);
    SM_RESULT SuspendStart();
    SM_RESULT
    SuspendIoAborted(
        _In_ DsmEvent Event);
    SM_RESULT
    SuspendArmed(
        _In_ DsmEvent Event);
    SM_RESULT
    SuspendIoPurged(
        _In_ DsmEvent Event);
    SM_RESULT
    SuspendPortPlain(
        _In_ DsmEvent Event);
    SM_RESULT
    SuspendPortArmed(
        _In_ DsmEvent Event);
    SM_RESULT
    SuspendPortArmFailed(
        _In_ DsmEvent Event);
    SM_RESULT
    SuspendEndedForHub(
        _In_ DsmEvent Event);
    SM_RESULT
    OnDeviceSuspended(
        _In_ DsmEvent Event);
    SM_RESULT SuspendedResumedByPort();
    SM_RESULT
    SuspendedIoStartedOnWake(
        _In_ DsmEvent Event);
    SM_RESULT
    SuspendedBackDown(
        _In_ DsmEvent Event);
    SM_RESULT
    SuspendedPortDownForHub(
        _In_ DsmEvent Event);
    SM_RESULT
    SuspendedPortResumed(
        _In_ DsmEvent Event);
    SM_RESULT
    SuspendedIoStarted(
        _In_ DsmEvent Event);
    SM_RESULT
    SuspendedDisarmed(
        _In_ DsmEvent Event);
    SM_RESULT SuspendedResumeDone();
    SM_RESULT SuspendedNeedsReset();
    SM_RESULT EnterAwaitingPowerUpAfterWake();
    SM_RESULT
    OnAwaitingPowerUpAfterWake(
        _In_ DsmEvent Event);
    SM_RESULT
    SuspendedPurgedAfterWake(
        _In_ DsmEvent Event);
    SM_RESULT
    SuspendedPortOffForStop(
        _In_ DsmEvent Event);
    SM_RESULT
    OnSuspendedHubSuspended(
        _In_ DsmEvent Event);
    SM_RESULT
    OnAwaitingPowerUpForReset(
        _In_ DsmEvent Event);
    SM_RESULT
    SuspendedPurgedForReset(
        _In_ DsmEvent Event);
    VOID DisablePortMarkingReset();
    SM_RESULT
    SuspendedPortOffForReset(
        _In_ DsmEvent Event);
    SM_RESULT
    OnOffAfterD3Cold(
        _In_ DsmEvent Event);

    /* Configured device, client requests and configuration, devsm_config.cpp */
    static const State Configured;
    static const State ConfiguredOn;
    static const State ConfiguredNotStarted;
    static const State Configuring;
    static const State SettingInterface;
    static const State ProgrammingEndpoints;
    static const State UpdatingLinkPower;
    static const State SuspendingConfigured;
    static const State BootDeviceSuspending;
    static const State SuspendedConfigured;
    static const State BootDeviceSuspended;
    static const State ConfiguredHubSuspended;
    static const State PreparingHibernation;

    SM_RESULT BackToConfigured();
    SM_RESULT GoConfiguredOn();
    SM_RESULT
    OnConfigured(
        _In_ DsmEvent Event);
    SM_RESULT ClientDoneOk();
    SM_RESULT ClientDoneWithLastStatus();
    SM_RESULT
    ClientPipeDone(
        _In_ DsmEvent Event);
    SM_RESULT
    ClientHaltCleared(
        _In_ DsmEvent Event);
    SM_RESULT
    ConfiguredNullSet(
        _In_ DsmEvent Event);
    SM_RESULT
    ConfiguredEndpointsDisabledForClient(
        _In_ DsmEvent Event);
    SM_RESULT ConfiguredFreeForClient();
    SM_RESULT ConfiguredResetBootDevice();
    SM_RESULT
    OnConfiguredOn(
        _In_ DsmEvent Event);
    SM_RESULT ConfiguredSignalAndStay();
    SM_RESULT ConfiguredCyclePort();
    SM_RESULT
    ConfiguredEndpointsDisabledForStop(
        _In_ DsmEvent Event);
    SM_RESULT StopDecision();
    SM_RESULT
    ConfiguredPurgedForHub(
        _In_ DsmEvent Event);
    SM_RESULT
    ConfiguredSuspendedForHub(
        _In_ DsmEvent Event);
    VOID PurgeIoForHibernation();
    SM_RESULT
    ConfiguredPurgedForHibernation(
        _In_ DsmEvent Event);
    SM_RESULT EnterPreparingHibernation();
    SM_RESULT
    OnPreparingHibernation(
        _In_ DsmEvent Event);
    SM_RESULT
    OnConfiguredNotStarted(
        _In_ DsmEvent Event);
    SM_RESULT
    ConfiguredNotStartedCleanup(
        _In_ DsmEvent Event);
    SM_RESULT
    ConfiguredNotStartedHubStop(
        _In_ DsmEvent Event);
    SM_RESULT
    ConfiguredNotStartedHubSuspend(
        _In_ DsmEvent Event);
    SM_RESULT SelectConfigValidate();
    SM_RESULT
    SelectConfigFailed(
        _In_ BOOLEAN LastStatus);
    SM_RESULT
    SelectConfigDone(
        _In_ DsmEvent Event);
    SM_RESULT SelectConfigCleanUp();
    SM_RESULT EnterConfiguring();
    SM_RESULT
    OnConfiguring(
        _In_ DsmEvent Event);
    SM_RESULT
    ConfigureOldDisabled(
        _In_ DsmEvent Event);
    SM_RESULT ConfigureDeleteOld();
    SM_RESULT
    ConfigureInterfaceSet(
        _In_ DsmEvent Event);
    SM_RESULT ConfigureCreateEndpoints();
    SM_RESULT ConfigureFailed();
    SM_RESULT
    ConfigureProgrammed(
        _In_ DsmEvent Event);
    SM_RESULT EnterProgrammingEndpoints();
    SM_RESULT
    OnProgrammingEndpoints(
        _In_ DsmEvent Event);
    SM_RESULT
    EndpointsProgrammed(
        _In_ DsmEvent Event);
    SM_RESULT EnterSettingInterface();
    SM_RESULT
    OnSettingInterface(
        _In_ DsmEvent Event);
    SM_RESULT InterfaceStart();
    SM_RESULT InterfaceCreateEndpoints();
    SM_RESULT
    InterfaceNewDisabled(
        _In_ DsmEvent Event);
    SM_RESULT
    InterfaceProgrammed(
        _In_ DsmEvent Event);
    SM_RESULT InterfaceRestoreOld();
    SM_RESULT
    InterfaceOldRestored(
        _In_ DsmEvent Event);
    SM_RESULT
    InterfaceSet(
        _In_ DsmEvent Event);
    SM_RESULT
    InterfaceNewEndpointsDisabled(
        _In_ DsmEvent Event);
    SM_RESULT InterfaceDeleteOld();
    SM_RESULT InterfaceDeleteNewAndFail();
    SM_RESULT InterfaceDeleteOldAndFail();
    SM_RESULT EnterUpdatingLinkPower();
    SM_RESULT
    OnUpdatingLinkPower(
        _In_ DsmEvent Event);
    SM_RESULT EnterSuspendingConfigured();
    SM_RESULT
    OnSuspendingConfigured(
        _In_ DsmEvent Event);
    SM_RESULT
    ConfiguredPurgedWhileSuspending(
        _In_ DsmEvent Event);
    SM_RESULT
    OnBootDeviceSuspending(
        _In_ DsmEvent Event);
    SM_RESULT
    OnSuspendedConfigured(
        _In_ DsmEvent Event);
    SM_RESULT ConfiguredResumed();
    SM_RESULT ConfiguredRestartWhileSuspended();
    SM_RESULT
    OnBootDeviceSuspended(
        _In_ DsmEvent Event);
    SM_RESULT
    OnConfiguredHubSuspended(
        _In_ DsmEvent Event);
    SM_RESULT
    ConfiguredResumedWithHub(
        _In_ DsmEvent Event);
    SM_RESULT
    ConfiguredIoStarted(
        _In_ DsmEvent Event);

    /* SuperSpeed U1 and U2 sub machine, devsm_lpm.cpp */

    SM_RESULT LinkPowerStart();
    SM_RESULT LpmCompute();
    SM_RESULT
    LpmProgrammedBeforeRaise(
        _In_ DsmEvent Event);
    SM_RESULT
    LpmLatencyRaised(
        _In_ DsmEvent Event);
    SM_RESULT LpmLatencyTooLarge();
    SM_RESULT LpmFailCleanup();
    SM_RESULT
    LpmCleanedUpAfterFailure(
        _In_ DsmEvent Event);
    SM_RESULT LpmDetachCleanup();
    SM_RESULT
    LpmCleanedUpAfterDetach(
        _In_ DsmEvent Event);
    SM_RESULT LpmU1();
    SM_RESULT
    LpmU1TimeoutSet(
        _In_ DsmEvent Event);
    SM_RESULT LpmEnableU1();
    SM_RESULT
    LpmU1Disabled(
        _In_ DsmEvent Event);
    SM_RESULT
    LpmU1Enabled(
        _In_ DsmEvent Event);
    SM_RESULT LpmU2();
    SM_RESULT
    LpmU2TimeoutSet(
        _In_ DsmEvent Event);
    SM_RESULT LpmEnableU2();
    SM_RESULT
    LpmU2Disabled(
        _In_ DsmEvent Event);
    SM_RESULT
    LpmU2Enabled(
        _In_ DsmEvent Event);
    SM_RESULT LpmLower();
    SM_RESULT
    LpmLatencyLowered(
        _In_ DsmEvent Event);
    SM_RESULT LpmFinalProgram();
    SM_RESULT
    LpmFinalProgrammed(
        _In_ DsmEvent Event);

    /* Boot, paging or hibernation file device, devsm_boot.cpp */
    static const State BootDeviceAwaitingReset;
    static const State BootDeviceDetached;
    static const State BootDeviceDetachedContextGone;
    static const State BootDeviceResetPending;
    static const State BootDeviceEvents;

    SM_RESULT EnterBootDeviceEvents();
    SM_RESULT PurgeOnDetach();
    VOID NotifyAndPurgeIo();
    SM_RESULT
    PurgedOnDetach(
        _In_ DsmEvent Event);
    SM_RESULT BootDeviceNotifyDetached();
    SM_RESULT BootDeviceDisableOnDetach();
    VOID DisableEndpointsPowerLost();
    SM_RESULT
    BootDeviceDisabledOnDetach(
        _In_ DsmEvent Event);
    SM_RESULT BootDeviceFailed();
    VOID DisableEndpointsAndNotify();
    SM_RESULT
    BootDeviceDisabledOnFailure(
        _In_ DsmEvent Event);
    SM_RESULT
    BootDeviceDetachDuringReset(
        _In_ BOOLEAN ContextGone);
    SM_RESULT
    OnBootDeviceAwaitingReset(
        _In_ DsmEvent Event);
    SM_RESULT EnterBootDeviceDetached();
    SM_RESULT EnterBootDeviceDetachedContextGone();
    SM_RESULT
    OnBootDeviceDetached(
        _In_ DsmEvent Event);
    SM_RESULT EnterBootDeviceResetPending();
    SM_RESULT
    OnBootDeviceResetPending(
        _In_ DsmEvent Event);
    SM_RESULT
    OnBootDeviceEvents(
        _In_ DsmEvent Event);

    /* Re-enumeration, devsm_reenum.cpp */
    static const State Reenumerating;
    static const State ReenumeratingForHubStart;
    static const State ReenumeratingForPower;
    static const State AtAddressZeroAgain;
    static const State AddressingAgain;
    static const State LinkPowerAgain;
    static const State ParkedForHubAgain;
    static const State ClaimingAddressZeroAgain;
    static const State CancelingClaimAgain;
    static const State BugChecking;

    SM_RESULT
    Reenumerate(
        _In_ const State* Target,
        _In_ DSM_REENUM Kind,
        _In_ BOOLEAN ForceReset,
        _In_ DSM_NEXT After);
    SM_RESULT EnterReenumerating();
    SM_RESULT
    OnReenumerating(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumTreePurgedConfigured(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumEndpointsDisabled(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumTreePurgedUnconfigured(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumEnabled(
        _In_ DsmEvent Event);
    VOID FailIoAndDisablePort();
    SM_RESULT
    ReenumPortOffDeviceDisabled(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumPortOffForHubDeviceDisabled(
        _In_ DsmEvent Event);
    SM_RESULT ReenumParkForHub();
    SM_RESULT
    OnParkedForHubAgain(
        _In_ DsmEvent Event);
    SM_RESULT
    OnClaimingAddressZeroAgain(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumClaimEndedOnDetach(
        _In_ DsmEvent Event);
    SM_RESULT
    OnCancelingClaimAgain(
        _In_ DsmEvent Event);
    SM_RESULT ReenumDisableForHub();
    SM_RESULT
    ReenumDisabledForHub(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumPortOffForHub(
        _In_ DsmEvent Event);
    SM_RESULT ReenumRetryOrFail();
    VOID FailIoAndPurge();
    SM_RESULT
    ReenumPurgedAfterFailure(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumPortOffAfterFailure(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumPortOffForHubDeviceEnabled(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumDisabledThenPark(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumRetryDelayEnded(
        _In_ DsmEvent Event);
    SM_RESULT ReenumStopRetryOnDetach();
    SM_RESULT
    ReenumTimerFlushedOnDetach(
        _In_ DsmEvent Event);
    SM_RESULT ReenumStopRetryForHub();
    SM_RESULT
    ReenumTimerFlushedForHub(
        _In_ DsmEvent Event);
    SM_RESULT EnterAtAddressZeroAgain();
    SM_RESULT
    OnAtAddressZeroAgain(
        _In_ DsmEvent Event);
    SM_RESULT ReenumRetryOrFailHoldingAddressZero();
    SM_RESULT
    ReenumPurgedHoldingAddressZero(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumPortOffHoldingAddressZero(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumRetryDelayEndedHoldingAddressZero(
        _In_ DsmEvent Event);
    SM_RESULT
    Again20FirstResetDone(
        _In_ DsmEvent Event);
    SM_RESULT
    Again20FirstResetNotified(
        _In_ DsmEvent Event);
    SM_RESULT
    Again20FirstDelayEnded(
        _In_ DsmEvent Event);
    SM_RESULT
    Again20FirstDescriptorRead(
        _In_ DsmEvent Event);
    SM_RESULT
    Again20SecondResetDone(
        _In_ DsmEvent Event);
    SM_RESULT
    Again20SecondResetNotified(
        _In_ DsmEvent Event);
    SM_RESULT
    Again20SecondDelayEnded(
        _In_ DsmEvent Event);
    SM_RESULT
    OnAddressingAgain(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumDescriptorRead(
        _In_ DsmEvent Event);
    SM_RESULT ReenumWrongDevice();
    SM_RESULT
    ReenumWrongDeviceLeft(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumAltEnumSent(
        _In_ DsmEvent Event);
    SM_RESULT ReenumSerialNumber();
    SM_RESULT
    ReenumSerialRead(
        _In_ DsmEvent Event);
    SM_RESULT ReenumConfigDescriptor();
    SM_RESULT
    ReenumConfigRead(
        _In_ DsmEvent Event);
    SM_RESULT EnterLinkPowerAgain();
    SM_RESULT
    OnLinkPowerAgain(
        _In_ DsmEvent Event);
    SM_RESULT Link20Start();
    SM_RESULT
    Link20Updated(
        _In_ DsmEvent Event);
    SM_RESULT ReenumPdCharging();
    SM_RESULT
    ReenumPdChargingSet(
        _In_ DsmEvent Event);
    SM_RESULT EnterBugChecking();
    SM_RESULT CycleAndPark();
    SM_RESULT
    ReenumDoneForClientUnconfigured(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumFailedForClient(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumDoneForClientConfigured(
        _In_ DsmEvent Event);
    SM_RESULT
    ReconfiguredForClient(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumDoneForBootDevice(
        _In_ DsmEvent Event);
    SM_RESULT
    ReconfiguredBootDevice(
        _In_ DsmEvent Event);
    SM_RESULT
    BootDeviceDisabledForReset(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumDoneForHubStart(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumDisabledThenReport(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumDisabledThenCycle(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumDoneForStart(
        _In_ DsmEvent Event);
    SM_RESULT ReenumCleanUpOldConfig();
    SM_RESULT
    ReenumDisabledThenSignalAndCycle(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumDoneForPreStart(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumDisabledThenAckAndCycle(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumDoneForHubResumeUnconfigured(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumFailedForHubResume(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumDoneForHubResumeConfigured(
        _In_ DsmEvent Event);
    SM_RESULT
    ReconfiguredAfterHubResume(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumPortOffAfterConfigFailure(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumDoneForResumeUnconfigured(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumFailedForResume(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumDoneForResumeConfigured(
        _In_ DsmEvent Event);
    SM_RESULT ReenumCycleAfterResumeFailure();
    SM_RESULT
    ReconfiguredAfterResume(
        _In_ DsmEvent Event);
    SM_RESULT
    ReenumDoneForBootResume(
        _In_ DsmEvent Event);
    SM_RESULT
    ReconfiguredBootAfterResume(
        _In_ DsmEvent Event);
    SM_RESULT ReenumForClientUnconfigured();
    SM_RESULT ReenumForClientConfigured();
    SM_RESULT
    ReenumForBootClient(
        _In_ BOOLEAN AfterReattach);
    SM_RESULT ReenumForHubStart();
    SM_RESULT
    ReenumForStart(
        _In_ DSM_REENUM Kind);
    SM_RESULT ReenumForPreStart();
    SM_RESULT
    ReenumForHubResume(
        _In_ BOOLEAN Configured);
    SM_RESULT
    ReenumForResume(
        _In_ BOOLEAN Configured);
    SM_RESULT ReenumForBootResume();

    /*
     * Actions and queries, implemented by the device object
     */
    VOID ReferenceDevice();
    VOID DereferenceDevice();
    VOID QueuePassiveWork();
    BOOLEAN CancelStateTimer();
    ULONG DeviceKind();

    BOOLEAN RegisterWithHub();
    VOID UnregisterFromHub();
    VOID MarkAttachFailed();
    VOID MarkAttachSucceeded();
    VOID ReferencePort();
    VOID DereferencePort();
    BOOLEAN ReferenceHubPower();
    VOID DereferenceHubPower();
    VOID ReferenceDevicePower();
    VOID DereferenceDevicePower();

    VOID NotifyPortDetached();
    VOID RequestPortDisable();
    VOID RequestPortSuspend();
    VOID RequestPortResume();
    VOID SignalPnpWaiter();
    BOOLEAN ResetOnResumeFromSx();
    VOID SignalPnpPowerFailure();
    VOID ConfirmStopWhileSuspended();
    VOID AskPortCycle();
    VOID ForwardToController();

    /* In place answers to requests the current state turns down */
    VOID FailClientRequest();
    VOID FailHubDescriptorRequest();
    VOID AckPreStart();
    VOID CyclePortForClient();
    VOID SignalQueryText();
    VOID ReleasePnpWaiter();

    VOID StartDuplicateRetries();
    DSM_ADD_CHILD AddChildToHub();
    BOOLEAN DuplicateRetriesUsedUp();
    VOID DropSerialNumber();
    BOOLEAN CreatePdo();
    VOID ForgetChild();
    BOOLEAN CreatePlaceholderPdo();
    VOID ReportEnumFailure();
    VOID ReportDeviceMissing();
    VOID MarkDisconnected();
    VOID NotifyDisconnected();
    VOID DeleteConfigEndpoints();
    VOID DisableAllEndpoints();

    VOID StartEnumRetries();
    BOOLEAN EnumRetriesUsedUp();
    VOID StartRetryTimer();
    BOOLEAN StopTimer();
    BOOLEAN DeviceProgrammingLost();

    VOID ClaimAddressZero();
    VOID ReleaseAddressZero();
    VOID CancelControllerIoctl();
    VOID UpdateDeviceInController();
    VOID DisableDeviceInController();
    VOID DeleteDefaultEndpoint();
    VOID DeleteDeviceInController();

    BOOLEAN SupportsPdCharging();
    VOID SetPdChargingPolicy();

    VOID RequestPortReset();
    VOID RequestPortWarmReset();
    BOOLEAN SetSpeedFor20();
    BOOLEAN SetSpeedFor30();
    VOID RecordDeviceVersion();
    BOOLEAN CreateDeviceInController();
    BOOLEAN CreateDefaultEndpoint();
    VOID EnableDeviceInController();
    BOOLEAN IsFirstEnumTry();
    VOID StartPostResetTimer();
    VOID StartLongPostResetTimer();
    VOID StartSuperSpeedPostResetTimer();
    VOID ArmPostAddressTimer();
    VOID ArmDuplicateDeviceTimer();
    VOID ReadFirstDescriptor();
    BOOLEAN FirstDescriptorValid();
    VOID UpdateDefaultEndpoint();
    BOOLEAN NeedsSecondReset();
    VOID NotifyDeviceReset();
    VOID RecordResetTimeout();
    DSM_RESET_KIND ResetKindNeeded();
    VOID SetDeviceAddress();
    BOOLEAN WaitNeededAfterAddress();

    VOID CancelTransfer();
    VOID ReadDeviceDescriptor();
    BOOLEAN DeviceDescriptorValid();
    BOOLEAN QueryRegistryValues();
    BOOLEAN AltEnumNeededInEnum();
    BOOLEAN AltEnumNeededAfterBos();
    VOID SendAltEnumCommand();
    BOOLEAN SuperSpeedMustBeDisabled();
    VOID RequestSuperSpeedDisable();
    VOID ReadConfigDescriptorHeader();
    VOID ReadFullConfigDescriptor();
    BOOLEAN ConfigLongerThanRead();
    BOOLEAN ConfigDescriptorValid();
    BOOLEAN BosQuerySkipped();
    VOID ReadBosHeader();
    BOOLEAN BosHeaderValid();
    BOOLEAN BosComplete();
    VOID ReadBos();
    BOOLEAN BosValid();
    BOOLEAN IgnoreDescriptorError();
    BOOLEAN DualRoleSupported();
    VOID SendUsbFeatures();
    BOOLEAN HasBillboard();
    BOOLEAN BillboardStringWanted();
    VOID ReadBillboardString();
    BOOLEAN BillboardStringValid();
    BOOLEAN AltModeStringWanted();
    VOID ReadAltModeString();
    BOOLEAN AltModeStringValid();
    BOOLEAN MsOs20Supported();
    VOID ReadMsOs20Set();
    BOOLEAN MsOs20SetValid();
    BOOLEAN ProductStringIndexZero();
    BOOLEAN MsOsQueryWanted();
    VOID ReadMsOsDescriptor();
    BOOLEAN MsOsDescriptorValid();
    VOID MarkMsOsUnsupported();
    VOID StoreMsOsVendorCode();
    BOOLEAN MsOsContainerIdSupported();
    VOID MarkContainerIdUnsupported();
    BOOLEAN IgnoreSerialNumber();
    BOOLEAN SerialNumberIndexZero();
    VOID ReadSerialNumber();
    BOOLEAN SerialNumberValid();
    BOOLEAN MsOsExtendedConfigSupported();
    VOID ReadExtendedConfigHeader();
    BOOLEAN ExtendedConfigHeaderValid();
    VOID ReadExtendedConfig();
    BOOLEAN ExtendedConfigValid();
    BOOLEAN ContainerIdWanted();
    VOID ReadContainerIdHeader();
    BOOLEAN ContainerIdHeaderValid();
    VOID ReadContainerId();
    BOOLEAN ContainerIdKnown();
    VOID ReadLanguageIds();
    BOOLEAN LanguageIdsValid();
    BOOLEAN ProductNameWanted();
    VOID ReadProductName();
    BOOLEAN ProductNameValid();
    VOID ReadQualifier();
    BOOLEAN QualifierValid();
    BOOLEAN IsochDelaySkipped();
    VOID SetIsochDelay();
    BOOLEAN SelSkipped();
    VOID SetSel();
    BOOLEAN LtmWanted();
    VOID EnableLtm();

    VOID StartDriverWaitTimer();
    BOOLEAN DisableWhenUnused();
    VOID ForceResetOnNextStart();
    VOID SetNullConfiguration();
    VOID CompletePreStart();
    VOID AllowIo();
    VOID CompleteClientRequest();
    BOOLEAN PdoStarted();
    VOID GetDescriptorForHub();
    VOID CompleteQueryText();
    BOOLEAN ExtPropertiesWanted();
    VOID MarkMsOsInstallHandled();
    BOOLEAN SetExtPropertiesSemaphore();
    VOID ReadExtPropertiesHeader();
    BOOLEAN MsOs20ValuesWanted();
    BOOLEAN InstallMsOs20Values();
    BOOLEAN ExtPropertiesHeaderValid();
    BOOLEAN AllocateExtPropertiesBuffer();
    VOID ReadExtProperties();
    BOOLEAN ExtPropertiesValid();
    BOOLEAN WriteCustomProperties();
    VOID FreeExtPropertiesBuffer();
    BOOLEAN U2NeededForEnumerated();
    VOID RequestU2Timeout();
    VOID ClearResetAtResume();
    BOOLEAN ArmForWakeWanted();
    VOID AbortDeviceIo();
    VOID PurgeIoForSuspend();
    VOID ArmForWake();
    VOID FinishWaitWake();
    VOID MarkResetAtResume();
    BOOLEAN DeviceArmedForWake();
    VOID StartDeviceIo();
    BOOLEAN DisarmOnResume();
    VOID DisarmWake();
    BOOLEAN ResetOnResumeFromS0();
    BOOLEAN ResetAtResumeMarked();
    VOID MarkSystemWakeSource();
    BOOLEAN IsBootDevice();
    BOOLEAN SelectInterfaceValid();
    BOOLEAN FindClientPipe();
    BOOLEAN PipeIsZeroBandwidth();
    VOID ClearEndpointHalt();
    VOID ResetEndpoint();
    BOOLEAN PipeIsIsochronous();
    VOID ResetEndpointAndTransfers();
    VOID CompleteClientRequestLastStatus();
    VOID CompleteClientRequestFailed();
    BOOLEAN AdjustLatencyForNoPing();
    VOID RequestHibernationPrep();
    BOOLEAN SelectConfigValid();
    BOOLEAN PrepareConfigLists();
    VOID SetConfigInfoInRequest();
    VOID DisablePendingEndpoints();
    VOID DeleteOldConfigEndpoints();
    VOID SetConfiguration();
    BOOLEAN AlternateSettingLeft();
    VOID SetInterface();
    BOOLEAN CreateEndpoints();
    VOID MarkPendingEndpointsDisabled();
    VOID ProgramEndpoints();
    VOID SetInterfaceInfoInRequest();
    BOOLEAN PrepareInterfaceLists();
    VOID DisableNewInterfaceEndpoints();
    VOID DeleteOldInterfaceEndpoints();
    VOID DeleteNewInterfaceEndpoints();
    VOID InitLinkStates();
    VOID CalcU1Timeout();
    VOID CalcU2Timeout();
    VOID ComputeExitLatency();
    BOOLEAN ExitLatencyMustRise();
    BOOLEAN EndpointsNeedProgramming();
    VOID UpdateExitLatency();
    BOOLEAN DisableLinkStatesForLatency();
    BOOLEAN EndpointsNeedDisableOnFailure();
    BOOLEAN U1TimeoutChanged();
    VOID RequestU1Timeout();
    VOID DisableU1();
    VOID EnableU1();
    VOID MarkU1Disabled();
    VOID MarkU1Enabled();
    BOOLEAN U2TimeoutChanged();
    VOID DisableU2();
    VOID EnableU2();
    VOID MarkU2Disabled();
    VOID MarkU2Enabled();
    BOOLEAN ExitLatencyMayDrop();
    VOID PurgeDeviceIo();
    VOID MarkPowerLost();
    DSM_FEATURE_CHANGE U1Change();
    DSM_FEATURE_CHANGE U2Change();
    VOID LogReenumeration();
    VOID PurgeDeviceTreeIo();
    VOID ClearReprogramNeeded();
    VOID SetFailIo();
    BOOLEAN DeviceSpeedChanged();
    BOOLEAN SameDeviceConnected();
    BOOLEAN AltEnumNeededOnReenum();
    VOID NotifyWrongDevice();
    BOOLEAN SerialNumberCompared();
    BOOLEAN SameSerialNumber();
    BOOLEAN ConfigReadOnReset();
    BOOLEAN UpdateLpm20();
    VOID BugCheckForBootDevice();
    BOOLEAN PrepareListsForReset();
    VOID NotifyReconnected();

    HubChild* m_Device;
    DSM_NEXT m_Next;
    DSM_THEN m_Then;
    DSM_THEN m_AfterDelete;
    const State* m_ReturnTo;
    DsmEvent m_Result;
    DsmEvent m_HubEvent;
    BOOLEAN m_FailedPoweredUp;
    BOOLEAN m_EndpointsConfigured;
    BOOLEAN m_ChildForgotten;
    BOOLEAN m_Unregistered;
    BOOLEAN m_PortMayResume;
    DSM_THEN m_AfterDisable;
    DSM_THEN m_Resume;
    DSM_REENUM m_ReenumKind;
    BOOLEAN m_ReenumForceReset;
    BOOLEAN m_DetachEndsReset;
    DSM_NEXT m_AfterReenum;
    BOOLEAN m_FromConfigured;
    BOOLEAN m_ContextGone;
    DSM_NEXT m_AfterConfigure;
    DSM_NEXT m_AfterProgram;
    BOOLEAN m_D3Cold;
    DSM_NEXT m_AfterPurge;
    DSM_DELAY m_DelayKind;
    DSM_ACTION m_Start;
    BOOLEAN m_ConfigFullLength;
};
