/* salsa20.h
 *
 * The Salsa20 stream cipher.
 */

/* nettle, low-level cryptographics library
 *
 * Copyright (C) 2012 Simon Josefsson
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

#ifndef NETTLE_SALSA20_H_INCLUDED
#define NETTLE_SALSA20_H_INCLUDED

#include "nettle-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Name mangling */
#define salsa20_set_key nettle_salsa20_set_key
#define salsa20_set_iv nettle_salsa20_set_iv
#define salsa20_crypt nettle_salsa20_crypt
#define _salsa20_core _nettle_salsa20_core

#define salsa20r12_crypt nettle_salsa20r12_crypt

/* Minimum and maximum keysizes, and a reasonable default. In
 * octets.*/
#define SALSA20_MIN_KEY_SIZE 16
#define SALSA20_MAX_KEY_SIZE 32
#define SALSA20_KEY_SIZE 32
#define SALSA20_BLOCK_SIZE 64

#define SALSA20_IV_SIZE 8

#define _SALSA20_INPUT_LENGTH 16

	struct salsa20_ctx {
		/* Indices 1-4 and 11-14 holds the key (two identical copies for the
		   shorter key size), indices 0, 5, 10, 15 are constant, indices 6, 7
		   are the IV, and indices 8, 9 are the block counter:

		   C K K K
		   K C I I
		   B B C K
		   K K K C
		 */
		uint32_t input[_SALSA20_INPUT_LENGTH];
	};

	void
	 salsa20_set_key(struct salsa20_ctx *ctx,
			 unsigned length, const uint8_t * key);

	void
	 salsa20_set_iv(struct salsa20_ctx *ctx, const uint8_t * iv);

	void
	 salsa20_crypt(struct salsa20_ctx *ctx,
		       unsigned length, uint8_t * dst,
		       const uint8_t * src);

	void
	 salsa20r12_crypt(struct salsa20_ctx *ctx,
			  unsigned length, uint8_t * dst,
			  const uint8_t * src);

	void
	 _salsa20_core(uint32_t * dst, const uint32_t * src,
		       unsigned rounds);

#ifdef __cplusplus
}
#endif
#endif				/* NETTLE_SALSA20_H_INCLUDED */
