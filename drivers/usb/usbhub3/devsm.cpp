/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Device state machine, event table, engine hooks and shared states
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include "usbhub3.h"

/* GLOBALS ********************************************************************/

const SM_EVENT_INFO DeviceMachine::EventInfo[] =
{
    { "PortAttached",                 SmEventCompletion, FALSE, FALSE },
    { "PortReattached",               SmEventCompletion, FALSE, FALSE },
    { "PortDetached",                 SmEventCompletion, FALSE, FALSE },
    { "PortDisableDone",                 SmEventCompletion, FALSE, FALSE },
    { "PortFault",                   SmEventCompletion, FALSE, FALSE },
    { "PortResetDone",                SmEventCompletion, FALSE, FALSE },
    { "PortResetAbortedForSuspend",   SmEventCompletion, FALSE, FALSE },
    { "PortResetTimeout",             SmEventCompletion, FALSE, FALSE },
    { "PortResumeDone",                  SmEventCompletion, FALSE, FALSE },
    { "PortResumeAbortedForSuspend",  SmEventCompletion, FALSE, FALSE },
    { "PortResumeTimeout",            SmEventCompletion, FALSE, FALSE },
    { "PortSuspendDone",                SmEventCompletion, FALSE, FALSE },
    { "PortNowDisabled",              SmEventCompletion, FALSE, FALSE },
    { "PortNowEnabled",               SmEventCompletion, FALSE, FALSE },
    { "PortNowEnabledOnReconnect",    SmEventCompletion, FALSE, FALSE },
    { "PortNowSuspended",             SmEventCompletion, FALSE, FALSE },
    { "PortTimeoutSet",               SmEventCompletion, FALSE, FALSE },
    { "HubStarted",                   SmEventCompletion, FALSE, FALSE },
    { "HubStopping",                  SmEventCompletion, FALSE, FALSE },
    { "HubStoppingAfterSuspend",      SmEventCompletion, FALSE, FALSE },
    { "HubSuspending",                SmEventCompletion, FALSE, FALSE },
    { "HubResumed",                   SmEventCompletion, FALSE, FALSE },
    { "HubResumedInS0",               SmEventCompletion, FALSE, FALSE },
    { "HubResumedWithReset",          SmEventCompletion, FALSE, FALSE },
    { "TimerFired",                   SmEventCompletion, FALSE, FALSE },
    { "TransferDone",                 SmEventCritical,   FALSE, FALSE },
    { "TransferFailed",               SmEventCritical,   FALSE, FALSE },
    { "TransferStalled",              SmEventCritical,   FALSE, FALSE },
    { "ControllerIoctlDone",          SmEventCritical,   FALSE, FALSE },
    { "ControllerIoctlFailed",        SmEventCritical,   FALSE, FALSE },
    { "ControllerExitLatencyTooLarge", SmEventCritical,  FALSE, FALSE },
    { "ControllerRequestDone",        SmEventCritical,   FALSE, FALSE },
    { "PdoPreStart",                  SmEventRequest,    FALSE, FALSE },
    { "PdoPowerUp",                   SmEventRequest,    FALSE, FALSE },
    { "PdoPowerDown",                 SmEventRequest,    FALSE, FALSE },
    { "PdoPowerDownFinal",            SmEventRequest,    FALSE, FALSE },
    { "PdoPrepareHibernate",          SmEventRequest,    FALSE, FALSE },
    { "PdoInstallMsOsExt",            SmEventRequest,    FALSE, FALSE },
    { "PdoReportedMissing",           SmEventRequest,    FALSE, FALSE },
    { "PdoCleanup",                   SmEventRequest,    FALSE, FALSE },
    { "ClientCyclePort",              SmEventRequest,    FALSE, FALSE },
    { "ClientResetDevice",            SmEventRequest,    FALSE, FALSE },
    { "ClientResetPipe",              SmEventRequest,    FALSE, FALSE },
    { "ClientSyncResetPipe",          SmEventRequest,    FALSE, FALSE },
    { "ClientClearStall",             SmEventRequest,    FALSE, FALSE },
    { "ClientSelectConfig",           SmEventRequest,    FALSE, FALSE },
    { "ClientUnconfigure",            SmEventRequest,    FALSE, FALSE },
    { "ClientSetInterface",           SmEventRequest,    FALSE, FALSE },
    { "ClientStreams",                SmEventRequest,    FALSE, FALSE },
    { "DeviceTextQuery",              SmEventRequest,    FALSE, FALSE },
    { "HubGetDescriptor",             SmEventRequest,    FALSE, FALSE },
    { "LpmSettingChanged",            SmEventRequest,    FALSE, TRUE },
    { "NoPingResponse",               SmEventRequest,    FALSE, TRUE },
    { "Succeeded",                    SmEventCompletion, FALSE, FALSE },
    { "Failed",                       SmEventCompletion, FALSE, FALSE },
    { "FailedDeviceEnabled",          SmEventCompletion, FALSE, FALSE },
    { "DetachedDeviceEnabled",        SmEventCompletion, FALSE, FALSE },
    { "PortOffForHubSuspend",         SmEventCompletion, FALSE, FALSE },
    { "HubStoppedHoldingReference",   SmEventCompletion, FALSE, FALSE },
    { "ResumeDone",                SmEventCompletion, FALSE, FALSE },
    { "ResumedWithHub",               SmEventCompletion, FALSE, FALSE },
    { "SuspendedAfterHubResume",      SmEventCompletion, FALSE, FALSE },
    { "NeedsReenumeration",           SmEventCompletion, FALSE, FALSE },
    { "FailEnumeration",              SmEventCompletion, FALSE, FALSE },
};

