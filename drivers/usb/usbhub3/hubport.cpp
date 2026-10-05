/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Port object lifetime, its timers and work item
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"

#define NDEBUG
#include <debug.h>

/* A port holds this on its hub so the hub outlives a port a device still uses */
static const char HubTagPort[] = "hub port";

/* Port timer durations in ms */
#define PORT_DEBOUNCE_TIME              100
#define PORT_OVERCURRENT_TIME           500
#define PORT_RESET_TIME                 3000
#define HUB_PORT_RESUME_WAIT_MS                500
#define PORT_RESUME_RECOVERY_TIME       10
#define PORT_POWER_ON_TIME_ROOT         100
#define PORT_POWER_ON_TIME              300
#define PORT_RECONNECT_TIME_BOOT        2000
#define PORT_RECONNECT_TIME             1000
#define PORT_SUPERSPEED_DISABLE_TIME    5000
#define PORT_RESET_POLL_TIME            500

HubPort*
HubPort::FromObject(
    _In_ WDFOBJECT Object)
{
    return HubGetPortContext(Object);
}

_IRQL_requires_(PASSIVE_LEVEL)
HubPort*
HubPort::Create(
    _In_ HubFdo* Hub,
    _In_ const HubPortInfo* Info)
{
    WDF_OBJECT_ATTRIBUTES Attributes;
    WDF_TIMER_CONFIG TimerConfig;
    HubPort* Port;
    WDFOBJECT Object;
    NTSTATUS Status;

    PAGED_CODE();

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, HubPort);
    Attributes.ParentObject = Hub->m_Device;
    Attributes.ExecutionLevel = WdfExecutionLevelPassive;
    Attributes.EvtCleanupCallback = EvtCleanup;
    Attributes.EvtDestroyCallback = EvtDestroy;

    Status = WdfObjectCreate(&Attributes, &Object);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p port %u object creation failed 0x%lx\n", Hub, Info->PortNumber, Status);
        return NULL;
    }

    WdfObjectReferenceWithTag(Hub->m_Device, (PVOID)HubTagPort);

    Port = new (HubGetPortContext(Object)) HubPort();
    Port->m_Object = Object;
    Port->m_Hub = Hub;
    Port->m_Info = *Info;
    Port->m_SelectedFeature = HUB_FEATURE_NONE;
    Port->m_D3ColdReconnectTimeout = PORT_RECONNECT_TIME;
    HubClearListEntry(&Port->m_HubLink);

    Status = Port->m_Control.Create(Object, WdfDeviceGetIoTarget(Hub->m_Device));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Port %u control request creation failed 0x%lx\n", Port->Number(), Status);
        goto Failed;
    }

    Port->m_Machine.Initialize(Port, Port->IsUsb30());
    Port->m_Timer.Initialize(TimerFired, Port);

    if (Hub->HasFlag(HubFlag::DelayAfterResetComplete))
    {
        WDF_TIMER_CONFIG_INIT(&TimerConfig, ResetPollFired);
        WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
        Attributes.ParentObject = Object;

        Status = WdfTimerCreate(&TimerConfig, &Attributes, &Port->m_ResetPollTimer);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Port %u reset poll timer creation failed 0x%lx\n", Port->Number(), Status);
            goto Failed;
        }
    }

    Port->m_WorkItem = Hub->AllocateWorkItem();
    if (Port->m_WorkItem == NULL)
    {
        DPRINT1("Port %u work item allocation failed\n", Port->Number());
        goto Failed;
    }

    DPRINT("Hub %p created port %p number %u\n", Hub, Port, Port->Number());
    return Port;

Failed:
    WdfObjectDelete(Object);
    return NULL;
}

/* A device still attached when the hub goes away is detached here */
VOID
NTAPI
HubPort::EvtCleanup(
    _In_ WDFOBJECT Object)
{
    HubPort* Port = HubGetPortContext(Object);

    if (Port->m_WorkItem == NULL)
        return;

    Port->Post(PortEvent::PortCleanup);
    Port->m_Hub->FlushAndDeleteWorkItem(&Port->m_WorkItem);
}

/* A running timer callback is waited for */
VOID
NTAPI
HubPort::EvtDestroy(
    _In_ WDFOBJECT Object)
{
    HubPort* Port = HubGetPortContext(Object);

    if (Port->m_Hub == NULL)
        return;

    Port->m_Timer.Cancel();
    KeFlushQueuedDpcs();

    WdfObjectDereferenceWithTag(Port->m_Hub->m_Device, (PVOID)HubTagPort);
}

