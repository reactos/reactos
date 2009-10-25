/*
 * Copyright (C) 2004, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1998-2001  Internet Software Consortium.
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

/* $Id: mb_7.c,v 1.45 2007/06/19 23:47:17 tbox Exp $ */

/* Reviewed: Wed Mar 15 17:31:26 PST 2000 by bwelling */

#ifndef RDATA_GENERIC_MB_7_C
#define RDATA_GENERIC_MB_7_C

#define RRTYPE_MB_ATTRIBUTES (0)

static inline isc_result_t
fromtext_mb(ARGS_FROMTEXT) {
	isc_token_t token;
	dns_name_t name;
	isc_buffer_t buffer;

	REQUIRE(type == 7);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(callbacks);

	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));

	dns_name_init(&name, NULL);
	buffer_fromregion(&buffer, &token.value.as_region);
	origin = (origin != NULL) ? origin : dns_rootname;
	RETTOK(dns_name_fromtext(&name, &buffer, origin, options, target));
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
totext_mb(ARGS_TOTEXT) {
	isc_region_t region;
	dns_name_t name;
	dns_name_t prefix;
	isc_boolean_t sub;

	REQUIRE(rdata->type == 7);
	REQUIRE(rdata->length != 0);

	dns_name_init(&name, NULL);
	dns_name_init(&prefix, NULL);

	dns_rdata_toregion(rdata, &region);
	dns_name_fromregion(&name, &region);

	sub = name_prefix(&name, tctx->origin, &prefix);

	return (dns_name_totext(&prefix, sub, target));
}

static inline isc_result_t
fromwire_mb(ARGS_FROMWIRE) {
        dns_name_t name;

	REQUIRE(type == 7);

	UNUSED(type);
	UNUSED(rdclass);

	dns_decompress_setmethods(dctx, DNS_COMPRESS_GLOBAL14);

        dns_name_init(&name, NULL);
        return (dns_name_fromwire(&name, source, dctx, options, target));
}

static inline isc_result_t
towire_mb(ARGS_TOWIRE) {
	dns_name_t name;
	dns_offsets_t offsets;
	isc_region_t region;

	REQUIRE(rdata->type == 7);
	REQUIRE(rdata->length != 0);

	dns_compress_setmethods(cctx, DNS_COMPRESS_GLOBAL14);

	dns_name_init(&name, offsets);
	dns_rdata_toregion(rdata, &region);
	dns_name_fromregion(&name, &region);

	return (dns_name_towire(&name, cctx, target));
}

static inline int
compare_mb(ARGS_COMPARE) {
	dns_name_t name1;
	dns_name_t name2;
	isc_region_t region1;
	isc_region_t region2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 7);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_name_init(&name1, NULL);
	dns_name_init(&name2, NULL);

	dns_rdata_toregion(rdata1, &region1);
	dns_rdata_toregion(rdata2, &region2);

	dns_name_fromregion(&name1, &region1);
	dns_name_fromregion(&name2, &region2);

	return (dns_name_rdatacompare(&name1, &name2));
}

static inline isc_result_t
fromstruct_mb(ARGS_FROMSTRUCT) {
	dns_rdata_mb_t *mb = source;
	isc_region_t region;

	REQUIRE(type == 7);
	REQUIRE(source != NULL);
	REQUIRE(mb->common.rdtype == type);
	REQUIRE(mb->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	dns_name_toregion(&mb->mb, &region);
	return (isc_buffer_copyregion(target, &region));
}

static inline isc_result_t
tostruct_mb(ARGS_TOSTRUCT) {
	isc_region_t region;
	dns_rdata_mb_t *mb = target;
	dns_name_t name;

	REQUIRE(rdata->type == 7);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	mb->common.rdclass = rdata->rdclass;
	mb->common.rdtype = rdata->type;
	ISC_LINK_INIT(&mb->common, link);

	dns_name_init(&name, NULL);
	dns_rdata_toregion(rdata, &region);
	dns_name_fromregion(&name, &region);
	dns_name_init(&mb->mb, NULL);
	RETERR(name_duporclone(&name, mctx, &mb->mb));
	mb->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static inline void
freestruct_mb(ARGS_FREESTRUCT) {
	dns_rdata_mb_t *mb = source;

	REQUIRE(source != NULL);

	if (mb->mctx == NULL)
		return;

	dns_name_free(&mb->mb, mb->mctx);
	mb->mctx = NULL;
}

static inline isc_result_t
additionaldata_mb(ARGS_ADDLDATA) {
	dns_name_t name;
	dns_offsets_t offsets;
	isc_region_t region;

	REQUIRE(rdata->type == 7);

	dns_name_init(&name, offsets);
	dns_rdata_toregion(rdata, &region);
	dns_name_fromregion(&name, &region);

	return ((add)(arg, &name, dns_rdatatype_a));
}

static inline isc_result_t
digest_mb(ARGS_DIGEST) {
	isc_region_t r;
	dns_name_t name;

	REQUIRE(rdata->type == 7);

	dns_rdata_toregion(rdata, &r);
	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &r);

	return (dns_name_digest(&name, digest, arg));
}

static inline isc_boolean_t
checkowner_mb(ARGS_CHECKOWNER) {

	REQUIRE(type == 7);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (dns_name_ismailbox(name));
}

static inline isc_boolean_t
checknames_mb(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == 7);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

#endif	/* RDATA_GENERIC_MB_7_C */
