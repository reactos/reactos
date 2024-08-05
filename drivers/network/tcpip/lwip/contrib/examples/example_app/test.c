/*
 * Copyright (c) 2001,2002 Florian Schulze.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the authors nor the names of the contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * test.c - This file is part of lwIP test
 *
 */

/* C runtime includes */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

/* lwIP core includes */
#include "lwip/opt.h"

#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "lwip/debug.h"
#include "lwip/stats.h"
#include "lwip/init.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/api.h"

#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/dns.h"
#include "lwip/dhcp.h"
#include "lwip/autoip.h"

/* lwIP netif includes */
#include "lwip/etharp.h"
#include "netif/ethernet.h"

/* applications includes */
#include "lwip/apps/netbiosns.h"
#include "lwip/apps/httpd.h"
#include "apps/httpserver/httpserver-netconn.h"
#include "apps/netio/netio.h"
#include "apps/ping/ping.h"
#include "apps/rtp/rtp.h"
#include "apps/chargen/chargen.h"
#include "apps/shell/shell.h"
#include "apps/tcpecho/tcpecho.h"
#include "apps/udpecho/udpecho.h"
#include "apps/tcpecho_raw/tcpecho_raw.h"
#include "apps/socket_examples/socket_examples.h"

#include "examples/lwiperf/lwiperf_example.h"
#include "examples/mdns/mdns_example.h"
#include "examples/snmp/snmp_example.h"
#include "examples/tftp/tftp_example.h"
#include "examples/sntp/sntp_example.h"
#include "examples/mqtt/mqtt_example.h"

#include "examples/httpd/cgi_example/cgi_example.h"
#include "examples/httpd/fs_example/fs_example.h"
#include "examples/httpd/https_example/https_example.h"
#include "examples/httpd/ssi_example/ssi_example.h"

#include "default_netif.h"

#if NO_SYS
/* ... then we need information about the timer intervals: */
#include "lwip/ip4_frag.h"
#include "lwip/igmp.h"
#endif /* NO_SYS */

#include "netif/ppp/ppp_opts.h"
#if PPP_SUPPORT
/* PPP includes */
#include "lwip/sio.h"
#include "netif/ppp/pppapi.h"
#include "netif/ppp/pppos.h"
#include "netif/ppp/pppoe.h"
#if !NO_SYS && !LWIP_PPP_API
#error With NO_SYS==0, LWIP_PPP_API==1 is required.
#endif
#endif /* PPP_SUPPORT */

/* include the port-dependent configuration */
#include "lwipcfg.h"

#ifndef LWIP_EXAMPLE_APP_ABORT
#define LWIP_EXAMPLE_APP_ABORT() 0
#endif

/** Define this to 1 to enable a port-specific ethernet interface as default interface. */
#ifndef USE_DEFAULT_ETH_NETIF
#define USE_DEFAULT_ETH_NETIF 1
#endif

/** Define this to 1 to enable a PPP interface. */
#ifndef USE_PPP
#define USE_PPP 0
#endif

/** Define this to 1 or 2 to support 1 or 2 SLIP interfaces. */
#ifndef USE_SLIPIF
#define USE_SLIPIF 0
#endif

/** Use an ethernet adapter? Default to enabled if port-specific ethernet netif or PPPoE are used. */
#ifndef USE_ETHERNET
#define USE_ETHERNET  (USE_DEFAULT_ETH_NETIF || PPPOE_SUPPORT)
#endif

/** Use an ethernet adapter for TCP/IP? By default only if port-specific ethernet netif is used. */
#ifndef USE_ETHERNET_TCPIP
#define USE_ETHERNET_TCPIP  (USE_DEFAULT_ETH_NETIF)
#endif

#if USE_SLIPIF
#include <netif/slipif.h>
#endif /* USE_SLIPIF */

#ifndef USE_DHCP
#define USE_DHCP    LWIP_DHCP
#endif
#ifndef USE_AUTOIP
#define USE_AUTOIP  LWIP_AUTOIP
#endif

