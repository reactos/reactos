/*
 * Copyright (C) 2004, 2007, 2009  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: soa_6.c,v 1.61.332.2 2009/02/16 23:47:15 tbox Exp $ */

/* Reviewed: Thu Mar 16 15:18:32 PST 2000 by explorer */

#ifndef RDATA_GENERIC_SOA_6_C
#define RDATA_GENERIC_SOA_6_C

#define RRTYPE_SOA_ATTRIBUTES (DNS_RDATATYPEATTR_SINGLETON)

static inline isc_result_t
fromtext_soa(ARGS_FROMTEXT) {
	isc_token_t token;
	dns_name_t name;
	isc_buffer_t buffer;
	int i;
	isc_uint32_t n;
	isc_boolean_t ok;

	REQUIRE(type == 6);

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
		if ((options & DNS_RDATA_CHECKNAMES) != 0)
			switch (i) {
			case 0:
				ok = dns_name_ishostname(&name, ISC_FALSE);
				break;
			case 1:
				ok = dns_name_ismailbox(&name);
				break;

			}
		if (!ok && (options & DNS_RDATA_CHECKNAMESFAIL) != 0)
			RETTOK(DNS_R_BADNAME);
		if (!ok && callbacks != NULL)
			warn_badname(&name, lexer, callbacks);
	}

	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      ISC_FALSE));
	RETERR(uint32_tobuffer(token.value.as_ulong, target));

	for (i = 0; i < 4; i++) {
		RETERR(isc_lex_getmastertoken(lexer, &token,
					      isc_tokentype_string,
					      ISC_FALSE));
		RETTOK(dns_counter_fromtext(&token.value.as_textregion, &n));
		RETERR(uint32_tobuffer(n, target));
	}

	return (ISC_R_SUCCESS);
}

static const char *soa_fieldnames[5] = {
	"serial", "refresh", "retry", "expire", "minimum"
};

