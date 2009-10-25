/*
 * Copyright (C) 2004, 2007, 2008  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2003  Internet Software Consortium.
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

/* $Id: nsec_47.c,v 1.11 2008/07/15 23:47:21 tbox Exp $ */

/* reviewed: Wed Mar 15 18:21:15 PST 2000 by brister */

/* RFC 3845 */

#ifndef RDATA_GENERIC_NSEC_47_C
#define RDATA_GENERIC_NSEC_47_C

/*
 * The attributes do not include DNS_RDATATYPEATTR_SINGLETON
 * because we must be able to handle a parent/child NSEC pair.
 */
#define RRTYPE_NSEC_ATTRIBUTES (DNS_RDATATYPEATTR_DNSSEC)

static inline isc_result_t
fromtext_nsec(ARGS_FROMTEXT) {
	isc_token_t token;
	dns_name_t name;
	isc_buffer_t buffer;
	unsigned char bm[8*1024]; /* 64k bits */
	dns_rdatatype_t covered;
	int octet;
	int window;

	REQUIRE(type == 47);

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
		RETTOK(dns_rdatatype_fromtext(&covered,
					      &token.value.as_textregion));
		bm[covered/8] |= (0x80>>(covered%8));
	} while (1);
	isc_lex_ungettoken(lexer, &token);
	for (window = 0; window < 256 ; window++) {
		/*
		 * Find if we have a type in this window.
		 */
		for (octet = 31; octet >= 0; octet--)
			if (bm[window * 32 + octet] != 0)
				break;
		if (octet < 0)
			continue;
		RETERR(uint8_tobuffer(window, target));
		RETERR(uint8_tobuffer(octet + 1, target));
		RETERR(mem_tobuffer(target, &bm[window * 32], octet + 1));
	}
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
totext_nsec(ARGS_TOTEXT) {
	isc_region_t sr;
	unsigned int i, j, k;
	dns_name_t name;
	dns_name_t prefix;
	isc_boolean_t sub;
	unsigned int window, len;

	REQUIRE(rdata->type == 47);
	REQUIRE(rdata->length != 0);

	dns_name_init(&name, NULL);
	dns_name_init(&prefix, NULL);
	dns_rdata_toregion(rdata, &sr);
	dns_name_fromregion(&name, &sr);
	isc_region_consume(&sr, name_length(&name));
	sub = name_prefix(&name, tctx->origin, &prefix);
	RETERR(dns_name_totext(&prefix, sub, target));


	for (i = 0; i < sr.length; i += len) {
		INSIST(i + 2 <= sr.length);
		window = sr.base[i];
		len = sr.base[i + 1];
		INSIST(len > 0 && len <= 32);
		i += 2;
		INSIST(i + len <= sr.length);
		for (j = 0; j < len; j++) {
			dns_rdatatype_t t;
			if (sr.base[i + j] == 0)
				continue;
			for (k = 0; k < 8; k++) {
				if ((sr.base[i + j] & (0x80 >> k)) == 0)
					continue;
				t = window * 256 + j * 8 + k;
				RETERR(str_totext(" ", target));
				if (dns_rdatatype_isknown(t)) {
					RETERR(dns_rdatatype_totext(t, target));
				} else {
					char buf[sizeof("TYPE65535")];
					sprintf(buf, "TYPE%u", t);
					RETERR(str_totext(buf, target));
				}
			}
		}
	}
	return (ISC_R_SUCCESS);
}

