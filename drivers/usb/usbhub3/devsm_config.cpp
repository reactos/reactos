/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Device state machine, configured device, client requests and
 *              configuration changes
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include "usbhub3.h"

/* GLOBALS ********************************************************************/

static const DsmEvent ConfigLpmDiscards[] = { DsmEvent::LpmSettingChanged, DsmEvent::Count };

/* A configuration is selected; handles the client requests both children share */
const DeviceMachine::State DeviceMachine::Configured =
    { NULL, "Configured", 0, &DeviceMachine::OnConfigured, NULL };

/* Configured and the PDO in D0 */
const DeviceMachine::State DeviceMachine::ConfiguredOn =
    { &Configured, "ConfiguredOn", SM_STATE_TAKES_REQUESTS, &DeviceMachine::OnConfiguredOn, NULL };

/* Configured before PnP started the PDO; some client drivers do that */
const DeviceMachine::State DeviceMachine::ConfiguredNotStarted =
    { &Configured, "ConfiguredNotStarted", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnConfiguredNotStarted, &DeviceMachine::EnterWithMsOsInstall,
      ConfigLpmDiscards };

/* Runs the configure sub machine; m_AfterConfigure decides what follows */
const DeviceMachine::State DeviceMachine::Configuring =
    { NULL, "Configuring", 0, &DeviceMachine::OnConfiguring, &DeviceMachine::EnterConfiguring };

/* Runs the set interface sub machine for a client request */
const DeviceMachine::State DeviceMachine::SettingInterface =
    { NULL, "SettingInterface", 0,
      &DeviceMachine::OnSettingInterface, &DeviceMachine::EnterSettingInterface };

/* Programs endpoints and, for SuperSpeed, link power; m_AfterProgram gets the result */
const DeviceMachine::State DeviceMachine::ProgrammingEndpoints =
    { NULL, "ProgrammingEndpoints", SM_STATE_YIELDS_TO_CALLER,
      &DeviceMachine::OnProgrammingEndpoints, &DeviceMachine::EnterProgrammingEndpoints };

/* Link power settings changed while configured */
const DeviceMachine::State DeviceMachine::UpdatingLinkPower =
    { NULL, "UpdatingLinkPower", 0,
      &DeviceMachine::OnUpdatingLinkPower, &DeviceMachine::EnterUpdatingLinkPower };

/* Configured device leaving D0 through the suspend sub machine */
const DeviceMachine::State DeviceMachine::SuspendingConfigured =
    { NULL, "SuspendingConfigured", 0,
      &DeviceMachine::OnSuspendingConfigured, &DeviceMachine::EnterSuspendingConfigured };

/* Same for the device holding the boot, paging or hibernation file */
const DeviceMachine::State DeviceMachine::BootDeviceSuspending =
    { NULL, "BootDeviceSuspending", 0,
      &DeviceMachine::OnBootDeviceSuspending, &DeviceMachine::EnterSuspendingConfigured };

const DeviceMachine::State DeviceMachine::SuspendedConfigured =
    { NULL, "SuspendedConfigured", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnSuspendedConfigured, &DeviceMachine::EnterSuspended };

const DeviceMachine::State DeviceMachine::BootDeviceSuspended =
    { NULL, "BootDeviceSuspended", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnBootDeviceSuspended, &DeviceMachine::EnterSuspended };

/* Configured device suspended because the hub suspended */
const DeviceMachine::State DeviceMachine::ConfiguredHubSuspended =
    { NULL, "ConfiguredHubSuspended", 0,
      &DeviceMachine::OnConfiguredHubSuspended, &DeviceMachine::EnterHubSuspendedReleasingPower };

/* Asked the port machine to get the boot device ready for hibernation */
const DeviceMachine::State DeviceMachine::PreparingHibernation =
    { NULL, "PreparingHibernation", 0,
      &DeviceMachine::OnPreparingHibernation, &DeviceMachine::EnterPreparingHibernation };

/* FUNCTIONS ******************************************************************/

/* Back to the configured D0 state, or to waiting for a start for an early configuration */
SM_RESULT
DeviceMachine::BackToConfigured()
{
    if (PdoStarted())
        return SmTransition(&ConfiguredOn);

    return SmTransition(&ConfiguredNotStarted);
}

