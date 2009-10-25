/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2001  Internet Software Consortium.
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

/* $Id: rp_17.c,v 1.42 2007/06/19 23:47:17 tbox Exp $ */

/* RFC1183 */

#ifndef RDATA_GENERIC_RP_17_C
#define RDATA_GENERIC_RP_17_C

#define RRTYPE_RP_ATTRIBUTES (0)

static inline isc_result_t
fromtext_rp(ARGS_FROMTEXT) {
	isc_token_t token;
	dns_name_t name;
	isc_buffer_t buffer;
	int i;
	isc_boolean_t ok;

	REQUIRE(type == 17);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(callbacks);

	origin = (origin != NULL) ? origin : dns_rootname;

	for (i = 0; i < 2; i++) {
		RETERR(isc_lex_getmastertoken(lexer, &token,
					      isc_tokentype_string,
					      ISC_FALSE));
		dns_name_init(&name, NULL);
		buffer_fromregion(&buffer, &token.value.as_region);
		RETTOK(dns_name_fromtext(&name, &buffer, origin,
					 options, target));
		ok = ISC_TRUE;
		if ((options & DNS_RDATA_CHECKNAMES) != 0 && i == 0)
			ok = dns_name_ismailbox(&name);
		if (!ok && (options & DNS_RDATA_CHECKNAMESFAIL) != 0)
			RETTOK(DNS_R_BADNAME);
		if (!ok && callbacks != NULL)
			warn_badname(&name, lexer, callbacks);
	}
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
totext_rp(ARGS_TOTEXT) {
	isc_region_t region;
	dns_name_t rmail;
	dns_name_t email;
	dns_name_t prefix;
	isc_boolean_t sub;

	REQUIRE(rdata->type == 17);
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
fromwire_rp(ARGS_FROMWIRE) {
        dns_name_t rmail;
        dns_name_t email;

	REQUIRE(type == 17);

	UNUSED(type);
	UNUSED(rdclass);

	dns_decompress_setmethods(dctx, DNS_COMPRESS_NONE);

        dns_name_init(&rmail, NULL);
        dns_name_init(&email, NULL);

        RETERR(dns_name_fromwire(&rmail, source, dctx, options, target));
        return (dns_name_fromwire(&email, source, dctx, options, target));
}

static inline isc_result_t
towire_rp(ARGS_TOWIRE) {
	isc_region_t region;
	dns_name_t rmail;
	dns_name_t email;
	dns_offsets_t roffsets;
	dns_offsets_t eoffsets;

	REQUIRE(rdata->type == 17);
	REQUIRE(rdata->length != 0);

	dns_compress_setmethods(cctx, DNS_COMPRESS_NONE);
	dns_name_init(&rmail, roffsets);
	dns_name_init(&email, eoffsets);

	dns_rdata_toregion(rdata, &region);

	dns_name_fromregion(&rmail, &region);
	isc_region_consume(&region, rmail.length);

	RETERR(dns_name_towire(&rmail, cctx, target));

	dns_name_fromregion(&rmail, &region);
	isc_region_consume(&region, rmail.length);

	return (dns_name_towire(&rmail, cctx, target));
}

static inline int
compare_rp(ARGS_COMPARE) {
	isc_region_t region1;
	isc_region_t region2;
	dns_name_t name1;
	dns_name_t name2;
	int order;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 17);
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

	return (dns_name_rdatacompare(&name1, &name2));
}

static inline isc_result_t
fromstruct_rp(ARGS_FROMSTRUCT) {
	dns_rdata_rp_t *rp = source;
	isc_region_t region;

	REQUIRE(type == 17);
	REQUIRE(source != NULL);
	REQUIRE(rp->common.rdtype == type);
	REQUIRE(rp->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	dns_name_toregion(&rp->mail, &region);
	RETERR(isc_buffer_copyregion(target, &region));
	dns_name_toregion(&rp->text, &region);
	return (isc_buffer_copyregion(target, &region));
}

static inline isc_result_t
tostruct_rp(ARGS_TOSTRUCT) {
	isc_result_t result;
	isc_region_t region;
	dns_rdata_rp_t *rp = target;
	dns_name_t name;

	REQUIRE(rdata->type == 17);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	rp->common.rdclass = rdata->rdclass;
	rp->common.rdtype = rdata->type;
	ISC_LINK_INIT(&rp->common, link);

	dns_name_init(&name, NULL);
	dns_rdata_toregion(rdata, &region);
	dns_name_fromregion(&name, &region);
	dns_name_init(&rp->mail, NULL);
	RETERR(name_duporclone(&name, mctx, &rp->mail));
	isc_region_consume(&region, name_length(&name));
	dns_name_fromregion(&name, &region);
	dns_name_init(&rp->text, NULL);
	result = name_duporclone(&name, mctx, &rp->text);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	rp->mctx = mctx;
	return (ISC_R_SUCCESS);

 cleanup:
	if (mctx != NULL)
		dns_name_free(&rp->mail, mctx);
	return (ISC_R_NOMEMORY);
}

static inline void
freestruct_rp(ARGS_FREESTRUCT) {
	dns_rdata_rp_t *rp = source;

	REQUIRE(source != NULL);
	REQUIRE(rp->common.rdtype == 17);

	if (rp->mctx == NULL)
		return;

	dns_name_free(&rp->mail, rp->mctx);
	dns_name_free(&rp->text, rp->mctx);
	rp->mctx = NULL;
}

static inline isc_result_t
additionaldata_rp(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == 17);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_rp(ARGS_DIGEST) {
	isc_region_t r;
	dns_name_t name;

	REQUIRE(rdata->type == 17);

	dns_rdata_toregion(rdata, &r);
	dns_name_init(&name, NULL);

	dns_name_fromregion(&name, &r);
	RETERR(dns_name_digest(&name, digest, arg));
	isc_region_consume(&r, name_length(&name));

	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &r);

	return (dns_name_digest(&name, digest, arg));
}

static inline isc_boolean_t
checkowner_rp(ARGS_CHECKOWNER) {

	REQUIRE(type == 17);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_rp(ARGS_CHECKNAMES) {
	isc_region_t region;
	dns_name_t name;

	REQUIRE(rdata->type == 17);

	UNUSED(owner);

	dns_rdata_toregion(rdata, &region);
	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &region);
	if (!dns_name_ismailbox(&name)) {
		if (bad != NULL)
				dns_name_clone(&name, bad);
		return (ISC_FALSE);
	}
	return (ISC_TRUE);
}

#endif	/* RDATA_GENERIC_RP_17_C */