static /* inline */ isc_result_t
fromwire_nsec(ARGS_FROMWIRE) {
	isc_region_t sr;
	dns_name_t name;
	unsigned int window, lastwindow = 0;
	unsigned int len;
	isc_boolean_t first = ISC_TRUE;
	unsigned int i;

	REQUIRE(type == 47);

	UNUSED(type);
	UNUSED(rdclass);

	dns_decompress_setmethods(dctx, DNS_COMPRESS_NONE);

	dns_name_init(&name, NULL);
	RETERR(dns_name_fromwire(&name, source, dctx, options, target));

	isc_buffer_activeregion(source, &sr);
	for (i = 0; i < sr.length; i += len) {
		/*
		 * Check for overflow.
		 */
		if (i + 2 > sr.length)
			RETERR(DNS_R_FORMERR);
		window = sr.base[i];
		len = sr.base[i + 1];
		i += 2;
		/*
		 * Check that bitmap windows are in the correct order.
		 */
		if (!first && window <= lastwindow)
			RETERR(DNS_R_FORMERR);
		/*
		 * Check for legal lengths.
		 */
		if (len < 1 || len > 32)
			RETERR(DNS_R_FORMERR);
		/*
		 * Check for overflow.
		 */
		if (i + len > sr.length)
			RETERR(DNS_R_FORMERR);
		/*
		 * The last octet of the bitmap must be non zero.
		 */
		if (sr.base[i + len - 1] == 0)
			RETERR(DNS_R_FORMERR);
		lastwindow = window;
		first = ISC_FALSE;
	}
	if (i != sr.length)
		return (DNS_R_EXTRADATA);
	if (first)
		RETERR(DNS_R_FORMERR);
	RETERR(mem_tobuffer(target, sr.base, sr.length));
	isc_buffer_forward(source, sr.length);
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
towire_nsec(ARGS_TOWIRE) {
	isc_region_t sr;
	dns_name_t name;
	dns_offsets_t offsets;

	REQUIRE(rdata->type == 47);
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
compare_nsec(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 47);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_nsec(ARGS_FROMSTRUCT) {
	dns_rdata_nsec_t *nsec = source;
	isc_region_t region;
	unsigned int i, len, window, lastwindow = 0;
	isc_boolean_t first = ISC_TRUE;

	REQUIRE(type == 47);
	REQUIRE(source != NULL);
	REQUIRE(nsec->common.rdtype == type);
	REQUIRE(nsec->common.rdclass == rdclass);
	REQUIRE(nsec->typebits != NULL || nsec->len == 0);

	UNUSED(type);
	UNUSED(rdclass);

	dns_name_toregion(&nsec->next, &region);
	RETERR(isc_buffer_copyregion(target, &region));
	/*
	 * Perform sanity check.
	 */
	for (i = 0; i < nsec->len ; i += len) {
		INSIST(i + 2 <= nsec->len);
		window = nsec->typebits[i];
		len = nsec->typebits[i+1];
		i += 2;
		INSIST(first || window > lastwindow);
		INSIST(len > 0 && len <= 32);
		INSIST(i + len <= nsec->len);
		INSIST(nsec->typebits[i + len - 1] != 0);
		lastwindow = window;
		first = ISC_FALSE;
	}
	INSIST(!first);
	return (mem_tobuffer(target, nsec->typebits, nsec->len));
}

static inline isc_result_t
tostruct_nsec(ARGS_TOSTRUCT) {
	isc_region_t region;
	dns_rdata_nsec_t *nsec = target;
	dns_name_t name;

	REQUIRE(rdata->type == 47);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	nsec->common.rdclass = rdata->rdclass;
	nsec->common.rdtype = rdata->type;
	ISC_LINK_INIT(&nsec->common, link);

	dns_name_init(&name, NULL);
	dns_rdata_toregion(rdata, &region);
	dns_name_fromregion(&name, &region);
	isc_region_consume(&region, name_length(&name));
	dns_name_init(&nsec->next, NULL);
	RETERR(name_duporclone(&name, mctx, &nsec->next));

	nsec->len = region.length;
	nsec->typebits = mem_maybedup(mctx, region.base, region.length);
	if (nsec->typebits == NULL)
		goto cleanup;

	nsec->mctx = mctx;
	return (ISC_R_SUCCESS);

 cleanup:
	if (mctx != NULL)
		dns_name_free(&nsec->next, mctx);
	return (ISC_R_NOMEMORY);
}

static inline void
freestruct_nsec(ARGS_FREESTRUCT) {
	dns_rdata_nsec_t *nsec = source;

	REQUIRE(source != NULL);
	REQUIRE(nsec->common.rdtype == 47);

	if (nsec->mctx == NULL)
		return;

	dns_name_free(&nsec->next, nsec->mctx);
	if (nsec->typebits != NULL)
		isc_mem_free(nsec->mctx, nsec->typebits);
	nsec->mctx = NULL;
}

static inline isc_result_t
additionaldata_nsec(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == 47);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_nsec(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == 47);

	dns_rdata_toregion(rdata, &r);
	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_nsec(ARGS_CHECKOWNER) {

       REQUIRE(type == 47);

       UNUSED(name);
       UNUSED(type);
       UNUSED(rdclass);
       UNUSED(wildcard);

       return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_nsec(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == 47);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

#endif	/* RDATA_GENERIC_NSEC_47_C */
