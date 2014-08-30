/* cbc.h
 *
 * Cipher block chaining mode.
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

#ifndef NETTLE_CBC_H_INCLUDED
#define NETTLE_CBC_H_INCLUDED

#include "nettle-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Name mangling */
#define cbc_encrypt nettle_cbc_encrypt
#define cbc_decrypt nettle_cbc_decrypt

	void
	 cbc_encrypt(void *ctx, nettle_crypt_func * f,
		     unsigned block_size, uint8_t * iv,
		     unsigned length, uint8_t * dst, const uint8_t * src);

	void
	 cbc_decrypt(void *ctx, nettle_crypt_func * f,
		     unsigned block_size, uint8_t * iv,
		     unsigned length, uint8_t * dst, const uint8_t * src);

#define CBC_CTX(type, size) \
{ type ctx; uint8_t iv[size]; }

#define CBC_SET_IV(ctx, data) \
memcpy((ctx)->iv, (data), sizeof((ctx)->iv))

/* NOTE: Avoid using NULL, as we don't include anything defining it. */
#define CBC_ENCRYPT(self, f, length, dst, src)		\
(0 ? ((f)(&(self)->ctx, 0, (void *)0, (void *)0))			\
   : cbc_encrypt((void *) &(self)->ctx,			\
                 (nettle_crypt_func *) (f),		\
		 sizeof((self)->iv), (self)->iv,	\
                 (length), (dst), (src)))

#define CBC_DECRYPT(self, f, length, dst, src)		\
(0 ? ((f)(&(self)->ctx, 0, (void *)0, (void *)0))			\
   : cbc_decrypt((void *) &(self)->ctx,			\
                 (nettle_crypt_func *) (f),		\
		 sizeof((self)->iv), (self)->iv,	\
                 (length), (dst), (src)))

#ifdef __cplusplus
}
#endif
#endif				/* NETTLE_CBC_H_INCLUDED */
