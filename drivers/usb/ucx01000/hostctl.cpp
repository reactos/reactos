/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     UCXCONTROLLER creation, exports and lifetime
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ucx01000.h"

#define NDEBUG
#include <debug.h>

UcxController*
UcxController::FromHandle(
    _In_ UCXCONTROLLER Handle)
{
    return UcxGetControllerContext(Handle);
}

UcxController*
UcxController::FromFdo(
    _In_ WDFDEVICE Fdo)
{
    return UcxGetFdoContext(Fdo)->Controller;
}

/** Config sizes the HCD may pass, one per UCX_CONTROLLER_CONFIG generation. */
static
BOOLEAN
NTAPI
UcxIsAcceptedConfigSize(
    _In_ ULONG Size)
{
    return Size == UCX_CONFIG_SIZE_V1 ||
           Size == UCX_CONFIG_SIZE_V3 ||
           Size == UCX_CONFIG_SIZE_V4;
}

_Must_inspect_result_
NTSTATUS
UcxController::Create(
    _In_ PUCX_DRIVER_GLOBALS Globals,
    _In_ WDFDEVICE Device,
    _In_ PUCX_CONTROLLER_CONFIG Config,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXCONTROLLER* Controller)
{
    UCX_CONTROLLER_CONFIG FullConfig;
    WDF_OBJECT_ATTRIBUTES FdoAttributes;
    WDF_OBJECT_ATTRIBUTES UcxAttributes;
    PWDF_OBJECT_ATTRIBUTES Primary;
    UcxFdoContext* FdoContext;
    UcxController* Context;
    WDFOBJECT Object;
    NTSTATUS Status;

    PAGED_CODE();

    if (Controller == NULL || Config == NULL || !UcxIsAcceptedConfigSize(Config->Size))
    {
        DPRINT1("Controller create rejected, config %p size %lu\n", Config, Config != NULL ? Config->Size : 0);
        return STATUS_INVALID_PARAMETER;
    }

    /* The two dump callbacks come as a pair */
    if ((Config->Reserved3 == NULL) != (Config->Reserved4 == NULL))
    {
        DPRINT1("Controller create rejected, only one dump callback given\n");
        return STATUS_INVALID_PARAMETER;
    }

    if (Attributes != NULL && Attributes->ParentObject != NULL)
    {
        DPRINT1("Controller create rejected, HCD attributes name a parent\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Older configs are upgraded: fields the HCD did not know about stay zero */
    UCX_CONTROLLER_CONFIG_INIT(&FullConfig, "");
    RtlCopyMemory(&FullConfig, Config, Config->Size);
    FullConfig.Size = sizeof(FullConfig);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&FdoAttributes, UcxFdoContext);
    Status = WdfObjectAllocateContext(Device, &FdoAttributes, (PVOID*)&FdoContext);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Controller FDO %p context allocation failed 0x%lx\n", Device, Status);
        return Status;
    }

    *Controller = NULL;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&UcxAttributes, UcxController);
    UcxAttributes.EvtDestroyCallback = UcxController::EvtDestroy;

    /* The HCD's attributes come first when given; parent and level are UCX's choice */
    Primary = (Attributes != NULL) ? Attributes : &UcxAttributes;
    Primary->ParentObject = Device;
    Primary->ExecutionLevel = WdfExecutionLevelPassive;

    Status = UcxCreateObjectWithTwoContexts(Primary,
                                            (Attributes != NULL) ? &UcxAttributes : NULL,
                                            &Object);
    if (!NT_SUCCESS(Status))
        return Status;

    Context = new (UcxGetControllerContext(Object)) UcxController();
    Context->m_Handle = (UCXCONTROLLER)Object;
    FdoContext->Controller = Context;

    Status = Context->Initialize(Globals, Device, &FullConfig);
    if (!NT_SUCCESS(Status))
    {
        WdfObjectDelete(Object);
        return Status;
    }

    *Controller = Context->m_Handle;
    DPRINT("Controller %p created on FDO %p, bus type %d\n", Context, Device, FullConfig.ParentBusType);
    return STATUS_SUCCESS;
}

