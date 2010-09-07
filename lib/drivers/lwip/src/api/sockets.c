/**
 * @file
 * Sockets BSD-Like API module
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
 * Improved by Marc Boucher <marc@mbsi.ca> and David Haas <dhaas@alum.rpi.edu>
 *
 */

#include "lwip/opt.h"

#if LWIP_SOCKET /* don't build if not configured for use in lwipopts.h */

#include "lwip/sockets.h"
#include "lwip/api.h"
#include "lwip/sys.h"
#include "lwip/igmp.h"
#include "lwip/inet.h"
#include "lwip/tcp.h"
#include "lwip/raw.h"
#include "lwip/udp.h"
#include "lwip/tcpip.h"

#include <string.h>

#define NUM_SOCKETS MEMP_NUM_NETCONN

/** Contains all internal pointers and states used for a socket */
struct lwip_socket {
  /** sockets currently are built on netconns, each socket has one netconn */
  struct netconn *conn;
  /** data that was left from the previous read */
  struct netbuf *lastdata;
  /** offset in the data that was left from the previous read */
  u16_t lastoffset;
  /** number of times data was received, set by event_callback(),
      tested by the receive and select functions */
  s16_t rcvevent;
  /** number of times data was received, set by event_callback(),
      tested by select */
  u16_t sendevent;
  /** socket flags (currently, only used for O_NONBLOCK) */
  u16_t flags;
  /** last error that occurred on this socket */
  int err;
};

/** Description for a task waiting in select */
struct lwip_select_cb {
  /** Pointer to the next waiting task */
  struct lwip_select_cb *next;
  /** readset passed to select */
  fd_set *readset;
  /** writeset passed to select */
  fd_set *writeset;
  /** unimplemented: exceptset passed to select */
  fd_set *exceptset;
  /** don't signal the same semaphore twice: set to 1 when signalled */
  int sem_signalled;
  /** semaphore to wake up a task waiting for select */
  sys_sem_t sem;
};

/** This struct is used to pass data to the set/getsockopt_internal
 * functions running in tcpip_thread context (only a void* is allowed) */
struct lwip_setgetsockopt_data {
  /** socket struct for which to change options */
  struct lwip_socket *sock;
  /** socket index for which to change options */
  int s;
  /** level of the option to process */
  int level;
  /** name of the option to process */
  int optname;
  /** set: value to set the option to
    * get: value of the option is stored here */
  void *optval;
  /** size of *optval */
  socklen_t *optlen;
  /** if an error occures, it is temporarily stored here */
  err_t err;
};

/** The global array of available sockets */
static struct lwip_socket sockets[NUM_SOCKETS];
/** The global list of tasks waiting for select */
static struct lwip_select_cb *select_cb_list;

/** Semaphore protecting the sockets array */
static sys_sem_t socksem;
/** Semaphore protecting select_cb_list */
static sys_sem_t selectsem;

/** Table to quickly map an lwIP error (err_t) to a socket error
  * by using -err as an index */
static const int err_to_errno_table[] = {
  0,             /* ERR_OK          0      No error, everything OK. */
  ENOMEM,        /* ERR_MEM        -1      Out of memory error.     */
  ENOBUFS,       /* ERR_BUF        -2      Buffer error.            */
  ETIMEDOUT,     /* ERR_TIMEOUT    -3      Timeout                  */
  EHOSTUNREACH,  /* ERR_RTE        -4      Routing problem.         */
  ECONNABORTED,  /* ERR_ABRT       -5      Connection aborted.      */
  ECONNRESET,    /* ERR_RST        -6      Connection reset.        */
  ESHUTDOWN,     /* ERR_CLSD       -7      Connection closed.       */
  ENOTCONN,      /* ERR_CONN       -8      Not connected.           */
  EINVAL,        /* ERR_VAL        -9      Illegal value.           */
  EIO,           /* ERR_ARG        -10     Illegal argument.        */
  EADDRINUSE,    /* ERR_USE        -11     Address in use.          */
  -1,            /* ERR_IF         -12     Low-level netif error    */
  -1,            /* ERR_ISCONN     -13     Already connected.       */
  EINPROGRESS    /* ERR_INPROGRESS -14     Operation in progress    */
};

#define ERR_TO_ERRNO_TABLE_SIZE \
  (sizeof(err_to_errno_table)/sizeof(err_to_errno_table[0]))

#define err_to_errno(err) \
  ((unsigned)(-(err)) < ERR_TO_ERRNO_TABLE_SIZE ? \
    err_to_errno_table[-(err)] : EIO)

#ifdef ERRNO
#ifndef set_errno
#define set_errno(err) errno = (err)
#endif
#else
#define set_errno(err)
#endif

#define sock_set_errno(sk, e) do { \
  sk->err = (e); \
  set_errno(sk->err); \
} while (0)

/* Forward delcaration of some functions */
static void event_callback(struct netconn *conn, enum netconn_evt evt, u16_t len);
static void lwip_getsockopt_internal(void *arg);
static void lwip_setsockopt_internal(void *arg);

/**
 * Initialize this module. This function has to be called before any other
 * functions in this module!
 */
void
lwip_socket_init(void)
{
  socksem   = sys_sem_new(1);
  selectsem = sys_sem_new(1);
}

/**
 * Map a externally used socket index to the internal socket representation.
 *
 * @param s externally used socket index
 * @return struct lwip_socket for the socket or NULL if not found
 */
static struct lwip_socket *
get_socket(int s)
{
  struct lwip_socket *sock;

  if ((s < 0) || (s >= NUM_SOCKETS)) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("get_socket(%d): invalid\n", s));
    set_errno(EBADF);
    return NULL;
  }

  sock = &sockets[s];

  if (!sock->conn) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("get_socket(%d): not active\n", s));
    set_errno(EBADF);
    return NULL;
  }

  return sock;
}

/**
 * Allocate a new socket for a given netconn.
 *
 * @param newconn the netconn for which to allocate a socket
 * @return the index of the new socket; -1 on error
 */
static int
alloc_socket(struct netconn *newconn)
{
  int i;

  /* Protect socket array */
  sys_sem_wait(socksem);

  /* allocate a new socket identifier */
  for (i = 0; i < NUM_SOCKETS; ++i) {
    if (!sockets[i].conn) {
      sockets[i].conn       = newconn;
      sockets[i].lastdata   = NULL;
      sockets[i].lastoffset = 0;
      sockets[i].rcvevent   = 0;
      sockets[i].sendevent  = 1; /* TCP send buf is empty */
      sockets[i].flags      = 0;
      sockets[i].err        = 0;
      sys_sem_signal(socksem);
      return i;
    }
  }
  sys_sem_signal(socksem);
  return -1;
}

/* Below this, the well-known socket functions are implemented.
 * Use google.com or opengroup.org to get a good description :-)
 *
 * Exceptions are documented!
 */

