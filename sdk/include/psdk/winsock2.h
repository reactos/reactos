#pragma once

#if !(defined _WINSOCK2API_ || defined _WINSOCKAPI_)
#define _WINSOCK2API_
#define _WINSOCKAPI_ /* to prevent later inclusion of winsock.h */

#define _GNU_H_WINDOWS32_SOCKETS

#if (!defined(_WIN64) && !defined(WIN32))
#include <pshpack4.h>
#define _NEED_POPPACK
#endif

#ifndef INCL_WINSOCK_API_PROTOTYPES
#define INCL_WINSOCK_API_PROTOTYPES 1
#endif

#ifndef INCL_WINSOCK_API_TYPEDEFS
#define INCL_WINSOCK_API_TYPEDEFS 0
#endif

#ifndef _INC_WINDOWS
#include <windows.h>
#endif

#if !defined(MAKEWORD)
#define MAKEWORD(low,high) ((WORD)(((BYTE)(low)) | ((WORD)((BYTE)(high))) << 8))
#endif

#ifndef WINSOCK_VERSION
#define WINSOCK_VERSION MAKEWORD(2,2)
#endif

#ifndef WINSOCK_API_LINKAGE
#ifdef DECLSPEC_IMPORT
#define WINSOCK_API_LINKAGE DECLSPEC_IMPORT
#else
#define WINSOCK_API_LINKAGE
#endif
#endif

#ifndef _M_AMD64
#if !defined(__ROS_LONG64__)
#ifdef __WINESRC__
#define __ROS_LONG64__
#endif
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Names common to Winsock1.1 and Winsock2 */
#if !defined ( _BSDTYPES_DEFINED )
/* also defined in gmon.h and in cygwin's sys/types */
typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
#ifndef __ROS_LONG64__
typedef unsigned long u_long;
#else
typedef unsigned int u_long;
#endif
#define _BSDTYPES_DEFINED
#endif /* ! def _BSDTYPES_DEFINED  */

#if(_WIN32_WINNT >= 0x0501)
typedef unsigned __int64 u_int64;
#endif /* (_WIN32_WINNT >= 0x0501) */

#include <ws2def.h>

typedef UINT_PTR SOCKET;

#ifndef FD_SETSIZE
#define FD_SETSIZE 64
#endif

#ifndef _SYS_TYPES_FD_SET

/* fd_set may be defined by the newlib <sys/types.h>
 * if __USE_W32_SOCKETS not defined.
 */
#ifdef fd_set
#undef fd_set
#endif

typedef struct fd_set {
  u_int fd_count;
  SOCKET fd_array[FD_SETSIZE];
} fd_set;

extern int PASCAL FAR __WSAFDIsSet(SOCKET,fd_set FAR*);

#ifndef FD_CLR
#define FD_CLR(fd, set) do { \
  u_int __i; \
  for (__i = 0; __i < ((fd_set FAR*)(set))->fd_count ; __i++) { \
    if (((fd_set FAR*)(set))->fd_array[__i] == fd) { \
      while (__i < ((fd_set FAR*)(set))->fd_count-1) { \
        ((fd_set FAR*)(set))->fd_array[__i] = \
        ((fd_set FAR*)(set))->fd_array[__i+1]; \
        __i++; \
      } \
    ((fd_set FAR*)(set))->fd_count--; \
    break; \
    } \
  } \
} while(0)
#endif

#ifndef FD_SET
/* this differs from the define in winsock.h and in cygwin sys/types.h */
#define FD_SET(fd, set) do { \
  u_int __i; \
  for (__i = 0; __i < ((fd_set FAR *)(set))->fd_count; __i++) { \
    if (((fd_set FAR *)(set))->fd_array[__i] == (fd)) { \
      break; \
    } \
  } \
  if (__i == ((fd_set FAR *)(set))->fd_count) { \
    if (((fd_set FAR *)(set))->fd_count < FD_SETSIZE) { \
      ((fd_set FAR *)(set))->fd_array[__i] = (fd); \
      ((fd_set FAR *)(set))->fd_count++; \
    } \
  } \
} while(0)
#endif

#ifndef FD_ZERO
#define FD_ZERO(set) (((fd_set FAR*)(set))->fd_count=0)
#endif

#ifndef FD_ISSET
#define FD_ISSET(fd, set) __WSAFDIsSet((SOCKET)(fd), (fd_set FAR*)(set))
#endif

#elif !defined (USE_SYS_TYPES_FD_SET)
#warning "fd_set and associated macros have been defined in sys/types.  \
    This may cause runtime problems with W32 sockets"
#endif /* ndef _SYS_TYPES_FD_SET */

#if !(defined (__INSIDE_CYGWIN__) || defined (__INSIDE_MSYS__))

#ifndef _TIMEVAL_DEFINED

/* also in sys/time.h */
#define _TIMEVAL_DEFINED
#define _STRUCT_TIMEVAL
struct timeval {
  LONG tv_sec;
  LONG tv_usec;
};

#define timerisset(tvp) ((tvp)->tv_sec || (tvp)->tv_usec)

#define timercmp(tvp, uvp, cmp) \
  (((tvp)->tv_sec != (uvp)->tv_sec) ? \
  ((tvp)->tv_sec cmp (uvp)->tv_sec) : \
  ((tvp)->tv_usec cmp (uvp)->tv_usec))

#define timerclear(tvp) (tvp)->tv_sec = (tvp)->tv_usec = 0

#endif /* _TIMEVAL_DEFINED */

#define h_addr h_addr_list[0]

struct hostent {
  char *h_name;
  char **h_aliases;
  short h_addrtype;
  short h_length;
  char **h_addr_list;
};

struct linger {
  u_short l_onoff;
  u_short l_linger;
};

#define FIONBIO _IOW('f', 126, u_long)

struct netent {
  char * n_name;
  char **n_aliases;
  short n_addrtype;
  u_long n_net;
};

struct servent {
  char FAR *s_name;
  char FAR **s_aliases;
#ifdef _WIN64
  char FAR *s_proto;
  short s_port;
#else
  short s_port;
  char FAR *s_proto;
#endif
};

struct protoent {
  char *p_name;
  char **p_aliases;
  short p_proto;
};

#define SOMAXCONN 0x7fffffff

#define MSG_OOB 1
#define MSG_PEEK 2
#define MSG_DONTROUTE 4
#if(_WIN32_WINNT >= 0x0502)
#define MSG_WAITALL 8
#endif

#define h_errno WSAGetLastError()
#define HOST_NOT_FOUND WSAHOST_NOT_FOUND
#define TRY_AGAIN WSATRY_AGAIN
#define NO_RECOVERY WSANO_RECOVERY
#define NO_DATA WSANO_DATA
#define NO_ADDRESS WSANO_ADDRESS

#endif /* !(defined (__INSIDE_CYGWIN__) || defined (__INSIDE_MSYS__)) */

#define FIONREAD _IOR('f', 127, u_long)
#define FIOASYNC _IOW('f', 125, u_long)
#define SIOCSHIWAT _IOW('s',  0, u_long)
#define SIOCGHIWAT _IOR('s',  1, u_long)
#define SIOCSLOWAT _IOW('s',  2, u_long)
#define SIOCGLOWAT _IOR('s',  3, u_long)
#define SIOCATMARK _IOR('s',  7, u_long)

#define IMPLINK_IP 155
#define IMPLINK_LOWEXPER 156
#define IMPLINK_HIGHEXPER 158

#define ADDR_ANY INADDR_ANY

#define WSADESCRIPTION_LEN 256
#define WSASYS_STATUS_LEN 128

#define INVALID_SOCKET (SOCKET)(~0)

#define SOCKET_ERROR (-1)

#define FROM_PROTOCOL_INFO (-1)

#define SO_PROTOCOL_INFOA 0x2004
#define SO_PROTOCOL_INFOW 0x2005
#ifdef UNICODE
#define SO_PROTOCOL_INFO SO_PROTOCOL_INFOW
#else
#define SO_PROTOCOL_INFO SO_PROTOCOL_INFOA
#endif
#define PVD_CONFIG 0x3001

#define PF_UNSPEC AF_UNSPEC
#define PF_UNIX AF_UNIX
#define PF_INET AF_INET
#define PF_IMPLINK AF_IMPLINK
#define PF_PUP AF_PUP
#define PF_CHAOS AF_CHAOS
#define PF_NS AF_NS
#define PF_IPX AF_IPX
#define PF_ISO AF_ISO
#define PF_OSI AF_OSI
#define PF_ECMA AF_ECMA
#define PF_DATAKIT AF_DATAKIT
#define PF_CCITT AF_CCITT
#define PF_SNA AF_SNA
#define PF_DECnet AF_DECnet
#define PF_DLI AF_DLI
#define PF_LAT AF_LAT
#define PF_HYLINK AF_HYLINK
#define PF_APPLETALK AF_APPLETALK
#define PF_VOICEVIEW AF_VOICEVIEW
#define PF_FIREFOX AF_FIREFOX
#define PF_UNKNOWN1 AF_UNKNOWN1
#define PF_BAN AF_BAN
#define PF_ATM AF_ATM
#define PF_INET6 AF_INET6
#if(_WIN32_WINNT >= 0x0600)
#define PF_BTH AF_BTH
#endif
#define PF_MAX AF_MAX

#define MSG_PARTIAL 0x8000
#define MSG_INTERRUPT 0x10
#define MSG_MAXIOVLEN 16

#define MAXGETHOSTSTRUCT 1024

#define FD_READ_BIT 0
#define FD_READ (1 << FD_READ_BIT)
#define FD_WRITE_BIT 1
#define FD_WRITE (1 << FD_WRITE_BIT)
#define FD_OOB_BIT 2
#define FD_OOB (1 << FD_OOB_BIT)
#define FD_ACCEPT_BIT 3
#define FD_ACCEPT (1 << FD_ACCEPT_BIT)
#define FD_CONNECT_BIT 4
#define FD_CONNECT (1 << FD_CONNECT_BIT)
#define FD_CLOSE_BIT 5
#define FD_CLOSE (1 << FD_CLOSE_BIT)
#define FD_QOS_BIT 6
#define FD_QOS (1 << FD_QOS_BIT)
#define FD_GROUP_QOS_BIT 7
#define FD_GROUP_QOS (1 << FD_GROUP_QOS_BIT)
#define FD_ROUTING_INTERFACE_CHANGE_BIT 8
#define FD_ROUTING_INTERFACE_CHANGE (1 << FD_ROUTING_INTERFACE_CHANGE_BIT)
#define FD_ADDRESS_LIST_CHANGE_BIT 9
#define FD_ADDRESS_LIST_CHANGE (1 << FD_ADDRESS_LIST_CHANGE_BIT)
#define FD_MAX_EVENTS 10
#define FD_ALL_EVENTS ((1 << FD_MAX_EVENTS) - 1)

#ifndef WSABASEERR

