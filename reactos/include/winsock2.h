/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 DLL
 * FILE:        include/winsock2.h
 * PURPOSE:     Header file for the WinSock 2 DLL
 *              and WinSock 2 applications
 * DEFINES:     UNICODE    - Use unicode prototypes
 *              FD_SETSIZE - Maximum size of an FD_SET (default is 64)
 */
#ifndef __WINSOCK2_H
#define __WINSOCK2_H

#include <windows.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#define WSAAPI  STDCALL


#if 0
typedef struct _GUID {
    ULONG Data1;
    USHORT Data2;
    USHORT Data3;
    UCHAR Data4[8];
} GUID, *PGUID, *LPGUID;
#endif

typedef UINT   SOCKET;

#define INVALID_SOCKET  ((SOCKET)~0)
#define SOCKET_ERROR    (-1)

/* Socket types */
#define SOCK_STREAM     1
#define SOCK_DGRAM      2
#define SOCK_RAW        3
#define SOCK_RDM        4
#define SOCK_SEQPACKET  5

/* Per socket flags */
#define SO_DEBUG        0x0001
#define SO_ACCEPTCONN   0x0002
#define SO_REUSEADDR    0x0004
#define SO_KEEPALIVE    0x0008
#define SO_DONTROUTE    0x0010
#define SO_BROADCAST    0x0020
#define SO_USELOOPBACK  0x0040
#define SO_LINGER       0x0080
#define SO_OOBINLINE    0x0100

#define SO_DONTLINGER   (UINT)(~SO_LINGER)

#define SO_SNDBUF       0x1001
#define SO_RCVBUF       0x1002
#define SO_SNDLOWAT     0x1003
#define SO_RCVLOWAT     0x1004
#define SO_SNDTIMEO     0x1005
#define SO_RCVTIMEO     0x1006
#define SO_ERROR        0x1007
#define SO_TYPE         0x1008


/* Address families */
#define AF_UNSPEC       0
#define AF_UNIX         1
#define AF_INET         2
#define AF_IMPLINK      3
#define AF_PUP          4
#define AF_CHAOS        5
#define AF_NS           6
#define AF_ISO          7
#define AF_OSI          AF_ISO
#define AF_ECMA         8
#define AF_DATAKIT      9
#define AF_CCITT        10
#define AF_SNA          11
#define AF_DECnet       12
#define AF_DLI          13
#define AF_LAT          14
#define AF_HYLINK       15
#define AF_APPLETALK    16
#define AF_NETBIOS      17

#define AF_MAX          18


/* Protocol families, same as address families */
#define PF_UNSPEC       AF_UNSPEC
#define PF_UNIX         AF_UNIX
#define PF_INET         AF_INET
#define PF_IMPLINK      AF_IMPLINK
#define PF_PUP          AF_PUP
#define PF_CHAOS        AF_CHAOS
#define PF_NS           AF_NS
#define PF_ISO          AF_ISO
#define PF_OSI          AF_OSI
#define PF_ECMA         AF_ECMA
#define PF_DATAKIT      AF_DATAKIT
#define PF_CCITT        AF_CCITT
#define PF_SNA          AF_SNA
#define PF_DECnet       AF_DECnet
#define PF_DLI          AF_DLI
#define PF_LAT          AF_LAT
#define PF_HYLINK       AF_HYLINK
#define PF_APPLETALK    AF_APPLETALK

#define PF_MAX          AF_MAX


#define SOL_SOCKET  0xffff

#define SOMAXCONN   5

#define MSG_OOB         0x1
#define MSG_PEEK        0x2
#define MSG_DONTROUTE   0x4

#define MSG_MAXIOVLEN   16


#define FD_READ         0x01
#define FD_WRITE        0x02
#define FD_OOB          0x04
#define FD_ACCEPT       0x08
#define FD_CONNECT      0x10
#define FD_CLOSE        0x20

#define FD_MAX_EVENTS   6
#define FD_ALL_EVENTS   ((1 << FD_MAX_EVENTS) - 1)


/* Error codes */

#define WSABASEERR              10000

#define WSAEINTR                (WSABASEERR+4)
#define WSAEBADF                (WSABASEERR+9)
#define WSAEACCES               (WSABASEERR+13)
#define WSAEFAULT               (WSABASEERR+14)
#define WSAEINVAL               (WSABASEERR+22)
#define WSAEMFILE               (WSABASEERR+24)

#define WSAEWOULDBLOCK          (WSABASEERR+35)
#define WSAEINPROGRESS          (WSABASEERR+36)
#define WSAEALREADY             (WSABASEERR+37)
#define WSAENOTSOCK             (WSABASEERR+38)
#define WSAEDESTADDRREQ         (WSABASEERR+39)
#define WSAEMSGSIZE             (WSABASEERR+40)
#define WSAEPROTOTYPE           (WSABASEERR+41)
#define WSAENOPROTOOPT          (WSABASEERR+42)
#define WSAEPROTONOSUPPORT      (WSABASEERR+43)
#define WSAESOCKTNOSUPPORT      (WSABASEERR+44)
#define WSAEOPNOTSUPP           (WSABASEERR+45)
#define WSAEPFNOSUPPORT         (WSABASEERR+46)
#define WSAEAFNOSUPPORT         (WSABASEERR+47)
#define WSAEADDRINUSE           (WSABASEERR+48)
#define WSAEADDRNOTAVAIL        (WSABASEERR+49)
#define WSAENETDOWN             (WSABASEERR+50)
#define WSAENETUNREACH          (WSABASEERR+51)
#define WSAENETRESET            (WSABASEERR+52)
#define WSAECONNABORTED         (WSABASEERR+53)
#define WSAECONNRESET           (WSABASEERR+54)
#define WSAENOBUFS              (WSABASEERR+55)
#define WSAEISCONN              (WSABASEERR+56)
#define WSAENOTCONN             (WSABASEERR+57)
#define WSAESHUTDOWN            (WSABASEERR+58)
#define WSAETOOMANYREFS         (WSABASEERR+59)
#define WSAETIMEDOUT            (WSABASEERR+60)
#define WSAECONNREFUSED         (WSABASEERR+61)
#define WSAELOOP                (WSABASEERR+62)
#define WSAENAMETOOLONG         (WSABASEERR+63)
#define WSAEHOSTDOWN            (WSABASEERR+64)
#define WSAEHOSTUNREACH         (WSABASEERR+65)
#define WSAENOTEMPTY            (WSABASEERR+66)
#define WSAEUSERS               (WSABASEERR+68)
#define WSAEDQUOT               (WSABASEERR+69)
#define WSAESTALE               (WSABASEERR+70)
#define WSAEREMOTE              (WSABASEERR+71)

