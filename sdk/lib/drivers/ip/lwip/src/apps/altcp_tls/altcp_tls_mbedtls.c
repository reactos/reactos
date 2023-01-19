/**
 * @file
 * Application layered TCP/TLS connection API (to be used from TCPIP thread)
 *
 * This file provides a TLS layer using mbedTLS
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
 * Watch out:
 * - 'sent' is always called with len==0 to the upper layer. This is because keeping
 *   track of the ratio of application data and TLS overhead would be too much.
 *
 * Mandatory security-related configuration:
 * - ensure to add at least one strong entropy source to your mbedtls port (implement
 *   mbedtls_platform_entropy_poll or mbedtls_hardware_poll providing strong entropy)
 * - define ALTCP_MBEDTLS_ENTROPY_PTR and ALTCP_MBEDTLS_ENTROPY_LEN to something providing
 *   GOOD custom entropy
 *
 * Missing things / @todo:
 * - some unhandled/untested things migh be caught by LWIP_ASSERTs...
 */

#include "lwip/opt.h"

#if LWIP_ALTCP /* don't build if not configured for use in lwipopts.h */

#include "lwip/apps/altcp_tls_mbedtls_opts.h"

#if LWIP_ALTCP_TLS && LWIP_ALTCP_TLS_MBEDTLS

#include "lwip/altcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/priv/altcp_priv.h"

#include "altcp_tls_mbedtls_structs.h"
#include "altcp_tls_mbedtls_mem.h"

/* @todo: which includes are really needed? */
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/certs.h"
#include "mbedtls/x509.h"
#include "mbedtls/ssl.h"
#include "mbedtls/net.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"
#include "mbedtls/platform.h"
#include "mbedtls/memory_buffer_alloc.h"
#include "mbedtls/ssl_cache.h"
#include "mbedtls/ssl_ticket.h"

#include "mbedtls/ssl_internal.h" /* to call mbedtls_flush_output after ERR_MEM */

#include <string.h>

#ifndef ALTCP_MBEDTLS_ENTROPY_PTR
#define ALTCP_MBEDTLS_ENTROPY_PTR   NULL
#endif
#ifndef ALTCP_MBEDTLS_ENTROPY_LEN
#define ALTCP_MBEDTLS_ENTROPY_LEN   0
#endif

/* Variable prototype, the actual declaration is at the end of this file
   since it contains pointers to static functions declared here */
extern const struct altcp_functions altcp_mbedtls_functions;

/** Our global mbedTLS configuration (server-specific, not connection-specific) */
struct altcp_tls_config {
  mbedtls_ssl_config conf;
  mbedtls_x509_crt *cert;
  mbedtls_pk_context *pkey;
  uint8_t cert_count;
  uint8_t cert_max;
  uint8_t pkey_count;
  uint8_t pkey_max;
  mbedtls_x509_crt *ca;
#if defined(MBEDTLS_SSL_CACHE_C) && ALTCP_MBEDTLS_USE_SESSION_CACHE
  /** Inter-connection cache for fast connection startup */
  struct mbedtls_ssl_cache_context cache;
#endif
#if defined(MBEDTLS_SSL_SESSION_TICKETS) && ALTCP_MBEDTLS_USE_SESSION_TICKETS
  mbedtls_ssl_ticket_context ticket_ctx;
#endif
};

/** Entropy and random generator are shared by all mbedTLS configuration */
struct altcp_tls_entropy_rng {
  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
  int ref;
};
static struct altcp_tls_entropy_rng *altcp_tls_entropy_rng;

static err_t altcp_mbedtls_lower_recv(void *arg, struct altcp_pcb *inner_conn, struct pbuf *p, err_t err);
static err_t altcp_mbedtls_setup(void *conf, struct altcp_pcb *conn, struct altcp_pcb *inner_conn);
static err_t altcp_mbedtls_lower_recv_process(struct altcp_pcb *conn, altcp_mbedtls_state_t *state);
static err_t altcp_mbedtls_handle_rx_appldata(struct altcp_pcb *conn, altcp_mbedtls_state_t *state);
static int altcp_mbedtls_bio_send(void *ctx, const unsigned char *dataptr, size_t size);


/* callback functions from inner/lower connection: */

/** Accept callback from lower connection (i.e. TCP)
 * Allocate one of our structures, assign it to the new connection's 'state' and
 * call the new connection's 'accepted' callback. If that succeeds, we wait
 * to receive connection setup handshake bytes from the client.
 */
static err_t
altcp_mbedtls_lower_accept(void *arg, struct altcp_pcb *accepted_conn, err_t err)
{
  struct altcp_pcb *listen_conn = (struct altcp_pcb *)arg;
  if (listen_conn && listen_conn->state && listen_conn->accept) {
    err_t setup_err;
    altcp_mbedtls_state_t *listen_state = (altcp_mbedtls_state_t *)listen_conn->state;
    /* create a new altcp_conn to pass to the next 'accept' callback */
    struct altcp_pcb *new_conn = altcp_alloc();
    if (new_conn == NULL) {
      return ERR_MEM;
    }
    setup_err = altcp_mbedtls_setup(listen_state->conf, new_conn, accepted_conn);
    if (setup_err != ERR_OK) {
      altcp_free(new_conn);
      return setup_err;
    }
    return listen_conn->accept(listen_conn->arg, new_conn, err);
  }
  return ERR_ARG;
}

/** Connected callback from lower connection (i.e. TCP).
 * Not really implemented/tested yet...
 */
static err_t
altcp_mbedtls_lower_connected(void *arg, struct altcp_pcb *inner_conn, err_t err)
{
  struct altcp_pcb *conn = (struct altcp_pcb *)arg;
  LWIP_UNUSED_ARG(inner_conn); /* for LWIP_NOASSERT */
  if (conn && conn->state) {
    LWIP_ASSERT("pcb mismatch", conn->inner_conn == inner_conn);
    /* upper connected is called when handshake is done */
    if (err != ERR_OK) {
      if (conn->connected) {
        return conn->connected(conn->arg, conn, err);
      }
    }
    return altcp_mbedtls_lower_recv_process(conn, (altcp_mbedtls_state_t *)conn->state);
  }
  return ERR_VAL;
}

