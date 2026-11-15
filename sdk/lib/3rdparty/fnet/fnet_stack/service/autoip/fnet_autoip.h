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
***************************************************************************/

#ifndef _FNET_AUTOIP_H_
#define _FNET_AUTOIP_H_

#if FNET_CFG_AUTOIP || defined(__DOXYGEN__)

/*! @addtogroup fnet_autoip
*
* Autop-IP service is used to automatically configure an interface with an IPv4 link-local address within the
* 169.254/16 prefix that is valid for communication with other devices
* connected to the same physical (or logical) link. @n
* IPv4 Link-Local addresses are not suitable for communication with
* devices not directly connected to the same physical (or logical)
* link, and are only used where stable, routable addresses are not
* available (such as on ad hoc or isolated networks).@n
* Upon receiving a conflicting ARP packet, a Auto-IP service
* immediately configure a new IPv4 Link-Local address.@n
* The protocol specification is defined by RFC3927.@n
* @n
* After the Auto-IP service is initialized by calling the @ref fnet_autoip_init() function,
* the user application should call the main service-polling function
* @ref fnet_service_poll() periodically in background. @n
* @n
* The Auto-IP service is released automatically if an application will change
* the interface IPv4-adress parameters manually or start DHCP service.
* @n
* For the Auto-IP service example, refer to the FNET Shell demo source code.@n
*
* Configuration parameters:
* - @ref FNET_CFG_AUTOIP
* - @ref FNET_CFG_AUTOIP_DEFEND_INTERVAL
*
*/

/*! @{ */

/**************************************************************************/ /*!
 * @def FNET_AUTOIP_NETWORK
 * @brief The Link-Local network address 169.254.0.0/16.
 * @showinitializer
 ******************************************************************************/
#define FNET_AUTOIP_NETWORK     FNET_IP4_ADDR_INIT(169U, 254U, 0U, 0U)
/**************************************************************************/ /*!
 * @def FNET_AUTOIP_NETMASK
 * @brief The Link-Local netmask 255.255.0.0/16.
 * @showinitializer
 ******************************************************************************/
#define FNET_AUTOIP_NETMASK     FNET_IP4_ADDR_INIT(255U, 255U, 0U, 0U)

/**************************************************************************/ /*!
 * @brief Auto-IP service descriptor.
 * @see fnet_autoip_init()
 ******************************************************************************/
typedef void *fnet_autoip_desc_t;

/**************************************************************************/ /*!
 * @brief Auto-IP event handler callback function prototype, that is
 * called when the Auto-IP service has updated the IPv4 parameters or adress probing.
 *
 * @param desc      Auto-IP service descriptor.
 * @param netif     Network interface descriptor which IPv4 parameters were updated.
 * @param cookie    User-application specific parameter. It's set during
 *                  the @ref fnet_autoip_set_callback_updated() call.
 *
 * @see fnet_autoip_set_callback_updated(), fnet_autoip_set_callback_probe()
 ******************************************************************************/
typedef void(*fnet_autoip_callback_t)(fnet_autoip_desc_t desc, fnet_netif_desc_t netif, void *cookie);

/**************************************************************************/ /*!
 * @brief Initialization parameters for the @ref fnet_autoip_init() function.
 ******************************************************************************/
typedef struct
{
    fnet_netif_desc_t   netif_desc;    /**< @brief Network interface descriptor to be used by the Link-Local service.*/
    fnet_ip4_addr_t     ip_address;    /**< @brief Suggested IP address.@n
                                       * For example, it may be used for the case when the network media indicates that it has been connected, the
                                       * autoconfiguration process begins again, and attempts to re-use the
                                       * previously assigned Link-Local address.@n
                                       * It must be the Link-Local network address 169.254.0.0/16, otherwise it will be ignored. @n
                                       * This parameter is optional and can be set to @c 0.*/
} fnet_autoip_params_t;

