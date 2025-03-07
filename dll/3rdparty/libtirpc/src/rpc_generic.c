/*
 * Copyright (c) 2009, Sun Microsystems, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of Sun Microsystems, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 1986-1991 by Sun Microsystems Inc. 
 */

//#include <sys/cdefs.h>

/*
 * rpc_generic.c, Miscl routines for RPC.
 *
 */
#include <wintirpc.h>
//#include <pthread.h>
#include <reentrant.h>
#include <sys/types.h>
//#include <sys/param.h>
//#include <sys/socket.h>
//#include <sys/time.h>
//#include <sys/un.h>
//#include <sys/resource.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>
#include <rpc/rpc.h>
//#include <ctype.h>
//#include <stddef.h>
#include <stdio.h>
//#include <netdb.h>
#include <netconfig.h>
#include <stdlib.h>
#include <string.h>
//#include <syslog.h>
#include <rpc/nettype.h>
#include "rpc_com.h"

struct handle {
	NCONF_HANDLE *nhandle;
	int nflag;		/* Whether NETPATH or NETCONFIG */
	int nettype;
};

static const struct _rpcnettype {
	const char *name;
	const int type;
} _rpctypelist[] = {
	{ "netpath", _RPC_NETPATH },
	{ "visible", _RPC_VISIBLE },
	{ "circuit_v", _RPC_CIRCUIT_V },
	{ "datagram_v", _RPC_DATAGRAM_V },
	{ "circuit_n", _RPC_CIRCUIT_N },
	{ "datagram_n", _RPC_DATAGRAM_N },
	{ "tcp", _RPC_TCP },
	{ "udp", _RPC_UDP },
	{ 0, _RPC_NONE }
};

struct netid_af {
	const char	*netid;
	ADDRESS_FAMILY		af;
	int		protocol;
};

static const struct netid_af na_cvt[] = {
	{ "udp",  AF_INET,  IPPROTO_UDP },
	{ "tcp",  AF_INET,  IPPROTO_TCP },
#ifdef INET6
	{ "udp6", AF_INET6, IPPROTO_UDP },
	{ "tcp6", AF_INET6, IPPROTO_TCP },
#endif
#ifdef AF_LOCAL
	{ "local", AF_LOCAL, 0 }
#endif
};

#if 0
static char *strlocase(char *);
#endif
static int getnettype(const char *);

/*
 * Cache the result of getrlimit(), so we don't have to do an
 * expensive call every time.
 */
int
__rpc_dtbsize()
{
#ifdef _WIN32
	return (WINSOCK_HANDLE_HASH_SIZE);
#else

	static int tbsize;
	struct rlimit rl;

	if (tbsize) {
		return (tbsize);
	}
	if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
		return (tbsize = (int)rl.rlim_max);
	}
	/*
	 * Something wrong.  I'll try to save face by returning a
	 * pessimistic number.
	 */
	return (32);
#endif
}


/*
 * Find the appropriate buffer size
 */
u_int
/*ARGSUSED*/
__rpc_get_t_size(af, proto, size)
	int af, proto;
	int size;	/* Size requested */
{
	int maxsize, defsize;

	maxsize = 256 * 1024;	/* XXX */
	switch (proto) {
	case IPPROTO_TCP:
		defsize = 1024 * 1024;	/* XXX */
		break;
	case IPPROTO_UDP:
		defsize = UDPMSGSIZE;
		break;
	default:
		defsize = RPC_MAXDATASIZE;
		break;
	}
	if (size == 0)
		return defsize;
#if 1
    /* cbodley- give us the size we ask for, or we'll get fragmented! */
    return (u_int)size;
#else
	/* Check whether the value is within the upper max limit */
	return (size > maxsize ? (u_int)maxsize : (u_int)size);
#endif
}

/*
 * Find the appropriate address buffer size
 */
