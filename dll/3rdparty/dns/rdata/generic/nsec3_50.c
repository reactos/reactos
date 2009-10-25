/*
 * Copyright (C) 2008, 2009  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: nsec3_50.c,v 1.4.48.2 2009/01/18 23:47:41 tbox Exp $ */

/*
 * Copyright (C) 2004  Nominet, Ltd.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND NOMINET DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* RFC 5155 */

#ifndef RDATA_GENERIC_NSEC3_50_C
#define RDATA_GENERIC_NSEC3_50_C

#include <isc/iterated_hash.h>
#include <isc/base32.h>

#define RRTYPE_NSEC3_ATTRIBUTES DNS_RDATATYPEATTR_DNSSEC

static inline isc_result_t
fromtext_nsec3(ARGS_FROMTEXT) {
	isc_token_t token;
	unsigned char bm[8*1024]; /* 64k bits */
	dns_rdatatype_t covered;
	int octet;
	int window;
	unsigned int flags;
	unsigned char hashalg;
	isc_buffer_t b;

	REQUIRE(type == 50);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(callbacks);
	UNUSED(origin);
	UNUSED(options);

	/* Hash. */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	RETTOK(dns_hashalg_fromtext(&hashalg, &token.value.as_textregion));
	RETERR(uint8_tobuffer(hashalg, target));

	/* Flags. */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      ISC_FALSE));
	flags = token.value.as_ulong;
	if (flags > 255U)
		RETTOK(ISC_R_RANGE);
	RETERR(uint8_tobuffer(flags, target));

	/* Iterations. */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      ISC_FALSE));
	if (token.value.as_ulong > 0xffffU)
		RETTOK(ISC_R_RANGE);
	RETERR(uint16_tobuffer(token.value.as_ulong, target));

	/* salt */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	if (token.value.as_textregion.length > (255*2))
		RETTOK(DNS_R_TEXTTOOLONG);
	if (strcmp(DNS_AS_STR(token), "-") == 0) {
		RETERR(uint8_tobuffer(0, target));
	} else {
		RETERR(uint8_tobuffer(strlen(DNS_AS_STR(token)) / 2, target));
		RETERR(isc_hex_decodestring(DNS_AS_STR(token), target));
	}

	/*
	 * Next hash a single base32hex word.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	isc_buffer_init(&b, bm, sizeof(bm));
	RETTOK(isc_base32hex_decodestring(DNS_AS_STR(token), &b));
	if (isc_buffer_usedlength(&b) > 0xffU)
		RETTOK(ISC_R_RANGE);
	RETERR(uint8_tobuffer(isc_buffer_usedlength(&b), target));
	RETERR(mem_tobuffer(target, &bm, isc_buffer_usedlength(&b)));

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
totext_nsec3(ARGS_TOTEXT) {
	isc_region_t sr;
	unsigned int i, j, k;
	unsigned int window, len;
	unsigned char hash;
	unsigned char flags;
	char buf[sizeof("65535 ")];
	isc_uint32_t iterations;

	REQUIRE(rdata->type == 50);
	REQUIRE(rdata->length != 0);

	UNUSED(tctx);

	dns_rdata_toregion(rdata, &sr);

	hash = uint8_fromregion(&sr);
	isc_region_consume(&sr, 1);

	flags = uint8_fromregion(&sr);
	isc_region_consume(&sr, 1);

	iterations = uint16_fromregion(&sr);
	isc_region_consume(&sr, 2);

	sprintf(buf, "%u ", hash);
	RETERR(str_totext(buf, target));

	sprintf(buf, "%u ", flags);
	RETERR(str_totext(buf, target));

	sprintf(buf, "%u ", iterations);
	RETERR(str_totext(buf, target));

	j = uint8_fromregion(&sr);
	isc_region_consume(&sr, 1);
	INSIST(j <= sr.length);

	if (j != 0) {
		i = sr.length;
		sr.length = j;
		RETERR(isc_hex_totext(&sr, 1, "", target));
		sr.length = i - j;
		RETERR(str_totext(" ", target));
	} else
		RETERR(str_totext("- ", target));

	j = uint8_fromregion(&sr);
	isc_region_consume(&sr, 1);
	INSIST(j <= sr.length);

	i = sr.length;
	sr.length = j;
	RETERR(isc_base32hex_totext(&sr, 1, "", target));
	sr.length = i - j;

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

static inline isc_result_t
fromwire_nsec3(ARGS_FROMWIRE) {
	isc_region_t sr, rr;
	unsigned int window, lastwindow = 0;
	unsigned int len;
	unsigned int saltlen, hashlen;
	isc_boolean_t first = ISC_TRUE;
	unsigned int i;

	REQUIRE(type == 50);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(options);
	UNUSED(dctx);

	isc_buffer_activeregion(source, &sr);
	rr = sr;

	/* hash(1), flags(1), iteration(2), saltlen(1) */
	if (sr.length < 5U)
		RETERR(DNS_R_FORMERR);
	saltlen = sr.base[4];
	isc_region_consume(&sr, 5);

	if (sr.length < saltlen)
		RETERR(DNS_R_FORMERR);
	isc_region_consume(&sr, saltlen);

	if (sr.length < 1U)
		RETERR(DNS_R_FORMERR);
	hashlen = sr.base[0];
	isc_region_consume(&sr, 1);

	if (sr.length < hashlen)
		RETERR(DNS_R_FORMERR);
	isc_region_consume(&sr, hashlen);

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
	RETERR(mem_tobuffer(target, rr.base, rr.length));
	isc_buffer_forward(source, rr.length);
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
towire_nsec3(ARGS_TOWIRE) {
	isc_region_t sr;

	REQUIRE(rdata->type == 50);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	dns_rdata_toregion(rdata, &sr);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static inline int
compare_nsec3(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 50);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_nsec3(ARGS_FROMSTRUCT) {
	dns_rdata_nsec3_t *nsec3 = source;
	unsigned int i, len, window, lastwindow = 0;
	isc_boolean_t first = ISC_TRUE;

	REQUIRE(type == 50);
	REQUIRE(source != NULL);
	REQUIRE(nsec3->common.rdtype == type);
	REQUIRE(nsec3->common.rdclass == rdclass);
	REQUIRE(nsec3->typebits != NULL || nsec3->len == 0);
	REQUIRE(nsec3->hash == dns_hash_sha1);

	UNUSED(type);
	UNUSED(rdclass);

	RETERR(uint8_tobuffer(nsec3->hash, target));
	RETERR(uint8_tobuffer(nsec3->flags, target));
	RETERR(uint16_tobuffer(nsec3->iterations, target));
	RETERR(uint8_tobuffer(nsec3->salt_length, target));
	RETERR(mem_tobuffer(target, nsec3->salt, nsec3->salt_length));
	RETERR(uint8_tobuffer(nsec3->next_length, target));
	RETERR(mem_tobuffer(target, nsec3->next, nsec3->next_length));

	/*
	 * Perform sanity check.
	 */
	for (i = 0; i < nsec3->len ; i += len) {
		INSIST(i + 2 <= nsec3->len);
		window = nsec3->typebits[i];
		len = nsec3->typebits[i+1];
		i += 2;
		INSIST(first || window > lastwindow);
		INSIST(len > 0 && len <= 32);
		INSIST(i + len <= nsec3->len);
		INSIST(nsec3->typebits[i + len - 1] != 0);
		lastwindow = window;
		first = ISC_FALSE;
	}
	return (mem_tobuffer(target, nsec3->typebits, nsec3->len));
}

static inline isc_result_t
tostruct_nsec3(ARGS_TOSTRUCT) {
	isc_region_t region;
	dns_rdata_nsec3_t *nsec3 = target;

	REQUIRE(rdata->type == 50);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	nsec3->common.rdclass = rdata->rdclass;
	nsec3->common.rdtype = rdata->type;
	ISC_LINK_INIT(&nsec3->common, link);

	region.base = rdata->data;
	region.length = rdata->length;
	nsec3->hash = uint8_consume_fromregion(&region);
	nsec3->flags = uint8_consume_fromregion(&region);
	nsec3->iterations = uint16_consume_fromregion(&region);

	nsec3->salt_length = uint8_consume_fromregion(&region);
	nsec3->salt = mem_maybedup(mctx, region.base, nsec3->salt_length);
	if (nsec3->salt == NULL)
		return (ISC_R_NOMEMORY);
	isc_region_consume(&region, nsec3->salt_length);

	nsec3->next_length = uint8_consume_fromregion(&region);
	nsec3->next = mem_maybedup(mctx, region.base, nsec3->next_length);
	if (nsec3->next == NULL)
		goto cleanup;
	isc_region_consume(&region, nsec3->next_length);

	nsec3->len = region.length;
	nsec3->typebits = mem_maybedup(mctx, region.base, region.length);
	if (nsec3->typebits == NULL)
		goto cleanup;

	nsec3->mctx = mctx;
	return (ISC_R_SUCCESS);

  cleanup:
	if (nsec3->next != NULL)
		isc_mem_free(mctx, nsec3->next);
	isc_mem_free(mctx, nsec3->salt);
	return (ISC_R_NOMEMORY);
}

static inline void
freestruct_nsec3(ARGS_FREESTRUCT) {
	dns_rdata_nsec3_t *nsec3 = source;

	REQUIRE(source != NULL);
	REQUIRE(nsec3->common.rdtype == 50);

	if (nsec3->mctx == NULL)
		return;

	if (nsec3->salt != NULL)
		isc_mem_free(nsec3->mctx, nsec3->salt);
	if (nsec3->next != NULL)
		isc_mem_free(nsec3->mctx, nsec3->next);
	if (nsec3->typebits != NULL)
		isc_mem_free(nsec3->mctx, nsec3->typebits);
	nsec3->mctx = NULL;
}

static inline isc_result_t
additionaldata_nsec3(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == 50);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_nsec3(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == 50);

	dns_rdata_toregion(rdata, &r);
	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_nsec3(ARGS_CHECKOWNER) {

       REQUIRE(type == 50);

       UNUSED(name);
       UNUSED(type);
       UNUSED(rdclass);
       UNUSED(wildcard);

       return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_nsec3(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == 50);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

#endif	/* RDATA_GENERIC_NSEC3_50_C */