int
lwip_accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
  struct lwip_socket *sock, *nsock;
  struct netconn *newconn;
  struct ip_addr naddr;
  u16_t port;
  int newsock;
  struct sockaddr_in sin;
  err_t err;

  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_accept(%d)...\n", s));
  sock = get_socket(s);
  if (!sock)
    return -1;

  if ((sock->flags & O_NONBLOCK) && (sock->rcvevent <= 0)) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_accept(%d): returning EWOULDBLOCK\n", s));
    sock_set_errno(sock, EWOULDBLOCK);
    return -1;
  }

  newconn = netconn_accept(sock->conn);
  if (!newconn) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_accept(%d) failed, err=%d\n", s, sock->conn->err));
    sock_set_errno(sock, err_to_errno(sock->conn->err));
    return -1;
  }

  /* get the IP address and port of the remote host */
  err = netconn_peer(newconn, &naddr, &port);
  if (err != ERR_OK) {
    netconn_delete(newconn);
    sock_set_errno(sock, err_to_errno(err));
    return -1;
  }

  /* Note that POSIX only requires us to check addr is non-NULL. addrlen must
   * not be NULL if addr is valid.
   */
  if (NULL != addr) {
    LWIP_ASSERT("addr valid but addrlen NULL", addrlen != NULL);
    memset(&sin, 0, sizeof(sin));
    sin.sin_len = sizeof(sin);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = naddr.addr;

    if (*addrlen > sizeof(sin))
      *addrlen = sizeof(sin);

    MEMCPY(addr, &sin, *addrlen);
  }

  newsock = alloc_socket(newconn);
  if (newsock == -1) {
    netconn_delete(newconn);
    sock_set_errno(sock, ENFILE);
    return -1;
  }
  LWIP_ASSERT("invalid socket index", (newsock >= 0) && (newsock < NUM_SOCKETS));
  newconn->callback = event_callback;
  nsock = &sockets[newsock];
  LWIP_ASSERT("invalid socket pointer", nsock != NULL);

  sys_sem_wait(socksem);
  /* See event_callback: If data comes in right away after an accept, even
   * though the server task might not have created a new socket yet.
   * In that case, newconn->socket is counted down (newconn->socket--),
   * so nsock->rcvevent is >= 1 here!
   */
  nsock->rcvevent += -1 - newconn->socket;
  newconn->socket = newsock;
  sys_sem_signal(socksem);

  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_accept(%d) returning new sock=%d addr=", s, newsock));
  ip_addr_debug_print(SOCKETS_DEBUG, &naddr);
  LWIP_DEBUGF(SOCKETS_DEBUG, (" port=%"U16_F"\n", port));

  sock_set_errno(sock, 0);
  return newsock;
}

int
lwip_bind(int s, const struct sockaddr *name, socklen_t namelen)
{
  struct lwip_socket *sock;
  struct ip_addr local_addr;
  u16_t local_port;
  err_t err;

  sock = get_socket(s);
  if (!sock)
    return -1;

  LWIP_ERROR("lwip_bind: invalid address", ((namelen == sizeof(struct sockaddr_in)) &&
             ((((const struct sockaddr_in *)name)->sin_family) == AF_INET)),
             sock_set_errno(sock, err_to_errno(ERR_ARG)); return -1;);

  local_addr.addr = ((const struct sockaddr_in *)name)->sin_addr.s_addr;
  local_port = ((const struct sockaddr_in *)name)->sin_port;

  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_bind(%d, addr=", s));
  ip_addr_debug_print(SOCKETS_DEBUG, &local_addr);
  LWIP_DEBUGF(SOCKETS_DEBUG, (" port=%"U16_F")\n", ntohs(local_port)));

  err = netconn_bind(sock->conn, &local_addr, ntohs(local_port));

  if (err != ERR_OK) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_bind(%d) failed, err=%d\n", s, err));
    sock_set_errno(sock, err_to_errno(err));
    return -1;
  }

  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_bind(%d) succeeded\n", s));
  sock_set_errno(sock, 0);
  return 0;
}

int
lwip_close(int s)
{
  struct lwip_socket *sock;

  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_close(%d)\n", s));

  sock = get_socket(s);
  if (!sock) {
    return -1;
  }

  netconn_delete(sock->conn);

  sys_sem_wait(socksem);
  if (sock->lastdata) {
    netbuf_delete(sock->lastdata);
  }
  sock->lastdata   = NULL;
  sock->lastoffset = 0;
  sock->conn       = NULL;
  sock_set_errno(sock, 0);
  sys_sem_signal(socksem);
  return 0;
}

int
lwip_connect(int s, const struct sockaddr *name, socklen_t namelen)
{
  struct lwip_socket *sock;
  err_t err;

  sock = get_socket(s);
  if (!sock)
    return -1;

  LWIP_ERROR("lwip_connect: invalid address", ((namelen == sizeof(struct sockaddr_in)) &&
             ((((const struct sockaddr_in *)name)->sin_family) == AF_INET)),
             sock_set_errno(sock, err_to_errno(ERR_ARG)); return -1;);

  if (((const struct sockaddr_in *)name)->sin_family == AF_UNSPEC) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_connect(%d, AF_UNSPEC)\n", s));
    err = netconn_disconnect(sock->conn);
  } else {
    struct ip_addr remote_addr;
    u16_t remote_port;

    remote_addr.addr = ((const struct sockaddr_in *)name)->sin_addr.s_addr;
    remote_port = ((const struct sockaddr_in *)name)->sin_port;

    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_connect(%d, addr=", s));
    ip_addr_debug_print(SOCKETS_DEBUG, &remote_addr);
    LWIP_DEBUGF(SOCKETS_DEBUG, (" port=%"U16_F")\n", ntohs(remote_port)));

    err = netconn_connect(sock->conn, &remote_addr, ntohs(remote_port));
  }

  if (err != ERR_OK) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_connect(%d) failed, err=%d\n", s, err));
    sock_set_errno(sock, err_to_errno(err));
    return -1;
  }

  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_connect(%d) succeeded\n", s));
  sock_set_errno(sock, 0);
  return 0;
}

/**
 * Set a socket into listen mode.
 * The socket may not have been used for another connection previously.
 *
 * @param s the socket to set to listening mode
 * @param backlog (ATTENTION: need TCP_LISTEN_BACKLOG=1)
 * @return 0 on success, non-zero on failure
 */
int
lwip_listen(int s, int backlog)
{
  struct lwip_socket *sock;
  err_t err;

  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_listen(%d, backlog=%d)\n", s, backlog));

  sock = get_socket(s);
  if (!sock)
    return -1;

  /* limit the "backlog" parameter to fit in an u8_t */
  if (backlog < 0) {
    backlog = 0;
  }
  if (backlog > 0xff) {
    backlog = 0xff;
  }

  err = netconn_listen_with_backlog(sock->conn, backlog);

  if (err != ERR_OK) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_listen(%d) failed, err=%d\n", s, err));
    sock_set_errno(sock, err_to_errno(err));
    return -1;
  }

  sock_set_errno(sock, 0);
  return 0;
}

int
lwip_recvfrom(int s, void *mem, size_t len, int flags,
        struct sockaddr *from, socklen_t *fromlen)
{
  struct lwip_socket *sock;
  struct netbuf      *buf;
  u16_t               buflen, copylen, off = 0;
  struct ip_addr     *addr;
  u16_t               port;
  u8_t                done = 0;

  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_recvfrom(%d, %p, %"SZT_F", 0x%x, ..)\n", s, mem, len, flags));
  sock = get_socket(s);
  if (!sock)
    return -1;

