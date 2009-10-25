/*
 * Copyright (C) 2004-2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2002, 2003  Internet Software Consortium.
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

/* $Id: ds.c,v 1.11 2007/06/19 23:47:16 tbox Exp $ */

/*! \file */

#include <config.h>

#include <string.h>

#include <isc/buffer.h>
#include <isc/region.h>
#include <isc/sha1.h>
#include <isc/sha2.h>
#include <isc/util.h>

#include <dns/ds.h>
#include <dns/fixedname.h>
#include <dns/name.h>
#include <dns/rdata.h>
#include <dns/rdatastruct.h>
#include <dns/result.h>

#include <dst/dst.h>

isc_result_t
dns_ds_buildrdata(dns_name_t *owner, dns_rdata_t *key,
		  unsigned int digest_type, unsigned char *buffer,
		  dns_rdata_t *rdata)
{
	dns_fixedname_t fname;
	dns_name_t *name;
	unsigned char digest[ISC_SHA256_DIGESTLENGTH];
	isc_region_t r;
	isc_buffer_t b;
	dns_rdata_ds_t ds;

	REQUIRE(key != NULL);
	REQUIRE(key->type == dns_rdatatype_dnskey);

	if (!dns_ds_digest_supported(digest_type))
		return (ISC_R_NOTIMPLEMENTED);

	dns_fixedname_init(&fname);
	name = dns_fixedname_name(&fname);
	(void)dns_name_downcase(owner, name, NULL);

	memset(buffer, 0, DNS_DS_BUFFERSIZE);
	isc_buffer_init(&b, buffer, DNS_DS_BUFFERSIZE);

	if (digest_type == DNS_DSDIGEST_SHA1) {
		isc_sha1_t sha1;
		isc_sha1_init(&sha1);
		dns_name_toregion(name, &r);
		isc_sha1_update(&sha1, r.base, r.length);
		dns_rdata_toregion(key, &r);
		INSIST(r.length >= 4);
		isc_sha1_update(&sha1, r.base, r.length);
		isc_sha1_final(&sha1, digest);
	} else {
		isc_sha256_t sha256;
		isc_sha256_init(&sha256);
		dns_name_toregion(name, &r);
		isc_sha256_update(&sha256, r.base, r.length);
		dns_rdata_toregion(key, &r);
		INSIST(r.length >= 4);
		isc_sha256_update(&sha256, r.base, r.length);
		isc_sha256_final(digest, &sha256);
	}

	ds.mctx = NULL;
	ds.common.rdclass = key->rdclass;
	ds.common.rdtype = dns_rdatatype_ds;
	ds.algorithm = r.base[3];
	ds.key_tag = dst_region_computeid(&r, ds.algorithm);
	ds.digest_type = digest_type;
	ds.length = (digest_type == DNS_DSDIGEST_SHA1) ?
		    ISC_SHA1_DIGESTLENGTH : ISC_SHA256_DIGESTLENGTH;
	ds.digest = digest;

	return (dns_rdata_fromstruct(rdata, key->rdclass, dns_rdatatype_ds,
				     &ds, &b));
}

isc_boolean_t
dns_ds_digest_supported(unsigned int digest_type) {
	return  (ISC_TF(digest_type == DNS_DSDIGEST_SHA1 ||
			digest_type == DNS_DSDIGEST_SHA256));
}
