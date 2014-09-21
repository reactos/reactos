/* arctwo.h
 *
 * The arctwo/rfc2268 block cipher.
 */

/* nettle, low-level cryptographics library
 *
 * Copyright (C) 2004 Simon Josefsson
 * Copyright (C) 2002, 2004 Niels Möller
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

#ifndef NETTLE_ARCTWO_H_INCLUDED
#define NETTLE_ARCTWO_H_INCLUDED

#include "nettle-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Name mangling */
#define arctwo_set_key nettle_arctwo_set_key
#define arctwo_set_key_ekb nettle_arctwo_set_key_ekb
#define arctwo_encrypt nettle_arctwo_encrypt
#define arctwo_decrypt nettle_arctwo_decrypt
#define arctwo_set_key_gutmann nettle_arctwo_set_key_gutmann

#define ARCTWO_BLOCK_SIZE 8

/* Variable key size from 1 byte to 128 bytes. */
#define ARCTWO_MIN_KEY_SIZE 1
#define ARCTWO_MAX_KEY_SIZE 128

#define ARCTWO_KEY_SIZE 8

	struct arctwo_ctx {
		uint16_t S[64];
	};

/* Key expansion function that takes the "effective key bits", 1-1024,
   as an explicit argument. 0 means maximum key bits. */
	void
	 arctwo_set_key_ekb(struct arctwo_ctx *ctx,
			    unsigned length, const uint8_t * key,
			    unsigned ekb);

/* Equvivalent to arctwo_set_key_ekb, with ekb = 8 * length */
	void
	 arctwo_set_key(struct arctwo_ctx *ctx, unsigned length,
			const uint8_t * key);

/* Equvivalent to arctwo_set_key_ekb, with ekb = 1024 */
	void
	 arctwo_set_key_gutmann(struct arctwo_ctx *ctx,
				unsigned length, const uint8_t * key);

	void
	 arctwo_encrypt(struct arctwo_ctx *ctx,
			unsigned length, uint8_t * dst,
			const uint8_t * src);
	void
	 arctwo_decrypt(struct arctwo_ctx *ctx,
			unsigned length, uint8_t * dst,
			const uint8_t * src);

#ifdef __cplusplus
}
#endif
#endif				/* NETTLE_ARCTWO_H_INCLUDED */
