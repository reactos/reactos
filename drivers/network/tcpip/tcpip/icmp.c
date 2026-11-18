/*
 * PROJECT:     ReactOS TCP/IP protocol driver
 * LICENCE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ICMP functions implementation
 * COPYRIGHT:   2019 Victor Perevertkin (victor.perevertkin@reactos.org)
 *              Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Victor Perevertkin (victor.perevertkin@reactos.org)
 *              Ryan Wilson <ryan@verosnt.com>
 *
 * ICMPv6 echo IOCTL path for Icmp6SendEcho2 (dll/win32/iphlpapi/icmp.c).
 * Echo types from RFC 4443 Sections 4.1-4.2
 * (https://www.rfc-editor.org/rfc/rfc4443).
 */

#include "precomp.h"
#include <checksum.h>
#include <dhcp6.h>

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
    BOOLEAN IsIPv6;
} ICMP_PACKET_CONTEXT, *PICMP_PACKET_CONTEXT;

static volatile INT16 IcmpSequence = 0;

static
UINT32
GetReplyStatus(PICMP_HEADER IcmpHeader, BOOLEAN IsIPv6)
{
    if (IsIPv6)
    {
        if (IcmpHeader->Type == ICMPV6_TYPE_ECHO_REPLY)
            return IP_SUCCESS;
        return IP_REQ_TIMED_OUT;
    }

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
        if (!Context->IsIPv6)
        {
            ((PICMP_ECHO_REPLY)Irp->AssociatedIrp.SystemBuffer)->Reserved = Context->nReplies;
        }
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = CurrentStack->Parameters.DeviceIoControl.OutputBufferLength;
    }
    else
    {
        if (Context->IsIPv6)
        {
            PICMPV6_ECHO_REPLY ReplyBuffer6 =
                (PICMPV6_ECHO_REPLY)Irp->AssociatedIrp.SystemBuffer;
            RtlZeroMemory(ReplyBuffer6, sizeof(*ReplyBuffer6));
            ReplyBuffer6->Status = IP_REQ_TIMED_OUT;
            Irp->IoStatus.Information = sizeof(*ReplyBuffer6);
        }
        else
        {
            PICMP_ECHO_REPLY ReplyBuffer = (PICMP_ECHO_REPLY)Irp->AssociatedIrp.SystemBuffer;
            RtlZeroMemory(ReplyBuffer, sizeof(*ReplyBuffer));
            ReplyBuffer->Status = IP_REQ_TIMED_OUT;
            Irp->IoStatus.Information = sizeof(*ReplyBuffer);
        }

        Irp->IoStatus.Status = STATUS_TIMEOUT;
    }

    // for debugging
    nReplies = Context->nReplies;

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
    UINT16 IpHeaderSize;
    PICMP_HEADER IcmpHeader;
    PVOID DataBuffer;
    INT32 DataSize;
    INT64 CurrentTime;
    UINT32 RoundTripTime;
    PUCHAR CurrentUserBuffer;
    UINT32 ReplyHeaderSize;

    if (Context->IsIPv6)
    {
        IpHeaderSize = IPv6_HEADER_SIZE;
        ReplyHeaderSize = sizeof(ICMPV6_ECHO_REPLY);
    }
    else
    {
        IpHeaderSize = (UINT16)(sizeof(IPv4_HEADER) + OptionsLength);
        ReplyHeaderSize = sizeof(ICMP_ECHO_REPLY);
    }

    IcmpHeader = (PICMP_HEADER)((PUCHAR)Tsdu + IpHeaderSize);
    DataBuffer = (PUCHAR)Tsdu + IpHeaderSize + sizeof(ICMP_HEADER);
    DataSize = min(BytesAvailable, UINT16_MAX) - IpHeaderSize - sizeof(ICMP_HEADER);

    /* Only an echo reply completes this request. Neighbor solicitations
     * share protocol 58, and counting one finishes the IRP with
     * IP_REQ_TIMED_OUT before the reply arrives (RFC 4443 Sections 4.1-4.2,
     * https://www.rfc-editor.org/rfc/rfc4443).
     * output/ndis-disk-20261117T021630Z.log: RXL 86, then PING CHECK timeout.
     */
    if (DataSize >= 0)
    {
        if (!Context->IsIPv6 && IcmpHeader->Type == ICMP_TYPE_ECHO_REQUEST)
            return STATUS_SUCCESS;
        if (Context->IsIPv6 && IcmpHeader->Type != ICMPV6_TYPE_ECHO_REPLY)
            return STATUS_SUCCESS;
    }

    KeWaitForSingleObject(&Context->InitializationFinishedEvent, Executive, KernelMode, FALSE, NULL);
    KeClearEvent(&Context->DatagramProcessedEvent);

    TI_DbgPrint(DEBUG_ICMP, ("Received datagram Context: 0x%p v6=%u\n",
                             TdiEventContext, Context->IsIPv6));

    CurrentTime = KeQueryPerformanceCounter(NULL).QuadPart;
    RoundTripTime = (CurrentTime - Context->StartTicks) * 1000 / Context->TimerResolution.QuadPart;

    if (Context->IsIPv6)
    {
        PICMPV6_ECHO_REPLY CurrentReply6 = (PICMPV6_ECHO_REPLY)Context->CurrentReply;

        if (Context->RemainingSize >= sizeof(ICMPV6_ECHO_REPLY) && DataSize >= 0 &&
            SourceAddressLength >= sizeof(IPv6_RAW_ADDRESS))
        {
            RtlZeroMemory(CurrentReply6, sizeof(*CurrentReply6));
            RtlCopyMemory(CurrentReply6->Address.sin6_addr,
                          SourceAddress,
                          sizeof(CurrentReply6->Address.sin6_addr));
            CurrentReply6->Status = GetReplyStatus(IcmpHeader, TRUE);
            CurrentReply6->RoundTripTime = RoundTripTime;

            Context->RemainingSize -= sizeof(ICMPV6_ECHO_REPLY);
            Context->CurrentReply += sizeof(ICMPV6_ECHO_REPLY);
            Context->nReplies++;
        }

        if (Context->RemainingSize < sizeof(ICMPV6_ECHO_REPLY))
        {
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
    else
    {
        PIPv4_HEADER IpHeader = Tsdu;
        PICMP_ECHO_REPLY CurrentReply = (PICMP_ECHO_REPLY)Context->CurrentReply;

        ASSERT(SourceAddressLength == sizeof(IPAddr));

        if (Context->RemainingSize >= sizeof(ICMP_ECHO_REPLY) && DataSize >= 0)
        {
            TI_DbgPrint(DEBUG_ICMP, ("RemainingSize: %u, RoundTripTime: %u\n", Context->RemainingSize, RoundTripTime));

            memcpy(&CurrentReply->Address, SourceAddress, sizeof(CurrentReply->Address));
            CurrentReply->Status = GetReplyStatus(IcmpHeader, FALSE);
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

        CurrentUserBuffer = (PUCHAR)Context->Irp->UserBuffer +
            (Context->CurrentReply - (PUCHAR)Context->Irp->AssociatedIrp.SystemBuffer);

        if (DataSize > 0 && Context->RemainingSize > 0)
        {
            UINT32 _DataSize = min(Context->RemainingSize, DataSize);

            memcpy(Context->CurrentReply + Context->RemainingSize - _DataSize, DataBuffer, _DataSize);
            CurrentReply->Data = CurrentUserBuffer + Context->RemainingSize - _DataSize;
            CurrentReply->DataSize = _DataSize;

            Context->RemainingSize -= _DataSize;
        }

        if (OptionsLength > 0 && Context->RemainingSize > 0)
        {
            UINT32 _OptSize = min(Context->RemainingSize, OptionsLength);

            memcpy(Context->CurrentReply + Context->RemainingSize + _OptSize, Options, _OptSize);
            CurrentReply->Options.OptionsData = CurrentUserBuffer + Context->RemainingSize + _OptSize;
            CurrentReply->Options.OptionsSize = _OptSize;

            Context->RemainingSize -= _OptSize;
        }

        Context->nReplies++;

        if (Context->RemainingSize < ReplyHeaderSize)
        {
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
    UINT32 OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    UINT32 InputBufferLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    NTSTATUS Status;
    TDI_CONNECTION_INFORMATION ConnectionInfo;
    PADDRESS_FILE AddrFile;
    ULONG DataUsed;
    PUCHAR Buffer;
    UINT16 RequestSize;
    PICMP_PACKET_CONTEXT SendContext;
    LARGE_INTEGER RequestTimeout;
    UINT8 SavedTtl;
    UINT32 TimeoutMs;
    USHORT DataSize;
    BOOLEAN IsIPv6 = (DeviceObject == IP6DeviceObject);
    IP_ADDRESS LocalIp, RemoteIp;

    /* TiDispatch may pass the driver's primary FDO; use the open device. */
    if (!IsIPv6 && IrpSp->FileObject &&
        IrpSp->FileObject->DeviceObject == IP6DeviceObject)
    {
        IsIPv6 = TRUE;
    }
    if (IsIPv6 && DeviceObject == IPDeviceObject &&
        IrpSp->FileObject &&
        IrpSp->FileObject->DeviceObject == IPDeviceObject)
    {
        IsIPv6 = FALSE;
    }

    DbgPrint("ICMP ECHO v6=%u dev=%p ip6=%p fo=%p in=%u\n",
             IsIPv6, DeviceObject, IP6DeviceObject,
             IrpSp->FileObject ? IrpSp->FileObject->DeviceObject : NULL,
             IrpSp->Parameters.DeviceIoControl.InputBufferLength);
    TI_DbgPrint(DEBUG_ICMP, ("About to send datagram, OutputBufferLength: %u, SystemBuffer: %p v6=%u\n",
                             OutputBufferLength, Irp->AssociatedIrp.SystemBuffer, IsIPv6));

    if (IsIPv6)
    {
        PICMPV6_ECHO_REQUEST Request6 = Irp->AssociatedIrp.SystemBuffer;

        if (OutputBufferLength < sizeof(ICMPV6_ECHO_REPLY) ||
            InputBufferLength < sizeof(ICMPV6_ECHO_REQUEST))
        {
            return STATUS_INVALID_PARAMETER;
        }

        DataSize = (USHORT)(InputBufferLength - sizeof(ICMPV6_ECHO_REQUEST));
        TimeoutMs = Request6->Timeout;
        SavedTtl = (UINT8)Request6->Ttl;

        RemoteIp.Type = IP_ADDRESS_V6;
        RtlCopyMemory(RemoteIp.Address.IPv6Address,
                      Request6->DestinationAddress.sin6_addr,
                      sizeof(IPv6_RAW_ADDRESS));
        AddrInitIPv6Unspecified(&LocalIp);
    }
    else
    {
        PICMP_ECHO_REQUEST Request = Irp->AssociatedIrp.SystemBuffer;

        if (OutputBufferLength < sizeof(ICMP_ECHO_REPLY) ||
            InputBufferLength < sizeof(ICMP_ECHO_REQUEST))
        {
            return STATUS_INVALID_PARAMETER;
        }

        if ((Request->DataSize > UINT16_MAX - sizeof(ICMP_HEADER) - sizeof(IPv4_HEADER)) ||
            ((UINT32)Request->DataOffset + Request->DataSize > InputBufferLength) ||
            ((UINT32)Request->OptionsOffset + Request->OptionsSize > InputBufferLength))
        {
            return STATUS_INVALID_PARAMETER;
        }

        DataSize = Request->DataSize;
        TimeoutMs = Request->Timeout;
        SavedTtl = Request->Ttl;
        AddrInitIPv4(&RemoteIp, Request->Address);
        AddrInitIPv4(&LocalIp, 0);
    }

    SendContext = ExAllocatePoolWithTag(NonPagedPool, sizeof(*SendContext), OUT_DATA_TAG);
    if (!SendContext)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(SendContext, sizeof(*SendContext));
    SendContext->TdiRequest.RequestContext = Irp;
    SendContext->IsIPv6 = IsIPv6;

    RtlZeroMemory(&ConnectionInfo, sizeof(ConnectionInfo));

    if (IsIPv6)
    {
        TA_IP6_ADDRESS RemoteAddressTa6, LocalAddressTa6;

        RtlZeroMemory(&RemoteAddressTa6, sizeof(RemoteAddressTa6));
        RtlZeroMemory(&LocalAddressTa6, sizeof(LocalAddressTa6));

        RemoteAddressTa6.TAAddressCount = 1;
        RemoteAddressTa6.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP6;
        RemoteAddressTa6.Address[0].AddressType = TDI_ADDRESS_TYPE_IP6;
        RtlCopyMemory(RemoteAddressTa6.Address[0].Address[0].sin6_addr,
                      RemoteIp.Address.IPv6Address,
                      sizeof(IPv6_RAW_ADDRESS));

        LocalAddressTa6.TAAddressCount = 1;
        LocalAddressTa6.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP6;
        LocalAddressTa6.Address[0].AddressType = TDI_ADDRESS_TYPE_IP6;

        Status = FileOpenAddress(&SendContext->TdiRequest,
                                 (PTRANSPORT_ADDRESS)&LocalAddressTa6,
                                 IPPROTO_ICMPV6,
                                 FALSE,
                                 NULL);
        if (!NT_SUCCESS(Status))
        {
            ExFreePoolWithTag(SendContext, OUT_DATA_TAG);
            return Status;
        }

        ConnectionInfo.RemoteAddress = &RemoteAddressTa6;
        ConnectionInfo.RemoteAddressLength = sizeof(RemoteAddressTa6);

        /* RemoteAddressTa6 is stack-local; copy into heap for the send call below
         * by rebuilding after open — store on stack until Send returns. */
        AddrFile = (PADDRESS_FILE)SendContext->TdiRequest.Handle.AddressHandle;

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
        RequestTimeout.QuadPart = (-1LL) * 10 * 1000 * TimeoutMs;

        RequestSize = sizeof(ICMP_HEADER) + DataSize;
        Buffer = ExAllocatePoolWithTag(NonPagedPool, RequestSize, OUT_DATA_TAG);
        if (!Buffer)
        {
            FileCloseAddress(&SendContext->TdiRequest);
            ExFreePoolWithTag(SendContext, OUT_DATA_TAG);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        ((PICMP_HEADER)Buffer)->Type = ICMPV6_TYPE_ECHO_REQUEST;
        ((PICMP_HEADER)Buffer)->Code = 0;
        ((PICMP_HEADER)Buffer)->Checksum = 0;
        ((PICMP_HEADER)Buffer)->Identifier = (UINT_PTR)PsGetCurrentProcessId() & UINT16_MAX;
        ((PICMP_HEADER)Buffer)->Seq = InterlockedIncrement16(&IcmpSequence);
        if (DataSize > 0)
        {
            RtlCopyMemory(Buffer + sizeof(ICMP_HEADER),
                          (PUCHAR)Irp->AssociatedIrp.SystemBuffer + sizeof(ICMPV6_ECHO_REQUEST),
                          DataSize);
        }

        /* Checksum needs src/dst; compute after route picks local ::1. */
        {
            PNEIGHBOR_CACHE_ENTRY NCE;
            IP_ADDRESS SrcForCsum = LocalIp;

            NCE = RouteGetRouteToDestination(&RemoteIp);
            if (NCE && NCE->Address.Type == IP_ADDRESS_V6)
            {
                if (!DHCP6SourceForDestination(NCE->Interface, &RemoteIp,
                                               &SrcForCsum))
                    SrcForCsum = NCE->Address;
            }

            ((PICMP_HEADER)Buffer)->Checksum =
                IPv6PseudoChecksum(&SrcForCsum,
                                   &RemoteIp,
                                   RequestSize,
                                   IPPROTO_ICMPV6,
                                   Buffer,
                                   RequestSize);
        }

        RtlZeroMemory(Irp->AssociatedIrp.SystemBuffer, OutputBufferLength);

        LockObject(AddrFile);
        AddrFile->TTL = SavedTtl ? SavedTtl : 255;
        AddrFile->ReceiveDatagramHandlerContext = SendContext;
        AddrFile->ReceiveDatagramHandler = ReceiveDatagram;
        AddrFile->RegisteredReceiveDatagramHandler = TRUE;
        UnlockObject(AddrFile);

        /* Arm the timer before send. A reply can arrive on the receive
         * path before the transmit completion returns. */
        SendContext->StartTicks = KeQueryPerformanceCounter(NULL).QuadPart;
        IoMarkIrpPending(Irp);
        KeSetTimer(&SendContext->TimeoutTimer, RequestTimeout, &SendContext->TimeoutDpc);
        KeSetEvent(&SendContext->InitializationFinishedEvent, IO_NO_INCREMENT, FALSE);

        Status = AddrFile->Send(AddrFile, &ConnectionInfo, (PCHAR)Buffer, RequestSize, &DataUsed);
        ExFreePoolWithTag(Buffer, OUT_DATA_TAG);

        if (!NT_SUCCESS(Status) &&
            KeCancelTimer(&SendContext->TimeoutTimer))
        {
            ClearReceiveHandler(AddrFile);
            FileCloseAddress(&SendContext->TdiRequest);
            IoFreeWorkItem(SendContext->FinishWorker);
            ExFreePoolWithTag(SendContext, OUT_DATA_TAG);
            Irp->IoStatus.Status = Status;
            Irp->IoStatus.Information = 0;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }

        return STATUS_PENDING;
    }
    else
    {
        PICMP_ECHO_REQUEST Request = Irp->AssociatedIrp.SystemBuffer;
        TA_IP_ADDRESS RemoteAddressTa, LocalAddressTa;

        RtlZeroMemory(&RemoteAddressTa, sizeof(RemoteAddressTa));
        RtlZeroMemory(&LocalAddressTa, sizeof(LocalAddressTa));

        RemoteAddressTa.TAAddressCount = 1;
        RemoteAddressTa.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
        RemoteAddressTa.Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
        RemoteAddressTa.Address[0].Address[0].in_addr = Request->Address;

        LocalAddressTa.TAAddressCount = 1;
        LocalAddressTa.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
        LocalAddressTa.Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
        LocalAddressTa.Address[0].Address[0].in_addr = 0;

        Status = FileOpenAddress(&SendContext->TdiRequest,
                                 (PTRANSPORT_ADDRESS)&LocalAddressTa,
                                 IPPROTO_ICMP,
                                 FALSE,
                                 NULL);
        if (!NT_SUCCESS(Status))
        {
            ExFreePoolWithTag(SendContext, OUT_DATA_TAG);
            return Status;
        }

        AddrFile = (PADDRESS_FILE)SendContext->TdiRequest.Handle.AddressHandle;

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
        RequestTimeout.QuadPart = (-1LL) * 10 * 1000 * TimeoutMs;

        ConnectionInfo.RemoteAddress = &RemoteAddressTa;
        ConnectionInfo.RemoteAddressLength = sizeof(RemoteAddressTa);

        RequestSize = sizeof(ICMP_HEADER) + DataSize;
        Buffer = ExAllocatePoolWithTag(NonPagedPool, RequestSize, OUT_DATA_TAG);
        if (!Buffer)
        {
            FileCloseAddress(&SendContext->TdiRequest);
            ExFreePoolWithTag(SendContext, OUT_DATA_TAG);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        ((PICMP_HEADER)Buffer)->Type = ICMP_TYPE_ECHO_REQUEST;
        ((PICMP_HEADER)Buffer)->Code = ICMP_TYPE_ECHO_REPLY;
        ((PICMP_HEADER)Buffer)->Checksum = 0;
        ((PICMP_HEADER)Buffer)->Identifier = (UINT_PTR)PsGetCurrentProcessId() & UINT16_MAX;
        ((PICMP_HEADER)Buffer)->Seq = InterlockedIncrement16(&IcmpSequence);
        memcpy(Buffer + sizeof(ICMP_HEADER),
               (PUCHAR)Request + Request->DataOffset,
               Request->DataSize);
        ((PICMP_HEADER)Buffer)->Checksum = IPv4Checksum(Buffer, RequestSize, 0);

        RtlZeroMemory(Irp->AssociatedIrp.SystemBuffer, OutputBufferLength);

        LockObject(AddrFile);
        AddrFile->TTL = SavedTtl;
        AddrFile->ReceiveDatagramHandlerContext = SendContext;
        AddrFile->ReceiveDatagramHandler = ReceiveDatagram;
        AddrFile->RegisteredReceiveDatagramHandler = TRUE;
        UnlockObject(AddrFile);

        Status = AddrFile->Send(AddrFile, &ConnectionInfo, (PCHAR)Buffer, RequestSize, &DataUsed);
        SendContext->StartTicks = KeQueryPerformanceCounter(NULL).QuadPart;
        ExFreePoolWithTag(Buffer, OUT_DATA_TAG);

        if (!NT_SUCCESS(Status))
        {
            ClearReceiveHandler(AddrFile);
            FileCloseAddress(&SendContext->TdiRequest);
            IoFreeWorkItem(SendContext->FinishWorker);
            ExFreePoolWithTag(SendContext, OUT_DATA_TAG);
            return Status;
        }

        IoMarkIrpPending(Irp);
        KeSetTimer(&SendContext->TimeoutTimer, RequestTimeout, &SendContext->TimeoutDpc);
        KeSetEvent(&SendContext->InitializationFinishedEvent, IO_NO_INCREMENT, FALSE);
        return STATUS_PENDING;
    }
}
