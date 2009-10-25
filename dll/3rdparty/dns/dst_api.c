/*
 * Portions Copyright (C) 2004-2009  Internet Systems Consortium, Inc. ("ISC")
 * Portions Copyright (C) 1999-2003  Internet Software Consortium.
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
 * $Id: dst_api.c,v 1.16.12.3 2009/03/02 02:00:34 marka Exp $
 */

/*! \file */

#include <config.h>

#include <stdlib.h>

#include <isc/buffer.h>
#include <isc/dir.h>
#include <isc/entropy.h>
#include <isc/fsaccess.h>
#include <isc/hmacsha.h>
#include <isc/lex.h>
#include <isc/mem.h>
#include <isc/once.h>
#include <isc/print.h>
#include <isc/random.h>
#include <isc/string.h>
#include <isc/time.h>
#include <isc/util.h>

#include <dns/fixedname.h>
#include <dns/keyvalues.h>
#include <dns/name.h>
#include <dns/rdata.h>
#include <dns/rdataclass.h>
#include <dns/ttl.h>
#include <dns/types.h>

#include <dst/result.h>

#include "dst_internal.h"

#define DST_AS_STR(t) ((t).value.as_textregion.base)

static dst_func_t *dst_t_func[DST_MAX_ALGS];
static isc_entropy_t *dst_entropy_pool = NULL;
static unsigned int dst_entropy_flags = 0;
static isc_boolean_t dst_initialized = ISC_FALSE;

void gss_log(int level, const char *fmt, ...) ISC_FORMAT_PRINTF(2, 3);

isc_mem_t *dst__memory_pool = NULL;

/*
 * Static functions.
 */
static dst_key_t *	get_key_struct(dns_name_t *name,
				       unsigned int alg,
				       unsigned int flags,
				       unsigned int protocol,
				       unsigned int bits,
				       dns_rdataclass_t rdclass,
				       isc_mem_t *mctx);
static isc_result_t	write_public_key(const dst_key_t *key, int type,
					 const char *directory);
static isc_result_t	buildfilename(dns_name_t *name,
				      dns_keytag_t id,
				      unsigned int alg,
				      unsigned int type,
				      const char *directory,
				      isc_buffer_t *out);
static isc_result_t	computeid(dst_key_t *key);
static isc_result_t	frombuffer(dns_name_t *name,
				   unsigned int alg,
				   unsigned int flags,
				   unsigned int protocol,
				   dns_rdataclass_t rdclass,
				   isc_buffer_t *source,
				   isc_mem_t *mctx,
				   dst_key_t **keyp);

static isc_result_t	algorithm_status(unsigned int alg);

static isc_result_t	addsuffix(char *filename, unsigned int len,
				  const char *ofilename, const char *suffix);

#define RETERR(x) 				\
	do {					\
		result = (x);			\
		if (result != ISC_R_SUCCESS)	\
			goto out;		\
	} while (0)

#define CHECKALG(alg)				\
	do {					\
		isc_result_t _r;		\
		_r = algorithm_status(alg);	\
		if (_r != ISC_R_SUCCESS)	\
			return (_r);		\
	} while (0);				\

#ifdef OPENSSL
static void *
default_memalloc(void *arg, size_t size) {
	UNUSED(arg);
	if (size == 0U)
		size = 1;
	return (malloc(size));
}

static void
default_memfree(void *arg, void *ptr) {
	UNUSED(arg);
	free(ptr);
}
#endif

isc_result_t
dst_lib_init(isc_mem_t *mctx, isc_entropy_t *ectx, unsigned int eflags) {
	isc_result_t result;

	REQUIRE(mctx != NULL && ectx != NULL);
	REQUIRE(dst_initialized == ISC_FALSE);

	dst__memory_pool = NULL;

#ifdef OPENSSL
	UNUSED(mctx);
	/*
	 * When using --with-openssl, there seems to be no good way of not
	 * leaking memory due to the openssl error handling mechanism.
	 * Avoid assertions by using a local memory context and not checking
	 * for leaks on exit.  Note: as there are leaks we cannot use
	 * ISC_MEMFLAG_INTERNAL as it will free up memory still being used
	 * by libcrypto.
	 */
	result = isc_mem_createx2(0, 0, default_memalloc, default_memfree,
				  NULL, &dst__memory_pool, 0);
	if (result != ISC_R_SUCCESS)
		return (result);
	isc_mem_setname(dst__memory_pool, "dst", NULL);
	isc_mem_setdestroycheck(dst__memory_pool, ISC_FALSE);
#else
	isc_mem_attach(mctx, &dst__memory_pool);
#endif
	isc_entropy_attach(ectx, &dst_entropy_pool);
	dst_entropy_flags = eflags;

	dst_result_register();

	memset(dst_t_func, 0, sizeof(dst_t_func));
	RETERR(dst__hmacmd5_init(&dst_t_func[DST_ALG_HMACMD5]));
	RETERR(dst__hmacsha1_init(&dst_t_func[DST_ALG_HMACSHA1]));
	RETERR(dst__hmacsha224_init(&dst_t_func[DST_ALG_HMACSHA224]));
	RETERR(dst__hmacsha256_init(&dst_t_func[DST_ALG_HMACSHA256]));
	RETERR(dst__hmacsha384_init(&dst_t_func[DST_ALG_HMACSHA384]));
	RETERR(dst__hmacsha512_init(&dst_t_func[DST_ALG_HMACSHA512]));
#ifdef OPENSSL
	RETERR(dst__openssl_init());
	RETERR(dst__opensslrsa_init(&dst_t_func[DST_ALG_RSAMD5]));
	RETERR(dst__opensslrsa_init(&dst_t_func[DST_ALG_RSASHA1]));
	RETERR(dst__opensslrsa_init(&dst_t_func[DST_ALG_NSEC3RSASHA1]));
#ifdef HAVE_OPENSSL_DSA
	RETERR(dst__openssldsa_init(&dst_t_func[DST_ALG_DSA]));
	RETERR(dst__openssldsa_init(&dst_t_func[DST_ALG_NSEC3DSA]));
#endif
	RETERR(dst__openssldh_init(&dst_t_func[DST_ALG_DH]));
#endif /* OPENSSL */
#ifdef GSSAPI
	RETERR(dst__gssapi_init(&dst_t_func[DST_ALG_GSSAPI]));
#endif
	dst_initialized = ISC_TRUE;
	return (ISC_R_SUCCESS);

 out:
	dst_lib_destroy();
	return (result);
}

