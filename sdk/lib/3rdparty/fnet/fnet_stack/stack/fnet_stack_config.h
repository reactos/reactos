/**************************************************************************
*
* Copyright 2008-2018 by Andrey Butok. FNET Community
*
***************************************************************************
*
*  Licensed under the Apache License, Version 2.0 (the "License"); you may
*  not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*  http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
*  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
***************************************************************************
*
* Main TCP/IP stack default configuration file.
*
***************************************************************************/

/************************************************************************
 * !!!DO NOT MODIFY THIS FILE!!!
 ************************************************************************/

#ifndef _FNET_STACK_CONFIG_H_

#define _FNET_STACK_CONFIG_H_

/*****************************************************************************
*     IP6-specific parameters.
******************************************************************************/
/*! @addtogroup fnet_platform_stack_ip6_config  */
/*! @{ */

/**************************************************************************/ /*!
 * @def     FNET_CFG_IP6
 * @brief   IPv6 protocol support:
 *               - @b @c 1 = is enabled.
 *               - @c 0 = is disabled (Default value).@n
 *
 * @see FNET_CFG_IP4
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_IP6
    #define FNET_CFG_IP6                        (0)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_ND6_NEIGHBOR_CACHE_SIZE
 * @brief   Maximum number of entries in neighbor cache (per interface).
 *
 * @note    A small cache size may result in an
 *          excessive number of Neighbor Discovery messages if entries are
 *          discarded and rebuilt in quick succession.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_ND6_NEIGHBOR_CACHE_SIZE
    #define FNET_CFG_ND6_NEIGHBOR_CACHE_SIZE     (5u)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_ND6_PREFIX_LIST_SIZE
 * @brief   Maximum number of entries in the Prefix list (per interface).
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_ND6_PREFIX_LIST_SIZE
    #define FNET_CFG_ND6_PREFIX_LIST_SIZE       (4u)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_ND6_ROUTER_LIST_SIZE
 * @brief   Maximum number of entries in the Default Router list (per interface).
 *
 * @note    RFC4861: However, a host MUST retain at least two router
 *          addresses and SHOULD retain more.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_ND6_ROUTER_LIST_SIZE
    #define FNET_CFG_ND6_ROUTER_LIST_SIZE       (2u)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_ND6_DAD_TRANSMITS
 * @brief   (RFC4862 5.1)The number of consecutive Neighbor
 *          Solicitation messages sent while performing Duplicate Address
 *          Detection on a tentative address. A value of zero indicates that
 *          Duplicate Address Detection is not performed on tentative
 *          addresses. A value of one indicates a single transmission with no
 *          follow-up retransmissions. @n
 *          Default is @c 1.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_ND6_DAD_TRANSMITS
    #define FNET_CFG_ND6_DAD_TRANSMITS          (1u)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_ND6_RDNSS
 * @brief   Recursive DNS Server Option (RFC6106) support via router advertisemnt:
 *               - @b @c 1 = is enabled (Default value).
 *               - @c 0 = is disabled.@n
 * @see FNET_CFG_ND6_RDNSS_LIST_SIZE
 *
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_ND6_RDNSS
    #define FNET_CFG_ND6_RDNSS                  (1)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_ND6_RDNSS_LIST_SIZE
 * @brief   Maximum number of entries in the RDNSS address list (per interface).
 *
 * @note    RFC6106: Specified a sufficient number of RDNSS addresses as three.
 *
 * @see FNET_CFG_ND6_RDNSS
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_ND6_RDNSS_LIST_SIZE
    #define FNET_CFG_ND6_RDNSS_LIST_SIZE        (3u)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_IP6_FRAGMENTATION
 * @brief    IPv6 fragmentation:
 *               - @b @c 1 = is enabled (Default value). @n The IPv6 will attempt
 *                        to reassemble the IPv6 packet fragments and will be able to
 *                        generate fragmented IPv6 packets.
 *               - @c 0 = is disabled. The IPv6 will
 *                        silently discard the fragmented IPv6 packets..
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_IP6_FRAGMENTATION
    #define FNET_CFG_IP6_FRAGMENTATION          (1)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_IP6_PMTU_DISCOVERY
 * @brief    Path MTU Discovery for IPv6:
 *               - @b @c 1 = is enabled (Default value).
 *               - @c 0 = is disabled.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_IP6_PMTU_DISCOVERY
    #define FNET_CFG_IP6_PMTU_DISCOVERY         (1)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_NETIF_IP6_ADDR_MAX
 * @brief    Maximum number of IPv6 addresses that can be bound to an interface.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_NETIF_IP6_ADDR_MAX
    #define FNET_CFG_NETIF_IP6_ADDR_MAX         (5u)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_MLD
 * @brief    Multicast Listener Discovery (MLDv1) support:
 *               - @c 1 = is enabled. Its enabling is optional for multicast
 *                        traffic that takes place inside only one local network.
 *               - @b @c 0 = is disabled (Default value).
 *
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_MLD
    #define FNET_CFG_MLD                        (0)
#endif


/*! @} */

