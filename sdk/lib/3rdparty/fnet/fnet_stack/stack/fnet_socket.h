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
*  Socket API.
*
***************************************************************************/

#ifndef _FNET_SOCKET_H_

#define _FNET_SOCKET_H_

#include "fnet_ip4.h"
#include "fnet_ip6.h"

/*! @addtogroup fnet_socket
* The Socket Application Program Interface (API) defines the way, in which the
* application program interacts with the TCP/IP stack. It has the
* BSD-like, non-blocking socket interface that makes porting of existing network applications
* to the FNET more convenient.@n
* @n
* The following table summarizes the supported socket API functions:
* <table>
* <caption>Socket functions</caption>
* <tr class="fnet_td_grey">
* <th ALIGN=CENTER>Category</th><th ALIGN=CENTER>FNET Routine</th><th ALIGN=CENTER>BSD Routine</th><th ALIGN=CENTER>Meaning</th><th ALIGN=CENTER>Server</th>
* <th ALIGN=CENTER>Client</th><th ALIGN=CENTER>@ref SOCK_STREAM</th> <th ALIGN=CENTER>@ref SOCK_DGRAM</th>
* </tr>
* <tr>
* <td>setup</td><td>@ref fnet_socket()</td><td>socket()</td><td>Creates a new unnamed socket within
* a specified communication domain family.</td><td>X</td><td>X</td><td>X</td>
* <td>X</td>
* </tr>
* <tr>
* <td>setup</td><td>@ref fnet_socket_bind()</td><td>bind()</td><td>Assigns a local address to
* a socket.</td><td>X</td><td>X</td><td>X</td><td>X</td>
* </tr>
* <tr>
* <td>server</td><td>@ref fnet_socket_listen()</td><td>listen()</td><td> Prepares a socket to accept
* the incoming connections.</td><td>X</td><td>@n</td><td>X</td><td>@n</td>
* </tr>
* <tr>
* <td>server</td><td>@ref fnet_socket_accept()</td><td>accept()</td><td>Accepts the connections.</td>
* <td>X</td><td>@n</td><td>X</td><td>@n</td>
* </tr>
* <tr>
* <td>client</td><td>@ref fnet_socket_connect()</td><td>connect()</td><td> Establishes a connection to
* a foreign socket.</td><td>@n</td><td>X</td><td>X</td><td>X</td>
* </tr>
* <tr>
* <td>input</td><td>@ref fnet_socket_recv()</td><td>recv()</td><td>Receives the data.</td><td>X</td>
* <td>X</td><td>X</td><td>X</td>
* </tr>
* <tr>
* <td>input</td><td>@ref fnet_socket_recvfrom()</td><td>recvfrom()</td><td>Receives the data and address of
* the sender.</td><td>X</td><td>X</td><td>@n</td><td>X</td>
* </tr>
* <tr>
* <td>output</td><td>@ref fnet_socket_send()</td><td>send()</td><td>Sends the data.</td><td>X</td>
* <td>X</td><td>X</td><td>X</td>
* </tr>
* <tr>
* <td>output</td><td>@ref fnet_socket_sendto()</td><td>sendto()</td><td>Sends the data to a specified
* address.</td><td>X</td><td>X</td><td>@n</td><td>X</td>
* </tr>
* <tr>
* <td>termination</td><td>@ref fnet_socket_shutdown()</td><td>shutdown()</td><td>Terminates a connection
* in one or both directions.</td><td>X</td><td>X</td><td>X</td><td>X</td>
* </tr>
* <tr>
* <td>termination</td><td>@ref fnet_socket_close()</td><td>closesocket()</td><td>Terminates a connection
* and releases the socket.</td><td>X</td><td>X</td><td>X</td><td>X</td>
* </tr>
* <tr>
* <td>administration</td><td>@ref fnet_socket_setopt()</td><td>fnet_socket_setopt()</td><td>Sets socket or protocol
* options.</td><td>X</td><td>X</td><td>X</td><td>X</td>
* </tr>
* <tr>
* <td>administration</td><td>@ref fnet_socket_getopt()</td><td>getsockopt()</td><td>Gets socket or protocol
* options.</td><td>X</td><td>X</td><td>X</td><td>X</td>
* </tr>
* <tr>
* <td>administration</td><td>@ref fnet_socket_getname()</td><td>getsockname()</td><td>Gets a local address
* assigned to a socket.</td><td>X</td><td>X</td><td>X</td><td>X</td>
* </tr>
* <tr>
* <td>administration</td><td>@ref fnet_socket_getpeername()</td><td>getpeername()</td><td>Gets a foreign address
* assigned to a socket.</td><td>X</td><td>X</td><td>X</td><td>X</td>
* </tr>
* </table>
* Configuration parameters:
* - @ref FNET_CFG_SOCKET_MAX
* - @ref FNET_CFG_SOCKET_TCP_TX_BUF_SIZE
* - @ref FNET_CFG_SOCKET_TCP_RX_BUF_SIZE
* - @ref FNET_CFG_SOCKET_UDP_TX_BUF_SIZE
* - @ref FNET_CFG_SOCKET_UDP_RX_BUF_SIZE
* - @ref FNET_CFG_SOCKET_TCP_MSS
* - @ref FNET_CFG_RAW
* - @ref FNET_CFG_SOCKET_BSD_NAMES
* - @ref FNET_CFG_SOCKET_CALLBACK_ON_RX
*/
/*! @{ */

/* Special addresses */

/**************************************************************************/ /*!
 * @brief It means to use any network interface.
 ******************************************************************************/
#define INADDR_ANY       (fnet_ip4_addr_t)(0x00000000U)

/**************************************************************************/ /*!
 * @brief  Broadcast address. @n
 * It can be used to send the broadcast UDP datagrams over an IP network.
 ******************************************************************************/
#define INADDR_BROADCAST (fnet_ip4_addr_t)(0xffffffffU)

/**************************************************************************/ /*!
 * @brief Address family type.
 * @see AF_UNSPEC, AF_INET, AF_INET6, AF_SUPPORTED
 ******************************************************************************/
typedef fnet_uint16_t fnet_address_family_t;

/**************************************************************************/ /*!
 * @brief cope zone index type, defining network interface.
 ******************************************************************************/
typedef fnet_uint32_t   fnet_scope_id_t;

/**************************************************************************/ /*!
 * @brief Unspecified address family.
 ******************************************************************************/
#define AF_UNSPEC   (0U)
/**************************************************************************/ /*!
 * @brief IPv4 address family.
 ******************************************************************************/
#define AF_INET     (1U)
/**************************************************************************/ /*!
 * @brief IPv6 address family.
 ******************************************************************************/
#define AF_INET6    (2U)
/**************************************************************************/ /*!
 * @brief Bitmask of supported address families.
 * @showinitializer
 ******************************************************************************/
#define AF_SUPPORTED ((fnet_address_family_t)((fnet_address_family_t)(AF_INET6*(fnet_address_family_t)FNET_CFG_IP6) | (fnet_address_family_t)(AF_INET*(fnet_address_family_t)FNET_CFG_IP4)))

/**************************************************************************/ /*!
 * @brief Size of sa_data[] field of @ref fnet_sockaddr structure. @n
 * It used to cover fnet_sockaddr_in and fnet_sockaddr_in6.
 * @showinitializer
 ******************************************************************************/
#if FNET_CFG_IP6
    #define FNET_SA_DATA_SIZE       (sizeof(struct fnet_in6_addr))
#else /* IPv4 */
    #define FNET_SA_DATA_SIZE       (sizeof(struct fnet_in_addr))
#endif

/**************************************************************************/ /*!
 * @def     FNET_IP_ADDR_STR_SIZE
 * @brief   Size of the string buffer that will contain
 *          null-terminated ASCII string of an IP address. It depends on
 *          if disabled or enabled IPv6.
 * @see fnet_inet_ntoa, fnet_inet_ntop
 * @showinitializer
 ******************************************************************************/
#if FNET_CFG_IP6
    #define FNET_IP_ADDR_STR_SIZE       FNET_IP6_ADDR_STR_SIZE
#else /* IPv4 */
    #define FNET_IP_ADDR_STR_SIZE       FNET_IP4_ADDR_STR_SIZE
