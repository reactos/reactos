/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Device management IOCTLs from the hub and the operations they drive
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

/*
 * The hub's own IRP reaches the HCD: UCX never builds a request here. Some
 * operations first walk the device's endpoint machines; the request is then
 * forwarded to the HCD or completed once every endpoint acknowledged.
 */

#include "ucx01000.h"

#define NDEBUG
#include <debug.h>

/** The device a device management request is about. */
static
UcxUsbDevice*
NTAPI
UcxDeviceFromManagementRequest(
    _In_ WDFREQUEST Request,
    _In_ ULONG IoControlCode)
{
    PVOID Arg1 = UcxRequestArgs(Request).Arg1;

    if (IoControlCode == IOCTL_INTERNAL_USB_SUBMIT_URB)
        return UcxUsbDevice::FromHandle((UCXUSBDEVICE)((PURB)Arg1)->UrbHeader.UsbdDeviceHandle);

    return UcxUsbDevice::FromHandle(((PUSBDEVICE_MGMT_HEADER)Arg1)->UsbDevice);
}

VOID
NTAPI
UcxEvtDeviceMgmtIoInternalDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode)
{
    UcxUsbDevice* Device = UcxDeviceFromManagementRequest(Request, IoControlCode);
    UcxController* Controller = Device->m_Controller;

    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    if (!Controller->BlockReset())
    {
        DPRINT("UsbDevice %p IOCTL 0x%lx during controller reset\n", Device->m_Handle, IoControlCode);
        Device->FailManagement(Request, IoControlCode);
        return;
    }

    /* Only an enable may bring back a device the last controller reset deprogrammed */
    if (Device->m_DeprogrammedByControllerReset)
    {
        if (IoControlCode != IOCTL_UCXHUB_DEVICE_ENABLE)
        {
            DPRINT("UsbDevice %p IOCTL 0x%lx after a controller reset deprogrammed it\n",
                   Device->m_Handle,
                   IoControlCode);
            Device->FailManagement(Request, IoControlCode);
            Controller->UnblockReset();
            return;
        }
        DPRINT("UsbDevice %p enabled again after a controller reset\n", Device->m_Handle);
        Device->m_DeprogrammedByControllerReset = FALSE;
    }

    Device->DispatchManagement(Request, IoControlCode);

    /* The operation usually continues; it does not need the reference */
    Controller->UnblockReset();
}

