/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        network/icmp.c
 * PURPOSE:     Internet Control Message Protocol routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"

#include <icmp.h>

NTSTATUS ICMPStartup()
{
    IPRegisterProtocol(IPPROTO_ICMP, ICMPReceive);

    return STATUS_SUCCESS;
}

NTSTATUS ICMPShutdown()
{
    IPRegisterProtocol(IPPROTO_ICMP, NULL);

    return STATUS_SUCCESS;
}

BOOLEAN PrepareICMPPacket(
    PADDRESS_FILE AddrFile,
    PIP_INTERFACE Interface,
    PIP_PACKET IPPacket,
    PIP_ADDRESS Destination,
    PCHAR Data,
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
    PNDIS_PACKET NdisPacket;
    NDIS_STATUS NdisStatus;
    PIPv4_HEADER IPHeader;
    ULONG Size;

    TI_DbgPrint(DEBUG_ICMP, ("Called. DataSize (%d).\n", DataSize));

    IPInitializePacket(IPPacket, IP_ADDRESS_V4);

    /* No special flags */
    IPPacket->Flags = 0;

    Size = sizeof(IPv4_HEADER) + DataSize;

    /* Allocate NDIS packet */
    NdisStatus = AllocatePacketWithBuffer( &NdisPacket, NULL, Size );

    if( !NT_SUCCESS(NdisStatus) ) return FALSE;

    IPPacket->NdisPacket = NdisPacket;
    IPPacket->MappedHeader = TRUE;

    GetDataPtr( IPPacket->NdisPacket, 0,
		(PCHAR *)&IPPacket->Header, &IPPacket->TotalSize );
    ASSERT(IPPacket->TotalSize == Size);

    TI_DbgPrint(DEBUG_ICMP, ("Size (%d). Data at (0x%X).\n", Size, Data));
    TI_DbgPrint(DEBUG_ICMP, ("NdisPacket at (0x%X).\n", NdisPacket));

    IPPacket->HeaderSize = sizeof(IPv4_HEADER);
    IPPacket->Data = ((PCHAR)IPPacket->Header) + IPPacket->HeaderSize;

    TI_DbgPrint(DEBUG_ICMP, ("Copying Address: %x -> %x\n",
			     &IPPacket->DstAddr, Destination));

    RtlCopyMemory(&IPPacket->DstAddr, Destination, sizeof(IP_ADDRESS));
    RtlCopyMemory(IPPacket->Data, Data, DataSize);

    /* Build IPv4 header. FIXME: IPv4 only */

    IPHeader = (PIPv4_HEADER)IPPacket->Header;

    /* Version = 4, Length = 5 DWORDs */
    IPHeader->VerIHL = 0x45;
    /* Normal Type-of-Service */
    IPHeader->Tos = 0;
    /* Length of data and header */
    IPHeader->TotalLength = WH2N((USHORT)DataSize + sizeof(IPv4_HEADER));
    /* Identification */
    IPHeader->Id = (USHORT)Random();
    /* One fragment at offset 0 */
    IPHeader->FlagsFragOfs = 0;
    /* Set TTL */
    if (AddrFile)
        IPHeader->Ttl = AddrFile->TTL;
    else
        IPHeader->Ttl = 128;
    /* Internet Control Message Protocol */
    IPHeader->Protocol = IPPROTO_ICMP;
    /* Checksum is 0 (for later calculation of this) */
    IPHeader->Checksum = 0;
    /* Source address */
    IPHeader->SrcAddr = Interface->Unicast.Address.IPv4Address;
    /* Destination address */
    IPHeader->DstAddr = Destination->Address.IPv4Address;


    TI_DbgPrint(MID_TRACE,("Leaving\n"));

    return TRUE;
}

NTSTATUS ICMPSendDatagram(
    PADDRESS_FILE AddrFile,
    PTDI_CONNECTION_INFORMATION ConnInfo,
    PCHAR BufferData,
    ULONG DataSize,
    PULONG DataUsed )
/*
 * FUNCTION: Sends an ICMP datagram to a remote address
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
    IP_ADDRESS RemoteAddress,  LocalAddress;
    NTSTATUS Status;
    PNEIGHBOR_CACHE_ENTRY NCE;
    KIRQL OldIrql;

    TI_DbgPrint(MID_TRACE,("Sending Datagram(%x %x %x %d)\n",
			   AddrFile, ConnInfo, BufferData, DataSize));
    TI_DbgPrint(MID_TRACE,("RemoteAddressTa: %x\n", RemoteAddressTa));

    switch( RemoteAddressTa->Address[0].AddressType ) {
        case TDI_ADDRESS_TYPE_IP:
            RemoteAddress.Type = IP_ADDRESS_V4;
            RemoteAddress.Address.IPv4Address =
                RemoteAddressTa->Address[0].Address[0].in_addr;
            break;

        default:
            return STATUS_UNSUCCESSFUL;
    }

    TI_DbgPrint(MID_TRACE,("About to get route to destination\n"));

    LockObject(AddrFile, &OldIrql);

    LocalAddress = AddrFile->Address;
    if (AddrIsUnspecified(&LocalAddress))
    {
        /* If the local address is unspecified (0),
         * then use the unicast address of the
         * interface we're sending over
         */
        if(!(NCE = RouteGetRouteToDestination( &RemoteAddress )))
        {
            UnlockObject(AddrFile, OldIrql);
            return STATUS_NETWORK_UNREACHABLE;
        }

        LocalAddress = NCE->Interface->Unicast;
    }
    else
    {
        if(!(NCE = NBLocateNeighbor( &LocalAddress, NULL )))
        {
            UnlockObject(AddrFile, OldIrql);
            return STATUS_INVALID_PARAMETER;
        }
    }

    Status = PrepareICMPPacket( AddrFile,
                                NCE->Interface,
                                &Packet,
                                &RemoteAddress,
                                BufferData,
                                DataSize );

    UnlockObject(AddrFile, OldIrql);

    if( !NT_SUCCESS(Status) )
        return Status;

    TI_DbgPrint(MID_TRACE,("About to send datagram\n"));

    Status = IPSendDatagram(&Packet, NCE);
    if (!NT_SUCCESS(Status))
        return Status;
    
    *DataUsed = DataSize;

    TI_DbgPrint(MID_TRACE,("Leaving\n"));

    return STATUS_SUCCESS;
}


