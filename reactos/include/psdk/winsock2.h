/*

  Definitions for winsock 2

  Initially taken from the Wine project.

  Portions Copyright (c) 1980, 1983, 1988, 1993
  The Regents of the University of California.  All rights reserved.

  Portions Copyright (c) 1993 by Digital Equipment Corporation.
 */

#if !(defined _WINSOCK2_H || defined _WINSOCK_H)
#define _WINSOCK2_H
#define _WINSOCK_H /* to prevent later inclusion of winsock.h */

#define _GNU_H_WINDOWS32_SOCKETS

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif
/*   Names common to Winsock1.1 and Winsock2  */
#if !defined ( _BSDTYPES_DEFINED )
/* also defined in gmon.h and in cygwin's sys/types */
typedef unsigned char	u_char;
typedef unsigned short	u_short;
typedef unsigned int	u_int;
typedef unsigned long	u_long;
#define _BSDTYPES_DEFINED
#endif /* ! def _BSDTYPES_DEFINED  */
typedef u_int	SOCKET;
#ifndef FD_SETSIZE
#define FD_SETSIZE	64
#endif

/* shutdown() how types */
#define SD_RECEIVE      0x00
#define SD_SEND         0x01
#define SD_BOTH         0x02

#ifndef _SYS_TYPES_FD_SET
/* fd_set may be defined by the newlib <sys/types.h>
 * if __USE_W32_SOCKETS not defined.
 */
#ifdef fd_set
#undef fd_set
#endif
typedef struct fd_set {
	u_int   fd_count;
	SOCKET  fd_array[FD_SETSIZE];
} fd_set;
int PASCAL __WSAFDIsSet(SOCKET,fd_set*);
#ifndef FD_CLR
#define FD_CLR(fd,set) do { u_int __i;\
for (__i = 0; __i < ((fd_set *)(set))->fd_count ; __i++) {\
	if (((fd_set *)(set))->fd_array[__i] == (fd)) {\
	while (__i < ((fd_set *)(set))->fd_count-1) {\
		((fd_set*)(set))->fd_array[__i] = ((fd_set*)(set))->fd_array[__i+1];\
		__i++;\
	}\
	((fd_set*)(set))->fd_count--;\
	break;\
	}\
}\
} while (0)
#endif
#ifndef FD_SET
/* this differs from the define in winsock.h and in cygwin sys/types.h */
#define FD_SET(fd, set) do { u_int __i;\
for (__i = 0; __i < ((fd_set *)(set))->fd_count ; __i++) {\
	if (((fd_set *)(set))->fd_array[__i] == (fd)) {\
		break;\
	}\
}\
if (__i == ((fd_set *)(set))->fd_count) {\
	if (((fd_set *)(set))->fd_count < FD_SETSIZE) {\
		((fd_set *)(set))->fd_array[__i] = (fd);\
		((fd_set *)(set))->fd_count++;\
	}\
}\
} while(0)
#endif
#ifndef FD_ZERO
#define FD_ZERO(set) (((fd_set *)(set))->fd_count=0)
#endif
#ifndef FD_ISSET
#define FD_ISSET(fd, set) __WSAFDIsSet((SOCKET)(fd), (fd_set *)(set))
#endif
#elif !defined (USE_SYS_TYPES_FD_SET)
#warning "fd_set and associated macros have been defined in sys/types.  \
    This may cause runtime problems with W32 sockets"
#endif /* ndef _SYS_TYPES_FD_SET */
#if !(defined (__INSIDE_CYGWIN__) || (__INSIDE_MSYS__))
#ifndef _TIMEVAL_DEFINED
/* also in sys/time.h */
#define _TIMEVAL_DEFINED
#define _STRUCT_TIMEVAL
struct timeval {
	long    tv_sec;
	long    tv_usec;
};
#define timerisset(tvp)	 ((tvp)->tv_sec || (tvp)->tv_usec)
#define timercmp(tvp, uvp, cmp) \
	(((tvp)->tv_sec != (uvp)->tv_sec) ? \
	((tvp)->tv_sec cmp (uvp)->tv_sec) : \
	((tvp)->tv_usec cmp (uvp)->tv_usec))
#define timerclear(tvp)	 (tvp)->tv_sec = (tvp)->tv_usec = 0
#endif /* _TIMEVAL_DEFINED */
struct  hostent {
	char	*h_name;
	char	**h_aliases;
	short	h_addrtype;
	short	h_length;
	char	**h_addr_list;
#define h_addr h_addr_list[0]
};
struct linger {
	u_short l_onoff;
	u_short l_linger;
};
#endif /* ! (__INSIDE_CYGWIN__ || __INSIDE_MSYS__) */
#define IOCPARM_MASK	0x7f
#define IOC_VOID	0x20000000
#define IOC_OUT	0x40000000
#define IOC_IN	0x80000000
#define IOC_INOUT	(IOC_IN|IOC_OUT)

#if ! (defined (__INSIDE_CYGWIN__) || defined (__INSIDE_MSYS__))
#define _IO(x,y)	(IOC_VOID|((x)<<8)|(y))
#define _IOR(x,y,t)	(IOC_OUT|(((long)sizeof(t)&IOCPARM_MASK)<<16)|((x)<<8)|(y))
#define _IOW(x,y,t)	(IOC_IN|(((long)sizeof(t)&IOCPARM_MASK)<<16)|((x)<<8)|(y))
#define FIONBIO	_IOW('f', 126, u_long)
#endif /* ! (__INSIDE_CYGWIN__ || __INSIDE_MSYS__) */

#define FIONREAD	_IOR('f', 127, u_long)
#define FIOASYNC	_IOW('f', 125, u_long)
#define SIOCSHIWAT	_IOW('s',  0, u_long)
#define SIOCGHIWAT	_IOR('s',  1, u_long)
#define SIOCSLOWAT	_IOW('s',  2, u_long)
#define SIOCGLOWAT	_IOR('s',  3, u_long)
#define SIOCATMARK	_IOR('s',  7, u_long)

#if ! (defined (__INSIDE_CYGWIN__) || defined (__INSIDE_MSYS__))
struct netent {
	char	* n_name;
	char	**n_aliases;
	short	n_addrtype;
	u_long n_net;
};
struct  servent {
	char	*s_name;
	char	**s_aliases;
	short	s_port;
	char	*s_proto;
};
struct  protoent {
	char	*p_name;
	char	**p_aliases;
	short	p_proto;
};
#endif /* ! (__INSIDE_CYGWIN__ || __INSIDE_MSYS__) */

