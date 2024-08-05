/**
 * @file
 * MDNS responder implementation - output related functionalities
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

#include "lwip/apps/mdns_out.h"
#include "lwip/apps/mdns_priv.h"
#include "lwip/apps/mdns_domain.h"
#include "lwip/prot/dns.h"
#include "lwip/prot/iana.h"
#include "lwip/udp.h"

#include <string.h>

#if LWIP_IPV6
#include "lwip/prot/ip6.h"
#endif


#if LWIP_MDNS_RESPONDER

/* Function prototypes */
static void mdns_clear_outmsg(struct mdns_outmsg *outmsg);

/**
 * Call user supplied function to setup TXT data
 * @param service The service to build TXT record for
 */
void
mdns_prepare_txtdata(struct mdns_service *service)
{
  memset(&service->txtdata, 0, sizeof(struct mdns_domain));
  if (service->txt_fn) {
    service->txt_fn(service, service->txt_userdata);
  }
}

/**
 * Write a question to an outpacket
 * A question contains domain, type and class. Since an answer also starts with these fields this function is also
 * called from mdns_add_answer().
 * @param outpkt The outpacket to write to
 * @param domain The domain name the answer is for
 * @param type The DNS type of the answer (like 'AAAA', 'SRV')
 * @param klass The DNS type of the answer (like 'IN')
 * @param unicast If highest bit in class should be set, to instruct the responder to
 *                reply with a unicast packet
 * @return ERR_OK on success, an err_t otherwise
 */
static err_t
mdns_add_question(struct mdns_outpacket *outpkt, struct mdns_domain *domain,
                  u16_t type, u16_t klass, u16_t unicast)
{
  u16_t question_len;
  u16_t field16;
  err_t res;

  if (!outpkt->pbuf) {
    /* If no pbuf is active, allocate one */
    outpkt->pbuf = pbuf_alloc(PBUF_TRANSPORT, MDNS_OUTPUT_PACKET_SIZE, PBUF_RAM);
    if (!outpkt->pbuf) {
      return ERR_MEM;
    }
    outpkt->write_offset = SIZEOF_DNS_HDR;
  }

  /* Worst case calculation. Domain string might be compressed */
  question_len = domain->length + sizeof(type) + sizeof(klass);
  if (outpkt->write_offset + question_len > outpkt->pbuf->tot_len) {
    /* No space */
    return ERR_MEM;
  }

  /* Write name */
  res = mdns_write_domain(outpkt, domain);
  if (res != ERR_OK) {
    return res;
  }

  /* Write type */
  field16 = lwip_htons(type);
  res = pbuf_take_at(outpkt->pbuf, &field16, sizeof(field16), outpkt->write_offset);
  if (res != ERR_OK) {
    return res;
  }
  outpkt->write_offset += sizeof(field16);

  /* Write class */
  if (unicast) {
    klass |= 0x8000;
  }
  field16 = lwip_htons(klass);
  res = pbuf_take_at(outpkt->pbuf, &field16, sizeof(field16), outpkt->write_offset);
  if (res != ERR_OK) {
    return res;
  }
  outpkt->write_offset += sizeof(field16);

  return ERR_OK;
}

/**
 * Write answer to reply packet.
 * buf or answer_domain can be null. The rd_length written will be buf_length +
 * size of (compressed) domain. Most uses will need either buf or answer_domain,
 * special case is SRV that starts with 3 u16 and then a domain name.
 * @param reply The outpacket to write to
 * @param domain The domain name the answer is for
 * @param type The DNS type of the answer (like 'AAAA', 'SRV')
 * @param klass The DNS type of the answer (like 'IN')
 * @param cache_flush If highest bit in class should be set, to instruct receiver that
 *                    this reply replaces any earlier answer for this domain/type/class
 * @param ttl Validity time in seconds to send out for IP address data in DNS replies
 * @param buf Pointer to buffer of answer data
 * @param buf_length Length of variable data
 * @param answer_domain A domain to write after any buffer data as answer
 * @return ERR_OK on success, an err_t otherwise
 */
