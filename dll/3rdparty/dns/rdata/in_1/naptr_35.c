/*
 * Copyright (C) 2004, 2005, 2007, 2008  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: naptr_35.c,v 1.53 2008/02/15 23:46:51 tbox Exp $ */

/* Reviewed: Thu Mar 16 16:52:50 PST 2000 by bwelling */

/* RFC2915 */

#ifndef RDATA_IN_1_NAPTR_35_C
#define RDATA_IN_1_NAPTR_35_C

#define RRTYPE_NAPTR_ATTRIBUTES (0)

static inline isc_result_t
fromtext_in_naptr(ARGS_FROMTEXT) {
	isc_token_t token;
	dns_name_t name;
	isc_buffer_t buffer;

	REQUIRE(type == 35);
	REQUIRE(rdclass == 1);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(callbacks);

	/*
	 * Order.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      ISC_FALSE));
	if (token.value.as_ulong > 0xffffU)
		RETTOK(ISC_R_RANGE);
	RETERR(uint16_tobuffer(token.value.as_ulong, target));

	/*
	 * Preference.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      ISC_FALSE));
	if (token.value.as_ulong > 0xffffU)
		RETTOK(ISC_R_RANGE);
	RETERR(uint16_tobuffer(token.value.as_ulong, target));

	/*
	 * Flags.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_qstring,
				      ISC_FALSE));
	RETTOK(txt_fromtext(&token.value.as_textregion, target));

	/*
	 * Service.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_qstring,
				      ISC_FALSE));
	RETTOK(txt_fromtext(&token.value.as_textregion, target));

	/*
	 * Regexp.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_qstring,
				      ISC_FALSE));
	RETTOK(txt_fromtext(&token.value.as_textregion, target));

	/*
	 * Replacement.
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
totext_in_naptr(ARGS_TOTEXT) {
	isc_region_t region;
	dns_name_t name;
	dns_name_t prefix;
	isc_boolean_t sub;
	char buf[sizeof("64000")];
	unsigned short num;

	REQUIRE(rdata->type == 35);
	REQUIRE(rdata->rdclass == 1);
	REQUIRE(rdata->length != 0);

	dns_name_init(&name, NULL);
	dns_name_init(&prefix, NULL);

	dns_rdata_toregion(rdata, &region);

	/*
	 * Order.
	 */
	num = uint16_fromregion(&region);
	isc_region_consume(&region, 2);
	sprintf(buf, "%u", num);
	RETERR(str_totext(buf, target));
	RETERR(str_totext(" ", target));

	/*
	 * Preference.
	 */
	num = uint16_fromregion(&region);
	isc_region_consume(&region, 2);
	sprintf(buf, "%u", num);
	RETERR(str_totext(buf, target));
	RETERR(str_totext(" ", target));

	/*
	 * Flags.
	 */
	RETERR(txt_totext(&region, target));
	RETERR(str_totext(" ", target));

	/*
	 * Service.
	 */
	RETERR(txt_totext(&region, target));
	RETERR(str_totext(" ", target));

	/*
	 * Regexp.
	 */
	RETERR(txt_totext(&region, target));
	RETERR(str_totext(" ", target));

	/*
	 * Replacement.
	 */
	dns_name_fromregion(&name, &region);
	sub = name_prefix(&name, tctx->origin, &prefix);
	return (dns_name_totext(&prefix, sub, target));
}

static inline isc_result_t
fromwire_in_naptr(ARGS_FROMWIRE) {
	dns_name_t name;
	isc_region_t sr;

	REQUIRE(type == 35);
	REQUIRE(rdclass == 1);

	UNUSED(type);
	UNUSED(rdclass);

	dns_decompress_setmethods(dctx, DNS_COMPRESS_NONE);

	dns_name_init(&name, NULL);

	/*
	 * Order, preference.
	 */
	isc_buffer_activeregion(source, &sr);
	if (sr.length < 4)
		return (ISC_R_UNEXPECTEDEND);
	RETERR(mem_tobuffer(target, sr.base, 4));
	isc_buffer_forward(source, 4);

	/*
	 * Flags.
	 */
	RETERR(txt_fromwire(source, target));

	/*
	 * Service.
	 */
	RETERR(txt_fromwire(source, target));

	/*
	 * Regexp.
	 */
	RETERR(txt_fromwire(source, target));

	/*
	 * Replacement.
	 */
	return (dns_name_fromwire(&name, source, dctx, options, target));
}

