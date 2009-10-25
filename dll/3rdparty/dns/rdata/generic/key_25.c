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

/* $Id: key_25.c,v 1.51 2007/06/19 23:47:17 tbox Exp $ */

/*
 * Reviewed: Wed Mar 15 16:47:10 PST 2000 by halley.
 */

/* RFC2535 */

#ifndef RDATA_GENERIC_KEY_25_C
#define RDATA_GENERIC_KEY_25_C

#include <dst/dst.h>

#define RRTYPE_KEY_ATTRIBUTES (0)

static inline isc_result_t
fromtext_key(ARGS_FROMTEXT) {
	isc_token_t token;
	dns_secalg_t alg;
	dns_secproto_t proto;
	dns_keyflags_t flags;

	REQUIRE(type == 25);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(origin);
	UNUSED(options);
	UNUSED(callbacks);

	/* flags */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	RETTOK(dns_keyflags_fromtext(&flags, &token.value.as_textregion));
	RETERR(uint16_tobuffer(flags, target));

	/* protocol */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	RETTOK(dns_secproto_fromtext(&proto, &token.value.as_textregion));
	RETERR(mem_tobuffer(target, &proto, 1));

	/* algorithm */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));
	RETTOK(dns_secalg_fromtext(&alg, &token.value.as_textregion));
	RETERR(mem_tobuffer(target, &alg, 1));

	/* No Key? */
	if ((flags & 0xc000) == 0xc000)
		return (ISC_R_SUCCESS);

	return (isc_base64_tobuffer(lexer, target, -1));
}

static inline isc_result_t
totext_key(ARGS_TOTEXT) {
	isc_region_t sr;
	char buf[sizeof("64000")];
	unsigned int flags;
	unsigned char algorithm;

	REQUIRE(rdata->type == 25);
	REQUIRE(rdata->length != 0);

	dns_rdata_toregion(rdata, &sr);

	/* flags */
	flags = uint16_fromregion(&sr);
	isc_region_consume(&sr, 2);
	sprintf(buf, "%u", flags);
	RETERR(str_totext(buf, target));
	RETERR(str_totext(" ", target));

	/* protocol */
	sprintf(buf, "%u", sr.base[0]);
	isc_region_consume(&sr, 1);
	RETERR(str_totext(buf, target));
	RETERR(str_totext(" ", target));

	/* algorithm */
	algorithm = sr.base[0];
	sprintf(buf, "%u", algorithm);
	isc_region_consume(&sr, 1);
	RETERR(str_totext(buf, target));

	/* No Key? */
	if ((flags & 0xc000) == 0xc000)
		return (ISC_R_SUCCESS);

	/* key */
	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0)
		RETERR(str_totext(" (", target));
	RETERR(str_totext(tctx->linebreak, target));
	RETERR(isc_base64_totext(&sr, tctx->width - 2,
				 tctx->linebreak, target));

	if ((tctx->flags & DNS_STYLEFLAG_COMMENT) != 0)
		RETERR(str_totext(tctx->linebreak, target));
	else if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0)
		RETERR(str_totext(" ", target));

	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0)
		RETERR(str_totext(")", target));

	if ((tctx->flags & DNS_STYLEFLAG_COMMENT) != 0) {
		isc_region_t tmpr;

		RETERR(str_totext(" ; key id = ", target));
		dns_rdata_toregion(rdata, &tmpr);
		sprintf(buf, "%u", dst_region_computeid(&tmpr, algorithm));
		RETERR(str_totext(buf, target));
	}
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
fromwire_key(ARGS_FROMWIRE) {
	isc_region_t sr;

	REQUIRE(type == 25);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(dctx);
	UNUSED(options);

	isc_buffer_activeregion(source, &sr);
	if (sr.length < 4)
		return (ISC_R_UNEXPECTEDEND);

	isc_buffer_forward(source, sr.length);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static inline isc_result_t
towire_key(ARGS_TOWIRE) {
	isc_region_t sr;

	REQUIRE(rdata->type == 25);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	dns_rdata_toregion(rdata, &sr);
	return (mem_tobuffer(target, sr.base, sr.length));
}

static inline int
compare_key(ARGS_COMPARE) {
	isc_region_t r1;
	isc_region_t r2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 25);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_rdata_toregion(rdata1, &r1);
	dns_rdata_toregion(rdata2, &r2);
	return (isc_region_compare(&r1, &r2));
}

static inline isc_result_t
fromstruct_key(ARGS_FROMSTRUCT) {
	dns_rdata_key_t *key = source;

	REQUIRE(type == 25);
	REQUIRE(source != NULL);
	REQUIRE(key->common.rdtype == type);
	REQUIRE(key->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	/* Flags */
	RETERR(uint16_tobuffer(key->flags, target));

	/* Protocol */
	RETERR(uint8_tobuffer(key->protocol, target));

	/* Algorithm */
	RETERR(uint8_tobuffer(key->algorithm, target));

	/* Data */
	return (mem_tobuffer(target, key->data, key->datalen));
}

static inline isc_result_t
tostruct_key(ARGS_TOSTRUCT) {
	dns_rdata_key_t *key = target;
	isc_region_t sr;

	REQUIRE(rdata->type == 25);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	key->common.rdclass = rdata->rdclass;
	key->common.rdtype = rdata->type;
	ISC_LINK_INIT(&key->common, link);

	dns_rdata_toregion(rdata, &sr);

	/* Flags */
	if (sr.length < 2)
		return (ISC_R_UNEXPECTEDEND);
	key->flags = uint16_fromregion(&sr);
	isc_region_consume(&sr, 2);

	/* Protocol */
	if (sr.length < 1)
		return (ISC_R_UNEXPECTEDEND);
	key->protocol = uint8_fromregion(&sr);
	isc_region_consume(&sr, 1);

	/* Algorithm */
	if (sr.length < 1)
		return (ISC_R_UNEXPECTEDEND);
	key->algorithm = uint8_fromregion(&sr);
	isc_region_consume(&sr, 1);

	/* Data */
	key->datalen = sr.length;
	key->data = mem_maybedup(mctx, sr.base, key->datalen);
	if (key->data == NULL)
		return (ISC_R_NOMEMORY);

	key->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static inline void
freestruct_key(ARGS_FREESTRUCT) {
	dns_rdata_key_t *key = (dns_rdata_key_t *) source;

	REQUIRE(source != NULL);
	REQUIRE(key->common.rdtype == 25);

	if (key->mctx == NULL)
		return;

	if (key->data != NULL)
		isc_mem_free(key->mctx, key->data);
	key->mctx = NULL;
}

static inline isc_result_t
additionaldata_key(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == 25);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_key(ARGS_DIGEST) {
	isc_region_t r;

	REQUIRE(rdata->type == 25);

	dns_rdata_toregion(rdata, &r);

	return ((digest)(arg, &r));
}

static inline isc_boolean_t
checkowner_key(ARGS_CHECKOWNER) {

	REQUIRE(type == 25);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_key(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == 25);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

#endif	/* RDATA_GENERIC_KEY_25_C */
