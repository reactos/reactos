#pragma once

#define _WS2IPDEF_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WS2IPDEF_ASSERT
#define WS2IPDEF_ASSERT(exp) ((VOID) 0)
#endif

#ifdef _MSC_VER
#define WS2TCPIP_INLINE __inline
#else
#define WS2TCPIP_INLINE static inline
#endif

#include <in6addr.h>

#define IFF_UP              0x00000001
#define IFF_BROADCAST       0x00000002
#define IFF_LOOPBACK        0x00000004
#define IFF_POINTTOPOINT    0x00000008
#define IFF_MULTICAST       0x00000010

#define IP_OPTIONS                 1
#define IP_HDRINCL                 2
#define IP_TOS                     3
#define IP_TTL                     4
#define IP_MULTICAST_IF            9
#define IP_MULTICAST_TTL          10
#define IP_MULTICAST_LOOP         11
#define IP_ADD_MEMBERSHIP         12
#define IP_DROP_MEMBERSHIP        13
#define IP_DONTFRAGMENT           14
#define IP_ADD_SOURCE_MEMBERSHIP  15
#define IP_DROP_SOURCE_MEMBERSHIP 16
#define IP_BLOCK_SOURCE           17
#define IP_UNBLOCK_SOURCE         18
#define IP_PKTINFO                19
#define IP_HOPLIMIT               21
#define IP_RECEIVE_BROADCAST      22
#define IP_RECVIF                 24
#define IP_RECVDSTADDR            25
#define IP_IFLIST                 28
#define IP_ADD_IFLIST             29
#define IP_DEL_IFLIST             30
#define IP_UNICAST_IF             31
#define IP_RTHDR                  32
#define IP_RECVRTHDR              38
#define IP_TCLASS                 39
#define IP_RECVTCLASS             40
#define IP_ORIGINAL_ARRIVAL_IF    47

#define IP_UNSPECIFIED_TYPE_OF_SERVICE -1

#define IPV6_ADDRESS_BITS RTL_BITS_OF(IN6_ADDR)

#define SS_PORT(ssp) (((PSOCKADDR_IN)(ssp))->sin_port)

#define SIO_GET_INTERFACE_LIST     _IOR('t', 127, ULONG)
#define SIO_GET_INTERFACE_LIST_EX  _IOR('t', 126, ULONG)
#define SIO_SET_MULTICAST_FILTER   _IOW('t', 125, ULONG)
#define SIO_GET_MULTICAST_FILTER   _IOW('t', 124 | IOC_IN, ULONG)
#define SIOCSIPMSFILTER            SIO_SET_MULTICAST_FILTER
#define SIOCGIPMSFILTER            SIO_GET_MULTICAST_FILTER

#define SIOCSMSFILTER     _IOW('t', 126, ULONG)
#define SIOCGMSFILTER     _IOW('t', 127 | IOC_IN, ULONG)

#if (NTDDI_VERSION >= NTDDI_VISTASP1)

#define IDEAL_SEND_BACKLOG_IOCTLS

#define SIO_IDEAL_SEND_BACKLOG_QUERY   _IOR('t', 123, ULONG)
#define SIO_IDEAL_SEND_BACKLOG_CHANGE   _IO('t', 122)

#endif /* (NTDDI_VERSION >= NTDDI_VISTASP1) */

#define MCAST_JOIN_GROUP            41
#define MCAST_LEAVE_GROUP           42
#define MCAST_BLOCK_SOURCE          43
#define MCAST_UNBLOCK_SOURCE        44
#define MCAST_JOIN_SOURCE_GROUP     45
#define MCAST_LEAVE_SOURCE_GROUP    46

#define IP_MSFILTER_SIZE(NumSources) \
  (sizeof(IP_MSFILTER) - sizeof(IN_ADDR) + (NumSources) * sizeof(IN_ADDR))

