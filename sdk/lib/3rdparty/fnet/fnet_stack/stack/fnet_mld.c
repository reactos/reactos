/**************************************************************************
*
* Copyright 2011-2018 by Andrey Butok. FNET Community.
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
*  MLD protocol implementation.
*
***************************************************************************/

#include "fnet.h"

#if FNET_CFG_MLD & FNET_CFG_IP6

#include "fnet_mld.h"
#include "fnet_checksum_prv.h"
#include "fnet_prot.h"

/* TBD Random delay timers */

/************************************************************************
*     Definitions
*************************************************************************/
#define FNET_MLD_HOP_LIMIT                   (1)             /* IPv6 Hop Limit for MLD is 1.*/

/* IPv6 Router Alert option in a Hop-by_Hop Options header.*/
static const fnet_mld_ra_option_header_t mld_ra_option = {  .next_header = FNET_PROT_ICMP6,         /* Next Header */
                                                            .hdr_ext_length = 0,                     /* Length-8 */
                                                            .ra_option_header = {.type = FNET_IP6_OPTION_TYPE_ROUTER_ALERT, .data_length = 2},  /* Router Alert Option. */
                                                            .ra_option_value = FNET_IP6_OPTION_TYPE_ROUTER_ALERT_VALUE_MLD,                     /* Router Alert Option value. */
                                                            .padn_option_header = {.type = FNET_IP6_OPTION_TYPE_PADN, .data_length = 0}         /* Padding. */
                                                         };

static void _fnet_mld_send( fnet_netif_t *netif, fnet_ip6_addr_t *group_addr, fnet_uint8_t type);

/************************************************************************
* DESCRIPTION: Sends Host Membership Reports.
*************************************************************************/
void _fnet_mld_join(fnet_netif_t *netif, fnet_ip6_addr_t *group_addr)
{
    _fnet_mld_send(netif, group_addr, FNET_ICMP6_TYPE_MULTICAST_LISTENER_REPORT);
}

/************************************************************************
* DESCRIPTION: Sends a Leave Group message.
*************************************************************************/
void _fnet_mld_leave(fnet_netif_t *netif, fnet_ip6_addr_t  *group_addr)
{
    _fnet_mld_send(netif, group_addr, FNET_ICMP6_TYPE_MULTICAST_LISTENER_DONE);
}

/************************************************************************
* DESCRIPTION: Generates new MLD Report messages for all multicast
*              addresses joined on the interface.
*************************************************************************/
void _fnet_mld_report_all(fnet_netif_t *netif)
{
    fnet_index_t i;

    /* Find existing entries for the interface.*/
    for(i = 0; i < FNET_CFG_MULTICAST_MAX; i++)
    {
        if((fnet_ip6_multicast_list[i].user_counter > 0u)
           && (fnet_ip6_multicast_list[i].netif == netif))
        {
            /* Send report.*/
            _fnet_mld_send(netif, &fnet_ip6_multicast_list[i].group_addr, FNET_ICMP6_TYPE_MULTICAST_LISTENER_REPORT);
        }
    }
}

