/*
 * Copyright (C) 2004-2007, 2009  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2001  Internet Software Consortium.
 *
 * This code is derived from software contributed to ISC by
 * Berkeley Software Design, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC AND BERKELEY SOFTWARE DESIGN, INC.
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE
 * FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id: getaddrinfo.c,v 1.52.254.2 2009/03/31 23:47:16 tbox Exp $ */

/*! \file */

/**
 *    lwres_getaddrinfo() is used to get a list of IP addresses and port
 *    numbers for host hostname and service servname. The function is the
 *    lightweight resolver's implementation of getaddrinfo() as defined in
 *    RFC2133. hostname and servname are pointers to null-terminated strings
 *    or NULL. hostname is either a host name or a numeric host address
 *    string: a dotted decimal IPv4 address or an IPv6 address. servname is
 *    either a decimal port number or a service name as listed in
 *    /etc/services.
 *
 *    If the operating system does not provide a struct addrinfo, the
 *    following structure is used:
 *
 * \code
 * struct  addrinfo {
 *         int             ai_flags;       // AI_PASSIVE, AI_CANONNAME
 *         int             ai_family;      // PF_xxx
 *         int             ai_socktype;    // SOCK_xxx
 *         int             ai_protocol;    // 0 or IPPROTO_xxx for IPv4 and IPv6
 *         size_t          ai_addrlen;     // length of ai_addr
 *         char            *ai_canonname;  // canonical name for hostname
 *         struct sockaddr *ai_addr;       // binary address
 *         struct addrinfo *ai_next;       // next structure in linked list
 * };
 * \endcode
 *
 *
 *    hints is an optional pointer to a struct addrinfo. This structure can
 *    be used to provide hints concerning the type of socket that the caller
 *    supports or wishes to use. The caller can supply the following
 *    structure elements in *hints:
 *
 * <ul>
 *    <li>ai_family:
 *           The protocol family that should be used. When ai_family is set
 *           to PF_UNSPEC, it means the caller will accept any protocol
 *           family supported by the operating system.</li>
 *
 *    <li>ai_socktype:
 *           denotes the type of socket -- SOCK_STREAM, SOCK_DGRAM or
 *           SOCK_RAW -- that is wanted. When ai_socktype is zero the caller
 *           will accept any socket type.</li>
 *
 *    <li>ai_protocol:
 *           indicates which transport protocol is wanted: IPPROTO_UDP or
 *           IPPROTO_TCP. If ai_protocol is zero the caller will accept any
 *           protocol.</li>
 *
 *    <li>ai_flags:
 *           Flag bits. If the AI_CANONNAME bit is set, a successful call to
 *           lwres_getaddrinfo() will return a null-terminated string
 *           containing the canonical name of the specified hostname in
 *           ai_canonname of the first addrinfo structure returned. Setting
 *           the AI_PASSIVE bit indicates that the returned socket address
 *           structure is intended for used in a call to bind(2). In this
 *           case, if the hostname argument is a NULL pointer, then the IP
 *           address portion of the socket address structure will be set to
 *           INADDR_ANY for an IPv4 address or IN6ADDR_ANY_INIT for an IPv6
 *           address.<br /><br />
 *
 *           When ai_flags does not set the AI_PASSIVE bit, the returned
 *           socket address structure will be ready for use in a call to
 *           connect(2) for a connection-oriented protocol or connect(2),
 *           sendto(2), or sendmsg(2) if a connectionless protocol was
 *           chosen. The IP address portion of the socket address structure
 *           will be set to the loopback address if hostname is a NULL
 *           pointer and AI_PASSIVE is not set in ai_flags.<br /><br />
 *
 *           If ai_flags is set to AI_NUMERICHOST it indicates that hostname
 *           should be treated as a numeric string defining an IPv4 or IPv6
 *           address and no name resolution should be attempted.
 * </li></ul>
 *
 *    All other elements of the struct addrinfo passed via hints must be
 *    zero.
 *
 *    A hints of NULL is treated as if the caller provided a struct addrinfo
 *    initialized to zero with ai_familyset to PF_UNSPEC.
 *
 *    After a successful call to lwres_getaddrinfo(), *res is a pointer to a
 *    linked list of one or more addrinfo structures. Each struct addrinfo
 *    in this list cn be processed by following the ai_next pointer, until a
 *    NULL pointer is encountered. The three members ai_family, ai_socktype,
 *    and ai_protocol in each returned addrinfo structure contain the
 *    corresponding arguments for a call to socket(2). For each addrinfo
 *    structure in the list, the ai_addr member points to a filled-in socket
 *    address structure of length ai_addrlen.
 *
 *    All of the information returned by lwres_getaddrinfo() is dynamically
 *    allocated: the addrinfo structures, and the socket address structures
 *    and canonical host name strings pointed to by the addrinfostructures.
 *    Memory allocated for the dynamically allocated structures created by a
 *    successful call to lwres_getaddrinfo() is released by
 *    lwres_freeaddrinfo(). ai is a pointer to a struct addrinfo created by
 *    a call to lwres_getaddrinfo().
 *
 * \section lwresreturn RETURN VALUES
 *
 *    lwres_getaddrinfo() returns zero on success or one of the error codes
 *    listed in gai_strerror() if an error occurs. If both hostname and
 *    servname are NULL lwres_getaddrinfo() returns #EAI_NONAME.
 *
 * \section lwressee SEE ALSO
 *
 *    lwres(3), lwres_getaddrinfo(), lwres_freeaddrinfo(),
 *    lwres_gai_strerror(), RFC2133, getservbyname(3), connect(2),
 *    sendto(2), sendmsg(2), socket(2).
 */

