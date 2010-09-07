/**
 * @file
 * Dynamic Host Configuration Protocol client
 *
 */

/*
 *
 * Copyright (c) 2001-2004 Leon Woestenberg <leon.woestenberg@gmx.net>
 * Copyright (c) 2001-2004 Axon Digital Design B.V., The Netherlands.
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
 * This file is a contribution to the lwIP TCP/IP stack.
 * The Swedish Institute of Computer Science and Adam Dunkels
 * are specifically granted permission to redistribute this
 * source code.
 *
 * Author: Leon Woestenberg <leon.woestenberg@gmx.net>
 *
 * This is a DHCP client for the lwIP TCP/IP stack. It aims to conform
 * with RFC 2131 and RFC 2132.
 *
 * TODO:
 * - Proper parsing of DHCP messages exploiting file/sname field overloading.
 * - Add JavaDoc style documentation (API, internals).
 * - Support for interfaces other than Ethernet (SLIP, PPP, ...)
 *
 * Please coordinate changes and requests with Leon Woestenberg
 * <leon.woestenberg@gmx.net>
 *
 * Integration with your code:
 *
 * In lwip/dhcp.h
 * #define DHCP_COARSE_TIMER_SECS (recommended 60 which is a minute)
 * #define DHCP_FINE_TIMER_MSECS (recommended 500 which equals TCP coarse timer)
 *
 * Then have your application call dhcp_coarse_tmr() and
 * dhcp_fine_tmr() on the defined intervals.
 *
 * dhcp_start(struct netif *netif);
 * starts a DHCP client instance which configures the interface by
 * obtaining an IP address lease and maintaining it.
 *
 * Use dhcp_release(netif) to end the lease and use dhcp_stop(netif)
 * to remove the DHCP client.
 *
 */

#include "lwip/opt.h"

#if LWIP_DHCP /* don't build if not configured for use in lwipopts.h */

#include "lwip/stats.h"
#include "lwip/mem.h"
#include "lwip/udp.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/inet.h"
#include "lwip/sys.h"
#include "lwip/dhcp.h"
#include "lwip/autoip.h"
#include "lwip/dns.h"
#include "netif/etharp.h"

#include <string.h>

/** Default for DHCP_GLOBAL_XID is 0xABCD0000
 * This can be changed by defining DHCP_GLOBAL_XID and DHCP_GLOBAL_XID_HEADER, e.g.
 *  #define DHCP_GLOBAL_XID_HEADER "stdlib.h"
 *  #define DHCP_GLOBAL_XID rand()
 */
#ifdef DHCP_GLOBAL_XID_HEADER
#include DHCP_GLOBAL_XID_HEADER /* include optional starting XID generation prototypes */
#endif

/** DHCP_OPTION_MAX_MSG_SIZE is set to the MTU
 * MTU is checked to be big enough in dhcp_start */
#define DHCP_MAX_MSG_LEN(netif)        (netif->mtu)
#define DHCP_MAX_MSG_LEN_MIN_REQUIRED  576
/** Minimum length for reply before packet is parsed */
#define DHCP_MIN_REPLY_LEN             44

#define REBOOT_TRIES 2

/* DHCP client state machine functions */
static void dhcp_handle_ack(struct netif *netif);
static void dhcp_handle_nak(struct netif *netif);
static void dhcp_handle_offer(struct netif *netif);

static err_t dhcp_discover(struct netif *netif);
static err_t dhcp_select(struct netif *netif);
static void dhcp_bind(struct netif *netif);
#if DHCP_DOES_ARP_CHECK
static void dhcp_check(struct netif *netif);
static err_t dhcp_decline(struct netif *netif);
#endif /* DHCP_DOES_ARP_CHECK */
static err_t dhcp_rebind(struct netif *netif);
static err_t dhcp_reboot(struct netif *netif);
static void dhcp_set_state(struct dhcp *dhcp, u8_t new_state);

/* receive, unfold, parse and free incoming messages */
static void dhcp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, u16_t port);
static err_t dhcp_unfold_reply(struct dhcp *dhcp, struct pbuf *p);
static u8_t *dhcp_get_option_ptr(struct dhcp *dhcp, u8_t option_type);
static u8_t dhcp_get_option_byte(u8_t *ptr);
#if 0
static u16_t dhcp_get_option_short(u8_t *ptr);
#endif
static u32_t dhcp_get_option_long(u8_t *ptr);
static void dhcp_free_reply(struct dhcp *dhcp);

/* set the DHCP timers */
static void dhcp_timeout(struct netif *netif);
static void dhcp_t1_timeout(struct netif *netif);
static void dhcp_t2_timeout(struct netif *netif);

/* build outgoing messages */
/* create a DHCP request, fill in common headers */
static err_t dhcp_create_request(struct netif *netif);
/* free a DHCP request */
static void dhcp_delete_request(struct netif *netif);
/* add a DHCP option (type, then length in bytes) */
static void dhcp_option(struct dhcp *dhcp, u8_t option_type, u8_t option_len);
/* add option values */
static void dhcp_option_byte(struct dhcp *dhcp, u8_t value);
static void dhcp_option_short(struct dhcp *dhcp, u16_t value);
static void dhcp_option_long(struct dhcp *dhcp, u32_t value);
/* always add the DHCP options trailer to end and pad */
static void dhcp_option_trailer(struct dhcp *dhcp);

/**
 * Back-off the DHCP client (because of a received NAK response).
 *
 * Back-off the DHCP client because of a received NAK. Receiving a
 * NAK means the client asked for something non-sensible, for
 * example when it tries to renew a lease obtained on another network.
 *
 * We clear any existing set IP address and restart DHCP negotiation
 * afresh (as per RFC2131 3.2.3).
 *
 * @param netif the netif under DHCP control
 */
static void
dhcp_handle_nak(struct netif *netif)
{
  struct dhcp *dhcp = netif->dhcp;
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_handle_nak(netif=%p) %c%c%"U16_F"\n", 
    (void*)netif, netif->name[0], netif->name[1], (u16_t)netif->num));
  /* Set the interface down since the address must no longer be used, as per RFC2131 */
  netif_set_down(netif);
  /* remove IP address from interface */
  netif_set_ipaddr(netif, IP_ADDR_ANY);
  netif_set_gw(netif, IP_ADDR_ANY);
  netif_set_netmask(netif, IP_ADDR_ANY); 
  /* Change to a defined state */
  dhcp_set_state(dhcp, DHCP_BACKING_OFF);
  /* We can immediately restart discovery */
  dhcp_discover(netif);
}

#if DHCP_DOES_ARP_CHECK
/**
 * Checks if the offered IP address is already in use.
 *
 * It does so by sending an ARP request for the offered address and
 * entering CHECKING state. If no ARP reply is received within a small
 * interval, the address is assumed to be free for use by us.
 *
 * @param netif the netif under DHCP control
 */
static void
dhcp_check(struct netif *netif)
{
  struct dhcp *dhcp = netif->dhcp;
  err_t result;
  u16_t msecs;
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_check(netif=%p) %c%c\n", (void *)netif, (s16_t)netif->name[0],
    (s16_t)netif->name[1]));
  dhcp_set_state(dhcp, DHCP_CHECKING);
  /* create an ARP query for the offered IP address, expecting that no host
     responds, as the IP address should not be in use. */
  result = etharp_query(netif, &dhcp->offered_ip_addr, NULL);
  if (result != ERR_OK) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING, ("dhcp_check: could not perform ARP query\n"));
  }
  dhcp->tries++;
  msecs = 500;
  dhcp->request_timeout = (msecs + DHCP_FINE_TIMER_MSECS - 1) / DHCP_FINE_TIMER_MSECS;
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_check(): set request timeout %"U16_F" msecs\n", msecs));
}
#endif /* DHCP_DOES_ARP_CHECK */

/**
 * Remember the configuration offered by a DHCP server.
 *
 * @param netif the netif under DHCP control
 */
static void
dhcp_handle_offer(struct netif *netif)
{
  struct dhcp *dhcp = netif->dhcp;
  /* obtain the server address */
  u8_t *option_ptr = dhcp_get_option_ptr(dhcp, DHCP_OPTION_SERVER_ID);
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_handle_offer(netif=%p) %c%c%"U16_F"\n",
    (void*)netif, netif->name[0], netif->name[1], (u16_t)netif->num));
  if (option_ptr != NULL) {
    dhcp->server_ip_addr.addr = htonl(dhcp_get_option_long(&option_ptr[2]));
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_STATE, ("dhcp_handle_offer(): server 0x%08"X32_F"\n", dhcp->server_ip_addr.addr));
    /* remember offered address */
    ip_addr_set(&dhcp->offered_ip_addr, (struct ip_addr *)&dhcp->msg_in->yiaddr);
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_STATE, ("dhcp_handle_offer(): offer for 0x%08"X32_F"\n", dhcp->offered_ip_addr.addr));

    dhcp_select(netif);
  }
}

/**
 * Select a DHCP server offer out of all offers.
 *
 * Simply select the first offer received.
 *
 * @param netif the netif under DHCP control
 * @return lwIP specific error (see error.h)
 */
