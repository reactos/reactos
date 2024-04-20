/**
 * @file
 *
 * Common 6LowPAN routines for IPv6. Uses ND tables for link-layer addressing. Fragments packets to 6LowPAN units.
 *
 * This implementation aims to conform to IEEE 802.15.4(-2015), RFC 4944 and RFC 6282.
 * @todo: RFC 6775.
 */

/*
 * Copyright (c) 2015 Inico Technologies Ltd.
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
 * Author: Ivan Delamer <delamer@inicotech.com>
 *
 *
 * Please coordinate changes and requests with Ivan Delamer
 * <delamer@inicotech.com>
 */

/**
 * @defgroup sixlowpan 6LoWPAN (RFC4944)
 * @ingroup netifs
 * 6LowPAN netif implementation
 */

#include "netif/lowpan6_common.h"

#if LWIP_IPV6

#include "lwip/ip.h"
#include "lwip/pbuf.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/udp.h"

#include <string.h>

/* Determine compression mode for unicast address. */
s8_t
lowpan6_get_address_mode(const ip6_addr_t *ip6addr, const struct lowpan6_link_addr *mac_addr)
{
  if (mac_addr->addr_len == 2) {
    if ((ip6addr->addr[2] == (u32_t)PP_HTONL(0x000000ff)) &&
        ((ip6addr->addr[3]  & PP_HTONL(0xffff0000)) == PP_NTOHL(0xfe000000))) {
      if ((ip6addr->addr[3]  & PP_HTONL(0x0000ffff)) == lwip_ntohl((mac_addr->addr[0] << 8) | mac_addr->addr[1])) {
        return 3;
      }
    }
  } else if (mac_addr->addr_len == 8) {
    if ((ip6addr->addr[2] == lwip_ntohl(((mac_addr->addr[0] ^ 2) << 24) | (mac_addr->addr[1] << 16) | mac_addr->addr[2] << 8 | mac_addr->addr[3])) &&
        (ip6addr->addr[3] == lwip_ntohl((mac_addr->addr[4] << 24) | (mac_addr->addr[5] << 16) | mac_addr->addr[6] << 8 | mac_addr->addr[7]))) {
      return 3;
    }
  }

  if ((ip6addr->addr[2] == PP_HTONL(0x000000ffUL)) &&
      ((ip6addr->addr[3]  & PP_HTONL(0xffff0000)) == PP_NTOHL(0xfe000000UL))) {
    return 2;
  }

  return 1;
}

#if LWIP_6LOWPAN_IPHC

/* Determine compression mode for multicast address. */
static s8_t
lowpan6_get_address_mode_mc(const ip6_addr_t *ip6addr)
{
  if ((ip6addr->addr[0] == PP_HTONL(0xff020000)) &&
      (ip6addr->addr[1] == 0) &&
      (ip6addr->addr[2] == 0) &&
      ((ip6addr->addr[3]  & PP_HTONL(0xffffff00)) == 0)) {
    return 3;
  } else if (((ip6addr->addr[0] & PP_HTONL(0xff00ffff)) == PP_HTONL(0xff000000)) &&
             (ip6addr->addr[1] == 0)) {
    if ((ip6addr->addr[2] == 0) &&
        ((ip6addr->addr[3]  & PP_HTONL(0xff000000)) == 0)) {
      return 2;
    } else if ((ip6addr->addr[2]  & PP_HTONL(0xffffff00)) == 0) {
      return 1;
    }
  }

  return 0;
}

#if LWIP_6LOWPAN_NUM_CONTEXTS > 0
static s8_t
lowpan6_context_lookup(const ip6_addr_t *lowpan6_contexts, const ip6_addr_t *ip6addr)
{
  s8_t i;

  for (i = 0; i < LWIP_6LOWPAN_NUM_CONTEXTS; i++) {
    if (ip6_addr_net_eq(&lowpan6_contexts[i], ip6addr)) {
      return i;
    }
  }
  return -1;
}
#endif /* LWIP_6LOWPAN_NUM_CONTEXTS > 0 */

/*
 * Compress IPv6 and/or UDP headers.
 * */
