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
*  Neighbor Discovery for IP6 API.
*
***************************************************************************/

#ifndef _FNET_ND6_H_

#define _FNET_ND6_H_
#include "fnet.h"
#include "fnet_timer_prv.h"
#include "fnet_ip6.h"
#include "fnet_icmp6.h"
#include "fnet_netbuf_prv.h"

/* Neighbor Cache and Default Router List combined to one list.*/
#define FNET_ND6_NEIGHBOR_CACHE_SIZE         (FNET_CFG_ND6_NEIGHBOR_CACHE_SIZE + FNET_CFG_ND6_ROUTER_LIST_SIZE)
#define FNET_ND6_PREFIX_LIST_SIZE            (FNET_CFG_ND6_PREFIX_LIST_SIZE + 1u) /* One more for link-local prefix.*/
#define FNET_ND6_REDIRECT_TABLE_SIZE         (4U) /* TBD config parameter.*/

/**************************************************************
* RFC4861. 10. Protocol Constants.
***************************************************************
*    Host constants:
*        MAX_RTR_SOLICITATION_DELAY 1 second
*        RTR_SOLICITATION_INTERVAL 4 seconds
*        MAX_RTR_SOLICITATIONS 3 transmissions
*    Node constants:
*        MAX_MULTICAST_SOLICIT 3 transmissions
*        MAX_UNICAST_SOLICIT 3 transmissions
*        MAX_ANYCAST_DELAY_TIME 1 second
*        MAX_NEIGHBOR_ADVERTISEMENT 3 transmissions
*        REACHABLE_TIME 30,000 milliseconds
*        RETRANS_TIMER 1,000 milliseconds
*        DELAY_FIRST_PROBE_TIME 5 seconds
*        MIN_RANDOM_FACTOR .5
*        MAX_RANDOM_FACTOR 1.5
***************************************************************/

/* If a host sends MAX_RTR_SOLICITATIONS solicitations, and receives no
 * Router Advertisements after having waited MAX_RTR_SOLICITATION_DELAY
 * seconds after sending the last solicitation, the host concludes that
 * there are no routers on the link for the purpose of [ADDRCONF].
 * However, the host continues to receive and process Router
 * Advertisements messages in the event that routers appear on the link.
 *
 * A host SHOULD transmit up to MAX_RTR_SOLICITATIONS Router
 * Solicitation messages, each separated by at least
 * RTR_SOLICITATION_INTERVAL seconds.
 */
#define FNET_ND6_MAX_RTR_SOLICITATIONS          (3U)        /* transmissions */
#define FNET_ND6_MAX_RTR_SOLICITATION_DELAY_MS  (1000U)     /* ms */
#define FNET_ND6_RTR_SOLICITATION_INTERVAL_MS   (4000U)     /* ms */

/* If no Neighbor Advertisement is received after MAX_MULTICAST_SOLICIT
* solicitations, address resolution has failed. The sender MUST return
* ICMP destination unreachable indications with code 3 (Address
* Unreachable) for each packet queued awaiting address resolution.
*/
#define FNET_ND6_MAX_MULTICAST_SOLICIT       (3U)        /* transmissions */

/*
 * Default value of the time between retransmissions of Neighbor
 * Solicitation messages to a neighbor when
 * resolving the address or when probing the
 * reachability of a neighbor. Also used during Duplicate
 * Address Detection (RFC4862).
 */
#define FNET_ND6_RETRANS_TIMER_MS            (1000U)     /* ms */

/*
 * Default value of the time a neighbor is considered reachable after
 * receiving a reachability confirmation.
 *
 * This value should be a uniformly distributed
 * random value between MIN_RANDOM_FACTOR and
 * MAX_RANDOM_FACTOR times BaseReachableTime
 * milliseconds. A new random value should be
 * calculated when BaseReachableTime changes (due to
 * Router Advertisements) or at least every few
 * hours even if
 */
#define FNET_ND6_REACHABLE_TIME_MS           (30000U)    /* ms */

/*
 * If no reachability confirmation is received
 * within DELAY_FIRST_PROBE_TIME seconds of entering the
 * DELAY state, send a Neighbor Solicitation and change
 * the state to PROBE.
 */
#define FNET_ND6_DELAY_FIRST_PROBE_TIME_MS   (5000U)     /*ms*/

