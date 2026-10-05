/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Device requests to the USB controller extension and endpoint programming
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"
#include "devhcd.h"

#define NDEBUG
#include <debug.h>

/* Size field of each device request: the full current layouts. UCX does not check Size. */
#define UCX_SIZE_ADDRESS0       sizeof(ADDRESS0_OWNERSHIP_ACQUIRE)
#define UCX_SIZE_ENABLE         sizeof(USBDEVICE_ENABLE)
#define UCX_SIZE_RESET          sizeof(USBDEVICE_RESET)
#define UCX_SIZE_ADDRESS        sizeof(USBDEVICE_ADDRESS)
#define UCX_SIZE_UPDATE         sizeof(USBDEVICE_UPDATE)
#define UCX_SIZE_DISABLE        sizeof(USBDEVICE_DISABLE)
#define UCX_SIZE_PURGE          sizeof(USBDEVICE_PURGEIO)
#define UCX_SIZE_START          sizeof(USBDEVICE_STARTIO)
#define UCX_SIZE_ABORT          sizeof(USBDEVICE_ABORTIO)
#define UCX_SIZE_CONFIGURE      sizeof(ENDPOINTS_CONFIGURE)
#define UCX_SIZE_EP0_UPDATE     sizeof(DEFAULT_ENDPOINT_UPDATE)
#define UCX_SIZE_EP_RESET       sizeof(ENDPOINT_RESET)

#ifndef _WIN64
C_ASSERT(UCX_SIZE_UPDATE == 0x30);
C_ASSERT(UCX_SIZE_RESET == 0x18);
#endif

/* Default endpoint packet size the controller starts with, per speed */
#define EP0_PACKET_SUPER        512
#define EP0_PACKET_HIGH_FULL    64
#define EP0_PACKET_LOW          8

/* How long a bandwidth failure waits before the user is told */
#define BANDWIDTH_RETRY_DELAY_MS  1000

/* Enumeration failure text for a failed SET_ADDRESS */
#define HUB_MSG_SET_ADDRESS_FAILED 0x40010001

/* Resume signaling a hub below the root port gives its downstream port, in ms */
#define HUB_ROOT_RESUME_SIGNAL_MS    20

/* USB 2.0 LPM BESL used when the device has no baseline, 400 us */
#define LPM20_DEFAULT_BESL      4

static EVT_WDF_REQUEST_COMPLETION_ROUTINE HubChildUcxIoctlComplete;
static EVT_WDF_REQUEST_COMPLETION_ROUTINE HubChildClientRequestComplete;

/* Shared helpers */

static
VOID
NTAPI
HubChildSetPayloadHeader(
    _In_ HubChild* Child,
    _In_ ULONG Size)
{
    PUSBDEVICE_MGMT_HEADER Header = &Child->m_UcxPayload->Header;

    Header->Size = Size;
    Header->Hub = Child->m_Hub->UsbDevice();
    Header->UsbDevice = Child->m_UsbDevice;
}

static
HubUcxPayload*
NTAPI
HubChildNewPayload(
    _In_ HubChild* Child,
    _In_ ULONG Size)
{
    RtlZeroMemory(Child->m_UcxPayload, sizeof(*Child->m_UcxPayload));
    HubChildSetPayloadHeader(Child, Size);
    return Child->m_UcxPayload;
}

static
VOID
NTAPI
HubChildStartBandwidthTimer(
    _In_ HubChild* Child)
{
    WdfTimerStart(Child->m_BandwidthRetryTimer, WDF_REL_TIMEOUT_IN_MS(BANDWIDTH_RETRY_DELAY_MS));
}

static
VOID
NTAPI
HubChildStopBandwidthTimer(
    _In_ HubChild* Child)
{
    WdfTimerStop(Child->m_BandwidthRetryTimer, FALSE);
}

static
VOID
NTAPI
HubChildFailStatus(
    _In_ HubChild* Child,
    _In_ NTSTATUS NtStatus,
    _In_ USBD_STATUS UsbdStatus)
{
    Child->m_LastNtStatus = NtStatus;
    Child->m_LastUsbdStatus = UsbdStatus;
}

/* Appends the first list to the unchanged list, never past the shared capacity */
static
VOID
NTAPI
HubChildKeepUnchanged(
    _In_ HubChild* Child,
    _In_reads_(Count) UCXENDPOINT* Endpoints,
    _In_ ULONG Count)
{
    ULONG Index;

    for (Index = 0; Index < Count; Index++)
    {
        if (Child->m_UnchangedCount >= Child->m_EndpointArrayCapacity)
            break;

        Child->m_EndpointsUnchanged[Child->m_UnchangedCount++] = Endpoints[Index];
    }
}

VOID
NTAPI
HubConfigMovePipes(
    _In_opt_ HubConfiguration* Config,
    _In_ PipeState From,
    _In_ PipeState To)
{
    PLIST_ENTRY Entry;
    HubInterface* Interface;
    ULONG Index;

    if (Config == NULL)
        return;

    for (Entry = Config->Interfaces.Flink; Entry != &Config->Interfaces; Entry = Entry->Flink)
    {
        Interface = CONTAINING_RECORD(Entry, HubInterface, Link);

        for (Index = 0; Index < Interface->PipeCount; Index++)
        {
            if (Interface->Pipes[Index].State == From)
                Interface->Pipes[Index].State = To;
        }
    }
}

static
VOID
NTAPI
HubInterfaceSetPipes(
    _In_ HubInterface* Interface,
    _In_ PipeState To)
{
    ULONG Index;

    for (Index = 0; Index < Interface->PipeCount; Index++)
        Interface->Pipes[Index].State = To;
}

/* Request channel */

NTSTATUS
NTAPI
HubChildSubmitUcxIoctl(
    _In_ HubChild* Child,
    _In_ ULONG IoControlCode)
{
    WDFIOTARGET Target = Child->m_Hub->m_RootHubTarget;
    WDFREQUEST Request = Child->m_UcxRequest;
    WDF_REQUEST_REUSE_PARAMS Reuse;
    NTSTATUS Status;

    WDF_REQUEST_REUSE_PARAMS_INIT(&Reuse, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_SUCCESS);
    Status = WdfRequestReuse(Request, &Reuse);
    if (!NT_SUCCESS(Status))
        DPRINT1("Device %p UCX request reuse failed 0x%lx\n", Child, Status);

    Child->m_UcxIoctl = IoControlCode;

    /* The request was made on the local target; that stack is at least as deep */
    Status = WdfIoTargetFormatRequestForInternalIoctlOthers(Target,
                                                            Request,
                                                            IoControlCode,
                                                            Child->m_UcxPayloadMemory,
                                                            NULL,
                                                            NULL,
                                                            NULL,
                                                            NULL,
                                                            NULL);
    if (NT_SUCCESS(Status))
    {
        WdfRequestSetCompletionRoutine(Request, HubChildUcxIoctlComplete, Child);

        if (WdfRequestSend(Request, Target, WDF_NO_SEND_OPTIONS))
            return STATUS_SUCCESS;

        Status = WdfRequestGetStatus(Request);
    }

    DPRINT1("Device %p UCX request 0x%lx not sent 0x%lx\n", Child, IoControlCode, Status);
    HubChildFailStatus(Child, Status, HubNtStatusToUsbd(Status));
    Child->Post(DsmEvent::ControllerIoctlFailed);
    return Status;
}

