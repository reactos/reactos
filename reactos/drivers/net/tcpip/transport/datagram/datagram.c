/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        transport/datagram/datagram.c
 * PURPOSE:     Routines for sending and receiving datagrams
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <tcpip.h>
#include <datagram.h>
#include <routines.h>
#include <transmit.h>
#include <address.h>
#include <route.h>
#include <pool.h>


/* Pending request queue */
LIST_ENTRY DGPendingListHead;
KSPIN_LOCK DGPendingListLock;
/* Work queue item for pending requests */
WORK_QUEUE_ITEM DGWorkItem;


VOID DatagramWorker(
  PVOID Context)
/*
 * FUNCTION: Handles pending send requests
 * ARGUMENTS:
 *     Context = Pointer to context information (unused)
 * NOTES:
 *     This routine is called after the driver has run out of resources.
 *     It processes send requests or shedules them to be processed
 */
{
  PLIST_ENTRY CurrentADFEntry;
  PLIST_ENTRY CurrentSREntry;
  PADDRESS_FILE CurrentADF;
  PDATAGRAM_SEND_REQUEST CurrentSR;
  KIRQL OldIrql1;
  KIRQL OldIrql2;

  TI_DbgPrint(MAX_TRACE, ("Called.\n"));

  KeAcquireSpinLock(&DGPendingListLock, &OldIrql1);

  CurrentADFEntry = DGPendingListHead.Flink;
  while (CurrentADFEntry != &DGPendingListHead) {
    RemoveEntryList(CurrentADFEntry);
    CurrentADF = CONTAINING_RECORD(CurrentADFEntry,
                                   ADDRESS_FILE,
                                   ListEntry);

    KeAcquireSpinLock(&CurrentADF->Lock, &OldIrql2);

    if (AF_IS_BUSY(CurrentADF)) {
      /* The send worker function is already running so we just
         set the pending send flag on the address file object */

      AF_SET_PENDING(CurrentADF, AFF_SEND);
      KeReleaseSpinLock(&CurrentADF->Lock, OldIrql2);
    } else {
      if (!IsListEmpty(&CurrentADF->TransmitQueue)) {
        /* The transmit queue is not empty. Dequeue a send
           request and process it */

        CurrentSREntry = RemoveHeadList(&CurrentADF->TransmitQueue);
        CurrentSR      = CONTAINING_RECORD(CurrentADFEntry,
                                           DATAGRAM_SEND_REQUEST,
                                           ListEntry);

        KeReleaseSpinLock(&CurrentADF->Lock, OldIrql2);

        DGSend(CurrentADF, CurrentSR);
      } else
        KeReleaseSpinLock(&CurrentADF->Lock, OldIrql2);
    }
    CurrentADFEntry = CurrentADFEntry->Flink;
  }

  KeReleaseSpinLock(&DGPendingListLock, OldIrql1);

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}


VOID SendDatagramComplete(
  PVOID Context,
  PNDIS_PACKET Packet,
  NDIS_STATUS NdisStatus)
/*
 * FUNCTION: Datagram transmit completion handler
 * ARGUMENTS:
 *     Context    = Pointer to context infomation (DATAGRAM_SEND_REQUEST)
 *     Packet     = Pointer to NDIS packet
 *     NdisStatus = Status of transmit operation
 * NOTES:
 *     This routine is called by IP when a datagram send completes.
 *     We shedule the out-of-resource worker function if there
 *     are pending address files in the queue
 */
{
  KIRQL OldIrql;
  ULONG BytesSent;
  PVOID CompleteContext;
  PNDIS_BUFFER NdisBuffer;
  PDATAGRAM_SEND_REQUEST SendRequest;
  DATAGRAM_COMPLETION_ROUTINE Complete;
  BOOLEAN QueueWorkItem;

  TI_DbgPrint(MAX_TRACE, ("Called.\n"));

  SendRequest     = (PDATAGRAM_SEND_REQUEST)Context;
  Complete        = SendRequest->Complete;
  CompleteContext = SendRequest->Context;
  BytesSent       = SendRequest->BufferSize;

  /* Remove data buffer before releasing memory for packet buffers */
  NdisQueryPacket(Packet, NULL, NULL, &NdisBuffer, NULL);
  NdisUnchainBufferAtBack(Packet, &NdisBuffer);
  FreeNdisPacket(Packet);
  DereferenceObject(SendRequest->RemoteAddress);
  ExFreePool(SendRequest);

  /* If there are pending send requests, shedule worker function */
  KeAcquireSpinLock(&DGPendingListLock, &OldIrql);
  QueueWorkItem = (!IsListEmpty(&DGPendingListHead));
  KeReleaseSpinLock(&DGPendingListLock, OldIrql);
  if (QueueWorkItem)
    ExQueueWorkItem(&DGWorkItem, CriticalWorkQueue);

  TI_DbgPrint(MAX_TRACE, ("Calling 0x%X.\n", Complete));

  /* Call completion routine for send request */
  (*Complete)(CompleteContext, NdisStatus, BytesSent);

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}


