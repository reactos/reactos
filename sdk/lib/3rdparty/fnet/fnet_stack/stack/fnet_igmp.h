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
*  Private. IGMPv1/v2 protocol function definitions, data structures, etc.
*
***************************************************************************/

#ifndef _FNET_IGMP_H_

#define _FNET_IGMP_H_

#include "fnet.h"

#if FNET_CFG_IGMP

#include "fnet.h"
#include "fnet_netif_prv.h"
#include "fnet_prot.h"

/************************************************************************
 * RFC2236: The Internet Group Management Protocol (IGMP) is used by IP hosts to
 * report their host group memberships to any immediately-neighboring
 * multicast routers.
 ************************************************************************/

/************************************************************************
*     Definition of type and code field values.
*************************************************************************/

/************************************************************************
 * @internal
 * @brief    IGMP message header.
 ************************************************************************
 *
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |      Type     |    Unused     |           Checksum            |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                         Group Address                         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 ************************************************************************/

FNET_COMP_PACKED_BEGIN
typedef struct
{
    fnet_uint8_t      type FNET_COMP_PACKED;           /* Type.*/
    fnet_uint8_t      max_resp_time FNET_COMP_PACKED;  /* IGMPv1 Unused field, zeroed when sent, ignored when received.*/
    /* IGMPv2 Max Response Time (is meaningful only in Membership Query).*/
    /* NOTE: Current version of FNET ignores this parameter.*/
    fnet_uint16_t     checksum FNET_COMP_PACKED;       /* The checksum is the 16-bit one’s complement of the one’s
                                                      * complement sum of the 8-octet IGMP message.*/
    fnet_ip4_addr_t group_addr FNET_COMP_PACKED;     /* Group address field.*/
} fnet_igmp_header_t;
FNET_COMP_PACKED_END

/* IGMP Types */
#define IGMP_HEADER_TYPE_QUERY          0x11 /* Membership Query.*/
#define IGMP_HEADER_TYPE_REPORT_V1      0x12 /* Version 1 Membership Report.*/
#define IGMP_HEADER_TYPE_REPORT_V2      0x16 /* Version 2 Membership Report.*/
#define IGMP_HEADER_TYPE_LEAVE_GROUP    0x17 /* Leave Group.*/

extern fnet_prot_if_t fnet_igmp_prot_if;

/************************************************************************
*     Function Prototypes
*************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

void _fnet_igmp_join( fnet_netif_t *netif, fnet_ip4_addr_t  group_addr );
void _fnet_igmp_leave( fnet_netif_t *netif, fnet_ip4_addr_t  group_addr );

#if defined(__cplusplus)
}
#endif

#endif /* FNET_CFG_IGMP */

#endif /* _FNET_IGMP_H_ */
