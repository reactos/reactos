/**
 * @file
 * MDNS responder implementation
 *
 * @defgroup mdns MDNS
 * @ingroup apps
 *
 * RFC 6762 - Multicast DNS<br>
 * RFC 6763 - DNS-Based Service Discovery
 *
 * You need to increase MEMP_NUM_SYS_TIMEOUT by one if you use MDNS!
 *
 * @verbinclude mdns.txt
 *
 * Things left to implement:
 * -------------------------
 *
 * - Sending goodbye messages (zero ttl) - shutdown, DHCP lease about to expire, DHCP turned off...
 * - Sending negative responses NSEC
 * - Fragmenting replies if required
 * - Individual known answer detection for all local IPv6 addresses
 * - Dynamic size of outgoing packet
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

#include "lwip/apps/mdns.h"
#include "lwip/apps/mdns_priv.h"
#include "lwip/apps/mdns_domain.h"
#include "lwip/apps/mdns_out.h"
#include "lwip/netif.h"
#include "lwip/udp.h"
#include "lwip/ip_addr.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/prot/dns.h"
#include "lwip/prot/iana.h"
#include "lwip/timeouts.h"
#include "lwip/sys.h"

#include <string.h> /* memset */
#include <stdio.h>  /* snprintf */

#if LWIP_MDNS_RESPONDER

#if (LWIP_IPV4 && !LWIP_IGMP)
#error "If you want to use MDNS with IPv4, you have to define LWIP_IGMP=1 in your lwipopts.h"
#endif
#if (LWIP_IPV6 && !LWIP_IPV6_MLD)
#error "If you want to use MDNS with IPv6, you have to define LWIP_IPV6_MLD=1 in your lwipopts.h"
#endif
#if (!LWIP_UDP)
#error "If you want to use MDNS, you have to define LWIP_UDP=1 in your lwipopts.h"
#endif
#ifndef LWIP_RAND
#error "If you want to use MDNS, you have to define LWIP_RAND=(random function) in your lwipopts.h"
#endif

#if LWIP_IPV4
#include "lwip/igmp.h"
/* IPv4 multicast group 224.0.0.251 */
static const ip_addr_t v4group = DNS_MQUERY_IPV4_GROUP_INIT;
#endif

#if LWIP_IPV6
#include "lwip/mld6.h"
/* IPv6 multicast group FF02::FB */
static const ip_addr_t v6group = DNS_MQUERY_IPV6_GROUP_INIT;
#endif

#define MDNS_IP_TTL  255

#if LWIP_MDNS_SEARCH
static struct mdns_request mdns_requests[MDNS_MAX_REQUESTS];
#endif

static u8_t mdns_netif_client_id;
static struct udp_pcb *mdns_pcb;
#if MDNS_RESP_USENETIF_EXTCALLBACK
NETIF_DECLARE_EXT_CALLBACK(netif_callback)
#endif
static mdns_name_result_cb_t mdns_name_result_cb;

#define NETIF_TO_HOST(netif) (struct mdns_host*)(netif_get_client_data(netif, mdns_netif_client_id))

/** Delayed response defines */
#define MDNS_RESPONSE_DELAY_MAX   120
#define MDNS_RESPONSE_DELAY_MIN    20
#define MDNS_RESPONSE_DELAY (LWIP_RAND() %(MDNS_RESPONSE_DELAY_MAX - \
                             MDNS_RESPONSE_DELAY_MIN) + MDNS_RESPONSE_DELAY_MIN)
/* Delayed response for truncated question defines */
#define MDNS_RESPONSE_TC_DELAY_MAX   500
#define MDNS_RESPONSE_TC_DELAY_MIN   400
#define MDNS_RESPONSE_TC_DELAY_MS (LWIP_RAND() % (MDNS_RESPONSE_TC_DELAY_MAX - \
                             MDNS_RESPONSE_TC_DELAY_MIN) + MDNS_RESPONSE_TC_DELAY_MIN)

/** Probing & announcing defines */
#define MDNS_PROBE_COUNT          3
#ifdef LWIP_RAND
/* first probe timeout SHOULD be random 0-250 ms*/
#define MDNS_INITIAL_PROBE_DELAY_MS (LWIP_RAND() % MDNS_PROBE_DELAY_MS)
#else
#define MDNS_INITIAL_PROBE_DELAY_MS MDNS_PROBE_DELAY_MS
#endif

#define MDNS_PROBE_TIEBREAK_CONFLICT_DELAY_MS    1000
#define MDNS_PROBE_TIEBREAK_MAX_ANSWERS          5

#define MDNS_LEXICOGRAPHICAL_EQUAL    0
#define MDNS_LEXICOGRAPHICAL_EARLIER  1
#define MDNS_LEXICOGRAPHICAL_LATER    2

/* Delay between successive announcements (RFC6762 section 8.3)
 * -> increase by a factor 2 with every response sent.
 */
#define MDNS_ANNOUNCE_DELAY_MS    1000
/* Minimum 2 announces, may send up to 8 (RFC6762 section 8.3) */
#define MDNS_ANNOUNCE_COUNT       2

/** Information about received packet */
struct mdns_packet {
  /** Sender IP/port */
  ip_addr_t source_addr;
  u16_t source_port;
  /** If packet was received unicast */
  u16_t recv_unicast;
  /** Packet data */
  struct pbuf *pbuf;
  /** Current parsing offset in packet */
  u16_t parse_offset;
  /** Identifier. Used in legacy queries */
  u16_t tx_id;
  /** Number of questions in packet,
   *  read from packet header */
  u16_t questions;
  /** Number of unparsed questions */
  u16_t questions_left;
  /** Number of answers in packet */
  u16_t answers;
  /** Number of unparsed answers */
  u16_t answers_left;
  /** Number of authoritative answers in packet */
  u16_t authoritative;
  /** Number of unparsed authoritative answers */
  u16_t authoritative_left;
  /** Number of additional answers in packet */
  u16_t additional;
  /** Number of unparsed additional answers */
  u16_t additional_left;
  /** Chained list of known answer received after a truncated question */
  struct mdns_packet *next_answer;
  /** Chained list of truncated question that are waiting */
  struct mdns_packet *next_tc_question;
};

/* list of received questions with TC flags set, waiting for known answers */
static struct mdns_packet *pending_tc_questions;

/* pool of received packets */
LWIP_MEMPOOL_DECLARE(MDNS_PKTS, MDNS_MAX_STORED_PKTS, sizeof (struct mdns_packet), "Stored mDNS packets")

struct mdns_question {
  struct mdns_rr_info info;
  /** unicast reply requested */
  u16_t unicast;
};

struct mdns_answer_list {
  u16_t offset[MDNS_PROBE_TIEBREAK_MAX_ANSWERS];
  u16_t size;
};

static err_t mdns_parse_pkt_questions(struct netif *netif,
                                      struct mdns_packet *pkt,
                                      struct mdns_outmsg *reply);
static void mdns_define_probe_rrs_to_send(struct netif *netif,
                                          struct mdns_outmsg *outmsg);
static void mdns_probe_and_announce(void* arg);
static void mdns_conflict_save_time(struct netif *netif);

/**
 *  Construction to make mdns struct accessible from mdns_out.c
 *  TODO:
 *  can we add the mdns struct to the netif like we do for dhcp, autoip,...?
 *  Then this is not needed any more.
 *
 *  @param netif  The network interface
 *  @return       mdns struct
 */
struct mdns_host*
netif_mdns_data(struct netif *netif) {
  return NETIF_TO_HOST(netif);
}

/**
 *  Construction to access the mdns udp pcb.
 *
 *  @return   udp_pcb struct of mdns
 */
struct udp_pcb*
get_mdns_pcb(void)
{
  return mdns_pcb;
}

/**
 * Check which replies we should send for a host/netif based on question
 * @param netif The network interface that received the question
 * @param rr Domain/type/class from a question
 * @param reverse_v6_reply Bitmask of which IPv6 addresses to send reverse PTRs for
 *                         if reply bit has REPLY_HOST_PTR_V6 set
 * @return Bitmask of which replies to send
 */
static int
check_host(struct netif *netif, struct mdns_rr_info *rr, u8_t *reverse_v6_reply)
{
  err_t res;
  int replies = 0;
  struct mdns_domain mydomain;

  LWIP_UNUSED_ARG(reverse_v6_reply); /* if ipv6 is disabled */

  if (rr->klass != DNS_RRCLASS_IN && rr->klass != DNS_RRCLASS_ANY) {
    /* Invalid class */
    return replies;
  }

  /* Handle PTR for our addresses */
  if (rr->type == DNS_RRTYPE_PTR || rr->type == DNS_RRTYPE_ANY) {
#if LWIP_IPV6
    int i;
    for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
      if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i))) {
        res = mdns_build_reverse_v6_domain(&mydomain, netif_ip6_addr(netif, i));
        if (res == ERR_OK && mdns_domain_eq(&rr->domain, &mydomain)) {
          replies |= REPLY_HOST_PTR_V6;
          /* Mark which addresses where requested */
          if (reverse_v6_reply) {
            *reverse_v6_reply |= (1 << i);
          }
        }
      }
    }
#endif
#if LWIP_IPV4
    if (!ip4_addr_isany_val(*netif_ip4_addr(netif))) {
      res = mdns_build_reverse_v4_domain(&mydomain, netif_ip4_addr(netif));
      if (res == ERR_OK && mdns_domain_eq(&rr->domain, &mydomain)) {
        replies |= REPLY_HOST_PTR_V4;
      }
    }
#endif
  }

  res = mdns_build_host_domain(&mydomain, NETIF_TO_HOST(netif));
  /* Handle requests for our hostname */
  if (res == ERR_OK && mdns_domain_eq(&rr->domain, &mydomain)) {
    /* TODO return NSEC if unsupported protocol requested */
#if LWIP_IPV4
    if (!ip4_addr_isany_val(*netif_ip4_addr(netif))
        && (rr->type == DNS_RRTYPE_A || rr->type == DNS_RRTYPE_ANY)) {
      replies |= REPLY_HOST_A;
    }
#endif
#if LWIP_IPV6
    if (rr->type == DNS_RRTYPE_AAAA || rr->type == DNS_RRTYPE_ANY) {
      replies |= REPLY_HOST_AAAA;
    }
#endif
  }

  return replies;
}

/**
 * Check which replies we should send for a service based on question
 * @param service A registered MDNS service
 * @param rr Domain/type/class from a question
 * @return Bitmask of which replies to send
 */
static int
check_service(struct mdns_service *service, struct mdns_rr_info *rr)
{
  err_t res;
  int replies = 0;
  struct mdns_domain mydomain;

  if (rr->klass != DNS_RRCLASS_IN && rr->klass != DNS_RRCLASS_ANY) {
    /* Invalid class */
    return 0;
  }

  res = mdns_build_dnssd_domain(&mydomain);
  if (res == ERR_OK && mdns_domain_eq(&rr->domain, &mydomain) &&
      (rr->type == DNS_RRTYPE_PTR || rr->type == DNS_RRTYPE_ANY)) {
    /* Request for all service types */
    replies |= REPLY_SERVICE_TYPE_PTR;
  }

  res = mdns_build_service_domain(&mydomain, service, 0);
  if (res == ERR_OK && mdns_domain_eq(&rr->domain, &mydomain) &&
      (rr->type == DNS_RRTYPE_PTR || rr->type == DNS_RRTYPE_ANY)) {
    /* Request for the instance of my service */
    replies |= REPLY_SERVICE_NAME_PTR;
  }

  res = mdns_build_service_domain(&mydomain, service, 1);
  if (res == ERR_OK && mdns_domain_eq(&rr->domain, &mydomain)) {
    /* Request for info about my service */
    if (rr->type == DNS_RRTYPE_SRV || rr->type == DNS_RRTYPE_ANY) {
      replies |= REPLY_SERVICE_SRV;
    }
    if (rr->type == DNS_RRTYPE_TXT || rr->type == DNS_RRTYPE_ANY) {
      replies |= REPLY_SERVICE_TXT;
    }
  }

  return replies;
}

#if LWIP_MDNS_SEARCH
/**
 * Check if question belong to a specified request
 * @param request A ongoing MDNS request
 * @param rr Domain/type/class from an answer
 * @return Bitmask of which matching replies
 */
static int
check_request(struct mdns_request *request, struct mdns_rr_info *rr)
{
  err_t res;
  int replies = 0;
  struct mdns_domain mydomain;

  if (rr->klass != DNS_RRCLASS_IN && rr->klass != DNS_RRCLASS_ANY) {
    /* Invalid class */
    return 0;
  }

  res = mdns_build_request_domain(&mydomain, request, 0);
  if (res == ERR_OK && mdns_domain_eq(&rr->domain, &mydomain) &&
      (rr->type == DNS_RRTYPE_PTR || rr->type == DNS_RRTYPE_ANY)) {
    /* Request for the instance of my service */
    replies |= REPLY_SERVICE_TYPE_PTR;
  }
  res = mdns_build_request_domain(&mydomain, request, 1);
  if (res == ERR_OK && mdns_domain_eq(&rr->domain, &mydomain)) {
    /* Request for info about my service */
    if (rr->type == DNS_RRTYPE_SRV || rr->type == DNS_RRTYPE_ANY) {
      replies |= REPLY_SERVICE_SRV;
    }
    if (rr->type == DNS_RRTYPE_TXT || rr->type == DNS_RRTYPE_ANY) {
      replies |= REPLY_SERVICE_TXT;
    }
  }
  return replies;
}
#endif

/**
 * Helper function for mdns_read_question/mdns_read_answer
 * Reads a domain, type and class from the packet
 * @param pkt The MDNS packet to read from. The parse_offset field will be
 *            incremented to point to the next unparsed byte.
 * @param info The struct to fill with domain, type and class
 * @return ERR_OK on success, an err_t otherwise
 */
