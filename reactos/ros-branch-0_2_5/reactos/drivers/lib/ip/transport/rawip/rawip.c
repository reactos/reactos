/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        transport/rawip/rawip.c
 * PURPOSE:     Raw IP routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"


BOOLEAN RawIPInitialized = FALSE;


NTSTATUS BuildRawIPPacket(
    PIP_PACKET Packet,
    UINT DataLength,
    PIP_ADDRESS LocalAddress,
    USHORT LocalPort )
/*
 * FUNCTION: Builds an UDP packet
 * ARGUMENTS:
 *     Context      = Pointer to context information (DATAGRAM_SEND_REQUEST)
 *     LocalAddress = Pointer to our local address (NULL)
 *     LocalPort    = The port we send this datagram from (0)
 *     IPPacket     = Address of pointer to IP packet
 * RETURNS:
 *     Status of operation
 */
{
    PVOID Header;
    NDIS_STATUS NdisStatus;
    PNDIS_BUFFER HeaderBuffer;
    PNDIS_PACKET NdisPacket = Packet->NdisPacket; 
    /* Will be zeroed in packet by IPInitializePacket */

    /* Prepare packet */
    IPInitializePacket(Packet,IP_ADDRESS_V4);
    Packet->Flags      = IP_PACKET_FLAG_RAW;    /* Don't touch IP header */
    Packet->TotalSize  = DataLength;
    Packet->NdisPacket = NdisPacket;

    if (MaxLLHeaderSize != 0) {
        Header = ExAllocatePool(NonPagedPool, MaxLLHeaderSize);
        if (!Header) {
            TI_DbgPrint(MIN_TRACE, ("Cannot allocate memory for packet headers.\n"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        TI_DbgPrint(MAX_TRACE, ("Allocated %d bytes for headers at 0x%X.\n",
            MaxLLHeaderSize, Header));

        /* Allocate NDIS buffer for maximum link level header */
        NdisAllocateBuffer(&NdisStatus,
			   &HeaderBuffer,
			   GlobalBufferPool,
			   Header,
			   MaxLLHeaderSize);

        if (NdisStatus != NDIS_STATUS_SUCCESS) {
            TI_DbgPrint(MIN_TRACE, ("Cannot allocate NDIS buffer for packet headers. NdisStatus = (0x%X)\n", NdisStatus));
            ExFreePool(Header);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Chain header at front of packet */
        NdisChainBufferAtFront(Packet->NdisPacket, HeaderBuffer);
    }

    DISPLAY_IP_PACKET(Packet);

    return STATUS_SUCCESS;
}

VOID RawIPSendComplete
( PVOID Context, PNDIS_PACKET Packet, NDIS_STATUS Status ) {
    FreeNdisPacket( Packet );
}

NTSTATUS RawIPSendDatagram(
    PADDRESS_FILE AddrFile,
    PTDI_CONNECTION_INFORMATION ConnInfo,
    PCHAR BufferData,
    ULONG BufferLen,
    PULONG DataUsed )
/*
 * FUNCTION: Sends a raw IP datagram to a remote address
 * ARGUMENTS:
 *     Request   = Pointer to TDI request
 *     ConnInfo  = Pointer to connection information
 *     Buffer    = Pointer to NDIS buffer with data
 *     DataSize  = Size in bytes of data to be sent
 * RETURNS:
 *     Status of operation
 */
{
    NDIS_STATUS Status;
    IP_PACKET Packet;
    PNEIGHBOR_CACHE_ENTRY NCE;
    IP_ADDRESS RemoteAddress;

    Status = AllocatePacketWithBuffer( &Packet.NdisPacket,
				       BufferData,
				       BufferLen );
    
    TI_DbgPrint(MID_TRACE,("Packet.NdisPacket %x\n", Packet.NdisPacket));

    *DataUsed = BufferLen;

    if( Status == NDIS_STATUS_SUCCESS )
	Status = BuildRawIPPacket( &Packet,
				   BufferLen,
				   &AddrFile->Address,
				   AddrFile->Port );

    if( Status == NDIS_STATUS_SUCCESS ) {
	RemoteAddress.Type = IP_ADDRESS_V4;
	RtlCopyMemory( &RemoteAddress.Address.IPv4Address,
		       BufferData + FIELD_OFFSET(IPv4_HEADER, DstAddr),
		       sizeof(IPv4_RAW_ADDRESS) );

	if(!(NCE = RouteGetRouteToDestination( &RemoteAddress ))) {
	    FreeNdisPacket( Packet.NdisPacket );
	    return STATUS_NO_SUCH_DEVICE;
	}
	
	IPSendDatagram( &Packet, NCE, RawIPSendComplete, NULL );
    } else
	FreeNdisPacket( Packet.NdisPacket );

    return Status;
}


VOID RawIPReceive(
    PIP_INTERFACE Interface,
    PIP_PACKET IPPacket)
/*
 * FUNCTION: Receives and queues a raw IP datagram
 * ARGUMENTS:
 *     NTE      = Pointer to net table entry which the packet was received on
 *     IPPacket = Pointer to an IP packet that was received
 * NOTES:
 *     This is the low level interface for receiving ICMP datagrams.
 *     It delivers the packet header and data to anyone that wants it
 *     When we get here the datagram has already passed sanity checks
 */
{
    PIP_ADDRESS DstAddress;

    TI_DbgPrint(MAX_TRACE, ("Called.\n"));

    switch (IPPacket->Type) {
    /* IPv4 packet */
    case IP_ADDRESS_V4:
        DstAddress = &IPPacket->DstAddr;
        break;

    /* IPv6 packet */
    case IP_ADDRESS_V6:
        TI_DbgPrint(MIN_TRACE, ("Discarded IPv6 raw IP datagram (%i bytes).\n",
          IPPacket->TotalSize));

        /* FIXME: IPv6 is not supported */
        return;

    default:
        return;
    }

    /* Locate a receive request on destination address file object
       and deliver the packet if one is found. If there is no receive
       request on the address file object, call the associated receive
       handler. If no receive handler is registered, drop the packet */

#if 0 /* Decide what to do here */
    AddrFile = AddrSearchFirst(DstAddress,
                               0,
                               IPPROTO_ICMP,
                               &SearchContext);
    if (AddrFile) {
        do {
            DGDeliverData(AddrFile,
                          DstAddress,
                          IPPacket,
                          IPPacket->TotalSize);
        } while ((AddrFile = AddrSearchNext(&SearchContext)) != NULL);
    } else {
        /* There are no open address files that will take this datagram */
        /* FIXME: IPv4 only */
        TI_DbgPrint(MID_TRACE, ("Cannot deliver IPv4 ICMP datagram to address (0x%X).\n",
            DN2H(DstAddress->Address.IPv4Address)));
    }
#endif
    TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}


NTSTATUS RawIPStartup(
    VOID)
/*
 * FUNCTION: Initializes the Raw IP subsystem
 * RETURNS:
 *     Status of operation
 */
{
    RawIPInitialized = TRUE;

    return STATUS_SUCCESS;
}


NTSTATUS RawIPShutdown(
    VOID)
/*
 * FUNCTION: Shuts down the Raw IP subsystem
 * RETURNS:
 *     Status of operation
 */
{
    if (!RawIPInitialized)
        return STATUS_SUCCESS;

    return STATUS_SUCCESS;
}

/* EOF */
