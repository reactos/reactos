/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
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

/* */
#ifndef IN_1_APL_42_H
#define IN_1_APL_42_H 1

/* $Id: apl_42.h,v 1.6 2007/06/19 23:47:17 tbox Exp $ */

typedef struct dns_rdata_apl_ent {
	isc_boolean_t	negative;
	isc_uint16_t	family;
	isc_uint8_t	prefix;
	isc_uint8_t	length;
	unsigned char	*data;
} dns_rdata_apl_ent_t;

typedef struct dns_rdata_in_apl {
	dns_rdatacommon_t	common;
	isc_mem_t		*mctx;
	/* type & class specific elements */
	unsigned char           *apl;
        isc_uint16_t            apl_len;
        /* private */
        isc_uint16_t            offset;
} dns_rdata_in_apl_t;

/*
 * ISC_LANG_BEGINDECLS and ISC_LANG_ENDDECLS are already done
 * via rdatastructpre.h and rdatastructsuf.h.
 */

isc_result_t
dns_rdata_apl_first(dns_rdata_in_apl_t *);

isc_result_t
dns_rdata_apl_next(dns_rdata_in_apl_t *);

isc_result_t
dns_rdata_apl_current(dns_rdata_in_apl_t *, dns_rdata_apl_ent_t *);

#endif /* IN_1_APL_42_H */