#define IPV6_HOPOPTS           1
#define IPV6_HDRINCL           2
#define IPV6_UNICAST_HOPS      4
#define IPV6_MULTICAST_IF      9
#define IPV6_MULTICAST_HOPS   10
#define IPV6_MULTICAST_LOOP   11
#define IPV6_ADD_MEMBERSHIP   12
#define IPV6_JOIN_GROUP       IPV6_ADD_MEMBERSHIP
#define IPV6_DROP_MEMBERSHIP  13
#define IPV6_LEAVE_GROUP      IPV6_DROP_MEMBERSHIP
#define IPV6_DONTFRAG         14
#define IPV6_PKTINFO          19
#define IPV6_HOPLIMIT         21
#define IPV6_PROTECTION_LEVEL 23
#define IPV6_RECVIF           24
#define IPV6_RECVDSTADDR      25
#define IPV6_CHECKSUM         26
#define IPV6_V6ONLY           27
#define IPV6_IFLIST           28
#define IPV6_ADD_IFLIST       29
#define IPV6_DEL_IFLIST       30
#define IPV6_UNICAST_IF       31
#define IPV6_RTHDR            32
#define IPV6_RECVRTHDR        38
#define IPV6_TCLASS           39
#define IPV6_RECVTCLASS       40

#define IP_UNSPECIFIED_HOP_LIMIT -1

#define IP_PROTECTION_LEVEL IPV6_PROTECTION_LEVEL
#define PROTECTION_LEVEL_UNRESTRICTED   10
#define PROTECTION_LEVEL_EDGERESTRICTED 20
#define PROTECTION_LEVEL_RESTRICTED     30

#if (NTDDI_VERSION < NTDDI_VISTA)
#define PROTECTION_LEVEL_DEFAULT PROTECTION_LEVEL_EDGERESTRICTED
#else
#define PROTECTION_LEVEL_DEFAULT ((UINT)-1)
#endif

#define INET_ADDRSTRLEN  22
#define INET6_ADDRSTRLEN 65

#define TCP_OFFLOAD_NO_PREFERENCE 0
#define TCP_OFFLOAD_NOT_PREFERRED 1
#define TCP_OFFLOAD_PREFERRED 2

#define TCP_EXPEDITED_1122       0x0002
#define TCP_KEEPALIVE            3
#define TCP_MAXSEG               4
#define TCP_MAXRT                5
#define TCP_STDURG               6
#define TCP_NOURG                7
#define TCP_ATMARK               8
#define TCP_NOSYNRETRIES         9
#define TCP_TIMESTAMPS           10
#define TCP_OFFLOAD_PREFERENCE   11
#define TCP_CONGESTION_ALGORITHM 12
#define TCP_DELAY_FIN_ACK        13

struct sockaddr_in6_old {
  SHORT sin6_family;
  USHORT sin6_port;
  ULONG sin6_flowinfo;
  IN6_ADDR sin6_addr;
};

typedef union sockaddr_gen {
  struct sockaddr Address;
  struct sockaddr_in AddressIn;
  struct sockaddr_in6_old AddressIn6;
} sockaddr_gen;

typedef struct _INTERFACE_INFO {
  ULONG iiFlags;
  sockaddr_gen iiAddress;
  sockaddr_gen iiBroadcastAddress;
  sockaddr_gen iiNetmask;
} INTERFACE_INFO, FAR *LPINTERFACE_INFO;

typedef struct _INTERFACE_INFO_EX {
  ULONG iiFlags;
  SOCKET_ADDRESS iiAddress;
  SOCKET_ADDRESS iiBroadcastAddress;
  SOCKET_ADDRESS iiNetmask;
} INTERFACE_INFO_EX, FAR *LPINTERFACE_INFO_EX;

typedef struct sockaddr_in6 {
  ADDRESS_FAMILY sin6_family;
  USHORT sin6_port;
  ULONG sin6_flowinfo;
  IN6_ADDR sin6_addr;
  union {
    ULONG sin6_scope_id;
    SCOPE_ID sin6_scope_struct;
  };
} SOCKADDR_IN6_LH, *PSOCKADDR_IN6_LH, FAR *LPSOCKADDR_IN6_LH;

typedef struct sockaddr_in6_w2ksp1 {
  short sin6_family;
  USHORT sin6_port;
  ULONG sin6_flowinfo;
  struct in6_addr sin6_addr;
  ULONG sin6_scope_id;
} SOCKADDR_IN6_W2KSP1, *PSOCKADDR_IN6_W2KSP1, FAR *LPSOCKADDR_IN6_W2KSP1;

#if (NTDDI_VERSION >= NTDDI_VISTA)

typedef SOCKADDR_IN6_LH SOCKADDR_IN6;
typedef SOCKADDR_IN6_LH *PSOCKADDR_IN6;
typedef SOCKADDR_IN6_LH FAR *LPSOCKADDR_IN6;

#elif(NTDDI_VERSION >= NTDDI_WIN2KSP1)

