/**
 * \file md.c
 *
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

#include "polarssl/md.h"
#include "polarssl/md_wrap.h"

#include <stdlib.h>

#if defined(_MSC_VER) && !defined strcasecmp && !defined(EFIX64) && \
    !defined(EFI32)
#define strcasecmp  _stricmp
#endif

/* Implementation that should never be optimized out by the compiler */
static void polarssl_zeroize( void *v, size_t n ) {
    volatile unsigned char *p = v; while( n-- ) *p++ = 0;
}

static const int supported_digests[] = {

#if defined(POLARSSL_SHA512_C)
        POLARSSL_MD_SHA512,
        POLARSSL_MD_SHA384,
#endif

#if defined(POLARSSL_SHA256_C)
        POLARSSL_MD_SHA256,
        POLARSSL_MD_SHA224,
#endif

#if defined(POLARSSL_SHA1_C)
        POLARSSL_MD_SHA1,
#endif

#if defined(POLARSSL_RIPEMD160_C)
        POLARSSL_MD_RIPEMD160,
#endif

#if defined(POLARSSL_MD5_C)
        POLARSSL_MD_MD5,
#endif

#if defined(POLARSSL_MD4_C)
        POLARSSL_MD_MD4,
#endif

#if defined(POLARSSL_MD2_C)
        POLARSSL_MD_MD2,
#endif

        POLARSSL_MD_NONE
};

const int *md_list( void )
{
    return( supported_digests );
}

const md_info_t *md_info_from_string( const char *md_name )
{
    if( NULL == md_name )
        return( NULL );

    /* Get the appropriate digest information */
#if defined(POLARSSL_MD2_C)
    if( !strcasecmp( "MD2", md_name ) )
        return md_info_from_type( POLARSSL_MD_MD2 );
#endif
#if defined(POLARSSL_MD4_C)
    if( !strcasecmp( "MD4", md_name ) )
        return md_info_from_type( POLARSSL_MD_MD4 );
#endif
#if defined(POLARSSL_MD5_C)
    if( !strcasecmp( "MD5", md_name ) )
        return md_info_from_type( POLARSSL_MD_MD5 );
#endif
#if defined(POLARSSL_RIPEMD160_C)
    if( !strcasecmp( "RIPEMD160", md_name ) )
        return md_info_from_type( POLARSSL_MD_RIPEMD160 );
#endif
#if defined(POLARSSL_SHA1_C)
    if( !strcasecmp( "SHA1", md_name ) || !strcasecmp( "SHA", md_name ) )
        return md_info_from_type( POLARSSL_MD_SHA1 );
#endif
#if defined(POLARSSL_SHA256_C)
    if( !strcasecmp( "SHA224", md_name ) )
        return md_info_from_type( POLARSSL_MD_SHA224 );
    if( !strcasecmp( "SHA256", md_name ) )
        return md_info_from_type( POLARSSL_MD_SHA256 );
#endif
#if defined(POLARSSL_SHA512_C)
    if( !strcasecmp( "SHA384", md_name ) )
        return md_info_from_type( POLARSSL_MD_SHA384 );
    if( !strcasecmp( "SHA512", md_name ) )
        return md_info_from_type( POLARSSL_MD_SHA512 );
#endif
    return( NULL );
}

const md_info_t *md_info_from_type( md_type_t md_type )
{
    switch( md_type )
    {
#if defined(POLARSSL_MD2_C)
        case POLARSSL_MD_MD2:
            return( &md2_info );
#endif
#if defined(POLARSSL_MD4_C)
        case POLARSSL_MD_MD4:
            return( &md4_info );
#endif
#if defined(POLARSSL_MD5_C)
        case POLARSSL_MD_MD5:
            return( &md5_info );
#endif
#if defined(POLARSSL_RIPEMD160_C)
        case POLARSSL_MD_RIPEMD160:
            return( &ripemd160_info );
#endif
#if defined(POLARSSL_SHA1_C)
        case POLARSSL_MD_SHA1:
            return( &sha1_info );
#endif
#if defined(POLARSSL_SHA256_C)
        case POLARSSL_MD_SHA224:
            return( &sha224_info );
        case POLARSSL_MD_SHA256:
            return( &sha256_info );
#endif
#if defined(POLARSSL_SHA512_C)
        case POLARSSL_MD_SHA384:
            return( &sha384_info );
        case POLARSSL_MD_SHA512:
            return( &sha512_info );
#endif
        default:
            return( NULL );
    }
}

void md_init( md_context_t *ctx )
{
    memset( ctx, 0, sizeof( md_context_t ) );
}