#define IPPROTO_IP	0
#define IPPROTO_ICMP 1
#define IPPROTO_IGMP 2
#define IPPROTO_GGP 3
#define IPPROTO_TCP	6
#define IPPROTO_PUP	12
#define IPPROTO_UDP	17
#define IPPROTO_IDP	22
#define IPPROTO_ND	77
#define IPPROTO_RAW	255
#define IPPROTO_MAX	256
/* IPv6 options */
#define IPPROTO_HOPOPTS		0  /* IPv6 Hop-by-Hop options */
#define IPPROTO_IPV6		41 /* IPv6 header */
#define IPPROTO_ROUTING		43 /* IPv6 Routing header */
#define IPPROTO_FRAGMENT	44 /* IPv6 fragmentation header */
#define IPPROTO_ESP		50 /* encapsulating security payload */
#define IPPROTO_AH		51 /* authentication header */
#define IPPROTO_ICMPV6		58 /* ICMPv6 */
#define IPPROTO_NONE		59 /* IPv6 no next header */
#define IPPROTO_DSTOPTS		60 /* IPv6 Destination options */
#define IPPORT_ECHO	7
#define IPPORT_DISCARD	9
#define IPPORT_SYSTAT	11
#define IPPORT_DAYTIME  13
#define IPPORT_NETSTAT  15
#define IPPORT_FTP      21
#define IPPORT_TELNET   23
#define IPPORT_SMTP     25
#define IPPORT_TIMESERVER 37
#define IPPORT_NAMESERVER 42
#define IPPORT_WHOIS	43
#define IPPORT_MTP	57
#define IPPORT_TFTP	69
#define IPPORT_RJE	77
#define IPPORT_FINGER	79
#define IPPORT_TTYLINK	87
#define IPPORT_SUPDUP	95
#define IPPORT_EXECSERVER	512
#define IPPORT_LOGINSERVER	513
#define IPPORT_CMDSERVER	514
#define IPPORT_EFSSERVER	520
#define IPPORT_BIFFUDP	512
#define IPPORT_WHOSERVER	513
#define IPPORT_ROUTESERVER	520
#define IPPORT_RESERVED	1024
#define IMPLINK_IP	155
#define IMPLINK_LOWEXPER	156
#define IMPLINK_HIGHEXPER       158
#ifndef s_addr
typedef struct in_addr {
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
} IN_ADDR, *PIN_ADDR;
#endif
#define IN_CLASSA(i)	((long)(i)&0x80000000)
#define IN_CLASSA_NET	0xff000000
#define IN_CLASSA_NSHIFT	24
#define IN_CLASSA_HOST	0x00ffffff
#define IN_CLASSA_MAX	128
#define IN_CLASSB(i)	(((long)(i)&0xc0000000)==0x80000000)
#define IN_CLASSB_NET	   0xffff0000
#define IN_CLASSB_NSHIFT	16
#define IN_CLASSB_HOST	  0x0000ffff
#define IN_CLASSB_MAX	   65536
#define IN_CLASSC(i)	(((long)(i)&0xe0000000)==0xc0000000)
#define IN_CLASSC_NET	   0xffffff00
#define IN_CLASSC_NSHIFT	8
#define IN_CLASSC_HOST	  0xff
#define INADDR_ANY	      (u_long)0
#define INADDR_LOOPBACK	 0x7f000001
#define INADDR_BROADCAST	(u_long)0xffffffff
#define INADDR_NONE	0xffffffff
struct sockaddr_in {
	short	sin_family;
	u_short	sin_port;
	struct	in_addr sin_addr;
	char	sin_zero[8];
};
#define WSADESCRIPTION_LEN	256
#define WSASYS_STATUS_LEN	128
typedef struct WSAData {
	WORD	wVersion;
	WORD	wHighVersion;
	char	szDescription[WSADESCRIPTION_LEN+1];
	char	szSystemStatus[WSASYS_STATUS_LEN+1];
	unsigned short	iMaxSockets;
	unsigned short	iMaxUdpDg;
	char * 	lpVendorInfo;
} WSADATA;
typedef WSADATA *LPWSADATA;

#if ! (defined (__INSIDE_CYGWIN__) || defined (__INSIDE_MSYS__))
#define IP_OPTIONS	1
#define SO_DEBUG	1
#define SO_ACCEPTCONN	2
#define SO_REUSEADDR	4
#define SO_KEEPALIVE	8
#define SO_DONTROUTE	16
#define SO_BROADCAST	32
#define SO_USELOOPBACK	64
#define SO_LINGER	128
#define SO_OOBINLINE	256
#define SO_DONTLINGER	(u_int)(~SO_LINGER)
#define SO_SNDBUF	0x1001
#define SO_RCVBUF	0x1002
#define SO_SNDLOWAT	0x1003
#define SO_RCVLOWAT	0x1004
#define SO_SNDTIMEO	0x1005
#define SO_RCVTIMEO	0x1006
#define SO_ERROR	0x1007
#define SO_TYPE	0x1008
#endif /* ! (__INSIDE_CYGWIN__ || __INSIDE_MSYS__) */

#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR	(-1)
#define SOCK_STREAM	1
#define SOCK_DGRAM	2
#define SOCK_RAW	3
#define SOCK_RDM	4
#define SOCK_SEQPACKET	5
#define TCP_NODELAY	0x0001
#define AF_UNSPEC	0
#define AF_UNIX	1
#define AF_INET	2
#define AF_IMPLINK	3
#define AF_PUP	4
#define AF_CHAOS	5
#define AF_IPX	6
#define AF_NS	6
#define AF_ISO	7
#define AF_OSI	AF_ISO
#define AF_ECMA	8
#define AF_DATAKIT	9
#define AF_CCITT	10
#define AF_SNA	11
#define AF_DECnet	12
#define AF_DLI	13
#define AF_LAT	14
#define AF_HYLINK	15
#define AF_APPLETALK	16
#define AF_NETBIOS	17
#define AF_VOICEVIEW	18
#define	AF_FIREFOX	19
#define	AF_UNKNOWN1	20
#define	AF_BAN	21
#define AF_ATM	22
#define AF_INET6	23
#define AF_CLUSTER  24
#define AF_12844    25
#define AF_IRDA     26
#define AF_NETDES   28
#if !(defined (__INSIDE_CYGWIN__) || defined (__INSIDE_MSYS__))
#define AF_MAX	29
struct sockaddr {
	u_short sa_family;
	char	sa_data[14];
};
#endif /* ! (__INSIDE_CYGWIN__ || __INSIDE_MSYS__) */

/* Portable IPv6/IPv4 version of sockaddr.
   Uses padding to force 8 byte alignment
   and maximum size of 128 bytes */
struct sockaddr_storage {
    short ss_family;
    char __ss_pad1[6];    /* pad to 8 */
    __int64 __ss_align; /* force alignment */
    char __ss_pad2[112];  /* pad to 128 */
};

struct sockproto {
	u_short sp_family;
	u_short sp_protocol;
};
#define PF_UNSPEC	AF_UNSPEC
#define PF_UNIX	AF_UNIX
#define PF_INET	AF_INET
#define PF_IMPLINK	AF_IMPLINK
#define PF_PUP	AF_PUP
#define PF_CHAOS	AF_CHAOS
#define PF_NS	AF_NS
#define PF_IPX	AF_IPX
#define PF_ISO	AF_ISO
#define PF_OSI	AF_OSI
#define PF_ECMA	AF_ECMA
#define PF_DATAKIT	AF_DATAKIT
#define PF_CCITT	AF_CCITT
#define PF_SNA	AF_SNA
#define PF_DECnet	AF_DECnet
#define PF_DLI	AF_DLI
#define PF_LAT	AF_LAT
#define PF_HYLINK	AF_HYLINK
#define PF_APPLETALK	AF_APPLETALK
#define PF_VOICEVIEW	AF_VOICEVIEW
#define PF_FIREFOX	AF_FIREFOX
#define PF_UNKNOWN1	AF_UNKNOWN1
#define PF_BAN	AF_BAN
#define PF_ATM	AF_ATM
#define PF_INET6	AF_INET6
#define PF_MAX	AF_MAX
#define SOL_SOCKET	0xffff
#if ! (defined (__INSIDE_CYGWIN__) || defined (__INSIDE_MSYS__))
#define SOMAXCONN	0x7fffffff /* (5) in WinSock1.1 */
#define MSG_OOB	1
#define MSG_PEEK	2
#define MSG_DONTROUTE	4
#define MSG_WAITALL	8
#endif  /* ! (__INSIDE_CYGWIN__ || __INSIDE_MSYS__) */
#define MSG_MAXIOVLEN	16
#define MSG_PARTIAL	0x8000
#define MAXGETHOSTSTRUCT	1024

