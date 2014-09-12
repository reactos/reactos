/* md2.h
 *
 * The MD2 hash function, described in RFC 1319.
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

#ifndef NETTLE_MD2_H_INCLUDED
#define NETTLE_MD2_H_INCLUDED

#include "nettle-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Name mangling */
#define md2_init nettle_md2_init
#define md2_update nettle_md2_update
#define md2_digest nettle_md2_digest

#define MD2_DIGEST_SIZE 16
#define MD2_DATA_SIZE 16

	struct md2_ctx {
		uint8_t C[MD2_DATA_SIZE];
		uint8_t X[3 * MD2_DATA_SIZE];
		uint8_t block[MD2_DATA_SIZE];	/* Block buffer */
		unsigned index;	/* Into buffer */
	};

	void
	 md2_init(struct md2_ctx *ctx);

	void
	 md2_update(struct md2_ctx *ctx,
		    unsigned length, const uint8_t * data);

	void
	 md2_digest(struct md2_ctx *ctx,
		    unsigned length, uint8_t * digest);


#ifdef __cplusplus
}
#endif
#endif				/* NETTLE_MD2_H_INCLUDED */
