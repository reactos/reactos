/**
 * @file: An implementation of TCP MD5 signatures by using various hooks in
 * lwIP to implement custom tcp options and custom socket options.
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
 * Author: Simon Goldschmidt <goldsimon@gmx.de>
 */

#include "tcp_md5.h"
#include "lwip/ip_addr.h"
#include "lwip/sys.h"
#include "lwip/prot/tcp.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/sockets.h"
#include "lwip/priv/sockets_priv.h"
#include "lwip/api.h"
#include <string.h>

/* pull in md5 of ppp? */
#include "netif/ppp/ppp_opts.h"
#if !PPP_SUPPORT || (!LWIP_USE_EXTERNAL_POLARSSL && !LWIP_USE_EXTERNAL_MBEDTLS)
#undef  LWIP_INCLUDED_POLARSSL_MD5
#define LWIP_INCLUDED_POLARSSL_MD5 1
#include "netif/ppp/polarssl/md5.h"
#endif

#if !LWIP_TCP_PCB_NUM_EXT_ARGS
#error tcp_md5 needs LWIP_TCP_PCB_NUM_EXT_ARGS
#endif

#define LWIP_TCP_OPT_MD5          19 /* number of the md5 option */
#define LWIP_TCP_OPT_LEN_MD5      18 /* length of the md5 option */
#define LWIP_TCP_OPT_LEN_MD5_OUT  20 /* 18 + alignment */

#define LWIP_TCP_MD5_DIGEST_LEN   16

/* This keeps the md5 state internally */
struct tcp_md5_conn_info {
  struct tcp_md5_conn_info *next;
  ip_addr_t remote_addr;
  u16_t remote_port;
  u8_t key[TCP_MD5SIG_MAXKEYLEN];
  u16_t key_len;
};

/* Callback function prototypes: */
static void tcp_md5_extarg_destroy(u8_t id, void *data);
static err_t tcp_md5_extarg_passive_open(u8_t id, struct tcp_pcb_listen *lpcb, struct tcp_pcb *cpcb);
/* Define our tcp ext arg callback structure: */
const struct tcp_ext_arg_callbacks tcp_md5_ext_arg_callbacks = {
  tcp_md5_extarg_destroy,
  tcp_md5_extarg_passive_open
};

static u8_t tcp_md5_extarg_id = LWIP_TCP_PCB_NUM_EXT_ARG_ID_INVALID;
static u8_t tcp_md5_opts_buf[40];

/** Initialize this module (allocates a tcp ext arg id) */
void
tcp_md5_init(void)
{
  tcp_md5_extarg_id = tcp_ext_arg_alloc_id();
}

/* Create a conn-info structure that holds the md5 state per connection */
static struct tcp_md5_conn_info *
tcp_md5_conn_info_alloc(void)
{
  return (struct tcp_md5_conn_info *)mem_malloc(sizeof(struct tcp_md5_conn_info));
}

/* Frees a conn-info structure that holds the md5 state per connection */
static void
tcp_md5_conn_info_free(struct tcp_md5_conn_info *info)
{
  mem_free(info);
}

/* A pcb is about to be destroyed. Free its extdata */
static void
tcp_md5_extarg_destroy(u8_t id, void *data)
{
  struct tcp_md5_conn_info *iter;

  LWIP_ASSERT("tcp_md5_extarg_id != LWIP_TCP_PCB_NUM_EXT_ARG_ID_INVALID",
    tcp_md5_extarg_id != LWIP_TCP_PCB_NUM_EXT_ARG_ID_INVALID);
  LWIP_ASSERT("id == tcp_md5_extarg_id", id == tcp_md5_extarg_id);
  LWIP_UNUSED_ARG(id);

  iter = (struct tcp_md5_conn_info *)data;
  while (iter != NULL) {
    struct tcp_md5_conn_info *info = iter;
    iter = iter->next;
    tcp_md5_conn_info_free(info);
  }
}

