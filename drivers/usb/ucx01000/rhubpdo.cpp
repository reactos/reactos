/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Root hub PDO: creation, PnP and power, and routing of the hub's IOCTLs
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ucx01000.h"

#define NDEBUG
#include <debug.h>

/* SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RW_RES_R */
static const UNICODE_STRING UcxRootHubPdoSddl =
    RTL_CONSTANT_STRING(L"D:P(A;;GA;;;SY)(A;;GRGWGX;;;BA)(A;;GRGW;;;WD)(A;;GR;;;RC)");

static IO_COMPLETION_ROUTINE UcxDeviceMgmtCompletion;
static IO_COMPLETION_ROUTINE UcxRootHubGetInfoCompletion;

#define UCX_ROOT_HUB_ID L"USB\\ROOT_HUB30"

/* Both carry their NUL inside Length */
static const WCHAR UcxRootHubDeviceIdText[] = UCX_ROOT_HUB_ID L"\0";
static const WCHAR UcxRootHubInstanceIdText[] = L"0\0";

/* Character counts of the three IDs when every field prints four wide */
#define UCX_HWID_FULL_CHARS  RTL_NUMBER_OF(UCX_ROOT_HUB_ID L"&VID0000&PID0000&REV0000")
#define UCX_HWID_SHORT_CHARS RTL_NUMBER_OF(UCX_ROOT_HUB_ID L"&VID0000&PID0000")
#define UCX_HWID_PLAIN_CHARS RTL_NUMBER_OF(UCX_ROOT_HUB_ID)
#define UCX_HWID_BUFFER_CHARS ((UCX_HWID_FULL_CHARS + UCX_HWID_SHORT_CHARS + UCX_HWID_PLAIN_CHARS + 1) * sizeof(WCHAR))

/* '#' marks each separator and becomes NUL at the offsets a four wide field gives */
static const SIZE_T UcxHardwareIdSeparators[] =
{
    UCX_HWID_FULL_CHARS - 1,
    UCX_HWID_FULL_CHARS + UCX_HWID_SHORT_CHARS - 1,
    UCX_HWID_FULL_CHARS + UCX_HWID_SHORT_CHARS + UCX_HWID_PLAIN_CHARS - 1
};

/** Most specific ID first, ending with the plain root hub ID. */
static
VOID
NTAPI
UcxBuildRootHubHardwareIds(
    _In_ const UCX_CONTROLLER_CONFIG* Config,
    _Out_writes_(BufferChars) PWCHAR Buffer,
    _In_ SIZE_T BufferChars,
    _Out_ PUNICODE_STRING Ids)
{
    const UCX_CONTROLLER_PCI_INFORMATION* Pci = &Config->PciDeviceInfo;
    const UCX_CONTROLLER_ACPI_INFORMATION* Acpi = &Config->AcpiDeviceInfo;
    ULONG Index;

    if (Config->ParentBusType != UcxControllerParentBusTypePci &&
        Config->ParentBusType != UcxControllerParentBusTypeAcpi)
    {
        Ids->Buffer = (PWCH)UcxRootHubDeviceIdText;
        Ids->Length = sizeof(UcxRootHubDeviceIdText) - sizeof(WCHAR);
        Ids->MaximumLength = sizeof(UcxRootHubDeviceIdText);
        return;
    }

    RtlInitEmptyUnicodeString(Ids, Buffer, (USHORT)(BufferChars * sizeof(WCHAR)));

    /* A truncated result is used as is */
    if (Config->ParentBusType == UcxControllerParentBusTypePci)
    {
        RtlUnicodeStringPrintf(Ids,
                               UCX_ROOT_HUB_ID L"&VID%04X&PID%04X&REV%04X#" UCX_ROOT_HUB_ID L"&VID%04X&PID%04X#" UCX_ROOT_HUB_ID L"#",
                               Pci->VendorId, Pci->DeviceId, Pci->RevisionId,
                               Pci->VendorId, Pci->DeviceId);
    }
    else
    {
        RtlUnicodeStringPrintf(Ids,
                               UCX_ROOT_HUB_ID L"&VID%S&PID%S&REV%S#" UCX_ROOT_HUB_ID L"&VID%S&PID%S#" UCX_ROOT_HUB_ID L"#",
                               Acpi->VendorId, Acpi->DeviceId, Acpi->RevisionId,
                               Acpi->VendorId, Acpi->DeviceId);
    }

    for (Index = 0; Index < RTL_NUMBER_OF(UcxHardwareIdSeparators); Index++)
        Buffer[UcxHardwareIdSeparators[Index]] = UNICODE_NULL;
}

