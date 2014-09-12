/* ecc.h */

/* nettle, low-level cryptographics library
 *
 * Copyright (C) 2013 Niels Möller
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

/* Development of Nettle's ECC support was funded by the .SE Internet Fund. */

#ifndef NETTLE_ECC_H_INCLUDED
#define NETTLE_ECC_H_INCLUDED

#include <gmp.h>

#include "nettle-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Name mangling */
#define ecc_point_init nettle_ecc_point_init
#define ecc_point_clear nettle_ecc_point_clear
#define ecc_point_set nettle_ecc_point_set
#define ecc_point_get nettle_ecc_point_get
#define ecc_point_mul nettle_ecc_point_mul
#define ecc_point_mul_g nettle_ecc_point_mul_g
#define ecc_scalar_init nettle_ecc_scalar_init
#define ecc_scalar_clear nettle_ecc_scalar_clear
#define ecc_scalar_set nettle_ecc_scalar_set
#define ecc_scalar_get nettle_ecc_scalar_get
#define ecc_scalar_random nettle_ecc_scalar_random
#define ecc_point_mul nettle_ecc_point_mul
#define ecc_size nettle_ecc_size
#define ecc_size_a nettle_ecc_size_a
#define ecc_size_j nettle_ecc_size_j
#define ecc_a_to_a_itch nettle_ecc_a_to_a_itch
#define ecc_a_to_a nettle_ecc_a_to_a
#define ecc_a_to_j nettle_ecc_a_to_j
#define ecc_j_to_a_itch nettle_ecc_j_to_a_itch
#define ecc_j_to_a nettle_ecc_j_to_a
#define ecc_dup_ja_itch nettle_ecc_dup_ja_itch
#define ecc_dup_ja nettle_ecc_dup_ja
#define ecc_dup_jj_itch nettle_ecc_dup_jj_itch
#define ecc_dup_jj nettle_ecc_dup_jj
#define ecc_add_jja_itch nettle_ecc_add_jja_itch
#define ecc_add_jja nettle_ecc_add_jja
#define ecc_add_jjj_itch nettle_ecc_add_jjj_itch
#define ecc_add_jjj nettle_ecc_add_jjj
#define ecc_mul_g_itch nettle_ecc_mul_g_itch
#define ecc_mul_g nettle_ecc_mul_g
#define ecc_mul_a_itch nettle_ecc_mul_a_itch
#define ecc_mul_a nettle_ecc_mul_a

	struct ecc_curve;

/* High level interface, for ECDSA, DH, etc */

/* Represents a point on the ECC curve */
	struct ecc_point {
		const struct ecc_curve *ecc;
		/* Allocated using the same allocation function as GMP. */
		mp_limb_t *p;
	};

/* Represents a non-zero scalar, an element of Z_q^*, where q is the
   group order of the curve. */
	struct ecc_scalar {
		const struct ecc_curve *ecc;
		/* Allocated using the same allocation function as GMP. */
		mp_limb_t *p;
	};

	void
	 ecc_point_init(struct ecc_point *p, const struct ecc_curve *ecc);
	void
	 ecc_point_clear(struct ecc_point *p);

/* Fails and returns zero if the point is not on the curve. */
	int
	 ecc_point_set(struct ecc_point *p, const mpz_t x, const mpz_t y);
	void
	 ecc_point_get(const struct ecc_point *p, mpz_t x, mpz_t y);

	void
	 ecc_scalar_init(struct ecc_scalar *s,
			 const struct ecc_curve *ecc);
	void
	 ecc_scalar_clear(struct ecc_scalar *s);

/* Fails and returns zero if the scalar is not in the proper range. */
	int
	 ecc_scalar_set(struct ecc_scalar *s, const mpz_t z);
	void
	 ecc_scalar_get(const struct ecc_scalar *s, mpz_t z);
/* Generates a random scalar, suitable as an ECDSA private key or a
   ECDH exponent. */
	void
	 ecc_scalar_random(struct ecc_scalar *s,
			   void *random_ctx, nettle_random_func * random);

/* Computes r = n p */
	void
	 ecc_point_mul(struct ecc_point *r, const struct ecc_scalar *n,
		       const struct ecc_point *p);

/* Computes r = n g */
	void
	 ecc_point_mul_g(struct ecc_point *r, const struct ecc_scalar *n);


/* Low-level interface */

/* Points on a curve are represented as arrays of mp_limb_t. For some
   curves, point coordinates are represented in montgomery form. We
   use either affine coordinates x,y, or Jacobian coordinates X, Y, Z,
   where x = X/Z^2 and y = X/Z^2.

   Since we use additive notation for the groups, the infinity point
   on the curve is denoted 0. The infinity point can be represented
   with x = y = 0 in affine coordinates, and Z = 0 in Jacobian
   coordinates. However, note that most of the ECC functions do *not*
   support infinity as an input or output.
*/

