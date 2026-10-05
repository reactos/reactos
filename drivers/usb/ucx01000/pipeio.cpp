/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Endpoint reset, abort pipe and static streams request paths
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ucx01000.h"

#define NDEBUG
#include <debug.h>

/* Endpoint reset */

VOID
UcxEndpoint::HandleHubReset(
    _In_ WDFREQUEST Request)
{
    SetPending(Request);
    Post(EpEvent::HubEndpointReset);
}

/* Held until the machine is done with the reset, whoever completed the request */
NTSTATUS
UcxEndpoint::OnHcdResetDone(
    _In_ PIRP Irp)
{
    SetPending(Irp);
    Post(EpEvent::EndpointResetDone);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

/* Abort pipe */

/** Only a running endpoint takes the abort; the IRP waits in the cancel safe queue. */
NTSTATUS
UcxEndpoint::AbortPipe(
    _In_ PIRP Irp,
    _In_ PURB Urb)
{
    if (!m_HasMachine || !Post(EpEvent::ClientAbortUrb))
    {
        DPRINT1("Endpoint %p abort pipe failed, endpoint is not running\n", m_Handle);
        return UcxCompleteUrb(Irp,
                              Urb,
                              STATUS_NO_SUCH_DEVICE,
                              UcxNtStatusToUsbdStatus(STATUS_NO_SUCH_DEVICE));
    }

    IoCsqInsertIrp(&m_Controller->m_AbortPipeCsq, Irp, &m_AbortCsqContext);
    Post(EpEvent::AbortUrbParked);

    return STATUS_PENDING;
}

/** Answered by the controller driver when it has the callback, otherwise not supported. */
NTSTATUS
UcxEndpoint::GetIsochPathDelays(
    _In_ PIRP Irp,
    _In_ PURB Urb)
{
    struct _URB_GET_ISOCH_PIPE_TRANSFER_PATH_DELAYS* Request = &Urb->UrbGetIsochPipeTransferPathDelays;
    UCX_ENDPOINT_ISOCH_TRANSFER_PATH_DELAYS Delays;
    NTSTATUS Status;

    if (m_Pipe.TransferType != UcxTransferType::Isochronous)
    {
        DPRINT1("Endpoint %p path delay query on a non isoch pipe\n", m_Handle);
        return UcxCompleteUrb(Irp, Urb, STATUS_INVALID_PARAMETER, USBD_STATUS_INVALID_PARAMETER);
    }

    if (m_Callbacks.GetIsochTransferPathDelays == NULL)
    {
        DPRINT("Endpoint %p has no path delay callback\n", m_Handle);
        return UcxCompleteUrb(Irp, Urb, STATUS_NOT_SUPPORTED, USBD_STATUS_NOT_SUPPORTED);
    }

    RtlZeroMemory(&Delays, sizeof(Delays));
    Status = m_Callbacks.GetIsochTransferPathDelays(m_Handle, &Delays);
    if (NT_SUCCESS(Status))
    {
        Request->MaximumSendPathDelayInMilliSeconds = Delays.MaximumSendPathDelayInMilliSeconds;
        Request->MaximumCompletionPathDelayInMilliSeconds = Delays.MaximumCompletionPathDelayInMilliSeconds;
    }
    else
    {
        DPRINT1("Endpoint %p path delay callback failed 0x%lx\n", m_Handle, Status);
    }

    return UcxCompleteUrb(Irp, Urb, Status, UcxNtStatusToUsbdStatus(Status));
}

/* Static streams */

/** Fails a streams URB on the submitting thread. */
static
NTSTATUS
NTAPI
UcxFailStreamsUrb(
    _In_ PIRP Irp,
    _In_ PURB Urb,
    _In_ NTSTATUS Status)
{
    return UcxCompleteUrb(Irp, Urb, Status, UcxNtStatusToUsbdStatus(Status));
}

/* Leaves nothing behind on failure */
static
NTSTATUS
NTAPI
UcxCreateStaticStreams(
    _In_ UcxEndpoint* Endpoint,
    _In_ ULONG Count,
    _Out_ UcxStaticStreams** StreamsOut)
{
    BOOLEAN Verifying = Endpoint->m_Controller->m_DriverVerifierEnabled;
    UcxStaticStreamsInit Init;
    UcxStaticStreams* Streams;
    ULONG Index;
    NTSTATUS Status;

    *StreamsOut = NULL;

    RtlZeroMemory(&Init, sizeof(Init));
    Init.Endpoint = Endpoint->m_Handle;
    Init.StreamCount = Count;

    Status = Endpoint->m_Callbacks.StaticStreamsAdd(Endpoint->m_Handle, Count, &Init);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Endpoint %p HCD streams add failed 0x%lx\n", Endpoint->m_Handle, Status);
        if (Init.Created != NULL)
            WdfObjectDelete(Init.Created);
        return Status;
    }

    /* A success without a streams object is a controller driver bug */
    if (Init.Created == NULL)
    {
        DPRINT1("Endpoint %p HCD streams add created no streams object\n", Endpoint->m_Handle);
        UcxVerifierBreak(Verifying);
        return STATUS_INTERNAL_ERROR;
    }

    Streams = UcxStaticStreams::FromHandle(Init.Created);
    Streams->m_Init = NULL;

    Status = Init.Failed ? STATUS_INTERNAL_ERROR : STATUS_SUCCESS;
    for (Index = 0; NT_SUCCESS(Status) && Index < Count; Index++)
    {
        if (!Streams->Stream(Index)->Filled)
            Status = STATUS_INTERNAL_ERROR;
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Endpoint %p HCD did not set every stream, %lu set calls for %lu streams\n",
                Endpoint->m_Handle,
                Init.SetInfoCalls,
                Count);
        UcxVerifierBreak(Verifying);
        WdfObjectDelete(Init.Created);
        return Status;
    }

    *StreamsOut = Streams;
    return STATUS_SUCCESS;
}