static err_t
mdns_add_answer(struct mdns_outpacket *reply, struct mdns_domain *domain,
                u16_t type, u16_t klass, u16_t cache_flush, u32_t ttl,
                const u8_t *buf, size_t buf_length, struct mdns_domain *answer_domain)
{
  u16_t answer_len;
  u16_t field16;
  u16_t rdlen_offset;
  u16_t answer_offset;
  u32_t field32;
  err_t res;

  if (!reply->pbuf) {
    /* If no pbuf is active, allocate one */
    reply->pbuf = pbuf_alloc(PBUF_TRANSPORT, MDNS_OUTPUT_PACKET_SIZE, PBUF_RAM);
    if (!reply->pbuf) {
      return ERR_MEM;
    }
    reply->write_offset = SIZEOF_DNS_HDR;
  }

  /* Worst case calculation. Domain strings might be compressed */
  answer_len = domain->length + sizeof(type) + sizeof(klass) + sizeof(ttl) + sizeof(field16)/*rd_length*/;
  if (buf) {
    answer_len += (u16_t)buf_length;
  }
  if (answer_domain) {
    answer_len += answer_domain->length;
  }
  if (reply->write_offset + answer_len > reply->pbuf->tot_len) {
    /* No space */
    return ERR_MEM;
  }

  /* Answer starts with same data as question, then more fields */
  mdns_add_question(reply, domain, type, klass, cache_flush);

  /* Write TTL */
  field32 = lwip_htonl(ttl);
  res = pbuf_take_at(reply->pbuf, &field32, sizeof(field32), reply->write_offset);
  if (res != ERR_OK) {
    return res;
  }
  reply->write_offset += sizeof(field32);

  /* Store offsets and skip forward to the data */
  rdlen_offset = reply->write_offset;
  reply->write_offset += sizeof(field16);
  answer_offset = reply->write_offset;

  if (buf) {
    /* Write static data */
    res = pbuf_take_at(reply->pbuf, buf, (u16_t)buf_length, reply->write_offset);
    if (res != ERR_OK) {
      return res;
    }
    reply->write_offset += (u16_t)buf_length;
  }

  if (answer_domain) {
    /* Write name answer (compressed if possible) */
    res = mdns_write_domain(reply, answer_domain);
    if (res != ERR_OK) {
      return res;
    }
  }

  /* Write rd_length after when we know the answer size */
  field16 = lwip_htons(reply->write_offset - answer_offset);
  res = pbuf_take_at(reply->pbuf, &field16, sizeof(field16), rdlen_offset);

  return res;
}

/** Write an ANY host question to outpacket */
static err_t
mdns_add_any_host_question(struct mdns_outpacket *outpkt,
                           struct mdns_host *mdns,
                           u16_t request_unicast_reply)
{
  struct mdns_domain host;
  mdns_build_host_domain(&host, mdns);
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Adding host question for ANY type\n"));
  return mdns_add_question(outpkt, &host, DNS_RRTYPE_ANY, DNS_RRCLASS_IN,
                           request_unicast_reply);
}

/** Write an ANY service instance question to outpacket */
static err_t
mdns_add_any_service_question(struct mdns_outpacket *outpkt,
                              struct mdns_service *service,
                              u16_t request_unicast_reply)
{
  struct mdns_domain domain;
  mdns_build_service_domain(&domain, service, 1);
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Adding service instance question for ANY type\n"));
  return mdns_add_question(outpkt, &domain, DNS_RRTYPE_ANY, DNS_RRCLASS_IN,
                           request_unicast_reply);
}

#if LWIP_IPV4
/** Write an IPv4 address (A) RR to outpacket */
static err_t
mdns_add_a_answer(struct mdns_outpacket *reply, struct mdns_outmsg *msg,
                  struct netif *netif)
{
  err_t res;
  u32_t ttl = MDNS_TTL_120;
  struct mdns_domain host;
  mdns_build_host_domain(&host, netif_mdns_data(netif));
  /* When answering to a legacy querier, we need to repeat the question and
   * limit the ttl to the short legacy ttl */
  if(msg->legacy_query) {
    /* Repeating the question only needs to be done for the question asked
     * (max one question), not for the additional records. */
    if(reply->questions < 1) {
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Add question for legacy query\n"));
      res = mdns_add_question(reply, &host, DNS_RRTYPE_A, DNS_RRCLASS_IN, 0);
      if (res != ERR_OK) {
        return res;
      }
      reply->questions = 1;
    }
    /* ttl of legacy answer may not be greater then 10 seconds */
    ttl = MDNS_TTL_10;
  }
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Responding with A record\n"));
  return mdns_add_answer(reply, &host, DNS_RRTYPE_A, DNS_RRCLASS_IN, msg->cache_flush,
                         ttl, (const u8_t *) netif_ip4_addr(netif),
                         sizeof(ip4_addr_t), NULL);
}

/** Write a 4.3.2.1.in-addr.arpa -> hostname.local PTR RR to outpacket */
static err_t
mdns_add_hostv4_ptr_answer(struct mdns_outpacket *reply, struct mdns_outmsg *msg,
                           struct netif *netif)
{
  err_t res;
  u32_t ttl = MDNS_TTL_120;
  struct mdns_domain host, revhost;
  mdns_build_host_domain(&host, netif_mdns_data(netif));
  mdns_build_reverse_v4_domain(&revhost, netif_ip4_addr(netif));
  /* When answering to a legacy querier, we need to repeat the question and
   * limit the ttl to the short legacy ttl */
  if(msg->legacy_query) {
    /* Repeating the question only needs to be done for the question asked
     * (max one question), not for the additional records. */
    if(reply->questions < 1) {
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Add question for legacy query\n"));
      res = mdns_add_question(reply, &revhost, DNS_RRTYPE_PTR, DNS_RRCLASS_IN, 0);
      if (res != ERR_OK) {
        return res;
      }
      reply->questions = 1;
    }
    /* ttl of legacy answer may not be greater then 10 seconds */
    ttl = MDNS_TTL_10;
  }
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Responding with v4 PTR record\n"));
  return mdns_add_answer(reply, &revhost, DNS_RRTYPE_PTR, DNS_RRCLASS_IN,
                         msg->cache_flush, ttl, NULL, 0, &host);
}
#endif