VOID
NTAPI
HubPort::TimerFired(
    _In_ PVOID Context)
{
    ((HubPort*)Context)->Post(PortEvent::TimerExpired);
}

VOID
NTAPI
HubPort::ResetPollFired(
    _In_ WDFTIMER Timer)
{
    HubGetPortContext(WdfTimerGetParentObject(Timer))->Post(PortEvent::ResetPollExpired);
}

VOID
NTAPI
HubPort::MachineWorkItem(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context,
    _In_ PUCXHUB_WORKITEM WorkItem)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(WorkItem);

    ((HubPort*)Context)->m_Machine.SmContinueOnPassive();
}

/* Engine hooks */

VOID
PortMachine::ReferencePort()
{
    WdfObjectReference(m_Port->m_Object);
}

VOID
PortMachine::DereferencePort()
{
    HubDereferenceDeferred(m_Port->m_Object);
}

VOID
PortMachine::QueuePassiveWork()
{
    m_Port->m_Hub->EnqueueWorkItem(m_Port->m_WorkItem,
                                   HubPort::MachineWorkItem,
                                   m_Port,
                                   m_Port->m_Hub->NeedsForwardProgress());
}

/* Keeps the hub out of idle while the port works; only while the hub is started */
BOOLEAN
PortMachine::HubPowerTake()
{
    HubFdo* Hub = m_Port->m_Hub;
    NTSTATUS Status;

    if (!Hub->HasFlag(HubFlag::PowerReferenceAllowed))
        return FALSE;

    Status = WdfDeviceStopIdle(Hub->m_Device, FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Port %u could not take a hub power reference 0x%lx\n", m_Port->Number(), Status);
        return FALSE;
    }

    m_Port->SetMuxFlag(PortMuxFlag::WdfPowerReference);
    return TRUE;
}

VOID
PortMachine::HubPowerDrop()
{
    if (!m_Port->HasMuxFlag(PortMuxFlag::WdfPowerReference))
        return;

    m_Port->ClearMuxFlag(PortMuxFlag::WdfPowerReference);
    WdfDeviceResumeIdle(m_Port->m_Hub->m_Device);
}

/* Port timers */

VOID
PortMachine::StartTimer(
    _In_ PSM_TIMER Timer)
{
    HubChild* Child = m_Port->m_Child;
    ULONG Time;

    switch (Timer)
    {
        case PsmTimerDebounce:
            Time = PORT_DEBOUNCE_TIME;
            break;

        case PsmTimerOverCurrent:
            Time = PORT_OVERCURRENT_TIME;
            DPRINT1("Port %u over current, waiting %lu ms\n", m_Port->Number(), Time);
            break;

        case PsmTimerResetCompletion:
            Time = PORT_RESET_TIME;
            break;

        case PsmTimerResumeCompletion:
            Time = HUB_PORT_RESUME_WAIT_MS;
            break;

        /* The device may ask for longer in its MS OS 2.0 descriptor set */
        case PsmTimerResumeRecovery:
            Time = (Child != NULL && Child->m_HasResumeRecoveryTime) ? Child->m_ResumeRecoveryTime
                                                                       : PORT_RESUME_RECOVERY_TIME;
            break;

        /* Longer on external hubs */
        case PsmTimerPowerOn:
            Time = m_Port->m_Hub->IsRootHub() ? PORT_POWER_ON_TIME_ROOT : PORT_POWER_ON_TIME;
            break;

        case PsmTimerReconnect:
            Time = m_Port->HasFlag(PortFlag::SupportsReattach) ? PORT_RECONNECT_TIME_BOOT
                                                               : PORT_RECONNECT_TIME;
            break;

        case PsmTimerD3ColdReconnect:
            Time = m_Port->m_D3ColdReconnectTimeout;
            DPRINT("Port %u waiting %lu ms for reconnect after D3 cold\n", m_Port->Number(), Time);
            break;

        case PsmTimerSuperSpeedDisable:
            Time = PORT_SUPERSPEED_DISABLE_TIME;
            break;

        /* A separate WDF timer, which the port stop never cancels */
        case PsmTimerResetPoll:
        {
            BOOLEAN Queued = WdfTimerStart(m_Port->m_ResetPollTimer, WDF_REL_TIMEOUT_IN_MS(PORT_RESET_POLL_TIME));

            if (Queued)
                DPRINT1("Port %u reset poll timer was already queued\n", m_Port->Number());
            NT_ASSERT(!Queued);
            return;
        }

        default:
            DPRINT1("Port %u asked for unknown timer %d\n", m_Port->Number(), (int)Timer);
            NT_ASSERT(FALSE);
            return;
    }

    m_Port->m_Timer.Start(Time);
}

