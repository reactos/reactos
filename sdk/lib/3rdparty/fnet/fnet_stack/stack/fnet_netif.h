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
*  FNET Network interface API.
*
***************************************************************************/

#ifndef _FNET_NETIF_H_

#define _FNET_NETIF_H_

#include "fnet_ip4.h"
#include "fnet_ip6.h"
#include "fnet_socket.h"

/*! @addtogroup fnet_netif
* The Network Interface API allows an application to control
* various interface parameters, such as the IP address, the gateway address,
* the subnet mask, and others.
*/
/*! @{ */

/**************************************************************************/ /*!
 * @brief  Network interface types.
 ******************************************************************************/
typedef enum
{
    FNET_NETIF_TYPE_OTHER = 0,  /**< @brief Unspecified interface.*/
    FNET_NETIF_TYPE_ETHERNET,   /**< @brief Ethernet interface.*/
    FNET_NETIF_TYPE_WIFI,       /**< @brief Wi-Fi interface.*/
    FNET_NETIF_TYPE_LOOPBACK    /**< @brief Loopback interface.*/
} fnet_netif_type_t;

/**************************************************************************/ /*!
 * @brief  Network interface statistics, used by the @ref fnet_netif_get_statistics().
 ******************************************************************************/
struct fnet_netif_statistics
{
    fnet_uint32_t tx_packet; /**< @brief Tx packet count.*/
    fnet_uint32_t rx_packet; /**< @brief Rx packet count.*/
};

/**************************************************************************/ /*!
 * @brief The maximum length of a network interface name.
 ******************************************************************************/
#define FNET_NETIF_NAMELEN  (8U)

/**************************************************************************/ /*!
 * @brief Network interface descriptor.
 ******************************************************************************/
typedef void *fnet_netif_desc_t;

#define FNET_NETIF_LL_ADDR_MAX          (16)
/**************************************************************************/ /*!
 * @brief Link-layer address.
 * For example, Ethernet interface uses the address with size set to 6.
 ******************************************************************************/
typedef fnet_uint8_t fnet_netif_ll_addr_t[FNET_NETIF_LL_ADDR_MAX];

/**************************************************************************/ /*!
 * @brief Copying Link-layer address.
 ******************************************************************************/
#define FNET_NETIF_LL_ADDR_COPY(from_addr, to_addr, ll_size)   \
    (fnet_memcpy(&(to_addr)[0], &(from_addr)[0], (ll_size)))

/**************************************************************************/ /*!
 * @brief Check Link-layer address Equality.
 ******************************************************************************/
#define FNET_NETIF_LL_ADDR_ARE_EQUAL(a, b, size)               \
    (fnet_memcmp(&(a)[0], &(b)[0], (size)) == 0)

/**************************************************************************/ /*!
 * @brief Possible IPv6 address states.
 * @see fnet_netif_get_ip6_addr(), fnet_netif_ip6_addr_info
 ******************************************************************************/
typedef enum
{
    FNET_NETIF_IP6_ADDR_STATE_NOT_USED = 0,     /**< @brief Not used.*/
    FNET_NETIF_IP6_ADDR_STATE_TENTATIVE = 1,    /**< @brief Tentative address - (RFC4862) an address whose uniqueness on a link is being
                                                 * verified, prior to its assignment to an interface. A tentative
                                                 * address is not considered assigned to an interface in the usual
                                                 * sense. An interface discards received packets addressed to a
                                                 * tentative address, but accepts Neighbor Discovery packets related
                                                 * to Duplicate Address Detection for the tentative address.*/
    FNET_NETIF_IP6_ADDR_STATE_PREFERRED = 2 	/**< @brief Preferred address - (RFC4862) an address assigned to an interface whose use by
                                                 * upper-layer protocols is unrestricted. Preferred addresses may be
                                                 * used as the source (or destination) address of packets sent from
                                                 * (or to) the interface.*/
} fnet_netif_ip6_addr_state_t;

/**************************************************************************/ /*!
 * @brief Possible IPv6 address types.
 * @see fnet_netif_get_ip6_addr(), fnet_netif_ip6_addr_info
 ******************************************************************************/
typedef enum
{
    FNET_NETIF_IP_ADDR_TYPE_MANUAL = 0,            /**< @brief The address is set manually.*/
    FNET_NETIF_IP_ADDR_TYPE_AUTOCONFIGURABLE = 1,  /**< @brief The address is set using "Auto-IP" link-local autoconfiguration. */
    FNET_NETIF_IP_ADDR_TYPE_DHCP = 2               /**< @brief The address is set using DHCP. */
} fnet_netif_ip_addr_type_t;

/**************************************************************************/ /*!
 * @brief Interface IPv6 address information structure.
 * @see fnet_netif_get_ip6_addr()
 ******************************************************************************/