#define WSABASEERR 10000
#define WSAEINTR (WSABASEERR+4)
#define WSAEBADF (WSABASEERR+9)
#define WSAEACCES (WSABASEERR+13)
#define WSAEFAULT (WSABASEERR+14)
#define WSAEINVAL (WSABASEERR+22)
#define WSAEMFILE (WSABASEERR+24)
#define WSAEWOULDBLOCK (WSABASEERR+35)
#define WSAEINPROGRESS (WSABASEERR+36)
#define WSAEALREADY (WSABASEERR+37)
#define WSAENOTSOCK (WSABASEERR+38)
#define WSAEDESTADDRREQ (WSABASEERR+39)
#define WSAEMSGSIZE (WSABASEERR+40)
#define WSAEPROTOTYPE (WSABASEERR+41)
#define WSAENOPROTOOPT (WSABASEERR+42)
#define WSAEPROTONOSUPPORT (WSABASEERR+43)
#define WSAESOCKTNOSUPPORT (WSABASEERR+44)
#define WSAEOPNOTSUPP (WSABASEERR+45)
#define WSAEPFNOSUPPORT (WSABASEERR+46)
#define WSAEAFNOSUPPORT (WSABASEERR+47)
#define WSAEADDRINUSE (WSABASEERR+48)
#define WSAEADDRNOTAVAIL (WSABASEERR+49)
#define WSAENETDOWN (WSABASEERR+50)
#define WSAENETUNREACH (WSABASEERR+51)
#define WSAENETRESET (WSABASEERR+52)
#define WSAECONNABORTED (WSABASEERR+53)
#define WSAECONNRESET (WSABASEERR+54)
#define WSAENOBUFS (WSABASEERR+55)
#define WSAEISCONN (WSABASEERR+56)
#define WSAENOTCONN (WSABASEERR+57)
#define WSAESHUTDOWN (WSABASEERR+58)
#define WSAETOOMANYREFS (WSABASEERR+59)
#define WSAETIMEDOUT (WSABASEERR+60)
#define WSAECONNREFUSED (WSABASEERR+61)
#define WSAELOOP (WSABASEERR+62)
#define WSAENAMETOOLONG (WSABASEERR+63)
#define WSAEHOSTDOWN (WSABASEERR+64)
#define WSAEHOSTUNREACH (WSABASEERR+65)
#define WSAENOTEMPTY (WSABASEERR+66)
#define WSAEPROCLIM (WSABASEERR+67)
#define WSAEUSERS (WSABASEERR+68)
#define WSAEDQUOT (WSABASEERR+69)
#define WSAESTALE (WSABASEERR+70)
#define WSAEREMOTE (WSABASEERR+71)
#define WSASYSNOTREADY (WSABASEERR+91)
#define WSAVERNOTSUPPORTED (WSABASEERR+92)
#define WSANOTINITIALISED (WSABASEERR+93)
#define WSAEDISCON (WSABASEERR+101)
#define WSAENOMORE (WSABASEERR+102)
#define WSAECANCELLED (WSABASEERR+103)
#define WSAEINVALIDPROCTABLE (WSABASEERR+104)
#define WSAEINVALIDPROVIDER (WSABASEERR+105)
#define WSAEPROVIDERFAILEDINIT (WSABASEERR+106)
#define WSASYSCALLFAILURE (WSABASEERR+107)
#define WSASERVICE_NOT_FOUND (WSABASEERR+108)
#define WSATYPE_NOT_FOUND (WSABASEERR+109)
#define WSA_E_NO_MORE (WSABASEERR+110)
#define WSA_E_CANCELLED (WSABASEERR+111)
#define WSAEREFUSED (WSABASEERR+112)
#define WSAHOST_NOT_FOUND (WSABASEERR+1001)
#define WSATRY_AGAIN (WSABASEERR+1002)
#define WSANO_RECOVERY (WSABASEERR+1003)
#define WSANO_DATA (WSABASEERR+1004)
#define WSA_QOS_RECEIVERS (WSABASEERR + 1005)
#define WSA_QOS_SENDERS (WSABASEERR + 1006)
#define WSA_QOS_NO_SENDERS (WSABASEERR + 1007)
#define WSA_QOS_NO_RECEIVERS (WSABASEERR + 1008)
#define WSA_QOS_REQUEST_CONFIRMED (WSABASEERR + 1009)
#define WSA_QOS_ADMISSION_FAILURE (WSABASEERR + 1010)
#define WSA_QOS_POLICY_FAILURE (WSABASEERR + 1011)
#define WSA_QOS_BAD_STYLE (WSABASEERR + 1012)
#define WSA_QOS_BAD_OBJECT (WSABASEERR + 1013)
#define WSA_QOS_TRAFFIC_CTRL_ERROR (WSABASEERR + 1014)
#define WSA_QOS_GENERIC_ERROR (WSABASEERR + 1015)
#define WSA_QOS_ESERVICETYPE (WSABASEERR + 1016)
#define WSA_QOS_EFLOWSPEC (WSABASEERR + 1017)
#define WSA_QOS_EPROVSPECBUF (WSABASEERR + 1018)
#define WSA_QOS_EFILTERSTYLE (WSABASEERR + 1019)
#define WSA_QOS_EFILTERTYPE (WSABASEERR + 1020)
#define WSA_QOS_EFILTERCOUNT (WSABASEERR + 1021)
#define WSA_QOS_EOBJLENGTH (WSABASEERR + 1022)
#define WSA_QOS_EFLOWCOUNT (WSABASEERR + 1023)
#define WSA_QOS_EUNKOWNPSOBJ (WSABASEERR + 1024)
#define WSA_QOS_EPOLICYOBJ (WSABASEERR + 1025)
#define WSA_QOS_EFLOWDESC (WSABASEERR + 1026)
#define WSA_QOS_EPSFLOWSPEC (WSABASEERR + 1027)
#define WSA_QOS_EPSFILTERSPEC (WSABASEERR + 1028)
#define WSA_QOS_ESDMODEOBJ (WSABASEERR + 1029)
#define WSA_QOS_ESHAPERATEOBJ (WSABASEERR + 1030)
#define WSA_QOS_RESERVED_PETYPE (WSABASEERR + 1031)

#endif /* !WSABASEERR */

#define WSANO_ADDRESS WSANO_DATA

#define CF_ACCEPT 0x0000
#define CF_REJECT 0x0001
#define CF_DEFER 0x0002
#define SD_RECEIVE 0x00
#define SD_SEND 0x01
#define SD_BOTH 0x02

#define SG_UNCONSTRAINED_GROUP 0x01
#define SG_CONSTRAINED_GROUP 0x02

#define MAX_PROTOCOL_CHAIN 7

#define BASE_PROTOCOL      1
#define LAYERED_PROTOCOL   0

#define WSAPROTOCOL_LEN 255

#define PFL_MULTIPLE_PROTO_ENTRIES          0x00000001
#define PFL_RECOMMENDED_PROTO_ENTRY         0x00000002
#define PFL_HIDDEN                          0x00000004
#define PFL_MATCHES_PROTOCOL_ZERO           0x00000008
#define PFL_NETWORKDIRECT_PROVIDER          0x00000010

#define XP1_CONNECTIONLESS                  0x00000001
#define XP1_GUARANTEED_DELIVERY             0x00000002
#define XP1_GUARANTEED_ORDER                0x00000004
#define XP1_MESSAGE_ORIENTED                0x00000008
#define XP1_PSEUDO_STREAM                   0x00000010
#define XP1_GRACEFUL_CLOSE                  0x00000020
#define XP1_EXPEDITED_DATA                  0x00000040
#define XP1_CONNECT_DATA                    0x00000080
#define XP1_DISCONNECT_DATA                 0x00000100
#define XP1_SUPPORT_BROADCAST               0x00000200
#define XP1_SUPPORT_MULTIPOINT              0x00000400
#define XP1_MULTIPOINT_CONTROL_PLANE        0x00000800
#define XP1_MULTIPOINT_DATA_PLANE           0x00001000
#define XP1_QOS_SUPPORTED                   0x00002000
#define XP1_INTERRUPT                       0x00004000
#define XP1_UNI_SEND                        0x00008000
#define XP1_UNI_RECV                        0x00010000
#define XP1_IFS_HANDLES                     0x00020000
#define XP1_PARTIAL_MESSAGE                 0x00040000
#define XP1_SAN_SUPPORT_SDP                 0x00080000

#define BIGENDIAN                           0x0000
#define LITTLEENDIAN                        0x0001

#define SECURITY_PROTOCOL_NONE              0x0000

#define JL_SENDER_ONLY                      0x01
#define JL_RECEIVER_ONLY                    0x02
#define JL_BOTH                             0x04

#define WSA_FLAG_OVERLAPPED                 0x01
#define WSA_FLAG_MULTIPOINT_C_ROOT          0x02
#define WSA_FLAG_MULTIPOINT_C_LEAF          0x04
#define WSA_FLAG_MULTIPOINT_D_ROOT          0x08
#define WSA_FLAG_MULTIPOINT_D_LEAF          0x10
#define WSA_FLAG_ACCESS_SYSTEM_SECURITY     0x40

#define TH_NETDEV                           0x00000001
#define TH_TAPI                             0x00000002

#define SERVICE_MULTIPLE                    0x00000001

#define RES_UNUSED_1    0x00000001
#define RES_FLUSH_CACHE 0x00000002
#ifndef RES_SERVICE
#define RES_SERVICE     0x00000004
#endif

#define SERVICE_TYPE_VALUE_IPXPORTA      "IpxSocket"
#define SERVICE_TYPE_VALUE_IPXPORTW     L"IpxSocket"
#define SERVICE_TYPE_VALUE_SAPIDA        "SapId"
#define SERVICE_TYPE_VALUE_SAPIDW       L"SapId"

#define SERVICE_TYPE_VALUE_TCPPORTA      "TcpPort"
#define SERVICE_TYPE_VALUE_TCPPORTW     L"TcpPort"

#define SERVICE_TYPE_VALUE_UDPPORTA      "UdpPort"
#define SERVICE_TYPE_VALUE_UDPPORTW     L"UdpPort"

#define SERVICE_TYPE_VALUE_OBJECTIDA     "ObjectId"
#define SERVICE_TYPE_VALUE_OBJECTIDW    L"ObjectId"

#ifdef UNICODE
#define SERVICE_TYPE_VALUE_SAPID        SERVICE_TYPE_VALUE_SAPIDW
#define SERVICE_TYPE_VALUE_TCPPORT      SERVICE_TYPE_VALUE_TCPPORTW
#define SERVICE_TYPE_VALUE_UDPPORT      SERVICE_TYPE_VALUE_UDPPORTW
#define SERVICE_TYPE_VALUE_OBJECTID     SERVICE_TYPE_VALUE_OBJECTIDW
#else
#define SERVICE_TYPE_VALUE_SAPID        SERVICE_TYPE_VALUE_SAPIDA
#define SERVICE_TYPE_VALUE_TCPPORT      SERVICE_TYPE_VALUE_TCPPORTA
#define SERVICE_TYPE_VALUE_UDPPORT      SERVICE_TYPE_VALUE_UDPPORTA
#define SERVICE_TYPE_VALUE_OBJECTID     SERVICE_TYPE_VALUE_OBJECTIDA
#endif

#define LUP_DEEP                0x0001
#define LUP_CONTAINERS          0x0002
#define LUP_NOCONTAINERS        0x0004
#define LUP_NEAREST             0x0008
#define LUP_RETURN_NAME         0x0010
#define LUP_RETURN_TYPE         0x0020
#define LUP_RETURN_VERSION      0x0040
#define LUP_RETURN_COMMENT      0x0080
#define LUP_RETURN_ADDR         0x0100
#define LUP_RETURN_BLOB         0x0200
#define LUP_RETURN_ALIASES      0x0400
#define LUP_RETURN_QUERY_STRING 0x0800
#define LUP_RETURN_ALL          0x0FF0
#define LUP_RES_SERVICE         0x8000
#define LUP_FLUSHCACHE          0x1000
#define LUP_FLUSHPREVIOUS       0x2000
#define LUP_NON_AUTHORITATIVE   0x4000
#define LUP_SECURE              0x8000
#define LUP_RETURN_PREFERRED_NAMES 0x10000
#define LUP_ADDRCONFIG          0x00100000
#define LUP_DUAL_ADDR           0x00200000
#define LUP_FILESERVER          0x00400000

#define RESULT_IS_ALIAS      0x0001
#if(_WIN32_WINNT >= 0x0501)
#define RESULT_IS_ADDED      0x0010
#define RESULT_IS_CHANGED    0x0020
#define RESULT_IS_DELETED    0x0040
#endif

#ifndef s_addr

#define s_addr S_un.S_addr
#define s_host S_un.S_un_b.s_b2
#define s_net S_un.S_un_b.s_b1
#define s_imp S_un.S_un_w.s_w2
#define s_impno S_un.S_un_b.s_b4
#define s_lh S_un.S_un_b.s_b3

typedef struct in_addr {
  union {
    struct { u_char s_b1,s_b2,s_b3,s_b4; } S_un_b;
    struct { u_short s_w1,s_w2; } S_un_w;
    u_long S_addr;
  } S_un;
} IN_ADDR, *PIN_ADDR;

#endif /* s_addr */

typedef struct WSAData {
  WORD wVersion;
  WORD wHighVersion;
#ifdef _WIN64
  unsigned short iMaxSockets;
  unsigned short iMaxUdpDg;
  char FAR *lpVendorInfo;
  char szDescription[WSADESCRIPTION_LEN+1];
  char szSystemStatus[WSASYS_STATUS_LEN+1];
#else
  char szDescription[WSADESCRIPTION_LEN+1];
  char szSystemStatus[WSASYS_STATUS_LEN+1];
  unsigned short iMaxSockets;
  unsigned short iMaxUdpDg;
  char FAR *lpVendorInfo;
#endif
} WSADATA, FAR *LPWSADATA;

