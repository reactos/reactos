/*
 * Copyright (C) 2006, 2007  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: dhcid_49.c,v 1.5 2007/06/19 23:47:17 tbox Exp $ */

/* RFC 4701 */

#ifndef RDATA_IN_1_DHCID_49_C
#define RDATA_IN_1_DHCID_49_C 1

#define RRTYPE_DHCID_ATTRIBUTES 0

static inline isc_result_t
fromtext_in_dhcid(ARGS_FROMTEXT) {

	REQUIRE(type == 49);
	REQUIRE(rdclass == 1);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(callbacks);

	return (isc_base64_tobuffer(lexer, target, -1));
}

static inline isc_result_t
totext_in_dhcid(ARGS_TOTEXT) {
	isc_region_t sr;
	char buf[sizeof(" ; 64000 255 64000")];
	size_t n;

	REQUIRE(rdata->type == 49);
	REQUIRE(rdata->rdclass == 1);
	REQUIRE(rdata->length != 0);

	dns_rdata_toregion(rdata, &sr);

	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0)
		RETERR(str_totext("( " /*)*/, target)); 
	RETERR(isc_base64_totext(&sr, tctx->width - 2, tctx->linebreak,
				 target));
	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0) {
		RETERR(str_totext(/* ( */ " )", target));
		if (rdata->length > 2) {
			n = snprintf(buf, sizeof(buf), " ; %u %u %u",
				     sr.base[0] * 256 + sr.base[1],
				     sr.base[2], rdata->length - 3);
			INSIST(n < sizeof(buf));
			RETERR(str_totext(buf, target));
		}
	}
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
fromwire_in_dhcid(ARGS_FROMWIRE) {
	isc_region_t sr;

	REQUIRE(type == 49);
	REQUIRE(rdclass == 1);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(dctx);
	UNUSED(options);

	isc_buffer_activeregion(source, &sr);
	if (sr.length == 0)
		return (ISC_R_UNEXPECTEDEND);

	isc_buffer_forward(source, sr.length);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static inline isc_result_t
towire_in_dhcid(ARGS_TOWIRE) {
	isc_region_t sr;

	REQUIRE(rdata->type == 49);
	REQUIRE(rdata->rdclass == 1);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	dns_rdata_toregion(rdata, &sr);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static inline int
compare_in_dhcid(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 49);
	REQUIRE(rdata1->rdclass == 1);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_in_dhcid(ARGS_FROMSTRUCT) {
	dns_rdata_in_dhcid_t *dhcid = source;

	REQUIRE(type == 49);
	REQUIRE(rdclass == 1);
	REQUIRE(source != NULL);
	REQUIRE(dhcid->common.rdtype == type);
	REQUIRE(dhcid->common.rdclass == rdclass);
	REQUIRE(dhcid->length != 0);

	UNUSED(type);
	UNUSED(rdclass);

	return (mem_tobuffer(target, dhcid->dhcid, dhcid->length));
}

static inline isc_result_t
tostruct_in_dhcid(ARGS_TOSTRUCT) {
	dns_rdata_in_dhcid_t *dhcid = target;
	isc_region_t region;

	REQUIRE(rdata->type == 49);
	REQUIRE(rdata->rdclass == 1);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	dhcid->common.rdclass = rdata->rdclass;
	dhcid->common.rdtype = rdata->type;
	ISC_LINK_INIT(&dhcid->common, link);

	dns_rdata_toregion(rdata, &region);

	dhcid->dhcid = mem_maybedup(mctx, region.base, region.length);
	if (dhcid->dhcid == NULL)
		return (ISC_R_NOMEMORY);

	dhcid->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static inline void
freestruct_in_dhcid(ARGS_FREESTRUCT) {
	dns_rdata_in_dhcid_t *dhcid = source;

	REQUIRE(dhcid != NULL);
	REQUIRE(dhcid->common.rdtype == 49);
	REQUIRE(dhcid->common.rdclass == 1);

	if (dhcid->mctx == NULL)
		return;

	if (dhcid->dhcid != NULL)
		isc_mem_free(dhcid->mctx, dhcid->dhcid);
	dhcid->mctx = NULL;
}

static inline isc_result_t
additionaldata_in_dhcid(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == 49);
	REQUIRE(rdata->rdclass == 1);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_in_dhcid(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == 49);
	REQUIRE(rdata->rdclass == 1);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_in_dhcid(ARGS_CHECKOWNER) {

	REQUIRE(type == 49);
	REQUIRE(rdclass == 1);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_in_dhcid(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == 49);
	REQUIRE(rdata->rdclass == 1);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

#endif	/* RDATA_IN_1_DHCID_49_C */
