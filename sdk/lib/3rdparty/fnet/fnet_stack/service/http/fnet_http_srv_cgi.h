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
*  FNET HTTP Server CGI API.
*
***************************************************************************/

#ifndef _FNET_HTTP_SRV_CGI_H_

#define _FNET_HTTP_SRV_CGI_H_

#if (FNET_CFG_HTTP_SRV && FNET_CFG_HTTP_SRV_CGI) || defined(__DOXYGEN__)

/*! @addtogroup fnet_http_srv
 @{ */

/**************************************************************************/ /*!
 * @brief CGI file extension. @n
 * All HTTP requests to the files that have this extension will be
 * handled by the CGI handler.
 * @showinitializer
 ******************************************************************************/
#define FNET_HTTP_SRV_CGI_EXTENSION    "cgi"

/**************************************************************************/ /*!
 * @brief Callback function prototype of the CGI query handler.
 *
 * @param session   HTTP session handle.
 * @param query     CGI query string (null-terminated). @n
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
 * @see fnet_http_srv_cgi_t, fnet_http_srv_cgi_send_t
 *
 * The CGI handler invokes this callback function when the server gets
 * CGI request and the requested CGI file name corresponds to the name
 * registered in the CGI table.@n
 * The @c query points to the CGI query string.
 * If the CGI request does not have any data, the @c query will point to the
 * blank string.@n
 * If the HTTP server works according to HTTP/1.x  (@ref FNET_CFG_HTTP_SRV_VERSION_MAJOR is @c 1),
 * this function may use fnet_http_srv_set_response_status_code() to change the default HTTP response status-code.
 *
 ******************************************************************************/
typedef fnet_return_t(*fnet_http_srv_cgi_handle_t)(fnet_http_srv_session_t session, fnet_char_t *query, fnet_uint32_t *cookie);


/**************************************************************************/ /*!
 * @brief Callback function prototype of the CGI response function.
 *
 * @param buffer           Output buffer where CGI response content will be copied to.
 *
 * @param buffer_size      Size of the output @c buffer. @n
 *                         It's defined by the @ref FNET_CFG_HTTP_SRV_REQUEST_SIZE_MAX parameter.
 *
 * @param eof              Condition flag:
 *                         - @c FNET_FALSE =  there is still more data to send.
 *                           The @ref fnet_http_srv_cgi_send_t function will be called
 *                           during the next iteration again.
 *                         - @c FNET_FALSE =  no more data is available for the CGI response.
 *                           It was the last call of the @ref fnet_http_srv_cgi_send_t
 *                           function for the current CGI response.
 * @param cookie    This parameter points to the value, initially set to zero,
 *                  which can be used to associate a custom information
 *                  with a connection instance. If application store context
 *                  information in the @c cookie, it will be preserved
 *                  for future calls for this request. This allows the
 *                  application to associate some request-specific state.
 *
 * @return This function returns the number of bytes actually written to the buffer
 *         pointed to by @c buffer.@n
 *         The condition flag @c eof indicates if the CGI data-end condition has occurred.
 *
 * @see fnet_http_srv_cgi_t, fnet_http_srv_cgi_handle_t
 *
 * This function creates the CGI response content.@n
 * An application should use the @c buffer as output buffer for the dynamic content
 * and set @c eof flag to @c 1 if no data for output are available.@n
 * The CGI handler invokes this callback function after successful call of the
 * @ref fnet_http_srv_cgi_handle_t function and continues to call this function repeatedly
 * till the @c eof will be set to @c 1 or the return result is set to @c 0.
 *
 ******************************************************************************/
typedef fnet_size_t (*fnet_http_srv_cgi_send_t)(fnet_uint8_t *buffer, fnet_size_t buffer_size, fnet_bool_t *eof, fnet_uint32_t *cookie);

/**************************************************************************/ /*!
 * @brief CGI callback function table.
 *
 * The last table element must have all fields set to zero as the end-of-table mark.@n
 * @n
 * The Common Gateway Interface (CGI) is a standard for external
 * gateway programs to interface with HTTP servers. A CGI function is
 * executed in real-time, so that it can output dynamic information.
 *
 * @see fnet_http_srv_params_t
 ******************************************************************************/
typedef struct
{
    fnet_char_t                 *name;      /**< @brief CGI file name. */
    fnet_http_srv_cgi_handle_t  handle;     /**< @brief Pointer to the CGI query handler. It's optional. */
    fnet_http_srv_cgi_send_t    send;       /**< @brief Pointer to the CGI response function.
                                         * This function actually creates dynamic content of
                                         * the CGI response. It's optional. */
} fnet_http_srv_cgi_t;

/*! @} */

#endif

#endif
