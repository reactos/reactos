/* 
   Sockets.h

   Windows Sockets specification version 1.1

   Copyright (C) 1996 Free Software Foundation, Inc.
   Thanks to Linux header files for supplying many needed definitions

   Author:  Scott Christley <scottc@net-community.com>
   Date: 1996
   
   This file is part of the Windows32 API Library.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   If you are interested in a warranty or support for this source code,
   contact Scott Christley <scottc@net-community.com> for more information.
   
   You should have received a copy of the GNU Library General Public
   License along with this library; see the file COPYING.LIB.
   If not, write to the Free Software Foundation, 
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/ 

/*-
 * Portions Copyright (c) 1980, 1983, 1988, 1993
 *     The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * -
 * Portions Copyright (c) 1993 by Digital Equipment Corporation.
 *
 * Permission to use, copy, modify and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies, and that
 * the name of Digital Equipment Corporation not be used in advertising or
 * publicity pertaining to distribution of the document or software without
 * specific, written prior permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 * -
 * --Copyright--
 */

#ifndef _GNU_H_WINDOWS32_SOCKETS
#define _GNU_H_WINDOWS32_SOCKETS

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* BSD */
#ifndef _SYS_TYPES_H
typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned int    u_int;
typedef unsigned long   u_long;
#endif

/*
  Default maximium number of sockets.
  Define this before including Sockets.h to increase; this does not
  mean that the underlying Windows Sockets implementation has to
  support that many!
  */
#ifndef FD_SETSIZE
#define FD_SETSIZE      64
#endif /* !FD_SETSIZE */

/*
  These macros are critical to the usage of Windows Sockets.
  According to the documentation, a SOCKET is no longer represented
  by a "small non-negative integer"; so all programs MUST use these
  macros for setting, initializing, clearing and checking the
  fd_set structures.
  */

typedef u_int           SOCKET;

/* fd_set may have been defined by the newlib <sys/types.h>.  */
#ifdef fd_set
#undef fd_set
#endif
typedef struct fd_set {
        u_int   fd_count;
        SOCKET  fd_array[FD_SETSIZE];
} fd_set;

/* Internal function, not documented except in winsock.h */
extern int PASCAL __WSAFDIsSet(SOCKET, fd_set*);

#ifdef FD_CLR
#undef FD_CLR
#endif
#define FD_CLR(fd, set) do { \
    u_int __i; \
    for (__i = 0; __i < ((fd_set*)(set))->fd_count ; __i++) { \
        if (((fd_set*)(set))->fd_array[__i] == fd) { \
            while (__i < ((fd_set*)(set))->fd_count-1) { \
                ((fd_set*)(set))->fd_array[__i] = \
                    ((fd_set*)(set))->fd_array[__i+1]; \
                __i++; \
            } \
            ((fd_set*)(set))->fd_count--; \
            break; \
        } \
    } \
} while(0)

#ifdef FD_SET
#undef FD_SET
#endif
#define FD_SET(fd, set) do { \
    if (((fd_set*)(set))->fd_count < FD_SETSIZE) \
        ((fd_set*)(set))->fd_array[((fd_set*)(set))->fd_count++]=(fd);\
} while(0)

#ifdef FD_ZERO
#undef FD_ZERO
#endif
#define FD_ZERO(set) (((fd_set*)(set))->fd_count=0)

#ifdef FD_ISSET
#undef FD_ISSET
#endif
#define FD_ISSET(fd, set) __WSAFDIsSet((SOCKET)(fd), (fd_set*)(set))

/*
  time structures
  */
struct timeval {
  long tv_sec;     /* seconds */
  long tv_usec;    /* microseconds */
};
struct timezone {
  int tz_minuteswest; /* minutes west of Greenwich */
  int tz_dsttime;     /* type of dst correction */
};

/*
 Operations on timevals.

 NB: timercmp does not work for >= or <=.
 */