typedef struct fnet_netif_ip6_addr_info
{
    fnet_ip6_addr_t             address;            /**< @brief IPv6 address.*/
    fnet_netif_ip6_addr_state_t state;              /**< @brief Address current state.*/
    fnet_netif_ip_addr_type_t   type;               /**< @brief How the address was acquired.*/
} fnet_netif_ip6_addr_info_t;

/**************************************************************************/ /*!
 * @brief Interface IPv6 Prefix structure.
 * @see fnet_netif_get_ip6_prefix()
 ******************************************************************************/
typedef struct fnet_netif_ip6_prefix
{
    fnet_ip6_addr_t         prefix;         /**< @brief  Prefix of an IPv6 address. */
    fnet_size_t             prefix_length;  /**< @brief  Prefix length (in bits). The number of leading bits
                                             * in the Prefix that are valid. */
} fnet_netif_ip6_prefix_t;

/**************************************************************************/ /*!
 * @brief Interface IPv6 Neighbor Cache structure.
 * @see fnet_netif_get_ip6_neighbor_cache()
 ******************************************************************************/
typedef struct fnet_netif_ip6_neighbor_cache
{
    fnet_ip6_addr_t         ip_addr;        /**< @brief Neighbor’s on-link unicast IP address. */
    fnet_netif_ll_addr_t    ll_addr;        /**< @brief Its link-layer address. Actual size is defiined by ll_addr_size. */
    fnet_size_t             ll_addr_size;   /**< @brief Size of link-layer address.*/
    fnet_bool_t             is_router;      /**< @brief A flag indicating whether the neighbor is a router (FNET_TRUE) or a host (FNET_FALSE).*/
} fnet_netif_ip6_neighbor_cache_t;

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************************************************/ /*!
 *
 * @brief    Looks for a network interface according to the specified name.
 *
 *
 * @param name       The name string of a network interface.@n Maximum length of
 *                   the interface name is defined by the @ref FNET_NETIF_NAMELEN.
 *
 * @return This function returns:
 *   - Network interface descriptor that matches the @c name parameter.
 *   - @ref FNET_NULL if there is no match.
 *
 * @see fnet_netif_get_by_ip4_addr()
 *
 ******************************************************************************
 *
 * This function scans the global network interface list looking for a network
 * interface matching the @c name parameter (for example "eth0", "loop").
 *
 ******************************************************************************/
fnet_netif_desc_t fnet_netif_get_by_name( const fnet_char_t *name );

/***************************************************************************/ /*!
 *
 * @brief    Looks for a network interface according to its number.
 *
 * @param n       Number of a network interface (from zero).
 *
 * @return This function returns:
 *   - Network interface descriptor that matches the n-th interface.
 *   - @ref FNET_NULL if n-th interface is not available.
 *
 * @see fnet_netif_get_by_name()
 *
 ******************************************************************************
 *
 * This function scans the global network interface list looking for a network
 * interface matching the @c n parameter.
 *
 ******************************************************************************/
fnet_netif_desc_t fnet_netif_get_by_number( fnet_index_t n );

/***************************************************************************/ /*!
 *
 * @brief    Looks for a network interface according to the specified IPv4 address.
 *
 *
 * @param addr       The IPv4 address of a network interface.
 *
 * @return This function returns:
 *   - Network interface descriptor that matches the @c addr parameter.
 *   - @ref FNET_NULL if there is no match.
 *
 * @see fnet_netif_get_by_name()
 *
 ******************************************************************************
 *
 * This function scans the global network interface list looking for a network
 * interface matching the @c addr parameter.
 *
 ******************************************************************************/
fnet_netif_desc_t fnet_netif_get_by_ip4_addr( fnet_ip4_addr_t addr );

/***************************************************************************/ /*!
 *
 * @brief    Retrieves a name of the specified network interface.
 *
 *
 * @param netif_desc          Network interface descriptor.
 *
 * @param name           String buffer that receives a name of the
 *                       interface @c netif_desc.
 *
 * @param name_size      Size of the @c name buffer. @n Maximum length of
 *                       the interface name is defined by the @ref FNET_NETIF_NAMELEN.
 *
 * @see fnet_netif_get_by_name()
 *
 ******************************************************************************
 *
 * This function retrieves the name of the specified network interface
 * @c netif_desc and stores it in the @c name (for example "eth0", "loop").
 *
 ******************************************************************************/
void fnet_netif_get_name( fnet_netif_desc_t netif_desc, fnet_char_t *name, fnet_size_t name_size );

/***************************************************************************/ /*!
 *
 * @brief    Assigns the default network interface.
 *
 *
 * @param netif_desc     Network interface descriptor to be assigned as default.
 *
 * @see fnet_netif_get_default()
 *
 ******************************************************************************
 *
 * This function sets the @c netif as the default network interface.@n
 * @n
 * By default, during the FNET stack initialization, the default network interface is
 * assigned to the Ethernet.
 *
 ******************************************************************************/
void fnet_netif_set_default( fnet_netif_desc_t netif_desc );