BOOLEAN
PortMachine::StopTimer()
{
    return m_Port->m_Timer.Cancel();
}

/* Notices to the device machine */

VOID
PortMachine::TellDevice(
    _In_ PSM_DEVICE_NOTICE Notice)
{
    static const DsmEvent NoticeEvents[] =
    {
        DsmEvent::PortFault,
        DsmEvent::PortDisableDone,
        DsmEvent::PortResetAbortedForSuspend,
        DsmEvent::PortResumeDone,
        DsmEvent::PortSuspendDone,
        DsmEvent::PortResumeAbortedForSuspend,
        DsmEvent::PortResumeTimeout,
        DsmEvent::PortNowDisabled,
        DsmEvent::PortNowEnabled,
        DsmEvent::PortNowSuspended,
        DsmEvent::PortResetTimeout,
        DsmEvent::PortResetDone,
        DsmEvent::PortTimeoutSet,
        DsmEvent::PortNowEnabledOnReconnect
    };

    NT_ASSERT((ULONG)Notice < RTL_NUMBER_OF(NoticeEvents));

    if (m_Port->m_Child != NULL)
        m_Port->m_Child->Post(NoticeEvents[Notice]);
}

/* Attach, detach and re-attach */

BOOLEAN
PortMachine::CreateDevice()
{
    return HubChild::Create(m_Port) != NULL;
}

/* The device machine is idle and handles PortAttached inside this post */
BOOLEAN
PortMachine::ConnectDevice()
{
    HubChild* Child = m_Port->m_Child;

    Child->ClearState(ChildState::AttachSucceeded);
    m_Port->ClearFlag(PortFlag::SupportsReattach);

    Child->Post(DsmEvent::PortAttached);

    if (!Child->HasState(ChildState::AttachSucceeded))
    {
        DPRINT1("Port %u device %p attach failed\n", m_Port->Number(), Child);
        m_Port->m_ConnectionStatus = DeviceGeneralFailure;
        return FALSE;
    }

    m_Port->SetFlag(PortFlag::DeviceConnected);
    m_Port->m_ConnectionStatus = DeviceEnumerating;
    return TRUE;
}

/* UCX keeps a boot device's transfers alive so it can come back */
VOID
PortMachine::DisconnectDevice()
{
    HubFdo* Hub = m_Port->m_Hub;

    m_Port->m_Child->Post(DsmEvent::PortDetached);

    if (!m_Port->HasFlag(PortFlag::SupportsReattach))
        Hub->m_Stack.DeviceDisconnect(Hub->UsbDevice(), m_Port->Number());
}

VOID
PortMachine::ReattachBootDevice()
{
    m_Port->m_Child->Post(DsmEvent::PortReattached);
    m_Port->SetFlag(PortFlag::DeviceConnected);
    m_Port->m_ConnectionStatus = DeviceEnumerating;
}

/* Runs when a DeviceGone is taken off the queue */
VOID
PortMachine::ForgetDetachedDevice()
{
    HubChild* Child = m_Port->m_Child;

    if (Child != NULL)
    {
        if (Child->HasState(ChildState::DifferentDeviceOnBootPort))
            DPRINT("Port %u wrong device on the boot device port detached\n", m_Port->Number());

        Child->ClearState(ChildState::DifferentDeviceOnBootPort);

        if (!m_Port->HasFlag(PortFlag::SupportsReattach))
        {
            DPRINT("Port %u deleting detached device %p\n", m_Port->Number(), Child);
            m_Port->m_Child = NULL;
            HubDeleteDeferred(Child->m_Object);
        }
    }

    m_Port->ClearFlag(PortFlag::DeviceConnected);

    if (m_Port->m_ConnectionStatus != DeviceCausedOvercurrent)
        m_Port->m_ConnectionStatus = NoDeviceConnected;
}