VOID DGSend(
  PVOID Context,
  PDATAGRAM_SEND_REQUEST SendRequest)
/*
 * FUNCTION: Sends a datagram to IP layer
 * ARGUMENTS:
 *     Context     = Pointer to context information (ADDRESS_FILE)
 *     SendRequest = Pointer to send request
 */
{
  KIRQL OldIrql;
  NTSTATUS Status;
  USHORT LocalPort;
  PIP_PACKET IPPacket;
  PROUTE_CACHE_NODE RCN;
  PLIST_ENTRY CurrentEntry;
  PADDRESS_FILE AddrFile = Context;
  PADDRESS_ENTRY ADE;

  TI_DbgPrint(MAX_TRACE, ("Called.\n"));

  /* Get the information we need from the address file
     now so we minimize the time we hold the spin lock */
  KeAcquireSpinLock(&AddrFile->Lock, &OldIrql);
  LocalPort = AddrFile->Port;
  ADE       = AddrFile->ADE;
  ReferenceObject(ADE);
  KeReleaseSpinLock(&AddrFile->Lock, OldIrql);

  /* Loop until there are no more send requests in the
     transmit queue or until we run out of resources */
  for (;;) {
    Status = (*SendRequest->Build)(SendRequest, ADE->Address, LocalPort, &IPPacket);
    if (!NT_SUCCESS(Status)) {
      KeAcquireSpinLock(&AddrFile->Lock, &OldIrql);
      /* An error occurred, enqueue the send request again and return */
      InsertTailList(&AddrFile->TransmitQueue, &SendRequest->ListEntry);
      DereferenceObject(ADE);
      KeReleaseSpinLock(&AddrFile->Lock, OldIrql);

      TI_DbgPrint(MIN_TRACE, ("Leaving (insufficient resources).\n"));
      return;
    }

    /* Get a route to the destination address */
    if (RouteGetRouteToDestination(SendRequest->RemoteAddress, ADE->NTE, &RCN) == IP_SUCCESS) {
      /* Set completion routine and send the packet */
      PC(IPPacket->NdisPacket)->Complete = SendDatagramComplete;
      PC(IPPacket->NdisPacket)->Context  = SendRequest;
      if (IPSendDatagram(IPPacket, RCN) != STATUS_SUCCESS)
          SendDatagramComplete(
            SendRequest,
            IPPacket->NdisPacket,
            NDIS_STATUS_REQUEST_ABORTED);
      /* We're done with the RCN */
      DereferenceObject(RCN);
    } else {
      /* No route to destination */
      /* FIXME: Which error code should we use here? */
      TI_DbgPrint(MIN_TRACE, ("No route to destination address (0x%X).\n",
          SendRequest->RemoteAddress->Address.IPv4Address));
      SendDatagramComplete(
        SendRequest,
        IPPacket->NdisPacket,
        NDIS_STATUS_REQUEST_ABORTED);
    }

    (*IPPacket->Free)(IPPacket);

    /* Check transmit queue for more to send */

    KeAcquireSpinLock(&AddrFile->Lock, &OldIrql);

    if (!IsListEmpty(&AddrFile->TransmitQueue)) {
      /* Transmit queue is not empty, process one more request */
      CurrentEntry = RemoveHeadList(&AddrFile->TransmitQueue);
      SendRequest  = CONTAINING_RECORD(CurrentEntry, DATAGRAM_SEND_REQUEST, ListEntry);

      KeReleaseSpinLock(&AddrFile->Lock, OldIrql);
    } else {
      /* Transmit queue is empty */
      AF_CLR_PENDING(AddrFile, AFF_SEND);
      DereferenceObject(ADE);
      KeReleaseSpinLock(&AddrFile->Lock, OldIrql);

      TI_DbgPrint(MAX_TRACE, ("Leaving (empty queue).\n"));
      return;
    }
  }
}


