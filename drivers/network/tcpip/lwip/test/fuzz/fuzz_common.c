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
 *         Simon Goldschmidt <goldsimon@gmx.de>
 *
 */

#include "fuzz_common.h"

#include "lwip/altcp_tcp.h"
#include "lwip/dns.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "lwip/udp.h"
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

static u8_t pktbuf[200000];
static const u8_t *remfuzz_ptr; /* remaining fuzz pointer */
static size_t     remfuzz_len;  /* remaining fuzz length  */

#ifndef FUZZ_DEBUG
#define FUZZ_DEBUG LWIP_DBG_OFF
#endif

#ifdef LWIP_FUZZ_SYS_NOW
/* This offset should be added to the time 'sys_now()' returns */
u32_t sys_now_offset;
#endif

/** Set this to 1 and define FUZZ_DUMP_PCAP_FILE to dump tx and rx packets into
 * a pcap file. At the same time, packet info is written via LWIP_DEBUGF so
 * packets can be matched to other events for debugging them.
 */
#ifndef FUZZ_DUMP_PCAP
#define FUZZ_DUMP_PCAP 0
#endif

#if FUZZ_DUMP_PCAP
const u8_t pcap_file_header[24] = {
  0xd4, 0xc3, 0xb2, 0xa1, 0x02, 0x00, 0x04, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00
};

static FILE* fpcap;
static u32_t pcap_packet;

static void pcap_dump_init(void)
{
  fpcap = fopen(FUZZ_DUMP_PCAP_FILE, "wb");
  if (fpcap != NULL) {
    /* write header */
    fwrite(pcap_file_header, 1, sizeof(pcap_file_header), fpcap);
  }
}

/* This function might have to be called from LWIP_PLATFORM_ASSERT()
 * in order to produce correct pcap results on crash.
 * Define this global so that for a test, we can call this from anywhere...
 */
void pcap_dump_stop(void);
void pcap_dump_stop(void)
{
  if (fpcap != NULL) {
    fclose(fpcap);
    fpcap = NULL;
  }
}

static void pcap_dump_packet(struct pbuf *p, int is_tx)
{
  if (fpcap != NULL) {
    struct pbuf *q;
    u32_t data;
    pcap_packet++;
    if (is_tx) {
      LWIP_DEBUGF(FUZZ_DEBUG, ("> %d fuzz: netif: send %u bytes\n", pcap_packet, p->tot_len));
    } else {
      LWIP_DEBUGF(FUZZ_DEBUG, ("< %d fuzz: RX packet of %u bytes\n", pcap_packet, p->tot_len));
      if (pcap_packet == 50 || pcap_packet == 33 || pcap_packet == 29) {
        pcap_packet++;
        pcap_packet--;
      }
    }
    /* write packet header */
    fwrite(&pcap_packet, 1, sizeof(pcap_packet), fpcap);
    data = 0;
    fwrite(&data, 1, sizeof(data), fpcap);
    data = p->tot_len;
    fwrite(&data, 1, sizeof(data), fpcap);
    fwrite(&data, 1, sizeof(data), fpcap);
    /* write packet data */
    for(q = p; q != NULL; q = q->next) {
      fwrite(q->payload, 1, q->len, fpcap);
    }
  }
}

static void pcap_dump_rx_packet(struct pbuf *p)
{
  pcap_dump_packet(p, 0);
}

static void pcap_dump_tx_packet(struct pbuf *p)
{
  pcap_dump_packet(p, 1);
}
#else /* FUZZ_DUMP_PCAP */
#define pcap_dump_rx_packet(p)
#define pcap_dump_tx_packet(p)
#define pcap_dump_init()
#define pcap_dump_stop()
#endif /* FUZZ_DUMP_PCAP */

/* no-op send function */
static err_t lwip_tx_func(struct netif *netif, struct pbuf *p)
{
  pcap_dump_tx_packet(p);
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
  netif_create_ip6_linklocal_address(netif, 1);
  netif->flags |= NETIF_FLAG_MLD6;
#endif

  return ERR_OK;
}

