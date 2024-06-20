/**
 * @file
 * Sockets stresstest
 *
 * This file uses the lwIP socket API to do stress tests that should test the
 * stability when used in many different situations, with many concurrent
 * sockets making concurrent transfers in different manners.
 *
 * - test rely on loopback sockets for now, so netif drivers are not tested
 * - all enabled functions shall be used
 * - parallelism of the tests depend on enough resources being available
 *   (configure your lwipopts.h settings high enough)
 * - test should also be able to run in a real target
 *
 * TODO:
 * - full duplex
 * - add asserts about internal socket/netconn/pcb state?
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
#include "sockets_stresstest.h"

#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "lwip/mem.h"

#include <stdio.h>
#include <string.h>

#if LWIP_SOCKET && LWIP_IPV4 /* this uses IPv4 loopback sockets, currently */

#ifndef TEST_SOCKETS_STRESS
#define TEST_SOCKETS_STRESS   LWIP_DBG_OFF
#endif

#define TEST_TIME_SECONDS     10
#define TEST_TXRX_BUFSIZE     (TCP_MSS * 2)
#define TEST_MAX_RXWAIT_MS    50
#define TEST_MAX_CONNECTIONS  50

#define TEST_SOCK_READABLE    0x01
#define TEST_SOCK_WRITABLE    0x02
#define TEST_SOCK_ERR         0x04

#define TEST_MODE_SELECT      0x01
#define TEST_MODE_POLL        0x02
#define TEST_MODE_NONBLOCKING 0x04
#define TEST_MODE_WAIT        0x08
#define TEST_MODE_RECVTIMEO   0x10
#define TEST_MODE_SLEEP       0x20

static int sockets_stresstest_numthreads;

struct test_settings {
  struct sockaddr_storage addr;
  int start_client;
  int loop_cnt;
};

struct sockets_stresstest_fullduplex {
  int s;
  volatile int closed;
};

static void
fill_test_data(void *buf, size_t buf_len_bytes)
{
  u8_t *p = (u8_t*)buf;
  u16_t i, chk;

  LWIP_ASSERT("buffer too short", buf_len_bytes >= 4);
  LWIP_ASSERT("buffer too big", buf_len_bytes <= 0xFFFF);
  /* store the total number of bytes */
  p[0] = (u8_t)(buf_len_bytes >> 8);
  p[1] = (u8_t)buf_len_bytes;

  /* fill buffer with random */
  chk = 0;
  for (i = 4; i < buf_len_bytes; i++) {
    u8_t rnd = (u8_t)LWIP_RAND();
    p[i] = rnd;
    chk += rnd;
  }
  /* store checksum */
  p[2] = (u8_t)(chk >> 8);
  p[3] = (u8_t)chk;
}

static size_t
check_test_data(const void *buf, size_t buf_len_bytes)
{
  u8_t *p = (u8_t*)buf;
  u16_t i, chk, chk_rx, len_rx;

  LWIP_ASSERT("buffer too short", buf_len_bytes >= 4);
  len_rx = (((u16_t)p[0]) << 8) | p[1];
  LWIP_ASSERT("len too short", len_rx >= 4);
  if (len_rx > buf_len_bytes) {
    /* not all data received in this segment */
    LWIP_DEBUGF(TEST_SOCKETS_STRESS | LWIP_DBG_TRACE, ("check-\n"));
    return buf_len_bytes;
  }
  chk_rx = (((u16_t)p[2]) << 8) | p[3];
  /* calculate received checksum */
  chk = 0;
  for (i = 4; i < len_rx; i++) {
    chk += p[i];
  }
  LWIP_ASSERT("invalid checksum", chk == chk_rx);
  if (len_rx < buf_len_bytes) {
    size_t data_left = buf_len_bytes - len_rx;
    memmove(p, &p[len_rx], data_left);
    return data_left;
  }
  /* if we come here, we received exactly one chunk
     -> next offset is 0 */
  return 0;
}