VOID DGDeliverData(
  PADDRESS_FILE AddrFile,
  PIP_ADDRESS Address,
  PIP_PACKET IPPacket,
  UINT DataSize)
/*
 * FUNCTION: Delivers datagram data to a user
 * ARGUMENTS:
 *     AddrFile = Address file to deliver data to
 *     Address  = Remote address the packet came from
 *     IPPacket = Pointer to IP packet to deliver
 *     DataSize = Number of bytes in data area
 *                (incl. IP header for raw IP file objects)
 * NOTES:
 *     If there is a receive request, then we copy the data to the
 *     buffer supplied by the user and complete the receive request.
 *     If no suitable receive request exists, then we call the event
 *     handler if it exists, otherwise we drop the packet.
 */
{
  KIRQL OldIrql;
  PTDI_IND_RECEIVE_DATAGRAM ReceiveHandler;
  PVOID HandlerContext;
  LONG AddressLength;
  PVOID SourceAddress;
  ULONG BytesTaken;
  NTSTATUS Status;
  PVOID DataBuffer;

  TI_DbgPrint(MAX_TRACE, ("Called.\n"));

  KeAcquireSpinLock(&AddrFile->Lock, &OldIrql);

  if (AddrFile->Protocol == IPPROTO_UDP) {
    DataBuffer = IPPacket->Data;
  } else {
    /* Give client the IP header too if it is a raw IP file object */
    DataBuffer = IPPacket->Header;
  }

  if (!IsListEmpty(&AddrFile->ReceiveQueue)) {
    PLIST_ENTRY CurrentEntry;
    PDATAGRAM_RECEIVE_REQUEST Current;
    BOOLEAN Found;

    TI_DbgPrint(MAX_TRACE, ("There is a receive request.\n"));

    /* Search receive request list to find a match */
    Found = FALSE;
    CurrentEntry = AddrFile->ReceiveQueue.Flink;
    while ((CurrentEntry != &AddrFile->ReceiveQueue) && (!Found)) {
      Current = CONTAINING_RECORD(CurrentEntry, DATAGRAM_RECEIVE_REQUEST, ListEntry);
      if (!Current->RemoteAddress)
        Found = TRUE;
      else if (AddrIsEqual(Address, Current->RemoteAddress))
        Found = TRUE;

      if (Found) {
        /* FIXME: Maybe we should check if the buffer of this
           receive request is large enough and if not, search
           for another */

        /* Remove the request from the queue */
        RemoveEntryList(&Current->ListEntry);
        AddrFile->RefCount--;
        break;
      }
      CurrentEntry = CurrentEntry->Flink;
    }

    KeReleaseSpinLock(&AddrFile->Lock, OldIrql);

    if (Found) {
      TI_DbgPrint(MAX_TRACE, ("Suitable receive request found.\n"));

      /* Copy the data into buffer provided by the user */
      CopyBufferToBufferChain(
        Current->Buffer,
        0,
        DataBuffer,
        DataSize);

      /* Complete the receive request */
      (*Current->Complete)(Current->Context, STATUS_SUCCESS, DataSize);

      /* Finally free the receive request */
      if (Current->RemoteAddress)
        DereferenceObject(Current->RemoteAddress);
      ExFreePool(Current);
    }
  } else if (AddrFile->RegisteredReceiveDatagramHandler) {
    TI_DbgPrint(MAX_TRACE, ("Calling receive event handler.\n"));

    ReceiveHandler = AddrFile->ReceiveDatagramHandler;
    HandlerContext = AddrFile->ReceiveDatagramHandlerContext;

    KeReleaseSpinLock(&AddrFile->Lock, OldIrql);

    if (Address->Type == IP_ADDRESS_V4) {
      AddressLength = sizeof(IPv4_RAW_ADDRESS);
      SourceAddress = &Address->Address.IPv4Address;
    } else /* (Address->Type == IP_ADDRESS_V6) */ {
      AddressLength = sizeof(IPv6_RAW_ADDRESS);
      SourceAddress = Address->Address.IPv6Address;
    }

    Status = (*ReceiveHandler)(
      HandlerContext,
      AddressLength,
      SourceAddress,
      0,
      NULL,
      TDI_RECEIVE_ENTIRE_MESSAGE,
      DataSize,
      DataSize,
      &BytesTaken,
      DataBuffer,
      NULL);
  } else {
    TI_DbgPrint(MAX_TRACE, ("Discarding datagram.\n"));
  }

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}


