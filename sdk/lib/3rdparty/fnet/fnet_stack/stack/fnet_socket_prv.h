/**************************************************************************
*
* Copyright 2008-2018 by Andrey Butok. FNET Community.
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
*  Private. Socket API.
*
***************************************************************************/

#ifndef _FNET_SOCKET_PRV_H_

#define _FNET_SOCKET_PRV_H_

#include "fnet_error.h"
#include "fnet_socket.h"

#if FNET_CFG_TCP

    #include "fnet_tcp.h"

#endif

#include "fnet_netbuf_prv.h"
#include "fnet_ip4_prv.h"
#include "fnet_ip6_prv.h"

/************************************************************************
*  Definitions.
*************************************************************************/

/*IANA: Port numbers are assigned in various ways, based on three ranges: System
* Ports (0-1023), User Ports (1024-49151), and the Dynamic and/or Private
* Ports (49152-65535).
* So, an allocated port will always be FNET_SOCKET_PORT_EPHEMERAL_BEGIN <= local_port <= FNET_SOCKET_PORT_EPHEMERAL_END (ephemeral port)
* The Internet Assigned Numbers Authority (IANA) suggests the range 49152 to 65535 for dynamic or private ports.*/
#define FNET_SOCKET_PORT_EPHEMERAL_BEGIN    (49152u)  /* In host byte order.*/
#define FNET_SOCKET_PORT_EPHEMERAL_END      (65535u)  /* In host byte order.*/

#define FNET_SOCKET_DESC_RESERVED       (-1)    /* The descriptor is reserved.*/

/**************************************************************************/ /*!
 * @internal
 * @brief    Structure of socket buffer.
 ******************************************************************************/
typedef struct
{
    fnet_size_t     count;              /**< Aactual chars in buffer.*/
    fnet_size_t     count_max;          /**< Max actual char count (9*1024).*/
    fnet_netbuf_t   *net_buf_chain;     /**< The net_buf chain.*/
    fnet_bool_t     is_shutdown;        /**< The socket has been shut down for read/write.*/
} fnet_socket_buffer_t;

/**************************************************************************/ /*!
 * @internal
 * @brief    Structure contains parameter of receive datagram
 * (only for SOCK_DGRAM).
 ******************************************************************************/
typedef struct
{
    struct fnet_sockaddr addr_s;
} fnet_socket_buffer_addr_t;

/**************************************************************************/ /*!
 * @internal
 * @brief    Socket options.
 ******************************************************************************/
typedef struct
{
#if FNET_CFG_IP4
    fnet_ip4_sockopt_t  ip4_opt;        /**< IPv4 options.*/
#endif
#if FNET_CFG_IP6
    fnet_ip6_sockopt_t  ip6_opt;        /**< IPv6 options.*/
#endif
#if FNET_CFG_TCP
    fnet_tcp_sockopt_t  tcp_opt;        /**< TCP options.*/
#endif

    fnet_error_t        error;          /**< Socket last error.*/
    fnet_error_t        local_error;    /**< Socket local error (ICMP, on timeout).*/
    fnet_time_t         linger_ms;      /**< Lingers on close if unsent data is present (in ms).*/
    fnet_bool_t         so_dontroute;
    fnet_bool_t         so_keepalive;
    fnet_bool_t         so_linger;
#if FNET_CFG_TCP_URGENT
    fnet_bool_t         so_oobinline;
#endif
} fnet_socket_option_t;

/************************************************************************
*    Structure per socket.
*************************************************************************/
typedef struct _fnet_socket_if_t
{
    struct _fnet_socket_if_t    *next;                  /**< Pointer to the next socket structure.*/
    struct _fnet_socket_if_t    *prev;                  /**< Pointer to the previous socket structure.*/
    fnet_socket_t               descriptor;             /**< Socket descriptor.*/
    fnet_socket_state_t         state;                  /**< Socket state.*/
    fnet_uint32_t               protocol_number;        /**< Protocol number.*/
    struct fnet_prot_if         *protocol_interface;    /**< Points to protocol specific functions (interface).*/
    void                        *protocol_control;      /**< Points to protocol control structure (optional).*/

    /* For sockets with SO_ACCEPTCONN.*/
    struct _fnet_socket_if_t    *partial_con;           /**< Queue of partial connections.*/
    fnet_size_t                 partial_con_len;        /**< Number of connections on partial_con.*/
    struct _fnet_socket_if_t    *incoming_con;          /**< Queue of incoming connections.*/
    fnet_size_t                 incoming_con_len;       /**< Number of connections on incoming_con.*/
    fnet_size_t                 con_limit;              /**< Max number queued connections (specified  by "listen").*/
    struct _fnet_socket_if_t    *head_con;              /**< Back pointer to accept socket.*/

    fnet_socket_buffer_t        receive_buffer;         /**< Socket buffer for incoming data.*/
    fnet_socket_buffer_t        send_buffer;            /**< Socket buffer for outgoing data.*/

    /* Common protocol params.*/
    struct fnet_sockaddr        foreign_addr;           /**< Foreign socket address.*/
    struct fnet_sockaddr        local_addr;             /**< Lockal socket address.*/
    fnet_socket_option_t        options;                /**< Collection of socket options.*/

#if FNET_CFG_MULTICAST
    /* Multicast params.*/
#if FNET_CFG_IP4
    fnet_ip4_multicast_list_entry_t *ip4_multicast_entry[FNET_CFG_MULTICAST_SOCKET_MAX];
#endif
#if FNET_CFG_IP6
    fnet_ip6_multicast_list_entry_t *ip6_multicast_entry[FNET_CFG_MULTICAST_SOCKET_MAX];
#endif
#endif /* FNET_CFG_MULTICAST */

} fnet_socket_if_t;