  do {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_recvfrom: top while sock->lastdata=%p\n", (void*)sock->lastdata));
    /* Check if there is data left from the last recv operation. */
    if (sock->lastdata) {
      buf = sock->lastdata;
    } else {
      /* If this is non-blocking call, then check first */
      if (((flags & MSG_DONTWAIT) || (sock->flags & O_NONBLOCK)) && 
          (sock->rcvevent <= 0)) {
        if (off > 0) {
          /* already received data, return that */
          sock_set_errno(sock, 0);
          return off;
        }
        LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_recvfrom(%d): returning EWOULDBLOCK\n", s));
        sock_set_errno(sock, EWOULDBLOCK);
        return -1;
      }

      /* No data was left from the previous operation, so we try to get
      some from the network. */
      sock->lastdata = buf = netconn_recv(sock->conn);
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_recvfrom: netconn_recv netbuf=%p\n", (void*)buf));

      if (!buf) {
        if (off > 0) {
          /* already received data, return that */
          sock_set_errno(sock, 0);
          return off;
        }
        /* We should really do some error checking here. */
        LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_recvfrom(%d): buf == NULL!\n", s));
        sock_set_errno(sock, (((sock->conn->pcb.ip != NULL) && (sock->conn->err == ERR_OK))
          ? ETIMEDOUT : err_to_errno(sock->conn->err)));
        return 0;
      }
    }

    buflen = netbuf_len(buf);
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_recvfrom: buflen=%"U16_F" len=%"SZT_F" off=%"U16_F" sock->lastoffset=%"U16_F"\n",
      buflen, len, off, sock->lastoffset));

    buflen -= sock->lastoffset;

    if (len > buflen) {
      copylen = buflen;
    } else {
      copylen = (u16_t)len;
    }

    /* copy the contents of the received buffer into
    the supplied memory pointer mem */
    netbuf_copy_partial(buf, (u8_t*)mem + off, copylen, sock->lastoffset);

    off += copylen;

    if (netconn_type(sock->conn) == NETCONN_TCP) {
      LWIP_ASSERT("invalid copylen, len would underflow", len >= copylen);
      len -= copylen;
      if ( (len <= 0) || 
           (buf->p->flags & PBUF_FLAG_PUSH) || 
           (sock->rcvevent <= 0) || 
           ((flags & MSG_PEEK)!=0)) {
        done = 1;
      }
    } else {
      done = 1;
    }

    /* Check to see from where the data was.*/
    if (done) {
      if (from && fromlen) {
        struct sockaddr_in sin;

        if (netconn_type(sock->conn) == NETCONN_TCP) {
          addr = (struct ip_addr*)&(sin.sin_addr.s_addr);
          netconn_getaddr(sock->conn, addr, &port, 0);
        } else {
          addr = netbuf_fromaddr(buf);
          port = netbuf_fromport(buf);
        }

        memset(&sin, 0, sizeof(sin));
        sin.sin_len = sizeof(sin);
        sin.sin_family = AF_INET;
        sin.sin_port = htons(port);
        sin.sin_addr.s_addr = addr->addr;

        if (*fromlen > sizeof(sin)) {
          *fromlen = sizeof(sin);
        }

        MEMCPY(from, &sin, *fromlen);

        LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_recvfrom(%d): addr=", s));
        ip_addr_debug_print(SOCKETS_DEBUG, addr);
        LWIP_DEBUGF(SOCKETS_DEBUG, (" port=%"U16_F" len=%"U16_F"\n", port, off));
      } else {
  #if SOCKETS_DEBUG
        struct sockaddr_in sin;

        if (netconn_type(sock->conn) == NETCONN_TCP) {
          addr = (struct ip_addr*)&(sin.sin_addr.s_addr);
          netconn_getaddr(sock->conn, addr, &port, 0);
        } else {
          addr = netbuf_fromaddr(buf);
          port = netbuf_fromport(buf);
        }

        LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_recvfrom(%d): addr=", s));
        ip_addr_debug_print(SOCKETS_DEBUG, addr);
        LWIP_DEBUGF(SOCKETS_DEBUG, (" port=%"U16_F" len=%"U16_F"\n", port, off));
  #endif /*  SOCKETS_DEBUG */
      }
    }

    /* If we don't peek the incoming message... */
    if ((flags & MSG_PEEK)==0) {
      /* If this is a TCP socket, check if there is data left in the
         buffer. If so, it should be saved in the sock structure for next
         time around. */
      if ((netconn_type(sock->conn) == NETCONN_TCP) && (buflen - copylen > 0)) {
        sock->lastdata = buf;
        sock->lastoffset += copylen;
        LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_recvfrom: lastdata now netbuf=%p\n", (void*)buf));
      } else {
        sock->lastdata = NULL;
        sock->lastoffset = 0;
        LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_recvfrom: deleting netbuf=%p\n", (void*)buf));
        netbuf_delete(buf);
      }
    }
  } while (!done);

  sock_set_errno(sock, 0);
  return off;
}

int
lwip_read(int s, void *mem, size_t len)
{
  return lwip_recvfrom(s, mem, len, 0, NULL, NULL);
}

int
lwip_recv(int s, void *mem, size_t len, int flags)
{
  return lwip_recvfrom(s, mem, len, flags, NULL, NULL);
}

int
lwip_send(int s, const void *data, size_t size, int flags)
{
  struct lwip_socket *sock;
  err_t err;

  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_send(%d, data=%p, size=%"SZT_F", flags=0x%x)\n",
                              s, data, size, flags));

  sock = get_socket(s);
  if (!sock)
    return -1;

  if (sock->conn->type != NETCONN_TCP) {
#if (LWIP_UDP || LWIP_RAW)
    return lwip_sendto(s, data, size, flags, NULL, 0);
#else
    sock_set_errno(sock, err_to_errno(ERR_ARG));
    return -1;
#endif /* (LWIP_UDP || LWIP_RAW) */
  }

  err = netconn_write(sock->conn, data, size, NETCONN_COPY | ((flags & MSG_MORE)?NETCONN_MORE:0));

  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_send(%d) err=%d size=%"SZT_F"\n", s, err, size));
  sock_set_errno(sock, err_to_errno(err));
  return (err == ERR_OK ? (int)size : -1);
}

int
lwip_sendto(int s, const void *data, size_t size, int flags,
       const struct sockaddr *to, socklen_t tolen)
{
  struct lwip_socket *sock;
  struct ip_addr remote_addr;
  err_t err;
  u16_t short_size;
#if !LWIP_TCPIP_CORE_LOCKING
  struct netbuf buf;
  u16_t remote_port;
#endif

  sock = get_socket(s);
  if (!sock)
    return -1;

  if (sock->conn->type == NETCONN_TCP) {
#if LWIP_TCP
    return lwip_send(s, data, size, flags);
#else
    sock_set_errno(sock, err_to_errno(ERR_ARG));
    return -1;
#endif /* LWIP_TCP */
  }

  LWIP_ASSERT("lwip_sendto: size must fit in u16_t", size <= 0xffff);
  short_size = (u16_t)size;
  LWIP_ERROR("lwip_sendto: invalid address", (((to == NULL) && (tolen == 0)) ||
             ((tolen == sizeof(struct sockaddr_in)) &&
             ((((const struct sockaddr_in *)to)->sin_family) == AF_INET))),
             sock_set_errno(sock, err_to_errno(ERR_ARG)); return -1;);

#if LWIP_TCPIP_CORE_LOCKING
  /* Should only be consider like a sample or a simple way to experiment this option (no check of "to" field...) */
  { struct pbuf* p;
  
    p = pbuf_alloc(PBUF_TRANSPORT, 0, PBUF_REF);
    if (p == NULL) {
      err = ERR_MEM;
    } else {
      p->payload = (void*)data;
      p->len = p->tot_len = short_size;
      
      remote_addr.addr = ((const struct sockaddr_in *)to)->sin_addr.s_addr;
      
      LOCK_TCPIP_CORE();
      if (sock->conn->type==NETCONN_RAW) {
        err = sock->conn->err = raw_sendto(sock->conn->pcb.raw, p, &remote_addr);
      } else {
        err = sock->conn->err = udp_sendto(sock->conn->pcb.udp, p, &remote_addr, ntohs(((const struct sockaddr_in *)to)->sin_port));
      }
      UNLOCK_TCPIP_CORE();
      
      pbuf_free(p);
    }
  }
#else
  /* initialize a buffer */
  buf.p = buf.ptr = NULL;
  if (to) {
    remote_addr.addr = ((const struct sockaddr_in *)to)->sin_addr.s_addr;
    remote_port      = ntohs(((const struct sockaddr_in *)to)->sin_port);
    buf.addr         = &remote_addr;
    buf.port         = remote_port;
  } else {
    remote_addr.addr = 0;
    remote_port      = 0;
    buf.addr         = NULL;
    buf.port         = 0;
  }

  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_sendto(%d, data=%p, short_size=%d"U16_F", flags=0x%x to=",
              s, data, short_size, flags));
  ip_addr_debug_print(SOCKETS_DEBUG, &remote_addr);
  LWIP_DEBUGF(SOCKETS_DEBUG, (" port=%"U16_F"\n", remote_port));

  /* make the buffer point to the data that should be sent */
#if LWIP_NETIF_TX_SINGLE_PBUF
  /* Allocate a new netbuf and copy the data into it. */
  if (netbuf_alloc(&buf, short_size) == NULL) {
    err = ERR_MEM;
  } else {
    err = netbuf_take(&buf, data, short_size);
  }
#else /* LWIP_NETIF_TX_SINGLE_PBUF */
  err = netbuf_ref(&buf, data, short_size);
