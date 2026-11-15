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
*  Private. TCP protocol definitions.
*
***************************************************************************/

#ifndef _FNET_TCP_H_

#define _FNET_TCP_H_

#include "fnet_netbuf_prv.h"
#include "fnet_netif.h"
#include "fnet_netif_prv.h"

/************************************************************************
*    Headers of options
*************************************************************************/

#define FNET_TCP_MSS_HEADER         (0x02040000u) /* MSS option*/
#define FNET_TCP_WINDOW_HEADER      (0x30300u)    /* Window scale option*/

/************************************************************************
*    Protocol structure
*************************************************************************/
extern struct fnet_prot_if fnet_tcp_prot_if;

/************************************************************************
*    Size of urgent data
*************************************************************************/
#define FNET_TCP_URGENT_DATA_SIZE   (1u)

/************************************************************************
*    Not used value
*************************************************************************/
#define FNET_TCP_NOT_USED           (-1)

/************************************************************************
*    Control values for timers
*************************************************************************/
#define FNET_TCP_TIMER_OFF          ((fnet_time_t)(-1))    /* Switch off value.*/
#define FNET_TCP_TIMER_ON_INCREMENT (0u)     /* Switch on value for increment timers.*/

/************************************************************************
*    Step of Initial sequence number (ISN)
*************************************************************************/
#define FNET_TCP_INITIAL_SEQ_NUMBER_STEP    (64000u)

/************************************************************************
*    Defaults values
*************************************************************************/
#define FNET_TCP_DEFAULT_MSS    (536u)                      /* Default value of MSS.*/
#define FNET_TCP_TX_BUF_MAX     (FNET_CFG_SOCKET_TCP_TX_BUF_SIZE) /* Default maximum size for TCP send socket buffer.*/
#define FNET_TCP_RX_BUF_MAX     (FNET_CFG_SOCKET_TCP_RX_BUF_SIZE) /* Default maximum size for TCP receive socket buffer.*/

/************************************************************************
*    Periods of timers
*************************************************************************/
#define FNET_TCP_SLOW_TIMER_PERIOD_MS       (500u)       /* Slow timer period (ms).*/
#define FNET_TCP_FAST_TIMER_PERIOD_MS       (200u)       /* Fast timer period (ms).*/

/************************************************************************
*    Keepalive timer parameters
*************************************************************************/
#define FNET_TCP_KEEPIDLE_DEFAULT   (2 * 60 * 60 * 1000 / FNET_TCP_SLOW_TIMER_PERIOD_MS) /* Standard value for keepalive timer (2 hours).*/
#define FNET_TCP_KEEPINTVL_DEFAULT  (75 * 1000 / FNET_TCP_SLOW_TIMER_PERIOD_MS) /* Standard value for retransmission of the keepalive segment (75 sec).*/
#define FNET_TCP_KEEPCNT_DEFAULT    (8u)     /* Number of keepalive segments in state of retransmission.*/

/************************************************************************
*    Initial value for the retransmission and persist timers (6 sec)
*************************************************************************/
#define FNET_TCP_TIMERS_INIT        (6000 / FNET_TCP_SLOW_TIMER_PERIOD_MS)

/************************************************************************
*    Limit of timers (60 sec)
*************************************************************************/
#define FNET_TCP_TIMERS_LIMIT       (60 * 1000 / FNET_TCP_SLOW_TIMER_PERIOD_MS)

/************************************************************************
*    Shifts of the retransmission variables
*************************************************************************/
#define FNET_TCP_RTT_SHIFT          (3u) /* Smoothed round trip time shift.*/
#define FNET_TCP_RTTVAR_SHIFT       (2u) /* Round trip time variance shift.*/

/************************************************************************
*    Maximal size of synchronized options
*************************************************************************/
#define FNET_TCP_MAX_OPT_SIZE       (8u)

/************************************************************************
*    Maximal window size
*************************************************************************/
#define FNET_TCP_MAXWIN             (12u*1024u)/* RFC:(0xffff)*/

/************************************************************************
*    Maximal value of the sequence number
*************************************************************************/
#define FNET_TCP_MAX_SEQ            (0xffffffffu)

