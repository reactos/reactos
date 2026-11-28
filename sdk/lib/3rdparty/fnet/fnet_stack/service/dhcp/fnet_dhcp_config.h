/**************************************************************************
*
* Copyright 2008-2017 by Andrey Butok. FNET Community.
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
***************************************************************************/

/**************************************************************************
 * !!!DO NOT MODIFY THIS FILE!!!
 **************************************************************************/

#ifndef _FNET_DHCP_CONFIG_H_

#define _FNET_DHCP_CONFIG_H_

/*! @addtogroup fnet_dhcp_cln_config */
/*! @{ */

/**************************************************************************/ /*!
 * @def      FNET_CFG_DHCP_CLN
 * @brief    DHCP Client service support:
 *               - @c 1..n = is enabled. Its value defines maximum number of the DHCPv4 Clients that can be run simultaneously.@n
 *                          @note It is not posible to run several DHCPv4 Clients on the same networking interface.
 *               - @b @c 0 = is disabled (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_DHCP_CLN
    #define FNET_CFG_DHCP_CLN       (0)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_DHCP_CLN_BOOTP
 * @brief   DHCP/BOOTP-service used protocol:
 *               - @c 1 = BOOTP (Bootstrap) protocol, defined by RFC951.
 *               - @b @c 0 = DHCP protocol (extension of BOOTP), defined by RFC2131 (Default value).@n
 *          @n
 *          BOOTP typically provides fixed allocation of a single IP address
 *          for each client, permanently reserving this address in the BOOTP
 *          server database.@n
 *          DHCP typically provides dynamic, leased allocation of available
 *          IP addresses, reserving each DHCP client address temporarily
 *          in the DHCP server database.@n
 *          DHCP can automatically rebind or renew their leases while BOOTP requires a system restart.@n
 *          Code size of BOOTP is less than DHCP by about 1.5KB.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_DHCP_CLN_BOOTP
    #define FNET_CFG_DHCP_CLN_BOOTP                 (0)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_DHCP_CLN_PORT
 * @brief   DHCP client port number (in network byte order).@n
 *          The client listen DHCP messages from a server on this port.@n
 *          Default value is @b @c 68.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_DHCP_CLN_PORT
    #define FNET_CFG_DHCP_CLN_PORT           (FNET_HTONS(68U))
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_DHCP_CLN_RESPONSE_TIMEOUT_MS
 * @brief   Timeout for the response from the remote DHCP server (in milliseconds).@n
 *          The recommended value is from @c 1000 till @c 10000 ms.@n
 *          If the DHCP client does not receive any response from a DHCP server during this time,
 *          it sends new discover message.
 *          During run time, it may be changed by @ref fnet_dhcp_cln_set_response_timeout().@n
 *          Default value is @b @c 4000 ms. @n
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_DHCP_CLN_RESPONSE_TIMEOUT_MS
    #define FNET_CFG_DHCP_CLN_RESPONSE_TIMEOUT_MS   (4000U)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_DHCP_CLN_BROADCAST
 * @brief   DHCP "Broadcast Address Option" support [RFC 2132 5.3]:
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled (Default value).@n
 *          @n
 *          [RFC 2132 9.3]: "This option specifies the broadcast address
 *          in use on the client's subnet."
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_DHCP_CLN_BROADCAST
    #define FNET_CFG_DHCP_CLN_BROADCAST             (0)
#endif

/*! @} */

/*! @addtogroup fnet_dhcp_srv_config */
/*! @{ */

/**************************************************************************/ /*!
 * @def      FNET_CFG_DHCP_SRV
 * @brief    DHCPv4 Server service support:
 *               - @c 1..n = is enabled. Its value defines maximum number of the DHCPv4 Servers that can be run simultaneously.@n
 *                          @note It is not posible to run several DHCPv4 servers on the same networking interface.
 *               - @b @c 0 = is disabled (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_DHCP_SRV
    #define FNET_CFG_DHCP_SRV                       (0)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_DHCP_SRV_PORT
 * @brief   DHCP server port number (in network byte order).@n
 *          DHCP messages from a client to a server are sent to this port.@n
 *          Default value is @b @c 67.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_DHCP_SRV_PORT
    #define FNET_CFG_DHCP_SRV_PORT                  (FNET_HTONS(67U))
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_DHCP_SRV_ADDR_POOL_SIZE
 * @brief   Size of the IPv4 address pool.@n
 *          It defines maximum number of IPv4 addresses that may be allocated by the DHCPv4 Server.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_DHCP_SRV_ADDR_POOL_SIZE
    #define FNET_CFG_DHCP_SRV_ADDR_POOL_SIZE        (5)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_DHCP_SRV_LEASE_TIME
 * @brief   Default lease time in seconds.
 *          It may be changed during DHCP Server initialization.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_DHCP_SRV_LEASE_TIME
    #define FNET_CFG_DHCP_SRV_LEASE_TIME            (60*5)
#endif

/*! @} */


#ifdef FNET_CFG_DHCP_MAX
    #error "FNET_CFG_DHCP_MAX parameter is obsolete. Use FNET_CFG_DHCP_CLN."
#endif

#endif /* _FNET_DHCP_CONFIG_H_ */
