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
*  TCP protocol implementation.
*
***************************************************************************/

#include "fnet.h"

#if FNET_CFG_TCP

#include "fnet_socket_prv.h"
#include "fnet_timer_prv.h"
#include "fnet_tcp.h"
#include "fnet_checksum_prv.h"
#include "fnet_prot.h"
#include "fnet_ip_prv.h"

/************************************************************************
*     Definitions
*************************************************************************/
struct fnet_tcp_segment
{
    fnet_socket_option_t    *sockoption;
    struct fnet_sockaddr    src_addr;
    struct fnet_sockaddr    dest_addr;
    fnet_uint32_t           seq;
    fnet_uint32_t           ack;
    fnet_uint8_t            flags;
    fnet_uint16_t           wnd;
    fnet_uint16_t           urgpointer;
    void                    *options;
    fnet_uint8_t            optlen;
    fnet_netbuf_t           *data;
};

/************************************************************************
*     Function Prototypes
*************************************************************************/
static void _fnet_tcp_slowtimo( fnet_uint32_t cookie );
static void _fnet_tcp_fasttimo( fnet_uint32_t cookie );
static void _fnet_tcp_slowtimosk( fnet_socket_if_t *sk );
static void _fnet_tcp_fasttimosk( fnet_socket_if_t *sk );
static fnet_bool_t _fnet_tcp_inputsk( fnet_socket_if_t *sk, fnet_netbuf_t *insegment, struct fnet_sockaddr *src_addr,  struct fnet_sockaddr *dest_addr);
static void _fnet_tcp_init_connection( fnet_socket_if_t *sk );
static fnet_bool_t _fnet_tcp_dataprocess( fnet_socket_if_t *sk, fnet_netbuf_t *insegment, fnet_flag_t *ackparam );
static fnet_error_t _fnet_tcp_send_headseg( fnet_socket_if_t *sk, fnet_uint8_t flags, void *options, fnet_uint8_t optlen );
static fnet_error_t _fnet_tcp_send_dataseg( fnet_socket_if_t *sk, void *options, fnet_uint8_t optlen, fnet_size_t datasize );
static fnet_uint32_t _fnet_tcp_get_rcvwnd( fnet_socket_if_t *sk );
static fnet_error_t _fnet_tcp_send_seg( struct fnet_tcp_segment *segment);
static void _fnet_tcp_send_rst( fnet_socket_option_t *sockoption, fnet_netbuf_t *insegment, struct fnet_sockaddr *src_addr, struct fnet_sockaddr *dest_addr);
static void _fnet_tcp_send_rstsk( fnet_socket_if_t *sk );
static void _fnet_tcp_send_ack( fnet_socket_if_t *sk );
static void _fnet_tcp_abort_socket( fnet_socket_if_t *sk );
static void _fnet_tcp_set_synopt( fnet_socket_if_t *sk, fnet_uint8_t *options, fnet_uint8_t *optionlen );
static void _fnet_tcp_get_synopt( fnet_socket_if_t *sk );
static fnet_error_t _fnet_tcp_addopt( fnet_netbuf_t *segment, fnet_size_t len, void *data );
static void _fnet_tcp_get_opt( fnet_socket_if_t *sk, fnet_netbuf_t *segment );
static fnet_uint32_t _fnet_tcp_get_size( fnet_uint32_t pos1, fnet_uint32_t pos2 );
static void _fnet_tcp_retransmission_timeo( fnet_socket_if_t *sk );
static void _fnet_tcp_keepalive_timeo( fnet_socket_if_t *sk );
static void _fnet_tcp_persist_timeo( fnet_socket_if_t *sk );
static fnet_bool_t _fnet_tcp_hit( fnet_uint32_t startpos, fnet_uint32_t endpos, fnet_uint32_t pos );
static fnet_bool_t _fnet_tcp_add_inpbuf( fnet_socket_if_t *sk, fnet_netbuf_t *insegment, fnet_flag_t *ackparam );
static fnet_socket_if_t *_fnet_tcp_find_socket( struct fnet_sockaddr *src_addr, struct fnet_sockaddr *dest_addr );
static void _fnet_tcp_add_partial_socket( fnet_socket_if_t *mainsk, fnet_socket_if_t *partialsk );
static void _fnet_tcp_move_socket2incominglist( fnet_socket_if_t *sk );
static void _fnet_tcp_close_socket( fnet_socket_if_t *sk );
static void _fnet_tcp_del_partialsocket( fnet_socket_if_t *sk );
static void _fnet_tcp_del_incomingsocket( fnet_socket_if_t *sk );
static void _fnet_tcp_del_cb( fnet_tcp_control_t *cb );
#if !FNET_CFG_TCP_DISCARD_OUT_OF_ORDER
    static void _fnet_tcp_del_tmpbuf( fnet_tcp_control_t *cb );
#endif
static void _fnet_tcp_del_socket( fnet_socket_if_t **head, fnet_socket_if_t *sk );
static fnet_bool_t _fnet_tcp_send_anydata( fnet_socket_if_t *sk, fnet_bool_t oneexec );
#if FNET_CFG_TCP_URGENT
    static void _fnet_tcp_process_urg( fnet_socket_if_t *sk, fnet_netbuf_t **segment, fnet_size_t repdatasize, fnet_flag_t *ackparam );
#endif
static void _fnet_tcp_process_fin( fnet_socket_if_t *sk, fnet_uint32_t ack );
static fnet_return_t _fnet_tcp_init( void );
static void _fnet_tcp_release( void );
static void _fnet_tcp_input(fnet_netif_t *netif, struct fnet_sockaddr *src_addr, struct fnet_sockaddr *dest_addr, fnet_netbuf_t *nb, fnet_netbuf_t *ip_nb);
static void _fnet_tcp_control_input(fnet_prot_notify_t command, struct fnet_sockaddr *src_addr, struct fnet_sockaddr *dest_addr, fnet_netbuf_t *nb);
static fnet_return_t _fnet_tcp_attach( fnet_socket_if_t *sk );
static fnet_return_t _fnet_tcp_close( fnet_socket_if_t *sk );
static fnet_return_t _fnet_tcp_connect( fnet_socket_if_t *sk, struct fnet_sockaddr *foreign_addr);
static fnet_socket_if_t *_fnet_tcp_accept( fnet_socket_if_t *listensk );
static fnet_ssize_t _fnet_tcp_rcv( fnet_socket_if_t *sk, fnet_uint8_t *buf, fnet_size_t len, fnet_flag_t flags, struct fnet_sockaddr *foreign_addr);
static fnet_ssize_t _fnet_tcp_snd( fnet_socket_if_t *sk, const fnet_uint8_t *buf, fnet_size_t len, fnet_flag_t flags, const struct fnet_sockaddr *foreign_addr);
static fnet_return_t _fnet_tcp_shutdown( fnet_socket_if_t *sk, fnet_sd_flags_t how );
static fnet_return_t _fnet_tcp_setsockopt( fnet_socket_if_t *sk, fnet_protocol_t level, fnet_socket_options_t optname, const void *optval, fnet_size_t optlen );
static fnet_return_t _fnet_tcp_getsockopt( fnet_socket_if_t *sk, fnet_protocol_t level, fnet_socket_options_t optname, void *optval, fnet_size_t *optlen );
static fnet_return_t _fnet_tcp_listen( fnet_socket_if_t *sk, fnet_size_t backlog );
static void _fnet_tcp_drain( void );
static void _fnet_tcp_initial_seq_number_update( void );

#if FNET_CFG_DEBUG_TRACE_TCP && FNET_CFG_DEBUG_TRACE
    void _fnet_tcp_trace(const fnet_char_t *str, fnet_tcp_header_t *tcp_hdr);
#else
    #define _fnet_tcp_trace(str, tcp_hdr)    do{}while(0)
#endif

#if 0 /* For Debug needs.*/
    fnet_return_t FNET_DEBUG_check_send_buffer(fnet_socket_if_t *sk);
#endif

/************************************************************************
*     Global Variables
*************************************************************************/
/* Initial Sequence Number
 * tcpcb_isntime is changed by STEPISN every 0.5 sec.
 * Additionaly, each time a connection is established,
 * tcpcb_isntime is also incremented by FNET_TCP_INITIAL_SEQ_NUMBER_STEP + random value */
static fnet_uint32_t _fnet_tcp_initial_seq_number;

/* Timers.*/
static fnet_timer_desc_t fnet_tcp_fasttimer;
static fnet_timer_desc_t fnet_tcp_slowtimer;

/*****************************************************************************
 * Protocol API structure.
 ******************************************************************************/
static const fnet_socket_prot_if_t fnet_tcp_socket_api =
{
    .con_req = FNET_TRUE,                   /* TRUE = connection required by protocol.*/
    .prot_attach = _fnet_tcp_attach,
    .prot_detach = _fnet_tcp_close,
    .prot_connect = _fnet_tcp_connect,
    .prot_accept = _fnet_tcp_accept,
    .prot_rcv = _fnet_tcp_rcv,
    .prot_snd = _fnet_tcp_snd,
    .prot_shutdown = _fnet_tcp_shutdown,
    .prot_setsockopt = _fnet_tcp_setsockopt,
    .prot_getsockopt = _fnet_tcp_getsockopt,
    .prot_listen = _fnet_tcp_listen
};

/* Protocol structure.*/
fnet_prot_if_t fnet_tcp_prot_if =
{
    .family = AF_SUPPORTED,             /* Protocol domain.*/
    .type = SOCK_STREAM,                /* Socket type.*/
    .protocol = FNET_PROT_TCP,          /* Protocol number.*/
    .prot_init = _fnet_tcp_init,         /* Protocol initialization function.*/
    .prot_release = _fnet_tcp_release,   /* Protocol release function.*/
    .prot_input = _fnet_tcp_input,       /* Protocol input function.*/
    .prot_control_input = _fnet_tcp_control_input, /* Control input */
    .prot_drain = _fnet_tcp_drain,       /* Protocol drain function. */
    .socket_api = &fnet_tcp_socket_api  /* Socket API */
};

/************************************************************************
* DESCRIPTION: This function performs a protocol initialization.
*
* RETURNS: If no error occurs, this function returns ERR_OK. Otherwise
*          it returns FNET_ERR.
*************************************************************************/
static fnet_return_t _fnet_tcp_init( void )
{
    /* Create the fast timer.*/
    fnet_tcp_fasttimer = _fnet_timer_new(FNET_TCP_FAST_TIMER_PERIOD_MS, _fnet_tcp_fasttimo, 0u);

    if(!fnet_tcp_fasttimer)
    {
        return FNET_ERR;
    }

    /* Create the slow timer.*/
    fnet_tcp_slowtimer = _fnet_timer_new(FNET_TCP_SLOW_TIMER_PERIOD_MS, _fnet_tcp_slowtimo, 0u);

    if(!fnet_tcp_slowtimer)
    {
        _fnet_timer_free(fnet_tcp_fasttimer);
        fnet_tcp_fasttimer = 0;
        return FNET_ERR;
    }

    /* Initialize ISN */
    _fnet_tcp_initial_seq_number_update();

    return FNET_OK;
}

/************************************************************************
* DESCRIPTION: This function resets and deletes sockets and releases timers.
*************************************************************************/
static void _fnet_tcp_release( void )
{
    fnet_tcp_control_t *cb;

    fnet_isr_lock();

    /* Release sockets.*/
    while(fnet_tcp_prot_if.head)
    {
        cb = (fnet_tcp_control_t *)fnet_tcp_prot_if.head->protocol_control;
        cb->tcpcb_flags |= FNET_TCP_CBF_CLOSE;
        _fnet_tcp_abort_socket(fnet_tcp_prot_if.head);
    }

    /* Free timers.*/
    _fnet_timer_free(fnet_tcp_fasttimer);
    _fnet_timer_free(fnet_tcp_slowtimer);

    fnet_tcp_fasttimer = 0;
    fnet_tcp_slowtimer = 0;

    fnet_isr_unlock();
}

/************************************************************************
* DESCRIPTION: This function receives, checks and routes input segments.
*
* RETURNS: FNET_OK.
*************************************************************************/
static void _fnet_tcp_input(fnet_netif_t *netif, struct fnet_sockaddr *src_addr, struct fnet_sockaddr *dest_addr, fnet_netbuf_t *nb, fnet_netbuf_t *ip_nb)
{
    fnet_socket_if_t   *sk;
    fnet_uint16_t   checksum;
    fnet_size_t     tcp_length;

    tcp_length = (fnet_size_t)FNET_TCP_LENGTH(nb);

    _fnet_netbuf_free_chain(ip_nb);

    /* The header must reside in contiguous area of the memory.*/
    if(_fnet_netbuf_pullup(&nb, tcp_length) == FNET_ERR)
    {
        goto DROP;
    }

    /*Checksum.*/
#if FNET_CFG_CPU_ETH_HW_RX_PROTOCOL_CHECKSUM || FNET_CFG_CPU_ETH_HW_TX_PROTOCOL_CHECKSUM
    if(nb->flags & FNET_NETBUF_FLAG_HW_PROTOCOL_CHECKSUM)
    {
        checksum = 0;
    }
    else
#endif
    {
        checksum = _fnet_checksum_pseudo_netbuf_start( nb, FNET_HTONS((fnet_uint16_t)FNET_PROT_TCP), (fnet_uint16_t)nb->total_length );
        checksum = _fnet_checksum_pseudo_netbuf_end( checksum, &src_addr->sa_data[0], &dest_addr->sa_data[0],
                   (fnet_size_t)((dest_addr->sa_family == AF_INET) ? sizeof(fnet_ip4_addr_t) : sizeof(fnet_ip6_addr_t)) );
    }

    if(checksum
       || (_fnet_socket_addr_is_broadcast(src_addr, netif))
       || (fnet_socket_addr_is_multicast(src_addr))
       || (nb->total_length < tcp_length) /* Check the length.*/)
    {
        goto DROP;
    }

    _fnet_tcp_trace("RX", nb->data_ptr); /* TCP trace.*/

    /* Set port numbers.*/
    src_addr->sa_port = FNET_TCP_SPORT(nb);
    dest_addr->sa_port = FNET_TCP_DPORT(nb);

    if((_fnet_socket_addr_is_broadcast(dest_addr, netif)) || (fnet_socket_addr_is_multicast(dest_addr)))
    {
        /* Send RST.*/
        _fnet_tcp_send_rst(0, nb, dest_addr, src_addr);
        goto DROP;
    }

    sk = _fnet_tcp_find_socket(src_addr,  dest_addr);

    if(sk)
    {
        fnet_bool_t drop_flag;

        if(sk->state == SS_LISTENING)
        {
            fnet_memcpy(&sk->foreign_addr, src_addr, sizeof(sk->foreign_addr));
        }

        nb->next_chain = 0;

        /* Process  the segment.*/
        drop_flag = _fnet_tcp_inputsk(sk, nb, src_addr, dest_addr);

        /* Wake-up user application.*/
#if FNET_CFG_SOCKET_CALLBACK_ON_RX
        fnet_event_raise(fnet_socket_event_rx);
#endif

        if(drop_flag == FNET_TRUE)
        {
            goto DROP;
        }
    }
    else
    {
        if((FNET_TCP_FLAGS(nb) & FNET_TCP_SGT_RST) == 0u)
        {
            _fnet_tcp_send_rst(0, nb, dest_addr, src_addr);
        }

        goto DROP;
    }

    return;

DROP:

    /* Delete the segment.*/
    _fnet_netbuf_free_chain(nb);
}

/************************************************************************
* DESCRIPTION: This function process ICMP errors.
*************************************************************************/
static void _fnet_tcp_control_input(fnet_prot_notify_t command, struct fnet_sockaddr *src_addr, struct fnet_sockaddr *dest_addr, fnet_netbuf_t *nb)
{
    fnet_tcp_header_t   *tcp_header;    /* Pointer to the TCP header.*/
    fnet_socket_if_t    *sk;            /* Pointer to the socket.*/
    fnet_tcp_control_t  *cb;

    if(src_addr && dest_addr && nb)
    {
        tcp_header = (fnet_tcp_header_t *)nb->data_ptr;

        /* Find the corresponding socket.*/
        dest_addr->sa_port = tcp_header->destination_port;
        src_addr->sa_port = tcp_header->source_port;

        sk = _fnet_tcp_find_socket(dest_addr, src_addr);

        if(sk)
        {
            /* Initialize the pointer of the control block.*/
            cb = (fnet_tcp_control_t *)sk->protocol_control;

            switch(command)
            {
                case FNET_PROT_NOTIFY_QUENCH: /* Someone said to slow down.*/
                    /* Begin the Slow Start algorithm.*/
                    cb->tcpcb_cwnd = cb->tcpcb_sndmss;
                    break;
                case FNET_PROT_NOTIFY_MSGSIZE:          /* Message size forced drop.*/
                    sk->options.local_error = FNET_ERR_MSGSIZE;
                    break;
                case FNET_PROT_NOTIFY_UNREACH_HOST:     /* No route to host.*/
                case FNET_PROT_NOTIFY_UNREACH_NET:      /* No route to network.*/
                case FNET_PROT_NOTIFY_UNREACH_SRCFAIL:  /* Source route failed.*/
                    sk->options.local_error = FNET_ERR_HOSTUNREACH;
                    break;
                case FNET_PROT_NOTIFY_PARAMPROB:                 /* Header incorrect.*/
                    sk->options.local_error = FNET_ERR_NOPROTOOPT; /* Bad protocol option.*/
                    break;
                case FNET_PROT_NOTIFY_UNREACH_PORT:              /* bad port #.*/
                case FNET_PROT_NOTIFY_UNREACH_PROTOCOL:
                    if(sk->state != SS_LISTENING) /* Avoid listening sockets.*/
                    {
                        sk->options.local_error = FNET_ERR_CONNRESET;
                        _fnet_tcp_close_socket(sk);
                    }
                    break;
                default:
                    break;
            }
        }
    }
}

/************************************************************************
* DESCRIPTION: This function performs the initialization of the
*              socket's options and creates the structure of the control blok.
*
* RETURNS: If no error occurs, this function returns FNET_OK. Otherwise
*          it returns FNET_ERR.
*************************************************************************/
static fnet_return_t _fnet_tcp_attach( fnet_socket_if_t *sk )
{
    struct tcpcb *cb;

    /* Create the control block.*/
    cb = (struct tcpcb *)_fnet_malloc_zero(sizeof(fnet_tcp_control_t));

    /* Check the memory allocation.*/
    if(!cb)
    {
        _fnet_socket_set_error(sk, FNET_ERR_NOMEM);
        return FNET_ERR;
    }

    sk->protocol_control = (void *)cb;

    /* Set the maximal segment size option.*/
    sk->options.tcp_opt.mss = FNET_CFG_SOCKET_TCP_MSS;

    /* Default setting of the flags.*/
    sk->options.tcp_opt.tcp_nodelay = FNET_TRUE;
#if FNET_CFG_TCP_URGENT
    sk->options.tcp_opt.tcp_bsd = FNET_TRUE;
#endif
    sk->options.tcp_opt.keep_idle = FNET_TCP_KEEPIDLE_DEFAULT;      /* TCP_KEEPIDLE option. */
    sk->options.tcp_opt.keep_intvl = FNET_TCP_KEEPINTVL_DEFAULT;    /* TCP_KEEPINTVL option. */
    sk->options.tcp_opt.keep_cnt = FNET_TCP_KEEPCNT_DEFAULT;        /* TCP_KEEPCNT option. */

    sk->options.so_dontroute = FNET_FALSE;
    sk->options.so_keepalive = FNET_TRUE;
    sk->options.so_linger = FNET_FALSE;
#if FNET_CFG_TCP_URGENT
    sk->options.so_oobinline = FNET_FALSE;
#endif

    /* Set the IP options.*/
#if FNET_CFG_IP4
    sk->options.ip4_opt.ttl = FNET_TCP_TTL_DEFAULT;
    sk->options.ip4_opt.tos = 0u;
#endif

#if FNET_CFG_IP6
    sk->options.ip6_opt.hops_unicast = 0u; /* Defined by ND.*/
#endif

    /* Set the buffer sizes.*/
    sk->send_buffer.count_max = FNET_TCP_TX_BUF_MAX;
    sk->receive_buffer.count_max = FNET_TCP_RX_BUF_MAX;

    return FNET_OK;
}

