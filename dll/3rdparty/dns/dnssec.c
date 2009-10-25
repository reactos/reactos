/*
 * Copyright (C) 2004-2009  Internet Systems Consortium, Inc. ("ISC")
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

/*
 * $Id: dnssec.c,v 1.93.12.4 2009/06/08 23:47:00 tbox Exp $
 */

/*! \file */

#include <config.h>

#include <stdlib.h>

#include <isc/buffer.h>
#include <isc/mem.h>
#include <isc/serial.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/db.h>
#include <dns/dnssec.h>
#include <dns/fixedname.h>
#include <dns/keyvalues.h>
#include <dns/message.h>
#include <dns/rdata.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/result.h>
#include <dns/tsig.h>		/* for DNS_TSIG_FUDGE */

#include <dst/result.h>

#define is_response(msg) (msg->flags & DNS_MESSAGEFLAG_QR)

#define RETERR(x) do { \
	result = (x); \
	if (result != ISC_R_SUCCESS) \
		goto failure; \
	} while (0)


#define TYPE_SIGN 0
#define TYPE_VERIFY 1

static isc_result_t
digest_callback(void *arg, isc_region_t *data);

static int
rdata_compare_wrapper(const void *rdata1, const void *rdata2);

static isc_result_t
rdataset_to_sortedarray(dns_rdataset_t *set, isc_mem_t *mctx,
			dns_rdata_t **rdata, int *nrdata);

static isc_result_t
digest_callback(void *arg, isc_region_t *data) {
	dst_context_t *ctx = arg;

	return (dst_context_adddata(ctx, data));
}

/*
 * Make qsort happy.
 */
static int
rdata_compare_wrapper(const void *rdata1, const void *rdata2) {
	return (dns_rdata_compare((const dns_rdata_t *)rdata1,
				  (const dns_rdata_t *)rdata2));
}

/*
 * Sort the rdataset into an array.
 */