VOID
UcxUsbDevice::DispatchManagement(
    _In_ WDFREQUEST Request,
    _In_ ULONG IoControlCode)
{
    UCXCONTROLLER Controller = m_Controller->m_Handle;
    const UCX_USBDEVICE_EVENT_CALLBACKS* Hcd = &m_Callbacks.Public;
    PVOID Arg1 = UcxRequestArgs(Request).Arg1;
    PURB Urb;

    KeQuerySystemTime((PLARGE_INTEGER)&m_Timestamp);

    DPRINT("UsbDevice %p IOCTL 0x%lx request %p\n", m_Handle, IoControlCode, Request);

    switch (IoControlCode)
    {
        case IOCTL_UCXHUB_DEVICE_PURGE_IO:
            HandleHubPurgeIo(Request, ((PUSBDEVICE_PURGEIO)Arg1)->OnSuspend);
            break;

        case IOCTL_UCXHUB_DEVICE_ABORT_IO:
            HandleHubAbortIo(Request);
            break;

        case IOCTL_UCXHUB_DEVICE_TREE_PURGE_IO:
            HandleHubTreePurge(Request);
            break;

        case IOCTL_UCXHUB_DEVICE_START_IO:
            HandleHubStartIo(Request);
            break;

        case IOCTL_INTERNAL_USB_SUBMIT_URB:
            Urb = (PURB)Arg1;
            if (Urb->UrbHeader.Function == URB_FUNCTION_OPEN_STATIC_STREAMS)
                UcxEndpoint::FromPipe(Urb->UrbOpenStaticStreams.PipeHandle)->HandleClientStreamsEnable(Request);
            else if (Urb->UrbHeader.Function == URB_FUNCTION_CLOSE_STATIC_STREAMS)
                UcxEndpoint::FromPipe(Urb->UrbPipeRequest.PipeHandle)->HandleClientStreamsDisable(Request);
            else
            {
                DPRINT1("UsbDevice %p unexpected URB function 0x%x\n", m_Handle, Urb->UrbHeader.Function);
                WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
            }
            break;

        case IOCTL_UCXHUB_DEVICE_RESET:
            HandleHubReset(Request, FALSE);
            break;

        case IOCTL_UCXHUB_DEVICE_ADDRESS:
            Hcd->EvtUsbDeviceAddress(Controller, Request);
            break;

        case IOCTL_UCXHUB_DEVICE_UPDATE:
            UpdateFromHub(Request);
            break;

        case IOCTL_UCXHUB_DEVICE_HUB_INFO:
            Hcd->EvtUsbDeviceHubInfo(Controller, Request);
            break;

        /* Goes to the device's default endpoint, not the one in the payload */
        case IOCTL_UCXHUB_DEFAULT_ENDPOINT_UPDATE:
            m_DefaultEndpoint->m_Callbacks.DefaultEndpointUpdate(Controller, Request);
            break;

        case IOCTL_UCXHUB_ENDPOINT_RESET:
            UcxEndpoint::FromHandle(((PENDPOINT_RESET)Arg1)->Endpoint)->HandleHubReset(Request);
            break;

        case IOCTL_UCXHUB_DEVICE_ENABLE:
            Hcd->EvtUsbDeviceEnable(Controller, Request);
            break;

        case IOCTL_UCXHUB_DEVICE_DISABLE:
            HandleHubDisable(Request, FALSE);
            break;

        case IOCTL_UCXHUB_ENDPOINTS_CONFIGURE:
            HandleHubEndpointsConfigure(Request, FALSE);
            break;

        default:
            DPRINT1("UsbDevice %p unexpected IOCTL 0x%lx\n", m_Handle, IoControlCode);
            WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
            break;
    }
}

/* During a reset only operations that run through the endpoints still go ahead */
VOID
UcxUsbDevice::FailManagement(
    _In_ WDFREQUEST Request,
    _In_ ULONG IoControlCode)
{
    PVOID Arg1 = UcxRequestArgs(Request).Arg1;
    PURB Urb;

    switch (IoControlCode)
    {
        /* The suspend hint is dropped on this path */
        case IOCTL_UCXHUB_DEVICE_PURGE_IO:
            HandleHubPurgeIo(Request, FALSE);
            break;

        case IOCTL_UCXHUB_DEVICE_ABORT_IO:
            HandleHubAbortIo(Request);
            break;

        case IOCTL_UCXHUB_DEVICE_TREE_PURGE_IO:
            HandleHubTreePurge(Request);
            break;

        case IOCTL_INTERNAL_USB_SUBMIT_URB:
            Urb = (PURB)Arg1;
            if (Urb->UrbHeader.Function == URB_FUNCTION_OPEN_STATIC_STREAMS)
            {
                UcxEndpoint* Endpoint = UcxEndpoint::FromPipe(Urb->UrbOpenStaticStreams.PipeHandle);

                NT_ASSERT(!Endpoint->m_OpenFailedOnReset);
                Endpoint->m_OpenFailedOnReset = TRUE;
                DPRINT1("Endpoint %p static streams open failed by controller reset\n", Endpoint->m_Handle);
                WdfRequestComplete(Request, STATUS_NO_SUCH_DEVICE);
            }
            else if (Urb->UrbHeader.Function == URB_FUNCTION_CLOSE_STATIC_STREAMS)
            {
                UcxEndpoint::FromPipe(Urb->UrbPipeRequest.PipeHandle)->HandleClientStreamsDisable(Request);
            }
            else
            {
                DPRINT1("UsbDevice %p unexpected URB function 0x%x during reset\n", m_Handle, Urb->UrbHeader.Function);
                WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
            }
            break;

        case IOCTL_UCXHUB_DEVICE_RESET:
            NT_ASSERT(!m_ResetFailedByControllerReset);
            m_ResetFailedByControllerReset = TRUE;
            HandleHubReset(Request, TRUE);
            break;

        case IOCTL_UCXHUB_DEVICE_DISABLE:
            HandleHubDisable(Request, TRUE);
            break;

        case IOCTL_UCXHUB_ENDPOINTS_CONFIGURE:
            HandleHubEndpointsConfigure(Request, TRUE);
            break;

        case IOCTL_UCXHUB_ENDPOINT_RESET:
            NT_ASSERT(!m_EndpointResetFailedByControllerReset);
            m_EndpointResetFailedByControllerReset = TRUE;
            DPRINT1("UsbDevice %p endpoint reset failed by controller reset\n", m_Handle);
            WdfRequestComplete(Request, STATUS_NO_SUCH_DEVICE);
            break;

        default:
            DPRINT1("UsbDevice %p IOCTL 0x%lx failed by controller reset\n", m_Handle, IoControlCode);
            WdfRequestComplete(Request, STATUS_NO_SUCH_DEVICE);
            break;
    }
}

