/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        transport/udp/udp.c
 * PURPOSE:     User Datagram Protocol routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"


BOOLEAN UDPInitialized = FALSE;
PORT_SET UDPPorts;

NTSTATUS AddUDPHeaderIPv4(
    PIP_ADDRESS RemoteAddress,
    USHORT RemotePort,
    PIP_ADDRESS LocalAddress,
    USHORT LocalPort,
    PIP_PACKET IPPacket,
    UINT DataLength)
/*
 * FUNCTION: Adds an IPv4 and UDP header to an IP packet
 * ARGUMENTS:
 *     SendRequest  = Pointer to send request
 *     LocalAddress = Pointer to our local address
 *     LocalPort    = The port we send this datagram from
 *     IPPacket     = Pointer to IP packet
 * RETURNS:
 *     Status of operation
 */
{
    PUDP_HEADER UDPHeader;
    NTSTATUS Status;

    TI_DbgPrint(MID_TRACE, ("Packet: %x NdisPacket %x\n",
			    IPPacket, IPPacket->NdisPacket));

    Status = AddGenericHeaderIPv4
        ( RemoteAddress, RemotePort,
          LocalAddress, LocalPort,
          IPPacket, DataLength, IPPROTO_UDP,
          sizeof(UDP_HEADER), (PVOID *)&UDPHeader );

    if (!NT_SUCCESS(Status))
        return Status;

    /* Build UDP header */
    UDPHeader = (PUDP_HEADER)((ULONG_PTR)IPPacket->Data - sizeof(UDP_HEADER));
    /* Port values are already big-endian values */
    UDPHeader->SourcePort = LocalPort;
    UDPHeader->DestPort   = RemotePort;
    /* FIXME: Calculate UDP checksum and put it in UDP header */
    UDPHeader->Checksum   = 0;
    /* Length of UDP header and data */
    UDPHeader->Length     = WH2N(DataLength + sizeof(UDP_HEADER));

    IPPacket->Data        = ((PCHAR)UDPHeader) + sizeof(UDP_HEADER);

    TI_DbgPrint(MID_TRACE, ("Packet: %d ip %d udp %d payload\n",
			    (PCHAR)UDPHeader - (PCHAR)IPPacket->Header,
			    (PCHAR)IPPacket->Data - (PCHAR)UDPHeader,
			    DataLength));

    return STATUS_SUCCESS;
}


NTSTATUS BuildUDPPacket(
    PIP_PACKET Packet,
    PIP_ADDRESS RemoteAddress,
    USHORT RemotePort,
    PIP_ADDRESS LocalAddress,
    USHORT LocalPort,
    PCHAR DataBuffer,
    UINT DataLen )
/*
 * FUNCTION: Builds an UDP packet
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

    TI_DbgPrint(MAX_TRACE, ("Called.\n"));

    /* FIXME: Assumes IPv4 */
    IPInitializePacket(Packet, IP_ADDRESS_V4);
    if (!Packet)
	return STATUS_INSUFFICIENT_RESOURCES;

    Packet->TotalSize = sizeof(IPv4_HEADER) + sizeof(UDP_HEADER) + DataLen;

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
	Status = AddUDPHeaderIPv4(RemoteAddress, RemotePort,
				  LocalAddress, LocalPort, Packet, DataLen);
	break;
    case IP_ADDRESS_V6:
	/* FIXME: Support IPv6 */
	TI_DbgPrint(MIN_TRACE, ("IPv6 UDP datagrams are not supported.\n"));
    default:
	Status = STATUS_UNSUCCESSFUL;
	break;
    }
    if (!NT_SUCCESS(Status)) {
	TI_DbgPrint(MIN_TRACE, ("Cannot add UDP header. Status = (0x%X)\n",
				Status));
	FreeNdisPacket(Packet->NdisPacket);
	return Status;
    }

    TI_DbgPrint(MID_TRACE, ("Copying data (hdr %x data %x (%d))\n",
			    Packet->Header, Packet->Data,
			    (PCHAR)Packet->Data - (PCHAR)Packet->Header));

    RtlCopyMemory( Packet->Data, DataBuffer, DataLen );

    TI_DbgPrint(MID_TRACE, ("Displaying packet\n"));

    DISPLAY_IP_PACKET(Packet);

    TI_DbgPrint(MID_TRACE, ("Leaving\n"));

    return STATUS_SUCCESS;
}

VOID UDPSendPacketComplete
( PVOID Context, PNDIS_PACKET Packet, NDIS_STATUS Status ) {
    FreeNdisPacket( Packet );
}

NTSTATUS UDPSendDatagram(
    PADDRESS_FILE AddrFile,
    PTDI_CONNECTION_INFORMATION ConnInfo,
    PCHAR BufferData,
    ULONG DataSize,
    PULONG DataUsed )
