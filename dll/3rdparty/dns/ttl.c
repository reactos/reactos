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

/* $Id: ttl.c,v 1.29 2007/06/19 23:47:16 tbox Exp $ */

/*! \file */

#include <config.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <isc/buffer.h>
#include <isc/parseint.h>
#include <isc/print.h>
#include <isc/region.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/result.h>
#include <dns/ttl.h>

#define RETERR(x) do { \
	isc_result_t _r = (x); \
	if (_r != ISC_R_SUCCESS) \
		return (_r); \
	} while (0)


static isc_result_t bind_ttl(isc_textregion_t *source, isc_uint32_t *ttl);

/*
 * Helper for dns_ttl_totext().
 */
static isc_result_t
ttlfmt(unsigned int t, const char *s, isc_boolean_t verbose,
       isc_boolean_t space, isc_buffer_t *target)
{
	char tmp[60];
	size_t len;
	isc_region_t region;

	if (verbose)
		len = snprintf(tmp, sizeof(tmp), "%s%u %s%s",
			       space ? " " : "",
			       t, s,
			       t == 1 ? "" : "s");
	else
		len = snprintf(tmp, sizeof(tmp), "%u%c", t, s[0]);

	INSIST(len + 1 <= sizeof(tmp));
	isc_buffer_availableregion(target, &region);
	if (len > region.length)
		return (ISC_R_NOSPACE);
	memcpy(region.base, tmp, len);
	isc_buffer_add(target, len);

	return (ISC_R_SUCCESS);
}

/*
 * Derived from bind8 ns_format_ttl().
 */
isc_result_t
dns_ttl_totext(isc_uint32_t src, isc_boolean_t verbose, isc_buffer_t *target) {
	unsigned secs, mins, hours, days, weeks, x;

	secs = src % 60;   src /= 60;
	mins = src % 60;   src /= 60;
	hours = src % 24;  src /= 24;
	days = src % 7;    src /= 7;
	weeks = src;       src = 0;

	x = 0;
	if (weeks != 0) {
		RETERR(ttlfmt(weeks, "week", verbose, ISC_TF(x > 0), target));
		x++;
	}
	if (days != 0) {
		RETERR(ttlfmt(days, "day", verbose, ISC_TF(x > 0), target));
		x++;
	}
	if (hours != 0) {
		RETERR(ttlfmt(hours, "hour", verbose, ISC_TF(x > 0), target));
		x++;
	}
	if (mins != 0) {
		RETERR(ttlfmt(mins, "minute", verbose, ISC_TF(x > 0), target));
		x++;
	}
	if (secs != 0 ||
	    (weeks == 0 && days == 0 && hours == 0 && mins == 0)) {
		RETERR(ttlfmt(secs, "second", verbose, ISC_TF(x > 0), target));
		x++;
	}
	INSIST (x > 0);
	/*
	 * If only a single unit letter is printed, print it
	 * in upper case. (Why?  Because BIND 8 does that.
	 * Presumably it has a reason.)
	 */
	if (x == 1 && !verbose) {
		isc_region_t region;
		/*
		 * The unit letter is the last character in the
		 * used region of the buffer.
		 *
		 * toupper() does not need its argument to be masked of cast
		 * here because region.base is type unsigned char *.
		 */
		isc_buffer_usedregion(target, &region);
		region.base[region.length - 1] =
			toupper(region.base[region.length - 1]);
	}
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_counter_fromtext(isc_textregion_t *source, isc_uint32_t *ttl) {
	return (bind_ttl(source, ttl));
}

isc_result_t
dns_ttl_fromtext(isc_textregion_t *source, isc_uint32_t *ttl) {
	isc_result_t result;

	result = bind_ttl(source, ttl);
	if (result != ISC_R_SUCCESS)
		result = DNS_R_BADTTL;
	return (result);
}

static isc_result_t
bind_ttl(isc_textregion_t *source, isc_uint32_t *ttl) {
	isc_uint32_t tmp = 0;
	isc_uint32_t n;
	char *s;
	char buf[64];
	char nbuf[64]; /* Number buffer */

	/*
	 * Copy the buffer as it may not be NULL terminated.
	 * No legal counter / ttl is longer that 63 characters.
	 */
	if (source->length > sizeof(buf) - 1)
		return (DNS_R_SYNTAX);
	strncpy(buf, source->base, source->length);
	buf[source->length] = '\0';
	s = buf;

	do {
		isc_result_t result;

		char *np = nbuf;
		while (*s != '\0' && isdigit((unsigned char)*s))
			*np++ = *s++;
		*np++ = '\0';
		INSIST(np - nbuf <= (int)sizeof(nbuf));
		result = isc_parse_uint32(&n, nbuf, 10);
		if (result != ISC_R_SUCCESS)
			return (DNS_R_SYNTAX);
		switch (*s) {
		case 'w':
		case 'W':
			tmp += n * 7 * 24 * 3600;
			s++;
			break;
		case 'd':
		case 'D':
			tmp += n * 24 * 3600;
			s++;
			break;
		case 'h':
		case 'H':
			tmp += n * 3600;
			s++;
			break;
		case 'm':
		case 'M':
			tmp += n * 60;
			s++;
			break;
		case 's':
		case 'S':
			tmp += n;
			s++;
			break;
		case '\0':
			/* Plain number? */
			if (tmp != 0)
				return (DNS_R_SYNTAX);
			tmp = n;
			break;
		default:
			return (DNS_R_SYNTAX);
		}
	} while (*s != '\0');
	*ttl = tmp;
	return (ISC_R_SUCCESS);
}
