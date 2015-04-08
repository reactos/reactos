/**
 * \file md_wrap.c

 * \brief Generic message digest wrapper for mbed TLS
 *
 * \author Adriaan de Jong <dejong@fox-it.com>
 *
 *  Copyright (C) 2006-2014, ARM Limited, All Rights Reserved
 *
 *  This file is part of mbed TLS (https://polarssl.org)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#if !defined(POLARSSL_CONFIG_FILE)
#include "polarssl/config.h"
#else
#include POLARSSL_CONFIG_FILE
#endif

#if defined(POLARSSL_MD_C)

#include "polarssl/md_wrap.h"

#if defined(POLARSSL_MD2_C)
#include "polarssl/md2.h"
#endif

#if defined(POLARSSL_MD4_C)
#include "polarssl/md4.h"
#endif

#if defined(POLARSSL_MD5_C)
#include "polarssl/md5.h"
#endif

#if defined(POLARSSL_RIPEMD160_C)
#include "polarssl/ripemd160.h"
#endif

#if defined(POLARSSL_SHA1_C)
#include "polarssl/sha1.h"
#endif

#if defined(POLARSSL_SHA256_C)
#include "polarssl/sha256.h"
#endif

#if defined(POLARSSL_SHA512_C)
#include "polarssl/sha512.h"
#endif

#if defined(POLARSSL_PLATFORM_C)
#include "polarssl/platform.h"
#else
#define polarssl_malloc     malloc
#define polarssl_free       free
#endif

#include <stdlib.h>

/* Implementation that should never be optimized out by the compiler */
static void polarssl_zeroize( void *v, size_t n ) {
    volatile unsigned char *p = v; while( n-- ) *p++ = 0;
}

#if defined(POLARSSL_MD2_C)

static void md2_starts_wrap( void *ctx )
{
    md2_starts( (md2_context *) ctx );
}

static void md2_update_wrap( void *ctx, const unsigned char *input,
                             size_t ilen )
{
    md2_update( (md2_context *) ctx, input, ilen );
}

static void md2_finish_wrap( void *ctx, unsigned char *output )
{
    md2_finish( (md2_context *) ctx, output );
}

static int md2_file_wrap( const char *path, unsigned char *output )
{
#if defined(POLARSSL_FS_IO)
    return md2_file( path, output );
#else
    ((void) path);
    ((void) output);
    return( POLARSSL_ERR_MD_FEATURE_UNAVAILABLE );
#endif
}

static void md2_hmac_starts_wrap( void *ctx, const unsigned char *key,
                                  size_t keylen )
{
    md2_hmac_starts( (md2_context *) ctx, key, keylen );
}

static void md2_hmac_update_wrap( void *ctx, const unsigned char *input,
                                  size_t ilen )
{
    md2_hmac_update( (md2_context *) ctx, input, ilen );
}

static void md2_hmac_finish_wrap( void *ctx, unsigned char *output )
{
    md2_hmac_finish( (md2_context *) ctx, output );
}

static void md2_hmac_reset_wrap( void *ctx )
{
    md2_hmac_reset( (md2_context *) ctx );
}

static void * md2_ctx_alloc( void )
{
    return polarssl_malloc( sizeof( md2_context ) );
}

static void md2_ctx_free( void *ctx )
{
    polarssl_zeroize( ctx, sizeof( md2_context ) );
    polarssl_free( ctx );
}

static void md2_process_wrap( void *ctx, const unsigned char *data )
{
    ((void) data);

    md2_process( (md2_context *) ctx );
}

const md_info_t md2_info = {
    POLARSSL_MD_MD2,
    "MD2",
    16,
    md2_starts_wrap,
    md2_update_wrap,
    md2_finish_wrap,
    md2,
    md2_file_wrap,
    md2_hmac_starts_wrap,
    md2_hmac_update_wrap,
    md2_hmac_finish_wrap,
    md2_hmac_reset_wrap,
    md2_hmac,
    md2_ctx_alloc,
    md2_ctx_free,
    md2_process_wrap,
};

#endif /* POLARSSL_MD2_C */

#if defined(POLARSSL_MD4_C)

static void md4_starts_wrap( void *ctx )
{
    md4_starts( (md4_context *) ctx );
}

static void md4_update_wrap( void *ctx, const unsigned char *input,
                             size_t ilen )
{
    md4_update( (md4_context *) ctx, input, ilen );
}