#define	timerisset(tvp)		((tvp)->tv_sec || (tvp)->tv_usec)
#define	timercmp(tvp, uvp, cmp)	\
    (((tvp)->tv_sec == (uvp)->tv_sec && (tvp)->tv_usec cmp (uvp)->tv_usec) \
    || (tvp)->tv_sec cmp (uvp)->tv_sec)
#define	timerclear(tvp)		((tvp)->tv_sec = (tvp)->tv_usec = 0)

/*
  ioctl command encoding.
  Some of this is different than what Linux has
  */
#define IOCPARM_MASK    0x7f
#define IOC_VOID        0x20000000
#define IOC_OUT         0x40000000
#define IOC_IN          0x80000000
#define IOC_INOUT       (IOC_IN | IOC_OUT)

/* _IO(magic, subcode) */
#define _IO(c,d)        (IOC_VOID | ((c)<<8) | (d))
/* _IOXX(magic, subcode, arg_t) */
#define _IOW(c,d,t)     (IOC_IN | (((long)sizeof(t) & IOCPARM_MASK)<<16) | \
			 ((c)<<8) | (d))
#define _IOR(c,d,t)     (IOC_OUT | (((long)sizeof(t) & IOCPARM_MASK)<<16) | \
			 ((c)<<8) | (d))

/*
  This stuff is hard-coded on Linux
  But winsock.h uses the macros defined above
  */
#define FIONREAD    _IOR('f', 127, u_long)
#define FIONBIO     _IOW('f', 126, u_long)
#define FIOASYNC    _IOW('f', 125, u_long)

#define SIOCSHIWAT  _IOW('s',  0, u_long)
#define SIOCGHIWAT  _IOR('s',  1, u_long)
#define SIOCSLOWAT  _IOW('s',  2, u_long)
#define SIOCGLOWAT  _IOR('s',  3, u_long)
#define SIOCATMARK  _IOR('s',  7, u_long)

/*
 Structures returned by network data base library, taken from the
 BSD file netdb.h.  All addresses are supplied in host order, and
 returned in network order (suitable for use in system calls).

 Slight modifications for differences between Linux and winsock.h
 */

struct  hostent {
  char    *h_name;                /* official name of host */
  char    **h_aliases;            /* alias list */
  short   h_addrtype;             /* host address type */
  short   h_length;               /* length of address */
  char    **h_addr_list;          /* list of addresses */
#define h_addr  h_addr_list[0]    /* address, for backward compat */
};

/*
 * Assumption here is that a network number
 * fits in an unsigned long -- someday that won't be true!
 */
struct  netent {
  char    *n_name;      /* official name of net */
  char    **n_aliases;  /* alias list */
  short   n_addrtype;   /* net address type */
  u_long  n_net;        /* network # */
};

struct  servent {
  char    *s_name;      /* official service name */
  char    **s_aliases;  /* alias list */
  short   s_port;       /* port # */
  char    *s_proto;     /* protocol to use */
};

struct  protoent {
  char    *p_name;      /* official protocol name */
  char    **p_aliases;  /* alias list */
  short   p_proto;      /* protocol # */
};

/*
  Standard well-known IP protocols.
  For some reason there are differences between Linx and winsock.h
  */
enum {
  IPPROTO_IP = 0,
  IPPROTO_ICMP = 1,
  IPPROTO_GGP = 2,               /* huh? */
  IPPROTO_IPIP = 4,
  IPPROTO_TCP = 6,               /* Transmission Control Protocol */
  IPPROTO_EGP = 8,
  IPPROTO_PUP = 12,
  IPPROTO_UDP = 17,              /* User Datagram Protocol */
  IPPROTO_IDP = 22,
  IPPROTO_ND = 77,               /* This one was in winsock.h */

  IPPROTO_RAW = 255,             /* raw IP packets */
  IPPROTO_MAX
};

