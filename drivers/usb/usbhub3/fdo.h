/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Hub FDO, its state and the event mux to ports and devices
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#pragma once

/* Per hub flags; bit positions are fixed */
enum class HubFlag : ULONG
{
    TtHub                          = 0x00000001,
    MultiTtHub                     = 0x00000002,
    PerPortPower                   = 0x00000004,
    PerPortOverCurrent             = 0x00000008,
    NoOverCurrentProtection        = 0x00000010,
    Initialized                    = 0x00000020,
    Configured                     = 0x00000040,
    ParentNoSelectiveSuspend       = 0x00000080,
    WakeOnConnect                  = 0x00000100,
    ArmedForWake                   = 0x00000200,
    WaitWakeQueued                 = 0x00000400,
    ResetTtOnCancel                = 0x00000800,
    NoClearTtOnCancel              = 0x00001000,
    PowerOnPortsOnStart            = 0x00002000,
    PowerReferencePending          = 0x00004000,
    DisableLpm                     = 0x00008000,
    DisableUsb20Lpm                = 0x00010000,
    PowerReferenceAllowed       = 0x00020000,
    InAcpiNamespace                = 0x00040000,
    DelayAfterResetComplete        = 0x00080000,
    ToleratesU0WhileDetached         = 0x00100000,
    IgnoreEnabledInSsInactive      = 0x00200000,
    InBootOrPagingPath             = 0x00400000,
    DisableSuperSpeed              = 0x00800000,
    DiscardEnableDuringReset           = 0x01000000,
    NoSelectiveSuspendIntegrated   = 0x02000000,
    S0IdleConfigured               = 0x04000000,
    GlobalPowerReference           = 0x08000000,
    DisallowU2AcceptOnly           = 0x10000000,
    OverCurrentDetected            = 0x20000000,
    DisableOnSoftRemove            = 0x40000000,
    PollingToComplianceOnStart     = 0x80000000
};

/* Which device fan out is collecting acknowledgements right now */
enum class HubDeviceFanOut : UCHAR
{
    None,
    PowerUp,
    PowerDown,
    StopWhileSuspended
};

/** Counters and lists that tie the hub machine to its port and device machines. */
struct HubMux
{
    KSPIN_LOCK Lock;

    volatile LONG OutstandingPortChanges;
    volatile LONG OutstandingPnpOperations;
    volatile LONG OutstandingResetOperations;
    LONG InterruptReferences;
    volatile LONG PortPowerReferences;
    LONG DevicePowerReferences;

    /* Fixed after CreateChildPorts, 2.0 ports first */
    LIST_ENTRY Ports;
    ULONG PortCount;

    /* Registered device machines, under Lock */
    LIST_ENTRY Devices;

    BOOLEAN InterruptReleasePending;
    BOOLEAN FailInterruptAcquire;
    BOOLEAN HubResetEnabled;
    BOOLEAN FailDeviceRegistration;
    HubDeviceFanOut DeviceFanOut;

    /* Read by the boot device code without the lock */
    volatile LONG HubResetInProgress;
};

/* What a hub's parent reports through IOCTL_UCXHUB_GET_HUB_INFO; the layout is shared by every usbhub3 */
struct HubParentInfo
{
    PDEVICE_OBJECT RootHubPdo;
    USB_DEVICE_DESCRIPTOR DeviceDescriptor;
    USHORT U1ExitLatency;
    USHORT U2ExitLatency;
    USHORT SlowestLinkU1ExitLatency;
    UCHAR SlowestLinkU1Depth;
    USHORT SlowestLinkU2ExitLatency;
    UCHAR SlowestLinkU2Depth;
    USHORT HostInitiatedU1ExitLatency;
    USHORT HostInitiatedU2ExitLatency;
    UCHAR TotalHubDepth;
    USHORT TotalTpPropagationDelay;
    ULONG Flags;
    PUSB_DEVICE_CAPABILITY_SUPERSPEEDPLUS_SPEED SublinkSpeedAttr;
    ULONG SublinkSpeedAttrCount;
};

/* HubParentInfo.Flags */
#define HUB_PARENT_DISABLE_LPM          0x00000001
#define HUB_PARENT_HIGH_SPEED_CAPABLE   0x00000002

