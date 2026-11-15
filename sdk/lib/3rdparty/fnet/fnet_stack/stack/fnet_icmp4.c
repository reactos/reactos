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
*  ICMPv4 protocol implementation.
*
***************************************************************************/

#include "fnet.h"

#if FNET_CFG_IP4

#include "fnet_icmp4.h"
#include "fnet_timer.h"
#include "fnet_prot.h"
#include "fnet_socket_prv.h"
#include "fnet_checksum_prv.h"

/************************************************************************
*     Function Prototypes
*************************************************************************/
static void _fnet_icmp4_input(fnet_netif_t *netif, struct fnet_sockaddr *src_addr,  struct fnet_sockaddr *dest_addr, fnet_netbuf_t *nb, fnet_netbuf_t *ip4_nb);
static void _fnet_icmp4_output( fnet_netif_t *netif, fnet_ip4_addr_t src_ip, fnet_ip4_addr_t dest_ip, fnet_netbuf_t *nb );
static void _fnet_icmp4_notify_protocol(fnet_netif_t *netif, fnet_prot_notify_t prot_cmd, fnet_netbuf_t *nb );

#if FNET_CFG_DEBUG_TRACE_ICMP4 && FNET_CFG_DEBUG_TRACE
    void fnet_icmp4_trace(const fnet_char_t *str, fnet_icmp4_header_t *icmpp_hdr);
#else
    #define fnet_icmp4_trace(str, icmp_hdr)  do{}while(0)
#endif

/************************************************************************
* Protocol API structure.
************************************************************************/
fnet_prot_if_t fnet_icmp4_prot_if =
{
    .family = AF_INET,              /* Address domain family.*/
    .type = SOCK_UNSPEC,            /* Socket type used for.*/
    .protocol = FNET_PROT_ICMP4,    /* Protocol number.*/
    .prot_input = _fnet_icmp4_input, /* Protocol input function.*/
};