struct sockproto {
  u_short sp_family;
  u_short sp_protocol;
};

#ifdef WIN32

#define WSAAPI FAR PASCAL
#define WSAEVENT HANDLE
#define LPWSAEVENT LPHANDLE
#define WSAOVERLAPPED OVERLAPPED
typedef struct _OVERLAPPED *LPWSAOVERLAPPED;
#define WSA_IO_PENDING (ERROR_IO_PENDING)
#define WSA_IO_INCOMPLETE (ERROR_IO_INCOMPLETE)
#define WSA_INVALID_HANDLE (ERROR_INVALID_HANDLE)
#define WSA_INVALID_PARAMETER (ERROR_INVALID_PARAMETER)
#define WSA_NOT_ENOUGH_MEMORY (ERROR_NOT_ENOUGH_MEMORY)
#define WSA_OPERATION_ABORTED (ERROR_OPERATION_ABORTED)
#define WSA_INVALID_EVENT ((WSAEVENT)NULL)
#define WSA_MAXIMUM_WAIT_EVENTS (MAXIMUM_WAIT_OBJECTS)
#define WSA_WAIT_FAILED ((DWORD)-1L)
#define WSA_WAIT_EVENT_0 (WAIT_OBJECT_0)
#define WSA_WAIT_IO_COMPLETION (WAIT_IO_COMPLETION)
#define WSA_WAIT_TIMEOUT (WAIT_TIMEOUT)
#define WSA_INFINITE (INFINITE)

#else /* WIN16 */

#define WSAAPI FAR PASCAL
typedef DWORD WSAEVENT, FAR * LPWSAEVENT;

typedef struct _WSAOVERLAPPED {
  DWORD Internal;
  DWORD InternalHigh;
  DWORD Offset;
  DWORD OffsetHigh;
  WSAEVENT hEvent;
} WSAOVERLAPPED, FAR * LPWSAOVERLAPPED;

#define WSA_IO_PENDING (WSAEWOULDBLOCK)
#define WSA_IO_INCOMPLETE (WSAEWOULDBLOCK)
#define WSA_INVALID_HANDLE (WSAENOTSOCK)
#define WSA_INVALID_PARAMETER (WSAEINVAL)
#define WSA_NOT_ENOUGH_MEMORY (WSAENOBUFS)
#define WSA_OPERATION_ABORTED (WSAEINTR)

#define WSA_INVALID_EVENT ((WSAEVENT)NULL)
#define WSA_MAXIMUM_WAIT_EVENTS (MAXIMUM_WAIT_OBJECTS)
#define WSA_WAIT_FAILED ((DWORD)-1L)
#define WSA_WAIT_EVENT_0 ((DWORD)0)
#define WSA_WAIT_TIMEOUT ((DWORD)0x102L)
#define WSA_INFINITE ((DWORD)-1L)

#endif /* WIN32 */

#include <qos.h>

typedef struct _QualityOfService {
  FLOWSPEC SendingFlowspec;
  FLOWSPEC ReceivingFlowspec;
  WSABUF ProviderSpecific;
} QOS, *LPQOS;

typedef unsigned int GROUP;

typedef struct _WSANETWORKEVENTS {
  LONG lNetworkEvents;
  int iErrorCode[FD_MAX_EVENTS];
} WSANETWORKEVENTS, *LPWSANETWORKEVENTS;

#ifndef GUID_DEFINED
#include <guiddef.h>
#endif

typedef struct _WSAPROTOCOLCHAIN {
  int ChainLen;
  DWORD ChainEntries[MAX_PROTOCOL_CHAIN];
} WSAPROTOCOLCHAIN, *LPWSAPROTOCOLCHAIN;

typedef struct _WSAPROTOCOL_INFOA {
  DWORD dwServiceFlags1;
  DWORD dwServiceFlags2;
  DWORD dwServiceFlags3;
  DWORD dwServiceFlags4;
  DWORD dwProviderFlags;
  GUID ProviderId;
  DWORD dwCatalogEntryId;
  WSAPROTOCOLCHAIN ProtocolChain;
  int iVersion;
  int iAddressFamily;
  int iMaxSockAddr;
  int iMinSockAddr;
  int iSocketType;
  int iProtocol;
  int iProtocolMaxOffset;
  int iNetworkByteOrder;
  int iSecurityScheme;
  DWORD dwMessageSize;
  DWORD dwProviderReserved;
  CHAR szProtocol[WSAPROTOCOL_LEN+1];
} WSAPROTOCOL_INFOA, *LPWSAPROTOCOL_INFOA;

typedef struct _WSAPROTOCOL_INFOW {
  DWORD dwServiceFlags1;
  DWORD dwServiceFlags2;
  DWORD dwServiceFlags3;
  DWORD dwServiceFlags4;
  DWORD dwProviderFlags;
  GUID ProviderId;
  DWORD dwCatalogEntryId;
  WSAPROTOCOLCHAIN ProtocolChain;
  int iVersion;
  int iAddressFamily;
  int iMaxSockAddr;
  int iMinSockAddr;
  int iSocketType;
  int iProtocol;
  int iProtocolMaxOffset;
  int iNetworkByteOrder;
  int iSecurityScheme;
  DWORD dwMessageSize;
  DWORD dwProviderReserved;
  WCHAR szProtocol[WSAPROTOCOL_LEN+1];
} WSAPROTOCOL_INFOW, * LPWSAPROTOCOL_INFOW;

#ifdef UNICODE
typedef WSAPROTOCOL_INFOW WSAPROTOCOL_INFO;
typedef LPWSAPROTOCOL_INFOW LPWSAPROTOCOL_INFO;
#else
typedef WSAPROTOCOL_INFOA WSAPROTOCOL_INFO;
typedef LPWSAPROTOCOL_INFOA LPWSAPROTOCOL_INFO;
#endif

typedef int
(CALLBACK *LPCONDITIONPROC)(
  IN LPWSABUF lpCallerId,
  IN LPWSABUF lpCallerData,
  IN OUT LPQOS lpSQOS,
  IN OUT LPQOS lpGQOS,
  IN LPWSABUF lpCalleeId,
  IN LPWSABUF lpCalleeData,
  OUT GROUP FAR *g,
  IN DWORD_PTR dwCallbackData);

typedef void
(CALLBACK *LPWSAOVERLAPPED_COMPLETION_ROUTINE)(
  IN DWORD dwError,
  IN DWORD cbTransferred,
  IN LPWSAOVERLAPPED lpOverlapped,
  IN DWORD dwFlags);

#if(_WIN32_WINNT >= 0x0501)

#define SIO_NSP_NOTIFY_CHANGE _WSAIOW(IOC_WS2,25)

typedef enum _WSACOMPLETIONTYPE {
  NSP_NOTIFY_IMMEDIATELY = 0,
  NSP_NOTIFY_HWND,
  NSP_NOTIFY_EVENT,
  NSP_NOTIFY_PORT,
  NSP_NOTIFY_APC
} WSACOMPLETIONTYPE, * PWSACOMPLETIONTYPE, *LPWSACOMPLETIONTYPE;

typedef struct _WSACOMPLETION {
  WSACOMPLETIONTYPE Type;
  union {
    struct {
      HWND hWnd;
      UINT uMsg;
      WPARAM context;
    } WindowMessage;
    struct {
      LPWSAOVERLAPPED lpOverlapped;
    } Event;
    struct {
      LPWSAOVERLAPPED lpOverlapped;
      LPWSAOVERLAPPED_COMPLETION_ROUTINE lpfnCompletionProc;
    } Apc;
    struct {
      LPWSAOVERLAPPED lpOverlapped;
      HANDLE hPort;
      ULONG_PTR Key;
    } Port;
  } Parameters;
} WSACOMPLETION, *PWSACOMPLETION, *LPWSACOMPLETION;

#endif /* (_WIN32_WINNT >= 0x0501) */

#ifndef __BLOB_T_DEFINED /* also in wtypes.h and nspapi.h */
#define __BLOB_T_DEFINED
/* wine is using a diff define */
#ifndef _tagBLOB_DEFINED
#define _tagBLOB_DEFINED
#define _BLOB_DEFINED
#define _LPBLOB_DEFINED

typedef struct _BLOB {
  ULONG cbSize;
  BYTE *pBlobData;
} BLOB,*PBLOB,*LPBLOB;

#endif /* _tagBLOB_DEFINED */

#endif /* __BLOB_T_DEFINED */

typedef struct _AFPROTOCOLS {
  INT iAddressFamily;
  INT iProtocol;
} AFPROTOCOLS, *PAFPROTOCOLS, *LPAFPROTOCOLS;

typedef enum _WSAEcomparator {
  COMP_EQUAL = 0,
  COMP_NOTLESS
} WSAECOMPARATOR, *PWSAECOMPARATOR, *LPWSAECOMPARATOR;

typedef struct _WSAVersion {
  DWORD dwVersion;
  WSAECOMPARATOR ecHow;
} WSAVERSION, *PWSAVERSION, *LPWSAVERSION;

typedef struct _WSAQuerySetA {
  DWORD dwSize;
  LPSTR lpszServiceInstanceName;
  LPGUID lpServiceClassId;
  LPWSAVERSION lpVersion;
  LPSTR lpszComment;
  DWORD dwNameSpace;
  LPGUID lpNSProviderId;
  LPSTR lpszContext;
  DWORD dwNumberOfProtocols;
  LPAFPROTOCOLS lpafpProtocols;
  LPSTR lpszQueryString;
  DWORD dwNumberOfCsAddrs;
  LPCSADDR_INFO lpcsaBuffer;
  DWORD dwOutputFlags;
  LPBLOB lpBlob;
} WSAQUERYSETA, *PWSAQUERYSETA, *LPWSAQUERYSETA;

typedef struct _WSAQuerySetW {
  DWORD dwSize;
  LPWSTR lpszServiceInstanceName;
  LPGUID lpServiceClassId;
  LPWSAVERSION lpVersion;
  LPWSTR lpszComment;
  DWORD dwNameSpace;
  LPGUID lpNSProviderId;
  LPWSTR lpszContext;
  DWORD dwNumberOfProtocols;
  LPAFPROTOCOLS lpafpProtocols;
  LPWSTR lpszQueryString;
  DWORD dwNumberOfCsAddrs;
  LPCSADDR_INFO lpcsaBuffer;
  DWORD dwOutputFlags;
  LPBLOB lpBlob;
} WSAQUERYSETW, *PWSAQUERYSETW, *LPWSAQUERYSETW;

typedef struct _WSAQuerySet2A {
  DWORD dwSize;
  LPSTR lpszServiceInstanceName;
  LPWSAVERSION lpVersion;
  LPSTR lpszComment;
  DWORD dwNameSpace;
  LPGUID lpNSProviderId;
  LPSTR lpszContext;
  DWORD dwNumberOfProtocols;
  LPAFPROTOCOLS lpafpProtocols;
  LPSTR lpszQueryString;
  DWORD dwNumberOfCsAddrs;
  LPCSADDR_INFO lpcsaBuffer;
  DWORD dwOutputFlags;
  LPBLOB lpBlob;
} WSAQUERYSET2A, *PWSAQUERYSET2A, *LPWSAQUERYSET2A;

typedef struct _WSAQuerySet2W {
  DWORD dwSize;
  LPWSTR lpszServiceInstanceName;
  LPWSAVERSION lpVersion;
  LPWSTR lpszComment;
  DWORD dwNameSpace;
  LPGUID lpNSProviderId;
  LPWSTR lpszContext;
  DWORD dwNumberOfProtocols;
  LPAFPROTOCOLS lpafpProtocols;
  LPWSTR lpszQueryString;
  DWORD dwNumberOfCsAddrs;
  LPCSADDR_INFO lpcsaBuffer;
  DWORD dwOutputFlags;
  LPBLOB lpBlob;
} WSAQUERYSET2W, *PWSAQUERYSET2W, *LPWSAQUERYSET2W;

