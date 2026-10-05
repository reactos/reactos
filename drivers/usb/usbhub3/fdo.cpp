/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Hub FDO creation, PnP and power callbacks, hub machine glue
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"

#define NDEBUG
#include <debug.h>

/* Hub timer durations in ms */
#define HUB_OVERCURRENT_TIME            500
#define HUB_RESET_RETRY_TIME            500
#define HUB_DESCRIPTOR_RETRY_TIME       100

#define HUB_DEFAULT_IDLE_TIMEOUT        50
#define HUB_ROOT_MAX_PORT_POWER         500

/* Hub resets in a row, and recoveries per window, before giving up */
#define HUB_RESET_LIMIT                 3
#define HUB_RECOVERY_LIMIT              10
#define HUB_RECOVERY_WINDOW             (60LL * 1000 * 1000 * 10)

/* PnP problem text "hub reset failed" */
#define HUB_MSG_HUB_RESET_FAILED        0x40020003

/* Container of devices built into the computer */
static const UNICODE_STRING HubSystemContainerId =
    RTL_CONSTANT_STRING(L"{00000000-0000-0000-FFFF-FFFFFFFFFFFF}");

/* Description shown for a hub whose SuperSpeed side was turned off by errata */
static const WCHAR HubNonFunctionalDescription[] = L"SuperSpeed USB Hub (Non Functional)";

HubFdo*
HubFdo::FromDevice(
    _In_ WDFDEVICE Device)
{
    return HubGetFdoContext(Device);
}

/* Work items */

PUCXHUB_WORKITEM
HubFdo::AllocateWorkItem()
{
    return m_Stack.WorkItemAllocate(UsbDevice(),
                                    WdfDeviceWdmGetDeviceObject(m_Device),
                                    UCXHUB_WORKITEM_FLAG_NEEDS_FLUSH);
}

VOID
HubFdo::EnqueueWorkItem(
    _In_ PUCXHUB_WORKITEM WorkItem,
    _In_ PUCXHUB_WORKITEM_ROUTINE Routine,
    _In_ PVOID Context,
    _In_ BOOLEAN ForwardProgress)
{
    m_Stack.WorkItemEnqueue(WorkItem,
                            Routine,
                            Context,
                            ForwardProgress ? UcxHubWorkItemDefault : UcxHubWorkItemForwardProgressNotRequired);
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
HubFdo::FlushAndDeleteWorkItem(
    _Inout_ PUCXHUB_WORKITEM* WorkItem)
{
    if (*WorkItem == NULL)
        return;

    m_Stack.WorkItemFlush(*WorkItem);
    m_Stack.WorkItemDelete(*WorkItem);
    *WorkItem = NULL;
}

/* The hub machine answers every PnP and power event by signaling m_PnpEvent */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
HubFdo::PostAndWait(
    _In_ HubEvent Event,
    _In_ PCSTR What)
{
    PAGED_CODE();

    DPRINT("Hub %p posting %s\n", m_Device, What);

    KeClearEvent(&m_PnpEvent);
    Post(Event);
    HubWaitForPnpEvent(&m_PnpEvent, What, m_Device);

    if (!NT_SUCCESS(m_PnpStatus))
        DPRINT1("Hub %p %s failed 0x%lx\n", m_Device, What, m_PnpStatus);

    return m_PnpStatus;
}

/*
 * Raised to DISPATCH_LEVEL so KMDF cannot call a blocking PDO power callback
 * on this thread while the hub machine holds it.
 */
VOID
HubFdo::AcquirePowerReference()
{
    KIRQL OldIrql;
    NTSTATUS Status;

    NT_ASSERT(!HasFlag(HubFlag::PowerReferencePending));

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    Status = WdfDeviceStopIdle(m_Device, FALSE);
    KeLowerIrql(OldIrql);

    if (NT_SUCCESS(Status))
        SetFlag(HubFlag::PowerReferencePending);
    else
        DPRINT1("Hub %p power reference failed 0x%lx\n", m_Device, Status);
}

VOID
HubFdo::ReleasePowerReference()
{
    KIRQL OldIrql;

    if (!HasFlag(HubFlag::PowerReferencePending))
        return;

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    WdfDeviceResumeIdle(m_Device);
    KeLowerIrql(OldIrql);

    ClearFlag(HubFlag::PowerReferencePending);
}

/* AddDevice pieces */

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
HubFdo::QueryCapabilities()
{
    WDF_REQUEST_REUSE_PARAMS Reuse;
    WDF_REQUEST_SEND_OPTIONS Options;
    IO_STACK_LOCATION Stack;
    WDFREQUEST Request;
    WDFIOTARGET Target = WdfDeviceGetIoTarget(m_Device);
    NTSTATUS Status;

    PAGED_CODE();

    Status = WdfRequestCreate(WDF_NO_OBJECT_ATTRIBUTES, Target, &Request);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p capabilities request create failed 0x%lx\n", m_Device, Status);
        return Status;
    }

    /* PnP IRPs must start out as not supported */
    WDF_REQUEST_REUSE_PARAMS_INIT(&Reuse, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);
    WdfRequestReuse(Request, &Reuse);

    RtlZeroMemory(&m_Capabilities, sizeof(m_Capabilities));
    m_Capabilities.Size = sizeof(m_Capabilities);
    m_Capabilities.Version = 1;
    m_Capabilities.Address = MAXULONG;
    m_Capabilities.UINumber = MAXULONG;

    RtlZeroMemory(&Stack, sizeof(Stack));
    Stack.MajorFunction = IRP_MJ_PNP;
    Stack.MinorFunction = IRP_MN_QUERY_CAPABILITIES;
    Stack.Parameters.DeviceCapabilities.Capabilities = &m_Capabilities;
    WdfRequestWdmFormatUsingStackLocation(Request, &Stack);

    WDF_REQUEST_SEND_OPTIONS_INIT(&Options, WDF_REQUEST_SEND_OPTION_SYNCHRONOUS);
    WdfRequestSend(Request, Target, &Options);
    Status = WdfRequestGetStatus(Request);
    if (!NT_SUCCESS(Status))
        DPRINT1("Hub %p capabilities query failed 0x%lx\n", m_Device, Status);

    WdfObjectDelete(Request);
    return Status;
}