VOID ICMPReceive(
    PIP_INTERFACE Interface,
    PIP_PACKET IPPacket)
/*
 * FUNCTION: Receives an ICMP packet
 * ARGUMENTS:
 *     NTE      = Pointer to net table entry which the packet was received on
 *     IPPacket = Pointer to an IP packet that was received
 */
{
    PICMP_HEADER ICMPHeader;

    TI_DbgPrint(DEBUG_ICMP, ("Called.\n"));

    ICMPHeader = (PICMP_HEADER)IPPacket->Data;

    TI_DbgPrint(DEBUG_ICMP, ("Size (%d).\n", IPPacket->TotalSize));

    TI_DbgPrint(DEBUG_ICMP, ("HeaderSize (%d).\n", IPPacket->HeaderSize));

    TI_DbgPrint(DEBUG_ICMP, ("Type (%d).\n", ICMPHeader->Type));

    TI_DbgPrint(DEBUG_ICMP, ("Code (%d).\n", ICMPHeader->Code));

    TI_DbgPrint(DEBUG_ICMP, ("Checksum (0x%X).\n", ICMPHeader->Checksum));

    /* Checksum ICMP header and data */
    if (!IPv4CorrectChecksum(IPPacket->Data, IPPacket->TotalSize - IPPacket->HeaderSize)) {
        TI_DbgPrint(DEBUG_ICMP, ("Bad ICMP checksum.\n"));
        /* Discard packet */
        return;
    }

    RawIpReceive(Interface, IPPacket);

    switch (ICMPHeader->Type) {
        case ICMP_TYPE_ECHO_REQUEST:
            ICMPReply( Interface, IPPacket, ICMP_TYPE_ECHO_REPLY, 0 );
            break;

        case ICMP_TYPE_ECHO_REPLY:
            break;

        default:
            TI_DbgPrint(DEBUG_ICMP,
                        ("Discarded ICMP datagram of unknown type %d.\n",
                         ICMPHeader->Type));
            /* Discard packet */
            break;
    }
}


VOID ICMPTransmit(
    PIP_PACKET IPPacket,
    PIP_TRANSMIT_COMPLETE Complete,
    PVOID Context)
/*
 * FUNCTION: Transmits an ICMP packet
 * ARGUMENTS:
 *     NTE      = Pointer to net table entry to use (NULL if don't care)
 *     IPPacket = Pointer to IP packet to transmit
 */
{
    PNEIGHBOR_CACHE_ENTRY NCE;

    TI_DbgPrint(DEBUG_ICMP, ("Called.\n"));

    /* Calculate checksum of ICMP header and data */
    ((PICMP_HEADER)IPPacket->Data)->Checksum = (USHORT)
        IPv4Checksum(IPPacket->Data, IPPacket->TotalSize - IPPacket->HeaderSize, 0);

    /* Get a route to the destination address */
    if ((NCE = RouteGetRouteToDestination(&IPPacket->DstAddr))) {
        /* Send the packet */
        IPSendDatagram(IPPacket, NCE);
    } else {
        /* No route to destination (or no free resources) */
        TI_DbgPrint(DEBUG_ICMP, ("No route to destination address 0x%X.\n",
                                 IPPacket->DstAddr.Address.IPv4Address));
        IPPacket->Free(IPPacket);
    }
}


VOID ICMPReply(
    PIP_INTERFACE Interface,
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
    IP_PACKET NewPacket;

    TI_DbgPrint(DEBUG_ICMP, ("Called. Type (%d)  Code (%d).\n", Type, Code));

    DataSize = IPPacket->TotalSize - IPPacket->HeaderSize;

    if( !PrepareICMPPacket(NULL, Interface, &NewPacket, &IPPacket->SrcAddr,
			   IPPacket->Data, DataSize) ) return;

    ((PICMP_HEADER)NewPacket.Data)->Type     = Type;
    ((PICMP_HEADER)NewPacket.Data)->Code     = Code;
    ((PICMP_HEADER)NewPacket.Data)->Checksum = 0;

    ICMPTransmit(&NewPacket, NULL, NULL);
}

/* EOF */
