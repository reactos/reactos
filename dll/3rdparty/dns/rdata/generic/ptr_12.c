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

/* $Id: ptr_12.c,v 1.43 2007/06/19 23:47:17 tbox Exp $ */

/* Reviewed: Thu Mar 16 14:05:12 PST 2000 by explorer */

#ifndef RDATA_GENERIC_PTR_12_C
#define RDATA_GENERIC_PTR_12_C

#define RRTYPE_PTR_ATTRIBUTES (0)

static inline isc_result_t
fromtext_ptr(ARGS_FROMTEXT) {
	isc_token_t token;
	dns_name_t name;
	isc_buffer_t buffer;

	REQUIRE(type == 12);

	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(callbacks);

	RETERR(isc_lex_getmastertoken(lexer, &token, isc_tokentype_string,
				      ISC_FALSE));

	dns_name_init(&name, NULL);
	buffer_fromregion(&buffer, &token.value.as_region);
	origin = (origin != NULL) ? origin : dns_rootname;
	RETTOK(dns_name_fromtext(&name, &buffer, origin, options, target));
	if (rdclass == dns_rdataclass_in &&
	    (options & DNS_RDATA_CHECKNAMES) != 0 &&
	    (options & DNS_RDATA_CHECKREVERSE) != 0) {
		isc_boolean_t ok;
		ok = dns_name_ishostname(&name, ISC_FALSE);
		if (!ok && (options & DNS_RDATA_CHECKNAMESFAIL) != 0)
			RETTOK(DNS_R_BADNAME);
		if (!ok && callbacks != NULL)
			warn_badname(&name, lexer, callbacks);
	}
	return (ISC_R_SUCCESS);
}

static inline isc_result_t
totext_ptr(ARGS_TOTEXT) {
	isc_region_t region;
	dns_name_t name;
	dns_name_t prefix;
	isc_boolean_t sub;

	REQUIRE(rdata->type == 12);
	REQUIRE(rdata->length != 0);

	dns_name_init(&name, NULL);
	dns_name_init(&prefix, NULL);

	dns_rdata_toregion(rdata, &region);
	dns_name_fromregion(&name, &region);

	sub = name_prefix(&name, tctx->origin, &prefix);

	return (dns_name_totext(&prefix, sub, target));
}

static inline isc_result_t
fromwire_ptr(ARGS_FROMWIRE) {
        dns_name_t name;

	REQUIRE(type == 12);

	UNUSED(type);
	UNUSED(rdclass);

	dns_decompress_setmethods(dctx, DNS_COMPRESS_GLOBAL14);

        dns_name_init(&name, NULL);
        return (dns_name_fromwire(&name, source, dctx, options, target));
}

static inline isc_result_t
towire_ptr(ARGS_TOWIRE) {
	dns_name_t name;
	dns_offsets_t offsets;
	isc_region_t region;

	REQUIRE(rdata->type == 12);
	REQUIRE(rdata->length != 0);

	dns_compress_setmethods(cctx, DNS_COMPRESS_GLOBAL14);

	dns_name_init(&name, offsets);
	dns_rdata_toregion(rdata, &region);
	dns_name_fromregion(&name, &region);

	return (dns_name_towire(&name, cctx, target));
}

static inline int
compare_ptr(ARGS_COMPARE) {
	dns_name_t name1;
	dns_name_t name2;
	isc_region_t region1;
	isc_region_t region2;

	REQUIRE(rdata1->type == rdata2->type);
	REQUIRE(rdata1->rdclass == rdata2->rdclass);
	REQUIRE(rdata1->type == 12);
	REQUIRE(rdata1->length != 0);
	REQUIRE(rdata2->length != 0);

	dns_name_init(&name1, NULL);
	dns_name_init(&name2, NULL);

	dns_rdata_toregion(rdata1, &region1);
	dns_rdata_toregion(rdata2, &region2);

	dns_name_fromregion(&name1, &region1);
	dns_name_fromregion(&name2, &region2);

	return (dns_name_rdatacompare(&name1, &name2));
}

static inline isc_result_t
fromstruct_ptr(ARGS_FROMSTRUCT) {
	dns_rdata_ptr_t *ptr = source;
	isc_region_t region;

	REQUIRE(type == 12);
	REQUIRE(source != NULL);
	REQUIRE(ptr->common.rdtype == type);
	REQUIRE(ptr->common.rdclass == rdclass);

	UNUSED(type);
	UNUSED(rdclass);

	dns_name_toregion(&ptr->ptr, &region);
	return (isc_buffer_copyregion(target, &region));
}