/*
 * If no response is
 * received after waiting RetransTimer milliseconds after sending the
 * MAX_UNICAST_SOLICIT solicitations, retransmissions cease and the
 * entry SHOULD be deleted.
 */
#define FNET_ND6_MAX_UNICAST_SOLICIT         (3U)        /*times*/

/*
 * ND6 general timer resolution.
 */
#define FNET_ND6_TIMER_PERIOD_MS             (100U)      /* ms */

#define FNET_ND6_PREFIX_LENGTH_DEFAULT       (64U)            /* Default prefix length, in bits.*/
#define FNET_ND6_PREFIX_LIFETIME_INFINITE    (0xFFFFFFFFU)    /* A lifetime value of all one bits (0xffffffff) represents infinity. */
#define FNET_ND6_RDNSS_LIFETIME_INFINITE     (0xFFFFFFFFU)    /* A lifetime value of all one bits (0xffffffff) represents infinity. */


/***********************************************************************
* Prefix state.
***********************************************************************/
typedef enum fnet_nd6_prefix_state
{
    FNET_ND6_PREFIX_STATE_NOTUSED = 0,      /* The entry is not used - free.*/
    FNET_ND6_PREFIX_STATE_USED = 1          /* The entry is used.*/
} fnet_nd6_prefix_state_t;

/***********************************************************************
* Prefix List entry, based on RFC4861.
* Prefix List entries are created from information received in Router
* Advertisements.
***********************************************************************/
typedef struct fnet_nd6_prefix_entry
{
    fnet_ip6_addr_t         prefix;         /* Prefix of an IPv6 address. */
    fnet_size_t             prefix_length;  /* Prefix length (in bits). The number of leading bits
                                             * in the Prefix that are valid. */
    fnet_nd6_prefix_state_t state;          /* Prefix state.*/
    fnet_time_t             lifetime_sec;   /* Valid Lifetime
                                             * 32-bit unsigned integer. The length of time in
                                             * seconds (relative to the time the packet is sent)
                                             * that the prefix is valid for the purpose of on-link
                                             * determination. A value of all one bits
                                             * (0xffffffff) represents infinity. The Valid
                                             * Lifetime is also used by [ADDRCONF].*/
    fnet_time_t             creation_time_sec;  /* Time of entry creation, in seconds.*/
} fnet_nd6_prefix_entry_t;

/**************************************************************
* Neighbor’s reachability states, based on RFC4861.
**************************************************************/
typedef enum fnet_nd6_neighbor_state
{
    FNET_ND6_NEIGHBOR_STATE_NOTUSED = 0,    /* The entry is not used - free.*/
    FNET_ND6_NEIGHBOR_STATE_INCOMPLETE = 1, /* Address resolution is in progress and the link-layer
                                             * address of the neighbor has not yet been determined.*/
    FNET_ND6_NEIGHBOR_STATE_REACHABLE = 2,  /* Roughly speaking, the neighbor is known to have been
                                             * reachable recently (within tens of seconds ago).*/
    FNET_ND6_NEIGHBOR_STATE_STALE = 3,      /* The neighbor is no longer known to be reachable but
                                             * until traffic is sent to the neighbor, no attempt
                                             * should be made to verify its reachability.*/
    FNET_ND6_NEIGHBOR_STATE_DELAY = 4,      /* The neighbor is no longer known to be reachable, and
                                             * traffic has recently been sent to the neighbor.
                                             * Rather than probe the neighbor immediately, however,
                                             * delay sending probes for a short while in order to
                                             * give upper-layer protocols a chance to provide
                                             * reachability confirmation.*/
    FNET_ND6_NEIGHBOR_STATE_PROBE = 5       /* The neighbor is no longer known to be reachable, and
                                             * unicast Neighbor Solicitation probes are being sent to
                                             * verify reachability.*/
} fnet_nd6_neighbor_state_t;