static void input_pkt(struct netif *netif, const u8_t *data, size_t len)
{
  struct pbuf *p, *q;
  err_t err;

  if (len > 0xFFFF) {
    printf("pkt too big (%#zX bytes)\n", len);
    return;
  }

  p = pbuf_alloc(PBUF_RAW, (u16_t)len, PBUF_POOL);
  LWIP_ASSERT("alloc failed", p);
  for(q = p; q != NULL; q = q->next) {
    MEMCPY(q->payload, data, q->len);
    data += q->len;
  }
  remfuzz_ptr += len;
  remfuzz_len -= len;
  pcap_dump_rx_packet(p);
  err = netif->input(p, netif);
  if (err != ERR_OK) {
    pbuf_free(p);
  }
}

static void input_pkts(enum lwip_fuzz_type type, struct netif *netif, const u8_t *data, size_t len)
{
  remfuzz_ptr = data;
  remfuzz_len = len;

  if (type == LWIP_FUZZ_SINGLE) {
    input_pkt(netif, data, len);
  } else {
    const u16_t max_packet_size = 1514;
    const size_t minlen = sizeof(u16_t) + (type == LWIP_FUZZ_MULTIPACKET_TIME ? sizeof(u32_t) : 0);

    while (remfuzz_len > minlen) {
      u16_t frame_len;
#ifdef LWIP_FUZZ_SYS_NOW
      u32_t external_delay = 0;
#endif
      if (type == LWIP_FUZZ_MULTIPACKET_TIME) {
#ifdef LWIP_FUZZ_SYS_NOW
        /* Extract external delay time from fuzz pool */
        memcpy(&external_delay, remfuzz_ptr, sizeof(u32_t));
        external_delay = ntohl(external_delay);
#endif
        remfuzz_ptr += sizeof(u32_t);
        remfuzz_len -= sizeof(u32_t);
      }
      memcpy(&frame_len, remfuzz_ptr, sizeof(u16_t));
      remfuzz_ptr += sizeof(u16_t);
      remfuzz_len -= sizeof(u16_t);
      frame_len = ntohs(frame_len) & 0x7FF;
      frame_len = LWIP_MIN(frame_len, max_packet_size);
      if (frame_len > remfuzz_len) {
        frame_len = (u16_t)remfuzz_len;
      }
      if (frame_len != 0) {
        if (type == LWIP_FUZZ_MULTIPACKET_TIME) {
#ifdef LWIP_FUZZ_SYS_NOW
          /* Update total external delay time, and check timeouts */
          sys_now_offset += external_delay;
          LWIP_DEBUGF(FUZZ_DEBUG, ("fuzz: sys_now_offset += %u -> %u\n", external_delay, sys_now_offset));
#endif
          sys_check_timeouts();
        }
        input_pkt(netif, remfuzz_ptr, frame_len);
        /* Check timeouts again */
        sys_check_timeouts();
      }
    }
  }
}

#if LWIP_TCP
static struct altcp_pcb *tcp_client_pcb;  /* a pcb for the TCP client */
static struct altcp_pcb *tcp_server_pcb;  /* a pcb for the TCP server */
static u16_t            tcp_remote_port;  /* a TCP port number of the destionation */
static u16_t            tcp_local_port;   /* a TCP port number of the local server */

/**
 * tcp_app_fuzz_input
 * Input fuzz with a write function for TCP.
 */
static void
tcp_app_fuzz_input(struct altcp_pcb *pcb)
{
  if (remfuzz_len > sizeof(u16_t)) {
    /*
     * (max IP packet size) - ((minimum IP header size) + (minimum TCP header size))
     * = 65535 - (20 + 20)
     * = 65495
     */
    const u16_t max_data_size = 65495;
    u16_t data_len;

    memcpy(&data_len, remfuzz_ptr, sizeof(u16_t));
    remfuzz_ptr += sizeof(u16_t);
    remfuzz_len -= sizeof(u16_t);
    data_len = ntohs(data_len);
    data_len = LWIP_MIN(data_len, max_data_size);
    if (data_len > remfuzz_len) {
      data_len = (u16_t)remfuzz_len;
    }

    if (data_len != 0) {
      LWIP_DEBUGF(FUZZ_DEBUG, ("fuzz: tcp: write %u bytes\n", data_len));
      altcp_write(pcb, remfuzz_ptr, data_len, TCP_WRITE_FLAG_COPY);
      altcp_output(pcb);
    } else {
      LWIP_DEBUGF(FUZZ_DEBUG, ("fuzz: tcp: close\n"));
      altcp_close(pcb);
    }

    remfuzz_ptr += data_len;
    remfuzz_len -= data_len;
  }
}