/** D2 is the wake state for every S state the controller can wake from. */
VOID
UcxRootHub::BuildPowerCapabilities()
{
    const DEVICE_CAPABILITIES* Hc = &m_Controller->m_HcCaps;
    SYSTEM_POWER_STATE FirstWake;
    ULONG State;

    WDF_DEVICE_POWER_CAPABILITIES_INIT(&m_PowerCaps);
    m_PowerCaps.DeviceD1 = WdfFalse;
    m_PowerCaps.DeviceD2 = WdfTrue;
    m_PowerCaps.WakeFromD0 = WdfTrue;
    m_PowerCaps.WakeFromD1 = WdfFalse;
    m_PowerCaps.WakeFromD2 = WdfTrue;
    m_PowerCaps.WakeFromD3 = WdfFalse;
    m_PowerCaps.DeviceWake = PowerDeviceD2;

    FirstWake = (Hc->SystemWake == PowerSystemUnspecified) ? PowerSystemWorking : Hc->SystemWake;
    m_PowerCaps.SystemWake = FirstWake;

    for (State = PowerSystemSleeping1; State <= PowerSystemHibernate; State++)
    {
        if (State <= (ULONG)Hc->SystemWake && Hc->DeviceState[State] != PowerDeviceUnspecified)
        {
            m_PowerCaps.DeviceState[State] = PowerDeviceD2;
            if (FirstWake == PowerSystemShutdown)
                m_PowerCaps.SystemWake = (SYSTEM_POWER_STATE)State;
        }
        else
        {
            m_PowerCaps.DeviceState[State] = PowerDeviceD3;
        }
    }

    /* Hybrid sleep: waking from S3 means waking from S4 too */
    if (m_PowerCaps.DeviceState[PowerSystemSleeping3] == PowerDeviceD2 &&
        m_PowerCaps.DeviceState[PowerSystemHibernate] != PowerDeviceD2)
    {
        m_PowerCaps.DeviceState[PowerSystemHibernate] = PowerDeviceD2;
        m_PowerCaps.SystemWake = PowerSystemHibernate;
    }
}

