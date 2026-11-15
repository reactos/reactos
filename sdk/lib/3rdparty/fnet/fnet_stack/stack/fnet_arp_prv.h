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
* Private. ARP protocol function definitions, data structures, etc.
*
***************************************************************************/

#ifndef _FNET_ARP_PRV_H_

#define _FNET_ARP_PRV_H_

#include "fnet.h"

#if FNET_CFG_IP4

#include "fnet_netif_prv.h"
#include "fnet_eth.h"
#include "fnet_timer_prv.h"

/************************************************************************
*    ARP definitions
*************************************************************************/
#define FNET_ARP_HARD_TYPE      (1U)         /* for Ethernet.*/

#define FNET_ARP_HARD_SIZE      (6U)         /* for Ethernet.*/
#define FNET_ARP_PROT_SIZE      (4U)         /* for IP.*/

#define FNET_ARP_OP_REQUEST     (1U)         /* ARP request.*/
#define FNET_ARP_OP_REPLY       (2U)         /* ARP reply.*/

#define FNET_ARP_TIMER_PERIOD_MS   ((FNET_CFG_ARP_EXPIRE_TIMEOUT*1000U)/4U)     /* in ms (20/4=5min).*/

/**************************************************************************/ /*!
 * @internal
 * @brief    ARP header structure.
 ******************************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct
{
    fnet_uint16_t   hard_type FNET_COMP_PACKED;         /**< The type of hardware address (=1 for Ethernet).*/
    fnet_uint16_t   prot_type FNET_COMP_PACKED;         /**< The type of protocol address (=0x0800 for IP).*/
    fnet_uint8_t    hard_size FNET_COMP_PACKED;         /**< The size in bytes of the hardware address (=6).*/
    fnet_uint8_t    prot_size FNET_COMP_PACKED;         /**< The size in bytes of the protocol address (=4).*/
    fnet_uint16_t   op FNET_COMP_PACKED;                /**< Opcode.*/
    fnet_mac_addr_t sender_hard_addr FNET_COMP_PACKED;  /**< Hardware address of sender of this packet.*/
    fnet_ip4_addr_t sender_prot_addr FNET_COMP_PACKED;  /**< Protocol address of sender of this packet.*/
    fnet_mac_addr_t target_hard_addr FNET_COMP_PACKED;  /**< Hardware address of target of this packet.*/
    fnet_ip4_addr_t targer_prot_addr FNET_COMP_PACKED;  /**< Protocol address of target of this packet.*/
} fnet_arp_header_t;
FNET_COMP_PACKED_END

/**************************************************************************/ /*!
 * @internal
 * @brief    ARP table entry structure.
 ******************************************************************************/
typedef struct
{
    fnet_mac_addr_t hard_addr;      /**< Hardware address.*/
    fnet_ip4_addr_t prot_addr;      /**< Protocol address.*/
    fnet_time_t     cr_time_ms;     /**< Time of entry creation.*/
    fnet_netbuf_t   *hold;          /**< Last packet until resolved/timeout.*/
    fnet_time_t     hold_time_ms;   /**< Time of the last request.*/
} fnet_arp_entry_t;

typedef struct fnet_arp_if
{
    fnet_arp_entry_t    arp_table[FNET_CFG_ARP_TABLE_SIZE];     /* ARP cache table.*/
    fnet_timer_desc_t   arp_tmr;                                /* ARP timer.*/
    fnet_event_desc_t   arp_event;                              /* ARP event - duplicate address event.*/
    fnet_ip4_addr_t     arp_probe_ipaddr;                       /* ARP probe address.*/
} fnet_arp_if_t;

/************************************************************************
*     Function Prototypes
*************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

fnet_return_t _fnet_arp_init( fnet_netif_t *netif, fnet_arp_if_t *arpif);
void _fnet_arp_release( fnet_netif_t *netif );
fnet_mac_addr_t *_fnet_arp_lookup( fnet_netif_t *netif, fnet_ip4_addr_t ipaddr );
void _fnet_arp_resolve( fnet_netif_t *netif, fnet_ip4_addr_t ipaddr, fnet_netbuf_t *nb );
void _fnet_arp_input( fnet_netif_t *netif, fnet_netbuf_t *nb );
void _fnet_arp_send_request(fnet_netif_t *netif, fnet_ip4_addr_t ip_addr );
void _fnet_arp_drain( fnet_netif_t *netif );

#if defined(__cplusplus)
}
#endif

#endif /* FNET_CFG_IP4 */

#endif /* _FNET_ARP_PRV_H_ */
