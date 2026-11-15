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
* ARP protocol implementation.
*
***************************************************************************/

#include "fnet.h"

#if FNET_CFG_IP4

#include "fnet_stack_prv.h"
#include "fnet_arp_prv.h"
#include "fnet_eth_prv.h"
#include "fnet_netif_prv.h"

#if FNET_CFG_DEBUG_ARP && FNET_CFG_DEBUG
#define FNET_DEBUG_ARP FNET_DEBUG
#else
#define FNET_DEBUG_ARP(...) \
    do                      \
    {                       \
    } while (0)
#endif

/************************************************************************
*     Function Prototypes
*************************************************************************/
#if FNET_CFG_ARP_EXPIRE_TIMEOUT
    static void _fnet_arp_timer(fnet_uint32_t cookie);
#endif

static fnet_arp_entry_t *_fnet_arp_add_entry(fnet_netif_t *netif, fnet_ip4_addr_t ipaddr, const fnet_mac_addr_t ethaddr);
static fnet_arp_entry_t *_fnet_arp_update_entry(fnet_netif_t *netif, fnet_ip4_addr_t ipaddr, fnet_mac_addr_t ethaddr);
static void _fnet_arp_ip4_addr_conflict(void *cookie);

#if FNET_CFG_DEBUG_TRACE_ARP && FNET_CFG_DEBUG_TRACE
static void fnet_arp_trace(const fnet_char_t *str, fnet_arp_header_t *arp_hdr);
#else
#define fnet_arp_trace(str, arp_hdr) \
    do                               \
    {                                \
    } while (0)
#endif

/************************************************************************
* DESCRIPTION: ARP module initialization.
*************************************************************************/
fnet_return_t _fnet_arp_init(fnet_netif_t *netif, fnet_arp_if_t *arpif)
{
    fnet_index_t    i;
    fnet_return_t   result = FNET_ERR;

    if(netif && arpif)
    {
        netif->arp_if_ptr = arpif;

        for (i = 0U; i < FNET_CFG_ARP_TABLE_SIZE; i++)
        {
            fnet_memset_zero(&(arpif->arp_table[i]), sizeof(fnet_arp_entry_t));
        }

#if FNET_CFG_ARP_EXPIRE_TIMEOUT
        arpif->arp_tmr = _fnet_timer_new(FNET_ARP_TIMER_PERIOD_MS, _fnet_arp_timer, (fnet_uint32_t)arpif);
#endif

        if (arpif->arp_tmr)
        {
            /* Install event Handler. */
            arpif->arp_event = fnet_event_init(_fnet_arp_ip4_addr_conflict, netif);
            if (arpif->arp_event)
            {
                result = FNET_OK;
            }
        }
    }

    return result;
}

/************************************************************************
* DESCRIPTION: ARP module release.
*************************************************************************/
void _fnet_arp_release(fnet_netif_t *netif)
{
    fnet_arp_if_t *arpif = netif->arp_if_ptr;

    if(arpif)
    {
        _fnet_timer_free(arpif->arp_tmr);

        arpif->arp_tmr = 0;
    }
}

/************************************************************************
* DESCRIPTION: ARP aging timer.
*************************************************************************/
#if FNET_CFG_ARP_EXPIRE_TIMEOUT
static void _fnet_arp_timer(fnet_uint32_t cookie)
{
    fnet_arp_if_t *arpif = (fnet_arp_if_t *)cookie;
    fnet_index_t i;

    for (i = 0U; i < FNET_CFG_ARP_TABLE_SIZE; i++)
    {
        if ((arpif->arp_table[i].prot_addr) && (((fnet_timer_get_ms() - arpif->arp_table[i].cr_time_ms)) > (FNET_CFG_ARP_EXPIRE_TIMEOUT * 1000U)))
        {
            if (arpif->arp_table[i].hold)
            {
                _fnet_netbuf_free_chain(arpif->arp_table[i].hold);
            }

            fnet_memset_zero(&(arpif->arp_table[i]), sizeof(fnet_arp_entry_t));
        }
    }
}
#endif

