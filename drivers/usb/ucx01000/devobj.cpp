/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     UCXUSBDEVICE creation, deletion, the device tree and per device services
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ucx01000.h"

#define NDEBUG
#include <debug.h>

#define UCX_COMPOSITE_VERSION         0x100
#define UCX_MAX_COMPOSITE_FUNCTIONS   255
#define UCX_MAX_FORWARD_PROGRESS_SIZE (512 * 1024)

UcxUsbDevice*
UcxUsbDevice::FromHandle(
    _In_ UCXUSBDEVICE Handle)
{
    return UcxGetUsbDeviceContext(Handle);
}

static
ULONG64
NTAPI
UcxSystemTimeNow(VOID)
{
    LARGE_INTEGER Now;

    KeQuerySystemTime(&Now);
    return (ULONG64)Now.QuadPart;
}

VOID
UcxUsbDevice::InitializeLists()
{
    InitializeListHead(&m_ChildList);
    UcxClearListEntry(&m_ChildLink);
    InitializeListHead(&m_EndpointList);
    InitializeListHead(&m_StaleEndpointList);
    KeInitializeSpinLock(&m_TrackingLock);
    InitializeListHead(&m_TrackingList);
    KeInitializeSpinLock(&m_RemoteWakeLock);
    KeInitializeSpinLock(&m_UsbdHandleLock);
    InitializeListHead(&m_UsbdHandleList);
}

VOID
UcxUsbDevice::InitializeAsRootHub(
    _In_ UCXROOTHUB Handle,
    _In_ UcxController* Controller)
{
    m_Handle = (UCXUSBDEVICE)Handle;
    m_Controller = Controller;
    m_Type = UCX_DEVICE_TYPE_HUB | UCX_DEVICE_TYPE_ROOT_HUB;

    /* Reported as SuperSpeed for filters that ask the controller about its speed */
    m_Info.Size = sizeof(m_Info);
    m_Info.DeviceSpeed = UsbSuperSpeed;

    InitializeLists();
    m_ChildrenMayLeavePurge = TRUE;
    m_EndpointsMayLeavePurge = TRUE;
}

/* Exports */

/* Newer HCDs pass one more slot than the 1.6 structure; never copy past what we hold */
VOID
UcxUsbDevice::InitSetEventCallbacks(
    _Inout_ PUCXUSBDEVICE_INIT Init,
    _In_ PUCX_USBDEVICE_EVENT_CALLBACKS Callbacks)
{
    RtlZeroMemory(&Init->Callbacks, sizeof(Init->Callbacks));
    RtlCopyMemory(&Init->Callbacks, Callbacks, min(Callbacks->Size, (ULONG)sizeof(Init->Callbacks)));
}