err_t
lowpan6_compress_headers(struct netif *netif, u8_t *inbuf, size_t inbuf_size, u8_t *outbuf, size_t outbuf_size,
                         u8_t *lowpan6_header_len_out, u8_t *hidden_header_len_out, ip6_addr_t *lowpan6_contexts,
                         const struct lowpan6_link_addr *src, const struct lowpan6_link_addr *dst)
{
  u8_t *buffer, *inptr;
  u8_t lowpan6_header_len;
  u8_t hidden_header_len = 0;
  s8_t i;
  struct ip6_hdr *ip6hdr;
  ip_addr_t ip6src, ip6dst;

  LWIP_ASSERT("netif != NULL", netif != NULL);
  LWIP_ASSERT("inbuf != NULL", inbuf != NULL);
  LWIP_ASSERT("outbuf != NULL", outbuf != NULL);
  LWIP_ASSERT("lowpan6_header_len_out != NULL", lowpan6_header_len_out != NULL);
  LWIP_ASSERT("hidden_header_len_out != NULL", hidden_header_len_out != NULL);

  /* Perform 6LowPAN IPv6 header compression according to RFC 6282 */
  buffer = outbuf;
  inptr = inbuf;

  if (inbuf_size < IP6_HLEN) {
    /* input buffer too short */
    return ERR_VAL;
  }
  if (outbuf_size < IP6_HLEN) {
    /* output buffer too short for worst case */
    return ERR_MEM;
  }

  /* Point to ip6 header and align copies of src/dest addresses. */
  ip6hdr = (struct ip6_hdr *)inptr;
  ip_addr_copy_from_ip6_packed(ip6dst, ip6hdr->dest);
  ip6_addr_assign_zone(ip_2_ip6(&ip6dst), IP6_UNKNOWN, netif);
  ip_addr_copy_from_ip6_packed(ip6src, ip6hdr->src);
  ip6_addr_assign_zone(ip_2_ip6(&ip6src), IP6_UNKNOWN, netif);

  /* Basic length of 6LowPAN header, set dispatch and clear fields. */
  lowpan6_header_len = 2;
  buffer[0] = 0x60;
  buffer[1] = 0;

  /* Determine whether there will be a Context Identifier Extension byte or not.
   * If so, set it already. */
#if LWIP_6LOWPAN_NUM_CONTEXTS > 0
  buffer[2] = 0;

  i = lowpan6_context_lookup(lowpan6_contexts, ip_2_ip6(&ip6src));
  if (i >= 0) {
    /* Stateful source address compression. */
    buffer[1] |= 0x40;
    buffer[2] |= (i & 0x0f) << 4;
  }

  i = lowpan6_context_lookup(lowpan6_contexts, ip_2_ip6(&ip6dst));
  if (i >= 0) {
    /* Stateful destination address compression. */
    buffer[1] |= 0x04;
    buffer[2] |= i & 0x0f;
  }

  if (buffer[2] != 0x00) {
    /* Context identifier extension byte is appended. */
    buffer[1] |= 0x80;
    lowpan6_header_len++;
  }
#else /* LWIP_6LOWPAN_NUM_CONTEXTS > 0 */
  LWIP_UNUSED_ARG(lowpan6_contexts);
#endif /* LWIP_6LOWPAN_NUM_CONTEXTS > 0 */

  /* Determine TF field: Traffic Class, Flow Label */
  if (IP6H_FL(ip6hdr) == 0) {
    /* Flow label is elided. */
    buffer[0] |= 0x10;
    if (IP6H_TC(ip6hdr) == 0) {
      /* Traffic class (ECN+DSCP) elided too. */
      buffer[0] |= 0x08;
    } else {
      /* Traffic class (ECN+DSCP) appended. */
      buffer[lowpan6_header_len++] = IP6H_TC(ip6hdr);
    }
  } else {
    if (((IP6H_TC(ip6hdr) & 0x3f) == 0)) {
      /* DSCP portion of Traffic Class is elided, ECN and FL are appended (3 bytes) */
      buffer[0] |= 0x08;

      buffer[lowpan6_header_len] = IP6H_TC(ip6hdr) & 0xc0;
      buffer[lowpan6_header_len++] |= (IP6H_FL(ip6hdr) >> 16) & 0x0f;
      buffer[lowpan6_header_len++] = (IP6H_FL(ip6hdr) >> 8) & 0xff;
      buffer[lowpan6_header_len++] = IP6H_FL(ip6hdr) & 0xff;
    } else {
      /* Traffic class and flow label are appended (4 bytes) */
      buffer[lowpan6_header_len++] = IP6H_TC(ip6hdr);
      buffer[lowpan6_header_len++] = (IP6H_FL(ip6hdr) >> 16) & 0x0f;
      buffer[lowpan6_header_len++] = (IP6H_FL(ip6hdr) >> 8) & 0xff;
      buffer[lowpan6_header_len++] = IP6H_FL(ip6hdr) & 0xff;
    }
  }

  /* Compress NH?
  * Only if UDP for now. @todo support other NH compression. */
  if (IP6H_NEXTH(ip6hdr) == IP6_NEXTH_UDP) {
    buffer[0] |= 0x04;
  } else {
    /* append nexth. */
    buffer[lowpan6_header_len++] = IP6H_NEXTH(ip6hdr);
  }

  /* Compress hop limit? */
  if (IP6H_HOPLIM(ip6hdr) == 255) {
    buffer[0] |= 0x03;
  } else if (IP6H_HOPLIM(ip6hdr) == 64) {
    buffer[0] |= 0x02;
  } else if (IP6H_HOPLIM(ip6hdr) == 1) {
    buffer[0] |= 0x01;
  } else {
    /* append hop limit */
    buffer[lowpan6_header_len++] = IP6H_HOPLIM(ip6hdr);
  }

  /* Compress source address */
  if (((buffer[1] & 0x40) != 0) ||
      (ip6_addr_islinklocal(ip_2_ip6(&ip6src)))) {
    /* Context-based or link-local source address compression. */
    i = lowpan6_get_address_mode(ip_2_ip6(&ip6src), src);
    buffer[1] |= (i & 0x03) << 4;
    if (i == 1) {
      MEMCPY(buffer + lowpan6_header_len, inptr + 16, 8);
      lowpan6_header_len += 8;
    } else if (i == 2) {
      MEMCPY(buffer + lowpan6_header_len, inptr + 22, 2);
      lowpan6_header_len += 2;
    }
  } else if (ip6_addr_isany(ip_2_ip6(&ip6src))) {
    /* Special case: mark SAC and leave SAM=0 */
    buffer[1] |= 0x40;
  } else {
    /* Append full address. */
    MEMCPY(buffer + lowpan6_header_len, inptr + 8, 16);
    lowpan6_header_len += 16;
  }

  /* Compress destination address */
  if (ip6_addr_ismulticast(ip_2_ip6(&ip6dst))) {
    /* @todo support stateful multicast address compression */

    buffer[1] |= 0x08;

    i = lowpan6_get_address_mode_mc(ip_2_ip6(&ip6dst));
    buffer[1] |= i & 0x03;
    if (i == 0) {
      MEMCPY(buffer + lowpan6_header_len, inptr + 24, 16);
      lowpan6_header_len += 16;
    } else if (i == 1) {
      buffer[lowpan6_header_len++] = inptr[25];
      MEMCPY(buffer + lowpan6_header_len, inptr + 35, 5);
      lowpan6_header_len += 5;
    } else if (i == 2) {
      buffer[lowpan6_header_len++] = inptr[25];
      MEMCPY(buffer + lowpan6_header_len, inptr + 37, 3);
      lowpan6_header_len += 3;
    } else if (i == 3) {
      buffer[lowpan6_header_len++] = (inptr)[39];
    }
  } else if (((buffer[1] & 0x04) != 0) ||
              (ip6_addr_islinklocal(ip_2_ip6(&ip6dst)))) {
    /* Context-based or link-local destination address compression. */
    i = lowpan6_get_address_mode(ip_2_ip6(&ip6dst), dst);
    buffer[1] |= i & 0x03;
    if (i == 1) {
      MEMCPY(buffer + lowpan6_header_len, inptr + 32, 8);
      lowpan6_header_len += 8;
    } else if (i == 2) {
      MEMCPY(buffer + lowpan6_header_len, inptr + 38, 2);
      lowpan6_header_len += 2;
    }
  } else {
    /* Append full address. */
    MEMCPY(buffer + lowpan6_header_len, inptr + 24, 16);
    lowpan6_header_len += 16;
  }

  /* Move to payload. */
  inptr += IP6_HLEN;
  hidden_header_len += IP6_HLEN;

#if LWIP_UDP
  /* Compress UDP header? */
  if (IP6H_NEXTH(ip6hdr) == IP6_NEXTH_UDP) {
    /* @todo support optional checksum compression */

    if (inbuf_size < IP6_HLEN + UDP_HLEN) {
      /* input buffer too short */
      return ERR_VAL;
    }
    if (outbuf_size < (size_t)(hidden_header_len + 7)) {
      /* output buffer too short for worst case */
      return ERR_MEM;
    }

    buffer[lowpan6_header_len] = 0xf0;

    /* determine port compression mode. */
    if ((inptr[0] == 0xf0) && ((inptr[1] & 0xf0) == 0xb0) &&
        (inptr[2] == 0xf0) && ((inptr[3] & 0xf0) == 0xb0)) {
      /* Compress source and dest ports. */
      buffer[lowpan6_header_len++] |= 0x03;
      buffer[lowpan6_header_len++] = ((inptr[1] & 0x0f) << 4) | (inptr[3] & 0x0f);
    } else if (inptr[0] == 0xf0) {
      /* Compress source port. */
      buffer[lowpan6_header_len++] |= 0x02;
      buffer[lowpan6_header_len++] = inptr[1];
      buffer[lowpan6_header_len++] = inptr[2];
      buffer[lowpan6_header_len++] = inptr[3];
    } else if (inptr[2] == 0xf0) {
      /* Compress dest port. */
      buffer[lowpan6_header_len++] |= 0x01;
      buffer[lowpan6_header_len++] = inptr[0];
      buffer[lowpan6_header_len++] = inptr[1];
      buffer[lowpan6_header_len++] = inptr[3];
    } else {
      /* append full ports. */
      lowpan6_header_len++;
      buffer[lowpan6_header_len++] = inptr[0];
      buffer[lowpan6_header_len++] = inptr[1];
      buffer[lowpan6_header_len++] = inptr[2];
      buffer[lowpan6_header_len++] = inptr[3];
    }

    /* elide length and copy checksum */
    buffer[lowpan6_header_len++] = inptr[6];
    buffer[lowpan6_header_len++] = inptr[7];

    hidden_header_len += UDP_HLEN;
  }
#endif /* LWIP_UDP */

  *lowpan6_header_len_out = lowpan6_header_len;
  *hidden_header_len_out = hidden_header_len;

  return ERR_OK;
}