NTSTATUS
UcxRootHub::CreatePdo()
{
    static const UNICODE_STRING InstanceId =
    {
        sizeof(UcxRootHubInstanceIdText) - sizeof(WCHAR),
        sizeof(UcxRootHubInstanceIdText),
        (PWCH)UcxRootHubInstanceIdText
    };
    static const UNICODE_STRING DeviceId =
    {
        sizeof(UcxRootHubDeviceIdText) - sizeof(WCHAR),
        sizeof(UcxRootHubDeviceIdText),
        (PWCH)UcxRootHubDeviceIdText
    };
    static UCHAR PnpMinor = IRP_MN_QUERY_INTERFACE;
    static UCHAR PowerMinor = IRP_MN_SET_POWER;
    WDF_PNPPOWER_EVENT_CALLBACKS PnpPower;
    WDF_PDO_EVENT_CALLBACKS PdoCallbacks;
    WDF_OBJECT_ATTRIBUTES Attributes;
    WDF_IO_QUEUE_CONFIG QueueConfig;
    PNP_BUS_INFORMATION BusInfo;
    WCHAR IdBuffer[UCX_HWID_BUFFER_CHARS];
    WCHAR NameBuffer[64];
    UNICODE_STRING HardwareIds;
    UNICODE_STRING Name;
    PWDFDEVICE_INIT Init;
    UcxRootHubPdoContext* PdoContext;
    WDFDEVICE Pdo = NULL;
    WDFDEVICE Fdo = m_Controller->m_Fdo;
    ULONG Index;
    NTSTATUS Status;

    BusInfo.BusTypeGuid = GUID_BUS_TYPE_USB;
    BusInfo.LegacyBusType = PNPBus;
    BusInfo.BusNumber = 0;
    WdfDeviceSetBusInformationForChildren(Fdo, &BusInfo);

    Init = WdfPdoInitAllocate(Fdo);
    if (Init == NULL)
    {
        DPRINT1("Root hub PDO init allocation failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = WdfPdoInitAssignInstanceID(Init, &InstanceId);
    if (NT_SUCCESS(Status))
        Status = WdfPdoInitAssignDeviceID(Init, &DeviceId);
    if (NT_SUCCESS(Status))
    {
        UcxBuildRootHubHardwareIds(&m_Controller->m_Config, IdBuffer, ARRAYSIZE(IdBuffer), &HardwareIds);
        Status = WdfPdoInitAddHardwareID(Init, &HardwareIds);
    }

    if (NT_SUCCESS(Status))
    {
        /* The root hub's queues live on the controller FDO */
        WdfPdoInitAllowForwardingRequestToParent(Init);

        Status = WdfDeviceInitAssignWdmIrpPreprocessCallback(Init,
                                                             UcxEvtRootHubPreprocessInternalIoctl,
                                                             IRP_MJ_INTERNAL_DEVICE_CONTROL,
                                                             NULL,
                                                             0);
    }
    if (NT_SUCCESS(Status))
    {
        Status = WdfDeviceInitAssignWdmIrpPreprocessCallback(Init,
                                                             UcxEvtRootHubPreprocessQueryInterface,
                                                             IRP_MJ_PNP,
                                                             &PnpMinor,
                                                             1);
    }
    if (NT_SUCCESS(Status))
    {
        Status = WdfDeviceInitAssignWdmIrpPreprocessCallback(Init,
                                                             UcxEvtRootHubPreprocessSetPower,
                                                             IRP_MJ_POWER,
                                                             &PowerMinor,
                                                             1);
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Root hub PDO ID or preprocess setup failed 0x%lx\n", Status);
        WdfDeviceInitFree(Init);
        return Status;
    }

    /* Every request reaching the HCD then carries the HCD's request context */
    WdfDeviceInitSetRequestAttributes(Init, &m_Config.WdfRequestAttributes);

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&PnpPower);
    PnpPower.EvtDevicePrepareHardware = UcxEvtRootHubPrepareHardware;
    PnpPower.EvtDeviceReleaseHardware = UcxEvtRootHubReleaseHardware;
    PnpPower.EvtDeviceD0Entry = UcxEvtRootHubD0Entry;
    PnpPower.EvtDeviceD0Exit = UcxEvtRootHubD0Exit;
    WdfDeviceInitSetPnpPowerEventCallbacks(Init, &PnpPower);

    WDF_PDO_EVENT_CALLBACKS_INIT(&PdoCallbacks);
    PdoCallbacks.EvtDeviceEnableWakeAtBus = UcxEvtRootHubEnableWakeAtBus;
    PdoCallbacks.EvtDeviceDisableWakeAtBus = UcxEvtRootHubDisableWakeAtBus;
    WdfPdoInitSetEventCallbacks(Init, &PdoCallbacks);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, UcxRootHubPdoContext);

    /* USBPDO-n is shared with the PDOs the hub creates; take the first free one */
    for (Index = 0;; Index++)
    {
        Status = RtlStringCchPrintfW(NameBuffer, ARRAYSIZE(NameBuffer), L"\\Device\\USBPDO-%d", Index);
        if (!NT_SUCCESS(Status))
            break;

        RtlInitUnicodeString(&Name, NameBuffer);
        Status = WdfDeviceInitAssignName(Init, &Name);
        if (!NT_SUCCESS(Status))
            break;

        Status = WdfDeviceInitAssignSDDLString(Init, &UcxRootHubPdoSddl);
        if (!NT_SUCCESS(Status))
            break;

        Status = WdfDeviceCreate(&Init, &Attributes, &Pdo);
        if (Status != STATUS_OBJECT_NAME_COLLISION)
            break;
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Root hub PDO create failed at USBPDO-%lu 0x%lx\n", Index, Status);
        if (Init != NULL)
            WdfDeviceInitFree(Init);
        return Status;
    }

    DPRINT("Root hub PDO %p is %wZ\n", Pdo, &Name);

    /* User mode IOCTLs on the root hub PDO are not served */
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&QueueConfig, WdfIoQueueDispatchParallel);
    QueueConfig.EvtIoDeviceControl = UcxEvtRootHubPdoIoDeviceControl;
    Status = WdfIoQueueCreate(Pdo, &QueueConfig, WDF_NO_OBJECT_ATTRIBUTES, WDF_NO_HANDLE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Root hub PDO %p queue create failed 0x%lx\n", Pdo, Status);
        WdfObjectDelete(Pdo);
        return Status;
    }

    WdfDeviceSetSpecialFileSupport(Pdo, WdfSpecialFilePaging, TRUE);
    WdfDeviceSetSpecialFileSupport(Pdo, WdfSpecialFileHibernation, TRUE);
    WdfDeviceSetSpecialFileSupport(Pdo, WdfSpecialFileDump, TRUE);
    WdfDeviceSetSpecialFileSupport(Pdo, WdfSpecialFileBoot, TRUE);

    WDF_DEVICE_PNP_CAPABILITIES_INIT(&m_PnpCaps);
    m_PnpCaps.Removable = WdfFalse;
    m_PnpCaps.Address = 0;
    m_PnpCaps.UniqueID = WdfFalse;
    WdfDeviceSetPnpCapabilities(Pdo, &m_PnpCaps);

    BuildPowerCapabilities();
    WdfDeviceSetPowerCapabilities(Pdo, &m_PowerCaps);

    PdoContext = UcxGetRootHubPdoContext(Pdo);
    PdoContext->RootHub = this;
    PdoContext->Controller = m_Controller;
    m_SystemPowerAction = PowerActionNone;

    Status = WdfDeviceCreateDeviceInterface(Pdo, &GUID_DEVINTERFACE_USB_HUB, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Root hub PDO %p hub interface create failed 0x%lx\n", Pdo, Status);
        WdfObjectDelete(Pdo);
        return Status;
    }

    {
        SpinLockGuard Guard(&m_PdoInfoLock);

        m_Pdo = Pdo;
    }

    Status = WdfFdoAddStaticChild(Fdo, Pdo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Root hub PDO %p static child add failed 0x%lx\n", Pdo, Status);
        m_Pdo = NULL;
        WdfObjectDelete(Pdo);
        return Status;
    }

    /* From here the PDO belongs to the FDO and is not deleted on failure */
    return AddQueryInterfaces();
}

