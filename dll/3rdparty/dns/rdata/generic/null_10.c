/*
 * Copyright (C) 2004, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1998-2002  Internet Software Consortium.
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

/* $Id: null_10.c,v 1.42 2007/06/19 23:47:17 tbox Exp $ */

/* Reviewed: Thu Mar 16 13:57:50 PST 2000 by explorer */

#ifndef RDATA_GENERIC_NULL_10_C
#define RDATA_GENERIC_NULL_10_C

#define RRTYPE_NULL_ATTRIBUTES (0)

static inline isc_result_t
fromtext_null(ARGS_FROMTEXT) {
	REQUIRE(type == 10);

	UNUSED(rdclass);
	UNUSED(type);
	UNUSED(lexer);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(target);
	UNUSED(callbacks);

	return (DNS_R_SYNTAX);
}

static inline isc_result_t
totext_null(ARGS_TOTEXT) {
	REQUIRE(rdata->type == 10);

	UNUSED(rdata);
	UNUSED(tctx);
	UNUSED(target);

	return (DNS_R_SYNTAX);
}

static inline isc_result_t
fromwire_null(ARGS_FROMWIRE) {
	isc_region_t sr;

	REQUIRE(type == 10);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(dctx);
	UNUSED(options);

	isc_buffer_activeregion(source, &sr);
	isc_buffer_forward(source, sr.length);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static inline isc_result_t
towire_null(ARGS_TOWIRE) {
	REQUIRE(rdata->type == 10);

	UNUSED(cctx);

	return (mem_tobuffer(target, rdata->data, rdata->length));
}

static inline int
compare_null(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 10);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_null(ARGS_FROMSTRUCT) {
	dns_rdata_null_t *null = source;

	REQUIRE(type == 10);
	REQUIRE(source != NULL);
	REQUIRE(null->common.rdtype == type);
	REQUIRE(null->common.rdclass == rdclass);
	REQUIRE(null->data != NULL || null->length == 0);

	UNUSED(type);
	UNUSED(rdclass);

	return (mem_tobuffer(target, null->data, null->length));
}

static inline isc_result_t
tostruct_null(ARGS_TOSTRUCT) {
	dns_rdata_null_t *null = target;
	isc_region_t r;

	REQUIRE(rdata->type == 10);
	REQUIRE(target != NULL);

	null->common.rdclass = rdata->rdclass;
	null->common.rdtype = rdata->type;
	ISC_LINK_INIT(&null->common, link);

	dns_rdata_toregion(rdata, &r);
	null->length = r.length;
	null->data = mem_maybedup(mctx, r.base, r.length);
	if (null->data == NULL)
		return (ISC_R_NOMEMORY);

	null->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static inline void
freestruct_null(ARGS_FREESTRUCT) {
	dns_rdata_null_t *null = source;

	REQUIRE(source != NULL);
	REQUIRE(null->common.rdtype == 10);

	if (null->mctx == NULL)
		return;

	if (null->data != NULL)
		isc_mem_free(null->mctx, null->data);
	null->mctx = NULL;
}

static inline isc_result_t
additionaldata_null(ARGS_ADDLDATA) {
	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	REQUIRE(rdata->type == 10);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_null(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == 10);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_null(ARGS_CHECKOWNER) {

	REQUIRE(type == 10);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_null(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == 10);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

#endif	/* RDATA_GENERIC_NULL_10_C */
