/*
 * Copyright (C) 2004-2008  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2001, 2003  Internet Software Consortium.
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
 * $Id: tkey.c,v 1.90 2008/04/03 00:45:23 marka Exp $
 */
/*! \file */
#include <config.h>

#include <isc/buffer.h>
#include <isc/entropy.h>
#include <isc/md5.h>
#include <isc/mem.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/dnssec.h>
#include <dns/fixedname.h>
#include <dns/keyvalues.h>
#include <dns/log.h>
#include <dns/message.h>
#include <dns/name.h>
#include <dns/rdata.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/result.h>
#include <dns/tkey.h>
#include <dns/tsig.h>

#include <dst/dst.h>
#include <dst/gssapi.h>

#define TKEY_RANDOM_AMOUNT 16

#define RETERR(x) do { \
	result = (x); \
	if (result != ISC_R_SUCCESS) \
		goto failure; \
	} while (0)

static void
tkey_log(const char *fmt, ...) ISC_FORMAT_PRINTF(1, 2);

static void
tkey_log(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	isc_log_vwrite(dns_lctx, DNS_LOGCATEGORY_GENERAL,
		       DNS_LOGMODULE_REQUEST, ISC_LOG_DEBUG(4), fmt, ap);
	va_end(ap);
}

static void
_dns_tkey_dumpmessage(dns_message_t *msg) {
	isc_buffer_t outbuf;
	unsigned char output[4096];
	isc_result_t result;

	isc_buffer_init(&outbuf, output, sizeof(output));
	result = dns_message_totext(msg, &dns_master_style_debug, 0,
				    &outbuf);
	/* XXXMLG ignore result */
	fprintf(stderr, "%.*s\n", (int)isc_buffer_usedlength(&outbuf),
		(char *)isc_buffer_base(&outbuf));
}

isc_result_t
dns_tkeyctx_create(isc_mem_t *mctx, isc_entropy_t *ectx, dns_tkeyctx_t **tctxp)
{
	dns_tkeyctx_t *tctx;

	REQUIRE(mctx != NULL);
	REQUIRE(ectx != NULL);
	REQUIRE(tctxp != NULL && *tctxp == NULL);

	tctx = isc_mem_get(mctx, sizeof(dns_tkeyctx_t));
	if (tctx == NULL)
		return (ISC_R_NOMEMORY);
	tctx->mctx = NULL;
	isc_mem_attach(mctx, &tctx->mctx);
	tctx->ectx = NULL;
	isc_entropy_attach(ectx, &tctx->ectx);
	tctx->dhkey = NULL;
	tctx->domain = NULL;
	tctx->gsscred = NULL;

	*tctxp = tctx;
	return (ISC_R_SUCCESS);
}

void
dns_tkeyctx_destroy(dns_tkeyctx_t **tctxp) {
	isc_mem_t *mctx;
	dns_tkeyctx_t *tctx;

	REQUIRE(tctxp != NULL && *tctxp != NULL);

	tctx = *tctxp;
	mctx = tctx->mctx;

	if (tctx->dhkey != NULL)
		dst_key_free(&tctx->dhkey);
	if (tctx->domain != NULL) {
		if (dns_name_dynamic(tctx->domain))
			dns_name_free(tctx->domain, mctx);
		isc_mem_put(mctx, tctx->domain, sizeof(dns_name_t));
	}
	if (tctx->gsscred != NULL)
		dst_gssapi_releasecred(&tctx->gsscred);
	isc_entropy_detach(&tctx->ectx);
	isc_mem_put(mctx, tctx, sizeof(dns_tkeyctx_t));
	isc_mem_detach(&mctx);
	*tctxp = NULL;
}

static isc_result_t
add_rdata_to_list(dns_message_t *msg, dns_name_t *name, dns_rdata_t *rdata,
		isc_uint32_t ttl, dns_namelist_t *namelist)
{
	isc_result_t result;
	isc_region_t r, newr;
	dns_rdata_t *newrdata = NULL;
	dns_name_t *newname = NULL;
	dns_rdatalist_t *newlist = NULL;
	dns_rdataset_t *newset = NULL;
	isc_buffer_t *tmprdatabuf = NULL;

	RETERR(dns_message_gettemprdata(msg, &newrdata));

	dns_rdata_toregion(rdata, &r);
	RETERR(isc_buffer_allocate(msg->mctx, &tmprdatabuf, r.length));
	isc_buffer_availableregion(tmprdatabuf, &newr);
	memcpy(newr.base, r.base, r.length);
	dns_rdata_fromregion(newrdata, rdata->rdclass, rdata->type, &newr);
	dns_message_takebuffer(msg, &tmprdatabuf);

	RETERR(dns_message_gettempname(msg, &newname));
	dns_name_init(newname, NULL);
	RETERR(dns_name_dup(name, msg->mctx, newname));

	RETERR(dns_message_gettemprdatalist(msg, &newlist));
	newlist->rdclass = newrdata->rdclass;
	newlist->type = newrdata->type;
	newlist->covers = 0;
	newlist->ttl = ttl;
	ISC_LIST_INIT(newlist->rdata);
	ISC_LIST_APPEND(newlist->rdata, newrdata, link);

	RETERR(dns_message_gettemprdataset(msg, &newset));
	dns_rdataset_init(newset);
	RETERR(dns_rdatalist_tordataset(newlist, newset));

	ISC_LIST_INIT(newname->list);
	ISC_LIST_APPEND(newname->list, newset, link);

	ISC_LIST_APPEND(*namelist, newname, link);

	return (ISC_R_SUCCESS);

 failure:
	if (newrdata != NULL) {
		if (ISC_LINK_LINKED(newrdata, link))
			ISC_LIST_UNLINK(newlist->rdata, newrdata, link);
		dns_message_puttemprdata(msg, &newrdata);
	}
	if (newname != NULL)
		dns_message_puttempname(msg, &newname);
	if (newset != NULL) {
		dns_rdataset_disassociate(newset);
		dns_message_puttemprdataset(msg, &newset);
	}
	if (newlist != NULL)
		dns_message_puttemprdatalist(msg, &newlist);
	return (result);
}

static void
free_namelist(dns_message_t *msg, dns_namelist_t *namelist) {
	dns_name_t *name;
	dns_rdataset_t *set;

	while (!ISC_LIST_EMPTY(*namelist)) {
		name = ISC_LIST_HEAD(*namelist);
		ISC_LIST_UNLINK(*namelist, name, link);
		while (!ISC_LIST_EMPTY(name->list)) {
			set = ISC_LIST_HEAD(name->list);
			ISC_LIST_UNLINK(name->list, set, link);
			dns_message_puttemprdataset(msg, &set);
		}
		dns_message_puttempname(msg, &name);
	}
}

