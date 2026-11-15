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
*  FNET Network interface implementation.
*
***************************************************************************/

#include "fnet.h"
#include "fnet_ip4_prv.h"
#include "fnet_ip6_prv.h"
#include "fnet_netif_prv.h"
#include "fnet_eth_prv.h"
#include "fnet_nd6.h"
#include "fnet_stack_prv.h"

#if FNET_CFG_NETIF_IP6_ADDR_MAX < 2u
    #error "FNET_CFG_NETIF_IP6_ADDR_MAX must be > 1"
#endif

/************************************************************************
*     Global Data Structures
*************************************************************************/
#define FNET_NETIF_PMTU_TIMEOUT_MS       (10u*60u*1000u)   /* ms. RFC1981: The recommended setting for this
                                                           * timer is twice its minimum value (10 minutes).*/
#define FNET_NETIF_PMTU_PERIOD_MS        (FNET_NETIF_PMTU_TIMEOUT_MS/10u)   /* ms. RFC1981: Once a minute.*/
#define FNET_NETIF_IS_CONNECTED_PERIOD   (200)  /* ms. Period used to limit netif->netif_api->is_connected() call rate, cause of high amount of time used by PHY register read.*/

fnet_netif_t *fnet_netif_list;          /* The list of network interfaces. */
fnet_netif_t *fnet_netif_default;       /* Default net_if. */

/* Duplicated IP event handler.*/
#if FNET_CFG_IP4
    static fnet_netif_callback_ip4_addr_conflict_t         fnet_netif_callback_ip4_addr_conflict;
#endif

/************************************************************************
*     Function Prototypes
*************************************************************************/
static void _fnet_netif_assign_scope_id( fnet_netif_t *netif );
#if FNET_CFG_IP6 && FNET_CFG_IP6_PMTU_DISCOVERY
    static void _fnet_netif_pmtu_timer( fnet_uint32_t cookie);
#endif

/************************************************************************
* DESCRIPTION: Releases all installed interfaces.
*************************************************************************/
void _fnet_netif_release_all( void )
{
    fnet_netif_t *net_if_ptr;

#if FNET_CFG_IP4
    fnet_netif_set_callback_on_ip4_addr_conflict(0); /* Reset dupip handler.*/
#endif

    for (net_if_ptr = fnet_netif_list; net_if_ptr; net_if_ptr = net_if_ptr->next)
    {
        _fnet_netif_release(net_if_ptr);
    }

    fnet_netif_default = 0;
    fnet_netif_list = 0;
}

/************************************************************************
* DESCRIPTION: This function calls "drain" functions of all currently
*              installed network interfaces.
*************************************************************************/
void _fnet_netif_drain( void )
{
    fnet_netif_t *net_if_ptr;

    fnet_isr_lock();

    for (net_if_ptr = fnet_netif_list; net_if_ptr; net_if_ptr = net_if_ptr->next)
    {
        if(net_if_ptr->netif_api->netif_drain)
        {
            net_if_ptr->netif_api->netif_drain(net_if_ptr);
        }
    }

    fnet_isr_unlock();
}

/************************************************************************
* DESCRIPTION: Returns a network interface given its name.
*************************************************************************/
fnet_netif_desc_t fnet_netif_get_by_name( const fnet_char_t *name )
{
    fnet_netif_t        *netif;
    fnet_netif_desc_t   result = (fnet_netif_desc_t)FNET_NULL;

    _fnet_stack_mutex_lock();

    if(name)
    {
        for (netif = fnet_netif_list; netif != 0; netif = netif->next)
        {
            if(fnet_strncmp(name, netif->netif_name, FNET_NETIF_NAMELEN) == 0)
            {
                result = (fnet_netif_desc_t)netif;
                break;
            }
        }
    }

    _fnet_stack_mutex_unlock();

    return result;
}

