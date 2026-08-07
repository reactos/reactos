/*
 * Copyright (C) 2009 Robert Shearman
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

#ifndef __WS2IPDEF__
#define __WS2IPDEF__

#include <in6addr.h>

#ifdef USE_WS_PREFIX
#define WS(x)    WS_##x
#else
#define WS(x)    x
#endif

typedef struct WS(sockaddr_in6_old)
{
   SHORT    sin6_family;
   USHORT   sin6_port;
   ULONG    sin6_flowinfo;
   IN6_ADDR sin6_addr;
} SOCKADDR_IN6_OLD,*PSOCKADDR_IN6_OLD, *LPSOCKADDR_IN6_OLD;

typedef union sockaddr_gen
{
   struct WS(sockaddr) Address;
   struct WS(sockaddr_in)  AddressIn;
   struct WS(sockaddr_in6_old) AddressIn6;
} WS(sockaddr_gen);

/* Structure to keep interface specific information */
typedef struct _INTERFACE_INFO
{
    ULONG             iiFlags;             /* Interface flags */
    WS(sockaddr_gen)  iiAddress;           /* Interface address */
    WS(sockaddr_gen)  iiBroadcastAddress;  /* Broadcast address */
    WS(sockaddr_gen)  iiNetmask;           /* Network mask */
} INTERFACE_INFO, * LPINTERFACE_INFO;

/* Possible flags for the  iiFlags - bitmask  */
#ifndef USE_WS_PREFIX
#define IFF_UP                0x00000001 /* Interface is up */
#define IFF_BROADCAST         0x00000002 /* Broadcast is  supported */
#define IFF_LOOPBACK          0x00000004 /* this is loopback interface */
#define IFF_POINTTOPOINT      0x00000008 /* this is point-to-point interface */
#define IFF_MULTICAST         0x00000010 /* multicast is supported */
#else
#define WS_IFF_UP             0x00000001 /* Interface is up */
#define WS_IFF_BROADCAST      0x00000002 /* Broadcast is  supported */
#define WS_IFF_LOOPBACK       0x00000004 /* this is loopback interface */
#define WS_IFF_POINTTOPOINT   0x00000008 /* this is point-to-point interface */
#define WS_IFF_MULTICAST      0x00000010 /* multicast is supported */
#endif /* USE_WS_PREFIX */

