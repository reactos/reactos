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
*  FNET HTTP Server Authentication API.
*
***************************************************************************/

#ifndef _FNET_HTTP_SRV_AUTH_H_

#define _FNET_HTTP_SRV_AUTH_H_

#if (FNET_CFG_HTTP_SRV && FNET_CFG_HTTP_SRV_AUTHENTICATION_BASIC && FNET_CFG_HTTP_SRV_VERSION_MAJOR) || defined(__DOXYGEN__)

/*! @addtogroup fnet_http_srv
 @{ */

/**************************************************************************/ /*!
 * @brief Type of Authentication Scheme
 * @see fnet_http_srv_auth_t
 ******************************************************************************/
typedef enum
{
    FNET_HTTP_SRV_AUTH_SCHEME_NONE = 0,     /**< @brief No authentication.
                                         */
    FNET_HTTP_SRV_AUTH_SCHEME_BASIC = 1,    /**< @brief Basic Access Authentication Scheme.
                                         */
    FNET_HTTP_SRV_AUTH_SCHEME_DIGEST = 2    /**< @brief Digest Access Authentication Scheme (NOT YET IMPLEMENTED).
                                         */
} fnet_http_srv_auth_scheme_t;


/**************************************************************************/ /*!
* @brief HTTP Authentication table.
*
* HTTP server protects specified directories from unauthorized access.
* Directories that are not registered in this table are accessible by anyone.
* The last table element must have all fields set to zero as the end-of-table mark.@n
* @n
*   With HTTP authentication, the following things occur:
*   -# A client requests access to a protected resource.
*   -# The web server returns a dialog box that requests the user name and password.
*   -# The client submits the user name and password to the server.
*   -# The server validates the credentials and, if successful, returns the requested resource.
*
* @see fnet_http_srv_auth_scheme_t, fnet_http_srv_params_t
******************************************************************************/
typedef struct
{
    fnet_char_t *realm;	            /**< @brief Name of realm. A realm can be seen as an area for which the credentials are used.
                                         This is also the string that is shown when a WEB browser pops up the login window, for example: "Please enter your username and password for <realm>"
                                         @n It is optional and can be set to zero. In this case the sent realm will be equal to the @c dir_name parameter.*/
    fnet_char_t *dir_name;	        /**< @brief Name of the directory to protect. To protect all directories, set it to "".*/
    fnet_char_t *userid;            /**< @brief Required user name to access this directory.*/
    fnet_char_t *password;          /**< @brief Required password to access this directory.*/
    fnet_http_srv_auth_scheme_t scheme; /**< @brief Used Authentication Scheme.*/

} fnet_http_srv_auth_t;

/*! @} */

#endif /* FNET_CFG_HTTP_SRV && FNET_CFG_HTTP_SRV_AUTHENTICATION_BASIC */

#endif
