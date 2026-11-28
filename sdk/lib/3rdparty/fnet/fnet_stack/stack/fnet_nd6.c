/**************************************************************************
*
* Copyright 2008-2018 by Andrey Butok. FNET Community
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
*  IPv6 Neighbor Discovery implementation.
*
***************************************************************************/

#include "fnet.h"

#if FNET_CFG_IP6
#include "fnet_nd6.h"
#include "fnet_icmp6.h"
#include "fnet_netif_prv.h"
#include "fnet_ip6_prv.h"
#include "fnet_mld.h"

/* Check minimum values.*/
#if FNET_CFG_ND6_ROUTER_LIST_SIZE < 1u
    #error "FNET_CFG_ND6_ROUTER_LIST_SIZE must be > 0"
#endif
#if FNET_CFG_ND6_NEIGHBOR_CACHE_SIZE  < 1u
    #error "FNET_CFG_ND6_NEIGHBOR_CACHE_SIZE must be > 0"
#endif
#if FNET_CFG_ND6_PREFIX_LIST_SIZE < 1u
    #error "FNET_CFG_ND6_PREFIX_LIST_SIZE must be > 0"
#endif

/* Control FNET_CFG_ND6_RDNSS_LIST_SIZE minimum value.*/
#if FNET_CFG_ND6_RDNSS_LIST_SIZE < 1u
    #error "FNET_CFG_ND6_RDNSS_LIST_SIZE must be > 0"
#endif

static void _fnet_nd6_timer( fnet_uint32_t cookie );
static void _fnet_nd6_dad_timer( fnet_netif_t *netif );
static void _fnet_nd6_dad_failed(fnet_netif_t *netif, fnet_netif_ip6_addr_t *addr_info);
static void _fnet_nd6_rd_timer(fnet_netif_t *netif);
static void _fnet_nd6_prefix_timer(fnet_netif_t *netif);
static void _fnet_nd6_neighbor_cache_timer(fnet_netif_t *netif);
static void _fnet_nd6_redirect_table_del(fnet_netif_t *if_ptr, const fnet_ip6_addr_t *target_addr);
static fnet_nd6_redirect_entry_t *_fnet_nd6_redirect_table_add(fnet_netif_t *if_ptr, const fnet_ip6_addr_t *destination_addr, const fnet_ip6_addr_t *target_addr);
static fnet_bool_t _fnet_nd6_is_firsthop_router(fnet_netif_t *netif, fnet_ip6_addr_t *router_ip);
static void _fnet_nd6_neighbor_send_waiting_netbuf(struct fnet_netif *netif, fnet_nd6_neighbor_entry_t *neighbor_entry);
static void _fnet_nd6_router_list_add( fnet_nd6_neighbor_entry_t *neighbor_entry, fnet_time_t lifetime_sec );
static void _fnet_nd6_router_list_del( fnet_nd6_neighbor_entry_t *neighbor_entry );
static fnet_nd6_prefix_entry_t *_fnet_nd6_prefix_list_get(struct fnet_netif *netif, fnet_ip6_addr_t *prefix);
static void _fnet_nd6_prefix_list_del(fnet_nd6_prefix_entry_t *prefix_entry);
static fnet_nd6_prefix_entry_t *_fnet_nd6_prefix_list_add(struct fnet_netif *if_ptr, const fnet_ip6_addr_t *prefix, fnet_size_t prefix_length, fnet_time_t lifetime_sec);
static void _fnet_nd6_neighbor_advertisement_send(struct fnet_netif *netif, const fnet_ip6_addr_t *ipsrc, const fnet_ip6_addr_t *ipdest, fnet_uint8_t na_flags);
static void _fnet_nd6_router_solicitation_send(struct fnet_netif *netif);

#if FNET_CFG_ND6_RDNSS && FNET_CFG_DNS
    static void _fnet_nd6_rdnss_list_update(fnet_netif_t *if_ptr, const fnet_ip6_addr_t *rdnss_addr, fnet_time_t lifetime_sec);
    static void _fnet_nd6_rdnss_list_del(fnet_nd6_rdnss_entry_t *rdnss_entry);
    static void _fnet_nd6_rdnss_timer(fnet_netif_t *netif);
#endif

/************************************************************************
* DESCRIPTION: Initializes the Neighbor Disscovery on an interface.
*************************************************************************/
fnet_return_t _fnet_nd6_init (struct fnet_netif *netif, fnet_nd6_if_t *nd6_if_ptr)
{
    fnet_return_t result = FNET_ERR;

    if(netif && nd6_if_ptr)
    {
        netif->nd6_if_ptr = nd6_if_ptr;

        /* Clear all parameters.*/
        fnet_memset_zero(nd6_if_ptr, sizeof(fnet_nd6_if_t));

        /* --- Initialize Prefix List. ----*/

        /* The link-local prefix is considered to be on the
         * prefix list with an infinite invalidation timer
         * regardless of whether routers are advertising a
         * prefix for it.
         */
        _fnet_nd6_prefix_list_add(netif, &fnet_ip6_addr_linklocal_prefix, FNET_ND6_PREFIX_LENGTH_DEFAULT, FNET_ND6_PREFIX_LIFETIME_INFINITE);

        /* ---- Initialize interface variables. ----*/
        /* The recommended MTU for the link. */
        nd6_if_ptr->mtu = netif->netif_mtu;
        if(nd6_if_ptr->mtu < FNET_IP6_DEFAULT_MTU)
        {
            nd6_if_ptr->mtu = FNET_IP6_DEFAULT_MTU;
        }

        /* Cur Hop Limit value */
        nd6_if_ptr->cur_hop_limit = FNET_IP6_HOPLIMIT_DEFAULT;

        /* Reachable time. used for NU.*/
        nd6_if_ptr->reachable_time_ms = FNET_ND6_REACHABLE_TIME_MS;

        /* Time between retransmissions of Neighbor Solicitation messages.*/
        nd6_if_ptr->retrans_timer_ms = FNET_ND6_RETRANS_TIMER_MS;

        /* --- Register timer to check ND lists and N cache. ---*/
        nd6_if_ptr->timer = _fnet_timer_new(FNET_ND6_TIMER_PERIOD_MS + 100u, _fnet_nd6_timer, (fnet_uint32_t)netif);

        if(nd6_if_ptr->timer != FNET_NULL)
        {
            result = FNET_OK;
        }
    }
    return result;
}

/************************************************************************
* DESCRIPTION: Release the Neighbor Disscovery for the interface.
*************************************************************************/
void _fnet_nd6_release (struct fnet_netif *netif)
{
    if(netif && (netif->nd6_if_ptr))
    {
        _fnet_timer_free(netif->nd6_if_ptr->timer);
        netif->nd6_if_ptr = 0;
    }
}

/************************************************************************
* DESCRIPTION: ND6 timer.
*************************************************************************/
static void _fnet_nd6_timer( fnet_uint32_t cookie )
{
    fnet_netif_t    *netif = (fnet_netif_t *)cookie;

    /* DAD timer.*/
    _fnet_nd6_dad_timer(netif);

    /* Neighbor Cache processing.*/
    _fnet_nd6_neighbor_cache_timer(netif);

    /* Check prefix lifetime.*/
    _fnet_nd6_prefix_timer(netif);

    /* Check address lifetime.*/
    _fnet_netif_ip6_addr_timer(netif);

    /* RD timer. */
    _fnet_nd6_rd_timer(netif);

#if FNET_CFG_ND6_RDNSS && FNET_CFG_DNS
    /* RDNSS timer. */
    _fnet_nd6_rdnss_timer(netif);
#endif
}

/************************************************************************
* DESCRIPTION: Get entry from Neighbor cache that corresponds ip_addr.
*               It returns FNET_NULL if no entry is found.
*************************************************************************/
fnet_nd6_neighbor_entry_t *_fnet_nd6_neighbor_cache_get(struct fnet_netif *netif, const fnet_ip6_addr_t *ip_addr)
{
    fnet_nd6_if_t               *nd6_if = netif->nd6_if_ptr;
    fnet_index_t                i;
    fnet_nd6_neighbor_entry_t   *result = FNET_NULL;

    if (nd6_if)
    {
        /* Find the entry in the cache. */
        for(i = 0u; i < FNET_ND6_NEIGHBOR_CACHE_SIZE; i++)
        {
            if( (nd6_if->neighbor_cache[i].state != FNET_ND6_NEIGHBOR_STATE_NOTUSED) &&
                FNET_IP6_ADDR_EQUAL(&nd6_if->neighbor_cache[i].ip_addr, ip_addr))
            {
                result = &nd6_if->neighbor_cache[i];
                break;
            }
        }
    }
    return result;
}

/************************************************************************
* DESCRIPTION: Deletes an entry from the Neighbor Cache.
*************************************************************************/
void _fnet_nd6_neighbor_cache_del(struct fnet_netif *netif, fnet_nd6_neighbor_entry_t *neighbor_entry)
{
    if (neighbor_entry)
    {
        /* Delete posible entry in the Redirect Table.*/
        _fnet_nd6_redirect_table_del(netif, &neighbor_entry->ip_addr);

        neighbor_entry->state = FNET_ND6_NEIGHBOR_STATE_NOTUSED;

        /* Free waiting queue.*/
        if(neighbor_entry->waiting_netbuf) /* TBD for bigger queue.*/
        {
            _fnet_netbuf_free_chain(neighbor_entry->waiting_netbuf);
        }
    }
}

/************************************************************************
* DESCRIPTION: Adds (TBD update) entry into the Neighbor cache.
*************************************************************************/
fnet_nd6_neighbor_entry_t *_fnet_nd6_neighbor_cache_add(struct fnet_netif *netif, const fnet_ip6_addr_t *ip_addr, fnet_netif_ll_addr_t ll_addr, fnet_nd6_neighbor_state_t state)
{
    fnet_nd6_if_t               *nd6_if = netif->nd6_if_ptr;
    fnet_index_t                i;
    fnet_nd6_neighbor_entry_t   *entry = FNET_NULL;

    if (nd6_if)
    {
        /* Find not used entry.*/
        for(i = 0u; i < FNET_ND6_NEIGHBOR_CACHE_SIZE; i++)
        {
            if( (nd6_if->neighbor_cache[i].state == FNET_ND6_NEIGHBOR_STATE_NOTUSED))
            {
                entry = &nd6_if->neighbor_cache[i];
                break;
            }
        }

        /* If no free entry is found.*/
        if(entry == FNET_NULL)
        {
            entry = &nd6_if->neighbor_cache[0];
            /* Try to find the oldest entry.*/
            for(i = 0u; i < FNET_ND6_NEIGHBOR_CACHE_SIZE; i++)
            {
                if(nd6_if->neighbor_cache[i].creation_time_sec < entry->creation_time_sec)
                {
                    entry = &nd6_if->neighbor_cache[i];
                }
            }
        }

        /* Fill the informationn.*/

        /* Clear enty structure.*/
        fnet_memset_zero(entry, sizeof(fnet_nd6_neighbor_entry_t));

        FNET_IP6_ADDR_COPY(ip_addr, &entry->ip_addr);
        if(ll_addr != FNET_NULL)
        {
            FNET_NETIF_LL_ADDR_COPY(ll_addr, entry->ll_addr, netif->netif_api->netif_hw_addr_size);
        }
        entry->creation_time_sec = fnet_timer_get_ms() / FNET_TIMER_MS_IN_SEC;
        entry->is_router = FNET_FALSE;
        entry->router_lifetime_sec = 0u;
        entry->state = state;

        /* TBD Init timers reachable; last send*/
    }
    return entry;
}