#ifndef USE_WS_PREFIX
#define IP_OPTIONS                      1
#define IP_HDRINCL                      2
#define IP_TOS                          3
#define IP_TTL                          4
#define IP_MULTICAST_IF                 9
#define IP_MULTICAST_TTL                10
#define IP_MULTICAST_LOOP               11
#define IP_ADD_MEMBERSHIP               12
#define IP_DROP_MEMBERSHIP              13
#define IP_DONTFRAGMENT                 14
#define IP_ADD_SOURCE_MEMBERSHIP        15
#define IP_DROP_SOURCE_MEMBERSHIP       16
#define IP_BLOCK_SOURCE                 17
#define IP_UNBLOCK_SOURCE               18
#define IP_PKTINFO                      19
#define IP_HOPLIMIT                     21
#define IP_RECVTTL                      21
#define IP_RECEIVE_BROADCAST            22
#define IP_RECVIF                       24
#define IP_RECVDSTADDR                  25
#define IP_IFLIST                       28
#define IP_ADD_IFLIST                   29
#define IP_DEL_IFLIST                   30
#define IP_UNICAST_IF                   31
#define IP_RTHDR                        32
#define IP_GET_IFLIST                   33
#define IP_RECVRTHDR                    38
#define IP_TCLASS                       39
#define IP_RECVTCLASS                   40
#define IP_RECVTOS                      40
#define IP_ORIGINAL_ARRIVAL_IF          47
#define IP_ECN                          50
#define IP_PKTINFO_EX                   51
#define IP_WFP_REDIRECT_RECORDS         60
#define IP_WFP_REDIRECT_CONTEXT         70
#define IP_MTU_DISCOVER                 71
#define IP_MTU                          73
#define IP_NRT_INTERFACE                74
#define IP_RECVERR                      75
#define IP_USER_MTU                     76
#else
#define WS_IP_OPTIONS                   1
#define WS_IP_HDRINCL                   2
#define WS_IP_TOS                       3
#define WS_IP_TTL                       4
#define WS_IP_MULTICAST_IF              9
#define WS_IP_MULTICAST_TTL             10
#define WS_IP_MULTICAST_LOOP            11
#define WS_IP_ADD_MEMBERSHIP            12
#define WS_IP_DROP_MEMBERSHIP           13
#define WS_IP_DONTFRAGMENT              14
#define WS_IP_ADD_SOURCE_MEMBERSHIP     15
#define WS_IP_DROP_SOURCE_MEMBERSHIP    16
#define WS_IP_BLOCK_SOURCE              17
#define WS_IP_UNBLOCK_SOURCE            18
#define WS_IP_PKTINFO                   19
#define WS_IP_HOPLIMIT                  21
#define WS_IP_RECVTTL                   21
#define WS_IP_RECEIVE_BROADCAST         22
#define WS_IP_RECVIF                    24
#define WS_IP_RECVDSTADDR               25
#define WS_IP_IFLIST                    28
#define WS_IP_ADD_IFLIST                29
#define WS_IP_DEL_IFLIST                30
#define WS_IP_UNICAST_IF                31
#define WS_IP_RTHDR                     32
#define WS_IP_GET_IFLIST                33
#define WS_IP_RECVRTHDR                 38
#define WS_IP_TCLASS                    39
#define WS_IP_RECVTCLASS                40
#define WS_IP_RECVTOS                   40
#define WS_IP_ORIGINAL_ARRIVAL_IF       47
#define WS_IP_ECN                       50
#define WS_IP_PKTINFO_EX                51
#define WS_IP_WFP_REDIRECT_RECORDS      60
#define WS_IP_WFP_REDIRECT_CONTEXT      70
#define WS_IP_MTU_DISCOVER              71
#define WS_IP_MTU                       73
#define WS_IP_NRT_INTERFACE             74
#define WS_IP_RECVERR                   75
#define WS_IP_USER_MTU                  76
#endif /* USE_WS_PREFIX */

typedef struct WS(sockaddr_in6)
{
   SHORT    sin6_family;
   USHORT   sin6_port;
   ULONG    sin6_flowinfo;
   IN6_ADDR sin6_addr;
   ULONG    sin6_scope_id;
} SOCKADDR_IN6,*PSOCKADDR_IN6, *LPSOCKADDR_IN6;

typedef struct WS(sockaddr_in6_pair)
{
    PSOCKADDR_IN6 SourceAddress;
    PSOCKADDR_IN6 DestinationAddress;
} SOCKADDR_IN6_PAIR, *PSOCKADDR_IN6_PAIR;

typedef union _SOCKADDR_INET
{
    SOCKADDR_IN     Ipv4;
    SOCKADDR_IN6    Ipv6;
    ADDRESS_FAMILY  si_family;
} SOCKADDR_INET, *PSOCKADDR_INET;

/*
 * Multicast group information
 */

typedef struct WS(ip_mreq)
{
    struct WS(in_addr) imr_multiaddr;
    struct WS(in_addr) imr_interface;
} WS(IP_MREQ), *WS(PIP_MREQ);

typedef struct WS(ipv6_mreq)
{
    struct WS(in6_addr) ipv6mr_multiaddr;
    unsigned int ipv6mr_interface;
} WS(IPV6_MREQ), *WS(PIPV6_MREQ);

typedef struct WS(ip_mreq_source) {
    struct WS(in_addr) imr_multiaddr;
    struct WS(in_addr) imr_sourceaddr;
    struct WS(in_addr) imr_interface;
} WS(IP_MREQ_SOURCE), *WS(PIP_MREQ_SOURCE);