static err_t
dhcp_select(struct netif *netif)
{
  struct dhcp *dhcp = netif->dhcp;
  err_t result;
  u16_t msecs;
#if LWIP_NETIF_HOSTNAME
  const char *p;
#endif /* LWIP_NETIF_HOSTNAME */

  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_select(netif=%p) %c%c%"U16_F"\n", (void*)netif, netif->name[0], netif->name[1], (u16_t)netif->num));
  dhcp_set_state(dhcp, DHCP_REQUESTING);

  /* create and initialize the DHCP message header */
  result = dhcp_create_request(netif);
  if (result == ERR_OK) {
    dhcp_option(dhcp, DHCP_OPTION_MESSAGE_TYPE, DHCP_OPTION_MESSAGE_TYPE_LEN);
    dhcp_option_byte(dhcp, DHCP_REQUEST);

    dhcp_option(dhcp, DHCP_OPTION_MAX_MSG_SIZE, DHCP_OPTION_MAX_MSG_SIZE_LEN);
    dhcp_option_short(dhcp, DHCP_MAX_MSG_LEN(netif));

    /* MUST request the offered IP address */
    dhcp_option(dhcp, DHCP_OPTION_REQUESTED_IP, 4);
    dhcp_option_long(dhcp, ntohl(dhcp->offered_ip_addr.addr));

    dhcp_option(dhcp, DHCP_OPTION_SERVER_ID, 4);
    dhcp_option_long(dhcp, ntohl(dhcp->server_ip_addr.addr));

    dhcp_option(dhcp, DHCP_OPTION_PARAMETER_REQUEST_LIST, 4/*num options*/);
    dhcp_option_byte(dhcp, DHCP_OPTION_SUBNET_MASK);
    dhcp_option_byte(dhcp, DHCP_OPTION_ROUTER);
    dhcp_option_byte(dhcp, DHCP_OPTION_BROADCAST);
    dhcp_option_byte(dhcp, DHCP_OPTION_DNS_SERVER);

#if LWIP_NETIF_HOSTNAME
    p = (const char*)netif->hostname;
    if (p != NULL) {
      dhcp_option(dhcp, DHCP_OPTION_HOSTNAME, strlen(p));
      while (*p) {
        dhcp_option_byte(dhcp, *p++);
      }
    }
#endif /* LWIP_NETIF_HOSTNAME */

    dhcp_option_trailer(dhcp);
    /* shrink the pbuf to the actual content length */
    pbuf_realloc(dhcp->p_out, sizeof(struct dhcp_msg) - DHCP_OPTIONS_LEN + dhcp->options_out_len);

    /* send broadcast to any DHCP server */
    udp_sendto_if(dhcp->pcb, dhcp->p_out, IP_ADDR_BROADCAST, DHCP_SERVER_PORT, netif);
    dhcp_delete_request(netif);
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_select: REQUESTING\n"));
  } else {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING, ("dhcp_select: could not allocate DHCP request\n"));
  }
  dhcp->tries++;
  msecs = (dhcp->tries < 6 ? 1 << dhcp->tries : 60) * 1000;
  dhcp->request_timeout = (msecs + DHCP_FINE_TIMER_MSECS - 1) / DHCP_FINE_TIMER_MSECS;
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_STATE, ("dhcp_select(): set request timeout %"U16_F" msecs\n", msecs));
  return result;
}

/**
 * The DHCP timer that checks for lease renewal/rebind timeouts.
 *
 */
void
dhcp_coarse_tmr()
{
  struct netif *netif = netif_list;
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_coarse_tmr()\n"));
  /* iterate through all network interfaces */
  while (netif != NULL) {
    /* only act on DHCP configured interfaces */
    if (netif->dhcp != NULL) {
      /* timer is active (non zero), and triggers (zeroes) now? */
      if (netif->dhcp->t2_timeout-- == 1) {
        LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_coarse_tmr(): t2 timeout\n"));
        /* this clients' rebind timeout triggered */
        dhcp_t2_timeout(netif);
      /* timer is active (non zero), and triggers (zeroes) now */
      } else if (netif->dhcp->t1_timeout-- == 1) {
        LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_coarse_tmr(): t1 timeout\n"));
        /* this clients' renewal timeout triggered */
        dhcp_t1_timeout(netif);
      }
    }
    /* proceed to next netif */
    netif = netif->next;
  }
}

/**
 * DHCP transaction timeout handling
 *
 * A DHCP server is expected to respond within a short period of time.
 * This timer checks whether an outstanding DHCP request is timed out.
 * 
 */
void
dhcp_fine_tmr()
{
  struct netif *netif = netif_list;
  /* loop through netif's */
  while (netif != NULL) {
    /* only act on DHCP configured interfaces */
    if (netif->dhcp != NULL) {
      /* timer is active (non zero), and is about to trigger now */      
      if (netif->dhcp->request_timeout > 1) {
        netif->dhcp->request_timeout--;
      }
      else if (netif->dhcp->request_timeout == 1) {
        netif->dhcp->request_timeout--;
        /* { netif->dhcp->request_timeout == 0 } */
        LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_fine_tmr(): request timeout\n"));
        /* this clients' request timeout triggered */
        dhcp_timeout(netif);
      }
    }
    /* proceed to next network interface */
    netif = netif->next;
  }
}

/**
 * A DHCP negotiation transaction, or ARP request, has timed out.
 *
 * The timer that was started with the DHCP or ARP request has
 * timed out, indicating no response was received in time.
 *
 * @param netif the netif under DHCP control
 */
static void
dhcp_timeout(struct netif *netif)
{
  struct dhcp *dhcp = netif->dhcp;
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_timeout()\n"));
  /* back-off period has passed, or server selection timed out */
  if ((dhcp->state == DHCP_BACKING_OFF) || (dhcp->state == DHCP_SELECTING)) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_timeout(): restarting discovery\n"));
    dhcp_discover(netif);
  /* receiving the requested lease timed out */
  } else if (dhcp->state == DHCP_REQUESTING) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_timeout(): REQUESTING, DHCP request timed out\n"));
    if (dhcp->tries <= 5) {
      dhcp_select(netif);
    } else {
      LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_timeout(): REQUESTING, releasing, restarting\n"));
      dhcp_release(netif);
      dhcp_discover(netif);
    }
#if DHCP_DOES_ARP_CHECK
  /* received no ARP reply for the offered address (which is good) */
  } else if (dhcp->state == DHCP_CHECKING) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_timeout(): CHECKING, ARP request timed out\n"));
    if (dhcp->tries <= 1) {
      dhcp_check(netif);
    /* no ARP replies on the offered address,
       looks like the IP address is indeed free */
    } else {
      /* bind the interface to the offered address */
      dhcp_bind(netif);
    }
#endif /* DHCP_DOES_ARP_CHECK */
  }
  /* did not get response to renew request? */
  else if (dhcp->state == DHCP_RENEWING) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_timeout(): RENEWING, DHCP request timed out\n"));
    /* just retry renewal */
    /* note that the rebind timer will eventually time-out if renew does not work */
    dhcp_renew(netif);
  /* did not get response to rebind request? */
  } else if (dhcp->state == DHCP_REBINDING) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_timeout(): REBINDING, DHCP request timed out\n"));
    if (dhcp->tries <= 8) {
      dhcp_rebind(netif);
    } else {
      LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_timeout(): RELEASING, DISCOVERING\n"));
      dhcp_release(netif);
      dhcp_discover(netif);
    }
  } else if (dhcp->state == DHCP_REBOOTING) {
    if (dhcp->tries < REBOOT_TRIES) {
      dhcp_reboot(netif);
    } else {
      dhcp_discover(netif);
    }
  }
}

/**
 * The renewal period has timed out.
 *
 * @param netif the netif under DHCP control
 */
static void
dhcp_t1_timeout(struct netif *netif)
{
  struct dhcp *dhcp = netif->dhcp;
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_STATE, ("dhcp_t1_timeout()\n"));
  if ((dhcp->state == DHCP_REQUESTING) || (dhcp->state == DHCP_BOUND) || (dhcp->state == DHCP_RENEWING)) {
    /* just retry to renew - note that the rebind timer (t2) will
     * eventually time-out if renew tries fail. */
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_t1_timeout(): must renew\n"));
    dhcp_renew(netif);
  }
}

/**
 * The rebind period has timed out.
 *
 * @param netif the netif under DHCP control
 */
static void
dhcp_t2_timeout(struct netif *netif)
{
  struct dhcp *dhcp = netif->dhcp;
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_t2_timeout()\n"));
  if ((dhcp->state == DHCP_REQUESTING) || (dhcp->state == DHCP_BOUND) || (dhcp->state == DHCP_RENEWING)) {
    /* just retry to rebind */
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_t2_timeout(): must rebind\n"));
    dhcp_rebind(netif);
  }
}

/**
 * Handle a DHCP ACK packet
 *
 * @param netif the netif under DHCP control
 */
