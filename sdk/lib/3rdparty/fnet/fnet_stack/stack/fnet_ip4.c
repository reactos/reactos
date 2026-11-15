/**************************************************************************
*
* Copyright 2008-2018 by Andrey Butok. FNET Community
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
*  IP protocol implementation.
*
***************************************************************************/

#include "fnet.h"
#include "fnet_ip4_prv.h"
#include "fnet_icmp4.h"
#include "fnet_checksum_prv.h"
#include "fnet_timer_prv.h"
#include "fnet_loop.h"
#include "fnet_raw.h"
#include "fnet_stack_prv.h"
#include "fnet_ip_prv.h"
#include "fnet_igmp.h"

/* Check max/min. values.*/
#if (FNET_IP4_MAX_PACKET > 65535U)
    #error  "FNET_IP4_MAX_PACKET may not be more than 65535."
#endif

#if (FNET_IP4_MAX_PACKET < 200U)
    #error  "FNET_IP4_MAX_PACKET must be more than 200."
#endif

#if FNET_CFG_IP4

#if FNET_CFG_IP4_FRAGMENTATION
    static fnet_ip4_frag_list_t *ip_frag_list_head;
    static fnet_timer_desc_t ip_timer_ptr;
#endif

static fnet_ip_queue_t   ip_queue;
static fnet_event_desc_t ip_event;

#if FNET_CFG_MULTICAST
    fnet_ip4_multicast_list_entry_t fnet_ip4_multicast_list[FNET_CFG_MULTICAST_MAX];
#endif /* FNET_CFG_MULTICAST */

/************************************************************************
*     Function Prototypes
*************************************************************************/
static void _fnet_ip4_netif_output(struct fnet_netif *netif, fnet_ip4_addr_t dest_ip_addr, fnet_netbuf_t *nb, fnet_bool_t do_not_route);
static void _fnet_ip4_input_low(void *cookie);
static fnet_bool_t _fnet_ip4_addr_is_onlink(fnet_netif_t *netif, fnet_ip4_addr_t addr);

#if FNET_CFG_IP4_FRAGMENTATION
    static void _fnet_ip4_reassembly( fnet_netbuf_t **nb_ptr );
    static void _fnet_ip4_frag_list_add( fnet_ip4_frag_list_t **head, fnet_ip4_frag_list_t *fl );
    static void _fnet_ip4_frag_list_del( fnet_ip4_frag_list_t **head, fnet_ip4_frag_list_t *fl );
    static void _fnet_ip4_frag_add( fnet_ip4_frag_header_t *FNET_COMP_PACKED_VAR *head, fnet_ip4_frag_header_t *frag, fnet_ip4_frag_header_t *frag_prev );
    static void _fnet_ip4_frag_del( fnet_ip4_frag_header_t *FNET_COMP_PACKED_VAR *head, fnet_ip4_frag_header_t *frag );
    static void _fnet_ip4_frag_list_free( fnet_ip4_frag_list_t *list );
    static void _fnet_ip4_timer(fnet_uint32_t cookie );
#endif

#if FNET_CFG_DEBUG_TRACE_IP4 && FNET_CFG_DEBUG_TRACE
    static void _fnet_ip4_trace(const fnet_char_t *str, fnet_ip4_header_t *ip_hdr);
#else
    #define _fnet_ip4_trace(str, ip_hdr)  do{}while(0)
#endif

/************************************************************************
* DESCRIPTION: This function makes initialization of the IP layer.
*************************************************************************/
fnet_return_t _fnet_ip4_init( void )
{
    fnet_return_t result = FNET_ERR;

#if FNET_CFG_IP4_FRAGMENTATION

    ip_frag_list_head = 0;
    ip_timer_ptr = _fnet_timer_new(FNET_IP4_TIMER_PERIOD, _fnet_ip4_timer, 0u);

    if(ip_timer_ptr)
    {
#endif

#if FNET_CFG_MULTICAST
        /* Clear the multicast list.*/
        fnet_memset_zero(fnet_ip4_multicast_list, sizeof(fnet_ip4_multicast_list));
#endif /* FNET_CFG_MULTICAST */

        /* Install SW Interrupt handler. */
        ip_event = fnet_event_init(_fnet_ip4_input_low, 0u);
        if(ip_event)
        {
            result = FNET_OK;
        }

#if FNET_CFG_IP4_FRAGMENTATION
    }
#endif

    return result;
}

/************************************************************************
* DESCRIPTION: This function makes release of the all resources
*              allocated for IP layer module.
*************************************************************************/
void _fnet_ip4_release( void )
{
    _fnet_ip4_drain();
#if FNET_CFG_IP4_FRAGMENTATION

    _fnet_timer_free(ip_timer_ptr);
    ip_timer_ptr = 0;

#endif
}

/************************************************************************
* DESCRIPTION: This function performs IP routing
*              on an outgoing IP packet.
*************************************************************************/
fnet_netif_t *_fnet_ip4_route( fnet_ip4_addr_t dest_ip )
{
    fnet_netif_t    *netif;
    fnet_netif_t    *res_netif = FNET_NULL;
    fnet_netif_t    *netif_default = _fnet_netif_get_default();

    /* Local network */
    for (netif = fnet_netif_list; netif != 0; netif = netif->next)
    {
        if(netif->ip4_addr.subnetmask && ((dest_ip & netif->ip4_addr.subnetmask) == (netif->ip4_addr.address & netif->ip4_addr.subnetmask)))
        {
            res_netif = netif;
            break;
        }
    }

    /* Public network. Get interface wit a gateway. */
    if(res_netif == FNET_NULL)
    {
        /* Default interface has priority. */
        if(netif_default && netif_default->ip4_addr.gateway)
        {
            res_netif = netif_default;
        }
        else
        {
            for (netif = fnet_netif_list; netif != 0; netif = netif->next)
            {
                /* If there is a gateway */
                if(netif->ip4_addr.gateway)
                {
                    res_netif = netif;
                    break;
                }
            }
        }
    }

    /* If there is no proper interface - choose the default one */
    if(res_netif == FNET_NULL)
    {
        res_netif = netif_default;
    }

#if FNET_CFG_LOOPBACK
    /* Anything sent to one of the host's own IP address is sent to the loopback interface.*/
    if(res_netif && (dest_ip == res_netif->ip4_addr.address))
    {
        res_netif = FNET_LOOP_IF;
    }
#endif /* FNET_CFG_LOOPBACK */

    return res_netif;
}