typedef SOCKADDR_IN6_W2KSP1 SOCKADDR_IN6;
typedef SOCKADDR_IN6_W2KSP1 *PSOCKADDR_IN6;
typedef SOCKADDR_IN6_W2KSP1 FAR *LPSOCKADDR_IN6;

#else

typedef SOCKADDR_IN6_LH SOCKADDR_IN6;
typedef SOCKADDR_IN6_LH *PSOCKADDR_IN6;
typedef SOCKADDR_IN6_LH FAR *LPSOCKADDR_IN6;

#endif /* (NTDDI_VERSION >= NTDDI_VISTA) */

typedef union _SOCKADDR_INET {
  SOCKADDR_IN Ipv4;
  SOCKADDR_IN6 Ipv6;
  ADDRESS_FAMILY si_family;
} SOCKADDR_INET, *PSOCKADDR_INET;

typedef struct _sockaddr_in6_pair {
  PSOCKADDR_IN6 SourceAddress;
  PSOCKADDR_IN6 DestinationAddress;
} SOCKADDR_IN6_PAIR, *PSOCKADDR_IN6_PAIR;

#if (NTDDI_VERSION >= NTDDI_WIN2KSP1)

#define IN6ADDR_ANY_INIT {0}
#define IN6ADDR_LOOPBACK_INIT {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}
#define IN6ADDR_ALLNODESONNODE_INIT {0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}
#define IN6ADDR_ALLNODESONLINK_INIT {0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}
#define IN6ADDR_ALLROUTERSONLINK_INIT {0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02}
#define IN6ADDR_ALLMLDV2ROUTERSONLINK_INIT {0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16}
#define IN6ADDR_TEREDOINITIALLINKLOCALADDRESS_INIT {0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe}
#define IN6ADDR_TEREDOOLDLINKLOCALADDRESSXP_INIT {0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 'T', 'E', 'R', 'E', 'D', 'O'}
#define IN6ADDR_TEREDOOLDLINKLOCALADDRESSVISTA_INIT {0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}
#define IN6ADDR_LINKLOCALPREFIX_INIT {0xfe, 0x80, }
#define IN6ADDR_MULTICASTPREFIX_INIT {0xff, 0x00, }
#define IN6ADDR_SOLICITEDNODEMULTICASTPREFIX_INIT {0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, }
#define IN6ADDR_V4MAPPEDPREFIX_INIT {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, }
#define IN6ADDR_6TO4PREFIX_INIT {0x20, 0x02, }
#define IN6ADDR_TEREDOPREFIX_INIT {0x20, 0x01, 0x00, 0x00, }
#define IN6ADDR_TEREDOPREFIX_INIT_OLD {0x3f, 0xfe, 0x83, 0x1f, }

#define IN6ADDR_LINKLOCALPREFIX_LENGTH 64
#define IN6ADDR_MULTICASTPREFIX_LENGTH 8
#define IN6ADDR_SOLICITEDNODEMULTICASTPREFIX_LENGTH 104
#define IN6ADDR_V4MAPPEDPREFIX_LENGTH 96
#define IN6ADDR_6TO4PREFIX_LENGTH 16
#define IN6ADDR_TEREDOPREFIX_LENGTH 32

extern CONST SCOPE_ID scopeid_unspecified;

extern CONST IN_ADDR in4addr_any;
extern CONST IN_ADDR in4addr_loopback;
extern CONST IN_ADDR in4addr_broadcast;
extern CONST IN_ADDR in4addr_allnodesonlink;
extern CONST IN_ADDR in4addr_allroutersonlink;
extern CONST IN_ADDR in4addr_alligmpv3routersonlink;
extern CONST IN_ADDR in4addr_allteredohostsonlink;
extern CONST IN_ADDR in4addr_linklocalprefix;
extern CONST IN_ADDR in4addr_multicastprefix;

extern CONST IN6_ADDR in6addr_any;
extern CONST IN6_ADDR in6addr_loopback;
extern CONST IN6_ADDR in6addr_allnodesonnode;
extern CONST IN6_ADDR in6addr_allnodesonlink;
extern CONST IN6_ADDR in6addr_allroutersonlink;
extern CONST IN6_ADDR in6addr_allmldv2routersonlink;
extern CONST IN6_ADDR in6addr_teredoinitiallinklocaladdress;
extern CONST IN6_ADDR in6addr_linklocalprefix;
extern CONST IN6_ADDR in6addr_multicastprefix;
extern CONST IN6_ADDR in6addr_solicitednodemulticastprefix;
extern CONST IN6_ADDR in6addr_v4mappedprefix;
extern CONST IN6_ADDR in6addr_6to4prefix;
extern CONST IN6_ADDR in6addr_teredoprefix;
extern CONST IN6_ADDR in6addr_teredoprefix_old;

