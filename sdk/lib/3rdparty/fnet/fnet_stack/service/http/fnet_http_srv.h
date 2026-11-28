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
*  FNET HTTP Server API.
*
***************************************************************************/

#ifndef _FNET_HTTP_SRV_H_

#define _FNET_HTTP_SRV_H_

#if FNET_CFG_HTTP_SRV || defined(__DOXYGEN__)

/*! @addtogroup fnet_http_srv
* The current version of the FNET HTTP Server supports:
* - HTTP/0.9 or HTTP/1.0 protocol responces.
* - GET and POST (enabled by @ref FNET_CFG_HTTP_SRV_POST) HTTP requests.
* - Server Side Includes (SSI). @n
*   SSI directives have the following format:
*   @code
*   <!--#command [parameter(s)]-->
*   @endcode
* - Common Gateway Interface (CGI).
* - Basic Access Authentication (enabled by @ref FNET_CFG_HTTP_SRV_AUTHENTICATION_BASIC).
* - FNET File System Interface.
*
* @n
* After the FNET HTTP server is initialized by calling the @ref fnet_http_srv_init()
* function, the user application should call the main service polling function
* @ref fnet_service_poll() periodically in background. @n
* @n
* For HTTP server usage example, refer to FNET demo application source code.@n
* @n
* Configuration parameters:
* - @ref FNET_CFG_HTTP_SRV
* - @ref FNET_CFG_HTTP_SRV_VERSION_MAJOR
* - @ref FNET_CFG_HTTP_SRV_SESSION_MAX
* - @ref FNET_CFG_HTTP_SRV_SSI
* - @ref FNET_CFG_HTTP_SRV_CGI
* - @ref FNET_CFG_HTTP_SRV_POST
* - @ref FNET_CFG_HTTP_SRV_AUTHENTICATION_BASIC
* - @ref FNET_CFG_HTTP_SRV_PORT
* - @ref FNET_CFG_HTTP_SRV_REQUEST_SIZE_MAX
* - @ref FNET_CFG_HTTP_SRV_TLS
* - @ref FNET_CFG_HTTP_SRV_TLS_PORT
*/
/*! @{ */

#include "fnet_http_srv_auth.h"

/**************************************************************************/ /*!
 * @def FNET_HTTP_SRV_VERSION_MAJOR
 * @brief The major version number of HTTP protocol supported by the HTTP server.
 ******************************************************************************/
#if FNET_CFG_HTTP_SRV_VERSION_MAJOR /*HTTP/1.x*/
    #define FNET_HTTP_SRV_VERSION_MAJOR     (1u)
#else   /*HTTP/0.9*/
    #define FNET_HTTP_SRV_VERSION_MAJOR     (0u)
#endif

/**************************************************************************/ /*!
 * @def FNET_HTTP_SRV_VERSION_MINOR
 * @brief The minor version number of HTTP protocol supported by the HTTP server.
 ******************************************************************************/
#if FNET_CFG_HTTP_SRV_VERSION_MAJOR /* HTTP/1.x*/
    #define FNET_HTTP_SRV_VERSION_MINOR     (0u)
#else   /*HTTP/0.9*/
    #define FNET_HTTP_SRV_VERSION_MINOR     (9u)
#endif


#if FNET_CFG_HTTP_SRV_VERSION_MAJOR /* HTTP/1.x*/ || defined(__DOXYGEN__)