void md_free( md_context_t *ctx )
{
    if( ctx == NULL )
        return;

    if( ctx->md_ctx )
        ctx->md_info->ctx_free_func( ctx->md_ctx );

    polarssl_zeroize( ctx, sizeof( md_context_t ) );
}

int md_init_ctx( md_context_t *ctx, const md_info_t *md_info )
{
    if( md_info == NULL || ctx == NULL )
        return( POLARSSL_ERR_MD_BAD_INPUT_DATA );

    memset( ctx, 0, sizeof( md_context_t ) );

    if( ( ctx->md_ctx = md_info->ctx_alloc_func() ) == NULL )
        return( POLARSSL_ERR_MD_ALLOC_FAILED );

    ctx->md_info = md_info;

    md_info->starts_func( ctx->md_ctx );

    return( 0 );
}

int md_free_ctx( md_context_t *ctx )
{
    md_free( ctx );

    return( 0 );
}

int md_starts( md_context_t *ctx )
{
    if( ctx == NULL || ctx->md_info == NULL )
        return( POLARSSL_ERR_MD_BAD_INPUT_DATA );

    ctx->md_info->starts_func( ctx->md_ctx );

    return( 0 );
}

int md_update( md_context_t *ctx, const unsigned char *input, size_t ilen )
{
    if( ctx == NULL || ctx->md_info == NULL )
        return( POLARSSL_ERR_MD_BAD_INPUT_DATA );

    ctx->md_info->update_func( ctx->md_ctx, input, ilen );

    return( 0 );
}

int md_finish( md_context_t *ctx, unsigned char *output )
{
    if( ctx == NULL || ctx->md_info == NULL )
        return( POLARSSL_ERR_MD_BAD_INPUT_DATA );

    ctx->md_info->finish_func( ctx->md_ctx, output );

    return( 0 );
}

int md( const md_info_t *md_info, const unsigned char *input, size_t ilen,
            unsigned char *output )
{
    if( md_info == NULL )
        return( POLARSSL_ERR_MD_BAD_INPUT_DATA );

    md_info->digest_func( input, ilen, output );

    return( 0 );
}

int md_file( const md_info_t *md_info, const char *path, unsigned char *output )
{
#if defined(POLARSSL_FS_IO)
    int ret;
#endif

    if( md_info == NULL )
        return( POLARSSL_ERR_MD_BAD_INPUT_DATA );

#if defined(POLARSSL_FS_IO)
    ret = md_info->file_func( path, output );
    if( ret != 0 )
        return( POLARSSL_ERR_MD_FILE_IO_ERROR + ret );

    return( ret );
#else
    ((void) path);
    ((void) output);

    return( POLARSSL_ERR_MD_FEATURE_UNAVAILABLE );
#endif /* POLARSSL_FS_IO */
}

int md_hmac_starts( md_context_t *ctx, const unsigned char *key, size_t keylen )
{
    if( ctx == NULL || ctx->md_info == NULL )
        return( POLARSSL_ERR_MD_BAD_INPUT_DATA );

    ctx->md_info->hmac_starts_func( ctx->md_ctx, key, keylen );

    return( 0 );
}

int md_hmac_update( md_context_t *ctx, const unsigned char *input, size_t ilen )
{
    if( ctx == NULL || ctx->md_info == NULL )
        return( POLARSSL_ERR_MD_BAD_INPUT_DATA );

    ctx->md_info->hmac_update_func( ctx->md_ctx, input, ilen );

    return( 0 );
}

int md_hmac_finish( md_context_t *ctx, unsigned char *output )
{
    if( ctx == NULL || ctx->md_info == NULL )
        return( POLARSSL_ERR_MD_BAD_INPUT_DATA );

    ctx->md_info->hmac_finish_func( ctx->md_ctx, output );

    return( 0 );
}

int md_hmac_reset( md_context_t *ctx )
{
    if( ctx == NULL || ctx->md_info == NULL )
        return( POLARSSL_ERR_MD_BAD_INPUT_DATA );

    ctx->md_info->hmac_reset_func( ctx->md_ctx );

    return( 0 );
}

int md_hmac( const md_info_t *md_info, const unsigned char *key, size_t keylen,
                const unsigned char *input, size_t ilen,
                unsigned char *output )
{
    if( md_info == NULL )
        return( POLARSSL_ERR_MD_BAD_INPUT_DATA );

    md_info->hmac_func( key, keylen, input, ilen, output );

    return( 0 );
}

int md_process( md_context_t *ctx, const unsigned char *data )
{
    if( ctx == NULL || ctx->md_info == NULL )
        return( POLARSSL_ERR_MD_BAD_INPUT_DATA );

    ctx->md_info->process_func( ctx->md_ctx, data );

    return( 0 );
}

#endif /* POLARSSL_MD_C */