/* The current interface layouts are asked for; the hub writes its own fields before each query */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
HubFdo::QueryInterfaces()
{
    NTSTATUS Status;

    PAGED_CODE();

    RtlZeroMemory(&m_Parent, sizeof(m_Parent));
    m_Parent.ChildHubObject = this;

    Status = WdfFdoQueryForInterface(m_Device,
                                     &GUID_UCXHUB_PARENT_INTERFACE,
                                     (PINTERFACE)&m_Parent,
                                     sizeof(m_Parent),
                                     UCXHUB_PARENT_INTERFACE_VERSION,
                                     NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p parent interface query failed 0x%lx\n", m_Device, Status);
        return Status;
    }

    if (IsRootHub())
    {
        m_ParentInfo.RootHubPdo = WdfIoTargetWdmGetTargetDeviceObject(WdfDeviceGetIoTarget(m_Device));
        m_RootHubTarget = WdfDeviceGetIoTarget(m_Device);
        m_MaxPortPower = HUB_ROOT_MAX_PORT_POWER;
    }

    return STATUS_SUCCESS;
}

/* The parent stack may not do selective suspend or SuperSpeedPlus isoch bursts */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
HubFdo::ProbeParentCapabilities()
{
    USBD_HANDLE Handle;
    NTSTATUS Status;

    PAGED_CODE();

    Status = USBD_CreateHandle(WdfDeviceWdmGetDeviceObject(m_Device),
                               WdfIoTargetWdmGetTargetDeviceObject(WdfDeviceGetIoTarget(m_Device)),
                               USBD_CLIENT_CONTRACT_VERSION_602,
                               HUB_TAG_HUB,
                               &Handle);
    if (NT_SUCCESS(Status))
    {
        if (!NT_SUCCESS(USBD_QueryUsbCapability(Handle,
                                                &GUID_USB_CAPABILITY_HIGH_BANDWIDTH_ISOCH,
                                                sizeof(m_SspIsochBurstCount),
                                                (PUCHAR)&m_SspIsochBurstCount,
                                                NULL)))
        {
            m_SspIsochBurstCount = 0;
        }

        Status = USBD_QueryUsbCapability(Handle, &GUID_USB_CAPABILITY_SELECTIVE_SUSPEND, 0, NULL, NULL);
        USBD_CloseHandle(Handle);
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p parent stack has no selective suspend 0x%lx\n", m_Device, Status);
        SetFlag(HubFlag::ParentNoSelectiveSuspend);
    }
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
HubFdo::AssignIdleSettings(
    _In_ WDF_POWER_POLICY_IDLE_TIMEOUT_TYPE TimeoutType)
{
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS Settings;

    PAGED_CODE();

    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&Settings, IdleCanWakeFromS0);
    Settings.IdleTimeout = m_IdleTimeout;
    Settings.UserControlOfIdleSettings = IdleAllowUserControl;
    Settings.Enabled = WdfUseDefault;
    Settings.DxState = PowerDeviceD2;
    Settings.IdleTimeoutType = TimeoutType;

    return WdfDeviceAssignS0IdleSettings(m_Device, &Settings);
}

/*
 * The root hub leaves its idle timeout to the power framework so it stays in D0
 * during surprise removal, which avoids a UCX deadlock on controller reset.
 * Failures here are ignored.
 */
_IRQL_requires_(PASSIVE_LEVEL)
VOID
HubFdo::ConfigureIdle()
{
    WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS Wake;
    NTSTATUS Status;

    PAGED_CODE();

    m_IdleTimeout = HUB_DEFAULT_IDLE_TIMEOUT;

    if (!HasFlag(HubFlag::ParentNoSelectiveSuspend) && m_Parent.ParentCanWake)
    {
        Status = AssignIdleSettings(IsRootHub() ? SystemManagedIdleTimeoutWithHint : DriverManagedIdleTimeout);
        if (NT_SUCCESS(Status))
        {
            SetFlag(HubFlag::S0IdleConfigured);

            if (HubDriver.RegisterPowerSetting != NULL &&
                !NT_SUCCESS(HubDriver.RegisterPowerSetting(WdfDeviceWdmGetDeviceObject(m_Device),
                                                           &GUID_HUB_IDLE_TIMEOUT_SETTING,
                                                           PowerSettingChanged,
                                                           this,
                                                           &m_IdleTimeoutSetting)))
            {
                DPRINT1("Hub %p idle timeout setting registration failed\n", m_Device);
                m_IdleTimeoutSetting = NULL;
            }
        }
        else
        {
            DPRINT1("Hub %p cannot idle, 0x%lx\n", m_Device, Status);
        }
    }

    m_PowerState = WdfPowerDeviceD3Final;

    WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_INIT(&Wake);
    Wake.ArmForWakeIfChildrenAreArmedForWake = TRUE;
    Wake.IndicateChildWakeOnParentWake = TRUE;
    Wake.Enabled = WdfFalse;
    Wake.UserControlOfWakeSettings = WakeDoNotAllowUserControl;

    Status = WdfDeviceAssignSxWakeSettings(m_Device, &Wake);
    if (!NT_SUCCESS(Status))
        DPRINT1("Hub %p Sx wake settings failed 0x%lx\n", m_Device, Status);
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
HubFdo::CreateQueue()
{
    WDF_IO_QUEUE_CONFIG Config;
    WDF_OBJECT_ATTRIBUTES Attributes;

    PAGED_CODE();

    /* Most requests return cached data; the ones that need D0 go through a machine */
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&Config, WdfIoQueueDispatchSequential);
    Config.PowerManaged = WdfFalse;
    Config.EvtIoDeviceControl = HubEvtIoDeviceControl;
    Config.EvtIoInternalDeviceControl = HubEvtIoInternalDeviceControl;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, HubQueueContext);
    Attributes.ExecutionLevel = WdfExecutionLevelPassive;

    return WdfIoQueueCreate(m_Device, &Config, &Attributes, WDF_NO_HANDLE);
}