#ifdef _WIN64
C_ASSERT(FIELD_OFFSET(HubParentInfo, U1ExitLatency) == 26);
C_ASSERT(FIELD_OFFSET(HubParentInfo, TotalTpPropagationDelay) == 44);
C_ASSERT(FIELD_OFFSET(HubParentInfo, Flags) == 48);
C_ASSERT(sizeof(HubParentInfo) == 72);
#else
C_ASSERT(FIELD_OFFSET(HubParentInfo, U1ExitLatency) == 22);
C_ASSERT(FIELD_OFFSET(HubParentInfo, TotalTpPropagationDelay) == 40);
C_ASSERT(FIELD_OFFSET(HubParentInfo, Flags) == 44);
C_ASSERT(sizeof(HubParentInfo) == 56);
#endif

/** Product identity of a USB4 hub's router, from its DROM. */
struct HubRouterIdentity
{
    USHORT VendorId;
    USHORT ProductId;
    USHORT BcdDevice;
    USHORT Revision;
    BOOLEAN Known;
};

class HubFdo
{
public:
    static
    EVT_WDF_DRIVER_DEVICE_ADD EvtDeviceAdd;

    static
    HubFdo*
    FromDevice(
        _In_ WDFDEVICE Device);

    static
    HubFdo*
    FromContext(
        _In_ UCXHUB_HUB_CONTEXT Context)
    {
        return (HubFdo*)Context;
    }

    /* Flags */

    BOOLEAN
    HasFlag(
        _In_ HubFlag Flag) const
    {
        return (m_Flags & (LONG)Flag) != 0;
    }

    VOID
    SetFlag(
        _In_ HubFlag Flag)
    {
        InterlockedOr(&m_Flags, (LONG)Flag);
    }

    VOID
    ClearFlag(
        _In_ HubFlag Flag)
    {
        InterlockedAnd(&m_Flags, ~(LONG)Flag);
    }

    BOOLEAN
    IsRootHub() const
    {
        return m_Parent.HubDepth == 0;
    }

    UCXUSBDEVICE
    UsbDevice() const
    {
        return m_Parent.Hub;
    }

    WDFDEVICE
    Device() const
    {
        return m_Device;
    }

    /* Hub machine glue */

    VOID
    Post(
        _In_ HubEvent Event)
    {
        m_Machine.SmPost(Event);
    }

    /** Posts a PnP or power event and waits until the machine answers it. */
    _IRQL_requires_(PASSIVE_LEVEL)
    NTSTATUS
    PostAndWait(
        _In_ HubEvent Event,
        _In_ PCSTR What);

    BOOLEAN
    NeedsForwardProgress() const
    {
        return m_NeedsForwardProgress;
    }

    /* UCX forward progress work items, through the stack interface */

    PUCXHUB_WORKITEM
    AllocateWorkItem();

    VOID
    EnqueueWorkItem(
        _In_ PUCXHUB_WORKITEM WorkItem,
        _In_ PUCXHUB_WORKITEM_ROUTINE Routine,
        _In_ PVOID Context,
        _In_ BOOLEAN ForwardProgress);

    _IRQL_requires_(PASSIVE_LEVEL)
    VOID
    FlushAndDeleteWorkItem(
        _Inout_ PUCXHUB_WORKITEM* WorkItem);

    /* Child lookups */

    HubPort*
    FindPort(
        _In_ ULONG PortNumber);

    /* Mux, see fanout.cpp */

    VOID
    InitializeMux();

    _IRQL_requires_(PASSIVE_LEVEL)
    BOOLEAN
    CreateChildPorts();

    VOID
    QueuePowerUpToPorts(
        _In_ PortEvent Event);

    VOID
    QueueStopToPorts();

    VOID
    QueueSuspendToPorts();

    VOID
    QueueSurpriseRemovalToPorts();

    VOID
    QueueResetToPorts();

    VOID
    QueueStatusChangeToPorts();

    VOID
    QueueFakeStatusChangeToPorts();

    VOID
    QueuePowerDownToDevices(
        _In_ DsmEvent Event,
        _In_ HubDeviceFanOut FanOut);

    VOID
    QueuePowerUpToDevices(
        _In_ DsmEvent Event);

    VOID
    AllowHubReset();

    BOOLEAN
    RequestHubReset();

    BOOLEAN
    PortInterruptRefsReleased();

    BOOLEAN
    TakeInterruptReference(
        _In_ HubPort* Port);

    VOID
    DropInterruptReference(
        _In_ HubPort* Port);

    VOID
    TakePortPowerReference(
        _In_ HubPort* Port);

    VOID
    DropPortPowerReference(
        _In_ HubPort* Port);

    VOID
    DropHubResetReference(
        _In_ HubPort* Port);

    VOID
    ResumeInterruptTransfer(
        _In_ HubPort* Port);