/************************************************************************
* DESCRIPTION: This function updates TCP ISN.
*************************************************************************/
static void _fnet_tcp_initial_seq_number_update( void )
{
    /* Increase Initial Sequence Number. */
    _fnet_tcp_initial_seq_number += FNET_TCP_INITIAL_SEQ_NUMBER_STEP + (fnet_rand() & 0xFFu);
}
/************************************************************************
* DESCRIPTION: This function performs the connection termination.
*
* RETURNS: If no error occurs, this function returns FNET_OK. Otherwise
*          it returns FNET_ERR.
*************************************************************************/
static fnet_return_t _fnet_tcp_close( fnet_socket_if_t *sk )
{
    fnet_tcp_control_t *cb = (fnet_tcp_control_t *)sk->protocol_control;

    /* If the connection is closed, free the memory.*/
    if(sk->state == SS_CLOSED)
    {
        cb->tcpcb_flags |= FNET_TCP_CBF_CLOSE;
        _fnet_tcp_close_socket(sk);
        return FNET_OK;
    }

    /* If SO_LINGER option is present.*/
    if(sk->options.so_linger == FNET_TRUE)
    {
        if(sk->options.linger_ms == 0u)
            /* Linger is 0 so close the socket immediately. */
        {
            /* Hard reset.*/
            fnet_isr_lock();
            cb->tcpcb_flags |= FNET_TCP_CBF_CLOSE;
            _fnet_tcp_abort_socket(sk);
            fnet_isr_unlock();
            return FNET_OK;
        }
    }

    fnet_isr_lock();

    if(sk->state != SS_CONNECTED)
    {
        cb->tcpcb_flags |= FNET_TCP_CBF_CLOSE;
        _fnet_tcp_abort_socket(sk);

        /* Unlock interrupts.*/
        fnet_isr_unlock();
        return FNET_OK;
    }

    /* If the socket is not unlocked, try to send the data.*/
    if(sk->send_buffer.is_shutdown == FNET_FALSE)
    {
        sk->send_buffer.is_shutdown = FNET_TRUE;
        _fnet_tcp_send_anydata(sk, FNET_TRUE);
    }

    fnet_isr_unlock();

    fnet_isr_lock();

    /* After this moment the unconnecetd socket must be deleted.*/
    cb->tcpcb_flags |= FNET_TCP_CBF_CLOSE;

    /* If the socket is already unconnected, close the socket.*/
    if(sk->state == SS_CLOSED)
    {
        _fnet_tcp_close_socket(sk);
    }
    else
    {
        if(cb->tcpcb_connection_state != FNET_TCP_CS_TIME_WAIT)
        {
            if((sk->options.so_linger == FNET_TRUE) && (sk->options.linger_ms))
            {
                cb->tcpcb_timers.connection = sk->options.linger_ms / FNET_TCP_SLOW_TIMER_PERIOD_MS;
            }
            else
            {
                cb->tcpcb_timers.connection = FNET_TCP_ABORT_INTERVAL;
            }

            sk->receive_buffer.is_shutdown = FNET_TRUE;
        }

        if(sk->receive_buffer.count)
        {
            _fnet_socket_buffer_release(&sk->receive_buffer);
        }
    }

    fnet_isr_unlock();

    return FNET_OK;
}

/************************************************************************
* DESCRIPTION: This function performs the connection establishment.
*
* RETURNS: If no error occurs, this function returns FNET_OK. Otherwise
*          it returns FNET_ERR.
*************************************************************************/
static fnet_return_t _fnet_tcp_connect( fnet_socket_if_t *sk, struct fnet_sockaddr *foreign_addr)
{
    fnet_tcp_control_t  *cb;
    fnet_uint8_t        options[FNET_TCP_MAX_OPT_SIZE];
    fnet_uint8_t        optionlen;
    fnet_error_t        error;
    fnet_netif_t        *netif;

    if((netif = _fnet_socket_addr_route(foreign_addr)) == FNET_NULL)
    {
        error = FNET_ERR_NETUNREACH;
        goto ERROR;
    }

    /* TCP doesn't support broadcasting and multicasting.*/
    if((_fnet_socket_addr_is_broadcast(foreign_addr, netif)) || (fnet_socket_addr_is_multicast(foreign_addr)))
    {
        error = FNET_ERR_ADDRNOTAVAIL;
        goto ERROR;
    }

    /* Initialize the pointer to the control block.*/
    cb = (fnet_tcp_control_t *)sk->protocol_control;

    /* Initialize the control block.*/
    _fnet_tcp_init_connection(sk);

    /* Set synchronized options.*/
    _fnet_tcp_set_synopt(sk, options, &optionlen);

    /* Initialize sequnece number parameters.*/
    cb->tcpcb_sndseq = _fnet_tcp_initial_seq_number;
    cb->tcpcb_maxrcvack = _fnet_tcp_initial_seq_number + 1u;
#if FNET_CFG_TCP_URGENT
    cb->tcpcb_sndurgseq = cb->tcpcb_sndseq - 1;
#endif /* FNET_CFG_TCP_URGENT */

    /* Set the foreign address.*/
    sk->foreign_addr = *foreign_addr;

    fnet_isr_lock();

    /* Send SYN segment.*/
    error = _fnet_tcp_send_headseg(sk, FNET_TCP_SGT_SYN, options, optionlen);

    /* Check the result.*/
    if(error)
    {
        fnet_isr_unlock();
        goto ERROR;
    }

    /* Change the states.*/
    cb->tcpcb_connection_state = FNET_TCP_CS_SYN_SENT;
    sk->state = SS_CONNECTING;

    /* Increase Initial Sequence Number.*/
    _fnet_tcp_initial_seq_number_update();

    /* Initialize Abort Timer.*/
    cb->tcpcb_timers.retransmission = cb->tcpcb_rto;
    cb->tcpcb_timers.connection = FNET_TCP_ABORT_INTERVAL_CON;

    fnet_isr_unlock();

    return FNET_OK;

ERROR:
    _fnet_socket_set_error(sk, error);
    return FNET_ERR;
}

/************************************************************************
* DESCRIPTION: This function removes the created socket
*              from the incoming queue of the listening socket,
*              and adds this socket to the main list.
*
* RETURNS: If the incoming socket is present, this function returns
*          the first created incoming socket. Otherwise, it returns 0.
*************************************************************************/
static fnet_socket_if_t *_fnet_tcp_accept( fnet_socket_if_t *listensk )
{
    fnet_socket_if_t *sk = FNET_NULL;

    /* If the incoming socket is not present, return.*/
    if(listensk->incoming_con)
    {
        fnet_isr_lock();
        /* Find the first incoming socket.*/
        sk = listensk->incoming_con;

        while(sk->next)
        {
            sk = sk->next;
        }

        /* Delete the incoming socket from the list.*/
        sk->head_con->incoming_con_len--;
        _fnet_socket_list_del(&sk->head_con->incoming_con, sk);
        sk->head_con = 0;

        fnet_isr_unlock();
    }

    return sk;
}

/************************************************************************
* DESCRIPTION: This function receives the data and sends the acknowledgment
*              (This acknowledgment segment informs about the new free size
*               in the input buffer).
*
* RETURNS: If no error occurs, this function returns the length
*          of the received data. Otherwise, it returns FNET_ERR.
*************************************************************************/
static fnet_ssize_t _fnet_tcp_rcv( fnet_socket_if_t *sk, fnet_uint8_t *buf, fnet_size_t len, fnet_flag_t flags, struct fnet_sockaddr *foreign_addr)
{
    fnet_tcp_control_t  *cb = (fnet_tcp_control_t *)sk->protocol_control;
    fnet_bool_t         flag_remove; /* Remove flag. 1 means that the data must be deleted
                                 * from the input buffer after the reading.*/
    fnet_error_t        error_code;

    /* Receive the flags.*/
    flag_remove = ((flags & MSG_PEEK) == 0u) ? FNET_TRUE : FNET_FALSE;

#if FNET_CFG_TCP_URGENT
    /* Reading of the OOB data.*/
    if(flags & MSG_OOB)
    {
        /* If the OOB data can't be read, return.*/
        if(sk->options.so_oobinline == FNET_TRUE)
        {
            error_code = FNET_ERR_INVAL;
            goto ERROR;
        }

        /* If the OOB data is present, read */
        if(cb->tcpcb_flags & FNET_TCP_CBF_RCVURGENT)
        {
            fnet_isr_lock();

            if(flag_remove)
            {
                cb->tcpcb_rcvurgmark = FNET_TCP_NOT_USED;
                cb->tcpcb_flags &= ~FNET_TCP_CBF_RCVURGENT;
            }

            *buf = cb->tcpcb_iobc;
            fnet_isr_unlock();

            return FNET_TCP_URGENT_DATA_SIZE;
        }
        else
        {
            /* If the socket is not connected , return with the error. */
            if(sk->state != SS_CONNECTED)
            {
                error_code = FNET_ERR_NOTCONN;
                goto ERROR;
            }

            return 0;
        }
    }
#endif /* FNET_CFG_TCP_URGENT */

    fnet_isr_lock();

#if FNET_CFG_TCP_URGENT
    /* Calculate the length of the data that can be received.*/
    if(cb->tcpcb_rcvurgmark > 0 && len >= cb->tcpcb_rcvurgmark)
    {
        len = cb->tcpcb_rcvurgmark;

        if(flag_remove)
        {
            cb->tcpcb_rcvurgmark = FNET_TCP_NOT_USED;
        }
    }
    else
#endif /* FNET_CFG_TCP_URGENT */
        if(sk->receive_buffer.count < len)
        {
            len = sk->receive_buffer.count;
        }

    /* Copy the data to the buffer.*/
    len = _fnet_socket_buffer_read_record(&sk->receive_buffer, buf, len, flag_remove);

    /* Remove the data from input buffer.*/
    if(flag_remove)
    {
        /* Recalculate the new free size in the input buffer.*/
        cb->tcpcb_newfreercvsize += len;

        /* If the window is opened, send acknowledgment.*/
        if(( ((cb->tcpcb_newfreercvsize) >= ((fnet_uint32_t)cb->tcpcb_rcvmss << 1))
             || (cb->tcpcb_newfreercvsize >= (cb->tcpcb_rcvcountmax >> 1)) /* More than half of RX buffer.*/
             || ((!cb->tcpcb_rcvwnd) && ( cb->tcpcb_newfreercvsize))) && (sk->state == SS_CONNECTED))
        {
            _fnet_tcp_send_ack(sk);
        }
    }

    /* If the socket is not connected and the data are not received, return with error.*/
    if((len == 0u) && (sk->state != SS_CONNECTED))
    {
        error_code = FNET_ERR_NOTCONN;
        goto ERROR_UNLOCK;
    }
    else
    {
        /* Set the foreign address and port.*/
        if(foreign_addr)
        {
            fnet_memcpy(foreign_addr, &sk->foreign_addr, sizeof(*foreign_addr));
        }

        /* If the socket is closed by peer and no data.*/
        if((len == 0u) && ((cb->tcpcb_flags & FNET_TCP_CBF_FIN_RCVD) != 0u))
        {
            error_code = FNET_ERR_CONNCLOSED;
            goto ERROR_UNLOCK;
        }
    }

    fnet_isr_unlock();
    return (fnet_int32_t)len;

ERROR_UNLOCK:
    fnet_isr_unlock();
#if FNET_CFG_TCP_URGENT
ERROR:
#endif
    _fnet_socket_set_error(sk, error_code);
    return FNET_ERR;
}

/************************************************************************
* DESCRIPTION: This function adds the data to the output buffer and
*              sends the data that can be sent.
*
* RETURNS: If no error occurs, this function returns the length
*          of the data that is added to the output buffer.
*          Otherwise, it returns FNET_ERR.
*************************************************************************/
static fnet_ssize_t _fnet_tcp_snd( fnet_socket_if_t *sk, const fnet_uint8_t *buf, fnet_size_t len, fnet_flag_t flags, const struct fnet_sockaddr *foreign_addr)
{
    fnet_tcp_control_t  *cb = (fnet_tcp_control_t *)sk->protocol_control;
    fnet_netbuf_t       *netbuf;
    fnet_size_t         sendlength = len;   /* Size of the data that must be sent.*/
    fnet_size_t         sentlength = 0u;    /* Length of the sent data.*/
    fnet_size_t         freespace;          /* Free space in the output buffer.*/
    fnet_size_t         currentlen;         /* Current length.*/
    fnet_bool_t         dontroute = FNET_FALSE;      /* Routing flag.*/
    fnet_size_t         malloc_max;
    fnet_error_t        error_code;

    FNET_COMP_UNUSED_ARG(foreign_addr);

    /* If the size of the data greater than the maximal size of the output buffer, return*/
    if(sendlength > FNET_TCP_MAX_BUFFER)
    {
        error_code = FNET_ERR_INVAL;
        goto ERROR;
    }

    /* If the socket is not connected, return*/
    if(sk->state != SS_CONNECTED)
    {
        error_code = FNET_ERR_NOTCONN;
        goto ERROR;
    }

    if(sendlength)
    {
        fnet_isr_lock();

        /* Caclulate a free space in the output buffer.*/
        freespace = (sk->send_buffer.count_max - sk->send_buffer.count);

        /* Check maximum allocated memory chunck */
        malloc_max = _fnet_malloc_max_netbuf();

        if(malloc_max < (fnet_size_t) (FNET_ETH_MTU + FNET_ETH_MTU / 2u)) /* TBD I do not like it ????*/
        {
            freespace = 0u;
        }
        else if(freespace > malloc_max)
        {
            freespace = malloc_max;
        }
        else
        {}

        /* If the function is nonblocking and the data length greater than the freespace, recalculate the size of the data*/
        if(freespace < sendlength)
        {
            /* If the data can't be added to the output buffer, return*/
            if(freespace == 0u)
            {
                fnet_isr_unlock();
                return 0;
            }
            else
            {
                /* Recalculate the data size.*/
                sendlength = freespace;
#if FNET_CFG_TCP_URGENT
                flags &= ~MSG_OOB;
#endif /* FNET_CFG_TCP_URGENT */
            }
        }

#if FNET_CFG_TCP_URGENT
        /* Process the OOB data.*/
        if(flags & MSG_OOB)
        {
            /* If the urgent data are already present, the urgent byte will not be added.*/
            if(FNET_TCP_COMP_GE(cb->tcpcb_sndurgseq, cb->tcpcb_rcvack))
            {
                sendlength--;

                /* If the  data size is 0, return.*/
                if(!sendlength)
                {
                    fnet_isr_unlock();
                    return 0;
                }
            }
            else
            {
                /* Calculate the new sequence number of the urgent data.*/
                cb->tcpcb_sndurgseq = cb->tcpcb_rcvack + sk->send_buffer.count + sendlength - 1;
            }

        }
#endif /* FNET_CFG_TCP_URGENT */

        /* If the routing tables should be bypassed for this message only, set dontroute flag.*/
        if(((flags & MSG_DONTROUTE) != 0u) && (sk->options.so_dontroute == FNET_FALSE) )
        {
            dontroute = FNET_TRUE;
            sk->options.so_dontroute = FNET_TRUE;
        }

        /* Try to add the data.*/
        if(freespace > 0u)
        {
            cb->tcpcb_flags |= FNET_TCP_CBF_INSND;

            /* Calculate the data size that can be added.*/
            if(sendlength > freespace)
            {
                currentlen = freespace;
            }
            else
            {
                currentlen = sendlength;
            }

            netbuf = _fnet_netbuf_from_buf(&buf[sentlength], currentlen, FNET_TRUE);

            /* Check the memory allocation.*/
            if(netbuf)
            {
                sentlength += currentlen;

                if(_fnet_socket_buffer_append_record(&sk->send_buffer, netbuf) == FNET_OK)
                {
                    /* If the window of another side is closed, set the persist timer.*/
                    if(!cb->tcpcb_sndwnd)
                    {
                        if(cb->tcpcb_timers.persist == FNET_TCP_TIMER_OFF)
                        {
                            cb->tcpcb_cprto = cb->tcpcb_rto;
                            cb->tcpcb_timers.persist = cb->tcpcb_cprto;
                        }
                    }
                    else
                    {
                        fnet_flag_t sendanydata = FNET_TRUE;

                        /* Try to send the data.*/
                        while(sendanydata)
                        {
                            /* If the connection is not established, delete the data. Otherwise try to send the data*/
                            if(sk->state == SS_CONNECTED)
                            {
                                if(!_fnet_tcp_send_anydata(sk, FNET_TRUE))
                                {
                                    cb->tcpcb_flags &= ~FNET_TCP_CBF_INSND;

                                    sendanydata = FNET_FALSE;
                                }
                            }
                            else
                            {
                                /* If socket is not connected, delete the output buffer.*/
                                _fnet_socket_buffer_release(&sk->send_buffer);
                                cb->tcpcb_flags &= ~FNET_TCP_CBF_INSND;

                                sendanydata = FNET_FALSE;
                            }
                        }
                    }
                }
                else /* Not able to add to the socket send buffer.*/
                {
                    _fnet_netbuf_free( netbuf );
                    fnet_isr_unlock();
                    return 0;
                }
            }
        }

#if FNET_CFG_TCP_URGENT
        if(FNET_TCP_COMP_GE(cb->tcpcb_sndurgseq, cb->tcpcb_rcvack + sk->send_buffer.count))
        {
            cb->tcpcb_sndurgseq = cb->tcpcb_rcvack - 1;
        }
#endif /* FNET_CFG_TCP_URGENT */

        /* Remove the dontroute flag.*/
        if(dontroute == FNET_TRUE)
        {
            sk->options.so_dontroute = FNET_FALSE;
        }

        fnet_isr_unlock();
    }
    return (fnet_int32_t)sentlength;

ERROR:
    _fnet_socket_set_error(sk, error_code);
    return FNET_ERR;
}

/************************************************************************
* DESCRIPTION: This function closes a write-half, read-half, or
*              both halves of the connection.
*
* RETURNS: If no error occurs, this function returns FNET_OK. Otherwise
*          it returns FNET_ERR.
*************************************************************************/
static fnet_return_t _fnet_tcp_shutdown( fnet_socket_if_t *sk, fnet_sd_flags_t how )
{
    /* If the socket is not connected or has already been closed, return error.*/
    if(sk->state == SS_CLOSED)
    {
        _fnet_socket_set_error(sk, FNET_ERR_NOTCONN);
        return FNET_ERR;
    }
    else
    {
        fnet_isr_lock();

        /* Shutdown the writing.*/
        if(((how & SD_WRITE) != 0u ) && (sk->send_buffer.is_shutdown == FNET_FALSE))
        {
            /* Set the flag of the buffer.*/
            sk->send_buffer.is_shutdown = FNET_TRUE;

            /* Send the data that is in the output buffer.*/
            _fnet_tcp_send_anydata(sk, FNET_TRUE);
        }

        /* Shutdown the reading.*/
        if(((how & SD_READ) != 0u) && (sk->receive_buffer.is_shutdown == FNET_FALSE))
        {
            _fnet_socket_buffer_release(&sk->receive_buffer);

            /* Set the flag of the buffer (Data can't be read).*/
            sk->receive_buffer.is_shutdown = FNET_TRUE;
        }

        fnet_isr_unlock();

        return FNET_OK;
    }
}