void
dst_lib_destroy(void) {
	int i;
	RUNTIME_CHECK(dst_initialized == ISC_TRUE);
	dst_initialized = ISC_FALSE;

	for (i = 0; i < DST_MAX_ALGS; i++)
		if (dst_t_func[i] != NULL && dst_t_func[i]->cleanup != NULL)
			dst_t_func[i]->cleanup();
#ifdef OPENSSL
	dst__openssl_destroy();
#endif
	if (dst__memory_pool != NULL)
		isc_mem_detach(&dst__memory_pool);
	if (dst_entropy_pool != NULL)
		isc_entropy_detach(&dst_entropy_pool);

}

isc_boolean_t
dst_algorithm_supported(unsigned int alg) {
	REQUIRE(dst_initialized == ISC_TRUE);

	if (alg >= DST_MAX_ALGS || dst_t_func[alg] == NULL)
		return (ISC_FALSE);
	return (ISC_TRUE);
}

isc_result_t
dst_context_create(dst_key_t *key, isc_mem_t *mctx, dst_context_t **dctxp) {
	dst_context_t *dctx;
	isc_result_t result;

	REQUIRE(dst_initialized == ISC_TRUE);
	REQUIRE(VALID_KEY(key));
	REQUIRE(mctx != NULL);
	REQUIRE(dctxp != NULL && *dctxp == NULL);

	if (key->func->createctx == NULL)
		return (DST_R_UNSUPPORTEDALG);
	if (key->keydata.generic == NULL)
		return (DST_R_NULLKEY);

	dctx = isc_mem_get(mctx, sizeof(dst_context_t));
	if (dctx == NULL)
		return (ISC_R_NOMEMORY);
	dctx->key = key;
	dctx->mctx = mctx;
	result = key->func->createctx(key, dctx);
	if (result != ISC_R_SUCCESS) {
		isc_mem_put(mctx, dctx, sizeof(dst_context_t));
		return (result);
	}
	dctx->magic = CTX_MAGIC;
	*dctxp = dctx;
	return (ISC_R_SUCCESS);
}

void
dst_context_destroy(dst_context_t **dctxp) {
	dst_context_t *dctx;

	REQUIRE(dctxp != NULL && VALID_CTX(*dctxp));

	dctx = *dctxp;
	INSIST(dctx->key->func->destroyctx != NULL);
	dctx->key->func->destroyctx(dctx);
	dctx->magic = 0;
	isc_mem_put(dctx->mctx, dctx, sizeof(dst_context_t));
	*dctxp = NULL;
}

isc_result_t
dst_context_adddata(dst_context_t *dctx, const isc_region_t *data) {
	REQUIRE(VALID_CTX(dctx));
	REQUIRE(data != NULL);
	INSIST(dctx->key->func->adddata != NULL);

	return (dctx->key->func->adddata(dctx, data));
}

isc_result_t
dst_context_sign(dst_context_t *dctx, isc_buffer_t *sig) {
	dst_key_t *key;

	REQUIRE(VALID_CTX(dctx));
	REQUIRE(sig != NULL);

	key = dctx->key;
	CHECKALG(key->key_alg);
	if (key->keydata.generic == NULL)
		return (DST_R_NULLKEY);

	if (key->func->sign == NULL)
		return (DST_R_NOTPRIVATEKEY);
	if (key->func->isprivate == NULL ||
	    key->func->isprivate(key) == ISC_FALSE)
		return (DST_R_NOTPRIVATEKEY);

	return (key->func->sign(dctx, sig));
}

isc_result_t
dst_context_verify(dst_context_t *dctx, isc_region_t *sig) {
	REQUIRE(VALID_CTX(dctx));
	REQUIRE(sig != NULL);

	CHECKALG(dctx->key->key_alg);
	if (dctx->key->keydata.generic == NULL)
		return (DST_R_NULLKEY);
	if (dctx->key->func->verify == NULL)
		return (DST_R_NOTPUBLICKEY);

	return (dctx->key->func->verify(dctx, sig));
}

isc_result_t
dst_key_computesecret(const dst_key_t *pub, const dst_key_t *priv,
		      isc_buffer_t *secret)
{
	REQUIRE(dst_initialized == ISC_TRUE);
	REQUIRE(VALID_KEY(pub) && VALID_KEY(priv));
	REQUIRE(secret != NULL);

	CHECKALG(pub->key_alg);
	CHECKALG(priv->key_alg);

	if (pub->keydata.generic == NULL || priv->keydata.generic == NULL)
		return (DST_R_NULLKEY);

	if (pub->key_alg != priv->key_alg ||
	    pub->func->computesecret == NULL ||
	    priv->func->computesecret == NULL)
		return (DST_R_KEYCANNOTCOMPUTESECRET);

	if (dst_key_isprivate(priv) == ISC_FALSE)
		return (DST_R_NOTPRIVATEKEY);

	return (pub->func->computesecret(pub, priv, secret));
}