static void md4_finish_wrap( void *ctx, unsigned char *output )
{
    md4_finish( (md4_context *) ctx, output );
}

static int md4_file_wrap( const char *path, unsigned char *output )
{
#if defined(POLARSSL_FS_IO)
    return md4_file( path, output );
#else
    ((void) path);
    ((void) output);
    return( POLARSSL_ERR_MD_FEATURE_UNAVAILABLE );
#endif
}

static void md4_hmac_starts_wrap( void *ctx, const unsigned char *key,
                                  size_t keylen )
{
    md4_hmac_starts( (md4_context *) ctx, key, keylen );
}

static void md4_hmac_update_wrap( void *ctx, const unsigned char *input,
                                  size_t ilen )
{
    md4_hmac_update( (md4_context *) ctx, input, ilen );
}

static void md4_hmac_finish_wrap( void *ctx, unsigned char *output )
{
    md4_hmac_finish( (md4_context *) ctx, output );
}

static void md4_hmac_reset_wrap( void *ctx )
{
    md4_hmac_reset( (md4_context *) ctx );
}

static void *md4_ctx_alloc( void )
{
    return polarssl_malloc( sizeof( md4_context ) );
}

static void md4_ctx_free( void *ctx )
{
    polarssl_zeroize( ctx, sizeof( md4_context ) );
    polarssl_free( ctx );
}

static void md4_process_wrap( void *ctx, const unsigned char *data )
{
    md4_process( (md4_context *) ctx, data );
}

const md_info_t md4_info = {
    POLARSSL_MD_MD4,
    "MD4",
    16,
    md4_starts_wrap,
    md4_update_wrap,
    md4_finish_wrap,
    md4,
    md4_file_wrap,
    md4_hmac_starts_wrap,
    md4_hmac_update_wrap,
    md4_hmac_finish_wrap,
    md4_hmac_reset_wrap,
    md4_hmac,
    md4_ctx_alloc,
    md4_ctx_free,
    md4_process_wrap,
};

#endif /* POLARSSL_MD4_C */

#if defined(POLARSSL_MD5_C)

static void md5_starts_wrap( void *ctx )
{
    md5_starts( (md5_context *) ctx );
}

static void md5_update_wrap( void *ctx, const unsigned char *input,
                             size_t ilen )
{
    md5_update( (md5_context *) ctx, input, ilen );
}

static void md5_finish_wrap( void *ctx, unsigned char *output )
{
    md5_finish( (md5_context *) ctx, output );
}

static int md5_file_wrap( const char *path, unsigned char *output )
{
#if defined(POLARSSL_FS_IO)
    return md5_file( path, output );
#else
    ((void) path);
    ((void) output);
    return( POLARSSL_ERR_MD_FEATURE_UNAVAILABLE );
#endif
}

static void md5_hmac_starts_wrap( void *ctx, const unsigned char *key,
                                  size_t keylen )
{
    md5_hmac_starts( (md5_context *) ctx, key, keylen );
}

static void md5_hmac_update_wrap( void *ctx, const unsigned char *input,
                                  size_t ilen )
{
    md5_hmac_update( (md5_context *) ctx, input, ilen );
}

static void md5_hmac_finish_wrap( void *ctx, unsigned char *output )
{
    md5_hmac_finish( (md5_context *) ctx, output );
}

static void md5_hmac_reset_wrap( void *ctx )
{
    md5_hmac_reset( (md5_context *) ctx );
}

static void * md5_ctx_alloc( void )
{
    return polarssl_malloc( sizeof( md5_context ) );
}

static void md5_ctx_free( void *ctx )
{
    polarssl_zeroize( ctx, sizeof( md5_context ) );
    polarssl_free( ctx );
}

static void md5_process_wrap( void *ctx, const unsigned char *data )
{
    md5_process( (md5_context *) ctx, data );
}

const md_info_t md5_info = {
    POLARSSL_MD_MD5,
    "MD5",
    16,
    md5_starts_wrap,
    md5_update_wrap,
    md5_finish_wrap,
    md5,
    md5_file_wrap,
    md5_hmac_starts_wrap,
    md5_hmac_update_wrap,
    md5_hmac_finish_wrap,
    md5_hmac_reset_wrap,
    md5_hmac,
    md5_ctx_alloc,
    md5_ctx_free,
    md5_process_wrap,
};