#ifndef __midl

WS2TCPIP_INLINE
BOOLEAN
IN6_ADDR_EQUAL(CONST IN6_ADDR *x, CONST IN6_ADDR *y) {
  __int64 UNALIGNED *a;
  __int64 UNALIGNED *b;

  a = (__int64 UNALIGNED *)x;
  b = (__int64 UNALIGNED *)y;
  return (BOOLEAN)((a[1] == b[1]) && (a[0] == b[0]));
}

#define IN6_ARE_ADDR_EQUAL IN6_ADDR_EQUAL

WS2TCPIP_INLINE
BOOLEAN
IN6_IS_ADDR_UNSPECIFIED(CONST IN6_ADDR *a) {
  return (BOOLEAN)((a->s6_words[0] == 0) &&
                   (a->s6_words[1] == 0) &&
                   (a->s6_words[2] == 0) &&
                   (a->s6_words[3] == 0) &&
                   (a->s6_words[4] == 0) &&
                   (a->s6_words[5] == 0) &&
                   (a->s6_words[6] == 0) &&
                   (a->s6_words[7] == 0));
}

WS2TCPIP_INLINE
BOOLEAN
IN6_IS_ADDR_LOOPBACK(CONST IN6_ADDR *a) {
  return (BOOLEAN)((a->s6_words[0] == 0) &&
                   (a->s6_words[1] == 0) &&
                   (a->s6_words[2] == 0) &&
                   (a->s6_words[3] == 0) &&
                   (a->s6_words[4] == 0) &&
                   (a->s6_words[5] == 0) &&
                   (a->s6_words[6] == 0) &&
                   (a->s6_words[7] == 0x0100));
}

WS2TCPIP_INLINE
BOOLEAN
IN6_IS_ADDR_MULTICAST(CONST IN6_ADDR *a) {
  return (BOOLEAN)(a->s6_bytes[0] == 0xff);
}

WS2TCPIP_INLINE
BOOLEAN
IN6_IS_ADDR_EUI64(CONST IN6_ADDR *a) {
  return (BOOLEAN)(((a->s6_bytes[0] & 0xe0) != 0) &&
                   !IN6_IS_ADDR_MULTICAST(a));
}

WS2TCPIP_INLINE
BOOLEAN
IN6_IS_ADDR_SUBNET_ROUTER_ANYCAST(CONST IN6_ADDR *a) {
  return (BOOLEAN)(IN6_IS_ADDR_EUI64(a) &&
                   (a->s6_words[4] == 0) &&
                   (a->s6_words[5] == 0) &&
                   (a->s6_words[6] == 0) &&
                   (a->s6_words[7] == 0));
}

WS2TCPIP_INLINE
BOOLEAN
IN6_IS_ADDR_SUBNET_RESERVED_ANYCAST(CONST IN6_ADDR *a) {
  return (BOOLEAN)(IN6_IS_ADDR_EUI64(a) &&
                   (a->s6_words[4] == 0xfffd) &&
                   (a->s6_words[5] == 0xffff) &&
                   (a->s6_words[6] == 0xffff) &&
                   ((a->s6_words[7] & 0x80ff) == 0x80ff));
}

WS2TCPIP_INLINE
BOOLEAN
IN6_IS_ADDR_ANYCAST(CONST IN6_ADDR *a) {
  return (IN6_IS_ADDR_SUBNET_RESERVED_ANYCAST(a) ||
          IN6_IS_ADDR_SUBNET_ROUTER_ANYCAST(a));
}

WS2TCPIP_INLINE
BOOLEAN
IN6_IS_ADDR_LINKLOCAL(CONST IN6_ADDR *a) {
  return (BOOLEAN)((a->s6_bytes[0] == 0xfe) &&
                   ((a->s6_bytes[1] & 0xc0) == 0x80));
}

WS2TCPIP_INLINE
BOOLEAN
IN6_IS_ADDR_SITELOCAL(CONST IN6_ADDR *a) {
  return (BOOLEAN)((a->s6_bytes[0] == 0xfe) &&
                   ((a->s6_bytes[1] & 0xc0) == 0xc0));
}

