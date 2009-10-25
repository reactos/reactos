/*
 * Copyright (C) 2005, 2007, 2009  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: ipseckey_45.c,v 1.4.332.2 2009/01/18 23:47:41 tbox Exp $ */

#ifndef RDATA_GENERIC_IPSECKEY_45_C
#define RDATA_GENERIC_IPSECKEY_45_C

#include <string.h>

#include <isc/net.h>

#define RRTYPE_IPSECKEY_ATTRIBUTES (0)

static inline isc_result_t
fromtext_ipseckey(ARGS_FROMTEXT) {
	isc_token_t token;
	dns_name_t name;
	isc_buffer_t buffer;
	unsigned int gateway;
	struct in_addr addr;
	unsigned char addr6[16];
	isc_region_t region;

	REQUIRE(type == 45);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(callbacks);

	/*
	 * Precedence.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      ISC_FALSE));
	if (token.value.as_ulong > 0xffU)
		RETTOK(ISC_R_RANGE);
	RETERR(uint8_tobuffer(token.value.as_ulong, target));

	/*
	 * Gateway type.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      ISC_FALSE));
	if (token.value.as_ulong > 0x3U)
		RETTOK(ISC_R_RANGE);
	RETERR(uint8_tobuffer(token.value.as_ulong, target));
	gateway = token.value.as_ulong;

	/*
	 * Algorithm.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_number,
				      ISC_FALSE));
	if (token.value.as_ulong > 0xffU)
		RETTOK(ISC_R_RANGE);
	RETERR(uint8_tobuffer(token.value.as_ulong, target));

	/*
	 * Gateway.
	 */
	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));

	switch (gateway) {
	case 0:
		if (strcmp(DNS_AS_STR(token), ".") != 0)
			RETTOK(DNS_R_SYNTAX);
		break;

	case 1:
		if (getquad(DNS_AS_STR(token), &addr, lexer, callbacks) != 1)
			RETTOK(DNS_R_BADDOTTEDQUAD);
		isc_buffer_availableregion(target, &region);
		if (region.length < 4)
			return (ISC_R_NOSPACE);
		memcpy(region.base, &addr, 4);
		isc_buffer_add(target, 4);
		break;

	case 2:
		if (inet_pton(AF_INET6, DNS_AS_STR(token), addr6) != 1)
			RETTOK(DNS_R_BADAAAA);
		isc_buffer_availableregion(target, &region);
		if (region.length < 16)
			return (ISC_R_NOSPACE);
		memcpy(region.base, addr6, 16);
		isc_buffer_add(target, 16);
		break;

	case 3:
		dns_name_init(&name, NULL);
		buffer_fromregion(&buffer, &token.value.as_region);
		origin = (origin != NULL) ? origin : dns_rootname;
		RETTOK(dns_name_fromtext(&name, &buffer, origin,
					 options, target));
		break;
	}

	/*
	 * Public key.
	 */
	return (isc_base64_tobuffer(lexer, target, -1));
}

