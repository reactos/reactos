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



/* ip.c
 *
 * This is the code for the IP layer for IPv6.
 *
 */


#include "lwip/opt.h"

#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/ip.h"
#include "lwip/inet.h"
#include "lwip/netif.h"
#include "lwip/icmp.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"

#include "lwip/stats.h"

#include "arch/perf.h"

/* ip_init:
 *
 * Initializes the IP layer.
 */

void
ip_init(void)
{
}

/* ip_route:
 *
 * Finds the appropriate network interface for a given IP address. It searches the
 * list of network interfaces linearly. A match is found if the masked IP address of
 * the network interface equals the masked IP address given to the function.
 */

struct netif *
ip_route(struct ip_addr *dest)
{
  struct netif *netif;

  for(netif = netif_list; netif != NULL; netif = netif->next) {
    if (ip_addr_netcmp(dest, &(netif->ip_addr), &(netif->netmask))) {
      return netif;
    }
  }

  return netif_default;
}

/* ip_forward:
 *
 * Forwards an IP packet. It finds an appropriate route for the packet, decrements
 * the TTL value of the packet, adjusts the checksum and outputs the packet on the
 * appropriate interface.
 */

static void
ip_forward(struct pbuf *p, struct ip_hdr *iphdr)
{
  struct netif *netif;

  PERF_START;

  if ((netif = ip_route((struct ip_addr *)&(iphdr->dest))) == NULL) {

    LWIP_DEBUGF(IP_DEBUG, ("ip_input: no forwarding route found for "));
#if IP_DEBUG
    ip_addr_debug_print(IP_DEBUG, ((struct ip_addr *)&(iphdr->dest)));
#endif /* IP_DEBUG */
    LWIP_DEBUGF(IP_DEBUG, ("\n"));
    pbuf_free(p);
    return;
  }
  /* Decrement TTL and send ICMP if ttl == 0. */
  if (--iphdr->hoplim == 0) {
#if LWIP_ICMP
    /* Don't send ICMP messages in response to ICMP messages */
    if (iphdr->nexthdr != IP_PROTO_ICMP) {
      icmp_time_exceeded(p, ICMP_TE_TTL);
    }
#endif /* LWIP_ICMP */
    pbuf_free(p);
    return;
  }

  /* Incremental update of the IP checksum. */
  /*  if (iphdr->chksum >= htons(0xffff - 0x100)) {
    iphdr->chksum += htons(0x100) + 1;
  } else {
    iphdr->chksum += htons(0x100);
    }*/


  LWIP_DEBUGF(IP_DEBUG, ("ip_forward: forwarding packet to "));
#if IP_DEBUG
  ip_addr_debug_print(IP_DEBUG, ((struct ip_addr *)&(iphdr->dest)));
#endif /* IP_DEBUG */
  LWIP_DEBUGF(IP_DEBUG, ("\n"));

  IP_STATS_INC(ip.fw);
  IP_STATS_INC(ip.xmit);

  PERF_STOP("ip_forward");

  netif->output(netif, p, (struct ip_addr *)&(iphdr->dest));
}

/* ip_input:
 *
 * This function is called by the network interface device driver when an IP packet is
 * received. The function does the basic checks of the IP header such as packet size
 * being at least larger than the header size etc. If the packet was not destined for
 * us, the packet is forwarded (using ip_forward). The IP checksum is always checked.
 *
 * Finally, the packet is sent to the upper layer protocol input function.
 */

