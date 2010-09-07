/**
 * @file
 * Transmission Control Protocol, outgoing traffic
 *
 * The output functions of TCP.
 *
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
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
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

#include "lwip/opt.h"

#if LWIP_TCP /* don't build if not configured for use in lwipopts.h */

#include "lwip/tcp.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/sys.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/inet.h"
#include "lwip/inet_chksum.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"

#include <string.h>

/* Forward declarations.*/
static void tcp_output_segment(struct tcp_seg *seg, struct tcp_pcb *pcb);

static struct tcp_hdr *
tcp_output_set_header(struct tcp_pcb *pcb, struct pbuf *p, int optlen,
                      u32_t seqno_be /* already in network byte order */)
{
  struct tcp_hdr *tcphdr = p->payload;
  tcphdr->src = htons(pcb->local_port);
  tcphdr->dest = htons(pcb->remote_port);
  tcphdr->seqno = seqno_be;
  tcphdr->ackno = htonl(pcb->rcv_nxt);
  TCPH_FLAGS_SET(tcphdr, TCP_ACK);
  tcphdr->wnd = htons(pcb->rcv_ann_wnd);
  tcphdr->urgp = 0;
  TCPH_HDRLEN_SET(tcphdr, (5 + optlen / 4));
  tcphdr->chksum = 0;

  /* If we're sending a packet, update the announced right window edge */
  pcb->rcv_ann_right_edge = pcb->rcv_nxt + pcb->rcv_ann_wnd;

  return tcphdr;
}

/**
 * Called by tcp_close() to send a segment including flags but not data.
 *
 * @param pcb the tcp_pcb over which to send a segment
 * @param flags the flags to set in the segment header
 * @return ERR_OK if sent, another err_t otherwise
 */
err_t
tcp_send_ctrl(struct tcp_pcb *pcb, u8_t flags)
{
  /* no data, no length, flags, copy=1, no optdata */
  return tcp_enqueue(pcb, NULL, 0, flags, TCP_WRITE_FLAG_COPY, 0);
}

/**
 * Write data for sending (but does not send it immediately).
 *
 * It waits in the expectation of more data being sent soon (as
 * it can send them more efficiently by combining them together).
 * To prompt the system to send data now, call tcp_output() after
 * calling tcp_write().
 * 
 * @param pcb Protocol control block of the TCP connection to enqueue data for.
 * @param data pointer to the data to send
 * @param len length (in bytes) of the data to send
 * @param apiflags combination of following flags :
 * - TCP_WRITE_FLAG_COPY (0x01) data will be copied into memory belonging to the stack
 * - TCP_WRITE_FLAG_MORE (0x02) for TCP connection, PSH flag will be set on last segment sent,
 * @return ERR_OK if enqueued, another err_t on error
 * 
 * @see tcp_write()
 */
err_t
tcp_write(struct tcp_pcb *pcb, const void *data, u16_t len, u8_t apiflags)
{
  LWIP_DEBUGF(TCP_OUTPUT_DEBUG, ("tcp_write(pcb=%p, data=%p, len=%"U16_F", apiflags=%"U16_F")\n", (void *)pcb,
    data, len, (u16_t)apiflags));
  /* connection is in valid state for data transmission? */
  if (pcb->state == ESTABLISHED ||
     pcb->state == CLOSE_WAIT ||
     pcb->state == SYN_SENT ||
     pcb->state == SYN_RCVD) {
    if (len > 0) {
#if LWIP_TCP_TIMESTAMPS
      return tcp_enqueue(pcb, (void *)data, len, 0, apiflags, 
                         pcb->flags & TF_TIMESTAMP ? TF_SEG_OPTS_TS : 0);
#else
      return tcp_enqueue(pcb, (void *)data, len, 0, apiflags, 0);
#endif
    }
    return ERR_OK;
  } else {
    LWIP_DEBUGF(TCP_OUTPUT_DEBUG | LWIP_DBG_STATE | LWIP_DBG_LEVEL_SEVERE, ("tcp_write() called in invalid state\n"));
    return ERR_CONN;
  }
}

/**
 * Enqueue data and/or TCP options for transmission
 *
 * Called by tcp_connect(), tcp_listen_input(), tcp_send_ctrl() and tcp_write().
 *
 * @param pcb Protocol control block for the TCP connection to enqueue data for.
 * @param arg Pointer to the data to be enqueued for sending.
 * @param len Data length in bytes
 * @param flags tcp header flags to set in the outgoing segment
 * @param apiflags combination of following flags :
 * - TCP_WRITE_FLAG_COPY (0x01) data will be copied into memory belonging to the stack
 * - TCP_WRITE_FLAG_MORE (0x02) for TCP connection, PSH flag will be set on last segment sent,
 * @param optflags options to include in segment later on (see definition of struct tcp_seg)
 */
