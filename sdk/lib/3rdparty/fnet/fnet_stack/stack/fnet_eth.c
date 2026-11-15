/**************************************************************************
*
* Copyright 2008-2018 by Andrey Butok. FNET Community.
*
***************************************************************************
*
*  Licensed under the Apache License, Version 2.0 (the "License"); you may
*  not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*  http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
*  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
***************************************************************************
*
* Ethernet platform independent API functions.
*
***************************************************************************/

#include "fnet.h"
#include "fnet_eth_prv.h"
#include "fnet_prot.h"
#include "fnet_stack_prv.h"

/************************************************************************
*     Global Data Structures
*************************************************************************/

/* Null MAC address */
const fnet_mac_addr_t fnet_eth_null_addr = FNET_MAC_ADDR_INIT(0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

/* Broadcast MAC address */
const fnet_mac_addr_t fnet_eth_broadcast = FNET_MAC_ADDR_INIT(0xff, 0xff, 0xff, 0xff, 0xff, 0xff);

/************************************************************************
* DESCRIPTION: Converts MAC address to an null-terminated string.
*************************************************************************/
fnet_char_t *fnet_mac_to_str( const fnet_mac_addr_t addr, fnet_char_t *str_mac )
{
    const fnet_uint8_t *p;
    if(str_mac)
    {
        p = (const fnet_uint8_t *)addr;
        fnet_sprintf(str_mac, "%02X:%02X:%02X:%02X:%02X:%02X", p[0], p[1], p[2], p[3], p[4], p[5]);
    }
    return str_mac;
}

/************************************************************************
* DESCRIPTION: This function interprets the character string into MAC addr.
*************************************************************************/
fnet_return_t fnet_str_to_mac( const fnet_char_t *str_mac, fnet_mac_addr_t addr )
{
    fnet_uint32_t   val;
    fnet_uint8_t    c;
    fnet_uint32_t   octet[6];
    fnet_uint32_t   *octetptr = octet;
    fnet_index_t    i;

    while(1)
    {
        val = 0U;

        while((c = *str_mac) != 0U)
        {
            if((c >= '0') && (c <= '9'))
            {
                val = (val * 16U) + (fnet_uint32_t)(c) - '0';
                str_mac++;
                continue;
            }
            else if(((c >= 'a') && (c <= 'f')) || ((c >= 'A') && (c <= 'F')))
            {
                val = (val << 4U) + ((fnet_uint32_t)c + 10U - (((c >= 'a') && (c <= 'f')) ? (fnet_uint32_t)'a' : (fnet_uint32_t)'A'));
                str_mac++;
                continue;
            }
            else
            {}

            break;
        }

        if(*str_mac == ':')
        {
            *octetptr++ = val;
            str_mac++;

            if(octetptr >= octet + 6)
            {
                goto ERROR;
            }

            continue;
        }

        break;
    }
    /* Check for trailing characters. */
    if((*str_mac) && (!(*str_mac == ' ')))
    {
        goto ERROR;
    }

    *octetptr++ = val;

    if((octetptr - octet) == 6)
    {
        for (i = 0u; i < 6u; i++)
        {
            addr[i] = (fnet_uint8_t)octet[i];
        }
    }
    else
    {
        goto ERROR;
    }

    return (FNET_OK);
ERROR:
    return (FNET_ERR);
}

/* Number of initialised ethernet devices.*/
static fnet_index_t fnet_eth_number = 0U;

/************************************************************************
*     List of Network Layer Protocols used by Ethernet Interface.
*************************************************************************/
static const fnet_eth_prot_if_t fnet_eth_prot_if_list[] =
{
#if FNET_CFG_IP4
    { /* ARP */
        FNET_HTONS(FNET_ETH_TYPE_ARP),  /* Protocol number */
        _fnet_arp_input                  /* Protocol input function.*/
    },
    {   /* IPv4 */
        FNET_HTONS(FNET_ETH_TYPE_IP4),  /* Protocol number */
        _fnet_ip4_input                  /* Protocol input function.*/
    }
#endif /* FNET_CFG_IP4 */
#if FNET_CFG_IP6
#if FNET_CFG_IP4
    ,
#endif /* FNET_CFG_IP4 */
    {   /* IPv6 */
        FNET_HTONS(FNET_ETH_TYPE_IP6),   /* Protocol number */
        _fnet_ip6_input                   /* Protocol input function.*/
    }
#endif /* FNET_CFG_IP6 */
    /* ADD HERE YOUR NETWORK LAYER PROTOCOL */
};

#define FNET_ETH_PROT_IF_LIST_SIZE  (sizeof(fnet_eth_prot_if_list)/sizeof(fnet_eth_prot_if_t))

/************************************************************************
 * Ethernet Multicast Address
 ***********************************************************************/
/* RFC1112 6.4: An IP host group address is mapped to an Ethernet multicast address
 * by placing the low-order 23-bits of the IP address into the low-order
 * 23 bits of the Ethernet multicast address 01-00-5E-00-00-00 (hex).
 */
#define FNET_ETH_MULTICAST_IP4_TO_MAC(ip4_addr, mac_addr)  \
    do{   \
        (mac_addr)[0] = 0x01U; \
        (mac_addr)[1] = 0x00U; \
        (mac_addr)[2] = 0x5EU; \
        (mac_addr)[3] = (fnet_uint8_t)(((fnet_uint8_t *)(&(ip4_addr)))[1] & 0x7FU); \
        (mac_addr)[4] = ((fnet_uint8_t *)(&(ip4_addr)))[2];  \
        (mac_addr)[5] = ((fnet_uint8_t *)(&(ip4_addr)))[3];  \
    }while(0)

/* For IPv6 */
#define FNET_ETH_MULTICAST_IP6_TO_MAC(ip6_addr, mac_addr)        \
    do{   \
        (mac_addr)[0] = 0x33U;               \
        (mac_addr)[1] = 0x33U;               \
        (mac_addr)[2] = (ip6_addr)->addr[12]; \
        (mac_addr)[3] = (ip6_addr)->addr[13]; \
        (mac_addr)[4] = (ip6_addr)->addr[14]; \
        (mac_addr)[5] = (ip6_addr)->addr[15];  \
    }while(0)

#if 0  /* Done by _fnet_netif_is_connected()*/
    #define FNET_ETH_TIMER_PERIOD_MS (500U) /*ms*/
#endif

/******************************************************************************
*     Function Prototypes
*******************************************************************************/
#if 0  /* Done by _fnet_netif_is_connected()*/
    static void _fnet_eth_timer(fnet_uint32_t cookie );
#endif

/************************************************************************
* DESCRIPTION: Eth. input function.
*************************************************************************/
void _fnet_eth_input( fnet_netif_t *netif, fnet_uint8_t *frame, fnet_size_t frame_size)
{
    fnet_index_t        i;
    fnet_eth_header_t   *ethheader = (fnet_eth_header_t *)frame; /* Point to the ethernet header.*/
    fnet_mac_addr_t     local_mac_addr;
    fnet_netbuf_t       *nb = 0;
    fnet_uint16_t       protocol_type;

    if(netif && frame && (frame_size > sizeof(fnet_eth_header_t)))
    {
        if(_fnet_netif_get_hw_addr(netif, local_mac_addr, sizeof(local_mac_addr)) != FNET_OK)
        {
            goto DROP;
        }
        /* Just ignore our own "bounced" frames.*/
        if(fnet_memcmp(ethheader->source_addr, local_mac_addr, sizeof(local_mac_addr)) == 0)
        {
            goto DROP;
        }

        fnet_eth_trace("\nRX", ethheader); /* Print ETH header.*/

        nb = _fnet_netbuf_from_buf( ((fnet_uint8_t *)ethheader + sizeof(fnet_eth_header_t)),
                                    (frame_size - sizeof(fnet_eth_header_t)), FNET_TRUE );
        if(nb)
        {
            if(FNET_MAC_ADDR_IS_BROADCAST(ethheader->destination_addr))    /* Broadcast */
            {
                nb->flags |= FNET_NETBUF_FLAG_BROADCAST;
            }

            if(FNET_MAC_ADDR_IS_MULTICAST(ethheader->destination_addr)) /* Multicast */
            {
                nb->flags |= FNET_NETBUF_FLAG_MULTICAST;
            }

            /* Network-layer input (IPv4/6, ARP).*/
            protocol_type = ethheader->type;

            /* Find Network-layer protocol.*/
            for(i = 0U; i < FNET_ETH_PROT_IF_LIST_SIZE; i++)
            {
                if(protocol_type == fnet_eth_prot_if_list[i].protocol)
                {
                    /* Call the protocol-input function.*/
                    fnet_eth_prot_if_list[i].input(netif, nb);
                    break;
                }
            }

            if(i == FNET_ETH_PROT_IF_LIST_SIZE)
            {
                /* No protocol found */
                _fnet_netbuf_free_chain(nb);
            }
        }
    }
DROP:
    return;
}

/************************************************************************
* DESCRIPTION: Ethernet low-level output function.
*************************************************************************/
void _fnet_eth_output(fnet_netif_t *netif, fnet_uint16_t type, const fnet_mac_addr_t dest_addr, fnet_netbuf_t *nb )
{
    fnet_eth_header_t   *eth_header;
    fnet_netbuf_t       *nb_header;
    fnet_eth_if_t       *eth_if = (fnet_eth_if_t *)(netif->netif_prv);

    /* Check MTU */
    if(nb)
    {
        if(nb->total_length > netif->netif_mtu)
        {
            goto DROP;
        }
    }

    /* Construct Eth header */
    if((nb_header = _fnet_netbuf_new(sizeof(fnet_eth_header_t), FNET_TRUE)) == 0)
    {
        goto DROP;
    }
    eth_header = (fnet_eth_header_t *)nb_header->data_ptr;

    if(_fnet_netif_get_hw_addr(netif, eth_header->source_addr, sizeof(eth_header->source_addr)) != FNET_OK)
    {
        goto DROP;
    }
    fnet_memcpy (eth_header->destination_addr, dest_addr, sizeof(eth_header->destination_addr));
    eth_header->type = fnet_htons(type);

    if(nb)
    {
        nb = _fnet_netbuf_concat(nb_header, nb);
    }
    else
    {
        nb = nb_header;
    }

    /* Ethernet driver output */
    eth_if->eth_output(netif, nb);

    return;
DROP:
    _fnet_netbuf_free_chain(nb);
    return;
}

/************************************************************************
* DESCRIPTION: Do initialization for an Ethernet-type interface.
*************************************************************************/
fnet_return_t _fnet_eth_init( fnet_netif_t *netif)
{
    fnet_return_t result  = FNET_ERR;
    fnet_eth_if_t *eth_if = (fnet_eth_if_t *)(netif->netif_prv);

    if(eth_if)
    {

#if FNET_CFG_IP4
        result = _fnet_arp_init(netif, &eth_if->arp_if); /* Init ARP for this interface.*/
#else
        result = FNET_OK;
#endif /* FNET_CFG_IP4 */

        if(result == FNET_OK)
        {

#if FNET_CFG_IP6
#if FNET_CFG_IP6_PMTU_DISCOVERY
            _fnet_netif_pmtu_init(netif);
#endif

            /* Init Neighbor Discovery.*/
            if( ( result = _fnet_nd6_init (netif, &eth_if->nd6_if) ) == FNET_OK)
            {
                /* RFC4861 6.3.3: The host joins the all-nodes multicast address on all
                 * multicastcapable interfaces.
                 */
                _fnet_ip6_multicast_join(netif, &fnet_ip6_addr_linklocal_allnodes);

                /* To speed the autoconfiguration process, a host may generate its linklocal
                 * address (and verify its uniqueness) in parallel with waiting
                 * for a Router Advertisement. Because a router may delay responding to
                 * a Router Solicitation for a few seconds, the total time needed to
                 * complete autoconfiguration can be significantly longer if the two
                 * steps are done serially.
                 */


                /* Link-Local Address Generation/Auto configuration.
                 * It comprises of '1111111010' as the first ten bits followed by 54 zeroes
                 * and a 64 bit interface identifier.
                 * For all autoconfiguration types, a link-local address is always configured.
                 */
                _fnet_netif_bind_ip6_addr( netif, &fnet_ip6_addr_any, FNET_NETIF_IP_ADDR_TYPE_AUTOCONFIGURABLE,
                                           FNET_NETIF_IP6_ADDR_LIFETIME_INFINITE /*in seconds*/, FNET_ND6_PREFIX_LENGTH_DEFAULT /* bits */ );

                /* RFC4862: The next phase of autoconfiguration involves obtaining a Router
                 * Advertisement or determining that no routers are present.  If routers
                 * are present, they will send Router Advertisements that specify what
                 * sort of autoconfiguration a host can do.
                 * To obtain an advertisement quickly, a host sends one or more Router
                 * Solicitations to the all-routers multicast group.
                 */
                _fnet_nd6_rd_start(netif);
            }
#endif /* FNET_CFG_IP6 */
#if 0  /* Done by _fnet_netif_is_connected()*/
            eth_if->eth_timer = _fnet_timer_new(FNET_ETH_TIMER_PERIOD_MS, _fnet_eth_timer, (fnet_uint32_t)netif);
#endif

            fnet_eth_number++;
        }
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Do relaese for an Ethernet-type interface.
*************************************************************************/
void _fnet_eth_release( fnet_netif_t *netif)
{

#if FNET_CFG_IP6

    _fnet_ip6_multicast_leave_all(netif);

#if FNET_CFG_IP6_PMTU_DISCOVERY
    _fnet_netif_pmtu_release(netif);
#endif

    _fnet_nd6_release(netif);

#endif /* FNET_CFG_IP6 */

#if 0
    _fnet_timer_free(((fnet_eth_if_t *)(netif->netif_prv))->eth_timer);
#endif

#if FNET_CFG_IP4
    _fnet_arp_release(netif);
#endif

    fnet_eth_number--;
}

/************************************************************************
* DESCRIPTION: This function tries to free not critical parts
*              of memory in the Ethernet driver.
*************************************************************************/
void _fnet_eth_drain(fnet_netif_t *netif)
{
#if FNET_CFG_IP4
    _fnet_arp_drain(netif);
#else
    FNET_COMP_UNUSED_ARG(netif);
#endif /* FNET_CFG_IP4 */
}

/************************************************************************
* DESCRIPTION:  This function is called on IP address change.
*               It issues a gratuitous ARP request.
*************************************************************************/
void _fnet_eth_change_addr_notify(fnet_netif_t *netif)
{
#if FNET_CFG_IP4
    if(netif->ip4_addr.address)
    {
        _fnet_arp_send_request( netif, netif->ip4_addr.address ); /* Gratuitous ARP request.*/
        /* When talking about gratuitous ARP, the packets are actually special ARP request packets,
           not ARP reply packets as one would perhaps expect. Some reasons for this are explained in RFC 5227.*/
    }
    else
    {
        fnet_mac_addr_t     sender_addr;
        if(_fnet_netif_get_hw_addr(netif, sender_addr, sizeof(fnet_mac_addr_t)) == FNET_OK)
        {
            /* Inform switches of the MAC address of the machine on a given switch port,
            so that the switch knows that it should transmit packets sent to that MAC address on that switch port.
            This is added mainly to work around first packet losing issue of the AirPort router. */
            _fnet_eth_output(netif, FNET_ETH_TYPE_RESERVED, sender_addr, FNET_NULL); /* Send dummy unicast Ethernet frame to myself*/
        }
    }
#else
    FNET_COMP_UNUSED_ARG(netif);
#endif /* FNET_CFG_IP4 */
}

/************************************************************************
* DESCRIPTION:
*************************************************************************/
#if 0  /* Done by _fnet_netif_is_connected()*/
static void _fnet_eth_timer(fnet_uint32_t cookie )
{
    fnet_netif_t    *netif = (fnet_netif_t *) cookie;

    _fnet_netif_is_connected(netif);
}
#endif

/************************************************************************
* DESCRIPTION: Ethernet IPv4 output function.
*************************************************************************/
#if FNET_CFG_IP4
void _fnet_eth_output_ip4(fnet_netif_t *netif, fnet_ip4_addr_t dest_ip_addr, fnet_netbuf_t *nb)
{
    fnet_mac_addr_t destination_addr; /* 48-bit destination address */

    /* Construct Ethernet header. Start with looking up deciding which
    * MAC address to use as a destination address. Broadcasts and
    * multicasts are special, all other addresses are looked up in the
    * ARP table. */
    if(_fnet_ip4_addr_is_broadcast(dest_ip_addr, netif))
    {
        fnet_memcpy (destination_addr, fnet_eth_broadcast, sizeof(fnet_mac_addr_t));
    }
    else if(FNET_IP4_ADDR_IS_MULTICAST(dest_ip_addr))
    {
        /* Hash IP multicast address to MAC address. */
        destination_addr[0] = 0x01U;
        destination_addr[1] = 0x0U;
        destination_addr[2] = 0x5eU;
        destination_addr[3] = (fnet_uint8_t)(FNET_IP4_ADDR2(dest_ip_addr) & 0x7fU);
        destination_addr[4] = (fnet_uint8_t)(FNET_IP4_ADDR3(dest_ip_addr));
        destination_addr[5] = (fnet_uint8_t)(FNET_IP4_ADDR4(dest_ip_addr));
        /* TBD Use macro. */
    }
    else
        /* Unicast address. */
    {
        if(fnet_arp_get_mac( (fnet_netif_desc_t) netif, dest_ip_addr, destination_addr) == FNET_FALSE)
        {
            _fnet_arp_resolve(netif, dest_ip_addr, nb);
            goto EXIT;
        }
    }

    /* Send Ethernet frame. */
    _fnet_eth_output(netif, FNET_ETH_TYPE_IP4, destination_addr, nb);
EXIT:
    return;
}
#endif /* FNET_CFG_IP4 */

#if FNET_CFG_IP6
/************************************************************************
* DESCRIPTION: Ethernet IPv6 output function.
*************************************************************************/
void _fnet_eth_output_ip6(fnet_netif_t *netif, const fnet_ip6_addr_t *src_ip_addr,  const fnet_ip6_addr_t *dest_ip_addr, fnet_netbuf_t *nb)
{
    fnet_mac_addr_t dest_mac_addr; /* 48-bit destination address */
    fnet_uint8_t *dest_mac_addr_ptr;

    /********************************************
    * Multicast.
    ********************************************/
    if (FNET_IP6_ADDR_IS_MULTICAST(dest_ip_addr))
    {
        FNET_ETH_MULTICAST_IP6_TO_MAC(dest_ip_addr, dest_mac_addr);

        dest_mac_addr_ptr = (fnet_uint8_t *)dest_mac_addr;
    }
    /********************************************
    * Unicast.
    ********************************************/
    else
    {
        fnet_nd6_neighbor_entry_t *neighbor;

        /* Possible redirection.*/
        _fnet_nd6_redirect_addr(netif, &dest_ip_addr);

        /* Check Neigbor cache.*/
        neighbor = _fnet_nd6_neighbor_cache_get(netif, dest_ip_addr);

        /* RFC4861 7.2.2: When a node has a unicast packet to send to a neighbor, but does not
         * know the neighbor�s link-layer address, it performs address resolution.
         * For multicast-capable interfaces, this entails creating a
         * Neighbor Cache entry in the INCOMPLETE state and transmitting a
         * Neighbor Solicitation message targeted at the neighbor. The
         * solicitation is sent to the solicited-node multicast address
         * corresponding to the target address.
         */
        if(neighbor == FNET_NULL)
        {
            /* RFC4861 7.2.Address resolution is performed only on addresses that are determined to be
             * on-link and for which the sender does not know the corresponding link-layer address.
             * Address resolution is never performed on multicast addresses.*/

            if(_fnet_nd6_addr_is_onlink(netif, dest_ip_addr) == FNET_TRUE)
                /* Destimnation is ON local-link.*/
            {
                /* Creating a Neighbor Cache entry in the INCOMPLETE state. */
                neighbor = _fnet_nd6_neighbor_cache_add(netif, dest_ip_addr, FNET_NULL, FNET_ND6_NEIGHBOR_STATE_INCOMPLETE);

                neighbor->state_time_ms = fnet_timer_get_ms();
                neighbor->solicitation_send_counter = 0u;
                FNET_IP6_ADDR_COPY(src_ip_addr, &neighbor->solicitation_src_ip_addr); /* Save src address for later usage.*/

                /* AR: Transmitting a Neighbor Solicitation message targeted at the neighbor.*/
                _fnet_nd6_neighbor_solicitation_send(netif, src_ip_addr, FNET_NULL /* FNET_NULL for AR */, dest_ip_addr);
            }
            /* Destination is OFF local-link.*/
            else
            {
                /* Try to use the router, if exists.*/
                neighbor = _fnet_nd6_default_router_get(netif);

                if(neighbor == FNET_NULL)
                    /* No Router exists.*/
                {
                    _fnet_netbuf_free_chain(nb); /* Discard datagram */
                    goto EXIT;
                }

                dest_ip_addr = &neighbor->ip_addr;   /* Chage destination address to the router one */
            }
        }

        /* Link -layer address is not initialized.*/
        if((neighbor->state != FNET_ND6_NEIGHBOR_STATE_INCOMPLETE)
           && (neighbor->ll_addr[0] == 0U) && (neighbor->ll_addr[1] == 0U) && (neighbor->ll_addr[2] == 0U)
           && (neighbor->ll_addr[3] == 0U) && (neighbor->ll_addr[4] == 0U) && (neighbor->ll_addr[5] == 0U) )
        {
            neighbor->state = FNET_ND6_NEIGHBOR_STATE_INCOMPLETE;
            neighbor->state_time_ms = fnet_timer_get_ms();
            neighbor->solicitation_send_counter = 0u;
            FNET_IP6_ADDR_COPY(src_ip_addr, &neighbor->solicitation_src_ip_addr); /* Save src address for later usage.*/
            /* AR: Transmitting a Neighbor Solicitation message targeted at the neighbor.*/
            _fnet_nd6_neighbor_solicitation_send(netif, src_ip_addr, FNET_NULL /* FNET_NULL for AR */, dest_ip_addr);
        }

        if(neighbor->state == FNET_ND6_NEIGHBOR_STATE_INCOMPLETE)
            /* Queue packet for later transmit.
             */
        {
            _fnet_nd6_neighbor_enqueue_waiting_netbuf(neighbor, nb);
            goto EXIT;
        }

        if(neighbor->state == FNET_ND6_NEIGHBOR_STATE_STALE)
            /* RFC4861 7.3.3: The first time a node sends a packet to a neighbor whose entry is
             * STALE, the sender changes the state to DELAY and sets a timer to
             * expire in DELAY_FIRST_PROBE_TIME seconds.
             */
        {
            neighbor->state = FNET_ND6_NEIGHBOR_STATE_DELAY;
            neighbor->state_time_ms = fnet_timer_get_ms();
        }

        /* Get destination MAC/HW address.*/
        dest_mac_addr_ptr = (fnet_uint8_t *)(&neighbor->ll_addr[0]);
    }

    /* Send Ethernet frame. */
    _fnet_eth_output(netif, FNET_ETH_TYPE_IP6, dest_mac_addr_ptr, nb);

EXIT:
    return;
}
#endif /* FNET_CFG_IP6 */

#if FNET_CFG_MULTICAST

#if FNET_CFG_IP4
/************************************************************************
* DESCRIPTION: Leavess a multicast group on an Ethernet interface.
*************************************************************************/
void _fnet_eth_multicast_leave_ip4(fnet_netif_t *netif, fnet_ip4_addr_t multicast_addr )
{
    FNET_ASSERT(netif != FNET_NULL);
    FNET_ASSERT(netif->netif_prv != FNET_NULL);

    fnet_mac_addr_t mac_addr;

    if(((fnet_eth_if_t *)(netif->netif_prv))->eth_multicast_leave)
    {
        FNET_ETH_MULTICAST_IP4_TO_MAC(multicast_addr, mac_addr);

        ((fnet_eth_if_t *)(netif->netif_prv))->eth_multicast_leave(netif, mac_addr);
    }
}

/************************************************************************
* DESCRIPTION: Joins a multicast group on an Ethernet interface.
*************************************************************************/
void _fnet_eth_multicast_join_ip4(fnet_netif_t *netif, fnet_ip4_addr_t  multicast_addr )
{
    FNET_ASSERT(netif != FNET_NULL);
    FNET_ASSERT(netif->netif_prv != FNET_NULL);

    fnet_mac_addr_t mac_addr;

    if(((fnet_eth_if_t *)(netif->netif_prv))->eth_multicast_join)
    {
        FNET_ETH_MULTICAST_IP4_TO_MAC(multicast_addr, mac_addr);

        ((fnet_eth_if_t *)(netif->netif_prv))->eth_multicast_join(netif, mac_addr);
    }
}
#endif /* FNET_CFG_IP4 */

#if FNET_CFG_IP6
/************************************************************************
* DESCRIPTION: Leavess a multicast group on an Ethernet interface.
*************************************************************************/
void _fnet_eth_multicast_leave_ip6(fnet_netif_t *netif, fnet_ip6_addr_t *multicast_addr )
{
    FNET_ASSERT(netif != FNET_NULL);
    FNET_ASSERT(netif->netif_prv != FNET_NULL);

    fnet_mac_addr_t mac_addr;

    if(((fnet_eth_if_t *)(netif->netif_prv))->eth_multicast_leave)
    {
        FNET_ETH_MULTICAST_IP6_TO_MAC(multicast_addr, mac_addr);

        ((fnet_eth_if_t *)(netif->netif_prv))->eth_multicast_leave(netif, mac_addr);
    }
}

/************************************************************************
* DESCRIPTION: Joins a multicast group on an Ethernet interface.
*************************************************************************/
void _fnet_eth_multicast_join_ip6(fnet_netif_t *netif, const fnet_ip6_addr_t  *multicast_addr )
{
    FNET_ASSERT(netif != FNET_NULL);
    FNET_ASSERT(netif->netif_prv != FNET_NULL);

    fnet_mac_addr_t mac_addr;

    if(((fnet_eth_if_t *)(netif->netif_prv))->eth_multicast_join)
    {
        FNET_ETH_MULTICAST_IP6_TO_MAC(multicast_addr, mac_addr);

        ((fnet_eth_if_t *)(netif->netif_prv))->eth_multicast_join(netif, mac_addr);
    }
}
#endif /* FNET_CFG_IP6 */

#endif /* FNET_CFG_MULTICAST */

/************************************************************************
* DESCRIPTION: Read a value from a Ethernet PHY's MII register.
*************************************************************************/
fnet_return_t  fnet_eth_phy_read(fnet_netif_desc_t netif_desc, fnet_uint32_t reg_addr, fnet_uint16_t *reg_data)
{
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;
    fnet_return_t   result;

    _fnet_stack_mutex_lock();
    result = _fnet_eth_phy_read(netif, reg_addr, reg_data);
    _fnet_stack_mutex_unlock();

    return result;
}
fnet_return_t  _fnet_eth_phy_read(fnet_netif_t *netif, fnet_uint32_t reg_addr, fnet_uint16_t *reg_data)
{
    fnet_return_t   result = FNET_ERR;

    if(netif && (netif->netif_api->netif_type == FNET_NETIF_TYPE_ETHERNET))
    {
        if(netif->netif_api->eth_api && netif->netif_api->eth_api->phy_read)
        {
            result = netif->netif_api->eth_api->phy_read(netif, reg_addr, reg_data);
        }
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Write a value to a PHY's MII register.
*************************************************************************/
fnet_return_t  fnet_eth_phy_write(fnet_netif_desc_t netif_desc, fnet_uint32_t reg_addr, fnet_uint16_t reg_data)
{
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;
    fnet_return_t   result;

    _fnet_stack_mutex_lock();
    result = _fnet_eth_phy_write(netif, reg_addr, reg_data);
    _fnet_stack_mutex_unlock();

    return result;
}
fnet_return_t  _fnet_eth_phy_write(fnet_netif_t *netif, fnet_uint32_t reg_addr, fnet_uint16_t reg_data)
{
    fnet_return_t   result = FNET_ERR;

    if(netif && (netif->netif_api->netif_type == FNET_NETIF_TYPE_ETHERNET))
    {
        if(netif->netif_api->eth_api && netif->netif_api->eth_api->phy_write)
        {
            result = netif->netif_api->eth_api->phy_write(netif, reg_addr, reg_data);
        }
    }
    return result;
}

/************************************************************************
* DESCRIPTION: Get Ethernet PHY address.
*************************************************************************/
fnet_uint8_t fnet_eth_phy_get_addr(fnet_netif_desc_t netif_desc)
{
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;
    fnet_uint8_t    result = 0;

    if(netif && (netif->netif_api->netif_type == FNET_NETIF_TYPE_ETHERNET))
    {
        _fnet_stack_mutex_lock();
        result = _fnet_eth_phy_get_addr(netif);
        _fnet_stack_mutex_unlock();
    }

    return result;
}
fnet_uint8_t _fnet_eth_phy_get_addr(fnet_netif_t *netif)
{
    fnet_uint8_t    result;

    result = ((fnet_eth_if_t *)(netif->netif_prv))->eth_phy_addr;

    return result;
}

/************************************************************************
* DESCRIPTION: Change Ethernet PHY address.
*************************************************************************/
fnet_return_t  fnet_eth_phy_set_addr(fnet_netif_desc_t netif_desc, fnet_uint8_t phy_addr )
{
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;
    fnet_return_t   result = FNET_ERR;

    if(netif && (netif->netif_api->netif_type == FNET_NETIF_TYPE_ETHERNET))
    {
        _fnet_stack_mutex_lock();
        _fnet_eth_phy_set_addr(netif, phy_addr);
        _fnet_stack_mutex_unlock();
        result = FNET_OK;
    }

    return result;
}
void _fnet_eth_phy_set_addr(fnet_netif_t *netif, fnet_uint8_t phy_addr )
{
    ((fnet_eth_if_t *)(netif->netif_prv))->eth_phy_addr = phy_addr;
}

/************************************************************************
* DESCRIPTION: Link status.
*************************************************************************/
fnet_bool_t _fnet_eth_is_connected(fnet_netif_t *netif)
{
    fnet_uint16_t   data;
    fnet_bool_t     res = FNET_FALSE;

#if 0 /* 0 = We do not want to skip disconnection. */
    /* Some PHY (e.g.DP8340) returns "unconnected" and than "connected" state
     *  just to show that was transition event from one state to another.
     *  As we need only curent state,  read 2 times and returtn
     *  the current/latest state. */
    _fnet_eth_phy_read(netif, FNET_ETH_MII_REG_SR, &data);
#endif

    if (_fnet_eth_phy_read(netif, FNET_ETH_MII_REG_SR, &data) == FNET_OK)
    {
        res = (((data & FNET_ETH_MII_REG_SR_LINK_STATUS) != 0u) ? FNET_TRUE : FNET_FALSE);
    }
    else /* Return previous value in case read PHY error. */
    {
        res = netif->is_connected;
    }

    return res;
}

/************************************************************************
* DESCRIPTION: Auto-discober Ethernet PHY address.
*************************************************************************/
void _fnet_eth_phy_discover_addr (fnet_netif_t *netif, fnet_uint8_t phy_addr_start)
{
    fnet_uint8_t i;
    fnet_uint8_t phy_addr = phy_addr_start;     /* Save old value just in case the discover
                                               * is failed, in case communication with
                                               * the PHY via MDIO is not possible.*/

    for (i = (fnet_uint8_t)phy_addr_start; i < 32U; i++)
    {
        fnet_uint16_t id = 0;

        _fnet_eth_phy_set_addr(netif, i);

        _fnet_eth_phy_read(netif, FNET_ETH_MII_REG_IDR1, &id);

        if (!((id == 0U) || (id == 0xffffU) || (id == 0x7fffU)))
        {
            return; /* FHY address is discovered.*/
        }
    }

    _fnet_eth_phy_set_addr(netif, phy_addr );
}

/************************************************************************
* DESCRIPTION: PHY initialization/reset.
*************************************************************************/
fnet_return_t _fnet_eth_phy_init(fnet_netif_t *netif)
{
    fnet_uint16_t       reg_value = 0;
    fnet_uint32_t       counter;
    fnet_return_t       result;
    fnet_eth_if_t       *eth_if = (fnet_eth_if_t *)(netif->netif_prv);

    /* Looking for a valid PHY address. */
#if FNET_CFG_CPU_ETH_PHY_ADDR_DISCOVER
    _fnet_eth_phy_discover_addr (netif, _fnet_eth_phy_get_addr(netif));
#endif


    /* == Reset PHY, wait for completion. == */
    _fnet_eth_phy_write(netif, FNET_ETH_MII_REG_CR, FNET_ETH_MII_REG_CR_RESET);
    for (counter = 10000; counter > 0; counter--)
    {
        _fnet_eth_phy_read(netif, FNET_ETH_MII_REG_CR, &reg_value);
        /* When the reset operation is done, CR_RESET bit is cleared to 0 automatically.*/
        if((reg_value & FNET_ETH_MII_REG_CR_RESET) == 0)
        {
            break; /* Completed */
        }
    }

    /* Platform-specific Ethernet PHY initialization */
    if(eth_if->eth_cpu_phy_init)
    {
        result = eth_if->eth_cpu_phy_init(netif);
    }
    else
    {
        result = FNET_OK;
    }

    if(result == FNET_OK)
    {
        /*== Start auto-negotiation  == */
        _fnet_eth_phy_read(netif, FNET_ETH_MII_REG_CR, &reg_value);
        _fnet_eth_phy_write(netif, FNET_ETH_MII_REG_CR, ((reg_value | FNET_ETH_MII_REG_CR_ANE | FNET_ETH_MII_REG_CR_ANE_RESTART) & (~FNET_ETH_MII_REG_CR_ISOL)));

        if(counter) /* No timeout */
        {
#if FNET_CFG_CPU_ETH_ATONEGOTIATION_TIMEOUT
            /* Wait for auto-negotiation completion. */
            {
                fnet_time_t     last_time =  fnet_timer_get_ms();

                do
                {
                    /* Read Basic Mode Status Register*/
                    if(_fnet_eth_phy_read(netif, FNET_ETH_MII_REG_SR, &reg_value) == FNET_ERR)
                    {
                        break;
                    }

                    /* Auto-Negotiation Complete */
                    if(reg_value & FNET_ETH_MII_REG_SR_AN_COMPLETE)
                    {
                        break;
                    }
                }
                while((fnet_timer_get_ms() - last_time) < FNET_CFG_CPU_ETH_ATONEGOTIATION_TIMEOUT);
            }
#endif /* FNET_CFG_CPU_ETH_ATONEGOTIATION_TIMEOUT */
        }
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Prints an Ethernet header. For debug needs only.
*************************************************************************/
#if FNET_CFG_DEBUG_TRACE_ETH && FNET_CFG_DEBUG_TRACE
void fnet_eth_trace(fnet_char_t *str, fnet_eth_header_t *eth_hdr)
{
    fnet_char_t mac_str[FNET_MAC_ADDR_STR_SIZE];

    fnet_printf(FNET_SERIAL_ESC_FG_GREEN"%s", str); /* Print app-specific header.*/
    fnet_println("[ETH header]"FNET_SERIAL_ESC_ATTR_RESET);
    fnet_println("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+/\\/\\/\\/-+");
    fnet_println("|(Dest)                                                "FNET_SERIAL_ESC_FG_BLUE"%17s"FNET_SERIAL_ESC_ATTR_RESET" |", fnet_mac_to_str(eth_hdr->destination_addr, mac_str));
    fnet_println("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+/\\/\\/\\/-+");
    fnet_println("|(Src)                                                 "FNET_SERIAL_ESC_FG_BLUE"%17s"FNET_SERIAL_ESC_ATTR_RESET" |", fnet_mac_to_str(eth_hdr->source_addr, mac_str));
    fnet_println("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+/\\/\\/\\/-+");
    fnet_println("|(Type)                  0x%04x |", fnet_ntohs(eth_hdr->type));
    fnet_println("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+");
}

#endif /* FNET_CFG_DEBUG_TRACE_ETH */

