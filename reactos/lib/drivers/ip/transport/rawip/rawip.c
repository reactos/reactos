/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        transport/rawip/rawip.c
 * PURPOSE:     User Datagram Protocol routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"

NTSTATUS AddGenericHeaderIPv4(
    PIP_ADDRESS RemoteAddress,
    USHORT RemotePort,
    PIP_ADDRESS LocalAddress,
    USHORT LocalPort,
    PIP_PACKET IPPacket,
    UINT DataLength,
    UINT Protocol,
    UINT ExtraLength,
    PVOID *NextHeader )
/*
 * FUNCTION: Adds an IPv4 and RawIp header to an IP packet
 * ARGUMENTS:
 *     SendRequest  = Pointer to send request
 *     LocalAddress = Pointer to our local address
 *     LocalPort    = The port we send this datagram from
 *     IPPacket     = Pointer to IP packet
 * RETURNS:
 *     Status of operation
 */
{
    PIPv4_HEADER IPHeader;
    ULONG BufferSize;

    TI_DbgPrint(MID_TRACE, ("Packet: %x NdisPacket %x\n",
			    IPPacket, IPPacket->NdisPacket));

    BufferSize = MaxLLHeaderSize + sizeof(IPv4_HEADER) + ExtraLength;

    GetDataPtr( IPPacket->NdisPacket,
		MaxLLHeaderSize,
		(PCHAR *)&IPPacket->Header,
		&IPPacket->ContigSize );

    IPPacket->HeaderSize = 20;

    TI_DbgPrint(MAX_TRACE, ("Allocated %d bytes for headers at 0x%X.\n",
			    BufferSize, IPPacket->Header));
    TI_DbgPrint(MAX_TRACE, ("Packet total length %d\n", IPPacket->TotalSize));

    /* Build IPv4 header */
    IPHeader = (PIPv4_HEADER)IPPacket->Header;
    /* Version = 4, Length = 5 DWORDs */
    IPHeader->VerIHL = 0x45;
    /* Normal Type-of-Service */
    IPHeader->Tos = 0;
    /* Length of header and data */
    IPHeader->TotalLength = WH2N((USHORT)IPPacket->TotalSize);
    /* Identification */
    IPHeader->Id = 0;
    /* One fragment at offset 0 */
    IPHeader->FlagsFragOfs = 0;
    /* Time-to-Live is 128 */
    IPHeader->Ttl = 128;
    /* User Datagram Protocol */
    IPHeader->Protocol = Protocol;
    /* Checksum is 0 (for later calculation of this) */
    IPHeader->Checksum = 0;
    /* Source address */
    IPHeader->SrcAddr = LocalAddress->Address.IPv4Address;
    /* Destination address. FIXME: IPv4 only */
    IPHeader->DstAddr = RemoteAddress->Address.IPv4Address;

    /* Build RawIp header */
    *NextHeader = (((PCHAR)IPHeader) + sizeof(IPv4_HEADER));
    IPPacket->Data = ((PCHAR)*NextHeader) + ExtraLength;

    return STATUS_SUCCESS;
}


NTSTATUS BuildRawIpPacket(
    PIP_PACKET Packet,
    PIP_ADDRESS RemoteAddress,
    USHORT RemotePort,
    PIP_ADDRESS LocalAddress,
    USHORT LocalPort,
    PCHAR DataBuffer,
    UINT DataLen )
