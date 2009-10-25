/*
 * Portions Copyright (C) 2004-2008  Internet Systems Consortium, Inc. ("ISC")
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

/*
 * Principal Author: Brian Wellington
 * $Id: openssldh_link.c,v 1.14 2008/04/01 23:47:10 tbox Exp $
 */

#ifdef OPENSSL

#include <config.h>

#include <ctype.h>

#include <isc/mem.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dst/result.h>

#include "dst_internal.h"
#include "dst_openssl.h"
#include "dst_parse.h"

#define PRIME768 "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088" \
	"A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF25" \
	"F14374FE1356D6D51C245E485B576625E7EC6F44C42E9A63A3620FFFFFFFFFFFFFFFF"

#define PRIME1024 "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E08" \
	"8A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF2" \
	"5F14374FE1356D6D51C245E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406" \
	"B7EDEE386BFB5A899FA5AE9F24117C4B1FE649286651ECE65381FFFFFFFFFFFFFFFF"

#define PRIME1536 "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1" \
	"29024E088A67CC74020BBEA63B139B22514A08798E3404DD" \
	"EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245" \
	"E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED" \
	"EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3D" \
	"C2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F" \
	"83655D23DCA3AD961C62F356208552BB9ED529077096966D" \
	"670C354E4ABC9804F1746C08CA237327FFFFFFFFFFFFFFFF"


static isc_result_t openssldh_todns(const dst_key_t *key, isc_buffer_t *data);

static BIGNUM bn2, bn768, bn1024, bn1536;

static isc_result_t
openssldh_computesecret(const dst_key_t *pub, const dst_key_t *priv,
			isc_buffer_t *secret)
{
	DH *dhpub, *dhpriv;
	int ret;
	isc_region_t r;
	unsigned int len;

	REQUIRE(pub->keydata.dh != NULL);
	REQUIRE(priv->keydata.dh != NULL);

	dhpub = pub->keydata.dh;
	dhpriv = priv->keydata.dh;

	len = DH_size(dhpriv);
	isc_buffer_availableregion(secret, &r);
	if (r.length < len)
		return (ISC_R_NOSPACE);
	ret = DH_compute_key(r.base, dhpub->pub_key, dhpriv);
	if (ret == 0)
		return (dst__openssl_toresult(DST_R_COMPUTESECRETFAILURE));
	isc_buffer_add(secret, len);
	return (ISC_R_SUCCESS);
}

static isc_boolean_t
openssldh_compare(const dst_key_t *key1, const dst_key_t *key2) {
	int status;
	DH *dh1, *dh2;

	dh1 = key1->keydata.dh;
	dh2 = key2->keydata.dh;

	if (dh1 == NULL && dh2 == NULL)
		return (ISC_TRUE);
	else if (dh1 == NULL || dh2 == NULL)
		return (ISC_FALSE);

	status = BN_cmp(dh1->p, dh2->p) ||
		 BN_cmp(dh1->g, dh2->g) ||
		 BN_cmp(dh1->pub_key, dh2->pub_key);

	if (status != 0)
		return (ISC_FALSE);

	if (dh1->priv_key != NULL || dh2->priv_key != NULL) {
		if (dh1->priv_key == NULL || dh2->priv_key == NULL)
			return (ISC_FALSE);
		if (BN_cmp(dh1->priv_key, dh2->priv_key) != 0)
			return (ISC_FALSE);
	}
	return (ISC_TRUE);
}

static isc_boolean_t
openssldh_paramcompare(const dst_key_t *key1, const dst_key_t *key2) {
	int status;
	DH *dh1, *dh2;

	dh1 = key1->keydata.dh;
	dh2 = key2->keydata.dh;

	if (dh1 == NULL && dh2 == NULL)
		return (ISC_TRUE);
	else if (dh1 == NULL || dh2 == NULL)
		return (ISC_FALSE);

	status = BN_cmp(dh1->p, dh2->p) ||
		 BN_cmp(dh1->g, dh2->g);

	if (status != 0)
		return (ISC_FALSE);
	return (ISC_TRUE);
}