/* Standard well-known ports */
enum {
  IPPORT_ECHO = 7,
  IPPORT_DISCARD = 9,
  IPPORT_SYSTAT = 11,
  IPPORT_DAYTIME = 13,
  IPPORT_NETSTAT = 15,
  IPPORT_FTP = 21,
  IPPORT_TELNET = 23,
  IPPORT_SMTP = 25,
  IPPORT_TIMESERVER = 37,
  IPPORT_NAMESERVER = 42,
  IPPORT_WHOIS = 43,
  IPPORT_MTP = 57,

  IPPORT_TFTP = 69,
  IPPORT_RJE = 77,
  IPPORT_FINGER = 79,
  IPPORT_TTYLINK = 87,
  IPPORT_SUPDUP = 95,

  IPPORT_EXECSERVER = 512,
  IPPORT_LOGINSERVER = 513,
  IPPORT_CMDSERVER = 514,
  IPPORT_EFSSERVER = 520,

  /* UDP ports. */
  IPPORT_BIFFUDP = 512,
  IPPORT_WHOSERVER = 513,
  IPPORT_ROUTESERVER = 520,

  /* Ports less than this value are reservered for privileged processes. */
  IPPORT_RESERVED = 1024,

  /* Ports greater than this value are reserved for 
     (non-privileged) processes */
  IPPORT_USERRESERVED = 5000
};

/* Link numbers. */
#define IMPLINK_IP              155
#define IMPLINK_LOWEXPER        156
#define IMPLINK_HIGHEXPER       158

/* Linux uses a simple unsigned long int, but winsock.h ... */
struct in_addr {
        union {
                struct { u_char s_b1,s_b2,s_b3,s_b4; } S_un_b;
                struct { u_short s_w1,s_w2; } S_un_w;
                u_long S_addr;
        } S_un;
#define s_addr  S_un.S_addr
#define s_host  S_un.S_un_b.s_b2
#define s_net   S_un.S_un_b.s_b1
#define s_imp   S_un.S_un_w.s_w2
#define s_impno S_un.S_un_b.s_b4
#define s_lh    S_un.S_un_b.s_b3
};

/*
 Definitions of bits in internet address integers.
 On subnets, host and network parts are found according
 to the subnet mask, not these masks.
 */
#define IN_CLASSA(i)            (((long)(i) & 0x80000000) == 0)
#define IN_CLASSA_NET           0xff000000
#define IN_CLASSA_NSHIFT        24
#define IN_CLASSA_HOST          0x00ffffff
#define IN_CLASSA_MAX           128

#define IN_CLASSB(i)            (((long)(i) & 0xc0000000) == 0x80000000)
#define IN_CLASSB_NET           0xffff0000
#define IN_CLASSB_NSHIFT        16
#define IN_CLASSB_HOST          0x0000ffff
#define IN_CLASSB_MAX           65536

#define IN_CLASSC(i)            (((long)(i) & 0xe0000000) == 0xc0000000)
#define IN_CLASSC_NET           0xffffff00
#define IN_CLASSC_NSHIFT        8
#define IN_CLASSC_HOST          0x000000ff

#define INADDR_ANY              (u_long)0x00000000
#define INADDR_LOOPBACK         0x7f000001
#define INADDR_BROADCAST        (u_long)0xffffffff    
#define INADDR_NONE             0xffffffff

/*
 Structure describing an Internet (IP) socket address.
 */
struct sockaddr_in {
  short   sin_family;
  u_short sin_port;
  struct  in_addr sin_addr;
  char    sin_zero[8];
};

/*
  EVERYTHING FROM THIS POINT IS MAINLY SPECIFIC TO Win32

  Structure which holds the detail for the underlying Window Sockets
  implementation.  Set when WSAStartup() is called.
  */
#define WSADESCRIPTION_LEN      256
#define WSASYS_STATUS_LEN       128

