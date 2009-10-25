/*
 * Copyright (C) 2004-2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2001  Internet Software Consortium.
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

/* $Id: key.c,v 1.8 2007/06/19 23:47:16 tbox Exp $ */

#include <config.h>

#include <stddef.h>
#include <stdlib.h>

#include <isc/region.h>
#include <isc/util.h>

#include <dns/keyvalues.h>

#include <dst/dst.h>

#include "dst_internal.h"

isc_uint16_t
dst_region_computeid(const isc_region_t *source, unsigned int alg) {
	isc_uint32_t ac;
	const unsigned char *p;
	int size;

	REQUIRE(source != NULL);
	REQUIRE(source->length >= 4);

	p = source->base;
	size = source->length;

	if (alg == DST_ALG_RSAMD5)
		return ((p[size - 3] << 8) + p[size - 2]);

	for (ac = 0; size > 1; size -= 2, p += 2)
		ac += ((*p) << 8) + *(p + 1);

	if (size > 0)
		ac += ((*p) << 8);
	ac += (ac >> 16) & 0xffff;

	return ((isc_uint16_t)(ac & 0xffff));
}

dns_name_t *
dst_key_name(const dst_key_t *key) {
	REQUIRE(VALID_KEY(key));
	return (key->key_name);
}

unsigned int
dst_key_size(const dst_key_t *key) {
	REQUIRE(VALID_KEY(key));
	return (key->key_size);
}

unsigned int
dst_key_proto(const dst_key_t *key) {
	REQUIRE(VALID_KEY(key));
	return (key->key_proto);
}

unsigned int
dst_key_alg(const dst_key_t *key) {
	REQUIRE(VALID_KEY(key));
	return (key->key_alg);
}

isc_uint32_t
dst_key_flags(const dst_key_t *key) {
	REQUIRE(VALID_KEY(key));
	return (key->key_flags);
}

dns_keytag_t
dst_key_id(const dst_key_t *key) {
	REQUIRE(VALID_KEY(key));
	return (key->key_id);
}

dns_rdataclass_t
dst_key_class(const dst_key_t *key) {
	REQUIRE(VALID_KEY(key));
	return (key->key_class);
}

isc_boolean_t
dst_key_iszonekey(const dst_key_t *key) {
	REQUIRE(VALID_KEY(key));

	if ((key->key_flags & DNS_KEYTYPE_NOAUTH) != 0)
		return (ISC_FALSE);
	if ((key->key_flags & DNS_KEYFLAG_OWNERMASK) != DNS_KEYOWNER_ZONE)
		return (ISC_FALSE);
	if (key->key_proto != DNS_KEYPROTO_DNSSEC &&
	    key->key_proto != DNS_KEYPROTO_ANY)
		return (ISC_FALSE);
	return (ISC_TRUE);
}

isc_boolean_t
dst_key_isnullkey(const dst_key_t *key) {
	REQUIRE(VALID_KEY(key));

	if ((key->key_flags & DNS_KEYFLAG_TYPEMASK) != DNS_KEYTYPE_NOKEY)
		return (ISC_FALSE);
	if ((key->key_flags & DNS_KEYFLAG_OWNERMASK) != DNS_KEYOWNER_ZONE)
		return (ISC_FALSE);
	if (key->key_proto != DNS_KEYPROTO_DNSSEC &&
	    key->key_proto != DNS_KEYPROTO_ANY)
		return (ISC_FALSE);
	return (ISC_TRUE);
}

void
dst_key_setbits(dst_key_t *key, isc_uint16_t bits) {
	unsigned int maxbits;
	REQUIRE(VALID_KEY(key));
	if (bits != 0) {
		RUNTIME_CHECK(dst_key_sigsize(key, &maxbits) == ISC_R_SUCCESS);
		maxbits *= 8;
		REQUIRE(bits <= maxbits);
	}
	key->key_bits = bits;
}

isc_uint16_t
dst_key_getbits(const dst_key_t *key) {
	REQUIRE(VALID_KEY(key));
	return (key->key_bits);
}

/*! \file */
