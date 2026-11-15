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
*  Private. IPv6 protocol API.
*
***************************************************************************/

#ifndef _FNET_IP6_PRV_H_

#define _FNET_IP6_PRV_H_

#include "fnet.h"
#include "fnet_ip6.h"
#include "fnet_netif.h"
#include "fnet_netif_prv.h"

/************************************************************************
*    Definitions.
*************************************************************************/
#define FNET_IP6_HOPLIMIT_DEFAULT       (64U)  /* Default hop-limit */
#define FNET_IP6_MAX_PACKET             (FNET_CFG_IP_MAX_PACKET)

/************************************************************************
*    IP implementation parameters.
*************************************************************************/
#define FNET_IP6_VERSION                (6U)   /* IPv6 version. */
#define FNET_IP6_HOP_LIMIT_MAX          (255U) /* Maximum hop limit. */
#define FNET_IP6_HOP_LIMIT_DEFAULT      (64U)  /* Default hop limit. */

#define FNET_IP6_TIMER_PERIOD           (500U)

/* Time to live of a datagram awaiting reassembly (no relation to the IP TTL) */
/* RFC 2460: If insufficient fragments are received to complete reassembly of a
 * packet within 60 seconds of the reception of the first-arriving
 * fragment of that packet, reassembly of that packet must be
 * abandoned and all the fragments that have been received for that
 * packet must be discarded.*/
#define FNET_IP6_FRAG_TTL        (60000U/FNET_IP6_TIMER_PERIOD) /* TTL for fragments to complete a datagram (60sec)*/

/* RFC4484:
 *  Multicast destination addresses have a 4-bit scope field that
 *  controls the propagation of the multicast packet.  The IPv6
 *  addressing architecture defines scope field values for interface-
 *  local (0x1), link-local (0x2), subnet-local (0x3), admin-local (0x4),
 *  site-local (0x5), organization-local (0x8), and global (0xE)
 *  scopes [11].
 */
#define FNET_IP6_ADDR_SCOPE_INTERFACELOCAL (0x01)
#define FNET_IP6_ADDR_SCOPE_LINKLOCAL      (0x02)
#define FNET_IP6_ADDR_SCOPE_SITELOCAL      (0x05)
#define FNET_IP6_ADDR_SCOPE_ORGLOCAL       (0x08)
#define FNET_IP6_ADDR_SCOPE_GLOBAL         (0x0e)

#define FNET_IP6_ADDR_MULTICAST_SCOPE(a) ((a)->addr[1] & 0x0fu)

/**************************************************************************/ /*!
 * @internal
 * @brief    IPv6 layer socket options.
 ******************************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct
{
    fnet_uint8_t   hops_unicast   FNET_COMP_PACKED;   /**< Unicast hop limit.*/
    fnet_uint8_t   hops_multicast FNET_COMP_PACKED;   /**< Multicast hop limit.*/
} fnet_ip6_sockopt_t;
FNET_COMP_PACKED_END

/*********************************************************************
* IP packet header
**********************************************************************
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |Version| Traffic Class |           Flow Label                  |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |         Payload Length        |  Next Header  |   Hop Limit   |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |                                                               |
*   +                                                               +
*   |                                                               |
*   +                         Source Address                        +
*   |                                                               |
*   +                                                               +
*   |                                                               |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |                                                               |
*   +                                                               +
*   |                                                               |
*   +                      Destination Address                      +
*   |                                                               |
*   +                                                               +
*   |                                                               |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
**********************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct
{
    fnet_uint8_t   version__tclass     FNET_COMP_PACKED;   /* 4-bit Internet Protocol version number = 6, 8-bit traffic class field. */
    fnet_uint8_t   tclass__flowl       FNET_COMP_PACKED;   /* 20-bit flow label. */
    fnet_uint16_t   flowl               FNET_COMP_PACKED;
    fnet_uint16_t   length              FNET_COMP_PACKED;   /* Length of the IPv6
								                             * payload, i.e., the rest of the packet following
							                                 * this IPv6 header, in octets. */
    fnet_uint8_t   next_header         FNET_COMP_PACKED;   /* Identifies the type of header
                        	    	                         * immediately following the IPv6 header.  Uses the
                        		                             * same values as the IPv4 Protocol field [RFC-1700
                        		                             * et seq.].*/
    fnet_uint8_t   hop_limit           FNET_COMP_PACKED;   /* Decremented by 1 by
                        		                             * each node that forwards the packet. The packet
                        		                             * is discarded if Hop Limit is decremented to
                        		                             * zero. */
    fnet_ip6_addr_t source_addr         FNET_COMP_PACKED;   /* 128-bit address of the originator of the packet. */
    fnet_ip6_addr_t destination_addr    FNET_COMP_PACKED;   /* 128-bit address of the intended recipient of the
								                             * packet (possibly not the ultimate recipient, if
								                             * a Routing header is present). */
} fnet_ip6_header_t;
FNET_COMP_PACKED_END

