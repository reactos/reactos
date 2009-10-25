/*
 * Copyright (C) 2004-2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2002  Internet Software Consortium.
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

/* $Id: order.h,v 1.9 2007/06/19 23:47:17 tbox Exp $ */

#ifndef DNS_ORDER_H
#define DNS_ORDER_H 1

/*! \file dns/order.h */

#include <isc/lang.h>
#include <isc/types.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

isc_result_t
dns_order_create(isc_mem_t *mctx, dns_order_t **orderp);
/*%<
 * Create a order object.
 *
 * Requires:
 * \li	'orderp' to be non NULL and '*orderp == NULL'.
 *\li	'mctx' to be valid.
 *
 * Returns:
 *\li	ISC_R_SUCCESS
 *\li	ISC_R_NOMEMORY
 */

isc_result_t
dns_order_add(dns_order_t *order, dns_name_t *name,
	      dns_rdatatype_t rdtype, dns_rdataclass_t rdclass,
	      unsigned int mode);
/*%<
 * Add a entry to the end of the order list.
 *
 * Requires:
 * \li	'order' to be valid.
 *\li	'name' to be valid.
 *\li	'mode' to be one of #DNS_RDATASERATTR_RANDOMIZE,
 *		#DNS_RDATASERATTR_RANDOMIZE or zero (#DNS_RDATASERATTR_CYCLIC).
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 */

unsigned int
dns_order_find(dns_order_t *order, dns_name_t *name,
	       dns_rdatatype_t rdtype, dns_rdataclass_t rdclass);
/*%<
 * Find the first matching entry on the list.
 *
 * Requires:
 *\li	'order' to be valid.
 *\li	'name' to be valid.
 *
 * Returns the mode set by dns_order_add() or zero.
 */

void
dns_order_attach(dns_order_t *source, dns_order_t **target);
/*%<
 * Attach to the 'source' object.
 *
 * Requires:
 * \li	'source' to be valid.
 *\li	'target' to be non NULL and '*target == NULL'.
 */

void
dns_order_detach(dns_order_t **orderp);
/*%<
 * Detach from the object.  Clean up if last this was the last
 * reference.
 *
 * Requires:
 *\li	'*orderp' to be valid.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_ORDER_H */