static_assert(RTL_NUMBER_OF(DeviceMachine::EventInfo) == static_cast<ULONG>(DsmEvent::Count),
              "EventInfo must have one entry per DsmEvent");

/* Runs the code m_Then names at passive level */
const DeviceMachine::State DeviceMachine::PassiveStep =
    { NULL, "PassiveStep", SM_STATE_NEEDS_PASSIVE, NULL, &DeviceMachine::EnterPassiveStep };

/* Ends the current sub machine with m_Result */
const DeviceMachine::State DeviceMachine::Ending =
    { NULL, "Ending", 0, NULL, &DeviceMachine::EnterEnding };

/* FUNCTIONS ******************************************************************/

VOID
DeviceMachine::Initialize(
    _In_ HubChild* Device)
{
    m_Device = Device;
    m_Next = NULL;
    m_Then = NULL;
    m_Start = NULL;
    m_AfterDelete = NULL;
    m_AfterDisable = NULL;
    m_Resume = NULL;
    m_AfterReenum = NULL;
    m_AfterConfigure = NULL;
    m_AfterProgram = NULL;
    m_AfterPurge = NULL;
    m_ReturnTo = NULL;
    m_Result = DsmEvent::Succeeded;
    m_HubEvent = DsmEvent::PortDetached;
    m_DelayKind = DsmDelayEnumRetry;
    m_ReenumKind = DsmReenumUnconfigured;
    m_ReenumForceReset = FALSE;
    m_DetachEndsReset = FALSE;
    m_ConfigFullLength = FALSE;
    m_FromConfigured = FALSE;
    m_FailedPoweredUp = FALSE;
    m_EndpointsConfigured = FALSE;
    m_ChildForgotten = FALSE;
    m_Unregistered = FALSE;
    m_PortMayResume = FALSE;
    m_D3Cold = FALSE;
    m_ContextGone = FALSE;

    SmInitialize(&AwaitingAttach);
}

VOID
DeviceMachine::SmReference()
{
    ReferenceDevice();
}

VOID
DeviceMachine::SmDereference()
{
    DereferenceDevice();
}

VOID
DeviceMachine::SmQueuePassive()
{
    QueuePassiveWork();
}

BOOLEAN
DeviceMachine::SmCancelTimer()
{
    return CancelStateTimer();
}

/**
 * @brief
 * A device with no version or speed known yet passes for any version or speed.
 */
