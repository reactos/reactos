/**************************************************************************
*
* Copyright 2011-2017 by Andrey Butok. FNET Community.
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
*  RAW socket implementation.
*
***************************************************************************/

#include "fnet.h"
#include "fnet_raw.h"
#include "fnet_ip_prv.h"
#include "fnet_prot.h"

#if FNET_CFG_RAW

/************************************************************************
*     Function Prototypes
*************************************************************************/
static fnet_return_t _fnet_raw_attach( fnet_socket_if_t *sk );
static fnet_return_t _fnet_raw_detach( fnet_socket_if_t *sk );
static fnet_return_t _fnet_raw_connect( fnet_socket_if_t *sk, struct fnet_sockaddr *foreign_addr);
static fnet_int32_t _fnet_raw_snd( fnet_socket_if_t *sk, const fnet_uint8_t *buf, fnet_size_t len, fnet_flag_t flags, const struct fnet_sockaddr *addr);
static fnet_int32_t _fnet_raw_rcv(fnet_socket_if_t *sk, fnet_uint8_t *buf, fnet_size_t len, fnet_flag_t flags, struct fnet_sockaddr *addr);
static fnet_return_t _fnet_raw_shutdown( fnet_socket_if_t *sk, fnet_sd_flags_t how );
static void _fnet_raw_release(void);
static fnet_error_t _fnet_raw_output(struct fnet_sockaddr *src_addr, const struct fnet_sockaddr *dest_addr, fnet_uint8_t protocol_number, fnet_socket_option_t *sockoption, fnet_netbuf_t *nb);

/************************************************************************
*     Global Data Structures
*************************************************************************/

/************************************************************************
 * Protocol API structures.
 ************************************************************************/
static const fnet_socket_prot_if_t fnet_raw_socket_api =
{
    FNET_FALSE,              /* Flag that protocol is connection oriented.*/
    _fnet_raw_attach,        /* Protocol "attach" function.*/
    _fnet_raw_detach,        /* Protocol "detach" function.*/
    _fnet_raw_connect,       /* Protocol "connect" function.*/
    0,                      /* Protocol "accept" function.*/
    _fnet_raw_rcv,           /* Protocol "receive" function.*/
    _fnet_raw_snd,           /* Protocol "send" function.*/
    _fnet_raw_shutdown,      /* Protocol "shutdown" function.*/
    _fnet_ip_setsockopt,     /* Protocol "setsockopt" function.*/
    _fnet_ip_getsockopt,     /* Protocol "getsockopt" function.*/
    0,                      /* Protocol "listen" function.*/
};

fnet_prot_if_t fnet_raw_prot_if =
{
    .family = AF_SUPPORTED,             /* Address domain family.*/
    .type = SOCK_RAW,                   /* Socket type used for.*/
    .prot_release = _fnet_raw_release,   /* Protocol release function.*/
    .prot_input = _fnet_raw_input,       /* Protocol input function,.*/
    .socket_api = &fnet_raw_socket_api  /* Socket API */
};

/************************************************************************
* DESCRIPTION: This function releases all sockets associated
*              with RAW protocol.
*************************************************************************/
static void _fnet_raw_release( void )
{
    while(fnet_raw_prot_if.head)
    {
        _fnet_socket_release(&fnet_raw_prot_if.head, fnet_raw_prot_if.head);
    }
}