/***********************************************************************
* Neighbor Cache entry, based on RFC4861.
***********************************************************************/
typedef struct fnet_nd6_neighbor_entry
{

    fnet_ip6_addr_t             ip_addr;        /* Neighbor’s on-link unicast IP address. */
    fnet_netif_ll_addr_t        ll_addr;        /* Its link-layer address. Actual size is defiined by fnet_netif_api_t->netif_hw_addr_size. */
    fnet_nd6_neighbor_state_t   state;          /* Neighbor’s reachability state.*/
    fnet_time_t                 state_time_ms;  /* Time of last state event.*/
    fnet_netbuf_t               *waiting_netbuf;/* Pointer to any queued packetwaiting for address resolution to complete.*/
    /* RFC 4861 7.2.2: While waiting for address resolution to complete, the sender MUST,
     * for each neighbor, retain a small queue of packets waiting for
     * address resolution to complete. The queue MUST hold at least one
     * packet, and MAY contain more.
     * When a queue  overflows, the new arrival SHOULD replace the oldest entry.*/
    fnet_index_t                solicitation_send_counter;  /* Counter - how many soicitations where sent.*/
    fnet_ip6_addr_t             solicitation_src_ip_addr;   /* IP address used during AR solicitation messages. */
    fnet_time_t                 creation_time_sec;          /* Time of entry creation, in seconds.*/
    /* Default Router list entry info.*/
    fnet_bool_t                 is_router;              /* A flag indicating whether the neighbor is a router or a host.*/
    fnet_time_t                 router_lifetime_sec;    /* The lifetime associated
                                                        * with the default router in units of seconds. The
                                                        * field can contain values up to 65535 and receivers
                                                        * should handle any value, while the sending rules in
                                                        * Section 6 limit the lifetime to 9000 seconds. A
                                                        * Lifetime of 0 indicates that the router is not a
                                                        * default router and SHOULD NOT appear on the default router list.
                                                        * It is used only if "is_router" is 1.*/
} fnet_nd6_neighbor_entry_t;

/***********************************************************************
* Redirect Table entry.
***********************************************************************/
typedef struct fnet_nd6_redirect_entry
{
    fnet_ip6_addr_t     destination_addr;   /* Destination Address. The IP address of the destination that is
                                             * redirected to the target. */
    fnet_ip6_addr_t     target_addr;        /* Target Address. An IP address that is a better first hop to use for
                                             * the ICMP Destination Address. When the target is
                                             * the actual endpoint of communication, i.e., the
                                             * destination is a neighbor, the Target Address field
                                             * MUST contain the same value as the ICMP Destination
                                             * Address field. Otherwise, the target is a better
                                             * first-hop router and the Target Address MUST be the
                                             * router’s link-local address so that hosts can
                                             * uniquely identify routers. */
    fnet_time_t         creation_time_sec;  /* Time of entry creation.*/
} fnet_nd6_redirect_entry_t;

/***********************************************************************
* Recursive DNS Server List entry, based on RFC6106.
***********************************************************************/
typedef struct fnet_nd6_rdnss_entry
{
    fnet_ip6_addr_t             rdnss_addr;         /* IPv6 address of the Recursive
                                                    * DNS Server, which is available for recursive DNS resolution
                                                    * service in the network advertising the RDNSS option. */
    fnet_time_t               creation_time_sec;    /* Time of entry creation, in seconds.*/
    fnet_time_t               lifetime_sec;         /* The maximum time, in
                                                    * seconds (relative to the time the packet is sent),
                                                    * over which this DNSSL domain name MAY be used for
                                                    * name resolution.
                                                    * A value of all one bits (0xffffffff) represents
                                                    * infinity.  A value of zero means that the DNSSL
                                                    * domain name MUST no longer be used.*/
} fnet_nd6_rdnss_entry_t;