BOOLEAN
DeviceMachine::KindWithin(
    _In_ ULONG Allowed)
{
    return (DeviceKind() & ~Allowed) == 0;
}

/* Go to a shared wait state; Next gets the event that ends the wait */
SM_RESULT
DeviceMachine::Wait(
    _In_ const State* Target,
    _In_ DSM_NEXT Next)
{
    m_Next = Next;
    return SmTransition(Target);
}

/* Run Then from a state entered at passive level */
SM_RESULT
DeviceMachine::AtPassive(
    _In_ DSM_THEN Then)
{
    m_Then = Then;
    return SmTransition(&PassiveStep);
}

/* End the sub machine of the frame the caller runs in */
SM_RESULT
DeviceMachine::EndWith(
    _In_ DsmEvent Result)
{
    m_Result = Result;
    return SmTransition(&Ending);
}

/* Handler of the shared waits: the event goes to the continuation */
SM_RESULT
DeviceMachine::OnShared(
    _In_ DsmEvent Event)
{
    return (this->*m_Next)(Event);
}

SM_RESULT
DeviceMachine::EnterPassiveStep()
{
    return (this->*m_Then)();
}

SM_RESULT
DeviceMachine::EnterEnding()
{
    return SmReturn(m_Result);
}

/* SHARED WAITS ***************************************************************/

static const DsmEvent HubEventDiscards[] =
{
    DsmEvent::HubStopping, DsmEvent::HubSuspending, DsmEvent::Count
};

/* Asked the port machine to disable the port */
const DeviceMachine::State DeviceMachine::DisablingPort =
    { NULL, "DisablingPort", 0, &DeviceMachine::OnShared, &DeviceMachine::EnterDisablingPort };

/* Waiting for a port, hub or timer event with nothing to start */
const DeviceMachine::State DeviceMachine::AwaitingPort =
    { NULL, "AwaitingPort", 0, &DeviceMachine::OnShared, NULL };

/* Running one of the machine's timers, m_DelayKind says which */
const DeviceMachine::State DeviceMachine::Delaying =
    { NULL, "Delaying", 0, &DeviceMachine::OnShared, &DeviceMachine::EnterDelaying };

/* Device gone, a canceled controller request still has to complete */
const DeviceMachine::State DeviceMachine::IoctlAfterDetach =
    { NULL, "IoctlAfterDetach", 0, &DeviceMachine::OnShared, NULL, HubEventDiscards };

/* Device gone, a timer that could not be stopped still has to fire */
const DeviceMachine::State DeviceMachine::TimerAfterDetach =
    { NULL, "TimerAfterDetach", 0, &DeviceMachine::OnShared, NULL, HubEventDiscards };

/* Controller disables the device and its default endpoint */
const DeviceMachine::State DeviceMachine::DisablingDevice =
    { NULL, "DisablingDevice", SM_STATE_CRITICAL_ONLY,
      &DeviceMachine::OnShared, &DeviceMachine::EnterDisablingDevice };

/* Sets the USB PD charging policy on the device */
const DeviceMachine::State DeviceMachine::SettingPdCharging =
    { NULL, "SettingPdCharging", SM_STATE_CRITICAL_ONLY,
      &DeviceMachine::OnShared, &DeviceMachine::EnterSettingPdCharging };

SM_RESULT
DeviceMachine::EnterDisablingPort()
{
    RequestPortDisable();
    return SmHandled();
}

SM_RESULT
DeviceMachine::EnterDelaying()
{
    switch (m_DelayKind)
    {
        case DsmDelayEnumRetry:
            StartRetryTimer();
            break;

        case DsmDelayPostReset:
            StartPostResetTimer();
            break;

        case DsmDelayPostResetLong:
            StartLongPostResetTimer();
            break;

        case DsmDelayPostResetSuperSpeed:
            StartSuperSpeedPostResetTimer();
            break;

        case DsmDelayPostAddress:
            ArmPostAddressTimer();
            break;

        case DsmDelayDuplicate:
            ArmDuplicateDeviceTimer();
            break;
    }

    return SmHandled();
}

