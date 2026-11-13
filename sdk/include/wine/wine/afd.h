/*
 * Socket driver ioctls
 *
 * Copyright 2020 Zebediah Figura for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_WINE_AFD_H
#define __WINE_WINE_AFD_H

#include <winternl.h>
#include <winioctl.h>
#include <mswsock.h>

struct afd_wsabuf_32
{
    UINT len;
    UINT buf;
};

#ifdef USE_WS_PREFIX
# define WS(x)    WS_##x
#else
# define WS(x)    x
#endif

#define IOCTL_AFD_BIND                      CTL_CODE(FILE_DEVICE_BEEP, 0x800, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define IOCTL_AFD_LISTEN                    CTL_CODE(FILE_DEVICE_BEEP, 0x802, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define IOCTL_AFD_RECV                      CTL_CODE(FILE_DEVICE_BEEP, 0x805, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define IOCTL_AFD_POLL                      CTL_CODE(FILE_DEVICE_BEEP, 0x809, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_AFD_GETSOCKNAME               CTL_CODE(FILE_DEVICE_BEEP, 0x80b, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define IOCTL_AFD_EVENT_SELECT              CTL_CODE(FILE_DEVICE_BEEP, 0x821, METHOD_NEITHER,  FILE_ANY_ACCESS)
#define IOCTL_AFD_GET_EVENTS                CTL_CODE(FILE_DEVICE_BEEP, 0x822, METHOD_NEITHER,  FILE_ANY_ACCESS)

enum afd_poll_bit
{
    AFD_POLL_BIT_READ           = 0,
    AFD_POLL_BIT_OOB            = 1,
    AFD_POLL_BIT_WRITE          = 2,
    AFD_POLL_BIT_HUP            = 3,
    AFD_POLL_BIT_RESET          = 4,
    AFD_POLL_BIT_CLOSE          = 5,
    AFD_POLL_BIT_CONNECT        = 6,
    AFD_POLL_BIT_ACCEPT         = 7,
    AFD_POLL_BIT_CONNECT_ERR    = 8,
    /* IOCTL_AFD_GET_EVENTS has space for 13 events. */
    AFD_POLL_BIT_UNK1           = 9,
    AFD_POLL_BIT_UNK2           = 10,
    AFD_POLL_BIT_UNK3           = 11,
    AFD_POLL_BIT_UNK4           = 12,
    AFD_POLL_BIT_COUNT          = 13,
};

#define AFD_POLL_READ           0x0001
#define AFD_POLL_OOB            0x0002
#define AFD_POLL_WRITE          0x0004
#define AFD_POLL_HUP            0x0008
#define AFD_POLL_RESET          0x0010
#define AFD_POLL_CLOSE          0x0020
#define AFD_POLL_CONNECT        0x0040
#define AFD_POLL_ACCEPT         0x0080
#define AFD_POLL_CONNECT_ERR    0x0100
/* I have never seen these reported, but StarCraft Remastered polls for them. */
#define AFD_POLL_UNK1           0x0200
#define AFD_POLL_UNK2           0x0400

struct afd_bind_params
{
    int unknown;
    struct WS(sockaddr) addr; /* variable size */
};
C_ASSERT( sizeof(struct afd_bind_params) == 20 );

struct afd_listen_params
{
    int unknown1;
    int backlog;
    int unknown2;
};
C_ASSERT( sizeof(struct afd_listen_params) == 12 );

#define AFD_RECV_FORCE_ASYNC    0x2

#define AFD_MSG_NOT_OOB         0x0020
#define AFD_MSG_OOB             0x0040
#define AFD_MSG_PEEK            0x0080
#define AFD_MSG_WAITALL         0x4000

struct afd_recv_params
{
    const WSABUF *buffers;
    unsigned int count;
    int recv_flags;
    int msg_flags;
};

struct afd_recv_params_32
{
    ULONG buffers;
    unsigned int count;
    int recv_flags;
    int msg_flags;
};

