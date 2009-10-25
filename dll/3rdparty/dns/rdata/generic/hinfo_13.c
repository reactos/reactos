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

/* $Id: hinfo_13.c,v 1.44 2007/06/19 23:47:17 tbox Exp $ */

/*
 * Reviewed: Wed Mar 15 16:47:10 PST 2000 by halley.
 */

#ifndef RDATA_GENERIC_HINFO_13_C
#define RDATA_GENERIC_HINFO_13_C

#define RRTYPE_HINFO_ATTRIBUTES (0)

static inline isc_result_t
fromtext_hinfo(ARGS_FROMTEXT) {
	isc_token_t token;
	int i;

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(callbacks);

	REQUIRE(type == 13);

	for (i = 0; i < 2; i++) {
		RETERR(isc_lex_getmastertoken(lexer, &token,
					      isc_tokentype_qstring,
					      ISC_FALSE));
		RETTOK(txt_fromtext(&token.value.as_textregion, target));
	}
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
totext_hinfo(ARGS_TOTEXT) {
	isc_region_t region;

	UNUSED(tctx);

	REQUIRE(rdata->type == 13);
	REQUIRE(rdata->length != 0);

	dns_rdata_toregion(rdata, &region);
	RETERR(txt_totext(&region, target));
	RETERR(str_totext(" ", target));
	return (txt_totext(&region, target));
}

static inline isc_result_t
fromwire_hinfo(ARGS_FROMWIRE) {

	REQUIRE(type == 13);

	UNUSED(type);
	UNUSED(dctx);
	UNUSED(rdclass);
	UNUSED(options);

	RETERR(txt_fromwire(source, target));
	return (txt_fromwire(source, target));
}

static inline isc_result_t
towire_hinfo(ARGS_TOWIRE) {

	UNUSED(cctx);

	REQUIRE(rdata->type == 13);
	REQUIRE(rdata->length != 0);

	return (mem_tobuffer(target, rdata->data, rdata->length));
}

static inline int
compare_hinfo(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 13);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_hinfo(ARGS_FROMSTRUCT) {
	dns_rdata_hinfo_t *hinfo = source;

	REQUIRE(type == 13);
	REQUIRE(source != NULL);
	REQUIRE(hinfo->common.rdtype == type);
	REQUIRE(hinfo->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	RETERR(uint8_tobuffer(hinfo->cpu_len, target));
	RETERR(mem_tobuffer(target, hinfo->cpu, hinfo->cpu_len));
	RETERR(uint8_tobuffer(hinfo->os_len, target));
	return (mem_tobuffer(target, hinfo->os, hinfo->os_len));
}

static inline isc_result_t
tostruct_hinfo(ARGS_TOSTRUCT) {
	dns_rdata_hinfo_t *hinfo = target;
	isc_region_t region;

	REQUIRE(rdata->type == 13);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	hinfo->common.rdclass = rdata->rdclass;
	hinfo->common.rdtype = rdata->type;
	ISC_LINK_INIT(&hinfo->common, link);

	dns_rdata_toregion(rdata, &region);
	hinfo->cpu_len = uint8_fromregion(&region);
	isc_region_consume(&region, 1);
	hinfo->cpu = mem_maybedup(mctx, region.base, hinfo->cpu_len);
	if (hinfo->cpu == NULL)
		return (ISC_R_NOMEMORY);
	isc_region_consume(&region, hinfo->cpu_len);

	hinfo->os_len = uint8_fromregion(&region);
	isc_region_consume(&region, 1);
	hinfo->os = mem_maybedup(mctx, region.base, hinfo->os_len);
	if (hinfo->os == NULL)
		goto cleanup;

	hinfo->mctx = mctx;
	return (ISC_R_SUCCESS);

 cleanup:
	if (mctx != NULL && hinfo->cpu != NULL)
		isc_mem_free(mctx, hinfo->cpu);
	return (ISC_R_NOMEMORY);
}

static inline void
freestruct_hinfo(ARGS_FREESTRUCT) {
	dns_rdata_hinfo_t *hinfo = source;

	REQUIRE(source != NULL);

	if (hinfo->mctx == NULL)
		return;

	if (hinfo->cpu != NULL)
		isc_mem_free(hinfo->mctx, hinfo->cpu);
	if (hinfo->os != NULL)
		isc_mem_free(hinfo->mctx, hinfo->os);
	hinfo->mctx = NULL;
}

static inline isc_result_t
additionaldata_hinfo(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == 13);

	UNUSED(add);
	UNUSED(arg);
	UNUSED(rdata);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_hinfo(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == 13);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_hinfo(ARGS_CHECKOWNER) {

	REQUIRE(type == 13);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_hinfo(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == 13);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

#endif	/* RDATA_GENERIC_HINFO_13_C */