#if LWIP_IPV6
/** Write an IPv6 address (AAAA) RR to outpacket */
static err_t
mdns_add_aaaa_answer(struct mdns_outpacket *reply, struct mdns_outmsg *msg,
                     struct netif *netif, int addrindex)
{
  err_t res;
  u32_t ttl = MDNS_TTL_120;
  struct mdns_domain host;
  mdns_build_host_domain(&host, netif_mdns_data(netif));
  /* When answering to a legacy querier, we need to repeat the question and
   * limit the ttl to the short legacy ttl */
  if(msg->legacy_query) {
    /* Repeating the question only needs to be done for the question asked
     * (max one question), not for the additional records. */
    if(reply->questions < 1) {
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Add question for legacy query\n"));
      res = mdns_add_question(reply, &host, DNS_RRTYPE_AAAA, DNS_RRCLASS_IN, 0);
      if (res != ERR_OK) {
        return res;
      }
      reply->questions = 1;
    }
    /* ttl of legacy answer may not be greater then 10 seconds */
    ttl = MDNS_TTL_10;
  }
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Responding with AAAA record\n"));
  return mdns_add_answer(reply, &host, DNS_RRTYPE_AAAA, DNS_RRCLASS_IN, msg->cache_flush,
                         ttl, (const u8_t *) netif_ip6_addr(netif, addrindex),
                         sizeof(ip6_addr_p_t), NULL);
}

/** Write a x.y.z.ip6.arpa -> hostname.local PTR RR to outpacket */
static err_t
mdns_add_hostv6_ptr_answer(struct mdns_outpacket *reply, struct mdns_outmsg *msg,
                           struct netif *netif, int addrindex)
{
  err_t res;
  u32_t ttl = MDNS_TTL_120;
  struct mdns_domain host, revhost;
  mdns_build_host_domain(&host, netif_mdns_data(netif));
  mdns_build_reverse_v6_domain(&revhost, netif_ip6_addr(netif, addrindex));
  /* When answering to a legacy querier, we need to repeat the question and
   * limit the ttl to the short legacy ttl */
  if(msg->legacy_query) {
    /* Repeating the question only needs to be done for the question asked
     * (max one question), not for the additional records. */
    if(reply->questions < 1) {
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Add question for legacy query\n"));
      res = mdns_add_question(reply, &revhost, DNS_RRTYPE_PTR, DNS_RRCLASS_IN, 0);
      if (res != ERR_OK) {
        return res;
      }
      reply->questions = 1;
    }
    /* ttl of legacy answer may not be greater then 10 seconds */
    ttl = MDNS_TTL_10;
  }
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Responding with v6 PTR record\n"));
  return mdns_add_answer(reply, &revhost, DNS_RRTYPE_PTR, DNS_RRCLASS_IN,
                         msg->cache_flush, ttl, NULL, 0, &host);
}
#endif

/** Write an all-services -> servicetype PTR RR to outpacket */
static err_t
mdns_add_servicetype_ptr_answer(struct mdns_outpacket *reply, struct mdns_outmsg *msg,
                                struct mdns_service *service)
{
  err_t res;
  u32_t ttl = MDNS_TTL_4500;
  struct mdns_domain service_type, service_dnssd;
  mdns_build_service_domain(&service_type, service, 0);
  mdns_build_dnssd_domain(&service_dnssd);
  /* When answering to a legacy querier, we need to repeat the question and
   * limit the ttl to the short legacy ttl */
  if(msg->legacy_query) {
    /* Repeating the question only needs to be done for the question asked
     * (max one question), not for the additional records. */
    if(reply->questions < 1) {
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Add question for legacy query\n"));
      res = mdns_add_question(reply, &service_dnssd, DNS_RRTYPE_PTR, DNS_RRCLASS_IN, 0);
      if (res != ERR_OK) {
        return res;
      }
      reply->questions = 1;
    }
    /* ttl of legacy answer may not be greater then 10 seconds */
    ttl = MDNS_TTL_10;
  }
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Responding with service type PTR record\n"));
  return mdns_add_answer(reply, &service_dnssd, DNS_RRTYPE_PTR, DNS_RRCLASS_IN,
                         0, ttl, NULL, 0, &service_type);
}

/** Write a servicetype -> servicename PTR RR to outpacket */
static err_t
mdns_add_servicename_ptr_answer(struct mdns_outpacket *reply, struct mdns_outmsg *msg,
                                struct mdns_service *service)
{
  err_t res;
  u32_t ttl = MDNS_TTL_120;
  struct mdns_domain service_type, service_instance;
  mdns_build_service_domain(&service_type, service, 0);
  mdns_build_service_domain(&service_instance, service, 1);
  /* When answering to a legacy querier, we need to repeat the question and
   * limit the ttl to the short legacy ttl */
  if(msg->legacy_query) {
    /* Repeating the question only needs to be done for the question asked
     * (max one question), not for the additional records. */
    if(reply->questions < 1) {
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Add question for legacy query\n"));
      res = mdns_add_question(reply, &service_type, DNS_RRTYPE_PTR, DNS_RRCLASS_IN, 0);
      if (res != ERR_OK) {
        return res;
      }
      reply->questions = 1;
    }
    /* ttl of legacy answer may not be greater then 10 seconds */
    ttl = MDNS_TTL_10;
  }
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Responding with service name PTR record\n"));
  return mdns_add_answer(reply, &service_type, DNS_RRTYPE_PTR, DNS_RRCLASS_IN,
                         0, ttl, NULL, 0, &service_instance);
}