/************************************************************************
* DESCRIPTION: This function returns pointer to id-th interface according
*              its index (from zero).
*              It returns FNET_NULL if id-th interface is not available.
*************************************************************************/
fnet_netif_desc_t fnet_netif_get_by_number( fnet_index_t n )
{
    fnet_netif_desc_t result;

    _fnet_stack_mutex_lock();
    result = (fnet_netif_desc_t)_fnet_netif_get_by_number(n);
    _fnet_stack_mutex_unlock();

    return result;
}
/* Private */
fnet_netif_t *_fnet_netif_get_by_number( fnet_index_t n )
{
    fnet_netif_t    *result = FNET_NULL;
    fnet_netif_t    *current;

    for (current = fnet_netif_list; current; n--)
    {
        if(n == 0u)
        {
            result = current;
            break;
        }

        current = current->next;
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Returns a network interface given its IPv4 address.
*************************************************************************/
fnet_netif_desc_t fnet_netif_get_by_ip4_addr( fnet_ip4_addr_t addr )
{
    fnet_netif_desc_t result;

    _fnet_stack_mutex_lock();
    result = (fnet_netif_desc_t)_fnet_netif_get_by_ip4_addr(addr);
    _fnet_stack_mutex_unlock();

    return result;
}
/* Private */
fnet_netif_t *_fnet_netif_get_by_ip4_addr( fnet_ip4_addr_t addr )
{
    fnet_netif_t *result = FNET_NULL;

#if FNET_CFG_IP4
    fnet_netif_t *netif;

    for (netif = fnet_netif_list; netif != 0; netif = netif->next)
    {
        if(addr == netif->ip4_addr.address)
        {
            result = (fnet_netif_desc_t)netif;
            break;
        }
    }
#else
    FNET_COMP_UNUSED_ARG(addr);
#endif /* FNET_CFG_IP4 */

    return result;
}

/************************************************************************
* DESCRIPTION: Returns a network interface based on socket address.
*************************************************************************/
fnet_netif_desc_t fnet_netif_get_by_sockaddr( const struct fnet_sockaddr *addr )
{
    fnet_netif_desc_t result;

    _fnet_stack_mutex_lock();
    result = (fnet_netif_desc_t)_fnet_netif_get_by_sockaddr(addr);
    _fnet_stack_mutex_unlock();

    return result;
}
/* Private */
fnet_netif_t *_fnet_netif_get_by_sockaddr( const struct fnet_sockaddr *addr )
{
    fnet_netif_desc_t result = (fnet_netif_desc_t)FNET_NULL;

    if(addr)
    {
        switch(addr->sa_family)
        {
#if FNET_CFG_IP4
            case AF_INET:
                result = _fnet_netif_get_by_ip4_addr( ((const struct fnet_sockaddr_in *)addr)->sin_addr.s_addr);
                break;
#endif /* FNET_CFG_IP4 */
#if FNET_CFG_IP6
            case AF_INET6:
                result = _fnet_netif_get_by_ip6_addr( &((const struct fnet_sockaddr_in6 *)addr)->sin6_addr.s6_addr);
                break;
#endif /* FNET_CFG_IP6 */
            default:
                break;
        }
    }

    return result;
}


#if FNET_CFG_IP6 && FNET_CFG_IP6_PMTU_DISCOVERY
/************************************************************************
* DESCRIPTION: Sets PMTU of the interface.
*************************************************************************/
void _fnet_netif_set_pmtu(fnet_netif_t *netif, fnet_size_t pmtu)
{
    /* Set Path MTU for the link. */
    netif->pmtu = pmtu;

    netif->pmtu_timestamp_ms = fnet_timer_get_ms();
}

/************************************************************************
* DESCRIPTION: Timer routine used to detect increase of PMTU
*************************************************************************/
static void _fnet_netif_pmtu_timer( fnet_uint32_t cookie )
{
    fnet_netif_t    *netif = (fnet_netif_t *)cookie;

    if((fnet_timer_get_ms() - netif->pmtu_timestamp_ms) > FNET_NETIF_PMTU_TIMEOUT_MS)
    {
        _fnet_netif_set_pmtu(netif, netif->netif_mtu);
    }
}

/************************************************************************
* DESCRIPTION: Initialize PMTU Discovery for the interface.
*************************************************************************/
void _fnet_netif_pmtu_init(fnet_netif_t *netif)
{
    /* Path MTU for the link. */
    _fnet_netif_set_pmtu(netif, netif->netif_mtu);

    /* Register timer, to detect increase of PMTU.*/
    netif->pmtu_timer = _fnet_timer_new(FNET_NETIF_PMTU_PERIOD_MS, _fnet_netif_pmtu_timer, (fnet_uint32_t)netif);
}

/************************************************************************
* DESCRIPTION: Release/Disable PMTU Discovery for the interface.
*************************************************************************/
void _fnet_netif_pmtu_release(fnet_netif_t *netif)
{
    _fnet_timer_free(netif->pmtu_timer);

    netif->pmtu = 0u;
}

#endif /* FNET_CFG_IP6 && FNET_CFG_IP6_PMTU_DISCOVERY */

/************************************************************************
* DESCRIPTION: This function installs & inits a network interface.
*************************************************************************/
fnet_return_t fnet_netif_init(fnet_netif_desc_t netif_desc, fnet_uint8_t *hw_addr, fnet_size_t hw_addr_size )
{
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;
    fnet_return_t   result;

    _fnet_stack_mutex_lock();
    result = _fnet_netif_init(netif, hw_addr, hw_addr_size);
    _fnet_stack_mutex_unlock();

    return result;
}
/* Private */
fnet_return_t _fnet_netif_init(fnet_netif_t *netif, fnet_uint8_t *hw_addr, fnet_size_t hw_addr_size )
{
    fnet_return_t   result = FNET_ERR;

    if(netif && (netif->netif_api) && (_fnet_netif_is_initialized(netif) == FNET_FALSE))
    {
        fnet_isr_lock();

        netif->next = fnet_netif_list;

        if(netif->next != 0)
        {
            netif->next->prev = netif;
        }

        netif->prev = 0;
        fnet_netif_list = netif;

        _fnet_netif_assign_scope_id( netif ); /* Assign Scope ID.*/

        netif->features = FNET_NETIF_FEATURE_NONE;

        /* Interface HW initialization.*/
        if((netif->netif_api->netif_init) && ((result = netif->netif_api->netif_init(netif)) == FNET_OK))
        {
#if FNET_CFG_IGMP && FNET_CFG_IP4
            /**************************************************************
             * RFC1112 7.2
             *   To support IGMP, every level 2 host must
             *   join the "all-hosts" group (address 224.0.0.1) on each network
             *   interface at initialization time and must remain a member for as long
             *   as the host is active.
             *
             * NOTE:(224.0.0.1) membership is never reported by IGMP.
             **************************************************************/
            /* Join HW interface. */
            _fnet_netif_join_ip4_multicast ( netif, FNET_IP4_ADDR_INIT(224, 0, 0, 1) );
#endif
            /* Set HW Address.*/
            if(hw_addr && hw_addr_size)
            {
                result = _fnet_netif_set_hw_addr(netif, hw_addr, hw_addr_size);
            }

            if(result == FNET_OK)
            {
                /* Interface-Type specific initialisation. */
                switch(netif->netif_api->netif_type)
                {
                    case (FNET_NETIF_TYPE_WIFI):
                    case (FNET_NETIF_TYPE_ETHERNET):
                        result = _fnet_eth_init(netif);
                        break;
                    default:
                        break;
                }
            }
        }

        if(result == FNET_OK)
        {
            if(fnet_netif_default == FNET_NULL)
            {
                /* Set default interface, if it is not done yet.*/
                fnet_netif_default = netif;
            }
        }
        else /* Clear if any error.*/
        {
            _fnet_netif_release(netif);
        }

        fnet_isr_unlock();
    }

    return result;
}

/************************************************************************
* DESCRIPTION: This function releases a network interface.
*************************************************************************/
void fnet_netif_release(fnet_netif_desc_t netif_desc)
{
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;

    _fnet_stack_mutex_lock();
    _fnet_netif_release(netif);
    _fnet_stack_mutex_unlock();
}
/* Private */
void _fnet_netif_release(fnet_netif_t *netif)
{
    if(netif && (_fnet_netif_is_initialized(netif) == FNET_TRUE))
    {
        if(netif->netif_api->netif_release)
        {
            netif->netif_api->netif_release(netif);
        }

        /* Remove from the interface list.*/
        if(netif->prev == 0)
        {
            fnet_netif_list = netif->next;
        }
        else
        {
            netif->prev->next = netif->next;
        }

        if(netif->next != 0)
        {
            netif->next->prev = netif->prev;
        }

        /* If this was the default interface.*/
        if(fnet_netif_default == netif)
        {
            /* Switch it to the first one. */
            fnet_netif_default = fnet_netif_list;
        }
    }
}

/************************************************************************
* DESCRIPTION: This function determines if the network interface is initialized.
*************************************************************************/
fnet_bool_t fnet_netif_is_initialized(fnet_netif_desc_t netif_desc)
{
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;
    fnet_bool_t     result;

    _fnet_stack_mutex_lock();
    result = _fnet_netif_is_initialized(netif);
    _fnet_stack_mutex_unlock();

    return result;
}
/* Private */
fnet_bool_t _fnet_netif_is_initialized(fnet_netif_t *netif)
{
    fnet_bool_t     result = FNET_FALSE;
    fnet_netif_t    *current;

    for (current = fnet_netif_list; current; current = current->next)
    {
        if(current == netif)
        {
            result = FNET_TRUE;
            break;
        }
    }

    return result;
}

/************************************************************************
* DESCRIPTION: This function sets the default network interface.
*************************************************************************/
void fnet_netif_set_default( fnet_netif_desc_t netif_desc )
{
    _fnet_stack_mutex_lock();
    fnet_netif_default = (fnet_netif_t *)netif_desc;
    _fnet_stack_mutex_unlock();
}

/************************************************************************
* DESCRIPTION: This function sets the IPv4 address.
*************************************************************************/
#if FNET_CFG_IP4
void fnet_netif_set_ip4_addr( fnet_netif_desc_t netif_desc, fnet_ip4_addr_t ipaddr, fnet_ip4_addr_t subnet_mask)
{
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;

    _fnet_stack_mutex_lock();
    _fnet_netif_set_ip4_addr(netif, ipaddr, subnet_mask);
    _fnet_stack_mutex_unlock();
}
/* Private */
void _fnet_netif_set_ip4_addr(fnet_netif_t *netif, fnet_ip4_addr_t ipaddr, fnet_ip4_addr_t subnet_mask)
{
    if(netif)
    {
        fnet_isr_lock();

        netif->ip4_addr.address = ipaddr; /* IP address */
        netif->ip4_addr.address_type = FNET_NETIF_IP_ADDR_TYPE_MANUAL; /* Adress is set manually. */
        netif->ip4_addr_conflict = FNET_FALSE; /* Clear IPv4 address duplication flag.*/

        if(ipaddr && (subnet_mask == INADDR_ANY))
        {
            /* No subnet mask assigned, so do it automatically.*/
            if(FNET_IP4_CLASS_A(netif->ip4_addr.address))
            {
                if(netif->ip4_addr.subnetmask == 0u)
                {
                    netif->ip4_addr.subnetmask = FNET_IP4_CLASS_A_NET;
                }

                netif->ip4_addr.netmask = FNET_IP4_CLASS_A_NET;
            }
            else
            {
                if(FNET_IP4_CLASS_B(netif->ip4_addr.address))
                {
                    if(netif->ip4_addr.subnetmask == 0u)
                    {
                        netif->ip4_addr.subnetmask = FNET_IP4_CLASS_B_NET;
                    }

                    netif->ip4_addr.netmask = FNET_IP4_CLASS_B_NET;
                }
                else
                {
                    if(FNET_IP4_CLASS_C(netif->ip4_addr.address))
                    {
                        if(netif->ip4_addr.subnetmask == 0u)
                        {
                            netif->ip4_addr.subnetmask = FNET_IP4_CLASS_C_NET;
                        }

                        netif->ip4_addr.netmask = FNET_IP4_CLASS_C_NET;
                    }
                    /* else: Is not supported */
                }
            }

            netif->ip4_addr.net = netif->ip4_addr.address & netif->ip4_addr.netmask;             /* Network address.*/
            netif->ip4_addr.subnet = netif->ip4_addr.address & netif->ip4_addr.subnetmask;       /* Network and subnet address.*/

            netif->ip4_addr.netbroadcast = netif->ip4_addr.address | (~netif->ip4_addr.netmask); /* Network broadcast address.*/
            netif->ip4_addr.subnetbroadcast = netif->ip4_addr.address
                                              | (~netif->ip4_addr.subnetmask);                   /* Subnet broadcast address.*/
        }
        else
        {
            netif->ip4_addr.subnetmask = subnet_mask;
            netif->ip4_addr.subnet = netif->ip4_addr.address & netif->ip4_addr.subnetmask; /* network and subnet address*/
            netif->ip4_addr.subnetbroadcast = netif->ip4_addr.address | (~netif->ip4_addr.subnetmask);     /* subnet broadcast address*/
        }

        /* Seed pseudo-random generator */
        fnet_srand((fnet_uint32_t)ipaddr);

        if(netif->netif_api->netif_change_addr_notify)
        {
            netif->netif_api->netif_change_addr_notify(netif);
        }

        fnet_isr_unlock();
    }
}
#endif /* FNET_CFG_IP4 */

/************************************************************************
* DESCRIPTION: This function sets the gateway IP address.
*************************************************************************/
#if FNET_CFG_IP4
void fnet_netif_set_ip4_gateway( fnet_netif_desc_t netif_desc, fnet_ip4_addr_t gw )
{
    fnet_netif_t *netif = (fnet_netif_t *)netif_desc;

    if(netif)
    {
        _fnet_stack_mutex_lock();
        netif->ip4_addr.gateway = gw;
        _fnet_stack_mutex_unlock();
    }
}
#endif /* FNET_CFG_IP4 */

#if FNET_CFG_DNS && FNET_CFG_IP4
/************************************************************************
* DESCRIPTION: This function sets the DNS IP address.
*************************************************************************/
void fnet_netif_set_ip4_dns( fnet_netif_desc_t netif_desc, fnet_ip4_addr_t dns )
{
    fnet_netif_t *netif = (fnet_netif_t *)netif_desc;

    if(netif)
    {
        _fnet_stack_mutex_lock();
        netif->ip4_addr.dns = dns;
        _fnet_stack_mutex_unlock();
    }
}
#endif /* FNET_CFG_DNS && FNET_CFG_IP4*/

/************************************************************************
* DESCRIPTION: This function gets the default network interface.
*************************************************************************/
fnet_netif_desc_t fnet_netif_get_default( void )
{
    fnet_netif_desc_t     result;

    _fnet_stack_mutex_lock();
    result = (fnet_netif_desc_t)_fnet_netif_get_default();
    _fnet_stack_mutex_unlock();

    return result;
}
/* Private */
fnet_netif_t *_fnet_netif_get_default( void )
{
    return fnet_netif_default;
}

/************************************************************************
* DESCRIPTION: This function returns the IP address of the net interface.
*************************************************************************/
#if FNET_CFG_IP4
fnet_ip4_addr_t fnet_netif_get_ip4_addr( fnet_netif_desc_t netif_desc )
{
    fnet_netif_t        *netif = (fnet_netif_t *)netif_desc;
    fnet_ip4_addr_t     result = INADDR_ANY;

    if(netif)
    {
        _fnet_stack_mutex_lock();
        result = netif->ip4_addr.address;
        _fnet_stack_mutex_unlock();
    }

    return result;
}
#endif /* FNET_CFG_IP4 */

/************************************************************************
* DESCRIPTION: This function returns the netmask of the net interface.
*************************************************************************/
#if FNET_CFG_IP4
fnet_ip4_addr_t fnet_netif_get_ip4_subnet_mask( fnet_netif_desc_t netif_desc )
{
    fnet_netif_t *netif = (fnet_netif_t *)netif_desc;
    fnet_ip4_addr_t     result = INADDR_ANY;

    if(netif)
    {
        _fnet_stack_mutex_lock();
        result = netif->ip4_addr.subnetmask;
        _fnet_stack_mutex_unlock();
    }

    return result;
}
#endif /* FNET_CFG_IP4 */

/************************************************************************
* DESCRIPTION: This function returns the gateway IP address of the net interface.
*************************************************************************/
#if FNET_CFG_IP4
fnet_ip4_addr_t fnet_netif_get_ip4_gateway( fnet_netif_desc_t netif_desc )
{
    fnet_netif_t *netif = (fnet_netif_t *)netif_desc;
    fnet_ip4_addr_t     result = INADDR_ANY;

    if(netif)
    {
        _fnet_stack_mutex_lock();
        result = netif->ip4_addr.gateway;
        _fnet_stack_mutex_unlock();
    }

    return result;
}
#endif /* FNET_CFG_IP4 */

#if FNET_CFG_DNS && FNET_CFG_IP4
/************************************************************************
* DESCRIPTION: This function returns the DNS address of the net interface.
*************************************************************************/
fnet_ip4_addr_t fnet_netif_get_ip4_dns( fnet_netif_desc_t netif_desc )
{
    fnet_netif_t *netif = (fnet_netif_t *)netif_desc;
    fnet_ip4_addr_t     result = INADDR_ANY;

    if(netif)
    {
        _fnet_stack_mutex_lock();
        result = netif->ip4_addr.dns;
        _fnet_stack_mutex_unlock();
    }

    return result;
}
#endif /* FNET_CFG_DNS && FNET_CFG_IP4*/

#if FNET_CFG_DNS && FNET_CFG_IP6
/************************************************************************
* DESCRIPTION: This function returns a DNSv6 Server address.
*************************************************************************/
fnet_bool_t fnet_netif_get_ip6_dns( fnet_netif_desc_t netif_desc, fnet_index_t n, fnet_ip6_addr_t *addr_dns )
{
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;
    fnet_bool_t     result = FNET_FALSE;

    if(netif && addr_dns)
    {
        /* TBD Manual configuration.*/

#if FNET_CFG_ND6_RDNSS
        _fnet_stack_mutex_lock();
        result =  _fnet_nd6_rdnss_get_addr(netif, n, addr_dns);
        _fnet_stack_mutex_unlock();
#endif
    }

    return result;
}
#endif /* FNET_CFG_DNS && FNET_CFG_IP6*/

#if FNET_CFG_IP6
/************************************************************************
* DESCRIPTION: This function returns an entry from IPv6 ND prefix list.
*************************************************************************/
fnet_bool_t fnet_netif_get_ip6_prefix( fnet_netif_desc_t netif_desc, fnet_index_t n, fnet_netif_ip6_prefix_t *ip6_prefix )
{
    fnet_netif_t        *netif = (fnet_netif_t *)netif_desc;
    fnet_bool_t         result = FNET_FALSE;
    struct fnet_nd6_if  *nd6_if;
    fnet_index_t        i;

    if(netif && ip6_prefix)
    {
        _fnet_stack_mutex_lock();
        fnet_isr_lock();

        nd6_if = netif->nd6_if_ptr;

        if(nd6_if )
        {
            for(i = 0u; i < FNET_ND6_PREFIX_LIST_SIZE; i++)
            {
                /* Skip NOT_USED prefixes. */
                if(nd6_if->prefix_list[i].state != FNET_ND6_PREFIX_STATE_NOTUSED)
                {
                    if(n == 0u)
                    {
                        FNET_IP6_ADDR_COPY(&nd6_if->prefix_list[i].prefix, &ip6_prefix->prefix);
                        ip6_prefix->prefix_length = nd6_if->prefix_list[i].prefix_length;

                        result = FNET_TRUE;
                        break;
                    }
                    n--;
                }
            }
        }
        fnet_isr_unlock();
        _fnet_stack_mutex_unlock();
    }

    return result;
}
#endif /* FNET_CFG_IP6*/

#if FNET_CFG_IP6
/************************************************************************
* DESCRIPTION: This function returns an entry from IPv6 neighbor cache.
*************************************************************************/
fnet_bool_t fnet_netif_get_ip6_neighbor_cache(fnet_netif_desc_t netif_desc, fnet_index_t n, fnet_netif_ip6_neighbor_cache_t *ip6_neighbor_cache )
{
    fnet_netif_t        *netif = (fnet_netif_t *)netif_desc;
    fnet_bool_t         result = FNET_FALSE;
    struct fnet_nd6_if  *nd6_if;
    fnet_index_t        i;

    if(netif && ip6_neighbor_cache)
    {
        _fnet_stack_mutex_lock();
        fnet_isr_lock();

        nd6_if = netif->nd6_if_ptr;

        if(nd6_if)
        {
            for(i = 0u; i < FNET_ND6_NEIGHBOR_CACHE_SIZE; i++)
            {
                /* Skip NOT_USED entries. */
                if(nd6_if->neighbor_cache[i].state != FNET_ND6_NEIGHBOR_STATE_NOTUSED)
                {
                    if(n == 0u)
                    {
                        FNET_IP6_ADDR_COPY(&nd6_if->neighbor_cache[i].ip_addr, &ip6_neighbor_cache->ip_addr);
                        FNET_NETIF_LL_ADDR_COPY(nd6_if->neighbor_cache[i].ll_addr, ip6_neighbor_cache->ll_addr, netif->netif_api->netif_hw_addr_size);
                        ip6_neighbor_cache->ll_addr_size = netif->netif_api->netif_hw_addr_size;
                        ip6_neighbor_cache->is_router = nd6_if->neighbor_cache[i].is_router;

                        result = FNET_TRUE;
                        break;
                    }
                    n--;
                }
            }
        }

        fnet_isr_unlock();
        _fnet_stack_mutex_unlock();
    }

    return result;
}
#endif /* FNET_CFG_IP6*/

/************************************************************************
* DESCRIPTION: This function returns network interface name (e.g. "eth0", "loop").
*************************************************************************/
void fnet_netif_get_name( fnet_netif_desc_t netif_desc, fnet_char_t *name, fnet_size_t name_size )
{
    fnet_netif_t *netif = (fnet_netif_t *)netif_desc;

    if(netif && name)
    {
        fnet_strlcpy(name, netif->netif_name, name_size);
    }
}

/************************************************************************
* DESCRIPTION: This function determines if the IPv4 parameters of the @c netif interface
*           were set manually, or obtained by the DHCP client or
*           set during link-local autoconfiguartion (AutoIP - TBD)
*************************************************************************/
#if FNET_CFG_IP4
fnet_netif_ip_addr_type_t fnet_netif_get_ip4_addr_type( fnet_netif_desc_t netif_desc )
{
    fnet_netif_t                *netif = (fnet_netif_t *)netif_desc;
    fnet_netif_ip_addr_type_t   result = FNET_NETIF_IP_ADDR_TYPE_MANUAL;

    if(netif)
    {
        _fnet_stack_mutex_lock();
        result = netif->ip4_addr.address_type;
        _fnet_stack_mutex_unlock();
    }

    return result;
}
#endif

/************************************************************************
* DESCRIPTION: This function sets type of the interface IPv4 address parameters, if they
*       were set manually, or obtained by the DHCP client or
*       set during link-local autoconfiguartion (AutoIP - TBD). @n
*       fnet_netif_set_ip4_addr() sets the type to FNET_NETIF_IP_ADDR_TYPE_MANUAL automatically.
*************************************************************************/
#if FNET_CFG_IP4
void fnet_netif_set_ip4_addr_type( fnet_netif_desc_t netif_desc, fnet_netif_ip_addr_type_t ipaddr_type )
{
    fnet_netif_t *netif = (fnet_netif_t *)netif_desc;

    if(netif)
    {
        _fnet_stack_mutex_lock();
        netif->ip4_addr.address_type = ipaddr_type;
        _fnet_stack_mutex_unlock();
    }
}
#endif /* FNET_CFG_IP4 */

/************************************************************************
* DESCRIPTION: This function reads HW interface address.
*              (MAC address in case of Ethernet interface)
*************************************************************************/
fnet_return_t fnet_netif_get_hw_addr( fnet_netif_desc_t netif_desc, fnet_uint8_t *hw_addr, fnet_size_t hw_addr_size )
{
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;
    fnet_return_t   result;

    _fnet_stack_mutex_lock();
    result = _fnet_netif_get_hw_addr(netif, hw_addr, hw_addr_size);
    _fnet_stack_mutex_unlock();

    return result;
}
/* Private */
fnet_return_t _fnet_netif_get_hw_addr(fnet_netif_t *netif, fnet_uint8_t *hw_addr, fnet_size_t hw_addr_size )
{
    fnet_return_t result = FNET_ERR;

    if(netif && hw_addr && hw_addr_size && (netif->netif_api)
       && (hw_addr_size >= netif->netif_api->netif_hw_addr_size)
       && (netif->netif_api->netif_get_hw_addr))
    {
        result = netif->netif_api->netif_get_hw_addr(netif, hw_addr);
    }

    return result;
}

/************************************************************************
* DESCRIPTION: This function sets HW interface address.
*              (MAC address in case of Ethernet interface)
*************************************************************************/
fnet_return_t fnet_netif_set_hw_addr( fnet_netif_desc_t netif_desc, fnet_uint8_t *hw_addr, fnet_size_t hw_addr_size )
{
    fnet_return_t result;

    _fnet_stack_mutex_lock();
    result = _fnet_netif_set_hw_addr(netif_desc, hw_addr, hw_addr_size);
    _fnet_stack_mutex_unlock();

    return result;
}
/* Private */
fnet_return_t _fnet_netif_set_hw_addr( fnet_netif_desc_t netif_desc, fnet_uint8_t *hw_addr, fnet_size_t hw_addr_size )
{
    fnet_return_t result;
    fnet_netif_t  *netif = (fnet_netif_t *)netif_desc;

    /* Seed pseudo-random generator */
    if(hw_addr_size >= sizeof(fnet_uint32_t))
    {
        fnet_srand(((fnet_uint32_t)hw_addr[0] << 24u) | ((fnet_uint32_t)hw_addr[1] << 16u) | ((fnet_uint32_t)hw_addr[2] << 8u) | hw_addr[3]);
    }

    if(netif && hw_addr
       && (netif->netif_api)
       && (hw_addr_size == netif->netif_api->netif_hw_addr_size)
       && (netif->netif_api->netif_set_hw_addr))
    {
        result = netif->netif_api->netif_set_hw_addr(netif, hw_addr);
    }
    else
    {
        result = FNET_ERR;
    }

    return result;
}

/************************************************************************
* DESCRIPTION: This function configures the HW interface to receive
* particular multicast MAC addresses.
* When the network interface picks up a packet which has a destination MAC
* that matches any of the multicast MAC addresses, it will pass it to
* the upper layers for further processing.
*************************************************************************/
#if FNET_CFG_MULTICAST & FNET_CFG_IP4
void fnet_netif_join_ip4_multicast ( fnet_netif_desc_t netif_desc, fnet_ip4_addr_t multicast_addr )
{
    _fnet_stack_mutex_lock();
    _fnet_netif_join_ip4_multicast(netif_desc, multicast_addr);
    _fnet_stack_mutex_unlock();
}
/* Private */
void _fnet_netif_join_ip4_multicast ( fnet_netif_desc_t netif_desc, fnet_ip4_addr_t multicast_addr )
{
    fnet_netif_t *netif = (fnet_netif_t *)netif_desc;

    if(netif && (netif->netif_api->netif_multicast_join_ip4))
    {
        netif->netif_api->netif_multicast_join_ip4(netif, multicast_addr);
    }
}

#endif /* FNET_CFG_MULTICAST & FNET_CFG_IP4 */
/************************************************************************
* DESCRIPTION:
*************************************************************************/
#if FNET_CFG_MULTICAST & FNET_CFG_IP4
void fnet_netif_leave_ip4_multicast ( fnet_netif_desc_t netif_desc, fnet_ip4_addr_t multicast_addr )
{
    _fnet_stack_mutex_lock();
    _fnet_netif_leave_ip4_multicast(netif_desc, multicast_addr);
    _fnet_stack_mutex_unlock();
}
/* Private */
void _fnet_netif_leave_ip4_multicast ( fnet_netif_desc_t netif_desc, fnet_ip4_addr_t multicast_addr )
{
    fnet_netif_t *netif = (fnet_netif_t *)netif_desc;

    if(netif && (netif->netif_api->netif_multicast_leave_ip4))
    {
        netif->netif_api->netif_multicast_leave_ip4(netif, multicast_addr);
    }
}
#endif /* FNET_CFG_MULTICAST & FNET_CFG_IP4*/

/************************************************************************
* DESCRIPTION: This function returns type of the network interface.
*************************************************************************/
fnet_netif_type_t fnet_netif_get_type( fnet_netif_desc_t netif_desc )
{
    fnet_netif_type_t   result = FNET_NETIF_TYPE_OTHER;
    fnet_netif_t        *netif = (fnet_netif_t *)netif_desc;

    if(netif)
    {
        result = netif->netif_api->netif_type;
    }

    return result;
}

/************************************************************************
* DESCRIPTION: This function gets physical link status.
*************************************************************************/
fnet_bool_t fnet_netif_is_connected( fnet_netif_desc_t netif_desc )
{
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;
    fnet_bool_t     result;

    _fnet_stack_mutex_lock();
    result = _fnet_netif_is_connected(netif);
    _fnet_stack_mutex_unlock();

    return result;
}
/* Private */
fnet_bool_t _fnet_netif_is_connected(fnet_netif_t *netif)
{
    fnet_bool_t     result = FNET_TRUE; /* Is connected by default.*/

    if(netif && (netif->netif_api->netif_is_connected))
    {
        fnet_time_t current_time_ms = fnet_timer_get_ms();

        if((current_time_ms - netif->is_connected_timestamp_ms) > FNET_NETIF_IS_CONNECTED_PERIOD )
        {
            fnet_bool_t     connection_flag = netif->is_connected;

            result = netif->netif_api->netif_is_connected(netif);
            /* Save last state.*/
            netif->is_connected = result;
            netif->is_connected_timestamp_ms = current_time_ms;

            if(result != connection_flag) /* Is any change in connection. */
            {
                if(connection_flag == FNET_FALSE)  /* =>Connected. */
                {
#if FNET_CFG_IP4
                    if(netif->netif_api->netif_change_addr_notify)
                    {
                        netif->netif_api->netif_change_addr_notify(netif); /* Send ARP announcement*/
                    }
#endif
#if FNET_CFG_IP6
                    _fnet_nd6_rd_start(netif); /* Restart IPv6 router discovery */
#endif
                }
            }
        }
        else
        {
            result = netif->is_connected;
        }
    }

    return result;
}

/************************************************************************
* DESCRIPTION: This function returns network interface statistics.
*************************************************************************/
fnet_return_t fnet_netif_get_statistics( fnet_netif_desc_t netif_desc, struct fnet_netif_statistics *statistics )
{
    fnet_return_t   result  = FNET_ERR;
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;

    if(netif && statistics && (netif->netif_api->netif_get_statistics))
    {
        _fnet_stack_mutex_lock();
        fnet_isr_lock();
        result = netif->netif_api->netif_get_statistics(netif, statistics);
        fnet_isr_unlock();
        _fnet_stack_mutex_unlock();
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Registers the "duplicated IP address" event handler.
************************************************************************/
#if FNET_CFG_IP4
void fnet_netif_set_callback_on_ip4_addr_conflict(fnet_netif_callback_ip4_addr_conflict_t callback)
{
    _fnet_stack_mutex_lock();
    fnet_isr_lock();
    fnet_netif_callback_ip4_addr_conflict = callback;
    fnet_isr_unlock();
    _fnet_stack_mutex_unlock();
}
#endif

/************************************************************************
* DESCRIPTION: This function detects if there is IPv4 adress conflict on network.
*************************************************************************/
#if FNET_CFG_IP4
fnet_bool_t fnet_netif_is_ip4_addr_conflict( fnet_netif_desc_t netif_desc )
{
    fnet_bool_t     result = FNET_FALSE;
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;

    if(netif)
    {
        _fnet_stack_mutex_lock();
        fnet_isr_lock();
        result = netif->ip4_addr_conflict;
        fnet_isr_unlock();
        _fnet_stack_mutex_unlock();
    }

    return result;
}
#endif

/************************************************************************
* DESCRIPTION: This function clears IPv4 adress conflict flag.
*************************************************************************/
#if FNET_CFG_IP4
void fnet_netif_clear_ip4_addr_conflict( fnet_netif_desc_t netif_desc )
{
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;

    if(netif)
    {
        _fnet_stack_mutex_lock();
        fnet_isr_lock();
        netif->ip4_addr_conflict = FNET_FALSE;
        fnet_isr_unlock();
        _fnet_stack_mutex_unlock();
    }
}
#endif

/************************************************************************
* DESCRIPTION:
************************************************************************/
#if FNET_CFG_IP4
void _fnet_netif_signal_ip4_addr_conflict(fnet_netif_desc_t netif )
{
    if(fnet_netif_callback_ip4_addr_conflict)
    {
        fnet_netif_callback_ip4_addr_conflict(netif);
    }
}
#endif

/************************************************************************
* DESCRIPTION: This function assign unique Scope ID to the interface.
*************************************************************************/
static void _fnet_netif_assign_scope_id( fnet_netif_t *netif )
{
    fnet_scope_id_t scope_id;
    fnet_netif_t    *current;
    fnet_bool_t     try_again;

    scope_id = 1u;

    do
    {
        try_again = FNET_FALSE;

        for (current = fnet_netif_list; current; current = current->next)
        {
            if(scope_id == current->scope_id)
            {
                try_again = FNET_TRUE;
                scope_id++;
                break;
            }
        }
    }
    while(try_again == FNET_TRUE);

    netif->scope_id = scope_id;
}

/************************************************************************
* DESCRIPTION: Gets Scope ID assigned to the interface.
*************************************************************************/
fnet_scope_id_t fnet_netif_get_scope_id(fnet_netif_desc_t netif_desc)
{
    fnet_scope_id_t result = 0u;
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;

    if(netif)
    {
        _fnet_stack_mutex_lock();
        result = netif->scope_id;
        _fnet_stack_mutex_unlock();
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Gets Maximum Transmission Unit (MTU) of the interface.
*************************************************************************/
fnet_size_t fnet_netif_get_mtu(fnet_netif_desc_t netif_desc)
{
    fnet_size_t     mtu = 0u;
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;

    if(netif)
    {
        _fnet_stack_mutex_lock();
        mtu = netif->netif_mtu;
        _fnet_stack_mutex_unlock();
    }

    return mtu;
}

/************************************************************************
* DESCRIPTION: This function returns interface decriptor according
*              its Scope ID.
*              It returns FNET_NULL if the interface with passed Scope ID
*              is not available.
*************************************************************************/
fnet_netif_desc_t fnet_netif_get_by_scope_id( fnet_scope_id_t scope_id )
{
    fnet_netif_desc_t result;

    _fnet_stack_mutex_lock();
    result = (fnet_netif_desc_t)_fnet_netif_get_by_scope_id(scope_id);
    _fnet_stack_mutex_unlock();

    return result;
}
/* Private */
fnet_netif_desc_t _fnet_netif_get_by_scope_id( fnet_scope_id_t scope_id )
{
    fnet_netif_t    *result = FNET_NULL;
    fnet_netif_t    *current;

    if(scope_id)
    {
        for (current = fnet_netif_list; current; current = current->next)
        {
            if(current->scope_id == scope_id)
            {
                result = current;
                break;
            }
        }
    }

    return result;
}


/************************************************************************
********************   IP6 Netif API ************************************
*************************************************************************/
#if FNET_CFG_IP6

/************************************************************************
* DESCRIPTION: This function is used to retrieve all IP addresses registerred
*              with the given interface.
*              Returns FNET_TRUE if successful and data structure filled
*              and FNET_FALSE in case of error.
*              It returns FNET_FALSE if n-th address is not available.
*************************************************************************/
fnet_bool_t fnet_netif_get_ip6_addr (fnet_netif_desc_t netif_desc, fnet_index_t n, fnet_netif_ip6_addr_info_t *addr_info)
{
    fnet_bool_t     result = FNET_FALSE;
    fnet_index_t    i;
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;

    if(netif && addr_info)
    {
        _fnet_stack_mutex_lock();
        fnet_isr_lock();
        for(i = 0u; i < FNET_NETIF_IP6_ADDR_MAX; i++)
        {
            /* Skip NOT_USED addresses. */
            if(netif->ip6_addr[i].state != FNET_NETIF_IP6_ADDR_STATE_NOT_USED)
            {
                if(n == 0u)
                {
                    FNET_IP6_ADDR_COPY(&netif->ip6_addr[i].address, &addr_info->address);   /* IPv6 address.*/
                    addr_info->state = netif->ip6_addr[i].state;                            /* Address current state.*/
                    addr_info->type = netif->ip6_addr[i].type;                              /* How the address was acquired.*/

                    result = FNET_TRUE;
                    break;
                }
                n--;
            }
        }
        fnet_isr_unlock();
        _fnet_stack_mutex_unlock();
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Joins the specified network interface to IPv6 multicast group.
*************************************************************************/
void fnet_netif_join_ip6_multicast ( fnet_netif_desc_t netif_desc, const fnet_ip6_addr_t *multicast_addr )
{
    _fnet_stack_mutex_lock();
    _fnet_netif_join_ip6_multicast (netif_desc, multicast_addr);
    _fnet_stack_mutex_unlock();
}
/* Private */
void _fnet_netif_join_ip6_multicast ( fnet_netif_desc_t netif_desc, const fnet_ip6_addr_t *multicast_addr )
{
    fnet_netif_t *netif = (fnet_netif_t *)netif_desc;

    if(netif && (netif->netif_api->netif_multicast_join_ip6))
    {
        netif->netif_api->netif_multicast_join_ip6(netif, multicast_addr);
    }
}

/************************************************************************
* DESCRIPTION: Leaves the specified network interface from IPv6 multicast group.
*************************************************************************/
void fnet_netif_leave_ip6_multicast ( fnet_netif_desc_t netif_desc, fnet_ip6_addr_t *multicast_addr )
{
    _fnet_stack_mutex_lock();
    _fnet_netif_leave_ip6_multicast(netif_desc, multicast_addr);
    _fnet_stack_mutex_unlock();
}
/* Private */
void _fnet_netif_leave_ip6_multicast ( fnet_netif_desc_t netif_desc, fnet_ip6_addr_t *multicast_addr )
{
    fnet_netif_t *netif = (fnet_netif_t *)netif_desc;

    if(netif && (netif->netif_api->netif_multicast_leave_ip6))
    {
        netif->netif_api->netif_multicast_leave_ip6(netif, multicast_addr);
    }
}

/************************************************************************
* DESCRIPTION: Gets address information structure that corresponds to the ip_addr.
*************************************************************************/
fnet_netif_ip6_addr_t *_fnet_netif_get_ip6_addr_info(fnet_netif_t *netif, const fnet_ip6_addr_t *ip_addr)
{
    fnet_index_t            i;
    fnet_netif_ip6_addr_t   *result = FNET_NULL;

    if(netif && ip_addr)
    {
        for(i = 0u; i < FNET_NETIF_IP6_ADDR_MAX; i++)
        {
            /* Skip NOT_USED addresses. */
            if((netif->ip6_addr[i].state != FNET_NETIF_IP6_ADDR_STATE_NOT_USED)
               && FNET_IP6_ADDR_EQUAL(ip_addr, &netif->ip6_addr[i].address))
            {
                result = &netif->ip6_addr[i];
                break;
            }
        }
    }

    return result;
}

/************************************************************************
* DESCRIPTION: This function returns valid link-local address,
*  if there is no vlaid address(DAD is in progress) it returns FNET_NULL.
*************************************************************************/
fnet_ip6_addr_t *_fnet_netif_get_ip6_addr_valid_link_local (fnet_netif_t *netif)
{
    fnet_ip6_addr_t     *result = FNET_NULL;
    fnet_index_t        i;

    if(netif)
    {
        for(i = 0u; i < FNET_NETIF_IP6_ADDR_MAX; i++)
        {
            /* Skip NOT_USED and TENTATIVE addresses. */
            if((netif->ip6_addr[i].state != FNET_NETIF_IP6_ADDR_STATE_NOT_USED)
               && (netif->ip6_addr[i].state != FNET_NETIF_IP6_ADDR_STATE_TENTATIVE)
               && FNET_IP6_ADDR_IS_LINKLOCAL(&netif->ip6_addr[i].address))
            {
                result = &netif->ip6_addr[i].address;
                break;
            }
        }
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Checks if an unicast address is attached/bound to the interface.
*              Returns FNET_TRUE if the address is attached/bound, otherwise FNET_FALSE.
*************************************************************************/
fnet_bool_t _fnet_netif_is_my_ip6_addr(fnet_netif_t *netif, const fnet_ip6_addr_t *ip_addr)
{
    fnet_bool_t     result;

    if(_fnet_netif_get_ip6_addr_info(netif, ip_addr) != FNET_NULL)
    {
        result = FNET_TRUE;
    }
    else
    {
        result = FNET_FALSE;
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Checks if a solicited multicast address is attached/bound
*              to the interface.
*              Returns FNET_TRUE if the solicited multicast address
*              is attached/bound, otherwise FNET_FALSE.
*************************************************************************/
fnet_bool_t _fnet_netif_is_my_ip6_solicited_multicast_addr(fnet_netif_t *netif, fnet_ip6_addr_t *ip_addr)
{
    fnet_index_t    i;
    fnet_bool_t     result = FNET_FALSE;

    if(netif && ip_addr)
    {
        for(i = 0u; i < FNET_NETIF_IP6_ADDR_MAX; i++)
        {
            /* Skip NOT_USED addresses. */
            if((netif->ip6_addr[i].state  != FNET_NETIF_IP6_ADDR_STATE_NOT_USED) &&
               FNET_IP6_ADDR_EQUAL(ip_addr, &netif->ip6_addr[i].solicited_multicast_addr))
            {
                result = FNET_TRUE;
                break;
            }
        }
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Returns a network interface given its IPv6 address.
*************************************************************************/
fnet_netif_desc_t fnet_netif_get_by_ip6_addr(const fnet_ip6_addr_t *ip_addr )
{
    fnet_netif_desc_t result;

    _fnet_stack_mutex_lock();
    result = (fnet_netif_desc_t)_fnet_netif_get_by_ip6_addr(ip_addr);
    _fnet_stack_mutex_unlock();

    return result;
}
/* Private */
fnet_netif_t *_fnet_netif_get_by_ip6_addr(const fnet_ip6_addr_t *ip_addr )
{
    fnet_netif_t        *netif;
    fnet_netif_t        *result = FNET_NULL;

    /* If the source address is explicitly specified by the user the
     * specified address is used.*/
    if(ip_addr)
    {
        for (netif = fnet_netif_list; netif != 0; netif = netif->next)
        {
            if(_fnet_netif_is_my_ip6_addr(netif, ip_addr) == FNET_TRUE)
            {
                result = netif;
                break;
            }
        }
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Overwrite the last 64 bits with the interface ID.
*              "addr" contains prefix to form an address
*************************************************************************/
fnet_return_t _fnet_netif_set_ip6_addr_autoconf(fnet_netif_t *netif, fnet_ip6_addr_t *ip_addr)
{
    fnet_return_t result;
    fnet_uint8_t hw_addr[8];

    result = _fnet_netif_get_hw_addr(netif, hw_addr, 8u );

    if(result == FNET_OK)
    {
        /* Build Interface identifier.*/
        /* Set the 8 last bytes of the IP address based on the Layer 2 identifier.*/
        switch(netif->netif_api->netif_hw_addr_size)
        {
            case 6: /* IEEE 48-bit MAC addresses. */
                fnet_memcpy(&(ip_addr->addr[8]), hw_addr,  3u);
                ip_addr->addr[11] = 0xffu;
                ip_addr->addr[12] = 0xfeu;
                fnet_memcpy(&(ip_addr->addr[13]), &hw_addr[3],  3u);
                ip_addr->addr[8] ^= 0x02u;
                break;
            case 8: /* IEEE EUI-64 identifier.*/
                fnet_memcpy(&(ip_addr->addr[8]), hw_addr,  8u);
                ip_addr->addr[8] ^= 0x02u;
                break;
            /* TBD for others.*/
            default:
                result = FNET_ERR;
                break;
        }
    }

    return result;
}

/************************************************************************
* DESCRIPTION: This function binds the IPv6 address to a network interface.
*************************************************************************/
fnet_return_t fnet_netif_bind_ip6_addr(fnet_netif_desc_t netif_desc, const fnet_ip6_addr_t *addr, fnet_netif_ip_addr_type_t addr_type)
{
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;
    fnet_return_t   result;

    _fnet_stack_mutex_lock();
    fnet_isr_lock();
    result = _fnet_netif_bind_ip6_addr(netif, addr, addr_type,
                                       FNET_NETIF_IP6_ADDR_LIFETIME_INFINITE,
                                       FNET_ND6_PREFIX_LENGTH_DEFAULT);
    fnet_isr_unlock();
    _fnet_stack_mutex_unlock();

    return result;
}
/* Private */
fnet_return_t _fnet_netif_bind_ip6_addr(fnet_netif_t *netif, const fnet_ip6_addr_t *addr, fnet_netif_ip_addr_type_t addr_type,
                                        fnet_time_t lifetime_sec /*in seconds*/, fnet_size_t prefix_length /* bits */ )
{
    fnet_return_t           result = FNET_ERR;
    fnet_netif_ip6_addr_t   *if_addr_ptr = FNET_NULL;
    fnet_index_t            i;

    /* Check input parameters. */
    if(netif && addr && (!FNET_IP6_ADDR_IS_MULTICAST(addr)))
    {
        /* Find free address entry.
         */
        for(i = 0u; i < FNET_NETIF_IP6_ADDR_MAX; i++)
        {
            if(netif->ip6_addr[i].state == FNET_NETIF_IP6_ADDR_STATE_NOT_USED)
            {
                if_addr_ptr = &netif->ip6_addr[i];
                break; /* Found free entry.*/
            }
        }

        if(if_addr_ptr)
        {
            /* Copying address. */
            FNET_IP6_ADDR_COPY(addr, &if_addr_ptr->address);

            /* If the address is zero => make it link-local.*/
            if(FNET_IP6_ADDR_EQUAL(&if_addr_ptr->address, &fnet_ip6_addr_any))
            {
                /* Set link-local address. */
                if_addr_ptr->address.addr[0] = 0xFEu;
                if_addr_ptr->address.addr[1] = 0x80u;
            }

            if_addr_ptr->type = addr_type; /* Set type.*/

            /* If we are doing Autoconfiguration, the ip_addr points to prefix.*/
            if(addr_type == FNET_NETIF_IP_ADDR_TYPE_AUTOCONFIGURABLE)
            {
                /* Construct address from prefix and interface id. */
                if((prefix_length != FNET_ND6_PREFIX_LENGTH_DEFAULT)
                   || (_fnet_netif_set_ip6_addr_autoconf(netif, &if_addr_ptr->address) == FNET_ERR))
                {
                    goto COMPLETE;
                }
            }

            /* Check if addr already exists. Do it here to cover Autoconfiguration case.*/
            if(_fnet_netif_get_ip6_addr_info(netif, &if_addr_ptr->address) != FNET_NULL)
            {
                /* The address is already bound.*/
                result = FNET_OK;
                goto COMPLETE;
            }

            /* Save creation time, in seconds.*/
            if_addr_ptr->creation_time_sec = fnet_timer_get_ms() / FNET_TIMER_MS_IN_SEC;

            /* Set lifetime, in seconds.*/
            if_addr_ptr->lifetime_sec = lifetime_sec;

            /* If supports ND6. */
            if(netif->nd6_if_ptr)
            {
                /* An address on which the Duplicate Address Detection procedure is
                 * applied is said to be tentative until the procedure has completed
                 * successfully.*/
                if_addr_ptr->state = FNET_NETIF_IP6_ADDR_STATE_TENTATIVE;

                /* Get&Set the solicited-node multicast group-address for assigned ip_addr. */
                _fnet_ip6_get_solicited_multicast_addr(&if_addr_ptr->address, &if_addr_ptr->solicited_multicast_addr);

                /*************************************************************************
                * Join Multicast ADDRESSES.
                * When a multicast-capable interface becomes enabled, the node MUST
                * join the all-nodes multicast address on that interface, as well as
                * the solicited-node multicast address corresponding to each of the IP
                * addresses assigned to the interface.
                **************************************************************************/
                /* Join solicited multicast address group.*/
                _fnet_ip6_multicast_join(netif, &if_addr_ptr->solicited_multicast_addr);

                /* Start Duplicate Address Detection (DAD).
                 * RFC4862:  The Duplicate Address Detection algorithm is performed on all addresses,
                 * independently of whether they are obtained via stateless
                 * autoconfiguration or DHCPv6.
                 */
                _fnet_nd6_dad_start(netif, if_addr_ptr);
            }
            else
            {
                if_addr_ptr->state = FNET_NETIF_IP6_ADDR_STATE_PREFERRED;
            }
            result = FNET_OK;
        }
    }

COMPLETE:

    return result;
}

/************************************************************************
* DESCRIPTION: Unbinds an IPV6 address from a hardware interface.
*************************************************************************/
fnet_return_t fnet_netif_unbind_ip6_addr(fnet_netif_desc_t netif_desc, const fnet_ip6_addr_t *addr )
{
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;
    fnet_return_t   result;
    fnet_netif_ip6_addr_t   *if_addr;

    _fnet_stack_mutex_lock();
    fnet_isr_lock();

    if_addr = _fnet_netif_get_ip6_addr_info(netif, addr);
    result = _fnet_netif_unbind_ip6_addr(netif, if_addr);

    fnet_isr_unlock();
    _fnet_stack_mutex_unlock();

    return result;
}
/* Private */
fnet_return_t _fnet_netif_unbind_ip6_addr( fnet_netif_t *netif, fnet_netif_ip6_addr_t *if_addr )
{
    fnet_return_t result;

    if(netif && if_addr && (if_addr->state != FNET_NETIF_IP6_ADDR_STATE_NOT_USED))
    {
        /* Leave Multicast group.*/
        _fnet_ip6_multicast_leave(netif, &if_addr->solicited_multicast_addr);

        /* Mark as Not Used.*/
        if_addr->state = FNET_NETIF_IP6_ADDR_STATE_NOT_USED;
        result = FNET_OK;
    }
    else
    {
        result = FNET_ERR;
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Timer that checks expiring of bound addresses.
*************************************************************************/
void _fnet_netif_ip6_addr_timer( fnet_netif_t *netif)
{
    fnet_index_t i;

    for(i = 0u; i < FNET_NETIF_IP6_ADDR_MAX; i++)
    {
        /* Check lifetime for address.*/
        if((netif->ip6_addr[i].state != FNET_NETIF_IP6_ADDR_STATE_NOT_USED)
           && (netif->ip6_addr[i].lifetime_sec != FNET_NETIF_IP6_ADDR_LIFETIME_INFINITE)
           && (((fnet_timer_get_ms() / FNET_TIMER_MS_IN_SEC) - netif->ip6_addr[i].creation_time_sec) > netif->ip6_addr[i].lifetime_sec)
          )
        {
            /* RFC4862 5.5.4: An address (and its association with an interface) becomes invalid
             * when its valid lifetime expires. An invalid address MUST NOT be used
             * as a source address in outgoing communications and MUST NOT be
             * recognized as a destination on a receiving interface.*/

            _fnet_netif_unbind_ip6_addr(netif, &netif->ip6_addr[i]);
        }
    }
}

#endif /* FNET_CFG_IP6 */
