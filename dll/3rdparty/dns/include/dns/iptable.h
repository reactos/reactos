/*
 * Copyright (C) 2007  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: iptable.h,v 1.4 2007/09/14 01:46:05 marka Exp $ */

#ifndef DNS_IPTABLE_H
#define DNS_IPTABLE_H 1

#include <isc/lang.h>
#include <isc/magic.h>
#include <isc/radix.h>

struct dns_iptable {
	unsigned int		magic;
	isc_mem_t		*mctx;
	isc_refcount_t		refcount;
	isc_radix_tree_t	*radix;
	ISC_LINK(dns_iptable_t)	nextincache;
};

#define DNS_IPTABLE_MAGIC	ISC_MAGIC('T','a','b','l')
#define DNS_IPTABLE_VALID(a)	ISC_MAGIC_VALID(a, DNS_IPTABLE_MAGIC)

/***
 *** Functions
 ***/

ISC_LANG_BEGINDECLS

isc_result_t
dns_iptable_create(isc_mem_t *mctx, dns_iptable_t **target);
/*
 * Create a new IP table and the underlying radix structure
 */

isc_result_t
dns_iptable_addprefix(dns_iptable_t *tab, isc_netaddr_t *addr,
		      isc_uint16_t bitlen, isc_boolean_t pos);
/*
 * Add an IP prefix to an existing IP table
 */

isc_result_t
dns_iptable_merge(dns_iptable_t *tab, dns_iptable_t *source, isc_boolean_t pos);
/*
 * Merge one IP table into another one.
 */

void
dns_iptable_attach(dns_iptable_t *source, dns_iptable_t **target);

void
dns_iptable_detach(dns_iptable_t **tabp);

ISC_LANG_ENDDECLS

#endif /* DNS_IPTABLE_H */