/* PCI device properties the hub reads from the root hub PDO to map root ports to USB4 hosts */
static const DEVPROPKEY UcxPciKeySerialNumber =
    { { 0x3ab22e31, 0x8264, 0x4b4e, { 0x9a, 0xf5, 0xa8, 0xd2, 0xd8, 0xe3, 0x3e, 0x62 } }, 40 };
static const DEVPROPKEY UcxPciKeyUsb4PortAttributes =
    { { 0x3ab22e31, 0x8264, 0x4b4e, { 0x9a, 0xf5, 0xa8, 0xd2, 0xd8, 0xe3, 0x3e, 0x62 } }, 42 };
static const DEVPROPKEY UcxPciKeyParentSerialNumber =
    { { 0x3ab22e31, 0x8264, 0x4b4e, { 0x9a, 0xf5, 0xa8, 0xd2, 0xd8, 0xe3, 0x3e, 0x62 } }, 45 };

/** Nothing is found unless the PCI bus driver publishes the properties. */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
UcxRootHubCopyPciProperties(
    _In_ WDFDEVICE Controller,
    _In_ WDFDEVICE Pdo)
{
    WDF_DEVICE_PROPERTY_DATA Property;
    WDFMEMORY Attributes = NULL;
    PVOID AttributesData = NULL;
    size_t AttributesSize = 0;
    DEVPROPTYPE AttributesType = DEVPROP_TYPE_UINT32;
    DEVPROPTYPE SerialType = DEVPROP_TYPE_EMPTY;
    ULONG64 Serial = 0;
    ULONG Required = 0;
    BOOLEAN Found;
    NTSTATUS Status;

    WDF_DEVICE_PROPERTY_DATA_INIT(&Property, &UcxPciKeySerialNumber);
    Status = WdfDeviceQueryPropertyEx(Controller, &Property, sizeof(Serial), &Serial, &Required, &SerialType);
    if (!NT_SUCCESS(Status))
    {
        WDF_DEVICE_PROPERTY_DATA_INIT(&Property, &UcxPciKeyParentSerialNumber);
        Status = WdfDeviceQueryPropertyEx(Controller, &Property, sizeof(Serial), &Serial, &Required, &SerialType);
    }

    Found = NT_SUCCESS(Status);
    if (Found)
    {
        WDF_DEVICE_PROPERTY_DATA_INIT(&Property, &UcxPciKeyUsb4PortAttributes);
        Status = WdfDeviceAllocAndQueryPropertyEx(Controller,
                                                  &Property,
                                                  PagedPool,
                                                  WDF_NO_OBJECT_ATTRIBUTES,
                                                  &Attributes,
                                                  &AttributesType);
        if (NT_SUCCESS(Status))
        {
            AttributesData = WdfMemoryGetBuffer(Attributes, &AttributesSize);
        }
        else
        {
            /* QUIRK: the serial number is dropped as well */
            DPRINT1("Controller %p USB4 port attributes query failed 0x%lx\n", Controller, Status);
            Attributes = NULL;
            Found = FALSE;
        }
    }

    WDF_DEVICE_PROPERTY_DATA_INIT(&Property, &UcxPciKeyUsb4PortAttributes);
    Status = WdfDeviceAssignProperty(Pdo, &Property, AttributesType, (ULONG)AttributesSize, AttributesData);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Root hub PDO %p USB4 port attributes assign failed 0x%lx\n", Pdo, Status);
    }
    else
    {
        /* QUIRK: a parent serial number is published under the serial number key */
        WDF_DEVICE_PROPERTY_DATA_INIT(&Property, &UcxPciKeySerialNumber);
        Status = WdfDeviceAssignProperty(Pdo,
                                         &Property,
                                         SerialType,
                                         Found ? sizeof(Serial) : 0,
                                         Found ? &Serial : NULL);
        if (!NT_SUCCESS(Status))
            DPRINT1("Root hub PDO %p serial number assign failed 0x%lx\n", Pdo, Status);
    }

    if (Attributes != NULL)
        WdfObjectDelete(Attributes);
}