/* Call recved for possibly more than an u16_t */
static void
altcp_mbedtls_lower_recved(struct altcp_pcb *inner_conn, int recvd_cnt)
{
  while (recvd_cnt > 0) {
    u16_t recvd_part = (u16_t)LWIP_MIN(recvd_cnt, 0xFFFF);
    altcp_recved(inner_conn, recvd_part);
    recvd_cnt -= recvd_part;
  }
}

/** Recv callback from lower connection (i.e. TCP)
 * This one mainly differs between connection setup/handshake (data is fed into mbedTLS only)
 * and application phase (data is decoded by mbedTLS and passed on to the application).
 */
static err_t
altcp_mbedtls_lower_recv(void *arg, struct altcp_pcb *inner_conn, struct pbuf *p, err_t err)
{
  altcp_mbedtls_state_t *state;
  struct altcp_pcb *conn = (struct altcp_pcb *)arg;

  LWIP_ASSERT("no err expected", err == ERR_OK);
  LWIP_UNUSED_ARG(err);

  if (!conn) {
    /* no connection given as arg? should not happen, but prevent pbuf/conn leaks */
    if (p != NULL) {
      pbuf_free(p);
    }
    altcp_close(inner_conn);
    return ERR_CLSD;
  }
  state = (altcp_mbedtls_state_t *)conn->state;
  LWIP_ASSERT("pcb mismatch", conn->inner_conn == inner_conn);
  if (!state) {
    /* already closed */
    if (p != NULL) {
      pbuf_free(p);
    }
    altcp_close(inner_conn);
    return ERR_CLSD;
  }

  /* handle NULL pbuf (inner connection closed) */
  if (p == NULL) {
    /* remote host sent FIN, remember this (SSL state is destroyed
        when both sides are closed only!) */
    if ((state->flags & (ALTCP_MBEDTLS_FLAGS_HANDSHAKE_DONE | ALTCP_MBEDTLS_FLAGS_UPPER_CALLED)) ==
        (ALTCP_MBEDTLS_FLAGS_HANDSHAKE_DONE | ALTCP_MBEDTLS_FLAGS_UPPER_CALLED)) {
      /* need to notify upper layer (e.g. 'accept' called or 'connect' succeeded) */
      if ((state->rx != NULL) || (state->rx_app != NULL)) {
        state->flags |= ALTCP_MBEDTLS_FLAGS_RX_CLOSE_QUEUED;
        /* this is a normal close (FIN) but we have unprocessed data, so delay the FIN */
        altcp_mbedtls_handle_rx_appldata(conn, state);
        return ERR_OK;
      }
      state->flags |= ALTCP_MBEDTLS_FLAGS_RX_CLOSED;
      if (conn->recv) {
        return conn->recv(conn->arg, conn, NULL, ERR_OK);
      }
    } else {
      /* before connection setup is done: call 'err' */
      if (conn->err) {
        conn->err(conn->arg, ERR_CLSD);
      }
      altcp_close(conn);
    }
    return ERR_OK;
  }

  /* If we come here, the connection is in good state (handshake phase or application data phase).
     Queue up the pbuf for processing as handshake data or application data. */
  if (state->rx == NULL) {
    state->rx = p;
  } else {
    LWIP_ASSERT("rx pbuf overflow", (int)p->tot_len + (int)p->len <= 0xFFFF);
    pbuf_cat(state->rx, p);
  }
  return altcp_mbedtls_lower_recv_process(conn, state);
}

static err_t
altcp_mbedtls_lower_recv_process(struct altcp_pcb *conn, altcp_mbedtls_state_t *state)
{
  if (!(state->flags & ALTCP_MBEDTLS_FLAGS_HANDSHAKE_DONE)) {
    /* handle connection setup (handshake not done) */
    int ret = mbedtls_ssl_handshake(&state->ssl_context);
    /* try to send data... */
    altcp_output(conn->inner_conn);
    if (state->bio_bytes_read) {
      /* acknowledge all bytes read */
      altcp_mbedtls_lower_recved(conn->inner_conn, state->bio_bytes_read);
      state->bio_bytes_read = 0;
    }

    if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
      /* handshake not done, wait for more recv calls */
      LWIP_ASSERT("in this state, the rx chain should be empty", state->rx == NULL);
      return ERR_OK;
    }
    if (ret != 0) {
      LWIP_DEBUGF(ALTCP_MBEDTLS_DEBUG, ("mbedtls_ssl_handshake failed: %d\n", ret));
      /* handshake failed, connection has to be closed */
      if (conn->err) {
        conn->err(conn->arg, ERR_CLSD);
      }

      if (altcp_close(conn) != ERR_OK) {
        altcp_abort(conn);
      }
      return ERR_OK;
    }
    /* If we come here, handshake succeeded. */
    LWIP_ASSERT("state", state->bio_bytes_read == 0);
    LWIP_ASSERT("state", state->bio_bytes_appl == 0);
    state->flags |= ALTCP_MBEDTLS_FLAGS_HANDSHAKE_DONE;
    /* issue "connect" callback" to upper connection (this can only happen for active open) */
    if (conn->connected) {
      err_t err;
      err = conn->connected(conn->arg, conn, ERR_OK);
      if (err != ERR_OK) {
        return err;
      }
    }
    if (state->rx == NULL) {
      return ERR_OK;
    }
  }
  /* handle application data */
  return altcp_mbedtls_handle_rx_appldata(conn, state);
}

/* Pass queued decoded rx data to application */
static err_t
altcp_mbedtls_pass_rx_data(struct altcp_pcb *conn, altcp_mbedtls_state_t *state)
{
  err_t err;
  struct pbuf *buf;
  LWIP_ASSERT("conn != NULL", conn != NULL);
  LWIP_ASSERT("state != NULL", state != NULL);
  buf = state->rx_app;
  if (buf) {
    state->rx_app = NULL;
    if (conn->recv) {
      u16_t tot_len = buf->tot_len;
      /* this needs to be increased first because the 'recved' call may come nested */
      state->rx_passed_unrecved += tot_len;
      state->flags |= ALTCP_MBEDTLS_FLAGS_UPPER_CALLED;
      err = conn->recv(conn->arg, conn, buf, ERR_OK);
      if (err != ERR_OK) {
        if (err == ERR_ABRT) {
          return ERR_ABRT;
        }
        /* not received, leave the pbuf(s) queued (and decrease 'unrecved' again) */
        LWIP_ASSERT("state == conn->state", state == conn->state);
        state->rx_app = buf;
        state->rx_passed_unrecved -= tot_len;
        LWIP_ASSERT("state->rx_passed_unrecved >= 0", state->rx_passed_unrecved >= 0);
        if (state->rx_passed_unrecved < 0) {
          state->rx_passed_unrecved = 0;
        }
        return err;
      }
    } else {
      pbuf_free(buf);
    }
  } else if ((state->flags & (ALTCP_MBEDTLS_FLAGS_RX_CLOSE_QUEUED | ALTCP_MBEDTLS_FLAGS_RX_CLOSED)) ==
             ALTCP_MBEDTLS_FLAGS_RX_CLOSE_QUEUED) {
    state->flags |= ALTCP_MBEDTLS_FLAGS_RX_CLOSED;
    if (conn->recv) {
      return conn->recv(conn->arg, conn, NULL, ERR_OK);
    }
  }

  /* application may have close the connection */
  if (conn->state != state) {
    /* return error code to ensure altcp_mbedtls_handle_rx_appldata() exits the loop */
    return ERR_CLSD;
  }
  return ERR_OK;
}