/* global variables for netifs */
#if USE_ETHERNET
#if LWIP_DHCP
/* dhcp struct for the ethernet netif */
static struct dhcp netif_dhcp;
#endif /* LWIP_DHCP */
#if LWIP_AUTOIP
/* autoip struct for the ethernet netif */
static struct autoip netif_autoip;
#endif /* LWIP_AUTOIP */
#endif /* USE_ETHERNET */
#if USE_PPP
/* THE PPP PCB */
static ppp_pcb *ppp;
/* THE PPP interface */
static struct netif ppp_netif;
/* THE PPP descriptor */
static u8_t sio_idx = 0;
static sio_fd_t ppp_sio;
#endif /* USE_PPP */
#if USE_SLIPIF
static struct netif slipif1;
#if USE_SLIPIF > 1
static struct netif slipif2;
#endif /* USE_SLIPIF > 1 */
#endif /* USE_SLIPIF */


#if USE_PPP
static void
pppLinkStatusCallback(ppp_pcb *pcb, int errCode, void *ctx)
{
  struct netif *pppif = ppp_netif(pcb);
  LWIP_UNUSED_ARG(ctx);

  switch(errCode) {
    case PPPERR_NONE: {             /* No error. */
      printf("pppLinkStatusCallback: PPPERR_NONE\n");
#if LWIP_IPV4
      printf("   our_ipaddr  = %s\n", ip4addr_ntoa(netif_ip4_addr(pppif)));
      printf("   his_ipaddr  = %s\n", ip4addr_ntoa(netif_ip4_gw(pppif)));
      printf("   netmask     = %s\n", ip4addr_ntoa(netif_ip4_netmask(pppif)));
#endif /* LWIP_IPV4 */
#if LWIP_DNS
      printf("   dns1        = %s\n", ipaddr_ntoa(dns_getserver(0)));
      printf("   dns2        = %s\n", ipaddr_ntoa(dns_getserver(1)));
#endif /* LWIP_DNS */
#if PPP_IPV6_SUPPORT
      printf("   our6_ipaddr = %s\n", ip6addr_ntoa(netif_ip6_addr(pppif, 0)));
#endif /* PPP_IPV6_SUPPORT */
      break;
    }
    case PPPERR_PARAM: {           /* Invalid parameter. */
      printf("pppLinkStatusCallback: PPPERR_PARAM\n");
      break;
    }
    case PPPERR_OPEN: {            /* Unable to open PPP session. */
      printf("pppLinkStatusCallback: PPPERR_OPEN\n");
      break;
    }
    case PPPERR_DEVICE: {          /* Invalid I/O device for PPP. */
      printf("pppLinkStatusCallback: PPPERR_DEVICE\n");
      break;
    }
    case PPPERR_ALLOC: {           /* Unable to allocate resources. */
      printf("pppLinkStatusCallback: PPPERR_ALLOC\n");
      break;
    }
    case PPPERR_USER: {            /* User interrupt. */
      printf("pppLinkStatusCallback: PPPERR_USER\n");
      break;
    }
    case PPPERR_CONNECT: {         /* Connection lost. */
      printf("pppLinkStatusCallback: PPPERR_CONNECT\n");
      break;
    }
    case PPPERR_AUTHFAIL: {        /* Failed authentication challenge. */
      printf("pppLinkStatusCallback: PPPERR_AUTHFAIL\n");
      break;
    }
    case PPPERR_PROTOCOL: {        /* Failed to meet protocol. */
      printf("pppLinkStatusCallback: PPPERR_PROTOCOL\n");
      break;
    }
    case PPPERR_PEERDEAD: {        /* Connection timeout */
      printf("pppLinkStatusCallback: PPPERR_PEERDEAD\n");
      break;
    }
    case PPPERR_IDLETIMEOUT: {     /* Idle Timeout */
      printf("pppLinkStatusCallback: PPPERR_IDLETIMEOUT\n");
      break;
    }
    case PPPERR_CONNECTTIME: {     /* Max connect time reached */
      printf("pppLinkStatusCallback: PPPERR_CONNECTTIME\n");
      break;
    }
    case PPPERR_LOOPBACK: {        /* Loopback detected */
      printf("pppLinkStatusCallback: PPPERR_LOOPBACK\n");
      break;
    }
    default: {
      printf("pppLinkStatusCallback: unknown errCode %d\n", errCode);
      break;
    }
  }
}

