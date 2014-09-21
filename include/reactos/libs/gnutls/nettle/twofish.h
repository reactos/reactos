/* twofish.h
 *
 * The twofish block cipher.
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

/*
 * Twofish is a 128-bit block cipher that accepts a variable-length
 * key up to 256 bits, designed by Bruce Schneier and others.  See
 * http://www.counterpane.com/twofish.html for details.
 */

#ifndef NETTLE_TWOFISH_H_INCLUDED
#define NETTLE_TWOFISH_H_INCLUDED

#include "nettle-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Name mangling */
#define twofish_set_key nettle_twofish_set_key
#define twofish_encrypt nettle_twofish_encrypt
#define twofish_decrypt nettle_twofish_decrypt

#define TWOFISH_BLOCK_SIZE 16

/* Variable key size between 128 and 256 bits. But the only valid
 * values are 16 (128 bits), 24 (192 bits) and 32 (256 bits). */
#define TWOFISH_MIN_KEY_SIZE 16
#define TWOFISH_MAX_KEY_SIZE 32

#define TWOFISH_KEY_SIZE 32

	struct twofish_ctx {
		uint32_t keys[40];
		uint32_t s_box[4][256];
	};

	void
	 twofish_set_key(struct twofish_ctx *ctx,
			 unsigned length, const uint8_t * key);

	void
	 twofish_encrypt(const struct twofish_ctx *ctx,
			 unsigned length, uint8_t * dst,
			 const uint8_t * src);
	void
	 twofish_decrypt(const struct twofish_ctx *ctx,
			 unsigned length, uint8_t * dst,
			 const uint8_t * src);

#ifdef __cplusplus
}
#endif
#endif				/* NETTLE_TWOFISH_H_INCLUDED */
