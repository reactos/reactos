/**************************************************************************
*
* Copyright 2016-2018 by Andrey Butok. FNET Community.
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
*  mDNS (Bonjour) Server/Responder configuration file.
*
***************************************************************************/

/**************************************************************************
 * !!!DO NOT MODIFY THIS FILE!!!
 **************************************************************************/

#ifndef _FNET_MDNS_CONFIG_H_

#define _FNET_MDNS_CONFIG_H_

/*! @addtogroup fnet_mdns_config */
/*! @{ */

/**************************************************************************/ /*!
 * @def      FNET_CFG_MDNS
 * @brief    Multicast Domain Name System (mDNS/Bonjour) Server/Responder (RFC6762) support:
 *               - @c 1..n = is enabled. Its value defines maximum number of
 *                   the mDNS Servers that can be run simultaneously.
 *               - @b @c 0 = is disabled (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_MDNS
    #define FNET_CFG_MDNS                       (0)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_MDNS_PORT
 * @brief   Default MDNS port number (in network byte order).@n
 *          The MDNS server/responder uses this port for sending and receiving of MDNS
 *          messages. @n
 *          RFC 6762: MUST listen for Multicast DNS replies sent to UDP destination port 5353.@n
 *          Default value is FNET_HTONS(5353). It is not recommended to change it.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_MDNS_PORT
    #define FNET_CFG_MDNS_PORT                  (FNET_HTONS(5353u))
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_MDNS_RR_TTL
 * @brief   Default TTL value for mDNS resource records. @n
 *          Default value is 120 seconds (recommended by RFC6762).
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_MDNS_RR_TTL
    #define FNET_CFG_MDNS_RR_TTL                (120)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_MDNS_SERVICE_MAX
 * @brief   Maximum number of advertised services supported by MDNS Server
 *          which may be registered by @ref fnet_mdns_service_register(). @n
 *          Default value is @b @c 2.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_MDNS_SERVICE_MAX
    #define FNET_CFG_MDNS_SERVICE_MAX           (2u)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_MDNS_SERVICE_TYPE_ENUMERATION
 * @brief    mDNS Service Tpe Enumeration (RFC6763) support:
 *               - @c 1 = is enabled (Default value).
 *               - @b @c 0 = is disabled .
 *
 *           @note: RFC6763: for problem diagnosis and network management tools, it may
 *           be useful for network administrators to find the list of advertised
 *           service types on the network, even if those Service Names are just
 *           opaque identifiers and not particularly informative in isolation.
 *           For this purpose, a special meta-query is defined.  A DNS query for
 *           PTR records with the name "_services._dns-sd._udp.Domain" yields a
 *           set of PTR records, where the rdata of each PTR record is the two-
 *           label Service name, plus the same domain.
 ******************************************************************************/
#ifndef FNET_CFG_MDNS_SERVICE_TYPE_ENUMERATION
    #define FNET_CFG_MDNS_SERVICE_TYPE_ENUMERATION                       (1)
#endif

/*! @} */

#ifdef FNET_CFG_MDNS_MAX
    #error "FNET_CFG_MDNS_MAX parameter is obsolete. Use FNET_CFG_MDNS."
#endif

#endif /* _FNET_LLMNR_CONFIG_H_ */