typedef struct WS(ip_msfilter) {
    struct WS(in_addr) imsf_multiaddr;
    struct WS(in_addr) imsf_interface;
    ULONG              imsf_fmode;
    ULONG              imsf_numsrc;
    struct WS(in_addr) imsf_slist[1];
} WS(IP_MSFILTER), *WS(PIP_MSFILTER);

typedef struct WS(in_pktinfo) {
    IN_ADDR ipi_addr;
    UINT    ipi_ifindex;
} IN_PKTINFO, *PIN_PKTINFO;

typedef struct WS(in6_pktinfo) {
    IN6_ADDR ipi6_addr;
    ULONG    ipi6_ifindex;
} IN6_PKTINFO, *PIN6_PKTINFO;

#ifndef USE_WS_PREFIX
#define IPV6_OPTIONS                    1
#define IPV6_HOPOPTS                    1
#define IPV6_HDRINCL                    2
#define IPV6_UNICAST_HOPS               4
#define IPV6_MULTICAST_IF               9
#define IPV6_MULTICAST_HOPS             10
#define IPV6_MULTICAST_LOOP             11
#define IPV6_ADD_MEMBERSHIP             12
#define IPV6_JOIN_GROUP                 IPV6_ADD_MEMBERSHIP
#define IPV6_DROP_MEMBERSHIP            13
#define IPV6_LEAVE_GROUP                IPV6_DROP_MEMBERSHIP
#define IPV6_DONTFRAG                   14
#define IPV6_PKTINFO                    19
#define IPV6_HOPLIMIT                   21
#define IPV6_PROTECTION_LEVEL           23
#define IPV6_RECVIF                     24
#define IPV6_RECVDSTADDR                25
#define IPV6_CHECKSUM                   26
#define IPV6_V6ONLY                     27
#define IPV6_IFLIST                     28
#define IPV6_ADD_IFLIST                 29
#define IPV6_DEL_IFLIST                 30
#define IPV6_UNICAST_IF                 31
#define IPV6_RTHDR                      32
#define IPV6_GET_IFLIST                 33
#define IPV6_RECVRTHDR                  38
#define IPV6_TCLASS                     39
#define IPV6_RECVTCLASS                 40
#define IPV6_ECN                        50
#define IPV6_PKTINFO_EX                 51
#define IPV6_WFP_REDIRECT_RECORDS       60
#define IPV6_WFP_REDIRECT_CONTEXT       70
#define IPV6_MTU_DISCOVER               71
#define IPV6_MTU                        72
#define IPV6_NRT_INTERFACE              74
#define IPV6_RECVERR                    75
#define IPV6_USER_MTU                   76
#else
#define WS_IPV6_OPTIONS                 1
#define WS_IPV6_HOPOPTS                 1
#define WS_IPV6_HDRINCL                 2
#define WS_IPV6_UNICAST_HOPS            4
#define WS_IPV6_MULTICAST_IF            9
#define WS_IPV6_MULTICAST_HOPS          10
#define WS_IPV6_MULTICAST_LOOP          11
#define WS_IPV6_ADD_MEMBERSHIP          12
#define WS_IPV6_DROP_MEMBERSHIP         13
#define WS_IPV6_LEAVE_GROUP             WS_IPV6_DROP_MEMBERSHIP
#define WS_IPV6_DONTFRAG                14
#define WS_IPV6_PKTINFO                 19
#define WS_IPV6_HOPLIMIT                21
#define WS_IPV6_PROTECTION_LEVEL        23
#define WS_IPV6_RECVIF                  24
#define WS_IPV6_RECVDSTADDR             25
#define WS_IPV6_CHECKSUM                26
#define WS_IPV6_V6ONLY                  27
#define WS_IPV6_IFLIST                  28
#define WS_IPV6_ADD_IFLIST              29
#define WS_IPV6_DEL_IFLIST              30
#define WS_IPV6_UNICAST_IF              31
#define WS_IPV6_RTHDR                   32
#define WS_IPV6_GET_IFLIST              33
#define WS_IPV6_RECVRTHDR               38
#define WS_IPV6_TCLASS                  39
#define WS_IPV6_RECVTCLASS              40
#define WS_IPV6_ECN                     50
#define WS_IPV6_PKTINFO_EX              51
#define WS_IPV6_WFP_REDIRECT_RECORDS    60
#define WS_IPV6_WFP_REDIRECT_CONTEXT    70
#define WS_IPV6_MTU_DISCOVER            71
#define WS_IPV6_MTU                     72
#define WS_IPV6_NRT_INTERFACE           74
#define WS_IPV6_RECVERR                 75
#define WS_IPV6_USER_MTU                76
#endif /* USE_WS_PREFIX */

