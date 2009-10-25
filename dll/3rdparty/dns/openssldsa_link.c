/*
 * Portions Copyright (C) 2004-2009  Internet Systems Consortium, Inc. ("ISC")
 * Portions Copyright (C) 1999-2002  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC AND NETWORK ASSOCIATES DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE
 * FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Portions Copyright (C) 1995-2000 by Network Associates, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC AND NETWORK ASSOCIATES DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE
 * FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id: openssldsa_link.c,v 1.13.120.2 2009/01/14 23:47:26 tbox Exp $ */

#ifdef OPENSSL
#ifndef USE_EVP
#define USE_EVP 1
#endif

#include <config.h>

#include <string.h>

#include <isc/entropy.h>
#include <isc/mem.h>
#include <isc/sha1.h>
#include <isc/util.h>

#include <dst/result.h>

#include "dst_internal.h"
#include "dst_openssl.h"
#include "dst_parse.h"

#include <openssl/dsa.h>

static isc_result_t openssldsa_todns(const dst_key_t *key, isc_buffer_t *data);

static isc_result_t
openssldsa_createctx(dst_key_t *key, dst_context_t *dctx) {
#if USE_EVP
	EVP_MD_CTX *evp_md_ctx;

	UNUSED(key);

	evp_md_ctx = EVP_MD_CTX_create();
	if (evp_md_ctx == NULL)
		return (ISC_R_NOMEMORY);

	if (!EVP_DigestInit_ex(evp_md_ctx, EVP_dss1(), NULL)) {
		EVP_MD_CTX_destroy(evp_md_ctx);
			return (ISC_R_FAILURE);
	}

	dctx->ctxdata.evp_md_ctx = evp_md_ctx;

	return (ISC_R_SUCCESS);
#else
	isc_sha1_t *sha1ctx;

	UNUSED(key);

	sha1ctx = isc_mem_get(dctx->mctx, sizeof(isc_sha1_t));
	isc_sha1_init(sha1ctx);
	dctx->ctxdata.sha1ctx = sha1ctx;
	return (ISC_R_SUCCESS);
#endif
}

static void
openssldsa_destroyctx(dst_context_t *dctx) {
#if USE_EVP
	EVP_MD_CTX *evp_md_ctx = dctx->ctxdata.evp_md_ctx;

	if (evp_md_ctx != NULL) {
		EVP_MD_CTX_destroy(evp_md_ctx);
		dctx->ctxdata.evp_md_ctx = NULL;
	}
#else
	isc_sha1_t *sha1ctx = dctx->ctxdata.sha1ctx;

	if (sha1ctx != NULL) {
		isc_sha1_invalidate(sha1ctx);
		isc_mem_put(dctx->mctx, sha1ctx, sizeof(isc_sha1_t));
		dctx->ctxdata.sha1ctx = NULL;
	}
#endif
}

static isc_result_t
openssldsa_adddata(dst_context_t *dctx, const isc_region_t *data) {
#if USE_EVP
	EVP_MD_CTX *evp_md_ctx = dctx->ctxdata.evp_md_ctx;

	if (!EVP_DigestUpdate(evp_md_ctx, data->base, data->length)) {
		return (ISC_R_FAILURE);
	}
#else
	isc_sha1_t *sha1ctx = dctx->ctxdata.sha1ctx;

	isc_sha1_update(sha1ctx, data->base, data->length);
#endif
	return (ISC_R_SUCCESS);
}

static int
BN_bn2bin_fixed(BIGNUM *bn, unsigned char *buf, int size) {
	int bytes = size - BN_num_bytes(bn);
	while (bytes-- > 0)
		*buf++ = 0;
	BN_bn2bin(bn, buf);
	return (size);
}

