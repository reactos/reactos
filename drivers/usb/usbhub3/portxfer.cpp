/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Port control transfers and the port change decisions
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"

#define NDEBUG
#include <debug.h>

/* Over current recovery gives up after this many inside the window */
#define PORT_OVER_CURRENT_LIMIT         5
#define PORT_OVER_CURRENT_WINDOW        (3LL * 1000 * 1000 * 10)

static EVT_WDF_REQUEST_COMPLETION_ROUTINE PortControlComplete;

/*
 * USBXHCI answers a root port GET_STATUS with "operation pending" while it
 * finishes a resume itself; the hub must not idle in that window.
 */
static
VOID
NTAPI
PortTrackPendingOperation(
    _In_ HubPort* Port,
    _In_ USBD_STATUS UrbStatus)
{
    HubFdo* Hub = Port->m_Hub;

    if (UrbStatus == USBD_STATUS_PORT_OPERATION_PENDING)
    {
        if (!Port->HasFlag(PortFlag::PendingHubPowerReference) &&
            NT_SUCCESS(WdfDeviceStopIdle(Hub->m_Device, FALSE)))
        {
            DPRINT("Hub %p port %u resume pending, hub kept awake\n", Hub, Port->Number());
            Port->SetFlag(PortFlag::PendingHubPowerReference);
        }
    }
    else if (Port->HasFlag(PortFlag::PendingHubPowerReference))
    {
        WdfDeviceResumeIdle(Hub->m_Device);
        Port->ClearFlag(PortFlag::PendingHubPowerReference);
    }
}

/* The hub never reports a change for these, so the cache is updated by hand */
static
VOID
NTAPI
PortCacheSetFeature(
    _In_ HubPort* Port)
{
    const UCHAR* Setup = Port->m_Control.Urb.SetupPacket;
    USHORT Feature = Setup[2] | (Setup[3] << 8);
    USHORT LinkState = Setup[5];
    USHORT* Current = &Port->m_Current.StatusChange.PortStatus.AsUshort16;

    if (Feature == PORT_F_SUSPEND)
    {
        Port->m_PreviousStatus |= PS_SUSPENDED;
        *Current |= PS_SUSPENDED;
    }
    else if (Feature == PORT_F_LINK_STATE && (LinkState == LINK_U3 || LinkState == LINK_SS_DISABLED))
    {
        Port->m_PreviousStatus = (Port->m_PreviousStatus & ~PS_LINK_MASK) | (LinkState << PS_LINK_SHIFT);
        *Current = (*Current & ~PS_LINK_MASK) | (LinkState << PS_LINK_SHIFT);
    }
}

static
VOID
NTAPI
PortControlComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context)
{
    HubPort* Port = (HubPort*)Context;
    NTSTATUS Status = Params->IoStatus.Status;
    UCHAR RequestCode = Port->m_Control.Urb.SetupPacket[1];

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(Target);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p port %u control request 0x%x failed 0x%lx, URB status 0x%lx\n",
                Port->m_Hub,
                Port->Number(),
                RequestCode,
                Status,
                Port->m_Control.Urb.Hdr.Status);
        Port->m_Hub->m_FailureMessageId = HUB_MSG_CONTROL_TRANSFER_FAILED;
    }
    else if (RequestCode == HUB_REQUEST_GET_STATUS)
    {
        DPRINT("Hub %p port %u status 0x%x change 0x%x\n",
               Port->m_Hub,
               Port->Number(),
               Port->m_Current.StatusChange.PortStatus.AsUshort16,
               Port->m_Current.StatusChange.PortChange.AsUshort16);
        PortTrackPendingOperation(Port, Port->m_Control.Urb.Hdr.Status);
    }
    else if (RequestCode == HUB_REQUEST_SET_FEATURE)
    {
        PortCacheSetFeature(Port);
    }

    Port->m_Control.Reuse();
    Port->Post(NT_SUCCESS(Status) ? PortEvent::TransferDone : PortEvent::TransferFailed);
}

