/**
 * @file
 * MDNS responder - output related functionalities
 */

 /*
 * Copyright (c) 2015 Verisure Innovation AB
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
 * Author: Erik Ekman <erik@kryo.se>
 * Author: Jasper Verschueren <jasper.verschueren@apart-audio.com>
 *
 */

#ifndef LWIP_HDR_APPS_MDNS_OUT_H
#define LWIP_HDR_APPS_MDNS_OUT_H

#include "lwip/apps/mdns_opts.h"
#include "lwip/apps/mdns_priv.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"

#ifdef __cplusplus
extern "C" {
#endif

#if LWIP_MDNS_RESPONDER

/** Bitmasks outmsg generation */
/* Probe for ALL types with hostname */
#define QUESTION_PROBE_HOST_ANY          0x10
/* Probe for ALL types with service instance name */
#define QUESTION_PROBE_SERVICE_NAME_ANY  0x10

/* Lookup from hostname -> IPv4 */
#define REPLY_HOST_A            0x01
/* Lookup from IPv4/v6 -> hostname */
#define REPLY_HOST_PTR_V4       0x02
/* Lookup from hostname -> IPv6 */
#define REPLY_HOST_AAAA         0x04
/* Lookup from hostname -> IPv6 */
#define REPLY_HOST_PTR_V6       0x08

/* Lookup for service types */
#define REPLY_SERVICE_TYPE_PTR  0x10
/* Lookup for instances of service */
#define REPLY_SERVICE_NAME_PTR  0x20
/* Lookup for location of service instance */
#define REPLY_SERVICE_SRV       0x40
/* Lookup for text info on service instance */
#define REPLY_SERVICE_TXT       0x80

/* RFC6762 section 6:
 * To protect the network against excessive packet flooding due to software bugs
 * or malicious attack, a Multicast DNS responder MUST NOT (except in the one
 * special case of answering probe queries) multicast a record on a given
 * interface until at least one second has elapsed since the last time that
 * record was multicast on that particular interface.
 */
#define MDNS_MULTICAST_TIMEOUT    1000

/* RFC6762 section 6:
 * In this special case only, when responding via multicast to a probe, a
 * Multicast DNS responder is only required to delay its transmission as
 * necessary to ensure an interval of at least 250 ms since the last time the
 * record was multicast on that interface.
 */
#define MDNS_MULTICAST_PROBE_TIMEOUT    250

/* RFC6762 section 5.4:
 * When receiving a question with the unicast-response bit set, a responder
 * SHOULD usually respond with a unicast packet directed back to the querier.
 * However, if the responder has not multicast that record recently (within one
 * quarter of its TTL), then the responder SHOULD instead multicast the response
 * so as to keep all the peer caches up to date, and to permit passive conflict
 * detection.
 * -> we implement a stripped down version. Depending on a timeout of 30s
 *    (25% of 120s) all QU questions are send via multicast or unicast.
 */
#define MDNS_MULTICAST_TIMEOUT_25TTL  30000

err_t mdns_create_outpacket(struct netif *netif, struct mdns_outmsg *msg,
                            struct mdns_outpacket *outpkt);
err_t mdns_send_outpacket(struct mdns_outmsg *msg, struct netif *netif);
void mdns_set_timeout(struct netif *netif, u32_t msecs,
                        sys_timeout_handler handler, u8_t *busy_flag);
#if LWIP_IPV4
void mdns_multicast_timeout_reset_ipv4(void *arg);
void mdns_multicast_probe_timeout_reset_ipv4(void *arg);
void mdns_multicast_timeout_25ttl_reset_ipv4(void *arg);
void mdns_send_multicast_msg_delayed_ipv4(void *arg);
void mdns_send_unicast_msg_delayed_ipv4(void *arg);
void mdns_start_multicast_timeouts_ipv4(struct netif *netif);
#endif
#if LWIP_IPV6
void mdns_multicast_timeout_reset_ipv6(void *arg);
void mdns_multicast_probe_timeout_reset_ipv6(void *arg);
void mdns_multicast_timeout_25ttl_reset_ipv6(void *arg);
void mdns_send_multicast_msg_delayed_ipv6(void *arg);
void mdns_send_unicast_msg_delayed_ipv6(void *arg);
void mdns_start_multicast_timeouts_ipv6(struct netif *netif);
#endif
void mdns_prepare_txtdata(struct mdns_service *service);
#ifdef LWIP_MDNS_SEARCH
err_t mdns_send_request(struct mdns_request *req, struct netif *netif, const ip_addr_t *destination);
#endif

#endif /* LWIP_MDNS_RESPONDER */

#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_APPS_MDNS_OUT_H */