/************************************************************************
* DESCRIPTION: Timer routine used for Neighbor Cache check.
*************************************************************************/
static void _fnet_nd6_neighbor_cache_timer(fnet_netif_t *netif)
{
    fnet_nd6_if_t               *nd6_if = netif->nd6_if_ptr;
    fnet_index_t                i;
    fnet_nd6_neighbor_entry_t   *neighbor_entry;

    /* Neighbor Cache processing.*/
    for(i = 0u; i < FNET_ND6_NEIGHBOR_CACHE_SIZE; i++)
    {
        neighbor_entry = &nd6_if->neighbor_cache[i];

        /* Check router expiration */
        if((neighbor_entry->state != FNET_ND6_NEIGHBOR_STATE_NOTUSED)
           && (neighbor_entry->is_router == FNET_TRUE)
           && (((fnet_timer_get_ms() / FNET_TIMER_MS_IN_SEC) - neighbor_entry->creation_time_sec) > neighbor_entry->router_lifetime_sec)
          )
        {
            if(neighbor_entry->router_lifetime_sec)
            {
                _fnet_nd6_router_list_del(neighbor_entry);
            }
        }

        switch(neighbor_entry->state)
        {
            /* =========== INCOMPLETE ============= */
            case FNET_ND6_NEIGHBOR_STATE_INCOMPLETE:

                if((fnet_timer_get_ms() - neighbor_entry->state_time_ms) > nd6_if->retrans_timer_ms )
                {
                    neighbor_entry->solicitation_send_counter++;
                    if(neighbor_entry->solicitation_send_counter >= FNET_ND6_MAX_MULTICAST_SOLICIT)
                        /* AR is failed.*/
                    {
                        /* RFC4861 7.3.3: If address resolution fails, the entry SHOULD be deleted, so that subsequent
                         * traffic to that neighbor invokes the next-hop determination procedure
                         * again. Invoking next-hop determination at this point ensures that
                         * alternate default routers are tried.
                         */
                        _fnet_nd6_neighbor_cache_del(netif, neighbor_entry);
                        /* TBD ICMP error to upper layer?*/
                    }
                    else
                        /* AR: transmit NS.*/
                    {
                        fnet_ip6_addr_t *dest_addr = &neighbor_entry->ip_addr;

                        /* Select source address.*/ /* PFI*/
                        fnet_ip6_addr_t *src_addr = &neighbor_entry->solicitation_src_ip_addr;

                        neighbor_entry->state_time_ms = fnet_timer_get_ms();
                        /* AR: Transmitting a Neighbor Solicitation message targeted at the neighbor.*/
                        _fnet_nd6_neighbor_solicitation_send(netif, src_addr, FNET_NULL /* FNET_NULL for AR */, dest_addr);
                    }
                }
                break;
            /* =========== REACHABLE ============= */
            case FNET_ND6_NEIGHBOR_STATE_REACHABLE:
                /*
                 * RFC 4861: When ReachableTime milliseconds have passed since receipt of the last
                 * reachability confirmation for a neighbor, the Neighbor Cache entry’s
                 * state changes from REACHABLE to STALE.
                 */
                if((fnet_timer_get_ms() - neighbor_entry->state_time_ms) > nd6_if->reachable_time_ms )
                    /* REACHABLE to STALE */
                {
                    neighbor_entry->state = FNET_ND6_NEIGHBOR_STATE_STALE;
                }
                break;
            /* =========== DELAY ================ */
            case FNET_ND6_NEIGHBOR_STATE_DELAY:
                /*
                * If the entry is still in the DELAY state when the timer expires,
                * the entry’s state changes to PROBE.
                */
                if((fnet_timer_get_ms() - neighbor_entry->state_time_ms) > FNET_ND6_DELAY_FIRST_PROBE_TIME_MS )
                    /* DELAY to PROBE */
                {
                    const fnet_ip6_addr_t   *src_addr;
                    fnet_ip6_addr_t         *dest_addr = &neighbor_entry->ip_addr;


                    neighbor_entry->state = FNET_ND6_NEIGHBOR_STATE_PROBE;
                    neighbor_entry->solicitation_send_counter = 0u;
                    /*
                     * Upon entering the PROBE state, a node sends a unicast Neighbor
                     * Solicitation message to the neighbor using the cached link-layer
                     * address.
                     */

                    neighbor_entry->state_time_ms = fnet_timer_get_ms();

                    /* Select source address.*/ /*PFI*/
                    src_addr = _fnet_ip6_select_src_addr(netif, dest_addr);

                    if(src_addr)
                    {
                        _fnet_nd6_neighbor_solicitation_send(netif, src_addr, dest_addr/*Unicast*/, dest_addr);
                    }
                }
                break;
            /* =========== PROBE ================ */
            case FNET_ND6_NEIGHBOR_STATE_PROBE:  /*PFI the same as INCOMPLETE*/
                /* RFC4861 7.3.3:
                 * While in the PROBE state, a node retransmits Neighbor
                 * Solicitation messages every RetransTimer milliseconds until
                 * reachability confirmation is obtained. Probes are retransmitted even
                 * if no additional packets are sent to the neighbor. If no response is
                 * received after waiting RetransTimer milliseconds after sending the
                 * MAX_UNICAST_SOLICIT solicitations, retransmissions cease and the
                 * entry SHOULD be deleted. Subsequent traffic to that neighbor will
                 *  recreate the entry and perform address resolution again.
                 */
                if((fnet_timer_get_ms() - neighbor_entry->state_time_ms) > nd6_if->retrans_timer_ms )
                {
                    neighbor_entry->solicitation_send_counter++;
                    if(neighbor_entry->solicitation_send_counter >= FNET_ND6_MAX_UNICAST_SOLICIT)
                        /* AR is failed.*/
                    {
                        /* RFC4861 7.3.3: If no response is
                         * received after waiting RetransTimer milliseconds after sending the
                         * MAX_UNICAST_SOLICIT solicitations
                         */
                        _fnet_nd6_neighbor_cache_del(netif, neighbor_entry);
                    }
                    else
                        /* NUD: transmit NS.*/
                    {
                        fnet_ip6_addr_t *dest_addr = &neighbor_entry->ip_addr;

                        /* Select source address.*/ /*PFI*/
                        const fnet_ip6_addr_t   *src_addr = _fnet_ip6_select_src_addr(netif, dest_addr);
                        if(src_addr)
                        {
                            neighbor_entry->state_time_ms = fnet_timer_get_ms();
                            dest_addr = (fnet_ip6_addr_t *)&neighbor_entry->ip_addr;
                            _fnet_nd6_neighbor_solicitation_send(netif, src_addr, dest_addr/*Unicast*/, dest_addr);
                        }
                    }
                }
                break;
            /* case FNET_ND6_NEIGHBOR_STATE_NOTUSED:*/
            default:
                break;
        }
    }
}

/************************************************************************
* DESCRIPTION: Put netbuf to the queue, waiting for address resolution to complete.
*************************************************************************/
void _fnet_nd6_neighbor_enqueue_waiting_netbuf(fnet_nd6_neighbor_entry_t *neighbor_entry, fnet_netbuf_t *waiting_netbuf)
{
    if (neighbor_entry && waiting_netbuf)
    {
        /* When a queue  overflows, the new arrival SHOULD replace the oldest entry.*/
        /*TBD for bigger queue.*/
        if(neighbor_entry->waiting_netbuf != FNET_NULL)
        {
            _fnet_netbuf_free_chain(neighbor_entry->waiting_netbuf); /* Free the oldest one.*/
        }

        neighbor_entry->waiting_netbuf = waiting_netbuf;
    }
}

/************************************************************************
* DESCRIPTION: Sends waiting PCBs, if any.
*************************************************************************/
static void _fnet_nd6_neighbor_send_waiting_netbuf(struct fnet_netif *netif, fnet_nd6_neighbor_entry_t *neighbor_entry)
{
    if (neighbor_entry && (neighbor_entry->waiting_netbuf != FNET_NULL))
    {
        /* Send.*/
        netif->netif_api->netif_output_ip6(netif, FNET_NULL /* not needed.*/,  &neighbor_entry->ip_addr, neighbor_entry->waiting_netbuf); /* IPv6 Transmit function.*/

        neighbor_entry->waiting_netbuf = FNET_NULL;
    }
}

/************************************************************************
* DESCRIPTION: Adds entry into the Router List.
*************************************************************************/
static void _fnet_nd6_router_list_add( fnet_nd6_neighbor_entry_t *neighbor_entry, fnet_time_t lifetime_sec )
{
    if (neighbor_entry)
    {
        if(lifetime_sec)
        {
            neighbor_entry->is_router = FNET_TRUE;
            neighbor_entry->router_lifetime_sec = lifetime_sec;
            neighbor_entry->creation_time_sec = fnet_timer_get_ms() / FNET_TIMER_MS_IN_SEC;
        }
        else
            /* If the address is already present in the host’s Default Router
             * List and the received Router Lifetime value is zero, immediately
             * time-out the entry.
             */
        {
            _fnet_nd6_router_list_del(neighbor_entry);
        }
    }
}

/************************************************************************
* DESCRIPTION: Deletes an entry from the Router List.
*************************************************************************/
static void _fnet_nd6_router_list_del( fnet_nd6_neighbor_entry_t *neighbor_entry )
{
    if (neighbor_entry)
    {
        neighbor_entry->is_router = FNET_FALSE;
        neighbor_entry->router_lifetime_sec = 0u;
    }
}

/************************************************************************
* DESCRIPTION: Chooses default router. Returns FNET_NULL if no router exists.
*************************************************************************/
fnet_nd6_neighbor_entry_t *_fnet_nd6_default_router_get(struct fnet_netif *netif)
{
    fnet_nd6_if_t               *nd6_if = netif->nd6_if_ptr;
    fnet_index_t                i;
    fnet_nd6_neighbor_entry_t   *result = FNET_NULL;

    if(nd6_if)
    {
        /* Find the first router. */
        for(i = 0u; i < FNET_ND6_NEIGHBOR_CACHE_SIZE; i++)
        {
            if( (nd6_if->neighbor_cache[i].state != FNET_ND6_NEIGHBOR_STATE_NOTUSED)
                && (nd6_if->neighbor_cache[i].is_router == FNET_TRUE)
                && (nd6_if->neighbor_cache[i].router_lifetime_sec != 0u) )
            {
                result = &nd6_if->neighbor_cache[i];
                /* Routers that are reachable or probably reachable (i.e., in any
                 * state other than INCOMPLETE) SHOULD be preferred over routers
                 * whose reachability is unknown or suspect.
                 */
                if(nd6_if->neighbor_cache[i].state != FNET_ND6_NEIGHBOR_STATE_INCOMPLETE)
                {
                    break;
                }
            }
        }

    }

    /* //TBD
     * RFFC4861: When no routers on the list are known to be reachable or
     * probably reachable, routers SHOULD be selected in a round-robin
     * fashion, so that subsequent requests for a default router do not
     * return the same router until all other routers have been
     * selected.
     * Cycling through the router list in this case ensures that all
     * available routers are actively probed by the Neighbor
     * Unreachability Detection algorithm. A request for a default
     * router is made in conjunction with the sending of a packet to a
     * router, and the selected router will be probed for reachability
     * as a side effect.
     */

    return result;
}

/************************************************************************
* DESCRIPTION: The IP source address of the Redirect is the same as the current
*                 first-hop router for the specified ICMP Destination Address.
*************************************************************************/
static fnet_bool_t _fnet_nd6_is_firsthop_router(fnet_netif_t *netif, fnet_ip6_addr_t *router_ip)
{
    fnet_nd6_if_t               *nd6_if = netif->nd6_if_ptr;
    fnet_index_t                i;
    fnet_nd6_neighbor_entry_t   *neighbor_cache_entry;
    fnet_bool_t                 result = FNET_FALSE;

    /* The IP source address of the Redirect is the same as the current
     * first-hop router for the specified ICMP Destination Address.*/
    neighbor_cache_entry = _fnet_nd6_neighbor_cache_get(netif, router_ip);
    if(neighbor_cache_entry)
    {
        if(neighbor_cache_entry->is_router == FNET_TRUE)
        {
            if(neighbor_cache_entry->router_lifetime_sec == 0u)
            {
                for(i = 0u; i < FNET_ND6_REDIRECT_TABLE_SIZE; i++)
                {
                    if(FNET_IP6_ADDR_EQUAL(&nd6_if->redirect_table[i].target_addr, router_ip))
                    {
                        result = FNET_TRUE;
                        break;
                    }
                }
            }
            else
            {
                result = FNET_TRUE;
            }
        }
    }
    return result;
}

/************************************************************************
* DESCRIPTION: Get entry from Prefix List that corresponds to "prefix".
* It returns FNET_NULL if no entry is found.
*************************************************************************/
static fnet_nd6_prefix_entry_t *_fnet_nd6_prefix_list_get(struct fnet_netif *netif, fnet_ip6_addr_t *prefix)
{
    fnet_nd6_if_t           *nd6_if = netif->nd6_if_ptr;
    fnet_index_t            i;
    fnet_nd6_prefix_entry_t *result = FNET_NULL;

    if (nd6_if)
    {
        /* Find the entry in the list.
        */
        for(i = 0u; i < FNET_ND6_PREFIX_LIST_SIZE; i++)
        {
            if( (nd6_if->prefix_list[i].state != FNET_ND6_PREFIX_STATE_NOTUSED) &&
                FNET_IP6_ADDR_EQUAL(&nd6_if->prefix_list[i].prefix, prefix))
            {
                result = &nd6_if->prefix_list[i];
                break;
            }
        }
    }
    return result;
}

