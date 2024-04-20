/**
 * @file
 *
 * ACD IPv4 Address Conflict Detection
 *
 * This is an IPv4 address conflict detection implementation for the lwIP TCP/IP
 * stack. It aims to be conform to RFC5227.
 *
 * @defgroup acd ACD
 * @ingroup ip4
 * ACD related functions
 * USAGE:
 *
 * define @ref LWIP_ACD 1 in your lwipopts.h
 * Options:
 * ACD_TMR_INTERVAL msecs,
 *   I recommend a value of 100. The value must divide 1000 with a remainder almost 0.
 *   Possible values are 1000, 500, 333, 250, 200, 166, 142, 125, 111, 100 ....
 *
 * For fixed IP:
 * - call acd_start after selecting an IP address. The caller will be informed
 *   on conflict status via the callback function.
 *
 * With AUTOIP:
 * - will be called from the autoip module. No extra's needed.
 *
 * With DHCP:
 * - enable LWIP_DHCP_DOES_ACD_CHECK. Then it will be called from the dhcp module.
 *   No extra's needed.
 */

/*
 *
 * Copyright (c) 2007 Dominik Spies <kontakt@dspies.de>
 * Copyright (c) 2018 Jasper Verschueren <jasper.verschueren@apart-audio.com>
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
 * Author: Jasper Verschueren <jasper.verschueren@apart-audio.com>
 * Author: Dominik Spies <kontakt@dspies.de>
 */

#include "lwip/opt.h"

/* don't build if not configured for use in lwipopts.h */
#if LWIP_IPV4 && LWIP_ACD

#include <string.h>

#include "lwip/acd.h"
#include "lwip/prot/acd.h"

#define ACD_FOREACH(acd, acd_list) for ((acd) = acd_list; (acd) != NULL; (acd) = (acd)->next)

#define ACD_TICKS_PER_SECOND  (1000 / ACD_TMR_INTERVAL)

/* Define good random function (LWIP_RAND) in lwipopts.h */
#ifdef LWIP_RAND
#define LWIP_ACD_RAND(netif, acd)    LWIP_RAND()
#else /* LWIP_RAND */
#ifdef LWIP_AUTOIP_RAND
#include "lwip/autoip.h"
#define LWIP_ACD_RAND(netif, acd)    LWIP_AUTOIP_RAND(netif) /* for backwards compatibility */
#else
#define LWIP_ACD_RAND(netif, acd) ((((u32_t)((netif->hwaddr[5]) & 0xff) << 24) | \
                                    ((u32_t)((netif->hwaddr[3]) & 0xff) << 16) | \
                                    ((u32_t)((netif->hwaddr[2]) & 0xff) << 8) | \
                                    ((u32_t)((netif->hwaddr[4]) & 0xff))) + \
                                    (acd->sent_num))
#endif /* LWIP_AUTOIP_RAND */
#endif /* LWIP_RAND */


#define ACD_RANDOM_PROBE_WAIT(netif, acd) (LWIP_ACD_RAND(netif, acd) % \
                                    (PROBE_WAIT * ACD_TICKS_PER_SECOND))

#define ACD_RANDOM_PROBE_INTERVAL(netif, acd) ((LWIP_ACD_RAND(netif, acd) % \
                                    ((PROBE_MAX - PROBE_MIN) * ACD_TICKS_PER_SECOND)) + \
                                    (PROBE_MIN * ACD_TICKS_PER_SECOND ))

/* Function definitions */
static void acd_restart(struct netif *netif, struct acd *acd);
static void acd_handle_arp_conflict(struct netif *netif, struct acd *acd);
static void acd_put_in_passive_mode(struct netif *netif, struct acd *acd);

/**
 * @ingroup acd
 * Add ACD client to the client list and initialize callback function
 *
 * @param netif                 network interface on which to start the acd
 *                              client
 * @param acd                   acd module to be added to the list
 * @param acd_conflict_callback callback to be called when conflict information
 *                              is available
 */