/*! @addtogroup fnet_platform_stack_ip4_config  */
/*! @{ */

/**************************************************************************/ /*!
 * @def     FNET_CFG_IP4
 * @brief   IPv4 protocol support:
 *               - @b @c 1 = is enabled (Default value).
 *               - @c 0 = is disabled.@n
 *
 * @see FNET_CFG_IP6
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_IP4
    #define FNET_CFG_IP4                        (1)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_IP4_FRAGMENTATION
 * @brief    IP fragmentation:
 *               - @c 1 = is enabled. The IP will attempt
 *                        to reassemble the IP packet fragments and will be able to
 *                        generate fragmented IP packets.
 *               - @b @c 0 = is disabled (Default value). The IP will
 *                        silently discard the fragmented IP packets..
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_IP4_FRAGMENTATION
    #define FNET_CFG_IP4_FRAGMENTATION          (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_LOOPBACK_IP4_ADDR
 * @brief    Defines the IP address for the Loopback interface.
 *           By default it is set to 127.0.0.1.
 *           At runtime, it can be changed by @ref fnet_netif_set_ip4_addr().
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_LOOPBACK_IP4_ADDR
    #define FNET_CFG_LOOPBACK_IP4_ADDR   (FNET_IP4_ADDR_INIT(127U, 0U, 0U, 1U))
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_IGMP
 * @brief    Internet Group Management Protocol (IGMP) support:
 *               - @c 1 = is enabled. It sets @ref FNET_CFG_MULTICAST to 1 automatically.
 *               - @b @c 0 = is disabled (Default value).@n
 * @see FNET_CFG_IGMP_VERSION
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_IGMP
    #define FNET_CFG_IGMP                   (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_IGMP_VERSION
 * @brief    Internet Group Management Protocol (IGMP) version:
 *               - @c 1 = IGMPv1 - RFC1112.
 *               - @b @c 2 = IGMPv2 - RFC2236 (Default value).@n
 * @see FNET_CFG_IGMP
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_IGMP_VERSION
    #define FNET_CFG_IGMP_VERSION           (2)
#endif

/*! @} */

/*! @addtogroup fnet_stack_config */
/*! @{ */

/*****************************************************************************
*     TCP/IP stack general parameters.
******************************************************************************/

