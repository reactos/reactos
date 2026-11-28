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
*  Private. FNET Network interface API.
*
***************************************************************************/

#ifndef _FNET_NETIF_PRV_H_

#define _FNET_NETIF_PRV_H_

#include "fnet.h"
#include "fnet_netbuf_prv.h"
#include "fnet_nd6.h"

/**************************************************************************/ /*!
 * @internal
 * @brief    Netif features.
 ******************************************************************************/
typedef enum
{
    FNET_NETIF_FEATURE_NONE                     = 0x00, /* No special feature.*/
    FNET_NETIF_FEATURE_HW_TX_IP_CHECKSUM        = 0x01, /* If an IP frame is transmitted, the checksum is inserted automatically. The IP header checksum field
                                                         * must be cleared. If a non-IP frame is transmitted the frame is not modified.*/
    FNET_NETIF_FEATURE_HW_TX_PROTOCOL_CHECKSUM  = 0x02, /* If an IP frame with a known protocol is transmitted (UDP,TCP,ICMP), the checksum is inserted automatically into the
                                                         * frame. The checksum field must be cleared. The other frames are not modified.*/
    FNET_NETIF_FEATURE_HW_RX_IP_CHECKSUM        = 0x04, /* If an IPv4 frame is received with a mismatching header checksum,
                                                         * the frame is discarded.*/
    FNET_NETIF_FEATURE_HW_RX_PROTOCOL_CHECKSUM  = 0x08  /* If a TCP/IP, UDP/IP, or ICMP/IP frame is received that has a wrong TCP, UDP, or ICMP checksum,
                                                         * the frame is discarded.*/
} fnet_netif_feature_t;

/**************************************************************************/ /*!
 * Interface IPv4 address structure.
 ******************************************************************************/
typedef struct
{
    fnet_ip4_addr_t address;            /**< The IP address.*/
    fnet_ip4_addr_t net;                /**< Network address.*/
    fnet_ip4_addr_t netmask;            /**< Network mask.*/
    fnet_ip4_addr_t subnet;             /**< Network and subnet address.*/
    fnet_ip4_addr_t subnetmask;         /**< Network and subnet mask.*/
    fnet_ip4_addr_t netbroadcast;       /**< Network broadcast address.*/
    fnet_ip4_addr_t subnetbroadcast;    /**< Subnet broadcast address.*/
    fnet_ip4_addr_t gateway;            /**< Gateway.*/
#if FNET_CFG_DNS
    fnet_ip4_addr_t dns;                /**< DNS address.*/
#endif
    fnet_netif_ip_addr_type_t address_type;
} fnet_netif_ip4_addr_t;

/* Maxinmum number of IPv6 addresses per interface.*/
#define FNET_NETIF_IP6_ADDR_MAX                 FNET_CFG_NETIF_IP6_ADDR_MAX
/* A lifetime value of all one bits (0xffffffff) represents infinity. */
#define FNET_NETIF_IP6_ADDR_LIFETIME_INFINITE   FNET_ND6_PREFIX_LIFETIME_INFINITE

/*********************************************************************
 * Interface IPv6 address structure.
 *********************************************************************/
typedef struct fnet_netif_ip6_addr
{
    fnet_ip6_addr_t             address;                    /* IPv6 address.*/
    fnet_netif_ip6_addr_state_t state;                      /* Address current state.*/
    fnet_netif_ip_addr_type_t   type;                       /* How the address was acquired.*/
    fnet_ip6_addr_t             solicited_multicast_addr;   /* Solicited-node multicast */

    fnet_time_t                 creation_time_sec;      /* Time of entry creation (in seconds).*/
    fnet_time_t                 lifetime_sec;           /* Address lifetime (in seconds). 0xFFFFFFFF = Infinite Lifetime
                                                         * RFC4862. A link-local address has an infinite preferred and valid lifetime; it
                                                         * is never timed out.*/
    fnet_size_t                 prefix_length;          /* Prefix length (in bits). The number of leading bits
                                                         * in the Prefix that are valid. */
    fnet_index_t                dad_transmit_counter;   /* Counter used by DAD. Equals to the number
                                                         * of NS transmits till DAD is finished.*/
    fnet_time_t                 state_time_ms;          /* Time of last state event.*/
} fnet_netif_ip6_addr_t;

struct fnet_netif; /* Forward declaration.*/
struct fnet_eth_api;
struct fnet_wifi_api;
/**************************************************************************/ /*!
 * @internal
 * @brief    Network-interface general API structure.
 ******************************************************************************/
