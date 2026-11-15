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
*  SNTP Client API.
*
***************************************************************************/

#ifndef _FNET_SNTP_H_

#define _FNET_SNTP_H_

#if FNET_CFG_SNTP || defined(__DOXYGEN__)

/*! @addtogroup fnet_sntp
*
* The SNTP (Simple Network Time Protocol) client service allows a user application to obtain a timestamp from a remote NTP server. @n
* The client implements SNTP Version 4 defined by RFC4330.@n
* After the SNTP client is initialized by calling the @ref fnet_sntp_init() function,
* the user application should call the main service-polling function
* @ref fnet_service_poll() periodically in background. @n
* The obtained timestamp will be passed to the @ref fnet_sntp_callback_resolved_t callback function,
* which is set during the SNTP-client service initialization.
* @n
* The SNTP client service is released automatically as soon as the requested timestamp
* obtained or an error occurs. Your application code may continue
* to call @ref fnet_service_poll() to handle other services, but this will not have any
* impact on the SNTP client communication until you initialize the next timestamp obtaining by calling
* @ref fnet_sntp_init() again. @n
* @n
* For the SNTP-client service example, refer to the FNET Shell demo source code.@n
* @note
* Current version of the SNTP client:
*  - can process only one request at a time.
*  - support unicast addressing mode. It does not support broadcast or multicast addressing mode.
*
* Configuration parameters:
* - @ref FNET_CFG_SNTP
* - @ref FNET_CFG_SNTP_PORT
* - @ref FNET_CFG_SNTP_RETRANSMISSION_MAX
* - @ref FNET_CFG_SNTP_RETRANSMISSION_TIMEOUT
*/

/*! @{ */

/**************************************************************************/ /*!
 * @brief SNTP timestamp (RFC4330) is represented as a 64-bit unsigned fixed-point number,
 * in seconds relative to 0h on 1 January 1900.
 * The integer part is in the first 32 bits, and the fraction part in the
 * last 32 bits. The maximum number that can be represented is 4,294,967,295 seconds with
 * a precision of about 232 picoseconds
 ******************************************************************************/
typedef struct
{
    fnet_uint32_t   seconds;            /**< @brief Seconds (host endian).*/
    fnet_uint32_t   seconds_fraction;   /**< @brief Seconds Fraction (host endian).*/
} fnet_sntp_timestamp_t;

/**************************************************************************/ /*!
 * @brief Coordinated Universal Time (UTC) type.
 * @see fnet_sntp_timestamp2utc()
 ******************************************************************************/
typedef struct
{
    fnet_uint32_t   year;                /**< @brief Year (1968-2104).*/
    fnet_uint32_t   month;               /**< @brief Month (1-12).*/
    fnet_uint32_t   day;                 /**< @brief Day of the month (1-31) .*/
    fnet_uint32_t   hour;                /**< @brief Hour (0-23).*/
    fnet_uint32_t   minute;              /**< @brief Minutes (0-59).*/
    fnet_uint32_t   second;              /**< @brief Seconds (0-59).*/
    fnet_uint32_t   millisecond;         /**< @brief Milliseconds (0-999).*/
} fnet_sntp_utc_t;

/**************************************************************************/ /*!
 * @brief Prototype of the SNTP-client callback function that is
 * called when the SNTP client has completed the timestamp resolving.
 *
 * @param timestamp         Pointer to the received timestamp or @ref FNET_NULL if the resolving is failed.
 * @param cookie            User-application specific parameter. It's set during
 *                          the SNTP-client service initialization as part of
 *                          @ref fnet_sntp_params_t.
 *
 * @see fnet_sntp_init(), fnet_sntp_params_t
 ******************************************************************************/
typedef void(*fnet_sntp_callback_resolved_t)(const fnet_sntp_timestamp_t *timestamp, void *cookie);

/**************************************************************************/ /*!
 * @brief Initialization parameters for the @ref fnet_sntp_init() function.
 ******************************************************************************/
typedef struct
{
    struct fnet_sockaddr            sntp_server_addr;   /**< @brief Socket address of the remote SNTP/NTP server to
                                                        * connect to. */
    fnet_sntp_callback_resolved_t   callback;          /**< @brief Pointer to the callback function defined by
                                                        * @ref fnet_sntp_callback_resolved_t. It is called when the
                                                        * SNTP-client resolving is finished or a timeout is occurred. */
    void                            *cookie;            /**< @brief Optional application-specific parameter. @n
                                                        * It's passed to the @c callback
                                                        * function as input parameter. */
} fnet_sntp_params_t;

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************************************************/ /*!
 *
 * @brief    Initializes the SNTP client service and starts the time resolving.
 *
 * @param params     Initialization parameters.
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs.
 *
 * @see fnet_sntp_params_t, fnet_sntp_callback_resolved_t, fnet_sntp_release()
 *
 ******************************************************************************
 *
 * This function initializes the SNTP client service and starts the
 * time resolving. It allocates all needed
 * resources and registers the SNTP service in the polling list.@n
 * After the initialization, the user application should call the main polling
 * function @ref fnet_service_poll() periodically to run the SNTP service routine
 * in the background.@n
 * The resolved timestamp will be passed to the @ref fnet_sntp_callback_resolved_t callback function,
 * which is set in @c params. @n
 * The SNTP service is released automatically as soon as the
 * resolving is finished or a timeout is occurred. The timeout equals to
 * FNET_CFG_SNTP_RETRANSMISSION_TIMEOUT * FNET_CFG_SNTP_RETRANSMISSION_MAX seconds.
 *
 ******************************************************************************/
fnet_return_t fnet_sntp_init( fnet_sntp_params_t *params );

/***************************************************************************/ /*!
 *
 * @brief    Aborts the time resolving and releases the SNTP-client service.
 *
 * @see fnet_sntp_init()
 *
 ******************************************************************************
 *
 * This function stops the SNTP-client service. It releases all resources
 * used by the service, and unregisters it from the polling list.@n
 * Use this function only in the case of the early termination of the service,
 * because the SNTP service is released automatically as soon as the
 * resolving is finished.
 *
 ******************************************************************************/
void fnet_sntp_release(void);

/***************************************************************************/ /*!
 *
 * @brief    Converts the SNTP timestamp to the Coordinated Universal Time (UTC).
 *
 * @param timestamp     SNTP timestamp.
 * @param utc           UTC time.
 *
 * @see fnet_sntp_callback_resolved_t
 *
 ******************************************************************************
 *
 * This function converts the SNTP timestamp to the Coordinated Universal Time (UTC).
 * The SNTP timestamp is provided by the SNTP-client callback function.
 *
 ******************************************************************************/
void fnet_sntp_timestamp2utc(const fnet_sntp_timestamp_t *timestamp, fnet_sntp_utc_t *utc);

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* FNET_CFG_SNTP */

#endif /* _FNET_SNTP_H_ */
