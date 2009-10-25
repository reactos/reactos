/*
 * Copyright (C) 2004-2007  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: sshfp_44.h,v 1.8 2007/06/19 23:47:17 tbox Exp $ */

/*!
 *  \brief Per RFC 4255 */

#ifndef GENERIC_SSHFP_44_H
#define GENERIC_SSHFP_44_H 1

typedef struct dns_rdata_sshfp {
	dns_rdatacommon_t	common;
	isc_mem_t		*mctx;
	isc_uint8_t		algorithm;
	isc_uint8_t		digest_type;
	isc_uint16_t		length;
	unsigned char		*digest;
} dns_rdata_sshfp_t;

#endif /* GENERIC_SSHFP_44_H */