/* The KTIMER based machine timer needs no allocation */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
HubFdo::InitializeMachine()
{
    WDF_OBJECT_ATTRIBUTES Attributes;
    WDFIOTARGET Target = WdfDeviceGetIoTarget(m_Device);
    HubKind Kind;
    NTSTATUS Status;

    PAGED_CODE();

    Status = m_Control.Create(m_Device, Target);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p control transfer setup failed 0x%lx\n", m_Device, Status);
        return Status;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = m_Device;

    Status = WdfRequestCreate(&Attributes, Target, &m_Interrupt.Request);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p interrupt request create failed 0x%lx\n", m_Device, Status);
        return Status;
    }

    KeInitializeEvent(&m_PnpEvent, NotificationEvent, FALSE);

    if (IsRootHub())
    {
        Kind = HubKind::Root;
    }
    else if (m_Parent.HubSpeed == UsbHighSpeed || m_Parent.HubSpeed == UsbFullSpeed)
    {
        Kind = HubKind::Usb20;
    }
    else if (m_Parent.HubSpeed == UsbSuperSpeed)
    {
        Kind = HubKind::Usb30;
    }
    else
    {
        /* A low speed hub cannot exist */
        DPRINT1("Hub %p has unsupported speed %lu\n", m_Device, (ULONG)m_Parent.HubSpeed);
        return STATUS_NOT_SUPPORTED;
    }

    m_Timer.Initialize(TimerFired, this);

    m_WorkItem = AllocateWorkItem();
    if (m_WorkItem == NULL)
    {
        DPRINT1("Hub %p work item allocation failed\n", m_Device);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    m_Machine.Initialize(this, Kind);
    return STATUS_SUCCESS;
}

/* Everything AddDevice does once the WDFDEVICE exists */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
HubFdo::Initialize(
    _In_ WDFDEVICE Device)
{
    WDF_DEVICE_PNP_CAPABILITIES PnpCaps;
    PNP_BUS_INFORMATION BusInfo;
    NTSTATUS Status;

    PAGED_CODE();

    Status = IoRegisterShutdownNotification(WdfDeviceWdmGetDeviceObject(Device));
    if (!NT_SUCCESS(Status))
        DPRINT1("Hub %p shutdown registration failed 0x%lx\n", Device, Status);

    m_Device = Device;
    m_WdmPdo = WdfDeviceWdmGetPhysicalDevice(Device);
    m_SelectedFeature = HUB_FEATURE_NONE;
    InitializeListHead(&m_HubListEntry);

    Status = HubAllocateHubNumber(&m_HubNumber);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p hub number allocation failed 0x%lx\n", Device, Status);
        return Status;
    }

    Status = HubUsb4Initialize(this);
    if (!NT_SUCCESS(Status))
        return Status;

    WdfDeviceSetSpecialFileSupport(Device, WdfSpecialFilePaging, TRUE);
    WdfDeviceSetSpecialFileSupport(Device, WdfSpecialFileHibernation, TRUE);
    WdfDeviceSetSpecialFileSupport(Device, WdfSpecialFileDump, TRUE);
    WdfDeviceSetSpecialFileSupport(Device, WdfSpecialFileBoot, TRUE);

    Status = QueryCapabilities();
    if (!NT_SUCCESS(Status))
        return Status;

    WDF_DEVICE_PNP_CAPABILITIES_INIT(&PnpCaps);
    PnpCaps.SurpriseRemovalOK = WdfTrue;
    WdfDeviceSetPnpCapabilities(Device, &PnpCaps);

    Status = QueryInterfaces();
    if (!NT_SUCCESS(Status))
        return Status;

    ProbeParentCapabilities();
    ConfigureIdle();

    RtlZeroMemory(&m_Stack, sizeof(m_Stack));
    m_Stack.Hub = UsbDevice();
    m_Stack.HubContext = (UCXHUB_HUB_CONTEXT)this;
    m_Stack.ClearTtBuffer = HubClearTtBuffer;
    m_Stack.NoPingResponse = HubNoPingResponse;

    Status = WdfFdoQueryForInterface(Device,
                                     &GUID_UCXHUB_STACK_INTERFACE,
                                     (PINTERFACE)&m_Stack,
                                     sizeof(m_Stack),
                                     UCXHUB_STACK_INTERFACE_VERSION_2,
                                     NULL);
    if (!NT_SUCCESS(Status))
    {
        /* A controller extension without endpoint priorities still answers version 1 */
        Status = WdfFdoQueryForInterface(Device,
                                         &GUID_UCXHUB_STACK_INTERFACE,
                                         (PINTERFACE)&m_Stack,
                                         sizeof(m_Stack),
                                         UCXHUB_STACK_INTERFACE_VERSION_1,
                                         NULL);
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p stack interface query failed 0x%lx\n", Device, Status);
        return Status;
    }

    RtlZeroMemory(&m_Usbdi, sizeof(m_Usbdi));
    Status = WdfFdoQueryForInterface(Device,
                                     &USB_BUS_INTERFACE_USBDI_GUID,
                                     (PINTERFACE)&m_Usbdi,
                                     sizeof(m_Usbdi),
                                     USB_BUSIF_USBDI_VERSION_3,
                                     IsRootHub() ? (PVOID)UsbDevice() : NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p USBDI interface query failed 0x%lx\n", Device, Status);
        return Status;
    }

    Status = CreateQueue();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p queue create failed 0x%lx\n", Device, Status);
        return Status;
    }

    BusInfo.BusTypeGuid = GUID_BUS_TYPE_USB;
    BusInfo.LegacyBusType = PNPBus;
    BusInfo.BusNumber = 0;
    WdfDeviceSetBusInformationForChildren(Device, &BusInfo);

    InitializeMux();

    Status = InitializeMachine();
    if (!NT_SUCCESS(Status))
        return Status;

    SetFlag(HubFlag::Initialized);
    return PostAndWait(HubEvent::DeviceAdded, "DeviceAdded");
}