/********************************************************************/ /*!
* @brief HTTP/1.0 Status-Code definitions according to RFC1945.
*
* Can be used by inside CGI and POST handlers: @ref fnet_http_srv_cgi_handle_t(),
* @ref fnet_http_srv_post_handle_t() and @ref fnet_http_srv_post_receive_t()
* call-back functions.
* @see fnet_http_srv_cgi_handle_t(), fnet_http_srv_post_handle_t(), fnet_http_srv_post_receive_t()
*************************************************************************/
typedef enum
{
    FNET_HTTP_SRV_STATUS_CODE_NONE                  = 0,    /**< @brief NONE.@n
                                                        * Status code is not defined.*/
    FNET_HTTP_SRV_STATUS_CODE_OK                    = 200,  /**< @brief OK.@n
                                                        * The request has succeeded.*/
    FNET_HTTP_SRV_STATUS_CODE_CREATED               = 201,  /**< @brief Created.@n
                                                        * The request has been fulfilled
                                                        * and resulted in a new resource
                                                        * being created.*/
    FNET_HTTP_SRV_STATUS_CODE_ACCEPTED              = 202,  /**< @brief Accepted.@n
                                                        * The request has been accepted
                                                        * for processing, but the processing
                                                        * has not been completed.*/
    FNET_HTTP_SRV_STATUS_CODE_NO_CONTENT            = 204,  /**< @brief No Content.@n
                                                        * The server has fulfilled the request
                                                        * but there is no new information to
                                                        * send back.*/
    FNET_HTTP_SRV_STATUS_CODE_MOVED_PERMANENTLY     = 301,  /**< @brief Moved Permanently.@n
                                                        * The requested resource has been
                                                        * assigned a new permanent URL and
                                                        * any future references to
                                                        * this resource should be
                                                        * done using that URL.@n
                                                        * It's not supported by the FNET HTTP Server.*/
    FNET_HTTP_SRV_STATUS_CODE_MOVED_TEMPORARILY     = 302,  /**< @brief Moved Temporarily.@n
                                                        * The requested resource resides
                                                        * temporarily under a different URL.@n
                                                        * It's not supported by the FNET HTTP Server.*/
    FNET_HTTP_SRV_STATUS_CODE_NOT_MODIFIED          = 304,  /**< @brief Not Modified.@n
                                                        * If the client has performed a
                                                        * conditional GET request and access is
                                                        * allowed, but the document has not been
                                                        * modified since the date and time specified
                                                        * in the If-Modified-Since field, the server
                                                        * must respond with this status code and
                                                        * not send an Entity-Body to the client.@n
                                                        * It's not supported by the FNET HTTP Server.*/
    FNET_HTTP_SRV_STATUS_CODE_BAD_REQUEST           = 400,  /**< @brief Bad Request.@n
                                                        * The request could not be understood by
                                                        * the server due to malformed syntax.*/
    FNET_HTTP_SRV_STATUS_CODE_UNAUTHORIZED          = 401,  /**< @brief Unauthorized.@n
                                                        * The request requires user authentication.*/
    FNET_HTTP_SRV_STATUS_CODE_FORBIDDEN             = 403,  /**< @brief Forbidden.@n
                                                        * The server understood the request, but is
                                                        * refusing to fulfill it.*/
    FNET_HTTP_SRV_STATUS_CODE_NOT_FOUND             = 404,  /**< @brief Not Found.@n
                                                        * The server has not found anything matching
                                                        * the Request-URI.*/
    FNET_HTTP_SRV_STATUS_CODE_INTERNAL_SERVER_ERROR = 500,  /**< @brief Internal Server Error.@n
                                                        * The server encountered an unexpected condition
                                                        * which prevented it from fulfilling the request.*/
    FNET_HTTP_SRV_STATUS_CODE_NOT_IMPLEMENTED       = 501,  /**< @brief Not Implemented.@n
                                                        * The server does not support the functionality
                                                        * required to fulfill the request.*/
    FNET_HTTP_SRV_STATUS_CODE_BAD_GATEWAY           = 502,  /**< @brief Bad Gateway.@n
                                                        * The server, while acting as a gateway or proxy,
                                                        * received an invalid response from the upstream
                                                        * server it accessed in attempting to fulfill the request.*/
    FNET_HTTP_SRV_STATUS_CODE_SERVICE_UNAVAILABLE   = 503   /**< @brief Service Unavailable.@n
                                                        * The server is currently unable to handle the
                                                        * request due to a temporary overloading or
                                                        * maintenance of the server.*/
} fnet_http_srv_status_code_t;
#endif

#if (FNET_CFG_HTTP_SRV_TLS && FNET_CFG_TLS) || defined(__DOXYGEN__)
/**************************************************************************/ /*!
 * @brief HTTP over TLS (HTTPS) input parameters for @ref fnet_http_srv_init().
 ******************************************************************************/
