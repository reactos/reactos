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

/* $Id: x25_19.c,v 1.39 2007/06/19 23:47:17 tbox Exp $ */

/* Reviewed: Thu Mar 16 16:15:57 PST 2000 by bwelling */

/* RFC1183 */

#ifndef RDATA_GENERIC_X25_19_C
#define RDATA_GENERIC_X25_19_C

#define RRTYPE_X25_ATTRIBUTES (0)

static inline isc_result_t
fromtext_x25(ARGS_FROMTEXT) {
	isc_token_t token;
	unsigned int i;

	REQUIRE(type == 19);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(callbacks);

	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_qstring,
				      ISC_FALSE));
	if (token.value.as_textregion.length < 4)
		RETTOK(DNS_R_SYNTAX);
	for (i = 0; i < token.value.as_textregion.length; i++)
		if (!isdigit(token.value.as_textregion.base[i] & 0xff))
			RETTOK(ISC_R_RANGE);
	RETTOK(txt_fromtext(&token.value.as_textregion, target));
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
totext_x25(ARGS_TOTEXT) {
	isc_region_t region;

	UNUSED(tctx);

	REQUIRE(rdata->type == 19);
	REQUIRE(rdata->length != 0);

	dns_rdata_toregion(rdata, &region);
	return (txt_totext(&region, target));
}

static inline isc_result_t
fromwire_x25(ARGS_FROMWIRE) {
	isc_region_t sr;

	REQUIRE(type == 19);

	UNUSED(type);
	UNUSED(dctx);
	UNUSED(rdclass);
	UNUSED(options);

	isc_buffer_activeregion(source, &sr);
	if (sr.length < 5)
		return (DNS_R_FORMERR);
	return (txt_fromwire(source, target));
}

static inline isc_result_t
towire_x25(ARGS_TOWIRE) {
	UNUSED(cctx);

	REQUIRE(rdata->type == 19);
	REQUIRE(rdata->length != 0);

	return (mem_tobuffer(target, rdata->data, rdata->length));
}

static inline int
compare_x25(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 19);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_x25(ARGS_FROMSTRUCT) {
	dns_rdata_x25_t *x25 = source;
	isc_uint8_t i;

	REQUIRE(type == 19);
	REQUIRE(source != NULL);
	REQUIRE(x25->common.rdtype == type);
	REQUIRE(x25->common.rdclass == rdclass);
	REQUIRE(x25->x25 != NULL && x25->x25_len != 0);

	UNUSED(type);
	UNUSED(rdclass);

	if (x25->x25_len < 4)
		return (ISC_R_RANGE);

	for (i = 0; i < x25->x25_len; i++)
		if (!isdigit(x25->x25[i] & 0xff))
			return (ISC_R_RANGE);

	RETERR(uint8_tobuffer(x25->x25_len, target));
	return (mem_tobuffer(target, x25->x25, x25->x25_len));
}

static inline isc_result_t
tostruct_x25(ARGS_TOSTRUCT) {
	dns_rdata_x25_t *x25 = target;
	isc_region_t r;

	REQUIRE(rdata->type == 19);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	x25->common.rdclass = rdata->rdclass;
	x25->common.rdtype = rdata->type;
	ISC_LINK_INIT(&x25->common, link);

	dns_rdata_toregion(rdata, &r);
	x25->x25_len = uint8_fromregion(&r);
	isc_region_consume(&r, 1);
	x25->x25 = mem_maybedup(mctx, r.base, x25->x25_len);
	if (x25->x25 == NULL)
		return (ISC_R_NOMEMORY);

	x25->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static inline void
freestruct_x25(ARGS_FREESTRUCT) {
	dns_rdata_x25_t *x25 = source;
	REQUIRE(source != NULL);
	REQUIRE(x25->common.rdtype == 19);

	if (x25->mctx == NULL)
		return;

	if (x25->x25 != NULL)
		isc_mem_free(x25->mctx, x25->x25);
	x25->mctx = NULL;
}

static inline isc_result_t
additionaldata_x25(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == 19);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_x25(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == 19);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_x25(ARGS_CHECKOWNER) {

	REQUIRE(type == 19);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_x25(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == 19);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

#endif	/* RDATA_GENERIC_X25_19_C */