err_t
tcp_enqueue(struct tcp_pcb *pcb, void *arg, u16_t len,
            u8_t flags, u8_t apiflags, u8_t optflags)
{
  struct pbuf *p;
  struct tcp_seg *seg, *useg, *queue;
  u32_t seqno;
  u16_t left, seglen;
  void *ptr;
  u16_t queuelen;
  u8_t optlen;

  LWIP_DEBUGF(TCP_OUTPUT_DEBUG, 
              ("tcp_enqueue(pcb=%p, arg=%p, len=%"U16_F", flags=%"X16_F", apiflags=%"U16_F")\n",
               (void *)pcb, arg, len, (u16_t)flags, (u16_t)apiflags));
  LWIP_ERROR("tcp_enqueue: packet needs payload, options, or SYN/FIN (programmer violates API)",
             ((len != 0) || (optflags != 0) || ((flags & (TCP_SYN | TCP_FIN)) != 0)),
             return ERR_ARG;);
  LWIP_ERROR("tcp_enqueue: len != 0 || arg == NULL (programmer violates API)", 
             ((len != 0) || (arg == NULL)), return ERR_ARG;);

  /* fail on too much data */
  if (len > pcb->snd_buf) {
    LWIP_DEBUGF(TCP_OUTPUT_DEBUG | LWIP_DBG_LEVEL_WARNING,
      ("tcp_enqueue: too much data (len=%"U16_F" > snd_buf=%"U16_F")\n", len, pcb->snd_buf));
    pcb->flags |= TF_NAGLEMEMERR;
    return ERR_MEM;
  }
  left = len;
  ptr = arg;

  optlen = LWIP_TCP_OPT_LENGTH(optflags);

  /* seqno will be the sequence number of the first segment enqueued
   * by the call to this function. */
  seqno = pcb->snd_lbb;

  LWIP_DEBUGF(TCP_QLEN_DEBUG, ("tcp_enqueue: queuelen: %"U16_F"\n", (u16_t)pcb->snd_queuelen));

  /* If total number of pbufs on the unsent/unacked queues exceeds the
   * configured maximum, return an error */
  queuelen = pcb->snd_queuelen;
  /* check for configured max queuelen and possible overflow */
  if ((queuelen >= TCP_SND_QUEUELEN) || (queuelen > TCP_SNDQUEUELEN_OVERFLOW)) {
    LWIP_DEBUGF(TCP_OUTPUT_DEBUG | LWIP_DBG_LEVEL_WARNING,
      ("tcp_enqueue: too long queue %"U16_F" (max %"U16_F")\n", queuelen, TCP_SND_QUEUELEN));
    TCP_STATS_INC(tcp.memerr);
    pcb->flags |= TF_NAGLEMEMERR;
    return ERR_MEM;
  }
  if (queuelen != 0) {
    LWIP_ASSERT("tcp_enqueue: pbufs on queue => at least one queue non-empty",
      pcb->unacked != NULL || pcb->unsent != NULL);
  } else {
    LWIP_ASSERT("tcp_enqueue: no pbufs on queue => both queues empty",
      pcb->unacked == NULL && pcb->unsent == NULL);
  }

  /* First, break up the data into segments and tuck them together in
   * the local "queue" variable. */
  useg = queue = seg = NULL;
  seglen = 0;
  while (queue == NULL || left > 0) {
    /* The segment length (including options) should be at most the MSS */
    seglen = left > (pcb->mss - optlen) ? (pcb->mss - optlen) : left;

    /* Allocate memory for tcp_seg, and fill in fields. */
    seg = memp_malloc(MEMP_TCP_SEG);
    if (seg == NULL) {
      LWIP_DEBUGF(TCP_OUTPUT_DEBUG | LWIP_DBG_LEVEL_SERIOUS, 
                  ("tcp_enqueue: could not allocate memory for tcp_seg\n"));
      goto memerr;
    }
    seg->next = NULL;
    seg->p = NULL;

    /* first segment of to-be-queued data? */
    if (queue == NULL) {
      queue = seg;
    }
    /* subsequent segments of to-be-queued data */
    else {
      /* Attach the segment to the end of the queued segments */
      LWIP_ASSERT("useg != NULL", useg != NULL);
      useg->next = seg;
    }
    /* remember last segment of to-be-queued data for next iteration */
    useg = seg;

    /* If copy is set, memory should be allocated
     * and data copied into pbuf, otherwise data comes from
     * ROM or other static memory, and need not be copied.  */
    if (apiflags & TCP_WRITE_FLAG_COPY) {
      if ((seg->p = pbuf_alloc(PBUF_TRANSPORT, seglen + optlen, PBUF_RAM)) == NULL) {
        LWIP_DEBUGF(TCP_OUTPUT_DEBUG | LWIP_DBG_LEVEL_SERIOUS, 
                    ("tcp_enqueue : could not allocate memory for pbuf copy size %"U16_F"\n", seglen));
        goto memerr;
      }
      LWIP_ASSERT("check that first pbuf can hold the complete seglen",
                  (seg->p->len >= seglen + optlen));
      queuelen += pbuf_clen(seg->p);
      if (arg != NULL) {
        MEMCPY((char *)seg->p->payload + optlen, ptr, seglen);
      }
      seg->dataptr = seg->p->payload;
    }
    /* do not copy data */
    else {
      /* First, allocate a pbuf for the headers. */
      if ((seg->p = pbuf_alloc(PBUF_TRANSPORT, optlen, PBUF_RAM)) == NULL) {
        LWIP_DEBUGF(TCP_OUTPUT_DEBUG | LWIP_DBG_LEVEL_SERIOUS, 
                    ("tcp_enqueue: could not allocate memory for header pbuf\n"));
        goto memerr;
      }
      queuelen += pbuf_clen(seg->p);

      /* Second, allocate a pbuf for holding the data.
       * since the referenced data is available at least until it is sent out on the
       * link (as it has to be ACKed by the remote party) we can safely use PBUF_ROM
       * instead of PBUF_REF here.
       */
      if (left > 0) {
        if ((p = pbuf_alloc(PBUF_RAW, seglen, PBUF_ROM)) == NULL) {
          /* If allocation fails, we have to deallocate the header pbuf as well. */
          pbuf_free(seg->p);
          seg->p = NULL;
          LWIP_DEBUGF(TCP_OUTPUT_DEBUG | LWIP_DBG_LEVEL_SERIOUS, 
                      ("tcp_enqueue: could not allocate memory for zero-copy pbuf\n"));
          goto memerr;
        }
        ++queuelen;
        /* reference the non-volatile payload data */
        p->payload = ptr;
        seg->dataptr = ptr;

        /* Concatenate the headers and data pbufs together. */
        pbuf_cat(seg->p/*header*/, p/*data*/);
        p = NULL;
      }
    }

    /* Now that there are more segments queued, we check again if the
    length of the queue exceeds the configured maximum or overflows. */
    if ((queuelen > TCP_SND_QUEUELEN) || (queuelen > TCP_SNDQUEUELEN_OVERFLOW)) {
      LWIP_DEBUGF(TCP_OUTPUT_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
        ("tcp_enqueue: queue too long %"U16_F" (%"U16_F")\n", queuelen, TCP_SND_QUEUELEN));
      goto memerr;
    }

    seg->len = seglen;

    /* build TCP header */
    if (pbuf_header(seg->p, TCP_HLEN)) {
      LWIP_DEBUGF(TCP_OUTPUT_DEBUG | LWIP_DBG_LEVEL_SERIOUS, ("tcp_enqueue: no room for TCP header in pbuf.\n"));
      TCP_STATS_INC(tcp.err);
      goto memerr;
    }
    seg->tcphdr = seg->p->payload;
    seg->tcphdr->src = htons(pcb->local_port);
    seg->tcphdr->dest = htons(pcb->remote_port);
    seg->tcphdr->seqno = htonl(seqno);
    seg->tcphdr->urgp = 0;
    TCPH_FLAGS_SET(seg->tcphdr, flags);
    /* don't fill in tcphdr->ackno and tcphdr->wnd until later */

    seg->flags = optflags;

    /* Set the length of the header */
    TCPH_HDRLEN_SET(seg->tcphdr, (5 + optlen / 4));
    LWIP_DEBUGF(TCP_OUTPUT_DEBUG | LWIP_DBG_TRACE, ("tcp_enqueue: queueing %"U32_F":%"U32_F" (0x%"X16_F")\n",
      ntohl(seg->tcphdr->seqno),
      ntohl(seg->tcphdr->seqno) + TCP_TCPLEN(seg),
      (u16_t)flags));

    left -= seglen;
    seqno += seglen;
    ptr = (void *)((u8_t *)ptr + seglen);
  }

  /* Now that the data to be enqueued has been broken up into TCP
  segments in the queue variable, we add them to the end of the
  pcb->unsent queue. */
  if (pcb->unsent == NULL) {
    useg = NULL;
  }
  else {
    for (useg = pcb->unsent; useg->next != NULL; useg = useg->next);
  }
  /* { useg is last segment on the unsent queue, NULL if list is empty } */

  /* If there is room in the last pbuf on the unsent queue,
  chain the first pbuf on the queue together with that. */
  if (useg != NULL &&
    TCP_TCPLEN(useg) != 0 &&
    !(TCPH_FLAGS(useg->tcphdr) & (TCP_SYN | TCP_FIN)) &&
    (!(flags & (TCP_SYN | TCP_FIN)) || (flags == TCP_FIN)) &&
    /* fit within max seg size */
    (useg->len + queue->len <= pcb->mss) &&
    /* only concatenate segments with the same options */
    (useg->flags == queue->flags) &&
    /* segments are consecutive */
    (ntohl(useg->tcphdr->seqno) + useg->len == ntohl(queue->tcphdr->seqno)) ) {
    /* Remove TCP header from first segment of our to-be-queued list */
    if(pbuf_header(queue->p, -(TCP_HLEN + optlen))) {
      /* Can we cope with this failing?  Just assert for now */
      LWIP_ASSERT("pbuf_header failed\n", 0);
      TCP_STATS_INC(tcp.err);
      goto memerr;
    }
    if (queue->p->len == 0) {
      /* free the first (header-only) pbuf if it is now empty (contained only headers) */
      struct pbuf *old_q = queue->p;
      queue->p = queue->p->next;
      old_q->next = NULL;
      queuelen--;
      pbuf_free(old_q);
    }
    if (flags & TCP_FIN) {
      /* the new segment contains only FIN, no data -> put the FIN into the last segment */
      LWIP_ASSERT("FIN enqueued together with data", queue->p == NULL && queue->len == 0);
      TCPH_SET_FLAG(useg->tcphdr, TCP_FIN);
    } else {
      LWIP_ASSERT("zero-length pbuf", (queue->p != NULL) && (queue->p->len > 0));
      pbuf_cat(useg->p, queue->p);
      useg->len += queue->len;
      useg->next = queue->next;
    }

    LWIP_DEBUGF(TCP_OUTPUT_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_STATE, ("tcp_enqueue: chaining segments, new len %"U16_F"\n", useg->len));
    if (seg == queue) {
      seg = useg;
      seglen = useg->len;
    }
    memp_free(MEMP_TCP_SEG, queue);
  }
  else {
    /* empty list */
    if (useg == NULL) {
      /* initialize list with this segment */
      pcb->unsent = queue;
    }
    /* enqueue segment */
    else {
      useg->next = queue;
    }
  }
  if ((flags & TCP_SYN) || (flags & TCP_FIN)) {
    ++len;
  }
  if (flags & TCP_FIN) {
    pcb->flags |= TF_FIN;
  }
  pcb->snd_lbb += len;

  pcb->snd_buf -= len;

  /* update number of segments on the queues */
  pcb->snd_queuelen = queuelen;
  LWIP_DEBUGF(TCP_QLEN_DEBUG, ("tcp_enqueue: %"S16_F" (after enqueued)\n", pcb->snd_queuelen));
  if (pcb->snd_queuelen != 0) {
    LWIP_ASSERT("tcp_enqueue: valid queue length",
      pcb->unacked != NULL || pcb->unsent != NULL);
  }

  /* Set the PSH flag in the last segment that we enqueued, but only
  if the segment has data (indicated by seglen > 0). */
  if (seg != NULL && seglen > 0 && seg->tcphdr != NULL && ((apiflags & TCP_WRITE_FLAG_MORE)==0)) {
    TCPH_SET_FLAG(seg->tcphdr, TCP_PSH);
  }

  return ERR_OK;
memerr:
  pcb->flags |= TF_NAGLEMEMERR;
  TCP_STATS_INC(tcp.memerr);

  if (queue != NULL) {
    tcp_segs_free(queue);
  }
  if (pcb->snd_queuelen != 0) {
    LWIP_ASSERT("tcp_enqueue: valid queue length", pcb->unacked != NULL ||
      pcb->unsent != NULL);
  }
  LWIP_DEBUGF(TCP_QLEN_DEBUG | LWIP_DBG_STATE, ("tcp_enqueue: %"S16_F" (with mem err)\n", pcb->snd_queuelen));
  return ERR_MEM;
}