#define FD_READ_BIT      0
#define FD_READ          (1 << FD_READ_BIT)
#define FD_WRITE_BIT     1
#define FD_WRITE         (1 << FD_WRITE_BIT)
#define FD_OOB_BIT       2
#define FD_OOB           (1 << FD_OOB_BIT)
#define FD_ACCEPT_BIT    3
#define FD_ACCEPT        (1 << FD_ACCEPT_BIT)
#define FD_CONNECT_BIT   4
#define FD_CONNECT       (1 << FD_CONNECT_BIT)
#define FD_CLOSE_BIT     5
#define FD_CLOSE         (1 << FD_CLOSE_BIT)
/* winsock1.1 defines stop at FD_CLOSE (32) */
#define FD_QOS_BIT       6
#define FD_QOS           (1 << FD_QOS_BIT)
#define FD_GROUP_QOS_BIT 7
#define FD_GROUP_QOS     (1 << FD_GROUP_QOS_BIT)
#define FD_ROUTING_INTERFACE_CHANGE_BIT 8
#define FD_ROUTING_INTERFACE_CHANGE     (1 << FD_ROUTING_INTERFACE_CHANGE_BIT)
#define FD_ADDRESS_LIST_CHANGE_BIT 9
#define FD_ADDRESS_LIST_CHANGE     (1 << FD_ADDRESS_LIST_CHANGE_BIT)
#define FD_MAX_EVENTS    10
#define FD_ALL_EVENTS    ((1 << FD_MAX_EVENTS) - 1)

#ifndef WSABASEERR
#define WSABASEERR	10000
#define WSAEINTR	(WSABASEERR+4)
#define WSAEBADF	(WSABASEERR+9)
#define WSAEACCES	(WSABASEERR+13)
#define WSAEFAULT	(WSABASEERR+14)
#define WSAEINVAL	(WSABASEERR+22)
#define WSAEMFILE	(WSABASEERR+24)
#define WSAEWOULDBLOCK	(WSABASEERR+35)
#define WSAEINPROGRESS	(WSABASEERR+36) /* deprecated on WinSock2 */
#define WSAEALREADY	(WSABASEERR+37)
#define WSAENOTSOCK	(WSABASEERR+38)
#define WSAEDESTADDRREQ	(WSABASEERR+39)
#define WSAEMSGSIZE	(WSABASEERR+40)
#define WSAEPROTOTYPE	(WSABASEERR+41)
#define WSAENOPROTOOPT	(WSABASEERR+42)
#define WSAEPROTONOSUPPORT	(WSABASEERR+43)
#define WSAESOCKTNOSUPPORT	(WSABASEERR+44)
#define WSAEOPNOTSUPP	(WSABASEERR+45)
#define WSAEPFNOSUPPORT	(WSABASEERR+46)
#define WSAEAFNOSUPPORT	(WSABASEERR+47)
#define WSAEADDRINUSE	(WSABASEERR+48)
#define WSAEADDRNOTAVAIL	(WSABASEERR+49)
#define WSAENETDOWN	(WSABASEERR+50)
#define WSAENETUNREACH	(WSABASEERR+51)
#define WSAENETRESET	(WSABASEERR+52)
#define WSAECONNABORTED	(WSABASEERR+53)
#define WSAECONNRESET	(WSABASEERR+54)
#define WSAENOBUFS	(WSABASEERR+55)
#define WSAEISCONN	(WSABASEERR+56)
#define WSAENOTCONN	(WSABASEERR+57)
#define WSAESHUTDOWN	(WSABASEERR+58)
#define WSAETOOMANYREFS	(WSABASEERR+59)
#define WSAETIMEDOUT	(WSABASEERR+60)
#define WSAECONNREFUSED	(WSABASEERR+61)
#define WSAELOOP	(WSABASEERR+62)
#define WSAENAMETOOLONG	(WSABASEERR+63)
#define WSAEHOSTDOWN	(WSABASEERR+64)
#define WSAEHOSTUNREACH	(WSABASEERR+65)
#define WSAENOTEMPTY	(WSABASEERR+66)
#define WSAEPROCLIM	(WSABASEERR+67)
#define WSAEUSERS	(WSABASEERR+68)
#define WSAEDQUOT	(WSABASEERR+69)
#define WSAESTALE	(WSABASEERR+70)
#define WSAEREMOTE	(WSABASEERR+71)
#define WSAEDISCON	(WSABASEERR+101)
#define WSASYSNOTREADY	(WSABASEERR+91)
#define WSAVERNOTSUPPORTED	(WSABASEERR+92)
#define WSANOTINITIALISED	(WSABASEERR+93)
#define WSAHOST_NOT_FOUND	(WSABASEERR+1001)
#define WSATRY_AGAIN	(WSABASEERR+1002)
#define WSANO_RECOVERY	(WSABASEERR+1003)
#define WSANO_DATA	(WSABASEERR+1004)

/* WinSock2 specific error codes */
#define WSAENOMORE	(WSABASEERR+102)
#define WSAECANCELLED	(WSABASEERR+103)
#define WSAEINVALIDPROCTABLE	(WSABASEERR+104)
#define WSAEINVALIDPROVIDER	(WSABASEERR+105)
#define WSAEPROVIDERFAILEDINIT	(WSABASEERR+106)
#define WSASYSCALLFAILURE	(WSABASEERR+107)
#define WSASERVICE_NOT_FOUND	(WSABASEERR+108)
#define WSATYPE_NOT_FOUND	(WSABASEERR+109)
#define WSA_E_NO_MORE	(WSABASEERR+110)
#define WSA_E_CANCELLED	(WSABASEERR+111)
#define WSAEREFUSED	(WSABASEERR+112)

/* WS QualityofService errors */
#define WSA_QOS_RECEIVERS	(WSABASEERR + 1005)
#define WSA_QOS_SENDERS	(WSABASEERR + 1006)
#define WSA_QOS_NO_SENDERS	(WSABASEERR + 1007)
#define WSA_QOS_NO_RECEIVERS	(WSABASEERR + 1008)
#define WSA_QOS_REQUEST_CONFIRMED	(WSABASEERR + 1009)
#define WSA_QOS_ADMISSION_FAILURE	(WSABASEERR + 1010)
#define WSA_QOS_POLICY_FAILURE	(WSABASEERR + 1011)
#define WSA_QOS_BAD_STYLE	(WSABASEERR + 1012)
#define WSA_QOS_BAD_OBJECT	(WSABASEERR + 1013)
#define WSA_QOS_TRAFFIC_CTRL_ERROR	(WSABASEERR + 1014)
#define WSA_QOS_GENERIC_ERROR	(WSABASEERR + 1015)
#define WSA_QOS_ESERVICETYPE	(WSABASEERR + 1016)
#define WSA_QOS_EFLOWSPEC	(WSABASEERR + 1017)
#define WSA_QOS_EPROVSPECBUF	(WSABASEERR + 1018)
#define WSA_QOS_EFILTERSTYLE	(WSABASEERR + 1019)
#define WSA_QOS_EFILTERTYPE	(WSABASEERR + 1020)
#define WSA_QOS_EFILTERCOUNT	(WSABASEERR + 1021)
#define WSA_QOS_EOBJLENGTH	(WSABASEERR + 1022)
#define WSA_QOS_EFLOWCOUNT	(WSABASEERR + 1023)
#define WSA_QOS_EUNKOWNPSOBJ	(WSABASEERR + 1024)
#define WSA_QOS_EPOLICYOBJ	(WSABASEERR + 1025)
#define WSA_QOS_EFLOWDESC	(WSABASEERR + 1026)
#define WSA_QOS_EPSFLOWSPEC	(WSABASEERR + 1027)
#define WSA_QOS_EPSFILTERSPEC	(WSABASEERR + 1028)
#define WSA_QOS_ESDMODEOBJ	(WSABASEERR + 1029)
#define WSA_QOS_ESHAPERATEOBJ	(WSABASEERR + 1030)
#define WSA_QOS_RESERVED_PETYPE	(WSABASEERR + 1031)