/* Extended Windows Sockets error codes */
#define WSASYSNOTREADY          (WSABASEERR+91)
#define WSAVERNOTSUPPORTED      (WSABASEERR+92)
#define WSANOTINITIALISED       (WSABASEERR+93)
#define WSAEDISCON              (WSABASEERR+101)
#define WSAENOMORE              (WSABASEERR+102)
#define WSAECANCELLED           (WSABASEERR+103)
#define WSAEINVALIDPROCTABLE    (WSABASEERR+104)
#define WSAEINVALIDPROVIDER     (WSABASEERR+105)
#define WSAEPROVIDERFAILEDINIT  (WSABASEERR+106)
#define WSASYSCALLFAILURE       (WSABASEERR+107)
#define WSASERVICE_NOT_FOUND    (WSABASEERR+108)
#define WSATYPE_NOT_FOUND       (WSABASEERR+109)
#define WSA_E_NO_MORE           (WSABASEERR+110)
#define WSA_E_CANCELLED         (WSABASEERR+111)
#define WSAEREFUSED             (WSABASEERR+112)

#define WSAHOST_NOT_FOUND       (WSABASEERR+1001)
#define WSATRY_AGAIN            (WSABASEERR+1002)
#define WSANO_RECOVERY          (WSABASEERR+1003)
#define WSANO_DATA              (WSABASEERR+1004)
#define WSANO_ADDRESS           WSANO_DATA

#define WSAEVENT                HANDLE
#define LPWSAEVENT              LPHANDLE

#define WSA_IO_PENDING          (ERROR_IO_PENDING)
#define WSA_IO_INCOMPLETE       (ERROR_IO_INCOMPLETE)
#define WSA_INVALID_HANDLE      (ERROR_INVALID_HANDLE)
#define WSA_INVALID_PARAMETER   (ERROR_INVALID_PARAMETER)
#define WSA_NOT_ENOUGH_MEMORY   (ERROR_NOT_ENOUGH_MEMORY)
#define WSA_OPERATION_ABORTED   (ERROR_OPERATION_ABORTED)

#define WSA_INVALID_EVENT       ((WSAEVENT)NULL)
#define WSA_MAXIMUM_WAIT_EVENTS (MAXIMUM_WAIT_OBJECTS)
#define WSA_WAIT_FAILED         ((DWORD)-1L)
#define WSA_WAIT_EVENT_0        (WAIT_OBJECT_0)
#define WSA_WAIT_IO_COMPLETION  (WAIT_IO_COMPLETION)
#define WSA_WAIT_TIMEOUT        (WAIT_TIMEOUT)
#define WSA_INFINITE            (INFINITE)



#define IOCPARM_MASK    0x7f
#define IOC_VOID        0x20000000
#define IOC_OUT         0x40000000
#define IOC_IN          0x80000000
#define IOC_INOUT       (IOC_IN|IOC_OUT)

#define _IO(x, y)    (IOC_VOID|(x<<8)|y)

#define _IOR(x, y, t) (IOC_OUT|(((LONG)sizeof(t)&IOCPARM_MASK)<<16)|(x<<8)|y)

#define _IOW(x, y, t) (IOC_IN|(((LONG)sizeof(t)&IOCPARM_MASK)<<16)|(x<<8)|y)

#define FIONREAD    _IOR('f', 127, ULONG)
#define FIONBIO     _IOW('f', 126, ULONG)
#define FIOASYNC    _IOW('f', 125, ULONG)

/* Socket I/O controls */
#define SIOCSHIWAT  _IOW('s',  0, ULONG)
#define SIOCGHIWAT  _IOR('s',  1, ULONG)
#define SIOCSLOWAT  _IOW('s',  2, ULONG)
#define SIOCGLOWAT  _IOR('s',  3, ULONG)
#define SIOCATMARK  _IOR('s',  7, ULONG)


struct in_addr {
    union {
        struct { UCHAR s_b1, s_b2,s_b3,s_b4; } S_un_b;
        struct { USHORT s_w1,s_w2; } S_un_w;
        ULONG S_addr;
    } S_un;
#define s_addr  S_un.S_addr
#define s_host  S_un.S_un_b.s_b2
#define s_net   S_un.S_un_b.s_b1
#define s_imp   S_un.S_un_w.s_w2
#define s_impno S_un.S_un_b.s_b4
#define s_lh    S_un.S_un_b.s_b3
};


struct timeval {
    LONG tv_sec;
    LONG tv_usec;
};


struct sockaddr {
    USHORT sa_family;
    CHAR sa_data[14];
};


struct linger {
    USHORT	l_onoff;
    USHORT	l_linger;
};


/* IP specific */

struct sockaddr_in {
        SHORT   sin_family;
        USHORT  sin_port;
        struct  in_addr sin_addr;
        CHAR    sin_zero[8];
};

#define INADDR_ANY          ((ULONG)0x00000000)
#define INADDR_LOOPBACK     ((ULONG)0x7f000001)
#define INADDR_BROADCAST    ((ULONG)0xffffffff)
#define INADDR_NONE         ((ULONG)0xffffffff)

