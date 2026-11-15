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
*  ICMPv6 protocol implementation.
*
***************************************************************************/
#include "fnet.h"

#if FNET_CFG_IP6

#include "fnet_icmp6.h"
#include "fnet_loop.h"
#include "fnet_timer.h"
#include "fnet_prot.h"
#include "fnet_socket.h"
#include "fnet_socket_prv.h"
#include "fnet_checksum_prv.h"
#include "fnet_mld.h"

/************************************************************************
*     Function Prototypes
*************************************************************************/
static void _fnet_icmp6_input(fnet_netif_t *netif, struct fnet_sockaddr *src_addr,  struct fnet_sockaddr *dest_addr, fnet_netbuf_t *nb, fnet_netbuf_t *ip6_nb);

/************************************************************************
* Protocol API structure.
************************************************************************/
fnet_prot_if_t fnet_icmp6_prot_if =
{
    0,                      /* Pointer to the head of the protocol's socket list.*/
    AF_INET6,               /* Address domain family.*/
    SOCK_UNSPEC,            /* Socket type used for.*/
    FNET_PROT_ICMP6,        /* Protocol number.*/
    0,                      /* Protocol initialization function.*/
    0,                      /* Protocol release function.*/
    _fnet_icmp6_input,      /* Protocol input function,.*/
    0,                      /* Protocol input control function.*/
    0,                      /* protocol drain function.*/
    0                       /* Socket API */
};

