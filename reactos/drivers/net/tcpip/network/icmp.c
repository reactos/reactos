/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        network/icmp.c
 * PURPOSE:     Internet Control Message Protocol routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <tcpip.h>
#include <icmp.h>
#include <rawip.h>
#include <checksum.h>
#include <routines.h>
#include <transmit.h>
#include <pool.h>


VOID SendICMPComplete(
    PVOID Context,
    PNDIS_PACKET Packet,
    NDIS_STATUS NdisStatus)
/*
 * FUNCTION: ICMP datagram transmit completion handler
 * ARGUMENTS:
 *     Context    = Pointer to context infomation (IP_PACKET)
 *     Packet     = Pointer to NDIS packet
 *     NdisStatus = Status of transmit operation
 * NOTES:
 *     This routine is called by IP when a ICMP send completes
 */
{
    PIP_PACKET IPPacket = (PIP_PACKET)Context;

    TI_DbgPrint(DEBUG_ICMP, ("Freeing NDIS packet (%X).\n", Packet));

    /* Free packet */
    FreeNdisPacket(Packet);

    TI_DbgPrint(DEBUG_ICMP, ("Freeing IP packet at %X.\n", IPPacket));

    PoolFreeBuffer(IPPacket);
}


PIP_PACKET PrepareICMPPacket(
    PNET_TABLE_ENTRY NTE,
    PIP_ADDRESS Destination,
    UINT DataSize)
/*
 * FUNCTION: Prepares an ICMP packet
 * ARGUMENTS:
 *     NTE         = Pointer to net table entry to use
 *     Destination = Pointer to destination address
 *     DataSize    = Size of dataarea
 * RETURNS:
 *     Pointer to IP packet, NULL if there is not enough free resources
 */
{
    PIP_PACKET IPPacket;
    PNDIS_PACKET NdisPacket;
    PNDIS_BUFFER NdisBuffer;
    NDIS_STATUS NdisStatus;
    PIPv4_HEADER IPHeader;
    PVOID DataBuffer;
    ULONG Size;

    TI_DbgPrint(DEBUG_ICMP, ("Called. DataSize (%d).\n", DataSize));

    /* Prepare ICMP packet */
    IPPacket = PoolAllocateBuffer(sizeof(IP_PACKET));
    if (!IPPacket)
        return NULL;

    TI_DbgPrint(DEBUG_ICMP, ("IPPacket at (0x%X).\n", IPPacket));

    /* No special flags */
    IPPacket->Flags = 0;

    Size = MaxLLHeaderSize + sizeof(IPv4_HEADER) +
        sizeof(ICMP_HEADER) + DataSize;
    DataBuffer = ExAllocatePool(NonPagedPool, Size);
    if (!DataBuffer) {
        PoolFreeBuffer(IPPacket);
        return NULL;
    }

    TI_DbgPrint(DEBUG_ICMP, ("Size (%d). Data at (0x%X).\n", Size, DataBuffer));

    /* Allocate NDIS packet */
    NdisAllocatePacket(&NdisStatus, &NdisPacket, GlobalPacketPool);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        PoolFreeBuffer(IPPacket);
        ExFreePool(DataBuffer);
        return NULL;
    }

    TI_DbgPrint(MAX_TRACE, ("NdisPacket at (0x%X).\n", NdisPacket));

    /* Allocate NDIS buffer for maximum link level header and ICMP packet */
    NdisAllocateBuffer(&NdisStatus, &NdisBuffer, GlobalBufferPool,
        DataBuffer, Size);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        PoolFreeBuffer(IPPacket);
        NdisFreePacket(NdisPacket);
        ExFreePool(DataBuffer);
        return NULL;
    }

    TI_DbgPrint(MAX_TRACE, ("NdisBuffer at (0x%X).\n", NdisBuffer));

    /* Link NDIS buffer into packet */
    NdisChainBufferAtFront(NdisPacket, NdisBuffer);
    IPPacket->NdisPacket = NdisPacket;
    IPPacket->Header     = (PVOID)((ULONG_PTR)DataBuffer + MaxLLHeaderSize);
    IPPacket->Data       = (PVOID)((ULONG_PTR)DataBuffer + MaxLLHeaderSize + sizeof(IPv4_HEADER));

    IPPacket->HeaderSize = sizeof(IPv4_HEADER);
    IPPacket->TotalSize  = Size - MaxLLHeaderSize;
    RtlCopyMemory(&IPPacket->DstAddr, Destination, sizeof(IP_ADDRESS));

    /* Build IPv4 header. FIXME: IPv4 only */

    IPHeader = (PIPv4_HEADER)IPPacket->Header;

    /* Version = 4, Length = 5 DWORDs */
    IPHeader->VerIHL = 0x45;
    /* Normal Type-of-Service */
    IPHeader->Tos = 0;
    /* Length of data and header */
    IPHeader->TotalLength = WH2N((USHORT)DataSize +
        sizeof(IPv4_HEADER) + sizeof(ICMP_HEADER));
    /* Identification */
    IPHeader->Id = (USHORT)Random();
    /* One fragment at offset 0 */
    IPHeader->FlagsFragOfs = 0;
    /* Time-to-Live is 128 */
    IPHeader->Ttl = 128;
    /* Internet Control Message Protocol */
    IPHeader->Protocol = IPPROTO_ICMP;
    /* Checksum is 0 (for later calculation of this) */
    IPHeader->Checksum = 0;
    /* Source address */
    IPHeader->SrcAddr = NTE->Address->Address.IPv4Address;
    /* Destination address */
    IPHeader->DstAddr = Destination->Address.IPv4Address;

    /* Completion handler */
    PC(NdisPacket)->Complete = SendICMPComplete;
    PC(NdisPacket)->Context  = IPPacket;

    return IPPacket;
}