/* Helper function that processes rx application data stored in rx pbuf chain */
static err_t
altcp_mbedtls_handle_rx_appldata(struct altcp_pcb *conn, altcp_mbedtls_state_t *state)
{
  int ret;
  LWIP_ASSERT("state != NULL", state != NULL);
  if (!(state->flags & ALTCP_MBEDTLS_FLAGS_HANDSHAKE_DONE)) {
    /* handshake not done yet */
    return ERR_VAL;
  }
  do {
    /* allocate a full-sized unchained PBUF_POOL: this is for RX! */
    struct pbuf *buf = pbuf_alloc(PBUF_RAW, PBUF_POOL_BUFSIZE, PBUF_POOL);
    if (buf == NULL) {
      /* We're short on pbufs, try again later from 'poll' or 'recv' callbacks.
         @todo: close on excessive allocation failures or leave this up to upper conn? */
      return ERR_OK;
    }

    /* decrypt application data, this pulls encrypted RX data off state->rx pbuf chain */
    ret = mbedtls_ssl_read(&state->ssl_context, (unsigned char *)buf->payload, PBUF_POOL_BUFSIZE);
    if (ret < 0) {
      if (ret == MBEDTLS_ERR_SSL_CLIENT_RECONNECT) {
        /* client is initiating a new connection using the same source port -> close connection or make handshake */
        LWIP_DEBUGF(ALTCP_MBEDTLS_DEBUG, ("new connection on same source port\n"));
        LWIP_ASSERT("TODO: new connection on same source port, close this connection", 0);
      } else if ((ret != MBEDTLS_ERR_SSL_WANT_READ) && (ret != MBEDTLS_ERR_SSL_WANT_WRITE)) {
        if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
          LWIP_DEBUGF(ALTCP_MBEDTLS_DEBUG, ("connection was closed gracefully\n"));
        } else if (ret == MBEDTLS_ERR_NET_CONN_RESET) {
          LWIP_DEBUGF(ALTCP_MBEDTLS_DEBUG, ("connection was reset by peer\n"));
        }
        pbuf_free(buf);
        return ERR_OK;
      } else {
        pbuf_free(buf);
        return ERR_OK;
      }
      pbuf_free(buf);
      altcp_abort(conn);
      return ERR_ABRT;
    } else {
      err_t err;
      if (ret) {
        LWIP_ASSERT("bogus receive length", ret <= PBUF_POOL_BUFSIZE);
        /* trim pool pbuf to actually decoded length */
        pbuf_realloc(buf, (u16_t)ret);

        state->bio_bytes_appl += ret;
        if (mbedtls_ssl_get_bytes_avail(&state->ssl_context) == 0) {
          /* Record is done, now we know the share between application and protocol bytes
             and can adjust the RX window by the protocol bytes.
             The rest is 'recved' by the application calling our 'recved' fn. */
          int overhead_bytes;
          LWIP_ASSERT("bogus byte counts", state->bio_bytes_read > state->bio_bytes_appl);
          overhead_bytes = state->bio_bytes_read - state->bio_bytes_appl;
          altcp_mbedtls_lower_recved(conn->inner_conn, overhead_bytes);
          state->bio_bytes_read = 0;
          state->bio_bytes_appl = 0;
        }

        if (state->rx_app == NULL) {
          state->rx_app = buf;
        } else {
          pbuf_cat(state->rx_app, buf);
        }
      } else {
        pbuf_free(buf);
        buf = NULL;
      }
      err = altcp_mbedtls_pass_rx_data(conn, state);
      if (err != ERR_OK) {
        if (err == ERR_ABRT) {
          /* recv callback needs to return this as the pcb is deallocated */
          return ERR_ABRT;
        }
        /* we hide all other errors as we retry feeding the pbuf to the app later */
        return ERR_OK;
      }
    }
  } while (ret > 0);
  return ERR_OK;
}

/** Receive callback function called from mbedtls (set via mbedtls_ssl_set_bio)
 * This function mainly copies data from pbufs and frees the pbufs after copying.
 */
static int
altcp_mbedtls_bio_recv(void *ctx, unsigned char *buf, size_t len)
{
  struct altcp_pcb *conn = (struct altcp_pcb *)ctx;
  altcp_mbedtls_state_t *state;
  struct pbuf *p;
  u16_t ret;
  u16_t copy_len;
  err_t err;

  LWIP_UNUSED_ARG(err); /* for LWIP_NOASSERT */
  if ((conn == NULL) || (conn->state == NULL)) {
    return MBEDTLS_ERR_NET_INVALID_CONTEXT;
  }
  state = (altcp_mbedtls_state_t *)conn->state;
  LWIP_ASSERT("state != NULL", state != NULL);
  p = state->rx;

  /* @todo: return MBEDTLS_ERR_NET_CONN_RESET/MBEDTLS_ERR_NET_RECV_FAILED? */

  if ((p == NULL) || ((p->len == 0) && (p->next == NULL))) {
    if (p) {
      pbuf_free(p);
    }
    state->rx = NULL;
    if ((state->flags & (ALTCP_MBEDTLS_FLAGS_RX_CLOSE_QUEUED | ALTCP_MBEDTLS_FLAGS_RX_CLOSED)) ==
        ALTCP_MBEDTLS_FLAGS_RX_CLOSE_QUEUED) {
      /* close queued but not passed up yet */
      return 0;
    }
    return MBEDTLS_ERR_SSL_WANT_READ;
  }
  /* limit number of bytes again to copy from first pbuf in a chain only */
  copy_len = (u16_t)LWIP_MIN(len, p->len);
  /* copy the data */
  ret = pbuf_copy_partial(p, buf, copy_len, 0);
  LWIP_ASSERT("ret == copy_len", ret == copy_len);
  /* hide the copied bytes from the pbuf */
  err = pbuf_remove_header(p, ret);
  LWIP_ASSERT("error", err == ERR_OK);
  if (p->len == 0) {
    /* the first pbuf has been fully read, free it */
    state->rx = p->next;
    p->next = NULL;
    pbuf_free(p);
  }

  state->bio_bytes_read += (int)ret;
  return ret;
}