static isc_result_t
openssldsa_sign(dst_context_t *dctx, isc_buffer_t *sig) {
	dst_key_t *key = dctx->key;
	DSA *dsa = key->keydata.dsa;
	isc_region_t r;
	DSA_SIG *dsasig;
#if USE_EVP
	EVP_MD_CTX *evp_md_ctx = dctx->ctxdata.evp_md_ctx;
	EVP_PKEY *pkey;
	unsigned char *sigbuf;
	const unsigned char *sb;
	unsigned int siglen;
#else
	isc_sha1_t *sha1ctx = dctx->ctxdata.sha1ctx;
	unsigned char digest[ISC_SHA1_DIGESTLENGTH];
#endif

	isc_buffer_availableregion(sig, &r);
	if (r.length < ISC_SHA1_DIGESTLENGTH * 2 + 1)
		return (ISC_R_NOSPACE);

#if USE_EVP
	pkey = EVP_PKEY_new();
	if (pkey == NULL)
		return (ISC_R_NOMEMORY);
	if (!EVP_PKEY_set1_DSA(pkey, dsa)) {
		EVP_PKEY_free(pkey);
		return (ISC_R_FAILURE);
	}
	sigbuf = malloc(EVP_PKEY_size(pkey));
	if (sigbuf == NULL) {
		EVP_PKEY_free(pkey);
		return (ISC_R_NOMEMORY);
	}
	if (!EVP_SignFinal(evp_md_ctx, sigbuf, &siglen, pkey)) {
		EVP_PKEY_free(pkey);
		free(sigbuf);
		return (ISC_R_FAILURE);
	}
	INSIST(EVP_PKEY_size(pkey) >= (int) siglen);
	EVP_PKEY_free(pkey);
	/* Convert from Dss-Sig-Value (RFC2459). */
	dsasig = DSA_SIG_new();
	if (dsasig == NULL) {
		free(sigbuf);
		return (ISC_R_NOMEMORY);
	}
	sb = sigbuf;
	if (d2i_DSA_SIG(&dsasig, &sb, (long) siglen) == NULL) {
		free(sigbuf);
		return (ISC_R_FAILURE);
	}
	free(sigbuf);
#elif 0
	/* Only use EVP for the Digest */
	if (!EVP_DigestFinal_ex(evp_md_ctx, digest, &siglen)) {
		return (ISC_R_FAILURE);
	}
	dsasig = DSA_do_sign(digest, ISC_SHA1_DIGESTLENGTH, dsa);
	if (dsasig == NULL)
		return (dst__openssl_toresult(DST_R_SIGNFAILURE));
#else
	isc_sha1_final(sha1ctx, digest);

	dsasig = DSA_do_sign(digest, ISC_SHA1_DIGESTLENGTH, dsa);
	if (dsasig == NULL)
		return (dst__openssl_toresult(DST_R_SIGNFAILURE));
#endif
	*r.base++ = (key->key_size - 512)/64;
	BN_bn2bin_fixed(dsasig->r, r.base, ISC_SHA1_DIGESTLENGTH);
	r.base += ISC_SHA1_DIGESTLENGTH;
	BN_bn2bin_fixed(dsasig->s, r.base, ISC_SHA1_DIGESTLENGTH);
	r.base += ISC_SHA1_DIGESTLENGTH;
	DSA_SIG_free(dsasig);
	isc_buffer_add(sig, ISC_SHA1_DIGESTLENGTH * 2 + 1);

	return (ISC_R_SUCCESS);
}