#endif /* LWIP_NETIF_TX_SINGLE_PBUF */
  if (err == ERR_OK) {
    /* send the data */
    err = netconn_send(sock->conn, &buf);
  }

  /* deallocated the buffer */
  netbuf_free(&buf);
#endif /* LWIP_TCPIP_CORE_LOCKING */
  sock_set_errno(sock, err_to_errno(err));
  return (err == ERR_OK ? short_size : -1);
}

int
lwip_socket(int domain, int type, int protocol)
{
  struct netconn *conn;
  int i;

  LWIP_UNUSED_ARG(domain);

  /* create a netconn */
  switch (type) {
  case SOCK_RAW:
    conn = netconn_new_with_proto_and_callback(NETCONN_RAW, (u8_t)protocol, event_callback);
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_socket(%s, SOCK_RAW, %d) = ",
                                 domain == PF_INET ? "PF_INET" : "UNKNOWN", protocol));
    break;
  case SOCK_DGRAM:
    conn = netconn_new_with_callback( (protocol == IPPROTO_UDPLITE) ?
                 NETCONN_UDPLITE : NETCONN_UDP, event_callback);
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_socket(%s, SOCK_DGRAM, %d) = ",
                                 domain == PF_INET ? "PF_INET" : "UNKNOWN", protocol));
    break;
  case SOCK_STREAM:
    conn = netconn_new_with_callback(NETCONN_TCP, event_callback);
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_socket(%s, SOCK_STREAM, %d) = ",
                                 domain == PF_INET ? "PF_INET" : "UNKNOWN", protocol));
    break;
  default:
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_socket(%d, %d/UNKNOWN, %d) = -1\n",
                                 domain, type, protocol));
    set_errno(EINVAL);
    return -1;
  }

  if (!conn) {
    LWIP_DEBUGF(SOCKETS_DEBUG, ("-1 / ENOBUFS (could not create netconn)\n"));
    set_errno(ENOBUFS);
    return -1;
  }

  i = alloc_socket(conn);

  if (i == -1) {
    netconn_delete(conn);
    set_errno(ENFILE);
    return -1;
  }
  conn->socket = i;
  LWIP_DEBUGF(SOCKETS_DEBUG, ("%d\n", i));
  set_errno(0);
  return i;
}

int
lwip_write(int s, const void *data, size_t size)
{
  return lwip_send(s, data, size, 0);
}

/**
 * Go through the readset and writeset lists and see which socket of the sockets
 * set in the sets has events. On return, readset, writeset and exceptset have
 * the sockets enabled that had events.
 *
 * exceptset is not used for now!!!
 *
 * @param maxfdp1 the highest socket index in the sets
 * @param readset in: set of sockets to check for read events;
 *                out: set of sockets that had read events
 * @param writeset in: set of sockets to check for write events;
 *                 out: set of sockets that had write events
 * @param exceptset not yet implemented
 * @return number of sockets that had events (read+write)
 */
static int
lwip_selscan(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset)
{
  int i, nready = 0;
  fd_set lreadset, lwriteset, lexceptset;
  struct lwip_socket *p_sock;
  
  FD_ZERO(&lreadset);
  FD_ZERO(&lwriteset);
  FD_ZERO(&lexceptset);
  
  /* Go through each socket in each list to count number of sockets which
  currently match */
  for(i = 0; i < maxfdp1; i++) {
    if (FD_ISSET(i, readset)) {
      /* See if netconn of this socket is ready for read */
      p_sock = get_socket(i);
      if (p_sock && (p_sock->lastdata || (p_sock->rcvevent > 0))) {
        FD_SET(i, &lreadset);
        LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_selscan: fd=%d ready for reading\n", i));
        nready++;
      }
    }
    if (FD_ISSET(i, writeset)) {
      /* See if netconn of this socket is ready for write */
      p_sock = get_socket(i);
      if (p_sock && p_sock->sendevent) {
        FD_SET(i, &lwriteset);
        LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_selscan: fd=%d ready for writing\n", i));
        nready++;
      }
    }
  }
  *readset = lreadset;
  *writeset = lwriteset;
  FD_ZERO(exceptset);
  
  return nready;
}


/**
 * Processing exceptset is not yet implemented.
 */
int
lwip_select(int maxfdp1, fd_set *readset, fd_set *writeset, fd_set *exceptset,
               struct timeval *timeout)
{
  int i;
  int nready;
  fd_set lreadset, lwriteset, lexceptset;
  u32_t msectimeout;
  struct lwip_select_cb select_cb;
  struct lwip_select_cb *p_selcb;

  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_select(%d, %p, %p, %p, tvsec=%ld tvusec=%ld)\n",
                  maxfdp1, (void *)readset, (void *) writeset, (void *) exceptset,
                  timeout ? (long)timeout->tv_sec : (long)-1,
                  timeout ? (long)timeout->tv_usec : (long)-1));

  select_cb.next = 0;
  select_cb.readset = readset;
  select_cb.writeset = writeset;
  select_cb.exceptset = exceptset;
  select_cb.sem_signalled = 0;

  /* Protect ourselves searching through the list */
  sys_sem_wait(selectsem);

  if (readset)
    lreadset = *readset;
  else
    FD_ZERO(&lreadset);
  if (writeset)
    lwriteset = *writeset;
  else
    FD_ZERO(&lwriteset);
  if (exceptset)
    lexceptset = *exceptset;
  else
    FD_ZERO(&lexceptset);

  /* Go through each socket in each list to count number of sockets which
     currently match */
  nready = lwip_selscan(maxfdp1, &lreadset, &lwriteset, &lexceptset);

  /* If we don't have any current events, then suspend if we are supposed to */
  if (!nready) {
    if (timeout && timeout->tv_sec == 0 && timeout->tv_usec == 0) {
      sys_sem_signal(selectsem);
      if (readset)
        FD_ZERO(readset);
      if (writeset)
        FD_ZERO(writeset);
      if (exceptset)
        FD_ZERO(exceptset);
  
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_select: no timeout, returning 0\n"));
      set_errno(0);
  
      return 0;
    }
    
    /* add our semaphore to list */
    /* We don't actually need any dynamic memory. Our entry on the
     * list is only valid while we are in this function, so it's ok
     * to use local variables */
    
    select_cb.sem = sys_sem_new(0);
    /* Note that we are still protected */
    /* Put this select_cb on top of list */
    select_cb.next = select_cb_list;
    select_cb_list = &select_cb;
    
    /* Now we can safely unprotect */
    sys_sem_signal(selectsem);
    
    /* Now just wait to be woken */
    if (timeout == 0)
      /* Wait forever */
      msectimeout = 0;
    else {
      msectimeout =  ((timeout->tv_sec * 1000) + ((timeout->tv_usec + 500)/1000));
      if(msectimeout == 0)
        msectimeout = 1;
    }
    
    i = sys_sem_wait_timeout(select_cb.sem, msectimeout);
    
    /* Take us off the list */
    sys_sem_wait(selectsem);
    if (select_cb_list == &select_cb)
      select_cb_list = select_cb.next;
    else
      for (p_selcb = select_cb_list; p_selcb; p_selcb = p_selcb->next) {
        if (p_selcb->next == &select_cb) {
          p_selcb->next = select_cb.next;
          break;
        }
      }
    
    sys_sem_signal(selectsem);
    
    sys_sem_free(select_cb.sem);
    if (i == 0)  {
      /* Timeout */
      if (readset)
        FD_ZERO(readset);
      if (writeset)
        FD_ZERO(writeset);
      if (exceptset)
        FD_ZERO(exceptset);
  
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_select: timeout expired\n"));
      set_errno(0);
  
      return 0;
    }
    
    if (readset)
      lreadset = *readset;
    else
      FD_ZERO(&lreadset);
    if (writeset)
      lwriteset = *writeset;
    else
      FD_ZERO(&lwriteset);
    if (exceptset)
      lexceptset = *exceptset;
    else
      FD_ZERO(&lexceptset);
    
    /* See what's set */
    nready = lwip_selscan(maxfdp1, &lreadset, &lwriteset, &lexceptset);
  } else
    sys_sem_signal(selectsem);
  
  if (readset)
    *readset = lreadset;
  if (writeset)
    *writeset = lwriteset;
  if (exceptset)
    *exceptset = lexceptset;
  
  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_select: nready=%d\n", nready));
  set_errno(0);
  
  return nready;
}

