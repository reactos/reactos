/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2003  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id: getipnode.c,v 1.42 2007/06/18 23:47:51 tbox Exp $ */

/*! \file */

/**
 *    These functions perform thread safe, protocol independent
 *    nodename-to-address and address-to-nodename translation as defined in
 *    RFC2553.  This use a struct hostent which is defined in namedb.h:
 * 
 * \code
 * struct  hostent {
 *         char    *h_name;        // official name of host
 *         char    **h_aliases;    // alias list
 *         int     h_addrtype;     // host address type
 *         int     h_length;       // length of address
 *         char    **h_addr_list;  // list of addresses from name server
 * };
 * #define h_addr  h_addr_list[0]  // address, for backward compatibility
 * \endcode
 * 
 *    The members of this structure are:
 * 
 * \li   h_name:
 *           The official (canonical) name of the host.
 * 
 * \li   h_aliases:
 *           A NULL-terminated array of alternate names (nicknames) for the
 *           host.
 * 
 * \li   h_addrtype:
 *           The type of address being returned - usually PF_INET or
 *           PF_INET6.
 * 
 * \li   h_length:
 *           The length of the address in bytes.
 * 
 * \li   h_addr_list:
 *           A NULL terminated array of network addresses for the host. Host
 *           addresses are returned in network byte order.
 * 
 *    lwres_getipnodebyname() looks up addresses of protocol family af for
 *    the hostname name. The flags parameter contains ORed flag bits to
 *    specify the types of addresses that are searched for, and the types of
 *    addresses that are returned. The flag bits are:
 * 
 * \li   #AI_V4MAPPED:
 *           This is used with an af of #AF_INET6, and causes IPv4 addresses
 *           to be returned as IPv4-mapped IPv6 addresses.
 * 
 * \li   #AI_ALL:
 *           This is used with an af of #AF_INET6, and causes all known
 *           addresses (IPv6 and IPv4) to be returned. If #AI_V4MAPPED is
 *           also set, the IPv4 addresses are return as mapped IPv6
 *           addresses.
 * 
 * \li   #AI_ADDRCONFIG:
 *           Only return an IPv6 or IPv4 address if here is an active
 *           network interface of that type. This is not currently
 *           implemented in the BIND 9 lightweight resolver, and the flag is
 *           ignored.
 * 
 * \li   #AI_DEFAULT:
 *           This default sets the #AI_V4MAPPED and #AI_ADDRCONFIG flag bits.
 * 
 *    lwres_getipnodebyaddr() performs a reverse lookup of address src which
 *    is len bytes long. af denotes the protocol family, typically PF_INET
 *    or PF_INET6.
 * 
 *    lwres_freehostent() releases all the memory associated with the struct
 *    hostent pointer. Any memory allocated for the h_name, h_addr_list
 *    and h_aliases is freed, as is the memory for the hostent structure
 *    itself.
 * 
 * \section getipnode_return Return Values
 * 
 *    If an error occurs, lwres_getipnodebyname() and
 *    lwres_getipnodebyaddr() set *error_num to an appropriate error code
 *    and the function returns a NULL pointer. The error codes and their
 *    meanings are defined in \link netdb.h <lwres/netdb.h>\endlink:
 * 
 * \li   #HOST_NOT_FOUND:
 *           No such host is known.
 * 
 * \li   #NO_ADDRESS:
 *           The server recognised the request and the name but no address
 *           is available. Another type of request to the name server for
 *           the domain might return an answer.
 * 
 * \li   #TRY_AGAIN:
 *           A temporary and possibly transient error occurred, such as a
 *           failure of a server to respond. The request may succeed if
 *           retried.
 * 
 * \li   #NO_RECOVERY:
 *           An unexpected failure occurred, and retrying the request is
 *           pointless.
 * 
 *    lwres_hstrerror() translates these error codes to suitable error
 *    messages.
 * 
 * \section getipnode_see See Also
 * 
 * getaddrinfo.c, gethost.c, getnameinfo.c, herror.c, RFC2553  
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <lwres/lwres.h>
#include <lwres/net.h>
#include <lwres/netdb.h>	/* XXX #include <netdb.h> */

#include "assert_p.h"

#ifndef INADDRSZ
#define INADDRSZ 4
#endif
#ifndef IN6ADDRSZ
#define IN6ADDRSZ 16
#endif

#ifdef LWRES_PLATFORM_NEEDIN6ADDRANY
LIBLWRES_EXTERNAL_DATA const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
#endif