/** Write a SRV RR to outpacket */
static err_t
mdns_add_srv_answer(struct mdns_outpacket *reply, struct mdns_outmsg *msg,
                    struct mdns_host *mdns, struct mdns_service *service)
{
  err_t res;
  u32_t ttl = MDNS_TTL_120;
  struct mdns_domain service_instance, srvhost;
  u16_t srvdata[3];
  mdns_build_service_domain(&service_instance, service, 1);
  mdns_build_host_domain(&srvhost, mdns);
  if (msg->legacy_query) {
    /* RFC 6762 section 18.14:
     * In legacy unicast responses generated to answer legacy queries,
     * name compression MUST NOT be performed on SRV records.
     */
    srvhost.skip_compression = 1;
    /* When answering to a legacy querier, we need to repeat the question and
     * limit the ttl to the short legacy ttl.
     * Repeating the question only needs to be done for the question asked
     * (max one question), not for the additional records. */
    if(reply->questions < 1) {
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Add question for legacy query\n"));
      res = mdns_add_question(reply, &service_instance, DNS_RRTYPE_SRV, DNS_RRCLASS_IN, 0);
      if (res != ERR_OK) {
        return res;
      }
      reply->questions = 1;
    }
    /* ttl of legacy answer may not be greater then 10 seconds */
    ttl = MDNS_TTL_10;
  }
  srvdata[0] = lwip_htons(SRV_PRIORITY);
  srvdata[1] = lwip_htons(SRV_WEIGHT);
  srvdata[2] = lwip_htons(service->port);
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Responding with SRV record\n"));
  return mdns_add_answer(reply, &service_instance, DNS_RRTYPE_SRV, DNS_RRCLASS_IN,
                         msg->cache_flush, ttl,
                         (const u8_t *) &srvdata, sizeof(srvdata), &srvhost);
}

/** Write a TXT RR to outpacket */
static err_t
mdns_add_txt_answer(struct mdns_outpacket *reply, struct mdns_outmsg *msg,
                    struct mdns_service *service)
{
  err_t res;
  u32_t ttl = MDNS_TTL_120;
  struct mdns_domain service_instance;
  mdns_build_service_domain(&service_instance, service, 1);
  mdns_prepare_txtdata(service);
  /* When answering to a legacy querier, we need to repeat the question and
   * limit the ttl to the short legacy ttl */
  if(msg->legacy_query) {
    /* Repeating the question only needs to be done for the question asked
     * (max one question), not for the additional records. */
    if(reply->questions < 1) {
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Add question for legacy query\n"));
      res = mdns_add_question(reply, &service_instance, DNS_RRTYPE_TXT, DNS_RRCLASS_IN, 0);
      if (res != ERR_OK) {
        return res;
      }
      reply->questions = 1;
    }
    /* ttl of legacy answer may not be greater then 10 seconds */
    ttl = MDNS_TTL_10;
  }
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Responding with TXT record\n"));
  return mdns_add_answer(reply, &service_instance, DNS_RRTYPE_TXT, DNS_RRCLASS_IN,
                         msg->cache_flush, ttl, (u8_t *) &service->txtdata.name,
                         service->txtdata.length, NULL);
}


static err_t
mdns_add_probe_questions_to_outpacket(struct mdns_outpacket *outpkt, struct mdns_outmsg *msg,
                                      struct netif *netif)
{
  err_t res;
  int i;
  struct mdns_host *mdns = netif_mdns_data(netif);

  /* Write host questions (probing or legacy query) */
  if(msg->host_questions & QUESTION_PROBE_HOST_ANY) {
    res = mdns_add_any_host_question(outpkt, mdns, 1);
    if (res != ERR_OK) {
      return res;
    }
    outpkt->questions++;
  }
  /* Write service questions (probing or legacy query) */
  for (i = 0; i < MDNS_MAX_SERVICES; i++) {
    struct mdns_service* service = mdns->services[i];
    if (!service) {
      continue;
    }
    if(msg->serv_questions[i] & QUESTION_PROBE_SERVICE_NAME_ANY) {
      res = mdns_add_any_service_question(outpkt, service, 1);
      if (res != ERR_OK) {
        return res;
      }
      outpkt->questions++;
    }
  }
  return ERR_OK;
}

#if LWIP_MDNS_SEARCH
static err_t
mdns_add_query_question_to_outpacket(struct mdns_outpacket *outpkt, struct mdns_outmsg *msg)
{
  err_t res;
  /* Write legacy query question */
  if(msg->query) {
    struct mdns_request *req = msg->query;
    struct mdns_domain dom;
    /* Build question domain */
    mdns_build_request_domain(&dom, req, req->name[0]);
    /* Add query question to output packet */
    res = mdns_add_question(outpkt, &dom, req->qtype, DNS_RRCLASS_IN, 0);
    if (res != ERR_OK) {
      return res;
    }
    outpkt->questions++;
  }
  return ERR_OK;
}
#endif

