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
*  HTTP client API.
*
***************************************************************************/

//DM !!!!!!!!!!! Under development

#ifndef _FNET_HTTP_CLN_H_

#define _FNET_HTTP_CLN_H_

#if 0 /* Under development */

#if FNET_CFG_HTTP_CLN || defined(__DOXYGEN__)

/*! @addtogroup fnet_http_cln
*
* The FNET HTTP client. @n
TBD
* After the FNET HTTP client is initialized by calling the @ref fnet_http_cln_init() function,
* the user application should call the main service-polling function
* @ref fnet_service_poll() periodically in background.@n
* The HTTP response results will be passed to the @ref fnet_http_cln_callback_response_t callback function,
* which is set during the HTTP client initialization.
* @n
* The HTTP client service is released automatically as soon as the HTTP session is
* finished or an error occurs. Your application may continue
* to call @ref fnet_service_poll() to handle other services, but this will not have any
* impact on the HTTP client communication until you initialize the next HTTP request using
* @ref fnet_http_cln_init() again. @n
* @n
* For the FNET HTTP client example, refer to the FNET Shell demo source code.@n
*
* Configuration parameters:
* - @ref FNET_CFG_HTTP_CLN
* - @ref FNET_CFG_HTTP_CLN_PORT
* - @ref FNET_CFG_HTTP_CLN_BUFFER_SIZE
*/

/*! @{ */

/**************************************************************************/ /*!
 * @brief HTTP client descriptor.
 * @see fnet_http_cln_init()
 ******************************************************************************/
typedef void *fnet_http_cln_desc_t;

/**************************************************************************/ /*!
 * @brief Callback function prototype of the HTTP client receive function.
 *
 * @param buffer           Data buffer that contains data received from
 *                         the remote HTTP server.
 *
 * @param buffer_size      Size of the input @c buffer in bytes.
 *
 * @param cookie            User-application specific parameter. It's set during
 *                          the HTTP-client service initialization as part of
 *                          @ref fnet_http_cln_params.
 *
 * @return This function must return:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs.
 * @see fnet_http_cln_init
 *
 * This function is invoked by the HTTP client when there is any data
 * in the entity-body of the HTTP response message.
 * This function can be invoked multiple times to process all received data.
 * At each invocation a new chunk of data must be processed.
 * //DM The HTTP server invokes this callback function after call of the
 * //DM @ref fnet_http_srv_post_handle_t function.@n
 *
 ******************************************************************************/
//response
//DM resposnse_staus_code: returns the status code from the HTTP response (200, 201, 400, 401, etc.)
//DM response_header: HTTP response headers so caller may inspect them
//    name:The name of the HTTP header to add. It is invalid for the name to include the ':' character or character codes outside the range 33-126.
//    value: The value to be assigned to the header.
//DM response_content: This is a buffer that is filled by the contents of the HTTP response body.
//typedef fnet_return_t(*fnet_http_cln_response_t)(fnet_http_cln_desc_t http_cln_desc, fnet_char_t *header, void *cookie);

typedef fnet_return_t(*fnet_http_cln_connect_t)(fnet_http_cln_desc_t http_cln_desc, fnet_result_t result);

/**************************************************************************/ /*!
 * @brief Initialization parameters for the @ref fnet_http_cln_init() function.
 ******************************************************************************/
struct fnet_http_cln_params
{
    struct fnet_sockaddr                    address;            /**< @brief Socket address of the remote HTTP server to connect to.@n
                                                                * If the address port number is set to @c 0, it will be assigned to the default port number defined by @ref FNET_CFG_HTTP_CLN_PORT.*/
    fnet_char_t                             *method;            /**< @brief HTTP method to be used. It may be "GET", "POST", "DELETE", "HEAD", "PUT" or "PATCH".*/
    fnet_char_t                             *uri;               /**< @brief Uniform Resource Identifier (URI). It specifies the relative path of the URL excluding the host name. */
    fnet_char_t                             *header;            /**< @brief Optional set of HTTP headers (name-value pairs) to be added to the HTTP request. */
    //   fnet_uint8_t                            *body;              /**< @brief Pointer to the request body. This value is optional and can be FNET_NULL. */
    //   fnet_size_t                             body_size;          /**< @brief Request body size. This value is optional and can be 0. */
    //   fnet_http_cln_response_t                response_callback;  /**< @brief HTTP response callback.*/
    fnet_http_cln_connect_t                 *callback;          /**< @brief Optional callback function which is called when the HTTP cleient connection is istablished or is failed. */
    void                                    *cookie;            /**< @brief Optional application-specific parameter, which is passed to the @c callback functions as input parameter. */
};

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************************************************/ /*!
 *
 * @brief    Initialize the HTTP client request.
 *
 * @param params     Initialization parameters defined by @ref fnet_http_cln_params.
 *
 * @return This function returns:
 *   - HTTP client descriptor if no error occurs.
 *   - @c FNET_NULL if an error occurs.
 *
 * @see fnet_http_cln_release()
 *
 ******************************************************************************
 *
 * This function initializes the HTTP client service. It allocates all
 * resources needed and registers the HTTP client in the service polling list.@n
 * After the initialization, the user application should call the main polling
 * function @ref fnet_service_poll() periodically to run the HTTP client service
 * in the background.@n
 ******************************************************************************/
fnet_http_cln_desc_t fnet_http_cln_init( struct fnet_http_cln_params *params );

/***************************************************************************/ /*!
 *
 * @brief    Abort the HTTP session and release the HTTP client service.
 *
 * @param desc     HTTP client descriptor to be released.
 *
 * @see fnet_http_cln_init()
 *
 ******************************************************************************
 *
 * This function releases the HTTP client. It releases all resources
 * used by the service, and unregisters it from the service polling list.@n
 * Use this function only in the case of the early termination of the service,
 * because the HTTP client service is released automatically as soon as the
 * HTTP session is finished.
 *
 ******************************************************************************/
void fnet_http_cln_release(fnet_http_cln_desc_t desc);

/***************************************************************************/ /*!
 *
 * @brief    Detect if the HTTP client service is enabled or disabled.
 *
 * @param desc     HTTP client descriptor
 *
 * @return This function returns:
 *          - @ref FNET_TRUE if the HTTP client is successfully initialized.
 *          - @ref FNET_FALSE if the HTTP client is not initialized or is released.
 *
 ******************************************************************************
 *
 * This function detects if the HTTP client service is initialized or is released.
 *
 ******************************************************************************/
fnet_bool_t fnet_http_cln_is_enabled(fnet_http_cln_desc_t desc);

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* FNET_CFG_HTTP_CLN */

#endif /* Under development */

#endif /* _FNET_HTTP_CLN_H_ */
