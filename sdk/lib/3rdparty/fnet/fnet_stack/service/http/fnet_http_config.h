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
*  FNET HTTP Server configuration file.
*
***************************************************************************/

/**************************************************************************
 * !!!DO NOT MODIFY THIS FILE!!!
 **************************************************************************/

#ifndef _FNET_HTTP_CONFIG_H_

#define _FNET_HTTP_CONFIG_H_

/*! @addtogroup fnet_http_srv_config */
/*! @{ */

/**************************************************************************/ /*!
 * @def      FNET_CFG_HTTP_SRV
 * @brief    HTTP Server service support:
 *               - @c 1..n = is enabled. Its value defines maximum number of the HTTP Servers that can be run simultaneously.
 *               - @b @c 0 = is disabled (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_HTTP_SRV
    #define FNET_CFG_HTTP_SRV                   (0)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_HTTP_SRV_SESSION_MAX
 * @brief   Maximum number of simultaneous user-session that can be handled
 *          by the HTTP server.@n
 *          Default value is @b @c 3.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_HTTP_SRV_SESSION_MAX
    #define FNET_CFG_HTTP_SRV_SESSION_MAX       (3u)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_HTTP_SRV_SSI
 * @brief   HTTP Server SSI (Server Side Includes) support:
 *               - @b @c 1 = is enabled (Default value).
 *               - @c 0 = is disabled.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_HTTP_SRV_SSI
    #define FNET_CFG_HTTP_SRV_SSI               (1)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_HTTP_SRV_CGI
 * @brief   HTTP Server CGI (Common Gateway Interface) support:
 *               - @b @c 1 = is enabled (Default value).
 *               - @c 0 = is disabled.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_HTTP_SRV_CGI
    #define FNET_CFG_HTTP_SRV_CGI               (1)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_HTTP_SRV_PORT
 * @brief   Default HTTP port number (in network byte order).@n
 *          It can be changed during the HTTP server initialization by the
 *          @ref fnet_http_srv_init() function.@n
 *          Default value FNET_HTONS(80).
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_HTTP_SRV_PORT
    #define FNET_CFG_HTTP_SRV_PORT              (FNET_HTONS(80u))
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_HTTP_SRV_TLS
 * @brief    HTTPS over TLS (HTTPS) Server service support:
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_HTTP_SRV_TLS
    #define FNET_CFG_HTTP_SRV_TLS               (0)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_HTTP_SRV_TLS_PORT
 * @brief   Default HTTP over TLS (HTTPS) port number (in network byte order).@n
 *          It can be changed during the HTTPS server initialization by the
 *          @ref fnet_http_srv_init() function.@n
 *          Default value FNET_HTONS(443).
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_HTTP_SRV_TLS_PORT
    #define FNET_CFG_HTTP_SRV_TLS_PORT          (FNET_HTONS(443u))
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_HTTP_SRV_REQUEST_SIZE_MAX
 * @brief   Maximum size of an incoming request.@n
 *          Also it defines the maximum number of bytes to use for internal
 *          buffering (parsing, receive and transmit buffering).@n
 *          Default value @b @c 300.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_HTTP_SRV_REQUEST_SIZE_MAX
    #define FNET_CFG_HTTP_SRV_REQUEST_SIZE_MAX  (300u)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_HTTP_SRV_VERSION_MAJOR
 * @brief   Hypertext Transfer Protocol HTTP version 1.x support:
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled. It means HTTP/0.9 (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_HTTP_SRV_VERSION_MAJOR
    #define FNET_CFG_HTTP_SRV_VERSION_MAJOR     (1u)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_HTTP_SRV_AUTHENTICATION_BASIC
 * @brief   The HTTP/1.x Basic Authentification Scheme (RFC2617) support:
 *               - @b @c 1 = is enabled (default value).
 *               - @c 0 = is disabled.
 ******************************************************************************/
#ifndef FNET_CFG_HTTP_SRV_AUTHENTICATION_BASIC
    #define FNET_CFG_HTTP_SRV_AUTHENTICATION_BASIC  (0)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_HTTP_SRV_POST
 * @brief   The HTTP/1.x POST method support:
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled (Default value).
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_HTTP_SRV_POST
    #define FNET_CFG_HTTP_SRV_POST                  (0)
#endif

/*! @} */

#if 0 /* Under development */

    /*! @addtogroup fnet_http_cln_config */
    /*! @{ */

    /**************************************************************************/ /*!
    * @def      FNET_CFG_HTTP_CLN
    * @brief    HTTP Client service support:
    *               - @c 1..n = is enabled. Its value defines maximum number of the HTTP Clients that can be run simultaneously.
    *               - @b @c 0 = is disabled (Default value).
    ******************************************************************************/
    #ifndef FNET_CFG_HTTP_CLN
        #define FNET_CFG_HTTP_CLN                   (0)
    #endif

    /**************************************************************************/ /*!
    * @def     FNET_CFG_HTTP_CLN_PORT
    * @brief   Default HTTP port number (in network byte order).@n
    *          It can be changed during the HTTP client initialization by the
    *          @ref fnet_http_cln_init() function.@n
    *          Default value FNET_HTONS(80).
    * @showinitializer
    ******************************************************************************/
    #ifndef FNET_CFG_HTTP_CLN_PORT
        #define FNET_CFG_HTTP_CLN_PORT              (FNET_HTONS(80u))
    #endif

    /**************************************************************************/ /*!
    * @def     FNET_CFG_HTTP_CLN_BUFFER_SIZE
    * @brief   Defines the HTTP client buffer size. It defines the maximum number
    *          of bytes to use for internal buffering (during receive and transmit).
    * @showinitializer
    ******************************************************************************/
    #ifndef FNET_CFG_HTTP_CLN_BUFFER_SIZE
        #define FNET_CFG_HTTP_CLN_BUFFER_SIZE       (500u)
    #endif

    /*! @} */
#endif /* Under development */

#ifdef FNET_CFG_HTTP_MAX
    #error "FNET_CFG_HTTP_MAX parameter is obsolete. Use FNET_CFG_HTTP_SRV."
#endif

#endif /* _FNET_HTTP_CONFIG_H_ */