static err_t
mdns_read_rr_info(struct mdns_packet *pkt, struct mdns_rr_info *info)
{
  u16_t field16, copied;
  pkt->parse_offset = mdns_readname(pkt->pbuf, pkt->parse_offset, &info->domain);
  if (pkt->parse_offset == MDNS_READNAME_ERROR) {
    return ERR_VAL;
  }

  copied = pbuf_copy_partial(pkt->pbuf, &field16, sizeof(field16), pkt->parse_offset);
  if (copied != sizeof(field16)) {
    return ERR_VAL;
  }
  pkt->parse_offset += copied;
  info->type = lwip_ntohs(field16);

  copied = pbuf_copy_partial(pkt->pbuf, &field16, sizeof(field16), pkt->parse_offset);
  if (copied != sizeof(field16)) {
    return ERR_VAL;
  }
  pkt->parse_offset += copied;
  info->klass = lwip_ntohs(field16);

  return ERR_OK;
}

/**
 * Read a question from the packet.
 * All questions have to be read before the answers.
 * @param pkt The MDNS packet to read from. The questions_left field will be decremented
 *            and the parse_offset will be updated.
 * @param question The struct to fill with question data
 * @return ERR_OK on success, an err_t otherwise
 */
static err_t
mdns_read_question(struct mdns_packet *pkt, struct mdns_question *question)
{
  /* Safety check */
  if (pkt->pbuf->tot_len < pkt->parse_offset) {
    return ERR_VAL;
  }

  if (pkt->questions_left) {
    err_t res;
    pkt->questions_left--;

    memset(question, 0, sizeof(struct mdns_question));
    res = mdns_read_rr_info(pkt, &question->info);
    if (res != ERR_OK) {
      return res;
    }

    /* Extract unicast flag from class field */
    question->unicast = question->info.klass & 0x8000;
    question->info.klass &= 0x7FFF;

    return ERR_OK;
  }
  return ERR_VAL;
}

/**
 * Read an answer from the packet
 * The variable length reply is not copied, its pbuf offset and length is stored instead.
 * @param pkt The MDNS packet to read. The num_left field will be decremented and
 *            the parse_offset will be updated.
 * @param answer    The struct to fill with answer data
 * @param num_left  number of answers left -> answers, authoritative or additional
 * @return ERR_OK on success, an err_t otherwise
 */
static err_t
mdns_read_answer(struct mdns_packet *pkt, struct mdns_answer *answer, u16_t *num_left)
{
  /* Read questions first */
  if (pkt->questions_left) {
    return ERR_VAL;
  }

  /* Safety check */
  if (pkt->pbuf->tot_len < pkt->parse_offset) {
    return ERR_VAL;
  }

  if (*num_left) {
    u16_t copied, field16;
    u32_t ttl;
    err_t res;
    (*num_left)--;

    memset(answer, 0, sizeof(struct mdns_answer));
    res = mdns_read_rr_info(pkt, &answer->info);
    if (res != ERR_OK) {
      return res;
    }

    /* Extract cache_flush flag from class field */
    answer->cache_flush = answer->info.klass & 0x8000;
    answer->info.klass &= 0x7FFF;

    copied = pbuf_copy_partial(pkt->pbuf, &ttl, sizeof(ttl), pkt->parse_offset);
    if (copied != sizeof(ttl)) {
      return ERR_VAL;
    }
    pkt->parse_offset += copied;
    answer->ttl = lwip_ntohl(ttl);

    copied = pbuf_copy_partial(pkt->pbuf, &field16, sizeof(field16), pkt->parse_offset);
    if (copied != sizeof(field16)) {
      return ERR_VAL;
    }
    pkt->parse_offset += copied;
    answer->rd_length = lwip_ntohs(field16);

    answer->rd_offset = pkt->parse_offset;
    pkt->parse_offset += answer->rd_length;

    return ERR_OK;
  }
  return ERR_VAL;
}

/**
 * Send unsolicited answer containing all our known data
 * @param netif The network interface to send on
 * @param destination The target address to send to (usually multicast address)
 */
static void
mdns_announce(struct netif *netif, const ip_addr_t *destination)
{
  struct mdns_outmsg announce;
  int i;
  struct mdns_host *mdns = NETIF_TO_HOST(netif);

  memset(&announce, 0, sizeof(announce));
  announce.cache_flush = 1;
#if LWIP_IPV4
  if (!ip4_addr_isany_val(*netif_ip4_addr(netif))) {
    announce.host_replies = REPLY_HOST_A | REPLY_HOST_PTR_V4;
  }
#endif
#if LWIP_IPV6
  for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
    if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i))) {
      announce.host_replies |= REPLY_HOST_AAAA | REPLY_HOST_PTR_V6;
      announce.host_reverse_v6_replies |= (1 << i);
    }
  }
#endif

  for (i = 0; i < MDNS_MAX_SERVICES; i++) {
    struct mdns_service *serv = mdns->services[i];
    if (serv) {
      announce.serv_replies[i] = REPLY_SERVICE_TYPE_PTR | REPLY_SERVICE_NAME_PTR |
                                 REPLY_SERVICE_SRV | REPLY_SERVICE_TXT;
    }
  }

  announce.dest_port = LWIP_IANA_PORT_MDNS;
  SMEMCPY(&announce.dest_addr, destination, sizeof(announce.dest_addr));
  announce.flags = DNS_FLAG1_RESPONSE | DNS_FLAG1_AUTHORATIVE;
  mdns_send_outpacket(&announce, netif);
}

/**
 * Perform lexicographical comparison to define the lexicographical order of the
 * records.
 *
 * @param pkt_a   first packet (needed for rr data)
 * @param pkt_b   second packet (needed for rr data)
 * @param ans_a   first rr
 * @param ans_b   second rr
 * @param result  pointer to save result in ->  MDNS_LEXICOGRAPHICAL_EQUAL,
 *                MDNS_LEXICOGRAPHICAL_LATER or MDNS_LEXICOGRAPHICAL_EARLIER.
 * @return err_t  ERR_OK if result is good, ERR_VAL if domain decompression failed.
 */
static err_t
mdns_lexicographical_comparison(struct mdns_packet *pkt_a, struct mdns_packet *pkt_b,
                                struct mdns_answer *ans_a, struct mdns_answer *ans_b,
                                u8_t *result)
{
  int len, i;
  u8_t a_rd, b_rd;
  u16_t res;
  struct mdns_domain domain_a, domain_b;

  /* Compare classes */
  if (ans_a->info.klass != ans_b->info.klass) {
    if (ans_a->info.klass > ans_b->info.klass) {
      *result = MDNS_LEXICOGRAPHICAL_LATER;
      return ERR_OK;
    }
    else {
      *result = MDNS_LEXICOGRAPHICAL_EARLIER;
      return ERR_OK;
    }
  }
  /* Compare types */
  if (ans_a->info.type != ans_b->info.type) {
    if (ans_a->info.type > ans_b->info.type) {
      *result = MDNS_LEXICOGRAPHICAL_LATER;
      return ERR_OK;
    }
    else {
      *result = MDNS_LEXICOGRAPHICAL_EARLIER;
      return ERR_OK;
    }
  }

  /* Compare rr data section
   * Name compression:
   * We have 4 different RR types in our authoritative section (if IPv4 and IPv6 is enabled): A,
   * AAAA, SRV and TXT. Only one of the 4 can be subject to name compression in the rdata, the SRV
   * record. As stated in the RFC6762 section 8.2: the names must be uncompressed before comparison.
   * We only need to take the SRV record into account. It's the only one that in a comparison with
   * compressed data could lead to rdata comparison. Others will already stop after the type
   * comparison. So if we get passed the class and type comparison we need to check if the
   * comparison contains an SRV record. If so, we need a different comparison method.
   */

  /* The answers do not contain an SRV record */
  if (ans_a->info.type != DNS_RRTYPE_SRV && ans_b->info.type != DNS_RRTYPE_SRV) {
    len = LWIP_MIN(ans_a->rd_length, ans_b->rd_length);
    for (i = 0; i < len; i++) {
      a_rd = pbuf_get_at(pkt_a->pbuf, (u16_t)(ans_a->rd_offset + i));
      b_rd = pbuf_get_at(pkt_b->pbuf, (u16_t)(ans_b->rd_offset + i));
      if (a_rd != b_rd) {
        if (a_rd > b_rd) {
          *result = MDNS_LEXICOGRAPHICAL_LATER;
          return ERR_OK;
        }
        else {
          *result = MDNS_LEXICOGRAPHICAL_EARLIER;
          return ERR_OK;
        }
      }
    }
    /* If the overlapping data is the same, compare the length */
    if (ans_a->rd_length != ans_b->rd_length) {
      if (ans_a->rd_length > ans_b->rd_length) {
        *result = MDNS_LEXICOGRAPHICAL_LATER;
        return ERR_OK;
      }
      else {
        *result = MDNS_LEXICOGRAPHICAL_EARLIER;
        return ERR_OK;
      }
    }
  }
  /* Because the types are guaranteed equal here, we know they are both SRV RRs */
  else {
    /* We will first compare the priority, weight and port */
    for (i = 0; i < 6; i++) {
      a_rd = pbuf_get_at(pkt_a->pbuf, (u16_t)(ans_a->rd_offset + i));
      b_rd = pbuf_get_at(pkt_b->pbuf, (u16_t)(ans_b->rd_offset + i));
      if (a_rd != b_rd) {
        if (a_rd > b_rd) {
          *result = MDNS_LEXICOGRAPHICAL_LATER;
          return ERR_OK;
        }
        else {
          *result = MDNS_LEXICOGRAPHICAL_EARLIER;
          return ERR_OK;
        }
      }
    }
    /* Decompress names if compressed and save in domain_a or domain_b */
    res = mdns_readname(pkt_a->pbuf, ans_a->rd_offset + 6, &domain_a);
    if (res == MDNS_READNAME_ERROR) {
      return ERR_VAL;
    }
    res = mdns_readname(pkt_b->pbuf, ans_b->rd_offset + 6, &domain_b);
    if (res == MDNS_READNAME_ERROR) {
      return ERR_VAL;
    }
    LWIP_DEBUGF(MDNS_DEBUG, ("mDNS: domain a: len = %d, name = ", domain_a.name[0]));
    mdns_domain_debug_print(&domain_a);
    LWIP_DEBUGF(MDNS_DEBUG, ("\n"));
    LWIP_DEBUGF(MDNS_DEBUG, ("mDNS: domain b: len = %d, name = ", domain_b.name[0]));
    mdns_domain_debug_print(&domain_b);
    LWIP_DEBUGF(MDNS_DEBUG, ("\n"));
    /* Compare names pairwise */
    len = LWIP_MIN(domain_a.length, domain_b.length);
    for (i = 0; i < len; i++) {
      if (domain_a.name[i] != domain_b.name[i]) {
        if (domain_a.name[i] > domain_b.name[i]) {
          *result = MDNS_LEXICOGRAPHICAL_LATER;
          return ERR_OK;
        }
        else {
          *result = MDNS_LEXICOGRAPHICAL_EARLIER;
          return ERR_OK;
        }
      }
    }
    /* If the overlapping data is the same, compare the length */
    if (domain_a.length != domain_b.length) {
      if (domain_a.length > domain_b.length) {
        *result = MDNS_LEXICOGRAPHICAL_LATER;
        return ERR_OK;
      }
      else {
        *result = MDNS_LEXICOGRAPHICAL_EARLIER;
        return ERR_OK;
      }
    }
  }
  /* They are exactly the same */
  *result = MDNS_LEXICOGRAPHICAL_EQUAL;
  return ERR_OK;
}

/**
 * Clear authoritative answer list
 *
 * @param a_list  answer list to clear
 */
static void
mdns_init_answer_list(struct mdns_answer_list *a_list)
{
  int i;
  a_list->size = 0;
  for(i = 0; i < MDNS_PROBE_TIEBREAK_MAX_ANSWERS; i++) {
    a_list->offset[i] = 0;
  }
}

/**
 * Pushes the offset of the answer on a lexicographically later sorted list.
 * We use a simple insertion sort because most of the time we are only sorting
 * two items. The answers are sorted from the smallest to the largest.
 *
 * @param a_list      Answer list to which to add the answer
 * @param pkt         Packet where answer originated
 * @param new_offset  Offset of the new answer in the packet
 * @param new_answer  The new answer
 * @return err_t ERR_MEM if list is full
 */
static err_t
mdns_push_answer_to_sorted_list(struct mdns_answer_list *a_list,
                                struct mdns_packet *pkt,
                                u16_t new_offset,
                                struct mdns_answer *new_answer)
{
  int i;
  struct mdns_answer a;
  int pos = a_list->size;
  err_t res = ERR_OK;
  u8_t result;
  u16_t num_left = pkt->authoritative;
  u16_t parse_offset = pkt->parse_offset;

  /* Check size */
  if ((a_list->size + 1) >= MDNS_PROBE_TIEBREAK_MAX_ANSWERS) {
    return ERR_MEM;
  }
  /* Search location and open a location */
  for (i = 0; i < a_list->size; i++) {
    /* Read answers already in the list from pkt */
    pkt->parse_offset = a_list->offset[i];
    res = mdns_read_answer(pkt, &a, &num_left);
    if (res != ERR_OK) {
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Failed to parse answer, skipping probe packet\n"));
      return res;
    }
    /* Compare them with the new answer to find it's place */
    res = mdns_lexicographical_comparison(pkt, pkt, &a, new_answer, &result);
    if (res != ERR_OK) {
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Failed to compare answers, skipping probe packet\n"));
      return res;
    }
    if (result == MDNS_LEXICOGRAPHICAL_LATER) {
      int j;
      pos = i;
      for (j = (a_list->size + 1); j>i; j--) {
        a_list->offset[j] = a_list->offset[j-1];
      }
      break;
    }
  }
  /* Insert new value */
  a_list->offset[pos] = new_offset;
  a_list->size++;
  /* Reset parse offset for further evaluation */
  pkt->parse_offset = parse_offset;
  return res;
}