isc_result_t
dst_key_tofile(const dst_key_t *key, int type, const char *directory) {
	isc_result_t ret = ISC_R_SUCCESS;

	REQUIRE(dst_initialized == ISC_TRUE);
	REQUIRE(VALID_KEY(key));
	REQUIRE((type & (DST_TYPE_PRIVATE | DST_TYPE_PUBLIC)) != 0);

	CHECKALG(key->key_alg);

	if (key->func->tofile == NULL)
		return (DST_R_UNSUPPORTEDALG);

	if (type & DST_TYPE_PUBLIC) {
		ret = write_public_key(key, type, directory);
		if (ret != ISC_R_SUCCESS)
			return (ret);
	}

	if ((type & DST_TYPE_PRIVATE) &&
	    (key->key_flags & DNS_KEYFLAG_TYPEMASK) != DNS_KEYTYPE_NOKEY)
		return (key->func->tofile(key, directory));
	else
		return (ISC_R_SUCCESS);
}

isc_result_t
dst_key_fromfile(dns_name_t *name, dns_keytag_t id,
		 unsigned int alg, int type, const char *directory,
		 isc_mem_t *mctx, dst_key_t **keyp)
{
	char filename[ISC_DIR_NAMEMAX];
	isc_buffer_t b;
	dst_key_t *key;
	isc_result_t result;

	REQUIRE(dst_initialized == ISC_TRUE);
	REQUIRE(dns_name_isabsolute(name));
	REQUIRE((type & (DST_TYPE_PRIVATE | DST_TYPE_PUBLIC)) != 0);
	REQUIRE(mctx != NULL);
	REQUIRE(keyp != NULL && *keyp == NULL);

	CHECKALG(alg);

	isc_buffer_init(&b, filename, sizeof(filename));
	result = buildfilename(name, id, alg, type, directory, &b);
	if (result != ISC_R_SUCCESS)
		return (result);

	key = NULL;
	result = dst_key_fromnamedfile(filename, type, mctx, &key);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = computeid(key);
	if (result != ISC_R_SUCCESS) {
		dst_key_free(&key);
		return (result);
	}

	if (!dns_name_equal(name, key->key_name) || id != key->key_id ||
	    alg != key->key_alg) {
		dst_key_free(&key);
		return (DST_R_INVALIDPRIVATEKEY);
	}
	key->key_id = id;

	*keyp = key;
	return (ISC_R_SUCCESS);
}

isc_result_t
dst_key_fromnamedfile(const char *filename, int type, isc_mem_t *mctx,
		      dst_key_t **keyp)
{
	isc_result_t result;
	dst_key_t *pubkey = NULL, *key = NULL;
	dns_keytag_t id;
	char *newfilename = NULL;
	int newfilenamelen = 0;
	isc_lex_t *lex = NULL;

	REQUIRE(dst_initialized == ISC_TRUE);
	REQUIRE(filename != NULL);
	REQUIRE((type & (DST_TYPE_PRIVATE | DST_TYPE_PUBLIC)) != 0);
	REQUIRE(mctx != NULL);
	REQUIRE(keyp != NULL && *keyp == NULL);

	newfilenamelen = strlen(filename) + 5;
	newfilename = isc_mem_get(mctx, newfilenamelen);
	if (newfilename == NULL)
		return (ISC_R_NOMEMORY);
	result = addsuffix(newfilename, newfilenamelen, filename, ".key");
	INSIST(result == ISC_R_SUCCESS);

	result = dst_key_read_public(newfilename, type, mctx, &pubkey);
	isc_mem_put(mctx, newfilename, newfilenamelen);
	newfilename = NULL;
	if (result != ISC_R_SUCCESS)
		return (result);

	if ((type & (DST_TYPE_PRIVATE | DST_TYPE_PUBLIC)) == DST_TYPE_PUBLIC ||
	    (pubkey->key_flags & DNS_KEYFLAG_TYPEMASK) == DNS_KEYTYPE_NOKEY) {
		result = computeid(pubkey);
		if (result != ISC_R_SUCCESS) {
			dst_key_free(&pubkey);
			return (result);
		}

		*keyp = pubkey;
		return (ISC_R_SUCCESS);
	}

	result = algorithm_status(pubkey->key_alg);
	if (result != ISC_R_SUCCESS) {
		dst_key_free(&pubkey);
		return (result);
	}

	key = get_key_struct(pubkey->key_name, pubkey->key_alg,
			     pubkey->key_flags, pubkey->key_proto, 0,
			     pubkey->key_class, mctx);
	id = pubkey->key_id;
	dst_key_free(&pubkey);

	if (key == NULL)
		return (ISC_R_NOMEMORY);

	if (key->func->parse == NULL)
		RETERR(DST_R_UNSUPPORTEDALG);

	newfilenamelen = strlen(filename) + 9;
	newfilename = isc_mem_get(mctx, newfilenamelen);
	if (newfilename == NULL)
		RETERR(ISC_R_NOMEMORY);
	result = addsuffix(newfilename, newfilenamelen, filename, ".private");
	INSIST(result == ISC_R_SUCCESS);

	RETERR(isc_lex_create(mctx, 1500, &lex));
	RETERR(isc_lex_openfile(lex, newfilename));
	isc_mem_put(mctx, newfilename, newfilenamelen);

	RETERR(key->func->parse(key, lex));
	isc_lex_destroy(&lex);

	RETERR(computeid(key));

	if (id != key->key_id)
		RETERR(DST_R_INVALIDPRIVATEKEY);

	*keyp = key;
	return (ISC_R_SUCCESS);
 out:
	if (newfilename != NULL)
		isc_mem_put(mctx, newfilename, newfilenamelen);
	if (lex != NULL)
		isc_lex_destroy(&lex);
	dst_key_free(&key);
	return (result);
}

