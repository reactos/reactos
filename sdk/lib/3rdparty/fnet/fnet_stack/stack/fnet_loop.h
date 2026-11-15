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
* Private. Loopback driver function definitions, data structures, etc.
*
***************************************************************************/

#ifndef _FNET_LOOP_H_

#define _FNET_LOOP_H_

#include "fnet.h"

#if FNET_CFG_LOOPBACK

#include "fnet_netif_prv.h"

/************************************************************************
*     Global Data Structures
*************************************************************************/

extern fnet_netif_t fnet_loop_if;
/******************************************************************************
 * Loopback interface descriptor @n
 *        It is availble only if FNET_CFG_LOOPBACK is set.
 ******************************************************************************/
#define FNET_LOOP_IF    ((fnet_netif_desc_t)(&fnet_loop_if))

/************************************************************************
*     Function Prototypes
*************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

void _fnet_loop_output_ip4(fnet_netif_t *netif, fnet_ip4_addr_t dest_ip_addr, fnet_netbuf_t *nb);
void _fnet_loop_output_ip6(struct fnet_netif *netif, const fnet_ip6_addr_t *src_ip_addr,  const fnet_ip6_addr_t *dest_ip_addr, fnet_netbuf_t *nb);

#if defined(__cplusplus)
}
#endif

#endif /* FNET_CFG_LOOPBACK */

#endif /* _FNET_LOOP_H_*/