NTSTATUS
NTAPI
HubFdo::EvtDeviceAdd(
    _In_ WDFDRIVER Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit)
{
    WDF_PNPPOWER_EVENT_CALLBACKS PnpPower;
    WDF_POWER_POLICY_EVENT_CALLBACKS PowerPolicy;
    WDF_OBJECT_ATTRIBUTES Attributes;
    WDFDEVICE Device;
    HubFdo* Hub;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, HubFdo);
    Attributes.EvtCleanupCallback = EvtCleanup;
    Attributes.EvtDestroyCallback = EvtDestroy;

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&PnpPower);
    PnpPower.EvtDeviceD0Entry = EvtD0Entry;
    PnpPower.EvtDeviceD0Exit = EvtD0Exit;
    PnpPower.EvtDevicePrepareHardware = EvtPrepareHardware;
    PnpPower.EvtDeviceReleaseHardware = EvtReleaseHardware;
    PnpPower.EvtDeviceSurpriseRemoval = EvtSurpriseRemoval;
    PnpPower.EvtDeviceUsageNotification = EvtUsageNotification;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &PnpPower);

    Status = WdfDeviceInitAssignWdmIrpPreprocessCallback(DeviceInit, EvtShutdown, IRP_MJ_SHUTDOWN, NULL, 0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Shutdown preprocess callback failed 0x%lx\n", Status);
        return Status;
    }

    WDF_POWER_POLICY_EVENT_CALLBACKS_INIT(&PowerPolicy);
    PowerPolicy.EvtDeviceArmWakeFromS0 = EvtArmWakeFromS0;
    PowerPolicy.EvtDeviceDisarmWakeFromS0 = EvtDisarmWakeFromS0;
    PowerPolicy.EvtDeviceArmWakeFromSx = EvtArmWakeFromSx;
    PowerPolicy.EvtDeviceDisarmWakeFromSx = EvtDisarmWakeFromSx;
    WdfDeviceInitSetPowerPolicyEventCallbacks(DeviceInit, &PowerPolicy);

    /* Children get release hardware first when D0 entry fails */
    WdfDeviceInitSetReleaseHardwareOrderOnFailure(DeviceInit, WdfReleaseHardwareOrderOnFailureAfterDescendants);

    Status = WdfDeviceCreate(&DeviceInit, &Attributes, &Device);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub WdfDeviceCreate failed 0x%lx\n", Status);
        return Status;
    }

    Hub = new (HubGetFdoContext(Device)) HubFdo();
    Status = Hub->Initialize(Device);

    /* A zero number keeps the cleanup callback from releasing it twice */
    if (!NT_SUCCESS(Status) && Hub->m_HubNumber != 0)
    {
        HubReleaseHubNumber(Hub->m_HubNumber);
        Hub->m_HubNumber = 0;
    }

    if (NT_SUCCESS(Status))
        DPRINT("Hub %p added, root %u\n", Device, Hub->IsRootHub());

    return Status;
}

/* PnP and power callbacks */

/* ReactOS PnP does not report container IDs yet, so a failed query counts as the system container */
_IRQL_requires_(PASSIVE_LEVEL)
static
BOOLEAN
NTAPI
HubFdoInSystemContainer(
    _In_ HubFdo* Hub)
{
    WDFMEMORY Memory;
    UNICODE_STRING ContainerId;
    PWCHAR Buffer;
    size_t Bytes;
    BOOLEAN Result;
    NTSTATUS Status;

    Status = WdfDeviceAllocAndQueryProperty(Hub->m_Device,
                                            DevicePropertyContainerID,
                                            PagedPool,
                                            WDF_NO_OBJECT_ATTRIBUTES,
                                            &Memory);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Hub %p container ID query failed 0x%lx\n", Hub, Status);
        return TRUE;
    }

    Buffer = (PWCHAR)WdfMemoryGetBuffer(Memory, &Bytes);
    ContainerId.Buffer = Buffer;
    ContainerId.Length = 0;
    while (ContainerId.Length + sizeof(WCHAR) <= Bytes && ContainerId.Length < MAXUSHORT - 1 &&
           Buffer[ContainerId.Length / sizeof(WCHAR)] != UNICODE_NULL)
    {
        ContainerId.Length += sizeof(WCHAR);
    }
    ContainerId.MaximumLength = ContainerId.Length;

    Result = RtlEqualUnicodeString(&ContainerId, &HubSystemContainerId, TRUE);
    WdfObjectDelete(Memory);
    return Result;
}

/**
 * @brief
 * Turns off link power management below a hub on a tunneled or unknown USB4 port, or below a root hub outside the system container.
 */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
HubFdoApplyTunnelLpmRule(
    _In_ HubFdo* Hub)
{
    HubPdo* ParentPdo;
    BOOLEAN Disable;

    if (Hub->IsRootHub())
    {
        Disable = !HubFdoInSystemContainer(Hub);
    }
    else
    {
        /* The parent interface context of an external hub is its PDO in the parent hub */
        ParentPdo = (HubPdo*)Hub->m_Parent.Header.Context;
        Disable = HubUsb4NeedsHostPower(ParentPdo->m_Child);
    }

    if (!Disable)
        return;

    DPRINT("Hub %p turns link power management off below it\n", Hub);
    InterlockedOr((volatile LONG*)&Hub->m_ParentInfo.Flags, HUB_PARENT_DISABLE_LPM);
}

