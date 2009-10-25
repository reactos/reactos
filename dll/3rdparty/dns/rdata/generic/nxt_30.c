/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2003  Internet Software Consortium.
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

/* $Id: nxt_30.c,v 1.63 2007/06/19 23:47:17 tbox Exp $ */

/* reviewed: Wed Mar 15 18:21:15 PST 2000 by brister */

/* RFC2535 */

#ifndef RDATA_GENERIC_NXT_30_C
#define RDATA_GENERIC_NXT_30_C

/*
 * The attributes do not include DNS_RDATATYPEATTR_SINGLETON
 * because we must be able to handle a parent/child NXT pair.
 */
#define RRTYPE_NXT_ATTRIBUTES (0)

static inline isc_result_t
fromtext_nxt(ARGS_FROMTEXT) {
	isc_token_t token;
	dns_name_t name;
	isc_buffer_t buffer;
	char *e;
	unsigned char bm[8*1024]; /* 64k bits */
	dns_rdatatype_t covered;
	dns_rdatatype_t maxcovered = 0;
	isc_boolean_t first = ISC_TRUE;
	long n;

	REQUIRE(type == 30);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(callbacks);

	/*
	 * Next domain.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	dns_name_init(&name, NULL);
	buffer_fromregion(&buffer, &token.value.as_region);
	origin = (origin != NULL) ? origin : dns_rootname;
	RETTOK(dns_name_fromtext(&name, &buffer, origin, options, target));

	memset(bm, 0, sizeof(bm));
	do {
		RETERR(isc_lex_getmastertoken(lexer, &token,
					      isc_tokentype_string, ISC_TRUE));
		if (token.type != isc_tokentype_string)
			break;
		n = strtol(DNS_AS_STR(token), &e, 10);
		if (e != DNS_AS_STR(token) && *e == '\0') {
			covered = (dns_rdatatype_t)n;
		} else if (dns_rdatatype_fromtext(&covered,
				&token.value.as_textregion) == DNS_R_UNKNOWN)
			RETTOK(DNS_R_UNKNOWN);
		/*
		 * NXT is only specified for types 1..127.
		 */
		if (covered < 1 || covered > 127)
			return (ISC_R_RANGE);
		if (first || covered > maxcovered)
			maxcovered = covered;
		first = ISC_FALSE;
		bm[covered/8] |= (0x80>>(covered%8));
	} while (1);
	isc_lex_ungettoken(lexer, &token);
	if (first)
		return (ISC_R_SUCCESS);
	n = (maxcovered + 8) / 8;
	return (mem_tobuffer(target, bm, n));
}

