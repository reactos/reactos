#pragma once

#define _WS2DEF_

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_WINSOCK2API_) && defined(_WINSOCKAPI_)
#error Include only winsock2.h, not both winsock.h and ws2def.h in the same module.
#endif

#if (_WIN32_WINNT >= 0x0600)

#ifdef _MSC_VER
#define WS2DEF_INLINE __inline
#else
#define WS2DEF_INLINE extern inline
#endif

#endif/* (_WIN32_WINNT >= 0x0600) */

#include <inaddr.h>

typedef USHORT ADDRESS_FAMILY;

#define AF_UNSPEC       0
#define AF_UNIX         1
#define AF_INET         2
#define AF_IMPLINK      3
#define AF_PUP          4
#define AF_CHAOS        5
#define AF_NS           6
#define AF_IPX          AF_NS
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
#define AF_VOICEVIEW    18
#define AF_FIREFOX      19
#define AF_UNKNOWN1     20
#define AF_BAN          21
#define AF_ATM          22
#define AF_INET6        23
#define AF_CLUSTER      24
#define AF_12844        25
#define AF_IRDA         26
#define AF_NETDES       28

#if (_WIN32_WINNT < 0x0501)

#define AF_MAX          29

#else

#define AF_TCNPROCESS   29
#define AF_TCNMESSAGE   30
#define AF_ICLFXBM      31

#if(_WIN32_WINNT < 0x0600)

#define AF_MAX          32

#else

#define AF_BTH          32
#if (_WIN32_WINNT < 0x0601)
#define AF_MAX          33
#else
#define AF_LINK         33
#define AF_MAX          34
#endif /* (_WIN32_WINNT < 0x0601) */

#endif /* (_WIN32_WINNT < 0x0600) */

#endif /* (_WIN32_WINNT < 0x0501) */

#define SOCK_STREAM     1
#define SOCK_DGRAM      2
#define SOCK_RAW        3
#define SOCK_RDM        4
#define SOCK_SEQPACKET  5

#define SOL_SOCKET      0xffff

#define SO_DEBUG        0x0001
#define SO_ACCEPTCONN   0x0002
#define SO_REUSEADDR    0x0004
#define SO_KEEPALIVE    0x0008
#define SO_DONTROUTE    0x0010
#define SO_BROADCAST    0x0020
#define SO_USELOOPBACK  0x0040
#define SO_LINGER       0x0080
#define SO_OOBINLINE    0x0100

#define SO_DONTLINGER (int)(~SO_LINGER)
#define SO_EXCLUSIVEADDRUSE ((int)(~SO_REUSEADDR))

#define SO_SNDBUF       0x1001
#define SO_RCVBUF       0x1002
#define SO_SNDLOWAT     0x1003
#define SO_RCVLOWAT     0x1004
#define SO_SNDTIMEO     0x1005
#define SO_RCVTIMEO     0x1006
#define SO_ERROR        0x1007
#define SO_TYPE         0x1008
#define SO_BSP_STATE    0x1009

#define SO_GROUP_ID          0x2001
#define SO_GROUP_PRIORITY    0x2002
#define SO_MAX_MSG_SIZE      0x2003

#define SO_CONDITIONAL_ACCEPT 0x3002
#define SO_PAUSE_ACCEPT       0x3003
#define SO_COMPARTMENT_ID     0x3004
#if (_WIN32_WINNT >= 0x0600)
#define SO_RANDOMIZE_PORT     0x3005
#define SO_PORT_SCALABILITY   0x3006
#endif /* (_WIN32_WINNT >= 0x0600) */

#define WSK_SO_BASE           0x4000

#define TCP_NODELAY           0x0001

#define _SS_MAXSIZE           128
#define _SS_ALIGNSIZE         (sizeof(__int64))

#if (_WIN32_WINNT >= 0x0600)

