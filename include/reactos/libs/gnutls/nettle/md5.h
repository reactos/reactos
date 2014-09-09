/* md5.h
 *
 * The MD5 hash function, described in RFC 1321.
 */

/* nettle, low-level cryptographics library
 *
 * Copyright (C) 2001 Niels Möller
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

#ifndef NETTLE_MD5_H_INCLUDED
#define NETTLE_MD5_H_INCLUDED

#include "nettle-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Name mangling */
#define md5_init nettle_md5_init
#define md5_update nettle_md5_update
#define md5_digest nettle_md5_digest

#define MD5_DIGEST_SIZE 16
#define MD5_DATA_SIZE 64

/* Digest is kept internally as 4 32-bit words. */
#define _MD5_DIGEST_LENGTH 4

	struct md5_ctx {
		uint32_t state[_MD5_DIGEST_LENGTH];
		uint32_t count_low, count_high;	/* Block count */
		uint8_t block[MD5_DATA_SIZE];	/* Block buffer */
		unsigned index;	/* Into buffer */
	};

	void
	 md5_init(struct md5_ctx *ctx);

	void
	 md5_update(struct md5_ctx *ctx,
		    unsigned length, const uint8_t * data);

	void
	 md5_digest(struct md5_ctx *ctx,
		    unsigned length, uint8_t * digest);

/* Internal compression function. STATE points to 4 uint32_t words,
   and DATA points to 64 bytes of input data, possibly unaligned. */
	void
	 _nettle_md5_compress(uint32_t * state, const uint8_t * data);

#ifdef __cplusplus
}
#endif
#endif				/* NETTLE_MD5_H_INCLUDED */
