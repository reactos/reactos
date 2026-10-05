/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Hub state machine
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include "usbhub3.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* USB4 router mailbox: 16 data registers, then metadata and opcode/status */
static const USHORT HubMailboxMetadata = 16;
static const USHORT HubMailboxOpcode = 17;
static const ULONG HubMailboxDataWords = 16;
static const ULONG HubMailboxBusy = 0x80000000UL;

/* Router operation DROM_READ, with the operation valid bit */
static const ULONG HubDromReadCommand = 0x80000024UL;

/* The header pass reads the first four DWORDs of the DROM */
static const ULONG HubDromHeaderWords = 4;

/* Bytes of the DROM that its length field does not count */
static const ULONG HubDromLengthBias = 13;
static const ULONG HubDromMinimumLength = 3;

/* How long the router may stay busy on one window, in 100 ns units */
static const ULONGLONG HubDromBusyLimit = 5ULL * 10 * 1000 * 1000;

const SM_EVENT_INFO HubMachine::EventInfo[] =
{
    { "DeviceAdded",                SmEventRequest,    FALSE },
    { "PrepareHardware",            SmEventRequest,    FALSE },
    { "ReleaseHardware",            SmEventRequest,    FALSE },
    { "DeviceCleanup",              SmEventRequest,    FALSE },
    { "D0Entry",                    SmEventRequest,    FALSE },
    { "D0EntryFromSx",              SmEventRequest,    FALSE },
    { "D0Exit",                     SmEventRequest,    FALSE },
    { "D0ExitFinal",                SmEventRequest,    FALSE },
    { "PortStatusRequest",          SmEventRequest,    FALSE },
    { "ResetRequested",             SmEventRequest,    FALSE },
    { "TimerFired",                 SmEventCompletion, FALSE },
    { "ConfigurationSet",           SmEventCompletion, FALSE },
    { "ConfigurationFailed",        SmEventCompletion, FALSE },
    { "IoctlDone",                  SmEventCompletion, FALSE },
    { "IoctlFailed",                SmEventCompletion, FALSE },
    { "InterruptDone",              SmEventCompletion, FALSE },
    { "InterruptFailed",            SmEventCompletion, FALSE },
    { "ResetDone",                  SmEventCompletion, FALSE },
    { "ResetFailed",                SmEventCompletion, FALSE },
    { "ResetFailedOnRemoval",       SmEventCompletion, FALSE },
    { "PortsAcquired",              SmEventCompletion, FALSE },
    { "PortsReleased",              SmEventCompletion, FALSE },
    { "PortsResetReleased",         SmEventCompletion, FALSE },
    { "PortsInterruptReleased",     SmEventCompletion, FALSE },
    { "PortsEnableInterrupt",       SmEventCompletion, FALSE },
    { "DevicesAcquired",            SmEventCompletion, FALSE },
    { "DevicesReleased",            SmEventCompletion, FALSE },
    { "DevicesStoppedAfterSuspend", SmEventCompletion, FALSE },
    { "TransferDone",               SmEventCritical,   FALSE },
    { "TransferFailed",             SmEventCritical,   FALSE },
    { "PipeResetDone",              SmEventCritical,   FALSE },
    { "PipeResetFailed",            SmEventCritical,   FALSE },
    { "CallSucceeded",              SmEventCompletion, FALSE },
    { "CallFailed",                 SmEventCompletion, FALSE },
};

static_assert(RTL_NUMBER_OF(HubMachine::EventInfo) == static_cast<ULONG>(HubEvent::Count),
              "EventInfo must have one entry per HubEvent");

const HubEvent HubMachine::ResetDiscards[] = { HubEvent::ResetRequested, HubEvent::Count };

#define HSM_REQ     SM_STATE_TAKES_REQUESTS
#define HSM_PASSIVE SM_STATE_NEEDS_PASSIVE
#define HSM_CRIT    SM_STATE_CRITICAL_ONLY

typedef HubMachine M;

/* Start up */
const M::State M::AwaitingAddDevice =
    { NULL, "AwaitingAddDevice", HSM_REQ, &M::OnAwaitingAddDevice, NULL };
const M::State M::Unconfigured =
    { NULL, "Unconfigured", HSM_REQ, &M::OnUnconfigured, NULL };
const M::State M::QueryingParentInfo =
    { NULL, "QueryingParentInfo", 0, &M::OnQueryingParentInfo, &M::EnterQueryingParentInfo };
const M::State M::Configuring =
    { NULL, "Configuring", 0, &M::OnConfiguring, &M::EnterConfiguring };
const M::State M::Reconfiguring =
    { NULL, "Reconfiguring", 0, &M::OnReconfiguring, &M::EnterReconfiguring };

/* USB4 router DROM read, every step is a mailbox control transfer */
const M::State M::DromRead =
    { NULL, "DromRead", 0, &M::OnDromRead, NULL };
const M::State M::DromReadingFirstStatus =
    { &DromRead, "DromReadingFirstStatus", HSM_PASSIVE,
      &M::OnDromReadingFirstStatus, &M::EnterDromReadingFirstStatus };
const M::State M::DromWritingMetadata =
    { &DromRead, "DromWritingMetadata", HSM_PASSIVE,
      &M::OnDromWritingMetadata, &M::EnterDromWritingMetadata };
const M::State M::DromSendingCommand =
    { &DromRead, "DromSendingCommand", HSM_PASSIVE,
      &M::OnDromSendingCommand, &M::EnterDromSendingCommand };
const M::State M::DromReadingStatus =
    { &DromRead, "DromReadingStatus", HSM_PASSIVE,
      &M::OnDromReadingStatus, &M::EnterDromReadingStatus };
const M::State M::DromReadingWord =
    { &DromRead, "DromReadingWord", HSM_PASSIVE,
      &M::OnDromReadingWord, &M::EnterDromReadingWord };

const M::State M::QueryingErrata =
    { NULL, "QueryingErrata", HSM_PASSIVE, NULL, &M::EnterQueryingErrata };
const M::State M::AwaitingD0 =
    { NULL, "AwaitingD0", HSM_REQ, &M::OnAwaitingD0, &M::EnterSignalPnp };

