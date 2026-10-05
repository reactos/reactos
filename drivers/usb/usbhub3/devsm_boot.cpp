/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Device state machine, device that holds the boot, paging or
 *              hibernation file. Its PDO never goes away; the machine waits
 *              for it to come back instead.
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES *******************************************************************/

#include "usbhub3.h"

/* GLOBALS ********************************************************************/

static const DsmEvent BootDiscards[] =
{
    DsmEvent::LpmSettingChanged, DsmEvent::NoPingResponse, DsmEvent::Count
};

/* Device failed; waiting for it to detach or for the client to reset it */
const DeviceMachine::State DeviceMachine::BootDeviceAwaitingReset =
    { NULL, "BootDeviceAwaitingReset", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnBootDeviceAwaitingReset, &DeviceMachine::EnterBootDeviceEvents };

/* Device detached; waiting for it to come back. m_ContextGone once the controller forgot it */
const DeviceMachine::State DeviceMachine::BootDeviceDetached =
    { NULL, "BootDeviceDetached", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnBootDeviceDetached, &DeviceMachine::EnterBootDeviceDetached };

const DeviceMachine::State DeviceMachine::BootDeviceDetachedContextGone =
    { NULL, "BootDeviceDetachedContextGone", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnBootDeviceDetached, &DeviceMachine::EnterBootDeviceDetachedContextGone,
      BootDiscards };

/* Client asked for a reset while detached; the reset runs on reattach */
const DeviceMachine::State DeviceMachine::BootDeviceResetPending =
    { NULL, "BootDeviceResetPending", SM_STATE_TAKES_REQUESTS,
      &DeviceMachine::OnBootDeviceResetPending, &DeviceMachine::EnterBootDeviceResetPending,
      BootDiscards };

/* Sub machine: keeps hub and PnP events balanced for the boot device */
const DeviceMachine::State DeviceMachine::BootDeviceEvents =
    { NULL, "BootDeviceEvents", SM_STATE_YIELDS_TO_CALLER, &DeviceMachine::OnBootDeviceEvents, NULL };

/* FUNCTIONS ******************************************************************/

SM_RESULT
DeviceMachine::EnterBootDeviceEvents()
{
    return SmCall(&BootDeviceEvents);
}

/* Configured device detached: purge its IO, then see whether it holds a system file */
SM_RESULT
DeviceMachine::PurgeOnDetach()
{
    return Request(&RequestingCriticalPassive, &DeviceMachine::NotifyAndPurgeIo,
                   &DeviceMachine::PurgedOnDetach);
}

VOID
DeviceMachine::NotifyAndPurgeIo()
{
    NotifyDisconnected();
    PurgeDeviceIo();
}

SM_RESULT
DeviceMachine::PurgedOnDetach(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    if (IsBootDevice())
        return BootDeviceDisableOnDetach();

    return DetachConfigured();
}

SM_RESULT
DeviceMachine::BootDeviceNotifyDetached()
{
    NotifyDisconnected();
    return BootDeviceDisableOnDetach();
}

SM_RESULT
DeviceMachine::BootDeviceDisableOnDetach()
{
    return Request(&RequestingCritical, &DeviceMachine::DisableEndpointsPowerLost,
                   &DeviceMachine::BootDeviceDisabledOnDetach);
}

VOID
DeviceMachine::DisableEndpointsPowerLost()
{
    DisableAllEndpoints();
    MarkPowerLost();
}

SM_RESULT
DeviceMachine::BootDeviceDisabledOnDetach(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    NotifyPortDetached();
    return SmTransition(&BootDeviceDetached);
}

/* The boot device failed: disable its endpoints and wait for a reset */
SM_RESULT
DeviceMachine::BootDeviceFailed()
{
    return Request(&RequestingCriticalPassive, &DeviceMachine::DisableEndpointsAndNotify,
                   &DeviceMachine::BootDeviceDisabledOnFailure);
}

VOID
DeviceMachine::DisableEndpointsAndNotify()
{
    DisableAllEndpoints();
    NotifyDisconnected();
}

SM_RESULT
DeviceMachine::BootDeviceDisabledOnFailure(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::ControllerIoctlDone)
        return SmUnhandled();

    m_ContextGone = FALSE;
    return SmTransition(&BootDeviceAwaitingReset);
}

/* Detached while a reset was running: the reset continues on reattach */
SM_RESULT
DeviceMachine::BootDeviceDetachDuringReset(
    _In_ BOOLEAN ContextGone)
{
    NotifyPortDetached();
    m_ContextGone = ContextGone;
    return SmTransition(&BootDeviceResetPending);
}

