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
*  IGMPv1/v2 protocol implementation.
*
***************************************************************************/

#include "fnet.h"

#if FNET_CFG_IGMP & FNET_CFG_IP4

#include "fnet_igmp.h"
#include "fnet_timer.h"
#include "fnet_prot.h"
#include "fnet_checksum_prv.h"

/* IGMP requires multicast support.*/
#if FNET_CFG_MULTICAST == 0
    #error "FNET_CFG_MULTICAST must be enabled for IGMP"
#endif

#if (FNET_CFG_IGMP_VERSION < 1) || (FNET_CFG_IGMP_VERSION > 2)
    #error "FNET_CFG_IGMP_VERSION must be set to 1 or to 2"
#endif

/* TBD Random delay timers */

/************************************************************************
*     Definitions
*************************************************************************/
#define FNET_IGMP_TTL                   (1)             /* IP time-to-live is 1.*/
#define FNET_IGMP_TOS                   (FNET_IP4_TOS_NORMAL) /* The TOS of IGMP messages.*/

/************************************************************************
*     Function Prototypes
*************************************************************************/
static void _fnet_igmp_input(fnet_netif_t *netif, struct fnet_sockaddr *src_addr,  struct fnet_sockaddr *dest_addr, fnet_netbuf_t *nb, fnet_netbuf_t *ip4_nb);

#if FNET_CFG_DEBUG_TRACE_IGMP && FNET_CFG_DEBUG_TRACE
    static void _fnet_igmp_trace(const fnet_char_t *str, fnet_igmp_header_t *icmpp_hdr);
#else
    #define _fnet_igmp_trace(str, icmp_hdr)  do {}while(0)
#endif

/************************************************************************
* Protocol API structure.
************************************************************************/
fnet_prot_if_t fnet_igmp_prot_if =
{
    .family = AF_INET,              /* Address domain family.*/
    .type = SOCK_UNSPEC,            /* Socket type used for.*/
    .protocol = FNET_PROT_IGMP,     /* Protocol number.*/
    .prot_input = _fnet_igmp_input,  /* Protocol input function.*/
};

/************************************************************************
* DESCRIPTION: IGMP input function.
*************************************************************************/
static void _fnet_igmp_input(fnet_netif_t *netif, struct fnet_sockaddr *src_addr,  struct fnet_sockaddr *dest_addr, fnet_netbuf_t *nb, fnet_netbuf_t *ip4_nb)
{
    fnet_igmp_header_t  *hdr;
    fnet_index_t        i;

    FNET_COMP_UNUSED_ARG(src_addr);
    FNET_COMP_UNUSED_ARG(dest_addr);

    _fnet_netbuf_free_chain(ip4_nb); /* Not used*/

    if((netif != 0) && (nb != 0) )
    {
        /* The header must reside in contiguous area of memory. */
        if(_fnet_netbuf_pullup(&nb, sizeof(fnet_igmp_header_t)) == FNET_ERR)
        {
            goto DISCARD;
        }

        hdr = nb->data_ptr;

        /* RFC2236 To be valid, the Query message
         * must be at least 8 octets long, have a correct IGMP
         * checksum.
         */
        if(_fnet_checksum_netbuf(nb, nb->total_length)  )
        {
            goto DISCARD;
        }

        _fnet_igmp_trace("RX", hdr);

        /**************************
        * IGMP QUERY Processing
        **************************/
        if(hdr->type == IGMP_HEADER_TYPE_QUERY)
        {
            /* RFC2236: The group address in the IGMP header must either be zero (a General
             * Query) or a valid multicast group address (a Group-Specific Query).
             * A General Query applies to all memberships on the interface from
             * which the Query is received. A Group-Specific Query applies to
             * membership in a single group on the interface from which the Query
             * is received. Queries are ignored for memberships in the Non-Member
             * state.
             */
            if(hdr->group_addr == 0)
                /* General Query */
            {
                /* Find all joined-groups for this interface.*/
                for(i = 0u; i < FNET_CFG_MULTICAST_MAX; i++)
                {
                    if((fnet_ip4_multicast_list[i].user_counter > 0) && (fnet_ip4_multicast_list[i].netif == netif))
                    {
                        /* Send report.*/
                        _fnet_igmp_join(netif, fnet_ip4_multicast_list[i].group_addr );
                    }
                }
            }
#if FNET_CFG_IGMP_VERSION == 2
            else if(FNET_IP4_ADDR_IS_MULTICAST(hdr->group_addr))
                /* A Group-Specific Query.*/
            {
                /* Find specific group.*/
                for(i = 0u; i < FNET_CFG_MULTICAST_MAX; i++)
                {
                    if((fnet_ip4_multicast_list[i].user_counter > 0) && (fnet_ip4_multicast_list[i].netif == netif) && (fnet_ip4_multicast_list[i].group_addr == hdr->group_addr))
                    {
                        /* Send report.*/
                        _fnet_igmp_join(netif, fnet_ip4_multicast_list[i].group_addr );
                        break;
                    }
                }
            }
#endif /* FNET_CFG_IGMP_VERSION */
            else
            {} /* wrong */
        }
        /************************
         * Ignore others
         ************************/
    }

DISCARD:
    _fnet_netbuf_free_chain(nb);
}

