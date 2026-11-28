/**************************************************************************
*
* Copyright 2016-2018 by Andrey Butok. FNET Community.
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
* ARP protocol Public API
*
***************************************************************************/

#ifndef _FNET_ARP_H_

#define _FNET_ARP_H_

#if FNET_CFG_IP4 || defined(__DOXYGEN__)

/*! @addtogroup fnet_arp
* Address Resolution Protocol (ARP)
*/
/*! @{ */

/**************************************************************************/ /*!
 * @brief ARP cache entry information structure.
 * @see fnet_arp_get_entry()
 ******************************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct fnet_arp_entry_info
{
    fnet_mac_addr_t mac_addr;      /**< @brief ARP cache entry Physical (MAC) address.*/
    fnet_ip4_addr_t ip_addr;       /**< @brief ARP cache entry IPv4 address.*/
} fnet_arp_entry_info_t;
FNET_COMP_PACKED_END

/************************************************************************
*     Function Prototypes
*************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/***************************************************************************/ /*!
 *
 * @brief    Retrieves ARP cache entry of the specified network interface.
 *
 * @param[in] netif_desc  Network interface descriptor.
 *
 * @param[in] n           Sequence number of ARP cache entry to retrieve (from @c 0).
 *
 * @param[out] entry_info   Pointer to the ARP cache entry information structure will contain the result.
 *
 * @return This function returns:
 *   - @ref FNET_TRUE if no error occurs and data structure is filled.
 *   - @ref FNET_FALSE in case of error or @c n-th entry is not available.
 *
 ******************************************************************************
 *
 * This function is used to retrieve ARP cache entries assigned
 * the given interface.
 *
 ******************************************************************************/
fnet_bool_t fnet_arp_get_entry ( fnet_netif_desc_t netif_desc, fnet_index_t n, fnet_arp_entry_info_t *entry_info );

/***************************************************************************/ /*!
 *
 * @brief    Gets MAC address of valid ARP cache entry.
 *
 * @param[in] netif_desc   Network interface descriptor.
 *
 * @param[in] ip_addr      IPv4 address to search for.
 *
 * @param[out] mac_addr    Buffer will contain a MAC address corresponding @c ipaddr.@n
 *                         This parameter is optional.
 *
 * @return This function returns:
 *   - @ref FNET_TRUE if no error occurs and entry corresponding to @c ip_addr exists.
 *   - @ref FNET_FALSE in case of error or entry corresponding to @c ip_addr does not exist.
 *
 ******************************************************************************
 *
 * This function searches for an entry corresponding to the provided IPv4 address.@n
 * If @c mac_addr parameter is present, it will contain a
 * valid MAC address corresponding to @c ip_addr.
 *
 ******************************************************************************/
fnet_bool_t fnet_arp_get_mac( fnet_netif_desc_t netif_desc, fnet_ip4_addr_t ip_addr, fnet_mac_addr_t mac_addr);

/***************************************************************************/ /*!
 *
 * @brief    Sends ARP request.
 *
 * @param[in] netif_desc  Network interface descriptor.
 *
 * @param[in] ip_addr      Target protocol address.
 *
 ******************************************************************************
 *
 * This function sends a broadcast ARP message, requesting an answer for @c ipaddr.@n
 * It can be used for ARP probing of IPv4 address before beginning to use. @n
 * Also, it can be used for a gratuitous ARP message when sender's IP address
 * or MAC address has changed.
 *
 ******************************************************************************/
void fnet_arp_send_request( fnet_netif_desc_t netif_desc, fnet_ip4_addr_t ip_addr );


#if defined(__cplusplus)
}
#endif

/*! @} */

#endif  /* FNET_CFG_IP4 */

#endif /* _FNET_ARP_H_ */
