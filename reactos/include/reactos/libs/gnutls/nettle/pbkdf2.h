/* pbkdf2.h
 *
 * PKCS #5 password-based key derivation function PBKDF2, see RFC 2898.
 */

/* nettle, low-level cryptographics library
 *
 * Copyright (C) 2012 Simon Josefsson
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

#ifndef NETTLE_PBKDF2_H_INCLUDED
#define NETTLE_PBKDF2_H_INCLUDED

#include "nettle-meta.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Namespace mangling */
#define pbkdf2 nettle_pbkdf2
#define pbkdf2_hmac_sha1 nettle_pbkdf2_hmac_sha1
#define pbkdf2_hmac_sha256 nettle_pbkdf2_hmac_sha256

	void
	 pbkdf2(void *mac_ctx,
		nettle_hash_update_func * update,
		nettle_hash_digest_func * digest,
		unsigned digest_size, unsigned iterations,
		unsigned salt_length, const uint8_t * salt,
		unsigned length, uint8_t * dst);

#define PBKDF2(ctx, update, digest, digest_size,			\
	       iterations, salt_length, salt, length, dst)		\
  (0 ? ((update)((ctx), 0, (uint8_t *) 0),				\
	(digest)((ctx), 0, (uint8_t *) 0))				\
   : pbkdf2 ((ctx),							\
	     (nettle_hash_update_func *)(update),			\
	     (nettle_hash_digest_func *)(digest),			\
	     (digest_size), (iterations),				\
	     (salt_length), (salt), (length), (dst)))

/* PBKDF2 with specific PRFs. */

	void
	 pbkdf2_hmac_sha1(unsigned key_length, const uint8_t * key,
			  unsigned iterations,
			  unsigned salt_length, const uint8_t * salt,
			  unsigned length, uint8_t * dst);

	void
	 pbkdf2_hmac_sha256(unsigned key_length, const uint8_t * key,
			    unsigned iterations,
			    unsigned salt_length, const uint8_t * salt,
			    unsigned length, uint8_t * dst);

#ifdef __cplusplus
}
#endif
#endif				/* NETTLE_PBKDF2_H_INCLUDED */