/* Requests parked while a controller reset ran come back here when it is over */
VOID
NTAPI
UcxEvtPendDuringResetIoInternalDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode)
{
    PVOID Arg1 = UcxRequestArgs(Request).Arg1;
    NTSTATUS Status;
    UcxUsbDevice* Device;
    PURB Urb;

    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    switch (IoControlCode)
    {
        case IOCTL_UCXHUB_DEVICE_RESET:
        case IOCTL_UCXHUB_DEVICE_DISABLE:
            Status = STATUS_SUCCESS;
            break;

        case IOCTL_UCXHUB_ENDPOINTS_CONFIGURE:
            Status = (((PENDPOINTS_CONFIGURE)Arg1)->EndpointsToEnableCount != 0) ? STATUS_NO_SUCH_DEVICE
                                                                                  : STATUS_SUCCESS;
            break;

        case IOCTL_INTERNAL_USB_SUBMIT_URB:
            Urb = (PURB)Arg1;
            if (Urb->UrbHeader.Function != URB_FUNCTION_CLOSE_STATIC_STREAMS)
            {
                DPRINT1("Parked request %p has unexpected URB function 0x%x\n", Request, Urb->UrbHeader.Function);
                NT_ASSERT(FALSE);
                Status = STATUS_NO_SUCH_DEVICE;
                break;
            }

            Device = UcxUsbDevice::FromHandle((UCXUSBDEVICE)Urb->UrbHeader.UsbdDeviceHandle);
            if (Device->m_FailNextStreamClose)
            {
                Device->m_FailNextStreamClose = FALSE;
                Status = STATUS_NO_SUCH_DEVICE;
            }
            else
            {
                Status = STATUS_SUCCESS;
            }
            break;

        default:
            DPRINT1("Parked request %p has unexpected IOCTL 0x%lx\n", Request, IoControlCode);
            NT_ASSERT(FALSE);
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    DPRINT("Parked request %p IOCTL 0x%lx released 0x%lx\n", Request, IoControlCode, Status);
    WdfRequestComplete(Request, Status);
}

/* One tree purge at a time: the sequential queue stays stopped until it is done */
VOID
NTAPI
UcxEvtTreePurgeIoInternalDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode)
{
    UcxController* Controller = UcxController::FromFdo(WdfIoQueueGetDevice(Queue));
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    if (IoControlCode != IOCTL_UCXHUB_DEVICE_TREE_PURGE_IO)
    {
        DPRINT1("Tree purge queue got unexpected IOCTL 0x%lx\n", IoControlCode);
        WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
        return;
    }

    WdfIoQueueStop(Queue, NULL, NULL);

    /* This IOCTL must not fail */
    Status = WdfRequestForwardToIoQueue(Request, Controller->m_DeviceMgmtQueue);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Tree purge request %p forward failed 0x%lx, completed anyway\n", Request, Status);
        WdfRequestComplete(Request, STATUS_SUCCESS);
        WdfIoQueueStart(Queue);
    }
}

