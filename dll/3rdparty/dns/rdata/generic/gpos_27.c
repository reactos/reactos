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

/* $Id: gpos_27.c,v 1.41 2007/06/19 23:47:17 tbox Exp $ */

/* reviewed: Wed Mar 15 16:48:45 PST 2000 by brister */

/* RFC1712 */

#ifndef RDATA_GENERIC_GPOS_27_C
#define RDATA_GENERIC_GPOS_27_C

#define RRTYPE_GPOS_ATTRIBUTES (0)

static inline isc_result_t
fromtext_gpos(ARGS_FROMTEXT) {
	isc_token_t token;
	int i;

	REQUIRE(type == 27);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(callbacks);

	for (i = 0; i < 3; i++) {
		RETERR(isc_lex_getmastertoken(lexer, &token,
					      isc_tokentype_qstring,
					      ISC_FALSE));
		RETTOK(txt_fromtext(&token.value.as_textregion, target));
	}
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
totext_gpos(ARGS_TOTEXT) {
	isc_region_t region;
	int i;

	REQUIRE(rdata->type == 27);
	REQUIRE(rdata->length != 0);

	UNUSED(tctx);

	dns_rdata_toregion(rdata, &region);

	for (i = 0; i < 3; i++) {
		RETERR(txt_totext(&region, target));
		if (i != 2)
			RETERR(str_totext(" ", target));
	}

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
fromwire_gpos(ARGS_FROMWIRE) {
	int i;

	REQUIRE(type == 27);

	UNUSED(type);
	UNUSED(dctx);
	UNUSED(rdclass);
	UNUSED(options);

	for (i = 0; i < 3; i++)
		RETERR(txt_fromwire(source, target));
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
towire_gpos(ARGS_TOWIRE) {

	REQUIRE(rdata->type == 27);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	return (mem_tobuffer(target, rdata->data, rdata->length));
}

static inline int
compare_gpos(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 27);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_gpos(ARGS_FROMSTRUCT) {
	dns_rdata_gpos_t *gpos = source;

	REQUIRE(type == 27);
	REQUIRE(source != NULL);
	REQUIRE(gpos->common.rdtype == type);
	REQUIRE(gpos->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	RETERR(uint8_tobuffer(gpos->long_len, target));
	RETERR(mem_tobuffer(target, gpos->longitude, gpos->long_len));
	RETERR(uint8_tobuffer(gpos->lat_len, target));
	RETERR(mem_tobuffer(target, gpos->latitude, gpos->lat_len));
	RETERR(uint8_tobuffer(gpos->alt_len, target));
	return (mem_tobuffer(target, gpos->altitude, gpos->alt_len));
}

static inline isc_result_t
tostruct_gpos(ARGS_TOSTRUCT) {
	dns_rdata_gpos_t *gpos = target;
	isc_region_t region;

	REQUIRE(rdata->type == 27);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	gpos->common.rdclass = rdata->rdclass;
	gpos->common.rdtype = rdata->type;
	ISC_LINK_INIT(&gpos->common, link);

	dns_rdata_toregion(rdata, &region);
	gpos->long_len = uint8_fromregion(&region);
	isc_region_consume(&region, 1);
	gpos->longitude = mem_maybedup(mctx, region.base, gpos->long_len);
	if (gpos->longitude == NULL)
		return (ISC_R_NOMEMORY);
	isc_region_consume(&region, gpos->long_len);

	gpos->lat_len = uint8_fromregion(&region);
	isc_region_consume(&region, 1);
	gpos->latitude = mem_maybedup(mctx, region.base, gpos->lat_len);
	if (gpos->latitude == NULL)
		goto cleanup_longitude;
	isc_region_consume(&region, gpos->lat_len);

	gpos->alt_len = uint8_fromregion(&region);
	isc_region_consume(&region, 1);
	if (gpos->lat_len > 0) {
		gpos->altitude =
			mem_maybedup(mctx, region.base, gpos->alt_len);
		if (gpos->altitude == NULL)
			goto cleanup_latitude;
	} else
		gpos->altitude = NULL;

	gpos->mctx = mctx;
	return (ISC_R_SUCCESS);

 cleanup_latitude:
	if (mctx != NULL && gpos->longitude != NULL)
		isc_mem_free(mctx, gpos->longitude);

 cleanup_longitude:
	if (mctx != NULL && gpos->latitude != NULL)
		isc_mem_free(mctx, gpos->latitude);
	return (ISC_R_NOMEMORY);
}

static inline void
freestruct_gpos(ARGS_FREESTRUCT) {
	dns_rdata_gpos_t *gpos = source;

	REQUIRE(source != NULL);
	REQUIRE(gpos->common.rdtype == 27);

	if (gpos->mctx == NULL)
		return;

	if (gpos->longitude != NULL)
		isc_mem_free(gpos->mctx, gpos->longitude);
	if (gpos->latitude != NULL)
		isc_mem_free(gpos->mctx, gpos->latitude);
	if (gpos->altitude != NULL)
		isc_mem_free(gpos->mctx, gpos->altitude);
	gpos->mctx = NULL;
}

static inline isc_result_t
additionaldata_gpos(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == 27);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_gpos(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == 27);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_gpos(ARGS_CHECKOWNER) {

	REQUIRE(type == 27);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_gpos(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == 27);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

#endif	/* RDATA_GENERIC_GPOS_27_C */
