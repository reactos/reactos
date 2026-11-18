/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        transport/tcp/if.c
 * PURPOSE:     lwIP netif glue for IPv4 and IPv6
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 *
 * Dual-stack ip_addr_t accessors from lwIP ip_addr.h. IPv6 link-local on
 * the netif mirrors the VerOS LinkLocal formed per RFC 4291 Section 2.5.6
 * (https://www.rfc-editor.org/rfc/rfc4291). Neighbor resolution for this
 * output path uses the VerOS NCE (same pattern as IPv4 with LWIP_ARP=0).
 */

#include "precomp.h"

#include "lwip/pbuf.h"
#include "lwip/netifapi.h"
#include "lwip/ip.h"
#include "lwip/ip6_addr.h"
#include "lwip/api.h"
#include "lwip/tcpip.h"
#include <ipifcons.h>

err_t
TCPSendDataCallback(struct netif *netif, struct pbuf *p, const ip4_addr_t *dest)
{
    NDIS_STATUS NdisStatus;
    PNEIGHBOR_CACHE_ENTRY NCE;
    IP_PACKET Packet;
    IP_ADDRESS RemoteAddress, LocalAddress;
    PIPv4_HEADER Header;
    ULONG Length;
    ULONG TotalLength;

    /* The caller frees the pbuf struct */

    if (((*(u8_t*)p->payload) & 0xF0) == 0x40)
    {
        Header = p->payload;

        LocalAddress.Type = IP_ADDRESS_V4;
        LocalAddress.Address.IPv4Address = Header->SrcAddr;

        RemoteAddress.Type = IP_ADDRESS_V4;
        RemoteAddress.Address.IPv4Address = Header->DstAddr;
    }
    else
    {
        return ERR_IF;
    }

    IPInitializePacket(&Packet, LocalAddress.Type);

    if (!(NCE = RouteGetRouteToDestination(&RemoteAddress)))
    {
        return ERR_RTE;
    }

    NdisStatus = AllocatePacketWithBuffer(&Packet.NdisPacket, NULL, p->tot_len);
    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        return ERR_MEM;
    }

    GetDataPtr(Packet.NdisPacket, 0, (PCHAR*)&Packet.Header, &Packet.TotalSize);
    Packet.MappedHeader = TRUE;

    ASSERT(Packet.TotalSize == p->tot_len);

    TotalLength = p->tot_len;
    Length = 0;
    while (Length < TotalLength)
    {
        ASSERT(p->len <= TotalLength - Length);
        ASSERT(p->tot_len == TotalLength - Length);
        RtlCopyMemory((PCHAR)Packet.Header + Length, p->payload, p->len);
        Length += p->len;
        p = p->next;
    }
    ASSERT(Length == TotalLength);

    Packet.HeaderSize = sizeof(IPv4_HEADER);
    Packet.TotalSize = TotalLength;
    Packet.SrcAddr = LocalAddress;
    Packet.DstAddr = RemoteAddress;

    NdisStatus = IPSendDatagram(&Packet, NCE);
    if (!NT_SUCCESS(NdisStatus))
        return ERR_RTE;

    return 0;
}

#if LWIP_IPV6
err_t
TCPSendDataCallbackIPv6(struct netif *netif, struct pbuf *p, const ip6_addr_t *dest)
{
    NDIS_STATUS NdisStatus;
    PNEIGHBOR_CACHE_ENTRY NCE;
    IP_PACKET Packet;
    IP_ADDRESS RemoteAddress, LocalAddress;
    PIPv6_HEADER Header;
    ULONG Length;
    ULONG TotalLength;

    UNREFERENCED_PARAMETER(netif);
    UNREFERENCED_PARAMETER(dest);

    if (((*(u8_t*)p->payload) & 0xF0) != 0x60)
        return ERR_IF;

    Header = p->payload;

    LocalAddress.Type = IP_ADDRESS_V6;
    RtlCopyMemory(LocalAddress.Address.IPv6Address, Header->SrcAddr,
                  sizeof(IPv6_RAW_ADDRESS));
    RemoteAddress.Type = IP_ADDRESS_V6;
    RtlCopyMemory(RemoteAddress.Address.IPv6Address, Header->DstAddr,
                  sizeof(IPv6_RAW_ADDRESS));

    IPInitializePacket(&Packet, IP_ADDRESS_V6);

    if (!(NCE = RouteGetRouteToDestination(&RemoteAddress)))
        return ERR_RTE;

    NdisStatus = AllocatePacketWithBuffer(&Packet.NdisPacket, NULL, p->tot_len);
    if (NdisStatus != NDIS_STATUS_SUCCESS)
        return ERR_MEM;

    GetDataPtr(Packet.NdisPacket, 0, (PCHAR*)&Packet.Header, &Packet.TotalSize);
    Packet.MappedHeader = TRUE;

    TotalLength = p->tot_len;
    Length = 0;
    while (Length < TotalLength)
    {
        RtlCopyMemory((PCHAR)Packet.Header + Length, p->payload, p->len);
        Length += p->len;
        p = p->next;
    }

    Packet.HeaderSize = IPv6_HEADER_SIZE;
    Packet.TotalSize = TotalLength;
    Packet.SrcAddr = LocalAddress;
    Packet.DstAddr = RemoteAddress;

    NdisStatus = IPSendDatagram(&Packet, NCE);
    if (!NT_SUCCESS(NdisStatus))
        return ERR_RTE;

    return ERR_OK;
}
#endif /* LWIP_IPV6 */

