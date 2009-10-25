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

/* $Id: time.h,v 1.17 2007/06/19 23:47:17 tbox Exp $ */

#ifndef DNS_TIME_H
#define DNS_TIME_H 1

/*! \file dns/time.h */

/***
 ***	Imports
 ***/

#include <isc/buffer.h>
#include <isc/lang.h>

ISC_LANG_BEGINDECLS

/***
 ***	Functions
 ***/

isc_result_t
dns_time64_fromtext(const char *source, isc_int64_t *target);
/*%<
 * Convert a date and time in YYYYMMDDHHMMSS text format at 'source'
 * into to a 64-bit count of seconds since Jan 1 1970 0:00 GMT.
 * Store the count at 'target'.
 */

isc_result_t
dns_time32_fromtext(const char *source, isc_uint32_t *target);
/*%<
 * Like dns_time64_fromtext, but returns the second count modulo 2^32
 * as per RFC2535.
 */


isc_result_t
dns_time64_totext(isc_int64_t value, isc_buffer_t *target);
/*%<
 * Convert a 64-bit count of seconds since Jan 1 1970 0:00 GMT into
 * a YYYYMMDDHHMMSS text representation and append it to 'target'.
 */

isc_result_t
dns_time32_totext(isc_uint32_t value, isc_buffer_t *target);
/*%<
 * Like dns_time64_totext, but for a 32-bit cyclic time value.
 * Of those dates whose counts of seconds since Jan 1 1970 0:00 GMT
 * are congruent with 'value' modulo 2^32, the one closest to the
 * current date is chosen.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_TIME_H */