/************************************************************************
* DESCRIPTION: Adds entry to the ARP table.
*************************************************************************/
static fnet_arp_entry_t *_fnet_arp_add_entry(fnet_netif_t *netif, fnet_ip4_addr_t ipaddr, const fnet_mac_addr_t ethaddr)
{
    fnet_arp_if_t *arpif = netif->arp_if_ptr;
    fnet_index_t i;
    fnet_index_t j;
    fnet_time_t max_time;

    /* Find an entry to update. */
    for (i = 0U; i < FNET_CFG_ARP_TABLE_SIZE; ++i)
    {
        /* Check if the source IP address of the incoming packet matches
         * the IP address in this ARP table entry.*/
        if (ipaddr == arpif->arp_table[i].prot_addr)
        {
            /* Update this and return. */
            fnet_memcpy(arpif->arp_table[i].hard_addr, ethaddr, sizeof(fnet_mac_addr_t));
            arpif->arp_table[i].cr_time_ms = fnet_timer_get_ms();
            goto ADDED;
        }
    }

    /* If we get here, no existing ARP table entry was found. */

    /* Find an unused entry in the ARP table. */
    for (i = 0U; i < FNET_CFG_ARP_TABLE_SIZE; ++i)
    {
        if (arpif->arp_table[i].prot_addr == 0U)
        {
            break;
        }
    }

    /* If no unused entry is found, we try to find the oldest entry and throw it
     * away.*/
    if (i == FNET_CFG_ARP_TABLE_SIZE)
    {
        max_time = 0U;
        j        = 0U;

        for (i = 0U; i < FNET_CFG_ARP_TABLE_SIZE; ++i)
        {
            if ((fnet_timer_get_ms() - arpif->arp_table[i].cr_time_ms) > max_time)
            {
                max_time = fnet_timer_get_ms() - arpif->arp_table[i].cr_time_ms;
                j = i;
            }
        }

        i = j;
    }

    /* Now, it is the ARP table entry which we will fill with the new information.
     */
    if (arpif->arp_table[i].hold)
    {
        _fnet_netbuf_free_chain(arpif->arp_table[i].hold);
        arpif->arp_table[i].hold      = 0;
        arpif->arp_table[i].hold_time_ms = 0U;
    }

    arpif->arp_table[i].prot_addr = ipaddr;
    fnet_memcpy(arpif->arp_table[i].hard_addr, ethaddr, sizeof(fnet_mac_addr_t));

    arpif->arp_table[i].cr_time_ms = fnet_timer_get_ms();
ADDED:
    return (&arpif->arp_table[i]);
}

/************************************************************************
* DESCRIPTION: Upates ARP table.
*************************************************************************/
static fnet_arp_entry_t *_fnet_arp_update_entry(fnet_netif_t *netif, fnet_ip4_addr_t ipaddr, fnet_mac_addr_t ethaddr)
{
    fnet_arp_if_t   *arpif = netif->arp_if_ptr;
    fnet_index_t    i;

    /* Find an entry to update. */
    for (i = 0U; i < FNET_CFG_ARP_TABLE_SIZE; ++i)
    {
        /* Check if the source IP address of the incoming packet matches
         * the IP address in this ARP table entry.*/
        if (ipaddr == arpif->arp_table[i].prot_addr)
        {
            /* Update this and return. */
            fnet_memcpy(arpif->arp_table[i].hard_addr, ethaddr, sizeof(fnet_mac_addr_t));
            arpif->arp_table[i].cr_time_ms = fnet_timer_get_ms();
            return (&arpif->arp_table[i]);
        }
    }

    return FNET_NULL;
}