/************************************************************************
* DESCRIPTION: This function returns FNET_TRUE if the protocol message
*              will be fragmented by IPv4, and FNET_FALSE otherwise.
*************************************************************************/
fnet_bool_t _fnet_ip4_will_fragment( fnet_netif_t *netif, fnet_size_t protocol_message_size)
{
    fnet_bool_t res;

    if((protocol_message_size + sizeof(fnet_ip4_header_t)) > netif->netif_mtu)
    {
        res = FNET_TRUE;
    }
    else
    {
        res = FNET_FALSE;
    }

    return res;
}

/************************************************************************
* DESCRIPTION: IP output function.
*
* RETURNS: FNET_OK=OK
*          FNET_ERR_NETUNREACH=No route
*          FNET_ERR_MSGSIZE=Size error
*          FNET_ERR_NOMEM=No memory
*************************************************************************/
fnet_error_t _fnet_ip4_output( fnet_netif_t *netif,    fnet_ip4_addr_t src_ip, fnet_ip4_addr_t dest_ip,
                               fnet_uint8_t protocol, fnet_uint8_t tos,     fnet_uint8_t ttl,
                               fnet_netbuf_t *nb, fnet_bool_t DF, fnet_bool_t do_not_route,
                               FNET_COMP_PACKED_VAR fnet_uint16_t *checksum )
{
    static fnet_uint16_t    ip_id = 0u;
    fnet_netbuf_t           *nb_header;
    fnet_ip4_header_t       *ipheader;
    fnet_size_t             total_length;
    fnet_error_t            error_code;

    if(netif == 0)
    {
        if((netif = _fnet_ip4_route(dest_ip)) == 0) /* No route */
        {
            error_code = FNET_ERR_NETUNREACH;
            goto DROP;
        }
    }

    /* If source address not specified, use address of outgoing interface */
    if(src_ip == INADDR_ANY)
    {
        src_ip = netif->ip4_addr.address;
    }

    if((nb->total_length + sizeof(fnet_ip4_header_t)) > FNET_IP4_MAX_PACKET)
    {
        error_code = FNET_ERR_MSGSIZE;
        goto DROP;
    }

    /* Construct IP header */
    if((nb_header = _fnet_netbuf_new(sizeof(fnet_ip4_header_t), FNET_TRUE)) == 0)
    {
        error_code = FNET_ERR_NOMEM;
        goto DROP;
    }

    /* Pseudo checksum. */
    if(checksum)
    {
        *checksum = _fnet_checksum_pseudo_netbuf_end( *checksum, (fnet_uint8_t *)&src_ip, (fnet_uint8_t *)&dest_ip, sizeof(fnet_ip4_addr_t) );
    }

    ipheader = (fnet_ip4_header_t *)nb_header->data_ptr;

    FNET_IP_HEADER_SET_VERSION(ipheader, (fnet_uint8_t)FNET_IP4_VERSION); /* version =4 */
    ipheader->id = fnet_htons(ip_id++);              /* Id */

    ipheader->tos = tos;                 /* Type of service */
    total_length = (fnet_uint16_t)(nb->total_length + sizeof(fnet_ip4_header_t)); /* total length*/
    FNET_IP_HEADER_SET_HEADER_LENGTH(ipheader, sizeof(fnet_ip4_header_t) >> 2);
    ipheader->flags_fragment_offset = 0x0000u; /* flags & fragment offset field */

    if(DF)
    {
        ipheader->flags_fragment_offset |= FNET_HTONS(FNET_IP4_DF);
    }

    ipheader->ttl = ttl;                 /* time to live */
    ipheader->protocol = protocol;       /* protocol */
    ipheader->source_addr = src_ip;      /* source address */
    ipheader->desination_addr = dest_ip; /* destination address */

    ipheader->total_length = fnet_htons((fnet_uint16_t)total_length);

    nb = _fnet_netbuf_concat(nb_header, nb);

    if(total_length > netif->netif_mtu) /* IP Fragmentation. */
    {
#if FNET_CFG_IP4_FRAGMENTATION

        fnet_size_t         first_frag_length;
        fnet_size_t         frag_length;            /* The number of data in each fragment. */
        fnet_size_t         offset;
        fnet_index_t        error = 0u;
        fnet_netbuf_t       *nb_prev;
        fnet_netbuf_t       **nb_next_ptr = &nb->next_chain;
        fnet_size_t         new_header_length;
        fnet_size_t         header_length = (fnet_size_t)(FNET_IP_HEADER_GET_HEADER_LENGTH(ipheader) << 2);
        fnet_ip4_header_t   *new_ipheader;

        frag_length = (netif->netif_mtu - header_length) & ~7u; /* rounded down to an 8-byte boundary.*/
        first_frag_length = frag_length;

        if(((ipheader->flags_fragment_offset & FNET_HTONS(FNET_IP4_DF)) != 0u) ||   /* The fragmentation is prohibited. */
           (frag_length < 8u))                                    /* The MTU is too small.*/
        {
            error_code = FNET_ERR_MSGSIZE;
            goto DROP;
        }

        /* The header (and options) must reside in contiguous area of memory.*/
        if(_fnet_netbuf_pullup(&nb, header_length) == FNET_ERR)
        {
            error_code = FNET_ERR_NOMEM;
            goto DROP;
        }

        ipheader = (fnet_ip4_header_t *)nb->data_ptr;

        nb_prev = nb;

        /* Go through the whole data segment after first fragment.*/
        for (offset = (header_length + frag_length); offset < total_length; offset += frag_length)
        {
            fnet_netbuf_t *nb_tmp;

            nb = _fnet_netbuf_new(header_length, FNET_FALSE); /* Allocate a new header.*/

            if(nb == 0)
            {
                error++;
                goto FRAG_END;
            }

            fnet_memcpy(nb->data_ptr, ipheader, header_length); /* Copy IP header.*/
            new_ipheader = (fnet_ip4_header_t *)nb->data_ptr;
            new_header_length = sizeof(fnet_ip4_header_t);

            FNET_IP_HEADER_SET_HEADER_LENGTH(new_ipheader, (fnet_uint8_t)(new_header_length >> 2));
            new_ipheader->flags_fragment_offset = fnet_htons((fnet_uint16_t)((offset - header_length) >> 3));

            if(offset + frag_length >= total_length)
            {
                frag_length = (total_length - offset);
            }
            else
            {
                new_ipheader->flags_fragment_offset |= FNET_HTONS(FNET_IP4_MF);
            }

            /* Copy the data from the original packet into the fragment.*/
            if((nb_tmp = _fnet_netbuf_copy(nb_prev, offset, frag_length, FNET_FALSE)) == 0)
            {
                error++;
                _fnet_netbuf_free_chain(nb);
                goto FRAG_END;
            }

            nb = _fnet_netbuf_concat(nb, nb_tmp);

            new_ipheader->total_length = fnet_htons((fnet_uint16_t)nb->total_length);

            *nb_next_ptr = nb;
            nb_next_ptr = &nb->next_chain;
        }

        /* Update the first fragment.*/
        nb = nb_prev;
        _fnet_netbuf_trim(&nb, (fnet_int32_t)(header_length + first_frag_length - fnet_ntohs(ipheader->total_length)));
        ipheader->total_length = fnet_htons((fnet_uint16_t)nb->total_length);
        ipheader->flags_fragment_offset |= FNET_HTONS(FNET_IP4_MF);

    FRAG_END:
        for (nb = nb_prev; nb; nb = nb_prev)    /* Send each fragment.*/
        {
            nb_prev = nb->next_chain;
            nb->next_chain = 0;

            if(error == 0u)
            {
                _fnet_ip4_trace("TX", nb->data_ptr); /* Print IP header. */
                _fnet_ip4_netif_output(netif, dest_ip, nb, do_not_route);
            }
            else
            {
                _fnet_netbuf_free_chain(nb);
            }
        }

#else

        error_code = FNET_ERR_MSGSIZE;   /* Discard datagram.*/
        goto DROP;

#endif  /* FNET_CFG_IP4_FRAGMENTATION */

    }
    else
    {
        _fnet_ip4_netif_output(netif, dest_ip, nb, do_not_route);
    }

    return (FNET_ERR_OK);

DROP:
    _fnet_netbuf_free_chain(nb);           /* Discard datagram */

    return (error_code);
}