_Must_inspect_result_
NTSTATUS
UcxUsbDevice::Create(
    _In_ UCXCONTROLLER Controller,
    _Inout_ PUCXUSBDEVICE_INIT* InitPointer,
    _In_opt_ PWDF_OBJECT_ATTRIBUTES Attributes,
    _Out_ UCXUSBDEVICE* UsbDevice)
{
    UcxUsbDeviceInit* Init = *InitPointer;
    UcxUsbDevice* Parent = FromHandle(Init->ParentHub);
    UcxController* ControllerContext = UcxController::FromHandle(Controller);
    WDF_OBJECT_ATTRIBUTES UcxAttributes;
    UcxUsbDevice* Device;
    WDFOBJECT Object;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    *UsbDevice = NULL;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&UcxAttributes, UcxUsbDevice);
    UcxAttributes.EvtDestroyCallback = UcxUsbDevice::EvtDestroy;

    /* Devices hang off their parent hub so deleting a hub deletes what is below it */
    if (Attributes != NULL)
        Attributes->ParentObject = Init->ParentHub;
    else
        UcxAttributes.ParentObject = Init->ParentHub;

    Status = UcxCreateObjectWithTwoContexts(Attributes != NULL ? Attributes : &UcxAttributes,
                                            Attributes != NULL ? &UcxAttributes : NULL,
                                            &Object);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("UsbDevice object create failed under hub %p 0x%lx\n", Init->ParentHub, Status);
        return Status;
    }

    Device = new (UcxGetUsbDeviceContext(Object)) UcxUsbDevice();
    Device->m_Handle = (UCXUSBDEVICE)Object;
    Device->m_Controller = ControllerContext;
    Device->m_ParentHub = Parent;
    Device->m_HubDeviceContext = Init->HubDeviceContext;
    Device->m_Info = Init->Info;
    Device->m_Callbacks = Init->Callbacks;
    Device->m_Timestamp = UcxSystemTimeNow();
    Device->InitializeLists();

    /* Keeps the tree intact past WdfObjectDelete for stale endpoints of deleted devices */
    WdfObjectReference(Parent->m_Handle);

    {
        SpinLockGuard Guard(&ControllerContext->m_TopologyLock);

        if (Parent->m_Disconnected)
        {
            Status = STATUS_NO_SUCH_DEVICE;
        }
        else
        {
            InsertTailList(&Parent->m_ChildList, &Device->m_ChildLink);
            ControllerContext->m_ChildDeviceCount++;

            /* A tree purge in progress above covers new devices too */
            Device->m_ChildrenMayLeavePurge = Parent->m_ChildrenMayLeavePurge;
            Device->m_EndpointsMayLeavePurge = Parent->m_ChildrenMayLeavePurge;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("UsbDevice %p not created, parent hub %p is disconnected\n", Object, Parent->m_Handle);
        WdfObjectDelete(Object);
        return Status;
    }

    DPRINT("UsbDevice %p created under hub %p\n", Device->m_Handle, Parent->m_Handle);

    *UsbDevice = Device->m_Handle;
    Init->Created = Device->m_Handle;
    *InitPointer = NULL;
    return STATUS_SUCCESS;
}

VOID
UcxUsbDevice::EvtDestroy(
    _In_ WDFOBJECT Object)
{
    UcxUsbDevice* Device = UcxGetUsbDeviceContext(Object);

    /* Creation failed before the context was set up */
    if (Device->m_Handle == NULL)
        return;

    {
        SpinLockGuard Guard(&Device->m_Controller->m_TopologyLock);

        if (UcxIsListEntryLinked(&Device->m_ChildLink))
        {
            RemoveEntryList(&Device->m_ChildLink);
            UcxClearListEntry(&Device->m_ChildLink);
            Device->m_Controller->m_ChildDeviceCount--;
        }
    }

    WdfObjectDereference(Device->m_ParentHub->m_Handle);
}

/** Function wake from a device; completes the matching armed notification. */
VOID
UcxUsbDevice::RemoteWakeNotification(
    _In_ ULONG Interface)
{
    UcxRemoteWakeSlot* Slot = NULL;
    WDFREQUEST Request;
    KIRQL OldIrql;
    ULONG Index;

    KeAcquireSpinLock(&m_RemoteWakeLock, &OldIrql);

    if (m_Functions == NULL)
    {
        if (m_RemoteWake.Request != NULL && m_RemoteWake.Params->Interface == Interface)
            Slot = &m_RemoteWake;
    }
    else
    {
        for (Index = 0; Index < m_FunctionCount; Index++)
        {
            UcxRemoteWakeSlot* Candidate = &m_Functions[Index].Wake;

            if (Candidate->Request != NULL && Candidate->Params->Interface == Interface)
            {
                Slot = Candidate;
                break;
            }
        }
    }

    if (Slot == NULL)
    {
        KeReleaseSpinLock(&m_RemoteWakeLock, OldIrql);
        DPRINT1("UsbDevice %p remote wake on interface %lu with no armed request\n", m_Handle, Interface);
        return;
    }

    /* A failed unmark means the cancel routine owns the request */
    if (!NT_SUCCESS(WdfRequestUnmarkCancelable(Slot->Request)))
    {
        KeReleaseSpinLock(&m_RemoteWakeLock, OldIrql);
        DPRINT("UsbDevice %p remote wake request is being canceled\n", m_Handle);
        return;
    }

    Request = Slot->Request;
    Slot->Request = NULL;
    Slot->Params = NULL;

    KeReleaseSpinLock(&m_RemoteWakeLock, OldIrql);

    DPRINT("UsbDevice %p remote wake on interface %lu\n", m_Handle, Interface);
    WdfRequestComplete(Request, STATUS_SUCCESS);
}