#ifdef UNICODE
typedef WSAQUERYSETW WSAQUERYSET;
typedef PWSAQUERYSETW PWSAQUERYSET;
typedef LPWSAQUERYSETW LPWSAQUERYSET;
typedef WSAQUERYSET2W WSAQUERYSET2;
typedef PWSAQUERYSET2W PWSAQUERYSET2;
typedef LPWSAQUERYSET2W LPWSAQUERYSET2;
#else
typedef WSAQUERYSETA WSAQUERYSET;
typedef PWSAQUERYSETA PWSAQUERYSET;
typedef LPWSAQUERYSETA LPWSAQUERYSET;
typedef WSAQUERYSET2A WSAQUERYSET2;
typedef PWSAQUERYSET2A PWSAQUERYSET2;
typedef LPWSAQUERYSET2A LPWSAQUERYSET2;
#endif /* UNICODE */

typedef enum _WSAESETSERVICEOP {
  RNRSERVICE_REGISTER=0,
  RNRSERVICE_DEREGISTER,
  RNRSERVICE_DELETE
} WSAESETSERVICEOP, *PWSAESETSERVICEOP, *LPWSAESETSERVICEOP;

typedef struct _WSANSClassInfoA {
  LPSTR lpszName;
  DWORD dwNameSpace;
  DWORD dwValueType;
  DWORD dwValueSize;
  LPVOID lpValue;
} WSANSCLASSINFOA, *PWSANSCLASSINFOA, *LPWSANSCLASSINFOA;

typedef struct _WSANSClassInfoW {
  LPWSTR lpszName;
  DWORD dwNameSpace;
  DWORD dwValueType;
  DWORD dwValueSize;
  LPVOID lpValue;
} WSANSCLASSINFOW, *PWSANSCLASSINFOW, *LPWSANSCLASSINFOW;

#ifdef UNICODE
typedef WSANSCLASSINFOW WSANSCLASSINFO;
typedef PWSANSCLASSINFOW PWSANSCLASSINFO;
typedef LPWSANSCLASSINFOW LPWSANSCLASSINFO;
#else
typedef WSANSCLASSINFOA WSANSCLASSINFO;
typedef PWSANSCLASSINFOA PWSANSCLASSINFO;
typedef LPWSANSCLASSINFOA LPWSANSCLASSINFO;
#endif

typedef struct _WSAServiceClassInfoA {
  LPGUID lpServiceClassId;
  LPSTR lpszServiceClassName;
  DWORD dwCount;
  LPWSANSCLASSINFOA lpClassInfos;
} WSASERVICECLASSINFOA, *PWSASERVICECLASSINFOA, *LPWSASERVICECLASSINFOA;

typedef struct _WSAServiceClassInfoW {
  LPGUID lpServiceClassId;
  LPWSTR lpszServiceClassName;
  DWORD dwCount;
  LPWSANSCLASSINFOW lpClassInfos;
} WSASERVICECLASSINFOW, *PWSASERVICECLASSINFOW, *LPWSASERVICECLASSINFOW;

#ifdef UNICODE
typedef WSASERVICECLASSINFOW WSASERVICECLASSINFO;
typedef PWSASERVICECLASSINFOW PWSASERVICECLASSINFO;
typedef LPWSASERVICECLASSINFOW LPWSASERVICECLASSINFO;
#else
typedef WSASERVICECLASSINFOA WSASERVICECLASSINFO;
typedef PWSASERVICECLASSINFOA PWSASERVICECLASSINFO;
typedef LPWSASERVICECLASSINFOA LPWSASERVICECLASSINFO;
#endif

typedef struct _WSANAMESPACE_INFOA {
  GUID NSProviderId;
  DWORD dwNameSpace;
  BOOL fActive;
  DWORD dwVersion;
  LPSTR lpszIdentifier;
} WSANAMESPACE_INFOA, *PWSANAMESPACE_INFOA, *LPWSANAMESPACE_INFOA;

typedef struct _WSANAMESPACE_INFOW {
  GUID NSProviderId;
  DWORD dwNameSpace;
  BOOL fActive;
  DWORD dwVersion;
  LPWSTR lpszIdentifier;
} WSANAMESPACE_INFOW, *PWSANAMESPACE_INFOW, *LPWSANAMESPACE_INFOW;

typedef struct _WSANAMESPACE_INFOEXA {
  GUID NSProviderId;
  DWORD dwNameSpace;
  BOOL fActive;
  DWORD dwVersion;
  LPSTR lpszIdentifier;
  BLOB ProviderSpecific;
} WSANAMESPACE_INFOEXA, *PWSANAMESPACE_INFOEXA, *LPWSANAMESPACE_INFOEXA;

typedef struct _WSANAMESPACE_INFOEXW {
  GUID NSProviderId;
  DWORD dwNameSpace;
  BOOL fActive;
  DWORD dwVersion;
  LPWSTR lpszIdentifier;
  BLOB ProviderSpecific;
} WSANAMESPACE_INFOEXW, *PWSANAMESPACE_INFOEXW, *LPWSANAMESPACE_INFOEXW;

#ifdef UNICODE
typedef WSANAMESPACE_INFOW WSANAMESPACE_INFO;
typedef PWSANAMESPACE_INFOW PWSANAMESPACE_INFO;
typedef LPWSANAMESPACE_INFOW LPWSANAMESPACE_INFO;
typedef WSANAMESPACE_INFOEXW WSANAMESPACE_INFOEX;
typedef PWSANAMESPACE_INFOEXW PWSANAMESPACE_INFOEX;
typedef LPWSANAMESPACE_INFOEXW LPWSANAMESPACE_INFOEX;
#else
typedef WSANAMESPACE_INFOA WSANAMESPACE_INFO;
typedef PWSANAMESPACE_INFOA PWSANAMESPACE_INFO;
typedef LPWSANAMESPACE_INFOA LPWSANAMESPACE_INFO;
typedef WSANAMESPACE_INFOEXA WSANAMESPACE_INFOEX;
typedef PWSANAMESPACE_INFOEXA PWSANAMESPACE_INFOEX;
typedef LPWSANAMESPACE_INFOEXA LPWSANAMESPACE_INFOEX;
#endif /* UNICODE */

#if(_WIN32_WINNT >= 0x0600)

#define POLLRDNORM  0x0100
#define POLLRDBAND  0x0200
#define POLLIN      (POLLRDNORM | POLLRDBAND)
#define POLLPRI     0x0400

#define POLLWRNORM  0x0010
#define POLLOUT     (POLLWRNORM)
#define POLLWRBAND  0x0020

#define POLLERR     0x0001
#define POLLHUP     0x0002
#define POLLNVAL    0x0004

typedef struct pollfd {
  SOCKET fd;
  SHORT events;
  SHORT revents;
} WSAPOLLFD, *PWSAPOLLFD, FAR *LPWSAPOLLFD;

#endif /* (_WIN32_WINNT >= 0x0600) */

#if INCL_WINSOCK_API_TYPEDEFS

typedef SOCKET
(WSAAPI *LPFN_ACCEPT)(
  IN SOCKET s,
  OUT struct sockaddr FAR *addr OPTIONAL,
  IN OUT int FAR *addrlen OPTIONAL);

typedef int
(WSAAPI *LPFN_BIND)(
  IN SOCKET s,
  IN const struct sockaddr FAR *name,
  IN int namelen);

typedef int
(WSAAPI *LPFN_CLOSESOCKET)(
  IN SOCKET s);

typedef int
(WSAAPI *LPFN_CONNECT)(
  IN SOCKET s,
  IN const struct sockaddr FAR *name,
  IN int namelen);

typedef int
(WSAAPI *LPFN_IOCTLSOCKET)(
  IN SOCKET s,
  IN long cmd,
  IN OUT u_long FAR *argp);

typedef int
(WSAAPI *LPFN_GETPEERNAME)(
  IN SOCKET s,
  OUT struct sockaddr FAR *name,
  IN OUT int FAR *namelen);

typedef int
(WSAAPI *LPFN_GETSOCKNAME)(
  IN SOCKET s,
  OUT struct sockaddr FAR *name,
  IN OUT int FAR *namelen);

typedef int
(WSAAPI *LPFN_GETSOCKOPT)(
  IN SOCKET s,
  IN int level,
  IN int optname,
  OUT char FAR *optval,
  IN OUT int FAR *optlen);

typedef u_long
(WSAAPI *LPFN_HTONL)(
  IN u_long hostlong);

typedef u_short
(WSAAPI *LPFN_HTONS)(
  IN u_short hostshort);

typedef unsigned long
(WSAAPI *LPFN_INET_ADDR)(
  IN const char FAR *cp);

typedef char FAR *
(WSAAPI *LPFN_INET_NTOA)(
  IN struct in_addr in);

typedef int
(WSAAPI *LPFN_LISTEN)(
  IN SOCKET s,
  IN int backlog);

typedef u_long
(WSAAPI *LPFN_NTOHL)(
  IN u_long netlong);

typedef u_short
(WSAAPI *LPFN_NTOHS)(
  IN u_short netshort);

typedef int
(WSAAPI *LPFN_RECV)(
  IN SOCKET s,
  OUT char FAR *buf,
  IN int len,
  IN int flags);

typedef int
(WSAAPI *LPFN_RECVFROM)(
  IN SOCKET s,
  OUT char FAR *buf,
  IN int len,
  IN int flags,
  OUT struct sockaddr FAR *from OPTIONAL,
  IN OUT int FAR * fromlen OPTIONAL);

typedef int
(WSAAPI *LPFN_SELECT)(
  IN int nfds,
  IN OUT fd_set FAR *readfds OPTIONAL,
  IN OUT fd_set FAR *writefds OPTIONAL,
  IN OUT fd_set FAR *exceptfds OPTIONAL,
  IN const struct timeval FAR *timeout OPTIONAL);

typedef int
(WSAAPI *LPFN_SEND)(
  IN SOCKET s,
  IN const char FAR *buf,
  IN int len,
  IN int flags);

typedef int
(WSAAPI *LPFN_SENDTO)(
  IN SOCKET s,
  IN const char FAR *buf,
  IN int len,
  IN int flags,
  IN const struct sockaddr FAR *to OPTIONAL,
  IN int tolen);

typedef int
(WSAAPI *LPFN_SETSOCKOPT)(
  IN SOCKET s,
  IN int level,
  IN int optname,
  IN const char FAR *optval OPTIONAL,
  IN int optlen);

typedef int
(WSAAPI *LPFN_SHUTDOWN)(
  IN SOCKET s,
  IN int how);

typedef SOCKET
(WSAAPI *LPFN_SOCKET)(
  IN int af,
  IN int type,
  IN int protocol);

typedef struct hostent FAR *
(WSAAPI *LPFN_GETHOSTBYADDR)(
  IN const char FAR *addr,
  IN int len,
  IN int type);

typedef struct hostent FAR *
(WSAAPI *LPFN_GETHOSTBYNAME)(
  IN const char FAR *name);

typedef int
(WSAAPI *LPFN_GETHOSTNAME)(
  OUT char FAR *name,
  IN int namelen);

typedef struct servent FAR *
(WSAAPI *LPFN_GETSERVBYPORT)(
  IN int port,
  IN const char FAR *proto);

typedef struct servent FAR *
(WSAAPI *LPFN_GETSERVBYNAME)(
  IN const char FAR *name,
  IN const char FAR *proto);

typedef struct protoent FAR *
(WSAAPI *LPFN_GETPROTOBYNUMBER)(
  IN int number);

typedef struct protoent FAR *
(WSAAPI *LPFN_GETPROTOBYNAME)(
  IN const char FAR *name);

typedef int
(WSAAPI *LPFN_WSASTARTUP)(
  IN WORD wVersionRequired,
  OUT LPWSADATA lpWSAData);

typedef int
(WSAAPI *LPFN_WSACLEANUP)(void);

typedef void
(WSAAPI *LPFN_WSASETLASTERROR)(
  IN int iError);

typedef int
(WSAAPI *LPFN_WSAGETLASTERROR)(void);

typedef BOOL
(WSAAPI *LPFN_WSAISBLOCKING)(void);

typedef int
(WSAAPI *LPFN_WSAUNHOOKBLOCKINGHOOK)(void);

typedef FARPROC
(WSAAPI *LPFN_WSASETBLOCKINGHOOK)(
  IN FARPROC lpBlockFunc);

typedef int
(WSAAPI *LPFN_WSACANCELBLOCKINGCALL)(void);