VOID DGCancelSendRequest(
  PADDRESS_FILE AddrFile,
  PVOID Context)
/*
 * FUNCTION: Cancels a datagram send request
 * ARGUMENTS:
 *     AddrFile = Pointer to address file of the request
 *     Context  = Pointer to context information for completion handler
 */
{
  KIRQL OldIrql;
  PLIST_ENTRY CurrentEntry;
  PDATAGRAM_SEND_REQUEST Current = NULL;
  BOOLEAN Found = FALSE;

  TI_DbgPrint(MAX_TRACE, ("Called.\n"));

  KeAcquireSpinLock(&AddrFile->Lock, &OldIrql);

  /* Search the request list for the specified request and remove it */
  CurrentEntry = AddrFile->TransmitQueue.Flink;
  while ((CurrentEntry != &AddrFile->TransmitQueue) && (!Found)) {
	  Current = CONTAINING_RECORD(CurrentEntry, DATAGRAM_SEND_REQUEST, ListEntry);
    if (Context == Current->Context) {
      /* We've found the request, now remove it from the queue */
      RemoveEntryList(CurrentEntry);
      AddrFile->RefCount--;
      Found = TRUE;
      break;
    }
    CurrentEntry = CurrentEntry->Flink;
  }

  KeReleaseSpinLock(&AddrFile->Lock, OldIrql);

  if (Found) {
    /* Complete the request and free its resources */
    (*Current->Complete)(Current->Context, STATUS_CANCELLED, 0);
    DereferenceObject(Current->RemoteAddress);
    ExFreePool(Current);
  } else {
    TI_DbgPrint(MID_TRACE, ("Cannot find send request.\n"));
  }
}


VOID DGCancelReceiveRequest(
  PADDRESS_FILE AddrFile,
  PVOID Context)
/*
 * FUNCTION: Cancels a datagram receive request
 * ARGUMENTS:
 *     AddrFile = Pointer to address file of the request
 *     Context  = Pointer to context information for completion handler
 */
{
  KIRQL OldIrql;
  PLIST_ENTRY CurrentEntry;
  PDATAGRAM_RECEIVE_REQUEST Current = NULL;
  BOOLEAN Found = FALSE;

  TI_DbgPrint(MAX_TRACE, ("Called.\n"));

  KeAcquireSpinLock(&AddrFile->Lock, &OldIrql);

  /* Search the request list for the specified request and remove it */
  CurrentEntry = AddrFile->ReceiveQueue.Flink;
  while ((CurrentEntry != &AddrFile->ReceiveQueue) && (!Found)) {
	  Current = CONTAINING_RECORD(CurrentEntry, DATAGRAM_RECEIVE_REQUEST, ListEntry);
    if (Context == Current->Context) {
      /* We've found the request, now remove it from the queue */
      RemoveEntryList(CurrentEntry);
      AddrFile->RefCount--;
      Found = TRUE;
      break;
    }
    CurrentEntry = CurrentEntry->Flink;
  }

  KeReleaseSpinLock(&AddrFile->Lock, OldIrql);

  if (Found) {
    /* Complete the request and free its resources */
    (*Current->Complete)(Current->Context, STATUS_CANCELLED, 0);
    /* Remote address can be NULL if the caller wants to receive
       packets sent from any address */
    if (Current->RemoteAddress)
      DereferenceObject(Current->RemoteAddress);
    ExFreePool(Current);
  } else {
    TI_DbgPrint(MID_TRACE, ("Cannot find receive request.\n"));
  }
}


