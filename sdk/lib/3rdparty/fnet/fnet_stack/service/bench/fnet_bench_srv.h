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
*  FNET benchmark server API.
*
***************************************************************************/

#ifndef _FNET_BENCH_SRV_H_

#define _FNET_BENCH_SRV_H_

#if FNET_CFG_BENCH_SRV || defined(__DOXYGEN__)

/*! @addtogroup fnet_bench_srv
*
* The FNET benchmark server/receiver is used for performance measuring and stress test of TCP and UDP.@n
* After the FNET benchmark server is initialized by calling the @ref fnet_bench_srv_init() function,
* the user application should call the main service-polling function
* @ref fnet_service_poll() periodically in background.
* @n
* For the FNET benchmark server example, refer to the FNET Bench demo source code.@n
*
* Configuration parameters:
* - @ref FNET_CFG_BENCH_SRV
* - @ref FNET_CFG_BENCH_SRV_PORT
* - @ref FNET_CFG_BENCH_SRV_BUFFER_SIZE
*/

/*! @{ */

/**************************************************************************/ /*!
 * @brief Benchmark server descriptor.
 * @see fnet_bench_srv_init()
 ******************************************************************************/
typedef void *fnet_bench_srv_desc_t;

/**************************************************************************/ /*!
 * @brief Initialization parameters for the @ref fnet_bench_srv_init() function.
 ******************************************************************************/
typedef struct
{
    struct fnet_sockaddr    address;            /**< @brief Server socket address. @n
                                                * If server IP address and Scope ID are set to @c 0s, the server will listen to all current network interfaces. @n
                                                * If server address family is set to @c 0, it will be assigned to @ref AF_SUPPORTED. @n
                                                * If server port number is set to @c 0, it will be assigned to the default port number defined by @ref FNET_CFG_BENCH_SRV_PORT.*/
    fnet_socket_type_t      type;               /**< @brief Protocol type. It can be @ref SOCK_STREAM (TCP) or @ref SOCK_DGRAM (UDP).*/
} fnet_bench_srv_params_t;

/**************************************************************************/ /*!
 * @brief Benchmark server result passed to the "session end" event handler callback.
 * @see fnet_bench_srv_set_callback_session_end()
 ******************************************************************************/
struct fnet_bench_srv_result
{
    fnet_size_t megabytes;              /**< @brief Number of received megabytes during the benchmark session.*/
    fnet_size_t bytes;                  /**< @brief Number of received bytes, in addition to megabytes, during the benchmark session.*/
    fnet_time_t time_ms;                /**< @brief Benchmark session lifetime in milliseconds.*/
};

/**************************************************************************/ /*!
 * @brief Benchmark server event handler callback function prototype, that is
 * called when the benchmark server has started a new benchmarking session.
 *
 * @param desc      Benchmark server descriptor.
 * @param address   Address of the remote client at the other end of the connection.
 * @param cookie    User-application specific parameter. It's set during
 *                  the @ref fnet_bench_srv_set_callback_session_begin() call.
 *
 * @see fnet_bench_srv_set_callback_session_begin()
 ******************************************************************************/
typedef void(*fnet_bench_srv_callback_session_begin_t)(fnet_bench_srv_desc_t desc, struct fnet_sockaddr address, void *cookie);

/**************************************************************************/ /*!
 * @brief Benchmark server event handler callback function prototype, that is
 * called when the benchmark server has finished the benchmarking session.
 *
 * @param desc              Benchmark server descriptor.
 * @param bench_srv_result  Benchmark session result.
 * @param cookie            User-application specific parameter. It's set during
 *                          the @ref fnet_bench_srv_set_callback_session_end() call.
 *
 * @see fnet_bench_srv_set_callback_session_end()
 ******************************************************************************/
typedef void(*fnet_bench_srv_callback_session_end_t)(fnet_bench_srv_desc_t desc, const struct fnet_bench_srv_result *bench_srv_result, void *cookie);

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************************************************/ /*!
 *
 * @brief    Initializes the Benchmark server service.
 *
 * @param params     Initialization parameters defined by @ref fnet_bench_srv_params_t.
 *
 * @return This function returns:
 *   - Benchmark server descriptor if no error occurs.
 *   - @c 0 if an error occurs.
 *
 * @see fnet_bench_srv_release()
 *
 ******************************************************************************
 *
 * This function initializes the Benchmark server service. It allocates all
 * resources needed and registers the Benchmark server in the service polling list.@n
 * After the initialization, the user application should call the main polling
 * function @ref fnet_service_poll() periodically to run the Benchmark server service
 * in the background.
 *
 ******************************************************************************/
fnet_bench_srv_desc_t fnet_bench_srv_init( fnet_bench_srv_params_t *params );

/***************************************************************************/ /*!
 *
 * @brief    Releases the Benchmark server service.
 *
 * @param desc     Benchmark server descriptor to be released.
 *
 * @see fnet_bench_srv_init()
 *
 ******************************************************************************
 *
 * This function releases the Benchmark server. It releases all resources
 * used by the server, and unregisters it from the service polling list.
 *
 ******************************************************************************/
void fnet_bench_srv_release(fnet_bench_srv_desc_t desc);

/***************************************************************************/ /*!
 *
 * @brief    Registers the "session begin" event handler callback.
 *
 * @param desc                      Benchmark server descriptor.
 * @param callback_session_begin    Pointer to the callback function defined by
 *                                  @ref fnet_bench_srv_callback_session_begin_t.
 * @param cookie                    Optional application-specific parameter. @n
 *                                  It's passed to the @c callback_session_begin
 *                                  function as the input parameter.
 *
 ******************************************************************************
 *
 * This function registers the @c callback_session_begin callback function for
 * the "session begin" event. This event occurs on a new benchmark session begin.
 * To stop the event handling, set the @c callback_session parameter to zero value.
 *
 ******************************************************************************/
void fnet_bench_srv_set_callback_session_begin (fnet_bench_srv_desc_t desc, fnet_bench_srv_callback_session_begin_t callback_session_begin, void *cookie);

/***************************************************************************/ /*!
 *
 * @brief    Registers the "session end" event handler callback.
 *
 * @param desc                      Benchmark server descriptor.
 * @param callback_session_end      Pointer to the callback function defined by
 *                                  @ref fnet_bench_srv_callback_session_end_t.
 * @param cookie                    Optional application-specific parameter. @n
 *                                  It's passed to the @c callback_session_end
 *                                  function as the input parameter.
 *
 ******************************************************************************
 *
 * This function registers the @c callback_session_end callback function for
 * the "session end" event. This event occurs on a benchmark session end.
 * To stop the event handling, set the @c callback_session_end parameter to zero value.
 *
 ******************************************************************************/
void fnet_bench_srv_set_callback_session_end (fnet_bench_srv_desc_t desc, fnet_bench_srv_callback_session_end_t callback_session_end, void *cookie);

/***************************************************************************/ /*!
 *
 * @brief    Detects if the Benchmark server service is enabled or disabled.
 *
 * @param desc     Benchmark server descriptor
 *
 * @return This function returns:
 *          - @ref FNET_TRUE if the Benchmark server is successfully initialized.
 *          - @ref FNET_FALSE if the Benchmark server is not initialized or is released.
 *
 ******************************************************************************
 *
 * This function detects if the Benchmark server service is initialized or is released.
 *
 ******************************************************************************/
fnet_bool_t fnet_bench_srv_is_enabled(fnet_bench_srv_desc_t desc);

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* FNET_CFG_BENCH_SRV */

#endif /* _FNET_BENCH_SRV_H_ */
