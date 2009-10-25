/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2001, 2003  Internet Software Consortium.
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

/* $Id: px_26.c,v 1.43 2007/06/19 23:47:17 tbox Exp $ */

/* Reviewed: Mon Mar 20 10:44:27 PST 2000 */

/* RFC2163 */

#ifndef RDATA_IN_1_PX_26_C
#define RDATA_IN_1_PX_26_C

#define RRTYPE_PX_ATTRIBUTES (0)

static inline isc_result_t
fromtext_in_px(ARGS_FROMTEXT) {
	isc_token_t token;
	dns_name_t name;
	isc_buffer_t buffer;

	REQUIRE(type == 26);
	REQUIRE(rdclass == 1);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(callbacks);

	/*
	 * Preference.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      ISC_FALSE));
	if (token.value.as_ulong > 0xffffU)
		RETTOK(ISC_R_RANGE);
	RETERR(uint16_tobuffer(token.value.as_ulong, target));

	/*
	 * MAP822.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	dns_name_init(&name, NULL);
	buffer_fromregion(&buffer, &token.value.as_region);
	origin = (origin != NULL) ? origin : dns_rootname;
	RETTOK(dns_name_fromtext(&name, &buffer, origin, options, target));

	/*
	 * MAPX400.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	dns_name_init(&name, NULL);
	buffer_fromregion(&buffer, &token.value.as_region);
	origin = (origin != NULL) ? origin : dns_rootname;
	RETTOK(dns_name_fromtext(&name, &buffer, origin, options, target));
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
totext_in_px(ARGS_TOTEXT) {
	isc_region_t region;
	dns_name_t name;
	dns_name_t prefix;
	isc_boolean_t sub;
	char buf[sizeof("64000")];
	unsigned short num;

	REQUIRE(rdata->type == 26);
	REQUIRE(rdata->rdclass == 1);
	REQUIRE(rdata->length != 0);

	dns_name_init(&name, NULL);
	dns_name_init(&prefix, NULL);

	/*
	 * Preference.
	 */
	dns_rdata_toregion(rdata, &region);
	num = uint16_fromregion(&region);
	isc_region_consume(&region, 2);
	sprintf(buf, "%u", num);
	RETERR(str_totext(buf, target));
	RETERR(str_totext(" ", target));

	/*
	 * MAP822.
	 */
	dns_name_fromregion(&name, &region);
	sub = name_prefix(&name, tctx->origin, &prefix);
	isc_region_consume(&region, name_length(&name));
	RETERR(dns_name_totext(&prefix, sub, target));
	RETERR(str_totext(" ", target));

	/*
	 * MAPX400.
	 */
	dns_name_fromregion(&name, &region);
	sub = name_prefix(&name, tctx->origin, &prefix);
	return(dns_name_totext(&prefix, sub, target));
}

static inline isc_result_t
fromwire_in_px(ARGS_FROMWIRE) {
        dns_name_t name;
	isc_region_t sregion;

	REQUIRE(type == 26);
	REQUIRE(rdclass == 1);

	UNUSED(type);
	UNUSED(rdclass);

	dns_decompress_setmethods(dctx, DNS_COMPRESS_NONE);

        dns_name_init(&name, NULL);

	/*
	 * Preference.
	 */
	isc_buffer_activeregion(source, &sregion);
	if (sregion.length < 2)
		return (ISC_R_UNEXPECTEDEND);
	RETERR(mem_tobuffer(target, sregion.base, 2));
	isc_buffer_forward(source, 2);

	/*
	 * MAP822.
	 */
	RETERR(dns_name_fromwire(&name, source, dctx, options, target));

	/*
	 * MAPX400.
	 */
	return (dns_name_fromwire(&name, source, dctx, options, target));
}

static inline isc_result_t
towire_in_px(ARGS_TOWIRE) {
	dns_name_t name;
	dns_offsets_t offsets;
	isc_region_t region;

	REQUIRE(rdata->type == 26);
	REQUIRE(rdata->rdclass == 1);
	REQUIRE(rdata->length != 0);

	dns_compress_setmethods(cctx, DNS_COMPRESS_NONE);
	/*
	 * Preference.
	 */
	dns_rdata_toregion(rdata, &region);
	RETERR(mem_tobuffer(target, region.base, 2));
	isc_region_consume(&region, 2);

	/*
	 * MAP822.
	 */
	dns_name_init(&name, offsets);
	dns_name_fromregion(&name, &region);
	RETERR(dns_name_towire(&name, cctx, target));
	isc_region_consume(&region, name_length(&name));

	/*
	 * MAPX400.
	 */
	dns_name_init(&name, offsets);
	dns_name_fromregion(&name, &region);
	return (dns_name_towire(&name, cctx, target));
}