isc_result_t
dst_key_todns(const dst_key_t *key, isc_buffer_t *target) {
	REQUIRE(dst_initialized == ISC_TRUE);
	REQUIRE(VALID_KEY(key));
	REQUIRE(target != NULL);

	CHECKALG(key->key_alg);

	if (key->func->todns == NULL)
		return (DST_R_UNSUPPORTEDALG);

	if (isc_buffer_availablelength(target) < 4)
		return (ISC_R_NOSPACE);
	isc_buffer_putuint16(target, (isc_uint16_t)(key->key_flags & 0xffff));
	isc_buffer_putuint8(target, (isc_uint8_t)key->key_proto);
	isc_buffer_putuint8(target, (isc_uint8_t)key->key_alg);

	if (key->key_flags & DNS_KEYFLAG_EXTENDED) {
		if (isc_buffer_availablelength(target) < 2)
			return (ISC_R_NOSPACE);
		isc_buffer_putuint16(target,
				     (isc_uint16_t)((key->key_flags >> 16)
						    & 0xffff));
	}

	if (key->keydata.generic == NULL) /*%< NULL KEY */
		return (ISC_R_SUCCESS);

	return (key->func->todns(key, target));
}

isc_result_t
dst_key_fromdns(dns_name_t *name, dns_rdataclass_t rdclass,
		isc_buffer_t *source, isc_mem_t *mctx, dst_key_t **keyp)
{
	isc_uint8_t alg, proto;
	isc_uint32_t flags, extflags;
	dst_key_t *key = NULL;
	dns_keytag_t id;
	isc_region_t r;
	isc_result_t result;

	REQUIRE(dst_initialized);

	isc_buffer_remainingregion(source, &r);

	if (isc_buffer_remaininglength(source) < 4)
		return (DST_R_INVALIDPUBLICKEY);
	flags = isc_buffer_getuint16(source);
	proto = isc_buffer_getuint8(source);
	alg = isc_buffer_getuint8(source);

	id = dst_region_computeid(&r, alg);

	if (flags & DNS_KEYFLAG_EXTENDED) {
		if (isc_buffer_remaininglength(source) < 2)
			return (DST_R_INVALIDPUBLICKEY);
		extflags = isc_buffer_getuint16(source);
		flags |= (extflags << 16);
	}

	result = frombuffer(name, alg, flags, proto, rdclass, source,
			    mctx, &key);
	if (result != ISC_R_SUCCESS)
		return (result);
	key->key_id = id;

	*keyp = key;
	return (ISC_R_SUCCESS);
}

isc_result_t
dst_key_frombuffer(dns_name_t *name, unsigned int alg,
		   unsigned int flags, unsigned int protocol,
		   dns_rdataclass_t rdclass,
		   isc_buffer_t *source, isc_mem_t *mctx, dst_key_t **keyp)
{
	dst_key_t *key = NULL;
	isc_result_t result;

	REQUIRE(dst_initialized);

	result = frombuffer(name, alg, flags, protocol, rdclass, source,
			    mctx, &key);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = computeid(key);
	if (result != ISC_R_SUCCESS) {
		dst_key_free(&key);
		return (result);
	}

	*keyp = key;
	return (ISC_R_SUCCESS);
}

isc_result_t
dst_key_tobuffer(const dst_key_t *key, isc_buffer_t *target) {
	REQUIRE(dst_initialized == ISC_TRUE);
	REQUIRE(VALID_KEY(key));
	REQUIRE(target != NULL);

	CHECKALG(key->key_alg);

	if (key->func->todns == NULL)
		return (DST_R_UNSUPPORTEDALG);

	return (key->func->todns(key, target));
}

isc_result_t
dst_key_privatefrombuffer(dst_key_t *key, isc_buffer_t *buffer) {
	isc_lex_t *lex = NULL;
	isc_result_t result = ISC_R_SUCCESS;

	REQUIRE(dst_initialized == ISC_TRUE);
	REQUIRE(VALID_KEY(key));
	REQUIRE(!dst_key_isprivate(key));
	REQUIRE(buffer != NULL);

	if (key->func->parse == NULL)
		RETERR(DST_R_UNSUPPORTEDALG);

	RETERR(isc_lex_create(key->mctx, 1500, &lex));
	RETERR(isc_lex_openbuffer(lex, buffer));
	RETERR(key->func->parse(key, lex));
 out:
	if (lex != NULL)
		isc_lex_destroy(&lex);
	return (result);
}

gss_ctx_id_t
dst_key_getgssctx(const dst_key_t *key)
{
	REQUIRE(key != NULL);

	return (key->keydata.gssctx);
}

isc_result_t
dst_key_fromgssapi(dns_name_t *name, gss_ctx_id_t gssctx, isc_mem_t *mctx,
		   dst_key_t **keyp)
{
	dst_key_t *key;

	REQUIRE(gssctx != NULL);
	REQUIRE(keyp != NULL && *keyp == NULL);

	key = get_key_struct(name, DST_ALG_GSSAPI, 0, DNS_KEYPROTO_DNSSEC,
			     0, dns_rdataclass_in, mctx);
	if (key == NULL)
		return (ISC_R_NOMEMORY);

	key->keydata.gssctx = gssctx;
	*keyp = key;
	return (ISC_R_SUCCESS);
}

