/*
 * Copyright (c) 1982, 1985, 1986, 1988, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
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
 *	@(#)socket.h	8.4 (Berkeley) 2/21/94
 */

#ifndef _OSK_SYS_SOCKET_H_
#define	_OSK_SYS_SOCKET_H_

/*
 * Definitions related to sockets: types, address families, options.
 */

/*
 * Types
 */
#define	SOCK_STREAM	1		/* stream socket */
#define	SOCK_DGRAM	2		/* datagram socket */
#define	SOCK_RAW	3		/* raw-protocol interface */
#define	SOCK_RDM	4		/* reliably-delivered message */
#define	SOCK_SEQPACKET	5		/* sequenced packet stream */

/*
 * Option flags per-socket.
 */
#define	SO_DEBUG	0x0001		/* turn on debugging info recording */
#define	SO_ACCEPTCONN	0x0002		/* socket has had listen() */
#define	SO_REUSEADDR	0x0004		/* allow local address reuse */
#define	SO_KEEPALIVE	0x0008		/* keep connections alive */
#define	SO_DONTROUTE	0x0010		/* just use interface addresses */
#define	SO_BROADCAST	0x0020		/* permit sending of broadcast msgs */
#define	SO_USELOOPBACK	0x0040		/* bypass hardware when possible */
#define	SO_LINGER	0x0080		/* linger on close if data present */
#define	SO_OOBINLINE	0x0100		/* leave received OOB data in line */
#define	SO_REUSEPORT	0x0200		/* allow local address & port reuse */

/*
 * Additional options, not kept in so_options.
 */
#define SO_SNDBUF	0x1001		/* send buffer size */
#define SO_RCVBUF	0x1002		/* receive buffer size */
#define SO_SNDLOWAT	0x1003		/* send low-water mark */
#define SO_RCVLOWAT	0x1004		/* receive low-water mark */
#define SO_SNDTIMEO	0x1005		/* send timeout */
#define SO_RCVTIMEO	0x1006		/* receive timeout */
#define	SO_ERROR	0x1007		/* get error status and clear */
#define	SO_TYPE		0x1008		/* get socket type */

/*
 * Structure used for manipulating linger option.
 */
#ifndef _WINSOCKAPI_
struct	linger {
	int	l_onoff;		/* option on/off */
	int	l_linger;		/* linger time */
};
#endif//_WINSOCKAPI_

/*
 * Level number for (get/set)sockopt() to apply to socket itself.
 */
#define	SOL_SOCKET	0xffff		/* options for socket level */

/*
 * Address families.
 */
#define	AF_UNSPEC	0		/* unspecified */
#define	AF_LOCAL	1		/* local to host (pipes, portals) */
#ifndef _WINSOCKAPI_
#define	AF_UNIX		AF_LOCAL	/* backward compatibility */
#endif//_WINSOCKAPI_
#define	AF_INET		2		/* internetwork: UDP, TCP, etc. */
#define	AF_IMPLINK	3		/* arpanet imp addresses */
#define	AF_PUP		4		/* pup protocols: e.g. BSP */
#define	AF_CHAOS	5		/* mit CHAOS protocols */
#define	AF_NS		6		/* XEROX NS protocols */
#define	AF_ISO		7		/* ISO protocols */
#define	AF_OSI		AF_ISO
#define	AF_ECMA		8		/* european computer manufacturers */
#define	AF_DATAKIT	9		/* datakit protocols */
#define	AF_CCITT	10		/* CCITT protocols, X.25 etc */
#define	AF_SNA		11		/* IBM SNA */
#define AF_DECnet	12		/* DECnet */
#define AF_DLI		13		/* DEC Direct data link interface */
#define AF_LAT		14		/* LAT */
#define	AF_HYLINK	15		/* NSC Hyperchannel */
#define	AF_APPLETALK	16		/* Apple Talk */
#define	AF_ROUTE	17		/* Internal Routing Protocol */
#define	AF_LINK		18		/* Link layer interface */
#define	pseudo_AF_XTP	19		/* eXpress Transfer Protocol (no AF) */
#define	AF_COIP		20		/* connection-oriented IP, aka ST II */
#define	AF_CNT		21		/* Computer Network Technology */
#define pseudo_AF_RTIP	22		/* Help Identify RTIP packets */
#ifndef _WINSOCKAPI_
#define	AF_IPX		23		/* Novell Internet Protocol */
#endif//_WINSOCKAPI_
#define	AF_SIP		24		/* Simple Internet Protocol */
#define	pseudo_AF_PIP	25		/* Help Identify PIP packets */
#define	AF_ISDN		26		/* Integrated Services Digital Network*/
#define	AF_E164		AF_ISDN		/* CCITT E.164 recommendation */
#define	pseudo_AF_KEY	27		/* Internal key-management function */
#ifndef _WINSOCKAPI_
#define	AF_INET6	28		/* IPv6 */

#define	AF_MAX		29
#endif//_WINSOCKAPI_