typedef struct fnet_netif_api
{
    fnet_netif_type_t   netif_type;                                            /* Data-link type. */
    fnet_size_t         netif_hw_addr_size;                                    /* HW address size.*/
    fnet_return_t       (*netif_init)( struct fnet_netif *netif );             /* Initialization function.*/
    void                (*netif_release)( struct fnet_netif *netif );          /* Shutdown/release function.*/
#if FNET_CFG_IP4
    void                (*netif_output_ip4)(struct fnet_netif *netif, fnet_ip4_addr_t dest_ip_addr, fnet_netbuf_t *nb); /* IPv4 Transmit function.*/
#endif
    void                (*netif_change_addr_notify)( struct fnet_netif *netif );    /* Address change notification function.*/
    void                (*netif_drain)( struct fnet_netif *netif );                 /* Memory drain function.*/
    fnet_return_t       (*netif_get_hw_addr)( struct fnet_netif *netif, fnet_uint8_t *hw_addr); /* Get HW address (Optional).*/
    fnet_return_t       (*netif_set_hw_addr)( struct fnet_netif *netif, fnet_uint8_t *hw_addr); /* Set HW address (Optional). */
    fnet_bool_t         (*netif_is_connected)( struct fnet_netif *netif );                      /* Connection state flag (Optional).*/
    fnet_return_t       (*netif_get_statistics)( struct fnet_netif *netif, struct fnet_netif_statistics *statistics ); /* Get statistics */
#if FNET_CFG_MULTICAST
#if FNET_CFG_IP4
    void                (*netif_multicast_join_ip4)( struct fnet_netif *netif, fnet_ip4_addr_t multicast_addr  );        /* Join IPv4 multicast group */
    void                (*netif_multicast_leave_ip4)( struct fnet_netif *netif, fnet_ip4_addr_t multicast_addr  );       /* Leave IPv4 multicast group */
#endif
#if FNET_CFG_IP6
    void                (*netif_multicast_join_ip6)( struct fnet_netif *netif, const fnet_ip6_addr_t *multicast_addr  ); /* Join IPv4 multicast group */
    void                (*netif_multicast_leave_ip6)( struct fnet_netif *netif, fnet_ip6_addr_t *multicast_addr  );      /* Leave IPv6 multicast group */
#endif
#endif
#if FNET_CFG_IP6
    void                (*netif_output_ip6)(struct fnet_netif *netif, const fnet_ip6_addr_t *src_ip_addr,  const fnet_ip6_addr_t *dest_ip_addr, fnet_netbuf_t *nb); /* IPv6 Transmit function.*/
#endif
    union                /* Points to interface specific API structure (Optional). */
    {
        const struct fnet_wifi_api   *wifi_api;  /* netif_type = FNET_NETIF_TYPE_WIFI */
        const struct fnet_eth_api   *eth_api;   /* netif_type = FNET_NETIF_TYPE_ETHERNET */
        /* Put here new type-specific APIs */
    };
} fnet_netif_api_t;

/* Forward declaration.*/
struct fnet_nd6_if;
struct fnet_arp_if;

/**************************************************************************/ /*!
 * @internal
 * @brief    Network interface structure.
 ******************************************************************************/
typedef struct fnet_netif
{
    fnet_char_t             netif_name[FNET_NETIF_NAMELEN];     /* Network interface name (e.g. "eth0", "loop"). */
    fnet_size_t             netif_mtu;                          /* Maximum transmission unit. */
    void                    *netif_prv;                         /* Points to interface specific control data structure (Optional). */
    const fnet_netif_api_t  *netif_api;                         /* Pointer to Interface API structure.*/

    /* Privat structure fields.*/
    struct fnet_netif       *next;                              /* Pointer to the next net_if structure. */
    struct fnet_netif       *prev;                              /* Pointer to the previous net_if structure. */
    fnet_scope_id_t         scope_id;                           /* Scope zone index, defining network interface. Used by IPv6 sockets.*/
    fnet_uint32_t           features;                           /* Supported features. Bitwise of fnet_netif_feature_t.*/
    fnet_bool_t             is_connected;                       /* Connection state, updated by _fnet_netif_is_connected() call.*/
    fnet_time_t             is_connected_timestamp_ms;          /* The timestamp, in milliseconds, when is_connected updated last time.*/
#if FNET_CFG_IP4
    fnet_netif_ip4_addr_t   ip4_addr;                           /* The interface IPv4 address structure. */
    fnet_bool_t             ip4_addr_conflict;                  /* Flag if the ip4_addr is duplicated.*/
    struct fnet_arp_if      *arp_if_ptr;                        /* Pointer to the ARP structure, if the interface supports ARP. */
#endif
#if FNET_CFG_IP6
    fnet_netif_ip6_addr_t   ip6_addr[FNET_NETIF_IP6_ADDR_MAX];  /* The interface IPv6 address structure. */
    struct fnet_nd6_if      *nd6_if_ptr;                        /* Pointer to the ND structure, if the interface supports ND. */
#if FNET_CFG_MLD
    fnet_bool_t             mld_invalid;                        /* Flag that, MLD message was sent with the unspecified address.
                                                                 * Once a valid link-local address is available, a node SHOULD generate
                                                                 * new MLD Report messages for all multicast addresses joined on the
                                                                 * interface.*/
#endif
#if FNET_CFG_IP6_PMTU_DISCOVERY
    fnet_size_t             pmtu;                               /* Path MTU, changed by Path MTU Discovery for IPv6. If 0 - is disabled.*/
    fnet_time_t             pmtu_timestamp_ms;                  /* The timestamp, in milliseconds, when PMTU was changed last time.*/
    fnet_timer_desc_t       pmtu_timer;                         /* PMTU timer,used to detect increases in PMTU.*/
#endif
#endif /* FNET_CFG_IP6 */
} fnet_netif_t;