typedef HANDLE
(WSAAPI *LPFN_WSAASYNCGETSERVBYNAME)(
  IN HWND hWnd,
  IN u_int wMsg,
  IN const char FAR *name,
  IN const char FAR *proto,
  OUT char FAR *buf,
  IN int buflen);

typedef HANDLE
(WSAAPI *LPFN_WSAASYNCGETSERVBYPORT)(
  IN HWND hWnd,
  IN u_int wMsg,
  IN int port,
  IN const char FAR *proto,
  OUT char FAR *buf,
  IN int buflen);

typedef HANDLE
(WSAAPI *LPFN_WSAASYNCGETPROTOBYNAME)(
  IN HWND hWnd,
  IN u_int wMsg,
  IN const char FAR *name,
  OUT char FAR *buf,
  IN int buflen);

typedef HANDLE
(WSAAPI *LPFN_WSAASYNCGETPROTOBYNUMBER)(
  IN HWND hWnd,
  IN u_int wMsg,
  IN int number,
  OUT char FAR *buf,
  IN int buflen);

typedef HANDLE
(WSAAPI *LPFN_WSAASYNCGETHOSTBYNAME)(
  IN HWND hWnd,
  IN u_int wMsg,
  IN const char FAR *name,
  OUT char FAR *buf,
  IN int buflen);

typedef HANDLE
(WSAAPI *LPFN_WSAASYNCGETHOSTBYADDR)(
  IN HWND hWnd,
  IN u_int wMsg,
  IN const char FAR *addr,
  IN int len,
  IN int type,
  OUT char FAR *buf,
  IN int buflen);

typedef int
(WSAAPI *LPFN_WSACANCELASYNCREQUEST)(
  IN HANDLE hAsyncTaskHandle);

typedef int
(WSAAPI *LPFN_WSAASYNCSELECT)(
  IN SOCKET s,
  IN HWND hWnd,
  IN u_int wMsg,
  IN long lEvent);

typedef SOCKET
(WSAAPI *LPFN_WSAACCEPT)(
  IN SOCKET s,
  OUT struct sockaddr FAR *addr OPTIONAL,
  IN OUT LPINT addrlen OPTIONAL,
  IN LPCONDITIONPROC lpfnCondition OPTIONAL,
  IN DWORD_PTR dwCallbackData OPTIONAL);

typedef BOOL
(WSAAPI *LPFN_WSACLOSEEVENT)(
  IN WSAEVENT hEvent);

typedef int
(WSAAPI *LPFN_WSACONNECT)(
  IN SOCKET s,
  IN const struct sockaddr FAR *name,
  IN int namelen,
  IN LPWSABUF lpCallerData OPTIONAL,
  OUT LPWSABUF lpCalleeData OPTIONAL,
  IN LPQOS lpSQOS OPTIONAL,
  IN LPQOS lpGQOS OPTIONAL);

typedef WSAEVENT
(WSAAPI *LPFN_WSACREATEEVENT)(void);

typedef int
(WSAAPI *LPFN_WSADUPLICATESOCKETA)(
  IN SOCKET s,
  IN DWORD dwProcessId,
  OUT LPWSAPROTOCOL_INFOA lpProtocolInfo);

typedef int
(WSAAPI *LPFN_WSADUPLICATESOCKETW)(
  IN SOCKET s,
  IN DWORD dwProcessId,
  OUT LPWSAPROTOCOL_INFOW lpProtocolInfo);

#ifdef UNICODE
#define LPFN_WSADUPLICATESOCKET LPFN_WSADUPLICATESOCKETW
#else
#define LPFN_WSADUPLICATESOCKET LPFN_WSADUPLICATESOCKETA
#endif

typedef int
(WSAAPI *LPFN_WSAENUMNETWORKEVENTS)(
  IN SOCKET s,
  IN WSAEVENT hEventObject,
  OUT LPWSANETWORKEVENTS lpNetworkEvents);

typedef int
(WSAAPI *LPFN_WSAENUMPROTOCOLSA)(
  IN LPINT lpiProtocols OPTIONAL,
  OUT LPWSAPROTOCOL_INFOA lpProtocolBuffer OPTIONAL,
  IN OUT LPDWORD lpdwBufferLength);

typedef int
(WSAAPI *LPFN_WSAENUMPROTOCOLSW)(
  IN LPINT lpiProtocols OPTIONAL,
  OUT LPWSAPROTOCOL_INFOW lpProtocolBuffer OPTIONAL,
  IN OUT LPDWORD lpdwBufferLength);

#ifdef UNICODE
#define LPFN_WSAENUMPROTOCOLS LPFN_WSAENUMPROTOCOLSW
#else
#define LPFN_WSAENUMPROTOCOLS LPFN_WSAENUMPROTOCOLSA
#endif

typedef int
(WSAAPI *LPFN_WSAEVENTSELECT)(
  IN SOCKET s,
  IN WSAEVENT hEventObject,
  IN long lNetworkEvents);

typedef BOOL
(WSAAPI *LPFN_WSAGETOVERLAPPEDRESULT)(
  IN SOCKET s,
  IN LPWSAOVERLAPPED lpOverlapped,
  OUT LPDWORD lpcbTransfer,
  IN BOOL fWait,
  OUT LPDWORD lpdwFlags);

typedef BOOL
(WSAAPI *LPFN_WSAGETQOSBYNAME)(
  IN SOCKET s,
  IN LPWSABUF lpQOSName,
  OUT LPQOS lpQOS);

typedef int
(WSAAPI *LPFN_WSAHTONL)(
  IN SOCKET s,
  IN u_long hostlong,
  OUT u_long FAR *lpnetlong);

typedef int
(WSAAPI *LPFN_WSAHTONS)(
  IN SOCKET s,
  IN u_short hostshort,
  OUT u_short FAR *lpnetshort);

typedef int
(WSAAPI *LPFN_WSAIOCTL)(
  IN SOCKET s,
  IN DWORD dwIoControlCode,
  IN LPVOID lpvInBuffer OPTIONAL,
  IN DWORD cbInBuffer,
  OUT LPVOID lpvOutBuffer OPTIONAL,
  IN DWORD cbOutBuffer,
  OUT LPDWORD lpcbBytesReturned,
  IN OUT LPWSAOVERLAPPED lpOverlapped OPTIONAL,
  IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL);

typedef SOCKET
(WSAAPI *LPFN_WSAJOINLEAF)(
  IN SOCKET s,
  IN const struct sockaddr FAR *name,
  IN int namelen,
  IN LPWSABUF lpCallerData OPTIONAL,
  OUT LPWSABUF lpCalleeData OPTIONAL,
  IN LPQOS lpSQOS OPTIONAL,
  IN LPQOS lpGQOS OPTIONAL,
  IN DWORD dwFlags);

typedef int
(WSAAPI *LPFN_WSANTOHL)(
  IN SOCKET s,
  IN u_long netlong,
  OUT u_long FAR *lphostlong);

typedef int
(WSAAPI *LPFN_WSANTOHS)(
  IN SOCKET s,
  IN u_short netshort,
  OUT u_short FAR *lphostshort);

typedef int
(WSAAPI *LPFN_WSARECV)(
  IN SOCKET s,
  IN LPWSABUF lpBuffers,
  IN DWORD dwBufferCount,
  OUT LPDWORD lpNumberOfBytesRecvd OPTIONAL,
  IN OUT LPDWORD lpFlags,
  IN OUT LPWSAOVERLAPPED lpOverlapped OPTIONAL,
  IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL);

typedef int
(WSAAPI *LPFN_WSARECVDISCONNECT)(
  IN SOCKET s,
  OUT LPWSABUF lpInboundDisconnectData);

typedef int
(WSAAPI *LPFN_WSARECVFROM)(
  IN SOCKET s,
  IN LPWSABUF lpBuffers,
  IN DWORD dwBufferCount,
  OUT LPDWORD lpNumberOfBytesRecvd OPTIONAL,
  IN OUT LPDWORD lpFlags,
  OUT struct sockaddr FAR *lpFrom OPTIONAL,
  IN OUT LPINT lpFromlen OPTIONAL,
  IN OUT LPWSAOVERLAPPED lpOverlapped OPTIONAL,
  IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL);

typedef BOOL
(WSAAPI *LPFN_WSARESETEVENT)(
  IN WSAEVENT hEvent);

typedef int
(WSAAPI *LPFN_WSASEND)(
  IN SOCKET s,
  IN LPWSABUF lpBuffers,
  IN DWORD dwBufferCount,
  OUT LPDWORD lpNumberOfBytesSent OPTIONAL,
  IN DWORD dwFlags,
  IN OUT LPWSAOVERLAPPED lpOverlapped OPTIONAL,
  IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL);

typedef int
(WSAAPI *LPFN_WSASENDDISCONNECT)(
  IN SOCKET s,
  IN LPWSABUF lpOutboundDisconnectData OPTIONAL);

typedef int
(WSAAPI *LPFN_WSASENDTO)(
  IN SOCKET s,
  IN LPWSABUF lpBuffers,
  IN DWORD dwBufferCount,
  OUT LPDWORD lpNumberOfBytesSent OPTIONAL,
  IN DWORD dwFlags,
  IN const struct sockaddr FAR *lpTo,
  IN int iTolen,
  IN OUT LPWSAOVERLAPPED lpOverlapped OPTIONAL,
  IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL);

typedef BOOL
(WSAAPI *LPFN_WSASETEVENT)(
  IN WSAEVENT hEvent);

typedef SOCKET
(WSAAPI *LPFN_WSASOCKETA)(
  IN int af,
  IN int type,
  IN int protocol,
  IN LPWSAPROTOCOL_INFOA lpProtocolInfo OPTIONAL,
  IN GROUP g,
  IN DWORD dwFlags);

typedef SOCKET
(WSAAPI *LPFN_WSASOCKETW)(
  IN int af,
  IN int type,
  IN int protocol,
  IN LPWSAPROTOCOL_INFOW lpProtocolInfo OPTIONAL,
  IN GROUP g,
  IN DWORD dwFlags);

#ifdef UNICODE
#define LPFN_WSASOCKET LPFN_WSASOCKETW
#else
#define LPFN_WSASOCKET LPFN_WSASOCKETA
#endif

typedef DWORD
(WSAAPI *LPFN_WSAWAITFORMULTIPLEEVENTS)(
  IN DWORD cEvents,
  IN const WSAEVENT FAR *lphEvents,
  IN BOOL fWaitAll,
  IN DWORD dwTimeout,
  IN BOOL fAlertable);

typedef INT
(WSAAPI *LPFN_WSAADDRESSTOSTRINGA)(
  IN LPSOCKADDR lpsaAddress,
  IN DWORD dwAddressLength,
  IN LPWSAPROTOCOL_INFOA lpProtocolInfo OPTIONAL,
  OUT LPSTR lpszAddressString,
  IN OUT LPDWORD lpdwAddressStringLength);

typedef INT
(WSAAPI *LPFN_WSAADDRESSTOSTRINGW)(
  IN LPSOCKADDR lpsaAddress,
  IN DWORD dwAddressLength,
  IN LPWSAPROTOCOL_INFOW lpProtocolInfo OPTIONAL,
  OUT LPWSTR lpszAddressString,
  IN OUT LPDWORD lpdwAddressStringLength);

#ifdef UNICODE
#define LPFN_WSAADDRESSTOSTRING LPFN_WSAADDRESSTOSTRINGW
#else
#define LPFN_WSAADDRESSTOSTRING LPFN_WSAADDRESSTOSTRINGA
#endif

typedef INT
(WSAAPI *LPFN_WSASTRINGTOADDRESSA)(
  IN LPSTR AddressString,
  IN INT AddressFamily,
  IN LPWSAPROTOCOL_INFOA lpProtocolInfo OPTIONAL,
  OUT LPSOCKADDR lpAddress,
  IN OUT LPINT lpAddressLength);

typedef INT
(WSAAPI *LPFN_WSASTRINGTOADDRESSW)(
  IN LPWSTR AddressString,
  IN INT AddressFamily,
  IN LPWSAPROTOCOL_INFOW lpProtocolInfo OPTIONAL,
  OUT LPSOCKADDR lpAddress,
  IN OUT LPINT lpAddressLength);