#define FNET_IP6_HEADER_GET_VERSION(x)                   (((x)->version__tclass & 0xF0u)>>4)

/******************************************************************
* Extension header types
*******************************************************************/
#define FNET_IP6_TYPE_HOP_BY_HOP_OPTIONS                     (0U)
#define FNET_IP6_TYPE_DESTINATION_OPTIONS                    (60U)
#define FNET_IP6_TYPE_ROUTING_HEADER                         (43U)
#define FNET_IP6_TYPE_FRAGMENT_HEADER                        (44U)
#define FNET_IP6_TYPE_AUTHENTICATION_HEADER                  (51U)
#define FNET_IP6_TYPE_ENCAPSULATION_SECURITY_PAYLOAD_HEADER  (50U)
#define FNET_IP6_TYPE_MOBILITY_HEADER                        (135U)
#define FNET_IP6_TYPE_NO_NEXT_HEADER                         (59U)   /* RFC 2460: The value 59 in the Next Header field of an IPv6 header or any
                                                                    * extension header indicates that there is nothing following that
                                                                    * header. If the Payload Length field of the IPv6 header indicates the
                                                                    * presence of octets past the end of a header whose Next Header field
                                                                    * contains 59, those octets must be ignored.*/

/***********************************************************************
 * Hop-by-Hop Options Header & Destination Options Header
 *
 * The Hop-by-Hop Options header is used to carry optional information
 * that must be examined by every node along a packet’s delivery path.
 *
 * The Destination Options header is used to carry optional information
 * that need be examined only by a packet’s destination node(s).
 ***********************************************************************
 * RFC 2460 4.3/4.6:
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  | Next Header   | Hdr Ext Len   |                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               +
 *  |                                                               |
 *  .                                                               .
 *  .                         Options                               .
 *  .                                                               .
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 ***********************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct fnet_ip6_options_header
{
    fnet_uint8_t   next_header     FNET_COMP_PACKED;   /* 8-bit selector. Identifies the type of header
                                                         * immediately following the Options
                                                         * header. */
    fnet_uint8_t   hdr_ext_length  FNET_COMP_PACKED;   /* 8-bit unsigned integer. Length of the Hop-by-
                                                         * Hop Options header in 8-octet units, not
                                                         * including the first 8 octets. */
    fnet_uint8_t   options[6]      FNET_COMP_PACKED;   /* Variable-length field, of length such that the
                                                         * complete Options header is an integer
                                                         * multiple of 8 octets long. */
} fnet_ip6_options_header_t;
FNET_COMP_PACKED_END

/***********************************************************************
 * Options (used in op-by-Hop Options Header & Destination Options Header)
 ***********************************************************************
 * RFC 2460 4.2:
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- - - - - - - - -
 * | Option Type   | Opt Data Len  | Option Data
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- - - - - - - - -
 ***********************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct fnet_ip6_option_header
{
    fnet_uint8_t    type           FNET_COMP_PACKED;   /* 8-bit identifier of the type of option. */
    fnet_uint8_t    data_length    FNET_COMP_PACKED;   /* 8-bit unsigned integer. Length of the Option
                                                         * Data field of this option, in octets. */
} fnet_ip6_option_header_t;
FNET_COMP_PACKED_END

/* RFC 2460 4.2: There are two padding options which are used when necessary to align
 * subsequent options and to pad out the containing header to a multiple
 * of 8 octets in length. */

#define FNET_IP6_OPTION_TYPE_PAD1   (0x00u)  /* The Pad1 option is used to insert 
                                             * one octet of padding into the Options area of a header.*/