#ifndef IN6_IS_ADDR_V4COMPAT
static const unsigned char in6addr_compat[12] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
#define IN6_IS_ADDR_V4COMPAT(x) (!memcmp((x)->s6_addr, in6addr_compat, 12) && \
                                 ((x)->s6_addr[12] != 0 || \
                                  (x)->s6_addr[13] != 0 || \
                                  (x)->s6_addr[14] != 0 || \
                                   ((x)->s6_addr[15] != 0 && \
                                    (x)->s6_addr[15] != 1)))
#endif
#ifndef IN6_IS_ADDR_V4MAPPED
#define IN6_IS_ADDR_V4MAPPED(x) (!memcmp((x)->s6_addr, in6addr_mapped, 12))
#endif

static const unsigned char in6addr_mapped[12] = {
        0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0xff, 0xff
};

/***
 ***	Forward declarations.
 ***/

static int
scan_interfaces(int *, int *);

static struct hostent *
copyandmerge(struct hostent *, struct hostent *, int, int *);

static struct hostent *
hostfromaddr(lwres_gnbaresponse_t *addr, int af, const void *src);

static struct hostent *
hostfromname(lwres_gabnresponse_t *name, int af);

/***
 ***	Public functions.
 ***/

/*!
 *	AI_V4MAPPED + AF_INET6
 *	If no IPv6 address then a query for IPv4 and map returned values.
 *
 *	AI_ALL + AI_V4MAPPED + AF_INET6
 *	Return IPv6 and IPv4 mapped.
 *
 *	AI_ADDRCONFIG
 *	Only return IPv6 / IPv4 address if there is an interface of that
 *	type active.
 */

struct hostent *
lwres_getipnodebyname(const char *name, int af, int flags, int *error_num) {
	int have_v4 = 1, have_v6 = 1;
	struct in_addr in4;
	struct in6_addr in6;
	struct hostent he, *he1 = NULL, *he2 = NULL, *he3 = NULL;
	int v4 = 0, v6 = 0;
	int tmp_err;
	lwres_context_t *lwrctx = NULL;
	lwres_gabnresponse_t *by = NULL;
	int n;

	/*
	 * If we care about active interfaces then check.
	 */
	if ((flags & AI_ADDRCONFIG) != 0)
		if (scan_interfaces(&have_v4, &have_v6) == -1) {
			*error_num = NO_RECOVERY;
			return (NULL);
		}

	/* Check for literal address. */
	if ((v4 = lwres_net_pton(AF_INET, name, &in4)) != 1)
		v6 = lwres_net_pton(AF_INET6, name, &in6);

	/*
	 * Impossible combination?
	 */
	if ((af == AF_INET6 && (flags & AI_V4MAPPED) == 0 && v4 == 1) ||
	    (af == AF_INET && v6 == 1) ||
	    (have_v4 == 0 && v4 == 1) ||
	    (have_v6 == 0 && v6 == 1) ||
	    (have_v4 == 0 && af == AF_INET) ||
	    (have_v6 == 0 && af == AF_INET6 &&
	     (((flags & AI_V4MAPPED) != 0 && have_v4) ||
	      (flags & AI_V4MAPPED) == 0))) {
		*error_num = HOST_NOT_FOUND;
		return (NULL);
	}

	/*
	 * Literal address?
	 */
	if (v4 == 1 || v6 == 1) {
		char *addr_list[2];
		char *aliases[1];
		char mappedname[sizeof("::ffff:123.123.123.123")];
		union {
			const char *const_name;
			char *deconst_name;
		} u;

		u.const_name = name;
		if (v4 == 1 && af == AF_INET6) {
			strcpy(mappedname, "::ffff:");
			lwres_net_ntop(AF_INET, (char *)&in4,
				       mappedname + sizeof("::ffff:") - 1,
				       sizeof(mappedname) - sizeof("::ffff:")
				       + 1);
			he.h_name = mappedname;
		} else
			he.h_name = u.deconst_name;
		he.h_addr_list = addr_list;
		he.h_addr_list[0] = (v4 == 1) ? (char *)&in4 : (char *)&in6;
		he.h_addr_list[1] = NULL;
		he.h_aliases = aliases;
		he.h_aliases[0] = NULL;
		he.h_length = (v4 == 1) ? INADDRSZ : IN6ADDRSZ;
		he.h_addrtype = (v4 == 1) ? AF_INET : AF_INET6;
		return (copyandmerge(&he, NULL, af, error_num));
	}

	n = lwres_context_create(&lwrctx, NULL, NULL, NULL, 0);
	if (n != 0) {
		*error_num = NO_RECOVERY;
		goto cleanup;
	}
	(void) lwres_conf_parse(lwrctx, lwres_resolv_conf);
	tmp_err = NO_RECOVERY;
	if (have_v6 && af == AF_INET6) {

		n = lwres_getaddrsbyname(lwrctx, name, LWRES_ADDRTYPE_V6, &by);
		if (n == 0) {
			he1 = hostfromname(by, AF_INET6);
			lwres_gabnresponse_free(lwrctx, &by);
			if (he1 == NULL) {
				*error_num = NO_RECOVERY;
				goto cleanup;
			}
		} else {
			tmp_err = HOST_NOT_FOUND;
		}
	}

	if (have_v4 &&
	    ((af == AF_INET) ||
	     (af == AF_INET6 && (flags & AI_V4MAPPED) != 0 &&
	      (he1 == NULL || (flags & AI_ALL) != 0)))) {
		n = lwres_getaddrsbyname(lwrctx, name, LWRES_ADDRTYPE_V4, &by);
		if (n == 0) {
			he2 = hostfromname(by, AF_INET);
			lwres_gabnresponse_free(lwrctx, &by);
			if (he2 == NULL) {
				*error_num = NO_RECOVERY;
				goto cleanup;
			}
		} else if (he1 == NULL) {
			if (n == LWRES_R_NOTFOUND)
				*error_num = HOST_NOT_FOUND;
			else
				*error_num = NO_RECOVERY;
			goto cleanup;
		}
	} else
		*error_num = tmp_err;

	he3 = copyandmerge(he1, he2, af, error_num);

 cleanup:
	if (he1 != NULL)
		lwres_freehostent(he1);
	if (he2 != NULL)
		lwres_freehostent(he2);
	if (lwrctx != NULL) {
		lwres_conf_clear(lwrctx);
		lwres_context_destroy(&lwrctx);
	}
	return (he3);
}