    BOOLEAN
    RegisterDevice(
        _In_ HubChild* Child);

    VOID
    UnregisterDevice(
        _In_ HubChild* Child);

    VOID
    ReferenceDevicePower(
        _In_ HubChild* Child);

    VOID
    DereferenceDevicePower(
        _In_ HubChild* Child);

    VOID
    ConfirmStopWhileSuspended(
        _In_ HubChild* Child);

    VOID
    MarkDevicesProgrammingLost();

    BOOLEAN
    IsHubResetInProgress() const
    {
        return m_Mux.HubResetInProgress != 0;
    }

    /* Power references on the FDO */

    VOID
    AcquirePowerReference();

    VOID
    ReleasePowerReference();

public:
    HubMachine m_Machine;

    WDFDEVICE m_Device;
    PDEVICE_OBJECT m_WdmPdo;
    ULONG m_HubNumber;
    volatile LONG m_Flags;

    /* Interfaces from the parent stack */
    UCXHUB_PARENT_INTERFACE m_Parent;
    UCXHUB_STACK_INTERFACE m_Stack;
    USB_BUS_INTERFACE_USBDI_V3 m_Usbdi;

    DEVICE_CAPABILITIES m_Capabilities;
    HubParentInfo m_ParentInfo;
    BOOLEAN m_StreamsSupported;

    /* Hub reset and status change pipe reset */
    ULONG m_ResetFlags;
    struct _URB_PIPE_REQUEST m_PipeUrb;

    /* Root hub: the FDO's own target; external hub: a target on the root hub PDO */
    WDFIOTARGET m_RootHubTarget;

    /* Parent bus identity of the controller, from QueryControllerBus */
    UCXHUB_CONTROLLER_INFO_V2 m_ControllerInfo;

    /* Root hub only: what ucx01000 reported about the root ports */
    UCXHUB_ROOTHUB_INFO m_RootHubInfo;
    PROOTHUB_20PORT_INFO* m_Root20Ports;
    PROOTHUB_30PORT_INFO_EX* m_Root30Ports;

    /* Ports and power */
    ULONG m_PortCount;
    USHORT m_First20Port;
    USHORT m_Last20Port;
    USHORT m_First30Port;
    USHORT m_Last30Port;
    ULONG m_MaxPortPower;
    ULONG m_SspIsochBurstCount;

    /* PnP handshake with the hub machine */
    KEVENT m_PnpEvent;
    NTSTATUS m_PnpStatus;

    /* The one port status request being served for a client or user mode */
    WDFREQUEST m_PortStatusRequest;
    HubPort* m_PortStatusTarget;

    /* Selective suspend */
    ULONG m_IdleTimeout;
    PVOID m_IdleTimeoutSetting;
    PVOID m_SelectiveSuspendSetting;
    WDF_POWER_DEVICE_STATE m_PowerState;

    /* Nonzero selects the PnP problem text reported on failure */
    ULONG m_FailureMessageId;

    LIST_ENTRY m_HubListEntry;
    BOOLEAN m_OnHubList;

    HubMux m_Mux;

    /* Hub machine resources */
    HubTimer m_Timer;
    PUCXHUB_WORKITEM m_WorkItem;
    BOOLEAN m_NeedsForwardProgress;
    HubControlRequest m_Control;
    HubInterruptRequest m_Interrupt;

    /* Hub level transfer data, see hubreq.cpp */
    union
    {
        USB_HUB_DESCRIPTOR Usb20;
        USB_30_HUB_DESCRIPTOR Usb30;
    } m_HubDescriptor;
    USB_DEVICE_STATUS m_DeviceStatus;
    USB_HUB_STATUS_AND_CHANGE m_HubStatus;
    USB_HUB_STATUS_AND_CHANGE m_PreviousHubStatus;
    PUSB_CONFIGURATION_DESCRIPTOR m_ConfigDescriptor;
    USHORT m_ConfigTotalLength;
    USHORT m_SelectedFeature;
    HubChange m_SelectedChange;
    USHORT m_InterruptMaxPacket;
    USBD_CONFIGURATION_HANDLE m_ConfigurationHandle;

    /* Retry counters; the reset and pipe reset counts share one field on purpose */
    USHORT m_ResetCount;
    USHORT m_DescriptorRetryCount;
    ULONG m_RecoveryCount;
    ULONG m_LifetimeRecoveryCount;
    LARGE_INTEGER m_FirstRecoveryTime;
    ULONG m_LastResetPortStatus;

