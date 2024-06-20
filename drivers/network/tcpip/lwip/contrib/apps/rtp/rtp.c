/**
 * @file
 * RTP client/server module
 *
 */

/*
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
 */

#include "lwip/opt.h"

#if LWIP_SOCKET && LWIP_IGMP /* don't build if not configured for use in lwipopts.h */

#include "lwip/sys.h"
#include "lwip/sockets.h"

#include "rtp.h"

#include "rtpdata.h"

#include <string.h>

/** This is an example of a "RTP" client/server based on a MPEG4 bitstream (with socket API).
 */

/**
 * RTP_DEBUG: Enable debugging for RTP.
 */
#ifndef RTP_DEBUG
#define RTP_DEBUG                   LWIP_DBG_ON
#endif

/** RTP stream port */
#ifndef RTP_STREAM_PORT
#define RTP_STREAM_PORT             4000
#endif

/** RTP stream multicast address as IPv4 address in "u32_t" format */
#ifndef RTP_STREAM_ADDRESS
#define RTP_STREAM_ADDRESS          inet_addr("232.0.0.0")
#endif

/** RTP send delay - in milliseconds */
#ifndef RTP_SEND_DELAY
#define RTP_SEND_DELAY              40
#endif

/** RTP receive timeout - in milliseconds */
#ifndef RTP_RECV_TIMEOUT
#define RTP_RECV_TIMEOUT            2000
#endif

/** RTP stats display period - in received packets */
#ifndef RTP_RECV_STATS
#define RTP_RECV_STATS              50
#endif

/** RTP macro to let the application process the data */
#ifndef RTP_RECV_PROCESSING
#define RTP_RECV_PROCESSING(p,s)
#endif

/** RTP packet/payload size */
#define RTP_PACKET_SIZE             1500
#define RTP_PAYLOAD_SIZE            1024

/** RTP header constants */
#define RTP_VERSION                 0x80
#define RTP_TIMESTAMP_INCREMENT     3600
#define RTP_SSRC                    0
#define RTP_PAYLOADTYPE             96
#define RTP_MARKER_MASK             0x80