SM_RESULT
DeviceMachine::GoConfiguredOn()
{
    return SmTransition(&ConfiguredOn);
}

/* CONFIGURED *****************************************************************/

SM_RESULT
DeviceMachine::OnConfigured(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ClientUnconfigure:
            return Request(&RequestingCritical, &DeviceMachine::SetNullConfiguration,
                           &DeviceMachine::ConfiguredNullSet);

        case DsmEvent::ClientResetDevice:
            ForceResetOnNextStart();
            if (IsBootDevice())
                return AtPassive(&DeviceMachine::ConfiguredResetBootDevice);

            return ReenumForClientConfigured();

        case DsmEvent::ClientStreams:
            m_Resume = &DeviceMachine::BackToConfigured;
            return ForwardStreams(NULL);

        case DsmEvent::ClientSelectConfig:
            m_FromConfigured = TRUE;
            return AtPassive(&DeviceMachine::SelectConfigValidate);

        case DsmEvent::ClientSetInterface:
            if (!SelectInterfaceValid())
                return ClientDoneWithLastStatus();

            return SmTransition(&SettingInterface);

        case DsmEvent::ClientClearStall:
            if (!FindClientPipe())
                return ClientDoneWithLastStatus();

            if (PipeIsZeroBandwidth())
                return ClientDoneOk();

            return Request(&RequestingCritical, &DeviceMachine::ClearEndpointHalt,
                           &DeviceMachine::ClientPipeDone);

        case DsmEvent::DeviceTextQuery:
            m_Resume = &DeviceMachine::BackToConfigured;
            return SmTransition(&ReadingDeviceText);

        case DsmEvent::ClientSyncResetPipe:
            if (!FindClientPipe())
                return ClientDoneWithLastStatus();

            if (PipeIsZeroBandwidth())
                return ClientDoneOk();

            return Request(&RequestingCritical, &DeviceMachine::ResetEndpoint,
                           &DeviceMachine::ClientPipeDone);

        case DsmEvent::HubGetDescriptor:
            m_Resume = &DeviceMachine::BackToConfigured;
            return Request(&RequestingCritical, &DeviceMachine::GetDescriptorForHub,
                           &DeviceMachine::HubDescriptorDone);

        case DsmEvent::ClientResetPipe:
            if (!FindClientPipe())
                return ClientDoneWithLastStatus();

            if (PipeIsZeroBandwidth())
                return ClientDoneOk();

            if (PipeIsIsochronous())
            {
                return Request(&RequestingCritical, &DeviceMachine::ResetEndpointAndTransfers,
                               &DeviceMachine::ClientPipeDone);
            }

            return Request(&RequestingCritical, &DeviceMachine::ClearEndpointHalt,
                           &DeviceMachine::ClientHaltCleared);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ClientDoneOk()
{
    CompleteClientRequest();
    return BackToConfigured();
}

SM_RESULT
DeviceMachine::ClientDoneWithLastStatus()
{
    CompleteClientRequestLastStatus();
    return BackToConfigured();
}

/* A pipe request finished in the controller or on the bus */
SM_RESULT
DeviceMachine::ClientPipeDone(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
        case DsmEvent::ControllerIoctlDone:
            return ClientDoneOk();

        case DsmEvent::TransferFailed:
        case DsmEvent::ControllerIoctlFailed:
            return ClientDoneWithLastStatus();

        default:
            return SmUnhandled();
    }
}

