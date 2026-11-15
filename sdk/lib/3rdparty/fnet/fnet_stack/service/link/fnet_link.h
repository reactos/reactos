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

#ifndef _FNET_LINK_H_
#define _FNET_LINK_H_

#if FNET_CFG_LINK || defined(__DOXYGEN__)

/*! @addtogroup fnet_link
*
* Link-Detection service is used for automatic detection of a network-link connection
* status change from connected to disconnected state and vice versa.
* @n
* After the Link-Detection service is initialized by calling the @ref fnet_link_init() function,
* the user application should call the main service-polling function
* @ref fnet_service_poll() periodically in background.@n
* @n
* For the Link-Detection service example, refer to the FNET Shell demo source code.@n
*
* Configuration parameters:
* - @ref FNET_CFG_LINK
*/

/*! @{ */

/**************************************************************************/ /*!
 * @brief Link-Detection service descriptor.
 * @see fnet_link_detection_init()
 ******************************************************************************/
typedef fnet_int32_t fnet_link_desc_t;

/**************************************************************************/ /*!
 * @brief Link-Detection event handler callback function prototype, which is
 * called when a networking interface has changed its link status from
 * connected stae to disconnected or vice versa.
 *
 * @param netif             Network interface descriptor.
 * @param connected         Link status of the network interface.
 * @param callback_param    User-application specific parameter. It's set during
 *                          the @ref fnet_link_init() call.
 *
 * @see fnet_link_init()
 ******************************************************************************/
typedef void(*fnet_link_callback_t)(fnet_netif_desc_t netif, fnet_bool_t connected, void *callback_param);

/**************************************************************************/ /*!
 * @brief Initialization parameters for the @ref fnet_llmnr_init() function.
 ******************************************************************************/
typedef struct
{
    fnet_netif_desc_t       netif_desc;         /**< @brief Network interface descriptor to be monitored by the Link-Detection service.*/
    fnet_link_callback_t    callback;           /**< @brief Pointer to the callback function defined by @ref fnet_link_callback_t. */
    void                    *callback_param;    /**< @brief Optional application-specific parameter. @n It's passed to the @c callback function as input parameter. */
} fnet_link_params_t;

/***************************************************************************/ /*!
 *
 * @brief    Initializes the Link-Detection service.
 *
 * @param params     Initialization parameters defined by @ref fnet_llmnr_params_t.
 *
 * @return This function returns:
 *   - Link-Detection service descriptor if no error occurs.
 *   - @c 0 if an error occurs.
 *
 * @see fnet_link_release(), fnet_link_params_t
 *
 ******************************************************************************
 *
 * This function starts the Link-Detection service on @c netif interface.
 * It allocates all needed resources and registers the Link-Detection service in the polling list.@n
 * After the initialization, the user application should call the main polling
 * function @ref fnet_service_poll() periodically to run the service routine
 * in the background.@n
 * It is posible to run several services on the same networking interface.
 *
 ******************************************************************************/
fnet_link_desc_t fnet_link_init( fnet_link_params_t *params );

/***************************************************************************/ /*!
 *
 * @brief    Releases the Link-Detection service.
 *
 * @param desc     Link-Detection service descriptor to be released.
 *
 * @see fnet_link_init()
 *
 ******************************************************************************
 *
 * This function releases the Link-Detection service.
 *
 ******************************************************************************/
void fnet_link_release(fnet_link_desc_t desc);

/***************************************************************************/ /*!
 *
 * @brief    Detects if the Link-Detection service is enabled or disabled.
 *
 * @param desc     Link-Detection service descriptor
 *
 * @return This function returns:
 *          - @ref FNET_TRUE if the Link-Detection service is successfully initialized.
 *          - @ref FNET_FALSE if the Link-Detection service is not initialized or is released.
 *
 ******************************************************************************
 *
 * This function detects if the Link-Detection service is initialized or is released.
 *
 ******************************************************************************/
fnet_bool_t fnet_link_is_enabled(fnet_link_desc_t desc);

/***************************************************************************/ /*!
 *
 * @brief    Looks for a Link-Detection service assigned to the specified network interface.
 *
 * @param netif      Network interface descriptor.
 *
 * @return This function returns:
 *          - Link-Detection service descriptor assigned to the @c netif network interface.
 *          - @ref FNET_NULL if there is no any Link-Detection service assigned to the network interface.
 *
 * @see fnet_link_init
 *
 ******************************************************************************
 *
 * This function is looking for a Link-Detection service assigned to
 * the @c netif network interface set during its initialization.
 *
 ******************************************************************************/
fnet_link_desc_t fnet_link_get_by_netif(fnet_netif_desc_t netif);

/*! @} */

#endif /* FNET_CFG_LINK || defined(__DOXYGEN__) */

#endif /* _FNET_AUTOIP_H_ */
