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

#define MEM_ALIGNMENT                   4

#define LWIP_ARP                        0

#define ARP_QUEUEING                    0

#define ETH_PAD_SIZE                    2

#define IP_FORWARD                      0

#define IP_REASS_MAX_PBUFS              0xFFFFFFFF

#define IP_DEFAULT_TTL                  128

#define IP_SOF_BROADCAST                1

#define IP_SOF_BROADCAST_RECV           1

#define LWIP_ICMP                       0

#define LWIP_RAW                        0

#define LWIP_DHCP                       0

#define LWIP_AUTOIP                     0

#define LWIP_SNMP                       0

#define LWIP_IGMP                       0

#define LWIP_DNS                        0

#define LWIP_UDP                        0

#define LWIP_UDPLITE                    0

#define LWIP_TCP                        1

#define TCP_QUEUE_OOSEQ                 1

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

#define LWIP_CALLBACK_API               1

#define LWIP_NETIF_API                  1

#define LWIP_SOCKET                     0

#define LWIP_NETCONN                    0

#define LWIP_NETIF_HWADDRHINT           0

#define LWIP_STATS                      0

#define ICMP_STATS                      0

#define PPP_SUPPORT                     0

#define PPPOE_SUPPORT                   0

#define PPPOS_SUPPORT                   0

/*
   ---------------------------------------
   ---------- Debugging options ----------
   ---------------------------------------
*/
/**
 * LWIP_DBG_MIN_LEVEL: After masking, the value of the debug is
 * compared against this value. If it is smaller, then debugging
 * messages are written.
 */
#define LWIP_DBG_MIN_LEVEL              LWIP_DBG_LEVEL_ALL

/**
 * LWIP_DBG_TYPES_ON: A mask that can be used to globally enable/disable
 * debug messages of certain types.
 */
#define LWIP_DBG_TYPES_ON               LWIP_DBG_ON

/**
 * NETIF_DEBUG: Enable debugging in netif.c.
 */
#define NETIF_DEBUG                     LWIP_DBG_OFF

/**
 * PBUF_DEBUG: Enable debugging in pbuf.c.
 */
#define PBUF_DEBUG                      LWIP_DBG_OFF

/**
 * INET_DEBUG: Enable debugging in inet.c.
 */
#define INET_DEBUG                      LWIP_DBG_OFF

/**
 * IP_DEBUG: Enable debugging for IP.
 */
#define IP_DEBUG                        LWIP_DBG_OFF

/**
 * IP_REASS_DEBUG: Enable debugging in ip_frag.c for both frag & reass.
 */
#define IP_REASS_DEBUG                  LWIP_DBG_OFF

/**
 * MEM_DEBUG: Enable debugging in mem.c.
 */
#define MEM_DEBUG                       LWIP_DBG_OFF

/**
 * MEMP_DEBUG: Enable debugging in memp.c.
 */
#define MEMP_DEBUG                      LWIP_DBG_OFF

/**
 * SYS_DEBUG: Enable debugging in sys.c.
 */
#define SYS_DEBUG                       LWIP_DBG_OFF

/**
 * TCP_DEBUG: Enable debugging for TCP.
 */
#define TCP_DEBUG                       LWIP_DBG_ON

/**
 * TCP_INPUT_DEBUG: Enable debugging in tcp_in.c for incoming debug.
 */
#define TCP_INPUT_DEBUG                 LWIP_DBG_OFF

/**
 * TCP_FR_DEBUG: Enable debugging in tcp_in.c for fast retransmit.
 */
#define TCP_FR_DEBUG                    LWIP_DBG_OFF

/**
 * TCP_RTO_DEBUG: Enable debugging in TCP for retransmit
 * timeout.
 */
#define TCP_RTO_DEBUG                   LWIP_DBG_OFF

/**
 * TCP_CWND_DEBUG: Enable debugging for TCP congestion window.
 */
#define TCP_CWND_DEBUG                  LWIP_DBG_OFF

/**
 * TCP_WND_DEBUG: Enable debugging in tcp_in.c for window updating.
 */
#define TCP_WND_DEBUG                   LWIP_DBG_OFF

/**
 * TCP_OUTPUT_DEBUG: Enable debugging in tcp_out.c output functions.
 */
#define TCP_OUTPUT_DEBUG                LWIP_DBG_OFF

/**
 * TCP_RST_DEBUG: Enable debugging for TCP with the RST message.
 */
#define TCP_RST_DEBUG                   LWIP_DBG_OFF

/**
 * TCP_QLEN_DEBUG: Enable debugging for TCP queue lengths.
 */
#define TCP_QLEN_DEBUG                  LWIP_DBG_OFF

/**
 * TCPIP_DEBUG: Enable debugging in tcpip.c.
 */
#define TCPIP_DEBUG                     LWIP_DBG_OFF