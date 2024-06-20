/*
 *   VDE (virtual distributed ethernet) interface for ale4net
 *   (based on tapif interface Adam Dunkels <adam@sics.se>)
 *   2005,2010,2011,2023 Renzo Davoli University of Bologna - Italy
 */

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
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/socket.h>

#include "lwip/opt.h"

#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/ip.h"
#include "lwip/mem.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "netif/etharp.h"
#include "lwip/ethip6.h"
#include <libvdeplug.h>

#include "netif/vdeif.h"

/* Define those to better describe your network interface. */
#define IFNAME0 'v'
#define IFNAME1 'd'

#ifndef VDEIF_DEBUG
#define VDEIF_DEBUG LWIP_DBG_OFF
#endif

static char vdedescr[] = "lwip";

struct vdeif {
  VDECONN *vdeconn;
};

/* Forward declarations. */
static void vdeif_input(struct netif *netif);
#if !NO_SYS
static void vdeif_thread(void *arg);
#endif /* !NO_SYS */

/*-----------------------------------------------------------------------------------*/
static void
low_level_init(struct netif *netif, char *vderl)
{
  struct vdeif *vdeif;
  int randaddr;
  struct timeval now;

  vdeif = (struct vdeif *)netif->state;
  gettimeofday(&now, NULL);
  srand(now.tv_sec + now.tv_usec);
  randaddr = rand();

  /* Obtain MAC address from network interface. */

  /* (We just fake an address...) */
  netif->hwaddr[0] = 0x02;
  netif->hwaddr[1] = 0x2;
  netif->hwaddr[2] = randaddr >> 24;
  netif->hwaddr[3] = randaddr >> 16;
  netif->hwaddr[4] = randaddr >> 8;
  netif->hwaddr[5] = randaddr;
  netif->hwaddr_len = 6;

  /* device capabilities */
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP;

  vdeif->vdeconn = vde_open(vderl, vdedescr, NULL);
  LWIP_DEBUGF(VDEIF_DEBUG, ("vdeif_init: ok = %d\n", !!vdeif->vdeconn));
  if (vdeif->vdeconn == NULL) {
    perror("vdeif_init: cannot open vde net");
    exit(1);
  }

  netif_set_link_up(netif);

#if !NO_SYS
  sys_thread_new("vdeif_thread", vdeif_thread, netif, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
#endif /* !NO_SYS */
}
/*-----------------------------------------------------------------------------------*/
/*
 * low_level_output():
 *
 * Should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 */
/*-----------------------------------------------------------------------------------*/

static err_t
low_level_output(struct netif *netif, struct pbuf *p)
{
  struct vdeif *vdeif = (struct vdeif *)netif->state;
  char buf[1518]; /* max packet size including VLAN excluding CRC */
  ssize_t written;

  if (p->tot_len > sizeof(buf)) {
    MIB2_STATS_NETIF_INC(netif, ifoutdiscards);
    perror("vdeif: packet too large");
    return ERR_IF;
  }

  /* initiate transfer(); */
  pbuf_copy_partial(p, buf, p->tot_len, 0);

  /* signal that packet should be sent(); */
  written = vde_send(vdeif->vdeconn, buf, p->tot_len, 0);
  if (written < p->tot_len) {
    MIB2_STATS_NETIF_INC(netif, ifoutdiscards);
    perror("vdeif: write");
    return ERR_IF;
  } else {
    MIB2_STATS_NETIF_ADD(netif, ifoutoctets, (u32_t)written);
    return ERR_OK;
  }
}
/*-----------------------------------------------------------------------------------*/
/*
 * low_level_input():
 *
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 */
/*-----------------------------------------------------------------------------------*/
static struct pbuf *
low_level_input(struct netif *netif)
{
  struct pbuf *p;
  u16_t len;
  ssize_t readlen;
  char buf[1518]; /* max packet size including VLAN excluding CRC */
  struct vdeif *vdeif = (struct vdeif *)netif->state;

  /* Obtain the size of the packet and put it into the "len"
     variable. */
  readlen = vde_recv(vdeif->vdeconn, buf, sizeof(buf), 0);
  if (readlen < 0) {
    perror("read returned -1");
    exit(1);
  }
  len = (u16_t)readlen;

  MIB2_STATS_NETIF_ADD(netif, ifinoctets, len);

  /* We allocate a pbuf chain of pbufs from the pool. */
  p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
  if (p != NULL) {
    pbuf_take(p, buf, len);
    /* acknowledge that packet has been read(); */
  } else {
    /* drop packet(); */
    MIB2_STATS_NETIF_INC(netif, ifindiscards);
    LWIP_DEBUGF(NETIF_DEBUG, ("vdeif_input: could not allocate pbuf\n"));
  }

  return p;
}

/*-----------------------------------------------------------------------------------*/
/*
 * vdeif_input():
 *
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface.
 *
 */
/*-----------------------------------------------------------------------------------*/
static void
vdeif_input(struct netif *netif)
{
  struct pbuf *p = low_level_input(netif);

  if (p == NULL) {
#if LINK_STATS
    LINK_STATS_INC(link.recv);
#endif /* LINK_STATS */
    LWIP_DEBUGF(VDEIF_DEBUG, ("vdeif_input: low_level_input returned NULL\n"));
    return;
  }

  if (netif->input(p, netif) != ERR_OK) {
    LWIP_DEBUGF(NETIF_DEBUG, ("vdeif_input: netif input error\n"));
    pbuf_free(p);
  }
}
/*-----------------------------------------------------------------------------------*/
/*
 * vdeif_init():
 *
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 */
/*-----------------------------------------------------------------------------------*/
err_t
vdeif_init(struct netif *netif)
{
  char *vderl = (char *) netif->state;
  struct vdeif *vdeif = (struct vdeif *)mem_malloc(sizeof(struct vdeif));

  if (vdeif == NULL) {
    LWIP_DEBUGF(NETIF_DEBUG, ("vdeif_init: out of memory for vdeif\n"));
    return ERR_MEM;
  }
  netif->state = vdeif;
  MIB2_INIT_NETIF(netif, snmp_ifType_other, 100000000);

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
#if LWIP_IPV4
  netif->output = etharp_output;
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
  netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
  netif->linkoutput = low_level_output;
  netif->mtu = 1500;

  low_level_init(netif, vderl);

  return ERR_OK;
}


/*-----------------------------------------------------------------------------------*/
void
vdeif_poll(struct netif *netif)
{
  vdeif_input(netif);
}

#if NO_SYS

int
vdeif_select(struct netif *netif)
{
  fd_set fdset;
  int ret;
  struct timeval tv;
  struct vdeif *vdeif;
  u32_t msecs = sys_timeouts_sleeptime();
  int datafd;

  vdeif = (struct vdeif *)netif->state;
  datafd = vde_datafd(vdeif->vdeconn);

  tv.tv_sec = msecs / 1000;
  tv.tv_usec = (msecs % 1000) * 1000;

  FD_ZERO(&fdset);
  FD_SET(datafd, &fdset);

  ret = select(datafd + 1, &fdset, NULL, NULL, &tv);
  if (ret > 0) {
    vdeif_input(netif);
  }
  return ret;
}

#else /* NO_SYS */

static void
vdeif_thread(void *arg)
{
  struct netif *netif;
  struct vdeif *vdeif;
  fd_set fdset;
  int ret;
  int datafd;

  netif = (struct netif *)arg;
  vdeif = (struct vdeif *)netif->state;
  datafd = vde_datafd(vdeif->vdeconn);

  while(1) {
    FD_ZERO(&fdset);
    FD_SET(datafd, &fdset);

    /* Wait for a packet to arrive. */
    ret = select(datafd + 1, &fdset, NULL, NULL, NULL);

    if(ret == 1) {
      /* Handle incoming packet. */
      vdeif_input(netif);
    } else if(ret == -1) {
      perror("vdeif_thread: select");
    }
  }
}

#endif /* NO_SYS */
