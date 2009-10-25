/*
 * Portions Copyright (C) 2004-2008  Internet Systems Consortium, Inc. ("ISC")
 * Portions Copyright (C) 2000-2002  Internet Software Consortium.
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

/* $Id: dst_internal.h,v 1.11 2008/04/01 23:47:10 tbox Exp $ */

#ifndef DST_DST_INTERNAL_H
#define DST_DST_INTERNAL_H 1

#include <isc/lang.h>
#include <isc/buffer.h>
#include <isc/int.h>
#include <isc/magic.h>
#include <isc/region.h>
#include <isc/types.h>
#include <isc/md5.h>
#include <isc/sha1.h>
#include <isc/hmacmd5.h>
#include <isc/hmacsha.h>

#include <dst/dst.h>

#ifdef OPENSSL
#include <openssl/dh.h>
#include <openssl/dsa.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/rsa.h>
#endif

ISC_LANG_BEGINDECLS

#define KEY_MAGIC       ISC_MAGIC('D','S','T','K')
#define CTX_MAGIC       ISC_MAGIC('D','S','T','C')

#define VALID_KEY(x) ISC_MAGIC_VALID(x, KEY_MAGIC)
#define VALID_CTX(x) ISC_MAGIC_VALID(x, CTX_MAGIC)

extern isc_mem_t *dst__memory_pool;

/***
 *** Types
 ***/

typedef struct dst_func dst_func_t;

typedef struct dst_hmacmd5_key    dst_hmacmd5_key_t;
typedef struct dst_hmacsha1_key   dst_hmacsha1_key_t;
typedef struct dst_hmacsha224_key dst_hmacsha224_key_t;
typedef struct dst_hmacsha256_key dst_hmacsha256_key_t;
typedef struct dst_hmacsha384_key dst_hmacsha384_key_t;
typedef struct dst_hmacsha512_key dst_hmacsha512_key_t;

/*% DST Key Structure */
struct dst_key {
	unsigned int	magic;
	dns_name_t *	key_name;	/*%< name of the key */
	unsigned int	key_size;	/*%< size of the key in bits */
	unsigned int	key_proto;	/*%< protocols this key is used for */
	unsigned int	key_alg;	/*%< algorithm of the key */
	isc_uint32_t	key_flags;	/*%< flags of the public key */
	isc_uint16_t	key_id;		/*%< identifier of the key */
	isc_uint16_t	key_bits;	/*%< hmac digest bits */
	dns_rdataclass_t key_class;	/*%< class of the key record */
	isc_mem_t	*mctx;		/*%< memory context */
	char		*engine;	/*%< engine name (HSM) */
	char		*label;		/*%< engine label (HSM) */
	union {
		void *generic;
		gss_ctx_id_t gssctx;
#ifdef OPENSSL
#if USE_EVP_RSA
		RSA *rsa;
#endif
		DSA *dsa;
		DH *dh;
		EVP_PKEY *pkey;
#endif
		dst_hmacmd5_key_t *hmacmd5;
		dst_hmacsha1_key_t *hmacsha1;
		dst_hmacsha224_key_t *hmacsha224;
		dst_hmacsha256_key_t *hmacsha256;
		dst_hmacsha384_key_t *hmacsha384;
		dst_hmacsha512_key_t *hmacsha512;

	} keydata;			/*%< pointer to key in crypto pkg fmt */
	dst_func_t *	func;		/*%< crypto package specific functions */
};

struct dst_context {
	unsigned int magic;
	dst_key_t *key;
	isc_mem_t *mctx;
	union {
		void *generic;
		dst_gssapi_signverifyctx_t *gssctx;
		isc_md5_t *md5ctx;
		isc_sha1_t *sha1ctx;
		isc_hmacmd5_t *hmacmd5ctx;
		isc_hmacsha1_t *hmacsha1ctx;
		isc_hmacsha224_t *hmacsha224ctx;
		isc_hmacsha256_t *hmacsha256ctx;
		isc_hmacsha384_t *hmacsha384ctx;
		isc_hmacsha512_t *hmacsha512ctx;
#ifdef OPENSSL
		EVP_MD_CTX *evp_md_ctx;
#endif
	} ctxdata;
};

struct dst_func {
	/*
	 * Context functions
	 */
	isc_result_t (*createctx)(dst_key_t *key, dst_context_t *dctx);
	void (*destroyctx)(dst_context_t *dctx);
	isc_result_t (*adddata)(dst_context_t *dctx, const isc_region_t *data);

	/*
	 * Key operations
	 */
	isc_result_t (*sign)(dst_context_t *dctx, isc_buffer_t *sig);
	isc_result_t (*verify)(dst_context_t *dctx, const isc_region_t *sig);
	isc_result_t (*computesecret)(const dst_key_t *pub,
				      const dst_key_t *priv,
				      isc_buffer_t *secret);
	isc_boolean_t (*compare)(const dst_key_t *key1, const dst_key_t *key2);
	isc_boolean_t (*paramcompare)(const dst_key_t *key1,
				      const dst_key_t *key2);
	isc_result_t (*generate)(dst_key_t *key, int parms);
	isc_boolean_t (*isprivate)(const dst_key_t *key);
	void (*destroy)(dst_key_t *key);

	/* conversion functions */
	isc_result_t (*todns)(const dst_key_t *key, isc_buffer_t *data);
	isc_result_t (*fromdns)(dst_key_t *key, isc_buffer_t *data);
	isc_result_t (*tofile)(const dst_key_t *key, const char *directory);
	isc_result_t (*parse)(dst_key_t *key, isc_lex_t *lexer);

	/* cleanup */
	void (*cleanup)(void);

	isc_result_t (*fromlabel)(dst_key_t *key, const char *engine,
				  const char *label, const char *pin);
};

/*%
 * Initializers
 */
isc_result_t dst__openssl_init(void);

isc_result_t dst__hmacmd5_init(struct dst_func **funcp);
isc_result_t dst__hmacsha1_init(struct dst_func **funcp);
isc_result_t dst__hmacsha224_init(struct dst_func **funcp);
isc_result_t dst__hmacsha256_init(struct dst_func **funcp);
isc_result_t dst__hmacsha384_init(struct dst_func **funcp);
isc_result_t dst__hmacsha512_init(struct dst_func **funcp);
isc_result_t dst__opensslrsa_init(struct dst_func **funcp);
isc_result_t dst__openssldsa_init(struct dst_func **funcp);
isc_result_t dst__openssldh_init(struct dst_func **funcp);
isc_result_t dst__gssapi_init(struct dst_func **funcp);

/*%
 * Destructors
 */
void dst__openssl_destroy(void);

/*%
 * Memory allocators using the DST memory pool.
 */
void * dst__mem_alloc(size_t size);
void   dst__mem_free(void *ptr);
void * dst__mem_realloc(void *ptr, size_t size);

/*%
 * Entropy retriever using the DST entropy pool.
 */
isc_result_t dst__entropy_getdata(void *buf, unsigned int len,
				  isc_boolean_t pseudo);

/*
 * Entropy status hook.
 */
unsigned int dst__entropy_status(void);

ISC_LANG_ENDDECLS

#endif /* DST_DST_INTERNAL_H */
/*! \file */
