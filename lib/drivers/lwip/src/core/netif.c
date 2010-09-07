/**
 * @file
 * lwIP network interface abstraction
 *
 */

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

#include "lwip/opt.h"

#include "lwip/def.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/tcp.h"
#include "lwip/snmp.h"
#include "lwip/igmp.h"
#include "netif/etharp.h"
#if ENABLE_LOOPBACK
#include "lwip/sys.h"
#if LWIP_NETIF_LOOPBACK_MULTITHREADING
#include "lwip/tcpip.h"
#endif /* LWIP_NETIF_LOOPBACK_MULTITHREADING */
#endif /* ENABLE_LOOPBACK */

#if LWIP_AUTOIP
#include "lwip/autoip.h"
#endif /* LWIP_AUTOIP */
#if LWIP_DHCP
#include "lwip/dhcp.h"
#endif /* LWIP_DHCP */

#if LWIP_NETIF_STATUS_CALLBACK
#define NETIF_STATUS_CALLBACK(n) { if (n->status_callback) (n->status_callback)(n); }
#else
#define NETIF_STATUS_CALLBACK(n) { /* NOP */ }
#endif /* LWIP_NETIF_STATUS_CALLBACK */ 

#if LWIP_NETIF_LINK_CALLBACK
#define NETIF_LINK_CALLBACK(n) { if (n->link_callback) (n->link_callback)(n); }
#else
#define NETIF_LINK_CALLBACK(n) { /* NOP */ }
#endif /* LWIP_NETIF_LINK_CALLBACK */ 

struct netif *netif_list;
struct netif *netif_default;

/**
 * Add a network interface to the list of lwIP netifs.
 *
 * @param netif a pre-allocated netif structure
 * @param ipaddr IP address for the new netif
 * @param netmask network mask for the new netif
 * @param gw default gateway IP address for the new netif
 * @param state opaque data passed to the new netif
 * @param init callback function that initializes the interface
 * @param input callback function that is called to pass
 * ingress packets up in the protocol layer stack.
 *
 * @return netif, or NULL if failed.
 */
struct netif *
netif_add(struct netif *netif, struct ip_addr *ipaddr, struct ip_addr *netmask,
  struct ip_addr *gw,
  void *state,
  err_t (* init)(struct netif *netif),
  err_t (* input)(struct pbuf *p, struct netif *netif))
{
  static u8_t netifnum = 0;

  /* reset new interface configuration state */
  netif->ip_addr.addr = 0;
  netif->netmask.addr = 0;
  netif->gw.addr = 0;
  netif->flags = 0;
#if LWIP_DHCP
  /* netif not under DHCP control by default */
  netif->dhcp = NULL;
#endif /* LWIP_DHCP */
#if LWIP_AUTOIP
  /* netif not under AutoIP control by default */
  netif->autoip = NULL;
#endif /* LWIP_AUTOIP */
#if LWIP_NETIF_STATUS_CALLBACK
  netif->status_callback = NULL;
#endif /* LWIP_NETIF_STATUS_CALLBACK */
#if LWIP_NETIF_LINK_CALLBACK
  netif->link_callback = NULL;
#endif /* LWIP_NETIF_LINK_CALLBACK */
#if LWIP_IGMP
  netif->igmp_mac_filter = NULL;
#endif /* LWIP_IGMP */
#if ENABLE_LOOPBACK
  netif->loop_first = NULL;
  netif->loop_last = NULL;
#endif /* ENABLE_LOOPBACK */

  /* remember netif specific state information data */
  netif->state = state;
  netif->num = netifnum++;
  netif->input = input;
#if LWIP_NETIF_HWADDRHINT
  netif->addr_hint = NULL;
#endif /* LWIP_NETIF_HWADDRHINT*/
#if ENABLE_LOOPBACK && LWIP_LOOPBACK_MAX_PBUFS
  netif->loop_cnt_current = 0;
#endif /* ENABLE_LOOPBACK && LWIP_LOOPBACK_MAX_PBUFS */

  netif_set_addr(netif, ipaddr, netmask, gw);

  /* call user specified initialization function for netif */
  if (init(netif) != ERR_OK) {
    return NULL;
  }

  /* add this netif to the list */
  netif->next = netif_list;
  netif_list = netif;
  snmp_inc_iflist();

#if LWIP_IGMP
  /* start IGMP processing */
  if (netif->flags & NETIF_FLAG_IGMP) {
    igmp_start( netif);
  }
#endif /* LWIP_IGMP */

  LWIP_DEBUGF(NETIF_DEBUG, ("netif: added interface %c%c IP addr ",
    netif->name[0], netif->name[1]));
  ip_addr_debug_print(NETIF_DEBUG, ipaddr);
  LWIP_DEBUGF(NETIF_DEBUG, (" netmask "));
  ip_addr_debug_print(NETIF_DEBUG, netmask);
  LWIP_DEBUGF(NETIF_DEBUG, (" gw "));
  ip_addr_debug_print(NETIF_DEBUG, gw);
  LWIP_DEBUGF(NETIF_DEBUG, ("\n"));
  return netif;
}