#define FNET_IP6_OPTION_TYPE_PADN   (0x01u)  /* The PadN option is used to insert two or more octets of padding
                                             * into the Options area of a header. For N octets of padding, the
                                             * Opt Data Len field contains the value N-2, and the Option Data
                                             * consists of N-2 zero-valued octets. */

#define FNET_IP6_OPTION_TYPE_ROUTER_ALERT   (0x05u) /* RFC2711:The presence of this option in an IPv6 datagram informs the router
                                                    * that the contents of this datagram is of interest to the router and
                                                    * to handle any control data accordingly.*/
#define FNET_IP6_OPTION_TYPE_ROUTER_ALERT_VALUE_MLD (0x0000u)    /* RFC2711: Datagram contains a Multicast 
                                                                 * Listener Discovery message [RFC-2710].*/


/* RFC 2460: The Option Type identifiers are internally encoded such that their
 * highest-order two bits specify the action that must be taken if the
 * processing IPv6 node does not recognize the Option Type:*/
#define FNET_IP6_OPTION_TYPE_UNRECOGNIZED_MASK          (0xC0u)

#define FNET_IP6_OPTION_TYPE_UNRECOGNIZED_SKIP          (0x00u)  /* 00 - skip over this option and continue processing the header.*/
#define FNET_IP6_OPTION_TYPE_UNRECOGNIZED_DISCARD       (0x40u)  /* 01 - discard the packet. */
#define FNET_IP6_OPTION_TYPE_UNRECOGNIZED_DISCARD_ICMP  (0x80u)  /* 10 - discard the packet and, regardless of whether or not the
                                                                 * packet’s Destination Address was a multicast address, send an
                                                                 * ICMP Parameter Problem, Code 2, message to the packet’s
                                                                 * Source Address, pointing to the unrecognized Option Type.*/
#define FNET_IP6_OPTION_TYPE_UNRECOGNIZED_DISCARD_UICMP (0xC0u)  /* 11 - discard the packet and, only if the packet’s Destination
                                                                 * Address was not a multicast address, send an ICMP Parameter
                                                                 * Problem, Code 2, message to the packet’s Source Address,
                                                                 * pointing to the unrecognized Option Type.*/

/***********************************************************************
 * Routing Header
 *
 * The Routing header is used by an IPv6 source to list one or more
 * intermediate nodes to be "visited" on the way to a packet’s
 * destination.
 ***********************************************************************
 * RFC 2460 4.4:
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  | Next Header   | Hdr Ext Len   | Routing Type  | Segments Left |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                                                               |
 *  .                                                               .
 *  .                       type-specific data                      .
 *  .                                                               .
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 ***********************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct fnet_ip6_routing_header
{
    fnet_uint8_t   next_header     FNET_COMP_PACKED;   /* 8-bit selector. Identifies the type of header
                                                         * immediately following the Options
                                                         * header. */
    fnet_uint8_t   hdr_ext_length  FNET_COMP_PACKED;   /* 8-bit unsigned integer. Length of the Hop-by-
                                                         * Hop Options header in 8-octet units, not
                                                         * including the first 8 octets. */
    fnet_uint8_t   routing_type    FNET_COMP_PACKED;   /* 8-bit identifier of a particular Routing header
                                                         * variant.*/
    fnet_uint8_t   segments_left   FNET_COMP_PACKED;   /* 8-bit unsigned integer. Number of route
                                                         * segments remaining, i.e., number of explicitly
                                                         * listed intermediate nodes still to be visited
                                                         * before reaching the final destination.*/
    fnet_uint8_t    data[4]         FNET_COMP_PACKED;   /* Variable-length field, of format determined by
                                                         * the Routing Type, and of length such that the
                                                         * complete Routing header is an integer multiple
                                                         * of 8 octets long. */
} fnet_ip6_routing_header_t;
FNET_COMP_PACKED_END



/* Time to live of a datagram awaiting reassembly (no relation to the IP TTL) */
/* RFC 2460: If insufficient fragments are received to complete reassembly of a
 * packet within 60 seconds of the reception of the first-arriving
 * fragment of that packet, reassembly of that packet must be
 * abandoned and all the fragments that have been received for that
 * packet must be discarded.*/
