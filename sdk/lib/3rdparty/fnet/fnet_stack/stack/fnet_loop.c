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
*  Loopback driver implementation.
*
***************************************************************************/

#include "fnet.h"
#if FNET_CFG_LOOPBACK

#include "fnet_loop.h"
#include "fnet_ip4_prv.h"
#include "fnet_ip6_prv.h"

#if (FNET_CFG_LOOPBACK_MTU < 200u)
    #error  "FNET_CFG_LOOPBACK_MTU must be more than 200."
#endif

/************************************************************************
*     Function Prototypes
*************************************************************************/
static fnet_return_t fnet_loop_init( fnet_netif_t *netif );

/************************************************************************
*     Global Data Structures
*************************************************************************/
/* Loopback API. */
const struct fnet_netif_api fnet_loop_api =
{
    .netif_type = FNET_NETIF_TYPE_LOOPBACK,     /* Data-link type. */
    .netif_init = fnet_loop_init,               /* Initialization function.*/
#if FNET_CFG_IP4
    .netif_output_ip4 = _fnet_loop_output_ip4,   /* IPv4 Transmit function.*/
#endif
#if FNET_CFG_IP6
    .netif_output_ip6 = _fnet_loop_output_ip6,   /* IPv6 Transmit function.*/
#endif /* FNET_CFG_IP6 */
};

/* Loopback Interface structure.*/
fnet_netif_t fnet_loop_if =
{
    .netif_name = FNET_CFG_LOOPBACK_NAME,   /* Network interface name.*/
    .netif_mtu = FNET_CFG_LOOPBACK_MTU,     /* Maximum transmission unit.*/
    .netif_api = &fnet_loop_api             /* Interface API */
};

/************************************************************************
* DESCRIPTION: This function just only sends outgoing packets to IP layer.
*************************************************************************/
#if FNET_CFG_IP4
void _fnet_loop_output_ip4(fnet_netif_t *netif, fnet_ip4_addr_t dest_ip_addr, fnet_netbuf_t *nb)
{
    FNET_COMP_UNUSED_ARG(dest_ip_addr);

    fnet_isr_lock();

    /* MTU check */
    if (nb->total_length <= netif->netif_mtu)
    {
        _fnet_ip4_input(netif, nb);
    }
    else
    {
        _fnet_netbuf_free_chain(nb);
    }

    fnet_isr_unlock();
}
#endif /* FNET_CFG_IP4 */

/************************************************************************
* DESCRIPTION: This function just only sends outgoing packets to IPv6 layer.
*************************************************************************/
#if FNET_CFG_IP6
void _fnet_loop_output_ip6(fnet_netif_t *netif, const fnet_ip6_addr_t *src_ip_addr, const fnet_ip6_addr_t *dest_ip_addr, fnet_netbuf_t *nb)
{
    FNET_COMP_UNUSED_ARG(dest_ip_addr);
    FNET_COMP_UNUSED_ARG(src_ip_addr);

    fnet_isr_lock();

    /* MTU check */
    if (nb->total_length <= netif->netif_mtu)
    {
        _fnet_ip6_input(netif, nb);
    }
    else
    {
        _fnet_netbuf_free_chain(nb);
    }

    fnet_isr_unlock();
}
#endif /* FNET_CFG_IP6 */

/************************************************************************
* DESCRIPTION: Initialization of loopback interface.
*************************************************************************/
static fnet_return_t fnet_loop_init(fnet_netif_t *netif)
{
    /* Set address parameters of the Loopback interface.*/
#if FNET_CFG_IP4
    _fnet_netif_set_ip4_addr(netif, FNET_CFG_LOOPBACK_IP4_ADDR, INADDR_ANY);
#endif /* FNET_CFG_LOOPBACK */
#if FNET_CFG_IP6
    _fnet_netif_bind_ip6_addr(netif, &fnet_ip6_addr_loopback, FNET_NETIF_IP_ADDR_TYPE_MANUAL,
                              FNET_NETIF_IP6_ADDR_LIFETIME_INFINITE,
                              FNET_ND6_PREFIX_LENGTH_DEFAULT);
#endif /* FNET_CFG_LOOPBACK */

    return FNET_OK;
}

#endif /* FNET_CFG_LOOPBACK */
