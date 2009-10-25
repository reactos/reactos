/*
 * Copyright (C) 2004-2008  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: rcode.h,v 1.21 2008/09/25 04:02:39 tbox Exp $ */

#ifndef DNS_RCODE_H
#define DNS_RCODE_H 1

/*! \file dns/rcode.h */

#include <isc/lang.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

isc_result_t dns_rcode_fromtext(dns_rcode_t *rcodep, isc_textregion_t *source);
/*%<
 * Convert the text 'source' refers to into a DNS error value.
 *
 * Requires:
 *\li	'rcodep' is a valid pointer.
 *
 *\li	'source' is a valid text region.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS			on success
 *\li	#DNS_R_UNKNOWN			type is unknown
 */

isc_result_t dns_rcode_totext(dns_rcode_t rcode, isc_buffer_t *target);
/*%<
 * Put a textual representation of error 'rcode' into 'target'.
 *
 * Requires:
 *\li	'rcode' is a valid rcode.
 *
 *\li	'target' is a valid text buffer.
 *
 * Ensures:
 *\li	If the result is success:
 *		The used space in 'target' is updated.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS			on success
 *\li	#ISC_R_NOSPACE			target buffer is too small
 */

isc_result_t dns_tsigrcode_fromtext(dns_rcode_t *rcodep,
				    isc_textregion_t *source);
/*%<
 * Convert the text 'source' refers to into a TSIG/TKEY error value.
 *
 * Requires:
 *\li	'rcodep' is a valid pointer.
 *
 *\li	'source' is a valid text region.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS			on success
 *\li	#DNS_R_UNKNOWN			type is unknown
 */

isc_result_t dns_tsigrcode_totext(dns_rcode_t rcode, isc_buffer_t *target);
/*%<
 * Put a textual representation of TSIG/TKEY error 'rcode' into 'target'.
 *
 * Requires:
 *\li	'rcode' is a valid TSIG/TKEY error code.
 *
 *\li	'target' is a valid text buffer.
 *
 * Ensures:
 *\li	If the result is success:
 *		The used space in 'target' is updated.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS			on success
 *\li	#ISC_R_NOSPACE			target buffer is too small
 */

isc_result_t
dns_hashalg_fromtext(unsigned char *hashalg, isc_textregion_t *source);
/*%<
 * Convert the text 'source' refers to into a has algorithm value.
 *
 * Requires:
 *\li	'hashalg' is a valid pointer.
 *
 *\li	'source' is a valid text region.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS			on success
 *\li	#DNS_R_UNKNOWN			type is unknown
 */

ISC_LANG_ENDDECLS

#endif /* DNS_RCODE_H */