/**
 * Callback registered in the netconn layer for each socket-netconn.
 * Processes recvevent (data available) and wakes up tasks waiting for select.
 */
static void
event_callback(struct netconn *conn, enum netconn_evt evt, u16_t len)
{
  int s;
  struct lwip_socket *sock;
  struct lwip_select_cb *scb;

  LWIP_UNUSED_ARG(len);

  /* Get socket */
  if (conn) {
    s = conn->socket;
    if (s < 0) {
      /* Data comes in right away after an accept, even though
       * the server task might not have created a new socket yet.
       * Just count down (or up) if that's the case and we
       * will use the data later. Note that only receive events
       * can happen before the new socket is set up. */
      sys_sem_wait(socksem);
      if (conn->socket < 0) {
        if (evt == NETCONN_EVT_RCVPLUS) {
          conn->socket--;
        }
        sys_sem_signal(socksem);
        return;
      }
      s = conn->socket;
      sys_sem_signal(socksem);
    }

    sock = get_socket(s);
    if (!sock) {
      return;
    }
  } else {
    return;
  }

  sys_sem_wait(selectsem);
  /* Set event as required */
  switch (evt) {
    case NETCONN_EVT_RCVPLUS:
      sock->rcvevent++;
      break;
    case NETCONN_EVT_RCVMINUS:
      sock->rcvevent--;
      break;
    case NETCONN_EVT_SENDPLUS:
      sock->sendevent = 1;
      break;
    case NETCONN_EVT_SENDMINUS:
      sock->sendevent = 0;
      break;
    default:
      LWIP_ASSERT("unknown event", 0);
      break;
  }
  sys_sem_signal(selectsem);

  /* Now decide if anyone is waiting for this socket */
  /* NOTE: This code is written this way to protect the select link list
     but to avoid a deadlock situation by releasing socksem before
     signalling for the select. This means we need to go through the list
     multiple times ONLY IF a select was actually waiting. We go through
     the list the number of waiting select calls + 1. This list is
     expected to be small. */
  while (1) {
    sys_sem_wait(selectsem);
    for (scb = select_cb_list; scb; scb = scb->next) {
      if (scb->sem_signalled == 0) {
        /* Test this select call for our socket */
        if (scb->readset && FD_ISSET(s, scb->readset))
          if (sock->rcvevent > 0)
            break;
        if (scb->writeset && FD_ISSET(s, scb->writeset))
          if (sock->sendevent)
            break;
      }
    }
    if (scb) {
      scb->sem_signalled = 1;
      sys_sem_signal(scb->sem);
      sys_sem_signal(selectsem);
    } else {
      sys_sem_signal(selectsem);
      break;
    }
  }
}

/**
 * Unimplemented: Close one end of a full-duplex connection.
 * Currently, the full connection is closed.
 */
int
lwip_shutdown(int s, int how)
{
  LWIP_UNUSED_ARG(how);
  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_shutdown(%d, how=%d)\n", s, how));
  return lwip_close(s); /* XXX temporary hack until proper implementation */
}

static int
lwip_getaddrname(int s, struct sockaddr *name, socklen_t *namelen, u8_t local)
{
  struct lwip_socket *sock;
  struct sockaddr_in sin;
  struct ip_addr naddr;

  sock = get_socket(s);
  if (!sock)
    return -1;

  memset(&sin, 0, sizeof(sin));
  sin.sin_len = sizeof(sin);
  sin.sin_family = AF_INET;

  /* get the IP address and port */
  netconn_getaddr(sock->conn, &naddr, &sin.sin_port, local);

  LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getaddrname(%d, addr=", s));
  ip_addr_debug_print(SOCKETS_DEBUG, &naddr);
  LWIP_DEBUGF(SOCKETS_DEBUG, (" port=%"U16_F")\n", sin.sin_port));

  sin.sin_port = htons(sin.sin_port);
  sin.sin_addr.s_addr = naddr.addr;

  if (*namelen > sizeof(sin))
    *namelen = sizeof(sin);

  MEMCPY(name, &sin, *namelen);
  sock_set_errno(sock, 0);
  return 0;
}

int
lwip_getpeername(int s, struct sockaddr *name, socklen_t *namelen)
{
  return lwip_getaddrname(s, name, namelen, 0);
}

int
lwip_getsockname(int s, struct sockaddr *name, socklen_t *namelen)
{
  return lwip_getaddrname(s, name, namelen, 1);
}

int
lwip_getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen)
{
  err_t err = ERR_OK;
  struct lwip_socket *sock = get_socket(s);
  struct lwip_setgetsockopt_data data;

  if (!sock)
    return -1;

  if ((NULL == optval) || (NULL == optlen)) {
    sock_set_errno(sock, EFAULT);
    return -1;
  }

  /* Do length and type checks for the various options first, to keep it readable. */
  switch (level) {
   
/* Level: SOL_SOCKET */
  case SOL_SOCKET:
    switch (optname) {
       
    case SO_ACCEPTCONN:
    case SO_BROADCAST:
    /* UNIMPL case SO_DEBUG: */
    /* UNIMPL case SO_DONTROUTE: */
    case SO_ERROR:
    case SO_KEEPALIVE:
    /* UNIMPL case SO_CONTIMEO: */
    /* UNIMPL case SO_SNDTIMEO: */
#if LWIP_SO_RCVTIMEO
    case SO_RCVTIMEO:
#endif /* LWIP_SO_RCVTIMEO */
#if LWIP_SO_RCVBUF
    case SO_RCVBUF:
#endif /* LWIP_SO_RCVBUF */
    /* UNIMPL case SO_OOBINLINE: */
    /* UNIMPL case SO_SNDBUF: */
    /* UNIMPL case SO_RCVLOWAT: */
    /* UNIMPL case SO_SNDLOWAT: */
#if SO_REUSE
    case SO_REUSEADDR:
    case SO_REUSEPORT:
#endif /* SO_REUSE */
    case SO_TYPE:
    /* UNIMPL case SO_USELOOPBACK: */
      if (*optlen < sizeof(int)) {
        err = EINVAL;
      }
      break;

    case SO_NO_CHECK:
      if (*optlen < sizeof(int)) {
        err = EINVAL;
      }
#if LWIP_UDP
      if ((sock->conn->type != NETCONN_UDP) ||
          ((udp_flags(sock->conn->pcb.udp) & UDP_FLAGS_UDPLITE) != 0)) {
        /* this flag is only available for UDP, not for UDP lite */
        err = EAFNOSUPPORT;
      }
#endif /* LWIP_UDP */
      break;

    default:
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, SOL_SOCKET, UNIMPL: optname=0x%x, ..)\n",
                                  s, optname));
      err = ENOPROTOOPT;
    }  /* switch (optname) */
    break;
                     
/* Level: IPPROTO_IP */
  case IPPROTO_IP:
    switch (optname) {
    /* UNIMPL case IP_HDRINCL: */
    /* UNIMPL case IP_RCVDSTADDR: */
    /* UNIMPL case IP_RCVIF: */
    case IP_TTL:
    case IP_TOS:
      if (*optlen < sizeof(int)) {
        err = EINVAL;
      }
      break;
#if LWIP_IGMP
    case IP_MULTICAST_TTL:
      if (*optlen < sizeof(u8_t)) {
        err = EINVAL;
      }
      break;
    case IP_MULTICAST_IF:
      if (*optlen < sizeof(struct in_addr)) {
        err = EINVAL;
      }
      break;
#endif /* LWIP_IGMP */

    default:
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_IP, UNIMPL: optname=0x%x, ..)\n",
                                  s, optname));
      err = ENOPROTOOPT;
    }  /* switch (optname) */
    break;
         
