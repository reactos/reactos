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

/* $Id: minfo_14.c,v 1.45 2007/06/19 23:47:17 tbox Exp $ */

/* reviewed: Wed Mar 15 17:45:32 PST 2000 by brister */

#ifndef RDATA_GENERIC_MINFO_14_C
#define RDATA_GENERIC_MINFO_14_C

#define RRTYPE_MINFO_ATTRIBUTES (0)

static inline isc_result_t
fromtext_minfo(ARGS_FROMTEXT) {
	isc_token_t token;
	dns_name_t name;
	isc_buffer_t buffer;
	int i;
	isc_boolean_t ok;

	REQUIRE(type == 14);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(callbacks);

	for (i = 0; i < 2; i++) {
		RETERR(isc_lex_getmastertoken(lexer, &token,
					      isc_tokentype_string,
					      ISC_FALSE));
		dns_name_init(&name, NULL);
		buffer_fromregion(&buffer, &token.value.as_region);
		origin = (origin != NULL) ? origin : dns_rootname;
		RETTOK(dns_name_fromtext(&name, &buffer, origin,
					 options, target));
		ok = ISC_TRUE;
		if ((options & DNS_RDATA_CHECKNAMES) != 0)
			ok = dns_name_ismailbox(&name);
		if (!ok && (options & DNS_RDATA_CHECKNAMESFAIL) != 0)
			RETTOK(DNS_R_BADNAME);
		if (!ok && callbacks != NULL)
			warn_badname(&name, lexer, callbacks);
	}
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
totext_minfo(ARGS_TOTEXT) {
	isc_region_t region;
	dns_name_t rmail;
	dns_name_t email;
	dns_name_t prefix;
	isc_boolean_t sub;

	REQUIRE(rdata->type == 14);
	REQUIRE(rdata->length != 0);

	dns_name_init(&rmail, NULL);
	dns_name_init(&email, NULL);
	dns_name_init(&prefix, NULL);

	dns_rdata_toregion(rdata, &region);

	dns_name_fromregion(&rmail, &region);
	isc_region_consume(&region, rmail.length);

	dns_name_fromregion(&email, &region);
	isc_region_consume(&region, email.length);

	sub = name_prefix(&rmail, tctx->origin, &prefix);

	RETERR(dns_name_totext(&prefix, sub, target));

	RETERR(str_totext(" ", target));

	sub = name_prefix(&email, tctx->origin, &prefix);
	return (dns_name_totext(&prefix, sub, target));
}

static inline isc_result_t
fromwire_minfo(ARGS_FROMWIRE) {
        dns_name_t rmail;
        dns_name_t email;

	REQUIRE(type == 14);

	UNUSED(type);
	UNUSED(rdclass);

	dns_decompress_setmethods(dctx, DNS_COMPRESS_GLOBAL14);

        dns_name_init(&rmail, NULL);
        dns_name_init(&email, NULL);

        RETERR(dns_name_fromwire(&rmail, source, dctx, options, target));
        return (dns_name_fromwire(&email, source, dctx, options, target));
}

static inline isc_result_t
towire_minfo(ARGS_TOWIRE) {
	isc_region_t region;
	dns_name_t rmail;
	dns_name_t email;
	dns_offsets_t roffsets;
	dns_offsets_t eoffsets;

	REQUIRE(rdata->type == 14);
	REQUIRE(rdata->length != 0);

	dns_compress_setmethods(cctx, DNS_COMPRESS_GLOBAL14);

	dns_name_init(&rmail, roffsets);
	dns_name_init(&email, eoffsets);

	dns_rdata_toregion(rdata, &region);

	dns_name_fromregion(&rmail, &region);
	isc_region_consume(&region, name_length(&rmail));

	RETERR(dns_name_towire(&rmail, cctx, target));

	dns_name_fromregion(&rmail, &region);
	isc_region_consume(&region, rmail.length);

	return (dns_name_towire(&rmail, cctx, target));
}

static inline int
compare_minfo(ARGS_COMPARE) {
	isc_region_t region1;
	isc_region_t region2;
	dns_name_t name1;
	dns_name_t name2;
	int order;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 14);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_name_init(&name1, NULL);
	dns_name_init(&name2, NULL);

	dns_rdata_toregion(rdata1, &region1);
	dns_rdata_toregion(rdata2, &region2);

	dns_name_fromregion(&name1, &region1);
	dns_name_fromregion(&name2, &region2);

	order = dns_name_rdatacompare(&name1, &name2);
	if (order != 0)
		return (order);

	isc_region_consume(&region1, name_length(&name1));
	isc_region_consume(&region2, name_length(&name2));

	dns_name_init(&name1, NULL);
	dns_name_init(&name2, NULL);

	dns_name_fromregion(&name1, &region1);
	dns_name_fromregion(&name2, &region2);

	order = dns_name_rdatacompare(&name1, &name2);
	return (order);
}

