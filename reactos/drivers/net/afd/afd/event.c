
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

NTSTATUS AfdpEventReceive
( IN PVOID TdiEventContext,
  IN CONNECTION_CONTEXT ConnectionContext,
  IN ULONG ReceiveFlags,
  IN ULONG BytesIndicated,
  IN ULONG BytesAvailable,
  OUT ULONG *BytesTaken,
  IN PVOID Tsdu,
  OUT PIRP *IoRequestPacket ) {
    PAFDFCB FCB = (PAFDFCB)TdiEventContext;
    PVOID ReceiveBuffer;
    PAFD_BUFFER Buffer;
    
    AFD_DbgPrint(MAX_TRACE, ("Called.\n"));
    
    AFD_DbgPrint(MID_TRACE, ("Receiving (%d of %d) bytes on (0x%X).\n",
			     BytesIndicated, BytesAvailable, ConnectionContext));
    
    ReceiveBuffer = ExAllocatePool(NonPagedPool, BytesAvailable);
    if (!ReceiveBuffer)
	return STATUS_INSUFFICIENT_RESOURCES;
    
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
    Buffer->ConsumedThisBuffer = 0;
    
    AFD_DbgPrint(MAX_TRACE,("Receive Queue: FLINK %x, BLINK %x\n",
			    FCB->ReceiveQueue.Flink,
			    FCB->ReceiveQueue.Blink));
    AFD_DbgPrint(MAX_TRACE,("Buffer: %x\n", Buffer));

    ExInterlockedInsertTailList(
	&FCB->ReceiveQueue,
	&Buffer->ListEntry,
	&FCB->ReceiveQueueLock);
    
    *BytesTaken = BytesIndicated;

    RegisterFCBForWork( FCB ); /* Will try to do stuff to FCB later */
    
    return STATUS_SUCCESS;
}

NTSTATUS AfdpEventDisconnect( 
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN LONG DisconnectDataLength,
    IN PVOID DisconnectData,
    IN LONG DisconnectInformationLength,
    IN PVOID DisconnectInformation,
    IN ULONG DisconnectFlags ) {
    PAFDFCB FCB = (PAFDFCB)TdiEventContext;
    if( DisconnectFlags == TDI_DISCONNECT_RELEASE ) {
	if( FCB->State == SOCKET_STATE_CONNECTED || 
	    FCB->State == SOCKET_STATE_SHUTDOWN_REMOTE ) 
	    FCB->State = SOCKET_STATE_SHUTDOWN_REMOTE;
    } else if( FCB->State == SOCKET_STATE_CONNECTED )
	FCB->State = SOCKET_STATE_SHUTDOWN;

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

    AFD_DbgPrint(MAX_TRACE, ("Connecting socket\n"));

    FCB->State = SOCKET_STATE_CONNECTED;
    RegisterFCBForWork( FCB ); /* Will try to do stuff to FCB later */

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
    return AfdpEventDisconnect( TdiEventContext,
				ConnectionContext,
				DisconnectDataLength,
				DisconnectData,
				DisconnectInformationLength,
				DisconnectInformation,
				DisconnectFlags );
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
  PVOID ReceiveBuffer;
  PAFD_BUFFER Buffer;

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

  *BytesTaken = BytesIndicated;
  RegisterFCBForWork( FCB );

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
