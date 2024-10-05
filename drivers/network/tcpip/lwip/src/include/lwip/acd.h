/**
 * @file
 *
 * ACD IPv4 Address Conflict Detection
 */

/*
 *
 * Copyright (c) 2007 Dominik Spies <kontakt@dspies.de>
 * Copyright (c) 2018 Jasper Verschueren <jasper.verschueren@apart-audio.com>
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
 * Author: Jasper Verschueren <jasper.verschueren@apart-audio.com>
 * Author: Dominik Spies <kontakt@dspies.de>
 */

#ifndef LWIP_HDR_ACD_H
#define LWIP_HDR_ACD_H

#include "lwip/opt.h"

/* don't build if not configured for use in lwipopts.h */
#if LWIP_IPV4 && LWIP_ACD

#include "lwip/netif.h"
#include "lwip/etharp.h"
#include "lwip/prot/acd.h"

#ifdef __cplusplus
extern "C" {
#endif

/** ACD Timing
 *  ACD_TMR_INTERVAL msecs, I recommend a value of 100.
 *  The value must divide 1000 with a remainder almost 0. Possible values are
 *  1000, 500, 333, 250, 200, 166, 142, 125, 111, 100 ....
 */
#define ACD_TMR_INTERVAL      100

/**
 * Callback function: Handle conflict information from ACD module
 *
 * @param netif   network interface to handle conflict information on
 * @param state   acd_callback_enum_t
 */
typedef void (*acd_conflict_callback_t)(struct netif *netif, acd_callback_enum_t state);

/** ACD state information per netif */
struct acd
{
  /** next acd module */
  struct acd *next;
  /** the currently selected, probed, announced or used IP-Address */
  ip4_addr_t ipaddr;
  /** current ACD state machine state */
  acd_state_enum_t state;
  /** sent number of probes or announces, dependent on state */
  u8_t sent_num;
  /** ticks to wait, tick is ACD_TMR_INTERVAL long */
  u16_t ttw;
  /** ticks until a conflict can again be solved by defending */
  u8_t lastconflict;
  /** total number of probed/used IP-Addresses that resulted in a conflict */
  u8_t num_conflicts;
  /** callback function -> let's the acd user know if the address is good or
      if a conflict is detected */
  acd_conflict_callback_t acd_conflict_callback;
};

err_t acd_add(struct netif *netif, struct acd *acd,
              acd_conflict_callback_t acd_conflict_callback);
void acd_remove(struct netif *netif, struct acd *acd);
err_t acd_start(struct netif *netif, struct acd *acd, ip4_addr_t ipaddr);
err_t acd_stop(struct acd *acd);
void acd_arp_reply(struct netif *netif, struct etharp_hdr *hdr);
void acd_tmr(void);
void acd_network_changed_link_down(struct netif *netif);
void acd_netif_ip_addr_changed(struct netif *netif, const ip_addr_t *old_addr,
                               const ip_addr_t *new_addr);

#ifdef __cplusplus
}
#endif

#endif /* LWIP_IPV4 && LWIP_ACD */

#endif /* LWIP_HDR_ACD_H */