/* A failed prepare still puts the hub on the list; release hardware takes it off */
NTSTATUS
NTAPI
HubFdo::EvtPrepareHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated)
{
    HubFdo* Hub = FromDevice(Device);
    UNICODE_STRING Name;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(ResourcesRaw);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    PAGED_CODE();

    DPRINT("Hub %p prepare hardware, root %u\n", Device, Hub->IsRootHub());

    HubReadHubRegistryValues(Hub);
    HubReadUsb4HostName(Hub);
    HubFdoApplyTunnelLpmRule(Hub);

    Status = Hub->PostAndWait(HubEvent::PrepareHardware, "PrepareHardware");
    if (!NT_SUCCESS(Status))
        goto Exit;

    if (Hub->HasFlag(HubFlag::DisableSuperSpeed))
    {
        Status = IoSetDevicePropertyData(Hub->m_WdmPdo,
                                         &DEVPKEY_Device_DeviceDesc,
                                         LOCALE_NEUTRAL,
                                         PLUGPLAY_PROPERTY_PERSISTENT,
                                         DEVPROP_TYPE_STRING,
                                         sizeof(HubNonFunctionalDescription),
                                         (PVOID)HubNonFunctionalDescription);
        if (!NT_SUCCESS(Status))
            DPRINT1("Hub %p description update failed 0x%lx\n", Device, Status);
    }

    RtlZeroMemory(&Name, sizeof(Name));
    Hub->m_Parent.QueryHubLinkName(Hub->m_Parent.Header.Context, &Name);
    if (Name.Length == 0)
    {
        DPRINT1("Hub %p has no symbolic link name\n", Device);
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    /* Twice the length is allocated */
    Hub->m_SymbolicLinkName.Buffer = (PWCH)ExAllocatePoolWithTag(NonPagedPool, Name.Length * 2, HUB_TAG_HUB);
    if (Hub->m_SymbolicLinkName.Buffer == NULL)
    {
        DPRINT1("Hub %p symbolic link name allocation failed\n", Device);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit;
    }

    Hub->m_SymbolicLinkName.MaximumLength = Name.Length;
    RtlCopyUnicodeString(&Hub->m_SymbolicLinkName, &Name);

    Status = HubAcpiReadPortAttributes(Hub);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p ACPI port walk failed 0x%lx\n", Device, Status);
        goto Exit;
    }

    HubConnectorMapPorts(Hub);
    HubUsb4MapDvsecHosts(Hub);

    Status = HubUsb4AssignPortMappings(Hub);
    if (!NT_SUCCESS(Status))
        DPRINT1("Hub %p USB4 port mapping property failed 0x%lx\n", Device, Status);

    HubWmiRegister(Hub);

    if (Hub->m_Stack.StreamsSupported(Hub->UsbDevice()))
        Hub->m_StreamsSupported = TRUE;

    HubDromReportFirmwareUpdateDevice(Hub);

    Status = STATUS_SUCCESS;

Exit:
    {
        HubWaitLockGuard Guard(HubDriver.HubListLock);

        InsertTailList(&HubDriver.HubList, &Hub->m_HubListEntry);
        Hub->m_OnHubList = TRUE;
    }

    DPRINT("Hub %p prepare hardware done 0x%lx, flags 0x%lx\n", Device, Status, (ULONG)Hub->m_Flags);

    return Status;
}

/* The controller is kept in D0 while the devices below are torn down in it */
NTSTATUS
NTAPI
HubFdo::EvtReleaseHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesTranslated)
{
    HubFdo* Hub = FromDevice(Device);
    UCXHUB_STOP_IDLE_CONTEXT StopIdle;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(ResourcesTranslated);

    PAGED_CODE();

    DPRINT("Hub %p release hardware\n", Device);

    Hub->ClearFlag(HubFlag::PowerReferenceAllowed);
    HubConnectorUnmapPorts(Hub);
    HubUsb4UnregisterNotification(Hub);

    RtlZeroMemory(&StopIdle, sizeof(StopIdle));
    Hub->m_Stack.BlockControllerIdle(Hub->UsbDevice(), &StopIdle);

    Status = Hub->PostAndWait(HubEvent::ReleaseHardware, "ReleaseHardware");

    Hub->m_Stack.AllowControllerIdle(Hub->UsbDevice(), &StopIdle);

    {
        HubWaitLockGuard Guard(HubDriver.HubListLock);

        if (Hub->m_OnHubList)
        {
            RemoveEntryList(&Hub->m_HubListEntry);
            InitializeListHead(&Hub->m_HubListEntry);
            Hub->m_OnHubList = FALSE;
        }
    }

    if (Hub->m_SymbolicLinkName.Buffer != NULL)
    {
        ExFreePoolWithTag(Hub->m_SymbolicLinkName.Buffer, HUB_TAG_HUB);
        RtlZeroMemory(&Hub->m_SymbolicLinkName, sizeof(Hub->m_SymbolicLinkName));
    }

    return Status;
}

/* The on/off setting takes a power reference, so it waits for the first D0 */
NTSTATUS
NTAPI
HubFdo::EvtD0Entry(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState)
{
    HubFdo* Hub = FromDevice(Device);

    UNREFERENCED_PARAMETER(PreviousState);

    Hub->m_PowerState = WdfPowerDeviceD0;
    Hub->SetFlag(HubFlag::PowerReferenceAllowed);
    HubUsb4RegisterNotification(Hub);

    if (Hub->m_SelectiveSuspendSetting == NULL &&
        Hub->HasFlag(HubFlag::S0IdleConfigured) &&
        HubDriver.RegisterPowerSetting != NULL &&
        !NT_SUCCESS(HubDriver.RegisterPowerSetting(WdfDeviceWdmGetDeviceObject(Device),
                                                   &GUID_HUB_SELECTIVE_SUSPEND_POLICY,
                                                   PowerSettingChanged,
                                                   Hub,
                                                   &Hub->m_SelectiveSuspendSetting)))
    {
        DPRINT1("Hub %p selective suspend setting registration failed\n", Device);
        Hub->m_SelectiveSuspendSetting = NULL;
    }

    DPRINT("Hub %p D0 entry from state %d, action %d\n",
           Device,
           (int)PreviousState,
           (int)WdfDeviceGetSystemPowerAction(Device));

    switch (WdfDeviceGetSystemPowerAction(Device))
    {
        case PowerActionSleep:
        case PowerActionHibernate:
        case PowerActionShutdown:
        case PowerActionShutdownReset:
        case PowerActionShutdownOff:
            return Hub->PostAndWait(HubEvent::D0EntryFromSx, "D0EntryFromSx");

        case PowerActionNone:
            return Hub->PostAndWait(HubEvent::D0Entry, "D0Entry");

        default:
            DPRINT1("Hub %p D0 entry with unexpected power action %d\n",
                    Device,
                    (int)WdfDeviceGetSystemPowerAction(Device));
            NT_ASSERT(FALSE);
            return STATUS_SUCCESS;
    }
}

