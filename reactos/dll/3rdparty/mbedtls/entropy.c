/*
 *  Entropy accumulator implementation
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

#if defined(POLARSSL_ENTROPY_C)

#include "polarssl/entropy.h"
#include "polarssl/entropy_poll.h"

#if defined(POLARSSL_FS_IO)
#include <stdio.h>
#endif

#if defined(POLARSSL_HAVEGE_C)
#include "polarssl/havege.h"
#endif

/* Implementation that should never be optimized out by the compiler */
static void polarssl_zeroize( void *v, size_t n ) {
    volatile unsigned char *p = v; while( n-- ) *p++ = 0;
}

#define ENTROPY_MAX_LOOP    256     /**< Maximum amount to loop before error */

void entropy_init( entropy_context *ctx )
{
    memset( ctx, 0, sizeof(entropy_context) );

#if defined(POLARSSL_THREADING_C)
    polarssl_mutex_init( &ctx->mutex );
#endif

#if defined(POLARSSL_ENTROPY_SHA512_ACCUMULATOR)
    sha512_starts( &ctx->accumulator, 0 );
#else
    sha256_starts( &ctx->accumulator, 0 );
#endif
#if defined(POLARSSL_HAVEGE_C)
    havege_init( &ctx->havege_data );
#endif

#if !defined(POLARSSL_NO_DEFAULT_ENTROPY_SOURCES)
#if !defined(POLARSSL_NO_PLATFORM_ENTROPY)
    entropy_add_source( ctx, platform_entropy_poll, NULL,
                        ENTROPY_MIN_PLATFORM );
#endif
#if defined(POLARSSL_TIMING_C)
    entropy_add_source( ctx, hardclock_poll, NULL, ENTROPY_MIN_HARDCLOCK );
#endif
#if defined(POLARSSL_HAVEGE_C)
    entropy_add_source( ctx, havege_poll, &ctx->havege_data,
                        ENTROPY_MIN_HAVEGE );
#endif
#endif /* POLARSSL_NO_DEFAULT_ENTROPY_SOURCES */
}

void entropy_free( entropy_context *ctx )
{
#if defined(POLARSSL_HAVEGE_C)
    havege_free( &ctx->havege_data );
#endif
    polarssl_zeroize( ctx, sizeof( entropy_context ) );
#if defined(POLARSSL_THREADING_C)
    polarssl_mutex_free( &ctx->mutex );
#endif
}

int entropy_add_source( entropy_context *ctx,
                        f_source_ptr f_source, void *p_source,
                        size_t threshold )
{
    int index, ret = 0;

#if defined(POLARSSL_THREADING_C)
    if( ( ret = polarssl_mutex_lock( &ctx->mutex ) ) != 0 )
        return( ret );
#endif

    index = ctx->source_count;
    if( index >= ENTROPY_MAX_SOURCES )
    {
        ret = POLARSSL_ERR_ENTROPY_MAX_SOURCES;
        goto exit;
    }

    ctx->source[index].f_source = f_source;
    ctx->source[index].p_source = p_source;
    ctx->source[index].threshold = threshold;

    ctx->source_count++;

exit:
#if defined(POLARSSL_THREADING_C)
    if( polarssl_mutex_unlock( &ctx->mutex ) != 0 )
        return( POLARSSL_ERR_THREADING_MUTEX_ERROR );
#endif

    return( ret );
}

/*
 * Entropy accumulator update
 */
static int entropy_update( entropy_context *ctx, unsigned char source_id,
                           const unsigned char *data, size_t len )
{
    unsigned char header[2];
    unsigned char tmp[ENTROPY_BLOCK_SIZE];
    size_t use_len = len;
    const unsigned char *p = data;

    if( use_len > ENTROPY_BLOCK_SIZE )
    {
#if defined(POLARSSL_ENTROPY_SHA512_ACCUMULATOR)
        sha512( data, len, tmp, 0 );
#else
        sha256( data, len, tmp, 0 );
#endif
        p = tmp;
        use_len = ENTROPY_BLOCK_SIZE;
    }

    header[0] = source_id;
    header[1] = use_len & 0xFF;

#if defined(POLARSSL_ENTROPY_SHA512_ACCUMULATOR)
    sha512_update( &ctx->accumulator, header, 2 );
    sha512_update( &ctx->accumulator, p, use_len );
#else
    sha256_update( &ctx->accumulator, header, 2 );
    sha256_update( &ctx->accumulator, p, use_len );
#endif

    return( 0 );
}