/**************************************************************************/ /*!
 * @def      FNET_CFG_ASSET
 * @brief    FNET_ASSERT() support:
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled (Default value).@n
 *           @n
 *           Asserts check that the given condition is true, otherwise it prints error message
 *           and stops the program execution. By default, it is disabled.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_ASSET
    #define FNET_CFG_ASSET                  (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_TCP
 * @brief    TCP protocol support:
 *               - @b @c 1 = is enabled (Default value).
 *               - @c 0 = is disabled.@n
 *           @n
 *           You can disable it to save a substantial amount of code, if
 *           your application needs the UDP only. By default, it is enabled.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_TCP
    #define FNET_CFG_TCP                    (1)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_LOOPBACK
 * @brief    Loopback interface:
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled (Default value).
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_LOOPBACK
    #define FNET_CFG_LOOPBACK              (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_LOOPBACK_NAME
 * @brief    Defines name for the Loopback interface.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_LOOPBACK_NAME
    #define FNET_CFG_LOOPBACK_NAME            "loop"
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_LOOPBACK_MULTICAST
 * @brief    Local loopback of multicast datagrams:
 *               - @c 1 = is enabled.@n
 *                        It means that the sending system should receive a
 *                        copy of the multicast datagrams that are transmitted.@n
 *                        It is valid only if @ref FNET_CFG_LOOPBACK is ser to @c 1.
 *               - @b @c 0 = is disabled (Default value).@n
 * @see FNET_CFG_LOOPBACK
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_LOOPBACK_MULTICAST
    #define FNET_CFG_LOOPBACK_MULTICAST     (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_LOOPBACK_BROADCAST
 * @brief    Local loopback of broadcast datagrams:
 *               - @c 1 = is enabled.@n
 *                        It means that the sending system should receive a
 *                        copy of the broadcast datagrams that are transmitted.@n
 *                        It is valid only if @ref FNET_CFG_LOOPBACK is ser to @c 1.
 *               - @b @c 0 = is disabled (Default value).@n
 * @see FNET_CFG_LOOPBACK
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_LOOPBACK_BROADCAST
    #define FNET_CFG_LOOPBACK_BROADCAST     (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_LOOPBACK_MTU
 * @brief    Defines the Maximum Transmission Unit for the Loopback interface.
 *           By default, it is set to 1576.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_LOOPBACK_MTU
    #define FNET_CFG_LOOPBACK_MTU           (1576U)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_MULTICAST
 * @brief    Multicast group support:
 *               - @c 1 = is enabled (Default value). @n
 *                 It is set automatically  to @c 1, if @c FNET_CFG_IP6 is set to @c 1.
 *               - @c 0 = is disabled .@n
 * @see FNET_CFG_MULTICAST_MAX, FNET_CFG_MULTICAST_SOCKET_MAX, FNET_CFG_IGMP
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_MULTICAST
    #define FNET_CFG_MULTICAST              (1)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_MULTICAST_MAX
 * @brief    Maximum number of unique multicast memberships may exist at
 *           the same time in the whole system.@n
 *           You may join the same host group address on multiple interfaces.
 * @see FNET_CFG_MULTICAST, FNET_CFG_MULTICAST_SOCKET_MAX
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_MULTICAST_MAX
    #define FNET_CFG_MULTICAST_MAX          (10u)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_MULTICAST_SOCKET_MAX
 * @brief    Maximum number of multicast memberships may exist at
 *           the same time per one socket.
 * @see FNET_CFG_MULTICAST, FNET_CFG_MULTICAST_MAX
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_MULTICAST_SOCKET_MAX
    #define FNET_CFG_MULTICAST_SOCKET_MAX   (1u)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_ARP_TABLE_SIZE
 * @brief    Maximum number of entries in the ARP table, by network interface.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_ARP_TABLE_SIZE
    #define FNET_CFG_ARP_TABLE_SIZE         (10U)
#endif

/**************************************************************************/ /*!
 * @def     FNET_CFG_ARP_EXPIRE_TIMEOUT
 * @brief   Period of time after which ARP cache entries are automatically expired (in seconds).@n
 *          If set to @c 0, the expiration is disabled (not recommended).
 *          Default value is 1200 seconds (20 minutes).
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_ARP_EXPIRE_TIMEOUT
    #define FNET_CFG_ARP_EXPIRE_TIMEOUT     (1200U)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_TCP_DISCARD_OUT_OF_ORDER
 * @brief    Discarding of TCP segments that are received out of order:
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled (Default value).@n
 *           @n
 *           But you may enable it, to save RAM.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_TCP_DISCARD_OUT_OF_ORDER
    #define FNET_CFG_TCP_DISCARD_OUT_OF_ORDER   (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_TCP_URGENT
 * @brief    TCP "urgent" (out-of-band) data processing:
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled (Default value).
 * @see SO_OOBINLINE, TCP_URGRCVD, TCP_BSD, MSG_OOB
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_TCP_URGENT
    #define FNET_CFG_TCP_URGENT                 (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_UDP
 * @brief    UDP protocol support:
 *               - @b @c 1 = is enabled (Default value).
 *               - @c 0 = is disabled.@n
 *           @n
 *           You can disable it to save some amount of code, if your
 *           application needs the TCP only. By default, it is enabled.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_UDP
    #define FNET_CFG_UDP                        (1)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_UDP_CHECKSUM
 * @brief    UDP checksum:
 *               - @b @c 1 = The UDP checksum will be generated for transmitted
 *                        datagrams and verified on received UDP datagrams (Default value).
 *               - @c 0 = The UDP checksum will not be generated for transmitted
 *                        datagrams and won't be verified on received UDP datagrams.
 *           @n@n
 *           You can disable it to speed the UDP applications up.
 *           By default, it is enabled.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_UDP_CHECKSUM
    #define FNET_CFG_UDP_CHECKSUM               (1)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_RAW
 * @brief    RAW socket support:
 *               - @c 1 = is enabled (Default value).
 *               - @b @c 0 = is disabled .@n
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_RAW
    #define FNET_CFG_RAW                        (1)
#endif


/*****************************************************************************
* 	TCP/IP stack parameters.
******************************************************************************/

