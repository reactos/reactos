/**
 * @file
 * ACD protocol definitions
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

#ifndef LWIP_HDR_PROT_ACD_H
#define LWIP_HDR_PROT_ACD_H

#ifdef __cplusplus
extern "C" {
#endif

/* RFC 5227 and RFC 3927 Constants */
#define PROBE_WAIT           1   /* second  (initial random delay)                    */
#define PROBE_MIN            1   /* second  (minimum delay till repeated probe)       */
#define PROBE_MAX            2   /* seconds (maximum delay till repeated probe)       */
#define PROBE_NUM            3   /*         (number of probe packets)                 */
#define ANNOUNCE_NUM         2   /*         (number of announcement packets)          */
#define ANNOUNCE_INTERVAL    2   /* seconds (time between announcement packets)       */
#define ANNOUNCE_WAIT        2   /* seconds (delay before announcing)                 */
#define MAX_CONFLICTS        10  /*         (max conflicts before rate limiting)      */
#define RATE_LIMIT_INTERVAL  60  /* seconds (delay between successive attempts)       */
#define DEFEND_INTERVAL      10  /* seconds (minimum interval between defensive ARPs) */

/* ACD states */
typedef enum {
  /* ACD is module is off */
  ACD_STATE_OFF,
  /* Waiting before probing can be started */
  ACD_STATE_PROBE_WAIT,
  /* Probing the ipaddr */
  ACD_STATE_PROBING,
  /* Waiting before announcing the probed ipaddr */
  ACD_STATE_ANNOUNCE_WAIT,
  /* Announcing the new ipaddr */
  ACD_STATE_ANNOUNCING,
  /* Performing ongoing conflict detection with one defend within defend inferval */
  ACD_STATE_ONGOING,
  /* Performing ongoing conflict detection but immediately back off and Release
   * the address when a conflict occurs. This state is used for LL addresses
   * that stay active even if the netif has a routable address selected.
   * In such a case, we cannot defend our address */
  ACD_STATE_PASSIVE_ONGOING,
  /* To many conflicts occurred, we need to wait before restarting the selection
   * process */
  ACD_STATE_RATE_LIMIT
} acd_state_enum_t;

typedef enum {
  ACD_IP_OK,            /* IP address is good, no conflicts found in checking state */
  ACD_RESTART_CLIENT,   /* Conflict found -> the client should try again */
  ACD_DECLINE           /* Decline the received IP address (rate limiting)*/
} acd_callback_enum_t;

#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_PROT_ACD_H */