#ifndef USE_WS_PREFIX
#define TCP_OFFLOAD_NO_PREFERENCE       0
#define TCP_OFFLOAD_NOT_PREFERRED       1
#define TCP_OFFLOAD_PREFERRED           2
#else
#define WS_TCP_OFFLOAD_NO_PREFERENCE    0
#define WS_TCP_OFFLOAD_NOT_PREFERRED    1
#define WS_TCP_OFFLOAD_PREFERRED        2
#endif /* USE_WS_PREFIX */

#ifndef USE_WS_PREFIX
/* TCP_NODELAY is defined elsewhere */
#define TCP_EXPEDITED_1122              2
#define TCP_KEEPALIVE                   3
#define TCP_MAXSEG                      4
#define TCP_MAXRT                       5
#define TCP_STDURG                      6
#define TCP_NOURG                       7
#define TCP_ATMARK                      8
#define TCP_NOSYNRETRIES                9
#define TCP_TIMESTAMPS                  10
#define TCP_OFFLOAD_PREFERENCE          11
#define TCP_CONGESTION_ALGORITHM        12
#define TCP_DELAY_FIN_ACK               13
#define TCP_KEEPCNT                     16
#define TCP_KEEPINTVL                   17
#else
/* WS_TCP_NODELAY is defined elsewhere */
#define WS_TCP_EXPEDITED_1122           2
#define WS_TCP_KEEPALIVE                3
#define WS_TCP_MAXSEG                   4
#define WS_TCP_MAXRT                    5
#define WS_TCP_STDURG                   6
#define WS_TCP_NOURG                    7
#define WS_TCP_ATMARK                   8
#define WS_TCP_NOSYNRETRIES             9
#define WS_TCP_TIMESTAMPS               10
#define WS_TCP_OFFLOAD_PREFERENCE       11
#define WS_TCP_CONGESTION_ALGORITHM     12
#define WS_TCP_DELAY_FIN_ACK            13
#define WS_TCP_KEEPCNT                  16
#define WS_TCP_KEEPINTVL                17
#endif /* USE_WS_PREFIX */

#define PROTECTION_LEVEL_UNRESTRICTED   10
#define PROTECTION_LEVEL_EDGERESTRICTED 20
#define PROTECTION_LEVEL_RESTRICTED     30
#define PROTECTION_LEVEL_DEFAULT        ((UINT)-1)

