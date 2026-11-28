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
*  IPv4 protocol API.
*
***************************************************************************/

#ifndef _FNET_IP4_H_

#define _FNET_IP4_H_

/*! @addtogroup fnet_socket */
/*! @{ */

/**************************************************************************/ /*!
 * @brief 32-bit IPv4 address type.
 ******************************************************************************/
typedef fnet_uint32_t fnet_ip4_addr_t;

/**************************************************************************/ /*!
 * @def FNET_IP4_ADDR_INIT
 * @param a First octet of an IP address.
 * @param b Second octet of an IP address.
 * @param c Third  octet of an IP address.
 * @param d Fourth  octet of an IP address.
 * @hideinitializer
 * @brief Converts the standard dotted-decimal notation @c a.b.c.d
 *        to an integer value, suitable for use as an Internet address (in network byte order).
 ******************************************************************************/
#define FNET_IP4_ADDR_INIT(a, b, c, d)   ((fnet_ip4_addr_t)(FNET_NTOHL((((fnet_uint32_t)(a)&0xFFUL)<< 24U) + (((fnet_uint32_t)(b)&0xFFUL)<< 16U) + (((fnet_uint32_t)(c)&0xFFUL)<< 8U ) + ((fnet_uint32_t)(d)&0xFFUL))))

/**************************************************************************/ /*!
 * @def     FNET_IP4_ADDR_STR_SIZE
 * @brief   Size of the string buffer that will contain
 *          null-terminated ASCII string of an IPv4 address
 *          in standard "." notation.
 * @see fnet_inet_ntoa, fnet_inet_ntop
 * @showinitializer
 ******************************************************************************/
#define FNET_IP4_ADDR_STR_SIZE          sizeof("255.255.255.255")

/* 169.254/16 prefix that is valid for Link-Local communication. RFC3927*/
#define FNET_IP4_ADDR_LINK_LOCAL_PREFIX      FNET_IP4_ADDR_INIT(169,254,0,0)

/* IPv4 Link-Local broadcast. RFC3927*/
#define FNET_IP4_ADDR_LINK_LOCAL_BROADCAST   FNET_IP4_ADDR_INIT(169,254,255,255)

/************************************************************************
*    Definitions for IP type of service.
*************************************************************************/
#define FNET_IP4_TOS_NORMAL      (0x0U)
#define FNET_IP4_TOS_LOWDELAY    (0x10U)
#define FNET_IP4_TOS_THROUGHPUT  (0x08U)
#define FNET_IP4_TOS_RELIABILITY (0x04U)

/************************************************************************
*    Definitions of five different classes.
*************************************************************************/
#define FNET_IP4_CLASS_A(i)     (((fnet_ip4_addr_t)(i) & FNET_HTONL(0x80000000U)) == 0U)
#define FNET_IP4_CLASS_A_NET    FNET_HTONL(0xff000000U)

#define FNET_IP4_CLASS_B(i)     (((fnet_ip4_addr_t)(i) & FNET_HTONL(0xc0000000U)) == FNET_HTONL(0x80000000U))
#define FNET_IP4_CLASS_B_NET    FNET_HTONL(0xffff0000U)

#define FNET_IP4_CLASS_C(i)     (((fnet_ip4_addr_t)(i) & FNET_HTONL(0xe0000000U)) == FNET_HTONL(0xc0000000U))
#define FNET_IP4_CLASS_C_NET    FNET_HTONL(0xffffff00U)

#define FNET_IP4_CLASS_D(i)     (((fnet_ip4_addr_t)(i) & FNET_HTONL(0xf0000000U)) == FNET_HTONL(0xe0000000U))
#define FNET_IP4_CLASS_D_NET    FNET_HTONL(0xf0000000U)
/* Host groups are identified by class D IP addresses.*/
#define FNET_IP4_ADDR_IS_MULTICAST(addr)   (FNET_IP4_CLASS_D(addr)?FNET_TRUE:FNET_FALSE)

#define FNET_IP4_ADDR_IS_UNSPECIFIED(addr) (((addr) == INADDR_ANY)?FNET_TRUE:FNET_FALSE)

#define FNET_IP4_CLASS_E(i)     (((fnet_ip4_addr_t)(i) & FNET_HTONL(0xf0000000U)) == FNET_HTONL(0xf0000000U))
#define FNET_IP4_EXPERIMENTAL(i) FNET_IP4_CLASS_E(i)
#define FNET_IP4_BADCLASS(i)     FNET_IP4_CLASS_E(i)

#define FNET_IP4_ADDR1(ipaddr)   ((fnet_uint8_t)(fnet_ntohl(ipaddr) >> 24U) & 0xffU)
#define FNET_IP4_ADDR2(ipaddr)   ((fnet_uint8_t)(fnet_ntohl(ipaddr) >> 16U) & 0xffU)
#define FNET_IP4_ADDR3(ipaddr)   ((fnet_uint8_t)(fnet_ntohl(ipaddr) >> 8U) & 0xffU)
#define FNET_IP4_ADDR4(ipaddr)   ((fnet_uint8_t)(fnet_ntohl(ipaddr)) & 0xffU)

/*! @} */

#endif