static inline isc_result_t
totext_ipseckey(ARGS_TOTEXT) {
	isc_region_t region;
	dns_name_t name;
	dns_name_t prefix;
	isc_boolean_t sub;
	char buf[sizeof("255 ")];
	unsigned short num;
	unsigned short gateway;

	REQUIRE(rdata->type == 45);
	REQUIRE(rdata->length >= 3);

	dns_name_init(&name, NULL);
	dns_name_init(&prefix, NULL);

	if (rdata->data[1] > 3U)
		return (ISC_R_NOTIMPLEMENTED);

	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0)
		RETERR(str_totext("( ", target));

	/*
	 * Precedence.
	 */
	dns_rdata_toregion(rdata, &region);
	num = uint8_fromregion(&region);
	isc_region_consume(&region, 1);
	sprintf(buf, "%u ", num);
	RETERR(str_totext(buf, target));

	/*
	 * Gateway type.
	 */
	gateway = uint8_fromregion(&region);
	isc_region_consume(&region, 1);
	sprintf(buf, "%u ", gateway);
	RETERR(str_totext(buf, target));

	/*
	 * Algorithm.
	 */
	num = uint8_fromregion(&region);
	isc_region_consume(&region, 1);
	sprintf(buf, "%u ", num);
	RETERR(str_totext(buf, target));

	/*
	 * Gateway.
	 */
	switch (gateway) {
	case 0:
		RETERR(str_totext(".", target));
		break;

	case 1:
		RETERR(inet_totext(AF_INET, &region, target));
		isc_region_consume(&region, 4);
		break;

	case 2:
		RETERR(inet_totext(AF_INET6, &region, target));
		isc_region_consume(&region, 16);
		break;

	case 3:
		dns_name_fromregion(&name, &region);
		sub = name_prefix(&name, tctx->origin, &prefix);
		RETERR(dns_name_totext(&prefix, sub, target));
		isc_region_consume(&region, name_length(&name));
		break;
	}

	/*
	 * Key.
	 */
	if (region.length > 0U) {
		RETERR(str_totext(tctx->linebreak, target));
		RETERR(isc_base64_totext(&region, tctx->width - 2,
					 tctx->linebreak, target));
	}

	if ((tctx->flags & DNS_STYLEFLAG_MULTILINE) != 0)
		RETERR(str_totext(" )", target));
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
fromwire_ipseckey(ARGS_FROMWIRE) {
	dns_name_t name;
	isc_region_t region;

	REQUIRE(type == 45);

	UNUSED(type);
	UNUSED(rdclass);

	dns_decompress_setmethods(dctx, DNS_COMPRESS_NONE);

	dns_name_init(&name, NULL);

	isc_buffer_activeregion(source, &region);
	if (region.length < 3)
		return (ISC_R_UNEXPECTEDEND);

	switch (region.base[1]) {
	case 0:
		isc_buffer_forward(source, region.length);
		return (mem_tobuffer(target, region.base, region.length));

	case 1:
		if (region.length < 7)
			return (ISC_R_UNEXPECTEDEND);
		isc_buffer_forward(source, region.length);
		return (mem_tobuffer(target, region.base, region.length));

	case 2:
		if (region.length < 19)
			return (ISC_R_UNEXPECTEDEND);
		isc_buffer_forward(source, region.length);
		return (mem_tobuffer(target, region.base, region.length));

	case 3:
		RETERR(mem_tobuffer(target, region.base, 3));
		isc_buffer_forward(source, 3);
		RETERR(dns_name_fromwire(&name, source, dctx, options, target));
		isc_buffer_activeregion(source, &region);
		return(mem_tobuffer(target, region.base, region.length));

	default:
		return (ISC_R_NOTIMPLEMENTED);
	}
}

static inline isc_result_t
towire_ipseckey(ARGS_TOWIRE) {
	isc_region_t region;

	REQUIRE(rdata->type == 45);
	REQUIRE(rdata->length != 0);

	UNUSED(cctx);

	dns_rdata_toregion(rdata, &region);
	return (mem_tobuffer(target, region.base, region.length));
}

static inline int
compare_ipseckey(ARGS_COMPARE) {
	isc_region_t region1;
	isc_region_t region2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 45);
	REQUIRE(rdata1->length >= 3);
	REQUIRE(rdata2->length >= 3);

	dns_rdata_toregion(rdata1, &region1);
	dns_rdata_toregion(rdata2, &region2);

	return (isc_region_compare(&region1, &region2));
}