static void
dhcp_handle_ack(struct netif *netif)
{
  struct dhcp *dhcp = netif->dhcp;
  u8_t *option_ptr;
  /* clear options we might not get from the ACK */
  dhcp->offered_sn_mask.addr = 0;
  dhcp->offered_gw_addr.addr = 0;
  dhcp->offered_bc_addr.addr = 0;

  /* lease time given? */
  option_ptr = dhcp_get_option_ptr(dhcp, DHCP_OPTION_LEASE_TIME);
  if (option_ptr != NULL) {
    /* remember offered lease time */
    dhcp->offered_t0_lease = dhcp_get_option_long(option_ptr + 2);
  }
  /* renewal period given? */
  option_ptr = dhcp_get_option_ptr(dhcp, DHCP_OPTION_T1);
  if (option_ptr != NULL) {
    /* remember given renewal period */
    dhcp->offered_t1_renew = dhcp_get_option_long(option_ptr + 2);
  } else {
    /* calculate safe periods for renewal */
    dhcp->offered_t1_renew = dhcp->offered_t0_lease / 2;
  }

  /* renewal period given? */
  option_ptr = dhcp_get_option_ptr(dhcp, DHCP_OPTION_T2);
  if (option_ptr != NULL) {
    /* remember given rebind period */
    dhcp->offered_t2_rebind = dhcp_get_option_long(option_ptr + 2);
  } else {
    /* calculate safe periods for rebinding */
    dhcp->offered_t2_rebind = dhcp->offered_t0_lease;
  }

  /* (y)our internet address */
  ip_addr_set(&dhcp->offered_ip_addr, &dhcp->msg_in->yiaddr);

/**
 * Patch #1308
 * TODO: we must check if the file field is not overloaded by DHCP options!
 */
#if 0
  /* boot server address */
  ip_addr_set(&dhcp->offered_si_addr, &dhcp->msg_in->siaddr);
  /* boot file name */
  if (dhcp->msg_in->file[0]) {
    dhcp->boot_file_name = mem_malloc(strlen(dhcp->msg_in->file) + 1);
    strcpy(dhcp->boot_file_name, dhcp->msg_in->file);
  }
#endif

  /* subnet mask */
  option_ptr = dhcp_get_option_ptr(dhcp, DHCP_OPTION_SUBNET_MASK);
  /* subnet mask given? */
  if (option_ptr != NULL) {
    dhcp->offered_sn_mask.addr = htonl(dhcp_get_option_long(&option_ptr[2]));
  }

  /* gateway router */
  option_ptr = dhcp_get_option_ptr(dhcp, DHCP_OPTION_ROUTER);
  if (option_ptr != NULL) {
    dhcp->offered_gw_addr.addr = htonl(dhcp_get_option_long(&option_ptr[2]));
  }

  /* broadcast address */
  option_ptr = dhcp_get_option_ptr(dhcp, DHCP_OPTION_BROADCAST);
  if (option_ptr != NULL) {
    dhcp->offered_bc_addr.addr = htonl(dhcp_get_option_long(&option_ptr[2]));
  }
  
  /* DNS servers */
  option_ptr = dhcp_get_option_ptr(dhcp, DHCP_OPTION_DNS_SERVER);
  if (option_ptr != NULL) {
    u8_t n;
    dhcp->dns_count = dhcp_get_option_byte(&option_ptr[1]) / (u32_t)sizeof(struct ip_addr);
    /* limit to at most DHCP_MAX_DNS DNS servers */
    if (dhcp->dns_count > DHCP_MAX_DNS)
      dhcp->dns_count = DHCP_MAX_DNS;
    for (n = 0; n < dhcp->dns_count; n++) {
      dhcp->offered_dns_addr[n].addr = htonl(dhcp_get_option_long(&option_ptr[2 + n * 4]));
#if LWIP_DNS
      dns_setserver( n, (struct ip_addr *)(&(dhcp->offered_dns_addr[n].addr)));
#endif /* LWIP_DNS */
    }
#if LWIP_DNS
    dns_setserver( n, (struct ip_addr *)(&ip_addr_any));
#endif /* LWIP_DNS */
  }
}

/**
 * Start DHCP negotiation for a network interface.
 *
 * If no DHCP client instance was attached to this interface,
 * a new client is created first. If a DHCP client instance
 * was already present, it restarts negotiation.
 *
 * @param netif The lwIP network interface
 * @return lwIP error code
 * - ERR_OK - No error
 * - ERR_MEM - Out of memory
 */
err_t
dhcp_start(struct netif *netif)
{
  struct dhcp *dhcp;
  err_t result = ERR_OK;

  LWIP_ERROR("netif != NULL", (netif != NULL), return ERR_ARG;);
  dhcp = netif->dhcp;
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_start(netif=%p) %c%c%"U16_F"\n", (void*)netif, netif->name[0], netif->name[1], (u16_t)netif->num));
  /* Remove the flag that says this netif is handled by DHCP,
     it is set when we succeeded starting. */
  netif->flags &= ~NETIF_FLAG_DHCP;

  /* check MTU of the netif */
  if (netif->mtu < DHCP_MAX_MSG_LEN_MIN_REQUIRED) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_start(): Cannot use this netif with DHCP: MTU is too small\n"));
    return ERR_MEM;
  }

  /* no DHCP client attached yet? */
  if (dhcp == NULL) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_start(): starting new DHCP client\n"));
    dhcp = mem_malloc(sizeof(struct dhcp));
    if (dhcp == NULL) {
      LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_start(): could not allocate dhcp\n"));
      return ERR_MEM;
    }
    /* store this dhcp client in the netif */
    netif->dhcp = dhcp;
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_start(): allocated dhcp"));
  /* already has DHCP client attached */
  } else {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_start(): restarting DHCP configuration\n"));
    if (dhcp->pcb != NULL) {
      udp_remove(dhcp->pcb);
    }
    LWIP_ASSERT("pbuf p_out wasn't freed", dhcp->p_out == NULL);
    LWIP_ASSERT("reply wasn't freed", dhcp->msg_in == NULL &&
      dhcp->options_in == NULL && dhcp->options_in_len == 0);
  }
    
  /* clear data structure */
  memset(dhcp, 0, sizeof(struct dhcp));
  /* allocate UDP PCB */
  dhcp->pcb = udp_new();
  if (dhcp->pcb == NULL) {
    LWIP_DEBUGF(DHCP_DEBUG  | LWIP_DBG_TRACE, ("dhcp_start(): could not obtain pcb\n"));
    mem_free((void *)dhcp);
    netif->dhcp = dhcp = NULL;
    return ERR_MEM;
  }
#if IP_SOF_BROADCAST
  dhcp->pcb->so_options|=SOF_BROADCAST;
#endif /* IP_SOF_BROADCAST */
  /* set up local and remote port for the pcb */
  udp_bind(dhcp->pcb, IP_ADDR_ANY, DHCP_CLIENT_PORT);
  udp_connect(dhcp->pcb, IP_ADDR_ANY, DHCP_SERVER_PORT);
  /* set up the recv callback and argument */
  udp_recv(dhcp->pcb, dhcp_recv, netif);
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_start(): starting DHCP configuration\n"));
  /* (re)start the DHCP negotiation */
  result = dhcp_discover(netif);
  if (result != ERR_OK) {
    /* free resources allocated above */
    dhcp_stop(netif);
    return ERR_MEM;
  }
  /* Set the flag that says this netif is handled by DHCP. */
  netif->flags |= NETIF_FLAG_DHCP;
  return result;
}

/**
 * Inform a DHCP server of our manual configuration.
 *
 * This informs DHCP servers of our fixed IP address configuration
 * by sending an INFORM message. It does not involve DHCP address
 * configuration, it is just here to be nice to the network.
 *
 * @param netif The lwIP network interface
 */
void
dhcp_inform(struct netif *netif)
{
  struct dhcp *dhcp, *old_dhcp;
  err_t result = ERR_OK;
  dhcp = mem_malloc(sizeof(struct dhcp));
  if (dhcp == NULL) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("dhcp_inform(): could not allocate dhcp\n"));
    return;
  }
  memset(dhcp, 0, sizeof(struct dhcp));

  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_inform(): allocated dhcp\n"));
  dhcp->pcb = udp_new();
  if (dhcp->pcb == NULL) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("dhcp_inform(): could not obtain pcb"));
    goto free_dhcp_and_return;
  }
  old_dhcp = netif->dhcp;
  netif->dhcp = dhcp;
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_inform(): created new udp pcb\n"));
  /* create and initialize the DHCP message header */
  result = dhcp_create_request(netif);
  if (result == ERR_OK) {

    dhcp_option(dhcp, DHCP_OPTION_MESSAGE_TYPE, DHCP_OPTION_MESSAGE_TYPE_LEN);
    dhcp_option_byte(dhcp, DHCP_INFORM);

    dhcp_option(dhcp, DHCP_OPTION_MAX_MSG_SIZE, DHCP_OPTION_MAX_MSG_SIZE_LEN);
    dhcp_option_short(dhcp, DHCP_MAX_MSG_LEN(netif));

    dhcp_option_trailer(dhcp);

    pbuf_realloc(dhcp->p_out, sizeof(struct dhcp_msg) - DHCP_OPTIONS_LEN + dhcp->options_out_len);

#if IP_SOF_BROADCAST
    dhcp->pcb->so_options|=SOF_BROADCAST;
#endif /* IP_SOF_BROADCAST */
    udp_bind(dhcp->pcb, IP_ADDR_ANY, DHCP_CLIENT_PORT);
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_inform: INFORMING\n"));
    udp_sendto_if(dhcp->pcb, dhcp->p_out, IP_ADDR_BROADCAST, DHCP_SERVER_PORT, netif);
    dhcp_delete_request(netif);
  } else {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("dhcp_inform: could not allocate DHCP request\n"));
  }

  udp_remove(dhcp->pcb);
  dhcp->pcb = NULL;
  netif->dhcp = old_dhcp;