/*% performs a reverse lookup of address src which is len bytes long. af denotes the protocol family, typically #PF_INET or PF_INET6. */
struct hostent *
lwres_getipnodebyaddr(const void *src, size_t len, int af, int *error_num) {
	struct hostent *he1, *he2;
	lwres_context_t *lwrctx = NULL;
	lwres_gnbaresponse_t *by = NULL;
	lwres_result_t n;
	union {
		const void *konst;
		struct in6_addr *in6;
	} u;

	/*
	 * Sanity checks.
	 */
	if (src == NULL) {
		*error_num = NO_RECOVERY;
		return (NULL);
	}

	switch (af) {
	case AF_INET:
		if (len != (unsigned int)INADDRSZ) {
			*error_num = NO_RECOVERY;
			return (NULL);
		}
		break;
	case AF_INET6:
		if (len != (unsigned int)IN6ADDRSZ) {
			*error_num = NO_RECOVERY;
			return (NULL);
		}
		break;
	default:
		*error_num = NO_RECOVERY;
		return (NULL);
	}

	/*
	 * The de-"const"-ing game is done because at least one
	 * vendor's system (RedHat 6.0) defines the IN6_IS_ADDR_*
	 * macros in such a way that they discard the const with
	 * internal casting, and gcc ends up complaining.  Rather
	 * than replacing their own (possibly optimized) definitions
	 * with our own, cleanly discarding the const is the easiest
	 * thing to do.
	 */
	u.konst = src;

	/*
	 * Look up IPv4 and IPv4 mapped/compatible addresses.
	 */
	if ((af == AF_INET6 && IN6_IS_ADDR_V4COMPAT(u.in6)) ||
	    (af == AF_INET6 && IN6_IS_ADDR_V4MAPPED(u.in6)) ||
	    (af == AF_INET)) {
		const unsigned char *cp = src;

		if (af == AF_INET6)
			cp += 12;
		n = lwres_context_create(&lwrctx, NULL, NULL, NULL, 0);
		if (n == LWRES_R_SUCCESS)
			(void) lwres_conf_parse(lwrctx, lwres_resolv_conf);
		if (n == LWRES_R_SUCCESS)
			n = lwres_getnamebyaddr(lwrctx, LWRES_ADDRTYPE_V4,
						INADDRSZ, cp, &by);
		if (n != LWRES_R_SUCCESS) {
			lwres_conf_clear(lwrctx);
			lwres_context_destroy(&lwrctx);
			if (n == LWRES_R_NOTFOUND)
				*error_num = HOST_NOT_FOUND;
			else
				*error_num = NO_RECOVERY;
			return (NULL);
		}
		he1 = hostfromaddr(by, AF_INET, cp);
		lwres_gnbaresponse_free(lwrctx, &by);
		lwres_conf_clear(lwrctx);
		lwres_context_destroy(&lwrctx);
		if (af != AF_INET6)
			return (he1);

		/*
		 * Convert from AF_INET to AF_INET6.
		 */
		he2 = copyandmerge(he1, NULL, af, error_num);
		lwres_freehostent(he1);
		if (he2 == NULL)
			return (NULL);
		/*
		 * Restore original address.
		 */
		memcpy(he2->h_addr, src, len);
		return (he2);
	}

	/*
	 * Lookup IPv6 address.
	 */
	if (memcmp(src, &in6addr_any, IN6ADDRSZ) == 0) {
		*error_num = HOST_NOT_FOUND;
		return (NULL);
	}

	n = lwres_context_create(&lwrctx, NULL, NULL, NULL, 0);
	if (n == LWRES_R_SUCCESS)
		(void) lwres_conf_parse(lwrctx, lwres_resolv_conf);
	if (n == LWRES_R_SUCCESS)
		n = lwres_getnamebyaddr(lwrctx, LWRES_ADDRTYPE_V6, IN6ADDRSZ,
					src, &by);
	if (n != 0) {
		lwres_conf_clear(lwrctx);
		lwres_context_destroy(&lwrctx);
		*error_num = HOST_NOT_FOUND;
		return (NULL);
	}
	he1 = hostfromaddr(by, AF_INET6, src);
	lwres_gnbaresponse_free(lwrctx, &by);
	if (he1 == NULL)
		*error_num = NO_RECOVERY;
	lwres_conf_clear(lwrctx);
	lwres_context_destroy(&lwrctx);
	return (he1);
}

