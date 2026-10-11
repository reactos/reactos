/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Queues ucx01000 creates on the controller driver's FDO
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ucx01000.h"

#define NDEBUG
#include <debug.h>

/** Creates one of the five queues the root hub PDO dispatches into. */
static
NTSTATUS
NTAPI
UcxCreateControllerQueue(
    _In_ UcxController* Controller,
    _In_ WDF_IO_QUEUE_DISPATCH_TYPE Dispatch,
    _In_ ULONG PresentedRequests,
    _In_opt_ PFN_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL Handler,
    _In_opt_ PCWDF_OBJECT_CONTEXT_TYPE_INFO ContextType,
    _In_opt_ PWDF_IO_QUEUE_FORWARD_PROGRESS_POLICY Policy,
    _Out_ WDFQUEUE* Queue)
{
    WDF_IO_QUEUE_CONFIG Config;
    WDF_OBJECT_ATTRIBUTES Attributes;
    NTSTATUS Status;

    WDF_IO_QUEUE_CONFIG_INIT(&Config, Dispatch);
    Config.PowerManaged = WdfFalse;
    Config.EvtIoInternalDeviceControl = Handler;
    if (Dispatch == WdfIoQueueDispatchParallel)
        Config.Settings.Parallel.NumberOfPresentedRequests = PresentedRequests;

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);
    Attributes.ParentObject = Controller->m_Handle;
    Attributes.ContextTypeInfo = ContextType;

    Status = WdfIoQueueCreate(Controller->m_Fdo, &Config, &Attributes, Queue);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Controller %p queue create failed 0x%lx\n", Controller, Status);
        return Status;
    }

    if (Policy != NULL)
    {
        Status = WdfIoQueueAssignForwardProgressPolicy(*Queue, Policy);
        if (!NT_SUCCESS(Status))
            DPRINT1("Controller %p queue %p forward progress policy failed 0x%lx\n", Controller, *Queue, Status);
    }

    return Status;
}

NTSTATUS
UcxController::CreateQueues()
{
    WDF_IO_QUEUE_FORWARD_PROGRESS_POLICY Reserved;
    WDF_IO_QUEUE_FORWARD_PROGRESS_POLICY Examine;
    NTSTATUS Status;

    /* One reserved request, always used when allocation fails */
    WDF_IO_QUEUE_FORWARD_PROGRESS_POLICY_DEFAULT_INIT(&Reserved, 1);
    WDF_IO_QUEUE_FORWARD_PROGRESS_POLICY_EXAMINE_INIT(&Examine, 1, UcxEvtDefaultQueueExamineIrp);

    Status = UcxCreateControllerQueue(this,
                                      WdfIoQueueDispatchSequential,
                                      1,
                                      UcxEvtAddress0IoInternalDeviceControl,
                                      WDF_GET_CONTEXT_TYPE_INFO(UcxAddress0QueueContext),
                                      &Reserved,
                                      &m_Address0Queue);
    if (!NT_SUCCESS(Status))
        return Status;

    UcxGetAddress0QueueContext(m_Address0Queue)->Controller = this;

    Status = UcxCreateControllerQueue(this,
                                      WdfIoQueueDispatchParallel,
                                      m_Config.NumberOfPresentedDeviceMgmtEvtCallbacks,
                                      UcxEvtDeviceMgmtIoInternalDeviceControl,
                                      NULL,
                                      &Reserved,
                                      &m_DeviceMgmtQueue);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = UcxCreateControllerQueue(this,
                                      WdfIoQueueDispatchSequential,
                                      1,
                                      UcxEvtTreePurgeIoInternalDeviceControl,
                                      NULL,
                                      &Reserved,
                                      &m_TreePurgeQueue);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = UcxCreateControllerQueue(this,
                                      WdfIoQueueDispatchParallel,
                                      MAXULONG,
                                      UcxEvtPendDuringResetIoInternalDeviceControl,
                                      NULL,
                                      NULL,
                                      &m_PendDuringResetQueue);
    if (!NT_SUCCESS(Status))
        return Status;

    return UcxCreateControllerQueue(this,
                                    WdfIoQueueDispatchParallel,
                                    MAXULONG,
                                    UcxEvtDefaultIoInternalDeviceControl,
                                    NULL,
                                    &Examine,
                                    &m_DefaultQueue);
}