/**
 * Change IP address configuration for a network interface (including netmask
 * and default gateway).
 *
 * @param netif the network interface to change
 * @param ipaddr the new IP address
 * @param netmask the new netmask
 * @param gw the new default gateway
 */
void
netif_set_addr(struct netif *netif, struct ip_addr *ipaddr, struct ip_addr *netmask,
    struct ip_addr *gw)
{
  netif_set_ipaddr(netif, ipaddr);
  netif_set_netmask(netif, netmask);
  netif_set_gw(netif, gw);
}

/**
 * Remove a network interface from the list of lwIP netifs.
 *
 * @param netif the network interface to remove
 */
void netif_remove(struct netif * netif)
{
  if ( netif == NULL ) return;

#if LWIP_IGMP
  /* stop IGMP processing */
  if (netif->flags & NETIF_FLAG_IGMP) {
    igmp_stop( netif);
  }
#endif /* LWIP_IGMP */

  snmp_delete_ipaddridx_tree(netif);

  /*  is it the first netif? */
  if (netif_list == netif) {
    netif_list = netif->next;
    snmp_dec_iflist();
  }
  else {
    /*  look for netif further down the list */
    struct netif * tmpNetif;
    for (tmpNetif = netif_list; tmpNetif != NULL; tmpNetif = tmpNetif->next) {
      if (tmpNetif->next == netif) {
        tmpNetif->next = netif->next;
        snmp_dec_iflist();
        break;
      }
    }
    if (tmpNetif == NULL)
      return; /*  we didn't find any netif today */
  }
  /* this netif is default? */
  if (netif_default == netif)
    /* reset default netif */
    netif_set_default(NULL);
  LWIP_DEBUGF( NETIF_DEBUG, ("netif_remove: removed netif\n") );
}

/**
 * Find a network interface by searching for its name
 *
 * @param name the name of the netif (like netif->name) plus concatenated number
 * in ascii representation (e.g. 'en0')
 */
struct netif *
netif_find(char *name)
{
  struct netif *netif;
  u8_t num;

  if (name == NULL) {
    return NULL;
  }

  num = name[2] - '0';

  for(netif = netif_list; netif != NULL; netif = netif->next) {
    if (num == netif->num &&
       name[0] == netif->name[0] &&
       name[1] == netif->name[1]) {
      LWIP_DEBUGF(NETIF_DEBUG, ("netif_find: found %c%c\n", name[0], name[1]));
      return netif;
    }
  }
  LWIP_DEBUGF(NETIF_DEBUG, ("netif_find: didn't find %c%c\n", name[0], name[1]));
  return NULL;
}

/**
 * Change the IP address of a network interface
 *
 * @param netif the network interface to change
 * @param ipaddr the new IP address
 *
 * @note call netif_set_addr() if you also want to change netmask and
 * default gateway
 */
void
netif_set_ipaddr(struct netif *netif, struct ip_addr *ipaddr)
{
  /* TODO: Handling of obsolete pcbs */
  /* See:  http://mail.gnu.org/archive/html/lwip-users/2003-03/msg00118.html */
#if LWIP_TCP
  struct tcp_pcb *pcb;
  struct tcp_pcb_listen *lpcb;

  /* address is actually being changed? */
  if ((ip_addr_cmp(ipaddr, &(netif->ip_addr))) == 0)
  {
    /* extern struct tcp_pcb *tcp_active_pcbs; defined by tcp.h */
    LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_STATE, ("netif_set_ipaddr: netif address being changed\n"));
    pcb = tcp_active_pcbs;
    while (pcb != NULL) {
      /* PCB bound to current local interface address? */
      if (ip_addr_cmp(&(pcb->local_ip), &(netif->ip_addr))) {
        /* this connection must be aborted */
        struct tcp_pcb *next = pcb->next;
        LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_STATE, ("netif_set_ipaddr: aborting TCP pcb %p\n", (void *)pcb));
        tcp_abort(pcb);
        pcb = next;
      } else {
        pcb = pcb->next;
      }
    }
    for (lpcb = tcp_listen_pcbs.listen_pcbs; lpcb != NULL; lpcb = lpcb->next) {
      /* PCB bound to current local interface address? */
      if ((!(ip_addr_isany(&(lpcb->local_ip)))) &&
          (ip_addr_cmp(&(lpcb->local_ip), &(netif->ip_addr)))) {
        /* The PCB is listening to the old ipaddr and
         * is set to listen to the new one instead */
        ip_addr_set(&(lpcb->local_ip), ipaddr);
      }
    }
  }
