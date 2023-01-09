#ifndef DEBUG
#define DEBUG
#endif

////// Non-supported options
//// sys.h
// doc: https://www.nongnu.org/lwip/2_1_x/sys_8h.html
/* Define LWIP_COMPAT_MUTEX if the port has no mutexes and binary semaphores
 should be used instead */
//#define LWIP_COMPAT_MUTEX               1

//// opt.h
// No doc available
#define LWIP_CALLBACK_API               1

//// ppp_opts.h
// No doc available
#define PPP_SUPPORT                     0
#define PPPOE_SUPPORT                   0
#define PPPOS_SUPPORT                   0

////// lwIP Infrastructure options
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__infrastructure.html

//// NO_SYS
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__nosys.html
//#define NO_SYS                          1

//// Timers
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__timers.html
// No timers used

//// memcpy
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__memcpy.html
// No memcpy used

//// Core locking and MPU
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__lock.html
// No core locking and MPU used

//// Heap and memory pools
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__mem.html
#define MEM_LIBC_MALLOC                 1 /* DEFAULT: 0 */
#define MEMP_MEM_MALLOC                 1 /* DEFAULT: 0 */
#define MEM_ALIGNMENT                   4 /* DEFAULT: 1 */

//// Internal memory pools
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__memp.html
// No internal memory pools option used

//// SNMP MIB2 callbacks
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__mib2.html
// No SNMP MIB2 callbacks option used

//// Multicast
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__multicast.html
// No multicast option used

//// Threading
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__thread.html
// No threading option used

//// Checksum
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__checksum.html
// No checksum option used

//// Hooks
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__hooks.html
// No hook option used


////// lwIP Callback-style APIs Options

//// RAW
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__raw.html
#define LWIP_RAW                        0

//// DNS
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__dns.html
#define LWIP_DNS                        0

//// UDP
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__udp.html
#define LWIP_UDP                        0
#define LWIP_UDPLITE                    0

//// TCP
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__tcp.html
#define LWIP_TCP                        1

#define TCP_WND                         0xFFFF /* DEFUALT: (4 * TCP_MSS) */
#define TCP_MAXRTX                      8 /* DEFAULT: 12 */
#define TCP_SYNMAXRTX                   4 /* DEFAULT: 6 */
#define TCP_QUEUE_OOSEQ                 1 /* DEFAULT: LWIP_TCP */
#define TCP_MSS                         1460
#define TCP_SND_BUF                     TCP_WND /* DEFAULT: (2 * TCP_MSS) */
#define TCP_LISTEN_BACKLOG              1
#define LWIP_TCP_TIMESTAMPS             1

/* FIXME: These MSS and TCP Window definitions assume an MTU
 * of 1500. We need to add some code to lwIP which would allow us
 * to change these values based upon the interface we are 
 * using. Currently ReactOS only supports Ethernet so we're
 * fine for now but it does need to be fixed later when we
 * add support for other transport mediums */


////// lwIP Thread-safe APIs Options
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__threadsafe__apis.html

//// Netconn
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__netconn.html
#define LWIP_NETCONN                    0

//// Sockets
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__socket.html
#define LWIP_SOCKET                     0

#define SO_REUSE                        1
#define SO_REUSE_RXTOALL                1


////// lwIP IPv4 Options
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__ipv4.html
//#define LWIP_IPV4                       1

#define IP_FORWARD                      0
#define IP_REASS_MAX_PBUFS              0xFFFFFFFF // DEFAULT: 10
#define IP_DEFAULT_TTL                  128        // DEFAULT: 255
#define IP_SOF_BROADCAST                1          // DEFAULT: 0
#define IP_SOF_BROADCAST_RECV           1          // DEFAULT: 0

//// ARP
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__arp.html
#define LWIP_ARP                        0

#define ARP_QUEUEING                    0
#define ETH_PAD_SIZE                    2 /* DEFAULT: 0 */

//// ICMP
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__icmp.html
#define LWIP_ICMP                       0

//// DHCP
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__dhcp.html
#define LWIP_DHCP                       0

//// AUTOIP
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__autoip.html
#define LWIP_AUTOIP                     0

//// IGMP
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__igmp.html
#define LWIP_IGMP                       0

//// PBUF
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__pbuf.html
// No PBUF option used

//// NETIF
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__netif.html
#define LWIP_NETIF_API                  1 // DEFAULT: 0
#define LWIP_NETIF_HWADDRHINT           0

//// IPv6
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__ipv6.html
//#define LWIP_IPV6                       0


////// lwIP Application Options
// doc: https://www.nongnu.org/lwip/2_1_x/group__snmp__opts.html
#define LWIP_SNMP                       0


#ifdef DEBUG
////// lwIP Debugging Options
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__debug.html

//// Assertion handling
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__assertions.html
// No assertion handling option used

//// Statistics
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__stats.html
#define LWIP_STATS                      0 /* DEFAULT: 1 */
#define ICMP_STATS                      0 /* DEFAULT: 1 */

//// Debug messages
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__debugmsg.html

#  define LWIP_DEBUG

#  define LWIP_DBG_MIN_LEVEL              LWIP_DBG_LEVEL_SEVERE
#  define LWIP_DBG_TYPES_ON               LWIP_DBG_ON

#  define ETHARP_DEBUG                    LWIP_DBG_ON
#  define NETIF_DEBUG                     LWIP_DBG_ON
#  define PBUF_DEBUG                      LWIP_DBG_ON
#  define API_LIB_DEBUG                   LWIP_DBG_ON
#  define API_MSG_DEBUG                   LWIP_DBG_ON
#  define SOCKETS_DEBUG                   LWIP_DBG_ON
#  define ICMP_DEBUG                      LWIP_DBG_ON
#  define IGMP_DEBUG                      LWIP_DBG_ON
#  define INET_DEBUG                      LWIP_DBG_ON
#  define IP_DEBUG                        LWIP_DBG_ON
#  define IP_REASS_DEBUG                  LWIP_DBG_ON
#  define RAW_DEBUG                       LWIP_DBG_ON
#  define MEM_DEBUG                       LWIP_DBG_ON
#  define MEMP_DEBUG                      LWIP_DBG_ON
#  define SYS_DEBUG                       LWIP_DBG_ON
#  define TIMERS_DEBUG                    LWIP_DBG_ON
#  define TCP_DEBUG                       LWIP_DBG_ON
#  define TCP_INPUT_DEBUG                 LWIP_DBG_ON
#  define TCP_FR_DEBUG                    LWIP_DBG_ON
#  define TCP_RTO_DEBUG                   LWIP_DBG_ON
#  define TCP_CWND_DEBUG                  LWIP_DBG_ON
#  define TCP_WND_DEBUG                   LWIP_DBG_ON
#  define TCP_OUTPUT_DEBUG                LWIP_DBG_ON
#  define TCP_RST_DEBUG                   LWIP_DBG_ON
#  define TCP_QLEN_DEBUG                  LWIP_DBG_ON
#  define UDP_DEBUG                       LWIP_DBG_ON
#  define TCPIP_DEBUG                     LWIP_DBG_ON
#  define SLIP_DEBUG                      LWIP_DBG_ON
#  define DHCP_DEBUG                      LWIP_DBG_ON
#  define AUTOIP_DEBUG                    LWIP_DBG_ON
#  define DNS_DEBUG                       LWIP_DBG_ON
#  define IP6_DEBUG                       LWIP_DBG_ON
#  define DHCP6_DEBUG                     LWIP_DBG_ON

//// Performance
// doc: https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__perf.html
// No performance option used 

#endif