/************************************************************************
* DESCRIPTION: This function sets a TCP option.
*
* RETURNS: If no error occurs, this function returns FNET_OK. Otherwise
*          it returns FNET_ERR.
*************************************************************************/
static fnet_return_t _fnet_tcp_setsockopt( fnet_socket_if_t *sk, fnet_protocol_t level, fnet_socket_options_t optname, const void *optval, fnet_size_t optlen )
{
    fnet_error_t error_code;

    /* If the level is not IPPROTO_TCP, go to IP processing.*/
    if(level == IPPROTO_TCP)
    {
        /* Check the option size.*/
        switch(optname)
        {
            case TCP_MSS:
            case TCP_KEEPCNT:
            case TCP_KEEPINTVL:
            case TCP_KEEPIDLE:
            case TCP_NODELAY:
#if FNET_CFG_TCP_URGENT
            case TCP_BSD:
#endif
                if(optlen != sizeof(fnet_uint32_t))
                {
                    error_code = FNET_ERR_INVAL;
                    goto ERROR;
                }
                break;
            default:
                break;
        }

        /* Process the option.*/
        switch(optname)
        {
            /* Maximal segment size option.*/
            case TCP_MSS:
                if(!(*((const fnet_uint32_t *)(optval))))
                {
                    error_code = FNET_ERR_INVAL;
                    goto ERROR;
                }

                sk->options.tcp_opt.mss = *((const fnet_uint32_t *)(optval));
                break;
            /* Keepalive probe retransmit limit.*/
            case TCP_KEEPCNT:
                if(!(*((const fnet_uint32_t *)(optval))))
                {
                    error_code = FNET_ERR_INVAL;
                    goto ERROR;
                }

                sk->options.tcp_opt.keep_cnt = *((const fnet_uint32_t *)(optval));
                break;
            /* Keepalive retransmit interval.*/
            case TCP_KEEPINTVL:
                if(!(*((const fnet_uint32_t *)(optval))))
                {
                    error_code = FNET_ERR_INVAL;
                    goto ERROR;
                }

                sk->options.tcp_opt.keep_intvl = (*((const fnet_uint32_t *)(optval)) * (1000u / FNET_TCP_SLOW_TIMER_PERIOD_MS));
                break;
            /* Time between keepalive probes.*/
            case TCP_KEEPIDLE:
                if(!(*((const fnet_uint32_t *)(optval))))
                {
                    error_code = FNET_ERR_INVAL;
                    goto ERROR;
                }

                sk->options.tcp_opt.keep_idle = (*((const fnet_uint32_t *)(optval)) * (1000u / FNET_TCP_SLOW_TIMER_PERIOD_MS));
                break;
#if FNET_CFG_TCP_URGENT
            /* BSD interpretation of the urgent pointer.*/
            case TCP_BSD:
                if(*((const fnet_uint32_t *)(optval)))
                {
                    sk->options.tcp_opt.tcp_bsd = FNET_TRUE;
                }
                else
                {
                    sk->options.tcp_opt.tcp_bsd = FNET_FALSE;
                }
                break;
#endif
            /* TCP_NO_DELAY option.*/
            case TCP_NODELAY:
                if(*((const fnet_uint32_t *)(optval)))
                {
                    sk->options.tcp_opt.tcp_nodelay = FNET_TRUE;
                }
                else
                {
                    sk->options.tcp_opt.tcp_nodelay = FNET_FALSE;
                }
                break;
            default:
                /* The option is not supported.*/
                error_code = FNET_ERR_NOPROTOOPT;
                goto ERROR;
        }

        return FNET_OK;
    }
    else
    {
        /* IP level option processing.*/
        return _fnet_ip_setsockopt(sk, level, optname, optval, optlen);
    }

ERROR:
    _fnet_socket_set_error(sk, error_code);
    return FNET_ERR;
}

/************************************************************************
* DESCRIPTION: This function receives a TCP option.
*
* RETURNS: If no error occurs, this function returns FNET_OK. Otherwise
*          it returns FNET_ERR.
*************************************************************************/
static fnet_return_t _fnet_tcp_getsockopt( fnet_socket_if_t *sk, fnet_protocol_t level, fnet_socket_options_t optname, void *optval, fnet_size_t *optlen )
{
    fnet_tcp_control_t  *cb = (fnet_tcp_control_t *)sk->protocol_control;

    if(level == IPPROTO_TCP)
    {
        switch(optname)
        {
            case TCP_KEEPCNT:
                *((fnet_uint32_t *)(optval)) = sk->options.tcp_opt.keep_cnt;
                break;
            case TCP_KEEPINTVL:
                *((fnet_uint32_t *)(optval)) = (sk->options.tcp_opt.keep_intvl / (1000u / FNET_TCP_SLOW_TIMER_PERIOD_MS));
                break;
            case TCP_KEEPIDLE:
                *((fnet_uint32_t *)(optval)) = (sk->options.tcp_opt.keep_idle / (1000u / FNET_TCP_SLOW_TIMER_PERIOD_MS));
                break;
#if FNET_CFG_TCP_URGENT
            case TCP_BSD:
                *((fnet_uint32_t *)(optval)) = sk->options.tcp_opt.tcp_bsd;
                break;
#endif
            case TCP_NODELAY:
                *((fnet_uint32_t *)(optval)) = sk->options.tcp_opt.tcp_nodelay;
                break;
            case TCP_FINRCVD:
                if((cb->tcpcb_flags & FNET_TCP_CBF_FIN_RCVD) != 0u)
                {
                    *((fnet_uint32_t *)(optval)) = 1u;
                }
                else
                {
                    *((fnet_uint32_t *)(optval)) = 0u;
                }
                break;
#if FNET_CFG_TCP_URGENT
            case TCP_URGRCVD:
                if(cb->tcpcb_flags & FNET_TCP_CBF_RCVURGENT)
                    *((fnet_uint32_t *)(optval)) = 1;
                else
                    *((fnet_uint32_t *)(optval)) = 0;
                break;
#endif
            case TCP_MSS:
                *((fnet_uint32_t *)(optval)) = sk->options.tcp_opt.mss;
                break;
            default:
                _fnet_socket_set_error(sk, FNET_ERR_NOPROTOOPT);
                return FNET_ERR;
        }

        *optlen = sizeof(fnet_uint32_t);

        return FNET_OK;
    }
    else
    {
        /* IP level option processing.*/
        return _fnet_ip_getsockopt(sk, level, optname, optval, optlen);
    }
}

/************************************************************************
* DESCRIPTION: This function changes
*              the state of the socket to the listening state.
*************************************************************************/
static fnet_return_t _fnet_tcp_listen( fnet_socket_if_t *sk, fnet_size_t backlog )
{
    /* Initializen the pointer to the control block.*/
    fnet_tcp_control_t  *cb = (fnet_tcp_control_t *)sk->protocol_control;

    if(sk->state == SS_LISTENING)
    {
        /* If the socket number of the listening socket is greater than the new backlog,
         * delete the sockets.*/
        fnet_isr_lock();

        while((backlog < (sk->partial_con_len + sk->incoming_con_len)) && (sk->partial_con_len))
        {
            _fnet_tcp_abort_socket(sk->partial_con);
        }

        while(backlog < sk->incoming_con_len)
        {
            _fnet_tcp_abort_socket(sk->incoming_con);
        }

        sk->con_limit = backlog;
        fnet_isr_unlock();
    }
    else
    {
        _fnet_tcp_init_connection(sk);

        /* Foreign address must be any.*/
        ((struct fnet_sockaddr_in *)(&sk->foreign_addr))->sin_addr.s_addr = INADDR_ANY;
        sk->foreign_addr.sa_port = 0u;
        sk->con_limit = backlog;

        /* Change the state.*/
        cb->tcpcb_connection_state = FNET_TCP_CS_LISTENING;
        sk->state = SS_LISTENING;
    }

    return FNET_OK;
}

/************************************************************************
* DESCRIPTION: Removes the temporary data.
*************************************************************************/
static void _fnet_tcp_drain( void )
{
    fnet_socket_if_t       *sk;
    fnet_socket_if_t       *delsk;
    fnet_tcp_control_t  *cb;

    fnet_isr_lock();

    /* Receive the pointer to the first socket.*/
    sk = fnet_tcp_prot_if.head;

    while(sk)
    {
        cb = (fnet_tcp_control_t *)sk->protocol_control;
        delsk = sk;
        sk = sk->next;

        /* if((cb->tcpcb_connection_state == FNET_TCP_CS_TIME_WAIT) && (cb->tcpcb_flags & FNET_TCP_CBF_CLOSE))*/
        if((cb->tcpcb_flags & FNET_TCP_CBF_CLOSE) != 0u)
        {
            /* Remove the socket that to be closed.  */
            _fnet_tcp_close_socket(delsk);
        }
        /* Delete all partial and incoming connections */
        else if(delsk->state == SS_LISTENING)
        {
#if 1 /* So far, disabled. It was reported that it may cause problems - not reproduced yet */
            while(delsk->partial_con)
            {
                _fnet_tcp_abort_socket(delsk->partial_con);
            }
#endif
#if 0
            while (delsk->incoming_con)
            {
                _fnet_tcp_abort_socket(delsk->incoming_con);
            }
#endif
        }
#if !FNET_CFG_TCP_DISCARD_OUT_OF_ORDER
        else
        {
            /* Remove the temporary data.*/
            _fnet_tcp_del_tmpbuf(cb);
        }
#endif
    }
    fnet_isr_unlock();
}

/************************************************************************
* DESCRIPTION: This function creates and initializes the control block,
*              and initializes the socket.
*************************************************************************/
static void _fnet_tcp_init_connection( fnet_socket_if_t *sk )
{
    fnet_tcp_control_t  *cb;

    cb = (fnet_tcp_control_t *)sk->protocol_control;

    fnet_memset_zero(cb, sizeof(fnet_tcp_control_t));

    /* Set the default maximal segment size value.*/
    cb->tcpcb_sndmss = FNET_TCP_DEFAULT_MSS;
    cb->tcpcb_rcvmss = (fnet_uint16_t)sk->options.tcp_opt.mss;

    /* Set the length of the socket buffers.*/
    cb->tcpcb_rcvcountmax = sk->receive_buffer.count_max;

    /* The input buffer can't be greater than the FNET_TCP_MAX_BUFFER value.*/
    if(cb->tcpcb_rcvcountmax > FNET_TCP_MAX_BUFFER)
    {
        cb->tcpcb_rcvcountmax = FNET_TCP_MAX_BUFFER;
    }

    /* If a segment size greater than the buffer length, recalculate the segment size.*/
    if(cb->tcpcb_rcvcountmax < cb->tcpcb_rcvmss)
    {
        cb->tcpcb_rcvmss = (fnet_uint16_t)cb->tcpcb_rcvcountmax;
    }

    /* Receive a scale of the input window.*/
    while(((fnet_uint32_t)FNET_TCP_MAXWIN << cb->tcpcb_recvscale) < cb->tcpcb_rcvcountmax)
    {
        cb->tcpcb_recvscale++;
    }

    /* Stop all timers.*/
    cb->tcpcb_timers.retransmission = FNET_TCP_TIMER_OFF;
    cb->tcpcb_timers.connection = FNET_TCP_TIMER_OFF;
    cb->tcpcb_timers.abort = FNET_TCP_TIMER_OFF;
    cb->tcpcb_timers.round_trip = FNET_TCP_TIMER_OFF;
    cb->tcpcb_timers.persist = FNET_TCP_TIMER_OFF;
    cb->tcpcb_timers.keepalive = FNET_TCP_TIMER_OFF;
    cb->tcpcb_timers.delayed_ack = FNET_TCP_TIMER_OFF;

    /* Initialize the retransmission timeout.*/
    cb->tcpcb_rto = FNET_TCP_TIMERS_INIT;
    cb->tcpcb_crto = FNET_TCP_TIMERS_INIT;

#if FNET_CFG_TCP_URGENT
    /* Initialize the receive urgent mark.*/
    cb->tcpcb_rcvurgmark = FNET_TCP_NOT_USED;
#endif /* FNET_CFG_TCP_URGENT */

    /* Initialize Slow Start Threshold.*/
    cb->tcpcb_ssthresh = FNET_TCP_MAX_BUFFER;

    /* Clear the input buffer.*/
    if(sk->receive_buffer.count)
    {
        _fnet_socket_buffer_release(&sk->receive_buffer);
    }
}