typedef struct WSAData {
  WORD wVersion;
  WORD wHighVersion;
  char szDescription[WSADESCRIPTION_LEN+1];
  char szSystemStatus[WSASYS_STATUS_LEN+1];
  unsigned short iMaxSockets;
  unsigned short iMaxUdpDg;
  char *lpVendorInfo;
} WSADATA, *LPWSADATA;

#define IP_OPTIONS          1
#define IP_MULTICAST_IF     2
#define IP_MULTICAST_TTL    3
#define IP_MULTICAST_LOOP   4
#define IP_ADD_MEMBERSHIP   5
#define IP_DROP_MEMBERSHIP  6

#define IP_DEFAULT_MULTICAST_TTL   1
#define IP_DEFAULT_MULTICAST_LOOP  1
#define IP_MAX_MEMBERSHIPS         20

struct ip_mreq {
  struct in_addr imr_multiaddr;
  struct in_addr imr_interface;
};

/*
 * Definitions related to sockets: types, address families, options,
 * taken from the BSD file sys/socket.h.
 */

/*
 * This is used instead of -1, since the
 * SOCKET type is unsigned.
 */
#define INVALID_SOCKET  (SOCKET)(~0)
#define SOCKET_ERROR            (-1)

/* Socket types. */
#define SOCK_STREAM     1
#define SOCK_DGRAM      2
#define SOCK_RAW        3
#define SOCK_RDM        4 
#define SOCK_SEQPACKET  5

/* For setsockoptions(2) */
#define SO_DEBUG        0x0001
#define SO_ACCEPTCONN   0x0002
#define SO_REUSEADDR    0x0004
#define SO_KEEPALIVE    0x0008
#define SO_DONTROUTE    0x0010
#define SO_BROADCAST    0x0020
#define SO_USELOOPBACK  0x0040
#define SO_LINGER       0x0080
#define SO_OOBINLINE    0x0100

#define SO_DONTLINGER   (u_int)(~SO_LINGER)

/*
 * Additional options.
 */
#define SO_SNDBUF       0x1001          /* send buffer size */
#define SO_RCVBUF       0x1002          /* receive buffer size */
#define SO_SNDLOWAT     0x1003          /* send low-water mark */
#define SO_RCVLOWAT     0x1004          /* receive low-water mark */
#define SO_SNDTIMEO     0x1005          /* send timeout */
#define SO_RCVTIMEO     0x1006          /* receive timeout */
#define SO_ERROR        0x1007          /* get error status and clear */
#define SO_TYPE         0x1008          /* get socket type */

/*
 * Options for connect and disconnect data and options.  Used only by
 * non-TCP/IP transports such as DECNet, OSI TP4, etc.
 */
#define SO_CONNDATA     0x7000
#define SO_CONNOPT      0x7001
#define SO_DISCDATA     0x7002
#define SO_DISCOPT      0x7003
#define SO_CONNDATALEN  0x7004
#define SO_CONNOPTLEN   0x7005
#define SO_DISCDATALEN  0x7006
#define SO_DISCOPTLEN   0x7007

/*
 * Option for opening sockets for synchronous access.
 */
#define SO_OPENTYPE     0x7008

#define SO_SYNCHRONOUS_ALERT    0x10
#define SO_SYNCHRONOUS_NONALERT 0x20

/*
 * Other NT-specific options.
 */
#define SO_MAXDG        0x7009
#define SO_MAXPATHDG    0x700A

/*
 * TCP options.
 */
#define TCP_NODELAY     0x0001
#define TCP_BSDURGENT   0x7000

/*
 * Address families.
 */