/**************************************************************************/ /*!
 * @def      FNET_CFG_MULTITHREADING
 * @brief    Multi-threading support:
 *               - @c 1 = is enabled (experimental feature). User application must provide Mutex API during FNET initialization. @n
 *                 If the FNET User API may be called simultaneously from different RTOS threads, this parameter must be enabled to avoid race conditions.
 *                 This feature is under development. User application must implement Mutex API defined fnet_mutex_api_t.
 *               - @b @c 0 = is disabled (Default value).@n
 *                 This is default state for bare-metal applications or if the FNET User API is called simultaneously only from one RTOS thread.
 * @see fnet_mutex_api_t
 ******************************************************************************/
#ifndef FNET_CFG_MULTITHREADING
    #define FNET_CFG_MULTITHREADING             (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_SOCKET_MAX
 * @brief    Maximum number of sockets that can exist at the same time.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_SOCKET_MAX
    #define FNET_CFG_SOCKET_MAX                 (12U)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_SOCKET_BSD_NAMES
 * @brief    BSD Socket API names:
 *               - @c 1 = are supported  (Default value).
 *               - @b @c 0 = are not supported.@n
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_SOCKET_BSD_NAMES
    #define FNET_CFG_SOCKET_BSD_NAMES           (1)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_SOCKET_CALLBACK_ON_RX
 * @brief    "Socket Rx" event handler callback:
 *               - @c 1 = is supported.
 *               - @b @c 0 = is not supported (Default value).@n
 * This event occurs when the socket layer receives any packet for any existing socket.
 * It can be a packet with or without data-payload including ACK, FIN and SYN.@n
 * The user callback is registered by fnet_socket_set_callback_on_rx().
 * @see fnet_socket_set_callback_on_rx()
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_SOCKET_CALLBACK_ON_RX
    #define FNET_CFG_SOCKET_CALLBACK_ON_RX         (0U)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_SOCKET_TCP_MSS
 * @brief    The default value of the @ref TCP_MSS option
 *           (TCP Maximum Segment Size).
 *      The TCP Maximum Segment Size (MSS) defines the maximum amount
 *      of data that a host is willing to accept in a single TCP segment.@n
 *      This Maximum Segment Size (MSS) announcement is sent from the
 *      data receiver to the data sender and says "I can accept TCP segments
 *      up to size X". The size (X) may be larger or smaller than the
 *      default.@n
 *      The MSS counts only data octets in the segment, it does not count the
 *      TCP header or the IP header.@n
 *      This option can be set to:
 *          - @b @c 0 = This is the default value. The selection of the MSS is
 *              automatic and is based on the MTU of the outgoing
 *              interface minus 40 (does not include
 *              the 20 byte IP header and the 20 byte TCP header).@n
 *              It is done to assist in avoiding of IP fragmentation
 *              at the endpoints of the TCP connection.
 *          - Non-zero value (up to 64K) = The TCP segment could be as large as 64K
 *              (the maximum IP datagram size), but it could be fragmented
 *              at the IP layer in order to be transmitted
 *              across the network to the receiving host.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_SOCKET_TCP_MSS
    #define FNET_CFG_SOCKET_TCP_MSS             (0u)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_SOCKET_TCP_TX_BUF_SIZE
 * @brief    Default maximum size for the TCP send-socket buffer.
 *           At runtime, it can be changed by @ref fnet_socket_setopt()
 *           using the @ref SO_SNDBUF socket option.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_SOCKET_TCP_TX_BUF_SIZE
    #define FNET_CFG_SOCKET_TCP_TX_BUF_SIZE     (2U * 1024U)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_SOCKET_TCP_RX_BUF_SIZE
 * @brief    Default maximum size for the TCP receive-socket buffer.
 *           At runtime, it can be changed by @ref fnet_socket_setopt()
 *           using the @ref SO_RCVBUF socket option.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_SOCKET_TCP_RX_BUF_SIZE
    #define FNET_CFG_SOCKET_TCP_RX_BUF_SIZE     (2U * 1024U)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_SOCKET_UDP_TX_BUF_SIZE
 * @brief    Default maximum size for the UDP send-socket buffer.
 *           At runtime, it can be changed by @ref fnet_socket_setopt()
 *           using the @ref SO_SNDBUF socket option.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_SOCKET_UDP_TX_BUF_SIZE
    #define FNET_CFG_SOCKET_UDP_TX_BUF_SIZE     (2U * 1024U)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_SOCKET_UDP_RX_BUF_SIZE
 * @brief    Default maximum size for the UDP receive-socket buffer.
 *           At runtime, it can be changed by @ref fnet_socket_setopt()
 *           using the @ref SO_RCVBUF socket option.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_SOCKET_UDP_RX_BUF_SIZE
    #define FNET_CFG_SOCKET_UDP_RX_BUF_SIZE     (2U * 1024U)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_SOCKET_RAW_TX_BUF_SIZE
 * @brief    Default maximum size for the RAW send-socket buffer.
 *           At runtime, it can be changed by @ref fnet_socket_setopt()
 *           using the @ref SO_SNDBUF socket option.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_SOCKET_RAW_TX_BUF_SIZE
    #define FNET_CFG_SOCKET_RAW_TX_BUF_SIZE     (2U * 1024U)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_SOCKET_RAW_RX_BUF_SIZE
 * @brief    Default maximum size for the RAW receive-socket buffer.
 *           At runtime, it can be changed by @ref fnet_socket_setopt()
 *           using the @ref SO_RCVBUF socket option.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_SOCKET_RAW_RX_BUF_SIZE
    #define FNET_CFG_SOCKET_RAW_RX_BUF_SIZE     (2U * 1024U)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_IP_MAX_PACKET
 * @brief    Maximum size for the IPv4 and IPv6 datagram,
 *           the largest allowed value is 65535. @n
 *           Default value is 10 KB.
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_IP_MAX_PACKET
    #define FNET_CFG_IP_MAX_PACKET              (10U*1024U)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_TIMER_ALT
 * @brief    Alternative timer support:
 *               - @c 1 = is enabled. User application must implement own Timer API defined by fnet_timer_api_t and provide it during FNET initialization.
 *                 Also, user application must call fnet_timer_poll() periodically with 100 ms period or less. @n
 *                 It is mainly used by applications that want to use RTOS timer instead of FNET bare-metal one.
 *               - @b @c 0 = is disabled (Default value).@n
 *                 FNET uses own bare-metal timer and does automatic timer polling in interrupt.
 * @see fnet_timer_api_t, fnet_timer_poll()
 ******************************************************************************/