WS2TCPIP_INLINE
BOOLEAN
IN6_IS_ADDR_GLOBAL(CONST IN6_ADDR *a) {
  ULONG High = (a->s6_bytes[0] & 0xf0);
  return (BOOLEAN)((High != 0) && (High != 0xf0));
}

WS2TCPIP_INLINE
BOOLEAN
IN6_IS_ADDR_V4MAPPED(CONST IN6_ADDR *a) {
  return (BOOLEAN)((a->s6_words[0] == 0) &&
                   (a->s6_words[1] == 0) &&
                   (a->s6_words[2] == 0) &&
                   (a->s6_words[3] == 0) &&
                   (a->s6_words[4] == 0) &&
                   (a->s6_words[5] == 0xffff));
}

WS2TCPIP_INLINE
BOOLEAN
IN6_IS_ADDR_V4COMPAT(CONST IN6_ADDR *a) {
  return (BOOLEAN)((a->s6_words[0] == 0) &&
                   (a->s6_words[1] == 0) &&
                   (a->s6_words[2] == 0) &&
                   (a->s6_words[3] == 0) &&
                   (a->s6_words[4] == 0) &&
                   (a->s6_words[5] == 0) &&
                   !((a->s6_words[6] == 0) &&
                     (a->s6_addr[14] == 0) &&
                     ((a->s6_addr[15] == 0) || (a->s6_addr[15] == 1))));
}

WS2TCPIP_INLINE
BOOLEAN
IN6_IS_ADDR_V4TRANSLATED(CONST IN6_ADDR *a) {
  return (BOOLEAN)((a->s6_words[0] == 0) &&
                   (a->s6_words[1] == 0) &&
                   (a->s6_words[2] == 0) &&
                   (a->s6_words[3] == 0) &&
                   (a->s6_words[4] == 0xffff) &&
                   (a->s6_words[5] == 0));
}

WS2TCPIP_INLINE
BOOLEAN
IN6_IS_ADDR_MC_NODELOCAL(CONST IN6_ADDR *a) {
  return (BOOLEAN)(IN6_IS_ADDR_MULTICAST(a) &&
                   ((a->s6_bytes[1] & 0xf) == 1));
}

WS2TCPIP_INLINE
BOOLEAN
IN6_IS_ADDR_MC_LINKLOCAL(CONST IN6_ADDR *a) {
  return (BOOLEAN)(IN6_IS_ADDR_MULTICAST(a) &&
                   ((a->s6_bytes[1] & 0xf) == 2));
}

WS2TCPIP_INLINE
BOOLEAN
IN6_IS_ADDR_MC_SITELOCAL(CONST IN6_ADDR *a) {
  return (BOOLEAN)(IN6_IS_ADDR_MULTICAST(a) &&
                   ((a->s6_bytes[1] & 0xf) == 5));
}

WS2TCPIP_INLINE
BOOLEAN
IN6_IS_ADDR_MC_ORGLOCAL(CONST IN6_ADDR *a) {
  return (BOOLEAN)(IN6_IS_ADDR_MULTICAST(a) &&
                   ((a->s6_bytes[1] & 0xf) == 8));
}

WS2TCPIP_INLINE
BOOLEAN
IN6_IS_ADDR_MC_GLOBAL(CONST IN6_ADDR *a) {
  return (BOOLEAN)(IN6_IS_ADDR_MULTICAST(a) &&
                   ((a->s6_bytes[1] & 0xf) == 0xe));
}

WS2TCPIP_INLINE
VOID
IN6_SET_ADDR_UNSPECIFIED(PIN6_ADDR a) {
  memset(a->s6_bytes, 0, sizeof(IN6_ADDR));
}

WS2TCPIP_INLINE
VOID
IN6_SET_ADDR_LOOPBACK(PIN6_ADDR a) {
  memset(a->s6_bytes, 0, sizeof(IN6_ADDR));
  a->s6_bytes[15] = 1;
}

WS2TCPIP_INLINE
VOID
IN6ADDR_SETANY(PSOCKADDR_IN6 a) {
  a->sin6_family = AF_INET6;
  a->sin6_port = 0;
  a->sin6_flowinfo = 0;
  IN6_SET_ADDR_UNSPECIFIED(&a->sin6_addr);
  a->sin6_scope_id = 0;
}

WS2TCPIP_INLINE
VOID
IN6ADDR_SETLOOPBACK(PSOCKADDR_IN6 a) {
  a->sin6_family = AF_INET6;
  a->sin6_port = 0;
  a->sin6_flowinfo = 0;
  IN6_SET_ADDR_LOOPBACK(&a->sin6_addr);
  a->sin6_scope_id = 0;
}