#define IPPROTO_IP      0
#define IPPROTO_ICMP    1
#define IPPROTO_IGMP    2
#define IPPROTO_GGP     3
#define IPPROTO_TCP     6
#define IPPROTO_PUP     12
#define IPPROTO_UDP     17
#define IPPROTO_IDP     22
#define IPPROTO_ND      77
#define IPPROTO_RAW     255
#define IPPROTO_MAX     256


#ifndef FD_SETSIZE
#define FD_SETSIZE  64
#endif /* FD_SETSIZE */

typedef struct fd_set {
    UINT fd_count;
    SOCKET fd_array[FD_SETSIZE];
} fd_set;

extern INT PASCAL FAR __WSAFDIsSet(SOCKET, fd_set FAR*);

#define FD_CLR(s, set) do { \
    UINT __i; \
    for (__i = 0; __i < ((fd_set FAR*)(set))->fd_count; __i++) { \
        if (((fd_set FAR*)(set))->fd_array[__i] == s) { \
            while (__i < ((fd_set FAR*)(set))->fd_count - 1) { \
                ((fd_set FAR *)(set))->fd_array[__i] = \
                    ((fd_set FAR*)(set))->fd_array[__i+1]; \
                __i++; \
            } \
            ((fd_set FAR *)(set))->fd_count--; \
            break; \
        } \
    } \
} while(0)

#define FD_SET(s, set) do { \
    UINT __i; \
    for (__i = 0; __i < ((fd_set FAR *)(set))->fd_count; __i++) { \
        if (((fd_set FAR *)(set))->fd_array[__i] == (s)) { \
            break; \
        } \
    } \
    if (__i == ((fd_set FAR *)(set))->fd_count) { \
        if (((fd_set FAR*)(set))->fd_count < FD_SETSIZE) { \
            ((fd_set FAR*)(set))->fd_array[__i] = (s); \
            ((fd_set FAR*)(set))->fd_count++; \
        } \
    } \
} while(0)

#define FD_ISSET(s, set) __WSAFDIsSet((SOCKET)(s), (fd_set FAR*)(set))

#define FD_ZERO(set) (((fd_set FAR*)(set))->fd_count = 0)


typedef struct _WSAOVERLAPPED {
    DWORD Internal;     // reserved
    DWORD InternalHigh; // reserved
    DWORD Offset;       // reserved
    DWORD OffsetHigh;   // reserved
    WSAEVENT hEvent;
} WSAOVERLAPPED, FAR* LPWSAOVERLAPPED;


typedef struct __WSABUF {
    ULONG len;     // buffer length
    CHAR FAR* buf; // poINTer to buffer
} WSABUF, FAR* LPWSABUF;


typedef ULONG  SERVICETYPE;

#define SERVICETYPE_NOTRAFFIC               0x00000000
#define SERVICETYPE_BESTEFFORT              0x00000001
#define SERVICETYPE_CONTROLLEDLOAD          0x00000002
#define SERVICETYPE_GUARANTEED              0x00000003
#define SERVICETYPE_NETWORK_UNAVAILABLE     0x00000004
#define SERVICETYPE_GENERAL_INFORMATION     0x00000005
#define SERVICETYPE_NOCHANGE                0x00000006
#define SERVICE_IMMEDIATE_TRAFFIC_CONTROL   0x00000007

typedef struct _flowspec {
    ULONG TokenRate;           /* In Bytes/sec */
    ULONG TokenBucketSize;     /* In Bytes */
    ULONG PeakBandwidth;       /* In Bytes/sec */
    ULONG Latency;             /* In microseconds */
    ULONG DelayVariation;      /* In microseconds */
    SERVICETYPE ServiceType;
    ULONG MaxSduSize;          /* In Bytes */
    ULONG MinimumPolicedSize;  /* In Bytes */
} FLOWSPEC, *PFLOWSPEC, FAR* LPFLOWSPEC;


typedef struct _QualityOfService {
    FLOWSPEC SendingFlowspec;   /* The flow spec for data sending */
    FLOWSPEC ReceivingFlowspec; /* The flow spec for data receiving */
    WSABUF ProviderSpecific;    /* Additional provider specific stuff */
} QOS, FAR* LPQOS;



typedef struct _WSANETWORKEVENTS {
    LONG lNetworkEvents;
    INT iErrorCode[FD_MAX_EVENTS];
} WSANETWORKEVENTS, FAR* LPWSANETWORKEVENTS;


#define MAX_PROTOCOL_CHAIN  7

#define BASE_PROTOCOL       1
#define LAYERED_PROTOCOL    0

typedef struct _WSAPROTOCOLCHAIN {
   INT ChainLen;
   DWORD ChainEntries[MAX_PROTOCOL_CHAIN];
} WSAPROTOCOLCHAIN, FAR* LPWSAPROTOCOLCHAIN;

#define WSAPROTOCOL_LEN 255

typedef struct _WSAPROTOCOL_INFOA {
    DWORD dwServiceFlags1;
    DWORD dwServiceFlags2;
    DWORD dwServiceFlags3;
    DWORD dwServiceFlags4;
    DWORD dwProviderFlags;
    GUID ProviderId;
    DWORD dwCatalogEntryId;
    WSAPROTOCOLCHAIN ProtocolChain;
    INT iVersion;
    INT iAddressFamily;
    INT iMaxSockAddr;
    INT iMinSockAddr;
    INT iSocketType;
    INT iProtocol;
    INT iProtocolMaxOffset;
    INT iNetworkByteOrder;
    INT iSecurityScheme;
    DWORD dwMessageSize;
    DWORD dwProviderReserved;
    CHAR szProtocol[WSAPROTOCOL_LEN + 1];
} WSAPROTOCOL_INFOA, FAR* LPWSAPROTOCOL_INFOA;

