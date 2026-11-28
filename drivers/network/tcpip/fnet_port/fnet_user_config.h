/*
 * PROJECT:     ReactOS TCP/IP protocol driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     FNET configuration for tcpip.sys
 * COPYRIGHT:   Copyright 2026 Muhammad Yahya Bin Ehsan Qureshi <yahyauni54@gmail.com>
 */

#ifndef _FNET_USER_CONFIG_H_
#define _FNET_USER_CONFIG_H_

#include "fnet_ros_cpu.h"

/* Compiler */
#if defined(_MSC_VER)
    /* Tested first: clang-cl defines both _MSC_VER and __clang__ */
    #define FNET_COMP_STR                       "MSVC"
    #define FNET_COMP_PACKED_BEGIN              __pragma(pack(push, 1))
    #define FNET_COMP_PACKED_END                __pragma(pack(pop))
#elif defined(__GNUC__) || defined(__clang__)
    #define FNET_CFG_COMP_GNUC                  (1)
#else
    #error Unsupported compiler
#endif

/* CPU */
#if defined(_M_AMD64) || defined(__x86_64__)
    #define FNET_CPU_STR                        "x64"
#else
    #define FNET_CPU_STR                        "x86"
#endif
#define FNET_CFG_CPU_CLOCK_HZ                   (0)
#define FNET_CFG_CPU_TIMER_VECTOR_NUMBER        (0)
#define FNET_CFG_CPU_LITTLE_ENDIAN              (1)
#define FNET_CFG_CPU_ETH0                       (0)
#define FNET_CFG_CPU_ETH1                       (0)
#define FNET_CFG_CPU_TIMER_NUMBER               (0)
#define FNET_CFG_CPU_TIMER_NUMBER_MAX           (0)
#define FNET_CFG_CPU_TIMER_VECTOR_PRIORITY      (1)
#define FNET_CFG_CPU_ETH_VECTOR_PRIORITY        (1)
#define FNET_CFG_CPU_VECTOR_PRIORITY_MAX        (1)
#define FNET_CFG_CPU_ETH_TX_BUFS_MAX            (2)
#define FNET_CFG_CPU_ETH_RX_BUFS_MAX            (2)

#define FNET_UINTPTR_T                          fnet_uintptr_t

/* Use the compiler and kernel memcpy */
#define FNET_CFG_OVERLOAD_MEMCPY                (1)
#define fnet_memcpy(to_ptr, from_ptr, n)        memcpy((to_ptr), (from_ptr), (n))

/* Timer comes from the port (KeQueryInterruptTime), polled by the worker thread */
#define FNET_CFG_TIMER_ALT                      (1)

/* One thread at a time enters the stack, under the port's FNET mutex */
#define FNET_CFG_MULTITHREADING                 (0)

/* Protocols. ReactOS keeps its own ARP, ICMP, UDP, raw IP and loopback. */
#define FNET_CFG_IP4                            (1)
#define FNET_CFG_IP6                            (0)
#define FNET_CFG_TCP                            (1)
#define FNET_CFG_UDP                            (0)
#define FNET_CFG_RAW                            (0)
#define FNET_CFG_LOOPBACK                       (0)
#define FNET_CFG_IGMP                           (0)
#define FNET_CFG_MULTICAST                      (0)
#define FNET_CFG_IP4_FRAGMENTATION              (0)

/* The glue's interfaces are FNET_NETIF_TYPE_OTHER and only carry TCP (see fnet_shim.c) */
#define FNET_CFG_ETHERNET                       (0)
#define FNET_CFG_ICMP4                          (0)

/* fnet_ip4_frag_header_t overlays the IP header and holds three pointers, so it does not fit on x64 */
#if FNET_CFG_IP4_FRAGMENTATION && defined(_WIN64)
    #error FNET IPv4 fragmentation is not usable on x64
#endif

/* ReactOS reassembles before handing packets to TCP, so allow full size datagrams */
#define FNET_CFG_IP_MAX_PACKET                  (65535U)

/* Heap unit size must hold fnet_mempool_unit_header_t (8 bytes on x86, 12 on x64) */
#if defined(_WIN64)
    #define FNET_CFG_HEAP_ALIGN                 FNET_MEMPOOL_ALIGN_16
#else
    #define FNET_CFG_HEAP_ALIGN                 FNET_MEMPOOL_ALIGN_8
#endif

/* The socket table holds pointers only; lwIP had no fixed connection limit */
#define FNET_CFG_SOCKET_MAX                     (4096U)

#define FNET_CFG_SOCKET_BSD_NAMES               (0)
#define FNET_CFG_SOCKET_TCP_MSS                 (0U)

/* Per socket limits on heap use, not reservations */
#define FNET_CFG_SOCKET_TCP_TX_BUF_SIZE         (64U * 1024U)
#define FNET_CFG_SOCKET_TCP_RX_BUF_SIZE         (255U * 1024U)

/* Per socket notifications for the glue (ReactOS patch, see REACTOS_PATCHES.md) */
#define FNET_CFG_SOCKET_EVENT_CALLBACK          (1)

/* No services in the kernel */
#define FNET_CFG_DHCP_CLN                       (0)
#define FNET_CFG_DHCP_SRV                       (0)
#define FNET_CFG_DNS                            (0)
#define FNET_CFG_AUTOIP                         (0)
#define FNET_CFG_LINK                           (0)
#define FNET_CFG_PING                           (0)
#define FNET_CFG_SNTP                           (0)
#define FNET_CFG_LLMNR                          (0)
#define FNET_CFG_MDNS                           (0)
#define FNET_CFG_HTTP_SRV                       (0)
#define FNET_CFG_HTTP_CLN                       (0)
#define FNET_CFG_TELNET                         (0)
#define FNET_CFG_TFTP_CLN                       (0)
#define FNET_CFG_TFTP_SRV                       (0)
#define FNET_CFG_TLS                            (0)
#define FNET_CFG_FS                             (0)
#define FNET_CFG_FLASH                          (0)
#define FNET_CFG_BENCH_SRV                      (0)
#define FNET_CFG_BENCH_CLN                      (0)
#define FNET_CFG_AZURE                          (0)

/* Debug output goes to DbgPrint, see fnet_ros_port.c */
#if DBG
    #define FNET_CFG_DEBUG                      (1)
#else
    #define FNET_CFG_DEBUG                      (0)
#endif
#define FNET_CFG_ASSERT                         (0)

#endif /* _FNET_USER_CONFIG_H_ */