/************************************************************************
* DESCRIPTION: This function processes the input segments of
*              the corresponding socket.
*
* RETURNS: TRUE if the input segment must be deleted. Otherwise
*          this function returns FALSE.
*************************************************************************/
static fnet_bool_t _fnet_tcp_inputsk( fnet_socket_if_t *sk, fnet_netbuf_t *insegment, struct fnet_sockaddr *src_addr, struct fnet_sockaddr *dest_addr)
{
    fnet_tcp_control_t  *cb = (fnet_tcp_control_t *)sk->protocol_control;
    fnet_tcp_control_t  *pcb;                   /* Pointer to the partial control block.*/
    fnet_uint8_t        sgmtype;                /* Flags of the segment.*/
    fnet_socket_if_t    *psk;                   /* Pointer to the partial socket.*/
    fnet_bool_t         result = FNET_TRUE;
    fnet_uint8_t        options[FNET_TCP_MAX_OPT_SIZE];
    fnet_uint8_t        optionlen;
    fnet_size_t         repsize;                /* Size of repeated data.*/
    fnet_flag_t         ackparam = 0u;          /* Acknowledgment parameter.*/
    fnet_uint32_t       tcp_seq = fnet_ntohl(FNET_TCP_SEQ(insegment));
    fnet_size_t         tcp_length = (fnet_size_t)FNET_TCP_LENGTH(insegment);
    fnet_uint32_t       tcp_ack = fnet_ntohl(FNET_TCP_ACK(insegment));
    fnet_bool_t         exit_flag = FNET_FALSE;

    /* Get the flags.*/
    sgmtype = (fnet_uint8_t)(FNET_TCP_FLAGS(insegment));

    /* Check the sequence number.*/
    switch(cb->tcpcb_connection_state)
    {
        case FNET_TCP_CS_SYN_SENT:
        case FNET_TCP_CS_LISTENING:
            break;
        default:
            if((cb->tcpcb_connection_state) == FNET_TCP_CS_SYN_RCVD)
            {
                if((cb->tcpcb_prev_connection_state == FNET_TCP_CS_SYN_SENT) && ((sgmtype & FNET_TCP_SGT_SYN) != 0u))
                {
                    /* Check the sequence number for simultaneouos open.*/
                    if(tcp_seq == cb->tcpcb_sndack - 1u)
                    {
                        break;
                    }
                }
            }

            if(FNET_TCP_COMP_G(cb->tcpcb_sndack, tcp_seq))
            {
                if(FNET_TCP_COMP_G(tcp_seq + insegment->total_length - tcp_length, cb->tcpcb_sndack))
                {
                    /* Delete the left repeated part.*/
                    repsize = _fnet_tcp_get_size(tcp_seq, cb->tcpcb_sndack);
                    _fnet_netbuf_cut_center(&insegment, tcp_length, repsize);

                    /* If urgent  flag is present, recalculate of the urgent pointer.*/
                    if((sgmtype & FNET_TCP_SGT_URG) != 0u)
                    {
                        if((fnet_size_t)fnet_ntohs(FNET_TCP_URG(insegment)) >= repsize)
                        {
                            FNET_TCP_URG(insegment) = fnet_htons((fnet_uint16_t)(fnet_ntohs(FNET_TCP_URG(insegment)) - repsize));
                        }
                        else
                        {
                            sgmtype &= ~FNET_TCP_SGT_URG;
                            FNET_TCP_SET_FLAGS(insegment) = sgmtype;
                        }
                    }

                    /* Set the sequence number.*/
                    tcp_seq = cb->tcpcb_sndack;
                    FNET_TCP_SEQ(insegment) = fnet_htonl(tcp_seq);

                    /* Acknowledgment must be sent immediatelly.*/
                    ackparam |= FNET_TCP_AP_SEND_IMMEDIATELLY;
                }
                else
                {
                    /* Segment is repeated */
                    /* Send the acknowledgment */
                    _fnet_tcp_send_ack(sk);
                    return FNET_TRUE;
                }
            }

            if(FNET_TCP_COMP_G(tcp_seq, cb->tcpcb_sndack + cb->tcpcb_rcvwnd))
            {
                /* Segment is not in the window*/
                /* Send the acknowledgment */
                _fnet_tcp_send_ack(sk);
                return FNET_TRUE;
            }
            else
            {
                if(FNET_TCP_COMP_G(tcp_seq + insegment->total_length - tcp_length, cb->tcpcb_sndack + cb->tcpcb_rcvwnd))
                {
                    /* Delete the right part that is not in the window.*/
                    _fnet_netbuf_trim(&insegment, -(fnet_int32_t)_fnet_tcp_get_size(cb->tcpcb_sndack + cb->tcpcb_rcvwnd,
                                      (fnet_uint32_t)(tcp_seq + insegment->total_length - tcp_length)));
                    /* Acknowledgment must be sent immediatelly.*/
                    ackparam |= FNET_TCP_AP_SEND_IMMEDIATELLY;
                }
            }
            break;
    }

    /* Process the reset segment with acknowledgment.*/
    if((sgmtype & (FNET_TCP_SGT_RST | FNET_TCP_SGT_ACK)) == (FNET_TCP_SGT_RST | FNET_TCP_SGT_ACK))
    {
        if(cb->tcpcb_connection_state == FNET_TCP_CS_SYN_SENT)
        {
            if(tcp_ack == cb->tcpcb_sndseq)
            {
                /* Close the socket (connecting is failed).*/
                sk->options.local_error = FNET_ERR_CONNRESET;
                _fnet_tcp_close_socket(sk);
            }

            return FNET_TRUE;
        }
    }

    /* Process the reset segment without acknowledgment.*/
    if((sgmtype & FNET_TCP_SGT_RST) != 0u)
    {
        switch(cb->tcpcb_connection_state)
        {
            case FNET_TCP_CS_LISTENING:
            case FNET_TCP_CS_SYN_SENT:
                break;
            default:
                /* Close socket.*/
                sk->options.local_error = FNET_ERR_CONNRESET;
                _fnet_tcp_close_socket(sk);
                break;
        }

        return FNET_TRUE;
    }

    /* Process the SYN segment.*/
    if((sgmtype & FNET_TCP_SGT_SYN) != 0u)
    {
        switch(cb->tcpcb_connection_state)
        {
            case FNET_TCP_CS_SYN_SENT:
            case FNET_TCP_CS_LISTENING:
                break;

            case FNET_TCP_CS_SYN_RCVD:
                if((cb->tcpcb_prev_connection_state == FNET_TCP_CS_SYN_SENT) && (tcp_seq == (cb->tcpcb_sndack - 1u)))
                {
                    break;
                }
            default:
                /* Close the socket and send the reset segment.*/
                _fnet_tcp_send_rst(&sk->options, insegment, dest_addr, src_addr);
                _fnet_tcp_close_socket(sk);
                return FNET_TRUE;
        }
    }
    else
    {
        /* Process the segment without SYN flag.*/
        switch(cb->tcpcb_connection_state)
        {
            case FNET_TCP_CS_LISTENING:
                if((sgmtype & FNET_TCP_SGT_ACK) != 0u)
                {
                    /* Send the reset segment.*/
                    _fnet_tcp_send_rst(&sk->options, insegment, dest_addr, src_addr);
                }
                exit_flag = FNET_TRUE;
                break;
            case FNET_TCP_CS_SYN_SENT:
                if((sgmtype & FNET_TCP_SGT_ACK) != 0u)
                {
                    /* Send the reset segment.*/
                    _fnet_tcp_send_rst(&sk->options, insegment, dest_addr, src_addr);
                }
                exit_flag = FNET_TRUE;
                break;
            default:
                break;  /* do nothing, avoid compiler warning "enumeration value not handled in switch" */
        }
    }

    if(exit_flag == FNET_TRUE)
    {
        return FNET_TRUE;
    }

    /* Process the segment with acknowledgment.*/
    if((sgmtype & FNET_TCP_SGT_ACK) != 0u)
    {
        switch(cb->tcpcb_connection_state)
        {
            case FNET_TCP_CS_SYN_SENT:
            case FNET_TCP_CS_SYN_RCVD:
                if(tcp_ack != cb->tcpcb_sndseq)
                {
                    /* Send the reset segment.*/
                    _fnet_tcp_send_rst(&sk->options, insegment, dest_addr, src_addr);
                    return FNET_TRUE;
                }
                break;
            case FNET_TCP_CS_LISTENING:
                /* Send the reset segment.*/
                _fnet_tcp_send_rst(&sk->options, insegment, dest_addr, src_addr);

                exit_flag = FNET_TRUE;
                break;
            default:
                if(!_fnet_tcp_hit(cb->tcpcb_rcvack, cb->tcpcb_maxrcvack, tcp_ack))
                {
                    if(FNET_TCP_COMP_G(tcp_ack, cb->tcpcb_maxrcvack))
                    {
                        /* Send the acknowledgment.*/
                        _fnet_tcp_send_ack(sk);
                    }
                    return FNET_TRUE;
                }
                break;
        }
    }
    else
    {
        /* Process the segment without acknowledgment.*/
        switch(cb->tcpcb_connection_state)
        {
            case FNET_TCP_CS_SYN_SENT:
            case FNET_TCP_CS_LISTENING:
                break;
            case FNET_TCP_CS_SYN_RCVD:
                _fnet_tcp_send_ack(sk);
                exit_flag = FNET_TRUE;
                break;
            default:
                return FNET_TRUE;
        }
    }

    if(exit_flag == FNET_TRUE)
    {
        return FNET_TRUE;
    }

    /* Set the window size (of another side).*/
    if((sgmtype & FNET_TCP_SGT_SYN) != 0u)
    {
        cb->tcpcb_sndwnd = fnet_ntohs(FNET_TCP_WND(insegment));
    }
    else
    {
        cb->tcpcb_sndwnd = ((fnet_uint32_t)fnet_ntohs(FNET_TCP_WND(insegment)) << cb->tcpcb_sendscale);
    }

    if(cb->tcpcb_maxwnd < cb->tcpcb_sndwnd)
    {
        cb->tcpcb_maxwnd = cb->tcpcb_sndwnd;
    }

    /* Main processing.*/
    switch(cb->tcpcb_connection_state)
    {
        case FNET_TCP_CS_SYN_SENT:
            cb->tcpcb_sndack = tcp_seq + 1u;

            /* Process the second segment of the open.*/
            if((sgmtype & FNET_TCP_SGT_ACK) != 0u)
            {
                cb->tcpcb_rcvack = tcp_ack;

#if FNET_CFG_TCP_URGENT
                /* Initialize the urgent sequence number.*/
                cb->tcpcb_rcvurgseq = tcp_seq;
#endif /* FNET_CFG_TCP_URGENT */

                /* Receive the options.*/
                _fnet_tcp_get_opt(sk, insegment);
                _fnet_tcp_get_synopt(sk);

                /* If MSS of another side 0, return.*/
                if(!cb->tcpcb_sndmss)
                {
                    _fnet_tcp_send_rst(&sk->options, insegment, dest_addr, src_addr);
                    _fnet_tcp_close_socket(sk);
                    return FNET_TRUE;
                }

                /* Stop the timers.*/
                cb->tcpcb_timers.retransmission = FNET_TCP_TIMER_OFF;
                cb->tcpcb_timers.connection = FNET_TCP_TIMER_OFF;

                /* Send the acknowledgment (third segment of the open).*/
                _fnet_tcp_send_ack(sk);

                /* Change the states.*/
                cb->tcpcb_connection_state = FNET_TCP_CS_ESTABLISHED;
                sk->state = SS_CONNECTED;

                /* Initialize the keepalive timer.*/
                if(sk->options.so_keepalive == FNET_TRUE)
                {
                    cb->tcpcb_timers.keepalive = sk->options.tcp_opt.keep_idle;
                }
            }
            else
                /* Process the simultaneous open.*/
            {
                /* Reinitialize the retrasmission timer.*/
                cb->tcpcb_timers.retransmission = cb->tcpcb_rto;

                /* Receive the options.*/
                _fnet_tcp_get_opt(sk, insegment);
                _fnet_tcp_get_synopt(sk);

                /* If MSS of another side 0, return.*/
                if(!cb->tcpcb_sndmss)
                {
                    _fnet_tcp_send_rst(&sk->options, insegment, dest_addr, src_addr);
                    _fnet_tcp_close_socket(sk);
                    return FNET_TRUE;
                }

#if FNET_CFG_TCP_URGENT
                /* Initialize the urgent sequence number.*/
                cb->tcpcb_rcvurgseq = tcp_seq;
#endif /* FNET_CFG_TCP_URGENT */

                /* Change the states.*/
                cb->tcpcb_connection_state = FNET_TCP_CS_SYN_RCVD;
                cb->tcpcb_prev_connection_state = FNET_TCP_CS_SYN_SENT;

                /* Send acknowledgment.*/
                _fnet_tcp_send_ack(sk);
            }
            break;
        /* Process the first segment.*/
        case FNET_TCP_CS_LISTENING:

            /* If socket can't be created, return.*/
            if(sk->partial_con_len + sk->incoming_con_len >= sk->con_limit)
            {
                fnet_memset_zero(&sk->foreign_addr, sizeof(sk->foreign_addr));
                cb->tcpcb_sndwnd = 0u;
                cb->tcpcb_maxwnd = 0u;
                break;
            }

            /* Create the socket.*/
            psk = _fnet_socket_copy(sk);

            fnet_memset_zero(&sk->foreign_addr, sizeof(sk->foreign_addr));

            /* Check the memory allocation.*/
            if(!psk)
            {
                cb->tcpcb_sndwnd = 0u;
                cb->tcpcb_maxwnd = 0u;
                break;
            }

            /* Set the local address.*/
            fnet_memcpy(&psk->local_addr, dest_addr, sizeof(psk->local_addr));

            /* Create the control block.*/
            pcb = (fnet_tcp_control_t *)_fnet_malloc_zero(sizeof(fnet_tcp_control_t));

            /* Check the memory allocation.*/
            if(!pcb)
            {
                _fnet_free(psk);
                cb->tcpcb_sndwnd = 0u;
                cb->tcpcb_maxwnd = 0u;
                break;
            }

            /* Initialize the pointer.*/
            psk->protocol_control = (void *)pcb;
            _fnet_tcp_init_connection(psk);

            /* Copy the control block parameters.*/
            pcb->tcpcb_sndwnd = cb->tcpcb_sndwnd;
            pcb->tcpcb_maxwnd = cb->tcpcb_maxwnd;
            cb->tcpcb_sndwnd = 0u;
            cb->tcpcb_maxwnd = 0u;

            /* Add the new socket to the partial list.*/
            _fnet_tcp_add_partial_socket(sk, psk);

            /* Initialize the parameters of the control block.*/
            pcb->tcpcb_sndack = tcp_seq + 1u;
            pcb->tcpcb_sndseq = _fnet_tcp_initial_seq_number;
            pcb->tcpcb_maxrcvack = _fnet_tcp_initial_seq_number + 1u;

#if FNET_CFG_TCP_URGENT
            pcb->tcpcb_sndurgseq = pcb->tcpcb_sndseq;
            pcb->tcpcb_rcvurgseq = tcp_seq;
#endif /* FNET_CFG_TCP_URGENT */

            /* Change the states.*/
            psk->state = SS_CONNECTING;
            pcb->tcpcb_prev_connection_state = FNET_TCP_CS_LISTENING;
            pcb->tcpcb_connection_state = FNET_TCP_CS_SYN_RCVD;

            /* Receive the options.*/
            _fnet_tcp_get_opt(psk, insegment);
            _fnet_tcp_get_synopt(psk);

            /* If MSS of another side 0, return.*/
            if(!pcb->tcpcb_sndmss)
            {
                _fnet_tcp_send_rst(&sk->options, insegment, dest_addr, src_addr);
                _fnet_tcp_close_socket(psk);
                break;
            }

            /* Set the options.*/
            _fnet_tcp_set_synopt(psk, options, &optionlen);

            /* Send SYN segment.*/
            _fnet_tcp_send_headseg(psk, FNET_TCP_SGT_SYN | FNET_TCP_SGT_ACK, options, optionlen);

            /* Increase ISN (Initial Sequence Number).*/
           _fnet_tcp_initial_seq_number_update();

            /* Initialization the connection timer.*/
            pcb->tcpcb_timers.connection = FNET_TCP_ABORT_INTERVAL_CON;
            pcb->tcpcb_timers.retransmission = pcb->tcpcb_rto;
            break;

        case FNET_TCP_CS_SYN_RCVD:

            /* Change the states.*/
            cb->tcpcb_connection_state = FNET_TCP_CS_ESTABLISHED;
            sk->state = SS_CONNECTED;

            /* Stop the connection and retransmission timers.*/
            cb->tcpcb_timers.connection = FNET_TCP_TIMER_OFF;
            cb->tcpcb_timers.retransmission = FNET_TCP_TIMER_OFF;

            cb->tcpcb_rcvack = tcp_ack;

            /* If previous state is FNET_TCP_CS_LISTENING, process the acknowledgment (third segment of the open)
            * Otherwise, process the SYN segment.*/
            if(cb->tcpcb_prev_connection_state == FNET_TCP_CS_LISTENING)
            {
                /* Add the partial socket to the list of incoming socket.*/
                _fnet_tcp_move_socket2incominglist(sk);

                /* Proceed the processing.*/
                result = _fnet_tcp_dataprocess(sk, insegment, &ackparam);
            }
            else
            {
                if((sgmtype & FNET_TCP_SGT_SYN) == 0u)
                {
                    /* Proseed the processing.*/
                    result = _fnet_tcp_dataprocess(sk, insegment, &ackparam);
                }
                else
                {
                    /* Initialize the keepalive timer.*/
                    if(sk->options.so_keepalive == FNET_TRUE)
                    {
                        cb->tcpcb_timers.keepalive = sk->options.tcp_opt.keep_idle;
                    }
                }
            }
            break;

        case FNET_TCP_CS_FIN_WAIT_2:
        case FNET_TCP_CS_CLOSE_WAIT:
        case FNET_TCP_CS_ESTABLISHED:
            /* Proseed the processing.*/
            result = _fnet_tcp_dataprocess(sk, insegment, &ackparam);
            break;
        case FNET_TCP_CS_FIN_WAIT_1:
            /* Proseed the processing.*/
            result = _fnet_tcp_dataprocess(sk, insegment, &ackparam);

            if((cb->tcpcb_sndseq == cb->tcpcb_rcvack) && (cb->tcpcb_connection_state == FNET_TCP_CS_FIN_WAIT_1))
            {
                /* Change the state.*/
                cb->tcpcb_connection_state = FNET_TCP_CS_FIN_WAIT_2;
            }
            break;
        case FNET_TCP_CS_LAST_ACK:
            if(tcp_ack == cb->tcpcb_sndseq)
            {
                /* Close the socket.*/
                _fnet_tcp_close_socket(sk);
            }

            exit_flag = FNET_TRUE;
            break;
        case FNET_TCP_CS_CLOSING:
            if(tcp_ack == cb->tcpcb_sndseq)
            {
                cb->tcpcb_connection_state = FNET_TCP_CS_TIME_WAIT;
                /* Set the  timeout of the TIME_WAIT state.*/
                cb->tcpcb_timers.connection = FNET_TCP_TIME_WAIT;
                cb->tcpcb_timers.retransmission = FNET_TCP_TIMER_OFF;
                cb->tcpcb_timers.keepalive = FNET_TCP_TIMER_OFF;
            }
            break;
        case FNET_TCP_CS_TIME_WAIT:
            _fnet_tcp_send_rst(&sk->options, insegment, dest_addr, src_addr);
            break;
        default:
            break;  /* do nothing, avoid compiler warning "enumeration value not handled in switch" */
    }

    if(exit_flag == FNET_TRUE)
    {
        return FNET_TRUE;
    }

    /* If the input buffer is closed, delete the input data.*/
    if((sk->receive_buffer.is_shutdown) && (sk->receive_buffer.count))
    {
        _fnet_socket_buffer_release(&sk->receive_buffer);
    }

    return result;
}

/************************************************************************
* DESCRIPTION: This function proceeds the processing of the input segment.
*
* RETURNS: TRUE if the input segment must be deleted. Otherwise
*          this function returns FALSE.
*************************************************************************/
static fnet_bool_t _fnet_tcp_dataprocess( fnet_socket_if_t *sk, fnet_netbuf_t *insegment, fnet_flag_t *ackparam )
{
    fnet_tcp_control_t  *cb = (fnet_tcp_control_t *)sk->protocol_control;
    fnet_size_t         size;
    fnet_int32_t        err;
    fnet_bool_t         delflag = FNET_TRUE;
    fnet_uint32_t       seq;
    fnet_uint32_t       tcp_ack = fnet_ntohl(FNET_TCP_ACK(insegment));
    fnet_uint8_t        insegment_flags = FNET_TCP_FLAGS(insegment);

    /* Reinitialize the keepalive timer.*/
    if(sk->options.so_keepalive == FNET_TRUE)
    {
        cb->tcpcb_timers.keepalive = sk->options.tcp_opt.keep_idle;
    }
    else
    {
        cb->tcpcb_timers.keepalive = FNET_TCP_TIMER_OFF;
    }

    /* Reinitialize delayed ACK timer.*/
    if((*ackparam & FNET_TCP_AP_SEND_WITH_DELAY) != 0u)
    {
        cb->tcpcb_timers.delayed_ack = (200 / FNET_TCP_FAST_TIMER_PERIOD_MS); /* Delay 200 ms*/
    }

    /* Reset the abort timer.*/
    cb->tcpcb_timers.abort = FNET_TCP_TIMER_OFF;

    /* If acknowledgment is repeated.*/
    if(cb->tcpcb_rcvack == tcp_ack)
    {
        /* If unacknowledged data is present.*/
        /*
        DUPLICATE ACKNOWLEDGMENT: An acknowledgment is considered a
              "duplicate" in the following algorithms when (a) the receiver of
              the ACK has outstanding data, (b) the incoming acknowledgment
              carries no data, (c) the SYN and FIN bits are both off, (d) the
              acknowledgment number is equal to the greatest acknowledgment
              received on the given connection (TCP.UNA from [RFC793]) and (e)-TBD
              the advertised window in the incoming acknowledgment equals the
              advertised window in the last incoming acknowledgment.*/
        if((cb->tcpcb_sndseq != cb->tcpcb_rcvack) && (sk->send_buffer.count != 0u) &&
           ((insegment->total_length - (fnet_size_t)FNET_TCP_LENGTH(insegment)) == 0u) &&
           ((insegment_flags & (FNET_TCP_SGT_FIN | FNET_TCP_SGT_SYN)) == 0u))
        {
            /* Increase the timer of repeated acknowledgments.*/
            cb->tcpcb_fastretrcounter++;

            /* If the number of repeated acknowledgments is FNET_TCP_NUMBER_FOR_FAST_RET,
             * process the fast retransmission.*/
            if(cb->tcpcb_fastretrcounter == FNET_TCP_NUMBER_FOR_FAST_RET)
            {
                /* Increase the timer of repeated acknowledgments.*/
                cb->tcpcb_fastretrcounter++;

                /* Recalculate the congestion window and slow start threshold values.*/
                if(cb->tcpcb_cwnd > cb->tcpcb_sndwnd)
                {
                    cb->tcpcb_ssthresh = cb->tcpcb_sndwnd >> 1;
                }
                else
                {
                    cb->tcpcb_ssthresh = cb->tcpcb_cwnd >> 1;
                }

                if(cb->tcpcb_ssthresh < ((fnet_uint32_t)cb->tcpcb_sndmss << 1))
                {
                    cb->tcpcb_ssthresh = ((fnet_uint32_t)cb->tcpcb_sndmss << 1);
                }

                cb->tcpcb_cwnd = cb->tcpcb_ssthresh;

                /* Retransmit the segment.*/
                seq = cb->tcpcb_sndseq;
                cb->tcpcb_sndseq = cb->tcpcb_rcvack;
                _fnet_tcp_send_dataseg(sk, 0, 0u, (fnet_size_t)cb->tcpcb_sndmss);
                cb->tcpcb_sndseq = seq;

                /* Acknowledgment is sent in retransmitted segment.*/
                *ackparam |= (fnet_flag_t)FNET_TCP_AP_NO_SENDING;

                /* Round trip time can't be measured in this case.*/
                cb->tcpcb_timers.round_trip = FNET_TCP_TIMER_OFF;
                cb->tcpcb_timing_state = TCP_TS_SEGMENT_LOST;
            }
        }
    }
    else
    {
        /* Reset the counter of repeated acknowledgments.*/
        cb->tcpcb_fastretrcounter = 0u;

        /* Recalculate the congestion window and slow start threshold values.*/
        size = _fnet_tcp_get_size(cb->tcpcb_rcvack, tcp_ack);

        if(size > sk->send_buffer.count)
        {
            size = sk->send_buffer.count;
        }

        if(cb->tcpcb_cwnd < FNET_TCP_MAX_BUFFER)
        {
            if(cb->tcpcb_cwnd > cb->tcpcb_ssthresh)
            {
                /* Congestion avoidance mode.*/
                cb->tcpcb_pcount += size;
            }
            else
            {
                /* Slow start mode.*/
                if(cb->tcpcb_cwnd + size > cb->tcpcb_ssthresh)
                {
                    cb->tcpcb_pcount = cb->tcpcb_pcount + cb->tcpcb_cwnd + size - cb->tcpcb_ssthresh;
                    cb->tcpcb_cwnd = cb->tcpcb_ssthresh;
                }
                else
                {
                    cb->tcpcb_cwnd += size;
                }
            }

            if(cb->tcpcb_pcount >= cb->tcpcb_cwnd)
            {
                cb->tcpcb_pcount -= cb->tcpcb_cwnd;
                cb->tcpcb_cwnd += cb->tcpcb_sndmss;
            }
        }

        /* Delete the acknowledged data.*/
        _fnet_netbuf_trim(&sk->send_buffer.net_buf_chain, (fnet_int32_t)size);
        sk->send_buffer.count -= size;

        /* Save the acknowledgment number.*/
        cb->tcpcb_rcvack = tcp_ack;

        if(FNET_TCP_COMP_G(cb->tcpcb_rcvack, cb->tcpcb_sndseq))
        {
            cb->tcpcb_sndseq = cb->tcpcb_rcvack;
        }

        /* Calculate the retransmission timeout ( using Jacobson method ).*/
        if((FNET_TCP_COMP_GE(cb->tcpcb_rcvack, cb->tcpcb_timingack)) && ((cb->tcpcb_timing_state) == TCP_TS_SEGMENT_SENT))
        {
            if(cb->tcpcb_srtt)
            {
                err = (fnet_int32_t)(cb->tcpcb_timers.round_trip - ((fnet_uint32_t)cb->tcpcb_srtt >> FNET_TCP_RTT_SHIFT));

                if((cb->tcpcb_srtt += err) <= 0)
                {
                    cb->tcpcb_srtt = 1;
                }

                if(err < 0)
                {
                    err = -err;
                }

                err -= (fnet_int32_t)((fnet_uint32_t)cb->tcpcb_rttvar >> FNET_TCP_RTTVAR_SHIFT);

                if((cb->tcpcb_rttvar += err) <= 0)
                {
                    cb->tcpcb_rttvar = 1;
                }
            }
            else
            {
                /* Initial calculation of the retransmission variables.*/
                cb->tcpcb_srtt = (fnet_int32_t)(((fnet_uint32_t)cb->tcpcb_timers.round_trip + 1u) << FNET_TCP_RTT_SHIFT);
                cb->tcpcb_rttvar = (fnet_int32_t)(((fnet_uint32_t)cb->tcpcb_timers.round_trip + 1u) << (FNET_TCP_RTTVAR_SHIFT - 1u));
            }

            cb->tcpcb_timing_state = TCP_TS_ACK_RECEIVED;
            cb->tcpcb_rto = ((fnet_uint32_t)cb->tcpcb_srtt >> FNET_TCP_RTT_SHIFT) + (fnet_uint32_t)cb->tcpcb_rttvar;
            cb->tcpcb_timers.round_trip = FNET_TCP_TIMER_OFF;
        }
    }

    /* If the final segment is not received, add the data to the input buffer.*/
    if((cb->tcpcb_flags & FNET_TCP_CBF_FIN_RCVD) == 0u)
    {
        delflag = (_fnet_tcp_add_inpbuf(sk, insegment, ackparam) == FNET_FALSE) ? FNET_TRUE : FNET_FALSE;

    }
    else if(insegment->total_length  > (fnet_size_t)FNET_TCP_LENGTH(insegment))
    {
        *ackparam |= FNET_TCP_AP_SEND_IMMEDIATELLY;
    }
    else
    {}

    /* Acknowledgment of the final segment must be send immediatelly.*/
    if(((*ackparam & FNET_TCP_AP_FIN_ACK) != 0u)
       || ((insegment_flags & FNET_TCP_SGT_PSH) != 0u))
    {
        _fnet_tcp_send_ack(sk);
    }

    /* Try to sent the data.*/
    if(_fnet_tcp_send_anydata(sk, ((cb->tcpcb_flags & FNET_TCP_CBF_INSND) != 0u) ? FNET_TRUE : FNET_FALSE))
    {
        *ackparam |= FNET_TCP_AP_NO_SENDING;
    }

    /* If the window of another side is closed, turn on the persist timer.*/
    if((!cb->tcpcb_sndwnd) && (sk->send_buffer.count))
    {
        if(cb->tcpcb_timers.persist == FNET_TCP_TIMER_OFF)
        {
            cb->tcpcb_cprto = cb->tcpcb_rto;
        }

        cb->tcpcb_timers.persist = cb->tcpcb_cprto;
    }
    else
    {
        if((cb->tcpcb_flags & FNET_TCP_CBF_SEND_TIMEOUT) == 0u)
        {
            cb->tcpcb_timers.persist = FNET_TCP_TIMER_OFF;
        }
    }

    /* If the sent data is acknowledged, turn of the retransmission timer.*/
    if(cb->tcpcb_rcvack == cb->tcpcb_sndseq)
    {
        cb->tcpcb_timers.retransmission = FNET_TCP_TIMER_OFF;
    }
    else
    {
        cb->tcpcb_timers.retransmission = cb->tcpcb_rto;
    }

    /* If the acknowledgment is sent, return
     * If the acknowledgment must be sent immediatelly, send it
     * If the acknowledgment must be sent after delay, turn on the acknowledgment timer.*/
    if(((*ackparam & FNET_TCP_AP_NO_SENDING) != 0u) || ((*ackparam & FNET_TCP_AP_FIN_ACK) != 0u))
    {
        return delflag;
    }

    if((*ackparam & FNET_TCP_AP_SEND_IMMEDIATELLY) != 0u)
    {
        _fnet_tcp_send_ack(sk);
        return delflag;
    }

    return delflag;
}

