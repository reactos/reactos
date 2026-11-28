/**************************************************************************
*
* Copyright 2011-2018 by Andrey Butok. FNET Community.
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
* PING API.
*
***************************************************************************/

#ifndef _FNET_PING_H_

#define _FNET_PING_H_

#if FNET_CFG_PING || defined(__DOXYGEN__)

/*! @addtogroup fnet_ping
*
* The PING service is used to test if a system can communicate over the
* network with another device or computer.@n
* It operates by sending Internet Control Message Protocol (ICMPv4/v6) Echo Request
* messages to the destination device and waiting for a response.
* @n
* After the PING service is initialized the @ref fnet_ping_init() function,
* the user application should call the main service-polling function
* @ref fnet_service_poll() periodically in background. @n
* When correct echo response is received, the PING service passes an
* echo-reply socket address to the @ref fnet_ping_callback_t callback function. @n
* For the PING service usage example, refer to the FNET Shell demo source code.@n
*
* @note The PING service uses the RAW sockets.
*
* Configuration parameters:
* - @ref FNET_CFG_PING
* - @ref FNET_CFG_PING_PACKET_MAX
* - @ref FNET_CFG_PING_IDENTIFIER
*/

/*! @{ */

/**************************************************************************/ /*!
 * @brief Prototype of the PING-service callback function that is
 * called when the PING-service has completed the requesting.
 *
 * @param result        Result of the PING-service request, which equals to:
 *                          - FNET_ERR_OK - if correct reply is received.
 *                          - FNET_ERR_TIMEDOUT - if there is no correct response during specified timeout.
 *                          - Error code, if any happened during receiving.
 * @param packet_count  Number of request packets to be sent, till the PING-service release.@n
 *                      It equals to 0, when the last packet was sent.
 * @param address       Pointer to a socket address, which equals to:
 *                          - Target socket address, if correct reply is received during specified timeout.
 *                          - @ref FNET_NULL, if there is no correct response during specified timeout.
 * @param cookie        User-application specific parameter. It's set during
 *                      the PING service initialization as part of @ref fnet_ping_params_t.
 *
 * @see fnet_ping_init(), fnet_ping_params_t
 ******************************************************************************/
typedef void(*fnet_ping_callback_t)(fnet_error_t result, fnet_size_t packet_count, struct fnet_sockaddr *target_addr, void *cookie);

/**************************************************************************/ /*!
 * @brief Initialization parameters for the @ref fnet_ping_init() function.
 ******************************************************************************/
typedef struct
{
    struct fnet_sockaddr    target_addr;    /**< @brief Socket address of the destination to ping.*/
    fnet_size_t             packet_size;    /**< @brief The size of the echo request, in bytes (without ICMP header). @n
                                            *  The maximum value is limited by @ref FNET_CFG_PING_PACKET_MAX value.*/
    fnet_size_t             packet_count;   /**< @brief Number of packets to be sent.*/
    fnet_time_t             timeout;        /**< @brief Timeout value in milliseconds, that service
                                            * waits for reply on ping request.*/
    fnet_uint8_t            ttl;            /**< @brief IPv4 Time To Live (TTL) or IPv6 Hop Limit value. @n*/
    fnet_uint8_t            pattern;        /**< @brief  Pattern byte to fill out the packet.@n
                                            *   This is useful for diagnosing data-dependent problems in a network.*/
    fnet_ping_callback_t    callback;      /**< @brief Pointer to the callback function defined by
                                            * @ref fnet_ping_callback_t. It is called when the
                                            * correct echo response is receved or timeout is occured.*/
    void                    *cookie;        /**< @brief Optional application-specific parameter. @n
                                            * It's passed to the @c callback function as input parameter.*/
} fnet_ping_params_t;

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************************************************/ /*!
 *
 * @brief    Initializes PING service.
 *
 * @param params     Initialization parameters.
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs.
 *
 * @see fnet_ping_params_t, fnet_ping_callback_t, fnet_ping_release()
 *
 ******************************************************************************
 *
 * This function initializes the PING service.
 * It allocates all resources needed and registers the PING service in
 * the polling list.@n
 * After the initialization, the user application should call the main polling
 * function @ref fnet_service_poll() periodically to run the PING service routine
 * in the background.@n
 * The @ref fnet_ping_callback_t callback function,
 * which is set in @c params, will be called when correct echo-reply is
 * received or timeout is occurred. @n
 * The PING service is released automatically.@n
 * Call fnet_ping_release() if you need to terminate it earlier.
 *
 ******************************************************************************/
fnet_return_t fnet_ping_init( fnet_ping_params_t *params );

/***************************************************************************/ /*!
 *
 * @brief    Releases the PING service.
 *
 * @see fnet_ping_init()
 *
 ******************************************************************************
 *
 * This function terminates the PING service. It releases all resources
 * used by the service, and unregisters it from the polling list.@n
 * Use this function only in the case of the early termination of the service,
 * because the PING service is released automatically as soon as the
 * pinging is finished.
 *
 ******************************************************************************/
void fnet_ping_release(void);

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* FNET_CFG_PING */

#endif /* _FNET_PING_H_ */