SM_RESULT
DeviceMachine::Delay(
    _In_ DSM_DELAY Kind,
    _In_ DSM_NEXT Next)
{
    m_DelayKind = Kind;
    return Wait(&Delaying, Next);
}

SM_RESULT
DeviceMachine::EnterDisablingDevice()
{
    DisableDeviceInController();
    return SmHandled();
}

SM_RESULT
DeviceMachine::EnterSettingPdCharging()
{
    SetPdChargingPolicy();
    return SmHandled();
}

/* Disable the device in the controller, delete it at passive level, then run After */
SM_RESULT
DeviceMachine::DisableAndDelete(
    _In_ DSM_THEN After)
{
    m_AfterDelete = After;
    return Wait(&DisablingDevice, &DeviceMachine::DeleteOnceDisabled);
}

SM_RESULT
DeviceMachine::DeleteOnceDisabled(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return AtPassive(&DeviceMachine::DeleteDeviceThen);
}

SM_RESULT
DeviceMachine::DeleteDeviceThen()
{
    DeleteDefaultEndpoint();
    DeleteDeviceInController();
    return (this->*m_AfterDelete)();
}

/* Controller enables the device and its default endpoint */
const DeviceMachine::State DeviceMachine::EnablingDevice =
    { NULL, "EnablingDevice", SM_STATE_CRITICAL_ONLY,
      &DeviceMachine::OnShared, &DeviceMachine::EnterEnablingDevice };

/* Asked the port machine to reset the port */
const DeviceMachine::State DeviceMachine::ResettingPort =
    { NULL, "ResettingPort", 0, &DeviceMachine::OnShared, &DeviceMachine::EnterResettingPort };

/* Controller learns the device was reset */
const DeviceMachine::State DeviceMachine::NotifyingReset =
    { NULL, "NotifyingReset", SM_STATE_CRITICAL_ONLY,
      &DeviceMachine::OnShared, &DeviceMachine::EnterNotifyingReset };

/* First 8 bytes of the device descriptor at address zero */
const DeviceMachine::State DeviceMachine::ReadingFirstDescriptor =
    { NULL, "ReadingFirstDescriptor", SM_STATE_CRITICAL_ONLY,
      &DeviceMachine::OnShared, &DeviceMachine::EnterReadingFirstDescriptor };

SM_RESULT
DeviceMachine::EnterEnablingDevice()
{
    EnableDeviceInController();
    return SmHandled();
}

SM_RESULT
DeviceMachine::EnterResettingPort()
{
    RequestPortReset();
    return SmHandled();
}

SM_RESULT
DeviceMachine::EnterNotifyingReset()
{
    NotifyDeviceReset();
    return SmHandled();
}

SM_RESULT
DeviceMachine::EnterReadingFirstDescriptor()
{
    ReadFirstDescriptor();
    return SmHandled();
}

/* Push a sub machine that starts in a shared wait */
SM_RESULT
DeviceMachine::CallWait(
    _In_ const State* Target,
    _In_ DSM_NEXT Next)
{
    m_Next = Next;
    return SmCall(Target);
}

/* Delete the default endpoint and the device at passive level, then run After */
SM_RESULT
DeviceMachine::DeleteThen(
    _In_ DSM_THEN After)
{
    m_AfterDelete = After;
    return AtPassive(&DeviceMachine::DeleteDeviceThen);
}

/* SUB MACHINE ENDINGS ********************************************************/

SM_RESULT
DeviceMachine::EndFailed()
{
    return EndWith(DsmEvent::Failed);
}

SM_RESULT
DeviceMachine::EndDetached()
{
    return EndWith(DsmEvent::PortDetached);
}

SM_RESULT
DeviceMachine::EndWithHubEvent()
{
    return EndWith(m_HubEvent);
}

/* A hub stop, hub suspend or detach ends the sub machine with that event */
SM_RESULT
DeviceMachine::EndOnHubOrDetach(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
        case DsmEvent::HubStopping:
        case DsmEvent::HubSuspending:
            return EndWith(Event);

        default:
            return SmUnhandled();
    }
}