#ifdef UNICODE
#define LPFN_WSASTRINGTOADDRESS LPFN_WSASTRINGTOADDRESSW
#else
#define LPFN_WSASTRINGTOADDRESS LPFN_WSASTRINGTOADDRESSA
#endif

typedef INT
(WSAAPI *LPFN_WSALOOKUPSERVICEBEGINA)(
  IN LPWSAQUERYSETA lpqsRestrictions,
  IN DWORD dwControlFlags,
  OUT LPHANDLE lphLookup);

typedef INT
(WSAAPI *LPFN_WSALOOKUPSERVICEBEGINW)(
  IN LPWSAQUERYSETW lpqsRestrictions,
  IN DWORD dwControlFlags,
  OUT LPHANDLE lphLookup);

#ifdef UNICODE
#define LPFN_WSALOOKUPSERVICEBEGIN LPFN_WSALOOKUPSERVICEBEGINW
#else
#define LPFN_WSALOOKUPSERVICEBEGIN LPFN_WSALOOKUPSERVICEBEGINA
#endif

typedef INT
(WSAAPI *LPFN_WSALOOKUPSERVICENEXTA)(
  IN HANDLE hLookup,
  IN DWORD dwControlFlags,
  IN OUT LPDWORD lpdwBufferLength,
  OUT LPWSAQUERYSETA lpqsResults);

typedef INT
(WSAAPI *LPFN_WSALOOKUPSERVICENEXTW)(
  IN HANDLE hLookup,
  IN DWORD dwControlFlags,
  IN OUT LPDWORD lpdwBufferLength,
  OUT LPWSAQUERYSETW lpqsResults OPTIONAL);

#ifdef UNICODE
#define LPFN_WSALOOKUPSERVICENEXT LPFN_WSALOOKUPSERVICENEXTW
#else
#define LPFN_WSALOOKUPSERVICENEXT LPFN_WSALOOKUPSERVICENEXTA
#endif

typedef INT
(WSAAPI *LPFN_WSALOOKUPSERVICEEND)(
  IN HANDLE hLookup);

typedef INT
(WSAAPI *LPFN_WSAINSTALLSERVICECLASSA)(
  IN LPWSASERVICECLASSINFOA lpServiceClassInfo);

typedef INT
(WSAAPI *LPFN_WSAINSTALLSERVICECLASSW)(
  IN LPWSASERVICECLASSINFOW lpServiceClassInfo);

#ifdef UNICODE
#define LPFN_WSAINSTALLSERVICECLASS LPFN_WSAINSTALLSERVICECLASSW
#else
#define LPFN_WSAINSTALLSERVICECLASS LPFN_WSAINSTALLSERVICECLASSA
#endif

typedef INT
(WSAAPI *LPFN_WSAREMOVESERVICECLASS)(
  IN LPGUID lpServiceClassId);

typedef INT
(WSAAPI *LPFN_WSAGETSERVICECLASSINFOA)(
  IN LPGUID lpProviderId,
  IN LPGUID lpServiceClassId,
  IN OUT LPDWORD lpdwBufSize,
  OUT LPWSASERVICECLASSINFOA lpServiceClassInfo);

typedef INT
(WSAAPI *LPFN_WSAGETSERVICECLASSINFOW)(
  IN LPGUID lpProviderId,
  IN LPGUID lpServiceClassId,
  IN OUT LPDWORD lpdwBufSize,
  OUT LPWSASERVICECLASSINFOW lpServiceClassInfo);

#ifdef UNICODE
#define LPFN_WSAGETSERVICECLASSINFO LPFN_WSAGETSERVICECLASSINFOW
#else
#define LPFN_WSAGETSERVICECLASSINFO LPFN_WSAGETSERVICECLASSINFOA
#endif

typedef INT
(WSAAPI *LPFN_WSAENUMNAMESPACEPROVIDERSA)(
  IN OUT LPDWORD lpdwBufferLength,
  OUT LPWSANAMESPACE_INFOA lpnspBuffer);

typedef INT
(WSAAPI *LPFN_WSAENUMNAMESPACEPROVIDERSW)(
  IN OUT LPDWORD lpdwBufferLength,
  OUT LPWSANAMESPACE_INFOW lpnspBuffer);

#ifdef UNICODE
#define LPFN_WSAENUMNAMESPACEPROVIDERS LPFN_WSAENUMNAMESPACEPROVIDERSW
#else
#define LPFN_WSAENUMNAMESPACEPROVIDERS LPFN_WSAENUMNAMESPACEPROVIDERSA
#endif

typedef INT
(WSAAPI *LPFN_WSAGETSERVICECLASSNAMEBYCLASSIDA)(
  IN LPGUID lpServiceClassId,
  OUT LPSTR lpszServiceClassName,
  IN OUT LPDWORD lpdwBufferLength);

typedef INT
(WSAAPI *LPFN_WSAGETSERVICECLASSNAMEBYCLASSIDW)(
  IN LPGUID lpServiceClassId,
  OUT LPWSTR lpszServiceClassName,
  IN OUT LPDWORD lpdwBufferLength);

#ifdef UNICODE
#define LPFN_WSAGETSERVICECLASSNAMEBYCLASSID LPFN_WSAGETSERVICECLASSNAMEBYCLASSIDW
#else
#define LPFN_WSAGETSERVICECLASSNAMEBYCLASSID LPFN_WSAGETSERVICECLASSNAMEBYCLASSIDA
#endif

typedef INT
(WSAAPI *LPFN_WSASETSERVICEA)(
  IN LPWSAQUERYSETA lpqsRegInfo,
  IN WSAESETSERVICEOP essoperation,
  IN DWORD dwControlFlags);

typedef INT
(WSAAPI *LPFN_WSASETSERVICEW)(
  IN LPWSAQUERYSETW lpqsRegInfo,
  IN WSAESETSERVICEOP essoperation,
  IN DWORD dwControlFlags);

#ifdef UNICODE
#define LPFN_WSASETSERVICE LPFN_WSASETSERVICEW
#else
#define LPFN_WSASETSERVICE LPFN_WSASETSERVICEA
#endif

typedef INT
(WSAAPI *LPFN_WSAPROVIDERCONFIGCHANGE)(
  IN OUT LPHANDLE lpNotificationHandle OPTIONAL,
  IN OUT LPWSAOVERLAPPED lpOverlapped OPTIONAL,
  IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL);

#if(_WIN32_WINNT >= 0x0501)
typedef INT
(WSAAPI *LPFN_WSANSPIOCTL)(
  IN HANDLE hLookup,
  IN DWORD dwControlCode,
  IN LPVOID lpvInBuffer OPTIONAL,
  IN DWORD cbInBuffer,
  OUT LPVOID lpvOutBuffer OPTIONAL,
  IN DWORD cbOutBuffer,
  OUT LPDWORD lpcbBytesReturned,
  IN LPWSACOMPLETION lpCompletion OPTIONAL);
#endif /* (_WIN32_WINNT >= 0x0501) */

#if (_WIN32_WINNT >= 0x0600)

typedef INT
(WSAAPI *LPFN_WSAENUMNAMESPACEPROVIDERSEXA)(
  IN OUT LPDWORD lpdwBufferLength,
  OUT LPWSANAMESPACE_INFOEXA lpnspBuffer);

typedef INT
(WSAAPI *LPFN_WSAENUMNAMESPACEPROVIDERSEXW)(
  IN OUT LPDWORD lpdwBufferLength,
  OUT LPWSANAMESPACE_INFOEXW lpnspBuffer);

#ifdef UNICODE
#define LPFN_WSAENUMNAMESPACEPROVIDERSEX LPFN_WSAENUMNAMESPACEPROVIDERSEXW
#else
#define LPFN_WSAENUMNAMESPACEPROVIDERSEX LPFN_WSAENUMNAMESPACEPROVIDERSEXA
#endif

#endif /* (_WIN32_WINNT >= 0x600) */

#endif /* INCL_WINSOCK_API_TYPEDEFS */

#if INCL_WINSOCK_API_PROTOTYPES

WINSOCK_API_LINKAGE
SOCKET
WSAAPI
accept(
  IN SOCKET s,
  OUT struct sockaddr FAR *addr OPTIONAL,
  IN OUT int FAR *addrlen OPTIONAL);

WINSOCK_API_LINKAGE
int
WSAAPI
bind(
  IN SOCKET s,
  IN const struct sockaddr FAR *addr,
  IN int namelen);

WINSOCK_API_LINKAGE
int
WSAAPI
closesocket(
  IN SOCKET s);

WINSOCK_API_LINKAGE
int
WSAAPI
connect(
  IN SOCKET s,
  IN const struct sockaddr FAR *name,
  IN int namelen);

WINSOCK_API_LINKAGE
int
WSAAPI
ioctlsocket(
  IN SOCKET s,
  IN long cmd,
  IN OUT u_long FAR *argp);

WINSOCK_API_LINKAGE
int
WSAAPI
getpeername(
  IN SOCKET s,
  OUT struct sockaddr FAR *name,
  IN OUT int FAR *namelen);

WINSOCK_API_LINKAGE
int
WSAAPI
getsockname(
  IN SOCKET s,
  OUT struct sockaddr FAR *name,
  IN OUT int FAR *namelen);

WINSOCK_API_LINKAGE
int
WSAAPI
getsockopt(
  IN SOCKET s,
  IN int level,
  IN int optname,
  OUT char FAR *optval,
  IN OUT int FAR *optlen);

WINSOCK_API_LINKAGE
u_long
WSAAPI
htonl(
  IN u_long hostlong);

WINSOCK_API_LINKAGE
u_short
WSAAPI
htons(
  IN u_short hostshort);

WINSOCK_API_LINKAGE
unsigned long
WSAAPI
inet_addr(
  IN const char FAR *cp);

WINSOCK_API_LINKAGE
char FAR *
WSAAPI
inet_ntoa(
  IN struct in_addr in);

WINSOCK_API_LINKAGE
int
WSAAPI
listen(
  IN SOCKET s,
  IN int backlog);

WINSOCK_API_LINKAGE
u_long
WSAAPI
ntohl(
  IN u_long netlong);

WINSOCK_API_LINKAGE
u_short
WSAAPI
ntohs(
  IN u_short netshort);

WINSOCK_API_LINKAGE
int
WSAAPI
recv(
  IN SOCKET s,
  OUT char FAR *buf,
  IN int len,
  IN int flags);

WINSOCK_API_LINKAGE
int
WSAAPI
recvfrom(
  IN SOCKET s,
  OUT char FAR *buf,
  IN int len,
  IN int flags,
  OUT struct sockaddr FAR *from OPTIONAL,
  IN OUT int FAR *fromlen OPTIONAL);

WINSOCK_API_LINKAGE
int
WSAAPI
select(
  IN int nfds,
  IN OUT fd_set FAR *readfds OPTIONAL,
  IN OUT fd_set FAR *writefds OPTIONAL,
  IN OUT fd_set FAR *exceptfds OPTIONAL,
  IN const struct timeval FAR *timeout OPTIONAL);

WINSOCK_API_LINKAGE
int
WSAAPI
send(
  IN SOCKET s,
  IN const char FAR *buf,
  IN int len,
  IN int flags);

WINSOCK_API_LINKAGE
int
WSAAPI
sendto(
  IN SOCKET s,
  IN const char FAR *buf,
  IN int len,
  IN int flags,
  IN const struct sockaddr FAR *to OPTIONAL,
  IN int tolen);

WINSOCK_API_LINKAGE
int
WSAAPI
setsockopt(
  IN SOCKET s,
  IN int level,
  IN int optname,
  IN const char FAR *optval OPTIONAL,
  IN int optlen);

WINSOCK_API_LINKAGE
int
WSAAPI
shutdown(
  IN SOCKET s,
  IN int how);

WINSOCK_API_LINKAGE
SOCKET
WSAAPI
socket(
  IN int af,
  IN int type,
  IN int protocol);

WINSOCK_API_LINKAGE
struct hostent FAR *
WSAAPI
gethostbyaddr(
  IN const char FAR *addr,
  IN int len,
  IN int type);

WINSOCK_API_LINKAGE
struct hostent FAR *
WSAAPI
gethostbyname(
  IN const char FAR *name);

WINSOCK_API_LINKAGE
int
WSAAPI
gethostname(
  OUT char FAR *name,
  IN int namelen);

