/* des-compat.h
 *
 * The des block cipher, libdes/openssl-style interface.
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

#ifndef NETTLE_DES_COMPAT_H_INCLUDED
#define NETTLE_DES_COMPAT_H_INCLUDED

/* According to Assar, des_set_key, des_set_key_odd_parity,
 * des_is_weak_key, plus the encryption functions (des_*_encrypt and
 * des_cbc_cksum) would be a pretty useful subset. */

/* NOTE: This is quite experimental, and not all functions are
 * implemented. Contributions, in particular test cases are welcome. */

#include "des.h"

#ifdef __cplusplus
extern "C" {
#endif

/* We use some name mangling, to avoid collisions with either other
 * nettle functions or with libcrypto. */

#define des_ecb3_encrypt nettle_openssl_des_ecb3_encrypt
#define des_cbc_cksum nettle_openssl_des_cbc_cksum
#define des_ncbc_encrypt nettle_openssl_des_ncbc_encrypt
#define des_cbc_encrypt nettle_openssl_des_cbc_encrypt
#define des_ecb_encrypt nettle_openssl_des_ecb_encrypt
#define des_ede3_cbc_encrypt nettle_openssl_des_ede3_cbc_encrypt
#define des_set_odd_parity nettle_openssl_des_set_odd_parity
#define des_check_key nettle_openssl_des_check_key
#define des_key_sched nettle_openssl_des_key_sched
#define des_is_weak_key nettle_openssl_des_is_weak_key

/* An extra alias */
#undef des_set_key
#define des_set_key nettle_openssl_des_key_sched

	enum { DES_DECRYPT = 0, DES_ENCRYPT = 1 };

/* Types */
	typedef uint32_t DES_LONG;

/* Note: Typedef:ed arrays should be avoided, but they're used here
 * for compatibility. */
	typedef struct des_ctx des_key_schedule[1];

	typedef uint8_t des_cblock[DES_BLOCK_SIZE];
/* Note: The proper definition,

     typedef const uint8_t const_des_cblock[DES_BLOCK_SIZE];

   would have worked, *if* all the prototypes had used arguments like
   foo(const_des_cblock src, des_cblock dst), letting argument arrays
   "decay" into pointers of type uint8_t * and const uint8_t *.

   But since openssl's prototypes use *pointers* const_des_cblock *src,
   des_cblock *dst, this ends up in type conflicts, and the workaround
   is to not use const at all.
*/
#define const_des_cblock des_cblock

/* Aliases */
#define des_ecb2_encrypt(i,o,k1,k2,e) \
	des_ecb3_encrypt((i),(o),(k1),(k2),(k1),(e))

#define des_ede2_cbc_encrypt(i,o,l,k1,k2,iv,e) \
	des_ede3_cbc_encrypt((i),(o),(l),(k1),(k2),(k1),(iv),(e))

/* Global flag */
	extern int des_check_key;

/* Prototypes */

/* Typing is a little confusing. Since both des_cblock and
   des_key_schedule are typedef:ed arrays, it automatically decay to
   a pointers.

   But the functions are declared taking pointers to des_cblock, i.e.
   pointers to arrays. And on the other hand, they take plain
   des_key_schedule arguments, which is equivalent to pointers to
   struct des_ctx.  */
	void
	 des_ecb3_encrypt(const_des_cblock * src, des_cblock * dst,
			  des_key_schedule k1,
			  des_key_schedule k2,
			  des_key_schedule k3, int enc);

/* des_cbc_cksum in libdes returns a 32 bit integer, representing the
 * latter half of the output block, using little endian byte order. */
	 uint32_t
	    des_cbc_cksum(const uint8_t * src, des_cblock * dst,
			  long length, des_key_schedule ctx,
			  const_des_cblock * iv);

/* NOTE: Doesn't update iv. */
	void
	 des_cbc_encrypt(const_des_cblock * src, des_cblock * dst,
			 long length, des_key_schedule ctx,
			 const_des_cblock * iv, int enc);

/* Similar, but updates iv. */
	void
	 des_ncbc_encrypt(const_des_cblock * src, des_cblock * dst,
			  long length, des_key_schedule ctx,
			  des_cblock * iv, int enc);

	void
	 des_ecb_encrypt(const_des_cblock * src, des_cblock * dst,
			 des_key_schedule ctx, int enc);

	void
	 des_ede3_cbc_encrypt(const_des_cblock * src, des_cblock * dst,
			      long length, des_key_schedule k1,
			      des_key_schedule k2, des_key_schedule k3,
			      des_cblock * iv, int enc);

	int
	 des_set_odd_parity(des_cblock * key);

	int
	 des_key_sched(const_des_cblock * key, des_key_schedule ctx);

	int
	 des_is_weak_key(const_des_cblock * key);

#ifdef __cplusplus
}
#endif
#endif				/* NETTLE_DES_COMPAT_H_INCLUDED */