isc_result_t
dst_key_fromlabel(dns_name_t *name, int alg, unsigned int flags,
		  unsigned int protocol, dns_rdataclass_t rdclass,
		  const char *engine, const char *label, const char *pin,
		  isc_mem_t *mctx, dst_key_t **keyp)
{
	dst_key_t *key;
	isc_result_t result;

	REQUIRE(dst_initialized == ISC_TRUE);
	REQUIRE(dns_name_isabsolute(name));
	REQUIRE(mctx != NULL);
	REQUIRE(keyp != NULL && *keyp == NULL);
	REQUIRE(label != NULL);

	CHECKALG(alg);

	key = get_key_struct(name, alg, flags, protocol, 0, rdclass, mctx);
	if (key == NULL)
		return (ISC_R_NOMEMORY);

	if (key->func->fromlabel == NULL) {
		dst_key_free(&key);
		return (DST_R_UNSUPPORTEDALG);
	}

	result = key->func->fromlabel(key, engine, label, pin);
	if (result != ISC_R_SUCCESS) {
		dst_key_free(&key);
		return (result);
	}

	result = computeid(key);
	if (result != ISC_R_SUCCESS) {
		dst_key_free(&key);
		return (result);
	}

	*keyp = key;
	return (ISC_R_SUCCESS);
}

isc_result_t
dst_key_generate(dns_name_t *name, unsigned int alg,
		 unsigned int bits, unsigned int param,
		 unsigned int flags, unsigned int protocol,
		 dns_rdataclass_t rdclass,
		 isc_mem_t *mctx, dst_key_t **keyp)
{
	dst_key_t *key;
	isc_result_t ret;

	REQUIRE(dst_initialized == ISC_TRUE);
	REQUIRE(dns_name_isabsolute(name));
	REQUIRE(mctx != NULL);
	REQUIRE(keyp != NULL && *keyp == NULL);

	CHECKALG(alg);

	key = get_key_struct(name, alg, flags, protocol, bits, rdclass, mctx);
	if (key == NULL)
		return (ISC_R_NOMEMORY);

	if (bits == 0) { /*%< NULL KEY */
		key->key_flags |= DNS_KEYTYPE_NOKEY;
		*keyp = key;
		return (ISC_R_SUCCESS);
	}

	if (key->func->generate == NULL) {
		dst_key_free(&key);
		return (DST_R_UNSUPPORTEDALG);
	}

	ret = key->func->generate(key, param);
	if (ret != ISC_R_SUCCESS) {
		dst_key_free(&key);
		return (ret);
	}

	ret = computeid(key);
	if (ret != ISC_R_SUCCESS) {
		dst_key_free(&key);
		return (ret);
	}

	*keyp = key;
	return (ISC_R_SUCCESS);
}

isc_boolean_t
dst_key_compare(const dst_key_t *key1, const dst_key_t *key2) {
	REQUIRE(dst_initialized == ISC_TRUE);
	REQUIRE(VALID_KEY(key1));
	REQUIRE(VALID_KEY(key2));

	if (key1 == key2)
		return (ISC_TRUE);
	if (key1 == NULL || key2 == NULL)
		return (ISC_FALSE);
	if (key1->key_alg == key2->key_alg &&
	    key1->key_id == key2->key_id &&
	    key1->func->compare != NULL &&
	    key1->func->compare(key1, key2) == ISC_TRUE)
		return (ISC_TRUE);
	else
		return (ISC_FALSE);
}

isc_boolean_t
dst_key_paramcompare(const dst_key_t *key1, const dst_key_t *key2) {
	REQUIRE(dst_initialized == ISC_TRUE);
	REQUIRE(VALID_KEY(key1));
	REQUIRE(VALID_KEY(key2));

	if (key1 == key2)
		return (ISC_TRUE);
	if (key1 == NULL || key2 == NULL)
		return (ISC_FALSE);
	if (key1->key_alg == key2->key_alg &&
	    key1->func->paramcompare != NULL &&
	    key1->func->paramcompare(key1, key2) == ISC_TRUE)
		return (ISC_TRUE);
	else
		return (ISC_FALSE);
}

void
dst_key_free(dst_key_t **keyp) {
	isc_mem_t *mctx;
	dst_key_t *key;

	REQUIRE(dst_initialized == ISC_TRUE);
	REQUIRE(keyp != NULL && VALID_KEY(*keyp));

	key = *keyp;
	mctx = key->mctx;

	if (key->keydata.generic != NULL) {
		INSIST(key->func->destroy != NULL);
		key->func->destroy(key);
	}
	if (key->engine != NULL)
		isc_mem_free(mctx, key->engine);
	if (key->label != NULL)
		isc_mem_free(mctx, key->label);
	dns_name_free(key->key_name, mctx);
	isc_mem_put(mctx, key->key_name, sizeof(dns_name_t));
	memset(key, 0, sizeof(dst_key_t));
	isc_mem_put(mctx, key, sizeof(dst_key_t));
	*keyp = NULL;
}

isc_boolean_t
dst_key_isprivate(const dst_key_t *key) {
	REQUIRE(VALID_KEY(key));
	INSIST(key->func->isprivate != NULL);
	return (key->func->isprivate(key));
}

isc_result_t
dst_key_buildfilename(const dst_key_t *key, int type,
		      const char *directory, isc_buffer_t *out) {

	REQUIRE(VALID_KEY(key));
	REQUIRE(type == DST_TYPE_PRIVATE || type == DST_TYPE_PUBLIC ||
		type == 0);

	return (buildfilename(key->key_name, key->key_id, key->key_alg,
			      type, directory, out));
}