/** Sent callback from lower connection (i.e. TCP)
 * This only informs the upper layer to try to send more, not about
 * the number of ACKed bytes.
 */
static err_t
altcp_mbedtls_lower_sent(void *arg, struct altcp_pcb *inner_conn, u16_t len)
{
  struct altcp_pcb *conn = (struct altcp_pcb *)arg;
  LWIP_UNUSED_ARG(inner_conn); /* for LWIP_NOASSERT */
  LWIP_UNUSED_ARG(len);
  if (conn) {
    altcp_mbedtls_state_t *state = (altcp_mbedtls_state_t *)conn->state;
    LWIP_ASSERT("pcb mismatch", conn->inner_conn == inner_conn);
    if (!state || !(state->flags & ALTCP_MBEDTLS_FLAGS_HANDSHAKE_DONE)) {
      /* @todo: do something here? */
      return ERR_OK;
    }
    /* try to send more if we failed before */
    mbedtls_ssl_flush_output(&state->ssl_context);
    /* call upper sent with len==0 if the application already sent data */
    if ((state->flags & ALTCP_MBEDTLS_FLAGS_APPLDATA_SENT) && conn->sent) {
      return conn->sent(conn->arg, conn, 0);
    }
  }
  return ERR_OK;
}

/** Poll callback from lower connection (i.e. TCP)
 * Just pass this on to the application.
 * @todo: retry sending?
 */
static err_t
altcp_mbedtls_lower_poll(void *arg, struct altcp_pcb *inner_conn)
{
  struct altcp_pcb *conn = (struct altcp_pcb *)arg;
  LWIP_UNUSED_ARG(inner_conn); /* for LWIP_NOASSERT */
  if (conn) {
    LWIP_ASSERT("pcb mismatch", conn->inner_conn == inner_conn);
    /* check if there's unreceived rx data */
    if (conn->state) {
      altcp_mbedtls_state_t *state = (altcp_mbedtls_state_t *)conn->state;
      /* try to send more if we failed before */
      mbedtls_ssl_flush_output(&state->ssl_context);
      if (altcp_mbedtls_handle_rx_appldata(conn, state) == ERR_ABRT) {
        return ERR_ABRT;
      }
    }
    if (conn->poll) {
      return conn->poll(conn->arg, conn);
    }
  }
  return ERR_OK;
}

static void
altcp_mbedtls_lower_err(void *arg, err_t err)
{
  struct altcp_pcb *conn = (struct altcp_pcb *)arg;
  if (conn) {
    conn->inner_conn = NULL; /* already freed */
    if (conn->err) {
      conn->err(conn->arg, err);
    }
    altcp_free(conn);
  }
}

/* setup functions */

static void
altcp_mbedtls_remove_callbacks(struct altcp_pcb *inner_conn)
{
  altcp_arg(inner_conn, NULL);
  altcp_recv(inner_conn, NULL);
  altcp_sent(inner_conn, NULL);
  altcp_err(inner_conn, NULL);
  altcp_poll(inner_conn, NULL, inner_conn->pollinterval);
}

static void
altcp_mbedtls_setup_callbacks(struct altcp_pcb *conn, struct altcp_pcb *inner_conn)
{
  altcp_arg(inner_conn, conn);
  altcp_recv(inner_conn, altcp_mbedtls_lower_recv);
  altcp_sent(inner_conn, altcp_mbedtls_lower_sent);
  altcp_err(inner_conn, altcp_mbedtls_lower_err);
  /* tcp_poll is set when interval is set by application */
  /* listen is set totally different :-) */
}

static err_t
altcp_mbedtls_setup(void *conf, struct altcp_pcb *conn, struct altcp_pcb *inner_conn)
{
  int ret;
  struct altcp_tls_config *config = (struct altcp_tls_config *)conf;
  altcp_mbedtls_state_t *state;
  if (!conf) {
    return ERR_ARG;
  }
  LWIP_ASSERT("invalid inner_conn", conn != inner_conn);

  /* allocate mbedtls context */
  state = altcp_mbedtls_alloc(conf);
  if (state == NULL) {
    return ERR_MEM;
  }
  /* initialize mbedtls context: */
  mbedtls_ssl_init(&state->ssl_context);
  ret = mbedtls_ssl_setup(&state->ssl_context, &config->conf);
  if (ret != 0) {
    LWIP_DEBUGF(ALTCP_MBEDTLS_DEBUG, ("mbedtls_ssl_setup failed\n"));
    /* @todo: convert 'ret' to err_t */
    altcp_mbedtls_free(conf, state);
    return ERR_MEM;
  }
  /* tell mbedtls about our I/O functions */
  mbedtls_ssl_set_bio(&state->ssl_context, conn, altcp_mbedtls_bio_send, altcp_mbedtls_bio_recv, NULL);

  altcp_mbedtls_setup_callbacks(conn, inner_conn);
  conn->inner_conn = inner_conn;
  conn->fns = &altcp_mbedtls_functions;
  conn->state = state;
  return ERR_OK;
}

struct altcp_pcb *
altcp_tls_wrap(struct altcp_tls_config *config, struct altcp_pcb *inner_pcb)
{
  struct altcp_pcb *ret;
  if (inner_pcb == NULL) {
    return NULL;
  }
  ret = altcp_alloc();
  if (ret != NULL) {
    if (altcp_mbedtls_setup(config, ret, inner_pcb) != ERR_OK) {
      altcp_free(ret);
      return NULL;
    }
  }
  return ret;
}

void *
altcp_tls_context(struct altcp_pcb *conn)
{
  if (conn && conn->state) {
    altcp_mbedtls_state_t *state = (altcp_mbedtls_state_t *)conn->state;
    return &state->ssl_context;
  }
  return NULL;
}