/**********************************************************************
* Neighbor Solicitation Message Format (RFC 4861)
***********************************************************************
* Nodes send Neighbor Solicitations to request the link-layer address
* of a target node while also providing their own link-layer address to
* the target.
*
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |     Type      |     Code      |          Checksum             |
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |                           Reserved                            |
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |                                                               |
*    +                                                               +
*    |                                                               |
*    +                       Target Address                          +
*    |                                                               |
*    +                                                               +
*    |                                                               |
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |   Options ...
*    +-+-+-+-+-+-+-+-+-+-+-+-
***********************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct fnet_nd6_ns_header
{
    fnet_icmp6_header_t icmp6_header    FNET_COMP_PACKED;
    fnet_uint8_t        _reserved[4]    FNET_COMP_PACKED;
    fnet_ip6_addr_t     target_addr     FNET_COMP_PACKED;
} fnet_nd6_ns_header_t;
FNET_COMP_PACKED_END

/**********************************************************************
* Neighbor Advertisement Message Format (RFC 4861)
***********************************************************************
* A node sends Neighbor Advertisements in response to Neighbor
* Solicitations and sends unsolicited Neighbor Advertisements in order
* to (unreliably) propagate new information quickly.
*
*	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |     Type      |     Code      |          Checksum             |
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |R|S|O|                     Reserved                            |
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |                                                               |
*    +                                                               +
*    |                                                               |
*    +                       Target Address                          +
*    |                                                               |
*    +                                                               +
*    |                                                               |
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |   Options ...
*    +-+-+-+-+-+-+-+-+-+-+-+-
***********************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct fnet_nd6_na_header
{
    fnet_icmp6_header_t icmp6_header    FNET_COMP_PACKED;
    fnet_uint8_t        flag            FNET_COMP_PACKED;
    fnet_uint8_t        _reserved[3]    FNET_COMP_PACKED;
    fnet_ip6_addr_t     target_addr     FNET_COMP_PACKED;
} fnet_nd6_na_header_t;
FNET_COMP_PACKED_END

/* NA flags.*/
#define FNET_ND6_NA_FLAG_ROUTER      (0x80U) /* Router flag. When set, the R-bit indicates that
                                             * the sender is a router. The R-bit is used by
                                             * Neighbor Unreachability Detection to detect a
                                             * router that changes to a host.*/
#define FNET_ND6_NA_FLAG_SOLICITED   (0x40U) /* Solicited flag. When set, the S-bit indicates that
                                             * the advertisement was sent in response to a
                                             * Neighbor Solicitation from the Destination address.
                                             * The S-bit is used as a reachability confirmation
                                             * for Neighbor Unreachability Detection. It MUST NOT
                                             * be set in multicast advertisements or in
                                             * unsolicited unicast advertisements.*/
#define FNET_ND6_NA_FLAG_OVERRIDE    (0x20U) /* Override flag. When set, the O-bit indicates that
                                             * the advertisement should override an existing cache
                                             * entry and update the cached link-layer address.
                                             * When it is not set the advertisement will not
                                             * update a cached link-layer address though it will
                                             * update an existing Neighbor Cache entry for which
                                             * no link-layer address is known. It SHOULD NOT be
                                             * set in solicited advertisements for anycast
                                             * addresses and in solicited proxy advertisements.
                                             * It SHOULD be set in other solicited advertisements
                                             * and in unsolicited advertisements.*/