/*% releases all the memory associated with the struct hostent pointer */
void
lwres_freehostent(struct hostent *he) {
	char **cpp;
	int names = 1;
	int addresses = 1;

	free(he->h_name);

	cpp = he->h_addr_list;
	while (*cpp != NULL) {
		free(*cpp);
		*cpp = NULL;
		cpp++;
		addresses++;
	}

	cpp = he->h_aliases;
	while (*cpp != NULL) {
		free(*cpp);
		cpp++;
		names++;
	}

	free(he->h_aliases);
	free(he->h_addr_list);
	free(he);
}

/*
 * Private
 */

/*
 * Scan the interface table and set have_v4 and have_v6 depending
 * upon whether there are IPv4 and IPv6 interface addresses.
 *
 * Returns:
 *	0 on success
 *	-1 on failure.
 */

#if defined(SIOCGLIFCONF) && defined(SIOCGLIFADDR) && \
    !defined(IRIX_EMUL_IOCTL_SIOCGIFCONF) 

#ifdef __hpux
#define lifc_len iflc_len
#define lifc_buf iflc_buf
#define lifc_req iflc_req
#define LIFCONF if_laddrconf
#else
#define ISC_HAVE_LIFC_FAMILY 1
#define ISC_HAVE_LIFC_FLAGS 1
#define LIFCONF lifconf
#endif
 
#ifdef __hpux
#define lifr_addr iflr_addr
#define lifr_name iflr_name
#define lifr_dstaddr iflr_dstaddr
#define lifr_flags iflr_flags
#define ss_family sa_family
#define LIFREQ if_laddrreq
#else
#define LIFREQ lifreq
#endif