#define AF_UNSPEC       0               /* unspecified */
#define AF_UNIX         1               /* local to host (pipes, portals) */
#define AF_INET         2               /* internetwork: UDP, TCP, etc. */
#define AF_IMPLINK      3               /* arpanet imp addresses */
#define AF_PUP          4               /* pup protocols: e.g. BSP */
#define AF_CHAOS        5               /* mit CHAOS protocols */
#define AF_IPX          6               /* IPX and SPX */
#define AF_NS           6               /* XEROX NS protocols */
#define AF_ISO          7               /* ISO protocols */
#define AF_OSI          AF_ISO          /* OSI is ISO */
#define AF_ECMA         8               /* european computer manufacturers */
#define AF_DATAKIT      9               /* datakit protocols */
#define AF_CCITT        10              /* CCITT protocols, X.25 etc */
#define AF_SNA          11              /* IBM SNA */
#define AF_DECnet       12              /* DECnet */
#define AF_DLI          13              /* Direct data link interface */
#define AF_LAT          14              /* LAT */
#define AF_HYLINK       15              /* NSC Hyperchannel */
#define AF_APPLETALK    16              /* AppleTalk */
#define AF_NETBIOS      17              /* NetBios-style addresses */

#define AF_MAX          18

/*
 * Structure used by kernel to store most
 * addresses.
 */
struct sockaddr {
  u_short sa_family;
  char    sa_data[14];
};

/*
 * Structure used by kernel to pass protocol
 * information in raw sockets.
 */
struct sockproto {
  u_short sp_family;
  u_short sp_protocol;
};

/*
 * Protocol families, same as address families for now.
 */
#define PF_UNSPEC       AF_UNSPEC
#define PF_UNIX         AF_UNIX
#define PF_INET         AF_INET
#define PF_IMPLINK      AF_IMPLINK
#define PF_PUP          AF_PUP
#define PF_CHAOS        AF_CHAOS
#define PF_NS           AF_NS
#define PF_IPX          AF_IPX
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

/*
 * Structure used for manipulating linger option.
 */
struct  linger {
  u_short l_onoff;
  u_short l_linger;
};

/*
 * Level number for (get/set)sockopt() to apply to socket itself.
 */
#define SOL_SOCKET      0xffff          /* options for socket level */

/*
 * Maximum queue length specifiable by listen.
 */
#define SOMAXCONN       5

#define MSG_OOB         0x1             /* process out-of-band data */
#define MSG_PEEK        0x2             /* peek at incoming message */
#define MSG_DONTROUTE   0x4             /* send without using routing tables */

#define MSG_MAXIOVLEN   16

#define	MSG_PARTIAL     0x8000          /* partial send or recv for message xport */

/*
 * Define constant based on rfc883, used by gethostbyxxxx() calls.
 */
#define MAXGETHOSTSTRUCT        1024
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN          MAXGETHOSTSTRUCT
#endif

/*
 * Define flags to be used with the WSAAsyncSelect() call.
 */
#define FD_READ         0x01
#define FD_WRITE        0x02
#define FD_OOB          0x04
#define FD_ACCEPT       0x08
#define FD_CONNECT      0x10
#define FD_CLOSE        0x20

/*
 * All Windows Sockets error constants are biased by WSABASEERR from
 * the "normal"
 */
#define WSABASEERR              10000
/*
 * Windows Sockets definitions of regular Microsoft C error constants
 */
#define WSAEINTR                (WSABASEERR+4)
#define WSAEBADF                (WSABASEERR+9)
#define WSAEACCES               (WSABASEERR+13)
#define WSAEFAULT               (WSABASEERR+14)
#define WSAEINVAL               (WSABASEERR+22)
#define WSAEMFILE               (WSABASEERR+24)

/*
 * Windows Sockets definitions of regular Berkeley error constants
 */
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
#define WSAEPROCLIM             (WSABASEERR+67)
#define WSAEUSERS               (WSABASEERR+68)
#define WSAEDQUOT               (WSABASEERR+69)
#define WSAESTALE               (WSABASEERR+70)
#define WSAEREMOTE              (WSABASEERR+71)

#define WSAEDISCON              (WSABASEERR+101)

/*
 * Extended Windows Sockets error constant definitions
 */