#endif

#define FNET_IP_ADDR_STR_SIZE_MAX       FNET_IP6_ADDR_STR_SIZE

/**************************************************************************/ /*!
 * @brief IPv4 address structure.
 *
 * @see fnet_sockaddr_in
 *
 * Actually, it represents an IPv4 address (in network endian).
 ******************************************************************************/
struct fnet_in_addr
{
    fnet_ip4_addr_t s_addr; /**< @brief 32-bit IPv4 address (in network byte order).*/
};

/**************************************************************************/ /*!
 * @brief IPv4 Socket address structure.
 *
 * @see fnet_sockaddr
 *
 * To make manipulation of the @ref fnet_sockaddr structure easier, the TCP/IPv4 stack
 * also defines the equivalent structure @ref fnet_sockaddr_in
 * ("in" means "Internet").
 ******************************************************************************/
struct fnet_sockaddr_in
{
    fnet_address_family_t   sin_family;     /**< @brief Specifies the address family. @n
                                            * It must ne set to @ref AF_INET.*/
    fnet_uint16_t           sin_port;       /**< @brief 16-bit port number used to
                                            * demultiplex the transport-level messages
                                            * (in network byte order).*/
    fnet_scope_id_t         sin_scope_id;   /**< @brief Scope zone index, defining network interface.*/
    struct fnet_in_addr     sin_addr;       /**< @brief 32-bit internet address.*/
};


/************************************************************************
*                           IPv6
*************************************************************************/


/**************************************************************************/ /*!
 * @brief IPv6 address structure.
 *
 * @see fnet_sockaddr_in6
 *
 * Actually, it represents an IPv6 address.
 ******************************************************************************/
struct fnet_in6_addr
{
    fnet_ip6_addr_t s6_addr;        /**< @brief 128-bit IPv6 address.*/
};

/**************************************************************************/ /*!
 * @brief IPv6 Socket address structure.
 *
 * @see fnet_sockaddr
 *
 * To make manipulation of the @ref fnet_sockaddr structure easier, the TCP/IPv6 stack
 * also defines the equivalent structure @ref fnet_sockaddr_in6
 * ("in" means "Internet").
 ******************************************************************************/
struct fnet_sockaddr_in6
{
    fnet_address_family_t   sin6_family;    /**< @brief Specifies the address family. @n
                                            * It must ne set to @ref AF_INET6. */
    fnet_uint16_t           sin6_port;      /**< @brief 16-bit port number used to
                                            * demultiplex the transport-level messages
                                            * (in network byte order).*/
    fnet_scope_id_t         sin6_scope_id;  /**< @brief Scope zone index, defining network interface.*/
    struct fnet_in6_addr    sin6_addr;      /**< @brief 128-bit IPv6 internet address.*/
};

/**************************************************************************/ /*!
 * @brief Socket address structure.
 *
 * @see fnet_sockaddr_in, fnet_sockaddr_in6
 *
 * The original goal of the @ref fnet_sockaddr structure is to support multiple
 * protocols. For the TCP/IP stack @c sa_data, it contains a destination address
 * and port number for a socket.
 ******************************************************************************/
struct fnet_sockaddr
{
    fnet_address_family_t   sa_family;      /**< @brief Address family. Specifies the
                                            * address family, to which the address belongs. @n
                                            * It is defined by @ref fnet_address_family_t.*/
    fnet_uint16_t           sa_port;        /**< @brief 16-bit port number used to
                                            * demultiplex the transport-level messages
                                            * (in network byte order).*/
    fnet_scope_id_t         sa_scope_id;    /**< @brief Scope zone index, defining network interface.*/
    fnet_uint8_t            sa_data[FNET_SA_DATA_SIZE];/**< @brief Address value. For the TCP/IP stack,
                                            * it contains the destination address and port
                                            * number for a socket.*/
};

/**************************************************************************/ /*!
 * @brief IPv4 multicast group information.
 *
 * The structure is used with the @ref IP_ADD_MEMBERSHIP and
 * @ref IP_DROP_MEMBERSHIP socket options.
 *
 * @see IP_ADD_MEMBERSHIP, IP_DROP_MEMBERSHIP
 *
 ******************************************************************************/
struct fnet_ip_mreq
{
    struct fnet_in_addr imr_multiaddr;      /**< @brief IPv4 multicast address of group. */
    fnet_scope_id_t     imr_interface;      /**< @brief Interface index. It equals to the scope zone index, defining network interface.@n
                                            * If this member is zero, the default interface is used. */
};

/**************************************************************************/ /*!
 * @brief IPv6 multicast group information.
 *
 * The structure is used with the @ref IPV6_JOIN_GROUP and
 * @ref IPV6_LEAVE_GROUP socket options.
 *
 * @see IPV6_JOIN_GROUP, IPV6_LEAVE_GROUP
 *
 ******************************************************************************/
struct fnet_ipv6_mreq
{
    struct fnet_in6_addr    ipv6imr_multiaddr;  /**< @brief IPv6 multicast address of group. */
    fnet_scope_id_t         ipv6imr_interface;  /**< @brief Interface index. It equals to the scope zone index, defining network interface.@n
                                                * If this member is zero, the default interface is used. */
};

/**************************************************************************/ /*!
 * @brief Socket types.
 ******************************************************************************/
typedef enum
{
    SOCK_UNSPEC = (0U),  /**< @brief Unspecified socket type.
                         */
    SOCK_STREAM = (1U),  /**< @brief Stream socket.@n
                         * Provides reliable, two-way, connection-based
                         * byte stream. It corresponds to the TCP protocol
                         */
    SOCK_DGRAM  = (2U),  /**< @brief Datagram socket.@n
                         * Provides unreliable, connectionless datagrams.
                         * It corresponds to the UDP protocol.
                         */
    SOCK_RAW    = (3U)   /**< @brief Raw socket.@n
                         * Raw sockets allow an application to have direct access to
                         * lower-level communication protocols.
                         * Raw sockets are intended to take advantage of some protocol feature
                         * that is not directly accessible through a normal interface,
                         * or to build new protocols on top of existing low-level protocols.@n
                         * It can be enabled by the @ref FNET_CFG_RAW option.
                         */
} fnet_socket_type_t;

/**************************************************************************/ /*!
 * @brief Socket state.
 ******************************************************************************/
typedef enum
{
    SS_CLOSED = (0),    /**< @brief Not connected to any socket.*/
    SS_CLOSING,         /**< @brief In process of closing connection. For TCP, after FIN receive.*/
    SS_CONNECTING,      /**< @brief In process of connecting.*/
    SS_CONNECTED,       /**< @brief Connected to a socket.*/
    SS_LISTENING        /**< @brief In listening state.*/
} fnet_socket_state_t;

/**************************************************************************/ /*!
 * @brief Protocol numbers and Level numbers for the @ref fnet_socket_setopt()
 * and the @ref fnet_socket_getopt().
 ******************************************************************************/
typedef enum
{
    IPPROTO_IP      = (0), /**< @brief IPv4 options level number
                            *   for @ref fnet_socket_getopt() and @ref fnet_socket_setopt().*/
    IPPROTO_ICMP    = (1), /**< @brief ICMPv4 protocol number.*/
    IPPROTO_IGMP    = (2), /**< @brief IGMP protocol number.*/
    IPPROTO_TCP     = (6), /**< @brief TCP protocol number; TCP options level number
                            *   for @ref fnet_socket_getopt() and @ref fnet_socket_setopt().*/
    IPPROTO_UDP     = (17),/**< @brief UDP protocol number.*/
    IPPROTO_IPV6    = (41), /**< @brief IPv6 options level number
                            *    for @ref fnet_socket_getopt() and @ref fnet_socket_setopt().*/
    IPPROTO_ICMPV6  = (58),/**< @brief ICMPv6 protocol number.*/
    SOL_SOCKET      = (255255)  /**< @brief Socket options level number for
                                 * @ref fnet_socket_getopt() and @ref fnet_socket_setopt().*/
} fnet_protocol_t;