/*
 * FUNCTION: Builds an RawIp packet
 * ARGUMENTS:
 *     Context      = Pointer to context information (DATAGRAM_SEND_REQUEST)
 *     LocalAddress = Pointer to our local address
 *     LocalPort    = The port we send this datagram from
 *     IPPacket     = Address of pointer to IP packet
 * RETURNS:
 *     Status of operation
 */
{
    NTSTATUS Status;
    PCHAR Payload;

    TI_DbgPrint(MAX_TRACE, ("Called.\n"));

    /* FIXME: Assumes IPv4 */
    IPInitializePacket(Packet, IP_ADDRESS_V4);
    if (!Packet)
	return STATUS_INSUFFICIENT_RESOURCES;

    Packet->TotalSize = sizeof(IPv4_HEADER) + DataLen;

    /* Prepare packet */
    Status = AllocatePacketWithBuffer( &Packet->NdisPacket,
				       NULL,
				       Packet->TotalSize + MaxLLHeaderSize );

    if( !NT_SUCCESS(Status) ) return Status;

    TI_DbgPrint(MID_TRACE, ("Allocated packet: %x\n", Packet->NdisPacket));
    TI_DbgPrint(MID_TRACE, ("Local Addr : %s\n", A2S(LocalAddress)));
    TI_DbgPrint(MID_TRACE, ("Remote Addr: %s\n", A2S(RemoteAddress)));

    switch (RemoteAddress->Type) {
    case IP_ADDRESS_V4:
	Status = AddGenericHeaderIPv4
            (RemoteAddress, RemotePort,
             LocalAddress, LocalPort, Packet, DataLen,
             IPPROTO_ICMP, /* XXX Figure out a better way to do this */
             0, (PVOID *)&Payload );
	break;
    case IP_ADDRESS_V6:
	/* FIXME: Support IPv6 */
        Status = STATUS_UNSUCCESSFUL;
	TI_DbgPrint(MIN_TRACE, ("IPv6 RawIp datagrams are not supported.\n"));
        break;

    default:
	Status = STATUS_UNSUCCESSFUL;
        TI_DbgPrint(MIN_TRACE, ("Bad Address Type %d\n", RemoteAddress->Type));
	break;
    }

    if( !NT_SUCCESS(Status) ) {
	TI_DbgPrint(MIN_TRACE, ("Cannot add header. Status = (0x%X)\n",
				Status));
	FreeNdisPacket(Packet->NdisPacket);
	return Status;
    }

    TI_DbgPrint(MID_TRACE, ("Copying data (hdr %x data %x (%d))\n",
			    Packet->Header, Packet->Data,
			    (PCHAR)Packet->Data - (PCHAR)Packet->Header));

    RtlCopyMemory( Packet->Data, DataBuffer, DataLen );

    Packet->Flags |= IP_PACKET_FLAG_RAW;

    TI_DbgPrint(MID_TRACE, ("Displaying packet\n"));

    DISPLAY_IP_PACKET(Packet);

    TI_DbgPrint(MID_TRACE, ("Leaving\n"));

    return STATUS_SUCCESS;
}

VOID RawIpSendPacketComplete
( PVOID Context, PNDIS_PACKET Packet, NDIS_STATUS Status ) {
    FreeNdisPacket( Packet );
}

NTSTATUS RawIPSendDatagram(
    PADDRESS_FILE AddrFile,
    PTDI_CONNECTION_INFORMATION ConnInfo,
    PCHAR BufferData,
    ULONG DataSize,
    PULONG DataUsed )
/*
 * FUNCTION: Sends an RawIp datagram to a remote address
 * ARGUMENTS:
 *     Request   = Pointer to TDI request
 *     ConnInfo  = Pointer to connection information
 *     Buffer    = Pointer to NDIS buffer with data
 *     DataSize  = Size in bytes of data to be sent
 * RETURNS:
 *     Status of operation
 */
{
    IP_PACKET Packet;
    PTA_IP_ADDRESS RemoteAddressTa = (PTA_IP_ADDRESS)ConnInfo->RemoteAddress;
    IP_ADDRESS RemoteAddress;
    USHORT RemotePort;
    NTSTATUS Status;
    PNEIGHBOR_CACHE_ENTRY NCE;

    TI_DbgPrint(MID_TRACE,("Sending Datagram(%x %x %x %d)\n",
			   AddrFile, ConnInfo, BufferData, DataSize));
    TI_DbgPrint(MID_TRACE,("RemoteAddressTa: %x\n", RemoteAddressTa));

    switch( RemoteAddressTa->Address[0].AddressType ) {
    case TDI_ADDRESS_TYPE_IP:
	RemoteAddress.Type = IP_ADDRESS_V4;
	RemoteAddress.Address.IPv4Address =
	    RemoteAddressTa->Address[0].Address[0].in_addr;
	RemotePort = RemoteAddressTa->Address[0].Address[0].sin_port;
	break;

    default:
	return STATUS_UNSUCCESSFUL;
    }

    Status = BuildRawIpPacket( &Packet,
                               &RemoteAddress,
                               RemotePort,
                               &AddrFile->Address,
                               AddrFile->Port,
                               BufferData,
                               DataSize );

    if( !NT_SUCCESS(Status) )
	return Status;

    TI_DbgPrint(MID_TRACE,("About to get route to destination\n"));

    if(!(NCE = RouteGetRouteToDestination( &RemoteAddress ))) {
        FreeNdisPacket(Packet.NdisPacket);
	return STATUS_UNSUCCESSFUL;
    }

    TI_DbgPrint(MID_TRACE,("About to send datagram\n"));

    if (!NT_SUCCESS(Status = IPSendDatagram( &Packet, NCE, RawIpSendPacketComplete, NULL )))
    {
        FreeNdisPacket(Packet.NdisPacket);
        return Status;
    }

    TI_DbgPrint(MID_TRACE,("Leaving\n"));

    return STATUS_SUCCESS;
}