/************************************************************************
*    Maximal urgent pointer
*************************************************************************/
#define FNET_TCP_MAX_URG_POINTER    (0xffffu)

/************************************************************************
*    Maximal size of the receive buffer
*************************************************************************/
#define FNET_TCP_MAX_BUFFER         (0xfff0000u)

/************************************************************************
*    Maximal window scale value
*************************************************************************/
#define FNET_TCP_MAX_WINSHIFT       (12u)

/************************************************************************
*    Abort interval for the data retransmission and the connection termination
*************************************************************************/
#define FNET_TCP_ABORT_INTERVAL     (120 * 1000 / (FNET_TCP_SLOW_TIMER_PERIOD_MS * 5) ) /* 2 minutes/5 */

/************************************************************************
*    Abort interval for the connection establishment
*************************************************************************/
#define FNET_TCP_ABORT_INTERVAL_CON (75 * 1000 / (FNET_TCP_SLOW_TIMER_PERIOD_MS * 5) ) /* 75 sec/5 */

/************************************************************************
*    Number of repeated acknowledgments for the fast retransmission
*************************************************************************/
#define FNET_TCP_NUMBER_FOR_FAST_RET    (3u)

/************************************************************************
*    Timewait delay
*************************************************************************/
#define FNET_TCP_TIME_WAIT              (120 * 1000 / FNET_TCP_SLOW_TIMER_PERIOD_MS) /* 2 minutes */

/************************************************************************
*    Receiving of a byte, word and double word
*************************************************************************/

#define FNET_TCP_GETUCHAR(addr, offset)    (*(fnet_uint8_t*)((fnet_uint32_t)(addr)+(offset)))
#define FNET_TCP_GETUSHORT(addr, offset)   (*(fnet_uint16_t*)((fnet_uint32_t)(addr)+(offset)))
#define FNET_TCP_GETULONG(addr, offset)    (*(fnet_uint32_t*)((fnet_uint32_t)(addr)+(offset)))

/************************************************************************
*    Comparison of sequence numbers
*************************************************************************/

#define FNET_TCP_COMP_GE(a,b)   _fnet_tcp_hit((fnet_uint32_t)(b), (fnet_uint32_t)((b) + 0x20000000u), (fnet_uint32_t)(a))   /* Greater or equal.*/
#define FNET_TCP_COMP_G(a,b)    _fnet_tcp_hit((fnet_uint32_t)((b)+1u), (fnet_uint32_t)((b) + 0x20000000u), (fnet_uint32_t)(a)) /* Greater.*/

/************************************************************************
*    Segment header fields
*************************************************************************/
/* TBD eliminate these definitions => use structure header. */
#define FNET_TCP_DPORT(segment)        FNET_TCP_GETUSHORT((segment)->data_ptr, 2u)
#define FNET_TCP_SPORT(segment)        FNET_TCP_GETUSHORT((segment)->data_ptr, 0u)
#define FNET_TCP_WND(segment)          FNET_TCP_GETUSHORT((segment)->data_ptr, 14u)
#define FNET_TCP_FLAGS(segment)        FNET_TCP_GETUCHAR((segment)->data_ptr, 13u)&0x3fu
#define FNET_TCP_SEQ(segment)          FNET_TCP_GETULONG((segment)->data_ptr, 4u)
#define FNET_TCP_ACK(segment)          FNET_TCP_GETULONG((segment)->data_ptr, 8u)
#define FNET_TCP_LENGTH(segment)       (((FNET_TCP_GETUCHAR((segment)->data_ptr, 12u)&0xf0u)>>4u)*4u)
#define FNET_TCP_URG(segment)          FNET_TCP_GETUSHORT((segment)->data_ptr, 18u)
#define FNET_TCP_CHECKSUM(segment)     FNET_TCP_GETUSHORT((segment)->data_ptr, 16u)
#define FNET_TCP_SET_LENGTH(segment)   FNET_TCP_GETUCHAR((segment)->data_ptr, 12u)
#define FNET_TCP_SET_FLAGS(segment)    FNET_TCP_GETUCHAR((segment)->data_ptr, 13u)