static isc_result_t
rdataset_to_sortedarray(dns_rdataset_t *set, isc_mem_t *mctx,
			dns_rdata_t **rdata, int *nrdata)
{
	isc_result_t ret;
	int i = 0, n;
	dns_rdata_t *data;

	n = dns_rdataset_count(set);

	data = isc_mem_get(mctx, n * sizeof(dns_rdata_t));
	if (data == NULL)
		return (ISC_R_NOMEMORY);

	ret = dns_rdataset_first(set);
	if (ret != ISC_R_SUCCESS) {
		isc_mem_put(mctx, data, n * sizeof(dns_rdata_t));
		return (ret);
	}

	/*
	 * Put them in the array.
	 */
	do {
		dns_rdata_init(&data[i]);
		dns_rdataset_current(set, &data[i++]);
	} while (dns_rdataset_next(set) == ISC_R_SUCCESS);

	/*
	 * Sort the array.
	 */
	qsort(data, n, sizeof(dns_rdata_t), rdata_compare_wrapper);
	*rdata = data;
	*nrdata = n;
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_dnssec_keyfromrdata(dns_name_t *name, dns_rdata_t *rdata, isc_mem_t *mctx,
			dst_key_t **key)
{
	isc_buffer_t b;
	isc_region_t r;

	INSIST(name != NULL);
	INSIST(rdata != NULL);
	INSIST(mctx != NULL);
	INSIST(key != NULL);
	INSIST(*key == NULL);
	REQUIRE(rdata->type == dns_rdatatype_key ||
		rdata->type == dns_rdatatype_dnskey);

	dns_rdata_toregion(rdata, &r);
	isc_buffer_init(&b, r.base, r.length);
	isc_buffer_add(&b, r.length);
	return (dst_key_fromdns(name, rdata->rdclass, &b, mctx, key));
}

static isc_result_t
digest_sig(dst_context_t *ctx, dns_rdata_t *sigrdata, dns_rdata_rrsig_t *sig) {
	isc_region_t r;
	isc_result_t ret;
	dns_fixedname_t fname;

	dns_rdata_toregion(sigrdata, &r);
	INSIST(r.length >= 19);

	r.length = 18;
	ret = dst_context_adddata(ctx, &r);
	if (ret != ISC_R_SUCCESS)
		return (ret);
	dns_fixedname_init(&fname);
	RUNTIME_CHECK(dns_name_downcase(&sig->signer,
					dns_fixedname_name(&fname), NULL)
		      == ISC_R_SUCCESS);
	dns_name_toregion(dns_fixedname_name(&fname), &r);
	return (dst_context_adddata(ctx, &r));
}

isc_result_t
dns_dnssec_sign(dns_name_t *name, dns_rdataset_t *set, dst_key_t *key,
		isc_stdtime_t *inception, isc_stdtime_t *expire,
		isc_mem_t *mctx, isc_buffer_t *buffer, dns_rdata_t *sigrdata)
{
	dns_rdata_rrsig_t sig;
	dns_rdata_t tmpsigrdata;
	dns_rdata_t *rdatas;
	int nrdatas, i;
	isc_buffer_t sigbuf, envbuf;
	isc_region_t r;
	dst_context_t *ctx = NULL;
	isc_result_t ret;
	isc_buffer_t *databuf = NULL;
	char data[256 + 8];
	isc_uint32_t flags;
	unsigned int sigsize;
	dns_fixedname_t fnewname;

	REQUIRE(name != NULL);
	REQUIRE(dns_name_countlabels(name) <= 255);
	REQUIRE(set != NULL);
	REQUIRE(key != NULL);
	REQUIRE(inception != NULL);
	REQUIRE(expire != NULL);
	REQUIRE(mctx != NULL);
	REQUIRE(sigrdata != NULL);

	if (*inception >= *expire)
		return (DNS_R_INVALIDTIME);

	/*
	 * Is the key allowed to sign data?
	 */
	flags = dst_key_flags(key);
	if (flags & DNS_KEYTYPE_NOAUTH)
		return (DNS_R_KEYUNAUTHORIZED);
	if ((flags & DNS_KEYFLAG_OWNERMASK) != DNS_KEYOWNER_ZONE)
		return (DNS_R_KEYUNAUTHORIZED);

	sig.mctx = mctx;
	sig.common.rdclass = set->rdclass;
	sig.common.rdtype = dns_rdatatype_rrsig;
	ISC_LINK_INIT(&sig.common, link);

	dns_name_init(&sig.signer, NULL);
	dns_name_clone(dst_key_name(key), &sig.signer);

	sig.covered = set->type;
	sig.algorithm = dst_key_alg(key);
	sig.labels = dns_name_countlabels(name) - 1;
	if (dns_name_iswildcard(name))
		sig.labels--;
	sig.originalttl = set->ttl;
	sig.timesigned = *inception;
	sig.timeexpire = *expire;
	sig.keyid = dst_key_id(key);
	ret = dst_key_sigsize(key, &sigsize);
	if (ret != ISC_R_SUCCESS)
		return (ret);
	sig.siglen = sigsize;
	/*
	 * The actual contents of sig.signature are not important yet, since
	 * they're not used in digest_sig().
	 */
	sig.signature = isc_mem_get(mctx, sig.siglen);
	if (sig.signature == NULL)
		return (ISC_R_NOMEMORY);

	ret = isc_buffer_allocate(mctx, &databuf, sigsize + 256 + 18);
	if (ret != ISC_R_SUCCESS)
		goto cleanup_signature;

	dns_rdata_init(&tmpsigrdata);
	ret = dns_rdata_fromstruct(&tmpsigrdata, sig.common.rdclass,
				   sig.common.rdtype, &sig, databuf);
	if (ret != ISC_R_SUCCESS)
		goto cleanup_databuf;

	ret = dst_context_create(key, mctx, &ctx);
	if (ret != ISC_R_SUCCESS)
		goto cleanup_databuf;

	/*
	 * Digest the SIG rdata.
	 */
	ret = digest_sig(ctx, &tmpsigrdata, &sig);
	if (ret != ISC_R_SUCCESS)
		goto cleanup_context;

	dns_fixedname_init(&fnewname);
	RUNTIME_CHECK(dns_name_downcase(name, dns_fixedname_name(&fnewname),
					NULL) == ISC_R_SUCCESS);
	dns_name_toregion(dns_fixedname_name(&fnewname), &r);

	/*
	 * Create an envelope for each rdata: <name|type|class|ttl>.
	 */
	isc_buffer_init(&envbuf, data, sizeof(data));
	memcpy(data, r.base, r.length);
	isc_buffer_add(&envbuf, r.length);
	isc_buffer_putuint16(&envbuf, set->type);
	isc_buffer_putuint16(&envbuf, set->rdclass);
	isc_buffer_putuint32(&envbuf, set->ttl);

	ret = rdataset_to_sortedarray(set, mctx, &rdatas, &nrdatas);
	if (ret != ISC_R_SUCCESS)
		goto cleanup_context;
	isc_buffer_usedregion(&envbuf, &r);

	for (i = 0; i < nrdatas; i++) {
		isc_uint16_t len;
		isc_buffer_t lenbuf;
		isc_region_t lenr;

		/*
		 * Skip duplicates.
		 */
		if (i > 0 && dns_rdata_compare(&rdatas[i], &rdatas[i-1]) == 0)
		    continue;

		/*
		 * Digest the envelope.
		 */
		ret = dst_context_adddata(ctx, &r);
		if (ret != ISC_R_SUCCESS)
			goto cleanup_array;

		/*
		 * Digest the length of the rdata.
		 */
		isc_buffer_init(&lenbuf, &len, sizeof(len));
		INSIST(rdatas[i].length < 65536);
		isc_buffer_putuint16(&lenbuf, (isc_uint16_t)rdatas[i].length);
		isc_buffer_usedregion(&lenbuf, &lenr);
		ret = dst_context_adddata(ctx, &lenr);
		if (ret != ISC_R_SUCCESS)
			goto cleanup_array;

		/*
		 * Digest the rdata.
		 */
		ret = dns_rdata_digest(&rdatas[i], digest_callback, ctx);
		if (ret != ISC_R_SUCCESS)
			goto cleanup_array;
	}

	isc_buffer_init(&sigbuf, sig.signature, sig.siglen);
	ret = dst_context_sign(ctx, &sigbuf);
	if (ret != ISC_R_SUCCESS)
		goto cleanup_array;
	isc_buffer_usedregion(&sigbuf, &r);
	if (r.length != sig.siglen) {
		ret = ISC_R_NOSPACE;
		goto cleanup_array;
	}
	memcpy(sig.signature, r.base, sig.siglen);

	ret = dns_rdata_fromstruct(sigrdata, sig.common.rdclass,
				  sig.common.rdtype, &sig, buffer);

cleanup_array:
	isc_mem_put(mctx, rdatas, nrdatas * sizeof(dns_rdata_t));
cleanup_context:
	dst_context_destroy(&ctx);
cleanup_databuf:
	isc_buffer_free(&databuf);
cleanup_signature:
	isc_mem_put(mctx, sig.signature, sig.siglen);

	return (ret);
}

isc_result_t
dns_dnssec_verify2(dns_name_t *name, dns_rdataset_t *set, dst_key_t *key,
		   isc_boolean_t ignoretime, isc_mem_t *mctx,
		   dns_rdata_t *sigrdata, dns_name_t *wild)
{
	dns_rdata_rrsig_t sig;
	dns_fixedname_t fnewname;
	isc_region_t r;
	isc_buffer_t envbuf;
	dns_rdata_t *rdatas;
	int nrdatas, i;
	isc_stdtime_t now;
	isc_result_t ret;
	unsigned char data[300];
	dst_context_t *ctx = NULL;
	int labels = 0;
	isc_uint32_t flags;

	REQUIRE(name != NULL);
	REQUIRE(set != NULL);
	REQUIRE(key != NULL);
	REQUIRE(mctx != NULL);
	REQUIRE(sigrdata != NULL && sigrdata->type == dns_rdatatype_rrsig);

	ret = dns_rdata_tostruct(sigrdata, &sig, NULL);
	if (ret != ISC_R_SUCCESS)
		return (ret);

	if (set->type != sig.covered)
		return (DNS_R_SIGINVALID);

	if (isc_serial_lt(sig.timeexpire, sig.timesigned))
		return (DNS_R_SIGINVALID);

	if (!ignoretime) {
		isc_stdtime_get(&now);

		/*
		 * Is SIG temporally valid?
		 */
		if (isc_serial_lt((isc_uint32_t)now, sig.timesigned))
			return (DNS_R_SIGFUTURE);
		else if (isc_serial_lt(sig.timeexpire, (isc_uint32_t)now))
			return (DNS_R_SIGEXPIRED);
	}

	/*
	 * NS, SOA and DNSSKEY records are signed by their owner.
	 * DS records are signed by the parent.
	 */
	switch (set->type) {
	case dns_rdatatype_ns:
	case dns_rdatatype_soa:
	case dns_rdatatype_dnskey:
		if (!dns_name_equal(name, &sig.signer))
			return (DNS_R_SIGINVALID);
		break;
	case dns_rdatatype_ds:
		if (dns_name_equal(name, &sig.signer))
			return (DNS_R_SIGINVALID);
		/* FALLTHROUGH */
	default:
		if (!dns_name_issubdomain(name, &sig.signer))
			return (DNS_R_SIGINVALID);
		break;
	}

	/*
	 * Is the key allowed to sign data?
	 */
	flags = dst_key_flags(key);
	if (flags & DNS_KEYTYPE_NOAUTH)
		return (DNS_R_KEYUNAUTHORIZED);
	if ((flags & DNS_KEYFLAG_OWNERMASK) != DNS_KEYOWNER_ZONE)
		return (DNS_R_KEYUNAUTHORIZED);

	ret = dst_context_create(key, mctx, &ctx);
	if (ret != ISC_R_SUCCESS)
		goto cleanup_struct;

	/*
	 * Digest the SIG rdata (not including the signature).
	 */
	ret = digest_sig(ctx, sigrdata, &sig);
	if (ret != ISC_R_SUCCESS)
		goto cleanup_context;

	/*
	 * If the name is an expanded wildcard, use the wildcard name.
	 */
	dns_fixedname_init(&fnewname);
	labels = dns_name_countlabels(name) - 1;
	RUNTIME_CHECK(dns_name_downcase(name, dns_fixedname_name(&fnewname),
					NULL) == ISC_R_SUCCESS);
	if (labels - sig.labels > 0)
		dns_name_split(dns_fixedname_name(&fnewname), sig.labels + 1,
			       NULL, dns_fixedname_name(&fnewname));

	dns_name_toregion(dns_fixedname_name(&fnewname), &r);

	/*
	 * Create an envelope for each rdata: <name|type|class|ttl>.
	 */
	isc_buffer_init(&envbuf, data, sizeof(data));
	if (labels - sig.labels > 0) {
		isc_buffer_putuint8(&envbuf, 1);
		isc_buffer_putuint8(&envbuf, '*');
		memcpy(data + 2, r.base, r.length);
	}
	else
		memcpy(data, r.base, r.length);
	isc_buffer_add(&envbuf, r.length);
	isc_buffer_putuint16(&envbuf, set->type);
	isc_buffer_putuint16(&envbuf, set->rdclass);
	isc_buffer_putuint32(&envbuf, sig.originalttl);

	ret = rdataset_to_sortedarray(set, mctx, &rdatas, &nrdatas);
	if (ret != ISC_R_SUCCESS)
		goto cleanup_context;

	isc_buffer_usedregion(&envbuf, &r);

	for (i = 0; i < nrdatas; i++) {
		isc_uint16_t len;
		isc_buffer_t lenbuf;
		isc_region_t lenr;

		/*
		 * Skip duplicates.
		 */
		if (i > 0 && dns_rdata_compare(&rdatas[i], &rdatas[i-1]) == 0)
		    continue;

		/*
		 * Digest the envelope.
		 */
		ret = dst_context_adddata(ctx, &r);
		if (ret != ISC_R_SUCCESS)
			goto cleanup_array;

		/*
		 * Digest the rdata length.
		 */
		isc_buffer_init(&lenbuf, &len, sizeof(len));
		INSIST(rdatas[i].length < 65536);
		isc_buffer_putuint16(&lenbuf, (isc_uint16_t)rdatas[i].length);
		isc_buffer_usedregion(&lenbuf, &lenr);

		/*
		 * Digest the rdata.
		 */
		ret = dst_context_adddata(ctx, &lenr);
		if (ret != ISC_R_SUCCESS)
			goto cleanup_array;
		ret = dns_rdata_digest(&rdatas[i], digest_callback, ctx);
		if (ret != ISC_R_SUCCESS)
			goto cleanup_array;
	}

	r.base = sig.signature;
	r.length = sig.siglen;
	ret = dst_context_verify(ctx, &r);
	if (ret == DST_R_VERIFYFAILURE)
		ret = DNS_R_SIGINVALID;

cleanup_array:
	isc_mem_put(mctx, rdatas, nrdatas * sizeof(dns_rdata_t));
cleanup_context:
	dst_context_destroy(&ctx);
cleanup_struct:
	dns_rdata_freestruct(&sig);

	if (ret == ISC_R_SUCCESS && labels - sig.labels > 0) {
		if (wild != NULL)
			RUNTIME_CHECK(dns_name_concatenate(dns_wildcardname,
						 dns_fixedname_name(&fnewname),
						 wild, NULL) == ISC_R_SUCCESS);
		ret = DNS_R_FROMWILDCARD;
	}
	return (ret);
}

isc_result_t
dns_dnssec_verify(dns_name_t *name, dns_rdataset_t *set, dst_key_t *key,
		  isc_boolean_t ignoretime, isc_mem_t *mctx,
		  dns_rdata_t *sigrdata)
{
	isc_result_t result;

	result = dns_dnssec_verify2(name, set, key, ignoretime, mctx,
				    sigrdata, NULL);
	if (result == DNS_R_FROMWILDCARD)
		result = ISC_R_SUCCESS;
	return (result);
}

#define is_zone_key(key) ((dst_key_flags(key) & DNS_KEYFLAG_OWNERMASK) \
			  == DNS_KEYOWNER_ZONE)