static inline isc_result_t
towire_in_naptr(ARGS_TOWIRE) {
	dns_name_t name;
	dns_offsets_t offsets;
	isc_region_t sr;

	REQUIRE(rdata->type == 35);
	REQUIRE(rdata->rdclass == 1);
	REQUIRE(rdata->length != 0);

	dns_compress_setmethods(cctx, DNS_COMPRESS_NONE);
	/*
	 * Order, preference.
	 */
	dns_rdata_toregion(rdata, &sr);
	RETERR(mem_tobuffer(target, sr.base, 4));
	isc_region_consume(&sr, 4);

	/*
	 * Flags.
	 */
	RETERR(mem_tobuffer(target, sr.base, sr.base[0] + 1));
	isc_region_consume(&sr, sr.base[0] + 1);

	/*
	 * Service.
	 */
	RETERR(mem_tobuffer(target, sr.base, sr.base[0] + 1));
	isc_region_consume(&sr, sr.base[0] + 1);

	/*
	 * Regexp.
	 */
	RETERR(mem_tobuffer(target, sr.base, sr.base[0] + 1));
	isc_region_consume(&sr, sr.base[0] + 1);

	/*
	 * Replacement.
	 */
	dns_name_init(&name, offsets);
	dns_name_fromregion(&name, &sr);
	return (dns_name_towire(&name, cctx, target));
}

static inline int
compare_in_naptr(ARGS_COMPARE) {
	dns_name_t name1;
	dns_name_t name2;
	isc_region_t region1;
	isc_region_t region2;
	int order, len;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 35);
	REQUIRE(rdata1->rdclass == 1);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &region1);
	dns_rdata_toregion(rdata2, &region2);

	/*
	 * Order, preference.
	 */
	order = memcmp(region1.base, region2.base, 4);
	if (order != 0)
		return (order < 0 ? -1 : 1);
	isc_region_consume(&region1, 4);
	isc_region_consume(&region2, 4);

	/*
	 * Flags.
	 */
	len = ISC_MIN(region1.base[0], region2.base[0]);
	order = memcmp(region1.base, region2.base, len + 1);
	if (order != 0)
		return (order < 0 ? -1 : 1);
	isc_region_consume(&region1, region1.base[0] + 1);
	isc_region_consume(&region2, region2.base[0] + 1);

	/*
	 * Service.
	 */
	len = ISC_MIN(region1.base[0], region2.base[0]);
	order = memcmp(region1.base, region2.base, len + 1);
	if (order != 0)
		return (order < 0 ? -1 : 1);
	isc_region_consume(&region1, region1.base[0] + 1);
	isc_region_consume(&region2, region2.base[0] + 1);

	/*
	 * Regexp.
	 */
	len = ISC_MIN(region1.base[0], region2.base[0]);
	order = memcmp(region1.base, region2.base, len + 1);
	if (order != 0)
		return (order < 0 ? -1 : 1);
	isc_region_consume(&region1, region1.base[0] + 1);
	isc_region_consume(&region2, region2.base[0] + 1);

	/*
	 * Replacement.
	 */
	dns_name_init(&name1, NULL);
	dns_name_init(&name2, NULL);

	dns_name_fromregion(&name1, &region1);
	dns_name_fromregion(&name2, &region2);

	return (dns_name_rdatacompare(&name1, &name2));
}

static inline isc_result_t
fromstruct_in_naptr(ARGS_FROMSTRUCT) {
	dns_rdata_in_naptr_t *naptr = source;
	isc_region_t region;

	REQUIRE(type == 35);
	REQUIRE(rdclass == 1);
	REQUIRE(source != NULL);
	REQUIRE(naptr->common.rdtype == type);
	REQUIRE(naptr->common.rdclass == rdclass);
	REQUIRE(naptr->flags != NULL || naptr->flags_len == 0);
	REQUIRE(naptr->service != NULL || naptr->service_len == 0);
	REQUIRE(naptr->regexp != NULL || naptr->regexp_len == 0);

	UNUSED(type);
	UNUSED(rdclass);

	RETERR(uint16_tobuffer(naptr->order, target));
	RETERR(uint16_tobuffer(naptr->preference, target));
	RETERR(uint8_tobuffer(naptr->flags_len, target));
	RETERR(mem_tobuffer(target, naptr->flags, naptr->flags_len));
	RETERR(uint8_tobuffer(naptr->service_len, target));
	RETERR(mem_tobuffer(target, naptr->service, naptr->service_len));
	RETERR(uint8_tobuffer(naptr->regexp_len, target));
	RETERR(mem_tobuffer(target, naptr->regexp, naptr->regexp_len));
	dns_name_toregion(&naptr->replacement, &region);
	return (isc_buffer_copyregion(target, &region));
}