/**
 * tcp_client_connected
 * A connected callback function (for the TCP client)
 */
static err_t
tcp_client_connected(void *arg, struct altcp_pcb *pcb, err_t err)
{
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(err);

  LWIP_DEBUGF(FUZZ_DEBUG, ("fuzz: tcp: tcp_client_connected\n"));
  tcp_app_fuzz_input(pcb);

  return ERR_OK;
}

/**
 * tcp_client_recv
 * A recv callback function (for the TCP client)
 */
static err_t
tcp_client_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err)
{
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(err);

  if (p == NULL) {
    altcp_close(pcb);
  } else {
    altcp_recved(pcb, p->tot_len);
    LWIP_DEBUGF(FUZZ_DEBUG, ("fuzz: tcp: tcp_client_recv: %d\n", p->tot_len));
    tcp_app_fuzz_input(pcb);
    pbuf_free(p);
  }

  return ERR_OK;
}

/**
 * tcp_client_sent
 * A sent callback function (for the TCP client)
 */
static err_t
tcp_client_sent(void *arg, struct altcp_pcb *pcb, u16_t len)
{
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(len);
  return ERR_OK;
}

/**
 * tcp_client_poll
 * A poll callback function (for the TCP client)
 */
static err_t
tcp_client_poll(void *arg, struct altcp_pcb *pcb)
{
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(pcb);
  return ERR_OK;
}

/**
 * tcp_client_err
 * An err callback function (for the TCP client)
 */
static void
tcp_client_err(void *arg, err_t err)
{
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(err);
}

/**
 * tcp_server_recv
 * A recv callback function (for the TCP server)
 */
static err_t
tcp_server_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err)
{
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(err);

  if (p == NULL) {
    altcp_close(pcb);
  } else {
    altcp_recved(pcb, p->tot_len);
    LWIP_DEBUGF(FUZZ_DEBUG, ("fuzz: tcp: tcp_server_recv: %d\n", p->tot_len));
    tcp_app_fuzz_input(pcb);
    pbuf_free(p);
  }

  return ERR_OK;
}

/**
 * tcp_server_sent
 * A sent callback function (for the TCP server)
 */
static err_t
tcp_server_sent(void *arg, struct altcp_pcb *pcb, u16_t len)
{
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(len);
  return ERR_OK;
}

/**
 * tcp_server_poll
 * A poll callback function (for the TCP server)
 */
static err_t
tcp_server_poll(void *arg, struct altcp_pcb *pcb)
{
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(pcb);
  return ERR_OK;
}

/**
 * tcp_server_err
 * An err callbuck function (for the TCP server)
 */
static void
tcp_server_err(void *arg, err_t err)
{
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(err);
}

/**
 * tcp_server_accept
 * An accept callbuck function (for the TCP server)
 */
static err_t
tcp_server_accept(void *arg, struct altcp_pcb *pcb, err_t err)
{
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(err);

  if ((err != ERR_OK) || (pcb == NULL)) {
    return ERR_VAL;
  }
  LWIP_DEBUGF(FUZZ_DEBUG, ("fuzz: accept from remote\n"));

  altcp_setprio(pcb, TCP_PRIO_MIN);

  altcp_recv(pcb, tcp_server_recv);
  altcp_err(pcb, tcp_server_err);
  altcp_poll(pcb, tcp_server_poll, 10);
  altcp_sent(pcb, tcp_server_sent);

  return ERR_OK;
}
#endif /* LWIP_TCP */

#if LWIP_UDP
static struct udp_pcb   *udp_client_pcb;  /* a pcb for the UDP client */
static struct udp_pcb   *udp_server_pcb;  /* a pcb for the UDP server */
static u16_t            udp_remote_port;  /* a UDP port number of the destination */
static u16_t            udp_local_port;   /* a UDP port number of the local server*/