/* Try to find an md5 connection info for the specified remote connection */
static struct tcp_md5_conn_info *
tcp_md5_get_info(const struct tcp_pcb *pcb, const ip_addr_t *remote_ip, u16_t remote_port)
{
  if (pcb != NULL) {
    struct tcp_md5_conn_info *info = (struct tcp_md5_conn_info *)tcp_ext_arg_get(pcb, tcp_md5_extarg_id);
    while (info != NULL) {
      if (ip_addr_eq(&info->remote_addr, remote_ip)) {
        if (info->remote_port == remote_port) {
          return info;
        }
      }
      info = info->next;
    }
  }
  return NULL;
}

/* Passive open: copy md5 connection info from listen pcb to connection pcb
 * or return error (connection will be closed)
 */
static err_t
tcp_md5_extarg_passive_open(u8_t id, struct tcp_pcb_listen *lpcb, struct tcp_pcb *cpcb)
{
  struct tcp_md5_conn_info *iter;

  LWIP_ASSERT("lpcb != NULL", lpcb != NULL);
  LWIP_ASSERT("cpcb != NULL", cpcb != NULL);
  LWIP_ASSERT("tcp_md5_extarg_id != LWIP_TCP_PCB_NUM_EXT_ARG_ID_INVALID",
    tcp_md5_extarg_id != LWIP_TCP_PCB_NUM_EXT_ARG_ID_INVALID);
  LWIP_ASSERT("id == tcp_md5_extarg_id", id == tcp_md5_extarg_id);
  LWIP_UNUSED_ARG(id);

  iter = (struct tcp_md5_conn_info *)tcp_ext_arg_get((struct tcp_pcb *)lpcb, id);
  while (iter != NULL) {
    if (iter->remote_port == cpcb->remote_port) {
      if (ip_addr_eq(&iter->remote_addr, &cpcb->remote_ip)) {
        struct tcp_md5_conn_info *info = tcp_md5_conn_info_alloc();
        if (info != NULL) {
          memcpy(info, iter, sizeof(struct tcp_md5_conn_info));
          tcp_ext_arg_set(cpcb, id, info);
          tcp_ext_arg_set_callbacks(cpcb, id, &tcp_md5_ext_arg_callbacks);
          return ERR_OK;
        } else {
          return ERR_MEM;
        }
      }
    }
    iter = iter->next;
  }
  /* remote connection not found */
  return ERR_VAL;
}

/* Parse tcp header options and return 1 if an md5 signature option was found */
static int
tcp_md5_parseopt(const u8_t *opts, u16_t optlen, u8_t *md5_digest_out)
{
  u8_t data;
  u16_t optidx;

  /* Parse the TCP MSS option, if present. */
  if (optlen != 0) {
    for (optidx = 0; optidx < optlen; ) {
      u8_t opt = opts[optidx++];
      switch (opt) {
        case LWIP_TCP_OPT_EOL:
          /* End of options. */
          LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_parseopt: EOL\n"));
          return 0;
        case LWIP_TCP_OPT_NOP:
          /* NOP option. */
          LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_parseopt: NOP\n"));
          break;
        case LWIP_TCP_OPT_MD5:
          LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_parseopt: MD5\n"));
          if (opts[optidx++] != LWIP_TCP_OPT_LEN_MD5 || (optidx - 2 + LWIP_TCP_OPT_LEN_MD5) > optlen) {
            /* Bad length */
            LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_parseopt: bad length\n"));
            return 0;
          }
          /* An MD5 option with the right option length. */
          memcpy(md5_digest_out, &opts[optidx], LWIP_TCP_MD5_DIGEST_LEN);
          /* no need to process the options further */
          return 1;
          break;
        default:
          LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_parseopt: other\n"));
          data = opts[optidx++];
          if (data < 2) {
            LWIP_DEBUGF(TCP_INPUT_DEBUG, ("tcp_parseopt: bad length\n"));
            /* If the length field is zero, the options are malformed
               and we don't process them further. */
            return 0;
          }
          /* All other options have a length field, so that we easily
             can skip past them. */
          optidx += data - 2;
      }
    }
  }
  return 0;
}