#endif /* POLARSSL_MD5_C */

#if defined(POLARSSL_RIPEMD160_C)

static void ripemd160_starts_wrap( void *ctx )
{
    ripemd160_starts( (ripemd160_context *) ctx );
}

static void ripemd160_update_wrap( void *ctx, const unsigned char *input,
                                   size_t ilen )
{
    ripemd160_update( (ripemd160_context *) ctx, input, ilen );
}

static void ripemd160_finish_wrap( void *ctx, unsigned char *output )
{
    ripemd160_finish( (ripemd160_context *) ctx, output );
}

static int ripemd160_file_wrap( const char *path, unsigned char *output )
{
#if defined(POLARSSL_FS_IO)
    return ripemd160_file( path, output );
#else
    ((void) path);
    ((void) output);
    return( POLARSSL_ERR_MD_FEATURE_UNAVAILABLE );
#endif
}

static void ripemd160_hmac_starts_wrap( void *ctx, const unsigned char *key,
                                        size_t keylen )
{
    ripemd160_hmac_starts( (ripemd160_context *) ctx, key, keylen );
}

static void ripemd160_hmac_update_wrap( void *ctx, const unsigned char *input,
                                        size_t ilen )
{
    ripemd160_hmac_update( (ripemd160_context *) ctx, input, ilen );
}

static void ripemd160_hmac_finish_wrap( void *ctx, unsigned char *output )
{
    ripemd160_hmac_finish( (ripemd160_context *) ctx, output );
}

static void ripemd160_hmac_reset_wrap( void *ctx )
{
    ripemd160_hmac_reset( (ripemd160_context *) ctx );
}

static void * ripemd160_ctx_alloc( void )
{
    ripemd160_context *ctx;
    ctx = (ripemd160_context *) polarssl_malloc( sizeof( ripemd160_context ) );

    if( ctx == NULL )
        return( NULL );

    ripemd160_init( ctx );

    return( ctx );
}

static void ripemd160_ctx_free( void *ctx )
{
    ripemd160_free( (ripemd160_context *) ctx );
    polarssl_free( ctx );
}

static void ripemd160_process_wrap( void *ctx, const unsigned char *data )
{
    ripemd160_process( (ripemd160_context *) ctx, data );
}

const md_info_t ripemd160_info = {
    POLARSSL_MD_RIPEMD160,
    "RIPEMD160",
    20,
    ripemd160_starts_wrap,
    ripemd160_update_wrap,
    ripemd160_finish_wrap,
    ripemd160,
    ripemd160_file_wrap,
    ripemd160_hmac_starts_wrap,
    ripemd160_hmac_update_wrap,
    ripemd160_hmac_finish_wrap,
    ripemd160_hmac_reset_wrap,
    ripemd160_hmac,
    ripemd160_ctx_alloc,
    ripemd160_ctx_free,
    ripemd160_process_wrap,
};

#endif /* POLARSSL_RIPEMD160_C */

#if defined(POLARSSL_SHA1_C)

static void sha1_starts_wrap( void *ctx )
{
    sha1_starts( (sha1_context *) ctx );
}

static void sha1_update_wrap( void *ctx, const unsigned char *input,
                              size_t ilen )
{
    sha1_update( (sha1_context *) ctx, input, ilen );
}

static void sha1_finish_wrap( void *ctx, unsigned char *output )
{
    sha1_finish( (sha1_context *) ctx, output );
}

static int sha1_file_wrap( const char *path, unsigned char *output )
{
#if defined(POLARSSL_FS_IO)
    return sha1_file( path, output );
#else
    ((void) path);
    ((void) output);
    return( POLARSSL_ERR_MD_FEATURE_UNAVAILABLE );
#endif
}

static void sha1_hmac_starts_wrap( void *ctx, const unsigned char *key,
                                   size_t keylen )
{
    sha1_hmac_starts( (sha1_context *) ctx, key, keylen );
}

static void sha1_hmac_update_wrap( void *ctx, const unsigned char *input,
                                   size_t ilen )
{
    sha1_hmac_update( (sha1_context *) ctx, input, ilen );
}

static void sha1_hmac_finish_wrap( void *ctx, unsigned char *output )
{
    sha1_hmac_finish( (sha1_context *) ctx, output );
}

