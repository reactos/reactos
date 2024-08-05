/*
 * Copyright (c) Espressif Systems (Shanghai) CO LTD
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
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <string.h>

#include "lwip/prot/dhcp.h"
#include "lwip/dhcp.h"
#include "lwip/netif.h"
#include "lwip/prot/iana.h"


void dhcp_parse_extra_opts(struct dhcp *dhcp, uint8_t state, uint8_t option, uint8_t len, struct pbuf* p, uint16_t offset)
{
  LWIP_UNUSED_ARG(dhcp);
  LWIP_UNUSED_ARG(state);
  LWIP_UNUSED_ARG(option);
  LWIP_UNUSED_ARG(len);
  LWIP_UNUSED_ARG(p);
  LWIP_UNUSED_ARG(offset);
#if LWIP_DHCP_ENABLE_MTU_UPDATE
  if ((option == DHCP_OPTION_MTU) &&
     (state == DHCP_STATE_REBOOTING || state == DHCP_STATE_REBINDING ||
      state == DHCP_STATE_RENEWING  || state == DHCP_STATE_REQUESTING)) {
    u32_t mtu = 0;
    struct netif *netif;
    LWIP_ERROR("dhcp_parse_extra_opts(): MTU option's len != 2", len == 2, return;);
    LWIP_ERROR("dhcp_parse_extra_opts(): extracting MTU option failed",
               pbuf_copy_partial(p, &mtu, 2, offset) == 2, return;);
    mtu = lwip_htons((u16_t)mtu);
    NETIF_FOREACH(netif) {
      /* find the netif related to this dhcp */
      if (dhcp == netif_dhcp_data(netif)) {
        if (mtu < netif->mtu) {
          netif->mtu = mtu;
          LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_parse_extra_opts(): Negotiated netif MTU is %d\n", netif->mtu));
        }
        return;
      }
    }
  } /* DHCP_OPTION_MTU */
#endif /* LWIP_DHCP_ENABLE_MTU_UPDATE */
}

void dhcp_append_extra_opts(struct netif *netif, uint8_t state, struct dhcp_msg *msg_out, uint16_t *options_out_len)
{
  LWIP_UNUSED_ARG(netif);
  LWIP_UNUSED_ARG(state);
  LWIP_UNUSED_ARG(msg_out);
  LWIP_UNUSED_ARG(options_out_len);
#if LWIP_DHCP_ENABLE_CLIENT_ID
  if (state == DHCP_STATE_RENEWING || state == DHCP_STATE_REBINDING ||
      state == DHCP_STATE_REBOOTING || state == DHCP_STATE_OFF ||
      state == DHCP_STATE_REQUESTING || state == DHCP_STATE_BACKING_OFF || state == DHCP_STATE_SELECTING) {
    size_t i;
    u8_t *options = msg_out->options + *options_out_len;
    LWIP_ERROR("dhcp_append(client_id): options_out_len + 3 + netif->hwaddr_len <= DHCP_OPTIONS_LEN",
               *options_out_len + 3U + netif->hwaddr_len <= DHCP_OPTIONS_LEN, return;);
    *options_out_len = *options_out_len + netif->hwaddr_len + 3;
    *options++ = DHCP_OPTION_CLIENT_ID;
    *options++ = netif->hwaddr_len + 1; /* option size */
    *options++ = LWIP_IANA_HWTYPE_ETHERNET;
    for (i = 0; i < netif->hwaddr_len; i++) {
      *options++ = netif->hwaddr[i];
    }
  }
#endif /* LWIP_DHCP_ENABLE_CLIENT_ID */
}