static size_t
recv_and_check_data_return_offset(int s, char *rxbuf, size_t rxbufsize, size_t rxoff, int *closed, const char *dbg)
{
  ssize_t ret;

  ret = lwip_read(s, &rxbuf[rxoff], rxbufsize - rxoff);
  if (ret == 0) {
    *closed = 1;
    return rxoff;
  }
  *closed = 0;
  LWIP_DEBUGF(TEST_SOCKETS_STRESS | LWIP_DBG_TRACE, ("%s %d rx %d\n", dbg, s, (int)ret));
  if (ret == -1) {
    /* TODO: for this to work, 'errno' has to support multithreading... */
    int err = errno;
    if (err == ENOTCONN) {
      *closed = 1;
      return 0;
    }
    LWIP_ASSERT("err == 0", err == 0);
  }
  LWIP_ASSERT("ret > 0", ret > 0);
  return check_test_data(rxbuf, rxoff + ret);
}

#if LWIP_SOCKET_SELECT
static int
sockets_stresstest_wait_readable_select(int s, int timeout_ms)
{
  int ret;
  struct timeval tv;
  fd_set fs_r;
  fd_set fs_w;
  fd_set fs_e;

  FD_ZERO(&fs_r);
  FD_ZERO(&fs_w);
  FD_ZERO(&fs_e);

  FD_SET(s, &fs_r);
  FD_SET(s, &fs_e);

  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec = (timeout_ms - (tv.tv_sec * 1000)) * 1000;
  ret = lwip_select(s + 1, &fs_r, &fs_w, &fs_e, &tv);
  LWIP_ASSERT("select error", ret >= 0);
  if (ret) {
    /* convert poll flags to our flags */
    ret = 0;
    if (FD_ISSET(s, &fs_r)) {
      ret |= TEST_SOCK_READABLE;
    }
    if (FD_ISSET(s, &fs_w)) {
      ret |= TEST_SOCK_WRITABLE;
    }
    if (FD_ISSET(s, &fs_e)) {
      ret |= TEST_SOCK_ERR;
    }
    return ret;
  }
  return 0;
}
#endif

#if LWIP_SOCKET_POLL
static int
sockets_stresstest_wait_readable_poll(int s, int timeout_ms)
{
  int ret;
  struct pollfd pfd;

  pfd.fd = s;
  pfd.revents = 0;
  pfd.events = POLLIN | POLLERR;

  ret = lwip_poll(&pfd, 1, timeout_ms);
  if (ret) {
    /* convert poll flags to our flags */
    ret = 0;
    if (pfd.revents & POLLIN) {
      ret |= TEST_SOCK_READABLE;
    }
    if (pfd.revents & POLLOUT) {
      ret |= TEST_SOCK_WRITABLE;
    }
    if (pfd.revents & POLLERR) {
      ret |= TEST_SOCK_ERR;
    }
    return ret;
  }
  return 0;
}
#endif

#if LWIP_SO_RCVTIMEO
static int
sockets_stresstest_wait_readable_recvtimeo(int s, int timeout_ms)
{
  int ret;
  char buf;
#if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
  int opt_on = timeout_ms;
  int opt_off = 0;
#else
  struct timeval opt_on, opt_off;
  opt_on.tv_sec = timeout_ms / 1000;
  opt_on.tv_usec = (timeout_ms - (opt_on.tv_sec * 1000)) * 1000;
  opt_off.tv_sec = 0;
  opt_off.tv_usec = 0;
#endif

  /* enable receive timeout */
  ret = lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &opt_on, sizeof(opt_on));
  LWIP_ASSERT("setsockopt error", ret == 0);

  /* peek for one byte with timeout */
  ret = lwip_recv(s, &buf, 1, MSG_PEEK);

  /* disable receive timeout */
  ret = lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &opt_off, sizeof(opt_off));
  LWIP_ASSERT("setsockopt error", ret == 0);

  if (ret == 1) {
    return TEST_SOCK_READABLE;
  }
  if (ret == 0) {
    return 0;
  }
  if (ret == -1) {
    return TEST_SOCK_ERR;
  }
  LWIP_ASSERT("invalid return value", 0);
  return TEST_SOCK_ERR;
}
#endif