static inline isc_result_t
tostruct_in_naptr(ARGS_TOSTRUCT) {
	dns_rdata_in_naptr_t *naptr = target;
	isc_region_t r;
	isc_result_t result;
	dns_name_t name;

	REQUIRE(rdata->type == 35);
	REQUIRE(rdata->rdclass == 1);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	naptr->common.rdclass = rdata->rdclass;
	naptr->common.rdtype = rdata->type;
	ISC_LINK_INIT(&naptr->common, link);

	naptr->flags = NULL;
	naptr->service = NULL;
	naptr->regexp = NULL;

	dns_rdata_toregion(rdata, &r);

	naptr->order = uint16_fromregion(&r);
	isc_region_consume(&r, 2);

	naptr->preference = uint16_fromregion(&r);
	isc_region_consume(&r, 2);

	naptr->flags_len = uint8_fromregion(&r);
	isc_region_consume(&r, 1);
	INSIST(naptr->flags_len <= r.length);
	naptr->flags = mem_maybedup(mctx, r.base, naptr->flags_len);
	if (naptr->flags == NULL)
		goto cleanup;
	isc_region_consume(&r, naptr->flags_len);

	naptr->service_len = uint8_fromregion(&r);
	isc_region_consume(&r, 1);
	INSIST(naptr->service_len <= r.length);
	naptr->service = mem_maybedup(mctx, r.base, naptr->service_len);
	if (naptr->service == NULL)
		goto cleanup;
	isc_region_consume(&r, naptr->service_len);

	naptr->regexp_len = uint8_fromregion(&r);
	isc_region_consume(&r, 1);
	INSIST(naptr->regexp_len <= r.length);
	naptr->regexp = mem_maybedup(mctx, r.base, naptr->regexp_len);
	if (naptr->regexp == NULL)
		goto cleanup;
	isc_region_consume(&r, naptr->regexp_len);

	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &r);
	dns_name_init(&naptr->replacement, NULL);
	result = name_duporclone(&name, mctx, &naptr->replacement);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	naptr->mctx = mctx;
	return (ISC_R_SUCCESS);

 cleanup:
	if (mctx != NULL && naptr->flags != NULL)
		isc_mem_free(mctx, naptr->flags);
	if (mctx != NULL && naptr->service != NULL)
		isc_mem_free(mctx, naptr->service);
	if (mctx != NULL && naptr->regexp != NULL)
		isc_mem_free(mctx, naptr->regexp);
	return (ISC_R_NOMEMORY);
}

static inline void
freestruct_in_naptr(ARGS_FREESTRUCT) {
	dns_rdata_in_naptr_t *naptr = source;

	REQUIRE(source != NULL);
	REQUIRE(naptr->common.rdclass == 1);
	REQUIRE(naptr->common.rdtype == 35);

	if (naptr->mctx == NULL)
		return;

	if (naptr->flags != NULL)
		isc_mem_free(naptr->mctx, naptr->flags);
	if (naptr->service != NULL)
		isc_mem_free(naptr->mctx, naptr->service);
	if (naptr->regexp != NULL)
		isc_mem_free(naptr->mctx, naptr->regexp);
	dns_name_free(&naptr->replacement, naptr->mctx);
	naptr->mctx = NULL;
}

static inline isc_result_t
additionaldata_in_naptr(ARGS_ADDLDATA) {
	dns_name_t name;
	dns_offsets_t offsets;
	isc_region_t sr;
	dns_rdatatype_t atype;
	unsigned int i, flagslen;
	char *cp;

	REQUIRE(rdata->type == 35);
	REQUIRE(rdata->rdclass == 1);

	/*
	 * Order, preference.
	 */
	dns_rdata_toregion(rdata, &sr);
	isc_region_consume(&sr, 4);

	/*
	 * Flags.
	 */
	atype = 0;
	flagslen = sr.base[0];
	cp = (char *)&sr.base[1];
	for (i = 0; i < flagslen; i++, cp++) {
		if (*cp == 'S' || *cp == 's') {
			atype = dns_rdatatype_srv;
			break;
		}
		if (*cp == 'A' || *cp == 'a') {
			atype = dns_rdatatype_a;
			break;
		}
	}
	isc_region_consume(&sr, flagslen + 1);

	/*
	 * Service.
	 */
	isc_region_consume(&sr, sr.base[0] + 1);

	/*
	 * Regexp.
	 */
	isc_region_consume(&sr, sr.base[0] + 1);

	/*
	 * Replacement.
	 */
	dns_name_init(&name, offsets);
	dns_name_fromregion(&name, &sr);

	if (atype != 0)
		return ((add)(arg, &name, atype));

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_in_naptr(ARGS_DIGEST) {
	isc_region_t r1, r2;
	unsigned int length, n;
	isc_result_t result;
	dns_name_t name;

	REQUIRE(rdata->type == 35);
	REQUIRE(rdata->rdclass == 1);

	dns_rdata_toregion(rdata, &r1);
	r2 = r1;
	length = 0;

	/*
	 * Order, preference.
	 */
	length += 4;
	isc_region_consume(&r2, 4);

	/*
	 * Flags.
	 */
	n = r2.base[0] + 1;
	length += n;
	isc_region_consume(&r2, n);

	/*
	 * Service.
	 */
	n = r2.base[0] + 1;
	length += n;
	isc_region_consume(&r2, n);

	/*
	 * Regexp.
	 */
	n = r2.base[0] + 1;
	length += n;
	isc_region_consume(&r2, n);

	/*
	 * Digest the RR up to the replacement name.
	 */
	r1.length = length;
	result = (digest)(arg, &r1);
	if (result != ISC_R_SUCCESS)
		return (result);

	/*
	 * Replacement.
	 */

	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &r2);

	return (dns_name_digest(&name, digest, arg));
}

static inline isc_boolean_t
checkowner_in_naptr(ARGS_CHECKOWNER) {

	REQUIRE(type == 35);
	REQUIRE(rdclass == 1);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_in_naptr(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == 35);
	REQUIRE(rdata->rdclass == 1);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

#endif	/* RDATA_IN_1_NAPTR_35_C */
