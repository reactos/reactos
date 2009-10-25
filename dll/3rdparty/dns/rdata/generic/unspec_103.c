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

/* $Id: unspec_103.c,v 1.35 2007/06/19 23:47:17 tbox Exp $ */

#ifndef RDATA_GENERIC_UNSPEC_103_C
#define RDATA_GENERIC_UNSPEC_103_C

#define RRTYPE_UNSPEC_ATTRIBUTES (0)

static inline isc_result_t
fromtext_unspec(ARGS_FROMTEXT) {

	REQUIRE(type == 103);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(callbacks);

	return (atob_tobuffer(lexer, target));
}

static inline isc_result_t
totext_unspec(ARGS_TOTEXT) {

	REQUIRE(rdata->type == 103);

	UNUSED(tctx);

	return (btoa_totext(rdata->data, rdata->length, target));
}

static inline isc_result_t
fromwire_unspec(ARGS_FROMWIRE) {
	isc_region_t sr;

	REQUIRE(type == 103);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(dctx);
	UNUSED(options);

	isc_buffer_activeregion(source, &sr);
	isc_buffer_forward(source, sr.length);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static inline isc_result_t
towire_unspec(ARGS_TOWIRE) {

	REQUIRE(rdata->type == 103);

	UNUSED(cctx);

	return (mem_tobuffer(target, rdata->data, rdata->length));
}

static inline int
compare_unspec(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 103);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_unspec(ARGS_FROMSTRUCT) {
	dns_rdata_unspec_t *unspec = source;

	REQUIRE(type == 103);
	REQUIRE(source != NULL);
	REQUIRE(unspec->common.rdtype == type);
	REQUIRE(unspec->common.rdclass == rdclass);
	REQUIRE(unspec->data != NULL || unspec->datalen == 0);

	UNUSED(type);
	UNUSED(rdclass);

	return (mem_tobuffer(target, unspec->data, unspec->datalen));
}

static inline isc_result_t
tostruct_unspec(ARGS_TOSTRUCT) {
	dns_rdata_unspec_t *unspec = target;
	isc_region_t r;

	REQUIRE(rdata->type == 103);
	REQUIRE(target != NULL);

	unspec->common.rdclass = rdata->rdclass;
	unspec->common.rdtype = rdata->type;
	ISC_LINK_INIT(&unspec->common, link);

	dns_rdata_toregion(rdata, &r);
	unspec->datalen = r.length;
	unspec->data = mem_maybedup(mctx, r.base, r.length);
	if (unspec->data == NULL)
		return (ISC_R_NOMEMORY);

	unspec->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static inline void
freestruct_unspec(ARGS_FREESTRUCT) {
	dns_rdata_unspec_t *unspec = source;

	REQUIRE(source != NULL);
	REQUIRE(unspec->common.rdtype == 103);

	if (unspec->mctx == NULL)
		return;

	if (unspec->data != NULL)
		isc_mem_free(unspec->mctx, unspec->data);
	unspec->mctx = NULL;
}

static inline isc_result_t
additionaldata_unspec(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == 103);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_unspec(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == 103);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_unspec(ARGS_CHECKOWNER) {

	REQUIRE(type == 103);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_unspec(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == 103);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

#endif	/* RDATA_GENERIC_UNSPEC_103_C */