#if PPPOS_SUPPORT
static u32_t
ppp_output_cb(ppp_pcb *pcb, const void *data, u32_t len, void *ctx)
{
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(ctx);
  return sio_write(ppp_sio, (const u8_t*)data, len);
}
#endif /* PPPOS_SUPPORT */
#endif /* USE_PPP */

#if LWIP_NETIF_STATUS_CALLBACK
static void
status_callback(struct netif *state_netif)
{
  if (netif_is_up(state_netif)) {
#if LWIP_IPV4
    printf("status_callback==UP, local interface IP is %s\n", ip4addr_ntoa(netif_ip4_addr(state_netif)));
#else
    printf("status_callback==UP\n");
#endif
  } else {
    printf("status_callback==DOWN\n");
  }
}
#endif /* LWIP_NETIF_STATUS_CALLBACK */

#if LWIP_NETIF_LINK_CALLBACK
static void
link_callback(struct netif *state_netif)
{
  if (netif_is_link_up(state_netif)) {
    printf("link_callback==UP\n");
  } else {
    printf("link_callback==DOWN\n");
  }
}
#endif /* LWIP_NETIF_LINK_CALLBACK */

/* This function initializes all network interfaces */
static void
test_netif_init(void)
{
#if LWIP_IPV4 && USE_ETHERNET
  ip4_addr_t ipaddr, netmask, gw;
#endif /* LWIP_IPV4 && USE_ETHERNET */
#if USE_SLIPIF
  u8_t num_slip1 = 0;
#if LWIP_IPV4
  ip4_addr_t ipaddr_slip1, netmask_slip1, gw_slip1;
#endif
#if USE_SLIPIF > 1
  u8_t num_slip2 = 1;
#if LWIP_IPV4
  ip4_addr_t ipaddr_slip2, netmask_slip2, gw_slip2;
#endif
#endif /* USE_SLIPIF > 1 */
#endif /* USE_SLIPIF */
#if USE_DHCP || USE_AUTOIP
  err_t err;
#endif

#if USE_PPP
  const char *username = NULL, *password = NULL;
#ifdef PPP_USERNAME
  username = PPP_USERNAME;
#endif
#ifdef PPP_PASSWORD
  password = PPP_PASSWORD;
#endif
  printf("ppp_connect: COM%d\n", (int)sio_idx);
#if PPPOS_SUPPORT
  ppp_sio = sio_open(sio_idx);
  if (ppp_sio == NULL) {
    printf("sio_open error\n");
  } else {
    ppp = pppos_create(&ppp_netif, ppp_output_cb, pppLinkStatusCallback, NULL);
    if (ppp == NULL) {
      printf("pppos_create error\n");
    } else {
      ppp_set_auth(ppp, PPPAUTHTYPE_ANY, username, password);
      ppp_connect(ppp, 0);
    }
  }
#endif /* PPPOS_SUPPORT */
#endif  /* USE_PPP */

#if USE_ETHERNET
#if LWIP_IPV4
  ip4_addr_set_zero(&gw);
  ip4_addr_set_zero(&ipaddr);
  ip4_addr_set_zero(&netmask);
#if USE_ETHERNET_TCPIP
#if USE_DHCP
  printf("Starting lwIP, local interface IP is dhcp-enabled\n");
#elif USE_AUTOIP
  printf("Starting lwIP, local interface IP is autoip-enabled\n");
#else /* USE_DHCP */
  LWIP_PORT_INIT_GW(&gw);
  LWIP_PORT_INIT_IPADDR(&ipaddr);
  LWIP_PORT_INIT_NETMASK(&netmask);
  printf("Starting lwIP, local interface IP is %s\n", ip4addr_ntoa(&ipaddr));
#endif /* USE_DHCP */
#endif /* USE_ETHERNET_TCPIP */
#else /* LWIP_IPV4 */
  printf("Starting lwIP, IPv4 disable\n");
#endif /* LWIP_IPV4 */

#if LWIP_IPV4
  init_default_netif(&ipaddr, &netmask, &gw);
#else
  init_default_netif();
#endif
#if LWIP_IPV6
  netif_create_ip6_linklocal_address(netif_default, 1);
  printf("ip6 linklocal address: %s\n", ip6addr_ntoa(netif_ip6_addr(netif_default, 0)));
#endif /* LWIP_IPV6 */
#if LWIP_NETIF_STATUS_CALLBACK
  netif_set_status_callback(netif_default, status_callback);
#endif /* LWIP_NETIF_STATUS_CALLBACK */
#if LWIP_NETIF_LINK_CALLBACK
  netif_set_link_callback(netif_default, link_callback);
#endif /* LWIP_NETIF_LINK_CALLBACK */

#if USE_ETHERNET_TCPIP
#if LWIP_AUTOIP
  autoip_set_struct(netif_default, &netif_autoip);
#endif /* LWIP_AUTOIP */
#if LWIP_DHCP
  dhcp_set_struct(netif_default, &netif_dhcp);
#endif /* LWIP_DHCP */
  netif_set_up(netif_default);
#if USE_DHCP
  err = dhcp_start(netif_default);
  LWIP_ASSERT("dhcp_start failed", err == ERR_OK);
#elif USE_AUTOIP
  err = autoip_start(netif_default);
  LWIP_ASSERT("autoip_start failed", err == ERR_OK);
#endif /* USE_DHCP */
#else /* USE_ETHERNET_TCPIP */
  /* Use ethernet for PPPoE only */
  netif.flags &= ~(NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP); /* no ARP */
  netif.flags |= NETIF_FLAG_ETHERNET; /* but pure ethernet */
#endif /* USE_ETHERNET_TCPIP */

#if USE_PPP && PPPOE_SUPPORT
  /* start PPPoE after ethernet netif is added! */
  ppp = pppoe_create(&ppp_netif, netif_default, NULL, NULL, pppLinkStatusCallback, NULL);
  if (ppp == NULL) {
    printf("pppoe_create error\n");
  } else {
    ppp_set_auth(ppp, PPPAUTHTYPE_ANY, username, password);
    ppp_connect(ppp, 0);
  }
#endif /* USE_PPP && PPPOE_SUPPORT */

#endif /* USE_ETHERNET */
#if USE_SLIPIF
#if LWIP_IPV4
#define SLIP1_ADDRS &ipaddr_slip1, &netmask_slip1, &gw_slip1,
  LWIP_PORT_INIT_SLIP1_IPADDR(&ipaddr_slip1);
  LWIP_PORT_INIT_SLIP1_GW(&gw_slip1);
  LWIP_PORT_INIT_SLIP1_NETMASK(&netmask_slip1);
  printf("Starting lwIP slipif, local interface IP is %s\n", ip4addr_ntoa(&ipaddr_slip1));
#else
#define SLIP1_ADDRS
  printf("Starting lwIP slipif\n");
#endif
#if defined(SIO_USE_COMPORT) && SIO_USE_COMPORT
  num_slip1++; /* COM ports cannot be 0-based */
#endif
  netif_add(&slipif1, SLIP1_ADDRS &num_slip1, slipif_init, ip_input);
#if !USE_ETHERNET
  netif_set_default(&slipif1);
#endif /* !USE_ETHERNET */
#if LWIP_IPV6
  netif_create_ip6_linklocal_address(&slipif1, 1);
  printf("SLIP ip6 linklocal address: %s\n", ip6addr_ntoa(netif_ip6_addr(&slipif1, 0)));
#endif /* LWIP_IPV6 */
#if LWIP_NETIF_STATUS_CALLBACK
  netif_set_status_callback(&slipif1, status_callback);
#endif /* LWIP_NETIF_STATUS_CALLBACK */
#if LWIP_NETIF_LINK_CALLBACK
  netif_set_link_callback(&slipif1, link_callback);
#endif /* LWIP_NETIF_LINK_CALLBACK */
  netif_set_up(&slipif1);

#if USE_SLIPIF > 1
#if LWIP_IPV4
#define SLIP2_ADDRS &ipaddr_slip2, &netmask_slip2, &gw_slip2,
  LWIP_PORT_INIT_SLIP2_IPADDR(&ipaddr_slip2);
  LWIP_PORT_INIT_SLIP2_GW(&gw_slip2);
  LWIP_PORT_INIT_SLIP2_NETMASK(&netmask_slip2);
  printf("Starting lwIP SLIP if #2, local interface IP is %s\n", ip4addr_ntoa(&ipaddr_slip2));
#else
#define SLIP2_ADDRS
  printf("Starting lwIP SLIP if #2\n");
#endif
#if defined(SIO_USE_COMPORT) && SIO_USE_COMPORT
  num_slip2++; /* COM ports cannot be 0-based */
#endif
  netif_add(&slipif2, SLIP2_ADDRS &num_slip2, slipif_init, ip_input);
#if LWIP_IPV6
  netif_create_ip6_linklocal_address(&slipif1, 1);
  printf("SLIP2 ip6 linklocal address: ");
  ip6_addr_debug_print(0xFFFFFFFF & ~LWIP_DBG_HALT, netif_ip6_addr(&slipif2, 0));
  printf("\n");
#endif /* LWIP_IPV6 */
#if LWIP_NETIF_STATUS_CALLBACK
  netif_set_status_callback(&slipif2, status_callback);
#endif /* LWIP_NETIF_STATUS_CALLBACK */
#if LWIP_NETIF_LINK_CALLBACK
  netif_set_link_callback(&slipif2, link_callback);
#endif /* LWIP_NETIF_LINK_CALLBACK */
  netif_set_up(&slipif2);
#endif /* USE_SLIPIF > 1*/
#endif /* USE_SLIPIF */
}