static
NTSTATUS
NTAPI
HubChildEnableDone(
    _In_ HubChild* Child,
    _In_ NTSTATUS Status)
{
    PUSBDEVICE_ENABLE Enable = &Child->m_UcxPayload->Enable;

    if (Enable->FailureFlags.InsufficientHardwareResourcesForDefaultEndpoint ||
        Enable->FailureFlags.InsufficientHardwareResourcesForDevice)
    {
        DPRINT("Device %p enable ran out of controller resources\n", Child);
        Child->m_LastUsbdStatus = USBD_STATUS_NO_BANDWIDTH;
        HubChildStartBandwidthTimer(Child);
        return STATUS_UNSUCCESSFUL;
    }

    if (NT_SUCCESS(Status))
    {
        HubChildStopBandwidthTimer(Child);
        return Status;
    }

    /* A generic failure is reported as out of resources */
    if (Status == STATUS_UNSUCCESSFUL)
        return STATUS_INSUFFICIENT_RESOURCES;

    return Status;
}

/** FALSE when the completion already posted its own event. */
static
BOOLEAN
NTAPI
HubChildConfigureDone(
    _In_ HubChild* Child,
    _Inout_ NTSTATUS* Status)
{
    PENDPOINTS_CONFIGURE Configure = &Child->m_UcxPayload->Configure;
    BOOLEAN Failed;

    HubConfigMovePipes(Child->m_CurrentConfig, PipeState::PendingDisable, PipeState::Disabled);
    HubConfigMovePipes(Child->m_OldConfig, PipeState::PendingDisable, PipeState::Disabled);
    if (Child->m_OldInterface != NULL)
        HubInterfaceSetPipes(Child->m_OldInterface, PipeState::Disabled);

    if (Configure->ExitLatencyDelta != 0)
    {
        /* The ELD exit latency is cleared either way */
        if (Configure->FailureFlags.MaxExitLatencyTooLarge)
        {
            DPRINT1("Device %p exit latency delta %lu rejected, effective latency %u\n",
                    Child,
                    Configure->ExitLatencyDelta,
                    Child->m_EffectiveExitLatency);
            Child->m_MaxExitLatencyFromEld = 0;
        }
        else if (NT_SUCCESS(*Status))
        {
            Child->m_MaxExitLatencyFromEld = Child->m_EffectiveExitLatency + Configure->ExitLatencyDelta;
            DPRINT("Device %p max exit latency from delta is %lu\n", Child, Child->m_MaxExitLatencyFromEld);
        }
        else
        {
            Child->m_MaxExitLatencyFromEld = 0;
        }
    }

    Failed = Configure->FailureFlags.InsufficientBandwidth ||
             Configure->FailureFlags.InsufficientHardwareResourcesForEndpoints ||
             Configure->FailureFlags.MaxExitLatencyTooLarge;

    if (Failed)
    {
        DPRINT("Device %p endpoint configure ran out of bandwidth or resources\n", Child);
        Child->m_LastUsbdStatus = USBD_STATUS_NO_BANDWIDTH;
        *Status = STATUS_UNSUCCESSFUL;

        /* Older controllers report a latency problem as a resource problem; retry without U1 and U2 */
        if (Child->m_EffectiveExitLatency != 0)
        {
            DPRINT1("Device %p endpoint configure failed at exit latency %u, retrying without U1 and U2\n",
                    Child,
                    Child->m_EffectiveExitLatency);
            HubChildKeepUnchanged(Child, Child->m_EndpointsToDisable, Child->m_DisableCount);
            Child->m_DisableCount = 0;

            /* The USBD code goes into the NT status here */
            Child->m_LastNtStatus = (NTSTATUS)HubNtStatusToUsbd(STATUS_UNSUCCESSFUL);
            Child->Post(DsmEvent::ControllerExitLatencyTooLarge);
            return FALSE;
        }

        HubChildStartBandwidthTimer(Child);
    }

    if (InterlockedAnd(&Child->m_StateFlags, ~(LONG)ChildState::AltSettingFiltered) &
        (LONG)ChildState::AltSettingFiltered)
    {
        DPRINT1("Device %p select failed by the alternate setting filter\n", Child);
        Child->m_LastUsbdStatus = USBD_STATUS_NO_BANDWIDTH;
        *Status = STATUS_UNSUCCESSFUL;
        HubChildStartBandwidthTimer(Child);
    }
    else if (Child->m_EnableCount != 0 && NT_SUCCESS(*Status))
    {
        HubChildStopBandwidthTimer(Child);
    }
    else if (*Status == STATUS_UNSUCCESSFUL)
    {
        *Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    HubConfigMovePipes(Child->m_CurrentConfig,
                       PipeState::PendingEnable,
                       NT_SUCCESS(*Status) ? PipeState::Enabled : PipeState::Disabled);

    Child->m_DisableCount = 0;
    Child->m_EnableCount = 0;
    Child->m_UnchangedCount = 0;
    return TRUE;
}

/* While any SuperSpeed device's last SET_ADDRESS failed, connects get more debounce time */
static
VOID
NTAPI
HubChildAddressDone(
    _In_ HubChild* Child,
    _In_ NTSTATUS Status)
{
    const LONG Flag = (LONG)ChildState::LastSetAddressFailed;

    if (NT_SUCCESS(Status))
    {
        Child->m_Address = (UCHAR)Child->m_UcxPayload->Address.Address;

        if (InterlockedAnd(&Child->m_StateFlags, ~Flag) & Flag)
        {
            DPRINT1("Device %p SET_ADDRESS works again after an earlier failure\n", Child);
            InterlockedDecrement(&HubDriver.SuperSpeedDebounceVotes);
        }

        return;
    }

    Child->m_EnumMessageId = HUB_MSG_SET_ADDRESS_FAILED;

    if ((Child->m_Kind & DSM_KIND_PORT30) &&
        !(InterlockedOr(&Child->m_StateFlags, Flag) & Flag))
    {
        DPRINT1("Device %p SET_ADDRESS failed, SuperSpeed connects now get extra debounce time\n", Child);
        InterlockedIncrement(&HubDriver.SuperSpeedDebounceVotes);
    }
}

/* A controller that leaves the answer at 0 keeps the device at "unknown" */
static
VOID
NTAPI
HubChildTakeTunnelState(
    _In_ HubChild* Child)
{
    PUSBDEVICE_UPDATE Update = &Child->m_UcxPayload->Update;
    UCHAR State;

    if (!Update->Flags.UpdateTunnelState)
        return;

    State = UcxHubUpdateTunnelAnswer(Update)->TunnelState;
    if (State == HUB_TUNNEL_STATE_NONE)
    {
        DPRINT1("Device %p controller gave no USB4 tunnel state\n", Child);
        return;
    }

    DPRINT("Device %p USB4 tunnel state %u\n", Child, State);
    Child->m_TunnelState = State;
}

static
VOID
NTAPI
HubChildUcxIoctlComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context)
{
    HubChild* Child = (HubChild*)Context;
    NTSTATUS Status = Params->IoStatus.Status;

    UNREFERENCED_PARAMETER(Request);
    UNREFERENCED_PARAMETER(Target);

    switch (Child->m_UcxIoctl)
    {
        case IOCTL_UCXHUB_DEVICE_ENABLE:
            Status = HubChildEnableDone(Child, Status);
            break;

        case IOCTL_UCXHUB_ENDPOINTS_CONFIGURE:
            if (!HubChildConfigureDone(Child, &Status))
                return;
            break;

        case IOCTL_UCXHUB_DEVICE_ADDRESS:
            HubChildAddressDone(Child, Status);
            break;

        case IOCTL_UCXHUB_DEVICE_RESET:
            HubConfigMovePipes(Child->m_CurrentConfig, PipeState::PendingDisable, PipeState::Disabled);
            break;

        case IOCTL_UCXHUB_DEVICE_UPDATE:
            /* Every successful update counts as taking the target latency, even ones without it */
            if (NT_SUCCESS(Status))
            {
                Child->m_EffectiveExitLatency = Child->m_TargetExitLatency;
                HubChildTakeTunnelState(Child);
            }
            else if (Child->m_UcxPayload->Update.FailureFlags.MaxExitLatencyTooLarge)
            {
                DPRINT1("Device %p exit latency %u too large for the controller 0x%lx\n",
                        Child,
                        Child->m_TargetExitLatency,
                        Status);
                Child->Post(DsmEvent::ControllerExitLatencyTooLarge);
                return;
            }
            break;

        default:
            break;
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Device %p UCX request 0x%lx failed 0x%lx\n", Child, Child->m_UcxIoctl, Status);

        /* A USBD status left over from an earlier failure wins */
        Child->m_LastNtStatus = Status;
        if (Child->m_LastUsbdStatus == 0)
            Child->m_LastUsbdStatus = HubNtStatusToUsbd(Status);
    }

    Child->Post(NT_SUCCESS(Status) ? DsmEvent::ControllerIoctlDone : DsmEvent::ControllerIoctlFailed);
}