/***************************************************************************/ /*!
 *
 * @brief    Retrieves the default network interface.
 *
 *
 * @return   This function returns the descriptor of the default network interface.
 *
 * @see fnet_netif_set_default()
 *
 ******************************************************************************
 *
 * This function returns the descriptor of the default network interface.@n
 * @n
 * By default, during the FNET stack initialization, the default network interface is
 * assigned to the Ethernet.
 *
 ******************************************************************************/
fnet_netif_desc_t fnet_netif_get_default( void );

/***************************************************************************/ /*!
 *
 * @brief    Sets the IPv4 address of the specified network interface.
 *
 * @param netif_desc     Network interface descriptor.
 *
 * @param ipaddr        The IPv4 address of the network interface.
 *
 * @param subnet_mask   The subnet mask of the network interface.
 *
 * @see fnet_netif_get_ip4_addr(), fnet_netif_get_ip4_subnet_mask
 *
 ******************************************************************************
 *
 * This function sets the IPv4 address and the subnet mask of the @c netif interface.@n
 * If subnet_mask is @ref INADDR_ANY, the function makes a recalculation of the subnet mask automatically.
 *
 ******************************************************************************/
void fnet_netif_set_ip4_addr( fnet_netif_desc_t netif_desc, fnet_ip4_addr_t ipaddr, fnet_ip4_addr_t subnet_mask);

/***************************************************************************/ /*!
 *
 * @brief    Retrieves an IPv4 address of the specified network interface.
 *
 * @param netif_desc  Network interface descriptor.
 *
 * @return       This function returns the IPv4 address of the @c netif interface.
 *
 * @see fnet_netif_set_ip4_addr()
 *
 ******************************************************************************
 *
 * This function returns the IPv4 address of the @c netif interface.
 *
 ******************************************************************************/
fnet_ip4_addr_t fnet_netif_get_ip4_addr( fnet_netif_desc_t netif_desc );

/***************************************************************************/ /*!
 *
 * @brief    Retrieves a subnet mask of the specified network interface.
 *
 * @param netif_desc  Network interface descriptor.
 *
 * @return       This function returns the subnet mask of the @c netif
 *               interface.
 *
 * @see fnet_netif_set_ip4_addr()
 *
 ******************************************************************************
 *
 * This function returns the subnet mask of the @c netif interface.
 *
 ******************************************************************************/
fnet_ip4_addr_t fnet_netif_get_ip4_subnet_mask( fnet_netif_desc_t netif_desc );

/***************************************************************************/ /*!
 *
 * @brief    Sets the gateway IP address of the specified network interface.
 *
 * @param netif_desc     Network interface descriptor.
 *
 * @param gw        The gateway IP address of the network interface.
 *
 * @see fnet_netif_get_ip4_gateway()
 *
 ******************************************************************************
 *
 * This function sets the gateway IP address of the @c netif interface
 * to the @c gw value.
 *
 ******************************************************************************/
void fnet_netif_set_ip4_gateway( fnet_netif_desc_t netif_desc, fnet_ip4_addr_t gw );

/***************************************************************************/ /*!
 *
 * @brief    Retrieves a gateway IPv4 address of the specified network interface.
 *
 * @param netif_desc  Network interface descriptor.
 *
 * @return       This function returns the gateway IPv4 address of the @c netif
 *               interface.
 *
 * @see fnet_netif_set_ip4_gateway()
 *
 ******************************************************************************
 *
 * This function returns the gateway IPv address of the @c netif interface.
 *
 ******************************************************************************/
fnet_ip4_addr_t fnet_netif_get_ip4_gateway( fnet_netif_desc_t netif_desc );


#if FNET_CFG_DNS || defined(__DOXYGEN__)
/***************************************************************************/ /*!
 *
 * @brief    Sets the DNS server IPv4 address of the specified network interface.
 *
 * @param netif_desc     Network interface descriptor.
 *
 * @param dns       The DNS server IPv4 address of the network interface.
 *
 * @see fnet_netif_get_ip4_dns(), FNET_CFG_DNS
 *
 ******************************************************************************
 *
 * This function sets the DNS server IPv4 address of the @c netif interface
 * to the @c dns value. @n
 * It is present only if @ref FNET_CFG_DNS is set to 1.
 *
 ******************************************************************************/
void fnet_netif_set_ip4_dns( fnet_netif_desc_t netif_desc, fnet_ip4_addr_t dns );

/***************************************************************************/ /*!
 *
 * @brief    Retrieves the DNS server IP address of the specified network interface.
 *
 * @param netif_desc  Network interface descriptor.
 *
 * @return       This function returns the DNS server IP address of the @c netif
 *               interface.
 *
 * @see fnet_netif_set_ip4_dns(), fnet_netif_set_ip6_dns(), FNET_CFG_DNS
 *
 ******************************************************************************
 *
 * This function is used to retrieve all DNS IPv4 addresses registered with
 * the given interface. @n
 * It is present only if @ref FNET_CFG_DNS is set to 1.
 *
 ******************************************************************************/