static void sha1_hmac_reset_wrap( void *ctx )
{
    sha1_hmac_reset( (sha1_context *) ctx );
}

static void * sha1_ctx_alloc( void )
{
    sha1_context *ctx;
    ctx = (sha1_context *) polarssl_malloc( sizeof( sha1_context ) );

    if( ctx == NULL )
        return( NULL );

    sha1_init( ctx );

    return( ctx );
}

static void sha1_ctx_free( void *ctx )
{
    sha1_free( (sha1_context *) ctx );
    polarssl_free( ctx );
}

static void sha1_process_wrap( void *ctx, const unsigned char *data )
{
    sha1_process( (sha1_context *) ctx, data );
}

const md_info_t sha1_info = {
    POLARSSL_MD_SHA1,
    "SHA1",
    20,
    sha1_starts_wrap,
    sha1_update_wrap,
    sha1_finish_wrap,
    sha1,
    sha1_file_wrap,
    sha1_hmac_starts_wrap,
    sha1_hmac_update_wrap,
    sha1_hmac_finish_wrap,
    sha1_hmac_reset_wrap,
    sha1_hmac,
    sha1_ctx_alloc,
    sha1_ctx_free,
    sha1_process_wrap,
};

#endif /* POLARSSL_SHA1_C */

/*
 * Wrappers for generic message digests
 */
#if defined(POLARSSL_SHA256_C)

static void sha224_starts_wrap( void *ctx )
{
    sha256_starts( (sha256_context *) ctx, 1 );
}

static void sha224_update_wrap( void *ctx, const unsigned char *input,
                                size_t ilen )
{
    sha256_update( (sha256_context *) ctx, input, ilen );
}

static void sha224_finish_wrap( void *ctx, unsigned char *output )
{
    sha256_finish( (sha256_context *) ctx, output );
}

static void sha224_wrap( const unsigned char *input, size_t ilen,
                    unsigned char *output )
{
    sha256( input, ilen, output, 1 );
}

static int sha224_file_wrap( const char *path, unsigned char *output )
{
#if defined(POLARSSL_FS_IO)
    return sha256_file( path, output, 1 );
#else
    ((void) path);
    ((void) output);
    return( POLARSSL_ERR_MD_FEATURE_UNAVAILABLE );
#endif
}

static void sha224_hmac_starts_wrap( void *ctx, const unsigned char *key,
                                     size_t keylen )
{
    sha256_hmac_starts( (sha256_context *) ctx, key, keylen, 1 );
}

static void sha224_hmac_update_wrap( void *ctx, const unsigned char *input,
                                     size_t ilen )
{
    sha256_hmac_update( (sha256_context *) ctx, input, ilen );
}

static void sha224_hmac_finish_wrap( void *ctx, unsigned char *output )
{
    sha256_hmac_finish( (sha256_context *) ctx, output );
}

static void sha224_hmac_reset_wrap( void *ctx )
{
    sha256_hmac_reset( (sha256_context *) ctx );
}

static void sha224_hmac_wrap( const unsigned char *key, size_t keylen,
        const unsigned char *input, size_t ilen,
        unsigned char *output )
{
    sha256_hmac( key, keylen, input, ilen, output, 1 );
}

static void * sha224_ctx_alloc( void )
{
    return polarssl_malloc( sizeof( sha256_context ) );
}

static void sha224_ctx_free( void *ctx )
{
    polarssl_zeroize( ctx, sizeof( sha256_context ) );
    polarssl_free( ctx );
}

static void sha224_process_wrap( void *ctx, const unsigned char *data )
{
    sha256_process( (sha256_context *) ctx, data );
}

const md_info_t sha224_info = {
    POLARSSL_MD_SHA224,
    "SHA224",
    28,
    sha224_starts_wrap,
    sha224_update_wrap,
    sha224_finish_wrap,
    sha224_wrap,
    sha224_file_wrap,
    sha224_hmac_starts_wrap,
    sha224_hmac_update_wrap,
    sha224_hmac_finish_wrap,
    sha224_hmac_reset_wrap,
    sha224_hmac_wrap,
    sha224_ctx_alloc,
    sha224_ctx_free,
    sha224_process_wrap,
};

static void sha256_starts_wrap( void *ctx )
{
    sha256_starts( (sha256_context *) ctx, 0 );
}

