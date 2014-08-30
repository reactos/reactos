/* rsa-compat.h
 *
 * The RSA publickey algorithm, RSAREF compatible interface.
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

#ifndef NETTLE_RSA_COMPAT_H_INCLUDED
#define NETTLE_RSA_COMPAT_H_INCLUDED

#include "rsa.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Name mangling */
#define R_SignInit nettle_R_SignInit
#define R_SignUpdate nettle_R_SignUpdate
#define R_SignFinal nettle_R_SignFinal
#define R_VerifyInit nettle_R_VerifyInit
#define R_VerifyUpdate nettle_R_VerifyUpdate
#define R_VerifyFinal nettle_R_VerifyFinal

/* 256 octets or 2048 bits */
#define MAX_RSA_MODULUS_LEN 256

	typedef struct {
		unsigned bits;
		uint8_t modulus[MAX_RSA_MODULUS_LEN];
		uint8_t exponent[MAX_RSA_MODULUS_LEN];
	} R_RSA_PUBLIC_KEY;

	typedef struct {
		unsigned bits;
		uint8_t modulus[MAX_RSA_MODULUS_LEN];
		uint8_t publicExponent[MAX_RSA_MODULUS_LEN];
		uint8_t exponent[MAX_RSA_MODULUS_LEN];
		uint8_t prime[2][MAX_RSA_MODULUS_LEN];
		uint8_t primeExponent[2][MAX_RSA_MODULUS_LEN];
		uint8_t coefficient[MAX_RSA_MODULUS_LEN];
	} R_RSA_PRIVATE_KEY;

/* Only MD5 is supported for now */
	typedef struct {
		struct md5_ctx hash;
	} R_SIGNATURE_CTX;

/* Digest algorithms */
/* DA_MD2 not implemented */
	enum { DA_MD5 = 1 };

/* Return values */
	enum {
		RE_SUCCESS = 0,
		RE_CONTENT_ENCODING,	/* encryptedContent has RFC 1421 encoding error */
		RE_DATA,	/* other party's private value out of range */
		RE_DIGEST_ALGORITHM,	/* message-digest algorithm is invalid */
		RE_ENCODING,	/* encoded block has RFC 1421 encoding error */
		RE_ENCRYPTION_ALGORITHM,	/* encryption algorithm is invalid */
		RE_KEY,		/* recovered data encryption key cannot decrypt */
		RE_KEY_ENCODING,	/* encrypted key has RFC 1421 encoding error */
		RE_LEN,		/* signatureLen out of range */
		RE_MODULUS_LEN,	/* modulus length invalid */
		RE_NEED_RANDOM,	/* random structure is not seeded */
		RE_PRIVATE_KEY,	/* private key cannot encrypt message digest, */
		RE_PUBLIC_KEY,	/* publicKey cannot decrypt signature */
		RE_SIGNATURE,	/* signature is incorrect */
		RE_SIGNATURE_ENCODING,	/* encodedSignature has RFC 1421 encoding error */
	};

	int
	 R_SignInit(R_SIGNATURE_CTX * ctx, int digestAlgorithm);

	int
	 R_SignUpdate(R_SIGNATURE_CTX * ctx, const uint8_t * data,
		      /* Length is an unsigned char according to rsaref.txt,
		       * but that must be a typo. */
		      unsigned length);

	int
	 R_SignFinal(R_SIGNATURE_CTX * ctx,
		     uint8_t * signature,
		     unsigned *length, R_RSA_PRIVATE_KEY * key);

	int
	 R_VerifyInit(R_SIGNATURE_CTX * ctx, int digestAlgorithm);

	int
	 R_VerifyUpdate(R_SIGNATURE_CTX * ctx, const uint8_t * data,
			/* Length is an unsigned char according to rsaref.txt,
			 * but that must be a typo. */
			unsigned length);

	int
	 R_VerifyFinal(R_SIGNATURE_CTX * ctx,
		       uint8_t * signature,
		       unsigned length, R_RSA_PUBLIC_KEY * key);

#ifdef __cplusplus
}
#endif
#endif				/* NETTLE_RSA_COMPAT_H_INCLUDED */
