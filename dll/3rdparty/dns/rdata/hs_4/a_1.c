/*
 * Copyright (C) 2004, 2007  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: a_1.c,v 1.31 2007/06/19 23:47:17 tbox Exp $ */

/* reviewed: Thu Mar 16 15:58:36 PST 2000 by brister */

#ifndef RDATA_HS_4_A_1_C
#define RDATA_HS_4_A_1_C

#include <isc/net.h>

#define RRTYPE_A_ATTRIBUTES (0)

static inline isc_result_t
fromtext_hs_a(ARGS_FROMTEXT) {
	isc_token_t token;
	struct in_addr addr;
	isc_region_t region;

	REQUIRE(type == 1);
	REQUIRE(rdclass == 4);

	UNUSED(type);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(rdclass);

	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));

	if (getquad(DNS_AS_STR(token), &addr, lexer, callbacks) != 1)
		RETTOK(DNS_R_BADDOTTEDQUAD);
	isc_buffer_availableregion(target, &region);
	if (region.length < 4)
		return (ISC_R_NOSPACE);
	memcpy(region.base, &addr, 4);
	isc_buffer_add(target, 4);
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
totext_hs_a(ARGS_TOTEXT) {
	isc_region_t region;

	REQUIRE(rdata->type == 1);
	REQUIRE(rdata->rdclass == 4);
	REQUIRE(rdata->length == 4);

	UNUSED(tctx);

	dns_rdata_toregion(rdata, &region);
	return (inet_totext(AF_INET, &region, target));
}

static inline isc_result_t
fromwire_hs_a(ARGS_FROMWIRE) {
	isc_region_t sregion;
	isc_region_t tregion;

	REQUIRE(type == 1);
	REQUIRE(rdclass == 4);

	UNUSED(type);
	UNUSED(dctx);
	UNUSED(options);
	UNUSED(rdclass);

	isc_buffer_activeregion(source, &sregion);
	isc_buffer_availableregion(target, &tregion);
	if (sregion.length < 4)
		return (ISC_R_UNEXPECTEDEND);
	if (tregion.length < 4)
		return (ISC_R_NOSPACE);

	memcpy(tregion.base, sregion.base, 4);
	isc_buffer_forward(source, 4);
	isc_buffer_add(target, 4);
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
towire_hs_a(ARGS_TOWIRE) {
	isc_region_t region;

	REQUIRE(rdata->type == 1);
	REQUIRE(rdata->rdclass == 4);
	REQUIRE(rdata->length == 4);

	UNUSED(cctx);

	isc_buffer_availableregion(target, &region);
	if (region.length < rdata->length)
		return (ISC_R_NOSPACE);
	memcpy(region.base, rdata->data, rdata->length);
	isc_buffer_add(target, 4);
	return (ISC_R_SUCCESS);
}

static inline int
compare_hs_a(ARGS_COMPARE) {
	int order;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 1);
	REQUIRE(rdata1->rdclass == 4);
	REQUIRE(rdata1->length == 4);
	REQUIRE(rdata2->length == 4);

	order = memcmp(rdata1->data, rdata2->data, 4);
	if (order != 0)
		order = (order < 0) ? -1 : 1;

	return (order);
}

static inline isc_result_t
fromstruct_hs_a(ARGS_FROMSTRUCT) {
	dns_rdata_hs_a_t *a = source;
	isc_uint32_t n;

	REQUIRE(type == 1);
	REQUIRE(rdclass == 4);
	REQUIRE(source != NULL);
	REQUIRE(a->common.rdtype == type);
	REQUIRE(a->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	n = ntohl(a->in_addr.s_addr);

	return (uint32_tobuffer(n, target));
}

static inline isc_result_t
tostruct_hs_a(ARGS_TOSTRUCT) {
	dns_rdata_hs_a_t *a = target;
	isc_uint32_t n;
	isc_region_t region;

	REQUIRE(rdata->type == 1);
	REQUIRE(rdata->rdclass == 4);
	REQUIRE(rdata->length == 4);

	UNUSED(mctx);

	a->common.rdclass = rdata->rdclass;
	a->common.rdtype = rdata->type;
	ISC_LINK_INIT(&a->common, link);

	dns_rdata_toregion(rdata, &region);
	n = uint32_fromregion(&region);
	a->in_addr.s_addr = htonl(n);

	return (ISC_R_SUCCESS);
}

static inline void
freestruct_hs_a(ARGS_FREESTRUCT) {
	UNUSED(source);

	REQUIRE(source != NULL);
}

static inline isc_result_t
additionaldata_hs_a(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == 1);
	REQUIRE(rdata->rdclass == 4);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_hs_a(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == 1);
	REQUIRE(rdata->rdclass == 4);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_hs_a(ARGS_CHECKOWNER) {

	REQUIRE(type == 1);
	REQUIRE(rdclass == 4);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_hs_a(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == 1);
	REQUIRE(rdata->rdclass == 4);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

#endif	/* RDATA_HS_4_A_1_C */