/************************************************************************
* DESCRIPTION:
*************************************************************************/
static void _fnet_ip4_netif_output(struct fnet_netif *netif, fnet_ip4_addr_t dest_ip_addr, fnet_netbuf_t *nb, fnet_bool_t do_not_route)
{
    fnet_ip4_header_t   *ipheader = (fnet_ip4_header_t *)nb->data_ptr;

    /* IPv4 Header Checksum*/
    ipheader->checksum = 0u;

#if FNET_CFG_CPU_ETH_HW_TX_IP_CHECKSUM
    if(netif->features & FNET_NETIF_FEATURE_HW_TX_IP_CHECKSUM)
        nb->flags |= FNET_NETBUF_FLAG_HW_IP_CHECKSUM;
    else
#endif
        ipheader->checksum = _fnet_checksum_netbuf(nb, (fnet_size_t)FNET_IP_HEADER_GET_HEADER_LENGTH(ipheader) << 2); /* IP checksum*/

    if( /* Datagrams sent to a broadcast address */
        (_fnet_ip4_addr_is_broadcast(dest_ip_addr, netif))
        /* Datagrams sent to a multicast address. */
        || FNET_IP4_ADDR_IS_MULTICAST(dest_ip_addr) )
    {
#if  FNET_CFG_LOOPBACK && (FNET_CFG_LOOPBACK_MULTICAST || FNET_CFG_LOOPBACK_BROADCAST)
        fnet_netbuf_t *nb_loop;

        if(netif != FNET_LOOP_IF) /* Avoid double send to the loopback interface.*/
        {
            /* Datagrams sent to a broadcast/multicast address are copied to the loopback interface.*/
            if((nb_loop = _fnet_netbuf_copy(nb, 0, FNET_NETBUF_COPYALL, FNET_TRUE)) != 0)
            {
                _fnet_loop_output_ip4(netif, dest_ip_addr, nb_loop);
            }
        }
#endif /* FNET_CFG_LOOPBACK && (FNET_CFG_LOOPBACK_MULTICAST || FNET_CFG_LOOPBACK_BROADCAST) */
    }
    else
    {
        if(!((do_not_route) || (_fnet_ip4_addr_is_onlink(netif, dest_ip_addr) == FNET_TRUE)))
        {
            /* Use the default router as the address to send.*/
            dest_ip_addr = netif->ip4_addr.gateway;
        }
    }

    /* Send to Interface.*/
    netif->netif_api->netif_output_ip4(netif, dest_ip_addr, nb);
}

/************************************************************************
* DESCRIPTION: Checks if the address is on-link.
*              Returns FNET_TRUE if it is on-link, FNET_FALSE otherwise.
*************************************************************************/
static fnet_bool_t _fnet_ip4_addr_is_onlink(fnet_netif_t *netif, fnet_ip4_addr_t addr)
{
    fnet_bool_t on_link;
    if(((addr & netif->ip4_addr.subnetmask) == (netif->ip4_addr.address & netif->ip4_addr.subnetmask))
       /* RFC3927: If the destination address is in the 169.254/16 prefix, then the sender
           MUST send its packet directly to the destination on the same physical link.  This MUST be
           done whether the interface is configured with a Link-Local or a routable IPv4 address.    */
       || ((addr & FNET_IP4_ADDR_LINK_LOCAL_PREFIX) == (FNET_IP4_ADDR_LINK_LOCAL_PREFIX)))

    {
        on_link = FNET_TRUE;
    }
    else
    {
        on_link = FNET_FALSE;
    }
    return on_link;
}

/************************************************************************
* DESCRIPTION: Prepare sockets addreses for upper protocol.
*************************************************************************/
void _fnet_ip4_set_socket_addr(fnet_netif_t *netif, fnet_ip4_header_t *ip_hdr, struct fnet_sockaddr *src_addr,  struct fnet_sockaddr *dest_addr )
{
    fnet_memset_zero(src_addr, sizeof(struct fnet_sockaddr));
    src_addr->sa_family = AF_INET;
    src_addr->sa_scope_id = netif->scope_id;
    ((struct fnet_sockaddr_in *)(src_addr))->sin_addr.s_addr = ip_hdr->source_addr;

    fnet_memset_zero(dest_addr, sizeof(struct fnet_sockaddr));
    dest_addr->sa_family = AF_INET;
    dest_addr->sa_scope_id = netif->scope_id;
    ((struct fnet_sockaddr_in *)(dest_addr))->sin_addr.s_addr = ip_hdr->desination_addr;
}

/************************************************************************
* DESCRIPTION: IP input function.
*************************************************************************/
void _fnet_ip4_input( fnet_netif_t *netif, fnet_netbuf_t *nb )
{
    if(netif && nb)
    {

        if(_fnet_ip_queue_append(&ip_queue, netif, nb) != FNET_OK)
        {
            _fnet_netbuf_free_chain(nb);
            return;
        }

        /* Initiate S/W Interrupt*/
        fnet_event_raise(ip_event);
    }
}