VOID
DeviceMachine::CancelControllerIoctl()
{
    if (!WdfRequestCancelSentRequest(m_Device->m_UcxRequest))
        DPRINT1("Device %p UCX request 0x%lx was not canceled\n", m_Device, m_Device->m_UcxIoctl);
}

/* Address zero */

VOID
DeviceMachine::ClaimAddressZero()
{
    HubChildNewPayload(m_Device, UCX_SIZE_ADDRESS0);
    HubChildSubmitUcxIoctl(m_Device, IOCTL_UCXHUB_ADDRESS0_OWNERSHIP_ACQUIRE);
}

VOID
DeviceMachine::ReleaseAddressZero()
{
    HubFdo* Hub = m_Device->m_Hub;

    Hub->m_Stack.Address0Release(Hub->UsbDevice(), m_Device->m_UsbDevice);
}

/* Device and default endpoint objects */

BOOLEAN
DeviceMachine::CreateDeviceInController()
{
    HubChild* Child = m_Device;
    HubFdo* Hub = Child->m_Hub;
    PUCXHUB_DEVICE_CREATE_INFO Info = &Child->m_CreateInfo;
    UCXUSBDEVICE UsbDevice;
    NTSTATUS Status;

    Info->Size = sizeof(*Info);

    if (Child->m_Kind & DSM_KIND_SUPER_SPEED)
    {
        Info->DeviceSpeed = UsbSuperSpeed;
        Child->m_MaxPacketSize0 = EP0_PACKET_SUPER;
    }
    else if (Child->m_Kind & DSM_KIND_HIGH_SPEED)
    {
        Info->DeviceSpeed = UsbHighSpeed;
        Child->m_MaxPacketSize0 = EP0_PACKET_HIGH_FULL;
    }
    else if (Child->m_Kind & DSM_KIND_LOW_SPEED)
    {
        Info->DeviceSpeed = UsbLowSpeed;
        Child->m_MaxPacketSize0 = EP0_PACKET_LOW;
    }
    else
    {
        Info->DeviceSpeed = UsbFullSpeed;
        Child->m_MaxPacketSize0 = EP0_PACKET_HIGH_FULL;
    }

    Info->PortNumber = Child->m_Port->Number();
    Info->HubDeviceContext = (UCXHUB_DEVICE_CONTEXT)Child;

    Status = Hub->m_Stack.DeviceCreate(Hub->UsbDevice(), Info, &UsbDevice);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Port %u controller device create failed 0x%lx\n", Child->m_Port->Number(), Status);
        HubChildFailStatus(Child, STATUS_INSUFFICIENT_RESOURCES, USBD_STATUS_INSUFFICIENT_RESOURCES);
        return FALSE;
    }

    Child->m_UsbDevice = UsbDevice;
    return TRUE;
}

BOOLEAN
DeviceMachine::CreateDefaultEndpoint()
{
    HubChild* Child = m_Device;
    HubFdo* Hub = Child->m_Hub;
    UCXENDPOINT Endpoint;
    NTSTATUS Status;

    Status = Hub->m_Stack.CreateControlEndpoint(Hub->UsbDevice(),
                                                Child->m_UsbDevice,
                                                Child->m_MaxPacketSize0,
                                                &Endpoint);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Device %p default endpoint create failed 0x%lx\n", Child, Status);
        HubChildFailStatus(Child, STATUS_INSUFFICIENT_RESOURCES, USBD_STATUS_INSUFFICIENT_RESOURCES);
        return FALSE;
    }

    Child->m_DefaultEndpoint = Endpoint;
    return TRUE;
}

VOID
DeviceMachine::DeleteDefaultEndpoint()
{
    HubFdo* Hub = m_Device->m_Hub;

    Hub->m_Stack.EndpointDelete(Hub->UsbDevice(), m_Device->m_UsbDevice, m_Device->m_DefaultEndpoint);
    m_Device->m_DefaultEndpoint = NULL;
}