static isc_result_t
openssldh_generate(dst_key_t *key, int generator) {
#if OPENSSL_VERSION_NUMBER > 0x00908000L
	BN_GENCB cb;
#endif
	DH *dh = NULL;

	if (generator == 0) {
		if (key->key_size == 768 ||
		    key->key_size == 1024 ||
		    key->key_size == 1536)
		{
			dh = DH_new();
			if (dh == NULL)
				return (dst__openssl_toresult(ISC_R_NOMEMORY));
			if (key->key_size == 768)
				dh->p = &bn768;
			else if (key->key_size == 1024)
				dh->p = &bn1024;
			else
				dh->p = &bn1536;
			dh->g = &bn2;
		} else
			generator = 2;
	}

	if (generator != 0) {
#if OPENSSL_VERSION_NUMBER > 0x00908000L
		dh = DH_new();
		if (dh == NULL)
			return (dst__openssl_toresult(DST_R_OPENSSLFAILURE));

		BN_GENCB_set_old(&cb, NULL, NULL);

		if (!DH_generate_parameters_ex(dh, key->key_size, generator,
					       &cb)) {
			DH_free(dh);
			return (dst__openssl_toresult(DST_R_OPENSSLFAILURE));
		}
#else
		dh = DH_generate_parameters(key->key_size, generator,
					    NULL, NULL);
#endif
	}

	if (dh == NULL)
		return (dst__openssl_toresult(DST_R_OPENSSLFAILURE));

	if (DH_generate_key(dh) == 0) {
		DH_free(dh);
		return (dst__openssl_toresult(DST_R_OPENSSLFAILURE));
	}
	dh->flags &= ~DH_FLAG_CACHE_MONT_P;

	key->keydata.dh = dh;

	return (ISC_R_SUCCESS);
}

static isc_boolean_t
openssldh_isprivate(const dst_key_t *key) {
	DH *dh = key->keydata.dh;
	return (ISC_TF(dh != NULL && dh->priv_key != NULL));
}

static void
openssldh_destroy(dst_key_t *key) {
	DH *dh = key->keydata.dh;

	if (dh == NULL)
		return;

	if (dh->p == &bn768 || dh->p == &bn1024 || dh->p == &bn1536)
		dh->p = NULL;
	if (dh->g == &bn2)
		dh->g = NULL;
	DH_free(dh);
	key->keydata.dh = NULL;
}

static void
uint16_toregion(isc_uint16_t val, isc_region_t *region) {
	*region->base++ = (val & 0xff00) >> 8;
	*region->base++ = (val & 0x00ff);
}

static isc_uint16_t
uint16_fromregion(isc_region_t *region) {
	isc_uint16_t val;
	unsigned char *cp = region->base;

	val = ((unsigned int)(cp[0])) << 8;
	val |= ((unsigned int)(cp[1]));

	region->base += 2;
	return (val);
}

static isc_result_t
openssldh_todns(const dst_key_t *key, isc_buffer_t *data) {
	DH *dh;
	isc_region_t r;
	isc_uint16_t dnslen, plen, glen, publen;

	REQUIRE(key->keydata.dh != NULL);

	dh = key->keydata.dh;

	isc_buffer_availableregion(data, &r);

	if (dh->g == &bn2 &&
	    (dh->p == &bn768 || dh->p == &bn1024 || dh->p == &bn1536)) {
		plen = 1;
		glen = 0;
	}
	else {
		plen = BN_num_bytes(dh->p);
		glen = BN_num_bytes(dh->g);
	}
	publen = BN_num_bytes(dh->pub_key);
	dnslen = plen + glen + publen + 6;
	if (r.length < (unsigned int) dnslen)
		return (ISC_R_NOSPACE);

	uint16_toregion(plen, &r);
	if (plen == 1) {
		if (dh->p == &bn768)
			*r.base = 1;
		else if (dh->p == &bn1024)
			*r.base = 2;
		else
			*r.base = 3;
	}
	else
		BN_bn2bin(dh->p, r.base);
	r.base += plen;

	uint16_toregion(glen, &r);
	if (glen > 0)
		BN_bn2bin(dh->g, r.base);
	r.base += glen;

	uint16_toregion(publen, &r);
	BN_bn2bin(dh->pub_key, r.base);
	r.base += publen;

	isc_buffer_add(data, dnslen);

	return (ISC_R_SUCCESS);
}