/** Decompress IPv6 and UDP headers compressed according to RFC 6282
 *
 * @param lowpan6_buffer compressed headers, first byte is the dispatch byte
 * @param lowpan6_bufsize size of lowpan6_buffer (may include data after headers)
 * @param decomp_buffer buffer where the decompressed headers are stored
 * @param decomp_bufsize size of decomp_buffer
 * @param hdr_size_comp returns the size of the compressed headers (skip to get to data)
 * @param hdr_size_decomp returns the size of the decompressed headers (IPv6 + UDP)
 * @param datagram_size datagram size from fragments or 0 if unfragmented
 * @param compressed_size compressed datagram size (for unfragmented rx)
 * @param lowpan6_contexts context addresses
 * @param src source address of the outer layer, used for address compression
 * @param dest destination address of the outer layer, used for address compression
 * @return ERR_OK if decompression succeeded, an error otherwise
 */
static err_t
lowpan6_decompress_hdr(u8_t *lowpan6_buffer, size_t lowpan6_bufsize,
                       u8_t *decomp_buffer, size_t decomp_bufsize,
                       u16_t *hdr_size_comp, u16_t *hdr_size_decomp,
                       u16_t datagram_size, u16_t compressed_size,
                       ip6_addr_t *lowpan6_contexts,
                       struct lowpan6_link_addr *src, struct lowpan6_link_addr *dest)
{
  u16_t lowpan6_offset;
  struct ip6_hdr *ip6hdr;
  s8_t i;
  u32_t header_temp;
  u16_t ip6_offset = IP6_HLEN;