/* The handle stays; the machine never uses it again after this */
VOID
DeviceMachine::DeleteDeviceInController()
{
    HubFdo* Hub = m_Device->m_Hub;

    Hub->m_Stack.DeviceDelete(Hub->UsbDevice(), m_Device->m_UsbDevice);
}

/* Enable, disable, address, update */

VOID
DeviceMachine::EnableDeviceInController()
{
    HubUcxPayload* Payload = HubChildNewPayload(m_Device, UCX_SIZE_ENABLE);

    Payload->Enable.DefaultEndpoint = m_Device->m_DefaultEndpoint;
    HubChildSubmitUcxIoctl(m_Device, IOCTL_UCXHUB_DEVICE_ENABLE);
}

VOID
DeviceMachine::DisableDeviceInController()
{
    HubUcxPayload* Payload = HubChildNewPayload(m_Device, UCX_SIZE_DISABLE);

    Payload->Disable.DefaultEndpoint = m_Device->m_DefaultEndpoint;
    HubChildSubmitUcxIoctl(m_Device, IOCTL_UCXHUB_DEVICE_DISABLE);
}

VOID
DeviceMachine::UpdateDefaultEndpoint()
{
    HubUcxPayload* Payload = HubChildNewPayload(m_Device, UCX_SIZE_EP0_UPDATE);

    Payload->DefaultEndpoint.DefaultEndpoint = m_Device->m_DefaultEndpoint;
    Payload->DefaultEndpoint.MaxPacketSize = m_Device->m_MaxPacketSize0;
    HubChildSubmitUcxIoctl(m_Device, IOCTL_UCXHUB_DEFAULT_ENDPOINT_UPDATE);
}

/* Address 0 in the request lets the controller pick one and report it back */
VOID
DeviceMachine::SetDeviceAddress()
{
    HubUcxPayload* Payload = HubChildNewPayload(m_Device, UCX_SIZE_ADDRESS);

    Payload->Address.Address = 0;
    HubChildSubmitUcxIoctl(m_Device, IOCTL_UCXHUB_DEVICE_ADDRESS);
}

/* USB 2.0 LPM attributes of the device, as the BOS validator accepts them */
struct HubUsb20Lpm
{
    BOOLEAN Capable;
    BOOLEAN Besl;
    BOOLEAN BaselineValid;
    BOOLEAN DeepValid;
    UCHAR Baseline;
    UCHAR Deep;
};

/* Combined over every USB 2.0 extension in the BOS; attributes are only ever added */
static
VOID
NTAPI
HubChildReadUsb20Lpm(
    _In_ HubChild* Child,
    _Out_ HubUsb20Lpm* Lpm)
{
    PUSB_BOS_DESCRIPTOR Bos = Child->m_Bos;
    PUSB_DEVICE_CAPABILITY_USB20_EXTENSION_DESCRIPTOR Extension;
    PUSB_COMMON_DESCRIPTOR Common;
    PUCHAR Current;
    PUCHAR End;
    UCHAR Baseline;
    UCHAR Deep;

    RtlZeroMemory(Lpm, sizeof(*Lpm));

    if (Bos == NULL)
        return;

    Current = (PUCHAR)Bos + Bos->bLength;
    End = (PUCHAR)Bos + Bos->wTotalLength;

    for (; Current + sizeof(*Common) <= End; Current += Common->bLength)
    {
        Common = (PUSB_COMMON_DESCRIPTOR)Current;
        if (Common->bLength == 0)
            break;

        Extension = (PUSB_DEVICE_CAPABILITY_USB20_EXTENSION_DESCRIPTOR)Current;

        if (Common->bDescriptorType != USB_DEVICE_CAPABILITY_DESCRIPTOR_TYPE ||
            Common->bLength < sizeof(*Extension) ||
            Current + sizeof(*Extension) > End ||
            Extension->bDevCapabilityType != USB_DEVICE_CAPABILITY_USB20_EXTENSION ||
            !Extension->bmAttributes.LPMCapable)
        {
            continue;
        }

        Baseline = (UCHAR)Extension->bmAttributes.BaselineBESL;
        Deep = (UCHAR)Extension->bmAttributes.DeepBESL;

        Lpm->Capable = TRUE;
        if (Extension->bmAttributes.BESLAndAlternateHIRDSupported)
            Lpm->Besl = TRUE;

        if (Extension->bmAttributes.BaselineBESLValid && Baseline != 0)
        {
            Lpm->BaselineValid = TRUE;
            Lpm->Baseline = Baseline;
        }

        /* A deep value not above a valid baseline is ignored */
        if (Extension->bmAttributes.DeepBESLValid && Deep != 0 &&
            !(Extension->bmAttributes.BaselineBESLValid && Deep <= Baseline))
        {
            Lpm->DeepValid = TRUE;
            Lpm->Deep = Deep;
        }
    }
}

/* Fills the USB 2.0 hardware LPM part of an update; TRUE when LPM is on */
static
BOOLEAN
NTAPI
HubChildComputeLpm20(
    _In_ HubChild* Child,
    _Inout_ PUSBDEVICE_UPDATE Update)
{
    HubLpm20Status Result;
    HubUsb20Lpm Lpm;

    HubChildReadUsb20Lpm(Child, &Lpm);

    if (Child->HasProperty(ChildProperty::IsHub))
        Result = HubLpm20Status::DeviceIsHub;
    else if (Child->HasHack(ChildHack::DisableUsb20Lpm))
        Result = HubLpm20Status::DeviceHack;
    else if (Child->m_Hub->HasFlag(HubFlag::DisableUsb20Lpm))
        Result = HubLpm20Status::HubHack;
    else if (!HubDriver.HasFlag(HubGlobal::EnableUsb20HardwareLpm))
        Result = HubLpm20Status::GloballyDisabled;
    else if (!Lpm.Capable)
        Result = HubLpm20Status::DeviceNotCapable;
    else if (!Lpm.Besl)
        Result = HubLpm20Status::FirstGenerationDevice;
    else if (!Child->m_Port->HasProperty(PortProperty::Usb20LpmCapable))
        Result = HubLpm20Status::PortNotCapable;
    else
        Result = HubLpm20Status::Enabled;

    if (Result == HubLpm20Status::Enabled)
    {
        Update->Flags.Update20HardwareLpmParameters = 1;
        Update->Usb20HardwareLpmParameters.HardwareLpmEnable = 1;
        Update->Usb20HardwareLpmParameters.L1Timeout = HubDriver.Usb20LpmTimeout;
        Update->Usb20HardwareLpmParameters.RemoteWakeEnable =
            Child->HasHack(ChildHack::DisableRemoteWakeForUsb20Lpm) ? 0 : 1;
        Update->Usb20HardwareLpmParameters.BestEffortServiceLatency = LPM20_DEFAULT_BESL;
        Update->Usb20HardwareLpmParameters.BestEffortServiceLatencyDeep = 0;
        Update->Usb20HardwareLpmParameters.HostInitiatedResumeDurationMode = 0;

        if (Child->m_Port->HasProperty(PortProperty::BeslCapable))
        {
            if (Lpm.BaselineValid)
                Update->Usb20HardwareLpmParameters.BestEffortServiceLatency = Lpm.Baseline;

            if (Lpm.DeepValid)
            {
                Update->Usb20HardwareLpmParameters.BestEffortServiceLatencyDeep = Lpm.Deep;
                Update->Usb20HardwareLpmParameters.HostInitiatedResumeDurationMode = 1;
            }
        }
    }

    /* Diagnostic only; the parameters still go out and UCX ignores them */
    Child->m_Lpm20Status = (Child->m_Kind & DSM_KIND_SUPER_SPEED) ? (ULONG)HubLpm20Status::RunningAtSuperSpeed
                                                                  : (ULONG)Result;
    DPRINT("Device %p USB 2.0 LPM status %lu\n", Child, Child->m_Lpm20Status);

    return Result == HubLpm20Status::Enabled;
}