#endif /* !WSABASEERR */

#define WSANO_ADDRESS	WSANO_DATA
#if !(defined (__INSIDE_CYGWIN__) || defined (__INSIDE_MSYS__))
#define h_errno WSAGetLastError()
#define HOST_NOT_FOUND	WSAHOST_NOT_FOUND
#define TRY_AGAIN	WSATRY_AGAIN
#define NO_RECOVERY	WSANO_RECOVERY
#define NO_DATA	WSANO_DATA
#define NO_ADDRESS	WSANO_ADDRESS
#endif /* ! (__INSIDE_CYGWIN__ || __INSIDE_MSYS__) */
SOCKET PASCAL accept(SOCKET,struct sockaddr*,int*);
int PASCAL bind(SOCKET,const struct sockaddr*,int);
int PASCAL closesocket(SOCKET);
int PASCAL connect(SOCKET,const struct sockaddr*,int);
int PASCAL ioctlsocket(SOCKET,long,u_long *);
int PASCAL getpeername(SOCKET,struct sockaddr*,int*);
int PASCAL getsockname(SOCKET,struct sockaddr*,int*);
int PASCAL getsockopt(SOCKET,int,int,char*,int*);
unsigned long PASCAL inet_addr(const char*);
DECLARE_STDCALL_P(char *) inet_ntoa(struct in_addr);
int PASCAL listen(SOCKET,int);
int PASCAL recv(SOCKET,char*,int,int);
int PASCAL recvfrom(SOCKET,char*,int,int,struct sockaddr*,int*);
int PASCAL send(SOCKET,const char*,int,int);
int PASCAL sendto(SOCKET,const char*,int,int,const struct sockaddr*,int);
int PASCAL setsockopt(SOCKET,int,int,const char*,int);
int PASCAL shutdown(SOCKET,int);
SOCKET PASCAL socket(int,int,int);
DECLARE_STDCALL_P(struct hostent *) gethostbyaddr(const char*,int,int);
DECLARE_STDCALL_P(struct hostent *) gethostbyname(const char*);
DECLARE_STDCALL_P(struct servent *) getservbyport(int,const char*);
DECLARE_STDCALL_P(struct servent *) getservbyname(const char*,const char*);
DECLARE_STDCALL_P(struct protoent *) getprotobynumber(int);
DECLARE_STDCALL_P(struct protoent *) getprotobyname(const char*);
int PASCAL WSAStartup(WORD,LPWSADATA);
int PASCAL WSACleanup(void);
void PASCAL WSASetLastError(int);
int PASCAL WSAGetLastError(void);
/*
 * Pseudo-blocking functions are deprecated in WinSock2
 * spec. Use threads instead.
 */
BOOL PASCAL WSAIsBlocking(void);
int PASCAL WSAUnhookBlockingHook(void);
FARPROC PASCAL WSASetBlockingHook(FARPROC);
int PASCAL WSACancelBlockingCall(void);

HANDLE PASCAL WSAAsyncGetServByName(HWND,u_int,const char*,const char*,char*,int);
HANDLE PASCAL WSAAsyncGetServByPort(HWND,u_int,int,const char*,char*,int);
HANDLE PASCAL WSAAsyncGetProtoByName(HWND,u_int,const char*,char*,int);
HANDLE PASCAL WSAAsyncGetProtoByNumber(HWND,u_int,int,char*,int);
HANDLE PASCAL WSAAsyncGetHostByName(HWND,u_int,const char*,char*,int);
HANDLE PASCAL WSAAsyncGetHostByAddr(HWND,u_int,const char*,int,int,char*,int);
int PASCAL WSACancelAsyncRequest(HANDLE);
int PASCAL WSAAsyncSelect(SOCKET,HWND,u_int,long);
#if ! (defined (__INSIDE_CYGWIN__) || defined (__INSIDE_MSYS__))
u_long PASCAL htonl(u_long);
u_long PASCAL ntohl(u_long);
u_short PASCAL htons(u_short);
u_short PASCAL ntohs(u_short);
#ifndef _SYS_SELECT_H /* Work around for Linux Compilers */
#define _SYS_SELECT_H
int PASCAL select(int nfds,fd_set*,fd_set*,fd_set*,const struct timeval*);
#endif
#endif /* ! (__INSIDE_CYGWIN__ || __INSIDE_MSYS__) */

int PASCAL gethostname(char*,int);

#define WSAMAKEASYNCREPLY(b,e)	MAKELONG(b,e)
#define WSAMAKESELECTREPLY(e,error)	MAKELONG(e,error)
#define WSAGETASYNCBUFLEN(l)	LOWORD(l)
#define WSAGETASYNCERROR(l)	HIWORD(l)
#define WSAGETSELECTEVENT(l)	LOWORD(l)
#define WSAGETSELECTERROR(l)	HIWORD(l)

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr *PSOCKADDR;
typedef struct sockaddr *LPSOCKADDR;
typedef struct sockaddr_storage SOCKADDR_STORAGE, *PSOCKADDR_STORAGE;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr_in *PSOCKADDR_IN;
typedef struct sockaddr_in *LPSOCKADDR_IN;
typedef struct linger LINGER;
typedef struct linger *PLINGER;
typedef struct linger *LPLINGER;
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

/* winsock2 additions */
#define ADDR_ANY	INADDR_ANY

#define	IN_CLASSD(i)	(((long)(i) & 0xf0000000) == 0xe0000000)
#define	IN_CLASSD_NET	0xf0000000
#define	IN_CLASSD_NSHIFT	28
#define	IN_CLASSD_HOST	0x0fffffff
#define	IN_MULTICAST(i)	IN_CLASSD(i)

#define	FROM_PROTOCOL_INFO	(-1)

#define SO_DONTLINGER	(u_int)(~SO_LINGER)
#define SO_GROUP_ID	0x2001
#define SO_GROUP_PRIORITY	0x2002
#define SO_MAX_MSG_SIZE	0x2003
#define SO_PROTOCOL_INFOA	0x2004
#define SO_PROTOCOL_INFOW	0x2005
#ifdef UNICODE
#define SO_PROTOCOL_INFO	SO_PROTOCOL_INFOW
#else
#define SO_PROTOCOL_INFO	SO_PROTOCOL_INFOA
#endif
#define PVD_CONFIG        0x3001

#define	MSG_INTERRUPT	0x10
#define	MSG_MAXIOVLEN	16

#define	WSAAPI	WINAPI
#define WSAEVENT	HANDLE
#define	LPWSAEVENT	LPHANDLE
#define	WSAOVERLAPPED	OVERLAPPED
typedef	struct _OVERLAPPED	*LPWSAOVERLAPPED;

#define	WSA_IO_PENDING	(ERROR_IO_PENDING)
#define	WSA_IO_INCOMPLETE	(ERROR_IO_INCOMPLETE)
#define	WSA_INVALID_HANDLE	(ERROR_INVALID_HANDLE)
#define	WSA_INVALID_PARAMETER	(ERROR_INVALID_PARAMETER)
#define	WSA_NOT_ENOUGH_MEMORY	(ERROR_NOT_ENOUGH_MEMORY)
#define	WSA_OPERATION_ABORTED	(ERROR_OPERATION_ABORTED)

#define	WSA_INVALID_EVENT	((WSAEVENT)NULL)
#define	WSA_MAXIMUM_WAIT_EVENTS	(MAXIMUM_WAIT_OBJECTS)
#define	WSA_WAIT_FAILED	((DWORD)-1L)
#define	WSA_WAIT_EVENT_0	(WAIT_OBJECT_0)
#define	WSA_WAIT_IO_COMPLETION	(WAIT_IO_COMPLETION)
#define	WSA_WAIT_TIMEOUT	(WAIT_TIMEOUT)
#define	WSA_INFINITE	(INFINITE)