static int
sockets_stresstest_wait_readable_wait_peek(int s, int timeout_ms)
{
  int ret;
  char buf;

  LWIP_UNUSED_ARG(timeout_ms); /* cannot time out here */

  /* peek for one byte */
  ret = lwip_recv(s, &buf, 1, MSG_PEEK);

  if (ret == 1) {
    return TEST_SOCK_READABLE;
  }
  if (ret == 0) {
    return 0;
  }
  if (ret == -1) {
    return TEST_SOCK_ERR;
  }
  LWIP_ASSERT("invalid return value", 0);
  return TEST_SOCK_ERR;
}

static int
sockets_stresstest_wait_readable_nonblock(int s, int timeout_ms)
{
  int ret;
  char buf;
  u32_t wait_until = sys_now() + timeout_ms;

  while(sys_now() < wait_until) {
    /* peek for one byte */
    ret = lwip_recv(s, &buf, 1, MSG_PEEK | MSG_DONTWAIT);

    if (ret == 1) {
      return TEST_SOCK_READABLE;
    }
    if (ret == -1) {
      /* TODO: for this to work, 'errno' has to support multithreading... */
      int err = errno;
      if (err != EWOULDBLOCK) {
        return TEST_SOCK_ERR;
      }
    }
    /* TODO: sleep? */
  }
  return 0;
}

static int sockets_stresstest_rand_mode(int allow_wait, int allow_rx)
{
  u32_t random_value = LWIP_RAND();
#if LWIP_SOCKET_SELECT
  if (random_value & TEST_MODE_SELECT) {
    return TEST_MODE_SELECT;
  }
#endif
#if LWIP_SOCKET_POLL
  if (random_value & TEST_MODE_POLL) {
    return TEST_MODE_POLL;
  }
#endif
  if (!allow_rx) {
    return TEST_MODE_SLEEP;
  }
#if LWIP_SO_RCVTIMEO
  if (random_value & TEST_MODE_RECVTIMEO) {
    return TEST_MODE_RECVTIMEO;
  }
#endif
  if (allow_wait) {
    if (random_value & TEST_MODE_RECVTIMEO) {
      return TEST_MODE_RECVTIMEO;
    }
  }
  return TEST_MODE_NONBLOCKING;
}

static int
sockets_stresstest_wait_readable(int mode, int s, int timeout_ms)
{
  switch(mode)
  {
#if LWIP_SOCKET_SELECT
  case TEST_MODE_SELECT:
    return sockets_stresstest_wait_readable_select(s, timeout_ms);
#endif
#if LWIP_SOCKET_POLL
  case TEST_MODE_POLL:
    return sockets_stresstest_wait_readable_poll(s, timeout_ms);
#endif
#if LWIP_SO_RCVTIMEO
  case TEST_MODE_RECVTIMEO:
    return sockets_stresstest_wait_readable_recvtimeo(s, timeout_ms);
#endif
  case TEST_MODE_WAIT:
    return sockets_stresstest_wait_readable_wait_peek(s, timeout_ms);
  case TEST_MODE_NONBLOCKING:
    return sockets_stresstest_wait_readable_nonblock(s, timeout_ms);
  case TEST_MODE_SLEEP:
    {
      sys_msleep(timeout_ms);
      return 1;
    }
  default:
    LWIP_ASSERT("invalid mode", 0);
    break;
  }
  return 0;
}

#if LWIP_NETCONN_FULLDUPLEX
static void
sockets_stresstest_conn_client_r(void *arg)
{
  struct sockets_stresstest_fullduplex *fd = (struct sockets_stresstest_fullduplex *)arg;
  int s = fd->s;
  size_t rxoff = 0;
  char rxbuf[TEST_TXRX_BUFSIZE];

  while (1) {
    int closed;
    if (fd->closed) {
      break;
    }
    rxoff = recv_and_check_data_return_offset(s, rxbuf, sizeof(rxbuf), rxoff, &closed, "cli");
    if (fd->closed) {
      break;
    }
    if (closed) {
      lwip_close(s);
      break;
    }
  }

  SYS_ARCH_DEC(sockets_stresstest_numthreads, 1);
  LWIP_ASSERT("", sockets_stresstest_numthreads >= 0);
}
#endif