fnet_ip4_addr_t fnet_netif_get_ip4_dns( fnet_netif_desc_t netif_desc );

#endif /*FNET_CFG_DNS*/

/***************************************************************************/ /*!
 *
 * @brief    Sets the hardware address of the specified network interface.
 *
 * @param netif_desc     Network interface descriptor.
 *
 * @param hw_addr        Buffer containing the hardware address
 *                      (for the @ref FNET_NETIF_TYPE_ETHERNET and FNET_NETIF_TYPE_WIFI interface type,
 *                       it contains the MAC address).
 *
 * @param hw_addr_size   Size of the hardware address in the @c hw_addr (for @ref
 *                       FNET_NETIF_TYPE_ETHERNET and FNET_NETIF_TYPE_WIFI interface types, it
 *                       equals @c 6).
 *
 * @return  @ref FNET_OK if no error occurs.
 * @return  @ref FNET_ERR if an error occurs.
 *
 * @see fnet_netif_get_hw_addr()
 *
 ******************************************************************************
 *
 * This function sets the hardware address of the @c netif interface
 * to the @c hw_addr value.@n
 * For @ref FNET_NETIF_TYPE_ETHERNET and FNET_NETIF_TYPE_WIFI interface types, this hardware address is
 * the MAC address.
 *
 ******************************************************************************/
fnet_return_t fnet_netif_set_hw_addr( fnet_netif_desc_t netif_desc, fnet_uint8_t *hw_addr, fnet_size_t hw_addr_size );

/***************************************************************************/ /*!
 *
 * @brief    Retrieves a hardware address of the specified network interface.
 *
 * @param netif_desc     Network interface descriptor.
 *
 * @param hw_addr        Buffer that receives a hardware address
 *                      (for @ref FNET_NETIF_TYPE_ETHERNET and and FNET_NETIF_TYPE_WIFI interface types,
 *                       it will contain the MAC address).
 *
 * @param hw_addr_size   Size of the hardware address in the @c hw_addr (for the @ref
 *                       FNET_NETIF_TYPE_ETHERNET and and FNET_NETIF_TYPE_WIFI interface types, it
 *                       equals @c 6).
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs.
 *
 * @see fnet_netif_set_hw_addr()
 *
 ******************************************************************************
 *
 * This function reads the hardware address of the @c netif interface
 * and puts it into the @c hw_addr buffer.@n
 * For the @ref FNET_NETIF_TYPE_ETHERNET interface type, this hardware address is
 * the MAC address.
 *
 ******************************************************************************/
fnet_return_t fnet_netif_get_hw_addr( fnet_netif_desc_t netif_desc, fnet_uint8_t *hw_addr, fnet_size_t hw_addr_size );

/***************************************************************************/ /*!
 *
 * @brief    Retrieves the type of the specified network interface.
 *
 * @param netif_desc  Network interface descriptor.
 *
 * @return       This function returns the type of the @c netif
 *               interface.@n
 *               The type is defined by the @ref fnet_netif_type_t.
 *
 ******************************************************************************
 *
 * This function returns the type of the @c netif interface that is defined
 * by the @ref fnet_netif_type_t.
 *
 ******************************************************************************/
fnet_netif_type_t fnet_netif_get_type( fnet_netif_desc_t netif_desc );

/***************************************************************************/ /*!
 *
 * @brief    Determines the way IPv4 address parameters were obtained.
 *
 * @param netif_desc  Network interface descriptor.
 *
 * @return        This function returns type of IPv4 address parameters
 *                bound to the interface.
 *
 * @see fnet_netif_set_ip4_addr_type
 *
 ******************************************************************************
 *
 * This function determines if the IPv4 parameters of the @c netif interface
 * were set manually, or obtained by the DHCP client or
 * set during link-local autoconfiguartion (AutoIP - TBD)
 *
 ******************************************************************************/
fnet_netif_ip_addr_type_t fnet_netif_get_ip4_addr_type( fnet_netif_desc_t netif_desc );

/***************************************************************************/ /*!
 *
 * @brief    Sets the way IPv4 address parameters were obtained.
 *
 * @param netif_desc  Network interface descriptor.
 *
 * @param ipaddr_type  Type of IPv4 address parameters.
 *
 * @see fnet_netif_get_ip4_addr_type()
 *
 ******************************************************************************
 *
 * This function sets type of the interface IPv4 address parameters, if they
 * were set manually, or obtained by the DHCP client or
 * set during link-local autoconfiguartion (AutoIP). @n
 * fnet_netif_set_ip4_addr(),
 * fnet_netif_set_ip4_gateway() and fnet_netif_set_ip4_dns() sets
 * the type to FNET_NETIF_IP_ADDR_TYPE_MANUAL automatically.
 *
 ******************************************************************************/
void fnet_netif_set_ip4_addr_type( fnet_netif_desc_t netif_desc, fnet_netif_ip_addr_type_t ipaddr_type );