#if LWIP_TCP_TIMESTAMPS
/* Build a timestamp option (12 bytes long) at the specified options pointer)
 *
 * @param pcb tcp_pcb
 * @param opts option pointer where to store the timestamp option
 */
static void
tcp_build_timestamp_option(struct tcp_pcb *pcb, u32_t *opts)
{
  /* Pad with two NOP options to make everything nicely aligned */
  opts[0] = htonl(0x0101080A);
  opts[1] = htonl(sys_now());
  opts[2] = htonl(pcb->ts_recent);
}
#endif

/** Send an ACK without data.
 *
 * @param pcb Protocol control block for the TCP connection to send the ACK
 */
err_t
tcp_send_empty_ack(struct tcp_pcb *pcb)
{
  struct pbuf *p;
  struct tcp_hdr *tcphdr;
  u8_t optlen = 0;

#if LWIP_TCP_TIMESTAMPS
  if (pcb->flags & TF_TIMESTAMP) {
    optlen = LWIP_TCP_OPT_LENGTH(TF_SEG_OPTS_TS);
  }
#endif
  p = pbuf_alloc(PBUF_IP, TCP_HLEN + optlen, PBUF_RAM);
  if (p == NULL) {
    LWIP_DEBUGF(TCP_OUTPUT_DEBUG, ("tcp_output: (ACK) could not allocate pbuf\n"));
    return ERR_BUF;
  }
  LWIP_DEBUGF(TCP_OUTPUT_DEBUG, 
              ("tcp_output: sending ACK for %"U32_F"\n", pcb->rcv_nxt));
  /* remove ACK flags from the PCB, as we send an empty ACK now */
  pcb->flags &= ~(TF_ACK_DELAY | TF_ACK_NOW);

  tcphdr = tcp_output_set_header(pcb, p, optlen, htonl(pcb->snd_nxt));

  /* NB. MSS option is only sent on SYNs, so ignore it here */
#if LWIP_TCP_TIMESTAMPS
  pcb->ts_lastacksent = pcb->rcv_nxt;

  if (pcb->flags & TF_TIMESTAMP) {
    tcp_build_timestamp_option(pcb, (u32_t *)(tcphdr + 1));
  }
#endif 

#if CHECKSUM_GEN_TCP
  tcphdr->chksum = inet_chksum_pseudo(p, &(pcb->local_ip), &(pcb->remote_ip),
        IP_PROTO_TCP, p->tot_len);
#endif
#if LWIP_NETIF_HWADDRHINT
  ip_output_hinted(p, &(pcb->local_ip), &(pcb->remote_ip), pcb->ttl, pcb->tos,
      IP_PROTO_TCP, &(pcb->addr_hint));
#else /* LWIP_NETIF_HWADDRHINT*/
  ip_output(p, &(pcb->local_ip), &(pcb->remote_ip), pcb->ttl, pcb->tos,
      IP_PROTO_TCP);
#endif /* LWIP_NETIF_HWADDRHINT*/
  pbuf_free(p);

  return ERR_OK;
}

