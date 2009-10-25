/*
 * Copyright (C) 2008  Internet Systems Consortium, Inc. ("ISC")
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


#ifndef GENERIC_NSEC3PARAM_51_H
#define GENERIC_NSEC3PARAM_51_H 1

/* $Id: nsec3param_51.h,v 1.4 2008/09/25 04:02:39 tbox Exp $ */

/*!
 * \brief Per RFC 5155 */

#include <isc/iterated_hash.h>

typedef struct dns_rdata_nsec3param {
	dns_rdatacommon_t	common;
	isc_mem_t		*mctx;
	dns_hash_t		hash;
	unsigned char		flags;		/* DNS_NSEC3FLAG_* */
	dns_iterations_t	iterations;
	unsigned char		salt_length;
	unsigned char		*salt;
} dns_rdata_nsec3param_t;

#endif /* GENERIC_NSEC3PARAM_51_H */
