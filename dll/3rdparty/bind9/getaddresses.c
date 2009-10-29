/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2001, 2002  Internet Software Consortium.
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

/* $Id: getaddresses.c,v 1.22 2007/06/19 23:47:16 tbox Exp $ */

/*! \file */

#include <config.h>
#include <string.h>

#include <isc/net.h>
#include <isc/netaddr.h>
#include <isc/netdb.h>
#include <isc/netscope.h>
#include <isc/result.h>
#include <isc/sockaddr.h>
#include <isc/util.h>

#include <bind9/getaddresses.h>

#ifdef HAVE_ADDRINFO
#ifdef HAVE_GETADDRINFO
#ifdef HAVE_GAISTRERROR
#define USE_GETADDRINFO
#endif
#endif
#endif

#ifndef USE_GETADDRINFO
#ifndef ISC_PLATFORM_NONSTDHERRNO
extern int h_errno;
#endif
#endif

isc_result_t
bind9_getaddresses(const char *hostname, in_port_t port,
		   isc_sockaddr_t *addrs, int addrsize, int *addrcount)
{
	struct in_addr in4;
	struct in6_addr in6;
	isc_boolean_t have_ipv4, have_ipv6;
	int i;

#ifdef USE_GETADDRINFO
	struct addrinfo *ai = NULL, *tmpai, hints;
	int result;
#else
	struct hostent *he;
#endif

	REQUIRE(hostname != NULL);
	REQUIRE(addrs != NULL);
	REQUIRE(addrcount != NULL);
	REQUIRE(addrsize > 0);

	have_ipv4 = ISC_TF((isc_net_probeipv4() == ISC_R_SUCCESS));
	have_ipv6 = ISC_TF((isc_net_probeipv6() == ISC_R_SUCCESS));

	/*
	 * Try IPv4, then IPv6.  In order to handle the extended format
	 * for IPv6 scoped addresses (address%scope_ID), we'll use a local
	 * working buffer of 128 bytes.  The length is an ad-hoc value, but
	 * should be enough for this purpose; the buffer can contain a string
	 * of at least 80 bytes for scope_ID in addition to any IPv6 numeric
	 * addresses (up to 46 bytes), the delimiter character and the
	 * terminating NULL character.
	 */
	if (inet_pton(AF_INET, hostname, &in4) == 1) {
		if (have_ipv4)
			isc_sockaddr_fromin(&addrs[0], &in4, port);
		else
			isc_sockaddr_v6fromin(&addrs[0], &in4, port);
		*addrcount = 1;
		return (ISC_R_SUCCESS);
	} else if (strlen(hostname) <= 127U) {
		char tmpbuf[128], *d;
		isc_uint32_t zone = 0;

		strcpy(tmpbuf, hostname);
		d = strchr(tmpbuf, '%');
		if (d != NULL)
			*d = '\0';

		if (inet_pton(AF_INET6, tmpbuf, &in6) == 1) {
			isc_netaddr_t na;

			if (!have_ipv6)
				return (ISC_R_FAMILYNOSUPPORT);

			if (d != NULL) {
#ifdef ISC_PLATFORM_HAVESCOPEID
				isc_result_t result;

				result = isc_netscope_pton(AF_INET6, d + 1,
							   &in6, &zone);
				    
				if (result != ISC_R_SUCCESS)
					return (result);
#else
				/*
				 * The extended format is specified while the
				 * system does not provide the ability to use
				 * it.  Throw an explicit error instead of
				 * ignoring the specified value.
				 */
				return (ISC_R_BADADDRESSFORM);
#endif
			}

			isc_netaddr_fromin6(&na, &in6);
			isc_netaddr_setzone(&na, zone);
			isc_sockaddr_fromnetaddr(&addrs[0],
						 (const isc_netaddr_t *)&na,
						 port);

			*addrcount = 1;
			return (ISC_R_SUCCESS);
			
		}
	}
#ifdef USE_GETADDRINFO
	memset(&hints, 0, sizeof(hints));
	if (!have_ipv6)
		hints.ai_family = PF_INET;
	else if (!have_ipv4)
		hints.ai_family = PF_INET6;
	else {
		hints.ai_family = PF_UNSPEC;
#ifdef AI_ADDRCONFIG
		hints.ai_flags = AI_ADDRCONFIG;
#endif
	}
	hints.ai_socktype = SOCK_STREAM;
#ifdef AI_ADDRCONFIG
 again:
#endif
	result = getaddrinfo(hostname, NULL, &hints, &ai);
	switch (result) {
	case 0:
		break;
	case EAI_NONAME:
#if defined(EAI_NODATA) && (EAI_NODATA != EAI_NONAME)
	case EAI_NODATA:
#endif
		return (ISC_R_NOTFOUND);
#ifdef AI_ADDRCONFIG
	case EAI_BADFLAGS:
		if ((hints.ai_flags & AI_ADDRCONFIG) != 0) {
			hints.ai_flags &= ~AI_ADDRCONFIG;
			goto again;
		}
#endif
	default:
		return (ISC_R_FAILURE);
	}
	for (tmpai = ai, i = 0;
	     tmpai != NULL && i < addrsize;
	     tmpai = tmpai->ai_next)
	{
		if (tmpai->ai_family != AF_INET &&
		    tmpai->ai_family != AF_INET6)
			continue;
		if (tmpai->ai_family == AF_INET) {
			struct sockaddr_in *sin;
			sin = (struct sockaddr_in *)tmpai->ai_addr;
			isc_sockaddr_fromin(&addrs[i], &sin->sin_addr, port);
		} else {
			struct sockaddr_in6 *sin6;
			sin6 = (struct sockaddr_in6 *)tmpai->ai_addr;
			isc_sockaddr_fromin6(&addrs[i], &sin6->sin6_addr,
					     port);
		}
		i++;

	}
	freeaddrinfo(ai);
	*addrcount = i;
#else
	he = gethostbyname(hostname);
	if (he == NULL) {
		switch (h_errno) {
		case HOST_NOT_FOUND:
#ifdef NO_DATA
		case NO_DATA:
#endif
#if defined(NO_ADDRESS) && (!defined(NO_DATA) || (NO_DATA != NO_ADDRESS))
		case NO_ADDRESS:
#endif
			return (ISC_R_NOTFOUND);
		default:
			return (ISC_R_FAILURE);
		}
	}
	if (he->h_addrtype != AF_INET && he->h_addrtype != AF_INET6)
		return (ISC_R_NOTFOUND);
	for (i = 0; i < addrsize; i++) {
		if (he->h_addrtype == AF_INET) {
			struct in_addr *inp;
			inp = (struct in_addr *)(he->h_addr_list[i]);
			if (inp == NULL)
				break;
			isc_sockaddr_fromin(&addrs[i], inp, port);
		} else {
			struct in6_addr *in6p;
			in6p = (struct in6_addr *)(he->h_addr_list[i]);
			if (in6p == NULL)
				break;
			isc_sockaddr_fromin6(&addrs[i], in6p, port);
		}
	}
	*addrcount = i;
#endif
	if (*addrcount == 0)
		return (ISC_R_NOTFOUND);
	else
		return (ISC_R_SUCCESS);
}
