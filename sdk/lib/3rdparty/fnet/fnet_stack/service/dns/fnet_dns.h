/**************************************************************************
*
* Copyright 2011-2020 by Andrej Butok. FNET Community.
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
*  DNS Resolver API.
*
***************************************************************************/

#ifndef _FNET_DNS_H_

#define _FNET_DNS_H_

#if FNET_CFG_DNS || defined(__DOXYGEN__)

/*! @addtogroup fnet_dns
*
* The DNS client/resolver service allows user application to resolve IP addresses
* of internet hosts that are identified by a host name. @n
* It does this by sending DNS request to a DNS Server.
* The IP address of a DNS Server is specified manually or can be obtained from
* the DHCP Server for the Local Area Network. @n
* @n
* After the DNS client is initialized by calling the @ref fnet_dns_init() function,
* the user application should call the main service-polling function
* @ref fnet_service_poll() periodically in background. @n
* The resolved IP-address will be passed to the @ref fnet_dns_callback_resolved_t callback function,
* which is set during the DNS-client service initialization.
* @n
* The DNS client service is released automatically as soon as the requested host name is
* fully resolved or an error occurs. Your application code may continue
* to call @ref fnet_service_poll() to handle other services, but this will not have any
* impact on the DNS client communication until you initialize the next IP address resolving by calling
* @ref fnet_dns_init() again. @n
* @n
* For the DNS-client service example, refer to the FNET Shell demo source code.@n
* @note
* Current version of the DNS client:
*  - does not cache the resolved IP addresses.
*  - can process only one request at a time.
*  - uses UDP protocol, without message truncation.
*  - does not support DNS servers without recursion (all real-life DNS servers support it).
*  - takes the first resolved IP address, even if the DNS server provides several ones.
*
* Configuration parameters:
* - @ref FNET_CFG_DNS
* - @ref FNET_CFG_DNS_PORT
* - @ref FNET_CFG_DNS_RETRANSMISSION_MAX
* - @ref FNET_CFG_DNS_RETRANSMISSION_TIMEOUT
* - @ref FNET_CFG_DNS_RESOLVED_ADDR_MAX
* - @ref FNET_CFG_ND6_RDNSS
* - @ref FNET_CFG_ND6_RDNSS_LIST_SIZE
*
*/

/*! @{ */

/**************************************************************************/ /*!
 * @brief Resolved address structure provided by @ref fnet_dns_callback_resolved_t
 * callback function.
 ******************************************************************************/
typedef struct
{
    struct fnet_sockaddr        resolved_addr;      /**< @brief Socket address of the resolved host name.*/
    fnet_uint32_t               resolved_addr_ttl;  /**< @brief Specifies the time interval (in seconds) that the
                                                    * resolved address may be cached before it should be discarded.*/
} fnet_dns_resolved_addr_t;

/**************************************************************************/ /*!
 * @brief DNS client descriptor.
 * @see fnet_dns_init(), fnet_dns_release()
 ******************************************************************************/
typedef void *fnet_dns_desc_t;

/**************************************************************************/ /*!
 * @brief Prototype of the DNS-client callback function that is
 * called when the DNS client has completed the resolving.
 *
 * @param addr_list         Pointer to the list of addresses or @ref FNET_NULL if the resolving is failed.
 * @param addr_list_size    Number of resolved addresses in @c addr_list. Its maximu number is defined by @ref FNET_CFG_DNS_RESOLVED_ADDR_MAX.
 * @param host_name         Pointer to the resolved host name. It's set during
 *                          the DNS-client service initialization as part of
 *                          @ref fnet_dns_params. 
 * @param cookie            User-application specific parameter. It's set during
 *                          the DNS-client service initialization as part of
 *                          @ref fnet_dns_params.
 *
 * @see fnet_dns_resolve(), fnet_dns_params
 ******************************************************************************/
typedef void(*fnet_dns_callback_resolved_t)(const fnet_dns_resolved_addr_t *addr_list, fnet_size_t addr_list_size, const fnet_char_t *host_name, void *cookie);

/**************************************************************************/ /*!
 * @brief Initialization parameters for the @ref fnet_dns_init() function.
 ******************************************************************************/
struct fnet_dns_params
{
    struct fnet_sockaddr            dns_server_addr;    /**< @brief Socket address of the remote DNS server to connect to. @n
                                                        * If it is unspecified, any registered DNS server is used. And if dns_server_addr->sa_family and/or dns_server_addr->sa_scope_id are set, they are used as a tip for DNS sever address.*/
    const fnet_char_t               *host_name;         /**< @brief Host name to resolve (null-terminated string). */
    fnet_address_family_t           addr_family;        /**< @brief Family of the IP Address which is queried.*/
    fnet_dns_callback_resolved_t    callback;            /**< @brief Pointer to the callback function defined by
                                                        * @ref fnet_dns_callback_resolved_t. It is called when the
                                                        * DNS-client resolving is finished or a timeout is occurred. */
    void                            *cookie;            /**< @brief Optional application-specific parameter which is passed to the @c callback
                                                        * function as input parameter. */
};

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************************************************/ /*!
 *
 * @brief    Initializes the DNS client service and starts the host name resolving.
 *
 * @param params     Initialization parameters.
 *
 * @return This function returns:
 *   - DNS client descriptor if no error occurs.
 *   - @c FNET_NULL if an error occurs.
 *
 * @see fnet_dns_params, fnet_dns_callback_resolved_t, fnet_dns_release()
 *
 ******************************************************************************
 *
 * This function initializes the DNS client service and starts the
 * host name resolving. It allocates all needed
 * resources and registers the DNS service in the polling list.@n
 * After the initialization, the user application should call the main polling
 * function @ref fnet_service_poll() periodically to run the DNS service routine
 * in the background.@n
 * The resolved IP-address will be passed to the @ref fnet_dns_callback_resolved_t callback function,
 * which is set in @c params. @n
 * The DNS service is released automatically as soon as the
 * resolving is finished or or a timeout is occurred. The timeout equals to
 * FNET_CFG_DNS_RETRANSMISSION_TIMEOUT * FNET_CFG_DNS_RETRANSMISSION_MAX seconds.
 *
 ******************************************************************************/
fnet_dns_desc_t fnet_dns_init( struct fnet_dns_params *params );

/***************************************************************************/ /*!
 *
 * @brief    Aborts the resolving and releases the DNS-client service.
 *
 * @param desc     DNS client descriptor to be released.
 *
 * @see fnet_dns_init()
 *
 ******************************************************************************
 *
 * This function stops the DNS-client service. It releases all resources
 * used by the service, and unregisters it from the service polling list.@n
 * Use this function only in the case of the early termination of the service,
 * because the DNS service is released automatically as soon as the
 * resolving is finished.
 *
 ******************************************************************************/
void fnet_dns_release(fnet_dns_desc_t desc);

/***************************************************************************/ /*!
 *
 * @brief    Detects if the DNS client service is enabled or disabled.
 *
 * @param desc     DNS client descriptor.
 *
 * @return This function returns:
 *          - @ref FNET_TRUE if the DNS client is successfully initialized.
 *          - @ref FNET_FALSE if the DNS client is not initialized or is released.
 *
 ******************************************************************************
 *
 * This function detects if the DNS client service is initialized or is released.
 *
 ******************************************************************************/
fnet_bool_t fnet_dns_is_enabled(fnet_dns_desc_t desc);

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* FNET_CFG_DNS */

#endif /* _FNET_DNS_H_ */
