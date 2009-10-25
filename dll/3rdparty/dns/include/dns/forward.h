/*
 * Copyright (C) 2004-2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2000, 2001  Internet Software Consortium.
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

/* $Id: forward.h,v 1.11 2007/06/19 23:47:16 tbox Exp $ */

#ifndef DNS_FORWARD_H
#define DNS_FORWARD_H 1

/*! \file dns/forward.h */

#include <isc/lang.h>
#include <isc/result.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

struct dns_forwarders {
	isc_sockaddrlist_t	addrs;
	dns_fwdpolicy_t		fwdpolicy;
};

isc_result_t
dns_fwdtable_create(isc_mem_t *mctx, dns_fwdtable_t **fwdtablep);
/*%<
 * Creates a new forwarding table.
 *
 * Requires:
 * \li 	mctx is a valid memory context.
 * \li	fwdtablep != NULL && *fwdtablep == NULL
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOMEMORY
 */

isc_result_t
dns_fwdtable_add(dns_fwdtable_t *fwdtable, dns_name_t *name,
		 isc_sockaddrlist_t *addrs, dns_fwdpolicy_t policy);
/*%<
 * Adds an entry to the forwarding table.  The entry associates
 * a domain with a list of forwarders and a forwarding policy.  The
 * addrs list is copied if not empty, so the caller should free its copy.
 *
 * Requires:
 * \li	fwdtable is a valid forwarding table.
 * \li	name is a valid name
 * \li	addrs is a valid list of sockaddrs, which may be empty.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOMEMORY
 */

isc_result_t
dns_fwdtable_find(dns_fwdtable_t *fwdtable, dns_name_t *name,
		  dns_forwarders_t **forwardersp);
/*%<
 * Finds a domain in the forwarding table.  The closest matching parent
 * domain is returned.
 *
 * Requires:
 * \li	fwdtable is a valid forwarding table.
 * \li	name is a valid name
 * \li	forwardersp != NULL && *forwardersp == NULL
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOTFOUND
 */

isc_result_t
dns_fwdtable_find2(dns_fwdtable_t *fwdtable, dns_name_t *name,
		   dns_name_t *foundname, dns_forwarders_t **forwardersp);
/*%<
 * Finds a domain in the forwarding table.  The closest matching parent
 * domain is returned.
 *
 * Requires:
 * \li	fwdtable is a valid forwarding table.
 * \li	name is a valid name
 * \li	forwardersp != NULL && *forwardersp == NULL
 * \li	foundname to be NULL or a valid name with buffer.
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_NOTFOUND
 */

void
dns_fwdtable_destroy(dns_fwdtable_t **fwdtablep);
/*%<
 * Destroys a forwarding table.
 *
 * Requires:
 * \li	fwtablep != NULL && *fwtablep != NULL
 *
 * Ensures:
 * \li	all memory associated with the forwarding table is freed.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_FORWARD_H */
