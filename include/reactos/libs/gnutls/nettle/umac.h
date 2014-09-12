/* umac.h
 *
 * UMAC message authentication code (RFC-4418).
 */

/* nettle, low-level cryptographics library
 *
 * Copyright (C) 2013 Niels Möller
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

#ifndef NETTLE_UMAC_H_INCLUDED
#define NETTLE_UMAC_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/* Namespace mangling */
#define umac32_set_key  nettle_umac32_set_key
#define umac64_set_key  nettle_umac64_set_key
#define umac96_set_key  nettle_umac96_set_key
#define umac128_set_key nettle_umac128_set_key
#define umac32_set_nonce  nettle_umac32_set_nonce
#define umac64_set_nonce  nettle_umac64_set_nonce
#define umac96_set_nonce  nettle_umac96_set_nonce
#define umac128_set_nonce nettle_umac128_set_nonce
#define umac32_update  nettle_umac32_update
#define umac64_update  nettle_umac64_update
#define umac96_update  nettle_umac96_update
#define umac128_update nettle_umac128_update
#define umac32_digest  nettle_umac32_digest
#define umac64_digest  nettle_umac64_digest
#define umac96_digest  nettle_umac96_digest
#define umac128_digest nettle_umac128_digest
#define _umac_set_key _nettle_umac_set_key
#define _umac_nh _nettle_umac_nh
#define _umac_nh_n _nettle_umac_nh_n
#define _umac_poly64 _nettle_umac_poly64
#define _umac_poly128 _nettle_umac_poly128
#define _umac_l2_init _nettle_umac_l2_init
#define _umac_l2 _nettle_umac_l2
#define _umac_l2_final _nettle_umac_l2_final
#define _umac_l3_init _nettle_umac_l3_init
#define _umac_l3 _nettle_umac_l3

#include "nettle-types.h"
#include "aes.h"

#define UMAC_KEY_SIZE 16
#define UMAC32_DIGEST_SIZE 4
#define UMAC64_DIGEST_SIZE 8
#define UMAC96_DIGEST_SIZE 12
#define UMAC128_DIGEST_SIZE 16
#define UMAC_DATA_SIZE 1024

/* Subkeys and state for UMAC with tag size 32*n bits. */
#define _UMAC_STATE(n)					\
  uint32_t l1_key[UMAC_DATA_SIZE/4 + 4*((n)-1)];	\
  /* Keys in 32-bit pieces, high first */		\
  uint32_t l2_key[6*(n)];				\
  uint64_t l3_key1[8*(n)];				\
  uint32_t l3_key2[(n)];				\
  /* AES cipher for encrypting the nonce */		\
  struct aes_ctx pdf_key;				\
  /* The l2_state consists of 2*n uint64_t, for poly64	\
     and poly128 hashing, followed by n additional	\
     uint64_t used as an input buffer. */		\
  uint64_t l2_state[3*(n)];				\
  /* Input to the pdf_key, zero-padded and low bits	\
     cleared if appropriate. */				\
  uint8_t nonce[AES_BLOCK_SIZE];			\
  unsigned short nonce_length	/* For incrementing */

	/* Buffering */
#define _UMAC_BUFFER					\
  unsigned index;					\
  /* Complete blocks processed */			\
  uint64_t count;					\
  uint8_t block[UMAC_DATA_SIZE]

#define _UMAC_NONCE_CACHED 0x80

	struct umac32_ctx {
		_UMAC_STATE(1);
		/* Low bits and cache flag. */
		unsigned short nonce_low;
		/* Previous padding block */
		uint32_t pad_cache[AES_BLOCK_SIZE / 4];
		 _UMAC_BUFFER;
	};

	struct umac64_ctx {
		_UMAC_STATE(2);
		/* Low bit and cache flag. */
		unsigned short nonce_low;
		/* Previous padding block */
		uint32_t pad_cache[AES_BLOCK_SIZE / 4];
		 _UMAC_BUFFER;
	};

	struct umac96_ctx {
		_UMAC_STATE(3);
		_UMAC_BUFFER;
	};

	struct umac128_ctx {
		_UMAC_STATE(4);
		_UMAC_BUFFER;
	};