/**********************************************************************
* Redirect Message Format (RFC 4861)
***********************************************************************
* Routers send Redirect packets to inform a host of a better first-hop
* node on the path to a destination.
*
*	 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |     Type      |     Code      |          Checksum             |
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |                           Reserved                            |
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |                                                               |
*    +                                                               +
*    |                                                               |
*    +                       Target Address                          +
*    |                                                               |
*    +                                                               +
*    |                                                               |
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |                                                               |
*    +                                                               +
*    |                                                               |
*    +                    Destination Address                        +
*    |                                                               |
*    +                                                               +
*    |                                                               |
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |   Options ...
*    +-+-+-+-+-+-+-+-+-+-+-+-
***********************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct fnet_nd6_rd_header
{
    fnet_icmp6_header_t icmp6_header        FNET_COMP_PACKED;
    fnet_uint8_t        _reserved[4]        FNET_COMP_PACKED;
    fnet_ip6_addr_t     target_addr         FNET_COMP_PACKED;
    fnet_ip6_addr_t     destination_addr    FNET_COMP_PACKED;
} fnet_nd6_rd_header_t;
FNET_COMP_PACKED_END


/**********************************************************************
* Router Solicitation Message Format
***********************************************************************
* Hosts send Router Solicitations in order to prompt routers to
* generate Router Advertisements quickly.
*
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |     Type      |     Code      |          Checksum             |
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |                           Reserved                            |
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |   Options ...
*    +-+-+-+-+-+-+-+-+-+-+-+-
***********************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct fnet_nd6_rs_header
{
    fnet_icmp6_header_t icmp6_header    FNET_COMP_PACKED;
    fnet_uint8_t        _reserved[4]    FNET_COMP_PACKED;
} fnet_nd6_rs_header_t;
FNET_COMP_PACKED_END

/**********************************************************************
* Router Advertisement Message Format
***********************************************************************
* Routers send out Router Advertisement messages periodically, or in
* response to Router Solicitations.
*
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |     Type      |     Code      |          Checksum             |
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    | Cur Hop Limit |M|O|  Reserved |       Router Lifetime         |
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |                      Reachable Time                           |
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |                      Retrans Timer                            |
*    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*    |   Options ...
*    +-+-+-+-+-+-+-+-+-+-+-+-
***********************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct fnet_nd6_ra_header
{
    fnet_icmp6_header_t icmp6_header    FNET_COMP_PACKED;       /* ICMPv6 header.*/
    fnet_uint8_t        cur_hop_limit   FNET_COMP_PACKED;       /* 8-bit unsigned integer. The default value that
                                                                * should be placed in the Hop Count field of the IP
                                                                * header for outgoing IP packets. A value of zero
                                                                * means unspecified (by this router). */
    fnet_uint8_t        flag            FNET_COMP_PACKED;       /* ND6_RS_FLAG_M and/or ND6_RS_FLAG_O flags.*/
    fnet_uint16_t       router_lifetime_sec FNET_COMP_PACKED;   /* 16-bit unsigned integer. The lifetime associated
                                                                * with the default router in units of seconds. The
                                                                * field can contain values up to 65535 and receivers
                                                                * should handle any value, while the sending rules in
                                                                * Section 6 limit the lifetime to 9000 seconds. A
                                                                * Lifetime of 0 indicates that the router is not a
                                                                * default router and SHOULD NOT appear on the default
                                                                * router list. The Router Lifetime applies only to
                                                                * the router’s usefulness as a default router; it
                                                                * does not apply to information contained in other
                                                                * message fields or options. Options that need time
                                                                * limits for their information include their own
                                                                * lifetime fields.*/
    fnet_uint32_t       reachable_time_ms  FNET_COMP_PACKED;    /* 32-bit unsigned integer. The time, in
                                                                * milliseconds, that a node assumes a neighbor is
                                                                * reachable after having received a reachability
                                                                * confirmation. Used by the Neighbor Unreachability
                                                                * Detection algorithm (see Section 7.3). A value of
                                                                * zero means unspecified (by this router). */
    fnet_uint32_t       retrans_timer_ms   FNET_COMP_PACKED;    /* 32-bit unsigned integer. The time, in
                                                                * milliseconds, between retransmitted Neighbor
                                                                * Solicitation messages. Used by address resolution
                                                                * and the Neighbor Unreachability Detection algorithm
                                                                * (see Sections 7.2 and 7.3). A value of zero means
                                                                * unspecified (by this router).*/

} fnet_nd6_ra_header_t;
FNET_COMP_PACKED_END

/* RA flags */
#define FNET_ND6_RA_FLAG_M   (0x80U) /* 1-bit "Managed address configuration" flag. When
                                     * set, it indicates that addresses are available via
                                     * Dynamic Host Configuration Protocol [DHCPv6].
                                     * If the M flag is set, the O flag is redundant and
                                     * can be ignored because DHCPv6 will return all
                                     * available configuration information.*/
#define FNET_ND6_RA_FLAG_O   (0x40U) /* 1-bit "Other configuration" flag. When set, it
                                     * indicates that other configuration information is
                                     * available via DHCPv6. Examples of such information
                                     * are DNS-related information or information on other
                                     * servers within the network.*/
/* Note: If neither M nor O flags are set, this indicates that no
 * information is available via DHCPv6.*/

/* Hop Limit when sending/receiving Neighbor Discovery messages. */
#define FNET_ND6_HOP_LIMIT                  (255U)

/* ND option types (RFC4861). */
#define FNET_ND6_OPTION_SOURCE_LLA          (1U)     /* Source Link-layer Address.*/
#define FNET_ND6_OPTION_TARGET_LLA          (2U)     /* Target Link-layer Address.*/
#define FNET_ND6_OPTION_PREFIX              (3U)     /* Prefix Information.*/
#define FNET_ND6_OPTION_REDIRECTED_HEADER   (4U)     /* Redirected Header.*/
#define FNET_ND6_OPTION_MTU                 (5U)     /* MTU. */
#define FNET_ND6_OPTION_RDNSS               (25U)    /* RDNSS RFC6106. */

