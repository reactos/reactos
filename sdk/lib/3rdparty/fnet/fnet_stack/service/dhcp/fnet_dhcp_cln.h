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
*  FNET DHCPv4 Client API.
*
***************************************************************************/

#ifndef _FNET_DHCP_CLN_H_

#define _FNET_DHCP_CLN_H_

#if FNET_CFG_DHCP_CLN || defined(__DOXYGEN__)

/*! @addtogroup fnet_dhcp_cln
* The user application can use the DHCP-client service to retrieve various network
* parameters (IP address, gateway address, subnet mask) from a DHCP server.@n
* The DHCP client code keeps the IP address parameters updated by renewing it
* automatically, without user application intervention.@n
* @n
* After the DHCP client is initialized by calling the fnet_dhcp_cln_init() function,
* the user application should call the main service-polling function
* @ref fnet_service_poll() periodically in background. @n
* @n
* The DHCP client will be released automatically if an application sets
* any of the IP-related parameters directly by the @ref fnet_netif_set_ip4_addr() or @ref fnet_netif_set_ip4_gateway() calls. @n
* @n
* For example:
* @code
* ...
*    fnet_netif_desc_t netif;
*    fnet_dhcp_cln_params_t dhcp_params;
*
*    // Get current net interface.
*    if((netif = fnet_netif_get_default()) == 0)
*    {
*        fnet_printf("ERROR: Network Interface is not configurated!");
*    }
*    else
*    {
*        fnet_memset_zero(&dhcp_params, sizeof(dhcp_params));
*        dhcp_params.netif = netif;
*
*        // Enable DHCP client.
*        if(fnet_dhcp_cln_init(&dhcp_params))
*        {
*            // Register DHCP event handler callbacks.
*            fnet_dhcp_cln_set_callback_updated(fapp_dhcp_cln_callback_updated_boottime, shl);
*            fnet_dhcp_cln_set_callback_discover(fapp_dhcp_cln_callback_discover, shl);
*        }
*        else
*        {
*            fnet_printf("ERROR: DHCP initialization is failed!");
*        }
*    }
* ...
* @endcode
* @n
* Configuration parameters:
* - @ref FNET_CFG_DHCP_CLN
* - @ref FNET_CFG_DHCP_CLN_BOOTP
* - @ref FNET_CFG_DHCP_CLN_PORT
* - @ref FNET_CFG_DHCP_SRV_PORT
* - @ref FNET_CFG_DHCP_CLN_RESPONSE_TIMEOUT_MS
* - @ref FNET_CFG_DNS
* - @ref FNET_CFG_DHCP_CLN_BROADCAST
*
*/
/*! @{ */

/**************************************************************************/ /*!
 * @def FNET_DHCP_CLN_LEASE_INFINITY
 * @brief Infinite lease time.@n
 * The lease-option value, when a client acquires a lease for a network
 * address for infinite time.
 * @showinitializer
 ******************************************************************************/
#define FNET_DHCP_CLN_LEASE_INFINITY    (0xFFFFFFFFU)

/**************************************************************************/ /*!
 * @brief Minimum lease-time value in seconds.@n
 * If a client-acquired lease time is less than the @ref FNET_DHCP_CLN_LEASE_MIN,
 * the lease time will be pushed to the @ref FNET_DHCP_CLN_LEASE_MIN value.
 ******************************************************************************/
#define FNET_DHCP_CLN_LEASE_MIN         (180U)

/**************************************************************************/ /*!
 * @brief Initialization parameters for the @ref fnet_dhcp_cln_init() function.
 *
 * The user application may use this structure to suggest a network address and/or
 * the lease time to the DHCP client service.@n
 ******************************************************************************/
typedef struct
{
    fnet_netif_desc_t   netif;                  /**< @brief Network interface descriptor to be used by the DHCP client.*/
    struct fnet_in_addr requested_ip_address;   /**< @brief Suggested IPv4 address (in network byte order).@n
                                                * The client can suggest to the DHCP server
                                                * that a particular IP address value should be
                                                * assigned to the client.@n
                                                * This parameter is optional and can be set to @c 0.*/
    fnet_uint32_t       requested_lease_time;   /**< @brief Suggested Lease time in seconds (in network byte order).@n
                                                * The client can suggest to the DHCP server
                                                * that a particular lease time should be
                                                * assigned to the client IP address.@n
                                                * This parameter is optional and can be set to @c 0.*/
    fnet_bool_t         probe_addr;             /**< @brief  Probing of the newly received address with ARP:
                                                *   - @c FNET_FALSE = is disabled.
                                                *   - @c FNET_TRUE = is enabled.*/
} fnet_dhcp_cln_params_t;

/**************************************************************************/ /*!
 * @brief DHCP options are retrieved from a DHCP server.
 *
 * This structure is used by the @ref fnet_dhcp_cln_get_options() function.@n
 * User application should never assign retrieved IP parameters directly
 * to network interface. The DHCP client service does it automatically.
 ******************************************************************************/