typedef struct _WSAPROTOCOL_INFOW {
    DWORD dwServiceFlags1;
    DWORD dwServiceFlags2;
    DWORD dwServiceFlags3;
    DWORD dwServiceFlags4;
    DWORD dwProviderFlags;
    GUID ProviderId;
    DWORD dwCatalogEntryId;
    WSAPROTOCOLCHAIN ProtocolChain;
    INT iVersion;
    INT iAddressFamily;
    INT iMaxSockAddr;
    INT iMinSockAddr;
    INT iSocketType;
    INT iProtocol;
    INT iProtocolMaxOffset;
    INT iNetworkByteOrder;
    INT iSecurityScheme;
    DWORD dwMessageSize;
    DWORD dwProviderReserved;
    WCHAR szProtocol[WSAPROTOCOL_LEN + 1];
} WSAPROTOCOL_INFOW, FAR * LPWSAPROTOCOL_INFOW;

#ifdef UNICODE
typedef WSAPROTOCOL_INFOW WSAPROTOCOL_INFO;
typedef LPWSAPROTOCOL_INFOW LPWSAPROTOCOL_INFO;
#else /* UNICODE */
typedef WSAPROTOCOL_INFOA WSAPROTOCOL_INFO;
typedef LPWSAPROTOCOL_INFOA LPWSAPROTOCOL_INFO;
#endif /* UNICODE */


/* WinSock 2 extended commands for WSAIoctl() */

#define IOC_UNIX    0x00000000
#define IOC_WS2     0x08000000
#define IOC_FAMILY  0x10000000
#define IOC_VENDOR  0x18000000

#define _WSAIO  (x,y) (IOC_VOID | (x) | (y))
#define _WSAIOR (x,y) (IOC_OUT  | (x) | (y))
#define _WSAIOW (x,y) (IOC_IN   | (x) | (y))
#define _WSAIORW(x,y) (IOC_INOUT| (x) | (y))

#define SIO_ASSOCIATE_HANDLE                _WSAIOW (IOC_WS2, 1)
#define SIO_ENABLE_CIRCULAR_QUEUEING        _WSAIO  (IOC_WS2, 2)
#define SIO_FIND_ROUTE                      _WSAIOR (IOC_WS2, 3)
#define SIO_FLUSH                           _WSAIO  (IOC_WS2, 4)
#define SIO_GET_BROADCAST_ADDRESS           -WSAIOR (IOC_WS2, 5)
#define SIO_GET_EXTENSION_FUNCTION_POINTER  _WSAIORW(IOC_WS2, 6)
#define SIO_GET_QOS                         _WSAIORW(IOC_WS2, 7)
#define SIO_GET_GROUP_QOS                   _WSAIORW(IOC_WS2, 8)
#define SIO_MULTIPOINT_LOOPBACK             _WSAIOW (IOC_WS2, 9)
#define SIO_MULTICAST_SCOPE                 _WSAIOW (IOC_WS2, 10)
#define SIO_SET_QOS                         _WSAIOW (IOC_WS2, 11)
#define SIO_SET_GROUP_QOS                   _WSAIOW (IOC_WS2, 12)
#define SIO_TRANSLATE_HANDLE                _WSAIORW(IOC_WS2, 13)
#define SIO_ROUTING_INTERFACE_QUERY         _WSAIORW(IOC_WS2, 20)
#define SIO_ROUTING_INTERFACE_CHANGE        _WSAIOW (IOC_WS2, 21)
#define SIO_ADDRESS_LIST_QUERY              _WSAIOR (IOC_WS2, 22)
#define SIO_ADDRESS_LIST_CHANGE             _WSAIO  (IOC_WS2, 23)
#define SIO_QUERY_TARGET_PNP_HANDLE         _WSAIOR (IOC_W32, 24)


/* Structures for WinSock 1.1 compatible namespace resolution */

struct hostent {
    CHAR FAR* h_name;
    CHAR FAR* FAR* h_aliases;
    SHORT h_addrtype;
    SHORT h_length;
    CHAR FAR* FAR* h_addr_list;
#define h_addr h_addr_list[0]   /* Backward compatible field name */
};

struct protoent {
    CHAR FAR* p_name;
    CHAR FAR* FAR* p_aliases;
    SHORT p_proto;
};

struct servent {
    CHAR FAR* s_name;
    CHAR FAR* FAR*	s_aliases;
    SHORT s_port;
    CHAR FAR* s_proto;
};


typedef UINT GROUP;

typedef
INT
(CALLBACK * LPCONDITIONPROC)(
    LPWSABUF lpCallerId,
    LPWSABUF lpCallerData,
    LPQOS lpSQOS,
    LPQOS lpGQOS,
    LPWSABUF lpCalleeId,
    LPWSABUF lpCalleeData,
    GROUP FAR* g,
    DWORD dwCallbackData);

typedef
VOID
(CALLBACK* LPWSAOVERLAPPED_COMPLETION_ROUTINE)(
    DWORD dwError,
    DWORD cbTransferred,
    LPWSAOVERLAPPED lpOverlapped,
    DWORD dwFlags);


/* Microsoft Windows extended data types */

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr *PSOCKADDR;
typedef struct sockaddr FAR *LPSOCKADDR;

typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr_in *PSOCKADDR_IN;
typedef struct sockaddr_in FAR *LPSOCKADDR_IN;

typedef struct linger LINGER;
typedef struct linger *PLINGER;
typedef struct linger FAR *LPLINGER;

typedef struct in_addr IN_ADDR;
typedef struct in_addr *PIN_ADDR;
typedef struct in_addr FAR *LPIN_ADDR;

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

#define WSAMAKEASYNCREPLY(buflen,error)     MAKELONG(buflen,error)
#define WSAMAKESELECTREPLY(event,error)     MAKELONG(event,error)
#define WSAGETASYNCBUFLEN(lParam)           LOWORD(lParam)
#define WSAGETASYNCERROR(lParam)            HIWORD(lParam)
#define WSAGETSELECTEVENT(lParam)           LOWORD(lParam)
#define WSAGETSELECTERROR(lParam)           HIWORD(lParam)


