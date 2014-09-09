/* dsa.h
 *
 * The DSA publickey algorithm.
 */

/* nettle, low-level cryptographics library
 *
 * Copyright (C) 2002 Niels Möller
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

#ifndef NETTLE_DSA_H_INCLUDED
#define NETTLE_DSA_H_INCLUDED

#include <gmp.h>

#include "nettle-types.h"

#include "sha1.h"
#include "sha2.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Name mangling */
#define dsa_public_key_init nettle_dsa_public_key_init
#define dsa_public_key_clear nettle_dsa_public_key_clear
#define dsa_private_key_init nettle_dsa_private_key_init
#define dsa_private_key_clear nettle_dsa_private_key_clear
#define dsa_signature_init nettle_dsa_signature_init
#define dsa_signature_clear nettle_dsa_signature_clear
#define dsa_sha1_sign nettle_dsa_sha1_sign
#define dsa_sha1_verify nettle_dsa_sha1_verify
#define dsa_sha256_sign nettle_dsa_sha256_sign
#define dsa_sha256_verify nettle_dsa_sha256_verify
#define dsa_sha1_sign_digest nettle_dsa_sha1_sign_digest
#define dsa_sha1_verify_digest nettle_dsa_sha1_verify_digest
#define dsa_sha256_sign_digest nettle_dsa_sha256_sign_digest
#define dsa_sha256_verify_digest nettle_dsa_sha256_verify_digest
#define dsa_generate_keypair nettle_dsa_generate_keypair
#define dsa_signature_from_sexp nettle_dsa_signature_from_sexp
#define dsa_keypair_to_sexp nettle_dsa_keypair_to_sexp
#define dsa_keypair_from_sexp_alist nettle_dsa_keypair_from_sexp_alist
#define dsa_sha1_keypair_from_sexp nettle_dsa_sha1_keypair_from_sexp
#define dsa_sha256_keypair_from_sexp nettle_dsa_sha256_keypair_from_sexp
#define dsa_params_from_der_iterator nettle_dsa_params_from_der_iterator
#define dsa_public_key_from_der_iterator nettle_dsa_public_key_from_der_iterator
#define dsa_openssl_private_key_from_der_iterator nettle_dsa_openssl_private_key_from_der_iterator
#define dsa_openssl_private_key_from_der nettle_openssl_provate_key_from_der
#define _dsa_sign _nettle_dsa_sign
#define _dsa_verify _nettle_dsa_verify

#define DSA_SHA1_MIN_P_BITS 512
#define DSA_SHA1_Q_OCTETS 20
#define DSA_SHA1_Q_BITS 160

#define DSA_SHA256_MIN_P_BITS 1024
#define DSA_SHA256_Q_OCTETS 32
#define DSA_SHA256_Q_BITS 256

	struct dsa_public_key {
		/* Modulo */
		mpz_t p;

		/* Group order */
		mpz_t q;

		/* Generator */
		mpz_t g;

		/* Public value */
		mpz_t y;
	};

	struct dsa_private_key {
		/* Unlike an rsa public key, private key operations will need both
		 * the private and the public information. */
		mpz_t x;
	};

	struct dsa_signature {
		mpz_t r;
		mpz_t s;
	};

/* Signing a message works as follows:
 *
 * Store the private key in a dsa_private_key struct.
 *
 * Initialize a hashing context, by callling
 *   sha1_init
 *
 * Hash the message by calling
 *   sha1_update
 *
 * Create the signature by calling
 *   dsa_sha1_sign
 *
 * The signature is represented as a struct dsa_signature. This call also
 * resets the hashing context.
 *
 * When done with the key and signature, don't forget to call
 * dsa_signature_clear.
 */

/* Calls mpz_init to initialize bignum storage. */
	void
	 dsa_public_key_init(struct dsa_public_key *key);

/* Calls mpz_clear to deallocate bignum storage. */
	void
	 dsa_public_key_clear(struct dsa_public_key *key);


/* Calls mpz_init to initialize bignum storage. */
	void
	 dsa_private_key_init(struct dsa_private_key *key);

/* Calls mpz_clear to deallocate bignum storage. */
	void
	 dsa_private_key_clear(struct dsa_private_key *key);

/* Calls mpz_init to initialize bignum storage. */
	void
	 dsa_signature_init(struct dsa_signature *signature);

