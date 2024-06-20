/*
 * PROJECT:     ReactOS TCP/IP protocol driver
 * LICENCE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ICMP functions implementation
 * COPYRIGHT:   2019 Victor Perevertkin (victor.perevertkin@reactos.org)
 */

#include "precomp.h"
#include <checksum.h>

typedef struct _ICMP_PACKET_CONTEXT
{
    TDI_REQUEST TdiRequest;
    KDPC TimeoutDpc;
    KEVENT InitializationFinishedEvent;
    KEVENT DatagramProcessedEvent;
    LARGE_INTEGER TimerResolution;
    INT64 StartTicks;
    PIRP Irp;
    PUCHAR CurrentReply;
    UINT32 RemainingSize;
    LONG nReplies;
    PIO_WORKITEM FinishWorker;
    KTIMER TimeoutTimer;
} ICMP_PACKET_CONTEXT, *PICMP_PACKET_CONTEXT;

static volatile INT16 IcmpSequence = 0;

static
UINT32
GetReplyStatus(PICMP_HEADER IcmpHeader)
{
    switch (IcmpHeader->Type)
    {
        case ICMP_TYPE_ECHO_REPLY:
            return IP_SUCCESS;
        case ICMP_TYPE_DEST_UNREACH:
            switch (IcmpHeader->Code)
            {
                case ICMP_CODE_DU_NET_UNREACH:
                    return IP_DEST_NET_UNREACHABLE;
                case ICMP_CODE_DU_HOST_UNREACH:
                    return IP_DEST_HOST_UNREACHABLE;
                case ICMP_CODE_DU_PROTOCOL_UNREACH:
                    return IP_DEST_PROT_UNREACHABLE;
                case ICMP_CODE_DU_PORT_UNREACH:
                    return IP_DEST_PORT_UNREACHABLE;
                case ICMP_CODE_DU_FRAG_DF_SET:
                    return IP_DEST_NET_UNREACHABLE;
                case ICMP_CODE_DU_SOURCE_ROUTE_FAILED:
                    return IP_BAD_ROUTE;
                default:
                    return IP_DEST_NET_UNREACHABLE;
            }
        case ICMP_TYPE_SOURCE_QUENCH:
            return IP_SOURCE_QUENCH;
        case ICMP_TYPE_TIME_EXCEEDED:
            if (IcmpHeader->Code == ICMP_CODE_TE_REASSEMBLY)
                return IP_TTL_EXPIRED_REASSEM;
            else
                return IP_TTL_EXPIRED_TRANSIT;
        case ICMP_TYPE_PARAMETER:
            return IP_PARAM_PROBLEM;
        default:
            return IP_REQ_TIMED_OUT;
    }
}

static
VOID
ClearReceiveHandler(
    _In_ PADDRESS_FILE AddrFile)
{
    LockObject(AddrFile);
    AddrFile->RegisteredReceiveDatagramHandler = FALSE;
    UnlockObject(AddrFile);
}

IO_WORKITEM_ROUTINE EndRequestHandler;

VOID
NTAPI
EndRequestHandler(
    PDEVICE_OBJECT DeviceObject,
    PVOID _Context)
{
    PICMP_PACKET_CONTEXT Context = (PICMP_PACKET_CONTEXT)_Context;
    PIO_STACK_LOCATION CurrentStack;
    PIRP Irp;
    UINT32 nReplies;
    KIRQL OldIrql;

    ClearReceiveHandler((PADDRESS_FILE)Context->TdiRequest.Handle.AddressHandle);

    KeWaitForSingleObject(&Context->DatagramProcessedEvent, Executive, KernelMode, FALSE, NULL);

    TI_DbgPrint(DEBUG_ICMP, ("Finishing request Context: %p\n", Context));

    Irp = Context->Irp;
    CurrentStack = IoGetCurrentIrpStackLocation(Irp);

    if (Context->nReplies > 0)
    {
        ((PICMP_ECHO_REPLY)Irp->AssociatedIrp.SystemBuffer)->Reserved = Context->nReplies;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = CurrentStack->Parameters.DeviceIoControl.OutputBufferLength;
    }
    else
    {
        PICMP_ECHO_REPLY ReplyBuffer = (PICMP_ECHO_REPLY)Irp->AssociatedIrp.SystemBuffer;
        RtlZeroMemory(ReplyBuffer, sizeof(*ReplyBuffer));
        ReplyBuffer->Status = IP_REQ_TIMED_OUT;

        Irp->IoStatus.Status = STATUS_TIMEOUT;
        Irp->IoStatus.Information = sizeof(*ReplyBuffer);
    }

    // for debugging
    nReplies = ((PICMP_ECHO_REPLY)Irp->AssociatedIrp.SystemBuffer)->Reserved;

    // taken from dispatch.c:IRPFinish
    IoAcquireCancelSpinLock(&OldIrql);
    IoSetCancelRoutine(Irp, NULL);
    IoReleaseCancelSpinLock(OldIrql);
    IoCompleteRequest(Irp, IO_NETWORK_INCREMENT);

    {
        NTSTATUS _Status = FileCloseAddress(&Context->TdiRequest);
        ASSERT(NT_SUCCESS(_Status));
    }

    IoFreeWorkItem(Context->FinishWorker);
    ExFreePoolWithTag(Context, OUT_DATA_TAG);

    TI_DbgPrint(DEBUG_ICMP, ("Leaving, nReplies: %u\n", nReplies));
}

