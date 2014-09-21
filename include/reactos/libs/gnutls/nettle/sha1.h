/* sha1.h
 *
 * The sha1 hash function.
 */

/* nettle, low-level cryptographics library
 *
 * Copyright (C) 2001, 2012 Niels Möller
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

#ifndef NETTLE_SHA1_H_INCLUDED
#define NETTLE_SHA1_H_INCLUDED

#include "nettle-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Name mangling */
#define sha1_init nettle_sha1_init
#define sha1_update nettle_sha1_update
#define sha1_digest nettle_sha1_digest

/* SHA1 */

#define SHA1_DIGEST_SIZE 20
#define SHA1_DATA_SIZE 64

/* Digest is kept internally as 5 32-bit words. */
#define _SHA1_DIGEST_LENGTH 5

	struct sha1_ctx {
		uint32_t state[_SHA1_DIGEST_LENGTH];	/* State variables */
		uint32_t count_low, count_high;	/* 64-bit block count */
		uint8_t block[SHA1_DATA_SIZE];	/* SHA1 data buffer */
		unsigned int index;	/* index into buffer */
	};

	void
	 sha1_init(struct sha1_ctx *ctx);

	void
	 sha1_update(struct sha1_ctx *ctx,
		     unsigned length, const uint8_t * data);

	void
	 sha1_digest(struct sha1_ctx *ctx,
		     unsigned length, uint8_t * digest);

/* Internal compression function. STATE points to 5 uint32_t words,
   and DATA points to 64 bytes of input data, possibly unaligned. */
	void
	 _nettle_sha1_compress(uint32_t * state, const uint8_t * data);

#ifdef __cplusplus
}
#endif
#endif				/* NETTLE_SHA1_H_INCLUDED */