#if LWIP_TCP
/* Level: IPPROTO_TCP */
  case IPPROTO_TCP:
    if (*optlen < sizeof(int)) {
      err = EINVAL;
      break;
    }
    
    /* If this is no TCP socket, ignore any options. */
    if (sock->conn->type != NETCONN_TCP)
      return 0;

    switch (optname) {
    case TCP_NODELAY:
    case TCP_KEEPALIVE:
#if LWIP_TCP_KEEPALIVE
    case TCP_KEEPIDLE:
    case TCP_KEEPINTVL:
    case TCP_KEEPCNT:
#endif /* LWIP_TCP_KEEPALIVE */
      break;
       
    default:
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_TCP, UNIMPL: optname=0x%x, ..)\n",
                                  s, optname));
      err = ENOPROTOOPT;
    }  /* switch (optname) */
    break;
#endif /* LWIP_TCP */
#if LWIP_UDP && LWIP_UDPLITE
/* Level: IPPROTO_UDPLITE */
  case IPPROTO_UDPLITE:
    if (*optlen < sizeof(int)) {
      err = EINVAL;
      break;
    }
    
    /* If this is no UDP lite socket, ignore any options. */
    if (sock->conn->type != NETCONN_UDPLITE)
      return 0;

    switch (optname) {
    case UDPLITE_SEND_CSCOV:
    case UDPLITE_RECV_CSCOV:
      break;
       
    default:
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_UDPLITE, UNIMPL: optname=0x%x, ..)\n",
                                  s, optname));
      err = ENOPROTOOPT;
    }  /* switch (optname) */
    break;
#endif /* LWIP_UDP && LWIP_UDPLITE*/
/* UNDEFINED LEVEL */
  default:
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, level=0x%x, UNIMPL: optname=0x%x, ..)\n",
                                  s, level, optname));
      err = ENOPROTOOPT;
  }  /* switch */

   
  if (err != ERR_OK) {
    sock_set_errno(sock, err);
    return -1;
  }

  /* Now do the actual option processing */
  data.sock = sock;
  data.level = level;
  data.optname = optname;
  data.optval = optval;
  data.optlen = optlen;
  data.err = err;
  tcpip_callback(lwip_getsockopt_internal, &data);
  sys_arch_sem_wait(sock->conn->op_completed, 0);
  /* maybe lwip_getsockopt_internal has changed err */
  err = data.err;

  sock_set_errno(sock, err);
  return err ? -1 : 0;
}

static void
lwip_getsockopt_internal(void *arg)
{
  struct lwip_socket *sock;
#ifdef LWIP_DEBUG
  int s;
#endif /* LWIP_DEBUG */
  int level, optname;
  void *optval;
  struct lwip_setgetsockopt_data *data;

  LWIP_ASSERT("arg != NULL", arg != NULL);

  data = (struct lwip_setgetsockopt_data*)arg;
  sock = data->sock;
#ifdef LWIP_DEBUG
  s = data->s;
#endif /* LWIP_DEBUG */
  level = data->level;
  optname = data->optname;
  optval = data->optval;

  switch (level) {
   
/* Level: SOL_SOCKET */
  case SOL_SOCKET:
    switch (optname) {

    /* The option flags */
    case SO_ACCEPTCONN:
    case SO_BROADCAST:
    /* UNIMPL case SO_DEBUG: */
    /* UNIMPL case SO_DONTROUTE: */
    case SO_KEEPALIVE:
    /* UNIMPL case SO_OOBINCLUDE: */
#if SO_REUSE
    case SO_REUSEADDR:
    case SO_REUSEPORT:
#endif /* SO_REUSE */
    /*case SO_USELOOPBACK: UNIMPL */
      *(int*)optval = sock->conn->pcb.ip->so_options & optname;
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, SOL_SOCKET, optname=0x%x, ..) = %s\n",
                                  s, optname, (*(int*)optval?"on":"off")));
      break;

    case SO_TYPE:
      switch (NETCONNTYPE_GROUP(sock->conn->type)) {
      case NETCONN_RAW:
        *(int*)optval = SOCK_RAW;
        break;
      case NETCONN_TCP:
        *(int*)optval = SOCK_STREAM;
        break;
      case NETCONN_UDP:
        *(int*)optval = SOCK_DGRAM;
        break;
      default: /* unrecognized socket type */
        *(int*)optval = sock->conn->type;
        LWIP_DEBUGF(SOCKETS_DEBUG,
                    ("lwip_getsockopt(%d, SOL_SOCKET, SO_TYPE): unrecognized socket type %d\n",
                    s, *(int *)optval));
      }  /* switch (sock->conn->type) */
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, SOL_SOCKET, SO_TYPE) = %d\n",
                  s, *(int *)optval));
      break;

    case SO_ERROR:
      if (sock->err == 0) {
        sock_set_errno(sock, err_to_errno(sock->conn->err));
      } 
      *(int *)optval = sock->err;
      sock->err = 0;
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, SOL_SOCKET, SO_ERROR) = %d\n",
                  s, *(int *)optval));
      break;

#if LWIP_SO_RCVTIMEO
    case SO_RCVTIMEO:
      *(int *)optval = sock->conn->recv_timeout;
      break;
#endif /* LWIP_SO_RCVTIMEO */
#if LWIP_SO_RCVBUF
    case SO_RCVBUF:
      *(int *)optval = sock->conn->recv_bufsize;
      break;
#endif /* LWIP_SO_RCVBUF */
#if LWIP_UDP
    case SO_NO_CHECK:
      *(int*)optval = (udp_flags(sock->conn->pcb.udp) & UDP_FLAGS_NOCHKSUM) ? 1 : 0;
      break;
#endif /* LWIP_UDP*/
    }  /* switch (optname) */
    break;

/* Level: IPPROTO_IP */
  case IPPROTO_IP:
    switch (optname) {
    case IP_TTL:
      *(int*)optval = sock->conn->pcb.ip->ttl;
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_IP, IP_TTL) = %d\n",
                  s, *(int *)optval));
      break;
    case IP_TOS:
      *(int*)optval = sock->conn->pcb.ip->tos;
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_IP, IP_TOS) = %d\n",
                  s, *(int *)optval));
      break;
#if LWIP_IGMP
    case IP_MULTICAST_TTL:
      *(u8_t*)optval = sock->conn->pcb.ip->ttl;
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_IP, IP_MULTICAST_TTL) = %d\n",
                  s, *(int *)optval));
      break;
    case IP_MULTICAST_IF:
      ((struct in_addr*) optval)->s_addr = sock->conn->pcb.udp->multicast_ip.addr;
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_IP, IP_MULTICAST_IF) = 0x%"X32_F"\n",
                  s, *(u32_t *)optval));
      break;
#endif /* LWIP_IGMP */
    }  /* switch (optname) */
    break;

#if LWIP_TCP
/* Level: IPPROTO_TCP */
  case IPPROTO_TCP:
    switch (optname) {
    case TCP_NODELAY:
      *(int*)optval = tcp_nagle_disabled(sock->conn->pcb.tcp);
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_TCP, TCP_NODELAY) = %s\n",
                  s, (*(int*)optval)?"on":"off") );
      break;
    case TCP_KEEPALIVE:
      *(int*)optval = (int)sock->conn->pcb.tcp->keep_idle;
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_IP, TCP_KEEPALIVE) = %d\n",
                  s, *(int *)optval));
      break;

#if LWIP_TCP_KEEPALIVE
    case TCP_KEEPIDLE:
      *(int*)optval = (int)(sock->conn->pcb.tcp->keep_idle/1000);
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_IP, TCP_KEEPIDLE) = %d\n",
                  s, *(int *)optval));
      break;
    case TCP_KEEPINTVL:
      *(int*)optval = (int)(sock->conn->pcb.tcp->keep_intvl/1000);
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_IP, TCP_KEEPINTVL) = %d\n",
                  s, *(int *)optval));
      break;
    case TCP_KEEPCNT:
      *(int*)optval = (int)sock->conn->pcb.tcp->keep_cnt;
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_IP, TCP_KEEPCNT) = %d\n",
                  s, *(int *)optval));
      break;
#endif /* LWIP_TCP_KEEPALIVE */

    }  /* switch (optname) */
    break;