#define _SS_PAD1SIZE (_SS_ALIGNSIZE - sizeof(USHORT))
#define _SS_PAD2SIZE (_SS_MAXSIZE - (sizeof(USHORT) + _SS_PAD1SIZE + _SS_ALIGNSIZE))

#else

#define _SS_PAD1SIZE (_SS_ALIGNSIZE - sizeof (short))
#define _SS_PAD2SIZE (_SS_MAXSIZE - (sizeof (short) + _SS_PAD1SIZE + _SS_ALIGNSIZE))

#endif /* (_WIN32_WINNT >= 0x0600) */

#define IOC_UNIX                       0x00000000
#define IOC_WS2                        0x08000000
#define IOC_PROTOCOL                   0x10000000
#define IOC_VENDOR                     0x18000000

#if (_WIN32_WINNT >= 0x0600)
#define IOC_WSK                        (IOC_WS2|0x07000000)
#endif

#define _WSAIO(x,y)                    (IOC_VOID|(x)|(y))
#define _WSAIOR(x,y)                   (IOC_OUT|(x)|(y))
#define _WSAIOW(x,y)                   (IOC_IN|(x)|(y))
#define _WSAIORW(x,y)                  (IOC_INOUT|(x)|(y))

#define SIO_ASSOCIATE_HANDLE                _WSAIOW(IOC_WS2,1)
#define SIO_ENABLE_CIRCULAR_QUEUEING        _WSAIO(IOC_WS2,2)
#define SIO_FIND_ROUTE                      _WSAIOR(IOC_WS2,3)
#define SIO_FLUSH                           _WSAIO(IOC_WS2,4)
#define SIO_GET_BROADCAST_ADDRESS           _WSAIOR(IOC_WS2,5)
#define SIO_GET_EXTENSION_FUNCTION_POINTER  _WSAIORW(IOC_WS2,6)
#define SIO_GET_QOS                         _WSAIORW(IOC_WS2,7)
#define SIO_GET_GROUP_QOS                   _WSAIORW(IOC_WS2,8)
#define SIO_MULTIPOINT_LOOPBACK             _WSAIOW(IOC_WS2,9)
#define SIO_MULTICAST_SCOPE                 _WSAIOW(IOC_WS2,10)
#define SIO_SET_QOS                         _WSAIOW(IOC_WS2,11)
#define SIO_SET_GROUP_QOS                   _WSAIOW(IOC_WS2,12)
#define SIO_TRANSLATE_HANDLE                _WSAIORW(IOC_WS2,13)
#define SIO_ROUTING_INTERFACE_QUERY         _WSAIORW(IOC_WS2,20)
#define SIO_ROUTING_INTERFACE_CHANGE        _WSAIOW(IOC_WS2,21)
#define SIO_ADDRESS_LIST_QUERY              _WSAIOR(IOC_WS2,22)
#define SIO_ADDRESS_LIST_CHANGE             _WSAIO(IOC_WS2,23)
#define SIO_QUERY_TARGET_PNP_HANDLE         _WSAIOR(IOC_WS2,24)

#if(_WIN32_WINNT >= 0x0501)
#define SIO_ADDRESS_LIST_SORT               _WSAIORW(IOC_WS2,25)
#endif

#if (_WIN32_WINNT >= 0x0600)
#define SIO_RESERVED_1                      _WSAIOW(IOC_WS2,26)
#define SIO_RESERVED_2                      _WSAIOW(IOC_WS2,33)
#endif

#define IPPROTO_IP                          0

#define IPPORT_TCPMUX           1
#define IPPORT_ECHO             7
#define IPPORT_DISCARD          9
#define IPPORT_SYSTAT           11
#define IPPORT_DAYTIME          13
#define IPPORT_NETSTAT          15
#define IPPORT_QOTD             17
#define IPPORT_MSP              18
#define IPPORT_CHARGEN          19
#define IPPORT_FTP_DATA         20
#define IPPORT_FTP              21
#define IPPORT_TELNET           23
#define IPPORT_SMTP             25
#define IPPORT_TIMESERVER       37
#define IPPORT_NAMESERVER       42
#define IPPORT_WHOIS            43
#define IPPORT_MTP              57