/************************************************************************
* DESCRIPTION: This function performs handling of incoming datagrams.
*************************************************************************/
static void _fnet_ip4_input_low(void *cookie)
{
    fnet_ip4_header_t       *hdr;
    fnet_netbuf_t           *ip4_nb;
    fnet_prot_if_t          *protocol;
    fnet_netif_t            *netif;
    fnet_netbuf_t           *nb;
    fnet_ip4_addr_t         destination_addr;
    fnet_size_t             total_length;
    fnet_size_t             header_length;
    struct fnet_sockaddr    src_addr;
    struct fnet_sockaddr    dest_addr;

    FNET_COMP_UNUSED_ARG(cookie);

    fnet_isr_lock();

    while((nb = _fnet_ip_queue_read(&ip_queue, &netif)) != 0)
    {
        nb->next_chain = 0;

        /* The header must reside in contiguous area of memory. */
        if(_fnet_netbuf_pullup(&nb, sizeof(fnet_ip4_header_t)) == FNET_ERR)
        {
            _fnet_netbuf_free_chain(nb);
            continue;
        }

        hdr = (fnet_ip4_header_t *)nb->data_ptr;
        destination_addr = hdr->desination_addr;
        total_length = fnet_ntohs(hdr->total_length);
        header_length = (fnet_size_t)FNET_IP_HEADER_GET_HEADER_LENGTH(hdr) << 2;

        _fnet_ip4_trace("RX", hdr); /* Print IP header. */

        if((nb->total_length >= total_length)                            /* Check the amount of data*/
           && (nb->total_length >= sizeof(fnet_ip4_header_t))
           && (FNET_IP_HEADER_GET_VERSION(hdr) == 4u)                   /* Check the IP Version*/
           && (header_length >= sizeof(fnet_ip4_header_t))               /* Check the IP header length*/
           && (total_length >= header_length)
#if FNET_CFG_CPU_ETH_HW_TX_IP_CHECKSUM || FNET_CFG_CPU_ETH_HW_RX_IP_CHECKSUM
           && ((netif->features & FNET_NETIF_FEATURE_HW_RX_IP_CHECKSUM)
               || (nb->flags | FNET_NETBUF_FLAG_HW_IP_CHECKSUM)
               || (_fnet_checksum_netbuf(nb, header_length) == 0u) )       /* Checksum*/
#else
           && (_fnet_checksum_netbuf(nb, header_length) == 0u)             /* Checksum*/
#endif
           && ((destination_addr == netif->ip4_addr.address)           /* It is final destination*/
               || (_fnet_ip4_addr_is_broadcast(destination_addr, netif))
               || ((netif->ip4_addr.address == INADDR_ANY) && ((nb->flags & FNET_NETBUF_FLAG_BROADCAST) == FNET_NETBUF_FLAG_BROADCAST)) /* Check frame broadcast flag, in case no address signed yet (e.g. DHCP).*/
#if FNET_CFG_MULTICAST
               || (FNET_IP4_ADDR_IS_MULTICAST(destination_addr))
#endif
               || (netif->netif_api->netif_type == FNET_NETIF_TYPE_LOOPBACK)
              )
          )
        {
            if(nb->total_length > total_length)
            {
                /* Logical size and the physical size of the packet should be the same.*/
                _fnet_netbuf_trim(&nb, (fnet_int32_t)(total_length - nb->total_length));
            }

            /* Reassembly.*/
            if((hdr->flags_fragment_offset & ~FNET_HTONS(FNET_IP4_DF)) != 0u) /* the MF bit or fragment offset is nonzero.*/
            {
#if FNET_CFG_IP4_FRAGMENTATION
                _fnet_ip4_reassembly(&nb);
                if(nb == FNET_NULL)
                {
                    continue;
                }

                hdr = (fnet_ip4_header_t *)nb->data_ptr;
                header_length = (fnet_size_t)FNET_IP_HEADER_GET_HEADER_LENGTH(hdr) << 2;
#else
                _fnet_netbuf_free_chain(nb);
                continue;
#endif
            }
#if FNET_CFG_CPU_ETH_HW_RX_PROTOCOL_CHECKSUM
            else
            {
                if((netif->features & FNET_NETIF_FEATURE_HW_RX_PROTOCOL_CHECKSUM) &&
                   ((hdr->protocol == FNET_PROT_ICMP4) ||
                    (hdr->protocol == FNET_PROT_UDP) ||
                    (hdr->protocol == FNET_PROT_TCP)) )
                {
                    nb->flags |= FNET_NETBUF_FLAG_HW_PROTOCOL_CHECKSUM;
                }
            }
#endif
            if(nb->total_length > FNET_IP4_MAX_PACKET)
            {
                _fnet_netbuf_free_chain(nb); /* Discard datagram */
                continue;
            }

            ip4_nb = _fnet_netbuf_copy(nb, 0u, (header_length + 8u), FNET_FALSE);

            _fnet_netbuf_trim(&nb, (fnet_int32_t)header_length);

            /**************************************
             *  Send to upper layers.
             **************************************/

            /* Prepare addreses for upper protocol.*/
            _fnet_ip4_set_socket_addr(netif, hdr, &src_addr,  &dest_addr );

#if FNET_CFG_RAW
            /* RAW Sockets inpput.*/
            _fnet_raw_input(netif, &src_addr, &dest_addr, nb, ip4_nb);
#endif

            /* Find transport protocol.*/
            if((protocol = _fnet_prot_find(AF_INET, SOCK_UNSPEC, (fnet_uint32_t)hdr->protocol)) != FNET_NULL)
            {
                protocol->prot_input(netif, &src_addr,  &dest_addr, nb, ip4_nb);
                /* After that nb may point to wrong place. Do not use it.*/
            }
            else
                /* No protocol found.*/
            {
                _fnet_netbuf_free_chain(nb);
                _fnet_icmp4_error(netif, FNET_ICMP4_UNREACHABLE, FNET_ICMP4_UNREACHABLE_PROTOCOL, ip4_nb);
            }
        }
        else
        {
            _fnet_netbuf_free_chain(nb);
        }
    } /* while end */

    fnet_isr_unlock();
}

/************************************************************************
* DESCRIPTION: This function attempts to assemble a complete datagram.
*************************************************************************/
#if FNET_CFG_IP4_FRAGMENTATION