err_t
acd_add(struct netif *netif, struct acd *acd,
         acd_conflict_callback_t acd_conflict_callback)
{
  struct acd *acd2;

  /* Set callback */
  LWIP_ASSERT_CORE_LOCKED();
  LWIP_ASSERT("acd_conflict_callback != NULL", acd_conflict_callback != NULL);
  acd->acd_conflict_callback = acd_conflict_callback;

  /* Check if the acd struct is already added */
  for (acd2 = netif->acd_list; acd2 != NULL; acd2 = acd2->next) {
    if (acd2 == acd) {
      LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
                  ("acd_add(): acd already added to list\n"));
      return ERR_OK;
    }
  }

  /* add acd struct to the list */
  acd->next = netif->acd_list;
  netif->acd_list = acd;

  return ERR_OK;
}

/**
 * @ingroup acd
 * Remvoe ACD client from the client list
 *
 * @param netif network interface from which to remove the acd client
 * @param acd   acd module to be removed from the list
 */
void
acd_remove(struct netif *netif, struct acd *acd)
{
  struct acd *acd2, *prev = NULL;

  LWIP_ASSERT_CORE_LOCKED();

  for (acd2 = netif->acd_list; acd2 != NULL; acd2 = acd2->next) {
    if (acd2 == acd) {
      if (prev) {
        prev->next = acd->next;
      } else {
        netif->acd_list = acd->next;
      }
      return;
    }
    prev = acd2;
  }
  LWIP_ASSERT(("acd_remove(): acd not on list\n"), 0);
}


/**
 * @ingroup acd
 * Start ACD client
 *
 * @param netif   network interface on which to start the acd client
 * @param acd     acd module to start
 * @param ipaddr  ip address to perform acd on
 */
err_t
acd_start(struct netif *netif, struct acd *acd, ip4_addr_t ipaddr)
{
  err_t result = ERR_OK;

  LWIP_UNUSED_ARG(netif);
  LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
              ("acd_start(netif=%p) %c%c%"U16_F"\n",
              (void *)netif, netif->name[0],
               netif->name[1], (u16_t)netif->num));

  /* init probing state */
  acd->sent_num = 0;
  acd->lastconflict = 0;
  ip4_addr_copy(acd->ipaddr, ipaddr);
  acd->state = ACD_STATE_PROBE_WAIT;

  acd->ttw = (u16_t)(ACD_RANDOM_PROBE_WAIT(netif, acd));

  return result;
}

/**
 * @ingroup acd
 * Stop ACD client
 *
 * @param acd   acd module to stop
 */
err_t
acd_stop(struct acd *acd)
{
  LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("acd_stop\n"));

  if (acd != NULL) {
    acd->state = ACD_STATE_OFF;
  }
  return ERR_OK;
}

/**
 * @ingroup acd
 * Inform the ACD modules when the link goes down
 *
 * @param netif network interface on which to inform the ACD clients
 */
void
acd_network_changed_link_down(struct netif *netif)
{
  struct acd *acd;
  /* loop over the acd's*/
  ACD_FOREACH(acd, netif->acd_list) {
    acd_stop(acd);
  }
}

/**
 * Has to be called in loop every ACD_TMR_INTERVAL milliseconds
 */
