/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
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

#include "shell.h"

#include "lwip/opt.h"

#if LWIP_NETCONN && LWIP_TCP

#include <string.h>
#include <stdio.h>

#include "lwip/mem.h"
#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/api.h"
#include "lwip/stats.h"

#if LWIP_SOCKET
#include "lwip/errno.h"
#include "lwip/if_api.h"
#endif

#ifdef WIN32
#define NEWLINE "\r\n"
#else /* WIN32 */
#define NEWLINE "\n"
#endif /* WIN32 */

/** Define this to 1 if you want to echo back all received characters
 * (e.g. so they are displayed on a remote telnet)
 */
#ifndef SHELL_ECHO
#define SHELL_ECHO 0
#endif

#define BUFSIZE             1024
static unsigned char buffer[BUFSIZE];

struct command {
  struct netconn *conn;
  s8_t (* exec)(struct command *);
  u8_t nargs;
  char *args[10];
};

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define ESUCCESS 0
#define ESYNTAX -1
#define ETOOFEW -2
#define ETOOMANY -3
#define ECLOSED -4

#define NCONNS 10
static struct netconn *conns[NCONNS];

/* help_msg is split into 3 strings to prevent exceeding the C89 maximum length of 509 per string */
static char help_msg1[] = "Available commands:"NEWLINE"\
open [IP address] [TCP port]: opens a TCP connection to the specified address."NEWLINE"\
lstn [TCP port]: sets up a server on the specified port."NEWLINE"\
acpt [connection #]: waits for an incoming connection request."NEWLINE"\
send [connection #] [message]: sends a message on a TCP connection."NEWLINE"\
udpc [local UDP port] [IP address] [remote port]: opens a UDP \"connection\"."NEWLINE"\
udpl [local UDP port] [IP address] [remote port]: opens a UDP-Lite \"connection\"."NEWLINE"";
static char help_msg2[] = "udpn [local UDP port] [IP address] [remote port]: opens a UDP \"connection\" without checksums."NEWLINE"\
udpb [local port] [remote port]: opens a UDP broadcast \"connection\"."NEWLINE"\
usnd [connection #] [message]: sends a message on a UDP connection."NEWLINE"\
recv [connection #]: receives data on a TCP or UDP connection."NEWLINE"\
clos [connection #]: closes a TCP or UDP connection."NEWLINE"\
stat: prints out lwIP statistics."NEWLINE"\
idxtoname [index]: outputs interface name from index."NEWLINE"\
nametoidx [name]: outputs interface index from name."NEWLINE;
static char help_msg3[] =
"gethostnm [name]: outputs IP address of host."NEWLINE"\
quit: quits"NEWLINE"";

#if LWIP_STATS
static char padding_10spaces[] = "          ";

#define PROTOCOL_STATS (LINK_STATS && ETHARP_STATS && IPFRAG_STATS && IP_STATS && ICMP_STATS && UDP_STATS && TCP_STATS)

#if PROTOCOL_STATS
static const char* shell_stat_proto_names[] = {
#if LINK_STATS
  "LINK      ",
#endif
#if ETHARP_STATS
  "ETHARP    ",
#endif
#if IPFRAG_STATS
  "IP_FRAG   ",
#endif
#if IP_STATS
  "IP        ",
#endif
#if ICMP_STATS
  "ICMP      ",
#endif
#if UDP_STATS
  "UDP       ",
#endif
#if TCP_STATS
  "TCP       ",
#endif
  "last"
};

static struct stats_proto* shell_stat_proto_stats[] = {
#if LINK_STATS
  &lwip_stats.link,
#endif
#if ETHARP_STATS
  &lwip_stats.etharp,
#endif
#if IPFRAG_STATS
  &lwip_stats.ip_frag,
#endif
#if IP_STATS
  &lwip_stats.ip,
#endif
#if ICMP_STATS
  &lwip_stats.icmp,
#endif
#if UDP_STATS
  &lwip_stats.udp,
#endif
#if TCP_STATS
  &lwip_stats.tcp,
#endif
};
static const size_t num_protostats = sizeof(shell_stat_proto_stats)/sizeof(struct stats_proto*);

static const char *stat_msgs_proto[] = {
  " * transmitted ",
  "           * received ",
  "             forwarded ",
  "           * dropped ",
  "           * checksum errors ",
  "           * length errors ",
  "           * memory errors ",
  "             routing errors ",
  "             protocol errors ",
  "             option errors ",
  "           * misc errors ",
  "             cache hits "
};
#endif /* PROTOCOL_STATS */
#endif /* LWIP_STATS */

