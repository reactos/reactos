/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        transport/rawip/rawip.c
 * PURPOSE:     Raw IP routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <roscfg.h>
#include <tcpip.h>
#include <rawip.h>
#include <routines.h>
#include <datagram.h>
#include <address.h>
#include <pool.h>


BOOLEAN RawIPInitialized = FALSE;


NTSTATUS BuildRawIPPacket(
    PVOID Context,
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
    PDATAGRAM_SEND_REQUEST SendRequest = (PDATAGRAM_SEND_REQUEST)Context;
    PIP_PACKET Packet = &SendRequest->Packet;

    /* Prepare packet */

    /* FIXME: Assumes IPv4 */
    if (!Packet)
        return STATUS_INSUFFICIENT_RESOURCES;

    IPInitializePacket(Packet,IP_ADDRESS_V4);
    Packet->Flags      = IP_PACKET_FLAG_RAW;    /* Don't touch IP header */
    Packet->TotalSize  = SendRequest->BufferSize;

    if (MaxLLHeaderSize != 0) {
        Header = ExAllocatePool(NonPagedPool, MaxLLHeaderSize);
        if (!Header) {
            TI_DbgPrint(MIN_TRACE, ("Cannot allocate memory for packet headers.\n"));
            FreeNdisPacket(Packet->NdisPacket);
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
            FreeNdisPacket(Packet->NdisPacket);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        /* Chain header at front of packet */
        NdisChainBufferAtFront(Packet->NdisPacket, HeaderBuffer);
    }

    DISPLAY_IP_PACKET(Packet);

    return STATUS_SUCCESS;
}


NTSTATUS RawIPSendDatagram(
    PTDI_REQUEST Request,
    PTDI_CONNECTION_INFORMATION ConnInfo,
    PNDIS_BUFFER Buffer,
    ULONG DataSize)
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
    PCHAR BufferData;
    UINT BufferLen;
    PADDRESS_FILE AddrFile = 
	(PADDRESS_FILE)Request->Handle.AddressHandle;
    PDATAGRAM_SEND_REQUEST SendRequest;

    SendRequest = ExAllocatePool( NonPagedPool, sizeof(*SendRequest) );

    NdisQueryBuffer( Buffer, &BufferData, &BufferLen );
    Status = AllocatePacketWithBuffer( &SendRequest->Packet.NdisPacket,
				       BufferData,
				       BufferLen );
    
    if( Status != NDIS_STATUS_SUCCESS ) {
	BuildRawIPPacket( SendRequest,
			  (PIP_ADDRESS)&AddrFile->ADE->Address->Address.
			  IPv4Address,
			  AddrFile->Port );
	Status = DGSendDatagram(Request, ConnInfo, &SendRequest->Packet);
	NdisFreeBuffer( Buffer );
    }

    return Status;
}


VOID RawIPReceive(
    PNET_TABLE_ENTRY NTE,
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
    AF_SEARCH SearchContext;
    PADDRESS_FILE AddrFile;
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