/***********************************************************************
* DESCRIPTION: This function adds the data to the input buffer and
*              returns the acknowledgement parameter.
*
* RETURNS: TRUE if the input segment is added to the buffer. Otherwise
*          this function returns FALSE.
*************************************************************************/
static fnet_bool_t _fnet_tcp_add_inpbuf( fnet_socket_if_t *sk, fnet_netbuf_t *insegment, fnet_flag_t *ackparam )
{
    fnet_tcp_control_t  *cb = (fnet_tcp_control_t *)sk->protocol_control;
    fnet_uint32_t       tcp_length = (fnet_uint32_t)FNET_TCP_LENGTH(insegment);
    fnet_uint32_t       tcp_flags = (fnet_uint32_t)FNET_TCP_FLAGS(insegment);
    fnet_bool_t         result;

    /* If segment doesn't include the data and the FIN or URG flag,
     * return from the function.*/
    if((!(insegment->total_length - tcp_length))
       && ((tcp_flags & (FNET_TCP_SGT_URG | FNET_TCP_SGT_FIN)) == 0u))
    {
        return FNET_FALSE; /* The data isn't added.*/
    }

    /* Process the segment that came in order.*/
    if((fnet_ntohl(FNET_TCP_SEQ(insegment)) == (cb->tcpcb_sndack))
#if !FNET_CFG_TCP_DISCARD_OUT_OF_ORDER
       && (!cb->tcpcb_count)
#endif
      )
    {
#if FNET_CFG_TCP_URGENT
        if(tcp_flags & FNET_TCP_SGT_URG)
        {
            /* Process the urgent data.*/
            _fnet_tcp_process_urg(sk, &insegment, 0, ackparam);
        }
        else
        {
            /* Pull  the receive urgent pointer
             * along with the receive window */
            cb->tcpcb_rcvurgseq = cb->tcpcb_sndack - 1;
        }
#endif /* FNET_CFG_TCP_URGENT */

        /* Process the final segment. */
        if((tcp_flags & FNET_TCP_SGT_FIN) != 0u)
        {
            _fnet_tcp_process_fin(sk, fnet_ntohl(FNET_TCP_ACK(insegment)));
            cb->tcpcb_sndack++;
            *ackparam |= FNET_TCP_AP_FIN_ACK;
        }

        /* Delete the header.*/
        _fnet_netbuf_trim(&insegment, (fnet_int32_t)tcp_length);

        /* Add the data.*/
        if(insegment)
        {
            cb->tcpcb_sndack += insegment->total_length;
            sk->receive_buffer.net_buf_chain = _fnet_netbuf_concat(sk->receive_buffer.net_buf_chain,
                                               insegment);
            sk->receive_buffer.count += insegment->total_length;

            *ackparam |= FNET_TCP_AP_SEND_WITH_DELAY;
        }

        return FNET_TRUE;
    }

    result = FNET_FALSE; /* The data is not added to the buffer.*/

#if !FNET_CFG_TCP_DISCARD_OUT_OF_ORDER
    {
        fnet_netbuf_t   *buf, *prevbuf;
        fnet_size_t     seq, size;


        /* Add to the temporary input buffer.*/
        if( ((cb->tcpcb_count + insegment->total_length) <= (cb->tcpcb_rcvcountmax))
            || (fnet_ntohl(FNET_TCP_SEQ(insegment)) == cb->tcpcb_sndack))
        {
            /* Acknowledgement must be sent immediately.*/
            *ackparam |= FNET_TCP_AP_SEND_IMMEDIATELLY;

            /* Data is added to the buffer.*/
            result = FNET_TRUE;

            /* Add the segment to the temporary buffer (with sorting).*/
            if(cb->tcpcb_rcvchain)
            {
                fnet_flag_t nextchain = FNET_TRUE;

                buf = cb->tcpcb_rcvchain;
                prevbuf = 0;

                while(nextchain)
                {
                    if(FNET_TCP_COMP_G(fnet_ntohl(FNET_TCP_SEQ(buf)), fnet_ntohl(FNET_TCP_SEQ(insegment))))
                    {
                        if(prevbuf)
                        {
                            prevbuf->next_chain = insegment;
                        }
                        else
                        {
                            cb->tcpcb_rcvchain = insegment;
                        }

                        insegment->next_chain = buf;

                        nextchain = FNET_FALSE;
                    }
                    else if(!buf->next_chain)
                    {
                        buf->next_chain = insegment;

                        nextchain = FNET_FALSE;
                    }
                    else
                    {
                        prevbuf = buf;
                        buf = buf->next_chain;
                    }
                }
            }
            else
            {
                cb->tcpcb_rcvchain = insegment;
            }

            cb->tcpcb_count += insegment->total_length;
        }

        /* If the temporary buffer received the lost segment
         * move the data from the temporary buffer to the input buffer of the socket.*/
        if(fnet_ntohl(FNET_TCP_SEQ(insegment)) == cb->tcpcb_sndack)
        {
            seq = cb->tcpcb_sndack;

            while(cb->tcpcb_rcvchain)
            {
                if(FNET_TCP_COMP_GE(seq, fnet_ntohl(FNET_TCP_SEQ(cb->tcpcb_rcvchain))))
                {
                    if(FNET_TCP_COMP_GE(fnet_ntohl(FNET_TCP_SEQ(cb->tcpcb_rcvchain)) + cb->tcpcb_rcvchain->total_length
                                        - (fnet_size_t)FNET_TCP_LENGTH(cb->tcpcb_rcvchain),
                                        seq))
                    {

                        /* Receive the size of the repeated segment.*/
                        size = _fnet_tcp_get_size(fnet_ntohl(FNET_TCP_SEQ(cb->tcpcb_rcvchain)), seq);

                        /* Receive the new sequnce number.*/
                        seq = fnet_ntohl(FNET_TCP_SEQ(cb->tcpcb_rcvchain)) + cb->tcpcb_rcvchain->total_length
                              - (fnet_size_t)FNET_TCP_LENGTH(cb->tcpcb_rcvchain);

                        /* Set the size of the temporary buffer.*/
                        cb->tcpcb_count -= cb->tcpcb_rcvchain->total_length;

                        buf = cb->tcpcb_rcvchain;
                        cb->tcpcb_rcvchain = cb->tcpcb_rcvchain->next_chain;
                        buf->next_chain = 0;

#if FNET_CFG_TCP_URGENT
                        if(FNET_TCP_FLAGS(buf) & FNET_TCP_SGT_URG)
                        {
                            /* Process the urgent data.*/
                            _fnet_tcp_process_urg(sk, &buf, size, ackparam);
                        }
                        else

                        {
                            /* Pull the receive urgent pointer
                             * along with the receive window */
                            cb->tcpcb_rcvurgseq = cb->tcpcb_sndack - 1;
                        }
#endif

                        /* Process the final segment.*/
                        if((FNET_TCP_FLAGS(buf) & FNET_TCP_SGT_FIN) != 0u)
                        {
                            _fnet_tcp_process_fin(sk, fnet_ntohl(FNET_TCP_ACK(buf)));
                            *ackparam |= FNET_TCP_AP_FIN_ACK;
                            seq++;
                        }

                        /* Delete the header and repeated part.*/
                        _fnet_netbuf_trim(&buf, (fnet_int32_t)((fnet_size_t)FNET_TCP_LENGTH(buf) + size));

                        /* Add the data.*/
                        if(buf)
                        {
                            sk->receive_buffer.count += buf->total_length;
                            sk->receive_buffer.net_buf_chain = _fnet_netbuf_concat(sk->receive_buffer.net_buf_chain, buf);
                        }

                        /* Set the  new acknowledgment number.*/
                        cb->tcpcb_sndack = seq;
                    }
                    else
                    {
                        /* Delete the repeated segment.*/
                        buf = cb->tcpcb_rcvchain;
                        cb->tcpcb_rcvchain = cb->tcpcb_rcvchain->next_chain;

                        /* Set the new size of the temporary buffer.*/
                        cb->tcpcb_count -= cb->tcpcb_rcvchain->total_length;
                        _fnet_netbuf_free_chain(buf);
                    }
                }
                else
                {
                    break;
                }
            }
        }
    }
#endif

    return result;
}

/***********************************************************************
* DESCRIPTION: This function sends the data that is allowed by the windows
*              (The congestion window and the window of another side).
*
* RETURNS: TRUE if some data are sent. Otherwise
*          this function returns FALSE.
*************************************************************************/
static fnet_bool_t _fnet_tcp_send_anydata( fnet_socket_if_t *sk, fnet_bool_t oneexec )
{
    fnet_tcp_control_t  *cb = (fnet_tcp_control_t *)sk->protocol_control;
    fnet_int32_t        sndwnd;     /* Size of the data that can be sent.*/
    fnet_uint32_t       cwnd;       /* Congestion window.*/
    fnet_size_t         sntdata;    /* Size of data that is sent.*/
    fnet_int32_t        wnd;        /* Windows */
    fnet_int32_t        datasize;   /* Size of the data that can be sent from the output buffer.*/
    fnet_bool_t         result = FNET_FALSE;

    /* Reset the silly window avoidance flag.*/
    cb->tcpcb_flags &= ~FNET_TCP_CBF_SEND_TIMEOUT;

    /* Receive the size of the data that is sent.*/
    sntdata = _fnet_tcp_get_size(cb->tcpcb_rcvack, cb->tcpcb_sndseq);

    /* Receive the size of the data that is allowed by the window.*/
    wnd = (fnet_int32_t)(cb->tcpcb_sndwnd - sntdata);

    if(wnd < 0)
    {
        /* The window is shrinked.*/
        cb->tcpcb_timing_state = TCP_TS_SEGMENT_LOST;
        cb->tcpcb_sndseq += (fnet_uint32_t)wnd;
        sntdata += (fnet_uint32_t)wnd;
        wnd = 0;
    }

    /* The size of the data in the output buffer that can be sent.*/
    datasize = (fnet_int32_t)(sk->send_buffer.count - sntdata);

    /* Congestion window.*/
    cwnd = cb->tcpcb_cwnd - sntdata;
    cwnd = ((fnet_uint32_t)(cwnd / cb->tcpcb_sndmss)) * cb->tcpcb_sndmss;

    /* Calculate sndwnd (size of the data that will be sent).*/
    if(wnd > (fnet_int32_t)cwnd)
    {
        sndwnd = (fnet_int32_t)cwnd;
    }
    else
    {
        sndwnd = wnd;
    }

    if(sndwnd > datasize)
    {
        sndwnd = datasize;
    }

    /* If the data can't be sent.*/
    if(sndwnd <= 0)
    {
        /* Send the final segment without data.*/
        if((sk->send_buffer.is_shutdown) && (!datasize) && ((cb->tcpcb_flags & FNET_TCP_CBF_FIN_SENT) == 0u))
        {
            _fnet_tcp_send_headseg(sk, FNET_TCP_SGT_FIN | FNET_TCP_SGT_ACK, 0, 0u);

            /* Reinitialize the retransmission timer.*/
            if(cb->tcpcb_timers.retransmission == FNET_TCP_TIMER_OFF)
            {
                cb->tcpcb_timers.retransmission = cb->tcpcb_rto;
            }

            result = FNET_TRUE;
        }

        return result;
    }

    /* The sntdata variable will include the size of the data that is sent.*/
    sntdata = 0u;

    while(sndwnd > 0)
    {
        /* Sending of the maximal size segment.*/
        if((fnet_int32_t)cb->tcpcb_sndmss <= sndwnd)
        {
            /* Send the full sized segment.*/
            _fnet_tcp_send_dataseg(sk, 0, 0u, (fnet_size_t)cb->tcpcb_sndmss);

            sndwnd -= (fnet_int32_t)cb->tcpcb_sndmss;
            sntdata += cb->tcpcb_sndmss;
        }
        else
        {
            /*The sending of the partial segments is occured in cases:
             * If the data must be sent in any cases
             * If the urgent data is present
             * If the size of the input buffer of another side greater than the half of its maximal size
             * If the Nagle algorithm is blocked and all data can be sent in this segment
             * If all sent data is acknowledged and all data can be sent in this segment.*/
            if((sndwnd > (fnet_int32_t)(cb->tcpcb_maxwnd >> 1))
               || ((cb->tcpcb_flags & FNET_TCP_CBF_FORCE_SEND) != 0u)
#if FNET_CFG_TCP_URGENT
               || FNET_TCP_COMP_GE(cb->tcpcb_sndurgseq, cb->tcpcb_sndseq)
#endif /* FNET_CFG_TCP_URGENT */
              )
            {
                /* Send the partial segment.*/
                _fnet_tcp_send_dataseg(sk, 0, 0u, (fnet_uint32_t)sndwnd);
                sntdata += (fnet_size_t)sndwnd;
            }
            else
            {
                if((cb->tcpcb_rcvack == cb->tcpcb_sndseq) || (sk->options.tcp_opt.tcp_nodelay != FNET_FALSE))
                {
                    if(sntdata + (fnet_size_t)sndwnd == (fnet_size_t)datasize)
                    {
                        /* Send the partial segment.*/
                        _fnet_tcp_send_dataseg(sk, 0, 0u, (fnet_uint32_t)sndwnd);
                        sntdata += (fnet_size_t)sndwnd;
                    }
                    else
                    {
                        /* Set the silly window avoidance flag.*/
                        if((cb->tcpcb_timers.persist == FNET_TCP_TIMER_OFF)
                           || (cb->tcpcb_timers.persist > cb->tcpcb_rto))
                        {
                            cb->tcpcb_cprto = cb->tcpcb_rto;
                            cb->tcpcb_timers.persist = cb->tcpcb_cprto;
                        }

                        cb->tcpcb_flags |= FNET_TCP_CBF_SEND_TIMEOUT;
                    }
                }
            }

            sndwnd = 0;
        }

        /* If the execution of the function must be without delay, return.*/
        if(oneexec)
        {
            break;
        }
    }

    /* If the data is sent.*/
    if(sntdata > 0u)
    {
        /* Process the states of round trip time measurement.*/
        if((cb->tcpcb_timing_state == TCP_TS_ACK_RECEIVED) || ((cb->tcpcb_timing_state == TCP_TS_SEGMENT_LOST)
                && (FNET_TCP_COMP_G(cb->tcpcb_sndseq, cb->tcpcb_timingack))))
        {
            cb->tcpcb_timingack = cb->tcpcb_sndseq;
            cb->tcpcb_timers.round_trip = FNET_TCP_TIMER_ON_INCREMENT;

            cb->tcpcb_timing_state = TCP_TS_SEGMENT_SENT;
        }

        result = FNET_TRUE;
        cb->tcpcb_timers.persist = FNET_TCP_TIMER_OFF;

        /* Reinitialize the retransmission timer.*/
        if(cb->tcpcb_timers.retransmission == FNET_TCP_TIMER_OFF)
        {
            cb->tcpcb_timers.retransmission = cb->tcpcb_rto;
        }
    }

    return result;
}