NTSTATUS DGTransmit(
  PADDRESS_FILE AddressFile,
  PDATAGRAM_SEND_REQUEST SendRequest)
/*
 * FUNCTION: Transmits or queues a send request
 * ARGUMENTS:
 *     AddressFile = Pointer to address file
 *     SendRequest = Pointer to send request
 * RETURNS:
 *     Status of operation
 */
{
  KIRQL OldIrql;
CP
  KeAcquireSpinLock(&AddressFile->Lock, &OldIrql);
CP
  if (AF_IS_BUSY(AddressFile)) {
CP
    /* Queue send request on the transmit queue */
    InsertTailList(&AddressFile->TransmitQueue, &SendRequest->ListEntry);
CP
    /* Reference address file and set pending send request flag */
    ReferenceObject(AddressFile);
CP
    AF_SET_PENDING(AddressFile, AFF_SEND);
CP
    KeReleaseSpinLock(&AddressFile->Lock, OldIrql);
CP
    TI_DbgPrint(MAX_TRACE, ("Leaving (queued).\n"));
  } else {
CP
    KeReleaseSpinLock(&AddressFile->Lock, OldIrql);
CP
    /* Send the datagram */
    DGSend(AddressFile, SendRequest);
CP
    TI_DbgPrint(MAX_TRACE, ("Leaving (pending).\n"));
  }
CP
  return STATUS_PENDING;
}

NTSTATUS DGSendDatagram(
  PTDI_REQUEST Request,
  PTDI_CONNECTION_INFORMATION ConnInfo,
  PNDIS_BUFFER Buffer,
  ULONG DataSize,
  DATAGRAM_BUILD_ROUTINE Build)
/*
 * FUNCTION: Sends a datagram to a remote address
 * ARGUMENTS:
 *     Request   = Pointer to TDI request
 *     ConnInfo  = Pointer to connection information
 *     Buffer    = Pointer to NDIS buffer with data
 *     DataSize  = Size in bytes of data to be sent
 *     Build     = Pointer to datagram build routine
 * RETURNS:
 *     Status of operation
 */
{
  PADDRESS_FILE AddrFile;
  KIRQL OldIrql;
  NTSTATUS Status;
  PDATAGRAM_SEND_REQUEST SendRequest;

  TI_DbgPrint(MAX_TRACE, ("Called.\n"));

  AddrFile = Request->Handle.AddressHandle;

  KeAcquireSpinLock(&AddrFile->Lock, &OldIrql);

  if (AF_IS_VALID(AddrFile)) {
    /* Initialize a send request */
    Status = BuildDatagramSendRequest(
      &SendRequest,
      NULL,
      0,
      Buffer,
      DataSize,
      Request->RequestNotifyObject,
      Request->RequestContext,
      Build,
      0);
    if (NT_SUCCESS(Status)) {
      Status = AddrGetAddress(
        ConnInfo->RemoteAddress,
        &SendRequest->RemoteAddress,
        &SendRequest->RemotePort,
        &AddrFile->AddrCache);
      if (NT_SUCCESS(Status)) {
        KeReleaseSpinLock(&AddrFile->Lock, OldIrql);
        return DGTransmit(AddrFile, SendRequest);
      } else {
        ExFreePool(SendRequest);
      }
    } else
      Status = STATUS_INSUFFICIENT_RESOURCES;
  } else
    Status = STATUS_ADDRESS_CLOSED;

  KeReleaseSpinLock(&AddrFile->Lock, OldIrql);

  TI_DbgPrint(MAX_TRACE, ("Leaving. Status (0x%X)\n", Status));

  return Status;
}


NTSTATUS DGReceiveDatagram(
  PTDI_REQUEST Request,
  PTDI_CONNECTION_INFORMATION ConnInfo,
  PNDIS_BUFFER Buffer,
  ULONG ReceiveLength,
  ULONG ReceiveFlags,
  PTDI_CONNECTION_INFORMATION ReturnInfo,
  PULONG BytesReceived)