#ifndef FNET_CFG_TIMER_ALT
    #define FNET_CFG_TIMER_ALT                  (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_TIME
 * @brief    FNET time() implementation, defined by <time.h>:
 *               - @c 1 = is enabled.
 *               - @c 0 = is disabled.
 * @note Required by Azure IoT Hub client.
 *       Also it may be used by mbedTLS when MBEDTLS_HAVE_TIME is defined.
 * @see fnet_time(), fnet_time_set()
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_TIME
    #define FNET_CFG_TIME                       (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_ASSERT
 * @brief    FNET assert macro FNET_ASSERT():
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled (Default value).@n
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_ASSERT
    #define FNET_CFG_ASSERT                     (0U)
#endif

/*****************************************************************************
 * Function Overload
 *****************************************************************************/
#ifndef FNET_CFG_OVERLOAD_CHECKSUM_LOW
    #define FNET_CFG_OVERLOAD_CHECKSUM_LOW      (0)
#endif

/**************************************************************************/ /*!
 * @def      FNET_CFG_OVERLOAD_MEMCPY
 * @brief    Your own alternate implementation for fnet_memcpy().
 *               - @c 1 = is enabled.
 *               - @b @c 0 = is disabled (Default value).@n
 *           For example, to use standard memcpy() instead of FNET own implementation, add  to fnet_user_config.h:
            @verbatim
               #define FNET_CFG_OVERLOAD_MEMCPY    1
               #define fnet_memcpy    memcpy
            @endverbatim
 * @showinitializer
 ******************************************************************************/