/************************************************************************
* DESCRIPTION: Gets MAC address of valid ARP cache entry.
*************************************************************************/
fnet_bool_t fnet_arp_get_mac( fnet_netif_desc_t netif_desc, fnet_ip4_addr_t ip_addr, fnet_mac_addr_t mac_addr)
{
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;
    fnet_arp_if_t   *arpif;
    fnet_mac_addr_t *macaddr_p;
    fnet_bool_t     result = FNET_FALSE;

    if(netif)
    {
        arpif = netif->arp_if_ptr;
        if(arpif)
        {
            fnet_isr_lock();

            macaddr_p = _fnet_arp_lookup(netif, ip_addr);

            if(macaddr_p)
            {
                if(mac_addr)
                {
                    fnet_memcpy (mac_addr, *macaddr_p, sizeof(fnet_mac_addr_t));
                }
                result = FNET_TRUE;
            }

            fnet_isr_unlock();
        }
    }
    return result;
}

/************************************************************************
* DESCRIPTION: This function looks up an entry corresponding to
*              the destination IP address
*************************************************************************/
fnet_mac_addr_t *_fnet_arp_lookup(fnet_netif_t *netif, fnet_ip4_addr_t ipaddr)
{
    fnet_arp_if_t   *arpif = netif->arp_if_ptr; /* PFI */
    fnet_index_t    i;
    fnet_mac_addr_t *result = FNET_NULL;

    /* Find an entry. */
    for (i = 0U; i < FNET_CFG_ARP_TABLE_SIZE; ++i)
    {
        if (ipaddr == arpif->arp_table[i].prot_addr)
        {
            if (fnet_memcmp(arpif->arp_table[i].hard_addr, fnet_eth_null_addr, sizeof(fnet_mac_addr_t)) != 0)
            {
                result = &arpif->arp_table[i].hard_addr;
            }
            /* Else => not found */
            break;
        }
    }

    return result;
}

/************************************************************************
* DESCRIPTION: This function finds the first unused or the oldest
*              ARP table entry and makes a new entry
*              to prepare it for an ARP reply.
*************************************************************************/
void _fnet_arp_resolve(fnet_netif_t *netif, fnet_ip4_addr_t ipaddr, fnet_netbuf_t *nb)
{
    fnet_arp_if_t       *arpif = netif->arp_if_ptr;
    fnet_index_t        i;
    fnet_arp_entry_t    *entry;
    fnet_bool_t         to_send_request = FNET_FALSE;

    fnet_isr_lock();
    for (i = 0U; i < FNET_CFG_ARP_TABLE_SIZE; i++)
    {
        if (ipaddr == arpif->arp_table[i].prot_addr)
        {
            break;
        }
    }

    /* If no unused entry is found, create it. */
    if (i == FNET_CFG_ARP_TABLE_SIZE)
    {
        entry = _fnet_arp_add_entry(netif, ipaddr, fnet_eth_null_addr);
    }
    else
    {
        entry = &arpif->arp_table[i];
    }

    if ((i == FNET_CFG_ARP_TABLE_SIZE) || ((entry->hold) && ((fnet_timer_get_ms() - entry->hold_time_ms) > 1000U)) || (!entry->hold))
    {
        to_send_request = FNET_TRUE;
    }

    if (entry->hold)
    {
        _fnet_netbuf_free_chain(entry->hold);
        entry->hold = FNET_NULL;
    }
    entry->hold = nb;

    if(to_send_request == FNET_TRUE)
    {
        entry->hold_time_ms = fnet_timer_get_ms();
        _fnet_arp_send_request(netif, ipaddr);
    }

    fnet_isr_unlock();
}