static inline isc_result_t
totext_soa(ARGS_TOTEXT) {
	isc_region_t dregion;
	dns_name_t mname;
	dns_name_t rname;
	dns_name_t prefix;
	isc_boolean_t sub;
	int i;
	isc_boolean_t multiline;
	isc_boolean_t comment;

	REQUIRE(rdata->type == 6);
	REQUIRE(rdata->length != 0);

	multiline = ISC_TF((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0);
	if (multiline)
		comment = ISC_TF((tctx->flags & DNS_STYLEFLAG_COMMENT) != 0);
	else
		comment = ISC_FALSE;


	dns_name_init(&mname, NULL);
	dns_name_init(&rname, NULL);
	dns_name_init(&prefix, NULL);

	dns_rdata_toregion(rdata, &dregion);

	dns_name_fromregion(&mname, &dregion);
	isc_region_consume(&dregion, name_length(&mname));

	dns_name_fromregion(&rname, &dregion);
	isc_region_consume(&dregion, name_length(&rname));

	sub = name_prefix(&mname, tctx->origin, &prefix);
	RETERR(dns_name_totext(&prefix, sub, target));

	RETERR(str_totext(" ", target));

	sub = name_prefix(&rname, tctx->origin, &prefix);
	RETERR(dns_name_totext(&prefix, sub, target));

	if (multiline)
		RETERR(str_totext(" (" , target));
	RETERR(str_totext(tctx->linebreak, target));

	for (i = 0; i < 5; i++) {
		char buf[sizeof("0123456789 ; ")];
		unsigned long num;
		num = uint32_fromregion(&dregion);
		isc_region_consume(&dregion, 4);
		sprintf(buf, comment ? "%-10lu ; " : "%lu", num);
		RETERR(str_totext(buf, target));
		if (comment) {
			RETERR(str_totext(soa_fieldnames[i], target));
			/* Print times in week/day/hour/minute/second form */
			if (i >= 1) {
				RETERR(str_totext(" (", target));
				RETERR(dns_ttl_totext(num, ISC_TRUE, target));
				RETERR(str_totext(")", target));
			}
			RETERR(str_totext(tctx->linebreak, target));
		} else if (i < 4) {
			RETERR(str_totext(tctx->linebreak, target));
		}
	}

	if (multiline)
		RETERR(str_totext(")", target));

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
fromwire_soa(ARGS_FROMWIRE) {
	dns_name_t mname;
	dns_name_t rname;
	isc_region_t sregion;
	isc_region_t tregion;

	REQUIRE(type == 6);

	UNUSED(type);
	UNUSED(rdclass);

	dns_decompress_setmethods(dctx, DNS_COMPRESS_GLOBAL14);

	dns_name_init(&mname, NULL);
	dns_name_init(&rname, NULL);

	RETERR(dns_name_fromwire(&mname, source, dctx, options, target));
	RETERR(dns_name_fromwire(&rname, source, dctx, options, target));

	isc_buffer_activeregion(source, &sregion);
	isc_buffer_availableregion(target, &tregion);

	if (sregion.length < 20)
		return (ISC_R_UNEXPECTEDEND);
	if (tregion.length < 20)
		return (ISC_R_NOSPACE);

	memcpy(tregion.base, sregion.base, 20);
	isc_buffer_forward(source, 20);
	isc_buffer_add(target, 20);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
towire_soa(ARGS_TOWIRE) {
	isc_region_t sregion;
	isc_region_t tregion;
	dns_name_t mname;
	dns_name_t rname;
	dns_offsets_t moffsets;
	dns_offsets_t roffsets;

	REQUIRE(rdata->type == 6);
	REQUIRE(rdata->length != 0);

	dns_compress_setmethods(cctx, DNS_COMPRESS_GLOBAL14);

	dns_name_init(&mname, moffsets);
	dns_name_init(&rname, roffsets);

	dns_rdata_toregion(rdata, &sregion);

	dns_name_fromregion(&mname, &sregion);
	isc_region_consume(&sregion, name_length(&mname));
	RETERR(dns_name_towire(&mname, cctx, target));

	dns_name_fromregion(&rname, &sregion);
	isc_region_consume(&sregion, name_length(&rname));
	RETERR(dns_name_towire(&rname, cctx, target));

	isc_buffer_availableregion(target, &tregion);
	if (tregion.length < 20)
		return (ISC_R_NOSPACE);

	memcpy(tregion.base, sregion.base, 20);
	isc_buffer_add(target, 20);
	return (ISC_R_SUCCESS);
}

static inline int
compare_soa(ARGS_COMPARE) {
	isc_region_t region1;
	isc_region_t region2;
	dns_name_t name1;
	dns_name_t name2;
	int order;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 6);
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
	if (order != 0)
		return (order);

	isc_region_consume(&region1, name_length(&name1));
	isc_region_consume(&region2, name_length(&name2));

	return (isc_region_compare(&region1, &region2));
}

static inline isc_result_t
fromstruct_soa(ARGS_FROMSTRUCT) {
	dns_rdata_soa_t *soa = source;
	isc_region_t region;

	REQUIRE(type == 6);
	REQUIRE(source != NULL);
	REQUIRE(soa->common.rdtype == type);
	REQUIRE(soa->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	dns_name_toregion(&soa->origin, &region);
	RETERR(isc_buffer_copyregion(target, &region));
	dns_name_toregion(&soa->contact, &region);
	RETERR(isc_buffer_copyregion(target, &region));
	RETERR(uint32_tobuffer(soa->serial, target));
	RETERR(uint32_tobuffer(soa->refresh, target));
	RETERR(uint32_tobuffer(soa->retry, target));
	RETERR(uint32_tobuffer(soa->expire, target));
	return (uint32_tobuffer(soa->minimum, target));
}

static inline isc_result_t
tostruct_soa(ARGS_TOSTRUCT) {
	isc_region_t region;
	dns_rdata_soa_t *soa = target;
	dns_name_t name;
	isc_result_t result;

	REQUIRE(rdata->type == 6);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	soa->common.rdclass = rdata->rdclass;
	soa->common.rdtype = rdata->type;
	ISC_LINK_INIT(&soa->common, link);


	dns_rdata_toregion(rdata, &region);

	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &region);
	isc_region_consume(&region, name_length(&name));
	dns_name_init(&soa->origin, NULL);
	RETERR(name_duporclone(&name, mctx, &soa->origin));

	dns_name_fromregion(&name, &region);
	isc_region_consume(&region, name_length(&name));
	dns_name_init(&soa->contact, NULL);
	result = name_duporclone(&name, mctx, &soa->contact);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	soa->serial = uint32_fromregion(&region);
	isc_region_consume(&region, 4);

	soa->refresh = uint32_fromregion(&region);
	isc_region_consume(&region, 4);

	soa->retry = uint32_fromregion(&region);
	isc_region_consume(&region, 4);

	soa->expire = uint32_fromregion(&region);
	isc_region_consume(&region, 4);

	soa->minimum = uint32_fromregion(&region);

	soa->mctx = mctx;
	return (ISC_R_SUCCESS);

 cleanup:
	if (mctx != NULL)
		dns_name_free(&soa->origin, mctx);
	return (ISC_R_NOMEMORY);
}

static inline void
freestruct_soa(ARGS_FREESTRUCT) {
	dns_rdata_soa_t *soa = source;

	REQUIRE(source != NULL);
	REQUIRE(soa->common.rdtype == 6);

	if (soa->mctx == NULL)
		return;

	dns_name_free(&soa->origin, soa->mctx);
	dns_name_free(&soa->contact, soa->mctx);
	soa->mctx = NULL;
}

static inline isc_result_t
additionaldata_soa(ARGS_ADDLDATA) {
	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	REQUIRE(rdata->type == 6);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_soa(ARGS_DIGEST) {
	isc_region_t r;
	dns_name_t name;

	REQUIRE(rdata->type == 6);

	dns_rdata_toregion(rdata, &r);

	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &r);
	RETERR(dns_name_digest(&name, digest, arg));
	isc_region_consume(&r, name_length(&name));

	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &r);
	RETERR(dns_name_digest(&name, digest, arg));
	isc_region_consume(&r, name_length(&name));

	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_soa(ARGS_CHECKOWNER) {

	REQUIRE(type == 6);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_soa(ARGS_CHECKNAMES) {
	isc_region_t region;
	dns_name_t name;

	REQUIRE(rdata->type == 6);

	UNUSED(owner);

	dns_rdata_toregion(rdata, &region);
	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &region);
	if (!dns_name_ishostname(&name, ISC_FALSE)) {
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

#endif	/* RDATA_GENERIC_SOA_6_C */
