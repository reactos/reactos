/* gosthash94.h
 *
 * The GOST R 34.11-94 hash function, described in RFC 5831.
 */

/* nettle, low-level cryptographics library
 *
 * Copyright (C) 2012 Nikos Mavrogiannopoulos, Niels Möller
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

/* Interface based on rhash gost.h. */

/* Copyright: 2009-2012 Aleksey Kravchenko <rhash.admin@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Ported to nettle by Nikos Mavrogiannopoulos.
 */

#ifndef NETTLE_GOSTHASH94_H_INCLUDED
#define NETTLE_GOSTHASH94_H_INCLUDED

#include "nettle-types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define gosthash94_init nettle_gosthash94_init
#define gosthash94_update nettle_gosthash94_update
#define gosthash94_digest nettle_gosthash94_digest

#define GOSTHASH94_DATA_SIZE 32
#define GOSTHASH94_DIGEST_SIZE 32

	struct gosthash94_ctx {
		uint32_t hash[8];	/* algorithm 256-bit state */
		uint32_t sum[8];	/* sum of processed message blocks */
		uint8_t message[GOSTHASH94_DATA_SIZE];	/* 256-bit buffer for leftovers */
		uint64_t length;	/* number of processed bytes */
	};

	void gosthash94_init(struct gosthash94_ctx *ctx);
	void gosthash94_update(struct gosthash94_ctx *ctx,
			       unsigned length, const uint8_t * msg);
	void gosthash94_digest(struct gosthash94_ctx *ctx,
			       unsigned length, uint8_t * result);

#ifdef __cplusplus
}
#endif
#endif				/* NETTLE_GOSTHASH94_H_INCLUDED */