/* FIXME: Also provided some compile time constants? */

/* Returns the size of a single coordinate. */
	 mp_size_t ecc_size(const struct ecc_curve *ecc);

/* Size of a point, using affine coordinates x, y. */
	 mp_size_t ecc_size_a(const struct ecc_curve *ecc);

/* Size of a point, using jacobian coordinates X, Y and Z. */
	 mp_size_t ecc_size_j(const struct ecc_curve *ecc);

/* FIXME: Rename the low-level (and side-channel silent) functions to
   _ecc_*, and provide public ecc_* functions which handle the
   infinity points properly? */

/* Converts the affine coordinates of a point into montgomery form, if
   used for this curve. */
	 mp_size_t ecc_a_to_a_itch(const struct ecc_curve *ecc);
	void
	 ecc_a_to_a(const struct ecc_curve *ecc,
		    mp_limb_t * r, const mp_limb_t * p,
		    mp_limb_t * scratch);

/* Converts a point P in affine coordinates into a point R in jacobian
   coordinates. If INITIAL is non-zero, and the curve uses montgomery
   coordinates, also convert coordinates to montgomery form. */
	void
	 ecc_a_to_j(const struct ecc_curve *ecc,
		    int initial, mp_limb_t * r, const mp_limb_t * p);

/* Converts a point P in jacobian coordinates into a point R in affine
   coordinates. If FLAGS has bit 0 set, and the curve uses montgomery
   coordinates, also undo the montgomery conversion. If flags has bit
   1 set, produce x coordinate only. */
	 mp_size_t ecc_j_to_a_itch(const struct ecc_curve *ecc);
	void
	 ecc_j_to_a(const struct ecc_curve *ecc,
		    int flags,
		    mp_limb_t * r, const mp_limb_t * p,
		    mp_limb_t * scratch);

/* Group operations */


/* Point doubling, with jacobian output and affine input. Corner
   cases: Correctly sets R = 0 (r_Z = 0) if p = 0 or 2p = 0. */
	 mp_size_t ecc_dup_ja_itch(const struct ecc_curve *ecc);
	void
	 ecc_dup_ja(const struct ecc_curve *ecc,
		    mp_limb_t * r, const mp_limb_t * p,
		    mp_limb_t * scratch);

/* Point doubling, with jacobian input and output. Corner cases:
   Correctly sets R = 0 (r_Z = 0) if p = 0 or 2p = 0. */
	 mp_size_t ecc_dup_jj_itch(const struct ecc_curve *ecc);
	void
	 ecc_dup_jj(const struct ecc_curve *ecc,
		    mp_limb_t * r, const mp_limb_t * p,
		    mp_limb_t * scratch);


/* Point addition, with jacobian output, one jacobian input and one
   affine input. Corner cases: Fails for the cases

     P = Q != 0                       Duplication of non-zero point
     P = 0, Q != 0 or P != 0, Q = 0   One input zero
   
     Correctly gives R = 0 if P = Q = 0 or P = -Q. */
	 mp_size_t ecc_add_jja_itch(const struct ecc_curve *ecc);
	void
	 ecc_add_jja(const struct ecc_curve *ecc,
		     mp_limb_t * r, const mp_limb_t * p,
		     const mp_limb_t * q, mp_limb_t * scratch);

/* Point addition with Jacobian input and output. */
	 mp_size_t ecc_add_jjj_itch(const struct ecc_curve *ecc);
	void
	 ecc_add_jjj(const struct ecc_curve *ecc,
		     mp_limb_t * r, const mp_limb_t * p,
		     const mp_limb_t * q, mp_limb_t * scratch);


/* Computes N * the group generator. N is an array of ecc_size()
   limbs. It must be in the range 0 < N < group order, then R != 0,
   and the algorithm can work without any intermediate values getting
   to zero. */
	 mp_size_t ecc_mul_g_itch(const struct ecc_curve *ecc);
	void
	 ecc_mul_g(const struct ecc_curve *ecc, mp_limb_t * r,
		   const mp_limb_t * np, mp_limb_t * scratch);

/* Computes N * P. The scalar N is the same as for ecc_mul_g. P is a
   non-zero point on the curve, in affine coordinates. Pass a non-zero
   INITIAL if the point coordinates have not previously been converted
   to Montgomery representation. Output R is a non-zero point, in
   Jacobian coordinates. */
	 mp_size_t ecc_mul_a_itch(const struct ecc_curve *ecc);
	void
	 ecc_mul_a(const struct ecc_curve *ecc,
		   int initial, mp_limb_t * r,
		   const mp_limb_t * np, const mp_limb_t * p,
		   mp_limb_t * scratch);

#ifdef __cplusplus
}
#endif
#endif				/* NETTLE_ECC_H_INCLUDED */