/**
 * Create packet with chosen answers as a reply
 *
 * Add all selected answers / questions
 * Add additional answers based on the selected answers
 */
err_t
mdns_create_outpacket(struct netif *netif, struct mdns_outmsg *msg,
                      struct mdns_outpacket *outpkt)
{
  struct mdns_host *mdns = netif_mdns_data(netif);
  struct mdns_service *service;
  err_t res;
  int i;
  u16_t answers = 0;

#if LWIP_MDNS_SEARCH
  res = mdns_add_query_question_to_outpacket(outpkt, msg);
  if (res != ERR_OK) {
    return res;
  }
#endif

  res = mdns_add_probe_questions_to_outpacket(outpkt, msg, netif);
  if (res != ERR_OK) {
    return res;
  }

  /* Write answers to host questions */
#if LWIP_IPV4
  if (msg->host_replies & REPLY_HOST_A) {
    res = mdns_add_a_answer(outpkt, msg, netif);
    if (res != ERR_OK) {
      return res;
    }
    answers++;
  }
  if (msg->host_replies & REPLY_HOST_PTR_V4) {
    res = mdns_add_hostv4_ptr_answer(outpkt, msg, netif);
    if (res != ERR_OK) {
      return res;
    }
    answers++;
  }
#endif
#if LWIP_IPV6
  if (msg->host_replies & REPLY_HOST_AAAA) {
    int addrindex;
    for (addrindex = 0; addrindex < LWIP_IPV6_NUM_ADDRESSES; addrindex++) {
      if (ip6_addr_isvalid(netif_ip6_addr_state(netif, addrindex))) {
        res = mdns_add_aaaa_answer(outpkt, msg, netif, addrindex);
        if (res != ERR_OK) {
          return res;
        }
        answers++;
      }
    }
  }
  if (msg->host_replies & REPLY_HOST_PTR_V6) {
    u8_t rev_addrs = msg->host_reverse_v6_replies;
    int addrindex = 0;
    while (rev_addrs) {
      if (rev_addrs & 1) {
        res = mdns_add_hostv6_ptr_answer(outpkt, msg, netif, addrindex);
        if (res != ERR_OK) {
          return res;
        }
        answers++;
      }
      addrindex++;
      rev_addrs >>= 1;
    }
  }
#endif

  /* Write answers to service questions */
  for (i = 0; i < MDNS_MAX_SERVICES; i++) {
    service = mdns->services[i];
    if (!service) {
      continue;
    }

    if (msg->serv_replies[i] & REPLY_SERVICE_TYPE_PTR) {
      res = mdns_add_servicetype_ptr_answer(outpkt, msg, service);
      if (res != ERR_OK) {
        return res;
      }
      answers++;
    }

    if (msg->serv_replies[i] & REPLY_SERVICE_NAME_PTR) {
      res = mdns_add_servicename_ptr_answer(outpkt, msg, service);
      if (res != ERR_OK) {
        return res;
      }
      answers++;
    }

    if (msg->serv_replies[i] & REPLY_SERVICE_SRV) {
      res = mdns_add_srv_answer(outpkt, msg, mdns, service);
      if (res != ERR_OK) {
        return res;
      }
      answers++;
    }

    if (msg->serv_replies[i] & REPLY_SERVICE_TXT) {
      res = mdns_add_txt_answer(outpkt, msg, service);
      if (res != ERR_OK) {
        return res;
      }
      answers++;
    }
  }

  /* if this is a response, the data above is anwers, else this is a probe and
   * the answers above goes into auth section */
  if (msg->flags & DNS_FLAG1_RESPONSE) {
    outpkt->answers += answers;
  } else {
    outpkt->authoritative += answers;
  }

  /* All answers written, add additional RRs */
  for (i = 0; i < MDNS_MAX_SERVICES; i++) {
    service = mdns->services[i];
    if (!service) {
      continue;
    }

    if (msg->serv_replies[i] & REPLY_SERVICE_NAME_PTR) {
      /* Our service instance requested, include SRV & TXT
       * if they are already not requested. */
      if (!(msg->serv_replies[i] & REPLY_SERVICE_SRV)) {
        res = mdns_add_srv_answer(outpkt, msg, mdns, service);
        if (res != ERR_OK) {
          return res;
        }
        outpkt->additional++;
      }

      if (!(msg->serv_replies[i] & REPLY_SERVICE_TXT)) {
        res = mdns_add_txt_answer(outpkt, msg, service);
        if (res != ERR_OK) {
          return res;
        }
        outpkt->additional++;
      }
    }

    /* If service instance, SRV, record or an IP address is requested,
     * supply all addresses for the host
     */
    if ((msg->serv_replies[i] & (REPLY_SERVICE_NAME_PTR | REPLY_SERVICE_SRV)) ||
        (msg->host_replies & (REPLY_HOST_A | REPLY_HOST_AAAA))) {
#if LWIP_IPV6
      if (!(msg->host_replies & REPLY_HOST_AAAA)) {
        int addrindex;
        for (addrindex = 0; addrindex < LWIP_IPV6_NUM_ADDRESSES; addrindex++) {
          if (ip6_addr_isvalid(netif_ip6_addr_state(netif, addrindex))) {
            res = mdns_add_aaaa_answer(outpkt, msg, netif, addrindex);
            if (res != ERR_OK) {
              return res;
            }
            outpkt->additional++;
          }
        }
      }
#endif
#if LWIP_IPV4
      if (!(msg->host_replies & REPLY_HOST_A) &&
          !ip4_addr_isany_val(*netif_ip4_addr(netif))) {
        res = mdns_add_a_answer(outpkt, msg, netif);
        if (res != ERR_OK) {
          return res;
        }
        outpkt->additional++;
      }
#endif
    }
  }

