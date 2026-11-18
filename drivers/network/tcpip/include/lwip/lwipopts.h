/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        drivers/network/tcpip/include/lwip/lwipopts.h
 * PURPOSE:     Product lwIP options for the TCP/IP driver
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 *
 * IPv6 enablement cites RFC 4291 / RFC 4861 for the companion VerOS IP
 * path (https://www.rfc-editor.org/rfc/rfc4291,
 * https://www.rfc-editor.org/rfc/rfc4861). Zones stay off in lwIP
 * (RFC 4007, https://www.rfc-editor.org/rfc/rfc4007).
 */

/*
   ------------------------------------
   ---------- Memory options ----------
   ------------------------------------
*/

/* This combo allows us to implement malloc, free, and realloc ourselves */
#define MEM_LIBC_MALLOC                 1
#define MEMP_MEM_MALLOC                 1

/* Define LWIP_COMPAT_MUTEX if the port has no mutexes and binary semaphores
 should be used instead */
#define LWIP_COMPAT_MUTEX               1
#define LWIP_COMPAT_MUTEX_ALLOWED       1

#define MEM_ALIGNMENT                   4

#define LWIP_ARP                        0

#define ETH_PAD_SIZE                    2

#define IP_REASS_MAX_PBUFS              0xFFFFFFFF

#define IP_DEFAULT_TTL                  128

#define IP_SOF_BROADCAST                1

#define IP_SOF_BROADCAST_RECV           1

#define LWIP_ICMP                       0

#define LWIP_RAW                        0

#define LWIP_UDP                        0

/*
 * Dual-stack: keep IPv4; enable IPv6 core for TCP over lwIP.
 * Link-local and NDP for ICMP/raw stay in the VerOS IP path (RFC 4291 /
 * RFC 4861). Disable lwIP autoconfig/RS/MLD/DHCPv6 so addresses and
 * neighbor resolution are not duplicated.
 */
#define LWIP_IPV4                       1
#define LWIP_IPV6                       1
#define LWIP_IPV6_MLD                   0
#define LWIP_IPV6_AUTOCONFIG            0
#define LWIP_IPV6_SEND_ROUTER_SOLICIT   0
#define LWIP_IPV6_DUP_DETECT_ATTEMPTS   0
#define LWIP_IPV6_DHCP6                 0
#define LWIP_ND6_QUEUEING               0
/* Zones stay with the VerOS address (RFC 4007,
 * https://www.rfc-editor.org/rfc/rfc4007). Link-local TCP binds the
 * 16-byte address the netif already holds. */
#define LWIP_IPV6_SCOPES                0
/* amd64 pointers exceed IP6_FRAG_HLEN; lwIP ip6_frag.c asserts without this.
 * Guest: output/ndis-disk-20261116T233301Z.log (ip6_frag.c:119). */
#define IPV6_FRAG_COPYHEADER            1

#define SO_REUSE                        1

#define SO_REUSE_RXTOALL                1

/* FIXME: These MSS and TCP Window definitions assume an MTU
 * of 1500. We need to add some code to lwIP which would allow us
 * to change these values based upon the interface we are
 * using. Currently ReactOS only supports Ethernet so we're
 * fine for now but it does need to be fixed later when we
 * add support for other transport mediums */
#define TCP_MSS                         1460

#define TCP_WND                         0xFFFF

#define TCP_SND_BUF                     TCP_WND

#define TCP_MAXRTX                      8

#define TCP_SYNMAXRTX                   4

#define TCP_LISTEN_BACKLOG              1

#define LWIP_TCP_TIMESTAMPS             1

#define LWIP_SOCKET                     0

#define LWIP_NETCONN                    0

#define LWIP_STATS                      0

#define ICMP_STATS                      0

/*
   ---------------------------------------
   ---------- Debugging options ----------
   ---------------------------------------
*/

/**
 * TCP_DEBUG: Enable debugging for TCP.
 */
#define TCP_DEBUG                       LWIP_DBG_ON
