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
 * Author: Simon Goldschmidt <goldsimon@gmx.de>
 *
 * To use the hooks in this file, make sure this file is included in LWIP_HOOK_FILENAME
 * and define these hooks:
 *
 * #define LWIP_HOOK_TCP_INPACKET_PCB(pcb, hdr, optlen, opt1len, opt2, p) tcp_md5_check_inpacket(pcb, hdr, optlen, opt1len, opt2, p)
 * #define LWIP_HOOK_TCP_OPT_LENGTH_SEGMENT(pcb, internal_len)            tcp_md5_get_additional_option_length(pcb, internal_len)
 * #define LWIP_HOOK_TCP_ADD_TX_OPTIONS(p, hdr, pcb, opts)                tcp_md5_add_tx_options(p, hdr, pcb,  opts)
 *
 * #define LWIP_HOOK_SOCKETS_SETSOCKOPT(s, sock, level, optname, optval, optlen, err) tcp_md5_setsockopt_hook(sock, level, optname, optval, optlen, err)
 */

#ifndef LWIP_HDR_CONTRIB_ADDONS_TCP_MD5_H
#define LWIP_HDR_CONTRIB_ADDONS_TCP_MD5_H

#include "lwip/opt.h"
#include "lwip/ip_addr.h"
#include "lwip/err.h"

#include "lwip/priv/sockets_priv.h"
#include "lwip/priv/tcp_priv.h"

#ifdef __cplusplus
extern "C" {
#endif

/* setsockopt definitions and structs: */

/* This is the optname (for level = IPPROTO_TCP) */
#ifndef TCP_MD5SIG
#define TCP_MD5SIG 14
#endif

#define TCP_MD5SIG_MAXKEYLEN 80

/* This is the optval type */
struct tcp_md5sig {
  struct  sockaddr_storage tcpm_addr;
  u16_t   __tcpm_pad1;
  u16_t   tcpm_keylen;
  u32_t   __tcpm_pad2;
  u8_t    tcpm_key[TCP_MD5SIG_MAXKEYLEN];
};

/* socket setsockopt hook: */
int tcp_md5_setsockopt_hook(struct lwip_sock *sock, int level, int optname, const void *optval, u32_t optlen, int *err);

/* Internal hook functions */
void tcp_md5_init(void);
err_t tcp_md5_check_inpacket(struct tcp_pcb* pcb, struct tcp_hdr *hdr, u16_t optlen, u16_t opt1len, u8_t *opt2, struct pbuf *p);
u8_t tcp_md5_get_additional_option_length(const struct tcp_pcb *pcb, u8_t internal_option_length);
u32_t *tcp_md5_add_tx_options(struct pbuf *p, struct tcp_hdr *hdr, const struct tcp_pcb *pcb, u32_t *opts);

#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_CONTRIB_ADDONS_TCP_MD5_H */