/* Only XRBs of 0x602 clients get here; the pipe handle is not validated */
NTSTATUS
UcxEndpoint::OpenStaticStreams(
    _In_ PIRP Irp,
    _In_ PURB Urb)
{
    UcxUsbdHandle* Client = UcxXrbFromUrb(Urb)->Handle;
    ULONG Count = Urb->UrbOpenStaticStreams.NumberOfStreams;
    UcxStaticStreams* Streams;
    KIRQL Irql = KeGetCurrentIrql();
    NTSTATUS Status;

    if (Irql != PASSIVE_LEVEL)
    {
        DPRINT1("Endpoint %p streams open at IRQL %u\n", m_Handle, Irql);
        if (Client->m_VerifierEnabled)
        {
            KeBugCheckEx(UCX_BUGCHECK_USB3,
                         UCX_USB3_STREAMS_IRQL,
                         Irql,
                         (ULONG_PTR)Irp,
                         (ULONG_PTR)Client->m_ClientDeviceObject);
        }
        return UcxFailStreamsUrb(Irp, Urb, STATUS_INVALID_PARAMETER);
    }

    if (!Client->m_StreamsGranted)
    {
        DPRINT1("Endpoint %p streams open without a streams grant\n", m_Handle);
        if (Client->m_VerifierEnabled)
        {
            KeBugCheckEx(UCX_BUGCHECK_USB3,
                         UCX_USB3_STREAMS_NOT_GRANTED,
                         (ULONG_PTR)Irp,
                         (ULONG_PTR)Urb,
                         (ULONG_PTR)Client->m_ClientDeviceObject);
        }
        return UcxFailStreamsUrb(Irp, Urb, STATUS_INVALID_PARAMETER);
    }

    if (Count == 0 || Count > Client->m_GrantedStreams)
    {
        DPRINT1("Endpoint %p streams open for %lu streams, %lu granted\n",
                m_Handle,
                Count,
                Client->m_GrantedStreams);
        if (Client->m_VerifierEnabled)
        {
            KeBugCheckEx(UCX_BUGCHECK_USB3,
                         UCX_USB3_STREAMS_BAD_COUNT,
                         Client->m_GrantedStreams,
                         Count,
                         (ULONG_PTR)Urb);
        }
        return UcxFailStreamsUrb(Irp, Urb, STATUS_INVALID_PARAMETER);
    }

    if (UcxVerifierWantsFailure(Client->m_VerifierFailEnableStaticStreams))
    {
        Status = UcxRandomErrorStatus();
        DPRINT1("Endpoint %p streams open failed by verifier 0x%lx\n", m_Handle, Status);
        return UcxFailStreamsUrb(Irp, Urb, Status);
    }

    /* One open at a time per endpoint */
    if (InterlockedIncrement(&m_StreamsOpenCount) != 1)
    {
        InterlockedDecrement(&m_StreamsOpenCount);
        DPRINT1("Endpoint %p static streams are already open\n", m_Handle);

        if (Client->m_VerifierEnabled)
        {
            KeBugCheckEx(UCX_BUGCHECK_USB3,
                         UCX_USB3_STREAMS_OPEN,
                         (ULONG_PTR)Irp,
                         (ULONG_PTR)Urb,
                         (ULONG_PTR)Client->m_ClientDeviceObject);
        }
        return UcxFailStreamsUrb(Irp, Urb, STATUS_INVALID_DEVICE_STATE);
    }

    Status = UcxCreateStaticStreams(this, Count, &Streams);
    if (!NT_SUCCESS(Status))
    {
        InterlockedDecrement(&m_StreamsOpenCount);
        return UcxFailStreamsUrb(Irp, Urb, Status);
    }

    {
        SpinLockGuard Guard(&m_Controller->m_TopologyLock);

        NT_ASSERT(m_Streams == NULL);
        m_Streams = Streams;
    }

    return UcxForwardStreamsUrb(m_Controller->m_RootHub->Pdo(), Irp, Urb, m_Controller);
}