#endif /* LWIP_TCP */
#if LWIP_UDP && LWIP_UDPLITE
  /* Level: IPPROTO_UDPLITE */
  case IPPROTO_UDPLITE:
    switch (optname) {
    case UDPLITE_SEND_CSCOV:
      *(int*)optval = sock->conn->pcb.udp->chksum_len_tx;
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_UDPLITE, UDPLITE_SEND_CSCOV) = %d\n",
                  s, (*(int*)optval)) );
      break;
    case UDPLITE_RECV_CSCOV:
      *(int*)optval = sock->conn->pcb.udp->chksum_len_rx;
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_getsockopt(%d, IPPROTO_UDPLITE, UDPLITE_RECV_CSCOV) = %d\n",
                  s, (*(int*)optval)) );
      break;
    }  /* switch (optname) */
    break;
#endif /* LWIP_UDP */
  } /* switch (level) */
  sys_sem_signal(sock->conn->op_completed);
}

int
lwip_setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen)
{
  struct lwip_socket *sock = get_socket(s);
  int err = ERR_OK;
  struct lwip_setgetsockopt_data data;

  if (!sock)
    return -1;

  if (NULL == optval) {
    sock_set_errno(sock, EFAULT);
    return -1;
  }

  /* Do length and type checks for the various options first, to keep it readable. */
  switch (level) {

/* Level: SOL_SOCKET */
  case SOL_SOCKET:
    switch (optname) {

    case SO_BROADCAST:
    /* UNIMPL case SO_DEBUG: */
    /* UNIMPL case SO_DONTROUTE: */
    case SO_KEEPALIVE:
    /* UNIMPL case case SO_CONTIMEO: */
    /* UNIMPL case case SO_SNDTIMEO: */
#if LWIP_SO_RCVTIMEO
    case SO_RCVTIMEO:
#endif /* LWIP_SO_RCVTIMEO */
#if LWIP_SO_RCVBUF
    case SO_RCVBUF:
#endif /* LWIP_SO_RCVBUF */
    /* UNIMPL case SO_OOBINLINE: */
    /* UNIMPL case SO_SNDBUF: */
    /* UNIMPL case SO_RCVLOWAT: */
    /* UNIMPL case SO_SNDLOWAT: */
#if SO_REUSE
    case SO_REUSEADDR:
    case SO_REUSEPORT:
#endif /* SO_REUSE */
    /* UNIMPL case SO_USELOOPBACK: */
      if (optlen < sizeof(int)) {
        err = EINVAL;
      }
      break;
    case SO_NO_CHECK:
      if (optlen < sizeof(int)) {
        err = EINVAL;
      }
#if LWIP_UDP
      if ((sock->conn->type != NETCONN_UDP) ||
          ((udp_flags(sock->conn->pcb.udp) & UDP_FLAGS_UDPLITE) != 0)) {
        /* this flag is only available for UDP, not for UDP lite */
        err = EAFNOSUPPORT;
      }
#endif /* LWIP_UDP */
      break;
    default:
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, SOL_SOCKET, UNIMPL: optname=0x%x, ..)\n",
                  s, optname));
      err = ENOPROTOOPT;
    }  /* switch (optname) */
    break;

/* Level: IPPROTO_IP */
  case IPPROTO_IP:
    switch (optname) {
    /* UNIMPL case IP_HDRINCL: */
    /* UNIMPL case IP_RCVDSTADDR: */
    /* UNIMPL case IP_RCVIF: */
    case IP_TTL:
    case IP_TOS:
      if (optlen < sizeof(int)) {
        err = EINVAL;
      }
      break;
#if LWIP_IGMP
    case IP_MULTICAST_TTL:
      if (optlen < sizeof(u8_t)) {
        err = EINVAL;
      }
      if (NETCONNTYPE_GROUP(sock->conn->type) != NETCONN_UDP) {
        err = EAFNOSUPPORT;
      }
      break;
    case IP_MULTICAST_IF:
      if (optlen < sizeof(struct in_addr)) {
        err = EINVAL;
      }
      if (NETCONNTYPE_GROUP(sock->conn->type) != NETCONN_UDP) {
        err = EAFNOSUPPORT;
      }
      break;
    case IP_ADD_MEMBERSHIP:
    case IP_DROP_MEMBERSHIP:
      if (optlen < sizeof(struct ip_mreq)) {
        err = EINVAL;
      }
      if (NETCONNTYPE_GROUP(sock->conn->type) != NETCONN_UDP) {
        err = EAFNOSUPPORT;
      }
      break;
#endif /* LWIP_IGMP */
      default:
        LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_IP, UNIMPL: optname=0x%x, ..)\n",
                    s, optname));
        err = ENOPROTOOPT;
    }  /* switch (optname) */
    break;

#if LWIP_TCP
/* Level: IPPROTO_TCP */
  case IPPROTO_TCP:
    if (optlen < sizeof(int)) {
      err = EINVAL;
      break;
    }

    /* If this is no TCP socket, ignore any options. */
    if (sock->conn->type != NETCONN_TCP)
      return 0;

    switch (optname) {
    case TCP_NODELAY:
    case TCP_KEEPALIVE:
#if LWIP_TCP_KEEPALIVE
    case TCP_KEEPIDLE:
    case TCP_KEEPINTVL:
    case TCP_KEEPCNT:
#endif /* LWIP_TCP_KEEPALIVE */
      break;

    default:
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_TCP, UNIMPL: optname=0x%x, ..)\n",
                  s, optname));
      err = ENOPROTOOPT;
    }  /* switch (optname) */
    break;
#endif /* LWIP_TCP */
#if LWIP_UDP && LWIP_UDPLITE
/* Level: IPPROTO_UDPLITE */
  case IPPROTO_UDPLITE:
    if (optlen < sizeof(int)) {
      err = EINVAL;
      break;
    }

    /* If this is no UDP lite socket, ignore any options. */
    if (sock->conn->type != NETCONN_UDPLITE)
      return 0;

    switch (optname) {
    case UDPLITE_SEND_CSCOV:
    case UDPLITE_RECV_CSCOV:
      break;

    default:
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_UDPLITE, UNIMPL: optname=0x%x, ..)\n",
                  s, optname));
      err = ENOPROTOOPT;
    }  /* switch (optname) */
    break;
#endif /* LWIP_UDP && LWIP_UDPLITE */
/* UNDEFINED LEVEL */
  default:
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, level=0x%x, UNIMPL: optname=0x%x, ..)\n",
                s, level, optname));
    err = ENOPROTOOPT;
  }  /* switch (level) */


  if (err != ERR_OK) {
    sock_set_errno(sock, err);
    return -1;
  }


  /* Now do the actual option processing */
  data.sock = sock;
  data.level = level;
  data.optname = optname;
  data.optval = (void*)optval;
  data.optlen = &optlen;
  data.err = err;
  tcpip_callback(lwip_setsockopt_internal, &data);
  sys_arch_sem_wait(sock->conn->op_completed, 0);
  /* maybe lwip_setsockopt_internal has changed err */
  err = data.err;

  sock_set_errno(sock, err);
  return err ? -1 : 0;
}

static void
lwip_setsockopt_internal(void *arg)
{
  struct lwip_socket *sock;
#ifdef LWIP_DEBUG
  int s;
#endif /* LWIP_DEBUG */
  int level, optname;
  const void *optval;
  struct lwip_setgetsockopt_data *data;

  LWIP_ASSERT("arg != NULL", arg != NULL);

  data = (struct lwip_setgetsockopt_data*)arg;
  sock = data->sock;
#ifdef LWIP_DEBUG
  s = data->s;
#endif /* LWIP_DEBUG */
  level = data->level;
  optname = data->optname;
  optval = data->optval;

  switch (level) {

/* Level: SOL_SOCKET */
  case SOL_SOCKET:
    switch (optname) {

    /* The option flags */
    case SO_BROADCAST:
    /* UNIMPL case SO_DEBUG: */
    /* UNIMPL case SO_DONTROUTE: */
    case SO_KEEPALIVE:
    /* UNIMPL case SO_OOBINCLUDE: */
#if SO_REUSE
    case SO_REUSEADDR:
    case SO_REUSEPORT:
#endif /* SO_REUSE */
    /* UNIMPL case SO_USELOOPBACK: */
      if (*(int*)optval) {
        sock->conn->pcb.ip->so_options |= optname;
      } else {
        sock->conn->pcb.ip->so_options &= ~optname;
      }
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, SOL_SOCKET, optname=0x%x, ..) -> %s\n",
                  s, optname, (*(int*)optval?"on":"off")));
      break;