/************************************************************************
* DESCRIPTION: ARP input function.
*************************************************************************/
void _fnet_arp_input(fnet_netif_t *netif, fnet_netbuf_t *nb)
{
    fnet_arp_if_t       *arpif = netif->arp_if_ptr;
    fnet_arp_header_t   *arp_hdr = (fnet_arp_header_t *)nb->data_ptr;
    fnet_mac_addr_t     local_addr;
    fnet_arp_entry_t    *entry;

    if (!((nb == 0) /* The packet is wrong. */
          || (nb->total_length < sizeof(fnet_arp_header_t))
          || (arp_hdr->hard_type != FNET_HTONS(FNET_ARP_HARD_TYPE))
          || (arp_hdr->hard_size != FNET_ARP_HARD_SIZE)
          || (arp_hdr->prot_type != FNET_HTONS(FNET_ETH_TYPE_IP4))
          || (arp_hdr->prot_size != FNET_ARP_PROT_SIZE)))
    {
        if (nb->total_length > sizeof(fnet_arp_header_t))
        {
            /* Logical size and the physical size of the packet should be the same.*/
            _fnet_netbuf_trim(&nb, (fnet_int32_t)sizeof(fnet_arp_header_t) - (fnet_int32_t)nb->total_length);
        }

        fnet_arp_trace("RX", arp_hdr); /* Print ARP header. */

        if(_fnet_netif_get_hw_addr(netif, local_addr, sizeof(fnet_mac_addr_t)) != FNET_OK)
        {
            goto DROP;
        }

        if (!((fnet_memcmp(arp_hdr->sender_hard_addr, local_addr, sizeof(fnet_mac_addr_t)) == 0)             /* It's from me => ignore it.*/
              || (fnet_memcmp(arp_hdr->sender_hard_addr, fnet_eth_broadcast, sizeof(fnet_mac_addr_t)) == 0)) /* It's broadcast=> error. */
           )
        {
            fnet_ip4_addr_t sender_prot_addr = arp_hdr->sender_prot_addr;
            fnet_ip4_addr_t targer_prot_addr = arp_hdr->targer_prot_addr;

            if (sender_prot_addr != netif->ip4_addr.address) /* Check Duplicate IP address.*/
            {
                if (targer_prot_addr == netif->ip4_addr.address) /* It's for me.*/
                {
                    entry = _fnet_arp_add_entry(netif, sender_prot_addr, arp_hdr->sender_hard_addr);
                }
                else
                {
                    entry = _fnet_arp_update_entry(netif, sender_prot_addr, arp_hdr->sender_hard_addr);
                }

                if (entry && (entry->hold))
                {
                    /* Send waiting data.*/
                    _fnet_eth_output(netif, FNET_ETH_TYPE_IP4, entry->hard_addr, entry->hold);

                    entry->hold      = 0;
                    entry->hold_time_ms = 0U;
                }
            }
            else
            {
                if(netif->ip4_addr.address /* If interface has a bind address.*/
                   ||  (netif->arp_if_ptr->arp_probe_ipaddr == targer_prot_addr)) /* Simultaneous probing. Two (or more) hosts attempt to configure
                                                                                    the same IPv4 Link-Local address at the same time. */
                {
                    /* IP is duplicated. */
                    netif->ip4_addr_conflict = FNET_TRUE; /* Set "conflict" flag. */

                    fnet_event_raise(arpif->arp_event);
                }
            }

            /* ARP request. If it asked for our address, we send out a reply.*/
            if ((arp_hdr->op == FNET_HTONS(FNET_ARP_OP_REQUEST)) && (targer_prot_addr == netif->ip4_addr.address))
            {
                arp_hdr->op = FNET_HTONS(FNET_ARP_OP_REPLY); /* Opcode */

                fnet_memcpy(arp_hdr->target_hard_addr, arp_hdr->sender_hard_addr, sizeof(fnet_mac_addr_t));
                fnet_memcpy(arp_hdr->sender_hard_addr, local_addr, sizeof(fnet_mac_addr_t));

                arp_hdr->targer_prot_addr = arp_hdr->sender_prot_addr;
                arp_hdr->sender_prot_addr = netif->ip4_addr.address;

                fnet_arp_trace("TX Reply", arp_hdr); /* Print ARP header. */

                _fnet_eth_output(netif, FNET_ETH_TYPE_ARP, arp_hdr->target_hard_addr, nb);
                return;
            }
        }
    }
DROP:
    _fnet_netbuf_free_chain(nb);
}

