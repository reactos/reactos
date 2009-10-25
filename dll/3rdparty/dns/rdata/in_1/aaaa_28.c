/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2002  Internet Software Consortium.
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

/* $Id: aaaa_28.c,v 1.45 2007/06/19 23:47:17 tbox Exp $ */

/* Reviewed: Thu Mar 16 16:52:50 PST 2000 by bwelling */

/* RFC1886 */

#ifndef RDATA_IN_1_AAAA_28_C
#define RDATA_IN_1_AAAA_28_C

#include <isc/net.h>

#define RRTYPE_AAAA_ATTRIBUTES (0)

static inline isc_result_t
fromtext_in_aaaa(ARGS_FROMTEXT) {
	isc_token_t token;
	unsigned char addr[16];
	isc_region_t region;

	REQUIRE(type == 28);
	REQUIRE(rdclass == 1);

	UNUSED(type);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(rdclass);
	UNUSED(callbacks);

	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));

	if (inet_pton(AF_INET6, DNS_AS_STR(token), addr) != 1)
		RETTOK(DNS_R_BADAAAA);
	isc_buffer_availableregion(target, &region);
	if (region.length < 16)
		return (ISC_R_NOSPACE);
	memcpy(region.base, addr, 16);
	isc_buffer_add(target, 16);
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
totext_in_aaaa(ARGS_TOTEXT) {
	isc_region_t region;

	UNUSED(tctx);

	REQUIRE(rdata->type == 28);
	REQUIRE(rdata->rdclass == 1);
	REQUIRE(rdata->length == 16);

	dns_rdata_toregion(rdata, &region);
	return (inet_totext(AF_INET6, &region, target));
}

static inline isc_result_t
fromwire_in_aaaa(ARGS_FROMWIRE) {
	isc_region_t sregion;
	isc_region_t tregion;

	REQUIRE(type == 28);
	REQUIRE(rdclass == 1);

	UNUSED(type);
	UNUSED(dctx);
	UNUSED(options);
	UNUSED(rdclass);

	isc_buffer_activeregion(source, &sregion);
	isc_buffer_availableregion(target, &tregion);
	if (sregion.length < 16)
		return (ISC_R_UNEXPECTEDEND);
	if (tregion.length < 16)
		return (ISC_R_NOSPACE);

	memcpy(tregion.base, sregion.base, 16);
	isc_buffer_forward(source, 16);
	isc_buffer_add(target, 16);
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
towire_in_aaaa(ARGS_TOWIRE) {
	isc_region_t region;

	UNUSED(cctx);

	REQUIRE(rdata->type == 28);
	REQUIRE(rdata->rdclass == 1);
	REQUIRE(rdata->length == 16);

	isc_buffer_availableregion(target, &region);
	if (region.length < rdata->length)
		return (ISC_R_NOSPACE);
	memcpy(region.base, rdata->data, rdata->length);
	isc_buffer_add(target, 16);
	return (ISC_R_SUCCESS);
}

static inline int
compare_in_aaaa(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 28);
	REQUIRE(rdata1->rdclass == 1);
	REQUIRE(rdata1->length == 16);
	REQUIRE(rdata2->length == 16);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_in_aaaa(ARGS_FROMSTRUCT) {
	dns_rdata_in_aaaa_t *aaaa = source;

	REQUIRE(type == 28);
	REQUIRE(rdclass == 1);
	REQUIRE(source != NULL);
	REQUIRE(aaaa->common.rdtype == type);
	REQUIRE(aaaa->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	return (mem_tobuffer(target, aaaa->in6_addr.s6_addr, 16));
}

static inline isc_result_t
tostruct_in_aaaa(ARGS_TOSTRUCT) {
	dns_rdata_in_aaaa_t *aaaa = target;
	isc_region_t r;

	REQUIRE(rdata->type == 28);
	REQUIRE(rdata->rdclass == 1);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length == 16);

	UNUSED(mctx);

	aaaa->common.rdclass = rdata->rdclass;
	aaaa->common.rdtype = rdata->type;
	ISC_LINK_INIT(&aaaa->common, link);

	dns_rdata_toregion(rdata, &r);
	INSIST(r.length == 16);
	memcpy(aaaa->in6_addr.s6_addr, r.base, 16);

	return (ISC_R_SUCCESS);
}

static inline void
freestruct_in_aaaa(ARGS_FREESTRUCT) {
	dns_rdata_in_aaaa_t *aaaa = source;

	REQUIRE(source != NULL);
	REQUIRE(aaaa->common.rdclass == 1);
	REQUIRE(aaaa->common.rdtype == 28);

	UNUSED(aaaa);
}

static inline isc_result_t
additionaldata_in_aaaa(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == 28);
	REQUIRE(rdata->rdclass == 1);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_in_aaaa(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == 28);
	REQUIRE(rdata->rdclass == 1);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_in_aaaa(ARGS_CHECKOWNER) {

	REQUIRE(type == 28);
	REQUIRE(rdclass == 1);

	UNUSED(type);
	UNUSED(rdclass);

	return (dns_name_ishostname(name, wildcard));
}

static inline isc_boolean_t
checknames_in_aaaa(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == 28);
	REQUIRE(rdata->rdclass == 1);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

#endif	/* RDATA_IN_1_AAAA_28_C */