/**
 * Check if the given answer answers the give question
 *
 * @param q     query to find answer for
 * @param a     answer to given query
 * @return      1 it a answers q, 0 if not
 */
static u8_t
mdns_is_answer_to_question(struct mdns_question *q, struct mdns_answer *a)
{
  if (q->info.type == DNS_RRTYPE_ANY || q->info.type == a->info.type) {
    /* The types match or question type is any */
    if (mdns_domain_eq(&q->info.domain, &a->info.domain)) {
      return 1;
    }
  }
  return 0;
}

/**
 * Converts the output packet to the input packet format for probe tiebreaking
 *
 * @param inpkt   destination packet for conversion
 * @param outpkt  source packet for conversion
 */
static void
mdns_convert_out_to_in_pkt(struct mdns_packet *inpkt, struct mdns_outpacket *outpkt)
{
  inpkt->pbuf = outpkt->pbuf;
  inpkt->parse_offset = SIZEOF_DNS_HDR;

  inpkt->questions = inpkt->questions_left = outpkt->questions;
  inpkt->answers = inpkt->answers_left = outpkt->answers;
  inpkt->authoritative = inpkt->authoritative_left = outpkt->authoritative;
  inpkt->additional = inpkt->additional_left = outpkt->additional;
}

/**
 * Debug print to print the answer part that is lexicographically compared
 *
 * @param pkt Packet where answer originated
 * @param a   The answer to print
 */
static void
mdns_debug_print_answer(struct mdns_packet *pkt, struct mdns_answer *a)
{
#ifdef LWIP_DEBUG
  /* Arbitrarily chose 200 -> don't want to see more then that. It's only
   * for debug so not that important. */
  char string[200];
  int i;
  int pos;

  pos = snprintf(string, sizeof(string), "Type = %2d, class = %1d, rdata = ", a->info.type, a->info.klass);
  for (i = 0; ((i < a->rd_length) && ((pos + 4*i) < 195)) ; i++) {
    snprintf(&string[pos + 4*i], 5, "%3d ", (u8_t)pbuf_get_at(pkt->pbuf, (u16_t)(a->rd_offset + i)));
  }
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: %s\n", string));
#else
  LWIP_UNUSED_ARG(pkt);
  LWIP_UNUSED_ARG(a);
#endif
}

/**
 * Perform probe tiebreaking according to RFC6762 section 8.2
 *
 * @param netif network interface of incoming packet
 * @param pkt   incoming packet
 */
static void
mdns_handle_probe_tiebreaking(struct netif *netif, struct mdns_packet *pkt)
{
  struct mdns_question pkt_q, my_q, q_dummy;
  struct mdns_answer pkt_a, my_a;
  struct mdns_outmsg myprobe_msg;
  struct mdns_outpacket myprobe_outpkt;
  struct mdns_packet myprobe_inpkt;
  struct mdns_answer_list pkt_a_list, my_a_list;
  u16_t save_parse_offset;
  u16_t pkt_parse_offset, myprobe_parse_offset, myprobe_questions_left;
  err_t res;
  u8_t match, result;
  int min, i;

  /* Generate probe packet to perform comparison.
   * This is a lot of calculation at this stage without any pre calculation
   * needed. It should be evaluated if this is the best approach.
   */
  mdns_define_probe_rrs_to_send(netif, &myprobe_msg);
  memset(&myprobe_outpkt, 0, sizeof(myprobe_outpkt));
  memset(&myprobe_inpkt, 0, sizeof(myprobe_inpkt));
  res = mdns_create_outpacket(netif, &myprobe_msg, &myprobe_outpkt);
  if (res != ERR_OK) {
    goto cleanup;
  }
  mdns_convert_out_to_in_pkt(&myprobe_inpkt, &myprobe_outpkt);

  /* Loop over all our probes to search for matches */
  while (myprobe_inpkt.questions_left) {
    /* Read one of our probe questions to check if pkt contains same question */
    res = mdns_read_question(&myprobe_inpkt, &my_q);
    if (res != ERR_OK) {
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Failed to parse question, skipping probe packet\n"));
      goto cleanup;
    }
    /* Remember parse offsets so we can restart the search for the next question */
    pkt_parse_offset = pkt->parse_offset;
    myprobe_parse_offset = myprobe_inpkt.parse_offset;
    /* Remember questions left of our probe packet */
    myprobe_questions_left = myprobe_inpkt.questions_left;
    /* Reset match flag */
    match = 0;
    /* Search for a matching probe in the incoming packet */
    while (pkt->questions_left) {
      /* Read probe questions one by one */
      res = mdns_read_question(pkt, &pkt_q);
      if (res != ERR_OK) {
        LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Failed to parse question, skipping probe packet\n"));
        goto cleanup;
      }
      /* Stop evaluating if the class is not supported */
      if (pkt_q.info.klass != DNS_RRCLASS_IN && pkt_q.info.klass != DNS_RRCLASS_ANY) {
        LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: question class not supported, skipping probe packet\n"));
        goto cleanup;
      }
      /* We probe for type any, so we do not have to compare types */
      /* Compare if we are probing for the same domain */
      if (mdns_domain_eq(&pkt_q.info.domain, &my_q.info.domain)) {
        LWIP_DEBUGF(MDNS_DEBUG, ("mDNS: We are probing for the same rr\n"));
        match = 1;
        break;
      }
    }
    /* When matched start evaluating the authoritative section */
    if (match) {
      /* Ignore all following questions to be able to get to the authoritative answers */
      while (pkt->questions_left) {
        res = mdns_read_question(pkt, &q_dummy);
        if (res != ERR_OK) {
          LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Failed to parse question, skipping probe packet\n"));
          goto cleanup;
        }
      }
      while (myprobe_inpkt.questions_left) {
        res = mdns_read_question(&myprobe_inpkt, &q_dummy);
        if (res != ERR_OK) {
          LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Failed to parse question, skipping probe packet\n"));
          goto cleanup;
        }
      }

      /* Extract and sort our authoritative answers that answer our question */
      mdns_init_answer_list(&my_a_list);
      while(myprobe_inpkt.authoritative_left) {
        save_parse_offset = myprobe_inpkt.parse_offset;
        res = mdns_read_answer(&myprobe_inpkt, &my_a, &myprobe_inpkt.authoritative_left);
        if (res != ERR_OK) {
          LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Failed to parse answer, skipping probe packet\n"));
          goto cleanup;
        }
        if (mdns_is_answer_to_question(&my_q, &my_a)) {
          /* Add to list */
          res = mdns_push_answer_to_sorted_list(&my_a_list, &myprobe_inpkt, save_parse_offset, &my_a);
          if (res != ERR_OK) {
            LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Failed to add answer, skipping probe packet\n"));
            goto cleanup;
          }
        }
      }
      /* Extract and sort the packets authoritative answers that answer the
         question */
      mdns_init_answer_list(&pkt_a_list);
      while(pkt->authoritative_left) {
        save_parse_offset = pkt->parse_offset;
        res = mdns_read_answer(pkt, &pkt_a, &pkt->authoritative_left);
        if (res != ERR_OK) {
          LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Failed to parse answer, skipping probe packet\n"));
          goto cleanup;
        }
        if (mdns_is_answer_to_question(&my_q, &pkt_a)) {
          /* Add to list */
          res = mdns_push_answer_to_sorted_list(&pkt_a_list, pkt, save_parse_offset, &pkt_a);
          if (res != ERR_OK) {
            LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Failed to add answer, skipping probe packet\n"));
            goto cleanup;
          }
        }
      }

      /* Reinitiate authoritative left */
      myprobe_inpkt.authoritative_left = myprobe_inpkt.authoritative;
      pkt->authoritative_left = pkt->authoritative;

      /* Compare pairwise.
       *  - lexicographically later? -> we win, ignore the packet.
       *  - lexicographically earlier? -> we loose, wait one second and retry.
       *  - lexicographically equal? -> no conflict, check other probes.
       */
      min = LWIP_MIN(my_a_list.size, pkt_a_list.size);
      for (i = 0; i < min; i++) {
        /* Get answer of our own list */
        myprobe_inpkt.parse_offset = my_a_list.offset[i];
        res = mdns_read_answer(&myprobe_inpkt, &my_a, &myprobe_inpkt.authoritative_left);
        if (res != ERR_OK) {
          LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Failed to parse answer, skipping probe packet\n"));
          goto cleanup;
        }
        /* Get answer of the packets list  */
        pkt->parse_offset = pkt_a_list.offset[i];
        res = mdns_read_answer(pkt, &pkt_a, &pkt->authoritative_left);
        if (res != ERR_OK) {
          LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Failed to parse answer, skipping probe packet\n"));
          goto cleanup;
        }
        /* Print both answers for debugging */
        mdns_debug_print_answer(pkt, &pkt_a);
        mdns_debug_print_answer(&myprobe_inpkt, &my_a);
        /* Define the winner */
        res = mdns_lexicographical_comparison(&myprobe_inpkt, pkt, &my_a, &pkt_a, &result);
        if (res != ERR_OK) {
          LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Failed to compare answers, skipping probe packet\n"));
          goto cleanup;
        }
        if (result == MDNS_LEXICOGRAPHICAL_LATER) {
          LWIP_DEBUGF(MDNS_DEBUG, ("mDNS: we win, we are lexicographically later\n"));
          goto cleanup;
        }
        else if (result == MDNS_LEXICOGRAPHICAL_EARLIER) {
          LWIP_DEBUGF(MDNS_DEBUG, ("mDNS: we loose, we are lexicographically earlier. 1s timeout started\n"));
          /* Increase the number of conflicts occurred */
          mdns_conflict_save_time(netif);
          /* then restart with 1s delay */
          mdns_resp_restart_delay(netif, MDNS_PROBE_TIEBREAK_CONFLICT_DELAY_MS);
          goto cleanup;
        }
        else {
          LWIP_DEBUGF(MDNS_DEBUG, ("mDNS: lexicographically equal, so no conclusion\n"));
        }
      }
      /* All compared RR were equal, otherwise we would not be here
       * -> check if one of both have more answers to the question */
      if (my_a_list.size != pkt_a_list.size) {
        if (my_a_list.size > pkt_a_list.size) {
          LWIP_DEBUGF(MDNS_DEBUG, ("mDNS: we win, we have more records answering the probe\n"));
          goto cleanup;
        }
        else {
          LWIP_DEBUGF(MDNS_DEBUG, ("mDNS: we loose, we have less records. 1s timeout started\n"));
          /* Increase the number of conflicts occurred */
          mdns_conflict_save_time(netif);
          /* then restart with 1s delay */
          mdns_resp_restart_delay(netif, MDNS_PROBE_TIEBREAK_CONFLICT_DELAY_MS);
          goto cleanup;
        }
      }
      else {
        /* There is no conflict on this probe, both devices have the same data
         * in the authoritative section. We should still check the other probes
         * for conflicts. */
        LWIP_DEBUGF(MDNS_DEBUG, ("mDNS: no conflict, all records answering the probe are equal\n"));
      }
    }
    /* Evaluate other probes if any. */
    /* Reinitiate parse offsets */
    pkt->parse_offset = pkt_parse_offset;
    myprobe_inpkt.parse_offset = myprobe_parse_offset;
    /* Reinitiate questions_left and authoritative_left */
    pkt->questions_left = pkt->questions;
    pkt->authoritative_left = pkt->authoritative;
    myprobe_inpkt.questions_left = myprobe_questions_left;
    myprobe_inpkt.authoritative_left = myprobe_inpkt.authoritative;
  }

cleanup:
  if (myprobe_inpkt.pbuf != NULL) {
    pbuf_free(myprobe_inpkt.pbuf);
  }
}

/**
 * Check the incoming packet and parse all questions
 *
 * @param netif network interface of incoming packet
 * @param pkt   incoming packet
 * @param reply outgoing message
 * @return err_t
 */
static err_t
mdns_parse_pkt_questions(struct netif *netif, struct mdns_packet *pkt,
                         struct mdns_outmsg *reply)
{
  struct mdns_host *mdns = NETIF_TO_HOST(netif);
  struct mdns_service *service;
  int i;
  err_t res;

  while (pkt->questions_left) {
    struct mdns_question q;

    res = mdns_read_question(pkt, &q);
    if (res != ERR_OK) {
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Failed to parse question, skipping query packet\n"));
      return res;
    }

    LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Query for domain "));
    mdns_domain_debug_print(&q.info.domain);
    LWIP_DEBUGF(MDNS_DEBUG, (" type %d class %d\n", q.info.type, q.info.klass));

    if (q.unicast) {
      /* Reply unicast if it is requested in the question */
      reply->unicast_reply_requested = 1;
    }

    reply->host_replies |= check_host(netif, &q.info, &reply->host_reverse_v6_replies);

    for (i = 0; i < MDNS_MAX_SERVICES; i++) {
      service = mdns->services[i];
      if (!service) {
        continue;
      }
      reply->serv_replies[i] |= check_service(service, &q.info);
    }
  }

  return ERR_OK;
}

/**
 * Check the incoming packet and parse all (known) answers
 *
 * @param netif network interface of incoming packet
 * @param pkt   incoming packet
 * @param reply outgoing message
 * @return err_t
 */