static int
scan_interfaces6(int *have_v4, int *have_v6) {
	struct LIFCONF lifc;
	struct LIFREQ lifreq;
	struct in_addr in4;
	struct in6_addr in6;
	char *buf = NULL, *cp, *cplim;
	static unsigned int bufsiz = 4095;
	int s, cpsize, n;

	/*
	 * Set to zero.  Used as loop terminators below.
	 */
	*have_v4 = *have_v6 = 0;

	/*
	 * Get interface list from system.
	 */
	if ((s = socket(AF_INET6, SOCK_DGRAM, 0)) == -1)
		goto err_ret;

	/*
	 * Grow buffer until large enough to contain all interface
	 * descriptions.
	 */
	for (;;) {
		buf = malloc(bufsiz);
		if (buf == NULL)
			goto err_ret;
#ifdef ISC_HAVE_LIFC_FAMILY
		lifc.lifc_family = AF_UNSPEC;	/* request all families */
#endif
#ifdef ISC_HAVE_LIFC_FLAGS
		lifc.lifc_flags = 0;
#endif
		lifc.lifc_len = bufsiz;
		lifc.lifc_buf = buf;
		if ((n = ioctl(s, SIOCGLIFCONF, (char *)&lifc)) != -1) {
			/*
			 * Some OS's just return what will fit rather
			 * than set EINVAL if the buffer is too small
			 * to fit all the interfaces in.  If 
			 * lifc.lifc_len is too near to the end of the
			 * buffer we will grow it just in case and
			 * retry.
			 */
			if (lifc.lifc_len + 2 * sizeof(lifreq) < bufsiz)
				break;
		}
		if ((n == -1) && errno != EINVAL)
			goto err_ret;

		if (bufsiz > 1000000)
			goto err_ret;

		free(buf);
		bufsiz += 4096;
	}

	/*
	 * Parse system's interface list.
	 */
	cplim = buf + lifc.lifc_len;    /* skip over if's with big ifr_addr's */
	for (cp = buf;
	     (*have_v4 == 0 || *have_v6 == 0) && cp < cplim;
	     cp += cpsize) {
		memcpy(&lifreq, cp, sizeof(lifreq));
#ifdef LWRES_PLATFORM_HAVESALEN
#ifdef FIX_ZERO_SA_LEN
		if (lifreq.lifr_addr.sa_len == 0)
			lifreq.lifr_addr.sa_len = 16;
#endif
#ifdef HAVE_MINIMUM_IFREQ
		cpsize = sizeof(lifreq);
		if (lifreq.lifr_addr.sa_len > sizeof(struct sockaddr))
			cpsize += (int)lifreq.lifr_addr.sa_len -
				(int)(sizeof(struct sockaddr));
#else
		cpsize = sizeof(lifreq.lifr_name) + lifreq.lifr_addr.sa_len;
#endif /* HAVE_MINIMUM_IFREQ */
#elif defined SIOCGIFCONF_ADDR
		cpsize = sizeof(lifreq);
#else
		cpsize = sizeof(lifreq.lifr_name);
		/* XXX maybe this should be a hard error? */
		if (ioctl(s, SIOCGLIFADDR, (char *)&lifreq) < 0)
			continue;
#endif
		switch (lifreq.lifr_addr.ss_family) {
		case AF_INET:
			if (*have_v4 == 0) {
				memcpy(&in4,
				       &((struct sockaddr_in *)
				       &lifreq.lifr_addr)->sin_addr,
				       sizeof(in4));
				if (in4.s_addr == INADDR_ANY)
					break;
				n = ioctl(s, SIOCGLIFFLAGS, (char *)&lifreq);
				if (n < 0)
					break;
				if ((lifreq.lifr_flags & IFF_UP) == 0)
					break;
				*have_v4 = 1;
			} 
			break;
		case AF_INET6:
			if (*have_v6 == 0) {
				memcpy(&in6,
				       &((struct sockaddr_in6 *)
				       &lifreq.lifr_addr)->sin6_addr, 
				       sizeof(in6));
				if (memcmp(&in6, &in6addr_any,
					   sizeof(in6)) == 0)
					break;
				n = ioctl(s, SIOCGLIFFLAGS, (char *)&lifreq);
				if (n < 0)
					break;
				if ((lifreq.lifr_flags & IFF_UP) == 0)
					break;
				*have_v6 = 1;
			}
			break;
		}
	}
	if (buf != NULL)
		free(buf);
	close(s);
	return (0);
 err_ret:
	if (buf != NULL)
		free(buf);
	if (s != -1)
		close(s);
	return (-1);
}
#endif