/* Reset pipe on a non isochronous pipe: halt cleared, now reset it in the controller */
SM_RESULT
DeviceMachine::ClientHaltCleared(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            return Request(&RequestingCritical, &DeviceMachine::ResetEndpointAndTransfers,
                           &DeviceMachine::ClientPipeDone);

        case DsmEvent::TransferFailed:
            return ClientDoneWithLastStatus();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ConfiguredNullSet(
    _In_ DsmEvent Event)
{
    if ((Event != DsmEvent::TransferDone) && (Event != DsmEvent::TransferFailed))
        return SmUnhandled();

    return Wait(&DisablingEndpoints, &DeviceMachine::ConfiguredEndpointsDisabledForClient);
}

SM_RESULT
DeviceMachine::ConfiguredEndpointsDisabledForClient(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return AtPassive(&DeviceMachine::ConfiguredFreeForClient);
}

SM_RESULT
DeviceMachine::ConfiguredFreeForClient()
{
    DeleteConfigEndpoints();
    CompleteClientRequest();
    return BackToUnconfigured();
}

SM_RESULT
DeviceMachine::ConfiguredResetBootDevice()
{
    NotifyDisconnected();
    return ReenumForBootClient(FALSE);
}

SM_RESULT
DeviceMachine::OnConfiguredOn(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ClientCyclePort:
            CyclePortForClient();
            return SmHandled();

        case DsmEvent::PdoPowerUp:
        case DsmEvent::PdoInstallMsOsExt:
            return Then(&DeviceMachine::ConfiguredSignalAndStay);

        case DsmEvent::HubSuspending:
            return Request(&RequestingCritical, &DeviceMachine::PurgeIoForSuspend,
                           &DeviceMachine::ConfiguredPurgedForHub);

        case DsmEvent::PdoPrepareHibernate:
            return Request(&RequestingCritical, &DeviceMachine::PurgeIoForHibernation,
                           &DeviceMachine::ConfiguredPurgedForHibernation);

        case DsmEvent::NoPingResponse:
            if (AdjustLatencyForNoPing())
                return SmTransition(&UpdatingLinkPower);

            return ConfiguredCyclePort();

        case DsmEvent::PortDetached:
            return PurgeOnDetach();

        case DsmEvent::PdoPowerDown:
            ClearResetAtResume();
            if (IsBootDevice())
                return SmTransition(&BootDeviceSuspending);

            return SmTransition(&SuspendingConfigured);

        case DsmEvent::LpmSettingChanged:
            return SmTransition(&UpdatingLinkPower);

        case DsmEvent::PdoPowerDownFinal:
            return Wait(&DisablingEndpoints, &DeviceMachine::ConfiguredEndpointsDisabledForStop);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ConfiguredSignalAndStay()
{
    SignalPnpWaiter();
    return SmTransition(&ConfiguredOn);
}

/* An error the device cannot recover from: cycle the port and wait */
SM_RESULT
DeviceMachine::ConfiguredCyclePort()
{
    AskPortCycle();
    return ToAwaitingDetachOrRemove(TRUE);
}

SM_RESULT
DeviceMachine::ConfiguredEndpointsDisabledForStop(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return StopDecision();
}

/* PDO stopped for good: disable the device, or keep it enabled for the next start */
SM_RESULT
DeviceMachine::StopDecision()
{
    if (DisableWhenUnused())
        return Wait(&DisablingDevice, &DeviceMachine::UnconfiguredDisabledForStop);

    SignalPnpWaiter();
    ForceResetOnNextStart();
    return SmTransition(&StoppedReady);
}

SM_RESULT
DeviceMachine::ConfiguredPurgedForHub(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return Wait(&SuspendingPort, &DeviceMachine::ConfiguredSuspendedForHub);
}

SM_RESULT
DeviceMachine::ConfiguredSuspendedForHub(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortFault:
            DereferenceDevicePower();
            return ConfiguredCyclePort();

        case DsmEvent::PortSuspendDone:
            return SmTransition(&ConfiguredHubSuspended);

        case DsmEvent::PortDetached:
            DereferenceDevicePower();
            return AtPassive(&DeviceMachine::RemovalNotifyThenReport);

        default:
            return SmUnhandled();
    }
}

/* Hibernation purge also clears the reset on last resume mark */
VOID
DeviceMachine::PurgeIoForHibernation()
{
    PurgeIoForSuspend();
    ClearResetAtResume();
}

SM_RESULT
DeviceMachine::ConfiguredPurgedForHibernation(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return SmTransition(&PreparingHibernation);
}

SM_RESULT
DeviceMachine::EnterPreparingHibernation()
{
    RequestHibernationPrep();
    return SmHandled();
}

SM_RESULT
DeviceMachine::OnPreparingHibernation(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortSuspendDone:
            return SmTransition(&BootDeviceSuspended);

        case DsmEvent::PortDetached:
            return AtPassive(&DeviceMachine::BootDeviceNotifyDetached);

        case DsmEvent::PortFault:
            SignalPnpWaiter();
            return BootDeviceFailed();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::OnConfiguredNotStarted(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PdoPreStart:
            AckPreStart();
            return SmHandled();

        case DsmEvent::PdoPowerUp:
            return Then(&DeviceMachine::ConfiguredSignalAndStay);

        case DsmEvent::PdoCleanup:
            return Wait(&DisablingEndpoints, &DeviceMachine::ConfiguredNotStartedCleanup);

        case DsmEvent::PortDetached:
            return AtPassive(&DeviceMachine::RemovalNotifyThenReport);

        case DsmEvent::HubStopping:
            return Wait(&DisablingEndpoints, &DeviceMachine::ConfiguredNotStartedHubStop);

        case DsmEvent::HubSuspending:
            return Wait(&DisablingEndpoints, &DeviceMachine::ConfiguredNotStartedHubSuspend);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ConfiguredNotStartedCleanup(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return Wait(&DisablingPort, &DeviceMachine::CleanupPortDisabled);
}

SM_RESULT
DeviceMachine::ConfiguredNotStartedHubStop(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return Wait(&DisablingPort, &DeviceMachine::StoppedPortOffForHubStop);
}

SM_RESULT
DeviceMachine::ConfiguredNotStartedHubSuspend(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return Wait(&SuspendingPort, &DeviceMachine::StoppedPortSuspendedForHub);
}

/* SELECT CONFIGURATION *******************************************************/

SM_RESULT
DeviceMachine::SelectConfigValidate()
{
    if (!SelectConfigValid())
        return SelectConfigFailed(TRUE);

    if (!PrepareConfigLists())
        return SelectConfigFailed(m_FromConfigured);

    m_AfterConfigure = &DeviceMachine::SelectConfigDone;
    return SmTransition(&Configuring);
}

/* The two failures differ in the status they complete the request with */
SM_RESULT
DeviceMachine::SelectConfigFailed(
    _In_ BOOLEAN LastStatus)
{
    if (LastStatus)
        CompleteClientRequestLastStatus();
    else
        CompleteClientRequestFailed();

    if (m_FromConfigured)
        return BackToConfigured();

    return BackToUnconfigured();
}

SM_RESULT
DeviceMachine::SelectConfigDone(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
            SetConfigInfoInRequest();
            CompleteClientRequest();
            return BackToConfigured();

        case DsmEvent::Failed:
            return AtPassive(&DeviceMachine::SelectConfigCleanUp);

        case DsmEvent::PortDetached:
            CompleteClientRequestFailed();
            return DetachConfigured();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::SelectConfigCleanUp()
{
    DeleteConfigEndpoints();
    CompleteClientRequestLastStatus();
    return BackToUnconfigured();
}

/* CONFIGURE SUB MACHINE ******************************************************/

SM_RESULT
DeviceMachine::EnterConfiguring()
{
    return CallRequest(&RequestingCritical, &DeviceMachine::DisablePendingEndpoints,
                       &DeviceMachine::ConfigureOldDisabled);
}

SM_RESULT
DeviceMachine::OnConfiguring(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
        case DsmEvent::Failed:
        case DsmEvent::PortDetached:
            return (this->*m_AfterConfigure)(Event);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ConfigureOldDisabled(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return AtPassive(&DeviceMachine::ConfigureDeleteOld);
}

SM_RESULT
DeviceMachine::ConfigureDeleteOld()
{
    DeleteOldConfigEndpoints();
    return Request(&RequestingCritical, &DeviceMachine::SetConfiguration,
                   &DeviceMachine::ConfigureInterfaceSet);
}

/* Configuration or one alternate setting set; set the next one or create endpoints */
SM_RESULT
DeviceMachine::ConfigureInterfaceSet(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            if (AlternateSettingLeft())
            {
                return Request(&RequestingCritical, &DeviceMachine::SetInterface,
                               &DeviceMachine::ConfigureInterfaceSet);
            }

            return AtPassive(&DeviceMachine::ConfigureCreateEndpoints);

        case DsmEvent::TransferFailed:
            return ConfigureFailed();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ConfigureCreateEndpoints()
{
    if (!CreateEndpoints())
        return ConfigureFailed();

    m_AfterProgram = &DeviceMachine::ConfigureProgrammed;
    return SmTransition(&ProgrammingEndpoints);
}

SM_RESULT
DeviceMachine::ConfigureFailed()
{
    MarkPendingEndpointsDisabled();
    return EndWith(DsmEvent::Failed);
}

SM_RESULT
DeviceMachine::ConfigureProgrammed(
    _In_ DsmEvent Event)
{
    if (Event == DsmEvent::Succeeded)
        return EndWith(DsmEvent::Succeeded);

    return EndWith(DsmEvent::Failed);
}

/* PROGRAMMING ENDPOINTS ******************************************************/

SM_RESULT
DeviceMachine::EnterProgrammingEndpoints()
{
    if (KindWithin(DSM_KIND_USB3X | DSM_KIND_ANY_SPEED | DSM_KIND_ANY_PORT))
        return CallStep(&DeviceMachine::LinkPowerStart);

    return Request(&RequestingCritical, &DeviceMachine::ProgramEndpoints,
                   &DeviceMachine::EndpointsProgrammed);
}

SM_RESULT
DeviceMachine::OnProgrammingEndpoints(
    _In_ DsmEvent Event)
{
    if ((Event != DsmEvent::Succeeded) && (Event != DsmEvent::Failed))
        return SmUnhandled();

    return (this->*m_AfterProgram)(Event);
}

SM_RESULT
DeviceMachine::EndpointsProgrammed(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ControllerIoctlDone:
            return (this->*m_AfterProgram)(DsmEvent::Succeeded);

        case DsmEvent::ControllerIoctlFailed:
            return (this->*m_AfterProgram)(DsmEvent::Failed);

        default:
            return SmUnhandled();
    }
}

/* SET INTERFACE **************************************************************/

SM_RESULT
DeviceMachine::EnterSettingInterface()
{
    return CallStep(&DeviceMachine::InterfaceStart);
}

SM_RESULT
DeviceMachine::OnSettingInterface(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Failed:
            return ClientDoneWithLastStatus();

        case DsmEvent::Succeeded:
            SetInterfaceInfoInRequest();
            return ClientDoneOk();

        case DsmEvent::PortDetached:
            CompleteClientRequestFailed();
            return DetachConfigured();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::InterfaceStart()
{
    if (!PrepareInterfaceLists())
        return AtPassive(&DeviceMachine::InterfaceDeleteOldAndFail);

    return AtPassive(&DeviceMachine::InterfaceCreateEndpoints);
}

SM_RESULT
DeviceMachine::InterfaceCreateEndpoints()
{
    if (!CreateEndpoints())
    {
        return Request(&RequestingCritical, &DeviceMachine::DisablePendingEndpoints,
                       &DeviceMachine::InterfaceNewDisabled);
    }

    m_AfterProgram = &DeviceMachine::InterfaceProgrammed;
    return SmTransition(&ProgrammingEndpoints);
}

SM_RESULT
DeviceMachine::InterfaceNewDisabled(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return InterfaceRestoreOld();
}

SM_RESULT
DeviceMachine::InterfaceProgrammed(
    _In_ DsmEvent Event)
{
    if (Event == DsmEvent::Succeeded)
    {
        return Request(&RequestingCritical, &DeviceMachine::SetInterface,
                       &DeviceMachine::InterfaceSet);
    }

    return InterfaceRestoreOld();
}

/* Put the old alternate setting back on the device so it matches the old endpoints */
SM_RESULT
DeviceMachine::InterfaceRestoreOld()
{
    return Request(&RequestingCritical, &DeviceMachine::SetInterface,
                   &DeviceMachine::InterfaceOldRestored);
}

SM_RESULT
DeviceMachine::InterfaceOldRestored(
    _In_ DsmEvent Event)
{
    if ((Event != DsmEvent::TransferDone) && (Event != DsmEvent::TransferFailed))
        return SmUnhandled();

    return AtPassive(&DeviceMachine::InterfaceDeleteNewAndFail);
}

SM_RESULT
DeviceMachine::InterfaceSet(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::TransferDone:
            return AtPassive(&DeviceMachine::InterfaceDeleteOld);

        case DsmEvent::TransferFailed:
            return Request(&RequestingCritical, &DeviceMachine::DisableNewInterfaceEndpoints,
                           &DeviceMachine::InterfaceNewEndpointsDisabled);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::InterfaceNewEndpointsDisabled(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    return AtPassive(&DeviceMachine::InterfaceDeleteNewAndFail);
}

SM_RESULT
DeviceMachine::InterfaceDeleteOld()
{
    DeleteOldInterfaceEndpoints();
    return EndWith(DsmEvent::Succeeded);
}

SM_RESULT
DeviceMachine::InterfaceDeleteNewAndFail()
{
    DeleteNewInterfaceEndpoints();
    return InterfaceDeleteOldAndFail();
}

SM_RESULT
DeviceMachine::InterfaceDeleteOldAndFail()
{
    DeleteOldInterfaceEndpoints();
    return EndWith(DsmEvent::Failed);
}

/* LINK POWER WHILE CONFIGURED ************************************************/

SM_RESULT
DeviceMachine::EnterUpdatingLinkPower()
{
    if (KindWithin(DSM_KIND_USB3X | DSM_KIND_ANY_SPEED | DSM_KIND_ANY_PORT))
        return CallStep(&DeviceMachine::LinkPowerStart);

    return SmTransition(&ConfiguredOn);
}

SM_RESULT
DeviceMachine::OnUpdatingLinkPower(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
            return SmTransition(&ConfiguredOn);

        case DsmEvent::Failed:
            return ConfiguredCyclePort();

        case DsmEvent::PortDetached:
            return PurgeOnDetach();

        default:
            return SmUnhandled();
    }
}

/* SUSPEND AND RESUME WHILE CONFIGURED ****************************************/

SM_RESULT
DeviceMachine::EnterSuspendingConfigured()
{
    return CallStep(&DeviceMachine::SuspendStart);
}

SM_RESULT
DeviceMachine::OnSuspendingConfigured(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
        case DsmEvent::Failed:
            return SmTransition(&SuspendedConfigured);

        case DsmEvent::PortDetached:
            SignalPnpWaiter();
            return DetachConfigured();

        case DsmEvent::HubSuspending:
            return Request(&RequestingCritical, &DeviceMachine::PurgeIoForSuspend,
                           &DeviceMachine::ConfiguredPurgedWhileSuspending);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ConfiguredPurgedWhileSuspending(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    SignalPnpWaiter();
    return SmTransition(&ConfiguredHubSuspended);
}

SM_RESULT
DeviceMachine::OnBootDeviceSuspending(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::Succeeded:
            return SmTransition(&BootDeviceSuspended);

        case DsmEvent::Failed:
            SignalPnpWaiter();
            return BootDeviceFailed();

        case DsmEvent::PortDetached:
            return AtPassive(&DeviceMachine::BootDeviceNotifyDetached);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::OnSuspendedConfigured(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ClientSelectConfig:
        case DsmEvent::ClientClearStall:
        case DsmEvent::ClientUnconfigure:
        case DsmEvent::ClientSetInterface:
        case DsmEvent::ClientResetPipe:
        case DsmEvent::ClientSyncResetPipe:
        case DsmEvent::ClientResetDevice:
            FailClientRequest();
            return SmHandled();

        case DsmEvent::NoPingResponse:
        case DsmEvent::LpmSettingChanged:
            return SmHandled();

        case DsmEvent::DeviceTextQuery:
            SignalQueryText();
            return SmHandled();

        case DsmEvent::ClientCyclePort:
            CyclePortForClient();
            return SmHandled();

        case DsmEvent::HubGetDescriptor:
            FailHubDescriptorRequest();
            return SmHandled();

        case DsmEvent::NeedsReenumeration:
            return ReenumForResume(TRUE);

        case DsmEvent::ResumeDone:
            return Then(&DeviceMachine::ConfiguredResumed);

        case DsmEvent::HubStopping:
            m_EndpointsConfigured = TRUE;
            return DisableForRemoval(&DeviceMachine::RemovalDropPowerAndPark);

        case DsmEvent::PortDetached:
            return AtPassive(&DeviceMachine::RemovalNotifyThenReport);

        case DsmEvent::PdoCleanup:
            m_EndpointsConfigured = TRUE;
            return DisableForRemoval(&DeviceMachine::RemovalDeleteForgetAndPark);

        case DsmEvent::ClientStreams:
            return ForwardStreams(&SuspendedConfigured);

        case DsmEvent::PortFault:
            return ConfiguredCyclePort();

        case DsmEvent::HubStoppingAfterSuspend:
            m_EndpointsConfigured = TRUE;
            return DisableForRemoval(&DeviceMachine::RemovalAckAndPark);

        case DsmEvent::PdoPreStart:
            m_EndpointsConfigured = TRUE;
            return DisableForRemoval(&DeviceMachine::ConfiguredRestartWhileSuspended);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ConfiguredResumed()
{
    SignalPnpWaiter();
    return SmTransition(&ConfiguredOn);
}

SM_RESULT
DeviceMachine::ConfiguredRestartWhileSuspended()
{
    return ReenumForPreStart();
}

SM_RESULT
DeviceMachine::OnBootDeviceSuspended(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ClientSyncResetPipe:
        case DsmEvent::ClientClearStall:
        case DsmEvent::ClientSetInterface:
        case DsmEvent::ClientResetPipe:
        case DsmEvent::ClientUnconfigure:
        case DsmEvent::ClientResetDevice:
        case DsmEvent::ClientSelectConfig:
            FailClientRequest();
            return SmHandled();

        case DsmEvent::NoPingResponse:
        case DsmEvent::LpmSettingChanged:
            return SmHandled();

        case DsmEvent::DeviceTextQuery:
            SignalQueryText();
            return SmHandled();

        case DsmEvent::HubGetDescriptor:
            FailHubDescriptorRequest();
            return SmHandled();

        case DsmEvent::NeedsReenumeration:
            return ReenumForBootResume();

        case DsmEvent::ResumeDone:
            return Then(&DeviceMachine::ConfiguredResumed);

        case DsmEvent::PortDetached:
            return AtPassive(&DeviceMachine::BootDeviceNotifyDetached);

        case DsmEvent::ClientStreams:
            return ForwardStreams(&BootDeviceSuspended);

        case DsmEvent::PortFault:
            return BootDeviceFailed();

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::OnConfiguredHubSuspended(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::NeedsReenumeration:
            return ReenumForHubResume(TRUE);

        case DsmEvent::SuspendedAfterHubResume:
            return Wait(&ResumingPort, &DeviceMachine::ConfiguredResumedWithHub);

        case DsmEvent::ResumedWithHub:
            return Request(&RequestingCritical, &DeviceMachine::StartDeviceIo,
                           &DeviceMachine::ConfiguredIoStarted);

        case DsmEvent::PortDetached:
            return AtPassive(&DeviceMachine::RemovalNotifyThenReport);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ConfiguredResumedWithHub(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::PortDetached:
            return AtPassive(&DeviceMachine::RemovalNotifyThenReport);

        case DsmEvent::PortFault:
        case DsmEvent::PortNowEnabledOnReconnect:
        case DsmEvent::PortResumeTimeout:
        case DsmEvent::PortNowDisabled:
            return ConfiguredCyclePort();

        case DsmEvent::PortResumeDone:
            return Request(&RequestingCritical, &DeviceMachine::StartDeviceIo,
                           &DeviceMachine::ConfiguredIoStarted);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::ConfiguredIoStarted(
    _In_ DsmEvent Event)
{
    if ((Event != DsmEvent::ControllerIoctlDone) && (Event != DsmEvent::ControllerIoctlFailed))
        return SmUnhandled();

    return SmTransition(&ConfiguredOn);
}
