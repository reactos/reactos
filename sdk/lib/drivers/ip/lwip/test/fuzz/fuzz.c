/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
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
 *
 */

#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/dns.h"
#include "netif/etharp.h"
#if LWIP_IPV6
#include "lwip/ethip6.h"
#include "lwip/nd6.h"
#endif

#include "lwip/apps/httpd.h"
#include "lwip/apps/snmp.h"
#include "lwip/apps/lwiperf.h"
#include "lwip/apps/mdns.h"

#include <string.h>
#include <stdio.h>

/* This define enables multi packet processing.
 * For this, the input is interpreted as 2 byte length + data + 2 byte length + data...
 * #define LWIP_FUZZ_MULTI_PACKET
*/
#ifdef LWIP_FUZZ_MULTI_PACKET
u8_t pktbuf[20000];
#else
u8_t pktbuf[2000];
#endif

/* no-op send function */
static err_t lwip_tx_func(struct netif *netif, struct pbuf *p)
{
  LWIP_UNUSED_ARG(netif);
  LWIP_UNUSED_ARG(p);
  return ERR_OK;
}

static err_t testif_init(struct netif *netif)
{
  netif->name[0] = 'f';
  netif->name[1] = 'z';
  netif->output = etharp_output;
  netif->linkoutput = lwip_tx_func;
  netif->mtu = 1500;
  netif->hwaddr_len = 6;
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP;

  netif->hwaddr[0] = 0x00;
  netif->hwaddr[1] = 0x23;
  netif->hwaddr[2] = 0xC1;
  netif->hwaddr[3] = 0xDE;
  netif->hwaddr[4] = 0xD0;
  netif->hwaddr[5] = 0x0D;

#if LWIP_IPV6
  netif->output_ip6 = ethip6_output;
  netif->ip6_autoconfig_enabled = 1;
  netif_create_ip6_linklocal_address(netif, 1);
  netif->flags |= NETIF_FLAG_MLD6;
#endif

  return ERR_OK;
}

static void input_pkt(struct netif *netif, const u8_t *data, size_t len)
{
  struct pbuf *p, *q;
  err_t err;

  LWIP_ASSERT("pkt too big", len <= 0xFFFF);
  p = pbuf_alloc(PBUF_RAW, (u16_t)len, PBUF_POOL);
  LWIP_ASSERT("alloc failed", p);
  for(q = p; q != NULL; q = q->next) {
    MEMCPY(q->payload, data, q->len);
    data += q->len;
  }
  err = netif->input(p, netif);
  if (err != ERR_OK) {
    pbuf_free(p);
  }
}

static void input_pkts(struct netif *netif, const u8_t *data, size_t len)
{
#ifdef LWIP_FUZZ_MULTI_PACKET
  const u16_t max_packet_size = 1514;
  const u8_t *ptr = data;
  size_t rem_len = len;

  while (rem_len > sizeof(u16_t)) {
    u16_t frame_len;
    memcpy(&frame_len, ptr, sizeof(u16_t));
    ptr += sizeof(u16_t);
    rem_len -= sizeof(u16_t);
    frame_len = htons(frame_len) & 0x7FF;
    frame_len = LWIP_MIN(frame_len, max_packet_size);
    if (frame_len > rem_len) {
      frame_len = (u16_t)rem_len;
    }
    if (frame_len != 0) {
      input_pkt(netif, ptr, frame_len);
    }
    ptr += frame_len;
    rem_len -= frame_len;
  }
#else /* LWIP_FUZZ_MULTI_PACKET */
  input_pkt(netif, data, len);
#endif /* LWIP_FUZZ_MULTI_PACKET */
}

int main(int argc, char** argv)
{
  struct netif net_test;
  ip4_addr_t addr;
  ip4_addr_t netmask;
  ip4_addr_t gw;
  size_t len;

  lwip_init();

  IP4_ADDR(&addr, 172, 30, 115, 84);
  IP4_ADDR(&netmask, 255, 255, 255, 0);
  IP4_ADDR(&gw, 172, 30, 115, 1);

  netif_add(&net_test, &addr, &netmask, &gw, &net_test, testif_init, ethernet_input);
  netif_set_up(&net_test);
  netif_set_link_up(&net_test);

#if LWIP_IPV6
  nd6_tmr(); /* tick nd to join multicast groups */
#endif
  dns_setserver(0, &net_test.gw);

  /* initialize apps */
  httpd_init();
  lwiperf_start_tcp_server_default(NULL, NULL);
  mdns_resp_init();
  mdns_resp_add_netif(&net_test, "hostname", 255);
  snmp_init();

  if(argc > 1) {
    FILE* f;
    const char* filename;
    printf("reading input from file... ");
    fflush(stdout);
    filename = argv[1];
    LWIP_ASSERT("invalid filename", filename != NULL);
    f = fopen(filename, "rb");
    LWIP_ASSERT("open failed", f != NULL);
    len = fread(pktbuf, 1, sizeof(pktbuf), f);
    fclose(f);
    printf("testing file: \"%s\"...\r\n", filename);
  } else {
    len = fread(pktbuf, 1, sizeof(pktbuf), stdin);
  }
  input_pkts(&net_test, pktbuf, len);

  return 0;
}