#define WSADESCRIPTION_LEN  256
#define WSASYS_STATUS_LEN   128

typedef struct WSAData {
    WORD wVersion;
    WORD wHighVersion;
    CHAR szDescription[WSADESCRIPTION_LEN + 1];
    CHAR szSystemStatus[WSASYS_STATUS_LEN + 1];
    USHORT iMaxSockets;
    USHORT iMaxUdpDg; 
    CHAR FAR* lpVendorInfo;
} WSADATA, FAR* LPWSADATA;

#if 0
typedef struct _BLOB {
    ULONG cbSize;
    BYTE *pBlobData;
} BLOB, *LPBLOB;
#endif
typedef BLOB *LPBLOB;

typedef struct _SOCKET_ADDRESS {
    LPSOCKADDR lpSockaddr;
    INT iSockaddrLength;
} SOCKET_ADDRESS, *PSOCKET_ADDRESS, FAR* LPSOCKET_ADDRESS;

typedef struct _SOCKET_ADDRESS_LIST {
    INT iAddressCount;
    SOCKET_ADDRESS Address[1];
} SOCKET_ADDRESS_LIST, FAR* LPSOCKET_ADDRESS_LIST;

typedef struct _CSADDR_INFO {
    SOCKET_ADDRESS LocalAddr;
    SOCKET_ADDRESS RemoteAddr;
    INT iSocketType;
    INT iProtocol;
} CSADDR_INFO, *PCSADDR_INFO, FAR* LPCSADDR_INFO;


/* Structures for namespace resolution */

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

#ifdef UNICODE
typedef WSANAMESPACE_INFOW WSANAMESPACE_INFO;
typedef PWSANAMESPACE_INFOW PWSANAMESPACE_INFO;
typedef LPWSANAMESPACE_INFOW LPWSANAMESPACE_INFO;
#else /* UNICODE */
typedef WSANAMESPACE_INFOA WSANAMESPACE_INFO;
typedef PWSANAMESPACE_INFOA PWSANAMESPACE_INFO;
typedef LPWSANAMESPACE_INFOA LPWSANAMESPACE_INFO;
#endif /* UNICODE */


typedef enum _WSAEcomparator
{
    COMP_EQUAL = 0,
    COMP_NOTLESS
} WSAECOMPARATOR, *PWSAECOMPARATOR, *LPWSAECOMPARATOR;

typedef struct _WSAVersion
{
    DWORD dwVersion;
    WSAECOMPARATOR ecHow;
}WSAVERSION, *PWSAVERSION, *LPWSAVERSION;


typedef struct _AFPROTOCOLS {
    INT iAddressFamily;
    INT iProtocol;
} AFPROTOCOLS, *PAFPROTOCOLS, *LPAFPROTOCOLS;


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

#ifdef UNICODE
typedef WSAQUERYSETW WSAQUERYSET;
typedef PWSAQUERYSETW PWSAQUERYSET;
typedef LPWSAQUERYSETW LPWSAQUERYSET;
#else /* UNICODE */
typedef WSAQUERYSETA WSAQUERYSET;
typedef PWSAQUERYSETA PWSAQUERYSET;
typedef LPWSAQUERYSETA LPWSAQUERYSET;
#endif /* UNICODE */