/*
 * Structure used by kernel to store most
 * addresses.
 */
#ifdef _WINSOCKAPI_
#define sockaddr bsd_sockaddr
#define sockproto bsd_sockproto
#endif//_WINSOCKAPI_

struct sockaddr {
	u_char	sa_len;			/* total length */
	u_char	sa_family;		/* address family */
	char	sa_data[14];		/* actually longer; address value */
};

/*
 * Structure used by kernel to pass protocol
 * information in raw sockets.
 */
struct sockproto {
	u_short	sp_family;		/* address family */
	u_short	sp_protocol;		/* protocol */
};

/*
 * Protocol families, same as address families for now.
 */
#define	PF_UNSPEC	AF_UNSPEC
#define	PF_LOCAL	AF_LOCAL
#ifndef _WINSOCKAPI_
#define	PF_UNIX		PF_LOCAL	/* backward compatibility */
#endif//_WINSOCKAPI_
#define	PF_INET		AF_INET
#define	PF_IMPLINK	AF_IMPLINK
#define	PF_PUP		AF_PUP
#define	PF_CHAOS	AF_CHAOS
#define	PF_NS		AF_NS
#define	PF_ISO		AF_ISO
#ifndef _WINSOCKAPI_
#define	PF_OSI		AF_ISO
#endif//_WINSOCKAPI_
#define	PF_ECMA		AF_ECMA
#define	PF_DATAKIT	AF_DATAKIT
#define	PF_CCITT	AF_CCITT
#define	PF_SNA		AF_SNA
#define PF_DECnet	AF_DECnet
#define PF_DLI		AF_DLI
#define PF_LAT		AF_LAT
#define	PF_HYLINK	AF_HYLINK
#define	PF_APPLETALK	AF_APPLETALK
#define	PF_ROUTE	AF_ROUTE
#define	PF_LINK		AF_LINK
#define	PF_XTP		pseudo_AF_XTP	/* really just proto family, no AF */
#define	PF_COIP		AF_COIP
#define	PF_CNT		AF_CNT
#define	PF_SIP		AF_SIP
#define	PF_IPX		AF_IPX		/* same format as AF_NS */
#define PF_RTIP		pseudo_AF_FTIP	/* same format as AF_INET */
#define PF_PIP		pseudo_AF_PIP
#define	PF_ISDN		AF_ISDN

#define	PF_MAX		AF_MAX

/*
 * Definitions for network related sysctl, CTL_NET.
 *
 * Second level is protocol family.
 * Third level is protocol number.
 *
 * Further levels are defined by the individual families below.
 */
#define NET_MAXID	AF_MAX

#define CTL_NET_NAMES { \
	{ 0, 0 }, \
	{ "unix", CTLTYPE_NODE }, \
	{ "inet", CTLTYPE_NODE }, \
	{ "implink", CTLTYPE_NODE }, \
	{ "pup", CTLTYPE_NODE }, \
	{ "chaos", CTLTYPE_NODE }, \
	{ "xerox_ns", CTLTYPE_NODE }, \
	{ "iso", CTLTYPE_NODE }, \
	{ "emca", CTLTYPE_NODE }, \
	{ "datakit", CTLTYPE_NODE }, \
	{ "ccitt", CTLTYPE_NODE }, \
	{ "ibm_sna", CTLTYPE_NODE }, \
	{ "decnet", CTLTYPE_NODE }, \
	{ "dec_dli", CTLTYPE_NODE }, \
	{ "lat", CTLTYPE_NODE }, \
	{ "hylink", CTLTYPE_NODE }, \
	{ "appletalk", CTLTYPE_NODE }, \
	{ "route", CTLTYPE_NODE }, \
	{ "link_layer", CTLTYPE_NODE }, \
	{ "xtp", CTLTYPE_NODE }, \
	{ "coip", CTLTYPE_NODE }, \
	{ "cnt", CTLTYPE_NODE }, \
	{ "rtip", CTLTYPE_NODE }, \
	{ "ipx", CTLTYPE_NODE }, \
	{ "sip", CTLTYPE_NODE }, \
	{ "pip", CTLTYPE_NODE }, \
}

/*
 * PF_ROUTE - Routing table
 *
 * Three additional levels are defined:
 *	Fourth: address family, 0 is wildcard
 *	Fifth: type of info, defined below
 *	Sixth: flag(s) to mask with for NET_RT_FLAGS
 */
#define NET_RT_DUMP	1		/* dump; may limit to a.f. */
#define NET_RT_FLAGS	2		/* by flags, e.g. RESOLVING */
#define NET_RT_IFLIST	3		/* survey interface list */
#define	NET_RT_MAXID	4

#define CTL_NET_RT_NAMES { \
	{ 0, 0 }, \
	{ "dump", CTLTYPE_STRUCT }, \
	{ "flags", CTLTYPE_STRUCT }, \
	{ "iflist", CTLTYPE_STRUCT }, \
}