static void
sockets_stresstest_conn_client(void *arg)
{
  struct sockaddr_storage addr;
  struct sockaddr_in *addr_in;
  int s, ret;
  char txbuf[TEST_TXRX_BUFSIZE];
  char rxbuf[TEST_TXRX_BUFSIZE];
  size_t rxoff = 0;
  u32_t max_time = sys_now() + (TEST_TIME_SECONDS * 1000);
  int do_rx = 1;
  struct sockets_stresstest_fullduplex *data = NULL;

  memcpy(&addr, arg, sizeof(addr));
  LWIP_ASSERT("", addr.ss_family == AF_INET);
  addr_in = (struct sockaddr_in *)&addr;
  addr_in->sin_addr.s_addr = inet_addr("127.0.0.1");

  /* sleep a random time between 1 and 2 seconds */
  sys_msleep(1000 + (LWIP_RAND() % 1000));

  /* connect to the server */
  s = lwip_socket(addr.ss_family, SOCK_STREAM, 0);
  LWIP_ASSERT("s >= 0", s >= 0);

#if LWIP_NETCONN_FULLDUPLEX
  if (LWIP_RAND() & 1) {
    sys_thread_t t;
    data = (struct sockets_stresstest_fullduplex*)mem_malloc(sizeof(struct sockets_stresstest_fullduplex));
    LWIP_ASSERT("data != NULL", data != 0);
    SYS_ARCH_INC(sockets_stresstest_numthreads, 1);
    data->s = s;
    data->closed = 0;
    t = sys_thread_new("sockets_stresstest_conn_client_r", sockets_stresstest_conn_client_r, data, 0, 0);
    LWIP_ASSERT("thread != NULL", t != 0);
    do_rx = 0;
  }
#endif

  /* @todo: nonblocking connect? */
  ret = lwip_connect(s, (struct sockaddr *)&addr, sizeof(struct sockaddr_storage));
  LWIP_ASSERT("ret == 0", ret == 0);

  while (sys_now() < max_time) {
    int closed;
    int mode = sockets_stresstest_rand_mode(0, do_rx);
    int timeout_ms = LWIP_RAND() % TEST_MAX_RXWAIT_MS;
    ret = sockets_stresstest_wait_readable(mode, s, timeout_ms);
    if (ret) {
      if (do_rx) {
        /* read some */
        LWIP_ASSERT("readable", ret == TEST_SOCK_READABLE);
        rxoff = recv_and_check_data_return_offset(s, rxbuf, sizeof(rxbuf), rxoff, &closed, "cli");
        LWIP_ASSERT("client got closed", !closed);
      }
    } else {
      /* timeout, send some */
      size_t send_len = (LWIP_RAND() % (sizeof(txbuf) - 4)) + 4;
      fill_test_data(txbuf, send_len);
      LWIP_DEBUGF(TEST_SOCKETS_STRESS | LWIP_DBG_TRACE, ("cli %d tx %d\n", s, (int)send_len));
      ret = lwip_write(s, txbuf, send_len);
      if (ret == -1) {
        /* TODO: for this to work, 'errno' has to support multithreading... */
        int err = errno;
        LWIP_ASSERT("err == 0", err == 0);
      }
      LWIP_ASSERT("ret == send_len", ret == (int)send_len);
    }
  }
  if (data) {
    data->closed = 1;
  }
  ret = lwip_close(s);
  LWIP_ASSERT("ret == 0", ret == 0);

  SYS_ARCH_DEC(sockets_stresstest_numthreads, 1);
  LWIP_ASSERT("", sockets_stresstest_numthreads >= 0);
}