isc_result_t
dns_dnssec_findzonekeys2(dns_db_t *db, dns_dbversion_t *ver,
			 dns_dbnode_t *node, dns_name_t *name,
			 const char *directory, isc_mem_t *mctx,
			 unsigned int maxkeys, dst_key_t **keys,
			 unsigned int *nkeys)
{
	dns_rdataset_t rdataset;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_result_t result;
	dst_key_t *pubkey = NULL;
	unsigned int count = 0;

	REQUIRE(nkeys != NULL);
	REQUIRE(keys != NULL);

	*nkeys = 0;
	dns_rdataset_init(&rdataset);
	RETERR(dns_db_findrdataset(db, node, ver, dns_rdatatype_dnskey, 0, 0,
				   &rdataset, NULL));
	RETERR(dns_rdataset_first(&rdataset));
	while (result == ISC_R_SUCCESS && count < maxkeys) {
		pubkey = NULL;
		dns_rdataset_current(&rdataset, &rdata);
		RETERR(dns_dnssec_keyfromrdata(name, &rdata, mctx, &pubkey));
		if (!is_zone_key(pubkey) ||
		    (dst_key_flags(pubkey) & DNS_KEYTYPE_NOAUTH) != 0)
			goto next;
		/* Corrupted .key file? */
		if (!dns_name_equal(name, dst_key_name(pubkey)))
			goto next;
		keys[count] = NULL;
		result = dst_key_fromfile(dst_key_name(pubkey),
					  dst_key_id(pubkey),
					  dst_key_alg(pubkey),
					  DST_TYPE_PUBLIC|DST_TYPE_PRIVATE,
					  directory,
					  mctx, &keys[count]);
		if (result == ISC_R_FILENOTFOUND) {
			keys[count] = pubkey;
			pubkey = NULL;
			count++;
			goto next;
		}
		if (result != ISC_R_SUCCESS)
			goto failure;
		if ((dst_key_flags(keys[count]) & DNS_KEYTYPE_NOAUTH) != 0) {
			/* We should never get here. */
			dst_key_free(&keys[count]);
			goto next;
		}
		count++;
 next:
		if (pubkey != NULL)
			dst_key_free(&pubkey);
		dns_rdata_reset(&rdata);
		result = dns_rdataset_next(&rdataset);
	}
	if (result != ISC_R_NOMORE)
		goto failure;
	if (count == 0)
		result = ISC_R_NOTFOUND;
	else
		result = ISC_R_SUCCESS;

 failure:
	if (dns_rdataset_isassociated(&rdataset))
		dns_rdataset_disassociate(&rdataset);
	if (pubkey != NULL)
		dst_key_free(&pubkey);
	if (result != ISC_R_SUCCESS)
		while (count > 0)
			dst_key_free(&keys[--count]);
	*nkeys = count;
	return (result);
}