static
VOID
NTAPI
PortSendControl(
    _In_ HubPort* Port,
    _In_ UCHAR RequestType,
    _In_ UCHAR Request,
    _In_ USHORT Value,
    _In_ USHORT Index,
    _In_reads_bytes_opt_(Length) PVOID Buffer,
    _In_ USHORT Length)
{
    HubFdo* Hub = Port->m_Hub;
    NTSTATUS Status;

    Port->m_Control.SetSetup(RequestType, Request, Value, Index, Length);
    DPRINT("Hub %p port %u request 0x%x value %u index 0x%x\n", Hub, Port->Number(), Request, Value, Index);

    Status = Port->m_Control.Send(Hub->m_RootHubTarget,
                                  Hub->UsbDevice(),
                                  PortControlComplete,
                                  Port,
                                  Buffer,
                                  Length,
                                  FALSE,
                                  Hub->NeedsForwardProgress());
    if (!NT_SUCCESS(Status))
        Port->Post(PortEvent::TransferFailed);
}

/* wIndex with the port number in the low byte and a selector in the high byte */
static
USHORT
NTAPI
PortIndex(
    _In_ HubPort* Port,
    _In_ UCHAR High)
{
    return (USHORT)((Port->Number() & 0xFF) | (High << 8));
}

VOID
PortMachine::SendGetStatus()
{
    BOOLEAN Extended = m_Port->HasProperty(PortProperty::EnhancedSuperSpeed);

    PortSendControl(m_Port,
                    0xA3,
                    HUB_REQUEST_GET_STATUS,
                    Extended ? 2 : 0,
                    m_Port->Number(),
                    &m_Port->m_Current,
                    Extended ? sizeof(m_Port->m_Current) : sizeof(m_Port->m_Current.StatusChange));
}

/* The selection is cleared before sending so a fast completion cannot race it */
VOID
PortMachine::SendAckChange()
{
    USHORT Feature = m_Port->m_SelectedFeature;

    m_Port->m_SelectedFeature = HUB_FEATURE_NONE;
    PortSendControl(m_Port, 0x23, HUB_REQUEST_CLEAR_FEATURE, Feature, m_Port->Number(), NULL, 0);
}

/* A reset starts the SS.Inactive error suppression over */
VOID
PortMachine::SendReset()
{
    m_Port->m_LastChange = PortEvent::ChangeNone;
    PortSendControl(m_Port, 0x23, HUB_REQUEST_SET_FEATURE, PORT_F_RESET, m_Port->Number(), NULL, 0);
}

VOID
PortMachine::SendWarmReset()
{
    m_Port->m_LastChange = PortEvent::ChangeNone;
    PortSendControl(m_Port, 0x23, HUB_REQUEST_SET_FEATURE, PORT_F_BH_RESET, m_Port->Number(), NULL, 0);
}

VOID
PortMachine::SendDisable()
{
    PortSendControl(m_Port, 0x23, HUB_REQUEST_CLEAR_FEATURE, PORT_F_ENABLE, m_Port->Number(), NULL, 0);
}

VOID
PortMachine::SendResume()
{
    if (m_Port->IsUsb30())
        PortSendControl(m_Port, 0x23, HUB_REQUEST_SET_FEATURE, PORT_F_LINK_STATE, PortIndex(m_Port, LINK_U0), NULL, 0);
    else
        PortSendControl(m_Port, 0x23, HUB_REQUEST_CLEAR_FEATURE, PORT_F_SUSPEND, m_Port->Number(), NULL, 0);
}

VOID
PortMachine::SendSuspend()
{
    if (m_Port->IsUsb30())
        PortSendControl(m_Port, 0x23, HUB_REQUEST_SET_FEATURE, PORT_F_LINK_STATE, PortIndex(m_Port, LINK_U3), NULL, 0);
    else
        PortSendControl(m_Port, 0x23, HUB_REQUEST_SET_FEATURE, PORT_F_SUSPEND, m_Port->Number(), NULL, 0);
}

VOID
PortMachine::SendLinkDisable()
{
    PortSendControl(m_Port, 0x23, HUB_REQUEST_SET_FEATURE, PORT_F_LINK_STATE, PortIndex(m_Port, LINK_SS_DISABLED), NULL, 0);
}

VOID
PortMachine::SendLinkRxDetect()
{
    PortSendControl(m_Port, 0x23, HUB_REQUEST_SET_FEATURE, PORT_F_LINK_STATE, PortIndex(m_Port, LINK_RX_DETECT), NULL, 0);
}