/************************************************************************
* DESCRIPTION: Sends ARP request.
*************************************************************************/
void fnet_arp_send_request( fnet_netif_desc_t netif_desc, fnet_ip4_addr_t ip_addr )
{
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;

    _fnet_stack_mutex_lock();
    _fnet_arp_send_request(netif, ip_addr);
    _fnet_stack_mutex_unlock();
}
/* Private */
void _fnet_arp_send_request( fnet_netif_t *netif, fnet_ip4_addr_t ip_addr )
{
    fnet_arp_if_t   *arpif;

    if(netif)
    {
        arpif = netif->arp_if_ptr;
        if(arpif)
        {
            fnet_arp_header_t   *arp_hdr;
            fnet_mac_addr_t     sender_addr;
            fnet_netbuf_t       *nb;

            fnet_isr_lock();

            if ((nb = _fnet_netbuf_new(sizeof(fnet_arp_header_t), FNET_TRUE)) != 0)
            {
                arp_hdr            = (fnet_arp_header_t *)nb->data_ptr;
                arp_hdr->hard_type = FNET_HTONS(FNET_ARP_HARD_TYPE); /* The type of hardware address (=1 for
                                                                        Ethernet).*/
                arp_hdr->prot_type = FNET_HTONS(FNET_ETH_TYPE_IP4);  /* The type of protocol address (=0x0800 for IP). */
                arp_hdr->hard_size = FNET_ARP_HARD_SIZE;             /* The size in bytes of the
                                                                        hardware address (=6). */
                arp_hdr->prot_size = FNET_ARP_PROT_SIZE;             /* The size in bytes of the
                                                                        protocol address (=4). */
                arp_hdr->op = FNET_HTONS(FNET_ARP_OP_REQUEST);       /* Opcode. */

                if(_fnet_netif_get_hw_addr(netif, sender_addr, sizeof(fnet_mac_addr_t)) == FNET_OK)
                {
                    fnet_memcpy(arp_hdr->target_hard_addr, fnet_eth_null_addr, sizeof(fnet_mac_addr_t));
                    fnet_memcpy(arp_hdr->sender_hard_addr, sender_addr, sizeof(fnet_mac_addr_t));

                    arp_hdr->targer_prot_addr = ip_addr;                  /* Protocol address of target of this packet.*/
                    arp_hdr->sender_prot_addr = netif->ip4_addr.address; /* Protocol address of sender of this packet.*/

                    if(netif->ip4_addr.address == INADDR_ANY) /* Is it probing?*/
                    {
                        /* Save probe IPv4 address, for later conflict detection. */
                        arpif->arp_probe_ipaddr = ip_addr;
                    }

                    fnet_arp_trace("TX", arp_hdr); /* Print ARP header. */

                    _fnet_eth_output(netif, FNET_ETH_TYPE_ARP, fnet_eth_broadcast, nb);
                }
            }

            fnet_isr_unlock();
        }
    }
}

/************************************************************************
* DESCRIPTION: This function is called on the IP address
*              duplication event.
*************************************************************************/
static void _fnet_arp_ip4_addr_conflict(void *cookie)
{
    FNET_DEBUG_ARP("");
    FNET_DEBUG_ARP("ARP: Duplicate IP address.");
    _fnet_netif_signal_ip4_addr_conflict((fnet_netif_t *)cookie);
}

/************************************************************************
* DESCRIPTION: This function tries to free not critical parts
*              of memory used by ARP protocol.
*************************************************************************/
void _fnet_arp_drain(fnet_netif_t *netif)
{
    fnet_index_t    i;
    fnet_arp_if_t   *arpif = netif->arp_if_ptr;

    fnet_isr_lock();

    /* ARP table drain.*/
    for (i = 0U; i < FNET_CFG_ARP_TABLE_SIZE; i++)
    {
        if (arpif->arp_table[i].hold)
        {
            _fnet_netbuf_free_chain(arpif->arp_table[i].hold);
            arpif->arp_table[i].hold      = 0;
            arpif->arp_table[i].hold_time_ms = 0U;
        }
    }

    fnet_isr_unlock();
}