NTSTATUS
UcxController::Initialize(
    _In_ PUCX_DRIVER_GLOBALS Globals,
    _In_ WDFDEVICE Fdo,
    _In_ const UCX_CONTROLLER_CONFIG* Config)
{
    PDEVICE_OBJECT FdoObject;
    NTSTATUS Status;

    m_Fdo = Fdo;
    m_Config = *Config;
    UcxClearListEntry(&m_DriverListEntry);

    KeInitializeSpinLock(&m_TopologyLock);
    m_ChildDeviceCount = 0;
    m_ChildEndpointCount = 0;

    FdoObject = WdfDeviceWdmGetDeviceObject(Fdo);

    m_ResetMachineWorkItem = UcxWorkItem::Allocate(this, FdoObject, UCXHUB_WORKITEM_NO_FLAGS);
    if (m_ResetMachineWorkItem == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    m_ResetMachine.Initialize(this);
    m_ResetMachineReady = TRUE;

    KeInitializeEvent(&m_RootHubMayExitD0, SynchronizationEvent, FALSE);
    KeInitializeEvent(&m_ResetCompleteProcessed, SynchronizationEvent, FALSE);

    KeInitializeSpinLock(&m_ResetLock);
    m_Failed = 0;
    m_ResetBlockCount = 1;
    m_PendingTreePurge = NULL;
    m_PendingTreePurgeEndpoints = 0;

    m_DriverVerifierEnabled = MmIsDriverVerifying(FdoObject->DriverObject) != 0 ||
                              (Globals->WdfDriverGlobals->DriverFlags & WdfVerifyOn) != 0;

    Status = CreateQueues();
    if (!NT_SUCCESS(Status))
        return Status;

    IoCsqInitialize(&m_AbortPipeCsq,
                    UcxCsqInsertAbortIrp,
                    UcxCsqRemoveAbortIrp,
                    UcxCsqPeekAbortIrp,
                    UcxCsqAcquireAbortLock,
                    UcxCsqReleaseAbortLock,
                    UcxCsqCompleteCanceledAbortIrp);
    KeInitializeSpinLock(&m_AbortPipeCsqLock);

    Status = WdfDeviceCreateDeviceInterface(Fdo, &GUID_DEVINTERFACE_USB_HOST_CONTROLLER, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Controller %p host controller interface create failed 0x%lx\n", this, Status);
        return Status;
    }

    {
        WDF_OBJECT_ATTRIBUTES StringAttributes;

        WDF_OBJECT_ATTRIBUTES_INIT(&StringAttributes);
        StringAttributes.ParentObject = m_Handle;

        Status = WdfStringCreate(NULL, &StringAttributes, &m_HostControllerInterfaceName);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Controller %p interface string create failed 0x%lx\n", this, Status);
            return Status;
        }

        Status = WdfDeviceRetrieveDeviceInterfaceString(Fdo,
                                                        &GUID_DEVINTERFACE_USB_HOST_CONTROLLER,
                                                        NULL,
                                                        m_HostControllerInterfaceName);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Controller %p interface name query failed 0x%lx\n", this, Status);
            return Status;
        }

        /* Kept past the parent's cleanup; dropped in Destroy */
        WdfObjectReferenceWithTag(m_HostControllerInterfaceName, (PVOID)UCX_POOL_TAG);
    }

    Status = QueryHcCapabilities();
    if (!NT_SUCCESS(Status))
        return Status;

    Status = RegisterWmi();
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Controller %p WMI registration failed 0x%lx\n", this, Status);
        return Status;
    }

    KeInitializeSpinLock(&m_WorkerLock);
    InitializeListHead(&m_WorkerItems);
    KeInitializeEvent(&m_WorkerWake, NotificationEvent, FALSE);
    m_WorkerParked = TRUE;
    m_WaitOnResetComplete = TRUE;
    m_HcdReservedIoReady = FALSE;
    KeInitializeMutex(&m_ForwardProgressMutex, 0);

    {
        SpinLockGuard Guard(&UcxDriver.ControllerListLock);

        InsertTailList(&UcxDriver.ControllerList, &m_DriverListEntry);
        UcxDriver.ControllerCount++;
    }

    return STATUS_SUCCESS;
}

