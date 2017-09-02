/* @(#)sha3.h	1.6 16/10/26 2015-2016 J. Schilling */
/* sha3.h */
/*
 * SHA3 hash code taken from
 * https://github.com/rhash/RHash/tree/master/librhash
 *
 * Portions Copyright (c) 2015-2016 J. Schilling
 */
#ifndef	_SCHILY_SHA3_H
#define	_SCHILY_SHA3_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#include <schily/types.h>
#include <schily/stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef	HAVE_LONGLONG

#define	sha3_224_hash_size	28
#define	sha3_256_hash_size	32
#define	sha3_384_hash_size	48
#define	sha3_512_hash_size	64
#define	sha3_max_permutation_size 25
#define	sha3_max_rate_in_qwords	24

#define	SHA3_224_DIGEST_LENGTH		sha3_224_hash_size
#define	SHA3_224_DIGEST_STRING_LENGTH	(SHA3_224_DIGEST_LENGTH * 2 + 1)
#define	SHA3_256_DIGEST_LENGTH		sha3_256_hash_size
#define	SHA3_256_DIGEST_STRING_LENGTH	(SHA3_256_DIGEST_LENGTH * 2 + 1)
#define	SHA3_384_DIGEST_LENGTH		sha3_384_hash_size
#define	SHA3_384_DIGEST_STRING_LENGTH	(SHA3_384_DIGEST_LENGTH * 2 + 1)
#define	SHA3_512_DIGEST_LENGTH		sha3_512_hash_size
#define	SHA3_512_DIGEST_STRING_LENGTH	(SHA3_512_DIGEST_LENGTH * 2 + 1)

/*
 * SHA3 Algorithm context.
 */
typedef struct sha3_ctx
{
	/* 1600 bits algorithm hashing state */
	UInt64_t hash[sha3_max_permutation_size];
	/* 1536-bit buffer for leftovers */
	UInt64_t message[sha3_max_rate_in_qwords];
	/* count of bytes in the message[] buffer */
	unsigned rest;
	/* size of a message block processed at once */
	unsigned block_size;
} sha3_ctx, SHA3_CTX;

/* methods for calculating the hash function */

void rhash_sha3_224_init __PR((sha3_ctx *ctx));
void rhash_sha3_256_init __PR((sha3_ctx *ctx));
void rhash_sha3_384_init __PR((sha3_ctx *ctx));
void rhash_sha3_512_init __PR((sha3_ctx *ctx));
void rhash_sha3_update __PR((sha3_ctx *ctx,
				const unsigned char *msg,
				size_t size));
void rhash_sha3_final __PR((sha3_ctx *ctx, unsigned char *result));

void SHA3_224_Init	__PR((SHA3_CTX *ctx));
void SHA3_256_Init	__PR((SHA3_CTX *ctx));
void SHA3_384_Init	__PR((SHA3_CTX *ctx));
void SHA3_512_Init	__PR((SHA3_CTX *ctx));
void SHA3_Update	__PR((SHA3_CTX *ctx,
				const unsigned char *msg,
				size_t size));
void SHA3_Final		__PR((unsigned char *result, SHA3_CTX *ctx));

#ifdef USE_KECCAK
#define	rhash_keccak_224_init	rhash_sha3_224_init
#define	rhash_keccak_256_init	rhash_sha3_256_init
#define	rhash_keccak_384_init	rhash_sha3_384_init
#define	rhash_keccak_512_init	rhash_sha3_512_init
#define	rhash_keccak_update	rhash_sha3_update
void rhash_keccak_final	__PR((sha3_ctx *ctx, unsigned char *result));
#endif

#endif	/* HAVE_LONGLONG */

#ifdef __cplusplus
}
#endif

#endif /* _SCHILY_SHA3_H */