/***************************************************************************/ /*!
 *
 * @brief    Initializes the Auto-IP service.
 *
 * @param params     Initialization parameters defined by @ref fnet_autoip_params_t.
 *
 * @return This function returns:
 *   - Auto-IP service descriptor if no error occurs.
 *   - @c 0 if an error occurs.
 *
 * @see fnet_autoip_release(), fnet_autoip_params_t
 *
 ******************************************************************************
 *
 * This function starts the IPv4 Link-Local address configuration procedur
 * on @c netif interface.
 *
 ******************************************************************************/
fnet_autoip_desc_t fnet_autoip_init( fnet_autoip_params_t *params );

/***************************************************************************/ /*!
 *
 * @brief    Releases the Auto-IP service.
 *
 * @param desc     Auto-IP service descriptor to be released.
 *
 * @see fnet_autoip_init()
 *
 ******************************************************************************
 *
 * This function releases the Auto-IP service. It removes the
 * link-local address from the interface, releases all resources
 * used by the service, and unregisters it from the polling list.
 *
 ******************************************************************************/
void fnet_autoip_release(fnet_autoip_desc_t desc);

/***************************************************************************/ /*!
 *
 * @brief    Registers the "IPv4 parameters updated" Auto-IP event handler callback.
 *
 * @param desc              Auto-IP service descriptor.
 * @param callback_updated  Pointer to the callback function defined by
 *                          @ref fnet_autoip_callback_t.
 * @param cookie            Optional application-specific parameter. @n
 *                          It's passed to the @c callback_updated callback
 *                          function as input parameter.
 *
 ******************************************************************************
 *
 * This function registers the @c callback_updated callback function for
 * the "IPv4 parameters updated" event. This event occurs when
 * the Auto-IP client has updated the IP parameters. @n
 * To stop the event handling, set the @c callback_updated parameter to zero value.
 *
 ******************************************************************************/
void fnet_autoip_set_callback_updated (fnet_autoip_desc_t desc, fnet_autoip_callback_t callback_updated, void *cookie);

/***************************************************************************/ /*!
 *
 * @brief    Registers the "Address probing" Auto-IP event handler callback.
 *
 * @param desc              Auto-IP client descriptor.
 * @param callback_probe    Pointer to the callback function defined by
 *                          @ref fnet_autoip_callback_t.
 * @param param             Optional application-specific parameter. @n
 *                          It's passed to the @c callback_probe callback
 *                          function as input parameter.
 *
 ******************************************************************************
 *
 * This function registers the @c handler callback_probe function for
 * the "Address probing" event. This event occurs when
 * the Auto-IP service sends the ARP probe message. A user interface program
 * may use it for indication of adress-probing attempts. @n
 * To stop the event handling, set the @c callback_probe parameter to zero value.
 *
 ******************************************************************************/
void fnet_autoip_set_callback_probe (fnet_autoip_desc_t desc, fnet_autoip_callback_t callback_probe, void *param);

/***************************************************************************/ /*!
 *
 * @brief    Detects if the Auto-IP service is enabled or disabled.
 *
 * @param desc     Auto-IP service descriptor
 *
 * @return This function returns:
 *          - @ref FNET_TRUE if the Auto-IP service is successfully initialized.
 *          - @ref FNET_FALSE if the Auto-IP service is not initialized or is released.
 *
 ******************************************************************************
 *
 * This function detects if the Auto-IP service is initialized or is released.
 *
 ******************************************************************************/
fnet_bool_t fnet_autoip_is_enabled(fnet_autoip_desc_t desc);

/***************************************************************************/ /*!
 *
 * @brief    Looks for a Auto-IP service assigned to the specified network interface.
 *
 * @param netif      Network interface descriptor.
 *
 * @return This function returns:
 *          - Auto-IP service descriptor assigned to the @c netif network interface.
 *          - @ref FNET_NULL if there is no any Auto-IP service assigned to the network interface.
 *
 * @see fnet_autoip_init
 *
 ******************************************************************************
 *
 * This function is looking for a Auto-IP service assigned to
 * the @c netif network interface set during its initialization.
 *
 ******************************************************************************/
fnet_autoip_desc_t fnet_autoip_get_by_netif(fnet_netif_desc_t netif);

/*! @} */

#endif /* FNET_CFG_AUTOIP || defined(__DOXYGEN__) */

#endif /* _FNET_AUTOIP_H_ */