static void _fnet_ip4_reassembly( fnet_netbuf_t **nb_ptr )
{
    fnet_ip4_frag_list_t     *frag_list_ptr;
    fnet_ip4_frag_header_t   *frag_ptr;
    fnet_ip4_frag_header_t   *cur_frag_ptr;
    fnet_netbuf_t           *nb = *nb_ptr;
    fnet_ip4_header_t       *iphdr;
    fnet_size_t             i;
    fnet_uint16_t           offset;
    fnet_size_t             hdr_length;

    /* For this algorithm the all datagram must reside in contiguous area of memory.*/
    if(_fnet_netbuf_pullup(&nb, nb->total_length) == FNET_ERR)
    {
        goto DROP_FRAG;
    }

    iphdr = (fnet_ip4_header_t *)nb->data_ptr;

    /* Liner search of the list to locate the appropriate datagram for the current fragment.*/
    for (frag_list_ptr = ip_frag_list_head; frag_list_ptr != 0; frag_list_ptr = frag_list_ptr->next)
    {
        if((frag_list_ptr->id == iphdr->id) && (frag_list_ptr->protocol == iphdr->protocol)
           && (frag_list_ptr->source_addr == iphdr->source_addr)
           && (frag_list_ptr->desination_addr == iphdr->desination_addr))
        {
            break;
        }
    }

    cur_frag_ptr = (fnet_ip4_frag_header_t *)iphdr;

    /* Exclude the standard IP header and options.*/
    hdr_length = (fnet_size_t)FNET_IP_HEADER_GET_HEADER_LENGTH(iphdr) << 2;
    _fnet_netbuf_trim(&nb, (fnet_int32_t)hdr_length);
    cur_frag_ptr->total_length = (fnet_uint16_t)(fnet_ntohs(cur_frag_ptr->total_length) - hdr_length); /* Host endian.*/


    if(frag_list_ptr == 0)                                                  /* The first fragment of the new datagram.*/
    {
        if((frag_list_ptr = (fnet_ip4_frag_list_t *)_fnet_malloc_zero(sizeof(fnet_ip4_frag_list_t))) == 0) /* Create list.*/
        {
            goto DROP_FRAG;
        }

        _fnet_ip4_frag_list_add(&ip_frag_list_head, frag_list_ptr);

        frag_list_ptr->ttl = (fnet_uint8_t)FNET_IP4_FRAG_TTL;
        frag_list_ptr->id = iphdr->id;
        frag_list_ptr->protocol = iphdr->protocol;
        frag_list_ptr->source_addr = iphdr->source_addr;
        frag_list_ptr->desination_addr = iphdr->desination_addr;
        frag_ptr = 0;

        if((iphdr->flags_fragment_offset & FNET_HTONS(FNET_IP4_MF)) != 0u)
        {
            cur_frag_ptr->mf |= 1u;
        }

        cur_frag_ptr->offset = (fnet_uint16_t)(fnet_ntohs(cur_frag_ptr->offset) << 3); /* Convert offset to bytes (Host endian).*/
        cur_frag_ptr->nb = nb;

        _fnet_ip4_frag_add(&frag_list_ptr->frag_ptr, cur_frag_ptr, FNET_NULL);
    }
    else
    {
        if((iphdr->flags_fragment_offset & FNET_HTONS(FNET_IP4_MF)) != 0u)
        {
            cur_frag_ptr->mf |= 1u;
        }

        cur_frag_ptr->offset = (fnet_uint16_t)(fnet_ntohs(cur_frag_ptr->offset) << 3); /* Convert offset to bytes.*/
        cur_frag_ptr->nb = nb;

        /* Find position in reassembly list.*/
        frag_ptr = frag_list_ptr->frag_ptr;
        do
        {
            if(frag_ptr->offset > cur_frag_ptr->offset)
            {
                break;
            }

            frag_ptr = frag_ptr->next;
        }
        while (frag_ptr != frag_list_ptr->frag_ptr);

        /* Trims or discards icoming fragments.*/
        if(frag_ptr != frag_list_ptr->frag_ptr)
        {
            if((i = (fnet_size_t)(frag_ptr->prev->offset + frag_ptr->prev->total_length - cur_frag_ptr->offset)) != 0u)
            {
                if(i >= cur_frag_ptr->total_length)
                {
                    goto DROP_FRAG;
                }

                _fnet_netbuf_trim(&nb, (fnet_int32_t)i);
                cur_frag_ptr->total_length -= (fnet_uint16_t)i;
                cur_frag_ptr->offset += (fnet_uint16_t)i;
            }
        }

        /* Trims or discards existing fragments.*/
        while((frag_ptr != frag_list_ptr->frag_ptr)
              && ((cur_frag_ptr->offset + cur_frag_ptr->total_length) > frag_ptr->offset))
        {
            i = (fnet_size_t)((cur_frag_ptr->offset + cur_frag_ptr->total_length) - frag_ptr->offset);

            if(i < frag_ptr->total_length)
            {
                frag_ptr->total_length -= (fnet_uint16_t)i;
                frag_ptr->offset += (fnet_uint16_t)i;
                _fnet_netbuf_trim((fnet_netbuf_t **)&frag_ptr->nb, (fnet_int32_t)i);
                break;
            }

            frag_ptr = frag_ptr->next;
            _fnet_netbuf_free_chain(frag_ptr->prev->nb);
            _fnet_ip4_frag_del(&frag_list_ptr->frag_ptr, frag_ptr->prev);
        }

        /* Insert fragment to the list.*/
        _fnet_ip4_frag_add(&frag_list_ptr->frag_ptr, cur_frag_ptr, frag_ptr->prev);
    }

    offset = 0u;
    frag_ptr = frag_list_ptr->frag_ptr;

    do
    {
        if(frag_ptr->offset != offset)
        {
            goto NEXT_FRAG;
        }

        offset += frag_ptr->total_length;
        frag_ptr = frag_ptr->next;
    }
    while (frag_ptr != frag_list_ptr->frag_ptr);

    if((frag_ptr->prev->mf & 1u) != 0u)
    {
        goto NEXT_FRAG;
    }

    /* Reconstruct datagram.*/
    frag_ptr = frag_list_ptr->frag_ptr;
    nb = frag_ptr->nb;
    frag_ptr = frag_ptr->next;

    while(frag_ptr != frag_list_ptr->frag_ptr)
    {
        nb = _fnet_netbuf_concat(nb, frag_ptr->nb);

        frag_ptr = frag_ptr->next;
    }

    /* Reconstruct datagram header.*/
    iphdr = (fnet_ip4_header_t *)frag_list_ptr->frag_ptr;
    nb->total_length += (fnet_size_t)FNET_IP_HEADER_GET_HEADER_LENGTH(iphdr) << 2;
    nb->length += (fnet_size_t)FNET_IP_HEADER_GET_HEADER_LENGTH(iphdr) << 2;
    nb->data_ptr = (fnet_uint8_t *)nb->data_ptr - (FNET_IP_HEADER_GET_HEADER_LENGTH(iphdr) << 2);

    iphdr->total_length = fnet_htons((fnet_uint16_t)nb->total_length);

    iphdr->source_addr = frag_list_ptr->source_addr;
    iphdr->desination_addr = frag_list_ptr->desination_addr;
    iphdr->protocol = frag_list_ptr->protocol;
    iphdr->tos &= ~1u;

    _fnet_ip4_frag_list_del(&ip_frag_list_head, frag_list_ptr);
    _fnet_free(frag_list_ptr);

    goto EXIT;

DROP_FRAG:
    _fnet_netbuf_free_chain(nb);
NEXT_FRAG:
    nb = FNET_NULL;
EXIT:
    *nb_ptr =  nb;
}
#endif /* FNET_CFG_IP4_FRAGMENTATION */

