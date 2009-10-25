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

/* $Id: kx_36.c,v 1.45 2007/06/19 23:47:17 tbox Exp $ */

/* Reviewed: Thu Mar 16 17:24:54 PST 2000 by explorer */

/* RFC2230 */

#ifndef RDATA_IN_1_KX_36_C
#define RDATA_IN_1_KX_36_C

#define RRTYPE_KX_ATTRIBUTES (0)

static inline isc_result_t
fromtext_in_kx(ARGS_FROMTEXT) {
	isc_token_t token;
	dns_name_t name;
	isc_buffer_t buffer;

	REQUIRE(type == 36);
	REQUIRE(rdclass == 1);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(callbacks);

	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      ISC_FALSE));
	if (token.value.as_ulong > 0xffffU)
		RETTOK(ISC_R_RANGE);
	RETERR(uint16_tobuffer(token.value.as_ulong, target));

	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	dns_name_init(&name, NULL);
	buffer_fromregion(&buffer, &token.value.as_region);
	origin = (origin != NULL) ? origin : dns_rootname;
	RETTOK(dns_name_fromtext(&name, &buffer, origin, options, target));
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
totext_in_kx(ARGS_TOTEXT) {
	isc_region_t region;
	dns_name_t name;
	dns_name_t prefix;
	isc_boolean_t sub;
	char buf[sizeof("64000")];
	unsigned short num;

	REQUIRE(rdata->type == 36);
	REQUIRE(rdata->rdclass == 1);
	REQUIRE(rdata->length != 0);

	dns_name_init(&name, NULL);
	dns_name_init(&prefix, NULL);

	dns_rdata_toregion(rdata, &region);
	num = uint16_fromregion(&region);
	isc_region_consume(&region, 2);
	sprintf(buf, "%u", num);
	RETERR(str_totext(buf, target));

	RETERR(str_totext(" ", target));

	dns_name_fromregion(&name, &region);
	sub = name_prefix(&name, tctx->origin, &prefix);
	return (dns_name_totext(&prefix, sub, target));
}

static inline isc_result_t
fromwire_in_kx(ARGS_FROMWIRE) {
        dns_name_t name;
	isc_region_t sregion;

	REQUIRE(type == 36);
	REQUIRE(rdclass == 1);

	UNUSED(type);
	UNUSED(rdclass);

	dns_decompress_setmethods(dctx, DNS_COMPRESS_NONE);

        dns_name_init(&name, NULL);

	isc_buffer_activeregion(source, &sregion);
	if (sregion.length < 2)
		return (ISC_R_UNEXPECTEDEND);
	RETERR(mem_tobuffer(target, sregion.base, 2));
	isc_buffer_forward(source, 2);
	return (dns_name_fromwire(&name, source, dctx, options, target));
}

static inline isc_result_t
towire_in_kx(ARGS_TOWIRE) {
	dns_name_t name;
	dns_offsets_t offsets;
	isc_region_t region;

	REQUIRE(rdata->type == 36);
	REQUIRE(rdata->rdclass == 1);
	REQUIRE(rdata->length != 0);

	dns_compress_setmethods(cctx, DNS_COMPRESS_NONE);
	dns_rdata_toregion(rdata, &region);
	RETERR(mem_tobuffer(target, region.base, 2));
	isc_region_consume(&region, 2);

	dns_name_init(&name, offsets);
	dns_name_fromregion(&name, &region);

	return (dns_name_towire(&name, cctx, target));
}

static inline int
compare_in_kx(ARGS_COMPARE) {
	dns_name_t name1;
	dns_name_t name2;
	isc_region_t region1;
	isc_region_t region2;
	int order;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 36);
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

	return (dns_name_rdatacompare(&name1, &name2));
}

static inline isc_result_t
fromstruct_in_kx(ARGS_FROMSTRUCT) {
	dns_rdata_in_kx_t *kx = source;
	isc_region_t region;

	REQUIRE(type == 36);
	REQUIRE(rdclass == 1);
	REQUIRE(source != NULL);
	REQUIRE(kx->common.rdtype == type);
	REQUIRE(kx->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	RETERR(uint16_tobuffer(kx->preference, target));
	dns_name_toregion(&kx->exchange, &region);
	return (isc_buffer_copyregion(target, &region));
}

static inline isc_result_t
tostruct_in_kx(ARGS_TOSTRUCT) {
	isc_region_t region;
	dns_rdata_in_kx_t *kx = target;
	dns_name_t name;

	REQUIRE(rdata->type == 36);
	REQUIRE(rdata->rdclass == 1);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	kx->common.rdclass = rdata->rdclass;
	kx->common.rdtype = rdata->type;
	ISC_LINK_INIT(&kx->common, link);

	dns_name_init(&name, NULL);
	dns_rdata_toregion(rdata, &region);

	kx->preference = uint16_fromregion(&region);
	isc_region_consume(&region, 2);

	dns_name_fromregion(&name, &region);
	dns_name_init(&kx->exchange, NULL);
	RETERR(name_duporclone(&name, mctx, &kx->exchange));
	kx->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static inline void
freestruct_in_kx(ARGS_FREESTRUCT) {
	dns_rdata_in_kx_t *kx = source;

	REQUIRE(source != NULL);
	REQUIRE(kx->common.rdclass == 1);
	REQUIRE(kx->common.rdtype == 36);

	if (kx->mctx == NULL)
		return;

	dns_name_free(&kx->exchange, kx->mctx);
	kx->mctx = NULL;
}

static inline isc_result_t
additionaldata_in_kx(ARGS_ADDLDATA) {
	dns_name_t name;
	dns_offsets_t offsets;
	isc_region_t region;

	REQUIRE(rdata->type == 36);
	REQUIRE(rdata->rdclass == 1);

	dns_name_init(&name, offsets);
	dns_rdata_toregion(rdata, &region);
	isc_region_consume(&region, 2);
	dns_name_fromregion(&name, &region);

	return ((add)(arg, &name, dns_rdatatype_a));
}

static inline isc_result_t
digest_in_kx(ARGS_DIGEST) {
	isc_region_t r1, r2;
	dns_name_t name;

	REQUIRE(rdata->type == 36);
	REQUIRE(rdata->rdclass == 1);

	dns_rdata_toregion(rdata, &r1);
	r2 = r1;
	isc_region_consume(&r2, 2);
	r1.length = 2;
	RETERR((digest)(arg, &r1));
	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &r2);
	return (dns_name_digest(&name, digest, arg));
}

static inline isc_boolean_t
checkowner_in_kx(ARGS_CHECKOWNER) {

	REQUIRE(type == 36);
	REQUIRE(rdclass == 1);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_in_kx(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == 36);
	REQUIRE(rdata->rdclass == 1);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

#endif	/* RDATA_IN_1_KX_36_C */
