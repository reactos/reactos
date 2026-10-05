/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     XRB submission, the fast transfer path and URB completion helpers
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ucx01000.h"

#define NDEBUG
#include <debug.h>

/* KMDF 1.11 reuse flag; the ReactOS wdfrequest.h does not list it yet */
#define UCX_REQUEST_REUSE_MUST_COMPLETE 0x00000002

static IO_COMPLETION_ROUTINE UcxXrbTransferCompletion;
static IO_COMPLETION_ROUTINE UcxXrbStreamsCompletion;

/* Completion helpers */

NTSTATUS
NTAPI
UcxCompleteUrb(
    _In_ PIRP Irp,
    _In_ PURB Urb,
    _In_ NTSTATUS Status,
    _In_ USBD_STATUS UsbdStatus)
{
    if (UcxIsXrbIrp(Irp, Urb))
        UcxXrbMarkInactive(Urb);

    Urb->UrbHeader.Status = UsbdStatus;
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

VOID
NTAPI
UcxCompleteUrbAtDispatch(
    _In_ PIRP Irp,
    _In_ PURB Urb,
    _In_ NTSTATUS Status,
    _In_ USBD_STATUS UsbdStatus)
{
    if (UcxIsXrbIrp(Irp, Urb))
    {
        UcxCompleteUrb(Irp, Urb, Status, UsbdStatus);
        return;
    }

    DispatchLevelGuard Raise;
    UcxCompleteUrb(Irp, Urb, Status, UsbdStatus);
}

/* By now the current location is the root hub PDO's again, so the XRB test works */
VOID
NTAPI
UcxCompleteHeldUrbIrp(
    _In_ PIRP Irp)
{
    PURB Urb = (PURB)IoGetCurrentIrpStackLocation(Irp)->Parameters.Others.Argument1;

    if (UcxIsXrbIrp(Irp, Urb))
        UcxXrbMarkInactive(Urb);

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

/* Submission */

/** Bugchecks a client that corrupted its XRB or submitted one that is still in flight. */
static
VOID
NTAPI
UcxXrbCheckOnSubmit(
    _In_ PIRP Irp,
    _In_ PURB Urb)
{
    UcxXrbPreamble* Xrb = UcxXrbFromUrb(Urb);

    if (Xrb->Signature != UCX_XRB_SIGNATURE)
    {
        DPRINT1("IRP %p submitted corrupted XRB for URB %p\n", Irp, Urb);
        KeBugCheckEx(UCX_BUGCHECK_USB3,
                     UCX_USB3_XRB_CORRUPTED,
                     (ULONG_PTR)Irp,
                     (ULONG_PTR)Urb,
                     (ULONG_PTR)Xrb->Handle->m_ClientDeviceObject);
    }

    if (Xrb->State != UCX_XRB_IDLE)
    {
        DPRINT1("IRP %p resubmitted URB %p while it is still active\n", Irp, Urb);
        KeBugCheckEx(UCX_BUGCHECK_USB3,
                     UCX_USB3_ACTIVE_URB_REUSED,
                     (ULONG_PTR)Irp,
                     (ULONG_PTR)Urb,
                     (ULONG_PTR)Xrb->Handle->m_ClientDeviceObject);
    }

    Xrb->State = UCX_XRB_IN_FLIGHT;
}

/* Clients on the 0x602 contract skip every legacy check */
static
NTSTATUS
NTAPI
UcxXrbSubmitTransfer(
    _In_ WDFDEVICE RootHubPdo,
    _In_ PIRP Irp,
    _In_ PURB Urb)
{
    UcxXrbPreamble* Xrb = UcxXrbFromUrb(Urb);
    UcxPipe* Pipe;
    WDF_REQUEST_REUSE_PARAMS Reuse;

    Urb->UrbHeader.Status = USBD_STATUS_PENDING;

    Pipe = UcxPipe::FromHandle(Urb->UrbBulkOrInterruptTransfer.PipeHandle);
    UcxSetTransferDirection(Urb, Pipe);
    UcxStampProcessorNumber(Urb);
    UcxLockTransferBuffer(Urb);

    if (Xrb->Request == NULL)
        return UcxForwardIrpToQueue(RootHubPdo, Irp, UcxXrbTransferCompletion, Urb, Pipe->Queue);

    /* The preallocated request saves a KMDF allocation per transfer */
    WDF_REQUEST_REUSE_PARAMS_INIT(&Reuse, WDF_REQUEST_REUSE_SET_NEW_IRP | UCX_REQUEST_REUSE_MUST_COMPLETE, STATUS_SUCCESS);
    WDF_REQUEST_REUSE_PARAMS_SET_NEW_IRP(&Reuse, Irp);
    WdfRequestReuse(Xrb->Request, &Reuse);

    return UcxForwardIrpWithRequest(RootHubPdo, Irp, UcxXrbTransferCompletion, Urb, Pipe->Queue, Xrb->Request);
}

static
NTSTATUS
NTAPI
UcxXrbSubmitChainedMdlTransfer(
    _In_ WDFDEVICE RootHubPdo,
    _In_ PIRP Irp,
    _In_ PURB Urb)
{
    UcxUsbdHandle* Handle = UcxXrbFromUrb(Urb)->Handle;

    if (!Handle->m_ChainedMdlGranted)
    {
        DPRINT1("Chained MDL URB %p from handle %p without the capability\n", Urb, Handle);

        if (Handle->m_VerifierEnabled)
        {
            KeBugCheckEx(UCX_BUGCHECK_USB3,
                         UCX_USB3_CHAINED_MDL,
                         (ULONG_PTR)Irp,
                         (ULONG_PTR)Urb,
                         (ULONG_PTR)Handle->m_ClientDeviceObject);
        }

        return UcxCompleteUrb(Irp, Urb, STATUS_INVALID_PARAMETER, USBD_STATUS_INVALID_PARAMETER);
    }

    return UcxXrbSubmitTransfer(RootHubPdo, Irp, Urb);
}

NTSTATUS
NTAPI
UcxProcessSubmitUrb(
    _In_ WDFDEVICE RootHubPdo,
    _In_ PIRP Irp)
{
    PURB Urb = (PURB)IoGetCurrentIrpStackLocation(Irp)->Parameters.Others.Argument1;

    if (!UcxIsXrbIrp(Irp, Urb))
        return UcxProcessLegacyUrb(RootHubPdo, Irp, Urb);

    UcxXrbCheckOnSubmit(Irp, Urb);

    /* Older contracts take the legacy path; its completions reset the state */
    if (UcxXrbFromUrb(Urb)->ContractVersion < USBD_CLIENT_CONTRACT_VERSION_602)
        return UcxProcessLegacyUrb(RootHubPdo, Irp, Urb);

    switch (Urb->UrbHeader.Function)
    {
        case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
        case URB_FUNCTION_ISOCH_TRANSFER:
            return UcxXrbSubmitTransfer(RootHubPdo, Irp, Urb);

        case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER_USING_CHAINED_MDL:
        case URB_FUNCTION_ISOCH_TRANSFER_USING_CHAINED_MDL:
            return UcxXrbSubmitChainedMdlTransfer(RootHubPdo, Irp, Urb);

        case URB_FUNCTION_OPEN_STATIC_STREAMS:
            return UcxEndpoint::FromPipe(Urb->UrbOpenStaticStreams.PipeHandle)->OpenStaticStreams(Irp, Urb);

        /* The pipe handle sits at the same offset in both stream URBs */
        case URB_FUNCTION_CLOSE_STATIC_STREAMS:
            return UcxEndpoint::FromPipe(Urb->UrbOpenStaticStreams.PipeHandle)->CloseStaticStreams(Irp, Urb);

        /* Only XRBs reach this; a plain URB with this function is invalid */
        case URB_FUNCTION_GET_ISOCH_PIPE_TRANSFER_PATH_DELAYS:
            return UcxEndpoint::FromPipe(Urb->UrbGetIsochPipeTransferPathDelays.PipeHandle)->GetIsochPathDelays(Irp, Urb);

        default:
            return UcxProcessLegacyUrb(RootHubPdo, Irp, Urb);
    }
}

NTSTATUS
NTAPI
UcxForwardStreamsUrb(
    _In_ WDFDEVICE RootHubPdo,
    _In_ PIRP Irp,
    _In_ PURB Urb,
    _In_ UcxController* Controller)
{
    Urb->UrbHeader.Status = USBD_STATUS_PENDING;

    return UcxForwardIrpToQueue(RootHubPdo, Irp, UcxXrbStreamsCompletion, Urb, Controller->m_DeviceMgmtQueue);
}

/* Completions */

/** Normalizes a URB the HCD never saw. */
static
VOID
NTAPI
UcxXrbFixUnprocessed(
    _Inout_ PIRP Irp,
    _Inout_ PURB Urb,
    _In_ UcxUsbDevice* Device)
{
    if (Device->m_Disconnected)
    {
        Urb->UrbHeader.Status = USBD_STATUS_DEVICE_GONE;
        Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
    }
    else
    {
        Urb->UrbHeader.Status = USBD_STATUS_CANCELED;
    }
}

static
NTSTATUS
NTAPI
UcxXrbTransferCompletion(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context)
{
    PURB Urb = (PURB)Context;
    UcxUsbDevice* Device = UcxUsbDevice::FromHandle((UCXUSBDEVICE)Urb->UrbHeader.UsbdDeviceHandle);
    ULONG Index;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (Urb->UrbHeader.Status == USBD_STATUS_PENDING)
    {
        DPRINT("XRB URB %p function 0x%x canceled before reaching the HCD\n", Urb, Urb->UrbHeader.Function);
        Device->m_TransferFailureCount++;
        Urb->UrbBulkOrInterruptTransfer.TransferBufferLength = 0;
        UcxXrbFixUnprocessed(Irp, Urb, Device);

        /* Only the plain isoch function; the chained MDL one keeps its packets */
        if (!Device->m_Disconnected && Urb->UrbHeader.Function == URB_FUNCTION_ISOCH_TRANSFER)
        {
            for (Index = 0; Index < Urb->UrbIsochronousTransfer.NumberOfPackets; Index++)
                Urb->UrbIsochronousTransfer.IsoPacket[Index].Status = 0xFFFFFFFF;
            Urb->UrbIsochronousTransfer.ErrorCount = 0;
        }
    }
    else
    {
        if (!USBD_SUCCESS(Urb->UrbHeader.Status))
        {
            DPRINT("XRB URB %p function 0x%x on device %p failed, USBD status 0x%lx\n",
                   Urb, Urb->UrbHeader.Function, Device, Urb->UrbHeader.Status);
            Device->m_TransferFailureCount++;
            Device->ReportNoPingResponseIfPending();
        }

        /* The URB status always wins on this path */
        Irp->IoStatus.Status = UcxUsbdStatusToNtStatus(Urb->UrbHeader.Status);
    }

    if (Irp->PendingReturned)
        IoMarkIrpPending(Irp);

    UcxUnlockTransferBuffer(Urb);
    UcxXrbMarkInactive(Urb);
    return STATUS_CONTINUE_COMPLETION;
}

static
NTSTATUS
NTAPI
UcxXrbStreamsCompletion(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context)
{
    PURB Urb = (PURB)Context;
    UcxUsbDevice* Device = UcxUsbDevice::FromHandle((UCXUSBDEVICE)Urb->UrbHeader.UsbdDeviceHandle);
    UcxEndpoint* Endpoint;

    UNREFERENCED_PARAMETER(DeviceObject);

    if (Urb->UrbHeader.Status == USBD_STATUS_PENDING)
    {
        DPRINT1("Streams URB %p function 0x%x never reached the HCD\n", Urb, Urb->UrbHeader.Function);
        UcxXrbFixUnprocessed(Irp, Urb, Device);
    }
    else
    {
        if (!USBD_SUCCESS(Urb->UrbHeader.Status))
        {
            DPRINT1("Streams URB %p function 0x%x failed, USBD status 0x%lx\n",
                    Urb, Urb->UrbHeader.Function, Urb->UrbHeader.Status);
        }

        Irp->IoStatus.Status = UcxUsbdStatusToNtStatus(Urb->UrbHeader.Status);
    }

    if (Irp->PendingReturned)
        IoMarkIrpPending(Irp);

    Endpoint = UcxEndpoint::FromPipe(Urb->UrbOpenStaticStreams.PipeHandle);

    switch (Urb->UrbHeader.Function)
    {
        case URB_FUNCTION_OPEN_STATIC_STREAMS:
            return Endpoint->OpenStaticStreamsComplete(Irp, Urb);

        case URB_FUNCTION_CLOSE_STATIC_STREAMS:
            return Endpoint->CloseStaticStreamsComplete(Irp);

        default:
            UcxXrbMarkInactive(Urb);
            return STATUS_CONTINUE_COMPLETION;
    }
}
