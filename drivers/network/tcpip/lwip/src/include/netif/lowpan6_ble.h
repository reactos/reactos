/**
 * @file
 * 6LowPAN over BLE for IPv6 (RFC7668).
 */

/*
 * Copyright (c) 2017 Benjamin Aigner
 * Copyright (c) 2015 Inico Technologies Ltd. , Author: Ivan Delamer <delamer@inicotech.com>
 *
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
 * Author: Benjamin Aigner <aignerb@technikum-wien.at>
 *
 * Based on the original 6lowpan implementation of lwIP ( @see 6lowpan.c)
 */

#ifndef LWIP_HDR_LOWPAN6_BLE_H
#define LWIP_HDR_LOWPAN6_BLE_H

#include "netif/lowpan6_opts.h"

#if LWIP_IPV6 /* don't build if not configured for use in lwipopts.h */

#include "netif/lowpan6_common.h"
#include "lwip/pbuf.h"
#include "lwip/ip.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"

#ifdef __cplusplus
extern "C" {
#endif

err_t rfc7668_output(struct netif *netif, struct pbuf *q, const ip6_addr_t *ip6addr);
err_t rfc7668_input(struct pbuf * p, struct netif *netif);
err_t rfc7668_set_local_addr_eui64(struct netif *netif, const u8_t *local_addr, size_t local_addr_len);
err_t rfc7668_set_local_addr_mac48(struct netif *netif, const u8_t *local_addr, size_t local_addr_len, int is_public_addr);
err_t rfc7668_set_peer_addr_eui64(struct netif *netif, const u8_t *peer_addr, size_t peer_addr_len);
err_t rfc7668_set_peer_addr_mac48(struct netif *netif, const u8_t *peer_addr, size_t peer_addr_len, int is_public_addr);
err_t rfc7668_set_context(u8_t index, const ip6_addr_t * context);
err_t rfc7668_if_init(struct netif *netif);

#if !NO_SYS
err_t tcpip_rfc7668_input(struct pbuf *p, struct netif *inp);
#endif

void ble_addr_to_eui64(u8_t *dst, const u8_t *src, int public_addr);
void eui64_to_ble_addr(u8_t *dst, const u8_t *src);

#ifdef __cplusplus
}
#endif

#endif /* LWIP_IPV6 */

#endif /* LWIP_HDR_LOWPAN6_BLE_H */