static void
sockets_stresstest_conn_server(void *arg)
{
  int s, ret;
  char txbuf[TEST_TXRX_BUFSIZE];
  char rxbuf[TEST_TXRX_BUFSIZE];
  size_t rxoff = 0;

  s = (int)arg;

  while (1) {
    int closed;
    int mode = sockets_stresstest_rand_mode(1, 1);
    int timeout_ms = LWIP_RAND() % TEST_MAX_RXWAIT_MS;
    ret = sockets_stresstest_wait_readable(mode, s, timeout_ms);
    if (ret) {
      if (ret & TEST_SOCK_ERR) {
        /* closed? */
        break;
      }
      /* read some */
      LWIP_ASSERT("readable", ret == TEST_SOCK_READABLE);
      rxoff = recv_and_check_data_return_offset(s, rxbuf, sizeof(rxbuf), rxoff, &closed, "srv");
      if (closed) {
        break;
      }
    } else {
      /* timeout, send some */
      size_t send_len = (LWIP_RAND() % (sizeof(txbuf) - 4)) + 4;
      fill_test_data(txbuf, send_len);
      LWIP_DEBUGF(TEST_SOCKETS_STRESS | LWIP_DBG_TRACE, ("srv %d tx %d\n", s, (int)send_len));
      ret = lwip_write(s, txbuf, send_len);
      if (ret == -1) {
        /* TODO: for this to work, 'errno' has to support multithreading... */
        int err = errno;
        if (err == ECONNRESET) {
          break;
        }
        if (err == ENOTCONN) {
          break;
        }
        LWIP_ASSERT("unknown error", 0);
      }
      LWIP_ASSERT("ret == send_len", ret == (int)send_len);
    }
  }
  ret = lwip_close(s);
  LWIP_ASSERT("ret == 0", ret == 0);

  SYS_ARCH_DEC(sockets_stresstest_numthreads, 1);
  LWIP_ASSERT("", sockets_stresstest_numthreads >= 0);
}

static int
sockets_stresstest_start_clients(const struct sockaddr_storage *remote_addr)
{
  /* limit the number of connections */
  const int max_connections = LWIP_MIN(TEST_MAX_CONNECTIONS, MEMP_NUM_TCP_PCB/3);
  int i;

  for (i = 0; i < max_connections; i++) {
    sys_thread_t t;
    SYS_ARCH_INC(sockets_stresstest_numthreads, 1);
    t = sys_thread_new("sockets_stresstest_conn_client", sockets_stresstest_conn_client, (void*)remote_addr, 0, 0);
    LWIP_ASSERT("thread != NULL", t != 0);
  }
  return max_connections;
}

static void
sockets_stresstest_listener(void *arg)
{
  int slisten;
  int ret;
  struct sockaddr_storage addr;
  socklen_t addr_len;
  struct test_settings *settings = (struct test_settings *)arg;
  int num_clients, num_servers = 0;

  slisten = lwip_socket(AF_INET, SOCK_STREAM, 0);
  LWIP_ASSERT("slisten >= 0", slisten >= 0);

  memcpy(&addr, &settings->addr, sizeof(struct sockaddr_storage));
  ret = lwip_bind(slisten, (struct sockaddr *)&addr, sizeof(addr));
  LWIP_ASSERT("ret == 0", ret == 0);

  ret = lwip_listen(slisten, 0);
  LWIP_ASSERT("ret == 0", ret == 0);

  addr_len = sizeof(addr);
  ret = lwip_getsockname(slisten, (struct sockaddr *)&addr, &addr_len);
  LWIP_ASSERT("ret == 0", ret == 0);

  num_clients = sockets_stresstest_start_clients(&addr);

  while (num_servers < num_clients) {
    struct sockaddr_storage aclient;
    socklen_t aclient_len = sizeof(aclient);
    int sclient = lwip_accept(slisten, (struct sockaddr *)&aclient, &aclient_len);
#if 1
    /* using server threads */
    {
      sys_thread_t t;
      SYS_ARCH_INC(sockets_stresstest_numthreads, 1);
      num_servers++;
      t = sys_thread_new("sockets_stresstest_conn_server", sockets_stresstest_conn_server, (void*)sclient, 0, 0);
      LWIP_ASSERT("thread != NULL", t != 0);
    }
#else
    /* using server select */
#endif
  }
  LWIP_DEBUGF(TEST_SOCKETS_STRESS | LWIP_DBG_STATE, ("sockets_stresstest_listener: all %d connections established\n", num_clients));

  /* accepted all clients */
  while (sockets_stresstest_numthreads > 0) {
    sys_msleep(1);
  }

  ret = lwip_close(slisten);
  LWIP_ASSERT("ret == 0", ret == 0);

  LWIP_DEBUGF(TEST_SOCKETS_STRESS |LWIP_DBG_STATE, ("sockets_stresstest_listener: done\n"));
}

