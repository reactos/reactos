/**
 * @file
 *
 * AutoIP Automatic LinkLocal IP Configuration
 */

/*
 *
 * Copyright (c) 2007 Dominik Spies <kontakt@dspies.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * Author: Dominik Spies <kontakt@dspies.de>
 *
 * This is a AutoIP implementation for the lwIP TCP/IP stack. It aims to conform
 * with RFC 3927.
 *
 */

#ifndef LWIP_HDR_AUTOIP_H
#define LWIP_HDR_AUTOIP_H

#include "lwip/opt.h"

#if LWIP_IPV4 && LWIP_AUTOIP /* don't build if not configured for use in lwipopts.h */

#include "lwip/netif.h"
/* #include "lwip/udp.h" */
#include "lwip/etharp.h"
#include "lwip/acd.h"

#ifdef __cplusplus
extern "C" {
#endif

/** AutoIP state information per netif */
struct autoip
{
  /** the currently selected, probed, announced or used LL IP-Address */
  ip4_addr_t llipaddr;
  /** current AutoIP state machine state */
  u8_t state;
  /** total number of probed/used Link Local IP-Addresses */
  u8_t tried_llipaddr;
  /** acd struct */
  struct acd acd;
};


void autoip_set_struct(struct netif *netif, struct autoip *autoip);
void autoip_remove_struct(struct netif *netif);
err_t autoip_start(struct netif *netif);
err_t autoip_stop(struct netif *netif);
void autoip_network_changed_link_up(struct netif *netif);
void autoip_network_changed_link_down(struct netif *netif);
u8_t autoip_supplied_address(struct netif *netif);

/* for lwIP internal use by ip4.c */
u8_t autoip_accept_packet(struct netif *netif, const ip4_addr_t *addr);

#define netif_autoip_data(netif) ((struct autoip*)netif_get_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_AUTOIP))

#ifdef __cplusplus
}
#endif

#endif /* LWIP_IPV4 && LWIP_AUTOIP */

#endif /* LWIP_HDR_AUTOIP_H */