static inline isc_result_t
tostruct_ptr(ARGS_TOSTRUCT) {
	isc_region_t region;
	dns_rdata_ptr_t *ptr = target;
	dns_name_t name;

	REQUIRE(rdata->type == 12);
	REQUIRE(target != NULL);
	REQUIRE(rdata->length != 0);

	ptr->common.rdclass = rdata->rdclass;
	ptr->common.rdtype = rdata->type;
	ISC_LINK_INIT(&ptr->common, link);

	dns_name_init(&name, NULL);
	dns_rdata_toregion(rdata, &region);
	dns_name_fromregion(&name, &region);
	dns_name_init(&ptr->ptr, NULL);
	RETERR(name_duporclone(&name, mctx, &ptr->ptr));
	ptr->mctx = mctx;
	return (ISC_R_SUCCESS);
}

static inline void
freestruct_ptr(ARGS_FREESTRUCT) {
	dns_rdata_ptr_t *ptr = source;

	REQUIRE(source != NULL);
	REQUIRE(ptr->common.rdtype == 12);

	if (ptr->mctx == NULL)
		return;

	dns_name_free(&ptr->ptr, ptr->mctx);
	ptr->mctx = NULL;
}

static inline isc_result_t
additionaldata_ptr(ARGS_ADDLDATA) {
	REQUIRE(rdata->type == 12);

	UNUSED(rdata);
	UNUSED(add);
	UNUSED(arg);

	return (ISC_R_SUCCESS);
}

static inline isc_result_t
digest_ptr(ARGS_DIGEST) {
	isc_region_t r;
	dns_name_t name;

	REQUIRE(rdata->type == 12);

	dns_rdata_toregion(rdata, &r);
	dns_name_init(&name, NULL);
	dns_name_fromregion(&name, &r);

	return (dns_name_digest(&name, digest, arg));
}

static inline isc_boolean_t
checkowner_ptr(ARGS_CHECKOWNER) {

	REQUIRE(type == 12);

	UNUSED(name);
	UNUSED(type);
	UNUSED(rdclass);
	UNUSED(wildcard);

	return (ISC_TRUE);
}

static unsigned char ip6_arpa_data[]  = "\003IP6\004ARPA";
static unsigned char ip6_arpa_offsets[] = { 0, 4, 9 };
static const dns_name_t ip6_arpa =
{
	DNS_NAME_MAGIC,
	ip6_arpa_data, 10, 3,
	DNS_NAMEATTR_READONLY | DNS_NAMEATTR_ABSOLUTE,
	ip6_arpa_offsets, NULL,
	{(void *)-1, (void *)-1},
	{NULL, NULL}
};

static unsigned char ip6_int_data[]  = "\003IP6\003INT";
static unsigned char ip6_int_offsets[] = { 0, 4, 8 };
static const dns_name_t ip6_int =
{
	DNS_NAME_MAGIC,
	ip6_int_data, 9, 3,
	DNS_NAMEATTR_READONLY | DNS_NAMEATTR_ABSOLUTE,
	ip6_int_offsets, NULL,
	{(void *)-1, (void *)-1},
	{NULL, NULL}
};

static unsigned char in_addr_arpa_data[]  = "\007IN-ADDR\004ARPA";
static unsigned char in_addr_arpa_offsets[] = { 0, 8, 13 };
static const dns_name_t in_addr_arpa =
{
	DNS_NAME_MAGIC,
	in_addr_arpa_data, 14, 3,
	DNS_NAMEATTR_READONLY | DNS_NAMEATTR_ABSOLUTE,
	in_addr_arpa_offsets, NULL,
	{(void *)-1, (void *)-1},
	{NULL, NULL}
};

static inline isc_boolean_t
checknames_ptr(ARGS_CHECKNAMES) {
	isc_region_t region;
	dns_name_t name;

	REQUIRE(rdata->type == 12);

	if (rdata->rdclass != dns_rdataclass_in)
	    return (ISC_TRUE);

	if (dns_name_issubdomain(owner, &in_addr_arpa) ||
	    dns_name_issubdomain(owner, &ip6_arpa) ||
	    dns_name_issubdomain(owner, &ip6_int)) {
		dns_rdata_toregion(rdata, &region);
		dns_name_init(&name, NULL);
		dns_name_fromregion(&name, &region);
		if (!dns_name_ishostname(&name, ISC_FALSE)) {
			if (bad != NULL)
				dns_name_clone(&name, bad);
			return (ISC_FALSE);
		}
	}
	return (ISC_TRUE);
}

#endif	/* RDATA_GENERIC_PTR_12_C */