int entropy_update_manual( entropy_context *ctx,
                           const unsigned char *data, size_t len )
{
    int ret;

#if defined(POLARSSL_THREADING_C)
    if( ( ret = polarssl_mutex_lock( &ctx->mutex ) ) != 0 )
        return( ret );
#endif

    ret = entropy_update( ctx, ENTROPY_SOURCE_MANUAL, data, len );

#if defined(POLARSSL_THREADING_C)
    if( polarssl_mutex_unlock( &ctx->mutex ) != 0 )
        return( POLARSSL_ERR_THREADING_MUTEX_ERROR );
#endif

    return( ret );
}

/*
 * Run through the different sources to add entropy to our accumulator
 */
static int entropy_gather_internal( entropy_context *ctx )
{
    int ret, i;
    unsigned char buf[ENTROPY_MAX_GATHER];
    size_t olen;

    if( ctx->source_count == 0 )
        return( POLARSSL_ERR_ENTROPY_NO_SOURCES_DEFINED );

    /*
     * Run through our entropy sources
     */
    for( i = 0; i < ctx->source_count; i++ )
    {
        olen = 0;
        if( ( ret = ctx->source[i].f_source( ctx->source[i].p_source,
                        buf, ENTROPY_MAX_GATHER, &olen ) ) != 0 )
        {
            return( ret );
        }

        /*
         * Add if we actually gathered something
         */
        if( olen > 0 )
        {
            entropy_update( ctx, (unsigned char) i, buf, olen );
            ctx->source[i].size += olen;
        }
    }

    return( 0 );
}

/*
 * Thread-safe wrapper for entropy_gather_internal()
 */
int entropy_gather( entropy_context *ctx )
{
    int ret;

#if defined(POLARSSL_THREADING_C)
    if( ( ret = polarssl_mutex_lock( &ctx->mutex ) ) != 0 )
        return( ret );
#endif

    ret = entropy_gather_internal( ctx );

#if defined(POLARSSL_THREADING_C)
    if( polarssl_mutex_unlock( &ctx->mutex ) != 0 )
        return( POLARSSL_ERR_THREADING_MUTEX_ERROR );
#endif

    return( ret );
}

int entropy_func( void *data, unsigned char *output, size_t len )
{
    int ret, count = 0, i, reached;
    entropy_context *ctx = (entropy_context *) data;
    unsigned char buf[ENTROPY_BLOCK_SIZE];

    if( len > ENTROPY_BLOCK_SIZE )
        return( POLARSSL_ERR_ENTROPY_SOURCE_FAILED );

#if defined(POLARSSL_THREADING_C)
    if( ( ret = polarssl_mutex_lock( &ctx->mutex ) ) != 0 )
        return( ret );
#endif

    /*
     * Always gather extra entropy before a call
     */
    do
    {
        if( count++ > ENTROPY_MAX_LOOP )
        {
            ret = POLARSSL_ERR_ENTROPY_SOURCE_FAILED;
            goto exit;
        }

        if( ( ret = entropy_gather_internal( ctx ) ) != 0 )
            goto exit;

        reached = 0;

        for( i = 0; i < ctx->source_count; i++ )
            if( ctx->source[i].size >= ctx->source[i].threshold )
                reached++;
    }
    while( reached != ctx->source_count );

    memset( buf, 0, ENTROPY_BLOCK_SIZE );

#if defined(POLARSSL_ENTROPY_SHA512_ACCUMULATOR)
    sha512_finish( &ctx->accumulator, buf );

    /*
     * Reset accumulator and counters and recycle existing entropy
     */
    memset( &ctx->accumulator, 0, sizeof( sha512_context ) );
    sha512_starts( &ctx->accumulator, 0 );
    sha512_update( &ctx->accumulator, buf, ENTROPY_BLOCK_SIZE );

    /*
     * Perform second SHA-512 on entropy
     */
    sha512( buf, ENTROPY_BLOCK_SIZE, buf, 0 );
#else /* POLARSSL_ENTROPY_SHA512_ACCUMULATOR */
    sha256_finish( &ctx->accumulator, buf );

    /*
     * Reset accumulator and counters and recycle existing entropy
     */
    memset( &ctx->accumulator, 0, sizeof( sha256_context ) );
    sha256_starts( &ctx->accumulator, 0 );
    sha256_update( &ctx->accumulator, buf, ENTROPY_BLOCK_SIZE );

    /*
     * Perform second SHA-256 on entropy
     */
    sha256( buf, ENTROPY_BLOCK_SIZE, buf, 0 );
#endif /* POLARSSL_ENTROPY_SHA512_ACCUMULATOR */

    for( i = 0; i < ctx->source_count; i++ )
        ctx->source[i].size = 0;

    memcpy( output, buf, len );

    ret = 0;

exit:
#if defined(POLARSSL_THREADING_C)
    if( polarssl_mutex_unlock( &ctx->mutex ) != 0 )
        return( POLARSSL_ERR_THREADING_MUTEX_ERROR );
#endif

    return( ret );
}