/************************************************************************
* DESCRIPTION: RAW output function
*************************************************************************/
static fnet_error_t _fnet_raw_output( struct fnet_sockaddr *src_addr, const struct fnet_sockaddr *dest_addr, fnet_uint8_t protocol_number,
                                      fnet_socket_option_t *sockoption, fnet_netbuf_t *nb )
{
    fnet_error_t error =  FNET_ERR_OK;

    fnet_netif_t *netif =  _fnet_netif_get_by_scope_id( dest_addr->sa_scope_id );

#if FNET_CFG_IP4
    if(dest_addr->sa_family == AF_INET)
    {
        error = _fnet_ip4_output(netif, ((struct fnet_sockaddr_in *)(src_addr))->sin_addr.s_addr,
                                 ((const struct fnet_sockaddr_in *)(dest_addr))->sin_addr.s_addr,
                                 protocol_number,
                                 sockoption->ip4_opt.tos,
#if FNET_CFG_MULTICAST
                                 (fnet_uint8_t)((FNET_IP4_ADDR_IS_MULTICAST(((const struct fnet_sockaddr_in *)(dest_addr))->sin_addr.s_addr) ? sockoption->ip4_opt.ttl_multicast : sockoption->ip4_opt.ttl)),
#else
                                 sockoption->ip4_opt.ttl,
#endif /* FNET_CFG_MULTICAST */
                                 nb, FNET_FALSE, sockoption->so_dontroute,
                                 0
                                );
    }
#endif

#if FNET_CFG_IP6
    if(dest_addr->sa_family == AF_INET6)
    {
        error = _fnet_ip6_output( netif,
                                  fnet_socket_addr_is_unspecified(src_addr) ? FNET_NULL : & ((struct fnet_sockaddr_in6 *)(src_addr))->sin6_addr.s6_addr,
                                  &((const struct fnet_sockaddr_in6 *)(dest_addr))->sin6_addr.s6_addr,
                                  protocol_number,
                                  FNET_IP6_ADDR_IS_MULTICAST(&((const struct fnet_sockaddr_in6 *)(dest_addr))->sin6_addr.s6_addr) ? sockoption->ip6_opt.hops_multicast : sockoption->ip6_opt.hops_unicast,
                                  nb, 0);
    }
#endif

    return (error);
}

/************************************************************************
* DESCRIPTION: RAW input function.
*************************************************************************/
void _fnet_raw_input(fnet_netif_t *netif, struct fnet_sockaddr *foreign_addr, struct fnet_sockaddr *local_addr, fnet_netbuf_t *nb, fnet_netbuf_t *ip_nb)
{
    fnet_socket_if_t       *sock;
    fnet_socket_if_t       *last;
    fnet_netbuf_t       *nb_tmp;
    fnet_uint32_t       protocol_number;

    if(netif && nb && ip_nb && (nb->total_length))
    {
#if FNET_CFG_IP4
        if(foreign_addr->sa_family == AF_INET)
        {
            protocol_number = (fnet_uint32_t)((fnet_ip4_header_t *)(ip_nb->data_ptr))->protocol;
        }
        else
#endif
#if FNET_CFG_IP6
            if(foreign_addr->sa_family == AF_INET6)
            {
                protocol_number = (fnet_uint32_t)((fnet_ip6_header_t *)(ip_nb->data_ptr))->next_header;
            }
            else
#endif
            {
                protocol_number = 0u;
            }

        /* Demultiplex broadcast & multicast datagrams.*/
        if((_fnet_socket_addr_is_broadcast(local_addr, netif)) || (fnet_socket_addr_is_multicast(local_addr)))
        {
            last = 0;

            for (sock = fnet_raw_prot_if.head; sock != 0; sock = sock->next)
            {
                /* Ignore local port number.*/

                /* Check protocol number.*/
                if(sock->protocol_number != protocol_number)
                {
                    continue; /* => ignore.*/
                }
#if FNET_CFG_MULTICAST
                if(fnet_socket_addr_is_multicast(local_addr))
                {
                    fnet_index_t m;
                    fnet_bool_t for_us = FNET_FALSE;

#if FNET_CFG_IP4
                    if(local_addr->sa_family == AF_INET)
                    {
                        for(m = 0u; m < FNET_CFG_MULTICAST_SOCKET_MAX; m++)
                        {
                            if(sock->ip4_multicast_entry[m])
                            {
                                if((sock->ip4_multicast_entry[m]->group_addr == ((struct fnet_sockaddr_in *)(local_addr))->sin_addr.s_addr) &&  (sock->ip4_multicast_entry[m]->netif == netif ))
                                {
                                    for_us = FNET_TRUE;
                                }
                            }
                        }
                    }
#endif
#if FNET_CFG_IP6
                    if(local_addr->sa_family == AF_INET6)
                    {
                        for(m = 0u; m < FNET_CFG_MULTICAST_SOCKET_MAX; m++)
                        {
                            if(sock->ip6_multicast_entry[m])
                            {
                                if(FNET_IP6_ADDR_EQUAL(&sock->ip6_multicast_entry[m]->group_addr, &((struct fnet_sockaddr_in6 *)(local_addr))->sin6_addr.s6_addr) && (sock->ip6_multicast_entry[m]->netif == netif))
                                {
                                    for_us = FNET_TRUE;
                                }
                            }
                        }
                    }
#endif

                    if(for_us == FNET_FALSE)
                    {
                        continue;
                    }
                }
                else
#endif /* FNET_CFG_MULTICAST */
                {
                    /* Compare local address.*/
                    if(!fnet_socket_addr_is_unspecified(&sock->local_addr))
                    {
                        if(!fnet_socket_addr_are_equal(&sock->local_addr, local_addr))
                        {
                            continue;
                        }
                    }

                    /* Compare foreign address and port number.*/
                    if(!fnet_socket_addr_is_unspecified(&sock->foreign_addr))
                    {
                        if((!fnet_socket_addr_are_equal(&sock->foreign_addr, foreign_addr)) )
                        {
                            continue;
                        }
                    }
                }

                if((last != 0) && (last->receive_buffer.is_shutdown == FNET_FALSE))
                {
                    if((nb_tmp = _fnet_netbuf_copy(nb, 0u, FNET_NETBUF_COPYALL, FNET_FALSE)) != 0)
                    {
                        if(_fnet_socket_buffer_append_address(&(last->receive_buffer), nb_tmp, foreign_addr) == FNET_ERR)
                        {
                            _fnet_netbuf_free_chain(nb_tmp);
                        }
                    }
                }
                last = sock;
            }

            if(last == 0)
            {
                goto BAD;
            }

            if(last->receive_buffer.is_shutdown) /* Is shutdown.*/
            {
                goto BAD;
            }

            /* Copy buffer.*/
            if((nb_tmp = _fnet_netbuf_copy(nb, 0u, FNET_NETBUF_COPYALL, FNET_FALSE)) != 0)
            {
                if(_fnet_socket_buffer_append_address(&(last->receive_buffer), nb_tmp, foreign_addr) == FNET_ERR)
                {
                    _fnet_netbuf_free_chain(nb_tmp);
                    goto BAD;
                }
            }
            else
            {
                goto BAD;
            }
        }
        else /* For unicast datagram.*/
        {
            sock = _fnet_socket_lookup(fnet_raw_prot_if.head, local_addr, foreign_addr, protocol_number);

            if(sock)
            {
                if(sock->receive_buffer.is_shutdown) /* Is shutdown.*/
                {
                    goto BAD;
                }

                /* Copy buffer.*/
                if((nb_tmp = _fnet_netbuf_copy(nb, 0u, FNET_NETBUF_COPYALL, FNET_FALSE)) != 0)
                {
                    if(_fnet_socket_buffer_append_address(&(sock->receive_buffer), nb_tmp, foreign_addr) == FNET_ERR)
                    {
                        _fnet_netbuf_free_chain(nb_tmp);
                        goto BAD;
                    }
                }
                else
                {
                    goto BAD;
                }
            }
        }
    }
BAD:
    return;
}

