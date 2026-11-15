/**************************************************************************
*
* Copyright 2008-2016 by Andrey Butok. FNET Community.
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
*  Transport protocol interface implementation.
*
***************************************************************************/

#include "fnet.h"
#include "fnet_prot.h"
#include "fnet_tcp.h"
#include "fnet_udp.h"
#include "fnet_netif_prv.h"
#include "fnet_icmp4.h"
#include "fnet_icmp6.h"
#include "fnet_igmp.h"
#include "fnet_raw.h"

/************************************************************************
*     Global Data Structures
*************************************************************************/

/************************************************************************
*     List of Transport Layer Protocols.
*************************************************************************/
static fnet_prot_if_t *const fnet_prot_if_list[] =
{
#if FNET_CFG_IP4
    &fnet_icmp4_prot_if,      /* ICMP */
#endif
#if FNET_CFG_IP6
    &fnet_icmp6_prot_if,    /* ICMPv6 */
#endif
#if FNET_CFG_TCP
    &fnet_tcp_prot_if,     /* TCP */
#endif
#if FNET_CFG_UDP
    &fnet_udp_prot_if,     /* UDP */
#endif
#if FNET_CFG_IGMP && FNET_CFG_IP4
    &fnet_igmp_prot_if,    /* IGMP */
#endif
#if FNET_CFG_RAW
    &fnet_raw_prot_if,    /* RAW */
#endif

    /* ADD HERE YOUR TRANSPORT LAYER PROTOCOL */
};

#define FNET_PROT_TRANSPORT_IF_LIST_SIZE  (sizeof(fnet_prot_if_list)/sizeof(fnet_prot_if_t*))

/************************************************************************
* DESCRIPTION: Transport and IP layers initialization.
*************************************************************************/
fnet_return_t _fnet_prot_init( void )
{
    fnet_index_t    i;
    fnet_return_t   result = FNET_OK;

    fnet_isr_lock();

    for(i = 0u; i < FNET_PROT_TRANSPORT_IF_LIST_SIZE; i++)
    {
        fnet_prot_if_list[i]->head = 0;
        if(fnet_prot_if_list[i]->prot_init)
        {
            if(fnet_prot_if_list[i]->prot_init() == FNET_ERR)
            {
                result = FNET_ERR;
                goto ERROR;
            }
        }
    }

#if FNET_CFG_IP4
    if(_fnet_ip4_init() == FNET_ERR)
    {
        result = FNET_ERR;
        goto ERROR;
    }
#endif /* FNET_CFG_IP6 */

#if FNET_CFG_IP6
    if(_fnet_ip6_init() == FNET_ERR)
    {
        result = FNET_ERR;
        goto ERROR;
    }
#endif /* FNET_CFG_IP6 */

ERROR:
    fnet_isr_unlock();
    return result;
}

/************************************************************************
* DESCRIPTION: Transport and IP layers release.
*************************************************************************/
void _fnet_prot_release( void )
{
    fnet_index_t i;

    fnet_isr_lock();

    for(i = 0u; i < FNET_PROT_TRANSPORT_IF_LIST_SIZE; i++)
    {
        if(fnet_prot_if_list[i]->prot_release)
        {
            fnet_prot_if_list[i]->prot_release();
        }
    }

#if FNET_CFG_IP4
    _fnet_ip4_release();
#endif

#if FNET_CFG_IP6
    _fnet_ip6_release();
#endif

    fnet_isr_unlock();
}

/************************************************************************
* DESCRIPTION: This function looks up a protocol by domain family name,
*              by type and by protocol number.
*************************************************************************/
fnet_prot_if_t *_fnet_prot_find( fnet_address_family_t family, fnet_socket_type_t type, fnet_uint32_t protocol )
{
    fnet_index_t i;

    for(i = 0u; i < FNET_PROT_TRANSPORT_IF_LIST_SIZE; i++)
    {
        if( ((fnet_prot_if_list[i]->family & family) != 0) &&
            ( ((fnet_prot_if_list[i]->type == type) && (((protocol == 0u) || (fnet_prot_if_list[i]->protocol == 0u)) ? 1 : (fnet_prot_if_list[i]->protocol == protocol)))
              || ((type == SOCK_UNSPEC) && (fnet_prot_if_list[i]->protocol == protocol)) ) )
        {
            return (fnet_prot_if_list[i]);
        }
    }

    return (FNET_NULL);
}

/************************************************************************
* DESCRIPTION: Tries to free not critical parts of
*              dynamic allocated memory in the stack, if possible.
*************************************************************************/
void _fnet_prot_drain( void )
{
    fnet_size_t i;

#if 0 /* For debug needs.*/
    fnet_println("DRAIN");
#endif

    for(i = 0u; i < FNET_PROT_TRANSPORT_IF_LIST_SIZE; i++)
    {
        if(fnet_prot_if_list[i]->prot_drain)
        {
            fnet_prot_if_list[i]->prot_drain();
        }
    }

#if FNET_CFG_IP4
    _fnet_ip4_drain();
#endif

#if FNET_CFG_IP6
    _fnet_ip6_drain();
#endif

    _fnet_netif_drain();
}
