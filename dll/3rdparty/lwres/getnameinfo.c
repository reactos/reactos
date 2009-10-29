/*
 * Portions Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Portions Copyright (C) 1999-2001, 2003  Internet Software Consortium.
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

/* $Id: getnameinfo.c,v 1.39 2007/06/19 23:47:22 tbox Exp $ */

/*! \file */

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * XXX
 * Issues to be discussed:
 * - Return values.  There seems to be no standard for return value (RFC2553)
 *   but INRIA implementation returns EAI_xxx defined for getaddrinfo().
 */


/**
 *    This function is equivalent to the getnameinfo(3) function defined in
 *    RFC2133. lwres_getnameinfo() returns the hostname for the struct
 *    sockaddr sa which is salen bytes long. The hostname is of length
 *    hostlen and is returned via *host. The maximum length of the hostname
 *    is 1025 bytes: #NI_MAXHOST.
 * 
 *    The name of the service associated with the port number in sa is
 *    returned in *serv. It is servlen bytes long. The maximum length of the
 *    service name is #NI_MAXSERV - 32 bytes.
 * 
 *    The flags argument sets the following bits:
 * 
 * \li   #NI_NOFQDN:
 *           A fully qualified domain name is not required for local hosts.
 *           The local part of the fully qualified domain name is returned
 *           instead.
 * 
 * \li   #NI_NUMERICHOST
 *           Return the address in numeric form, as if calling inet_ntop(),
 *           instead of a host name.
 * 
 * \li   #NI_NAMEREQD
 *           A name is required. If the hostname cannot be found in the DNS
 *           and this flag is set, a non-zero error code is returned. If the
 *           hostname is not found and the flag is not set, the address is
 *           returned in numeric form.
 * 
 * \li   #NI_NUMERICSERV
 *           The service name is returned as a digit string representing the
 *           port number.
 * 
 * \li   #NI_DGRAM
 *           Specifies that the service being looked up is a datagram
 *           service, and causes getservbyport() to be called with a second
 *           argument of "udp" instead of its default of "tcp". This is
 *           required for the few ports (512-514) that have different
 *           services for UDP and TCP.
 * 
 * \section getnameinfo_return Return Values
 * 
 *    lwres_getnameinfo() returns 0 on success or a non-zero error code if
 *    an error occurs.
 * 
 * \section getname_see See Also
 * 
 *    RFC2133, getservbyport(), 
 *    lwres_getnamebyaddr(). lwres_net_ntop().
 * 
 * \section getnameinfo_bugs Bugs
 * 
 *    RFC2133 fails to define what the nonzero return values of
 *    getnameinfo() are.
 */

#include <config.h>

#include <stdio.h>
#include <string.h>

#include <lwres/lwres.h>
#include <lwres/net.h>
#include <lwres/netdb.h>
#include "print_p.h"

#include "assert_p.h"

#define SUCCESS 0

/*% afd structure definition */
static struct afd {
	int a_af;
	size_t a_addrlen;
	size_t a_socklen;
} afdl [] = {
	/*!
	 * First entry is linked last...
	 */
	{ AF_INET, sizeof(struct in_addr), sizeof(struct sockaddr_in) },
	{ AF_INET6, sizeof(struct in6_addr), sizeof(struct sockaddr_in6) },
	{0, 0, 0},
};

#define ENI_NOSERVNAME	1
#define ENI_NOHOSTNAME	2
#define ENI_MEMORY	3
#define ENI_SYSTEM	4
#define ENI_FAMILY	5
#define ENI_SALEN	6
#define ENI_NOSOCKET 	7

/*!
 * The test against 0 is there to keep the Solaris compiler
 * from complaining about "end-of-loop code not reached".
 */
#define ERR(code) \
	do { result = (code);			\
		if (result != 0) goto cleanup;	\
	} while (0)

