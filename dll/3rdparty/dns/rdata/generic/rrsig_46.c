/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: rrsig_46.c,v 1.10 2007/06/19 23:47:17 tbox Exp $ */

/* Reviewed: Fri Mar 17 09:05:02 PST 2000 by gson */

/* RFC2535 */

#ifndef RDATA_GENERIC_RRSIG_46_C
#define RDATA_GENERIC_RRSIG_46_C

#define RRTYPE_RRSIG_ATTRIBUTES (DNS_RDATATYPEATTR_DNSSEC)

static inline isc_result_t
fromtext_rrsig(ARGS_FROMTEXT) {
	isc_token_t token;
	unsigned char c;
	long i;
	dns_rdatatype_t covered;
	char *e;
	isc_result_t result;
	dns_name_t name;
	isc_buffer_t buffer;
	isc_uint32_t time_signed, time_expire;

	REQUIRE(type == 46);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(callbacks);

	/*
	 * Type covered.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	result = dns_rdatatype_fromtext(&covered, &token.value.as_textregion);
	if (result != ISC_R_SUCCESS && result != ISC_R_NOTIMPLEMENTED) {
		i = strtol(DNS_AS_STR(token), &e, 10);
		if (i < 0 || i > 65535)
			RETTOK(ISC_R_RANGE);
		if (*e != 0)
			RETTOK(result);
		covered = (dns_rdatatype_t)i;
	}
	RETERR(uint16_tobuffer(covered, target));

	/*
	 * Algorithm.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	RETTOK(dns_secalg_fromtext(&c, &token.value.as_textregion));
	RETERR(mem_tobuffer(target, &c, 1));

	/*
	 * Labels.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      ISC_FALSE));
	if (token.value.as_ulong > 0xffU)
		RETTOK(ISC_R_RANGE);
	c = (unsigned char)token.value.as_ulong;
	RETERR(mem_tobuffer(target, &c, 1));

	/*
	 * Original ttl.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      ISC_FALSE));
	RETERR(uint32_tobuffer(token.value.as_ulong, target));

	/*
	 * Signature expiration.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	RETTOK(dns_time32_fromtext(DNS_AS_STR(token), &time_expire));
	RETERR(uint32_tobuffer(time_expire, target));

	/*
	 * Time signed.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	RETTOK(dns_time32_fromtext(DNS_AS_STR(token), &time_signed));
	RETERR(uint32_tobuffer(time_signed, target));

	/*
	 * Key footprint.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      ISC_FALSE));
	RETERR(uint16_tobuffer(token.value.as_ulong, target));

	/*
	 * Signer.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	dns_name_init(&name, NULL);
	buffer_fromregion(&buffer, &token.value.as_region);
	origin = (origin != NULL) ? origin : dns_rootname;
	RETTOK(dns_name_fromtext(&name, &buffer, origin, options, target));

	/*
	 * Sig.
	 */
	return (isc_base64_tobuffer(lexer, target, -1));
}