/************************************************************************
*    Types of segments
*************************************************************************/
#define FNET_TCP_SGT_FIN            (0x01u)
#define FNET_TCP_SGT_SYN            (0x02u)
#define FNET_TCP_SGT_RST            (0x04u)
#define FNET_TCP_SGT_PSH            (0x08u)
#define FNET_TCP_SGT_ACK            (0x10u)
#define FNET_TCP_SGT_URG            (0x20u)

/************************************************************************
*    TCP options
*************************************************************************/
#define FNET_TCP_OTYPES_END         (0u) /* End of Options.*/
#define FNET_TCP_OTYPES_NOP         (1u) /* No Option.*/
#define FNET_TCP_OTYPES_MSS         (2u) /* Maximal segment size.*/
#define FNET_TCP_OTYPES_WINDOW      (3u) /* Scale window.*/

#define FNET_TCP_MSS_SIZE           (4u) /* MSS option size.*/
#define FNET_TCP_WINDOW_SIZE        (3u) /* Window scale option size.*/

/**************************************************************************/ /*!
 * @internal
 * @brief    TCP options structure.
 ******************************************************************************/
typedef struct
{

    fnet_uint32_t   mss;            /* TCP_MSS option. Maximal segment size*/
    fnet_uint32_t   keep_idle;      /* TCP_KEEPIDLE option. */
    fnet_uint32_t   keep_intvl;     /* TCP_KEEPINTVL option. */
    fnet_uint32_t   keep_cnt;       /* TCP_KEEPCNT option. */
    fnet_bool_t     tcp_nodelay;    /*  If this option is set to FNET_TRUE, the Nagle algorithm
                                    *   is disabled (and vice versa). @n
                                    *   The Nagle algorithm is effective in reducing the number
                                    *   of small packets sent by the host by essentially buffering
                                    *   send data, if there is unacknowledged data already "in flight",
                                    *   or until a full-size packet can be sent.
                                    *   But for some applications this algorithm can impede
                                    *   performance, especially for a bulky data transfer*/
#if FNET_CFG_TCP_URGENT
    fnet_bool_t tcp_bsd;    /*  If this option is set to FNET_TRUE, the BSD interpretation of
                                    *   the urgent pointer is used. In this case the
                                    *   urgent pointer of the TCP segment points to
                                    *   the next byte following after the urgent byte.
                                    *   Most of the TCP implementations use this
                                    *   interpretation by default.
                                    *   If this option is set to FNET_FALSE, the interpretation of
                                    *   the TCP specification is used. In this case the
                                    *   urgent pointer of the TCP segment points
                                    *   to the urgent byte.
                                    *   This option is avalable only if FNET_CFG_TCP_URGENT is set to 1.*/
#endif

} fnet_tcp_sockopt_t;

/************************************************************************
*    TCP header structure. //TBD use it instead of definitions.
*************************************************************************/
FNET_COMP_PACKED_BEGIN
typedef struct
{
    fnet_uint16_t   source_port     FNET_COMP_PACKED;       /* Source port number.*/
    fnet_uint16_t   destination_port    FNET_COMP_PACKED;   /* Destination port number.*/
    fnet_uint32_t   sequence_number     FNET_COMP_PACKED;   /* Sequence Number.*/
    fnet_uint32_t   ack_number  FNET_COMP_PACKED;           /* Sequence Number.*/
    fnet_uint16_t   hdrlength__flags FNET_COMP_PACKED;      /* (4 bits) Number of 32 bit words in the TCP Header. (6 bits) Reserved. (6bits) Flags.*/
    fnet_uint16_t   window  FNET_COMP_PACKED;               /* Window.*/
    fnet_uint16_t   checksum    FNET_COMP_PACKED;           /* Checksum.*/
    fnet_uint16_t   urgent_ptr  FNET_COMP_PACKED;           /* Urgent pointer.*/
} fnet_tcp_header_t;
FNET_COMP_PACKED_END

#define FNET_TCP_HEADER_GET_HDRLENGTH(x)    (fnet_ntohs((x)->hdrlength__flags)>>12)
#define FNET_TCP_HEADER_GET_FLAGS(x)        (fnet_ntohs((x)->hdrlength__flags)&0x3Fu)