static isc_result_t
compute_secret(isc_buffer_t *shared, isc_region_t *queryrandomness,
	       isc_region_t *serverrandomness, isc_buffer_t *secret)
{
	isc_md5_t md5ctx;
	isc_region_t r, r2;
	unsigned char digests[32];
	unsigned int i;

	isc_buffer_usedregion(shared, &r);

	/*
	 * MD5 ( query data | DH value ).
	 */
	isc_md5_init(&md5ctx);
	isc_md5_update(&md5ctx, queryrandomness->base,
		       queryrandomness->length);
	isc_md5_update(&md5ctx, r.base, r.length);
	isc_md5_final(&md5ctx, digests);

	/*
	 * MD5 ( server data | DH value ).
	 */
	isc_md5_init(&md5ctx);
	isc_md5_update(&md5ctx, serverrandomness->base,
		       serverrandomness->length);
	isc_md5_update(&md5ctx, r.base, r.length);
	isc_md5_final(&md5ctx, &digests[ISC_MD5_DIGESTLENGTH]);

	/*
	 * XOR ( DH value, MD5-1 | MD5-2).
	 */
	isc_buffer_availableregion(secret, &r);
	isc_buffer_usedregion(shared, &r2);
	if (r.length < sizeof(digests) || r.length < r2.length)
		return (ISC_R_NOSPACE);
	if (r2.length > sizeof(digests)) {
		memcpy(r.base, r2.base, r2.length);
		for (i = 0; i < sizeof(digests); i++)
			r.base[i] ^= digests[i];
		isc_buffer_add(secret, r2.length);
	} else {
		memcpy(r.base, digests, sizeof(digests));
		for (i = 0; i < r2.length; i++)
			r.base[i] ^= r2.base[i];
		isc_buffer_add(secret, sizeof(digests));
	}
	return (ISC_R_SUCCESS);

}

static isc_result_t
process_dhtkey(dns_message_t *msg, dns_name_t *signer, dns_name_t *name,
	       dns_rdata_tkey_t *tkeyin, dns_tkeyctx_t *tctx,
	       dns_rdata_tkey_t *tkeyout,
	       dns_tsig_keyring_t *ring, dns_namelist_t *namelist)
{
	isc_result_t result = ISC_R_SUCCESS;
	dns_name_t *keyname, ourname;
	dns_rdataset_t *keyset = NULL;
	dns_rdata_t keyrdata = DNS_RDATA_INIT, ourkeyrdata = DNS_RDATA_INIT;
	isc_boolean_t found_key = ISC_FALSE, found_incompatible = ISC_FALSE;
	dst_key_t *pubkey = NULL;
	isc_buffer_t ourkeybuf, *shared = NULL;
	isc_region_t r, r2, ourkeyr;
	unsigned char keydata[DST_KEY_MAXSIZE];
	unsigned int sharedsize;
	isc_buffer_t secret;
	unsigned char *randomdata = NULL, secretdata[256];
	dns_ttl_t ttl = 0;

	if (tctx->dhkey == NULL) {
		tkey_log("process_dhtkey: tkey-dhkey not defined");
		tkeyout->error = dns_tsigerror_badalg;
		return (DNS_R_REFUSED);
	}

	if (!dns_name_equal(&tkeyin->algorithm, DNS_TSIG_HMACMD5_NAME)) {
		tkey_log("process_dhtkey: algorithms other than "
			 "hmac-md5 are not supported");
		tkeyout->error = dns_tsigerror_badalg;
		return (ISC_R_SUCCESS);
	}

	/*
	 * Look for a DH KEY record that will work with ours.
	 */
	for (result = dns_message_firstname(msg, DNS_SECTION_ADDITIONAL);
	     result == ISC_R_SUCCESS && !found_key;
	     result = dns_message_nextname(msg, DNS_SECTION_ADDITIONAL)) {
		keyname = NULL;
		dns_message_currentname(msg, DNS_SECTION_ADDITIONAL, &keyname);
		keyset = NULL;
		result = dns_message_findtype(keyname, dns_rdatatype_key, 0,
					      &keyset);
		if (result != ISC_R_SUCCESS)
			continue;

		for (result = dns_rdataset_first(keyset);
		     result == ISC_R_SUCCESS && !found_key;
		     result = dns_rdataset_next(keyset)) {
			dns_rdataset_current(keyset, &keyrdata);
			pubkey = NULL;
			result = dns_dnssec_keyfromrdata(keyname, &keyrdata,
							 msg->mctx, &pubkey);
			if (result != ISC_R_SUCCESS) {
				dns_rdata_reset(&keyrdata);
				continue;
			}
			if (dst_key_alg(pubkey) == DNS_KEYALG_DH) {
				if (dst_key_paramcompare(pubkey, tctx->dhkey))
				{
					found_key = ISC_TRUE;
					ttl = keyset->ttl;
					break;
				} else
					found_incompatible = ISC_TRUE;
			}
			dst_key_free(&pubkey);
			dns_rdata_reset(&keyrdata);
		}
	}

	if (!found_key) {
		if (found_incompatible) {
			tkey_log("process_dhtkey: found an incompatible key");
			tkeyout->error = dns_tsigerror_badkey;
			return (ISC_R_SUCCESS);
		} else {
			tkey_log("process_dhtkey: failed to find a key");
			return (DNS_R_FORMERR);
		}
	}

	RETERR(add_rdata_to_list(msg, keyname, &keyrdata, ttl, namelist));

	isc_buffer_init(&ourkeybuf, keydata, sizeof(keydata));
	RETERR(dst_key_todns(tctx->dhkey, &ourkeybuf));
	isc_buffer_usedregion(&ourkeybuf, &ourkeyr);
	dns_rdata_fromregion(&ourkeyrdata, dns_rdataclass_any,
			     dns_rdatatype_key, &ourkeyr);

	dns_name_init(&ourname, NULL);
	dns_name_clone(dst_key_name(tctx->dhkey), &ourname);

	/*
	 * XXXBEW The TTL should be obtained from the database, if it exists.
	 */
	RETERR(add_rdata_to_list(msg, &ourname, &ourkeyrdata, 0, namelist));

	RETERR(dst_key_secretsize(tctx->dhkey, &sharedsize));
	RETERR(isc_buffer_allocate(msg->mctx, &shared, sharedsize));

	result = dst_key_computesecret(pubkey, tctx->dhkey, shared);
	if (result != ISC_R_SUCCESS) {
		tkey_log("process_dhtkey: failed to compute shared secret: %s",
			 isc_result_totext(result));
		goto failure;
	}
	dst_key_free(&pubkey);

	isc_buffer_init(&secret, secretdata, sizeof(secretdata));

	randomdata = isc_mem_get(tkeyout->mctx, TKEY_RANDOM_AMOUNT);
	if (randomdata == NULL)
		goto failure;

	result = isc_entropy_getdata(tctx->ectx, randomdata,
				     TKEY_RANDOM_AMOUNT, NULL, 0);
	if (result != ISC_R_SUCCESS) {
		tkey_log("process_dhtkey: failed to obtain entropy: %s",
			 isc_result_totext(result));
		goto failure;
	}

	r.base = randomdata;
	r.length = TKEY_RANDOM_AMOUNT;
	r2.base = tkeyin->key;
	r2.length = tkeyin->keylen;
	RETERR(compute_secret(shared, &r2, &r, &secret));
	isc_buffer_free(&shared);

	RETERR(dns_tsigkey_create(name, &tkeyin->algorithm,
				  isc_buffer_base(&secret),
				  isc_buffer_usedlength(&secret),
				  ISC_TRUE, signer, tkeyin->inception,
				  tkeyin->expire, ring->mctx, ring, NULL));

	/* This key is good for a long time */
	tkeyout->inception = tkeyin->inception;
	tkeyout->expire = tkeyin->expire;

	tkeyout->key = randomdata;
	tkeyout->keylen = TKEY_RANDOM_AMOUNT;

	return (ISC_R_SUCCESS);

 failure:
	if (!ISC_LIST_EMPTY(*namelist))
		free_namelist(msg, namelist);
	if (shared != NULL)
		isc_buffer_free(&shared);
	if (pubkey != NULL)
		dst_key_free(&pubkey);
	if (randomdata != NULL)
		isc_mem_put(tkeyout->mctx, randomdata, TKEY_RANDOM_AMOUNT);
	return (result);
}