static inline isc_result_t
totext_nxt(ARGS_TOTEXT) {
	isc_region_t sr;
	unsigned int i, j;
	dns_name_t name;
	dns_name_t prefix;
	isc_boolean_t sub;

	REQUIRE(rdata->type == 30);
	REQUIRE(rdata->length != 0);

	dns_name_init(&name, NULL);
	dns_name_init(&prefix, NULL);
	dns_rdata_toregion(rdata, &sr);
	dns_name_fromregion(&name, &sr);
	isc_region_consume(&sr, name_length(&name));
	sub = name_prefix(&name, tctx->origin, &prefix);
	RETERR(dns_name_totext(&prefix, sub, target));

	for (i = 0; i < sr.length; i++) {
		if (sr.base[i] != 0)
			for (j = 0; j < 8; j++)
				if ((sr.base[i] & (0x80 >> j)) != 0) {
					dns_rdatatype_t t = i * 8 + j;
					RETERR(str_totext(" ", target));
					if (dns_rdatatype_isknown(t)) {
						RETERR(dns_rdatatype_totext(t,
								      target));
					} else {
						char buf[sizeof("65535")];
						sprintf(buf, "%u", t);
						RETERR(str_totext(buf,
								  target));
					}
				}
	}
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
fromwire_nxt(ARGS_FROMWIRE) {
	isc_region_t sr;
	dns_name_t name;

	REQUIRE(type == 30);

	UNUSED(type);
	UNUSED(rdclass);

	dns_decompress_setmethods(dctx, DNS_COMPRESS_NONE);

	dns_name_init(&name, NULL);
	RETERR(dns_name_fromwire(&name, source, dctx, options, target));

	isc_buffer_activeregion(source, &sr);
	if (sr.length > 0 && (sr.base[0] & 0x80) == 0 &&
	    ((sr.length > 16) || sr.base[sr.length - 1] == 0))
		return (DNS_R_BADBITMAP);
	RETERR(mem_tobuffer(target, sr.base, sr.length));
	isc_buffer_forward(source, sr.length);
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
towire_nxt(ARGS_TOWIRE) {
	isc_region_t sr;
	dns_name_t name;
	dns_offsets_t offsets;

	REQUIRE(rdata->type == 30);
	REQUIRE(rdata->length != 0);

	dns_compress_setmethods(cctx, DNS_COMPRESS_NONE);
	dns_name_init(&name, offsets);
	dns_rdata_toregion(rdata, &sr);
	dns_name_fromregion(&name, &sr);
	isc_region_consume(&sr, name_length(&name));
	RETERR(dns_name_towire(&name, cctx, target));

	return (mem_tobuffer(target, sr.base, sr.length));
}

static inline int
compare_nxt(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;
	dns_name_t name1;
	dns_name_t name2;
	int order;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 30);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_name_init(&name1, NULL);
	dns_name_init(&name2, NULL);
	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	dns_name_fromregion(&name1, &r1);
	dns_name_fromregion(&name2, &r2);
	order = dns_name_rdatacompare(&name1, &name2);
	if (order != 0)
		return (order);

	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_nxt(ARGS_FROMSTRUCT) {
	dns_rdata_nxt_t *nxt = source;
	isc_region_t region;

	REQUIRE(type == 30);
	REQUIRE(source != NULL);
	REQUIRE(nxt->common.rdtype == type);
	REQUIRE(nxt->common.rdclass == rdclass);
	REQUIRE(nxt->typebits != NULL || nxt->len == 0);
	if (nxt->typebits != NULL && (nxt->typebits[0] & 0x80) == 0) {
		REQUIRE(nxt->len <= 16);
		REQUIRE(nxt->typebits[nxt->len - 1] != 0);
	}

	UNUSED(type);
	UNUSED(rdclass);

	dns_name_toregion(&nxt->next, &region);
	RETERR(isc_buffer_copyregion(target, &region));

	return (mem_tobuffer(target, nxt->typebits, nxt->len));
}

static inline isc_result_t
tostruct_nxt(ARGS_TOSTRUCT) {
	isc_region_t region;
	dns_rdata_nxt_t *nxt = target;
	dns_name_t name;

	REQUIRE(rdata->type == 30);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	nxt->common.rdclass = rdata->rdclass;
	nxt->common.rdtype = rdata->type;
	ISC_LINK_INIT(&nxt->common, link);

	dns_name_init(&name, NULL);
	dns_rdata_toregion(rdata, &region);
	dns_name_fromregion(&name, &region);
	isc_region_consume(&region, name_length(&name));
	dns_name_init(&nxt->next, NULL);
	RETERR(name_duporclone(&name, mctx, &nxt->next));

	nxt->len = region.length;
	nxt->typebits = mem_maybedup(mctx, region.base, region.length);
	if (nxt->typebits == NULL)
		goto cleanup;

	nxt->mctx = mctx;
	return (ISC_R_SUCCESS);

 cleanup:
	if (mctx != NULL)
		dns_name_free(&nxt->next, mctx);
	return (ISC_R_NOMEMORY);
}

static inline void
freestruct_nxt(ARGS_FREESTRUCT) {
	dns_rdata_nxt_t *nxt = source;

	REQUIRE(source != NULL);
	REQUIRE(nxt->common.rdtype == 30);

	if (nxt->mctx == NULL)
		return;

	dns_name_free(&nxt->next, nxt->mctx);
	if (nxt->typebits != NULL)
		isc_mem_free(nxt->mctx, nxt->typebits);
	nxt->mctx = NULL;
}

static inline isc_result_t
additionaldata_nxt(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == 30);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_nxt(ARGS_DIGEST) {
	isc_region_t r;
	dns_name_t name;
	isc_result_t result;

	REQUIRE(rdata->type == 30);

	dns_rdata_toregion(rdata, &r);
	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &r);
	result = dns_name_digest(&name, digest, arg);
	if (result != ISC_R_SUCCESS)
		return (result);
	isc_region_consume(&r, name_length(&name));

	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_nxt(ARGS_CHECKOWNER) {

	REQUIRE(type == 30);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_nxt(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == 30);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

#endif	/* RDATA_GENERIC_NXT_30_C */