/* Powering the port again ends an over current episode for user mode */
VOID
PortMachine::SendPortPower()
{
    if (m_Port->m_ConnectionStatus == DeviceCausedOvercurrent)
        m_Port->m_ConnectionStatus = NoDeviceConnected;

    PortSendControl(m_Port, 0x23, HUB_REQUEST_SET_FEATURE, PORT_F_POWER, m_Port->Number(), NULL, 0);
}

/* Only sent while a device is attached */
VOID
PortMachine::SendU1Timeout()
{
    PortSendControl(m_Port,
                    0x23,
                    HUB_REQUEST_SET_FEATURE,
                    PORT_F_U1_TIMEOUT,
                    PortIndex(m_Port, m_Port->m_Child->m_U1Timeout),
                    NULL,
                    0);
}

VOID
PortMachine::SendU2Timeout()
{
    PortSendControl(m_Port,
                    0x23,
                    HUB_REQUEST_SET_FEATURE,
                    PORT_F_U2_TIMEOUT,
                    PortIndex(m_Port, m_Port->m_Child->m_U2Timeout),
                    NULL,
                    0);
}

/* Wake on connect, disconnect and over current, or none */
VOID
PortMachine::SendRemoteWake(
    _In_ BOOLEAN Enable)
{
    PortSendControl(m_Port,
                    0x23,
                    HUB_REQUEST_SET_FEATURE,
                    PORT_F_REMOTE_WAKE_MASK,
                    PortIndex(m_Port, Enable ? 0x07 : 0x00),
                    NULL,
                    0);
}

/* Change selection */

/* Order of the changes, 2.0 and 3.0 column by column */
struct PortChangeOrder
{
    USHORT Bit20;
    USHORT Feature20;
    USHORT Bit30;
    USHORT Feature30;
};

static const PortChangeOrder PortChanges[] =
{
    { PC_CONNECT, PORT_C_CONNECTION, PC_CONNECT, PORT_C_CONNECTION },
    { PC_OVER_CURRENT, PORT_C_OVER_CURRENT, PC_OVER_CURRENT, PORT_C_OVER_CURRENT },
    { PC_RESET, PORT_C_RESET, PC_RESET, PORT_C_RESET },
    { PC_SUSPEND, PORT_C_SUSPEND, PC_LINK_STATE, PORT_C_LINK_STATE },
    { PC_ENABLE, PORT_C_ENABLE, PC_BH_RESET, PORT_C_BH_RESET },
    { 0, 0, PC_CONFIG_ERROR, PORT_C_CONFIG_ERROR }
};

/*
 * Bits already acked since the last accumulator reset are ignored, so a
 * hub that keeps reporting one change cannot loop the port.
 */
PSM_PENDING
PortMachine::PendingChange()
{
    USHORT* Change = &m_Port->m_Current.StatusChange.PortChange.AsUshort16;
    USHORT Bit;
    ULONG Index;

    *Change &= ~m_Port->m_ChangeAccumulator;

    for (Index = 0; Index < RTL_NUMBER_OF(PortChanges); Index++)
    {
        Bit = m_Usb3 ? PortChanges[Index].Bit30 : PortChanges[Index].Bit20;
        if (Bit == 0 || !(*Change & Bit))
            continue;

        *Change &= ~Bit;
        m_Port->m_ChangeAccumulator |= Bit;
        m_Port->m_SelectedFeature = m_Usb3 ? PortChanges[Index].Feature30 : PortChanges[Index].Feature20;

        if (Bit == PC_OVER_CURRENT)
            m_Port->m_ConnectionStatus = DeviceCausedOvercurrent;

        return PsmPendingChange;
    }

    if (*Change != 0)
        DPRINT1("Hub %p port %u reports unknown change 0x%x\n", m_Port->m_Hub, m_Port->Number(), *Change);

    return (*Change != 0) ? PsmPendingError : PsmPendingNone;
}

/* Change decisions */

/* Some hubs lose a change if they idle before reporting it */
static
VOID
NTAPI
PortDropInitialConnectReference(
    _In_ HubPort* Port)
{
    if (!Port->m_InitialConnectPowerReference)
        return;

    WdfDeviceResumeIdle(Port->m_Hub->m_Device);
    Port->m_InitialConnectPowerReference = FALSE;
}