#if defined(POLARSSL_FS_IO)
int entropy_write_seed_file( entropy_context *ctx, const char *path )
{
    int ret = POLARSSL_ERR_ENTROPY_FILE_IO_ERROR;
    FILE *f;
    unsigned char buf[ENTROPY_BLOCK_SIZE];

    if( ( f = fopen( path, "wb" ) ) == NULL )
        return( POLARSSL_ERR_ENTROPY_FILE_IO_ERROR );

    if( ( ret = entropy_func( ctx, buf, ENTROPY_BLOCK_SIZE ) ) != 0 )
        goto exit;

    if( fwrite( buf, 1, ENTROPY_BLOCK_SIZE, f ) != ENTROPY_BLOCK_SIZE )
    {
        ret = POLARSSL_ERR_ENTROPY_FILE_IO_ERROR;
        goto exit;
    }

    ret = 0;

exit:
    fclose( f );
    return( ret );
}

int entropy_update_seed_file( entropy_context *ctx, const char *path )
{
    FILE *f;
    size_t n;
    unsigned char buf[ ENTROPY_MAX_SEED_SIZE ];

    if( ( f = fopen( path, "rb" ) ) == NULL )
        return( POLARSSL_ERR_ENTROPY_FILE_IO_ERROR );

    fseek( f, 0, SEEK_END );
    n = (size_t) ftell( f );
    fseek( f, 0, SEEK_SET );

    if( n > ENTROPY_MAX_SEED_SIZE )
        n = ENTROPY_MAX_SEED_SIZE;

    if( fread( buf, 1, n, f ) != n )
    {
        fclose( f );
        return( POLARSSL_ERR_ENTROPY_FILE_IO_ERROR );
    }

    fclose( f );

    entropy_update_manual( ctx, buf, n );

    return( entropy_write_seed_file( ctx, path ) );
}
#endif /* POLARSSL_FS_IO */

#if defined(POLARSSL_SELF_TEST)

#if defined(POLARSSL_PLATFORM_C)
#include "polarssl/platform.h"
#else
#include <stdio.h>
#define polarssl_printf     printf
#endif

/*
 * Dummy source function
 */
static int entropy_dummy_source( void *data, unsigned char *output,
                                 size_t len, size_t *olen )
{
    ((void) data);

    memset( output, 0x2a, len );
    *olen = len;

    return( 0 );
}

/*
 * The actual entropy quality is hard to test, but we can at least
 * test that the functions don't cause errors and write the correct
 * amount of data to buffers.
 */
int entropy_self_test( int verbose )
{
    int ret = 0;
    entropy_context ctx;
    unsigned char buf[ENTROPY_BLOCK_SIZE] = { 0 };
    unsigned char acc[ENTROPY_BLOCK_SIZE] = { 0 };
    size_t i, j;

    if( verbose != 0 )
        polarssl_printf( "  ENTROPY test: " );

    entropy_init( &ctx );

    ret = entropy_add_source( &ctx, entropy_dummy_source, NULL, 16 );
    if( ret != 0 )
        goto cleanup;

    if( ( ret = entropy_gather( &ctx ) ) != 0 )
        goto cleanup;

    if( ( ret = entropy_update_manual( &ctx, buf, sizeof buf ) ) != 0 )
        goto cleanup;

    /*
     * To test that entropy_func writes correct number of bytes:
     * - use the whole buffer and rely on ASan to detect overruns
     * - collect entropy 8 times and OR the result in an accumulator:
     *   any byte should then be 0 with probably 2^(-64), so requiring
     *   each of the 32 or 64 bytes to be non-zero has a false failure rate
     *   of at most 2^(-58) which is acceptable.
     */
    for( i = 0; i < 8; i++ )
    {
        if( ( ret = entropy_func( &ctx, buf, sizeof( buf ) ) ) != 0 )
            goto cleanup;

        for( j = 0; j < sizeof( buf ); j++ )
            acc[j] |= buf[j];
    }

    for( j = 0; j < sizeof( buf ); j++ )
    {
        if( acc[j] == 0 )
        {
            ret = 1;
            goto cleanup;
        }
    }

cleanup:
    entropy_free( &ctx );

    if( verbose != 0 )
    {
        if( ret != 0 )
            polarssl_printf( "failed\n" );
        else
            polarssl_printf( "passed\n" );

        polarssl_printf( "\n" );
    }

    return( ret != 0 );
}
#endif /* POLARSSL_SELF_TEST */

#endif /* POLARSSL_ENTROPY_C */
