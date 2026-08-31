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

#ifndef _WS2DEF_
#define _WS2DEF_

#include <inaddr.h>

#ifdef USE_WS_PREFIX
#define WS(x)    WS_##x
#else
#define WS(x)    x
#endif

typedef USHORT ADDRESS_FAMILY;

typedef struct WS(sockaddr)
{
    unsigned short sa_family;
    char sa_data[14];
} SOCKADDR, *PSOCKADDR, *LPSOCKADDR;

#ifndef USE_WS_PREFIX
#define AF_UNSPEC                  0
#define AF_UNIX                    1
#define AF_INET                    2
#define AF_IMPLINK                 3
#define AF_PUP                     4
#define AF_CHAOS                   5
#define AF_NS                      6
#define AF_IPX                     AF_NS
#define AF_ISO                     7
#define AF_OSI                     AF_ISO
#define AF_ECMA                    8
#define AF_DATAKIT                 9
#define AF_CCITT                   10
#define AF_SNA                     11
#define AF_DECnet                  12
#define AF_DLI                     13
#define AF_LAT                     14
#define AF_HYLINK                  15
#define AF_APPLETALK               16
#define AF_NETBIOS                 17
#define AF_VOICEVIEW               18
#define AF_FIREFOX                 19
#define AF_UNKNOWN1                20
#define AF_BAN                     21
#define AF_ATM                     22
#define AF_INET6                   23
#define AF_CLUSTER                 24
#define AF_12844                   25
#define AF_IRDA                    26
#define AF_MAX                     27
#else /* USE_WS_PREFIX */
#define WS_AF_UNSPEC               0
#define WS_AF_UNIX                 1
#define WS_AF_INET                 2
#define WS_AF_IMPLINK              3
#define WS_AF_PUP                  4
#define WS_AF_CHAOS                5
#define WS_AF_NS                   6
#define WS_AF_IPX                  WS_AF_NS
#define WS_AF_ISO                  7
#define WS_AF_OSI                  WS_AF_ISO
#define WS_AF_ECMA                 8
#define WS_AF_DATAKIT              9
#define WS_AF_CCITT                10
#define WS_AF_SNA                  11
#define WS_AF_DECnet               12
#define WS_AF_DLI                  13
#define WS_AF_LAT                  14
#define WS_AF_HYLINK               15
#define WS_AF_APPLETALK            16
#define WS_AF_NETBIOS              17
#define WS_AF_VOICEVIEW            18
#define WS_AF_FIREFOX              19
#define WS_AF_UNKNOWN1             20
#define WS_AF_BAN                  21
#define WS_AF_ATM                  22
#define WS_AF_INET6                23
#define WS_AF_CLUSTER              24
#define WS_AF_12844                25
#define WS_AF_IRDA                 26
#define WS_AF_MAX                  27
#endif /* USE_WS_PREFIX */

#ifndef USE_WS_PREFIX
#define IPPROTO_IP 0
#else
#define WS_IPPROTO_IP 0
#endif

typedef enum
{
    WS(IPPROTO_ICMP)    = 1,
    WS(IPPROTO_IGMP)    = 2,
    WS(IPPROTO_GGP)     = 3,
    WS(IPPROTO_IPV4)    = 4,
    WS(IPPROTO_TCP)     = 6,
    WS(IPPROTO_UDP)     = 17,
    WS(IPPROTO_IDP)     = 22,
    WS(IPPROTO_IPV6)    = 41,
    WS(IPPROTO_ICMPV6)  = 58,
    WS(IPPROTO_ND)      = 77,
    WS(IPPROTO_RAW)     = 255,
    WS(IPPROTO_MAX)     = 256,
} IPPROTO;

#ifndef USE_WS_PREFIX
#define INADDR_ANY          ((ULONG)0x00000000)
#define INADDR_LOOPBACK     0x7f000001
#define INADDR_BROADCAST    ((ULONG)0xffffffff)
#define INADDR_NONE         0xffffffff
#else
#define WS_INADDR_ANY       ((ULONG)0x00000000)
#define WS_INADDR_LOOPBACK  0x7f000001
#define WS_INADDR_BROADCAST ((ULONG)0xffffffff)
#define WS_INADDR_NONE      0xffffffff
#endif