#if LWIP_DNS_APP && LWIP_DNS
static void
dns_found(const char *name, const ip_addr_t *addr, void *arg)
{
  LWIP_UNUSED_ARG(arg);
  printf("%s: %s\n", name, addr ? ipaddr_ntoa(addr) : "<not found>");
}

static void
dns_dorequest(void *arg)
{
  const char* dnsname = "3com.com";
  ip_addr_t dnsresp;
  LWIP_UNUSED_ARG(arg);

  if (dns_gethostbyname(dnsname, &dnsresp, dns_found, NULL) == ERR_OK) {
    dns_found(dnsname, &dnsresp, NULL);
  }
}
#endif /* LWIP_DNS_APP && LWIP_DNS */

/* This function initializes applications */
static void
apps_init(void)
{
#if LWIP_DNS_APP && LWIP_DNS
  /* wait until the netif is up (for dhcp, autoip or ppp) */
  sys_timeout(5000, dns_dorequest, NULL);
#endif /* LWIP_DNS_APP && LWIP_DNS */

#if LWIP_CHARGEN_APP && LWIP_SOCKET
  chargen_init();
#endif /* LWIP_CHARGEN_APP && LWIP_SOCKET */

#if LWIP_PING_APP && LWIP_RAW && LWIP_ICMP
  ping_init(&netif_default->gw);
#endif /* LWIP_PING_APP && LWIP_RAW && LWIP_ICMP */

#if LWIP_NETBIOS_APP && LWIP_UDP
  netbiosns_init();
#ifndef NETBIOS_LWIP_NAME
#if LWIP_NETIF_HOSTNAME
  netbiosns_set_name(netif_default->hostname);
#else
  netbiosns_set_name("NETBIOSLWIPDEV");
#endif
#endif
#endif /* LWIP_NETBIOS_APP && LWIP_UDP */

#if LWIP_HTTPD_APP && LWIP_TCP
#ifdef LWIP_HTTPD_APP_NETCONN
  http_server_netconn_init();
#else /* LWIP_HTTPD_APP_NETCONN */
#if defined(LWIP_HTTPD_EXAMPLE_CUSTOMFILES) && LWIP_HTTPD_EXAMPLE_CUSTOMFILES && defined(LWIP_HTTPD_EXAMPLE_CUSTOMFILES_ROOTDIR)
  fs_ex_init(LWIP_HTTPD_EXAMPLE_CUSTOMFILES_ROOTDIR);
#endif
  httpd_init();
#if defined(LWIP_HTTPD_EXAMPLE_SSI_SIMPLE) && LWIP_HTTPD_EXAMPLE_SSI_SIMPLE
  ssi_ex_init();
#endif
#if defined(LWIP_HTTPD_EXAMPLE_CGI_SIMPLE) && LWIP_HTTPD_EXAMPLE_CGI_SIMPLE
  cgi_ex_init();
#endif
#if defined(LWIP_HTTPD_EXAMPLE_HTTPS) && LWIP_HTTPD_EXAMPLE_HTTPS
  https_ex_init();
#endif
#endif /* LWIP_HTTPD_APP_NETCONN */
#endif /* LWIP_HTTPD_APP && LWIP_TCP */

#if LWIP_NETIO_APP && LWIP_TCP
  netio_init();
#endif /* LWIP_NETIO_APP && LWIP_TCP */

#if LWIP_RTP_APP && LWIP_SOCKET && LWIP_IGMP
  rtp_init();
#endif /* LWIP_RTP_APP && LWIP_SOCKET && LWIP_IGMP */

#if LWIP_SHELL_APP && LWIP_NETCONN
  shell_init();
#endif /* LWIP_SHELL_APP && LWIP_NETCONN */
#if LWIP_TCPECHO_APP
#if LWIP_NETCONN && defined(LWIP_TCPECHO_APP_NETCONN)
  tcpecho_init();
#else /* LWIP_NETCONN && defined(LWIP_TCPECHO_APP_NETCONN) */
  tcpecho_raw_init();
#endif
#endif /* LWIP_TCPECHO_APP && LWIP_NETCONN */
#if LWIP_UDPECHO_APP && LWIP_NETCONN
  udpecho_init();
#endif /* LWIP_UDPECHO_APP && LWIP_NETCONN */
#if LWIP_SOCKET_EXAMPLES_APP && LWIP_SOCKET
  socket_examples_init();
#endif /* LWIP_SOCKET_EXAMPLES_APP && LWIP_SOCKET */
#if LWIP_MDNS_APP
  mdns_example_init();
#endif
#if LWIP_SNMP_APP
  snmp_example_init();
#endif
#if LWIP_SNTP_APP
  sntp_example_init();
#endif
#if LWIP_TFTP_APP
  tftp_example_init_server();
#endif
#if LWIP_TFTP_CLIENT_APP
  tftp_example_init_client();
#endif
#if LWIP_LWIPERF_APP
  lwiperf_example_init();
#endif
#if LWIP_MQTT_APP
  mqtt_example_init();
#endif

#ifdef LWIP_APP_INIT
  LWIP_APP_INIT();
#endif
}