/************************************************************************
* DESCRIPTION: ICMPv6 input function.
*************************************************************************/
static void _fnet_icmp6_input(fnet_netif_t *netif, struct fnet_sockaddr *src_addr,  struct fnet_sockaddr *dest_addr, fnet_netbuf_t *nb, fnet_netbuf_t *ip6_nb)
{
    fnet_icmp6_header_t     *hdr;
    fnet_uint16_t           sum;
    fnet_ip6_addr_t         *src_ip_rx;
    fnet_ip6_addr_t         *dest_ip_rx;
    fnet_ip6_addr_t         *src_ip_tx;
    fnet_ip6_addr_t         *dest_ip_tx;
    fnet_prot_notify_t      prot_cmd;
    fnet_bool_t             discard_flag = FNET_FALSE;

    if((netif != 0) && (nb != 0))
    {
        /* The header must reside in contiguous area of memory. */
        if(_fnet_netbuf_pullup(&nb, sizeof(fnet_icmp6_header_t)) == FNET_ERR)
        {
            goto DISCARD;
        }

        hdr = (fnet_icmp6_header_t *)nb->data_ptr;

        dest_ip_rx = &((struct fnet_sockaddr_in6 *)(dest_addr))->sin6_addr.s6_addr;
        src_ip_rx = &((struct fnet_sockaddr_in6 *)(src_addr))->sin6_addr.s6_addr;

        /* Swap source and destination addresses.*/
        dest_ip_tx = src_ip_rx;
        src_ip_tx = dest_ip_rx;

        /* Drop Multicast loopback.*/
#if FNET_CFG_LOOPBACK
        if ((netif == FNET_LOOP_IF) && FNET_IP6_ADDR_IS_MULTICAST(dest_ip_rx))
        {
            goto DISCARD;
        }
#endif /* FNET_CFG_LOOPBACK */

        /* Verify the checksum. */
        sum = _fnet_checksum_pseudo_netbuf_start( nb, FNET_HTONS((fnet_uint16_t)FNET_PROT_ICMP6), (fnet_uint16_t)nb->total_length );
        sum = _fnet_checksum_pseudo_netbuf_end( sum, (fnet_uint8_t *)src_ip_rx, (fnet_uint8_t *)dest_ip_rx, sizeof(fnet_ip6_addr_t) );
        if(sum)
        {
            goto DISCARD;
        }

        /************************************************************
        * Process incoming ICMPv6 packets.
        *************************************************************/
        switch (hdr->type)
        {
            /**************************
             * Neighbor Solicitation.
             **************************/
            case FNET_ICMP6_TYPE_NEIGHBOR_SOLICITATION:
                _fnet_nd6_neighbor_solicitation_receive(netif, src_ip_rx, dest_ip_rx, nb, ip6_nb);
                break;
            /**************************
             * Neighbor Advertisemnt.
             **************************/
            case FNET_ICMP6_TYPE_NEIGHBOR_ADVERTISEMENT:
                _fnet_nd6_neighbor_advertisement_receive(netif, src_ip_rx, dest_ip_rx, nb, ip6_nb);
                break;
            /**************************
             * Router Advertisemnt.
             **************************/
            case FNET_ICMP6_TYPE_ROUTER_ADVERTISEMENT:
                _fnet_nd6_router_advertisement_receive(netif, src_ip_rx, dest_ip_rx, nb, ip6_nb);
                break;
            /**************************
             * Router Advertisemnt.
             **************************/
            case FNET_ICMP6_TYPE_REDIRECT:
                _fnet_nd6_redirect_receive(netif, src_ip_rx, dest_ip_rx, nb, ip6_nb);
                break;
#if FNET_CFG_MLD
            /**************************
             * Multicast Listener Query.
             **************************/
            case FNET_ICMP6_TYPE_MULTICAST_LISTENER_QUERY:
                _fnet_mld_query_receive(netif, src_ip_rx, dest_ip_rx, nb, ip6_nb);
                break;
#endif
            /**************************
             * Echo Request.
             * RFC4443 4.1: Every node MUST implement an ICMPv6 Echo responder function that
             * receives Echo Requests and originates corresponding Echo Replies.
             **************************/
            case FNET_ICMP6_TYPE_ECHO_REQ:
                hdr->type = FNET_ICMP6_TYPE_ECHO_REPLY;

                /* RFC4443: the source address of the reply MUST be a unicast
                 * address belonging to the interface on which
                 * the Echo Request message was received.*/
                if(FNET_IP6_ADDR_IS_MULTICAST(dest_ip_rx))
                {
                    src_ip_tx = FNET_NULL;
                }

                /* Swap source and destination addresses.*/
                _fnet_icmp6_output(netif, src_ip_tx, dest_ip_tx, 0u, nb);
                _fnet_netbuf_free_chain(ip6_nb);
                break;
#if FNET_CFG_IP6_PMTU_DISCOVERY
            /**************************
             * Packet Too Big Message.
             **************************/
            case FNET_ICMP6_TYPE_PACKET_TOOBIG:
                if(netif->pmtu) /* If PMTU is enabled for the interface.*/
                {
                    fnet_uint32_t           pmtu;
                    fnet_icmp6_err_header_t *icmp6_err = (fnet_icmp6_err_header_t *)nb->data_ptr;

                    /* The header must reside in contiguous area of memory. */
                    if(_fnet_netbuf_pullup(&nb, sizeof(fnet_icmp6_err_header_t)) == FNET_ERR)
                    {
                        goto DISCARD;
                    }

                    /* RFC 1981.Upon receipt of such a
                     * message, the source node reduces its assumed PMTU for the path based
                     * on the MTU of the constricting hop as reported in the Packet Too Big
                     * message.*/
                    pmtu = fnet_ntohl(icmp6_err->data);

                    /* A node MUST NOT increase its estimate of the Path MTU in response to
                     * the contents of a Packet Too Big message. */
                    if(netif->pmtu > pmtu)
                    {
                        _fnet_netif_set_pmtu(netif, pmtu);
                    }
                }

                discard_flag = FNET_TRUE;
                break;
#endif
            /**************************
             * Destination Unreachable.
             **************************/
            case FNET_ICMP6_TYPE_DEST_UNREACH:
                switch(hdr->code)
                {
                    case FNET_ICMP6_CODE_DU_NO_ROUTE:           /* No route to destination. */
                    case FNET_ICMP6_CODE_DU_BEYOND_SCOPE:       /* Beyond scope of source address.*/
                        prot_cmd = FNET_PROT_NOTIFY_UNREACH_NET;
                        break;
                    case FNET_ICMP6_CODE_DU_ADMIN_PROHIBITED:   /* Communication with destination administratively prohibited. */
                    case FNET_ICMP6_CODE_DU_ADDR_UNREACH:       /* Address unreachable.*/
                        prot_cmd = FNET_PROT_NOTIFY_UNREACH_HOST;
                        break;
                    case FNET_ICMP6_CODE_DU_PORT_UNREACH:       /* Port unreachable.*/
                        prot_cmd = FNET_PROT_NOTIFY_UNREACH_PORT;
                        break;
                    default:
                        goto DISCARD;
                }

                /* Protocol notification.*/
                {
                    fnet_ip6_header_t       *ip_header;
                    fnet_prot_if_t          *protocol;
                    fnet_size_t            hdr_err_length = sizeof(fnet_icmp6_err_header_t) + sizeof(fnet_ip6_header_t);
                    fnet_size_t            hdr_err_data_length = hdr_err_length + 8u; /* 8 bytes is enough for transport protocol (port numbers).*/


                    if(nb->total_length < hdr_err_data_length)
                    {
                        goto DISCARD;
                    }
                    if(nb->total_length > hdr_err_data_length)
                    {
                        _fnet_netbuf_trim(&nb, (fnet_int32_t)(hdr_err_data_length - nb->total_length));
                    }
                    if(_fnet_netbuf_pullup(&nb, nb->total_length) == FNET_ERR) /* The header must reside in contiguous area of memory.*/
                    {
                        goto DISCARD;
                    }

                    ip_header = (fnet_ip6_header_t *)((fnet_uint8_t *)nb->data_ptr  + sizeof(fnet_icmp6_err_header_t));

                    if((protocol = _fnet_prot_find(AF_INET6, SOCK_UNSPEC, (fnet_uint32_t)ip_header->next_header)) != 0)
                    {
                        if(protocol->prot_control_input)
                        {
                            struct fnet_sockaddr     err_src_addr;
                            struct fnet_sockaddr     err_dest_addr;

                            /* Prepare addreses for upper protocol.*/
                            _fnet_ip6_set_socket_addr(netif, ip_header, &err_src_addr, &err_dest_addr );

                            _fnet_netbuf_trim(&nb, (fnet_int32_t)(hdr_err_length)); /* Cut the ICMP error header.*/

                            protocol->prot_control_input(prot_cmd, &err_src_addr, &err_dest_addr, nb);
                        }
                    }
                }
                discard_flag = FNET_TRUE;
                break;
            default:
                discard_flag = FNET_TRUE;
                break;
        }
    }
    else
    {
        discard_flag = FNET_TRUE;
    }

    if(discard_flag == FNET_TRUE)
    {
    DISCARD:
        _fnet_netbuf_free_chain(ip6_nb);
        _fnet_netbuf_free_chain(nb);
    }
}