/************************************************************************
* DESCRIPTION: This function Retrieves ARP cache entry of
*             the specified network interface.
*************************************************************************/
fnet_bool_t fnet_arp_get_entry ( fnet_netif_desc_t netif_desc, fnet_index_t n, fnet_arp_entry_info_t *entry_info )
{
    fnet_netif_t    *netif = (fnet_netif_t *)netif_desc;
    fnet_bool_t     result = FNET_FALSE;
    fnet_arp_if_t   *arpif;
    fnet_index_t    i;

    if(netif && entry_info)
    {
        arpif = netif->arp_if_ptr;

        if(arpif)
        {
            _fnet_stack_mutex_lock();
            fnet_isr_lock();
            for(i = 0u; i < FNET_CFG_ARP_TABLE_SIZE; i++)
            {
                /* Skip NOT_USED prefixes. */
                if(arpif->arp_table[i].prot_addr != 0U)
                {
                    if(n == 0u)
                    {
                        /* Fill entry.*/
                        entry_info->ip_addr = arpif->arp_table[i].prot_addr;
                        fnet_memcpy(entry_info->mac_addr, arpif->arp_table[i].hard_addr, sizeof(fnet_mac_addr_t));

                        result = FNET_TRUE;
                        break;
                    }
                    n--;
                }
            }
            fnet_isr_unlock();
            _fnet_stack_mutex_unlock();
        }
    }
    return result;
}

/************************************************************************
* DESCRIPTION: Prints ARP header. For debugging purposes.
*************************************************************************/
#if FNET_CFG_DEBUG_TRACE_ARP && FNET_CFG_DEBUG_TRACE
static void fnet_arp_trace(const fnet_char_t *str, fnet_arp_header_t *arp_hdr)
{
    fnet_char_t mac_str[FNET_MAC_ADDR_STR_SIZE];
    fnet_char_t ip_str[FNET_IP4_ADDR_STR_SIZE];

    fnet_printf(FNET_SERIAL_ESC_FG_GREEN "%s", str); /* Print app-specific header.*/
    fnet_println("[ARP header]" FNET_SERIAL_ESC_ATTR_RESET);
    fnet_println("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+");
    fnet_println("|(HWType)                0x%04x |(PrType)                0x%04x |", fnet_ntohs(arp_hdr->hard_type), fnet_ntohs(arp_hdr->prot_type));
    fnet_println("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+");
    fnet_println("|(HWSize)  0x%02x |(PrSize)  0x%02x |(Opcode)                 %5u |", arp_hdr->hard_size, arp_hdr->prot_size, fnet_ntohs(arp_hdr->op));
    fnet_println(
        "+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+"
        "-+/\\/\\/\\/-+");
    fnet_println(
        "|(SenderHWAddr)                                       "
        " " FNET_SERIAL_ESC_FG_BLUE "%17s" FNET_SERIAL_ESC_ATTR_RESET " |",
        fnet_mac_to_str(arp_hdr->sender_hard_addr, mac_str));
    fnet_println(
        "+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+"
        "-+/\\/\\/\\/-+");
    fnet_println("|(SenderPrAddr)                                 " FNET_SERIAL_ESC_FG_BLUE "%15s" FNET_SERIAL_ESC_ATTR_RESET " |",
                 fnet_inet_ntoa(*(struct fnet_in_addr *)(&arp_hdr->sender_prot_addr), ip_str));
    fnet_println(
        "+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+"
        "-+/\\/\\/\\/-+");
    fnet_println(
        "|(TargetHWAddr)                                       "
        " " FNET_SERIAL_ESC_FG_BLUE "%17s" FNET_SERIAL_ESC_ATTR_RESET " |",
        fnet_mac_to_str(arp_hdr->target_hard_addr, mac_str));
    fnet_println(
        "+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+"
        "-+/\\/\\/\\/-+");
    fnet_println("|(TargetPrAddr)                                 " FNET_SERIAL_ESC_FG_BLUE "%15s" FNET_SERIAL_ESC_ATTR_RESET " |",
                 fnet_inet_ntoa(*(struct fnet_in_addr *)(&arp_hdr->targer_prot_addr), ip_str));
    fnet_println("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+");
}

#endif /* FNET_CFG_IP4 */

#endif