#endif
  snmp_delete_ipaddridx_tree(netif);
  snmp_delete_iprteidx_tree(0,netif);
  /* set new IP address to netif */
  ip_addr_set(&(netif->ip_addr), ipaddr);
  snmp_insert_ipaddridx_tree(netif);
  snmp_insert_iprteidx_tree(0,netif);

  LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("netif: IP address of interface %c%c set to %"U16_F".%"U16_F".%"U16_F".%"U16_F"\n",
    netif->name[0], netif->name[1],
    ip4_addr1(&netif->ip_addr),
    ip4_addr2(&netif->ip_addr),
    ip4_addr3(&netif->ip_addr),
    ip4_addr4(&netif->ip_addr)));
}

/**
 * Change the default gateway for a network interface
 *
 * @param netif the network interface to change
 * @param gw the new default gateway
 *
 * @note call netif_set_addr() if you also want to change ip address and netmask
 */
void
netif_set_gw(struct netif *netif, struct ip_addr *gw)
{
  ip_addr_set(&(netif->gw), gw);
  LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("netif: GW address of interface %c%c set to %"U16_F".%"U16_F".%"U16_F".%"U16_F"\n",
    netif->name[0], netif->name[1],
    ip4_addr1(&netif->gw),
    ip4_addr2(&netif->gw),
    ip4_addr3(&netif->gw),
    ip4_addr4(&netif->gw)));
}

/**
 * Change the netmask of a network interface
 *
 * @param netif the network interface to change
 * @param netmask the new netmask
 *
 * @note call netif_set_addr() if you also want to change ip address and
 * default gateway
 */
void
netif_set_netmask(struct netif *netif, struct ip_addr *netmask)
{
  snmp_delete_iprteidx_tree(0, netif);
  /* set new netmask to netif */
  ip_addr_set(&(netif->netmask), netmask);
  snmp_insert_iprteidx_tree(0, netif);
  LWIP_DEBUGF(NETIF_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("netif: netmask of interface %c%c set to %"U16_F".%"U16_F".%"U16_F".%"U16_F"\n",
    netif->name[0], netif->name[1],
    ip4_addr1(&netif->netmask),
    ip4_addr2(&netif->netmask),
    ip4_addr3(&netif->netmask),
    ip4_addr4(&netif->netmask)));
}

/**
 * Set a network interface as the default network interface
 * (used to output all packets for which no specific route is found)
 *
 * @param netif the default network interface
 */
void
netif_set_default(struct netif *netif)
{
  if (netif == NULL)
  {
    /* remove default route */
    snmp_delete_iprteidx_tree(1, netif);
  }
  else
  {
    /* install default route */
    snmp_insert_iprteidx_tree(1, netif);
  }
  netif_default = netif;
  LWIP_DEBUGF(NETIF_DEBUG, ("netif: setting default interface %c%c\n",
           netif ? netif->name[0] : '\'', netif ? netif->name[1] : '\''));
}

/**
 * Bring an interface up, available for processing
 * traffic.
 * 
 * @note: Enabling DHCP on a down interface will make it come
 * up once configured.
 * 
 * @see dhcp_start()
 */ 
void netif_set_up(struct netif *netif)
{
  if ( !(netif->flags & NETIF_FLAG_UP )) {
    netif->flags |= NETIF_FLAG_UP;
    
#if LWIP_SNMP
    snmp_get_sysuptime(&netif->ts);
#endif /* LWIP_SNMP */

    NETIF_LINK_CALLBACK(netif);
    NETIF_STATUS_CALLBACK(netif);

#if LWIP_ARP
    /* For Ethernet network interfaces, we would like to send a "gratuitous ARP" */ 
    if (netif->flags & NETIF_FLAG_ETHARP) {
      etharp_gratuitous(netif);
    }
#endif /* LWIP_ARP */

#if LWIP_IGMP
    /* resend IGMP memberships */
    if (netif->flags & NETIF_FLAG_IGMP) {
      igmp_report_groups( netif);
    }
#endif /* LWIP_IGMP */
  }
}