/************************************************************************
* DESCRIPTION: IP timer function.
*************************************************************************/
#if FNET_CFG_IP4_FRAGMENTATION

static void _fnet_ip4_timer(fnet_uint32_t cookie)
{
    fnet_ip4_frag_list_t *frag_list_ptr;
    fnet_ip4_frag_list_t *tmp_frag_list_ptr;

    FNET_COMP_UNUSED_ARG(cookie);

    fnet_isr_lock();
    frag_list_ptr = ip_frag_list_head;

    while(frag_list_ptr != 0)
    {
        frag_list_ptr->ttl--;

        if(frag_list_ptr->ttl == 0u)
        {
            tmp_frag_list_ptr = frag_list_ptr->next;
            _fnet_ip4_frag_list_free(frag_list_ptr);
            frag_list_ptr = tmp_frag_list_ptr;
        }
        else
        {
            frag_list_ptr = frag_list_ptr->next;
        }
    }

    fnet_isr_unlock();
}

#endif

/************************************************************************
* DESCRIPTION: This function tries to free not critical parts
*              of memory occupied by the IP module.
*************************************************************************/
void _fnet_ip4_drain( void )
{
    fnet_isr_lock();

#if FNET_CFG_IP4_FRAGMENTATION

    while(((volatile fnet_ip4_frag_list_t *)ip_frag_list_head) != 0)
    {
        _fnet_ip4_frag_list_free(ip_frag_list_head);
    }

#endif

    while(((volatile fnet_netbuf_t *)ip_queue.head) != 0)
    {
        _fnet_netbuf_queue_del(&ip_queue.head, ip_queue.head);
    }

    ip_queue.count = 0u;

    fnet_isr_unlock();
}

/************************************************************************
* DESCRIPTION: This function frees list of datagram fragments.
*************************************************************************/
#if FNET_CFG_IP4_FRAGMENTATION
static void _fnet_ip4_frag_list_free( fnet_ip4_frag_list_t *list )
{
    fnet_netbuf_t *nb;

    if(list)
    {
        fnet_isr_lock();

        while((volatile fnet_ip4_frag_header_t *)(list->frag_ptr) != 0)
        {
            nb = list->frag_ptr->nb;
            _fnet_ip4_frag_del((fnet_ip4_frag_header_t **)(&list->frag_ptr), list->frag_ptr);
            _fnet_netbuf_free_chain(nb);
        }

        _fnet_ip4_frag_list_del(&ip_frag_list_head, list);
        _fnet_free(list);

        fnet_isr_unlock();
    }
}
#endif /* FNET_CFG_IP4_FRAGMENTATION */

/************************************************************************
* DESCRIPTION: Adds frag list to the general frag list.
*************************************************************************/
#if FNET_CFG_IP4_FRAGMENTATION
static void _fnet_ip4_frag_list_add( fnet_ip4_frag_list_t **head, fnet_ip4_frag_list_t *fl )
{
    fl->next = *head;

    if(fl->next != 0)
    {
        fl->next->prev = fl;
    }

    fl->prev = 0;
    *head = fl;
}

#endif /* FNET_CFG_IP4_FRAGMENTATION */

/************************************************************************
* DESCRIPTION: Deletes frag list from the general frag list.
*************************************************************************/
#if FNET_CFG_IP4_FRAGMENTATION
static void _fnet_ip4_frag_list_del( fnet_ip4_frag_list_t **head, fnet_ip4_frag_list_t *fl )
{
    if(fl->prev == 0)
    {
        *head = fl->next;
    }
    else
    {
        fl->prev->next = fl->next;
    }

    if(fl->next != 0)
    {
        fl->next->prev = fl->prev;
    }
}

#endif /* FNET_CFG_IP4_FRAGMENTATION */

/************************************************************************
* DESCRIPTION: Adds frag to the frag list.
*************************************************************************/
#if FNET_CFG_IP4_FRAGMENTATION
static void _fnet_ip4_frag_add( fnet_ip4_frag_header_t *FNET_COMP_PACKED_VAR *head, fnet_ip4_frag_header_t *frag,
                                fnet_ip4_frag_header_t *frag_prev )
{
    if(frag_prev && ( *head))
    {
        frag->next = frag_prev->next;
        frag->prev = frag_prev;
        frag_prev->next->prev = frag;
        frag_prev->next = frag;

        if((*head)->offset > frag->offset)
        {
            *head = frag;
        }
    }
    else
    {
        frag->next = frag;
        frag->prev = frag;
        *head = frag;
    }
}
#endif /* FNET_CFG_IP4_FRAGMENTATION */

/************************************************************************
* DESCRIPTION: Deletes frag from the frag list.
*************************************************************************/
#if FNET_CFG_IP4_FRAGMENTATION
static void _fnet_ip4_frag_del( fnet_ip4_frag_header_t *FNET_COMP_PACKED_VAR *head, fnet_ip4_frag_header_t *frag )
{
    if(frag->prev == frag)
    {
        *head = 0;
    }
    else
    {
        frag->prev->next = frag->next;
        frag->next->prev = frag->prev;

        if(*head == frag)
        {
            *head = frag->next;
        }
    }
}
#endif /* FNET_CFG_IP4_FRAGMENTATION */

#if FNET_CFG_MULTICAST