/* Get tcp options into contiguous memory. May be required if input pbufs
 * are chained.
 */
static const u8_t*
tcp_md5_options_singlebuf(struct tcp_hdr *hdr, u16_t optlen, u16_t opt1len, u8_t *opt2)
{
  const u8_t *opts;
  LWIP_ASSERT("hdr != NULL", hdr != NULL);
  LWIP_ASSERT("optlen >= opt1len", optlen >= opt1len);
  opts = (const u8_t *)hdr + TCP_HLEN;
  if (optlen == opt1len) {
    /* arleady in one piece */
    return opts;
  }
  if (optlen > sizeof(tcp_md5_opts_buf)) {
    /* options too long */
    return NULL;
  }
  LWIP_ASSERT("opt2 != NULL", opt2 != NULL);
  /* copy first part */
  memcpy(tcp_md5_opts_buf, opts, opt1len);
  /* copy second part */
  memcpy(&tcp_md5_opts_buf[opt1len], opt2, optlen - opt1len);
  return tcp_md5_opts_buf;
}

/* Create the md5 digest for a given segment */
static int
tcp_md5_create_digest(const ip_addr_t *ip_src, const ip_addr_t *ip_dst, const struct tcp_hdr *hdr,
                      const u8_t *key, size_t key_len, u8_t *digest_out, struct pbuf *p)
{
  md5_context ctx;
  u8_t tmp8;
  u16_t tmp16;
  const size_t addr_len = IP_ADDR_RAW_SIZE(*ip_src);

  if (p != NULL) {
    LWIP_ASSERT("pbuf must not point to tcp header here!", (const void *)hdr != p->payload);
  }

  /* Generate the hash, using MD5. */
  md5_starts(&ctx);
  /* 1. the TCP pseudo-header (in the order: source IP address,
          destination IP address, zero-padded protocol number, and
          segment length) */
  md5_update(&ctx, (const unsigned char*)ip_src, addr_len);
  md5_update(&ctx, (const unsigned char*)ip_dst, addr_len);
  tmp8 = 0; /* zero-padded */
  md5_update(&ctx, &tmp8, 1);
  tmp8 = IP_PROTO_TCP;
  md5_update(&ctx, &tmp8, 1);
  tmp16 = lwip_htons(TCPH_HDRLEN_BYTES(hdr) + (p ? p->tot_len : 0));
  md5_update(&ctx, (const unsigned char*)&tmp16, 2);
  /* 2. the TCP header, excluding options, and assuming a checksum of
          zero */
  md5_update(&ctx, (const unsigned char*)hdr, sizeof(struct tcp_hdr));
  /* 3. the TCP segment data (if any) */
  if ((p != NULL) && (p->tot_len != 0)) {
    struct pbuf *q;
    for (q = p; q != NULL; q = q->next) {
      md5_update(&ctx, (const unsigned char*)q->payload, q->len);
    }
  }
  /* 4. an independently-specified key or password, known to both TCPs
          and presumably connection-specific */
  md5_update(&ctx, key, key_len);

  md5_finish(&ctx, digest_out);
  return 1;
}