#define IPPORT_TFTP             69
#define IPPORT_RJE              77
#define IPPORT_FINGER           79
#define IPPORT_TTYLINK          87
#define IPPORT_SUPDUP           95

#define IPPORT_POP3             110
#define IPPORT_NTP              123
#define IPPORT_EPMAP            135
#define IPPORT_NETBIOS_NS       137
#define IPPORT_NETBIOS_DGM      138
#define IPPORT_NETBIOS_SSN      139
#define IPPORT_IMAP             143
#define IPPORT_SNMP             161
#define IPPORT_SNMP_TRAP        162
#define IPPORT_IMAP3            220
#define IPPORT_LDAP             389
#define IPPORT_HTTPS            443
#define IPPORT_MICROSOFT_DS     445
#define IPPORT_EXECSERVER       512
#define IPPORT_LOGINSERVER      513
#define IPPORT_CMDSERVER        514
#define IPPORT_EFSSERVER        520

#define IPPORT_BIFFUDP          512
#define IPPORT_WHOSERVER        513
#define IPPORT_ROUTESERVER      520
#define IPPORT_RESERVED         1024

#if (_WIN32_WINNT >= 0x0600)

#define IPPORT_REGISTERED_MIN   IPPORT_RESERVED
#define IPPORT_REGISTERED_MAX   0xbfff
#define IPPORT_DYNAMIC_MIN      0xc000
#define IPPORT_DYNAMIC_MAX      0xffff

#endif /* (_WIN32_WINNT >= 0x0600) */

#define IN_CLASSA(i)            (((LONG)(i) & 0x80000000) == 0)
#define IN_CLASSA_NET           0xff000000
#define IN_CLASSA_NSHIFT        24
#define IN_CLASSA_HOST          0x00ffffff
#define IN_CLASSA_MAX           128

#define IN_CLASSB(i)            (((LONG)(i) & 0xc0000000) == 0x80000000)
#define IN_CLASSB_NET           0xffff0000
#define IN_CLASSB_NSHIFT        16
#define IN_CLASSB_HOST          0x0000ffff
#define IN_CLASSB_MAX           65536

#define IN_CLASSC(i)            (((LONG)(i) & 0xe0000000) == 0xc0000000)
#define IN_CLASSC_NET           0xffffff00
#define IN_CLASSC_NSHIFT        8
#define IN_CLASSC_HOST          0x000000ff

#define IN_CLASSD(i)            (((long)(i) & 0xf0000000) == 0xe0000000)
#define IN_CLASSD_NET           0xf0000000
#define IN_CLASSD_NSHIFT        28
#define IN_CLASSD_HOST          0x0fffffff
#define IN_MULTICAST(i)         IN_CLASSD(i)

#define INADDR_ANY              (ULONG)0x00000000
#define INADDR_LOOPBACK         0x7f000001
#define INADDR_BROADCAST        (ULONG)0xffffffff
#define INADDR_NONE             0xffffffff

#define SCOPEID_UNSPECIFIED_INIT    {0}

#define IOCPARM_MASK    0x7f
#define IOC_VOID        0x20000000
#define IOC_OUT         0x40000000
#define IOC_IN          0x80000000
#define IOC_INOUT       (IOC_IN|IOC_OUT)

#define _IO(x,y)        (IOC_VOID|((x)<<8)|(y))
#define _IOR(x,y,t)     (IOC_OUT|(((long)sizeof(t)&IOCPARM_MASK)<<16)|((x)<<8)|(y))
#define _IOW(x,y,t)     (IOC_IN|(((long)sizeof(t)&IOCPARM_MASK)<<16)|((x)<<8)|(y))

#define MSG_TRUNC       0x0100
#define MSG_CTRUNC      0x0200
#define MSG_BCAST       0x0400
#define MSG_MCAST       0x0800