#if FNET_CFG_TCP_URGENT
/***********************************************************************
* DESCRIPTION: This function processes the urgent data.
*************************************************************************/
static void _fnet_tcp_process_urg( fnet_socket_if_t *sk, fnet_netbuf_t **segment, fnet_size_t repdatasize, fnet_flag_t *ackparam )
{
    fnet_tcp_control_t  *cb = (fnet_tcp_control_t *)sk->protocol_control;
    fnet_uint16_t      upointer;
    fnet_uint16_t      uoffset;
    fnet_netbuf_t       *buf;
    fnet_uint32_t       tcp_seq;

    buf = (fnet_netbuf_t *) *segment;

    /* Receive the urgent pointer.*/
    upointer = fnet_ntohs(FNET_TCP_URG(buf));

    /* Different interpretation of the urgent pointer in the TCP header.*/
    if(sk->options.tcp_opt.tcp_bsd == FNET_TRUE)
    {
        /* BSD interpretation.*/
        if(!upointer)
            return;

        upointer--;
    }

    /*  Only  the new data must be processed.*/
    if(upointer < repdatasize || (repdatasize && (buf->total_length - FNET_TCP_LENGTH(buf) <= repdatasize)))
        return;

    tcp_seq = fnet_ntohl(FNET_TCP_SEQ(buf));

    /* Receive the new urgent pointer.*/
    if(FNET_TCP_COMP_G(tcp_seq + upointer, cb->tcpcb_rcvurgseq))
        cb->tcpcb_rcvurgseq = tcp_seq + upointer;
    else
        upointer = (fnet_uint16_t)(cb->tcpcb_rcvurgseq - tcp_seq);

    /* If the urgent byte is not in this segment.*/
    if(upointer >= buf->total_length - FNET_TCP_LENGTH(buf))
    {
        return;
    }

    /* Set the number of bytes before the urgent character.*/
    cb->tcpcb_rcvurgmark = (fnet_int32_t)(sk->receive_buffer.count + upointer);

    /* If the urgent data must be in the stream, return from this function. */
    if(sk->options.so_oobinline == FNET_TRUE)
    {
        return;
    }

    /* Find and read the urgent byte.*/
    uoffset = (fnet_uint16_t)(upointer + FNET_TCP_LENGTH(buf));

    while(uoffset >= buf->length)
    {
        uoffset -= buf->length;
        buf = buf->next;
    }

    cb->tcpcb_flags |= FNET_TCP_CBF_RCVURGENT;
    cb->tcpcb_iobc = (fnet_uint8_t)FNET_TCP_GETUCHAR(buf->data_ptr, uoffset);
    cb->tcpcb_sndack++;

    /* Delete the urgent byte from the stream.*/
    _fnet_netbuf_cut_center(segment, upointer + FNET_TCP_LENGTH((fnet_netbuf_t *)(*segment)), FNET_TRUE );

    /* Acknowledgment must be sent immediately.*/
    *ackparam |= FNET_TCP_AP_SEND_IMMEDIATELLY;
}
#endif /* FNET_CFG_TCP_URGENT */

/***********************************************************************
* DESCRIPTION: This function processes the final segment.
*************************************************************************/
static void _fnet_tcp_process_fin( fnet_socket_if_t *sk, fnet_uint32_t ack )
{
    /* Initialize the pointer to the control block.*/
    fnet_tcp_control_t  *cb = (fnet_tcp_control_t *)sk->protocol_control;

    /* Process the final segment depend on state.*/
    switch(cb->tcpcb_connection_state)
    {
        case FNET_TCP_CS_ESTABLISHED:
            cb->tcpcb_connection_state = FNET_TCP_CS_CLOSE_WAIT;
            /*RStevens: The connection can remain in this state forever.
             * The other end is still in the CLOSE_WAIT state, and can remain
             * there forever, until the application decides to issue its close.*/

            sk->state = SS_CLOSING;
            break;

        case FNET_TCP_CS_FIN_WAIT_1:
        case FNET_TCP_CS_FIN_WAIT_2:
            if( (cb->tcpcb_connection_state == FNET_TCP_CS_FIN_WAIT_1)
                && (ack != cb->tcpcb_sndseq) )
            {
                /* Simultaneous close.*/
                cb->tcpcb_connection_state = FNET_TCP_CS_CLOSING;
                break;
            }

            cb->tcpcb_connection_state = FNET_TCP_CS_TIME_WAIT;
            /* Set timewait timeout.*/
            if((cb->tcpcb_timers.connection == FNET_TCP_TIMER_OFF) || (cb->tcpcb_timers.connection == 0u)) /* If it was not already set before by other state. */
            {
                cb->tcpcb_timers.connection = FNET_TCP_TIME_WAIT;
            }

            cb->tcpcb_timers.keepalive = FNET_TCP_TIMER_OFF;

            sk->state = SS_CLOSING;
            break;

        default:
            return;
    }

    cb->tcpcb_flags |= FNET_TCP_CBF_FIN_RCVD;
}

/***********************************************************************
* DESCRIPTION: This function receives a new window size.
*
* RETURNS: The new window.
*************************************************************************/
static fnet_uint32_t _fnet_tcp_get_rcvwnd( fnet_socket_if_t *sk )
{
    fnet_int32_t    wnd;
    fnet_uint32_t   rcvwnd;

    /* Initialize the pointer to the control block.*/
    fnet_tcp_control_t *cb = (fnet_tcp_control_t *)sk->protocol_control;

    rcvwnd = cb->tcpcb_rcvwnd;

    /* Set the receive window size.*/
    wnd = (fnet_int32_t)(cb->tcpcb_rcvcountmax - sk->receive_buffer.count);

    if(wnd < 0)
    {
        wnd = 0;
    }

    if(rcvwnd < (fnet_uint32_t)wnd)
    {
        /* Window can be opened only on condition that*/
        if((((fnet_uint32_t)wnd - rcvwnd) >= cb->tcpcb_rcvmss) || (((fnet_uint32_t)wnd - rcvwnd) >= (cb->tcpcb_rcvcountmax >> 1)) || (!rcvwnd))
        {
            rcvwnd = (fnet_uint32_t)wnd;
        }
    }
    else
    {
        rcvwnd = (fnet_uint32_t)wnd;
    }

    return rcvwnd;
}

/************************************************************************
* DESCRIPTION: This function processes the timeouts.
*              (it is performed every 500 ms).
*************************************************************************/
static void _fnet_tcp_slowtimo(fnet_uint32_t cookie)
{
    fnet_socket_if_t *sk;
    fnet_socket_if_t *addedsk;
    fnet_socket_if_t *nextsk;

    FNET_COMP_UNUSED_ARG(cookie);

    fnet_isr_lock();
    sk = fnet_tcp_prot_if.head;

    while(sk)
    {
        addedsk = sk->partial_con;

        while(addedsk)
        {
            nextsk = addedsk->next;
            /* Process the partial sockets.*/
            _fnet_tcp_slowtimosk(addedsk);
            addedsk = nextsk;
        }

        addedsk = sk->incoming_con;

        while(addedsk)
        {
            nextsk = addedsk->next;
            /* Process the incoming sockets.*/
            _fnet_tcp_slowtimosk(addedsk);
            addedsk = nextsk;
        }

        /* Processg the main socket.*/
        nextsk = sk->next;
        _fnet_tcp_slowtimosk(sk);
        sk = nextsk;
    }

    _fnet_tcp_initial_seq_number_update();

    fnet_isr_unlock();
}

/************************************************************************
* DESCRIPTION: This function processes the timeouts
*              (it is performed every FNET_TCP_FAST_TIMER_PERIOD_MS).
*************************************************************************/
static void _fnet_tcp_fasttimo( fnet_uint32_t cookie )
{
    fnet_socket_if_t *sk;
    fnet_socket_if_t *addedsk;

    FNET_COMP_UNUSED_ARG(cookie);

    fnet_isr_lock();
    sk = fnet_tcp_prot_if.head;

    while(sk)
    {
        _fnet_tcp_fasttimosk(sk);
        addedsk = sk->incoming_con;

        while(addedsk)
        {
            _fnet_tcp_fasttimosk(addedsk);
            addedsk = addedsk->next;
        }

        sk = sk->next;
    }

    fnet_isr_unlock();
}

/************************************************************************
* DESCRIPTION: This function processes the timers of the socket
*              (it is performed every FNET_TCP_SLOW_TIMER_PERIOD_MS).
*************************************************************************/
static void _fnet_tcp_slowtimosk( fnet_socket_if_t *sk )
{
    fnet_tcp_control_t *cb = (fnet_tcp_control_t *)sk->protocol_control;

    /* If the socket is not connected, return.*/
    if(sk->state != SS_CLOSED)
    {
        /* Check the abort timer.*/
        if(cb->tcpcb_timers.abort != FNET_TCP_TIMER_OFF)
        {
            cb->tcpcb_timers.abort--;

            if(!cb->tcpcb_timers.abort)
            {
                cb->tcpcb_timers.abort = FNET_TCP_TIMER_OFF;

                if(sk->options.local_error != FNET_ERR_HOSTUNREACH)
                {
                    sk->options.local_error = FNET_ERR_CONNABORTED;
                }
                _fnet_tcp_close_socket(sk);
                return;
            }
        }

        /* Check the connection timer.*/
        if(cb->tcpcb_timers.connection != FNET_TCP_TIMER_OFF)
        {
            cb->tcpcb_timers.connection--;

            if(!cb->tcpcb_timers.connection)
            {
                cb->tcpcb_timers.connection = FNET_TCP_TIMER_OFF;

                if((cb->tcpcb_flags & FNET_TCP_CBF_CLOSE) != 0u)
                {
                    if(cb->tcpcb_connection_state == FNET_TCP_CS_TIME_WAIT)
                    {
                        _fnet_tcp_close_socket(sk);
                    }
                    else
                    {
                        _fnet_tcp_abort_socket(sk);
                    }
                }
                else
                {
                    if(((sk->options.local_error) != FNET_ERR_HOSTUNREACH)
                       && ((sk->options.local_error) != FNET_ERR_NOPROTOOPT))
                    {
                        sk->options.local_error = FNET_ERR_TIMEDOUT;
                    }

                    _fnet_tcp_close_socket(sk);
                }
                return;
            }
        }

        /* Check the retransmission timer.*/
        if(cb->tcpcb_timers.retransmission != FNET_TCP_TIMER_OFF)
        {
            cb->tcpcb_timers.retransmission--;

            if(!cb->tcpcb_timers.retransmission)
            {
                cb->tcpcb_timers.retransmission = FNET_TCP_TIMER_OFF;
                _fnet_tcp_retransmission_timeo(sk);
            }
        }

        /* Check the keepalive timer.*/
        if(cb->tcpcb_timers.keepalive != FNET_TCP_TIMER_OFF)
        {
            cb->tcpcb_timers.keepalive--;

            if(!cb->tcpcb_timers.keepalive)
            {
                cb->tcpcb_timers.keepalive = FNET_TCP_TIMER_OFF;
                _fnet_tcp_keepalive_timeo(sk);
            }
        }

        /* Check the persist timer.*/
        if(cb->tcpcb_timers.persist != FNET_TCP_TIMER_OFF)
        {
            cb->tcpcb_timers.persist--;

            if(!cb->tcpcb_timers.persist)
            {
                cb->tcpcb_timers.persist = FNET_TCP_TIMER_OFF;
                _fnet_tcp_persist_timeo(sk);
            }
        }

        /* If the round trip timer is turned on, recalculate it.*/
        if(cb->tcpcb_timers.round_trip != FNET_TCP_TIMER_OFF)
        {
            cb->tcpcb_timers.round_trip++;
        }
    }
}

/************************************************************************
* DESCRIPTION: This function processes the delayed acknowledgment timer
*              of the socket (it is performed every FNET_TCP_FAST_TIMER_PERIOD_MS).
*************************************************************************/
static void _fnet_tcp_fasttimosk( fnet_socket_if_t *sk )
{
    fnet_tcp_control_t *cb = (fnet_tcp_control_t *)sk->protocol_control;

    if(sk->state != SS_CLOSED)
    {
        /* Check the delayed acknowledgment timer.*/
        if(cb->tcpcb_timers.delayed_ack != FNET_TCP_TIMER_OFF)
        {
            cb->tcpcb_timers.delayed_ack--;

            if(!cb->tcpcb_timers. delayed_ack)
            {
                _fnet_tcp_send_ack(sk);
            }
        }
    }
}

/************************************************************************
* DESCRIPTION: This function processes the timeout of the retransmission
*              timer.
*************************************************************************/
static void _fnet_tcp_retransmission_timeo( fnet_socket_if_t *sk )
{
    fnet_tcp_control_t  *cb = (fnet_tcp_control_t *)sk->protocol_control;
    fnet_uint8_t        options[FNET_TCP_MAX_OPT_SIZE];
    fnet_uint8_t        optionlen;

    switch(cb->tcpcb_connection_state)
    {
        case FNET_TCP_CS_SYN_RCVD:
            /* Reinitialize of the sequence number.*/
            cb->tcpcb_sndseq--;
            _fnet_tcp_set_synopt(sk, options, &optionlen);

            /* Create and send the SYN segment.*/
            _fnet_tcp_send_headseg(sk, FNET_TCP_SGT_SYN | FNET_TCP_SGT_ACK, options, optionlen);
            break;
        case FNET_TCP_CS_SYN_SENT:
            /* Retransmission of the first segment.*/

            /* Recalculate the sequence number.*/
            cb->tcpcb_sndseq--;

            _fnet_tcp_set_synopt(sk, options, &optionlen);

            /* Send the SYN segment.*/
            _fnet_tcp_send_headseg(sk, FNET_TCP_SGT_SYN, options, optionlen);
            break;
        default:
            /* If FIN segment is sent, it must be retransmitted.*/
            cb->tcpcb_flags &= ~FNET_TCP_CBF_FIN_SENT;

            /* Initialize of the abort timer.*/
            if(cb->tcpcb_timers.abort == FNET_TCP_TIMER_OFF)
            {
                cb->tcpcb_timers.abort = FNET_TCP_ABORT_INTERVAL;
            }

            /* Recalculate the sequence number.*/
            cb->tcpcb_sndseq = cb->tcpcb_rcvack;

            /* Recalculate the congestion window and slow start threshold values (for case of retransmission).*/
            if(cb->tcpcb_cwnd > cb->tcpcb_sndwnd)
            {
                cb->tcpcb_ssthresh = cb->tcpcb_sndwnd >> 1;
            }
            else
            {
                cb->tcpcb_ssthresh = cb->tcpcb_cwnd >> 1;
            }

            if(cb->tcpcb_ssthresh < ((fnet_uint32_t)cb->tcpcb_sndmss << 1))
            {
                cb->tcpcb_ssthresh = ((fnet_uint32_t)cb->tcpcb_sndmss << 1);
            }

            cb->tcpcb_cwnd = cb->tcpcb_sndmss;

            /* Round trip time can't be measured in this case.*/
            cb->tcpcb_timers.round_trip = FNET_TCP_TIMER_OFF;
            cb->tcpcb_timing_state = TCP_TS_SEGMENT_LOST;

            cb->tcpcb_flags |= FNET_TCP_CBF_FORCE_SEND;
            _fnet_tcp_send_anydata(sk, FNET_FALSE);
            cb->tcpcb_flags &= ~FNET_TCP_CBF_FORCE_SEND;
            break;
    }

    /* If the first timeout is occured, initialize the retransmission timer.*/
    if(cb->tcpcb_retrseq != cb->tcpcb_rcvack)
    {
        cb->tcpcb_crto = cb->tcpcb_rto;
        cb->tcpcb_retrseq = cb->tcpcb_rcvack;
    }

    /* Recalculate the retransission timer.*/
    if(cb->tcpcb_crto != FNET_TCP_TIMERS_LIMIT)
    {
        if((cb->tcpcb_crto << 1) > FNET_TCP_TIMERS_LIMIT)
        {
            /* Timeout must be less or equal than TIMER_LIMIT.*/
            cb->tcpcb_crto = FNET_TCP_TIMERS_LIMIT;
        }
        else
        {
            /* Increase the timeout (*2).*/
            cb->tcpcb_crto <<= 1;
        }
    }

    cb->tcpcb_timers.retransmission = cb->tcpcb_crto;
}

/************************************************************************
* DESCRIPTION: This function processes the timeout of the keepalive
*              timer.
*************************************************************************/
static void _fnet_tcp_keepalive_timeo( fnet_socket_if_t *sk )
{
    fnet_netbuf_t           *data;
    fnet_uint16_t           rcvwnd;
    fnet_tcp_control_t      *cb = (fnet_tcp_control_t *)sk->protocol_control;
    struct fnet_tcp_segment segment;

    /* Create the keepalive segment.*/
    data = _fnet_netbuf_new(1u, FNET_FALSE);

    /* Check the memory allocation.*/
    if(!data)
    {
        return;
    }

    /* Receive the window size.*/
    rcvwnd = (fnet_uint16_t)(_fnet_tcp_get_rcvwnd(sk) >> cb->tcpcb_recvscale);

    /* Send the segment.*/
    segment.sockoption = &sk->options;
    /* segment.src_addr = sk->local_addr */
    fnet_memcpy(&segment.src_addr, &sk->local_addr, sizeof(segment.src_addr));
    /* segment.dest_addr = sk->foreign_addr */
    fnet_memcpy(&segment.dest_addr, &sk->foreign_addr, sizeof(segment.dest_addr));
    segment.seq = cb->tcpcb_rcvack - 1u;
    segment.ack = cb->tcpcb_sndack;
    segment.flags = FNET_TCP_SGT_ACK;
    segment.wnd = rcvwnd;
    segment.urgpointer = 0u;
    segment.options = 0;
    segment.optlen = 0u;
    segment.data = data;

    _fnet_tcp_send_seg(&segment);    /* TBD res check */

    /* Set the timers.*/
    cb->tcpcb_timers.keepalive = sk->options.tcp_opt.keep_intvl;

    if((cb->tcpcb_timers.abort == FNET_TCP_TIMER_OFF) || (cb->tcpcb_timers.abort > (sk->options.tcp_opt.keep_cnt * sk->options.tcp_opt.keep_intvl) ))
    {
        cb->tcpcb_timers.abort = sk->options.tcp_opt.keep_cnt * sk->options.tcp_opt.keep_intvl;
    }
}

/************************************************************************
* DESCRIPTION: This function processes the timeout of the persist
*              timer.
*************************************************************************/
static void _fnet_tcp_persist_timeo( fnet_socket_if_t *sk )
{
    fnet_tcp_control_t *cb = (fnet_tcp_control_t *)sk->protocol_control;

    /* If the window is closed, send the probe segment
     * Otherwise, try to send any data.*/
    if(!cb->tcpcb_sndwnd)
    {
        _fnet_tcp_send_dataseg(sk, 0, 0u, 1u);
        cb->tcpcb_sndseq--;
    }
    else
    {
        cb->tcpcb_flags |= FNET_TCP_CBF_FORCE_SEND;
        _fnet_tcp_send_anydata(sk, FNET_FALSE);
        cb->tcpcb_flags &= ~FNET_TCP_CBF_FORCE_SEND;
        return;
    }

    /* Reinitialize the persist timer.*/
    if(cb->tcpcb_cprto != FNET_TCP_TIMERS_LIMIT)
    {
        if((cb->tcpcb_cprto << 1) > FNET_TCP_TIMERS_LIMIT)
        {
            /* Timeout must be less or equal than TIMER_LIMIT.*/
            cb->tcpcb_cprto = FNET_TCP_TIMERS_LIMIT;
        }
        else
        {
            /* Increase the timeout (*2).*/
            cb->tcpcb_cprto <<= 1;
        }
    }

    cb->tcpcb_timers.persist = cb->tcpcb_cprto;

    /* Initialize the abort timer.*/
    if(cb->tcpcb_timers.abort == FNET_TCP_TIMER_OFF)
    {
        cb->tcpcb_timers.abort = FNET_TCP_ABORT_INTERVAL;
    }
}