/**
 * udp_app_fuzz_input
 * Input fuzz with write functions for UDP.
 */
static void
udp_app_fuzz_input(struct udp_pcb *pcb, const ip_addr_t *addr, u16_t port)
{
  if (remfuzz_len > sizeof(u16_t)) {
    /*
     * (max IP packet size) - ((minimum IP header size) - (minimum UDP header size))
     * = 65535 - (20 + 8)
     * = 65507
     */
    const u16_t max_data_size = 65507;
    u16_t data_len;

    memcpy(&data_len, remfuzz_ptr, sizeof(u16_t));
    remfuzz_ptr += sizeof(u16_t);
    remfuzz_len -= sizeof(u16_t);
    data_len = ntohs(data_len);
    data_len = LWIP_MIN(data_len, max_data_size);
    if (data_len > remfuzz_len) {
      data_len = (u16_t)remfuzz_len;
    }

    LWIP_DEBUGF(FUZZ_DEBUG, ("fuzz: udp: send %u bytes\n", data_len));
    if (data_len != 0) {
      struct pbuf *p, *q;

      p = pbuf_alloc(PBUF_RAW, (u16_t)data_len, PBUF_POOL);
      LWIP_ASSERT("alloc failed", p);

      for (q = p; q != NULL; q = q->next) {
        MEMCPY(q->payload, remfuzz_ptr, q->len);
        remfuzz_ptr += q->len;
      }
      remfuzz_len -= data_len;

      /*
       * Trying input from ...
       *
       * client:
       *     The pcb has information about the destination.
       *     We use udp_send().
       *
       * server:
       *     The pcb does NOT have infomation about the destionation.
       *     We use udp_sendto().
       */
      if (addr == NULL) {
        udp_send(pcb, p);
      } else {
        udp_sendto(pcb, p, addr, port);
      }
      pbuf_free(p);
    }
  }
}

/**
 * udp_client_recv
 * A recv callback function (for the UDP client)
 */
static void
udp_client_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(p);
  LWIP_UNUSED_ARG(addr);
  LWIP_UNUSED_ARG(port);

  if (p == NULL) {
    udp_disconnect(pcb);
  } else {
    /*
     * We call the function with 2nd argument set to NULL
     * to input fuzz from udp_send.
     */
    udp_app_fuzz_input(pcb, NULL, port);
    pbuf_free(p);
  }
}

/**
 * udp_server_recv
 * A recv callback functyion (for the UDP server)
 */
static void
udp_server_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(p);
  LWIP_UNUSED_ARG(addr);
  LWIP_UNUSED_ARG(port);

  if (p != NULL) {
    udp_app_fuzz_input(pcb, addr, port);
    pbuf_free(p);
  }
}
#endif /* LWIP_UDP */