SM_RESULT
DeviceMachine::OnBootDeviceAwaitingReset(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ClientUnconfigure:
        case DsmEvent::ClientClearStall:
        case DsmEvent::ClientSyncResetPipe:
        case DsmEvent::ClientResetPipe:
        case DsmEvent::ClientSetInterface:
        case DsmEvent::ClientSelectConfig:
            FailClientRequest();
            return SmHandled();

        case DsmEvent::HubGetDescriptor:
            FailHubDescriptorRequest();
            return SmHandled();

        case DsmEvent::NoPingResponse:
        case DsmEvent::LpmSettingChanged:
            return SmHandled();

        case DsmEvent::DeviceTextQuery:
            SignalQueryText();
            return SmHandled();

        case DsmEvent::PdoPreStart:
            AckPreStart();
            return SmHandled();

        case DsmEvent::ClientResetDevice:
            return ReenumForBootClient(TRUE);

        case DsmEvent::PortDetached:
            if (m_ContextGone)
                return SmTransition(&BootDeviceDetachedContextGone);

            NotifyPortDetached();
            return SmTransition(&BootDeviceDetached);

        case DsmEvent::ClientStreams:
            return ForwardStreams(&BootDeviceAwaitingReset);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnterBootDeviceDetached()
{
    m_ContextGone = FALSE;
    MarkPowerLost();
    return SmCall(&BootDeviceEvents);
}

SM_RESULT
DeviceMachine::EnterBootDeviceDetachedContextGone()
{
    m_ContextGone = TRUE;
    NotifyPortDetached();
    MarkPowerLost();
    return SmCall(&BootDeviceEvents);
}

SM_RESULT
DeviceMachine::OnBootDeviceDetached(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ClientSetInterface:
        case DsmEvent::ClientSelectConfig:
        case DsmEvent::ClientResetPipe:
        case DsmEvent::ClientSyncResetPipe:
        case DsmEvent::ClientUnconfigure:
        case DsmEvent::ClientClearStall:
            FailClientRequest();
            return SmHandled();

        case DsmEvent::NoPingResponse:
        case DsmEvent::LpmSettingChanged:
            return SmHandled();

        case DsmEvent::HubGetDescriptor:
            FailHubDescriptorRequest();
            return SmHandled();

        case DsmEvent::DeviceTextQuery:
            SignalQueryText();
            return SmHandled();

        case DsmEvent::ClientResetDevice:
            return SmTransition(&BootDeviceResetPending);

        case DsmEvent::PortReattached:
            return SmTransition(&BootDeviceAwaitingReset);

        case DsmEvent::ClientStreams:
            return ForwardStreams(m_ContextGone ? &BootDeviceDetachedContextGone :
                                                   &BootDeviceDetached);

        default:
            return SmUnhandled();
    }
}

SM_RESULT
DeviceMachine::EnterBootDeviceResetPending()
{
    MarkPowerLost();
    return SmHandled();
}

SM_RESULT
DeviceMachine::OnBootDeviceResetPending(
    _In_ DsmEvent Event)
{
    if (Event != DsmEvent::PortReattached)
        return SmUnhandled();

    return ReenumForBootClient(TRUE);
}

SM_RESULT
DeviceMachine::OnBootDeviceEvents(
    _In_ DsmEvent Event)
{
    switch (Event)
    {
        case DsmEvent::ClientResetPipe:
        case DsmEvent::ClientClearStall:
        case DsmEvent::ClientSyncResetPipe:
            FailClientRequest();
            return SmHandled();

        case DsmEvent::LpmSettingChanged:
        case DsmEvent::NoPingResponse:
            return SmHandled();

        case DsmEvent::HubGetDescriptor:
            FailHubDescriptorRequest();
            return SmHandled();

        case DsmEvent::HubResumed:
        case DsmEvent::HubResumedWithReset:
        case DsmEvent::HubResumedInS0:
            ReferenceDevicePower();
            return SmTransition(&BootDeviceEvents);

        case DsmEvent::PdoInstallMsOsExt:
        case DsmEvent::PdoPowerDown:
        case DsmEvent::PdoPowerUp:
            SignalPnpWaiter();
            return SmTransition(&BootDeviceEvents);

        case DsmEvent::HubSuspending:
            DereferenceDevicePower();
            return SmTransition(&BootDeviceEvents);

        default:
            return SmUnhandled();
    }
}
