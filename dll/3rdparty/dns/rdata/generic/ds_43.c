/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2002  Internet Software Consortium.
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

/* $Id: ds_43.c,v 1.12 2007/06/18 23:47:43 tbox Exp $ */

/* draft-ietf-dnsext-delegation-signer-05.txt */

#ifndef RDATA_GENERIC_DS_43_C
#define RDATA_GENERIC_DS_43_C

#define RRTYPE_DS_ATTRIBUTES \
	(DNS_RDATATYPEATTR_DNSSEC|DNS_RDATATYPEATTR_ATPARENT)

#include <isc/sha1.h>
#include <isc/sha2.h>

#include <dns/ds.h>

static inline isc_result_t
fromtext_ds(ARGS_FROMTEXT) {
	isc_token_t token;
	unsigned char c;
	int length;

	REQUIRE(type == 43);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(callbacks);

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
	RETTOK(dns_secalg_fromtext(&c, &token.value.as_textregion));
	RETERR(mem_tobuffer(target, &c, 1));

	/*
	 * Digest type.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      ISC_FALSE));
	if (token.value.as_ulong > 0xffU)
		RETTOK(ISC_R_RANGE);
	RETERR(uint8_tobuffer(token.value.as_ulong, target));
	c = (unsigned char) token.value.as_ulong;

	/*
	 * Digest.
	 */
	if (c == DNS_DSDIGEST_SHA1)
		length = ISC_SHA1_DIGESTLENGTH;
	else if (c == DNS_DSDIGEST_SHA256)
		length = ISC_SHA256_DIGESTLENGTH;
	else
		length = -1;
	return (isc_hex_tobuffer(lexer, target, length));
}

static inline isc_result_t
totext_ds(ARGS_TOTEXT) {
	isc_region_t sr;
	char buf[sizeof("64000 ")];
	unsigned int n;

	REQUIRE(rdata->type == 43);
	REQUIRE(rdata->length != 0);

	UNUSED(tctx);

	dns_rdata_toregion(rdata, &sr);

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
fromwire_ds(ARGS_FROMWIRE) {
	isc_region_t sr;

	REQUIRE(type == 43);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(dctx);
	UNUSED(options);

	isc_buffer_activeregion(source, &sr);

	/*
	 * Check digest lengths if we know them.
	 */
	if (sr.length < 4 ||
	    (sr.base[3] == DNS_DSDIGEST_SHA1 &&
	     sr.length < 4 + ISC_SHA1_DIGESTLENGTH) ||
	    (sr.base[3] == DNS_DSDIGEST_SHA256 &&
	     sr.length < 4 + ISC_SHA256_DIGESTLENGTH))
		return (ISC_R_UNEXPECTEDEND);

	/*
	 * Only copy digest lengths if we know them.
	 * If there is extra data dns_rdata_fromwire() will
	 * detect that.
	 */
	if (sr.base[3] == DNS_DSDIGEST_SHA1)
		sr.length = 4 + ISC_SHA1_DIGESTLENGTH;
	else if (sr.base[3] == DNS_DSDIGEST_SHA256)
		sr.length = 4 + ISC_SHA256_DIGESTLENGTH;

	isc_buffer_forward(source, sr.length);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static inline isc_result_t
towire_ds(ARGS_TOWIRE) {
	isc_region_t sr;

	REQUIRE(rdata->type == 43);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	dns_rdata_toregion(rdata, &sr);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static inline int
compare_ds(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 43);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_ds(ARGS_FROMSTRUCT) {
	dns_rdata_ds_t *ds = source;

	REQUIRE(type == 43);
	REQUIRE(source != NULL);
	REQUIRE(ds->common.rdtype == type);
	REQUIRE(ds->common.rdclass == rdclass);
	switch (ds->digest_type) {
	case DNS_DSDIGEST_SHA1:
		REQUIRE(ds->length == ISC_SHA1_DIGESTLENGTH);
		break;
	case DNS_DSDIGEST_SHA256:
		REQUIRE(ds->length == ISC_SHA256_DIGESTLENGTH);
		break;
	}

	UNUSED(type);
	UNUSED(rdclass);

	RETERR(uint16_tobuffer(ds->key_tag, target));
	RETERR(uint8_tobuffer(ds->algorithm, target));
	RETERR(uint8_tobuffer(ds->digest_type, target));

	return (mem_tobuffer(target, ds->digest, ds->length));
}

static inline isc_result_t
tostruct_ds(ARGS_TOSTRUCT) {
	dns_rdata_ds_t *ds = target;
	isc_region_t region;

	REQUIRE(rdata->type == 43);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	ds->common.rdclass = rdata->rdclass;
	ds->common.rdtype = rdata->type;
	ISC_LINK_INIT(&ds->common, link);

	dns_rdata_toregion(rdata, &region);

	ds->key_tag = uint16_fromregion(&region);
	isc_region_consume(&region, 2);
	ds->algorithm = uint8_fromregion(&region);
	isc_region_consume(&region, 1);
	ds->digest_type = uint8_fromregion(&region);
	isc_region_consume(&region, 1);
	ds->length = region.length;

	ds->digest = mem_maybedup(mctx, region.base, region.length);
	if (ds->digest == NULL)
		return (ISC_R_NOMEMORY);

	ds->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static inline void
freestruct_ds(ARGS_FREESTRUCT) {
	dns_rdata_ds_t *ds = source;

	REQUIRE(ds != NULL);
	REQUIRE(ds->common.rdtype == 43);

	if (ds->mctx == NULL)
		return;

	if (ds->digest != NULL)
		isc_mem_free(ds->mctx, ds->digest);
	ds->mctx = NULL;
}

static inline isc_result_t
additionaldata_ds(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == 43);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_ds(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == 43);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_ds(ARGS_CHECKOWNER) {

	REQUIRE(type == 43);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_ds(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == 43);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

#endif	/* RDATA_GENERIC_DS_43_C */