WINSOCK_API_LINKAGE
struct servent FAR *
WSAAPI
getservbyport(
  IN int port,
  IN const char FAR *proto);

WINSOCK_API_LINKAGE
struct servent FAR *
WSAAPI
getservbyname(
  IN const char FAR *name,
  IN const char FAR *proto);

WINSOCK_API_LINKAGE
struct protoent FAR *
WSAAPI
getprotobynumber(
  IN int number);

WINSOCK_API_LINKAGE
struct protoent FAR *
WSAAPI
getprotobyname(
  IN const char FAR *name);

WINSOCK_API_LINKAGE
int
WSAAPI
WSAStartup(
  IN WORD wVersionRequired,
  OUT LPWSADATA lpWSAData);

WINSOCK_API_LINKAGE
int
WSAAPI
WSACleanup(void);

WINSOCK_API_LINKAGE
void
WSAAPI
WSASetLastError(
  IN int iError);

WINSOCK_API_LINKAGE
int
WSAAPI
WSAGetLastError(void);

WINSOCK_API_LINKAGE
BOOL
WSAAPI
WSAIsBlocking(void);

WINSOCK_API_LINKAGE
int
WSAAPI
WSAUnhookBlockingHook(void);

WINSOCK_API_LINKAGE
FARPROC
WSAAPI
WSASetBlockingHook(
  IN FARPROC lpBlockFunc);

WINSOCK_API_LINKAGE
int
WSAAPI
WSACancelBlockingCall(void);

WINSOCK_API_LINKAGE
HANDLE
WSAAPI
WSAAsyncGetServByName(
  IN HWND hWnd,
  IN u_int wMsg,
  IN const char FAR *name,
  IN const char FAR *proto,
  OUT char FAR *buf,
  IN int buflen);

WINSOCK_API_LINKAGE
HANDLE
WSAAPI
WSAAsyncGetServByPort(
  IN HWND hWnd,
  IN u_int wMsg,
  IN int port,
  IN const char FAR *proto,
  OUT char FAR *buf,
  IN int buflen);

WINSOCK_API_LINKAGE
HANDLE
WSAAPI
WSAAsyncGetProtoByName(
  IN HWND hWnd,
  IN u_int wMsg,
  IN const char FAR *name,
  OUT char FAR *buf,
  IN int buflen);

WINSOCK_API_LINKAGE
HANDLE
WSAAPI
WSAAsyncGetProtoByNumber(
  IN HWND hWnd,
  IN u_int wMsg,
  IN int number,
  OUT char FAR *buf,
  IN int buflen);

WINSOCK_API_LINKAGE
HANDLE
WSAAPI
WSAAsyncGetHostByName(
  IN HWND hWnd,
  IN u_int wMsg,
  IN const char FAR *name,
  OUT char FAR *buf,
  IN int buflen);

WINSOCK_API_LINKAGE
HANDLE
WSAAPI
WSAAsyncGetHostByAddr(
  IN HWND hWnd,
  IN u_int wMsg,
  IN const char FAR *addr,
  IN int len,
  IN int type,
  OUT char FAR *buf,
  IN int buflen);

WINSOCK_API_LINKAGE
int
WSAAPI
WSACancelAsyncRequest(
  IN HANDLE hAsyncTaskHandle);

WINSOCK_API_LINKAGE
int
WSAAPI
WSAAsyncSelect(
  IN SOCKET s,
  IN HWND hWnd,
  IN u_int wMsg,
  IN long lEvent);

WINSOCK_API_LINKAGE
SOCKET
WSAAPI
WSAAccept(
  IN SOCKET s,
  OUT struct sockaddr FAR *addr OPTIONAL,
  IN OUT LPINT addrlen OPTIONAL,
  IN LPCONDITIONPROC lpfnCondition OPTIONAL,
  IN DWORD_PTR dwCallbackData OPTIONAL);

WINSOCK_API_LINKAGE
BOOL
WSAAPI
WSACloseEvent(
  IN WSAEVENT hEvent);

WINSOCK_API_LINKAGE
int
WSAAPI
WSAConnect(
  IN SOCKET s,
  IN const struct sockaddr FAR *name,
  IN int namelen,
  IN LPWSABUF lpCallerData OPTIONAL,
  OUT LPWSABUF lpCalleeData OPTIONAL,
  IN LPQOS lpSQOS OPTIONAL,
  IN LPQOS lpGQOS OPTIONAL);

#ifdef UNICODE
#define WSAConnectByName WSAConnectByNameW
#else
#define WSAConnectByName WSAConnectByNameA
#endif

BOOL
PASCAL
WSAConnectByNameW(
  IN SOCKET s,
  IN LPWSTR nodename,
  IN LPWSTR servicename,
  IN OUT LPDWORD LocalAddressLength,
  OUT LPSOCKADDR LocalAddress,
  IN OUT LPDWORD RemoteAddressLength,
  OUT LPSOCKADDR RemoteAddress,
  IN const struct timeval *timeout,
  IN LPWSAOVERLAPPED Reserved OPTIONAL);

BOOL
PASCAL
WSAConnectByNameA(
  IN SOCKET s,
  IN LPCSTR nodename,
  IN LPCSTR servicename,
  IN OUT LPDWORD LocalAddressLength,
  OUT LPSOCKADDR LocalAddress,
  IN OUT LPDWORD RemoteAddressLength,
  OUT LPSOCKADDR RemoteAddress,
  IN const struct timeval *timeout,
  IN LPWSAOVERLAPPED Reserved OPTIONAL);

BOOL
PASCAL
WSAConnectByList(
  IN SOCKET s,
  IN PSOCKET_ADDRESS_LIST SocketAddress,
  IN OUT LPDWORD LocalAddressLength,
  OUT LPSOCKADDR LocalAddress,
  IN OUT LPDWORD RemoteAddressLength,
  OUT LPSOCKADDR RemoteAddress,
  IN const struct timeval *timeout,
  IN LPWSAOVERLAPPED Reserved OPTIONAL);

WINSOCK_API_LINKAGE
WSAEVENT
WSAAPI
WSACreateEvent(void);

WINSOCK_API_LINKAGE
int
WSAAPI
WSADuplicateSocketA(
  IN SOCKET s,
  IN DWORD dwProcessId,
  OUT LPWSAPROTOCOL_INFOA lpProtocolInfo);

WINSOCK_API_LINKAGE
int
WSAAPI
WSADuplicateSocketW(
  IN SOCKET s,
  IN DWORD dwProcessId,
  OUT LPWSAPROTOCOL_INFOW lpProtocolInfo);

#ifdef UNICODE
#define WSADuplicateSocket WSADuplicateSocketW
#else
#define WSADuplicateSocket WSADuplicateSocketA
#endif

WINSOCK_API_LINKAGE
int
WSAAPI
WSAEnumNetworkEvents(
  IN SOCKET s,
  IN WSAEVENT hEventObject,
  OUT LPWSANETWORKEVENTS lpNetworkEvents);

WINSOCK_API_LINKAGE
int
WSAAPI
WSAEnumProtocolsA(
  IN LPINT lpiProtocols OPTIONAL,
  OUT LPWSAPROTOCOL_INFOA lpProtocolBuffer OPTIONAL,
  IN OUT LPDWORD lpdwBufferLength);

WINSOCK_API_LINKAGE
int
WSAAPI
WSAEnumProtocolsW(
  IN LPINT lpiProtocols OPTIONAL,
  OUT LPWSAPROTOCOL_INFOW lpProtocolBuffer OPTIONAL,
  IN OUT LPDWORD lpdwBufferLength);

#ifdef UNICODE
#define WSAEnumProtocols WSAEnumProtocolsW
#else
#define WSAEnumProtocols WSAEnumProtocolsA
#endif

WINSOCK_API_LINKAGE
int
WSAAPI
WSAEventSelect(
  IN SOCKET s,
  IN WSAEVENT hEventObject,
  IN long lNetworkEvents);

WINSOCK_API_LINKAGE
BOOL
WSAAPI
WSAGetOverlappedResult(
  IN SOCKET s,
  IN LPWSAOVERLAPPED lpOverlapped,
  OUT LPDWORD lpcbTransfer,
  IN BOOL fWait,
  OUT LPDWORD lpdwFlags);

WINSOCK_API_LINKAGE
BOOL
WSAAPI
WSAGetQOSByName(
  IN SOCKET s,
  IN LPWSABUF lpQOSName,
  OUT LPQOS lpQOS);

WINSOCK_API_LINKAGE
int
WSAAPI
WSAHtonl(
  IN SOCKET s,
  IN u_long hostlong,
  OUT u_long FAR *lpnetlong);

WINSOCK_API_LINKAGE
int
WSAAPI
WSAHtons(
  IN SOCKET s,
  IN u_short hostshort,
  OUT u_short FAR *lpnetshort);

WINSOCK_API_LINKAGE
int
WSAAPI
WSAIoctl(
  IN SOCKET s,
  IN DWORD dwIoControlCode,
  IN LPVOID lpvInBuffer OPTIONAL,
  IN DWORD cbInBuffer,
  OUT LPVOID lpvOutBuffer OPTIONAL,
  IN DWORD cbOutBuffer,
  OUT LPDWORD lpcbBytesReturned,
  IN OUT LPWSAOVERLAPPED lpOverlapped OPTIONAL,
  IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL);

WINSOCK_API_LINKAGE
SOCKET
WSAAPI
WSAJoinLeaf(
  IN SOCKET s,
  IN const struct sockaddr FAR *name,
  IN int namelen,
  IN LPWSABUF lpCallerData OPTIONAL,
  OUT LPWSABUF lpCalleeData OPTIONAL,
  IN LPQOS lpSQOS OPTIONAL,
  IN LPQOS lpGQOS OPTIONAL,
  IN DWORD dwFlags);

WINSOCK_API_LINKAGE
int
WSAAPI
WSANtohl(
  IN SOCKET s,
  IN u_long netlong,
  OUT u_long FAR *lphostlong);

WINSOCK_API_LINKAGE
int
WSAAPI
WSANtohs(
  IN SOCKET s,
  IN u_short netshort,
  OUT u_short FAR *lphostshort);

WINSOCK_API_LINKAGE
int
WSAAPI
WSARecv(
  IN SOCKET s,
  IN LPWSABUF lpBuffers,
  IN DWORD dwBufferCount,
  OUT LPDWORD lpNumberOfBytesRecvd OPTIONAL,
  IN OUT LPDWORD lpFlags,
  IN OUT LPWSAOVERLAPPED lpOverlapped OPTIONAL,
  IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL);

WINSOCK_API_LINKAGE
int
WSAAPI
WSARecvDisconnect(
  IN SOCKET s,
  IN LPWSABUF lpInboundDisconnectData OPTIONAL);

WINSOCK_API_LINKAGE
int
WSAAPI
WSARecvFrom(
  IN SOCKET s,
  IN OUT LPWSABUF lpBuffers,
  IN DWORD dwBufferCount,
  OUT LPDWORD lpNumberOfBytesRecvd OPTIONAL,
  IN OUT LPDWORD lpFlags,
  OUT struct sockaddr FAR *lpFrom,
  IN OUT LPINT lpFromlen OPTIONAL,
  IN OUT LPWSAOVERLAPPED lpOverlapped OPTIONAL,
  IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL);

WINSOCK_API_LINKAGE
BOOL
WSAAPI
WSAResetEvent(
  IN WSAEVENT hEvent);

WINSOCK_API_LINKAGE
int
WSAAPI
WSASendDisconnect(
  IN SOCKET s,
  IN LPWSABUF lpOutboundDisconnectData OPTIONAL);

WINSOCK_API_LINKAGE
int
WSAAPI
WSASend(
  IN SOCKET s,
  IN LPWSABUF lpBuffers,
  IN DWORD dwBufferCount,
  OUT LPDWORD lpNumberOfBytesSent OPTIONAL,
  IN DWORD dwFlags,
  IN OUT LPWSAOVERLAPPED lpOverlapped OPTIONAL,
  IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL);

WINSOCK_API_LINKAGE
int
WSAAPI
WSASendTo(
  IN SOCKET s,
  IN LPWSABUF lpBuffers,
  IN DWORD dwBufferCount,
  OUT LPDWORD lpNumberOfBytesSent OPTIONAL,
  IN DWORD dwFlags,
  IN const struct sockaddr FAR *lpTo OPTIONAL,
  IN int iTolen,
  IN OUT LPWSAOVERLAPPED lpOverlapped OPTIONAL,
  IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL);