/** Synchronous IRP_MN_QUERY_CAPABILITIES down the FDO's stack. */
NTSTATUS
UcxController::QueryHcCapabilities()
{
    WDF_REQUEST_SEND_OPTIONS SendOptions;
    WDF_REQUEST_REUSE_PARAMS ReuseParams;
    IO_STACK_LOCATION Stack;
    WDFIOTARGET Target;
    WDFREQUEST Request;
    NTSTATUS Status;

    Target = WdfDeviceGetIoTarget(m_Fdo);

    Status = WdfRequestCreate(WDF_NO_OBJECT_ATTRIBUTES, Target, &Request);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Controller %p capabilities request create failed 0x%lx\n", this, Status);
        return Status;
    }

    /* PnP IRPs must start out as not supported */
    WDF_REQUEST_REUSE_PARAMS_INIT(&ReuseParams, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_NOT_SUPPORTED);
    Status = WdfRequestReuse(Request, &ReuseParams);
    if (NT_SUCCESS(Status))
    {
        RtlZeroMemory(&m_HcCaps, sizeof(m_HcCaps));
        m_HcCaps.Size = sizeof(m_HcCaps);
        m_HcCaps.Version = 1;
        m_HcCaps.Address = MAXULONG;
        m_HcCaps.UINumber = MAXULONG;

        RtlZeroMemory(&Stack, sizeof(Stack));
        Stack.MajorFunction = IRP_MJ_PNP;
        Stack.MinorFunction = IRP_MN_QUERY_CAPABILITIES;
        Stack.Parameters.DeviceCapabilities.Capabilities = &m_HcCaps;
        WdfRequestWdmFormatUsingStackLocation(Request, &Stack);

        /* A send that fails to go out still records its status in the request */
        WDF_REQUEST_SEND_OPTIONS_INIT(&SendOptions, WDF_REQUEST_SEND_OPTION_SYNCHRONOUS);
        WdfRequestSend(Request, Target, &SendOptions);
        Status = WdfRequestGetStatus(Request);
    }

    WdfObjectDelete(Request);

    if (!NT_SUCCESS(Status))
        DPRINT1("Controller %p query capabilities failed 0x%lx\n", this, Status);

    return Status;
}

NTSTATUS
UcxController::RegisterWmi()
{
    WDF_WMI_PROVIDER_CONFIG ProviderConfig;
    WDF_WMI_INSTANCE_CONFIG InstanceConfig;

    WDF_WMI_PROVIDER_CONFIG_INIT(&ProviderConfig, &GUID_USB_WMI_NODE_INFO);
    WDF_WMI_INSTANCE_CONFIG_INIT_PROVIDER_CONFIG(&InstanceConfig, &ProviderConfig);
    InstanceConfig.Register = TRUE;
    InstanceConfig.EvtWmiInstanceQueryInstance = UcxEvtWmiNodeInfoQueryInstance;

    return WdfWmiInstanceCreate(m_Fdo, &InstanceConfig, WDF_NO_OBJECT_ATTRIBUTES, WDF_NO_HANDLE);
}

/** Leaves room for the terminator the copy adds. */
static
BOOLEAN
NTAPI
UcxIsValidIdString(
    _In_opt_ PCUNICODE_STRING String)
{
    if (String == NULL || (String->Length % sizeof(WCHAR)) != 0)
        return FALSE;

    if (String->Length > MAXUSHORT - sizeof(UNICODE_NULL))
        return FALSE;

    return String->Length == 0 || String->Buffer != NULL;
}