/* Address 0 ownership: the sequential queue stays stopped while a hub holds address 0 */
VOID
NTAPI
UcxEvtAddress0IoInternalDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode)
{
    UcxAddress0QueueContext* Context = UcxGetAddress0QueueContext(Queue);
    PADDRESS0_OWNERSHIP_ACQUIRE Acquire;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    if (IoControlCode != IOCTL_UCXHUB_ADDRESS0_OWNERSHIP_ACQUIRE)
    {
        DPRINT1("Address 0 queue got unexpected IOCTL 0x%lx\n", IoControlCode);
        WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
        return;
    }

    WdfIoQueueStop(Queue, NULL, NULL);

    Acquire = (PADDRESS0_OWNERSHIP_ACQUIRE)UcxRequestArgs(Request).Arg1;
    Context->OwnerHub = Acquire->Header.Hub;
    Context->OwnerDevice = Acquire->Header.UsbDevice;
    DPRINT("Address 0 owned by hub %p device %p\n", Context->OwnerHub, Context->OwnerDevice);

    WdfRequestComplete(Request, STATUS_SUCCESS);
}

/* QUERY_USB_CAPABILITY */

static
VOID
NTAPI
UcxApplyStreamCountOverride(
    _In_ UcxUsbdHandle* Handle,
    _Inout_ PUSHORT StreamCount)
{
    ULONG Override = Handle->m_VerifierStaticStreamCountOverride;
    ULONG Count = *StreamCount;

    if (Override == 0)
        return;

    if (Handle->m_GrantedStreams != 0)
        Count = min(Handle->m_GrantedStreams, (ULONG)*StreamCount);
    else if (Override == MAXULONG)
        Count = UcxRandomInRange(*StreamCount);
    else if (Override < *StreamCount)
        Count = Override;

    *StreamCount = (USHORT)Count;
}

