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
    TI_DbgPrint(DEBUG_ICMP, ("Sending ICMP datagram (0x%x)\n", AddrFile));

    /* just forward the call to RawIP handler */
    return RawIPSendDatagram(AddrFile, ConnInfo, BufferData, DataSize, DataUsed);
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
    PICMP_HEADER ICMPHeader = (PICMP_HEADER)IPPacket->Data;
    UINT32 DataSize = IPPacket->TotalSize - IPPacket->HeaderSize;

    TI_DbgPrint(DEBUG_ICMP, ("ICMPReceive: Size (%d) HeaderSize (%d) Type (%d) Code (%d) Checksum (0x%x)\n",
        IPPacket->TotalSize, IPPacket->HeaderSize, ICMPHeader->Type, ICMPHeader->Code, ICMPHeader->Checksum));

    /* Discard too short packets */
    if (DataSize < sizeof(ICMP_HEADER))
    {
        TI_DbgPrint(DEBUG_ICMP, ("Packet doesn't fit ICMP header. Discarded\n"));
        return;
    }

    /* Discard packets with bad checksum */
    if (!IPv4CorrectChecksum(IPPacket->Data, DataSize))
    {
        TI_DbgPrint(DEBUG_ICMP, ("Bad ICMP checksum. Packet discarded\n"));
        return;
    }

    RawIpReceive(Interface, IPPacket);

    if (ICMPHeader->Type == ICMP_TYPE_ECHO_REQUEST)
    {
        ICMPReply(Interface, IPPacket, ICMP_TYPE_ECHO_REPLY, 0);
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
    ADDRESS_FILE FakeAddrFile;
    PNEIGHBOR_CACHE_ENTRY NCE;

    TI_DbgPrint(DEBUG_ICMP, ("Called. Type (%d)  Code (%d).\n", Type, Code));

    DataSize = IPPacket->TotalSize - IPPacket->HeaderSize;

    /* First check if we have a route to sender */
    NCE = RouteGetRouteToDestination(&IPPacket->SrcAddr);
    if (!NCE)
    {
        return;
    }

    /* This is the only data needed to generate a packet */
    FakeAddrFile.Protocol = IPPROTO_ICMP;
    FakeAddrFile.TTL = 128;

    if (!NT_SUCCESS(BuildRawIpPacket(
        &FakeAddrFile, &NewPacket, &IPPacket->SrcAddr, 0, &Interface->Unicast, 0, IPPacket->Data, DataSize)))
    {
        return;
    }

    ((PICMP_HEADER)NewPacket.Data)->Type     = Type;
    ((PICMP_HEADER)NewPacket.Data)->Code     = Code;
    ((PICMP_HEADER)NewPacket.Data)->Checksum = 0;
    ((PICMP_HEADER)NewPacket.Data)->Checksum = (USHORT)IPv4Checksum(NewPacket.Data, DataSize, 0);

    IPSendDatagram(&NewPacket, NCE);
}

/* EOF */