#define WSASYSNOTREADY          (WSABASEERR+91)
#define WSAVERNOTSUPPORTED      (WSABASEERR+92)
#define WSANOTINITIALISED       (WSABASEERR+93)

/*
 * Error return codes from gethostbyname() and gethostbyaddr()
 * (when using the resolver). Note that these errors are
 * retrieved via WSAGetLastError() and must therefore follow
 * the rules for avoiding clashes with error numbers from
 * specific implementations or language run-time systems.
 * For this reason the codes are based at WSABASEERR+1001.
 * Note also that [WSA]NO_ADDRESS is defined only for
 * compatibility purposes.
 */

#define h_errno         WSAGetLastError()

/* Authoritative Answer: Host not found */
#define WSAHOST_NOT_FOUND       (WSABASEERR+1001)
#define HOST_NOT_FOUND          WSAHOST_NOT_FOUND

/* Non-Authoritative: Host not found, or SERVERFAIL */
#define WSATRY_AGAIN            (WSABASEERR+1002)
#define TRY_AGAIN               WSATRY_AGAIN

/* Non recoverable errors, FORMERR, REFUSED, NOTIMP */
#define WSANO_RECOVERY          (WSABASEERR+1003)
#define NO_RECOVERY             WSANO_RECOVERY

/* Valid name, no data record of requested type */
#define WSANO_DATA              (WSABASEERR+1004)
#define NO_DATA                 WSANO_DATA

/* no address, look for MX record */
#define WSANO_ADDRESS           WSANO_DATA
#define NO_ADDRESS              WSANO_ADDRESS

/*
 * Windows Sockets errors redefined as regular Berkeley error constants.
 * These are commented out in Windows NT to avoid conflicts with errno.h.
 * Use the WSA constants instead.
 */
#if 0
#define EWOULDBLOCK             WSAEWOULDBLOCK
#define EINPROGRESS             WSAEINPROGRESS
#define EALREADY                WSAEALREADY
#define ENOTSOCK                WSAENOTSOCK
#define EDESTADDRREQ            WSAEDESTADDRREQ
#define EMSGSIZE                WSAEMSGSIZE
#define EPROTOTYPE              WSAEPROTOTYPE
#define ENOPROTOOPT             WSAENOPROTOOPT
#define EPROTONOSUPPORT         WSAEPROTONOSUPPORT
#define ESOCKTNOSUPPORT         WSAESOCKTNOSUPPORT
#define EOPNOTSUPP              WSAEOPNOTSUPP
#define EPFNOSUPPORT            WSAEPFNOSUPPORT
#define EAFNOSUPPORT            WSAEAFNOSUPPORT
#define EADDRINUSE              WSAEADDRINUSE
#define EADDRNOTAVAIL           WSAEADDRNOTAVAIL
#define ENETDOWN                WSAENETDOWN
#define ENETUNREACH             WSAENETUNREACH
#define ENETRESET               WSAENETRESET
#define ECONNABORTED            WSAECONNABORTED
#define ECONNRESET              WSAECONNRESET
#define ENOBUFS                 WSAENOBUFS
#define EISCONN                 WSAEISCONN
#define ENOTCONN                WSAENOTCONN
#define ESHUTDOWN               WSAESHUTDOWN
#define ETOOMANYREFS            WSAETOOMANYREFS
#define ETIMEDOUT               WSAETIMEDOUT
#define ECONNREFUSED            WSAECONNREFUSED
#define ELOOP                   WSAELOOP
#define ENAMETOOLONG            WSAENAMETOOLONG
#define EHOSTDOWN               WSAEHOSTDOWN
#define EHOSTUNREACH            WSAEHOSTUNREACH
#define ENOTEMPTY               WSAENOTEMPTY
#define EPROCLIM                WSAEPROCLIM
#define EUSERS                  WSAEUSERS
#define EDQUOT                  WSAEDQUOT
#define ESTALE                  WSAESTALE
#define EREMOTE                 WSAEREMOTE
#endif