  LWIP_ASSERT("lowpan6_buffer != NULL", lowpan6_buffer != NULL);
  LWIP_ASSERT("decomp_buffer != NULL", decomp_buffer != NULL);
  LWIP_ASSERT("src != NULL", src != NULL);
  LWIP_ASSERT("dest != NULL", dest != NULL);
  LWIP_ASSERT("hdr_size_comp != NULL", hdr_size_comp != NULL);
  LWIP_ASSERT("dehdr_size_decompst != NULL", hdr_size_decomp != NULL);

  ip6hdr = (struct ip6_hdr *)decomp_buffer;
  if (decomp_bufsize < IP6_HLEN) {
    return ERR_MEM;
  }

  /* output the full compressed packet, if set in @see lowpan6_opts.h */
#if LWIP_LOWPAN6_IP_COMPRESSED_DEBUG
  {
    u16_t j;
    LWIP_DEBUGF(LWIP_LOWPAN6_IP_COMPRESSED_DEBUG, ("lowpan6_decompress_hdr: IP6 payload (compressed): \n"));
    for (j = 0; j < lowpan6_bufsize; j++) {
      if ((j % 4) == 0) {
        LWIP_DEBUGF(LWIP_LOWPAN6_IP_COMPRESSED_DEBUG, ("\n"));
      }
      LWIP_DEBUGF(LWIP_LOWPAN6_IP_COMPRESSED_DEBUG, ("%2X ", lowpan6_buffer[j]));
    }
    LWIP_DEBUGF(LWIP_LOWPAN6_IP_COMPRESSED_DEBUG, ("\np->len: %d\n", lowpan6_bufsize));
  }
#endif

  /* offset for inline IP headers (RFC 6282 ch3)*/
  lowpan6_offset = 2;
  /* if CID is set (context identifier), the context byte
   * follows immediately after the header, so other IPHC fields are @+3 */
  if (lowpan6_buffer[1] & 0x80) {
    lowpan6_offset++;
  }