/* Duplicate a tcp header and make sure the fields are in network byte order */
static void
tcp_md5_dup_tcphdr(struct tcp_hdr *tcphdr_copy, const struct tcp_hdr *tcphdr_in, int tcphdr_in_is_host_order)
{
  memcpy(tcphdr_copy, tcphdr_in, sizeof(struct tcp_hdr));
  tcphdr_copy->chksum = 0; /* checksum is zero for the pseudo header */
  if (tcphdr_in_is_host_order) {
    /* lwIP writes the TCP header values back to the buffer, we need to invert that here: */
    tcphdr_copy->src = lwip_htons(tcphdr_copy->src);
    tcphdr_copy->dest = lwip_htons(tcphdr_copy->dest);
    tcphdr_copy->seqno = lwip_htonl(tcphdr_copy->seqno);
    tcphdr_copy->ackno = lwip_htonl(tcphdr_copy->ackno);
    tcphdr_copy->wnd = lwip_htons(tcphdr_copy->wnd);
    tcphdr_copy->urgp = lwip_htons(tcphdr_copy->urgp);
  }
}

/* Check if md5 is enabled on a given pcb */
static int
tcp_md5_is_enabled_on_pcb(const struct tcp_pcb *pcb)
{
  if (tcp_md5_extarg_id != LWIP_TCP_PCB_NUM_EXT_ARG_ID_INVALID) {
    struct tcp_md5_conn_info *info = (struct tcp_md5_conn_info *)tcp_ext_arg_get(pcb, tcp_md5_extarg_id);
    if (info != NULL) {
      return 1;
    }
  }
  return 0;
}

/* Check if md5 is enabled on a given listen pcb */
static int
tcp_md5_is_enabled_on_lpcb(const struct tcp_pcb_listen *lpcb)
{
  /* same as for connection pcbs */
  return tcp_md5_is_enabled_on_pcb((const struct tcp_pcb *)lpcb);
}

/* Hook implementation for LWIP_HOOK_TCP_OPT_LENGTH_SEGMENT */
u8_t
tcp_md5_get_additional_option_length(const struct tcp_pcb *pcb, u8_t internal_option_length)
{
  if ((pcb != NULL) && tcp_md5_is_enabled_on_pcb(pcb)) {
    u8_t new_option_length = internal_option_length + LWIP_TCP_OPT_LEN_MD5_OUT;
    LWIP_ASSERT("overflow", new_option_length > internal_option_length);
    LWIP_ASSERT("options too long", new_option_length <= TCP_MAX_OPTION_BYTES);
    return new_option_length;
  }
  return internal_option_length;
}

/* Hook implementation for LWIP_HOOK_TCP_INPACKET_PCB when called for listen pcbs */
static err_t
tcp_md5_check_listen(struct tcp_pcb_listen* lpcb, struct tcp_hdr *hdr, u16_t optlen, u16_t opt1len, u8_t *opt2)
{
  LWIP_ASSERT("lpcb != NULL", lpcb != NULL);

  if (tcp_md5_is_enabled_on_lpcb(lpcb)) {
    const u8_t *opts;
    u8_t digest_received[LWIP_TCP_MD5_DIGEST_LEN];
    u8_t digest_calculated[LWIP_TCP_MD5_DIGEST_LEN];
    const struct tcp_md5_conn_info *info = tcp_md5_get_info((struct tcp_pcb *)lpcb, ip_current_src_addr(), hdr->src);
    if (info != NULL) {
      opts = tcp_md5_options_singlebuf(hdr, optlen, opt1len, opt2);
      if (opts != NULL) {
        if (tcp_md5_parseopt(opts, optlen, digest_received)) {
          struct tcp_hdr tcphdr_copy;
          tcp_md5_dup_tcphdr(&tcphdr_copy, hdr, 1);
          if (tcp_md5_create_digest(ip_current_src_addr(), ip_current_dest_addr(), &tcphdr_copy, info->key, info->key_len, digest_calculated, NULL)) {
            /* everything set up, compare the digests */
            if (!memcmp(digest_received, digest_calculated, LWIP_TCP_MD5_DIGEST_LEN)) {
              /* equal */
              return ERR_OK;
            }
            /* not equal */
          }
        }
      }
    }
    /* md5 enabled on this pcb but no match or other error -> fail */
    return ERR_VAL;
  }
  return ERR_OK;
}

