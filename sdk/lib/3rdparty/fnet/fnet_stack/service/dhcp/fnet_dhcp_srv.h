/**************************************************************************
*
* Copyright 2017-2018 by Andrey Butok. FNET Community.
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
*  FNET DHCPv4 Server API.
*
***************************************************************************/

#ifndef _FNET_DHCP_SRV_H_

#define _FNET_DHCP_SRV_H_

#if FNET_CFG_DHCP_SRV || defined(__DOXYGEN__)

/*! @addtogroup fnet_dhcp_srv
* TBD @n
* @n
* After the DHCPv4 server is initialized by calling the fnet_dhcp_srv_init() function,
* the user application should call the main service-polling function
* @ref fnet_service_poll() periodically in background. @n
* @n
* Configuration parameters:
* - @ref FNET_CFG_DHCP_SRV
* - @ref FNET_CFG_DHCP_SRV_PORT
* - @ref FNET_CFG_DHCP_SRV_ADDR_POOL_SIZE
* - @ref FNET_CFG_DHCP_SRV_LEASE_TIME
*
*/
/*! @{ */


/**************************************************************************/ /*!
 * @brief Initialization parameters for the @ref fnet_dhcp_srv_init() function.
 ******************************************************************************/
typedef struct
{
    fnet_netif_desc_t   netif;                  /**< @brief Network interface descriptor to be used by the DHCP server.*/
    fnet_ip4_addr_t     ip_addr_pool_start;     /**< @brief The first allocated IPv4 address in the DHCPv4 server pool. @n The address pool size is defined by @c ip_addr_pool_size.
                                                    If it is set to @c INADDR_ANY, the @c netif IPv4 address plus one is used.*/
    fnet_size_t         ip_addr_pool_size;      /**< @brief Size of the address pool. It may not be set a value more than @ref FNET_CFG_DHCP_SRV_ADDR_POOL_SIZE.
                                                     If it is set to @c 0, the @ref FNET_CFG_DHCP_SRV_ADDR_POOL_SIZE value is used.*/
    fnet_uint32_t       lease_time;             /**< @brief Default lease time in seconds.@n
                                                      If it is set to @c 0, the @ref FNET_CFG_DHCP_SRV_LEASE_TIME value is used.*/
    fnet_ip4_addr_t     subnet_mask;            /**< @brief Subnet mask.@n
                                                    If it is set to @c INADDR_ANY, a subnet mask of the @c netif interface is used.*/
    fnet_ip4_addr_t     gateway;                /**< @brief Gateway IPv4 address.@n
                                                    If it is set to @c INADDR_ANY, a gateway address of the @c netif interface is used.*/
#if FNET_CFG_DNS
    fnet_ip4_addr_t     dns;                    /**< @brief DNS server IPv4 address.@n
                                                    If it is set to @c INADDR_ANY, a DNS server address of the @c netif interface is used.*/
#endif
} fnet_dhcp_srv_params_t;

/**************************************************************************/ /*!
 * @brief Address pool information structure.
 ******************************************************************************/
typedef struct
{
    fnet_mac_addr_t                     client_mac_addr;    /**< @brief Client-identifier (MAC address)*/
    fnet_ip4_addr_t                     client_ip4_addr;    /**< @brief Client IPv4 address allocated by the DHCPv4 server.*/
    fnet_time_t                         lease_time;         /**< @brief Lease time (in seconds).*/
} fnet_dhcp_srv_addr_pool_info_t;

/**************************************************************************/ /*!
 * @brief DHCPv4 server descriptor.
 * @see fnet_dhcp_srv_init()
 ******************************************************************************/
typedef void *fnet_dhcp_srv_desc_t;

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************************************************/ /*!
 *
 * @brief    Initializes the DHCPv4 server service.
 *
 * @param params     Initialization parameters defined by @ref fnet_dhcp_srv_params_t.
 *
 * @return This function returns:
 *   - DHCPv4 server descriptor if no error occurs.
 *   - @c 0 if an error occurs.
 *
 * @see fnet_dhcp_srv_release()
 *
 ******************************************************************************
 *
 * This function initializes the DHCPv4 server service. It allocates all
 * resources needed and registers the DHCPv4 server in the service polling list.@n
 * After the initialization, the user application should call the main polling
 * function @ref fnet_service_poll() periodically to run the DHCPv4 server service
 * in the background.
 *
 ******************************************************************************/
fnet_dhcp_srv_desc_t fnet_dhcp_srv_init( fnet_dhcp_srv_params_t *params );

/***************************************************************************/ /*!
 *
 * @brief    Releases the DHCPv4 server service.
 *
 * @param desc     DHCPv4 server descriptor to be released.
 *
 * @see fnet_dhcp_srv_init()
 *
 ******************************************************************************
 *
 * This function releases the DHCPv4 server. It releases all resources
 * used by the server, and unregisters it from the service polling list.
 *
 ******************************************************************************/
void fnet_dhcp_srv_release(fnet_dhcp_srv_desc_t desc);

/***************************************************************************/ /*!
 *
 * @brief    Retrieves an address pool information.
 *
 * @param desc        DHCPv4 server descriptor.
 *
 * @param n           Sequence number of the address pool entry to retrieve (from @c 0).
 *
 * @param addr_info   Pointer to the address information structure will contain the result.
 *
 * @return This function returns:
 *   - @ref FNET_TRUE if no error occurs and data structure is filled.
 *   - @ref FNET_FALSE in case of error or @c n-th address is not available.
 *
 ******************************************************************************
 *
 * This function is used to retrieve an information about addresses allocated by
 * the DHCPv4 server.
 *
 ******************************************************************************/
fnet_bool_t fnet_dhcp_srv_get_addr_pool_info(fnet_dhcp_srv_desc_t desc, fnet_index_t n, fnet_dhcp_srv_addr_pool_info_t *addr_info);

/***************************************************************************/ /*!
 *
 * @brief    Detects if the DHCP server service is enabled or disabled.
 *
 * @param desc     DHCPv4 server descriptor
 *
 * @return This function returns:
 *          - @ref FNET_TRUE if the DHCP server is successfully initialized.
 *          - @ref FNET_FALSE if the DHCP server is not initialized or is released.
 *
 ******************************************************************************
 *
 * This function detects if the DHCP server service is initialized or is released.
 *
 ******************************************************************************/
fnet_bool_t fnet_dhcp_srv_is_enabled(fnet_dhcp_srv_desc_t desc);

/***************************************************************************/ /*!
 *
 * @brief    Looks for a DHCP Server assigned to the specified network interface.
 *
 * @param netif      Network interface descriptor.
 *
 * @return This function returns:
 *          - DHCP server descriptor assigned to the @c netif network interface.
 *          - @ref FNET_NULL if there is no any DHCP server assigned to the network interface.
 *
 * @see fnet_dhcp_srv_init
 *
 ******************************************************************************
 *
 * This function is looking for a DHCP Server service assigned to
 * the @c netif network interface set during its initialization.
 *
 ******************************************************************************/
fnet_dhcp_srv_desc_t fnet_dhcp_srv_get_by_netif(fnet_netif_desc_t netif);


#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* FNET_CFG_DHCP_SRV */

#endif /* _FNET_DHCP_SRV_H_ */
