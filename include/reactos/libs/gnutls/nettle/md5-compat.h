/* md5-compat.h
 *
 * The md5 hash function, RFC 1321-style interface.
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

#ifndef NETTLE_MD5_COMPAT_H_INCLUDED
#define NETTLE_MD5_COMPAT_H_INCLUDED

#include "md5.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Name mangling */
#define MD5Init nettle_MD5Init
#define MD5Update nettle_MD5Update
#define MD5Final nettle_MD5Final

	typedef struct md5_ctx MD5_CTX;

	void MD5Init(MD5_CTX * ctx);
	void MD5Update(MD5_CTX * ctx, const unsigned char *data,
		       unsigned int length);
	void MD5Final(unsigned char *out, MD5_CTX * ctx);

#ifdef __cplusplus
}
#endif
#endif				/* NETTLE_MD5_COMPAT_H_INCLUDED */