typedef struct _WSABUF {
	unsigned long len;
	char *buf;
} WSABUF, *LPWSABUF;

typedef enum
{
	BestEffortService,
	ControlledLoadService,
	PredictiveService,
	GuaranteedDelayService,
	GuaranteedService
} GUARANTEE;

#include <qos.h>
typedef struct _QualityOfService
{
	FLOWSPEC	SendingFlowspec;
	FLOWSPEC	ReceivingFlowspec;
	WSABUF	ProviderSpecific;
} QOS, *LPQOS;

#define	CF_ACCEPT	0x0000
#define	CF_REJECT	0x0001
#define	CF_DEFER	0x0002
#define	SD_RECEIVE	0x00
#define	SD_SEND	0x01
#define	SD_BOTH	0x02
typedef unsigned int	GROUP;

#define SG_UNCONSTRAINED_GROUP	0x01
#define SG_CONSTRAINED_GROUP	0x02
typedef struct _WSANETWORKEVENTS {
	long	lNetworkEvents;
	int	iErrorCode[FD_MAX_EVENTS];
} WSANETWORKEVENTS, *LPWSANETWORKEVENTS;

#define	MAX_PROTOCOL_CHAIN 7

#define	BASE_PROTOCOL      1
#define	LAYERED_PROTOCOL   0

typedef enum _WSAESETSERVICEOP
{
	RNRSERVICE_REGISTER=0,
	RNRSERVICE_DEREGISTER,
	RNRSERVICE_DELETE
} WSAESETSERVICEOP, *PWSAESETSERVICEOP, *LPWSAESETSERVICEOP;

typedef struct _AFPROTOCOLS {
	INT iAddressFamily;
	INT iProtocol;
} AFPROTOCOLS, *PAFPROTOCOLS, *LPAFPROTOCOLS;

typedef enum _WSAEcomparator
{
	COMP_EQUAL = 0,
	COMP_NOTLESS
} WSAECOMPARATOR, *PWSAECOMPARATOR, *LPWSAECOMPARATOR;

typedef struct _WSAVersion
{
	DWORD	dwVersion;
	WSAECOMPARATOR	ecHow;
} WSAVERSION, *PWSAVERSION, *LPWSAVERSION;

#ifndef __CSADDR_T_DEFINED /* also in nspapi.h */
#define __CSADDR_T_DEFINED
typedef struct _SOCKET_ADDRESS {
	LPSOCKADDR lpSockaddr;
	INT iSockaddrLength;
} SOCKET_ADDRESS,*PSOCKET_ADDRESS,*LPSOCKET_ADDRESS;
typedef struct _CSADDR_INFO {
	SOCKET_ADDRESS LocalAddr;
	SOCKET_ADDRESS RemoteAddr;
	INT iSocketType;
	INT iProtocol;
} CSADDR_INFO,*PCSADDR_INFO,*LPCSADDR_INFO;
#endif

typedef struct _SOCKET_ADDRESS_LIST {
    INT             iAddressCount;
    SOCKET_ADDRESS  Address[1];
} SOCKET_ADDRESS_LIST, * LPSOCKET_ADDRESS_LIST;

#ifndef __BLOB_T_DEFINED /* also in wtypes.h and nspapi.h */
#define __BLOB_T_DEFINED
/* wine is using a diff define */
#ifndef _tagBLOB_DEFINED
#define _tagBLOB_DEFINED
#define _BLOB_DEFINED
#define _LPBLOB_DEFINED

typedef struct _BLOB {
	ULONG	cbSize;
	BYTE	*pBlobData;
} BLOB,*PBLOB,*LPBLOB;
#endif
#endif

typedef struct _WSAQuerySetA
{
	DWORD	dwSize;
	LPSTR	lpszServiceInstanceName;
	LPGUID	lpServiceClassId;
	LPWSAVERSION	lpVersion;
	LPSTR	lpszComment;
	DWORD	dwNameSpace;
	LPGUID	lpNSProviderId;
	LPSTR	lpszContext;
	DWORD	dwNumberOfProtocols;
	LPAFPROTOCOLS	lpafpProtocols;
	LPSTR	lpszQueryString;
	DWORD	dwNumberOfCsAddrs;
	LPCSADDR_INFO	lpcsaBuffer;
	DWORD	dwOutputFlags;
	LPBLOB	lpBlob;
} WSAQUERYSETA, *PWSAQUERYSETA, *LPWSAQUERYSETA;

typedef struct _WSAQuerySetW
{
	DWORD	dwSize;
	LPWSTR	lpszServiceInstanceName;
	LPGUID	lpServiceClassId;
	LPWSAVERSION	lpVersion;
	LPWSTR	lpszComment;
	DWORD	dwNameSpace;
	LPGUID	lpNSProviderId;
	LPWSTR	lpszContext;
	DWORD	dwNumberOfProtocols;
	LPAFPROTOCOLS	lpafpProtocols;
	LPWSTR	lpszQueryString;
	DWORD	dwNumberOfCsAddrs;
	LPCSADDR_INFO	lpcsaBuffer;
	DWORD	dwOutputFlags;
	LPBLOB	lpBlob;
} WSAQUERYSETW, *PWSAQUERYSETW, *LPWSAQUERYSETW;

#ifdef UNICODE
typedef WSAQUERYSETW WSAQUERYSET;
typedef PWSAQUERYSETW PWSAQUERYSET;
typedef LPWSAQUERYSETW LPWSAQUERYSET;
#else
typedef WSAQUERYSETA WSAQUERYSET;
typedef PWSAQUERYSETA PWSAQUERYSET;
typedef LPWSAQUERYSETA LPWSAQUERYSET;
#endif

typedef struct _WSANSClassInfoA
{
	LPSTR	lpszName;
	DWORD	dwNameSpace;
	DWORD	dwValueType;
	DWORD	dwValueSize;
	LPVOID	lpValue;
} WSANSCLASSINFOA, *PWSANSCLASSINFOA, *LPWSANSCLASSINFOA;

typedef struct _WSANSClassInfoW
{
	LPWSTR	lpszName;
	DWORD	dwNameSpace;
	DWORD	dwValueType;
	DWORD	dwValueSize;
	LPVOID	lpValue;
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

typedef struct _WSAServiceClassInfoA
{
	LPGUID	lpServiceClassId;
	LPSTR	lpszServiceClassName;
	DWORD	dwCount;
	LPWSANSCLASSINFOA	lpClassInfos;
} WSASERVICECLASSINFOA, *PWSASERVICECLASSINFOA, *LPWSASERVICECLASSINFOA;

typedef struct _WSAServiceClassInfoW
{
	LPGUID	lpServiceClassId;
	LPWSTR	lpszServiceClassName;
	DWORD	dwCount;
	LPWSANSCLASSINFOW	lpClassInfos;
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
	GUID	NSProviderId;
	DWORD	dwNameSpace;
	BOOL	fActive;
	DWORD	dwVersion;
	LPSTR	lpszIdentifier;
} WSANAMESPACE_INFOA, *PWSANAMESPACE_INFOA, *LPWSANAMESPACE_INFOA;

typedef struct _WSANAMESPACE_INFOW {
	GUID	NSProviderId;
	DWORD	dwNameSpace;
	BOOL	fActive;
	DWORD	dwVersion;
	LPWSTR	lpszIdentifier;
} WSANAMESPACE_INFOW, *PWSANAMESPACE_INFOW, *LPWSANAMESPACE_INFOW;

#ifdef UNICODE
typedef WSANAMESPACE_INFOW WSANAMESPACE_INFO;
typedef PWSANAMESPACE_INFOW PWSANAMESPACE_INFO;
typedef LPWSANAMESPACE_INFOW LPWSANAMESPACE_INFO;
#else
typedef WSANAMESPACE_INFOA WSANAMESPACE_INFO;
typedef PWSANAMESPACE_INFOA PWSANAMESPACE_INFO;
typedef LPWSANAMESPACE_INFOA LPWSANAMESPACE_INFO;
#endif

typedef struct _WSAPROTOCOLCHAIN {
	int ChainLen;
	DWORD ChainEntries[MAX_PROTOCOL_CHAIN];
} WSAPROTOCOLCHAIN, *LPWSAPROTOCOLCHAIN;

#define WSAPROTOCOL_LEN  255

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
	WCHAR  szProtocol[WSAPROTOCOL_LEN+1];
} WSAPROTOCOL_INFOW, * LPWSAPROTOCOL_INFOW;