static inline int
compare_in_px(ARGS_COMPARE) {
	dns_name_t name1;
	dns_name_t name2;
	isc_region_t region1;
	isc_region_t region2;
	int order;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 26);
	REQUIRE(rdata1->rdclass == 1);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	order = memcmp(rdata1->data, rdata2->data, 2);
	if (order != 0)
		return (order < 0 ? -1 : 1);

	dns_name_init(&name1, NULL);
	dns_name_init(&name2, NULL);

	dns_rdata_toregion(rdata1, &region1);
	dns_rdata_toregion(rdata2, &region2);

	isc_region_consume(&region1, 2);
	isc_region_consume(&region2, 2);

	dns_name_fromregion(&name1, &region1);
	dns_name_fromregion(&name2, &region2);

	order = dns_name_rdatacompare(&name1, &name2);
	if (order != 0)
		return (order);

	isc_region_consume(&region1, name_length(&name1));
	isc_region_consume(&region2, name_length(&name2));

	dns_name_fromregion(&name1, &region1);
	dns_name_fromregion(&name2, &region2);

	return (dns_name_rdatacompare(&name1, &name2));
}

static inline isc_result_t
fromstruct_in_px(ARGS_FROMSTRUCT) {
	dns_rdata_in_px_t *px = source;
	isc_region_t region;

	REQUIRE(type == 26);
	REQUIRE(rdclass == 1);
	REQUIRE(source != NULL);
	REQUIRE(px->common.rdtype == type);
	REQUIRE(px->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	RETERR(uint16_tobuffer(px->preference, target));
	dns_name_toregion(&px->map822, &region);
	RETERR(isc_buffer_copyregion(target, &region));
	dns_name_toregion(&px->mapx400, &region);
	return (isc_buffer_copyregion(target, &region));
}

static inline isc_result_t
tostruct_in_px(ARGS_TOSTRUCT) {
	dns_rdata_in_px_t *px = target;
	dns_name_t name;
	isc_region_t region;
	isc_result_t result;

	REQUIRE(rdata->type == 26);
	REQUIRE(rdata->rdclass == 1);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	px->common.rdclass = rdata->rdclass;
	px->common.rdtype = rdata->type;
	ISC_LINK_INIT(&px->common, link);

	dns_name_init(&name, NULL);
	dns_rdata_toregion(rdata, &region);

	px->preference = uint16_fromregion(&region);
	isc_region_consume(&region, 2);

	dns_name_fromregion(&name, &region);

	dns_name_init(&px->map822, NULL);
	RETERR(name_duporclone(&name, mctx, &px->map822));
	isc_region_consume(&region, name_length(&px->map822));

	dns_name_init(&px->mapx400, NULL);
	result = name_duporclone(&name, mctx, &px->mapx400);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	px->mctx = mctx;
	return (result);

 cleanup:
	dns_name_free(&px->map822, mctx);
	return (ISC_R_NOMEMORY);
}

static inline void
freestruct_in_px(ARGS_FREESTRUCT) {
	dns_rdata_in_px_t *px = source;

	REQUIRE(source != NULL);
	REQUIRE(px->common.rdclass == 1);
	REQUIRE(px->common.rdtype == 26);

	if (px->mctx == NULL)
		return;

	dns_name_free(&px->map822, px->mctx);
	dns_name_free(&px->mapx400, px->mctx);
	px->mctx = NULL;
}

static inline isc_result_t
additionaldata_in_px(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == 26);
	REQUIRE(rdata->rdclass == 1);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_in_px(ARGS_DIGEST) {
	isc_region_t r1, r2;
	dns_name_t name;
	isc_result_t result;

	REQUIRE(rdata->type == 26);
	REQUIRE(rdata->rdclass == 1);

	dns_rdata_toregion(rdata, &r1);
	r2 = r1;
	isc_region_consume(&r2, 2);
	r1.length = 2;
	result = (digest)(arg, &r1);
	if (result != ISC_R_SUCCESS)
		return (result);
	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &r2);
	result = dns_name_digest(&name, digest, arg);
	if (result != ISC_R_SUCCESS)
		return (result);
	isc_region_consume(&r2, name_length(&name));
	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &r2);

	return (dns_name_digest(&name, digest, arg));
}

static inline isc_boolean_t
checkowner_in_px(ARGS_CHECKOWNER) {

	REQUIRE(type == 26);
	REQUIRE(rdclass == 1);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_in_px(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == 26);
	REQUIRE(rdata->rdclass == 1);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

#endif	/* RDATA_IN_1_PX_26_C */