static
NTSTATUS
NTAPI
UcxQueryUsbCapability(
    _In_ UcxController* Controller,
    _Inout_ PUCXHUB_QUERY_CAPABILITY Query,
    _Out_writes_bytes_opt_(Query->OutputBufferLength) PVOID OutputBuffer)
{
    UcxUsbdHandle* Handle;
    BOOLEAN IsChainedMdl;
    BOOLEAN IsStreams;
    NTSTATUS Status;

    if (Query->Size != sizeof(*Query) ||
        Query->Version != UCXHUB_QUERY_CAPABILITY_VERSION ||
        Query->UsbdHandle == NULL)
    {
        DPRINT1("Capability query %p malformed, size %u version %u\n", Query, Query->Size, Query->Version);
        return STATUS_INVALID_PARAMETER;
    }

    Handle = (UcxUsbdHandle*)Query->UsbdHandle;
    Query->ResultLength = 0;

    if ((Query->OutputBufferLength != 0) != (OutputBuffer != NULL))
    {
        DPRINT1("Capability query %p length %lu does not match buffer %p\n",
                Query,
                Query->OutputBufferLength,
                OutputBuffer);
        return STATUS_INVALID_PARAMETER;
    }

    IsChainedMdl = IsEqualGUID(Query->CapabilityType, GUID_USB_CAPABILITY_CHAINED_MDLS);
    IsStreams = IsEqualGUID(Query->CapabilityType, GUID_USB_CAPABILITY_STATIC_STREAMS);

    if (IsChainedMdl)
    {
        if (OutputBuffer != NULL)
        {
            DPRINT1("Chained MDL query %p must not have a buffer\n", Query);
            return STATUS_INVALID_PARAMETER;
        }

        if (!Handle->m_ChainedMdlGranted &&
            UcxVerifierWantsFailure(Handle->m_VerifierFailChainedMdl))
        {
            Status = UcxRandomErrorStatus();
            DPRINT1("Verifier fails chained MDL query %p with 0x%lx\n", Query, Status);
            return Status;
        }
    }
    else if (IsEqualGUID(Query->CapabilityType, GUID_USB_CAPABILITY_SELECTIVE_SUSPEND))
    {
        if (OutputBuffer != NULL)
        {
            DPRINT1("Selective suspend query %p must not have a buffer\n", Query);
            return STATUS_INVALID_PARAMETER;
        }
    }
    else if (IsEqualGUID(Query->CapabilityType, GUID_USB_CAPABILITY_HIGH_BANDWIDTH_ISOCH))
    {
        if (Query->OutputBufferLength != sizeof(ULONG))
        {
            DPRINT1("High bandwidth isoch query %p bad length %lu\n", Query, Query->OutputBufferLength);
            return STATUS_INVALID_PARAMETER;
        }
    }
    else if (IsStreams)
    {
        if (Query->OutputBufferLength != sizeof(USHORT))
        {
            DPRINT1("Static streams query %p bad length %lu\n", Query, Query->OutputBufferLength);
            return STATUS_INVALID_PARAMETER;
        }

        if (UcxVerifierWantsFailure(Handle->m_VerifierFailStaticStreamSupport))
        {
            Status = UcxRandomErrorStatus();
            DPRINT1("Verifier fails static streams query %p with 0x%lx\n", Query, Status);
            return Status;
        }
    }
    else
    {
        DPRINT("Capability query %p has unknown type %08lx\n", Query, Query->CapabilityType.Data1);
    }

    /* Unknown capabilities still go to the HCD, which decides */
    if (Controller->m_Config.EvtControllerQueryUsbCapability == NULL)
    {
        DPRINT1("Controller %p has no capability query callback\n", Controller);
        return STATUS_NOT_SUPPORTED;
    }

    Status = Controller->m_Config.EvtControllerQueryUsbCapability(Controller->m_Handle,
                                                                  &Query->CapabilityType,
                                                                  Query->OutputBufferLength,
                                                                  OutputBuffer,
                                                                  &Query->ResultLength);
    if (!NT_SUCCESS(Status))
    {
        /* An unsupported capability is a normal answer, not a failure */
        if (Status == STATUS_NOT_SUPPORTED)
            DPRINT("Controller %p capability %p not supported\n", Controller, Query);
        else
            DPRINT1("Controller %p capability query %p failed 0x%lx\n", Controller, Query, Status);
        return Status;
    }

    if (IsStreams)
    {
        UcxApplyStreamCountOverride(Handle, (PUSHORT)OutputBuffer);
        Handle->m_StreamsGranted = TRUE;
        Handle->m_GrantedStreams = *(PUSHORT)OutputBuffer;
        DPRINT("Granted %u static streams to handle %p\n", *(PUSHORT)OutputBuffer, Handle);
    }
    else if (IsChainedMdl)
    {
        Handle->m_ChainedMdlGranted = TRUE;
    }

    return Status;
}

/* Dump support for crash dump and hibernation on USB storage */

static
VOID
NTAPI
UcxDispatchGetDumpData(
    _In_ UcxController* Controller,
    _In_ WDFREQUEST Request)
{
    PFN_UCX_HCD_GET_DUMP_DATA GetDumpData = Controller->HcdGetDumpData();
    PUCXHUB_DUMP_DEVICE_INFO DeviceInfo;
    PIRP Irp = WdfRequestWdmGetIrp(Request);
    NTSTATUS Status;

    if (GetDumpData == NULL)
    {
        DPRINT1("Controller %p has no dump data callback\n", Controller);
        WdfRequestComplete(Request, STATUS_NOT_SUPPORTED);
        return;
    }

    DeviceInfo = (PUCXHUB_DUMP_DEVICE_INFO)Irp->AssociatedIrp.SystemBuffer;
    Status = GetDumpData(Controller->m_Handle, DeviceInfo->Device, DeviceInfo, Irp->UserBuffer);
    if (!NT_SUCCESS(Status))
        DPRINT1("Controller %p get dump data failed 0x%lx\n", Controller, Status);

    WdfRequestCompleteWithInformation(Request, Status, 0);
}