void
ip_input(struct pbuf *p, struct netif *inp) {
  struct ip_hdr *iphdr;
  struct netif *netif;


  PERF_START;

#if IP_DEBUG
  ip_debug_print(p);
#endif /* IP_DEBUG */


  IP_STATS_INC(ip.recv);

  /* identify the IP header */
  iphdr = p->payload;


  if (iphdr->v != 6) {
    LWIP_DEBUGF(IP_DEBUG, ("IP packet dropped due to bad version number\n"));
#if IP_DEBUG
    ip_debug_print(p);
#endif /* IP_DEBUG */
    pbuf_free(p);
    IP_STATS_INC(ip.err);
    IP_STATS_INC(ip.drop);
    return;
  }

  /* is this packet for us? */
  for(netif = netif_list; netif != NULL; netif = netif->next) {
#if IP_DEBUG
    LWIP_DEBUGF(IP_DEBUG, ("ip_input: iphdr->dest "));
    ip_addr_debug_print(IP_DEBUG, ((struct ip_addr *)&(iphdr->dest)));
    LWIP_DEBUGF(IP_DEBUG, ("netif->ip_addr "));
    ip_addr_debug_print(IP_DEBUG, ((struct ip_addr *)&(iphdr->dest)));
    LWIP_DEBUGF(IP_DEBUG, ("\n"));
#endif /* IP_DEBUG */
    if (ip_addr_cmp(&(iphdr->dest), &(netif->ip_addr))) {
      break;
    }
  }


  if (netif == NULL) {
    /* packet not for us, route or discard */
#if IP_FORWARD
    ip_forward(p, iphdr);
#endif
    pbuf_free(p);
    return;
  }

  pbuf_realloc(p, IP_HLEN + ntohs(iphdr->len));

  /* send to upper layers */
#if IP_DEBUG
  /*  LWIP_DEBUGF("ip_input: \n");
  ip_debug_print(p);
  LWIP_DEBUGF("ip_input: p->len %"U16_F" p->tot_len %"U16_F"\n", p->len, p->tot_len);*/
#endif /* IP_DEBUG */

  if(pbuf_header(p, -IP_HLEN)) {
    LWIP_ASSERT("Can't move over header in packet", 0);
    return;
  }

  switch (iphdr->nexthdr) {
  case IP_PROTO_UDP:
    udp_input(p, inp);
    break;
  case IP_PROTO_TCP:
    tcp_input(p, inp);
    break;
#if LWIP_ICMP
  case IP_PROTO_ICMP:
    icmp_input(p, inp);
    break;
#endif /* LWIP_ICMP */
  default:
#if LWIP_ICMP
    /* send ICMP destination protocol unreachable */
    icmp_dest_unreach(p, ICMP_DUR_PROTO);
#endif /* LWIP_ICMP */
    pbuf_free(p);
    LWIP_DEBUGF(IP_DEBUG, ("Unsupported transport protocol %"U16_F"\n",
          iphdr->nexthdr));

    IP_STATS_INC(ip.proterr);
    IP_STATS_INC(ip.drop);
  }
  PERF_STOP("ip_input");
}


/* ip_output_if:
 *
 * Sends an IP packet on a network interface. This function constructs the IP header
 * and calculates the IP header checksum. If the source IP address is NULL,
 * the IP address of the outgoing network interface is filled in as source address.
 */

err_t
ip_output_if (struct pbuf *p, struct ip_addr *src, struct ip_addr *dest,
       u8_t ttl,
       u8_t proto, struct netif *netif)
{
  struct ip_hdr *iphdr;

  PERF_START;

  LWIP_DEBUGF(IP_DEBUG, ("len %"U16_F" tot_len %"U16_F"\n", p->len, p->tot_len));
  if (pbuf_header(p, IP_HLEN)) {
    LWIP_DEBUGF(IP_DEBUG, ("ip_output: not enough room for IP header in pbuf\n"));
    IP_STATS_INC(ip.err);

    return ERR_BUF;
  }
  LWIP_DEBUGF(IP_DEBUG, ("len %"U16_F" tot_len %"U16_F"\n", p->len, p->tot_len));

  iphdr = p->payload;


  if (dest != IP_HDRINCL) {
    LWIP_DEBUGF(IP_DEBUG, ("!IP_HDRLINCL\n"));
    iphdr->hoplim = ttl;
    iphdr->nexthdr = proto;
    iphdr->len = htons(p->tot_len - IP_HLEN);
    ip_addr_set(&(iphdr->dest), dest);

    iphdr->v = 6;

    if (ip_addr_isany(src)) {
      ip_addr_set(&(iphdr->src), &(netif->ip_addr));
    } else {
      ip_addr_set(&(iphdr->src), src);
    }

  } else {
    dest = &(iphdr->dest);
  }

  IP_STATS_INC(ip.xmit);

  LWIP_DEBUGF(IP_DEBUG, ("ip_output_if: %c%c (len %"U16_F")\n", netif->name[0], netif->name[1], p->tot_len));
#if IP_DEBUG
  ip_debug_print(p);
#endif /* IP_DEBUG */

  PERF_STOP("ip_output_if");
  return netif->output(netif, p, dest);
}

/* ip_output:
 *
 * Simple interface to ip_output_if. It finds the outgoing network interface and
 * calls upon ip_output_if to do the actual work.
 */

