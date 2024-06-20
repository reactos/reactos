/**
 * @file
 * MDNS responder private definitions
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
#ifndef LWIP_HDR_MDNS_PRIV_H
#define LWIP_HDR_MDNS_PRIV_H

#include "lwip/apps/mdns.h"
#include "lwip/apps/mdns_opts.h"
#include "lwip/pbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

#if LWIP_MDNS_RESPONDER

#define MDNS_READNAME_ERROR 0xFFFF
#define NUM_DOMAIN_OFFSETS 10

#define SRV_PRIORITY 0
#define SRV_WEIGHT   0

/* mDNS TTL: (RFC6762 section 10)
 *  - 120 seconds if the hostname appears somewhere in the RR
 *  - 75 minutes if not (4500 seconds)
 *  - 10 seconds if responding to a legacy query
 */
#define MDNS_TTL_10    10
#define MDNS_TTL_120   120
#define MDNS_TTL_4500  4500

/* RFC6762 section 8.1: If fifteen conflicts occur within any ten-second period,
 * then the host MUST wait at least five seconds before each successive
 * additional probe attempt.
 */
#define MDNS_PROBE_MAX_CONFLICTS_BEFORE_RATE_LIMIT  15
#define MDNS_PROBE_MAX_CONFLICTS_TIME_WINDOW        10000
#define MDNS_PROBE_MAX_CONFLICTS_TIMEOUT            5000

#if LWIP_MDNS_SEARCH
/** Description of a search request */
struct mdns_request {
  /** Name of service, like 'myweb' */
  char name[MDNS_LABEL_MAXLEN + 1];
  /** Type of service, like '_http' or '_services._dns-sd' */
  struct mdns_domain service;
  /** Callback function called for each response */
  search_result_fn_t result_fn;
  void *arg;
  /** Protocol, TCP or UDP */
  u16_t proto;
  /** Query type (PTR, SRV, ...) */
  u8_t qtype;
  /** PTR only request. */
  u16_t only_ptr;
};
#endif

/** Description of a service */
struct mdns_service {
  /** TXT record to answer with */
  struct mdns_domain txtdata;
  /** Name of service, like 'myweb' */
  char name[MDNS_LABEL_MAXLEN + 1];
  /** Type of service, like '_http' */
  char service[MDNS_LABEL_MAXLEN + 1];
  /** Callback function and userdata
   * to update txtdata buffer */
  service_get_txt_fn_t txt_fn;
  void *txt_userdata;
  /** Protocol, TCP or UDP */
  u16_t proto;
  /** Port of the service */
  u16_t port;
};

/** mDNS output packet */
struct mdns_outpacket {
  /** Packet data */
  struct pbuf *pbuf;
  /** Current write offset in packet */
  u16_t write_offset;
  /** Number of questions written */
  u16_t questions;
  /** Number of normal answers written */
  u16_t answers;
  /** Number of authoritative answers written */
  u16_t authoritative;
  /** Number of additional answers written */
  u16_t additional;
  /** Offsets for written domain names in packet.
   *  Used for compression */
  u16_t domain_offsets[NUM_DOMAIN_OFFSETS];
};

/** mDNS output message */
struct mdns_outmsg {
  /** Identifier. Used in legacy queries */
  u16_t tx_id;
  /** dns flags */
  u8_t flags;
  /** Destination IP/port if sent unicast */
  ip_addr_t dest_addr;
  u16_t dest_port;
  /** If all answers in packet should set cache_flush bit */
  u8_t cache_flush;
  /** If reply should be sent unicast (as requested) */
  u8_t unicast_reply_requested;
  /** If legacy query. (tx_id needed, and write
   *  question again in reply before answer) */
  u8_t legacy_query;
  /** If the query is a probe msg we need to respond immediately. Independent of
   *  the QU or QM flag. */
  u8_t probe_query_recv;
  /* Question bitmask for host information */
  u8_t host_questions;
  /* Questions bitmask per service */
  u8_t serv_questions[MDNS_MAX_SERVICES];
  /* Reply bitmask for host information */
  u8_t host_replies;
  /* Bitmask for which reverse IPv6 hosts to answer */
  u8_t host_reverse_v6_replies;
  /* Reply bitmask per service */
  u8_t serv_replies[MDNS_MAX_SERVICES];
#ifdef LWIP_MDNS_SEARCH
  /** Search query to send */
  struct mdns_request *query;
#endif
};

/** Delayed msg info */
struct mdns_delayed_msg {
  /** Signals if a multicast msg needs to be send out */
  u8_t multicast_msg_waiting;
  /** Multicast timeout for all multicast traffic except probe answers */
  u8_t multicast_timeout;
  /** Multicast timeout only for probe answers */
  u8_t multicast_probe_timeout;
  /** Output msg used for delayed multicast responses */
  struct mdns_outmsg delayed_msg_multicast;
  /** Prefer multicast over unicast timeout -> 25% of TTL = we take 30s as
      general delay. */
  u8_t multicast_timeout_25TTL;
  /** Only send out new unicast message if previous was send */
  u8_t unicast_msg_in_use;
  /** Output msg used for delayed unicast responses */
  struct mdns_outmsg delayed_msg_unicast;
};

/* MDNS states */
typedef enum {
  /* MDNS module is off */
  MDNS_STATE_OFF,
  /* Waiting before probing can be started */
  MDNS_STATE_PROBE_WAIT,
  /* Probing the unique records */
  MDNS_STATE_PROBING,
  /* Waiting before announcing the probed unique records */
  MDNS_STATE_ANNOUNCE_WAIT,
  /* Announcing all records */
  MDNS_STATE_ANNOUNCING,
  /* Probing and announcing completed */
  MDNS_STATE_COMPLETE
} mdns_resp_state_enum_t;

/** Description of a host/netif */
struct mdns_host {
  /** Hostname */
  char name[MDNS_LABEL_MAXLEN + 1];
  /** Pointer to services */
  struct mdns_service *services[MDNS_MAX_SERVICES];
  /** Number of probes/announces sent for the current name */
  u8_t sent_num;
  /** State of the mdns responder */
  mdns_resp_state_enum_t state;
#if LWIP_IPV4
  /** delayed msg struct for IPv4 */
  struct mdns_delayed_msg ipv4;
#endif
#if LWIP_IPV6
  /** delayed msg struct for IPv6 */
  struct mdns_delayed_msg ipv6;
#endif
  /** Timestamp of probe conflict saved in list */
  u32_t conflict_time[MDNS_PROBE_MAX_CONFLICTS_BEFORE_RATE_LIMIT];
  /** Rate limit flag */
  u8_t rate_limit_activated;
  /** List index for timestamps */
  u8_t index;
  /** number of conflicts since startup */
  u8_t num_conflicts;
};

struct mdns_host* netif_mdns_data(struct netif *netif);
struct udp_pcb* get_mdns_pcb(void);

#endif /* LWIP_MDNS_RESPONDER */

#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_MDNS_PRIV_H */
