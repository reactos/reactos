/**
 * @file
 * @defgroup altcp Application layered TCP Functions
 * @ingroup altcp_api
 *
 * This file contains the common functions for altcp to work.
 * For more details see @ref altcp_api.
 */

/**
 * @defgroup altcp_api Application layered TCP Introduction
 * @ingroup callbackstyle_api
 *
 * Overview
 * --------
 * altcp (application layered TCP connection API; to be used from TCPIP thread)
 * is an abstraction layer that prevents applications linking hard against the
 * @ref tcp.h functions while providing the same functionality. It is used to
 * e.g. add SSL/TLS (see LWIP_ALTCP_TLS) or proxy-connect support to an application
 * written for the tcp callback API without that application knowing the
 * protocol details.
 *
 * * This interface mimics the tcp callback API to the application while preventing
 *   direct linking (much like virtual functions).
 * * This way, an application can make use of other application layer protocols
 *   on top of TCP without knowing the details (e.g. TLS, proxy connection).
 * * This is achieved by simply including "lwip/altcp.h" instead of "lwip/tcp.h",
 *   replacing "struct tcp_pcb" with "struct altcp_pcb" and prefixing all functions
 *   with "altcp_" instead of "tcp_".
 *
 * With altcp support disabled (LWIP_ALTCP==0), applications written against the
 * altcp API can still be compiled but are directly linked against the tcp.h
 * callback API and then cannot use layered protocols. To minimize code changes
 * in this case, the use of altcp_allocators is strongly suggested.
 *
 * Usage
 * -----
 * To make use of this API from an existing tcp raw API application:
 * * Include "lwip/altcp.h" instead of "lwip/tcp.h"
 * * Replace "struct tcp_pcb" with "struct altcp_pcb"
 * * Prefix all called tcp API functions with "altcp_" instead of "tcp_" to link
 *   against the altcp functions
 * * @ref altcp_new (and @ref altcp_new_ip_type/@ref altcp_new_ip6) take
 *   an @ref altcp_allocator_t as an argument, whereas the original tcp API
 *   functions take no arguments.
 * * An @ref altcp_allocator_t allocator is an object that holds a pointer to an
 *   allocator object and a corresponding state (e.g. for TLS, the corresponding
 *   state may hold certificates or keys). This way, the application does not
 *   even need to know if it uses TLS or pure TCP, this is handled at runtime
 *   by passing a specific allocator.
 * * An application can alternatively bind hard to the altcp_tls API by calling
 *   @ref altcp_tls_new or @ref altcp_tls_wrap.
 * * The TLS layer is not directly implemented by lwIP, but a port to mbedTLS is
 *   provided.
 * * Another altcp layer is proxy-connect to use TLS behind a HTTP proxy (see
 *   @ref altcp_proxyconnect.h)
 *
 * altcp_allocator_t
 * -----------------
 * An altcp allocator is created by the application by combining an allocator
 * callback function and a corresponding state, e.g.:\code{.c}
 * static const unsigned char cert[] = {0x2D, ... (see mbedTLS doc for how to create this)};
 * struct altcp_tls_config * conf = altcp_tls_create_config_client(cert, sizeof(cert));
 * altcp_allocator_t tls_allocator = {
 *   altcp_tls_alloc, conf
 * };
 * \endcode
 *
 *
 * struct altcp_tls_config
 * -----------------------
 * The struct altcp_tls_config holds state that is needed to create new TLS client
 * or server connections (e.g. certificates and private keys).
 *
 * It is not defined by lwIP itself but by the TLS port (e.g. altcp_tls to mbedTLS
 * adaption). However, the parameters used to create it are defined in @ref
 * altcp_tls.h (see @ref altcp_tls_create_config_server_privkey_cert for servers
 * and @ref altcp_tls_create_config_client/@ref altcp_tls_create_config_client_2wayauth
 * for clients).
 *
 * For mbedTLS, ensure that certificates can be parsed by 'mbedtls_x509_crt_parse()' and
 * private keys can be parsed by 'mbedtls_pk_parse_key()'.
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
#include "lwip/priv/altcp_priv.h"
#include "lwip/altcp_tcp.h"
#include "lwip/tcp.h"
#include "lwip/mem.h"

#include <string.h>

extern const struct altcp_functions altcp_tcp_functions;

/**
 * For altcp layer implementations only: allocate a new struct altcp_pcb from the pool
 * and zero the memory
 */
struct altcp_pcb *
altcp_alloc(void)
{
  struct altcp_pcb *ret = (struct altcp_pcb *)memp_malloc(MEMP_ALTCP_PCB);
  if (ret != NULL) {
    memset(ret, 0, sizeof(struct altcp_pcb));
  }
  return ret;
}

/**
 * For altcp layer implementations only: return a struct altcp_pcb to the pool
 */
