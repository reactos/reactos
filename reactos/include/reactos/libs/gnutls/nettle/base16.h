/* base16.h
 *
 * Hex encoding and decoding, following spki conventions (i.e.
 * allowing whitespace between digits).
 */

/* nettle, low-level cryptographics library
 *
 * Copyright (C) 2002 Niels Möller
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

#ifndef NETTLE_BASE16_H_INCLUDED
#define NETTLE_BASE16_H_INCLUDED

#include "nettle-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Name mangling */
#define base16_encode_single nettle_base16_encode_single
#define base16_encode_update nettle_base16_encode_update
#define base16_decode_init nettle_base16_decode_init
#define base16_decode_single nettle_base16_decode_single
#define base16_decode_update nettle_base16_decode_update
#define base16_decode_final nettle_base16_decode_final

/* Base16 encoding */

/* Maximum length of output for base16_encode_update. */
#define BASE16_ENCODE_LENGTH(length) ((length) * 2)

/* Encodes a single byte. Always stores two digits in dst[0] and dst[1]. */
	void
	 base16_encode_single(uint8_t * dst, uint8_t src);

/* Always stores BASE16_ENCODE_LENGTH(length) digits in dst. */
	void
	 base16_encode_update(uint8_t * dst,
			      unsigned length, const uint8_t * src);


/* Base16 decoding */

/* Maximum length of output for base16_decode_update. */
/* We have at most 4 buffered bits, and a total of (length + 1) * 4 bits. */
#define BASE16_DECODE_LENGTH(length) (((length) + 1) / 2)

	struct base16_decode_ctx {
		unsigned word;	/* Leftover bits */
		unsigned bits;	/* Number buffered bits */
	};

	void
	 base16_decode_init(struct base16_decode_ctx *ctx);

/* Decodes a single byte. Returns amount of output (0 or 1), or -1 on
 * errors. */
	int
	 base16_decode_single(struct base16_decode_ctx *ctx,
			      uint8_t * dst, uint8_t src);

/* Returns 1 on success, 0 on error. DST should point to an area of
 * size at least BASE16_DECODE_LENGTH(length), and for sanity
 * checking, *DST_LENGTH should be initialized to the size of that
 * area before the call. *DST_LENGTH is updated to the amount of
 * decoded output. */

/* Currently results in an assertion failure if *DST_LENGTH is
 * too small. FIXME: Return some error instead? */
	int
	 base16_decode_update(struct base16_decode_ctx *ctx,
			      unsigned *dst_length,
			      uint8_t * dst,
			      unsigned src_length, const uint8_t * src);

/* Returns 1 on success. */
	int
	 base16_decode_final(struct base16_decode_ctx *ctx);

#ifdef __cplusplus
}
#endif
#endif				/* NETTLE_BASE16_H_INCLUDED */