#include <pshpack4.h>
struct afd_poll_params
{
    LONGLONG timeout;
    unsigned int count;
    BOOLEAN exclusive;
    BOOLEAN padding[3];
    struct afd_poll_socket
    {
        SOCKET socket;
        int flags;
        int status;
    } sockets[1];
};

struct afd_poll_params_64
{
    LONGLONG timeout;
    unsigned int count;
    BOOLEAN exclusive;
    BOOLEAN padding[3];
    struct afd_poll_socket_64
    {
        ULONGLONG socket;
        int flags;
        int status;
    } sockets[1];
};

struct afd_poll_params_32
{
    LONGLONG timeout;
    unsigned int count;
    BOOLEAN exclusive;
    BOOLEAN padding[3];
    struct afd_poll_socket_32
    {
        ULONG socket;
        int flags;
        int status;
    } sockets[1];
};
#include <poppack.h>

struct afd_event_select_params
{
    HANDLE event;
    int mask;
};

struct afd_event_select_params_64
{
    ULONGLONG event;
    int mask;
};

struct afd_event_select_params_32
{
    ULONG event;
    int mask;
};

struct afd_get_events_params
{
    int flags;
    int status[13];
};
C_ASSERT( sizeof(struct afd_get_events_params) == 56 );

#define WINE_AFD_IOC(x) CTL_CODE(FILE_DEVICE_NETWORK, x, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_AFD_WINE_CREATE                           WINE_AFD_IOC(200)
#define IOCTL_AFD_WINE_ACCEPT                           WINE_AFD_IOC(201)
#define IOCTL_AFD_WINE_ACCEPT_INTO                      WINE_AFD_IOC(202)
#define IOCTL_AFD_WINE_CONNECT                          WINE_AFD_IOC(203)
#define IOCTL_AFD_WINE_SHUTDOWN                         WINE_AFD_IOC(204)
#define IOCTL_AFD_WINE_RECVMSG                          WINE_AFD_IOC(205)
#define IOCTL_AFD_WINE_SENDMSG                          WINE_AFD_IOC(206)
#define IOCTL_AFD_WINE_TRANSMIT                         WINE_AFD_IOC(207)
#define IOCTL_AFD_WINE_ADDRESS_LIST_CHANGE              WINE_AFD_IOC(208)
#define IOCTL_AFD_WINE_FIONBIO                          WINE_AFD_IOC(209)
#define IOCTL_AFD_WINE_COMPLETE_ASYNC                   WINE_AFD_IOC(210)
#define IOCTL_AFD_WINE_FIONREAD                         WINE_AFD_IOC(211)
#define IOCTL_AFD_WINE_SIOCATMARK                       WINE_AFD_IOC(212)
#define IOCTL_AFD_WINE_GET_INTERFACE_LIST               WINE_AFD_IOC(213)
#define IOCTL_AFD_WINE_KEEPALIVE_VALS                   WINE_AFD_IOC(214)
#define IOCTL_AFD_WINE_MESSAGE_SELECT                   WINE_AFD_IOC(215)
#define IOCTL_AFD_WINE_GETPEERNAME                      WINE_AFD_IOC(216)
#define IOCTL_AFD_WINE_DEFER                            WINE_AFD_IOC(217)
#define IOCTL_AFD_WINE_GET_INFO                         WINE_AFD_IOC(218)
#define IOCTL_AFD_WINE_GET_SO_ACCEPTCONN                WINE_AFD_IOC(219)
#define IOCTL_AFD_WINE_GET_SO_BROADCAST                 WINE_AFD_IOC(220)
#define IOCTL_AFD_WINE_SET_SO_BROADCAST                 WINE_AFD_IOC(221)
#define IOCTL_AFD_WINE_GET_SO_ERROR                     WINE_AFD_IOC(222)
#define IOCTL_AFD_WINE_GET_SO_KEEPALIVE                 WINE_AFD_IOC(223)
#define IOCTL_AFD_WINE_SET_SO_KEEPALIVE                 WINE_AFD_IOC(224)
#define IOCTL_AFD_WINE_GET_SO_LINGER                    WINE_AFD_IOC(225)
#define IOCTL_AFD_WINE_SET_SO_LINGER                    WINE_AFD_IOC(226)
#define IOCTL_AFD_WINE_GET_SO_OOBINLINE                 WINE_AFD_IOC(227)
#define IOCTL_AFD_WINE_SET_SO_OOBINLINE                 WINE_AFD_IOC(228)
#define IOCTL_AFD_WINE_SET_SO_RCVBUF                    WINE_AFD_IOC(229)
#define IOCTL_AFD_WINE_GET_SO_RCVBUF                    WINE_AFD_IOC(230)
#define IOCTL_AFD_WINE_SET_SO_RCVTIMEO                  WINE_AFD_IOC(231)
#define IOCTL_AFD_WINE_GET_SO_RCVTIMEO                  WINE_AFD_IOC(232)
#define IOCTL_AFD_WINE_GET_SO_REUSEADDR                 WINE_AFD_IOC(233)
#define IOCTL_AFD_WINE_SET_SO_REUSEADDR                 WINE_AFD_IOC(234)
#define IOCTL_AFD_WINE_SET_SO_SNDBUF                    WINE_AFD_IOC(235)
#define IOCTL_AFD_WINE_GET_SO_SNDBUF                    WINE_AFD_IOC(236)
#define IOCTL_AFD_WINE_GET_SO_SNDTIMEO                  WINE_AFD_IOC(237)
#define IOCTL_AFD_WINE_SET_SO_SNDTIMEO                  WINE_AFD_IOC(238)
#define IOCTL_AFD_WINE_SET_IP_ADD_MEMBERSHIP            WINE_AFD_IOC(239)
#define IOCTL_AFD_WINE_SET_IP_ADD_SOURCE_MEMBERSHIP     WINE_AFD_IOC(240)
#define IOCTL_AFD_WINE_SET_IP_BLOCK_SOURCE              WINE_AFD_IOC(241)
#define IOCTL_AFD_WINE_GET_IP_DONTFRAGMENT              WINE_AFD_IOC(242)
#define IOCTL_AFD_WINE_SET_IP_DONTFRAGMENT              WINE_AFD_IOC(243)
#define IOCTL_AFD_WINE_SET_IP_DROP_MEMBERSHIP           WINE_AFD_IOC(244)
#define IOCTL_AFD_WINE_SET_IP_DROP_SOURCE_MEMBERSHIP    WINE_AFD_IOC(245)
#define IOCTL_AFD_WINE_GET_IP_HDRINCL                   WINE_AFD_IOC(246)
#define IOCTL_AFD_WINE_SET_IP_HDRINCL                   WINE_AFD_IOC(247)
#define IOCTL_AFD_WINE_GET_IP_MULTICAST_IF              WINE_AFD_IOC(248)
#define IOCTL_AFD_WINE_SET_IP_MULTICAST_IF              WINE_AFD_IOC(249)
#define IOCTL_AFD_WINE_GET_IP_MULTICAST_LOOP            WINE_AFD_IOC(250)
#define IOCTL_AFD_WINE_SET_IP_MULTICAST_LOOP            WINE_AFD_IOC(251)
#define IOCTL_AFD_WINE_GET_IP_MULTICAST_TTL             WINE_AFD_IOC(252)
#define IOCTL_AFD_WINE_SET_IP_MULTICAST_TTL             WINE_AFD_IOC(253)
#define IOCTL_AFD_WINE_GET_IP_OPTIONS                   WINE_AFD_IOC(254)
#define IOCTL_AFD_WINE_SET_IP_OPTIONS                   WINE_AFD_IOC(255)
#define IOCTL_AFD_WINE_GET_IP_PKTINFO                   WINE_AFD_IOC(256)
#define IOCTL_AFD_WINE_SET_IP_PKTINFO                   WINE_AFD_IOC(257)
#define IOCTL_AFD_WINE_GET_IP_TOS                       WINE_AFD_IOC(258)
#define IOCTL_AFD_WINE_SET_IP_TOS                       WINE_AFD_IOC(259)
#define IOCTL_AFD_WINE_GET_IP_TTL                       WINE_AFD_IOC(260)
#define IOCTL_AFD_WINE_SET_IP_TTL                       WINE_AFD_IOC(261)
#define IOCTL_AFD_WINE_SET_IP_UNBLOCK_SOURCE            WINE_AFD_IOC(262)
#define IOCTL_AFD_WINE_GET_IP_UNICAST_IF                WINE_AFD_IOC(263)
#define IOCTL_AFD_WINE_SET_IP_UNICAST_IF                WINE_AFD_IOC(264)
#define IOCTL_AFD_WINE_SET_IPV6_ADD_MEMBERSHIP          WINE_AFD_IOC(265)
#define IOCTL_AFD_WINE_GET_IPV6_DONTFRAG                WINE_AFD_IOC(266)
#define IOCTL_AFD_WINE_SET_IPV6_DONTFRAG                WINE_AFD_IOC(267)
#define IOCTL_AFD_WINE_SET_IPV6_DROP_MEMBERSHIP         WINE_AFD_IOC(268)
#define IOCTL_AFD_WINE_GET_IPV6_MULTICAST_HOPS          WINE_AFD_IOC(269)
#define IOCTL_AFD_WINE_SET_IPV6_MULTICAST_HOPS          WINE_AFD_IOC(270)
#define IOCTL_AFD_WINE_GET_IPV6_MULTICAST_IF            WINE_AFD_IOC(271)
#define IOCTL_AFD_WINE_SET_IPV6_MULTICAST_IF            WINE_AFD_IOC(272)
#define IOCTL_AFD_WINE_GET_IPV6_MULTICAST_LOOP          WINE_AFD_IOC(273)
#define IOCTL_AFD_WINE_SET_IPV6_MULTICAST_LOOP          WINE_AFD_IOC(274)
#define IOCTL_AFD_WINE_GET_IPV6_UNICAST_HOPS            WINE_AFD_IOC(275)
#define IOCTL_AFD_WINE_SET_IPV6_UNICAST_HOPS            WINE_AFD_IOC(276)
#define IOCTL_AFD_WINE_GET_IPV6_UNICAST_IF              WINE_AFD_IOC(277)
#define IOCTL_AFD_WINE_SET_IPV6_UNICAST_IF              WINE_AFD_IOC(278)
#define IOCTL_AFD_WINE_GET_IPV6_V6ONLY                  WINE_AFD_IOC(279)
#define IOCTL_AFD_WINE_SET_IPV6_V6ONLY                  WINE_AFD_IOC(280)
#define IOCTL_AFD_WINE_GET_IPX_PTYPE                    WINE_AFD_IOC(281)
#define IOCTL_AFD_WINE_SET_IPX_PTYPE                    WINE_AFD_IOC(282)
#define IOCTL_AFD_WINE_GET_IRLMP_ENUMDEVICES            WINE_AFD_IOC(283)
#define IOCTL_AFD_WINE_GET_TCP_NODELAY                  WINE_AFD_IOC(284)
#define IOCTL_AFD_WINE_SET_TCP_NODELAY                  WINE_AFD_IOC(285)
#define IOCTL_AFD_WINE_GET_IPV6_RECVHOPLIMIT            WINE_AFD_IOC(286)
#define IOCTL_AFD_WINE_SET_IPV6_RECVHOPLIMIT            WINE_AFD_IOC(287)
#define IOCTL_AFD_WINE_GET_IPV6_RECVPKTINFO             WINE_AFD_IOC(288)
#define IOCTL_AFD_WINE_SET_IPV6_RECVPKTINFO             WINE_AFD_IOC(289)
#define IOCTL_AFD_WINE_GET_IPV6_RECVTCLASS              WINE_AFD_IOC(290)
#define IOCTL_AFD_WINE_SET_IPV6_RECVTCLASS              WINE_AFD_IOC(291)
#define IOCTL_AFD_WINE_GET_SO_CONNECT_TIME              WINE_AFD_IOC(292)
#define IOCTL_AFD_WINE_GET_IP_RECVTTL                   WINE_AFD_IOC(293)
#define IOCTL_AFD_WINE_SET_IP_RECVTTL                   WINE_AFD_IOC(294)
#define IOCTL_AFD_WINE_GET_IP_RECVTOS                   WINE_AFD_IOC(295)
#define IOCTL_AFD_WINE_SET_IP_RECVTOS                   WINE_AFD_IOC(296)
#define IOCTL_AFD_WINE_GET_SO_EXCLUSIVEADDRUSE          WINE_AFD_IOC(297)
#define IOCTL_AFD_WINE_SET_SO_EXCLUSIVEADDRUSE          WINE_AFD_IOC(298)
#define IOCTL_AFD_WINE_GET_TCP_KEEPALIVE                WINE_AFD_IOC(299)
#define IOCTL_AFD_WINE_SET_TCP_KEEPALIVE                WINE_AFD_IOC(300)
#define IOCTL_AFD_WINE_GET_TCP_KEEPCNT                  WINE_AFD_IOC(301)
#define IOCTL_AFD_WINE_SET_TCP_KEEPCNT                  WINE_AFD_IOC(302)
#define IOCTL_AFD_WINE_GET_TCP_KEEPINTVL                WINE_AFD_IOC(303)
#define IOCTL_AFD_WINE_SET_TCP_KEEPINTVL                WINE_AFD_IOC(304)