VOID
DeviceMachine::UpdateDeviceInController()
{
    HubChild* Child = m_Device;
    HubUcxPayload* Payload = HubChildNewPayload(Child, UCX_SIZE_UPDATE);
    PUSBDEVICE_UPDATE Update = &Payload->Update;
    BOOLEAN IsHub = Child->HasProperty(ChildProperty::IsHub);

    /* Only ports bound to a USB4 host ask; the U2 decision below sees the preset, not the answer */
    Child->m_TunnelState = HUB_TUNNEL_STATE_NOT_USB4;
    if (Child->m_Port->HasFlag(PortFlag::Usb4Host))
    {
        Child->m_TunnelState = HUB_TUNNEL_STATE_UNKNOWN;
        Update->Flags.UpdateTunnelState = 1;
    }

    Update->Flags.UpdateDeviceDescriptor = 1;
    Update->Flags.UpdateBosDescriptor = 1;
    Update->Flags.UpdateMaxExitLatency = 1;
    Update->Flags.UpdateIsHub = 1;
    if (Child->HasHack(ChildHack::TolerateStalePipes))
        Update->Flags.UpdateAllowIoOnInvalidPipeHandles = 1;

    Update->DeviceDescriptor = &Child->m_DeviceDescriptor;
    Update->BosDescriptor = Child->m_Bos;
    Update->MaxExitLatency = HubChildU2ForEnumerated(Child) ? Child->m_HostU2ExitLatency : 0;
    Update->IsHub = IsHub;

    HubChildComputeLpm20(Child, Update);

    /* Only a root port uses the device's own resume signaling time */
    if (Child->m_HasResumeRecoveryTime &&
        Child->HasProperty(ChildProperty::NotRemovable) &&
        !IsHub &&
        Child->m_Port->m_Info.Protocol == 0x200)
    {
        Update->Flags.UpdateRootPortResumeTime = 1;
        Update->RootPortResumeTime = Child->m_Hub->IsRootHub() ? Child->m_ResumeSignalingTime
                                                               : HUB_ROOT_RESUME_SIGNAL_MS;
    }

    HubChildSubmitUcxIoctl(Child, IOCTL_UCXHUB_DEVICE_UPDATE);
}

/* FALSE sends nothing; TRUE means an event comes, even when the send failed */
BOOLEAN
DeviceMachine::UpdateLpm20()
{
    HubChild* Child = m_Device;

    RtlZeroMemory(Child->m_UcxPayload, sizeof(*Child->m_UcxPayload));
    if (!HubChildComputeLpm20(Child, &Child->m_UcxPayload->Update))
        return FALSE;

    HubChildSetPayloadHeader(Child, UCX_SIZE_UPDATE);
    HubChildSubmitUcxIoctl(Child, IOCTL_UCXHUB_DEVICE_UPDATE);
    return TRUE;
}

VOID
DeviceMachine::UpdateExitLatency()
{
    HubChild* Child = m_Device;
    HubUcxPayload* Payload;

    if (Child->m_MaxExitLatencyFromEld != 0 &&
        Child->m_TargetExitLatency > Child->m_MaxExitLatencyFromEld)
    {
        DPRINT1("Device %p exit latency %u is above the controller limit %lu\n",
                Child,
                Child->m_TargetExitLatency,
                Child->m_MaxExitLatencyFromEld);
        Child->Post(DsmEvent::ControllerExitLatencyTooLarge);
        return;
    }

    Payload = HubChildNewPayload(Child, UCX_SIZE_UPDATE);
    Payload->Update.Flags.UpdateMaxExitLatency = 1;
    Payload->Update.MaxExitLatency = Child->m_TargetExitLatency;
    HubChildSubmitUcxIoctl(Child, IOCTL_UCXHUB_DEVICE_UPDATE);
}

/*
 * The enabled endpoints go into the request as PendingDisable; the
 * device's own disable count is left alone.
 */
VOID
DeviceMachine::NotifyDeviceReset()
{
    HubChild* Child = m_Device;
    HubConfiguration* Config = Child->m_CurrentConfig;
    UCXENDPOINT* Endpoints = NULL;
    HubUcxPayload* Payload;
    HubInterface* Interface;
    PLIST_ENTRY Entry;
    ULONG Count = 0;
    ULONG Index;
    NTSTATUS Status = STATUS_SUCCESS;

    if (Config != NULL && Child->m_EndpointArrayCapacity != 0)
    {
        Endpoints = Child->m_EndpointsToDisable;

        for (Entry = Config->Interfaces.Flink;
             Entry != &Config->Interfaces && NT_SUCCESS(Status);
             Entry = Entry->Flink)
        {
            Interface = CONTAINING_RECORD(Entry, HubInterface, Link);

            for (Index = 0; Index < Interface->PipeCount; Index++)
            {
                if (Interface->Pipes[Index].State != PipeState::Enabled)
                    continue;

                if (Endpoints == NULL)
                {
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }

                Interface->Pipes[Index].State = PipeState::PendingDisable;
                Endpoints[Count++] = Interface->Pipes[Index].Endpoint;
            }
        }

        /* Fails the request so the device machine does not wait forever */
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Device %p reset has enabled endpoints but no disable list 0x%lx\n", Child, Status);
            HubChildFailStatus(Child, Status, HubNtStatusToUsbd(Status));
            Child->Post(DsmEvent::ControllerIoctlFailed);
        }
    }

    if (NT_SUCCESS(Status))
    {
        Payload = HubChildNewPayload(Child, UCX_SIZE_RESET);
        Payload->Reset.DefaultEndpoint = Child->m_DefaultEndpoint;
        Payload->Reset.EndpointsToDisableCount = Count;
        Payload->Reset.EndpointsToDisable = Endpoints;

        Status = HubChildSubmitUcxIoctl(Child, IOCTL_UCXHUB_DEVICE_RESET);
    }

    if (!NT_SUCCESS(Status))
        HubConfigMovePipes(Config, PipeState::PendingDisable, PipeState::Enabled);
}

