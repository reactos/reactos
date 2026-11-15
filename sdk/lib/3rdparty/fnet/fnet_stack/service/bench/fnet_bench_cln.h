/**************************************************************************
*
* Copyright 2018 by Andrey Butok. FNET Community.
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
*  FNET benchmark client API.
*
***************************************************************************/

#ifndef _FNET_BENCH_CLN_H_

#define _FNET_BENCH_CLN_H_

#if FNET_CFG_BENCH_CLN || defined(__DOXYGEN__)

/*! @addtogroup fnet_bench_cln
*
* The FNET benchmark client/transmitter is used for performance measuring and stress test of TCP and UDP. @n
* After the FNET benchmark client is initialized by calling the @ref fnet_bench_cln_init() function,
* the user application should call the main service-polling function
* @ref fnet_service_poll() periodically in background.@n
* The benchmark results will be passed to the @ref fnet_bench_cln_callback_session_end_t callback function,
* which is set during the benchmark client initialization.
* @n
* The benchmark client service is released automatically as soon as the benchmark session is
* finished or an error occurs. Your application may continue
* to call @ref fnet_service_poll() to handle other services, but this will not have any
* impact on the benchmark client communication until you initialize the next benchmark
* @ref fnet_bench_cln_init() again. @n
* @n
* For the FNET benchmark client example, refer to the FNET Bench demo source code.@n
*
* Configuration parameters:
* - @ref FNET_CFG_BENCH_CLN
* - @ref FNET_CFG_BENCH_CLN_PORT
* - @ref FNET_CFG_BENCH_CLN_BUFFER_SIZE
*/

/*! @{ */

/**************************************************************************/ /*!
 * @brief Benchmark client descriptor.
 * @see fnet_bench_cln_init()
 ******************************************************************************/
typedef void *fnet_bench_cln_desc_t;

/**************************************************************************/ /*!
 * @brief Benchmark client result passed to the "session end" event handler callback.
 * @see fnet_bench_cln_set_callback_session_end()
 ******************************************************************************/
typedef struct
{
    fnet_size_t megabytes;              /**< @brief Number of transmitted megabytes during the benchmark session.*/
    fnet_size_t bytes;                  /**< @brief Number of transmitted bytes, in addition to megabytes, during the benchmark session.*/
    fnet_time_t time_ms;                /**< @brief Benchmark session lifetime in milliseconds.*/
} fnet_bench_cln_result_t;

/**************************************************************************/ /*!
 * @brief Benchmark client event handler callback function prototype, that is
 * called when the benchmark client has finished the benchmarking session.
 *
 * @param bench_cln_desc        Benchmark client descriptor.
 * @param bench_cln_result      Benchmark session result.
 * @param cookie                User-application specific parameter. It's set during
 *                              the @ref fnet_bench_cln_init.
 ******************************************************************************/
typedef void(*fnet_bench_cln_callback_session_end_t)(fnet_bench_cln_desc_t bench_cln_desc, const fnet_bench_cln_result_t *bench_cln_result, void *cookie);

/**************************************************************************/ /*!
 * @brief Initialization parameters for the @ref fnet_bench_cln_init() function.
 ******************************************************************************/
typedef struct
{
    struct fnet_sockaddr                    address;            /**< @brief Socket address of the remote benchmark server to connect to.@n
                                                                 * If the address port number is set to @c 0, it will be assigned to the default port number defined by @ref FNET_CFG_BENCH_CLN_PORT.*/
    fnet_socket_type_t                      type;               /**< @brief Protocol type. It can be @ref SOCK_STREAM (TCP) or @ref SOCK_DGRAM (UDP).*/
    fnet_size_t                             message_size;       /**< @brief Size of one message to send. Its value must be less than @ref FNET_CFG_BENCH_CLN_BUFFER_SIZE. */
    fnet_index_t                            message_number;     /**< @brief Number of messages to send. */
    fnet_bench_cln_callback_session_end_t   callback;           /**< @brief Pointer to the callback function defined by
                                                                * @ref fnet_bench_cln_callback_session_end_t. It is called when the
                                                                * benchmark session is finished. */
    void                                    *cookie;            /**< @brief Optional application-specific parameter. @n
                                                                * It's passed to the @c callback
                                                                * function as input parameter. */
} fnet_bench_cln_params_t;

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************************************************/ /*!
 *
 * @brief    Initializes the Benchmark client service and starts the benchmark session.
 *
 * @param params     Initialization parameters defined by @ref fnet_bench_cln_params_t.
 *
 * @return This function returns:
 *   - Benchmark client descriptor if no error occurs.
 *   - @c FNET_NULL if an error occurs.
 *
 * @see fnet_bench_cln_release()
 *
 ******************************************************************************
 *
 * This function initializes the Benchmark client/transmitter service and starts
 * the benchmark session. It allocates all
 * resources needed and registers the Benchmark client in the service polling list.@n
 * After the initialization, the user application should call the main polling
 * function @ref fnet_service_poll() periodically to run the Benchmark client service
 * in the background.@n
 * The benchmark results will be passed to the @ref fnet_bench_cln_callback_session_end_t callback function,
 * which is set @c params.@n
 * The Benchmark service is released automatically as soon as the
 * session is finished.
 *
 ******************************************************************************/
fnet_bench_cln_desc_t fnet_bench_cln_init( fnet_bench_cln_params_t *params );

/***************************************************************************/ /*!
 *
 * @brief    Aborts the benchmark session and releases the benchmark client service.
 *
 * @param desc     Benchmark client descriptor to be released.
 *
 * @see fnet_bench_cln_init()
 *
 ******************************************************************************
 *
 * This function releases the Benchmark client. It releases all resources
 * used by the service, and unregisters it from the service polling list.@n
 * Use this function only in the case of the early termination of the service,
 * because the Benchmark client service is released automatically as soon as the
 * benchmark session is finished.
 *
 ******************************************************************************/
void fnet_bench_cln_release(fnet_bench_cln_desc_t desc);

/***************************************************************************/ /*!
 *
 * @brief    Detects if the Benchmark client service is enabled or disabled.
 *
 * @param desc     Benchmark client descriptor
 *
 * @return This function returns:
 *          - @ref FNET_TRUE if the Benchmark client is successfully initialized.
 *          - @ref FNET_FALSE if the Benchmark client is not initialized or is released.
 *
 ******************************************************************************
 *
 * This function detects if the Benchmark client service is initialized or is released.
 *
 ******************************************************************************/
fnet_bool_t fnet_bench_cln_is_enabled(fnet_bench_cln_desc_t desc);

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* FNET_CFG_BENCH_CLN */

#endif /* _FNET_BENCH_CLN_H_ */
