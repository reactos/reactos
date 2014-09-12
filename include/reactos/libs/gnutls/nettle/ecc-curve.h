/* ecc-curve.h */

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

#ifndef NETTLE_ECC_CURVE_H_INCLUDED
#define NETTLE_ECC_CURVE_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/* The contets of this struct is internal. */
	struct ecc_curve;

	extern const struct ecc_curve nettle_secp_192r1;
	extern const struct ecc_curve nettle_secp_224r1;
	extern const struct ecc_curve nettle_secp_256r1;
	extern const struct ecc_curve nettle_secp_384r1;
	extern const struct ecc_curve nettle_secp_521r1;

#ifdef __cplusplus
}
#endif
#endif				/* NETTLE_ECC_CURVE_H_INCLUDED */