/* Pending operations */

/* The count is set by the caller or the fan out, always before the first post */
VOID
UcxUsbDevice::StartOperation(
    _In_ PVOID Pending,
    _In_ PFN_UCX_OPERATION_DONE Done,
    _In_ LONG Count)
{
    NT_ASSERT(m_PendingOperation == NULL && m_PendingOperationDone == NULL);

    m_PendingOperation = Pending;
    m_PendingOperationDone = Done;
    m_PendingOperationCount = Count;
}

VOID
UcxUsbDevice::CompleteHubOperation()
{
    PFN_UCX_OPERATION_DONE Done;

    if (InterlockedDecrement(&m_PendingOperationCount) != 0)
        return;

    Done = m_PendingOperationDone;
    m_PendingOperationDone = NULL;
    (this->*Done)();
}

/* With no endpoints nothing is posted and the hub's request never completes */
VOID
UcxUsbDevice::FanOutToEndpoints(
    _In_ EpEvent Event)
{
    LIST_ENTRY Endpoints;
    PLIST_ENTRY Entry;
    UcxEndpoint* Endpoint;
    LONG Count = 0;

    InitializeListHead(&Endpoints);

    {
        SpinLockGuard Guard(&m_Controller->m_TopologyLock);

        for (Entry = m_EndpointList.Flink; Entry != &m_EndpointList; Entry = Entry->Flink)
        {
            Endpoint = CONTAINING_RECORD(Entry, UcxEndpoint, m_DeviceLink);

            WdfObjectReference(Endpoint->m_Handle);
            InsertTailList(&Endpoints, &Endpoint->m_OperationLink);
            Count++;
        }
    }

    m_PendingOperationCount = Count;

    while (!IsListEmpty(&Endpoints))
    {
        Entry = RemoveHeadList(&Endpoints);
        UcxClearListEntry(Entry);
        Endpoint = CONTAINING_RECORD(Entry, UcxEndpoint, m_OperationLink);

        Endpoint->Post(Event);
        WdfObjectDereference(Endpoint->m_Handle);
    }
}

VOID
UcxUsbDevice::HandleHubPurgeIo(
    _In_ WDFREQUEST Request,
    _In_ BOOLEAN OnSuspend)
{
    if (OnSuspend && m_Callbacks.Public.EvtUsbDeviceSuspend != NULL)
        m_Callbacks.Public.EvtUsbDeviceSuspend(m_Controller->m_Handle, m_Handle);

    StartOperation(Request, &UcxUsbDevice::CompleteHubRequest, 0);
    FanOutToEndpoints(EpEvent::HubPurge);
}

/* The hub only sends STARTIO on resume from suspend */
VOID
UcxUsbDevice::HandleHubStartIo(
    _In_ WDFREQUEST Request)
{
    if (m_Callbacks.Public.EvtUsbDeviceResume != NULL)
        m_Callbacks.Public.EvtUsbDeviceResume(m_Controller->m_Handle, m_Handle);

    StartOperation(Request, &UcxUsbDevice::CompleteHubRequest, 0);
    FanOutToEndpoints(EpEvent::HubStartRequest);
}

VOID
UcxUsbDevice::HandleHubAbortIo(
    _In_ WDFREQUEST Request)
{
    StartOperation(Request, &UcxUsbDevice::CompleteHubRequest, 0);
    FanOutToEndpoints(EpEvent::HubAbort);
}