/*
 * FUNCTION: Sends an UDP datagram to a remote address
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

    Status = BuildUDPPacket( &Packet,
			     &RemoteAddress,
			     RemotePort,
			     &AddrFile->Address,
			     AddrFile->Port,
			     BufferData,
			     DataSize );

    if( !NT_SUCCESS(Status) )
	return Status;

    if(!(NCE = RouteGetRouteToDestination( &RemoteAddress ))) {
        FreeNdisPacket(Packet.NdisPacket);
	return STATUS_UNSUCCESSFUL;
    }

    IPSendDatagram( &Packet, NCE, UDPSendPacketComplete, NULL );

    return STATUS_SUCCESS;
}


VOID UDPReceive(PIP_INTERFACE Interface, PIP_PACKET IPPacket)
/*
 * FUNCTION: Receives and queues a UDP datagram
 * ARGUMENTS:
 *     NTE      = Pointer to net table entry which the packet was received on
*     IPPacket = Pointer to an IP packet that was received
* NOTES:
*     This is the low level interface for receiving UDP datagrams. It strips
*     the UDP header from a packet and delivers the data to anyone that wants it
*/
{
  AF_SEARCH SearchContext;
  PIPv4_HEADER IPv4Header;
  PADDRESS_FILE AddrFile;
  PUDP_HEADER UDPHeader;
  PIP_ADDRESS DstAddress, SrcAddress;
  UINT DataSize, i;

  TI_DbgPrint(MAX_TRACE, ("Called.\n"));

  switch (IPPacket->Type) {
  /* IPv4 packet */
  case IP_ADDRESS_V4:
    IPv4Header = IPPacket->Header;
    DstAddress = &IPPacket->DstAddr;
    SrcAddress = &IPPacket->SrcAddr;
    break;

  /* IPv6 packet */
  case IP_ADDRESS_V6:
    TI_DbgPrint(MIN_TRACE, ("Discarded IPv6 UDP datagram (%i bytes).\n", IPPacket->TotalSize));

    /* FIXME: IPv6 is not supported */
    return;

  default:
    return;
  }

  UDPHeader = (PUDP_HEADER)IPPacket->Data;

  /* FIXME: Calculate and validate UDP checksum */

  /* Sanity checks */
  i = WH2N(UDPHeader->Length);
  if ((i < sizeof(UDP_HEADER)) || (i > IPPacket->TotalSize - IPPacket->Position)) {
    /* Incorrect or damaged packet received, discard it */
    TI_DbgPrint(MIN_TRACE, ("Incorrect or damaged UDP packet received.\n"));
    return;
  }

  DataSize = i - sizeof(UDP_HEADER);

  /* Go to UDP data area */
  IPPacket->Data = (PVOID)((ULONG_PTR)IPPacket->Data + sizeof(UDP_HEADER));

  /* Locate a receive request on destination address file object
     and deliver the packet if one is found. If there is no receive
     request on the address file object, call the associated receive
     handler. If no receive handler is registered, drop the packet */

  AddrFile = AddrSearchFirst(DstAddress,
                             UDPHeader->DestPort,
                             IPPROTO_UDP,
                             &SearchContext);
  if (AddrFile) {
    do {
      DGDeliverData(AddrFile,
		    SrcAddress,
                    DstAddress,
		    UDPHeader->SourcePort,
		    UDPHeader->DestPort,
                    IPPacket,
                    DataSize);
    } while ((AddrFile = AddrSearchNext(&SearchContext)) != NULL);
  } else {
    /* There are no open address files that will take this datagram */
    /* FIXME: IPv4 only */
    TI_DbgPrint(MID_TRACE, ("Cannot deliver IPv4 UDP datagram to address (0x%X).\n",
      DN2H(DstAddress->Address.IPv4Address)));

    /* FIXME: Send ICMP reply */
  }
  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}


NTSTATUS UDPStartup(
  VOID)
/*
 * FUNCTION: Initializes the UDP subsystem
 * RETURNS:
 *     Status of operation
 */
{
#ifdef __NTDRIVER__
  RtlZeroMemory(&UDPStats, sizeof(UDP_STATISTICS));
#endif
  
  NTSTATUS Status;

  Status = PortsStartup( &UDPPorts, 1, 0xfffe );

  if( !NT_SUCCESS(Status) ) return Status;

  /* Register this protocol with IP layer */
  IPRegisterProtocol(IPPROTO_UDP, UDPReceive);

  UDPInitialized = TRUE;

  return STATUS_SUCCESS;
}


NTSTATUS UDPShutdown(
  VOID)
/*
 * FUNCTION: Shuts down the UDP subsystem
 * RETURNS:
 *     Status of operation
 */
{
  if (!UDPInitialized)
      return STATUS_SUCCESS;

  PortsShutdown( &UDPPorts );

  /* Deregister this protocol with IP layer */
  IPRegisterProtocol(IPPROTO_UDP, NULL);

  UDPInitialized = FALSE;

  return STATUS_SUCCESS;
}

UINT UDPAllocatePort( UINT HintPort ) {
    if( HintPort ) {
        if( AllocatePort( &UDPPorts, HintPort ) ) return HintPort;
        else return (UINT)-1;
    } else return AllocatePortFromRange
               ( &UDPPorts, UDP_STARTING_PORT,
                 UDP_STARTING_PORT + UDP_DYNAMIC_PORTS );
}

VOID UDPFreePort( UINT Port ) {
    DeallocatePort( &UDPPorts, Port );
}

/* EOF */
