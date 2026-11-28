/**************************************************************************
*
* Copyright 2015-2018 by Andrey Butok. FNET Community.
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
*  FNET TLS library configuration file.
*
***************************************************************************/

/**************************************************************************
 * !!!DO NOT MODIFY THIS FILE!!!
 **************************************************************************/

#ifndef _FNET_TLS_CONFIG_H_

#define _FNET_TLS_CONFIG_H_

/** @addtogroup fnet_tls_config */
/** @{ */

/**************************************************************************/ /*!
 * @def      FNET_CFG_TLS
 * @brief    TLS Library support:
 *               - @c 1..n = is enabled (mbedTLS). Its value defines
 *                      the maximum number of the TLS contexts that can be initialized
 *                      simultaneously, using fnet_tls_init().
 *               - @b @c 0 = is disabled (Default value).
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_TLS
    #define FNET_CFG_TLS                (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_TLS_SOCKET_MAX
 * @brief    Maximum number of TLS sockets that may exist at the same time. @n
 *          Default value is @b @c 3.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_TLS_SOCKET_MAX
    #define FNET_CFG_TLS_SOCKET_MAX     (3u)
#endif

/** @} */

#ifdef FNET_CFG_TLS_MAX
    #error "FNET_CFG_TLS_MAX parameter is obsolete. Use FNET_CFG_TLS."
#endif

#endif /* _FNET_TLS_CONFIG_H_ */