/* I/O control */

VOID
DeviceMachine::AbortDeviceIo()
{
    HubChildNewPayload(m_Device, UCX_SIZE_ABORT);
    HubChildSubmitUcxIoctl(m_Device, IOCTL_UCXHUB_DEVICE_ABORT_IO);
}

VOID
DeviceMachine::PurgeDeviceIo()
{
    HubUcxPayload* Payload = HubChildNewPayload(m_Device, UCX_SIZE_PURGE);

    Payload->Purge.OnSuspend = FALSE;
    HubChildSubmitUcxIoctl(m_Device, IOCTL_UCXHUB_DEVICE_PURGE_IO);
}

VOID
DeviceMachine::PurgeIoForSuspend()
{
    HubUcxPayload* Payload = HubChildNewPayload(m_Device, UCX_SIZE_PURGE);

    Payload->Purge.OnSuspend = TRUE;
    HubChildSubmitUcxIoctl(m_Device, IOCTL_UCXHUB_DEVICE_PURGE_IO);
}

/* The tree purge uses the plain purge size */
VOID
DeviceMachine::PurgeDeviceTreeIo()
{
    HubChildNewPayload(m_Device, UCX_SIZE_PURGE);
    HubChildSubmitUcxIoctl(m_Device, IOCTL_UCXHUB_DEVICE_TREE_PURGE_IO);
}

VOID
DeviceMachine::StartDeviceIo()
{
    HubChildNewPayload(m_Device, UCX_SIZE_START);
    HubChildSubmitUcxIoctl(m_Device, IOCTL_UCXHUB_DEVICE_START_IO);
}

/* Endpoint reset */

static
VOID
NTAPI
HubChildResetTargetEndpoint(
    _In_ HubChild* Child,
    _In_ ULONG Flags)
{
    HubUcxPayload* Payload = HubChildNewPayload(Child, UCX_SIZE_EP_RESET);

    if (Child->m_TargetPipe == NULL)
    {
        DPRINT1("Device %p endpoint reset without a target pipe\n", Child);
        HubChildFailStatus(Child, STATUS_INVALID_PARAMETER, USBD_STATUS_INVALID_PIPE_HANDLE);
        Child->Post(DsmEvent::ControllerIoctlFailed);
        return;
    }

    Payload->EndpointReset.Endpoint = Child->m_TargetPipe->Endpoint;
    Payload->EndpointReset.Flags = (ENDPOINT_RESET_FLAGS)Flags;
    HubChildSubmitUcxIoctl(Child, IOCTL_UCXHUB_ENDPOINT_RESET);
}

VOID
DeviceMachine::ResetEndpoint()
{
    HubChildResetTargetEndpoint(m_Device, FlagEndpointResetPreserveTransferState);
}

VOID
DeviceMachine::ResetEndpointAndTransfers()
{
    HubChildResetTargetEndpoint(m_Device, 0);
}

/* Endpoint programming */

VOID
DeviceMachine::ProgramEndpoints()
{
    HubChild* Child = m_Device;
    BOOLEAN FailAlternate = Child->HasState(ChildState::AltSettingFiltered);
    PENDPOINTS_CONFIGURE Configure;

    /* Selecting a zero bandwidth setting takes the bandwidth popup away */
    if (Child->m_DisableCount == 0 && Child->m_EnableCount == 0)
    {
        DPRINT("Device %p has no endpoints to program\n", Child);
        HubChildStopBandwidthTimer(Child);
        Child->Post(DsmEvent::ControllerIoctlDone);
        return;
    }

    /* The flag stays set on this path */
    if (Child->m_DisableCount == 0 && FailAlternate)
    {
        DPRINT1("Device %p select failed by the alternate setting filter\n", Child);
        HubConfigMovePipes(Child->m_CurrentConfig, PipeState::PendingEnable, PipeState::Disabled);
        HubChildFailStatus(Child, STATUS_UNSUCCESSFUL, USBD_STATUS_NO_BANDWIDTH);
        Child->Post(DsmEvent::ControllerIoctlFailed);
        return;
    }

    Configure = &HubChildNewPayload(Child, UCX_SIZE_CONFIGURE)->Configure;

    if (!FailAlternate)
    {
        Configure->EndpointsToEnableCount = Child->m_EnableCount;
        Configure->EndpointsToEnable = Child->m_EndpointsToEnable;
    }

    Configure->EndpointsToDisableCount = Child->m_DisableCount;
    Configure->EndpointsToDisable = Child->m_EndpointsToDisable;
    Configure->EndpointsEnabledAndUnchangedCount = Child->m_UnchangedCount;
    Configure->EndpointsEnabledAndUnchanged = Child->m_EndpointsUnchanged;

    if (Child->m_CurrentConfig != NULL)
        Configure->ConfigurationValue = Child->m_CurrentConfig->Descriptor.bConfigurationValue;

    /* The next and new interfaces are always set together */
    if (Child->m_NextInterface != NULL && Child->m_NewInterface != NULL)
    {
        Configure->InterfaceNumber = Child->m_NewInterface->Descriptor->bInterfaceNumber;
        Configure->AlternateSetting = Child->m_NewInterface->Descriptor->bAlternateSetting;
    }

    HubChildSubmitUcxIoctl(Child, IOCTL_UCXHUB_ENDPOINTS_CONFIGURE);
}

VOID
DeviceMachine::DisableAllEndpoints()
{
    HubChild* Child = m_Device;
    HubConfiguration* Config = Child->m_CurrentConfig;
    PLIST_ENTRY Entry;

    if (Config == NULL || Child->m_EndpointArrayCapacity == 0)
    {
        DPRINT("Device %p has no endpoints to disable\n", Child);
        Child->Post(DsmEvent::ControllerIoctlDone);
        return;
    }

    Child->m_EnableCount = 0;
    Child->m_DisableCount = 0;
    Child->m_UnchangedCount = 0;

    for (Entry = Config->Interfaces.Flink; Entry != &Config->Interfaces; Entry = Entry->Flink)
        HubChildCollectForDisable(Child, CONTAINING_RECORD(Entry, HubInterface, Link));

    if (Child->m_DisableCount == 0)
    {
        Child->Post(DsmEvent::ControllerIoctlDone);
        return;
    }

    ProgramEndpoints();
}

