/**
 * @file
 * Application layered TCP connection API (to be used from TCPIP thread)<br>
 * This interface mimics the tcp callback API to the application while preventing
 * direct linking (much like virtual functions).
 * This way, an application can make use of other application layer protocols
 * on top of TCP without knowing the details (e.g. TLS, proxy connection).
 *
 * This file contains allocation implementation that combine several layers.
 */

/*
 * Copyright (c) 2017 Simon Goldschmidt
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

#include "lwip/opt.h"

#if LWIP_ALTCP /* don't build if not configured for use in lwipopts.h */

#include "lwip/altcp.h"
#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/priv/altcp_priv.h"
#include "lwip/mem.h"

#include <string.h>

#if LWIP_ALTCP_TLS

/** This standard allocator function creates an altcp pcb for
 * TLS over TCP */
struct altcp_pcb *
altcp_tls_new(struct altcp_tls_config *config, u8_t ip_type)
{
  struct altcp_pcb *inner_conn, *ret;
  LWIP_UNUSED_ARG(ip_type);

  inner_conn = altcp_tcp_new_ip_type(ip_type);
  if (inner_conn == NULL) {
    return NULL;
  }
  ret = altcp_tls_wrap(config, inner_conn);
  if (ret == NULL) {
    altcp_close(inner_conn);
  }
  return ret;
}

/** This standard allocator function creates an altcp pcb for
 * TLS over TCP */
struct altcp_pcb *
altcp_tls_alloc(void *arg, u8_t ip_type)
{
  return altcp_tls_new((struct altcp_tls_config *)arg, ip_type);
}

#endif /* LWIP_ALTCP_TLS */

#endif /* LWIP_ALTCP */
