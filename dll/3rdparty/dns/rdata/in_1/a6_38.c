/*
 * Copyright (C) 2004, 2007  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: a6_38.c,v 1.54 2007/06/19 23:47:17 tbox Exp $ */

/* RFC2874 */

#ifndef RDATA_IN_1_A6_28_C
#define RDATA_IN_1_A6_28_C

#include <isc/net.h>

#define RRTYPE_A6_ATTRIBUTES (0)

static inline isc_result_t
fromtext_in_a6(ARGS_FROMTEXT) {
	isc_token_t token;
	unsigned char addr[16];
	unsigned char prefixlen;
	unsigned char octets;
	unsigned char mask;
	dns_name_t name;
	isc_buffer_t buffer;
	isc_boolean_t ok;

	REQUIRE(type == 38);
	REQUIRE(rdclass == 1);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(callbacks);

	/*
	 * Prefix length.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      ISC_FALSE));
	if (token.value.as_ulong > 128U)
		RETTOK(ISC_R_RANGE);

	prefixlen = (unsigned char)token.value.as_ulong;
	RETERR(mem_tobuffer(target, &prefixlen, 1));

	/*
	 * Suffix.
	 */
	if (prefixlen != 128) {
		/*
		 * Prefix 0..127.
		 */
		octets = prefixlen/8;
		/*
		 * Octets 0..15.
		 */
		RETERR(isc_lex_getmastertoken(lexer, &token,
					      isc_tokentype_string,
					      ISC_FALSE));
		if (inet_pton(AF_INET6, DNS_AS_STR(token), addr) != 1)
			RETTOK(DNS_R_BADAAAA);
		mask = 0xff >> (prefixlen % 8);
		addr[octets] &= mask;
		RETERR(mem_tobuffer(target, &addr[octets], 16 - octets));
	}

	if (prefixlen == 0)
		return (ISC_R_SUCCESS);

	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	dns_name_init(&name, NULL);
	buffer_fromregion(&buffer, &token.value.as_region);
	origin = (origin != NULL) ? origin : dns_rootname;
	RETTOK(dns_name_fromtext(&name, &buffer, origin, options, target));
	ok = ISC_TRUE;
	if ((options & DNS_RDATA_CHECKNAMES) != 0)
		ok = dns_name_ishostname(&name, ISC_FALSE);
	if (!ok && (options & DNS_RDATA_CHECKNAMESFAIL) != 0)
		RETTOK(DNS_R_BADNAME);
	if (!ok && callbacks != NULL)
		warn_badname(&name, lexer, callbacks);
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
totext_in_a6(ARGS_TOTEXT) {
	isc_region_t sr, ar;
	unsigned char addr[16];
	unsigned char prefixlen;
	unsigned char octets;
	unsigned char mask;
	char buf[sizeof("128")];
	dns_name_t name;
	dns_name_t prefix;
	isc_boolean_t sub;

	REQUIRE(rdata->type == 38);
	REQUIRE(rdata->rdclass == 1);
	REQUIRE(rdata->length != 0);

	dns_rdata_toregion(rdata, &sr);
	prefixlen = sr.base[0];
	INSIST(prefixlen <= 128);
	isc_region_consume(&sr, 1);
	sprintf(buf, "%u", prefixlen);
	RETERR(str_totext(buf, target));
	RETERR(str_totext(" ", target));

	if (prefixlen != 128) {
		octets = prefixlen/8;
		memset(addr, 0, sizeof(addr));
		memcpy(&addr[octets], sr.base, 16 - octets);
		mask = 0xff >> (prefixlen % 8);
		addr[octets] &= mask;
		ar.base = addr;
		ar.length = sizeof(addr);
		RETERR(inet_totext(AF_INET6, &ar, target));
		isc_region_consume(&sr, 16 - octets);
	}

	if (prefixlen == 0)
		return (ISC_R_SUCCESS);

	RETERR(str_totext(" ", target));
	dns_name_init(&name, NULL);
	dns_name_init(&prefix, NULL);
	dns_name_fromregion(&name, &sr);
	sub = name_prefix(&name, tctx->origin, &prefix);
	return (dns_name_totext(&prefix, sub, target));
}

static inline isc_result_t
fromwire_in_a6(ARGS_FROMWIRE) {
	isc_region_t sr;
	unsigned char prefixlen;
	unsigned char octets;
	unsigned char mask;
	dns_name_t name;

	REQUIRE(type == 38);
	REQUIRE(rdclass == 1);

	UNUSED(type);
	UNUSED(rdclass);

	dns_decompress_setmethods(dctx, DNS_COMPRESS_NONE);

	isc_buffer_activeregion(source, &sr);
	/*
	 * Prefix length.
	 */
	if (sr.length < 1)
		return (ISC_R_UNEXPECTEDEND);
	prefixlen = sr.base[0];
	if (prefixlen > 128)
		return (ISC_R_RANGE);
	isc_region_consume(&sr, 1);
	RETERR(mem_tobuffer(target, &prefixlen, 1));
	isc_buffer_forward(source, 1);

	/*
	 * Suffix.
	 */
	if (prefixlen != 128) {
		octets = 16 - prefixlen / 8;
		if (sr.length < octets)
			return (ISC_R_UNEXPECTEDEND);
		mask = 0xff >> (prefixlen % 8);
		sr.base[0] &= mask;	/* Ensure pad bits are zero. */
		RETERR(mem_tobuffer(target, sr.base, octets));
		isc_buffer_forward(source, octets);
	}

	if (prefixlen == 0)
		return (ISC_R_SUCCESS);

	dns_name_init(&name, NULL);
	return (dns_name_fromwire(&name, source, dctx, options, target));
}

static inline isc_result_t
towire_in_a6(ARGS_TOWIRE) {
	isc_region_t sr;
	dns_name_t name;
	dns_offsets_t offsets;
	unsigned char prefixlen;
	unsigned char octets;

	REQUIRE(rdata->type == 38);
	REQUIRE(rdata->rdclass == 1);
	REQUIRE(rdata->length != 0);

	dns_compress_setmethods(cctx, DNS_COMPRESS_NONE);
	dns_rdata_toregion(rdata, &sr);
	prefixlen = sr.base[0];
	INSIST(prefixlen <= 128);

	octets = 1 + 16 - prefixlen / 8;
	RETERR(mem_tobuffer(target, sr.base, octets));
	isc_region_consume(&sr, octets);

	if (prefixlen == 0)
		return (ISC_R_SUCCESS);

	dns_name_init(&name, offsets);
	dns_name_fromregion(&name, &sr);
	return (dns_name_towire(&name, cctx, target));
}

static inline int
compare_in_a6(ARGS_COMPARE) {
	int order;
	unsigned char prefixlen1, prefixlen2;
	unsigned char octets;
	dns_name_t name1;
	dns_name_t name2;
	isc_region_t region1;
	isc_region_t region2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 38);
	REQUIRE(rdata1->rdclass == 1);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &region1);
	dns_rdata_toregion(rdata2, &region2);
	prefixlen1 = region1.base[0];
	prefixlen2 = region2.base[0];
	isc_region_consume(&region1, 1);
	isc_region_consume(&region2, 1);
	if (prefixlen1 < prefixlen2)
		return (-1);
	else if (prefixlen1 > prefixlen2)
		return (1);
	/*
	 * Prefix lengths are equal.
	 */
	octets = 16 - prefixlen1 / 8;

	if (octets > 0) {
		order = memcmp(region1.base, region2.base, octets);
		if (order < 0)
			return (-1);
		else if (order > 0)
			return (1);
		/*
		 * Address suffixes are equal.
		 */
		if (prefixlen1 == 0)
			return (order);
		isc_region_consume(&region1, octets);
		isc_region_consume(&region2, octets);
	}

	dns_name_init(&name1, NULL);
	dns_name_init(&name2, NULL);
	dns_name_fromregion(&name1, &region1);
	dns_name_fromregion(&name2, &region2);
	return (dns_name_rdatacompare(&name1, &name2));
}