static inline isc_result_t
fromstruct_ipseckey(ARGS_FROMSTRUCT) {
	dns_rdata_ipseckey_t *ipseckey = source;
	isc_region_t region;
	isc_uint32_t n;

	REQUIRE(type == 45);
	REQUIRE(source != NULL);
	REQUIRE(ipseckey->common.rdtype == type);
	REQUIRE(ipseckey->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	if (ipseckey->gateway_type > 3U)
		return (ISC_R_NOTIMPLEMENTED);

	RETERR(uint8_tobuffer(ipseckey->precedence, target));
	RETERR(uint8_tobuffer(ipseckey->gateway_type, target));
	RETERR(uint8_tobuffer(ipseckey->algorithm, target));

	switch  (ipseckey->gateway_type) {
	case 0:
		break;

	case 1:
		n = ntohl(ipseckey->in_addr.s_addr);
		RETERR(uint32_tobuffer(n, target));
		break;

	case 2:
		RETERR(mem_tobuffer(target, ipseckey->in6_addr.s6_addr, 16));
		break;

	case 3:
		dns_name_toregion(&ipseckey->gateway, &region);
		RETERR(isc_buffer_copyregion(target, &region));
		break;
	}

	return (mem_tobuffer(target, ipseckey->key, ipseckey->keylength));
}

static inline isc_result_t
tostruct_ipseckey(ARGS_TOSTRUCT) {
	isc_region_t region;
	dns_rdata_ipseckey_t *ipseckey = target;
	dns_name_t name;
	isc_uint32_t n;

	REQUIRE(rdata->type == 45);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length >= 3);

	if (rdata->data[1] > 3U)
		return (ISC_R_NOTIMPLEMENTED);

	ipseckey->common.rdclass = rdata->rdclass;
	ipseckey->common.rdtype = rdata->type;
	ISC_LINK_INIT(&ipseckey->common, link);

	dns_name_init(&name, NULL);
	dns_rdata_toregion(rdata, &region);

	ipseckey->precedence = uint8_fromregion(&region);
	isc_region_consume(&region, 1);

	ipseckey->gateway_type = uint8_fromregion(&region);
	isc_region_consume(&region, 1);

	ipseckey->algorithm = uint8_fromregion(&region);
	isc_region_consume(&region, 1);

	switch (ipseckey->gateway_type) {
	case 0:
		break;

	case 1:
		n = uint32_fromregion(&region);
		ipseckey->in_addr.s_addr = htonl(n);
		isc_region_consume(&region, 4);
		break;

	case 2:
		memcpy(ipseckey->in6_addr.s6_addr, region.base, 16);
		isc_region_consume(&region, 16);
		break;

	case 3:
		dns_name_init(&ipseckey->gateway, NULL);
		dns_name_fromregion(&name, &region);
		RETERR(name_duporclone(&name, mctx, &ipseckey->gateway));
		isc_region_consume(&region, name_length(&name));
		break;
	}

	ipseckey->keylength = region.length;
	if (ipseckey->keylength != 0U) {
		ipseckey->key = mem_maybedup(mctx, region.base,
					     ipseckey->keylength);
		if (ipseckey->key == NULL) {
			if (ipseckey->gateway_type == 3)
				dns_name_free(&ipseckey->gateway,
					      ipseckey->mctx);
			return (ISC_R_NOMEMORY);
		}
	} else
		ipseckey->key = NULL;

	ipseckey->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static inline void
freestruct_ipseckey(ARGS_FREESTRUCT) {
	dns_rdata_ipseckey_t *ipseckey = source;

	REQUIRE(source != NULL);
	REQUIRE(ipseckey->common.rdtype == 45);

	if (ipseckey->mctx == NULL)
		return;

	if (ipseckey->gateway_type == 3)
		dns_name_free(&ipseckey->gateway, ipseckey->mctx);

	if (ipseckey->key != NULL)
		isc_mem_free(ipseckey->mctx, ipseckey->key);

	ipseckey->mctx = NULL;
}

static inline isc_result_t
additionaldata_ipseckey(ARGS_ADDLDATA) {

	REQUIRE(rdata->type == 45);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_ipseckey(ARGS_DIGEST) {
	isc_region_t region;

	REQUIRE(rdata->type == 45);

	dns_rdata_toregion(rdata, &region);
	return ((digest)(arg, &region));
}

static inline isc_boolean_t
checkowner_ipseckey(ARGS_CHECKOWNER) {

	REQUIRE(type == 45);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static inline isc_boolean_t
checknames_ipseckey(ARGS_CHECKNAMES) {

	REQUIRE(rdata->type == 45);

	UNUSED(rdata);
	UNUSED(owner);
	UNUSED(bad);

	return (ISC_TRUE);
}

#endif	/* RDATA_GENERIC_IPSECKEY_45_C */