/**
 * Find out what we can send and send it
 *
 * @param pcb Protocol control block for the TCP connection to send data
 * @return ERR_OK if data has been sent or nothing to send
 *         another err_t on error
 */
err_t
tcp_output(struct tcp_pcb *pcb)
{
  struct tcp_seg *seg, *useg;
  u32_t wnd, snd_nxt;
#if TCP_CWND_DEBUG
  s16_t i = 0;
#endif /* TCP_CWND_DEBUG */

  /* First, check if we are invoked by the TCP input processing
     code. If so, we do not output anything. Instead, we rely on the
     input processing code to call us when input processing is done
     with. */
  if (tcp_input_pcb == pcb) {
    return ERR_OK;
  }

  wnd = LWIP_MIN(pcb->snd_wnd, pcb->cwnd);

  seg = pcb->unsent;

  /* If the TF_ACK_NOW flag is set and no data will be sent (either
   * because the ->unsent queue is empty or because the window does
   * not allow it), construct an empty ACK segment and send it.
   *
   * If data is to be sent, we will just piggyback the ACK (see below).
   */
  if (pcb->flags & TF_ACK_NOW &&
     (seg == NULL ||
      ntohl(seg->tcphdr->seqno) - pcb->lastack + seg->len > wnd)) {
     return tcp_send_empty_ack(pcb);
  }

  /* useg should point to last segment on unacked queue */
  useg = pcb->unacked;
  if (useg != NULL) {
    for (; useg->next != NULL; useg = useg->next);
  }

#if TCP_OUTPUT_DEBUG
  if (seg == NULL) {
    LWIP_DEBUGF(TCP_OUTPUT_DEBUG, ("tcp_output: nothing to send (%p)\n",
                                   (void*)pcb->unsent));
  }
#endif /* TCP_OUTPUT_DEBUG */
#if TCP_CWND_DEBUG
  if (seg == NULL) {
    LWIP_DEBUGF(TCP_CWND_DEBUG, ("tcp_output: snd_wnd %"U16_F
                                 ", cwnd %"U16_F", wnd %"U32_F
                                 ", seg == NULL, ack %"U32_F"\n",
                                 pcb->snd_wnd, pcb->cwnd, wnd, pcb->lastack));
  } else {
    LWIP_DEBUGF(TCP_CWND_DEBUG, 
                ("tcp_output: snd_wnd %"U16_F", cwnd %"U16_F", wnd %"U32_F
                 ", effwnd %"U32_F", seq %"U32_F", ack %"U32_F"\n",
                 pcb->snd_wnd, pcb->cwnd, wnd,
                 ntohl(seg->tcphdr->seqno) - pcb->lastack + seg->len,
                 ntohl(seg->tcphdr->seqno), pcb->lastack));
  }
#endif /* TCP_CWND_DEBUG */
  /* data available and window allows it to be sent? */
  while (seg != NULL &&
         ntohl(seg->tcphdr->seqno) - pcb->lastack + seg->len <= wnd) {
    LWIP_ASSERT("RST not expected here!", 
                (TCPH_FLAGS(seg->tcphdr) & TCP_RST) == 0);
    /* Stop sending if the nagle algorithm would prevent it
     * Don't stop:
     * - if tcp_enqueue had a memory error before (prevent delayed ACK timeout) or
     * - if FIN was already enqueued for this PCB (SYN is always alone in a segment -
     *   either seg->next != NULL or pcb->unacked == NULL;
     *   RST is no sent using tcp_enqueue/tcp_output.
     */
    if((tcp_do_output_nagle(pcb) == 0) &&
      ((pcb->flags & (TF_NAGLEMEMERR | TF_FIN)) == 0)){
      break;
    }
#if TCP_CWND_DEBUG
    LWIP_DEBUGF(TCP_CWND_DEBUG, ("tcp_output: snd_wnd %"U16_F", cwnd %"U16_F", wnd %"U32_F", effwnd %"U32_F", seq %"U32_F", ack %"U32_F", i %"S16_F"\n",
                            pcb->snd_wnd, pcb->cwnd, wnd,
                            ntohl(seg->tcphdr->seqno) + seg->len -
                            pcb->lastack,
                            ntohl(seg->tcphdr->seqno), pcb->lastack, i));
    ++i;
#endif /* TCP_CWND_DEBUG */

    pcb->unsent = seg->next;

    if (pcb->state != SYN_SENT) {
      TCPH_SET_FLAG(seg->tcphdr, TCP_ACK);
      pcb->flags &= ~(TF_ACK_DELAY | TF_ACK_NOW);
    }

    tcp_output_segment(seg, pcb);
    snd_nxt = ntohl(seg->tcphdr->seqno) + TCP_TCPLEN(seg);
    if (TCP_SEQ_LT(pcb->snd_nxt, snd_nxt)) {
      pcb->snd_nxt = snd_nxt;
    }
    /* put segment on unacknowledged list if length > 0 */
    if (TCP_TCPLEN(seg) > 0) {
      seg->next = NULL;
      /* unacked list is empty? */
      if (pcb->unacked == NULL) {
        pcb->unacked = seg;
        useg = seg;
      /* unacked list is not empty? */
      } else {
        /* In the case of fast retransmit, the packet should not go to the tail
         * of the unacked queue, but rather somewhere before it. We need to check for
         * this case. -STJ Jul 27, 2004 */
        if (TCP_SEQ_LT(ntohl(seg->tcphdr->seqno), ntohl(useg->tcphdr->seqno))){
          /* add segment to before tail of unacked list, keeping the list sorted */
          struct tcp_seg **cur_seg = &(pcb->unacked);
          while (*cur_seg &&
            TCP_SEQ_LT(ntohl((*cur_seg)->tcphdr->seqno), ntohl(seg->tcphdr->seqno))) {
              cur_seg = &((*cur_seg)->next );
          }
          seg->next = (*cur_seg);
          (*cur_seg) = seg;
        } else {
          /* add segment to tail of unacked list */
          useg->next = seg;
          useg = useg->next;
        }
      }
    /* do not queue empty segments on the unacked list */
    } else {
      tcp_seg_free(seg);
    }
    seg = pcb->unsent;
  }

  if (seg != NULL && pcb->persist_backoff == 0 && 
      ntohl(seg->tcphdr->seqno) - pcb->lastack + seg->len > pcb->snd_wnd) {
    /* prepare for persist timer */
    pcb->persist_cnt = 0;
    pcb->persist_backoff = 1;
  }

  pcb->flags &= ~TF_NAGLEMEMERR;
  return ERR_OK;
}