static
VOID
NTAPI
UcxDispatchFreeDumpData(
    _In_ UcxController* Controller,
    _In_ WDFREQUEST Request)
{
    PFN_UCX_HCD_FREE_DUMP_DATA FreeDumpData = Controller->HcdFreeDumpData();
    PIRP Irp = WdfRequestWdmGetIrp(Request);

    if (FreeDumpData == NULL)
    {
        DPRINT1("Controller %p has no free dump data callback\n", Controller);
        WdfRequestComplete(Request, STATUS_NOT_SUPPORTED);
        return;
    }

    FreeDumpData(Controller->m_Handle, Irp->AssociatedIrp.SystemBuffer);
    WdfRequestComplete(Request, STATUS_SUCCESS);
}

/** Root hub information IOCTLs, each guarded by a reset reference while the HCD sees it. */
static
VOID
NTAPI
UcxDispatchRootHubQuery(
    _In_ UcxController* Controller,
    _In_ WDFREQUEST Request,
    _In_ ULONG IoControlCode)
{
    UcxRootHub* RootHub = Controller->m_RootHub;

    if (!Controller->BlockReset())
    {
        DPRINT1("Controller %p unavailable, failing root hub IOCTL 0x%lx\n", Controller, IoControlCode);
        WdfRequestComplete(Request, STATUS_NO_SUCH_DEVICE);
        return;
    }

    switch (IoControlCode)
    {
        case IOCTL_UCXHUB_ROOTHUB_GET_INFO:
            RootHub->DispatchGetInfo(Request, (PUCXHUB_ROOTHUB_INFO)UcxRequestArgs(Request).Arg1);
            break;

        case IOCTL_UCXHUB_ROOTHUB_GET_20PORT_INFO:
            RootHub->m_Config.EvtRootHubGet20PortInfo(RootHub->m_Handle, Request);
            break;

        default:
            RootHub->m_Config.EvtRootHubGet30PortInfo(RootHub->m_Handle, Request);
            break;
    }

    /* The HCD may still own the request; that does not need the reference */
    Controller->UnblockReset();
}

VOID
NTAPI
UcxEvtDefaultIoInternalDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode)
{
    UcxController* Controller = UcxController::FromFdo(WdfIoQueueGetDevice(Queue));
    PUCXHUB_QUERY_CAPABILITY Query;
    UcxUsbDevice* Device;
    UcxRequestArgs Args(Request);
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    DPRINT("Controller %p request %p IOCTL 0x%lx\n", Controller, Request, IoControlCode);

    switch (IoControlCode)
    {
        case IOCTL_UCXHUB_ROOTHUB_GET_INFO:
        case IOCTL_UCXHUB_ROOTHUB_GET_20PORT_INFO:
        case IOCTL_UCXHUB_ROOTHUB_GET_30PORT_INFO:
            UcxDispatchRootHubQuery(Controller, Request, IoControlCode);
            break;

        case IOCTL_INTERNAL_USB_REGISTER_COMPOSITE_DEVICE:
            Device = UcxUsbDevice::FromHandle((UCXUSBDEVICE)Args.Arg2);
            Device->RegisterComposite(Request);
            break;

        case IOCTL_INTERNAL_USB_UNREGISTER_COMPOSITE_DEVICE:
            Device = UcxUsbDevice::FromHandle((UCXUSBDEVICE)Args.Arg2);
            Device->UnregisterComposite(Request);
            break;

        case IOCTL_UCXHUB_SET_FUNCTION_HANDLE_DATA:
            Device = UcxUsbDevice::FromHandle((UCXUSBDEVICE)Args.Arg2);
            Device->SetFunctionData(Request);
            break;

        case IOCTL_INTERNAL_USB_REQUEST_REMOTE_WAKE_NOTIFICATION:
            Device = UcxUsbDevice::FromHandle((UCXUSBDEVICE)Args.Arg2);
            Device->RequestRemoteWakeNotification(Request);
            break;

        case IOCTL_UCXHUB_QUERY_USB_CAPABILITY:
            Query = (PUCXHUB_QUERY_CAPABILITY)Args.Arg1;
            Status = UcxQueryUsbCapability(Controller,
                                           Query,
                                           WdfRequestWdmGetIrp(Request)->AssociatedIrp.SystemBuffer);
            WdfRequestComplete(Request, Status);
            break;

        case IOCTL_UCXHUB_GET_DUMP_DATA:
            UcxDispatchGetDumpData(Controller, Request);
            break;

        case IOCTL_UCXHUB_FREE_DUMP_DATA:
            UcxDispatchFreeDumpData(Controller, Request);
            break;

        case IOCTL_UCXHUB_NOTIFY_FORWARD_PROGRESS:
            Status = Controller->EnableForwardProgress((PUCXHUB_FORWARD_PROGRESS_INFO)Args.Arg1);
            WdfRequestComplete(Request, Status);
            break;

        default:
            DPRINT1("Controller %p default queue got unexpected IOCTL 0x%lx\n", Controller, IoControlCode);
            NT_ASSERT(FALSE);
            WdfRequestComplete(Request, STATUS_INVALID_DEVICE_REQUEST);
            break;
    }
}