/* The _set_key function initialize the nonce to zero. */
	void
	 umac32_set_key(struct umac32_ctx *ctx, const uint8_t * key);
	void
	 umac64_set_key(struct umac64_ctx *ctx, const uint8_t * key);
	void
	 umac96_set_key(struct umac96_ctx *ctx, const uint8_t * key);
	void
	 umac128_set_key(struct umac128_ctx *ctx, const uint8_t * key);

/* Optional, if not used, messages get incrementing nonces starting from zero. */
	void
	 umac32_set_nonce(struct umac32_ctx *ctx,
			  unsigned nonce_length, const uint8_t * nonce);
	void
	 umac64_set_nonce(struct umac64_ctx *ctx,
			  unsigned nonce_length, const uint8_t * nonce);
	void
	 umac96_set_nonce(struct umac96_ctx *ctx,
			  unsigned nonce_length, const uint8_t * nonce);
	void
	 umac128_set_nonce(struct umac128_ctx *ctx,
			   unsigned nonce_length, const uint8_t * nonce);

	void
	 umac32_update(struct umac32_ctx *ctx,
		       unsigned length, const uint8_t * data);
	void
	 umac64_update(struct umac64_ctx *ctx,
		       unsigned length, const uint8_t * data);
	void
	 umac96_update(struct umac96_ctx *ctx,
		       unsigned length, const uint8_t * data);
	void
	 umac128_update(struct umac128_ctx *ctx,
			unsigned length, const uint8_t * data);

/* The _digest functions increment the nonce */
	void
	 umac32_digest(struct umac32_ctx *ctx,
		       unsigned length, uint8_t * digest);
	void
	 umac64_digest(struct umac64_ctx *ctx,
		       unsigned length, uint8_t * digest);
	void
	 umac96_digest(struct umac96_ctx *ctx,
		       unsigned length, uint8_t * digest);
	void
	 umac128_digest(struct umac128_ctx *ctx,
			unsigned length, uint8_t * digest);


/* Internal functions */
#define UMAC_POLY64_BLOCKS 16384

#define UMAC_P64_OFFSET 59
#define UMAC_P64 (- (uint64_t) UMAC_P64_OFFSET)

#define UMAC_P128_OFFSET 159
#define UMAC_P128_HI (~(uint64_t) 0)
#define UMAC_P128_LO (-(uint64_t) UMAC_P128_OFFSET)

	void
	 _umac_set_key(uint32_t * l1_key, uint32_t * l2_key,
		       uint64_t * l3_key1, uint32_t * l3_key2,
		       struct aes_ctx *pad, const uint8_t * key,
		       unsigned n);

	 uint64_t
	    _umac_nh(const uint32_t * key, unsigned length,
		     const uint8_t * msg);

/* Equivalent to

   for (i = 0; i < n; i++)
     out[i] = _umac_nh (key + 4*i, length, msg);

   but processing input only once.
*/
	void
	 _umac_nh_n(uint64_t * out, unsigned n, const uint32_t * key,
		    unsigned length, const uint8_t * msg);

/* Returns y*k + m (mod p), including "marker" processing. Return
   value is *not* in canonical representation, and must be normalized
   before the output is used. */
	 uint64_t
	    _umac_poly64(uint32_t kh, uint32_t kl, uint64_t y, uint64_t m);

	void
	 _umac_poly128(const uint32_t * k, uint64_t * y, uint64_t mh,
		       uint64_t ml);

	void
	 _umac_l2_init(unsigned size, uint32_t * k);

	void
	 _umac_l2(const uint32_t * key, uint64_t * state, unsigned n,
		  uint64_t count, const uint64_t * m);

	void
	 _umac_l2_final(const uint32_t * key, uint64_t * state, unsigned n,
			uint64_t count);

	void
	 _umac_l3_init(unsigned size, uint64_t * k);

	 uint32_t _umac_l3(const uint64_t * key, const uint64_t * m);

#ifdef __cplusplus
}
#endif
#endif				/* NETTLE_UMAC_H_INCLUDED */