#if ALTCP_MBEDTLS_LIB_DEBUG != LWIP_DBG_OFF
static void
altcp_mbedtls_debug(void *ctx, int level, const char *file, int line, const char *str)
{
  LWIP_UNUSED_ARG(ctx);
  LWIP_UNUSED_ARG(file);
  LWIP_UNUSED_ARG(line);
  LWIP_UNUSED_ARG(str);

  if (level >= ALTCP_MBEDTLS_LIB_DEBUG_LEVEL_MIN) {
    LWIP_DEBUGF(ALTCP_MBEDTLS_LIB_DEBUG, ("%s:%04d: %s", file, line, str));
  }
}
#endif

/** Create new TLS configuration
 * ATTENTION: Server certificate and private key have to be added outside this function!
 */
static struct altcp_tls_config *
altcp_tls_create_config(int is_server, uint8_t cert_count, uint8_t pkey_count, int have_ca)
{
  size_t sz;
  int ret;
  struct altcp_tls_config *conf;
  mbedtls_x509_crt *mem;

  if (TCP_WND < MBEDTLS_SSL_MAX_CONTENT_LEN) {
    LWIP_DEBUGF(ALTCP_MBEDTLS_DEBUG|LWIP_DBG_LEVEL_SERIOUS,
      ("altcp_tls: TCP_WND is smaller than the RX decrypion buffer, connection RX might stall!\n"));
  }

  altcp_mbedtls_mem_init();

  sz = sizeof(struct altcp_tls_config);
  if (cert_count > 0) {
    sz += (cert_count * sizeof(mbedtls_x509_crt));
  }
  if (have_ca) {
    sz += sizeof(mbedtls_x509_crt);
  }
  if (pkey_count > 0) {
    sz += (pkey_count * sizeof(mbedtls_pk_context));
  }

  conf = (struct altcp_tls_config *)altcp_mbedtls_alloc_config(sz);
  if (conf == NULL) {
    return NULL;
  }
  conf->cert_max = cert_count;
  mem = (mbedtls_x509_crt *)(conf + 1);
  if (cert_count > 0) {
    conf->cert = mem;
    mem += cert_count;
  }
  if (have_ca) {
    conf->ca = mem;
    mem++;
  }
  conf->pkey_max = pkey_count;
  if (pkey_count > 0) {
    conf->pkey = (mbedtls_pk_context *)mem;
  }

  mbedtls_ssl_config_init(&conf->conf);

  if (!altcp_tls_entropy_rng) {
    altcp_tls_entropy_rng = (struct altcp_tls_entropy_rng *)altcp_mbedtls_alloc_config(sizeof(struct altcp_tls_entropy_rng));
    if (altcp_tls_entropy_rng) {
      altcp_tls_entropy_rng->ref = 1;
      mbedtls_entropy_init(&altcp_tls_entropy_rng->entropy);
      mbedtls_ctr_drbg_init(&altcp_tls_entropy_rng->ctr_drbg);
      /* Seed the RNG, only once */
      ret = mbedtls_ctr_drbg_seed(&altcp_tls_entropy_rng->ctr_drbg,
                                  mbedtls_entropy_func, &altcp_tls_entropy_rng->entropy,
                                  ALTCP_MBEDTLS_ENTROPY_PTR, ALTCP_MBEDTLS_ENTROPY_LEN);
      if (ret != 0) {
        LWIP_DEBUGF(ALTCP_MBEDTLS_DEBUG, ("mbedtls_ctr_drbg_seed failed: %d\n", ret));
        mbedtls_ctr_drbg_free(&altcp_tls_entropy_rng->ctr_drbg);
        mbedtls_entropy_free(&altcp_tls_entropy_rng->entropy);
        altcp_mbedtls_free_config(altcp_tls_entropy_rng);
        altcp_tls_entropy_rng = NULL;
        altcp_mbedtls_free_config(conf);
        return NULL;
      }
    } else {
      altcp_mbedtls_free_config(conf);
      return NULL;
    }
  } else {
    altcp_tls_entropy_rng->ref++;
  }

  /* Setup ssl context (@todo: what's different for a client here? -> might better be done on listen/connect) */
  ret = mbedtls_ssl_config_defaults(&conf->conf, is_server ? MBEDTLS_SSL_IS_SERVER : MBEDTLS_SSL_IS_CLIENT,
                                    MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT);
  if (ret != 0) {
    LWIP_DEBUGF(ALTCP_MBEDTLS_DEBUG, ("mbedtls_ssl_config_defaults failed: %d\n", ret));
    if (altcp_tls_entropy_rng->ref == 1) {
      mbedtls_ctr_drbg_free(&altcp_tls_entropy_rng->ctr_drbg);
      mbedtls_entropy_free(&altcp_tls_entropy_rng->entropy);
      altcp_mbedtls_free_config(altcp_tls_entropy_rng);
      altcp_tls_entropy_rng = NULL;
    }
    altcp_mbedtls_free_config(conf);
    return NULL;
  }
  mbedtls_ssl_conf_authmode(&conf->conf, MBEDTLS_SSL_VERIFY_OPTIONAL);

  mbedtls_ssl_conf_rng(&conf->conf, mbedtls_ctr_drbg_random, &altcp_tls_entropy_rng->ctr_drbg);
#if ALTCP_MBEDTLS_LIB_DEBUG != LWIP_DBG_OFF
  mbedtls_ssl_conf_dbg(&conf->conf, altcp_mbedtls_debug, stdout);
#endif
#if defined(MBEDTLS_SSL_CACHE_C) && ALTCP_MBEDTLS_USE_SESSION_CACHE
  mbedtls_ssl_conf_session_cache(&conf->conf, &conf->cache, mbedtls_ssl_cache_get, mbedtls_ssl_cache_set);
  mbedtls_ssl_cache_set_timeout(&conf->cache, ALTCP_MBEDTLS_SESSION_CACHE_TIMEOUT_SECONDS);
  mbedtls_ssl_cache_set_max_entries(&conf->cache, ALTCP_MBEDTLS_SESSION_CACHE_SIZE);
#endif

#if defined(MBEDTLS_SSL_SESSION_TICKETS) && ALTCP_MBEDTLS_USE_SESSION_TICKETS
  mbedtls_ssl_ticket_init(&conf->ticket_ctx);

  ret = mbedtls_ssl_ticket_setup(&conf->ticket_ctx, mbedtls_ctr_drbg_random, &altcp_tls_entropy_rng->ctr_drbg,
    ALTCP_MBEDTLS_SESSION_TICKET_CIPHER, ALTCP_MBEDTLS_SESSION_TICKET_TIMEOUT_SECONDS);
  if (ret) {
    LWIP_DEBUGF(ALTCP_MBEDTLS_DEBUG, ("mbedtls_ssl_ticket_setup failed: %d\n", ret));
    altcp_mbedtls_free_config(conf);
    return NULL;
  }

  mbedtls_ssl_conf_session_tickets_cb(&conf->conf, mbedtls_ssl_ticket_write, mbedtls_ssl_ticket_parse,
    &conf->ticket_ctx);
#endif

  return conf;
}