static int
scan_interfaces(int *have_v4, int *have_v6) {
#if !defined(SIOCGIFCONF) || !defined(SIOCGIFADDR)
	*have_v4 = *have_v6 = 1;
	return (0);
#else
	struct ifconf ifc;
	union {
		char _pad[256];		/* leave space for IPv6 addresses */
		struct ifreq ifreq;
	} u;
	struct in_addr in4;
	struct in6_addr in6;
	char *buf = NULL, *cp, *cplim;
	static unsigned int bufsiz = 4095;
	int s, n;
	size_t cpsize;

#ifdef WIN32
	InitSockets();
#endif
#if defined(SIOCGLIFCONF) && defined(SIOCGLIFADDR) && \
    !defined(IRIX_EMUL_IOCTL_SIOCGIFCONF) 
	/*
	 * Try to scan the interfaces using IPv6 ioctls().
	 */
	if (!scan_interfaces6(have_v4, have_v6)) {
#ifdef WIN32
		DestroySockets();
#endif
		return (0);
	}
#endif

	/*
	 * Set to zero.  Used as loop terminators below.
	 */
	*have_v4 = *have_v6 = 0;

	/*
	 * Get interface list from system.
	 */
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		goto err_ret;

	/*
	 * Grow buffer until large enough to contain all interface
	 * descriptions.
	 */
	for (;;) {
		buf = malloc(bufsiz);
		if (buf == NULL)
			goto err_ret;
		ifc.ifc_len = bufsiz;
		ifc.ifc_buf = buf;
#ifdef IRIX_EMUL_IOCTL_SIOCGIFCONF
		/*
		 * This is a fix for IRIX OS in which the call to ioctl with
		 * the flag SIOCGIFCONF may not return an entry for all the
		 * interfaces like most flavors of Unix.
		 */
		if (emul_ioctl(&ifc) >= 0)
			break;
#else
		if ((n = ioctl(s, SIOCGIFCONF, (char *)&ifc)) != -1) {
			/*
			 * Some OS's just return what will fit rather
			 * than set EINVAL if the buffer is too small
			 * to fit all the interfaces in.  If 
			 * ifc.ifc_len is too near to the end of the
			 * buffer we will grow it just in case and
			 * retry.
			 */
			if (ifc.ifc_len + 2 * sizeof(u.ifreq) < bufsiz)
				break;
		}
#endif
		if ((n == -1) && errno != EINVAL)
			goto err_ret;

		if (bufsiz > 1000000)
			goto err_ret;

		free(buf);
		bufsiz += 4096;
	}

	/*
	 * Parse system's interface list.
	 */
	cplim = buf + ifc.ifc_len;    /* skip over if's with big ifr_addr's */
	for (cp = buf;
	     (*have_v4 == 0 || *have_v6 == 0) && cp < cplim;
	     cp += cpsize) {
		memcpy(&u.ifreq, cp, sizeof(u.ifreq));
#ifdef LWRES_PLATFORM_HAVESALEN
#ifdef FIX_ZERO_SA_LEN
		if (u.ifreq.ifr_addr.sa_len == 0)
			u.ifreq.ifr_addr.sa_len = 16;
#endif
#ifdef HAVE_MINIMUM_IFREQ
		cpsize = sizeof(u.ifreq);
		if (u.ifreq.ifr_addr.sa_len > sizeof(struct sockaddr))
			cpsize += (int)u.ifreq.ifr_addr.sa_len -
				(int)(sizeof(struct sockaddr));
#else
		cpsize = sizeof(u.ifreq.ifr_name) + u.ifreq.ifr_addr.sa_len;
#endif /* HAVE_MINIMUM_IFREQ */
		if (cpsize > sizeof(u.ifreq) && cpsize <= sizeof(u))
			memcpy(&u.ifreq, cp, cpsize);
#elif defined SIOCGIFCONF_ADDR
		cpsize = sizeof(u.ifreq);
#else
		cpsize = sizeof(u.ifreq.ifr_name);
		/* XXX maybe this should be a hard error? */
		if (ioctl(s, SIOCGIFADDR, (char *)&u.ifreq) < 0)
			continue;
#endif
		switch (u.ifreq.ifr_addr.sa_family) {
		case AF_INET:
			if (*have_v4 == 0) {
				memcpy(&in4,
				       &((struct sockaddr_in *)
				       &u.ifreq.ifr_addr)->sin_addr,
				       sizeof(in4));
				if (in4.s_addr == INADDR_ANY)
					break;
				n = ioctl(s, SIOCGIFFLAGS, (char *)&u.ifreq);
				if (n < 0)
					break;
				if ((u.ifreq.ifr_flags & IFF_UP) == 0)
					break;
				*have_v4 = 1;
			} 
			break;
		case AF_INET6:
			if (*have_v6 == 0) {
				memcpy(&in6,
				       &((struct sockaddr_in6 *)
				       &u.ifreq.ifr_addr)->sin6_addr,
				       sizeof(in6));
				if (memcmp(&in6, &in6addr_any,
					   sizeof(in6)) == 0)
					break;
				n = ioctl(s, SIOCGIFFLAGS, (char *)&u.ifreq);
				if (n < 0)
					break;
				if ((u.ifreq.ifr_flags & IFF_UP) == 0)
					break;
				*have_v6 = 1;
			}
			break;
		}
	}
	if (buf != NULL)
		free(buf);
#ifdef WIN32
	DestroySockets();
#endif
	close(s);
	return (0);

 err_ret:
	if (buf != NULL)
		free(buf);
	if (s != -1)
		close(s);
#ifdef WIN32
	DestroySockets();
#endif
	return (-1);
#endif
}

