/**
 * @file
 * Application layered TCP connection API that executes a proxy-connect.
 *
 * This file provides a starting layer that executes a proxy-connect e.g. to
 * set up TLS connections through a http proxy.
 */

/*
 * Copyright (c) 2018 Simon Goldschmidt
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
 * Author: Simon Goldschmidt <goldsimon@gmx.de>
 *
 */

#ifndef LWIP_HDR_APPS_ALTCP_PROXYCONNECT_H
#define LWIP_HDR_APPS_ALTCP_PROXYCONNECT_H

#include "lwip/opt.h"

#if LWIP_ALTCP /* don't build if not configured for use in lwipopts.h */

#include "lwip/ip_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

struct altcp_proxyconnect_config {
  ip_addr_t proxy_addr;
  u16_t proxy_port;
};


struct altcp_pcb *altcp_proxyconnect_new(struct altcp_proxyconnect_config *config, struct altcp_pcb *inner_pcb);
struct altcp_pcb *altcp_proxyconnect_new_tcp(struct altcp_proxyconnect_config *config, u8_t ip_type);

struct altcp_pcb *altcp_proxyconnect_alloc(void *arg, u8_t ip_type);

#if LWIP_ALTCP_TLS
struct altcp_proxyconnect_tls_config {
  struct altcp_proxyconnect_config proxy;
  struct altcp_tls_config *tls_config;
};

struct altcp_pcb *altcp_proxyconnect_tls_alloc(void *arg, u8_t ip_type);
#endif /* LWIP_ALTCP_TLS */

#ifdef __cplusplus
}
#endif

#endif /* LWIP_ALTCP */
#endif /* LWIP_HDR_APPS_ALTCP_PROXYCONNECT_H */
