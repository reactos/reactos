/*
 * Copyright (C) 2004-2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2003  Internet Software Consortium.
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

/* $Id: portlist.h,v 1.9 2007/06/19 23:47:17 tbox Exp $ */

/*! \file dns/portlist.h */

#include <isc/lang.h>
#include <isc/net.h>
#include <isc/types.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

isc_result_t
dns_portlist_create(isc_mem_t *mctx, dns_portlist_t **portlistp);
/*%<
 * Create a port list.
 * 
 * Requires:
 *\li	'mctx' to be valid.
 *\li	'portlistp' to be non NULL and '*portlistp' to be NULL;
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 *\li	#ISC_R_UNEXPECTED
 */

isc_result_t
dns_portlist_add(dns_portlist_t *portlist, int af, in_port_t port);
/*%<
 * Add the given <port,af> tuple to the portlist.
 *
 * Requires:
 *\li	'portlist' to be valid.
 *\li	'af' to be AF_INET or AF_INET6
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 */

void
dns_portlist_remove(dns_portlist_t *portlist, int af, in_port_t port);
/*%<
 * Remove the given <port,af> tuple to the portlist.
 *
 * Requires:
 *\li	'portlist' to be valid.
 *\li	'af' to be AF_INET or AF_INET6
 */

isc_boolean_t
dns_portlist_match(dns_portlist_t *portlist, int af, in_port_t port);
/*%<
 * Find the given <port,af> tuple to the portlist.
 *
 * Requires:
 *\li	'portlist' to be valid.
 *\li	'af' to be AF_INET or AF_INET6
 *
 * Returns
 * \li	#ISC_TRUE if the tuple is found, ISC_FALSE otherwise.
 */

void
dns_portlist_attach(dns_portlist_t *portlist, dns_portlist_t **portlistp);
/*%<
 * Attach to a port list.
 *
 * Requires:
 *\li	'portlist' to be valid.
 *\li	'portlistp' to be non NULL and '*portlistp' to be NULL;
 */

void
dns_portlist_detach(dns_portlist_t **portlistp);
/*%<
 * Detach from a port list.
 *
 * Requires:
 *\li	'*portlistp' to be valid.
 */

ISC_LANG_ENDDECLS
