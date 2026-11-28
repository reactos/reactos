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
*  IPv6 protocol implementation.
*
***************************************************************************/

#include "fnet.h"

#if FNET_CFG_IP6

#include "fnet_ip6_prv.h"
#include "fnet_ip_prv.h"
#include "fnet_icmp4.h"
#include "fnet_checksum_prv.h"
#include "fnet_timer_prv.h"
#include "fnet_socket_prv.h"
#include "fnet_netif_prv.h"
#include "fnet_prot.h"
#include "fnet_loop.h"
#include "fnet_prot.h"
#include "fnet_raw.h"
#include "fnet_mld.h"
#include "fnet_stack_prv.h"

#if FNET_CFG_MULTICAST == 0 /* Multicast must be enabled for IPv6. */
    #error  "FNET_CFG_MULTICAST must be enabled for IPv6."
#endif

/* Check max. values.*/
#if (FNET_IP6_MAX_PACKET > 65535U)
    #error  "FNET_IP6_MAX_PACKET may not be more than 65535."
#endif

/******************************************************************
* Ext. header handler results.
*******************************************************************/
typedef enum
{
    FNET_IP6_EXT_HEADER_HANDLER_RESULT_EXIT,      /* Stop processing.*/
    FNET_IP6_EXT_HEADER_HANDLER_RESULT_NEXT,      /* Go to the next Ext. Header processing.*/
    FNET_IP6_EXT_HEADER_HANDLER_RESULT_TRANSPORT  /* Go to Transport Layer processing.*/
} fnet_ip6_ext_header_handler_result_t;

/******************************************************************
* Extension Header Handler structure
*******************************************************************/
typedef struct fnet_ip6_ext_header
{
    fnet_uint32_t type;                                                     /* Identifies the type of header. */
    fnet_ip6_ext_header_handler_result_t (*handler)(fnet_netif_t *netif, fnet_uint8_t **next_header_p, fnet_ip6_addr_t *src_ip, fnet_ip6_addr_t *dest_ip, fnet_netbuf_t **nb_p, fnet_netbuf_t *ip6_nb); /* Extension header handler.*/

} fnet_ip6_ext_header_t;

/******************************************************************
* Function Prototypes
*******************************************************************/
static void _fnet_ip6_netif_output(struct fnet_netif *netif, const fnet_ip6_addr_t *src_ip, const fnet_ip6_addr_t *dest_ip, fnet_netbuf_t *nb);
static fnet_return_t _fnet_ip6_ext_header_process(fnet_netif_t *netif, fnet_uint8_t **next_header_p, fnet_ip6_addr_t *src_ip, fnet_ip6_addr_t *dest_ip, fnet_netbuf_t **nb_p, fnet_netbuf_t *ip6_nb);
static fnet_ip6_ext_header_handler_result_t _fnet_ip6_ext_header_handler_fragment_header(fnet_netif_t *netif, fnet_uint8_t **next_header_p, fnet_ip6_addr_t *src_ip, fnet_ip6_addr_t *dest_ip, fnet_netbuf_t **nb_p, fnet_netbuf_t *ip6_nb);
static fnet_ip6_ext_header_handler_result_t _fnet_ip6_ext_header_handler_routing_header(fnet_netif_t *netif, fnet_uint8_t **next_header_p, fnet_ip6_addr_t *src_ip, fnet_ip6_addr_t *dest_ip, fnet_netbuf_t **nb_p, fnet_netbuf_t *ip6_nb);
static fnet_ip6_ext_header_handler_result_t _fnet_ip6_ext_header_handler_options (fnet_netif_t *netif, fnet_uint8_t **next_header_p, fnet_ip6_addr_t *src_ip, fnet_ip6_addr_t *dest_ip, fnet_netbuf_t **nb_p, fnet_netbuf_t *ip6_nb);
static fnet_ip6_ext_header_handler_result_t _fnet_ip6_ext_header_handler_no_next_header (fnet_netif_t *netif, fnet_uint8_t **next_header_p, fnet_ip6_addr_t *src_ip, fnet_ip6_addr_t *dest_ip, fnet_netbuf_t **nb_p, fnet_netbuf_t *ip6_nb);
static void _fnet_ip6_input_low(void *cookie);
static fnet_uint32_t _fnet_ip6_policy_label( const fnet_ip6_addr_t *addr );

#if FNET_CFG_IP6_FRAGMENTATION
    static void _fnet_ip6_frag_list_add( fnet_ip6_frag_list_t **head, fnet_ip6_frag_list_t *fl );
    static void _fnet_ip6_frag_list_del( fnet_ip6_frag_list_t **head, fnet_ip6_frag_list_t *fl );
    static void _fnet_ip6_frag_add( fnet_ip6_frag_header_t **head, fnet_ip6_frag_header_t *frag,  fnet_ip6_frag_header_t *frag_prev );
    static void _fnet_ip6_frag_del( fnet_ip6_frag_header_t **head, fnet_ip6_frag_header_t *frag );
    static void _fnet_ip6_frag_list_free( fnet_ip6_frag_list_t *list );
    static fnet_netbuf_t *_fnet_ip6_reassembly(fnet_netif_t *netif, fnet_netbuf_t **nb_p, fnet_netbuf_t *ip6_nb, fnet_ip6_addr_t *src_ip, fnet_ip6_addr_t *dest_ip );
    static void _fnet_ip6_timer(fnet_uint32_t cookie);
#endif

/******************************************************************
* Extension Header Handler List
*******************************************************************/
static fnet_ip6_ext_header_t fnet_ip6_ext_header_list[] =
{
    {FNET_IP6_TYPE_NO_NEXT_HEADER,          _fnet_ip6_ext_header_handler_no_next_header}
    , {FNET_IP6_TYPE_HOP_BY_HOP_OPTIONS,     _fnet_ip6_ext_header_handler_options}
    , {FNET_IP6_TYPE_DESTINATION_OPTIONS,    _fnet_ip6_ext_header_handler_options}
    , {FNET_IP6_TYPE_ROUTING_HEADER,         _fnet_ip6_ext_header_handler_routing_header}
    , {FNET_IP6_TYPE_FRAGMENT_HEADER,        _fnet_ip6_ext_header_handler_fragment_header}
    /* ADD YOUR EXTENSION HEADER HANDLERS HERE.*/
};

#define FNET_IP6_EXT_HEADER_LIST_SIZE    (sizeof(fnet_ip6_ext_header_list)/sizeof(fnet_ip6_ext_header_t))

/************************************************************************
*     Policy Table (RFC3484)
*************************************************************************/
typedef struct fnet_ip6_if_policy_entry
{
    fnet_ip6_addr_t prefix;             /* Prefix of an IP address. */
    fnet_size_t     prefix_length;      /* Prefix length (in bits). The number of leading bits
                                         * in the Prefix that are valid. */
    fnet_uint32_t   precedence;         /* Precedence value used for sorting destination addresses.*/
    fnet_uint32_t   label;              /* The label value Label(A) allows for policies that prefer a particular
                                         * source address prefix for use with a destination address prefix.*/
} fnet_ip6_if_policy_entry_t;

/* RFC3484 Default policy table:*/
static const fnet_ip6_if_policy_entry_t fnet_ip6_if_policy_table[] =
{
    {FNET_IP6_ADDR_INIT(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1),          128,    50, 0},
    {FNET_IP6_ADDR_INIT(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),          0,      40, 1},
    {FNET_IP6_ADDR_INIT(0x20, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),    16,     30, 2},
    {FNET_IP6_ADDR_INIT(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0),          96,     20, 3},
    {FNET_IP6_ADDR_INIT(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 0, 0, 0, 0),    96,     10, 4}
};
#define FNET_IP6_IF_POLICY_TABLE_SIZE    (sizeof(fnet_ip6_if_policy_table)/sizeof(fnet_ip6_if_policy_entry_t))


static fnet_ip_queue_t ip6_queue;
static fnet_event_desc_t ip6_event;

#if FNET_CFG_IP6_FRAGMENTATION
    static fnet_ip6_frag_list_t *ip6_frag_list_head;
    static fnet_timer_desc_t ip6_timer_ptr;
#endif

/******************************************************************
* Definitions of some costant IP6 addresses (BSD-like).
*******************************************************************/
const fnet_ip6_addr_t fnet_ip6_addr_any = FNET_IP6_ADDR_ANY_INIT;
const fnet_ip6_addr_t fnet_ip6_addr_loopback = FNET_IP6_ADDR_LOOPBACK_INIT;
const fnet_ip6_addr_t fnet_ip6_addr_nodelocal_allnodes = FNET_IP6_ADDR_NODELOCAL_ALLNODES_INIT;
const fnet_ip6_addr_t fnet_ip6_addr_linklocal_allnodes = FNET_IP6_ADDR_LINKLOCAL_ALLNODES_INIT;
const fnet_ip6_addr_t fnet_ip6_addr_linklocal_allrouters = FNET_IP6_ADDR_LINKLOCAL_ALLROUTERS_INIT;
const fnet_ip6_addr_t fnet_ip6_addr_linklocal_allv2routers = FNET_IP6_ADDR_LINKLOCAL_ALLV2ROUTERS_INIT;
const fnet_ip6_addr_t fnet_ip6_addr_linklocal_prefix = FNET_IP6_ADDR_LINKLOCAL_PREFIX_INIT;

/* IPv6 Multicast list.*/
fnet_ip6_multicast_list_entry_t fnet_ip6_multicast_list[FNET_CFG_MULTICAST_MAX];

/************************************************************************
* DESCRIPTION: This function makes initialization of the IPv6 layer.
*************************************************************************/
fnet_return_t _fnet_ip6_init( void )
{
    fnet_return_t result = FNET_ERR;

#if FNET_CFG_IP6_FRAGMENTATION

    ip6_frag_list_head = 0;

    ip6_timer_ptr = _fnet_timer_new(FNET_IP6_TIMER_PERIOD, _fnet_ip6_timer, 0u);

    if(ip6_timer_ptr)
    {
#endif
        /* Install IPv6 event handler. */
        ip6_event = fnet_event_init(_fnet_ip6_input_low, 0u);

        if(ip6_event)
        {
            result = FNET_OK;
        }

#if FNET_CFG_IP6_FRAGMENTATION
    }
#endif

    /* Clear the multicast list.*/
    fnet_memset_zero( fnet_ip6_multicast_list, sizeof(fnet_ip6_multicast_list));

    return result;
}

/************************************************************************
* DESCRIPTION: This function makes release of the all resources
*              allocated for IPv6 layer module.
*************************************************************************/
void _fnet_ip6_release( void )
{
    _fnet_ip6_drain();
#if FNET_CFG_IP6_FRAGMENTATION
    _fnet_timer_free(ip6_timer_ptr);
    ip6_timer_ptr = 0;
#endif
}

