/**************************************************************************
*
* Copyright 2011-2018 by Andrey Butok. FNET Community.
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
*  PING service configuration file.
*
***************************************************************************/

/**************************************************************************
 * !!!DO NOT MODIFY THIS FILE!!!
 **************************************************************************/

#ifndef _FNET_PING_CONFIG_H_

#define _FNET_PING_CONFIG_H_

/*! @addtogroup fnet_ping_config */
/*! @{ */

/**************************************************************************/ /*!
 * @def      FNET_CFG_PING
 * @brief    PING service support:
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_PING
    #define FNET_CFG_PING           (0)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_PING_PACKET_MAX
 * @brief   The maximum size of the echo request packet. It defines send
 *          and receive buffer size, used by the PING service.@n
 *          Default value is @b @c 64.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_PING_PACKET_MAX
    #define FNET_CFG_PING_PACKET_MAX            (64U)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_PING_IDENTIFIER
 * @brief   The ICMP Identifier (in network byte order).@n
 *          It is used to help match echo requests to the associated reply.@n
 *          Default value is @b @c 1.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_PING_IDENTIFIER
    #define FNET_CFG_PING_IDENTIFIER            (FNET_HTONS(1U))
#endif

/*! @} */

#endif /* _FNET_PING_CONFIG_H_ */