/************************************************************************
* DESCRIPTION: RAW attach function.
*************************************************************************/
static fnet_return_t _fnet_raw_attach( fnet_socket_if_t *sk )
{
#if FNET_CFG_IP4
    sk->options.ip4_opt.ttl = FNET_RAW_TTL;
    sk->options.ip4_opt.tos = 0u;
#endif

#if FNET_CFG_IP6
    sk->options.ip6_opt.hops_unicast = 0u; /* Defined by ND.*/
#endif

    sk->send_buffer.count_max = FNET_RAW_TX_BUF_MAX;
    sk->receive_buffer.count_max = FNET_RAW_RX_BUF_MAX;
    return (FNET_OK);
}

/************************************************************************
* DESCRIPTION: RAW close function.
*************************************************************************/
static fnet_return_t _fnet_raw_detach( fnet_socket_if_t *sk )
{
    fnet_isr_lock();
    _fnet_socket_release(&fnet_raw_prot_if.head, sk);
    fnet_isr_unlock();
    return (FNET_OK);
}

/************************************************************************
* DESCRIPTION:  RAW shutdown function.
*************************************************************************/
static fnet_return_t _fnet_raw_shutdown( fnet_socket_if_t *sk, fnet_sd_flags_t how )
{
    fnet_isr_lock();

    if((how & SD_READ) != 0u)
    {
        sk->receive_buffer.is_shutdown = FNET_TRUE;
        _fnet_socket_buffer_release(&sk->receive_buffer);
    }

    if((how & SD_WRITE) != 0u)
    {
        sk->send_buffer.is_shutdown = FNET_TRUE;
    }

    fnet_isr_unlock();

    return (FNET_OK);
}