void
acd_tmr(void)
{
  struct netif *netif;
  struct acd *acd;
  /* loop through netif's */
  NETIF_FOREACH(netif) {
    ACD_FOREACH(acd, netif->acd_list) {
      if (acd->lastconflict > 0) {
        acd->lastconflict--;
      }

      LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE,
                  ("acd_tmr() ACD-State: %"U16_F", ttw=%"U16_F"\n",
                   (u16_t)(acd->state), acd->ttw));

      if (acd->ttw > 0) {
        acd->ttw--;
      }

      switch (acd->state) {
        case ACD_STATE_PROBE_WAIT:
        case ACD_STATE_PROBING:
          if (acd->ttw == 0) {
            acd->state = ACD_STATE_PROBING;
            etharp_acd_probe(netif, &acd->ipaddr);
            LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE,
                        ("acd_tmr() PROBING Sent Probe\n"));
            acd->sent_num++;
            if (acd->sent_num >= PROBE_NUM) {
              /* Switch to ANNOUNCE_WAIT: last probe is sent*/
              acd->state = ACD_STATE_ANNOUNCE_WAIT;

              acd->sent_num = 0;

              /* calculate time to wait before announcing */
              acd->ttw = (u16_t)(ANNOUNCE_WAIT * ACD_TICKS_PER_SECOND);
            } else {
              /* calculate time to wait to next probe */
              acd->ttw = (u16_t)(ACD_RANDOM_PROBE_INTERVAL(netif, acd));
            }
          }
          break;

        case ACD_STATE_ANNOUNCE_WAIT:
        case ACD_STATE_ANNOUNCING:
          if (acd->ttw == 0) {
            if (acd->sent_num == 0) {
              acd->state = ACD_STATE_ANNOUNCING;

              /* reset conflict count to ensure fast re-probing after announcing */
              acd->num_conflicts = 0;

              LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
                    ("acd_tmr(): changing state to ANNOUNCING: %"U16_F".%"U16_F".%"U16_F".%"U16_F"\n",
                     ip4_addr1_16(&acd->ipaddr), ip4_addr2_16(&acd->ipaddr),
                     ip4_addr3_16(&acd->ipaddr), ip4_addr4_16(&acd->ipaddr)));
            }

            etharp_acd_announce(netif, &acd->ipaddr);
            LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE,
                        ("acd_tmr() ANNOUNCING Sent Announce\n"));
            acd->ttw = ANNOUNCE_INTERVAL * ACD_TICKS_PER_SECOND;
            acd->sent_num++;

            if (acd->sent_num >= ANNOUNCE_NUM) {
              acd->state = ACD_STATE_ONGOING;
              acd->sent_num = 0;
              acd->ttw = 0;
              LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
                    ("acd_tmr(): changing state to ONGOING: %"U16_F".%"U16_F".%"U16_F".%"U16_F"\n",
                     ip4_addr1_16(&acd->ipaddr), ip4_addr2_16(&acd->ipaddr),
                     ip4_addr3_16(&acd->ipaddr), ip4_addr4_16(&acd->ipaddr)));

              /* finally, let acd user know that the address is good and can be used */
              acd->acd_conflict_callback(netif, ACD_IP_OK);
            }
          }
          break;

        case ACD_STATE_RATE_LIMIT:
          if (acd->ttw == 0) {
            /* acd should be stopped because ipaddr isn't valid any more */
            acd_stop(acd);
            /* let the acd user (after rate limit interval) know that their is
             * a conflict detected. So it can restart the address acquiring
             * process.*/
            acd->acd_conflict_callback(netif, ACD_RESTART_CLIENT);
          }
          break;

        default:
          /* nothing to do in other states */
          break;
      }
    }
  }
}

/**
 * Restarts the acd module
 *
 * The number of conflicts is increased and the upper layer is informed.
 */
static void
acd_restart(struct netif *netif, struct acd *acd)
{
  /* increase conflict counter. */
  acd->num_conflicts++;

  /* Decline the address */
  acd->acd_conflict_callback(netif, ACD_DECLINE);

  /* if we tried more then MAX_CONFLICTS we must limit our rate for
   * acquiring and probing addresses. compliant to RFC 5227 Section 2.1.1 */
  if (acd->num_conflicts >= MAX_CONFLICTS) {
    acd->state = ACD_STATE_RATE_LIMIT;
    acd->ttw = (u16_t)(RATE_LIMIT_INTERVAL * ACD_TICKS_PER_SECOND);
    LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE | LWIP_DBG_LEVEL_WARNING,
                ("acd_restart(): rate limiting initiated. too many conflicts\n"));
  }
  else {
    /* acd should be stopped because ipaddr isn't valid any more */
    acd_stop(acd);
    /* let the acd user know right away that their is a conflict detected.
     * So it can restart the address acquiring process. */
    acd->acd_conflict_callback(netif, ACD_RESTART_CLIENT);
  }
}