static inline isc_result_t
fromstruct_minfo(ARGS_FROMSTRUCT) {
	dns_rdata_minfo_t *minfo = source;
	isc_region_t region;

	REQUIRE(type == 14);
	REQUIRE(source != NULL);
	REQUIRE(minfo->common.rdtype == type);
	REQUIRE(minfo->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	dns_name_toregion(&minfo->rmailbox, &region);
	RETERR(isc_buffer_copyregion(target, &region));
	dns_name_toregion(&minfo->emailbox, &region);
	return (isc_buffer_copyregion(target, &region));
}

static inline isc_result_t
tostruct_minfo(ARGS_TOSTRUCT) {
	dns_rdata_minfo_t *minfo = target;
	isc_region_t region;
	dns_name_t name;
	isc_result_t result;

	REQUIRE(rdata->type == 14);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	minfo->common.rdclass = rdata->rdclass;
	minfo->common.rdtype = rdata->type;
	ISC_LINK_INIT(&minfo->common, link);

	dns_name_init(&name, NULL);
	dns_rdata_toregion(rdata, &region);
	dns_name_fromregion(&name, &region);
	dns_name_init(&minfo->rmailbox, NULL);
	RETERR(name_duporclone(&name, mctx, &minfo->rmailbox));
	isc_region_consume(&region, name_length(&name));

	dns_name_fromregion(&name, &region);
	dns_name_init(&minfo->emailbox, NULL);
	result = name_duporclone(&name, mctx, &minfo->emailbox);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	minfo->mctx = mctx;
	return (ISC_R_SUCCESS);

 cleanup:
	if (mctx != NULL)
		dns_name_free(&minfo->rmailbox, mctx);
	return (ISC_R_NOMEMORY);
}

static inline void
freestruct_minfo(ARGS_FREESTRUCT) {
	dns_rdata_minfo_t *minfo = source;

	REQUIRE(source != NULL);
	REQUIRE(minfo->common.rdtype == 14);

	if (minfo->mctx == NULL)
		return;

	dns_name_free(&minfo->rmailbox, minfo->mctx);
	dns_name_free(&minfo->emailbox, minfo->mctx);
	minfo->mctx = NULL;
}

static inline isc_result_t
additionaldata_minfo(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == 14);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_minfo(ARGS_DIGEST) {
	isc_region_t r;
	dns_name_t name;
	isc_result_t result;

	REQUIRE(rdata->type == 14);

	dns_rdata_toregion(rdata, &r);
	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &r);
	result = dns_name_digest(&name, digest, arg);
	if (result != ISC_R_SUCCESS)
		return (result);
	isc_region_consume(&r, name_length(&name));
	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &r);

	return (dns_name_digest(&name, digest, arg));
}

static inline isc_boolean_t
checkowner_minfo(ARGS_CHECKOWNER) {

	REQUIRE(type == 14);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_minfo(ARGS_CHECKNAMES) {
	isc_region_t region;
	dns_name_t name;

	REQUIRE(rdata->type == 14);

	UNUSED(owner);

	dns_rdata_toregion(rdata, &region);
	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &region);
	if (!dns_name_ismailbox(&name)) {
		if (bad != NULL)
			dns_name_clone(&name, bad);
		return (ISC_FALSE);
	}
	isc_region_consume(&region, name_length(&name));
	dns_name_fromregion(&name, &region);
	if (!dns_name_ismailbox(&name)) {
		if (bad != NULL)
			dns_name_clone(&name, bad);
		return (ISC_FALSE);
	}
	return (ISC_TRUE);
}

#endif	/* RDATA_GENERIC_MINFO_14_C */