/************************************************************************
* DESCRIPTION: Deletes an entry from the Prefix List.
*************************************************************************/
static void _fnet_nd6_prefix_list_del(fnet_nd6_prefix_entry_t *prefix_entry)
{
    if (prefix_entry)
    {
        prefix_entry->state = FNET_ND6_PREFIX_STATE_NOTUSED;
    }
}

/************************************************************************
* DESCRIPTION: Adds (TBD update) entry into the Prefix List.
*************************************************************************/
static fnet_nd6_prefix_entry_t *_fnet_nd6_prefix_list_add(struct fnet_netif *if_ptr, const fnet_ip6_addr_t *prefix, fnet_size_t prefix_length, fnet_time_t lifetime_sec)
{
    struct fnet_nd6_if      *nd6_if = if_ptr->nd6_if_ptr;
    fnet_index_t            i;
    fnet_nd6_prefix_entry_t *entry = FNET_NULL;

    if (nd6_if)
    {
        /* Find an unused entry in the cache. Skip 1st Link_locak prefix.
         */
        for(i = 1u; i < FNET_ND6_PREFIX_LIST_SIZE; i++)
        {
            if(nd6_if->prefix_list[i].state == FNET_ND6_PREFIX_STATE_NOTUSED)
            {
                entry = &nd6_if->prefix_list[i];
                break;
            }
        }

        /* If no free entry is found.
         */
        if(entry == FNET_NULL)
        {
            entry = &nd6_if->prefix_list[1];
            /* Try to find the oldest entry.
             */
            for(i = 1u; i < FNET_ND6_PREFIX_LIST_SIZE; i++)
            {
                if(nd6_if->prefix_list[i].creation_time_sec < entry->creation_time_sec)
                {
                    entry = &nd6_if->prefix_list[i];
                }
            }
        }

        /* Fill the informationn.
         */
        FNET_IP6_ADDR_COPY(prefix, &entry->prefix);
        entry->prefix_length = prefix_length;
        entry->lifetime_sec = lifetime_sec;
        entry->creation_time_sec = fnet_timer_get_ms() / FNET_TIMER_MS_IN_SEC;
        entry->state = FNET_ND6_PREFIX_STATE_USED;
    }

    return entry;
}

/************************************************************************
* DESCRIPTION: Timer routine used for Prefix List lifetime check.
*************************************************************************/
static void _fnet_nd6_prefix_timer(fnet_netif_t *netif)
{
    fnet_nd6_if_t   *nd6_if = netif->nd6_if_ptr;
    fnet_index_t    i;

    /* Check lifetime for prefixes.*/
    for(i = 1u; i < FNET_ND6_PREFIX_LIST_SIZE; i++)
    {
        if((nd6_if->prefix_list[i].state != FNET_ND6_PREFIX_STATE_NOTUSED)
           && (nd6_if->prefix_list[i].lifetime_sec != FNET_ND6_PREFIX_LIFETIME_INFINITE)
           && (((fnet_timer_get_ms() / FNET_TIMER_MS_IN_SEC) - nd6_if->prefix_list[i].creation_time_sec) > nd6_if->prefix_list[i].lifetime_sec) )
        {
            _fnet_nd6_prefix_list_del(&nd6_if->prefix_list[i]);
        }
    }
}

/************************************************************************
* DESCRIPTION: Add entry into the Redirect Table.
*************************************************************************/
static fnet_nd6_redirect_entry_t *_fnet_nd6_redirect_table_add(fnet_netif_t *if_ptr, const fnet_ip6_addr_t *destination_addr, const fnet_ip6_addr_t *target_addr)
{
    struct fnet_nd6_if          *nd6_if = if_ptr->nd6_if_ptr;
    fnet_index_t                i;
    fnet_nd6_redirect_entry_t   *entry = FNET_NULL;

    if (nd6_if)
    {

        /* Check if the destination address exists.*/
        for(i = 0u; i < FNET_ND6_REDIRECT_TABLE_SIZE; i++)
        {
            if(FNET_IP6_ADDR_EQUAL(&nd6_if->redirect_table[i].destination_addr, destination_addr))
            {
                /* Found existing destination address.*/
                entry = &nd6_if->redirect_table[i];
                break;
            }
        }

        if(entry == FNET_NULL)
        {
            /* Find an unused entry in the table.
             */
            for(i = 0u; i < FNET_ND6_REDIRECT_TABLE_SIZE; i++)
            {
                /* Check if it is free.*/
                if(FNET_IP6_ADDR_IS_UNSPECIFIED(&nd6_if->redirect_table[i].destination_addr))
                {
                    entry = &nd6_if->redirect_table[i];
                    break;
                }
            }

            /* If no free entry is found.
             */
            if(entry == FNET_NULL)
            {
                entry = &nd6_if->redirect_table[0];
                /* Try to find the oldest entry.
                 */
                for(i = 1u; i < FNET_ND6_REDIRECT_TABLE_SIZE; i++)
                {
                    if(nd6_if->redirect_table[i].creation_time_sec < entry->creation_time_sec)
                    {
                        entry = &nd6_if->redirect_table[i];
                    }
                }
            }
        }
        /* Fill the informationn.
         */
        FNET_IP6_ADDR_COPY(destination_addr, &entry->destination_addr);
        FNET_IP6_ADDR_COPY(target_addr, &entry->target_addr);
        entry->creation_time_sec = fnet_timer_get_ms() / FNET_TIMER_MS_IN_SEC;
    }

    return entry;
}

/************************************************************************
* DESCRIPTION: Deletes an entry from the Redirect Table.
*************************************************************************/
static void _fnet_nd6_redirect_table_del(fnet_netif_t *if_ptr, const fnet_ip6_addr_t *target_addr)
{
    struct fnet_nd6_if          *nd6_if = if_ptr->nd6_if_ptr;
    fnet_index_t                i;

    if (nd6_if)
    {
        for(i = 0u; i < FNET_ND6_REDIRECT_TABLE_SIZE; i++)
        {
            if(FNET_IP6_ADDR_EQUAL(&nd6_if->redirect_table[i].target_addr, target_addr))
            {
                /* Clear it.*/
                fnet_memset_zero(&nd6_if->redirect_table[i], sizeof(fnet_nd6_redirect_entry_t));
            }
        }
    }
}

/************************************************************************
* DESCRIPTION: Redirects destination address, if needed.
*************************************************************************/
void _fnet_nd6_redirect_addr(struct fnet_netif *if_ptr, const fnet_ip6_addr_t **destination_addr_p)
{
    struct fnet_nd6_if          *nd6_if = if_ptr->nd6_if_ptr;
    fnet_index_t                i;

    if (nd6_if)
    {
        for(i = 0u; i < FNET_ND6_REDIRECT_TABLE_SIZE; i++)
        {
            if(FNET_IP6_ADDR_EQUAL(&nd6_if->redirect_table[i].destination_addr, *destination_addr_p))
            {
                /* Found redirection.*/
                *destination_addr_p = &nd6_if->redirect_table[i].target_addr;
                break;
            }
        }
    }
}

/************************************************************************
* DESCRIPTION: Checks if the address is on-link.
*              Returns FNET_TRUE if it is on-line, FNET_FALSE otherwise.
*************************************************************************/
fnet_bool_t _fnet_nd6_addr_is_onlink(struct fnet_netif *netif, const fnet_ip6_addr_t *addr)
{
    fnet_nd6_if_t   *nd6_if = netif->nd6_if_ptr;
    fnet_index_t    i;
    fnet_bool_t     result = FNET_FALSE;

    if (nd6_if)
    {
        /* Find the entry in the list. */
        for(i = 0u; i < FNET_ND6_PREFIX_LIST_SIZE; i++)
        {
            if( (nd6_if->prefix_list[i].state != FNET_ND6_PREFIX_STATE_NOTUSED) &&
                (_fnet_ip6_addr_pefix_cmp(&nd6_if->prefix_list[i].prefix, addr, nd6_if->prefix_list[i].prefix_length) == FNET_TRUE) )
            {
                result = FNET_TRUE;
                break;
            }
        }
    }
    return result;
}

/************************************************************************
* DESCRIPTION: Sends an Neighbor Solicitation Message.
*   Nodes send Neighbor Solicitations to request the link-layer address
*   of a target node while also providing their own link-layer address to
*   the target.
*************************************************************************/
void _fnet_nd6_neighbor_solicitation_send(struct fnet_netif *netif /*MUST*/, const fnet_ip6_addr_t *ipsrc /* FNET_NULL for, DAD */, const fnet_ip6_addr_t *ipdest /*set for NUD,  FNET_NULL for DAD & AR */, const fnet_ip6_addr_t *target_addr)
{
    fnet_size_t                     ns_packet_size;
    fnet_netbuf_t                   *ns_nb;
    fnet_nd6_ns_header_t            *ns_packet;
    fnet_nd6_option_lla_header_t    *nd_option_slla;
    fnet_ip6_addr_t                 _ip_dest;

    /* Check if, this is DAD.
     * Duplicate Address Detection sends Neighbor Solicitation
     * messages with an unspecified source address targeting its own
     * "tentative" address and without SLLAO.*/
    ns_packet_size = sizeof(fnet_nd6_ns_header_t) + ((ipsrc == FNET_NULL /* DAD */) ? 0u : (sizeof(fnet_nd6_option_header_t) + netif->netif_api->netif_hw_addr_size));
    if((ns_nb = _fnet_netbuf_new(ns_packet_size, FNET_TRUE)) != 0u)
    {
        /*
         * Neighbor Solicitations are multicast when the node needs
         * to resolve an address and unicast when the node seeks to verify the
         * reachability of a neighbor.
         */
        if(ipdest == FNET_NULL)
        {
            /* AR, DAD */
            /* Generate Solicited Multicast destination address from the target address.*/
            _fnet_ip6_get_solicited_multicast_addr(target_addr, &_ip_dest);
            ipdest = &_ip_dest;
        }
        /* else  NUD */

        /* Fill ICMP Header */
        ns_packet = (fnet_nd6_ns_header_t *)ns_nb->data_ptr;
        ns_packet->icmp6_header.type = FNET_ICMP6_TYPE_NEIGHBOR_SOLICITATION;
        ns_packet->icmp6_header.code = 0u;

        /* Fill NS Header.*/
        fnet_memset_zero( ns_packet->_reserved, sizeof(ns_packet->_reserved));   /* Set to zeros the reserved field.*/
        FNET_IP6_ADDR_COPY(target_addr, &ns_packet->target_addr);               /* Set NS target address.*/

        /*
         * RFC4861: The link-layer address for the sender. MUST NOT be
         * included when the source IP address is the
         * unspecified address (DAD). Otherwise, on link layers
         * that have addresses this option MUST be included in
         * multicast solicitations and SHOULD be included in
         * unicast solicitations.
         */
        if(ipsrc != FNET_NULL /* AR or NUD */)
        {
            /* RFC4861 7.2.2: If the source address of the packet prompting the solicitation is the
             * same as one of the addresses assigned to the outgoing interface, that
             * address SHOULD be placed in the IP Source Address of the outgoing
             * solicitation. Otherwise, any one of the addresses assigned to the
             * interface should be used.
             */
            if(_fnet_netif_is_my_ip6_addr(netif, ipsrc) == FNET_FALSE)
            {
                ipsrc = _fnet_ip6_select_src_addr(netif, ipdest);

                if(ipsrc == FNET_NULL)
                {
                    goto DROP; /* Just in case. Should never happen.*/
                }
            }

            /* Fill Source link-layer address option.*/
            nd_option_slla = (fnet_nd6_option_lla_header_t *)((fnet_uint32_t)ns_packet + sizeof(fnet_nd6_ns_header_t));
            nd_option_slla->option_header.type = FNET_ND6_OPTION_SOURCE_LLA; /* Type. */
            nd_option_slla->option_header.length = (fnet_uint8_t)((netif->netif_api->netif_hw_addr_size + sizeof(fnet_nd6_option_header_t)) >> 3); /* Option size devided by 8,*/

            if( _fnet_netif_get_hw_addr(netif, nd_option_slla->addr, netif->netif_api->netif_hw_addr_size) != FNET_OK)
            {
                goto DROP;
            }

            /*TBD Padd if needed.%8*/
        }
        else
        {
            /* Source IP address is the
             * unspecified address for DAD.
             */
            ipsrc = &fnet_ip6_addr_any;
        }

        /* Send ICMPv6 message.*/
        _fnet_icmp6_output( netif, ipsrc, ipdest, FNET_ND6_HOP_LIMIT, ns_nb);
    }

    return;

DROP:
    _fnet_netbuf_free_chain(ns_nb);
}