isc_result_t
dst_key_sigsize(const dst_key_t *key, unsigned int *n) {
	REQUIRE(dst_initialized == ISC_TRUE);
	REQUIRE(VALID_KEY(key));
	REQUIRE(n != NULL);

	/* XXXVIX this switch statement is too sparse to gen a jump table. */
	switch (key->key_alg) {
	case DST_ALG_RSAMD5:
	case DST_ALG_RSASHA1:
	case DST_ALG_NSEC3RSASHA1:
		*n = (key->key_size + 7) / 8;
		break;
	case DST_ALG_DSA:
	case DST_ALG_NSEC3DSA:
		*n = DNS_SIG_DSASIGSIZE;
		break;
	case DST_ALG_HMACMD5:
		*n = 16;
		break;
	case DST_ALG_HMACSHA1:
		*n = ISC_SHA1_DIGESTLENGTH;
		break;
	case DST_ALG_HMACSHA224:
		*n = ISC_SHA224_DIGESTLENGTH;
		break;
	case DST_ALG_HMACSHA256:
		*n = ISC_SHA256_DIGESTLENGTH;
		break;
	case DST_ALG_HMACSHA384:
		*n = ISC_SHA384_DIGESTLENGTH;
		break;
	case DST_ALG_HMACSHA512:
		*n = ISC_SHA512_DIGESTLENGTH;
		break;
	case DST_ALG_GSSAPI:
		*n = 128; /*%< XXX */
		break;
	case DST_ALG_DH:
	default:
		return (DST_R_UNSUPPORTEDALG);
	}
	return (ISC_R_SUCCESS);
}

isc_result_t
dst_key_secretsize(const dst_key_t *key, unsigned int *n) {
	REQUIRE(dst_initialized == ISC_TRUE);
	REQUIRE(VALID_KEY(key));
	REQUIRE(n != NULL);

	if (key->key_alg == DST_ALG_DH)
		*n = (key->key_size + 7) / 8;
	else
		return (DST_R_UNSUPPORTEDALG);
	return (ISC_R_SUCCESS);
}

/***
 *** Static methods
 ***/

/*%
 * Allocates a key structure and fills in some of the fields.
 */
static dst_key_t *
get_key_struct(dns_name_t *name, unsigned int alg,
	       unsigned int flags, unsigned int protocol,
	       unsigned int bits, dns_rdataclass_t rdclass,
	       isc_mem_t *mctx)
{
	dst_key_t *key;
	isc_result_t result;

	key = (dst_key_t *) isc_mem_get(mctx, sizeof(dst_key_t));
	if (key == NULL)
		return (NULL);

	memset(key, 0, sizeof(dst_key_t));
	key->magic = KEY_MAGIC;

	key->key_name = isc_mem_get(mctx, sizeof(dns_name_t));
	if (key->key_name == NULL) {
		isc_mem_put(mctx, key, sizeof(dst_key_t));
		return (NULL);
	}
	dns_name_init(key->key_name, NULL);
	result = dns_name_dup(name, mctx, key->key_name);
	if (result != ISC_R_SUCCESS) {
		isc_mem_put(mctx, key->key_name, sizeof(dns_name_t));
		isc_mem_put(mctx, key, sizeof(dst_key_t));
		return (NULL);
	}
	key->key_alg = alg;
	key->key_flags = flags;
	key->key_proto = protocol;
	key->mctx = mctx;
	key->keydata.generic = NULL;
	key->key_size = bits;
	key->key_class = rdclass;
	key->func = dst_t_func[alg];
	return (key);
}

/*%
 * Reads a public key from disk
 */
isc_result_t
dst_key_read_public(const char *filename, int type,
		    isc_mem_t *mctx, dst_key_t **keyp)
{
	u_char rdatabuf[DST_KEY_MAXSIZE];
	isc_buffer_t b;
	dns_fixedname_t name;
	isc_lex_t *lex = NULL;
	isc_token_t token;
	isc_result_t ret;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	unsigned int opt = ISC_LEXOPT_DNSMULTILINE;
	dns_rdataclass_t rdclass = dns_rdataclass_in;
	isc_lexspecials_t specials;
	isc_uint32_t ttl;
	isc_result_t result;
	dns_rdatatype_t keytype;

	/*
	 * Open the file and read its formatted contents
	 * File format:
	 *    domain.name [ttl] [class] [KEY|DNSKEY] <flags> <protocol> <algorithm> <key>
	 */

	/* 1500 should be large enough for any key */
	ret = isc_lex_create(mctx, 1500, &lex);
	if (ret != ISC_R_SUCCESS)
		goto cleanup;

	memset(specials, 0, sizeof(specials));
	specials['('] = 1;
	specials[')'] = 1;
	specials['"'] = 1;
	isc_lex_setspecials(lex, specials);
	isc_lex_setcomments(lex, ISC_LEXCOMMENT_DNSMASTERFILE);

	ret = isc_lex_openfile(lex, filename);
	if (ret != ISC_R_SUCCESS)
		goto cleanup;

#define NEXTTOKEN(lex, opt, token) { \
	ret = isc_lex_gettoken(lex, opt, token); \
	if (ret != ISC_R_SUCCESS) \
		goto cleanup; \
	}

