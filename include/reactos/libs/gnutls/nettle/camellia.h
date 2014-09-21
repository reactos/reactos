/* camellia.h
 *
 * Copyright (C) 2006,2007
 * NTT (Nippon Telegraph and Telephone Corporation).
 *
 * Copyright (C) 2010 Niels Möller
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef NETTLE_CAMELLIA_H_INCLUDED
#define NETTLE_CAMELLIA_H_INCLUDED

#include "nettle-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Name mangling */
#define camellia_set_encrypt_key nettle_camellia_set_encrypt_key
#define camellia_set_decrypt_key nettle_camellia_set_decrypt_key
#define camellia_invert_key nettle_camellia_invert_key
#define camellia_crypt nettle_camellia_crypt
#define camellia_crypt nettle_camellia_crypt

#define CAMELLIA_BLOCK_SIZE 16
/* Valid key sizes are 128, 192 or 256 bits (16, 24 or 32 bytes) */
#define CAMELLIA_MIN_KEY_SIZE 16
#define CAMELLIA_MAX_KEY_SIZE 32
#define CAMELLIA_KEY_SIZE 32

	struct camellia_ctx {
		/* Number of subkeys. */
		unsigned nkeys;

		/* For 128-bit keys, there are 18 regular rounds, pre- and
		   post-whitening, and two FL and FLINV rounds, using a total of 26
		   subkeys, each of 64 bit. For 192- and 256-bit keys, there are 6
		   additional regular rounds and one additional FL and FLINV, using
		   a total of 34 subkeys. */
		/* The clever combination of subkeys imply one of the pre- and
		   post-whitening keys is folded with the round keys, so that subkey
		   #1 and the last one (#25 or #33) is not used. The result is that
		   we have only 24 or 32 subkeys at the end of key setup. */
		uint64_t keys[32];
	};

	void
	 camellia_set_encrypt_key(struct camellia_ctx *ctx,
				  unsigned length, const uint8_t * key);

	void
	 camellia_set_decrypt_key(struct camellia_ctx *ctx,
				  unsigned length, const uint8_t * key);

	void
	 camellia_invert_key(struct camellia_ctx *dst,
			     const struct camellia_ctx *src);

	void
	 camellia_crypt(const struct camellia_ctx *ctx,
			unsigned length, uint8_t * dst,
			const uint8_t * src);
#ifdef  __cplusplus
}
#endif
#endif				/* NETTLE_CAMELLIA_H_INCLUDED */