free_dhcp_and_return:
  mem_free((void *)dhcp);
}

/** Handle a possible change in the network configuration.
 *
 * This enters the REBOOTING state to verify that the currently bound
 * address is still valid.
 */
void
dhcp_network_changed(struct netif *netif)
{
  struct dhcp *dhcp = netif->dhcp;
  if (!dhcp)
    return;
  switch (dhcp->state) {
  case DHCP_REBINDING:
  case DHCP_RENEWING:
  case DHCP_BOUND:
  case DHCP_REBOOTING:
    netif_set_down(netif);
    dhcp->tries = 0;
    dhcp_reboot(netif);
    break;
  case DHCP_OFF:
    /* stay off */
    break;
  default:
    dhcp->tries = 0;
    dhcp_discover(netif);
    break;
  }
}

#if DHCP_DOES_ARP_CHECK
/**
 * Match an ARP reply with the offered IP address.
 *
 * @param netif the network interface on which the reply was received
 * @param addr The IP address we received a reply from
 */
void dhcp_arp_reply(struct netif *netif, struct ip_addr *addr)
{
  LWIP_ERROR("netif != NULL", (netif != NULL), return;);
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_arp_reply()\n"));
  /* is a DHCP client doing an ARP check? */
  if ((netif->dhcp != NULL) && (netif->dhcp->state == DHCP_CHECKING)) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_arp_reply(): CHECKING, arp reply for 0x%08"X32_F"\n", addr->addr));
    /* did a host respond with the address we
       were offered by the DHCP server? */
    if (ip_addr_cmp(addr, &netif->dhcp->offered_ip_addr)) {
      /* we will not accept the offered address */
      LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE | LWIP_DBG_LEVEL_WARNING,
        ("dhcp_arp_reply(): arp reply matched with offered address, declining\n"));
      dhcp_decline(netif);
    }
  }
}

/**
 * Decline an offered lease.
 *
 * Tell the DHCP server we do not accept the offered address.
 * One reason to decline the lease is when we find out the address
 * is already in use by another host (through ARP).
 *
 * @param netif the netif under DHCP control
 */
static err_t
dhcp_decline(struct netif *netif)
{
  struct dhcp *dhcp = netif->dhcp;
  err_t result = ERR_OK;
  u16_t msecs;
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_decline()\n"));
  dhcp_set_state(dhcp, DHCP_BACKING_OFF);
  /* create and initialize the DHCP message header */
  result = dhcp_create_request(netif);
  if (result == ERR_OK) {
    dhcp_option(dhcp, DHCP_OPTION_MESSAGE_TYPE, DHCP_OPTION_MESSAGE_TYPE_LEN);
    dhcp_option_byte(dhcp, DHCP_DECLINE);

    dhcp_option(dhcp, DHCP_OPTION_REQUESTED_IP, 4);
    dhcp_option_long(dhcp, ntohl(dhcp->offered_ip_addr.addr));

    dhcp_option_trailer(dhcp);
    /* resize pbuf to reflect true size of options */
    pbuf_realloc(dhcp->p_out, sizeof(struct dhcp_msg) - DHCP_OPTIONS_LEN + dhcp->options_out_len);

    /* per section 4.4.4, broadcast DECLINE messages */
    udp_sendto_if(dhcp->pcb, dhcp->p_out, IP_ADDR_BROADCAST, DHCP_SERVER_PORT, netif);
    dhcp_delete_request(netif);
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_decline: BACKING OFF\n"));
  } else {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS,
      ("dhcp_decline: could not allocate DHCP request\n"));
  }
  dhcp->tries++;
  msecs = 10*1000;
  dhcp->request_timeout = (msecs + DHCP_FINE_TIMER_MSECS - 1) / DHCP_FINE_TIMER_MSECS;
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_decline(): set request timeout %"U16_F" msecs\n", msecs));
  return result;
}
#endif


/**
 * Start the DHCP process, discover a DHCP server.
 *
 * @param netif the netif under DHCP control
 */
static err_t
dhcp_discover(struct netif *netif)
{
  struct dhcp *dhcp = netif->dhcp;
  err_t result = ERR_OK;
  u16_t msecs;
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_discover()\n"));
  ip_addr_set(&dhcp->offered_ip_addr, IP_ADDR_ANY);
  dhcp_set_state(dhcp, DHCP_SELECTING);
  /* create and initialize the DHCP message header */
  result = dhcp_create_request(netif);
  if (result == ERR_OK) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_discover: making request\n"));
    dhcp_option(dhcp, DHCP_OPTION_MESSAGE_TYPE, DHCP_OPTION_MESSAGE_TYPE_LEN);
    dhcp_option_byte(dhcp, DHCP_DISCOVER);

    dhcp_option(dhcp, DHCP_OPTION_MAX_MSG_SIZE, DHCP_OPTION_MAX_MSG_SIZE_LEN);
    dhcp_option_short(dhcp, DHCP_MAX_MSG_LEN(netif));

    dhcp_option(dhcp, DHCP_OPTION_PARAMETER_REQUEST_LIST, 4/*num options*/);
    dhcp_option_byte(dhcp, DHCP_OPTION_SUBNET_MASK);
    dhcp_option_byte(dhcp, DHCP_OPTION_ROUTER);
    dhcp_option_byte(dhcp, DHCP_OPTION_BROADCAST);
    dhcp_option_byte(dhcp, DHCP_OPTION_DNS_SERVER);

    dhcp_option_trailer(dhcp);

    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_discover: realloc()ing\n"));
    pbuf_realloc(dhcp->p_out, sizeof(struct dhcp_msg) - DHCP_OPTIONS_LEN + dhcp->options_out_len);

    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_discover: sendto(DISCOVER, IP_ADDR_BROADCAST, DHCP_SERVER_PORT)\n"));
    udp_sendto_if(dhcp->pcb, dhcp->p_out, IP_ADDR_BROADCAST, DHCP_SERVER_PORT, netif);
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_discover: deleting()ing\n"));
    dhcp_delete_request(netif);
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_discover: SELECTING\n"));
  } else {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("dhcp_discover: could not allocate DHCP request\n"));
  }
  dhcp->tries++;
#if LWIP_DHCP_AUTOIP_COOP
  if(dhcp->tries >= LWIP_DHCP_AUTOIP_COOP_TRIES && dhcp->autoip_coop_state == DHCP_AUTOIP_COOP_STATE_OFF) {
    dhcp->autoip_coop_state = DHCP_AUTOIP_COOP_STATE_ON;
    autoip_start(netif);
  }
#endif /* LWIP_DHCP_AUTOIP_COOP */
  msecs = (dhcp->tries < 6 ? 1 << dhcp->tries : 60) * 1000;
  dhcp->request_timeout = (msecs + DHCP_FINE_TIMER_MSECS - 1) / DHCP_FINE_TIMER_MSECS;
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_discover(): set request timeout %"U16_F" msecs\n", msecs));
  return result;
}


/**
 * Bind the interface to the offered IP address.
 *
 * @param netif network interface to bind to the offered address
 */
