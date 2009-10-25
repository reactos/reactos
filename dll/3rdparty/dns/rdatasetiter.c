/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2001  Internet Software Consortium.
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

/* $Id: rdatasetiter.c,v 1.16 2007/06/19 23:47:16 tbox Exp $ */

/*! \file */

#include <config.h>

#include <stddef.h>

#include <isc/util.h>

#include <dns/rdataset.h>
#include <dns/rdatasetiter.h>

void
dns_rdatasetiter_destroy(dns_rdatasetiter_t **iteratorp) {
	/*
	 * Destroy '*iteratorp'.
	 */

	REQUIRE(iteratorp != NULL);
	REQUIRE(DNS_RDATASETITER_VALID(*iteratorp));

	(*iteratorp)->methods->destroy(iteratorp);

	ENSURE(*iteratorp == NULL);
}

isc_result_t
dns_rdatasetiter_first(dns_rdatasetiter_t *iterator) {
	/*
	 * Move the rdataset cursor to the first rdataset at the node (if any).
	 */

	REQUIRE(DNS_RDATASETITER_VALID(iterator));

	return (iterator->methods->first(iterator));
}

isc_result_t
dns_rdatasetiter_next(dns_rdatasetiter_t *iterator) {
	/*
	 * Move the rdataset cursor to the next rdataset at the node (if any).
	 */

	REQUIRE(DNS_RDATASETITER_VALID(iterator));

	return (iterator->methods->next(iterator));
}

void
dns_rdatasetiter_current(dns_rdatasetiter_t *iterator,
			 dns_rdataset_t *rdataset)
{
	/*
	 * Return the current rdataset.
	 */

	REQUIRE(DNS_RDATASETITER_VALID(iterator));
	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(! dns_rdataset_isassociated(rdataset));

	iterator->methods->current(iterator, rdataset);
}