#include <config.h>

#include <errno.h>

#include <isc/string.h>

#include <lwres/lwres.h>
#include <lwres/net.h>
#include <lwres/netdb.h>
#include <lwres/stdlib.h>

#define SA(addr)	((struct sockaddr *)(addr))
#define SIN(addr)	((struct sockaddr_in *)(addr))
#define SIN6(addr)	((struct sockaddr_in6 *)(addr))
#define SLOCAL(addr)	((struct sockaddr_un *)(addr))

/*! \struct addrinfo
 */
static struct addrinfo
	*ai_reverse(struct addrinfo *oai),
	*ai_clone(struct addrinfo *oai, int family),
	*ai_alloc(int family, int addrlen);
#ifdef AF_LOCAL
static int get_local(const char *name, int socktype, struct addrinfo **res);
#endif

static int add_ipv4(const char *hostname, int flags, struct addrinfo **aip,
    int socktype, int port);
static int add_ipv6(const char *hostname, int flags, struct addrinfo **aip,
    int socktype, int port);
static void set_order(int, int (**)(const char *, int, struct addrinfo **,
	 int, int));

#define FOUND_IPV4	0x1
#define FOUND_IPV6	0x2
#define FOUND_MAX	2

#define ISC_AI_MASK (AI_PASSIVE|AI_CANONNAME|AI_NUMERICHOST)
/*% Get a list of IP addresses and port numbers for host hostname and service servname. */
int
lwres_getaddrinfo(const char *hostname, const char *servname,
	const struct addrinfo *hints, struct addrinfo **res)
{
	struct servent *sp;
	const char *proto;
	int family, socktype, flags, protocol;
	struct addrinfo *ai, *ai_list;
	int port, err, i;
	int (*net_order[FOUND_MAX+1])(const char *, int, struct addrinfo **,
		 int, int);

	if (hostname == NULL && servname == NULL)
		return (EAI_NONAME);

	proto = NULL;
	if (hints != NULL) {
		if ((hints->ai_flags & ~(ISC_AI_MASK)) != 0)
			return (EAI_BADFLAGS);
		if (hints->ai_addrlen || hints->ai_canonname ||
		    hints->ai_addr || hints->ai_next) {
			errno = EINVAL;
			return (EAI_SYSTEM);
		}
		family = hints->ai_family;
		socktype = hints->ai_socktype;
		protocol = hints->ai_protocol;
		flags = hints->ai_flags;
		switch (family) {
		case AF_UNSPEC:
			switch (hints->ai_socktype) {
			case SOCK_STREAM:
				proto = "tcp";
				break;
			case SOCK_DGRAM:
				proto = "udp";
				break;
			}
			break;
		case AF_INET:
		case AF_INET6:
			switch (hints->ai_socktype) {
			case 0:
				break;
			case SOCK_STREAM:
				proto = "tcp";
				break;
			case SOCK_DGRAM:
				proto = "udp";
				break;
			case SOCK_RAW:
				break;
			default:
				return (EAI_SOCKTYPE);
			}
			break;
#ifdef	AF_LOCAL
		case AF_LOCAL:
			switch (hints->ai_socktype) {
			case 0:
				break;
			case SOCK_STREAM:
				break;
			case SOCK_DGRAM:
				break;
			default:
				return (EAI_SOCKTYPE);
			}
			break;
#endif
		default:
			return (EAI_FAMILY);
		}
	} else {
		protocol = 0;
		family = 0;
		socktype = 0;
		flags = 0;
	}

#ifdef	AF_LOCAL
	/*!
	 * First, deal with AF_LOCAL.  If the family was not set,
	 * then assume AF_LOCAL if the first character of the
	 * hostname/servname is '/'.
	 */

	if (hostname != NULL &&
	    (family == AF_LOCAL || (family == 0 && *hostname == '/')))
		return (get_local(hostname, socktype, res));

	if (servname != NULL &&
	    (family == AF_LOCAL || (family == 0 && *servname == '/')))
		return (get_local(servname, socktype, res));
#endif

	/*
	 * Ok, only AF_INET and AF_INET6 left.
	 */
	ai_list = NULL;

	/*
	 * First, look up the service name (port) if it was
	 * requested.  If the socket type wasn't specified, then
	 * try and figure it out.
	 */
	if (servname != NULL) {
		char *e;

		port = strtol(servname, &e, 10);
		if (*e == '\0') {
			if (socktype == 0)
				return (EAI_SOCKTYPE);
			if (port < 0 || port > 65535)
				return (EAI_SERVICE);
			port = htons((unsigned short) port);
		} else {
			sp = getservbyname(servname, proto);
			if (sp == NULL)
				return (EAI_SERVICE);
			port = sp->s_port;
			if (socktype == 0) {
				if (strcmp(sp->s_proto, "tcp") == 0)
					socktype = SOCK_STREAM;
				else if (strcmp(sp->s_proto, "udp") == 0)
					socktype = SOCK_DGRAM;
			}
		}
	} else
		port = 0;

	/*
	 * Next, deal with just a service name, and no hostname.
	 * (we verified that one of them was non-null up above).
	 */
	if (hostname == NULL && (flags & AI_PASSIVE) != 0) {
		if (family == AF_INET || family == 0) {
			ai = ai_alloc(AF_INET, sizeof(struct sockaddr_in));
			if (ai == NULL)
				return (EAI_MEMORY);
			ai->ai_socktype = socktype;
			ai->ai_protocol = protocol;
			SIN(ai->ai_addr)->sin_port = port;
			ai->ai_next = ai_list;
			ai_list = ai;
		}

		if (family == AF_INET6 || family == 0) {
			ai = ai_alloc(AF_INET6, sizeof(struct sockaddr_in6));
			if (ai == NULL) {
				lwres_freeaddrinfo(ai_list);
				return (EAI_MEMORY);
			}
			ai->ai_socktype = socktype;
			ai->ai_protocol = protocol;
			SIN6(ai->ai_addr)->sin6_port = port;
			ai->ai_next = ai_list;
			ai_list = ai;
		}

		*res = ai_list;
		return (0);
	}

	/*
	 * If the family isn't specified or AI_NUMERICHOST specified,
	 * check first to see if it is a numeric address.
	 * Though the gethostbyname2() routine
	 * will recognize numeric addresses, it will only recognize
	 * the format that it is being called for.  Thus, a numeric
	 * AF_INET address will be treated by the AF_INET6 call as
	 * a domain name, and vice versa.  Checking for both numerics
	 * here avoids that.
	 */
	if (hostname != NULL &&
	    (family == 0 || (flags & AI_NUMERICHOST) != 0)) {
		char abuf[sizeof(struct in6_addr)];
		char nbuf[NI_MAXHOST];
		int addrsize, addroff;
#ifdef LWRES_HAVE_SIN6_SCOPE_ID
		char *p, *ep;
		char ntmp[NI_MAXHOST];
		lwres_uint32_t scopeid;
#endif

#ifdef LWRES_HAVE_SIN6_SCOPE_ID
		/*
		 * Scope identifier portion.
		 */
		ntmp[0] = '\0';
		if (strchr(hostname, '%') != NULL) {
			strncpy(ntmp, hostname, sizeof(ntmp) - 1);
			ntmp[sizeof(ntmp) - 1] = '\0';
			p = strchr(ntmp, '%');
			ep = NULL;

			/*
			 * Vendors may want to support non-numeric
			 * scopeid around here.
			 */

			if (p != NULL)
				scopeid = (lwres_uint32_t)strtoul(p + 1,
								  &ep, 10);
			if (p != NULL && ep != NULL && ep[0] == '\0')
				*p = '\0';
			else {
				ntmp[0] = '\0';
				scopeid = 0;
			}
		} else
			scopeid = 0;
#endif

	       if (lwres_net_pton(AF_INET, hostname, (struct in_addr *)abuf)
		   == 1)
	       {
			if (family == AF_INET6) {
				/*
				 * Convert to a V4 mapped address.
				 */
				struct in6_addr *a6 = (struct in6_addr *)abuf;
				memcpy(&a6->s6_addr[12], &a6->s6_addr[0], 4);
				memset(&a6->s6_addr[10], 0xff, 2);
				memset(&a6->s6_addr[0], 0, 10);
				goto inet6_addr;
			}
			addrsize = sizeof(struct in_addr);
			addroff = (char *)(&SIN(0)->sin_addr) - (char *)0;
			family = AF_INET;
			goto common;
#ifdef LWRES_HAVE_SIN6_SCOPE_ID
		} else if (ntmp[0] != '\0' &&
			   lwres_net_pton(AF_INET6, ntmp, abuf) == 1)
		{
			if (family && family != AF_INET6)
				return (EAI_NONAME);
			addrsize = sizeof(struct in6_addr);
			addroff = (char *)(&SIN6(0)->sin6_addr) - (char *)0;
			family = AF_INET6;
			goto common;
#endif
		} else if (lwres_net_pton(AF_INET6, hostname, abuf) == 1) {
			if (family != 0 && family != AF_INET6)
				return (EAI_NONAME);
		inet6_addr:
			addrsize = sizeof(struct in6_addr);
			addroff = (char *)(&SIN6(0)->sin6_addr) - (char *)0;
			family = AF_INET6;

		common:
			ai = ai_clone(ai_list, family);
			if (ai == NULL)
				return (EAI_MEMORY);
			ai_list = ai;
			ai->ai_socktype = socktype;
			SIN(ai->ai_addr)->sin_port = port;
			memcpy((char *)ai->ai_addr + addroff, abuf, addrsize);
			if (flags & AI_CANONNAME) {
#if defined(LWRES_HAVE_SIN6_SCOPE_ID)
				if (ai->ai_family == AF_INET6)
					SIN6(ai->ai_addr)->sin6_scope_id =
									scopeid;
#endif
				if (lwres_getnameinfo(ai->ai_addr,
				    ai->ai_addrlen, nbuf, sizeof(nbuf),
						      NULL, 0,
						      NI_NUMERICHOST) == 0) {
					ai->ai_canonname = strdup(nbuf);
					if (ai->ai_canonname == NULL) {
						lwres_freeaddrinfo(ai_list);
						return (EAI_MEMORY);
					}
				} else {
					/* XXX raise error? */
					ai->ai_canonname = NULL;
				}
			}
			goto done;
		} else if ((flags & AI_NUMERICHOST) != 0) {
			return (EAI_NONAME);
		}
	}

	set_order(family, net_order);
	for (i = 0; i < FOUND_MAX; i++) {
		if (net_order[i] == NULL)
			break;
		err = (net_order[i])(hostname, flags, &ai_list,
				     socktype, port);
		if (err != 0)
			return (err);
	}

	if (ai_list == NULL)
		return (EAI_NODATA);

done:
	ai_list = ai_reverse(ai_list);

	*res = ai_list;
	return (0);
}