/**
 * Called by tcp_output() to actually send a TCP segment over IP.
 *
 * @param seg the tcp_seg to send
 * @param pcb the tcp_pcb for the TCP connection used to send the segment
 */
static void
tcp_output_segment(struct tcp_seg *seg, struct tcp_pcb *pcb)
{
  u16_t len;
  struct netif *netif;
  u32_t *opts;

  /** @bug Exclude retransmitted segments from this count. */
  snmp_inc_tcpoutsegs();

  /* The TCP header has already been constructed, but the ackno and
   wnd fields remain. */
  seg->tcphdr->ackno = htonl(pcb->rcv_nxt);

  /* advertise our receive window size in this TCP segment */
  seg->tcphdr->wnd = htons(pcb->rcv_ann_wnd);

  pcb->rcv_ann_right_edge = pcb->rcv_nxt + pcb->rcv_ann_wnd;

  /* Add any requested options.  NB MSS option is only set on SYN
     packets, so ignore it here */
  opts = (u32_t *)(seg->tcphdr + 1);
  if (seg->flags & TF_SEG_OPTS_MSS) {
    TCP_BUILD_MSS_OPTION(*opts);
    opts += 1;
  }
#if LWIP_TCP_TIMESTAMPS
  pcb->ts_lastacksent = pcb->rcv_nxt;

  if (seg->flags & TF_SEG_OPTS_TS) {
    tcp_build_timestamp_option(pcb, opts);
    opts += 3;
  }
#endif

  /* If we don't have a local IP address, we get one by
     calling ip_route(). */
  if (ip_addr_isany(&(pcb->local_ip))) {
    netif = ip_route(&(pcb->remote_ip));
    if (netif == NULL) {
      return;
    }
    ip_addr_set(&(pcb->local_ip), &(netif->ip_addr));
  }

  /* Set retransmission timer running if it is not currently enabled */
  if(pcb->rtime == -1)
    pcb->rtime = 0;

  if (pcb->rttest == 0) {
    pcb->rttest = tcp_ticks;
    pcb->rtseq = ntohl(seg->tcphdr->seqno);

    LWIP_DEBUGF(TCP_RTO_DEBUG, ("tcp_output_segment: rtseq %"U32_F"\n", pcb->rtseq));
  }
  LWIP_DEBUGF(TCP_OUTPUT_DEBUG, ("tcp_output_segment: %"U32_F":%"U32_F"\n",
          htonl(seg->tcphdr->seqno), htonl(seg->tcphdr->seqno) +
          seg->len));

  len = (u16_t)((u8_t *)seg->tcphdr - (u8_t *)seg->p->payload);

  seg->p->len -= len;
  seg->p->tot_len -= len;

  seg->p->payload = seg->tcphdr;

  seg->tcphdr->chksum = 0;
#if CHECKSUM_GEN_TCP
  seg->tcphdr->chksum = inet_chksum_pseudo(seg->p,
             &(pcb->local_ip),
             &(pcb->remote_ip),
             IP_PROTO_TCP, seg->p->tot_len);
#endif
  TCP_STATS_INC(tcp.xmit);

#if LWIP_NETIF_HWADDRHINT
  ip_output_hinted(seg->p, &(pcb->local_ip), &(pcb->remote_ip), pcb->ttl, pcb->tos,
      IP_PROTO_TCP, &(pcb->addr_hint));
#else /* LWIP_NETIF_HWADDRHINT*/
  ip_output(seg->p, &(pcb->local_ip), &(pcb->remote_ip), pcb->ttl, pcb->tos,
      IP_PROTO_TCP);
#endif /* LWIP_NETIF_HWADDRHINT*/
}

