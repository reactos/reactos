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
*  Private. MLD protocol function definitions, data structures, etc.
*
***************************************************************************/

#ifndef _FNET_MLD_H_

#define _FNET_MLD_H_

#include "fnet.h"

#if FNET_CFG_MLD

#include "fnet_netif_prv.h"
#include "fnet_icmp6.h"
#include "fnet_ip6_prv.h"

/************************************************************************
 * RFC3810: MLD is used by an IPv6 router to discover the presence of
 * multicast listeners on directly attached links, and to discover which
 * multicast addresses are of interest to those neighboring nodes.
 ************************************************************************/

/**********************************************************************
* MLD messages have the following format (RFC 2710)
***********************************************************************
*
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |     Type      |     Code      |          Checksum             |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |     Maximum Response Delay    |          Reserved             |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*   |                                                               |
*   +                                                               +
*   |                                                               |
*   +                       Multicast Address                       +
*   |                                                               |
*   +                                                               +
*   |                                                               |
*   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
***********************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct fnet_mld_header
{
    fnet_icmp6_header_t icmp6_header        FNET_COMP_PACKED;
    fnet_uint16_t       max_resp_delay      FNET_COMP_PACKED;
    fnet_uint8_t        _reserved[2]        FNET_COMP_PACKED;
    fnet_ip6_addr_t     multicast_addr      FNET_COMP_PACKED;
} fnet_mld_header_t;
FNET_COMP_PACKED_END

/***********************************************************************
 * MLD Router Alert option, in  IPv6 Hop-by-Hop Options.
 ***********************************************************************
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  | NextHeader(58)| Hdr Ext Len(0)|Option Type (5)|Opt Data Len(2)|
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *  | Value (0=MLD)                 |Option Type (1)|Opt Data Len(0)|                         |
 *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 ***********************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct fnet_mld_ra_option
{
    fnet_uint8_t                next_header         FNET_COMP_PACKED;   /* 8-bit selector. Identifies the type of header
                                                                         * immediately following the Options
                                                                         * header. */
    fnet_uint8_t                hdr_ext_length      FNET_COMP_PACKED;   /* 8-bit unsigned integer. Length of the Hop-by-
                                                                         * Hop Options header in 8-octet units, not
                                                                         * including the first 8 octets. */
    fnet_ip6_option_header_t    ra_option_header    FNET_COMP_PACKED;   /* Router Alert Option. */
    fnet_uint16_t               ra_option_value     FNET_COMP_PACKED;   /* Router Alert Option value. */
    fnet_ip6_option_header_t    padn_option_header  FNET_COMP_PACKED;   /* Padding. It must be multiple to 8 octets. */
} fnet_mld_ra_option_header_t;
FNET_COMP_PACKED_END

/************************************************************************
*     Function Prototypes
*************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

void _fnet_mld_join(fnet_netif_t *netif, fnet_ip6_addr_t  *group_addr);
void _fnet_mld_leave(fnet_netif_t *netif, fnet_ip6_addr_t  *group_addr);
void _fnet_mld_report_all(fnet_netif_t *netif);
void _fnet_mld_query_receive(fnet_netif_t *netif, fnet_ip6_addr_t *src_ip, fnet_ip6_addr_t *dest_ip, fnet_netbuf_t *nb, fnet_netbuf_t *ip6_nb);

#if defined(__cplusplus)
}
#endif

#endif /* FNET_CFG_MLD */

#endif /* _FNET_MLD_H_ */