static void
dhcp_bind(struct netif *netif)
{
  u32_t timeout;
  struct dhcp *dhcp;
  struct ip_addr sn_mask, gw_addr;
  LWIP_ERROR("dhcp_bind: netif != NULL", (netif != NULL), return;);
  dhcp = netif->dhcp;
  LWIP_ERROR("dhcp_bind: dhcp != NULL", (dhcp != NULL), return;);
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_bind(netif=%p) %c%c%"U16_F"\n", (void*)netif, netif->name[0], netif->name[1], (u16_t)netif->num));

  /* temporary DHCP lease? */
  if (dhcp->offered_t1_renew != 0xffffffffUL) {
    /* set renewal period timer */
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_bind(): t1 renewal timer %"U32_F" secs\n", dhcp->offered_t1_renew));
    timeout = (dhcp->offered_t1_renew + DHCP_COARSE_TIMER_SECS / 2) / DHCP_COARSE_TIMER_SECS;
    if(timeout > 0xffff) {
      timeout = 0xffff;
    }
    dhcp->t1_timeout = (u16_t)timeout;
    if (dhcp->t1_timeout == 0) {
      dhcp->t1_timeout = 1;
    }
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_bind(): set request timeout %"U32_F" msecs\n", dhcp->offered_t1_renew*1000));
  }
  /* set renewal period timer */
  if (dhcp->offered_t2_rebind != 0xffffffffUL) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_bind(): t2 rebind timer %"U32_F" secs\n", dhcp->offered_t2_rebind));
    timeout = (dhcp->offered_t2_rebind + DHCP_COARSE_TIMER_SECS / 2) / DHCP_COARSE_TIMER_SECS;
    if(timeout > 0xffff) {
      timeout = 0xffff;
    }
    dhcp->t2_timeout = (u16_t)timeout;
    if (dhcp->t2_timeout == 0) {
      dhcp->t2_timeout = 1;
    }
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_bind(): set request timeout %"U32_F" msecs\n", dhcp->offered_t2_rebind*1000));
  }
  /* copy offered network mask */
  ip_addr_set(&sn_mask, &dhcp->offered_sn_mask);

  /* subnet mask not given? */
  /* TODO: this is not a valid check. what if the network mask is 0? */
  if (sn_mask.addr == 0) {
    /* choose a safe subnet mask given the network class */
    u8_t first_octet = ip4_addr1(&sn_mask);
    if (first_octet <= 127) {
      sn_mask.addr = htonl(0xff000000);
    } else if (first_octet >= 192) {
      sn_mask.addr = htonl(0xffffff00);
    } else {
      sn_mask.addr = htonl(0xffff0000);
    }
  }

  ip_addr_set(&gw_addr, &dhcp->offered_gw_addr);
  /* gateway address not given? */
  if (gw_addr.addr == 0) {
    /* copy network address */
    gw_addr.addr = (dhcp->offered_ip_addr.addr & sn_mask.addr);
    /* use first host address on network as gateway */
    gw_addr.addr |= htonl(0x00000001);
  }

#if LWIP_DHCP_AUTOIP_COOP
  if(dhcp->autoip_coop_state == DHCP_AUTOIP_COOP_STATE_ON) {
    autoip_stop(netif);
    dhcp->autoip_coop_state = DHCP_AUTOIP_COOP_STATE_OFF;
  }
#endif /* LWIP_DHCP_AUTOIP_COOP */

  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_STATE, ("dhcp_bind(): IP: 0x%08"X32_F"\n", dhcp->offered_ip_addr.addr));
  netif_set_ipaddr(netif, &dhcp->offered_ip_addr);
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_STATE, ("dhcp_bind(): SN: 0x%08"X32_F"\n", sn_mask.addr));
  netif_set_netmask(netif, &sn_mask);
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_STATE, ("dhcp_bind(): GW: 0x%08"X32_F"\n", gw_addr.addr));
  netif_set_gw(netif, &gw_addr);
  /* bring the interface up */
  netif_set_up(netif);
  /* netif is now bound to DHCP leased address */
  dhcp_set_state(dhcp, DHCP_BOUND);
}

/**
 * Renew an existing DHCP lease at the involved DHCP server.
 *
 * @param netif network interface which must renew its lease
 */
err_t
dhcp_renew(struct netif *netif)
{
  struct dhcp *dhcp = netif->dhcp;
  err_t result;
  u16_t msecs;
#if LWIP_NETIF_HOSTNAME
  const char *p;
#endif /* LWIP_NETIF_HOSTNAME */
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_renew()\n"));
  dhcp_set_state(dhcp, DHCP_RENEWING);

  /* create and initialize the DHCP message header */
  result = dhcp_create_request(netif);
  if (result == ERR_OK) {

    dhcp_option(dhcp, DHCP_OPTION_MESSAGE_TYPE, DHCP_OPTION_MESSAGE_TYPE_LEN);
    dhcp_option_byte(dhcp, DHCP_REQUEST);

    dhcp_option(dhcp, DHCP_OPTION_MAX_MSG_SIZE, DHCP_OPTION_MAX_MSG_SIZE_LEN);
    dhcp_option_short(dhcp, DHCP_MAX_MSG_LEN(netif));

#if LWIP_NETIF_HOSTNAME
    p = (const char*)netif->hostname;
    if (p != NULL) {
      dhcp_option(dhcp, DHCP_OPTION_HOSTNAME, strlen(p));
      while (*p) {
        dhcp_option_byte(dhcp, *p++);
      }
    }
#endif /* LWIP_NETIF_HOSTNAME */

#if 0
    dhcp_option(dhcp, DHCP_OPTION_REQUESTED_IP, 4);
    dhcp_option_long(dhcp, ntohl(dhcp->offered_ip_addr.addr));
#endif

#if 0
    dhcp_option(dhcp, DHCP_OPTION_SERVER_ID, 4);
    dhcp_option_long(dhcp, ntohl(dhcp->server_ip_addr.addr));
#endif
    /* append DHCP message trailer */
    dhcp_option_trailer(dhcp);

    pbuf_realloc(dhcp->p_out, sizeof(struct dhcp_msg) - DHCP_OPTIONS_LEN + dhcp->options_out_len);

    udp_sendto_if(dhcp->pcb, dhcp->p_out, &dhcp->server_ip_addr, DHCP_SERVER_PORT, netif);
    dhcp_delete_request(netif);

    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_renew: RENEWING\n"));
  } else {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("dhcp_renew: could not allocate DHCP request\n"));
  }
  dhcp->tries++;
  /* back-off on retries, but to a maximum of 20 seconds */
  msecs = dhcp->tries < 10 ? dhcp->tries * 2000 : 20 * 1000;
  dhcp->request_timeout = (msecs + DHCP_FINE_TIMER_MSECS - 1) / DHCP_FINE_TIMER_MSECS;
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_renew(): set request timeout %"U16_F" msecs\n", msecs));
  return result;
}

/**
 * Rebind with a DHCP server for an existing DHCP lease.
 *
 * @param netif network interface which must rebind with a DHCP server
 */
static err_t
dhcp_rebind(struct netif *netif)
{
  struct dhcp *dhcp = netif->dhcp;
  err_t result;
  u16_t msecs;
#if LWIP_NETIF_HOSTNAME
  const char *p;
#endif /* LWIP_NETIF_HOSTNAME */
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_rebind()\n"));
  dhcp_set_state(dhcp, DHCP_REBINDING);

  /* create and initialize the DHCP message header */
  result = dhcp_create_request(netif);
  if (result == ERR_OK) {

    dhcp_option(dhcp, DHCP_OPTION_MESSAGE_TYPE, DHCP_OPTION_MESSAGE_TYPE_LEN);
    dhcp_option_byte(dhcp, DHCP_REQUEST);

    dhcp_option(dhcp, DHCP_OPTION_MAX_MSG_SIZE, DHCP_OPTION_MAX_MSG_SIZE_LEN);
    dhcp_option_short(dhcp, DHCP_MAX_MSG_LEN(netif));

#if LWIP_NETIF_HOSTNAME
    p = (const char*)netif->hostname;
    if (p != NULL) {
      dhcp_option(dhcp, DHCP_OPTION_HOSTNAME, strlen(p));
      while (*p) {
        dhcp_option_byte(dhcp, *p++);
      }
    }
#endif /* LWIP_NETIF_HOSTNAME */

#if 0
    dhcp_option(dhcp, DHCP_OPTION_REQUESTED_IP, 4);
    dhcp_option_long(dhcp, ntohl(dhcp->offered_ip_addr.addr));

    dhcp_option(dhcp, DHCP_OPTION_SERVER_ID, 4);
    dhcp_option_long(dhcp, ntohl(dhcp->server_ip_addr.addr));
#endif

    dhcp_option_trailer(dhcp);

    pbuf_realloc(dhcp->p_out, sizeof(struct dhcp_msg) - DHCP_OPTIONS_LEN + dhcp->options_out_len);

    /* broadcast to server */
    udp_sendto_if(dhcp->pcb, dhcp->p_out, IP_ADDR_BROADCAST, DHCP_SERVER_PORT, netif);
    dhcp_delete_request(netif);
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_rebind: REBINDING\n"));
  } else {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("dhcp_rebind: could not allocate DHCP request\n"));
  }
  dhcp->tries++;
  msecs = dhcp->tries < 10 ? dhcp->tries * 1000 : 10 * 1000;
  dhcp->request_timeout = (msecs + DHCP_FINE_TIMER_MSECS - 1) / DHCP_FINE_TIMER_MSECS;
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_rebind(): set request timeout %"U16_F" msecs\n", msecs));
  return result;
}

/**
 * Enter REBOOTING state to verify an existing lease
 *
 * @param netif network interface which must reboot
 */
