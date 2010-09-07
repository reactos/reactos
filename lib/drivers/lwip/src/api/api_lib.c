/**
 * @file
 * Sequential API External module
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

/* This is the part of the API that is linked with
   the application */

#include "lwip/opt.h"

#if LWIP_NETCONN /* don't build if not configured for use in lwipopts.h */

#include "lwip/api.h"
#include "lwip/tcpip.h"
#include "lwip/memp.h"

#include "lwip/ip.h"
#include "lwip/raw.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"

#include <string.h>

/**
 * Create a new netconn (of a specific type) that has a callback function.
 * The corresponding pcb is also created.
 *
 * @param t the type of 'connection' to create (@see enum netconn_type)
 * @param proto the IP protocol for RAW IP pcbs
 * @param callback a function to call on status changes (RX available, TX'ed)
 * @return a newly allocated struct netconn or
 *         NULL on memory error
 */
struct netconn*
netconn_new_with_proto_and_callback(enum netconn_type t, u8_t proto, netconn_callback callback)
{
  struct netconn *conn;
  struct api_msg msg;

  conn = netconn_alloc(t, callback);
  if (conn != NULL ) {
    msg.function = do_newconn;
    msg.msg.msg.n.proto = proto;
    msg.msg.conn = conn;
    TCPIP_APIMSG(&msg);

    if (conn->err != ERR_OK) {
      LWIP_ASSERT("freeing conn without freeing pcb", conn->pcb.tcp == NULL);
      LWIP_ASSERT("conn has no op_completed", conn->op_completed != SYS_SEM_NULL);
      LWIP_ASSERT("conn has no recvmbox", conn->recvmbox != SYS_MBOX_NULL);
      LWIP_ASSERT("conn->acceptmbox shouldn't exist", conn->acceptmbox == SYS_MBOX_NULL);
      sys_sem_free(conn->op_completed);
      sys_mbox_free(conn->recvmbox);
      memp_free(MEMP_NETCONN, conn);
      return NULL;
    }
  }
  return conn;
}

/**
 * Close a netconn 'connection' and free its resources.
 * UDP and RAW connection are completely closed, TCP pcbs might still be in a waitstate
 * after this returns.
 *
 * @param conn the netconn to delete
 * @return ERR_OK if the connection was deleted
 */
err_t
netconn_delete(struct netconn *conn)
{
  struct api_msg msg;

  /* No ASSERT here because possible to get a (conn == NULL) if we got an accept error */
  if (conn == NULL) {
    return ERR_OK;
  }

  msg.function = do_delconn;
  msg.msg.conn = conn;
  tcpip_apimsg(&msg);

  conn->pcb.tcp = NULL;
  netconn_free(conn);

  return ERR_OK;
}

/**
 * Get the local or remote IP address and port of a netconn.
 * For RAW netconns, this returns the protocol instead of a port!
 *
 * @param conn the netconn to query
 * @param addr a pointer to which to save the IP address
 * @param port a pointer to which to save the port (or protocol for RAW)
 * @param local 1 to get the local IP address, 0 to get the remote one
 * @return ERR_CONN for invalid connections
 *         ERR_OK if the information was retrieved
 */
err_t
netconn_getaddr(struct netconn *conn, struct ip_addr *addr, u16_t *port, u8_t local)
{
  struct api_msg msg;

  LWIP_ERROR("netconn_getaddr: invalid conn", (conn != NULL), return ERR_ARG;);
  LWIP_ERROR("netconn_getaddr: invalid addr", (addr != NULL), return ERR_ARG;);
  LWIP_ERROR("netconn_getaddr: invalid port", (port != NULL), return ERR_ARG;);

  msg.function = do_getaddr;
  msg.msg.conn = conn;
  msg.msg.msg.ad.ipaddr = addr;
  msg.msg.msg.ad.port = port;
  msg.msg.msg.ad.local = local;
  TCPIP_APIMSG(&msg);

  return conn->err;
}

/**
 * Bind a netconn to a specific local IP address and port.
 * Binding one netconn twice might not always be checked correctly!
 *
 * @param conn the netconn to bind
 * @param addr the local IP address to bind the netconn to (use IP_ADDR_ANY
 *             to bind to all addresses)
 * @param port the local port to bind the netconn to (not used for RAW)
 * @return ERR_OK if bound, any other err_t on failure
 */