typedef int (CALLBACK *LPCONDITIONPROC)(LPWSABUF, LPWSABUF, LPQOS, LPQOS, LPWSABUF, LPWSABUF, GROUP *, DWORD);
typedef void (WINAPI *LPWSAOVERLAPPED_COMPLETION_ROUTINE)(DWORD, DWORD, LPWSAOVERLAPPED, DWORD);


#ifdef UNICODE
typedef WSAPROTOCOL_INFOW WSAPROTOCOL_INFO;
typedef LPWSAPROTOCOL_INFOW LPWSAPROTOCOL_INFO;
#else
typedef WSAPROTOCOL_INFOA WSAPROTOCOL_INFO;
typedef LPWSAPROTOCOL_INFOA LPWSAPROTOCOL_INFO;
#endif

/* Needed for XP & .NET Server function WSANSPIoctl.  */
typedef enum _WSACOMPLETIONTYPE {
    NSP_NOTIFY_IMMEDIATELY = 0,
    NSP_NOTIFY_HWND,
    NSP_NOTIFY_EVENT,
    NSP_NOTIFY_PORT,
    NSP_NOTIFY_APC
} WSACOMPLETIONTYPE, * PWSACOMPLETIONTYPE, * LPWSACOMPLETIONTYPE;
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

#define PFL_MULTIPLE_PROTO_ENTRIES          0x00000001
#define PFL_RECOMMENDED_PROTO_ENTRY         0x00000002
#define PFL_HIDDEN                          0x00000004
#define PFL_MATCHES_PROTOCOL_ZERO           0x00000008
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

#define BIGENDIAN                           0x0000
#define LITTLEENDIAN                        0x0001

#define SECURITY_PROTOCOL_NONE              0x0000
#define JL_SENDER_ONLY    0x01
#define JL_RECEIVER_ONLY  0x02
#define JL_BOTH           0x04
#define WSA_FLAG_OVERLAPPED           0x01
#define WSA_FLAG_MULTIPOINT_C_ROOT    0x02
#define WSA_FLAG_MULTIPOINT_C_LEAF    0x04
#define WSA_FLAG_MULTIPOINT_D_ROOT    0x08
#define WSA_FLAG_MULTIPOINT_D_LEAF    0x10
#define IOC_UNIX                      0x00000000
#define IOC_WS2                       0x08000000
#define IOC_PROTOCOL                  0x10000000
#define IOC_VENDOR                    0x18000000

#define _WSAIO(x,y)                   (IOC_VOID|(x)|(y))
#define _WSAIOR(x,y)                  (IOC_OUT|(x)|(y))
#define _WSAIOW(x,y)                  (IOC_IN|(x)|(y))
#define _WSAIORW(x,y)                 (IOC_INOUT|(x)|(y))

#define SIO_ASSOCIATE_HANDLE          _WSAIOW(IOC_WS2,1)
#define SIO_ENABLE_CIRCULAR_QUEUEING  _WSAIO(IOC_WS2,2)
#define SIO_FIND_ROUTE                _WSAIOR(IOC_WS2,3)
#define SIO_FLUSH                     _WSAIO(IOC_WS2,4)
#define SIO_GET_BROADCAST_ADDRESS     _WSAIOR(IOC_WS2,5)
#define SIO_GET_EXTENSION_FUNCTION_POINTER  _WSAIORW(IOC_WS2,6)
#define SIO_GET_QOS                   _WSAIORW(IOC_WS2,7)
#define SIO_GET_GROUP_QOS             _WSAIORW(IOC_WS2,8)
#define SIO_MULTIPOINT_LOOPBACK       _WSAIOW(IOC_WS2,9)
#define SIO_MULTICAST_SCOPE           _WSAIOW(IOC_WS2,10)
#define SIO_SET_QOS                   _WSAIOW(IOC_WS2,11)
#define SIO_SET_GROUP_QOS             _WSAIOW(IOC_WS2,12)
#define SIO_TRANSLATE_HANDLE          _WSAIORW(IOC_WS2,13)
#define SIO_ROUTING_INTERFACE_QUERY   _WSAIORW(IOC_WS2,20)
#define SIO_ROUTING_INTERFACE_CHANGE  _WSAIOW(IOC_WS2,21)
#define SIO_ADDRESS_LIST_QUERY        _WSAIOR(IOC_WS2,22)
#define SIO_ADDRESS_LIST_CHANGE       _WSAIO(IOC_WS2,23)
#define SIO_QUERY_TARGET_PNP_HANDLE   _WSAIOR(IOC_WS2,24)
#define SIO_NSP_NOTIFY_CHANGE         _WSAIOW(IOC_WS2,25)

#define TH_NETDEV	0x00000001
#define TH_TAPI	0x00000002