/* Not running, port status requests fail with no such device */
const M::State M::Down =
    { NULL, "Down", HSM_REQ, &M::OnDown, NULL };
const M::State M::Stopped =
    { &Down, "Stopped", 0, &M::OnStopped, NULL };
const M::State M::AwaitingReleaseHardware =
    { &Down, "AwaitingReleaseHardware", 0, &M::OnAwaitingReleaseHardware,
      &M::EnterSignalPnp, ResetDiscards };
const M::State M::AwaitingReleaseAfterFailure =
    { &Down, "AwaitingReleaseAfterFailure", 0, &M::OnAwaitingReleaseAfterFailure, NULL };
const M::State M::AwaitingStop =
    { &Down, "AwaitingStop", 0, &M::OnAwaitingStop, NULL, ResetDiscards };
const M::State M::SurpriseRemoved =
    { &Down, "SurpriseRemoved", 0, &M::OnSurpriseRemoved, &M::EnterSignalPnp, ResetDiscards };
const M::State M::Deleted =
    { NULL, "Deleted", 0, NULL, &M::EnterSignalPnp };

/* Power up and running */
const M::State M::PortsAcquiring =
    { NULL, "PortsAcquiring", 0, &M::OnPortsAcquiring, NULL };
const M::State M::DevicesAcquiring =
    { NULL, "DevicesAcquiring", 0, &M::OnDevicesAcquiring, NULL };
const M::State M::Listening =
    { NULL, "Listening", HSM_REQ, &M::OnListening, NULL };
const M::State M::NotListening =
    { NULL, "NotListening", HSM_REQ, &M::OnNotListening, NULL };
const M::State M::ReadingPortStatus =
    { NULL, "ReadingPortStatus", HSM_CRIT, &M::OnReadingPortStatus, NULL };
const M::State M::ReadingHubStatus =
    { NULL, "ReadingHubStatus", 0, &M::OnReadingHubStatus, NULL };
const M::State M::AckingHubChange =
    { NULL, "AckingHubChange", 0, &M::OnAckingHubChange, NULL };
const M::State M::AwaitingOverCurrentTimer =
    { NULL, "AwaitingOverCurrentTimer", 0, &M::OnAwaitingOverCurrentTimer, NULL };
const M::State M::ResettingPipe =
    { NULL, "ResettingPipe", HSM_CRIT, &M::OnResettingPipe, NULL };

/* Tear down, m_Teardown says where it ends */
const M::State M::DrainingInterruptRefs =
    { NULL, "DrainingInterruptRefs", 0, &M::OnDrainingInterruptRefs, NULL };
const M::State M::CancelingInterrupt =
    { NULL, "CancelingInterrupt", 0, &M::OnCancelingInterrupt, NULL };
const M::State M::AwaitingInterruptRefs =
    { NULL, "AwaitingInterruptRefs", 0, &M::OnAwaitingInterruptRefs, NULL };
const M::State M::AwaitingInterruptEnable =
    { NULL, "AwaitingInterruptEnable", 0, &M::OnAwaitingInterruptEnable, NULL };
const M::State M::DevicesReleasing =
    { NULL, "DevicesReleasing", 0, &M::OnDevicesReleasing, NULL };
const M::State M::PortsReleasing =
    { NULL, "PortsReleasing", 0, &M::OnPortsReleasing, NULL };
const M::State M::PortsResetting =
    { NULL, "PortsResetting", 0, &M::OnPortsResetting, NULL };
const M::State M::CheckingRecoveryLimit =
    { NULL, "CheckingRecoveryLimit", HSM_PASSIVE, NULL, &M::EnterCheckingRecoveryLimit };
const M::State M::ReportingFailure =
    { NULL, "ReportingFailure", HSM_PASSIVE, NULL, &M::EnterReportingFailure };
const M::State M::Halted =
    { NULL, "Halted", 0, NULL, &M::EnterHalted };

/* Hub reset through the parent */
const M::State M::HubResetting =
    { NULL, "HubResetting", 0, &M::OnHubResetting, &M::EnterHubResetting, ResetDiscards };
const M::State M::LoggingReset =
    { NULL, "LoggingReset", HSM_PASSIVE, NULL, &M::EnterLoggingReset };
const M::State M::AwaitingResetRetry =
    { NULL, "AwaitingResetRetry", 0, &M::OnAwaitingResetRetry, NULL };
const M::State M::ReconfiguringAfterReset =
    { NULL, "ReconfiguringAfterReset", HSM_PASSIVE,
      &M::OnReconfiguringAfterReset, &M::EnterReconfiguringAfterReset };
const M::State M::ResettingOnResume =
    { NULL, "ResettingOnResume", 0, &M::OnResettingOnResume, &M::EnterHubResetting };
const M::State M::ReconfiguringAfterResume =
    { NULL, "ReconfiguringAfterResume", 0,
      &M::OnReconfiguringAfterResume, &M::EnterReconfiguringAfterResume };
const M::State M::CheckingPowerLoss =
    { NULL, "CheckingPowerLoss", 0, &M::OnCheckingPowerLoss, NULL };

/* Suspended, port status requests fail with a hardware error */
const M::State M::Asleep =
    { NULL, "Asleep", HSM_REQ, &M::OnAsleep, NULL };
const M::State M::Suspended =
    { &Asleep, "Suspended", 0, &M::OnSuspended, NULL };
const M::State M::RecycleSuspended =
    { &Asleep, "RecycleSuspended", 0, &M::OnRecycleSuspended, NULL };
const M::State M::SuspendedResetPending =
    { &Asleep, "SuspendedResetPending", 0,
      &M::OnSuspendedResetPending, &M::EnterSuspendedResetPending };
const M::State M::AwaitingStopAfterSuspend =
    { NULL, "AwaitingStopAfterSuspend", 0, &M::OnAwaitingStopAfterSuspend, NULL };

/* Called from Configuring and Reconfiguring for external hubs */
const M::State M::ReadingHubDescriptor =
    { NULL, "ReadingHubDescriptor", 0, &M::OnReadingHubDescriptor, &M::EnterReadingHubDescriptor };
