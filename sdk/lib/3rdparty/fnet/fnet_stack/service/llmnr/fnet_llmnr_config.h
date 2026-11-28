/**************************************************************************
*
* Copyright 2014-2017 by Andrey Butok. FNET Community.
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
*  LLMNR Server/Responder configuration file.
*
***************************************************************************/

/**************************************************************************
 * !!!DO NOT MODIFY THIS FILE!!!
 **************************************************************************/

#ifndef _FNET_LLMNR_CONFIG_H_

#define _FNET_LLMNR_CONFIG_H_

/*! @addtogroup fnet_llmnr_config */
/*! @{ */

/**************************************************************************/ /*!
 * @def      FNET_CFG_LLMNR
 * @brief    Link-Local Multicast Name Resolution (LLMNR) Server/Responder support:
 *               - @c 1..n = is enabled. Its value defines maximum number of
 *                   the LLMNR Servers that can be run simultaneously.
 *               - @b @c 0 = is disabled (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_LLMNR
    #define FNET_CFG_LLMNR                      (0)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_LLMNR_PORT
 * @brief   Default LLMNR port number (in network byte order).@n
 *          The LLMNR server/responder uses this port for sending and receiving of LLMNR
 *          messages. @n
 *          RFC 4795: Responders MUST listen on UDP port 5355.@n
 *          Default value is FNET_HTONS(5355). It is not recommended to change it.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_LLMNR_PORT
    #define FNET_CFG_LLMNR_PORT                 (FNET_HTONS(5355u))
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_LLMNR_HOSTNAME_TTL
 * @brief   Default TTL value that indicates for how many seconds link-local
 *          host name is valid for LLMNR querier.@n
 *          A default is 30 seconds (recommended by RFC4795).@n
 *          In highly dynamic environments (such as mobile ad-hoc
 *          networks), the TTL value may need to be reduced.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_LLMNR_HOSTNAME_TTL
    #define FNET_CFG_LLMNR_HOSTNAME_TTL         (30)
#endif

/*! @} */

#ifdef FNET_CFG_LLMNR_MAX
    #error "FNET_CFG_LLMNR_MAX parameter is obsolete. Use FNET_CFG_LLMNR."
#endif

#endif /* _FNET_LLMNR_CONFIG_H_ */
