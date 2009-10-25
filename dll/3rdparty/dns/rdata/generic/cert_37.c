/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2003  Internet Software Consortium.
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

/* $Id: cert_37.c,v 1.50 2007/06/19 23:47:17 tbox Exp $ */

/* Reviewed: Wed Mar 15 21:14:32 EST 2000 by tale */

/* RFC2538 */

#ifndef RDATA_GENERIC_CERT_37_C
#define RDATA_GENERIC_CERT_37_C

#define RRTYPE_CERT_ATTRIBUTES (0)

static inline isc_result_t
fromtext_cert(ARGS_FROMTEXT) {
	isc_token_t token;
	dns_secalg_t secalg;
	dns_cert_t cert;

	REQUIRE(type == 37);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(callbacks);

	/*
	 * Cert type.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	RETTOK(dns_cert_fromtext(&cert, &token.value.as_textregion));
	RETERR(uint16_tobuffer(cert, target));

	/*
	 * Key tag.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      ISC_FALSE));
	if (token.value.as_ulong > 0xffffU)
		RETTOK(ISC_R_RANGE);
	RETERR(uint16_tobuffer(token.value.as_ulong, target));

	/*
	 * Algorithm.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	RETTOK(dns_secalg_fromtext(&secalg, &token.value.as_textregion));
	RETERR(mem_tobuffer(target, &secalg, 1));

	return (isc_base64_tobuffer(lexer, target, -1));
}

static inline isc_result_t
totext_cert(ARGS_TOTEXT) {
	isc_region_t sr;
	char buf[sizeof("64000 ")];
	unsigned int n;

	REQUIRE(rdata->type == 37);
	REQUIRE(rdata->length != 0);

	UNUSED(tctx);

	dns_rdata_toregion(rdata, &sr);

	/*
	 * Type.
	 */
	n = uint16_fromregion(&sr);
	isc_region_consume(&sr, 2);
	RETERR(dns_cert_totext((dns_cert_t)n, target));
	RETERR(str_totext(" ", target));

	/*
	 * Key tag.
	 */
	n = uint16_fromregion(&sr);
	isc_region_consume(&sr, 2);
	sprintf(buf, "%u ", n);
	RETERR(str_totext(buf, target));

	/*
	 * Algorithm.
	 */
	RETERR(dns_secalg_totext(sr.base[0], target));
	isc_region_consume(&sr, 1);

	/*
	 * Cert.
	 */
	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0)
		RETERR(str_totext(" (", target));
	RETERR(str_totext(tctx->linebreak, target));
	RETERR(isc_base64_totext(&sr, tctx->width - 2,
				 tctx->linebreak, target));
	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0)
		RETERR(str_totext(" )", target));
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
fromwire_cert(ARGS_FROMWIRE) {
	isc_region_t sr;

	REQUIRE(type == 37);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(dctx);
	UNUSED(options);

	isc_buffer_activeregion(source, &sr);
	if (sr.length < 5)
		return (ISC_R_UNEXPECTEDEND);

	isc_buffer_forward(source, sr.length);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static inline isc_result_t
towire_cert(ARGS_TOWIRE) {
	isc_region_t sr;

	REQUIRE(rdata->type == 37);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	dns_rdata_toregion(rdata, &sr);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static inline int
compare_cert(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 37);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_cert(ARGS_FROMSTRUCT) {
	dns_rdata_cert_t *cert = source;

	REQUIRE(type == 37);
	REQUIRE(source != NULL);
	REQUIRE(cert->common.rdtype == type);
	REQUIRE(cert->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	RETERR(uint16_tobuffer(cert->type, target));
	RETERR(uint16_tobuffer(cert->key_tag, target));
	RETERR(uint8_tobuffer(cert->algorithm, target));

	return (mem_tobuffer(target, cert->certificate, cert->length));
}

static inline isc_result_t
tostruct_cert(ARGS_TOSTRUCT) {
	dns_rdata_cert_t *cert = target;
	isc_region_t region;

	REQUIRE(rdata->type == 37);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	cert->common.rdclass = rdata->rdclass;
	cert->common.rdtype = rdata->type;
	ISC_LINK_INIT(&cert->common, link);

	dns_rdata_toregion(rdata, &region);

	cert->type = uint16_fromregion(&region);
	isc_region_consume(&region, 2);
	cert->key_tag = uint16_fromregion(&region);
	isc_region_consume(&region, 2);
	cert->algorithm = uint8_fromregion(&region);
	isc_region_consume(&region, 1);
	cert->length = region.length;

	cert->certificate = mem_maybedup(mctx, region.base, region.length);
	if (cert->certificate == NULL)
		return (ISC_R_NOMEMORY);

	cert->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static inline void
freestruct_cert(ARGS_FREESTRUCT) {
	dns_rdata_cert_t *cert = source;

	REQUIRE(cert != NULL);
	REQUIRE(cert->common.rdtype == 37);

	if (cert->mctx == NULL)
		return;

	if (cert->certificate != NULL)
		isc_mem_free(cert->mctx, cert->certificate);
	cert->mctx = NULL;
}

static inline isc_result_t
additionaldata_cert(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == 37);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_cert(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == 37);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_cert(ARGS_CHECKOWNER) {

	REQUIRE(type == 37);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_cert(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == 37);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

#endif	/* RDATA_GENERIC_CERT_37_C */