/* Hub interface */

/** Low and full speed devices take the nearest high speed hub as their TT. */
static
NTSTATUS
NTAPI
UcxBuildPortPath(
    _In_ UcxUsbDevice* Hub,
    _In_ ULONG PortNumber,
    _Inout_ PUCXUSBDEVICE_INFO Info)
{
    PUSB_DEVICE_PORT_PATH Path = &Info->PortPath;
    ULONG ParentDepth;

    if (Hub->IsRootHub())
    {
        Path->Size = sizeof(*Path);
        Path->PortPathDepth = 1;
        Path->PortPath[0] = PortNumber;
        Path->TTHubDepth = 0;
        Info->TtHub = NULL;
        return STATUS_SUCCESS;
    }

    ParentDepth = Hub->m_Info.PortPath.PortPathDepth;
    if (ParentDepth + 1 > MAX_USB_DEVICE_DEPTH)
    {
        DPRINT1("Hub %p port %lu is too deep for a device, depth %lu\n", Hub->m_Handle, PortNumber, ParentDepth + 1);
        return STATUS_INVALID_PARAMETER;
    }

    *Path = Hub->m_Info.PortPath;
    Path->PortPathDepth = ParentDepth + 1;
    Path->PortPath[ParentDepth] = PortNumber;

    if (Info->DeviceSpeed == UsbLowSpeed || Info->DeviceSpeed == UsbFullSpeed)
    {
        if (Hub->Speed() == UsbHighSpeed)
        {
            Path->TTHubDepth = ParentDepth;
            Info->TtHub = Hub->m_Handle;
        }
        else
        {
            Path->TTHubDepth = Hub->m_Info.PortPath.TTHubDepth;
            Info->TtHub = Hub->m_Info.TtHub;
        }
    }

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
UcxUsbDevice::CreateFromHub(
    _In_ UCXUSBDEVICE Hub,
    _In_ PUCXHUB_DEVICE_CREATE_INFO Info,
    _Out_ UCXUSBDEVICE* UsbDevice)
{
    UcxUsbDevice* HubDevice = FromHandle(Hub);
    UcxController* Controller = HubDevice->m_Controller;
    UcxUsbDeviceInit Init;
    NTSTATUS Status;

    PAGED_CODE();
    NT_ASSERT(HubDevice->IsHub());

    RtlZeroMemory(&Init, sizeof(Init));
    Init.ParentHub = Hub;
    Init.HubDeviceContext = Info->HubDeviceContext;
    Init.Info.Size = sizeof(Init.Info);
    Init.Info.DeviceSpeed = Info->DeviceSpeed;

    Status = UcxBuildPortPath(HubDevice, Info->PortNumber, &Init.Info);
    if (!NT_SUCCESS(Status))
    {
        *UsbDevice = NULL;
        return Status;
    }

    if (!Controller->BlockReset())
    {
        Status = STATUS_NO_SUCH_DEVICE;
        DPRINT1("Hub %p port %lu device add during controller reset 0x%lx\n", Hub, Info->PortNumber, Status);
    }
    else
    {
        Status = Controller->m_Config.EvtControllerUsbDeviceAdd(Controller->m_Handle, &Init.Info, &Init);
        Controller->UnblockReset();

        if (!NT_SUCCESS(Status))
            DPRINT1("Hub %p port %lu HCD device add failed 0x%lx\n", Hub, Info->PortNumber, Status);
    }

    if (NT_SUCCESS(Status))
    {
        *UsbDevice = Init.Created;
        return Status;
    }

    *UsbDevice = NULL;
    if (Init.Created != NULL)
        FromHandle(Init.Created)->DeleteFromHub();

    return Status;
}

VOID
UcxUsbDevice::DeleteFromHub()
{
    LIST_ENTRY Stale;
    PLIST_ENTRY Entry;
    UcxEndpoint* Endpoint;

    PAGED_CODE();

    DPRINT("UsbDevice %p deleted by the hub\n", m_Handle);

    UcxDropLeakedUsbdHandles(this);

    /* Endpoints still alive skip the stale stage from here on */
    m_PendingDelete = TRUE;

    InitializeListHead(&Stale);
    {
        SpinLockGuard Guard(&m_Controller->m_TopologyLock);

        if (!IsListEmpty(&m_StaleEndpointList))
        {
            /* Move the whole stale list over in one step */
            Stale.Flink = m_StaleEndpointList.Flink;
            Stale.Blink = m_StaleEndpointList.Blink;
            Stale.Flink->Blink = &Stale;
            Stale.Blink->Flink = &Stale;
            InitializeListHead(&m_StaleEndpointList);
        }
    }

    while (!IsListEmpty(&Stale))
    {
        Entry = RemoveHeadList(&Stale);
        UcxClearListEntry(Entry);

        Endpoint = CONTAINING_RECORD(Entry, UcxEndpoint, m_DeviceLink);
        Endpoint->Post(EpEvent::StaleReplaced);
    }

    WdfObjectDelete(m_Handle);
}

/* Tree walks */

VOID
UcxUsbDevice::WalkSubtree(
    _In_ PFN_UCX_DEVICE_VISITOR Visitor,
    _In_opt_ PVOID Context,
    _In_ BOOLEAN IncludeDisconnected)
{
    PLIST_ENTRY Entry;

    if (!IncludeDisconnected && m_Disconnected)
        return;

    Visitor(this, Context);

    for (Entry = m_ChildList.Flink; Entry != &m_ChildList; Entry = Entry->Flink)
    {
        CONTAINING_RECORD(Entry, UcxUsbDevice, m_ChildLink)->WalkSubtree(Visitor,
                                                                        Context,
                                                                        IncludeDisconnected);
    }
}

/* Children first at the head of the list, so the deepest endpoints purge first */
static
VOID
NTAPI
UcxDisconnectVisitor(
    _In_ UcxUsbDevice* Device,
    _In_opt_ PVOID Context)
{
    PLIST_ENTRY Endpoints = (PLIST_ENTRY)Context;
    PLIST_ENTRY Entry;
    UcxEndpoint* Endpoint;

    Device->m_Disconnected = TRUE;

    for (Entry = Device->m_EndpointList.Flink; Entry != &Device->m_EndpointList; Entry = Entry->Flink)
    {
        Endpoint = CONTAINING_RECORD(Entry, UcxEndpoint, m_DeviceLink);

        WdfObjectReference(Endpoint->m_Handle);
        InsertHeadList(Endpoints, &Endpoint->m_DisconnectLink);
    }
}

VOID
UcxUsbDevice::DisconnectPort(
    _In_ ULONG PortNumber)
{
    LIST_ENTRY Endpoints;
    PLIST_ENTRY Entry;
    UcxUsbDevice* Child;
    UcxEndpoint* Endpoint;

    InitializeListHead(&Endpoints);

    DPRINT("Hub %p disconnect on port %lu\n", m_Handle, PortNumber);

    {
        SpinLockGuard Guard(&m_Controller->m_TopologyLock);

        if (PortNumber == 0)
        {
            WalkSubtree(UcxDisconnectVisitor, &Endpoints, FALSE);
        }
        else
        {
            /* A new enumeration may already sit on the port beside the old device */
            for (Entry = m_ChildList.Flink; Entry != &m_ChildList; Entry = Entry->Flink)
            {
                Child = CONTAINING_RECORD(Entry, UcxUsbDevice, m_ChildLink);
                const USB_DEVICE_PORT_PATH* Path = &Child->m_Info.PortPath;

                if (Path->PortPath[Path->PortPathDepth - 1] == PortNumber)
                    Child->WalkSubtree(UcxDisconnectVisitor, &Endpoints, FALSE);
            }
        }
    }

    while (!IsListEmpty(&Endpoints))
    {
        Entry = RemoveHeadList(&Endpoints);
        UcxClearListEntry(Entry);
        Endpoint = CONTAINING_RECORD(Entry, UcxEndpoint, m_DisconnectLink);

        /* Root hub endpoints have no machine */
        if (Endpoint->HasMachine())
            Endpoint->Post(EpEvent::HubDisconnect);

        WdfObjectDereference(Endpoint->m_Handle);
    }
}

/* Diagnostics only, no reference is taken */
VOID
UcxUsbDevice::SetPdo(
    _In_ PDEVICE_OBJECT Pdo)
{
    NT_ASSERT(m_Pdo == NULL);
    m_Pdo = Pdo;
}

/* Per device services */

/** Pipe handles are checked against the live endpoint list from now on. */
VOID
UcxUsbDevice::TrackStaleHandles()
{
    PLIST_ENTRY Entry;
    UcxEndpoint* Endpoint;

    SpinLockGuard Topology(&m_Controller->m_TopologyLock);

    KeAcquireSpinLockAtDpcLevel(&m_TrackingLock);
    if (!m_StalePipeIoAllowed)
    {
        for (Entry = m_EndpointList.Flink; Entry != &m_EndpointList; Entry = Entry->Flink)
        {
            Endpoint = CONTAINING_RECORD(Entry, UcxEndpoint, m_DeviceLink);
            InsertTailList(&m_TrackingList, &Endpoint->m_TrackingLink);
        }
        m_StalePipeIoAllowed = TRUE;
    }
    KeReleaseSpinLockFromDpcLevel(&m_TrackingLock);
}

VOID
UcxUsbDevice::ReportNoPingResponseIfPending()
{
    UcxUsbDevice* Parent = m_ParentHub;

    if (InterlockedExchange(&m_PendingNoPingResponse, 0) != 1)
        return;

    if (Parent != NULL && Parent->m_HubNoPingResponse != NULL)
        Parent->m_HubNoPingResponse(Parent->m_HubContext, m_HubDeviceContext);
}

/** Validates one forward progress size: non zero and at most 512 KB. */
static
BOOLEAN
NTAPI
UcxIsValidForwardProgressSize(
    _In_ ULONG Size)
{
    return Size != 0 && Size <= UCX_MAX_FORWARD_PROGRESS_SIZE;
}

/* Called with the controller's forward progress mutex held */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
UcxUsbDevice::EnableForwardProgress(
    _In_ PUCXHUB_FORWARD_PROGRESS_INFO Info)
{
    ULONG ControlSize = Info->ControlPipeMaxTransferSize;
    UcxPipe* DefaultPipe = m_DefaultPipe;
    PFN_UCX_HCD_ENDPOINT_ENABLE_FORWARD_PROGRESS Enable;
    UcxPipe* Pipe;
    ULONG Index;
    NTSTATUS Status;

    PAGED_CODE();

    if (ControlSize > UCX_MAX_FORWARD_PROGRESS_SIZE)
    {
        DPRINT1("UsbDevice %p control forward progress size %lu too large\n", m_Handle, ControlSize);
        return STATUS_INVALID_PARAMETER;
    }

    if (ControlSize != 0 && DefaultPipe->ReservedIoActive)
    {
        DPRINT1("UsbDevice %p control pipe forward progress already enabled\n", m_Handle);
        return STATUS_INVALID_PARAMETER;
    }

    for (Index = 0; Index < Info->NumberOfPipes; Index++)
    {
        Pipe = UcxPipe::FromHandle(Info->Pipes[Index].PipeHandle);

        if (!UcxIsValidForwardProgressSize(Info->Pipes[Index].ReservedTransferLimit) ||
            Pipe->ReservedIoActive)
        {
            DPRINT1("UsbDevice %p pipe %lu bad forward progress size %lu or already enabled\n",
                    m_Handle,
                    Index,
                    Info->Pipes[Index].ReservedTransferLimit);
            return STATUS_INVALID_PARAMETER;
        }
    }

    /* The control pipe keeps resources it already got if they are big enough */
    if (ControlSize != 0 &&
        !(DefaultPipe->ForwardProgressAllocatedByHcd &&
          DefaultPipe->ReservedIoMaxBytes >= ControlSize))
    {
        Enable = m_DefaultEndpoint->m_Callbacks.EnableForwardProgress;
        if (Enable == NULL)
        {
            DPRINT1("UsbDevice %p HCD has no forward progress callback for the control pipe\n", m_Handle);
            return STATUS_NOT_SUPPORTED;
        }

        Status = Enable(m_DefaultEndpoint->m_Handle, ControlSize);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("UsbDevice %p control pipe forward progress failed 0x%lx\n", m_Handle, Status);
            return Status;
        }

        DefaultPipe->ForwardProgressAllocatedByHcd = TRUE;
        DefaultPipe->ReservedIoMaxBytes = ControlSize;
    }

    for (Index = 0; Index < Info->NumberOfPipes; Index++)
    {
        Pipe = UcxPipe::FromHandle(Info->Pipes[Index].PipeHandle);

        Enable = Pipe->Endpoint->m_Callbacks.EnableForwardProgress;
        if (Enable == NULL)
        {
            DPRINT1("Endpoint %p HCD has no forward progress callback\n", Pipe->Endpoint->m_Handle);
            return STATUS_NOT_SUPPORTED;
        }

        Status = Enable(Pipe->Endpoint->m_Handle, Info->Pipes[Index].ReservedTransferLimit);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Endpoint %p forward progress failed 0x%lx\n", Pipe->Endpoint->m_Handle, Status);
            return Status;
        }

        Pipe->ForwardProgressAllocatedByHcd = TRUE;
        Pipe->ReservedIoMaxBytes = Info->Pipes[Index].ReservedTransferLimit;
    }

    /* Only marked once every pipe got its resources */
    if (ControlSize != 0)
        DefaultPipe->ReservedIoActive = TRUE;

    for (Index = 0; Index < Info->NumberOfPipes; Index++)
        UcxPipe::FromHandle(Info->Pipes[Index].PipeHandle)->ReservedIoActive = TRUE;

    return STATUS_SUCCESS;
}

