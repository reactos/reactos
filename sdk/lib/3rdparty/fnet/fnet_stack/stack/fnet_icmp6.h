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
*  Private. ICMPv6 protocol function definitions, data structures, etc.
*
***************************************************************************/
#ifndef _FNET_ICMP6_H_
#define _FNET_ICMP6_H_

#include "fnet.h"
#include "fnet_netbuf_prv.h"

/************************************************************************
*     Definition of type and code field values.
*************************************************************************/

/******************************************************************
* ICMPv6 message types (RFC 4443)
******************************************************************/

/* ICMPv6 error messages:*/
#define FNET_ICMP6_TYPE_DEST_UNREACH                (1u)     /* Destination Unreachable. */
#define FNET_ICMP6_TYPE_PACKET_TOOBIG               (2u)     /* Packet Too Big. */
#define FNET_ICMP6_TYPE_TIME_EXCEED                 (3u)     /* Time Exceeded. */
#define FNET_ICMP6_TYPE_PARAM_PROB                  (4u)     /* Parameter Problem. */

/* ICMPv6 informational messages:*/
#define FNET_ICMP6_TYPE_ECHO_REQ                    (128u)   /* Echo Request. */
#define FNET_ICMP6_TYPE_ECHO_REPLY                  (129u)	/* Echo Reply. */

/* MLD messages (RFC2710):*/
#define FNET_ICMP6_TYPE_MULTICAST_LISTENER_QUERY    (130u)   /* Multicast Listener Query */
#define FNET_ICMP6_TYPE_MULTICAST_LISTENER_REPORT   (131u)   /* Multicast Listener Report */
#define FNET_ICMP6_TYPE_MULTICAST_LISTENER_DONE     (132u)   /* Multicast Listener Done */

/*  Neighbor Discovery defines five different ICMP packet types (RFC4861):*/
#define FNET_ICMP6_TYPE_ROUTER_SOLICITATION         (133u)   /* Router Solicitation. */
#define FNET_ICMP6_TYPE_ROUTER_ADVERTISEMENT        (134u)   /* Router Advertisement. */
#define FNET_ICMP6_TYPE_NEIGHBOR_SOLICITATION       (135u)   /* Neighbor Solicitation. */
#define FNET_ICMP6_TYPE_NEIGHBOR_ADVERTISEMENT      (136u)   /* Neighbor Advertisement. */
#define FNET_ICMP6_TYPE_REDIRECT                    (137u)   /* Redirect.*/

/* Destination Unreachable codes */
#define FNET_ICMP6_CODE_DU_NO_ROUTE                 (0u)     /* No route to destination. */
#define FNET_ICMP6_CODE_DU_ADMIN_PROHIBITED         (1u)     /* Communication with destination administratively prohibited. */
#define FNET_ICMP6_CODE_DU_BEYOND_SCOPE             (2u)     /* Beyond scope of source address.*/
#define FNET_ICMP6_CODE_DU_ADDR_UNREACH             (3u)     /* Address unreachable.*/
#define FNET_ICMP6_CODE_DU_PORT_UNREACH             (4u)     /* Port unreachable.*/
#define FNET_ICMP6_CODE_DU_ADDR_FAILED              (5u)     /* Source address failed ingress/egress policy.*/
#define FNET_ICMP6_CODE_DU_REJECT_ROUTE             (6u)     /* Reject route to destination.*/

/* Packet Too Big codes */
#define FNET_ICMP6_CODE_PTB                         (0u)

/* Time Exceeded codes */
#define FNET_ICMP6_CODE_TE_HOP_LIMIT                (0u)     /* Hop limit exceeded in transit.*/
#define FNET_ICMP6_CODE_TE_FRG_REASSEMBLY           (1u)     /* Fragment reassembly time exceeded.*/

/* Parameter Problem codes */
#define FNET_ICMP6_CODE_PP_HEADER                   (0u)     /* Erroneous header field encountered.*/
#define FNET_ICMP6_CODE_PP_NEXT_HEADER              (1u)     /* Unrecognized Next Header type encountered.*/
#define FNET_ICMP6_CODE_PP_OPTION                   (2u)     /* Unrecognized IPv6 option encountered.*/

/* The Hop Limit of ICMPv6 messages.*/
#define FNET_ICMP6_HOP_LIMIT                        (FNET_IP6_HOP_LIMIT_DEFAULT)

/*
* Error messages are identified as such by a
* zero in the high-order bit of their message Type field values. Thus,
* error messages have message types from 0 to 127; informational
* messages have message types from 128 to 255.
*/
#define FNET_ICMP6_TYPE_IS_ERROR(t) (((t) & 0x80u) == 0x00u)

/***********************************************************************
 * Generic ICMP packet header
 ***********************************************************************
 *
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |     Type      |     Code      |          Checksum             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                                                               |
 * +                         Message Body                          +
 * |                                                               |
 *
 ***********************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct
{
    fnet_uint8_t  type     FNET_COMP_PACKED;   /* The type of the message.*/
    fnet_uint8_t  code     FNET_COMP_PACKED;   /* The code of the message.*/
    fnet_uint16_t  checksum FNET_COMP_PACKED;   /* The checksum of the message.*/
} fnet_icmp6_header_t;
FNET_COMP_PACKED_END

/***********************************************************************
 * ICMPv6 Echo packet
 ***********************************************************************
 * RFC4443 4:
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |    Type       |       Code    |             Checksum          |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |             Identifier        |       Sequence Number         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | Data ...
 * +-+-+-+-+-
 ***********************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct fnet_icmp6_echo_header
{
    fnet_icmp6_header_t  icmp6_header    FNET_COMP_PACKED;
    fnet_uint16_t        id              FNET_COMP_PACKED;
    fnet_uint16_t        seq_number      FNET_COMP_PACKED;
} fnet_icmp6_echo_header_t;
FNET_COMP_PACKED_END

/***********************************************************************
 * ICMPv6 Error packet
 ***********************************************************************/
typedef struct fnet_icmp6_err_header
{
    fnet_icmp6_header_t  icmp6_header    FNET_COMP_PACKED;
    fnet_uint32_t        data            FNET_COMP_PACKED;
} fnet_icmp6_err_header_t;

extern struct fnet_prot_if fnet_icmp6_prot_if;

/************************************************************************
*     Function Prototypes
*************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif
struct fnet_netif; /* Forward declaration.*/
void _fnet_icmp6_error(struct fnet_netif *netif, fnet_uint8_t type, fnet_uint8_t code, fnet_uint32_t param, fnet_netbuf_t *origin_nb);
void _fnet_icmp6_output(struct fnet_netif *netif, const fnet_ip6_addr_t *src_ip, const fnet_ip6_addr_t *dest_ip, fnet_uint8_t hop_limit, fnet_netbuf_t *nb );
#if defined(__cplusplus)
}
#endif

#endif