static isc_result_t
openssldsa_verify(dst_context_t *dctx, const isc_region_t *sig) {
	dst_key_t *key = dctx->key;
	DSA *dsa = key->keydata.dsa;
	int status = 0;
	unsigned char *cp = sig->base;
	DSA_SIG *dsasig;
#if USE_EVP
	EVP_MD_CTX *evp_md_ctx = dctx->ctxdata.evp_md_ctx;
#if 0
	EVP_PKEY *pkey;
	unsigned char *sigbuf;
#endif
	unsigned int siglen;
#else
	isc_sha1_t *sha1ctx = dctx->ctxdata.sha1ctx;
#endif
	unsigned char digest[ISC_SHA1_DIGESTLENGTH];


#if USE_EVP
#if 1
	/* Only use EVP for the digest */
	if (!EVP_DigestFinal_ex(evp_md_ctx, digest, &siglen)) {
		return (ISC_R_FAILURE);
	}
#endif
#else
	isc_sha1_final(sha1ctx, digest);
#endif

	if (sig->length != 2 * ISC_SHA1_DIGESTLENGTH + 1) {
		return (DST_R_VERIFYFAILURE);
	}

	cp++;	/*%< Skip T */
	dsasig = DSA_SIG_new();
	if (dsasig == NULL)
		return (ISC_R_NOMEMORY);
	dsasig->r = BN_bin2bn(cp, ISC_SHA1_DIGESTLENGTH, NULL);
	cp += ISC_SHA1_DIGESTLENGTH;
	dsasig->s = BN_bin2bn(cp, ISC_SHA1_DIGESTLENGTH, NULL);
	cp += ISC_SHA1_DIGESTLENGTH;

#if 0
	pkey = EVP_PKEY_new();
	if (pkey == NULL)
		return (ISC_R_NOMEMORY);
	if (!EVP_PKEY_set1_DSA(pkey, dsa)) {
		EVP_PKEY_free(pkey);
		return (ISC_R_FAILURE);
	}
	/* Convert to Dss-Sig-Value (RFC2459). */
	sigbuf = malloc(EVP_PKEY_size(pkey) + 50);
	if (sigbuf == NULL) {
		EVP_PKEY_free(pkey);
		return (ISC_R_NOMEMORY);
	}
	siglen = (unsigned) i2d_DSA_SIG(dsasig, &sigbuf);
	INSIST(EVP_PKEY_size(pkey) >= (int) siglen);
	status = EVP_VerifyFinal(evp_md_ctx, sigbuf, siglen, pkey);
	EVP_PKEY_free(pkey);
	free(sigbuf);
#else
	status = DSA_do_verify(digest, ISC_SHA1_DIGESTLENGTH, dsasig, dsa);
#endif
	DSA_SIG_free(dsasig);
	if (status != 1)
		return (dst__openssl_toresult(DST_R_VERIFYFAILURE));

	return (ISC_R_SUCCESS);
}

static isc_boolean_t
openssldsa_compare(const dst_key_t *key1, const dst_key_t *key2) {
	int status;
	DSA *dsa1, *dsa2;

	dsa1 = key1->keydata.dsa;
	dsa2 = key2->keydata.dsa;

	if (dsa1 == NULL && dsa2 == NULL)
		return (ISC_TRUE);
	else if (dsa1 == NULL || dsa2 == NULL)
		return (ISC_FALSE);

	status = BN_cmp(dsa1->p, dsa2->p) ||
		 BN_cmp(dsa1->q, dsa2->q) ||
		 BN_cmp(dsa1->g, dsa2->g) ||
		 BN_cmp(dsa1->pub_key, dsa2->pub_key);

	if (status != 0)
		return (ISC_FALSE);

	if (dsa1->priv_key != NULL || dsa2->priv_key != NULL) {
		if (dsa1->priv_key == NULL || dsa2->priv_key == NULL)
			return (ISC_FALSE);
		if (BN_cmp(dsa1->priv_key, dsa2->priv_key))
			return (ISC_FALSE);
	}
	return (ISC_TRUE);
}