/** NUL terminated copy owned by the controller. */
static
NTSTATUS
NTAPI
UcxCopyIdString(
    _In_ PCUNICODE_STRING Source,
    _Out_ PUNICODE_STRING Copy)
{
    USHORT Size = Source->Length + sizeof(UNICODE_NULL);

    Copy->Buffer = (PWCH)ExAllocatePoolWithTag(NonPagedPool, Size, UCX_POOL_TAG);
    if (Copy->Buffer == NULL)
    {
        Copy->Length = 0;
        Copy->MaximumLength = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(Copy->Buffer, Source->Buffer, Source->Length);
    Copy->Buffer[Source->Length / sizeof(WCHAR)] = UNICODE_NULL;
    Copy->Length = Source->Length;
    Copy->MaximumLength = Size;

    return STATUS_SUCCESS;
}

static
VOID
NTAPI
UcxFreeIdString(
    _Inout_ PUNICODE_STRING String)
{
    if (String->Buffer != NULL)
        ExFreePoolWithTag(String->Buffer, UCX_POOL_TAG);

    RtlZeroMemory(String, sizeof(*String));
}

VOID
UcxController::EvtDestroy(
    _In_ WDFOBJECT Object)
{
    UcxGetControllerContext(Object)->Destroy();
}

VOID
UcxController::Destroy()
{
    if (m_WorkerThread != NULL)
    {
        {
            SpinLockGuard Guard(&m_WorkerLock);

            m_WorkerExit = TRUE;
            KeSetEvent(&m_WorkerWake, IO_NO_INCREMENT, FALSE);
        }

        KeWaitForSingleObject(m_WorkerThread, Executive, KernelMode, FALSE, NULL);
        ObDereferenceObject(m_WorkerThread);
        m_WorkerThread = NULL;
    }

    if (m_ResetMachineWorkItem != NULL)
    {
        m_ResetMachineWorkItem->Free();
        m_ResetMachineWorkItem = NULL;
    }
    m_ResetMachineReady = FALSE;

    NT_ASSERT(m_UsbdInterfaceCount == 0);

    /* A create that failed early never linked the entry */
    {
        SpinLockGuard Guard(&UcxDriver.ControllerListLock);

        if (UcxIsListEntryLinked(&m_DriverListEntry))
        {
            RemoveEntryList(&m_DriverListEntry);
            UcxDriver.ControllerCount--;
        }
        UcxClearListEntry(&m_DriverListEntry);
    }

    if (m_HostControllerInterfaceName != NULL)
    {
        WdfObjectDereferenceWithTag(m_HostControllerInterfaceName, (PVOID)UCX_POOL_TAG);
        m_HostControllerInterfaceName = NULL;
    }

    UcxFreeIdString(&m_ManufacturerName);
    UcxFreeIdString(&m_ModelName);
    UcxFreeIdString(&m_ModelNumber);
}

/* Exports */

VOID
UcxController::NeedsReset()
{
    DPRINT1("Controller %p reports it needs a reset\n", this);
    PostResetEvent(CrEvent::ControllerNeedsReset);
}

VOID
UcxController::ResetComplete(
    _In_ PUCX_CONTROLLER_RESET_COMPLETE_INFO Info)
{
    if (Info->UcxControllerState == UcxControllerStateLost)
        m_DeviceContextsLost = 1;

    m_RootHubResetSeen = 1;

    DPRINT("Controller %p reset complete, state %d\n", this, Info->UcxControllerState);
    PostResetEvent(CrEvent::ControllerResetDone);

    /* While the root hub is out of D0 the machine releases this thread when done */
    if (m_WaitOnResetComplete)
        KeWaitForSingleObject(&m_ResetCompleteProcessed, Executive, KernelMode, FALSE, NULL);
}

VOID
UcxController::SetFailed()
{
    if (InterlockedCompareExchange(&m_Failed, 1, 0) != 0)
        return;

    DPRINT1("Controller %p marked failed by the HCD\n", this);

    /* Tear the tree down first so the purges recurse on one stack */
    m_RootHub->Device()->DisconnectPort(0);

    PostResetEvent(CrEvent::ControllerLost);
}

NTSTATUS
UcxController::SetIdStrings(
    _In_ PUNICODE_STRING Manufacturer,
    _In_ PUNICODE_STRING ModelName,
    _In_ PUNICODE_STRING ModelNumber)
{
    NTSTATUS Status;

    /* Only a non empty manufacturer counts as already set */
    if (m_ManufacturerName.Length != 0)
    {
        DPRINT1("Controller %p ID strings already set\n", this);
        return STATUS_INVALID_PARAMETER;
    }

    if (!UcxIsValidIdString(Manufacturer) || !UcxIsValidIdString(ModelName) || !UcxIsValidIdString(ModelNumber))
    {
        DPRINT1("Controller %p ID strings malformed\n", this);
        return STATUS_INVALID_PARAMETER;
    }

    /* An earlier call with an empty manufacturer may have left copies behind */
    UcxFreeIdString(&m_ManufacturerName);
    UcxFreeIdString(&m_ModelName);
    UcxFreeIdString(&m_ModelNumber);

    Status = UcxCopyIdString(Manufacturer, &m_ManufacturerName);
    if (NT_SUCCESS(Status))
        Status = UcxCopyIdString(ModelName, &m_ModelName);
    if (NT_SUCCESS(Status))
        Status = UcxCopyIdString(ModelNumber, &m_ModelNumber);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Controller %p ID string copy failed 0x%lx\n", this, Status);
        UcxFreeIdString(&m_ManufacturerName);
        UcxFreeIdString(&m_ModelName);
        UcxFreeIdString(&m_ModelNumber);
        return Status;
    }

    return STATUS_SUCCESS;
}