/* Hook implementation for LWIP_HOOK_TCP_INPACKET_PCB */
err_t
tcp_md5_check_inpacket(struct tcp_pcb* pcb, struct tcp_hdr *hdr, u16_t optlen, u16_t opt1len, u8_t *opt2, struct pbuf *p)
{
  LWIP_ASSERT("pcb != NULL", pcb != NULL);

  if (pcb->state == LISTEN) {
    return tcp_md5_check_listen((struct tcp_pcb_listen *)pcb, hdr, optlen, opt1len, opt2);
  }

  if (tcp_md5_is_enabled_on_pcb(pcb)) {
    const struct tcp_md5_conn_info *info = tcp_md5_get_info(pcb, ip_current_src_addr(), hdr->src);
    if (info != NULL) {
      const u8_t *opts;
      u8_t digest_received[LWIP_TCP_MD5_DIGEST_LEN];
      u8_t digest_calculated[LWIP_TCP_MD5_DIGEST_LEN];
      opts = tcp_md5_options_singlebuf(hdr, optlen, opt1len, opt2);
      if (opts != NULL) {
        if (tcp_md5_parseopt(opts, optlen, digest_received)) {
          struct tcp_hdr hdr_copy;
          tcp_md5_dup_tcphdr(&hdr_copy, hdr, 1);
          if (tcp_md5_create_digest(&pcb->remote_ip, &pcb->local_ip, &hdr_copy, info->key, info->key_len, digest_calculated, p)) {
            /* everything set up, compare the digests */
            if (!memcmp(digest_received, digest_calculated, LWIP_TCP_MD5_DIGEST_LEN)) {
              /* equal */
              return ERR_OK;
            }
            /* not equal */
          }
        }
      }
    }
    /* md5 enabled on this pcb but no match or other error -> fail */
    return ERR_VAL;
  }
  return ERR_OK;
}

/* Hook implementation for LWIP_HOOK_TCP_ADD_TX_OPTIONS */
u32_t *
tcp_md5_add_tx_options(struct pbuf *p, struct tcp_hdr *hdr, const struct tcp_pcb *pcb, u32_t *opts)
{
  LWIP_ASSERT("p != NULL", p != NULL);
  LWIP_ASSERT("hdr != NULL", hdr != NULL);
  LWIP_ASSERT("pcb != NULL", pcb != NULL);
  LWIP_ASSERT("opts != NULL", opts != NULL);

  if (tcp_md5_is_enabled_on_pcb(pcb)) {
    u8_t digest_calculated[LWIP_TCP_MD5_DIGEST_LEN];
    u32_t *opts_ret = opts + 5; /* we use 20 bytes: 2 bytes padding + 18 bytes for this option */
    u8_t *ptr = (u8_t*)opts;

    const struct tcp_md5_conn_info *info = tcp_md5_get_info(pcb, &pcb->remote_ip, pcb->remote_port);
    if (info != NULL) {
      struct tcp_hdr hdr_copy;
      size_t hdrsize = TCPH_HDRLEN_BYTES(hdr);
      tcp_md5_dup_tcphdr(&hdr_copy, hdr, 0);
      /* p->payload points to the tcp header */
      LWIP_ASSERT("p->payload == hdr", p->payload == hdr);
      if (!pbuf_remove_header(p, hdrsize)) {
        u8_t ret;
        if (!tcp_md5_create_digest(&pcb->local_ip, &pcb->remote_ip, &hdr_copy, info->key, info->key_len, digest_calculated, p)) {
          info = NULL;
        }
        ret = pbuf_add_header_force(p, hdrsize);
        LWIP_ASSERT("tcp_md5_add_tx_options: pbuf_add_header_force failed", !ret);
        LWIP_UNUSED_ARG(ret);
      } else  {
        LWIP_ASSERT("error", 0);
      }
    }
    if (info == NULL) {
      /* create an invalid signature by zeroing the digest */
      memset(&digest_calculated, 0, sizeof(digest_calculated));
    }

    *ptr++ = LWIP_TCP_OPT_NOP;
    *ptr++ = LWIP_TCP_OPT_NOP;
    *ptr++ = LWIP_TCP_OPT_MD5;
    *ptr++ = LWIP_TCP_OPT_LEN_MD5;
    memcpy(ptr, digest_calculated, LWIP_TCP_MD5_DIGEST_LEN);
    ptr += LWIP_TCP_MD5_DIGEST_LEN;
    LWIP_ASSERT("ptr == opts_ret", ptr == (u8_t *)opts_ret);
    return opts_ret;
  }
  return opts;
}