/**************************************************************************/ /*!
 * @internal
 * @brief    Transport Protocol interface general API structure.
 ******************************************************************************/
typedef struct fnet_socket_prot_if
{
    fnet_bool_t     con_req;                                                                                        /* Flag that protocol is connection oriented.*/
    fnet_return_t   (*prot_attach)(fnet_socket_if_t *sk);                                                           /* Protocol "attach" function. */
    fnet_return_t   (*prot_detach)(fnet_socket_if_t *sk);                                                           /* Protocol "detach" function. */
    fnet_return_t   (*prot_connect)(fnet_socket_if_t *sk, struct fnet_sockaddr *foreign_addr);                           /* Protocol "connect" function. */
    fnet_socket_if_t *( *prot_accept)(fnet_socket_if_t *sk);                                                        /* Protocol "accept" function. */
    fnet_ssize_t    (*prot_rcv)(fnet_socket_if_t *sk, fnet_uint8_t *buf, fnet_size_t len, fnet_flag_t flags, struct fnet_sockaddr *foreign_addr );        /* Protocol "receive" function. */
    fnet_ssize_t    (*prot_snd)(fnet_socket_if_t *sk, const fnet_uint8_t *buf, fnet_size_t len, fnet_flag_t flags, const struct fnet_sockaddr *foreign_addr );  /* Protocol "send" function. */
    fnet_return_t   (*prot_shutdown)(fnet_socket_if_t *sk, fnet_sd_flags_t how);                                    /* Protocol "shutdown" function. */
    fnet_return_t   (*prot_setsockopt)(fnet_socket_if_t *sk, fnet_protocol_t level, fnet_socket_options_t optname, const void *optval, fnet_size_t optlen); /* Protocol "setsockopt" function. */
    fnet_return_t   (*prot_getsockopt)(fnet_socket_if_t *sk, fnet_protocol_t level, fnet_socket_options_t optname, void *optval, fnet_size_t *optlen);      /* Protocol "getsockopt" function. */
    fnet_return_t   (*prot_listen)(fnet_socket_if_t *sk, fnet_size_t backlog);                                      /* Protocol "listen" function.*/
} fnet_socket_prot_if_t;

#if FNET_CFG_SOCKET_CALLBACK_ON_RX
    extern fnet_event_desc_t fnet_socket_event_rx;
#endif

/************************************************************************
*     Function Prototypes
*************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

fnet_return_t _fnet_socket_init( void );
void _fnet_socket_list_add( fnet_socket_if_t **head, fnet_socket_if_t *s );
void _fnet_socket_list_del( fnet_socket_if_t **head, fnet_socket_if_t *s );
void _fnet_socket_set_error( fnet_socket_if_t *sock, fnet_error_t error );
fnet_socket_if_t *_fnet_socket_lookup( fnet_socket_if_t *head,  struct fnet_sockaddr *local_addr, struct fnet_sockaddr *foreign_addr, fnet_uint32_t protocol_number);
fnet_uint16_t  _fnet_socket_get_uniqueport(fnet_socket_if_t *head, struct fnet_sockaddr *local_addr);
fnet_bool_t _fnet_socket_conflict( fnet_socket_if_t *head,  const struct fnet_sockaddr *local_addr, const struct fnet_sockaddr *foreign_addr /*optional*/, fnet_bool_t wildcard );
fnet_socket_if_t *_fnet_socket_copy( fnet_socket_if_t *sock );
void _fnet_socket_release( fnet_socket_if_t **head, fnet_socket_if_t *sock );
fnet_return_t _fnet_socket_buffer_append_address( fnet_socket_buffer_t *sb, fnet_netbuf_t *nb, struct fnet_sockaddr *addr);
fnet_return_t _fnet_socket_buffer_append_record( fnet_socket_buffer_t *sb, fnet_netbuf_t *nb );
fnet_int32_t _fnet_socket_buffer_read_address( fnet_socket_buffer_t *sb, fnet_uint8_t *buf, fnet_size_t len, struct fnet_sockaddr *foreign_addr, fnet_bool_t remove );
fnet_size_t _fnet_socket_buffer_read_record( fnet_socket_buffer_t *sb, fnet_uint8_t *buf, fnet_size_t len, fnet_bool_t remove );
void _fnet_socket_buffer_release( fnet_socket_buffer_t *sb );
fnet_bool_t _fnet_socket_addr_is_broadcast(const struct fnet_sockaddr *addr, fnet_netif_t *netif);
void _fnet_socket_ip_addr_copy(const struct fnet_sockaddr *from_addr, struct fnet_sockaddr *to_addr);
void _fnet_socket_addr_copy(const struct fnet_sockaddr *from_addr, struct fnet_sockaddr *to_addr);
fnet_netif_t *_fnet_socket_addr_route(const struct fnet_sockaddr *dest_addr);

#if defined(__cplusplus)
}
#endif

#endif /* _FNET_SOCKET_PRV_H_ */