VOID ICMPReceive(
    PNET_TABLE_ENTRY NTE,
    PIP_PACKET IPPacket)
/*
 * FUNCTION: Receives an ICMP packet
 * ARGUMENTS:
 *     NTE      = Pointer to net table entry which the packet was received on
 *     IPPacket = Pointer to an IP packet that was received
 */
{
    PICMP_HEADER ICMPHeader;
    PIP_PACKET NewPacket;
    UINT DataSize;

    TI_DbgPrint(DEBUG_ICMP, ("Called.\n"));

    ICMPHeader = (PICMP_HEADER)IPPacket->Data;

    TI_DbgPrint(DEBUG_ICMP, ("Size (%d).\n", IPPacket->TotalSize));

    TI_DbgPrint(DEBUG_ICMP, ("HeaderSize (%d).\n", IPPacket->HeaderSize));

    TI_DbgPrint(DEBUG_ICMP, ("Type (%d).\n", ICMPHeader->Type));

    TI_DbgPrint(DEBUG_ICMP, ("Code (%d).\n", ICMPHeader->Code));

    TI_DbgPrint(DEBUG_ICMP, ("Checksum (0x%X).\n", ICMPHeader->Checksum));

    /* Checksum ICMP header and data */
    if (!CorrectChecksum(IPPacket->Data, IPPacket->TotalSize - IPPacket->HeaderSize)) {
        TI_DbgPrint(DEBUG_ICMP, ("Bad ICMP checksum.\n"));
        /* Discard packet */
        return;
    }

    switch (ICMPHeader->Type) {
    case ICMP_TYPE_ECHO_REQUEST:
        /* Reply with an ICMP echo reply message */
        DataSize  = IPPacket->TotalSize - IPPacket->HeaderSize - sizeof(ICMP_HEADER);
        NewPacket = PrepareICMPPacket(NTE, &IPPacket->SrcAddr, DataSize);
        if (!NewPacket)
            return;

        /* Copy ICMP header and data into new packet */
        RtlCopyMemory(NewPacket->Data, IPPacket->Data, DataSize  + sizeof(ICMP_HEADER));
        ((PICMP_HEADER)NewPacket->Data)->Type     = ICMP_TYPE_ECHO_REPLY;
        ((PICMP_HEADER)NewPacket->Data)->Code     = 0;
        ((PICMP_HEADER)NewPacket->Data)->Checksum = 0;

        ICMPTransmit(NTE, NewPacket);

        TI_DbgPrint(DEBUG_ICMP, ("Echo reply sent.\n"));
        return;

    case ICMP_TYPE_ECHO_REPLY:
        break;

    default:
        TI_DbgPrint(DEBUG_ICMP, ("Discarded ICMP datagram of unknown type %d.\n",
            ICMPHeader->Type));
        /* Discard packet */
        break;
    }

    /* Send datagram up the protocol stack */
    RawIPReceive(NTE, IPPacket);
}


