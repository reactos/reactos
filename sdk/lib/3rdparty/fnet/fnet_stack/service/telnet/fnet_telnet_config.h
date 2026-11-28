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
* FNET Telnet Server configuration file.
*
***************************************************************************/

/**************************************************************************
 * !!!DO NOT MODIFY THIS FILE!!!
 **************************************************************************/

#ifndef _FNET_TELNET_CONFIG_H_

#define _FNET_TELNET_CONFIG_H_

/** @addtogroup fnet_telnet_config */
/** @{ */

/**************************************************************************/ /*!
 * @def      FNET_CFG_TELNET
 * @brief    Telnet server support:
 *               - @c 1..n = is enabled. Its value defines maximum number of
 *                   the Telnet Servers that can be run simultaneously.
 *               - @b @c 0 = is disabled (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_TELNET
    #define FNET_CFG_TELNET                     (0)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_TELNET_SESSION_MAX
 * @brief   Maximum number of simultaneous user-session that can be handled
 *          by the Telnet server.@n
 *          Default value is @b @c 1.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_TELNET_SESSION_MAX
    #define FNET_CFG_TELNET_SESSION_MAX         (1u)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_TELNET_PORT
 * @brief   Default Telnet port number (in network byte order).@n
 *          It can be changed during the Telnet server initialization by the
 *          @ref fnet_telnet_init() function. @n
 *          Default value is FNET_HTONS(23).
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_TELNET_PORT
    #define FNET_CFG_TELNET_PORT                (FNET_HTONS(23u))
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_TELNET_SHELL_ECHO
 * @brief   Echo in the Tenet shell:
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled (Default value).
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_TELNET_SHELL_ECHO
    #define FNET_CFG_TELNET_SHELL_ECHO          (0)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_TELNET_SOCKET_BUF_SIZE
 * @brief   Size of the socket RX & TX buffer used by the Telnet server. @n
 *          Default value is @b @c 60.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_TELNET_SOCKET_BUF_SIZE
    #define FNET_CFG_TELNET_SOCKET_BUF_SIZE     (60u)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_TELNET_CMD_LINE_BUF_SIZE
 * @brief   Size of the command-line buffer used by the Telnet Shell. @n
 8          It defines the maximum length of the entered input-command line.
 *          Default value is @b @c 60.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_TELNET_CMD_LINE_BUF_SIZE
    #define FNET_CFG_TELNET_CMD_LINE_BUF_SIZE   (60u)
#endif

/** @} */

#ifdef FNET_CFG_TELNET_MAX
    #error "FNET_CFG_TELNET_MAX parameter is obsolete. Use FNET_CFG_TELNET."
#endif

#endif /* _FNET_TELNET_CONFIG_H_ */
