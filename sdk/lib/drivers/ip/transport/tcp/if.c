
#include "precomp.h"

#include "lwip/pbuf.h"
#include "lwip/netifapi.h"
#include "lwip/ip.h"
#include "lwip/api.h"
#include "lwip/tcpip.h"

void
TCPPrintNetifInfo(struct netif *netif);

#ifndef DEBUG
#define DPRINT_NETIF(_netif)
#else
#define DPRINT_NETIF(_netif) TCPPrintNetifInfo(_netif)
#endif

void
TCPPrintNetifInfo(struct netif *netif) {
    DbgPrint("########## NETIF INFOS ##########\n");
    
    DbgPrint("Memory address:    %p\n", netif);
    DbgPrint("Next interface:    %p\n\n", netif->next);

    DbgPrint("IP Address:        %u.%u.%u.%u\n",
        (netif->ip_addr.addr      ) & 0xFF, (netif->ip_addr.addr >> 8 ) & 0xFF,
        (netif->ip_addr.addr >> 16) & 0xFF, (netif->ip_addr.addr >> 24) & 0xFF);
    DbgPrint("Input  fn:         %p\n", netif->input);
    DbgPrint("Output fn:         %p\n", netif->output);
    DbgPrint("State:             %p\n", netif->state);
    DbgPrint("MTU:               %u\n", netif->mtu);
    DbgPrint("HW Address Length: %lu\n", netif->hwaddr_len);
    if (netif->hwaddr_len == 6U)
        DbgPrint("HW Address:    %02x:%02x:%02x:%02x:%02x:%02x\n",
            netif->hwaddr[0], netif->hwaddr[1],
            netif->hwaddr[2], netif->hwaddr[3],
            netif->hwaddr[4], netif->hwaddr[5]);
    DbgPrint("Flags (%u): \n", netif->flags);
    if(netif->flags & NETIF_FLAG_UP)
        DbgPrint("\t- NETIF_FLAG_UP\n");
    if(netif->flags & NETIF_FLAG_BROADCAST)
        DbgPrint("\t- NETIF_FLAG_BROADCAST\n");
    if(netif->flags & NETIF_FLAG_LINK_UP)
        DbgPrint("\t- NETIF_FLAG_LINK_UP\n");
    if(netif->flags & NETIF_FLAG_ETHARP)
        DbgPrint("\t- NETIF_FLAG_ETHARP\n");
    if(netif->flags & NETIF_FLAG_ETHERNET)
        DbgPrint("\t- NETIF_FLAG_ETHERNET\n");
    if(netif->flags & NETIF_FLAG_IGMP)
        DbgPrint("\t- NETIF_FLAG_IGMP\n");
    if(netif->flags & NETIF_FLAG_MLD6)
        DbgPrint("\t- NETIF_FLAG_MLD6\n");
    DbgPrint("Interface name:    %c%c\n", netif->name[0], netif->name[1]);
    DbgPrint("Interface number:  %u\n", netif->num);

    DbgPrint("#################################\n");
}

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

    return ERR_OK;
}

// FIXME: Implement this function
// Remove LINK_UP flag from TCPInterfaceInit
VOID
TCPUpdateInterfaceLinkStatus(PIP_INTERFACE IF)
{
    DPRINT1("fixme: TCPUpdateInterfaceLinkStatus() not implemented!");
#if 0
    ULONG OperationalStatus;

    GetInterfaceConnectionStatus(IF, &OperationalStatus);

    if (OperationalStatus == MIB_IF_OPER_STATUS_OPERATIONAL)
        netif_set_link_up(IF->TCPContext);
    else
        netif_set_link_down(IF->TCPContext);
#endif
}

err_t
TCPInterfaceInit(struct netif *netif)
{
    PIP_INTERFACE IF = netif->state;

    DPRINT1("### TCP Interface Init (%p)\n", IF);

    RtlCopyMemory(netif->hwaddr, IF->Address, IF->AddressLength);

    netif->hwaddr_len = IF->AddressLength;

    netif->output = TCPSendDataCallback;
    netif->mtu = IF->MTU;

    netif->name[0] = 'e';
    netif->name[1] = 'n';

    netif->flags |= NETIF_FLAG_BROADCAST;
    netif->flags |= NETIF_FLAG_LINK_UP;

    DPRINT_NETIF(netif);

    TCPUpdateInterfaceLinkStatus(IF);
    TCPUpdateInterfaceIPInformation(IF);

    return ERR_OK;
}

VOID
TCPRegisterInterface(PIP_INTERFACE IF)
{
    ip_addr_t ipaddr;
    ip_addr_t netmask;
    ip_addr_t gw;

    DPRINT1("### TCP Register Interface (%p)\n", IF);

    gw.addr = 0;
    ipaddr.addr = 0;
    netmask.addr = 0;

    DPRINT1("(%p):[%p] TCP Registering interface\n", IF, IF->TCPContext);

    IF->TCPContext = netif_add(IF->TCPContext,
                               &ipaddr,
                               &netmask,
                               &gw,
                               IF,
                               TCPInterfaceInit,
                               tcpip_input);

    DPRINT_NETIF(IF->TCPContext);
}

VOID
TCPUnregisterInterface(PIP_INTERFACE IF)
{
    DPRINT1("### TCP Unregister Interface (%p)\n", IF);
    DPRINT1("(%p):[%p] Unregistering netif interface\n", IF, IF->TCPContext);
    netif_remove(IF->TCPContext);
    DPRINT1("(%p):[%p] Unregistered\n", IF, IF->TCPContext);
}

VOID
TCPUpdateInterfaceIPInformation(PIP_INTERFACE IF)
{
    ip4_addr_t ipaddr;
    ip4_addr_t netmask;

    DPRINT1("### TCP Update Interface IP Information (%p):[%p]\n", IF, IF->TCPContext);

    GetInterfaceIPv4Address(IF,
                            ADE_UNICAST,
                            (PULONG)&ipaddr.addr);

    GetInterfaceIPv4Address(IF,
                            ADE_ADDRMASK,
                            (PULONG)&netmask.addr);

    netif_set_ipaddr(IF->TCPContext, &ipaddr);
    netif_set_netmask(IF->TCPContext, &netmask);

    if (ipaddr.addr != 0)
    {
        netif_set_up(IF->TCPContext);
        netif_set_default(IF->TCPContext);
    }
    else
    {
        netif_set_down(IF->TCPContext);
    }

    DPRINT_NETIF(IF->TCPContext);
    DPRINT1("(%p):[%p] Interface is %s\n", IF, IF->TCPContext, ipaddr.addr ? "up" : "down");
}
