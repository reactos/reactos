/*
 *  ws2tcpip.h : TCP/IP specific extensions in Windows Sockets 2
 *
 * Portions Copyright (c) 1980, 1983, 1988, 1993
 * The Regents of the University of California.  All rights reserved.
 *
 */

#ifndef _WS2TCPIP_H
#define _WS2TCPIP_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#if (defined _WINSOCK_H && !defined _WINSOCK2_H)
#error "ws2tcpip.h is not compatable with winsock.h. Include winsock2.h instead."
#endif

#include <winsock2.h>
#ifdef  __cplusplus
extern "C" {
#endif

/* 
 * The IP_* macros are also defined in winsock.h, but some values are different there.
 * The values defined in winsock.h for 1.1 and used in wsock32.dll are consistent
 * with the original values Steve Deering defined in his document "IP Multicast Extensions
 * for 4.3BSD UNIX related systems (MULTICAST 1.2 Release)." However, these conflicted with
 * the definitions for some IPPROTO_IP level socket options already assigned by BSD,
 * so Berkeley changed all the values by adding 7.  WinSock2 (ws2_32.dll)  uses
 * the BSD 4.4 compatible values defined here.
 *
 * See also: msdn kb article Q257460
 * http://support.microsoft.com/support/kb/articles/Q257/4/60.asp
 */

/* This is also defined in winsock.h; value hasn't changed */
#define	IP_OPTIONS  1

#define	IP_HDRINCL  2
/*
 * These are also be defined in winsock.h,
 * but values have changed for WinSock2 interface
 */
#define IP_TOS			3   /* old (winsock 1.1) value 8 */
#define IP_TTL			4   /* old value 7 */
#define IP_MULTICAST_IF		9   /* old value 2 */
#define IP_MULTICAST_TTL	10  /* old value 3 */
#define IP_MULTICAST_LOOP	11  /* old value 4 */
#define IP_ADD_MEMBERSHIP	12  /* old value 5 */
#define IP_DROP_MEMBERSHIP	13  /* old value 6 */
#define IP_DONTFRAGMENT		14  /* old value 9 */
#define IP_ADD_SOURCE_MEMBERSHIP	15
#define IP_DROP_SOURCE_MEMBERSHIP	16
#define IP_BLOCK_SOURCE			17
#define IP_UNBLOCK_SOURCE		18
#define IP_PKTINFO			19

/*
 * As with BSD implementation, IPPROTO_IPV6 level socket options have
 * same values as IPv4 counterparts.
 */
#define IPV6_UNICAST_HOPS	4
#define IPV6_MULTICAST_IF	9
#define IPV6_MULTICAST_HOPS	10
#define IPV6_MULTICAST_LOOP	11
#define IPV6_ADD_MEMBERSHIP	12
#define IPV6_DROP_MEMBERSHIP	13
#define IPV6_JOIN_GROUP		IPV6_ADD_MEMBERSHIP
#define IPV6_LEAVE_GROUP	IPV6_DROP_MEMBERSHIP
#define IPV6_PKTINFO		19

#define IP_DEFAULT_MULTICAST_TTL 1 
#define IP_DEFAULT_MULTICAST_LOOP 1 
#define IP_MAX_MEMBERSHIPS 20 

#define TCP_EXPEDITED_1122  2

#define UDP_NOCHECKSUM 1

/* INTERFACE_INFO iiFlags */
#define IFF_UP  1
#define IFF_BROADCAST   2
#define IFF_LOOPBACK    4
#define IFF_POINTTOPOINT    8
#define IFF_MULTICAST   16

#define SIO_GET_INTERFACE_LIST  _IOR('t', 127, u_long)

#define INET_ADDRSTRLEN  16
#define INET6_ADDRSTRLEN 46

/* getnameinfo constants */ 
#define NI_MAXHOST	1025
#define NI_MAXSERV	32

#define NI_NOFQDN 	0x01
#define NI_NUMERICHOST	0x02
#define NI_NAMEREQD	0x04
#define NI_NUMERICSERV	0x08
#define NI_DGRAM	0x10

/* getaddrinfo constants */
#define AI_PASSIVE	1
#define AI_CANONNAME	2
#define AI_NUMERICHOST	4

/* getaddrinfo error codes */
#define EAI_AGAIN	WSATRY_AGAIN
#define EAI_BADFLAGS	WSAEINVAL
#define EAI_FAIL	WSANO_RECOVERY
#define EAI_FAMILY	WSAEAFNOSUPPORT
#define EAI_MEMORY	WSA_NOT_ENOUGH_MEMORY
#define EAI_NODATA	WSANO_DATA
#define EAI_NONAME	WSAHOST_NOT_FOUND
#define EAI_SERVICE	WSATYPE_NOT_FOUND
#define EAI_SOCKTYPE	WSAESOCKTNOSUPPORT

/*
 *   ip_mreq also in winsock.h for WinSock1.1,
 *   but online msdn docs say it is defined here for WinSock2.
 */ 

struct ip_mreq {
	struct in_addr	imr_multiaddr;
	struct in_addr	imr_interface;
};

struct ip_mreq_source {
	struct in_addr	imr_multiaddr;
	struct in_addr	imr_sourceaddr;
	struct in_addr	imr_interface;
};

struct ip_msfilter {
	struct in_addr	imsf_multiaddr;
	struct in_addr	imsf_interface;
	u_long		imsf_fmode;
	u_long		imsf_numsrc;
	struct in_addr	imsf_slist[1];
};

#define IP_MSFILTER_SIZE(numsrc) \
   (sizeof(struct ip_msfilter) - sizeof(struct in_addr) \
   + (numsrc) * sizeof(struct in_addr))

struct in_pktinfo {
	IN_ADDR ipi_addr;
	UINT    ipi_ifindex;
};
typedef struct in_pktinfo IN_PKTINFO;


/* ipv6 */ 
/* These require XP or .NET Server or use of add-on IPv6 stacks on NT 4
  or higher */

/* This is based on the example given in RFC 2553 with stdint types
   changed to BSD types.  For now, use these  field names until there
   is some consistency in MS docs. In this file, we only use the
   in6_addr structure start address, with casts to get the right offsets
   when testing addresses */
  
struct in6_addr {
    union {
        u_char	_S6_u8[16];
        u_short	_S6_u16[8];
        u_long	_S6_u32[4];
        } _S6_un;
};
/* s6_addr is the standard name */
#define s6_addr		_S6_un._S6_u8

/* These are GLIBC names */ 
#define s6_addr16	_S6_un._S6_u16
#define s6_addr32	_S6_un._S6_u16

/* These are used in some MS code */
#define in_addr6	in6_addr
#define _s6_bytes	_S6_un._S6_u8
#define _s6_words	_S6_un._S6_u16

typedef struct in6_addr IN6_ADDR,  *PIN6_ADDR, *LPIN6_ADDR;

struct sockaddr_in6 {
	short sin6_family;	/* AF_INET6 */
	u_short sin6_port; 	/* transport layer port # */
	u_long sin6_flowinfo;	/* IPv6 traffic class & flow info */
	struct in6_addr sin6_addr;  /* IPv6 address */
	u_long sin6_scope_id;	/* set of interfaces for a scope */
};
typedef struct sockaddr_in6 SOCKADDR_IN6, *PSOCKADDR_IN6, *LPSOCKADDR_IN6;

extern const struct in6_addr in6addr_any;
extern const struct in6_addr in6addr_loopback;
/* the above can get initialised using: */ 
#define IN6ADDR_ANY_INIT        { 0 }
#define IN6ADDR_LOOPBACK_INIT   { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 }

/* Described in RFC 2292, but not in 2553 */
/* int IN6_ARE_ADDR_EQUAL(const struct in6_addr * a, const struct in6_addr * b) */
#define IN6_ARE_ADDR_EQUAL(a, b)	\
    (memcmp ((void*)(a), (void*)(b), sizeof (struct in6_addr)) == 0)


/* Address Testing Macros 

 These macro functions all take const struct in6_addr* as arg.
 Static inlines would allow type checking, but RFC 2553 says they
 macros.	 
 NB: These are written specifically for little endian host */

#define IN6_IS_ADDR_UNSPECIFIED(_addr) \
	(   (((const u_long *)(_addr))[0] == 0)	\
	 && (((const u_long *)(_addr))[1] == 0)	\
	 && (((const u_long *)(_addr))[2] == 0)	\
	 && (((const u_long *)(_addr))[3] == 0))

#define IN6_IS_ADDR_LOOPBACK(_addr) \
	(   (((const u_long *)(_addr))[0] == 0)	\
	 && (((const u_long *)(_addr))[1] == 0)	\
	 && (((const u_long *)(_addr))[2] == 0)	\
	 && (((const u_long *)(_addr))[3] == 0x01000000)) /* Note byte order reversed */
/*	    (((const u_long *)(_addr))[3] == ntohl(1))  */

#define IN6_IS_ADDR_MULTICAST(_addr) (((const u_char *) (_addr))[0] == 0xff)

#define IN6_IS_ADDR_LINKLOCAL(_addr) \
	(   (((const u_char *)(_addr))[0] == 0xfe)	\
	 && ((((const u_char *)(_addr))[1] & 0xc0) == 0x80))

#define IN6_IS_ADDR_SITELOCAL(_addr) \
	(   (((const u_char *)(_addr))[0] == 0xfe)	\
	 && ((((const u_char *)(_addr))[1] & 0xc0) == 0xc0))

#define IN6_IS_ADDR_V4MAPPED(_addr) \
	(   (((const u_long *)(_addr))[0] == 0)		\
	 && (((const u_long *)(_addr))[1] == 0)		\
	 && (((const u_long *)(_addr))[2] == 0xffff0000)) /* Note byte order reversed */
/* 	    (((const u_long *)(_addr))[2] == ntohl(0x0000ffff))) */

#define IN6_IS_ADDR_V4COMPAT(_addr) \
	(   (((const u_long *)(_addr))[0] == 0)		\
	 && (((const u_long *)(_addr))[1] == 0)		\
	 && (((const u_long *)(_addr))[2] == 0)		\
	 && (((const u_long *)(_addr))[3] != 0)		\
	 && (((const u_long *)(_addr))[3] != 0x01000000)) /* Note byte order reversed */
/*           (ntohl (((const u_long *)(_addr))[3]) > 1 ) */


#define IN6_IS_ADDR_MC_NODELOCAL(_addr)	\
	(   IN6_IS_ADDR_MULTICAST(_addr)		\
	 && ((((const u_char *)(_addr))[1] & 0xf) == 0x1)) 

#define IN6_IS_ADDR_MC_LINKLOCAL(_addr)	\
	(   IN6_IS_ADDR_MULTICAST (_addr)		\
	 && ((((const u_char *)(_addr))[1] & 0xf) == 0x2))

#define IN6_IS_ADDR_MC_SITELOCAL(_addr)	\
	(   IN6_IS_ADDR_MULTICAST(_addr)		\
	 && ((((const u_char *)(_addr))[1] & 0xf) == 0x5))

#define IN6_IS_ADDR_MC_ORGLOCAL(_addr)	\
	(   IN6_IS_ADDR_MULTICAST(_addr)		\
	 && ((((const u_char *)(_addr))[1] & 0xf) == 0x8))

#define IN6_IS_ADDR_MC_GLOBAL(_addr)	\
	(   IN6_IS_ADDR_MULTICAST(_addr)	\
	 && ((((const u_char *)(_addr))[1] & 0xf) == 0xe))


typedef int socklen_t;

struct ipv6_mreq {
	struct in6_addr ipv6mr_multiaddr;
	unsigned int    ipv6mr_interface;
};
typedef struct ipv6_mreq IPV6_MREG;

struct in6_pktinfo {
	IN6_ADDR ipi6_addr;
	UINT     ipi6_ifindex;
};
typedef struct  in6_pktinfo IN6_PKTINFO;

struct addrinfo {
	int     ai_flags;
	int     ai_family;
	int     ai_socktype;
	int     ai_protocol;
	size_t  ai_addrlen;
	char   *ai_canonname;
	struct sockaddr  *ai_addr;
	struct addrinfo  *ai_next;
};

void WSAAPI freeaddrinfo (struct addrinfo*);
int WSAAPI getaddrinfo (const char*,const char*,const struct addrinfo*,
		        struct addrinfo**);

char* WSAAPI gai_strerrorA(int);
WCHAR* WSAAPI gai_strerrorW(int);
#ifdef UNICODE
#define gai_strerror   gai_strerrorW
#else
#define gai_strerror   gai_strerrorA
#endif  /* UNICODE */

int WSAAPI getnameinfo(const struct sockaddr*,socklen_t,char*,DWORD,
		       char*,DWORD,int);


/* Some older IPv4/IPv6 compatability stuff */

/* This struct lacks sin6_scope_id; retained for use in sockaddr_gen */
struct sockaddr_in6_old {
	short   sin6_family;
	u_short sin6_port;
	u_long  sin6_flowinfo;
	struct in6_addr sin6_addr;
};

typedef union sockaddr_gen{
	struct sockaddr		Address;
	struct sockaddr_in	AddressIn;
	struct sockaddr_in6_old	AddressIn6;
} sockaddr_gen;


typedef struct _INTERFACE_INFO {
	u_long		iiFlags;
	sockaddr_gen	iiAddress;
	sockaddr_gen	iiBroadcastAddress;
	sockaddr_gen	iiNetmask;
} INTERFACE_INFO, *LPINTERFACE_INFO;

/*
   The definition above can cause problems on NT4,prior to sp4.
   To workaround, include the following struct and typedef and
   #define INTERFACE_INFO OLD_INTERFACE_INFO
   See: FIX: WSAIoctl SIO_GET_INTERFACE_LIST Option Problem
   (Q181520) in MSDN KB.

   The old definition causes problems on newer NT and on XP.

typedef struct _OLD_INTERFACE_INFO {
	u_long		iiFlags;
	struct sockaddr	iiAddress;
 	struct sockaddr	iiBroadcastAddress;
 	struct sockaddr	iiNetmask;
} OLD_INTERFACE_INFO;
*/

#ifdef  __cplusplus
}
#endif

#endif	/* _WS2TCPIP_H */