struct altcp_tls_config *altcp_tls_create_config_server(uint8_t cert_count)
{
  struct altcp_tls_config *conf = altcp_tls_create_config(1, cert_count, cert_count, 0);
  if (conf == NULL) {
    return NULL;
  }

  mbedtls_ssl_conf_ca_chain(&conf->conf, NULL, NULL);
  return conf;
}

err_t altcp_tls_config_server_add_privkey_cert(struct altcp_tls_config *config,
      const u8_t *privkey, size_t privkey_len,
      const u8_t *privkey_pass, size_t privkey_pass_len,
      const u8_t *cert, size_t cert_len)
{
  int ret;
  mbedtls_x509_crt *srvcert;
  mbedtls_pk_context *pkey;

  if (config->cert_count >= config->cert_max) {
    return ERR_MEM;
  }
  if (config->pkey_count >= config->pkey_max) {
    return ERR_MEM;
  }

  srvcert = config->cert + config->cert_count;
  mbedtls_x509_crt_init(srvcert);

  pkey = config->pkey + config->pkey_count;
  mbedtls_pk_init(pkey);

  /* Load the certificates and private key */
  ret = mbedtls_x509_crt_parse(srvcert, cert, cert_len);
  if (ret != 0) {
    LWIP_DEBUGF(ALTCP_MBEDTLS_DEBUG, ("mbedtls_x509_crt_parse failed: %d\n", ret));
    return ERR_VAL;
  }

  ret = mbedtls_pk_parse_key(pkey, (const unsigned char *) privkey, privkey_len, privkey_pass, privkey_pass_len);
  if (ret != 0) {
    LWIP_DEBUGF(ALTCP_MBEDTLS_DEBUG, ("mbedtls_pk_parse_public_key failed: %d\n", ret));
    mbedtls_x509_crt_free(srvcert);
    return ERR_VAL;
  }

  ret = mbedtls_ssl_conf_own_cert(&config->conf, srvcert, pkey);
  if (ret != 0) {
    LWIP_DEBUGF(ALTCP_MBEDTLS_DEBUG, ("mbedtls_ssl_conf_own_cert failed: %d\n", ret));
    mbedtls_x509_crt_free(srvcert);
    mbedtls_pk_free(pkey);
    return ERR_VAL;
  }

  config->cert_count++;
  config->pkey_count++;
  return ERR_OK;
}

/** Create new TLS configuration
 * This is a suboptimal version that gets the encrypted private key and its password,
 * as well as the server certificate.
 */
struct altcp_tls_config *
altcp_tls_create_config_server_privkey_cert(const u8_t *privkey, size_t privkey_len,
    const u8_t *privkey_pass, size_t privkey_pass_len,
    const u8_t *cert, size_t cert_len)
{
  struct altcp_tls_config *conf = altcp_tls_create_config_server(1);
  if (conf == NULL) {
    return NULL;
  }

  if (altcp_tls_config_server_add_privkey_cert(conf, privkey, privkey_len,
    privkey_pass, privkey_pass_len, cert, cert_len) != ERR_OK) {
    altcp_mbedtls_free_config(conf);
    return NULL;
  }

  return conf;
}

static struct altcp_tls_config *
altcp_tls_create_config_client_common(const u8_t *ca, size_t ca_len, int is_2wayauth)
{
  int ret;
  struct altcp_tls_config *conf = altcp_tls_create_config(0, (is_2wayauth) ? 1 : 0, (is_2wayauth) ? 1 : 0, ca != NULL);
  if (conf == NULL) {
    return NULL;
  }

  /* Initialize the CA certificate if provided
   * CA certificate is optional (to save memory) but recommended for production environment
   * Without CA certificate, connection will be prone to man-in-the-middle attacks */
  if (ca) {
    mbedtls_x509_crt_init(conf->ca);
    ret = mbedtls_x509_crt_parse(conf->ca, ca, ca_len);
    if (ret != 0) {
      LWIP_DEBUGF(ALTCP_MBEDTLS_DEBUG, ("mbedtls_x509_crt_parse ca failed: %d 0x%x", ret, -1*ret));
      altcp_mbedtls_free_config(conf);
      return NULL;
    }

    mbedtls_ssl_conf_ca_chain(&conf->conf, conf->ca, NULL);
  }
  return conf;
}

struct altcp_tls_config *
altcp_tls_create_config_client(const u8_t *ca, size_t ca_len)
{
  return altcp_tls_create_config_client_common(ca, ca_len, 0);
}

struct altcp_tls_config *
altcp_tls_create_config_client_2wayauth(const u8_t *ca, size_t ca_len, const u8_t *privkey, size_t privkey_len,
                                        const u8_t *privkey_pass, size_t privkey_pass_len,
                                        const u8_t *cert, size_t cert_len)
{
  int ret;
  struct altcp_tls_config *conf;

  if (!cert || !privkey) {
    LWIP_DEBUGF(ALTCP_MBEDTLS_DEBUG, ("altcp_tls_create_config_client_2wayauth: certificate and priv key required"));
    return NULL;
  }

  conf = altcp_tls_create_config_client_common(ca, ca_len, 1);
  if (conf == NULL) {
    return NULL;
  }

  /* Initialize the client certificate and corresponding private key */
  mbedtls_x509_crt_init(conf->cert);
  ret = mbedtls_x509_crt_parse(conf->cert, cert, cert_len);
  if (ret != 0) {
    LWIP_DEBUGF(ALTCP_MBEDTLS_DEBUG, ("mbedtls_x509_crt_parse cert failed: %d 0x%x", ret, -1*ret));
    altcp_mbedtls_free_config(conf->cert);
    return NULL;
  }

  mbedtls_pk_init(conf->pkey);
  ret = mbedtls_pk_parse_key(conf->pkey, privkey, privkey_len, privkey_pass, privkey_pass_len);
  if (ret != 0) {
    LWIP_DEBUGF(ALTCP_MBEDTLS_DEBUG, ("mbedtls_pk_parse_key failed: %d 0x%x", ret, -1*ret));
    altcp_mbedtls_free_config(conf);
    return NULL;
  }

  ret = mbedtls_ssl_conf_own_cert(&conf->conf, conf->cert, conf->pkey);
  if (ret != 0) {
    LWIP_DEBUGF(ALTCP_MBEDTLS_DEBUG, ("mbedtls_ssl_conf_own_cert failed: %d 0x%x", ret, -1*ret));
    altcp_mbedtls_free_config(conf);
    return NULL;
  }

  return conf;
}