typedef enum _WSAESETSERVICEOP {
    RNRSERVICE_REGISTER = 0,
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
#else /* UNICODE */
typedef WSANSCLASSINFOA WSANSCLASSINFO;
typedef PWSANSCLASSINFOA PWSANSCLASSINFO;
typedef LPWSANSCLASSINFOA LPWSANSCLASSINFO;
#endif /* UNICODE */

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
#else /* UNICODE */
typedef WSASERVICECLASSINFOA WSASERVICECLASSINFO;
typedef PWSASERVICECLASSINFOA PWSASERVICECLASSINFO;
typedef LPWSASERVICECLASSINFOA LPWSASERVICECLASSINFO;
#endif /* UNICODE */


/* WinSock 2 DLL prototypes */

SOCKET
WSAAPI
accept(
    IN  SOCKET s,
    OUT LPSOCKADDR addr,
    OUT INT FAR* addrlen);

INT
WSAAPI
bind(
    IN  SOCKET s,
    IN  CONST LPSOCKADDR name,
    IN  INT namelen);

INT
WSAAPI
closesocket(
    IN  SOCKET s);

INT
WSAAPI
connect(
    IN  SOCKET s,
    IN  CONST LPSOCKADDR name,
    IN  INT namelen);

INT
WSAAPI
getpeername(
    IN      SOCKET s,
    OUT     LPSOCKADDR name,
    IN OUT  INT FAR* namelen);

INT
WSAAPI
getsockname(
    IN      SOCKET s,
    OUT     LPSOCKADDR name,
    IN OUT  INT FAR* namelen);

INT
WSAAPI
getsockopt(
    IN      SOCKET s,
    IN      INT level,
    IN      INT optname,
    OUT     CHAR FAR* optval,
    IN OUT  INT FAR* optlen);

ULONG
WSAAPI
htonl(
    IN  ULONG hostlong);

USHORT
WSAAPI
htons(
    IN  USHORT hostshort);

INT
WSAAPI
ioctlsocket(
    IN      SOCKET s,
    IN      LONG cmd,
    IN OUT  ULONG FAR* argp);

INT
WSAAPI
listen(
    IN  SOCKET s,
    IN  INT backlog);

ULONG
WSAAPI
ntohl(
    IN  ULONG netlong);

USHORT
WSAAPI
ntohs(
    IN  USHORT netshort);

INT
WSAAPI
recv(
    IN  SOCKET s,
    OUT CHAR FAR* buf,
    IN  INT len,
    IN  INT flags);

INT
WSAAPI
recvfrom(
    IN      SOCKET s,
    OUT     CHAR FAR* buf,
    IN      INT len,
    IN      INT flags,
    OUT     LPSOCKADDR from,
    IN OUT  INT FAR* fromlen);

INT
WSAAPI
select(
    IN      INT nfds, 
    IN OUT  LPFD_SET readfds, 
    IN OUT  LPFD_SET writefds, 
    IN OUT  LPFD_SET exceptfds, 
    IN      CONST LPTIMEVAL timeout);

INT
WSAAPI
send( 
    IN  SOCKET s, 
    IN  CONST CHAR FAR* buf, 
    IN  INT len, 
    IN  INT flags);

INT
WSAAPI
sendto(
    IN  SOCKET s,
    IN  CONST CHAR FAR* buf,
    IN  INT len,
    IN  INT flags,
    IN  CONST LPSOCKADDR to, 
    IN  INT tolen);

INT
WSAAPI
setsockopt(
    IN  SOCKET s,
    IN  INT level,
    IN  INT optname,
    IN  CONST CHAR FAR* optval,
    IN  INT optlen);

INT
WSAAPI
shutdown(
    IN  SOCKET s,
    IN  INT how);

SOCKET
WSAAPI
socket(
    IN  INT af,
    IN  INT type,
    IN  INT protocol);

SOCKET
WSAAPI
WSAAccept(
    IN      SOCKET s,
    OUT     LPSOCKADDR addr,
    IN OUT  LPINT addrlen,
    IN      LPCONDITIONPROC lpfnCondition,
    IN      DWORD dwCallbackData);

INT
WSAAPI
WSAAsyncSelect(
    IN  SOCKET s,
    IN  HWND hWnd,
    IN  UINT wMsg,
    IN  LONG lEvent);

INT
WSAAPI
WSACancelBlockingCall(VOID);

INT
WSAAPI
WSACleanup(VOID);

BOOL
WSAAPI
WSACloseEvent(
    IN  WSAEVENT hEvent);

INT
WSAAPI
WSAConnect(
    IN  SOCKET s,
    IN  CONST LPSOCKADDR name,
    IN  INT namelen,
    IN  LPWSABUF lpCallerData,
    OUT LPWSABUF lpCalleeData,
    IN  LPQOS lpSQOS,
    IN  LPQOS lpGQOS);

WSAEVENT
WSAAPI
WSACreateEvent(VOID);

INT
WSAAPI
WSADuplicateSocketA(
    IN  SOCKET s,
    IN  DWORD dwProcessId,
    OUT LPWSAPROTOCOL_INFOA lpProtocolInfo);

INT
WSAAPI
WSADuplicateSocketW(
    IN  SOCKET s,
    IN  DWORD dwProcessId,
    OUT LPWSAPROTOCOL_INFOW lpProtocolInfo);

#ifdef UNICODE
#define WSADuplicateSocket WSADuplicateSocketA
#else /* UNICODE */
#define WSADuplicateSocket WSADuplicateSocketW
#endif /* UNICODE */

INT
WSAAPI
WSAEnumNetworkEvents(
    IN  SOCKET s,
    IN  WSAEVENT hEventObject,
    OUT LPWSANETWORKEVENTS lpNetworkEvents);

INT
WSAAPI
WSAEnumProtocolsA(
    IN      LPINT lpiProtocols,
    OUT     LPWSAPROTOCOL_INFOA lpProtocolBuffer,
    IN OUT  LPDWORD lpdwBufferLength);

INT
WSAAPI
WSAEnumProtocolsW(
    IN      LPINT lpiProtocols,
    OUT     LPWSAPROTOCOL_INFOW lpProtocolBuffer,
    IN OUT  LPDWORD lpdwBufferLength);

#ifdef UNICODE
#define WSAEnumProtocols WSAEnumProtocolsA
#else /* UNICODE */
#define WSAEnumProtocols WSAEnumProtocolsW
#endif /* UNICODE */

INT
WSAAPI
WSAEventSelect(
    IN  SOCKET s,
    IN  WSAEVENT hEventObject,
    IN  LONG lNetworkEvents);

INT
WSAAPI
WSAGetLastError(VOID);

BOOL
WSAAPI
WSAGetOverlappedResult(
    IN  SOCKET s,
    IN  LPWSAOVERLAPPED lpOverlapped,
    OUT LPDWORD lpcbTransfer,
    IN  BOOL fWait,
    OUT LPDWORD lpdwFlags);

BOOL
WSAAPI
WSAGetQOSByName(
    IN      SOCKET s, 
    IN OUT  LPWSABUF lpQOSName, 
    OUT     LPQOS lpQOS);

INT
WSAAPI
WSAHtonl(
    IN  SOCKET s,
    IN  ULONG hostlong,
    OUT ULONG FAR* lpnetlong);

INT
WSAAPI
WSAHtons(
    IN  SOCKET s,
    IN  USHORT hostshort,
    OUT USHORT FAR* lpnetshort);

INT
WSAAPI
WSAIoctl(
    IN  SOCKET s,
    IN  DWORD dwIoControlCode,
    IN  LPVOID lpvInBuffer,
    IN  DWORD cbInBuffer,
    OUT LPVOID lpvOutBuffer,
    IN  DWORD cbOutBuffer,
    OUT LPDWORD lpcbBytesReturned,
    IN  LPWSAOVERLAPPED lpOverlapped,
    IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

BOOL
WSAAPI
WSAIsBlocking(VOID);

SOCKET
WSAAPI
WSAJoinLeaf(
    IN  SOCKET s,
    IN  CONST LPSOCKADDR name,
    IN  INT namelen,
    IN  LPWSABUF lpCallerData,
    OUT LPWSABUF lpCalleeData,
    IN  LPQOS lpSQOS,
    IN  LPQOS lpGQOS,
    IN  DWORD dwFlags);

INT
WSAAPI
WSANtohl(
    IN  SOCKET s,
    IN  ULONG netlong,
    OUT ULONG FAR* lphostlong);

INT
WSAAPI
WSANtohs(
    IN  SOCKET s,
    IN  USHORT netshort,
    OUT USHORT FAR* lphostshort);

INT
WSAAPI
WSARecv(
    IN      SOCKET s,
    IN OUT  LPWSABUF lpBuffers,
    IN      DWORD dwBufferCount,
    OUT     LPDWORD lpNumberOfBytesRecvd,
    IN OUT  LPDWORD lpFlags,
    IN      LPWSAOVERLAPPED lpOverlapped,
    IN      LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

INT
WSAAPI
WSARecvDisconnect(
    IN  SOCKET s,
    OUT LPWSABUF lpInboundDisconnectData);

INT
WSAAPI
WSARecvFrom(
    IN      SOCKET s,
    IN OUT  LPWSABUF lpBuffers,
    IN      DWORD  dwBufferCount,
    OUT     LPDWORD  lpNumberOfBytesRecvd,
    IN OUT  LPDWORD lpFlags,
    OUT	    LPSOCKADDR lpFrom,
    IN OUT  LPINT lpFromlen,
    IN      LPWSAOVERLAPPED lpOverlapped,
    IN      LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

BOOL
WSAAPI
WSAResetEvent(
    IN  WSAEVENT hEvent);

INT
WSAAPI
WSASend(
    IN  SOCKET s,
    IN  LPWSABUF lpBuffers,
    IN  DWORD dwBufferCount,
    OUT LPDWORD lpNumberOfBytesSent,
    IN  DWORD dwFlags,
    IN  LPWSAOVERLAPPED lpOverlapped,
    IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

INT
WSAAPI
WSASendDisconnect(
    IN  SOCKET s,
    IN  LPWSABUF lpOutboundDisconnectData);

INT
WSAAPI
WSASendTo(
    IN  SOCKET s,
    IN  LPWSABUF lpBuffers,
    IN  DWORD dwBufferCount,
    OUT LPDWORD lpNumberOfBytesSent,
    IN  DWORD dwFlags,
    IN  CONST LPSOCKADDR lpTo,
    IN  INT iToLen,
    IN  LPWSAOVERLAPPED lpOverlapped,
    IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

FARPROC
WSAAPI
WSASetBlockingHook(
    IN  FARPROC lpBlockFunc);

BOOL
WSAAPI
WSASetEvent(
    IN  WSAEVENT hEvent);

VOID
WSAAPI
WSASetLastError(
    IN  INT iError);

SOCKET
WSAAPI
WSASocketA(
    IN  INT af,
    IN  INT type,
    IN  INT protocol,
    IN  LPWSAPROTOCOL_INFOA lpProtocolInfo,
    IN  GROUP g,
    IN  DWORD dwFlags);

SOCKET
WSAAPI
WSASocketW(
    IN  INT af,
    IN  INT type,
    IN  INT protocol,
    IN  LPWSAPROTOCOL_INFOW lpProtocolInfo,
    IN  GROUP g,
    IN  DWORD dwFlags);

#ifdef UNICODE
#define WSASocket WSASocketW
#else /* UNICODE */
#define WSASocket WSASocketA
#endif /* UNICODE */

INT
WSAAPI
WSAStartup(
    IN  WORD wVersionRequested,
    OUT LPWSADATA lpWSAData);

INT
WSAAPI
WSAUnhookBlockingHook(VOID);

DWORD
WSAAPI
WSAWaitForMultipleEvents(
    IN  DWORD cEvents,
    IN  CONST WSAEVENT FAR* lphEvents,
    IN  BOOL fWaitAll,
    IN  DWORD dwTimeout,
    IN  BOOL fAlertable);

INT
WSAAPI
WSAProviderConfigChange(
    IN OUT  LPHANDLE lpNotificationHandle,
    IN      LPWSAOVERLAPPED lpOverlapped,
    IN      LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

/* Name resolution APIs */

INT
WSAAPI
WSAAddressToStringA(
    IN      LPSOCKADDR lpsaAddress,
    IN      DWORD dwAddressLength,
    IN      LPWSAPROTOCOL_INFOA lpProtocolInfo,
    OUT     LPSTR lpszAddressString,
    IN OUT  LPDWORD lpdwAddressStringLength);

INT
WSAAPI
WSAAddressToStringW(
    IN      LPSOCKADDR lpsaAddress,
    IN      DWORD dwAddressLength,
    IN      LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT     LPWSTR lpszAddressString,
    IN OUT  LPDWORD lpdwAddressStringLength);

#ifdef UNICODE
#define WSAAddressToString WSAAddressToStringW
#else /* UNICODE */
#define WSAAddressToString WSAAddressToStringA
#endif /* UNICODE */

INT
WSAAPI
WSAEnumNameSpaceProvidersA(
    IN OUT  LPDWORD lpdwBufferLength,
    OUT     LPWSANAMESPACE_INFOA lpnspBuffer);

INT
WSAAPI
WSAEnumNameSpaceProvidersW(
    IN OUT  LPDWORD lpdwBufferLength,
    OUT     LPWSANAMESPACE_INFOW lpnspBuffer);

#ifdef UNICODE
#define WSAEnumNameSpaceProviders WSAEnumNameSpaceProvidersW
#else /* UNICODE */
#define WSAEnumNameSpaceProviders WSAEnumNameSpaceProvidersA
#endif /* UNICODE */

INT
WSAAPI
WSAGetServiceClassInfoA(
    IN      LPGUID lpProviderId,
    IN      LPGUID lpServiceClassId,
    IN OUT  LPDWORD lpdwBufferLength,
    OUT     LPWSASERVICECLASSINFOA lpServiceClassInfo);

INT
WSAAPI
WSAGetServiceClassInfoW(
    IN      LPGUID lpProviderId,
    IN      LPGUID lpServiceClassId,
    IN OUT  LPDWORD lpdwBufferLength,
    OUT     LPWSASERVICECLASSINFOW lpServiceClassInfo);

#ifdef UNICODE
#define WSAGetServiceClassInfo WSAGetServiceClassInfoW
#else /* UNICODE */
#define WSAGetServiceClassInfo WSAGetServiceClassInfoA
#endif /* UNICODE */

INT
WSAAPI
WSAGetServiceClassNameByClassIdA(
    IN      LPGUID  lpServiceClassId,
    OUT     LPSTR   lpszServiceClassName,
    IN OUT  LPDWORD lpdwBufferLength);

INT
WSAAPI
WSAGetServiceClassNameByClassIdW(
    IN      LPGUID  lpServiceClassId,
    OUT     LPWSTR  lpszServiceClassName,
    IN OUT  LPDWORD lpdwBufferLength);

#ifdef UNICODE
#define WSAGetServiceClassNameByClassId WSAGetServiceClassNameByClassIdW
#else /* UNICODE */
#define WSAGetServiceClassNameByClassId WSAGetServiceClassNameByClassIdA
#endif /* UNICODE */

INT
WSAAPI
WSAInstallServiceClassA(
    IN  LPWSASERVICECLASSINFOA lpServiceClassInfo);

INT
WSAAPI
WSAInstallServiceClassW(
    IN  LPWSASERVICECLASSINFOW lpServiceClassInfo);

#ifdef UNICODE
#define WSAInstallServiceClass WSAInstallServiceClassW
#else /* UNICODE */
#define WSAInstallServiceClass WSAInstallServiceClassA
#endif /* UNICODE */

INT
WSAAPI
WSALookupServiceBegin(
    IN  LPWSAQUERYSET lpqsRestrictions,
    IN  DWORD dwControlFlags,
    OUT LPHANDLE lphLookup);

INT
WSAAPI
WSALookupServiceEnd(
    IN  HANDLE hLookup);

INT
WSAAPI
WSALookupServiceNext(
    IN      HANDLE hLookup,
    IN      DWORD dwControlFlags,
    IN OUT  LPDWORD lpdwBufferLength,
    OUT     LPWSAQUERYSET lpqsResults);

INT
WSAAPI
WSARemoveServiceClass(
    IN  LPGUID lpServiceClassId);

INT
WSAAPI
WSASetService(
    IN  LPWSAQUERYSET lpqsRegInfo,
    IN  WSAESETSERVICEOP essOperation,
    IN  DWORD dwControlFlags);

INT
WSAAPI
WSAStringToAddressA(
    IN      LPSTR AddressString,
    IN      INT AddressFamily,
    IN      LPWSAPROTOCOL_INFOA lpProtocolInfo,
    OUT     LPSOCKADDR lpAddress,
    IN OUT  LPINT lpAddressLength);

INT
WSAAPI
WSAStringToAddressW(
    IN      LPWSTR AddressString,
    IN      INT AddressFamily,
    IN      LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT     LPSOCKADDR lpAddress,
    IN OUT  LPINT lpAddressLength);

#ifdef UNICODE
#define WSAStringToAddress WSAStringToAddressW
#else /* UNICODE */
#define WSAStringToAddress WSAStringToAddressA
#endif /* UNICODE */

/* WinSock 1.1 compatible name resolution APIs */

LPHOSTENT
WSAAPI
gethostbyaddr(
    IN  CONST CHAR FAR* addr,
    IN  INT len,
    IN  INT type);

LPHOSTENT
WSAAPI
gethostbyname(
    IN  CONST CHAR FAR* name);

INT
WSAAPI
gethostname(
    OUT CHAR FAR* name,
    IN  INT namelen);

LPPROTOENT
WSAAPI
getprotobyname(
    IN  CONST CHAR FAR* name);

LPPROTOENT
WSAAPI
getprotobynumber(
    IN  INT number);

LPSERVENT
WSAAPI
getservbyname(
    IN  CONST CHAR FAR* name, 
    IN  CONST CHAR FAR* proto);

LPSERVENT
WSAAPI
getservbyport(
    IN  INT port, 
    IN  CONST CHAR FAR* proto);

ULONG
WSAAPI
inet_addr(
    IN  CONST CHAR FAR* cp);

CHAR FAR*
WSAAPI
inet_ntoa(
    IN  IN_ADDR in);

HANDLE
WSAAPI
WSAAsyncGetHostByAddr(
    IN  HWND hWnd,
    IN  UINT wMsg,
    IN  CONST CHAR FAR* addr, 
    IN  INT len,
    IN  INT type, 
    OUT CHAR FAR* buf, 
    IN  INT buflen);

HANDLE
WSAAPI
WSAAsyncGetHostByName(
    IN  HWND hWnd, 
    IN  UINT wMsg,  
    IN  CONST CHAR FAR* name, 
    OUT CHAR FAR* buf, 
    IN  INT buflen);

HANDLE
WSAAPI
WSAAsyncGetProtoByName(
    IN  HWND hWnd,
    IN  UINT wMsg,
    IN  CONST CHAR FAR* name,
    OUT CHAR FAR* buf,
    IN  INT buflen);

HANDLE
WSAAPI
WSAAsyncGetProtoByNumber(
    IN  HWND hWnd,
    IN  UINT wMsg,
    IN  INT number,
    OUT CHAR FAR* buf,
    IN  INT buflen);

HANDLE
WSAAPI
WSAAsyncGetServByName(
    IN  HWND hWnd,
    IN  UINT wMsg,
    IN  CONST CHAR FAR* name,
    IN  CONST CHAR FAR* proto,
    OUT CHAR FAR* buf,
    IN  INT buflen);

HANDLE
WSAAPI
WSAAsyncGetServByPort(
    IN  HWND hWnd,
    IN  UINT wMsg,
    IN  INT port,
    IN  CONST CHAR FAR* proto,
    OUT CHAR FAR* buf,
    IN  INT buflen);

INT
WSAAPI
WSACancelAsyncRequest(
    IN  HANDLE hAsyncTaskHandle);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* __WINSOCK2_H */

/* EOF */