isc_result_t
dns_dnssec_findzonekeys(dns_db_t *db, dns_dbversion_t *ver,
			dns_dbnode_t *node, dns_name_t *name, isc_mem_t *mctx,
			unsigned int maxkeys, dst_key_t **keys,
			unsigned int *nkeys)
{
	return (dns_dnssec_findzonekeys2(db, ver, node, name, NULL, mctx,
					 maxkeys, keys, nkeys));
}

isc_result_t
dns_dnssec_signmessage(dns_message_t *msg, dst_key_t *key) {
	dns_rdata_sig_t sig;	/* SIG(0) */
	unsigned char data[512];
	unsigned char header[DNS_MESSAGE_HEADERLEN];
	isc_buffer_t headerbuf, databuf, sigbuf;
	unsigned int sigsize;
	isc_buffer_t *dynbuf = NULL;
	dns_rdata_t *rdata;
	dns_rdatalist_t *datalist;
	dns_rdataset_t *dataset;
	isc_region_t r;
	isc_stdtime_t now;
	dst_context_t *ctx = NULL;
	isc_mem_t *mctx;
	isc_result_t result;
	isc_boolean_t signeedsfree = ISC_TRUE;

	REQUIRE(msg != NULL);
	REQUIRE(key != NULL);

	if (is_response(msg))
		REQUIRE(msg->query.base != NULL);

	mctx = msg->mctx;

	memset(&sig, 0, sizeof(sig));

	sig.mctx = mctx;
	sig.common.rdclass = dns_rdataclass_any;
	sig.common.rdtype = dns_rdatatype_sig;	/* SIG(0) */
	ISC_LINK_INIT(&sig.common, link);

	sig.covered = 0;
	sig.algorithm = dst_key_alg(key);
	sig.labels = 0; /* the root name */
	sig.originalttl = 0;

	isc_stdtime_get(&now);
	sig.timesigned = now - DNS_TSIG_FUDGE;
	sig.timeexpire = now + DNS_TSIG_FUDGE;

	sig.keyid = dst_key_id(key);

	dns_name_init(&sig.signer, NULL);
	dns_name_clone(dst_key_name(key), &sig.signer);

	sig.siglen = 0;
	sig.signature = NULL;

	isc_buffer_init(&databuf, data, sizeof(data));

	RETERR(dst_context_create(key, mctx, &ctx));

	/*
	 * Digest the fields of the SIG - we can cheat and use
	 * dns_rdata_fromstruct.  Since siglen is 0, the digested data
	 * is identical to dns format.
	 */
	RETERR(dns_rdata_fromstruct(NULL, dns_rdataclass_any,
				    dns_rdatatype_sig /* SIG(0) */,
				    &sig, &databuf));
	isc_buffer_usedregion(&databuf, &r);
	RETERR(dst_context_adddata(ctx, &r));

	/*
	 * If this is a response, digest the query.
	 */
	if (is_response(msg))
		RETERR(dst_context_adddata(ctx, &msg->query));

	/*
	 * Digest the header.
	 */
	isc_buffer_init(&headerbuf, header, sizeof(header));
	dns_message_renderheader(msg, &headerbuf);
	isc_buffer_usedregion(&headerbuf, &r);
	RETERR(dst_context_adddata(ctx, &r));

	/*
	 * Digest the remainder of the message.
	 */
	isc_buffer_usedregion(msg->buffer, &r);
	isc_region_consume(&r, DNS_MESSAGE_HEADERLEN);
	RETERR(dst_context_adddata(ctx, &r));

	RETERR(dst_key_sigsize(key, &sigsize));
	sig.siglen = sigsize;
	sig.signature = (unsigned char *) isc_mem_get(mctx, sig.siglen);
	if (sig.signature == NULL) {
		result = ISC_R_NOMEMORY;
		goto failure;
	}

	isc_buffer_init(&sigbuf, sig.signature, sig.siglen);
	RETERR(dst_context_sign(ctx, &sigbuf));
	dst_context_destroy(&ctx);

	rdata = NULL;
	RETERR(dns_message_gettemprdata(msg, &rdata));
	RETERR(isc_buffer_allocate(msg->mctx, &dynbuf, 1024));
	RETERR(dns_rdata_fromstruct(rdata, dns_rdataclass_any,
				    dns_rdatatype_sig /* SIG(0) */,
				    &sig, dynbuf));

	isc_mem_put(mctx, sig.signature, sig.siglen);
	signeedsfree = ISC_FALSE;

	dns_message_takebuffer(msg, &dynbuf);

	datalist = NULL;
	RETERR(dns_message_gettemprdatalist(msg, &datalist));
	datalist->rdclass = dns_rdataclass_any;
	datalist->type = dns_rdatatype_sig;	/* SIG(0) */
	datalist->covers = 0;
	datalist->ttl = 0;
	ISC_LIST_INIT(datalist->rdata);
	ISC_LIST_APPEND(datalist->rdata, rdata, link);
	dataset = NULL;
	RETERR(dns_message_gettemprdataset(msg, &dataset));
	dns_rdataset_init(dataset);
	RUNTIME_CHECK(dns_rdatalist_tordataset(datalist, dataset) == ISC_R_SUCCESS);
	msg->sig0 = dataset;

	return (ISC_R_SUCCESS);

failure:
	if (dynbuf != NULL)
		isc_buffer_free(&dynbuf);
	if (signeedsfree)
		isc_mem_put(mctx, sig.signature, sig.siglen);
	if (ctx != NULL)
		dst_context_destroy(&ctx);

	return (result);
}