static char *
lwres_strsep(char **stringp, const char *delim) {
	char *string = *stringp;
	char *s;
	const char *d;
	char sc, dc;

	if (string == NULL)
		return (NULL);

	for (s = string; *s != '\0'; s++) {
		sc = *s;
		for (d = delim; (dc = *d) != '\0'; d++)
			if (sc == dc) {
				*s++ = '\0';
				*stringp = s;
				return (string);
			}
	}
	*stringp = NULL;
	return (string);
}

static void
set_order(int family, int (**net_order)(const char *, int, struct addrinfo **,
					int, int))
{
	char *order, *tok;
	int found;

	if (family) {
		switch (family) {
		case AF_INET:
			*net_order++ = add_ipv4;
			break;
		case AF_INET6:
			*net_order++ = add_ipv6;
			break;
		}
	} else {
		order = getenv("NET_ORDER");
		found = 0;
		while (order != NULL) {
			/*
			 * We ignore any unknown names.
			 */
			tok = lwres_strsep(&order, ":");
			if (strcasecmp(tok, "inet6") == 0) {
				if ((found & FOUND_IPV6) == 0)
					*net_order++ = add_ipv6;
				found |= FOUND_IPV6;
			} else if (strcasecmp(tok, "inet") == 0 ||
			    strcasecmp(tok, "inet4") == 0) {
				if ((found & FOUND_IPV4) == 0)
					*net_order++ = add_ipv4;
				found |= FOUND_IPV4;
			}
		}

		/*
		 * Add in anything that we didn't find.
		 */
		if ((found & FOUND_IPV4) == 0)
			*net_order++ = add_ipv4;
		if ((found & FOUND_IPV6) == 0)
			*net_order++ = add_ipv6;
	}
	*net_order = NULL;
	return;
}