/************************************************************************
* DESCRIPTION: This function sends the segment.
*
* RETURNS: FNET_OK if the segment is sent. Otherwise
*          this function returns the error code.
*************************************************************************/
static fnet_error_t _fnet_tcp_send_seg(struct fnet_tcp_segment *segment)
{
    fnet_netbuf_t                           *nb;
    fnet_error_t                            error = FNET_ERR_OK;
    fnet_netif_t                            *netif;
    FNET_COMP_PACKED_VAR fnet_uint16_t      *checksum_p;

    netif = _fnet_netif_get_by_scope_id( segment->dest_addr.sa_scope_id );

    /* Create the header.*/
    nb = _fnet_netbuf_new(FNET_TCP_SIZE_HEADER, FNET_FALSE);

    if(!nb)
    {
        if(segment->data)
        {
            _fnet_netbuf_free_chain(segment->data);
        }

        return FNET_ERR_NOMEM;
    }

    fnet_memset_zero(nb->data_ptr, FNET_TCP_SIZE_HEADER);

    /* Add TCP options.*/
    if((segment->options) && (segment->optlen))
    {
        error = _fnet_tcp_addopt(nb, (fnet_size_t)segment->optlen, segment->options);

        if(error)
        {
            _fnet_netbuf_free_chain(nb);

            if(segment->data)
            {
                _fnet_netbuf_free_chain(segment->data);
            }

            return error;
        }
    }
    else
    {
        FNET_TCP_SET_LENGTH(nb) = FNET_TCP_SIZE_HEADER << 2;  /* (FNET_TCP_SIZE_HEADER/4 + opt_len/4) */
    }

    /* Initialization of the header.*/
    FNET_TCP_SPORT(nb) = segment->src_addr.sa_port;
    FNET_TCP_DPORT(nb) = segment->dest_addr.sa_port;
    FNET_TCP_SEQ(nb) = fnet_htonl(segment->seq);
    FNET_TCP_ACK(nb) = fnet_htonl(segment->ack);
    FNET_TCP_SET_FLAGS(nb) = segment->flags;
    FNET_TCP_WND(nb) = fnet_htons(segment->wnd);

    /* Add the data.*/
    nb = _fnet_netbuf_concat(nb, segment->data);

    /* Set the pointer to the urgent data.*/
    FNET_TCP_URG(nb) = fnet_htons(segment->urgpointer);

    /* Checksum calculation.*/
    FNET_TCP_CHECKSUM(nb) = 0u;

#if FNET_CFG_UDP_CHECKSUM

#if FNET_CFG_CPU_ETH_HW_TX_IP_CHECKSUM
    if( 0
#if FNET_CFG_IP4
        || ( (segment->dest_addr.sa_family == AF_INET)
             && (netif || ((netif = _fnet_ip4_route(((struct fnet_sockaddr_in *)(&segment->dest_addr))->sin_addr.s_addr)) != FNET_NULL))
             && (netif->features & FNET_NETIF_FEATURE_HW_TX_PROTOCOL_CHECKSUM)
             && (_fnet_ip4_will_fragment(netif, nb->total_length) == FNET_FALSE) /* Fragmented packets are not inspected.*/  )
#endif
#if FNET_CFG_IP6
        || ( (segment->dest_addr.sa_family == AF_INET6)
             && (netif || ((netif = _fnet_ip6_route(&((struct fnet_sockaddr_in6 *)(&segment->src_addr))->sin6_addr.s6_addr, &((struct fnet_sockaddr_in6 *)(&segment->dest_addr))->sin6_addr.s6_addr)) != FNET_NULL ) )
             && (netif->features & FNET_NETIF_FEATURE_HW_TX_PROTOCOL_CHECKSUM)
             && (_fnet_ip6_will_fragment(netif, nb->total_length) == FNET_FALSE) /* Fragmented packets are not inspected.*/  )
#endif
      )
    {
        nb->flags |= FNET_NETBUF_FLAG_HW_PROTOCOL_CHECKSUM;
        checksum_p = 0;
    }
    else
#endif /* FNET_CFG_CPU_ETH_HW_TX_IP_CHECKSUM */
    {
        FNET_TCP_CHECKSUM(nb) = _fnet_checksum_pseudo_netbuf_start(nb, FNET_HTONS((fnet_uint16_t)FNET_PROT_TCP), (fnet_uint16_t)nb->total_length);
        checksum_p = &FNET_TCP_CHECKSUM(nb);
    }
#endif /* FNET_CFG_UDP_CHECKSUM */

#if FNET_CFG_IP4
    if(segment->dest_addr.sa_family == AF_INET)
    {
        error = _fnet_ip4_output(netif, ((struct fnet_sockaddr_in *)(&segment->src_addr))->sin_addr.s_addr,
                                 ((struct fnet_sockaddr_in *)(&segment->dest_addr))->sin_addr.s_addr,
                                 FNET_PROT_TCP,
                                 (fnet_uint8_t)(segment->sockoption ? segment->sockoption->ip4_opt.tos : 0u),
                                 (fnet_uint8_t)(segment->sockoption ? segment->sockoption->ip4_opt.ttl : FNET_TCP_TTL_DEFAULT),
                                 nb, FNET_FALSE,
                                 (segment->sockoption ? segment->sockoption->so_dontroute : FNET_FALSE),
                                 checksum_p);
    }
    else
#endif
#if FNET_CFG_IP6
        if(segment->dest_addr.sa_family == AF_INET6)
        {
            error = _fnet_ip6_output( netif,
                                      &((struct fnet_sockaddr_in6 *)(&segment->src_addr))->sin6_addr.s6_addr,
                                      &((struct fnet_sockaddr_in6 *)(&segment->dest_addr))->sin6_addr.s6_addr,
                                      FNET_PROT_TCP,
                                      (fnet_uint8_t)(segment->sockoption ? segment->sockoption->ip6_opt.hops_unicast : 0u /*default*/),
                                      nb,
                                      checksum_p );
        }
        else
#endif
        {}

    return error;
}

/************************************************************************
* DESCRIPTION: This function sends the segment wihout data.
*
* RETURNS: TRUE if no error occurs. Otherwise
*          this function returns the error code.
*************************************************************************/
static fnet_error_t _fnet_tcp_send_headseg( fnet_socket_if_t *sk, fnet_uint8_t flags, void *options, fnet_uint8_t optlen )
{
    fnet_uint32_t   seq;
    fnet_uint16_t   wnd;
    fnet_error_t    error = FNET_ERR_OK;
    fnet_uint32_t   ack = 0u;
    fnet_uint16_t   urgpointer = 0u;
    struct fnet_tcp_segment segment;

    fnet_tcp_control_t *cb = (fnet_tcp_control_t *)sk->protocol_control;

    /* Get the sequence number.*/
    seq = cb->tcpcb_sndseq;

    /* Get the window.*/
    cb->tcpcb_rcvwnd = _fnet_tcp_get_rcvwnd(sk);

    /* Process the SYN flag.*/
    if((flags & FNET_TCP_SGT_SYN) != 0u)
    {
        seq++;

        if(cb->tcpcb_rcvwnd > FNET_TCP_MAXWIN)
        {
            wnd = FNET_TCP_MAXWIN;
        }
        else
        {
            wnd = (fnet_uint16_t)cb->tcpcb_rcvwnd;
        }
    }
    else
    {
        wnd = (fnet_uint16_t)(cb->tcpcb_rcvwnd >> cb->tcpcb_recvscale);
    }

    /* Process the FIN flag.*/
    if((flags & FNET_TCP_SGT_FIN) != 0u)
    {
        seq++;

        /* Change the state after sending of the final segment*/
        if(cb->tcpcb_connection_state == FNET_TCP_CS_ESTABLISHED)
        {
            cb->tcpcb_connection_state = FNET_TCP_CS_FIN_WAIT_1;
        }
        else if(cb->tcpcb_connection_state == FNET_TCP_CS_CLOSE_WAIT)
        {
            cb->tcpcb_connection_state = FNET_TCP_CS_LAST_ACK;
        }
        else
        {}

        cb->tcpcb_flags |= FNET_TCP_CBF_FIN_SENT;
    }

#if FNET_CFG_TCP_URGENT
    /* Process the URG flag.*/
    if((flags & FNET_TCP_SGT_URG)
       && _fnet_tcp_get_size(cb->tcpcb_sndseq, cb->tcpcb_sndurgseq) < FNET_TCP_MAXWIN)
    {
        urgpointer = (fnet_uint16_t)_fnet_tcp_get_size(cb->tcpcb_sndseq, cb->tcpcb_sndurgseq);

        if(sk->options.tcp_opt.tcp_bsd == FNET_TRUE)
        {
            urgpointer++;
        }
    }
#endif /* FNET_CFG_TCP_URGENT */

    /* Process the ACK flag.*/
    if((flags & FNET_TCP_SGT_ACK) != 0u)
    {
        ack = cb->tcpcb_sndack;
    }

    /* Send the segment.*/
    segment.sockoption = &sk->options;
    fnet_memcpy(&segment.src_addr, &sk->local_addr, sizeof(segment.src_addr));
    fnet_memcpy(&segment.dest_addr, &sk->foreign_addr, sizeof(segment.dest_addr));
    segment.seq = cb->tcpcb_sndseq;
    segment.ack = ack;
    segment.flags = flags;
    segment.wnd = wnd;
    segment.urgpointer = urgpointer;
    segment.options = options;
    segment.optlen = optlen;
    segment.data = 0;

    /* Turn off the delayed acknowledgment timer.*/
    cb->tcpcb_timers.delayed_ack = FNET_TCP_TIMER_OFF;

    error = _fnet_tcp_send_seg(&segment);

    cb->tcpcb_newfreercvsize = 0u;

    /* Set the new sequence number.*/
    cb->tcpcb_sndseq = seq;

    if(FNET_TCP_COMP_G(cb->tcpcb_sndseq, cb->tcpcb_maxrcvack))
    {
        cb->tcpcb_maxrcvack = cb->tcpcb_sndseq;
    }

    return error;
}

/************************************************************************
* DESCRIPTION: This function sends the data segment.
*
* RETURNS: TRUE if no error occurs. Otherwise
*          this function returns the error code.
*************************************************************************/
static fnet_error_t _fnet_tcp_send_dataseg( fnet_socket_if_t *sk, void *options, fnet_uint8_t optlen, fnet_size_t datasize )
{
    fnet_uint8_t            flags;
    fnet_size_t             newdatasize;    /* Datasize that can be sent.*/
    fnet_uint32_t           urgpointer = 0u; /* Pointer of the urgent data.*/
    fnet_netbuf_t           *data = 0;      /* Data pointer.*/
    fnet_uint32_t           seq;            /* Sequence number.*/
    fnet_error_t            error;
    fnet_uint32_t           tmp;
    struct fnet_tcp_segment segment;
    fnet_tcp_control_t      *cb = (fnet_tcp_control_t *)sk->protocol_control;

    /* Receive the sequence number.*/
    seq = cb->tcpcb_sndseq;

    /* Reset the flags.*/
    flags = 0u;

    /* Data that can be sent.*/
    newdatasize = (sk->send_buffer.count - _fnet_tcp_get_size(cb->tcpcb_rcvack, cb->tcpcb_sndseq));

    /* Check the data size.*/
    if(datasize > newdatasize)
    {
        datasize = (fnet_size_t)newdatasize;
    }

#if FNET_CFG_IP4
    tmp = _fnet_ip4_maximum_packet(((struct fnet_sockaddr_in *)(&sk->foreign_addr))->sin_addr.s_addr);
#else /* TBD */
    tmp = 0u;
#endif

    if((datasize + FNET_TCP_SIZE_HEADER) > tmp)
    {
        datasize = (tmp - FNET_TCP_SIZE_HEADER);
    }

    /* Create the flags.*/
    flags |= FNET_TCP_SGT_ACK;

    /* If the data of the segment is the last in the output buffer,*/
    /* Set PSH flag.*/
    if((newdatasize == datasize) && datasize)
    {
        flags |= FNET_TCP_SGT_PSH;
    }

#if FNET_CFG_TCP_URGENT
    /* Process the urgent request.*/
    if(FNET_TCP_COMP_GE(cb->tcpcb_sndurgseq, cb->tcpcb_sndseq))
    {
        if(_fnet_tcp_get_size(cb->tcpcb_sndseq, cb->tcpcb_sndurgseq) < FNET_TCP_MAX_URG_POINTER)
        {
            /* Set the urgent pointer.*/
            urgpointer = _fnet_tcp_get_size(cb->tcpcb_sndseq, cb->tcpcb_sndurgseq);
            flags |= FNET_TCP_SGT_URG;

            /* If the BSD interpretation, increase the urgent pointer.*/
            if(sk->options.tcp_opt.tcp_bsd == FNET_TRUE)
            {
                urgpointer++;
            }
        }
    }
    else
    {
        /* Pull the send urgent pointer along with the send window.*/
        if(FNET_TCP_COMP_G(cb->tcpcb_rcvack - 1, cb->tcpcb_sndurgseq))
            cb->tcpcb_sndurgseq = cb->tcpcb_rcvack - 1;
    }
#endif /* FNET_CFG_TCP_URGENT */

    /* If final segment must be sent.*/
    if(sk->send_buffer.is_shutdown)
    {
        if(newdatasize == datasize)
        {
            /* End of the data sending.*/
            flags |= FNET_TCP_SGT_FIN;
            seq++;

            /* Change the state after the sending of the final segment.*/
            if(cb->tcpcb_connection_state == FNET_TCP_CS_ESTABLISHED)
            {
                cb->tcpcb_connection_state = FNET_TCP_CS_FIN_WAIT_1;
            }
            else if(cb->tcpcb_connection_state == FNET_TCP_CS_CLOSE_WAIT)
            {
                cb->tcpcb_connection_state = FNET_TCP_CS_LAST_ACK;
            }
            else
            {}

            cb->tcpcb_flags |= FNET_TCP_CBF_FIN_SENT;
        }
    }

    /* Receive the window size.*/
    cb->tcpcb_rcvwnd = _fnet_tcp_get_rcvwnd(sk);

    /* If the data is present, add it.*/
    if(datasize)
    {
        data = _fnet_netbuf_copy(sk->send_buffer.net_buf_chain, (sk->send_buffer.count - newdatasize), datasize, FNET_FALSE);

        /* Check the memory allocation.*/
        if(!data)
        {
            cb->tcpcb_sndseq = seq + datasize;
            return FNET_ERR_NOMEM;
        }
    }

    /* Send the segment.*/
    segment.sockoption = &sk->options;
    fnet_memcpy(&segment.src_addr, &sk->local_addr, sizeof(segment.src_addr));
    fnet_memcpy(&segment.dest_addr, &sk->foreign_addr, sizeof(segment.dest_addr));
    segment.seq = cb->tcpcb_sndseq;
    segment.ack = cb->tcpcb_sndack;
    segment.flags = flags;
    segment.wnd = (fnet_uint16_t)(cb->tcpcb_rcvwnd >> cb->tcpcb_recvscale);
    segment.urgpointer = (fnet_uint16_t)urgpointer;
    segment.options = options;
    segment.optlen = optlen;
    segment.data = data;

    /* Turn off the delayed acknowledgment timer.*/
    cb->tcpcb_timers.delayed_ack = FNET_TCP_TIMER_OFF;

    error = _fnet_tcp_send_seg(&segment);

    cb->tcpcb_newfreercvsize = 0u;

    /* Set the new sequence number.*/
    cb->tcpcb_sndseq = seq + datasize;

    if(FNET_TCP_COMP_G(cb->tcpcb_sndseq, cb->tcpcb_maxrcvack))
    {
        cb->tcpcb_maxrcvack = cb->tcpcb_sndseq;
    }

    return error;
}

/************************************************************************
* DESCRIPTION: This function sends the acknowledgement segment.
*************************************************************************/
static void _fnet_tcp_send_ack( fnet_socket_if_t *sk )
{
    fnet_uint8_t        options[FNET_TCP_MAX_OPT_SIZE];
    fnet_uint8_t        optionlen;

    fnet_tcp_control_t  *cb = (fnet_tcp_control_t *)sk->protocol_control;

    switch(cb->tcpcb_connection_state)
    {
        /* In the SYN_RCVD state acknowledgement must be with the SYN flag.*/
        case FNET_TCP_CS_SYN_RCVD:
            /* Reinitialize of the sequnce number.*/
            cb->tcpcb_sndseq--;

            _fnet_tcp_set_synopt(sk, options, &optionlen);

            /* Create and send the SYN segment.*/
            _fnet_tcp_send_headseg(sk, FNET_TCP_SGT_SYN | FNET_TCP_SGT_ACK, options, optionlen);
            break;
        /* In the FIN_WAIT_1 and LAST_ACK states acknowledgement must be with the FIN flag.*/
        case FNET_TCP_CS_FIN_WAIT_1:
        case FNET_TCP_CS_LAST_ACK:

            /* Reinitialize the sequnce number.*/
            cb->tcpcb_sndseq--;

            /* Create and send the FIN segment.*/
            _fnet_tcp_send_headseg(sk, FNET_TCP_SGT_FIN | FNET_TCP_SGT_ACK, 0, 0u);
            break;
        default:
#if FNET_CFG_TCP_URGENT
            /* If the urgent data is present, set the urgent flag.*/
            if(FNET_TCP_COMP_GE(cb->tcpcb_sndurgseq, cb->tcpcb_sndseq))
            {
                _fnet_tcp_send_headseg(sk, FNET_TCP_SGT_ACK | FNET_TCP_SGT_URG, 0, 0);
            }
            else
#endif /* FNET_CFG_TCP_URGENT */
            {
                _fnet_tcp_send_headseg(sk, FNET_TCP_SGT_ACK, 0, 0u);
            }
            break;
    }

    /* Turn of the delayed acknowledgment timer.*/
    cb->tcpcb_timers.delayed_ack = FNET_TCP_TIMER_OFF;
}

/************************************************************************
* DESCRIPTION: This function sends the reset segment.
*************************************************************************/
static void _fnet_tcp_send_rst( fnet_socket_option_t *sockoption, fnet_netbuf_t *insegment,
                                struct fnet_sockaddr *src_addr, struct fnet_sockaddr *dest_addr)
{
    struct fnet_tcp_segment segment;

    if((FNET_TCP_FLAGS(insegment) & FNET_TCP_SGT_ACK) != 0u)
    {
        /* Send the reset segment without acknowledgment*/
        segment.seq = fnet_ntohl(FNET_TCP_ACK(insegment));
        segment.ack = 0u;
        segment.flags = FNET_TCP_SGT_RST;
    }
    else
    {
        /* Send the reset segment with acknowledgment*/
        segment.seq = 0u;
        segment.ack = fnet_ntohl(FNET_TCP_SEQ(insegment)) + insegment->total_length - (fnet_size_t)FNET_TCP_LENGTH(insegment)
                      + (fnet_size_t)(FNET_TCP_SGT_FIN & FNET_TCP_FLAGS(insegment)) + (fnet_size_t)((FNET_TCP_SGT_SYN & FNET_TCP_FLAGS(insegment)) >> 1);
        segment.flags = FNET_TCP_SGT_RST | FNET_TCP_SGT_ACK;
    }

    segment.sockoption = sockoption;
    fnet_memcpy(&segment.src_addr, src_addr, sizeof(segment.src_addr));
    fnet_memcpy(&segment.dest_addr, dest_addr, sizeof(segment.dest_addr));
    segment.wnd = 0u;
    segment.urgpointer = 0u;
    segment.options = 0;
    segment.optlen = 0u;
    segment.data = 0;

    _fnet_tcp_send_seg(&segment);
}