static isc_result_t
process_gsstkey(dns_message_t *msg, dns_name_t *signer, dns_name_t *name,
		dns_rdata_tkey_t *tkeyin, dns_tkeyctx_t *tctx,
		dns_rdata_tkey_t *tkeyout,
		dns_tsig_keyring_t *ring, dns_namelist_t *namelist)
{
	isc_result_t result = ISC_R_SUCCESS;
	dst_key_t *dstkey = NULL;
	dns_tsigkey_t *tsigkey = NULL;
	dns_fixedname_t principal;
	isc_stdtime_t now;
	isc_region_t intoken;
	isc_buffer_t *outtoken = NULL;
	gss_ctx_id_t gss_ctx = NULL;

	UNUSED(namelist);
	UNUSED(signer);

	if (tctx->gsscred == NULL)
		return (ISC_R_NOPERM);

	if (!dns_name_equal(&tkeyin->algorithm, DNS_TSIG_GSSAPI_NAME) &&
	    !dns_name_equal(&tkeyin->algorithm, DNS_TSIG_GSSAPIMS_NAME)) {
		tkeyout->error = dns_tsigerror_badalg;
		tkey_log("process_gsstkey(): dns_tsigerror_badalg");	/* XXXSRA */
		return (ISC_R_SUCCESS);
	}

	/*
	 * XXXDCL need to check for key expiry per 4.1.1
	 * XXXDCL need a way to check fully established, perhaps w/key_flags
	 */

	intoken.base = tkeyin->key;
	intoken.length = tkeyin->keylen;

	result = dns_tsigkey_find(&tsigkey, name, &tkeyin->algorithm, ring);
	if (result == ISC_R_SUCCESS)
		gss_ctx = dst_key_getgssctx(tsigkey->key);


	dns_fixedname_init(&principal);

	result = dst_gssapi_acceptctx(tctx->gsscred, &intoken,
				      &outtoken, &gss_ctx,
				      dns_fixedname_name(&principal),
				      tctx->mctx);

	if (tsigkey != NULL)
		dns_tsigkey_detach(&tsigkey);

	if (result == DNS_R_INVALIDTKEY) {
		tkeyout->error = dns_tsigerror_badkey;
		tkey_log("process_gsstkey(): dns_tsigerror_badkey");    /* XXXSRA */
		return (ISC_R_SUCCESS);
	} else if (result == ISC_R_FAILURE)
		goto failure;
	ENSURE(result == DNS_R_CONTINUE || result == ISC_R_SUCCESS);
	/*
	 * XXXDCL Section 4.1.3: Limit GSS_S_CONTINUE_NEEDED to 10 times.
	 */

	if (tsigkey == NULL) {
		RETERR(dst_key_fromgssapi(name, gss_ctx, msg->mctx, &dstkey));
		RETERR(dns_tsigkey_createfromkey(name, &tkeyin->algorithm,
						 dstkey, ISC_TRUE,
						 dns_fixedname_name(&principal),
						 tkeyin->inception,
						 tkeyin->expire,
						 ring->mctx, ring, NULL));
	}

	isc_stdtime_get(&now);
	tkeyout->inception = tkeyin->inception;
	tkeyout->expire = tkeyin->expire;

	if (outtoken) {
		tkeyout->key = isc_mem_get(tkeyout->mctx,
					   isc_buffer_usedlength(outtoken));
		if (tkeyout->key == NULL) {
			result = ISC_R_NOMEMORY;
			goto failure;
		}
		tkeyout->keylen = isc_buffer_usedlength(outtoken);
		memcpy(tkeyout->key, isc_buffer_base(outtoken),
		       isc_buffer_usedlength(outtoken));
		isc_buffer_free(&outtoken);
	} else {
		tkeyout->key = isc_mem_get(tkeyout->mctx, tkeyin->keylen);
		if (tkeyout->key == NULL) {
			result = ISC_R_NOMEMORY;
			goto failure;
		}
		tkeyout->keylen = tkeyin->keylen;
		memcpy(tkeyout->key, tkeyin->key, tkeyin->keylen);
	}

	tkeyout->error = dns_rcode_noerror;

	tkey_log("process_gsstkey(): dns_tsigerror_noerror");   /* XXXSRA */

	return (ISC_R_SUCCESS);

failure:
	if (dstkey != NULL)
		dst_key_free(&dstkey);

	if (outtoken != NULL)
		isc_buffer_free(&outtoken);

	tkey_log("process_gsstkey(): %s",
		isc_result_totext(result));	/* XXXSRA */

	return (result);
}

