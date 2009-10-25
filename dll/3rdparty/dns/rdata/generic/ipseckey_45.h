/*
 * Copyright (C) 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: ipseckey_45.h,v 1.4 2007/06/19 23:47:17 tbox Exp $ */

#ifndef GENERIC_IPSECKEY_45_H
#define GENERIC_IPSECKEY_45_H 1

typedef struct dns_rdata_ipseckey {
	dns_rdatacommon_t	common;
	isc_mem_t		*mctx;
	isc_uint8_t		precedence;
	isc_uint8_t		gateway_type;
	isc_uint8_t		algorithm;
	struct in_addr		in_addr;	/* gateway type 1 */
	struct in6_addr		in6_addr;	/* gateway type 2 */
	dns_name_t		gateway;	/* gateway type 3 */
	unsigned char		*key;
	isc_uint16_t		keylength;
} dns_rdata_ipseckey_t;

#endif /* GENERIC_IPSECKEY_45_H */
