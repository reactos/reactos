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

/* $Id: a_1.h,v 1.5 2007/06/19 23:47:17 tbox Exp $ */

/* by Bjorn.Victor@it.uu.se, 2005-05-07 */
/* Based on generic/mx_15.h */

#ifndef CH_3_A_1_H
#define CH_3_A_1_H 1

typedef isc_uint16_t ch_addr_t;

typedef struct dns_rdata_ch_a {
	dns_rdatacommon_t	common;
	isc_mem_t		*mctx;
  	dns_name_t		ch_addr_dom; /* ch-addr domain for back mapping */
	ch_addr_t		ch_addr; /* chaos address (16 bit) network order */
} dns_rdata_ch_a_t;

#endif /* CH_3_A_1_H */