void
altcp_tls_free_config(struct altcp_tls_config *conf)
{
  if (conf->pkey) {
    mbedtls_pk_free(conf->pkey);
  }
  if (conf->cert) {
    mbedtls_x509_crt_free(conf->cert);
  }
  if (conf->ca) {
    mbedtls_x509_crt_free(conf->ca);
  }
  altcp_mbedtls_free_config(conf);
  if (altcp_tls_entropy_rng && altcp_tls_entropy_rng->ref)
      altcp_tls_entropy_rng->ref--;
}

void
altcp_tls_free_entropy(void)
{
  if (altcp_tls_entropy_rng && altcp_tls_entropy_rng->ref == 0) {
    mbedtls_ctr_drbg_free(&altcp_tls_entropy_rng->ctr_drbg);
    mbedtls_entropy_free(&altcp_tls_entropy_rng->entropy);
    altcp_mbedtls_free_config(altcp_tls_entropy_rng);
    altcp_tls_entropy_rng = NULL;
  }
}

/* "virtual" functions */
static void
altcp_mbedtls_set_poll(struct altcp_pcb *conn, u8_t interval)
{
  if (conn != NULL) {
    altcp_poll(conn->inner_conn, altcp_mbedtls_lower_poll, interval);
  }
}

static void
altcp_mbedtls_recved(struct altcp_pcb *conn, u16_t len)
{
  u16_t lower_recved;
  altcp_mbedtls_state_t *state;
  if (conn == NULL) {
    return;
  }
  state = (altcp_mbedtls_state_t *)conn->state;
  if (state == NULL) {
    return;
  }
  if (!(state->flags & ALTCP_MBEDTLS_FLAGS_HANDSHAKE_DONE)) {
    return;
  }
  lower_recved = len;
  if (lower_recved > state->rx_passed_unrecved) {
    LWIP_DEBUGF(ALTCP_MBEDTLS_DEBUG, ("bogus recved count (len > state->rx_passed_unrecved / %d / %d)",
                                      len, state->rx_passed_unrecved));
    lower_recved = (u16_t)state->rx_passed_unrecved;
  }
  state->rx_passed_unrecved -= lower_recved;

  altcp_recved(conn->inner_conn, lower_recved);
}

static err_t
altcp_mbedtls_connect(struct altcp_pcb *conn, const ip_addr_t *ipaddr, u16_t port, altcp_connected_fn connected)
{
  if (conn == NULL) {
    return ERR_VAL;
  }
  conn->connected = connected;
  return altcp_connect(conn->inner_conn, ipaddr, port, altcp_mbedtls_lower_connected);
}

static struct altcp_pcb *
altcp_mbedtls_listen(struct altcp_pcb *conn, u8_t backlog, err_t *err)
{
  struct altcp_pcb *lpcb;
  if (conn == NULL) {
    return NULL;
  }
  lpcb = altcp_listen_with_backlog_and_err(conn->inner_conn, backlog, err);
  if (lpcb != NULL) {
    altcp_mbedtls_state_t *state = (altcp_mbedtls_state_t *)conn->state;
    /* Free members of the ssl context (not used on listening pcb). This
       includes freeing input/output buffers, so saves ~32KByte by default */
    mbedtls_ssl_free(&state->ssl_context);

    conn->inner_conn = lpcb;
    altcp_accept(lpcb, altcp_mbedtls_lower_accept);
    return conn;
  }
  return NULL;
}

static void
altcp_mbedtls_abort(struct altcp_pcb *conn)
{
  if (conn != NULL) {
    altcp_abort(conn->inner_conn);
  }
}

static err_t
altcp_mbedtls_close(struct altcp_pcb *conn)
{
  struct altcp_pcb *inner_conn;
  if (conn == NULL) {
    return ERR_VAL;
  }
  inner_conn = conn->inner_conn;
  if (inner_conn) {
    err_t err;
    altcp_poll_fn oldpoll = inner_conn->poll;
    altcp_mbedtls_remove_callbacks(conn->inner_conn);
    err = altcp_close(conn->inner_conn);
    if (err != ERR_OK) {
      /* not closed, set up all callbacks again */
      altcp_mbedtls_setup_callbacks(conn, inner_conn);
      /* poll callback is not included in the above */
      altcp_poll(inner_conn, oldpoll, inner_conn->pollinterval);
      return err;
    }
    conn->inner_conn = NULL;
  }
  altcp_free(conn);
  return ERR_OK;
}

/** Allow caller of altcp_write() to limit to negotiated chunk size
 *  or remaining sndbuf space of inner_conn.
 */
static u16_t
altcp_mbedtls_sndbuf(struct altcp_pcb *conn)
{
  if (conn) {
    altcp_mbedtls_state_t *state;
    state = (altcp_mbedtls_state_t*)conn->state;
    if (!state || !(state->flags & ALTCP_MBEDTLS_FLAGS_HANDSHAKE_DONE)) {
      return 0;
    }
    if (conn->inner_conn) {
      u16_t sndbuf = altcp_sndbuf(conn->inner_conn);
      /* Take care of record header, IV, AuthTag */
      int ssl_expan = mbedtls_ssl_get_record_expansion(&state->ssl_context);
      if (ssl_expan > 0) {
        size_t ssl_added = (u16_t)LWIP_MIN(ssl_expan, 0xFFFF);
        /* internal sndbuf smaller than our offset */
        if (ssl_added < sndbuf) {
          size_t max_len = 0xFFFF;
          size_t ret;
#if defined(MBEDTLS_SSL_MAX_FRAGMENT_LENGTH)
          /* @todo: adjust ssl_added to real value related to negociated cipher */
          size_t max_frag_len = mbedtls_ssl_get_max_frag_len(&state->ssl_context);
          max_len = LWIP_MIN(max_frag_len, max_len);
#endif
          /* Adjust sndbuf of inner_conn with what added by SSL */
          ret = LWIP_MIN(sndbuf - ssl_added, max_len);
          LWIP_ASSERT("sndbuf overflow", ret <= 0xFFFF);
          return (u16_t)ret;
        }
      }
    }
  }
  /* fallback: use sendbuf of the inner connection */
  return altcp_default_sndbuf(conn);
}