/* Hub stop or suspend came during a port reset; the reset result is moot */
SM_RESULT
DeviceMachine::EndOnResetForHub(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            return EndWith(DsmEvent::PortDetached);

        case DsmEvent::PortResetDone:
        case DsmEvent::PortResetTimeout:
        case DsmEvent::PortResetAbortedForSuspend:
        case DsmEvent::PortFault:
            return EndWith(m_HubEvent);

        default:
            return SmUnhandled();
    }
}

/* Stop the running timer, then end with Event once it can no longer fire */
SM_RESULT
DeviceMachine::EndAfterTimer(
    _In_ DsmEvent Event)
{
    if (Event == DsmEvent::PortDetached)
    {
        if (StopTimer())
            return EndWith(DsmEvent::PortDetached);

        return Wait(&TimerAfterDetach, &DeviceMachine::EndDetachedOnTimer);
    }

    m_HubEvent = Event;
    if (StopTimer())
        return EndWith(Event);

    return Wait(&AwaitingPort, &DeviceMachine::EndForHubOnTimer);
}

SM_RESULT
DeviceMachine::EndDetachedOnTimer(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::TimerFired)
        return SmUnhandled();

    return EndWith(DsmEvent::PortDetached);
}

SM_RESULT
DeviceMachine::EndForHubOnTimer(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            return Wait(&TimerAfterDetach, &DeviceMachine::EndDetachedOnTimer);

        case DsmEvent::TimerFired:
            return EndWith(m_HubEvent);

        default:
            return SmUnhandled();
    }
}

/* Push a sub machine that starts with code run at passive level */
SM_RESULT
DeviceMachine::CallAtPassive(
    _In_ DSM_THEN Then)
{
    m_Then = Then;
    return SmCall(&PassiveStep);
}

/* ONE SHOT REQUESTS **********************************************************/

/* Runs the code m_Then names in a new frame, for sub machines that start with code */
const DeviceMachine::State DeviceMachine::Starting =
    { NULL, "Starting", 0, NULL, &DeviceMachine::EnterStarting };

/* A control transfer or controller request the machine waits for, m_Start sends it */
const DeviceMachine::State DeviceMachine::Requesting =
    { NULL, "Requesting", 0, &DeviceMachine::OnShared, &DeviceMachine::EnterRequesting };

/* Same, letting nothing but completions through */
const DeviceMachine::State DeviceMachine::RequestingCritical =
    { NULL, "RequestingCritical", SM_STATE_CRITICAL_ONLY,
      &DeviceMachine::OnShared, &DeviceMachine::EnterRequesting };

/* Same, inside a sub machine its caller may leave at any time */
const DeviceMachine::State DeviceMachine::RequestingYielding =
    { NULL, "RequestingYielding", SM_STATE_YIELDS_TO_CALLER,
      &DeviceMachine::OnShared, &DeviceMachine::EnterRequesting };

/* Canceled a control transfer; the sub machine ends with m_HubEvent once it completes */
const DeviceMachine::State DeviceMachine::CancelingTransfer =
    { NULL, "CancelingTransfer", SM_STATE_CRITICAL_ONLY,
      &DeviceMachine::OnShared, &DeviceMachine::EnterCancelingTransfer };

SM_RESULT
DeviceMachine::EnterStarting()
{
    return (this->*m_Then)();
}

SM_RESULT
DeviceMachine::CallStep(
    _In_ DSM_THEN Then)
{
    m_Then = Then;
    return SmCall(&Starting);
}

SM_RESULT
DeviceMachine::EnterRequesting()
{
    (this->*m_Start)();
    return SmHandled();
}

/* Send a request with Start and wait in Target; Next gets the completion */
SM_RESULT
DeviceMachine::Request(
    _In_ const State* Target,
    _In_ DSM_ACTION Start,
    _In_ DSM_NEXT Next)
{
    m_Start = Start;
    return Wait(Target, Next);
}

SM_RESULT
DeviceMachine::CallRequest(
    _In_ const State* Target,
    _In_ DSM_ACTION Start,
    _In_ DSM_NEXT Next)
{
    m_Start = Start;
    return CallWait(Target, Next);
}

