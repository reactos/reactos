/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2003  Internet Software Consortium.
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

#ifndef GENERIC_DNSSIG_46_H
#define GENERIC_DNSSIG_46_H 1

/* $Id: rrsig_46.h,v 1.7 2007/06/19 23:47:17 tbox Exp $ */

/*!
 *  \brief Per RFC2535 */
typedef struct dns_rdata_rrsig {
	dns_rdatacommon_t	common;
	isc_mem_t *		mctx;
	dns_rdatatype_t		covered;
	dns_secalg_t		algorithm;
	isc_uint8_t		labels;
	isc_uint32_t		originalttl;
	isc_uint32_t		timeexpire;
	isc_uint32_t		timesigned;
	isc_uint16_t		keyid;
        dns_name_t		signer;
	isc_uint16_t		siglen;
	unsigned char *		signature;
} dns_rdata_rrsig_t;


#endif /* GENERIC_DNSSIG_46_H */