/** Write data to a TLS connection. Calls into mbedTLS, which in turn calls into
 * @ref altcp_mbedtls_bio_send() to send the encrypted data
 */
static err_t
altcp_mbedtls_write(struct altcp_pcb *conn, const void *dataptr, u16_t len, u8_t apiflags)
{
  int ret;
  altcp_mbedtls_state_t *state;

  LWIP_UNUSED_ARG(apiflags);

  if (conn == NULL) {
    return ERR_VAL;
  }

  state = (altcp_mbedtls_state_t *)conn->state;
  if (state == NULL) {
    /* @todo: which error? */
    return ERR_CLSD;
  }
  if (!(state->flags & ALTCP_MBEDTLS_FLAGS_HANDSHAKE_DONE)) {
    /* @todo: which error? */
    return ERR_VAL;
  }

  /* HACK: if thre is something left to send, try to flush it and only
     allow sending more if this succeeded (this is a hack because neither
     returning 0 nor MBEDTLS_ERR_SSL_WANT_WRITE worked for me) */
  if (state->ssl_context.out_left) {
    mbedtls_ssl_flush_output(&state->ssl_context);
    if (state->ssl_context.out_left) {
      return ERR_MEM;
    }
  }
  ret = mbedtls_ssl_write(&state->ssl_context, (const unsigned char *)dataptr, len);
  /* try to send data... */
  altcp_output(conn->inner_conn);
  if (ret >= 0) {
    if (ret == len) {
      state->flags |= ALTCP_MBEDTLS_FLAGS_APPLDATA_SENT;
      return ERR_OK;
    } else {
      /* @todo/@fixme: assumption: either everything sent or error */
      LWIP_ASSERT("ret <= 0", 0);
      return ERR_MEM;
    }
  } else {
    if (ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
      /* @todo: convert error to err_t */
      return ERR_MEM;
    }
    LWIP_ASSERT("unhandled error", 0);
    return ERR_VAL;
  }
}

/** Send callback function called from mbedtls (set via mbedtls_ssl_set_bio)
 * This function is either called during handshake or when sending application
 * data via @ref altcp_mbedtls_write (or altcp_write)
 */
static int
altcp_mbedtls_bio_send(void *ctx, const unsigned char *dataptr, size_t size)
{
  struct altcp_pcb *conn = (struct altcp_pcb *) ctx;
  int written = 0;
  size_t size_left = size;
  u8_t apiflags = TCP_WRITE_FLAG_COPY;

  LWIP_ASSERT("conn != NULL", conn != NULL);
  if ((conn == NULL) || (conn->inner_conn == NULL)) {
    return MBEDTLS_ERR_NET_INVALID_CONTEXT;
  }

  while (size_left) {
    u16_t write_len = (u16_t)LWIP_MIN(size_left, 0xFFFF);
    err_t err = altcp_write(conn->inner_conn, (const void *)dataptr, write_len, apiflags);
    if (err == ERR_OK) {
      written += write_len;
      size_left -= write_len;
    } else if (err == ERR_MEM) {
      if (written) {
        return written;
      }
      return 0; /* MBEDTLS_ERR_SSL_WANT_WRITE; */
    } else {
      LWIP_ASSERT("tls_write, tcp_write: err != ERR MEM", 0);
      /* @todo: return MBEDTLS_ERR_NET_CONN_RESET or MBEDTLS_ERR_NET_SEND_FAILED */
      return MBEDTLS_ERR_NET_SEND_FAILED;
    }
  }
  return written;
}

static u16_t
altcp_mbedtls_mss(struct altcp_pcb *conn)
{
  if (conn == NULL) {
    return 0;
  }
  /* @todo: LWIP_MIN(mss, mbedtls_ssl_get_max_frag_len()) ? */
  return altcp_mss(conn->inner_conn);
}

static void
altcp_mbedtls_dealloc(struct altcp_pcb *conn)
{
  /* clean up and free tls state */
  if (conn) {
    altcp_mbedtls_state_t *state = (altcp_mbedtls_state_t *)conn->state;
    if (state) {
      mbedtls_ssl_free(&state->ssl_context);
      state->flags = 0;
      if (state->rx) {
        /* free leftover (unhandled) rx pbufs */
        pbuf_free(state->rx);
        state->rx = NULL;
      }
      altcp_mbedtls_free(state->conf, state);
      conn->state = NULL;
    }
  }
}

const struct altcp_functions altcp_mbedtls_functions = {
  altcp_mbedtls_set_poll,
  altcp_mbedtls_recved,
  altcp_default_bind,
  altcp_mbedtls_connect,
  altcp_mbedtls_listen,
  altcp_mbedtls_abort,
  altcp_mbedtls_close,
  altcp_default_shutdown,
  altcp_mbedtls_write,
  altcp_default_output,
  altcp_mbedtls_mss,
  altcp_mbedtls_sndbuf,
  altcp_default_sndqueuelen,
  altcp_default_nagle_disable,
  altcp_default_nagle_enable,
  altcp_default_nagle_disabled,
  altcp_default_setprio,
  altcp_mbedtls_dealloc,
  altcp_default_get_tcp_addrinfo,
  altcp_default_get_ip,
  altcp_default_get_port
#if LWIP_TCP_KEEPALIVE
  , altcp_default_keepalive_disable
  , altcp_default_keepalive_enable
#endif
#ifdef LWIP_DEBUG
  , altcp_default_dbg_get_tcp_state
#endif
};

#endif /* LWIP_ALTCP_TLS && LWIP_ALTCP_TLS_MBEDTLS */
#endif /* LWIP_ALTCP */