static err_t
mdns_parse_pkt_known_answers(struct netif *netif, struct mdns_packet *pkt,
                             struct mdns_outmsg *reply)
{
  struct mdns_host *mdns = NETIF_TO_HOST(netif);
  struct mdns_service *service;
  int i;
  err_t res;

  while (pkt->answers_left) {
    struct mdns_answer ans;
    u8_t rev_v6;
    int match;
    u32_t rr_ttl = MDNS_TTL_120;

    res = mdns_read_answer(pkt, &ans, &pkt->answers_left);
    if (res != ERR_OK) {
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Failed to parse answer, skipping query packet\n"));
      return res;
    }

    LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Known answer for domain "));
    mdns_domain_debug_print(&ans.info.domain);
    LWIP_DEBUGF(MDNS_DEBUG, (" type %d class %d\n", ans.info.type, ans.info.klass));


    if (ans.info.type == DNS_RRTYPE_ANY || ans.info.klass == DNS_RRCLASS_ANY) {
      /* Skip known answers for ANY type & class */
      continue;
    }

    rev_v6 = 0;
    match = reply->host_replies & check_host(netif, &ans.info, &rev_v6);
    if (match && (ans.ttl > (rr_ttl / 2))) {
      /* The RR in the known answer matches an RR we are planning to send,
       * and the TTL is less than half gone.
       * If the payload matches we should not send that answer.
       */
      if (ans.info.type == DNS_RRTYPE_PTR) {
        /* Read domain and compare */
        struct mdns_domain known_ans, my_ans;
        u16_t len;
        len = mdns_readname(pkt->pbuf, ans.rd_offset, &known_ans);
        res = mdns_build_host_domain(&my_ans, mdns);
        if (len != MDNS_READNAME_ERROR && res == ERR_OK && mdns_domain_eq(&known_ans, &my_ans)) {
#if LWIP_IPV4
          if (match & REPLY_HOST_PTR_V4) {
            LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Skipping known answer: v4 PTR\n"));
            reply->host_replies &= ~REPLY_HOST_PTR_V4;
          }
#endif
#if LWIP_IPV6
          if (match & REPLY_HOST_PTR_V6) {
            LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Skipping known answer: v6 PTR\n"));
            reply->host_reverse_v6_replies &= ~rev_v6;
            if (reply->host_reverse_v6_replies == 0) {
              reply->host_replies &= ~REPLY_HOST_PTR_V6;
            }
          }
#endif
        }
      } else if (match & REPLY_HOST_A) {
#if LWIP_IPV4
        if (ans.rd_length == sizeof(ip4_addr_t) &&
            pbuf_memcmp(pkt->pbuf, ans.rd_offset, netif_ip4_addr(netif), ans.rd_length) == 0) {
          LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Skipping known answer: A\n"));
          reply->host_replies &= ~REPLY_HOST_A;
        }
#endif
      } else if (match & REPLY_HOST_AAAA) {
#if LWIP_IPV6
        if (ans.rd_length == sizeof(ip6_addr_p_t) &&
            /* TODO this clears all AAAA responses if first addr is set as known */
            pbuf_memcmp(pkt->pbuf, ans.rd_offset, netif_ip6_addr(netif, 0), ans.rd_length) == 0) {
          LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Skipping known answer: AAAA\n"));
          reply->host_replies &= ~REPLY_HOST_AAAA;
        }
#endif
      }
    }

    for (i = 0; i < MDNS_MAX_SERVICES; i++) {
      service = mdns->services[i];
      if (!service) {
        continue;
      }
      match = reply->serv_replies[i] & check_service(service, &ans.info);
      if (match & REPLY_SERVICE_TYPE_PTR) {
        rr_ttl = MDNS_TTL_4500;
      }
      if (match && (ans.ttl > (rr_ttl / 2))) {
        /* The RR in the known answer matches an RR we are planning to send,
         * and the TTL is less than half gone.
         * If the payload matches we should not send that answer.
         */
        if (ans.info.type == DNS_RRTYPE_PTR) {
          /* Read domain and compare */
          struct mdns_domain known_ans, my_ans;
          u16_t len;
          len = mdns_readname(pkt->pbuf, ans.rd_offset, &known_ans);
          if (len != MDNS_READNAME_ERROR) {
            if (match & REPLY_SERVICE_TYPE_PTR) {
              res = mdns_build_service_domain(&my_ans, service, 0);
              if (res == ERR_OK && mdns_domain_eq(&known_ans, &my_ans)) {
                LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Skipping known answer: service type PTR\n"));
                reply->serv_replies[i] &= ~REPLY_SERVICE_TYPE_PTR;
              }
            }
            if (match & REPLY_SERVICE_NAME_PTR) {
              res = mdns_build_service_domain(&my_ans, service, 1);
              if (res == ERR_OK && mdns_domain_eq(&known_ans, &my_ans)) {
                LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Skipping known answer: service name PTR\n"));
                reply->serv_replies[i] &= ~REPLY_SERVICE_NAME_PTR;
              }
            }
          }
        } else if (match & REPLY_SERVICE_SRV) {
          /* Read and compare to my SRV record */
          u16_t field16, len, read_pos;
          struct mdns_domain known_ans, my_ans;
          read_pos = ans.rd_offset;
          do {
            /* Check priority field */
            len = pbuf_copy_partial(pkt->pbuf, &field16, sizeof(field16), read_pos);
            if (len != sizeof(field16) || lwip_ntohs(field16) != SRV_PRIORITY) {
              break;
            }
            read_pos += len;
            /* Check weight field */
            len = pbuf_copy_partial(pkt->pbuf, &field16, sizeof(field16), read_pos);
            if (len != sizeof(field16) || lwip_ntohs(field16) != SRV_WEIGHT) {
              break;
            }
            read_pos += len;
            /* Check port field */
            len = pbuf_copy_partial(pkt->pbuf, &field16, sizeof(field16), read_pos);
            if (len != sizeof(field16) || lwip_ntohs(field16) != service->port) {
              break;
            }
            read_pos += len;
            /* Check host field */
            len = mdns_readname(pkt->pbuf, read_pos, &known_ans);
            mdns_build_host_domain(&my_ans, mdns);
            if (len == MDNS_READNAME_ERROR || !mdns_domain_eq(&known_ans, &my_ans)) {
              break;
            }
            LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Skipping known answer: SRV\n"));
            reply->serv_replies[i] &= ~REPLY_SERVICE_SRV;
          } while (0);
        } else if (match & REPLY_SERVICE_TXT) {
          mdns_prepare_txtdata(service);
          if (service->txtdata.length == ans.rd_length &&
              pbuf_memcmp(pkt->pbuf, ans.rd_offset, service->txtdata.name, ans.rd_length) == 0) {
            LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Skipping known answer: TXT\n"));
            reply->serv_replies[i] &= ~REPLY_SERVICE_TXT;
          }
        }
      }
    }
  }

  return ERR_OK;
}

/**
 * Check the incoming packet and parse all authoritative answers to see if the
 * query is a probe query.
 *
 * @param netif network interface of incoming packet
 * @param pkt   incoming packet
 * @param reply outgoing message
 * @return err_t
 */
static err_t
mdns_parse_pkt_authoritative_answers(struct netif *netif, struct mdns_packet *pkt,
                                     struct mdns_outmsg *reply)
{
  struct mdns_host *mdns = NETIF_TO_HOST(netif);
  struct mdns_service *service;
  int i;
  err_t res;

  while (pkt->authoritative_left) {
    struct mdns_answer ans;
    u8_t rev_v6;
    int match;

    res = mdns_read_answer(pkt, &ans, &pkt->authoritative_left);
    if (res != ERR_OK) {
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Failed to parse answer, skipping query packet\n"));
      return res;
    }

    LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Authoritative answer for domain "));
    mdns_domain_debug_print(&ans.info.domain);
    LWIP_DEBUGF(MDNS_DEBUG, (" type %d class %d\n", ans.info.type, ans.info.klass));


    if (ans.info.type == DNS_RRTYPE_ANY || ans.info.klass == DNS_RRCLASS_ANY) {
      /* Skip known answers for ANY type & class */
      continue;
    }

    rev_v6 = 0;
    match = reply->host_replies & check_host(netif, &ans.info, &rev_v6);
    if (match) {
      reply->probe_query_recv = 1;
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Probe for own host info received\n"));
    }

    for (i = 0; i < MDNS_MAX_SERVICES; i++) {
      service = mdns->services[i];
      if (!service) {
        continue;
      }
      match = reply->serv_replies[i] & check_service(service, &ans.info);

      if (match) {
        reply->probe_query_recv = 1;
        LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Probe for own service info received\n"));
      }
    }
  }

  return ERR_OK;
}

/**
 * Add / copy message to delaying message buffer.
 *
 * @param dest destination msg struct
 * @param src  source msg struct
 */
static void
mdns_add_msg_to_delayed(struct mdns_outmsg *dest, struct mdns_outmsg *src)
{
  int i;

  dest->host_questions |= src->host_questions;
  dest->host_replies |= src->host_replies;
  dest->host_reverse_v6_replies |= src->host_reverse_v6_replies;
  for (i = 0; i < MDNS_MAX_SERVICES; i++) {
    dest->serv_questions[i] |= src->serv_questions[i];
    dest->serv_replies[i] |= src->serv_replies[i];
  }

  dest->flags = src->flags;
  dest->cache_flush = src->cache_flush;
  dest->tx_id = src->tx_id;
  dest->legacy_query = src->legacy_query;
}

/**
 * Handle question MDNS packet
 * - Perform probe tiebreaking when in probing state
 * - Parse all questions and set bits what answers to send
 * - Clear pending answers if known answers are supplied
 * - Define which type of answer is requested
 * - Send out packet or put it on hold until after random time
 *
 * @param pkt   incoming packet (in stack)
 * @param netif network interface of incoming packet
 */