#ifndef USE_WS_PREFIX
#define INET_ADDRSTRLEN         22
#define INET6_ADDRSTRLEN        65
#define IN6ADDR_6BONETESTPREFIX_INIT                    { 0x3f,0xfe,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }
#define IN6ADDR_6TO4PREFIX_INIT                         { 0x20,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }
#define IN6ADDR_ALLMLDV2ROUTERSONLINK_INIT              { 0xff,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0x16 }
#define IN6ADDR_ALLNODESONLINK_INIT                     { 0xff,2,0,0,0,0,0,0,0,0,0,0,0,0,0,1 }
#define IN6ADDR_ALLNODESONNODE_INIT                     { 0xff,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1 }
#define IN6ADDR_ALLROUTERSONLINK_INIT                   { 0xff,2,0,0,0,0,0,0,0,0,0,0,0,0,0,2 }
#define IN6ADDR_ANY_INIT                                { 0 }
#define IN6ADDR_LINKLOCALPREFIX_INIT                    { 0xfe,0x80,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }
#define IN6ADDR_LOOPBACK_INIT                           { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 }
#define IN6ADDR_MULTICASTPREFIX_INIT                    { 0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }
#define IN6ADDR_SITELOCALPREFIX_INIT                    { 0xfe,0xc0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }
#define IN6ADDR_SOLICITEDNODEMULTICASTPREFIX_INIT       { 0xff,2,0,0,0,0,0,0,0,0,0,0x01,0xff,0,0,0 }
#define IN6ADDR_TEREDOINITIALLINKLOCALADDRESS_INIT      { 0xfe,0x80,0,0,0,0,0,0,0,0,0xff,0xff,0xff,0xff,0xff,0xfe }
#define IN6ADDR_TEREDOPREFIX_INIT                       { 0x20,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }
#define IN6ADDR_TEREDOPREFIX_INIT_OLD                   { 0x3f,0xfe,0x83,0x1f,0,0,0,0,0,0,0,0,0,0,0,0 }
#define IN6ADDR_ULAPREFIX_INIT                          { 0xfc,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }
#define IN6ADDR_V4MAPPEDPREFIX_INIT                     { 0,0,0,0,0,0,0,0,0,0,0xff,0xff,0,0,0,0 }
#else
#define WS_INET_ADDRSTRLEN      22
#define WS_INET6_ADDRSTRLEN     65
#define WS_IN6ADDR_6BONETESTPREFIX_INIT                 { 0x3f,0xfe,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }
#define WS_IN6ADDR_6TO4PREFIX_INIT                      { 0x20,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }
#define WS_IN6ADDR_ALLMLDV2ROUTERSONLINK_INIT           { 0xff,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0x16 }
#define WS_IN6ADDR_ALLNODESONLINK_INIT                  { 0xff,2,0,0,0,0,0,0,0,0,0,0,0,0,0,1 }
#define WS_IN6ADDR_ALLNODESONNODE_INIT                  { 0xff,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1 }
#define WS_IN6ADDR_ALLROUTERSONLINK_INIT                { 0xff,2,0,0,0,0,0,0,0,0,0,0,0,0,0,2 }
#define WS_IN6ADDR_ANY_INIT                             { 0 }
#define WS_IN6ADDR_LINKLOCALPREFIX_INIT                 { 0xfe,0x80,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }
#define WS_IN6ADDR_LOOPBACK_INIT                        { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 }
#define WS_IN6ADDR_MULTICASTPREFIX_INIT                 { 0xff,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }
#define WS_IN6ADDR_SITELOCALPREFIX_INIT                 { 0xfe,0xc0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }
#define WS_IN6ADDR_SOLICITEDNODEMULTICASTPREFIX_INIT    { 0xff,2,0,0,0,0,0,0,0,0,0,0x01,0xff,0,0,0 }
#define WS_IN6ADDR_TEREDOINITIALLINKLOCALADDRESS_INIT   { 0xfe,0x80,0,0,0,0,0,0,0,0,0xff,0xff,0xff,0xff,0xff,0xfe }
#define WS_IN6ADDR_TEREDOPREFIX_INIT                    { 0x20,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }
#define WS_IN6ADDR_TEREDOPREFIX_INIT_OLD                { 0x3f,0xfe,0x83,0x1f,0,0,0,0,0,0,0,0,0,0,0,0 }
#define WS_IN6ADDR_ULAPREFIX_INIT                       { 0xfc,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }
#define WS_IN6ADDR_V4MAPPEDPREFIX_INIT                  { 0,0,0,0,0,0,0,0,0,0,0xff,0xff,0,0,0,0 }
#endif /* USE_WS_PREFIX */

