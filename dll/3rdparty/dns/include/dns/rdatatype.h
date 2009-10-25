/*
 * Copyright (C) 2004-2008  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1998-2001  Internet Software Consortium.
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

/* $Id: rdatatype.h,v 1.26 2008/09/25 04:02:39 tbox Exp $ */

#ifndef DNS_RDATATYPE_H
#define DNS_RDATATYPE_H 1

/*! \file dns/rdatatype.h */

#include <isc/lang.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

isc_result_t
dns_rdatatype_fromtext(dns_rdatatype_t *typep, isc_textregion_t *source);
/*%<
 * Convert the text 'source' refers to into a DNS rdata type.
 *
 * Requires:
 *\li	'typep' is a valid pointer.
 *
 *\li	'source' is a valid text region.
 *
 * Returns:
 *\li	ISC_R_SUCCESS			on success
 *\li	DNS_R_UNKNOWN			type is unknown
 */

isc_result_t
dns_rdatatype_totext(dns_rdatatype_t type, isc_buffer_t *target);
/*%<
 * Put a textual representation of type 'type' into 'target'.
 *
 * Requires:
 *\li	'type' is a valid type.
 *
 *\li	'target' is a valid text buffer.
 *
 * Ensures,
 *	if the result is success:
 *\li		The used space in 'target' is updated.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS			on success
 *\li	#ISC_R_NOSPACE			target buffer is too small
 */

void
dns_rdatatype_format(dns_rdatatype_t rdtype,
		     char *array, unsigned int size);
/*%<
 * Format a human-readable representation of the type 'rdtype'
 * into the character array 'array', which is of size 'size'.
 * The resulting string is guaranteed to be null-terminated.
 */

#define DNS_RDATATYPE_FORMATSIZE sizeof("NSEC3PARAM")

/*%<
 * Minimum size of array to pass to dns_rdatatype_format().
 * May need to be adjusted if a new RR type with a very long
 * name is defined.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_RDATATYPE_H */