/************************************************************************
* DESCRIPTION: ICMP input function.
*************************************************************************/
static void _fnet_icmp4_input(fnet_netif_t *netif, struct fnet_sockaddr *src_addr,  struct fnet_sockaddr *dest_addr, fnet_netbuf_t *nb, fnet_netbuf_t *ip4_nb)
{
    fnet_icmp4_header_t     *hdr;
    fnet_prot_notify_t      prot_cmd;
    fnet_ip4_addr_t         src_ip_rx;
    fnet_ip4_addr_t         dest_ip_rx;
    fnet_ip4_addr_t         src_ip_tx;
    fnet_ip4_addr_t         dest_ip_tx;

    _fnet_netbuf_free_chain(ip4_nb); /* Not used.*/

    if((netif != 0) && (nb != 0) )
    {
        if(_fnet_netbuf_pullup(&nb, sizeof(fnet_icmp4_header_t)) == FNET_ERR) /* The header must reside in contiguous area of memory. */
        {
            goto DISCARD;
        }

        hdr = (fnet_icmp4_header_t *)nb->data_ptr;

        src_ip_rx = ((struct fnet_sockaddr_in *)(src_addr))->sin_addr.s_addr;
        dest_ip_rx = ((struct fnet_sockaddr_in *)(dest_addr))->sin_addr.s_addr;

        /* Swap source and destination addresses.*/
        src_ip_tx = dest_ip_rx;
        dest_ip_tx = src_ip_rx;

        if((
#if FNET_CFG_CPU_ETH_HW_RX_PROTOCOL_CHECKSUM || FNET_CFG_CPU_ETH_HW_TX_PROTOCOL_CHECKSUM
               ((nb->flags & FNET_NETBUF_FLAG_HW_PROTOCOL_CHECKSUM) == 0) &&
#endif
               (_fnet_checksum_netbuf(nb, nb->total_length)))
           || (_fnet_ip4_addr_is_broadcast(src_ip_rx, netif))
           || FNET_IP4_ADDR_IS_MULTICAST(src_ip_rx))
        {
            goto DISCARD;
        }

        fnet_icmp4_trace("RX", hdr);

        switch(hdr->type)
        {
            /**************************
             * ICMP Request Processing
             **************************/
            case FNET_ICMP4_ECHO:
                if((nb->total_length < sizeof(fnet_icmp4_echo_header_t)) ||
                   /* An ICMP Echo Request destined to an IP broadcast or IP
                   * multicast address MAY be silently discarded.(RFC1122)*/
                   (_fnet_ip4_addr_is_broadcast(dest_ip_rx, netif)) || FNET_IP4_ADDR_IS_MULTICAST(dest_ip_rx))
                {
                    goto DISCARD;
                }

                hdr->type = FNET_ICMP4_ECHOREPLY;

                _fnet_icmp4_output(netif, src_ip_tx, dest_ip_tx, nb);
                break;
#if 0 /* Optional functionality.*/
            /************************
             * Time Stamp Query
             ************************/
            case FNET_ICMP4_TSTAMP:

                /* The header must reside in contiguous area of memory. */
                if(_fnet_netbuf_pullup(&nb, sizeof(fnet_icmp4_timestamp_header_t)) == FNET_ERR)
                {
                    goto DISCARD;
                }

                hdr = nb->data_ptr;

                hdr->type = FNET_ICMP4_TSTAMPREPLY;
                /* All times are in milliseconds since the stack timer start modulo 1 day.
                * The high-order bit is set as the time value is recorded in nonstandard units. */
                ((fnet_icmp4_timestamp_header_t *)hdr)->receive_timestamp
                    = fnet_htonl((fnet_timer_get_ms() % (24 * FNET_TIMER_MS_IN_HOUR)) | (0x80000000));

                dest_ip = netif->ip4_addr.address;

                _fnet_icmp4_output(netif, src_ip_tx, dest_ip_tx, nb);
                break;
            /************************
             * Address Mask Query
             ************************/
            case FNET_ICMP4_MASKREQ:
                /* The header must reside in contiguous area of memory*/
                if(_fnet_netbuf_pullup(&nb, sizeof(fnet_icmp4_mask_header_t)) == FNET_ERR)
                {
                    goto DISCARD;
                }

                hdr = nb->data_ptr;

                hdr->type = FNET_ICMP4_MASKREPLY;

                ((fnet_icmp4_mask_header_t *)hdr)->mask = netif->ip4_addr.subnetmask;

                dest_ip = netif->ip4_addr.address;

                _fnet_icmp4_output(netif, src_ip_tx, dest_ip_tx, nb);
                break;
#endif
            /**************************
             * ICMP Error Processing
             **************************/
            case FNET_ICMP4_UNREACHABLE:
                switch(hdr->code)
                {
                    case FNET_ICMP4_UNREACHABLE_NET:           /* net unreachable */
                    case FNET_ICMP4_UNREACHABLE_NET_UNKNOWN:   /* unknown net */
                    case FNET_ICMP4_UNREACHABLE_NET_PROHIB:    /* prohibited access */
                    case FNET_ICMP4_UNREACHABLE_TOSNET:        /* bad tos for net */
                        prot_cmd = FNET_PROT_NOTIFY_UNREACH_NET;
                        break;

                    case FNET_ICMP4_UNREACHABLE_HOST:          /* host unreachable */
                    case FNET_ICMP4_UNREACHABLE_HOST_UNKNOWN:  /* unknown host */
                    case FNET_ICMP4_UNREACHABLE_ISOLATED:      /* src host isolated */
                    case FNET_ICMP4_UNREACHABLE_HOST_PROHIB:   /* ditto */
                    case FNET_ICMP4_UNREACHABLE_TOSHOST:       /* bad tos for host */
                        prot_cmd = FNET_PROT_NOTIFY_UNREACH_HOST;
                        break;

                    case FNET_ICMP4_UNREACHABLE_PROTOCOL:      /* protocol unreachable */
                        prot_cmd = FNET_PROT_NOTIFY_UNREACH_PROTOCOL;
                        break;

                    case FNET_ICMP4_UNREACHABLE_PORT:          /* port unreachable */
                        prot_cmd = FNET_PROT_NOTIFY_UNREACH_PORT;
                        break;

                    case FNET_ICMP4_UNREACHABLE_SRCFAIL:       /* source route failed */
                        prot_cmd = FNET_PROT_NOTIFY_UNREACH_SRCFAIL;
                        break;

                    case FNET_ICMP4_UNREACHABLE_NEEDFRAG:      /* fragmentation needed and DF set*/
                        prot_cmd = FNET_PROT_NOTIFY_MSGSIZE;
                        break;

                    default:
                        goto DISCARD;
                }
                _fnet_icmp4_notify_protocol(netif, prot_cmd, nb);  /* Protocol notification.*/
                break;
            case FNET_ICMP4_TIMXCEED:
                switch(hdr->code)
                {
                    case FNET_ICMP4_TIMXCEED_INTRANS:          /* time to live exceeded in transit (ttl==0)*/
                        prot_cmd = FNET_PROT_NOTIFY_TIMXCEED_INTRANS;
                        break;

                    case FNET_ICMP4_TIMXCEED_REASS:            /* fragment reassembly time exceeded (ttl==0)*/
                        prot_cmd = FNET_PROT_NOTIFY_TIMXCEED_REASS;
                        break;

                    default:
                        goto DISCARD;
                }

                _fnet_icmp4_notify_protocol(netif, prot_cmd, nb);  /* Protocol notification.*/
                break;
            case FNET_ICMP4_PARAMPROB:                       /* Parameter Problem Message.*/
                if(hdr->code > 1u)
                {
                    goto DISCARD;
                }

                prot_cmd = FNET_PROT_NOTIFY_PARAMPROB;
                _fnet_icmp4_notify_protocol(netif, prot_cmd, nb);  /* Protocol notification.*/
                break;
            case FNET_ICMP4_SOURCEQUENCH:                    /* Source Quench Message; packet lost, slow down.*/
                if(hdr->code)
                {
                    goto DISCARD;
                }

                prot_cmd = FNET_PROT_NOTIFY_QUENCH;
                _fnet_icmp4_notify_protocol(netif, prot_cmd, nb);  /* Protocol notification.*/
                break;
            /************************
             * Ignore others
             ************************/
            /*
            case FNET_ICMP4_REDIRECT:
            case FNET_ICMP4_ECHOREPLY:
            case FNET_ICMP4_ROUTERADVERT:
            case FNET_ICMP4_ROUTERSOLICIT:
            case FNET_ICMP4_TSTAMPREPLY:
            case FNET_ICMP4_IREQREPLY:
            case FNET_ICMP4_MASKREPLY:*/
            default:
                goto DISCARD;

        }/* switch(hdr->type) */
    }
    else
    {
    DISCARD:
        _fnet_netbuf_free_chain(nb);
    }
}

