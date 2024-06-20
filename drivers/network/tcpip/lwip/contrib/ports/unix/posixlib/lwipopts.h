/*
 * Copyright (C) 2023 Joan Lledó <jlledom@member.fsf.org>
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
 */

#ifndef UNIX_LWIP_LWIPOPTS_H
#define UNIX_LWIP_LWIPOPTS_H

/* An OS is present */
#define NO_SYS    0

/* Sockets API config */
#define LWIP_COMPAT_SOCKETS       0
#define LWIP_SOCKET_OFFSET        1
#define LWIP_POLL                 1

/* User posix socket headers */
#define LWIP_SOCKET_EXTERNAL_HEADERS            1
#define LWIP_SOCKET_EXTERNAL_HEADER_SOCKETS_H   "posix/sockets.h"
#define LWIP_SOCKET_EXTERNAL_HEADER_INET_H      "posix/inet.h"
#define LWIP_DONT_PROVIDE_BYTEORDER_FUNCTIONS   1

/* Use Glibc malloc()/free() */
#define MEM_LIBC_MALLOC   1
#define MEMP_MEM_MALLOC   1
#define MEM_USE_POOLS     0

/* Only send complete packets to the device */
#define LWIP_NETIF_TX_SINGLE_PBUF 1

/* Randomize local ports */
#define LWIP_RANDOMIZE_INITIAL_LOCAL_PORTS  1

/* Glibc sends more than one packet in a row during an ARP resolution */
#define ARP_QUEUEING    1
#define ARP_QUEUE_LEN   10

/*
 * Activate loopback, but don't use lwip's default loopback interface,
 * we provide our own.
 */
#define LWIP_NETIF_LOOPBACK   1
#define LWIP_HAVE_LOOPIF      0

/* IPv4 stuff */
#define IP_FORWARD  1

/* SLAAC support and other IPv6 stuff */
#define LWIP_IPV6_DUP_DETECT_ATTEMPTS 1
#define LWIP_IPV6_SEND_ROUTER_SOLICIT 1
#define LWIP_IPV6_AUTOCONFIG          1
#define LWIP_IPV6_FORWARD             1
#define MEMP_NUM_MLD6_GROUP           16
#define LWIP_IPV6_NUM_ADDRESSES       6
#define IPV6_FRAG_COPYHEADER          1

/* TCP tuning */
#define TCP_MSS         1460
#define TCP_WND         0xFFFF
#define LWIP_WND_SCALE  1
#define TCP_RCV_SCALE   0x1
#define TCP_SND_BUF     TCP_WND

/* Throughput settings */
#define LWIP_CHECKSUM_ON_COPY   1

/* Disable stats */
#define LWIP_STATS          0
#define LWIP_STATS_DISPLAY  0

/* Enable all socket operations */
#define LWIP_TCP_KEEPALIVE          1
#define LWIP_SO_SNDTIMEO            1
#define LWIP_SO_RCVTIMEO            1
#define LWIP_SO_RCVBUF              1
#define LWIP_SO_LINGER              1
#define SO_REUSE                    1
#define LWIP_MULTICAST_TX_OPTIONS   1

/* Enable modules */
#define LWIP_ARP              1
#define LWIP_ETHERNET         1
#define LWIP_IPV4             1
#define LWIP_ICMP             1
#define LWIP_IGMP             1
#define LWIP_RAW              1
#define LWIP_UDP              1
#define LWIP_UDPLITE          1
#define LWIP_TCP              1
#define LWIP_IPV6             1
#define LWIP_ICMP6            1
#define LWIP_IPV6_MLD         1

/* Don't abort the whole stack when an error is detected */
#define LWIP_NOASSERT_ON_ERROR   1

/* Threading options */
#define LWIP_TCPIP_CORE_LOCKING   1

/* If the system is 64 bit */
#if defined __LP64__
#define MEM_ALIGNMENT            8
#else
#define MEM_ALIGNMENT            4
#endif

#if !NO_SYS
void sys_check_core_locking(void);
#define LWIP_ASSERT_CORE_LOCKED()  sys_check_core_locking()
#if 0
void sys_mark_tcpip_thread(void);
#define LWIP_MARK_TCPIP_THREAD()   sys_mark_tcpip_thread()

#if LWIP_TCPIP_CORE_LOCKING
void sys_lock_tcpip_core(void);
#define LOCK_TCPIP_CORE()          sys_lock_tcpip_core()
void sys_unlock_tcpip_core(void);
#define UNLOCK_TCPIP_CORE()        sys_unlock_tcpip_core()
#endif
#endif
#endif

/* Debug mode */
#ifdef LWIP_DEBUG
#define ETHARP_DEBUG      LWIP_DBG_OFF
#define NETIF_DEBUG       LWIP_DBG_OFF
#define PBUF_DEBUG        LWIP_DBG_OFF
#define API_LIB_DEBUG     LWIP_DBG_OFF
#define API_MSG_DEBUG     LWIP_DBG_OFF
#define SOCKETS_DEBUG     LWIP_DBG_OFF
#define ICMP_DEBUG        LWIP_DBG_OFF
#define IGMP_DEBUG        LWIP_DBG_OFF
#define INET_DEBUG        LWIP_DBG_OFF
#define IP_DEBUG          LWIP_DBG_OFF
#define IP_REASS_DEBUG    LWIP_DBG_OFF
#define RAW_DEBUG         LWIP_DBG_OFF
#define MEM_DEBUG         LWIP_DBG_OFF
#define MEMP_DEBUG        LWIP_DBG_OFF
#define SYS_DEBUG         LWIP_DBG_OFF
#define TIMERS_DEBUG      LWIP_DBG_OFF
#define TCP_DEBUG         LWIP_DBG_OFF
#define TCP_INPUT_DEBUG   LWIP_DBG_OFF
#define TCP_FR_DEBUG      LWIP_DBG_OFF
#define TCP_RTO_DEBUG     LWIP_DBG_OFF
#define TCP_CWND_DEBUG    LWIP_DBG_OFF
#define TCP_WND_DEBUG     LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG  LWIP_DBG_OFF
#define TCP_RST_DEBUG     LWIP_DBG_OFF
#define TCP_QLEN_DEBUG    LWIP_DBG_OFF
#define UDP_DEBUG         LWIP_DBG_OFF
#define TCPIP_DEBUG       LWIP_DBG_OFF
#define SLIP_DEBUG        LWIP_DBG_OFF
#define DHCP_DEBUG        LWIP_DBG_OFF
#define AUTOIP_DEBUG      LWIP_DBG_OFF
#define DNS_DEBUG         LWIP_DBG_OFF
#define IP6_DEBUG         LWIP_DBG_OFF
#endif

#endif /* UNIX_LWIP_LWIPOPTS_H */