/* Disconnected devices are included: their endpoints may not have reached purge yet */
static
VOID
NTAPI
UcxTreePurgeVisitor(
    _In_ UcxUsbDevice* Device,
    _In_opt_ PVOID Context)
{
    PLIST_ENTRY Endpoints = (PLIST_ENTRY)Context;
    PLIST_ENTRY Entry;
    UcxEndpoint* Endpoint;

    Device->m_ChildrenMayLeavePurge = FALSE;
    Device->m_EndpointsMayLeavePurge = FALSE;

    for (Entry = Device->m_EndpointList.Flink; Entry != &Device->m_EndpointList; Entry = Entry->Flink)
    {
        Endpoint = CONTAINING_RECORD(Entry, UcxEndpoint, m_DeviceLink);

        WdfObjectReference(Endpoint->m_Handle);
        InsertTailList(Endpoints, &Endpoint->m_TreePurgeLink);
    }
}

VOID
UcxUsbDevice::HandleHubTreePurge(
    _In_ WDFREQUEST Request)
{
    UcxController* Controller = m_Controller;
    LIST_ENTRY Endpoints;
    PLIST_ENTRY Entry;
    UcxEndpoint* Endpoint;
    LONG Count = 0;

    InitializeListHead(&Endpoints);

    {
        SpinLockGuard Guard(&Controller->m_TopologyLock);

        WalkSubtree(UcxTreePurgeVisitor, &Endpoints, TRUE);
    }

    for (Entry = Endpoints.Flink; Entry != &Endpoints; Entry = Entry->Flink)
        Count++;

    NT_ASSERT(Controller->m_PendingTreePurge == NULL && Controller->m_PendingTreePurgeEndpoints == 0);
    Controller->m_PendingTreePurge = Request;
    Controller->m_PendingTreePurgeEndpoints = Count;

    while (!IsListEmpty(&Endpoints))
    {
        Entry = RemoveHeadList(&Endpoints);
        UcxClearListEntry(Entry);
        Endpoint = CONTAINING_RECORD(Entry, UcxEndpoint, m_TreePurgeLink);

        Endpoint->Post(EpEvent::HubTreePurge);
        WdfObjectDereference(Endpoint->m_Handle);
    }
}

VOID
UcxUsbDevice::HandleHubReset(
    _In_ WDFREQUEST Request,
    _In_ BOOLEAN AfterReset)
{
    PUSBDEVICE_RESET Reset = (PUSBDEVICE_RESET)UcxRequestArgs(Request).Arg1;
    ULONG Count = Reset->EndpointsToDisableCount;
    UCXENDPOINT* ToDisable = Reset->EndpointsToDisable;
    UCXENDPOINT DefaultEndpoint = Reset->DefaultEndpoint;
    ULONG Index;

    {
        SpinLockGuard Guard(&m_Controller->m_TopologyLock);

        /* A tree purged device may leave tree purge once its parent has */
        if (!m_EndpointsMayLeavePurge && m_ParentHub->m_ChildrenMayLeavePurge)
            m_EndpointsMayLeavePurge = TRUE;
    }

    StartOperation(Request,
                   AfterReset ? &UcxUsbDevice::PendReset : &UcxUsbDevice::SendResetToController,
                   Count + 1);

    /* The payload may be gone once the last post finishes the operation */
    for (Index = 0; Index < Count; Index++)
        UcxEndpoint::FromHandle(ToDisable[Index])->Post(EpEvent::HubDeviceReset);

    UcxEndpoint::FromHandle(DefaultEndpoint)->Post(EpEvent::HubDeviceReset);
}

/* Only the default endpoint is told; configure disabled the others first */
VOID
UcxUsbDevice::HandleHubDisable(
    _In_ WDFREQUEST Request,
    _In_ BOOLEAN AfterReset)
{
    PUSBDEVICE_DISABLE Disable = (PUSBDEVICE_DISABLE)UcxRequestArgs(Request).Arg1;
    UCXENDPOINT DefaultEndpoint = Disable->DefaultEndpoint;

    m_Enabled = FALSE;
    StartOperation(Request,
                   AfterReset ? &UcxUsbDevice::PendDisable : &UcxUsbDevice::SendDisableToController,
                   1);

    UcxEndpoint::FromHandle(DefaultEndpoint)->Post(EpEvent::HubDisable);
}

