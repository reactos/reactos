/* gcm.h
 *
 * Galois counter mode, specified by NIST,
 * http://csrc.nist.gov/publications/nistpubs/800-38D/SP-800-38D.pdf
 *
 */

/* NOTE: Tentative interface, subject to change. No effort will be
   made to avoid incompatible changes. */

/* nettle, low-level cryptographics library
 *
 * Copyright (C) 2011 Niels Möller
 * Copyright (C) 2011 Katholieke Universiteit Leuven
 * 
 * Contributed by Nikos Mavrogiannopoulos
 *
 * The nettle library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 * 
 * The nettle library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with the nettle library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02111-1301, USA.
 */

#ifndef NETTLE_GCM_H_INCLUDED
#define NETTLE_GCM_H_INCLUDED

#include "aes.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Name mangling */
#define gcm_set_key nettle_gcm_set_key
#define gcm_set_iv nettle_gcm_set_iv
#define gcm_update nettle_gcm_update
#define gcm_encrypt nettle_gcm_encrypt
#define gcm_decrypt nettle_gcm_decrypt
#define gcm_digest nettle_gcm_digest

#define gcm_aes_set_key nettle_gcm_aes_set_key
#define gcm_aes_set_iv nettle_gcm_aes_set_iv
#define gcm_aes_update nettle_gcm_aes_update
#define gcm_aes_encrypt nettle_gcm_aes_encrypt
#define gcm_aes_decrypt nettle_gcm_aes_decrypt
#define gcm_aes_digest nettle_gcm_aes_digest

#define GCM_BLOCK_SIZE 16
#define GCM_IV_SIZE (GCM_BLOCK_SIZE - 4)

#define GCM_TABLE_BITS 8

/* To make sure that we have proper alignment. */
	union gcm_block {
		uint8_t b[GCM_BLOCK_SIZE];
		unsigned long w[GCM_BLOCK_SIZE / sizeof(unsigned long)];
	};

/* Hashing subkey */
	struct gcm_key {
		union gcm_block h[1 << GCM_TABLE_BITS];
	};

/* Per-message state, depending on the iv */
	struct gcm_ctx {
		/* Original counter block */
		union gcm_block iv;
		/* Updated for each block. */
		union gcm_block ctr;
		/* Hashing state */
		union gcm_block x;
		uint64_t auth_size;
		uint64_t data_size;
	};

/* FIXME: Should use const for the cipher context. Then needs const for
   nettle_crypt_func, which also rules out using that abstraction for
   arcfour. */
	void
	 gcm_set_key(struct gcm_key *key,
		     void *cipher, nettle_crypt_func * f);

	void
	 gcm_set_iv(struct gcm_ctx *ctx, const struct gcm_key *key,
		    unsigned length, const uint8_t * iv);

	void
	 gcm_update(struct gcm_ctx *ctx, const struct gcm_key *key,
		    unsigned length, const uint8_t * data);

	void
	 gcm_encrypt(struct gcm_ctx *ctx, const struct gcm_key *key,
		     void *cipher, nettle_crypt_func * f,
		     unsigned length, uint8_t * dst, const uint8_t * src);

	void
	 gcm_decrypt(struct gcm_ctx *ctx, const struct gcm_key *key,
		     void *cipher, nettle_crypt_func * f,
		     unsigned length, uint8_t * dst, const uint8_t * src);

	void
	 gcm_digest(struct gcm_ctx *ctx, const struct gcm_key *key,
		    void *cipher, nettle_crypt_func * f,
		    unsigned length, uint8_t * digest);

/* Convenience macrology (not sure how useful it is) */

/* All-in-one context, with cipher, hash subkey, and message state. */
#define GCM_CTX(type) \
{ type cipher; struct gcm_key key; struct gcm_ctx gcm; }

/* NOTE: Avoid using NULL, as we don't include anything defining it. */
#define GCM_SET_KEY(ctx, set_key, encrypt, length, data)	\
  do {								\
    (set_key)(&(ctx)->cipher, (length), (data));		\
    if (0) (encrypt)(&(ctx)->cipher, 0, (void *)0, (void *)0);	\
    gcm_set_key(&(ctx)->key, &(ctx)->cipher,			\
		(nettle_crypt_func *) (encrypt));		\
  } while (0)

#define GCM_SET_IV(ctx, length, data)				\
  gcm_set_iv(&(ctx)->gcm, &(ctx)->key, (length), (data))

#define GCM_UPDATE(ctx, length, data)			\
  gcm_update(&(ctx)->gcm, &(ctx)->key, (length), (data))

#define GCM_ENCRYPT(ctx, encrypt, length, dst, src)			\
  (0 ? (encrypt)(&(ctx)->cipher, 0, (void *)0, (void *)0)		\
     : gcm_encrypt(&(ctx)->gcm, &(ctx)->key, &(ctx)->cipher,		\
		   (nettle_crypt_func *) (encrypt),			\
		   (length), (dst), (src)))

#define GCM_DECRYPT(ctx, encrypt, length, dst, src)			\
  (0 ? (encrypt)(&(ctx)->cipher, 0, (void *)0, (void *)0)		\
     : gcm_decrypt(&(ctx)->gcm,  &(ctx)->key, &(ctx)->cipher,		\
		   (nettle_crypt_func *) (encrypt),			\
		   (length), (dst), (src)))

#define GCM_DIGEST(ctx, encrypt, length, digest)			\
  (0 ? (encrypt)(&(ctx)->cipher, 0, (void *)0, (void *)0)		\
     : gcm_digest(&(ctx)->gcm, &(ctx)->key, &(ctx)->cipher,		\
		  (nettle_crypt_func *) (encrypt),			\
		  (length), (digest)))

	struct gcm_aes_ctx GCM_CTX(struct aes_ctx);

	void
	 gcm_aes_set_key(struct gcm_aes_ctx *ctx,
			 unsigned length, const uint8_t * key);

	void
	 gcm_aes_set_iv(struct gcm_aes_ctx *ctx,
			unsigned length, const uint8_t * iv);

	void
	 gcm_aes_update(struct gcm_aes_ctx *ctx,
			unsigned length, const uint8_t * data);

	void
	 gcm_aes_encrypt(struct gcm_aes_ctx *ctx,
			 unsigned length, uint8_t * dst,
			 const uint8_t * src);

	void
	 gcm_aes_decrypt(struct gcm_aes_ctx *ctx,
			 unsigned length, uint8_t * dst,
			 const uint8_t * src);

	void
	 gcm_aes_digest(struct gcm_aes_ctx *ctx, unsigned length,
			uint8_t * digest);

#ifdef __cplusplus
}
#endif
#endif				/* NETTLE_GCM_H_INCLUDED */