static err_t
dhcp_reboot(struct netif *netif)
{
  struct dhcp *dhcp = netif->dhcp;
  err_t result;
  u16_t msecs;
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_reboot()\n"));
  dhcp_set_state(dhcp, DHCP_REBOOTING);

  /* create and initialize the DHCP message header */
  result = dhcp_create_request(netif);
  if (result == ERR_OK) {

    dhcp_option(dhcp, DHCP_OPTION_MESSAGE_TYPE, DHCP_OPTION_MESSAGE_TYPE_LEN);
    dhcp_option_byte(dhcp, DHCP_REQUEST);

    dhcp_option(dhcp, DHCP_OPTION_MAX_MSG_SIZE, DHCP_OPTION_MAX_MSG_SIZE_LEN);
    dhcp_option_short(dhcp, 576);

    dhcp_option(dhcp, DHCP_OPTION_REQUESTED_IP, 4);
    dhcp_option_long(dhcp, ntohl(dhcp->offered_ip_addr.addr));

    dhcp_option_trailer(dhcp);

    pbuf_realloc(dhcp->p_out, sizeof(struct dhcp_msg) - DHCP_OPTIONS_LEN + dhcp->options_out_len);

    /* broadcast to server */
    udp_sendto_if(dhcp->pcb, dhcp->p_out, IP_ADDR_BROADCAST, DHCP_SERVER_PORT, netif);
    dhcp_delete_request(netif);
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_reboot: REBOOTING\n"));
  } else {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("dhcp_reboot: could not allocate DHCP request\n"));
  }
  dhcp->tries++;
  msecs = dhcp->tries < 10 ? dhcp->tries * 1000 : 10 * 1000;
  dhcp->request_timeout = (msecs + DHCP_FINE_TIMER_MSECS - 1) / DHCP_FINE_TIMER_MSECS;
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_reboot(): set request timeout %"U16_F" msecs\n", msecs));
  return result;
}


/**
 * Release a DHCP lease.
 *
 * @param netif network interface which must release its lease
 */
err_t
dhcp_release(struct netif *netif)
{
  struct dhcp *dhcp = netif->dhcp;
  err_t result;
  u16_t msecs;
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_release()\n"));

  /* idle DHCP client */
  dhcp_set_state(dhcp, DHCP_OFF);
  /* clean old DHCP offer */
  dhcp->server_ip_addr.addr = 0;
  dhcp->offered_ip_addr.addr = dhcp->offered_sn_mask.addr = 0;
  dhcp->offered_gw_addr.addr = dhcp->offered_bc_addr.addr = 0;
  dhcp->offered_t0_lease = dhcp->offered_t1_renew = dhcp->offered_t2_rebind = 0;
  dhcp->dns_count = 0;
  
  /* create and initialize the DHCP message header */
  result = dhcp_create_request(netif);
  if (result == ERR_OK) {
    dhcp_option(dhcp, DHCP_OPTION_MESSAGE_TYPE, DHCP_OPTION_MESSAGE_TYPE_LEN);
    dhcp_option_byte(dhcp, DHCP_RELEASE);

    dhcp_option_trailer(dhcp);

    pbuf_realloc(dhcp->p_out, sizeof(struct dhcp_msg) - DHCP_OPTIONS_LEN + dhcp->options_out_len);

    udp_sendto_if(dhcp->pcb, dhcp->p_out, &dhcp->server_ip_addr, DHCP_SERVER_PORT, netif);
    dhcp_delete_request(netif);
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_release: RELEASED, DHCP_OFF\n"));
  } else {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("dhcp_release: could not allocate DHCP request\n"));
  }
  dhcp->tries++;
  msecs = dhcp->tries < 10 ? dhcp->tries * 1000 : 10 * 1000;
  dhcp->request_timeout = (msecs + DHCP_FINE_TIMER_MSECS - 1) / DHCP_FINE_TIMER_MSECS;
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("dhcp_release(): set request timeout %"U16_F" msecs\n", msecs));
  /* bring the interface down */
  netif_set_down(netif);
  /* remove IP address from interface */
  netif_set_ipaddr(netif, IP_ADDR_ANY);
  netif_set_gw(netif, IP_ADDR_ANY);
  netif_set_netmask(netif, IP_ADDR_ANY);
  
  /* TODO: netif_down(netif); */
  return result;
}

/**
 * Remove the DHCP client from the interface.
 *
 * @param netif The network interface to stop DHCP on
 */
void
dhcp_stop(struct netif *netif)
{
  struct dhcp *dhcp = netif->dhcp;
  LWIP_ERROR("dhcp_stop: netif != NULL", (netif != NULL), return;);
  /* Remove the flag that says this netif is handled by DHCP. */
  netif->flags &= ~NETIF_FLAG_DHCP;

  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_stop()\n"));
  /* netif is DHCP configured? */
  if (dhcp != NULL) {
#if LWIP_DHCP_AUTOIP_COOP
  if(dhcp->autoip_coop_state == DHCP_AUTOIP_COOP_STATE_ON) {
    autoip_stop(netif);
    dhcp->autoip_coop_state = DHCP_AUTOIP_COOP_STATE_OFF;
  }
#endif /* LWIP_DHCP_AUTOIP_COOP */

    if (dhcp->pcb != NULL) {
      udp_remove(dhcp->pcb);
      dhcp->pcb = NULL;
    }
    LWIP_ASSERT("reply wasn't freed", dhcp->msg_in == NULL &&
      dhcp->options_in == NULL && dhcp->options_in_len == 0);
    mem_free((void *)dhcp);
    netif->dhcp = NULL;
  }
}

/*
 * Set the DHCP state of a DHCP client.
 *
 * If the state changed, reset the number of tries.
 *
 * TODO: we might also want to reset the timeout here?
 */
static void
dhcp_set_state(struct dhcp *dhcp, u8_t new_state)
{
  if (new_state != dhcp->state) {
    dhcp->state = new_state;
    dhcp->tries = 0;
  }
}

/*
 * Concatenate an option type and length field to the outgoing
 * DHCP message.
 *
 */
static void
dhcp_option(struct dhcp *dhcp, u8_t option_type, u8_t option_len)
{
  LWIP_ASSERT("dhcp_option: dhcp->options_out_len + 2 + option_len <= DHCP_OPTIONS_LEN", dhcp->options_out_len + 2U + option_len <= DHCP_OPTIONS_LEN);
  dhcp->msg_out->options[dhcp->options_out_len++] = option_type;
  dhcp->msg_out->options[dhcp->options_out_len++] = option_len;
}
/*
 * Concatenate a single byte to the outgoing DHCP message.
 *
 */
static void
dhcp_option_byte(struct dhcp *dhcp, u8_t value)
{
  LWIP_ASSERT("dhcp_option_byte: dhcp->options_out_len < DHCP_OPTIONS_LEN", dhcp->options_out_len < DHCP_OPTIONS_LEN);
  dhcp->msg_out->options[dhcp->options_out_len++] = value;
}

static void
dhcp_option_short(struct dhcp *dhcp, u16_t value)
{
  LWIP_ASSERT("dhcp_option_short: dhcp->options_out_len + 2 <= DHCP_OPTIONS_LEN", dhcp->options_out_len + 2U <= DHCP_OPTIONS_LEN);
  dhcp->msg_out->options[dhcp->options_out_len++] = (u8_t)((value & 0xff00U) >> 8);
  dhcp->msg_out->options[dhcp->options_out_len++] = (u8_t) (value & 0x00ffU);
}

static void
dhcp_option_long(struct dhcp *dhcp, u32_t value)
{
  LWIP_ASSERT("dhcp_option_long: dhcp->options_out_len + 4 <= DHCP_OPTIONS_LEN", dhcp->options_out_len + 4U <= DHCP_OPTIONS_LEN);
  dhcp->msg_out->options[dhcp->options_out_len++] = (u8_t)((value & 0xff000000UL) >> 24);
  dhcp->msg_out->options[dhcp->options_out_len++] = (u8_t)((value & 0x00ff0000UL) >> 16);
  dhcp->msg_out->options[dhcp->options_out_len++] = (u8_t)((value & 0x0000ff00UL) >> 8);
  dhcp->msg_out->options[dhcp->options_out_len++] = (u8_t)((value & 0x000000ffUL));
}

/**
 * Extract the DHCP message and the DHCP options.
 *
 * Extract the DHCP message and the DHCP options, each into a contiguous
 * piece of memory. As a DHCP message is variable sized by its options,
 * and also allows overriding some fields for options, the easy approach
 * is to first unfold the options into a conitguous piece of memory, and
 * use that further on.
 *
 */