/************************************************************************
*    TCP header size without options
*************************************************************************/
#define FNET_TCP_SIZE_HEADER        (20u)

/************************************************************************
*    Maximal optios size
*************************************************************************/
#define FNET_TCP_SIZE_OPTIONS       (40u)

/************************************************************************
*    Default TTL
*************************************************************************/
#define FNET_TCP_TTL_DEFAULT        (64u)

/************************************************************************
*    Acknowledgement parameters
*************************************************************************/
#define FNET_TCP_AP_NO_SENDING          (1u) /* Ackonwledgment is already sent.*/
#define FNET_TCP_AP_SEND_IMMEDIATELLY   (2u) /* Ackonwledgment must be sent immediatelly.*/
#define FNET_TCP_AP_SEND_WITH_DELAY     (4u) /* Ackonwledgment can be sent with delay.*/
#define FNET_TCP_AP_FIN_ACK             (8u) /* Acknowledgment of the final segment.*/

/************************************************************************
*    Flags of control block
*************************************************************************/
#define FNET_TCP_CBF_CLOSE          (0x01u)  /* Socket must be closed and deleted.*/
#define FNET_TCP_CBF_RCVURGENT      (0x02u)  /* Urgent byte is received.*/
#define FNET_TCP_CBF_FIN_SENT       (0x04u)  /* Final segment is sent.*/
#define FNET_TCP_CBF_FIN_RCVD       (0x08u)  /* Final segment is received.*/
#define FNET_TCP_CBF_FORCE_SEND     (0x10u)  /* Data must be sent in any case.*/
#define FNET_TCP_CBF_RCVD_SCALE     (0x20u)  /* Another side uses the scale option.*/
#define FNET_TCP_CBF_SEND_TIMEOUT   (0x40u)  /* Silly window avoidance flag.*/
#define FNET_TCP_CBF_INSND          (0x80u)  /* The _fnet_tcp_snd function is executed now.*/

/************************************************************************
*    Standart states for TCP ( described in RFC793)
*************************************************************************/
typedef enum
{
    FNET_TCP_CS_NO_STATE = 0,
    FNET_TCP_CS_SYN_SENT = 1,
    FNET_TCP_CS_SYN_RCVD = 2,
    FNET_TCP_CS_LISTENING = 3,
    FNET_TCP_CS_ESTABLISHED = 4,
    FNET_TCP_CS_FIN_WAIT_1 = 5,
    FNET_TCP_CS_FIN_WAIT_2 = 6,
    FNET_TCP_CS_CLOSE_WAIT = 7,
    FNET_TCP_CS_CLOSING    = 8,
    FNET_TCP_CS_LAST_ACK   = 9,
    FNET_TCP_CS_TIME_WAIT  = 10
} fnet_tcp_connection_state_t;

/************************************************************************
*     Timing  states
*************************************************************************/
typedef enum
{
    TCP_TS_ACK_RECEIVED = 0,    /* Acknowledgment of the timing segments is received.*/
    TCP_TS_SEGMENT_SENT = 1,    /* Timing segments are sent.*/
    TCP_TS_SEGMENT_LOST = 2    /* Timing segments are lost.*/
} fnet_tcp_timing_state_t;

/************************************************************************
*    TCP timers structure
*************************************************************************/
typedef struct
{
    fnet_time_t abort;              /* Main timer (used for timing of the abort interval)*/
    fnet_time_t keepalive;          /* Keepalive timer.
                                    * It detects when the other end on an otherwise idle connection crashes or reboots.*/
    fnet_time_t connection;         /* Connection timer.*/
    fnet_time_t retransmission;     /* Retransmission timer.
                                    * It is used when expecting an acknowledgment from the other end. */
    fnet_time_t persist;            /* Persist timer.
                                    * It keeps window size information flowing even if the other end closes its receive window.*/
    fnet_time_t delayed_ack;        /* Delayed acknowledgment timer.*/
    fnet_time_t round_trip;         /* Round trip timer.*/
} fnet_tcp_timers_t;