/* No hub IOCTL pends for these notifications yet, so there is nobody to tell */
VOID
UcxController::NotifyTransportCharacteristicsChange(
    _In_ PUCX_CONTROLLER_TRANSPORT_CHARACTERISTICS Characteristics)
{
    UNREFERENCED_PARAMETER(Characteristics);
}

/* Reset references */

BOOLEAN
UcxController::BlockReset()
{
    InterlockedIncrement(&m_ResetBlockCount);

    if (m_ResetInProgress)
    {
        UnblockReset();
        return FALSE;
    }

    return TRUE;
}

/** Drops a reset reference; the last one during a reset tells the machine once. */
VOID
UcxController::UnblockReset()
{
    ULONG Generation = m_ResetGeneration;

    if (InterlockedDecrement(&m_ResetBlockCount) != 0)
        return;

    {
        SpinLockGuard Guard(&m_ResetLock);

        if (m_ResetInitiated || Generation != m_ResetGeneration)
            return;

        m_ResetInitiated = TRUE;
    }

    PostResetEvent(CrEvent::ResetBlocksCleared);
}

VOID
UcxController::MarkResetInProgress()
{
    SpinLockGuard Guard(&m_ResetLock);

    NT_ASSERT(!m_ResetInProgress && !m_ResetInitiated);
    m_ResetInProgress = TRUE;
}

VOID
UcxController::MarkResetFinished()
{
    SpinLockGuard Guard(&m_ResetLock);

    m_ResetGeneration++;
    m_ResetInProgress = FALSE;
    m_ResetInitiated = FALSE;

    /* The controller's own reference comes back */
    InterlockedIncrement(&m_ResetBlockCount);
}

NTSTATUS
UcxController::GetCurrentFrameNumber(
    _Out_ PULONG FrameNumber)
{
    NTSTATUS Status;
    ULONG Frame;

    if (m_Config.EvtControllerGetCurrentFrameNumber == NULL)
    {
        DPRINT1("Controller %p has no frame number callback\n", this);
        NT_ASSERT(FALSE);
        return STATUS_NOT_SUPPORTED;
    }

    if (!BlockReset())
    {
        *FrameNumber = m_CachedFrameNumber;
        return STATUS_SUCCESS;
    }

    Frame = m_CachedFrameNumber;
    Status = m_Config.EvtControllerGetCurrentFrameNumber(m_Handle, &Frame);
    UnblockReset();

    if (!NT_SUCCESS(Status))
    {
        /* The caller still gets whatever the HCD left in the buffer */
        DPRINT1("Controller %p frame number query failed 0x%lx\n", this, Status);
    }
    else if (Frame == MAXULONG)
    {
        /* All ones means the HCD is not in D0; report the cached frame */
        DPRINT("Controller %p has no frame number, reporting cached frame %lu\n", this, m_CachedFrameNumber);
        Frame = m_CachedFrameNumber;
    }
    else
    {
        m_CachedFrameNumber = Frame;
    }

    *FrameNumber = Frame;
    return Status;
}