/**
 * Bring an interface down, disabling any traffic processing.
 *
 * @note: Enabling DHCP on a down interface will make it come
 * up once configured.
 * 
 * @see dhcp_start()
 */ 
void netif_set_down(struct netif *netif)
{
  if ( netif->flags & NETIF_FLAG_UP )
    {
      netif->flags &= ~NETIF_FLAG_UP;
#if LWIP_SNMP
      snmp_get_sysuptime(&netif->ts);
#endif
      
      NETIF_LINK_CALLBACK(netif);
      NETIF_STATUS_CALLBACK(netif);
    }
}

/**
 * Ask if an interface is up
 */ 
u8_t netif_is_up(struct netif *netif)
{
  return (netif->flags & NETIF_FLAG_UP)?1:0;
}

#if LWIP_NETIF_STATUS_CALLBACK
/**
 * Set callback to be called when interface is brought up/down
 */
void netif_set_status_callback(struct netif *netif, void (* status_callback)(struct netif *netif ))
{
    if ( netif )
        netif->status_callback = status_callback;
}
#endif /* LWIP_NETIF_STATUS_CALLBACK */

#if LWIP_NETIF_LINK_CALLBACK
/**
 * Called by a driver when its link goes up
 */
void netif_set_link_up(struct netif *netif )
{
  netif->flags |= NETIF_FLAG_LINK_UP;

#if LWIP_DHCP
  if (netif->dhcp) {
    dhcp_network_changed(netif);
  }
#endif /* LWIP_DHCP */

#if LWIP_AUTOIP
  if (netif->autoip) {
    autoip_network_changed(netif);
  }
#endif /* LWIP_AUTOIP */

  if (netif->flags & NETIF_FLAG_UP) {
#if LWIP_ARP
  /* For Ethernet network interfaces, we would like to send a "gratuitous ARP" */ 
  if (netif->flags & NETIF_FLAG_ETHARP) {
    etharp_gratuitous(netif);
  }
#endif /* LWIP_ARP */

#if LWIP_IGMP
    /* resend IGMP memberships */
    if (netif->flags & NETIF_FLAG_IGMP) {
      igmp_report_groups( netif);
    }
#endif /* LWIP_IGMP */
  }
  NETIF_LINK_CALLBACK(netif);
}

/**
 * Called by a driver when its link goes down
 */
void netif_set_link_down(struct netif *netif )
{
  netif->flags &= ~NETIF_FLAG_LINK_UP;
  NETIF_LINK_CALLBACK(netif);
}

/**
 * Ask if a link is up
 */ 
u8_t netif_is_link_up(struct netif *netif)
{
  return (netif->flags & NETIF_FLAG_LINK_UP) ? 1 : 0;
}

/**
 * Set callback to be called when link is brought up/down
 */
void netif_set_link_callback(struct netif *netif, void (* link_callback)(struct netif *netif ))
{
  if (netif) {
    netif->link_callback = link_callback;
  }
}
#endif /* LWIP_NETIF_LINK_CALLBACK */

#if ENABLE_LOOPBACK
/**
 * Send an IP packet to be received on the same netif (loopif-like).
 * The pbuf is simply copied and handed back to netif->input.
 * In multithreaded mode, this is done directly since netif->input must put
 * the packet on a queue.
 * In callback mode, the packet is put on an internal queue and is fed to
 * netif->input by netif_poll().
 *
 * @param netif the lwip network interface structure
 * @param p the (IP) packet to 'send'
 * @param ipaddr the ip address to send the packet to (not used)
 * @return ERR_OK if the packet has been sent
 *         ERR_MEM if the pbuf used to copy the packet couldn't be allocated
 */
err_t
netif_loop_output(struct netif *netif, struct pbuf *p,
       struct ip_addr *ipaddr)
{
  struct pbuf *r;
  err_t err;
  struct pbuf *last;
#if LWIP_LOOPBACK_MAX_PBUFS
  u8_t clen = 0;
#endif /* LWIP_LOOPBACK_MAX_PBUFS */
  SYS_ARCH_DECL_PROTECT(lev);
  LWIP_UNUSED_ARG(ipaddr);