WINSOCK_API_LINKAGE
BOOL
WSAAPI
WSASetEvent(
  IN WSAEVENT hEvent);

WINSOCK_API_LINKAGE
SOCKET
WSAAPI
WSASocketA(
  IN int af,
  IN int type,
  IN int protocol,
  IN LPWSAPROTOCOL_INFOA lpProtocolInfo OPTIONAL,
  IN GROUP g,
  IN DWORD dwFlags);

WINSOCK_API_LINKAGE
SOCKET
WSAAPI
WSASocketW(
  IN int af,
  IN int type,
  IN int protocol,
  IN LPWSAPROTOCOL_INFOW lpProtocolInfo OPTIONAL,
  IN GROUP g,
  IN DWORD dwFlags);

#ifdef UNICODE
#define WSASocket WSASocketW
#else
#define WSASocket WSASocketA
#endif

WINSOCK_API_LINKAGE
DWORD
WSAAPI
WSAWaitForMultipleEvents(
  IN DWORD cEvents,
  IN const WSAEVENT FAR *lphEvents,
  IN BOOL fWaitAll,
  IN DWORD dwTimeout,
  IN BOOL fAlertable);

WINSOCK_API_LINKAGE
INT
WSAAPI
WSAAddressToStringA(
  IN LPSOCKADDR lpsaAddress,
  IN DWORD dwAddressLength,
  IN LPWSAPROTOCOL_INFOA lpProtocolInfo OPTIONAL,
  OUT LPSTR lpszAddressString,
  IN OUT LPDWORD lpdwAddressStringLength);

WINSOCK_API_LINKAGE
INT
WSAAPI
WSAAddressToStringW(
  IN LPSOCKADDR lpsaAddress,
  IN DWORD dwAddressLength,
  IN LPWSAPROTOCOL_INFOW lpProtocolInfo OPTIONAL,
  OUT LPWSTR lpszAddressString,
  IN OUT LPDWORD lpdwAddressStringLength);

#ifdef UNICODE
#define WSAAddressToString WSAAddressToStringW
#else
#define WSAAddressToString WSAAddressToStringA
#endif

WINSOCK_API_LINKAGE
INT
WSAAPI
WSAStringToAddressA(
  IN LPSTR AddressString,
  IN INT AddressFamily,
  IN LPWSAPROTOCOL_INFOA lpProtocolInfo OPTIONAL,
  OUT LPSOCKADDR lpAddress,
  IN OUT LPINT lpAddressLength);

WINSOCK_API_LINKAGE
INT
WSAAPI
WSAStringToAddressW(
  IN LPWSTR AddressString,
  IN INT AddressFamily,
  IN LPWSAPROTOCOL_INFOW lpProtocolInfo OPTIONAL,
  OUT LPSOCKADDR lpAddress,
  IN OUT LPINT lpAddressLength);

#ifdef UNICODE
#define WSAStringToAddress WSAStringToAddressW
#else
#define WSAStringToAddress WSAStringToAddressA
#endif

WINSOCK_API_LINKAGE
INT
WSAAPI
WSALookupServiceBeginA(
  IN LPWSAQUERYSETA lpqsRestrictions,
  IN DWORD dwControlFlags,
  OUT LPHANDLE lphLookup);

WINSOCK_API_LINKAGE
INT
WSAAPI
WSALookupServiceBeginW(
  IN LPWSAQUERYSETW lpqsRestrictions,
  IN DWORD dwControlFlags,
  OUT LPHANDLE lphLookup);

#ifdef UNICODE
#define WSALookupServiceBegin WSALookupServiceBeginW
#else
#define WSALookupServiceBegin WSALookupServiceBeginA
#endif

WINSOCK_API_LINKAGE
INT
WSAAPI
WSALookupServiceNextA(
  IN HANDLE hLookup,
  IN DWORD dwControlFlags,
  IN OUT LPDWORD lpdwBufferLength,
  OUT LPWSAQUERYSETA lpqsResults);

WINSOCK_API_LINKAGE
INT
WSAAPI
WSALookupServiceNextW(
  IN HANDLE hLookup,
  IN DWORD dwControlFlags,
  IN OUT LPDWORD lpdwBufferLength,
  OUT LPWSAQUERYSETW lpqsResults OPTIONAL);

#ifdef UNICODE
#define WSALookupServiceNext WSALookupServiceNextW
#else
#define WSALookupServiceNext WSALookupServiceNextA
#endif

WINSOCK_API_LINKAGE
INT
WSAAPI
WSALookupServiceEnd(
  IN HANDLE hLookup);

WINSOCK_API_LINKAGE
INT
WSAAPI
WSAInstallServiceClassA(
  IN LPWSASERVICECLASSINFOA lpServiceClassInfo);

WINSOCK_API_LINKAGE
INT
WSAAPI
WSAInstallServiceClassW(
  IN LPWSASERVICECLASSINFOW lpServiceClassInfo);

#ifdef UNICODE
#define WSAInstallServiceClass WSAInstallServiceClassW
#else
#define WSAInstallServiceClass WSAInstallServiceClassA
#endif

WINSOCK_API_LINKAGE
INT
WSAAPI
WSARemoveServiceClass(
  IN LPGUID lpServiceClassId);

WINSOCK_API_LINKAGE
INT
WSAAPI
WSAGetServiceClassInfoA(
  IN LPGUID lpProviderId,
  IN LPGUID lpServiceClassId,
  IN OUT LPDWORD lpdwBufSize,
  OUT LPWSASERVICECLASSINFOA lpServiceClassInfo);

WINSOCK_API_LINKAGE
INT
WSAAPI
WSAGetServiceClassInfoW(
  IN LPGUID lpProviderId,
  IN LPGUID lpServiceClassId,
  IN OUT LPDWORD lpdwBufSize,
  OUT LPWSASERVICECLASSINFOW lpServiceClassInfo);

#ifdef UNICODE
#define WSAGetServiceClassInfo WSAGetServiceClassInfoW
#else
#define WSAGetServiceClassInfo WSAGetServiceClassInfoA
#endif

WINSOCK_API_LINKAGE
INT
WSAAPI
WSAEnumNameSpaceProvidersA(
  IN OUT LPDWORD lpdwBufferLength,
  OUT LPWSANAMESPACE_INFOA lpnspBuffer);

WINSOCK_API_LINKAGE
INT
WSAAPI
WSAEnumNameSpaceProvidersW(
  IN OUT LPDWORD lpdwBufferLength,
  OUT LPWSANAMESPACE_INFOW lpnspBuffer);

#ifdef UNICODE
#define WSAEnumNameSpaceProviders WSAEnumNameSpaceProvidersW
#else
#define WSAEnumNameSpaceProviders WSAEnumNameSpaceProvidersA
#endif

WINSOCK_API_LINKAGE
INT
WSAAPI
WSAGetServiceClassNameByClassIdA(
  IN LPGUID lpServiceClassId,
  OUT LPSTR lpszServiceClassName,
  IN OUT LPDWORD lpdwBufferLength);

WINSOCK_API_LINKAGE
INT
WSAAPI
WSAGetServiceClassNameByClassIdW(
  IN LPGUID lpServiceClassId,
  OUT LPWSTR lpszServiceClassName,
  IN OUT LPDWORD lpdwBufferLength);

#ifdef UNICODE
#define WSAGetServiceClassNameByClassId WSAGetServiceClassNameByClassIdW
#else
#define WSAGetServiceClassNameByClassId WSAGetServiceClassNameByClassIdA
#endif

WINSOCK_API_LINKAGE
INT
WSAAPI
WSASetServiceA(
  IN LPWSAQUERYSETA lpqsRegInfo,
  IN WSAESETSERVICEOP essoperation,
  IN DWORD dwControlFlags);

WINSOCK_API_LINKAGE
INT
WSAAPI
WSASetServiceW(
  IN LPWSAQUERYSETW lpqsRegInfo,
  IN WSAESETSERVICEOP essoperation,
  IN DWORD dwControlFlags);

#ifdef UNICODE
#define WSASetService WSASetServiceW
#else
#define WSASetService WSASetServiceA
#endif

WINSOCK_API_LINKAGE
INT
WSAAPI
WSAProviderConfigChange(
  IN OUT LPHANDLE lpNotificationHandle OPTIONAL,
  IN OUT LPWSAOVERLAPPED lpOverlapped OPTIONAL,
  IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL);

#if(_WIN32_WINNT >= 0x0501)
WINSOCK_API_LINKAGE
INT
WSAAPI
WSANSPIoctl(
  IN HANDLE hLookup,
  IN DWORD dwControlCode,
  IN LPVOID lpvInBuffer OPTIONAL,
  IN DWORD cbInBuffer,
  OUT LPVOID lpvOutBuffer OPTIONAL,
  IN DWORD cbOutBuffer,
  OUT LPDWORD lpcbBytesReturned,
  IN LPWSACOMPLETION lpCompletion OPTIONAL);
#endif /* (_WIN32_WINNT >= 0x0501) */

#if(_WIN32_WINNT >= 0x0600)

WINSOCK_API_LINKAGE
int
WSAAPI
WSASendMsg(
  IN SOCKET Handle,
  IN LPWSAMSG lpMsg,
  IN DWORD dwFlags,
  OUT LPDWORD lpNumberOfBytesSent OPTIONAL,
  IN OUT LPWSAOVERLAPPED lpOverlapped OPTIONAL,
  IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine OPTIONAL);

WINSOCK_API_LINKAGE
INT
WSAAPI
WSAEnumNameSpaceProvidersExA(
  IN OUT LPDWORD lpdwBufferLength,
  OUT LPWSANAMESPACE_INFOEXA lpnspBuffer);

WINSOCK_API_LINKAGE
INT
WSAAPI
WSAEnumNameSpaceProvidersExW(
  IN OUT LPDWORD lpdwBufferLength,
  OUT LPWSANAMESPACE_INFOEXW lpnspBuffer);

#ifdef UNICODE
#define WSAEnumNameSpaceProvidersEx WSAEnumNameSpaceProvidersExW
#else
#define WSAEnumNameSpaceProvidersEx WSAEnumNameSpaceProvidersExA
#endif

WINSOCK_API_LINKAGE
int
WSAAPI
WSAPoll(
  IN OUT LPWSAPOLLFD fdArray,
  IN ULONG fds,
  IN INT timeout);

#endif /* (_WIN32_WINNT >= 0x0600) */

#endif /* INCL_WINSOCK_API_PROTOTYPES */

typedef struct sockaddr_in FAR *LPSOCKADDR_IN;
typedef struct linger LINGER;
typedef struct linger *PLINGER;
typedef struct linger FAR *LPLINGER;
typedef struct fd_set FD_SET;
typedef struct fd_set *PFD_SET;
typedef struct fd_set FAR *LPFD_SET;
typedef struct hostent HOSTENT;
typedef struct hostent *PHOSTENT;
typedef struct hostent FAR *LPHOSTENT;
typedef struct servent SERVENT;
typedef struct servent *PSERVENT;
typedef struct servent FAR *LPSERVENT;
typedef struct protoent PROTOENT;
typedef struct protoent *PPROTOENT;
typedef struct protoent FAR *LPPROTOENT;
typedef struct timeval TIMEVAL;
typedef struct timeval *PTIMEVAL;
typedef struct timeval FAR *LPTIMEVAL;

#define WSAMAKEASYNCREPLY(buflen,error) MAKELONG(buflen,error)
#define WSAMAKESELECTREPLY(event,error) MAKELONG(event,error)
#define WSAGETASYNCBUFLEN(lParam) LOWORD(lParam)
#define WSAGETASYNCERROR(lParam) HIWORD(lParam)
#define WSAGETSELECTEVENT(lParam) LOWORD(lParam)
#define WSAGETSELECTERROR(lParam) HIWORD(lParam)

#ifdef __cplusplus
}
#endif

#ifdef _NEED_POPPACK
#include <poppack.h>
#endif

/* FIXME : 
#if(_WIN32_WINNT >= 0x0501)
#ifdef IPV6STRICT
#include <wsipv6ok.h>
#endif
#endif */

#endif /* !(defined _WINSOCK2API_ || defined _WINSOCKAPI_) */