static isc_result_t
process_deletetkey(dns_message_t *msg, dns_name_t *signer, dns_name_t *name,
		   dns_rdata_tkey_t *tkeyin,
		   dns_rdata_tkey_t *tkeyout,
		   dns_tsig_keyring_t *ring,
		   dns_namelist_t *namelist)
{
	isc_result_t result;
	dns_tsigkey_t *tsigkey = NULL;
	dns_name_t *identity;

	UNUSED(msg);
	UNUSED(namelist);

	result = dns_tsigkey_find(&tsigkey, name, &tkeyin->algorithm, ring);
	if (result != ISC_R_SUCCESS) {
		tkeyout->error = dns_tsigerror_badname;
		return (ISC_R_SUCCESS);
	}

	/*
	 * Only allow a delete if the identity that created the key is the
	 * same as the identity that signed the message.
	 */
	identity = dns_tsigkey_identity(tsigkey);
	if (identity == NULL || !dns_name_equal(identity, signer)) {
		dns_tsigkey_detach(&tsigkey);
		return (DNS_R_REFUSED);
	}

	/*
	 * Set the key to be deleted when no references are left.  If the key
	 * was not generated with TKEY and is in the config file, it may be
	 * reloaded later.
	 */
	dns_tsigkey_setdeleted(tsigkey);

	/* Release the reference */
	dns_tsigkey_detach(&tsigkey);

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_tkey_processquery(dns_message_t *msg, dns_tkeyctx_t *tctx,
		      dns_tsig_keyring_t *ring)
{
	isc_result_t result = ISC_R_SUCCESS;
	dns_rdata_tkey_t tkeyin, tkeyout;
	isc_boolean_t freetkeyin = ISC_FALSE;
	dns_name_t *qname, *name, *keyname, *signer, tsigner;
	dns_fixedname_t fkeyname;
	dns_rdataset_t *tkeyset;
	dns_rdata_t rdata;
	dns_namelist_t namelist;
	char tkeyoutdata[512];
	isc_buffer_t tkeyoutbuf;

	REQUIRE(msg != NULL);
	REQUIRE(tctx != NULL);
	REQUIRE(ring != NULL);

	ISC_LIST_INIT(namelist);

	/*
	 * Interpret the question section.
	 */
	result = dns_message_firstname(msg, DNS_SECTION_QUESTION);
	if (result != ISC_R_SUCCESS)
		return (DNS_R_FORMERR);

	qname = NULL;
	dns_message_currentname(msg, DNS_SECTION_QUESTION, &qname);

	/*
	 * Look for a TKEY record that matches the question.
	 */
	tkeyset = NULL;
	name = NULL;
	result = dns_message_findname(msg, DNS_SECTION_ADDITIONAL, qname,
				      dns_rdatatype_tkey, 0, &name, &tkeyset);
	if (result != ISC_R_SUCCESS) {
		/*
		 * Try the answer section, since that's where Win2000
		 * puts it.
		 */
		if (dns_message_findname(msg, DNS_SECTION_ANSWER, qname,
					 dns_rdatatype_tkey, 0, &name,
					 &tkeyset) != ISC_R_SUCCESS) {
			result = DNS_R_FORMERR;
			tkey_log("dns_tkey_processquery: couldn't find a TKEY "
				 "matching the question");
			goto failure;
		}
	}
	result = dns_rdataset_first(tkeyset);
	if (result != ISC_R_SUCCESS) {
		result = DNS_R_FORMERR;
		goto failure;
	}
	dns_rdata_init(&rdata);
	dns_rdataset_current(tkeyset, &rdata);

	RETERR(dns_rdata_tostruct(&rdata, &tkeyin, NULL));
	freetkeyin = ISC_TRUE;

	if (tkeyin.error != dns_rcode_noerror) {
		result = DNS_R_FORMERR;
		goto failure;
	}

	/*
	 * Before we go any farther, verify that the message was signed.
	 * GSSAPI TKEY doesn't require a signature, the rest do.
	 */
	dns_name_init(&tsigner, NULL);
	result = dns_message_signer(msg, &tsigner);
	if (result != ISC_R_SUCCESS) {
		if (tkeyin.mode == DNS_TKEYMODE_GSSAPI &&
		    result == ISC_R_NOTFOUND)
		       signer = NULL;
		else {
			tkey_log("dns_tkey_processquery: query was not "
				 "properly signed - rejecting");
			result = DNS_R_FORMERR;
			goto failure;
		}
	} else
		signer = &tsigner;

	tkeyout.common.rdclass = tkeyin.common.rdclass;
	tkeyout.common.rdtype = tkeyin.common.rdtype;
	ISC_LINK_INIT(&tkeyout.common, link);
	tkeyout.mctx = msg->mctx;

	dns_name_init(&tkeyout.algorithm, NULL);
	dns_name_clone(&tkeyin.algorithm, &tkeyout.algorithm);

	tkeyout.inception = tkeyout.expire = 0;
	tkeyout.mode = tkeyin.mode;
	tkeyout.error = 0;
	tkeyout.keylen = tkeyout.otherlen = 0;
	tkeyout.key = tkeyout.other = NULL;

	/*
	 * A delete operation must have a fully specified key name.  If this
	 * is not a delete, we do the following:
	 * if (qname != ".")
	 *	keyname = qname + defaultdomain
	 * else
	 *	keyname = <random hex> + defaultdomain
	 */
	if (tkeyin.mode != DNS_TKEYMODE_DELETE) {
		dns_tsigkey_t *tsigkey = NULL;

		if (tctx->domain == NULL && tkeyin.mode != DNS_TKEYMODE_GSSAPI) {
			tkey_log("dns_tkey_processquery: tkey-domain not set");
			result = DNS_R_REFUSED;
			goto failure;
		}

		dns_fixedname_init(&fkeyname);
		keyname = dns_fixedname_name(&fkeyname);

		if (!dns_name_equal(qname, dns_rootname)) {
			unsigned int n = dns_name_countlabels(qname);
			RUNTIME_CHECK(dns_name_copy(qname, keyname, NULL)
				      == ISC_R_SUCCESS);
			dns_name_getlabelsequence(keyname, 0, n - 1, keyname);
		} else {
			static char hexdigits[16] = {
				'0', '1', '2', '3', '4', '5', '6', '7',
				'8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
			unsigned char randomdata[16];
			char randomtext[32];
			isc_buffer_t b;
			unsigned int i, j;

			result = isc_entropy_getdata(tctx->ectx,
						     randomdata,
						     sizeof(randomdata),
						     NULL, 0);
			if (result != ISC_R_SUCCESS)
				goto failure;

			for (i = 0, j = 0; i < sizeof(randomdata); i++) {
				unsigned char val = randomdata[i];
				randomtext[j++] = hexdigits[val >> 4];
				randomtext[j++] = hexdigits[val & 0xF];
			}
			isc_buffer_init(&b, randomtext, sizeof(randomtext));
			isc_buffer_add(&b, sizeof(randomtext));
			result = dns_name_fromtext(keyname, &b, NULL,
						   ISC_FALSE, NULL);
			if (result != ISC_R_SUCCESS)
				goto failure;
		}

		if (tkeyin.mode == DNS_TKEYMODE_GSSAPI) {
			/* Yup.  This is a hack */
			result = dns_name_concatenate(keyname, dns_rootname,
						      keyname, NULL);
			if (result != ISC_R_SUCCESS)
				goto failure;
		} else {
			result = dns_name_concatenate(keyname, tctx->domain,
						      keyname, NULL);
			if (result != ISC_R_SUCCESS)
				goto failure;
		}

		result = dns_tsigkey_find(&tsigkey, keyname, NULL, ring);

		if (result == ISC_R_SUCCESS) {
			tkeyout.error = dns_tsigerror_badname;
			dns_tsigkey_detach(&tsigkey);
			goto failure_with_tkey;
		} else if (result != ISC_R_NOTFOUND)
			goto failure;
	} else
		keyname = qname;

	switch (tkeyin.mode) {
		case DNS_TKEYMODE_DIFFIEHELLMAN:
			tkeyout.error = dns_rcode_noerror;
			RETERR(process_dhtkey(msg, signer, keyname, &tkeyin,
					      tctx, &tkeyout, ring,
					      &namelist));
			break;
		case DNS_TKEYMODE_GSSAPI:
			tkeyout.error = dns_rcode_noerror;
			RETERR(process_gsstkey(msg, signer, keyname, &tkeyin,
					       tctx, &tkeyout, ring,
					       &namelist));

			break;
		case DNS_TKEYMODE_DELETE:
			tkeyout.error = dns_rcode_noerror;
			RETERR(process_deletetkey(msg, signer, keyname,
						  &tkeyin, &tkeyout,
						  ring, &namelist));
			break;
		case DNS_TKEYMODE_SERVERASSIGNED:
		case DNS_TKEYMODE_RESOLVERASSIGNED:
			result = DNS_R_NOTIMP;
			goto failure;
		default:
			tkeyout.error = dns_tsigerror_badmode;
	}

 failure_with_tkey:
	dns_rdata_init(&rdata);
	isc_buffer_init(&tkeyoutbuf, tkeyoutdata, sizeof(tkeyoutdata));
	result = dns_rdata_fromstruct(&rdata, tkeyout.common.rdclass,
				      tkeyout.common.rdtype, &tkeyout,
				      &tkeyoutbuf);

	if (freetkeyin) {
		dns_rdata_freestruct(&tkeyin);
		freetkeyin = ISC_FALSE;
	}

	if (tkeyout.key != NULL)
		isc_mem_put(tkeyout.mctx, tkeyout.key, tkeyout.keylen);
	if (tkeyout.other != NULL)
		isc_mem_put(tkeyout.mctx, tkeyout.other, tkeyout.otherlen);
	if (result != ISC_R_SUCCESS)
		goto failure;

	RETERR(add_rdata_to_list(msg, keyname, &rdata, 0, &namelist));

	RETERR(dns_message_reply(msg, ISC_TRUE));

	name = ISC_LIST_HEAD(namelist);
	while (name != NULL) {
		dns_name_t *next = ISC_LIST_NEXT(name, link);
		ISC_LIST_UNLINK(namelist, name, link);
		dns_message_addname(msg, name, DNS_SECTION_ANSWER);
		name = next;
	}

	return (ISC_R_SUCCESS);

 failure:
	if (freetkeyin)
		dns_rdata_freestruct(&tkeyin);
	if (!ISC_LIST_EMPTY(namelist))
		free_namelist(msg, &namelist);
	return (result);
}

static isc_result_t
buildquery(dns_message_t *msg, dns_name_t *name,
	   dns_rdata_tkey_t *tkey, isc_boolean_t win2k)
{
	dns_name_t *qname = NULL, *aname = NULL;
	dns_rdataset_t *question = NULL, *tkeyset = NULL;
	dns_rdatalist_t *tkeylist = NULL;
	dns_rdata_t *rdata = NULL;
	isc_buffer_t *dynbuf = NULL;
	isc_result_t result;

	REQUIRE(msg != NULL);
	REQUIRE(name != NULL);
	REQUIRE(tkey != NULL);

	RETERR(dns_message_gettempname(msg, &qname));
	RETERR(dns_message_gettempname(msg, &aname));

	RETERR(dns_message_gettemprdataset(msg, &question));
	dns_rdataset_init(question);
	dns_rdataset_makequestion(question, dns_rdataclass_any,
				  dns_rdatatype_tkey);

	RETERR(isc_buffer_allocate(msg->mctx, &dynbuf, 4096));
	RETERR(dns_message_gettemprdata(msg, &rdata));

	RETERR(dns_rdata_fromstruct(rdata, dns_rdataclass_any,
				    dns_rdatatype_tkey, tkey, dynbuf));
	dns_message_takebuffer(msg, &dynbuf);

	RETERR(dns_message_gettemprdatalist(msg, &tkeylist));
	tkeylist->rdclass = dns_rdataclass_any;
	tkeylist->type = dns_rdatatype_tkey;
	tkeylist->covers = 0;
	tkeylist->ttl = 0;
	ISC_LIST_INIT(tkeylist->rdata);
	ISC_LIST_APPEND(tkeylist->rdata, rdata, link);

	RETERR(dns_message_gettemprdataset(msg, &tkeyset));
	dns_rdataset_init(tkeyset);
	RETERR(dns_rdatalist_tordataset(tkeylist, tkeyset));

	dns_name_init(qname, NULL);
	dns_name_clone(name, qname);

	dns_name_init(aname, NULL);
	dns_name_clone(name, aname);

	ISC_LIST_APPEND(qname->list, question, link);
	ISC_LIST_APPEND(aname->list, tkeyset, link);

	dns_message_addname(msg, qname, DNS_SECTION_QUESTION);

	/*
	 * Windows 2000 needs this in the answer section, not the additional
	 * section where the RFC specifies.
	 */
	if (win2k)
		dns_message_addname(msg, aname, DNS_SECTION_ANSWER);
	else
		dns_message_addname(msg, aname, DNS_SECTION_ADDITIONAL);

	return (ISC_R_SUCCESS);

 failure:
	if (qname != NULL)
		dns_message_puttempname(msg, &qname);
	if (aname != NULL)
		dns_message_puttempname(msg, &aname);
	if (question != NULL) {
		dns_rdataset_disassociate(question);
		dns_message_puttemprdataset(msg, &question);
	}
	if (dynbuf != NULL)
		isc_buffer_free(&dynbuf);
	printf("buildquery error\n");
	return (result);
}

isc_result_t
dns_tkey_builddhquery(dns_message_t *msg, dst_key_t *key, dns_name_t *name,
		      dns_name_t *algorithm, isc_buffer_t *nonce,
		      isc_uint32_t lifetime)
{
	dns_rdata_tkey_t tkey;
	dns_rdata_t *rdata = NULL;
	isc_buffer_t *dynbuf = NULL;
	isc_region_t r;
	dns_name_t keyname;
	dns_namelist_t namelist;
	isc_result_t result;
	isc_stdtime_t now;

	REQUIRE(msg != NULL);
	REQUIRE(key != NULL);
	REQUIRE(dst_key_alg(key) == DNS_KEYALG_DH);
	REQUIRE(dst_key_isprivate(key));
	REQUIRE(name != NULL);
	REQUIRE(algorithm != NULL);

	tkey.common.rdclass = dns_rdataclass_any;
	tkey.common.rdtype = dns_rdatatype_tkey;
	ISC_LINK_INIT(&tkey.common, link);
	tkey.mctx = msg->mctx;
	dns_name_init(&tkey.algorithm, NULL);
	dns_name_clone(algorithm, &tkey.algorithm);
	isc_stdtime_get(&now);
	tkey.inception = now;
	tkey.expire = now + lifetime;
	tkey.mode = DNS_TKEYMODE_DIFFIEHELLMAN;
	if (nonce != NULL)
		isc_buffer_usedregion(nonce, &r);
	else {
		r.base = isc_mem_get(msg->mctx, 0);
		r.length = 0;
	}
	tkey.error = 0;
	tkey.key = r.base;
	tkey.keylen =  r.length;
	tkey.other = NULL;
	tkey.otherlen = 0;

	RETERR(buildquery(msg, name, &tkey, ISC_FALSE));

	if (nonce == NULL)
		isc_mem_put(msg->mctx, r.base, 0);

	RETERR(dns_message_gettemprdata(msg, &rdata));
	RETERR(isc_buffer_allocate(msg->mctx, &dynbuf, 1024));
	RETERR(dst_key_todns(key, dynbuf));
	isc_buffer_usedregion(dynbuf, &r);
	dns_rdata_fromregion(rdata, dns_rdataclass_any,
			     dns_rdatatype_key, &r);
	dns_message_takebuffer(msg, &dynbuf);

	dns_name_init(&keyname, NULL);
	dns_name_clone(dst_key_name(key), &keyname);

	ISC_LIST_INIT(namelist);
	RETERR(add_rdata_to_list(msg, &keyname, rdata, 0, &namelist));
	dns_message_addname(msg, ISC_LIST_HEAD(namelist),
			    DNS_SECTION_ADDITIONAL);

	return (ISC_R_SUCCESS);

 failure:

	if (dynbuf != NULL)
		isc_buffer_free(&dynbuf);
	return (result);
}

isc_result_t
dns_tkey_buildgssquery(dns_message_t *msg, dns_name_t *name, dns_name_t *gname,
		       isc_buffer_t *intoken, isc_uint32_t lifetime,
		       gss_ctx_id_t *context, isc_boolean_t win2k)
{
	dns_rdata_tkey_t tkey;
	isc_result_t result;
	isc_stdtime_t now;
	isc_buffer_t token;
	unsigned char array[4096];

	UNUSED(intoken);

	REQUIRE(msg != NULL);
	REQUIRE(name != NULL);
	REQUIRE(gname != NULL);
	REQUIRE(context != NULL);

	isc_buffer_init(&token, array, sizeof(array));
	result = dst_gssapi_initctx(gname, NULL, &token, context);
	if (result != DNS_R_CONTINUE && result != ISC_R_SUCCESS)
		return (result);

	tkey.common.rdclass = dns_rdataclass_any;
	tkey.common.rdtype = dns_rdatatype_tkey;
	ISC_LINK_INIT(&tkey.common, link);
	tkey.mctx = NULL;
	dns_name_init(&tkey.algorithm, NULL);

	if (win2k)
		dns_name_clone(DNS_TSIG_GSSAPIMS_NAME, &tkey.algorithm);
	else
		dns_name_clone(DNS_TSIG_GSSAPI_NAME, &tkey.algorithm);

	isc_stdtime_get(&now);
	tkey.inception = now;
	tkey.expire = now + lifetime;
	tkey.mode = DNS_TKEYMODE_GSSAPI;
	tkey.error = 0;
	tkey.key = isc_buffer_base(&token);
	tkey.keylen = isc_buffer_usedlength(&token);
	tkey.other = NULL;
	tkey.otherlen = 0;

	RETERR(buildquery(msg, name, &tkey, win2k));

	return (ISC_R_SUCCESS);

 failure:
	return (result);
}

isc_result_t
dns_tkey_builddeletequery(dns_message_t *msg, dns_tsigkey_t *key) {
	dns_rdata_tkey_t tkey;

	REQUIRE(msg != NULL);
	REQUIRE(key != NULL);

	tkey.common.rdclass = dns_rdataclass_any;
	tkey.common.rdtype = dns_rdatatype_tkey;
	ISC_LINK_INIT(&tkey.common, link);
	tkey.mctx = msg->mctx;
	dns_name_init(&tkey.algorithm, NULL);
	dns_name_clone(key->algorithm, &tkey.algorithm);
	tkey.inception = tkey.expire = 0;
	tkey.mode = DNS_TKEYMODE_DELETE;
	tkey.error = 0;
	tkey.keylen = tkey.otherlen = 0;
	tkey.key = tkey.other = NULL;

	return (buildquery(msg, &key->name, &tkey, ISC_FALSE));
}

static isc_result_t
find_tkey(dns_message_t *msg, dns_name_t **name, dns_rdata_t *rdata,
	  int section)
{
	dns_rdataset_t *tkeyset;
	isc_result_t result;

	result = dns_message_firstname(msg, section);
	while (result == ISC_R_SUCCESS) {
		*name = NULL;
		dns_message_currentname(msg, section, name);
		tkeyset = NULL;
		result = dns_message_findtype(*name, dns_rdatatype_tkey, 0,
					      &tkeyset);
		if (result == ISC_R_SUCCESS) {
			result = dns_rdataset_first(tkeyset);
			if (result != ISC_R_SUCCESS)
				return (result);
			dns_rdataset_current(tkeyset, rdata);
			return (ISC_R_SUCCESS);
		}
		result = dns_message_nextname(msg, section);
	}
	if (result == ISC_R_NOMORE)
		return (ISC_R_NOTFOUND);
	return (result);
}

isc_result_t
dns_tkey_processdhresponse(dns_message_t *qmsg, dns_message_t *rmsg,
			   dst_key_t *key, isc_buffer_t *nonce,
			   dns_tsigkey_t **outkey, dns_tsig_keyring_t *ring)
{
	dns_rdata_t qtkeyrdata = DNS_RDATA_INIT, rtkeyrdata = DNS_RDATA_INIT;
	dns_name_t keyname, *tkeyname, *theirkeyname, *ourkeyname, *tempname;
	dns_rdataset_t *theirkeyset = NULL, *ourkeyset = NULL;
	dns_rdata_t theirkeyrdata = DNS_RDATA_INIT;
	dst_key_t *theirkey = NULL;
	dns_rdata_tkey_t qtkey, rtkey;
	unsigned char secretdata[256];
	unsigned int sharedsize;
	isc_buffer_t *shared = NULL, secret;
	isc_region_t r, r2;
	isc_result_t result;
	isc_boolean_t freertkey = ISC_FALSE;

	REQUIRE(qmsg != NULL);
	REQUIRE(rmsg != NULL);
	REQUIRE(key != NULL);
	REQUIRE(dst_key_alg(key) == DNS_KEYALG_DH);
	REQUIRE(dst_key_isprivate(key));
	if (outkey != NULL)
		REQUIRE(*outkey == NULL);

	if (rmsg->rcode != dns_rcode_noerror)
		return (ISC_RESULTCLASS_DNSRCODE + rmsg->rcode);
	RETERR(find_tkey(rmsg, &tkeyname, &rtkeyrdata, DNS_SECTION_ANSWER));
	RETERR(dns_rdata_tostruct(&rtkeyrdata, &rtkey, NULL));
	freertkey = ISC_TRUE;

	RETERR(find_tkey(qmsg, &tempname, &qtkeyrdata,
			 DNS_SECTION_ADDITIONAL));
	RETERR(dns_rdata_tostruct(&qtkeyrdata, &qtkey, NULL));

	if (rtkey.error != dns_rcode_noerror ||
	    rtkey.mode != DNS_TKEYMODE_DIFFIEHELLMAN ||
	    rtkey.mode != qtkey.mode ||
	    !dns_name_equal(&rtkey.algorithm, &qtkey.algorithm) ||
	    rmsg->rcode != dns_rcode_noerror) {
		tkey_log("dns_tkey_processdhresponse: tkey mode invalid "
			 "or error set(1)");
		result = DNS_R_INVALIDTKEY;
		dns_rdata_freestruct(&qtkey);
		goto failure;
	}

	dns_rdata_freestruct(&qtkey);

	dns_name_init(&keyname, NULL);
	dns_name_clone(dst_key_name(key), &keyname);

	ourkeyname = NULL;
	ourkeyset = NULL;
	RETERR(dns_message_findname(rmsg, DNS_SECTION_ANSWER, &keyname,
				    dns_rdatatype_key, 0, &ourkeyname,
				    &ourkeyset));

	result = dns_message_firstname(rmsg, DNS_SECTION_ANSWER);
	while (result == ISC_R_SUCCESS) {
		theirkeyname = NULL;
		dns_message_currentname(rmsg, DNS_SECTION_ANSWER,
					&theirkeyname);
		if (dns_name_equal(theirkeyname, ourkeyname))
			goto next;
		theirkeyset = NULL;
		result = dns_message_findtype(theirkeyname, dns_rdatatype_key,
					      0, &theirkeyset);
		if (result == ISC_R_SUCCESS) {
			RETERR(dns_rdataset_first(theirkeyset));
			break;
		}
 next:
		result = dns_message_nextname(rmsg, DNS_SECTION_ANSWER);
	}

	if (theirkeyset == NULL) {
		tkey_log("dns_tkey_processdhresponse: failed to find server "
			 "key");
		result = ISC_R_NOTFOUND;
		goto failure;
	}

	dns_rdataset_current(theirkeyset, &theirkeyrdata);
	RETERR(dns_dnssec_keyfromrdata(theirkeyname, &theirkeyrdata,
				       rmsg->mctx, &theirkey));

	RETERR(dst_key_secretsize(key, &sharedsize));
	RETERR(isc_buffer_allocate(rmsg->mctx, &shared, sharedsize));

	RETERR(dst_key_computesecret(theirkey, key, shared));

	isc_buffer_init(&secret, secretdata, sizeof(secretdata));

	r.base = rtkey.key;
	r.length = rtkey.keylen;
	if (nonce != NULL)
		isc_buffer_usedregion(nonce, &r2);
	else {
		r2.base = isc_mem_get(rmsg->mctx, 0);
		r2.length = 0;
	}
	RETERR(compute_secret(shared, &r2, &r, &secret));
	if (nonce == NULL)
		isc_mem_put(rmsg->mctx, r2.base, 0);

	isc_buffer_usedregion(&secret, &r);
	result = dns_tsigkey_create(tkeyname, &rtkey.algorithm,
				    r.base, r.length, ISC_TRUE,
				    NULL, rtkey.inception, rtkey.expire,
				    rmsg->mctx, ring, outkey);
	isc_buffer_free(&shared);
	dns_rdata_freestruct(&rtkey);
	dst_key_free(&theirkey);
	return (result);

 failure:
	if (shared != NULL)
		isc_buffer_free(&shared);

	if (theirkey != NULL)
		dst_key_free(&theirkey);

	if (freertkey)
		dns_rdata_freestruct(&rtkey);

	return (result);
}

isc_result_t
dns_tkey_processgssresponse(dns_message_t *qmsg, dns_message_t *rmsg,
			    dns_name_t *gname, gss_ctx_id_t *context,
			    isc_buffer_t *outtoken, dns_tsigkey_t **outkey,
			    dns_tsig_keyring_t *ring)
{
	dns_rdata_t rtkeyrdata = DNS_RDATA_INIT, qtkeyrdata = DNS_RDATA_INIT;
	dns_name_t *tkeyname;
	dns_rdata_tkey_t rtkey, qtkey;
	dst_key_t *dstkey = NULL;
	isc_buffer_t intoken;
	isc_result_t result;
	unsigned char array[1024];

	REQUIRE(outtoken != NULL);
	REQUIRE(qmsg != NULL);
	REQUIRE(rmsg != NULL);
	REQUIRE(gname != NULL);
	if (outkey != NULL)
		REQUIRE(*outkey == NULL);

	if (rmsg->rcode != dns_rcode_noerror)
		return (ISC_RESULTCLASS_DNSRCODE + rmsg->rcode);
	RETERR(find_tkey(rmsg, &tkeyname, &rtkeyrdata, DNS_SECTION_ANSWER));
	RETERR(dns_rdata_tostruct(&rtkeyrdata, &rtkey, NULL));

	/*
	 * Win2k puts the item in the ANSWER section, while the RFC
	 * specifies it should be in the ADDITIONAL section.  Check first
	 * where it should be, and then where it may be.
	 */
	result = find_tkey(qmsg, &tkeyname, &qtkeyrdata,
			   DNS_SECTION_ADDITIONAL);
	if (result == ISC_R_NOTFOUND)
		result = find_tkey(qmsg, &tkeyname, &qtkeyrdata,
				   DNS_SECTION_ANSWER);
	if (result != ISC_R_SUCCESS)
		goto failure;

	RETERR(dns_rdata_tostruct(&qtkeyrdata, &qtkey, NULL));

	if (rtkey.error != dns_rcode_noerror ||
	    rtkey.mode != DNS_TKEYMODE_GSSAPI ||
	    !dns_name_equal(&rtkey.algorithm, &qtkey.algorithm)) {
		tkey_log("dns_tkey_processgssresponse: tkey mode invalid "
			 "or error set(2) %d", rtkey.error);
		_dns_tkey_dumpmessage(qmsg);
		_dns_tkey_dumpmessage(rmsg);
		result = DNS_R_INVALIDTKEY;
		goto failure;
	}

	isc_buffer_init(outtoken, array, sizeof(array));
	isc_buffer_init(&intoken, rtkey.key, rtkey.keylen);
	RETERR(dst_gssapi_initctx(gname, &intoken, outtoken, context));

	dstkey = NULL;
	RETERR(dst_key_fromgssapi(dns_rootname, *context, rmsg->mctx,
				  &dstkey));

	RETERR(dns_tsigkey_createfromkey(tkeyname, DNS_TSIG_GSSAPI_NAME,
					 dstkey, ISC_FALSE, NULL,
					 rtkey.inception, rtkey.expire,
					 ring->mctx, ring, outkey));

	dns_rdata_freestruct(&rtkey);
	return (result);

 failure:
	/*
	 * XXXSRA This probably leaks memory from rtkey and qtkey.
	 */
	return (result);
}

isc_result_t
dns_tkey_processdeleteresponse(dns_message_t *qmsg, dns_message_t *rmsg,
			       dns_tsig_keyring_t *ring)
{
	dns_rdata_t qtkeyrdata = DNS_RDATA_INIT, rtkeyrdata = DNS_RDATA_INIT;
	dns_name_t *tkeyname, *tempname;
	dns_rdata_tkey_t qtkey, rtkey;
	dns_tsigkey_t *tsigkey = NULL;
	isc_result_t result;

	REQUIRE(qmsg != NULL);
	REQUIRE(rmsg != NULL);

	if (rmsg->rcode != dns_rcode_noerror)
		return(ISC_RESULTCLASS_DNSRCODE + rmsg->rcode);

	RETERR(find_tkey(rmsg, &tkeyname, &rtkeyrdata, DNS_SECTION_ANSWER));
	RETERR(dns_rdata_tostruct(&rtkeyrdata, &rtkey, NULL));

	RETERR(find_tkey(qmsg, &tempname, &qtkeyrdata,
			 DNS_SECTION_ADDITIONAL));
	RETERR(dns_rdata_tostruct(&qtkeyrdata, &qtkey, NULL));

	if (rtkey.error != dns_rcode_noerror ||
	    rtkey.mode != DNS_TKEYMODE_DELETE ||
	    rtkey.mode != qtkey.mode ||
	    !dns_name_equal(&rtkey.algorithm, &qtkey.algorithm) ||
	    rmsg->rcode != dns_rcode_noerror) {
		tkey_log("dns_tkey_processdeleteresponse: tkey mode invalid "
			 "or error set(3)");
		result = DNS_R_INVALIDTKEY;
		dns_rdata_freestruct(&qtkey);
		dns_rdata_freestruct(&rtkey);
		goto failure;
	}

	dns_rdata_freestruct(&qtkey);

	RETERR(dns_tsigkey_find(&tsigkey, tkeyname, &rtkey.algorithm, ring));

	dns_rdata_freestruct(&rtkey);

	/*
	 * Mark the key as deleted.
	 */
	dns_tsigkey_setdeleted(tsigkey);
	/*
	 * Release the reference.
	 */
	dns_tsigkey_detach(&tsigkey);

 failure:
	return (result);
}

isc_result_t
dns_tkey_gssnegotiate(dns_message_t *qmsg, dns_message_t *rmsg,
		      dns_name_t *server, gss_ctx_id_t *context,
		      dns_tsigkey_t **outkey, dns_tsig_keyring_t *ring,
		      isc_boolean_t win2k)
{
	dns_rdata_t rtkeyrdata = DNS_RDATA_INIT, qtkeyrdata = DNS_RDATA_INIT;
	dns_name_t *tkeyname;
	dns_rdata_tkey_t rtkey, qtkey;
	isc_buffer_t intoken, outtoken;
	dst_key_t *dstkey = NULL;
	isc_result_t result;
	unsigned char array[1024];

	REQUIRE(qmsg != NULL);
	REQUIRE(rmsg != NULL);
	REQUIRE(server != NULL);
	if (outkey != NULL)
		REQUIRE(*outkey == NULL);

	if (rmsg->rcode != dns_rcode_noerror)
		return (ISC_RESULTCLASS_DNSRCODE + rmsg->rcode);

	RETERR(find_tkey(rmsg, &tkeyname, &rtkeyrdata, DNS_SECTION_ANSWER));
	RETERR(dns_rdata_tostruct(&rtkeyrdata, &rtkey, NULL));

	if (win2k == ISC_TRUE)
		RETERR(find_tkey(qmsg, &tkeyname, &qtkeyrdata,
			 DNS_SECTION_ANSWER));
	else
		RETERR(find_tkey(qmsg, &tkeyname, &qtkeyrdata,
			 DNS_SECTION_ADDITIONAL));

	RETERR(dns_rdata_tostruct(&qtkeyrdata, &qtkey, NULL));

	if (rtkey.error != dns_rcode_noerror ||
	    rtkey.mode != DNS_TKEYMODE_GSSAPI ||
	    !dns_name_equal(&rtkey.algorithm, &qtkey.algorithm))
	{
		tkey_log("dns_tkey_processdhresponse: tkey mode invalid "
			 "or error set(4)");
		result = DNS_R_INVALIDTKEY;
		goto failure;
	}

	isc_buffer_init(&intoken, rtkey.key, rtkey.keylen);
	isc_buffer_init(&outtoken, array, sizeof(array));

	result = dst_gssapi_initctx(server, &intoken, &outtoken, context);
	if (result != DNS_R_CONTINUE && result != ISC_R_SUCCESS)
		return (result);

	dstkey = NULL;
	RETERR(dst_key_fromgssapi(dns_rootname, *context, rmsg->mctx,
				  &dstkey));

	/*
	 * XXXSRA This seems confused.  If we got CONTINUE from initctx,
	 * the GSS negotiation hasn't completed yet, so we can't sign
	 * anything yet.
	 */

	RETERR(dns_tsigkey_createfromkey(tkeyname,
					 (win2k
					  ? DNS_TSIG_GSSAPIMS_NAME
					  : DNS_TSIG_GSSAPI_NAME),
					 dstkey, ISC_TRUE, NULL,
					 rtkey.inception, rtkey.expire,
					 ring->mctx, ring, outkey));

	dns_rdata_freestruct(&rtkey);
	return (result);

 failure:
	/*
	 * XXXSRA This probably leaks memory from qtkey.
	 */
	dns_rdata_freestruct(&rtkey);
	return (result);
}
