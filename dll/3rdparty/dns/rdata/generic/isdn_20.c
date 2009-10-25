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

/* $Id: isdn_20.c,v 1.38 2007/06/19 23:47:17 tbox Exp $ */

/* Reviewed: Wed Mar 15 16:53:11 PST 2000 by bwelling */

/* RFC1183 */

#ifndef RDATA_GENERIC_ISDN_20_C
#define RDATA_GENERIC_ISDN_20_C

#define RRTYPE_ISDN_ATTRIBUTES (0)

static inline isc_result_t
fromtext_isdn(ARGS_FROMTEXT) {
	isc_token_t token;

	REQUIRE(type == 20);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(callbacks);

	/* ISDN-address */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_qstring,
				      ISC_FALSE));
	RETTOK(txt_fromtext(&token.value.as_textregion, target));

	/* sa: optional */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_qstring,
				      ISC_TRUE));
	if (token.type != isc_tokentype_string &&
	    token.type != isc_tokentype_qstring) {
		isc_lex_ungettoken(lexer, &token);
		return (ISC_R_SUCCESS);
	}
	RETTOK(txt_fromtext(&token.value.as_textregion, target));
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
totext_isdn(ARGS_TOTEXT) {
	isc_region_t region;

	REQUIRE(rdata->type == 20);
	REQUIRE(rdata->length != 0);

	UNUSED(tctx);

	dns_rdata_toregion(rdata, &region);
	RETERR(txt_totext(&region, target));
	if (region.length == 0)
		return (ISC_R_SUCCESS);
	RETERR(str_totext(" ", target));
	return (txt_totext(&region, target));
}

static inline isc_result_t
fromwire_isdn(ARGS_FROMWIRE) {
	REQUIRE(type == 20);

	UNUSED(type);
	UNUSED(dctx);
	UNUSED(rdclass);
	UNUSED(options);

	RETERR(txt_fromwire(source, target));
	if (buffer_empty(source))
		return (ISC_R_SUCCESS);
	return (txt_fromwire(source, target));
}

static inline isc_result_t
towire_isdn(ARGS_TOWIRE) {
	UNUSED(cctx);

	REQUIRE(rdata->type == 20);
	REQUIRE(rdata->length != 0);

	return (mem_tobuffer(target, rdata->data, rdata->length));
}

static inline int
compare_isdn(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 20);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_isdn(ARGS_FROMSTRUCT) {
	dns_rdata_isdn_t *isdn = source;

	REQUIRE(type == 20);
	REQUIRE(source != NULL);
	REQUIRE(isdn->common.rdtype == type);
	REQUIRE(isdn->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	RETERR(uint8_tobuffer(isdn->isdn_len, target));
	RETERR(mem_tobuffer(target, isdn->isdn, isdn->isdn_len));
	RETERR(uint8_tobuffer(isdn->subaddress_len, target));
	return (mem_tobuffer(target, isdn->subaddress, isdn->subaddress_len));
}

static inline isc_result_t
tostruct_isdn(ARGS_TOSTRUCT) {
	dns_rdata_isdn_t *isdn = target;
	isc_region_t r;

	REQUIRE(rdata->type == 20);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	isdn->common.rdclass = rdata->rdclass;
	isdn->common.rdtype = rdata->type;
	ISC_LINK_INIT(&isdn->common, link);

	dns_rdata_toregion(rdata, &r);

	isdn->isdn_len = uint8_fromregion(&r);
	isc_region_consume(&r, 1);
	isdn->isdn = mem_maybedup(mctx, r.base, isdn->isdn_len);
	if (isdn->isdn == NULL)
		return (ISC_R_NOMEMORY);
	isc_region_consume(&r, isdn->isdn_len);

	isdn->subaddress_len = uint8_fromregion(&r);
	isc_region_consume(&r, 1);
	isdn->subaddress = mem_maybedup(mctx, r.base, isdn->subaddress_len);
	if (isdn->subaddress == NULL)
		goto cleanup;

	isdn->mctx = mctx;
	return (ISC_R_SUCCESS);

 cleanup:
	if (mctx != NULL && isdn->isdn != NULL)
		isc_mem_free(mctx, isdn->isdn);
	return (ISC_R_NOMEMORY);
}

static inline void
freestruct_isdn(ARGS_FREESTRUCT) {
	dns_rdata_isdn_t *isdn = source;

	REQUIRE(source != NULL);

	if (isdn->mctx == NULL)
		return;

	if (isdn->isdn != NULL)
		isc_mem_free(isdn->mctx, isdn->isdn);
	if (isdn->subaddress != NULL)
		isc_mem_free(isdn->mctx, isdn->subaddress);
	isdn->mctx = NULL;
}

static inline isc_result_t
additionaldata_isdn(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == 20);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_isdn(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == 20);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_isdn(ARGS_CHECKOWNER) {

	REQUIRE(type == 20);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_isdn(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == 20);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

#endif	/* RDATA_GENERIC_ISDN_20_C */