  return res;
}

/**
 * Send chosen answers as a reply
 *
 * Create the packet
 * Send the packet
 */
err_t
mdns_send_outpacket(struct mdns_outmsg *msg, struct netif *netif)
{
  struct mdns_outpacket outpkt;
  err_t res;

  memset(&outpkt, 0, sizeof(outpkt));

  res = mdns_create_outpacket(netif, msg, &outpkt);
  if (res != ERR_OK) {
    goto cleanup;
  }

  if (outpkt.pbuf) {
    struct dns_hdr hdr;

    /* Write header */
    memset(&hdr, 0, sizeof(hdr));
    hdr.flags1 = msg->flags;
    hdr.numquestions = lwip_htons(outpkt.questions);
    hdr.numanswers = lwip_htons(outpkt.answers);
    hdr.numauthrr = lwip_htons(outpkt.authoritative);
    hdr.numextrarr = lwip_htons(outpkt.additional);
    hdr.id = lwip_htons(msg->tx_id);
    pbuf_take(outpkt.pbuf, &hdr, sizeof(hdr));

    /* Shrink packet */
    pbuf_realloc(outpkt.pbuf, outpkt.write_offset);

    /* Send created packet */
    LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Sending packet, len=%d\n",
                outpkt.write_offset));

    res = udp_sendto_if(get_mdns_pcb(), outpkt.pbuf, &msg->dest_addr, msg->dest_port, netif);
  }

cleanup:
  if (outpkt.pbuf) {
    pbuf_free(outpkt.pbuf);
    outpkt.pbuf = NULL;
  }
  return res;
}

#if LWIP_IPV4
/**
 *  Called by timeouts when timer is passed, allows multicast IPv4 traffic again.
 *
 *  @param arg  pointer to netif of timeout.
 */
void
mdns_multicast_timeout_reset_ipv4(void *arg)
{
  struct netif *netif = (struct netif*)arg;
  struct mdns_host *mdns = netif_mdns_data(netif);

  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: multicast timeout finished - IPv4\n"));

  mdns->ipv4.multicast_timeout = 0;
}

/**
 *  Called by timeouts when timer is passed, allows direct multicast IPv4 probe
 *  response traffic again and sends out probe response if one was pending
 *
 *  @param arg  pointer to netif of timeout.
 */
void
mdns_multicast_probe_timeout_reset_ipv4(void *arg)
{
  struct netif *netif = (struct netif*)arg;
  struct mdns_host *mdns = netif_mdns_data(netif);
  err_t res;

  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: multicast probe timeout finished - IPv4\n"));

  mdns->ipv4.multicast_probe_timeout = 0;

  if (mdns->ipv4.multicast_msg_waiting) {
    res = mdns_send_outpacket(&mdns->ipv4.delayed_msg_multicast, netif);
    if(res != ERR_OK) {
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Waiting probe multicast send failed - IPv4\n"));
    }
    else {
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Waiting probe multicast send successful - IPv4\n"));
      mdns_clear_outmsg(&mdns->ipv4.delayed_msg_multicast);
      mdns->ipv4.multicast_msg_waiting = 0;
      mdns_start_multicast_timeouts_ipv4(netif);
    }
  }
}

/**
 *  Called by timeouts when timer is passed, allows to send an answer on a QU
 *  question via multicast.
 *
 *  @param arg  pointer to netif of timeout.
 */
void
mdns_multicast_timeout_25ttl_reset_ipv4(void *arg)
{
  struct netif *netif = (struct netif*)arg;
  struct mdns_host *mdns = netif_mdns_data(netif);

  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: multicast timeout 1/4 of ttl finished - IPv4\n"));

  mdns->ipv4.multicast_timeout_25TTL = 0;
}

/**
 *  Called by timeouts when timer is passed, sends out delayed multicast IPv4 response.
 *
 *  @param arg  pointer to netif of timeout.
 */