/* Tells the kernel a boot device went away, once per absence */
VOID
PortMachine::NotifyBootDeviceRemoval()
{
    HubChild* Child = m_Port->m_Child;

    if (!m_Port->HasFlag(PortFlag::SupportsReattach) ||
        m_Port->m_Hub->IsHubResetInProgress() ||
        (m_Port->m_PreviousStatus & PS_CONNECTED) ||
        Child == NULL ||
        InterlockedExchange(&Child->m_BootReportedMissing, 1) != 0)
    {
        return;
    }

    if (!Child->HasProperty(ChildProperty::IsHub))
        m_Port->SetFlag(PortFlag::PendingRecoveryUpdate);

    DPRINT("Port %u boot device %p reported missing\n", m_Port->Number(), Child);
    HubNotifyBootDeviceRemoval(Child->m_BootHandle);
}

/* Device kind bits; the device machine is not running yet */

VOID
PortMachine::MarkUsb2Device()
{
    m_Port->m_Child->m_Kind |= DSM_KIND_PORT20;
}

VOID
PortMachine::MarkUsb3Device()
{
    m_Port->m_Child->m_Kind |= DSM_KIND_SUPER_SPEED | DSM_KIND_PORT30;
}

VOID
PortMachine::ForceResetOnEnumeration()
{
    m_Port->m_Child->SetState(ChildState::WarmResetOnEnumeration);
}

/* Saved status updates and queries */

VOID
PortMachine::ResetChangeAccumulator()
{
    m_Port->m_ChangeAccumulator = 0;
}

VOID
PortMachine::RecordPortDisabled()
{
    m_Port->m_Current.StatusChange.PortStatus.AsUshort16 &= ~PS_ENABLED;
}

VOID
PortMachine::RecordPortSuspended()
{
    m_Port->m_Current.StatusChange.PortStatus.AsUshort16 |= PS_SUSPENDED;
}

BOOLEAN
PortMachine::DeviceConnected()
{
    return (m_Port->m_Current.StatusChange.PortStatus.AsUshort16 & PS_CONNECTED) != 0;
}

/* SS.Inactive and Compliance Mode mean something is attached even when not connected */
BOOLEAN
PortMachine::DevicePresent()
{
    USHORT Status = m_Port->m_Current.StatusChange.PortStatus.AsUshort16;
    USHORT Link = HubLinkState(Status);

    return (Status & PS_CONNECTED) || Link == LINK_COMPLIANCE || Link == LINK_SS_INACTIVE;
}

BOOLEAN
PortMachine::OverCurrentActive()
{
    if (!(m_Port->m_Current.StatusChange.PortStatus.AsUshort16 & PS_OVER_CURRENT))
        return FALSE;

    m_Port->m_ConnectionStatus = DeviceCausedOvercurrent;
    HubNoteHubOverCurrent(m_Port->m_Hub);
    return TRUE;
}

VOID
PortMachine::MarkOverCurrentCause()
{
    m_Port->m_ConnectionStatus = DeviceCausedOvercurrent;
}

BOOLEAN
PortMachine::LinkInU0()
{
    return HubLinkState(m_Port->m_Current.StatusChange.PortStatus.AsUshort16) <= 2;
}

BOOLEAN
PortMachine::PortPowered()
{
    return (m_Port->m_Current.StatusChange.PortStatus.AsUshort16 & PS_POWER_30) != 0;
}

/* Selects the change for the next ack */
BOOLEAN
PortMachine::ConnectChangedOnResume()
{
    USHORT* Change = &m_Port->m_Current.StatusChange.PortChange.AsUshort16;

    if (*Change & PC_CONNECT)
    {
        *Change &= ~PC_CONNECT;
        m_Port->m_SelectedFeature = PORT_C_CONNECTION;
        return TRUE;
    }

    if (*Change & PC_SUSPEND)
    {
        *Change &= ~PC_SUSPEND;
        m_Port->m_SelectedFeature = PORT_C_SUSPEND;
        return TRUE;
    }

    return FALSE;
}

BOOLEAN
PortMachine::PollResetCompletion()
{
    return (m_Port->m_ChangeAccumulator & PC_RESET) &&
           m_Port->m_Hub->HasFlag(HubFlag::DelayAfterResetComplete);
}

BOOLEAN
PortMachine::OldDevicePresent()
{
    return m_Port->HasFlag(PortFlag::DeviceConnected);
}