NTSTATUS
NTAPI
HubFdo::EvtD0Exit(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState)
{
    HubFdo* Hub = FromDevice(Device);

    Hub->m_PowerState = TargetState;
    Hub->ClearFlag(HubFlag::PowerReferenceAllowed);

    DPRINT("Hub %p D0 exit to state %d, action %d\n",
           Device,
           (int)TargetState,
           (int)WdfDeviceGetSystemPowerAction(Device));

    switch (WdfDeviceGetSystemPowerAction(Device))
    {
        case PowerActionSleep:
        case PowerActionHibernate:
        case PowerActionShutdown:
        case PowerActionShutdownReset:
        case PowerActionShutdownOff:
            return Hub->PostAndWait(HubEvent::D0Exit, "D0Exit");

        case PowerActionNone:
            if (TargetState == WdfPowerDeviceD3Final)
                return Hub->PostAndWait(HubEvent::D0ExitFinal, "D0ExitFinal");
            return Hub->PostAndWait(HubEvent::D0Exit, "D0Exit");

        default:
            DPRINT1("Hub %p D0 exit with unexpected power action %d\n",
                    Device,
                    (int)WdfDeviceGetSystemPowerAction(Device));
            return STATUS_SUCCESS;
    }
}

/* The hub machine never sees surprise removal; only the ports do */
VOID
NTAPI
HubFdo::EvtSurpriseRemoval(
    _In_ WDFDEVICE Device)
{
    DPRINT("Hub %p surprise removed\n", Device);
    FromDevice(Device)->QueueSurpriseRemovalToPorts();
}

/* Leaving a path is ignored and wake stays armed */
VOID
NTAPI
HubFdo::EvtUsageNotification(
    _In_ WDFDEVICE Device,
    _In_ WDF_SPECIAL_FILE_TYPE NotificationType,
    _In_ BOOLEAN IsInNotificationPath)
{
    static BOOLEAN CodeLocked = FALSE;
    HubFdo* Hub = FromDevice(Device);
    WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS Wake;
    NTSTATUS Status;

    PAGED_CODE();

    if (!IsInNotificationPath)
        return;

    DPRINT("Hub %p in special file path %d\n", Device, (int)NotificationType);

    switch (NotificationType)
    {
        case WdfSpecialFilePaging:
        case WdfSpecialFileBoot:
            Hub->SetFlag(HubFlag::InBootOrPagingPath);

            WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_INIT(&Wake);
            Wake.ArmForWakeIfChildrenAreArmedForWake = TRUE;
            Wake.IndicateChildWakeOnParentWake = TRUE;
            Wake.Enabled = WdfTrue;
            Wake.UserControlOfWakeSettings = WakeDoNotAllowUserControl;
            Status = WdfDeviceAssignSxWakeSettings(Device, &Wake);
            if (!NT_SUCCESS(Status))
                DPRINT1("Hub %p boot path Sx wake settings failed 0x%lx\n", Device, Status);

            /* Fall through */
        case WdfSpecialFileHibernation:
            if (Hub->m_NeedsForwardProgress)
                return;

            Hub->m_NeedsForwardProgress = TRUE;
            HubRequestReservedIo(Hub);

            /* Never unlocked */
            if (Hub->IsRootHub() && !CodeLocked)
            {
                CodeLocked = TRUE;
                MmLockPagableCodeSection((PVOID)HubFdo::EvtUsageNotification);
            }
            break;

        default:
            break;
    }
}

NTSTATUS
NTAPI
HubFdo::EvtArmWakeFromS0(
    _In_ WDFDEVICE Device)
{
    HubFdo* Hub = FromDevice(Device);

    Hub->SetFlag(HubFlag::ArmedForWake);
    Hub->SetFlag(HubFlag::WaitWakeQueued);
    return STATUS_SUCCESS;
}

VOID
NTAPI
HubFdo::EvtDisarmWakeFromS0(
    _In_ WDFDEVICE Device)
{
    HubFdo* Hub = FromDevice(Device);

    Hub->ClearFlag(HubFlag::ArmedForWake);
    Hub->ClearFlag(HubFlag::WaitWakeQueued);
}

NTSTATUS
NTAPI
HubFdo::EvtArmWakeFromSx(
    _In_ WDFDEVICE Device)
{
    FromDevice(Device)->SetFlag(HubFlag::WaitWakeQueued);
    return STATUS_SUCCESS;
}

VOID
NTAPI
HubFdo::EvtDisarmWakeFromSx(
    _In_ WDFDEVICE Device)
{
    FromDevice(Device)->ClearFlag(HubFlag::WaitWakeQueued);
}

VOID
NTAPI
HubFdo::EvtCleanup(
    _In_ WDFOBJECT Object)
{
    HubFdo* Hub = HubGetFdoContext(Object);
    NTSTATUS Status;

    PAGED_CODE();

    DPRINT("Hub %p cleanup\n", Object);

    HubUsb4UnregisterNotification(Hub);
    HubUsb4CloseAllHosts(Hub);

    if (Hub->m_IdleTimeoutSetting != NULL)
    {
        Status = HubDriver.UnregisterPowerSetting(Hub->m_IdleTimeoutSetting);
        if (!NT_SUCCESS(Status))
            DPRINT1("Hub %p idle timeout setting unregister failed 0x%lx\n", Object, Status);
        Hub->m_IdleTimeoutSetting = NULL;
    }

    if (Hub->m_SelectiveSuspendSetting != NULL)
    {
        Status = HubDriver.UnregisterPowerSetting(Hub->m_SelectiveSuspendSetting);
        if (!NT_SUCCESS(Status))
            DPRINT1("Hub %p selective suspend setting unregister failed 0x%lx\n", Object, Status);
        Hub->m_SelectiveSuspendSetting = NULL;
    }

    if (Hub->HasFlag(HubFlag::Initialized))
        Hub->PostAndWait(HubEvent::DeviceCleanup, "DeviceCleanup");

    Hub->FlushAndDeleteWorkItem(&Hub->m_WorkItem);

    if (Hub->m_ConfigDescriptor != NULL)
    {
        ExFreePoolWithTag(Hub->m_ConfigDescriptor, HUB_TAG_HUB);
        Hub->m_ConfigDescriptor = NULL;
    }

    if (Hub->m_HubNumber != 0)
    {
        HubReleaseHubNumber(Hub->m_HubNumber);
        Hub->m_HubNumber = 0;
    }
}