VOID RawIpReceive(PIP_INTERFACE Interface, PIP_PACKET IPPacket)
/*
 * FUNCTION: Receives and queues a RawIp datagram
 * ARGUMENTS:
 *     NTE      = Pointer to net table entry which the packet was received on
*     IPPacket = Pointer to an IP packet that was received
* NOTES:
*     This is the low level interface for receiving RawIp datagrams. It strips
*     the RawIp header from a packet and delivers the data to anyone that wants it
*/
{
  AF_SEARCH SearchContext;
  PIPv4_HEADER IPv4Header;
  PADDRESS_FILE AddrFile;
  PIP_ADDRESS DstAddress, SrcAddress;
  UINT DataSize;

  TI_DbgPrint(MAX_TRACE, ("Called.\n"));

  switch (IPPacket->Type) {
  /* IPv4 packet */
  case IP_ADDRESS_V4:
    IPv4Header = IPPacket->Header;
    DstAddress = &IPPacket->DstAddr;
    SrcAddress = &IPPacket->SrcAddr;
    DataSize = IPPacket->TotalSize;
    break;

  /* IPv6 packet */
  case IP_ADDRESS_V6:
    TI_DbgPrint(MIN_TRACE, ("Discarded IPv6 datagram (%i bytes).\n", IPPacket->TotalSize));

    /* FIXME: IPv6 is not supported */
    return;

  default:
    return;
  }

  /* Locate a receive request on destination address file object
     and deliver the packet if one is found. If there is no receive
     request on the address file object, call the associated receive
     handler. If no receive handler is registered, drop the packet */

  AddrFile = AddrSearchFirst(DstAddress,
                             0,
                             IPv4Header->Protocol,
                             &SearchContext);
  if (AddrFile) {
    do {
      DGDeliverData(AddrFile,
		    SrcAddress,
                    DstAddress,
                    0,
                    0,
                    IPPacket,
                    DataSize);
    } while ((AddrFile = AddrSearchNext(&SearchContext)) != NULL);
  } else {
    /* There are no open address files that will take this datagram */
    /* FIXME: IPv4 only */
    TI_DbgPrint(MID_TRACE, ("Cannot deliver IPv4 raw datagram to address (0x%X).\n",
                            DN2H(DstAddress->Address.IPv4Address)));

    /* FIXME: Send ICMP reply */
  }
  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}


NTSTATUS RawIPStartup(VOID)
/*
 * FUNCTION: Initializes the UDP subsystem
 * RETURNS:
 *     Status of operation
 */
{
#ifdef __NTDRIVER__
  RtlZeroMemory(&UDPStats, sizeof(UDP_STATISTICS));
#endif

  /* Register this protocol with IP layer */
  IPRegisterProtocol(IPPROTO_ICMP, RawIpReceive);

  return STATUS_SUCCESS;
}


NTSTATUS RawIPShutdown(VOID)
/*
 * FUNCTION: Shuts down the UDP subsystem
 * RETURNS:
 *     Status of operation
 */
{
  /* Deregister this protocol with IP layer */
  IPRegisterProtocol(IPPROTO_ICMP, NULL);

  return STATUS_SUCCESS;
}

/* EOF */