/************************************************************************
*     Global Data Structures
*************************************************************************/
extern fnet_netif_t *fnet_netif_list;       /* The list of network interfaces.*/
extern fnet_netif_t *fnet_netif_default;    /* Default net_if. */

/************************************************************************
*     Function Prototypes
*************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

void _fnet_netif_release(fnet_netif_t *netif);
fnet_bool_t _fnet_netif_is_initialized(fnet_netif_t *netif);
void _fnet_netif_release_all( void );
void _fnet_netif_drain( void );
fnet_bool_t _fnet_netif_is_connected(fnet_netif_t *netif);
fnet_netif_t *_fnet_netif_get_default( void );
fnet_netif_t *_fnet_netif_get_by_number(fnet_index_t n);
fnet_netif_t *_fnet_netif_get_by_ip4_addr(fnet_ip4_addr_t addr);
fnet_netif_t *_fnet_netif_get_by_sockaddr( const struct fnet_sockaddr *addr );
fnet_return_t _fnet_netif_get_hw_addr(fnet_netif_t *netif, fnet_uint8_t *hw_addr, fnet_size_t hw_addr_size);
fnet_netif_desc_t _fnet_netif_get_by_scope_id(fnet_scope_id_t scope_id);
fnet_return_t _fnet_netif_init(fnet_netif_t *netif, fnet_uint8_t *hw_addr, fnet_size_t hw_addr_size );

#if FNET_CFG_IP4
void _fnet_netif_signal_ip4_addr_conflict( fnet_netif_desc_t netif );
void _fnet_netif_set_ip4_addr(fnet_netif_t *netif, fnet_ip4_addr_t ipaddr, fnet_ip4_addr_t subnet_mask);
#if FNET_CFG_MULTICAST
void _fnet_netif_join_ip4_multicast (fnet_netif_desc_t netif_desc, fnet_ip4_addr_t multicast_addr);
void _fnet_netif_leave_ip4_multicast (fnet_netif_desc_t netif_desc, fnet_ip4_addr_t multicast_addr);
#endif
#endif

fnet_return_t _fnet_netif_set_hw_addr(fnet_netif_desc_t netif_desc, fnet_uint8_t *hw_addr, fnet_size_t hw_addr_size);

#if FNET_CFG_IP6
fnet_netif_ip6_addr_t *_fnet_netif_get_ip6_addr_info(fnet_netif_t *netif, const fnet_ip6_addr_t *ip_addr);
fnet_return_t _fnet_netif_bind_ip6_addr(fnet_netif_t *netif, const fnet_ip6_addr_t *addr, fnet_netif_ip_addr_type_t addr_type,
                                        fnet_time_t lifetime_sec, fnet_size_t prefix_length /* bits */ );
fnet_return_t _fnet_netif_unbind_ip6_addr ( fnet_netif_t *netif, fnet_netif_ip6_addr_t *if_addr );
fnet_bool_t _fnet_netif_is_my_ip6_addr(fnet_netif_t *netif, const fnet_ip6_addr_t *ip_addr);
fnet_netif_t *_fnet_netif_get_by_ip6_addr(const fnet_ip6_addr_t *ip_addr );
fnet_bool_t _fnet_netif_is_my_ip6_solicited_multicast_addr(fnet_netif_t *netif, fnet_ip6_addr_t *ip_addr);
void _fnet_netif_ip6_addr_timer ( fnet_netif_t *netif);
fnet_return_t _fnet_netif_set_ip6_addr_autoconf(fnet_netif_t *netif, fnet_ip6_addr_t *ip_addr);
fnet_ip6_addr_t *_fnet_netif_get_ip6_addr_valid_link_local (fnet_netif_t *netif);
#if FNET_CFG_IP6_PMTU_DISCOVERY
void _fnet_netif_pmtu_init(fnet_netif_t *netif);
void _fnet_netif_pmtu_release(fnet_netif_t *netif);
void _fnet_netif_set_pmtu(fnet_netif_t *netif, fnet_size_t pmtu);
#endif
void _fnet_netif_join_ip6_multicast (fnet_netif_desc_t netif_desc, const fnet_ip6_addr_t *multicast_addr);
void _fnet_netif_leave_ip6_multicast (fnet_netif_desc_t netif_desc, fnet_ip6_addr_t *multicast_addr);
#endif /* FNET_CFG_IP6 */

#if defined(__cplusplus)
}
#endif

#endif /* _FNET_NETIF_PRV_H_ */
