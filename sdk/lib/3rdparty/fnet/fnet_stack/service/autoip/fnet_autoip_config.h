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
***************************************************************************/
#ifndef _FNET_AUTOIP_CONFIG_H_

#define _FNET_AUTOIP_CONFIG_H_

/*! @addtogroup fnet_autoip_config */
/*! @{ */

/**************************************************************************/ /*!
 * @def      FNET_CFG_AUTOIP
 * @brief    Auto-IP service support (RFC3927 "Dynamic Configuration of IPv4 Link-Local Addresses"):
 *               - @c 1..n = is enabled. Its value defines maximum number of the Auto-IP services that can be run simultaneously.
 *                          @note It is not posible to run several Auto-IP servers on the same networking interface.
 *               - @b @c 0 = is disabled (Default value).
 ******************************************************************************/
#ifndef FNET_CFG_AUTOIP
    #define FNET_CFG_AUTOIP       (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_AUTOIP_DEFEND_INTERVAL
 * @brief    Auto-IP DEFEND_INTERVAL (RFC3927) support:
 *               - @c @c 1 = is enabled (Default value).@n
 *                          Upon receiving a conflicting ARP packet, if a host has not seen any
 *                          other conflicting ARP packets within the last DEFEND_INTERVAL (10)
 *                          seconds, it attempts to defend its address by
 *                          recording the time that the conflicting ARP packet was received, and
 *                          then broadcasting one single ARP announcement, giving its own IP and
 *                          hardware addresses as the sender addresses of the ARP.  Having done
 *                          this, the host can then continue to use the address normally without
 *                          any further special action.  However, if this is not the first
 *                          conflicting ARP packet the host has seen, and the time recorded for
 *                          the previous conflicting ARP packet is recent, within DEFEND_INTERVAL
 *                          seconds, then the host MUST immediately cease using this address and
 *                          configure a new IPv4 Link-Local address as described above.  This is
 *                          necessary to ensure that two hosts do not get stuck in an endless
 *                          loop with both hosts trying to defend the same address.
 *               - @b @c 0 = is disabled.@n
 *                          Upon receiving a conflicting ARP packet, a host
 *                          immediately configure a new IPv4 Link-Local address.
 ******************************************************************************/
#ifndef FNET_CFG_AUTOIP_DEFEND_INTERVAL
    #define FNET_CFG_AUTOIP_DEFEND_INTERVAL       (1)
#endif

/*! @} */

#ifdef FNET_CFG_AUTOIP_MAX
    #error "FNET_CFG_AUTOIP_MAX parameter is obsolete. Use FNET_CFG_AUTOIP."
#endif

#endif /*_FNET_AUTOIP_CONFIG_H_ */