static void
mdns_handle_question(struct mdns_packet *pkt, struct netif *netif)
{
  struct mdns_host *mdns = NETIF_TO_HOST(netif);
  struct mdns_outmsg reply;
  u8_t rrs_to_send;
  u8_t shared_answer = 0;
  u8_t delay_response = 1;
  u8_t send_unicast = 0;
  u8_t listen_to_QU_bit = 0;
  int i;
  err_t res;

  if ((mdns->state == MDNS_STATE_PROBING) ||
      (mdns->state == MDNS_STATE_ANNOUNCE_WAIT)) {
    /* Probe Tiebreaking */
    /* Check if packet is a probe message */
    if ((pkt->questions > 0) && (pkt->answers == 0) &&
        (pkt->authoritative > 0) && (pkt->additional == 0)) {
      /* This should be a probe message -> call probe handler */
      mdns_handle_probe_tiebreaking(netif, pkt);
    }
  }

  if ((mdns->state != MDNS_STATE_COMPLETE) &&
      (mdns->state != MDNS_STATE_ANNOUNCING)) {
    /* Don't answer questions until we've verified our domains via probing */
    /* @todo we should check incoming questions during probing for tiebreaking */
    return;
  }

  memset(&reply, 0, sizeof(struct mdns_outmsg));

  /* Parse question */
  res = mdns_parse_pkt_questions(netif, pkt, &reply);
  if (res != ERR_OK) {
    return;
  }
  /* Parse answers -> count as known answers because it's a question */
  res = mdns_parse_pkt_known_answers(netif, pkt, &reply);
  if (res != ERR_OK) {
    return;
  }
  if (pkt->next_answer) {
    /* Also parse known-answers from additional packets */
    struct mdns_packet *pkta = pkt->next_answer;
    while (pkta) {
      res = mdns_parse_pkt_known_answers(netif, pkta, &reply);
      if (res != ERR_OK) {
        return;
      }
      pkta = pkta->next_answer;
    }
  }
  /* Parse authoritative answers -> probing */
  /* If it's a probe query, we need to directly answer via unicast. */
  res = mdns_parse_pkt_authoritative_answers(netif, pkt, &reply);
  if (res != ERR_OK) {
    return;
  }
  /* Ignore additional answers -> do not have any need for them at the moment */
  if(pkt->additional) {
    LWIP_DEBUGF(MDNS_DEBUG,
      ("MDNS: Query contains additional answers -> they are discarded\n"));
  }

  /* Any replies on question? */
  rrs_to_send = reply.host_replies | reply.host_questions;
  for (i = 0; i < MDNS_MAX_SERVICES; i++) {
    rrs_to_send |= reply.serv_replies[i] | reply.serv_questions[i];
  }

  if (!rrs_to_send) {
    /* This case is most common */
    LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Nothing to answer\n"));
    return;
  }

  reply.flags =  DNS_FLAG1_RESPONSE | DNS_FLAG1_AUTHORATIVE;

  /* Detect if it's a legacy querier asking the question
   * How to detect legacy DNS query? (RFC6762 section 6.7)
   *  - source port != 5353
   *  - a legacy query can only contain 1 question
   */
  if (pkt->source_port != LWIP_IANA_PORT_MDNS) {
    if (pkt->questions == 1) {
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: request from legacy querier\n"));
      reply.legacy_query = 1;
      reply.tx_id = pkt->tx_id;
      reply.cache_flush = 0;
    }
    else {
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: ignore query if (src UDP port != 5353) && (!= legacy query)\n"));
      return;
    }
  }
  else {
    reply.cache_flush = 1;
  }

  /* Delaying response. (RFC6762 section 6)
   * Always delay the response, unicast or multicast, except when:
   *  - Answering to a single question with a unique answer (not a probe).
   *  - Answering to a probe query via unicast.
   *  - Answering to a probe query via multicast if not multicasted within 250ms.
   *
   * unique answer? -> not if it includes service type or name ptr's
   */
  for (i = 0; i < MDNS_MAX_SERVICES; i++) {
    shared_answer |= (reply.serv_replies[i] &
                      (REPLY_SERVICE_TYPE_PTR | REPLY_SERVICE_NAME_PTR));
  }
  if (   ((pkt->questions == 1) && (!shared_answer) && !reply.probe_query_recv)
      || (reply.probe_query_recv && reply.unicast_reply_requested)) {
    delay_response = 0;
  }
#if LWIP_IPV6
  if (IP_IS_V6_VAL(pkt->source_addr) && reply.probe_query_recv
      && !reply.unicast_reply_requested && !mdns->ipv6.multicast_probe_timeout) {
    delay_response = 0;
  }
#endif
#if LWIP_IPV4
  if (IP_IS_V4_VAL(pkt->source_addr) && reply.probe_query_recv
      && !reply.unicast_reply_requested && !mdns->ipv4.multicast_probe_timeout) {
    delay_response = 0;
  }
#endif
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: response %s delayed\n", (delay_response ? "randomly" : "not")));

  /* Unicast / multicast response:
   * Answering to (m)DNS querier via unicast response.
   * When:
   *  a) Unicast reply requested && recently multicasted 1/4ttl (RFC6762 section 5.4)
   *  b) Direct unicast query to port 5353 (RFC6762 section 5.5)
   *  c) Reply to Legacy DNS querier (RFC6762 section 6.7)
   *  d) A probe message is received requesting unicast (RFC6762 section 6)
   */

#if LWIP_IPV6
  if ((IP_IS_V6_VAL(pkt->source_addr) && mdns->ipv6.multicast_timeout_25TTL)) {
    listen_to_QU_bit = 1;
  }
#endif
#if LWIP_IPV4
  if ((IP_IS_V4_VAL(pkt->source_addr) && mdns->ipv4.multicast_timeout_25TTL)) {
    listen_to_QU_bit = 1;
  }
#endif
  if (   (reply.unicast_reply_requested && listen_to_QU_bit)
      || pkt->recv_unicast
      || reply.legacy_query
      || (reply.probe_query_recv && reply.unicast_reply_requested)) {
    send_unicast = 1;
  }
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: send response via %s\n", (send_unicast ? "unicast" : "multicast")));

  /* Send out or put on waiting list */
  if (delay_response) {
    if (send_unicast) {
#if LWIP_IPV6
      /* Add answers to IPv6 waiting list if:
       *  - it's a IPv6 incoming packet
       *  - no message is in it yet
       */
      if (IP_IS_V6_VAL(pkt->source_addr) && !mdns->ipv6.unicast_msg_in_use) {
        LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: add answers to unicast IPv6 waiting list\n"));
        SMEMCPY(&mdns->ipv6.delayed_msg_unicast.dest_addr, &pkt->source_addr, sizeof(ip_addr_t));
        mdns->ipv6.delayed_msg_unicast.dest_port = pkt->source_port;

        mdns_add_msg_to_delayed(&mdns->ipv6.delayed_msg_unicast, &reply);

        mdns_set_timeout(netif, MDNS_RESPONSE_DELAY, mdns_send_unicast_msg_delayed_ipv6,
                         &mdns->ipv6.unicast_msg_in_use);
      }
#endif
#if LWIP_IPV4
      /* Add answers to IPv4 waiting list if:
       *  - it's a IPv4 incoming packet
       *  - no message is in it yet
       */
      if (IP_IS_V4_VAL(pkt->source_addr) && !mdns->ipv4.unicast_msg_in_use) {
        LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: add answers to unicast IPv4 waiting list\n"));
        SMEMCPY(&mdns->ipv4.delayed_msg_unicast.dest_addr, &pkt->source_addr, sizeof(ip_addr_t));
        mdns->ipv4.delayed_msg_unicast.dest_port = pkt->source_port;

        mdns_add_msg_to_delayed(&mdns->ipv4.delayed_msg_unicast, &reply);

        mdns_set_timeout(netif, MDNS_RESPONSE_DELAY, mdns_send_unicast_msg_delayed_ipv4,
                         &mdns->ipv4.unicast_msg_in_use);
      }
#endif
    }
    else {
#if LWIP_IPV6
      /* Add answers to IPv6 waiting list if:
       *  - it's a IPv6 incoming packet
       *  - the 1 second timeout is passed (RFC6762 section 6)
       *  - and it's not a probe packet
       * Or if:
       *  - it's a IPv6 incoming packet
       *  - and it's a probe packet
       */
      if (IP_IS_V6_VAL(pkt->source_addr) && !mdns->ipv6.multicast_timeout
          && !reply.probe_query_recv) {
        LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: add answers to multicast IPv6 waiting list\n"));

        mdns_add_msg_to_delayed(&mdns->ipv6.delayed_msg_multicast, &reply);

        mdns_set_timeout(netif, MDNS_RESPONSE_DELAY, mdns_send_multicast_msg_delayed_ipv6,
                         &mdns->ipv6.multicast_msg_waiting);
      }
      else if (IP_IS_V6_VAL(pkt->source_addr) && reply.probe_query_recv) {
        LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: add answers to probe multicast IPv6 waiting list\n"));

        mdns_add_msg_to_delayed(&mdns->ipv6.delayed_msg_multicast, &reply);

        mdns->ipv6.multicast_msg_waiting = 1;
      }
#endif
#if LWIP_IPV4
      /* Add answers to IPv4 waiting list if:
       *  - it's a IPv4 incoming packet
       *  - the 1 second timeout is passed (RFC6762 section 6)
       *  - and it's not a probe packet
       * Or if:
       *  - it's a IPv4 incoming packet
       *  - and it's a probe packet
       */
      if (IP_IS_V4_VAL(pkt->source_addr) && !mdns->ipv4.multicast_timeout
          && !reply.probe_query_recv) {
        LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: add answers to multicast IPv4 waiting list\n"));

        mdns_add_msg_to_delayed(&mdns->ipv4.delayed_msg_multicast, &reply);

        mdns_set_timeout(netif, MDNS_RESPONSE_DELAY, mdns_send_multicast_msg_delayed_ipv4,
                         &mdns->ipv4.multicast_msg_waiting);
      }
      else if (IP_IS_V4_VAL(pkt->source_addr) && reply.probe_query_recv) {
        LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: add answers to probe multicast IPv4 waiting list\n"));

        mdns_add_msg_to_delayed(&mdns->ipv4.delayed_msg_multicast, &reply);

        mdns->ipv4.multicast_msg_waiting = 1;
      }
#endif
    }
  }
  else {
    if (send_unicast) {
      /* Copy source IP/port to use when responding unicast */
      SMEMCPY(&reply.dest_addr, &pkt->source_addr, sizeof(ip_addr_t));
      reply.dest_port = pkt->source_port;
      /* send answer directly via unicast */
      res = mdns_send_outpacket(&reply, netif);
      if (res != ERR_OK) {
        LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Unicast answer could not be send\n"));
      }
      else {
        LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Unicast answer send successfully\n"));
      }
      return;
    }
    else {
      /* Set IP/port to use when responding multicast */
#if LWIP_IPV6
      if (IP_IS_V6_VAL(pkt->source_addr)) {
        if (mdns->ipv6.multicast_timeout && !reply.probe_query_recv) {
          LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: we just multicasted, ignore question\n"));
          return;
        }
        SMEMCPY(&reply.dest_addr, &v6group, sizeof(ip_addr_t));
      }
#endif
#if LWIP_IPV4
      if (IP_IS_V4_VAL(pkt->source_addr)) {
        if (mdns->ipv4.multicast_timeout && !reply.probe_query_recv) {
          LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: we just multicasted, ignore question\n"));
          return;
        }
        SMEMCPY(&reply.dest_addr, &v4group, sizeof(ip_addr_t));
      }
#endif
      reply.dest_port = LWIP_IANA_PORT_MDNS;
      /* send answer directly via multicast */
      res = mdns_send_outpacket(&reply, netif);
      if (res != ERR_OK) {
        LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Multicast answer could not be send\n"));
      }
      else {
        LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Multicast answer send successfully\n"));
#if LWIP_IPV6
        if (IP_IS_V6_VAL(pkt->source_addr)) {
          mdns_start_multicast_timeouts_ipv6(netif);
        }
#endif
#if LWIP_IPV4
        if (IP_IS_V4_VAL(pkt->source_addr)) {
          mdns_start_multicast_timeouts_ipv4(netif);
        }
#endif
      }
      return;
    }
  }
}

/**
 * Handle truncated question MDNS packet
 * - Called by timer
 * - Call mdns_handle_question
 * - Do cleanup
 *
 * @param arg   incoming packet (in pool)
 */
static void
mdns_handle_tc_question(void *arg)
{
  struct mdns_packet *pkt = (struct mdns_packet *)arg;
  struct netif *from = netif_get_by_index(pkt->pbuf->if_idx);
  /* timer as elapsed, now handle this question */
  mdns_handle_question(pkt, from);
  /* remove from pending list */
  if (pending_tc_questions == pkt) {
    pending_tc_questions = pkt->next_tc_question;
  }
  else {
    struct mdns_packet *prev = pending_tc_questions;
    while (prev && prev->next_tc_question != pkt) {
      prev = prev->next_tc_question;
    }
    LWIP_ASSERT("pkt not found in pending_tc_questions list", prev != NULL);
    prev->next_tc_question = pkt->next_tc_question;
  }
  /* free linked answers and this question */
  while (pkt->next_answer) {
    struct mdns_packet *ans = pkt->next_answer;
    pkt->next_answer = ans->next_answer;
    pbuf_free(ans->pbuf);
    LWIP_MEMPOOL_FREE(MDNS_PKTS, ans);
  }
  pbuf_free(pkt->pbuf);
  LWIP_MEMPOOL_FREE(MDNS_PKTS, pkt);
}

/**
 * Save time when a probe conflict occurs:
 *  - Check if we exceeded the maximum of 15 conflicts in 10seconds.
 *
 * @param netif network interface on which the conflict occurred.
 */
static void
mdns_conflict_save_time(struct netif *netif)
{
  struct mdns_host* mdns = NETIF_TO_HOST(netif);
  int i;
  u32_t diff;
  u8_t index2;

  /* Increase the number of conflicts occurred */
  mdns->num_conflicts++;
  mdns->conflict_time[mdns->index] = sys_now();
  /* Print timestamp list */
  LWIP_DEBUGF(MDNS_DEBUG, ("mDNS: conflict timestamp list, insert index = %d\n", mdns->index));
  for(i = 0; i < MDNS_PROBE_MAX_CONFLICTS_BEFORE_RATE_LIMIT; i++) {
    LWIP_DEBUGF(MDNS_DEBUG, ("mDNS: time no. %d = %"U32_F"\n", i, mdns->conflict_time[i]));
  }
  /* Check if we had enough conflicts, minimum 15 */
  if (mdns->num_conflicts >= MDNS_PROBE_MAX_CONFLICTS_BEFORE_RATE_LIMIT) {
    /* Get the index to the oldest timestamp */
    index2 = (mdns->index + 1) % MDNS_PROBE_MAX_CONFLICTS_BEFORE_RATE_LIMIT;
    /* Compare the oldest vs newest time stamp */
    diff = mdns->conflict_time[mdns->index] - mdns->conflict_time[index2];
    /* If they are less then 10 seconds apart, initiate rate limit */
    if (diff < MDNS_PROBE_MAX_CONFLICTS_TIME_WINDOW) {
      LWIP_DEBUGF(MDNS_DEBUG, ("mDNS: probe rate limit enabled\n"));
      mdns->rate_limit_activated = 1;
    }
  }
  /* Increase index */
  mdns->index = (mdns->index + 1) % MDNS_PROBE_MAX_CONFLICTS_BEFORE_RATE_LIMIT;
}

/**
 * Handle a probe conflict:
 *  - Check if we exceeded the maximum of 15 conflicts in 10seconds.
 *  - Let the user know there is a conflict.
 *
 * @param netif network interface on which the conflict occurred.
 * @param slot service index +1 on which the conflict occurred (0 indicate hostname conflict).
 */
static void
mdns_probe_conflict(struct netif *netif, s8_t slot)
{
  /* Increase the number of conflicts occurred and check rate limiting */
  mdns_conflict_save_time(netif);

  /* Disable currently running probe / announce timer */
  sys_untimeout(mdns_probe_and_announce, netif);

  /* Inform the host on the conflict, if a callback is set */
  if (mdns_name_result_cb != NULL) {
    mdns_name_result_cb(netif, MDNS_PROBING_CONFLICT, slot);
  }
  /* TODO: rename and call restart if no mdns_name_result_cb was set? */
}

/**
 * Loockup matching request for response MDNS packet
 */
#if LWIP_MDNS_SEARCH
static struct mdns_request *
mdns_lookup_request(struct mdns_rr_info *rr)
{
  int i;
  /* search originating request */
  for (i = 0; i < MDNS_MAX_REQUESTS; i++) {
    if ((mdns_requests[i].result_fn != NULL) &&
        (check_request(&mdns_requests[i], rr) != 0)) {
      return &mdns_requests[i];
    }
  }
  return NULL;
}
#endif

/**
 * Handle response MDNS packet:
 *  - Handle responses on probe query
 *  - Perform conflict resolution on every packet (RFC6762 section 9)
 *
 * @param pkt   incoming packet
 * @param netif network interface on which packet was received
 */