struct afd_iovec
{
    ULONGLONG ptr;
    ULONG len;
};

struct afd_create_params
{
    int family, type, protocol;
    unsigned int flags;
};
C_ASSERT( sizeof(struct afd_create_params) == 16 );

struct afd_accept_into_params
{
    ULONG accept_handle;
    unsigned int recv_len, local_len;
};
C_ASSERT( sizeof(struct afd_accept_into_params) == 12 );

struct afd_connect_params
{
    int addr_len;
    int synchronous;
    /* VARARG(addr, struct WS(sockaddr), addr_len); */
    /* VARARG(data, bytes); */
};
C_ASSERT( sizeof(struct afd_connect_params) == 8 );

struct afd_recvmsg_params
{
    ULONGLONG control_ptr; /* WSABUF */
    ULONGLONG addr_ptr; /* WS(sockaddr) */
    ULONGLONG addr_len_ptr; /* int */
    ULONGLONG ws_flags_ptr; /* unsigned int */
    int force_async;
    unsigned int count;
    ULONGLONG buffers_ptr; /* WSABUF[] */
};
C_ASSERT( sizeof(struct afd_recvmsg_params) == 48 );

struct afd_sendmsg_params
{
    ULONGLONG addr_ptr; /* const struct WS(sockaddr) */
    unsigned int addr_len;
    unsigned int ws_flags;
    int force_async;
    unsigned int count;
    ULONGLONG buffers_ptr; /* const WSABUF[] */
};
C_ASSERT( sizeof(struct afd_sendmsg_params) == 32 );

struct afd_transmit_params
{
    LARGE_INTEGER offset;
    ULONGLONG head_ptr;
    ULONGLONG tail_ptr;
    DWORD head_len;
    DWORD tail_len;
    ULONG file;
    DWORD file_len;
    DWORD buffer_size;
    DWORD flags;
};
C_ASSERT( sizeof(struct afd_transmit_params) == 48 );

struct afd_message_select_params
{
    ULONG handle;
    ULONG window;
    unsigned int message;
    int mask;
};
C_ASSERT( sizeof(struct afd_message_select_params) == 16 );

struct afd_get_info_params
{
    int family, type, protocol;
};
C_ASSERT( sizeof(struct afd_get_info_params) == 12 );

#endif
