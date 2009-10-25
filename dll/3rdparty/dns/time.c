/*
 * Copyright (C) 2004, 2005, 2007, 2009  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1998-2003  Internet Software Consortium.
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

/* $Id: time.c,v 1.31.332.2 2009/01/18 23:47:40 tbox Exp $ */

/*! \file */

#include <config.h>

#include <stdio.h>
#include <isc/string.h>		/* Required for HP/UX (and others?) */
#include <time.h>

#include <isc/print.h>
#include <isc/region.h>
#include <isc/stdtime.h>
#include <isc/util.h>

#include <dns/result.h>
#include <dns/time.h>

static int days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

isc_result_t
dns_time64_totext(isc_int64_t t, isc_buffer_t *target) {
	struct tm tm;
	char buf[sizeof("YYYYMMDDHHMMSS")];
	int secs;
	unsigned int l;
	isc_region_t region;

	REQUIRE(t >= 0);

#define is_leap(y) ((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0)
#define year_secs(y) ((is_leap(y) ? 366 : 365 ) * 86400)
#define month_secs(m,y) ((days[m] + ((m == 1 && is_leap(y)) ? 1 : 0 )) * 86400)

	tm.tm_year = 70;
	while ((secs = year_secs(tm.tm_year + 1900)) <= t) {
		t -= secs;
		tm.tm_year++;
		if (tm.tm_year + 1900 > 9999)
			return (ISC_R_RANGE);
	}
	tm.tm_mon = 0;
	while ((secs = month_secs(tm.tm_mon, tm.tm_year + 1900)) <= t) {
		t -= secs;
		tm.tm_mon++;
	}
	tm.tm_mday = 1;
	while (86400 <= t) {
		t -= 86400;
		tm.tm_mday++;
	}
	tm.tm_hour = 0;
	while (3600 <= t) {
		t -= 3600;
		tm.tm_hour++;
	}
	tm.tm_min = 0;
	while (60 <= t) {
		t -= 60;
		tm.tm_min++;
	}
	tm.tm_sec = (int)t;
				 /* yyyy  mm  dd  HH  MM  SS */
	snprintf(buf, sizeof(buf), "%04d%02d%02d%02d%02d%02d",
		 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		 tm.tm_hour, tm.tm_min, tm.tm_sec);

	isc_buffer_availableregion(target, &region);
	l = strlen(buf);

	if (l > region.length)
		return (ISC_R_NOSPACE);

	memcpy(region.base, buf, l);
	isc_buffer_add(target, l);
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_time32_totext(isc_uint32_t value, isc_buffer_t *target) {
	isc_stdtime_t now;
	isc_int64_t start;
	isc_int64_t base;
	isc_int64_t t;

	/*
	 * Adjust the time to the closest epoch.  This should be changed
	 * to use a 64-bit counterpart to isc_stdtime_get() if one ever
	 * is defined, but even the current code is good until the year
	 * 2106.
	 */
	isc_stdtime_get(&now);
	start = (isc_int64_t) now;
	start -= 0x7fffffff;
	base = 0;
	while ((t = (base + value)) < start) {
		base += 0x80000000;
		base += 0x80000000;
	}
	return (dns_time64_totext(t, target));
}

isc_result_t
dns_time64_fromtext(const char *source, isc_int64_t *target) {
	int year, month, day, hour, minute, second;
	isc_int64_t value;
	int secs;
	int i;

#define RANGE(min, max, value) \
	do { \
		if (value < (min) || value > (max)) \
			return (ISC_R_RANGE); \
	} while (0)

	if (strlen(source) != 14U)
		return (DNS_R_SYNTAX);
	if (sscanf(source, "%4d%2d%2d%2d%2d%2d",
		   &year, &month, &day, &hour, &minute, &second) != 6)
		return (DNS_R_SYNTAX);

	RANGE(1970, 9999, year);
	RANGE(1, 12, month);
	RANGE(1, days[month - 1] +
		 ((month == 2 && is_leap(year)) ? 1 : 0), day);
	RANGE(0, 23, hour);
	RANGE(0, 59, minute);
	RANGE(0, 60, second);		/* 60 == leap second. */

	/*
	 * Calculate seconds since epoch.
	 */
	value = second + (60 * minute) + (3600 * hour) + ((day - 1) * 86400);
	for (i = 0; i < (month - 1); i++)
		value += days[i] * 86400;
	if (is_leap(year) && month > 2)
		value += 86400;
	for (i = 1970; i < year; i++) {
		secs = (is_leap(i) ? 366 : 365) * 86400;
		value += secs;
	}

	*target = value;
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_time32_fromtext(const char *source, isc_uint32_t *target) {
	isc_int64_t value64;
	isc_result_t result;
	result = dns_time64_fromtext(source, &value64);
	if (result != ISC_R_SUCCESS)
		return (result);
	*target = (isc_uint32_t)value64;

	return (ISC_R_SUCCESS);
}