VOID
TCPUpdateInterfaceLinkStatus(PIP_INTERFACE IF)
{
    ULONG OperationalStatus;

    GetInterfaceConnectionStatus(IF, &OperationalStatus);

    if (OperationalStatus == MIB_IF_OPER_STATUS_OPERATIONAL)
        netif_set_link_up(IF->TCPContext);
    else
        netif_set_link_down(IF->TCPContext);
}

err_t
TCPInterfaceInit(struct netif *netif)
{
    PIP_INTERFACE IF = netif->state;

    netif->hwaddr_len = IF->AddressLength;
    RtlCopyMemory(netif->hwaddr, IF->Address, netif->hwaddr_len);

    netif->output = TCPSendDataCallback;
#if LWIP_IPV6
    netif->output_ip6 = TCPSendDataCallbackIPv6;
#endif
    netif->mtu = IF->MTU;

    netif->name[0] = 'e';
    netif->name[1] = 'n';

    netif->flags |= NETIF_FLAG_BROADCAST;

    TCPUpdateInterfaceLinkStatus(IF);

    TCPUpdateInterfaceIPInformation(IF);

    return 0;
}

VOID
TCPRegisterInterface(PIP_INTERFACE IF)
{
    ip4_addr_t ipaddr;
    ip4_addr_t netmask;
    ip4_addr_t gw;

    ip4_addr_set_zero(&gw);
    ip4_addr_set_zero(&ipaddr);
    ip4_addr_set_zero(&netmask);

    IF->TCPContext = netif_add(IF->TCPContext,
                               &ipaddr,
                               &netmask,
                               &gw,
                               IF,
                               TCPInterfaceInit,
                               tcpip_input);
}

VOID
TCPUnregisterInterface(PIP_INTERFACE IF)
{
    netif_remove(IF->TCPContext);
}

VOID
TCPUpdateInterfaceIPInformation(PIP_INTERFACE IF)
{
    ip4_addr_t ipaddr;
    ip4_addr_t netmask;
    ip4_addr_t gw;

    ip4_addr_set_zero(&gw);

    GetInterfaceIPv4Address(IF,
                            ADE_UNICAST,
                            (PULONG)&ipaddr.addr);

    GetInterfaceIPv4Address(IF,
                            ADE_ADDRMASK,
                            (PULONG)&netmask.addr);

    netif_set_addr(IF->TCPContext, &ipaddr, &netmask, &gw);

#if LWIP_IPV6
    if (!AddrIsUnspecified(&IF->LinkLocal))
    {
        ip6_addr_t ip6;

        /* Both layouts store the 128-bit address in network byte order. */
        ip6_addr_set_zero(&ip6);
        RtlCopyMemory(ip6.addr, IF->LinkLocal.Address.IPv6Address, 16);
        netif_ip6_addr_set(IF->TCPContext, 0, &ip6);
        netif_ip6_addr_set_state(IF->TCPContext, 0, IP6_ADDR_PREFERRED);
    }
#endif

    if (!ip4_addr_isany_val(ipaddr)
#if LWIP_IPV6
        || !AddrIsUnspecified(&IF->LinkLocal)
#endif
        )
    {
        netif_set_up(IF->TCPContext);
        netif_set_default(IF->TCPContext);
    }
    else
    {
        netif_set_down(IF->TCPContext);
    }
}