/***********************************************************************
 * ND option header
 ***********************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct fnet_nd6_option_header
{
    fnet_uint8_t type      FNET_COMP_PACKED;   /* Identifier of the type of option.*/
    fnet_uint8_t length    FNET_COMP_PACKED;   /* The length of the option
                                                 * (including the type and length fields) in units of
                                                 * 8 octets.  The value 0 is invalid.  Nodes MUST
                                                 * silently discard an ND packet that contains an
                                                 * option with length zero.*/
} fnet_nd6_option_header_t;
FNET_COMP_PACKED_END

/***********************************************************************
 * Source/Target Link-layer Address option header:
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     Type      |     Length    |       Link-Layer Address ...
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 ***********************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct fnet_nd6_option_lla_header
{
    fnet_nd6_option_header_t    option_header   FNET_COMP_PACKED;   /* Option general header.*/
    fnet_uint8_t                addr[6]         FNET_COMP_PACKED;   /* The length of the option. Can be more or less than 6.*/
} fnet_nd6_option_lla_header_t;
FNET_COMP_PACKED_END

/***********************************************************************
 * MTU option header:
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     Type      |     Length    |           Reserved            |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                          MTU                                  |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 ***********************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct fnet_nd6_option_mtu_header
{
    fnet_nd6_option_header_t    option_header   FNET_COMP_PACKED;   /* Option general header.*/
    fnet_uint8_t                _reserved[2]    FNET_COMP_PACKED;
    fnet_uint32_t               mtu             FNET_COMP_PACKED;   /* The recommended MTU for the link.*/
} fnet_nd6_option_mtu_header_t;
FNET_COMP_PACKED_END

/***********************************************************************
 * Prefix Information option header:
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |     Type      |     Length    | Prefix Length |L|A| Reserved1 |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                         Valid Lifetime                        |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                       Preferred Lifetime                      |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                           Reserved2                           |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  |                                                               |
 *  +                                                               +
 *  |                                                               |
 *  +                             Prefix                            +
 *  |                                                               |
 *  +                                                               +
 *  |                                                               |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 ***********************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct fnet_nd6_option_prefix_header
{
    fnet_nd6_option_header_t    option_header   FNET_COMP_PACKED;   /* Option general header.*/
    fnet_uint8_t                prefix_length   FNET_COMP_PACKED;   /* The number of leading bits
                                                                     * in the Prefix that are valid. The value ranges
                                                                     * from 0 to 128. The prefix length field provides
                                                                     * necessary information for on-link determination
                                                                     * (when combined with the L flag in the prefix
                                                                     * information option). It also assists with address
                                                                     * autoconfiguration as specified in [ADDRCONF], for
                                                                     * which there may be more restrictions on the prefix
                                                                     * length.*/
    fnet_uint8_t                flag            FNET_COMP_PACKED;   /* ND6_OPTION_FLAG_L and/or ND6_OPTION_FLAG_O flags.*/
    fnet_uint32_t               valid_lifetime_sec  FNET_COMP_PACKED;/* The length of time in
                                                                     * seconds (relative to the time the packet is sent)
                                                                     * that the prefix is valid for the purpose of on-link
                                                                     * determination. A value of all one bits
                                                                     * (0xffffffff) represents infinity. The Valid
                                                                     * Lifetime is also used by [ADDRCONF].*/
    fnet_uint32_t               prefered_lifetime FNET_COMP_PACKED; /* The length of time in
                                                                     * seconds (relative to the time the packet is sent)
                                                                     * that addresses generated from the prefix via
                                                                     * stateless address autoconfiguration remain
                                                                     * preferred [ADDRCONF]. A value of all one bits
                                                                     * (0xffffffff) represents infinity. See [ADDRCONF].
                                                                     * Note that the value of this field MUST NOT exceed
                                                                     * the Valid Lifetime field to avoid preferring
                                                                     * addresses that are no longer valid.*/
    fnet_uint32_t               _reserved       FNET_COMP_PACKED;
    fnet_ip6_addr_t             prefix          FNET_COMP_PACKED;   /* An IP address or a prefix of an IP address. The
                                                                     * Prefix Length field contains the number of valid
                                                                     * leading bits in the prefix. The bits in the prefix
                                                                     * after the prefix length are reserved and MUST be
                                                                     * initialized to zero by the sender and ignored by
                                                                     * the receiver. A router SHOULD NOT send a prefix
                                                                     * option for the link-local prefix and a host SHOULD
                                                                     * ignore such a prefix option.*/

} fnet_nd6_option_prefix_header_t;
FNET_COMP_PACKED_END


