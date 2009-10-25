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

/* $Id: nsec3param_51.c,v 1.4.48.2 2009/01/18 23:47:41 tbox Exp $ */

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

#ifndef RDATA_GENERIC_NSEC3PARAM_51_C
#define RDATA_GENERIC_NSEC3PARAM_51_C

#include <isc/iterated_hash.h>
#include <isc/base32.h>

#define RRTYPE_NSEC3PARAM_ATTRIBUTES (DNS_RDATATYPEATTR_DNSSEC)

static inline isc_result_t
fromtext_nsec3param(ARGS_FROMTEXT) {
	isc_token_t token;
	unsigned int flags = 0;
	unsigned char hashalg;

	REQUIRE(type == 51);

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

	/* Salt. */
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

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
totext_nsec3param(ARGS_TOTEXT) {
	isc_region_t sr;
	unsigned int i, j;
	unsigned char hash;
	unsigned char flags;
	char buf[sizeof("65535 ")];
	isc_uint32_t iterations;

	REQUIRE(rdata->type == 51);
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
	} else
		RETERR(str_totext("-", target));

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
fromwire_nsec3param(ARGS_FROMWIRE) {
	isc_region_t sr, rr;
	unsigned int saltlen;

	REQUIRE(type == 51);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(options);
	UNUSED(dctx);

	isc_buffer_activeregion(source, &sr);
	rr = sr;

	/* hash(1), flags(1), iterations(2), saltlen(1) */
	if (sr.length < 5U)
		RETERR(DNS_R_FORMERR);
	saltlen = sr.base[4];
	isc_region_consume(&sr, 5);

	if (sr.length < saltlen)
		RETERR(DNS_R_FORMERR);
	isc_region_consume(&sr, saltlen);
	RETERR(mem_tobuffer(target, rr.base, rr.length));
	isc_buffer_forward(source, rr.length);
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
towire_nsec3param(ARGS_TOWIRE) {
	isc_region_t sr;

	REQUIRE(rdata->type == 51);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	dns_rdata_toregion(rdata, &sr);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static inline int
compare_nsec3param(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 51);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_nsec3param(ARGS_FROMSTRUCT) {
	dns_rdata_nsec3param_t *nsec3param = source;

	REQUIRE(type == 51);
	REQUIRE(source != NULL);
	REQUIRE(nsec3param->common.rdtype == type);
	REQUIRE(nsec3param->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	RETERR(uint8_tobuffer(nsec3param->hash, target));
	RETERR(uint8_tobuffer(nsec3param->flags, target));
	RETERR(uint16_tobuffer(nsec3param->iterations, target));
	RETERR(uint8_tobuffer(nsec3param->salt_length, target));
	RETERR(mem_tobuffer(target, nsec3param->salt,
			    nsec3param->salt_length));
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
tostruct_nsec3param(ARGS_TOSTRUCT) {
	isc_region_t region;
	dns_rdata_nsec3param_t *nsec3param = target;

	REQUIRE(rdata->type == 51);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	nsec3param->common.rdclass = rdata->rdclass;
	nsec3param->common.rdtype = rdata->type;
	ISC_LINK_INIT(&nsec3param->common, link);

	region.base = rdata->data;
	region.length = rdata->length;
	nsec3param->hash = uint8_consume_fromregion(&region);
	nsec3param->flags = uint8_consume_fromregion(&region);
	nsec3param->iterations = uint16_consume_fromregion(&region);

	nsec3param->salt_length = uint8_consume_fromregion(&region);
	nsec3param->salt = mem_maybedup(mctx, region.base,
					nsec3param->salt_length);
	if (nsec3param->salt == NULL)
		return (ISC_R_NOMEMORY);
	isc_region_consume(&region, nsec3param->salt_length);

	nsec3param->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static inline void
freestruct_nsec3param(ARGS_FREESTRUCT) {
	dns_rdata_nsec3param_t *nsec3param = source;

	REQUIRE(source != NULL);
	REQUIRE(nsec3param->common.rdtype == 51);

	if (nsec3param->mctx == NULL)
		return;

	if (nsec3param->salt != NULL)
		isc_mem_free(nsec3param->mctx, nsec3param->salt);
	nsec3param->mctx = NULL;
}

static inline isc_result_t
additionaldata_nsec3param(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == 51);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_nsec3param(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == 51);

	dns_rdata_toregion(rdata, &r);
	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_nsec3param(ARGS_CHECKOWNER) {

       REQUIRE(type == 51);

       UNUSED(name);
       UNUSED(type);
       UNUSED(rdclass);
       UNUSED(wildcard);

       return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_nsec3param(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == 51);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

#endif	/* RDATA_GENERIC_NSEC3PARAM_51_C */