static void sha256_update_wrap( void *ctx, const unsigned char *input,
                                size_t ilen )
{
    sha256_update( (sha256_context *) ctx, input, ilen );
}

static void sha256_finish_wrap( void *ctx, unsigned char *output )
{
    sha256_finish( (sha256_context *) ctx, output );
}

static void sha256_wrap( const unsigned char *input, size_t ilen,
                    unsigned char *output )
{
    sha256( input, ilen, output, 0 );
}

static int sha256_file_wrap( const char *path, unsigned char *output )
{
#if defined(POLARSSL_FS_IO)
    return sha256_file( path, output, 0 );
#else
    ((void) path);
    ((void) output);
    return( POLARSSL_ERR_MD_FEATURE_UNAVAILABLE );
#endif
}

static void sha256_hmac_starts_wrap( void *ctx, const unsigned char *key,
                                     size_t keylen )
{
    sha256_hmac_starts( (sha256_context *) ctx, key, keylen, 0 );
}

static void sha256_hmac_update_wrap( void *ctx, const unsigned char *input,
                                     size_t ilen )
{
    sha256_hmac_update( (sha256_context *) ctx, input, ilen );
}

static void sha256_hmac_finish_wrap( void *ctx, unsigned char *output )
{
    sha256_hmac_finish( (sha256_context *) ctx, output );
}

static void sha256_hmac_reset_wrap( void *ctx )
{
    sha256_hmac_reset( (sha256_context *) ctx );
}

static void sha256_hmac_wrap( const unsigned char *key, size_t keylen,
        const unsigned char *input, size_t ilen,
        unsigned char *output )
{
    sha256_hmac( key, keylen, input, ilen, output, 0 );
}

static void * sha256_ctx_alloc( void )
{
    sha256_context *ctx;
    ctx = (sha256_context *) polarssl_malloc( sizeof( sha256_context ) );

    if( ctx == NULL )
        return( NULL );

    sha256_init( ctx );

    return( ctx );
}

static void sha256_ctx_free( void *ctx )
{
    sha256_free( (sha256_context *) ctx );
    polarssl_free( ctx );
}

static void sha256_process_wrap( void *ctx, const unsigned char *data )
{
    sha256_process( (sha256_context *) ctx, data );
}

const md_info_t sha256_info = {
    POLARSSL_MD_SHA256,
    "SHA256",
    32,
    sha256_starts_wrap,
    sha256_update_wrap,
    sha256_finish_wrap,
    sha256_wrap,
    sha256_file_wrap,
    sha256_hmac_starts_wrap,
    sha256_hmac_update_wrap,
    sha256_hmac_finish_wrap,
    sha256_hmac_reset_wrap,
    sha256_hmac_wrap,
    sha256_ctx_alloc,
    sha256_ctx_free,
    sha256_process_wrap,
};

#endif /* POLARSSL_SHA256_C */

#if defined(POLARSSL_SHA512_C)

static void sha384_starts_wrap( void *ctx )
{
    sha512_starts( (sha512_context *) ctx, 1 );
}

static void sha384_update_wrap( void *ctx, const unsigned char *input,
                                size_t ilen )
{
    sha512_update( (sha512_context *) ctx, input, ilen );
}

static void sha384_finish_wrap( void *ctx, unsigned char *output )
{
    sha512_finish( (sha512_context *) ctx, output );
}

static void sha384_wrap( const unsigned char *input, size_t ilen,
                    unsigned char *output )
{
    sha512( input, ilen, output, 1 );
}

static int sha384_file_wrap( const char *path, unsigned char *output )
{
#if defined(POLARSSL_FS_IO)
    return sha512_file( path, output, 1 );
#else
    ((void) path);
    ((void) output);
    return( POLARSSL_ERR_MD_FEATURE_UNAVAILABLE );
#endif
}

static void sha384_hmac_starts_wrap( void *ctx, const unsigned char *key,
                                     size_t keylen )
{
    sha512_hmac_starts( (sha512_context *) ctx, key, keylen, 1 );
}

static void sha384_hmac_update_wrap( void *ctx, const unsigned char *input,
                                     size_t ilen )
{
    sha512_hmac_update( (sha512_context *) ctx, input, ilen );
}

static void sha384_hmac_finish_wrap( void *ctx, unsigned char *output )
{
    sha512_hmac_finish( (sha512_context *) ctx, output );
}