/************************************************************************
* DESCRIPTION: Upper protocol notification..
*************************************************************************/
static void _fnet_icmp4_notify_protocol(fnet_netif_t *netif, fnet_prot_notify_t prot_cmd, fnet_netbuf_t *nb)
{
    fnet_icmp4_err_header_t *hdr_err = (fnet_icmp4_err_header_t *)nb->data_ptr;
    fnet_ip4_header_t       *ip_header = &hdr_err->ip;
    fnet_size_t             hdr_err_length = sizeof(fnet_icmp4_err_header_t) /*+ ((FNET_IP_HEADER_GET_HEADER_LENGTH(ip_header) << 2) - sizeof(fnet_ip4_header_t))*/;
    fnet_size_t             hdr_err_data_length = hdr_err_length + 8u; /* 8 bytes is enough for transport protocol (port numbers).*/
    fnet_prot_if_t          *protocol;

    if(nb->total_length < hdr_err_data_length)
    {
        goto DISCARD;
    }

    if(nb->total_length > hdr_err_data_length)
    {
        _fnet_netbuf_trim(&nb, (fnet_int32_t)hdr_err_data_length - (fnet_int32_t)nb->total_length);
    }

    if(_fnet_netbuf_pullup(&nb, nb->total_length) == FNET_ERR) /* The header must reside in contiguous area of memory.*/
    {
        goto DISCARD;
    }

    hdr_err = (fnet_icmp4_err_header_t *)nb->data_ptr;
    ip_header = &hdr_err->ip;

    if((protocol = _fnet_prot_find(AF_INET, SOCK_UNSPEC, (fnet_uint32_t)hdr_err->ip.protocol)) != 0)
    {
        if(protocol->prot_control_input)
        {
            struct fnet_sockaddr     err_src_addr;
            struct fnet_sockaddr     err_dest_addr;

            /* Prepare addreses for upper protocol.*/
            _fnet_ip4_set_socket_addr(netif, ip_header, &err_src_addr,  &err_dest_addr );

            _fnet_netbuf_trim(&nb, (fnet_int32_t)(hdr_err_length)); /* Cut the ICMP error header.*/

            protocol->prot_control_input(prot_cmd, &err_src_addr, &err_dest_addr, nb);
        }
    }

DISCARD:
    _fnet_netbuf_free_chain(nb);
}


