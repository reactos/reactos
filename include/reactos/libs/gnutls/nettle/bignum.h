/* bignum.h
 *
 * bignum operations that are missing from gmp.
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

#ifndef NETTLE_BIGNUM_H_INCLUDED
#define NETTLE_BIGNUM_H_INCLUDED

#include "nettle-meta.h"

#include <gmp.h>
#include "nettle-types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Size needed for signed encoding, including extra sign byte if
 * necessary. */
	unsigned
	 nettle_mpz_sizeinbase_256_s(const mpz_t x);

/* Size needed for unsigned encoding */
	unsigned
	 nettle_mpz_sizeinbase_256_u(const mpz_t x);

/* Writes an integer as length octets, using big endian byte order,
 * and two's complement for negative numbers. */
	void
	 nettle_mpz_get_str_256(unsigned length, uint8_t * s,
				const mpz_t x);

/* Reads a big endian, two's complement, integer. */
	void
	 nettle_mpz_set_str_256_s(mpz_t x,
				  unsigned length, const uint8_t * s);

	void
	 nettle_mpz_init_set_str_256_s(mpz_t x,
				       unsigned length, const uint8_t * s);

/* Similar, but for unsigned format. These function don't interpret
 * the most significant bit as the sign. */
	void
	 nettle_mpz_set_str_256_u(mpz_t x,
				  unsigned length, const uint8_t * s);

	void
	 nettle_mpz_init_set_str_256_u(mpz_t x,
				       unsigned length, const uint8_t * s);

/* Returns a uniformly distributed random number 0 <= x < 2^n */
	void
	 nettle_mpz_random_size(mpz_t x,
				void *ctx, nettle_random_func * random,
				unsigned bits);

/* Returns a number x, almost uniformly random in the range
 * 0 <= x < n. */
	void
	 nettle_mpz_random(mpz_t x,
			   void *ctx, nettle_random_func * random,
			   const mpz_t n);

	void
	 nettle_next_prime(mpz_t p, mpz_t n, unsigned count,
			   unsigned prime_limit, void *progress_ctx,
			   nettle_progress_func * progress);

	void
	 nettle_random_prime(mpz_t p, unsigned bits, int top_bits_set,
			     void *ctx, nettle_random_func * random,
			     void *progress_ctx,
			     nettle_progress_func * progress);

	void
	 _nettle_generate_pocklington_prime(mpz_t p, mpz_t r,
					    unsigned bits,
					    int top_bits_set, void *ctx,
					    nettle_random_func * random,
					    const mpz_t p0, const mpz_t q,
					    const mpz_t p0q);

/* sexp parsing */
	struct sexp_iterator;

/* If LIMIT is non-zero, the number must be at most LIMIT bits.
 * Implies sexp_iterator_next. */
	int
	 nettle_mpz_set_sexp(mpz_t x, unsigned limit,
			     struct sexp_iterator *i);


/* der parsing */
	struct asn1_der_iterator;

	int
	 nettle_asn1_der_get_bignum(struct asn1_der_iterator *iterator,
				    mpz_t x, unsigned max_bits);

#ifdef __cplusplus
}
#endif
#endif				/* NETTLE_BIGNUM_H_INCLUDED */