int lwip_fuzztest(int argc, char** argv, enum lwip_fuzz_type type, u32_t test_apps)
{
  struct netif net_test;
  ip4_addr_t addr;
  ip4_addr_t netmask;
  ip4_addr_t gw;
  size_t len;
  err_t err;
  ip_addr_t remote_addr;      /* a IPv4 addr of the destination */
  struct eth_addr remote_mac = ETH_ADDR(0x28, 0x00, 0x00, 0x22, 0x2b, 0x38); /* a MAC addr of the destination */

  pcap_dump_init();
  lwip_init();

  IP4_ADDR(&addr, 172, 30, 115, 84);
  IP4_ADDR(&netmask, 255, 255, 255, 0);
  IP4_ADDR(&gw, 172, 30, 115, 1);

  netif_add(&net_test, &addr, &netmask, &gw, &net_test, testif_init, ethernet_input);
  netif_set_up(&net_test);
  netif_set_link_up(&net_test);

  if (test_apps & LWIP_FUZZ_STATICARP) {
    /* Add the ARP entry */
    IP_ADDR4(&remote_addr, 172, 30, 115, 37);
    etharp_add_static_entry(&(remote_addr.u_addr.ip4), &remote_mac);
  }

#if LWIP_IPV6
  nd6_tmr(); /* tick nd to join multicast groups */
#endif
  dns_setserver(0, &net_test.gw);

  if (test_apps & LWIP_FUZZ_DEFAULT) {
    /* initialize apps */
    httpd_init();
    lwiperf_start_tcp_server_default(NULL, NULL);
    mdns_resp_init();
    mdns_resp_add_netif(&net_test, "hostname");
    snmp_init();
  }
  if (test_apps & LWIP_FUZZ_TCP_CLIENT) {
    tcp_client_pcb = altcp_tcp_new_ip_type(IPADDR_TYPE_ANY);
    LWIP_ASSERT("Error: altcp_new() failed", tcp_client_pcb != NULL);
    tcp_remote_port = 80;
    err = altcp_connect(tcp_client_pcb, &remote_addr, tcp_remote_port, tcp_client_connected);
    LWIP_ASSERT("Error: altcp_connect() failed", err == ERR_OK);
    altcp_recv(tcp_client_pcb, tcp_client_recv);
    altcp_err(tcp_client_pcb, tcp_client_err);
    altcp_poll(tcp_client_pcb, tcp_client_poll, 10);
    altcp_sent(tcp_client_pcb, tcp_client_sent);
  }
  if (test_apps & LWIP_FUZZ_TCP_SERVER) {
    tcp_server_pcb = altcp_tcp_new_ip_type(IPADDR_TYPE_ANY);
    LWIP_ASSERT("Error: altcp_new() failed", tcp_server_pcb != NULL);
    altcp_setprio(tcp_server_pcb, TCP_PRIO_MIN);
    tcp_local_port = 80;
    err = altcp_bind(tcp_server_pcb, IP_ANY_TYPE, tcp_local_port);
    LWIP_ASSERT("Error: altcp_bind() failed", err == ERR_OK);
    tcp_server_pcb = altcp_listen(tcp_server_pcb);
    LWIP_ASSERT("Error: altcp_listen() failed", err == ERR_OK);
    altcp_accept(tcp_server_pcb, tcp_server_accept);
  }
  if (test_apps & LWIP_FUZZ_UDP_CLIENT) {
    udp_client_pcb = udp_new();
    udp_new_ip_type(IPADDR_TYPE_ANY);
    udp_recv(udp_client_pcb, udp_client_recv, NULL);
    udp_remote_port = 161;
    udp_connect(udp_client_pcb, &remote_addr, udp_remote_port);
  }
  if (test_apps & LWIP_FUZZ_UDP_SERVER) {
    udp_server_pcb = udp_new();
    udp_new_ip_type(IPADDR_TYPE_ANY);
    udp_local_port = 161;
    udp_bind(udp_server_pcb, IP_ANY_TYPE, udp_local_port);
    udp_recv(udp_server_pcb, udp_server_recv, NULL);
  }

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
  input_pkts(type, &net_test, pktbuf, len);

  pcap_dump_stop();
  return 0;
}

#ifdef LWIP_RAND_FOR_FUZZ
u32_t lwip_fuzz_rand(void)
{
#ifdef LWIP_RAND_FOR_FUZZ_SIMULATE_GLIBC
  /* this is what glibc rand() returns (first 20 numbers) */
  static u32_t rand_nrs[] = {0x6b8b4567, 0x327b23c6, 0x643c9869, 0x66334873, 0x74b0dc51,
    0x19495cff, 0x2ae8944a, 0x625558ec, 0x238e1f29, 0x46e87ccd,
    0x3d1b58ba, 0x507ed7ab, 0x2eb141f2, 0x41b71efb, 0x79e2a9e3,
    0x7545e146, 0x515f007c, 0x5bd062c2, 0x12200854, 0x4db127f8};
  static unsigned idx = 0;
  u32_t ret = rand_nrs[idx];
  idx++;
  if (idx >= sizeof(rand_nrs)/sizeof((rand_nrs)[0])) {
    idx = 0;
  }
  return ret;
#else
  /* a simple LCG, unsafe but should give the same result for every execution (best for fuzzing) */
  u32_t result;
  static s32_t state[1] = {0xdeadbeef};
  uint64_t val = state[0] & 0xffffffff;
  val = ((val * 1103515245) + 12345) & 0x7fffffff;
  state[0] = val;
  result = val;
  return result;
#endif
}
#endif