/* Capability queries */

BOOLEAN
UcxController::QueryClearTtBufferOnCancel()
{
    ULONG ResultLength = 0;

    if (m_Config.EvtControllerQueryUsbCapability == NULL)
        return FALSE;

    return NT_SUCCESS(m_Config.EvtControllerQueryUsbCapability(
        m_Handle,
        (PGUID)&GUID_USB_CAPABILITY_CLEAR_TT_BUFFER_ON_ASYNC_TRANSFER_CANCEL,
        0,
        NULL,
        &ResultLength));
}

BOOLEAN
UcxController::QueryStreamsSupported()
{
    USHORT StreamCount = 0;
    ULONG ResultLength = 0;
    NTSTATUS Status;

    if (m_Config.EvtControllerQueryUsbCapability == NULL)
        return FALSE;

    Status = m_Config.EvtControllerQueryUsbCapability(m_Handle,
                                                      (PGUID)&GUID_USB_CAPABILITY_STATIC_STREAMS,
                                                      sizeof(StreamCount),
                                                      &StreamCount,
                                                      &ResultLength);

    return NT_SUCCESS(Status) && StreamCount > 0;
}

/* Hub interface functions that act on the controller */

VOID
UcxController::Address0Release(
    _In_ UCXUSBDEVICE Hub)
{
    UcxAddress0QueueContext* Context = UcxGetAddress0QueueContext(m_Address0Queue);

    /* A mismatched hub is only a checked build complaint; the release still happens */
    NT_ASSERT(Context->OwnerHub == Hub);
    UNREFERENCED_PARAMETER(Hub);

    Context->OwnerHub = NULL;
    Context->OwnerDevice = NULL;

    WdfIoQueueStart(m_Address0Queue);
}

NTSTATUS
UcxController::StopIdle(
    _Inout_ PUCXHUB_STOP_IDLE_CONTEXT Context)
{
    NTSTATUS Status;

    PAGED_CODE();

    Status = WdfDeviceStopIdle(m_Fdo, TRUE);
    if (NT_SUCCESS(Status))
        Context->PowerReferenceAcquired = TRUE;
    else
        DPRINT1("Controller %p stop idle failed 0x%lx\n", this, Status);

    return Status;
}

VOID
UcxController::ResumeIdle(
    _Inout_ PUCXHUB_STOP_IDLE_CONTEXT Context)
{
    if (!Context->PowerReferenceAcquired)
        return;

    WdfDeviceResumeIdle(m_Fdo);
    Context->PowerReferenceAcquired = FALSE;
}

/** Grown is TRUE for hubs that asked for the larger stack interface; they expect the V2 layout. */
VOID
UcxController::GetInfo(
    _Out_ PVOID Info,
    _In_ BOOLEAN Grown)
{
    PUCX_CONTROLLER_PCI_INFORMATION Pci;
    PUCX_CONTROLLER_ACPI_INFORMATION Acpi;

    if (Grown)
    {
        PUCXHUB_CONTROLLER_INFO_V2 Out = (PUCXHUB_CONTROLLER_INFO_V2)Info;

        RtlZeroMemory(Out, sizeof(*Out));
        Out->Type = m_Config.ParentBusType;
        Pci = &Out->Pci;
        Acpi = &Out->Acpi;
    }
    else
    {
        PUCXHUB_CONTROLLER_INFO Out = (PUCXHUB_CONTROLLER_INFO)Info;

        RtlZeroMemory(Out, sizeof(*Out));
        Out->Type = m_Config.ParentBusType;
        Pci = &Out->Pci;
        Acpi = &Out->Acpi;
    }

    if (m_Config.ParentBusType == UcxControllerParentBusTypePci)
        *Pci = m_Config.PciDeviceInfo;
    else if (m_Config.ParentBusType == UcxControllerParentBusTypeAcpi)
        *Acpi = m_Config.AcpiDeviceInfo;
}