/* Waits for a running timer callback */
VOID
NTAPI
HubFdo::EvtDestroy(
    _In_ WDFOBJECT Object)
{
    HubFdo* Hub = HubGetFdoContext(Object);

    if (!Hub->HasFlag(HubFlag::Initialized))
        return;

    Hub->m_Timer.Cancel();
    KeFlushQueuedDpcs();
}

/* The IRP is completed here and never sent down */
NTSTATUS
NTAPI
HubFdo::EvtShutdown(
    _In_ WDFDEVICE Device,
    _Inout_ PIRP Irp)
{
    HubUxdShutdownCleanup(FromDevice(Device));

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

/* The timeout path always assigns a driver managed timeout, even on the root hub */
NTSTATUS
NTAPI
HubFdo::PowerSettingChanged(
    _In_ LPCGUID SettingGuid,
    _In_reads_bytes_(ValueLength) PVOID Value,
    _In_ ULONG ValueLength,
    _Inout_opt_ PVOID Context)
{
    HubFdo* Hub = (HubFdo*)Context;
    ULONG Setting;
    NTSTATUS Status;

    PAGED_CODE();

    if (!Hub->HasFlag(HubFlag::S0IdleConfigured))
    {
        DPRINT1("Hub %p power setting change without idle support\n", Hub->m_Device);
        return STATUS_INVALID_PARAMETER;
    }

    if (IsEqualGUID(*SettingGuid, GUID_HUB_SELECTIVE_SUSPEND_POLICY))
    {
        if (ValueLength < sizeof(ULONG) || *(PULONG)Value > 1)
        {
            DPRINT1("Hub %p bad selective suspend setting, length %lu\n", Hub->m_Device, ValueLength);
            return STATUS_INVALID_PARAMETER;
        }

        if (*(PULONG)Value == 1)
        {
            if (Hub->HasFlag(HubFlag::GlobalPowerReference))
            {
                DPRINT("Hub %p selective suspend on, dropping power reference\n", Hub->m_Device);
                WdfDeviceResumeIdle(Hub->m_Device);
                Hub->ClearFlag(HubFlag::GlobalPowerReference);
            }
        }
        else if (!Hub->HasFlag(HubFlag::GlobalPowerReference))
        {
            DPRINT("Hub %p selective suspend off, taking power reference\n", Hub->m_Device);

            Status = WdfDeviceStopIdle(Hub->m_Device, FALSE);
            if (NT_SUCCESS(Status))
                Hub->SetFlag(HubFlag::GlobalPowerReference);
            else
                DPRINT1("Hub %p stop idle failed 0x%lx\n", Hub->m_Device, Status);
        }

        return STATUS_SUCCESS;
    }

    if (IsEqualGUID(*SettingGuid, GUID_HUB_IDLE_TIMEOUT_SETTING))
    {
        if (ValueLength < sizeof(ULONG))
        {
            DPRINT1("Hub %p bad idle timeout setting, length %lu\n", Hub->m_Device, ValueLength);
            return STATUS_INVALID_PARAMETER;
        }

        Setting = *(PULONG)Value;
        if (Setting == Hub->m_IdleTimeout)
            return STATUS_SUCCESS;

        DPRINT("Hub %p idle timeout now %lu ms\n", Hub->m_Device, Setting);

        Hub->m_IdleTimeout = Setting;
        Status = Hub->AssignIdleSettings(DriverManagedIdleTimeout);
        if (!NT_SUCCESS(Status))
            DPRINT1("Hub %p idle timeout update failed 0x%lx\n", Hub->m_Device, Status);

        return STATUS_SUCCESS;
    }

    DPRINT1("Hub %p unknown power setting\n", Hub->m_Device);
    return STATUS_INVALID_PARAMETER;
}

/* Hub machine engine hooks */

VOID
NTAPI
HubFdo::TimerFired(
    _In_ PVOID Context)
{
    ((HubFdo*)Context)->Post(HubEvent::TimerFired);
}

VOID
NTAPI
HubFdo::MachineWorkItem(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context,
    _In_ PUCXHUB_WORKITEM WorkItem)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(WorkItem);

    ((HubFdo*)Context)->m_Machine.SmContinueOnPassive();
}

VOID
HubMachine::ReferenceHub()
{
    WdfObjectReference(m_Hub->m_Device);
}

VOID
HubMachine::DereferenceHub()
{
    WdfObjectDereference(m_Hub->m_Device);
}

VOID
HubMachine::QueuePassiveWork()
{
    m_Hub->EnqueueWorkItem(m_Hub->m_WorkItem, HubFdo::MachineWorkItem, m_Hub, m_Hub->m_NeedsForwardProgress);
}

/* Hub machine actions */

VOID
HubMachine::AcquirePowerReference()
{
    m_Hub->AcquirePowerReference();
}

VOID
HubMachine::ReleasePowerReference()
{
    m_Hub->ReleasePowerReference();
}

VOID
HubMachine::SignalPnpEvent()
{
    DPRINT("Hub %p PnP event done in %s\n", m_Hub->m_Device, SmCurrent()->Name);

    m_Hub->m_PnpStatus = STATUS_SUCCESS;
    KeSetEvent(&m_Hub->m_PnpEvent, IO_NO_INCREMENT, FALSE);
}