struct fnet_dhcp_cln_options
{
    struct fnet_in_addr ip_address;  /**< @brief Client IP address assigned by the DHCP server (in network byte order). */
    struct fnet_in_addr netmask;     /**< @brief Subnet Mask (in network byte order).*/
    struct fnet_in_addr gateway;     /**< @brief The IP address of a router on the client's subnet (in network byte order).*/
    struct fnet_in_addr dhcp_server; /**< @brief The DHCP server IP address (in network byte order).*/
#if FNET_CFG_DHCP_CLN_BROADCAST
    struct fnet_in_addr broadcast;   /**< @brief Broadcast address in use on the client's subnet (in network byte order).@n
                                    * This option is present only if @ref FNET_CFG_DHCP_CLN_BROADCAST is set.*/
#endif
#if FNET_CFG_DNS
    struct fnet_in_addr dns;         /**< @brief DNS (Domain Name System) server address (in network byte order).@n
                                * The DHCP client stores only the first DNS address, even if the DHCP server has
                                * provided several DNS addresses.@n
                                * This option is present only if @ref FNET_CFG_DNS is set.*/
#endif
    /* For debug needs: */
    fnet_uint32_t t1;           /**< @brief Renewal (T1) Time Value in seconds (in network byte order).@n
                                * This option specifies the time interval from
                                * address assignment, until the client transitions
                                * to the RENEWING state.@n
                                * A user application may ignore this option value.
                                * It is used for internal and debug purposes only.*/
    fnet_uint32_t t2;           /**< @brief Rebinding (T2) Time Value in seconds (in network byte order).@n
                                * This option specifies the time interval from
                                * address assignment until the client transitions
                                * to the REBINDING state.@n
                                * A user application may ignore this option value.
                                * It is used for internal and debug purposes only.*/
    fnet_uint32_t lease_time;   /**< @brief The IP Address Lease Time in seconds (in network byte order).@n
                                * @c t1 < @c t2 < @c lease_time.@n
                                * By default, @c t1=0.5*lease_time; @c t2=0.875*lease_time.@n
                                * A user application may ignore this option value.
                                * It is used for internal and debug purposes only.*/
};