const M::State M::AwaitingDescriptorRetry =
    { NULL, "AwaitingDescriptorRetry", 0,
      &M::OnAwaitingDescriptorRetry, &M::EnterAwaitingDescriptorRetry };
const M::State M::ReadingStandardStatus =
    { NULL, "ReadingStandardStatus", 0,
      &M::OnReadingStandardStatus, &M::EnterReadingStandardStatus };
const M::State M::ReadingConfigDescriptor =
    { NULL, "ReadingConfigDescriptor", 0,
      &M::OnReadingConfigDescriptor, &M::EnterReadingConfigDescriptor };
const M::State M::UpdatingController =
    { NULL, "UpdatingController", 0, &M::OnUpdatingController, &M::EnterUpdatingController };
const M::State M::SettingConfiguration =
    { NULL, "SettingConfiguration", 0,
      &M::OnSettingConfiguration, &M::EnterSettingConfiguration };
const M::State M::FinishingConfiguration =
    { NULL, "FinishingConfiguration", 0,
      &M::OnFinishingConfiguration, &M::EnterFinishingConfiguration };

/* Called from Configuring for the root hub */
const M::State M::ReadingRootHubInfo =
    { NULL, "ReadingRootHubInfo", 0, &M::OnReadingRootHubInfo, &M::EnterReadingRootHubInfo };
const M::State M::ReadingRootHub20Ports =
    { NULL, "ReadingRootHub20Ports", 0,
      &M::OnReadingRootHub20Ports, &M::EnterReadingRootHub20Ports };
const M::State M::ReadingRootHub30Ports =
    { NULL, "ReadingRootHub30Ports", 0,
      &M::OnReadingRootHub30Ports, &M::EnterReadingRootHub30Ports };

/* Called for USB 3 hubs after configuration and after a reset */
const M::State M::ProgrammingHubDepth =
    { NULL, "ProgrammingHubDepth", 0, &M::OnProgrammingHubDepth, &M::EnterProgrammingHubDepth };

#undef HSM_REQ
#undef HSM_PASSIVE
#undef HSM_CRIT

/* FUNCTIONS ******************************************************************/

VOID
HubMachine::Initialize(
    _In_ HubFdo* Hub,
    _In_ HubKind Kind)
{
    m_Hub = Hub;
    m_Kind = Kind;
    m_Teardown = Teardown::Recover;
    m_PowerUp = PowerUp::Start;
    m_StatusRead = StatusRead::Change;
    m_PortStatusReturn = &NotListening;
    m_ReleaseToUnconfigured = FALSE;
    m_FullConfigRead = FALSE;
    m_DromPollStart = 0;
    m_DromIndex = 0;
    m_DromWords = 0;
    m_DromPhase = DromPhase::Header;
    m_DromNext = &QueryingErrata;

    SmInitialize(&AwaitingAddDevice);
}

VOID
HubMachine::SmReference()
{
    ReferenceHub();
}

VOID
HubMachine::SmDereference()
{
    DereferenceHub();
}

VOID
HubMachine::SmQueuePassive()
{
    QueuePassiveWork();
}

/* Start up *******************************************************************/

SM_RESULT
HubMachine::OnAwaitingAddDevice(
    _In_ HubEvent Event)
{
    if (Event != HubEvent::DeviceAdded)
        return SmUnhandled();

    SignalPnpEvent();
    return SmTransition(&Unconfigured);
}

