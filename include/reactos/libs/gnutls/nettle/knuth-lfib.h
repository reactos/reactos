/* knuth-lfib.h
 *
 * A "lagged fibonacci" pseudorandomness generator.
 *
 * Described in Knuth, TAOCP, 3.6
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

/* NOTE: This generator is totally inappropriate for cryptographic
 * applications. It is useful for generating deterministic but
 * random-looking test data, and is used by the Nettle testsuite. */
#ifndef NETTLE_KNUTH_LFIB_H_INCLUDED
#define NETTLE_KNUTH_LFIB_H_INCLUDED

#include "nettle-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Namespace mangling */
#define knuth_lfib_init nettle_knuth_lfib_init
#define knuth_lfib_get nettle_knuth_lfib_get
#define knuth_lfib_get_array nettle_knuth_lfib_get_array
#define knuth_lfib_random nettle_knuth_lfib_random

#define _KNUTH_LFIB_KK 100

	struct knuth_lfib_ctx {
		uint32_t x[_KNUTH_LFIB_KK];
		unsigned index;
	};

	void
	 knuth_lfib_init(struct knuth_lfib_ctx *ctx, uint32_t seed);

/* Get's a single number in the range 0 ... 2^30-1 */
	 uint32_t knuth_lfib_get(struct knuth_lfib_ctx *ctx);

/* Get an array of numbers */
	void
	 knuth_lfib_get_array(struct knuth_lfib_ctx *ctx,
			      unsigned n, uint32_t * a);

/* Get an array of octets. */
	void
	 knuth_lfib_random(struct knuth_lfib_ctx *ctx,
			   unsigned n, uint8_t * dst);

#ifdef __cplusplus
}
#endif
#endif				/* NETTLE_KNUTH_LFIB_H_INCLUDED */
