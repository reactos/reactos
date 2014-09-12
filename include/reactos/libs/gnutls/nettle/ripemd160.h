/* ripemd160.h
 *
 * RIPEMD-160 hash function.
 */

/* nettle, low-level cryptographics library
 *
 * Copyright (C) 2011 Andres Mejia
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

#ifndef NETTLE_RIPEMD160_H_INCLUDED
#define NETTLE_RIPEMD160_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "nettle-types.h"

/* Name mangling */
#define ripemd160_init nettle_ripemd160_init
#define ripemd160_update nettle_ripemd160_update
#define ripemd160_digest nettle_ripemd160_digest

/* RIPEMD160 */

#define RIPEMD160_DIGEST_SIZE 20
#define RIPEMD160_DATA_SIZE 64

/* Digest is kept internally as 5 32-bit words. */
#define _RIPEMD160_DIGEST_LENGTH 5

	struct ripemd160_ctx {
		uint32_t state[_RIPEMD160_DIGEST_LENGTH];
		uint32_t count_low, count_high;	/* 64-bit block count */
		uint8_t block[RIPEMD160_DATA_SIZE];
		unsigned int index;
	};

	void
	 ripemd160_init(struct ripemd160_ctx *ctx);

	void
	 ripemd160_update(struct ripemd160_ctx *ctx,
			  unsigned length, const uint8_t * data);

	void
	 ripemd160_digest(struct ripemd160_ctx *ctx,
			  unsigned length, uint8_t * digest);

/* Internal compression function. STATE points to 5 uint32_t words,
   and DATA points to 64 bytes of input data, possibly unaligned. */
	void
	 _nettle_ripemd160_compress(uint32_t * state,
				    const uint8_t * data);

#ifdef __cplusplus
}
#endif
#endif				/* NETTLE_RIPEMD160_H_INCLUDED */