void
mdns_send_multicast_msg_delayed_ipv4(void *arg)
{
  struct netif *netif = (struct netif*)arg;
  struct mdns_host *mdns = netif_mdns_data(netif);
  err_t res;

  res = mdns_send_outpacket(&mdns->ipv4.delayed_msg_multicast, netif);
  if(res != ERR_OK) {
    LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Delayed multicast send failed - IPv4\n"));
  }
  else {
    LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Delayed multicast send successful - IPv4\n"));
    mdns_clear_outmsg(&mdns->ipv4.delayed_msg_multicast);
    mdns->ipv4.multicast_msg_waiting = 0;
    mdns_start_multicast_timeouts_ipv4(netif);
  }
}

/**
 *  Called by timeouts when timer is passed, sends out delayed unicast IPv4 response.
 *
 *  @param arg  pointer to netif of timeout.
 */
void
mdns_send_unicast_msg_delayed_ipv4(void *arg)
{
  struct netif *netif = (struct netif*)arg;
  struct mdns_host *mdns = netif_mdns_data(netif);
  err_t res;

  res = mdns_send_outpacket(&mdns->ipv4.delayed_msg_unicast, netif);
  if(res != ERR_OK) {
    LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Delayed unicast send failed - IPv4\n"));
  }
  else {
    LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Delayed unicast send successful - IPv4\n"));
    mdns_clear_outmsg(&mdns->ipv4.delayed_msg_unicast);
    mdns->ipv4.unicast_msg_in_use = 0;
  }
}

/** Start all multicast timeouts for IPv4
 *  Timeouts started:
 *    - do not multicast within one second
 *    - do not multicast a probe response within 250ms
 *    - send a multicast answer on a QU question if not send recently.
 *
 *  @param netif network interface to start timeouts on
 */
void
mdns_start_multicast_timeouts_ipv4(struct netif *netif)
{
  struct mdns_host *mdns = netif_mdns_data(netif);

  mdns_set_timeout(netif, MDNS_MULTICAST_TIMEOUT, mdns_multicast_timeout_reset_ipv4,
                   &mdns->ipv4.multicast_timeout);
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: multicast timeout started - IPv4\n"));
  mdns_set_timeout(netif, MDNS_MULTICAST_PROBE_TIMEOUT, mdns_multicast_probe_timeout_reset_ipv4,
                   &mdns->ipv4.multicast_probe_timeout);
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: multicast probe timeout started - IPv4\n"));
  mdns_set_timeout(netif, MDNS_MULTICAST_TIMEOUT_25TTL, mdns_multicast_timeout_25ttl_reset_ipv4,
                   &mdns->ipv4.multicast_timeout_25TTL);
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: multicast timeout 1/4 of ttl started - IPv4\n"));
}
#endif

#if LWIP_IPV6
/**
 *  Called by timeouts when timer is passed, allows multicast IPv6 traffic again.
 *
 *  @param arg  pointer to netif of timeout.
 */
void
mdns_multicast_timeout_reset_ipv6(void *arg)
{
  struct netif *netif = (struct netif*)arg;
  struct mdns_host *mdns = netif_mdns_data(netif);

  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: multicast timeout finished - IPv6\n"));

  mdns->ipv6.multicast_timeout = 0;
}

/**
 *  Called by timeouts when timer is passed, allows direct multicast IPv6 probe
 *  response traffic again and sends out probe response if one was pending
 *
 *  @param arg  pointer to netif of timeout.
 */
void
mdns_multicast_probe_timeout_reset_ipv6(void *arg)
{
  struct netif *netif = (struct netif*)arg;
  struct mdns_host *mdns = netif_mdns_data(netif);
  err_t res;

  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: multicast probe timeout finished - IPv6\n"));

  mdns->ipv6.multicast_probe_timeout = 0;

  if (mdns->ipv6.multicast_msg_waiting) {
    res = mdns_send_outpacket(&mdns->ipv6.delayed_msg_multicast, netif);
    if(res != ERR_OK) {
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Waiting probe multicast send failed - IPv6\n"));
    }
    else {
      LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Waiting probe multicast send successful - IPv6\n"));
      mdns_clear_outmsg(&mdns->ipv6.delayed_msg_multicast);
      mdns->ipv6.multicast_msg_waiting = 0;
      mdns_start_multicast_timeouts_ipv6(netif);
    }
  }
}

/**
 *  Called by timeouts when timer is passed, allows to send an answer on a QU
 *  question via multicast.
 *
 *  @param arg  pointer to netif of timeout.
 */
void
mdns_multicast_timeout_25ttl_reset_ipv6(void *arg)
{
  struct netif *netif = (struct netif*)arg;
  struct mdns_host *mdns = netif_mdns_data(netif);

  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: multicast timeout 1/4 of ttl finished - IPv6\n"));

  mdns->ipv6.multicast_timeout_25TTL = 0;
}

/**
 *  Called by timeouts when timer is passed, sends out delayed multicast IPv6 response.
 *
 *  @param arg  pointer to netif of timeout.
 */
void
mdns_send_multicast_msg_delayed_ipv6(void *arg)
{
  struct netif *netif = (struct netif*)arg;
  struct mdns_host *mdns = netif_mdns_data(netif);
  err_t res;

  res = mdns_send_outpacket(&mdns->ipv6.delayed_msg_multicast, netif);
  if(res != ERR_OK) {
    LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Delayed multicast send failed - IPv6\n"));
  }
  else {
    LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Delayed multicast send successful - IPv6\n"));
    mdns_clear_outmsg(&mdns->ipv6.delayed_msg_multicast);
    mdns->ipv6.multicast_msg_waiting = 0;
    mdns_start_multicast_timeouts_ipv6(netif);
  }
}