static void sha384_hmac_reset_wrap( void *ctx )
{
    sha512_hmac_reset( (sha512_context *) ctx );
}

static void sha384_hmac_wrap( const unsigned char *key, size_t keylen,
        const unsigned char *input, size_t ilen,
        unsigned char *output )
{
    sha512_hmac( key, keylen, input, ilen, output, 1 );
}

static void * sha384_ctx_alloc( void )
{
    return polarssl_malloc( sizeof( sha512_context ) );
}

static void sha384_ctx_free( void *ctx )
{
    polarssl_zeroize( ctx, sizeof( sha512_context ) );
    polarssl_free( ctx );
}

static void sha384_process_wrap( void *ctx, const unsigned char *data )
{
    sha512_process( (sha512_context *) ctx, data );
}

const md_info_t sha384_info = {
    POLARSSL_MD_SHA384,
    "SHA384",
    48,
    sha384_starts_wrap,
    sha384_update_wrap,
    sha384_finish_wrap,
    sha384_wrap,
    sha384_file_wrap,
    sha384_hmac_starts_wrap,
    sha384_hmac_update_wrap,
    sha384_hmac_finish_wrap,
    sha384_hmac_reset_wrap,
    sha384_hmac_wrap,
    sha384_ctx_alloc,
    sha384_ctx_free,
    sha384_process_wrap,
};

static void sha512_starts_wrap( void *ctx )
{
    sha512_starts( (sha512_context *) ctx, 0 );
}

static void sha512_update_wrap( void *ctx, const unsigned char *input,
                                size_t ilen )
{
    sha512_update( (sha512_context *) ctx, input, ilen );
}

static void sha512_finish_wrap( void *ctx, unsigned char *output )
{
    sha512_finish( (sha512_context *) ctx, output );
}

static void sha512_wrap( const unsigned char *input, size_t ilen,
                    unsigned char *output )
{
    sha512( input, ilen, output, 0 );
}

static int sha512_file_wrap( const char *path, unsigned char *output )
{
#if defined(POLARSSL_FS_IO)
    return sha512_file( path, output, 0 );
#else
    ((void) path);
    ((void) output);
    return( POLARSSL_ERR_MD_FEATURE_UNAVAILABLE );
#endif
}

static void sha512_hmac_starts_wrap( void *ctx, const unsigned char *key,
                                     size_t keylen )
{
    sha512_hmac_starts( (sha512_context *) ctx, key, keylen, 0 );
}

static void sha512_hmac_update_wrap( void *ctx, const unsigned char *input,
                                     size_t ilen )
{
    sha512_hmac_update( (sha512_context *) ctx, input, ilen );
}

static void sha512_hmac_finish_wrap( void *ctx, unsigned char *output )
{
    sha512_hmac_finish( (sha512_context *) ctx, output );
}

static void sha512_hmac_reset_wrap( void *ctx )
{
    sha512_hmac_reset( (sha512_context *) ctx );
}

static void sha512_hmac_wrap( const unsigned char *key, size_t keylen,
        const unsigned char *input, size_t ilen,
        unsigned char *output )
{
    sha512_hmac( key, keylen, input, ilen, output, 0 );
}

static void * sha512_ctx_alloc( void )
{
    sha512_context *ctx;
    ctx = (sha512_context *) polarssl_malloc( sizeof( sha512_context ) );

    if( ctx == NULL )
        return( NULL );

    sha512_init( ctx );

    return( ctx );
}

static void sha512_ctx_free( void *ctx )
{
    sha512_free( (sha512_context *) ctx );
    polarssl_free( ctx );
}

static void sha512_process_wrap( void *ctx, const unsigned char *data )
{
    sha512_process( (sha512_context *) ctx, data );
}

const md_info_t sha512_info = {
    POLARSSL_MD_SHA512,
    "SHA512",
    64,
    sha512_starts_wrap,
    sha512_update_wrap,
    sha512_finish_wrap,
    sha512_wrap,
    sha512_file_wrap,
    sha512_hmac_starts_wrap,
    sha512_hmac_update_wrap,
    sha512_hmac_finish_wrap,
    sha512_hmac_reset_wrap,
    sha512_hmac_wrap,
    sha512_ctx_alloc,
    sha512_ctx_free,
    sha512_process_wrap,
};

#endif /* POLARSSL_SHA512_C */

#endif /* POLARSSL_MD_C */