/* The 2.0 flavor never answers "no change"; it answers an error where the 3.0 flavor would */
static
PortEvent
NTAPI
PortNextChange20(
    _In_ HubPort* Port,
    _In_ USHORT Status,
    _In_ USHORT Previous,
    _In_ USHORT Change)
{
    BOOLEAN Usable = (Status & PS_ENABLED) && !(Status & PS_SUSPENDED);

    if (Status & PS_OVER_CURRENT)
    {
        Port->m_ConnectionStatus = DeviceCausedOvercurrent;
        return PortEvent::ChangeOverCurrent;
    }

    if (Change & PC_OVER_CURRENT)
    {
        if (Previous & PS_OVER_CURRENT)
            return PortEvent::ChangeError;

        Port->m_ConnectionStatus = DeviceCausedOvercurrent;
        return PortEvent::ChangeOverCurrent;
    }

    if (!(Status & PS_POWER_20))
        return PortEvent::ChangeError;

    if ((Change & PC_CONNECT) || ((Status ^ Previous) & PS_CONNECTED))
        return PortEvent::ChangeConnect;

    if ((Status & PS_RESET) || !(Status & PS_CONNECTED))
        return PortEvent::ChangeError;

    if (Change & PC_RESET)
        return Usable ? PortEvent::ChangeResetDone : PortEvent::ChangeError;

    if (Change & PC_SUSPEND)
        return Usable ? PortEvent::ChangeResumed : PortEvent::ChangeError;

    return PortEvent::ChangeError;
}

static
PortEvent
NTAPI
PortNextChange30(
    _In_ HubPort* Port,
    _In_ USHORT Status,
    _In_ USHORT Previous,
    _In_ USHORT Change)
{
    HubFdo* Hub = Port->m_Hub;
    USHORT Link = HubLinkState(Status);
    USHORT PreviousLink = HubLinkState(Previous);
    BOOLEAN Connected = (Status & PS_CONNECTED) != 0;
    BOOLEAN ConnectChanged = (Change & PC_CONNECT) || ((Status ^ Previous) & PS_CONNECTED);

    if (Status & PS_OVER_CURRENT)
    {
        Port->m_ConnectionStatus = DeviceCausedOvercurrent;
        return PortEvent::ChangeOverCurrent;
    }

    if (Change & PC_OVER_CURRENT)
    {
        if (Previous & PS_OVER_CURRENT)
            return PortEvent::ChangeNone;

        Port->m_ConnectionStatus = DeviceCausedOvercurrent;
        return PortEvent::ChangeOverCurrent;
    }

    if (!(Status & PS_POWER_30))
        return PortEvent::ChangeNeedsHubReset;

    /* Caught in the middle of a reset */
    if (Status & PS_RESET)
    {
        if ((Status & PS_ENABLED) && !Hub->HasFlag(HubFlag::DiscardEnableDuringReset))
            return PortEvent::ChangeNeedsHubReset;
        if (Connected)
            return PortEvent::ChangeResetBusy;
        return ConnectChanged ? PortEvent::ChangeConnect : PortEvent::ChangeNone;
    }

    /* A disconnect can pass through Rx.Detect, or park in Polling on re-driver systems */
    if (!Connected && (Previous & PS_POWER_30))
    {
        if (Link == LINK_RX_DETECT && PreviousLink != LINK_RX_DETECT)
            return PortEvent::ChangeConnect;
        if (Link == LINK_POLLING && PreviousLink != LINK_POLLING)
            return PortEvent::ChangeConnect;
    }

    if (Link == LINK_LOOPBACK)
        return PortEvent::ChangeNeedsHubReset;

    if (Link == LINK_SS_DISABLED && PreviousLink != LINK_SS_DISABLED)
        return PortEvent::ChangeNeedsHubReset;

    if (Link == LINK_COMPLIANCE)
        return PortEvent::ChangeLinkError;

    /* The same SS.Inactive error is reported once until a reset */
    if (Link == LINK_SS_INACTIVE)
    {
        if (Change & PC_CONFIG_ERROR)
            return PortEvent::ChangeConnect;
        if (Port->m_LastChange == PortEvent::ChangeLinkError)
        {
            DPRINT1("Hub %p port %u still in SS.Inactive, already reported\n", Hub, Port->Number());
            return PortEvent::ChangeNone;
        }
        if ((Status & PS_ENABLED) && !Hub->HasFlag(HubFlag::IgnoreEnabledInSsInactive))
            return PortEvent::ChangeNeedsHubReset;
        return PortEvent::ChangeLinkError;
    }

    if (ConnectChanged)
        return PortEvent::ChangeConnect;

    if (!Connected)
    {
        if (Status & PS_ENABLED)
            return PortEvent::ChangeNeedsHubReset;
        if (Link <= LINK_U3)
            return Hub->HasFlag(HubFlag::ToleratesU0WhileDetached) ? PortEvent::ChangeNone : PortEvent::ChangeNeedsHubReset;
        return PortEvent::ChangeNone;
    }

    if (Link == LINK_RX_DETECT)
        return PortEvent::ChangeLinkError;

    if (!(Status & PS_ENABLED))
        return PortEvent::ChangeNeedsHubReset;

    if (Link > LINK_U3 && Link != LINK_RECOVERY)
        return PortEvent::ChangeNeedsHubReset;

    if (Change & PC_RESET)
        return (Link == LINK_U3) ? PortEvent::ChangeNeedsHubReset : PortEvent::ChangeResetDone;

    if (PreviousLink == LINK_U3 && Link != LINK_U3)
        return PortEvent::ChangeResumed;

    return PortEvent::ChangeNone;
}

