/*
 * Copyright (C) 2004, 2006, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2003  Internet Software Consortium.
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

/* $Id: sshfp_44.c,v 1.7 2007/06/19 23:47:17 tbox Exp $ */

/* RFC 4255 */

#ifndef RDATA_GENERIC_SSHFP_44_C
#define RDATA_GENERIC_SSHFP_44_C

#define RRTYPE_SSHFP_ATTRIBUTES (0)

static inline isc_result_t
fromtext_sshfp(ARGS_FROMTEXT) {
	isc_token_t token;

	REQUIRE(type == 44);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(callbacks);

	/*
	 * Algorithm.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      ISC_FALSE));
	if (token.value.as_ulong > 0xffU)
		RETTOK(ISC_R_RANGE);
	RETERR(uint8_tobuffer(token.value.as_ulong, target));

	/*
	 * Digest type.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      ISC_FALSE));
	if (token.value.as_ulong > 0xffU)
		RETTOK(ISC_R_RANGE);
	RETERR(uint8_tobuffer(token.value.as_ulong, target));
	type = (isc_uint16_t) token.value.as_ulong;

	/*
	 * Digest.
	 */
	return (isc_hex_tobuffer(lexer, target, -1));
}

static inline isc_result_t
totext_sshfp(ARGS_TOTEXT) {
	isc_region_t sr;
	char buf[sizeof("64000 ")];
	unsigned int n;

	REQUIRE(rdata->type == 44);
	REQUIRE(rdata->length != 0);

	UNUSED(tctx);

	dns_rdata_toregion(rdata, &sr);

	/*
	 * Algorithm.
	 */
	n = uint8_fromregion(&sr);
	isc_region_consume(&sr, 1);
	sprintf(buf, "%u ", n);
	RETERR(str_totext(buf, target));

	/*
	 * Digest type.
	 */
	n = uint8_fromregion(&sr);
	isc_region_consume(&sr, 1);
	sprintf(buf, "%u", n);
	RETERR(str_totext(buf, target));

	/*
	 * Digest.
	 */
	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0)
		RETERR(str_totext(" (", target));
	RETERR(str_totext(tctx->linebreak, target));
	RETERR(isc_hex_totext(&sr, tctx->width - 2, tctx->linebreak, target));
	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0)
		RETERR(str_totext(" )", target));
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
fromwire_sshfp(ARGS_FROMWIRE) {
	isc_region_t sr;

	REQUIRE(type == 44);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(dctx);
	UNUSED(options);

	isc_buffer_activeregion(source, &sr);
	if (sr.length < 4)
		return (ISC_R_UNEXPECTEDEND);

	isc_buffer_forward(source, sr.length);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static inline isc_result_t
towire_sshfp(ARGS_TOWIRE) {
	isc_region_t sr;

	REQUIRE(rdata->type == 44);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	dns_rdata_toregion(rdata, &sr);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static inline int
compare_sshfp(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 44);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_sshfp(ARGS_FROMSTRUCT) {
	dns_rdata_sshfp_t *sshfp = source;

	REQUIRE(type == 44);
	REQUIRE(source != NULL);
	REQUIRE(sshfp->common.rdtype == type);
	REQUIRE(sshfp->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	RETERR(uint8_tobuffer(sshfp->algorithm, target));
	RETERR(uint8_tobuffer(sshfp->digest_type, target));

	return (mem_tobuffer(target, sshfp->digest, sshfp->length));
}

static inline isc_result_t
tostruct_sshfp(ARGS_TOSTRUCT) {
	dns_rdata_sshfp_t *sshfp = target;
	isc_region_t region;

	REQUIRE(rdata->type == 44);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	sshfp->common.rdclass = rdata->rdclass;
	sshfp->common.rdtype = rdata->type;
	ISC_LINK_INIT(&sshfp->common, link);

	dns_rdata_toregion(rdata, &region);

	sshfp->algorithm = uint8_fromregion(&region);
	isc_region_consume(&region, 1);
	sshfp->digest_type = uint8_fromregion(&region);
	isc_region_consume(&region, 1);
	sshfp->length = region.length;

	sshfp->digest = mem_maybedup(mctx, region.base, region.length);
	if (sshfp->digest == NULL)
		return (ISC_R_NOMEMORY);

	sshfp->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static inline void
freestruct_sshfp(ARGS_FREESTRUCT) {
	dns_rdata_sshfp_t *sshfp = source;

	REQUIRE(sshfp != NULL);
	REQUIRE(sshfp->common.rdtype == 44);

	if (sshfp->mctx == NULL)
		return;

	if (sshfp->digest != NULL)
		isc_mem_free(sshfp->mctx, sshfp->digest);
	sshfp->mctx = NULL;
}

static inline isc_result_t
additionaldata_sshfp(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == 44);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_sshfp(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == 44);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_sshfp(ARGS_CHECKOWNER) {

	REQUIRE(type == 44);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_sshfp(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == 44);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

#endif	/* RDATA_GENERIC_SSHFP_44_C */