/************************************************************************
* DESCRIPTION: Sends Host Membership Reports.
*************************************************************************/
void _fnet_igmp_join( fnet_netif_t *netif, fnet_ip4_addr_t  group_addr )
{
    fnet_netbuf_t *nb_header;
    fnet_igmp_header_t *igmp_header;

    /* Construct IGMP header*/
    if((nb_header = _fnet_netbuf_new(sizeof(fnet_igmp_header_t), FNET_FALSE)) != 0)
    {
        igmp_header = nb_header->data_ptr;
        /* Type.*/
#if FNET_CFG_IGMP_VERSION == 1
        igmp_header->type = IGMP_HEADER_TYPE_REPORT_V1;
#else /* FNET_CFG_IGMP_VERSION == 2 */
        igmp_header->type = IGMP_HEADER_TYPE_REPORT_V2;
#endif
        igmp_header->max_resp_time = 0;             /* Unused field, zeroed when sent, ignored when received.*/
        igmp_header->group_addr = group_addr ;      /* Group address field.*/


        igmp_header->checksum = 0;
        igmp_header->checksum = _fnet_checksum_netbuf(nb_header, nb_header->total_length);

        /* RFC 1112: A Report is sent with an IP destination address equal to the
         * host group address being reported, and with an IP time-to-live of 1.
         */
        _fnet_ip4_output(netif, INADDR_ANY, group_addr /*dest_addr*/, FNET_PROT_IGMP, FNET_IGMP_TOS, FNET_IGMP_TTL, nb_header, FNET_FALSE, FNET_FALSE, 0);
    }
}


/************************************************************************
* DESCRIPTION: Sends a Leave Group message.
*************************************************************************/
void _fnet_igmp_leave( fnet_netif_t *netif, fnet_ip4_addr_t  group_addr )
{
#if FNET_CFG_IGMP_VERSION == 2
    fnet_netbuf_t *nb_header;
    fnet_igmp_header_t *igmp_header;
    fnet_ip4_addr_t dest_ip = FNET_IP4_ADDR_INIT(224, 0, 0, 2); /* All-routers multicast group.*/

    /* Construct IGMP header*/
    if((nb_header = _fnet_netbuf_new(sizeof(fnet_igmp_header_t), FNET_FALSE)) != 0)
    {
        /*
         * When a host leaves a multicast group, if it was the last host to
         * reply to a Query with a Membership Report for that group, it SHOULD
         * send a Leave Group message to the all-routers multicast group (224.0.0.2).
         */

        igmp_header = nb_header->data_ptr;

        igmp_header->type = IGMP_HEADER_TYPE_LEAVE_GROUP; /* Type.*/
        igmp_header->max_resp_time = 0;             /* Unused field, zeroed when sent, ignored when received.*/
        igmp_header->group_addr = group_addr ;      /* Group address field.*/


        igmp_header->checksum = 0;
        igmp_header->checksum = _fnet_checksum_netbuf(nb_header, nb_header->total_length);

        /* RFC 1112: A Report is sent with an IP destination address equal to the
         * host group address being reported, and with an IP time-to-live of 1.*/

        _fnet_ip4_output(netif, INADDR_ANY, dest_ip /*dest_addr*/, FNET_PROT_IGMP, FNET_IGMP_TOS, FNET_IGMP_TTL, nb_header, FNET_FALSE, FNET_FALSE, 0);
    }
#endif /* FNET_CFG_IGMP_VERSION */
}

/************************************************************************
* DESCRIPTION: Prints an IGMP header. For debug needs only.
*************************************************************************/
#if FNET_CFG_DEBUG_TRACE_IGMP && FNET_CFG_DEBUG_TRACE
static void _fnet_igmp_trace(const fnet_char_t *str, fnet_igmp_header_t *igmp_hdr)
{
    fnet_char_t ip_str[FNET_IP4_ADDR_STR_SIZE];

    fnet_printf(FNET_SERIAL_ESC_FG_GREEN"%s", str); /* Print app-specific header.*/
    fnet_println("[IGMP header]"FNET_SERIAL_ESC_ATTR_RESET);
    fnet_println("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+");
    fnet_println("|(Type)    "FNET_SERIAL_ESC_FG_BLUE"0x%2x"FNET_SERIAL_ESC_ATTR_RESET" |(Res Time) %3u |(Cheksum)               0x%04x |",
                 igmp_hdr->type,
                 igmp_hdr->max_resp_time,
                 fnet_ntohs(igmp_hdr->checksum));
    fnet_println("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+");
    fnet_println("|(Group Addr)                                   "FNET_SERIAL_ESC_FG_BLUE"%15s"FNET_SERIAL_ESC_ATTR_RESET" |",
                 fnet_inet_ntoa(*(struct fnet_in_addr *)(&igmp_hdr->group_addr), ip_str));
    fnet_println("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+");

}
#endif /* FNET_CFG_DEBUG_TRACE_IGMP */

#endif /* FNET_CFG_IGMP */
