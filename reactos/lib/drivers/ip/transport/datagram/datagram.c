/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        transport/datagram/datagram.c
 * PURPOSE:     Routines for sending and receiving datagrams
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"

VOID DGRemoveIRP(
    PADDRESS_FILE AddrFile,
    PIRP Irp)
{
    PLIST_ENTRY ListEntry;
    PDATAGRAM_RECEIVE_REQUEST ReceiveRequest;
    KIRQL OldIrql;

    TI_DbgPrint(MAX_TRACE, ("Called (Cancel IRP %08x for file %08x).\n",
                            Irp, AddrFile));

    KeAcquireSpinLock(&AddrFile->Lock, &OldIrql);

    for( ListEntry = AddrFile->ReceiveQueue.Flink; 
         ListEntry != &AddrFile->ReceiveQueue;
         ListEntry = ListEntry->Flink )
    {
        ReceiveRequest = CONTAINING_RECORD
            (ListEntry, DATAGRAM_RECEIVE_REQUEST, ListEntry);

        TI_DbgPrint(MAX_TRACE, ("Request: %08x?\n", ReceiveRequest));

        if (ReceiveRequest->Irp == Irp)
        {
            RemoveEntryList(&ReceiveRequest->ListEntry);
            exFreePool(ReceiveRequest);
            break;
        }
    }

    KeReleaseSpinLock(&AddrFile->Lock, OldIrql);

    TI_DbgPrint(MAX_TRACE, ("Done.\n"));
}