static inline isc_result_t
fromstruct_in_a6(ARGS_FROMSTRUCT) {
	dns_rdata_in_a6_t *a6 = source;
	isc_region_t region;
	int octets;
	isc_uint8_t bits;
	isc_uint8_t first;
	isc_uint8_t mask;

	REQUIRE(type == 38);
	REQUIRE(rdclass == 1);
	REQUIRE(source != NULL);
	REQUIRE(a6->common.rdtype == type);
	REQUIRE(a6->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	if (a6->prefixlen > 128)
		return (ISC_R_RANGE);

	RETERR(uint8_tobuffer(a6->prefixlen, target));

	/* Suffix */
	if (a6->prefixlen != 128) {
		octets = 16 - a6->prefixlen / 8;
		bits = a6->prefixlen % 8;
		if (bits != 0) {
			mask = 0xffU >> bits;
			first = a6->in6_addr.s6_addr[16 - octets] & mask;
			RETERR(uint8_tobuffer(first, target));
			octets--;
		}
		if (octets > 0)
			RETERR(mem_tobuffer(target,
					    a6->in6_addr.s6_addr + 16 - octets,
					    octets));
	}

	if (a6->prefixlen == 0)
		return (ISC_R_SUCCESS);
	dns_name_toregion(&a6->prefix, &region);
	return (isc_buffer_copyregion(target, &region));
}

static inline isc_result_t
tostruct_in_a6(ARGS_TOSTRUCT) {
	dns_rdata_in_a6_t *a6 = target;
	unsigned char octets;
	dns_name_t name;
	isc_region_t r;

	REQUIRE(rdata->type == 38);
	REQUIRE(rdata->rdclass == 1);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	a6->common.rdclass = rdata->rdclass;
	a6->common.rdtype = rdata->type;
	ISC_LINK_INIT(&a6->common, link);

	dns_rdata_toregion(rdata, &r);

	a6->prefixlen = uint8_fromregion(&r);
	isc_region_consume(&r, 1);
	memset(a6->in6_addr.s6_addr, 0, sizeof(a6->in6_addr.s6_addr));

	/*
	 * Suffix.
	 */
	if (a6->prefixlen != 128) {
		octets = 16 - a6->prefixlen / 8;
		INSIST(r.length >= octets);
		memcpy(a6->in6_addr.s6_addr + 16 - octets, r.base, octets);
		isc_region_consume(&r, octets);
	}

	/*
	 * Prefix.
	 */
	dns_name_init(&a6->prefix, NULL);
	if (a6->prefixlen != 0) {
		dns_name_init(&name, NULL);
		dns_name_fromregion(&name, &r);
		RETERR(name_duporclone(&name, mctx, &a6->prefix));
	}
	a6->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static inline void
freestruct_in_a6(ARGS_FREESTRUCT) {
	dns_rdata_in_a6_t *a6 = source;

	REQUIRE(source != NULL);
	REQUIRE(a6->common.rdclass == 1);
	REQUIRE(a6->common.rdtype == 38);

	if (a6->mctx == NULL)
		return;

	if (dns_name_dynamic(&a6->prefix))
		dns_name_free(&a6->prefix, a6->mctx);
	a6->mctx = NULL;
}

static inline isc_result_t
additionaldata_in_a6(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == 38);
	REQUIRE(rdata->rdclass == 1);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_in_a6(ARGS_DIGEST) {
	isc_region_t r1, r2;
	unsigned char prefixlen, octets;
	isc_result_t result;
	dns_name_t name;

	REQUIRE(rdata->type == 38);
	REQUIRE(rdata->rdclass == 1);

	dns_rdata_toregion(rdata, &r1);
	r2 = r1;
	prefixlen = r1.base[0];
	octets = 1 + 16 - prefixlen / 8;

	r1.length = octets;
	result = (digest)(arg, &r1);
	if (result != ISC_R_SUCCESS)
		return (result);
	if (prefixlen == 0)
		return (ISC_R_SUCCESS);

	isc_region_consume(&r2, octets);
	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &r2);
	return (dns_name_digest(&name, digest, arg));
}

static inline isc_boolean_t
checkowner_in_a6(ARGS_CHECKOWNER) {

	REQUIRE(type == 38);
	REQUIRE(rdclass == 1);

	UNUSED(type);
	UNUSED(rdclass);

	return (dns_name_ishostname(name, wildcard));
}

static inline isc_boolean_t
checknames_in_a6(ARGS_CHECKNAMES) {
	isc_region_t region;
	dns_name_t name;
	unsigned int prefixlen;

	REQUIRE(rdata->type == 38);
	REQUIRE(rdata->rdclass == 1);

	UNUSED(owner);

	dns_rdata_toregion(rdata, &region);
	prefixlen = uint8_fromregion(&region);
	if (prefixlen == 0)
		return (ISC_TRUE);
	isc_region_consume(&region, 1 + 16 - prefixlen / 8);
	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &region);
	if (!dns_name_ishostname(&name, ISC_FALSE)) {
		if (bad != NULL)
			dns_name_clone(&name, bad);
		return (ISC_FALSE);
	}
	return (ISC_TRUE);
}

#endif	/* RDATA_IN_1_A6_38_C */