isc_result_t
dns_dnssec_verifymessage(isc_buffer_t *source, dns_message_t *msg,
			 dst_key_t *key)
{
	dns_rdata_sig_t sig;	/* SIG(0) */
	unsigned char header[DNS_MESSAGE_HEADERLEN];
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_region_t r, source_r, sig_r, header_r;
	isc_stdtime_t now;
	dst_context_t *ctx = NULL;
	isc_mem_t *mctx;
	isc_result_t result;
	isc_uint16_t addcount;
	isc_boolean_t signeedsfree = ISC_FALSE;

	REQUIRE(source != NULL);
	REQUIRE(msg != NULL);
	REQUIRE(key != NULL);

	mctx = msg->mctx;

	msg->verify_attempted = 1;

	if (is_response(msg)) {
		if (msg->query.base == NULL)
			return (DNS_R_UNEXPECTEDTSIG);
	}

	isc_buffer_usedregion(source, &source_r);

	RETERR(dns_rdataset_first(msg->sig0));
	dns_rdataset_current(msg->sig0, &rdata);

	RETERR(dns_rdata_tostruct(&rdata, &sig, NULL));
	signeedsfree = ISC_TRUE;

	if (sig.labels != 0) {
		result = DNS_R_SIGINVALID;
		goto failure;
	}

	if (isc_serial_lt(sig.timeexpire, sig.timesigned)) {
		result = DNS_R_SIGINVALID;
		msg->sig0status = dns_tsigerror_badtime;
		goto failure;
	}

	isc_stdtime_get(&now);
	if (isc_serial_lt((isc_uint32_t)now, sig.timesigned)) {
		result = DNS_R_SIGFUTURE;
		msg->sig0status = dns_tsigerror_badtime;
		goto failure;
	}
	else if (isc_serial_lt(sig.timeexpire, (isc_uint32_t)now)) {
		result = DNS_R_SIGEXPIRED;
		msg->sig0status = dns_tsigerror_badtime;
		goto failure;
	}

	if (!dns_name_equal(dst_key_name(key), &sig.signer)) {
		result = DNS_R_SIGINVALID;
		msg->sig0status = dns_tsigerror_badkey;
		goto failure;
	}

	RETERR(dst_context_create(key, mctx, &ctx));

	/*
	 * Digest the SIG(0) record, except for the signature.
	 */
	dns_rdata_toregion(&rdata, &r);
	r.length -= sig.siglen;
	RETERR(dst_context_adddata(ctx, &r));

	/*
	 * If this is a response, digest the query.
	 */
	if (is_response(msg))
		RETERR(dst_context_adddata(ctx, &msg->query));

	/*
	 * Extract the header.
	 */
	memcpy(header, source_r.base, DNS_MESSAGE_HEADERLEN);

	/*
	 * Decrement the additional field counter.
	 */
	memcpy(&addcount, &header[DNS_MESSAGE_HEADERLEN - 2], 2);
	addcount = htons((isc_uint16_t)(ntohs(addcount) - 1));
	memcpy(&header[DNS_MESSAGE_HEADERLEN - 2], &addcount, 2);

	/*
	 * Digest the modified header.
	 */
	header_r.base = (unsigned char *) header;
	header_r.length = DNS_MESSAGE_HEADERLEN;
	RETERR(dst_context_adddata(ctx, &header_r));

	/*
	 * Digest all non-SIG(0) records.
	 */
	r.base = source_r.base + DNS_MESSAGE_HEADERLEN;
	r.length = msg->sigstart - DNS_MESSAGE_HEADERLEN;
	RETERR(dst_context_adddata(ctx, &r));

	sig_r.base = sig.signature;
	sig_r.length = sig.siglen;
	result = dst_context_verify(ctx, &sig_r);
	if (result != ISC_R_SUCCESS) {
		msg->sig0status = dns_tsigerror_badsig;
		goto failure;
	}

	msg->verified_sig = 1;

	dst_context_destroy(&ctx);
	dns_rdata_freestruct(&sig);

	return (ISC_R_SUCCESS);

failure:
	if (signeedsfree)
		dns_rdata_freestruct(&sig);
	if (ctx != NULL)
		dst_context_destroy(&ctx);

	return (result);
}
