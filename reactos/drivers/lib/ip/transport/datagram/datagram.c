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
      PDATAGRAM_RECEIVE_REQUEST Current;
      BOOLEAN Found;
  
      TI_DbgPrint(MAX_TRACE, ("There is a receive request.\n"));
  
      /* Search receive request list to find a match */
      Found = FALSE;
      CurrentEntry = AddrFile->ReceiveQueue.Flink;
      while ((CurrentEntry != &AddrFile->ReceiveQueue) && (!Found))
        {
          Current = CONTAINING_RECORD(CurrentEntry, DATAGRAM_RECEIVE_REQUEST, ListEntry);
	  if (AddrIsEqual(Address, &Current->RemoteAddress))
            Found = TRUE;
    
          if (Found)
            {
              /* FIXME: Maybe we should check if the buffer of this
                 receive request is large enough and if not, search
                 for another */
    
              /* Remove the request from the queue */
              RemoveEntryList(&Current->ListEntry);
              break;
            }
          CurrentEntry = CurrentEntry->Flink;
        }

      TcpipReleaseSpinLock(&AddrFile->Lock, OldIrql);
  
      if (Found)
        {
          TI_DbgPrint(MAX_TRACE, ("Suitable receive request found.\n"));
    
          /* Copy the data into buffer provided by the user */
	  RtlCopyMemory( Current->Buffer,
			 DataBuffer,
			 DataSize );
  
          /* Complete the receive request */
          (*Current->Complete)(Current->Context, STATUS_SUCCESS, DataSize);
    
          exFreePool(Current);
        }
    }
  else if (AddrFile->RegisteredReceiveDatagramHandler)
    {
      TI_DbgPrint(MAX_TRACE, ("Calling receive event handler.\n"));

      ReceiveHandler = AddrFile->ReceiveDatagramHandler;
      HandlerContext = AddrFile->ReceiveDatagramHandlerContext;

      TcpipReleaseSpinLock(&AddrFile->Lock, OldIrql);

      if (Address->Type == IP_ADDRESS_V4)
        {
          AddressLength = sizeof(IPv4_RAW_ADDRESS);
          SourceAddress = &Address->Address.IPv4Address;
        }
      else /* (Address->Type == IP_ADDRESS_V6) */
        {
          AddressLength = sizeof(IPv6_RAW_ADDRESS);
          SourceAddress = Address->Address.IPv6Address;
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
      TI_DbgPrint(MAX_TRACE, ("Discarding datagram.\n"));
    }

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}