static isc_result_t
openssldh_fromdns(dst_key_t *key, isc_buffer_t *data) {
	DH *dh;
	isc_region_t r;
	isc_uint16_t plen, glen, publen;
	int special = 0;

	isc_buffer_remainingregion(data, &r);
	if (r.length == 0)
		return (ISC_R_SUCCESS);

	dh = DH_new();
	if (dh == NULL)
		return (dst__openssl_toresult(ISC_R_NOMEMORY));
	dh->flags &= ~DH_FLAG_CACHE_MONT_P;

	/*
	 * Read the prime length.  1 & 2 are table entries, > 16 means a
	 * prime follows, otherwise an error.
	 */
	if (r.length < 2) {
		DH_free(dh);
		return (DST_R_INVALIDPUBLICKEY);
	}
	plen = uint16_fromregion(&r);
	if (plen < 16 && plen != 1 && plen != 2) {
		DH_free(dh);
		return (DST_R_INVALIDPUBLICKEY);
	}
	if (r.length < plen) {
		DH_free(dh);
		return (DST_R_INVALIDPUBLICKEY);
	}
	if (plen == 1 || plen == 2) {
		if (plen == 1)
			special = *r.base++;
		else
			special = uint16_fromregion(&r);
		switch (special) {
			case 1:
				dh->p = &bn768;
				break;
			case 2:
				dh->p = &bn1024;
				break;
			case 3:
				dh->p = &bn1536;
				break;
			default:
				DH_free(dh);
				return (DST_R_INVALIDPUBLICKEY);
		}
	}
	else {
		dh->p = BN_bin2bn(r.base, plen, NULL);
		r.base += plen;
	}

	/*
	 * Read the generator length.  This should be 0 if the prime was
	 * special, but it might not be.  If it's 0 and the prime is not
	 * special, we have a problem.
	 */
	if (r.length < 2) {
		DH_free(dh);
		return (DST_R_INVALIDPUBLICKEY);
	}
	glen = uint16_fromregion(&r);
	if (r.length < glen) {
		DH_free(dh);
		return (DST_R_INVALIDPUBLICKEY);
	}
	if (special != 0) {
		if (glen == 0)
			dh->g = &bn2;
		else {
			dh->g = BN_bin2bn(r.base, glen, NULL);
			if (BN_cmp(dh->g, &bn2) == 0) {
				BN_free(dh->g);
				dh->g = &bn2;
			}
			else {
				DH_free(dh);
				return (DST_R_INVALIDPUBLICKEY);
			}
		}
	}
	else {
		if (glen == 0) {
			DH_free(dh);
			return (DST_R_INVALIDPUBLICKEY);
		}
		dh->g = BN_bin2bn(r.base, glen, NULL);
	}
	r.base += glen;

	if (r.length < 2) {
		DH_free(dh);
		return (DST_R_INVALIDPUBLICKEY);
	}
	publen = uint16_fromregion(&r);
	if (r.length < publen) {
		DH_free(dh);
		return (DST_R_INVALIDPUBLICKEY);
	}
	dh->pub_key = BN_bin2bn(r.base, publen, NULL);
	r.base += publen;

	key->key_size = BN_num_bits(dh->p);

	isc_buffer_forward(data, plen + glen + publen + 6);

	key->keydata.dh = dh;

	return (ISC_R_SUCCESS);
}

static isc_result_t
openssldh_tofile(const dst_key_t *key, const char *directory) {
	int i;
	DH *dh;
	dst_private_t priv;
	unsigned char *bufs[4];
	isc_result_t result;

	if (key->keydata.dh == NULL)
		return (DST_R_NULLKEY);

	dh = key->keydata.dh;

	for (i = 0; i < 4; i++) {
		bufs[i] = isc_mem_get(key->mctx, BN_num_bytes(dh->p));
		if (bufs[i] == NULL) {
			result = ISC_R_NOMEMORY;
			goto fail;
		}
	}

	i = 0;

	priv.elements[i].tag = TAG_DH_PRIME;
	priv.elements[i].length = BN_num_bytes(dh->p);
	BN_bn2bin(dh->p, bufs[i]);
	priv.elements[i].data = bufs[i];
	i++;

	priv.elements[i].tag = TAG_DH_GENERATOR;
	priv.elements[i].length = BN_num_bytes(dh->g);
	BN_bn2bin(dh->g, bufs[i]);
	priv.elements[i].data = bufs[i];
	i++;

	priv.elements[i].tag = TAG_DH_PRIVATE;
	priv.elements[i].length = BN_num_bytes(dh->priv_key);
	BN_bn2bin(dh->priv_key, bufs[i]);
	priv.elements[i].data = bufs[i];
	i++;

	priv.elements[i].tag = TAG_DH_PUBLIC;
	priv.elements[i].length = BN_num_bytes(dh->pub_key);
	BN_bn2bin(dh->pub_key, bufs[i]);
	priv.elements[i].data = bufs[i];
	i++;

	priv.nelements = i;
	result = dst__privstruct_writefile(key, &priv, directory);
 fail:
	for (i = 0; i < 4; i++) {
		if (bufs[i] == NULL)
			break;
		isc_mem_put(key->mctx, bufs[i], BN_num_bytes(dh->p));
	}
	return (result);
}