err_t
netconn_bind(struct netconn *conn, struct ip_addr *addr, u16_t port)
{
  struct api_msg msg;

  LWIP_ERROR("netconn_bind: invalid conn", (conn != NULL), return ERR_ARG;);

  msg.function = do_bind;
  msg.msg.conn = conn;
  msg.msg.msg.bc.ipaddr = addr;
  msg.msg.msg.bc.port = port;
  TCPIP_APIMSG(&msg);
  return conn->err;
}

/**
 * Connect a netconn to a specific remote IP address and port.
 *
 * @param conn the netconn to connect
 * @param addr the remote IP address to connect to
 * @param port the remote port to connect to (no used for RAW)
 * @return ERR_OK if connected, return value of tcp_/udp_/raw_connect otherwise
 */
err_t
netconn_connect(struct netconn *conn, struct ip_addr *addr, u16_t port)
{
  struct api_msg msg;

  LWIP_ERROR("netconn_connect: invalid conn", (conn != NULL), return ERR_ARG;);

  msg.function = do_connect;
  msg.msg.conn = conn;
  msg.msg.msg.bc.ipaddr = addr;
  msg.msg.msg.bc.port = port;
  /* This is the only function which need to not block tcpip_thread */
  tcpip_apimsg(&msg);
  return conn->err;
}

/**
 * Disconnect a netconn from its current peer (only valid for UDP netconns).
 *
 * @param conn the netconn to disconnect
 * @return TODO: return value is not set here...
 */
err_t
netconn_disconnect(struct netconn *conn)
{
  struct api_msg msg;

  LWIP_ERROR("netconn_disconnect: invalid conn", (conn != NULL), return ERR_ARG;);

  msg.function = do_disconnect;
  msg.msg.conn = conn;
  TCPIP_APIMSG(&msg);
  return conn->err;
}

/**
 * Set a TCP netconn into listen mode
 *
 * @param conn the tcp netconn to set to listen mode
 * @param backlog the listen backlog, only used if TCP_LISTEN_BACKLOG==1
 * @return ERR_OK if the netconn was set to listen (UDP and RAW netconns
 *         don't return any error (yet?))
 */
err_t
netconn_listen_with_backlog(struct netconn *conn, u8_t backlog)
{
  struct api_msg msg;

  /* This does no harm. If TCP_LISTEN_BACKLOG is off, backlog is unused. */
  LWIP_UNUSED_ARG(backlog);

  LWIP_ERROR("netconn_listen: invalid conn", (conn != NULL), return ERR_ARG;);

  msg.function = do_listen;
  msg.msg.conn = conn;
#if TCP_LISTEN_BACKLOG
  msg.msg.msg.lb.backlog = backlog;
#endif /* TCP_LISTEN_BACKLOG */
  TCPIP_APIMSG(&msg);
  return conn->err;
}

/**
 * Accept a new connection on a TCP listening netconn.
 *
 * @param conn the TCP listen netconn
 * @return the newly accepted netconn or NULL on timeout
 */
struct netconn *
netconn_accept(struct netconn *conn)
{
  struct netconn *newconn;

  LWIP_ERROR("netconn_accept: invalid conn",       (conn != NULL),                      return NULL;);
  LWIP_ERROR("netconn_accept: invalid acceptmbox", (conn->acceptmbox != SYS_MBOX_NULL), return NULL;);

#if LWIP_SO_RCVTIMEO
  if (sys_arch_mbox_fetch(conn->acceptmbox, (void *)&newconn, conn->recv_timeout) == SYS_ARCH_TIMEOUT) {
    newconn = NULL;
  } else
#else
  sys_arch_mbox_fetch(conn->acceptmbox, (void *)&newconn, 0);
#endif /* LWIP_SO_RCVTIMEO*/
  {
    /* Register event with callback */
    API_EVENT(conn, NETCONN_EVT_RCVMINUS, 0);

#if TCP_LISTEN_BACKLOG
    if (newconn != NULL) {
      /* Let the stack know that we have accepted the connection. */
      struct api_msg msg;
      msg.function = do_recv;
      msg.msg.conn = conn;
      TCPIP_APIMSG(&msg);
    }
#endif /* TCP_LISTEN_BACKLOG */
  }

  return newconn;
}

/**
 * Receive data (in form of a netbuf containing a packet buffer) from a netconn
 *
 * @param conn the netconn from which to receive data
 * @return a new netbuf containing received data or NULL on memory error or timeout
 */