/* Port status the hub should not report, and disconnects */
static
VOID
NTAPI
PortReportChange(
    _In_ HubPort* Port,
    _In_ PortEvent Result,
    _In_ USHORT Status,
    _In_ USHORT Previous,
    _In_ USHORT Change)
{
    const char* What;

    switch (Result)
    {
        case PortEvent::ChangeOverCurrent:
            What = "over current";
            break;

        case PortEvent::ChangeError:
            /* An empty 2.0 port with no change also answers ChangeError */
            if (!(Status & PS_CONNECTED) && Change == 0)
            {
                DPRINT("Hub %p port %u empty, no change\n", Port->m_Hub, Port->Number());
                return;
            }

            What = "invalid status";
            break;

        case PortEvent::ChangeNeedsHubReset:
            What = "invalid status, hub reset";
            break;

        case PortEvent::ChangeLinkError:
            What = "link error, port reset";
            break;

        case PortEvent::ChangeConnect:
            if (Status & PS_CONNECTED)
            {
                DPRINT("Hub %p port %u connect change\n", Port->m_Hub, Port->Number());
                return;
            }

            What = "disconnect";
            break;

        default:
            return;
    }

    DPRINT1("Hub %p port %u %s: status 0x%x previous 0x%x change 0x%x\n",
            Port->m_Hub,
            Port->Number(),
            What,
            Status,
            Previous,
            Change);
}

PortEvent
PortMachine::NextChange()
{
    USHORT Status = m_Port->m_Current.StatusChange.PortStatus.AsUshort16;
    USHORT Previous = m_Port->m_PreviousStatus;
    USHORT Change = m_Port->m_ChangeAccumulator;
    PortEvent Result;

    PortDropInitialConnectReference(m_Port);
    m_Port->m_PreviousStatus = Status;

    if (!m_Usb3)
    {
        Result = PortNextChange20(m_Port, Status, Previous, Change);
        PortReportChange(m_Port, Result, Status, Previous, Change);
        m_Port->m_LastChange = Result;
        return Result;
    }

    Result = PortNextChange30(m_Port, Status, Previous, Change);
    PortReportChange(m_Port, Result, Status, Previous, Change);

    if (Result == PortEvent::ChangeLinkError)
        m_Port->m_LinkErrorCount++;

    if (Result == PortEvent::ChangeNeedsHubReset)
    {
        m_Port->m_Hub->m_FailureMessageId = HUB_MSG_INVALID_PORT_STATUS;
        m_Port->m_Hub->m_LastResetPortStatus = Status | ((ULONG)Change << 16);
    }

    if (Result != PortEvent::ChangeNone)
        m_Port->m_LastChange = Result;

    return Result;
}