static void
mdns_handle_response(struct mdns_packet *pkt, struct netif *netif)
{
  struct mdns_host* mdns = NETIF_TO_HOST(netif);
  u16_t total_answers_left;
#if LWIP_MDNS_SEARCH
  struct mdns_request *req = NULL;
  s8_t first = 1;
#endif

  /* Ignore responses with a source port different from 5353
   * (LWIP_IANA_PORT_MDNS) -> RFC6762 section 6 */
  if (pkt->source_port != LWIP_IANA_PORT_MDNS) {
    return;
  }

  /* Ignore all questions */
  while (pkt->questions_left) {
    struct mdns_question q;
    err_t res;
    res = mdns_read_question(pkt, &q);
    if (res != ERR_OK) {
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Failed to parse question, skipping response packet\n"));
      return;
    }
#if LWIP_MDNS_SEARCH
    else {
      req = mdns_lookup_request(&q.info);
    }
#endif
  }
  /* We need to check all resource record sections: answers, authoritative and additional */
  total_answers_left = pkt->answers_left + pkt->authoritative_left + pkt->additional_left;
  while (total_answers_left) {
    struct mdns_answer ans;
    err_t res;

    res = mdns_read_answer(pkt, &ans, &total_answers_left);
    if (res != ERR_OK) {
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Failed to parse answer, skipping response packet\n"));
      return;
    }

    LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Answer for domain "));
    mdns_domain_debug_print(&ans.info.domain);
    LWIP_DEBUGF(MDNS_DEBUG, (" type %d class %d\n", ans.info.type, ans.info.klass));

    if (ans.info.type == DNS_RRTYPE_ANY || ans.info.klass != DNS_RRCLASS_IN) {
      /* Skip answers for ANY type or if class != IN */
      continue;
    }

#if LWIP_MDNS_SEARCH
    if (req && req->only_ptr) {
      /* Need to recheck that this answer match request that match previous answer */
      if (memcmp (req->service.name, ans.info.domain.name, req->service.length) != 0)
        req = NULL;
    }
    if (!req) {
      /* Try hard to search matching request */
      req = mdns_lookup_request(&ans.info);
    }
    if (req && req->result_fn) {
      u16_t offset;
      struct pbuf *p;
      int flags = (first ? MDNS_SEARCH_RESULT_FIRST : 0) |
          (!total_answers_left ? MDNS_SEARCH_RESULT_LAST : 0);
      if (req->only_ptr) {
          if (ans.info.type != DNS_RRTYPE_PTR)
              continue; /* Ignore non matching answer type */
          flags = MDNS_SEARCH_RESULT_FIRST | MDNS_SEARCH_RESULT_LAST;
      }
      p = pbuf_skip(pkt->pbuf, ans.rd_offset, &offset);
      if (p == NULL) {
        LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Malformed response packet, aborting\n"));
        return;
      }
      if (ans.info.type == DNS_RRTYPE_PTR || ans.info.type == DNS_RRTYPE_SRV) {
        /* Those RR types have compressed domain name. Must uncompress here,
           since cannot be done without pbuf. */
        struct {
          u16_t values[3];        /* SRV: Prio, Weight, Port */
          struct mdns_domain dom; /* PTR & SRV: Domain (uncompressed) */
        } data;
        u16_t off = (ans.info.type == DNS_RRTYPE_SRV ? 6 : 0);
        u16_t len = mdns_readname(pkt->pbuf, ans.rd_offset + off, &data.dom);
        if (len == MDNS_READNAME_ERROR) {
          /* Ensure result_fn is called anyway, just copy failed domain as is */
          data.dom.length = ans.rd_length - off;
          memcpy(&data.dom, (const char *)p->payload + offset + off, data.dom.length);
        }
        /* Adjust len/off according RR type */
        if (ans.info.type == DNS_RRTYPE_SRV) {
          memcpy(&data, (const char *)p->payload + offset, 6);
          len = data.dom.length + 6;
          off = 0;
        } else {
          len = data.dom.length;
          off = 6;
        }
        req->result_fn(&ans, (const char *)&data + off, len, flags, req->arg);
      } else {
        /* Direct call result_fn with varpart pointing in pbuf payload */
        req->result_fn(&ans, (const char *)p->payload + offset, ans.rd_length, flags, req->arg);
      }
      first = 0;
    }
#endif

    /* "Conflicting Multicast DNS responses received *before* the first probe
     * packet is sent MUST be silently ignored" so drop answer if we haven't
     * started probing yet. */
    if ((mdns->state == MDNS_STATE_PROBING) ||
        (mdns->state == MDNS_STATE_ANNOUNCE_WAIT)) {
      struct mdns_domain domain;
      u8_t i;

      res = mdns_build_host_domain(&domain, mdns);
      if (res == ERR_OK && mdns_domain_eq(&ans.info.domain, &domain)) {
        LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Probe response matches host domain!\n"));
        mdns_probe_conflict(netif, 0);
        break;
      }

      for (i = 0; i < MDNS_MAX_SERVICES; i++) {
        struct mdns_service* service = mdns->services[i];
        if (!service) {
          continue;
        }
        res = mdns_build_service_domain(&domain, service, 1);
        if ((res == ERR_OK) && mdns_domain_eq(&ans.info.domain, &domain)) {
          LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Probe response matches service domain!\n"));
          mdns_probe_conflict(netif, i + 1);
          break;
        }
      }
      if (i < MDNS_MAX_SERVICES)
        break;
    }
    /* Perform conflict resolution (RFC6762 section 9):
     * We assume a conflict if the hostname or service name matches the answers
     * domain. Only if the rdata matches exactly we reset our assumption to no
     * conflict. As stated in the RFC:
     * What may be considered inconsistent is context sensitive, except that
     * resource records with identical rdata are never considered inconsistent,
     * even if they originate from different hosts.
     */
    else if ((mdns->state == MDNS_STATE_ANNOUNCING) ||
             (mdns->state == MDNS_STATE_COMPLETE)) {
      struct mdns_domain domain;
      u8_t i;
      u8_t conflict = 0;

      /* Evaluate unique hostname records -> A and AAAA */
      res = mdns_build_host_domain(&domain, mdns);
      if (res == ERR_OK && mdns_domain_eq(&ans.info.domain, &domain)) {
        LWIP_DEBUGF(MDNS_DEBUG, ("mDNS: response matches host domain, assuming conflict\n"));
        /* This means a conflict has taken place, except when the packet contains
         * exactly the same rdata. */
        conflict = 1;
        /* Evaluate rdata -> to see if it's a copy of our own data */
        if (ans.info.type == DNS_RRTYPE_A) {
#if LWIP_IPV4
          if (ans.rd_length == sizeof(ip4_addr_t) &&
              pbuf_memcmp(pkt->pbuf, ans.rd_offset, netif_ip4_addr(netif), ans.rd_length) == 0) {
            LWIP_DEBUGF(MDNS_DEBUG, ("mDNS: response equals our own IPv4 address record -> no conflict\n"));
            conflict = 0;
          }
#endif
        }
        else if (ans.info.type == DNS_RRTYPE_AAAA) {
#if LWIP_IPV6
          if (ans.rd_length == sizeof(ip6_addr_p_t)) {
            for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
              if (pbuf_memcmp(pkt->pbuf, ans.rd_offset, netif_ip6_addr(netif, i), ans.rd_length) == 0) {
                LWIP_DEBUGF(MDNS_DEBUG, ("mDNS: response equals our own iPv6 address record, num = %d -> no conflict\n",i));
                conflict = 0;
              }
            }
          }
#endif
        }
      }
      /* Evaluate unique service name records -> SRV and TXT */
      for (i = 0; i < MDNS_MAX_SERVICES; i++) {
        struct mdns_service* service = mdns->services[i];
        if (!service) {
          continue;
        }
        res = mdns_build_service_domain(&domain, service, 1);
        if ((res == ERR_OK) && mdns_domain_eq(&ans.info.domain, &domain)) {
          LWIP_DEBUGF(MDNS_DEBUG, ("mDNS: response matches service domain, assuming conflict\n"));
          /* This means a conflict has taken place, except when the packet contains
           * exactly the same rdata. */
          conflict = 1;
          /* Evaluate rdata -> to see if it's a copy of our own data */
          if (ans.info.type == DNS_RRTYPE_SRV) {
            /* Read and compare to with our SRV record */
            u16_t field16, len, read_pos;
            struct mdns_domain srv_ans, my_ans;
            read_pos = ans.rd_offset;
            do {
              /* Check priority field */
              len = pbuf_copy_partial(pkt->pbuf, &field16, sizeof(field16), read_pos);
              if (len != sizeof(field16) || lwip_ntohs(field16) != SRV_PRIORITY) {
                break;
              }
              read_pos += len;
              /* Check weight field */
              len = pbuf_copy_partial(pkt->pbuf, &field16, sizeof(field16), read_pos);
              if (len != sizeof(field16) || lwip_ntohs(field16) != SRV_WEIGHT) {
                break;
              }
              read_pos += len;
              /* Check port field */
              len = pbuf_copy_partial(pkt->pbuf, &field16, sizeof(field16), read_pos);
              if (len != sizeof(field16) || lwip_ntohs(field16) != service->port) {
                break;
              }
              read_pos += len;
              /* Check host field */
              len = mdns_readname(pkt->pbuf, read_pos, &srv_ans);
              mdns_build_host_domain(&my_ans, mdns);
              if (len == MDNS_READNAME_ERROR || !mdns_domain_eq(&srv_ans, &my_ans)) {
                break;
              }
              LWIP_DEBUGF(MDNS_DEBUG, ("mDNS: response equals our own SRV record -> no conflict\n"));
              conflict = 0;
            } while (0);
          } else if (ans.info.type == DNS_RRTYPE_TXT) {
            mdns_prepare_txtdata(service);
            if (service->txtdata.length == ans.rd_length &&
                pbuf_memcmp(pkt->pbuf, ans.rd_offset, service->txtdata.name, ans.rd_length) == 0) {
              LWIP_DEBUGF(MDNS_DEBUG, ("mDNS: response equals our own TXT record -> no conflict\n"));
              conflict = 0;
            }
          }
        }
      }
      if (conflict != 0) {
        /* Reset host to probing to reconfirm uniqueness */
        LWIP_DEBUGF(MDNS_DEBUG, ("mDNS: Conflict resolution -> reset to probing state\n"));
        mdns_resp_restart(netif);
        break;
      }
    }
  }
  /* Clear all xxx_left variables because we parsed all answers */
  pkt->answers_left = 0;
  pkt->authoritative_left = 0;
  pkt->additional_left = 0;
}

/**
 * Receive input function for MDNS packets.
 * Handles both IPv4 and IPv6 UDP pcbs.
 */
static void
mdns_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
  struct dns_hdr hdr;
  struct mdns_packet packet;
  struct netif *recv_netif = ip_current_input_netif();
  u16_t offset = 0;

  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(pcb);

  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Received IPv%d MDNS packet, len %d\n", IP_IS_V6(addr) ? 6 : 4, p->tot_len));

  if (NETIF_TO_HOST(recv_netif) == NULL) {
    /* From netif not configured for MDNS */
    goto dealloc;
  }

  if (pbuf_copy_partial(p, &hdr, SIZEOF_DNS_HDR, offset) < SIZEOF_DNS_HDR) {
    /* Too small */
    goto dealloc;
  }
  offset += SIZEOF_DNS_HDR;

  if (DNS_HDR_GET_OPCODE(&hdr)) {
    /* Ignore non-standard queries in multicast packets (RFC 6762, section 18.3) */
    goto dealloc;
  }

  memset(&packet, 0, sizeof(packet));
  SMEMCPY(&packet.source_addr, addr, sizeof(packet.source_addr));
  packet.source_port = port;
  packet.pbuf = p;
  packet.parse_offset = offset;
  packet.tx_id = lwip_ntohs(hdr.id);
  packet.questions = packet.questions_left = lwip_ntohs(hdr.numquestions);
  packet.answers = packet.answers_left = lwip_ntohs(hdr.numanswers);
  packet.authoritative = packet.authoritative_left = lwip_ntohs(hdr.numauthrr);
  packet.additional = packet.additional_left = lwip_ntohs(hdr.numextrarr);

  /*  Source address check (RFC6762 section 11) -> for responses.
   *  Source address check (RFC6762 section 5.5) -> for queries.
   *  When the dest addr == multicast addr we know the packet originated on that
   *  link. If not, we need to check the source address. We only accept queries
   *  that originated on the link. Others are discarded.
   */
#if LWIP_IPV6
  if (IP_IS_V6(ip_current_dest_addr())) {
    /* instead of having one 'v6group' per netif, just compare zoneless here */
    if (!ip_addr_zoneless_eq(ip_current_dest_addr(), &v6group)) {
      packet.recv_unicast = 1;

      if (ip6_addr_ismulticast_global(ip_2_ip6(ip_current_src_addr()))
          || ip6_addr_isglobal(ip_2_ip6(ip_current_src_addr()))) {
        goto dealloc;
      }
    }
  }
#endif
#if LWIP_IPV4
  if (!IP_IS_V6(ip_current_dest_addr())) {
    if (!ip_addr_eq(ip_current_dest_addr(), &v4group)) {
      packet.recv_unicast = 1;

      if (!ip4_addr_net_eq(ip_2_ip4(ip_current_src_addr()),
                          netif_ip4_addr(recv_netif),
                          netif_ip4_netmask(recv_netif))){
           goto dealloc;
         }
    }
  }