/* A remote wake request can pend forever, so it must never take the reserved request */
WDF_IO_FORWARD_PROGRESS_ACTION
NTAPI
UcxEvtDefaultQueueExamineIrp(
    _In_ WDFQUEUE Queue,
    _In_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(Queue);

    if (IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.IoControlCode ==
        IOCTL_INTERNAL_USB_REQUEST_REMOTE_WAKE_NOTIFICATION)
    {
        DPRINT1("No reserved request for remote wake IRP %p\n", Irp);
        return WdfIoForwardProgressActionFailRequest;
    }

    return WdfIoForwardProgressActionUseReservedRequest;
}

/* Only synchronizes cancellation of the one parked abort IRP per endpoint */

VOID
NTAPI
UcxCsqInsertAbortIrp(
    _In_ PIO_CSQ Csq,
    _In_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(Csq);
    UNREFERENCED_PARAMETER(Irp);
}

VOID
NTAPI
UcxCsqRemoveAbortIrp(
    _In_ PIO_CSQ Csq,
    _In_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(Csq);
    UNREFERENCED_PARAMETER(Irp);
}

PIRP
NTAPI
UcxCsqPeekAbortIrp(
    _In_ PIO_CSQ Csq,
    _In_opt_ PIRP Irp,
    _In_opt_ PVOID PeekContext)
{
    UNREFERENCED_PARAMETER(Csq);
    UNREFERENCED_PARAMETER(Irp);
    UNREFERENCED_PARAMETER(PeekContext);

    NT_ASSERT(FALSE);
    return NULL;
}

_IRQL_raises_(DISPATCH_LEVEL)
VOID
NTAPI
UcxCsqAcquireAbortLock(
    _In_ PIO_CSQ Csq,
    _Out_ PKIRQL Irql)
{
    UcxController* Controller = CONTAINING_RECORD(Csq, UcxController, m_AbortPipeCsq);

    KeAcquireSpinLock(&Controller->m_AbortPipeCsqLock, Irql);
}

VOID
NTAPI
UcxCsqReleaseAbortLock(
    _In_ PIO_CSQ Csq,
    _In_ KIRQL Irql)
{
    UcxController* Controller = CONTAINING_RECORD(Csq, UcxController, m_AbortPipeCsq);

    KeReleaseSpinLock(&Controller->m_AbortPipeCsqLock, Irql);
}

VOID
NTAPI
UcxCsqCompleteCanceledAbortIrp(
    _In_ PIO_CSQ Csq,
    _In_ PIRP Irp)
{
    PURB Urb = (PURB)IoGetCurrentIrpStackLocation(Irp)->Parameters.Others.Argument1;

    UNREFERENCED_PARAMETER(Csq);

    DPRINT("Abort pipe IRP %p canceled\n", Irp);

    /* The mixed status pair is what USBPORT reported */
    UcxCompleteUrbAtDispatch(Irp, Urb, STATUS_CANCELLED, USBD_STATUS_DEVICE_GONE);
}