VOID
HubMachine::SignalPnpFailure()
{
    DPRINT1("Hub %p start failed in state %s, depth %lu\n",
            m_Hub->m_Device,
            SmCurrent()->Name,
            SmDepth());

    m_Hub->m_PnpStatus = STATUS_UNSUCCESSFUL;
    KeSetEvent(&m_Hub->m_PnpEvent, IO_NO_INCREMENT, FALSE);
}

VOID
HubMachine::ReportFailureToPnp()
{
    DPRINT1("Hub %p failed in state %s, message 0x%lx\n",
            m_Hub->m_Device,
            SmCurrent()->Name,
            m_Hub->m_FailureMessageId);

    if (m_Hub->m_FailureMessageId != 0)
        HubReportPnpProblem(m_Hub->m_Device, m_Hub->m_FailureMessageId);

    WdfDeviceSetFailed(m_Hub->m_Device, WdfDeviceFailedNoRestart);
}

VOID
HubMachine::BugcheckBootHub()
{
    DPRINT1("Boot path hub %p failed, bugchecking\n", m_Hub->m_Device);

    KeBugCheckEx(HUB_BUGCHECK_USB3,
                 HUB_USB3_BOOT_DEVICE_FAILED,
                 (ULONG_PTR)WdfDeviceWdmGetDeviceObject(m_Hub->m_Device),
                 HUB_BOOT_DEVICE_IS_HUB,
                 0);
}

/* Write failures are ignored */
VOID
HubMachine::LogResetRecovery()
{
    DECLARE_CONST_UNICODE_STRING(CountName, L"HardResetCount");
    DECLARE_CONST_UNICODE_STRING(StatusName, L"LastHubResetPortStatus");
    WDFKEY Key;

    m_Hub->m_LifetimeRecoveryCount++;

    DPRINT1("Hub %p reset recovery %lu, last port status 0x%lx\n",
            m_Hub->m_Device,
            m_Hub->m_LifetimeRecoveryCount,
            (ULONG)m_Hub->m_LastResetPortStatus);

    if (!NT_SUCCESS(WdfDeviceOpenRegistryKey(m_Hub->m_Device,
                                             PLUGPLAY_REGKEY_DEVICE,
                                             KEY_WRITE,
                                             WDF_NO_OBJECT_ATTRIBUTES,
                                             &Key)))
    {
        return;
    }

    WdfRegistryAssignULong(Key, &CountName, m_Hub->m_LifetimeRecoveryCount);

    if (m_Hub->m_LastResetPortStatus != 0)
    {
        WdfRegistryAssignULong(Key, &StatusName, m_Hub->m_LastResetPortStatus);
        m_Hub->m_LastResetPortStatus = 0;
    }

    WdfRegistryClose(Key);
}

VOID
HubMachine::LogResetOnResume()
{
    DPRINT1("Hub %p was reset on resume\n", m_Hub->m_Device);
}

BOOLEAN
HubMachine::IsInBootPath()
{
    return m_Hub->HasFlag(HubFlag::InBootOrPagingPath);
}

BOOLEAN
HubMachine::IsDepthZero()
{
    return m_Hub->IsRootHub();
}

/* Timers */

VOID
HubMachine::ArmResetRetryTimer()
{
    m_Hub->m_Timer.Start(HUB_RESET_RETRY_TIME);
}

VOID
HubMachine::ArmOverCurrentTimer()
{
    m_Hub->m_Timer.Start(HUB_OVERCURRENT_TIME);
}

VOID
HubMachine::StartDescriptorRetryTimer()
{
    m_Hub->m_Timer.Start(HUB_DESCRIPTOR_RETRY_TIME);
}

/* Counters; the hub reset and pipe reset counts are one field */

VOID
HubMachine::InitializeResetCount()
{
    m_Hub->m_ResetCount = 0;
}

VOID
HubMachine::ClearPipeResetCount()
{
    m_Hub->m_ResetCount = 0;
}

VOID
HubMachine::InitializeDescriptorRetries()
{
    m_Hub->m_DescriptorRetryCount = 0;
}

BOOLEAN
HubMachine::ResetLimitReached()
{
    USHORT Previous = m_Hub->m_ResetCount++;

    if (Previous <= HUB_RESET_LIMIT)
        return FALSE;

    DPRINT1("Hub %p reset failed %lu times\n", m_Hub->m_Device, (ULONG)Previous + 1);
    m_Hub->m_FailureMessageId = HUB_MSG_HUB_RESET_FAILED;
    return TRUE;
}

BOOLEAN
HubMachine::PipeResetLimitReached()
{
    if (++m_Hub->m_ResetCount <= HUB_RESET_LIMIT)
        return FALSE;

    DPRINT1("Hub %p interrupt pipe reset limit reached\n", m_Hub->m_Device);
    return TRUE;
}

/* The hub reset limit is used here too */
BOOLEAN
HubMachine::DescriptorRetriesExhausted()
{
    if (m_Hub->m_DescriptorRetryCount++ <= HUB_RESET_LIMIT)
        return FALSE;

    DPRINT1("Hub %p hub descriptor retries exhausted\n", m_Hub->m_Device);
    return TRUE;
}

/* At most ten recoveries a minute; the first call always opens a new window */
BOOLEAN
HubMachine::RecoveryLimitReached()
{
    LARGE_INTEGER Now;

    KeQuerySystemTime(&Now);

    if (Now.QuadPart - m_Hub->m_FirstRecoveryTime.QuadPart > HUB_RECOVERY_WINDOW)
    {
        m_Hub->m_RecoveryCount = 1;
        m_Hub->m_FirstRecoveryTime = Now;
        return FALSE;
    }

    if (++m_Hub->m_RecoveryCount < HUB_RECOVERY_LIMIT)
        return FALSE;

    DPRINT1("Hub %p too many resets in a minute\n", m_Hub->m_Device);
    m_Hub->m_FailureMessageId = HUB_MSG_HUB_RESET_FAILED;
    return TRUE;
}