#if LWIP_SO_RCVTIMEO
    case SO_RCVTIMEO:
      sock->conn->recv_timeout = ( *(int*)optval );
      break;
#endif /* LWIP_SO_RCVTIMEO */
#if LWIP_SO_RCVBUF
    case SO_RCVBUF:
      sock->conn->recv_bufsize = ( *(int*)optval );
      break;
#endif /* LWIP_SO_RCVBUF */
#if LWIP_UDP
    case SO_NO_CHECK:
      if (*(int*)optval) {
        udp_setflags(sock->conn->pcb.udp, udp_flags(sock->conn->pcb.udp) | UDP_FLAGS_NOCHKSUM);
      } else {
        udp_setflags(sock->conn->pcb.udp, udp_flags(sock->conn->pcb.udp) & ~UDP_FLAGS_NOCHKSUM);
      }
      break;
#endif /* LWIP_UDP */
    }  /* switch (optname) */
    break;

/* Level: IPPROTO_IP */
  case IPPROTO_IP:
    switch (optname) {
    case IP_TTL:
      sock->conn->pcb.ip->ttl = (u8_t)(*(int*)optval);
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_IP, IP_TTL, ..) -> %d\n",
                  s, sock->conn->pcb.ip->ttl));
      break;
    case IP_TOS:
      sock->conn->pcb.ip->tos = (u8_t)(*(int*)optval);
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_IP, IP_TOS, ..)-> %d\n",
                  s, sock->conn->pcb.ip->tos));
      break;
#if LWIP_IGMP
    case IP_MULTICAST_TTL:
      sock->conn->pcb.udp->ttl = (u8_t)(*(u8_t*)optval);
      break;
    case IP_MULTICAST_IF:
      sock->conn->pcb.udp->multicast_ip.addr = ((struct in_addr*) optval)->s_addr;
      break;
    case IP_ADD_MEMBERSHIP:
    case IP_DROP_MEMBERSHIP:
      {
        /* If this is a TCP or a RAW socket, ignore these options. */
        struct ip_mreq *imr = (struct ip_mreq *)optval;
        if(optname == IP_ADD_MEMBERSHIP){
          data->err = igmp_joingroup((struct ip_addr*)&(imr->imr_interface.s_addr), (struct ip_addr*)&(imr->imr_multiaddr.s_addr));
        } else {
          data->err = igmp_leavegroup((struct ip_addr*)&(imr->imr_interface.s_addr), (struct ip_addr*)&(imr->imr_multiaddr.s_addr));
        }
        if(data->err != ERR_OK) {
          data->err = EADDRNOTAVAIL;
        }
      }
      break;
#endif /* LWIP_IGMP */
    }  /* switch (optname) */
    break;

#if LWIP_TCP
/* Level: IPPROTO_TCP */
  case IPPROTO_TCP:
    switch (optname) {
    case TCP_NODELAY:
      if (*(int*)optval) {
        tcp_nagle_disable(sock->conn->pcb.tcp);
      } else {
        tcp_nagle_enable(sock->conn->pcb.tcp);
      }
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_TCP, TCP_NODELAY) -> %s\n",
                  s, (*(int *)optval)?"on":"off") );
      break;
    case TCP_KEEPALIVE:
      sock->conn->pcb.tcp->keep_idle = (u32_t)(*(int*)optval);
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_TCP, TCP_KEEPALIVE) -> %"U32_F"\n",
                  s, sock->conn->pcb.tcp->keep_idle));
      break;

#if LWIP_TCP_KEEPALIVE
    case TCP_KEEPIDLE:
      sock->conn->pcb.tcp->keep_idle = 1000*(u32_t)(*(int*)optval);
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_TCP, TCP_KEEPIDLE) -> %"U32_F"\n",
                  s, sock->conn->pcb.tcp->keep_idle));
      break;
    case TCP_KEEPINTVL:
      sock->conn->pcb.tcp->keep_intvl = 1000*(u32_t)(*(int*)optval);
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_TCP, TCP_KEEPINTVL) -> %"U32_F"\n",
                  s, sock->conn->pcb.tcp->keep_intvl));
      break;
    case TCP_KEEPCNT:
      sock->conn->pcb.tcp->keep_cnt = (u32_t)(*(int*)optval);
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_TCP, TCP_KEEPCNT) -> %"U32_F"\n",
                  s, sock->conn->pcb.tcp->keep_cnt));
      break;
#endif /* LWIP_TCP_KEEPALIVE */

    }  /* switch (optname) */
    break;
#endif /* LWIP_TCP*/
#if LWIP_UDP && LWIP_UDPLITE
  /* Level: IPPROTO_UDPLITE */
  case IPPROTO_UDPLITE:
    switch (optname) {
    case UDPLITE_SEND_CSCOV:
      if ((*(int*)optval != 0) && (*(int*)optval < 8)) {
        /* don't allow illegal values! */
        sock->conn->pcb.udp->chksum_len_tx = 8;
      } else {
        sock->conn->pcb.udp->chksum_len_tx = *(int*)optval;
      }
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_UDPLITE, UDPLITE_SEND_CSCOV) -> %d\n",
                  s, (*(int*)optval)) );
      break;
    case UDPLITE_RECV_CSCOV:
      if ((*(int*)optval != 0) && (*(int*)optval < 8)) {
        /* don't allow illegal values! */
        sock->conn->pcb.udp->chksum_len_rx = 8;
      } else {
        sock->conn->pcb.udp->chksum_len_rx = *(int*)optval;
      }
      LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_setsockopt(%d, IPPROTO_UDPLITE, UDPLITE_RECV_CSCOV) -> %d\n",
                  s, (*(int*)optval)) );
      break;
    }  /* switch (optname) */
    break;
#endif /* LWIP_UDP */
  }  /* switch (level) */
  sys_sem_signal(sock->conn->op_completed);
}

int
lwip_ioctl(int s, long cmd, void *argp)
{
  struct lwip_socket *sock = get_socket(s);
  u16_t buflen = 0;
  s16_t recv_avail;

  if (!sock)
    return -1;

  switch (cmd) {
  case FIONREAD:
    if (!argp) {
      sock_set_errno(sock, EINVAL);
      return -1;
    }

    SYS_ARCH_GET(sock->conn->recv_avail, recv_avail);
    if (recv_avail < 0)
      recv_avail = 0;
    *((u16_t*)argp) = (u16_t)recv_avail;

    /* Check if there is data left from the last recv operation. /maq 041215 */
    if (sock->lastdata) {
      buflen = netbuf_len(sock->lastdata);
      buflen -= sock->lastoffset;

      *((u16_t*)argp) += buflen;
    }

    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_ioctl(%d, FIONREAD, %p) = %"U16_F"\n", s, argp, *((u16_t*)argp)));
    sock_set_errno(sock, 0);
    return 0;

  case FIONBIO:
    if (argp && *(u32_t*)argp)
      sock->flags |= O_NONBLOCK;
    else
      sock->flags &= ~O_NONBLOCK;
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_ioctl(%d, FIONBIO, %d)\n", s, !!(sock->flags & O_NONBLOCK)));
    sock_set_errno(sock, 0);
    return 0;

  default:
    LWIP_DEBUGF(SOCKETS_DEBUG, ("lwip_ioctl(%d, UNIMPL: 0x%lx, %p)\n", s, cmd, argp));
    sock_set_errno(sock, ENOSYS); /* not yet implemented */
    return -1;
  } /* switch (cmd) */
}

#endif /* LWIP_SOCKET */