static struct hostent *
copyandmerge(struct hostent *he1, struct hostent *he2, int af, int *error_num)
{
	struct hostent *he = NULL;
	int addresses = 1;	/* NULL terminator */
	int names = 1;		/* NULL terminator */
	int len = 0;
	char **cpp, **npp;

	/*
	 * Work out array sizes.
	 */
	if (he1 != NULL) {
		cpp = he1->h_addr_list;
		while (*cpp != NULL) {
			addresses++;
			cpp++;
		}
		cpp = he1->h_aliases;
		while (*cpp != NULL) {
			names++;
			cpp++;
		}
	}

	if (he2 != NULL) {
		cpp = he2->h_addr_list;
		while (*cpp != NULL) {
			addresses++;
			cpp++;
		}
		if (he1 == NULL) {
			cpp = he2->h_aliases;
			while (*cpp != NULL) {
				names++;
				cpp++;
			}
		}
	}

	if (addresses == 1) {
		*error_num = NO_ADDRESS;
		return (NULL);
	}

	he = malloc(sizeof(*he));
	if (he == NULL)
		goto no_recovery;

	he->h_addr_list = malloc(sizeof(char *) * (addresses));
	if (he->h_addr_list == NULL)
		goto cleanup0;
	memset(he->h_addr_list, 0, sizeof(char *) * (addresses));

	/*
	 * Copy addresses.
	 */
	npp = he->h_addr_list;
	if (he1 != NULL) {
		cpp = he1->h_addr_list;
		while (*cpp != NULL) {
			*npp = malloc((af == AF_INET) ? INADDRSZ : IN6ADDRSZ);
			if (*npp == NULL)
				goto cleanup1;
			/*
			 * Convert to mapped if required.
			 */
			if (af == AF_INET6 && he1->h_addrtype == AF_INET) {
				memcpy(*npp, in6addr_mapped,
				       sizeof(in6addr_mapped));
				memcpy(*npp + sizeof(in6addr_mapped), *cpp,
				       INADDRSZ);
			} else {
				memcpy(*npp, *cpp,
				       (af == AF_INET) ? INADDRSZ : IN6ADDRSZ);
			}
			cpp++;
			npp++;
		}
	}

	if (he2 != NULL) {
		cpp = he2->h_addr_list;
		while (*cpp != NULL) {
			*npp = malloc((af == AF_INET) ? INADDRSZ : IN6ADDRSZ);
			if (*npp == NULL)
				goto cleanup1;
			/*
			 * Convert to mapped if required.
			 */
			if (af == AF_INET6 && he2->h_addrtype == AF_INET) {
				memcpy(*npp, in6addr_mapped,
				       sizeof(in6addr_mapped));
				memcpy(*npp + sizeof(in6addr_mapped), *cpp,
				       INADDRSZ);
			} else {
				memcpy(*npp, *cpp,
				       (af == AF_INET) ? INADDRSZ : IN6ADDRSZ);
			}
			cpp++;
			npp++;
		}
	}

	he->h_aliases = malloc(sizeof(char *) * (names));
	if (he->h_aliases == NULL)
		goto cleanup1;
	memset(he->h_aliases, 0, sizeof(char *) * (names));

	/*
	 * Copy aliases.
	 */
	npp = he->h_aliases;
	cpp = (he1 != NULL) ? he1->h_aliases : he2->h_aliases;
	while (*cpp != NULL) {
		len = strlen (*cpp) + 1;
		*npp = malloc(len);
		if (*npp == NULL)
			goto cleanup2;
		strcpy(*npp, *cpp);
		npp++;
		cpp++;
	}

	/*
	 * Copy hostname.
	 */
	he->h_name = malloc(strlen((he1 != NULL) ?
			    he1->h_name : he2->h_name) + 1);
	if (he->h_name == NULL)
		goto cleanup2;
	strcpy(he->h_name, (he1 != NULL) ? he1->h_name : he2->h_name);

	/*
	 * Set address type and length.
	 */
	he->h_addrtype = af;
	he->h_length = (af == AF_INET) ? INADDRSZ : IN6ADDRSZ;
	return (he);

 cleanup2:
	cpp = he->h_aliases;
	while (*cpp != NULL) {
		free(*cpp);
		cpp++;
	}
	free(he->h_aliases);

 cleanup1:
	cpp = he->h_addr_list;
	while (*cpp != NULL) {
		free(*cpp);
		*cpp = NULL;
		cpp++;
	}
	free(he->h_addr_list);

 cleanup0:
	free(he);

 no_recovery:
	*error_num = NO_RECOVERY;
	return (NULL);
}