#define FNET_IP6REASM_TTL           60000U       /* 60 secs */

/***********************************************************************
 * Fragment Header
 ***********************************************************************
 * RFC 2460 4.5:
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |  Next Header  |   Reserved    |      Fragment Offset    |Res|M|
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         Identification                        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 ***********************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct fnet_ip6_fragment_header
{
    fnet_uint8_t    next_header    FNET_COMP_PACKED;   /* 8-bit selector.  Identifies the initial header
                                                         * type of the Fragmentable Part of the original
                                                         * packet (defined below).  Uses the same values as
                                                         * the IPv4 Protocol field [RFC-1700 et seq.]. */
    fnet_uint8_t    _reserved      FNET_COMP_PACKED;   /* Initialized to zero for
                                                         * transmission; ignored on reception. */
    fnet_uint16_t    offset_more    FNET_COMP_PACKED;   /* @ 13-bit unsigned integer.  The offset, in 8-octet
                                                         * units, of the data following this header,
                                                         * relative to the start of the Fragmentable Part
                                                         * of the original packet.
                                                         * @ 2-bit reserved field.  Initialized to zero for
                                                         * transmission; ignored on reception.
                                                         * @ 1 = more fragments; 0 = last fragment.*/
    fnet_uint32_t    id             FNET_COMP_PACKED;   /* Identification. */
} fnet_ip6_fragment_header_t;
FNET_COMP_PACKED_END

#define FNET_IP6_FRAGMENT_MF_MASK               (0x1u)     /* If 1, this is not last fragment */
#define FNET_IP6_FRAGMENT_MF(offset_more)       (fnet_ntohs(offset_more)&FNET_IP6_FRAGMENT_MF_MASK)
#define FNET_IP6_FRAGMENT_OFFSET_MASK           (0xFFF8u)  /* The offset of frag in dgram */
#define FNET_IP6_FRAGMENT_OFFSET(offset_more)   (fnet_ntohs(offset_more)&FNET_IP6_FRAGMENT_OFFSET_MASK)        /* The offset of frag in dgram */

/**************************************************************************/ /*!
 * @internal
 * @brief    Structure of IP fragment header.
 ******************************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct fnet_ip6_frag_header
{
    fnet_uint8_t    mf                      FNET_COMP_PACKED;
    fnet_uint16_t   total_length            FNET_COMP_PACKED;   /**< data-payload total length (Host endian)*/
    fnet_uint16_t   offset                  FNET_COMP_PACKED;   /**< offset field (measured in 8-byte order). (Host endian)*/
    fnet_netbuf_t   *nb                     FNET_COMP_PACKED;
    struct fnet_ip6_frag_header *next       FNET_COMP_PACKED;   /**< Pointer to the next fragment.*/
    struct fnet_ip6_frag_header *prev       FNET_COMP_PACKED;   /**< Pointer to the previous fragment.*/
} fnet_ip6_frag_header_t;
FNET_COMP_PACKED_END

/**************************************************************************/ /*!
 * @internal
 * @brief    Structure of the head of each reassembly list.
 ******************************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct fnet_ip6_frag_list
{
    struct fnet_ip6_frag_list   *next               FNET_COMP_PACKED;   /**< Pointer to the next reassembly list.*/
    struct fnet_ip6_frag_list   *prev               FNET_COMP_PACKED;   /**< Pointer to the previous reassembly list.*/
    fnet_uint8_t                ttl                 FNET_COMP_PACKED;   /**< TTL for reassembly.*/
    fnet_uint8_t                next_header         FNET_COMP_PACKED;   /**< protocol.*/
    fnet_uint32_t               id                  FNET_COMP_PACKED;   /**< identification.*/
    fnet_ip6_addr_t             source_addr         FNET_COMP_PACKED;   /**< source address.*/
    fnet_ip6_addr_t             destination_addr    FNET_COMP_PACKED;   /**< destination address.*/
    fnet_uint16_t               hdr_length          FNET_COMP_PACKED;   /* Length of the IPv6 payload of first packet (for ICMPv6 error).*/
    fnet_uint8_t                hdr_hop_limit       FNET_COMP_PACKED;
    fnet_netif_t                *netif              FNET_COMP_PACKED;
    fnet_ip6_frag_header_t      *frag_ptr           FNET_COMP_PACKED;   /**< Pointer to the first fragment of the list.*/
} fnet_ip6_frag_list_t;
FNET_COMP_PACKED_END