static isc_result_t
openssldh_parse(dst_key_t *key, isc_lex_t *lexer) {
	dst_private_t priv;
	isc_result_t ret;
	int i;
	DH *dh = NULL;
	isc_mem_t *mctx;
#define DST_RET(a) {ret = a; goto err;}

	mctx = key->mctx;

	/* read private key file */
	ret = dst__privstruct_parse(key, DST_ALG_DH, lexer, mctx, &priv);
	if (ret != ISC_R_SUCCESS)
		return (ret);

	dh = DH_new();
	if (dh == NULL)
		DST_RET(ISC_R_NOMEMORY);
	dh->flags &= ~DH_FLAG_CACHE_MONT_P;
	key->keydata.dh = dh;

	for (i = 0; i < priv.nelements; i++) {
		BIGNUM *bn;
		bn = BN_bin2bn(priv.elements[i].data,
			       priv.elements[i].length, NULL);
		if (bn == NULL)
			DST_RET(ISC_R_NOMEMORY);

		switch (priv.elements[i].tag) {
			case TAG_DH_PRIME:
				dh->p = bn;
				break;
			case TAG_DH_GENERATOR:
				dh->g = bn;
				break;
			case TAG_DH_PRIVATE:
				dh->priv_key = bn;
				break;
			case TAG_DH_PUBLIC:
				dh->pub_key = bn;
				break;
		}
	}
	dst__privstruct_free(&priv, mctx);

	key->key_size = BN_num_bits(dh->p);

	if ((key->key_size == 768 ||
	     key->key_size == 1024 ||
	     key->key_size == 1536) &&
	    BN_cmp(dh->g, &bn2) == 0)
	{
		if (key->key_size == 768 && BN_cmp(dh->p, &bn768) == 0) {
			BN_free(dh->p);
			BN_free(dh->g);
			dh->p = &bn768;
			dh->g = &bn2;
		} else if (key->key_size == 1024 &&
			   BN_cmp(dh->p, &bn1024) == 0) {
			BN_free(dh->p);
			BN_free(dh->g);
			dh->p = &bn1024;
			dh->g = &bn2;
		} else if (key->key_size == 1536 &&
			   BN_cmp(dh->p, &bn1536) == 0) {
			BN_free(dh->p);
			BN_free(dh->g);
			dh->p = &bn1536;
			dh->g = &bn2;
		}
	}

	return (ISC_R_SUCCESS);

 err:
	openssldh_destroy(key);
	dst__privstruct_free(&priv, mctx);
	memset(&priv, 0, sizeof(priv));
	return (ret);
}

static void
BN_fromhex(BIGNUM *b, const char *str) {
	static const char hexdigits[] = "0123456789abcdef";
	unsigned char data[512];
	unsigned int i;
	BIGNUM *out;

	RUNTIME_CHECK(strlen(str) < 1024U && strlen(str) % 2 == 0U);
	for (i = 0; i < strlen(str); i += 2) {
		char *s;
		unsigned int high, low;

		s = strchr(hexdigits, tolower((unsigned char)str[i]));
		RUNTIME_CHECK(s != NULL);
		high = s - hexdigits;

		s = strchr(hexdigits, tolower((unsigned char)str[i + 1]));
		RUNTIME_CHECK(s != NULL);
		low = s - hexdigits;

		data[i/2] = (unsigned char)((high << 4) + low);
	}
	out = BN_bin2bn(data, strlen(str)/2, b);
	RUNTIME_CHECK(out != NULL);
}

static void
openssldh_cleanup(void) {
	BN_free(&bn2);
	BN_free(&bn768);
	BN_free(&bn1024);
	BN_free(&bn1536);
}

static dst_func_t openssldh_functions = {
	NULL, /*%< createctx */
	NULL, /*%< destroyctx */
	NULL, /*%< adddata */
	NULL, /*%< openssldh_sign */
	NULL, /*%< openssldh_verify */
	openssldh_computesecret,
	openssldh_compare,
	openssldh_paramcompare,
	openssldh_generate,
	openssldh_isprivate,
	openssldh_destroy,
	openssldh_todns,
	openssldh_fromdns,
	openssldh_tofile,
	openssldh_parse,
	openssldh_cleanup,
	NULL, /*%< fromlabel */
};

isc_result_t
dst__openssldh_init(dst_func_t **funcp) {
	REQUIRE(funcp != NULL);
	if (*funcp == NULL) {
		BN_init(&bn2);
		BN_init(&bn768);
		BN_init(&bn1024);
		BN_init(&bn1536);
		BN_set_word(&bn2, 2);
		BN_fromhex(&bn768, PRIME768);
		BN_fromhex(&bn1024, PRIME1024);
		BN_fromhex(&bn1536, PRIME1536);
		*funcp = &openssldh_functions;
	}
	return (ISC_R_SUCCESS);
}

#else /* OPENSSL */

#include <isc/util.h>

EMPTY_TRANSLATION_UNIT

#endif /* OPENSSL */
/*! \file */