/************************************************************************
* DESCRIPTION: This function sends the reset segment.
*************************************************************************/
static void _fnet_tcp_send_rstsk( fnet_socket_if_t *sk )
{
    struct fnet_tcp_segment segment;

    /* Initialize the pointer to the control block.*/
    fnet_tcp_control_t *cb = (fnet_tcp_control_t *)sk->protocol_control;

    if((sk->state != SS_CLOSED) && (cb->tcpcb_connection_state != FNET_TCP_CS_SYN_SENT)
       && (cb->tcpcb_connection_state != FNET_TCP_CS_TIME_WAIT))
    {
        if(cb->tcpcb_connection_state == FNET_TCP_CS_SYN_RCVD)
        {
            /* Send the reset segment with acknowledgment.*/
            segment.seq = 0u;
            segment.ack = cb->tcpcb_sndack;
            segment.flags = FNET_TCP_SGT_RST | FNET_TCP_SGT_ACK;
        }
        else
        {
            /* Send the reset segment without acknowledgment.*/
            segment.seq = cb->tcpcb_rcvack;
            segment.ack = 0u;
            segment.flags = FNET_TCP_SGT_RST;
        }

        segment.sockoption = &sk->options;
        fnet_memcpy(&segment.src_addr, &sk->local_addr, sizeof(segment.src_addr));
        fnet_memcpy(&segment.dest_addr, &sk->foreign_addr, sizeof(segment.dest_addr));
        segment.wnd = 0u;
        segment.urgpointer = 0u;
        segment.options = 0;
        segment.optlen = 0u;
        segment.data = 0;

        _fnet_tcp_send_seg(&segment);
    }
}

/************************************************************************
* DESCRIPTION: This function sends the reset segment and closes the socket.
*************************************************************************/
static void _fnet_tcp_abort_socket( fnet_socket_if_t *sk )
{
    /* If the incoming or the partial socket is present, abort it.*/
    if(sk->state == SS_LISTENING)
    {
        while(sk->partial_con)
        {
            _fnet_tcp_send_rstsk(sk->partial_con);
            _fnet_tcp_close_socket(sk->partial_con);
        }

        while(sk->incoming_con)
        {
            _fnet_tcp_send_rstsk(sk->incoming_con);
            _fnet_tcp_close_socket(sk->incoming_con);
        }
    }
    else
    {
        _fnet_tcp_send_rstsk(sk);
    }

    /* Close the socket.*/
    _fnet_tcp_close_socket(sk);
}

/************************************************************************
* DESCRIPTION: This function adds the options in the segment.
*              It can be used only before the data adding and only after
*              the creating of the main header.
*
* RETURNS: If no error occurs, this function returns FNET_OK. Otherwise,
*          it returns FNET_ERR.
*************************************************************************/
static fnet_error_t _fnet_tcp_addopt( fnet_netbuf_t *segment, fnet_size_t len, void *data )
{
    fnet_netbuf_t   *buf;
    fnet_index_t    i;
    fnet_size_t     size;   /* Size of options.*/

    /* Create the buffer of the options.*/
    buf = _fnet_netbuf_new(FNET_TCP_SIZE_OPTIONS, FNET_FALSE);

    /* Check the memory allocatio.*/
    if(!buf)
    {
        return FNET_ERR_NOMEM;
    }

    /* Reset the buffer and add it to the header.*/
    fnet_memset_zero(buf->data_ptr, FNET_TCP_SIZE_OPTIONS); /* Set to FNET_TCP_OTYPES_END.*/
    segment = _fnet_netbuf_concat(segment, buf);
    buf = segment->next;

    /* Copy the options.*/
    for (i = 0u; i < len; ++i)
    {
        FNET_TCP_GETUCHAR(buf->data_ptr, i) = FNET_TCP_GETUCHAR(data, i);
    }

    /* Set the length  (this value is saved in 4-byte words format).*/
    if((len & 0x3u) != 0u)
    {
        size = (fnet_uint8_t)((len & 0xfffCu) + 4u);
    }
    else
    {
        size = (fnet_uint8_t)len;
    }

    /* Trim the buffer of the options.*/
    _fnet_netbuf_trim(&segment, (fnet_int32_t)(size - FNET_TCP_SIZE_OPTIONS));

    /* Recalculate the header length.*/
    FNET_TCP_SET_LENGTH(segment) = (fnet_uint8_t)((buf->length + FNET_TCP_SIZE_HEADER) << 2);

    return FNET_ERR_OK;
}

/************************************************************************
* DESCRIPTION: This function processes the received options.
*************************************************************************/
static void _fnet_tcp_get_opt( fnet_socket_if_t *sk, fnet_netbuf_t *segment )
{
    fnet_index_t i;

    fnet_tcp_control_t *cb = (fnet_tcp_control_t *)sk->protocol_control;

    if(!segment)
    {
        return;
    }

    /* Start position of the options.*/
    i = FNET_TCP_SIZE_HEADER;

    /* Receive the of the options.*/
    while((i < FNET_TCP_LENGTH(segment)) && (FNET_TCP_GETUCHAR(segment->data_ptr, i) != FNET_TCP_OTYPES_END))
    {
        if(FNET_TCP_GETUCHAR(segment->data_ptr, i) == FNET_TCP_OTYPES_NOP)
        {
            ++i;
        }
        else
        {
            if(((i + 1u) >= FNET_TCP_LENGTH(segment)) || (i + FNET_TCP_GETUCHAR(segment->data_ptr, i + 1u) - 1u >= FNET_TCP_LENGTH(segment)))
            {
                break;
            }

            /* Process the options.*/
            switch(FNET_TCP_GETUCHAR(segment->data_ptr, i))
            {
                case FNET_TCP_OTYPES_MSS:
                    cb->tcpcb_sndmss = fnet_ntohs(FNET_TCP_GETUSHORT(segment->data_ptr, i + 2u));
                    break;

                case FNET_TCP_OTYPES_WINDOW:
                    cb->tcpcb_sendscale = FNET_TCP_GETUCHAR(segment->data_ptr, i + 2u);

                    if(cb->tcpcb_sendscale > FNET_TCP_MAX_WINSHIFT)
                    {
                        cb->tcpcb_sendscale = FNET_TCP_MAX_WINSHIFT;
                    }

                    cb->tcpcb_flags |= FNET_TCP_CBF_RCVD_SCALE;
                    break;
                default:
                    break;
            }

            i += FNET_TCP_GETUCHAR(segment->data_ptr, i + 1u);
        }
    }
}

/************************************************************************
* DESCRIPTION: This function sets the options of the synchronized (SYN )
*              segment.
*************************************************************************/
static void _fnet_tcp_set_synopt( fnet_socket_if_t *sk, fnet_uint8_t *options, fnet_uint8_t *optionlen )
{
    fnet_tcp_control_t *cb = (fnet_tcp_control_t *)sk->protocol_control;

    *optionlen = 0u;

    /* If 0, detect MSS based on interface MTU minus "TCP,IP header size".*/
    if(cb->tcpcb_rcvmss == 0u)
    {
#if FNET_CFG_IP4
        fnet_netif_t *netif;

        if((netif = _fnet_ip4_route(((struct fnet_sockaddr_in *)(&sk->foreign_addr))->sin_addr.s_addr)) != 0)
        {
            cb->tcpcb_rcvmss = (fnet_uint16_t)(netif->netif_mtu - 40u); /* MTU - [TCP,IP header size].*/
        }
#else
        cb->tcpcb_rcvmss = 0;
#endif /* FNET_CFG_IP4 */
    }

    /* Set the MSS option.*/
    *((fnet_uint32_t *)(options + *optionlen)) = fnet_htonl((fnet_uint32_t)(cb->tcpcb_rcvmss | FNET_TCP_MSS_HEADER));
    *optionlen += FNET_TCP_MSS_SIZE;

    /* Set the window scale option.*/
    *((fnet_uint32_t *)(options + *optionlen))
        = fnet_htonl((fnet_uint32_t)((cb->tcpcb_recvscale | FNET_TCP_WINDOW_HEADER) << 8));
    *optionlen += FNET_TCP_WINDOW_SIZE;
}

/************************************************************************
* DESCRIPTION: This function performs the initialization depend on
*              options of the synchronized (SYN) segment.
*************************************************************************/
static void _fnet_tcp_get_synopt( fnet_socket_if_t *sk )
{
    /* Pointer to the control block.*/
    fnet_tcp_control_t *cb = (fnet_tcp_control_t *)sk->protocol_control;

    if((cb->tcpcb_flags & FNET_TCP_CBF_RCVD_SCALE) == 0u)
    {
        /* Reset the scale variable if the scale option is not received.*/
        cb->tcpcb_recvscale = 0u;

        /* If the size of the input buffer greater than
         * the maximal size of the window , reinitialze the buffer size.*/
        if(cb->tcpcb_rcvcountmax > FNET_TCP_MAXWIN)
        {
            cb->tcpcb_rcvcountmax = FNET_TCP_MAXWIN;
        }
    }

    /* Initialize the congestion window.*/
    cb->tcpcb_cwnd = cb->tcpcb_sndmss;
}

/************************************************************************
* DESCRIPTION: This function finds the socket with parameters that allow
*              to receive and process the segment.
*
* RETURNS: If the socket is found this function returns the pointer to the
*          socket. Otherwise, this function returns 0.
*************************************************************************/
static fnet_socket_if_t *_fnet_tcp_find_socket( struct fnet_sockaddr *src_addr, struct fnet_sockaddr *dest_addr )
{
    fnet_socket_if_t   *listensk = 0;
    fnet_socket_if_t   *sk;

    fnet_isr_lock();

    sk = fnet_tcp_prot_if.head;

    while(sk)
    {
        if(sk->local_addr.sa_port == dest_addr->sa_port)
        {
            /* Listening socket.*/
            if(sk->state == SS_LISTENING)
            {
                if((!listensk) && ((fnet_socket_addr_is_unspecified(&sk->local_addr)) || (fnet_socket_addr_are_equal(&sk->local_addr, dest_addr)) ))
                {
                    listensk = sk;
                }
            }
            else
            {
                /* Not listening socket.*/
                if((fnet_socket_addr_are_equal(&sk->local_addr, dest_addr)) && (sk->state != SS_CLOSED) &&
                   (fnet_socket_addr_are_equal(&sk->foreign_addr, src_addr)) && (sk->foreign_addr.sa_port == src_addr->sa_port))
                {
                    break;
                }
            }
        }
        sk = sk->next;
    }

    /* If the listening socket with the same local parameters (address and port) is present.*/
    if((!sk) && listensk)
    {
        /* Search the partial socket with the same foreign parameters (address and port).*/
        sk = listensk->partial_con;

        while(sk)
        {
            if((fnet_socket_addr_are_equal(&sk->foreign_addr, src_addr)) && (sk->foreign_addr.sa_port == src_addr->sa_port))
            {
                break;
            }

            sk = sk->next;
        }

        if(!sk)
        {
            /* Search the incoming socket with the same foreign parameters (address and port).*/
            sk = listensk->incoming_con;

            while(sk)
            {
                if((fnet_socket_addr_are_equal(&sk->foreign_addr, src_addr)) && (sk->foreign_addr.sa_port == src_addr->sa_port))
                {
                    break;
                }

                sk = sk->next;
            }
        }

        /* Listening socket is unique (with the same local parameters).*/
        if(!sk)
        {
            sk = listensk;
        }
    }

    fnet_isr_unlock();

    return sk;
}

/***********************************************************************
* DESCRIPTION: This function adds the socket to the partial sockets list.
*************************************************************************/
static void _fnet_tcp_add_partial_socket( fnet_socket_if_t *mainsk, fnet_socket_if_t *partialsk )
{
    mainsk->partial_con_len++;
    partialsk->head_con = mainsk;
    _fnet_socket_list_add(&mainsk->partial_con, partialsk);
}

/***********************************************************************
* DESCRIPTION: This function moves socket from the partial list
*              to the incoming list.
*************************************************************************/
static void _fnet_tcp_move_socket2incominglist( fnet_socket_if_t *sk )
{
    fnet_socket_if_t *mainsk;
    mainsk = sk->head_con;

    mainsk->partial_con_len--;
    mainsk->incoming_con_len++;

    _fnet_socket_list_del(&mainsk->partial_con, sk);
    _fnet_socket_list_add(&mainsk->incoming_con, sk);

#if FNET_CFG_SOCKET_CALLBACK_ON_RX
    /* Wake-up user application.*/
    fnet_event_raise(fnet_socket_event_rx);
#endif
}

/***********************************************************************
* DESCRIPTION: This function closes the socket.
*************************************************************************/
static void _fnet_tcp_close_socket( fnet_socket_if_t *sk )
{
    if(sk->head_con)
    {
        /* If the socket is partial or incoming.*/
        if(sk->state == SS_CONNECTING)
        {
            /* Delete from the partial list.*/
            _fnet_tcp_del_partialsocket(sk);
        }
        else
        {
            /* Delete from the incoming list.*/
            _fnet_tcp_del_incomingsocket(sk);
        }
    }
    else
    {
        fnet_tcp_control_t *cb = (fnet_tcp_control_t *)sk->protocol_control;

        /* If the socket must be deleted, free the structures
         * Otherwise, change the state and free the unused data.*/
        if((cb->tcpcb_flags & FNET_TCP_CBF_CLOSE) != 0u)
        {
            _fnet_tcp_del_socket(&fnet_tcp_prot_if.head, sk);
        }
        else
        {
#if !FNET_CFG_TCP_DISCARD_OUT_OF_ORDER
            _fnet_tcp_del_tmpbuf(cb);
#endif
            _fnet_socket_buffer_release(&sk->send_buffer);
            sk->state = SS_CLOSED;
            fnet_memset_zero(&sk->foreign_addr, sizeof(sk->foreign_addr));
        }
    }
}
/***********************************************************************
* DESCRIPTION: This function  deletes the socket from
*              the partial list.
*************************************************************************/
static void _fnet_tcp_del_partialsocket( fnet_socket_if_t *sk )
{
    sk->head_con->partial_con_len--;
    _fnet_tcp_del_socket(&sk->head_con->partial_con, sk);
}

/***********************************************************************
* DESCRIPTION: This function deletes the socket from the incoming list.
*************************************************************************/
static void _fnet_tcp_del_incomingsocket( fnet_socket_if_t *sk )
{
    sk->head_con->incoming_con_len--;
    _fnet_tcp_del_socket(&sk->head_con->incoming_con, sk);
}

/***********************************************************************
* DESCRIPTION: This function deletes the control block.
*************************************************************************/
static void _fnet_tcp_del_cb( fnet_tcp_control_t *cb )
{
    if(cb)
    {
#if !FNET_CFG_TCP_DISCARD_OUT_OF_ORDER
        _fnet_tcp_del_tmpbuf(cb);
#endif
        _fnet_free(cb);
    }
}

/***********************************************************************
* DESCRIPTION: This function deletes the temporary buffer.
*************************************************************************/
#if !FNET_CFG_TCP_DISCARD_OUT_OF_ORDER
static void _fnet_tcp_del_tmpbuf( fnet_tcp_control_t *cb )
{
    fnet_netbuf_t *buf, *delbuf;

    buf = cb->tcpcb_rcvchain;

    while(buf)
    {
        delbuf = buf;
        buf = buf->next_chain;
        _fnet_netbuf_free_chain(delbuf);
    }

    cb->tcpcb_count = 0u;
    cb->tcpcb_rcvchain = 0;
}
#endif

/***********************************************************************
* DESCRIPTION: This function deletes the socket.
*************************************************************************/
static void _fnet_tcp_del_socket( fnet_socket_if_t **head, fnet_socket_if_t *sk )
{
    _fnet_tcp_del_cb((fnet_tcp_control_t *)sk->protocol_control);
    _fnet_socket_release(head, sk);
}

/***********************************************************************
* DESCRIPTION: This function returns TRUE if position (pos) is located
*           in the interval between startpos and endpos. Otherwise
*           this function returns FALSE.
*************************************************************************/
static fnet_bool_t _fnet_tcp_hit( fnet_uint32_t startpos, fnet_uint32_t endpos, fnet_uint32_t pos )
{
    fnet_bool_t result = FNET_TRUE;

    if(endpos >= startpos)
    {
        if((pos < startpos) || (pos > endpos))
        {
            result = FNET_FALSE;
        }
    }
    else
    {
        if((pos < startpos) && (pos > endpos))
        {
            result = FNET_FALSE;
        }
    }

    return result;
}

/***********************************************************************
* DESCRIPTION: This function returns the length of the interval
*              (from pos1 to pos2 ).
*************************************************************************/
static fnet_uint32_t _fnet_tcp_get_size( fnet_uint32_t pos1, fnet_uint32_t pos2 )
{
    fnet_uint32_t size;

    if(pos1 <= pos2)
    {
        size = pos2 - pos1;
    }
    else
    {
        size = FNET_TCP_MAX_SEQ - pos1 + pos2 + 1u;
    }

    return size;
}

/************************************************************************
* DESCRIPTION: Prints TCP header. For debugging purposes.
*************************************************************************/
#if FNET_CFG_DEBUG_TRACE_TCP && FNET_CFG_DEBUG_TRACE
void _fnet_tcp_trace(const fnet_char_t *str, fnet_tcp_header_t *tcp_hdr)
{
    fnet_printf(FNET_SERIAL_ESC_FG_GREEN"%s", str); /* Print app-specific header.*/
    fnet_println("[TCP header]"FNET_SERIAL_ESC_ATTR_RESET);
    fnet_println("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+");
    fnet_println("|(SrcPort)                 "FNET_SERIAL_ESC_FG_BLUE"%3u"FNET_SERIAL_ESC_ATTR_RESET" |(DestPort)                 "FNET_SERIAL_ESC_FG_BLUE"%3u"FNET_SERIAL_ESC_ATTR_RESET" |",
                 fnet_ntohs(tcp_hdr->source_port),
                 fnet_ntohs(tcp_hdr->destination_port));
    fnet_println("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+");
    fnet_println("|(Sequence number)                                 0x%010u |",
                 fnet_ntohl(tcp_hdr->sequence_number));
    fnet_println("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+");
    fnet_println("|(ACK number)                                      0x%010u |",
                 fnet_ntohl(tcp_hdr->ack_number));
    fnet_println("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+");
    fnet_println("|(HL)%2u |           |(F)  %u%u%u%u%u%u|(Window)                 %5u |",
                 FNET_TCP_HEADER_GET_HDRLENGTH(tcp_hdr),
                 FNET_TCP_HEADER_GET_FLAGS(tcp_hdr) >> 5 & 1,
                 FNET_TCP_HEADER_GET_FLAGS(tcp_hdr) >> 4 & 1,
                 FNET_TCP_HEADER_GET_FLAGS(tcp_hdr) >> 3 & 1,
                 FNET_TCP_HEADER_GET_FLAGS(tcp_hdr) >> 2 & 1,
                 FNET_TCP_HEADER_GET_FLAGS(tcp_hdr) >> 1 & 1,
                 FNET_TCP_HEADER_GET_FLAGS(tcp_hdr) & 1,
                 fnet_ntohs(tcp_hdr->window)  );
    fnet_println("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+");
    fnet_println("|(Checksum)             0x%04u |(Urgent ptr)             %5u |",
                 fnet_ntohs(tcp_hdr->checksum),
                 fnet_ntohs(tcp_hdr->urgent_ptr));
    fnet_println("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+");
}
#endif /* FNET_CFG_DEBUG_TRACE_TCP */



#if 0 /* For Debug needs.*/
/*************************************************************************/
fnet_return_t FNET_DEBUG_check_send_buffer( fnet_socket_if_t *sk)
{
    fnet_netbuf_t   *head_nb;
    fnet_netbuf_t   *nb = (sk->send_buffer.net_buf_chain);
    fnet_index_t    i = 0u;

    if(nb == 0)
    {
        return FNET_OK;
    }

    head_nb = nb;

    while(nb->next != 0)
    {
        i++;
        nb = nb->next;
        if(i > 100)
        {
            fnet_println("!!!SEND BUF CHAIN CRASH!!!");
            return FNET_ERR;
        }
    }

    return FNET_OK;
}
#endif

#endif