/*
 * Maximum queue length specifiable by listen.
 */
#ifndef _WINSOCKAPI_
#define	SOMAXCONN	128
#endif//_WINSOCKAPI_

/*
 * Message header for recvmsg and sendmsg calls.
 * Used value-result for recvmsg, value only for sendmsg.
 */
struct msghdr {
	caddr_t	msg_name;		/* optional address */
	u_int	msg_namelen;		/* size of address */
	struct	iovec *msg_iov;		/* scatter/gather array */
	u_int	msg_iovlen;		/* # elements in msg_iov */
	caddr_t	msg_control;		/* ancillary data, see below */
	u_int	msg_controllen;		/* ancillary data buffer len */
	int	msg_flags;		/* flags on received message */
};

#endif

#ifndef MSG_OOB
#define	MSG_OOB		0x1		/* process out-of-band data */
#define	MSG_PEEK	0x2		/* peek at incoming message */
#define	MSG_DONTROUTE	0x4		/* send without using routing tables */
#endif
#ifndef MSG_EOR
#define	MSG_EOR		0x8		/* data completes record */
#define	MSG_TRUNC	0x10		/* data discarded before delivery */
#define	MSG_CTRUNC	0x20		/* control data lost before delivery */
#define	MSG_WAITALL	0x40		/* wait for full request or error */
#endif
#ifndef MSG_DONTWAIT
#define	MSG_DONTWAIT	0x80		/* this message should be nonblocking */
#define	MSG_EOF		0x100		/* data completes connection */
#define MSG_COMPAT      0x8000		/* used in sendit() */
#endif

#ifndef __REACTOS__
/*
 * Header for ancillary data objects in msg_control buffer.
 * Used for additional information with/about a datagram
 * not expressible by flags.  The format is a sequence
 * of message elements headed by cmsghdr structures.
 */
struct cmsghdr {
	u_int	cmsg_len;		/* data byte count, including hdr */
	int	cmsg_level;		/* originating protocol */
	int	cmsg_type;		/* protocol-specific type */
/* followed by	u_char  cmsg_data[]; */
};

/* given pointer to struct cmsghdr, return pointer to data */
#define	CMSG_DATA(cmsg)		((u_char *)((cmsg) + 1))

/* given pointer to struct cmsghdr, return pointer to next cmsghdr */
#define	CMSG_NXTHDR(mhdr, cmsg)	\
	(((caddr_t)(cmsg) + (cmsg)->cmsg_len + sizeof(struct cmsghdr) > \
	    (mhdr)->msg_control + (mhdr)->msg_controllen) ? \
	    (struct cmsghdr *)NULL : \
	    (struct cmsghdr *)((caddr_t)(cmsg) + ALIGN((cmsg)->cmsg_len)))

#define	CMSG_FIRSTHDR(mhdr)	((struct cmsghdr *)(mhdr)->msg_control)

/* "Socket"-level control message types: */
#define	SCM_RIGHTS	0x01		/* access rights (array of int) */

/*
 * 4.3 compat sockaddr, move to compat file later
 */
struct osockaddr {
	u_short	sa_family;		/* address family */
	char	sa_data[14];		/* up to 14 bytes of direct address */
};

/*
 * 4.3-compat message header (move to compat file later).
 */
struct omsghdr {
	caddr_t	msg_name;		/* optional address */
	int	msg_namelen;		/* size of address */
	struct	iovec *msg_iov;		/* scatter/gather array */
	int	msg_iovlen;		/* # elements in msg_iov */
	caddr_t	msg_accrights;		/* access rights sent/received */
	int	msg_accrightslen;
};

#ifndef	KERNEL

#include <sys/cdefs.h>

__BEGIN_DECLS
int	accept __P((int, struct sockaddr *, int *));
int	bind __P((int, const struct sockaddr *, int));
int	connect __P((int, const struct sockaddr *, int));
int	getpeername __P((int, struct sockaddr *, int *));
int	getsockname __P((int, struct sockaddr *, int *));
int	getsockopt __P((int, int, int, void *, int *));
int	listen __P((int, int));
ssize_t	recv __P((int, void *, size_t, int));
ssize_t	recvfrom __P((int, void *, size_t, int, struct sockaddr *, int *));
ssize_t	recvmsg __P((int, struct msghdr *, int));
ssize_t	send __P((int, const void *, size_t, int));
ssize_t	sendto __P((int, const void *,
	    size_t, int, const struct sockaddr *, int));
ssize_t	sendmsg __P((int, const struct msghdr *, int));
int	setsockopt __P((int, int, int, const void *, int));
int	shutdown __P((int, int));
int	socket __P((int, int, int));
int	socketpair __P((int, int, int, int *));
__END_DECLS

#else /* KERNEL */
void	pfctlinput __P((int, struct sockaddr *));
#endif /* !KERNEL */
#endif /* !_OSK_SYS_SOCKET_H_ */