/**
 * Send a TCP RESET packet (empty segment with RST flag set) either to
 * abort a connection or to show that there is no matching local connection
 * for a received segment.
 *
 * Called by tcp_abort() (to abort a local connection), tcp_input() (if no
 * matching local pcb was found), tcp_listen_input() (if incoming segment
 * has ACK flag set) and tcp_process() (received segment in the wrong state)
 *
 * Since a RST segment is in most cases not sent for an active connection,
 * tcp_rst() has a number of arguments that are taken from a tcp_pcb for
 * most other segment output functions.
 *
 * @param seqno the sequence number to use for the outgoing segment
 * @param ackno the acknowledge number to use for the outgoing segment
 * @param local_ip the local IP address to send the segment from
 * @param remote_ip the remote IP address to send the segment to
 * @param local_port the local TCP port to send the segment from
 * @param remote_port the remote TCP port to send the segment to
 */
void
tcp_rst(u32_t seqno, u32_t ackno,
  struct ip_addr *local_ip, struct ip_addr *remote_ip,
  u16_t local_port, u16_t remote_port)
{
  struct pbuf *p;
  struct tcp_hdr *tcphdr;
  p = pbuf_alloc(PBUF_IP, TCP_HLEN, PBUF_RAM);
  if (p == NULL) {
      LWIP_DEBUGF(TCP_DEBUG, ("tcp_rst: could not allocate memory for pbuf\n"));
      return;
  }
  LWIP_ASSERT("check that first pbuf can hold struct tcp_hdr",
              (p->len >= sizeof(struct tcp_hdr)));

  tcphdr = p->payload;
  tcphdr->src = htons(local_port);
  tcphdr->dest = htons(remote_port);
  tcphdr->seqno = htonl(seqno);
  tcphdr->ackno = htonl(ackno);
  TCPH_FLAGS_SET(tcphdr, TCP_RST | TCP_ACK);
  tcphdr->wnd = htons(TCP_WND);
  tcphdr->urgp = 0;
  TCPH_HDRLEN_SET(tcphdr, 5);

  tcphdr->chksum = 0;
#if CHECKSUM_GEN_TCP
  tcphdr->chksum = inet_chksum_pseudo(p, local_ip, remote_ip,
              IP_PROTO_TCP, p->tot_len);
#endif
  TCP_STATS_INC(tcp.xmit);
  snmp_inc_tcpoutrsts();
   /* Send output with hardcoded TTL since we have no access to the pcb */
  ip_output(p, local_ip, remote_ip, TCP_TTL, 0, IP_PROTO_TCP);
  pbuf_free(p);
  LWIP_DEBUGF(TCP_RST_DEBUG, ("tcp_rst: seqno %"U32_F" ackno %"U32_F".\n", seqno, ackno));
}

