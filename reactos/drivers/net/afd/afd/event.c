/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver
 * FILE:        afd/event.c
 * PURPOSE:     TDI event handlers
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */
#include <ntddk.h>
#include <afd.h>

NTSTATUS AfdpEventReceive(
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket)
{
  PAFDFCB FCB = (PAFDFCB)TdiEventContext;
  PAFD_READ_REQUEST ReadRequest;
  PVOID ReceiveBuffer;
  PAFD_BUFFER Buffer;
  PLIST_ENTRY Entry;
  NTSTATUS Status;
  KIRQL OldIrql;
  ULONG Count;

  AFD_DbgPrint(MAX_TRACE, ("Called.\n"));

  AFD_DbgPrint(MID_TRACE, ("Receiving (%d of %d) bytes on (0x%X).\n",
    BytesIndicated, BytesAvailable, ConnectionContext));

  ReceiveBuffer = ExAllocatePool(NonPagedPool, BytesAvailable);
  if (!ReceiveBuffer)
    return STATUS_INSUFFICIENT_RESOURCES;

  /*Buffer = (PAFD_BUFFER)ExAllocateFromNPagedLookasideList(
    &BufferLookasideList);*/
  Buffer = (PAFD_BUFFER)ExAllocatePool(NonPagedPool, sizeof(AFD_BUFFER));
  if (!Buffer) {
    ExFreePool(ReceiveBuffer);
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  if (BytesIndicated != BytesAvailable)
    {
      AFD_DbgPrint(MIN_TRACE, ("WARNING: BytesIndicated != BytesAvailable.\n",
        BytesIndicated, BytesAvailable));
    }

  /* Copy the data to a local buffer */
  RtlCopyMemory(ReceiveBuffer, Tsdu, BytesIndicated);

  Buffer->Buffer.len = BytesIndicated;
  Buffer->Buffer.buf = ReceiveBuffer;

  ExInterlockedInsertTailList(
    &FCB->ReceiveQueue,
    &Buffer->ListEntry,
    &FCB->ReceiveQueueLock);

  KeAcquireSpinLock(&FCB->ReadRequestQueueLock, &OldIrql);

  if (!IsListEmpty(&FCB->ReadRequestQueue)) {
    AFD_DbgPrint(MAX_TRACE, ("Satisfying read request.\n"));

    Entry = RemoveHeadList(&FCB->ReceiveQueue);
    ReadRequest = CONTAINING_RECORD(Entry, AFD_READ_REQUEST, ListEntry);

    Status = FillWSABuffers(
      FCB,
      ReadRequest->Recv.Request->Buffers,
      ReadRequest->Recv.Request->BufferCount,
      &Count);
    ReadRequest->Recv.Reply->NumberOfBytesRecvd = Count;
    ReadRequest->Recv.Reply->Status = NO_ERROR;

    ReadRequest->Irp->IoStatus.Information = 0;
    ReadRequest->Irp->IoStatus.Status = Status;

    AFD_DbgPrint(MAX_TRACE, ("Completing IRP at (0x%X).\n", ReadRequest->Irp));

    IoCompleteRequest(ReadRequest->Irp, IO_NETWORK_INCREMENT);
  }

  KeReleaseSpinLock(&FCB->ReadRequestQueueLock, OldIrql);

  *BytesTaken = BytesIndicated;

  AFD_DbgPrint(MAX_TRACE, ("Leaving.\n"));

  return STATUS_SUCCESS;
}


NTSTATUS AfdEventError(
    IN PVOID TdiEventContext,
    IN NTSTATUS Status)
{
    AFD_DbgPrint(MAX_TRACE, ("Called.\n"));

    return STATUS_SUCCESS;
}

/* All pending connect requests succeed at once on TCP sockets. */

NTSTATUS AfdEventConnect(
    IN PVOID TdiEventContext,
    IN LONG ConnectDataLength,
    IN PVOID ConnectData,
    IN LONG ConnectInformationLength,
    IN PVOID ConnectInformation,
    IN LONG OptionsLength,
    IN PVOID Options,
    OUT CONNECTION_CONTEXT *OutConnectionContext,
    OUT PIRP ConnectIrp)
{
    PAFDFCB FCB = (PAFDFCB)TdiEventContext;
    PAFD_CONNECT_REQUEST ConnectRequest;
    PLIST_ENTRY Entry;
    KIRQL OldIrql;

    AFD_DbgPrint(MAX_TRACE, ("Connecting socket\n"));

    KeAcquireSpinLock(&FCB->ConnectRequestQueueLock, &OldIrql);
    
    FCB->State = SOCKET_STATE_CONNECTED;

    while (!IsListEmpty(&FCB->ConnectRequestQueue)) {
	AFD_DbgPrint(MAX_TRACE, ("Satisfying connect request.\n"));
	
	Entry = RemoveHeadList(&FCB->ConnectRequestQueue);	
	ConnectRequest = CONTAINING_RECORD(Entry, AFD_CONNECT_REQUEST, ListEntry);
#if 0
	IoAcquireCancelSpinLock(&OldIrql);
	IoSetCancelRoutine(ConnectRequest->Irp, NULL);
	IoReleaseCancelSpinLock(OldIrql);
	IoCompleteRequest(ConnectRequest->Irp, IO_NETWORK_INCREMENT);
#endif
    }

    KeReleaseSpinLock(&FCB->ConnectRequestQueueLock, OldIrql);

    AFD_DbgPrint(MAX_TRACE, ("Connect notification done\n"));

    return STATUS_SUCCESS;
}


NTSTATUS AfdEventDisconnect(
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN LONG DisconnectDataLength,
    IN PVOID DisconnectData,
    IN LONG DisconnectInformationLength,
    IN PVOID DisconnectInformation,
    IN ULONG DisconnectFlags)
{
    AFD_DbgPrint(MAX_TRACE, ("Called.\n"));

    return STATUS_SUCCESS;
}


NTSTATUS AfdEventReceive(
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket)
{
  return AfdpEventReceive(TdiEventContext,
    ConnectionContext,
    ReceiveFlags,
    BytesIndicated,
    BytesAvailable,
    BytesTaken,
    Tsdu,
    IoRequestPacket);
}


TDI_STATUS ClientEventReceiveExpedited(
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket)
{
  return AfdpEventReceive(TdiEventContext,
    ConnectionContext,
    ReceiveFlags,
    BytesIndicated,
    BytesAvailable,
    BytesTaken,
    Tsdu,
    IoRequestPacket);
}


NTSTATUS ClientEventChainedReceive(
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG ReceiveLength,
    IN ULONG StartingOffset,
    IN PMDL Tsdu,
    IN PVOID TsduDescriptor)
{
  AFD_DbgPrint(MAX_TRACE, ("Called.\n"));

  return STATUS_SUCCESS;
}


NTSTATUS AfdEventReceiveDatagramHandler(
  IN PVOID TdiEventContext,
  IN LONG SourceAddressLength,
  IN PVOID SourceAddress,
  IN LONG OptionsLength,
  IN PVOID Options,
  IN ULONG ReceiveDatagramFlags,
  IN ULONG BytesIndicated,
  IN ULONG BytesAvailable,
  OUT ULONG *BytesTaken,
  IN PVOID Tsdu,
  OUT PIRP *IoRequestPacket)
{
  PAFDFCB FCB = (PAFDFCB)TdiEventContext;
  PAFD_READ_REQUEST ReadRequest;
  PVOID ReceiveBuffer;
  PAFD_BUFFER Buffer;
  PLIST_ENTRY Entry;
  NTSTATUS Status;
  KIRQL OldIrql;
  ULONG Count;

  AFD_DbgPrint(MAX_TRACE, ("Called.\n"));

  AFD_DbgPrint(MID_TRACE, ("Receiving (%d of %d) bytes from (0x%X).\n",
    BytesIndicated, BytesAvailable, *(PULONG)SourceAddress));

  ReceiveBuffer = ExAllocatePool(NonPagedPool, BytesAvailable);
  if (!ReceiveBuffer)
    return STATUS_INSUFFICIENT_RESOURCES;

  /*Buffer = (PAFD_BUFFER)ExAllocateFromNPagedLookasideList(
    &BufferLookasideList);*/
  Buffer = (PAFD_BUFFER)ExAllocatePool(NonPagedPool, sizeof(AFD_BUFFER));
  if (!Buffer) {
    ExFreePool(ReceiveBuffer);
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  if (BytesIndicated != BytesAvailable)
    {
      AFD_DbgPrint(MIN_TRACE, ("WARNING: BytesIndicated != BytesAvailable.\n",
        BytesIndicated, BytesAvailable));
    }

  /* Copy the data to a local buffer */
  RtlCopyMemory(ReceiveBuffer, Tsdu, BytesIndicated);

  Buffer->Buffer.len = BytesIndicated;
  Buffer->Buffer.buf = ReceiveBuffer;

  ExInterlockedInsertTailList(
    &FCB->ReceiveQueue,
    &Buffer->ListEntry,
    &FCB->ReceiveQueueLock);

  KeAcquireSpinLock(&FCB->ReadRequestQueueLock, &OldIrql);

  if (!IsListEmpty(&FCB->ReadRequestQueue)) {
    AFD_DbgPrint(MAX_TRACE, ("Satisfying read request.\n"));

    Entry = RemoveHeadList(&FCB->ReceiveQueue);
    ReadRequest = CONTAINING_RECORD(Entry, AFD_READ_REQUEST, ListEntry);

    Status = FillWSABuffers(
      FCB,
      ReadRequest->RecvFrom.Request->Buffers,
      ReadRequest->RecvFrom.Request->BufferCount,
      &Count);
    ReadRequest->RecvFrom.Reply->NumberOfBytesRecvd = Count;
    ReadRequest->RecvFrom.Reply->Status = NO_ERROR;

    ReadRequest->Irp->IoStatus.Information = 0;
    ReadRequest->Irp->IoStatus.Status = Status;

    AFD_DbgPrint(MAX_TRACE, ("Completing IRP at (0x%X).\n", ReadRequest->Irp));

    IoCompleteRequest(ReadRequest->Irp, IO_NETWORK_INCREMENT);
  }

  KeReleaseSpinLock(&FCB->ReadRequestQueueLock, OldIrql);

  *BytesTaken = BytesIndicated;

  AFD_DbgPrint(MAX_TRACE, ("Leaving.\n"));

  return STATUS_SUCCESS;
}


NTSTATUS AfdRegisterEventHandlers(
    PAFDFCB FCB)
{
    NTSTATUS Status;

    assert(FCB->TdiAddressObject);

    /* Report errors for all types of sockets */
    Status = TdiSetEventHandler(FCB->TdiAddressObject,
        TDI_EVENT_ERROR,
        (PVOID)AfdEventError,
        (PVOID)FCB);
    if (!NT_SUCCESS(Status)) { return Status; }

    switch (FCB->SocketType) {
    case SOCK_STREAM:
        Status = TdiSetEventHandler(FCB->TdiAddressObject,
            TDI_EVENT_DISCONNECT,
            (PVOID)AfdEventDisconnect,
            (PVOID)FCB);
        if (!NT_SUCCESS(Status)) { return Status; }

        Status = TdiSetEventHandler(FCB->TdiAddressObject,
            TDI_EVENT_RECEIVE,
            (PVOID)AfdEventReceive,
            (PVOID)FCB);
        if (!NT_SUCCESS(Status)) { return Status; }

        Status = TdiSetEventHandler(FCB->TdiAddressObject,
            TDI_EVENT_RECEIVE_EXPEDITED,
            (PVOID)ClientEventReceiveExpedited,
            (PVOID)FCB);
        if (!NT_SUCCESS(Status)) { return Status; }

        Status = TdiSetEventHandler(FCB->TdiAddressObject,
            TDI_EVENT_CHAINED_RECEIVE,
            (PVOID)ClientEventChainedReceive,
            (PVOID)FCB);
        if (!NT_SUCCESS(Status)) { return Status; }

	Status = TdiSetEventHandler(FCB->TdiAddressObject,
				    TDI_EVENT_CONNECT,
				    AfdEventConnect,
				    (PVOID)FCB);
	if(!NT_SUCCESS(Status)) { return Status; }
        break;

    case SOCK_DGRAM:
    case SOCK_RAW:
        Status = TdiSetEventHandler(FCB->TdiAddressObject,
            TDI_EVENT_RECEIVE_DATAGRAM,
            (PVOID)AfdEventReceiveDatagramHandler,
            (PVOID)FCB);
        if (!NT_SUCCESS(Status)) { return Status; }
    }
    return STATUS_SUCCESS;
}


NTSTATUS AfdDeregisterEventHandlers(
    PAFDFCB FCB)
{
    NTSTATUS Status;

    Status = TdiSetEventHandler(FCB->TdiAddressObject,
        TDI_EVENT_ERROR,
        NULL,
        NULL);
    if (!NT_SUCCESS(Status)) { return Status; }

    switch (FCB->SocketType) {
    case SOCK_STREAM:
        Status = TdiSetEventHandler(FCB->TdiAddressObject,
            TDI_EVENT_DISCONNECT,
            NULL,
            NULL);
        if (!NT_SUCCESS(Status)) { return Status; }

        Status = TdiSetEventHandler(FCB->TdiAddressObject,
            TDI_EVENT_RECEIVE,
            NULL,
            NULL);
        if (!NT_SUCCESS(Status)) { return Status; }

        Status = TdiSetEventHandler(FCB->TdiAddressObject,
            TDI_EVENT_RECEIVE_EXPEDITED,
            NULL,
            NULL);
        if (!NT_SUCCESS(Status)) { return Status; }

        Status = TdiSetEventHandler(FCB->TdiAddressObject,
            TDI_EVENT_CHAINED_RECEIVE,
            NULL,
            NULL);
        if (!NT_SUCCESS(Status)) { return Status; }
        break;

    case SOCK_DGRAM:
    case SOCK_RAW:
        Status = TdiSetEventHandler(FCB->TdiAddressObject,
            TDI_EVENT_RECEIVE_DATAGRAM,
            NULL,
            NULL);
        if (!NT_SUCCESS(Status)) { return Status; }
    }
    return STATUS_SUCCESS;
}


/* EOF */