/* Calls mpz_clear to deallocate bignum storage. */
	void
	 dsa_signature_clear(struct dsa_signature *signature);


	int
	 dsa_sha1_sign(const struct dsa_public_key *pub,
		       const struct dsa_private_key *key,
		       void *random_ctx, nettle_random_func * random,
		       struct sha1_ctx *hash,
		       struct dsa_signature *signature);

	int
	 dsa_sha256_sign(const struct dsa_public_key *pub,
			 const struct dsa_private_key *key,
			 void *random_ctx, nettle_random_func * random,
			 struct sha256_ctx *hash,
			 struct dsa_signature *signature);

	int
	 dsa_sha1_verify(const struct dsa_public_key *key,
			 struct sha1_ctx *hash,
			 const struct dsa_signature *signature);

	int
	 dsa_sha256_verify(const struct dsa_public_key *key,
			   struct sha256_ctx *hash,
			   const struct dsa_signature *signature);

	int
	 dsa_sha1_sign_digest(const struct dsa_public_key *pub,
			      const struct dsa_private_key *key,
			      void *random_ctx,
			      nettle_random_func * random,
			      const uint8_t * digest,
			      struct dsa_signature *signature);
	int
	 dsa_sha256_sign_digest(const struct dsa_public_key *pub,
				const struct dsa_private_key *key,
				void *random_ctx,
				nettle_random_func * random,
				const uint8_t * digest,
				struct dsa_signature *signature);

	int
	 dsa_sha1_verify_digest(const struct dsa_public_key *key,
				const uint8_t * digest,
				const struct dsa_signature *signature);

	int
	 dsa_sha256_verify_digest(const struct dsa_public_key *key,
				  const uint8_t * digest,
				  const struct dsa_signature *signature);

/* Key generation */

	int
	 dsa_generate_keypair(struct dsa_public_key *pub,
			      struct dsa_private_key *key,
			      void *random_ctx,
			      nettle_random_func * random,
			      void *progress_ctx,
			      nettle_progress_func * progress,
			      unsigned p_bits, unsigned q_bits);

/* Keys in sexp form. */

	struct nettle_buffer;

/* Generates a public-key expression if PRIV is NULL .*/
	int
	 dsa_keypair_to_sexp(struct nettle_buffer *buffer, const char *algorithm_name,	/* NULL means "dsa" */
			     const struct dsa_public_key *pub,
			     const struct dsa_private_key *priv);

	struct sexp_iterator;

	int
	 dsa_signature_from_sexp(struct dsa_signature *rs,
				 struct sexp_iterator *i, unsigned q_bits);

	int
	 dsa_keypair_from_sexp_alist(struct dsa_public_key *pub,
				     struct dsa_private_key *priv,
				     unsigned p_max_bits,
				     unsigned q_bits,
				     struct sexp_iterator *i);

/* If PRIV is NULL, expect a public-key expression. If PUB is NULL,
 * expect a private key expression and ignore the parts not needed for
 * the public key. */
/* Keys must be initialized before calling this function, as usual. */
	int
	 dsa_sha1_keypair_from_sexp(struct dsa_public_key *pub,
				    struct dsa_private_key *priv,
				    unsigned p_max_bits,
				    unsigned length, const uint8_t * expr);

	int
	 dsa_sha256_keypair_from_sexp(struct dsa_public_key *pub,
				      struct dsa_private_key *priv,
				      unsigned p_max_bits,
				      unsigned length,
				      const uint8_t * expr);

/* Keys in X.509 andd OpenSSL format. */
	struct asn1_der_iterator;

	int
	 dsa_params_from_der_iterator(struct dsa_public_key *pub,
				      unsigned p_max_bits,
				      struct asn1_der_iterator *i);
	int
	 dsa_public_key_from_der_iterator(struct dsa_public_key *pub,
					  unsigned p_max_bits,
					  struct asn1_der_iterator *i);

	int
	 dsa_openssl_private_key_from_der_iterator(struct dsa_public_key
						   *pub,
						   struct dsa_private_key
						   *priv,
						   unsigned p_max_bits,
						   struct asn1_der_iterator
						   *i);

	int
	 dsa_openssl_private_key_from_der(struct dsa_public_key *pub,
					  struct dsa_private_key *priv,
					  unsigned p_max_bits,
					  unsigned length,
					  const uint8_t * data);


/* Internal functions. */
	int
	 _dsa_sign(const struct dsa_public_key *pub,
		   const struct dsa_private_key *key,
		   void *random_ctx, nettle_random_func * random,
		   unsigned digest_size,
		   const uint8_t * digest,
		   struct dsa_signature *signature);

	int
	 _dsa_verify(const struct dsa_public_key *key,
		     unsigned digest_size,
		     const uint8_t * digest,
		     const struct dsa_signature *signature);

#ifdef __cplusplus
}
#endif
#endif				/* NETTLE_DSA_H_INCLUDED */