/************************************************************************
* DESCRIPTION: Process IPv6 Extension Headers.
*
* RETURNS: FNET_OK - continue processing by transport layer.
*          FNET_ERR - stop processing.
*************************************************************************/
static fnet_return_t _fnet_ip6_ext_header_process(fnet_netif_t *netif, fnet_uint8_t **next_header_p, fnet_ip6_addr_t *src_ip, fnet_ip6_addr_t *dest_ip, fnet_netbuf_t **nb_p, fnet_netbuf_t *ip6_nb)
{
    fnet_bool_t             try_upper_layer = FNET_FALSE;
    fnet_uint8_t            next_header;
    fnet_index_t            ext_header_counter = 0u;
    fnet_ip6_ext_header_t   *ext_header;

    /* RFC 2460 4:
     * Therefore, extension headers must
     * be processed strictly in the order they appear in the packet; a
     * receiver must not, for example, scan through a packet looking for a
     * particular kind of extension header and process that header prior to
     * processing all preceding ones.
     */

    /* Process headers.*/
    do
    {
        fnet_index_t j;

        next_header = **next_header_p;
        ext_header = FNET_NULL;

        /* IPv6 nodes must accept and attempt to process extension headers in
         * any order and occurring any number of times in the same packet,
         * except for the Hop-by-Hop Options header which is restricted to
         * appear immediately after an IPv6 header only.*/
        /*TBD try to put it to main handler */
        if( (next_header == FNET_IP6_TYPE_HOP_BY_HOP_OPTIONS) && ext_header_counter)
        {
            _fnet_netbuf_free_chain(*nb_p);
            _fnet_icmp6_error( netif, FNET_ICMP6_TYPE_PARAM_PROB, FNET_ICMP6_CODE_PP_NEXT_HEADER,
                               (fnet_uint32_t)(*next_header_p) - (fnet_uint32_t)(ip6_nb->data_ptr), ip6_nb );
            return FNET_ERR;
        }

        for(j = 0u; j < FNET_IP6_EXT_HEADER_LIST_SIZE; j++)
        {
            if(next_header == fnet_ip6_ext_header_list[j].type)
            {
                ext_header = &fnet_ip6_ext_header_list[j];
                break;
            }
        }

        if(ext_header)
        {
            fnet_ip6_ext_header_handler_result_t handler_result = ext_header->handler(netif, next_header_p, src_ip, dest_ip, nb_p, ip6_nb);
            fnet_bool_t exit_flag = FNET_FALSE;

            switch(handler_result)
            {
                case FNET_IP6_EXT_HEADER_HANDLER_RESULT_EXIT:
                    exit_flag = FNET_TRUE;
                    break;
                case FNET_IP6_EXT_HEADER_HANDLER_RESULT_NEXT:
                    break;
                case FNET_IP6_EXT_HEADER_HANDLER_RESULT_TRANSPORT:
                    try_upper_layer = FNET_TRUE;
                    break;
                default:
                    break;
            }

            if(exit_flag == FNET_TRUE)
            {
                return FNET_ERR;
            }
        }
        else
        {
            try_upper_layer = FNET_TRUE;
        }

        ext_header_counter++;
    }
    while((try_upper_layer == FNET_FALSE) || (ext_header != FNET_NULL));

    return FNET_OK;
}

/************************************************************************
* DESCRIPTION: Process No Next Header
*************************************************************************/
static fnet_ip6_ext_header_handler_result_t _fnet_ip6_ext_header_handler_no_next_header (fnet_netif_t *netif, fnet_uint8_t **next_header_p, fnet_ip6_addr_t *src_ip, fnet_ip6_addr_t *dest_ip, fnet_netbuf_t **nb_p, fnet_netbuf_t *ip6_nb)
{
    FNET_COMP_UNUSED_ARG(netif);
    FNET_COMP_UNUSED_ARG(src_ip);
    FNET_COMP_UNUSED_ARG(dest_ip);
    FNET_COMP_UNUSED_ARG(next_header_p);

    /* RFC 2460: The value 59 in the Next Header field of an IPv6 header or any
     * extension header indicates that there is nothing following that
     * header. If the Payload Length field of the IPv6 header indicates the
     * presence of octets past the end of a header whose Next Header field
     * contains 59, those octets must be ignored.*/
    _fnet_netbuf_free_chain(ip6_nb);
    _fnet_netbuf_free_chain(*nb_p);
    return FNET_IP6_EXT_HEADER_HANDLER_RESULT_EXIT;
}

/************************************************************************
* DESCRIPTION: Process Hop by Hop Options and Destimation Options
*************************************************************************/
static fnet_ip6_ext_header_handler_result_t _fnet_ip6_ext_header_handler_options (fnet_netif_t *netif, fnet_uint8_t **next_header_p, fnet_ip6_addr_t *src_ip, fnet_ip6_addr_t *dest_ip, fnet_netbuf_t **nb_p, fnet_netbuf_t *ip6_nb)
{
    fnet_netbuf_t               *nb = *nb_p;
    fnet_ip6_options_header_t   *options_h = (fnet_ip6_options_header_t *)nb->data_ptr;
    fnet_ip6_option_header_t    *option;
    fnet_size_t                 offset;
    fnet_size_t                 length = (((fnet_uint32_t)options_h->hdr_ext_length << 3) + (8u - 2u));
    fnet_bool_t                 exit_flag = FNET_FALSE;

    FNET_COMP_UNUSED_ARG(src_ip);

    /* Check for a valid length. */
    if(length > (nb->length - 2u))
    {   /* Malformed option header */
        _fnet_netbuf_free_chain(nb);
        _fnet_netbuf_free_chain(ip6_nb);
        return FNET_IP6_EXT_HEADER_HANDLER_RESULT_EXIT; 
    }

    offset = 0u;
    while(offset < length)
    {
        option = (fnet_ip6_option_header_t *)&options_h->options[offset];

        /* The RFC2460 supports only PAD0 and PADN options.*/
        switch(option->type)
        {
            /* Pad1 option */
            case FNET_IP6_OPTION_TYPE_PAD1:
                offset++;
                break;
            /* Pad1 option */
            case FNET_IP6_OPTION_TYPE_PADN:
                offset += sizeof(fnet_ip6_option_header_t) + (fnet_size_t)option->data_length;
                break;
            /* Unrecognized Options.*/
            default:
                /* The Option Type identifiers are internally encoded such that their
                 * highest-order two bits specify the action that must be taken if the
                 * processing IPv6 node does not recognize the Option Type.*/
                switch(option->type & FNET_IP6_OPTION_TYPE_UNRECOGNIZED_MASK)
                {
                    /* 00 - skip over this option and continue processing the header.*/
                    case FNET_IP6_OPTION_TYPE_UNRECOGNIZED_SKIP:
                        break;
                    /* 01 - discard the packet. */
                    case FNET_IP6_OPTION_TYPE_UNRECOGNIZED_DISCARD:
                        _fnet_netbuf_free_chain(nb);
                        _fnet_netbuf_free_chain(ip6_nb);

                        exit_flag = FNET_TRUE;
                        break;
                    /* 10 - discard the packet and, regardless of whether or not the
                     *      packet’s Destination Address was a multicast address, send an
                     *      ICMP Parameter Problem, Code 2, message to the packet’s
                     *      Source Address, pointing to the unrecognized Option Type.*/
                    case FNET_IP6_OPTION_TYPE_UNRECOGNIZED_DISCARD_ICMP:
                        _fnet_netbuf_free_chain(nb);
                        _fnet_icmp6_error( netif, FNET_ICMP6_TYPE_PARAM_PROB, FNET_ICMP6_CODE_PP_OPTION,
                                           (fnet_uint32_t)(&option->type) - (fnet_uint32_t)(ip6_nb->data_ptr), ip6_nb ); /* TBD not tested.*/

                        exit_flag = FNET_TRUE;
                        break;
                    /* 11 - discard the packet and, only if the packet’s Destination
                     *      Address was not a multicast address, send an ICMP Parameter
                     *      Problem, Code 2, message to the packet’s Source Address,
                     *      pointing to the unrecognized Option Type.*/
                    case FNET_IP6_OPTION_TYPE_UNRECOGNIZED_DISCARD_UICMP:
                        _fnet_netbuf_free_chain(nb);
                        if(!FNET_IP6_ADDR_IS_MULTICAST(dest_ip))
                        {
                            _fnet_icmp6_error( netif, FNET_ICMP6_TYPE_PARAM_PROB, FNET_ICMP6_CODE_PP_OPTION,
                                               (fnet_uint32_t)(&option->type) - (fnet_uint32_t)(ip6_nb->data_ptr), ip6_nb ); /* TBD not tested.*/
                        }
                        else
                        {
                            _fnet_netbuf_free_chain(ip6_nb);
                        }

                        exit_flag = FNET_TRUE;
                        break;
                    default:
                        break;
                }

                if(exit_flag == FNET_TRUE)
                {
                    return FNET_IP6_EXT_HEADER_HANDLER_RESULT_EXIT;
                }

                offset += sizeof(fnet_ip6_option_header_t) + (fnet_size_t)option->data_length;
                break;
        }
    }

    *next_header_p = &options_h->next_header;
    _fnet_netbuf_trim(nb_p, (fnet_int32_t)(length + 2u));

    return FNET_IP6_EXT_HEADER_HANDLER_RESULT_NEXT;
}