static inline isc_result_t
totext_rrsig(ARGS_TOTEXT) {
	isc_region_t sr;
	char buf[sizeof("4294967295")];
	dns_rdatatype_t covered;
	unsigned long ttl;
	unsigned long when;
	unsigned long exp;
	unsigned long foot;
	dns_name_t name;
	dns_name_t prefix;
	isc_boolean_t sub;

	REQUIRE(rdata->type == 46);
	REQUIRE(rdata->length != 0);

	dns_rdata_toregion(rdata, &sr);

	/*
	 * Type covered.
	 */
	covered = uint16_fromregion(&sr);
	isc_region_consume(&sr, 2);
	/*
	 * XXXAG We should have something like dns_rdatatype_isknown()
	 * that does the right thing with type 0.
	 */
	if (dns_rdatatype_isknown(covered) && covered != 0) {
		RETERR(dns_rdatatype_totext(covered, target));
	} else {
		char buf[sizeof("TYPE65535")];
		sprintf(buf, "TYPE%u", covered);
		RETERR(str_totext(buf, target));
	}
	RETERR(str_totext(" ", target));

	/*
	 * Algorithm.
	 */
	sprintf(buf, "%u", sr.base[0]);
	isc_region_consume(&sr, 1);
	RETERR(str_totext(buf, target));
	RETERR(str_totext(" ", target));

	/*
	 * Labels.
	 */
	sprintf(buf, "%u", sr.base[0]);
	isc_region_consume(&sr, 1);
	RETERR(str_totext(buf, target));
	RETERR(str_totext(" ", target));

	/*
	 * Ttl.
	 */
	ttl = uint32_fromregion(&sr);
	isc_region_consume(&sr, 4);
	sprintf(buf, "%lu", ttl);
	RETERR(str_totext(buf, target));
	RETERR(str_totext(" ", target));

	/*
	 * Sig exp.
	 */
	exp = uint32_fromregion(&sr);
	isc_region_consume(&sr, 4);
	RETERR(dns_time32_totext(exp, target));

	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0)
		RETERR(str_totext(" (", target));
	RETERR(str_totext(tctx->linebreak, target));

	/*
	 * Time signed.
	 */
	when = uint32_fromregion(&sr);
	isc_region_consume(&sr, 4);
	RETERR(dns_time32_totext(when, target));
	RETERR(str_totext(" ", target));

	/*
	 * Footprint.
	 */
	foot = uint16_fromregion(&sr);
	isc_region_consume(&sr, 2);
	sprintf(buf, "%lu", foot);
	RETERR(str_totext(buf, target));
	RETERR(str_totext(" ", target));

	/*
	 * Signer.
	 */
	dns_name_init(&name, NULL);
	dns_name_init(&prefix, NULL);
	dns_name_fromregion(&name, &sr);
	isc_region_consume(&sr, name_length(&name));
	sub = name_prefix(&name, tctx->origin, &prefix);
	RETERR(dns_name_totext(&prefix, sub, target));

	/*
	 * Sig.
	 */
	RETERR(str_totext(tctx->linebreak, target));
	RETERR(isc_base64_totext(&sr, tctx->width - 2,
				    tctx->linebreak, target));
	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0)
		RETERR(str_totext(" )", target));

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
fromwire_rrsig(ARGS_FROMWIRE) {
	isc_region_t sr;
	dns_name_t name;

	REQUIRE(type == 46);

	UNUSED(type);
	UNUSED(rdclass);

	dns_decompress_setmethods(dctx, DNS_COMPRESS_NONE);

	isc_buffer_activeregion(source, &sr);
	/*
	 * type covered: 2
	 * algorithm: 1
	 * labels: 1
	 * original ttl: 4
	 * signature expiration: 4
	 * time signed: 4
	 * key footprint: 2
	 */
	if (sr.length < 18)
		return (ISC_R_UNEXPECTEDEND);

	isc_buffer_forward(source, 18);
	RETERR(mem_tobuffer(target, sr.base, 18));

	/*
	 * Signer.
	 */
	dns_name_init(&name, NULL);
	RETERR(dns_name_fromwire(&name, source, dctx, options, target));

	/*
	 * Sig.
	 */
	isc_buffer_activeregion(source, &sr);
	isc_buffer_forward(source, sr.length);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static inline isc_result_t
towire_rrsig(ARGS_TOWIRE) {
	isc_region_t sr;
	dns_name_t name;
	dns_offsets_t offsets;

	REQUIRE(rdata->type == 46);
	REQUIRE(rdata->length != 0);

	dns_compress_setmethods(cctx, DNS_COMPRESS_NONE);
	dns_rdata_toregion(rdata, &sr);
	/*
	 * type covered: 2
	 * algorithm: 1
	 * labels: 1
	 * original ttl: 4
	 * signature expiration: 4
	 * time signed: 4
	 * key footprint: 2
	 */
	RETERR(mem_tobuffer(target, sr.base, 18));
	isc_region_consume(&sr, 18);

	/*
	 * Signer.
	 */
	dns_name_init(&name, offsets);
	dns_name_fromregion(&name, &sr);
	isc_region_consume(&sr, name_length(&name));
	RETERR(dns_name_towire(&name, cctx, target));

	/*
	 * Signature.
	 */
	return (mem_tobuffer(target, sr.base, sr.length));
}

static inline int
compare_rrsig(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 46);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_rrsig(ARGS_FROMSTRUCT) {
	dns_rdata_rrsig_t *sig = source;

	REQUIRE(type == 46);
	REQUIRE(source != NULL);
	REQUIRE(sig->common.rdtype == type);
	REQUIRE(sig->common.rdclass == rdclass);
	REQUIRE(sig->signature != NULL || sig->siglen == 0);

	UNUSED(type);
	UNUSED(rdclass);

	/*
	 * Type covered.
	 */
	RETERR(uint16_tobuffer(sig->covered, target));

	/*
	 * Algorithm.
	 */
	RETERR(uint8_tobuffer(sig->algorithm, target));

	/*
	 * Labels.
	 */
	RETERR(uint8_tobuffer(sig->labels, target));

	/*
	 * Original TTL.
	 */
	RETERR(uint32_tobuffer(sig->originalttl, target));

	/*
	 * Expire time.
	 */
	RETERR(uint32_tobuffer(sig->timeexpire, target));

	/*
	 * Time signed.
	 */
	RETERR(uint32_tobuffer(sig->timesigned, target));

	/*
	 * Key ID.
	 */
	RETERR(uint16_tobuffer(sig->keyid, target));

	/*
	 * Signer name.
	 */
	RETERR(name_tobuffer(&sig->signer, target));

	/*
	 * Signature.
	 */
	return (mem_tobuffer(target, sig->signature, sig->siglen));
}

static inline isc_result_t
tostruct_rrsig(ARGS_TOSTRUCT) {
	isc_region_t sr;
	dns_rdata_rrsig_t *sig = target;
	dns_name_t signer;

	REQUIRE(rdata->type == 46);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	sig->common.rdclass = rdata->rdclass;
	sig->common.rdtype = rdata->type;
	ISC_LINK_INIT(&sig->common, link);

	dns_rdata_toregion(rdata, &sr);

	/*
	 * Type covered.
	 */
	sig->covered = uint16_fromregion(&sr);
	isc_region_consume(&sr, 2);

	/*
	 * Algorithm.
	 */
	sig->algorithm = uint8_fromregion(&sr);
	isc_region_consume(&sr, 1);

	/*
	 * Labels.
	 */
	sig->labels = uint8_fromregion(&sr);
	isc_region_consume(&sr, 1);

	/*
	 * Original TTL.
	 */
	sig->originalttl = uint32_fromregion(&sr);
	isc_region_consume(&sr, 4);

	/*
	 * Expire time.
	 */
	sig->timeexpire = uint32_fromregion(&sr);
	isc_region_consume(&sr, 4);

	/*
	 * Time signed.
	 */
	sig->timesigned = uint32_fromregion(&sr);
	isc_region_consume(&sr, 4);

	/*
	 * Key ID.
	 */
	sig->keyid = uint16_fromregion(&sr);
	isc_region_consume(&sr, 2);

	dns_name_init(&signer, NULL);
	dns_name_fromregion(&signer, &sr);
	dns_name_init(&sig->signer, NULL);
	RETERR(name_duporclone(&signer, mctx, &sig->signer));
	isc_region_consume(&sr, name_length(&sig->signer));

	/*
	 * Signature.
	 */
	sig->siglen = sr.length;
	sig->signature = mem_maybedup(mctx, sr.base, sig->siglen);
	if (sig->signature == NULL)
		goto cleanup;


	sig->mctx = mctx;
	return (ISC_R_SUCCESS);

 cleanup:
	if (mctx != NULL)
		dns_name_free(&sig->signer, mctx);
	return (ISC_R_NOMEMORY);
}

static inline void
freestruct_rrsig(ARGS_FREESTRUCT) {
	dns_rdata_rrsig_t *sig = (dns_rdata_rrsig_t *) source;

	REQUIRE(source != NULL);
	REQUIRE(sig->common.rdtype == 46);

	if (sig->mctx == NULL)
		return;

	dns_name_free(&sig->signer, sig->mctx);
	if (sig->signature != NULL)
		isc_mem_free(sig->mctx, sig->signature);
	sig->mctx = NULL;
}

static inline isc_result_t
additionaldata_rrsig(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == 46);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_rrsig(ARGS_DIGEST) {

	REQUIRE(rdata->type == 46);

	UNUSED(rdata);
	UNUSED(digest);
	UNUSED(arg);

	return (ISC_R_NOTIMPLEMENTED);
}

static inline dns_rdatatype_t
covers_rrsig(dns_rdata_t *rdata) {
	dns_rdatatype_t type;
	isc_region_t r;

	REQUIRE(rdata->type == 46);

	dns_rdata_toregion(rdata, &r);
	type = uint16_fromregion(&r);

	return (type);
}

static inline isc_boolean_t
checkowner_rrsig(ARGS_CHECKOWNER) {

	REQUIRE(type == 46);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_rrsig(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == 46);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

#endif	/* RDATA_GENERIC_RRSIG_46_C */
