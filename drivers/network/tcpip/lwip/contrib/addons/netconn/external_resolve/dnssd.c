/**
 * @file
 * DNS-SD APIs used by LWIP_HOOK_NETCONN_EXTERNAL_RESOLVE
 *
 * This implementation assumes the DNS-SD API implementation (most likely provided by
 * mDNSResponder) is implemented in the same process space as LwIP and can directly
 * invoke the callback for DNSServiceGetAddrInfo.  This is the typical deployment in
 * an embedded environment where as a traditional OS requires pumping the callback results
 * through an IPC mechanism (see DNSServiceRefSockFD/DNSServiceProcessResult)
 *
 * @defgroup dnssd DNS-SD
 * @ingroup dns
 */

/*
 * Copyright (c) 2017 Joel Cunningham, Garmin International, Inc. <joel.cunningham@garmin.com>
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
 * Author: Joel Cunningham <joel.cunningham@me.com>
 *
 */
#include "lwip/opt.h"

#include "lwip/err.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "dnssd.h"

/* External headers */
#include <string.h>
#include <dns_sd.h>

/* This timeout should allow for multiple queries.
mDNSResponder has the following query timeline:
  Query 1: time = 0s
  Query 2: time = 1s
  Query 3: time = 4s
*/
#define GETADDR_TIMEOUT_MS  5000
#define LOCAL_DOMAIN        ".local"

/* Only consume .local hosts */
#ifndef CONSUME_LOCAL_ONLY
#define CONSUME_LOCAL_ONLY  1
#endif

struct addr_clbk_msg {
  sys_sem_t sem;
  struct sockaddr_storage addr;
  err_t err;
};

static void addr_info_callback(DNSServiceRef ref, DNSServiceFlags flags, u32_t interface_index,
                               DNSServiceErrorType error_code, char const* hostname,
                               const struct sockaddr* address, u32_t ttl, void* context);

int
lwip_dnssd_gethostbyname(const char *name, ip_addr_t *addr, u8_t addrtype, err_t *err)
{
  DNSServiceErrorType result;
  DNSServiceRef ref;
  struct addr_clbk_msg msg;
  char *p;

  /* @todo: use with IPv6 */
  LWIP_UNUSED_ARG(addrtype);

#if CONSUME_LOCAL_ONLY
  /* check if this is a .local host. If it is, then we consume the query */
  p = strstr(name, LOCAL_DOMAIN);
  if (p == NULL) {
    return 0; /* not consumed */
  }
  p += (sizeof(LOCAL_DOMAIN) - 1);
  /* check to make sure .local isn't a substring (only allow .local\0 or .local.\0) */
  if ((*p != '.' && *p != '\0') ||
      (*p == '.' && *(p + 1) != '\0')) {
    return 0; /* not consumed */
  }
#endif /* CONSUME_LOCAL_ONLY */

  msg.err = sys_sem_new(&msg.sem, 0);
  if (msg.err != ERR_OK) {
    goto query_done;
  }

  msg.err = ERR_TIMEOUT;
  result = DNSServiceGetAddrInfo(&ref, 0, 0, kDNSServiceProtocol_IPv4, name, addr_info_callback, &msg);
  if (result == kDNSServiceErr_NoError) {
    sys_arch_sem_wait(&msg.sem, GETADDR_TIMEOUT_MS);
    DNSServiceRefDeallocate(ref);

    /* We got a response */
    if (msg.err == ERR_OK) {
      struct sockaddr_in* addr_in = (struct sockaddr_in *)&msg.addr;
      if (addr_in->sin_family == AF_INET) {
        inet_addr_to_ip4addr(ip_2_ip4(addr), &addr_in->sin_addr);
      } else {
        /* @todo add IPv6 support */
        msg.err = ERR_VAL;
      }
    }
  }
  sys_sem_free(&msg.sem);

/* Query has been consumed and is finished */
query_done:
*err = msg.err;
return 1;
}

static void
addr_info_callback(DNSServiceRef ref, DNSServiceFlags flags, u32_t interface_index,
                   DNSServiceErrorType error_code, char const* hostname,
                   const struct sockaddr* address, u32_t ttl, void* context)
{
  struct addr_clbk_msg* msg = (struct addr_clbk_msg*)context;
  struct sockaddr_in*  addr_in = (struct sockaddr_in *)address;

  LWIP_UNUSED_ARG(ref);
  LWIP_UNUSED_ARG(flags);
  LWIP_UNUSED_ARG(interface_index);
  LWIP_UNUSED_ARG(hostname);
  LWIP_UNUSED_ARG(ttl);
  LWIP_UNUSED_ARG(context);

  if ((error_code == kDNSServiceErr_NoError) &&
      (addr_in->sin_family == AF_INET)) {
    MEMCPY(&msg->addr, addr_in, sizeof(*addr_in));
    msg->err = ERR_OK;
  }
  else {
   /* @todo add IPv6 support */
   msg->err = ERR_VAL;
  }

  sys_sem_signal(&msg->sem);
} /* addr_info_callback() */