/* PnP and power */

NTSTATUS
NTAPI
UcxEvtRootHubPrepareHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesRaw,
    _In_ WDFCMRESLIST ResourcesTranslated)
{
    UcxRootHubPdoContext* Context = UcxGetRootHubPdoContext(Device);
    UcxRootHub* RootHub = Context->RootHub;
    WDF_OBJECT_ATTRIBUTES Attributes;
    WDFSTRING Name;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(ResourcesRaw);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Device;

    Status = WdfStringCreate(NULL, &Attributes, &Name);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Root hub PDO %p interface string create failed 0x%lx\n", Device, Status);
        return Status;
    }

    Status = WdfDeviceRetrieveDeviceInterfaceString(Device, &GUID_DEVINTERFACE_USB_HUB, NULL, Name);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Root hub PDO %p hub interface name query failed 0x%lx\n", Device, Status);
        WdfObjectDelete(Name);
        return Status;
    }

    /* Controller errata are known by now */
    Context->Controller->m_ClearTtBufferOnAsyncCancel = Context->Controller->QueryClearTtBufferOnCancel();

    {
        SpinLockGuard Guard(&RootHub->m_PdoInfoLock);

        RootHub->m_PdoStarted = TRUE;
        RootHub->m_SymbolicName = Name;
    }

    UcxRootHubCopyPciProperties(Context->Controller->m_Fdo, Device);

    /* Unknown until the first S IRP, as USBPORT reports it */
    RootHub->m_LastSystemSleepState = PowerSystemUnspecified;
    DPRINT("Root hub PDO %p prepared\n", Device);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