VOID
UcxUsbDevice::HandleHubEndpointsConfigure(
    _In_ WDFREQUEST Request,
    _In_ BOOLEAN AfterReset)
{
    PENDPOINTS_CONFIGURE Configure = (PENDPOINTS_CONFIGURE)UcxRequestArgs(Request).Arg1;
    ULONG Count = Configure->EndpointsToDisableCount;
    UCXENDPOINT* ToDisable = Configure->EndpointsToDisable;
    ULONG Index;

    if (Count == 0)
    {
        /* Unlike the parked path, nothing to disable after a reset is a failure here */
        if (AfterReset)
        {
            DPRINT1("UsbDevice %p endpoints configure failed by controller reset\n", m_Handle);
            WdfRequestComplete(Request, STATUS_NO_SUCH_DEVICE);
        }
        else
            m_Callbacks.Public.EvtUsbDeviceEndpointsConfigure(m_Controller->m_Handle, Request);
        return;
    }

    StartOperation(Request,
                   AfterReset ? &UcxUsbDevice::PendEndpointsConfigure
                              : &UcxUsbDevice::SendEndpointsConfigureToController,
                   Count);

    for (Index = 0; Index < Count; Index++)
        UcxEndpoint::FromHandle(ToDisable[Index])->Post(EpEvent::HubDisable);
}

/* Applied before the HCD sees the request, whatever it then answers */
VOID
UcxUsbDevice::UpdateFromHub(
    _In_ WDFREQUEST Request)
{
    PUSBDEVICE_UPDATE Update = (PUSBDEVICE_UPDATE)UcxRequestArgs(Request).Arg1;

    if (Update->Flags.UpdateIsHub && Update->IsHub)
        m_Type |= UCX_DEVICE_TYPE_HUB;

    if (Update->Flags.UpdateAllowIoOnInvalidPipeHandles)
        TrackStaleHandles();

    if (Update->Flags.UpdateDeviceDescriptor)
        m_BcdUsb = Update->DeviceDescriptor->bcdUSB;

    m_Callbacks.Public.EvtUsbDeviceUpdate(m_Controller->m_Handle, Request);
}

/* Completions run when the last endpoint acknowledged */

VOID
UcxUsbDevice::SendResetToController()
{
    m_Callbacks.Public.EvtUsbDeviceReset(m_Controller->m_Handle, (WDFREQUEST)TakeOperation());
}

VOID
UcxUsbDevice::SendDisableToController()
{
    m_Callbacks.Public.EvtUsbDeviceDisable(m_Controller->m_Handle, (WDFREQUEST)TakeOperation());
}

VOID
UcxUsbDevice::SendEndpointsConfigureToController()
{
    m_Callbacks.Public.EvtUsbDeviceEndpointsConfigure(m_Controller->m_Handle, (WDFREQUEST)TakeOperation());
}

/* The parked request completes once the controller reset machine releases it */
static
VOID
NTAPI
UcxParkDuringReset(
    _In_ UcxUsbDevice* Device,
    _In_ WDFREQUEST Request)
{
    NTSTATUS Status;

    Status = WdfRequestForwardToIoQueue(Request, Device->m_Controller->m_PendDuringResetQueue);
    if (!NT_SUCCESS(Status))
        DPRINT1("UsbDevice %p could not park request %p 0x%lx\n", Device->m_Handle, Request, Status);
}

VOID
UcxUsbDevice::PendReset()
{
    UcxParkDuringReset(this, (WDFREQUEST)TakeOperation());
}

VOID
UcxUsbDevice::PendDisable()
{
    UcxParkDuringReset(this, (WDFREQUEST)TakeOperation());
}

