/*
 * Copyright (C) 2004-2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2000, 2001  Internet Software Consortium.
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

/* $Id: hmacmd5.h,v 1.12 2007/06/19 23:47:18 tbox Exp $ */

/*! \file isc/hmacmd5.h
 * \brief This is the header file for the HMAC-MD5 keyed hash algorithm
 * described in RFC2104.
 */

#ifndef ISC_HMACMD5_H
#define ISC_HMACMD5_H 1

#include <isc/lang.h>
#include <isc/md5.h>
#include <isc/types.h>

#define ISC_HMACMD5_KEYLENGTH 64

typedef struct {
	isc_md5_t md5ctx;
	unsigned char key[ISC_HMACMD5_KEYLENGTH];
} isc_hmacmd5_t;

ISC_LANG_BEGINDECLS

void
isc_hmacmd5_init(isc_hmacmd5_t *ctx, const unsigned char *key,
		 unsigned int len);

void
isc_hmacmd5_invalidate(isc_hmacmd5_t *ctx);

void
isc_hmacmd5_update(isc_hmacmd5_t *ctx, const unsigned char *buf,
		   unsigned int len);

void
isc_hmacmd5_sign(isc_hmacmd5_t *ctx, unsigned char *digest);

isc_boolean_t
isc_hmacmd5_verify(isc_hmacmd5_t *ctx, unsigned char *digest);

isc_boolean_t
isc_hmacmd5_verify2(isc_hmacmd5_t *ctx, unsigned char *digest, size_t len);

ISC_LANG_ENDDECLS

#endif /* ISC_HMACMD5_H */