VOID
DeviceMachine::DisableNewInterfaceEndpoints()
{
    HubChild* Child = m_Device;

    if (Child->m_NewInterface == NULL)
    {
        Child->Post(DsmEvent::ControllerIoctlDone);
        return;
    }

    Child->m_EnableCount = 0;
    Child->m_DisableCount = 0;
    Child->m_UnchangedCount = 0;

    HubChildCollectForDisable(Child, Child->m_NewInterface);

    if (Child->m_DisableCount == 0)
    {
        Child->Post(DsmEvent::ControllerIoctlDone);
        return;
    }

    ProgramEndpoints();
}

VOID
DeviceMachine::DisablePendingEndpoints()
{
    HubChild* Child = m_Device;

    Child->m_UnchangedCount = 0;

    if (Child->m_DisableCount == 0)
    {
        Child->Post(DsmEvent::ControllerIoctlDone);
        return;
    }

    Child->m_EnableCount = 0;
    ProgramEndpoints();
}

/* The caller guarantees a current configuration */
VOID
DeviceMachine::MarkPendingEndpointsDisabled()
{
    HubConfigMovePipes(m_Device->m_CurrentConfig, PipeState::PendingEnable, PipeState::Disabled);
}

BOOLEAN
DeviceMachine::EndpointsNeedProgramming()
{
    return m_Device->m_DisableCount != 0 || m_Device->m_EnableCount != 0;
}

BOOLEAN
DeviceMachine::EndpointsNeedDisableOnFailure()
{
    HubChild* Child = m_Device;

    if (Child->m_EnableCount != 0 && Child->m_CurrentConfig != NULL)
    {
        HubConfigMovePipes(Child->m_CurrentConfig, PipeState::PendingEnable, PipeState::Disabled);
        HubChildKeepUnchanged(Child, Child->m_EndpointsToEnable, Child->m_EnableCount);
        Child->m_EnableCount = 0;
    }

    return Child->m_DisableCount != 0;
}

/*
 * Creates every endpoint the controller does not know yet. Zero bandwidth
 * endpoints made before a failure stay created as Disabled.
 */
BOOLEAN
DeviceMachine::CreateEndpoints()
{
    HubChild* Child = m_Device;
    HubFdo* Hub = Child->m_Hub;
    HubConfiguration* Config = Child->m_CurrentConfig;
    HubInterface* Interface;
    HubPipe* Pipe;
    PLIST_ENTRY Entry;
    ULONG Index;
    NTSTATUS Status = STATUS_SUCCESS;

    if (Child->m_EndpointArrayCapacity == 0 || Config == NULL)
    {
        DPRINT("Device %p has no endpoints to create\n", Child);
        return TRUE;
    }

    for (Entry = Config->Interfaces.Flink;
         Entry != &Config->Interfaces && NT_SUCCESS(Status);
         Entry = Entry->Flink)
    {
        Interface = CONTAINING_RECORD(Entry, HubInterface, Link);

        for (Index = 0; Index < Interface->PipeCount; Index++)
        {
            Pipe = &Interface->Pipes[Index];
            if (Pipe->State != PipeState::NotCreated)
                continue;

            Pipe->State = PipeState::PendingCreate;
            Status = Hub->m_Stack.EndpointCreate(Hub->UsbDevice(),
                                                 Child->m_UsbDevice,
                                                 Pipe->Descriptor,
                                                 Pipe->BytesToEnd,
                                                 Pipe->Companion,
                                                 &Pipe->Endpoint);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Device %p endpoint 0x%x create failed 0x%lx\n",
                        Child,
                        Pipe->Descriptor->bEndpointAddress,
                        Status);
                Pipe->State = PipeState::NotCreated;
                break;
            }

            if (Pipe->ZeroBandwidth)
            {
                Pipe->State = PipeState::Disabled;
                Child->m_EndpointsUnchanged[Child->m_UnchangedCount++] = Pipe->Endpoint;
            }
            else
            {
                Child->m_EndpointsToEnable[Child->m_EnableCount++] = Pipe->Endpoint;
            }
        }
    }

    for (Entry = Config->Interfaces.Flink; Entry != &Config->Interfaces; Entry = Entry->Flink)
    {
        Interface = CONTAINING_RECORD(Entry, HubInterface, Link);

        for (Index = 0; Index < Interface->PipeCount; Index++)
        {
            Pipe = &Interface->Pipes[Index];
            if (Pipe->State != PipeState::PendingCreate)
                continue;

            if (NT_SUCCESS(Status))
            {
                Pipe->State = PipeState::PendingEnable;
            }
            else
            {
                Pipe->State = PipeState::NotCreated;
                Hub->m_Stack.EndpointDelete(Hub->UsbDevice(), Child->m_UsbDevice, Pipe->Endpoint);
                Pipe->Endpoint = NULL;
            }
        }
    }

    if (!NT_SUCCESS(Status))
    {
        Child->m_EnableCount = 0;
        Child->m_UnchangedCount = 0;
        HubChildFailStatus(Child, STATUS_INSUFFICIENT_RESOURCES, USBD_STATUS_INSUFFICIENT_RESOURCES);
        return FALSE;
    }

    return TRUE;
}

/* Deletion, all at passive level */

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubChildDeleteInterface(
    _In_ HubChild* Child,
    _In_ HubInterface* Interface)
{
    HubFdo* Hub = Child->m_Hub;
    HubPipe* Pipe;
    ULONG Index;

    for (Index = 0; Index < Interface->PipeCount; Index++)
    {
        Pipe = &Interface->Pipes[Index];
        if (Pipe->State != PipeState::Disabled)
            continue;

        Hub->m_Stack.EndpointDelete(Hub->UsbDevice(), Child->m_UsbDevice, Pipe->Endpoint);
        Pipe->State = PipeState::Deleted;
    }

    WdfObjectDelete(Interface->Memory);
}

/* The FDO reads the configuration under the lock, so it is taken off before it goes */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubChildDeleteConfiguration(
    _In_ HubChild* Child,
    _Inout_ HubConfiguration** Slot)
{
    HubConfiguration* Config;
    PLIST_ENTRY Entry;
    KIRQL Irql;

    KeAcquireSpinLock(&Child->m_ConfigLock, &Irql);
    Config = *Slot;
    *Slot = NULL;
    KeReleaseSpinLock(&Child->m_ConfigLock, Irql);

    if (Config == NULL)
        return;

    while (!IsListEmpty(&Config->Interfaces))
    {
        Entry = RemoveHeadList(&Config->Interfaces);
        HubChildDeleteInterface(Child, CONTAINING_RECORD(Entry, HubInterface, Link));
    }

    WdfObjectDelete(Config->Memory);
}