    /* Hub symbolic link, copied at prepare hardware */
    UNICODE_STRING m_SymbolicLinkName;

    ULONG m_VerifierFlags;

    /* wmi */

    /* GUID_USB_WMI_STD_DATA instance; user notifications are fired on it */
    WDFWMIINSTANCE m_WmiInstance;

    /* USB4 */

    /* HubFWUpdateProtocol from the hardware key; 1 asks for the router DROM read */
    ULONG m_FwUpdateProtocol;

    /* Router DROM read: the header area, the pool buffer and the mailbox register value */
    ULONG m_DromHeader[4];
    PULONG m_DromBuffer;
    PULONG m_DromPool;
    ULONG m_DromLength;
    ULONG m_DromMailbox;
    HubRouterIdentity m_RouterIdentity;

    /* Usb4HostName override from the hardware key, or NULL */
    WDFSTRING m_Usb4HostName;

    /* At least one port is bound to a USB4 host router; never cleared */
    BOOLEAN m_HasUsb4Ports;

    /* Errata: tunneled devices below this hub never get U2 */
    BOOLEAN m_TunneledDevicesSkipU2;

    /* HubUsb4Host records in WDFMEMORY objects, under m_Usb4HostsLock */
    WDFCOLLECTION m_Usb4Hosts;
    WDFWAITLOCK m_Usb4HostsLock;

    /* Host router power PDO interface notification entry */
    PVOID m_Usb4Notification;

private:
    _IRQL_requires_(PASSIVE_LEVEL)
    NTSTATUS
    Initialize(
        _In_ WDFDEVICE Device);

    _IRQL_requires_(PASSIVE_LEVEL)
    NTSTATUS
    QueryCapabilities();

    _IRQL_requires_(PASSIVE_LEVEL)
    NTSTATUS
    QueryInterfaces();

    _IRQL_requires_(PASSIVE_LEVEL)
    VOID
    ProbeParentCapabilities();

    _IRQL_requires_(PASSIVE_LEVEL)
    VOID
    ConfigureIdle();

    _IRQL_requires_(PASSIVE_LEVEL)
    NTSTATUS
    CreateQueue();

    _IRQL_requires_(PASSIVE_LEVEL)
    NTSTATUS
    InitializeMachine();

    _IRQL_requires_(PASSIVE_LEVEL)
    NTSTATUS
    AssignIdleSettings(
        _In_ WDF_POWER_POLICY_IDLE_TIMEOUT_TYPE TimeoutType);

    VOID
    PostToAllPorts(
        _In_ PortEvent Event);

    VOID
    FinishDeviceFanOut();

    static
    VOID
    NTAPI
    TimerFired(
        _In_ PVOID Context);

    static EVT_WDF_DEVICE_PREPARE_HARDWARE EvtPrepareHardware;
    static EVT_WDF_DEVICE_RELEASE_HARDWARE EvtReleaseHardware;
    static EVT_WDF_DEVICE_D0_ENTRY EvtD0Entry;
    static EVT_WDF_DEVICE_D0_EXIT EvtD0Exit;
    static EVT_WDF_DEVICE_SURPRISE_REMOVAL EvtSurpriseRemoval;
    static EVT_WDF_DEVICE_USAGE_NOTIFICATION EvtUsageNotification;
    static EVT_WDF_DEVICE_ARM_WAKE_FROM_S0 EvtArmWakeFromS0;
    static EVT_WDF_DEVICE_DISARM_WAKE_FROM_S0 EvtDisarmWakeFromS0;
    static EVT_WDF_DEVICE_ARM_WAKE_FROM_SX EvtArmWakeFromSx;
    static EVT_WDF_DEVICE_DISARM_WAKE_FROM_SX EvtDisarmWakeFromSx;
    static EVT_WDF_OBJECT_CONTEXT_CLEANUP EvtCleanup;
    static EVT_WDF_OBJECT_CONTEXT_DESTROY EvtDestroy;
    static EVT_WDFDEVICE_WDM_IRP_PREPROCESS EvtShutdown;
    static POWER_SETTING_CALLBACK PowerSettingChanged;
    static UCXHUB_WORKITEM_ROUTINE MachineWorkItem;

    friend class HubMachine;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(HubFdo, HubGetFdoContext);

/* Default queue handlers, hubioctl.cpp */
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL HubEvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL HubEvtIoInternalDeviceControl;

/* Hub queue context: one control transfer block for user descriptor requests */
struct HubQueueContext
{
    struct _URB_CONTROL_TRANSFER_EX Urb;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(HubQueueContext, HubGetQueueContext);