#define AI_PASSIVE                  0x00000001
#define AI_CANONNAME                0x00000002
#define AI_NUMERICHOST              0x00000004
#define AI_NUMERICSERV              0x00000008

#define AI_ALL                      0x00000100
#define AI_ADDRCONFIG               0x00000400
#define AI_V4MAPPED                 0x00000800

#define AI_NON_AUTHORITATIVE        0x00004000
#define AI_SECURE                   0x00008000
#define AI_RETURN_PREFERRED_NAMES   0x00010000

#define AI_FQDN                     0x00020000
#define AI_FILESERVER               0x00040000

#define AI_DISABLE_IDN_ENCODING 0x00080000

#define NS_ALL                      0

#define NS_SAP                      1
#define NS_NDS                      2
#define NS_PEER_BROWSE              3
#define NS_SLP                      5
#define NS_DHCP                     6

#define NS_TCPIP_LOCAL              10
#define NS_TCPIP_HOSTS              11
#define NS_DNS                      12
#define NS_NETBT                    13
#define NS_WINS                     14

#if(_WIN32_WINNT >= 0x0501)
#define NS_NLA                      15
#endif

#if(_WIN32_WINNT >= 0x0600)
#define NS_BTH                      16
#endif

#define NS_NBP                      20

#define NS_MS                       30
#define NS_STDA                     31
#define NS_NTDS                     32

#if(_WIN32_WINNT >= 0x0600)
#define NS_EMAIL                    37
#define NS_PNRPNAME                 38
#define NS_PNRPCLOUD                39
#endif

#define NS_X500                     40
#define NS_NIS                      41
#define NS_NISPLUS                  42

#define NS_WRQ                      50

#define NS_NETDES                   60

#define NI_NOFQDN       0x01
#define NI_NUMERICHOST  0x02
#define NI_NAMEREQD     0x04
#define NI_NUMERICSERV  0x08
#define NI_DGRAM        0x10

#define NI_MAXHOST      1025
#define NI_MAXSERV      32

typedef struct sockaddr {
#if (_WIN32_WINNT < 0x0600)
  u_short sa_family;
#else
  ADDRESS_FAMILY sa_family;
#endif
  CHAR sa_data[14];
} SOCKADDR, *PSOCKADDR, FAR *LPSOCKADDR;

#ifndef __CSADDR_DEFINED__
#define __CSADDR_DEFINED__

typedef struct _SOCKET_ADDRESS {
  LPSOCKADDR lpSockaddr;
  INT iSockaddrLength;
} SOCKET_ADDRESS, *PSOCKET_ADDRESS, *LPSOCKET_ADDRESS;

typedef struct _SOCKET_ADDRESS_LIST {
  INT iAddressCount;
  SOCKET_ADDRESS Address[1];
} SOCKET_ADDRESS_LIST, *PSOCKET_ADDRESS_LIST, FAR *LPSOCKET_ADDRESS_LIST;

#if (_WIN32_WINNT >= 0x0600)
#define SIZEOF_SOCKET_ADDRESS_LIST(AddressCount) \
    (FIELD_OFFSET(SOCKET_ADDRESS_LIST, Address) + \
     AddressCount * sizeof(SOCKET_ADDRESS))
#endif

typedef struct _CSADDR_INFO {
  SOCKET_ADDRESS LocalAddr;
  SOCKET_ADDRESS RemoteAddr;
  INT iSocketType;
  INT iProtocol;
} CSADDR_INFO, *PCSADDR_INFO, FAR *LPCSADDR_INFO ;

#endif /* __CSADDR_DEFINED__ */

typedef struct sockaddr_storage {
  ADDRESS_FAMILY ss_family;
  CHAR __ss_pad1[_SS_PAD1SIZE];
  __int64 __ss_align;
  CHAR __ss_pad2[_SS_PAD2SIZE];
} SOCKADDR_STORAGE_LH, *PSOCKADDR_STORAGE_LH, FAR *LPSOCKADDR_STORAGE_LH;