/**************************************************************************/ /*!
 * @brief Socket options for the @ref fnet_socket_setopt() and the @ref fnet_socket_getopt().
 *
 * <table>
 *<caption>Socket SOL_SOCKET, IPPROTO_TCP, IPPROTO_IP and IPPROTO_IPV6 level options</caption>
 *<tr class="fnet_td_grey">
 *<th ALIGN=CENTER>Option</th><th ALIGN=CENTER>Option Type</th><th ALIGN=CENTER>
 * Default Value</th><th ALIGN=CENTER>Read/Write</th>
 *</tr>
 *<tr>
 *<td>@ref SO_ACCEPTCONN</td><td>fnet_uint32_t</td><td>0</td><td>R</td>
 *</tr>
 *<tr>
 *<td>@ref SO_KEEPALIVE</td><td>fnet_uint32_t</td><td>1</td><td>RW</td>
 *</tr>
 *<tr>
 *<td>@ref SO_DONTROUTE</td><td>fnet_uint32_t</td><td>0</td><td>RW</td>
 *</tr>
 *<tr>
 *<td>@ref SO_LINGER</td><td>struct @ref fnet_linger</td><td>{0,0}</td><td>RW</td>
 *</tr>
 *<tr>
 *<td>@ref SO_LISTENQLIMIT</td><td>fnet_uint32_t</td><td>0</td><td>R</td>
 *</tr>
 *<tr>
 *<td>@ref SO_LISTENQLEN</td><td>fnet_uint32_t</td><td>0</td><td>R</td>
 *</tr>
 *<tr>
 *<td>@ref SO_OOBINLINE</td><td>fnet_uint32_t</td><td>0</td><td>RW</td>
 *</tr>
 *<tr>
 *<td>@ref SO_SNDBUF</td><td>fnet_uint32_t</td>
 *<td>@ref FNET_CFG_SOCKET_UDP_TX_BUF_SIZE for UDP @n @ref FNET_CFG_SOCKET_TCP_TX_BUF_SIZE for TCP</td><td>RW</td>
 *</tr>
 *<tr>
 *<td>@ref SO_RCVBUF</td><td>fnet_uint32_t</td>
 *<td>@ref FNET_CFG_SOCKET_UDP_RX_BUF_SIZE for UDP @n @ref FNET_CFG_SOCKET_TCP_RX_BUF_SIZE for TCP</td><td>RW</td>
 *</tr>
 *<tr>
 *<td>@ref SO_STATE</td><td>@ref fnet_socket_state_t</td><td>@ref SS_CLOSED</td><td>R</td>
 *</tr>
 *<tr>
 *<td>@ref SO_ERROR</td><td>@ref fnet_error_t</td><td>@ref FNET_ERR_OK</td><td>R</td>
 *</tr>
 *<tr>
 *<td>@ref SO_TYPE</td><td>@ref fnet_socket_type_t</td><td>@ref SOCK_DGRAM for
 * UDP @n @ref SOCK_STREAM for TCP</td><td>R</td>
 *</tr>
 *<tr>
 *<td>@ref SO_RCVNUM</td><td>fnet_uint32_t</td><td>0</td><td>R</td>
 *</tr>
 *<tr>
 *<td>@ref SO_SNDNUM</td><td>fnet_uint32_t</td><td>0</td><td>R</td>
 *</tr>
*<tr>
 *<td>@ref TCP_MSS</td><td>fnet_uint32_t</td><td>536</td><td>RW</td>
 *</tr>
 *<tr>
 *<td>@ref TCP_BSD</td><td>fnet_uint32_t</td><td>1</td><td>RW</td>
 *</tr>
 *<tr>
 *<td>@ref TCP_NODELAY</td><td>fnet_uint32_t</td><td>1</td><td>RW</td>
 *</tr>
 *<tr>
 *<td>@ref TCP_FINRCVD</td><td>fnet_uint32_t</td><td>0</td><td>R</td>
 *</tr>
 *<tr>
 *<td>@ref TCP_URGRCVD</td><td>fnet_uint32_t</td><td>0</td><td>R</td>
 *</tr>
 *<tr>
 *<td>@ref TCP_KEEPIDLE</td><td>fnet_uint32_t</td><td>7200</td><td>RW</td>
 *</tr>
 *<tr>
 *<td>@ref TCP_KEEPINTVL</td><td>fnet_uint32_t</td><td>75</td><td>RW</td>
 *</tr>
 *<tr>
 *<td>@ref TCP_KEEPCNT</td><td>fnet_uint32_t</td><td>8</td><td>RW</td>
 *</tr>
 *<tr>
 *<td>@ref IP_TOS</td><td>fnet_uint32_t</td><td>0</td><td>RW</td>
 *</tr>
 *<tr>
 *<td>@ref IP_TTL</td><td>fnet_uint32_t</td><td>64</td><td>RW</td>
 *</tr>
 *<tr>
 *<td>@ref IP_MULTICAST_TTL</td><td>fnet_uint32_t</td><td>1</td><td>RW</td>
 *</tr>
 *<tr>
 *<td>@ref IP_ADD_MEMBERSHIP</td><td>@ref fnet_ip_mreq</td><td>N/A</td><td>W</td>
 *</tr>
 *<tr>
 *<td>@ref IP_DROP_MEMBERSHIP</td><td>@ref fnet_ip_mreq</td><td>N/A</td><td>W</td>
 *</tr>
 *<tr>
 *<td>@ref IPV6_UNICAST_HOPS</td><td>fnet_uint32_t</td><td>0 (64)</td><td>RW</td>
 *</tr>
 *<tr>
 *<td>@ref IPV6_MULTICAST_HOPS</td><td>fnet_uint32_t</td><td>1</td><td>RW</td>
 *</tr>
 *<tr>
 *<td>@ref IPV6_JOIN_GROUP</td><td>@ref fnet_ipv6_mreq</td><td>N/A</td><td>W</td>
 *</tr>
 *<tr>
 *<td>@ref IPV6_LEAVE_GROUP</td><td>@ref fnet_ipv6_mreq</td><td>N/A</td><td>W</td>
 *</tr>
 *</table>
 ******************************************************************************/