/**
 *  Called by timeouts when timer is passed, sends out delayed unicast IPv6 response.
 *
 *  @param arg  pointer to netif of timeout.
 */
void
mdns_send_unicast_msg_delayed_ipv6(void *arg)
{
  struct netif *netif = (struct netif*)arg;
  struct mdns_host *mdns = netif_mdns_data(netif);
  err_t res;

  res = mdns_send_outpacket(&mdns->ipv6.delayed_msg_unicast, netif);
  if(res != ERR_OK) {
    LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Delayed unicast send failed - IPv6\n"));
  }
  else {
    LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Delayed unicast send successful - IPv6\n"));
    mdns_clear_outmsg(&mdns->ipv6.delayed_msg_unicast);
    mdns->ipv6.unicast_msg_in_use = 0;
  }
}

/** Start all multicast timeouts for IPv6
 *  Timeouts started:
 *    - do not multicast within one second
 *    - do not multicast a probe response within 250ms
 *    - send a multicast answer on a QU question if not send recently.
 *
 *  @param netif network interface to start timeouts on
 */
void
mdns_start_multicast_timeouts_ipv6(struct netif *netif)
{
  struct mdns_host *mdns = netif_mdns_data(netif);

  mdns_set_timeout(netif, MDNS_MULTICAST_TIMEOUT, mdns_multicast_timeout_reset_ipv6,
                   &mdns->ipv6.multicast_timeout);
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: multicast timeout started - IPv6\n"));
  mdns_set_timeout(netif, MDNS_MULTICAST_PROBE_TIMEOUT, mdns_multicast_probe_timeout_reset_ipv6,
                   &mdns->ipv6.multicast_probe_timeout);
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: multicast probe timeout started - IPv6\n"));
  mdns_set_timeout(netif, MDNS_MULTICAST_TIMEOUT_25TTL, mdns_multicast_timeout_25ttl_reset_ipv6,
                   &mdns->ipv6.multicast_timeout_25TTL);
  LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: multicast timeout 1/4 of ttl started - IPv6\n"));
}
#endif

/**
 *  This function clears the output message without changing the destination
 *  address or port. This is useful for clearing the delayed msg structs without
 *  losing the set IP.
 *
 *  @param outmsg pointer to output message to clear.
 */
static void
mdns_clear_outmsg(struct mdns_outmsg *outmsg)
{
  int i;

  outmsg->tx_id = 0;
  outmsg->flags = 0;
  outmsg->cache_flush = 0;
  outmsg->unicast_reply_requested = 0;
  outmsg->legacy_query = 0;
  outmsg->probe_query_recv = 0;
  outmsg->host_questions = 0;
  outmsg->host_replies = 0;
  outmsg->host_reverse_v6_replies = 0;

  for(i = 0; i < MDNS_MAX_SERVICES; i++) {
    outmsg->serv_questions[i] = 0;
    outmsg->serv_replies[i] = 0;
  }
}

/**
 *  Sets a timer that calls the handler when finished.
 *  Depending on the busy_flag the timer is restarted or started. The flag is
 *  set before return. Sys_timeout does not give us this functionality.
 *
 *  @param netif      Network interface info
 *  @param msecs      Time value to set
 *  @param handler    Callback function to call
 *  @param busy_flag  Pointer to flag that displays if the timer is running or not.
 */
void
mdns_set_timeout(struct netif *netif, u32_t msecs, sys_timeout_handler handler,
                 u8_t *busy_flag)
{
  if(*busy_flag) {
    /* restart timer */
    sys_untimeout(handler, netif);
    sys_timeout(msecs, handler, netif);
  }
  else {
    /* start timer */
    sys_timeout(msecs, handler, netif);
  }
  /* Now we have a timer running */
  *busy_flag = 1;
}

#ifdef LWIP_MDNS_SEARCH
/**
 * Send search request containing all our known data
 * @param req The request to send
 * @param netif The network interface to send on
 * @param destination The target address to send to (usually multicast address)
 */
err_t
mdns_send_request(struct mdns_request *req, struct netif *netif, const ip_addr_t *destination)
{
  struct mdns_outmsg outmsg;
  err_t res;

  memset(&outmsg, 0, sizeof(outmsg));
  outmsg.query = req;
  outmsg.dest_port = LWIP_IANA_PORT_MDNS;
  SMEMCPY(&outmsg.dest_addr, destination, sizeof(outmsg.dest_addr));
  res = mdns_send_outpacket(&outmsg, netif);
  if(res != ERR_OK) {
    LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Multicast query request send failed\n"));
  }
  else {
    LWIP_DEBUGF(MDNS_DEBUG, ("MDNS: Multicast query request send successful\n"));
  }
  return res;
}
#endif

#endif /* LWIP_MDNS_RESPONDER */