/**
 * Handles every incoming ARP Packet, called by etharp_input().
 *
 * @param netif network interface to use for acd processing
 * @param hdr   Incoming ARP packet
 */
void
acd_arp_reply(struct netif *netif, struct etharp_hdr *hdr)
{
  struct acd *acd;
  ip4_addr_t sipaddr, dipaddr;
  struct eth_addr netifaddr;
  SMEMCPY(netifaddr.addr, netif->hwaddr, ETH_HWADDR_LEN);

  /* Copy struct ip4_addr_wordaligned to aligned ip4_addr, to support
   * compilers without structure packing (not using structure copy which
   * breaks strict-aliasing rules).
   */
  IPADDR_WORDALIGNED_COPY_TO_IP4_ADDR_T(&sipaddr, &hdr->sipaddr);
  IPADDR_WORDALIGNED_COPY_TO_IP4_ADDR_T(&dipaddr, &hdr->dipaddr);

  LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE, ("acd_arp_reply()\n"));

  /* loop over the acd's*/
  ACD_FOREACH(acd, netif->acd_list) {
    switch(acd->state) {
      case ACD_STATE_OFF:
      case ACD_STATE_RATE_LIMIT:
      default:
        /* do nothing */
        break;

      case ACD_STATE_PROBE_WAIT:
      case ACD_STATE_PROBING:
      case ACD_STATE_ANNOUNCE_WAIT:
        /* RFC 5227 Section 2.1.1:
         * from beginning to after ANNOUNCE_WAIT seconds we have a conflict if
         * ip.src == ipaddr (someone is already using the address)
         * OR
         * ip.dst == ipaddr && hw.src != own hwaddr (someone else is probing it)
         */
        if ((ip4_addr_eq(&sipaddr, &acd->ipaddr)) ||
            (ip4_addr_isany_val(sipaddr) &&
             ip4_addr_eq(&dipaddr, &acd->ipaddr) &&
             !eth_addr_eq(&netifaddr, &hdr->shwaddr))) {
          LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE | LWIP_DBG_LEVEL_WARNING,
                      ("acd_arp_reply(): Probe Conflict detected\n"));
          acd_restart(netif, acd);
        }
        break;

      case ACD_STATE_ANNOUNCING:
      case ACD_STATE_ONGOING:
      case ACD_STATE_PASSIVE_ONGOING:
        /* RFC 5227 Section 2.4:
         * in any state we have a conflict if
         * ip.src == ipaddr && hw.src != own hwaddr (someone is using our address)
         */
        if (ip4_addr_eq(&sipaddr, &acd->ipaddr) &&
            !eth_addr_eq(&netifaddr, &hdr->shwaddr)) {
          LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE | LWIP_DBG_LEVEL_WARNING,
                      ("acd_arp_reply(): Conflicting ARP-Packet detected\n"));
          acd_handle_arp_conflict(netif, acd);
        }
        break;
    }
  }
}

/**
 * Handle a IP address conflict after an ARP conflict detection
 */