/* A connect change still in flight holds a single idle reference until it arrives */
PortEvent
PortMachine::LostChange()
{
    USHORT Status = m_Port->m_Current.StatusChange.PortStatus.AsUshort16;
    USHORT Change = m_Port->m_Current.StatusChange.PortChange.AsUshort16;
    USHORT Previous = m_Port->m_PreviousStatus;
    PortEvent Result = PortEvent::SubDone;

    m_Port->m_PreviousStatus = Status;

    if ((Status ^ Previous) & PS_CONNECTED)
    {
        if (!(Change & PC_CONNECT))
        {
            Result = PortEvent::ChangeConnect;
        }
        else if (!m_Port->m_InitialConnectPowerReference &&
                 NT_SUCCESS(WdfDeviceStopIdle(m_Port->m_Hub->m_Device, FALSE)))
        {
            m_Port->m_InitialConnectPowerReference = TRUE;
        }
    }
    else if ((Status & PS_OVER_CURRENT) && !(Previous & PS_OVER_CURRENT) && !(Change & PC_OVER_CURRENT))
    {
        m_Port->m_ConnectionStatus = DeviceCausedOvercurrent;
        Result = PortEvent::ChangeOverCurrent;
    }
    else if (!(Status & PS_OVER_CURRENT) && (Previous & PS_OVER_CURRENT) && !(Change & PC_OVER_CURRENT))
    {
        m_Port->m_ConnectionStatus = NoDeviceConnected;
        Result = PortEvent::ChangeOverCurrentCleared;
        DPRINT1("Hub %p port %u over current cleared without a change\n", m_Port->m_Hub, m_Port->Number());
    }
    else if (!(Status & PS_ENABLED) && (Previous & PS_ENABLED) && !(Change & PC_ENABLE))
    {
        Result = PortEvent::ChangeDisabled;
    }
    else if (!(Status & PS_SUSPENDED) && (Previous & PS_SUSPENDED) && !(Change & PC_SUSPEND))
    {
        Result = PortEvent::ChangeResumed;
    }

    PortReportChange(m_Port, Result, Status, Previous, Change);
    m_Port->m_LastChange = Result;
    return Result;
}

PortEvent
PortMachine::ErrorResponseDuringReset()
{
    USHORT Status = m_Port->m_Current.StatusChange.PortStatus.AsUshort16;
    USHORT Change = m_Port->m_ChangeAccumulator;

    DPRINT1("Hub %p port %u bad status during reset: status 0x%x change 0x%x\n",
            m_Port->m_Hub,
            m_Port->Number(),
            Status,
            Change);

    if (Change & PC_CONNECT)
        return (Status & PS_CONNECTED) ? PortEvent::ErrorIgnore : PortEvent::ErrorCycle;

    if (Change & PC_RESET)
    {
        if (!(Status & PS_CONNECTED))
            return (Status & PS_ENABLED) ? PortEvent::ErrorCycle : PortEvent::ChangeNeedsHubReset;

        if (Status & PS_SUSPENDED)
            return PortEvent::ErrorCycle;

        if (Status & PS_OVER_CURRENT)
        {
            m_Port->m_ConnectionStatus = DeviceCausedOvercurrent;
            return PortEvent::ChangeOverCurrent;
        }
    }

    if (Change & PC_ENABLE)
        return (Status & PS_ENABLED) ? PortEvent::ErrorEnabledWhileReset : PortEvent::ErrorIgnore;

    return PortEvent::ErrorIgnore;
}

PortEvent
PortMachine::ErrorResponse()
{
    if ((m_Port->m_ChangeAccumulator & PC_RESET) &&
        (m_Port->m_Current.StatusChange.PortStatus.AsUshort16 & PS_ENABLED))
    {
        return PortEvent::ErrorCycle;
    }

    return PortEvent::ErrorIgnore;
}

VOID
PortMachine::ClearPortStatus()
{
    m_Port->m_PreviousStatus = 0;
}

/* Intentionally empty; no port state needs fixing */
VOID
PortMachine::FixPortStateAfterPowerUp()
{
}

BOOLEAN
PortMachine::SuperSpeedBlocked()
{
    return m_Port->m_Hub->HasFlag(HubFlag::DisableSuperSpeed);
}

/* More than five over currents, each within three seconds of the last */
BOOLEAN
PortMachine::OverCurrentPersists()
{
    LARGE_INTEGER Now;

    KeQuerySystemTime(&Now);

    if (Now.QuadPart - m_Port->m_LastOverCurrentTime.QuadPart > PORT_OVER_CURRENT_WINDOW)
        m_Port->m_OverCurrentCount = 0;

    m_Port->m_LastOverCurrentTime = Now;
    return ++m_Port->m_OverCurrentCount > PORT_OVER_CURRENT_LIMIT;
}