/************************************************************************
* DESCRIPTION: Join a multicast group. Returns pointer to the entry in
*              the multicast list, or FNET_NULL if any error;
*************************************************************************/
fnet_ip4_multicast_list_entry_t *_fnet_ip4_multicast_join( fnet_netif_t *netif, fnet_ip4_addr_t group_addr )
{
    fnet_index_t                    i;
    fnet_ip4_multicast_list_entry_t *result = FNET_NULL;

    /* Find existing entry or free one.*/
    for(i = 0u; i < FNET_CFG_MULTICAST_MAX; i++)
    {
        if(fnet_ip4_multicast_list[i].user_counter > 0u)
        {
            if((fnet_ip4_multicast_list[i].netif == netif) && (fnet_ip4_multicast_list[i].group_addr == group_addr))
            {
                result = &fnet_ip4_multicast_list[i];
                break; /* Found.*/
            }
        }
        else /* user_counter == 0.*/
        {
            result = &fnet_ip4_multicast_list[i]; /* Save the last free.*/
        }
    }

    if(result)
    {
        result->user_counter++; /* Increment user counter.*/

        if(result->user_counter == 1u) /* New entry.*/
        {
            result->group_addr = group_addr;
            result->netif = netif;

            /* Join HW interface. */
            _fnet_netif_join_ip4_multicast ( (fnet_netif_desc_t) netif, group_addr );

#if FNET_CFG_IGMP  /* Send IGMP report.*/
            /*
             * When a host joins a new group, it should immediately transmit a
             * Report for that group.
             * TBD To cover the possibility of the initial Report being lost or damaged, it is
             * recommended that it be repeated once or twice after short delays.
             */
            _fnet_igmp_join(netif, group_addr );
#endif /* FNET_CFG_IGMP */
        }
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Leave a multicast group.
*************************************************************************/
void _fnet_ip4_multicast_leave_entry( fnet_ip4_multicast_list_entry_t *multicastentry )
{
    if(multicastentry)
    {
        multicastentry->user_counter--; /* Decrement user counter.*/

        if(multicastentry->user_counter == 0u)
        {

#if FNET_CFG_IGMP  /* Send IGMP leave.*/
            /* Leave via IGMP */
            _fnet_igmp_leave( multicastentry->netif, multicastentry->group_addr );
#endif /* FNET_CFG_IGMP */

            /* Leave HW interface. */
            _fnet_netif_leave_ip4_multicast ( (fnet_netif_desc_t) multicastentry->netif, multicastentry->group_addr );
        }
    }
}

#endif /* FNET_CFG_MULTICAST */

/************************************************************************
* DESCRIPTION: Is the address is broadcast?
*************************************************************************/
fnet_bool_t _fnet_ip4_addr_is_broadcast( fnet_ip4_addr_t addr, fnet_netif_t *netif )
{
    fnet_netif_ip4_addr_t   *netif_addr;
    fnet_bool_t             result = FNET_FALSE;

    if((addr == INADDR_BROADCAST)                   /* Limited broadcast */
       || (addr == INADDR_ANY)
       || (addr == FNET_IP4_ADDR_LINK_LOCAL_BROADCAST) )/* Link-local broadcast (RFC3927)*/
    {
        result = FNET_TRUE;
    }
    else if(netif == FNET_NULL)
    {
        _fnet_stack_mutex_lock();
        for (netif = fnet_netif_list; netif != 0; netif = netif->next)
        {
            netif_addr = &(netif->ip4_addr);

            if( (addr == netif_addr->netbroadcast) ||          /* Net-directed broadcast */
                (addr == netif_addr->subnetbroadcast) ||      /* Subnet-directed broadcast */
                (addr == netif_addr->subnet))
            {
                result = FNET_TRUE;
                break;
            }
        }
        _fnet_stack_mutex_unlock();
    }
    else
    {
        netif_addr = &(netif->ip4_addr);

        if( (addr == netif_addr->netbroadcast) ||              /* Net-directed broadcast */
            (addr == netif_addr->subnetbroadcast) ||          /* Subnet-directed broadcast */
            (addr == netif_addr->subnet))
        {
            result = FNET_TRUE;
        }
    }
    return result;
}

/*Todo path MTU discovery feature
 *Todo get MTU from a routing table, depending on destination MTU*/
fnet_size_t _fnet_ip4_maximum_packet( fnet_ip4_addr_t dest_ip )
{
    fnet_size_t result;

#if FNET_CFG_IP4_FRAGMENTATION == 0

    {
        fnet_netif_t *netif;

        if((netif = _fnet_ip4_route(dest_ip)) == 0) /* No route*/
        {
            result = FNET_IP4_MAX_PACKET;
        }
        else
        {
            result = netif->netif_mtu;
        }
    }

#else

    FNET_COMP_UNUSED_ARG(dest_ip);

    result = FNET_IP4_MAX_PACKET;

#endif

    result = (result - (FNET_IP4_MAX_OPTIONS + sizeof(fnet_ip4_header_t))) & (~0x3LU);

    return result;
}

/************************************************************************
* DESCRIPTION: This function retrieves the current value
*              of IPv4 socket option.
*************************************************************************/
fnet_error_t _fnet_ip4_getsockopt(struct _fnet_socket_if_t *sock, fnet_socket_options_t optname, void *optval, fnet_size_t *optlen )
{
    fnet_error_t result = FNET_ERR_OK;

    switch(optname)      /* Socket options processing. */
    {
        case IP_TOS: /* Get IP TOS for outgoing datagrams.*/
            if(*optlen < sizeof(fnet_uint32_t))
            {
                result = FNET_ERR_INVAL;
                break;
            }

            *((fnet_uint32_t *)optval) = (fnet_uint32_t)sock->options.ip4_opt.tos;
            *optlen = sizeof(fnet_uint32_t);
            break;
        case IP_TTL: /* Get IP TTL for outgoing datagrams.*/
            if(*optlen < sizeof(fnet_uint32_t))
            {
                result = FNET_ERR_INVAL;
                break;
            }

            *((fnet_uint32_t *)optval) = (fnet_uint32_t)sock->options.ip4_opt.ttl;
            *optlen = sizeof(fnet_uint32_t);
            break;
#if FNET_CFG_MULTICAST
        case IP_MULTICAST_TTL: /* Get IP TTL for outgoing datagrams.*/
            if(*optlen < sizeof(fnet_uint32_t))
            {
                result = FNET_ERR_INVAL;
                break;
            }

            *((fnet_uint32_t *)optval) = (fnet_uint32_t)sock->options.ip4_opt.ttl_multicast;
            *optlen = sizeof(fnet_uint32_t);
            break;
#endif /* FNET_CFG_MULTICAST */
        default:
            result = FNET_ERR_NOPROTOOPT; /* The option is unknown or unsupported.*/
            break;
    }

    return result;
}

/************************************************************************
* DESCRIPTION: This function sets the value of IPv4 socket option.
*************************************************************************/
fnet_error_t _fnet_ip4_setsockopt(struct _fnet_socket_if_t  *sock, fnet_socket_options_t optname, const void *optval, fnet_size_t optlen )
{
    fnet_error_t result = FNET_ERR_OK;

    switch(optname)      /* Socket options processing. */
    {
        /******************************/
        case IP_TOS: /* Set IP TOS for outgoing datagrams. */
            if(optlen < sizeof(fnet_uint32_t))
            {
                result = FNET_ERR_INVAL;
                break;
            }

            sock->options.ip4_opt.tos = (fnet_uint8_t) (*((const fnet_uint32_t *)(optval)));
            break;
        /******************************/
        case IP_TTL: /* Set IP TTL for outgoing datagrams. */
            if(optlen < sizeof(fnet_uint32_t))
            {
                result = FNET_ERR_INVAL;
                break;
            }

            sock->options.ip4_opt.ttl = (fnet_uint8_t) (*((const fnet_uint32_t *)(optval)));
            break;
#if FNET_CFG_MULTICAST
        /******************************/
        case IP_MULTICAST_TTL: /* Set IP TTL for outgoing Multicast datagrams. */
            /* Validation.*/
            if( (optlen < sizeof(fnet_uint32_t)) || (!(sock->protocol_interface)) || (sock->protocol_interface->type != SOCK_DGRAM )  )
            {
                result = FNET_ERR_INVAL;
                break;
            }
            sock->options.ip4_opt.ttl_multicast = (fnet_uint8_t) (*((const fnet_uint32_t *)(optval)));
            break;
        /******************************/
        case IP_ADD_MEMBERSHIP:     /* Join the socket to the supplied multicast group on
                                     * the specified interface. */
        case IP_DROP_MEMBERSHIP:    /* Drops membership to the given multicast group and interface.*/
        {
            fnet_index_t                    i;
            fnet_ip4_multicast_list_entry_t **multicast_entry = FNET_NULL;
            const struct fnet_ip_mreq       *mreq = (const struct fnet_ip_mreq *)optval;
            fnet_netif_t                    *netif;


            if(mreq->imr_interface == 0u)
            {
                netif = _fnet_netif_get_default();
            }
            else
            {
                netif = _fnet_netif_get_by_scope_id(mreq->imr_interface);
            }

            if((optlen != sizeof(struct fnet_ip_mreq)) /* Check size.*/
               || (netif == FNET_NULL) /* Found IF.*/
               || (!FNET_IP4_ADDR_IS_MULTICAST(mreq->imr_multiaddr.s_addr)) /* Check if the address is multicast.*/
               || (!(sock->protocol_interface)) || (sock->protocol_interface->type != SOCK_DGRAM )
              )
            {
                result = FNET_ERR_INVAL;
                break;
            }

            /* Find the existing entry with same parameters.*/
            for(i = 0u; i < FNET_CFG_MULTICAST_SOCKET_MAX; i++)
            {
                if( (sock->ip4_multicast_entry[i] != FNET_NULL)
                    && (sock->ip4_multicast_entry[i]->netif == netif)
                    && (sock->ip4_multicast_entry[i]->group_addr == mreq->imr_multiaddr.s_addr) )
                {
                    multicast_entry = &sock->ip4_multicast_entry[i];
                    break; /* Found.*/
                }
            }

            /******************************/
            if(optname == IP_ADD_MEMBERSHIP)
            {
                if(multicast_entry != FNET_NULL)
                {
                    /* Already joined.*/
                    result = FNET_ERR_ADDRINUSE;
                    break;
                }

                /* Find free entry.*/
                for(i = 0u; i < FNET_CFG_MULTICAST_SOCKET_MAX; i++)
                {
                    if(sock->ip4_multicast_entry[i] == FNET_NULL)
                    {
                        multicast_entry = &sock->ip4_multicast_entry[i];
                        break; /* Found.*/
                    }
                }

                if(multicast_entry != FNET_NULL)
                {
                    *multicast_entry = _fnet_ip4_multicast_join( netif, mreq->imr_multiaddr.s_addr );

                    if(*multicast_entry == FNET_NULL)
                    {
                        result = FNET_ERR_ADDRINUSE;
                        break;
                    }
                }
                else
                {
                    result = FNET_ERR_ADDRINUSE;
                    break;
                }

            }
            /******************************/
            else /* IP_DROP_MEMBERSHIP */
            {
                if(multicast_entry != FNET_NULL)
                {
                    /* Leave the group.*/
                    _fnet_ip4_multicast_leave_entry(*multicast_entry);
                    *multicast_entry = FNET_NULL;
                }
                else
                {
                    /* Join entry is not foud.*/
                    result = FNET_ERR_INVAL;
                    break;
                }
            }
        }
        break;
#endif /* FNET_CFG_MULTICAST */
        /******************************/
        default:
            result = FNET_ERR_NOPROTOOPT; /* The option is unknown or unsupported. */
            break;
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Prints an IP header. For debug needs only.
*************************************************************************/
#if FNET_CFG_DEBUG_TRACE_IP4 && FNET_CFG_DEBUG_TRACE
static void _fnet_ip4_trace(const fnet_char_t *str, fnet_ip4_header_t *ip_hdr)
{
    fnet_char_t ip_str[FNET_IP4_ADDR_STR_SIZE];

    fnet_printf(FNET_SERIAL_ESC_FG_GREEN"%s", str); /* Print app-specific header.*/
    fnet_println("[IPv4 header]"FNET_SERIAL_ESC_ATTR_RESET);
    fnet_println("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+");
    fnet_println("|(V) %2d |(HL)%2d |(TOS)     0x%02x |(L)                      %5u |",
                 FNET_IP_HEADER_GET_VERSION(ip_hdr),
                 FNET_IP_HEADER_GET_HEADER_LENGTH(ip_hdr),
                 ip_hdr->tos,
                 fnet_ntohs(ip_hdr->total_length));
    fnet_println("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+");
    fnet_println("|(Id)                     %5u |(F)%u |(Offset)            %4u |",
                 fnet_ntohs(ip_hdr->id),
                 fnet_ntohs(FNET_IP_HEADER_GET_FLAG(ip_hdr)) >> 15,
                 fnet_ntohs(FNET_IP_HEADER_GET_OFFSET(ip_hdr)));
    fnet_println("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+");
    fnet_println("|(TTL)      %3u |(Proto)    "FNET_SERIAL_ESC_FG_BLUE"%3u"FNET_SERIAL_ESC_ATTR_RESET" |(Cheksum)               0x%04x |",
                 ip_hdr->ttl,
                 ip_hdr->protocol,
                 fnet_ntohs(ip_hdr->checksum));
    fnet_println("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+");
    fnet_println("|(Src)                                          "FNET_SERIAL_ESC_FG_BLUE"%15s"FNET_SERIAL_ESC_ATTR_RESET" |",
                 fnet_inet_ntoa(*(struct fnet_in_addr *)(&ip_hdr->source_addr), ip_str));
    fnet_println("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+");
    fnet_println("|(Dest)                                         "FNET_SERIAL_ESC_FG_BLUE"%15s"FNET_SERIAL_ESC_ATTR_RESET" |",
                 fnet_inet_ntoa(*(struct fnet_in_addr *)(&ip_hdr->desination_addr), ip_str));
    fnet_println("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+");

}
#endif /* FNET_CFG_DEBUG_TRACE_IP4 */

#endif /* FNET_CFG_IP4 */