/************************************************************************
* DESCRIPTION: ICMP output function.
*************************************************************************/
static void _fnet_icmp4_output( fnet_netif_t *netif, fnet_ip4_addr_t src_ip,
                                fnet_ip4_addr_t dest_ip, fnet_netbuf_t *nb )
{
    fnet_icmp4_header_t *hdr = (fnet_icmp4_header_t *)nb->data_ptr;

    hdr->checksum = 0u;
#if FNET_CFG_CPU_ETH_HW_TX_PROTOCOL_CHECKSUM
    if( netif
        && (netif->features & FNET_NETIF_FEATURE_HW_TX_PROTOCOL_CHECKSUM)
        && (_fnet_ip4_will_fragment(netif, nb->total_length) == FNET_FALSE) /* Fragmented packets are not inspected.*/  )
    {
        nb->flags |= FNET_NETBUF_FLAG_HW_PROTOCOL_CHECKSUM;
    }
    else
#endif
        hdr->checksum = _fnet_checksum_netbuf(nb, nb->total_length);

    _fnet_ip4_output(netif, src_ip, dest_ip, FNET_PROT_ICMP4, FNET_ICMP4_TOS, FNET_ICMP4_TTL, nb, FNET_FALSE, FNET_FALSE, 0);
}

/************************************************************************
* DESCRIPTION: Sends ICMP error message.
*************************************************************************/
void _fnet_icmp4_error( fnet_netif_t *netif, fnet_uint8_t type, fnet_uint8_t code, fnet_netbuf_t *nb )
{
    fnet_ip4_header_t       *ipheader;
    fnet_netbuf_t           *nb_header;
    fnet_icmp4_err_header_t *icmpheader;
    fnet_ip4_addr_t         source_addr;
    fnet_ip4_addr_t         destination_addr;

    if(nb)
    {
        ipheader = (fnet_ip4_header_t *)nb->data_ptr;

        source_addr = ipheader->source_addr;
        destination_addr = ipheader->desination_addr;

        /* Do not send error if not the first fragment of message (RFC1122)*/
        if((FNET_IP_HEADER_GET_OFFSET(ipheader) != 0u) ||
           /* Do not send error on ICMP error messages*/
           ((ipheader->protocol == FNET_PROT_ICMP4)
            && (!FNET_ICMP4_IS_QUERY_TYPE(((fnet_icmp4_header_t *)((fnet_uint8_t *)(nb->data_ptr) + (FNET_IP_HEADER_GET_HEADER_LENGTH(ipheader) << 2)))->type)))
           /* Do not send error on a datagram whose source address does not define a single
            * host -- e.g., a zero address, a loopback address, a
            * broadcast address, a multicast address, or a Class E
            * address.*/
           || (_fnet_ip4_addr_is_broadcast(source_addr, netif))
           || FNET_IP4_ADDR_IS_MULTICAST(source_addr)
           || FNET_IP4_CLASS_E(source_addr)
           /* Do not send error on a datagram destined to an IP broadcast or IP multicast address*/
           || (_fnet_ip4_addr_is_broadcast(destination_addr, netif))
           || FNET_IP4_ADDR_IS_MULTICAST(destination_addr)
           /* Do not send error on datagram sent as a link-layer broadcast or multicast.*/
           || ((nb->flags & FNET_NETBUF_FLAG_BROADCAST) != 0u) || ((nb->flags & FNET_NETBUF_FLAG_MULTICAST) != 0u)
          )
        {
            goto FREE_NB;
        }

        /* Construct ICMP error header*/
        if((nb_header = _fnet_netbuf_new((sizeof(fnet_icmp4_err_header_t) - sizeof(fnet_ip4_header_t)), FNET_FALSE)) == 0)
        {
            goto FREE_NB;
        }

        icmpheader = (fnet_icmp4_err_header_t *)nb_header->data_ptr;
        icmpheader->fields.unused = 0u;

        if((type == FNET_ICMP4_PARAMPROB) && (code == FNET_ICMP4_UNREACHABLE_NEEDFRAG) && netif)
        {
            icmpheader->fields.mtu = fnet_htons((fnet_uint16_t)netif->netif_mtu);
        }
        else if(type == FNET_ICMP4_PARAMPROB)
        {
            icmpheader->fields.ptr = fnet_htons((fnet_uint16_t)code);
            code = 0u;
        }
        else
        {}

        icmpheader->header.type = type;
        icmpheader->header.code = code;

        if((fnet_size_t)((FNET_IP_HEADER_GET_HEADER_LENGTH(ipheader) << 2) + 8u) < nb->total_length)
        {
            _fnet_netbuf_trim(&nb, (fnet_int32_t)((fnet_size_t)((FNET_IP_HEADER_GET_HEADER_LENGTH(ipheader) << 2) + 8u) - nb->total_length));
        }

        nb = _fnet_netbuf_concat(nb_header, nb);

        _fnet_icmp4_output(netif, destination_addr, source_addr, nb);

        return;

    FREE_NB:
        _fnet_netbuf_free_chain(nb);
    }
}

/************************************************************************
* DESCRIPTION: Prints an ICMP header. For debug needs only.
*************************************************************************/
#if FNET_CFG_DEBUG_TRACE_ICMP4 && FNET_CFG_DEBUG_TRACE
void fnet_icmp4_trace(const fnet_char_t *str, fnet_icmp4_header_t *icmp_hdr)
{
    fnet_printf(FNET_SERIAL_ESC_FG_GREEN"%s", str); /* Print app-specific header.*/
    fnet_println("[ICMP4 header]"FNET_SERIAL_ESC_ATTR_RESET);
    fnet_println("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+");
    fnet_println("|(Type)     "FNET_SERIAL_ESC_FG_BLUE"%3u"FNET_SERIAL_ESC_ATTR_RESET" |(Code)     %3u |(Cheksum)               0x%04x |",
                 icmp_hdr->type,
                 icmp_hdr->code,
                 fnet_ntohs(icmp_hdr->checksum));
    fnet_println("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+");
}
#endif /* FNET_CFG_DEBUG_TRACE_ICMP4 */

#endif /* FNET_CFG_IP4 */