  /* Set IPv6 version, traffic class and flow label. (RFC6282, ch 3.1.1.)*/
  if ((lowpan6_buffer[0] & 0x18) == 0x00) {
    header_temp = ((lowpan6_buffer[lowpan6_offset+1] & 0x0f) << 16) | \
      (lowpan6_buffer[lowpan6_offset + 2] << 8) | lowpan6_buffer[lowpan6_offset+3];
    LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("TF: 00, ECN: 0x%"X8_F", Flowlabel+DSCP: 0x%8"X32_F"\n", \
      lowpan6_buffer[lowpan6_offset],header_temp));
    IP6H_VTCFL_SET(ip6hdr, 6, lowpan6_buffer[lowpan6_offset], header_temp);
    /* increase offset, processed 4 bytes here:
     * TF=00:  ECN + DSCP + 4-bit Pad + Flow Label (4 bytes)*/
    lowpan6_offset += 4;
  } else if ((lowpan6_buffer[0] & 0x18) == 0x08) {
    header_temp = ((lowpan6_buffer[lowpan6_offset] & 0x0f) << 16) | (lowpan6_buffer[lowpan6_offset + 1] << 8) | lowpan6_buffer[lowpan6_offset+2];
    LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("TF: 01, ECN: 0x%"X8_F", Flowlabel: 0x%2"X32_F", DSCP ignored\n", \
      lowpan6_buffer[lowpan6_offset] & 0xc0,header_temp));
    IP6H_VTCFL_SET(ip6hdr, 6, lowpan6_buffer[lowpan6_offset] & 0xc0, header_temp);
    /* increase offset, processed 3 bytes here:
     * TF=01:  ECN + 2-bit Pad + Flow Label (3 bytes), DSCP is elided.*/
    lowpan6_offset += 3;
  } else if ((lowpan6_buffer[0] & 0x18) == 0x10) {
    LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("TF: 10, DCSP+ECN: 0x%"X8_F", Flowlabel ignored\n", lowpan6_buffer[lowpan6_offset]));
    IP6H_VTCFL_SET(ip6hdr, 6, lowpan6_buffer[lowpan6_offset],0);
    /* increase offset, processed 1 byte here:
     * ECN + DSCP (1 byte), Flow Label is elided.*/
    lowpan6_offset += 1;
  } else if ((lowpan6_buffer[0] & 0x18) == 0x18) {
    LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("TF: 11, DCSP/ECN & Flowlabel ignored\n"));
    /* don't increase offset, no bytes processed here */
    IP6H_VTCFL_SET(ip6hdr, 6, 0, 0);
  }

  /* Set Next Header (NH) */
  if ((lowpan6_buffer[0] & 0x04) == 0x00) {
    /* 0: full next header byte carried inline (increase offset)*/
    LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("NH: 0x%2X\n", lowpan6_buffer[lowpan6_offset+1]));
    IP6H_NEXTH_SET(ip6hdr, lowpan6_buffer[lowpan6_offset++]);
  } else {
    /* 1: NH compression, LOWPAN_NHC (RFC6282, ch 4.1) */
    /* We should fill this later with NHC decoding */
    LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("NH: skipped, later done with NHC\n"));
    IP6H_NEXTH_SET(ip6hdr, 0);
  }

  /* Set Hop Limit, either carried inline or 3 different hops (1,64,255) */
  if ((lowpan6_buffer[0] & 0x03) == 0x00) {
    LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("Hops: full value: %d\n", lowpan6_buffer[lowpan6_offset+1]));
    IP6H_HOPLIM_SET(ip6hdr, lowpan6_buffer[lowpan6_offset++]);
  } else if ((lowpan6_buffer[0] & 0x03) == 0x01) {
    LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("Hops: compressed: 1\n"));
    IP6H_HOPLIM_SET(ip6hdr, 1);
  } else if ((lowpan6_buffer[0] & 0x03) == 0x02) {
    LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("Hops: compressed: 64\n"));
    IP6H_HOPLIM_SET(ip6hdr, 64);
  } else if ((lowpan6_buffer[0] & 0x03) == 0x03) {
    LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("Hops: compressed: 255\n"));
    IP6H_HOPLIM_SET(ip6hdr, 255);
  }

  /* Source address decoding. */
  if ((lowpan6_buffer[1] & 0x40) == 0x00) {
    /* Source address compression (SAC) = 0 -> stateless compression */
    LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("SAC == 0, no context byte\n"));
    /* Stateless compression */
    if ((lowpan6_buffer[1] & 0x30) == 0x00) {
      LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("SAM == 00, no src compression, fetching 128bits inline\n"));
      /* copy full address, increase offset by 16 Bytes */
      MEMCPY(&ip6hdr->src.addr[0], lowpan6_buffer + lowpan6_offset, 16);
      lowpan6_offset += 16;
    } else if ((lowpan6_buffer[1] & 0x30) == 0x10) {
      LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("SAM == 01, src compression, 64bits inline\n"));
      /* set 64 bits to link local */
      ip6hdr->src.addr[0] = PP_HTONL(0xfe800000UL);
      ip6hdr->src.addr[1] = 0;
      /* copy 8 Bytes, increase offset */
      MEMCPY(&ip6hdr->src.addr[2], lowpan6_buffer + lowpan6_offset, 8);
      lowpan6_offset += 8;
    } else if ((lowpan6_buffer[1] & 0x30) == 0x20) {
      LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("SAM == 10, src compression, 16bits inline\n"));
      /* set 96 bits to link local */
      ip6hdr->src.addr[0] = PP_HTONL(0xfe800000UL);
      ip6hdr->src.addr[1] = 0;
      ip6hdr->src.addr[2] = PP_HTONL(0x000000ffUL);
      /* extract remaining 16bits from inline bytes, increase offset */
      ip6hdr->src.addr[3] = lwip_htonl(0xfe000000UL | (lowpan6_buffer[lowpan6_offset] << 8) |
                                       lowpan6_buffer[lowpan6_offset + 1]);
      lowpan6_offset += 2;
    } else if ((lowpan6_buffer[1] & 0x30) == 0x30) {
      LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("SAM == 11, src compression, 0bits inline, using other headers\n"));
      /* no information available, using other layers, see RFC6282 ch 3.2.2 */
      ip6hdr->src.addr[0] = PP_HTONL(0xfe800000UL);
      ip6hdr->src.addr[1] = 0;
      if (src->addr_len == 2) {
        ip6hdr->src.addr[2] = PP_HTONL(0x000000ffUL);
        ip6hdr->src.addr[3] = lwip_htonl(0xfe000000UL | (src->addr[0] << 8) | src->addr[1]);
      } else if (src->addr_len == 8) {
        ip6hdr->src.addr[2] = lwip_htonl(((src->addr[0] ^ 2) << 24) | (src->addr[1] << 16) |
                                         (src->addr[2] << 8) | src->addr[3]);
        ip6hdr->src.addr[3] = lwip_htonl((src->addr[4] << 24) | (src->addr[5] << 16) |
                                         (src->addr[6] << 8) | src->addr[7]);
      } else {
        /* invalid source address length */
        LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("Invalid source address length\n"));
        return ERR_VAL;
      }
    }
  } else {
    /* Source address compression (SAC) = 1 -> stateful/context-based compression */
    LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("SAC == 1, additional context byte\n"));
    if ((lowpan6_buffer[1] & 0x30) == 0x00) {
      /* SAM=00, address=> :: (ANY) */
      ip6hdr->src.addr[0] = 0;
      ip6hdr->src.addr[1] = 0;
      ip6hdr->src.addr[2] = 0;
      ip6hdr->src.addr[3] = 0;
      LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("SAM == 00, context compression, ANY (::)\n"));
    } else {
      /* Set prefix from context info */
      if (lowpan6_buffer[1] & 0x80) {
        i = (lowpan6_buffer[2] >> 4) & 0x0f;
      } else {
        i = 0;
      }
      if (i >= LWIP_6LOWPAN_NUM_CONTEXTS) {
        /* Error */
        return ERR_VAL;
      }
#if LWIP_6LOWPAN_NUM_CONTEXTS > 0
      ip6hdr->src.addr[0] = lowpan6_contexts[i].addr[0];
      ip6hdr->src.addr[1] = lowpan6_contexts[i].addr[1];
      LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("SAM == xx, context compression found @%d: %8"X32_F", %8"X32_F"\n", (int)i, ip6hdr->src.addr[0], ip6hdr->src.addr[1]));