static err_t
dhcp_unfold_reply(struct dhcp *dhcp, struct pbuf *p)
{
  u16_t ret;
  LWIP_ERROR("dhcp != NULL", (dhcp != NULL), return ERR_ARG;);
  /* free any left-overs from previous unfolds */
  dhcp_free_reply(dhcp);
  /* options present? */
  if (p->tot_len > (sizeof(struct dhcp_msg) - DHCP_OPTIONS_LEN)) {
    dhcp->options_in_len = p->tot_len - (sizeof(struct dhcp_msg) - DHCP_OPTIONS_LEN);
    dhcp->options_in = mem_malloc(dhcp->options_in_len);
    if (dhcp->options_in == NULL) {
      LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS,
        ("dhcp_unfold_reply(): could not allocate dhcp->options\n"));
      dhcp->options_in_len = 0;
      return ERR_MEM;
    }
  }
  dhcp->msg_in = mem_malloc(sizeof(struct dhcp_msg) - DHCP_OPTIONS_LEN);
  if (dhcp->msg_in == NULL) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS,
      ("dhcp_unfold_reply(): could not allocate dhcp->msg_in\n"));
    if (dhcp->options_in != NULL) {
      mem_free(dhcp->options_in);
      dhcp->options_in = NULL;
      dhcp->options_in_len = 0;
    }
    return ERR_MEM;
  }

  /** copy the DHCP message without options */
  ret = pbuf_copy_partial(p, dhcp->msg_in, sizeof(struct dhcp_msg) - DHCP_OPTIONS_LEN, 0);
  LWIP_ASSERT("ret == sizeof(struct dhcp_msg) - DHCP_OPTIONS_LEN", ret == sizeof(struct dhcp_msg) - DHCP_OPTIONS_LEN);
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_unfold_reply(): copied %"U16_F" bytes into dhcp->msg_in[]\n",
     sizeof(struct dhcp_msg) - DHCP_OPTIONS_LEN));

  if (dhcp->options_in != NULL) {
    /** copy the DHCP options */
    ret = pbuf_copy_partial(p, dhcp->options_in, dhcp->options_in_len, sizeof(struct dhcp_msg) - DHCP_OPTIONS_LEN);
    LWIP_ASSERT("ret == dhcp->options_in_len", ret == dhcp->options_in_len);
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_unfold_reply(): copied %"U16_F" bytes to dhcp->options_in[]\n",
      dhcp->options_in_len));
  }
  LWIP_UNUSED_ARG(ret);
  return ERR_OK;
}

/**
 * Free the incoming DHCP message including contiguous copy of
 * its DHCP options.
 */
static void dhcp_free_reply(struct dhcp *dhcp)
{
  if (dhcp->msg_in != NULL) {
    mem_free((void *)dhcp->msg_in);
    dhcp->msg_in = NULL;
  }
  if (dhcp->options_in) {
    mem_free(dhcp->options_in);
    dhcp->options_in = NULL;
    dhcp->options_in_len = 0;
  }
  LWIP_DEBUGF(DHCP_DEBUG, ("dhcp_free_reply(): free'd\n"));
}

/**
 * If an incoming DHCP message is in response to us, then trigger the state machine
 */
static void dhcp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, struct ip_addr *addr, u16_t port)
{
  struct netif *netif = (struct netif *)arg;
  struct dhcp *dhcp = netif->dhcp;
  struct dhcp_msg *reply_msg = (struct dhcp_msg *)p->payload;
  u8_t *options_ptr;
  u8_t msg_type;
  u8_t i;
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("dhcp_recv(pbuf = %p) from DHCP server %"U16_F".%"U16_F".%"U16_F".%"U16_F" port %"U16_F"\n", (void*)p,
    (u16_t)(ntohl(addr->addr) >> 24 & 0xff), (u16_t)(ntohl(addr->addr) >> 16 & 0xff),
    (u16_t)(ntohl(addr->addr) >>  8 & 0xff), (u16_t)(ntohl(addr->addr) & 0xff), port));
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("pbuf->len = %"U16_F"\n", p->len));
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("pbuf->tot_len = %"U16_F"\n", p->tot_len));
  /* prevent warnings about unused arguments */
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(addr);
  LWIP_UNUSED_ARG(port);

  LWIP_ASSERT("reply wasn't freed", dhcp->msg_in == NULL &&
    dhcp->options_in == NULL && dhcp->options_in_len == 0);

  if (p->len < DHCP_MIN_REPLY_LEN) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING, ("DHCP reply message too short\n"));
    goto free_pbuf_and_return;
  }

  if (reply_msg->op != DHCP_BOOTREPLY) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING, ("not a DHCP reply message, but type %"U16_F"\n", (u16_t)reply_msg->op));
    goto free_pbuf_and_return;
  }
  /* iterate through hardware address and match against DHCP message */
  for (i = 0; i < netif->hwaddr_len; i++) {
    if (netif->hwaddr[i] != reply_msg->chaddr[i]) {
      LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING,
        ("netif->hwaddr[%"U16_F"]==%02"X16_F" != reply_msg->chaddr[%"U16_F"]==%02"X16_F"\n",
        (u16_t)i, (u16_t)netif->hwaddr[i], (u16_t)i, (u16_t)reply_msg->chaddr[i]));
      goto free_pbuf_and_return;
    }
  }
  /* match transaction ID against what we expected */
  if (ntohl(reply_msg->xid) != dhcp->xid) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING,
      ("transaction id mismatch reply_msg->xid(%"X32_F")!=dhcp->xid(%"X32_F")\n",ntohl(reply_msg->xid),dhcp->xid));
    goto free_pbuf_and_return;
  }
  /* option fields could be unfold? */
  if (dhcp_unfold_reply(dhcp, p) != ERR_OK) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS,
      ("problem unfolding DHCP message - too short on memory?\n"));
    goto free_pbuf_and_return;
  }

  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("searching DHCP_OPTION_MESSAGE_TYPE\n"));
  /* obtain pointer to DHCP message type */
  options_ptr = dhcp_get_option_ptr(dhcp, DHCP_OPTION_MESSAGE_TYPE);
  if (options_ptr == NULL) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING, ("DHCP_OPTION_MESSAGE_TYPE option not found\n"));
    goto free_pbuf_and_return;
  }

  /* read DHCP message type */
  msg_type = dhcp_get_option_byte(options_ptr + 2);
  /* message type is DHCP ACK? */
  if (msg_type == DHCP_ACK) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("DHCP_ACK received\n"));
    /* in requesting state? */
    if (dhcp->state == DHCP_REQUESTING) {
      dhcp_handle_ack(netif);
      dhcp->request_timeout = 0;
#if DHCP_DOES_ARP_CHECK
      /* check if the acknowledged lease address is already in use */
      dhcp_check(netif);
#else
      /* bind interface to the acknowledged lease address */
      dhcp_bind(netif);
#endif
    }
    /* already bound to the given lease address? */
    else if ((dhcp->state == DHCP_REBOOTING) || (dhcp->state == DHCP_REBINDING) || (dhcp->state == DHCP_RENEWING)) {
      dhcp->request_timeout = 0;
      dhcp_bind(netif);
    }
  }
  /* received a DHCP_NAK in appropriate state? */
  else if ((msg_type == DHCP_NAK) &&
    ((dhcp->state == DHCP_REBOOTING) || (dhcp->state == DHCP_REQUESTING) ||
     (dhcp->state == DHCP_REBINDING) || (dhcp->state == DHCP_RENEWING  ))) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("DHCP_NAK received\n"));
    dhcp->request_timeout = 0;
    dhcp_handle_nak(netif);
  }
  /* received a DHCP_OFFER in DHCP_SELECTING state? */
  else if ((msg_type == DHCP_OFFER) && (dhcp->state == DHCP_SELECTING)) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("DHCP_OFFER received in DHCP_SELECTING state\n"));
    dhcp->request_timeout = 0;
    /* remember offered lease */
    dhcp_handle_offer(netif);
  }
free_pbuf_and_return:
  dhcp_free_reply(dhcp);
  pbuf_free(p);
}

/**
 * Create a DHCP request, fill in common headers
 *
 * @param netif the netif under DHCP control
 */
static err_t
dhcp_create_request(struct netif *netif)
{
  struct dhcp *dhcp;
  u16_t i;
#ifndef DHCP_GLOBAL_XID
  /** default global transaction identifier starting value (easy to match
   *  with a packet analyser). We simply increment for each new request.
   *  Predefine DHCP_GLOBAL_XID to a better value or a function call to generate one
   *  at runtime, any supporting function prototypes can be defined in DHCP_GLOBAL_XID_HEADER */
  static u32_t xid = 0xABCD0000;
#else
  static u32_t xid;
  static u8_t xid_initialised = 0;
  if (!xid_initialised) {
    xid = DHCP_GLOBAL_XID;
    xid_initialised = !xid_initialised;
  }
#endif
  LWIP_ERROR("dhcp_create_request: netif != NULL", (netif != NULL), return ERR_ARG;);
  dhcp = netif->dhcp;
  LWIP_ERROR("dhcp_create_request: dhcp != NULL", (dhcp != NULL), return ERR_VAL;);
  LWIP_ASSERT("dhcp_create_request: dhcp->p_out == NULL", dhcp->p_out == NULL);
  LWIP_ASSERT("dhcp_create_request: dhcp->msg_out == NULL", dhcp->msg_out == NULL);
  dhcp->p_out = pbuf_alloc(PBUF_TRANSPORT, sizeof(struct dhcp_msg), PBUF_RAM);
  if (dhcp->p_out == NULL) {
    LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS,
      ("dhcp_create_request(): could not allocate pbuf\n"));
    return ERR_MEM;
  }
  LWIP_ASSERT("dhcp_create_request: check that first pbuf can hold struct dhcp_msg",
           (dhcp->p_out->len >= sizeof(struct dhcp_msg)));

  /* reuse transaction identifier in retransmissions */
  if (dhcp->tries==0)
      xid++;
  dhcp->xid = xid;
  LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE,
              ("transaction id xid(%"X32_F")\n", xid));

  dhcp->msg_out = (struct dhcp_msg *)dhcp->p_out->payload;

  dhcp->msg_out->op = DHCP_BOOTREQUEST;
  /* TODO: make link layer independent */
  dhcp->msg_out->htype = DHCP_HTYPE_ETH;
  /* TODO: make link layer independent */
  dhcp->msg_out->hlen = DHCP_HLEN_ETH;
  dhcp->msg_out->hops = 0;
  dhcp->msg_out->xid = htonl(dhcp->xid);
  dhcp->msg_out->secs = 0;
  dhcp->msg_out->flags = 0;
  dhcp->msg_out->ciaddr.addr = 0;
  if (dhcp->state==DHCP_BOUND || dhcp->state==DHCP_RENEWING || dhcp->state==DHCP_REBINDING) {
    dhcp->msg_out->ciaddr.addr = netif->ip_addr.addr;
  }
  dhcp->msg_out->yiaddr.addr = 0;
  dhcp->msg_out->siaddr.addr = 0;
  dhcp->msg_out->giaddr.addr = 0;
  for (i = 0; i < DHCP_CHADDR_LEN; i++) {
    /* copy netif hardware address, pad with zeroes */
    dhcp->msg_out->chaddr[i] = (i < netif->hwaddr_len) ? netif->hwaddr[i] : 0/* pad byte*/;
  }
  for (i = 0; i < DHCP_SNAME_LEN; i++) {
    dhcp->msg_out->sname[i] = 0;
  }
  for (i = 0; i < DHCP_FILE_LEN; i++) {
    dhcp->msg_out->file[i] = 0;
  }
  dhcp->msg_out->cookie = htonl(0x63825363UL);
  dhcp->options_out_len = 0;
  /* fill options field with an incrementing array (for debugging purposes) */
  for (i = 0; i < DHCP_OPTIONS_LEN; i++) {
    dhcp->msg_out->options[i] = (u8_t)i; /* for debugging only, no matter if truncated */
  }
  return ERR_OK;
}