/************************************************************************
* DESCRIPTION: Handles received Neighbor Solicitation message.
*************************************************************************/
void _fnet_nd6_neighbor_solicitation_receive(struct fnet_netif *netif, fnet_ip6_addr_t *src_ip, fnet_ip6_addr_t *dest_ip, fnet_netbuf_t *nb, fnet_netbuf_t *ip6_nb)
{
    fnet_icmp6_header_t             *icmp6_packet = (fnet_icmp6_header_t *)nb->data_ptr;
    fnet_size_t                     icmp6_packet_size = nb->total_length;
    fnet_nd6_ns_header_t            *ns_packet = (fnet_nd6_ns_header_t *)icmp6_packet;
    fnet_nd6_option_lla_header_t    *nd_option_slla = FNET_NULL;
    fnet_size_t                     nd_option_offset;
    fnet_netif_ip6_addr_t           *target_if_addr_info;
    fnet_ip6_header_t               *ip6_packet = (fnet_ip6_header_t *)ip6_nb->data_ptr;


    /************************************************************
    * Validation.
    *************************************************************/
    if(
        (icmp6_packet_size < sizeof(fnet_nd6_ns_header_t))
        /* Validation RFC4861 (7.1.1).*/
        || (ip6_packet->hop_limit != FNET_ND6_HOP_LIMIT)        /* The IP Hop Limit field has a value of 255.*/
        || (icmp6_packet->code != 0u)                           /* ICMP Code is 0.*/
        || FNET_IP6_ADDR_IS_MULTICAST(&ns_packet->target_addr)  /* Target Address is not a multicast address. */
        || (_fnet_netif_is_my_ip6_addr(netif, src_ip))            /* Duplicate IP address check. */ /*TBD???*/
    )
    {
        goto DROP;
    }


    {
        /* Validation passed.
         */

        /************************************************************
         * Handle posible options.
         ************************************************************/
        nd_option_offset = sizeof(fnet_nd6_ns_header_t);
        while(icmp6_packet_size > nd_option_offset + sizeof(fnet_nd6_option_header_t))
        {
            fnet_nd6_option_header_t *nd_option;

            nd_option =  (fnet_nd6_option_header_t *) ((fnet_uint8_t *)icmp6_packet + nd_option_offset) ;
            /* Validation RFC4861 (7.1.1). All included options have a length that is greater than zero.
             */
            if(nd_option->length == 0u)
            {
                goto DROP;
            }

            /* Ensure the full option body (length * 8 bytes) lies within the
             * received packet before any option field is dereferenced. */
            if((nd_option_offset + ((fnet_size_t)nd_option->length << 3)) > icmp6_packet_size)
            {
                goto DROP;
            }

            /* Handle Source link-layer address option only.
             */
            if((nd_option->type == FNET_ND6_OPTION_SOURCE_LLA)
               && ( (((fnet_size_t)nd_option->length << 3) - sizeof(fnet_nd6_option_header_t)) >= netif->netif_api->netif_hw_addr_size) )
            {
                nd_option_slla = (fnet_nd6_option_lla_header_t *)nd_option; /* Source Link-layer Address option is found.*/
            }
            /* else, silently ignore any options they do not recognize
             * and continue processing the message.
             */

            nd_option_offset += ((fnet_size_t)nd_option->length << 3);
        }

        if(nd_option_slla  != FNET_NULL)
        {
            /* Validation RFC4861 (7.1.1): If the IP source address is the
             * unspecified address, there is no source link-layer address option in the message.
             * NOTE: RFC4861 (7):Duplicate Address Detection sends Neighbor Solicitation
             * messages with an unspecified source address targeting its own
             * "tentative" address.
             */
            if(FNET_IP6_ADDR_IS_UNSPECIFIED(src_ip))
            {
                /* If the Source Address is the unspecified address, the node MUST NOT
                 * create or update the Neighbor Cache entry.
                 */
                goto DROP;
            }
            else
            {
                fnet_nd6_neighbor_entry_t *neighbor_cache_entry;

                /* RFC 48617.2.3: the recipient
                 * SHOULD create or update the Neighbor Cache entry for the IP Source
                 * Address of the solicitation.
                 */
                neighbor_cache_entry = _fnet_nd6_neighbor_cache_get(netif, src_ip);
                if(neighbor_cache_entry == FNET_NULL)
                {
                    /* If an entry does not already exist, the
                     * node SHOULD create a new one and set its reachability state to STALE
                     * as specified in Section 7.3.3.
                     * If a Neighbor Cache entry is created, the IsRouter flag SHOULD be set
                     * to FALSE. This will be the case even if the Neighbor Solicitation is
                     * sent by a router since the Neighbor Solicitation messages do not
                     * contain an indication of whether or not the sender is a router. In
                     * the event that the sender is a router, subsequent Neighbor
                     * Advertisement or Router Advertisement messages will set the correct
                     * IsRouter value.
                     */
                    _fnet_nd6_neighbor_cache_add(netif, src_ip, nd_option_slla->addr, FNET_ND6_NEIGHBOR_STATE_STALE);
                }
                else
                {
                    /* If an entry already exists, and the
                     * cached link-layer address differs from the one in the received Source
                     * Link-Layer option, the cached address should be replaced by the
                     * received address, and the entry’s reachability state MUST be set to
                     * STALE.
                     * If a Neighbor Cache entry already exists, its
                     * IsRouter flag MUST NOT be modified.
                     */
                    if(!FNET_NETIF_LL_ADDR_ARE_EQUAL(nd_option_slla->addr, neighbor_cache_entry->ll_addr, netif->netif_api->netif_hw_addr_size))
                    {
                        FNET_NETIF_LL_ADDR_COPY(nd_option_slla->addr, neighbor_cache_entry->ll_addr, netif->netif_api->netif_hw_addr_size);
                        neighbor_cache_entry->state = FNET_ND6_NEIGHBOR_STATE_STALE;
                    }
                    else
                    {
                        /* RFC4861: Appendix C
                         */ /*TBD ??*/
                        if(neighbor_cache_entry->state == FNET_ND6_NEIGHBOR_STATE_INCOMPLETE)
                        {
                            neighbor_cache_entry->state = FNET_ND6_NEIGHBOR_STATE_STALE;
                        }
                    }

                    /* - It sends any packets queued for the neighbor awaiting address
                     *   resolution.*/
                    _fnet_nd6_neighbor_send_waiting_netbuf(netif, neighbor_cache_entry);
                }
            }
        }

        /* Get Target Address Info, according tp Target Address of NS message.
         */
        target_if_addr_info = _fnet_netif_get_ip6_addr_info(netif, &ns_packet->target_addr);

        /* Check if we are the target.
         */
        if(target_if_addr_info != FNET_NULL)
        {
            /* Duplicate Address Detection (DAD) sends Neighbor Solicitation
             * messages with an unspecified source address targeting its own
             * "tentative" address.
             */
            if(FNET_IP6_ADDR_IS_UNSPECIFIED(src_ip))
            {
                /* === Duplicate Address Detection ===*/

                /* IP Destination address must be our solitation address.
                 */
                if(_fnet_netif_is_my_ip6_solicited_multicast_addr(netif, dest_ip))
                {
                    if(target_if_addr_info->state != FNET_NETIF_IP6_ADDR_STATE_TENTATIVE)
                    {
                        /* If the source of the solicitation is the unspecified address, the
                         * node MUST set the Solicited flag to zero and multicast the
                         * advertisement to the all-nodes address.
                         */
                        _fnet_nd6_neighbor_advertisement_send(netif, &target_if_addr_info->address /*ipsrc*/, &fnet_ip6_addr_linklocal_allnodes/*ipdest*/, FNET_ND6_NA_FLAG_OVERRIDE);
                    }
                    else
                    {
                        _fnet_nd6_dad_failed(netif, target_if_addr_info);  /* => DAD is failed. */
                        /* MUST be silently discarded, if the Target Address is a "tentative"
                         * address on which Duplicate Address Detection is being performed [ADDRCONF].*/
                    }

                }
                /* else DROP IT.
                 */
            }
            else if(    /* === Address Resolution === */
                (_fnet_netif_is_my_ip6_solicited_multicast_addr(netif, dest_ip))
                ||  /* === Neighbor Unreachability Detection === */
                ((FNET_IP6_ADDR_EQUAL(&ns_packet->target_addr, dest_ip))) )
            {
                /*
                 * Sends a Neighbor Advertisement response.
                 */
                _fnet_nd6_neighbor_advertisement_send(netif, &target_if_addr_info->address /*ipsrc*/, src_ip/*ipdest*/, FNET_ND6_NA_FLAG_SOLICITED | FNET_ND6_NA_FLAG_OVERRIDE);
            }
            else
            {} /* else: Bad packet.*/

        }
        /* else: Not for us => Discarded.
         */
    }

DROP:
    _fnet_netbuf_free_chain(ip6_nb);
    _fnet_netbuf_free_chain(nb);
}

/************************************************************************
* DESCRIPTION: Sends an Neighbor Advertisement message.
*************************************************************************/
static void _fnet_nd6_neighbor_advertisement_send(struct fnet_netif *netif, const fnet_ip6_addr_t *ipsrc, const fnet_ip6_addr_t *ipdest, fnet_uint8_t na_flags)
{
    fnet_size_t                     na_packet_size;
    fnet_netbuf_t                   *na_nb;
    fnet_nd6_na_header_t            *na_packet;
    fnet_nd6_option_lla_header_t    *nd_option_tlla;

    na_packet_size = sizeof(fnet_nd6_na_header_t) + sizeof(fnet_nd6_option_header_t) + netif->netif_api->netif_hw_addr_size;

    if((na_nb = _fnet_netbuf_new(na_packet_size, FNET_TRUE)) != 0)
    {
        /* Fill ICMP Header */
        na_packet = (fnet_nd6_na_header_t *)na_nb->data_ptr;
        na_packet->icmp6_header.type = FNET_ICMP6_TYPE_NEIGHBOR_ADVERTISEMENT;
        na_packet->icmp6_header.code = 0u;

        /* NA header.*/
        na_packet->flag = na_flags;    /* Flag parameter.*/
        fnet_memset_zero( na_packet->_reserved, sizeof(na_packet->_reserved));  /* Set to zeros the reserved field.*/
        FNET_IP6_ADDR_COPY(ipsrc, &na_packet->target_addr);                     /* Set NA target address, the same as for NS.*/

        /* Fill Target Link-Layer Address option.*/
        nd_option_tlla = (fnet_nd6_option_lla_header_t *)((fnet_uint32_t)na_packet + sizeof(fnet_nd6_na_header_t));
        nd_option_tlla->option_header.type = FNET_ND6_OPTION_TARGET_LLA; /* Type. */
        nd_option_tlla->option_header.length = (fnet_uint8_t)((netif->netif_api->netif_hw_addr_size + sizeof(fnet_nd6_option_header_t)) >> 3); /* Option size devided by 8,*/

        if(_fnet_netif_get_hw_addr(netif, nd_option_tlla->addr, netif->netif_api->netif_hw_addr_size) != FNET_OK)    /* Link-Layer Target address.*/
        {
            goto DROP;
        }

        /* Send ICMPv6 message.*/
        _fnet_icmp6_output( netif, ipsrc, ipdest, FNET_ND6_HOP_LIMIT, na_nb);
    }

    return;

DROP:
    _fnet_netbuf_free_chain(na_nb);
}