SOCKET WINAPI WSAAccept(SOCKET, struct sockaddr *, LPINT, LPCONDITIONPROC, DWORD);
INT WINAPI WSAAddressToStringA(LPSOCKADDR, DWORD, LPWSAPROTOCOL_INFOA, LPSTR, LPDWORD);
INT WINAPI WSAAddressToStringW(LPSOCKADDR, DWORD, LPWSAPROTOCOL_INFOW, LPWSTR, LPDWORD);
BOOL WINAPI WSACloseEvent(WSAEVENT);
int WINAPI WSAConnect(SOCKET, const struct sockaddr *, int, LPWSABUF, LPWSABUF, LPQOS, LPQOS);
WSAEVENT WINAPI WSACreateEvent(void);
int WINAPI WSADuplicateSocketA(SOCKET, DWORD, LPWSAPROTOCOL_INFOA);
int WINAPI WSADuplicateSocketW(SOCKET, DWORD, LPWSAPROTOCOL_INFOW);
INT WINAPI WSAEnumNameSpaceProvidersA(LPDWORD, LPWSANAMESPACE_INFOA);
INT WINAPI WSAEnumNameSpaceProvidersW(LPDWORD, LPWSANAMESPACE_INFOW);
int WINAPI WSAEnumNetworkEvents(SOCKET, WSAEVENT, LPWSANETWORKEVENTS);
int WINAPI WSAEnumProtocolsA(LPINT, LPWSAPROTOCOL_INFOA, LPDWORD);
int WINAPI WSAEnumProtocolsW(LPINT, LPWSAPROTOCOL_INFOW, LPDWORD);
int WINAPI WSAEventSelect(SOCKET, WSAEVENT, long);
BOOL WINAPI WSAGetOverlappedResult(SOCKET, LPWSAOVERLAPPED, LPDWORD, BOOL, LPDWORD);
BOOL WINAPI WSAGetQOSByName(SOCKET, LPWSABUF, LPQOS);
INT WINAPI WSAGetServiceClassInfoA(LPGUID, LPGUID, LPDWORD, LPWSASERVICECLASSINFOA);
INT WINAPI WSAGetServiceClassInfoW(LPGUID, LPGUID, LPDWORD, LPWSASERVICECLASSINFOW);
INT WINAPI WSAGetServiceClassNameByClassIdA(LPGUID, LPSTR, LPDWORD);
INT WINAPI WSAGetServiceClassNameByClassIdW(LPGUID, LPWSTR, LPDWORD);
int WINAPI WSAHtonl(SOCKET, unsigned long, unsigned long *);
int WINAPI WSAHtons(SOCKET, unsigned short, unsigned short *);
INT WINAPI WSAInstallServiceClassA(LPWSASERVICECLASSINFOA);
INT WINAPI WSAInstallServiceClassW(LPWSASERVICECLASSINFOW);
int WINAPI WSAIoctl(SOCKET, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
SOCKET WINAPI WSAJoinLeaf(SOCKET, const struct sockaddr *, int, LPWSABUF, LPWSABUF, LPQOS, LPQOS, DWORD);
INT WINAPI WSALookupServiceBeginA(LPWSAQUERYSETA, DWORD, LPHANDLE);
INT WINAPI WSALookupServiceBeginW(LPWSAQUERYSETW lpqsRestrictions, DWORD, LPHANDLE);
INT WINAPI WSALookupServiceNextA(HANDLE, DWORD, LPDWORD, LPWSAQUERYSETA);
INT WINAPI WSALookupServiceNextW(HANDLE, DWORD, LPDWORD, LPWSAQUERYSETW);
INT WINAPI WSALookupServiceEnd(HANDLE);
int WINAPI WSANSPIoctl(HANDLE,DWORD,LPVOID,DWORD,LPVOID,DWORD,LPDWORD,LPWSACOMPLETION); /* XP or .NET Server */
int WINAPI WSANtohl(SOCKET, unsigned long, unsigned long *);
int WINAPI WSANtohs(SOCKET, unsigned short, unsigned short *);
int WINAPI WSARecv(SOCKET, LPWSABUF, DWORD, LPDWORD, LPDWORD, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
int WINAPI WSARecvDisconnect(SOCKET, LPWSABUF);
int WINAPI WSARecvFrom(SOCKET, LPWSABUF, DWORD, LPDWORD, LPDWORD, struct sockaddr *, LPINT, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
INT WINAPI WSARemoveServiceClass(LPGUID);
BOOL WINAPI WSAResetEvent(WSAEVENT);
int WINAPI WSASend(SOCKET, LPWSABUF, DWORD, LPDWORD, DWORD, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
int WINAPI WSASendDisconnect(SOCKET, LPWSABUF);
int WINAPI WSASendTo(SOCKET, LPWSABUF, DWORD, LPDWORD, DWORD, const struct sockaddr *, int, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
BOOL WINAPI WSASetEvent(WSAEVENT);
INT WSAAPI WSASetServiceA(LPWSAQUERYSETA, WSAESETSERVICEOP, DWORD);
INT WINAPI WSASetServiceW(LPWSAQUERYSETW, WSAESETSERVICEOP, DWORD);
SOCKET WINAPI WSASocketA(int, int, int, LPWSAPROTOCOL_INFOA, GROUP, DWORD);
SOCKET WINAPI WSASocketW(int, int, int, LPWSAPROTOCOL_INFOW, GROUP, DWORD);
INT WINAPI WSAStringToAddressA(LPSTR, INT, LPWSAPROTOCOL_INFOA, LPSOCKADDR, LPINT);
INT WINAPI WSAStringToAddressW(LPWSTR, INT, LPWSAPROTOCOL_INFOW, LPSOCKADDR, LPINT);
DWORD WINAPI WSAWaitForMultipleEvents(DWORD, const WSAEVENT *, BOOL, DWORD, BOOL);
typedef SOCKET (WINAPI *LPFN_WSAACCEPT)(SOCKET, struct sockaddr *, LPINT, LPCONDITIONPROC, DWORD);
typedef INT (WINAPI *LPFN_WSAADDRESSTOSTRINGA)(LPSOCKADDR, DWORD, LPWSAPROTOCOL_INFOA, LPSTR, LPDWORD);
typedef INT (WINAPI *LPFN_WSAADDRESSTOSTRINGW)(LPSOCKADDR, DWORD, LPWSAPROTOCOL_INFOW, LPWSTR, LPDWORD);
typedef BOOL (WINAPI *LPFN_WSACLOSEEVENT)(WSAEVENT);
typedef int (WINAPI *LPFN_WSACONNECT)(SOCKET, const struct sockaddr *, int, LPWSABUF, LPWSABUF, LPQOS, LPQOS);
typedef WSAEVENT (WINAPI *LPFN_WSACREATEEVENT)(void);
typedef int (WINAPI *LPFN_WSADUPLICATESOCKETA)(SOCKET, DWORD, LPWSAPROTOCOL_INFOA);
typedef int (WINAPI *LPFN_WSADUPLICATESOCKETW)(SOCKET, DWORD, LPWSAPROTOCOL_INFOW);
typedef INT (WINAPI *LPFN_WSAENUMNAMESPACEPROVIDERSA)(LPDWORD, LPWSANAMESPACE_INFOA);
typedef INT (WINAPI *LPFN_WSAENUMNAMESPACEPROVIDERSW)(LPDWORD, LPWSANAMESPACE_INFOW);
typedef int (WINAPI *LPFN_WSAENUMNETWORKEVENTS)(SOCKET, WSAEVENT, LPWSANETWORKEVENTS);
typedef int (WINAPI *LPFN_WSAENUMPROTOCOLSA)(LPINT, LPWSAPROTOCOL_INFOA, LPDWORD);
typedef int (WINAPI *LPFN_WSAENUMPROTOCOLSW)(LPINT, LPWSAPROTOCOL_INFOW, LPDWORD);
typedef int (WINAPI *LPFN_WSAEVENTSELECT)(SOCKET, WSAEVENT, long);
typedef BOOL (WINAPI *LPFN_WSAGETOVERLAPPEDRESULT)(SOCKET, LPWSAOVERLAPPED, LPDWORD, BOOL, LPDWORD);
typedef BOOL (WINAPI *LPFN_WSAGETQOSBYNAME)(SOCKET, LPWSABUF, LPQOS);
typedef INT (WINAPI *LPFN_WSAGETSERVICECLASSINFOA)(LPGUID, LPGUID, LPDWORD, LPWSASERVICECLASSINFOA);
typedef INT (WINAPI *LPFN_WSAGETSERVICECLASSINFOW)(LPGUID, LPGUID, LPDWORD, LPWSASERVICECLASSINFOW);
typedef INT (WINAPI *LPFN_WSAGETSERVICECLASSNAMEBYCLASSIDA)(LPGUID, LPSTR, LPDWORD);
typedef INT (WINAPI *LPFN_WSAGETSERVICECLASSNAMEBYCLASSIDW)(LPGUID, LPWSTR, LPDWORD);
typedef int (WINAPI *LPFN_WSAHTONL)(SOCKET, unsigned long, unsigned long *);
typedef int (WINAPI *LPFN_WSAHTONS)(SOCKET, unsigned short, unsigned short *);
typedef INT (WINAPI *LPFN_WSAINSTALLSERVICECLASSA)(LPWSASERVICECLASSINFOA);
typedef INT (WINAPI *LPFN_WSAINSTALLSERVICECLASSW)(LPWSASERVICECLASSINFOW);
typedef int (WINAPI *LPFN_WSAIOCTL)(SOCKET, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
typedef SOCKET (WINAPI *LPFN_WSAJOINLEAF)(SOCKET, const struct sockaddr *, int, LPWSABUF, LPWSABUF, LPQOS, LPQOS, DWORD);
typedef INT (WINAPI *LPFN_WSALOOKUPSERVICEBEGINA)(LPWSAQUERYSETA, DWORD, LPHANDLE);
typedef INT (WINAPI *LPFN_WSALOOKUPSERVICEBEGINW)(LPWSAQUERYSETW, DWORD, LPHANDLE);
typedef INT (WINAPI *LPFN_WSALOOKUPSERVICENEXTA)(HANDLE, DWORD, LPDWORD, LPWSAQUERYSETA);
typedef INT (WINAPI *LPFN_WSALOOKUPSERVICENEXTW)(HANDLE, DWORD, LPDWORD, LPWSAQUERYSETW);
typedef INT (WINAPI *LPFN_WSALOOKUPSERVICEEND)(HANDLE);
typedef int (WINAPI *LPFN_WSANSPIoctl)(HANDLE, DWORD,LPVOID,DWORD,LPVOID,DWORD,LPDWORD,LPWSACOMPLETION);
typedef int (WINAPI *LPFN_WSANTOHL)(SOCKET, unsigned long, unsigned long *);
typedef int (WINAPI *LPFN_WSANTOHS)(SOCKET, unsigned short, unsigned short *);
typedef int (WINAPI *LPFN_WSARECV)(SOCKET, LPWSABUF, DWORD, LPDWORD, LPDWORD, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
typedef int (WINAPI *LPFN_WSARECVDISCONNECT)(SOCKET, LPWSABUF);
typedef int (WINAPI *LPFN_WSARECVFROM)(SOCKET, LPWSABUF, DWORD, LPDWORD, LPDWORD, struct sockaddr *, LPINT, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
typedef INT (WINAPI *LPFN_WSAREMOVESERVICECLASS)(LPGUID);
typedef BOOL (WINAPI *LPFN_WSARESETEVENT)(WSAEVENT);
typedef int (WINAPI *LPFN_WSASEND)(SOCKET, LPWSABUF, DWORD, LPDWORD, DWORD, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
typedef int (WINAPI *LPFN_WSASENDDISCONNECT)(SOCKET, LPWSABUF);
typedef int (WINAPI *LPFN_WSASENDTO)(SOCKET, LPWSABUF, DWORD, LPDWORD, DWORD, const struct sockaddr *, int, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE);
typedef BOOL (WINAPI *LPFN_WSASETEVENT)(WSAEVENT);
typedef INT (WINAPI *LPFN_WSASETSERVICEA)(LPWSAQUERYSETA, WSAESETSERVICEOP, DWORD);
typedef INT (WINAPI *LPFN_WSASETSERVICEW)(LPWSAQUERYSETW, WSAESETSERVICEOP, DWORD);
typedef SOCKET (WINAPI *LPFN_WSASOCKETA)(int, int, int, LPWSAPROTOCOL_INFOA, GROUP, DWORD);
typedef SOCKET (WINAPI *LPFN_WSASOCKETW)(int, int, int, LPWSAPROTOCOL_INFOW, GROUP, DWORD);
typedef INT (WINAPI *LPFN_WSASTRINGTOADDRESSA)(LPSTR, INT, LPWSAPROTOCOL_INFOA, LPSOCKADDR, LPINT);
typedef INT (WINAPI *LPFN_WSASTRINGTOADDRESSW)(LPWSTR, INT, LPWSAPROTOCOL_INFOW, LPSOCKADDR, LPINT);
typedef DWORD (WINAPI *LPFN_WSAWAITFORMULTIPLEEVENTS)(DWORD, const WSAEVENT *, BOOL, DWORD, BOOL);

#ifdef UNICODE
#define LPFN_WSAADDRESSTOSTRING LPFN_WSAADDRESSTOSTRINGW
#define LPFN_WSADUPLICATESOCKET LPFN_WSADUPLICATESOCKETW
#define LPFN_WSAENUMNAMESPACEPROVIDERS LPFN_WSAENUMNAMESPACEPROVIDERSW
#define LPFN_WSAENUMPROTOCOLS LPFN_WSAENUMPROTOCOLSW
#define LPFN_WSAGETSERVICECLASSINFO LPFN_WSAGETSERVICECLASSINFOW
#define LPFN_WSAGETSERVICECLASSNAMEBYCLASSID LPFN_WSAGETSERVICECLASSNAMEBYCLASSIDW
#define LPFN_WSAINSTALLSERVICECLASS LPFN_WSAINSTALLSERVICECLASSW
#define LPFN_WSALOOKUPSERVICEBEGIN LPFN_WSALOOKUPSERVICEBEGINW
#define LPFN_WSALOOKUPSERVICENEXT LPFN_WSALOOKUPSERVICENEXTW
#define LPFN_WSASETSERVICE  LPFN_WSASETSERVICEW
#define LPFN_WSASOCKET LPFN_WSASOCKETW
#define LPFN_WSASTRINGTOADDRESS LPFN_WSASTRINGTOADDRESSW
#define WSAAddressToString WSAAddressToStringW
#define WSADuplicateSocket WSADuplicateSocketW
#define WSAEnumNameSpaceProviders WSAEnumNameSpaceProvidersW
#define WSAEnumProtocols WSAEnumProtocolsW
#define WSAGetServiceClassInfo WSAGetServiceClassInfoW
#define WSAGetServiceClassNameByClassId WSAGetServiceClassNameByClassIdW
#define WSASetService WSASetServiceW
#define WSASocket WSASocketW
#define WSAStringToAddress WSAStringToAddressW
#define WSALookupServiceBegin WSALookupServiceBeginW
#define WSALookupServiceNext WSALookupServiceNextW
#define WSAInstallServiceClass WSAInstallServiceClassW
#else
#define LPFN_WSAADDRESSTOSTRING LPFN_WSAADDRESSTOSTRINGA
#define LPFN_WSADUPLICATESOCKET LPFN_WSADUPLICATESOCKETW
#define LPFN_WSAENUMNAMESPACEPROVIDERS LPFN_WSAENUMNAMESPACEPROVIDERSA
#define LPFN_WSAENUMPROTOCOLS LPFN_WSAENUMPROTOCOLSA
#define LPFN_WSAGETSERVICECLASSINFO LPFN_WSAGETSERVICECLASSINFOA
#define LPFN_WSAGETSERVICECLASSNAMEBYCLASSID LPFN_WSAGETSERVICECLASSNAMEBYCLASSIDA
#define LPFN_WSAINSTALLSERVICECLASS LPFN_WSAINSTALLSERVICECLASSA
#define LPFN_WSALOOKUPSERVICEBEGIN LPFN_WSALOOKUPSERVICEBEGINA
#define LPFN_WSALOOKUPSERVICENEXT LPFN_WSALOOKUPSERVICENEXTA
#define LPFN_WSASETSERVICE  LPFN_WSASETSERVICEA
#define LPFN_WSASOCKET LPFN_WSASOCKETA
#define LPFN_WSASTRINGTOADDRESS LPFN_WSASTRINGTOADDRESSA
#define WSAAddressToString WSAAddressToStringA
#define WSADuplicateSocket WSADuplicateSocketA
#define WSAEnumNameSpaceProviders WSAEnumNameSpaceProvidersA
#define WSAEnumProtocols WSAEnumProtocolsA
#define WSAGetServiceClassInfo WSAGetServiceClassInfoA
#define WSAGetServiceClassNameByClassId WSAGetServiceClassNameByClassIdA
#define WSAInstallServiceClass WSAInstallServiceClassA
#define WSALookupServiceBegin WSALookupServiceBeginA
#define WSALookupServiceNext WSALookupServiceNextA
#define WSASocket WSASocketA
#define WSAStringToAddress WSAStringToAddressA
#define WSASetService WSASetServiceA
#endif

#ifdef __cplusplus
}
#endif
#endif
