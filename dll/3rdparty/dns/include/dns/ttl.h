/*
 * Copyright (C) 2004-2007  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: ttl.h,v 1.19 2007/06/19 23:47:17 tbox Exp $ */

#ifndef DNS_TTL_H
#define DNS_TTL_H 1

/*! \file dns/ttl.h */

/***
 ***	Imports
 ***/

#include <isc/lang.h>
#include <isc/types.h>

ISC_LANG_BEGINDECLS

/***
 ***	Functions
 ***/

isc_result_t
dns_ttl_totext(isc_uint32_t src, isc_boolean_t verbose,
	       isc_buffer_t *target);
/*%<
 * Output a TTL or other time interval in a human-readable form.
 * The time interval is given as a count of seconds in 'src'.
 * The text representation is appended to 'target'.
 *
 * If 'verbose' is ISC_FALSE, use the terse BIND 8 style, like "1w2d3h4m5s".
 *
 * If 'verbose' is ISC_TRUE, use a verbose style like the SOA comments
 * in "dig", like "1 week 2 days 3 hours 4 minutes 5 seconds".
 *
 * Returns:
 * \li	ISC_R_SUCCESS
 * \li	ISC_R_NOSPACE
 */

isc_result_t
dns_counter_fromtext(isc_textregion_t *source, isc_uint32_t *ttl);
/*%<
 * Converts a counter from either a plain number or a BIND 8 style value.
 *
 * Returns:
 *\li	ISC_R_SUCCESS
 *\li	DNS_R_SYNTAX
 */

isc_result_t
dns_ttl_fromtext(isc_textregion_t *source, isc_uint32_t *ttl);
/*%<
 * Converts a ttl from either a plain number or a BIND 8 style value.
 *
 * Returns:
 *\li	ISC_R_SUCCESS
 *\li	DNS_R_BADTTL
 */

ISC_LANG_ENDDECLS

#endif /* DNS_TTL_H */