/**
 * Requeue all unacked segments for retransmission
 *
 * Called by tcp_slowtmr() for slow retransmission.
 *
 * @param pcb the tcp_pcb for which to re-enqueue all unacked segments
 */
void
tcp_rexmit_rto(struct tcp_pcb *pcb)
{
  struct tcp_seg *seg;

  if (pcb->unacked == NULL) {
    return;
  }

  /* Move all unacked segments to the head of the unsent queue */
  for (seg = pcb->unacked; seg->next != NULL; seg = seg->next);
  /* concatenate unsent queue after unacked queue */
  seg->next = pcb->unsent;
  /* unsent queue is the concatenated queue (of unacked, unsent) */
  pcb->unsent = pcb->unacked;
  /* unacked queue is now empty */
  pcb->unacked = NULL;

  /* increment number of retransmissions */
  ++pcb->nrtx;

  /* Don't take any RTT measurements after retransmitting. */
  pcb->rttest = 0;

  /* Do the actual retransmission */
  tcp_output(pcb);
}

/**
 * Requeue the first unacked segment for retransmission
 *
 * Called by tcp_receive() for fast retramsmit.
 *
 * @param pcb the tcp_pcb for which to retransmit the first unacked segment
 */
void
tcp_rexmit(struct tcp_pcb *pcb)
{
  struct tcp_seg *seg;
  struct tcp_seg **cur_seg;

  if (pcb->unacked == NULL) {
    return;
  }

  /* Move the first unacked segment to the unsent queue */
  /* Keep the unsent queue sorted. */
  seg = pcb->unacked;
  pcb->unacked = seg->next;

  cur_seg = &(pcb->unsent);
  while (*cur_seg &&
    TCP_SEQ_LT(ntohl((*cur_seg)->tcphdr->seqno), ntohl(seg->tcphdr->seqno))) {
      cur_seg = &((*cur_seg)->next );
  }
  seg->next = *cur_seg;
  *cur_seg = seg;

  ++pcb->nrtx;

  /* Don't take any rtt measurements after retransmitting. */
  pcb->rttest = 0;

  /* Do the actual retransmission. */
  snmp_inc_tcpretranssegs();
  /* No need to call tcp_output: we are always called from tcp_input()
     and thus tcp_output directly returns. */
}


/**
 * Handle retransmission after three dupacks received
 *
 * @param pcb the tcp_pcb for which to retransmit the first unacked segment
 */
void 
tcp_rexmit_fast(struct tcp_pcb *pcb)
{
  if (pcb->unacked != NULL && !(pcb->flags & TF_INFR)) {
    /* This is fast retransmit. Retransmit the first unacked segment. */
    LWIP_DEBUGF(TCP_FR_DEBUG, 
                ("tcp_receive: dupacks %"U16_F" (%"U32_F
                 "), fast retransmit %"U32_F"\n",
                 (u16_t)pcb->dupacks, pcb->lastack,
                 ntohl(pcb->unacked->tcphdr->seqno)));
    tcp_rexmit(pcb);

    /* Set ssthresh to half of the minimum of the current
     * cwnd and the advertised window */
    if (pcb->cwnd > pcb->snd_wnd)
      pcb->ssthresh = pcb->snd_wnd / 2;
    else
      pcb->ssthresh = pcb->cwnd / 2;
    
    /* The minimum value for ssthresh should be 2 MSS */
    if (pcb->ssthresh < 2*pcb->mss) {
      LWIP_DEBUGF(TCP_FR_DEBUG, 
                  ("tcp_receive: The minimum value for ssthresh %"U16_F
                   " should be min 2 mss %"U16_F"...\n",
                   pcb->ssthresh, 2*pcb->mss));
      pcb->ssthresh = 2*pcb->mss;
    }
    
    pcb->cwnd = pcb->ssthresh + 3 * pcb->mss;
    pcb->flags |= TF_INFR;
  } 
}


/**
 * Send keepalive packets to keep a connection active although
 * no data is sent over it.
 *
 * Called by tcp_slowtmr()
 *
 * @param pcb the tcp_pcb for which to send a keepalive packet
 */