/*
 * FUNCTION: Attempts to receive a datagram from a remote address
 * ARGUMENTS:
 *     Request       = Pointer to TDI request
 *     ConnInfo      = Pointer to connection information
 *     Buffer        = Pointer to NDIS buffer chain to store received data
 *     ReceiveLength = Maximum size to use of buffer (0 if all can be used)
 *     ReceiveFlags  = Receive flags (None, Normal, Peek)
 *     ReturnInfo    = Pointer to structure for return information
 *     BytesReceive  = Pointer to structure for number of bytes received
 * RETURNS:
 *     Status of operation
 * NOTES:
 *     This is the high level interface for receiving datagrams
 */
{
  PADDRESS_FILE AddrFile;
  KIRQL OldIrql;
  NTSTATUS Status;
  PDATAGRAM_RECEIVE_REQUEST ReceiveRequest;

  TI_DbgPrint(MAX_TRACE, ("Called.\n"));

  AddrFile = Request->Handle.AddressHandle;

  KeAcquireSpinLock(&AddrFile->Lock, &OldIrql);

  if (AF_IS_VALID(AddrFile)) {
    ReceiveRequest = ExAllocatePool(NonPagedPool, sizeof(DATAGRAM_RECEIVE_REQUEST));
    if (ReceiveRequest) {
      /* Initialize a receive request */

      /* Extract the remote address filter from the request (if any) */
      if (((ConnInfo->RemoteAddressLength != 0)) && (ConnInfo->RemoteAddress)) {
        Status = AddrGetAddress(ConnInfo->RemoteAddress,
          &ReceiveRequest->RemoteAddress,
          &ReceiveRequest->RemotePort,
          &AddrFile->AddrCache);
        if (!NT_SUCCESS(Status)) {
          KeReleaseSpinLock(&AddrFile->Lock, OldIrql);
          ExFreePool(ReceiveRequest);
          return Status;
        }
      } else {
        ReceiveRequest->RemotePort    = 0;
        ReceiveRequest->RemoteAddress = NULL;
      }
      ReceiveRequest->ReturnInfo = ReturnInfo;
      ReceiveRequest->Buffer     = Buffer;
      /* If ReceiveLength is 0, the whole buffer is available to us */
      ReceiveRequest->BufferSize = (ReceiveLength == 0) ?
        MmGetMdlByteCount(Buffer) : ReceiveLength;
      ReceiveRequest->Complete   = Request->RequestNotifyObject;
      ReceiveRequest->Context    = Request->RequestContext;

      /* Queue receive request */
      InsertTailList(&AddrFile->ReceiveQueue, &ReceiveRequest->ListEntry);

      /* Reference address file and set pending receive request flag */
      AddrFile->RefCount++;
      AF_SET_PENDING(AddrFile, AFF_RECEIVE);

      KeReleaseSpinLock(&AddrFile->Lock, OldIrql);

      TI_DbgPrint(MAX_TRACE, ("Leaving (pending).\n"));

      return STATUS_PENDING;
    } else
      Status = STATUS_INSUFFICIENT_RESOURCES;
  } else
    Status = STATUS_INVALID_ADDRESS;

  KeReleaseSpinLock(&AddrFile->Lock, OldIrql);

  TI_DbgPrint(MAX_TRACE, ("Leaving with errors (0x%X).\n", Status));

  return Status;
}


NTSTATUS DGStartup(
  VOID)
/*
 * FUNCTION: Initializes the datagram subsystem
 * RETURNS:
 *     Status of operation
 */
{
  InitializeListHead(&DGPendingListHead);

  KeInitializeSpinLock(&DGPendingListLock);

  ExInitializeWorkItem(&DGWorkItem, DatagramWorker, NULL);

  return STATUS_SUCCESS;
}


NTSTATUS DGShutdown(
  VOID)
/*
 * FUNCTION: Shuts down the datagram subsystem
 * RETURNS:
 *     Status of operation
 */
{
  return STATUS_SUCCESS;
}


/* EOF */
