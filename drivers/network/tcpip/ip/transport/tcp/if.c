
#include "precomp.h"

#include "lwip/pbuf.h"
#include "lwip/netifapi.h"
#include "lwip/ip.h"
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
    ip_addr_t ipaddr;
    ip_addr_t netmask;
    ip_addr_t gw;

    gw.addr = 0;
    ipaddr.addr = 0;
    netmask.addr = 0;

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
    ip_addr_t ipaddr;
    ip_addr_t netmask;
    ip_addr_t gw;

    gw.addr = 0;

    GetInterfaceIPv4Address(IF,
                            ADE_UNICAST,
                            (PULONG)&ipaddr.addr);

    GetInterfaceIPv4Address(IF,
                            ADE_ADDRMASK,
                            (PULONG)&netmask.addr);

    netif_set_addr(IF->TCPContext, &ipaddr, &netmask, &gw);

    if (ipaddr.addr != 0)
    {
        netif_set_up(IF->TCPContext);
        netif_set_default(IF->TCPContext);
    }
    else
    {
        netif_set_down(IF->TCPContext);
    }
}
