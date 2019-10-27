/**
 * @file
 * MDNS responder
 */

 /*
 * Copyright (c) 2015 Verisure Innovation AB
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

#ifndef LWIP_HDR_APPS_MDNS_H
#define LWIP_HDR_APPS_MDNS_H

#include "lwip/apps/mdns_opts.h"
#include "lwip/netif.h"

#ifdef __cplusplus
extern "C" {
#endif

#if LWIP_MDNS_RESPONDER

enum mdns_sd_proto {
  DNSSD_PROTO_UDP = 0,
  DNSSD_PROTO_TCP = 1
};

#define MDNS_PROBING_CONFLICT   0
#define MDNS_PROBING_SUCCESSFUL 1

#define MDNS_LABEL_MAXLEN  63

struct mdns_host;
struct mdns_service;

/** Callback function to add text to a reply, called when generating the reply */
typedef void (*service_get_txt_fn_t)(struct mdns_service *service, void *txt_userdata);

/** Callback function to let application know the result of probing network for name
 * uniqueness, called with result MDNS_PROBING_SUCCESSFUL if no other node claimed
 * use for the name for the netif or a service and is safe to use, or MDNS_PROBING_CONFLICT
 * if another node is already using it and mdns is disabled on this interface */
typedef void (*mdns_name_result_cb_t)(struct netif* netif, u8_t result);

void mdns_resp_init(void);

void mdns_resp_register_name_result_cb(mdns_name_result_cb_t cb);

err_t mdns_resp_add_netif(struct netif *netif, const char *hostname, u32_t dns_ttl);
err_t mdns_resp_remove_netif(struct netif *netif);
err_t mdns_resp_rename_netif(struct netif *netif, const char *hostname);

s8_t  mdns_resp_add_service(struct netif *netif, const char *name, const char *service, enum mdns_sd_proto proto, u16_t port, u32_t dns_ttl, service_get_txt_fn_t txt_fn, void *txt_userdata);
err_t mdns_resp_del_service(struct netif *netif, s8_t slot);
err_t mdns_resp_rename_service(struct netif *netif, s8_t slot, const char *name);

err_t mdns_resp_add_service_txtitem(struct mdns_service *service, const char *txt, u8_t txt_len);

void mdns_resp_restart(struct netif *netif);
void mdns_resp_announce(struct netif *netif);

/**
 * @ingroup mdns
 * Announce IP settings have changed on netif.
 * Call this in your callback registered by netif_set_status_callback().
 * No need to call this function when LWIP_NETIF_EXT_STATUS_CALLBACK==1,
 * this handled automatically for you.
 * @param netif The network interface where settings have changed.
 */
#define mdns_resp_netif_settings_changed(netif) mdns_resp_announce(netif)

#endif /* LWIP_MDNS_RESPONDER */

#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_APPS_MDNS_H */