u_int
__rpc_get_a_size(af)
	int af;
{
	switch (af) {
	case AF_INET:
		return sizeof (struct sockaddr_in);
#ifdef INET6
	case AF_INET6:
		return sizeof (struct sockaddr_in6);
#endif
#ifdef AF_LOCAL
	case AF_LOCAL:
		return sizeof (struct sockaddr_un);
#endif
	default:
		break;
	}
	return ((u_int)RPC_MAXADDRSIZE);
}

#if 0
static char *
strlocase(p)
	char *p;
{
	char *t = p;

	for (; *p; p++)
		if (isupper(*p))
			*p = tolower(*p);
	return (t);
}
#endif

/*
 * Returns the type of the network as defined in <rpc/nettype.h>
 * If nettype is NULL, it defaults to NETPATH.
 */
static int
getnettype(nettype)
	const char *nettype;
{
	int i;

	if ((nettype == NULL) || (nettype[0] == 0)) {
		return (_RPC_NETPATH);	/* Default */
	}

#if 0
	nettype = strlocase(nettype);
#endif
	for (i = 0; _rpctypelist[i].name; i++)
		if (strcasecmp(nettype, _rpctypelist[i].name) == 0) {
			return (_rpctypelist[i].type);
		}
	return (_rpctypelist[i].type);
}

/*
 * For the given nettype (tcp or udp only), return the first structure found.
 * This should be freed by calling freenetconfigent()
 */
struct netconfig *
__rpc_getconfip(nettype)
	const char *nettype;
{
	char *netid;
	char *netid_tcp = (char *) NULL;
	char *netid_udp = (char *) NULL;
	struct netconfig *dummy;
	extern thread_key_t tcp_key, udp_key;
	extern mutex_t tsd_lock;

	if (tcp_key == -1) {
		mutex_lock(&tsd_lock);
		if (tcp_key == -1)
			tcp_key = TlsAlloc();	//thr_keycreate(&tcp_key, free);
		mutex_unlock(&tsd_lock);
	}
	netid_tcp = (char *)thr_getspecific(tcp_key);
	if (udp_key == -1) {
		mutex_lock(&tsd_lock);
		if (udp_key == -1)
			udp_key = TlsAlloc();	//thr_keycreate(&udp_key, free);
		mutex_unlock(&tsd_lock);
	}
	netid_udp = (char *)thr_getspecific(udp_key);
	if (!netid_udp && !netid_tcp) {
		struct netconfig *nconf;
		void *confighandle;

		if (!(confighandle = setnetconfig())) {
			//syslog (LOG_ERR, "rpc: failed to open " NETCONFIG);
			return (NULL);
		}
		while ((nconf = getnetconfig(confighandle)) != NULL) {
			if (strcmp(nconf->nc_protofmly, NC_INET) == 0 ||
			    strcmp(nconf->nc_protofmly, NC_INET6) == 0) {
				if (strcmp(nconf->nc_proto, NC_TCP) == 0 &&
						netid_tcp == NULL) {
					netid_tcp = strdup(nconf->nc_netid);
					thr_setspecific(tcp_key,
							(void *) netid_tcp);
				} else
				if (strcmp(nconf->nc_proto, NC_UDP) == 0 &&
						netid_udp == NULL) {
					netid_udp = strdup(nconf->nc_netid);
					thr_setspecific(udp_key,
						(void *) netid_udp);
				}
			}
		}
		endnetconfig(confighandle);
	}
	if (strcmp(nettype, "udp") == 0)
		netid = netid_udp;
	else if (strcmp(nettype, "tcp") == 0)
		netid = netid_tcp;
	else {
		return (NULL);
	}
	if ((netid == NULL) || (netid[0] == 0)) {
		return (NULL);
	}
	dummy = getnetconfigent(netid);
	return (dummy);
}

/*
 * Returns the type of the nettype, which should then be used with
 * __rpc_getconf().
 */