VOID
DeviceMachine::DeleteConfigEndpoints()
{
    HubChildDeleteConfiguration(m_Device, &m_Device->m_CurrentConfig);
}

VOID
DeviceMachine::DeleteOldConfigEndpoints()
{
    HubChildDeleteConfiguration(m_Device, &m_Device->m_OldConfig);
}

VOID
DeviceMachine::DeleteOldInterfaceEndpoints()
{
    HubInterface* Interface = m_Device->m_OldInterface;

    if (Interface == NULL)
        return;

    m_Device->m_OldInterface = NULL;
    HubChildDeleteInterface(m_Device, Interface);
}

/* Enabled endpoints are not deleted; the machine disabled them before */
VOID
DeviceMachine::DeleteNewInterfaceEndpoints()
{
    HubChild* Child = m_Device;
    HubInterface* Interface = Child->m_NewInterface;
    KIRQL Irql;

    if (Interface == NULL)
        return;

    KeAcquireSpinLock(&Child->m_ConfigLock, &Irql);
    RemoveEntryList(&Interface->Link);
    if (Child->m_CurrentConfig != NULL)
        Child->m_CurrentConfig->EndpointCount -= Interface->PipeCount;
    Child->m_NewInterface = NULL;
    KeReleaseSpinLock(&Child->m_ConfigLock, Irql);

    if (Child->m_NextInterface == Interface)
        Child->m_NextInterface = NULL;

    HubChildDeleteInterface(Child, Interface);
}

/* Client request forwarding */

static
VOID
NTAPI
HubChildClientRequestComplete(
    _In_ WDFREQUEST Request,
    _In_ WDFIOTARGET Target,
    _In_ PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_ WDFCONTEXT Context)
{
    HubChild* Child = (HubChild*)Context;

    UNREFERENCED_PARAMETER(Target);

    WdfRequestCompleteWithInformation(Request, Params->IoStatus.Status, Params->IoStatus.Information);
    Child->Post(DsmEvent::ControllerRequestDone);
}

/* The request goes down as the client sent it, straight to the controller */
VOID
NTAPI
HubChildForwardToController(
    _In_ HubChild* Child)
{
    WDFREQUEST Request = Child->m_ClientRequest;

    WdfRequestFormatRequestUsingCurrentType(Request);
    WdfRequestSetCompletionRoutine(Request, HubChildClientRequestComplete, Child);

    if (!WdfRequestSend(Request, Child->m_Hub->m_RootHubTarget, WDF_NO_SEND_OPTIONS))
    {
        DPRINT1("Device %p client request not sent 0x%lx\n", Child, WdfRequestGetStatus(Request));
        WdfRequestComplete(Request, WdfRequestGetStatus(Request));
        Child->Post(DsmEvent::ControllerRequestDone);
    }
}

VOID
DeviceMachine::ForwardToController()
{
    HubChildForwardToController(m_Device);
}

/* Services for the child PDO */

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubChildSetPdo(
    _In_ HubChild* Child,
    _In_ PDEVICE_OBJECT Pdo)
{
    HubFdo* Hub = Child->m_Hub;

    Hub->m_Stack.DeviceSetPdo(Hub->UsbDevice(), Child->m_UsbDevice, Pdo);
}

/* Level 1 bus information, asked once more with the size the bus wants */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubQueryBusInformation(
    _In_ HubFdo* Hub,
    _In_ PVOID BusContext,
    _Outptr_result_maybenull_ PUSB_BUS_INFORMATION_LEVEL_1* Result)
{
    PUSB_BUS_INFORMATION_LEVEL_1 Buffer;
    ULONG Length = sizeof(*Buffer);
    ULONG Actual = 0;
    NTSTATUS Status;

    *Result = NULL;

    Buffer = (PUSB_BUS_INFORMATION_LEVEL_1)ExAllocatePoolWithTag(PagedPool, Length, HUB_TAG_HUB);
    if (Buffer == NULL)
    {
        DPRINT1("Hub %p no memory for bus information\n", Hub);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = Hub->m_Usbdi.QueryBusInformation(BusContext, 1, Buffer, &Length, &Actual);
    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        ExFreePoolWithTag(Buffer, HUB_TAG_HUB);

        Length = Actual;
        Buffer = (PUSB_BUS_INFORMATION_LEVEL_1)ExAllocatePoolWithTag(PagedPool, Length, HUB_TAG_HUB);
        if (Buffer == NULL)
        {
            DPRINT1("Hub %p no memory for %lu bytes of bus information\n", Hub, Length);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Status = Hub->m_Usbdi.QueryBusInformation(BusContext, 1, Buffer, &Length, &Actual);
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p bus information query failed 0x%lx\n", Hub, Status);
        ExFreePoolWithTag(Buffer, HUB_TAG_HUB);
        return Status;
    }

    *Result = Buffer;
    return Status;
}

/* A short copy still succeeds; ActualLength says how much there is */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubQueryControllerName(
    _In_ HubFdo* Hub,
    _Out_writes_bytes_(Length) PUSB_HUB_NAME Name,
    _In_ ULONG Length)
{
    PUSB_BUS_INFORMATION_LEVEL_1 Info;
    ULONG Copy;
    NTSTATUS Status;

    Status = HubQueryBusInformation(Hub, Hub->m_Usbdi.BusContext, &Info);
    if (!NT_SUCCESS(Status))
        return Status;

    if (Length < FIELD_OFFSET(USB_HUB_NAME, HubName))
    {
        DPRINT1("Hub %p controller name buffer of %lu bytes is too small\n", Hub, Length);
        Status = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        Name->ActualLength = Info->ControllerNameLength;
        Copy = min(Info->ControllerNameLength, Length - FIELD_OFFSET(USB_HUB_NAME, HubName));
        RtlCopyMemory(Name->HubName, Info->ControllerNameUnicodeString, Copy);
    }

    ExFreePoolWithTag(Info, HUB_TAG_HUB);
    return Status;
}

/* The device handle stands in for the bus context, which UCX accepts */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubQueryDeviceBusInfo(
    _In_ HubChild* Child,
    _Out_ PUSB_BUS_NOTIFICATION Notification)
{
    PUSB_BUS_INFORMATION_LEVEL_1 Info;
    NTSTATUS Status;

    Status = HubQueryBusInformation(Child->m_Hub, Child->m_UsbDevice, &Info);
    if (!NT_SUCCESS(Status))
        return Status;

    Notification->TotalBandwidth = Info->TotalBandwidth;
    Notification->ConsumedBandwidth = Info->ConsumedBandwidth;
    Notification->ControllerNameLength = Info->ControllerNameLength;

    ExFreePoolWithTag(Info, HUB_TAG_HUB);
    return Status;
}