/* Composite devices and remote wake */

VOID
UcxUsbDevice::RegisterComposite(
    _In_ WDFREQUEST Request)
{
    PREGISTER_COMPOSITE_DEVICE Register = (PREGISTER_COMPOSITE_DEVICE)UcxRequestArgs(Request).Arg1;
    PUSBD_FUNCTION_HANDLE Handles;
    WDF_OBJECT_ATTRIBUTES Attributes;
    UcxFunctionRecord* Records;
    WDFMEMORY Memory;
    ULONG Index;
    NTSTATUS Status;

    Handles = (PUSBD_FUNCTION_HANDLE)WdfRequestWdmGetIrp(Request)->AssociatedIrp.SystemBuffer;

    if (Register == NULL || Handles == NULL ||
        Register->Size != sizeof(*Register) ||
        Register->Version != UCX_COMPOSITE_VERSION ||
        Register->FunctionCount == 0 ||
        Register->FunctionCount > UCX_MAX_COMPOSITE_FUNCTIONS)
    {
        DPRINT1("UsbDevice %p composite register rejected, request %p\n", m_Handle, Request);
        WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
        return;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = m_Handle;

    Status = WdfMemoryCreate(&Attributes,
                             NonPagedPool,
                             UCX_POOL_TAG,
                             Register->FunctionCount * sizeof(UcxFunctionRecord),
                             &Memory,
                             (PVOID*)&Records);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("UsbDevice %p function records allocation failed 0x%lx\n", m_Handle, Status);
        WdfRequestComplete(Request, Status);
        return;
    }

    RtlZeroMemory(Records, Register->FunctionCount * sizeof(UcxFunctionRecord));

    {
        SpinLockGuard Guard(&m_RemoteWakeLock);

        if (m_FunctionsMemory != NULL)
        {
            Status = STATUS_INVALID_DEVICE_STATE;
        }
        else
        {
            m_FunctionCount = Register->FunctionCount;
            m_Functions = Records;
            m_FunctionsMemory = Memory;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("UsbDevice %p is already registered as composite\n", m_Handle);
        WdfObjectDelete(Memory);
        WdfRequestComplete(Request, Status);
        return;
    }

    /* The output length is not checked */
    for (Index = 0; Index < Register->FunctionCount; Index++)
    {
        Records[Index].Index = Index;
        Records[Index].Device = this;
        Handles[Index] = (USBD_FUNCTION_HANDLE)&Records[Index];
    }

    WdfRequestComplete(Request, STATUS_SUCCESS);
}

/* Remote wake requests still armed on functions are not canceled here */
VOID
UcxUsbDevice::UnregisterComposite(
    _In_ WDFREQUEST Request)
{
    WDFMEMORY Memory = NULL;

    {
        SpinLockGuard Guard(&m_RemoteWakeLock);

        if (m_FunctionsMemory != NULL)
        {
            Memory = m_FunctionsMemory;
            m_FunctionsMemory = NULL;
            m_Functions = NULL;
            m_FunctionCount = 0;
        }
    }

    if (Memory == NULL)
    {
        DPRINT1("UsbDevice %p composite unregister without a registration\n", m_Handle);
        WdfRequestComplete(Request, STATUS_INVALID_DEVICE_STATE);
        return;
    }

    WdfObjectDelete(Memory);
    WdfRequestComplete(Request, STATUS_SUCCESS);
}

VOID
UcxUsbDevice::SetFunctionData(
    _In_ WDFREQUEST Request)
{
    PUCXHUB_FUNCTION_DATA Data;

    Data = (PUCXHUB_FUNCTION_DATA)WdfRequestWdmGetIrp(Request)->AssociatedIrp.SystemBuffer;

    {
        SpinLockGuard Guard(&m_RemoteWakeLock);

        ((UcxFunctionRecord*)Data->FunctionHandle)->ClientPdo = Data->PhysicalDeviceObject;
    }

    WdfRequestComplete(Request, STATUS_SUCCESS);
}

VOID
UcxUsbDevice::RequestRemoteWakeNotification(
    _In_ WDFREQUEST Request)
{
    PREQUEST_REMOTE_WAKE_NOTIFICATION Params;
    UcxRemoteWakeSlot* Slot;
    NTSTATUS Status;
    KIRQL OldIrql;

    Params = (PREQUEST_REMOTE_WAKE_NOTIFICATION)UcxRequestArgs(Request).Arg1;
    if (Params == NULL)
    {
        DPRINT1("UsbDevice %p remote wake request %p has no parameters\n", m_Handle, Request);
        WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
        return;
    }

    KeAcquireSpinLock(&m_RemoteWakeLock, &OldIrql);

    if ((m_Functions == NULL) != (Params->UsbdFunctionHandle == NULL))
    {
        Status = STATUS_INVALID_PARAMETER;
    }
    else
    {
        Slot = (m_Functions == NULL) ? &m_RemoteWake
                                     : &((UcxFunctionRecord*)Params->UsbdFunctionHandle)->Wake;

        if (Slot->Request != NULL)
        {
            Status = STATUS_INVALID_DEVICE_STATE;
        }
        else
        {
            Status = WdfRequestMarkCancelableEx(Request, UcxUsbDevice::EvtRemoteWakeCancel);
            if (NT_SUCCESS(Status))
            {
                Slot->Request = Request;
                Slot->Params = Params;
            }
        }
    }

    KeReleaseSpinLock(&m_RemoteWakeLock, OldIrql);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("UsbDevice %p remote wake request %p failed 0x%lx\n", m_Handle, Request, Status);
        WdfRequestComplete(Request, Status);
    }
}

VOID
UcxUsbDevice::EvtRemoteWakeCancel(
    _In_ WDFREQUEST Request)
{
    UcxRequestArgs Args(Request);
    PREQUEST_REMOTE_WAKE_NOTIFICATION Params = (PREQUEST_REMOTE_WAKE_NOTIFICATION)Args.Arg1;
    UcxUsbDevice* Device = FromHandle((UCXUSBDEVICE)Args.Arg2);
    UcxRemoteWakeSlot* Slot;

    {
        SpinLockGuard Guard(&Device->m_RemoteWakeLock);

        if (Params->UsbdFunctionHandle == NULL)
        {
            Slot = &Device->m_RemoteWake;
        }
        else
        {
            /* The client may have unregistered the functions already */
            Slot = (Device->m_Functions != NULL)
                       ? &((UcxFunctionRecord*)Params->UsbdFunctionHandle)->Wake
                       : NULL;
        }

        if (Slot != NULL)
        {
            Slot->Request = NULL;
            Slot->Params = NULL;
        }
    }

    DPRINT("UsbDevice %p remote wake request %p canceled\n", Device->m_Handle, Request);
    WdfRequestComplete(Request, STATUS_CANCELLED);
}
