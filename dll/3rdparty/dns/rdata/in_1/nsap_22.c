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

/* $Id: nsap_22.c,v 1.42 2007/06/19 23:47:17 tbox Exp $ */

/* Reviewed: Fri Mar 17 10:41:07 PST 2000 by gson */

/* RFC1706 */

#ifndef RDATA_IN_1_NSAP_22_C
#define RDATA_IN_1_NSAP_22_C

#define RRTYPE_NSAP_ATTRIBUTES (0)

static inline isc_result_t
fromtext_in_nsap(ARGS_FROMTEXT) {
	isc_token_t token;
	isc_textregion_t *sr;
	int n;
	int digits;
	unsigned char c = 0;

	REQUIRE(type == 22);
	REQUIRE(rdclass == 1);

	UNUSED(type);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(rdclass);
	UNUSED(callbacks);

	/* 0x<hex.string.with.periods> */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	sr = &token.value.as_textregion;
	if (sr->length < 2)
		RETTOK(ISC_R_UNEXPECTEDEND);
	if (sr->base[0] != '0' || (sr->base[1] != 'x' && sr->base[1] != 'X'))
		RETTOK(DNS_R_SYNTAX);
	isc_textregion_consume(sr, 2);
	digits = 0;
	n = 0;
	while (sr->length > 0) {
		if (sr->base[0] == '.') {
			isc_textregion_consume(sr, 1);
			continue;
		}
		if ((n = hexvalue(sr->base[0])) == -1)
			RETTOK(DNS_R_SYNTAX);
		c <<= 4;
		c += n;
		if (++digits == 2) {
			RETERR(mem_tobuffer(target, &c, 1));
			digits = 0;
		}
		isc_textregion_consume(sr, 1);
	}
	if (digits)
		RETTOK(ISC_R_UNEXPECTEDEND);
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
totext_in_nsap(ARGS_TOTEXT) {
	isc_region_t region;
	char buf[sizeof("xx")];

	REQUIRE(rdata->type == 22);
	REQUIRE(rdata->rdclass == 1);
	REQUIRE(rdata->length != 0);

	UNUSED(tctx);

	dns_rdata_toregion(rdata, &region);
	RETERR(str_totext("0x", target));
	while (region.length != 0) {
		sprintf(buf, "%02x", region.base[0]);
		isc_region_consume(&region, 1);
		RETERR(str_totext(buf, target));
	}
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
fromwire_in_nsap(ARGS_FROMWIRE) {
	isc_region_t region;

	REQUIRE(type == 22);
	REQUIRE(rdclass == 1);

	UNUSED(type);
	UNUSED(dctx);
	UNUSED(options);
	UNUSED(rdclass);

	isc_buffer_activeregion(source, &region);
	if (region.length < 1)
		return (ISC_R_UNEXPECTEDEND);

	RETERR(mem_tobuffer(target, region.base, region.length));
	isc_buffer_forward(source, region.length);
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
towire_in_nsap(ARGS_TOWIRE) {
	REQUIRE(rdata->type == 22);
	REQUIRE(rdata->rdclass == 1);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	return (mem_tobuffer(target, rdata->data, rdata->length));
}

static inline int
compare_in_nsap(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 22);
	REQUIRE(rdata1->rdclass == 1);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_in_nsap(ARGS_FROMSTRUCT) {
	dns_rdata_in_nsap_t *nsap = source;

	REQUIRE(type == 22);
	REQUIRE(rdclass == 1);
	REQUIRE(source != NULL);
	REQUIRE(nsap->common.rdtype == type);
	REQUIRE(nsap->common.rdclass == rdclass);
	REQUIRE(nsap->nsap != NULL || nsap->nsap_len == 0);

	UNUSED(type);
	UNUSED(rdclass);

	return (mem_tobuffer(target, nsap->nsap, nsap->nsap_len));
}

static inline isc_result_t
tostruct_in_nsap(ARGS_TOSTRUCT) {
	dns_rdata_in_nsap_t *nsap = target;
	isc_region_t r;

	REQUIRE(rdata->type == 22);
	REQUIRE(rdata->rdclass == 1);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	nsap->common.rdclass = rdata->rdclass;
	nsap->common.rdtype = rdata->type;
	ISC_LINK_INIT(&nsap->common, link);

	dns_rdata_toregion(rdata, &r);
	nsap->nsap_len = r.length;
	nsap->nsap = mem_maybedup(mctx, r.base, r.length);
	if (nsap->nsap == NULL)
		return (ISC_R_NOMEMORY);

	nsap->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static inline void
freestruct_in_nsap(ARGS_FREESTRUCT) {
	dns_rdata_in_nsap_t *nsap = source;

	REQUIRE(source != NULL);
	REQUIRE(nsap->common.rdclass == 1);
	REQUIRE(nsap->common.rdtype == 22);

	if (nsap->mctx == NULL)
		return;

	if (nsap->nsap != NULL)
		isc_mem_free(nsap->mctx, nsap->nsap);
	nsap->mctx = NULL;
}

static inline isc_result_t
additionaldata_in_nsap(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == 22);
	REQUIRE(rdata->rdclass == 1);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_in_nsap(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == 22);
	REQUIRE(rdata->rdclass == 1);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_in_nsap(ARGS_CHECKOWNER) {

	REQUIRE(type == 22);
	REQUIRE(rdclass == 1);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_in_nsap(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == 22);
	REQUIRE(rdata->rdclass == 1);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

#endif	/* RDATA_IN_1_NSAP_22_C */