#ifndef USE_WS_PREFIX
#define IN_CLASSA_NSHIFT            24
#define IN_CLASSA_MAX               128
#define IN_CLASSA_NET               0xff000000
#define IN_CLASSA_HOST              0x00ffffff
#define IN_CLASSA(i)                (((LONG)(i) & 0x80000000) == 0)
#define IN_CLASSB_NSHIFT            16
#define IN_CLASSB_MAX               65536
#define IN_CLASSB_NET               0xffff0000
#define IN_CLASSB_HOST              0x0000ffff
#define IN_CLASSB(i)                (((LONG)(i) & 0xc0000000) == 0x80000000)
#define IN_CLASSC_NSHIFT            8
#define IN_CLASSC_NET               0xffffff00
#define IN_CLASSC_HOST              0x000000ff
#define IN_CLASSC(i)                (((LONG)(i) & 0xe0000000) == 0xc0000000)
#else
#define WS_IN_CLASSA_NSHIFT         24
#define WS_IN_CLASSA_MAX            128
#define WS_IN_CLASSA_NET            0xff000000
#define WS_IN_CLASSA_HOST           0x00ffffff
#define WS_IN_CLASSA(i)             (((LONG)(i) & 0x80000000) == 0)
#define WS_IN_CLASSB_NSHIFT         16
#define WS_IN_CLASSB_MAX            65536
#define WS_IN_CLASSB_NET            0xffff0000
#define WS_IN_CLASSB_HOST           0x0000ffff
#define WS_IN_CLASSB(i)             (((LONG)(i) & 0xc0000000) == 0x80000000)
#define WS_IN_CLASSC_NSHIFT         8
#define WS_IN_CLASSC_NET            0xffffff00
#define WS_IN_CLASSC_HOST           0x000000ff
#define WS_IN_CLASSC(i)             (((LONG)(i) & 0xe0000000) == 0xc0000000)
#endif /* USE_WS_PREFIX */

#ifndef USE_WS_PREFIX
#define SO_BSP_STATE                0x1009
#define SO_RANDOMIZE_PORT           0x3005
#define SO_PORT_SCALABILITY         0x3006
#define SO_REUSE_UNICASTPORT        0x3007
#define SO_REUSE_MULTICASTPORT      0x3008
#define TCP_NODELAY                 0x0001
#else
#define WS_SO_BSP_STATE             0x1009
#define WS_SO_RANDOMIZE_PORT        0x3005
#define WS_SO_PORT_SCALABILITY      0x3006
#define WS_SO_REUSE_UNICASTPORT     0x3007
#define WS_SO_REUSE_MULTICASTPORT   0x3008
#define WS_TCP_NODELAY              0x0001
#endif

typedef struct WS(sockaddr_in)
{
    short sin_family;
    unsigned short sin_port;
    struct WS(in_addr) sin_addr;
    char sin_zero[8];
} SOCKADDR_IN, *PSOCKADDR_IN, *LPSOCKADDR_IN;

#ifndef __CSADDR_DEFINED__
#define __CSADDR_DEFINED__

typedef struct _SOCKET_ADDRESS {
        LPSOCKADDR      lpSockaddr;
        INT             iSockaddrLength;
} SOCKET_ADDRESS, *PSOCKET_ADDRESS, *LPSOCKET_ADDRESS;

typedef struct _CSADDR_INFO {
        SOCKET_ADDRESS  LocalAddr;
        SOCKET_ADDRESS  RemoteAddr;
        INT             iSocketType;
        INT             iProtocol;
} CSADDR_INFO, *PCSADDR_INFO, *LPCSADDR_INFO;
#endif

#ifdef USE_WS_PREFIX
#define WS__SS_MAXSIZE 128
#define WS__SS_ALIGNSIZE (sizeof(__int64))
#define WS__SS_PAD1SIZE (WS__SS_ALIGNSIZE - sizeof(short))
#define WS__SS_PAD2SIZE (WS__SS_MAXSIZE - 2 * WS__SS_ALIGNSIZE)
#else
#define _SS_MAXSIZE 128
#define _SS_ALIGNSIZE (sizeof(__int64))
#define _SS_PAD1SIZE (_SS_ALIGNSIZE - sizeof(short))
#define _SS_PAD2SIZE (_SS_MAXSIZE - 2 * _SS_ALIGNSIZE)
#endif