#else
      LWIP_UNUSED_ARG(lowpan6_contexts);
#endif
    }

    /* determine further address bits */
    if ((lowpan6_buffer[1] & 0x30) == 0x10) {
      /* SAM=01, load additional 64bits */
      MEMCPY(&ip6hdr->src.addr[2], lowpan6_buffer + lowpan6_offset, 8);
      LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("SAM == 01, context compression, 64bits inline\n"));
      lowpan6_offset += 8;
    } else if ((lowpan6_buffer[1] & 0x30) == 0x20) {
      /* SAM=01, load additional 16bits */
      ip6hdr->src.addr[2] = PP_HTONL(0x000000ffUL);
      ip6hdr->src.addr[3] = lwip_htonl(0xfe000000UL | (lowpan6_buffer[lowpan6_offset] << 8) | lowpan6_buffer[lowpan6_offset + 1]);
      LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("SAM == 10, context compression, 16bits inline\n"));
      lowpan6_offset += 2;
    } else if ((lowpan6_buffer[1] & 0x30) == 0x30) {
      /* SAM=11, address is fully elided, load from other layers */
      LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("SAM == 11, context compression, 0bits inline, using other headers\n"));
      if (src->addr_len == 2) {
        ip6hdr->src.addr[2] = PP_HTONL(0x000000ffUL);
        ip6hdr->src.addr[3] = lwip_htonl(0xfe000000UL | (src->addr[0] << 8) | src->addr[1]);
      } else if (src->addr_len == 8) {
        ip6hdr->src.addr[2] = lwip_htonl(((src->addr[0] ^ 2) << 24) | (src->addr[1] << 16) | (src->addr[2] << 8) | src->addr[3]);
        ip6hdr->src.addr[3] = lwip_htonl((src->addr[4] << 24) | (src->addr[5] << 16) | (src->addr[6] << 8) | src->addr[7]);
      } else {
        /* invalid source address length */
        LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("Invalid source address length\n"));
        return ERR_VAL;
      }
    }
  }

  /* Destination address decoding. */
  if (lowpan6_buffer[1] & 0x08) {
    LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("M=1: multicast\n"));
    /* Multicast destination */
    if (lowpan6_buffer[1] & 0x04) {
      LWIP_DEBUGF(LWIP_DBG_ON,("DAC == 1, context multicast: unsupported!!!\n"));
      /* @todo support stateful multicast addressing */
      return ERR_VAL;
    }

    if ((lowpan6_buffer[1] & 0x03) == 0x00) {
      /* DAM = 00, copy full address (128bits) */
      LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("DAM == 00, no dst compression, fetching 128bits inline\n"));
      MEMCPY(&ip6hdr->dest.addr[0], lowpan6_buffer + lowpan6_offset, 16);
      lowpan6_offset += 16;
    } else if ((lowpan6_buffer[1] & 0x03) == 0x01) {
      /* DAM = 01, copy 4 bytes (32bits) */
      LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("DAM == 01, dst address form (48bits): ffXX::00XX:XXXX:XXXX\n"));
      ip6hdr->dest.addr[0] = lwip_htonl(0xff000000UL | (lowpan6_buffer[lowpan6_offset++] << 16));
      ip6hdr->dest.addr[1] = 0;
      ip6hdr->dest.addr[2] = lwip_htonl(lowpan6_buffer[lowpan6_offset++]);
      ip6hdr->dest.addr[3] = lwip_htonl((lowpan6_buffer[lowpan6_offset] << 24) | (lowpan6_buffer[lowpan6_offset + 1] << 16) | (lowpan6_buffer[lowpan6_offset + 2] << 8) | lowpan6_buffer[lowpan6_offset + 3]);
      lowpan6_offset += 4;
    } else if ((lowpan6_buffer[1] & 0x03) == 0x02) {
      /* DAM = 10, copy 3 bytes (24bits) */
      LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("DAM == 10, dst address form (32bits): ffXX::00XX:XXXX\n"));
      ip6hdr->dest.addr[0] = lwip_htonl(0xff000000UL | (lowpan6_buffer[lowpan6_offset++] << 16));
      ip6hdr->dest.addr[1] = 0;
      ip6hdr->dest.addr[2] = 0;
      ip6hdr->dest.addr[3] = lwip_htonl((lowpan6_buffer[lowpan6_offset] << 16) | (lowpan6_buffer[lowpan6_offset + 1] << 8) | lowpan6_buffer[lowpan6_offset + 2]);
      lowpan6_offset += 3;
    } else if ((lowpan6_buffer[1] & 0x03) == 0x03) {
      /* DAM = 11, copy 1 byte (8bits) */
      LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("DAM == 11, dst address form (8bits): ff02::00XX\n"));
      ip6hdr->dest.addr[0] = PP_HTONL(0xff020000UL);
      ip6hdr->dest.addr[1] = 0;
      ip6hdr->dest.addr[2] = 0;
      ip6hdr->dest.addr[3] = lwip_htonl(lowpan6_buffer[lowpan6_offset++]);
    }

  } else {
    /* no Multicast (M=0) */
    if (lowpan6_buffer[1] & 0x04) {
      LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("DAC == 1, stateful compression\n"));
      /* Stateful destination compression */
      /* Set prefix from context info */
      if (lowpan6_buffer[1] & 0x80) {
        i = lowpan6_buffer[2] & 0x0f;
      } else {
        i = 0;
      }
      if (i >= LWIP_6LOWPAN_NUM_CONTEXTS) {
        /* Error */
        return ERR_VAL;
      }
#if LWIP_6LOWPAN_NUM_CONTEXTS > 0
      ip6hdr->dest.addr[0] = lowpan6_contexts[i].addr[0];
      ip6hdr->dest.addr[1] = lowpan6_contexts[i].addr[1];
#endif
    } else {
      LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("DAC == 0, stateless compression, setting link local prefix\n"));
      /* Link local address compression */
      ip6hdr->dest.addr[0] = PP_HTONL(0xfe800000UL);
      ip6hdr->dest.addr[1] = 0;
    }

    /* M=0, DAC=0, determining destination address length via DAM=xx */
    if ((lowpan6_buffer[1] & 0x03) == 0x00) {
      LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("DAM == 00, no dst compression, fetching 128bits inline\n"));
      /* DAM=00, copy full address */
      MEMCPY(&ip6hdr->dest.addr[0], lowpan6_buffer + lowpan6_offset, 16);
      lowpan6_offset += 16;
    } else if ((lowpan6_buffer[1] & 0x03) == 0x01) {
      LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("DAM == 01, dst compression, 64bits inline\n"));
      /* DAM=01, copy 64 inline bits, increase offset */
      MEMCPY(&ip6hdr->dest.addr[2], lowpan6_buffer + lowpan6_offset, 8);
      lowpan6_offset += 8;
    } else if ((lowpan6_buffer[1] & 0x03) == 0x02) {
      LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("DAM == 01, dst compression, 16bits inline\n"));
      /* DAM=10, copy 16 inline bits, increase offset */
      ip6hdr->dest.addr[2] = PP_HTONL(0x000000ffUL);
      ip6hdr->dest.addr[3] = lwip_htonl(0xfe000000UL | (lowpan6_buffer[lowpan6_offset] << 8) | lowpan6_buffer[lowpan6_offset + 1]);
      lowpan6_offset += 2;
    } else if ((lowpan6_buffer[1] & 0x03) == 0x03) {
      /* DAM=11, no bits available, use other headers (not done here) */
      LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG,("DAM == 01, dst compression, 0bits inline, using other headers\n"));
      if (dest->addr_len == 2) {
        ip6hdr->dest.addr[2] = PP_HTONL(0x000000ffUL);
        ip6hdr->dest.addr[3] = lwip_htonl(0xfe000000UL | (dest->addr[0] << 8) | dest->addr[1]);
      } else if (dest->addr_len == 8) {
        ip6hdr->dest.addr[2] = lwip_htonl(((dest->addr[0] ^ 2) << 24) | (dest->addr[1] << 16) | dest->addr[2] << 8 | dest->addr[3]);
        ip6hdr->dest.addr[3] = lwip_htonl((dest->addr[4] << 24) | (dest->addr[5] << 16) | dest->addr[6] << 8 | dest->addr[7]);
      } else {
        /* invalid destination address length */
        LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("Invalid destination address length\n"));
        return ERR_VAL;
      }
    }
  }


  /* Next Header Compression (NHC) decoding? */
  if (lowpan6_buffer[0] & 0x04) {
    LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("NHC decoding\n"));