/************************************************************************
* DESCRIPTION: Process Routing header.
*************************************************************************/
static fnet_ip6_ext_header_handler_result_t _fnet_ip6_ext_header_handler_routing_header(fnet_netif_t *netif, fnet_uint8_t **next_header_p, fnet_ip6_addr_t *src_ip, fnet_ip6_addr_t *dest_ip, fnet_netbuf_t **nb_p, fnet_netbuf_t *ip6_nb)
{
    fnet_ip6_ext_header_handler_result_t    result;
    fnet_netbuf_t                           *nb = *nb_p;
    fnet_ip6_routing_header_t               *routing_h;

    FNET_COMP_UNUSED_ARG(src_ip);
    FNET_COMP_UNUSED_ARG(dest_ip);

    routing_h = (fnet_ip6_routing_header_t *)nb->data_ptr;

    /* RFC 2460 4.4: If, while processing a received packet, a node encounters a Routing
    * header with an unrecognized Routing Type value, the required behavior
    * of the node depends on the value of the Segments Left field, as
    * follows:
    * -If Segments Left is non-zero, the node must discard the packet and
    *  send an ICMP Parameter Problem, Code 0, message to the packet’s
    *  Source Address, pointing to the unrecognized Routing Type.
    */
    if( routing_h->segments_left > 0u)
    {
        _fnet_netbuf_free_chain(nb);
        _fnet_icmp6_error( netif, FNET_ICMP6_TYPE_PARAM_PROB, FNET_ICMP6_CODE_PP_HEADER,
                           (fnet_uint32_t)(&routing_h->routing_type) - (fnet_uint32_t)(ip6_nb->data_ptr), ip6_nb ); /* TBD not tested.*/
        result = FNET_IP6_EXT_HEADER_HANDLER_RESULT_EXIT;
    }
    else
    {
        /* -If Segments Left is zero, the node must ignore the Routing header
         *  and proceed to process the next header in the packet, whose type
         *  is identified by the Next Header field in the Routing header.*/

        *next_header_p = &routing_h->next_header;
        _fnet_netbuf_trim(nb_p, (fnet_int32_t)(((fnet_uint32_t)routing_h->hdr_ext_length << 3u) + (8u)));

        result = FNET_IP6_EXT_HEADER_HANDLER_RESULT_NEXT;
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Process IPv6 Fragment header.
*************************************************************************/
#if FNET_CFG_IP6_FRAGMENTATION
static fnet_ip6_ext_header_handler_result_t _fnet_ip6_ext_header_handler_fragment_header(fnet_netif_t *netif, fnet_uint8_t **next_header_p, fnet_ip6_addr_t *src_ip, fnet_ip6_addr_t *dest_ip, fnet_netbuf_t **nb_p, fnet_netbuf_t *ip6_nb)
{
    fnet_ip6_ext_header_handler_result_t result;

    if( (*nb_p = _fnet_ip6_reassembly(netif, nb_p, ip6_nb, src_ip, dest_ip )) != FNET_NULL)
    {
        fnet_ip6_header_t  *ip6_header = (fnet_ip6_header_t *)ip6_nb->data_ptr;
        *next_header_p = &ip6_header->next_header;

        result = FNET_IP6_EXT_HEADER_HANDLER_RESULT_TRANSPORT;
    }
    else
    {
        result = FNET_IP6_EXT_HEADER_HANDLER_RESULT_EXIT;
    }

    return result;
}
#endif

/************************************************************************
* DESCRIPTION: Prepare sockets addreses for upper protocol.
*************************************************************************/
void _fnet_ip6_set_socket_addr(fnet_netif_t *netif, fnet_ip6_header_t *ip_hdr, struct fnet_sockaddr *src_addr,  struct fnet_sockaddr *dest_addr )
{
    fnet_memset_zero(src_addr, sizeof(struct fnet_sockaddr));
    src_addr->sa_family = AF_INET6;
    FNET_IP6_ADDR_COPY(&ip_hdr->source_addr, &((struct fnet_sockaddr_in6 *)(src_addr))->sin6_addr.s6_addr);
    ((struct fnet_sockaddr_in6 *)(src_addr))->sin6_scope_id = netif->scope_id;

    fnet_memset_zero(dest_addr, sizeof(struct fnet_sockaddr));
    dest_addr->sa_family = AF_INET6;
    FNET_IP6_ADDR_COPY(&ip_hdr->destination_addr, &((struct fnet_sockaddr_in6 *)(dest_addr))->sin6_addr.s6_addr);
    ((struct fnet_sockaddr_in6 *)(dest_addr))->sin6_scope_id = netif->scope_id;
}

/************************************************************************
* DESCRIPTION: IPv6 input function.
*************************************************************************/
void _fnet_ip6_input( fnet_netif_t *netif, fnet_netbuf_t *nb )
{
    if(netif && nb)
    {
        if(_fnet_ip_queue_append(&ip6_queue, netif, nb) != FNET_OK)
        {
            _fnet_netbuf_free_chain(nb);
            return;
        }

        /* Raise IPv6 event.*/
        fnet_event_raise(ip6_event);
    }
}

/************************************************************************
* DESCRIPTION: This function performs handling of incoming IPv6 datagrams.
*************************************************************************/
static void _fnet_ip6_input_low(void *cookie)
{
    fnet_ip6_header_t       *hdr;
    fnet_netif_t            *netif;
    fnet_netbuf_t           *nb;
    fnet_prot_if_t          *protocol;
    fnet_ip6_addr_t         *source_addr;
    fnet_ip6_addr_t         *destination_addr;
    fnet_uint16_t           payload_length;
    struct fnet_sockaddr    src_addr;
    struct fnet_sockaddr    dest_addr;

    FNET_COMP_UNUSED_ARG(cookie);

    fnet_isr_lock();

    while((nb = _fnet_ip_queue_read(&ip6_queue, &netif)) != 0)
    {
        nb->next_chain = 0;

        /* RFC 4862: By disabling IP operation,
         * silently drop any IP packets received on the interface.*/
        if((netif->nd6_if_ptr) && (netif->nd6_if_ptr->ip6_disabled))
        {
            goto DROP;
        }

        /* The header must reside in contiguous area of memory. */
        if(_fnet_netbuf_pullup(&nb, sizeof(fnet_ip6_header_t)) == FNET_ERR)
        {
            goto DROP;
        }

        hdr = (fnet_ip6_header_t *) nb->data_ptr;
        source_addr = &hdr->source_addr; /* TBD Save copy or do ICMP copy*/
        destination_addr = &hdr->destination_addr;

        payload_length = fnet_ntohs(hdr->length);

        if(nb->total_length > (sizeof(fnet_ip6_header_t) + (fnet_size_t)payload_length))
        {
            /* Logical size and the physical size of the packet should be the same.*/
            _fnet_netbuf_trim(&nb, (fnet_int32_t)sizeof(fnet_ip6_header_t) + (fnet_int32_t)payload_length - (fnet_int32_t)nb->total_length );
        }

        /*******************************************************************
         * Start IPv6 header  processing.
         *******************************************************************/
        if( (nb->total_length >= sizeof(fnet_ip6_header_t))                 /* Check Size of the packet. */
            && (nb->total_length >= (sizeof(fnet_ip6_header_t) + (fnet_size_t)payload_length))
            && (FNET_IP6_HEADER_GET_VERSION(hdr) == 6u)                     /* Check the IP Version. */
            && (!FNET_IP6_ADDR_IS_MULTICAST(&hdr->source_addr))             /* Validate source address. */
            && ((_fnet_netif_is_my_ip6_addr(netif, &hdr->destination_addr))  /* Validate destination address. */
                || FNET_IP6_ADDR_IS_MULTICAST(&hdr->destination_addr)        /* Pass multicast destination address.*/
                || (netif->netif_api->netif_type == FNET_NETIF_TYPE_LOOPBACK) )
          )
        {
            fnet_uint8_t    *next_header = &hdr->next_header;
            fnet_netbuf_t   *ip6_nb;

            ip6_nb = _fnet_netbuf_copy(nb, 0u, ((nb->total_length > FNET_IP6_DEFAULT_MTU) ? FNET_IP6_DEFAULT_MTU : FNET_NETBUF_COPYALL), FNET_FALSE); /* Used mainly for ICMP errors .*/
            if(ip6_nb == FNET_NULL)
            {
                goto DROP;
            }

#if FNET_CFG_CPU_ETH_HW_RX_PROTOCOL_CHECKSUM
            if((netif->features & FNET_NETIF_FEATURE_HW_RX_PROTOCOL_CHECKSUM) &&
               ((hdr->next_header == FNET_PROT_ICMP4) ||
                (hdr->next_header == FNET_PROT_UDP) ||
                (hdr->next_header == FNET_PROT_TCP)) )
            {
                nb->flags |= FNET_NETBUF_FLAG_HW_PROTOCOL_CHECKSUM;
            }
#endif

            _fnet_netbuf_trim(&nb, (fnet_int32_t)sizeof(fnet_ip6_header_t));

            /********************************************
             * Extension headers processing.
             *********************************************/
            if(_fnet_ip6_ext_header_process(netif, &next_header, source_addr, destination_addr, &nb, ip6_nb) == FNET_ERR)
            {
                continue;
            }

            /* Prepare addreses for upper protocol.*/
            _fnet_ip6_set_socket_addr(netif, hdr, &src_addr,  &dest_addr );

#if FNET_CFG_RAW
            /* RAW Sockets input.*/
            _fnet_raw_input(netif, &src_addr, &dest_addr, nb, ip6_nb);
#endif

            /* Note: (http://www.cisco.com/web/about/ac123/ac147/archived_issues/ipj_9-3/ipv6_internals.html)
             * Note that there is no standard extension header format, meaning that when a host
             * encounters a header that it does not recognize in the protocol chain, the only thing
             * it can do is discard the packet. Worse, firewalls and routers configured to filter IPv6
             * have the same problem: as soon as they encounter an unknown extension header,
             * they must decide to allow or disallow the packet, even though another header deeper
             * inside the packet would possibly trigger the opposite behavior. In other words, an IPv6
             * packet with a TCP payload that would normally be allowed through could be blocked if
             * there is an unknown extension header between the IPv6 and TCP headers.
             */

            /********************************************
             * Transport layer processing (ICMP/TCP/UDP).
             *********************************************/
            /* Find transport protocol.*/
            if((protocol = _fnet_prot_find(AF_INET6, SOCK_UNSPEC, (fnet_uint32_t) * next_header)) != FNET_NULL)
            {
                protocol->prot_input(netif, &src_addr,  &dest_addr, nb, ip6_nb);
                /* After that nb may point to wrong place. Do not use it.*/
            }
            else
                /* No protocol found.*/
            {
                _fnet_netbuf_free_chain(nb);
                /* RFC 2460 4:If, as a result of processing a header, a node is required to proceed
                 * to the next header but the Next Header value in the current header is
                 * unrecognized by the node, it should discard the packet and send an
                 * ICMP Parameter Problem message to the source of the packet, with an
                 * ICMP Code value of 1 ("unrecognized Next Header type encountered")
                 * and the ICMP Pointer field containing the offset of the unrecognized
                 * value within the original packet. The same action should be taken if
                 * a node encounters a Next Header value of zero in any header other
                 * than an IPv6 header.*/
                _fnet_icmp6_error( netif, FNET_ICMP6_TYPE_PARAM_PROB, FNET_ICMP6_CODE_PP_NEXT_HEADER,
                                   (fnet_uint32_t)(next_header) - (fnet_uint32_t)(ip6_nb->data_ptr), ip6_nb ); /* TBD not tested.*/
            }
        }
        else
        {
        DROP:
            _fnet_netbuf_free_chain(nb);
        }
    } /* while end */

    fnet_isr_unlock();
}

/************************************************************************
* DESCRIPTION: Returns scope of the IPv6 address (Node-local,
* link-local, site-local or global.).
*************************************************************************/
fnet_int32_t _fnet_ip6_addr_scope(const fnet_ip6_addr_t *ip_addr)
{
    fnet_int32_t result = FNET_IP6_ADDR_SCOPE_GLOBAL;

    /* Local Host. */
    if(FNET_IP6_ADDR_IS_LINKLOCAL(ip_addr))
    {
        result = FNET_IP6_ADDR_SCOPE_LINKLOCAL;
    }
    else if(FNET_IP6_ADDR_IS_SITELOCAL(ip_addr))
    {
        result = FNET_IP6_ADDR_SCOPE_SITELOCAL;
    }
    else if(FNET_IP6_ADDR_IS_MULTICAST(ip_addr))/* Multicast. */
    {
        fnet_int32_t scope = (fnet_int32_t)FNET_IP6_ADDR_MULTICAST_SCOPE(ip_addr);

        switch(scope)
        {
            case FNET_IP6_ADDR_SCOPE_INTERFACELOCAL:
                result = FNET_IP6_ADDR_SCOPE_INTERFACELOCAL;
                break;
            case FNET_IP6_ADDR_SCOPE_LINKLOCAL:
                result = FNET_IP6_ADDR_SCOPE_LINKLOCAL;
                break;
            case FNET_IP6_ADDR_SCOPE_SITELOCAL:
                result = FNET_IP6_ADDR_SCOPE_SITELOCAL;
                break;
            default:
                break;
        }
    }
    else if(FNET_IP6_ADDR_EQUAL(ip_addr, &fnet_ip6_addr_loopback)) /* Loopback interface - special case.*/
    {
        result = FNET_IP6_ADDR_SCOPE_INTERFACELOCAL;
    }
    else
    {}

    return result;
}

/************************************************************************
* DESCRIPTION: RFC3484 2.2. Common Prefix Length
*  We define the common prefix length CommonPrefixLen(A, B) of two
*  addresses A and B as the length of the longest prefix (looking at the
*  most significant, or leftmost, bits) that the two addresses have in
*  common. It ranges from 0 to 128.
*************************************************************************/
fnet_size_t _fnet_ip6_common_prefix_length(const fnet_ip6_addr_t *ip_addr_1, const fnet_ip6_addr_t *ip_addr_2)
{
    fnet_size_t     length = 0u;
    fnet_index_t    i;
    fnet_index_t    bit_i;
    fnet_uint8_t    byte_xor;

    for(i = 0u; i < sizeof(fnet_ip6_addr_t); i++)
    {
        /* XOR */
        byte_xor = (fnet_uint8_t)(ip_addr_1->addr[i] ^ ip_addr_2->addr[i]);

        for(bit_i = 0u; bit_i < 8u; bit_i++)
        {
            if((byte_xor & 0x80u) == 0x00u)
            {
                length++;
                byte_xor <<= 1; /* Shift to the next bit*/
            }
            else
            {
                break;
            }
        }
    }

    return length;
}

/************************************************************************
* DESCRIPTION:  Returns label value from policy table.
*************************************************************************/
static fnet_uint32_t _fnet_ip6_policy_label( const fnet_ip6_addr_t *addr )
{
    fnet_index_t    i;
    fnet_uint32_t   label = 0u;
    fnet_size_t     biggest_prefix_length = 0u;

    /* Find the entry in the list. */
    for(i = 0u; i < FNET_IP6_IF_POLICY_TABLE_SIZE; i++)
    {
        fnet_size_t prefix_length = fnet_ip6_if_policy_table[i].prefix_length;

        if(_fnet_ip6_addr_pefix_cmp(&fnet_ip6_if_policy_table[i].prefix, addr, prefix_length) == FNET_TRUE)
        {
            if(prefix_length > biggest_prefix_length)
            {
                biggest_prefix_length = prefix_length;
                label = fnet_ip6_if_policy_table[i].label;
            }

        }
    }
    return label;
}

/************************************************************************
* DESCRIPTION: Compares first "prefix_length" bits of the addresses.
*************************************************************************/
fnet_bool_t _fnet_ip6_addr_pefix_cmp(const fnet_ip6_addr_t *addr_1, const fnet_ip6_addr_t *addr_2, fnet_size_t prefix_length)
{
    fnet_bool_t     result;
    fnet_size_t     prefix_length_bytes = prefix_length >> 3;
    fnet_uint32_t   prefix_length_bits_mask = (fnet_uint32_t)((1u << (prefix_length % 8u)) - 1u) << (8u - (prefix_length % 8u));

    if((prefix_length <= 128u)
       && (fnet_memcmp(addr_1, addr_2, prefix_length_bytes) == 0)
       && ((addr_1->addr[prefix_length_bytes] & prefix_length_bits_mask) == (addr_2->addr[prefix_length_bytes] & prefix_length_bits_mask)) )
    {
        result = FNET_TRUE;
    }
    else
    {
        result = FNET_FALSE;
    }

    return result;
}

/************************************************************************
* DESCRIPTION:  Selects the best source address to use with a
*               destination address, Based on RFC3484.
*************************************************************************/
const fnet_ip6_addr_t *fnet_ip6_select_src_addr( const fnet_ip6_addr_t *dest_addr)
{
    const fnet_ip6_addr_t   *result;

    _fnet_stack_mutex_lock();
    result = _fnet_ip6_select_src_addr(FNET_NULL, dest_addr);
    _fnet_stack_mutex_unlock();

    return result;
}
/* Private */
const fnet_ip6_addr_t *_fnet_ip6_select_src_addr(fnet_netif_t *netif /* Optional.*/, const fnet_ip6_addr_t *dest_addr)
{
    fnet_index_t    i;
    fnet_ip6_addr_t *best_addr = FNET_NULL;
    fnet_int32_t    best_scope;
    fnet_int32_t    dest_scope;
    fnet_int32_t    new_scope;
    fnet_size_t     best_prefix_length;
    fnet_size_t     new_prefix_length;
    fnet_uint32_t   dest_label;
    fnet_uint32_t   best_label;
    fnet_uint32_t   new_label;
    fnet_netif_t    *if_dest_cur;
    fnet_netif_t    *if_dest_best = FNET_NULL;
    fnet_bool_t     terminate_algorithm = FNET_FALSE;

    /* Just take the first/last interface.*/
    if(dest_addr)
    {
        for(if_dest_cur = fnet_netif_list; (if_dest_cur != FNET_NULL) && (terminate_algorithm == FNET_FALSE) ; if_dest_cur = if_dest_cur->next)
        {
            dest_scope = _fnet_ip6_addr_scope(dest_addr);
            dest_label = _fnet_ip6_policy_label(dest_addr);

            /* Just continue the first loop.*/
            for(i = 0u; i < FNET_NETIF_IP6_ADDR_MAX; i++)
            {
                /* Skip not used enries. */
                if(if_dest_cur->ip6_addr[i].state == FNET_NETIF_IP6_ADDR_STATE_NOT_USED)
                {
                    continue;
                }
                else if(best_addr == FNET_NULL)
                {
                    /* Just take the first used address, by default.=> Link-local address.*/
                    best_addr = &if_dest_cur->ip6_addr[i].address;
                    if_dest_best = if_dest_cur;
                }
                else
                {}

                /* RFC3484 Source Address Selection.*/
                /* Rule 1:  Prefer same address.*/
                if(FNET_IP6_ADDR_EQUAL(dest_addr, &if_dest_cur->ip6_addr[i].address))
                {
                    best_addr = &if_dest_cur->ip6_addr[i].address;
                    if_dest_best = if_dest_cur;
                    /* IBM z/OS: If either address is the destination address, choose that address as the source address and terminate the entire algorithm.*/
                    terminate_algorithm = FNET_TRUE;
                    break;
                }

                /* Rule 2:  Prefer appropriate scope.*/
                /* If Scope(SA) < Scope(SB): If Scope(SA) < Scope(D), then prefer SB
                 * and otherwise prefer SA.  Similarly, if Scope(SB) < Scope(SA): If
                 * Scope(SB) < Scope(D), then prefer SA and otherwise prefer SB.
                 */
                best_scope = _fnet_ip6_addr_scope(best_addr);
                new_scope = _fnet_ip6_addr_scope(&if_dest_cur->ip6_addr[i].address);

                if(best_scope < new_scope)
                {
                    if(best_scope < dest_scope)
                    {
                        best_addr = &if_dest_cur->ip6_addr[i].address; /* PFI take pointer at the begining.*/
                        if_dest_best = if_dest_cur;
                    }
                    continue;
                }
                else if( new_scope < best_scope)
                {
                    if( new_scope >= dest_scope)
                    {
                        best_addr = &if_dest_cur->ip6_addr[i].address;
                        if_dest_best = if_dest_cur;
                    }
                    continue;
                }
                else
                {}

                /* Rule 3:  Avoid deprecated addresses.
                 * XX: Not implemented - we do not store depricated addresses."*/

                /* Rule 4:  Prefer home addresses.
                 * XX: Not implemented - we do not have Mobile IPv6.*/

                /* Rule 5:  Prefer outgoing interface.
                 */
                if((if_dest_best == netif ) && (if_dest_cur != netif))
                {
                    continue;
                }
                if((if_dest_best != netif ) && (if_dest_cur == netif))
                {
                    best_addr = &if_dest_cur->ip6_addr[i].address;
                    if_dest_best = if_dest_cur;
                    continue;
                }

                /* Rule 6:  Prefer matching label.
                 * If Label(SA) = Label(D) and Label(SB) <> Label(D), then prefer SA.
                 * Similarly, if Label(SB) = Label(D) and Label(SA) <> Label(D), then
                 * prefer SB.
                 */
                best_label = _fnet_ip6_policy_label(best_addr);
                new_label = _fnet_ip6_policy_label(&if_dest_cur->ip6_addr[i].address);

                if(best_label == dest_label)
                {
                    if(new_label != dest_label)
                    {
                        continue; /* Prefer SA.*/
                    }
                }
                if(new_label == dest_label)
                {
                    if(new_label != dest_label)
                    {
                        best_addr = &if_dest_cur->ip6_addr[i].address;
                        if_dest_best = if_dest_cur;
                        continue;
                    }
                }

                /* Rule 7:  Prefer public addresses.
                 * If SA is a public address and SB is a temporary address, then prefer
                 * SA.  Similarly, if SB is a public address and SA is a temporary
                 * address, then prefer SB.
                 * XX: We do not support "temporary"/"random" addresses.*/

                /* Rule 8:  Use longest matching prefix.
                 * If CommonPrefixLen(SA, D) > CommonPrefixLen(SB, D), then prefer SA.
                 * Similarly, if CommonPrefixLen(SB, D) > CommonPrefixLen(SA, D), then
                 * prefer SB.
                 */
                best_prefix_length = _fnet_ip6_common_prefix_length(best_addr, dest_addr);
                new_prefix_length = _fnet_ip6_common_prefix_length(&if_dest_cur->ip6_addr[i].address, dest_addr);

                if(new_prefix_length > best_prefix_length)
                    /* Found better one.*/
                {
                    best_addr = &if_dest_cur->ip6_addr[i].address;
                    if_dest_best = if_dest_cur;
                }
            } /* for(IP6_IF_ADDRESSES_MAX) */
        }/* for(if_dest_cur) */
    } /* if(dest_addr) */

    return best_addr;
}

/************************************************************************
* DESCRIPTION: Returns MTU (based on ND and PMTU).
*
* RETURNS: MTU
*************************************************************************/
fnet_size_t _fnet_ip6_mtu( fnet_netif_t *netif)
{
    fnet_size_t mtu;

    if(netif)
    {
#if FNET_CFG_IP6_PMTU_DISCOVERY
        if(netif->pmtu) /* If PMTU is enabled for the interface.*/
        {
            mtu = netif->pmtu;
        }
        else
#endif
        {
            mtu = netif->nd6_if_ptr  ? netif->nd6_if_ptr->mtu : netif->netif_mtu;
        }

        if(mtu < FNET_IP6_DEFAULT_MTU)
        {
            mtu = FNET_IP6_DEFAULT_MTU;
        }
    }
    else
    {
        mtu = 0u;
    }

    return mtu;
}

/************************************************************************
* DESCRIPTION: This function performs IPv6 routing
*              on an outgoing IP packet.
*************************************************************************/
fnet_netif_t *_fnet_ip6_route(const fnet_ip6_addr_t *src_ip /*optional*/, const fnet_ip6_addr_t *dest_ip)
{
    fnet_netif_t        *netif = FNET_NULL;

    /* Validate destination address. */
    /* RFC3513: The unspecified address must not be used as the destination address
     * of IPv6 packets or in IPv6 Routing Headers.*/
    if(dest_ip && (!FNET_IP6_ADDR_IS_UNSPECIFIED(dest_ip)))
    {
        /*
         * The specified source address may
         * influence the candidate set, by affecting the choice of outgoing
         * interface.  If the application or upper-layer specifies a source
         * address that is in the candidate set for the destination, then the
         * network layer MUST respect that choice.  If the application or
         * upper-layer does not specify a source address, then the network layer
         * uses the source address selection algorithm
         */
        if((src_ip == FNET_NULL) || FNET_IP6_ADDR_IS_UNSPECIFIED(src_ip))
            /* Determine a source address. */
        {
            src_ip = _fnet_ip6_select_src_addr(FNET_NULL, dest_ip);
        }

        if(src_ip != FNET_NULL)
        {
            /* Determine an output interface. */
            netif = _fnet_netif_get_by_ip6_addr(src_ip);
        }
    }

    return netif;
}

/************************************************************************
* DESCRIPTION: This function returns FNET_TRUE if the protocol message
*              will be fragmented by IPv6, and FNET_FALSE otherwise.
*************************************************************************/
fnet_bool_t _fnet_ip6_will_fragment( fnet_netif_t *netif, fnet_size_t protocol_message_size)
{
    fnet_bool_t res;

    if(
#if FNET_CFG_IP6_PMTU_DISCOVERY
        /*
         * In response to an IPv6 packet that is sent to an IPv4 destination
         * (i.e., a packet that undergoes translation from IPv6 to IPv4), the
         * originating IPv6 node may receive an ICMP Packet Too Big message
         * reporting a Next-Hop MTU less than 1280.  In that case, the IPv6 node
         * is not required to reduce the size of subsequent packets to less than
         * 1280, but must include a Fragment header in those packets so that the
         * IPv6-to-IPv4 translating router can obtain a suitable Identification
         * value to use in resulting IPv4 fragments.  Note that this means the
         * payload may have to be reduced to 1232 octets (1280 minus 40 for the
         * IPv6 header and 8 for the Fragment header), and smaller still if
         * additional extension headers are used.
         */

        ((netif->pmtu) /* If PMTU is enabled.*/ &&  ((protocol_message_size + sizeof(fnet_ip6_header_t)) > netif->pmtu)) ||
        ( (!netif->pmtu) &&
#endif
          ((protocol_message_size + sizeof(fnet_ip6_header_t)) > _fnet_ip6_mtu(netif))
#if FNET_CFG_IP6_PMTU_DISCOVERY
        )/* IP Fragmentation. */
#endif
    )
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
* DESCRIPTION: IPv6 output function.
*
* RETURNS: FNET_OK=OK
*          FNET_ERR_NETUNREACH=No route
*          FNET_ERR_MSGSIZE=Size error
*          FNET_ERR_NOMEM=No memory
*************************************************************************/
fnet_error_t _fnet_ip6_output(fnet_netif_t *netif /*optional*/, const fnet_ip6_addr_t *src_ip /*optional*/, const fnet_ip6_addr_t *dest_ip,
                              fnet_uint8_t protocol, fnet_uint8_t hop_limit /*optional*/, fnet_netbuf_t *nb, FNET_COMP_PACKED_VAR fnet_uint16_t *checksum)
{
    fnet_error_t        error_code;
    fnet_netbuf_t       *nb_header;
    fnet_ip6_header_t   *ip6_header;
    fnet_size_t         mtu;

    /* Check maximum packet size. */
    if((nb->total_length + sizeof(fnet_ip6_header_t)) > FNET_IP6_MAX_PACKET)
    {
        error_code = FNET_ERR_MSGSIZE;
        goto DROP;
    }

    /* Validate destination address. */
    /* RFC3513: The unspecified address must not be used as the destination address
     * of IPv6 packets or in IPv6 Routing Headers.*/
    if((dest_ip == FNET_NULL) || FNET_IP6_ADDR_IS_UNSPECIFIED(dest_ip))
    {
        error_code = FNET_ERR_DESTADDRREQ;
        goto DROP;
    }

    /*
     * The specified source address may
     * influence the candidate set, by affecting the choice of outgoing
     * interface.  If the application or upper-layer specifies a source
     * address that is in the candidate set for the destination, then the
     * network layer MUST respect that choice.  If the application or
     * upper-layer does not specify a source address, then the network layer
     * uses the source address selection algorithm
     */
    if(src_ip == FNET_NULL) /* It may be any address.*/
        /* Determine a source address. */
    {
        src_ip = _fnet_ip6_select_src_addr(netif, dest_ip);

        if(src_ip == FNET_NULL)
        {
            error_code = FNET_ERR_NETUNREACH;
            goto DROP;
        }
    }

    if(netif == FNET_NULL)
        /* Determine an output interface. */
    {
        netif = _fnet_netif_get_by_ip6_addr(src_ip);

        if(netif == FNET_NULL) /* Ther is no any initializaed IF.*/
        {
            error_code = FNET_ERR_NETUNREACH;
            goto DROP;
        }
    }

    /* RFC 4862: By disabling IP operation, the node will then not
     * send any IP packets from the interface.*/
    if((netif->nd6_if_ptr) && (netif->nd6_if_ptr->ip6_disabled))
    {
        error_code = FNET_ERR_IPDISABLED;
        goto DROP;
    }

    /* Pseudo checksum. */
    if(checksum)
    {
        *checksum = _fnet_checksum_pseudo_netbuf_end( *checksum, (const fnet_uint8_t *)src_ip, (const fnet_uint8_t *)dest_ip, sizeof(fnet_ip6_addr_t) );
    }

    /****** Construct IP header. ******/
    if((nb_header = _fnet_netbuf_new(sizeof(fnet_ip6_header_t), FNET_TRUE)) == 0)
    {
        error_code = FNET_ERR_NOMEM;
        goto DROP;
    }

    ip6_header = (fnet_ip6_header_t *)nb_header->data_ptr;

    ip6_header->version__tclass = FNET_IP6_VERSION << 4;
    ip6_header->tclass__flowl = 0u;
    ip6_header->flowl = 0u;
    ip6_header->length = fnet_htons((fnet_uint16_t)nb->total_length);
    ip6_header->next_header = protocol;

    /* Set Hop Limit.*/
    if(netif->nd6_if_ptr && (hop_limit == 0u))
    {
        hop_limit = netif->nd6_if_ptr->cur_hop_limit; /* Defined by ND.*/
    }
    ip6_header->hop_limit = hop_limit;


    FNET_IP6_ADDR_COPY(src_ip, &ip6_header->source_addr);
    FNET_IP6_ADDR_COPY(dest_ip, &ip6_header->destination_addr);

    mtu = _fnet_ip6_mtu(netif);

    if(
#if FNET_CFG_IP6_PMTU_DISCOVERY
        /*
         * In response to an IPv6 packet that is sent to an IPv4 destination
         * (i.e., a packet that undergoes translation from IPv6 to IPv4), the
         * originating IPv6 node may receive an ICMP Packet Too Big message
         * reporting a Next-Hop MTU less than 1280.  In that case, the IPv6 node
         * is not required to reduce the size of subsequent packets to less than
         * 1280, but must include a Fragment header in those packets so that the
         * IPv6-to-IPv4 translating router can obtain a suitable Identification
         * value to use in resulting IPv4 fragments.  Note that this means the
         * payload may have to be reduced to 1232 octets (1280 minus 40 for the
         * IPv6 header and 8 for the Fragment header), and smaller still if
         * additional extension headers are used.
         */

        ((netif->pmtu) /* If PMTU is enabled.*/ &&  ((nb->total_length + nb_header->total_length) > netif->pmtu)) ||
        ( (!netif->pmtu) &&
#endif
          ((nb->total_length + nb_header->total_length) > mtu)
#if FNET_CFG_IP6_PMTU_DISCOVERY
        )/* IP Fragmentation. */
#endif
    )
    {
#if FNET_CFG_IP6_FRAGMENTATION
        fnet_size_t                 first_frag_length;
        fnet_size_t                 frag_length; /* The number of data in each fragment. */
        fnet_int32_t                tmp;
        fnet_size_t                 offset;
        fnet_index_t                error = 0u;
        fnet_netbuf_t               *nb_prev;
        fnet_netbuf_t                **nb_next_ptr;
        fnet_size_t                 header_length = sizeof(fnet_ip6_header_t) + sizeof(fnet_ip6_fragment_header_t);
        fnet_ip6_header_t           *ip6_header_new;
        fnet_ip6_fragment_header_t  *ip6_fragment_header;
        fnet_ip6_fragment_header_t  *ip6_fragment_header_new;
        fnet_netbuf_t               *nb_frag_header;
        fnet_size_t                 total_length;
        static fnet_uint32_t        ip6_id = 0u;

        tmp = (fnet_int32_t)((mtu - header_length) & ~7u); /* Rounded down to an 8-byte boundary.*/
        if(tmp < 8)             /* The MTU is too small.*/
        {
            error_code = FNET_ERR_MSGSIZE;
            _fnet_netbuf_free_chain(nb_header);
            goto DROP;
        }
        frag_length = (fnet_size_t)tmp;

        first_frag_length = frag_length;

        if((nb_frag_header = _fnet_netbuf_new(sizeof(fnet_ip6_fragment_header_t), FNET_TRUE)) == 0)
        {
            error_code = FNET_ERR_NOMEM;
            _fnet_netbuf_free_chain(nb_header);
            goto DROP;
        }

        nb = _fnet_netbuf_concat(nb_frag_header, nb);
        nb = _fnet_netbuf_concat(nb_header, nb);

        nb_next_ptr = &nb->next_chain;

        /* The header (and options) must reside in contiguous area of memory.*/
        if(_fnet_netbuf_pullup(&nb,  header_length) == FNET_ERR)
        {
            error_code = FNET_ERR_NOMEM;
            goto DROP;
        }

        ip6_header = (fnet_ip6_header_t *)nb->data_ptr;
        ip6_fragment_header = (fnet_ip6_fragment_header_t *)((fnet_uint32_t)ip6_header + sizeof(fnet_ip6_header_t));

        nb_prev = nb;

        total_length = nb->total_length;

        ip6_id++;

        ip6_header->next_header = FNET_IP6_TYPE_FRAGMENT_HEADER;

        ip6_fragment_header->id = fnet_htonl(ip6_id);
        ip6_fragment_header->_reserved = 0u;
        ip6_fragment_header->next_header = protocol;
        ip6_fragment_header->offset_more = FNET_HTONS(FNET_IP6_FRAGMENT_MF_MASK);

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

            ip6_header_new = (fnet_ip6_header_t *)nb->data_ptr;
            ip6_fragment_header_new = (fnet_ip6_fragment_header_t *)((fnet_uint32_t)ip6_header_new + sizeof(fnet_ip6_header_t));

            fnet_memcpy(ip6_header_new, ip6_header, header_length); /* Copy IPv6 header.*/


            ip6_fragment_header_new->offset_more = fnet_htons((fnet_uint16_t)(offset - header_length) );

            if(offset + frag_length >= total_length)
            {
                frag_length = (total_length - offset);
            }
            else
            {
                ip6_fragment_header_new->offset_more |= FNET_HTONS(FNET_IP6_FRAGMENT_MF_MASK);
            }

            /* Copy the data from the original packet into the fragment.*/
            if((nb_tmp = _fnet_netbuf_copy(nb_prev, offset, frag_length, FNET_FALSE)) == 0)
            {
                error++;
                _fnet_netbuf_free_chain(nb);
                goto FRAG_END;
            }

            nb = _fnet_netbuf_concat(nb, nb_tmp);

            ip6_header_new->length = fnet_htons((fnet_uint16_t)(nb->total_length - sizeof(fnet_ip6_header_t)) );

            *nb_next_ptr = nb;
            nb_next_ptr = &nb->next_chain;
        }

        /* Update the first fragment.*/
        nb = nb_prev;
        _fnet_netbuf_trim(&nb, (fnet_int32_t)(/*header_length +*/ first_frag_length -  fnet_ntohs(ip6_header->length) /*- sizeof(fnet_ip6_header_t)*/) );

        ip6_header->length = fnet_htons((fnet_uint16_t)(nb->total_length - sizeof(fnet_ip6_header_t)) );

    FRAG_END:
        for (nb = nb_prev; nb; nb = nb_prev)    /* Send each fragment.*/
        {
            nb_prev = nb->next_chain;
            nb->next_chain = 0;

            if(error == 0u)
            {
                _fnet_ip6_netif_output(netif, src_ip, dest_ip, nb);
            }
            else
            {
                _fnet_netbuf_free_chain(nb);
            }
        }

#else
        error_code = FNET_ERR_MSGSIZE;   /* Discard datagram.*/
        goto DROP;
#endif  /* FNET_CFG_IP6_FRAGMENTATION */
    }
    else
    {
        nb = _fnet_netbuf_concat(nb_header, nb);
        _fnet_ip6_netif_output(netif, src_ip, dest_ip, nb);
    }

    return (FNET_ERR_OK);

DROP:
    _fnet_netbuf_free_chain(nb);           /* Discard datagram */

    return (error_code);
}

/************************************************************************
* DESCRIPTION:
*************************************************************************/
static void _fnet_ip6_netif_output(struct fnet_netif *netif, const fnet_ip6_addr_t *src_ip, const fnet_ip6_addr_t *dest_ip, fnet_netbuf_t *nb)
{

#if FNET_CFG_LOOPBACK
    /***********************************************
     * Handle possible loopback
     ***********************************************/

    /* Anything sent to one of the host's own IP address is sent to the loopback interface.*/
    if(_fnet_netif_is_my_ip6_addr(netif, dest_ip) == FNET_TRUE)
    {
        netif = FNET_LOOP_IF;
    }
    else
#endif /* FNET_CFG_LOOPBACK */
    {
#if  FNET_CFG_LOOPBACK && FNET_CFG_LOOPBACK_MULTICAST
        /* Send Multicast packets also to the loopback.*/
        if((netif != FNET_LOOP_IF) && FNET_IP6_ADDR_IS_MULTICAST(dest_ip))
        {
            fnet_netbuf_t *nb_loop;

            /* Datagrams sent to amulticast address are copied to the loopback interface.*/
            if((nb_loop = _fnet_netbuf_copy(nb, 0, FNET_NETBUF_COPYALL, FNET_TRUE)) != 0)
            {
                _fnet_loop_output_ip6(netif, src_ip,  dest_ip, nb_loop);
            }
        }
#endif /* FNET_CFG_LOOPBACK && FNET_CFG_LOOPBACK_MULTICAST */
    }
    netif->netif_api->netif_output_ip6(netif, src_ip,  dest_ip, nb); /* IPv6 Transmit function.*/
}

/************************************************************************
* DESCRIPTION: Get IPv6 solicited-node multicast address.
*        It has the prefix FF02:0:0:0:0:1:FF00:0000/104 concatenated
*        with the 24 low-order bits of a corresponding IPv6 unicast
*        or anycast address.
*************************************************************************/
void _fnet_ip6_get_solicited_multicast_addr(const fnet_ip6_addr_t *ip_addr, fnet_ip6_addr_t *solicited_multicast_addr)
{
    solicited_multicast_addr->addr[0] = 0xFFu;
    solicited_multicast_addr->addr[1] = 0x02u;
    solicited_multicast_addr->addr[2] = 0x00u;
    solicited_multicast_addr->addr[3] = 0x00u;
    solicited_multicast_addr->addr[4] = 0x00u;
    solicited_multicast_addr->addr[5] = 0x00u;
    solicited_multicast_addr->addr[6] = 0x00u;
    solicited_multicast_addr->addr[7] = 0x00u;
    solicited_multicast_addr->addr[8] = 0x00u;
    solicited_multicast_addr->addr[9] = 0x00u;
    solicited_multicast_addr->addr[10] = 0x00u;
    solicited_multicast_addr->addr[11] = 0x01u;
    solicited_multicast_addr->addr[12] = 0xFFu;
    solicited_multicast_addr->addr[13] = ip_addr->addr[13];
    solicited_multicast_addr->addr[14] = ip_addr->addr[14];
    solicited_multicast_addr->addr[15] = ip_addr->addr[15];
}

/************************************************************************
* DESCRIPTION: This function frees list of datagram fragments.
*************************************************************************/
#if FNET_CFG_IP6_FRAGMENTATION  /* PFI create general library fo list, linked lists etc.*/
static void _fnet_ip6_frag_list_free( fnet_ip6_frag_list_t *list )
{
    fnet_netbuf_t *nb;

    fnet_isr_lock();

    if(list)
    {
        while((volatile fnet_ip6_frag_header_t *)(list->frag_ptr) != 0)
        {
            nb = list->frag_ptr->nb;
            _fnet_ip6_frag_del((fnet_ip6_frag_header_t **)(&list->frag_ptr), list->frag_ptr);
            _fnet_netbuf_free_chain(nb);
        }

        _fnet_ip6_frag_list_del(&ip6_frag_list_head, list);
        _fnet_free(list);
    }

    fnet_isr_unlock();
}

#endif /* FNET_CFG_IP6_FRAGMENTATION */

/************************************************************************
* DESCRIPTION: Adds frag list to the general frag list.
*************************************************************************/
#if FNET_CFG_IP6_FRAGMENTATION
static void _fnet_ip6_frag_list_add( fnet_ip6_frag_list_t **head, fnet_ip6_frag_list_t *fl )
{
    fl->next = *head;

    if(fl->next != 0)
    {
        fl->next->prev = fl;
    }

    fl->prev = 0;
    *head = fl;
}

#endif /* FNET_CFG_IP6_FRAGMENTATION */

/************************************************************************
* DESCRIPTION: Deletes frag list from the general frag list.
*************************************************************************/
#if FNET_CFG_IP6_FRAGMENTATION
static void _fnet_ip6_frag_list_del( fnet_ip6_frag_list_t **head, fnet_ip6_frag_list_t *fl )
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
#endif /* FNET_CFG_IP6_FRAGMENTATION */

/************************************************************************
* DESCRIPTION: Adds frag to the frag list.
*************************************************************************/
#if FNET_CFG_IP6_FRAGMENTATION
static void _fnet_ip6_frag_add( fnet_ip6_frag_header_t **head, fnet_ip6_frag_header_t *frag,
                                fnet_ip6_frag_header_t *frag_prev )
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
#endif /* FNET_CFG_IP6_FRAGMENTATION */

/************************************************************************
* DESCRIPTION: Deletes frag from the frag list.
*************************************************************************/
#if FNET_CFG_IP6_FRAGMENTATION
static void _fnet_ip6_frag_del( fnet_ip6_frag_header_t **head, fnet_ip6_frag_header_t *frag )
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

    _fnet_free(frag);
}
#endif /* FNET_CFG_IP6_FRAGMENTATION */

/************************************************************************
* DESCRIPTION: This function attempts to assemble a complete datagram.
*************************************************************************/
#if FNET_CFG_IP6_FRAGMENTATION
static fnet_netbuf_t *_fnet_ip6_reassembly(fnet_netif_t *netif, fnet_netbuf_t **nb_p, fnet_netbuf_t *ip6_nb, fnet_ip6_addr_t *src_ip, fnet_ip6_addr_t *dest_ip )
{
    fnet_ip6_frag_list_t        *frag_list_ptr;
    fnet_ip6_frag_header_t      *frag_ptr;
    fnet_ip6_frag_header_t      *cur_frag_ptr;
    fnet_netbuf_t               *nb = *nb_p;
    fnet_size_t                 i;
    fnet_uint8_t                next_header;
    fnet_uint32_t               id;
    fnet_uint16_t               offset;
    fnet_uint8_t                mf;
    fnet_uint16_t               total_length;
    fnet_ip6_fragment_header_t  *ip6_fragment_header;
    fnet_ip6_header_t           *iphdr = (fnet_ip6_header_t *)ip6_nb->data_ptr;


    /* For this algorithm the all datagram must reside in contiguous area of memory.*/
    if(_fnet_netbuf_pullup(nb_p, (*nb_p)->total_length) == FNET_ERR)
    {
        goto DROP_FRAG_1;
    }

    nb = *nb_p;

    /* Process fragment header.*/
    ip6_fragment_header = (fnet_ip6_fragment_header_t *)nb->data_ptr;
    next_header = ip6_fragment_header->next_header;
    id = ip6_fragment_header->id;
    offset = (fnet_uint16_t)FNET_IP6_FRAGMENT_OFFSET(ip6_fragment_header->offset_more);
    mf = (fnet_uint8_t)FNET_IP6_FRAGMENT_MF(ip6_fragment_header->offset_more);

    _fnet_netbuf_trim(nb_p, (fnet_int32_t)sizeof(fnet_ip6_fragment_header_t));
    nb = *nb_p;

    total_length = (fnet_uint16_t)nb->length;

    if(mf)
    {
        /* Fragments (except the last) must be multiples of 8 bytes */
        if ((total_length & 0x07u) != 0u)
        {
            /* If the length of a fragment, as derived from the fragment packet’s
             * Payload Length field, is not a multiple of 8 octets and the M flag
             * of that fragment is 1, then that fragment must be discarded and an
             * ICMP Parameter Problem, Code 0, message should be sent to the
             * source of the fragment, pointing to the Payload Length field of
             * the fragment packet. */
            _fnet_icmp6_error( netif, FNET_ICMP6_TYPE_PARAM_PROB, FNET_ICMP6_CODE_PP_HEADER,
                               (fnet_uint32_t)((fnet_uint32_t)(&iphdr->length) - (fnet_uint32_t)ip6_nb->data_ptr), ip6_nb ); /* TBD not tested.*/
            goto DROP_FRAG_0;
        }
    }

    /* Create fragment header.*/
    if((cur_frag_ptr = (fnet_ip6_frag_header_t *)_fnet_malloc(sizeof(fnet_ip6_frag_header_t))) == 0)
    {
        goto DROP_FRAG_1;
    }

    cur_frag_ptr->mf = mf;
    cur_frag_ptr->offset = offset;
    cur_frag_ptr->nb = nb;
    cur_frag_ptr->total_length = total_length;

    /* Liner search of the list to locate the appropriate datagram for the current fragment.*/
    for (frag_list_ptr = ip6_frag_list_head; frag_list_ptr != 0; frag_list_ptr = frag_list_ptr->next)
    {
        if( (frag_list_ptr->id == id)
            && (frag_list_ptr->next_header == next_header)
            && FNET_IP6_ADDR_EQUAL(&frag_list_ptr->source_addr, src_ip)
            && FNET_IP6_ADDR_EQUAL(&frag_list_ptr->destination_addr, dest_ip))
        {
            break;
        }
    }

    /* The first fragment of the new datagram.*/
    if(frag_list_ptr == 0)
    {
        /* Create list.*/
        if((frag_list_ptr = (fnet_ip6_frag_list_t *)_fnet_malloc_zero(sizeof(fnet_ip6_frag_list_t))) == 0)
        {
            goto DROP_FRAG_2;
        }

        _fnet_ip6_frag_list_add(&ip6_frag_list_head, frag_list_ptr);

        frag_list_ptr->ttl = (fnet_uint8_t)FNET_IP6_FRAG_TTL;
        frag_list_ptr->id = id;
        frag_list_ptr->next_header = next_header;
        FNET_IP6_ADDR_COPY(src_ip, &frag_list_ptr->source_addr);
        FNET_IP6_ADDR_COPY(dest_ip, &frag_list_ptr->destination_addr);
        frag_ptr = 0;
    }
    else
    {
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
                if(i > cur_frag_ptr->total_length)
                {
                    goto DROP_FRAG_2;
                }

                _fnet_netbuf_trim(nb_p, (fnet_int32_t)i);
                /* nb = *nb_p */
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
            _fnet_ip6_frag_del((fnet_ip6_frag_header_t **)&frag_list_ptr->frag_ptr, frag_ptr->prev);
        }
    }

    if(offset == 0u) /* First fragment */
    {
        frag_list_ptr->hdr_length = iphdr->length;
        frag_list_ptr->hdr_hop_limit = iphdr->hop_limit;
        frag_list_ptr->netif = netif;
    }

    /* Insert fragment to the list.*/
    _fnet_ip6_frag_add((fnet_ip6_frag_header_t **)(&frag_list_ptr->frag_ptr), cur_frag_ptr, frag_ptr ? frag_ptr->prev : 0);

    {
        fnet_uint16_t offset_l = 0u;

        frag_ptr = frag_list_ptr->frag_ptr;

        do
        {
            if(frag_ptr->offset != offset_l)
            {
                goto NEXT_FRAG;
            }

            offset_l += frag_ptr->total_length;
            frag_ptr = frag_ptr->next;
        }
        while (frag_ptr != frag_list_ptr->frag_ptr);
    }

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
    iphdr = (fnet_ip6_header_t *)ip6_nb->data_ptr;
    iphdr->length = fnet_htons((fnet_uint16_t)nb->total_length);
    iphdr->next_header = frag_list_ptr->next_header;

    while(frag_list_ptr->frag_ptr != 0)
    {
        _fnet_ip6_frag_del((fnet_ip6_frag_header_t **)(&frag_list_ptr->frag_ptr), frag_list_ptr->frag_ptr);
    }

    _fnet_ip6_frag_list_del(&ip6_frag_list_head, frag_list_ptr);
    _fnet_free(frag_list_ptr);

    return (nb);

DROP_FRAG_2:
    _fnet_free(cur_frag_ptr);
DROP_FRAG_1:
    _fnet_netbuf_free_chain(ip6_nb);
DROP_FRAG_0:
    _fnet_netbuf_free_chain(nb);
    return (FNET_NULL);

NEXT_FRAG:
    _fnet_netbuf_free_chain(ip6_nb);
    return (FNET_NULL);
}
#endif /* FNET_CFG_IP4_FRAGMENTATION */