void
tcp_keepalive(struct tcp_pcb *pcb)
{
  struct pbuf *p;
  struct tcp_hdr *tcphdr;

  LWIP_DEBUGF(TCP_DEBUG, ("tcp_keepalive: sending KEEPALIVE probe to %"U16_F".%"U16_F".%"U16_F".%"U16_F"\n",
                          ip4_addr1(&pcb->remote_ip), ip4_addr2(&pcb->remote_ip),
                          ip4_addr3(&pcb->remote_ip), ip4_addr4(&pcb->remote_ip)));

  LWIP_DEBUGF(TCP_DEBUG, ("tcp_keepalive: tcp_ticks %"U32_F"   pcb->tmr %"U32_F" pcb->keep_cnt_sent %"U16_F"\n", 
                          tcp_ticks, pcb->tmr, pcb->keep_cnt_sent));
   
  p = pbuf_alloc(PBUF_IP, TCP_HLEN, PBUF_RAM);
   
  if(p == NULL) {
    LWIP_DEBUGF(TCP_DEBUG, 
                ("tcp_keepalive: could not allocate memory for pbuf\n"));
    return;
  }
  LWIP_ASSERT("check that first pbuf can hold struct tcp_hdr",
              (p->len >= sizeof(struct tcp_hdr)));

  tcphdr = tcp_output_set_header(pcb, p, 0, htonl(pcb->snd_nxt - 1));

#if CHECKSUM_GEN_TCP
  tcphdr->chksum = inet_chksum_pseudo(p, &pcb->local_ip, &pcb->remote_ip,
                                      IP_PROTO_TCP, p->tot_len);
#endif
  TCP_STATS_INC(tcp.xmit);

  /* Send output to IP */
#if LWIP_NETIF_HWADDRHINT
  ip_output_hinted(p, &pcb->local_ip, &pcb->remote_ip, pcb->ttl, 0, IP_PROTO_TCP,
    &(pcb->addr_hint));
#else /* LWIP_NETIF_HWADDRHINT*/
  ip_output(p, &pcb->local_ip, &pcb->remote_ip, pcb->ttl, 0, IP_PROTO_TCP);
#endif /* LWIP_NETIF_HWADDRHINT*/

  pbuf_free(p);

  LWIP_DEBUGF(TCP_DEBUG, ("tcp_keepalive: seqno %"U32_F" ackno %"U32_F".\n",
                          pcb->snd_nxt - 1, pcb->rcv_nxt));
}


/**
 * Send persist timer zero-window probes to keep a connection active
 * when a window update is lost.
 *
 * Called by tcp_slowtmr()
 *
 * @param pcb the tcp_pcb for which to send a zero-window probe packet
 */
void
tcp_zero_window_probe(struct tcp_pcb *pcb)
{
  struct pbuf *p;
  struct tcp_hdr *tcphdr;
  struct tcp_seg *seg;
  u16_t len;
  u8_t is_fin;

  LWIP_DEBUGF(TCP_DEBUG, 
              ("tcp_zero_window_probe: sending ZERO WINDOW probe to %"
               U16_F".%"U16_F".%"U16_F".%"U16_F"\n",
               ip4_addr1(&pcb->remote_ip), ip4_addr2(&pcb->remote_ip),
               ip4_addr3(&pcb->remote_ip), ip4_addr4(&pcb->remote_ip)));

  LWIP_DEBUGF(TCP_DEBUG, 
              ("tcp_zero_window_probe: tcp_ticks %"U32_F
               "   pcb->tmr %"U32_F" pcb->keep_cnt_sent %"U16_F"\n", 
               tcp_ticks, pcb->tmr, pcb->keep_cnt_sent));

  seg = pcb->unacked;

  if(seg == NULL)
    seg = pcb->unsent;

  if(seg == NULL)
    return;

  is_fin = ((TCPH_FLAGS(seg->tcphdr) & TCP_FIN) != 0) && (seg->len == 0);
  len = is_fin ? TCP_HLEN : TCP_HLEN + 1;

  p = pbuf_alloc(PBUF_IP, len, PBUF_RAM);
  if(p == NULL) {
    LWIP_DEBUGF(TCP_DEBUG, ("tcp_zero_window_probe: no memory for pbuf\n"));
    return;
  }
  LWIP_ASSERT("check that first pbuf can hold struct tcp_hdr",
              (p->len >= sizeof(struct tcp_hdr)));

  tcphdr = tcp_output_set_header(pcb, p, 0, seg->tcphdr->seqno);

  if (is_fin) {
    /* FIN segment, no data */
    TCPH_FLAGS_SET(tcphdr, TCP_ACK | TCP_FIN);
  } else {
    /* Data segment, copy in one byte from the head of the unacked queue */
    *((char *)p->payload + sizeof(struct tcp_hdr)) = *(char *)seg->dataptr;
  }

#if CHECKSUM_GEN_TCP
  tcphdr->chksum = inet_chksum_pseudo(p, &pcb->local_ip, &pcb->remote_ip,
                                      IP_PROTO_TCP, p->tot_len);
#endif
  TCP_STATS_INC(tcp.xmit);

  /* Send output to IP */
#if LWIP_NETIF_HWADDRHINT
  ip_output_hinted(p, &pcb->local_ip, &pcb->remote_ip, pcb->ttl, 0, IP_PROTO_TCP,
    &(pcb->addr_hint));
#else /* LWIP_NETIF_HWADDRHINT*/
  ip_output(p, &pcb->local_ip, &pcb->remote_ip, pcb->ttl, 0, IP_PROTO_TCP);
#endif /* LWIP_NETIF_HWADDRHINT*/

  pbuf_free(p);

  LWIP_DEBUGF(TCP_DEBUG, ("tcp_zero_window_probe: seqno %"U32_F
                          " ackno %"U32_F".\n",
                          pcb->snd_nxt - 1, pcb->rcv_nxt));
}
#endif /* LWIP_TCP */