struct netbuf *
netconn_recv(struct netconn *conn)
{
  struct api_msg msg;
  struct netbuf *buf = NULL;
  struct pbuf *p;
  u16_t len;

  LWIP_ERROR("netconn_recv: invalid conn",  (conn != NULL), return NULL;);

  if (conn->recvmbox == SYS_MBOX_NULL) {
    /* @todo: should calling netconn_recv on a TCP listen conn be fatal (ERR_CONN)?? */
    /* TCP listen conns don't have a recvmbox! */
    conn->err = ERR_CONN;
    return NULL;
  }

  if (ERR_IS_FATAL(conn->err)) {
    return NULL;
  }

  if (conn->type == NETCONN_TCP) {
#if LWIP_TCP
    if (conn->state == NETCONN_LISTEN) {
      /* @todo: should calling netconn_recv on a TCP listen conn be fatal?? */
      conn->err = ERR_CONN;
      return NULL;
    }

    buf = memp_malloc(MEMP_NETBUF);

    if (buf == NULL) {
      conn->err = ERR_MEM;
      return NULL;
    }

#if LWIP_SO_RCVTIMEO
    if (sys_arch_mbox_fetch(conn->recvmbox, (void *)&p, conn->recv_timeout)==SYS_ARCH_TIMEOUT) {
      memp_free(MEMP_NETBUF, buf);
      conn->err = ERR_TIMEOUT;
      return NULL;
    }
#else
    sys_arch_mbox_fetch(conn->recvmbox, (void *)&p, 0);
#endif /* LWIP_SO_RCVTIMEO*/

    if (p != NULL) {
      len = p->tot_len;
      SYS_ARCH_DEC(conn->recv_avail, len);
    } else {
      len = 0;
    }

    /* Register event with callback */
    API_EVENT(conn, NETCONN_EVT_RCVMINUS, len);

    /* If we are closed, we indicate that we no longer wish to use the socket */
    if (p == NULL) {
      memp_free(MEMP_NETBUF, buf);
      /* Avoid to lose any previous error code */
      if (conn->err == ERR_OK) {
        conn->err = ERR_CLSD;
      }
      return NULL;
    }

    buf->p = p;
    buf->ptr = p;
    buf->port = 0;
    buf->addr = NULL;

    /* Let the stack know that we have taken the data. */
    msg.function = do_recv;
    msg.msg.conn = conn;
    if (buf != NULL) {
      msg.msg.msg.r.len = buf->p->tot_len;
    } else {
      msg.msg.msg.r.len = 1;
    }
    TCPIP_APIMSG(&msg);
#endif /* LWIP_TCP */
  } else {
#if (LWIP_UDP || LWIP_RAW)
#if LWIP_SO_RCVTIMEO
    if (sys_arch_mbox_fetch(conn->recvmbox, (void *)&buf, conn->recv_timeout)==SYS_ARCH_TIMEOUT) {
      buf = NULL;
    }
#else
    sys_arch_mbox_fetch(conn->recvmbox, (void *)&buf, 0);
#endif /* LWIP_SO_RCVTIMEO*/
    if (buf!=NULL) {
      SYS_ARCH_DEC(conn->recv_avail, buf->p->tot_len);
      /* Register event with callback */
      API_EVENT(conn, NETCONN_EVT_RCVMINUS, buf->p->tot_len);
    }
#endif /* (LWIP_UDP || LWIP_RAW) */
  }

  LWIP_DEBUGF(API_LIB_DEBUG, ("netconn_recv: received %p (err %d)\n", (void *)buf, conn->err));

  return buf;
}

/**
 * Send data (in form of a netbuf) to a specific remote IP address and port.
 * Only to be used for UDP and RAW netconns (not TCP).
 *
 * @param conn the netconn over which to send data
 * @param buf a netbuf containing the data to send
 * @param addr the remote IP address to which to send the data
 * @param port the remote port to which to send the data
 * @return ERR_OK if data was sent, any other err_t on error
 */
err_t
netconn_sendto(struct netconn *conn, struct netbuf *buf, struct ip_addr *addr, u16_t port)
{
  if (buf != NULL) {
    buf->addr = addr;
    buf->port = port;
    return netconn_send(conn, buf);
  }
  return ERR_VAL;
}

/**
 * Send data over a UDP or RAW netconn (that is already connected).
 *
 * @param conn the UDP or RAW netconn over which to send data
 * @param buf a netbuf containing the data to send
 * @return ERR_OK if data was sent, any other err_t on error
 */
err_t
netconn_send(struct netconn *conn, struct netbuf *buf)
{
  struct api_msg msg;

  LWIP_ERROR("netconn_send: invalid conn",  (conn != NULL), return ERR_ARG;);

  LWIP_DEBUGF(API_LIB_DEBUG, ("netconn_send: sending %"U16_F" bytes\n", buf->p->tot_len));
  msg.function = do_send;
  msg.msg.conn = conn;
  msg.msg.msg.b = buf;
  TCPIP_APIMSG(&msg);
  return conn->err;
}

