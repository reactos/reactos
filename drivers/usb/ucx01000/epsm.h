/*
 * PROJECT:     ReactOS USB Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Endpoint state machine
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

#include <drivers/usb3/smengine.h>

class UcxEndpoint;

enum class EpEvent : UCHAR
{
    /* Requests from the hub and the client, wait until the endpoint is idle */
    HubStartRequest,
    HubPurge,
    HubTreePurge,
    HubAbort,
    HubDisconnect,
    HubDisable,
    HubDeviceReset,
    DeviceResetDone,
    ConfigureDone,
    HubEndpointReset,
    Delete,
    ClientAbortUrb,
    ClientStreamsEnable,
    ClientStreamsDisable,

    /* Completions of work the machine started */
    AbortUrbParked,
    AbortDone,
    PurgeDone,
    EndpointResetDone,
    StreamsOpened,
    StreamsClosed,
    ControllerResetStarting,
    StaleReplaced,

    /* Posted by the controller once a reset finishes */
    ControllerResetDone,

    Count
};

class EndpointMachine : public SmMachine<EndpointMachine, EpEvent>
{
    friend class SmMachine<EndpointMachine, EpEvent>;

public:
    VOID
    Initialize(
        _In_ UcxEndpoint* Endpoint);

    UcxEndpoint*
    Endpoint() const
    {
        return m_Endpoint;
    }

    static const SM_EVENT_INFO EventInfo[];

private:
    enum PURGE_FOLLOW_UP
    {
        PurgeFollowUpNone,
        PurgeFollowUpHubOperation,
        PurgeFollowUpTreePurge
    };

    /* States */
    static const State Inactive;
    static const State Created;
    static const State Disabled;
    static const State Retired;
    static const State Stale;
    static const State AwaitingDestroy;
    static const State Enabled;
    static const State Busy;
    static const State Purging;
    static const State Operation;
    static const State AwaitingAbortUrb;
    static const State Aborting;
    static const State EnablingStreams;
    static const State DisablingStreams;
    static const State ResettingPipe;
    static const State OperationHeld;
    static const State OperationUnwinding;

    /* Engine hooks */
    BOOLEAN
    SmAccepts(
        _In_ EpEvent Event);

    VOID
    SmReference();

    VOID
    SmDereference();

    /* Handlers */
    SM_RESULT
    OnInactive(
        _In_ EpEvent Event);
    SM_RESULT
    OnCreated(
        _In_ EpEvent Event);
    SM_RESULT
    OnDisabled(
        _In_ EpEvent Event);
    SM_RESULT
    OnRetired(
        _In_ EpEvent Event);
    SM_RESULT
    OnStale(
        _In_ EpEvent Event);
    SM_RESULT
    OnEnabled(
        _In_ EpEvent Event);
    SM_RESULT
    OnBusy(
        _In_ EpEvent Event);
    SM_RESULT
    OnPurging(
        _In_ EpEvent Event);
    SM_RESULT
    OnOperation(
        _In_ EpEvent Event);
    SM_RESULT
    OnAwaitingAbortUrb(
        _In_ EpEvent Event);
    SM_RESULT
    OnAborting(
        _In_ EpEvent Event);
    SM_RESULT
    OnEnablingStreams(
        _In_ EpEvent Event);
    SM_RESULT
    OnDisablingStreams(
        _In_ EpEvent Event);
    SM_RESULT
    OnResettingPipe(
        _In_ EpEvent Event);
    SM_RESULT
    OnOperationHeld(
        _In_ EpEvent Event);
    SM_RESULT
    OnOperationUnwinding(
        _In_ EpEvent Event);

    /* Entry actions */
    SM_RESULT EnterPurging();
    SM_RESULT EnterAborting();
    SM_RESULT EnterEnablingStreams();
    SM_RESULT EnterDisablingStreams();
    SM_RESULT EnterResettingPipe();
    SM_RESULT EnterOperationHeld();

    /* Helpers */
    SM_RESULT
    PauseForControllerReset();

    SM_RESULT
    StartPurge(
        _In_ PURGE_FOLLOW_UP FollowUp);

    SM_RESULT
    BeginOperation(
        _In_ const State* Target,
        _In_ const State* ReturnTo);

    SM_RESULT
    TryStart();

    SM_RESULT
    Retire();

    BOOLEAN
    CanStart();

    /* Actions and queries, implemented by the endpoint object */
    VOID ReferenceEndpoint();
    VOID DereferenceEndpoint();
    VOID StartEndpoint();
    VOID PurgeEndpoint();
    VOID AbortEndpoint();
    VOID ForwardResetRequest();
    VOID FailResetRequest();
    VOID FinishEndpointResetRequest();
    VOID ForwardStreamsEnableRequest();
    VOID RejectStreamsEnableRequest();
    VOID
    FinishStreamsOpenRequest(
        _In_ BOOLEAN ForceFailure);
    VOID ForwardStreamsDisableRequest();
    VOID ParkStreamsDisableRequest();
    VOID FinishStreamsCloseRequest();
    VOID CompleteAbortUrb();
    VOID CompleteHubOperation();
    VOID CompleteTreePurge();
    VOID AckControllerReset();
    VOID DeleteEndpoint();
    BOOLEAN ParkAsStale();
    VOID DeleteStaleEndpoint();
    BOOLEAN DeviceAllowsStart();
    BOOLEAN DeviceDeprogrammed();
    BOOLEAN DeviceDisconnected();
    BOOLEAN ClientHoldsHandle();

    UcxEndpoint* m_Endpoint;
    const State* m_ReturnTo;
    const State* m_HeldOperation;
    PURGE_FOLLOW_UP m_AfterPurge;
    BOOLEAN m_Enabled;
    BOOLEAN m_AbortForHub;
    BOOLEAN m_PauseAfterPurge;
};