/************************************************************************
* DESCRIPTION: Sends an Router Solicitation Message.
*   Nodes send Neighbor Solicitations to request the link-layer address
*   of a target node while also providing their own link-layer address to
*   the target.
*************************************************************************/
static void _fnet_nd6_router_solicitation_send(struct fnet_netif *netif)
{
    fnet_size_t                     rs_packet_size;
    fnet_netbuf_t                   *rs_nb;
    fnet_nd6_rs_header_t            *rs_packet;
    fnet_nd6_option_lla_header_t    *nd_option_slla;
    const fnet_ip6_addr_t           *ip_dest;
    const fnet_ip6_addr_t           *ip_src;

    /* Destination Address is the all-routers multicast address.*/
    ip_dest = &fnet_ip6_addr_linklocal_allrouters;
    /* Choose source address.*/
    ip_src = _fnet_ip6_select_src_addr(netif, ip_dest);

    rs_packet_size = sizeof(fnet_nd6_rs_header_t) + ((ip_src == FNET_NULL /* no address */) ? 0u : (sizeof(fnet_nd6_option_header_t) + netif->netif_api->netif_hw_addr_size));

    if((rs_nb = _fnet_netbuf_new(rs_packet_size, FNET_TRUE)) != 0)
    {
        /* Fill ICMP Header */
        rs_packet = (fnet_nd6_rs_header_t *)rs_nb->data_ptr;
        rs_packet->icmp6_header.type = FNET_ICMP6_TYPE_ROUTER_SOLICITATION;
        rs_packet->icmp6_header.code = 0u;

        /* Fill RS Header.*/
        fnet_memset_zero( rs_packet->_reserved, sizeof(rs_packet->_reserved));    /* Set to zeros the reserved field.*/

        /*
         * RFC4861: The link-layer address of the sender, if
         * known. MUST NOT be included if the Source Address
         * is the unspecified address. Otherwise, it SHOULD
         * be included on link layers that have addresses.
         */
        if(ip_src != FNET_NULL )
        {
            /* Fill Source link-layer address option.*/
            nd_option_slla = (fnet_nd6_option_lla_header_t *)((fnet_uint32_t)rs_packet + sizeof(fnet_nd6_rs_header_t));
            nd_option_slla->option_header.type = FNET_ND6_OPTION_SOURCE_LLA;    /* Type. */
            nd_option_slla->option_header.length = (fnet_uint8_t)((netif->netif_api->netif_hw_addr_size + sizeof(fnet_nd6_option_header_t)) >> 3); /* Option size devided by 8,*/

            if(_fnet_netif_get_hw_addr(netif, nd_option_slla->addr, netif->netif_api->netif_hw_addr_size) != FNET_OK)    /* Link-Layer Target address.*/
            {
                goto DROP;
            }

            /* TBD Padd if needed.%8 */
        }
        else
        {
            /* Source IP address is the
             * unspecified address.
             */
            ip_src = &fnet_ip6_addr_any;
        }

        /* Send ICMPv6 message.*/
        _fnet_icmp6_output( netif, ip_src, ip_dest, FNET_ND6_HOP_LIMIT, rs_nb);
    }

    return;

DROP:
    _fnet_netbuf_free_chain(rs_nb);
}

/************************************************************************
* DESCRIPTION: Handles received Neighbor Advertisement message.
*************************************************************************/
void _fnet_nd6_neighbor_advertisement_receive(struct fnet_netif *netif, fnet_ip6_addr_t *src_ip, fnet_ip6_addr_t *dest_ip, fnet_netbuf_t *nb, fnet_netbuf_t *ip6_nb)
{
    fnet_icmp6_header_t             *icmp6_packet = (fnet_icmp6_header_t *)nb->data_ptr;
    fnet_size_t                     icmp6_packet_size = nb->total_length;
    fnet_nd6_na_header_t            *na_packet = (fnet_nd6_na_header_t *)icmp6_packet;
    fnet_nd6_option_lla_header_t    *nd_option_tlla = FNET_NULL;
    fnet_size_t                     nd_option_offset;
    fnet_netif_ip6_addr_t           *target_if_addr_info;
    fnet_ip6_header_t               *ip6_packet = (fnet_ip6_header_t *)ip6_nb->data_ptr;
    fnet_nd6_neighbor_entry_t       *neighbor_cache_entry;
    fnet_bool_t                     is_solicited;
    fnet_bool_t                     is_router;
    fnet_bool_t                     is_override;
    fnet_bool_t                     is_ll_addr_changed;

    FNET_COMP_UNUSED_ARG(src_ip);

    /************************************************************
    * Validation.
    *************************************************************/
    if(
        (icmp6_packet_size < sizeof(fnet_nd6_na_header_t))          /* Check mi. size of packet*/
        /* Validation RFC4861 (7.1.2).*/
        || (ip6_packet->hop_limit != FNET_ND6_HOP_LIMIT)            /* The IP Hop Limit field has a value of 255.*/
        || (icmp6_packet->code != 0u)                                /* ICMP Code is 0.*/
        || FNET_IP6_ADDR_IS_MULTICAST(&na_packet->target_addr)      /* Target Address is not a multicast address. */
        || ( (  (is_solicited = (((na_packet->flag & FNET_ND6_NA_FLAG_SOLICITED) != 0u) ? FNET_TRUE : FNET_FALSE)) != FNET_FALSE)
             && FNET_IP6_ADDR_IS_MULTICAST(dest_ip) )                /* If the IP Destination Address is a multicast
                                                                     * address the Solicited flag is zero.*/
    )
    {
        goto DROP;
    }

    {
        /************************************************************
         * Handle posible options.
         ************************************************************/
        nd_option_offset = sizeof(fnet_nd6_na_header_t);
        while(icmp6_packet_size > nd_option_offset + sizeof(fnet_nd6_option_header_t))
        {
            fnet_nd6_option_header_t *nd_option;

            nd_option =  (fnet_nd6_option_header_t *) ((fnet_uint8_t *)icmp6_packet + nd_option_offset) ;
            /* Validation RFC4861 (7.1.2). All included options have a length that is greater than zero.
             */
            if(nd_option->length == 0u)
            {
                goto DROP;
            }

            /* Ensure the full option body (length * 8 bytes) lies within the
             * received packet before any option field is dereferenced. */
            if((nd_option_offset + ((fnet_size_t)nd_option->length << 3)) > icmp6_packet_size)
            {
                goto DROP;
            }

            /* Handle Target Link-Layer Address option only.
             */
            if((nd_option->type == FNET_ND6_OPTION_TARGET_LLA)
               && ( (((fnet_size_t)nd_option->length << 3) - sizeof(fnet_nd6_option_header_t)) >= netif->netif_api->netif_hw_addr_size) )
            {
                nd_option_tlla = (fnet_nd6_option_lla_header_t *)nd_option; /* Target Link-layer Address option is found.*/
            }
            /* else, silently ignore any options they do not recognize
             * and continue processing the message.
             */

            nd_option_offset += ((fnet_size_t)nd_option->length << 3);
        }

        /* Get Target Address Info, according to Target Address of NA message.
        */
        target_if_addr_info = _fnet_netif_get_ip6_addr_info(netif, &na_packet->target_addr);
        if(target_if_addr_info != FNET_NULL)
        {
            /* Duplicated address!!!!! */
            if(target_if_addr_info->state == FNET_NETIF_IP6_ADDR_STATE_TENTATIVE)
            {
                _fnet_nd6_dad_failed(netif, target_if_addr_info); /* => DAD is failed. */
            }

            goto DROP;
        }

        /* A Neighbor Advertisements that passes the validity checks is called a
         * "valid advertisement".
         */

        /* Extract flag values.*/
        is_router = ((na_packet->flag & FNET_ND6_NA_FLAG_ROUTER) != 0u) ? FNET_TRUE : FNET_FALSE;
        is_override = ((na_packet->flag & FNET_ND6_NA_FLAG_OVERRIDE) != 0u) ? FNET_TRUE : FNET_FALSE;


        /************************************************************
         * Handle NA message.
         ************************************************************/

        /* RFC4861 7.2.5: Neighbor Cache is searched for the target’s entry.
         */
        neighbor_cache_entry = _fnet_nd6_neighbor_cache_get(netif, &na_packet->target_addr);
        if(neighbor_cache_entry == FNET_NULL)
        {
            /* If no entry exists, the advertisement SHOULD be silently discarded.
             */
            goto DROP;
        }

        /* If the target’s Neighbor Cache entry is in the INCOMPLETE state.*/
        if(neighbor_cache_entry->state == FNET_ND6_NEIGHBOR_STATE_INCOMPLETE)
        {
            /* If the link layer has addresses and no Target Link-Layer Address option is
             * included, the receiving node SHOULD silently discard the received
             * advertisement.*/
            if(nd_option_tlla == FNET_NULL)
            {
                goto DROP;
            }
            /* Otherwise, the receiving node performs the following
             * steps:
             * - It records the link-layer address in the Neighbor Cache entry.
             */
            FNET_NETIF_LL_ADDR_COPY(nd_option_tlla->addr, neighbor_cache_entry->ll_addr, netif->netif_api->netif_hw_addr_size);

            /* - If the advertisement’s Solicited flag is set, the state of the
             *   entry is set to REACHABLE; otherwise, it is set to STALE.
             */
            if (is_solicited)
            {
                neighbor_cache_entry->state = FNET_ND6_NEIGHBOR_STATE_REACHABLE;
                /* Reset Reachable Timer. */
                neighbor_cache_entry->state_time_ms = fnet_timer_get_ms();
            }
            else
            {
                neighbor_cache_entry->state = FNET_ND6_NEIGHBOR_STATE_STALE;
            }

            /* - It sets the IsRouter flag in the cache entry based on the Router
             *   flag in the received advertisement.
             */
            neighbor_cache_entry->is_router = is_router;

            /* - It sends any packets queued for the neighbor awaiting address
             *   resolution.
             */
            _fnet_nd6_neighbor_send_waiting_netbuf(netif, neighbor_cache_entry);
        }
        else
        {
            /* If the target’s Neighbor Cache entry is in any state other than
             * INCOMPLETE.
             */
            if( nd_option_tlla )
            {
                /* If supplied link-layer address differs. */
                is_ll_addr_changed = (!FNET_NETIF_LL_ADDR_ARE_EQUAL(nd_option_tlla->addr, neighbor_cache_entry->ll_addr, netif->netif_api->netif_hw_addr_size)) ? FNET_TRUE : FNET_FALSE;
            }
            else
            {
                is_ll_addr_changed = FNET_FALSE;
            }

            /* I. If the Override flag is clear and the supplied link-layer address
             *     differs from that in the cache.
             */
            if((is_override == FNET_FALSE) && is_ll_addr_changed)
            {
                /* a. If the state of the entry is REACHABLE, set it to STALE, but
                 *    do not update the entry in any other way.
                 */
                if(neighbor_cache_entry->state == FNET_ND6_NEIGHBOR_STATE_REACHABLE)
                {
                    neighbor_cache_entry->state = FNET_ND6_NEIGHBOR_STATE_STALE;
                }
                /* b. Otherwise, the received advertisement should be ignored and
                 *    MUST NOT update the cache.
                 */
                goto DROP;
            }
            /* II. If the Override flag is set, or the supplied link-layer address
             *     is the same as that in the cache, or no Target Link-Layer Address
             *     option was supplied, the received advertisement MUST update the
             *     Neighbor Cache entry as follows:
             */
            else
            {
                /* - The link-layer address in the Target Link-Layer Address option
                 *   MUST be inserted in the cache (if one is supplied and differs
                 *   from the already recorded address).
                 */
                if(nd_option_tlla)
                {
                    FNET_NETIF_LL_ADDR_COPY(nd_option_tlla->addr, neighbor_cache_entry->ll_addr, netif->netif_api->netif_hw_addr_size);
                }

                /* - If the Solicited flag is set, the state of the entry MUST be
                 *   set to REACHABLE.
                 */
                if(is_solicited)
                {
                    neighbor_cache_entry->state = FNET_ND6_NEIGHBOR_STATE_REACHABLE;
                    /* Reset Reachable Timer.*/
                    neighbor_cache_entry->state_time_ms = fnet_timer_get_ms();
                }
                /* If the Solicited flag is zero and the linklayer
                 *   address was updated with a different address, the state
                 *   MUST be set to STALE. Otherwise, the entry’s state remains
                 *   unchanged.
                 */
                else if(is_ll_addr_changed)
                {
                    neighbor_cache_entry->state = FNET_ND6_NEIGHBOR_STATE_STALE;
                }
                else
                {}
                /* - The IsRouter flag in the cache entry MUST be set based on the
                 *   Router flag in the received advertisement. In those cases
                 *   where the IsRouter flag changes from TRUE to FALSE as a result
                 *   of this update, the node MUST remove that router from the
                 *   Default Router List and update the Destination Cache entries
                 *   for all destinations using that neighbor as a router as
                 *   specified in Section 7.3.3.
                 */
                if((neighbor_cache_entry->is_router) && (is_router == FNET_FALSE))
                {
                    _fnet_nd6_router_list_del(neighbor_cache_entry);
                }
            }
        }
    }

DROP:
    _fnet_netbuf_free_chain(ip6_nb);
    _fnet_netbuf_free_chain(nb);
}