/* Socket function prototypes */

SOCKET PASCAL accept (SOCKET s, struct sockaddr *addr,
                          int *addrlen);

int PASCAL bind (SOCKET s, const struct sockaddr *addr, int namelen);

int PASCAL closesocket (SOCKET s);

int PASCAL connect (SOCKET s, const struct sockaddr *name, int namelen);

int PASCAL ioctlsocket (SOCKET s, long cmd, u_long *argp);

int PASCAL getpeername (SOCKET s, struct sockaddr *name,
                            int * namelen);

int PASCAL getsockname (SOCKET s, struct sockaddr *name,
                            int * namelen);

int PASCAL getsockopt (SOCKET s, int level, int optname,
                           char * optval, int *optlen);

u_long PASCAL htonl (u_long hostlong);

/* For some reason WSOCK.LIB has htons defined as a 4 byte paramter?! */
#ifdef _WIN32
u_short PASCAL htons (u_long hostshort);
#else
u_short PASCAL htons (u_short hostshort);
#endif /* _WIN32 */

unsigned long PASCAL inet_addr (const char * cp);

char * PASCAL inet_ntoa (struct in_addr in);

int PASCAL listen (SOCKET s, int backlog);

u_long PASCAL ntohl (u_long netlong);

/* For some reason WSOCK.LIB has ntohs defined as a 4 byte paramter?! */
#ifdef _WIN32
u_short PASCAL ntohs (u_long netshort);
#else
u_short PASCAL ntohs (u_short netshort);
#endif /* _WIN32 */

int PASCAL recv (SOCKET s, char * buf, int len, int flags);

int PASCAL recvfrom (SOCKET s, char * buf, int len, int flags,
                         struct sockaddr *from, int * fromlen);

int PASCAL select (int nfds, fd_set *readfds, fd_set *writefds,
                       fd_set *exceptfds, const struct timeval *timeout);

int PASCAL send (SOCKET s, const char * buf, int len, int flags);

int PASCAL sendto (SOCKET s, const char * buf, int len, int flags,
                       const struct sockaddr *to, int tolen);

int PASCAL setsockopt (SOCKET s, int level, int optname,
                           const char * optval, int optlen);

int PASCAL shutdown (SOCKET s, int how);

SOCKET PASCAL socket (int af, int type, int protocol);

/* Database function prototypes */

struct hostent * PASCAL gethostbyaddr(const char * addr,
                                              int len, int type);

struct hostent * PASCAL gethostbyname(const char * name);

int PASCAL gethostname (char * name, int namelen);

struct servent * PASCAL getservbyport(int port, const char * proto);

struct servent * PASCAL getservbyname(const char * name,
                                              const char * proto);

struct protoent * PASCAL getprotobynumber(int proto);

struct protoent * PASCAL getprotobyname(const char * name);

/* Microsoft Windows Extension function prototypes */

/* int PASCAL WSAStartup(WORD wVersionRequired, LPWSADATA lpWSAData); */
int PASCAL WSAStartup(int wVersionRequired, LPWSADATA lpWSAData);


int PASCAL WSACleanup(void);

void PASCAL WSASetLastError(int iError);

int PASCAL WSAGetLastError(void);

WINBOOL PASCAL WSAIsBlocking(void);

int PASCAL WSAUnhookBlockingHook(void);

FARPROC PASCAL WSASetBlockingHook(FARPROC lpBlockFunc);

int PASCAL WSACancelBlockingCall(void);

HANDLE PASCAL WSAAsyncGetServByName(HWND hWnd, u_int wMsg,
                                        const char * name, 
                                        const char * proto,
                                        char * buf, int buflen);

HANDLE PASCAL WSAAsyncGetServByPort(HWND hWnd, u_int wMsg, int port,
                                        const char * proto, char * buf,
                                        int buflen);

