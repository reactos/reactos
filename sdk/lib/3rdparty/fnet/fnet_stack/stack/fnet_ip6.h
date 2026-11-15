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
*  IPv6 protocol API.
*
***************************************************************************/

#ifndef _FNET_IP6_H_

#define _FNET_IP6_H_

/*! @addtogroup fnet_socket */
/*! @{ */

/**************************************************************************/ /*!
 * @brief 128-bit IPv6 address type.
 ******************************************************************************/
FNET_COMP_PACKED_BEGIN
/* 128-bit IP6 address */
typedef union
{
    fnet_uint8_t   addr[16];
    fnet_uint16_t  addr16[8];
    fnet_uint32_t  addr32[4];
} fnet_ip6_addr_t;
FNET_COMP_PACKED_END

/**************************************************************************/ /*!
 * @brief   Size of the string buffer that will contain
 *          null-terminated ASCII string of an IPv6 address
 *          in standard ":" notation.
 * @see fnet_inet_ntop
 * @showinitializer
 ******************************************************************************/
#define FNET_IP6_ADDR_STR_SIZE       sizeof("abcd:abcd:abcd:abcd:abcd:abcd:abcd:abcd")

/*! @} */

/******************************************************************
* Constants
*******************************************************************/
#define FNET_IP6_HEAD_SIZE       40u     /* IPv6 Header size.  */

#define FNET_IP6_DEFAULT_MTU     1280u   /* Minimum IPv6 datagram size which    
                                         * must be supported by all IPv6 hosts */

/****************************************************************
 *
 * Helpful macros.
 *
 *****************************************************************/
#define FNET_IP6_ADDR_INIT(a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16)      \
    {{ (a1), (a2), (a3), (a4), (a5), (a6), (a7), (a8), (a9), (a10), (a11), (a12), (a13), (a14), (a15), (a16) }}

/*
 * Definition of some useful macros to handle IP6 addresses
 */
#define FNET_IP6_ADDR_ANY_INIT                      FNET_IP6_ADDR_INIT(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)
#define FNET_IP6_ADDR_LOOPBACK_INIT                 FNET_IP6_ADDR_INIT(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01)
#define FNET_IP6_ADDR_NODELOCAL_ALLNODES_INIT       FNET_IP6_ADDR_INIT(0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01)
#define FNET_IP6_ADDR_INTFACELOCAL_ALLNODES_INIT    FNET_IP6_ADDR_INIT(0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01)
#define FNET_IP6_ADDR_LINKLOCAL_ALLNODES_INIT       FNET_IP6_ADDR_INIT(0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01)
#define FNET_IP6_ADDR_LINKLOCAL_ALLROUTERS_INIT     FNET_IP6_ADDR_INIT(0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02)
#define FNET_IP6_ADDR_LINKLOCAL_ALLV2ROUTERS_INIT   FNET_IP6_ADDR_INIT(0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16)
#define FNET_IP6_ADDR_LINKLOCAL_PREFIX_INIT         FNET_IP6_ADDR_INIT(0xFE, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)

extern const fnet_ip6_addr_t fnet_ip6_addr_any;
extern const fnet_ip6_addr_t fnet_ip6_addr_loopback;
extern const fnet_ip6_addr_t fnet_ip6_addr_nodelocal_allnodes;
extern const fnet_ip6_addr_t fnet_ip6_addr_linklocal_allnodes;
extern const fnet_ip6_addr_t fnet_ip6_addr_linklocal_allrouters;
extern const fnet_ip6_addr_t fnet_ip6_addr_linklocal_allv2routers;
extern const fnet_ip6_addr_t fnet_ip6_addr_linklocal_prefix;

/* Equality. */
#define FNET_IP6_ADDR_EQUAL(a, b)			\
    ((fnet_memcmp(&(a)->addr[0], &(b)->addr[0], sizeof(fnet_ip6_addr_t)) == 0)?FNET_TRUE:FNET_FALSE)

/* Copying address. */
#define FNET_IP6_ADDR_COPY(from_addr, to_addr)  \
    (fnet_memcpy(&(to_addr)->addr[0], &(from_addr)->addr[0], sizeof(fnet_ip6_addr_t)))

/* Unspecified.*/
#define FNET_IP6_ADDR_IS_UNSPECIFIED(a) \
    (((((a)->addr32[0]) == 0U) &&	\
      (((a)->addr32[1]) == 0U) &&	\
      (((a)->addr32[2]) == 0U) &&	\
      (((a)->addr32[3]) == 0U))?FNET_TRUE:FNET_FALSE)

/* Loopback.*/
#define FNET_IP6_ADDR_IS_LOOPBACK(a)    \
    ((((a)->addr32[0]) == 0U) &&	\
     (((a)->addr32[1]) == 0U) &&	\
     (((a)->addr32[2]) == 0U) &&	\
     (((a)->addr32[3]) == FNET_NTOHL(1U)))

/* Multicast. */
#define FNET_IP6_ADDR_IS_MULTICAST(a)	((((a)->addr[0]) == 0xffU)?FNET_TRUE:FNET_FALSE)

/* Unicast Scope.*/
#define FNET_IP6_ADDR_IS_LINKLOCAL(a)	\
    ((((a)->addr[0]) == 0xfeU) && ((((a)->addr[1]) & 0xc0U) == 0x80U))
#define FNET_IP6_ADDR_IS_SITELOCAL(a)	\
    ((((a)->addr[0]) == 0xfeU) && ((((a)->addr[1]) & 0xc0U) == 0xc0U))

/************************************************************************
*     Function Prototypes
*************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/* Selects the best source address to use with a destination address, Based on RFC3484. */
const fnet_ip6_addr_t *fnet_ip6_select_src_addr(const fnet_ip6_addr_t *dest_addr);

#if defined(__cplusplus)
}
#endif

#endif  /* _FNET_IP6_H_ */