SM_RESULT
HubMachine::OnUnconfigured(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::PrepareHardware:
            /* The root hub has no parent to ask */
            if (m_Kind == HubKind::Root)
                return SmTransition(&Configuring);

            return SmTransition(&QueryingParentInfo);

        case HubEvent::DeviceCleanup:
            return SmTransition(&Deleted);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
HubMachine::EnterQueryingParentInfo()
{
    GetParentInfo();
    return SmHandled();
}

SM_RESULT
HubMachine::OnQueryingParentInfo(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::IoctlDone:
            return SmTransition(&Configuring);

        case HubEvent::IoctlFailed:
            return FailStart(TRUE);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
HubMachine::EnterConfiguring()
{
    QueryControllerInfo();

    if (m_Kind == HubKind::Root)
        return SmCall(&ReadingRootHubInfo);

    return StartHubConfiguration();
}

SM_RESULT
HubMachine::OnConfiguring(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::CallSucceeded:
            return StartDromOrSkip(&QueryingErrata);

        case HubEvent::CallFailed:
            return FailStart(TRUE);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
HubMachine::EnterReconfiguring()
{
    if (m_Kind == HubKind::Root)
        return SmTransition(&AwaitingD0);

    return StartHubConfiguration();
}

SM_RESULT
HubMachine::OnReconfiguring(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        /* A restart reads the DROM again, as the first start does */
        case HubEvent::CallSucceeded:
            return StartDromOrSkip(&AwaitingD0);

        case HubEvent::CallFailed:
            return FailStart(FALSE);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
HubMachine::StartHubConfiguration()
{
    InitializeDescriptorRetries();
    return SmCall(&ReadingHubDescriptor);
}

/**
 * @brief
 * A failed first start returns to Unconfigured, a failed restart to Stopped.
 */
SM_RESULT
HubMachine::FailStart(
    _In_ BOOLEAN FirstStart)
{
    SignalPnpFailure();
    m_ReleaseToUnconfigured = FirstStart;
    return SmTransition(&AwaitingReleaseAfterFailure);
}

SM_RESULT
HubMachine::EnterQueryingErrata()
{
    QueryErrataFlags();

    if (!CreateChildPorts())
        return FailStart(TRUE);

    return SmTransition(&AwaitingD0);
}

SM_RESULT
HubMachine::EnterSignalPnp()
{
    SignalPnpEvent();
    return SmHandled();
}

SM_RESULT
HubMachine::OnAwaitingD0(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::PortStatusRequest:
            FailPortStatus(FALSE);
            return SmHandled();

        case HubEvent::D0Entry:
            AcquirePowerReference();
            return StartPowerUp(PowerUp::Start);

        case HubEvent::ReleaseHardware:
            SignalPnpEvent();
            return SmTransition(&Stopped);

        default:
            return SmUnhandled();
    }
}

/* USB4 DROM ******************************************************************/

/**
 * @brief
 * Reads the router DROM through the vendor mailbox; no failure here fails the start.
 */
SM_RESULT
HubMachine::StartDromOrSkip(
    _In_ const State* Next)
{
    m_DromNext = Next;

    if (!IsDromReadWanted())
        return SmTransition(Next);

    m_DromPhase = DromPhase::Header;
    m_DromIndex = 0;
    m_DromWords = HubDromHeaderWords;
    UseDromHeaderBuffer();

    return SmTransition(&DromReadingFirstStatus);
}

SM_RESULT
HubMachine::OnDromRead(
    _In_ HubEvent Event)
{
    if (Event == HubEvent::TransferFailed)
        return FinishDrom();

    return SmUnhandled();
}

/* QUIRK: this status read is only traced, so its failure is ignored */
SM_RESULT
HubMachine::EnterDromReadingFirstStatus()
{
    ReadDromRegister(HubMailboxOpcode);
    return SmHandled();
}

SM_RESULT
HubMachine::OnDromReadingFirstStatus(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::TransferDone:
            DPRINT("Hub %p router mailbox status 0x%02lx\n", m_Hub, (MailboxValue() >> 24) & 0x3F);
            return SmTransition(&DromWritingMetadata);

        case HubEvent::TransferFailed:
            return SmTransition(&DromWritingMetadata);

        default:
            return SmUnhandled();
    }
}

/* Metadata: first DWORD of the window at bits 14:2, DWORD count at bits 19:15 */
SM_RESULT
HubMachine::EnterDromWritingMetadata()
{
    ULONG Count = min(HubMailboxDataWords, m_DromWords - m_DromIndex);
    ULONG Metadata = ((m_DromIndex & 0x1FFF) << 2) | ((Count & 0x1F) << 15);

    WriteDromRegister(HubMailboxMetadata, Metadata);
    return SmHandled();
}

SM_RESULT
HubMachine::OnDromWritingMetadata(
    _In_ HubEvent Event)
{
    if (Event == HubEvent::TransferDone)
        return SmTransition(&DromSendingCommand);

    return SmUnhandled();
}

SM_RESULT
HubMachine::EnterDromSendingCommand()
{
    WriteDromRegister(HubMailboxOpcode, HubDromReadCommand);
    return SmHandled();
}

SM_RESULT
HubMachine::OnDromSendingCommand(
    _In_ HubEvent Event)
{
    if (Event != HubEvent::TransferDone)
        return SmUnhandled();

    m_DromPollStart = KeQueryInterruptTime();
    return SmTransition(&DromReadingStatus);
}

SM_RESULT
HubMachine::EnterDromReadingStatus()
{
    ReadDromRegister(HubMailboxOpcode);
    return SmHandled();
}

SM_RESULT
HubMachine::OnDromReadingStatus(
    _In_ HubEvent Event)
{
    if (Event == HubEvent::TransferDone)
        return CheckDromStatus();

    return SmUnhandled();
}

/* QUIRK: only the busy bit is checked; a refused command reads whatever the data registers hold */
SM_RESULT
HubMachine::CheckDromStatus()
{
    if (!(MailboxValue() & HubMailboxBusy))
        return SmTransition(&DromReadingWord);

    if (KeQueryInterruptTime() - m_DromPollStart <= HubDromBusyLimit)
        return SmTransition(&DromReadingStatus);

    DPRINT1("Hub %p router still busy after 5 s, DROM read stopped at DWORD %lu\n", m_Hub, m_DromIndex);
    return FinishDrom();
}

/**
 * @brief
 * The DWORD count is rounded up from the total size, not from the length field.
 */
SM_RESULT
HubMachine::StartDromWords()
{
    ULONG DataLength = DromDataLength();
    ULONG TotalBytes;

    if (DataLength < HubDromMinimumLength)
    {
        DPRINT1("Hub %p router DROM length %lu is too short\n", m_Hub, DataLength);
        return FinishDrom();
    }

    TotalBytes = DataLength + HubDromLengthBias;
    if (!AllocateDromBuffer(TotalBytes))
        return FinishDrom();

    m_DromPhase = DromPhase::Full;
    m_DromIndex = 0;
    m_DromWords = (TotalBytes + sizeof(ULONG) - 1) / sizeof(ULONG);
    return SmTransition(&DromWritingMetadata);
}

SM_RESULT
HubMachine::EnterDromReadingWord()
{
    ReadDromRegister(static_cast<USHORT>(m_DromIndex % HubMailboxDataWords));
    return SmHandled();
}

SM_RESULT
HubMachine::OnDromReadingWord(
    _In_ HubEvent Event)
{
    if (Event == HubEvent::TransferDone)
        return StoreDromWordAndContinue();

    return SmUnhandled();
}

/* Each window starts at a multiple of 16, so the data register is the slot in the window */
SM_RESULT
HubMachine::StoreDromWordAndContinue()
{
    StoreDromWord(m_DromIndex);
    m_DromIndex += 1;

    if ((m_DromIndex < m_DromWords) && (m_DromIndex % HubMailboxDataWords) != 0)
        return SmTransition(&DromReadingWord);

    if (m_DromPhase == DromPhase::Header)
        return StartDromWords();

    if (m_DromIndex < m_DromWords)
        return SmTransition(&DromWritingMetadata);

    return FinishDrom();
}

/* Every way out of the read ends here; only a complete full pass is parsed */
SM_RESULT
HubMachine::FinishDrom()
{
    if (m_DromPhase == DromPhase::Full && m_DromIndex >= m_DromWords)
        ParseDromIdentity();

    ReleaseDromBuffer();
    m_DromPhase = DromPhase::Header;
    m_DromIndex = 0;
    m_DromWords = 0;
    return SmTransition(m_DromNext);
}

/* Not running ****************************************************************/

SM_RESULT
HubMachine::OnDown(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::PortStatusRequest:
            FailPortStatus(FALSE);
            return SmHandled();

        case HubEvent::ResetRequested:
            return SmHandled();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
HubMachine::OnStopped(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::PrepareHardware:
            return SmTransition(&Reconfiguring);

        case HubEvent::DeviceCleanup:
            return SmTransition(&Deleted);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
HubMachine::OnAwaitingReleaseHardware(
    _In_ HubEvent Event)
{
    if (Event != HubEvent::ReleaseHardware)
        return SmUnhandled();

    SignalPnpEvent();
    return SmTransition(&Stopped);
}

SM_RESULT
HubMachine::OnAwaitingReleaseAfterFailure(
    _In_ HubEvent Event)
{
    if (Event != HubEvent::ReleaseHardware)
        return SmUnhandled();

    SignalPnpEvent();
    return SmTransition(m_ReleaseToUnconfigured ? &Unconfigured : &Stopped);
}

/* Failure reported to PnP, waiting for the stop */
SM_RESULT
HubMachine::OnAwaitingStop(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::D0Exit:
            ReleasePowerReference();
            SignalPnpEvent();
            return SmTransition(&RecycleSuspended);

        case HubEvent::D0ExitFinal:
            ReleasePowerReference();
            QueueStopToDevices();
            m_Teardown = Teardown::StopAfterReset;
            return SmTransition(&DevicesReleasing);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
HubMachine::OnSurpriseRemoved(
    _In_ HubEvent Event)
{
    if (Event == HubEvent::DeviceCleanup)
        return SmTransition(&Deleted);

    return SmUnhandled();
}

/* Power up *******************************************************************/

SM_RESULT
HubMachine::StartPowerUp(
    _In_ PowerUp Kind)
{
    switch (Kind)
    {
        case PowerUp::Resume:
            QueueResumeToPorts();
            break;

        case PowerUp::ResumeInS0:
            QueueResumeInS0ToPorts();
            break;

        case PowerUp::ResumeWithReset:
            LogResetOnResume();
            QueueResumeWithResetToPorts();
            break;

        default:
            AllowHubReset();
            QueueStartToPorts();
            break;
    }

    m_PowerUp = Kind;
    return SmTransition(&PortsAcquiring);
}

SM_RESULT
HubMachine::OnPortsAcquiring(
    _In_ HubEvent Event)
{
    if (Event != HubEvent::PortsAcquired)
        return SmUnhandled();

    switch (m_PowerUp)
    {
        case PowerUp::PostReset:
            return ReadStatus(StatusRead::LostChanges);

        case PowerUp::Resume:
            QueueResumeToDevices();
            SignalPnpEvent();
            break;

        case PowerUp::ResumeInS0:
            QueueResumeInS0ToDevices();
            break;

        case PowerUp::ResumeWithReset:
            QueueResumeWithResetToDevices();
            SignalPnpEvent();
            break;

        default:
            QueueStartToDevices();
            break;
    }

    return SmTransition(&DevicesAcquiring);
}

SM_RESULT
HubMachine::OnDevicesAcquiring(
    _In_ HubEvent Event)
{
    if (Event != HubEvent::DevicesAcquired)
        return SmUnhandled();

    switch (m_PowerUp)
    {
        case PowerUp::Start:
            SignalPnpEvent();
            return ReadStatus(StatusRead::LostChanges);

        case PowerUp::ResumeInS0:
            QueueFakeStatusChangeToPorts();
            SignalPnpEvent();
            return SmTransition(&NotListening);

        case PowerUp::ResumeResetFailed:
            QueueStopToPorts();
            m_Teardown = Teardown::ResumeFailed;
            return SmTransition(&PortsReleasing);

        default:
            return ReadStatus(StatusRead::LostChanges);
    }
}

/* Running ********************************************************************/

SM_RESULT
HubMachine::OnListening(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::ResetRequested:
            AcquirePowerReference();
            return QuiesceWithInterrupt(Teardown::Recover);

        case HubEvent::D0Exit:
            return QuiesceWithInterrupt(Teardown::Suspend);

        case HubEvent::D0ExitFinal:
            return QuiesceWithInterrupt(Teardown::Stop);

        case HubEvent::InterruptDone:
            AcquirePowerReference();
            switch (CheckHubChangeBit())
            {
                case HubCheck::Yes:
                    return ReadStatus(StatusRead::Change);

                case HubCheck::Error:
                    return HandleHubStatusFault();

                default:
                    QueueStatusChangeToPorts();
                    return SmTransition(&NotListening);
            }

        case HubEvent::InterruptFailed:
            AcquirePowerReference();
            if (IsDepthZero() || PipeResetLimitReached())
                return QuiesceWithoutInterrupt(Teardown::Recover);

            ResetStatusChangePipe();
            return SmTransition(&ResettingPipe);

        case HubEvent::PortStatusRequest:
            return ReadPortStatus(&Listening);

        default:
            return SmUnhandled();
    }
}

/* The ports hold the interrupt transfer back until they ask for it again */
SM_RESULT
HubMachine::OnNotListening(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::ResetRequested:
            m_Teardown = Teardown::Recover;
            return SmTransition(&AwaitingInterruptEnable);

        case HubEvent::D0Exit:
            ReleasePowerReference();
            m_Teardown = Teardown::Suspend;
            return SmTransition(&AwaitingInterruptEnable);

        case HubEvent::D0ExitFinal:
            ReleasePowerReference();
            m_Teardown = Teardown::Stop;
            return SmTransition(&AwaitingInterruptEnable);

        case HubEvent::PortsEnableInterrupt:
            return SendInterrupt(TRUE);

        case HubEvent::PortStatusRequest:
            return ReadPortStatus(&NotListening);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
HubMachine::ReadPortStatus(
    _In_ const State* ReturnTo)
{
    GetPortStatus();
    m_PortStatusReturn = ReturnTo;
    return SmTransition(&ReadingPortStatus);
}

SM_RESULT
HubMachine::OnReadingPortStatus(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::TransferDone:
            ReplyPortStatus();
            return SmTransition(m_PortStatusReturn);

        case HubEvent::TransferFailed:
            FailPortStatus(FALSE);
            return SmTransition(m_PortStatusReturn);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
HubMachine::SendInterrupt(
    _In_ BOOLEAN ClearPipeResets)
{
    ReleasePowerReference();
    StartStatusChangeRead();
    if (ClearPipeResets)
        ClearPipeResetCount();

    return SmTransition(&Listening);
}

SM_RESULT
HubMachine::ReadStatus(
    _In_ StatusRead Purpose)
{
    GetHubStatus();
    m_StatusRead = Purpose;
    return SmTransition(&ReadingHubStatus);
}

SM_RESULT
HubMachine::OnReadingHubStatus(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::TransferDone:
            if (m_StatusRead == StatusRead::LostChanges)
                return HandleHubChange(LostHubChange());

            if (m_StatusRead == StatusRead::OverCurrent)
            {
                if (OverCurrentCleared())
                    return QuiesceWithoutInterrupt(Teardown::Recover);

                return OverCurrentError();
            }

            switch (SelectHubChange())
            {
                case HubCheck::Yes:
                    AckHubChange();
                    return SmTransition(&AckingHubChange);

                case HubCheck::Error:
                    return HandleHubStatusFault();

                default:
                    return SendInterrupt(TRUE);
            }

        case HubEvent::TransferFailed:
            if (m_StatusRead == StatusRead::OverCurrent)
                return OverCurrentError();

            return QuiesceWithoutInterrupt(Teardown::Recover);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
HubMachine::OnAckingHubChange(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::TransferDone:
            return HandleHubChange(PendingHubChange());

        case HubEvent::TransferFailed:
            return QuiesceWithoutInterrupt(Teardown::Recover);

        default:
            return SmUnhandled();
    }
}

/**
 * @brief
 * None comes only from the lost change query, Error only from the pending one.
 */
SM_RESULT
HubMachine::HandleHubChange(
    _In_ HubChange Change)
{
    switch (Change)
    {
        case HubChange::OverCurrent:
            ArmOverCurrentTimer();
            return SmTransition(&AwaitingOverCurrentTimer);

        case HubChange::OverCurrentCleared:
            return QuiesceWithoutInterrupt(Teardown::Recover);

        case HubChange::Error:
            return HandleHubStatusFault();

        default:
            return SendInterrupt(TRUE);
    }
}

SM_RESULT
HubMachine::HandleHubStatusFault()
{
    if (IsErrorFatal())
        return QuiesceWithoutInterrupt(Teardown::Recover);

    return SendInterrupt(TRUE);
}

SM_RESULT
HubMachine::OverCurrentError()
{
    if (IsErrorFatal())
        return QuiesceWithoutInterrupt(Teardown::Fatal);

    return QuiesceWithoutInterrupt(Teardown::Recover);
}

SM_RESULT
HubMachine::OnAwaitingOverCurrentTimer(
    _In_ HubEvent Event)
{
    if (Event == HubEvent::TimerFired)
        return ReadStatus(StatusRead::OverCurrent);

    return SmUnhandled();
}

SM_RESULT
HubMachine::OnResettingPipe(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::PipeResetDone:
            return SendInterrupt(FALSE);

        case HubEvent::PipeResetFailed:
            return QuiesceWithoutInterrupt(Teardown::Recover);

        default:
            return SmUnhandled();
    }
}

/* Tear down ******************************************************************/

/* The interrupt transfer is still pending */
SM_RESULT
HubMachine::QuiesceWithInterrupt(
    _In_ Teardown Reason)
{
    BOOLEAN released = PortInterruptRefsReleased();

    if (Reason == Teardown::Recover)
        QueueSurpriseRemovalToPorts();

    m_Teardown = Reason;

    if (!released)
        return SmTransition(&DrainingInterruptRefs);

    CancelStatusChangeRead();
    return SmTransition(&CancelingInterrupt);
}

/* No interrupt transfer is pending */
SM_RESULT
HubMachine::QuiesceWithoutInterrupt(
    _In_ Teardown Reason)
{
    if ((Reason == Teardown::Recover) || (Reason == Teardown::Fatal))
        QueueSurpriseRemovalToPorts();

    m_Teardown = Reason;

    if (!PortInterruptRefsReleased())
        return SmTransition(&AwaitingInterruptRefs);

    return AfterInterruptQuiet();
}

SM_RESULT
HubMachine::AfterInterruptQuiet()
{
    switch (m_Teardown)
    {
        case Teardown::Stop:
            QueueStopToDevices();
            return SmTransition(&DevicesReleasing);

        case Teardown::Suspend:
            QueueSuspendToDevices();
            return SmTransition(&DevicesReleasing);

        default:
            QueueStopToPorts();
            return SmTransition(&PortsReleasing);
    }
}

SM_RESULT
HubMachine::OnDrainingInterruptRefs(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::InterruptDone:
        case HubEvent::InterruptFailed:
            return SmTransition(&AwaitingInterruptRefs);

        case HubEvent::PortsInterruptReleased:
            CancelStatusChangeRead();
            return SmTransition(&CancelingInterrupt);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
HubMachine::OnCancelingInterrupt(
    _In_ HubEvent Event)
{
    if ((Event == HubEvent::InterruptDone) || (Event == HubEvent::InterruptFailed))
        return AfterInterruptQuiet();

    return SmUnhandled();
}

SM_RESULT
HubMachine::OnAwaitingInterruptRefs(
    _In_ HubEvent Event)
{
    if (Event == HubEvent::PortsInterruptReleased)
        return AfterInterruptQuiet();

    return SmUnhandled();
}

SM_RESULT
HubMachine::OnAwaitingInterruptEnable(
    _In_ HubEvent Event)
{
    if (Event == HubEvent::PortsEnableInterrupt)
        return QuiesceWithoutInterrupt(m_Teardown);

    return SmUnhandled();
}

SM_RESULT
HubMachine::OnDevicesReleasing(
    _In_ HubEvent Event)
{
    if (Event != HubEvent::DevicesReleased)
        return SmUnhandled();

    switch (m_Teardown)
    {
        case Teardown::Stop:
            QueueStopToPorts();
            return SmTransition(&PortsReleasing);

        case Teardown::Suspend:
            QueueSuspendToPorts();
            return SmTransition(&PortsReleasing);

        default:
            return SmTransition(&AwaitingReleaseHardware);
    }
}

SM_RESULT
HubMachine::OnPortsReleasing(
    _In_ HubEvent Event)
{
    if (Event != HubEvent::PortsReleased)
        return SmUnhandled();

    switch (m_Teardown)
    {
        case Teardown::Stop:
            return SmTransition(&AwaitingReleaseHardware);

        case Teardown::Suspend:
            SignalPnpEvent();
            return SmTransition(&Suspended);

        default:
            QueueResetToPorts();
            return SmTransition(&PortsResetting);
    }
}

SM_RESULT
HubMachine::OnPortsResetting(
    _In_ HubEvent Event)
{
    if (Event != HubEvent::PortsResetReleased)
        return SmUnhandled();

    switch (m_Teardown)
    {
        case Teardown::Fatal:
            return SmTransition(&ReportingFailure);

        case Teardown::ResumeFailed:
            SignalPnpEvent();
            InitializeResetCount();
            return SmTransition(&HubResetting);

        default:
            return SmTransition(&CheckingRecoveryLimit);
    }
}

SM_RESULT
HubMachine::EnterCheckingRecoveryLimit()
{
    if (!RecoveryLimitReached())
    {
        InitializeResetCount();
        return SmTransition(&HubResetting);
    }

    if (IsInBootPath())
        return SmTransition(&Halted);

    return SmTransition(&ReportingFailure);
}

SM_RESULT
HubMachine::EnterReportingFailure()
{
    ReportFailureToPnp();
    return SmTransition(&AwaitingStop);
}

SM_RESULT
HubMachine::EnterHalted()
{
    BugcheckBootHub();
    return SmHandled();
}

/* Hub reset ******************************************************************/

SM_RESULT
HubMachine::EnterHubResetting()
{
    ResetHub();
    return SmHandled();
}

SM_RESULT
HubMachine::OnHubResetting(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::ResetDone:
            return SmTransition(&ReconfiguringAfterReset);

        case HubEvent::ResetFailed:
            return SmTransition(&LoggingReset);

        case HubEvent::ResetFailedOnRemoval:
            ArmResetRetryTimer();
            return SmTransition(&AwaitingResetRetry);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
HubMachine::EnterLoggingReset()
{
    LogResetRecovery();
    ArmResetRetryTimer();
    return SmTransition(&AwaitingResetRetry);
}

/* Requests never run here; the handling only mirrors the previous table */
SM_RESULT
HubMachine::OnAwaitingResetRetry(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::TimerFired:
            if (ResetLimitReached())
                return SmTransition(&ReportingFailure);

            return SmTransition(&HubResetting);

        case HubEvent::PortStatusRequest:
            FailPortStatus(TRUE);
            return SmHandled();

        case HubEvent::ResetRequested:
            return SmHandled();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
HubMachine::CallHubDepthOr(
    _In_ PowerUp Kind)
{
    if (m_Kind == HubKind::Usb30)
        return SmCall(&ProgrammingHubDepth);

    return StartPowerUp(Kind);
}

SM_RESULT
HubMachine::EnterReconfiguringAfterReset()
{
    LogResetRecovery();
    return CallHubDepthOr(PowerUp::PostReset);
}

SM_RESULT
HubMachine::OnReconfiguringAfterReset(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::CallSucceeded:
            return StartPowerUp(PowerUp::PostReset);

        case HubEvent::CallFailed:
            return QuiesceWithoutInterrupt(Teardown::Recover);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
HubMachine::OnResettingOnResume(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::ResetDone:
            return SmTransition(&ReconfiguringAfterResume);

        case HubEvent::ResetFailed:
        case HubEvent::ResetFailedOnRemoval:
            return ResumeResetFailed();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
HubMachine::ResumeResetFailed()
{
    LogResetOnResume();
    QueueResumeWithResetToDevices();
    m_PowerUp = PowerUp::ResumeResetFailed;
    return SmTransition(&DevicesAcquiring);
}

SM_RESULT
HubMachine::EnterReconfiguringAfterResume()
{
    return CallHubDepthOr(PowerUp::ResumeWithReset);
}

SM_RESULT
HubMachine::OnReconfiguringAfterResume(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::CallSucceeded:
            return StartPowerUp(PowerUp::ResumeWithReset);

        case HubEvent::CallFailed:
            return ResumeResetFailed();

        default:
            return SmUnhandled();
    }
}

/* External hubs read the hub descriptor to see if power was lost while in Sx */
SM_RESULT
HubMachine::CheckPowerLoss()
{
    if (m_Kind == HubKind::Root)
        return StartPowerUp(PowerUp::Resume);

    GetHubDescriptor();
    return SmTransition(&CheckingPowerLoss);
}

SM_RESULT
HubMachine::OnCheckingPowerLoss(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::TransferDone:
            return StartPowerUp(PowerUp::Resume);

        case HubEvent::TransferFailed:
            return SmTransition(&ResettingOnResume);

        default:
            return SmUnhandled();
    }
}

/* Suspended ******************************************************************/

SM_RESULT
HubMachine::OnAsleep(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::PortStatusRequest:
            FailPortStatus(TRUE);
            return SmHandled();

        case HubEvent::ResetRequested:
            return SmHandled();

        case HubEvent::ReleaseHardware:
            QueueStopAfterSuspendToDevices();
            return SmTransition(&AwaitingStopAfterSuspend);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
HubMachine::OnSuspended(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::ResetRequested:
            return SmTransition(&SuspendedResetPending);

        case HubEvent::PortStatusRequest:
            ReplyPortStatus();
            return SmHandled();

        case HubEvent::D0Entry:
            AcquirePowerReference();
            return StartPowerUp(PowerUp::ResumeInS0);

        case HubEvent::D0EntryFromSx:
            AcquirePowerReference();
            if (WasResetByParent())
                return SmTransition(&ReconfiguringAfterResume);

            return CheckPowerLoss();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
HubMachine::OnRecycleSuspended(
    _In_ HubEvent Event)
{
    if ((Event != HubEvent::D0Entry) && (Event != HubEvent::D0EntryFromSx))
        return SmUnhandled();

    AcquirePowerReference();
    SignalPnpEvent();
    return SmTransition(&AwaitingStop);
}

SM_RESULT
HubMachine::EnterSuspendedResetPending()
{
    AcquirePowerReference();
    return SmHandled();
}

/* The hub is reset on resume either way; the query only keeps the parent in step */
SM_RESULT
HubMachine::OnSuspendedResetPending(
    _In_ HubEvent Event)
{
    if ((Event != HubEvent::D0Entry) && (Event != HubEvent::D0EntryFromSx))
        return SmUnhandled();

    (VOID)WasResetByParent();
    return SmTransition(&ResettingOnResume);
}

SM_RESULT
HubMachine::OnAwaitingStopAfterSuspend(
    _In_ HubEvent Event)
{
    if (Event == HubEvent::DevicesStoppedAfterSuspend)
        return SmTransition(&SurpriseRemoved);

    return SmUnhandled();
}

/* Hub configuration (called) *************************************************/

SM_RESULT
HubMachine::EnterReadingHubDescriptor()
{
    GetHubDescriptor();
    return SmHandled();
}

SM_RESULT
HubMachine::OnReadingHubDescriptor(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::TransferDone:
            if (!ParseHubDescriptor())
                return SmReturn(HubEvent::CallFailed);

            return SmTransition(&ReadingStandardStatus);

        case HubEvent::TransferFailed:
            if (DescriptorRetriesExhausted())
                return SmReturn(HubEvent::CallFailed);

            return SmTransition(&AwaitingDescriptorRetry);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
HubMachine::EnterAwaitingDescriptorRetry()
{
    StartDescriptorRetryTimer();
    return SmHandled();
}

SM_RESULT
HubMachine::OnAwaitingDescriptorRetry(
    _In_ HubEvent Event)
{
    if (Event == HubEvent::TimerFired)
        return SmTransition(&ReadingHubDescriptor);

    return SmUnhandled();
}

SM_RESULT
HubMachine::EnterReadingStandardStatus()
{
    GetStandardStatus();
    return SmHandled();
}

SM_RESULT
HubMachine::OnReadingStandardStatus(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::TransferDone:
            if (!ParsePowerStatus())
                return SmReturn(HubEvent::CallFailed);

            m_FullConfigRead = FALSE;
            return SmTransition(&ReadingConfigDescriptor);

        case HubEvent::TransferFailed:
            return SmReturn(HubEvent::CallFailed);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
HubMachine::EnterReadingConfigDescriptor()
{
    GetConfigDescriptor(m_FullConfigRead);
    return SmHandled();
}

/* The first read uses a default length, a second read fetches the whole thing */
SM_RESULT
HubMachine::OnReadingConfigDescriptor(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::TransferDone:
            if (!m_FullConfigRead && ConfigDescriptorTruncated())
            {
                m_FullConfigRead = TRUE;
                return SmTransition(&ReadingConfigDescriptor);
            }

            if (!CacheConfigDescriptor())
                return SmReturn(HubEvent::CallFailed);

            return SmTransition(&UpdatingController);

        case HubEvent::TransferFailed:
            return SmReturn(HubEvent::CallFailed);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
HubMachine::EnterUpdatingController()
{
    UpdateControllerHubInfo();
    return SmHandled();
}

SM_RESULT
HubMachine::OnUpdatingController(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::IoctlDone:
            return SmTransition(&SettingConfiguration);

        case HubEvent::IoctlFailed:
            return SmReturn(HubEvent::CallFailed);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
HubMachine::EnterSettingConfiguration()
{
    SelectHubConfiguration();
    return SmHandled();
}

SM_RESULT
HubMachine::OnSettingConfiguration(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::ConfigurationSet:
            if (m_Kind == HubKind::Usb30)
                return SmTransition(&FinishingConfiguration);

            return SmReturn(HubEvent::CallSucceeded);

        case HubEvent::ConfigurationFailed:
            return SmReturn(HubEvent::CallFailed);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
HubMachine::EnterFinishingConfiguration()
{
    return SmCall(&ProgrammingHubDepth);
}

SM_RESULT
HubMachine::OnFinishingConfiguration(
    _In_ HubEvent Event)
{
    if ((Event == HubEvent::CallSucceeded) || (Event == HubEvent::CallFailed))
        return SmReturn(Event);

    return SmUnhandled();
}

/* Root hub configuration (called) ********************************************/

SM_RESULT
HubMachine::EnterReadingRootHubInfo()
{
    QueryControllerInfo();
    GetRootHubInfo();
    return SmHandled();
}

SM_RESULT
HubMachine::OnReadingRootHubInfo(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::IoctlDone:
            return SmTransition(&ReadingRootHub20Ports);

        case HubEvent::IoctlFailed:
            return SmReturn(HubEvent::CallFailed);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
HubMachine::EnterReadingRootHub20Ports()
{
    GetRootHub20Ports();
    return SmHandled();
}

SM_RESULT
HubMachine::OnReadingRootHub20Ports(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::IoctlDone:
            return SmTransition(&ReadingRootHub30Ports);

        case HubEvent::IoctlFailed:
            return SmReturn(HubEvent::CallFailed);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
HubMachine::EnterReadingRootHub30Ports()
{
    GetRootHub30Ports();
    return SmHandled();
}

SM_RESULT
HubMachine::OnReadingRootHub30Ports(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::IoctlDone:
            return SmReturn(HubEvent::CallSucceeded);

        case HubEvent::IoctlFailed:
            return SmReturn(HubEvent::CallFailed);

        default:
            return SmUnhandled();
    }
}

/* Hub depth (called) *********************************************************/

SM_RESULT
HubMachine::EnterProgrammingHubDepth()
{
    SetHubDepth();
    return SmHandled();
}

SM_RESULT
HubMachine::OnProgrammingHubDepth(
    _In_ HubEvent Event)
{
    switch (Event)
    {
        case HubEvent::TransferDone:
            return SmReturn(HubEvent::CallSucceeded);

        case HubEvent::TransferFailed:
            return SmReturn(HubEvent::CallFailed);

        default:
            return SmUnhandled();
    }
}