/***************************************************************************/ /*!
 *
 * @brief    Determines the link status of the network interface.
 *
 * @param netif_desc  Network interface descriptor.
 *
 * @return       This function returns:
 *   - @c FNET_FALSE if the network link is disconnected.
 *   - @c FNET_TRUE if the network link is connected.
 *
 ******************************************************************************
 *
 * This function determines if the @c netif interface is marked as connected to
 * a network or not.
 * The Ethernet network interface gets this status from the PHY.
 *
 ******************************************************************************/
fnet_bool_t fnet_netif_is_connected( fnet_netif_desc_t netif_desc );

/***************************************************************************/ /*!
 *
 * @brief    Retrieves the network interface statistics.
 *
 * @param netif_desc  Network interface descriptor.
 *
 * @param statistics  Structure that receives the network interface statistics
 *                    defined by the @ref fnet_netif_statistics structure.
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs or the network interface driver does
 *          not support this statistics.
 *
 ******************************************************************************
 *
 * This function retrieves the network statistics of the @c netif interface
 * and puts it into the @c statistics defined by the @ref fnet_netif_statistics
 * structure.
 *
 ******************************************************************************/
fnet_return_t fnet_netif_get_statistics( fnet_netif_desc_t netif_desc, struct fnet_netif_statistics *statistics );


#if FNET_CFG_IP4 || defined(__DOXYGEN__)

/**************************************************************************/ /*!
 * @brief Event handler callback function prototype, that is
 * called when there is an IP address conflict with another system
 * on the network.
 *
 * @param netif      Network interface descriptor that has duplicated IP
 *                   address.
 *
 * @see fnet_socket_rx_handler_init()
 ******************************************************************************/
typedef void(*fnet_netif_callback_ip4_addr_conflict_t)( fnet_netif_desc_t netif );

/***************************************************************************/ /*!
 *
 * @brief    Registers the "IPv4 address conflict" event handler.
 *
 * @param callback    Pointer to the event-handler callback function defined by
 *                   @ref fnet_netif_callback_ip4_addr_conflict_t.
 *
 * @see fnet_netif_is_ip4_addr_conflict()
 *
 ******************************************************************************
 *
 * This function registers the @c handler callback function for
 * the "IP4 address conflict" event. This event occurs when there is
 * an IPv4 address conflict with another system on the network. It is detected by ARP protocol. @n
 * To stop the event handling, set @c callback parameter to zero value.
 *
 ******************************************************************************/
void fnet_netif_set_callback_on_ip4_addr_conflict (fnet_netif_callback_ip4_addr_conflict_t callback);

/***************************************************************************/ /*!
 *
 * @brief    Determines if there is IPv4 address conflict.
 *
 * @param netif_desc  Network interface descriptor.
 *
 * @return       This function returns:
 *   - @c FNET_FALSE if the IPv4 address conflict is not detected.
 *   - @c FNET_TRUE if the IPv4 address conflict is detected.
 *
 * @see fnet_netif_set_callback_on_ip4_addr_conflict(), fnet_netif_clear_ip4_addr_conflict()
 *
 ******************************************************************************
 *
 * This function determines if there is the @c netif interface IPv4 address conflict
 * with another system on the network.@n
 * The address conflict is detected by ARP protocol.@n
 * The conflict flag is cleared on changing of the interface IPv4 address,
 * using fnet_netif_set_ip4_addr() or calling fnet_netif_clear_ip4_addr_conflict().
 *
 ******************************************************************************/
fnet_bool_t fnet_netif_is_ip4_addr_conflict(fnet_netif_desc_t netif_desc);

/***************************************************************************/ /*!
 *
 * @brief    Clears IPv4 address conflict flag.
 *
 * @param netif_desc  Network interface descriptor.
 *
 * @see fnet_netif_set_callback_on_ip4_addr_conflict(), fnet_netif_is_ip4_addr_conflict()
 *
 ******************************************************************************
 *
 * This function clears the @c netif interface IPv4 address conflict flag.@n
 * The address conflict is detected by ARP protocol.@n
 * The conflict flag is also cleared on changing of the interface IPv4 address,
 * using fnet_netif_set_ip4_addr().
 *
 ******************************************************************************/
void fnet_netif_clear_ip4_addr_conflict( fnet_netif_desc_t netif_desc );

#endif /* FNET_CFG_IP4 */

#if (FNET_CFG_MULTICAST & FNET_CFG_IP4) || defined(__DOXYGEN__)

/***************************************************************************/ /*!
 *
 * @brief    Joins the specified network interface to IPv4 multicast group.
 *
 * @param netif_desc        Network interface descriptor.
 *
 * @param multicast_addr    Multicast-group address to be joined by the interface.
 *
 * @see fnet_netif_leave_ip4_multicast()
 *
 ******************************************************************************
 *
 * This function configures the HW interface to receive
 * particular multicast address.
 * When the network interface picks up a packet which has a destination
 * that matches any of the joined-multicast addresses, it will pass it to
 * the upper layers for further processing. @n
 * @note User application should not call this function. @n
 * This function is available only if @ref FNET_CFG_MULTICAST and
 * @ref FNET_CFG_IP4 are set to @c 1.
 *
 ******************************************************************************/
