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
*  Private. Ethernet platform independent API functions.
*
***************************************************************************/

#ifndef _FNET_ETHERNET_PRV_H_

#define _FNET_ETHERNET_PRV_H_

#include "fnet.h"

#include "fnet_netif_prv.h"
#include "fnet_arp_prv.h"

/************************************************************************
*     Definitions
*************************************************************************/
/* Ethernet Frame Types */
#define FNET_ETH_TYPE_IP4       (0x0800U)
#define FNET_ETH_TYPE_ARP       (0x0806U)
#define FNET_ETH_TYPE_IP6       (0x86DDU)
#define FNET_ETH_TYPE_RESERVED  (0xFFFFU)

#define FNET_ETH_HDR_SIZE       (14U)    /* Size of Ethernet header.*/
#define FNET_ETH_CRC_SIZE       (4U)     /* Size of Ethernet CRC.*/


/* The MDC clock corresponds to a maximum frequency of 2.5 MHz to be compliant with the IEEE 802.3 MII specification. */
#define FNET_ETH_MII_CLOCK_KHZ                   (2500U)

/************************************************************************
*    Network Layer Protocol interface control structure.
*************************************************************************/
typedef struct fnet_eth_prot_if
{
    fnet_uint16_t protocol;                                    /* Protocol number */
    void (*input)( fnet_netif_t *netif, fnet_netbuf_t *nb );    /* Protocol input function.*/
} fnet_eth_prot_if_t;

/*****************************************************************************
*     Ethernet Frame header.
******************************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct
{
    fnet_mac_addr_t destination_addr    FNET_COMP_PACKED;   /**< 48-bit destination address.*/
    fnet_mac_addr_t source_addr         FNET_COMP_PACKED;   /**< 48-bit source address.*/
    fnet_uint16_t   type                FNET_COMP_PACKED;   /**< 16-bit type field.*/
} fnet_eth_header_t;
FNET_COMP_PACKED_END

/**************************************************************************/ /*!
 * @internal
 * @brief    Ethernet interface API structure.
 ******************************************************************************/
typedef struct fnet_eth_api
{
    fnet_return_t (*phy_read)(fnet_netif_t *netif, fnet_uint32_t reg_addr, fnet_uint16_t *data); /* Read a value from a Ethernet PHY's MII register. */
    fnet_return_t (*phy_write)(fnet_netif_t *netif, fnet_uint32_t reg_addr, fnet_uint16_t data); /* Write a value to a PHY's MII register. */
} fnet_eth_api_t;

/*****************************************************************************
*     Ethernet Control data structure
******************************************************************************/
typedef struct fnet_eth_if
{
    void                            *eth_prv;           /* Points to Ethernet driver-specific control data structure. */
    fnet_index_t                    eth_mac_number;     /* MAC module number [0-1]. */
    fnet_uint8_t                    eth_phy_addr;       /* PHY address */
    void                            ( *eth_output)(fnet_netif_t *netif, fnet_netbuf_t *nb); /* Ethernet driver output.*/
#if FNET_CFG_MULTICAST
    void                            ( *eth_multicast_join)(fnet_netif_t *netif, fnet_mac_addr_t multicast_addr);    /* Ethernet driver join multicast group (optional).*/
    void                            ( *eth_multicast_leave)(fnet_netif_t *netif, fnet_mac_addr_t multicast_addr);   /* Ethernet driver leave multicast group (optional).*/
#endif /* FNET_CFG_MULTICAST */
    fnet_return_t                   (*eth_cpu_init)(fnet_netif_t *netif);  /* Platform-specific Ethernet module initialization. Used mainly for IO pin and clock initialization (optional).*/
    fnet_return_t                   (*eth_cpu_phy_init)(fnet_netif_t *netif);  /* Platform-specific PHY module initialization. Used mainly for special reset or special PHY register initialization (optional).*/

    /* Internal parameters.*/
#if 0  /* Done by _fnet_netif_is_connected()*/
    fnet_timer_desc_t               eth_timer;          /* Optional ETH timer.*/
#endif
#if FNET_CFG_IP4
    fnet_arp_if_t                   arp_if;             /* ARP interface.*/
#endif
#if FNET_CFG_IP6
    fnet_nd6_if_t                   nd6_if;             /* ND inteface.*/
#endif
} fnet_eth_if_t;

/************************************************************************
*     Function Prototypes
*************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

fnet_return_t _fnet_eth_init(fnet_netif_t *netif);
void _fnet_eth_release( fnet_netif_t *netif);
void _fnet_eth_drain(fnet_netif_t *netif);
void _fnet_eth_change_addr_notify(fnet_netif_t *netif);

#if FNET_CFG_IP4
void _fnet_eth_output_ip4(fnet_netif_t *netif, fnet_ip4_addr_t dest_ip_addr, fnet_netbuf_t *nb);
#endif

void _fnet_eth_output( fnet_netif_t *netif, fnet_uint16_t type, const fnet_mac_addr_t dest_addr, fnet_netbuf_t *nb );
void _fnet_eth_input( fnet_netif_t *netif, fnet_uint8_t *frame, fnet_size_t frame_size);

fnet_return_t  _fnet_eth_phy_read(fnet_netif_t *netif, fnet_uint32_t reg_addr, fnet_uint16_t *reg_data);
fnet_return_t  _fnet_eth_phy_write(fnet_netif_t *netif, fnet_uint32_t reg_addr, fnet_uint16_t reg_data);
void _fnet_eth_phy_set_addr(fnet_netif_t *netif, fnet_uint8_t phy_addr);
fnet_uint8_t _fnet_eth_phy_get_addr(fnet_netif_t *netif);
fnet_bool_t _fnet_eth_is_connected(fnet_netif_t *netif);
void _fnet_eth_phy_discover_addr (fnet_netif_t *netif, fnet_uint8_t phy_addr_start);
fnet_return_t _fnet_eth_phy_init(fnet_netif_t *netif);

#if FNET_CFG_MULTICAST
#if FNET_CFG_IP4
void _fnet_eth_multicast_leave_ip4(fnet_netif_t *netif, fnet_ip4_addr_t multicast_addr );
void _fnet_eth_multicast_join_ip4(fnet_netif_t *netif, fnet_ip4_addr_t  multicast_addr );
#endif
#if FNET_CFG_IP6
void _fnet_eth_multicast_leave_ip6(fnet_netif_t *netif, fnet_ip6_addr_t *multicast_addr );
void _fnet_eth_multicast_join_ip6(fnet_netif_t *netif, const fnet_ip6_addr_t  *multicast_addr );
#endif
#endif /* FNET_CFG_MULTICAST */

#if FNET_CFG_IP6
void _fnet_eth_output_ip6(fnet_netif_t *netif, const fnet_ip6_addr_t *src_ip_addr,  const fnet_ip6_addr_t *dest_ip_addr, fnet_netbuf_t *nb);
#endif

void fnet_eth_phy_reg_print(fnet_netif_desc_t netif_desc);

#if FNET_CFG_DEBUG_TRACE_ETH && FNET_CFG_DEBUG_TRACE
void fnet_eth_trace(fnet_char_t *str, fnet_eth_header_t *eth_hdr);
#else
#define fnet_eth_trace(str, eth_hdr)    do{}while(0)
#endif

#if defined(__cplusplus)
}
#endif

#endif /* _FNET_ETHERNET_PRV_H_ */