/* A close while the root hub sleeps succeeds and leaves the streams in place */
NTSTATUS
UcxEndpoint::CloseStaticStreams(
    _In_ PIRP Irp,
    _In_ PURB Urb)
{
    if (!m_Controller->m_RootHubInD0)
        return UcxCompleteUrb(Irp, Urb, STATUS_SUCCESS, USBD_STATUS_SUCCESS);

    if (KeGetCurrentIrql() != PASSIVE_LEVEL)
    {
        DPRINT1("Endpoint %p streams close at IRQL %u\n", m_Handle, KeGetCurrentIrql());
        return UcxFailStreamsUrb(Irp, Urb, STATUS_INVALID_PARAMETER);
    }

    if (m_Streams == NULL)
    {
        DPRINT1("Endpoint %p streams close with no streams open\n", m_Handle);
        return UcxFailStreamsUrb(Irp, Urb, STATUS_INVALID_DEVICE_STATE);
    }

    return UcxForwardStreamsUrb(m_Controller->m_RootHub->Pdo(), Irp, Urb, m_Controller);
}

VOID
UcxEndpoint::HandleClientStreamsEnable(
    _In_ WDFREQUEST Request)
{
    SetPending(Request);
    Post(EpEvent::ClientStreamsEnable);
}

VOID
UcxEndpoint::HandleClientStreamsDisable(
    _In_ WDFREQUEST Request)
{
    SetPending(Request);
    Post(EpEvent::ClientStreamsDisable);
}

NTSTATUS
UcxEndpoint::OpenStaticStreamsComplete(
    _In_ PIRP Irp,
    _In_ PURB Urb)
{
    PUSBD_STREAM_INFORMATION Info = Urb->UrbOpenStaticStreams.Streams;
    UcxStaticStreams* Streams;
    UcxStream* Entry;
    ULONG Index;

    if (!NT_SUCCESS(Irp->IoStatus.Status))
    {
        DPRINT1("Endpoint %p HCD streams enable failed 0x%lx\n", m_Handle, Irp->IoStatus.Status);

        {
            SpinLockGuard Guard(&m_Controller->m_TopologyLock);

            Streams = m_Streams;
            m_Streams = NULL;
        }

        if (Streams != NULL)
            WdfObjectDelete(Streams->m_Handle);
    }
    else
    {
        Streams = m_Streams;

        for (Index = 0; Index < Streams->m_StreamCount; Index++)
        {
            Entry = Streams->Stream(Index);

            Info[Index].PipeHandle = Entry->Pipe.Handle();
            Info[Index].StreamID = Entry->StreamId;
            Info[Index].MaximumTransferSize = Entry->Pipe.MaximumTransferSize;
            Info[Index].PipeFlags = 0;
        }
    }

    /* The machine never saw this open; finish it here */
    if (m_OpenFailedOnReset)
    {
        m_OpenFailedOnReset = FALSE;
        InterlockedDecrement(&m_StreamsOpenCount);
        UcxCompleteHeldUrbIrp(Irp);

        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    SetPending(Irp);
    Post(EpEvent::StreamsOpened);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
UcxEndpoint::CloseStaticStreamsComplete(
    _In_ PIRP Irp)
{
    SetPending(Irp);
    Post(EpEvent::StreamsClosed);

    return STATUS_MORE_PROCESSING_REQUIRED;
}