VOID DGDeliverData(
  PADDRESS_FILE AddrFile,
  PIP_ADDRESS SrcAddress,
  PIP_ADDRESS DstAddress,
  USHORT SrcPort,
  USHORT DstPort,
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

  TcpipAcquireSpinLock(&AddrFile->Lock, &OldIrql);

  if (AddrFile->Protocol == IPPROTO_UDP)
    {
      DataBuffer = IPPacket->Data;
    }
  else
    {
      /* Give client the IP header too if it is a raw IP file object */
      DataBuffer = IPPacket->Header;
    }

  if (!IsListEmpty(&AddrFile->ReceiveQueue))
    {
      PLIST_ENTRY CurrentEntry;
      PDATAGRAM_RECEIVE_REQUEST Current = NULL;
      BOOLEAN Found;
      PTA_IP_ADDRESS RTAIPAddress;

      TI_DbgPrint(MAX_TRACE, ("There is a receive request.\n"));

      /* Search receive request list to find a match */
      Found = FALSE;
      CurrentEntry = AddrFile->ReceiveQueue.Flink;
      while((CurrentEntry != &AddrFile->ReceiveQueue) && (!Found)) {
          Current = CONTAINING_RECORD(CurrentEntry, DATAGRAM_RECEIVE_REQUEST, ListEntry);

	  if( DstPort == AddrFile->Port &&
              (AddrIsEqual(DstAddress, &AddrFile->Address) ||
               AddrIsUnspecified(&AddrFile->Address) ||
               AddrIsUnspecified(DstAddress))) {
	      Found = TRUE;
	      /* Remove the request from the queue */
	      RemoveEntryList(&Current->ListEntry);
	      break;
	  } else {
	      CurrentEntry = CurrentEntry->Flink;
	  }
      }

      TcpipReleaseSpinLock(&AddrFile->Lock, OldIrql);

      if (Found)
        {
          TI_DbgPrint(MAX_TRACE, ("Suitable receive request found.\n"));

          TI_DbgPrint(MAX_TRACE,
                       ("Target Buffer: %x, Source Buffer: %x, Size %d\n",
                        Current->Buffer, DataBuffer, DataSize));

          if (DataSize > Current->BufferSize)
          {
              Current->Complete(Current->Context, STATUS_BUFFER_TOO_SMALL, DataSize);
              return;
          }

          /* Copy the data into buffer provided by the user */
	  RtlCopyMemory( Current->Buffer,
			 DataBuffer,
			 DataSize );

	  RTAIPAddress = (PTA_IP_ADDRESS)Current->ReturnInfo->RemoteAddress;
	  RTAIPAddress->TAAddressCount = 1;
	  RTAIPAddress->Address->AddressType = TDI_ADDRESS_TYPE_IP;
	  RTAIPAddress->Address->Address->sin_port = SrcPort;

	  TI_DbgPrint(MAX_TRACE, ("(A: %08x) Addr %08x Port %04x\n",
				  RTAIPAddress,
				  SrcAddress->Address.IPv4Address, SrcPort));

	  RtlCopyMemory( &RTAIPAddress->Address->Address->in_addr,
			 &SrcAddress->Address.IPv4Address,
			 sizeof(SrcAddress->Address.IPv4Address) );

          /* Complete the receive request */
          Current->Complete(Current->Context, STATUS_SUCCESS, DataSize);
        }
    }
  else if (AddrFile->RegisteredReceiveDatagramHandler)
    {
      TI_DbgPrint(MAX_TRACE, ("Calling receive event handler.\n"));

      ReceiveHandler = AddrFile->ReceiveDatagramHandler;
      HandlerContext = AddrFile->ReceiveDatagramHandlerContext;

      TcpipReleaseSpinLock(&AddrFile->Lock, OldIrql);

      if (SrcAddress->Type == IP_ADDRESS_V4)
        {
          AddressLength = sizeof(IPv4_RAW_ADDRESS);
          SourceAddress = &SrcAddress->Address.IPv4Address;
        }
      else /* (Address->Type == IP_ADDRESS_V6) */
        {
          AddressLength = sizeof(IPv6_RAW_ADDRESS);
          SourceAddress = SrcAddress->Address.IPv6Address;
        }

      Status = (*ReceiveHandler)(HandlerContext,
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
    }
  else
    {
      TcpipReleaseSpinLock(&AddrFile->Lock, OldIrql);
      TI_DbgPrint(MAX_TRACE, ("Discarding datagram.\n"));
    }

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}


VOID DGReceiveComplete(PVOID Context, NTSTATUS Status, ULONG Count) {
    PDATAGRAM_RECEIVE_REQUEST ReceiveRequest =
	(PDATAGRAM_RECEIVE_REQUEST)Context;
    TI_DbgPrint(MAX_TRACE,("Called (%08x:%08x)\n", Status, Count));
    ReceiveRequest->UserComplete( ReceiveRequest->UserContext, Status, Count );
    exFreePool( ReceiveRequest );
    TI_DbgPrint(MAX_TRACE,("Done\n"));
}

NTSTATUS DGReceiveDatagram(
    PADDRESS_FILE AddrFile,
    PTDI_CONNECTION_INFORMATION ConnInfo,
    PCHAR BufferData,
    ULONG ReceiveLength,
    ULONG ReceiveFlags,
    PTDI_CONNECTION_INFORMATION ReturnInfo,
    PULONG BytesReceived,
    PDATAGRAM_COMPLETION_ROUTINE Complete,
    PVOID Context,
    PIRP Irp)
/*
 * FUNCTION: Attempts to receive an DG datagram from a remote address
 * ARGUMENTS:
 *     Request       = Pointer to TDI request
 *     ConnInfo      = Pointer to connection information
 *     Buffer        = Pointer to NDIS buffer chain to store received data
 *     ReceiveLength = Maximum size to use of buffer, 0 if all can be used
 *     ReceiveFlags  = Receive flags (None, Normal, Peek)
 *     ReturnInfo    = Pointer to structure for return information
 *     BytesReceive  = Pointer to structure for number of bytes received
 * RETURNS:
 *     Status of operation
 * NOTES:
 *     This is the high level interface for receiving DG datagrams
 */
{
    KIRQL OldIrql;
    NTSTATUS Status;
    PDATAGRAM_RECEIVE_REQUEST ReceiveRequest;

    TI_DbgPrint(MAX_TRACE, ("Called.\n"));

    TcpipAcquireSpinLock(&AddrFile->Lock, &OldIrql);

    if (AF_IS_VALID(AddrFile))
    {
	ReceiveRequest = exAllocatePool(NonPagedPool, sizeof(DATAGRAM_RECEIVE_REQUEST));
	if (ReceiveRequest)
        {
	    /* Initialize a receive request */

	    /* Extract the remote address filter from the request (if any) */
	    if ((ConnInfo->RemoteAddressLength != 0) &&
		(ConnInfo->RemoteAddress))
            {
		Status = AddrGetAddress(ConnInfo->RemoteAddress,
					&ReceiveRequest->RemoteAddress,
					&ReceiveRequest->RemotePort);
		if (!NT_SUCCESS(Status))
                {
		    TcpipReleaseSpinLock(&AddrFile->Lock, OldIrql);
		    exFreePool(ReceiveRequest);
		    return Status;
                }
            }
	    else
            {
		ReceiveRequest->RemotePort    = 0;
            }

	    IoMarkIrpPending(Irp);

	    ReceiveRequest->ReturnInfo = ReturnInfo;
	    ReceiveRequest->Buffer = BufferData;
	    ReceiveRequest->BufferSize = ReceiveLength;
	    ReceiveRequest->UserComplete = Complete;
	    ReceiveRequest->UserContext = Context;
	    ReceiveRequest->Complete =
		(PDATAGRAM_COMPLETION_ROUTINE)DGReceiveComplete;
	    ReceiveRequest->Context = ReceiveRequest;
            ReceiveRequest->AddressFile = AddrFile;
            ReceiveRequest->Irp = Irp;

	    /* Queue receive request */
	    InsertTailList(&AddrFile->ReceiveQueue, &ReceiveRequest->ListEntry);
	    AF_SET_PENDING(AddrFile, AFF_RECEIVE);

	    TcpipReleaseSpinLock(&AddrFile->Lock, OldIrql);

	    TI_DbgPrint(MAX_TRACE, ("Leaving (pending %08x).\n", ReceiveRequest));

	    return STATUS_PENDING;
        }
	else
        {
	    Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
	Status = STATUS_INVALID_ADDRESS;
    }

    TcpipReleaseSpinLock(&AddrFile->Lock, OldIrql);

    TI_DbgPrint(MAX_TRACE, ("Leaving with errors (0x%X).\n", Status));

    return Status;
}