#ifndef FNET_CFG_OVERLOAD_MEMCPY
    #define FNET_CFG_OVERLOAD_MEMCPY            (0)
#endif

/* IPv4 and/or IPv6 must enaqbled.*/
#if !FNET_CFG_IP4 && !FNET_CFG_IP6
    #error "Please enable IPv4 or/and IPv6, by FNET_CFG_IP4 or/and FNET_CFG_IP6."
#endif

/*****************************************************************************
 * DEBUGING INFO OUTPUT
 *****************************************************************************/

/**************************************************************************/ /*!
 * @internal
 * @brief    Debugging output:
 *               - @c 1 = is enabled.
 *               - @c 0 = is disabled.
 * @internal
 ******************************************************************************/
#ifndef FNET_CFG_DEBUG
    #define FNET_CFG_DEBUG              (0)
#endif

#ifndef FNET_CFG_DEBUG_AUTOIP
    #define FNET_CFG_DEBUG_AUTOIP       (0)
#endif

#ifndef FNET_CFG_DEBUG_TIMER  /* It will be printed to the UART '!' sign every second. */
    #define FNET_CFG_DEBUG_TIMER        (0)
#endif

#ifndef FNET_CFG_DEBUG_HTTP
    #define FNET_CFG_DEBUG_HTTP         (0)
#endif

#ifndef FNET_CFG_DEBUG_DHCP_CLN
    #define FNET_CFG_DEBUG_DHCP_CLN     (0)
#endif

#ifndef FNET_CFG_DEBUG_DHCP_SRV
    #define FNET_CFG_DEBUG_DHCP_SRV     (0)
#endif

#ifndef FNET_CFG_DEBUG_ARP
    #define FNET_CFG_DEBUG_ARP          (0)
#endif

#ifndef FNET_CFG_DEBUG_MEMPOOL
    #define FNET_CFG_DEBUG_MEMPOOL      (0)
#endif

#ifndef FNET_CFG_DEBUG_TFTP_CLN
    #define FNET_CFG_DEBUG_TFTP_CLN     (0)
#endif

#ifndef FNET_CFG_DEBUG_TFTP_SRV
    #define FNET_CFG_DEBUG_TFTP_SRV     (0)
#endif

#ifndef FNET_CFG_DEBUG_STACK
    #define FNET_CFG_DEBUG_STACK        (0)
#endif

#ifndef FNET_CFG_DEBUG_TELNET
    #define FNET_CFG_DEBUG_TELNET       (0)
#endif

#ifndef FNET_CFG_DEBUG_SHELL
    #define FNET_CFG_DEBUG_SHELL        (0)
#endif

#ifndef FNET_CFG_DEBUG_DNS
    #define FNET_CFG_DEBUG_DNS          (0)
#endif

#ifndef FNET_CFG_DEBUG_DNS
    #define FNET_CFG_DEBUG_DNS          (0)
#endif

#ifndef FNET_CFG_DEBUG_AZURE
    #define FNET_CFG_DEBUG_AZURE        (0)
#endif

#ifndef FNET_CFG_DEBUG_IP6
    #define FNET_CFG_DEBUG_IP6          (0)
#endif

#ifndef FNET_CFG_DEBUG_LINK
    #define FNET_CFG_DEBUG_LINK         (0)
#endif

#ifndef FNET_CFG_DEBUG_LLMNR
    #define FNET_CFG_DEBUG_LLMNR        (0)
#endif

#ifndef FNET_CFG_DEBUG_MDNS
    #define FNET_CFG_DEBUG_MDNS        (0)
#endif

#ifndef FNET_CFG_DEBUG_PING
    #define FNET_CFG_DEBUG_PING         (0)
#endif

#ifndef FNET_CFG_DEBUG_SNTP
    #define FNET_CFG_DEBUG_SNTP        (0)
#endif

#ifndef FNET_CFG_DEBUG_QCA
    #define FNET_CFG_DEBUG_QCA         (0)
#endif

#ifndef FNET_CFG_DEBUG_ENET
    #define FNET_CFG_DEBUG_ENET        (0)
#endif