void fnet_netif_join_ip4_multicast ( fnet_netif_desc_t netif_desc, fnet_ip4_addr_t multicast_addr );

/***************************************************************************/ /*!
 *
 * @brief    Leaves the specified network interface from IPv4 multicast group.
 *
 * @param netif_desc        Network interface descriptor.
 *
 * @param multicast_addr    Multicast-group address to be leaved by the interface.
 *
 * @see fnet_netif_join_ip4_multicast()
 *
 ******************************************************************************
 *
 * This function configures the HW interface to ignore
 * particular multicast address. @n
 * @note User application should not call this function. @n
 * This function is available only if @ref FNET_CFG_MULTICAST and
 * @ref FNET_CFG_IP4 are set to @c 1.
 *
 ******************************************************************************/
void fnet_netif_leave_ip4_multicast ( fnet_netif_desc_t netif_desc, fnet_ip4_addr_t multicast_addr );

#endif /* FNET_CFG_MULTICAST & FNET_CFG_IP4*/

#if FNET_CFG_IP6 || defined(__DOXYGEN__)

/***************************************************************************/ /*!
 *
 * @brief    Joins the specified network interface to IPv6 multicast group.
 *
 * @param netif_desc        Network interface descriptor.
 *
 * @param multicast_addr    Multicast-group address to be joined by the interface.
 *
 * @see fnet_netif_leave_ip6_multicast()
 *
 ******************************************************************************
 *
 * This function configures the HW interface to receive
 * particular multicast address.
 * When the network interface picks up a packet which has a destination
 * that matches any of the joined-multicast addresses, it will pass it to
 * the upper layers for further processing. @n
 * @note User application should not call this function. @n
 * This function is available only if @ref FNET_CFG_IP6 is set to @c 1.
 *
 ******************************************************************************/
void fnet_netif_join_ip6_multicast ( fnet_netif_desc_t netif_desc, const fnet_ip6_addr_t *multicast_addr );

/***************************************************************************/ /*!
 *
 * @brief    Leaves the specified network interface from IPv6 multicast group.
 *
 * @param netif_desc        Network interface descriptor.
 *
 * @param multicast_addr    Multicast-group address to be leaved by the interface.
 *
 * @see fnet_netif_join_ip6_multicast()
 *
 ******************************************************************************
 *
 * This function configures the HW interface to ignore
 * particular multicast address. @n
 * @note User application should not call this function. @n
 * This function is available only if @ref FNET_CFG_IP6 is set to @c 1.
 *
 ******************************************************************************/
void fnet_netif_leave_ip6_multicast ( fnet_netif_desc_t netif_desc, fnet_ip6_addr_t *multicast_addr );

/***************************************************************************/ /*!
 *
 * @brief    Retrieves an IPv6 address of the specified network interface.
 *
 * @param netif_desc  Network interface descriptor.
 *
 * @param n           Sequence number of IPv6 address to retrieve (from @c 0).
 *
 * @param addr_info   Pointer to address information structure will contain the result.
 *
 * @return This function returns:
 *   - @ref FNET_TRUE if no error occurs and data structure is filled.
 *   - @ref FNET_FALSE in case of error or @c n-th address is not available.
 *
 ******************************************************************************
 *
 * This function is used to retrieve all IPv6 addresses registered with
 * the given interface.
 *
 ******************************************************************************/
fnet_bool_t fnet_netif_get_ip6_addr ( fnet_netif_desc_t netif_desc, fnet_index_t n, fnet_netif_ip6_addr_info_t *addr_info );

#if FNET_CFG_DNS || defined(__DOXYGEN__)
/***************************************************************************/ /*!
 *
 * @brief    Retrieves the n-th DNS IPv6 address of the specified network interface.
 *
 * @param netif_desc  Network interface descriptor.
 *
 * @param n           Sequence number of DNS IPv6 address to retrieve (from @c 0).
 *
 * @param addr_dns    Pointer to DNS IPv6 address that will contain the result.
 *
 * @return This function returns:
 *   - @ref FNET_TRUE if no error occurs and  @c addr_dns is filled.
 *   - @ref FNET_FALSE in case of error or @c n-th DNS address is not available.
 *
 * @see FNET_CFG_ND6_RDNSS, FNET_CFG_DNS
 ******************************************************************************
 *
 * This function is used to retrieve all DNS IPv6 addresses registered with
 * the given interface.@n
 * It is present only if @ref FNET_CFG_DNS is set to 1.
 *
 ******************************************************************************/
fnet_bool_t fnet_netif_get_ip6_dns( fnet_netif_desc_t netif_desc, fnet_index_t n, fnet_ip6_addr_t *addr_dns );
#endif /* FNET_CFG_DNS */