/************************************************************************
* DESCRIPTION: Sends MLD message defined by type:
*        FNET_ICMP6_TYPE_MULTICAST_LISTENER_REPORT or FNET_ICMP6_TYPE_MULTICAST_LISTENER_DONE
*************************************************************************/
static void _fnet_mld_send(fnet_netif_t *netif, fnet_ip6_addr_t *group_addr, fnet_uint8_t type)
{
    fnet_netbuf_t                       *nb;
    fnet_netbuf_t                       *nb_option;
    fnet_mld_header_t                   *mld_header;
    fnet_mld_ra_option_header_t         *ra_option_header;
    FNET_COMP_PACKED_VAR fnet_uint16_t  *checksum_p;
    fnet_ip6_addr_t                     *ip_src;
    fnet_ip6_addr_t                     *ip_dst;

    /* [RFC2710] EXCLUDING the link-scope all-nodes address and any multicast
     * addresses of scope 0 (reserved) or 1(node-local).*/
    if((FNET_IP6_ADDR_MULTICAST_SCOPE(group_addr) > FNET_IP6_ADDR_SCOPE_INTERFACELOCAL)
       && !FNET_IP6_ADDR_EQUAL(&fnet_ip6_addr_linklocal_allnodes, group_addr))
    {
        /* Construct Router Alert option + MLD meassage */
        if((nb = _fnet_netbuf_new(sizeof(fnet_mld_header_t), FNET_FALSE)) != 0)
        {
            if((nb_option = _fnet_netbuf_new(sizeof(fnet_mld_ra_option_header_t), FNET_FALSE)) != 0)
            {
                /* Fill Hop-by_Hop Options header.*/
                ra_option_header = (fnet_mld_ra_option_header_t *)(nb_option->data_ptr);
                fnet_memcpy ((void *)ra_option_header, (void *)(&mld_ra_option), sizeof(mld_ra_option));

                /* Fill MLD message. */
                mld_header = (fnet_mld_header_t *)(nb->data_ptr);
                fnet_memset_zero(mld_header, sizeof(fnet_mld_header_t));
                mld_header->icmp6_header.type = type;
                FNET_IP6_ADDR_COPY(group_addr, &mld_header->multicast_addr);

                /* Checksum calculation.*/
                mld_header->icmp6_header.checksum = 0;
                mld_header->icmp6_header.checksum = _fnet_checksum_pseudo_netbuf_start(nb, FNET_HTONS((fnet_uint16_t)FNET_PROT_ICMP6), (fnet_uint16_t)nb->total_length);
                checksum_p = &mld_header->icmp6_header.checksum;

                /* Concatanate Hop-by_Hop Options with MLD header. */
                nb = _fnet_netbuf_concat(nb_option, nb);

                /* Source Address Selection for MLD, by RFC3590.*/

                /* [RFC3590] MLD Report and Done messages are sent with a link-local address as
                 * the IPv6 source address, if a valid address is available on the interface.*/
                ip_src = _fnet_netif_get_ip6_addr_valid_link_local(netif);

                /* [RFC3590] If a valid link-local address is not available (e.g., one has not been configured),
                 * the message is sent with the unspecified address. */
                if(ip_src == FNET_NULL)
                {
                    ip_src = (fnet_ip6_addr_t *)&fnet_ip6_addr_any;
                    netif->mld_invalid = FNET_TRUE;
                }
                else
                {
                    netif->mld_invalid = FNET_FALSE;
                }

                /* When a node ceases to listen to a multicast address on an interface,
                 * it SHOULD send a single Done message to the link-scope all-routers
                 * multicast address (FF02::2)*/
                if(type == FNET_ICMP6_TYPE_MULTICAST_LISTENER_DONE)
                {
                    ip_dst = (fnet_ip6_addr_t *)&fnet_ip6_addr_linklocal_allrouters;
                }
                else
                {
                    ip_dst = group_addr;
                }

                /* Send via IPv6*/
                _fnet_ip6_output(netif, ip_src, ip_dst, FNET_IP6_TYPE_HOP_BY_HOP_OPTIONS, FNET_MLD_HOP_LIMIT, nb, checksum_p);
            }
            else
            {
                _fnet_netbuf_free_chain(nb);
            }
        }
    }
}

/************************************************************************
* DESCRIPTION: Handles received Multicast Listener Query message.
*************************************************************************/
void _fnet_mld_query_receive(fnet_netif_t *netif, fnet_ip6_addr_t *src_ip, fnet_ip6_addr_t *dest_ip, fnet_netbuf_t *nb, fnet_netbuf_t *ip6_nb)
{
    fnet_mld_header_t               *mld_packet = nb->data_ptr;
    fnet_size_t                     mld_packet_size = nb->total_length;
    fnet_ip6_header_t               *ip6_packet = (fnet_ip6_header_t *)ip6_nb->data_ptr;

    FNET_COMP_UNUSED_ARG(dest_ip);

    /************************************************************
    * Validation of MLD Query Message.
    *************************************************************/
    if(
        (mld_packet_size < sizeof(fnet_mld_header_t))
        /* Validation RFC2710.*/
        || (ip6_packet->hop_limit != FNET_MLD_HOP_LIMIT)    /* The IP Hop Limit field has a value of 1.*/
        || !FNET_IP6_ADDR_IS_LINKLOCAL(src_ip)              /* MUST be the link-local address.*/
    )
    {
        goto DROP;
    }

    /* [RFC2710] The Multicast Address field in the MLD
     * message must contain either zero (a General Query) or a valid
     * multicast address (a Multicast- Address-Specific Query).*/
    if(FNET_IP6_ADDR_IS_MULTICAST(&mld_packet->multicast_addr))
    {
        /* [RFC2710] A Multicast-Address-Specific
         * Query applies to a single multicast address on the interface from
         * which the Query is received. */
        if(_fnet_ip6_multicast_find_entry(netif, &mld_packet->multicast_addr))
            _fnet_mld_join(netif, &mld_packet->multicast_addr);
    }
    else if(FNET_IP6_ADDR_EQUAL(&fnet_ip6_addr_any, &mld_packet->multicast_addr))
    {
        /* [RFC2710] General Query applies to all multicast addresses on the interface
         * from which the Query is received. */
        _fnet_mld_report_all(netif);
    }
    else
    {}

DROP:
    _fnet_netbuf_free_chain(ip6_nb);
    _fnet_netbuf_free_chain(nb);
}

#endif /* FNET_CFG_MLD */