/************************************************************************
* DESCRIPTION: ICMPv6 output function.
*************************************************************************/
void _fnet_icmp6_output( struct fnet_netif *netif, const fnet_ip6_addr_t *src_ip, const fnet_ip6_addr_t *dest_ip, fnet_uint8_t hop_limit, fnet_netbuf_t *nb )
{
    fnet_icmp6_header_t                     *hdr = (fnet_icmp6_header_t *)nb->data_ptr;
    FNET_COMP_PACKED_VAR fnet_uint16_t      *checksum_p;

    /* Checksum calculation.*/
    hdr->checksum = 0u;

    hdr->checksum = _fnet_checksum_pseudo_netbuf_start(nb, FNET_HTONS((fnet_uint16_t)FNET_PROT_ICMP6), (fnet_uint16_t)nb->total_length);
    checksum_p = &hdr->checksum;

    _fnet_ip6_output(netif, src_ip, dest_ip, FNET_PROT_ICMP6, hop_limit, nb, checksum_p);
}

/************************************************************************
* DESCRIPTION: Sends ICMPv6 error message.
*************************************************************************/
void _fnet_icmp6_error( struct fnet_netif *netif, fnet_uint8_t type, fnet_uint8_t code, fnet_uint32_t param, fnet_netbuf_t *origin_nb )
{
    fnet_ip6_header_t       *ip6_header;
    fnet_icmp6_err_header_t *icmp6_err_header;
    const fnet_ip6_addr_t   *src_ip_rx;
    const fnet_ip6_addr_t   *dest_ip_rx;
    const fnet_ip6_addr_t   *src_ip_tx;
    const fnet_ip6_addr_t   *dest_ip_tx;
    fnet_netbuf_t           *nb_header;

    if(origin_nb)
    {
        /* Limit to FNET_IP6_DEFAULT_MTU. */
        if(origin_nb->total_length > (FNET_IP6_DEFAULT_MTU - (sizeof(fnet_icmp6_err_header_t) + sizeof(fnet_ip6_header_t))  ))
        {
            _fnet_netbuf_trim(&origin_nb, (fnet_int32_t)(FNET_IP6_DEFAULT_MTU - (sizeof(fnet_icmp6_err_header_t) + sizeof(fnet_ip6_header_t)) - origin_nb->total_length));
        }

        ip6_header = (fnet_ip6_header_t *)origin_nb->data_ptr;

        src_ip_rx = &ip6_header->source_addr;
        dest_ip_rx = &ip6_header->destination_addr;

        /* Swap source and destination addresses.*/
        src_ip_tx = dest_ip_rx;
        dest_ip_tx = src_ip_rx;

        /*******************************************************************
         * RFC 4443:
         * (e) An ICMPv6 error message MUST NOT be originated as a result of
         * receiving the following:
         *******************************************************************/
        /* (e.1) An ICMPv6 error message. */
        /* (e.2) An ICMPv6 REDIRECT message [IPv6-DISC].*/
        if (ip6_header->next_header == FNET_PROT_ICMP6) /* TBD Extension header case.*/
        {
            /* Make sure the packet has at least a 'TYPE' field */
            if (ip6_header->length == 0u)
            {
                goto FREE_NB;
            }

            icmp6_err_header = (fnet_icmp6_err_header_t *)((fnet_uint8_t *)ip6_header + sizeof(fnet_ip6_header_t));
            if (FNET_ICMP6_TYPE_IS_ERROR(icmp6_err_header->icmp6_header.type) || (icmp6_err_header->icmp6_header.type == FNET_ICMP6_TYPE_REDIRECT ) )
            {
                goto FREE_NB;
            }
        }

        /*
         * (e.3) A packet destined to an IPv6 multicast address. (There are
         * two exceptions to this rule: (1) the Packet Too Big Message
         * (Section 3.2) to allow Path MTU discovery to work for IPv6
         * multicast, and (2) the Parameter Problem Message, Code 2
         * (Section 3.4) reporting an unrecognized IPv6 option (see
         * Section 4.2 of [IPv6]) that has the Option Type highestorder
         * two bits set to 10).
         * (e.4) A packet sent as a link-layer multicast (the exceptions
         * from e.3 apply to this case, too).
         */
        if(FNET_IP6_ADDR_IS_MULTICAST(dest_ip_rx)
           && (!( (type == FNET_ICMP6_TYPE_PACKET_TOOBIG)
                  || ((type == FNET_ICMP6_TYPE_PARAM_PROB) && (code == FNET_ICMP6_CODE_PP_OPTION)))) )
        {
            goto FREE_NB;
        }
        else
        {
            if(FNET_IP6_ADDR_IS_MULTICAST(dest_ip_rx))
            {
                /* We may not use multicast address as source. Get real source address. */
                src_ip_tx = _fnet_ip6_select_src_addr(netif, src_ip_rx /*dest*/);
            }
        }

        /*
         * (e.5) A packet sent as a link-layer broadcast (the exceptions
         *  from e.3 apply to this case, too). TBD
         */


        /*
         * (e.6) A packet whose source address does not uniquely identify a
         * single node -- e.g., the IPv6 Unspecified Address, an IPv6
         * multicast address, or an address known by the ICMP message
         * originator to be an IPv6 anycast address.
         */
        if(FNET_IP6_ADDR_IS_MULTICAST(src_ip_rx) || FNET_IP6_ADDR_EQUAL(&fnet_ip6_addr_any, src_ip_rx))
        {
            goto FREE_NB;
        }

        /* Construct ICMPv6 error header.*/
        if((nb_header = _fnet_netbuf_new((sizeof(fnet_icmp6_err_header_t)), FNET_FALSE)) == 0)
        {
            goto FREE_NB;
        }

        icmp6_err_header = (fnet_icmp6_err_header_t *)nb_header->data_ptr;

        icmp6_err_header->icmp6_header.type = type;
        icmp6_err_header->icmp6_header.code = code;
        icmp6_err_header->data = fnet_htonl(param);

        origin_nb = _fnet_netbuf_concat(nb_header, origin_nb);

        /* Swap source and destination addresses.*/
        _fnet_icmp6_output( netif, src_ip_tx, dest_ip_tx, 0u, origin_nb);

        return;

    FREE_NB:
        _fnet_netbuf_free_chain(origin_nb);
    }

}
#endif /* FNET_CFG_IP6 */