#if defined(__cplusplus)
extern "C" {
#endif

/**************************************************************************/ /*!
 * @brief DHCPv4 client descriptor.
 * @see fnet_dhcp_cln_init()
 ******************************************************************************/
typedef void *fnet_dhcp_cln_desc_t;

/***************************************************************************/ /*!
 *
 * @brief    Initialize the DHCPv4 client service.
 *
 * @param params     Initialization parameters.
 *
 * @return This function returns:
 *   - DHCP client descriptor if no error occurs.
 *   - @c 0 if an error occurs.
 *
 * @see fnet_dhcp_cln_release()
 *
 ******************************************************************************
 *
 * This function initializes the DHCPv4 client service. It allocates all
 * resources needed and registers the DHCP service in the polling list.@n
 * After the initialization, the user application should call the main polling
 * function @ref fnet_service_poll() periodically to run the DHCP service routine
 * in the background.
 *
 ******************************************************************************/
fnet_dhcp_cln_desc_t fnet_dhcp_cln_init( fnet_dhcp_cln_params_t *params );

/***************************************************************************/ /*!
 *
 * @brief    Release the DHCPv4 client service.
 *
 * @param desc     DHCP client descriptor to be released.
 *
 * @see fnet_dhcp_cln_init()
 *
 ******************************************************************************
 *
 * This function releases the DHCPv4 client service. It releases all resources
 * used by the service, and unregisters it from the polling list.
 *
 ******************************************************************************/
void fnet_dhcp_cln_release(fnet_dhcp_cln_desc_t desc);

/***************************************************************************/ /*!
 *
 * @brief    Retrieve the current DHCPv4 client options retrieved from a
 *           DHCP server.
 *
 * @param desc      DHCP client descriptor.
 * @param options   DHCP options received from a DHCP server.
 *
 ******************************************************************************
 *
 * This function returns the DHCPv4 @c options that were retrieved
 * by the DHCPv4 client from a DHCPv4 server.@n
 * These options are valid after the DHCP client reaches the BOUND state, or
 * after the @c callback_updated event.
 *
 ******************************************************************************/
void fnet_dhcp_cln_get_options(fnet_dhcp_cln_desc_t desc, struct fnet_dhcp_cln_options *options );

/**************************************************************************/ /*!
 * @brief DHCP client event handler callback function prototype, that is
 * called when the DHCP client has updated the IPv4 parameters (in BOUND state) or sends the DHCP discover message.
 *
 * @param desc      DHCP client descriptor.
 * @param netif     Network interface descriptor which IP parameters were updated.
 * @param cookie     User-application specific parameter. It's set during
 *                  the @ref fnet_dhcp_cln_set_callback_updated() and the fnet_dhcp_cln_set_callback_discover() call.
 *
 * @see fnet_dhcp_cln_set_callback_updated(), fnet_dhcp_cln_set_callback_discover()
 ******************************************************************************/
typedef void(*fnet_dhcp_cln_callback_t)(fnet_dhcp_cln_desc_t desc, fnet_netif_desc_t netif, void *cookie);

/***************************************************************************/ /*!
 *
 * @brief    Register the "IP parameters updated" DHCP event handler callback.
 *
 * @param desc          DHCP client descriptor.
 * @param callback_updated Pointer to the callback function defined by
 *                      @ref fnet_dhcp_cln_callback_t.
 * @param cookie        Optional application-specific parameter. @n
 *                      It's passed to the @c callback_updated callback
 *                      function as input parameter.
 *
 ******************************************************************************
 *
 * This function registers the @c callback_updated callback function for
 * the "IP parameters updated" event. This event occurs when
 * the DHCP client has updated the IP parameters. @n
 * To stop the event handling, set the @c callback_updated parameter to zero value.
 *
 ******************************************************************************/
void fnet_dhcp_cln_set_callback_updated (fnet_dhcp_cln_desc_t desc, fnet_dhcp_cln_callback_t callback_updated, void *cookie);

/***************************************************************************/ /*!
 *
 * @brief    Register the "Discover message sent" DHCP event handler callback.
 *
 * @param desc              DHCP client descriptor.
 * @param callback_discover  Pointer to the callback function defined by
 *                          @ref fnet_dhcp_cln_callback_t.
 * @param cookie            Optional application-specific parameter. @n
 *                          It's passed to the @c callback_discover callback
 *                          function as input parameter.
 *
 * @see FNET_CFG_DHCP_CLN_RESPONSE_TIMEOUT_MS
 *
 ******************************************************************************
 *
 * This function registers the @c callback_discover callback function for
 * the "Discover message sent" event. This event occurs when
 * the DHCP client sends the DHCP discover message. A user interface program
 * may use it for indication of retransmission attempts. @n
 * To stop the event handling, set the @c callback_discover parameter to zero value.
 *
 ******************************************************************************/
void fnet_dhcp_cln_set_callback_discover (fnet_dhcp_cln_desc_t desc, fnet_dhcp_cln_callback_t callback_discover, void *cookie);

/***************************************************************************/ /*!
 *
 * @brief    Change timeout for a DHCP server response.
 *
 * @param desc                  DHCP client descriptor.
 *
 * @param response_timout_ms    Response timeout (in milliseconds).
 *
 * @see FNET_CFG_DHCP_CLN_RESPONSE_TIMEOUT_MS
 *
 ******************************************************************************
 *
 * This function sets the timeout for a response from a remote DHCP server.
 * If the DHCP client does not receive any response from a DHCP server during this time,
 * the client sends new discover message.@n
 * If the response_timout_ms is 0, the function reset the timeout to @ref FNET_CFG_DHCP_CLN_RESPONSE_TIMEOUT_MS. @n
 * Using of this function is optional, the default timeout value is defined by
 * @ref FNET_CFG_DHCP_CLN_RESPONSE_TIMEOUT_MS.
 *
 ******************************************************************************/
void fnet_dhcp_cln_set_response_timeout(fnet_dhcp_cln_desc_t desc, fnet_time_t response_timout_ms);

/***************************************************************************/ /*!
 *
 * @brief    Detect if the DHCP Client service is enabled or disabled.
 *
 * @param desc     DHCP Client descriptor
 *
 * @return This function returns:
 *          - @ref FNET_TRUE if the DHCP Client is successfully initialized.
 *          - @ref FNET_FALSE if the DHCP Client is not initialized or is released.
 *
 ******************************************************************************
 *
 * This function detects if the DHCP Client service is initialized or is released.
 *
 ******************************************************************************/
fnet_bool_t fnet_dhcp_cln_is_enabled(fnet_dhcp_cln_desc_t desc);

/***************************************************************************/ /*!
 *
 * @brief    Look for a DHCP Client assigned to the specified network interface.
 *
 * @param netif      Network interface descriptor.
 *
 * @return This function returns:
 *          - DHCP client descriptor assigned to the @c netif network interface.
 *          - @ref FNET_NULL if there is no any DHCP client assigned to the network interface.
 *
 * @see fnet_dhcp_cln_init
 *
 ******************************************************************************
 *
 * This function is looking for a DHCP Client service assigned to
 * the @c netif network interface set during its initialization.
 *
 ******************************************************************************/
fnet_dhcp_cln_desc_t fnet_dhcp_cln_get_by_netif(fnet_netif_desc_t netif);

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* FNET_CFG_DHCP_CLN */

#endif /* _FNET_DHCP_CLN_H_ */