/*% lightweight resolver socket address structure to hostname and service name */
int
lwres_getnameinfo(const struct sockaddr *sa, size_t salen, char *host,
		  size_t hostlen, char *serv, size_t servlen, int flags)
{
	struct afd *afd;
	struct servent *sp;
	unsigned short port;
#ifdef LWRES_PLATFORM_HAVESALEN
	size_t len;
#endif
	int family, i;
	const void *addr;
	char *p;
#if 0
	unsigned long v4a;
	unsigned char pfx;
#endif
	char numserv[sizeof("65000")];
	char numaddr[sizeof("abcd:abcd:abcd:abcd:abcd:abcd:255.255.255.255")
		    + 1 + sizeof("4294967295")];
	const char *proto;
	lwres_uint32_t lwf = 0;
	lwres_context_t *lwrctx = NULL;
	lwres_gnbaresponse_t *by = NULL;
	int result = SUCCESS;
	int n;

	if (sa == NULL)
		ERR(ENI_NOSOCKET);

#ifdef LWRES_PLATFORM_HAVESALEN
	len = sa->sa_len;
	if (len != salen)
		ERR(ENI_SALEN);
#endif

	family = sa->sa_family;
	for (i = 0; afdl[i].a_af; i++)
		if (afdl[i].a_af == family) {
			afd = &afdl[i];
			goto found;
		}
	ERR(ENI_FAMILY);

 found:
	if (salen != afd->a_socklen)
		ERR(ENI_SALEN);

	switch (family) {
	case AF_INET:
		port = ((const struct sockaddr_in *)sa)->sin_port;
		addr = &((const struct sockaddr_in *)sa)->sin_addr.s_addr;
		break;

	case AF_INET6:
		port = ((const struct sockaddr_in6 *)sa)->sin6_port;
		addr = ((const struct sockaddr_in6 *)sa)->sin6_addr.s6_addr;
		break;

	default:
		port = 0;
		addr = NULL;
		INSIST(0);
	}
	proto = (flags & NI_DGRAM) ? "udp" : "tcp";

	if (serv == NULL || servlen == 0U) {
		/*
		 * Caller does not want service.
		 */
	} else if ((flags & NI_NUMERICSERV) != 0 ||
		   (sp = getservbyport(port, proto)) == NULL) {
		snprintf(numserv, sizeof(numserv), "%d", ntohs(port));
		if ((strlen(numserv) + 1) > servlen)
			ERR(ENI_MEMORY);
		strcpy(serv, numserv);
	} else {
		if ((strlen(sp->s_name) + 1) > servlen)
			ERR(ENI_MEMORY);
		strcpy(serv, sp->s_name);
	}

#if 0
	switch (sa->sa_family) {
	case AF_INET:
		v4a = ((struct sockaddr_in *)sa)->sin_addr.s_addr;
		if (IN_MULTICAST(v4a) || IN_EXPERIMENTAL(v4a))
			flags |= NI_NUMERICHOST;
		v4a >>= IN_CLASSA_NSHIFT;
		if (v4a == 0 || v4a == IN_LOOPBACKNET)
			flags |= NI_NUMERICHOST;
		break;

	case AF_INET6:
		pfx = ((struct sockaddr_in6 *)sa)->sin6_addr.s6_addr[0];
		if (pfx == 0 || pfx == 0xfe || pfx == 0xff)
			flags |= NI_NUMERICHOST;
		break;
	}
#endif

	if (host == NULL || hostlen == 0U) {
		/*
		 * What should we do?
		 */
	} else if (flags & NI_NUMERICHOST) {
		if (lwres_net_ntop(afd->a_af, addr, numaddr, sizeof(numaddr))
		    == NULL)
			ERR(ENI_SYSTEM);
#if defined(LWRES_HAVE_SIN6_SCOPE_ID)
		if (afd->a_af == AF_INET6 &&
		    ((const struct sockaddr_in6 *)sa)->sin6_scope_id) {
			char *p = numaddr + strlen(numaddr);
			const char *stringscope = NULL;
#if 0
			if ((flags & NI_NUMERICSCOPE) == 0) {
				/*
				 * Vendors may want to add support for
				 * non-numeric scope identifier.
				 */
				stringscope = foo;
			}
#endif
			if (stringscope == NULL) {
				snprintf(p, sizeof(numaddr) - (p - numaddr),
				    "%%%u",
				    ((const struct sockaddr_in6 *)sa)->sin6_scope_id);
			} else {
				snprintf(p, sizeof(numaddr) - (p - numaddr),
				    "%%%s", stringscope);
			}
		}
#endif
		if (strlen(numaddr) + 1 > hostlen)
			ERR(ENI_MEMORY);
		strcpy(host, numaddr);
	} else {
		switch (family) {
		case AF_INET:
			lwf = LWRES_ADDRTYPE_V4;
			break;
		case AF_INET6:
			lwf = LWRES_ADDRTYPE_V6;
			break;
		default:
			INSIST(0);
		}

		n = lwres_context_create(&lwrctx, NULL, NULL, NULL, 0);
		if (n == 0)
			(void) lwres_conf_parse(lwrctx, lwres_resolv_conf);

		if (n == 0)
			n = lwres_getnamebyaddr(lwrctx, lwf,
						(lwres_uint16_t)afd->a_addrlen,
						addr, &by);
		if (n == 0) {
			if (flags & NI_NOFQDN) {
				p = strchr(by->realname, '.');
				if (p)
					*p = '\0';
			}
			if ((strlen(by->realname) + 1) > hostlen)
				ERR(ENI_MEMORY);
			strcpy(host, by->realname);
		} else {
			if (flags & NI_NAMEREQD)
				ERR(ENI_NOHOSTNAME);
			if (lwres_net_ntop(afd->a_af, addr, numaddr,
					   sizeof(numaddr))
			    == NULL)
				ERR(ENI_NOHOSTNAME);
			if ((strlen(numaddr) + 1) > hostlen)
				ERR(ENI_MEMORY);
			strcpy(host, numaddr);
		}
	}
	result = SUCCESS;
 cleanup:
	if (by != NULL)
		lwres_gnbaresponse_free(lwrctx, &by);
	if (lwrctx != NULL) {
		lwres_conf_clear(lwrctx);
		lwres_context_destroy(&lwrctx);
	}
	return (result);
}