#define FNET_ND6_OPTION_FLAG_L  (0x80U)  /* 1-bit on-link flag. When set, indicates that this
                                         * prefix can be used for on-link determination. When
                                         * not set the advertisement makes no statement about
                                         * on-link or off-link properties of the prefix. In
                                         * other words, if the L flag is not set a host MUST
                                         * NOT conclude that an address derived from the
                                         * prefix is off-link. That is, it MUST NOT update a
                                         * previous indication that the address is on-link.*/
#define FNET_ND6_OPTION_FLAG_A  (0x40U)  /* 1-bit autonomous address-configuration flag. When
                                         * set indicates that this prefix can be used for
                                         * stateless address configuration as specified in
                                         * [ADDRCONF].*/

/***********************************************************************
 * Recursive DNS Server header (RFC 6106):
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |     Type      |     Length    |           Reserved            |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |                           Lifetime                            |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |                                                               |
 *   :            Addresses of IPv6 Recursive DNS Servers            :
 *   |                                                               |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 ***********************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct fnet_nd6_option_rdnss_header
{
    fnet_nd6_option_header_t    option_header   FNET_COMP_PACKED;   /* Option general header.*/
    fnet_uint16_t               _reserved       FNET_COMP_PACKED;
    fnet_uint32_t               lifetime_sec    FNET_COMP_PACKED;   /* The maximum time, in
                                                                     * seconds (relative to the time the packet is sent),
                                                                     * over which this RDNSS address MAY be used for name
                                                                     * resolution.*/
    fnet_ip6_addr_t             address[1]      FNET_COMP_PACKED;   /* One or more 128-bit IPv6 addresses of the recursive
                                                                     * DNS servers.  The number of addresses is determined
                                                                     * by the Length field.  That is, the number of
                                                                     * addresses is equal to (Length - 1) / 2.*/
} fnet_nd6_option_rdnss_header_t;
FNET_COMP_PACKED_END


/***********************************************************************
* Neighbor Descovery Configuration
***********************************************************************/
typedef struct fnet_nd6_if
{
    /*************************************************************
    * Neighbor Cache.
    * RFC4861 5.1: A set of entries about individual neighbors to
    * which traffic has been sent recently.
    **************************************************************/
    /*************************************************************
    * Combined with Default Router List.
    * RFC4861 5.1: A list of routers to which packets may be sent..
    **************************************************************/
    fnet_nd6_neighbor_entry_t  neighbor_cache[FNET_ND6_NEIGHBOR_CACHE_SIZE];

    /*************************************************************
    * Prefix List.
    * RFC4861 5.1: A list of the prefixes that define a set of
    * addresses that are on-link.
    **************************************************************/
    fnet_nd6_prefix_entry_t     prefix_list[FNET_ND6_PREFIX_LIST_SIZE];

    /* Redirect Table. Used only when target address != destination address. */
    fnet_nd6_redirect_entry_t   redirect_table[FNET_ND6_REDIRECT_TABLE_SIZE];

#if FNET_CFG_ND6_RDNSS && FNET_CFG_DNS
    fnet_nd6_rdnss_entry_t      rdnss_list[FNET_CFG_ND6_RDNSS_LIST_SIZE];
#endif

    fnet_timer_desc_t           timer;                  /* General ND timer.*/

    /* Router Discovery variables.*/
    fnet_index_t                rd_transmit_counter;    /* Counter used by RD. Equals to the number
                                                         * of RS transmits till RD is finished.*/
    fnet_time_t                 rd_time_ms;             /* Time of last RD transmit.*/

    /* Interface variables */
    fnet_size_t                 mtu;                    /* The recommended MTU for the link.
                                                         * Updated by RA messages.*/
    fnet_uint8_t                cur_hop_limit;          /* The default value that
                                                         * should be placed in the Hop Count field of the IP
                                                         * header for outgoing IP packets.*/
    fnet_time_t                 reachable_time_ms;      /* The time, in milliseconds,
                                                         * that a node assumes a neighbor is
                                                         * reachable after having received a reachability
                                                         * confirmation. Used by the Neighbor Unreachability
                                                         * Detection algorithm.*/
    fnet_time_t                 retrans_timer_ms;       /* The time, in milliseconds,
                                                         * between retransmitted Neighbor
                                                         * Solicitation messages. Used by address resolution
                                                         * and the Neighbor Unreachability Detection algorithm
                                                         * (see Sections 7.2 and 7.3).*/
    fnet_bool_t                 ip6_disabled;           /* IP operation on the interface is disabled.*/
} fnet_nd6_if_t;

