/*
 * PROJECT:     ReactOS USB Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Controller reset state machine
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include <drivers/usb3/smengine.h>

class UcxController;

enum class CrEvent : UCHAR
{
    /* Root hub and failure notifications, wait until the machine is idle */
    ControllerLost,
    HubRequestsReset,
    RootHubPoweredUp,
    RootHubPoweringDown,

    /* From the controller driver, a second copy is refused while one is queued */
    ControllerNeedsReset,
    ControllerResetDone,

    /* Completions of work the machine started */
    ResetBlocksCleared,
    DevicesReady,

    Count
};

class ControllerResetMachine : public SmMachine<ControllerResetMachine, CrEvent>
{
    friend class SmMachine<ControllerResetMachine, CrEvent>;

public:
    VOID
    Initialize(
        _In_ UcxController* Controller);

    UcxController*
    Controller() const
    {
        return m_Controller;
    }

    static const SM_EVENT_INFO EventInfo[];

private:
    /* States */
    static const State RootHubOn;
    static const State RootHubOff;
    static const State PreparingDevicesOff;
    static const State Resetting;
    static const State DrainingReferences;
    static const State PreparingDevices;
    static const State ResettingController;
    static const State AwaitingResetDone;
    static const State AwaitingHubReset;
    static const State Failed;
    static const State FailedOn;
    static const State FailedOff;

    /* Engine hooks */
    VOID
    SmReference();

    VOID
    SmDereference();

    VOID
    SmQueuePassive();

    /* Handlers */
    SM_RESULT
    OnRootHubOn(
        _In_ CrEvent Event);
    SM_RESULT
    OnRootHubOff(
        _In_ CrEvent Event);
    SM_RESULT
    OnPreparingDevicesOff(
        _In_ CrEvent Event);
    SM_RESULT
    OnResetting(
        _In_ CrEvent Event);
    SM_RESULT
    OnDrainingReferences(
        _In_ CrEvent Event);
    SM_RESULT
    OnPreparingDevices(
        _In_ CrEvent Event);
    SM_RESULT
    OnResettingController(
        _In_ CrEvent Event);
    SM_RESULT
    OnAwaitingResetDone(
        _In_ CrEvent Event);
    SM_RESULT
    OnAwaitingHubReset(
        _In_ CrEvent Event);
    SM_RESULT
    OnFailed(
        _In_ CrEvent Event);
    SM_RESULT
    OnFailedOn(
        _In_ CrEvent Event);
    SM_RESULT
    OnFailedOff(
        _In_ CrEvent Event);

    /* Entry actions */
    SM_RESULT EnterDrainingReferences();
    SM_RESULT EnterPreparingDevices();
    SM_RESULT EnterResettingController();

    /* Helpers */
    SM_RESULT
    StartReset(
        _In_ BOOLEAN ForHub);

    SM_RESULT
    EndResetForHub();

    SM_RESULT
    PowerDown(
        _In_ BOOLEAN ResetSeen);

    /* Actions, implemented by the controller object */
    VOID ReferenceController();
    VOID DereferenceController();
    VOID QueuePassiveWork();
    VOID HoldDeviceRequests();
    VOID ReleaseDeviceRequests();
    VOID MarkResetInProgress();
    VOID MarkResetFinished();
    VOID FailRootHubIo();
    VOID UnblockRootHubTraffic();
    VOID DropResetBlock();
    VOID PrepareDevicesForReset();
    VOID NotifyDevicesResetDone();
    VOID ResetController();
    VOID
    CompleteHubReset(
        _In_ BOOLEAN Succeeded);
    VOID SignalPortChange();
    VOID AllowRootHubPowerDown();
    VOID LetResetCompletionProceed();
    VOID ReleaseResetCompleteWaiter();

    UcxController* m_Controller;
    BOOLEAN m_ForHub;
    BOOLEAN m_ResetPending;
    BOOLEAN m_ResetSeen;
};