void *
__rpc_setconf(nettype)
	const char *nettype;
{
	struct handle *handle;

	handle = (struct handle *) malloc(sizeof (struct handle));
	if (handle == NULL) {
		return (NULL);
	}
	switch (handle->nettype = getnettype(nettype)) {
	case _RPC_NETPATH:
	case _RPC_CIRCUIT_N:
	case _RPC_DATAGRAM_N:
		if (!(handle->nhandle = setnetpath())) {
			free(handle);
			return (NULL);
		}
		handle->nflag = TRUE;
		break;
	case _RPC_VISIBLE:
	case _RPC_CIRCUIT_V:
	case _RPC_DATAGRAM_V:
	case _RPC_TCP:
	case _RPC_UDP:
		if (!(handle->nhandle = setnetconfig())) {
		        //syslog (LOG_ERR, "rpc: failed to open " NETCONFIG);
			free(handle);
			return (NULL);
		}
		handle->nflag = FALSE;
		break;
	default:
		return (NULL);
	}

	return (handle);
}

/*
 * Returns the next netconfig struct for the given "net" type.
 * __rpc_setconf() should have been called previously.
 */
struct netconfig *
__rpc_getconf(vhandle)
	void *vhandle;
{
	struct handle *handle;
	struct netconfig *nconf;

	handle = (struct handle *)vhandle;
	if (handle == NULL) {
		return (NULL);
	}
	for (;;) {
		if (handle->nflag)
			nconf = getnetpath(handle->nhandle);
		else
			nconf = getnetconfig(handle->nhandle);
		if (nconf == NULL)
			break;
		if ((nconf->nc_semantics != NC_TPI_CLTS) &&
			(nconf->nc_semantics != NC_TPI_COTS) &&
			(nconf->nc_semantics != NC_TPI_COTS_ORD))
			continue;
		switch (handle->nettype) {
		case _RPC_VISIBLE:
			if (!(nconf->nc_flag & NC_VISIBLE))
				continue;
			/* FALLTHROUGH */
		case _RPC_NETPATH:	/* Be happy */
			break;
		case _RPC_CIRCUIT_V:
			if (!(nconf->nc_flag & NC_VISIBLE))
				continue;
			/* FALLTHROUGH */
		case _RPC_CIRCUIT_N:
			if ((nconf->nc_semantics != NC_TPI_COTS) &&
				(nconf->nc_semantics != NC_TPI_COTS_ORD))
				continue;
			break;
		case _RPC_DATAGRAM_V:
			if (!(nconf->nc_flag & NC_VISIBLE))
				continue;
			/* FALLTHROUGH */
		case _RPC_DATAGRAM_N:
			if (nconf->nc_semantics != NC_TPI_CLTS)
				continue;
			break;
		case _RPC_TCP:
			if (((nconf->nc_semantics != NC_TPI_COTS) &&
				(nconf->nc_semantics != NC_TPI_COTS_ORD)) ||
				(strcmp(nconf->nc_protofmly, NC_INET)
#ifdef INET6
				 && strcmp(nconf->nc_protofmly, NC_INET6))
#else
				)
#endif
				||
				strcmp(nconf->nc_proto, NC_TCP))
				continue;
			break;
		case _RPC_UDP:
			if ((nconf->nc_semantics != NC_TPI_CLTS) ||
				(strcmp(nconf->nc_protofmly, NC_INET)
#ifdef INET6
				&& strcmp(nconf->nc_protofmly, NC_INET6))
#else
				)
#endif
				||
				strcmp(nconf->nc_proto, NC_UDP))
				continue;
			break;
		}
		break;
	}
	return (nconf);
}

void
__rpc_endconf(vhandle)
	void * vhandle;
{
	struct handle *handle;

	handle = (struct handle *) vhandle;
	if (handle == NULL) {
		return;
	}
	if (handle->nflag) {
		endnetpath(handle->nhandle);
	} else {
		endnetconfig(handle->nhandle);
	}
	free(handle);
}