static char v4_loop[4] = { 127, 0, 0, 1 };

/*
 * The test against 0 is there to keep the Solaris compiler
 * from complaining about "end-of-loop code not reached".
 */
#define SETERROR(code) \
	do { result = (code);			\
		if (result != 0) goto cleanup;	\
	} while (0)

static int
add_ipv4(const char *hostname, int flags, struct addrinfo **aip,
	int socktype, int port)
{
	struct addrinfo *ai;
	lwres_context_t *lwrctx = NULL;
	lwres_gabnresponse_t *by = NULL;
	lwres_addr_t *addr;
	lwres_result_t lwres;
	int result = 0;

	lwres = lwres_context_create(&lwrctx, NULL, NULL, NULL, 0);
	if (lwres != LWRES_R_SUCCESS)
		SETERROR(EAI_FAIL);
	(void) lwres_conf_parse(lwrctx, lwres_resolv_conf);
	if (hostname == NULL && (flags & AI_PASSIVE) == 0) {
		ai = ai_clone(*aip, AF_INET);
		if (ai == NULL) {
			lwres_freeaddrinfo(*aip);
			SETERROR(EAI_MEMORY);
		}

		*aip = ai;
		ai->ai_socktype = socktype;
		SIN(ai->ai_addr)->sin_port = port;
		memcpy(&SIN(ai->ai_addr)->sin_addr, v4_loop, 4);
	} else {
		lwres = lwres_getaddrsbyname(lwrctx, hostname,
					     LWRES_ADDRTYPE_V4, &by);
		if (lwres != LWRES_R_SUCCESS) {
			if (lwres == LWRES_R_NOTFOUND)
				goto cleanup;
			else
				SETERROR(EAI_FAIL);
		}
		addr = LWRES_LIST_HEAD(by->addrs);
		while (addr != NULL) {
			ai = ai_clone(*aip, AF_INET);
			if (ai == NULL) {
				lwres_freeaddrinfo(*aip);
				SETERROR(EAI_MEMORY);
			}
			*aip = ai;
			ai->ai_socktype = socktype;
			SIN(ai->ai_addr)->sin_port = port;
			memcpy(&SIN(ai->ai_addr)->sin_addr,
			       addr->address, 4);
			if (flags & AI_CANONNAME) {
				ai->ai_canonname = strdup(by->realname);
				if (ai->ai_canonname == NULL)
					SETERROR(EAI_MEMORY);
			}
			addr = LWRES_LIST_NEXT(addr, link);
		}
	}
 cleanup:
	if (by != NULL)
		lwres_gabnresponse_free(lwrctx, &by);
	if (lwrctx != NULL) {
		lwres_conf_clear(lwrctx);
		lwres_context_destroy(&lwrctx);
	}
	return (result);
}