#endif

  if (hdr.flags1 & DNS_FLAG1_RESPONSE) {
    mdns_handle_response(&packet, recv_netif);
  } else {
    if (packet.questions && hdr.flags1 & DNS_FLAG1_TRUNC) {
      /* this is a new truncated question */
      struct mdns_packet *pkt = (struct mdns_packet *)LWIP_MEMPOOL_ALLOC(MDNS_PKTS);
      if (!pkt)
        goto dealloc; /* don't reply truncated question if alloc error */
      SMEMCPY(pkt, &packet, sizeof(packet));
      /* insert this question in pending list */
      pkt->next_tc_question = pending_tc_questions;
      pending_tc_questions = pkt;
      /* question with truncated flags, need to wait 400-500ms before replying */
      sys_timeout(MDNS_RESPONSE_TC_DELAY_MS, mdns_handle_tc_question, pkt);
      /* return without dealloc pbuf */
      return;
    }
    else if (!packet.questions && packet.answers && pending_tc_questions) {
      /* this packet is a known-answer packet for a truncated question previously received */
      struct mdns_packet *q = pending_tc_questions;
      while (q) {
        if ((packet.source_port == q->source_port) &&
            ip_addr_eq(&packet.source_addr, &q->source_addr))
          break;
        q = q->next_tc_question;
      }
      if (q) {
        /* found question from the same source */
        struct mdns_packet *pkt = (struct mdns_packet *)LWIP_MEMPOOL_ALLOC(MDNS_PKTS);
        if (!pkt)
          goto dealloc; /* don't reply truncated question if alloc error */
        SMEMCPY(pkt, &packet, sizeof(packet));
        /* insert this known-ansert in question */
        pkt->next_answer = q->next_answer;
        q->next_answer = pkt;
        /* nothing more to do */
        return;
      }
    }
    /* if previous tests fail, handle this question normally */
    mdns_handle_question(&packet, recv_netif);
  }

dealloc:
  pbuf_free(p);
}

#if LWIP_NETIF_EXT_STATUS_CALLBACK && MDNS_RESP_USENETIF_EXTCALLBACK
static void
mdns_netif_ext_status_callback(struct netif *netif, netif_nsc_reason_t reason, const netif_ext_callback_args_t *args)
{
  LWIP_UNUSED_ARG(args);

  /* MDNS enabled on netif? */
  if (NETIF_TO_HOST(netif) == NULL) {
    return;
  }

  if (reason & LWIP_NSC_STATUS_CHANGED) {
    if (args->status_changed.state != 0) {
      mdns_resp_restart(netif);
    }
    /* TODO: send goodbye message */
  }
  if (reason & LWIP_NSC_LINK_CHANGED) {
    if (args->link_changed.state != 0) {
      mdns_resp_restart(netif);
    }
  }
  if (reason & (LWIP_NSC_IPV4_ADDRESS_CHANGED | LWIP_NSC_IPV4_GATEWAY_CHANGED |
      LWIP_NSC_IPV4_NETMASK_CHANGED | LWIP_NSC_IPV4_SETTINGS_CHANGED |
      LWIP_NSC_IPV6_SET | LWIP_NSC_IPV6_ADDR_STATE_CHANGED)) {
    mdns_resp_restart(netif);
  }
}
#endif /* LWIP_NETIF_EXT_STATUS_CALLBACK && MDNS_RESP_USENETIF_EXTCALLBACK */

static void
mdns_define_probe_rrs_to_send(struct netif *netif, struct mdns_outmsg *outmsg)
{
  struct mdns_host *mdns = NETIF_TO_HOST(netif);
  int i;

  memset(outmsg, 0, sizeof(struct mdns_outmsg));

  /* Add unicast questions with rtype ANY for all our desired records */
  outmsg->host_questions = QUESTION_PROBE_HOST_ANY;

  for (i = 0; i < MDNS_MAX_SERVICES; i++) {
    struct mdns_service* service = mdns->services[i];
    if (!service) {
      continue;
    }
    outmsg->serv_questions[i] = QUESTION_PROBE_SERVICE_NAME_ANY;
  }

  /* Add answers to the questions above into the authority section for tiebreaking */
#if LWIP_IPV4
  if (!ip4_addr_isany_val(*netif_ip4_addr(netif))) {
    outmsg->host_replies = REPLY_HOST_A;
  }
#endif
#if LWIP_IPV6
  for (i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
    if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i))) {
      outmsg->host_replies |= REPLY_HOST_AAAA;
    }
  }
#endif

  for (i = 0; i < MDNS_MAX_SERVICES; i++) {
    struct mdns_service *serv = mdns->services[i];
    if (serv) {
      outmsg->serv_replies[i] = REPLY_SERVICE_SRV;
    }
  }
}

static err_t
mdns_send_probe(struct netif* netif, const ip_addr_t *destination)
{
  struct mdns_outmsg outmsg;

  mdns_define_probe_rrs_to_send(netif, &outmsg);

  outmsg.tx_id = 0;
  outmsg.dest_port = LWIP_IANA_PORT_MDNS;
  SMEMCPY(&outmsg.dest_addr, destination, sizeof(outmsg.dest_addr));
  return mdns_send_outpacket(&outmsg, netif);
}

/**
 * Timer callback for probing and announcing on the network.
 */
static void
mdns_probe_and_announce(void* arg)
{
  struct netif *netif = (struct netif *)arg;
  struct mdns_host* mdns = NETIF_TO_HOST(netif);
  u32_t announce_delay;


  switch (mdns->state) {
    case MDNS_STATE_OFF:
    case MDNS_STATE_PROBE_WAIT:
    case MDNS_STATE_PROBING:
#if LWIP_IPV4
      /*if ipv4 wait with probing until address is set*/
      if (!ip4_addr_isany_val(*netif_ip4_addr(netif)) &&
          mdns_send_probe(netif, &v4group) == ERR_OK)
#endif
      {
#if LWIP_IPV6
        if (mdns_send_probe(netif, &v6group) == ERR_OK)
#endif
        {
          mdns->state = MDNS_STATE_PROBING;
          mdns->sent_num++;
        }
      }

      if (mdns->sent_num >= MDNS_PROBE_COUNT) {
        mdns->state = MDNS_STATE_ANNOUNCE_WAIT;
        mdns->sent_num = 0;
      }

      if (mdns->sent_num && mdns->rate_limit_activated == 1) {
        /* delay second probe if rate limiting activated */
        sys_timeout(MDNS_PROBE_MAX_CONFLICTS_TIMEOUT, mdns_probe_and_announce, netif);
      }
      else {
        sys_timeout(MDNS_PROBE_DELAY_MS, mdns_probe_and_announce, netif);
      }
      break;
    case MDNS_STATE_ANNOUNCE_WAIT:
    case MDNS_STATE_ANNOUNCING:
      if (mdns->sent_num == 0) {
        /* probing was successful, announce all records */
        mdns->state = MDNS_STATE_ANNOUNCING;
        /* Reset rate limit max probe conflict timeout flag */
        mdns->rate_limit_activated = 0;
        /* Let the client know probing was successful */
        if (mdns_name_result_cb != NULL) {
          mdns_name_result_cb(netif, MDNS_PROBING_SUCCESSFUL, 0);
        }
      }

      mdns_resp_announce(netif);
      mdns->sent_num++;

      if (mdns->sent_num >= MDNS_ANNOUNCE_COUNT) {
        /* Announcing and probing complete */
        mdns->state = MDNS_STATE_COMPLETE;
        mdns->sent_num = 0;
      }
      else {
        announce_delay = MDNS_ANNOUNCE_DELAY_MS * (1 << (mdns->sent_num - 1));
        sys_timeout(announce_delay, mdns_probe_and_announce, netif);
      }
      break;
    case MDNS_STATE_COMPLETE:
    default:
      /* Do nothing */
      break;
  }
}

/**
 * @ingroup mdns
 * Activate MDNS responder for a network interface.
 * @param netif The network interface to activate.
 * @param hostname Name to use. Queries for &lt;hostname&gt;.local will be answered
 *                 with the IP addresses of the netif. The hostname will be copied, the
 *                 given pointer can be on the stack.
 * @return ERR_OK if netif was added, an err_t otherwise
 */
err_t
mdns_resp_add_netif(struct netif *netif, const char *hostname)
{
  err_t res;
  struct mdns_host *mdns;

  LWIP_ASSERT_CORE_LOCKED();
  LWIP_ERROR("mdns_resp_add_netif: netif != NULL", (netif != NULL), return ERR_VAL);
  LWIP_ERROR("mdns_resp_add_netif: Hostname too long", (strlen(hostname) <= MDNS_LABEL_MAXLEN), return ERR_VAL);

  LWIP_ASSERT("mdns_resp_add_netif: Double add", NETIF_TO_HOST(netif) == NULL);
  mdns = (struct mdns_host *) mem_calloc(1, sizeof(struct mdns_host));
  LWIP_ERROR("mdns_resp_add_netif: Alloc failed", (mdns != NULL), return ERR_MEM);

  netif_set_client_data(netif, mdns_netif_client_id, mdns);

  MEMCPY(&mdns->name, hostname, LWIP_MIN(MDNS_LABEL_MAXLEN, strlen(hostname)));

  /* Init delayed message structs with address and port */
#if LWIP_IPV4
  mdns->ipv4.delayed_msg_multicast.dest_port = LWIP_IANA_PORT_MDNS;
  SMEMCPY(&mdns->ipv4.delayed_msg_multicast.dest_addr, &v4group,
            sizeof(ip_addr_t));
#endif

#if LWIP_IPV6
  mdns->ipv6.delayed_msg_multicast.dest_port = LWIP_IANA_PORT_MDNS;
  SMEMCPY(&mdns->ipv6.delayed_msg_multicast.dest_addr, &v6group,
            sizeof(ip_addr_t));
#endif

  /* Join multicast groups */
#if LWIP_IPV4
  res = igmp_joingroup_netif(netif, ip_2_ip4(&v4group));
  if (res != ERR_OK) {
    goto cleanup;
  }
#endif
#if LWIP_IPV6
  res = mld6_joingroup_netif(netif, ip_2_ip6(&v6group));
  if (res != ERR_OK) {
    goto cleanup;
  }
#endif

  mdns_resp_restart(netif);

  return ERR_OK;

cleanup:
  mem_free(mdns);
  netif_set_client_data(netif, mdns_netif_client_id, NULL);
  return res;
}

/**
 * @ingroup mdns
 * Stop responding to MDNS queries on this interface, leave multicast groups,
 * and free the helper structure and any of its services.
 * @param netif The network interface to remove.
 * @return ERR_OK if netif was removed, an err_t otherwise
 */
err_t
mdns_resp_remove_netif(struct netif *netif)
{
  int i;
  struct mdns_host *mdns;

  LWIP_ASSERT_CORE_LOCKED();
  LWIP_ASSERT("mdns_resp_remove_netif: Null pointer", netif);
  mdns = NETIF_TO_HOST(netif);
  LWIP_ERROR("mdns_resp_remove_netif: Not an active netif", (mdns != NULL), return ERR_VAL);

  sys_untimeout(mdns_probe_and_announce, netif);

  for (i = 0; i < MDNS_MAX_SERVICES; i++) {
    struct mdns_service *service = mdns->services[i];
    if (service) {
      mem_free(service);
    }
  }

  /* Leave multicast groups */
#if LWIP_IPV4
  igmp_leavegroup_netif(netif, ip_2_ip4(&v4group));
#endif
#if LWIP_IPV6
  mld6_leavegroup_netif(netif, ip_2_ip6(&v6group));
#endif

  mem_free(mdns);
  netif_set_client_data(netif, mdns_netif_client_id, NULL);
  return ERR_OK;
}

/**
 * @ingroup mdns
 * Update MDNS hostname for a network interface.
 * @param netif The network interface to activate.
 * @param hostname Name to use. Queries for &lt;hostname&gt;.local will be answered
 *                 with the IP addresses of the netif. The hostname will be copied, the
 *                 given pointer can be on the stack.
 * @return ERR_OK if name could be set on netif, an err_t otherwise
 */
err_t
mdns_resp_rename_netif(struct netif *netif, const char *hostname)
{
  struct mdns_host *mdns;
  size_t len;

  LWIP_ASSERT_CORE_LOCKED();
  len = strlen(hostname);
  LWIP_ERROR("mdns_resp_rename_netif: netif != NULL", (netif != NULL), return ERR_VAL);
  LWIP_ERROR("mdns_resp_rename_netif: Hostname too long", (len <= MDNS_LABEL_MAXLEN), return ERR_VAL);
  mdns = NETIF_TO_HOST(netif);
  LWIP_ERROR("mdns_resp_rename_netif: Not an mdns netif", (mdns != NULL), return ERR_VAL);

  MEMCPY(&mdns->name, hostname, LWIP_MIN(MDNS_LABEL_MAXLEN, len));
  mdns->name[len] = '\0'; /* null termination in case new name is shorter than previous */

  mdns_resp_restart_delay(netif, MDNS_PROBE_DELAY_MS);

  return ERR_OK;
}

/**
 * @ingroup mdns
 * Checks if an MDNS responder is active for a given network interface.
 * @param netif The network interface to test.
 * @return nonzero if responder active, zero otherwise.
 */
int
mdns_resp_netif_active(struct netif *netif)
{
	return NETIF_TO_HOST(netif) != NULL;
}

/**
 * @ingroup mdns
 * Add a service to the selected network interface.
 * @param netif The network interface to publish this service on
 * @param name The name of the service
 * @param service The service type, like "_http"
 * @param proto The service protocol, DNSSD_PROTO_TCP for TCP ("_tcp") and DNSSD_PROTO_UDP
 *              for others ("_udp")
 * @param port The port the service listens to
 * @param txt_fn Callback to get TXT data. Will be called each time a TXT reply is created to
 *               allow dynamic replies.
 * @param txt_data Userdata pointer for txt_fn
 * @return service_id if the service was added to the netif, an err_t otherwise
 */