typedef enum
{
    SO_ACCEPTCONN,              /**< @brief Returns @c 1 if a socket is in
                                *   listening mode and returns @c 0 when vice versa.
                                *   This is the read-only option and it is
                                *   valid only for connection-oriented protocols (TCP).*/
    SO_KEEPALIVE,               /**< @brief This option enables keep-alive probes
                                *   for a socket connection. These probes are used
                                *   to maintain a TCP connection and regularly
                                *   test the connection to ensure that it's
                                *   still available. It is only valid for
                                *   connection-oriented protocols (TCP).*/
    SO_DONTROUTE,               /**< @brief This option enables bypassing of a routing algorithm.
                                *   It means that the network interface tries to send a datagram without
                                *   a gateway.*/
    SO_LINGER,                  /**< @brief This option controls the action
                                *   taken when unsent data is present, and
                                *   @ref fnet_socket_close() is called. This option
                                *   is defined by the @ref fnet_linger structure.*/
    SO_OOBINLINE,               /**< @brief This option specifies that out-of-band (OOB)
                                *   data will be received in line with regular data.
                                *   It is valid only for the TCP protocol. @n
                                *   This option is avalable only if
                                *   @ref FNET_CFG_TCP_URGENT is set to @c 1.*/
    SO_SNDBUF,                  /**< @brief This option defines the maximum per-socket
                                *   buffer size for output data.*/
    SO_RCVBUF,                  /**< @brief This option defines the maximum per-socket
                                *   buffer size for input data.*/
    SO_STATE,                   /**< @brief This option defines the current state of the socket.@n
                                *   This is the read-only option and it is defined by the @ref fnet_socket_state_t type.*/
    SO_ERROR,                   /**< @brief This option returns a per-socket-based error code.@n
                                *   The error code is defined by the @ref fnet_error_t type.@n
                                *   After the error value has been read in the @ref fnet_socket_getopt function, it is cleared.
                                *   But a successful call using the socket routines does not clear
                                *   the socket-based error.*/
    SO_TYPE,                    /**< @brief This option defines the type of the socket.
                                *   This is a read-only option and it is defined by the @ref fnet_socket_type_t type.*/
    SO_RCVNUM,                  /**< @brief This option is used to determine the amount of data
                                *   pending in the socket-input buffer.@n
                                *   This is a read-only option.*/
    SO_SNDNUM,                  /**< @brief This option is used to determine the amount of data
                                *   in the socket output buffer. @n
                                *   This is a read-only option.*/
    SO_LISTENQLIMIT,            /**< @brief Backlog limit of the socket or the maximal number of queued connections, which is set by @ref fnet_socket_listen(). @n
                                *   This is a read-only option.*/
    SO_LISTENQLEN,              /**< @brief Complete queue length of the socket. If it has the nonzeo value, unaccepted complete connections are waiting for fnet_socket_accept(). @n
                                *   This is a read-only option.*/
    /* TCP level (IPPROTO_TCP) options */

    TCP_MSS,                /**< @brief This option defines the maximum size of
                             *   the input segments (MSS). @n
                             * The TCP Maximum Segment Size (MSS) defines the maximum amount
                             * of data that a host is willing to accept in a single TCP segment.@n
                             * This Maximum Segment Size (MSS) announcement is sent from the
                             * data receiver to the data sender and says "I can accept TCP segments
                             * up to size X". The size (X) may be larger or smaller than the
                             * default.@n
                             * The MSS counts only data octets in the segment, it does not count the
                             * TCP header or the IP header.@n
                             * The default value is defined by the @ref FNET_CFG_SOCKET_TCP_MSS
                             * user-configuration option. @n
                             * This option can be set to:
                             *     - @c 0 = The selection of the MSS is
                             *              automatic and is based on the MTU of the outgoing
                             *              interface minus 40 (does not include
                             *              the 20 byte IP header and the 20 byte TCP header).@n
                             *              It is done to assist in avoiding of IP fragmentation
                             *              at the endpoints of the TCP connection.
                             *     - Non-zero value (up to 64K) = The TCP segment could be as large as 64K
                             *              (the maximum IP datagram size), but it could be fragmented
                             *              at the IP layer in order to be transmitted
                             *              across the network to the receiving host.*/
    TCP_BSD,                /**< @brief If this option is set to @c 1, the BSD interpretation of
                             *   the urgent pointer is used. In this case the
                             *   urgent pointer of the TCP segment points to
                             *   the next byte following after the urgent byte.
                             *   Most of the TCP implementations use this
                             *   interpretation by default.@n
                             *   If this option is set to @c 0, the interpretation of
                             *   the TCP specification is used. In this case the
                             *   urgent pointer of the TCP segment points
                             *   to the urgent byte. @n
                             *   This option is avalable only if
                             *   @ref FNET_CFG_TCP_URGENT is set to @c 1.*/
    TCP_NODELAY,            /**< @brief If this option is set to @c 1, the Nagle algorithm
                             *   is disabled (and vice versa). @n
                             *   The Nagle algorithm is effective in reducing the number
                             *   of small packets sent by the host by essentially buffering
                             *   send data, if there is unacknowledged data already "in flight",
                             *   or until a full-size packet can be sent.@n
                             *   But for some applications this algorithm can impede
                             *   performance, especially for a bulky data transfer.*/
    TCP_FINRCVD,            /**< @brief This option is set when the final (FIN) segment arrives. @n
                             *   This option indicates that another side will not send any data
                             *   in the current connection.@n
                             *   This is the read-only option.*/
    TCP_URGRCVD,            /**< @brief This option is set when the urgent byte arrives, and
                             *   reset when this byte is read.@n
                             *   This option can be set only if the @ref SO_OOBINLINE option is set to @c 0.@n
                             *   This is the read-only option. @n
                             *   This option is avalable only if
                             *   @ref FNET_CFG_TCP_URGENT is set to @c 1.*/
    TCP_KEEPIDLE,           /**< @brief When the @ref SO_KEEPALIVE option is enabled, TCP probes a connection that
                             *   has been idle for some amount of time.  The default value for this idle
                             *   period is @c 7200 seconds (2 hours).  The @ref TCP_KEEPIDLE option can be used to affect this
                             *   value for a given socket, and specifies the number of seconds of idle
                             *   time between keepalive probes.*/
    TCP_KEEPINTVL,          /**< @brief When the @ref SO_KEEPALIVE option is enabled, TCP probes a connection that
                             *   has been idle for some amount of time.  If the remote system does not
                             *   respond to a keepalive probe, TCP retransmits the probe after some
                             *   amount of time.  The default value for this retransmit interval is @c 75
                             *   seconds. The @ref TCP_KEEPINTVL option can be used to affect this value for
                             *   a given socket, and specifies the number of seconds to wait before
                             *   retransmitting a keepalive probe.*/
    TCP_KEEPCNT,            /**< @brief When the @ref SO_KEEPALIVE option is enabled, TCP probes a connection that
                             *   has been idle for some amount of time.  If the remote system does not
                             *   respond to a keepalive probe, TCP retransmits the probe a certain
                             *   number of times before a connection is considered to be broken.  The
                             *   default value for this keepalive probe retransmit limit is @c 8.  The
                             *   @ref TCP_KEEPCNT option can be used to affect this value for a given socket,
                             *   and specifies the maximum number of keepalive probes to be sent.*/

    /* IPv4 level (IPPROTO_IP) options */

    IP_TOS,                 /**< @brief  This option defines the IPv4 TOS
                            *   (type-of-service) field for outgoing datagrams.*/
    IP_TTL,                 /**< @brief  This option defines the IPv4 TTL
                            *   (time-to-live) vlaue for outgoing datagrams.*/
    IP_MULTICAST_TTL,       /**< @brief This option allows to change IPv4 "time to live" (TTL)
                            * value for outgoing multicast datagrams.
                            * Otherwise, multicast datagrams are sent with a default value of 1,
                            * to prevent them to be forwarded beyond the local network.@n
                            * This option is available only if @ref FNET_CFG_MULTICAST is set to @c 1.*/
    IP_ADD_MEMBERSHIP,      /**< @brief  Join the socket to the IPv4 multicast group on
                            * the specified interface. It tells the system to receive packets on
                            * the network whose destination is the group address (but not its own).
                            * It is valid only for the SOCK_DGRAM (UDP) sockets.@n
                            * This option is available only if @ref FNET_CFG_MULTICAST is set to @c 1.*/
    IP_DROP_MEMBERSHIP,     /**< @brief  Drops membership to a IPv4 multicast group and interface.@n
                            * This option is available only if @ref FNET_CFG_MULTICAST is set to @c 1.*/
    /* IPv6 level (IPPROTO_IPV6) options */

    IPV6_UNICAST_HOPS,      /**< @brief  This option defines hop limit used
                            * for outgoing unicast IPv6 packets. @n
                            * Its value can be from 0 till 255.@n
                            * By default the option is set to 0. It means that the hop limit is defined
                            * by local IPv6 router, otherwise it equals to 64.*/
    IPV6_MULTICAST_HOPS,    /**< @brief Set the hop limit to use for outgoing multicast IPv6 packets.@n
                            * If IPV6_MULTICAST_HOPS is not set, the default is 1.*/
    IPV6_JOIN_GROUP,        /**< @brief (RFC3493) Join a multicast group on a specified local interface.@n
                            *  It is valid only for the SOCK_DGRAM (UDP) sockets.*/
    IPV6_LEAVE_GROUP        /**< @brief (RFC3493)  Leave a multicast group on a specified interface.@n
                            *  It is valid only for the SOCK_DGRAM (UDP) sockets.*/
} fnet_socket_options_t;

/**************************************************************************/ /*!
 * @brief This structure is used for the @ref SO_LINGER option.
 ******************************************************************************/
struct fnet_linger
{
    fnet_bool_t    l_onoff;  /**< @brief Determines, whether the option will be
                              *   turned on @c FNET_TRUE, or off @c FNET_FALSE.
                              */
    fnet_uint16_t  l_linger; /**< @brief Specifies the amount of time (in seconds)
                              *   to wait when the connection is closed and unsent data
                              *   is discarded.
                              *   If @c l_onoff is @c 0, the @c l_linger value is ignored
                              *   and the stack continues to try to send the data as usually.
                              */
};

/**************************************************************************/ /*!
 * @brief Socket descriptor.
 ******************************************************************************/
typedef void *fnet_socket_t;