/************************************************************************
* DESCRIPTION: IP timer function.
*************************************************************************/
#if FNET_CFG_IP6_FRAGMENTATION
static void _fnet_ip6_timer(fnet_uint32_t cookie)
{
    fnet_ip6_frag_list_t *frag_list_ptr;
    fnet_ip6_frag_list_t *tmp_frag_list_ptr;

    FNET_COMP_UNUSED_ARG(cookie);

    fnet_isr_lock();
    frag_list_ptr = ip6_frag_list_head;

    while(frag_list_ptr != 0)
    {
        frag_list_ptr->ttl--;

        if(frag_list_ptr->ttl == 0u)
        {
            /* If the first fragment (i.e., the one
             * with a Fragment Offset of zero) has been received, an ICMP Time
             * Exceeded -- Fragment Reassembly Time Exceeded message should be
             * sent to the source of that fragment.
             */
            if( (frag_list_ptr->frag_ptr) && (frag_list_ptr->frag_ptr->offset == 0u) )
            {
                fnet_netbuf_t              *nb_header;
                fnet_netbuf_t              *nb;
                fnet_ip6_header_t          *ip6_header;
                fnet_ip6_fragment_header_t *ip6_fragment_header;
                /*************************************
                 * Reconstact PCB for ICMP error.
                 *************************************/
                nb_header = _fnet_netbuf_new(sizeof(fnet_ip6_header_t) + sizeof(fnet_ip6_fragment_header_t), FNET_FALSE); /* Allocate a new header.*/

                if(nb_header == FNET_NULL)
                {
                    goto FREE_LIST;
                }
                nb = _fnet_netbuf_copy(frag_list_ptr->frag_ptr->nb, 0u, FNET_NETBUF_COPYALL, FNET_FALSE);
                if(nb == FNET_NULL)
                {
                    _fnet_netbuf_free(nb_header);
                    goto FREE_LIST;
                }

                nb = _fnet_netbuf_concat(nb_header, nb);

                ip6_header = (fnet_ip6_header_t *)nb->data_ptr;
                ip6_fragment_header = (fnet_ip6_fragment_header_t *)((fnet_uint32_t)ip6_header + sizeof(fnet_ip6_header_t));

                /* IPv6 header.*/
                ip6_header->version__tclass = FNET_IP6_VERSION << 4; /* PFI copy/save header*/
                ip6_header->tclass__flowl = 0u;
                ip6_header->flowl = 0u;
                ip6_header->length = frag_list_ptr->hdr_length;
                ip6_header->next_header = FNET_IP6_TYPE_FRAGMENT_HEADER;
                ip6_header->hop_limit = frag_list_ptr->hdr_hop_limit;
                FNET_IP6_ADDR_COPY(&frag_list_ptr->source_addr, &ip6_header->source_addr);
                FNET_IP6_ADDR_COPY(&frag_list_ptr->destination_addr, &ip6_header->destination_addr);

                /* Fragment header.*/

                ip6_fragment_header->next_header = frag_list_ptr->next_header;
                ip6_fragment_header->_reserved = 0u;
                ip6_fragment_header->offset_more = fnet_htons((fnet_uint16_t)frag_list_ptr->frag_ptr->mf);
                ip6_fragment_header->id = frag_list_ptr->id;

                _fnet_icmp6_error( frag_list_ptr->netif, FNET_ICMP6_TYPE_TIME_EXCEED, FNET_ICMP6_CODE_TE_FRG_REASSEMBLY,
                                   0u, nb ); /* TBD not tested.*/
            }

        FREE_LIST:
            tmp_frag_list_ptr = frag_list_ptr->next;
            _fnet_ip6_frag_list_free(frag_list_ptr);
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
*              of memory occupied by the IPv6 module.
*************************************************************************/
void _fnet_ip6_drain( void )
{
    fnet_isr_lock();

#if FNET_CFG_IP6_FRAGMENTATION

    while(((volatile fnet_ip6_frag_list_t *)ip6_frag_list_head) != 0)
    {
        _fnet_ip6_frag_list_free(ip6_frag_list_head);
    }

#endif

    while(((volatile fnet_netbuf_t *)ip6_queue.head) != 0)
    {
        _fnet_netbuf_queue_del(&ip6_queue.head, ip6_queue.head);
    }

    ip6_queue.count = 0u;

    fnet_isr_unlock();
}

/************************************************************************
* DESCRIPTION: This function retrieves the current value
*              of IPv6 socket option.
*************************************************************************/
fnet_error_t _fnet_ip6_getsockopt(struct _fnet_socket_if_t *sock, fnet_socket_options_t optname, void *optval, fnet_size_t *optlen )
{
    fnet_error_t result = FNET_ERR_OK;

    switch(optname)      /* Socket options processing. */
    {
        case IPV6_UNICAST_HOPS: /* Get IPv6 hop limit for outgoing unicast datagrams.*/
            if(*optlen < sizeof(fnet_uint32_t))
            {
                result = FNET_ERR_INVAL;
                break;
            }

            *((fnet_uint32_t *)optval) = (fnet_uint32_t)sock->options.ip6_opt.hops_unicast;
            *optlen = sizeof(fnet_uint32_t);
            break;
        case IPV6_MULTICAST_HOPS: /* Get IPv6 hop limit for outgoing multicast datagrams.*/
            if(*optlen < sizeof(fnet_uint32_t))
            {
                result = FNET_ERR_INVAL;
                break;
            }

            *((fnet_uint32_t *)optval) = (fnet_uint32_t)sock->options.ip6_opt.hops_multicast;
            *optlen = sizeof(fnet_uint32_t);
            break;
        default:
            result = FNET_ERR_NOPROTOOPT; /* The option is unknown or unsupported.*/
            break;
    }

    return result;
}

/************************************************************************
* DESCRIPTION: This function sets the value of IPv6 socket option.
*************************************************************************/
fnet_error_t _fnet_ip6_setsockopt(struct _fnet_socket_if_t *sock, fnet_socket_options_t optname, const void *optval, fnet_size_t optlen )
{
    fnet_error_t result = FNET_ERR_OK;

    switch(optname)      /* Socket options processing. */
    {
        /******************************/
        case IPV6_UNICAST_HOPS: /* Set IPv6 hop limit for outgoing unicast datagrams. */
            if(optlen < sizeof(fnet_uint32_t))
            {
                result = FNET_ERR_INVAL;
                break;
            }

            sock->options.ip6_opt.hops_unicast = (fnet_uint8_t) (*((const fnet_uint32_t *)(optval)));
            break;
        /******************************/
        case IPV6_MULTICAST_HOPS: /* Set IPv6 hop limit for outgoing multicast datagrams. */
            /* Validation.*/
            if( (optlen < sizeof(fnet_uint32_t)) || (!(sock->protocol_interface)) || (sock->protocol_interface->type != SOCK_DGRAM )  )
            {
                result = FNET_ERR_INVAL;
                break;
            }
            sock->options.ip6_opt.hops_multicast = (fnet_uint8_t) (*((const fnet_uint32_t *)(optval)));
            break;
        /******************************/
        case IPV6_JOIN_GROUP:     /* Join the socket to the supplied IPv6 multicast group on
                                   * the specified interface. */
        case IPV6_LEAVE_GROUP:    /* Drops membership to the given IPv6 multicast group and interface.*/
        {
            fnet_index_t                    i;
            fnet_ip6_multicast_list_entry_t **multicast_entry = FNET_NULL;
            const struct fnet_ipv6_mreq     *mreq = (const struct fnet_ipv6_mreq *)optval;
            fnet_netif_t                    *netif;

            if(mreq->ipv6imr_interface == 0u)
            {
                netif = _fnet_netif_get_default();
            }
            else
            {
                netif = _fnet_netif_get_by_scope_id(mreq->ipv6imr_interface);
            }

            if((optlen != sizeof(struct fnet_ipv6_mreq)) /* Check size.*/
               || (netif == FNET_NULL) /* Found IF.*/
               || (!FNET_IP6_ADDR_IS_MULTICAST(&mreq->ipv6imr_multiaddr.s6_addr)) /* Check if the address is multicast.*/
               || (!(sock->protocol_interface)) || (sock->protocol_interface->type != SOCK_DGRAM )
              )
            {
                result = FNET_ERR_INVAL;
                break;
            }

            /* Find the existing entry with same parameters (if any).*/
            for(i = 0u; i < FNET_CFG_MULTICAST_SOCKET_MAX; i++)
            {
                if( (sock->ip6_multicast_entry[i] != FNET_NULL)
                    && (sock->ip6_multicast_entry[i]->user_counter)
                    && (sock->ip6_multicast_entry[i]->netif == netif)
                    && FNET_IP6_ADDR_EQUAL(&sock->ip6_multicast_entry[i]->group_addr, &mreq->ipv6imr_multiaddr.s6_addr))
                {
                    multicast_entry = &sock->ip6_multicast_entry[i];
                    break; /* Found.*/
                }
            }

            /******************************/
            if(optname == IPV6_JOIN_GROUP)
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
                    if(sock->ip6_multicast_entry[i] == FNET_NULL)
                    {
                        multicast_entry = &sock->ip6_multicast_entry[i];
                        break; /* Found.*/
                    }
                }

                if(multicast_entry != FNET_NULL)
                {
                    *multicast_entry = _fnet_ip6_multicast_join( netif, &mreq->ipv6imr_multiaddr.s6_addr );

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
            else /* IPV6_LEAVE_GROUP */
            {
                if(multicast_entry != FNET_NULL)
                {
                    /* Leave the group.*/
                    _fnet_ip6_multicast_leave_entry(*multicast_entry);
                    *multicast_entry = FNET_NULL; /* Clear entry.*/
                }
                else
                {
                    /* Join entry is not found.*/
                    result = FNET_ERR_INVAL;
                    break;
                }
            }
        }
        break;
        /******************************/
        default:
            result = FNET_ERR_NOPROTOOPT; /* The option is unknown or unsupported. */
            break;
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Join a IPv6 multicast group. Returns pointer to the entry in
*              the multicast list, or FNET_NULL if any error.
*************************************************************************/
fnet_ip6_multicast_list_entry_t *_fnet_ip6_multicast_join(fnet_netif_t *netif, const fnet_ip6_addr_t *group_addr )
{
    fnet_index_t                    i;
    fnet_ip6_multicast_list_entry_t *result = FNET_NULL;

    /* Find existing entry or free one.*/
    for(i = 0u; i < FNET_CFG_MULTICAST_MAX; i++)
    {
        if(fnet_ip6_multicast_list[i].user_counter > 0u)
        {
            if((fnet_ip6_multicast_list[i].netif == netif) && FNET_IP6_ADDR_EQUAL(&fnet_ip6_multicast_list[i].group_addr, group_addr))
            {
                result = &fnet_ip6_multicast_list[i];
                break; /* Found.*/
            }
        }
        else /* user_counter == 0.*/
        {
            result = &fnet_ip6_multicast_list[i]; /* Save the last free one.*/
        }
    }

    if(result)
    {
        result->user_counter++; /* Increment user counter.*/

        if(result->user_counter == 1u) /* New entry.*/
        {
            FNET_IP6_ADDR_COPY(group_addr, &result->group_addr);
            result->netif = netif;

            /* Join HW interface. */
            _fnet_netif_join_ip6_multicast ( (fnet_netif_desc_t) netif, group_addr );

#if FNET_CFG_MLD
            /*
             * When a host joins a new group, it should immediately transmit a
             * Report for that group.
             * //TBD To cover the possibility of the initial Report being lost or damaged, it is
             * recommended that it be repeated once or twice after short delays.
             */
            _fnet_mld_join(netif, (fnet_ip6_addr_t *)group_addr);
#endif
        }
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Find a IPv6 multicast group entry.
*************************************************************************/
fnet_ip6_multicast_list_entry_t *_fnet_ip6_multicast_find_entry(fnet_netif_t *netif, const fnet_ip6_addr_t *group_addr )
{
    fnet_index_t                    i;
    fnet_ip6_multicast_list_entry_t *result = FNET_NULL;

    /* Find existing entry or free one.*/
    for(i = 0u; i < FNET_CFG_MULTICAST_MAX; i++)
    {
        if((fnet_ip6_multicast_list[i].user_counter > 0u)
           && (fnet_ip6_multicast_list[i].netif == netif)
           && FNET_IP6_ADDR_EQUAL(&fnet_ip6_multicast_list[i].group_addr, group_addr))
        {
            result = &fnet_ip6_multicast_list[i];
            break; /* Found.*/
        }
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Leave the IPv6 multicast group.
*************************************************************************/
void _fnet_ip6_multicast_leave(fnet_netif_t *netif, const fnet_ip6_addr_t *group_addr)
{
    fnet_ip6_multicast_list_entry_t *multicast_entry;

    multicast_entry = _fnet_ip6_multicast_find_entry(netif, group_addr);

    _fnet_ip6_multicast_leave_entry(multicast_entry);
}

/************************************************************************
* DESCRIPTION: Leave the all IPv6 multicast groups.
*************************************************************************/
void _fnet_ip6_multicast_leave_all(fnet_netif_t *netif)
{
    fnet_index_t i;

    for(i = 0u; i < FNET_CFG_MULTICAST_MAX; i++)
    {
        if((fnet_ip6_multicast_list[i].user_counter > 0u)
           && (fnet_ip6_multicast_list[i].netif == netif))
        {
            _fnet_ip6_multicast_leave_entry(&fnet_ip6_multicast_list[i]);
        }
    }
}

/************************************************************************
* DESCRIPTION: Leave a multicast group.
*************************************************************************/
void _fnet_ip6_multicast_leave_entry( fnet_ip6_multicast_list_entry_t *multicastentry )
{
    if(multicastentry && (multicastentry->user_counter))
    {
        multicastentry->user_counter--; /* Decrement user counter.*/

        if(multicastentry->user_counter == 0u)
        {
#if FNET_CFG_MLD
            /* Leave via MLD */
            _fnet_mld_leave(multicastentry->netif, &multicastentry->group_addr);
#endif

            /* Leave HW interface. */
            _fnet_netif_leave_ip6_multicast( (fnet_netif_desc_t) multicastentry->netif, &multicastentry->group_addr );
        }
    }
}

#endif /* FNET_CFG_IP6 */
