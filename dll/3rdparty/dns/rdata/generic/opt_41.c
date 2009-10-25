/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1998-2002  Internet Software Consortium.
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

/* $Id: opt_41.c,v 1.33 2007/06/19 23:47:17 tbox Exp $ */

/* Reviewed: Thu Mar 16 14:06:44 PST 2000 by gson */

/* RFC2671 */

#ifndef RDATA_GENERIC_OPT_41_C
#define RDATA_GENERIC_OPT_41_C

#define RRTYPE_OPT_ATTRIBUTES (DNS_RDATATYPEATTR_SINGLETON | \
			       DNS_RDATATYPEATTR_META | \
			       DNS_RDATATYPEATTR_NOTQUESTION)

static inline isc_result_t
fromtext_opt(ARGS_FROMTEXT) {
	/*
	 * OPT records do not have a text format.
	 */

	REQUIRE(type == 41);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(lexer);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(target);
	UNUSED(callbacks);

	return (ISC_R_NOTIMPLEMENTED);
}

static inline isc_result_t
totext_opt(ARGS_TOTEXT) {
	isc_region_t r;
	isc_region_t or;
	isc_uint16_t option;
	isc_uint16_t length;
	char buf[sizeof("64000 64000")];

	/*
	 * OPT records do not have a text format.
	 */

	REQUIRE(rdata->type == 41);

	dns_rdata_toregion(rdata, &r);
	while (r.length > 0) {
		option = uint16_fromregion(&r);
		isc_region_consume(&r, 2);
		length = uint16_fromregion(&r);
		isc_region_consume(&r, 2);
		sprintf(buf, "%u %u", option, length);
		RETERR(str_totext(buf, target));
		INSIST(r.length >= length);
		if (length > 0) {
			if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0)
				RETERR(str_totext(" (", target));
			RETERR(str_totext(tctx->linebreak, target));
			or = r;
			or.length = length;
			RETERR(isc_base64_totext(&or, tctx->width - 2,
						 tctx->linebreak, target));
			isc_region_consume(&r, length);
			if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0)
				RETERR(str_totext(" )", target));
		}
		if (r.length > 0)
			RETERR(str_totext(" ", target));
	}

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
fromwire_opt(ARGS_FROMWIRE) {
	isc_region_t sregion;
	isc_region_t tregion;
	isc_uint16_t length;
	unsigned int total;

	REQUIRE(type == 41);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(dctx);
	UNUSED(options);

	isc_buffer_activeregion(source, &sregion);
	total = 0;
	while (sregion.length != 0) {
		if (sregion.length < 4)
			return (ISC_R_UNEXPECTEDEND);
		/*
		 * Eat the 16bit option code.  There is nothing to
		 * be done with it currently.
		 */
		isc_region_consume(&sregion, 2);
		length = uint16_fromregion(&sregion);
		isc_region_consume(&sregion, 2);
		total += 4;
		if (sregion.length < length)
			return (ISC_R_UNEXPECTEDEND);
		isc_region_consume(&sregion, length);
		total += length;
	}

	isc_buffer_activeregion(source, &sregion);
	isc_buffer_availableregion(target, &tregion);
	if (tregion.length < total)
		return (ISC_R_NOSPACE);
	memcpy(tregion.base, sregion.base, total);
	isc_buffer_forward(source, total);
	isc_buffer_add(target, total);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
towire_opt(ARGS_TOWIRE) {

	REQUIRE(rdata->type == 41);

	UNUSED(cctx);

	return (mem_tobuffer(target, rdata->data, rdata->length));
}

static inline int
compare_opt(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 41);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_opt(ARGS_FROMSTRUCT) {
	dns_rdata_opt_t *opt = source;
	isc_region_t region;
	isc_uint16_t length;

	REQUIRE(type == 41);
	REQUIRE(source != NULL);
	REQUIRE(opt->common.rdtype == type);
	REQUIRE(opt->common.rdclass == rdclass);
	REQUIRE(opt->options != NULL || opt->length == 0);

	UNUSED(type);
	UNUSED(rdclass);

	region.base = opt->options;
	region.length = opt->length;
	while (region.length >= 4) {
		isc_region_consume(&region, 2);	/* opt */
		length = uint16_fromregion(&region);
		isc_region_consume(&region, 2);
		if (region.length < length)
			return (ISC_R_UNEXPECTEDEND);
		isc_region_consume(&region, length);
	}
	if (region.length != 0)
		return (ISC_R_UNEXPECTEDEND);

	return (mem_tobuffer(target, opt->options, opt->length));
}

static inline isc_result_t
tostruct_opt(ARGS_TOSTRUCT) {
	dns_rdata_opt_t *opt = target;
	isc_region_t r;

	REQUIRE(rdata->type == 41);
	REQUIRE(target != NULL);

	opt->common.rdclass = rdata->rdclass;
	opt->common.rdtype = rdata->type;
	ISC_LINK_INIT(&opt->common, link);

	dns_rdata_toregion(rdata, &r);
	opt->length = r.length;
	opt->options = mem_maybedup(mctx, r.base, r.length);
	if (opt->options == NULL)
		return (ISC_R_NOMEMORY);

	opt->offset = 0;
	opt->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static inline void
freestruct_opt(ARGS_FREESTRUCT) {
	dns_rdata_opt_t *opt = source;

	REQUIRE(source != NULL);
	REQUIRE(opt->common.rdtype == 41);

	if (opt->mctx == NULL)
		return;

	if (opt->options != NULL)
		isc_mem_free(opt->mctx, opt->options);
	opt->mctx = NULL;
}

static inline isc_result_t
additionaldata_opt(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == 41);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_opt(ARGS_DIGEST) {

	/*
	 * OPT records are not digested.
	 */

	REQUIRE(rdata->type == 41);

	UNUSED(rdata);
	UNUSED(digest);
	UNUSED(arg);

	return (ISC_R_NOTIMPLEMENTED);
}

static inline isc_boolean_t
checkowner_opt(ARGS_CHECKOWNER) {

	REQUIRE(type == 41);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (dns_name_equal(name, dns_rootname));
}

static inline isc_boolean_t
checknames_opt(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == 41);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

#endif	/* RDATA_GENERIC_OPT_41_C */