/************************************************************************
* DESCRIPTION: Handles received Router Advertisement message.
*************************************************************************/
void _fnet_nd6_router_advertisement_receive(struct fnet_netif *netif, fnet_ip6_addr_t *src_ip, fnet_ip6_addr_t *dest_ip, fnet_netbuf_t *nb, fnet_netbuf_t *ip6_nb)
{
    fnet_icmp6_header_t             *icmp6_packet = (fnet_icmp6_header_t *)nb->data_ptr;
    fnet_size_t                     icmp6_packet_size = nb->total_length;
    fnet_nd6_ra_header_t            *ra_packet = (fnet_nd6_ra_header_t *)icmp6_packet;
    fnet_nd6_option_lla_header_t    *nd_option_slla = FNET_NULL;
    fnet_nd6_option_mtu_header_t    *nd_option_mtu = FNET_NULL;
    fnet_nd6_option_prefix_header_t *nd_option_prefix[FNET_CFG_ND6_PREFIX_LIST_SIZE];
    fnet_index_t                    prefix_index;
    fnet_size_t                     nd_option_offset;
    fnet_ip6_header_t               *ip6_packet = (fnet_ip6_header_t *)ip6_nb->data_ptr;
    fnet_index_t                    i;
    fnet_nd6_neighbor_entry_t       *neighbor_cache_entry;


    FNET_COMP_UNUSED_ARG(dest_ip);

    /************************************************************
    * Validation of Router Advertisement Message.
    *************************************************************/
    if(
        (icmp6_packet_size < sizeof(fnet_nd6_ra_header_t))
        /* Validation RFC4861 (6.1.2).*/
        || (ip6_packet->hop_limit != FNET_ND6_HOP_LIMIT)    /* The IP Hop Limit field has a value of 255.*/
        || (icmp6_packet->code != 0u)                        /* ICMP Code is 0.*/
        || (!FNET_IP6_ADDR_IS_LINKLOCAL(src_ip))              /* MUST be the link-local address.*/
    )
    {
        goto DROP;
    }

    {
        /************************************************************
         * Handle posible options.
         ************************************************************
         * The contents of any defined options that are not specified
         * to be used with Router Advertisement messages MUST be
         * ignored and the packet processed as normal. The only defined
         * options that may appear are the Source Link-Layer Address,
         * Prefix Information and MTU options.
         ************************************************************/
        nd_option_offset = sizeof(fnet_nd6_ra_header_t);
        prefix_index = 0u;
        while(icmp6_packet_size > nd_option_offset + sizeof(fnet_nd6_option_header_t))
        {
            fnet_nd6_option_header_t *nd_option;

            nd_option =  (fnet_nd6_option_header_t *) ((fnet_uint8_t *)icmp6_packet + nd_option_offset) ;
            /* Validation RFC4861 (6.1.2). All included options have a length that is greater than zero.
             */
            if(nd_option->length == 0u)
            {
                goto DROP;
            }

            /* Ensure the full option body (length * 8 bytes) lies within the
             * received packet before any option field is dereferenced. */
            if((nd_option_offset + ((fnet_size_t)nd_option->length << 3)) > icmp6_packet_size)
            {
                goto DROP;
            }

            /* The advertisement is scanned for valid options.
             */

            /* Source Link-Layer Address option.*/
            if( (nd_option->type == FNET_ND6_OPTION_SOURCE_LLA)
                && ( ((fnet_size_t)nd_option->length << 3) >= (netif->netif_api->netif_hw_addr_size) + sizeof(fnet_nd6_option_header_t)) )
            {
                nd_option_slla = (fnet_nd6_option_lla_header_t *)nd_option; /* Target Link-layer Address option is found.*/
            }
            /* MTU option */
            else if( (nd_option->type == FNET_ND6_OPTION_MTU)
                     && (((fnet_size_t)nd_option->length << 3) >= sizeof(fnet_nd6_option_mtu_header_t)) )
            {
                nd_option_mtu = (fnet_nd6_option_mtu_header_t *)nd_option; /* MTU option is found.*/
            }
            /* Prefix option */
            else if( (nd_option->type == FNET_ND6_OPTION_PREFIX)
                     && (((fnet_size_t)nd_option->length << 3) >= sizeof(fnet_nd6_option_prefix_header_t)) )
            {
                /* Note that there can be multiple "Prefix information" options included in a router advertisement.*/
                if(prefix_index < FNET_CFG_ND6_PREFIX_LIST_SIZE)
                {
                    nd_option_prefix[prefix_index] = (fnet_nd6_option_prefix_header_t *)nd_option; /* Prefix information.*/
                    prefix_index++;
                }
            }
#if FNET_CFG_ND6_RDNSS && FNET_CFG_DNS
            /* RDNSS option */
            else if( (nd_option->type == FNET_ND6_OPTION_RDNSS)
                     && (((fnet_size_t)nd_option->length << 3) >= sizeof(fnet_nd6_option_rdnss_header_t)) )
            {
                /*************************************************************
                * Process RDNSS options.
                *************************************************************/
                fnet_nd6_option_rdnss_header_t  *nd_option_rdnss = (fnet_nd6_option_rdnss_header_t *)nd_option;
                /* The number of addresses is equal to (Length - 1) / 2.*/
                fnet_index_t                     rdnss_number = (fnet_index_t)(nd_option->length - 1u) / 2u;

                for(i = 0u; i < rdnss_number; i++)
                {
                    _fnet_nd6_rdnss_list_update(netif, &nd_option_rdnss->address[i], fnet_ntohl(nd_option_rdnss->lifetime_sec));
                }
            }
#endif /* FNET_CFG_ND6_RDNSS && FNET_CFG_DNS*/
            else
            {}  /* else, silently ignore any options they do not recognize
                * and continue processing the message. */

            nd_option_offset += ((fnet_size_t)nd_option->length << 3);
        }

        /************************************************************
         * Set parameters.
         ************************************************************/

        /* RFC4861 6.3.4: If the received Cur Hop Limit value is non-zero, the host SHOULD set
         * its CurHopLimit variable to the received value.
         */
        if(ra_packet->cur_hop_limit != 0u)
        {
            netif->nd6_if_ptr->cur_hop_limit = ra_packet->cur_hop_limit;
        }

        /* RFC4861 6.3.4: If the received Reachable Time value is non-zero, the host SHOULD set
         * its BaseReachableTime variable to the received value.
         */
        if(ra_packet->reachable_time_ms != 0u)
        {
            netif->nd6_if_ptr->reachable_time_ms = fnet_ntohl(ra_packet->reachable_time_ms);
        }

        /* RFC4861 6.3.4:The RetransTimer variable SHOULD be copied from the Retrans Timer
         * field, if the received value is non-zero.
         */
        if(fnet_ntohl(ra_packet->retrans_timer_ms) != 0u)
        {
            netif->nd6_if_ptr->retrans_timer_ms = fnet_ntohl(ra_packet->retrans_timer_ms);
        }

        /* RFC4861: Hosts SHOULD copy the option’s value
         * into LinkMTU so long as the value is greater than or equal to the
         * minimum link MTU [IPv6] and does not exceed the maximum LinkMTU value
         * specified in the link-type-specific document.
         */
        if(nd_option_mtu)
        {
            fnet_size_t mtu = fnet_ntohl(nd_option_mtu->mtu);

            if(mtu < netif->netif_mtu)
            {
                if(mtu < FNET_IP6_DEFAULT_MTU)
                {
                    netif->nd6_if_ptr->mtu = FNET_IP6_DEFAULT_MTU;
                }
                else
                {
                    netif->nd6_if_ptr->mtu =  mtu;
                }

#if FNET_CFG_IP6_PMTU_DISCOVERY
                if((netif->pmtu) /* If PMTU is enabled.*/ &&
                   (netif->pmtu > netif->nd6_if_ptr->mtu))
                {
                    _fnet_netif_set_pmtu(netif, netif->nd6_if_ptr->mtu);
                }
#endif
            }
        }

        /* RFC4861: If the advertisement contains a Source Link-Layer Address
         * option, the link-layer address SHOULD be recorded in the Neighbor
         * Cache entry for the router (creating an entry if necessary) and the
         * IsRouter flag in the Neighbor Cache entry MUST be set to TRUE. */
        neighbor_cache_entry = _fnet_nd6_neighbor_cache_get(netif, src_ip);
        if(nd_option_slla)
        {
            if(neighbor_cache_entry == FNET_NULL)
                /* Creating an entry if necessary */
            {
                neighbor_cache_entry = _fnet_nd6_neighbor_cache_add(netif, src_ip, nd_option_slla->addr, FNET_ND6_NEIGHBOR_STATE_STALE);
            }
            else
            {
                /* If a cache entry already exists and is
                 * updated with a different link-layer address, the reachability state
                 * MUST also be set to STALE.*/
                if( !FNET_NETIF_LL_ADDR_ARE_EQUAL(nd_option_slla->addr, neighbor_cache_entry->ll_addr, netif->netif_api->netif_hw_addr_size) )
                {
                    FNET_NETIF_LL_ADDR_COPY(nd_option_slla->addr, neighbor_cache_entry->ll_addr, netif->netif_api->netif_hw_addr_size);
                    neighbor_cache_entry->state = FNET_ND6_NEIGHBOR_STATE_STALE;
                }
            }

            /* Sends any packets queued for the neighbor awaiting address resolution.*/
            _fnet_nd6_neighbor_send_waiting_netbuf(netif, neighbor_cache_entry);
        }
        else
        {
            if(neighbor_cache_entry == FNET_NULL)
            {
                neighbor_cache_entry = _fnet_nd6_neighbor_cache_add(netif, src_ip, FNET_NULL, FNET_ND6_NEIGHBOR_STATE_STALE);
            }
        }

        /* RFC4861: If the address is already present in the host’s Default Router
         * List as a result of a previously received advertisement, reset
         * its invalidation timer to the Router Lifetime value in the newly
         * received advertisement.
         * If the address is already present in the host’s Default Router
         * List and the received Router Lifetime value is zero, immediately
         * time-out the entry.*/
        _fnet_nd6_router_list_add( neighbor_cache_entry, (fnet_time_t)fnet_ntohs(ra_packet->router_lifetime_sec));

        /*************************************************************
         * Prcess Prefix options.
         *************************************************************/
        if(prefix_index > 0u)
        {
            for(i = 0u; i < prefix_index; i++)
            {
                fnet_nd6_prefix_entry_t *prefix_entry;

#if FNET_CFG_DEBUG_IP6 && FNET_CFG_DEBUG
                fnet_char_t numaddr[FNET_IP6_ADDR_STR_SIZE] = {0};
                fnet_inet_ntop(AF_INET6, (fnet_uint8_t *)&nd_option_prefix[i]->prefix, numaddr, sizeof(numaddr));

                fnet_println("Prefix[%d]= %s \n", i, numaddr);
#endif /* FNET_CFG_DEBUG_IP6 */


                /* RFC4861: A router SHOULD NOT send a prefix
                 * option for the link-local prefix and a host SHOULD
                 * ignore such a prefix option.*/
                if( (!FNET_IP6_ADDR_IS_LINKLOCAL(&nd_option_prefix[i]->prefix))
                    /* RFC4861: The value of Prefered Lifetime field MUST NOT exceed
                     * the Valid Lifetime field to avoid preferring
                     * addresses that are no longer valid.*/
                    && (fnet_ntohl(nd_option_prefix[i]->prefered_lifetime) <= fnet_ntohl(nd_option_prefix[i]->valid_lifetime_sec)) )
                {
                    /*************************************************************
                     * Prefix Information option with the on-link flag set.
                     *************************************************************/
                    if( (nd_option_prefix[i]->flag & FNET_ND6_OPTION_FLAG_L) == FNET_ND6_OPTION_FLAG_L )
                    {
                        prefix_entry = _fnet_nd6_prefix_list_get(netif, &nd_option_prefix[i]->prefix);

                        /* RFC4861: If the prefix is not already present in the Prefix List, and the
                         * Prefix Information option’s Valid Lifetime field is non-zero,
                         * create a new entry for the prefix and initialize its
                         * invalidation timer to the Valid Lifetime value in the Prefix
                         * Information option.*/
                        if(prefix_entry == FNET_NULL)
                        {
                            if(fnet_ntohl(nd_option_prefix[i]->valid_lifetime_sec) != 0u)
                            {
                                /* Create a new entry for the prefix.*/
                                _fnet_nd6_prefix_list_add(netif, &nd_option_prefix[i]->prefix,
                                                          (fnet_size_t)nd_option_prefix[i]->prefix_length,
                                                          fnet_ntohl(nd_option_prefix[i]->valid_lifetime_sec));
                            }
                        }
                        else
                        {
                            /* RFC4861: If the prefix is already present in the host’s Prefix List as
                             * the result of a previously received advertisement, reset its
                             * invalidation timer to the Valid Lifetime value in the Prefix
                             * Information option. If the new Lifetime value is zero, time-out
                             * the prefix immediately.*/
                            if(fnet_ntohl(nd_option_prefix[i]->valid_lifetime_sec) != 0u)
                            {
                                /* Reset Timer. */
                                prefix_entry->lifetime_sec = fnet_ntohl(nd_option_prefix[i]->valid_lifetime_sec);
                                prefix_entry->creation_time_sec = fnet_timer_get_ms() / FNET_TIMER_MS_IN_SEC;
                            }
                            else
                            {
                                /* Time-out the prefix immediately. */
                                _fnet_nd6_prefix_list_del(prefix_entry);
                            }
                        }
                    }

                    /*************************************************************
                     * Stateless Address Autconfiguration.
                     *************************************************************/

                    /* For each Prefix-Information option in the Router Advertisement:*/
                    if( ((nd_option_prefix[i]->flag & FNET_ND6_OPTION_FLAG_A) == FNET_ND6_OPTION_FLAG_A )
                        && (fnet_ntohl(nd_option_prefix[i]->valid_lifetime_sec) != 0u) )
                    {
                        fnet_index_t            j;
                        fnet_netif_ip6_addr_t   *addr_info = FNET_NULL;

                        /* RFC4862 5.5.3:If the advertised prefix is equal to the prefix of an address
                         * configured by stateless autoconfiguration in the list, the
                         * preferred lifetime of the address is reset to the Preferred
                         * Lifetime in the received advertisement. */

                        /* Lookup the address */
                        for(j = 0u; j < FNET_NETIF_IP6_ADDR_MAX; j++)
                        {
                            if((netif->ip6_addr[j].state != FNET_NETIF_IP6_ADDR_STATE_NOT_USED)
                               && (netif->ip6_addr[j].type == FNET_NETIF_IP_ADDR_TYPE_AUTOCONFIGURABLE)
                               && (netif->ip6_addr[j].prefix_length == nd_option_prefix[i]->prefix_length)
                               && (_fnet_ip6_addr_pefix_cmp(&nd_option_prefix[i]->prefix, &netif->ip6_addr[j].address, (fnet_size_t)nd_option_prefix[i]->prefix_length) == FNET_TRUE) )
                            {
                                addr_info = &netif->ip6_addr[j];
                                break;
                            }
                        }

                        if( addr_info != FNET_NULL)
                        {
                            /* RFC4862 5.5.3: The specific action to
                             * perform for the valid lifetime of the address depends on the Valid
                             * Lifetime in the received advertisement and the remaining time to
                             * the valid lifetime expiration of the previously autoconfigured
                             * address. We call the remaining time "RemainingLifetime" in the
                             * following discussion:*/
                            if( (fnet_ntohl(nd_option_prefix[i]->valid_lifetime_sec) > (60u * 60u * 2u) /* 2 hours */)
                                || ( fnet_ntohl(nd_option_prefix[i]->valid_lifetime_sec) >  ((addr_info->creation_time_sec + addr_info->lifetime_sec) - (fnet_timer_get_ms() / FNET_TIMER_MS_IN_SEC)) ) )
                            {
                                /* 1. If the received Valid Lifetime is greater than 2 hours or
                                 *    greater than RemainingLifetime, set the valid lifetime of the
                                 *    corresponding address to the advertised Valid Lifetime. */
                                addr_info->lifetime_sec = fnet_ntohl(nd_option_prefix[i]->valid_lifetime_sec);

                            }
                            else
                            {
                                /* 2. If RemainingLifetime is less than or equal to 2 hours, ignore
                                 *    the Prefix Information option with regards to the valid
                                 *    lifetime, unless the Router Advertisement from which this
                                 *    option was obtained has been authenticated (e.g., via Secure
                                 *    Neighbor Discovery [RFC3971]). If the Router Advertisement
                                 *    was authenticated, the valid lifetime of the corresponding
                                 *    address should be set to the Valid Lifetime in the received
                                 *    option.
                                 * 3. Otherwise, reset the valid lifetime of the corresponding
                                 *    address to 2 hours. */
                                addr_info->lifetime_sec = (60u * 60u * 2u) /* 2 hours */;
                            }
                            addr_info->creation_time_sec = fnet_timer_get_ms() / FNET_TIMER_MS_IN_SEC;
                        }
                        else
                        {
                            /* RFC4862 5.5.3: If the prefix advertised is not equal to the prefix of an
                             * address configured by stateless autoconfiguration already in the
                             * list of addresses associated with the interface (where "equal"
                             * means the two prefix lengths are the same and the first prefixlength
                             * bits of the prefixes are identical), and if the Valid
                             * Lifetime is not 0, form an address (and add it to the list) by
                             * combining the advertised prefix with an interface identifier. */
                            _fnet_netif_bind_ip6_addr (netif, &nd_option_prefix[i]->prefix, FNET_NETIF_IP_ADDR_TYPE_AUTOCONFIGURABLE,
                                                       fnet_ntohl(nd_option_prefix[i]->valid_lifetime_sec), (fnet_size_t)nd_option_prefix[i]->prefix_length);
                        }
                    }
                    /* else. RFC4862: If the Autonomous flag is not set, silently ignore the Prefix Information option.*/
                }
            } /* for() end. */
        } /* End processing prefix options.*/

    }

DROP:
    _fnet_netbuf_free_chain(ip6_nb);
    _fnet_netbuf_free_chain(nb);
}