BOOLEAN
PortMachine::BootDeviceReturning()
{
    return m_Port->HasFlag(PortFlag::SupportsReattach);
}

/* While a SuperSpeed device's last SET_ADDRESS failed, connects wait longer */
BOOLEAN
PortMachine::NeedsDebounce()
{
    return !HubDriver.HasFlag(HubGlobal::PreventSuperSpeedDebounce) &&
           HubDriver.SuperSpeedDebounceVotes > 0;
}

BOOLEAN
PortMachine::D3ColdEnabled()
{
    HubChild* Child = m_Port->m_Child;
    BOOLEAN Enabled;

    Enabled = Child->HasProperty(ChildProperty::AcpiAllowsD3Cold) &&
              Child->HasState(ChildState::D3ColdEnabledByDriver) &&
              !m_Port->HasProperty(PortProperty::Removable) &&
              Child->m_PowerState == PowerDeviceD3;

    if (Enabled)
        DPRINT("Port %u device %p is in D3 cold\n", m_Port->Number(), Child);

    return Enabled;
}

VOID
PortMachine::ReadPortBits(
    _Out_ PPSM_PORT_BITS Bits)
{
    USHORT Status = m_Port->m_Current.StatusChange.PortStatus.AsUshort16;
    USHORT Change = m_Port->m_Current.StatusChange.PortChange.AsUshort16;

    Bits->Connected = (Status & PS_CONNECTED) != 0;
    Bits->Enabled = (Status & PS_ENABLED) != 0;
    Bits->OverCurrent = (Status & PS_OVER_CURRENT) != 0;
    Bits->OverCurrentChanged = (Change & PC_OVER_CURRENT) != 0;
}

/* Over current reset requested by user mode, see the WMI module */

VOID
PortMachine::NotifyUserOfOverCurrent()
{
    m_Port->SetFlag(PortFlag::OverCurrentResetArmed);
    DPRINT1("Hub %p port %u over current persists, notifying user\n", m_Port->m_Hub, m_Port->Number());
    HubWmiNotifyOverCurrent(m_Port->m_Hub, m_Port->Number());
}

VOID
PortMachine::CancelUserOverCurrentReset()
{
    m_Port->ClearFlag(PortFlag::OverCurrentResetArmed);
}

/* The enumeration failure text the device shows for a broken link */
VOID
PortMachine::LogLinkStateError()
{
    USHORT Link = HubLinkState(m_Port->m_PreviousStatus);

    if (Link == LINK_SS_INACTIVE)
        m_Port->m_Child->m_EnumMessageId = HUB_ENUM_LINK_SS_INACTIVE;
    else if (Link == LINK_COMPLIANCE)
        m_Port->m_Child->m_EnumMessageId = HUB_ENUM_LINK_COMPLIANCE;
}

/* Device machine requests to its port */

VOID
DeviceMachine::NotifyPortDetached()
{
    m_Device->m_Port->Post(PortEvent::DeviceGone);
}

VOID
DeviceMachine::RequestPortDisable()
{
    m_Device->m_Port->Post(PortEvent::DisableRequest);
}

VOID
DeviceMachine::RequestPortSuspend()
{
    m_Device->m_Port->Post(PortEvent::SuspendRequest);
}

VOID
DeviceMachine::RequestPortResume()
{
    m_Device->m_Port->Post(PortEvent::ResumeRequest);
}

VOID
DeviceMachine::AskPortCycle()
{
    m_Device->m_Port->Post(PortEvent::CycleRequest);
}

VOID
DeviceMachine::RequestPortReset()
{
    m_Device->m_Port->Post(PortEvent::ResetRequest);
}

VOID
DeviceMachine::RequestPortWarmReset()
{
    m_Device->m_Port->Post(PortEvent::WarmResetRequest);
}

VOID
DeviceMachine::RequestSuperSpeedDisable()
{
    m_Device->m_Port->Post(PortEvent::DisableSuperSpeedRequest);
}

VOID
DeviceMachine::RequestU1Timeout()
{
    m_Device->m_Port->Post(PortEvent::SetU1Timeout);
}

VOID
DeviceMachine::RequestU2Timeout()
{
    m_Device->m_Port->Post(PortEvent::SetU2Timeout);
}

VOID
DeviceMachine::RequestHibernationPrep()
{
    m_Device->m_Port->Post(PortEvent::HibernateRequest);
}
