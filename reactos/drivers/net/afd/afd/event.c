/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver
 * FILE:        afd/event.c
 * PURPOSE:     TDI event handlers
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */
#include <afd.h>

NTSTATUS AfdEventError(
    IN PVOID TdiEventContext,
    IN NTSTATUS Status)
{
    AFD_DbgPrint(MAX_TRACE, ("Called.\n"));

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
    AFD_DbgPrint(MAX_TRACE, ("Called.\n"));

    return STATUS_SUCCESS;
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
    AFD_DbgPrint(MAX_TRACE, ("Called.\n"));

    return STATUS_SUCCESS;
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

  AFD_DbgPrint(MAX_TRACE, ("Called.\n"));

  AFD_DbgPrint(MID_TRACE, ("Receiving (%d) bytes from (0x%X).\n",
    BytesAvailable, *(PULONG)SourceAddress));

  ReceiveBuffer = ExAllocatePool(NonPagedPool, BytesAvailable);
  if (!ReceiveBuffer) {
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  /*Buffer = (PAFD_BUFFER)ExAllocateFromNPagedLookasideList(
    &BufferLookasideList);*/
  Buffer = (PAFD_BUFFER)ExAllocatePool(NonPagedPool, sizeof(AFD_BUFFER));
  if (!Buffer) {
    ExFreePool(ReceiveBuffer);
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  /* Copy the data to a local buffer */
  RtlCopyMemory(ReceiveBuffer, Tsdu, BytesAvailable);

  Buffer->Buffer.len = BytesAvailable;
  Buffer->Buffer.buf = ReceiveBuffer;

  ExInterlockedInsertTailList(
    &FCB->ReceiveQueue,
    &Buffer->ListEntry,
    &FCB->ReceiveQueueLock);

  KeAcquireSpinLock(&FCB->ReadRequestQueueLock, &OldIrql);

  if (!IsListEmpty(&FCB->ReadRequestQueue)) {
    Entry = RemoveHeadList(&FCB->ReceiveQueue);
    ReadRequest = CONTAINING_RECORD(Entry, AFD_READ_REQUEST, ListEntry);

    Status = FillWSABuffers(
      FCB,
      ReadRequest->RecvFromRequest->Buffers,
      ReadRequest->RecvFromRequest->BufferCount,
      &ReadRequest->RecvFromReply->NumberOfBytesRecvd);
    ReadRequest->RecvFromReply->Status = NO_ERROR;

    ReadRequest->Irp->IoStatus.Information = 0;
    ReadRequest->Irp->IoStatus.Status = Status;
    IoCompleteRequest(ReadRequest->Irp, IO_NETWORK_INCREMENT);
  }

  KeReleaseSpinLock(&FCB->ReadRequestQueueLock, OldIrql);

  *BytesTaken = BytesAvailable;

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