#if LWIP_UDP
    if ((lowpan6_buffer[lowpan6_offset] & 0xf8) == 0xf0) {
      /* NHC: UDP */
      struct udp_hdr *udphdr;
      LWIP_DEBUGF(LWIP_LOWPAN6_DECOMPRESSION_DEBUG, ("NHC: UDP\n"));

      /* UDP compression */
      IP6H_NEXTH_SET(ip6hdr, IP6_NEXTH_UDP);
      udphdr = (struct udp_hdr *)((u8_t *)decomp_buffer + ip6_offset);
      if (decomp_bufsize < IP6_HLEN + UDP_HLEN) {
        return ERR_MEM;
      }

      /* Checksum decompression */
      if (lowpan6_buffer[lowpan6_offset] & 0x04) {
        /* @todo support checksum decompress */
        LWIP_DEBUGF(LWIP_DBG_ON, ("NHC: UDP chechsum decompression UNSUPPORTED\n"));
        return ERR_VAL;
      }

      /* Decompress ports, according to RFC4944 */
      i = lowpan6_buffer[lowpan6_offset++] & 0x03;
      if (i == 0) {
        udphdr->src = lwip_htons(lowpan6_buffer[lowpan6_offset] << 8 | lowpan6_buffer[lowpan6_offset + 1]);
        udphdr->dest = lwip_htons(lowpan6_buffer[lowpan6_offset + 2] << 8 | lowpan6_buffer[lowpan6_offset + 3]);
        lowpan6_offset += 4;
      } else if (i == 0x01) {
        udphdr->src = lwip_htons(lowpan6_buffer[lowpan6_offset] << 8 | lowpan6_buffer[lowpan6_offset + 1]);
        udphdr->dest = lwip_htons(0xf000 | lowpan6_buffer[lowpan6_offset + 2]);
        lowpan6_offset += 3;
      } else if (i == 0x02) {
        udphdr->src = lwip_htons(0xf000 | lowpan6_buffer[lowpan6_offset]);
        udphdr->dest = lwip_htons(lowpan6_buffer[lowpan6_offset + 1] << 8 | lowpan6_buffer[lowpan6_offset + 2]);
        lowpan6_offset += 3;
      } else if (i == 0x03) {
        udphdr->src = lwip_htons(0xf0b0 | ((lowpan6_buffer[lowpan6_offset] >> 4) & 0x0f));
        udphdr->dest = lwip_htons(0xf0b0 | (lowpan6_buffer[lowpan6_offset] & 0x0f));
        lowpan6_offset += 1;
      }

      udphdr->chksum = lwip_htons(lowpan6_buffer[lowpan6_offset] << 8 | lowpan6_buffer[lowpan6_offset + 1]);
      lowpan6_offset += 2;
      ip6_offset += UDP_HLEN;
      if (datagram_size == 0) {
        datagram_size = compressed_size - lowpan6_offset + ip6_offset;
      }
      udphdr->len = lwip_htons(datagram_size - IP6_HLEN);

    } else