/** RTP message header */
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/bpstruct.h"
#endif
PACK_STRUCT_BEGIN
struct rtp_hdr {
  PACK_STRUCT_FLD_8(u8_t  version);
  PACK_STRUCT_FLD_8(u8_t  payloadtype);
  PACK_STRUCT_FIELD(u16_t seqNum);
  PACK_STRUCT_FIELD(u32_t timestamp);
  PACK_STRUCT_FIELD(u32_t ssrc);
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END
#ifdef PACK_STRUCT_USE_INCLUDES
#  include "arch/epstruct.h"
#endif

/** RTP packets */
static u8_t rtp_send_packet[RTP_PACKET_SIZE];
static u8_t rtp_recv_packet[RTP_PACKET_SIZE];

/**
 * RTP send packets
 */
static void
rtp_send_packets( int sock, struct sockaddr_in* to)
{
  struct rtp_hdr* rtphdr;
  u8_t*           rtp_payload;
  size_t          rtp_payload_size;
  size_t          rtp_data_index;

  /* prepare RTP packet */
  rtphdr = (struct rtp_hdr*)rtp_send_packet;
  rtphdr->version     = RTP_VERSION;
  rtphdr->payloadtype = 0;
  rtphdr->ssrc        = PP_HTONL(RTP_SSRC);
  rtphdr->timestamp   = lwip_htonl(lwip_ntohl(rtphdr->timestamp) + RTP_TIMESTAMP_INCREMENT);

  /* send RTP stream packets */
  rtp_data_index = 0;
  do {
    rtp_payload      = rtp_send_packet+sizeof(struct rtp_hdr);
    rtp_payload_size = LWIP_MIN(RTP_PAYLOAD_SIZE, sizeof(rtp_data) - rtp_data_index);

    MEMCPY(rtp_payload, rtp_data + rtp_data_index, rtp_payload_size);

    /* set MARKER bit in RTP header on the last packet of an image */
    if ((rtp_data_index + rtp_payload_size) >= sizeof(rtp_data)) {
      rtphdr->payloadtype = RTP_PAYLOADTYPE | RTP_MARKER_MASK;
    } else {
      rtphdr->payloadtype = RTP_PAYLOADTYPE;
    }

    /* send RTP stream packet */
    if (lwip_sendto(sock, rtp_send_packet, sizeof(struct rtp_hdr) + rtp_payload_size,
        0, (struct sockaddr *)to, sizeof(struct sockaddr)) >= 0) {
      rtphdr->seqNum  = lwip_htons((u16_t)(lwip_ntohs(rtphdr->seqNum) + 1));
      rtp_data_index += rtp_payload_size;
    } else {
      LWIP_DEBUGF(RTP_DEBUG, ("rtp_sender: not sendto==%i\n", errno));
    }
  }while (rtp_data_index < sizeof(rtp_data));
}

/**
 * RTP send thread
 */
static void
rtp_send_thread(void *arg)
{
  int                sock;
  struct sockaddr_in local;
  struct sockaddr_in to;
  u32_t              rtp_stream_address;

  LWIP_UNUSED_ARG(arg);

  /* initialize RTP stream address */
  rtp_stream_address = RTP_STREAM_ADDRESS;

  /* if we got a valid RTP stream address... */
  if (rtp_stream_address != 0) {
    /* create new socket */
    sock = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    if (sock >= 0) {
      /* prepare local address */
      memset(&local, 0, sizeof(local));
      local.sin_family      = AF_INET;
      local.sin_port        = PP_HTONS(INADDR_ANY);
      local.sin_addr.s_addr = PP_HTONL(INADDR_ANY);

      /* bind to local address */
      if (lwip_bind(sock, (struct sockaddr *)&local, sizeof(local)) == 0) {
        /* prepare RTP stream address */
        memset(&to, 0, sizeof(to));
        to.sin_family      = AF_INET;
        to.sin_port        = PP_HTONS(RTP_STREAM_PORT);
        to.sin_addr.s_addr = rtp_stream_address;

        /* send RTP packets */
        memset(rtp_send_packet, 0, sizeof(rtp_send_packet));
        while (1) {
          rtp_send_packets( sock, &to);
          sys_msleep(RTP_SEND_DELAY);
        }
      }

      /* close the socket */
      lwip_close(sock);
    }
  }
}

/**
 * RTP recv thread
 */
static void
rtp_recv_thread(void *arg)
{
  int                sock;
  struct sockaddr_in local;
  struct sockaddr_in from;
  int                fromlen;
  struct ip_mreq     ipmreq;
  struct rtp_hdr*    rtphdr;
  u32_t              rtp_stream_address;
  int                timeout;
  int                result;
  int                recvrtppackets  = 0;
  int                lostrtppackets  = 0;
  u16_t              lastrtpseq = 0;

  LWIP_UNUSED_ARG(arg);

  /* initialize RTP stream address */
  rtp_stream_address = RTP_STREAM_ADDRESS;

  /* if we got a valid RTP stream address... */
  if (rtp_stream_address != 0) {
    /* create new socket */
    sock = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    if (sock >= 0) {
      /* prepare local address */
      memset(&local, 0, sizeof(local));
      local.sin_family      = AF_INET;
      local.sin_port        = PP_HTONS(RTP_STREAM_PORT);
      local.sin_addr.s_addr = PP_HTONL(INADDR_ANY);

      /* bind to local address */
      if (lwip_bind(sock, (struct sockaddr *)&local, sizeof(local)) == 0) {
        /* set recv timeout */
        timeout = RTP_RECV_TIMEOUT;
        result = lwip_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
        if (result) {
          LWIP_DEBUGF(RTP_DEBUG, ("rtp_recv_thread: setsockopt(SO_RCVTIMEO) failed: errno=%d\n", errno));
        }

        /* prepare multicast "ip_mreq" struct */
        ipmreq.imr_multiaddr.s_addr = rtp_stream_address;
        ipmreq.imr_interface.s_addr = PP_HTONL(INADDR_ANY);

        /* join multicast group */
        if (lwip_setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &ipmreq, sizeof(ipmreq)) == 0) {
          /* receive RTP packets */
          while(1) {
            fromlen = sizeof(from);
            result  = lwip_recvfrom(sock, rtp_recv_packet, sizeof(rtp_recv_packet), 0,
              (struct sockaddr *)&from, (socklen_t *)&fromlen);
            if ((result > 0) && ((size_t)result >= sizeof(struct rtp_hdr))) {
              size_t recved = (size_t)result;
              rtphdr = (struct rtp_hdr *)rtp_recv_packet;
              recvrtppackets++;
              if ((lastrtpseq == 0) || ((lastrtpseq + 1) == lwip_ntohs(rtphdr->seqNum))) {
                RTP_RECV_PROCESSING((rtp_recv_packet + sizeof(rtp_hdr)), (recved-sizeof(rtp_hdr)));
                LWIP_UNUSED_ARG(recved); /* just in case... */
              } else {
                lostrtppackets++;
              }
              lastrtpseq = lwip_ntohs(rtphdr->seqNum);
              if ((recvrtppackets % RTP_RECV_STATS) == 0) {
                LWIP_DEBUGF(RTP_DEBUG, ("rtp_recv_thread: recv %6i packet(s) / lost %4i packet(s) (%.4f%%)...\n", recvrtppackets, lostrtppackets, (lostrtppackets*100.0)/recvrtppackets));
              }
            } else {
              LWIP_DEBUGF(RTP_DEBUG, ("rtp_recv_thread: recv timeout...\n"));
            }
          }

          /* leave multicast group */
          /* TODO: this code is never reached
          result = lwip_setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &ipmreq, sizeof(ipmreq));
          if (result) {
            LWIP_DEBUGF(RTP_DEBUG, ("rtp_recv_thread: setsockopt(IP_DROP_MEMBERSHIP) failed: errno=%d\n", errno));
          }*/
        }
      }

      /* close the socket */
      lwip_close(sock);
    }
  }
}

void
rtp_init(void)
{
  sys_thread_new("rtp_send_thread", rtp_send_thread, NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
  sys_thread_new("rtp_recv_thread", rtp_recv_thread, NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
}

#endif /* LWIP_SOCKET && LWIP_IGMP */