typedef struct sockaddr_storage_xp {
  short ss_family;
  CHAR __ss_pad1[_SS_PAD1SIZE];
  __int64 __ss_align;
  CHAR __ss_pad2[_SS_PAD2SIZE];
} SOCKADDR_STORAGE_XP, *PSOCKADDR_STORAGE_XP, FAR *LPSOCKADDR_STORAGE_XP;

#if (_WIN32_WINNT >= 0x0600)

typedef SOCKADDR_STORAGE_LH SOCKADDR_STORAGE;
typedef SOCKADDR_STORAGE *PSOCKADDR_STORAGE, FAR *LPSOCKADDR_STORAGE;

#elif (_WIN32_WINNT >= 0x0501)

typedef SOCKADDR_STORAGE_XP SOCKADDR_STORAGE;
typedef SOCKADDR_STORAGE *PSOCKADDR_STORAGE, FAR *LPSOCKADDR_STORAGE;

#endif /* (_WIN32_WINNT >= 0x0600) */

typedef enum {
#if (_WIN32_WINNT >= 0x0501)
  IPPROTO_HOPOPTS = 0,
#endif
  IPPROTO_ICMP = 1,
  IPPROTO_IGMP = 2,
  IPPROTO_GGP = 3,
#if (_WIN32_WINNT >= 0x0501)
  IPPROTO_IPV4 = 4,
#endif
#if (_WIN32_WINNT >= 0x0600)
  IPPROTO_ST = 5,
#endif
  IPPROTO_TCP = 6,
#if (_WIN32_WINNT >= 0x0600)
  IPPROTO_CBT = 7,
  IPPROTO_EGP = 8,
  IPPROTO_IGP = 9,
#endif
  IPPROTO_PUP = 12,
  IPPROTO_UDP = 17,
  IPPROTO_IDP = 22,
#if (_WIN32_WINNT >= 0x0600)
  IPPROTO_RDP = 27,
#endif
#if (_WIN32_WINNT >= 0x0501)
  IPPROTO_IPV6 = 41,
  IPPROTO_ROUTING = 43,
  IPPROTO_FRAGMENT = 44,
  IPPROTO_ESP = 50,
  IPPROTO_AH = 51,
  IPPROTO_ICMPV6 = 58,
  IPPROTO_NONE = 59,
  IPPROTO_DSTOPTS = 60,
#endif /* (_WIN32_WINNT >= 0x0501) */
  IPPROTO_ND = 77,
#if(_WIN32_WINNT >= 0x0501)
  IPPROTO_ICLFXBM = 78,
#endif
#if (_WIN32_WINNT >= 0x0600)
  IPPROTO_PIM = 103,
  IPPROTO_PGM = 113,
  IPPROTO_L2TP = 115,
  IPPROTO_SCTP = 132,
#endif /* (_WIN32_WINNT >= 0x0600) */
  IPPROTO_RAW = 255,
  IPPROTO_MAX = 256,
  IPPROTO_RESERVED_RAW = 257,
  IPPROTO_RESERVED_IPSEC = 258,
  IPPROTO_RESERVED_IPSECOFFLOAD = 259,
  IPPROTO_RESERVED_MAX = 260
} IPPROTO, *PIPROTO;

typedef enum {
  ScopeLevelInterface = 1,
  ScopeLevelLink = 2,
  ScopeLevelSubnet = 3,
  ScopeLevelAdmin = 4,
  ScopeLevelSite = 5,
  ScopeLevelOrganization = 8,
  ScopeLevelGlobal = 14,
  ScopeLevelCount = 16
} SCOPE_LEVEL;

typedef struct {
  union {
    struct {
      ULONG Zone:28;
      ULONG Level:4;
    };
    ULONG Value;
  };
} SCOPE_ID, *PSCOPE_ID;