#define SS_PORT(ssp) (((PSOCKADDR_IN)(ssp))->sin_port)

#ifndef USE_WS_PREFIX
#define SIO_IDEAL_SEND_BACKLOG_CHANGE _IO ('t', 122)
#define SIO_IDEAL_SEND_BACKLOG_QUERY  _IOR('t', 123, ULONG)
#else
#define WS_SIO_IDEAL_SEND_BACKLOG_CHANGE WS__IO ('t', 122)
#define WS_SIO_IDEAL_SEND_BACKLOG_QUERY  WS__IOR('t', 123, ULONG)
#endif

#ifndef USE_WS_PREFIX
extern const IN_ADDR in4addr_alligmpv3routersonlink;
extern const IN_ADDR in4addr_allnodesonlink;
extern const IN_ADDR in4addr_allroutersonlink;
extern const IN_ADDR in4addr_allteredohostsonlink;
extern const IN_ADDR in4addr_any;
extern const IN_ADDR in4addr_broadcast;
extern const IN_ADDR in4addr_linklocalprefix;
extern const IN_ADDR in4addr_loopback;
extern const IN_ADDR in4addr_multicastprefix;
extern const IN6_ADDR in6addr_6to4prefix;
extern const IN6_ADDR in6addr_allmldv2routersonlink;
extern const IN6_ADDR in6addr_allnodesonlink;
extern const IN6_ADDR in6addr_allnodesonnode;
extern const IN6_ADDR in6addr_allroutersonlink;
extern const IN6_ADDR in6addr_any;
extern const IN6_ADDR in6addr_linklocalprefix;
extern const IN6_ADDR in6addr_loopback;
extern const IN6_ADDR in6addr_multicastprefix;
extern const IN6_ADDR in6addr_solicitednodemulticastprefix;
extern const IN6_ADDR in6addr_teredoinitiallinklocaladdress;
extern const IN6_ADDR in6addr_teredoprefix;
extern const IN6_ADDR in6addr_teredoprefix_old;
extern const IN6_ADDR in6addr_v4mappedprefix;
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline BOOLEAN WS(IN6_IS_ADDR_LOOPBACK) ( const IN6_ADDR *a )
{
    return ((a->s6_words[0] == 0) &&
            (a->s6_words[1] == 0) &&
            (a->s6_words[2] == 0) &&
            (a->s6_words[3] == 0) &&
            (a->s6_words[4] == 0) &&
            (a->s6_words[5] == 0) &&
            (a->s6_words[6] == 0) &&
            (a->s6_words[7] == 0x0100));
}

static inline BOOLEAN WS(IN6_IS_ADDR_MULTICAST) ( const IN6_ADDR *a )
{
    return (a->s6_bytes[0] == 0xff);
}

static inline BOOLEAN WS(IN6_IS_ADDR_UNSPECIFIED) ( const IN6_ADDR *a )
{
    return ((a->s6_words[0] == 0) &&
            (a->s6_words[1] == 0) &&
            (a->s6_words[2] == 0) &&
            (a->s6_words[3] == 0) &&
            (a->s6_words[4] == 0) &&
            (a->s6_words[5] == 0) &&
            (a->s6_words[6] == 0) &&
            (a->s6_words[7] == 0));
}

static inline BOOLEAN WS(IN6_IS_ADDR_LINKLOCAL) ( const IN6_ADDR *a )
{
    return ((a->s6_bytes[0] == 0xfe) && ((a->s6_bytes[1] & 0xc0) == 0x80));
}

static inline BOOLEAN WS(IN6_IS_ADDR_SITELOCAL) ( const IN6_ADDR *a )
{
    return ((a->s6_bytes[0] == 0xfe) && ((a->s6_bytes[1] & 0xc0) == 0xc0));
}

#ifdef __cplusplus
}
#endif

#endif /* __WS2IPDEF__ */