/**************************************************************************/ /*!
 * @brief The flags parameters for receiving and sending functions
 * @ref fnet_socket_recv(), @ref fnet_socket_recvfrom(), @ref fnet_socket_send(), and @ref fnet_socket_sendto().
 *
 * The optional flag specifies the way, in which the call is made.
 * It can be constructed by using the bitwise OR.
 ******************************************************************************/
typedef enum
{
    MSG_OOB       = (0x1U),  /**< @brief Process out-of-band
                             * data instead of regular data. @n
                             *   This option is avalable only if
                             *   @ref FNET_CFG_TCP_URGENT is set to @c 1.
                             */
    MSG_PEEK      = (0x2U),  /**< @brief Receive a copy of the
                             * data without consuming it.
                             */
    MSG_DONTROUTE = (0x4U)   /**< @brief Send without using
                             * routing tables.
                             */
} fnet_msg_flags_t;

/**************************************************************************/ /*!
 * @brief The flags used by @ref fnet_socket_shutdown().
 *
 * They describe what types of socket operation will no longer be allowed.
 ******************************************************************************/
typedef enum
{
    SD_READ  = (0x1U), /**< @brief Data receiving is disabled.
                       */
    SD_WRITE = (0x2U), /**< @brief Data sending is disabled.
                       */
    SD_BOTH  = (SD_READ | SD_WRITE) /**< @brief Both receiving and
                                     * sending are disabled.
                                     */
} fnet_sd_flags_t;

/**************************************************************************/ /*!
* @brief Socket event type, used by @ref fnet_socket_poll_t.
* @see fnet_socket_poll()
******************************************************************************/
typedef enum
{
    FNET_SOCKET_EVENT_NONE = 0,    /**< @brief There is no registered event. */
    FNET_SOCKET_EVENT_IN =  1 << 0, /**< @brief There is data for reading or a new pending connection for accepting. */
    FNET_SOCKET_EVENT_OUT = 1 << 1,  /**< @brief There is a free buffer space and the socket may accept data for writing.
                                        Also, it may be used as the mark that the TCP socket is connected. */
    FNET_SOCKET_EVENT_ERR = 1 << 2,  /**< @brief There is a socket error. It may happen when a connect attempt is failed, or connection is closed by remote peer
                                        or any other socket error (to determine the error value application may examine the socket @ref SO_ERROR option).*/
    FNET_SOCKET_EVENT_ALL = FNET_SOCKET_EVENT_IN | FNET_SOCKET_EVENT_OUT | FNET_SOCKET_EVENT_ERR /**< @brief Bitmask of all event types.*/
} fnet_socket_event_t;

/**************************************************************************/ /*!
 * @brief Socket poll structure used by @ref fnet_socket_poll().
 ******************************************************************************/
typedef struct
{
    fnet_socket_t          s;                      /**< @brief Socket descriptor. If its value is zero, it is ignored.*/
    fnet_socket_event_t    events;                 /**< @brief A bit mask specifying the events the application is interested in.*/
    fnet_socket_event_t    events_occurred;        /**< @brief A bit mask specifying the events that actually occurred.
                                                             It is filled by by the fnet_socket_poll() and can include any event specified in the @c events field. */
} fnet_socket_poll_t;