err_t
ip_output(struct pbuf *p, struct ip_addr *src, struct ip_addr *dest,
    u8_t ttl, u8_t proto)
{
  struct netif *netif;
  if ((netif = ip_route(dest)) == NULL) {
    LWIP_DEBUGF(IP_DEBUG, ("ip_output: No route to 0x%"X32_F"\n", dest->addr));
    IP_STATS_INC(ip.rterr);
    return ERR_RTE;
  }

  return ip_output_if (p, src, dest, ttl, proto, netif);
}

#if LWIP_NETIF_HWADDRHINT
err_t
ip_output_hinted(struct pbuf *p, struct ip_addr *src, struct ip_addr *dest,
          u8_t ttl, u8_t tos, u8_t proto, u8_t *addr_hint)
{
  struct netif *netif;
  err_t err;

  if ((netif = ip_route(dest)) == NULL) {
    LWIP_DEBUGF(IP_DEBUG, ("ip_output: No route to 0x%"X32_F"\n", dest->addr));
    IP_STATS_INC(ip.rterr);
    return ERR_RTE;
  }

  netif->addr_hint = addr_hint;
  err = ip_output_if(p, src, dest, ttl, tos, proto, netif);
  netif->addr_hint = NULL;

  return err;
}
#endif /* LWIP_NETIF_HWADDRHINT*/

#if IP_DEBUG
void
ip_debug_print(struct pbuf *p)
{
  struct ip_hdr *iphdr = p->payload;

  LWIP_DEBUGF(IP_DEBUG, ("IP header:\n"));
  LWIP_DEBUGF(IP_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(IP_DEBUG, ("|%2"S16_F" |  %"X16_F"%"X16_F"  |      %"X16_F"%"X16_F"           | (v, traffic class, flow label)\n",
        iphdr->v,
        iphdr->tclass1, iphdr->tclass2,
        iphdr->flow1, iphdr->flow2));
  LWIP_DEBUGF(IP_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(IP_DEBUG, ("|    %5"U16_F"      | %2"U16_F"  |  %2"U16_F"   | (len, nexthdr, hoplim)\n",
        ntohs(iphdr->len),
        iphdr->nexthdr,
        iphdr->hoplim));
  LWIP_DEBUGF(IP_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(IP_DEBUG, ("|       %4"X32_F"      |       %4"X32_F"     | (src)\n",
        (ntohl(iphdr->src.addr[0]) >> 16) & 0xffff,
        ntohl(iphdr->src.addr[0]) & 0xffff));
  LWIP_DEBUGF(IP_DEBUG, ("|       %4"X32_F"      |       %4"X32_F"     | (src)\n",
        (ntohl(iphdr->src.addr[1]) >> 16) & 0xffff,
        ntohl(iphdr->src.addr[1]) & 0xffff));
  LWIP_DEBUGF(IP_DEBUG, ("|       %4"X32_F"      |       %4"X32_F"     | (src)\n",
        (ntohl(iphdr->src.addr[2]) >> 16) & 0xffff,
        ntohl(iphdr->src.addr[2]) & 0xffff));
  LWIP_DEBUGF(IP_DEBUG, ("|       %4"X32_F"      |       %4"X32_F"     | (src)\n",
        (ntohl(iphdr->src.addr[3]) >> 16) & 0xffff,
        ntohl(iphdr->src.addr[3]) & 0xffff));
  LWIP_DEBUGF(IP_DEBUG, ("+-------------------------------+\n"));
  LWIP_DEBUGF(IP_DEBUG, ("|       %4"X32_F"      |       %4"X32_F"     | (dest)\n",
        (ntohl(iphdr->dest.addr[0]) >> 16) & 0xffff,
        ntohl(iphdr->dest.addr[0]) & 0xffff));
  LWIP_DEBUGF(IP_DEBUG, ("|       %4"X32_F"      |       %4"X32_F"     | (dest)\n",
        (ntohl(iphdr->dest.addr[1]) >> 16) & 0xffff,
        ntohl(iphdr->dest.addr[1]) & 0xffff));
  LWIP_DEBUGF(IP_DEBUG, ("|       %4"X32_F"      |       %4"X32_F"     | (dest)\n",
        (ntohl(iphdr->dest.addr[2]) >> 16) & 0xffff,
        ntohl(iphdr->dest.addr[2]) & 0xffff));
  LWIP_DEBUGF(IP_DEBUG, ("|       %4"X32_F"      |       %4"X32_F"     | (dest)\n",
        (ntohl(iphdr->dest.addr[3]) >> 16) & 0xffff,
        ntohl(iphdr->dest.addr[3]) & 0xffff));
  LWIP_DEBUGF(IP_DEBUG, ("+-------------------------------+\n"));
}
#endif /* IP_DEBUG */
