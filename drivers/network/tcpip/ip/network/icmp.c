/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        network/icmp.c
 * PURPOSE:     Internet Control Message Protocol routines
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Ryan Wilson <ryan@verosnt.com>
 * REVISIONS:
 *   CSH 01/08-2000 Created
 *
 * ICMPv6 echo request/reply from RFC 4443 Sections 4.1-4.2
 * (https://www.rfc-editor.org/rfc/rfc4443). IPv6 pseudo-header checksum
 * from RFC 8200 Section 8.1 (https://www.rfc-editor.org/rfc/rfc8200).
 * Neighbor Solicitation/Advertisement dispatched per RFC 4861
 * (https://www.rfc-editor.org/rfc/rfc4861).
 */

#include "precomp.h"

#include <icmp.h>
#include <ndp.h>
#include <checksum.h>

NTSTATUS ICMPStartup()
{
    IPRegisterProtocol(IPPROTO_ICMP, ICMPReceive);
    IPRegisterProtocol(IPPROTO_ICMPV6, ICMP6Receive);

    return STATUS_SUCCESS;
}

NTSTATUS ICMPShutdown()
{
    IPRegisterProtocol(IPPROTO_ICMP, NULL);
    IPRegisterProtocol(IPPROTO_ICMPV6, NULL);

    return STATUS_SUCCESS;
}

USHORT IPv6PseudoChecksum(
    PIP_ADDRESS SrcAddress,
    PIP_ADDRESS DstAddress,
    USHORT Length,
    UCHAR NextHeader,
    PVOID Data,
    UINT DataLength)
/*
 * FUNCTION: Compute ICMPv6 checksum with IPv6 pseudo-header (RFC 8200 8.1)
 */
{
    ULONG Sum;
    UCHAR Pseudo[40];
    ULONG LengthHost;
    ULONG NextHost;

    RtlZeroMemory(Pseudo, sizeof(Pseudo));
    RtlCopyMemory(Pseudo, SrcAddress->Address.IPv6Address, 16);
    RtlCopyMemory(Pseudo + 16, DstAddress->Address.IPv6Address, 16);
    LengthHost = Length;
    NextHost = NextHeader;
    /* Upper-Layer Packet Length and Next Header as 32-bit big-endian fields. */
    Pseudo[32] = (UCHAR)((LengthHost >> 24) & 0xFF);
    Pseudo[33] = (UCHAR)((LengthHost >> 16) & 0xFF);
    Pseudo[34] = (UCHAR)((LengthHost >> 8) & 0xFF);
    Pseudo[35] = (UCHAR)(LengthHost & 0xFF);
    Pseudo[39] = (UCHAR)(NextHost & 0xFF);

    Sum = ChecksumCompute(Pseudo, sizeof(Pseudo), 0);
    Sum = ChecksumCompute(Data, DataLength, Sum);
    return (USHORT)(~ChecksumFold(Sum));
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

VOID ICMP6Receive(
    PIP_INTERFACE Interface,
    PIP_PACKET IPPacket)
/*
 * FUNCTION: Receives an ICMPv6 packet (RFC 4443)
 */
{
    PICMPV6_HEADER IcmpHeader = (PICMPV6_HEADER)IPPacket->Data;
    UINT32 DataSize = IPPacket->TotalSize - IPPacket->HeaderSize;
    USHORT Checksum;

    TI_DbgPrint(DEBUG_ICMP, ("ICMP6Receive: Size (%d) Type (%d) Code (%d)\n",
        DataSize, IcmpHeader->Type, IcmpHeader->Code));

    if (DataSize < sizeof(ICMPV6_HEADER))
    {
        TI_DbgPrint(DEBUG_ICMP, ("Packet doesn't fit ICMPv6 header. Discarded\n"));
        return;
    }

    Checksum = IcmpHeader->Checksum;
    IcmpHeader->Checksum = 0;
    if (IPv6PseudoChecksum(&IPPacket->SrcAddr,
                           &IPPacket->DstAddr,
                           (USHORT)DataSize,
                           IPPROTO_ICMPV6,
                           IPPacket->Data,
                           DataSize) != Checksum)
    {
        IcmpHeader->Checksum = Checksum;
        TI_DbgPrint(DEBUG_ICMP, ("Bad ICMPv6 checksum. Packet discarded\n"));
        return;
    }
    IcmpHeader->Checksum = Checksum;

    RawIpReceive(Interface, IPPacket);

    if (IcmpHeader->Type == ICMPV6_TYPE_ECHO_REQUEST)
    {
        ICMP6Reply(Interface, IPPacket, ICMPV6_TYPE_ECHO_REPLY, 0);
    }
    else if (IcmpHeader->Type == ICMPV6_TYPE_NEIGHBOR_SOLICIT)
    {
        NDPReceiveSolicit(Interface, IPPacket);
    }
    else if (IcmpHeader->Type == ICMPV6_TYPE_NEIGHBOR_ADVERT)
    {
        NDPReceiveAdvert(Interface, IPPacket);
    }
}

VOID ICMP6Reply(
    PIP_INTERFACE Interface,
    PIP_PACKET IPPacket,
    UCHAR Type,
    UCHAR Code)
/*
 * FUNCTION: Transmits an ICMPv6 echo reply (RFC 4443 Section 4.2)
 */
{
    UINT DataSize;
    IP_PACKET NewPacket;
    ADDRESS_FILE FakeAddrFile;
    PNEIGHBOR_CACHE_ENTRY NCE;
    PICMPV6_HEADER IcmpHeader;

    TI_DbgPrint(DEBUG_ICMP, ("ICMP6Reply Type (%d) Code (%d).\n", Type, Code));

    DataSize = IPPacket->TotalSize - IPPacket->HeaderSize;

    NCE = RouteGetRouteToDestination(&IPPacket->SrcAddr);
    if (!NCE)
    {
        return;
    }

    FakeAddrFile.Protocol = IPPROTO_ICMPV6;
    FakeAddrFile.TTL = 255;

    if (!NT_SUCCESS(BuildRawIpPacket(
        &FakeAddrFile, &NewPacket, &IPPacket->SrcAddr, 0,
        &IPPacket->DstAddr, 0, IPPacket->Data, DataSize)))
    {
        return;
    }

    IcmpHeader = (PICMPV6_HEADER)NewPacket.Data;
    IcmpHeader->Type = Type;
    IcmpHeader->Code = Code;
    IcmpHeader->Checksum = 0;
    /* Reply source is the request destination (RFC 4443 Section 4.2). */
    IcmpHeader->Checksum = IPv6PseudoChecksum(&IPPacket->DstAddr,
                                              &IPPacket->SrcAddr,
                                              (USHORT)DataSize,
                                              IPPROTO_ICMPV6,
                                              NewPacket.Data,
                                              DataSize);

    IPSendDatagram(&NewPacket, NCE);
}

/* EOF */