/**
 * Free previously allocated memory used to send a DHCP request.
 *
 * @param netif the netif under DHCP control
 */
static void
dhcp_delete_request(struct netif *netif)
{
  struct dhcp *dhcp;
  LWIP_ERROR("dhcp_delete_request: netif != NULL", (netif != NULL), return;);
  dhcp = netif->dhcp;
  LWIP_ERROR("dhcp_delete_request: dhcp != NULL", (dhcp != NULL), return;);
  LWIP_ASSERT("dhcp_delete_request: dhcp->p_out != NULL", dhcp->p_out != NULL);
  LWIP_ASSERT("dhcp_delete_request: dhcp->msg_out != NULL", dhcp->msg_out != NULL);
  if (dhcp->p_out != NULL) {
    pbuf_free(dhcp->p_out);
  }
  dhcp->p_out = NULL;
  dhcp->msg_out = NULL;
}

/**
 * Add a DHCP message trailer
 *
 * Adds the END option to the DHCP message, and if
 * necessary, up to three padding bytes.
 *
 * @param dhcp DHCP state structure
 */
static void
dhcp_option_trailer(struct dhcp *dhcp)
{
  LWIP_ERROR("dhcp_option_trailer: dhcp != NULL", (dhcp != NULL), return;);
  LWIP_ASSERT("dhcp_option_trailer: dhcp->msg_out != NULL\n", dhcp->msg_out != NULL);
  LWIP_ASSERT("dhcp_option_trailer: dhcp->options_out_len < DHCP_OPTIONS_LEN\n", dhcp->options_out_len < DHCP_OPTIONS_LEN);
  dhcp->msg_out->options[dhcp->options_out_len++] = DHCP_OPTION_END;
  /* packet is too small, or not 4 byte aligned? */
  while ((dhcp->options_out_len < DHCP_MIN_OPTIONS_LEN) || (dhcp->options_out_len & 3)) {
    /* LWIP_DEBUGF(DHCP_DEBUG,("dhcp_option_trailer:dhcp->options_out_len=%"U16_F", DHCP_OPTIONS_LEN=%"U16_F, dhcp->options_out_len, DHCP_OPTIONS_LEN)); */
    LWIP_ASSERT("dhcp_option_trailer: dhcp->options_out_len < DHCP_OPTIONS_LEN\n", dhcp->options_out_len < DHCP_OPTIONS_LEN);
    /* add a fill/padding byte */
    dhcp->msg_out->options[dhcp->options_out_len++] = 0;
  }
}

/**
 * Find the offset of a DHCP option inside the DHCP message.
 *
 * @param dhcp DHCP client
 * @param option_type
 *
 * @return a byte offset into the UDP message where the option was found, or
 * zero if the given option was not found.
 */
static u8_t *dhcp_get_option_ptr(struct dhcp *dhcp, u8_t option_type)
{
  u8_t overload = DHCP_OVERLOAD_NONE;

  /* options available? */
  if ((dhcp->options_in != NULL) && (dhcp->options_in_len > 0)) {
    /* start with options field */
    u8_t *options = (u8_t *)dhcp->options_in;
    u16_t offset = 0;
    /* at least 1 byte to read and no end marker, then at least 3 bytes to read? */
    while ((offset < dhcp->options_in_len) && (options[offset] != DHCP_OPTION_END)) {
      /* LWIP_DEBUGF(DHCP_DEBUG, ("msg_offset=%"U16_F", q->len=%"U16_F, msg_offset, q->len)); */
      /* are the sname and/or file field overloaded with options? */
      if (options[offset] == DHCP_OPTION_OVERLOAD) {
        LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("overloaded message detected\n"));
        /* skip option type and length */
        offset += 2;
        overload = options[offset++];
      }
      /* requested option found */
      else if (options[offset] == option_type) {
        LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("option found at offset %"U16_F" in options\n", offset));
        return &options[offset];
      /* skip option */
      } else {
         LWIP_DEBUGF(DHCP_DEBUG, ("skipping option %"U16_F" in options\n", options[offset]));
        /* skip option type */
        offset++;
        /* skip option length, and then length bytes */
        offset += 1 + options[offset];
      }
    }
    /* is this an overloaded message? */
    if (overload != DHCP_OVERLOAD_NONE) {
      u16_t field_len;
      if (overload == DHCP_OVERLOAD_FILE) {
        LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("overloaded file field\n"));
        options = (u8_t *)&dhcp->msg_in->file;
        field_len = DHCP_FILE_LEN;
      } else if (overload == DHCP_OVERLOAD_SNAME) {
        LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("overloaded sname field\n"));
        options = (u8_t *)&dhcp->msg_in->sname;
        field_len = DHCP_SNAME_LEN;
      /* TODO: check if else if () is necessary */
      } else {
        LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("overloaded sname and file field\n"));
        options = (u8_t *)&dhcp->msg_in->sname;
        field_len = DHCP_FILE_LEN + DHCP_SNAME_LEN;
      }
      offset = 0;

      /* at least 1 byte to read and no end marker */
      while ((offset < field_len) && (options[offset] != DHCP_OPTION_END)) {
        if (options[offset] == option_type) {
           LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("option found at offset=%"U16_F"\n", offset));
          return &options[offset];
        /* skip option */
        } else {
          LWIP_DEBUGF(DHCP_DEBUG | LWIP_DBG_TRACE, ("skipping option %"U16_F"\n", options[offset]));
          /* skip option type */
          offset++;
          offset += 1 + options[offset];
        }
      }
    }
  }
  return NULL;
}

/**
 * Return the byte of DHCP option data.
 *
 * @param client DHCP client.
 * @param ptr pointer obtained by dhcp_get_option_ptr().
 *
 * @return byte value at the given address.
 */
static u8_t
dhcp_get_option_byte(u8_t *ptr)
{
  LWIP_DEBUGF(DHCP_DEBUG, ("option byte value=%"U16_F"\n", (u16_t)(*ptr)));
  return *ptr;
}

#if 0 /* currently unused */
/**
 * Return the 16-bit value of DHCP option data.
 *
 * @param client DHCP client.
 * @param ptr pointer obtained by dhcp_get_option_ptr().
 *
 * @return byte value at the given address.
 */
static u16_t
dhcp_get_option_short(u8_t *ptr)
{
  u16_t value;
  value = *ptr++ << 8;
  value |= *ptr;
  LWIP_DEBUGF(DHCP_DEBUG, ("option short value=%"U16_F"\n", value));
  return value;
}
#endif

/**
 * Return the 32-bit value of DHCP option data.
 *
 * @param client DHCP client.
 * @param ptr pointer obtained by dhcp_get_option_ptr().
 *
 * @return byte value at the given address.
 */
static u32_t dhcp_get_option_long(u8_t *ptr)
{
  u32_t value;
  value = (u32_t)(*ptr++) << 24;
  value |= (u32_t)(*ptr++) << 16;
  value |= (u32_t)(*ptr++) << 8;
  value |= (u32_t)(*ptr++);
  LWIP_DEBUGF(DHCP_DEBUG, ("option long value=%"U32_F"\n", value));
  return value;
}

#endif /* LWIP_DHCP */
