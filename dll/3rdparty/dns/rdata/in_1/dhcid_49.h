/*
 * Copyright (C) 2006, 2007  Internet Systems Consortium, Inc. ("ISC")
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

/* */
#ifndef IN_1_DHCID_49_H
#define IN_1_DHCID_49_H 1

/* $Id: dhcid_49.h,v 1.5 2007/06/19 23:47:17 tbox Exp $ */

typedef struct dns_rdata_in_dhcid {
	dns_rdatacommon_t	common;
	isc_mem_t		*mctx;
	unsigned char		*dhcid;
	unsigned int		length;
} dns_rdata_in_dhcid_t;

#endif /* IN_1_DHCID_49_H */