/******************************************************************************
 * Multicast related structures.
 ******************************************************************************/

/* Entry of the IPv6multicast group list.*/
typedef struct fnet_ip6_multicast_list_entry
{
    fnet_netif_t    *netif;         /* Interface to join on. */
    fnet_ip6_addr_t group_addr;     /* IPv6 address of joined multicast group. */
    fnet_index_t    user_counter;   /* User counter. Keeps a reference count of the number
                                     * of requests to join a particular host group. */
} fnet_ip6_multicast_list_entry_t;

/* Global IPv6 multicast list.*/
extern fnet_ip6_multicast_list_entry_t fnet_ip6_multicast_list[FNET_CFG_MULTICAST_MAX];


/************************************************************************
*     Function Prototypes
*************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

fnet_return_t _fnet_ip6_init(void);
void _fnet_ip6_release(void);
void _fnet_ip6_input(fnet_netif_t *netif, fnet_netbuf_t *nb);
void _fnet_ip6_get_solicited_multicast_addr(const fnet_ip6_addr_t *ip_addr, fnet_ip6_addr_t *solicited_multicast_addr);
fnet_int32_t _fnet_ip6_addr_scope(const fnet_ip6_addr_t *ip_addr);
fnet_bool_t _fnet_ip6_addr_pefix_cmp(const fnet_ip6_addr_t *addr_1, const fnet_ip6_addr_t *addr_2, fnet_size_t prefix_length);
fnet_size_t _fnet_ip6_common_prefix_length(const fnet_ip6_addr_t *ip_addr_1, const fnet_ip6_addr_t *ip_addr_2);
const fnet_ip6_addr_t *_fnet_ip6_select_src_addr(fnet_netif_t *netif /* Optional.*/, const fnet_ip6_addr_t *dest_addr);
fnet_error_t _fnet_ip6_output(fnet_netif_t *netif /*optional*/, const fnet_ip6_addr_t *src_ip /*optional*/, const fnet_ip6_addr_t *dest_ip,
                              fnet_uint8_t protocol, fnet_uint8_t hop_limit /*optional*/, fnet_netbuf_t *nb, FNET_COMP_PACKED_VAR fnet_uint16_t *checksum);
void _fnet_ip6_drain(void);
fnet_size_t _fnet_ip6_mtu(fnet_netif_t *netif);
fnet_netif_t *_fnet_ip6_route(const fnet_ip6_addr_t *src_ip /*optional*/, const fnet_ip6_addr_t *dest_ip);
fnet_bool_t _fnet_ip6_will_fragment( fnet_netif_t *netif, fnet_size_t protocol_message_size);
struct _fnet_socket_if_t; /* Forward declaration.*/
fnet_error_t _fnet_ip6_getsockopt(struct _fnet_socket_if_t *sock, fnet_socket_options_t optname, void *optval, fnet_size_t *optlen);
fnet_error_t _fnet_ip6_setsockopt(struct _fnet_socket_if_t *sock, fnet_socket_options_t optname, const void *optval, fnet_size_t optlen );
fnet_ip6_multicast_list_entry_t *_fnet_ip6_multicast_join(fnet_netif_t *netif, const fnet_ip6_addr_t *group_addr);
void _fnet_ip6_multicast_leave_entry(fnet_ip6_multicast_list_entry_t *multicastentry);
void _fnet_ip6_multicast_leave(fnet_netif_t *netif, const fnet_ip6_addr_t *group_addr);
fnet_ip6_multicast_list_entry_t *_fnet_ip6_multicast_find_entry(fnet_netif_t *netif, const fnet_ip6_addr_t *group_addr);
void _fnet_ip6_multicast_leave_all(fnet_netif_t *netif);
void _fnet_ip6_set_socket_addr(fnet_netif_t *netif, fnet_ip6_header_t *ip_hdr, struct fnet_sockaddr *src_addr,  struct fnet_sockaddr *dest_addr );

#if defined(__cplusplus)
}
#endif

#endif /* _FNET_IP6_PRV_H_ */