/************************************************************************
*    Control block structure
*************************************************************************/
typedef struct
{
    /* Receive variables.*/
#if !FNET_CFG_TCP_DISCARD_OUT_OF_ORDER
    fnet_netbuf_t   *tcpcb_rcvchain;        /* Temporary buffer.*/
    fnet_size_t     tcpcb_count;            /* Size of data in the temporary buffer.*/
#endif
    fnet_size_t     tcpcb_rcvcountmax;      /* Size of the input and temporary buffers.*/

    fnet_uint32_t tcpcb_rcvack;             /* Highest acknowledged number of sent segments.*/
    fnet_uint32_t tcpcb_maxrcvack;          /* Maximal acknowledgment.*/
    fnet_uint32_t tcpcb_timingack;          /* Acknowledgment number for timing.*/
    fnet_uint32_t tcpcb_rcvwnd;             /* Window.*/
    fnet_uint16_t tcpcb_rcvmss;             /* Maximal RX segment size (MSS).*/
    fnet_uint8_t tcpcb_recvscale;           /* Scale of the window.*/
#if FNET_CFG_TCP_URGENT
    fnet_uint32_t tcpcb_rcvurgseq;          /* Sequence number of the urgent byte.*/
    fnet_int32_t tcpcb_rcvurgmark;          /* Number of bytes before the urgent character (-1 if urgent character is not present)*/
    fnet_uint8_t tcpcb_iobc;                /* Input character of the urgent data.*/
#endif /* FNET_CFG_TCP_URGENT */

    /* Send variables.*/
    fnet_uint32_t tcpcb_sndseq;         /* Current sequence number.*/
    fnet_uint32_t tcpcb_sndack;         /* Acknowledgment number.*/
    fnet_uint32_t tcpcb_sndwnd;         /* Window.*/
    fnet_uint32_t tcpcb_maxwnd;         /* Maximal window of another side.*/
    fnet_uint32_t tcpcb_cwnd;           /* Congestion window.*/
    fnet_uint32_t tcpcb_pcount;         /* Counter of the tcpcb_cwnd parts.*/
    fnet_uint32_t tcpcb_ssthresh;       /* Slow start threshold.*/
    fnet_uint16_t tcpcb_sndmss;         /* Maximal segment size (MSS).*/
    fnet_uint8_t tcpcb_sendscale;       /* Scale of the window.*/
#if FNET_CFG_TCP_URGENT
    fnet_uint32_t tcpcb_sndurgseq;      /* Sequence number of the urgent byte.*/
#endif /* FNET_CFG_TCP_URGENT */

    /* Input buffer variables.*/
    fnet_size_t   tcpcb_newfreercvsize; /* Free size of the input buffer.*/

    /* Retransmission variables.*/
    fnet_index_t tcpcb_fastretrcounter;         /* Repeated acknowledgment counter (for fast retransmission).*/
    fnet_time_t tcpcb_rto;                      /* Retransmission timeout.*/
    fnet_time_t tcpcb_crto;                     /* Current retransmission timeout.*/
    fnet_time_t tcpcb_cprto;                    /* Current retransmission timeout for persist timer.*/
    fnet_uint32_t tcpcb_retrseq;                /* Sequenc number of the retransmitting data.*/
    fnet_int32_t tcpcb_srtt;                    /* Smoothed round trip time.*/
    fnet_int32_t tcpcb_rttvar;                  /* Round trip time variance.*/
    fnet_tcp_timing_state_t tcpcb_timing_state; /* Timing state, defined by fnet_tcp_timing_state_t.*/

    /* Timers.*/
    fnet_tcp_timers_t tcpcb_timers;             /* Structure of the timers.*/

    fnet_tcp_connection_state_t tcpcb_connection_state;         /* Connection state, defined by fnet_tcp_connection_state_t.*/
    fnet_tcp_connection_state_t tcpcb_prev_connection_state;    /* Previous connection state (used only for simultaneous open),
                                                                * defined by fnet_tcp_connection_state_t.*/
    /* Flags.*/
    fnet_flag_t tcpcb_flags;
} fnet_tcp_control_t;

/*************************************************************************/

#endif