NTSTATUS
NTAPI
ReceiveDatagram(
    _In_opt_ PVOID TdiEventContext,
    _In_ LONG SourceAddressLength,
    _In_reads_bytes_(SourceAddressLength) PVOID SourceAddress,
    _In_ LONG OptionsLength,
    _In_reads_bytes_opt_(OptionsLength) PVOID Options,
    _In_ ULONG ReceiveDatagramFlags,
    _In_ ULONG BytesIndicated,
    _In_ ULONG BytesAvailable,
    _Out_ ULONG *OutBytesTaken,
    _In_ PVOID Tsdu,
    _Out_opt_ PIRP *IoRequestPacket)
{
    PICMP_PACKET_CONTEXT Context = TdiEventContext;
    PIPv4_HEADER IpHeader = Tsdu;
    UINT16 IpHeaderSize = sizeof(IPv4_HEADER) + OptionsLength;
    PICMP_HEADER IcmpHeader = (PICMP_HEADER)((PUCHAR)Tsdu + IpHeaderSize);

    PVOID DataBuffer = (PUCHAR)Tsdu + IpHeaderSize + sizeof(ICMP_HEADER);
    INT32 DataSize = min(BytesAvailable, UINT16_MAX) - IpHeaderSize - sizeof(ICMP_HEADER);

    INT64 CurrentTime;
    UINT32 RoundTripTime;
    PICMP_ECHO_REPLY CurrentReply;
    PUCHAR CurrentUserBuffer;

    // do not handle echo requests
    if (DataSize >= 0 && IcmpHeader->Type == ICMP_TYPE_ECHO_REQUEST)
    {
        return STATUS_SUCCESS;
    }

    KeWaitForSingleObject(&Context->InitializationFinishedEvent, Executive, KernelMode, FALSE, NULL);
    KeClearEvent(&Context->DatagramProcessedEvent);

    ASSERT(SourceAddressLength == sizeof(IPAddr));
    TI_DbgPrint(DEBUG_ICMP, ("Received datagram Context: 0x%p\n", TdiEventContext));

    CurrentTime = KeQueryPerformanceCounter(NULL).QuadPart;
    RoundTripTime = (CurrentTime - Context->StartTicks) * 1000 / Context->TimerResolution.QuadPart;
    CurrentReply = (PICMP_ECHO_REPLY)Context->CurrentReply;

    if (Context->RemainingSize >= sizeof(ICMP_ECHO_REPLY) && DataSize >= 0)
    {
        TI_DbgPrint(DEBUG_ICMP, ("RemainingSize: %u, RoundTripTime: %u\n", Context->RemainingSize, RoundTripTime));

        memcpy(&CurrentReply->Address, SourceAddress, sizeof(CurrentReply->Address));
        CurrentReply->Status = GetReplyStatus(IcmpHeader);
        CurrentReply->RoundTripTime = RoundTripTime;
        CurrentReply->Reserved = 0;
        CurrentReply->Data = NULL;
        CurrentReply->DataSize = 0;
        CurrentReply->Options.Ttl = IpHeader->Ttl;
        CurrentReply->Options.Tos = IpHeader->Tos;
        CurrentReply->Options.Flags = IpHeader->FlagsFragOfs >> 13;
        CurrentReply->Options.OptionsData = NULL;
        CurrentReply->Options.OptionsSize = 0;

        Context->RemainingSize -= sizeof(ICMP_ECHO_REPLY);
        Context->CurrentReply += sizeof(ICMP_ECHO_REPLY);
    }

    CurrentUserBuffer = (PUCHAR)Context->Irp->UserBuffer + (Context->CurrentReply - (PUCHAR)Context->Irp->AssociatedIrp.SystemBuffer);

    if (DataSize > 0 && Context->RemainingSize > 0)
    {
        UINT32 _DataSize = min(Context->RemainingSize, DataSize);

        memcpy(Context->CurrentReply + Context->RemainingSize - _DataSize, DataBuffer, _DataSize);
        CurrentReply->Data = CurrentUserBuffer + Context->RemainingSize - _DataSize;
        CurrentReply->DataSize = _DataSize;

        Context->RemainingSize -= _DataSize;
        // Context->ReplyBuffer += _DataSize;
    }
    else
    {
        TI_DbgPrint(DEBUG_ICMP, ("RemainingSize: %u, DataSize: %d\n", Context->RemainingSize, DataSize));
    }

    if (OptionsLength > 0 && Context->RemainingSize > 0)
    {
        UINT32 _OptSize = min(Context->RemainingSize, OptionsLength);

        memcpy(Context->CurrentReply + Context->RemainingSize + _OptSize, Options, _OptSize);
        CurrentReply->Options.OptionsData = CurrentUserBuffer + Context->RemainingSize + _OptSize;
        CurrentReply->Options.OptionsSize = _OptSize;

        Context->RemainingSize -= _OptSize;
        // Context->ReplyBuffer += _OptSize;
    }
    else
    {
        TI_DbgPrint(DEBUG_ICMP, ("RemainingSize: %u, OptSize: %d\n", Context->RemainingSize, OptionsLength));
    }

    Context->nReplies++;

    if (Context->RemainingSize < sizeof(ICMP_ECHO_REPLY))
    {
        TI_DbgPrint(DEBUG_ICMP, ("The space is over: %u\n", Context->RemainingSize));

        // if the timer was inserted, that means DPC has not been queued yet
        if (KeCancelTimer(&Context->TimeoutTimer))
        {
            PADDRESS_FILE AddrFile = (PADDRESS_FILE)Context->TdiRequest.Handle.AddressHandle;
            ClearReceiveHandler(AddrFile);

            IoQueueWorkItem(Context->FinishWorker, &EndRequestHandler, DelayedWorkQueue, Context);
        }
    }

    KeSetEvent(&Context->DatagramProcessedEvent, IO_NO_INCREMENT, FALSE);
    return STATUS_SUCCESS;
}