/* Forward declaration.*/
struct fnet_netif;
struct fnet_netif_ip6_addr;
/***********************************************************************
* Function Prototypes
***********************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

fnet_return_t _fnet_nd6_init (struct fnet_netif *netif, fnet_nd6_if_t *nd6_if_ptr);
void _fnet_nd6_release (struct fnet_netif *netif);
fnet_bool_t _fnet_nd6_addr_is_onlink(struct fnet_netif *netif, const fnet_ip6_addr_t *addr);
fnet_nd6_neighbor_entry_t *_fnet_nd6_neighbor_cache_get(struct fnet_netif *netif, const fnet_ip6_addr_t *ip_addr);
void _fnet_nd6_neighbor_cache_del(struct fnet_netif *netif, fnet_nd6_neighbor_entry_t *neighbor_entry);
fnet_nd6_neighbor_entry_t *_fnet_nd6_neighbor_cache_add(struct fnet_netif *netif, const fnet_ip6_addr_t *ip_addr, fnet_netif_ll_addr_t ll_addr, fnet_nd6_neighbor_state_t state);
void _fnet_nd6_neighbor_enqueue_waiting_netbuf(fnet_nd6_neighbor_entry_t *neighbor_entry, fnet_netbuf_t *waiting_netbuf);
fnet_nd6_neighbor_entry_t *_fnet_nd6_default_router_get(struct fnet_netif *netif);
void _fnet_nd6_neighbor_solicitation_send(struct fnet_netif *netif, const fnet_ip6_addr_t *ipsrc /* FNET_NULL for, DAD */, const fnet_ip6_addr_t *ipdest /*set for NUD,  FNET_NULL for DAD & AR */, const fnet_ip6_addr_t *target_addr);
void _fnet_nd6_neighbor_solicitation_receive(struct fnet_netif *netif, fnet_ip6_addr_t *src_ip, fnet_ip6_addr_t *dest_ip, fnet_netbuf_t *nb, fnet_netbuf_t *ip6_nb);
void _fnet_nd6_neighbor_advertisement_receive(struct fnet_netif *netif, fnet_ip6_addr_t *src_ip, fnet_ip6_addr_t *dest_ip, fnet_netbuf_t *nb, fnet_netbuf_t *ip6_nb);
void _fnet_nd6_router_advertisement_receive(struct fnet_netif *netif, fnet_ip6_addr_t *src_ip, fnet_ip6_addr_t *dest_ip, fnet_netbuf_t *nb, fnet_netbuf_t *ip6_nb);
void _fnet_nd6_redirect_receive(struct fnet_netif *netif, fnet_ip6_addr_t *src_ip, fnet_ip6_addr_t *dest_ip, fnet_netbuf_t *nb, fnet_netbuf_t *ip6_nb);
void _fnet_nd6_redirect_addr(struct fnet_netif *if_ptr, const fnet_ip6_addr_t **destination_addr_p);
void _fnet_nd6_dad_start(struct fnet_netif *netif, struct fnet_netif_ip6_addr *addr_info);
void _fnet_nd6_rd_start(struct fnet_netif *netif);
void _fnet_nd6_debug_print_prefix_list(struct fnet_netif *netif);
#if FNET_CFG_ND6_RDNSS && FNET_CFG_DNS
fnet_bool_t _fnet_nd6_rdnss_get_addr(struct fnet_netif *netif, fnet_index_t n, fnet_ip6_addr_t *addr_dns);
#endif

#if defined(__cplusplus)
}
#endif

#endif /* _FNET_ND6_H_ */