static isc_result_t
openssldsa_generate(dst_key_t *key, int unused) {
#if OPENSSL_VERSION_NUMBER > 0x00908000L
	BN_GENCB cb;
#endif
	DSA *dsa;
	unsigned char rand_array[ISC_SHA1_DIGESTLENGTH];
	isc_result_t result;

	UNUSED(unused);

	result = dst__entropy_getdata(rand_array, sizeof(rand_array),
				      ISC_FALSE);
	if (result != ISC_R_SUCCESS)
		return (result);

#if OPENSSL_VERSION_NUMBER > 0x00908000L
	dsa = DSA_new();
	if (dsa == NULL)
		return (dst__openssl_toresult(DST_R_OPENSSLFAILURE));

	BN_GENCB_set_old(&cb, NULL, NULL);

	if (!DSA_generate_parameters_ex(dsa, key->key_size, rand_array,
					ISC_SHA1_DIGESTLENGTH,  NULL, NULL,
					&cb))
	{
		DSA_free(dsa);
		return (dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}
#else
	dsa = DSA_generate_parameters(key->key_size, rand_array,
				      ISC_SHA1_DIGESTLENGTH, NULL, NULL,
				      NULL, NULL);
	if (dsa == NULL)
		return (dst__openssl_toresult(DST_R_OPENSSLFAILURE));
#endif

	if (DSA_generate_key(dsa) == 0) {
		DSA_free(dsa);
		return (dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}
	dsa->flags &= ~DSA_FLAG_CACHE_MONT_P;

	key->keydata.dsa = dsa;

	return (ISC_R_SUCCESS);
}

static isc_boolean_t
openssldsa_isprivate(const dst_key_t *key) {
	DSA *dsa = key->keydata.dsa;
	return (ISC_TF(dsa != NULL && dsa->priv_key != NULL));
}

static void
openssldsa_destroy(dst_key_t *key) {
	DSA *dsa = key->keydata.dsa;
	DSA_free(dsa);
	key->keydata.dsa = NULL;
}


static isc_result_t
openssldsa_todns(const dst_key_t *key, isc_buffer_t *data) {
	DSA *dsa;
	isc_region_t r;
	int dnslen;
	unsigned int t, p_bytes;

	REQUIRE(key->keydata.dsa != NULL);

	dsa = key->keydata.dsa;

	isc_buffer_availableregion(data, &r);

	t = (BN_num_bytes(dsa->p) - 64) / 8;
	if (t > 8)
		return (DST_R_INVALIDPUBLICKEY);
	p_bytes = 64 + 8 * t;

	dnslen = 1 + (key->key_size * 3)/8 + ISC_SHA1_DIGESTLENGTH;
	if (r.length < (unsigned int) dnslen)
		return (ISC_R_NOSPACE);

	*r.base++ = t;
	BN_bn2bin_fixed(dsa->q, r.base, ISC_SHA1_DIGESTLENGTH);
	r.base += ISC_SHA1_DIGESTLENGTH;
	BN_bn2bin_fixed(dsa->p, r.base, key->key_size/8);
	r.base += p_bytes;
	BN_bn2bin_fixed(dsa->g, r.base, key->key_size/8);
	r.base += p_bytes;
	BN_bn2bin_fixed(dsa->pub_key, r.base, key->key_size/8);
	r.base += p_bytes;

	isc_buffer_add(data, dnslen);

	return (ISC_R_SUCCESS);
}

static isc_result_t
openssldsa_fromdns(dst_key_t *key, isc_buffer_t *data) {
	DSA *dsa;
	isc_region_t r;
	unsigned int t, p_bytes;
	isc_mem_t *mctx = key->mctx;

	UNUSED(mctx);

	isc_buffer_remainingregion(data, &r);
	if (r.length == 0)
		return (ISC_R_SUCCESS);

	dsa = DSA_new();
	if (dsa == NULL)
		return (ISC_R_NOMEMORY);
	dsa->flags &= ~DSA_FLAG_CACHE_MONT_P;

	t = (unsigned int) *r.base++;
	if (t > 8) {
		DSA_free(dsa);
		return (DST_R_INVALIDPUBLICKEY);
	}
	p_bytes = 64 + 8 * t;

	if (r.length < 1 + ISC_SHA1_DIGESTLENGTH + 3 * p_bytes) {
		DSA_free(dsa);
		return (DST_R_INVALIDPUBLICKEY);
	}

	dsa->q = BN_bin2bn(r.base, ISC_SHA1_DIGESTLENGTH, NULL);
	r.base += ISC_SHA1_DIGESTLENGTH;

	dsa->p = BN_bin2bn(r.base, p_bytes, NULL);
	r.base += p_bytes;

	dsa->g = BN_bin2bn(r.base, p_bytes, NULL);
	r.base += p_bytes;

	dsa->pub_key = BN_bin2bn(r.base, p_bytes, NULL);
	r.base += p_bytes;

	key->key_size = p_bytes * 8;

	isc_buffer_forward(data, 1 + ISC_SHA1_DIGESTLENGTH + 3 * p_bytes);

	key->keydata.dsa = dsa;

	return (ISC_R_SUCCESS);
}


static isc_result_t
openssldsa_tofile(const dst_key_t *key, const char *directory) {
	int cnt = 0;
	DSA *dsa;
	dst_private_t priv;
	unsigned char bufs[5][128];

	if (key->keydata.dsa == NULL)
		return (DST_R_NULLKEY);

	dsa = key->keydata.dsa;

	priv.elements[cnt].tag = TAG_DSA_PRIME;
	priv.elements[cnt].length = BN_num_bytes(dsa->p);
	BN_bn2bin(dsa->p, bufs[cnt]);
	priv.elements[cnt].data = bufs[cnt];
	cnt++;

	priv.elements[cnt].tag = TAG_DSA_SUBPRIME;
	priv.elements[cnt].length = BN_num_bytes(dsa->q);
	BN_bn2bin(dsa->q, bufs[cnt]);
	priv.elements[cnt].data = bufs[cnt];
	cnt++;

	priv.elements[cnt].tag = TAG_DSA_BASE;
	priv.elements[cnt].length = BN_num_bytes(dsa->g);
	BN_bn2bin(dsa->g, bufs[cnt]);
	priv.elements[cnt].data = bufs[cnt];
	cnt++;

	priv.elements[cnt].tag = TAG_DSA_PRIVATE;
	priv.elements[cnt].length = BN_num_bytes(dsa->priv_key);
	BN_bn2bin(dsa->priv_key, bufs[cnt]);
	priv.elements[cnt].data = bufs[cnt];
	cnt++;

	priv.elements[cnt].tag = TAG_DSA_PUBLIC;
	priv.elements[cnt].length = BN_num_bytes(dsa->pub_key);
	BN_bn2bin(dsa->pub_key, bufs[cnt]);
	priv.elements[cnt].data = bufs[cnt];
	cnt++;

	priv.nelements = cnt;
	return (dst__privstruct_writefile(key, &priv, directory));
}

static isc_result_t
openssldsa_parse(dst_key_t *key, isc_lex_t *lexer) {
	dst_private_t priv;
	isc_result_t ret;
	int i;
	DSA *dsa = NULL;
	isc_mem_t *mctx = key->mctx;
#define DST_RET(a) {ret = a; goto err;}

	/* read private key file */
	ret = dst__privstruct_parse(key, DST_ALG_DSA, lexer, mctx, &priv);
	if (ret != ISC_R_SUCCESS)
		return (ret);

	dsa = DSA_new();
	if (dsa == NULL)
		DST_RET(ISC_R_NOMEMORY);
	dsa->flags &= ~DSA_FLAG_CACHE_MONT_P;
	key->keydata.dsa = dsa;

	for (i=0; i < priv.nelements; i++) {
		BIGNUM *bn;
		bn = BN_bin2bn(priv.elements[i].data,
			       priv.elements[i].length, NULL);
		if (bn == NULL)
			DST_RET(ISC_R_NOMEMORY);

		switch (priv.elements[i].tag) {
			case TAG_DSA_PRIME:
				dsa->p = bn;
				break;
			case TAG_DSA_SUBPRIME:
				dsa->q = bn;
				break;
			case TAG_DSA_BASE:
				dsa->g = bn;
				break;
			case TAG_DSA_PRIVATE:
				dsa->priv_key = bn;
				break;
			case TAG_DSA_PUBLIC:
				dsa->pub_key = bn;
				break;
		}
	}
	dst__privstruct_free(&priv, mctx);

	key->key_size = BN_num_bits(dsa->p);

	return (ISC_R_SUCCESS);

 err:
	openssldsa_destroy(key);
	dst__privstruct_free(&priv, mctx);
	memset(&priv, 0, sizeof(priv));
	return (ret);
}

static dst_func_t openssldsa_functions = {
	openssldsa_createctx,
	openssldsa_destroyctx,
	openssldsa_adddata,
	openssldsa_sign,
	openssldsa_verify,
	NULL, /*%< computesecret */
	openssldsa_compare,
	NULL, /*%< paramcompare */
	openssldsa_generate,
	openssldsa_isprivate,
	openssldsa_destroy,
	openssldsa_todns,
	openssldsa_fromdns,
	openssldsa_tofile,
	openssldsa_parse,
	NULL, /*%< cleanup */
	NULL, /*%< fromlabel */
};

isc_result_t
dst__openssldsa_init(dst_func_t **funcp) {
	REQUIRE(funcp != NULL);
	if (*funcp == NULL)
		*funcp = &openssldsa_functions;
	return (ISC_R_SUCCESS);
}

#else /* OPENSSL */

#include <isc/util.h>

EMPTY_TRANSLATION_UNIT

#endif /* OPENSSL */
/*! \file */