void
altcp_free(struct altcp_pcb *conn)
{
  if (conn) {
    if (conn->fns && conn->fns->dealloc) {
      conn->fns->dealloc(conn);
    }
    memp_free(MEMP_ALTCP_PCB, conn);
  }
}

/**
 * @ingroup altcp
 * altcp_new_ip6: @ref altcp_new for IPv6 
 */
struct altcp_pcb *
altcp_new_ip6(altcp_allocator_t *allocator)
{
  return altcp_new_ip_type(allocator, IPADDR_TYPE_V6);
}

/** 
 * @ingroup altcp
 * altcp_new: @ref altcp_new for IPv4 
 */
struct altcp_pcb *
altcp_new(altcp_allocator_t *allocator)
{
  return altcp_new_ip_type(allocator, IPADDR_TYPE_V4);
}

/**
 * @ingroup altcp
 * altcp_new_ip_type: called by applications to allocate a new pcb with the help of an
 * allocator function.
 *
 * @param allocator allocator function and argument
 * @param ip_type IP version of the pcb (@ref lwip_ip_addr_type)
 * @return a new altcp_pcb or NULL on error
 */
struct altcp_pcb *
altcp_new_ip_type(altcp_allocator_t *allocator, u8_t ip_type)
{
  struct altcp_pcb *conn;
  if (allocator == NULL) {
    /* no allocator given, create a simple TCP connection */
    return altcp_tcp_new_ip_type(ip_type);
  }
  if (allocator->alloc == NULL) {
    /* illegal allocator */
    return NULL;
  }
  conn = allocator->alloc(allocator->arg, ip_type);
  if (conn == NULL) {
    /* allocation failed */
    return NULL;
  }
  return conn;
}

/**
 * @ingroup altcp
 * @see tcp_arg()
 */
void
altcp_arg(struct altcp_pcb *conn, void *arg)
{
  if (conn) {
    conn->arg = arg;
  }
}

/**
 * @ingroup altcp
 * @see tcp_accept()
 */
void
altcp_accept(struct altcp_pcb *conn, altcp_accept_fn accept)
{
  if (conn != NULL) {
    conn->accept = accept;
  }
}

/**
 * @ingroup altcp
 * @see tcp_recv()
 */
void
altcp_recv(struct altcp_pcb *conn, altcp_recv_fn recv)
{
  if (conn) {
    conn->recv = recv;
  }
}

/**
 * @ingroup altcp
 * @see tcp_sent()
 */
void
altcp_sent(struct altcp_pcb *conn, altcp_sent_fn sent)
{
  if (conn) {
    conn->sent = sent;
  }
}

/**
 * @ingroup altcp
 * @see tcp_poll()
 */
void
altcp_poll(struct altcp_pcb *conn, altcp_poll_fn poll, u8_t interval)
{
  if (conn) {
    conn->poll = poll;
    conn->pollinterval = interval;
    if (conn->fns && conn->fns->set_poll) {
      conn->fns->set_poll(conn, interval);
    }
  }
}

/**
 * @ingroup altcp
 * @see tcp_err()
 */
void
altcp_err(struct altcp_pcb *conn, altcp_err_fn err)
{
  if (conn) {
    conn->err = err;
  }
}

/* Generic functions calling the "virtual" ones */

/**
 * @ingroup altcp
 * @see tcp_recved()
 */
void
altcp_recved(struct altcp_pcb *conn, u16_t len)
{
  if (conn && conn->fns && conn->fns->recved) {
    conn->fns->recved(conn, len);
  }
}

/**
 * @ingroup altcp
 * @see tcp_bind()
 */
err_t
altcp_bind(struct altcp_pcb *conn, const ip_addr_t *ipaddr, u16_t port)
{
  if (conn && conn->fns && conn->fns->bind) {
    return conn->fns->bind(conn, ipaddr, port);
  }
  return ERR_VAL;
}

/**
 * @ingroup altcp
 * @see tcp_connect()
 */
err_t
altcp_connect(struct altcp_pcb *conn, const ip_addr_t *ipaddr, u16_t port, altcp_connected_fn connected)
{
  if (conn && conn->fns && conn->fns->connect) {
    return conn->fns->connect(conn, ipaddr, port, connected);
  }
  return ERR_VAL;
}

/**
 * @ingroup altcp
 * @see tcp_listen_with_backlog_and_err()
 */
struct altcp_pcb *
altcp_listen_with_backlog_and_err(struct altcp_pcb *conn, u8_t backlog, err_t *err)
{
  if (conn && conn->fns && conn->fns->listen) {
    return conn->fns->listen(conn, backlog, err);
  }
  return NULL;
}

/**
 * @ingroup altcp
 * @see tcp_abort()
 */