KDEFERRED_ROUTINE TimeoutHandler;

VOID
NTAPI
TimeoutHandler(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID _Context,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{
    PICMP_PACKET_CONTEXT Context = (PICMP_PACKET_CONTEXT)_Context;

    IoQueueWorkItem(Context->FinishWorker, &EndRequestHandler, DelayedWorkQueue, _Context);
}

NTSTATUS
DispEchoRequest(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IrpSp)
{
    PICMP_ECHO_REQUEST Request = Irp->AssociatedIrp.SystemBuffer;
    UINT32 OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    UINT32 InputBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    NTSTATUS Status;
    TDI_CONNECTION_INFORMATION ConnectionInfo;
    TA_IP_ADDRESS RemoteAddressTa, LocalAddressTa;
    PADDRESS_FILE AddrFile;
    ULONG DataUsed;
    PUCHAR Buffer;
    UINT16 RequestSize;
    PICMP_PACKET_CONTEXT SendContext;
    LARGE_INTEGER RequestTimeout;
    UINT8 SavedTtl;

    TI_DbgPrint(DEBUG_ICMP, ("About to send datagram, OutputBufferLength: %u, SystemBuffer: %p\n", OutputBufferLength, Irp->AssociatedIrp.SystemBuffer));

    // check buffers
    if (OutputBufferLength < sizeof(ICMP_ECHO_REPLY) || InputBufferLength < sizeof(ICMP_ECHO_REQUEST))
    {
        return STATUS_INVALID_PARAMETER;
    }

    // check request parameters
    if ((Request->DataSize > UINT16_MAX - sizeof(ICMP_HEADER) - sizeof(IPv4_HEADER)) ||
        ((UINT32)Request->DataOffset + Request->DataSize > InputBufferLength) ||
        ((UINT32)Request->OptionsOffset + Request->OptionsSize > InputBufferLength))
    {
        return STATUS_INVALID_PARAMETER;
    }

    SendContext = ExAllocatePoolWithTag(NonPagedPool, sizeof(*SendContext), OUT_DATA_TAG);
    if (!SendContext)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(&SendContext->TdiRequest, sizeof(SendContext->TdiRequest));
    SendContext->TdiRequest.RequestContext = Irp;

    // setting up everything needed for sending the packet

    RtlZeroMemory(&RemoteAddressTa, sizeof(RemoteAddressTa));
    RtlZeroMemory(&LocalAddressTa, sizeof(LocalAddressTa));
    RtlZeroMemory(&ConnectionInfo, sizeof(ConnectionInfo));

    RemoteAddressTa.TAAddressCount = 1;
    RemoteAddressTa.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
    RemoteAddressTa.Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
    RemoteAddressTa.Address[0].Address[0].in_addr = Request->Address;

    LocalAddressTa.TAAddressCount = 1;
    LocalAddressTa.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
    LocalAddressTa.Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
    LocalAddressTa.Address[0].Address[0].in_addr = 0;

    Status = FileOpenAddress(&SendContext->TdiRequest, &LocalAddressTa, IPPROTO_ICMP, FALSE, NULL);

    if (!NT_SUCCESS(Status))
    {
        TI_DbgPrint(DEBUG_ICMP, ("Failed to open address file status: 0x%x\n", Status));

        ExFreePoolWithTag(SendContext, OUT_DATA_TAG);

        return Status;
    }

    AddrFile = (PADDRESS_FILE)SendContext->TdiRequest.Handle.AddressHandle;

    // setting up the context

    KeQueryPerformanceCounter(&SendContext->TimerResolution);
    SendContext->Irp = Irp;
    SendContext->CurrentReply = Irp->AssociatedIrp.SystemBuffer;
    SendContext->RemainingSize = OutputBufferLength;
    SendContext->nReplies = 0;
    SendContext->FinishWorker = IoAllocateWorkItem(DeviceObject);
    KeInitializeEvent(&SendContext->InitializationFinishedEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&SendContext->DatagramProcessedEvent, NotificationEvent, TRUE);

    KeInitializeDpc(&SendContext->TimeoutDpc, &TimeoutHandler, SendContext);
    KeInitializeTimerEx(&SendContext->TimeoutTimer, SynchronizationTimer);

    RequestTimeout.QuadPart = (-1LL) * 10 * 1000 * Request->Timeout;

    ConnectionInfo.RemoteAddress = &RemoteAddressTa;
    ConnectionInfo.RemoteAddressLength = sizeof(RemoteAddressTa);

    RequestSize = sizeof(ICMP_HEADER) + Request->DataSize;

    // making up the request packet
    Buffer = ExAllocatePoolWithTag(NonPagedPool, RequestSize, OUT_DATA_TAG);

    if (!Buffer)
    {
        ExFreePoolWithTag(SendContext, OUT_DATA_TAG);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ((PICMP_HEADER)Buffer)->Type = ICMP_TYPE_ECHO_REQUEST;
    ((PICMP_HEADER)Buffer)->Code = ICMP_TYPE_ECHO_REPLY;
    ((PICMP_HEADER)Buffer)->Checksum = 0;
    ((PICMP_HEADER)Buffer)->Identifier = (UINT_PTR)PsGetCurrentProcessId() & UINT16_MAX;
    ((PICMP_HEADER)Buffer)->Seq = InterlockedIncrement16(&IcmpSequence);
    memcpy(Buffer + sizeof(ICMP_HEADER), (PUCHAR)Request + Request->DataOffset, Request->DataSize);
    ((PICMP_HEADER)Buffer)->Checksum = IPv4Checksum(Buffer, RequestSize, 0);
    SavedTtl = Request->Ttl;

    RtlZeroMemory(Irp->AssociatedIrp.SystemBuffer, OutputBufferLength);

    LockObject(AddrFile);

    AddrFile->TTL = SavedTtl;
    AddrFile->ReceiveDatagramHandlerContext = SendContext;
    AddrFile->ReceiveDatagramHandler = ReceiveDatagram;
    AddrFile->RegisteredReceiveDatagramHandler = TRUE;

    UnlockObject(AddrFile);

    Status = AddrFile->Send(AddrFile, &ConnectionInfo, (PCHAR)Buffer, RequestSize, &DataUsed);

    // From this point we may receive a reply packet.
    // But we are not ready for it thus InitializationFinishedEvent is needed (see below)

    SendContext->StartTicks = KeQueryPerformanceCounter(NULL).QuadPart;

    ExFreePoolWithTag(Buffer, OUT_DATA_TAG);

    if (!NT_SUCCESS(Status))
    {
        NTSTATUS _Status;

        ClearReceiveHandler(AddrFile);
        _Status = FileCloseAddress(&SendContext->TdiRequest);
        ASSERT(NT_SUCCESS(_Status));

        IoFreeWorkItem(SendContext->FinishWorker);
        ExFreePoolWithTag(SendContext, OUT_DATA_TAG);

        TI_DbgPrint(DEBUG_ICMP, ("Failed to send a datagram: 0x%x\n", Status));
        return Status;
    }

    IoMarkIrpPending(Irp);
    KeSetTimer(&SendContext->TimeoutTimer, RequestTimeout, &SendContext->TimeoutDpc);
    KeSetEvent(&SendContext->InitializationFinishedEvent, IO_NO_INCREMENT, FALSE);

    return STATUS_PENDING;
}
