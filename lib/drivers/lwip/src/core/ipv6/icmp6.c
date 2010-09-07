/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
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
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/* Some ICMP messages should be passed to the transport protocols. This
   is not implemented. */

#include "lwip/opt.h"

#if LWIP_ICMP /* don't build if not configured for use in lwipopts.h */

#include "lwip/icmp.h"
#include "lwip/inet.h"
#include "lwip/ip.h"
#include "lwip/def.h"
#include "lwip/stats.h"

void
icmp_input(struct pbuf *p, struct netif *inp)
{
  u8_t type;
  struct icmp_echo_hdr *iecho;
  struct ip_hdr *iphdr;
  struct ip_addr tmpaddr;

  ICMP_STATS_INC(icmp.recv);

  /* TODO: check length before accessing payload! */

  type = ((u8_t *)p->payload)[0];

  switch (type) {
  case ICMP6_ECHO:
    LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: ping\n"));

    if (p->tot_len < sizeof(struct icmp_echo_hdr)) {
      LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: bad ICMP echo received\n"));

      pbuf_free(p);
      ICMP_STATS_INC(icmp.lenerr);
      return;
    }
    iecho = p->payload;
    iphdr = (struct ip_hdr *)((u8_t *)p->payload - IP_HLEN);
    if (inet_chksum_pbuf(p) != 0) {
      LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: checksum failed for received ICMP echo (%"X16_F")\n", inet_chksum_pseudo(p, &(iphdr->src), &(iphdr->dest), IP_PROTO_ICMP, p->tot_len)));
      ICMP_STATS_INC(icmp.chkerr);
    /*      return;*/
    }
    LWIP_DEBUGF(ICMP_DEBUG, ("icmp: p->len %"S16_F" p->tot_len %"S16_F"\n", p->len, p->tot_len));
    ip_addr_set(&tmpaddr, &(iphdr->src));
    ip_addr_set(&(iphdr->src), &(iphdr->dest));
    ip_addr_set(&(iphdr->dest), &tmpaddr);
    iecho->type = ICMP6_ER;
    /* adjust the checksum */
    if (iecho->chksum >= htons(0xffff - (ICMP6_ECHO << 8))) {
      iecho->chksum += htons(ICMP6_ECHO << 8) + 1;
    } else {
      iecho->chksum += htons(ICMP6_ECHO << 8);
    }
    LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: checksum failed for received ICMP echo (%"X16_F")\n", inet_chksum_pseudo(p, &(iphdr->src), &(iphdr->dest), IP_PROTO_ICMP, p->tot_len)));
    ICMP_STATS_INC(icmp.xmit);

    /*    LWIP_DEBUGF("icmp: p->len %"U16_F" p->tot_len %"U16_F"\n", p->len, p->tot_len);*/
    ip_output_if (p, &(iphdr->src), IP_HDRINCL,
     iphdr->hoplim, IP_PROTO_ICMP, inp);
    break;
  default:
    LWIP_DEBUGF(ICMP_DEBUG, ("icmp_input: ICMP type %"S16_F" not supported.\n", (s16_t)type));
    ICMP_STATS_INC(icmp.proterr);
    ICMP_STATS_INC(icmp.drop);
  }

  pbuf_free(p);
}

void
icmp_dest_unreach(struct pbuf *p, enum icmp_dur_type t)
{
  struct pbuf *q;
  struct ip_hdr *iphdr;
  struct icmp_dur_hdr *idur;

  /* @todo: can this be PBUF_LINK instead of PBUF_IP? */
  q = pbuf_alloc(PBUF_IP, 8 + IP_HLEN + 8, PBUF_RAM);
  /* ICMP header + IP header + 8 bytes of data */
  if (q == NULL) {
    LWIP_DEBUGF(ICMP_DEBUG, ("icmp_dest_unreach: failed to allocate pbuf for ICMP packet.\n"));
    pbuf_free(p);
    return;
  }
  LWIP_ASSERT("check that first pbuf can hold icmp message",
             (q->len >= (8 + IP_HLEN + 8)));

  iphdr = p->payload;

  idur = q->payload;
  idur->type = (u8_t)ICMP6_DUR;
  idur->icode = (u8_t)t;

  SMEMCPY((u8_t *)q->payload + 8, p->payload, IP_HLEN + 8);

  /* calculate checksum */
  idur->chksum = 0;
  idur->chksum = inet_chksum(idur, q->len);
  ICMP_STATS_INC(icmp.xmit);

  ip_output(q, NULL,
      (struct ip_addr *)&(iphdr->src), ICMP_TTL, IP_PROTO_ICMP);
  pbuf_free(q);
}

void
icmp_time_exceeded(struct pbuf *p, enum icmp_te_type t)
{
  struct pbuf *q;
  struct ip_hdr *iphdr;
  struct icmp_te_hdr *tehdr;

  LWIP_DEBUGF(ICMP_DEBUG, ("icmp_time_exceeded\n"));

  /* @todo: can this be PBUF_LINK instead of PBUF_IP? */
  q = pbuf_alloc(PBUF_IP, 8 + IP_HLEN + 8, PBUF_RAM);
  /* ICMP header + IP header + 8 bytes of data */
  if (q == NULL) {
    LWIP_DEBUGF(ICMP_DEBUG, ("icmp_dest_unreach: failed to allocate pbuf for ICMP packet.\n"));
    pbuf_free(p);
    return;
  }
  LWIP_ASSERT("check that first pbuf can hold icmp message",
             (q->len >= (8 + IP_HLEN + 8)));

  iphdr = p->payload;

  tehdr = q->payload;
  tehdr->type = (u8_t)ICMP6_TE;
  tehdr->icode = (u8_t)t;

  /* copy fields from original packet */
  SMEMCPY((u8_t *)q->payload + 8, (u8_t *)p->payload, IP_HLEN + 8);

  /* calculate checksum */
  tehdr->chksum = 0;
  tehdr->chksum = inet_chksum(tehdr, q->len);
  ICMP_STATS_INC(icmp.xmit);
  ip_output(q, NULL,
      (struct ip_addr *)&(iphdr->src), ICMP_TTL, IP_PROTO_ICMP);
  pbuf_free(q);
}

#endif /* LWIP_ICMP */
