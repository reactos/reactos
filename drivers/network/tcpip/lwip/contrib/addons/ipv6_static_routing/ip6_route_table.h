/**
 * @file
 *
 * IPv6 static route table.
 */

/*
 * Copyright (c) 2015 Nest Labs, Inc.
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
 * Author: Pradip De <pradipd@google.com>
 *
 *
 * Please coordinate changes and requests with Pradip De
 * <pradipd@google.com>
 */

#ifndef __LWIP_IP6_ROUTE_TABLE_H__
#define __LWIP_IP6_ROUTE_TABLE_H__

#include "lwip/opt.h"

#if LWIP_IPV6  /* don't build if not configured for use in lwipopts.h */

#include "lwip/ip6_addr.h"
#include "lwip/err.h"

#ifdef __cplusplus
extern "C" {
#endif

struct netif;

/**
 * LWIP_IPV6_NUM_ROUTES: Number of IPV6 routes that can be kept in the static route table.
 */
#ifndef LWIP_IPV6_NUM_ROUTE_ENTRIES
#define LWIP_IPV6_NUM_ROUTE_ENTRIES         (8)
#endif

#define IP6_MAX_PREFIX_LEN                  (128)
#define IP6_PREFIX_ALLOWED_GRANULARITY      (8)
/* Prefix length cannot be greater than 128 bits and needs to be at a byte boundary */
#define ip6_prefix_valid(prefix_len)        (((prefix_len) <= IP6_MAX_PREFIX_LEN) &&                 \
                                             (((prefix_len) % IP6_PREFIX_ALLOWED_GRANULARITY) == 0))

struct ip6_prefix {
  ip6_addr_t addr;
  u8_t prefix_len; /* prefix length in bits at byte boundaries */
};

struct ip6_route_entry {
  struct ip6_prefix prefix;
  struct netif *netif;
  const ip6_addr_t *gateway;
};

err_t ip6_add_route_entry(const struct ip6_prefix *ip6_prefix, struct netif *netif,
                          const ip6_addr_t *gateway, s8_t *idx);
void ip6_remove_route_entry(const struct ip6_prefix *ip6_prefix);
s8_t ip6_find_route_entry(const ip6_addr_t *ip6_dest_addr);
struct netif *ip6_static_route(const ip6_addr_t *src, const ip6_addr_t *dest);
const ip6_addr_t *ip6_get_gateway(struct netif *netif, const ip6_addr_t *dest);
const struct ip6_route_entry *ip6_get_route_table(void);

#ifdef __cplusplus
}
#endif

#endif /* LWIP_IPV6 */

#endif /* __LWIP_IP6_ROUTE_TABLE_H__ */