static void
sockets_stresstest_listener_loop(void *arg)
{
  int i;
  struct test_settings *settings = (struct test_settings *)arg;

  if (settings->loop_cnt) {
    for (i = 0; i < settings->loop_cnt; i++) {
      LWIP_DEBUGF(TEST_SOCKETS_STRESS |LWIP_DBG_STATE, ("sockets_stresstest_listener_loop: iteration %d\n", i));
      sockets_stresstest_listener(arg);
      sys_msleep(2);
    }
    LWIP_DEBUGF(TEST_SOCKETS_STRESS |LWIP_DBG_STATE, ("sockets_stresstest_listener_loop: done\n"));
  } else {
    for (i = 0; ; i++) {
      LWIP_DEBUGF(TEST_SOCKETS_STRESS |LWIP_DBG_STATE, ("sockets_stresstest_listener_loop: iteration %d\n", i));
      sockets_stresstest_listener(arg);
      sys_msleep(2);
    }
  }
}

void
sockets_stresstest_init_loopback(int addr_family)
{
  sys_thread_t t;
  struct test_settings *settings = (struct test_settings *)mem_malloc(sizeof(struct test_settings));

  LWIP_ASSERT("OOM", settings != NULL);
  memset(settings, 0, sizeof(struct test_settings));
#if LWIP_IPV4 && LWIP_IPV6
  LWIP_ASSERT("invalid addr_family", (addr_family == AF_INET) || (addr_family == AF_INET6));
#endif
  settings->addr.ss_family = (sa_family_t)addr_family;
  LWIP_UNUSED_ARG(addr_family);
  settings->start_client = 1;

  t = sys_thread_new("sockets_stresstest_listener_loop", sockets_stresstest_listener_loop, settings, 0, 0);
  LWIP_ASSERT("thread != NULL", t != 0);
}

void
sockets_stresstest_init_server(int addr_family, u16_t server_port)
{
  sys_thread_t t;
  struct test_settings *settings = (struct test_settings *)mem_malloc(sizeof(struct test_settings));

  LWIP_ASSERT("OOM", settings != NULL);
  memset(settings, 0, sizeof(struct test_settings));
#if LWIP_IPV4 && LWIP_IPV6
  LWIP_ASSERT("invalid addr_family", (addr_family == AF_INET) || (addr_family == AF_INET6));
  settings->addr.ss_family = (sa_family_t)addr_family;
#endif
  LWIP_UNUSED_ARG(addr_family);
  ((struct sockaddr_in *)(&settings->addr))->sin_port = server_port;

  t = sys_thread_new("sockets_stresstest_listener", sockets_stresstest_listener, settings, 0, 0);
  LWIP_ASSERT("thread != NULL", t != 0);
}

void
sockets_stresstest_init_client(const char *remote_ip, u16_t remote_port)
{
#if LWIP_IPV4
  ip4_addr_t ip4;
#endif
#if LWIP_IPV6
  ip6_addr_t ip6;
#endif
  struct sockaddr_storage *addr = (struct sockaddr_storage *)mem_malloc(sizeof(struct sockaddr_storage));

  LWIP_ASSERT("OOM", addr != NULL);
  memset(addr, 0, sizeof(struct test_settings));
#if LWIP_IPV4
  if (ip4addr_aton(remote_ip, &ip4)) {
    addr->ss_family = AF_INET;
    ((struct sockaddr_in *)addr)->sin_addr.s_addr = ip4_addr_get_u32(&ip4);
  }
#endif
#if LWIP_IPV4 && LWIP_IPV6
  else
#endif
#if LWIP_IPV6
  if (ip6addr_aton(remote_ip, &ip6)) {
    addr->ss_family = AF_INET6;
    /* todo: copy ipv6 address */
  }
#endif
  ((struct sockaddr_in *)addr)->sin_port = remote_port;
  sockets_stresstest_start_clients(addr);
}

#endif /* LWIP_SOCKET && LWIP_IPV4 */