void
altcp_abort(struct altcp_pcb *conn)
{
  if (conn && conn->fns && conn->fns->abort) {
    conn->fns->abort(conn);
  }
}

/**
 * @ingroup altcp
 * @see tcp_close()
 */
err_t
altcp_close(struct altcp_pcb *conn)
{
  if (conn && conn->fns && conn->fns->close) {
    return conn->fns->close(conn);
  }
  return ERR_VAL;
}

/**
 * @ingroup altcp
 * @see tcp_shutdown()
 */
err_t
altcp_shutdown(struct altcp_pcb *conn, int shut_rx, int shut_tx)
{
  if (conn && conn->fns && conn->fns->shutdown) {
    return conn->fns->shutdown(conn, shut_rx, shut_tx);
  }
  return ERR_VAL;
}

/**
 * @ingroup altcp
 * @see tcp_write()
 */
err_t
altcp_write(struct altcp_pcb *conn, const void *dataptr, u16_t len, u8_t apiflags)
{
  if (conn && conn->fns && conn->fns->write) {
    return conn->fns->write(conn, dataptr, len, apiflags);
  }
  return ERR_VAL;
}

/**
 * @ingroup altcp
 * @see tcp_output()
 */
err_t
altcp_output(struct altcp_pcb *conn)
{
  if (conn && conn->fns && conn->fns->output) {
    return conn->fns->output(conn);
  }
  return ERR_VAL;
}

/**
 * @ingroup altcp
 * @see tcp_mss()
 */
u16_t
altcp_mss(struct altcp_pcb *conn)
{
  if (conn && conn->fns && conn->fns->mss) {
    return conn->fns->mss(conn);
  }
  return 0;
}

/**
 * @ingroup altcp
 * @see tcp_sndbuf()
 */
u16_t
altcp_sndbuf(struct altcp_pcb *conn)
{
  if (conn && conn->fns && conn->fns->sndbuf) {
    return conn->fns->sndbuf(conn);
  }
  return 0;
}

/**
 * @ingroup altcp
 * @see tcp_sndqueuelen()
 */
u16_t
altcp_sndqueuelen(struct altcp_pcb *conn)
{
  if (conn && conn->fns && conn->fns->sndqueuelen) {
    return conn->fns->sndqueuelen(conn);
  }
  return 0;
}

void
altcp_nagle_disable(struct altcp_pcb *conn)
{
  if (conn && conn->fns && conn->fns->nagle_disable) {
    conn->fns->nagle_disable(conn);
  }
}

void
altcp_nagle_enable(struct altcp_pcb *conn)
{
  if (conn && conn->fns && conn->fns->nagle_enable) {
    conn->fns->nagle_enable(conn);
  }
}

int
altcp_nagle_disabled(struct altcp_pcb *conn)
{
  if (conn && conn->fns && conn->fns->nagle_disabled) {
    return conn->fns->nagle_disabled(conn);
  }
  return 0;
}

/**
 * @ingroup altcp
 * @see tcp_setprio()
 */
void
altcp_setprio(struct altcp_pcb *conn, u8_t prio)
{
  if (conn && conn->fns && conn->fns->setprio) {
    conn->fns->setprio(conn, prio);
  }
}

err_t
altcp_get_tcp_addrinfo(struct altcp_pcb *conn, int local, ip_addr_t *addr, u16_t *port)
{
  if (conn && conn->fns && conn->fns->addrinfo) {
    return conn->fns->addrinfo(conn, local, addr, port);
  }
  return ERR_VAL;
}

ip_addr_t *
altcp_get_ip(struct altcp_pcb *conn, int local)
{
  if (conn && conn->fns && conn->fns->getip) {
    return conn->fns->getip(conn, local);
  }
  return NULL;
}

u16_t
altcp_get_port(struct altcp_pcb *conn, int local)
{
  if (conn && conn->fns && conn->fns->getport) {
    return conn->fns->getport(conn, local);
  }
  return 0;
}

#if LWIP_TCP_KEEPALIVE
void
altcp_keepalive_disable(struct altcp_pcb *conn)
{
  if (conn && conn->fns && conn->fns->keepalive_disable) {
    conn->fns->keepalive_disable(conn);
  }
}

void
altcp_keepalive_enable(struct altcp_pcb *conn, u32_t idle, u32_t intvl, u32_t count)
{
  if (conn && conn->fns && conn->fns->keepalive_enable) {
      conn->fns->keepalive_enable(conn, idle, intvl, count);
  }
}
#endif

#ifdef LWIP_DEBUG
enum tcp_state
altcp_dbg_get_tcp_state(struct altcp_pcb *conn)
{
  if (conn && conn->fns && conn->fns->dbg_get_tcp_state) {
    return conn->fns->dbg_get_tcp_state(conn);
  }
  return CLOSED;
}
#endif

/* Default implementations for the "virtual" functions */