typedef struct
{
    const fnet_uint8_t  *certificate_buffer;        /**< @brief Buffer holding the certificate data in PEM or DER format. */
    fnet_size_t         certificate_buffer_size;    /**< @brief Size of the certificate buffer (including the terminating null byte for PEM data). */
    const fnet_uint8_t  *private_key_buffer;        /**< @brief Buffer holding the private key in PEM or DER format. */
    fnet_size_t         private_key_buffer_size;    /**< @brief Size of the private key buffer (including the terminating null byte for PEM data). */
} fnet_http_srv_tls_params_t;
#endif

/**************************************************************************/ /*!
 * @brief HTTP server descriptor.
 * @see fnet_http_srv_init()
 ******************************************************************************/
typedef fnet_int32_t fnet_http_srv_desc_t;

/**************************************************************************/ /*!
 * @brief HTTP session handle.
 * @see fnet_http_srv_cgi_handle_t
 ******************************************************************************/
typedef fnet_int32_t fnet_http_srv_session_t;

#include "fnet_http_srv_ssi.h"
#include "fnet_http_srv_cgi.h"
#include "fnet_http_srv_post.h"

/**************************************************************************/ /*!
 * @brief Input parameters for @ref fnet_http_srv_init().
 ******************************************************************************/
typedef struct
{
    fnet_char_t *root_path;                     /**< @brief Server root-directory path (null-terminated string). */
    fnet_char_t *index_path;                    /**< @brief Index file path (null-terminated string). @n
                                                *   It's relative to the @c root_path.*/
    struct fnet_sockaddr address;               /**< @brief Server socket address. @n
                                                * If server IP address and Scope ID are set to @c 0s, the server will listen to all current network interfaces. @n
                                                * If server address family is set to @c 0, it will be assigned to @ref AF_SUPPORTED. @n
                                                * If server port number is set to @c 0, it will be assigned to the default port number defined by @ref FNET_CFG_HTTP_SRV_PORT.*/
#if FNET_CFG_HTTP_SRV_SSI || defined(__DOXYGEN__)
    const fnet_http_srv_ssi_t *ssi_table;      /**< @brief Pointer to the optional SSI callback function table. */
#endif
#if FNET_CFG_HTTP_SRV_CGI || defined(__DOXYGEN__)
    const fnet_http_srv_cgi_t *cgi_table;      /**< @brief Pointer to the optional CGI callback function table. */
#endif
#if (FNET_CFG_HTTP_SRV_AUTHENTICATION_BASIC && FNET_CFG_HTTP_SRV_VERSION_MAJOR) || defined(__DOXYGEN__)
    const fnet_http_srv_auth_t      *auth_table;   /**< @brief Pointer to the optional HTTP Access Authentification table. */
#endif
#if (FNET_CFG_HTTP_SRV_POST && FNET_CFG_HTTP_SRV_VERSION_MAJOR) || defined(__DOXYGEN__)
    const fnet_http_srv_post_t *post_table;    /**< @brief Pointer to the optional POST callback function table. */
#endif
#if (FNET_CFG_HTTP_SRV_TLS && FNET_CFG_TLS) || defined(__DOXYGEN__)
    fnet_http_srv_tls_params_t *tls_params;    /**< @brief Pointer to the optional HTTP over TLS (HTTPS) parameters. */
#endif
} fnet_http_srv_params_t;