typedef struct sockaddr_in {
#if(_WIN32_WINNT < 0x0600)
  short sin_family;
#else
  ADDRESS_FAMILY sin_family;
#endif
  USHORT sin_port;
  IN_ADDR sin_addr;
  CHAR sin_zero[8];
} SOCKADDR_IN, *PSOCKADDR_IN;

#if(_WIN32_WINNT >= 0x0601)
typedef struct sockaddr_dl {
  ADDRESS_FAMILY sdl_family;
  UCHAR sdl_data[8];
  UCHAR sdl_zero[4];
} SOCKADDR_DL, *PSOCKADDR_DL;
#endif

typedef struct _WSABUF {
  ULONG len;
  CHAR FAR *buf;
} WSABUF, FAR * LPWSABUF;

typedef struct _WSAMSG {
  LPSOCKADDR name;
  INT namelen;
  LPWSABUF lpBuffers;
#if (_WIN32_WINNT >= 0x0600)
  ULONG dwBufferCount;
#else
  DWORD dwBufferCount;
#endif
  WSABUF Control;
#if (_WIN32_WINNT >= 0x0600)
  ULONG dwFlags;
#else
  DWORD dwFlags;
#endif
} WSAMSG, *PWSAMSG, *FAR LPWSAMSG;

#if (_WIN32_WINNT >= 0x0600)
#define _WSACMSGHDR cmsghdr
#endif

typedef struct _WSACMSGHDR {
  SIZE_T cmsg_len;
  INT cmsg_level;
  INT cmsg_type;
} WSACMSGHDR, *PWSACMSGHDR, FAR *LPWSACMSGHDR;

#if (_WIN32_WINNT >= 0x0600)
typedef WSACMSGHDR CMSGHDR, *PCMSGHDR;
#endif

#define WSA_CMSGHDR_ALIGN(length) (((length) + TYPE_ALIGNMENT(WSACMSGHDR)-1) &  \
                                   (~(TYPE_ALIGNMENT(WSACMSGHDR)-1)))

#define WSA_CMSGDATA_ALIGN(length) (((length) + MAX_NATURAL_ALIGNMENT-1) &      \
                                    (~(MAX_NATURAL_ALIGNMENT-1)))

#if(_WIN32_WINNT >= 0x0600)
#define CMSGHDR_ALIGN WSA_CMSGHDR_ALIGN
#define CMSGDATA_ALIGN WSA_CMSGDATA_ALIGN
#endif

/*
 *  WSA_CMSG_FIRSTHDR
 *
 *  Returns a pointer to the first ancillary data object,
 *  or a null pointer if there is no ancillary data in the
 *  control buffer of the WSAMSG structure.
 *
 *  LPCMSGHDR
 *  WSA_CMSG_FIRSTHDR (
 *      LPWSAMSG    msg
 *      );
 */
#define WSA_CMSG_FIRSTHDR(msg) (((msg)->Control.len >= sizeof(WSACMSGHDR))  \
                                ? (LPWSACMSGHDR)(msg)->Control.buf          \
                                : (LPWSACMSGHDR)NULL)

#if(_WIN32_WINNT >= 0x0600)
#define CMSG_FIRSTHDR WSA_CMSG_FIRSTHDR
#endif

/*
 *  WSA_CMSG_NXTHDR
 *
 *  Returns a pointer to the next ancillary data object,
 *  or a null if there are no more data objects.
 *
 *  LPCMSGHDR
 *  WSA_CMSG_NEXTHDR (
 *      LPWSAMSG        msg,
 *      LPWSACMSGHDR    cmsg
 *      );
 */
#define WSA_CMSG_NXTHDR(msg, cmsg)                          \
    ( ((cmsg) == NULL)                                      \
        ? WSA_CMSG_FIRSTHDR(msg)                            \
        : ( ( ((PUCHAR)(cmsg) +                             \
                    WSA_CMSGHDR_ALIGN((cmsg)->cmsg_len) +   \
                    sizeof(WSACMSGHDR) ) >                  \
                (PUCHAR)((msg)->Control.buf) +              \
                    (msg)->Control.len )                    \
            ? (LPWSACMSGHDR)NULL                            \
            : (LPWSACMSGHDR)((PUCHAR)(cmsg) +               \
                WSA_CMSGHDR_ALIGN((cmsg)->cmsg_len)) ) )

