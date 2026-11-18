/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        drivers/network/tcpip/ip/network/ndp.c
 * PURPOSE:     Neighbor Discovery Solicitation and Advertisement
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 *
 * Neighbor Solicitation and Advertisement from RFC 4861 Sections 4.3, 4.4,
 * 4.6, and 7.2 (https://www.rfc-editor.org/rfc/rfc4861). Link-local and
 * solicited-node addressing from RFC 4291 Sections 2.5.6 and 2.7.1
 * (https://www.rfc-editor.org/rfc/rfc4291). Ethernet IPv6 multicast mapping
 * from RFC 2464 Section 7 (https://www.rfc-editor.org/rfc/rfc2464).
 * ICMPv6 checksum uses IPv6PseudoChecksum (RFC 8200 Section 8.1).
 */

#include "precomp.h"

#include <icmp.h>
#include <ndp.h>
#include <checksum.h>

static VOID
NDPTransmitComplete(
    PVOID Context,
    PNDIS_PACKET NdisPacket,
    NDIS_STATUS NdisStatus)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(NdisStatus);
    FreeNdisPacket(NdisPacket);
}

static VOID
NDPMapMulticastEthernet(
    PUCHAR LinkAddress,
    PIP_ADDRESS Multicast)
/*
 * FUNCTION: Map an IPv6 multicast address to Ethernet (RFC 2464 Section 7)
 */
{
    UCHAR Bytes[16];

    RtlCopyMemory(Bytes, Multicast->Address.IPv6Address, sizeof(Bytes));
    LinkAddress[0] = 0x33;
    LinkAddress[1] = 0x33;
    LinkAddress[2] = Bytes[12];
    LinkAddress[3] = Bytes[13];
    LinkAddress[4] = Bytes[14];
    LinkAddress[5] = Bytes[15];
}

static PND_OPTION_LINKADDR
NDPFindLinkOption(
    PVOID Options,
    UINT OptionsLength,
    UINT8 Type)
{
    PUCHAR Cursor = (PUCHAR)Options;
    UINT Remaining = OptionsLength;

    while (Remaining >= 2)
    {
        UINT8 OptType = Cursor[0];
        UINT8 OptLen = Cursor[1];
        UINT OptBytes;

        if (OptLen == 0)
            return NULL;

        OptBytes = (UINT)OptLen * 8;
        if (OptBytes > Remaining)
            return NULL;

        if (OptType == Type && OptBytes >= sizeof(ND_OPTION_LINKADDR))
            return (PND_OPTION_LINKADDR)Cursor;

        Cursor += OptBytes;
        Remaining -= OptBytes;
    }

    return NULL;
}

static BOOLEAN
NDPTransmitIPv6(
    PIP_INTERFACE Interface,
    PIP_ADDRESS SrcAddress,
    PIP_ADDRESS DstAddress,
    PVOID IcmpPayload,
    UINT PayloadLength,
    PVOID LinkAddress)
/*
 * FUNCTION: Build an IPv6+ICMPv6 packet and send on the link
 */
{
    PNDIS_PACKET NdisPacket;
    NDIS_STATUS NdisStatus;
    PIPv6_HEADER Header;
    PVOID Data;
    UINT Contig;
    UINT TotalSize;
    USHORT Checksum;

    TotalSize = IPv6_HEADER_SIZE + PayloadLength;
    NdisStatus = AllocatePacketWithBuffer(&NdisPacket, NULL, TotalSize);
    if (!NT_SUCCESS(NdisStatus))
        return FALSE;

    GetDataPtr(NdisPacket, 0, (PCHAR *)&Data, &Contig);
    if (!Data || Contig < TotalSize)
    {
        FreeNdisPacket(NdisPacket);
        return FALSE;
    }

    RtlZeroMemory(Data, TotalSize);
    Header = (PIPv6_HEADER)Data;
    Header->VTF = DH2N(0x60000000);
    Header->PayloadLength = WH2N((USHORT)PayloadLength);
    Header->NextHeader = IPPROTO_ICMPV6;
    Header->HopLimit = 255;
    RtlCopyMemory(Header->SrcAddr, SrcAddress->Address.IPv6Address,
                  sizeof(IPv6_RAW_ADDRESS));
    RtlCopyMemory(Header->DstAddr, DstAddress->Address.IPv6Address,
                  sizeof(IPv6_RAW_ADDRESS));
    RtlCopyMemory((PUCHAR)Data + IPv6_HEADER_SIZE, IcmpPayload, PayloadLength);

    ((PICMPV6_HEADER)((PUCHAR)Data + IPv6_HEADER_SIZE))->Checksum = 0;
    Checksum = IPv6PseudoChecksum(SrcAddress, DstAddress,
                                  (USHORT)PayloadLength, IPPROTO_ICMPV6,
                                  (PUCHAR)Data + IPv6_HEADER_SIZE,
                                  PayloadLength);
    ((PICMPV6_HEADER)((PUCHAR)Data + IPv6_HEADER_SIZE))->Checksum = Checksum;

    ASSERT_KM_POINTER(PC(NdisPacket));
    PC(NdisPacket)->DLComplete = NDPTransmitComplete;

    (*Interface->Transmit)(Interface->Context, NdisPacket, 0,
                           LinkAddress, LAN_PROTO_IPv6);
    return TRUE;
}

BOOLEAN NDPSendSolicit(
    PIP_INTERFACE Interface,
    PIP_ADDRESS Target)
/*
 * FUNCTION: Send Neighbor Solicitation for Target (RFC 4861 Section 7.2.2)
 */
{
    UCHAR Buffer[sizeof(ND_NEIGHBOR_SOLICIT) + sizeof(ND_OPTION_LINKADDR)];
    PND_NEIGHBOR_SOLICIT Ns;
    PND_OPTION_LINKADDR Opt;
    IP_ADDRESS DstMulticast;
    IP_ADDRESS SrcAddress;
    UCHAR LinkAddress[6];
    UINT PayloadLength;

    if (!Interface || !Target || Target->Type != IP_ADDRESS_V6)
        return FALSE;

    if (AddrIsUnspecified(&Interface->LinkLocal))
        return FALSE;

    if (Interface->AddressLength != 6 || !Interface->Address)
        return FALSE;

    AddrSolicitedNodeMulticast(&DstMulticast, Target);
    SrcAddress = Interface->LinkLocal;
    NDPMapMulticastEthernet(LinkAddress, &DstMulticast);

    RtlZeroMemory(Buffer, sizeof(Buffer));
    Ns = (PND_NEIGHBOR_SOLICIT)Buffer;
    Ns->Type = ICMPV6_TYPE_NEIGHBOR_SOLICIT;
    Ns->Code = 0;
    RtlCopyMemory(Ns->Target, Target->Address.IPv6Address,
                  sizeof(IPv6_RAW_ADDRESS));

    Opt = (PND_OPTION_LINKADDR)(Buffer + sizeof(ND_NEIGHBOR_SOLICIT));
    Opt->Type = ND_OPT_SOURCE_LINKADDR;
    Opt->Length = 1;
    RtlCopyMemory(Opt->Address, Interface->Address, 6);
    PayloadLength = sizeof(Buffer);

    TI_DbgPrint(DEBUG_NCACHE, ("NDP NS for %s\n", A2S(Target)));

    return NDPTransmitIPv6(Interface, &SrcAddress, &DstMulticast,
                           Buffer, PayloadLength, LinkAddress);
}

VOID NDPReceiveSolicit(
    PIP_INTERFACE Interface,
    PIP_PACKET IPPacket)
/*
 * FUNCTION: Handle Neighbor Solicitation (RFC 4861 Section 7.2.3 / 7.2.4)
 */
{
    PND_NEIGHBOR_SOLICIT Ns;
    PND_OPTION_LINKADDR Slla;
    UINT DataSize;
    IP_ADDRESS Target;
    UCHAR NaBuffer[sizeof(ND_NEIGHBOR_ADVERT) + sizeof(ND_OPTION_LINKADDR)];
    PND_NEIGHBOR_ADVERT Na;
    PND_OPTION_LINKADDR Tlla;
    PNEIGHBOR_CACHE_ENTRY NCE;

    DataSize = IPPacket->TotalSize - IPPacket->HeaderSize;
    if (DataSize < sizeof(ND_NEIGHBOR_SOLICIT))
        return;

    if (AddrIsUnspecified(&Interface->LinkLocal))
        return;

    Ns = (PND_NEIGHBOR_SOLICIT)IPPacket->Data;
    Target.Type = IP_ADDRESS_V6;
    RtlCopyMemory(Target.Address.IPv6Address, Ns->Target,
                  sizeof(IPv6_RAW_ADDRESS));

    if (!AddrIsEqual(&Target, &Interface->LinkLocal))
        return;

    Slla = NDPFindLinkOption((PUCHAR)IPPacket->Data + sizeof(ND_NEIGHBOR_SOLICIT),
                             DataSize - sizeof(ND_NEIGHBOR_SOLICIT),
                             ND_OPT_SOURCE_LINKADDR);
    if (Slla && !AddrIsUnspecified(&IPPacket->SrcAddr))
    {
        NCE = NBLocateNeighbor(&IPPacket->SrcAddr, Interface);
        if (NCE)
            NBUpdateNeighbor(NCE, Slla->Address, 0);
        else
            NBAddNeighbor(Interface, &IPPacket->SrcAddr, Slla->Address,
                          Interface->AddressLength, 0, ARP_COMPLETE_TIMEOUT);
    }

    if (AddrIsUnspecified(&IPPacket->SrcAddr))
    {
        /* DAD probe: no NA to unspecified source (RFC 4861 7.2.4). */
        return;
    }

    if (Interface->AddressLength != 6 || !Interface->Address)
        return;

    RtlZeroMemory(NaBuffer, sizeof(NaBuffer));
    Na = (PND_NEIGHBOR_ADVERT)NaBuffer;
    Na->Type = ICMPV6_TYPE_NEIGHBOR_ADVERT;
    Na->Code = 0;
    Na->FlagsReserved = DH2N(ND_NA_FLAG_SOLICITED | ND_NA_FLAG_OVERRIDE);
    RtlCopyMemory(Na->Target, Interface->LinkLocal.Address.IPv6Address,
                  sizeof(IPv6_RAW_ADDRESS));
    Tlla = (PND_OPTION_LINKADDR)(NaBuffer + sizeof(ND_NEIGHBOR_ADVERT));
    Tlla->Type = ND_OPT_TARGET_LINKADDR;
    Tlla->Length = 1;
    RtlCopyMemory(Tlla->Address, Interface->Address, 6);

    NCE = RouteGetRouteToDestination(&IPPacket->SrcAddr);
    if (!NCE)
        return;

    {
        IP_PACKET NewPacket;
        ADDRESS_FILE FakeAddrFile;

        FakeAddrFile.Protocol = IPPROTO_ICMPV6;
        FakeAddrFile.TTL = 255;

        if (!NT_SUCCESS(BuildRawIpPacket(
                &FakeAddrFile, &NewPacket, &IPPacket->SrcAddr, 0,
                &Interface->LinkLocal, 0, NaBuffer, sizeof(NaBuffer))))
        {
            return;
        }

        ((PICMPV6_HEADER)NewPacket.Data)->Checksum = 0;
        ((PICMPV6_HEADER)NewPacket.Data)->Checksum =
            IPv6PseudoChecksum(&Interface->LinkLocal, &IPPacket->SrcAddr,
                               (USHORT)sizeof(NaBuffer), IPPROTO_ICMPV6,
                               NewPacket.Data, sizeof(NaBuffer));
        IPSendDatagram(&NewPacket, NCE);
    }
}

VOID NDPReceiveAdvert(
    PIP_INTERFACE Interface,
    PIP_PACKET IPPacket)
/*
 * FUNCTION: Handle Neighbor Advertisement (RFC 4861 Section 7.2.5)
 */
{
    PND_NEIGHBOR_ADVERT Na;
    PND_OPTION_LINKADDR Tlla;
    UINT DataSize;
    IP_ADDRESS Target;
    PNEIGHBOR_CACHE_ENTRY NCE;

    DataSize = IPPacket->TotalSize - IPPacket->HeaderSize;
    if (DataSize < sizeof(ND_NEIGHBOR_ADVERT))
        return;

    Na = (PND_NEIGHBOR_ADVERT)IPPacket->Data;
    Target.Type = IP_ADDRESS_V6;
    RtlCopyMemory(Target.Address.IPv6Address, Na->Target,
                  sizeof(IPv6_RAW_ADDRESS));

    Tlla = NDPFindLinkOption((PUCHAR)IPPacket->Data + sizeof(ND_NEIGHBOR_ADVERT),
                             DataSize - sizeof(ND_NEIGHBOR_ADVERT),
                             ND_OPT_TARGET_LINKADDR);
    if (!Tlla)
        return;

    NCE = NBLocateNeighbor(&Target, Interface);
    if (NCE)
    {
        NBUpdateNeighbor(NCE, Tlla->Address, 0);
    }
    else
    {
        NBAddNeighbor(Interface, &Target, Tlla->Address,
                      Interface->AddressLength, 0, ARP_COMPLETE_TIMEOUT);
    }
}

/* EOF */