#if defined(__cplusplus)
extern "C" {
#endif

/***************************************************************************/ /*!
 *
 * @brief    Initializes the HTTP Server service.
 *
 * @param params     Initialization parameters defined by @ref fnet_http_srv_params_t.
 *
 * @return This function returns:
 *   - HTTP server descriptor if no error occurs.
 *   - @c 0 if an error occurs.
 *
 * @see fnet_http_srv_release()
 *
 ******************************************************************************
 *
 * This function initializes the HTTP server service. It allocates all
 * resources needed, and registers the HTTP server service in the polling list.@n
 * After the initialization, the user application should call the main polling
 * function  @ref fnet_service_poll() periodically to run the HTTP server in background.
 *
 ******************************************************************************/
fnet_http_srv_desc_t fnet_http_srv_init( fnet_http_srv_params_t *params);

/***************************************************************************/ /*!
 *
 * @brief    Releases the HTTP Server service.
 *
 * @param desc     HTTP server descriptor to be unregistered.
 *
 * @see fnet_http_srv_init()
 *
 ******************************************************************************
 *
 * This function releases the HTTP Server assigned to the @c desc
 * descriptor.@n
 * It releases all occupied resources, and unregisters the HTTP service from
 * the polling list.
 *
 ******************************************************************************/
void fnet_http_srv_release(fnet_http_srv_desc_t desc);

/***************************************************************************/ /*!
 *
 * @brief    Detects if the HTTP Server service is enabled or disabled.
 *
 * @param desc     HTTP server descriptor
 *
 * @return This function returns:
 *          - @ref FNET_TRUE if the HTTP Server is successfully initialized.
 *          - @ref FNET_FALSE if the HTTP Server is not initialized or is released.
 *
 ******************************************************************************
 *
 * This function detects if the HTTP Server service is initialized or is released.
 *
 ******************************************************************************/
fnet_bool_t fnet_http_srv_is_enabled(fnet_http_srv_desc_t desc);

/***************************************************************************/ /*!
 *
 * @brief    Converts escaped string to an original format.
 *
 * @param dest    Destination string.
 *
 * @param src     Source string.
 *
 ******************************************************************************
 *
 * This function converts encoded string to the original format.
 * The '+' symbol is replaced by the space symbol, and the % symbol followed
 * by two hexadecimal digits is replaced by proper ASCII value
 * (for example the exclamation mark encoded as \%21). @n
 * This function should be used by CGI functions to eliminate escape symbols
 * from a query string.@n
 * @n
 * INFO:@n
 * RFC1945:The Request-URI is transmitted as an encoded string, where some
 * characters may be escaped using the "% HEX HEX" encoding defined by
 * RFC 1738. The origin server must decode the Request-URI in order
 * to properly interpret the request.
 *
 ******************************************************************************/
void fnet_http_srv_query_unencode(fnet_uint8_t *dest, fnet_uint8_t *src);

#if FNET_CFG_HTTP_SRV_VERSION_MAJOR /* HTTP/1.x*/ || defined(__DOXYGEN__)

/**************************************************************************/ /*!
 * @brief Sets status code in HTTP response status-line.
 *
 * @param session       HTTP session handle.
 * @param status_code   HTTP Response Status-Code.
 *
 * If the HTTP server works according to HTTP/1.x (@ref FNET_CFG_HTTP_SRV_VERSION_MAJOR is @c 1),
 * this function may be used to change the default HTTP response status-code.
 *
 ******************************************************************************/
void fnet_http_srv_set_response_status_code (fnet_http_srv_session_t session, fnet_http_srv_status_code_t status_code);

/**************************************************************************/ /*!
 * @brief Sets content length in HTTP response header.
 *
 * @param session       HTTP session handle.
 * @param content_length   HTTP Response Content length.
 *
 * If the HTTP server works according to HTTP/1.x  (@ref FNET_CFG_HTTP_SRV_VERSION_MAJOR is @c 1),
 * this function may be used to change the default HTTP response content-length.
 *
 ******************************************************************************/
void fnet_http_srv_set_response_content_length (fnet_http_srv_session_t session, fnet_size_t content_length);

/**************************************************************************/ /*!
 * @brief Disables sending of HTTP response status-code and header.
 *
 * @param session       HTTP session handle.
 *
 * If the HTTP server works according to HTTP/1.x (@ref FNET_CFG_HTTP_SRV_VERSION_MAJOR is @c 1),
 * this function may be used to disables automatic sending of HTTP response status-code and header.
 * It can be sent by a user handler.
 *
 ******************************************************************************/
void fnet_http_srv_set_response_no_header (fnet_http_srv_session_t session);

#endif

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* FNET_CFG_HTTP_SRV */

#endif