/***************************************************************************/ /*!
 *
 * @brief    Binds the IPv6 address to the specified network interface.
 *
 * @param netif_desc    Network interface descriptor.
 *
 * @param addr        The IPv6 address for the network interface.
 *
 * @param addr_type   The IPv6 address type that defines the way the IPv6
 *                    address to be assigned to the interface.
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR in case of error.
 *
 * @see fnet_netif_unbind_ip6_addr()
 *
 ******************************************************************************
 *
 * This function binds the IPv6 address to the @c netif interface.@n
 * The @c addr_type parameter defines the way the IPv6 address is assigned
 * to the interface:
 *   - @c FNET_NETIF_IP_ADDR_TYPE_AUTOCONFIGURABLE: value of the @c addr
 *             parameter defines the first 64bits of the bind IPv6 address.
 *            The last 64bits of the IPv6 address are overwritten with the
 *            Interface Identifier. In case of Ethernet interface,
 *            the Interface Identifier is formed from 48-bit MAC address,
 *            according to [RFC2464].
 *   - @c !FNET_NETIF_IP_ADDR_TYPE_AUTOCONFIGURABLE:  value of the @c addr parameter
 *             defines the whole IPv6 address to be bind to the interface.
 *
 ******************************************************************************/
fnet_return_t fnet_netif_bind_ip6_addr(fnet_netif_desc_t netif_desc, const fnet_ip6_addr_t *addr, fnet_netif_ip_addr_type_t addr_type);

/***************************************************************************/ /*!
 *
 * @brief    Unbinds the IPv6 address from the specified network interface.
 *
 * @param netif_desc    Network interface descriptor.
 *
 * @param addr        The IPv6 address to unbind.
 *
 * @return This function returns:
 *   - @ref FNET_OK if successful.
 *   - @ref FNET_ERR if failed.
 *
 * @see fnet_netif_bind_ip6_addr()
 *
 ******************************************************************************
 *
 * This function unbinds the IPv6 address from the @c netif interface.
 *
 ******************************************************************************/
fnet_return_t fnet_netif_unbind_ip6_addr(fnet_netif_desc_t netif_desc, const fnet_ip6_addr_t *addr);

/***************************************************************************/ /*!
 *
 * @brief    Retrieves the n-th IPv6 prefix of the specified network interface.
 *
 * @param netif_desc  Network interface descriptor.
 *
 * @param n           Sequence number of IPv6 prefix to retrieve (from @c 0).
 *
 * @param ip6_prefix    Pointer to prefix entry structure that will contain the result.
 *
 * @return This function returns:
 *   - @ref FNET_TRUE if no error occurs and  @c ip6_prefix is filled.
 *   - @ref FNET_FALSE in case of error or @c n-th prefix is not available.
 *
 * @see Ffnet_netif_ip6_prefix_t
 ******************************************************************************
 *
 * This function is used to retrieve the IPv6 prefix list for
 * the given interface.
 *
 ******************************************************************************/
fnet_bool_t fnet_netif_get_ip6_prefix( fnet_netif_desc_t netif_desc, fnet_index_t n, fnet_netif_ip6_prefix_t *ip6_prefix);

/***************************************************************************/ /*!
 *
 * @brief    Retrieves the n-th IPv6 neighbor cache entry of the specified network interface.
 *
 * @param netif_desc  Network interface descriptor.
 *
 * @param n           Sequence number of IPv6 neighbor cache entry to retrieve (from @c 0).
 *
 * @param ip6_neighbor_cache    Pointer to neighbor cache entry that will contain the result.
 *
 * @return This function returns:
 *   - @ref FNET_TRUE if no error occurs and  @c ip6_neighbor_cache is filled.
 *   - @ref FNET_FALSE in case of error or @c n-th neighbor cache entry is not available.
 *
 * @see fnet_netif_ip6_neighbor_cache_t
 ******************************************************************************
 *
 * This function is used to retrieve the Ipv6 neighbor cache for
 * the given interface.@n
 *
 ******************************************************************************/
fnet_bool_t fnet_netif_get_ip6_neighbor_cache(fnet_netif_desc_t netif_desc, fnet_index_t n, fnet_netif_ip6_neighbor_cache_t *ip6_neighbor_cache );


/***************************************************************************/ /*!
 *
 * @brief    Looks for a network interface according to the specified IPv6 address.
 *
 *
 * @param ip_addr       The IPv6 address of a network interface.
 *
 * @return This function returns:
 *   - Network interface descriptor that matches the @c ip_addr parameter.
 *   - @ref FNET_NULL if there is no match.
 *
 * @see fnet_netif_get_by_name()
 *
 ******************************************************************************
 *
 * This function scans the global network interface list looking for a network
 * interface matching the @c ip_addr parameter.
 *
 ******************************************************************************/
fnet_netif_desc_t fnet_netif_get_by_ip6_addr(const fnet_ip6_addr_t *ip_addr );

#endif /* FNET_CFG_IP6 */