#endif /* LWIP_UDP */
    {
      LWIP_DEBUGF(LWIP_DBG_ON,("NHC: unsupported protocol!\n"));
      /* @todo support NHC other than UDP */
      return ERR_VAL;
    }
  }
  if (datagram_size == 0) {
    datagram_size = compressed_size - lowpan6_offset + ip6_offset;
  }
  /* Infer IPv6 payload length for header */
  IP6H_PLEN_SET(ip6hdr, datagram_size - IP6_HLEN);

  if (lowpan6_offset > lowpan6_bufsize) {
    /* input buffer overflow */
    return ERR_VAL;
  }
  *hdr_size_comp = lowpan6_offset;
  *hdr_size_decomp = ip6_offset;

  return ERR_OK;
}

struct pbuf *
lowpan6_decompress(struct pbuf *p, u16_t datagram_size, ip6_addr_t *lowpan6_contexts,
                   struct lowpan6_link_addr *src, struct lowpan6_link_addr *dest)
{
  struct pbuf *q;
  u16_t lowpan6_offset, ip6_offset;
  err_t err;

#if LWIP_UDP
#define UDP_HLEN_ALLOC UDP_HLEN
#else
#define UDP_HLEN_ALLOC 0
#endif

  /* Allocate a buffer for decompression. This buffer will be too big and will be
     trimmed once the final size is known. */
  q = pbuf_alloc(PBUF_IP, p->len + IP6_HLEN + UDP_HLEN_ALLOC, PBUF_POOL);
  if (q == NULL) {
    pbuf_free(p);
    return NULL;
  }
  if (q->len < IP6_HLEN + UDP_HLEN_ALLOC) {
    /* The headers need to fit into the first pbuf */
    pbuf_free(p);
    pbuf_free(q);
    return NULL;
  }

  /* Decompress the IPv6 (and possibly UDP) header(s) into the new pbuf */
  err = lowpan6_decompress_hdr((u8_t *)p->payload, p->len, (u8_t *)q->payload, q->len,
    &lowpan6_offset, &ip6_offset, datagram_size, p->tot_len, lowpan6_contexts, src, dest);
  if (err != ERR_OK) {
    pbuf_free(p);
    pbuf_free(q);
    return NULL;
  }

  /* Now we copy leftover contents from p to q, so we have all L2 and L3 headers
     (and L4?) in a single pbuf: */

  /* Hide the compressed headers in p */
  pbuf_remove_header(p, lowpan6_offset);
  /* Temporarily hide the headers in q... */
  pbuf_remove_header(q, ip6_offset);
  /* ... copy the rest of p into q... */
  pbuf_copy(q, p);
  /* ... and reveal the headers again... */
  pbuf_add_header_force(q, ip6_offset);
  /* ... trim the pbuf to its correct size... */
  pbuf_realloc(q, ip6_offset + p->len);
  /* ... and cat possibly remaining (data-only) pbufs */
  if (p->next != NULL) {
    pbuf_cat(q, p->next);
  }
  /* the original (first) pbuf can now be freed */
  p->next = NULL;
  pbuf_free(p);

  /* all done */
  return q;
}

#endif /* LWIP_6LOWPAN_IPHC */
#endif /* LWIP_IPV6 */
