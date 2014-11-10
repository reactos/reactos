/* md4.h
 *
 * The MD4 hash function, described in RFC 1320.
 */

/* nettle, low-level cryptographics library
 *
 * Copyright (C) 2003 Niels Möller
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

#ifndef NETTLE_MD4_H_INCLUDED
#define NETTLE_MD4_H_INCLUDED

#include "nettle-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Name mangling */
#define md4_init nettle_md4_init
#define md4_update nettle_md4_update
#define md4_digest nettle_md4_digest

#define MD4_DIGEST_SIZE 16
#define MD4_DATA_SIZE 64

/* Digest is kept internally as 4 32-bit words. */
#define _MD4_DIGEST_LENGTH 4

/* FIXME: Identical to md5_ctx */
	struct md4_ctx {
		uint32_t state[_MD4_DIGEST_LENGTH];
		uint32_t count_low, count_high;	/* Block count */
		uint8_t block[MD4_DATA_SIZE];	/* Block buffer */
		unsigned index;	/* Into buffer */
	};

	void
	 md4_init(struct md4_ctx *ctx);

	void
	 md4_update(struct md4_ctx *ctx,
		    unsigned length, const uint8_t * data);

	void
	 md4_digest(struct md4_ctx *ctx,
		    unsigned length, uint8_t * digest);


#ifdef __cplusplus
}
#endif
#endif				/* NETTLE_MD4_H_INCLUDED */