#define BADTOKEN() { \
	ret = ISC_R_UNEXPECTEDTOKEN; \
	goto cleanup; \
	}

	/* Read the domain name */
	NEXTTOKEN(lex, opt, &token);
	if (token.type != isc_tokentype_string)
		BADTOKEN();

	/*
	 * We don't support "@" in .key files.
	 */
	if (!strcmp(DST_AS_STR(token), "@"))
		BADTOKEN();

	dns_fixedname_init(&name);
	isc_buffer_init(&b, DST_AS_STR(token), strlen(DST_AS_STR(token)));
	isc_buffer_add(&b, strlen(DST_AS_STR(token)));
	ret = dns_name_fromtext(dns_fixedname_name(&name), &b, dns_rootname,
				ISC_FALSE, NULL);
	if (ret != ISC_R_SUCCESS)
		goto cleanup;

	/* Read the next word: either TTL, class, or 'KEY' */
	NEXTTOKEN(lex, opt, &token);

	/* If it's a TTL, read the next one */
	result = dns_ttl_fromtext(&token.value.as_textregion, &ttl);
	if (result == ISC_R_SUCCESS)
		NEXTTOKEN(lex, opt, &token);

	if (token.type != isc_tokentype_string)
		BADTOKEN();

	ret = dns_rdataclass_fromtext(&rdclass, &token.value.as_textregion);
	if (ret == ISC_R_SUCCESS)
		NEXTTOKEN(lex, opt, &token);

	if (token.type != isc_tokentype_string)
		BADTOKEN();

	if (strcasecmp(DST_AS_STR(token), "DNSKEY") == 0)
		keytype = dns_rdatatype_dnskey;
	else if (strcasecmp(DST_AS_STR(token), "KEY") == 0)
		keytype = dns_rdatatype_key; /*%< SIG(0), TKEY */
	else
		BADTOKEN();

	if (((type & DST_TYPE_KEY) != 0 && keytype != dns_rdatatype_key) ||
	    ((type & DST_TYPE_KEY) == 0 && keytype != dns_rdatatype_dnskey)) {
		ret = DST_R_BADKEYTYPE;
		goto cleanup;
	}

	isc_buffer_init(&b, rdatabuf, sizeof(rdatabuf));
	ret = dns_rdata_fromtext(&rdata, rdclass, keytype, lex, NULL,
				 ISC_FALSE, mctx, &b, NULL);
	if (ret != ISC_R_SUCCESS)
		goto cleanup;

	ret = dst_key_fromdns(dns_fixedname_name(&name), rdclass, &b, mctx,
			      keyp);
	if (ret != ISC_R_SUCCESS)
		goto cleanup;

 cleanup:
	if (lex != NULL)
		isc_lex_destroy(&lex);
	return (ret);
}

static isc_boolean_t
issymmetric(const dst_key_t *key) {
	REQUIRE(dst_initialized == ISC_TRUE);
	REQUIRE(VALID_KEY(key));

	/* XXXVIX this switch statement is too sparse to gen a jump table. */
	switch (key->key_alg) {
	case DST_ALG_RSAMD5:
	case DST_ALG_RSASHA1:
	case DST_ALG_NSEC3RSASHA1:
	case DST_ALG_DSA:
	case DST_ALG_NSEC3DSA:
	case DST_ALG_DH:
		return (ISC_FALSE);
	case DST_ALG_HMACMD5:
	case DST_ALG_GSSAPI:
		return (ISC_TRUE);
	default:
		return (ISC_FALSE);
	}
}

/*%
 * Writes a public key to disk in DNS format.
 */
static isc_result_t
write_public_key(const dst_key_t *key, int type, const char *directory) {
	FILE *fp;
	isc_buffer_t keyb, textb, fileb, classb;
	isc_region_t r;
	char filename[ISC_DIR_NAMEMAX];
	unsigned char key_array[DST_KEY_MAXSIZE];
	char text_array[DST_KEY_MAXTEXTSIZE];
	char class_array[10];
	isc_result_t ret;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_fsaccess_t access;

	REQUIRE(VALID_KEY(key));

	isc_buffer_init(&keyb, key_array, sizeof(key_array));
	isc_buffer_init(&textb, text_array, sizeof(text_array));
	isc_buffer_init(&classb, class_array, sizeof(class_array));

	ret = dst_key_todns(key, &keyb);
	if (ret != ISC_R_SUCCESS)
		return (ret);

	isc_buffer_usedregion(&keyb, &r);
	dns_rdata_fromregion(&rdata, key->key_class, dns_rdatatype_dnskey, &r);

	ret = dns_rdata_totext(&rdata, (dns_name_t *) NULL, &textb);
	if (ret != ISC_R_SUCCESS)
		return (DST_R_INVALIDPUBLICKEY);

	ret = dns_rdataclass_totext(key->key_class, &classb);
	if (ret != ISC_R_SUCCESS)
		return (DST_R_INVALIDPUBLICKEY);

	/*
	 * Make the filename.
	 */
	isc_buffer_init(&fileb, filename, sizeof(filename));
	ret = dst_key_buildfilename(key, DST_TYPE_PUBLIC, directory, &fileb);
	if (ret != ISC_R_SUCCESS)
		return (ret);

	/*
	 * Create public key file.
	 */
	if ((fp = fopen(filename, "w")) == NULL)
		return (DST_R_WRITEERROR);

	if (issymmetric(key)) {
		access = 0;
		isc_fsaccess_add(ISC_FSACCESS_OWNER,
				 ISC_FSACCESS_READ | ISC_FSACCESS_WRITE,
				 &access);
		(void)isc_fsaccess_set(filename, access);
	}

	ret = dns_name_print(key->key_name, fp);
	if (ret != ISC_R_SUCCESS) {
		fclose(fp);
		return (ret);
	}

	fprintf(fp, " ");

	isc_buffer_usedregion(&classb, &r);
	fwrite(r.base, 1, r.length, fp);

	if ((type & DST_TYPE_KEY) != 0)
		fprintf(fp, " KEY ");
	else
		fprintf(fp, " DNSKEY ");

	isc_buffer_usedregion(&textb, &r);
	fwrite(r.base, 1, r.length, fp);

	fputc('\n', fp);
	fflush(fp);
	if (ferror(fp))
		ret = DST_R_WRITEERROR;
	fclose(fp);

	return (ret);
}