SM_RESULT
DeviceMachine::EnterCancelingTransfer()
{
    CancelTransfer();
    return SmHandled();
}

/* Detach, hub stop or hub suspend during a transfer: cancel it, then end the sub machine */
SM_RESULT
DeviceMachine::CancelTransferAndEnd(
    _In_ DsmEvent Event)
{
    m_HubEvent = Event;
    return Wait(&CancelingTransfer, &DeviceMachine::EndOnTransferCanceled);
}

SM_RESULT
DeviceMachine::EndOnTransferCanceled(
    _In_ DsmEvent Event)
{
    if ((Event != DsmEvent::TransferDone) && (Event != DsmEvent::TransferFailed))
        return SmUnhandled();

    return EndWith(m_HubEvent);
}

/* Asked the port machine to suspend the port */
const DeviceMachine::State DeviceMachine::SuspendingPort =
    { NULL, "SuspendingPort", 0, &DeviceMachine::OnShared, &DeviceMachine::EnterSuspendingPort };

/* Asked the port machine to resume the port */
const DeviceMachine::State DeviceMachine::ResumingPort =
    { NULL, "ResumingPort", 0, &DeviceMachine::OnShared, &DeviceMachine::EnterResumingPort };

SM_RESULT
DeviceMachine::EnterSuspendingPort()
{
    RequestPortSuspend();
    return SmHandled();
}

SM_RESULT
DeviceMachine::EnterResumingPort()
{
    RequestPortResume();
    return SmHandled();
}

/* Waiting with hub events dropped, the device is going away */
const DeviceMachine::State DeviceMachine::IgnoringHub =
    { NULL, "IgnoringHub", 0, &DeviceMachine::OnShared, NULL, HubEventDiscards };

/* The controller handles a client streams request; then back to m_ReturnTo */
const DeviceMachine::State DeviceMachine::ForwardingStreams =
    { NULL, "ForwardingStreams", SM_STATE_CRITICAL_ONLY | SM_STATE_NEEDS_PASSIVE,
      &DeviceMachine::OnForwardingStreams, &DeviceMachine::EnterForwardingStreams };

SM_RESULT
DeviceMachine::ForwardStreams(
    _In_ const State* ReturnTo)
{
    m_ReturnTo = ReturnTo;
    return SmTransition(&ForwardingStreams);
}

SM_RESULT
DeviceMachine::EnterForwardingStreams()
{
    ForwardToController();
    return SmHandled();
}

SM_RESULT
DeviceMachine::OnForwardingStreams(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerRequestDone)
        return SmUnhandled();

    if (m_ReturnTo == NULL)
        return (this->*m_Resume)();

    return SmTransition(m_ReturnTo);
}

/* Controller disables every endpoint of the current configuration */
const DeviceMachine::State DeviceMachine::DisablingEndpoints =
    { NULL, "DisablingEndpoints", SM_STATE_CRITICAL_ONLY,
      &DeviceMachine::OnShared, &DeviceMachine::EnterDisablingEndpoints };

SM_RESULT
DeviceMachine::EnterDisablingEndpoints()
{
    DisableAllEndpoints();
    return SmHandled();
}

/* Waiting for the port machine inside a sub machine its caller may leave */
const DeviceMachine::State DeviceMachine::AwaitingPortYielding =
    { NULL, "AwaitingPortYielding", SM_STATE_YIELDS_TO_CALLER, &DeviceMachine::OnShared, NULL };

/* Same as RequestingCritical, for requests that must be sent at passive level */
const DeviceMachine::State DeviceMachine::RequestingCriticalPassive =
    { NULL, "RequestingCriticalPassive", SM_STATE_CRITICAL_ONLY | SM_STATE_NEEDS_PASSIVE,
      &DeviceMachine::OnShared, &DeviceMachine::EnterRequesting };

/* A controller request started by a query is running; nothing else may interrupt */
const DeviceMachine::State DeviceMachine::WaitingForController =
    { NULL, "WaitingForController", SM_STATE_CRITICAL_ONLY, &DeviceMachine::OnShared, NULL };