static char v6_loop[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };

static int
add_ipv6(const char *hostname, int flags, struct addrinfo **aip,
	 int socktype, int port)
{
	struct addrinfo *ai;
	lwres_context_t *lwrctx = NULL;
	lwres_gabnresponse_t *by = NULL;
	lwres_addr_t *addr;
	lwres_result_t lwres;
	int result = 0;

	lwres = lwres_context_create(&lwrctx, NULL, NULL, NULL, 0);
	if (lwres != LWRES_R_SUCCESS)
		SETERROR(EAI_FAIL);
	(void) lwres_conf_parse(lwrctx, lwres_resolv_conf);

	if (hostname == NULL && (flags & AI_PASSIVE) == 0) {
		ai = ai_clone(*aip, AF_INET6);
		if (ai == NULL) {
			lwres_freeaddrinfo(*aip);
			SETERROR(EAI_MEMORY);
		}

		*aip = ai;
		ai->ai_socktype = socktype;
		SIN6(ai->ai_addr)->sin6_port = port;
		memcpy(&SIN6(ai->ai_addr)->sin6_addr, v6_loop, 16);
	} else {
		lwres = lwres_getaddrsbyname(lwrctx, hostname,
					     LWRES_ADDRTYPE_V6, &by);
		if (lwres != LWRES_R_SUCCESS) {
			if (lwres == LWRES_R_NOTFOUND)
				goto cleanup;
			else
				SETERROR(EAI_FAIL);
		}
		addr = LWRES_LIST_HEAD(by->addrs);
		while (addr != NULL) {
			ai = ai_clone(*aip, AF_INET6);
			if (ai == NULL) {
				lwres_freeaddrinfo(*aip);
				SETERROR(EAI_MEMORY);
			}
			*aip = ai;
			ai->ai_socktype = socktype;
			SIN6(ai->ai_addr)->sin6_port = port;
			memcpy(&SIN6(ai->ai_addr)->sin6_addr,
			       addr->address, 16);
			if (flags & AI_CANONNAME) {
				ai->ai_canonname = strdup(by->realname);
				if (ai->ai_canonname == NULL)
					SETERROR(EAI_MEMORY);
			}
			addr = LWRES_LIST_NEXT(addr, link);
		}
	}
 cleanup:
	if (by != NULL)
		lwres_gabnresponse_free(lwrctx, &by);
	if (lwrctx != NULL) {
		lwres_conf_clear(lwrctx);
		lwres_context_destroy(&lwrctx);
	}
	return (result);
}