HANDLE PASCAL WSAAsyncGetProtoByName(HWND hWnd, u_int wMsg,
                                         const char * name, char * buf,
                                         int buflen);

HANDLE PASCAL WSAAsyncGetProtoByNumber(HWND hWnd, u_int wMsg,
                                           int number, char * buf,
                                           int buflen);

HANDLE PASCAL WSAAsyncGetHostByName(HWND hWnd, u_int wMsg,
                                        const char * name, char * buf,
                                        int buflen);

HANDLE PASCAL WSAAsyncGetHostByAddr(HWND hWnd, u_int wMsg,
                                        const char * addr, int len, int type,
                                        char * buf, int buflen);

int PASCAL WSACancelAsyncRequest(HANDLE hAsyncTaskHandle);

int PASCAL WSAAsyncSelect(SOCKET s, HWND hWnd, u_int wMsg,
                               long lEvent);

int PASCAL WSARecvEx (SOCKET s, char * buf, int len, int *flags);

/* Microsoft Windows Extended data types */
typedef struct sockaddr SOCKADDR;
typedef struct sockaddr *PSOCKADDR;
typedef struct sockaddr *LPSOCKADDR;

typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr_in *PSOCKADDR_IN;
typedef struct sockaddr_in *LPSOCKADDR_IN;

typedef struct linger LINGER;
typedef struct linger *PLINGER;
typedef struct linger *LPLINGER;

typedef struct in_addr IN_ADDR;
typedef struct in_addr *PIN_ADDR;
typedef struct in_addr *LPIN_ADDR;

typedef struct fd_set FD_SET;
typedef struct fd_set *PFD_SET;
typedef struct fd_set *LPFD_SET;

typedef struct hostent HOSTENT;
typedef struct hostent *PHOSTENT;
typedef struct hostent *LPHOSTENT;

typedef struct servent SERVENT;
typedef struct servent *PSERVENT;
typedef struct servent *LPSERVENT;

typedef struct protoent PROTOENT;
typedef struct protoent *PPROTOENT;
typedef struct protoent *LPPROTOENT;

typedef struct timeval TIMEVAL;
typedef struct timeval *PTIMEVAL;
typedef struct timeval *LPTIMEVAL;

/*
 * Windows message parameter composition and decomposition
 * macros.
 *
 * WSAMAKEASYNCREPLY is intended for use by the Windows Sockets implementation
 * when constructing the response to a WSAAsyncGetXByY() routine.
 */
#define WSAMAKEASYNCREPLY(buflen,error)     MAKELONG(buflen,error)
/*
 * WSAMAKESELECTREPLY is intended for use by the Windows Sockets implementation
 * when constructing the response to WSAAsyncSelect().
 */
#define WSAMAKESELECTREPLY(event,error)     MAKELONG(event,error)
/*
 * WSAGETASYNCBUFLEN is intended for use by the Windows Sockets application
 * to extract the buffer length from the lParam in the response
 * to a WSAGetXByY().
 */
#define WSAGETASYNCBUFLEN(lParam)           LOWORD(lParam)
/*
 * WSAGETASYNCERROR is intended for use by the Windows Sockets application
 * to extract the error code from the lParam in the response
 * to a WSAGetXByY().
 */
#define WSAGETASYNCERROR(lParam)            HIWORD(lParam)
/*
 * WSAGETSELECTEVENT is intended for use by the Windows Sockets application
 * to extract the event code from the lParam in the response
 * to a WSAAsyncSelect().
 */
#define WSAGETSELECTEVENT(lParam)           LOWORD(lParam)
/*
 * WSAGETSELECTERROR is intended for use by the Windows Sockets application
 * to extract the error code from the lParam in the response
 * to a WSAAsyncSelect().
 */
#define WSAGETSELECTERROR(lParam)           HIWORD(lParam)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _GNU_H_WINDOWS32_SOCKETS */