s8_t
mdns_resp_add_service(struct netif *netif, const char *name, const char *service, enum mdns_sd_proto proto, u16_t port, service_get_txt_fn_t txt_fn, void *txt_data)
{
  u8_t slot;
  struct mdns_service *srv;
  struct mdns_host *mdns;

  LWIP_ASSERT_CORE_LOCKED();
  LWIP_ASSERT("mdns_resp_add_service: netif != NULL", netif);
  mdns = NETIF_TO_HOST(netif);
  LWIP_ERROR("mdns_resp_add_service: Not an mdns netif", (mdns != NULL), return ERR_VAL);

  LWIP_ERROR("mdns_resp_add_service: Name too long", (strlen(name) <= MDNS_LABEL_MAXLEN), return ERR_VAL);
  LWIP_ERROR("mdns_resp_add_service: Service too long", (strlen(service) <= MDNS_LABEL_MAXLEN), return ERR_VAL);
  LWIP_ERROR("mdns_resp_add_service: Bad proto (need TCP or UDP)", (proto == DNSSD_PROTO_TCP || proto == DNSSD_PROTO_UDP), return ERR_VAL);

  for (slot = 0; slot < MDNS_MAX_SERVICES; slot++) {
    if (mdns->services[slot] == NULL) {
      break;
    }
  }
  LWIP_ERROR("mdns_resp_add_service: Service list full (increase MDNS_MAX_SERVICES)", (slot < MDNS_MAX_SERVICES), return ERR_MEM);

  srv = (struct mdns_service *)mem_calloc(1, sizeof(struct mdns_service));
  LWIP_ERROR("mdns_resp_add_service: Alloc failed", (srv != NULL), return ERR_MEM);

  MEMCPY(&srv->name, name, LWIP_MIN(MDNS_LABEL_MAXLEN, strlen(name)));
  MEMCPY(&srv->service, service, LWIP_MIN(MDNS_LABEL_MAXLEN, strlen(service)));
  srv->txt_fn = txt_fn;
  srv->txt_userdata = txt_data;
  srv->proto = (u16_t)proto;
  srv->port = port;

  mdns->services[slot] = srv;

  mdns_resp_restart(netif);

  return slot;
}

/**
 * @ingroup mdns
 * Delete a service on the selected network interface.
 * @param netif The network interface on which service should be removed
 * @param slot The service slot number returned by mdns_resp_add_service
 * @return ERR_OK if the service was removed from the netif, an err_t otherwise
 */
err_t
mdns_resp_del_service(struct netif *netif, u8_t slot)
{
  struct mdns_host *mdns;
  struct mdns_service *srv;
  LWIP_ASSERT("mdns_resp_del_service: netif != NULL", netif);
  mdns = NETIF_TO_HOST(netif);
  LWIP_ERROR("mdns_resp_del_service: Not an mdns netif", (mdns != NULL), return ERR_VAL);
  LWIP_ERROR("mdns_resp_del_service: Invalid Service ID", slot < MDNS_MAX_SERVICES, return ERR_VAL);
  LWIP_ERROR("mdns_resp_del_service: Invalid Service ID", (mdns->services[slot] != NULL), return ERR_VAL);

  srv = mdns->services[slot];
  mdns->services[slot] = NULL;
  mem_free(srv);
  return ERR_OK;
}

/**
 * @ingroup mdns
 * Update name for an MDNS service.
 * @param netif The network interface to activate.
 * @param slot The service slot number returned by mdns_resp_add_service
 * @param name The new name for the service
 * @return ERR_OK if name could be set on service, an err_t otherwise
 */
err_t
mdns_resp_rename_service(struct netif *netif, u8_t slot, const char *name)
{
  struct mdns_service *srv;
  struct mdns_host *mdns;
  size_t len;

  LWIP_ASSERT_CORE_LOCKED();
  len = strlen(name);
  LWIP_ASSERT("mdns_resp_rename_service: netif != NULL", netif);
  mdns = NETIF_TO_HOST(netif);
  LWIP_ERROR("mdns_resp_rename_service: Not an mdns netif", (mdns != NULL), return ERR_VAL);
  LWIP_ERROR("mdns_resp_rename_service: Name too long", (len <= MDNS_LABEL_MAXLEN), return ERR_VAL);
  LWIP_ERROR("mdns_resp_rename_service: Invalid Service ID", slot < MDNS_MAX_SERVICES, return ERR_VAL);
  LWIP_ERROR("mdns_resp_rename_service: Invalid Service ID", (mdns->services[slot] != NULL), return ERR_VAL);

  srv = mdns->services[slot];

  MEMCPY(&srv->name, name, LWIP_MIN(MDNS_LABEL_MAXLEN, len));
  srv->name[len] = '\0'; /* null termination in case new name is shorter than previous */

  mdns_resp_restart_delay(netif, MDNS_PROBE_DELAY_MS);

  return ERR_OK;
}

/**
 * @ingroup mdns
 * Call this function from inside the service_get_txt_fn_t callback to add text data.
 * Buffer for TXT data is 256 bytes, and each field is prefixed with a length byte.
 * @param service The service provided to the get_txt callback
 * @param txt String to add to the TXT field.
 * @param txt_len Length of string
 * @return ERR_OK if the string was added to the reply, an err_t otherwise
 */
err_t
mdns_resp_add_service_txtitem(struct mdns_service *service, const char *txt, u8_t txt_len)
{
  LWIP_ASSERT_CORE_LOCKED();
  LWIP_ASSERT("mdns_resp_add_service_txtitem: service != NULL", service);

  /* Use a mdns_domain struct to store txt chunks since it is the same encoding */
  return mdns_domain_add_label(&service->txtdata, txt, txt_len);
}

#if LWIP_MDNS_SEARCH
/**
 * @ingroup mdns
 * Stop a search request.
 * @param request_id The search request to stop
 */
void
mdns_search_stop(u8_t request_id)
{
  struct mdns_request *req;
  LWIP_ASSERT("mdns_search_stop: bad request_id", request_id < MDNS_MAX_REQUESTS);
  req = &mdns_requests[request_id];
  if (req && req->result_fn) {
    req->result_fn = NULL;
  }
}

/**
 * @ingroup mdns
 * Search a specific service on the network.
 * @param name The name of the service
 * @param service The service type, like "_http"
 * @param proto The service protocol, DNSSD_PROTO_TCP for TCP ("_tcp") and DNSSD_PROTO_UDP
 *              for others ("_udp")
 * @param netif The network interface where to send search request
 * @param result_fn Callback to send answer received. Will be called for each answer of a
 *                  response frame matching request sent.
 * @param arg Userdata pointer for result_fn
 * @param request_id Returned request identifier to allow stop it.
 * @return ERR_OK if the search request was created and sent, an err_t otherwise
 */
err_t
mdns_search_service(const char *name, const char *service, enum mdns_sd_proto proto,
                    struct netif *netif, search_result_fn_t result_fn, void *arg,
                    u8_t *request_id)
{
  u8_t slot;
  struct mdns_request *req;
  if (name) {
    LWIP_ERROR("mdns_search_service: Name too long", (strlen(name) <= MDNS_LABEL_MAXLEN), return ERR_VAL);
  }
  LWIP_ERROR("mdns_search_service: Service too long", (strlen(service) < MDNS_DOMAIN_MAXLEN), return ERR_VAL);
  LWIP_ERROR("mdns_search_service: Bad reqid pointer", request_id, return ERR_VAL);
  LWIP_ERROR("mdns_search_service: Bad proto (need TCP or UDP)", (proto == DNSSD_PROTO_TCP || proto == DNSSD_PROTO_UDP), return ERR_VAL);
  for (slot = 0; slot < MDNS_MAX_REQUESTS; slot++) {
    if (mdns_requests[slot].result_fn == NULL) {
      break;
    }
  }
  if (slot >= MDNS_MAX_REQUESTS) {
    /* Don't assert if no more space in mdns_request table. Just return an error. */
    return ERR_MEM;
  }

  req = &mdns_requests[slot];
  memset(req, 0, sizeof(struct mdns_request));
  req->result_fn = result_fn;
  req->arg = arg;
  req->proto = (u16_t)proto;
  req->qtype = DNS_RRTYPE_PTR;
  if (proto == DNSSD_PROTO_UDP && strcmp(service, "_services._dns-sd") == 0) {
      req->only_ptr = 1; /* don't check other answers */
  }
  mdns_domain_add_string(&req->service, service);
  if (name) {
    MEMCPY(&req->name, name, LWIP_MIN(MDNS_LABEL_MAXLEN, strlen(name)));
  }
  /* save request id (slot) in pointer provided by caller */
  *request_id = slot;
  /* now prepare a MDNS request and send it (on specified interface) */
#if LWIP_IPV6
  mdns_send_request(req, netif, &v6group);
#endif
#if LWIP_IPV4
  mdns_send_request(req, netif, &v4group);
#endif
  return ERR_OK;
}
#endif

/**
 * @ingroup mdns
 * Send unsolicited answer containing all our known data
 * @param netif The network interface to send on
 */
void
mdns_resp_announce(struct netif *netif)
{
  struct mdns_host* mdns;
  LWIP_ASSERT_CORE_LOCKED();
  LWIP_ERROR("mdns_resp_announce: netif != NULL", (netif != NULL), return);

  mdns = NETIF_TO_HOST(netif);
  if (mdns == NULL) {
    return;
  }

  /* Do not announce if the mdns responder is off, waiting to probe, probing or
   * waiting to announce. */
  if (mdns->state >= MDNS_STATE_ANNOUNCING) {
    /* Announce on IPv6 and IPv4 */
#if LWIP_IPV6
    mdns_announce(netif, &v6group);
    mdns_start_multicast_timeouts_ipv6(netif);
#endif
#if LWIP_IPV4
    if (!ip4_addr_isany_val(*netif_ip4_addr(netif))) {
      mdns_announce(netif, &v4group);
      mdns_start_multicast_timeouts_ipv4(netif);
    }
#endif
  } /* else: ip address changed while probing was ongoing? @todo reset counter to restart? */
}

/** Register a callback function that is called if probing is completed successfully
 * or with a conflict. */
void
mdns_resp_register_name_result_cb(mdns_name_result_cb_t cb)
{
  mdns_name_result_cb = cb;
}

/**
 * @ingroup mdns
 * Restart mdns responder after a specified delay. Call this when cable is connected
 * after being disconnected or administrative interface is set up after being down
 * @param netif The network interface to send on
 * @param delay The delay to use before sending probe
 */
void
mdns_resp_restart_delay(struct netif *netif, uint32_t delay)
{
  struct mdns_host* mdns;
  LWIP_ASSERT_CORE_LOCKED();
  LWIP_ERROR("mdns_resp_restart: netif != NULL", (netif != NULL), return);

  mdns = NETIF_TO_HOST(netif);
  if (mdns == NULL) {
    return;
  }
  /* Make sure timer is not running */
  sys_untimeout(mdns_probe_and_announce, netif);

  mdns->sent_num = 0;
  mdns->state = MDNS_STATE_PROBE_WAIT;

  /* RFC6762 section 8.1: If fifteen conflicts occur within any ten-second period,
   * then the host MUST wait at least five seconds before each successive
   * additional probe attempt.
   */
  if (mdns->rate_limit_activated == 1) {
    sys_timeout(MDNS_PROBE_MAX_CONFLICTS_TIMEOUT, mdns_probe_and_announce, netif);
  }
  else {
    /* Adjust probe delay according sent probe count. */
    sys_timeout(delay, mdns_probe_and_announce, netif);
  }
}

/**
 * @ingroup mdns
 * Restart mdns responder. Call this when cable is connected after being disconnected or
 * administrative interface is set up after being down
 * @param netif The network interface to send on
 */
void
mdns_resp_restart(struct netif *netif)
{
  mdns_resp_restart_delay(netif, MDNS_INITIAL_PROBE_DELAY_MS);
}

/**
 * @ingroup mdns
 * Initiate MDNS responder. Will open UDP sockets on port 5353
 */
void
mdns_resp_init(void)
{
  err_t res;

  /* LWIP_ASSERT_CORE_LOCKED(); is checked by udp_new() */
#if LWIP_MDNS_SEARCH
  memset(mdns_requests, 0, sizeof(mdns_requests));
#endif
  LWIP_MEMPOOL_INIT(MDNS_PKTS);
  mdns_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
  LWIP_ASSERT("Failed to allocate pcb", mdns_pcb != NULL);
#if LWIP_MULTICAST_TX_OPTIONS
  udp_set_multicast_ttl(mdns_pcb, MDNS_IP_TTL);
#else
  mdns_pcb->ttl = MDNS_IP_TTL;
#endif
  res = udp_bind(mdns_pcb, IP_ANY_TYPE, LWIP_IANA_PORT_MDNS);
  LWIP_UNUSED_ARG(res); /* in case of LWIP_NOASSERT */
  LWIP_ASSERT("Failed to bind pcb", res == ERR_OK);
  udp_recv(mdns_pcb, mdns_recv, NULL);

  mdns_netif_client_id = netif_alloc_client_data_id();

#if MDNS_RESP_USENETIF_EXTCALLBACK
  /* register for netif events when started on first netif */
  netif_add_ext_callback(&netif_callback, mdns_netif_ext_status_callback);
#endif
}

/**
 * @ingroup mdns
 * Return TXT userdata of a specific service on a network interface.
 * @param netif Network interface.
 * @param slot Service index.
 */
void *mdns_get_service_txt_userdata(struct netif *netif, s8_t slot)
{
  struct mdns_host *mdns = NETIF_TO_HOST(netif);
  struct mdns_service *s;
  LWIP_ASSERT("mdns_get_service_txt_userdata: index out of range", slot < MDNS_MAX_SERVICES);
  s = mdns->services[slot];
  return s ? s->txt_userdata : NULL;
}

#endif /* LWIP_MDNS_RESPONDER */