  /* Allocate a new pbuf */
  r = pbuf_alloc(PBUF_LINK, p->tot_len, PBUF_RAM);
  if (r == NULL) {
    return ERR_MEM;
  }
#if LWIP_LOOPBACK_MAX_PBUFS
  clen = pbuf_clen(r);
  /* check for overflow or too many pbuf on queue */
  if(((netif->loop_cnt_current + clen) < netif->loop_cnt_current) ||
    ((netif->loop_cnt_current + clen) > LWIP_LOOPBACK_MAX_PBUFS)) {
      pbuf_free(r);
      r = NULL;
      return ERR_MEM;
  }
  netif->loop_cnt_current += clen;
#endif /* LWIP_LOOPBACK_MAX_PBUFS */

  /* Copy the whole pbuf queue p into the single pbuf r */
  if ((err = pbuf_copy(r, p)) != ERR_OK) {
    pbuf_free(r);
    r = NULL;
    return err;
  }

  /* Put the packet on a linked list which gets emptied through calling
     netif_poll(). */

  /* let last point to the last pbuf in chain r */
  for (last = r; last->next != NULL; last = last->next);

  SYS_ARCH_PROTECT(lev);
  if(netif->loop_first != NULL) {
    LWIP_ASSERT("if first != NULL, last must also be != NULL", netif->loop_last != NULL);
    netif->loop_last->next = r;
    netif->loop_last = last;
  } else {
    netif->loop_first = r;
    netif->loop_last = last;
  }
  SYS_ARCH_UNPROTECT(lev);

#if LWIP_NETIF_LOOPBACK_MULTITHREADING
  /* For multithreading environment, schedule a call to netif_poll */
  tcpip_callback((void (*)(void *))(netif_poll), netif);
#endif /* LWIP_NETIF_LOOPBACK_MULTITHREADING */

  return ERR_OK;
}

/**
 * Call netif_poll() in the main loop of your application. This is to prevent
 * reentering non-reentrant functions like tcp_input(). Packets passed to
 * netif_loop_output() are put on a list that is passed to netif->input() by
 * netif_poll().
 */
void
netif_poll(struct netif *netif)
{
  struct pbuf *in;
  SYS_ARCH_DECL_PROTECT(lev);

  do {
    /* Get a packet from the list. With SYS_LIGHTWEIGHT_PROT=1, this is protected */
    SYS_ARCH_PROTECT(lev);
    in = netif->loop_first;
    if(in != NULL) {
      struct pbuf *in_end = in;
#if LWIP_LOOPBACK_MAX_PBUFS
      u8_t clen = pbuf_clen(in);
      /* adjust the number of pbufs on queue */
      LWIP_ASSERT("netif->loop_cnt_current underflow",
        ((netif->loop_cnt_current - clen) < netif->loop_cnt_current));
      netif->loop_cnt_current -= clen;
#endif /* LWIP_LOOPBACK_MAX_PBUFS */
      while(in_end->len != in_end->tot_len) {
        LWIP_ASSERT("bogus pbuf: len != tot_len but next == NULL!", in_end->next != NULL);
        in_end = in_end->next;
      }
      /* 'in_end' now points to the last pbuf from 'in' */
      if(in_end == netif->loop_last) {
        /* this was the last pbuf in the list */
        netif->loop_first = netif->loop_last = NULL;
      } else {
        /* pop the pbuf off the list */
        netif->loop_first = in_end->next;
        LWIP_ASSERT("should not be null since first != last!", netif->loop_first != NULL);
      }
      /* De-queue the pbuf from its successors on the 'loop_' list. */
      in_end->next = NULL;
    }
    SYS_ARCH_UNPROTECT(lev);

    if(in != NULL) {
      /* loopback packets are always IP packets! */
      if(ip_input(in, netif) != ERR_OK) {
        pbuf_free(in);
      }
      /* Don't reference the packet any more! */
      in = NULL;
    }
  /* go on while there is a packet on the list */
  } while(netif->loop_first != NULL);
}

#if !LWIP_NETIF_LOOPBACK_MULTITHREADING
/**
 * Calls netif_poll() for every netif on the netif_list.
 */
void
netif_poll_all(void)
{
  struct netif *netif = netif_list;
  /* loop through netifs */
  while (netif != NULL) {
    netif_poll(netif);
    /* proceed to next network interface */
    netif = netif->next;
  }
}
#endif /* !LWIP_NETIF_LOOPBACK_MULTITHREADING */
#endif /* ENABLE_LOOPBACK */