VOID
UcxUsbDevice::PendEndpointsConfigure()
{
    UcxParkDuringReset(this, (WDFREQUEST)TakeOperation());
}

VOID
UcxUsbDevice::CompleteHubRequest()
{
    WdfRequestComplete((WDFREQUEST)TakeOperation(), STATUS_SUCCESS);
}

static
VOID
NTAPI
UcxResumeIrpCompletion(
    _In_ PIRP Irp,
    _In_ NTSTATUS Status)
{
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

/* A controller reset that hit while the default endpoint finished still fails the enable */
VOID
UcxUsbDevice::CompleteEnableIrp()
{
    PIRP Irp = (PIRP)TakeOperation();

    if (m_DeprogrammedByControllerReset)
        DPRINT1("UsbDevice %p enable failed by controller reset\n", m_Handle);

    UcxResumeIrpCompletion(Irp, m_DeprogrammedByControllerReset ? STATUS_NO_SUCH_DEVICE : STATUS_SUCCESS);
}

VOID
UcxUsbDevice::CompleteEndpointsConfigureIrp()
{
    UcxResumeIrpCompletion((PIRP)TakeOperation(), STATUS_SUCCESS);
}

/* A reset device lets its own children leave tree purge again; the HCD's status is not used */
VOID
UcxUsbDevice::FinishEndpointResetRequest()
{
    PIRP Irp;

    {
        SpinLockGuard Guard(&m_Controller->m_TopologyLock);

        if (m_EndpointsMayLeavePurge && !m_ChildrenMayLeavePurge)
            m_ChildrenMayLeavePurge = TRUE;
    }

    Irp = (PIRP)TakeOperation();
    UcxResumeIrpCompletion(Irp, STATUS_SUCCESS);
}

/* Up paths from the device management completion routine */

NTSTATUS
UcxUsbDevice::OnHcdEnableDone(
    _In_ PIRP Irp,
    _In_ PUSBDEVICE_ENABLE Enable)
{
    /* Completed again right here; the hub's routine runs nested */
    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        DPRINT1("UsbDevice %p HCD enable failed 0x%lx\n", m_Handle, Irp->IoStatus.Status);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    StartOperation(Irp, &UcxUsbDevice::CompleteEnableIrp, 1);
    m_Enabled = TRUE;
    UcxEndpoint::FromHandle(Enable->DefaultEndpoint)->Post(EpEvent::ConfigureDone);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

/* The endpoints that were disabled stay disabled until a later configure */
NTSTATUS
UcxUsbDevice::OnHcdResetDone(
    _In_ PIRP Irp,
    _In_ PUSBDEVICE_RESET Reset)
{
    NT_ASSERT(NT_SUCCESS(Irp->IoStatus.Status));

    StartOperation(Irp, &UcxUsbDevice::FinishEndpointResetRequest, 1);
    UcxEndpoint::FromHandle(Reset->DefaultEndpoint)->Post(EpEvent::DeviceResetDone);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
UcxUsbDevice::OnHcdEndpointsConfigureDone(
    _In_ PIRP Irp,
    _In_ PENDPOINTS_CONFIGURE Configure)
{
    ULONG Count = Configure->EndpointsToEnableCount;
    UCXENDPOINT* ToEnable = Configure->EndpointsToEnable;
    ULONG Index;

    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        DPRINT1("UsbDevice %p HCD endpoints configure failed 0x%lx\n", m_Handle, Irp->IoStatus.Status);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    if (Count == 0)
    {
        UcxResumeIrpCompletion(Irp, STATUS_SUCCESS);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    StartOperation(Irp, &UcxUsbDevice::CompleteEndpointsConfigureIrp, Count);

    for (Index = 0; Index < Count; Index++)
        UcxEndpoint::FromHandle(ToEnable[Index])->Post(EpEvent::ConfigureDone);

    return STATUS_MORE_PROCESSING_REQUIRED;
}