static void
acd_handle_arp_conflict(struct netif *netif, struct acd *acd)
{
  /* RFC5227, 2.4 "Ongoing Address Conflict Detection and Address Defense"
     allows three options where:
     a) means retreat on the first conflict,
     b) allows to keep an already configured address when having only one
        conflict in DEFEND_INTERVAL seconds and
     c) the host will not give up it's address and defend it indefinitely

     We use option b) when the acd module represents the netif address, since it
     helps to improve the chance that one of the two conflicting hosts may be
     able to retain its address. while we are flexible enough to help network
     performance

     We use option a) when the acd module does not represent the netif address,
     since we cannot have the acd module announcing or restarting. This
     situation occurs for the LL acd module when a routable address is used on
     the netif but the LL address is still open in the background. */

  if (acd->state == ACD_STATE_PASSIVE_ONGOING) {
    /* Immediately back off on a conflict. */
    LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
      ("acd_handle_arp_conflict(): conflict when we are in passive mode -> back off\n"));
    acd_stop(acd);
    acd->acd_conflict_callback(netif, ACD_DECLINE);
  }
  else {
    if (acd->lastconflict > 0) {
      /* retreat, there was a conflicting ARP in the last DEFEND_INTERVAL seconds */
      LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
        ("acd_handle_arp_conflict(): conflict within DEFEND_INTERVAL -> retreating\n"));

      /* Active TCP sessions are aborted when removing the ip address but a bad
       * connection was inevitable anyway with conflicting hosts */
       acd_restart(netif, acd);
    } else {
      LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
          ("acd_handle_arp_conflict(): we are defending, send ARP Announce\n"));
      etharp_acd_announce(netif, &acd->ipaddr);
      acd->lastconflict = DEFEND_INTERVAL * ACD_TICKS_PER_SECOND;
    }
  }
}

/**
 * Put the acd module in passive ongoing conflict detection.
 */
static void
acd_put_in_passive_mode(struct netif *netif, struct acd *acd)
{
  switch(acd->state) {
    case ACD_STATE_OFF:
    case ACD_STATE_PASSIVE_ONGOING:
    default:
      /* do nothing */
      break;

    case ACD_STATE_PROBE_WAIT:
    case ACD_STATE_PROBING:
    case ACD_STATE_ANNOUNCE_WAIT:
    case ACD_STATE_RATE_LIMIT:
      acd_stop(acd);
      acd->acd_conflict_callback(netif, ACD_DECLINE);
      break;

    case ACD_STATE_ANNOUNCING:
    case ACD_STATE_ONGOING:
      acd->state = ACD_STATE_PASSIVE_ONGOING;
      LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
        ("acd_put_in_passive_mode()\n"));
      break;
  }
}

/**
 * @ingroup acd
 * Inform the ACD modules of address changes
 *
 * @param netif     network interface on which the address is changing
 * @param old_addr  old ip address
 * @param new_addr  new ip address
 */
void
acd_netif_ip_addr_changed(struct netif *netif, const ip_addr_t *old_addr,
                          const ip_addr_t *new_addr)
{
  struct acd *acd;

  LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
    ("acd_netif_ip_addr_changed(): Address changed\n"));

  LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
    ("acd_netif_ip_addr_changed(): old address = %s\n", ipaddr_ntoa(old_addr)));
  LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
    ("acd_netif_ip_addr_changed(): new address = %s\n", ipaddr_ntoa(new_addr)));

  /* If we change from ANY to an IP or from an IP to ANY we do nothing */
  if (ip_addr_isany(old_addr) || ip_addr_isany(new_addr)) {
    return;
  }

  ACD_FOREACH(acd, netif->acd_list) {
    /* Find ACD module of old address */
    if(ip4_addr_eq(&acd->ipaddr, ip_2_ip4(old_addr))) {
      /* Did we change from a LL address to a routable address? */
      if (ip_addr_islinklocal(old_addr) && !ip_addr_islinklocal(new_addr)) {
        LWIP_DEBUGF(ACD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE,
          ("acd_netif_ip_addr_changed(): changed from LL to routable address\n"));
        /* Put the module in passive conflict detection mode */
        acd_put_in_passive_mode(netif, acd);
      }
    }
  }
}

#endif /* LWIP_IPV4 && LWIP_ACD */