#if defined(__cplusplus)
extern "C" {
#endif

/***************************************************************************/ /*!
 *
 * @brief    Creates a socket.
 *
 *
 * @param family   Address family that the socket will use, defined
 *                 by the @ref fnet_address_family_t.
 *
 * @param type     Type specification for the new socket, defined by @ref
 *                 fnet_socket_type_t. It can be @ref SOCK_STREAM (TCP) or
 *                 @ref SOCK_DGRAM (UDP).
 *
 * @param protocol Protocol to be used with the socket that is specific to
 *                 the indicated address family. This stack supports
 *                 @ref IPPROTO_TCP and @ref IPPROTO_UDP.@n
 *                 This parameter is optional, and can be set to zero, as
 *                 the @c type already defines the proper protocol.
 *
 * @return This function returns:
 *   - Socket descriptor referencing the new socket, if no error occurs.
 *   - @ref FNET_NULL if an error occurs. @n
 *     The specific error code can be retrieved using the @ref fnet_error_get().
 *
 * @see fnet_socket_close()
 *
 ******************************************************************************
 *
 * This function creates a socket and returns its descriptor.@n
 * The @ref fnet_socket() function causes a socket descriptor and any related
 * resources to be allocated and bound to a specific transport-service
 * provider that supports the requested combination of address family,
 * socket type, and protocol parameters.@n
 * @n
 * After a socket is created:
 * - Connection-oriented sockets, such as the @ref SOCK_STREAM, provide
 *   full-duplex connections. Before any data can be sent or received,
 *   it must be in a connected state . A connection to
 *   another socket is established with the @ref fnet_socket_connect() call.
 *   Once connected, the data can be transferred using the @ref fnet_socket_send() and  the @ref
 *   fnet_socket_recv() calls. When a session has been completed, the @ref fnet_socket_close()
 *   must be performed.
 * - Connectionless, message-oriented sockets, such as the @ref SOCK_DGRAM, allow
 *   sending and receiving of datagrams to and from arbitrary peers using
 *   the @ref fnet_socket_sendto() and the @ref fnet_socket_recvfrom(). If such a socket is connected
 *   to a specific peer, datagrams can be sent to that peer using the
 *   @ref fnet_socket_send(), and can be received only from this peer using the @ref fnet_socket_recv().
 *
 ******************************************************************************/
fnet_socket_t fnet_socket( fnet_address_family_t family, fnet_socket_type_t type, fnet_uint32_t protocol );

/***************************************************************************/ /*!
 *
 * @brief    Assigns a local address to a socket.
 *
 *
 * @param s       Descriptor, identifying a socket to bind.
 *
 * @param name    The address to be assigned to the socket, from the @ref
 *                fnet_sockaddr structure.
 *
 * @param namelen The length of the @c name parameter.
 *                Normally @c namelen is set to @c sizeof(name).
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs. @n
 *     The specific error code can be retrieved using the @ref fnet_error_get().
 *
 * @see fnet_socket()
 *
 ******************************************************************************
 *
 * This function associates a local address with the socket.@n
 * The @ref fnet_socket_bind() function is used on an unconnected socket before
 * subsequent calls to the @ref fnet_socket_connect() or the @ref fnet_socket_listen() functions.
 * It is used to bind to either connection-oriented (stream) or connectionless
 * (datagram) sockets.@n
 * Within the @ref fnet_sockaddr structure, the address may be the address assigned
 * to a network interface on the host or the @ref INADDR_ANY.
 * Using the @ref INADDR_ANY causes the stack to use the default network
 * interface address. Using a port number of 0 causes the service provider
 * to assign a unique ephemeral port number to the socket with a value between 49152 to 65535 (sugested by IANA)
 *
 ******************************************************************************/
fnet_return_t fnet_socket_bind( fnet_socket_t s, const struct fnet_sockaddr *name, fnet_size_t namelen );

/***************************************************************************/ /*!
 *
 * @brief    Places the socket into a state, where it is listening for
 *           an incoming connection.
 *
 *
 * @param s          Descriptor identifying a socket that will be
 *                   used for listening.
 *
 * @param backlog    Maximum length of the queue of pending connections.
 *
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs. @n
 *     The specific error code can be retrieved using the @ref fnet_error_get().
 *
 * @see fnet_socket(), fnet_socket_bind(), fnet_socket_accept()
 *
 ******************************************************************************
 *
 * Only connection-oriented sockets (@ref SOCK_STREAM) are used with the
 * @ref fnet_socket_listen().@n
 * The socket is put into passive mode, where the incoming connection
 * requests are acknowledged and queued pending acceptance by the listening socket.
 * The @ref fnet_socket_listen() function is typically used by servers that can have
 * more than one connection request at a time. @n
 * An application can call @ref fnet_socket_listen() more than once on the same socket.
 * If there are more pending connections than the new backlog value,
 * the excess pending connections will be reset and dropped.
 * Once pending connections accepted by application using fnet_socket_accept(),
 * they are removed from backlog.
 ******************************************************************************/
fnet_return_t fnet_socket_listen( fnet_socket_t s, fnet_size_t backlog );

/***************************************************************************/ /*!
 *
 * @brief    Accepts a connection on the specified socket.
 *
 *
 * @param s          Descriptor, identifying an unconnected socket.
 *
 * @param addr       Optional pointer to a buffer that receives the address
 *                   of the remote host at the other end of the connection.
 *
 * @param addrlen    Optional pointer to an integer that contains the
 *                   length of the @c addr parameter.
 *
 *
 * @return This function returns:
 *   - Socket descriptor referencing the new socket, if no error occurs.
 *   - @ref FNET_NULL if an error occurs. @n
 *     The specific error code can be retrieved using @ref fnet_error_get().
 *
 * @see fnet_socket(), fnet_socket_bind()
 *
 ******************************************************************************
 *
 * The function extracts the first connection in the queue of pending
 * connections on the listening socket @c s, and returns the new socket descriptor.@n
 * The newly-created socket is the socket that will handle the actual
 * connection and has the same properties as the socket @c s.
 * The original socket remains open and listens for new connection requests.
 * If no pending connections are present in the queue of the socket,
 * the @ref fnet_socket_accept() returns FNET_NULL and the specific error code is set to
 * @ref FNET_ERR_AGAIN.@n
 * The parameter @c addr is a result parameter that is filled in with the
 * address of the remote host. The @c addrlen should initially contain the
 * amount of space pointed to by the @c addr; on return it will contain the
 * actual length of the address returned (in bytes).@n
 * The @ref fnet_socket_accept() function is used only with connection-oriented socket
 * types (@ref SOCK_STREAM).@n
 * If @c addr and/or @c addrlen are equal to @c 0, then no information
 * about the remote address of the accepted socket is returned.
 *
 ******************************************************************************/
fnet_socket_t fnet_socket_accept( fnet_socket_t s, struct fnet_sockaddr *addr, fnet_size_t *addrlen );

/***************************************************************************/ /*!
 *
 * @brief    Establishes a connection with the specified socket.
 *
 *
 * @param s          Descriptor identifying an unconnected socket.
 *
 * @param name       Address (name) of the socket, with which the connection
 *                   should be established.
 *
 * @param namelen    Length of the @c name.
 *
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs. @n
 *     The specific error code can be retrieved using the @ref fnet_error_get().
 *
 * @see fnet_socket(), fnet_socket_bind()
 *
 ******************************************************************************
 *
 * For connection-oriented sockets (@ref SOCK_STREAM), this function activates
 * an active connection with the foreign host.
 *
 * When the socket call completes successfully, the socket is ready to send
 * and receive data. Any attempt to reconnect the active connection will fail
 * with the error code @ref FNET_ERR_ISCONN. For connection-oriented
 * sockets, it is often not possible to complete the connection
 * immediately. Until the connection attempt
 * completes on a socket, all subsequent calls to @ref fnet_socket_connect()
 * on the same socket will fail with the error code @ref FNET_ERR_INPROGRESS,
 * or succeed with @ref FNET_ERR_ISCONN when the connection completes successfully.
 * Use the @ref fnet_socket_getopt() function to determine the completion of the
 * connection request by checking the @ref SO_STATE to see if the socket is
 * connected (@ref SS_CONNECTED), is still connecting (@ref SS_CONNECTING)
 * or the connection has failed (@ref SS_CLOSED). @n
 * @n
 * For a connectionless socket (@ref SOCK_DGRAM), the operation performed
 * by @ref fnet_socket_connect() is merely to establish a default destination address
 * that can be used on subsequent @ref fnet_socket_send() and @ref fnet_socket_recv() calls.
 * Any datagrams received from an address other than the destination address
 * specified will be discarded. The default destination can be changed by
 * simply calling the @ref fnet_socket_connect() again, even if the socket is connected already.
 * Any datagrams queued for receipt are discarded from the previous
 * @ref fnet_socket_connect(). For connectionless sockets, the @c name can indicate any
 * valid address, including a broadcast address.@n
 * @n
 * If the socket @c s is unbound, unique values are assigned to the local
 * association by the stack, and the socket is marked as bound.
 *
 ******************************************************************************/
fnet_return_t fnet_socket_connect( fnet_socket_t s, struct fnet_sockaddr *name, fnet_size_t namelen );

/***************************************************************************/ /*!
 *
 * @brief    Receives the data from a connected socket.
 *
 *
 * @param s      Descriptor identifying a connected socket.
 *
 * @param buf    Buffer for the incoming data.
 *
 * @param len    Length of the @c buf.
 *
 * @param flags  Optional flag specifying the way, in which the call is made.
 *               It can be constructed by using the bitwise OR operator with
 *               any of the values defined by the @ref fnet_msg_flags_t.
 *
 *
 * @return This function returns:
 *   - The number of bytes received, if no error occurs.
 *     The return value is set to zero, if there
 *     is no input data.
 *   - @ref FNET_ERR if an error occurs. @n
 *     The specific error code can be retrieved using the @ref fnet_error_get().
 *
 * @see fnet_socket_connect(), fnet_socket_recvfrom()
 *
 ******************************************************************************
 *
 * For connection-oriented sockets (@ref SOCK_STREAM), calling the @ref fnet_socket_recv()
 * will return as much information as is currently available up to the size
 * of the buffer supplied.@n
 * @n
 * For connectionless sockets (@ref SOCK_DGRAM), the data is extracted from the
 * first enqueued datagram (message), whose destination address is specified
 * by the @ref fnet_socket_connect() function.
 * If the datagram is larger than the supplied buffer, the buffer is filled
 * with the first part of the datagram, and the @ref fnet_socket_recv() generates the error
 * @ref FNET_ERR_MSGSIZE and the excess data is lost.@n
 * @n
 * The @c flags parameter can be used to influence the behavior of the function
 * invocation beyond the options specified for the associated socket.
 *
 ******************************************************************************/
fnet_ssize_t fnet_socket_recv( fnet_socket_t s, void *buf, fnet_size_t len, fnet_flag_t flags );

/***************************************************************************/ /*!
 *
 * @brief    Receives the data and captures the address, from which the data
 *           was sent.
 *
 *
 * @param s      Descriptor, identifying a bound socket.
 *
 * @param buf    Buffer for the incoming data.
 *
 * @param len    Length of the @c buf.
 *
 * @param flags  Optional flag specifying the way in which the call is made.
 *               It can be constructed by using the bitwise OR operator with
 *               any of the values defined by the @ref fnet_msg_flags_t.
 *
 * @param from       Optional pointer to a buffer that will hold the
 *                   source address upon return.
 *
 * @param fromlen    Optional pointer to the size of the @c from buffer.
 *
 *
 * @return This function returns:
 *   - The number of bytes received, if no error occurs.
 *     The return value is set to zero, if the socket is nonblocking and there
 *     is no input data.
 *   - @ref FNET_ERR if an error occurs. @n
 *     The specific error code can be retrieved using the @ref fnet_error_get().
 *
 * @see fnet_socket_bind(), fnet_socket_recv()
 *
 ******************************************************************************
 *
 * For connection-oriented sockets (@ref SOCK_STREAM), calling the @ref fnet_socket_recvfrom()
 * will return as much information as is currently available up to the size of
 * the buffer supplied. The @ref fnet_socket_recvfrom() call is rarely used for
 * stream sockets, because the routine provides the peer-socket information,
 * and stream sockets are tied to a specific peer.@n
 * @n
 * For connectionless sockets (@ref SOCK_DGRAM), the data is extracted from the
 * first enqueued datagram.
 * If the datagram is larger than the buffer supplied, the buffer is filled
 * with the first part of the datagram, and the @ref fnet_socket_recvfrom() generates the
 * error @ref FNET_ERR_MSGSIZE and the excess data is lost.@n
 * @n
 * If the @c from parameter is non-zero, the network address of the peer
 * that sent the data is copied to the corresponding @ref fnet_sockaddr structure.
 * The value pointed to by @c fromlen is initialized to the size of this
 * structure and is modified on return, to indicate the actual size of
 * the address stored in the @ref fnet_sockaddr structure.@n
 * @n
 * The flags parameter can be used to influence the behavior of the function
 * invocation beyond the options specified for the associated socket.
 *
 ******************************************************************************/
fnet_ssize_t fnet_socket_recvfrom( fnet_socket_t s, void *buf, fnet_size_t len, fnet_flag_t flags, struct fnet_sockaddr *from, fnet_size_t *fromlen );

/***************************************************************************/ /*!
 *
 * @brief    Sends the data on a connected socket.
 *
 *
 * @param s      Descriptor, identifying a connected socket.
 *
 * @param buf    Buffer containing the data to be transmitted.
 *
 * @param len    Length of the data in @c buf.
 *
 * @param flags  Optional flag specifying the way in which the call is made.
 *               It can be constructed by using the bitwise OR operator with
 *               any of the values defined by the @ref fnet_msg_flags_t.
 *
 *
 * @return This function returns:
 *   - The total number of bytes sent, if no error occurs.
 *     It can be less than the number indicated by @c len.
 *   - @ref FNET_ERR if an error occurs. @n
 *     The specific error code can be retrieved using the @ref fnet_error_get().
 *
 * @see fnet_socket_connect(), fnet_socket_sendto()
 *
 ******************************************************************************
 *
 * For stream-oriented sockets (@ref SOCK_STREAM), the number of
 * actually sent bytes can be between @c 0 and the requested length, depending on
 * buffer availability on both client and server machines.@n
 * @n
 * For message-oriented sockets (@ref SOCK_DGRAM), care must be taken not to
 * exceed the maximum datagram size, which can be obtained by using the @ref
 * fnet_socket_getopt() to retrieve the value of the socket option @ref SO_SNDBUF.
 * If the data is too long to pass atomically through the underlying
 * datagram protocol, the error @ref FNET_ERR_MSGSIZE is returned
 * and no data is transmitted.@n
 * Calling the @ref fnet_socket_send() with a zero @c len parameter is permissible.
 * In such cases, the @ref fnet_socket_send() will return zero as a valid value, and
 * for message-oriented sockets (@ref SOCK_DGRAM) a zero-length transport
 * datagram is sent.@n
 * @n
 * The @c flags parameter can be used to influence the behavior of the function
 * invocation beyond the options specified for the associated socket.
 *
 ******************************************************************************/
fnet_ssize_t fnet_socket_send( fnet_socket_t s, const void *buf, fnet_size_t len, fnet_flag_t flags );

/***************************************************************************/ /*!
 *
 * @brief    Sends the data to a specific destination.
 *
 *
 * @param s      Descriptor, identifying a socket.
 *
 * @param buf    Buffer containing the data to be transmitted.
 *
 * @param len    Length of the data in @c buf.
 *
 * @param flags  Optional flag specifying the way, in which the call is made.
 *               It can be constructed by using the bitwise OR operator with
 *               any of the values defined by the @ref fnet_msg_flags_t.
 *
 * @param to     Pointer to the address of the target socket.
 *
 * @param tolen  Size of the address in @c to.
 *
 *
 * @return This function returns:
 *   - The total number of bytes sent, if no error occurs.
 *     It can be lesser than the number indicated by @c len.
 *   - @ref FNET_ERR if an error occurs. @n
 *     The specific error code can be retrieved using the @ref fnet_error_get().
 *
 * @see fnet_socket_connect(), fnet_socket_send()
 *
 ******************************************************************************
 *
 * For nonb-locking stream-oriented sockets (@ref SOCK_STREAM), the number of
 * actually sent bytes can be between @c 1 and the requested length, depending on
 * buffer availability on both client and server machines.@n
 * @n
 * For message-oriented sockets (@ref SOCK_DGRAM), care must be taken not not
 * exceed the maximum datagram size, which can be obtained by using the @ref
 * fnet_socket_getopt() to retrieve the value of the socket option @ref SO_SNDBUF.
 * If the data is too long to pass atomically through the underlying
 * datagram protocol, the error @ref FNET_ERR_MSGSIZE is returned
 * and no data is transmitted.@n
 * Calling @ref fnet_socket_sendto() with a zero @c len parameter is permissible.
 * In such cases, the @ref fnet_socket_sendto() will return zero as a valid value, and
 * for message-oriented sockets (@ref SOCK_DGRAM), a zero-length transport
 * datagram is sent.@n
 * @n
 * The @ref fnet_socket_sendto() function is normally used on a connectionless socket
 * (@ref SOCK_DGRAM) to send a datagram to a specific peer socket identified
 * by the @c to parameter. Even if the connectionless socket has been previously
 * connected to a specific address, the @c to parameter overrides the
 * destination address for that particular datagram only.@n
 * On a connection-oriented socket (@ref SOCK_STREAM), the @c to and @c tolen
 * parameters are ignored, making the @ref fnet_socket_sendto() equivalent to @ref fnet_socket_send().@n
 * @n
 * The @c flags parameter can be used to influence the behavior of the function
 * invocation beyond the options specified for the associated socket.
 *
 ******************************************************************************/
fnet_ssize_t fnet_socket_sendto( fnet_socket_t s, const void *buf, fnet_size_t len, fnet_flag_t flags, const struct fnet_sockaddr *to, fnet_size_t tolen );

/***************************************************************************/ /*!
 *
 * @brief    Terminates the connection in one or both directions.
 *
 *
 * @param s      Descriptor, identifying a socket.
 *
 * @param how    Flag that describes what types of operation will no longer
 *               be allowed, it's defined by the @ref fnet_sd_flags_t.
 *
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs. @n
 *     The specific error code can be retrieved using the @ref fnet_error_get().
 *
 * @see fnet_socket_close()
 *
 ******************************************************************************
 *
 * This function is used on all types of sockets to disable reception,
 * transmission, or both.@n
 * If the @c how parameter is @ref SD_READ, subsequent calls to the @ref fnet_socket_recv()
 * function on the socket will be unallowed. If the @c how parameter is
 * @ref SD_WRITE, subsequent calls to the @ref fnet_socket_send() function are unallowed.
 * Setting the @c how to the @ref SD_BOTH disables both sends and receives as
 * described above.@n
 * The @ref fnet_socket_shutdown() function does not close the socket. Any resources
 * attached to the socket will not be freed until the @ref fnet_socket_close() is invoked.
 *
 ******************************************************************************/
fnet_return_t fnet_socket_shutdown( fnet_socket_t s, fnet_sd_flags_t how );

/***************************************************************************/ /*!
 *
 * @brief    Closes an existing socket.
 *
 * @param s      Descriptor, identifying a socket to close.
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs. @n
 *     The specific error code can be retrieved using the @ref fnet_error_get().
 *
 * @see fnet_socket()
 *
 ******************************************************************************
 *
 * This function releases the socket descriptor @c s, so that further
 * references to @c s fail with the error @ref FNET_ERR_BAD_DESC.@n
 * An application should always have a matching call to the @ref fnet_socket_close() for
 * each successful call to the @ref fnet_socket() to return any socket resources to
 * the system.@n
 * @n
 * For the @ref SOCK_STREAM sockets, the @ref SO_LINGER option controls the action
 * of the closing, when unsent data is present on the socket and the @ref
 * fnet_socket_close() function is performed. This option has effect on
 * TCP sockets only. To enable or disable the @ref SO_LINGER option, the application
 * should use the @ref fnet_linger structure. This structure includes the
 * @c l_onoff and the @c l_linger fields. If the @c l_onoff has a non-zero value,
 * then the @ref SO_LINGER is enabled. Otherwise, the @ref SO_LINGER option is
 * disabled. The @c l_linger field includes the timeout in seconds.
 * If the @ref SO_LINGER option is not used
 * , the remaining data of the output buffer can be sent after the execution
 * of the @ref fnet_socket_close() in the background.@n
 * If a timeout expires and unsent data is present, the hard reset occurs and
 * unsent data will be deleted.@n
 * If the @ref SO_LINGER option is used with a zero timeout interval, the
 * hard reset occurs immediately and unsent data will be deleted.
 *
 ******************************************************************************/
fnet_return_t fnet_socket_close( fnet_socket_t s );

/***************************************************************************/ /*!
 *
 * @brief    Sets a socket option.
 *
 *
 * @param s          Descriptor identifying a socket.
 *
 * @param level      Level, at which the option is defined.
 *                   The supported levels are @ref SOL_SOCKET,
 *                   @ref IPPROTO_IP, @ref IPPROTO_IPV6 or @ref IPPROTO_TCP.
 *
 * @param optname    Socket option for which the value is to be set.
 *
 * @param optval     Pointer to a buffer, in which the value for the requested
 *                   option is supplied.
 *
 * @param optvallen     Size of the @c optval buffer.
 *
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs. @n
 *     The specific error code can be retrieved using the @ref fnet_error_get().
 *
 * @see fnet_socket_getopt()
 *
 ******************************************************************************
 *
 * This function sets the current value for a socket option associated with the
 * specified socket.@n
 * The various socket options are described by the @ref fnet_socket_options_t.
 *
 ******************************************************************************/
fnet_return_t fnet_socket_setopt( fnet_socket_t s, fnet_protocol_t level, fnet_socket_options_t optname, const void *optval, fnet_size_t optvallen );

/***************************************************************************/ /*!
 *
 * @brief    Gets a socket option.
 *
 *
 * @param s          Descriptor identifying a socket.
 *
 * @param level      Level, at which the option is defined.
 *                   The supported levels are the @ref SOL_SOCKET,
 *                   @ref IPPROTO_IP, @ref IPPROTO_IPV6 or @ref IPPROTO_TCP.
 *
 * @param optname    Socket option, for which the value is to be retrievied.
 *
 * @param optval     Pointer to a buffer in which the value for the requested
 *                   option has to be returned.
 *
 * @param optvallen     Pointer to the size of the @c optval buffer.
 *
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs. @n
 *     The specific error code can be retrieved using the @ref fnet_error_get().
 *
 * @see fnet_socket_setopt()
 *
 ******************************************************************************
 *
 * This function retrieves the current value for a socket option associated
 * with the socket @c s, and stores the result in the @c optval.@n
 * The various socket options are described by the @ref fnet_socket_options_t.
 *
 ******************************************************************************/
fnet_return_t fnet_socket_getopt( fnet_socket_t s, fnet_protocol_t level, fnet_socket_options_t optname, void *optval, fnet_size_t *optvallen );

/***************************************************************************/ /*!
 *
 * @brief    Retrieves the name of a peer connected to a socket.
 *
 *
 * @param s          Descriptor identifying a connected socket.
 *
 * @param name       Structure that receives the name (address) of the peer.
 *
 * @param namelen    Pointer to the size of the @c name structure.
 *
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs. @n
 *     The specific error code can be retrieved using the @ref fnet_error_get().
 *
 * @see fnet_socket_getname()
 *
 ******************************************************************************
 *
 * This function retrieves the name of the peer connected to the socket @c s
 * and stores it in the @ref fnet_sockaddr structure identified by @c name.@n
 * The @ref fnet_socket_getpeername() function can be used only with a connected socket.
 * For datagram sockets, only the name of a peer specified in a previous
 * @ref fnet_socket_connect() call will be returned - any name specified by a previous
 * @ref fnet_socket_sendto() call will not be returned by the @ref fnet_socket_getpeername().
 *
 ******************************************************************************/
fnet_return_t fnet_socket_getpeername( fnet_socket_t s, struct fnet_sockaddr *name, fnet_size_t *namelen );

/***************************************************************************/ /*!
 *
 * @brief    Retrieves the current name for the specified socket.
 *
 *
 * @param s          Descriptor identifying a socket.
 *
 * @param name       Structure that receives the name (address) of the
 *                   socket @c s.
 *
 * @param namelen    Pointer to the size of the @c name structure.
 *
 *
 * @return This function returns:
 *   - @ref FNET_OK if no error occurs.
 *   - @ref FNET_ERR if an error occurs. @n
 *     The specific error code can be retrieved using the @ref fnet_error_get().
 *
 * @see fnet_socket_getpeername()
 *
 ******************************************************************************
 *
 * This function retrieves the current name for the specified socket @c s
 * and stores it in the @c name.@n
 * It is used on a bound or connected socket.
 *
 ******************************************************************************/
fnet_return_t fnet_socket_getname( fnet_socket_t s, struct fnet_sockaddr *name, fnet_size_t *namelen );

/***************************************************************************/ /*!
 *
 * @brief    Compares socket addresses.
 *
 * @param addr1     First socket address to be compared.
 *
 * @param addr2     Second socket address to be compared.
 *
 * @return This function returns:
 *   - @ref FNET_TRUE if addresses are equal.
 *   - @ref FNET_FALSE if addresses are not equal.
 *
 ******************************************************************************
 *
 * This function compares specified socket addresses, depending on address family. @n
 * Port number and Scope Id do not take part in process of comparison.
 *
 ******************************************************************************/
fnet_bool_t fnet_socket_addr_are_equal(const struct fnet_sockaddr *addr1, const struct fnet_sockaddr *addr2);

/***************************************************************************/ /*!
 *
 * @brief    Determines, if socket address is unspecified.
 *
 * @param addr  Socket address.
 *
 * @return       This function returns:
 *   - @c FNET_TRUE if the address is unspecified.
 *   - @c FNET_FALSE if the address is specified.
 *
 ******************************************************************************
 *
 * This function determines if the socket address is unspecified
 *  (IP address is set to all zeros) or not.
 *
 ******************************************************************************/
fnet_bool_t fnet_socket_addr_is_unspecified(const struct fnet_sockaddr *addr);

/***************************************************************************/ /*!
 *
 * @brief    Determines, if socket address is multicast.
 *
 * @param addr  Socket address.
 *
 * @return       This function returns:
 *   - @c FNET_TRUE if the address is multicast.
 *   - @c FNET_FALSE if the address is not multicast.
 *
 ******************************************************************************
 *
 * This function determines if the socket address is multicast or not.
 *
 ******************************************************************************/
fnet_bool_t fnet_socket_addr_is_multicast(const struct fnet_sockaddr *addr);

/***************************************************************************/ /*!
 *
 * @brief    Polls socket event state.
 *
 * @param socket_poll          Array for socket events to be checked.
 * @param socket_poll_size     Number of items in the @c socket_poll array.
 *
 * @return       This function returns number of items in @c socket_poll array which have nonzero @c events_occurred fields.@n
 *               Zero value indicates that no event has occurred.
 *
 ******************************************************************************
 *
 * This function polls for any set of occurred socket events. @n
 * It may be used to check if one or more sockets become ready to perform input/output activity.
 *
 ******************************************************************************/
fnet_size_t fnet_socket_poll(fnet_socket_poll_t *socket_poll, fnet_size_t socket_poll_size);

#if FNET_CFG_SOCKET_CALLBACK_ON_RX || defined(__DOXYGEN__)
/***************************************************************************/ /*!
 *
 * @brief    Registers the "Socket Rx" event handler callback.
 *
 * @param callback    Pointer to the event-handler callback function.
 *
 * @see FNET_CFG_SOCKET_CALLBACK_ON_RX
 *
 ******************************************************************************
 *
 * This function registers the @c handler callback function for
 * the "Socket Rx" event.@n
 * This event occurs when the socket layer receives any packet for any existing socket.
 * It can be a packet with or without data-payload including ACK, FIN and SYN.@n
 * For example, it can be used to wake-up a sleeping RTOS application waiting for a network activity.
 * To stop the event handling, set @c callback parameter to zero value. @n
 * This function is available only if @ref FNET_CFG_SOCKET_CALLBACK_ON_RX is set to @c 1.
 *
 ******************************************************************************/
void fnet_socket_set_callback_on_rx( void(*callback)(void));
#endif

/*  BSD Socket API names */
#if FNET_CFG_SOCKET_BSD_NAMES
#define socket              fnet_socket
#define bind                fnet_socket_bind
#define listen              fnet_socket_listen
#define accept              fnet_socket_accept
#define connect             fnet_socket_connect
#define recv                fnet_socket_recv
#define recvfrom            fnet_socket_recvfrom
#define send                fnet_socket_send
#define sendto              fnet_socket_sendto
#define shutdown            fnet_socket_shutdown
#define closesocket         fnet_socket_close
#define setsockopt          fnet_socket_setopt
#define getsockopt          fnet_socket_getopt
#define getpeername         fnet_socket_getpeername
#define getsockname         fnet_socket_getname
#endif

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* _FNET_SOCKET_H_ */