/************************************************************************
* DESCRIPTION: Handles received Redirect message.
*************************************************************************/
void _fnet_nd6_redirect_receive(struct fnet_netif *netif, fnet_ip6_addr_t *src_ip, fnet_ip6_addr_t *dest_ip, fnet_netbuf_t *nb, fnet_netbuf_t *ip6_nb)
{
    fnet_icmp6_header_t             *icmp6_packet = (fnet_icmp6_header_t *)nb->data_ptr;
    fnet_size_t                     icmp6_packet_size = nb->total_length;
    fnet_nd6_rd_header_t            *rd_packet = (fnet_nd6_rd_header_t *)icmp6_packet;
    fnet_nd6_option_lla_header_t    *nd_option_tlla = FNET_NULL;
    fnet_size_t                     nd_option_offset;
    fnet_ip6_header_t               *ip6_packet = (fnet_ip6_header_t *)ip6_nb->data_ptr;
    fnet_nd6_neighbor_entry_t       *neighbor_cache_entry;
    fnet_ip6_addr_t                *target_addr = &rd_packet->target_addr;
    fnet_ip6_addr_t                *destination_addr = &rd_packet->destination_addr;


    FNET_COMP_UNUSED_ARG(dest_ip);

    /************************************************************
    * Validation of Redirect Message RFC4861 (8.1).
    *************************************************************/
    if(
        (icmp6_packet_size < sizeof(fnet_nd6_rd_header_t))
        || (ip6_packet->hop_limit != FNET_ND6_HOP_LIMIT)     /* The IP Hop Limit field has a value of 255.*/
        || (icmp6_packet->code != 0u)                        /* ICMP Code is 0.*/
        || (!FNET_IP6_ADDR_IS_LINKLOCAL(src_ip))             /* IP Source Address is a link-local address.*/
        /* The ICMP Destination Address field in the redirect message does
         * not contain a multicast address.*/
        || (FNET_IP6_ADDR_IS_MULTICAST(destination_addr))
        /* The ICMP Target Address is either a link-local address (when
         * redirected to a router) or the same as the ICMP Destination
         * Address (when redirected to the on-link destination). */
        || ((!FNET_IP6_ADDR_IS_LINKLOCAL(target_addr))
            && (!FNET_IP6_ADDR_EQUAL(destination_addr, target_addr)))
        /* The IP source address of the Redirect is the same as the current
         * first-hop router for the specified ICMP Destination Address.*/
        || (_fnet_nd6_is_firsthop_router(netif, src_ip) == FNET_FALSE)
    )
    {
        goto DROP;
    }


    /************************************************************
     * Handle posible options.
     ************************************************************
     * The contents of any defined options that are not specified
     * to be used with Redirect messages MUST be
     * ignored and the packet processed as normal. The only defined
     * options that may appear are the Target Link-Layer Address,
     * Prefix Information and MTU options.
     ************************************************************/
    nd_option_offset = sizeof(fnet_nd6_rd_header_t);
    while(icmp6_packet_size > nd_option_offset + sizeof(fnet_nd6_option_header_t))
    {
        fnet_nd6_option_header_t *nd_option;

        nd_option =  (fnet_nd6_option_header_t *) ((fnet_uint8_t *)icmp6_packet + nd_option_offset) ;
        /* Validation RFC4861 (8.1). All included options have a length that is greater than zero.
         */
        if(nd_option->length == 0u)
        {
            goto DROP;
        }

        /* Ensure the full option body (length * 8 bytes) lies within the
         * received packet before any option field is dereferenced. */
        if((nd_option_offset + ((fnet_size_t)nd_option->length << 3)) > icmp6_packet_size)
        {
            goto DROP;
        }

        /* The advertisement is scanned for valid options.
         */

        /* Target Link-Layer Address option.*/
        if( (nd_option->type == FNET_ND6_OPTION_TARGET_LLA)
            && ( ((fnet_size_t)nd_option->length << 3) >= (netif->netif_api->netif_hw_addr_size + sizeof(fnet_nd6_option_header_t))) )
        {
            nd_option_tlla = (fnet_nd6_option_lla_header_t *)nd_option; /* Target Link-layer Address option is found.*/
        }
        /* else, silently ignore any options they do not recognize
         * and continue processing the message.
         */

        nd_option_offset += ((fnet_size_t)nd_option->length << 3);
    }

    /* RFC4861: If the redirect contains a Target Link-Layer Address option, the host
     * either creates or updates the Neighbor Cache entry for the target. */
    neighbor_cache_entry = _fnet_nd6_neighbor_cache_get(netif, target_addr);
    if(nd_option_tlla)
    {
        if(neighbor_cache_entry == FNET_NULL)
        {
            /*  If a Neighbor Cache entry is
             * created for the target, its reachability state MUST be set to STALE
             * as specified in Section 7.3.3. */
            neighbor_cache_entry = _fnet_nd6_neighbor_cache_add(netif, target_addr, nd_option_tlla->addr, FNET_ND6_NEIGHBOR_STATE_STALE);
        }
        else
        {
            /* If a cache entry already existed and
             * it is updated with a different link-layer address, its reachability
             * state MUST also be set to STALE. */
            if( !FNET_NETIF_LL_ADDR_ARE_EQUAL(nd_option_tlla->addr, neighbor_cache_entry->ll_addr, netif->netif_api->netif_hw_addr_size) )
            {
                FNET_NETIF_LL_ADDR_COPY(nd_option_tlla->addr, neighbor_cache_entry->ll_addr, netif->netif_api->netif_hw_addr_size);
                neighbor_cache_entry->state = FNET_ND6_NEIGHBOR_STATE_STALE;
            }
            /* else. If the link-layer address is the
             * same as that already in the cache, the cache entry’s state remains
             * unchanged. */
        }

        /* Sends any packets queued for the neighbor awaiting address resolution.*/
        _fnet_nd6_neighbor_send_waiting_netbuf(netif, neighbor_cache_entry);
    }
    else
    {
        if(neighbor_cache_entry == FNET_NULL)
        {
            /*  If a Neighbor Cache entry is
             * created for the target, its reachability state MUST be set to STALE
             * as specified in Section 7.3.3. */
            neighbor_cache_entry = _fnet_nd6_neighbor_cache_add(netif, target_addr, FNET_NULL, FNET_ND6_NEIGHBOR_STATE_STALE);
        }
    }

    /* If the Target Address is not the same
     * as the Destination Address, the host MUST set IsRouter to TRUE for
     * the target.*/
    if( !FNET_IP6_ADDR_EQUAL(destination_addr, target_addr) )
    {
        neighbor_cache_entry->is_router = FNET_TRUE;

        /* Add to redirect table.*/
        _fnet_nd6_redirect_table_add(netif, destination_addr, target_addr);
    }

DROP:
    _fnet_netbuf_free_chain(ip6_nb);
    _fnet_netbuf_free_chain(nb);
}

/************************************************************************
* DESCRIPTION: Start the Router Discovery for the interface.
*************************************************************************/
void _fnet_nd6_rd_start(struct fnet_netif *netif)
{
    if(netif->nd6_if_ptr)
    {
        netif->nd6_if_ptr->rd_transmit_counter = FNET_ND6_MAX_RTR_SOLICITATIONS - 1u;
        netif->nd6_if_ptr->rd_time_ms = fnet_timer_get_ms();  /* Save send time.*/
        _fnet_nd6_router_solicitation_send(netif);
    }
    /* TBD Randomise first send.*/
}