#ifndef FNET_CFG_DEBUG_TRACE
    #define FNET_CFG_DEBUG_TRACE        (0)
#endif

#ifndef FNET_CFG_DEBUG_TRACE_IP4
    #define FNET_CFG_DEBUG_TRACE_IP4    (0)
#endif

#ifndef FNET_CFG_DEBUG_TRACE_ICMP4
    #define FNET_CFG_DEBUG_TRACE_ICMP4  (0)
#endif

#ifndef FNET_CFG_DEBUG_TRACE_IGMP
    #define FNET_CFG_DEBUG_TRACE_IGMP   (0)
#endif

#ifndef FNET_CFG_DEBUG_TRACE_ETH
    #define FNET_CFG_DEBUG_TRACE_ETH    (0)
#endif

#ifndef FNET_CFG_DEBUG_TRACE_ARP
    #define FNET_CFG_DEBUG_TRACE_ARP    (0)
#endif

#ifndef FNET_CFG_DEBUG_TRACE_UDP
    #define FNET_CFG_DEBUG_TRACE_UDP    (0)
#endif

#ifndef FNET_CFG_DEBUG_TRACE_TCP
    #define FNET_CFG_DEBUG_TRACE_TCP    (0)
#endif

/******************************************************************************
 * Obsolete configuration parameters
 ******************************************************************************/
#ifdef FNET_CFG_ETH_IP4_ADDR
    #error "FNET_CFG_ETH_IP4_ADDR parameter is obsolete. It is user application parameter now."
#endif
#ifdef FNET_CFG_ETH_IP4_MASK
    #error "FNET_CFG_ETH_IP4_MASK parameter is obsolete. It is user application parameter now."
#endif
#ifdef FNET_CFG_ETH_IP4_GW
    #error "FNET_CFG_ETH_IP4_GW parameter is obsolete. It is user application parameter now."
#endif
#ifdef FNET_CFG_ETH_IP4_DNS
    #error "FNET_CFG_ETH_IP4_DNS parameter is obsolete. It is user application parameter now."
#endif
#ifdef FNET_CFG_ETH_MAC_ADDR
    #error "FNET_CFG_ETH_MAC_ADDR parameter is obsolete. It is user application parameter now."
#endif
#ifdef FNET_CFG_ETH
    #error "FNET_CFG_ETH parameter is obsolete."
#endif
#ifdef FNET_CFG_ETH_MTU
    #error "FNET_CFG_ETH_MTU parameter is obsolete."
#endif
#ifdef FNET_CFG_ETH0_IP4_ADDR
    #error "FNET_CFG_ETH0_IP4_ADDR parameter is obsolete. It is user application parameter now."
#endif
#ifdef FNET_CFG_ETH0_IP4_MASK
    #error "FNET_CFG_ETH0_IP4_MASK parameter is obsolete. It is user application parameter now."
#endif
#ifdef FNET_CFG_ETH0_IP4_GW
    #error "FNET_CFG_ETH0_IP4_GW parameter is obsolete. It is user application parameter now."
#endif
#ifdef FNET_CFG_ETH0_IP4_DNS
    #error "FNET_CFG_ETH0_IP4_DNS parameter is obsolete. It is user application parameter now."
#endif
#ifdef FNET_CFG_ETH1_IP4_ADDR
    #error "FNET_CFG_ETH1_IP4_ADDR parameter is obsolete. It is user application parameter now."
#endif
#ifdef FNET_CFG_ETH1_IP4_MASK
    #error "FNET_CFG_ETH1_IP4_MASK parameter is obsolete. It is user application parameter now."
#endif
#ifdef FNET_CFG_ETH1_IP4_GW
    #error "FNET_CFG_ETH1_IP4_GW parameter is obsolete. It is user application parameter now."
#endif
#ifdef FNET_CFG_ETH1_IP4_DNS
    #error "FNET_CFG_ETH1_IP4_DNS parameter is obsolete. It is user application parameter now."
#endif
#ifdef FNET_CFG_DEFAULT_IF
    #error "FNET_CFG_DEFAULT_IF parameter is obsolete. It is user application parameter now."
#endif
#ifdef FNET_CFG_HEAP_SIZE
    #error "FNET_CFG_HEAP_SIZE parameter is obsolete. It is user application parameter now."
#endif

/*! @} */

#endif