/*-----------------------------------------------------------------------------------*/
static void
sendstr(const char *str, struct netconn *conn)
{
  netconn_write(conn, (const void *)str, strlen(str), NETCONN_NOCOPY);
}
/*-----------------------------------------------------------------------------------*/
static s8_t
com_open(struct command *com)
{
  ip_addr_t ipaddr;
  u16_t port;
  int i;
  err_t err;
  long tmp;

  if (ipaddr_aton(com->args[0], &ipaddr) == -1) {
    sendstr(strerror(errno), com->conn);
    return ESYNTAX;
  }
  tmp = strtol(com->args[1], NULL, 10);
  if((tmp < 0) || (tmp > 0xffff)) {
    sendstr("Invalid port number."NEWLINE, com->conn);
    return ESUCCESS;
  }
  port = (u16_t)tmp;

  /* Find the first unused connection in conns. */
  for(i = 0; i < NCONNS && conns[i] != NULL; i++);

  if (i == NCONNS) {
    sendstr("No more connections available, sorry."NEWLINE, com->conn);
    return ESUCCESS;
  }

  sendstr("Opening connection to ", com->conn);
  netconn_write(com->conn, com->args[0], strlen(com->args[0]), NETCONN_COPY);
  sendstr(":", com->conn);
  netconn_write(com->conn, com->args[1], strlen(com->args[1]), NETCONN_COPY);
  sendstr(NEWLINE, com->conn);

  conns[i] = netconn_new(NETCONN_TCP);
  if (conns[i] == NULL) {
    sendstr("Could not create connection identifier (out of memory)."NEWLINE, com->conn);
    return ESUCCESS;
  }
  err = netconn_connect(conns[i], &ipaddr, port);
  if (err != ERR_OK) {
    fprintf(stderr, "error %s"NEWLINE, lwip_strerr(err));
    sendstr("Could not connect to remote host: ", com->conn);
#ifdef LWIP_DEBUG
    sendstr(lwip_strerr(err), com->conn);
#else
    sendstr("(debugging must be turned on for error message to appear)", com->conn);
#endif /* LWIP_DEBUG */
    sendstr(NEWLINE, com->conn);
    netconn_delete(conns[i]);
    conns[i] = NULL;
    return ESUCCESS;
  }

  sendstr("Opened connection, connection identifier is ", com->conn);
  snprintf((char *)buffer, sizeof(buffer), "%d"NEWLINE, i);
  netconn_write(com->conn, buffer, strlen((const char *)buffer), NETCONN_COPY);

  return ESUCCESS;
}
/*-----------------------------------------------------------------------------------*/
static s8_t
com_lstn(struct command *com)
{
  u16_t port;
  int i;
  err_t err;
  long tmp;

  tmp = strtol(com->args[0], NULL, 10);
  if((tmp < 0) || (tmp > 0xffff)) {
    sendstr("Invalid port number."NEWLINE, com->conn);
    return ESUCCESS;
  }
  port = (u16_t)tmp;

  /* Find the first unused connection in conns. */
  for(i = 0; i < NCONNS && conns[i] != NULL; i++);

  if (i == NCONNS) {
    sendstr("No more connections available, sorry."NEWLINE, com->conn);
    return ESUCCESS;
  }

  sendstr("Opening a listening connection on port ", com->conn);
  netconn_write(com->conn, com->args[0], strlen(com->args[0]), NETCONN_COPY);
  sendstr(NEWLINE, com->conn);

  conns[i] = netconn_new(NETCONN_TCP);
  if (conns[i] == NULL) {
    sendstr("Could not create connection identifier (out of memory)."NEWLINE, com->conn);
    return ESUCCESS;
  }

  err = netconn_bind(conns[i], IP_ADDR_ANY, port);
  if (err != ERR_OK) {
    netconn_delete(conns[i]);
    conns[i] = NULL;
    sendstr("Could not bind: ", com->conn);
#ifdef LWIP_DEBUG
    sendstr(lwip_strerr(err), com->conn);
#else
    sendstr("(debugging must be turned on for error message to appear)", com->conn);
#endif /* LWIP_DEBUG */
    sendstr(NEWLINE, com->conn);
    return ESUCCESS;
  }

  err = netconn_listen(conns[i]);
  if (err != ERR_OK) {
    netconn_delete(conns[i]);
    conns[i] = NULL;
    sendstr("Could not listen: ", com->conn);
#ifdef LWIP_DEBUG
    sendstr(lwip_strerr(err), com->conn);
#else
    sendstr("(debugging must be turned on for error message to appear)", com->conn);
#endif /* LWIP_DEBUG */
    sendstr(NEWLINE, com->conn);
    return ESUCCESS;
  }

  sendstr("Opened connection, connection identifier is ", com->conn);
  snprintf((char *)buffer, sizeof(buffer), "%d"NEWLINE, i);
  netconn_write(com->conn, buffer, strlen((const char *)buffer), NETCONN_COPY);

  return ESUCCESS;
}
/*-----------------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------------*/
static s8_t
com_clos(struct command *com)
{
  int i;
  err_t err;

  i = strtol(com->args[0], NULL, 10);

  if (i > NCONNS) {
    sendstr("Connection identifier too high."NEWLINE, com->conn);
    return ESUCCESS;
  }
  if (conns[i] == NULL) {
    sendstr("Connection identifier not in use."NEWLINE, com->conn);
    return ESUCCESS;
  }

  err = netconn_close(conns[i]);
  if (err != ERR_OK) {
    sendstr("Could not close connection: ", com->conn);
#ifdef LWIP_DEBUG
    sendstr(lwip_strerr(err), com->conn);
#else
    sendstr("(debugging must be turned on for error message to appear)", com->conn);
#endif /* LWIP_DEBUG */
    sendstr(NEWLINE, com->conn);
    return ESUCCESS;
  }

  sendstr("Connection closed."NEWLINE, com->conn);
  netconn_delete(conns[i]);
  conns[i] = NULL;
  return ESUCCESS;
}
/*-----------------------------------------------------------------------------------*/
static s8_t
com_acpt(struct command *com)
{
  int i, j;
  err_t err;

  /* Find the first unused connection in conns. */
  for(j = 0; j < NCONNS && conns[j] != NULL; j++);

  if (j == NCONNS) {
    sendstr("No more connections available, sorry."NEWLINE, com->conn);
    return ESUCCESS;
  }

  i = strtol(com->args[0], NULL, 10);

  if (i > NCONNS) {
    sendstr("Connection identifier too high."NEWLINE, com->conn);
    return ESUCCESS;
  }
  if (conns[i] == NULL) {
    sendstr("Connection identifier not in use."NEWLINE, com->conn);
    return ESUCCESS;
  }

  err = netconn_accept(conns[i], &conns[j]);

  if (err != ERR_OK) {
    sendstr("Could not accept connection: ", com->conn);
#ifdef LWIP_DEBUG
    sendstr(lwip_strerr(err), com->conn);
#else
    sendstr("(debugging must be turned on for error message to appear)", com->conn);
#endif /* LWIP_DEBUG */
    sendstr(NEWLINE, com->conn);
    return ESUCCESS;
  }

  sendstr("Accepted connection, connection identifier for new connection is ", com->conn);
  snprintf((char *)buffer, sizeof(buffer), "%d"NEWLINE, j);
  netconn_write(com->conn, buffer, strlen((const char *)buffer), NETCONN_COPY);

  return ESUCCESS;
}
/*-----------------------------------------------------------------------------------*/
#if LWIP_STATS
static void
com_stat_write_mem(struct netconn *conn, struct stats_mem *elem, int i)
{
  u16_t len;
  char buf[100];
  size_t slen;

#ifdef LWIP_DEBUG
  LWIP_UNUSED_ARG(i);
  slen = strlen(elem->name);
  netconn_write(conn, elem->name, slen, NETCONN_COPY);
#else /*  LWIP_DEBUG */
  len = (u16_t)sprintf(buf, "%d", i);
  slen = strlen(buf);
  netconn_write(conn, buf, slen, NETCONN_COPY);
#endif /*  LWIP_DEBUG */
  if(slen < 10) {
    netconn_write(conn, padding_10spaces, 10-slen, NETCONN_COPY);
  }

  len = (u16_t)sprintf(buf, " * available %"MEM_SIZE_F NEWLINE, elem->avail);
  netconn_write(conn, buf, len, NETCONN_COPY);
  len = (u16_t)sprintf(buf, "           * used %"MEM_SIZE_F NEWLINE, elem->used);
  netconn_write(conn, buf, len, NETCONN_COPY);
  len = (u16_t)sprintf(buf, "           * high water mark %"MEM_SIZE_F NEWLINE, elem->max);
  netconn_write(conn, buf, len, NETCONN_COPY);
  len = (u16_t)sprintf(buf, "           * errors %"STAT_COUNTER_F NEWLINE, elem->err);
  netconn_write(conn, buf, len, NETCONN_COPY);
  len = (u16_t)sprintf(buf, "           * illegal %"STAT_COUNTER_F NEWLINE, elem->illegal);
  netconn_write(conn, buf, len, NETCONN_COPY);
}
static void
com_stat_write_sys(struct netconn *conn, struct stats_syselem *elem, const char *name)
{
  u16_t len;
  char buf[100];
  size_t slen = strlen(name);

  netconn_write(conn, name, slen, NETCONN_COPY);
  if(slen < 10) {
    netconn_write(conn, padding_10spaces, 10-slen, NETCONN_COPY);
  }

  len = (u16_t)sprintf(buf, " * used %"STAT_COUNTER_F NEWLINE, elem->used);
  netconn_write(conn, buf, len, NETCONN_COPY);
  len = (u16_t)sprintf(buf, "           * high water mark %"STAT_COUNTER_F NEWLINE, elem->max);
  netconn_write(conn, buf, len, NETCONN_COPY);
  len = (u16_t)sprintf(buf, "           * errors %"STAT_COUNTER_F NEWLINE, elem->err);
  netconn_write(conn, buf, len, NETCONN_COPY);
}
static s8_t
com_stat(struct command *com)
{
#if PROTOCOL_STATS || MEMP_STATS
  size_t i;
#endif /* PROTOCOL_STATS || MEMP_STATS */
#if PROTOCOL_STATS
  size_t k;
  char buf[100];
  u16_t len;

  /* protocol stats, @todo: add IGMP */
  for(i = 0; i < num_protostats; i++) {
    size_t s = sizeof(struct stats_proto)/sizeof(STAT_COUNTER);
    STAT_COUNTER *c = &shell_stat_proto_stats[i]->xmit;
    LWIP_ASSERT("stats not in sync", s == sizeof(stat_msgs_proto)/sizeof(char*));
    netconn_write(com->conn, shell_stat_proto_names[i], strlen(shell_stat_proto_names[i]), NETCONN_COPY);
    for(k = 0; k < s; k++) {
      len = (u16_t)sprintf(buf, "%s%"STAT_COUNTER_F NEWLINE, stat_msgs_proto[k], c[k]);
      netconn_write(com->conn, buf, len, NETCONN_COPY);
    }
  }
#endif /* PROTOCOL_STATS */
#if MEM_STATS
  com_stat_write_mem(com->conn, &lwip_stats.mem, -1);
#endif /* MEM_STATS */
#if MEMP_STATS
  for(i = 0; i < MEMP_MAX; i++) {
    com_stat_write_mem(com->conn, lwip_stats.memp[i], -1);
  }
#endif /* MEMP_STATS */
#if SYS_STATS
  com_stat_write_sys(com->conn, &lwip_stats.sys.sem,   "SEM       ");
  com_stat_write_sys(com->conn, &lwip_stats.sys.mutex, "MUTEX     ");
  com_stat_write_sys(com->conn, &lwip_stats.sys.mbox,  "MBOX      ");
#endif /* SYS_STATS */

  return ESUCCESS;
}
#endif
/*-----------------------------------------------------------------------------------*/
static s8_t
com_send(struct command *com)
{
  int i;
  err_t err;
  size_t len;

  i = strtol(com->args[0], NULL, 10);

  if (i > NCONNS) {
    sendstr("Connection identifier too high."NEWLINE, com->conn);
    return ESUCCESS;
  }

  if (conns[i] == NULL) {
    sendstr("Connection identifier not in use."NEWLINE, com->conn);
    return ESUCCESS;
  }

  len = strlen(com->args[1]);
  com->args[1][len] = '\r';
  com->args[1][len + 1] = '\n';
  com->args[1][len + 2] = 0;

  err = netconn_write(conns[i], com->args[1], len + 3, NETCONN_COPY);
  if (err != ERR_OK) {
    sendstr("Could not send data: ", com->conn);
#ifdef LWIP_DEBUG
    sendstr(lwip_strerr(err), com->conn);
#else
    sendstr("(debugging must be turned on for error message to appear)", com->conn);
#endif /* LWIP_DEBUG */
    sendstr(NEWLINE, com->conn);
    return ESUCCESS;
  }

  sendstr("Data enqueued for sending."NEWLINE, com->conn);
  return ESUCCESS;
}
/*-----------------------------------------------------------------------------------*/
static s8_t
com_recv(struct command *com)
{
  int i;
  err_t err;
  struct netbuf *buf;
  u16_t len;

  i = strtol(com->args[0], NULL, 10);

  if (i > NCONNS) {
    sendstr("Connection identifier too high."NEWLINE, com->conn);
    return ESUCCESS;
  }

  if (conns[i] == NULL) {
    sendstr("Connection identifier not in use."NEWLINE, com->conn);
    return ESUCCESS;
  }

  err = netconn_recv(conns[i], &buf);
  if (err == ERR_OK) {

    netbuf_copy(buf, buffer, BUFSIZE);
    len = netbuf_len(buf);
    sendstr("Reading from connection:"NEWLINE, com->conn);
    netconn_write(com->conn, buffer, len, NETCONN_COPY);
    netbuf_delete(buf);
  } else {
    sendstr("EOF."NEWLINE, com->conn);
  }
  err = netconn_err(conns[i]);
  if (err != ERR_OK) {
    sendstr("Could not receive data: ", com->conn);
#ifdef LWIP_DEBUG
    sendstr(lwip_strerr(err), com->conn);
#else
    sendstr("(debugging must be turned on for error message to appear)", com->conn);
#endif /* LWIP_DEBUG */
    sendstr(NEWLINE, com->conn);
    return ESUCCESS;
  }
  return ESUCCESS;
}
/*-----------------------------------------------------------------------------------*/
static s8_t
com_udpc(struct command *com)
{
  ip_addr_t ipaddr;
  u16_t lport, rport;
  int i;
  err_t err;
  long tmp;

  tmp = strtol(com->args[0], NULL, 10);
  if((tmp < 0) || (tmp > 0xffff)) {
    sendstr("Invalid port number."NEWLINE, com->conn);
    return ESUCCESS;
  }
  lport = (u16_t)tmp;
  if (ipaddr_aton(com->args[1], &ipaddr) == -1) {
    sendstr(strerror(errno), com->conn);
    return ESYNTAX;
  }
  tmp = strtol(com->args[2], NULL, 10);
  if((tmp < 0) || (tmp > 0xffff)) {
    sendstr("Invalid port number."NEWLINE, com->conn);
    return ESUCCESS;
  }
  rport = (u16_t)tmp;

  /* Find the first unused connection in conns. */
  for(i = 0; i < NCONNS && conns[i] != NULL; i++);

  if (i == NCONNS) {
    sendstr("No more connections available, sorry."NEWLINE, com->conn);
    return ESUCCESS;
  }

  sendstr("Setting up UDP connection from port ", com->conn);
  netconn_write(com->conn, com->args[0], strlen(com->args[0]), NETCONN_COPY);
  sendstr(" to ", com->conn);
  netconn_write(com->conn, com->args[1], strlen(com->args[1]), NETCONN_COPY);
  sendstr(":", com->conn);
  netconn_write(com->conn, com->args[2], strlen(com->args[2]), NETCONN_COPY);
  sendstr(NEWLINE, com->conn);

  conns[i] = netconn_new(NETCONN_UDP);
  if (conns[i] == NULL) {
    sendstr("Could not create connection identifier (out of memory)."NEWLINE, com->conn);
    return ESUCCESS;
  }

  err = netconn_connect(conns[i], &ipaddr, rport);
  if (err != ERR_OK) {
    netconn_delete(conns[i]);
    conns[i] = NULL;
    sendstr("Could not connect to remote host: ", com->conn);
#ifdef LWIP_DEBUG
    sendstr(lwip_strerr(err), com->conn);
#else
    sendstr("(debugging must be turned on for error message to appear)", com->conn);
#endif /* LWIP_DEBUG */
    sendstr(NEWLINE, com->conn);
    return ESUCCESS;
  }

  err = netconn_bind(conns[i], IP_ADDR_ANY, lport);
  if (err != ERR_OK) {
    netconn_delete(conns[i]);
    conns[i] = NULL;
    sendstr("Could not bind: ", com->conn);
#ifdef LWIP_DEBUG
    sendstr(lwip_strerr(err), com->conn);
#else
    sendstr("(debugging must be turned on for error message to appear)", com->conn);
#endif /* LWIP_DEBUG */
    sendstr(NEWLINE, com->conn);
    return ESUCCESS;
  }

  sendstr("Connection set up, connection identifier is ", com->conn);
  snprintf((char *)buffer, sizeof(buffer), "%d"NEWLINE, i);
  netconn_write(com->conn, buffer, strlen((const char *)buffer), NETCONN_COPY);

  return ESUCCESS;
}
/*-----------------------------------------------------------------------------------*/
static s8_t
com_udpl(struct command *com)
{
  ip_addr_t ipaddr;
  u16_t lport, rport;
  int i;
  err_t err;
  long tmp;

  tmp = strtol(com->args[0], NULL, 10);
  if((tmp < 0) || (tmp > 0xffff)) {
    sendstr("Invalid port number."NEWLINE, com->conn);
    return ESUCCESS;
  }
  lport = (u16_t)tmp;
  if (ipaddr_aton(com->args[1], &ipaddr) == -1) {
    sendstr(strerror(errno), com->conn);
    return ESYNTAX;
  }
  tmp = strtol(com->args[2], NULL, 10);
  if((tmp < 0) || (tmp > 0xffff)) {
    sendstr("Invalid port number."NEWLINE, com->conn);
    return ESUCCESS;
  }
  rport = (u16_t)tmp;

  /* Find the first unused connection in conns. */
  for(i = 0; i < NCONNS && conns[i] != NULL; i++);

  if (i == NCONNS) {
    sendstr("No more connections available, sorry."NEWLINE, com->conn);
    return ESUCCESS;
  }

  sendstr("Setting up UDP-Lite connection from port ", com->conn);
  netconn_write(com->conn, com->args[0], strlen(com->args[0]), NETCONN_COPY);
  sendstr(" to ", com->conn);
  netconn_write(com->conn, com->args[1], strlen(com->args[1]), NETCONN_COPY);
  sendstr(":", com->conn);
  netconn_write(com->conn, com->args[2], strlen(com->args[2]), NETCONN_COPY);
  sendstr(NEWLINE, com->conn);

  conns[i] = netconn_new(NETCONN_UDPLITE);
  if (conns[i] == NULL) {
    sendstr("Could not create connection identifier (out of memory)."NEWLINE, com->conn);
    return ESUCCESS;
  }

  err = netconn_connect(conns[i], &ipaddr, rport);
  if (err != ERR_OK) {
    netconn_delete(conns[i]);
    conns[i] = NULL;
    sendstr("Could not connect to remote host: ", com->conn);
#ifdef LWIP_DEBUG
    sendstr(lwip_strerr(err), com->conn);
#else
    sendstr("(debugging must be turned on for error message to appear)", com->conn);
#endif /* LWIP_DEBUG */
    sendstr(NEWLINE, com->conn);
    return ESUCCESS;
  }

  err = netconn_bind(conns[i], IP_ADDR_ANY, lport);
  if (err != ERR_OK) {
    netconn_delete(conns[i]);
    conns[i] = NULL;
    sendstr("Could not bind: ", com->conn);
#ifdef LWIP_DEBUG
    sendstr(lwip_strerr(err), com->conn);
#else
    sendstr("(debugging must be turned on for error message to appear)", com->conn);
#endif /* LWIP_DEBUG */
    sendstr(NEWLINE, com->conn);
    return ESUCCESS;
  }

  sendstr("Connection set up, connection identifier is ", com->conn);
  snprintf((char *)buffer, sizeof(buffer), "%d"NEWLINE, i);
  netconn_write(com->conn, buffer, strlen((const char *)buffer), NETCONN_COPY);

  return ESUCCESS;
}
/*-----------------------------------------------------------------------------------*/
static s8_t
com_udpn(struct command *com)
{
  ip_addr_t ipaddr;
  u16_t lport, rport;
  int i;
  err_t err;
  long tmp;

  tmp = strtol(com->args[0], NULL, 10);
  if((tmp < 0) || (tmp > 0xffff)) {
    sendstr("Invalid port number."NEWLINE, com->conn);
    return ESUCCESS;
  }
  lport = (u16_t)tmp;
  if (ipaddr_aton(com->args[1], &ipaddr) == -1) {
    sendstr(strerror(errno), com->conn);
    return ESYNTAX;
  }
  tmp = strtol(com->args[2], NULL, 10);
  if((tmp < 0) || (tmp > 0xffff)) {
    sendstr("Invalid port number."NEWLINE, com->conn);
    return ESUCCESS;
  }
  rport = (u16_t)tmp;

  /* Find the first unused connection in conns. */
  for(i = 0; i < NCONNS && conns[i] != NULL; i++);

  if (i == NCONNS) {
    sendstr("No more connections available, sorry."NEWLINE, com->conn);
    return ESUCCESS;
  }

  sendstr("Setting up UDP connection without checksums from port ", com->conn);
  netconn_write(com->conn, com->args[0], strlen(com->args[0]), NETCONN_COPY);
  sendstr(" to ", com->conn);
  netconn_write(com->conn, com->args[1], strlen(com->args[1]), NETCONN_COPY);
  sendstr(":", com->conn);
  netconn_write(com->conn, com->args[2], strlen(com->args[2]), NETCONN_COPY);
  sendstr(NEWLINE, com->conn);

  conns[i] = netconn_new(NETCONN_UDPNOCHKSUM);
  if (conns[i] == NULL) {
    sendstr("Could not create connection identifier (out of memory)."NEWLINE, com->conn);
    return ESUCCESS;
  }

  err = netconn_connect(conns[i], &ipaddr, rport);
  if (err != ERR_OK) {
    netconn_delete(conns[i]);
    conns[i] = NULL;
    sendstr("Could not connect to remote host: ", com->conn);
#ifdef LWIP_DEBUG
    sendstr(lwip_strerr(err), com->conn);
#else
    sendstr("(debugging must be turned on for error message to appear)", com->conn);
#endif /* LWIP_DEBUG */
    sendstr(NEWLINE, com->conn);
    return ESUCCESS;
  }

  err = netconn_bind(conns[i], IP_ADDR_ANY, lport);
  if (err != ERR_OK) {
    netconn_delete(conns[i]);
    conns[i] = NULL;
    sendstr("Could not bind: ", com->conn);
#ifdef LWIP_DEBUG
    sendstr(lwip_strerr(err), com->conn);
#else
    sendstr("(debugging must be turned on for error message to appear)", com->conn);
#endif /* LWIP_DEBUG */
    sendstr(NEWLINE, com->conn);
    return ESUCCESS;
  }

  sendstr("Connection set up, connection identifier is ", com->conn);
  snprintf((char *)buffer, sizeof(buffer), "%d"NEWLINE, i);
  netconn_write(com->conn, buffer, strlen((const char *)buffer), NETCONN_COPY);

  return ESUCCESS;
}
/*-----------------------------------------------------------------------------------*/
static s8_t
com_udpb(struct command *com)
{
  ip_addr_t ipaddr;
#if LWIP_IPV4
  u16_t lport;
#endif /* LWIP_IPV4 */
  u16_t rport;
  int i;
  err_t err;
  long tmp;

  tmp = strtol(com->args[0], NULL, 10);
  if((tmp < 0) || (tmp > 0xffff)) {
    sendstr("Invalid port number."NEWLINE, com->conn);
    return ESUCCESS;
  }
#if LWIP_IPV4
  lport = (u16_t)tmp;
#endif /* LWIP_IPV4 */
  if (ipaddr_aton(com->args[1], &ipaddr) == -1) {
    sendstr(strerror(errno), com->conn);
    return ESYNTAX;
  }
  tmp = strtol(com->args[2], NULL, 10);
  if((tmp < 0) || (tmp > 0xffff)) {
    sendstr("Invalid port number."NEWLINE, com->conn);
    return ESUCCESS;
  }
  rport = (u16_t)tmp;

  /* Find the first unused connection in conns. */
  for(i = 0; i < NCONNS && conns[i] != NULL; i++);

  if (i == NCONNS) {
    sendstr("No more connections available, sorry."NEWLINE, com->conn);
    return ESUCCESS;
  }

  sendstr("Setting up UDP broadcast connection from port ", com->conn);
  netconn_write(com->conn, com->args[0], strlen(com->args[0]), NETCONN_COPY);
  sendstr(" to ", com->conn);
  netconn_write(com->conn, com->args[1], strlen(com->args[1]), NETCONN_COPY);
  sendstr(NEWLINE, com->conn);

  conns[i] = netconn_new(NETCONN_UDP);
  if (conns[i] == NULL) {
    sendstr("Could not create connection identifier (out of memory)."NEWLINE, com->conn);
    return ESUCCESS;
  }

  err = netconn_connect(conns[i], &ipaddr, rport);
  if (err != ERR_OK) {
    netconn_delete(conns[i]);
    conns[i] = NULL;
    sendstr("Could not connect to remote host: ", com->conn);
#ifdef LWIP_DEBUG
    sendstr(lwip_strerr(err), com->conn);
#else
    sendstr("(debugging must be turned on for error message to appear)", com->conn);
#endif /* LWIP_DEBUG */
    sendstr(NEWLINE, com->conn);
    return ESUCCESS;
  }

#if LWIP_IPV4
  if (IP_IS_V6_VAL(ipaddr)) {
    err = netconn_bind(conns[i], &ip_addr_broadcast, lport);
    if (err != ERR_OK) {
      netconn_delete(conns[i]);
      conns[i] = NULL;
      sendstr("Could not bind: ", com->conn);
#ifdef LWIP_DEBUG
      sendstr(lwip_strerr(err), com->conn);
#else
      sendstr("(debugging must be turned on for error message to appear)", com->conn);
#endif /* LWIP_DEBUG */
      sendstr(NEWLINE, com->conn);
      return ESUCCESS;
    }
  }
#endif /* LWIP_IPV4 */

  sendstr("Connection set up, connection identifier is ", com->conn);
  snprintf((char *)buffer, sizeof(buffer), "%d"NEWLINE, i);
  netconn_write(com->conn, buffer, strlen((const char *)buffer), NETCONN_COPY);

  return ESUCCESS;
}
/*-----------------------------------------------------------------------------------*/
static s8_t
com_usnd(struct command *com)
{
  long i;
  err_t err;
  struct netbuf *buf;
  char *mem;
  u16_t len;
  size_t tmp;

  i = strtol(com->args[0], NULL, 10);

  if (i > NCONNS) {
    sendstr("Connection identifier too high."NEWLINE, com->conn);
    return ESUCCESS;
  }

  if (conns[i] == NULL) {
    sendstr("Connection identifier not in use."NEWLINE, com->conn);
    return ESUCCESS;
  }
  tmp = strlen(com->args[1]) + 1;
  if (tmp > 0xffff) {
    sendstr("Invalid length."NEWLINE, com->conn);
    return ESUCCESS;
  }
  len = (u16_t)tmp;

  buf = netbuf_new();
  mem = (char *)netbuf_alloc(buf, len);
  if (mem == NULL) {
    sendstr("Could not allocate memory for sending."NEWLINE, com->conn);
    return ESUCCESS;
  }
  strncpy(mem, com->args[1], len);
  err = netconn_send(conns[i], buf);
  netbuf_delete(buf);
  if (err != ERR_OK) {
    sendstr("Could not send data: ", com->conn);
#ifdef LWIP_DEBUG
    sendstr(lwip_strerr(err), com->conn);
#else
    sendstr("(debugging must be turned on for error message to appear)", com->conn);
#endif /* LWIP_DEBUG */
    sendstr(NEWLINE, com->conn);
    return ESUCCESS;
  }

  sendstr("Data sent."NEWLINE, com->conn);
  return ESUCCESS;
}
/*-----------------------------------------------------------------------------------*/
#if LWIP_SOCKET
/*-----------------------------------------------------------------------------------*/
static s8_t
com_idxtoname(struct command *com)
{
  long i = strtol(com->args[0], NULL, 10);

  if (lwip_if_indextoname((unsigned int)i, (char *)buffer)) {
    netconn_write(com->conn, buffer, strlen((const char *)buffer), NETCONN_COPY);
    sendstr(NEWLINE, com->conn);
  } else {
    snprintf((char *)buffer, sizeof(buffer), "if_indextoname() failed: %d"NEWLINE, errno);
    netconn_write(com->conn, buffer, strlen((const char *)buffer), NETCONN_COPY);
  }
  return ESUCCESS;
}
/*-----------------------------------------------------------------------------------*/
static s8_t
com_nametoidx(struct command *com)
{
  unsigned int idx = lwip_if_nametoindex(com->args[0]);

  if (idx) {
    snprintf((char *)buffer, sizeof(buffer), "%u"NEWLINE, idx);
    netconn_write(com->conn, buffer, strlen((const char *)buffer), NETCONN_COPY);
  } else {
    sendstr("No interface found"NEWLINE, com->conn);
  }
  return ESUCCESS;
}
#endif /* LWIP_SOCKET */
/*-----------------------------------------------------------------------------------*/
#if LWIP_DNS
static s8_t
com_gethostbyname(struct command *com)
{
  ip_addr_t addr;
  err_t err = netconn_gethostbyname(com->args[0], &addr);

  if (err == ERR_OK) {
    if (ipaddr_ntoa_r(&addr, (char *)buffer, sizeof(buffer))) {
      sendstr("Host found: ", com->conn);
      sendstr((char *)buffer, com->conn);
      sendstr(NEWLINE, com->conn);
    } else {
        sendstr("ipaddr_ntoa_r failed", com->conn);
    }
  } else {
    sendstr("No host found"NEWLINE, com->conn);
  }
  return ESUCCESS;
}
#endif /* LWIP_DNS */
/*-----------------------------------------------------------------------------------*/
static s8_t
com_help(struct command *com)
{
  sendstr(help_msg1, com->conn);
  sendstr(help_msg2, com->conn);
  sendstr(help_msg3, com->conn);
  return ESUCCESS;
}
/*-----------------------------------------------------------------------------------*/
static s8_t
parse_command(struct command *com, u32_t len)
{
  u16_t i;
  u16_t bufp;

  if (strncmp((const char *)buffer, "open", 4) == 0) {
    com->exec = com_open;
    com->nargs = 2;
  } else if (strncmp((const char *)buffer, "lstn", 4) == 0) {
    com->exec = com_lstn;
    com->nargs = 1;
  } else if (strncmp((const char *)buffer, "acpt", 4) == 0) {
    com->exec = com_acpt;
    com->nargs = 1;
  } else if (strncmp((const char *)buffer, "clos", 4) == 0) {
    com->exec = com_clos;
    com->nargs = 1;
#if LWIP_STATS
  } else if (strncmp((const char *)buffer, "stat", 4) == 0) {
    com->exec = com_stat;
    com->nargs = 0;
#endif
  } else if (strncmp((const char *)buffer, "send", 4) == 0) {
    com->exec = com_send;
    com->nargs = 2;
  } else if (strncmp((const char *)buffer, "recv", 4) == 0) {
    com->exec = com_recv;
    com->nargs = 1;
  } else if (strncmp((const char *)buffer, "udpc", 4) == 0) {
    com->exec = com_udpc;
    com->nargs = 3;
  } else if (strncmp((const char *)buffer, "udpb", 4) == 0) {
    com->exec = com_udpb;
    com->nargs = 2;
  } else if (strncmp((const char *)buffer, "udpl", 4) == 0) {
    com->exec = com_udpl;
    com->nargs = 3;
  } else if (strncmp((const char *)buffer, "udpn", 4) == 0) {
    com->exec = com_udpn;
    com->nargs = 3;
  } else if (strncmp((const char *)buffer, "usnd", 4) == 0) {
    com->exec = com_usnd;
    com->nargs = 2;
#if LWIP_SOCKET
  } else if (strncmp((const char *)buffer, "idxtoname", 9) == 0) {
    com->exec = com_idxtoname;
    com->nargs = 1;
  } else if (strncmp((const char *)buffer, "nametoidx", 9) == 0) {
    com->exec = com_nametoidx;
    com->nargs = 1;
#endif /* LWIP_SOCKET */
#if LWIP_DNS
  } else if (strncmp((const char *)buffer, "gethostnm", 9) == 0) {
    com->exec = com_gethostbyname;
    com->nargs = 1;
#endif /* LWIP_DNS */
  } else if (strncmp((const char *)buffer, "help", 4) == 0) {
    com->exec = com_help;
    com->nargs = 0;
  } else if (strncmp((const char *)buffer, "quit", 4) == 0) {
    printf("quit"NEWLINE);
    return ECLOSED;
  } else {
    return ESYNTAX;
  }

  if (com->nargs == 0) {
    return ESUCCESS;
  }
  bufp = 0;
  for(; bufp < len && buffer[bufp] != ' '; bufp++);
  for(i = 0; i < 10; i++) {
    for(; bufp < len && buffer[bufp] == ' '; bufp++);
    if (buffer[bufp] == '\r' ||
       buffer[bufp] == '\n') {
      buffer[bufp] = 0;
      if (i < com->nargs - 1) {
        return ETOOFEW;
      }
      if (i > com->nargs - 1) {
        return ETOOMANY;
      }
      break;
    }
    if (bufp > len) {
      return ETOOFEW;
    }
    com->args[i] = (char *)&buffer[bufp];
    for(; bufp < len && buffer[bufp] != ' ' && buffer[bufp] != '\r' &&
      buffer[bufp] != '\n'; bufp++) {
      if (buffer[bufp] == '\\') {
        buffer[bufp] = ' ';
      }
    }
    if (bufp > len) {
      return ESYNTAX;
    }
    buffer[bufp] = 0;
    bufp++;
    if (i == com->nargs - 1) {
      break;
    }

  }

  return ESUCCESS;
}
/*-----------------------------------------------------------------------------------*/
static void
shell_error(s8_t err, struct netconn *conn)
{
  switch (err) {
  case ESYNTAX:
    sendstr("## Syntax error"NEWLINE, conn);
    break;
  case ETOOFEW:
    sendstr("## Too few arguments to command given"NEWLINE, conn);
    break;
  case ETOOMANY:
    sendstr("## Too many arguments to command given"NEWLINE, conn);
    break;
  case ECLOSED:
    sendstr("## Connection closed"NEWLINE, conn);
    break;
  default:
    /* unknown error, don't assert here */
    break;
  }
}
/*-----------------------------------------------------------------------------------*/
static void
prompt(struct netconn *conn)
{
  sendstr("> ", conn);
}
/*-----------------------------------------------------------------------------------*/
static void
shell_main(struct netconn *conn)
{
  struct pbuf *p;
  u16_t len = 0, cur_len;
  struct command com;
  s8_t err;
  int i;
  err_t ret;
#if SHELL_ECHO
  void *echomem;
#endif /* SHELL_ECHO */

  do {
    ret = netconn_recv_tcp_pbuf(conn, &p);
    if (ret == ERR_OK) {
      pbuf_copy_partial(p, &buffer[len], (u16_t)(BUFSIZE - len), 0);
      cur_len = p->tot_len;
      len = (u16_t)(len + cur_len);
      if ((len < cur_len) || (len > BUFSIZE)) {
        len = BUFSIZE;
      }
#if SHELL_ECHO
      echomem = mem_malloc(cur_len);
      if (echomem != NULL) {
        pbuf_copy_partial(p, echomem, cur_len, 0);
        netconn_write(conn, echomem, cur_len, NETCONN_COPY);
        mem_free(echomem);
      }
#endif /* SHELL_ECHO */
      pbuf_free(p);
      if (((len > 0) && ((buffer[len-1] == '\r') || (buffer[len-1] == '\n'))) ||
          (len >= BUFSIZE)) {
        if (buffer[0] != 0xff &&
           buffer[1] != 0xfe) {
          err = parse_command(&com, len);
          if (err == ESUCCESS) {
            com.conn = conn;
            err = com.exec(&com);
          }
          if (err == ECLOSED) {
            printf("Closed"NEWLINE);
            shell_error(err, conn);
            goto close;
          }
          if (err != ESUCCESS) {
            shell_error(err, conn);
          }
        } else {
          sendstr(NEWLINE NEWLINE
                  "lwIP simple interactive shell."NEWLINE
                  "(c) Copyright 2001, Swedish Institute of Computer Science."NEWLINE
                  "Written by Adam Dunkels."NEWLINE
                  "For help, try the \"help\" command."NEWLINE, conn);
        }
        if (ret == ERR_OK) {
          prompt(conn);
        }
        len = 0;
      }
    }
  } while (ret == ERR_OK);
  printf("err %s"NEWLINE, lwip_strerr(ret));

close:
  netconn_close(conn);

  for(i = 0; i < NCONNS; i++) {
    if (conns[i] != NULL) {
      netconn_delete(conns[i]);
    }
    conns[i] = NULL;
  }
}
/*-----------------------------------------------------------------------------------*/
static void
shell_thread(void *arg)
{
  struct netconn *conn, *newconn;
  err_t err;
  LWIP_UNUSED_ARG(arg);

#if LWIP_IPV6
  conn = netconn_new(NETCONN_TCP_IPV6);
  LWIP_ERROR("shell: invalid conn", (conn != NULL), return;);
  err = netconn_bind(conn, IP6_ADDR_ANY, 23);
#else /* LWIP_IPV6 */
  conn = netconn_new(NETCONN_TCP);
  LWIP_ERROR("shell: invalid conn", (conn != NULL), return;);
  err = netconn_bind(conn, IP_ADDR_ANY, 23);
#endif /* LWIP_IPV6 */
  LWIP_ERROR("shell: netconn_bind failed", (err == ERR_OK), netconn_delete(conn); return;);
  err = netconn_listen(conn);
  LWIP_ERROR("shell: netconn_listen failed", (err == ERR_OK), netconn_delete(conn); return;);

  while (1) {
    err = netconn_accept(conn, &newconn);
    if (err == ERR_OK) {
      shell_main(newconn);
      netconn_delete(newconn);
    }
  }
}
/*-----------------------------------------------------------------------------------*/
void
shell_init(void)
{
  sys_thread_new("shell_thread", shell_thread, NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
}

#endif /* LWIP_NETCONN && LWIP_TCP */