VOID ICMPTransmit(
    PNET_TABLE_ENTRY NTE,
    PIP_PACKET IPPacket)
/*
 * FUNCTION: Transmits an ICMP packet
 * ARGUMENTS:
 *     NTE      = Pointer to net table entry to use (NULL if don't care)
 *     IPPacket = Pointer to IP packet to transmit
 */
{
    PROUTE_CACHE_NODE RCN;

    TI_DbgPrint(DEBUG_ICMP, ("Called.\n"));

    /* Calculate checksum of ICMP header and data */
    ((PICMP_HEADER)IPPacket->Data)->Checksum = (USHORT)
        IPv4Checksum(IPPacket->Data, IPPacket->TotalSize - IPPacket->HeaderSize, 0);

    /* Get a route to the destination address */
    if (RouteGetRouteToDestination(&IPPacket->DstAddr, NTE, &RCN) == IP_SUCCESS) {
        /* Send the packet */
        if (IPSendDatagram(IPPacket, RCN) != STATUS_SUCCESS) {
            FreeNdisPacket(IPPacket->NdisPacket);
            PoolFreeBuffer(IPPacket);
        }
        /* We're done with the RCN */
        DereferenceObject(RCN);
    } else {
        TI_DbgPrint(MIN_TRACE, ("RCN at (0x%X).\n", RCN));

        /* No route to destination (or no free resources) */
        TI_DbgPrint(DEBUG_ICMP, ("No route to destination address 0x%X.\n",
            IPPacket->DstAddr.Address.IPv4Address));
        /* Discard packet */
        FreeNdisPacket(IPPacket->NdisPacket);
        PoolFreeBuffer(IPPacket);
    }
}


VOID ICMPReply(
    PNET_TABLE_ENTRY NTE,
    PIP_PACKET IPPacket,
	  UCHAR Type,
 	  UCHAR Code)
/*
 * FUNCTION: Transmits an ICMP packet in response to an incoming packet
 * ARGUMENTS:
 *     NTE      = Pointer to net table entry to use
 *     IPPacket = Pointer to IP packet that was received
 *     Type     = ICMP message type
 *     Code     = ICMP message code
 * NOTES:
 *     We have received a packet from someone and is unable to
 *     process it due to error(s) in the packet or we have run out
 *     of resources. We transmit an ICMP message to the host to
 *     notify him of the problem
 */
{
    UINT DataSize;
    PIP_PACKET NewPacket;

    TI_DbgPrint(DEBUG_ICMP, ("Called. Type (%d)  Code (%d).\n", Type, Code));

    DataSize = IPPacket->TotalSize;
    if ((DataSize) > (576 - sizeof(IPv4_HEADER) - sizeof(ICMP_HEADER)))
        DataSize = 576;

    NewPacket = PrepareICMPPacket(NTE, &IPPacket->SrcAddr, DataSize);
    if (!NewPacket) {
        TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return;
    }

    RtlCopyMemory((PVOID)((ULONG_PTR)NewPacket->Data + sizeof(ICMP_HEADER)),
        IPPacket->Header, DataSize);
    ((PICMP_HEADER)NewPacket->Data)->Type     = Type;
    ((PICMP_HEADER)NewPacket->Data)->Code     = Code;
    ((PICMP_HEADER)NewPacket->Data)->Checksum = 0;

    ICMPTransmit(NTE, NewPacket);
}

/* EOF */