/************************************************************************
* DESCRIPTION: RAW connect function.
*************************************************************************/
static fnet_return_t _fnet_raw_connect( fnet_socket_if_t *sk, struct fnet_sockaddr *foreign_addr)
{
    fnet_isr_lock();

    fnet_memcpy(&sk->foreign_addr, foreign_addr, sizeof(sk->foreign_addr));
    sk->local_addr.sa_port = 0u;
    sk->foreign_addr.sa_port = 0u;
    sk->state = SS_CONNECTED;
    _fnet_socket_buffer_release(&sk->receive_buffer);

    fnet_isr_unlock();
    return (FNET_OK);
}

/************************************************************************
* DESCRIPTION: RAW send function.
*************************************************************************/
static fnet_int32_t _fnet_raw_snd( fnet_socket_if_t *sk, const fnet_uint8_t *buf, fnet_size_t len, fnet_flag_t flags, const struct fnet_sockaddr *addr)
{
    fnet_netbuf_t               *nb;
    fnet_error_t                error = FNET_ERR_OK;
    const struct fnet_sockaddr  *foreign_addr;
    fnet_bool_t                 flags_save = FNET_FALSE;

    fnet_isr_lock();

    if(len > sk->send_buffer.count_max)
    {
        error = FNET_ERR_MSGSIZE;   /* Message too long. */
        goto ERROR;
    }

    if(addr)
    {
        foreign_addr = addr;
    }
    else
    {
        foreign_addr = &sk->foreign_addr;
    }

    if((nb = _fnet_netbuf_from_buf(buf, len, FNET_FALSE)) == 0)
    {
        error = FNET_ERR_NOMEM;     /* Cannot allocate memory.*/
        goto ERROR;
    }

    if((flags & MSG_DONTROUTE) != 0u) /* Save */
    {
        flags_save = sk->options.so_dontroute;
        sk->options.so_dontroute = FNET_TRUE;
    }

    error = _fnet_raw_output(&sk->local_addr, foreign_addr, (fnet_uint8_t)sk->protocol_number, &(sk->options), nb);

    if((flags & MSG_DONTROUTE) != 0u) /* Restore.*/
    {
        sk->options.so_dontroute = flags_save;
    }

    if((error == FNET_ERR_OK) && (sk->options.local_error == FNET_ERR_OK)) /* We get RAW or ICMP error.*/
    {
        fnet_isr_unlock();
        return (fnet_int32_t)(len);
    }

ERROR:
    _fnet_socket_set_error(sk, error);
    fnet_isr_unlock();
    return (FNET_ERR);
}

/************************************************************************
* DESCRIPTION :RAW receive function.
*************************************************************************/
static fnet_int32_t _fnet_raw_rcv(fnet_socket_if_t *sk, fnet_uint8_t *buf, fnet_size_t len, fnet_flag_t flags, struct fnet_sockaddr *addr)
{
    fnet_error_t            error = FNET_ERR_OK;
    fnet_int32_t            length;
    struct fnet_sockaddr    foreign_addr;

    fnet_memset_zero ((void *)&foreign_addr, sizeof(foreign_addr));

#if FNET_CFG_TCP_URGENT
    if(flags & MSG_OOB)
    {
        error = FNET_ERR_OPNOTSUPP; /* Operation not supported.*/
        goto ERROR;
    }
#endif /* FNET_CFG_TCP_URGENT */

    if((length = _fnet_socket_buffer_read_address(&(sk->receive_buffer), buf,
                 len, &foreign_addr, ((flags & MSG_PEEK) == 0u) ? FNET_TRUE : FNET_FALSE)) == FNET_ERR)
    {
        /* The message was too large to fit into the specified buffer and was truncated.*/
        error = FNET_ERR_MSGSIZE;
        goto ERROR;
    }

    if((error == FNET_ERR_OK) && (sk->options.local_error == FNET_ERR_OK)) /* We get RAW or ICMP error.*/
    {
        if(addr)
        {
            _fnet_socket_addr_copy(&foreign_addr, addr);
        }

        return (length);
    }

ERROR:
    _fnet_socket_set_error(sk, error);
    return (FNET_ERR);
}

#endif  /* FNET_CFG_RAW */