/**
 * Send data over a TCP netconn.
 *
 * @param conn the TCP netconn over which to send data
 * @param dataptr pointer to the application buffer that contains the data to send
 * @param size size of the application data to send
 * @param apiflags combination of following flags :
 * - NETCONN_COPY (0x01) data will be copied into memory belonging to the stack
 * - NETCONN_MORE (0x02) for TCP connection, PSH flag will be set on last segment sent
 * @return ERR_OK if data was sent, any other err_t on error
 */
err_t
netconn_write(struct netconn *conn, const void *dataptr, size_t size, u8_t apiflags)
{
  struct api_msg msg;

  LWIP_ERROR("netconn_write: invalid conn",  (conn != NULL), return ERR_ARG;);
  LWIP_ERROR("netconn_write: invalid conn->type",  (conn->type == NETCONN_TCP), return ERR_VAL;);

  msg.function = do_write;
  msg.msg.conn = conn;
  msg.msg.msg.w.dataptr = dataptr;
  msg.msg.msg.w.apiflags = apiflags;
  msg.msg.msg.w.len = size;
  /* For locking the core: this _can_ be delayed on low memory/low send buffer,
     but if it is, this is done inside api_msg.c:do_write(), so we can use the
     non-blocking version here. */
  TCPIP_APIMSG(&msg);
  return conn->err;
}

/**
 * Close a TCP netconn (doesn't delete it).
 *
 * @param conn the TCP netconn to close
 * @return ERR_OK if the netconn was closed, any other err_t on error
 */
err_t
netconn_close(struct netconn *conn)
{
  struct api_msg msg;

  LWIP_ERROR("netconn_close: invalid conn",  (conn != NULL), return ERR_ARG;);

  msg.function = do_close;
  msg.msg.conn = conn;
  tcpip_apimsg(&msg);
  return conn->err;
}

#if LWIP_IGMP
/**
 * Join multicast groups for UDP netconns.
 *
 * @param conn the UDP netconn for which to change multicast addresses
 * @param multiaddr IP address of the multicast group to join or leave
 * @param interface the IP address of the network interface on which to send
 *                  the igmp message
 * @param join_or_leave flag whether to send a join- or leave-message
 * @return ERR_OK if the action was taken, any err_t on error
 */
err_t
netconn_join_leave_group(struct netconn *conn,
                         struct ip_addr *multiaddr,
                         struct ip_addr *interface,
                         enum netconn_igmp join_or_leave)
{
  struct api_msg msg;

  LWIP_ERROR("netconn_join_leave_group: invalid conn",  (conn != NULL), return ERR_ARG;);

  msg.function = do_join_leave_group;
  msg.msg.conn = conn;
  msg.msg.msg.jl.multiaddr = multiaddr;
  msg.msg.msg.jl.interface = interface;
  msg.msg.msg.jl.join_or_leave = join_or_leave;
  TCPIP_APIMSG(&msg);
  return conn->err;
}
#endif /* LWIP_IGMP */

#if LWIP_DNS
/**
 * Execute a DNS query, only one IP address is returned
 *
 * @param name a string representation of the DNS host name to query
 * @param addr a preallocated struct ip_addr where to store the resolved IP address
 * @return ERR_OK: resolving succeeded
 *         ERR_MEM: memory error, try again later
 *         ERR_ARG: dns client not initialized or invalid hostname
 *         ERR_VAL: dns server response was invalid
 */
err_t
netconn_gethostbyname(const char *name, struct ip_addr *addr)
{
  struct dns_api_msg msg;
  err_t err;
  sys_sem_t sem;

  LWIP_ERROR("netconn_gethostbyname: invalid name", (name != NULL), return ERR_ARG;);
  LWIP_ERROR("netconn_gethostbyname: invalid addr", (addr != NULL), return ERR_ARG;);

  sem = sys_sem_new(0);
  if (sem == SYS_SEM_NULL) {
    return ERR_MEM;
  }

  msg.name = name;
  msg.addr = addr;
  msg.err = &err;
  msg.sem = sem;

  tcpip_callback(do_gethostbyname, &msg);
  sys_sem_wait(sem);
  sys_sem_free(sem);

  return err;
}
#endif /* LWIP_DNS*/

#endif /* LWIP_NETCONN */