/************************************************************************
* DESCRIPTION: Timer routine used by Router Discovery.
*************************************************************************/
static void _fnet_nd6_rd_timer(fnet_netif_t *netif)
{
    if(netif->nd6_if_ptr->rd_transmit_counter > 0u)
    {
        if(_fnet_nd6_default_router_get(netif) == FNET_NULL)
            /* Router is not found yet.*/
        {
            if((fnet_timer_get_ms() - netif->nd6_if_ptr->rd_time_ms) > FNET_ND6_RTR_SOLICITATION_INTERVAL_MS)
            {
                netif->nd6_if_ptr->rd_transmit_counter--;
                netif->nd6_if_ptr->rd_time_ms = fnet_timer_get_ms();  /* Save send time.*/
                _fnet_nd6_router_solicitation_send(netif);
            }
        }
        else
            /* Router is found.*/
        {
#if FNET_CFG_DEBUG_IP6 && FNET_CFG_DEBUG
            fnet_println("RD: ROUTER is FOUND!!\n");
#endif
            netif->nd6_if_ptr->rd_transmit_counter = 0u; /* Stop timer.*/
        }
    }
}

/************************************************************************
* DESCRIPTION: Start the Duplicate Address Detection for the address.
*************************************************************************/
void _fnet_nd6_dad_start(struct fnet_netif *netif, struct fnet_netif_ip6_addr *addr_info)
{
#if FNET_CFG_ND6_DAD_TRANSMITS > 0u
    if(addr_info && (addr_info->state == FNET_NETIF_IP6_ADDR_STATE_TENTATIVE))
    {
        /* To check an address, a node sends DAD Neighbor
         * Solicitations, each separated by 1 second(TBD)..
         */
        addr_info->dad_transmit_counter = FNET_CFG_ND6_DAD_TRANSMITS;
        addr_info->state_time_ms = fnet_timer_get_ms();  /* Save state time.*/
        _fnet_nd6_neighbor_solicitation_send(netif, FNET_NULL, FNET_NULL, &addr_info->address);
    }
#endif /* FNET_CFG_ND6_DAD_TRANSMITS */
}

/************************************************************************
* DESCRIPTION: Timer routine used by Duplicate Address Detection.
*************************************************************************/
static void _fnet_nd6_dad_timer(fnet_netif_t *netif )
{
#if FNET_CFG_ND6_DAD_TRANSMITS > 0u
    fnet_index_t            i;
    fnet_netif_ip6_addr_t   *addr_info;

    for(i = 0u; i < FNET_NETIF_IP6_ADDR_MAX; i++)
    {
        addr_info = &netif->ip6_addr[i];

        if(addr_info->state == FNET_NETIF_IP6_ADDR_STATE_TENTATIVE)
        {
            if( (fnet_timer_get_ms() - addr_info->state_time_ms) > netif->nd6_if_ptr->retrans_timer_ms )
            {
                addr_info->dad_transmit_counter--;
                if(addr_info->dad_transmit_counter == 0u)
                {
                    /* DAD succeeded, for this address.
                     * Once an address is determined to be unique,
                     * it may be assigned to an interface.*/
                    addr_info->state = FNET_NETIF_IP6_ADDR_STATE_PREFERRED;

#if FNET_CFG_MLD
                    /* [RFC3590] Once a valid link-local address is available, a node SHOULD generate
                     * new MLD Report messages for all multicast addresses joined on the interface.*/
                    if(netif->mld_invalid == FNET_TRUE)
                    {
                        _fnet_mld_report_all(netif);
                    }
#endif

#if FNET_CFG_DEBUG_IP6 && FNET_CFG_DEBUG
                    {
                        fnet_char_t numaddr[FNET_IP6_ADDR_STR_SIZE] = {0};
                        fnet_inet_ntop(AF_INET6, (fnet_uint8_t *)&addr_info->address, numaddr, sizeof(numaddr));
                        fnet_println("%s is PREFERED NOW\n", numaddr);
                    }
#endif
                }
                else
                {
                    addr_info->state_time_ms = fnet_timer_get_ms();
                    _fnet_nd6_neighbor_solicitation_send(netif, FNET_NULL, FNET_NULL, &addr_info->address);
                }
            }
        }
    }
#endif /* FNET_CFG_ND6_DAD_TRANSMITS */
}

/************************************************************************
* DESCRIPTION: Called when DAD is failed.
*************************************************************************/
static void _fnet_nd6_dad_failed(fnet_netif_t *netif, fnet_netif_ip6_addr_t *addr_info)
{
    fnet_ip6_addr_t             if_ip6_address;

#if FNET_CFG_DEBUG_IP6 && FNET_CFG_DEBUG
    {
        fnet_char_t numaddr[FNET_IP6_ADDR_STR_SIZE] = {0};
        fnet_inet_ntop(AF_INET6, (fnet_uint8_t *)&addr_info->address, numaddr, sizeof(numaddr));
        fnet_println("%s DAD is FAILED!!!!\n", numaddr);
    }
#endif
    /* RFC 4862: */
    /* 5.4.5. When Duplicate Address Detection Fails */
    /* Just remove address, or TBD mark it as dupicate.*/
    _fnet_netif_unbind_ip6_addr( netif, addr_info);

    /* If the address is a link-local address formed from an interface
     * identifier based on the hardware address, which is supposed to be
     * uniquely assigned (e.g., EUI-64 for an Ethernet interface), IP
     * operation on the interface SHOULD be disabled.
     * In this case, the IP address duplication probably means duplicate
     * hardware addresses are in use, and trying to recover from it by
     * configuring another IP address will not result in a usable network.*/
    fnet_memset_zero(&if_ip6_address.addr[0], sizeof(fnet_ip6_addr_t));
    if_ip6_address.addr[0] = 0xFEu;
    if_ip6_address.addr[1] = 0x80u;
    _fnet_netif_set_ip6_addr_autoconf(netif, &if_ip6_address);

    if(FNET_IP6_ADDR_EQUAL(&if_ip6_address, &addr_info->address))
    {
        netif->nd6_if_ptr->ip6_disabled = FNET_TRUE;
    }
}

#if FNET_CFG_ND6_RDNSS && FNET_CFG_DNS
/************************************************************************
* DESCRIPTION: Update entry in the RDNSS List.
*************************************************************************/
static void _fnet_nd6_rdnss_list_update(fnet_netif_t *if_ptr, const fnet_ip6_addr_t *rdnss_addr, fnet_time_t lifetime_sec)
{
    struct fnet_nd6_if      *nd6_if = if_ptr->nd6_if_ptr;
    fnet_index_t            i;
    fnet_nd6_rdnss_entry_t  *entry = FNET_NULL;

    if (nd6_if)
    {
        /* RFC6106: For each RDNSS address, if it already exists in the DNS
        * Server List, then just update the value of the Expiration-time field.*/
        for(i = 0u; i < FNET_CFG_ND6_RDNSS_LIST_SIZE; i++)
        {
            if( (nd6_if->rdnss_list[i].lifetime_sec != 0u) &&
                FNET_IP6_ADDR_EQUAL(&nd6_if->rdnss_list[i].rdnss_addr, rdnss_addr))
            {
                entry = &nd6_if->rdnss_list[i];
                break;
            }
        }

        /* If no existing entry is found. */
        if((entry == FNET_NULL) && lifetime_sec)
        {
            /* Find an unused entry in the RNDSS List. */
            for(i = 0u; i < FNET_CFG_ND6_RDNSS_LIST_SIZE; i++)
            {
                if(nd6_if->rdnss_list[i].lifetime_sec == 0u)
                {
                    entry = &nd6_if->rdnss_list[i];
                    break;
                }
            }

            /* If no free entry is found. */
            if(entry == FNET_NULL)
            {
                /* RFC 6106: In the case where the data structure for the
                 * DNS Server List is full of RDNSS entries (that is, has more
                 * RDNSSes than the sufficient number discussed in Section 5.3.1),
                 * delete from the DNS Server List the entry with the shortest
                 * Expiration-time (i.e., the entry that will expire first).*/

                entry = &nd6_if->rdnss_list[0];

                for(i = 1u; i < FNET_CFG_ND6_RDNSS_LIST_SIZE; i++)
                {
                    if( (entry->lifetime_sec == FNET_ND6_RDNSS_LIFETIME_INFINITE)
                        || ((nd6_if->rdnss_list[i].lifetime_sec != FNET_ND6_RDNSS_LIFETIME_INFINITE)
                            && ((nd6_if->rdnss_list[i].lifetime_sec - ((fnet_timer_get_ms() / FNET_TIMER_MS_IN_SEC) - nd6_if->rdnss_list[i].creation_time_sec))
                                < (entry->lifetime_sec - ((fnet_timer_get_ms() / FNET_TIMER_MS_IN_SEC) - entry->creation_time_sec))) )
                      )
                    {
                        entry = &nd6_if->rdnss_list[i];
                    }
                }
            }
        }

        if(entry)
        {
            /* Fill the entry parameters.*/
            FNET_IP6_ADDR_COPY(rdnss_addr, &entry->rdnss_addr);
            entry->lifetime_sec = lifetime_sec;
            entry->creation_time_sec = fnet_timer_get_ms() / FNET_TIMER_MS_IN_SEC;
        }
    }
}

/************************************************************************
* DESCRIPTION: Deletes an entry from the RDNSS List.
*************************************************************************/
static void _fnet_nd6_rdnss_list_del(fnet_nd6_rdnss_entry_t *rdnss_entry)
{
    if (rdnss_entry)
    {
        rdnss_entry->lifetime_sec = 0u;
    }
}

/************************************************************************
* DESCRIPTION: This function returns a RDNS Server address.
*************************************************************************/
fnet_bool_t _fnet_nd6_rdnss_get_addr(struct fnet_netif *netif, fnet_index_t n, fnet_ip6_addr_t *addr_dns )
{
    fnet_bool_t         result = FNET_FALSE;
    struct fnet_nd6_if  *nd6_if = netif->nd6_if_ptr;
    fnet_index_t        i;

    if(nd6_if)
    {
        for(i = 0u; i < FNET_CFG_ND6_RDNSS_LIST_SIZE; i++)
        {
            /* Skip NOT_USED addresses. */
            if(nd6_if->rdnss_list[i].lifetime_sec != 0u)
            {
                if(n == 0u)
                {
                    FNET_IP6_ADDR_COPY(&nd6_if->rdnss_list[i].rdnss_addr, addr_dns);
                    result = FNET_TRUE;
                    break;
                }
                n--;
            }
        }
    }

    return result;
}

/************************************************************************
* DESCRIPTION: Timer routine used for RDNSS lifetime check.
*************************************************************************/
static void _fnet_nd6_rdnss_timer(fnet_netif_t *netif)
{
    fnet_nd6_if_t   *nd6_if = netif->nd6_if_ptr;
    fnet_index_t    i;

    /* Check lifetime for prefixes.*/
    for(i = 0u; i < FNET_CFG_ND6_RDNSS_LIST_SIZE; i++)
    {
        /* RFC 6106: Whenever an entry
        * expires in the DNS Server List, the expired entry is deleted from the
        * DNS Server List */
        if((nd6_if->rdnss_list[i].lifetime_sec != 0u)
           && (nd6_if->rdnss_list[i].lifetime_sec != FNET_ND6_RDNSS_LIFETIME_INFINITE)
           && (((fnet_timer_get_ms() / FNET_TIMER_MS_IN_SEC) - nd6_if->rdnss_list[i].creation_time_sec) > nd6_if->rdnss_list[i].lifetime_sec) )
        {
            _fnet_nd6_rdnss_list_del(&nd6_if->rdnss_list[i]);
        }
    }
}
#endif /* FNET_CFG_ND6_RDNSS && FNET_CFG_DNS */



/*=================== For DEBUG needs only. =============================*/

/************************************************************************
* DESCRIPTION: Prints prefix list. For DEBUG needs only.
*************************************************************************/
void _fnet_nd6_debug_print_prefix_list( struct fnet_netif *netif )
{
    fnet_nd6_if_t           *nd6_if;
    fnet_index_t            i;
    fnet_nd6_prefix_entry_t *entry ;

    if(netif == FNET_NULL)
    {
        netif = _fnet_netif_get_by_number(0u); /* Get the first one.*/
    }

    if(netif)
    {
        nd6_if = netif->nd6_if_ptr;

        if (nd6_if)
        {
            fnet_println("Prefix List:");
            for(i = 0u; i < FNET_ND6_PREFIX_LIST_SIZE; i++)
            {
                if(nd6_if->prefix_list[i].state != FNET_ND6_PREFIX_STATE_NOTUSED)
                {
                    fnet_char_t numaddr[FNET_IP6_ADDR_STR_SIZE] = {0};
                    entry = &nd6_if->prefix_list[i];

                    fnet_inet_ntop(AF_INET6, (fnet_uint8_t *)entry->prefix.addr, numaddr, sizeof(numaddr));

                    /* Print entry.*/
                    fnet_println("[%d] %s /%d State(%d) LifeTime(%d)[%d]", i, numaddr, entry->prefix_length, entry->state,  entry->lifetime_sec, entry->creation_time_sec);
                }
            }

        }
    }
}

#endif /* FNET_CFG_IP6 */