UcxEvtRootHubReleaseHardware(
    _In_ WDFDEVICE Device,
    _In_ WDFCMRESLIST ResourcesTranslated)
{
    UcxRootHub* RootHub = UcxGetRootHubPdoContext(Device)->RootHub;
    WDFSTRING Name;

    UNREFERENCED_PARAMETER(ResourcesTranslated);

    {
        SpinLockGuard Guard(&RootHub->m_PdoInfoLock);

        RootHub->m_PdoStarted = FALSE;
        Name = RootHub->m_SymbolicName;
        RootHub->m_SymbolicName = NULL;
    }

    if (Name != NULL)
        WdfObjectDelete(Name);

    DPRINT("Root hub PDO %p released\n", Device);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
UcxEvtRootHubD0Entry(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE PreviousState)
{
    UcxRootHubPdoContext* Context = UcxGetRootHubPdoContext(Device);
    UcxInterruptQueueState* State = Context->RootHub->m_InterruptState;

    UNREFERENCED_PARAMETER(PreviousState);

    DPRINT("Root hub PDO %p D0 entry, previous WDF state %d\n", Device, PreviousState);

    {
        SpinLockGuard Guard(&State->PortChangeLock);

        State->IndicateWakeEnabled = FALSE;
    }

    /* A port change may still be inside WdfDeviceIndicateWakeStatus */
    KeWaitForSingleObject(&State->NoIndicateWakeInProgress, Executive, KernelMode, FALSE, NULL);

    Context->Controller->PostResetEvent(CrEvent::RootHubPoweredUp);
    Context->Controller->m_RootHubInD0 = TRUE;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
UcxEvtRootHubD0Exit(
    _In_ WDFDEVICE Device,
    _In_ WDF_POWER_DEVICE_STATE TargetState)
{
    UcxRootHubPdoContext* Context = UcxGetRootHubPdoContext(Device);
    UcxController* Controller = Context->Controller;
    UcxRootHub* RootHub = Context->RootHub;
    UcxInterruptQueueState* State = RootHub->m_InterruptState;
    BOOLEAN WakeNow = FALSE;

    UNREFERENCED_PARAMETER(TargetState);

    DPRINT("Root hub PDO %p D0 exit, target WDF state %d\n", Device, TargetState);

    /* Waits out any reset recovery that still needs the root hub */
    Controller->PostResetEvent(CrEvent::RootHubPoweringDown);
    KeWaitForSingleObject(&Controller->m_RootHubMayExitD0, Executive, KernelMode, FALSE, NULL);
    Controller->m_RootHubInD0 = FALSE;

    {
        SpinLockGuard Guard(&State->PortChangeLock);

        /* The hub may have dropped its last transfer; make the next one reach the HCD */
        if (!State->LastTransferCanceled)
            State->PortChangeGeneration++;

        /* S0 idle: changes from now on must wake the root hub */
        if (RootHub->m_SystemPowerAction == PowerActionNone)
        {
            State->IndicateWakeEnabled = TRUE;
            WakeNow = (State->PortChangeGeneration != State->PortChangeGenerationProcessed);
        }
    }

    if (WakeNow)
        WdfDeviceIndicateWakeStatus(Device, STATUS_SUCCESS);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
UcxEvtRootHubEnableWakeAtBus(
    _In_ WDFDEVICE Device,
    _In_ SYSTEM_POWER_STATE PowerState)
{
    UNREFERENCED_PARAMETER(PowerState);

    if (UcxGetRootHubPdoContext(Device)->Controller->IsResetInProgress())
    {
        DPRINT1("Root hub PDO %p wake arm failed, controller reset in progress\n", Device);
        return STATUS_NO_SUCH_DEVICE;
    }

    DPRINT("Root hub PDO %p wake armed\n", Device);
    return STATUS_SUCCESS;
}

VOID
NTAPI
UcxEvtRootHubDisableWakeAtBus(
    _In_ WDFDEVICE Device)
{
    UNREFERENCED_PARAMETER(Device);
}

VOID
NTAPI
UcxEvtRootHubPdoIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode)
{
    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);
    DPRINT1("Root hub PDO user IOCTL 0x%lx not supported\n", IoControlCode);
    WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
}

/* Internal IOCTL routing */

enum class UcxStackSetup
{
    Skip,
    DeviceMgmtCompletion,
    GetInfoCompletion
};

/** The controller queue a hub IOCTL goes to, or NULL when the root hub does not serve it. */
static
WDFQUEUE
NTAPI
UcxRouteHubIoctl(
    _In_ UcxController* Controller,
    _In_ ULONG IoControlCode,
    _Out_ UcxStackSetup* Setup)
{
    *Setup = UcxStackSetup::Skip;

    switch (IoControlCode)
    {
        case IOCTL_UCXHUB_ENDPOINTS_CONFIGURE:
        case IOCTL_UCXHUB_DEVICE_ENABLE:
        case IOCTL_UCXHUB_DEVICE_RESET:
        case IOCTL_UCXHUB_ENDPOINT_RESET:
            *Setup = UcxStackSetup::DeviceMgmtCompletion;
            return Controller->m_DeviceMgmtQueue;

        case IOCTL_UCXHUB_DEVICE_ADDRESS:
        case IOCTL_UCXHUB_DEVICE_UPDATE:
        case IOCTL_UCXHUB_DEVICE_HUB_INFO:
        case IOCTL_UCXHUB_DEVICE_DISABLE:
        case IOCTL_UCXHUB_DEVICE_PURGE_IO:
        case IOCTL_UCXHUB_DEVICE_START_IO:
        case IOCTL_UCXHUB_DEFAULT_ENDPOINT_UPDATE:
        case IOCTL_UCXHUB_DEVICE_ABORT_IO:
            return Controller->m_DeviceMgmtQueue;

        case IOCTL_UCXHUB_DEVICE_TREE_PURGE_IO:
            return Controller->m_TreePurgeQueue;

        case IOCTL_UCXHUB_ADDRESS0_OWNERSHIP_ACQUIRE:
            return Controller->m_Address0Queue;

        /* The port info IOCTLs get the completion routine only so HCD failures print */
        case IOCTL_UCXHUB_ROOTHUB_GET_INFO:
        case IOCTL_UCXHUB_ROOTHUB_GET_20PORT_INFO:
        case IOCTL_UCXHUB_ROOTHUB_GET_30PORT_INFO:
            *Setup = UcxStackSetup::GetInfoCompletion;
            return Controller->m_DefaultQueue;

        case IOCTL_INTERNAL_USB_REGISTER_COMPOSITE_DEVICE:
        case IOCTL_INTERNAL_USB_UNREGISTER_COMPOSITE_DEVICE:
        case IOCTL_INTERNAL_USB_REQUEST_REMOTE_WAKE_NOTIFICATION:
        case IOCTL_UCXHUB_QUERY_USB_CAPABILITY:
        case IOCTL_UCXHUB_SET_FUNCTION_HANDLE_DATA:
        case IOCTL_UCXHUB_GET_DUMP_DATA:
        case IOCTL_UCXHUB_FREE_DUMP_DATA:
        case IOCTL_UCXHUB_NOTIFY_FORWARD_PROGRESS:
            return Controller->m_DefaultQueue;

        default:
            return NULL;
    }
}

static
NTSTATUS
NTAPI
UcxCompleteIrpWithStatus(
    _In_ PIRP Irp,
    _In_ NTSTATUS Status)
{
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

/** Keeps ucx01000 loaded until the routine ran when the system allows it. */
static
VOID
NTAPI
UcxSetCompletionRoutine(
    _In_ WDFDEVICE Device,
    _In_ PIRP Irp,
    _In_ PIO_COMPLETION_ROUTINE Routine,
    _In_ PVOID Context)
{
    NTSTATUS Status;

    Status = IoSetCompletionRoutineEx(WdfDeviceWdmGetDeviceObject(Device),
                                      Irp,
                                      Routine,
                                      Context,
                                      TRUE,
                                      TRUE,
                                      TRUE);
    if (!NT_SUCCESS(Status))
        IoSetCompletionRoutine(Irp, Routine, Context, TRUE, TRUE, TRUE);
}

static
NTSTATUS
NTAPI
UcxQueueAsyncPortReset(
    _In_ UcxController* Controller,
    _In_ PIRP Irp)
{
    if (Controller->m_Config.EvtControllerReset == NULL)
    {
        DPRINT1("Controller %p has no reset callback, failing async port reset\n", Controller);
        return UcxCompleteIrpWithStatus(Irp, STATUS_NOT_SUPPORTED);
    }

    if (Controller->HasFailed())
    {
        DPRINT1("Controller %p has failed, refusing async port reset\n", Controller);
        return UcxCompleteIrpWithStatus(Irp, STATUS_UNSUCCESSFUL);
    }

    DPRINT("Controller %p async port reset requested by hub\n", Controller);

    IoMarkIrpPending(Irp);

    NT_ASSERT(Controller->m_RootHub->m_PendingAsyncReset == NULL);
    Controller->m_RootHub->m_PendingAsyncReset = Irp;

    Controller->PostResetEvent(CrEvent::HubRequestsReset);
    return STATUS_PENDING;
}

NTSTATUS
NTAPI
UcxEvtRootHubPreprocessInternalIoctl(
    _In_ WDFDEVICE Device,
    _Inout_ PIRP Irp)
{
    UcxController* Controller = UcxGetRootHubPdoContext(Device)->Controller;
    ULONG Code = IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.IoControlCode;
    UcxStackSetup Setup;
    WDFQUEUE Queue;

    if (Code == IOCTL_INTERNAL_USB_SUBMIT_URB)
        return UcxProcessSubmitUrb(Device, Irp);

    if (Code == IOCTL_UCXHUB_RESET_PORT_ASYNC)
        return UcxQueueAsyncPortReset(Controller, Irp);

    Queue = UcxRouteHubIoctl(Controller, Code, &Setup);
    if (Queue == NULL)
    {
        DPRINT1("Root hub PDO internal IOCTL 0x%lx not supported, IRP %p\n", Code, Irp);
        return UcxCompleteIrpWithStatus(Irp, STATUS_NOT_SUPPORTED);
    }

    switch (Setup)
    {
        case UcxStackSetup::DeviceMgmtCompletion:
            IoCopyCurrentIrpStackLocationToNext(Irp);
            UcxSetCompletionRoutine(Device, Irp, UcxDeviceMgmtCompletion, Device);
            break;

        case UcxStackSetup::GetInfoCompletion:
            IoCopyCurrentIrpStackLocationToNext(Irp);
            UcxSetCompletionRoutine(Device, Irp, UcxRootHubGetInfoCompletion, Device);
            break;

        default:
            IoSkipCurrentIrpStackLocation(Irp);
            break;
    }

    return WdfDeviceWdmDispatchIrpToIoQueue(Device, Irp, Queue, WDF_DISPATCH_IRP_TO_IO_QUEUE_PREPROCESSED_IRP);
}

/* KMDF cannot version one interface GUID by size, so the requested size and version go along */
NTSTATUS
NTAPI
UcxEvtRootHubPreprocessQueryInterface(
    _In_ WDFDEVICE Device,
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    const GUID* Type = Stack->Parameters.QueryInterface.InterfaceType;

    if (IsEqualGUID(*Type, USB_BUS_INTERFACE_USBDI_GUID) ||
        IsEqualGUID(*Type, GUID_UCXHUB_PARENT_INTERFACE) ||
        IsEqualGUID(*Type, GUID_UCXHUB_STACK_INTERFACE))
    {
        Stack->Parameters.QueryInterface.Interface->Size = Stack->Parameters.QueryInterface.Size;
        Stack->Parameters.QueryInterface.Interface->Version = Stack->Parameters.QueryInterface.Version;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    return WdfDeviceWdmDispatchPreprocessedIrp(Device, Irp);
}

/* Track the S IRPs since the D IRP reports a stale power action */
NTSTATUS
NTAPI
UcxEvtRootHubPreprocessSetPower(
    _In_ WDFDEVICE Device,
    _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    UcxRootHub* RootHub = UcxGetRootHubPdoContext(Device)->RootHub;
    SYSTEM_POWER_STATE Target;

    if (Stack->Parameters.Power.Type == SystemPowerState)
    {
        Target = (SYSTEM_POWER_STATE)Stack->Parameters.Power.SystemPowerStateContext.TargetSystemState;

        if (Target != PowerSystemWorking)
        {
            RootHub->m_LastSystemSleepState = Target;
            RootHub->m_SystemPowerAction = Stack->Parameters.Power.ShutdownType;
        }
        else
        {
            RootHub->m_SystemPowerAction = PowerActionNone;
        }
    }

    IoSkipCurrentIrpStackLocation(Irp);
    return WdfDeviceWdmDispatchPreprocessedIrp(Device, Irp);
}

/* Completion routines */

static
NTSTATUS
NTAPI
UcxDeviceMgmtCompletion(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    PVOID Payload = Stack->Parameters.Others.Argument1;
    UcxUsbDevice* Device;

    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Context);

    if (Irp->PendingReturned)
        IoMarkIrpPending(Irp);

    switch (Stack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_UCXHUB_ENDPOINTS_CONFIGURE:
            Device = UcxUsbDevice::FromHandle(((PENDPOINTS_CONFIGURE)Payload)->Header.UsbDevice);
            return Device->OnHcdEndpointsConfigureDone(Irp, (PENDPOINTS_CONFIGURE)Payload);

        case IOCTL_UCXHUB_DEVICE_ENABLE:
            Device = UcxUsbDevice::FromHandle(((PUSBDEVICE_ENABLE)Payload)->Header.UsbDevice);
            return Device->OnHcdEnableDone(Irp, (PUSBDEVICE_ENABLE)Payload);

        case IOCTL_UCXHUB_DEVICE_RESET:
            Device = UcxUsbDevice::FromHandle(((PUSBDEVICE_RESET)Payload)->Header.UsbDevice);
            if (Device->m_ResetFailedByControllerReset)
            {
                Device->m_ResetFailedByControllerReset = FALSE;
                return STATUS_CONTINUE_COMPLETION;
            }
            return Device->OnHcdResetDone(Irp, (PUSBDEVICE_RESET)Payload);

        case IOCTL_UCXHUB_ENDPOINT_RESET:
            Device = UcxUsbDevice::FromHandle(((PENDPOINT_RESET)Payload)->Header.UsbDevice);
            if (Device->m_EndpointResetFailedByControllerReset)
            {
                Device->m_EndpointResetFailedByControllerReset = FALSE;
                return STATUS_CONTINUE_COMPLETION;
            }
            return UcxEndpoint::FromHandle(((PENDPOINT_RESET)Payload)->Endpoint)->OnHcdResetDone(Irp);

        default:
            return STATUS_CONTINUE_COMPLETION;
    }
}

/* Gives the hub its full structure size back and remembers the port counts */
static
NTSTATUS
NTAPI
UcxRootHubGetInfoCompletion(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context)
{
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
    ULONG Code = Stack->Parameters.DeviceIoControl.IoControlCode;
    PUCXHUB_ROOTHUB_INFO Info;
    UcxRootHub* RootHub;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (Irp->PendingReturned)
        IoMarkIrpPending(Irp);

    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        DPRINT1("Root hub PDO %p IOCTL 0x%lx failed by HCD 0x%lx\n",
                Context, Code, Irp->IoStatus.Status);
        return STATUS_CONTINUE_COMPLETION;
    }

    if (Code != IOCTL_UCXHUB_ROOTHUB_GET_INFO)
        return STATUS_CONTINUE_COMPLETION;

    Info = (PUCXHUB_ROOTHUB_INFO)Stack->Parameters.Others.Argument1;
    RootHub = UcxGetRootHubPdoContext((WDFDEVICE)Context)->RootHub;

    Info->Info.Size = sizeof(*Info);
    RootHub->m_NumberOf20Ports = Info->Info.NumberOf20Ports;
    RootHub->m_NumberOf30Ports = Info->Info.NumberOf30Ports;

    DPRINT("Root hub %p has %lu USB 2 and %lu USB 3 ports\n",
           RootHub, (ULONG)Info->Info.NumberOf20Ports, (ULONG)Info->Info.NumberOf30Ports);

    return STATUS_CONTINUE_COMPLETION;
}