typedef struct WS(sockaddr_storage) {
        short ss_family;
        char __ss_pad1[WS(_SS_PAD1SIZE)];
        __int64 DECLSPEC_ALIGN(8) __ss_align;
        char __ss_pad2[WS(_SS_PAD2SIZE)];
} SOCKADDR_STORAGE, *PSOCKADDR_STORAGE, *LPSOCKADDR_STORAGE;

/*socket address list */
typedef struct _SOCKET_ADDRESS_LIST {
        INT             iAddressCount;
        SOCKET_ADDRESS  Address[1];
} SOCKET_ADDRESS_LIST, *LPSOCKET_ADDRESS_LIST;

typedef enum {
    ScopeLevelInterface    = 1,
    ScopeLevelLink         = 2,
    ScopeLevelSubnet       = 3,
    ScopeLevelAdmin        = 4,
    ScopeLevelSite         = 5,
    ScopeLevelOrganization = 8,
    ScopeLevelGlobal       = 14,
    ScopeLevelCount        = 16,
} SCOPE_LEVEL;

typedef struct
{
    union {
        struct {
            ULONG Zone  : 28;
            ULONG Level : 4;
        } DUMMYSTRUCTNAME;
        ULONG Value;
    } DUMMYUNIONNAME;
} SCOPE_ID, *PSCOPE_ID;

typedef struct _WSABUF
{
    ULONG len;
    CHAR* buf;
} WSABUF, *LPWSABUF;

typedef struct _WSAMSG {
    LPSOCKADDR  name;
    INT         namelen;
    LPWSABUF    lpBuffers;
    DWORD       dwBufferCount;
    WSABUF      Control;
    DWORD       dwFlags;
} WSAMSG, *PWSAMSG, *LPWSAMSG;

/*
 * Macros for retrieving control message data returned by WSARecvMsg()
 */
#define WSA_CMSG_DATA(cmsg)     ((UCHAR*)((WSACMSGHDR*)(cmsg)+1))
#define WSA_CMSG_FIRSTHDR(mhdr) ((mhdr)->Control.len >= sizeof(WSACMSGHDR) ? (WSACMSGHDR *) (mhdr)->Control.buf : (WSACMSGHDR *) 0)
#define WSA_CMSG_ALIGN(len)     (((len) + sizeof(SIZE_T) - 1) & ~(sizeof(SIZE_T) - 1))
/*
 * Next Header: If the response is too short (or the next message in the response
 * is too short) then return NULL, otherwise return the next control message.
 */
#define WSA_CMSG_NXTHDR(mhdr,cmsg) \
        (!(cmsg) ? WSA_CMSG_FIRSTHDR(mhdr) : \
         ((mhdr)->Control.len < sizeof(WSACMSGHDR) ? NULL : \
         (((unsigned char*)(((WSACMSGHDR*)((unsigned char*)cmsg + WSA_CMSG_ALIGN(cmsg->cmsg_len)))+1) > ((unsigned char*)(mhdr)->Control.buf + (mhdr)->Control.len)) ? NULL : \
          (((unsigned char*)cmsg + WSA_CMSG_ALIGN(cmsg->cmsg_len)+WSA_CMSG_ALIGN(((WSACMSGHDR*)((unsigned char*)cmsg + WSA_CMSG_ALIGN(cmsg->cmsg_len)))->cmsg_len) > ((unsigned char*)(mhdr)->Control.buf + (mhdr)->Control.len)) ? NULL : \
           (WSACMSGHDR*)((unsigned char*)cmsg + WSA_CMSG_ALIGN(cmsg->cmsg_len))))))

#ifndef USE_WS_PREFIX
#define AI_DNS_ONLY     0x00000010
#else
#define WS_AI_DNS_ONLY  0x00000010
#endif

typedef struct addrinfoexA {
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    SIZE_T ai_addrlen;
    char *ai_canonname;
    struct WS(sockaddr) *ai_addr;
    void *ai_blob;
    SIZE_T ai_bloblen;
    GUID *ai_provider;
    struct addrinfoexA *ai_next;
} ADDRINFOEXA, *PADDRINFOEXA, *LPADDRINFOEXA;

typedef struct addrinfoexW {
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    SIZE_T ai_addrlen;
    WCHAR *ai_canonname;
    struct WS(sockaddr) *ai_addr;
    void *ai_blob;
    SIZE_T ai_bloblen;
    GUID *ai_provider;
    struct addrinfoexW *ai_next;
} ADDRINFOEXW, *PADDRINFOEXW, *LPADDRINFOEXW;

#endif /* _WS2DEF_ */