static isc_result_t
buildfilename(dns_name_t *name, dns_keytag_t id,
	      unsigned int alg, unsigned int type,
	      const char *directory, isc_buffer_t *out)
{
	const char *suffix = "";
	unsigned int len;
	isc_result_t result;

	REQUIRE(out != NULL);
	if ((type & DST_TYPE_PRIVATE) != 0)
		suffix = ".private";
	else if (type == DST_TYPE_PUBLIC)
		suffix = ".key";
	if (directory != NULL) {
		if (isc_buffer_availablelength(out) < strlen(directory))
			return (ISC_R_NOSPACE);
		isc_buffer_putstr(out, directory);
		if (strlen(directory) > 0U &&
		    directory[strlen(directory) - 1] != '/')
			isc_buffer_putstr(out, "/");
	}
	if (isc_buffer_availablelength(out) < 1)
		return (ISC_R_NOSPACE);
	isc_buffer_putstr(out, "K");
	result = dns_name_tofilenametext(name, ISC_FALSE, out);
	if (result != ISC_R_SUCCESS)
		return (result);
	len = 1 + 3 + 1 + 5 + strlen(suffix) + 1;
	if (isc_buffer_availablelength(out) < len)
		return (ISC_R_NOSPACE);
	sprintf((char *) isc_buffer_used(out), "+%03d+%05d%s", alg, id,
		suffix);
	isc_buffer_add(out, len);

	return (ISC_R_SUCCESS);
}

static isc_result_t
computeid(dst_key_t *key) {
	isc_buffer_t dnsbuf;
	unsigned char dns_array[DST_KEY_MAXSIZE];
	isc_region_t r;
	isc_result_t ret;

	isc_buffer_init(&dnsbuf, dns_array, sizeof(dns_array));
	ret = dst_key_todns(key, &dnsbuf);
	if (ret != ISC_R_SUCCESS)
		return (ret);

	isc_buffer_usedregion(&dnsbuf, &r);
	key->key_id = dst_region_computeid(&r, key->key_alg);
	return (ISC_R_SUCCESS);
}

static isc_result_t
frombuffer(dns_name_t *name, unsigned int alg, unsigned int flags,
	   unsigned int protocol, dns_rdataclass_t rdclass,
	   isc_buffer_t *source, isc_mem_t *mctx, dst_key_t **keyp)
{
	dst_key_t *key;
	isc_result_t ret;

	REQUIRE(dns_name_isabsolute(name));
	REQUIRE(source != NULL);
	REQUIRE(mctx != NULL);
	REQUIRE(keyp != NULL && *keyp == NULL);

	key = get_key_struct(name, alg, flags, protocol, 0, rdclass, mctx);
	if (key == NULL)
		return (ISC_R_NOMEMORY);

	if (isc_buffer_remaininglength(source) > 0) {
		ret = algorithm_status(alg);
		if (ret != ISC_R_SUCCESS) {
			dst_key_free(&key);
			return (ret);
		}
		if (key->func->fromdns == NULL) {
			dst_key_free(&key);
			return (DST_R_UNSUPPORTEDALG);
		}

		ret = key->func->fromdns(key, source);
		if (ret != ISC_R_SUCCESS) {
			dst_key_free(&key);
			return (ret);
		}
	}

	*keyp = key;
	return (ISC_R_SUCCESS);
}

static isc_result_t
algorithm_status(unsigned int alg) {
	REQUIRE(dst_initialized == ISC_TRUE);

	if (dst_algorithm_supported(alg))
		return (ISC_R_SUCCESS);
#ifndef OPENSSL
	if (alg == DST_ALG_RSAMD5 || alg == DST_ALG_RSASHA1 ||
	    alg == DST_ALG_DSA || alg == DST_ALG_DH ||
	    alg == DST_ALG_HMACMD5 || alg == DST_ALG_NSEC3DSA ||
	    alg == DST_ALG_NSEC3RSASHA1)
		return (DST_R_NOCRYPTO);
#endif
	return (DST_R_UNSUPPORTEDALG);
}

static isc_result_t
addsuffix(char *filename, unsigned int len, const char *ofilename,
	  const char *suffix)
{
	int olen = strlen(ofilename);
	int n;

	if (olen > 1 && ofilename[olen - 1] == '.')
		olen -= 1;
	else if (olen > 8 && strcmp(ofilename + olen - 8, ".private") == 0)
		olen -= 8;
	else if (olen > 4 && strcmp(ofilename + olen - 4, ".key") == 0)
		olen -= 4;

	n = snprintf(filename, len, "%.*s%s", olen, ofilename, suffix);
	if (n < 0)
		return (ISC_R_NOSPACE);
	return (ISC_R_SUCCESS);
}

isc_result_t
dst__entropy_getdata(void *buf, unsigned int len, isc_boolean_t pseudo) {
	unsigned int flags = dst_entropy_flags;
	if (pseudo)
		flags &= ~ISC_ENTROPY_GOODONLY;
	return (isc_entropy_getdata(dst_entropy_pool, buf, len, NULL, flags));
}

unsigned int
dst__entropy_status(void) {
	return (isc_entropy_status(dst_entropy_pool));
}
