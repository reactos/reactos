/* ctr.h
 *
 * Counter mode, using an network byte order incremented counter,
 * matching the testcases of NIST 800-38A.
 */

/* nettle, low-level cryptographics library
 *
 * Copyright (C) 2005 Niels Möller
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

#ifndef NETTLE_CTR_H_INCLUDED
#define NETTLE_CTR_H_INCLUDED

#include "nettle-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Name mangling */
#define ctr_crypt nettle_ctr_crypt

	void
	 ctr_crypt(void *ctx, nettle_crypt_func * f,
		   unsigned block_size, uint8_t * ctr,
		   unsigned length, uint8_t * dst, const uint8_t * src);

#define CTR_CTX(type, size) \
{ type ctx; uint8_t ctr[size]; }

#define CTR_SET_COUNTER(ctx, data) \
memcpy((ctx)->ctr, (data), sizeof((ctx)->ctr))

#define CTR_CRYPT(self, f, length, dst, src)		\
(0 ? ((f)(&(self)->ctx, 0, NULL, NULL))			\
   : ctr_crypt((void *) &(self)->ctx,			\
               (nettle_crypt_func *) (f),		\
	       sizeof((self)->ctr), (self)->ctr,	\
               (length), (dst), (src)))

#ifdef __cplusplus
}
#endif
#endif				/* NETTLE_CTR_H_INCLUDED */