/***************************************************************************/ /*!
 *
 * @brief    Retrieves an Scope ID of the specified network interface.
 *
 * @param netif_desc  Network interface descriptor.
 *
 * @return This function returns:
 *          - Scope ID value.
 *          - @c 0 if no Scope ID was assigned to the interface.
 *
 ******************************************************************************
 *
 * This function is used to retrieve Scope ID assigned to the given interface.
 *
 ******************************************************************************/
fnet_scope_id_t fnet_netif_get_scope_id(fnet_netif_desc_t netif_desc);

/***************************************************************************/ /*!
 *
 * @brief    Retrieves a Maximum Transmission Unit (MTU) of the specified network interface.
 *
 * @param netif_desc  Network interface descriptor.
 *
 * @return This function returns:
 *          - MTU value.
 *
 ******************************************************************************
 *
 * This function is used to retrieve Maximum Transmission Unit (MTU) of the given interface.
 *
 ******************************************************************************/
fnet_size_t fnet_netif_get_mtu(fnet_netif_desc_t netif_desc);

/***************************************************************************/ /*!
 *
 * @brief    Looks for a network interface according to the specified Scope ID.
 *
 * @param scope_id       The Scope ID of a network interface.
 *
 * @return This function returns:
 *   - Network interface descriptor that matches the @c scope_id parameter.
 *   - @ref FNET_NULL if there is no match.
 *
 * @see fnet_netif_get_scope_id()
 *
 ******************************************************************************
 *
 * This function scans the global network interface list looking for a network
 * interface matching the specified Scope ID.
 *
 ******************************************************************************/
fnet_netif_desc_t fnet_netif_get_by_scope_id(fnet_scope_id_t scope_id);

/***************************************************************************/ /*!
 *
 * @brief    Looks for a network interface according to the specified socket
 *           address.
 *
 * @param addr       The socket address of a network interface.
 *
 * @return This function returns:
 *   - Network interface descriptor that matches the @c addr parameter.
 *   - @ref FNET_NULL if there is no match.
 *
 * @see fnet_netif_get_by_scope_id()
 *
 ******************************************************************************
 *
 * This function scans the global network interface list looking for a network
 * interface matching the specified socket address.
 *
 ******************************************************************************/
fnet_netif_desc_t fnet_netif_get_by_sockaddr( const struct fnet_sockaddr *addr );

/***************************************************************************/ /*!
 *
 * @brief    Initializes network interface and adds it to FNET stack
 *
 * @param netif_desc    Network interface descriptor to be initialized (FNET_CPU_ETH0_IF,FNET_CPU_ETH1_IF).
 *
 * @param hw_addr        Buffer containing the hardware address
 *                      (for the Ethernet interface, it contains the MAC address).
 *
 * @param hw_addr_size   Size of the hardware address in the @c hw_addr (for the Ethernet interface, it
 *                       equals @c 6).
 *
 * @return This function returns:
 *   - @ref FNET_OK if successful.
 *   - @ref FNET_ERR if failed.
 *
 * @see fnet_init(), fnet_netif_release()
 *
 ******************************************************************************
 *
 * This function initializes network interface defined by @c netif_desc and
 * adds it to the FNET stack.@n
 * FNET declares descriptors for Ethernet interfaces (FNET_CPU_ETH0_IF, FNET_CPU_ETH1_IF).
 * @note Loop-back interface (if set FNET_CFG_LOOPBACK) is initialized automatically during stack initialization.
 *
 ******************************************************************************/
fnet_return_t fnet_netif_init(fnet_netif_desc_t netif_desc, fnet_uint8_t *hw_addr, fnet_size_t hw_addr_size );

/***************************************************************************/ /*!
 *
 * @brief    Releases network interface and removes it from FNET stack
 *
 * @param netif_desc    Network interface descriptor to be released (FNET_CPU_ETH0_IF,FNET_CPU_ETH1_IF).
 *
 * @see fnet_init(), fnet_netif_init()
 *
 ******************************************************************************
 *
 * This function releases network interface defined by @c netif_desc and
 * removes it from the FNET stack.@n
 * FNET declares descriptors for Ethernet interfaces (FNET_CPU_ETH0_IF, FNET_CPU_ETH1_IF).
 * @note fnet_release() automatically releases all initialized interfaces.
 *
 ******************************************************************************/
void fnet_netif_release(fnet_netif_desc_t netif_desc);

/***************************************************************************/ /*!
 *
 * @brief    Determines if the network interface is initialized.
 *
 * @param netif_desc  Network interface descriptor.
 *
 * @return       This function returns:
 *   - @c FNET_FALSE if the network link is released or not initialized.
 *   - @c FNET_TRUE if the network link is initialized.
 *
 ******************************************************************************
 *
 * This function determines if the @c netif interface is initialized,
 * using ref fnet_netif_init().
 *
 ******************************************************************************/
fnet_bool_t fnet_netif_is_initialized(fnet_netif_desc_t netif_desc);

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* _FNET_NETIF_H_ */