/* Forward progress */

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
UcxController::EnableForwardProgress(
    _In_ PUCXHUB_FORWARD_PROGRESS_INFO Info)
{
    PFN_UCX_HCD_ENABLE_FORWARD_PROGRESS HcdEnable;
    UcxUsbDevice* Device;
    NTSTATUS Status;

    PAGED_CODE();

    KeWaitForSingleObject(&m_ForwardProgressMutex, Executive, KernelMode, FALSE, NULL);

    /* The thread stays even when a later step fails */
    Status = (m_WorkerThread == NULL) ? StartWorkerThread() : STATUS_SUCCESS;

    if (NT_SUCCESS(Status) && !m_HcdReservedIoReady)
    {
        HcdEnable = HcdEnableForwardProgress();
        if (HcdEnable != NULL)
        {
            Status = HcdEnable(m_Handle);
            if (NT_SUCCESS(Status))
                m_HcdReservedIoReady = TRUE;
            else
                DPRINT1("Controller %p HCD forward progress enable failed 0x%lx\n", this, Status);
        }
    }

    if (NT_SUCCESS(Status))
    {
        Device = UcxUsbDevice::FromHandle(Info->Device);
        if (Device->IsRootHub())
            Status = m_RootHub->EnableForwardProgress();
        else
            Status = Device->EnableForwardProgress(Info);

        if (!NT_SUCCESS(Status))
            DPRINT1("Controller %p forward progress for device %p failed 0x%lx\n", this, Device, Status);
    }

    KeReleaseMutex(&m_ForwardProgressMutex, FALSE);
    return Status;
}

NTSTATUS
UcxController::StartWorkerThread()
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ThreadHandle;
    NTSTATUS Status;

    InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

    Status = PsCreateSystemThread(&ThreadHandle,
                                  THREAD_ALL_ACCESS,
                                  &ObjectAttributes,
                                  NULL,
                                  NULL,
                                  UcxController::WorkerThreadRoutine,
                                  this);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Controller %p system thread create failed 0x%lx\n", this, Status);
        return Status;
    }

    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       0,
                                       NULL,
                                       KernelMode,
                                       (PVOID*)&m_WorkerThread,
                                       NULL);
    ZwClose(ThreadHandle);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Controller %p system thread reference failed 0x%lx\n", this, Status);
        m_WorkerThread = NULL;
    }

    return Status;
}

VOID
UcxController::QueueOnWorkerThread(
    _In_ UcxWorkItem* WorkItem)
{
    SpinLockGuard Guard(&m_WorkerLock);

    InsertTailList(&m_WorkerItems, &WorkItem->ThreadQueueEntry);
    KeSetEvent(&m_WorkerWake, IO_NO_INCREMENT, FALSE);
}

/** Runs work items the worker pool could not take, so paging I/O keeps moving. */
VOID
NTAPI
UcxController::WorkerThreadRoutine(
    _In_ PVOID StartContext)
{
    UcxController* Controller = (UcxController*)StartContext;
    PLIST_ENTRY Entry;

    for (;;)
    {
        if (KeReadStateEvent(&Controller->m_WorkerWake) == 0)
            Controller->m_WorkerParked = TRUE;

        KeWaitForSingleObject(&Controller->m_WorkerWake, Executive, KernelMode, FALSE, NULL);
        NT_ASSERT(Controller->m_RootHubInD0 || Controller->m_WorkerExit);
        Controller->m_WorkerParked = FALSE;

        {
            SpinLockGuard Guard(&Controller->m_WorkerLock);

            if (Controller->m_WorkerExit)
            {
                NT_ASSERT(IsListEmpty(&Controller->m_WorkerItems));
                break;
            }

            NT_ASSERT(!IsListEmpty(&Controller->m_WorkerItems));
            Entry = RemoveHeadList(&Controller->m_WorkerItems);
            if (IsListEmpty(&Controller->m_WorkerItems))
                KeClearEvent(&Controller->m_WorkerWake);
        }

        CONTAINING_RECORD(Entry, UcxWorkItem, ThreadQueueEntry)->Run();
    }

    PsTerminateSystemThread(STATUS_SUCCESS);
}