/* Hook implementation for LWIP_HOOK_SOCKETS_SETSOCKOPT */
int
tcp_md5_setsockopt_hook(struct lwip_sock *sock, int level, int optname, const void *optval, socklen_t optlen, int *err)
{
  LWIP_ASSERT("sock != NULL", sock != NULL);
  LWIP_ASSERT("err != NULL", err != NULL);

  if ((level == IPPROTO_TCP) && (optname == TCP_MD5SIG)) {
    const struct tcp_md5sig *md5 = (const struct tcp_md5sig*)optval;
    if ((optval == NULL) || (optlen < sizeof(struct tcp_md5sig))) {
      *err = EINVAL;
    } else {
      if (sock->conn && (NETCONNTYPE_GROUP(netconn_type(sock->conn)) == NETCONN_TCP) && (sock->conn->pcb.tcp != NULL)) {
        if (tcp_md5_extarg_id == LWIP_TCP_PCB_NUM_EXT_ARG_ID_INVALID) {
          /* not initialized */
          *err = EINVAL;
        } else {
          struct tcp_md5_conn_info *info = tcp_md5_conn_info_alloc();
          if (info == NULL) {
            *err = ENOMEM;
          } else {
            int addr_valid = 0;
            /* OK, fill and link this request */
            memcpy(info->key, md5->tcpm_key, TCP_MD5SIG_MAXKEYLEN);
            info->key_len = md5->tcpm_keylen;
            memset(&info->remote_addr, 0, sizeof(info->remote_addr));
            if (md5->tcpm_addr.ss_family == AF_INET) {
#if LWIP_IPV4
              const struct sockaddr_in *sin = (const struct sockaddr_in *)&md5->tcpm_addr;
              memcpy(&info->remote_addr, &sin->sin_addr, sizeof(sin->sin_addr));
              IP_SET_TYPE_VAL(info->remote_addr, IPADDR_TYPE_V4);
              info->remote_port = lwip_htons(sin->sin_port);
              addr_valid = 1;
#endif /* LWIP_IPV4 */
            } else if (md5->tcpm_addr.ss_family == AF_INET6) {
#if LWIP_IPV6
              const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *)&md5->tcpm_addr;
              memcpy(&info->remote_addr, &sin6->sin6_addr, sizeof(sin6->sin6_addr));
              IP_SET_TYPE_VAL(info->remote_addr, IPADDR_TYPE_V6);
              info->remote_port = lwip_htons(sin6->sin6_port);
              addr_valid = 1;
#endif /* LWIP_IPV6 */
            }
            if (addr_valid) {
              /* store it */
              tcp_ext_arg_set_callbacks(sock->conn->pcb.tcp, tcp_md5_extarg_id, &tcp_md5_ext_arg_callbacks);
              info->next = (struct tcp_md5_conn_info *)tcp_ext_arg_get(sock->conn->pcb.tcp, tcp_md5_extarg_id);
              tcp_ext_arg_set(sock->conn->pcb.tcp, tcp_md5_extarg_id, info);
            } else {
              *err = EINVAL;
              tcp_md5_conn_info_free(info);
            }
          }
        }
      } else {
        /* not a tcp netconn */
        *err = EINVAL;
      }
    }
    return 1;
  }
  return 0;
}