static struct hostent *
hostfromaddr(lwres_gnbaresponse_t *addr, int af, const void *src) {
	struct hostent *he;
	int i;

	he = malloc(sizeof(*he));
	if (he == NULL)
		goto cleanup;
	memset(he, 0, sizeof(*he));

	/*
	 * Set family and length.
	 */
	he->h_addrtype = af;
	switch (af) {
	case AF_INET:
		he->h_length = INADDRSZ;
		break;
	case AF_INET6:
		he->h_length = IN6ADDRSZ;
		break;
	default:
		INSIST(0);
	}

	/*
	 * Copy name.
	 */
	he->h_name = strdup(addr->realname);
	if (he->h_name == NULL)
		goto cleanup;

	/*
	 * Copy aliases.
	 */
	he->h_aliases = malloc(sizeof(char *) * (addr->naliases + 1));
	if (he->h_aliases == NULL)
		goto cleanup;
	for (i = 0; i < addr->naliases; i++) {
		he->h_aliases[i] = strdup(addr->aliases[i]);
		if (he->h_aliases[i] == NULL)
			goto cleanup;
	}
	he->h_aliases[i] = NULL;

	/*
	 * Copy address.
	 */
	he->h_addr_list = malloc(sizeof(char *) * 2);
	if (he->h_addr_list == NULL)
		goto cleanup;
	he->h_addr_list[0] = malloc(he->h_length);
	if (he->h_addr_list[0] == NULL)
		goto cleanup;
	memcpy(he->h_addr_list[0], src, he->h_length);
	he->h_addr_list[1] = NULL;
	return (he);

 cleanup:
	if (he != NULL && he->h_addr_list != NULL) {
		for (i = 0; he->h_addr_list[i] != NULL; i++)
			free(he->h_addr_list[i]);
		free(he->h_addr_list);
	}
	if (he != NULL && he->h_aliases != NULL) {
		for (i = 0; he->h_aliases[i] != NULL; i++)
			free(he->h_aliases[i]);
		free(he->h_aliases);
	}
	if (he != NULL && he->h_name != NULL)
		free(he->h_name);
	if (he != NULL)
		free(he);
	return (NULL);
}

static struct hostent *
hostfromname(lwres_gabnresponse_t *name, int af) {
	struct hostent *he;
	int i;
	lwres_addr_t *addr;

	he = malloc(sizeof(*he));
	if (he == NULL)
		goto cleanup;
	memset(he, 0, sizeof(*he));

	/*
	 * Set family and length.
	 */
	he->h_addrtype = af;
	switch (af) {
	case AF_INET:
		he->h_length = INADDRSZ;
		break;
	case AF_INET6:
		he->h_length = IN6ADDRSZ;
		break;
	default:
		INSIST(0);
	}

	/*
	 * Copy name.
	 */
	he->h_name = strdup(name->realname);
	if (he->h_name == NULL)
		goto cleanup;

	/*
	 * Copy aliases.
	 */
	he->h_aliases = malloc(sizeof(char *) * (name->naliases + 1));
	for (i = 0; i < name->naliases; i++) {
		he->h_aliases[i] = strdup(name->aliases[i]);
		if (he->h_aliases[i] == NULL)
			goto cleanup;
	}
	he->h_aliases[i] = NULL;

	/*
	 * Copy addresses.
	 */
	he->h_addr_list = malloc(sizeof(char *) * (name->naddrs + 1));
	addr = LWRES_LIST_HEAD(name->addrs);
	i = 0;
	while (addr != NULL) {
		he->h_addr_list[i] = malloc(he->h_length);
		if (he->h_addr_list[i] == NULL)
			goto cleanup;
		memcpy(he->h_addr_list[i], addr->address, he->h_length);
		addr = LWRES_LIST_NEXT(addr, link);
		i++;
	}
	he->h_addr_list[i] = NULL;
	return (he);

 cleanup:
	if (he != NULL && he->h_addr_list != NULL) {
		for (i = 0; he->h_addr_list[i] != NULL; i++)
			free(he->h_addr_list[i]);
		free(he->h_addr_list);
	}
	if (he != NULL && he->h_aliases != NULL) {
		for (i = 0; he->h_aliases[i] != NULL; i++)
			free(he->h_aliases[i]);
		free(he->h_aliases);
	}
	if (he != NULL && he->h_name != NULL)
		free(he->h_name);
	if (he != NULL)
		free(he);
	return (NULL);
}