/* This function initializes this lwIP test. When NO_SYS=1, this is done in
 * the main_loop context (there is no other one), when NO_SYS=0, this is done
 * in the tcpip_thread context */
static void
test_init(void * arg)
{ /* remove compiler warning */
#if NO_SYS
  LWIP_UNUSED_ARG(arg);
#else /* NO_SYS */
  sys_sem_t *init_sem;
  LWIP_ASSERT("arg != NULL", arg != NULL);
  init_sem = (sys_sem_t*)arg;
#endif /* NO_SYS */

  /* init randomizer again (seed per thread) */
  srand((unsigned int)time(NULL));

  /* init network interfaces */
  test_netif_init();

  /* init apps */
  apps_init();

#if !NO_SYS
  sys_sem_signal(init_sem);
#endif /* !NO_SYS */
}

/* This is somewhat different to other ports: we have a main loop here:
 * a dedicated task that waits for packets to arrive. This would normally be
 * done from interrupt context with embedded hardware, but we don't get an
 * interrupt in windows for that :-) */
static void
main_loop(void)
{
#if !NO_SYS
  err_t err;
  sys_sem_t init_sem;
#endif /* NO_SYS */
#if USE_PPP
#if !USE_ETHERNET
  int count;
  u8_t rxbuf[1024];
#endif
  volatile int callClosePpp = 0;
#endif /* USE_PPP */

  /* initialize lwIP stack, network interfaces and applications */
#if NO_SYS
  lwip_init();
  test_init(NULL);
#else /* NO_SYS */
  err = sys_sem_new(&init_sem, 0);
  LWIP_ASSERT("failed to create init_sem", err == ERR_OK);
  LWIP_UNUSED_ARG(err);
  tcpip_init(test_init, &init_sem);
  /* we have to wait for initialization to finish before
   * calling update_adapter()! */
  sys_sem_wait(&init_sem);
  sys_sem_free(&init_sem);
#endif /* NO_SYS */

#if (LWIP_SOCKET || LWIP_NETCONN) && LWIP_NETCONN_SEM_PER_THREAD
  netconn_thread_init();
#endif

  /* MAIN LOOP for driver update (and timers if NO_SYS) */
  while (!LWIP_EXAMPLE_APP_ABORT()) {
#if NO_SYS
    /* handle timers (already done in tcpip.c when NO_SYS=0) */
    sys_check_timeouts();
#endif /* NO_SYS */

#if USE_ETHERNET
    default_netif_poll();
#else /* USE_ETHERNET */
    /* try to read characters from serial line and pass them to PPPoS */
    count = sio_read(ppp_sio, (u8_t*)rxbuf, 1024);
    if(count > 0) {
      pppos_input(ppp, rxbuf, count);
    } else {
      /* nothing received, give other tasks a chance to run */
      sys_msleep(1);
    }

#endif /* USE_ETHERNET */
#if USE_SLIPIF
    slipif_poll(&slipif1);
#if USE_SLIPIF > 1
    slipif_poll(&slipif2);
#endif /* USE_SLIPIF > 1 */
#endif /* USE_SLIPIF */
#if ENABLE_LOOPBACK && !LWIP_NETIF_LOOPBACK_MULTITHREADING
    /* check for loopback packets on all netifs */
    netif_poll_all();
#endif /* ENABLE_LOOPBACK && !LWIP_NETIF_LOOPBACK_MULTITHREADING */
#if USE_PPP
    {
    int do_hup = 0;
    if(do_hup) {
      ppp_close(ppp, 1);
      do_hup = 0;
    }
    }
    if(callClosePpp && ppp) {
      /* make sure to disconnect PPP before stopping the program... */
      callClosePpp = 0;
#if NO_SYS
      ppp_close(ppp, 0);
#else
      pppapi_close(ppp, 0);
#endif
      ppp = NULL;
    }
#endif /* USE_PPP */
  }

#if USE_PPP
    if(ppp) {
      u32_t started;
      printf("Closing PPP connection...\n");
      /* make sure to disconnect PPP before stopping the program... */
#if NO_SYS
      ppp_close(ppp, 0);
#else
      pppapi_close(ppp, 0);
#endif
      ppp = NULL;
      /* Wait for some time to let PPP finish... */
      started = sys_now();
      do
      {
#if USE_ETHERNET
        default_netif_poll();
#endif
        /* @todo: need a better check here: only wait until PPP is down */
      } while(sys_now() - started < 5000);
    }
#endif /* USE_PPP */
#if (LWIP_SOCKET || LWIP_NETCONN) && LWIP_NETCONN_SEM_PER_THREAD
  netconn_thread_cleanup();
#endif
#if USE_ETHERNET
  default_netif_shutdown();
#endif /* USE_ETHERNET */
}

#if USE_PPP && PPPOS_SUPPORT
int main(int argc, char **argv)
#else /* USE_PPP && PPPOS_SUPPORT */
int main(void)
#endif /* USE_PPP && PPPOS_SUPPORT */
{
#if USE_PPP && PPPOS_SUPPORT
  if(argc > 1) {
    sio_idx = (u8_t)atoi(argv[1]);
  }
  printf("Using serial port %d for PPP\n", sio_idx);
#endif /* USE_PPP && PPPOS_SUPPORT */
  /* no stdio-buffering, please! */
  setvbuf(stdout, NULL,_IONBF, 0);

  main_loop();

  return 0;
}

/* This function is only required to prevent arch.h including stdio.h
 * (which it does if LWIP_PLATFORM_ASSERT is undefined)
 */
void lwip_example_app_platform_assert(const char *msg, int line, const char *file)
{
  printf("Assertion \"%s\" failed at line %d in %s\n", msg, line, file);
  fflush(NULL);
  abort();
}