/*
 * Used to ping the NULL procedure for clnt handle.
 * Returns NULL if fails, else a non-NULL pointer.
 */
void *
rpc_nullproc(clnt)
	CLIENT *clnt;
{
	struct timeval TIMEOUT = {25, 0};

	if (clnt_call(clnt, NULLPROC, (xdrproc_t) xdr_void, NULL,
		(xdrproc_t) xdr_void, NULL, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return ((void *) clnt);
}

/*
 * Try all possible transports until
 * one succeeds in finding the netconf for the given fd.
 */
struct netconfig *
__rpcgettp(fd)
	SOCKET fd;
{
	const char *netid;
	struct __rpc_sockinfo si;

	if (!__rpc_fd2sockinfo(fd, &si))
		return NULL;

	if (!__rpc_sockinfo2netid(&si, &netid))
		return NULL;

	/*LINTED const castaway*/
	return getnetconfigent((char *)netid);
}

int
__rpc_fd2sockinfo(SOCKET fd, struct __rpc_sockinfo *sip)
{
	socklen_t len;
	int type, proto;
	struct sockaddr_storage ss;

#ifdef _WIN32
	WSAPROTOCOL_INFO proto_info;
	int proto_info_size = sizeof(proto_info);
	if (getsockopt(fd, SOL_SOCKET, SO_PROTOCOL_INFO, (char *)&proto_info, &proto_info_size) == SOCKET_ERROR) {
#ifndef __REACTOS__
		int err = WSAGetLastError();
#endif
		return 0;
	}
	len = proto_info.iMaxSockAddr;
	ss.ss_family = (ADDRESS_FAMILY)proto_info.iAddressFamily;
#else
	len = sizeof ss;
	if (getsockname(fd, (struct sockaddr *)&ss, &len) == SOCKET_ERROR) {
		return 0;
	}
#endif
	sip->si_alen = len;

	len = sizeof type;
	if (getsockopt(fd, SOL_SOCKET, SO_TYPE, (char *)&type, &len) == SOCKET_ERROR) {
#ifndef __REACTOS__
		int err = WSAGetLastError();
#endif
		return 0;
	}

	/* XXX */
#ifdef AF_LOCAL
	if (ss.ss_family != AF_LOCAL) {
#endif
		if (type == SOCK_STREAM)
			proto = IPPROTO_TCP;
		else if (type == SOCK_DGRAM)
			proto = IPPROTO_UDP;
		else
			return 0;
#ifdef AF_LOCAL
	} else
		proto = 0;
#endif

	sip->si_af = ss.ss_family;
	sip->si_proto = proto;
	sip->si_socktype = type;

	return 1;
}

/*
 * Linear search, but the number of entries is small.
 */
int
__rpc_nconf2sockinfo(const struct netconfig *nconf, struct __rpc_sockinfo *sip)
{
	int i;

	for (i = 0; i < (sizeof na_cvt) / (sizeof (struct netid_af)); i++)
		if (strcmp(na_cvt[i].netid, nconf->nc_netid) == 0 || (
		    strcmp(nconf->nc_netid, "unix") == 0 &&
		    strcmp(na_cvt[i].netid, "local") == 0)) {
			sip->si_af = na_cvt[i].af;
			sip->si_proto = na_cvt[i].protocol;
			sip->si_socktype =
			    __rpc_seman2socktype((int)nconf->nc_semantics);
			if (sip->si_socktype == -1)
				return 0;
			sip->si_alen = __rpc_get_a_size(sip->si_af);
			return 1;
		}

	return 0;
}

SOCKET
__rpc_nconf2fd(const struct netconfig *nconf)
{
	struct __rpc_sockinfo si;
	SOCKET fd;

	if (!__rpc_nconf2sockinfo(nconf, &si))
		return 0;

	if ((fd = socket(si.si_af, si.si_socktype, si.si_proto)) != INVALID_SOCKET &&
	    si.si_af == AF_INET6) {
		int val = 1;

		setsockopt(fd, SOL_IPV6, IPV6_V6ONLY, (const char *)&val, sizeof(val));
	}
	return fd;
}

int
__rpc_sockinfo2netid(struct __rpc_sockinfo *sip, const char **netid)
{
	int i;
	struct netconfig *nconf;

	nconf = getnetconfigent("local");

	for (i = 0; i < (sizeof na_cvt) / (sizeof (struct netid_af)); i++) {
		if (na_cvt[i].af == sip->si_af &&
		    na_cvt[i].protocol == sip->si_proto) {
			if (strcmp(na_cvt[i].netid, "local") == 0 && nconf == NULL) {
				if (netid)
					*netid = "unix";
			} else {
				if (netid)
					*netid = na_cvt[i].netid;
			}
			if (nconf != NULL)
				freenetconfigent(nconf);
			return 1;
		}
	}
	if (nconf != NULL)
		freenetconfigent(nconf);

	return 0;
}

char *
taddr2uaddr(const struct netconfig *nconf, const struct netbuf *nbuf)
{
	struct __rpc_sockinfo si;

	if (!__rpc_nconf2sockinfo(nconf, &si))
		return NULL;
	return __rpc_taddr2uaddr_af(si.si_af, nbuf);
}

struct netbuf *
uaddr2taddr(const struct netconfig *nconf, const char *uaddr)
{
	struct __rpc_sockinfo si;
	
	if (!__rpc_nconf2sockinfo(nconf, &si))
		return NULL;
	return __rpc_uaddr2taddr_af(si.si_af, uaddr);
}

void freeuaddr(char *uaddr)
{
	free(uaddr);
}

void freenetbuf(struct netbuf *nbuf)
{
	if (nbuf) {
		free(nbuf->buf);
		free(nbuf);
	}
}

#ifdef __REACTOS__
PCSTR
WSAAPI
inet_ntop(INT af, const VOID *src, PSTR dst, size_t cnt)
{
	struct in_addr in;
	char *text_addr;

	if (af == AF_INET) {
		memcpy(&in.s_addr, src, sizeof(in.s_addr));
		text_addr = inet_ntoa(in);
		if (text_addr && dst) {
			strncpy(dst, text_addr, cnt);
			return dst;
		}
	}

	return 0;
}
#endif

char *
__rpc_taddr2uaddr_af(int af, const struct netbuf *nbuf)
{
	char *ret;
	struct sockaddr_in *sin;
#ifdef AF_LOCAL
	struct sockaddr_un *sun;
#endif
	char namebuf[INET_ADDRSTRLEN];
#ifdef INET6
	struct sockaddr_in6 *sin6;
	char namebuf6[INET6_ADDRSTRLEN];
#endif
	u_int16_t port;

	if (nbuf->len <= 0)
		return NULL;

	switch (af) {
	case AF_INET:
#ifdef __REACTOS__ // CVE-2017-8779
		if (nbuf->len < sizeof(*sin)) {
			return NULL;
		}
#endif
		sin = nbuf->buf;
		if (inet_ntop(af, &sin->sin_addr, namebuf, sizeof namebuf)
		    == NULL)
			return NULL;
		port = ntohs(sin->sin_port);
		if (asprintf(&ret, "%s.%u.%u", namebuf, ((u_int32_t)port) >> 8,
		    port & 0xff) < 0)
			return NULL;
		break;
#ifdef INET6
	case AF_INET6:
#ifdef __REACTOS__ // CVE-2017-8779
		if (nbuf->len < sizeof(*sin6)) {
			return NULL;
		}
#endif
		sin6 = nbuf->buf;
		if (inet_ntop(af, &sin6->sin6_addr, namebuf6, sizeof namebuf6)
		    == NULL)
			return NULL;
		port = ntohs(sin6->sin6_port);
		if (asprintf(&ret, "%s.%u.%u", namebuf6, ((u_int32_t)port) >> 8,
		    port & 0xff) < 0)
			return NULL;
		break;
#endif
#ifdef AF_LOCAL
	case AF_LOCAL:
		sun = nbuf->buf;
		/*	if (asprintf(&ret, "%.*s", (int)(sun->sun_len -
		    offsetof(struct sockaddr_un, sun_path)),
		    sun->sun_path) < 0)*/
		if (asprintf(&ret, "%.*s", (int)(sizeof(*sun) -
						 offsetof(struct sockaddr_un, sun_path)),
			     sun->sun_path) < 0)

			return (NULL);
		break;
#endif
	default:
		return NULL;
	}

	return ret;
}

struct netbuf *
__rpc_uaddr2taddr_af(int af, const char *uaddr)
{
	struct netbuf *ret = NULL;
	char *addrstr, *p;
	unsigned short port, portlo, porthi;
	struct sockaddr_in *sin;
#ifdef INET6
	struct sockaddr_in6 *sin6;
#endif
#ifdef AF_LOCAL
	struct sockaddr_un *sun;
#endif

	port = 0;
	sin = NULL;
#ifdef __REACTOS__ // CVE-2017-8779
	if (uaddr == NULL)
		return NULL;
#endif
	addrstr = strdup(uaddr);
	if (addrstr == NULL)
		return NULL;

	/*
	 * AF_LOCAL addresses are expected to be absolute
	 * pathnames, anything else will be AF_INET or AF_INET6.
	 */
	if (*addrstr != '/') {
		p = strrchr(addrstr, '.');
		if (p == NULL)
			goto out;
		portlo = (unsigned)atoi(p + 1);
		*p = '\0';

		p = strrchr(addrstr, '.');
		if (p == NULL)
			goto out;
		porthi = (unsigned)atoi(p + 1);
		*p = '\0';
		port = (porthi << 8) | portlo;
	}

	ret = (struct netbuf *)malloc(sizeof *ret);
	if (ret == NULL)
		goto out;
	
	switch (af) {
	case AF_INET:
		sin = (struct sockaddr_in *)malloc(sizeof *sin);
		if (sin == NULL)
			goto out;
		memset(sin, 0, sizeof *sin);
		sin->sin_family = AF_INET;
		sin->sin_port = htons(port);
#ifndef __REACTOS__
		if (inet_pton(AF_INET, addrstr, &sin->sin_addr) <= 0) {
#else
		sin->sin_addr.S_un.S_addr = inet_addr(addrstr);
		if (sin->sin_addr.S_un.S_addr == INADDR_NONE) {
#endif
			free(sin);
			free(ret);
			ret = NULL;
			goto out;
		}
		ret->maxlen = ret->len = sizeof *sin;
		ret->buf = sin;
		break;
#ifdef INET6
	case AF_INET6:
		sin6 = (struct sockaddr_in6 *)malloc(sizeof *sin6);
		if (sin6 == NULL)
			goto out;
		memset(sin6, 0, sizeof *sin6);
		sin6->sin6_family = AF_INET6;
		sin6->sin6_port = htons(port);
		if (inet_pton(AF_INET6, addrstr, &sin6->sin6_addr) <= 0) {
			free(sin6);
			free(ret);
			ret = NULL;
			goto out;
		}
		ret->maxlen = ret->len = sizeof *sin6;
		ret->buf = sin6;
		break;
#endif
#ifdef AF_LOCAL
	case AF_LOCAL:
		sun = (struct sockaddr_un *)malloc(sizeof *sun);
		if (sun == NULL)
			goto out;
		memset(sun, 0, sizeof *sun);
		sun->sun_family = AF_LOCAL;
		strncpy(sun->sun_path, addrstr, sizeof(sun->sun_path) - 1);
		ret->len = SUN_LEN(sun);
		ret->maxlen = sizeof(struct sockaddr_un);
		ret->buf = sun;
		break;
#endif
	default:
		break;
	}
out:
	free(addrstr);
	return ret;
}

int
__rpc_seman2socktype(int semantics)
{
	switch (semantics) {
	case NC_TPI_CLTS:
		return SOCK_DGRAM;
	case NC_TPI_COTS_ORD:
		return SOCK_STREAM;
	case NC_TPI_RAW:
		return SOCK_RAW;
	default:
		break;
	}

	return -1;
}

int
__rpc_socktype2seman(int socktype)
{
	switch (socktype) {
	case SOCK_DGRAM:
		return NC_TPI_CLTS;
	case SOCK_STREAM:
		return NC_TPI_COTS_ORD;
	case SOCK_RAW:
		return NC_TPI_RAW;
	default:
		break;
	}

	return -1;
}

/*
 * XXXX - IPv6 scope IDs can't be handled in universal addresses.
 * Here, we compare the original server address to that of the RPC
 * service we just received back from a call to rpcbind on the remote
 * machine. If they are both "link local" or "site local", copy
 * the scope id of the server address over to the service address.
 */
int
__rpc_fixup_addr(struct netbuf *new, const struct netbuf *svc)
{
#ifdef INET6
	struct sockaddr *sa_new, *sa_svc;
	struct sockaddr_in6 *sin6_new, *sin6_svc;

	sa_svc = (struct sockaddr *)svc->buf;
	sa_new = (struct sockaddr *)new->buf;

	if (sa_new->sa_family == sa_svc->sa_family &&
	    sa_new->sa_family == AF_INET6) {
		sin6_new = (struct sockaddr_in6 *)new->buf;
		sin6_svc = (struct sockaddr_in6 *)svc->buf;

		if ((IN6_IS_ADDR_LINKLOCAL(&sin6_new->sin6_addr) &&
		     IN6_IS_ADDR_LINKLOCAL(&sin6_svc->sin6_addr)) ||
		    (IN6_IS_ADDR_SITELOCAL(&sin6_new->sin6_addr) &&
		     IN6_IS_ADDR_SITELOCAL(&sin6_svc->sin6_addr))) {
			sin6_new->sin6_scope_id = sin6_svc->sin6_scope_id;
		}
	}
#endif
	return 1;
}

int
__rpc_sockisbound(SOCKET fd)
{
	struct sockaddr_storage ss;
	union {
		struct sockaddr_in  sin;
		struct sockaddr_in6 sin6;
#ifdef AF_LOCAL
		struct sockaddr_un  usin;
#endif
	} u_addr;
	socklen_t slen;

	slen = sizeof (struct sockaddr_storage);
	if (getsockname(fd, (struct sockaddr *)(void *)&ss, &slen) == SOCKET_ERROR)
		return 0;

	switch (ss.ss_family) {
		case AF_INET:
			memcpy(&u_addr.sin, &ss, sizeof(u_addr.sin)); 
			return (u_addr.sin.sin_port != 0);
#ifdef INET6
		case AF_INET6:
			memcpy(&u_addr.sin6, &ss, sizeof(u_addr.sin6)); 
			return (u_addr.sin6.sin6_port != 0);
#endif
#ifdef AF_LOCAL
		case AF_LOCAL:
			/* XXX check this */
			memcpy(&u_addr.usin, &ss, sizeof(u_addr.usin)); 
			return (u_addr.usin.sun_path[0] != 0);
#endif
		default:
			break;
	}

	return 0;
}

/*
 * Helper function to set up a netbuf
 */
struct netbuf *
__rpc_set_netbuf(struct netbuf *nb, const void *ptr, size_t len)
{
	if (nb->len != len) {
		if (nb->len)
			mem_free(nb->buf, nb->len);
		nb->buf = mem_alloc(len);
		if (nb->buf == NULL)
			return NULL;

		nb->maxlen = nb->len = len;
	}
	memcpy(nb->buf, ptr, len);
	return nb;
}
