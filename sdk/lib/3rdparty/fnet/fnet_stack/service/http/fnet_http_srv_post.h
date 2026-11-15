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
*  FNET HTTP Server POST method API.
*
***************************************************************************/

#ifndef _FNET_HTTP_SRV_POST_H_

#define _FNET_HTTP_SRV_POST_H_

#if (FNET_CFG_HTTP_SRV && FNET_CFG_HTTP_SRV_POST && FNET_CFG_HTTP_SRV_VERSION_MAJOR)|| defined(__DOXYGEN__)

/*! @addtogroup fnet_http_srv
 @{ */

/**************************************************************************/ /*!
 * @brief Callback function prototype of the POST-method query handler.
 *
 * @param query     POST-method query string (null-terminated). @n
 *                  The query string is set to whatever appears
 *                  after the question mark in the URL itself.
 * @param cookie    This parameter points to the value, initially set to zero,
 *                  which can be used to associate a custom information
 *                  with a connection instance. If application store context
 *                  information in the @c cookie, it will be preserved
 *                  for future calls for this request. This allows the
 *                  application to associate some request-specific state.
 *
 * @return This function must return:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs.
 * @see fnet_http_srv_post_t, fnet_http_srv_post_receive_t, fnet_http_srv_post_send_t
 *
 * The HTTP server invokes this callback function when gets
 * POST-method request and the requested file name corresponds to the name
 * registered in the POST table defined @ref fnet_http_srv_post_t.@n
 * If the query string does not have any data, the @c query will point to the
 * blank string.@n
 * If the HTTP server works according to HTTP/1.x  (@ref FNET_CFG_HTTP_SRV_VERSION_MAJOR is @c 1),
 * this function may use fnet_http_srv_set_response_status_code() to change the default HTTP response status-code.
 *
 ******************************************************************************/
typedef fnet_return_t(*fnet_http_srv_post_handle_t)(fnet_http_srv_session_t session, fnet_char_t *query, fnet_uint32_t *cookie);

/**************************************************************************/ /*!
 * @brief Callback function prototype of the POST-method receive function.
 *
 * @param buffer           Data buffer that contains data received from
 *                         the remote HTTP client.
 *
 * @param buffer_size      Size of the input @c buffer in bytes.
 *
 * @param cookie    This parameter points to the value,
 *                  which can be used to associate a custom information
 *                  with a connection instance. If application store context
 *                  information in the @c cookie, it will be preserved
 *                  for future calls for this request. This allows the
 *                  application to associate some request-specific state.
 *
 * @return This function must return:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs.
 * @see fnet_http_srv_post_t, fnet_http_srv_post_handle_t, fnet_http_srv_post_send_t
 *
 * This function is invoked by the HTTP server when there is any data
 * in the entity-body of the POST request.
 * This function can be invoked multiple times to process all received data.
 * At each invocation a new chunk of data must be processed.
 * The HTTP server invokes this callback function after call of the
 * @ref fnet_http_srv_post_handle_t function.@n
 * If the HTTP server works according to HTTP/1.x  (@ref FNET_CFG_HTTP_SRV_VERSION_MAJOR is @c 1),
 * this function may use fnet_http_srv_set_response_status_code() to change the default HTTP response status-code.
 *
 ******************************************************************************/
typedef fnet_return_t(*fnet_http_srv_post_receive_t)(fnet_http_srv_session_t session, fnet_uint8_t *buffer, fnet_size_t buffer_size, fnet_uint32_t *cookie);

/**************************************************************************/ /*!
 * @brief Callback function prototype of the POST-method response function.
 *
 * @param buffer           Output buffer where POST response content will be copied to.
 *
 * @param buffer_size      Size of the output @c buffer. @n
 *                         It's defined by the @ref FNET_CFG_HTTP_SRV_REQUEST_SIZE_MAX parameter.
 *
 * @param eof              Condition flag:
 *                         - @c FNET_FALSE =  there is still more data to send.
 *                           The @ref fnet_http_srv_post_send_t function will be called
 *                           during the next iteration again.
 *                         - @c FNET_TRUE =  no more data is available for the POST response.
 *                           It was the last call of the @ref fnet_http_srv_post_send_t
 *                           function for the current POST response.
 *
 * @param cookie    This parameter points to the value,
 *                  which can be used to associate a custom information
 *                  with a connection instance. If application store context
 *                  information in the @c cookie, it will be preserved
 *                  for future calls for this request. This allows the
 *                  application to associate some request-specific state.
 *
 * @return This function must return the number of bytes actually written to the buffer
 *         pointed to by @c buffer.@n
 *         The condition flag @c eof indicates if the POST-method end condition has occurred.
 *
 * @see fnet_http_srv_post_t, fnet_http_srv_post_handle_t, fnet_http_srv_post_receive_t
 *
 * This function creates the POST-method response content.@n
 * An application should use the @c buffer as output buffer for the dynamic content
 * and set @c eof flag to @c 1 if no data for output are available.@n
 * The HTTP server invokes this callback function after call of the
 * @ref fnet_http_srv_post_handle_t and @ref fnet_http_srv_post_receive_t functions and continues to call this function repeatedly
 * till the @c eof will be set to @c 1 or the return result is set to @c 0.
 *
 ******************************************************************************/
typedef fnet_size_t (*fnet_http_srv_post_send_t)(fnet_uint8_t *buffer, fnet_size_t buffer_size, fnet_bool_t *eof, fnet_uint32_t *cookie);

/**************************************************************************/ /*!
 * @brief POST-method callback function table.
 *
 * The last table element must have all fields set to zero as the end-of-table mark.@n
 * @n
 * The POST request method is used when the client needs to send data to the server
 * as part of the request, such as when uploading a file or submitting a completed form.
 *
 * @see fnet_http_srv_params_t
 ******************************************************************************/
typedef struct
{
    fnet_char_t *name;		                /**< @brief File name associated with the POST-request. */
    fnet_http_srv_post_handle_t post_handle;    /**< @brief Pointer to the POST query handler. It's optional. */
    fnet_http_srv_post_receive_t post_receive;  /**< @brief Pointer to the POST receive function. It's optional.@n
                                            * This function can be invoked multiple times to process
                                            * all received data.*/
    fnet_http_srv_post_send_t post_send;        /**< @brief Pointer to the POST response function. It's optional.@n
                                            * This function actually creates dynamic content of
                                            * the POST response.  */

} fnet_http_srv_post_t;

/*! @} */

#endif

#endif
