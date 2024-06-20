/**
 * @file
 *
 * A netif implementing the ZigBee Eencapsulation Protocol (ZEP).
 * This is used to tunnel 6LowPAN over UDP.
 */

/*
 * Copyright (c) 2018 Simon Goldschmidt
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
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Simon Goldschmidt <goldsimon@gmx.de>
 *
 */

#ifndef LWIP_HDR_ZEPIF_H
#define LWIP_HDR_ZEPIF_H

#include "lwip/opt.h"
#include "netif/lowpan6.h"

#if LWIP_IPV6 && LWIP_UDP /* don't build if not configured for use in lwipopts.h */

#include "lwip/netif.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ZEPIF_DEFAULT_UDP_PORT  17754

/** Pass this struct as 'state' to netif_add to control the behaviour
 * of this netif. If NULL is passed, default behaviour is chosen */
struct zepif_init {
  /** The UDP port used to ZEP frames from (0 = default) */
  u16_t               zep_src_udp_port;
  /** The UDP port used to ZEP frames to (0 = default) */
  u16_t               zep_dst_udp_port;
  /** The IP address to sed ZEP frames from (NULL = ANY) */
  const ip_addr_t    *zep_src_ip_addr;
  /** The IP address to sed ZEP frames to (NULL = BROADCAST) */
  const ip_addr_t    *zep_dst_ip_addr;
  /** If != NULL, the udp pcb is bound to this netif */
  const struct netif *zep_netif;
  /** MAC address of the 6LowPAN device */
  u8_t                addr[6];
};

err_t zepif_init(struct netif *netif);

#ifdef __cplusplus
}
#endif

#endif /* LWIP_IPV6 && LWIP_UDP */

#endif /* LWIP_HDR_ZEPIF_H */