WS2TCPIP_INLINE
BOOLEAN
IN6ADDR_ISANY(CONST SOCKADDR_IN6 *a) {
  WS2IPDEF_ASSERT(a->sin6_family == AF_INET6);
  return IN6_IS_ADDR_UNSPECIFIED(&a->sin6_addr);
}

WS2TCPIP_INLINE
BOOLEAN
IN6ADDR_ISLOOPBACK(CONST SOCKADDR_IN6 *a) {
  WS2IPDEF_ASSERT(a->sin6_family == AF_INET6);
  return IN6_IS_ADDR_LOOPBACK(&a->sin6_addr);
}

WS2TCPIP_INLINE
BOOLEAN
IN6ADDR_ISEQUAL(CONST SOCKADDR_IN6 *a, CONST SOCKADDR_IN6 *b) {
  WS2IPDEF_ASSERT(a->sin6_family == AF_INET6);
  return (BOOLEAN)(a->sin6_scope_id == b->sin6_scope_id &&
                   IN6_ADDR_EQUAL(&a->sin6_addr, &b->sin6_addr));
}

WS2TCPIP_INLINE
BOOLEAN
IN6ADDR_ISUNSPECIFIED(CONST SOCKADDR_IN6 *a) {
  WS2IPDEF_ASSERT(a->sin6_family == AF_INET6);
  return (BOOLEAN)(a->sin6_scope_id == 0 &&
                   IN6_IS_ADDR_UNSPECIFIED(&a->sin6_addr));
}

#endif /* __midl */

#endif /* (NTDDI_VERSION >= NTDDI_WIN2KSP1) */

typedef enum _MULTICAST_MODE_TYPE {
  MCAST_INCLUDE = 0,
  MCAST_EXCLUDE
} MULTICAST_MODE_TYPE;

typedef struct ip_mreq {
  IN_ADDR imr_multiaddr;
  IN_ADDR imr_interface;
} IP_MREQ, *PIP_MREQ;

typedef struct ip_mreq_source {
  IN_ADDR imr_multiaddr;
  IN_ADDR imr_sourceaddr;
  IN_ADDR imr_interface;
} IP_MREQ_SOURCE, *PIP_MREQ_SOURCE;

typedef struct ip_msfilter {
  IN_ADDR imsf_multiaddr;
  IN_ADDR imsf_interface;
  MULTICAST_MODE_TYPE imsf_fmode;
  ULONG imsf_numsrc;
  IN_ADDR imsf_slist[1];
} IP_MSFILTER, *PIP_MSFILTER;

typedef struct ipv6_mreq {
  IN6_ADDR ipv6mr_multiaddr;
  ULONG ipv6mr_interface;
} IPV6_MREQ, *PIPV6_MREQ;

#if (NTDDI_VERSION >= NTDDI_WINXP)

typedef struct group_req {
  ULONG gr_interface;
  SOCKADDR_STORAGE gr_group;
} GROUP_REQ, *PGROUP_REQ;

typedef struct group_source_req {
  ULONG gsr_interface;
  SOCKADDR_STORAGE gsr_group;
  SOCKADDR_STORAGE gsr_source;
} GROUP_SOURCE_REQ, *PGROUP_SOURCE_REQ;

typedef struct group_filter {
  ULONG gf_interface;
  SOCKADDR_STORAGE gf_group;
  MULTICAST_MODE_TYPE gf_fmode;
  ULONG gf_numsrc;
  SOCKADDR_STORAGE gf_slist[1];
} GROUP_FILTER, *PGROUP_FILTER;

#define GROUP_FILTER_SIZE(numsrc) \
  (sizeof(GROUP_FILTER) - sizeof(SOCKADDR_STORAGE) \
  + (numsrc) * sizeof(SOCKADDR_STORAGE))

#endif /* (NTDDI_VERSION >= NTDDI_WINXP) */

typedef struct in_pktinfo {
  IN_ADDR ipi_addr;
  ULONG ipi_ifindex;
} IN_PKTINFO, *PIN_PKTINFO;

C_ASSERT(sizeof(IN_PKTINFO) == 8);

typedef struct in6_pktinfo {
  IN6_ADDR ipi6_addr;
  ULONG ipi6_ifindex;
} IN6_PKTINFO, *PIN6_PKTINFO;

C_ASSERT(sizeof(IN6_PKTINFO) == 20);

#ifdef __cplusplus
}
#endif