#if(_WIN32_WINNT >= 0x0600)
#define CMSG_NXTHDR WSA_CMSG_NXTHDR
#endif

/*
 *  WSA_CMSG_DATA
 *
 *  Returns a pointer to the first byte of data (what is referred
 *  to as the cmsg_data member though it is not defined in
 *  the structure).
 *
 *  Note that RFC 2292 defines this as CMSG_DATA, but that name
 *  is already used by wincrypt.h, and so Windows has used WSA_CMSG_DATA.
 *
 *  PUCHAR
 *  WSA_CMSG_DATA (
 *      LPWSACMSGHDR   pcmsg
 *      );
 */
#define WSA_CMSG_DATA(cmsg) ((PUCHAR)(cmsg) + WSA_CMSGDATA_ALIGN(sizeof(WSACMSGHDR)))

/*
 *  WSA_CMSG_SPACE
 *
 *  Returns total size of an ancillary data object given
 *  the amount of data. Used to allocate the correct amount
 *  of space.
 *
 *  SIZE_T
 *  WSA_CMSG_SPACE (
 *      SIZE_T length
 *      );
 */
#define WSA_CMSG_SPACE(length) (WSA_CMSGDATA_ALIGN(sizeof(WSACMSGHDR) + WSA_CMSGHDR_ALIGN(length)))

#if(_WIN32_WINNT >= 0x0600)
#define CMSG_SPACE WSA_CMSG_SPACE
#endif

/*
 *  WSA_CMSG_LEN
 *
 *  Returns the value to store in cmsg_len given the amount of data.
 *
 *  SIZE_T
 *  WSA_CMSG_LEN (
 *      SIZE_T length
 *  );
 */
#define WSA_CMSG_LEN(length) (WSA_CMSGDATA_ALIGN(sizeof(WSACMSGHDR)) + length)

#if(_WIN32_WINNT >= 0x0600)
#define CMSG_LEN WSA_CMSG_LEN
#endif

typedef struct addrinfo {
  int ai_flags;
  int ai_family;
  int ai_socktype;
  int ai_protocol;
  size_t ai_addrlen;
  char *ai_canonname;
  struct sockaddr *ai_addr;
  struct addrinfo *ai_next;
} ADDRINFOA, *PADDRINFOA;

typedef struct addrinfoW {
  int ai_flags;
  int ai_family;
  int ai_socktype;
  int ai_protocol;
  size_t ai_addrlen;
  PWSTR ai_canonname;
  struct sockaddr *ai_addr;
  struct addrinfoW *ai_next;
} ADDRINFOW, *PADDRINFOW;

#if (_WIN32_WINNT >= 0x0600)

typedef struct addrinfoexA {
  int ai_flags;
  int ai_family;
  int ai_socktype;
  int ai_protocol;
  size_t ai_addrlen;
  char *ai_canonname;
  struct sockaddr *ai_addr;
  void *ai_blob;
  size_t ai_bloblen;
  LPGUID ai_provider;
  struct addrinfoexA *ai_next;
} ADDRINFOEXA, *PADDRINFOEXA, *LPADDRINFOEXA;

typedef struct addrinfoexW {
  int ai_flags;
  int ai_family;
  int ai_socktype;
  int ai_protocol;
  size_t ai_addrlen;
  PWSTR ai_canonname;
  struct sockaddr *ai_addr;
  void *ai_blob;
  size_t ai_bloblen;
  LPGUID ai_provider;
  struct addrinfoexW *ai_next;
} ADDRINFOEXW, *PADDRINFOEXW, *LPADDRINFOEXW;

#endif /* (_WIN32_WINNT >= 0x0600) */

#ifdef __cplusplus
}
#endif