void
altcp_default_set_poll(struct altcp_pcb *conn, u8_t interval)
{
  if (conn && conn->inner_conn) {
    altcp_poll(conn->inner_conn, conn->poll, interval);
  }
}

void
altcp_default_recved(struct altcp_pcb *conn, u16_t len)
{
  if (conn && conn->inner_conn) {
    altcp_recved(conn->inner_conn, len);
  }
}

err_t
altcp_default_bind(struct altcp_pcb *conn, const ip_addr_t *ipaddr, u16_t port)
{
  if (conn && conn->inner_conn) {
    return altcp_bind(conn->inner_conn, ipaddr, port);
  }
  return ERR_VAL;
}

err_t
altcp_default_shutdown(struct altcp_pcb *conn, int shut_rx, int shut_tx)
{
  if (conn) {
    if (shut_rx && shut_tx && conn->fns && conn->fns->close) {
      /* default shutdown for both sides is close */
      return conn->fns->close(conn);
    }
    if (conn->inner_conn) {
      return altcp_shutdown(conn->inner_conn, shut_rx, shut_tx);
    }
  }
  return ERR_VAL;
}

err_t
altcp_default_write(struct altcp_pcb *conn, const void *dataptr, u16_t len, u8_t apiflags)
{
  if (conn && conn->inner_conn) {
    return altcp_write(conn->inner_conn, dataptr, len, apiflags);
  }
  return ERR_VAL;
}

err_t
altcp_default_output(struct altcp_pcb *conn)
{
  if (conn && conn->inner_conn) {
    return altcp_output(conn->inner_conn);
  }
  return ERR_VAL;
}

u16_t
altcp_default_mss(struct altcp_pcb *conn)
{
  if (conn && conn->inner_conn) {
    return altcp_mss(conn->inner_conn);
  }
  return 0;
}

u16_t
altcp_default_sndbuf(struct altcp_pcb *conn)
{
  if (conn && conn->inner_conn) {
    return altcp_sndbuf(conn->inner_conn);
  }
  return 0;
}

u16_t
altcp_default_sndqueuelen(struct altcp_pcb *conn)
{
  if (conn && conn->inner_conn) {
    return altcp_sndqueuelen(conn->inner_conn);
  }
  return 0;
}

void
altcp_default_nagle_disable(struct altcp_pcb *conn)
{
  if (conn && conn->inner_conn) {
    altcp_nagle_disable(conn->inner_conn);
  }
}

void
altcp_default_nagle_enable(struct altcp_pcb *conn)
{
  if (conn && conn->inner_conn) {
    altcp_nagle_enable(conn->inner_conn);
  }
}

int
altcp_default_nagle_disabled(struct altcp_pcb *conn)
{
  if (conn && conn->inner_conn) {
    return altcp_nagle_disabled(conn->inner_conn);
  }
  return 0;
}

void
altcp_default_setprio(struct altcp_pcb *conn, u8_t prio)
{
  if (conn && conn->inner_conn) {
    altcp_setprio(conn->inner_conn, prio);
  }
}

void
altcp_default_dealloc(struct altcp_pcb *conn)
{
  LWIP_UNUSED_ARG(conn);
  /* nothing to do */
}

err_t
altcp_default_get_tcp_addrinfo(struct altcp_pcb *conn, int local, ip_addr_t *addr, u16_t *port)
{
  if (conn && conn->inner_conn) {
    return altcp_get_tcp_addrinfo(conn->inner_conn, local, addr, port);
  }
  return ERR_VAL;
}

ip_addr_t *
altcp_default_get_ip(struct altcp_pcb *conn, int local)
{
  if (conn && conn->inner_conn) {
    return altcp_get_ip(conn->inner_conn, local);
  }
  return NULL;
}

u16_t
altcp_default_get_port(struct altcp_pcb *conn, int local)
{
  if (conn && conn->inner_conn) {
    return altcp_get_port(conn->inner_conn, local);
  }
  return 0;
}

#if LWIP_TCP_KEEPALIVE
void
altcp_default_keepalive_disable(struct altcp_pcb *conn)
{
  if (conn && conn->inner_conn) {
    altcp_keepalive_disable(conn->inner_conn);
  }
}

void
altcp_default_keepalive_enable(struct altcp_pcb *conn, u32_t idle, u32_t intvl, u32_t count)
{
  if (conn && conn->inner_conn) {
      altcp_keepalive_enable(conn->inner_conn, idle, intvl, count);
  }
}
#endif

#ifdef LWIP_DEBUG
enum tcp_state
altcp_default_dbg_get_tcp_state(struct altcp_pcb *conn)
{
  if (conn && conn->inner_conn) {
    return altcp_dbg_get_tcp_state(conn->inner_conn);
  }
  return CLOSED;
}
#endif


#endif /* LWIP_ALTCP */