/*% Free address info. */
void
lwres_freeaddrinfo(struct addrinfo *ai) {
	struct addrinfo *ai_next;

	while (ai != NULL) {
		ai_next = ai->ai_next;
		if (ai->ai_addr != NULL)
			free(ai->ai_addr);
		if (ai->ai_canonname)
			free(ai->ai_canonname);
		free(ai);
		ai = ai_next;
	}
}

#ifdef AF_LOCAL
static int
get_local(const char *name, int socktype, struct addrinfo **res) {
	struct addrinfo *ai;
	struct sockaddr_un *slocal;

	if (socktype == 0)
		return (EAI_SOCKTYPE);

	ai = ai_alloc(AF_LOCAL, sizeof(*slocal));
	if (ai == NULL)
		return (EAI_MEMORY);

	slocal = SLOCAL(ai->ai_addr);
	strncpy(slocal->sun_path, name, sizeof(slocal->sun_path));

	ai->ai_socktype = socktype;
	/*
	 * ai->ai_flags, ai->ai_protocol, ai->ai_canonname,
	 * and ai->ai_next were initialized to zero.
	 */

	*res = ai;
	return (0);
}
#endif

/*!
 * Allocate an addrinfo structure, and a sockaddr structure
 * of the specificed length.  We initialize:
 *	ai_addrlen
 *	ai_family
 *	ai_addr
 *	ai_addr->sa_family
 *	ai_addr->sa_len	(LWRES_PLATFORM_HAVESALEN)
 * and everything else is initialized to zero.
 */
static struct addrinfo *
ai_alloc(int family, int addrlen) {
	struct addrinfo *ai;

	ai = (struct addrinfo *)calloc(1, sizeof(*ai));
	if (ai == NULL)
		return (NULL);

	ai->ai_addr = SA(calloc(1, addrlen));
	if (ai->ai_addr == NULL) {
		free(ai);
		return (NULL);
	}
	ai->ai_addrlen = addrlen;
	ai->ai_family = family;
	ai->ai_addr->sa_family = family;
#ifdef LWRES_PLATFORM_HAVESALEN
	ai->ai_addr->sa_len = addrlen;
#endif
	return (ai);
}

static struct addrinfo *
ai_clone(struct addrinfo *oai, int family) {
	struct addrinfo *ai;

	ai = ai_alloc(family, ((family == AF_INET6) ?
	    sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in)));

	if (ai == NULL) {
		lwres_freeaddrinfo(oai);
		return (NULL);
	}
	if (oai == NULL)
		return (ai);

	ai->ai_flags = oai->ai_flags;
	ai->ai_socktype = oai->ai_socktype;
	ai->ai_protocol = oai->ai_protocol;
	ai->ai_canonname = NULL;
	ai->ai_next = oai;
	return (ai);
}

static struct addrinfo *
ai_reverse(struct addrinfo *oai) {
	struct addrinfo *nai, *tai;

	nai = NULL;

	while (oai != NULL) {
		/*
		 * Grab one off the old list.
		 */
		tai = oai;
		oai = oai->ai_next;
		/*
		 * Put it on the front of the new list.
		 */
		tai->ai_next = nai;
		nai = tai;
	}
	return (nai);
}
