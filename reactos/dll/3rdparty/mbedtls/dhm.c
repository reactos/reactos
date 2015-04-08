/*
 *  Diffie-Hellman-Merkle key exchange
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
/*
 *  Reference:
 *
 *  http://www.cacr.math.uwaterloo.ca/hac/ (chapter 12)
 */

#if !defined(POLARSSL_CONFIG_FILE)
#include "polarssl/config.h"
#else
#include POLARSSL_CONFIG_FILE
#endif

#if defined(POLARSSL_DHM_C)

#include "polarssl/dhm.h"

#if defined(POLARSSL_PEM_PARSE_C)
#include "polarssl/pem.h"
#endif

#if defined(POLARSSL_ASN1_PARSE_C)
#include "polarssl/asn1.h"
#endif

#if defined(POLARSSL_PLATFORM_C)
#include "polarssl/platform.h"
#else
#include <stdlib.h>
#define polarssl_printf     printf
#define polarssl_malloc     malloc
#define polarssl_free       free
#endif

/* Implementation that should never be optimized out by the compiler */
static void polarssl_zeroize( void *v, size_t n ) {
    volatile unsigned char *p = v; while( n-- ) *p++ = 0;
}

/*
 * helper to validate the mpi size and import it
 */
static int dhm_read_bignum( mpi *X,
                            unsigned char **p,
                            const unsigned char *end )
{
    int ret, n;

    if( end - *p < 2 )
        return( POLARSSL_ERR_DHM_BAD_INPUT_DATA );

    n = ( (*p)[0] << 8 ) | (*p)[1];
    (*p) += 2;

    if( (int)( end - *p ) < n )
        return( POLARSSL_ERR_DHM_BAD_INPUT_DATA );

    if( ( ret = mpi_read_binary( X, *p, n ) ) != 0 )
        return( POLARSSL_ERR_DHM_READ_PARAMS_FAILED + ret );

    (*p) += n;

    return( 0 );
}

/*
 * Verify sanity of parameter with regards to P
 *
 * Parameter should be: 2 <= public_param <= P - 2
 *
 * For more information on the attack, see:
 *  http://www.cl.cam.ac.uk/~rja14/Papers/psandqs.pdf
 *  http://web.nvd.nist.gov/view/vuln/detail?vulnId=CVE-2005-2643
 */
static int dhm_check_range( const mpi *param, const mpi *P )
{
    mpi L, U;
    int ret = POLARSSL_ERR_DHM_BAD_INPUT_DATA;

    mpi_init( &L ); mpi_init( &U );

    MPI_CHK( mpi_lset( &L, 2 ) );
    MPI_CHK( mpi_sub_int( &U, P, 2 ) );

    if( mpi_cmp_mpi( param, &L ) >= 0 &&
        mpi_cmp_mpi( param, &U ) <= 0 )
    {
        ret = 0;
    }

cleanup:
    mpi_free( &L ); mpi_free( &U );
    return( ret );
}

void dhm_init( dhm_context *ctx )
{
    memset( ctx, 0, sizeof( dhm_context ) );
}

/*
 * Parse the ServerKeyExchange parameters
 */
int dhm_read_params( dhm_context *ctx,
                     unsigned char **p,
                     const unsigned char *end )
{
    int ret;

    if( ( ret = dhm_read_bignum( &ctx->P,  p, end ) ) != 0 ||
        ( ret = dhm_read_bignum( &ctx->G,  p, end ) ) != 0 ||
        ( ret = dhm_read_bignum( &ctx->GY, p, end ) ) != 0 )
        return( ret );

    if( ( ret = dhm_check_range( &ctx->GY, &ctx->P ) ) != 0 )
        return( ret );

    ctx->len = mpi_size( &ctx->P );

    return( 0 );
}

/*
 * Setup and write the ServerKeyExchange parameters
 */
int dhm_make_params( dhm_context *ctx, int x_size,
                     unsigned char *output, size_t *olen,
                     int (*f_rng)(void *, unsigned char *, size_t),
                     void *p_rng )
{
    int ret, count = 0;
    size_t n1, n2, n3;
    unsigned char *p;

    if( mpi_cmp_int( &ctx->P, 0 ) == 0 )
        return( POLARSSL_ERR_DHM_BAD_INPUT_DATA );

    /*
     * Generate X as large as possible ( < P )
     */
    do
    {
        mpi_fill_random( &ctx->X, x_size, f_rng, p_rng );

        while( mpi_cmp_mpi( &ctx->X, &ctx->P ) >= 0 )
            MPI_CHK( mpi_shift_r( &ctx->X, 1 ) );

        if( count++ > 10 )
            return( POLARSSL_ERR_DHM_MAKE_PARAMS_FAILED );
    }
    while( dhm_check_range( &ctx->X, &ctx->P ) != 0 );

    /*
     * Calculate GX = G^X mod P
     */
    MPI_CHK( mpi_exp_mod( &ctx->GX, &ctx->G, &ctx->X,
                          &ctx->P , &ctx->RP ) );

    if( ( ret = dhm_check_range( &ctx->GX, &ctx->P ) ) != 0 )
        return( ret );

    /*
     * export P, G, GX
     */
#define DHM_MPI_EXPORT(X,n)                     \
    MPI_CHK( mpi_write_binary( X, p + 2, n ) ); \
    *p++ = (unsigned char)( n >> 8 );           \
    *p++ = (unsigned char)( n      ); p += n;

    n1 = mpi_size( &ctx->P  );
    n2 = mpi_size( &ctx->G  );
    n3 = mpi_size( &ctx->GX );

    p = output;
    DHM_MPI_EXPORT( &ctx->P , n1 );
    DHM_MPI_EXPORT( &ctx->G , n2 );
    DHM_MPI_EXPORT( &ctx->GX, n3 );

    *olen  = p - output;

    ctx->len = n1;

cleanup:

    if( ret != 0 )
        return( POLARSSL_ERR_DHM_MAKE_PARAMS_FAILED + ret );

    return( 0 );
}

/*
 * Import the peer's public value G^Y
 */
int dhm_read_public( dhm_context *ctx,
                     const unsigned char *input, size_t ilen )
{
    int ret;

    if( ctx == NULL || ilen < 1 || ilen > ctx->len )
        return( POLARSSL_ERR_DHM_BAD_INPUT_DATA );

    if( ( ret = mpi_read_binary( &ctx->GY, input, ilen ) ) != 0 )
        return( POLARSSL_ERR_DHM_READ_PUBLIC_FAILED + ret );

    return( 0 );
}

/*
 * Create own private value X and export G^X
 */
int dhm_make_public( dhm_context *ctx, int x_size,
                     unsigned char *output, size_t olen,
                     int (*f_rng)(void *, unsigned char *, size_t),
                     void *p_rng )
{
    int ret, count = 0;

    if( ctx == NULL || olen < 1 || olen > ctx->len )
        return( POLARSSL_ERR_DHM_BAD_INPUT_DATA );

    if( mpi_cmp_int( &ctx->P, 0 ) == 0 )
        return( POLARSSL_ERR_DHM_BAD_INPUT_DATA );

    /*
     * generate X and calculate GX = G^X mod P
     */
    do
    {
        mpi_fill_random( &ctx->X, x_size, f_rng, p_rng );

        while( mpi_cmp_mpi( &ctx->X, &ctx->P ) >= 0 )
            MPI_CHK( mpi_shift_r( &ctx->X, 1 ) );

        if( count++ > 10 )
            return( POLARSSL_ERR_DHM_MAKE_PUBLIC_FAILED );
    }
    while( dhm_check_range( &ctx->X, &ctx->P ) != 0 );

    MPI_CHK( mpi_exp_mod( &ctx->GX, &ctx->G, &ctx->X,
                          &ctx->P , &ctx->RP ) );

    if( ( ret = dhm_check_range( &ctx->GX, &ctx->P ) ) != 0 )
        return( ret );

    MPI_CHK( mpi_write_binary( &ctx->GX, output, olen ) );

cleanup:

    if( ret != 0 )
        return( POLARSSL_ERR_DHM_MAKE_PUBLIC_FAILED + ret );

    return( 0 );
}

/*
 * Use the blinding method and optimisation suggested in section 10 of:
 *  KOCHER, Paul C. Timing attacks on implementations of Diffie-Hellman, RSA,
 *  DSS, and other systems. In : Advances in Cryptology—CRYPTO’96. Springer
 *  Berlin Heidelberg, 1996. p. 104-113.
 */
static int dhm_update_blinding( dhm_context *ctx,
                    int (*f_rng)(void *, unsigned char *, size_t), void *p_rng )
{
    int ret, count;

    /*
     * Don't use any blinding the first time a particular X is used,
     * but remember it to use blinding next time.
     */
    if( mpi_cmp_mpi( &ctx->X, &ctx->pX ) != 0 )
    {
        MPI_CHK( mpi_copy( &ctx->pX, &ctx->X ) );
        MPI_CHK( mpi_lset( &ctx->Vi, 1 ) );
        MPI_CHK( mpi_lset( &ctx->Vf, 1 ) );

        return( 0 );
    }

    /*
     * Ok, we need blinding. Can we re-use existing values?
     * If yes, just update them by squaring them.
     */
    if( mpi_cmp_int( &ctx->Vi, 1 ) != 0 )
    {
        MPI_CHK( mpi_mul_mpi( &ctx->Vi, &ctx->Vi, &ctx->Vi ) );
        MPI_CHK( mpi_mod_mpi( &ctx->Vi, &ctx->Vi, &ctx->P ) );

        MPI_CHK( mpi_mul_mpi( &ctx->Vf, &ctx->Vf, &ctx->Vf ) );
        MPI_CHK( mpi_mod_mpi( &ctx->Vf, &ctx->Vf, &ctx->P ) );

        return( 0 );
    }

    /*
     * We need to generate blinding values from scratch
     */

    /* Vi = random( 2, P-1 ) */
    count = 0;
    do
    {
        mpi_fill_random( &ctx->Vi, mpi_size( &ctx->P ), f_rng, p_rng );

        while( mpi_cmp_mpi( &ctx->Vi, &ctx->P ) >= 0 )
            MPI_CHK( mpi_shift_r( &ctx->Vi, 1 ) );

        if( count++ > 10 )
            return( POLARSSL_ERR_MPI_NOT_ACCEPTABLE );
    }
    while( mpi_cmp_int( &ctx->Vi, 1 ) <= 0 );

    /* Vf = Vi^-X mod P */
    MPI_CHK( mpi_inv_mod( &ctx->Vf, &ctx->Vi, &ctx->P ) );
    MPI_CHK( mpi_exp_mod( &ctx->Vf, &ctx->Vf, &ctx->X, &ctx->P, &ctx->RP ) );

cleanup:
    return( ret );
}

/*
 * Derive and export the shared secret (G^Y)^X mod P
 */
int dhm_calc_secret( dhm_context *ctx,
                     unsigned char *output, size_t *olen,
                     int (*f_rng)(void *, unsigned char *, size_t),
                     void *p_rng )
{
    int ret;
    mpi GYb;

    if( ctx == NULL || *olen < ctx->len )
        return( POLARSSL_ERR_DHM_BAD_INPUT_DATA );

    if( ( ret = dhm_check_range( &ctx->GY, &ctx->P ) ) != 0 )
        return( ret );

    mpi_init( &GYb );

    /* Blind peer's value */
    if( f_rng != NULL )
    {
        MPI_CHK( dhm_update_blinding( ctx, f_rng, p_rng ) );
        MPI_CHK( mpi_mul_mpi( &GYb, &ctx->GY, &ctx->Vi ) );
        MPI_CHK( mpi_mod_mpi( &GYb, &GYb, &ctx->P ) );
    }
    else
        MPI_CHK( mpi_copy( &GYb, &ctx->GY ) );

    /* Do modular exponentiation */
    MPI_CHK( mpi_exp_mod( &ctx->K, &GYb, &ctx->X,
                          &ctx->P, &ctx->RP ) );

    /* Unblind secret value */
    if( f_rng != NULL )
    {
        MPI_CHK( mpi_mul_mpi( &ctx->K, &ctx->K, &ctx->Vf ) );
        MPI_CHK( mpi_mod_mpi( &ctx->K, &ctx->K, &ctx->P ) );
    }

    *olen = mpi_size( &ctx->K );

    MPI_CHK( mpi_write_binary( &ctx->K, output, *olen ) );

cleanup:
    mpi_free( &GYb );

    if( ret != 0 )
        return( POLARSSL_ERR_DHM_CALC_SECRET_FAILED + ret );

    return( 0 );
}

/*
 * Free the components of a DHM key
 */
void dhm_free( dhm_context *ctx )
{
    mpi_free( &ctx->pX); mpi_free( &ctx->Vf ); mpi_free( &ctx->Vi );
    mpi_free( &ctx->RP ); mpi_free( &ctx->K ); mpi_free( &ctx->GY );
    mpi_free( &ctx->GX ); mpi_free( &ctx->X ); mpi_free( &ctx->G );
    mpi_free( &ctx->P );

    polarssl_zeroize( ctx, sizeof( dhm_context ) );
}

#if defined(POLARSSL_ASN1_PARSE_C)
/*
 * Parse DHM parameters
 */
int dhm_parse_dhm( dhm_context *dhm, const unsigned char *dhmin,
                   size_t dhminlen )
{
    int ret;
    size_t len;
    unsigned char *p, *end;
#if defined(POLARSSL_PEM_PARSE_C)
    pem_context pem;

    pem_init( &pem );

    ret = pem_read_buffer( &pem,
                           "-----BEGIN DH PARAMETERS-----",
                           "-----END DH PARAMETERS-----",
                           dhmin, NULL, 0, &dhminlen );

    if( ret == 0 )
    {
        /*
         * Was PEM encoded
         */
        dhminlen = pem.buflen;
    }
    else if( ret != POLARSSL_ERR_PEM_NO_HEADER_FOOTER_PRESENT )
        goto exit;

    p = ( ret == 0 ) ? pem.buf : (unsigned char *) dhmin;
#else
    p = (unsigned char *) dhmin;
#endif /* POLARSSL_PEM_PARSE_C */
    end = p + dhminlen;

    /*
     *  DHParams ::= SEQUENCE {
     *      prime            INTEGER,  -- P
     *      generator        INTEGER,  -- g
     *  }
     */
    if( ( ret = asn1_get_tag( &p, end, &len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
    {
        ret = POLARSSL_ERR_DHM_INVALID_FORMAT + ret;
        goto exit;
    }

    end = p + len;

    if( ( ret = asn1_get_mpi( &p, end, &dhm->P  ) ) != 0 ||
        ( ret = asn1_get_mpi( &p, end, &dhm->G ) ) != 0 )
    {
        ret = POLARSSL_ERR_DHM_INVALID_FORMAT + ret;
        goto exit;
    }

    if( p != end )
    {
        ret = POLARSSL_ERR_DHM_INVALID_FORMAT +
              POLARSSL_ERR_ASN1_LENGTH_MISMATCH;
        goto exit;
    }

    ret = 0;

    dhm->len = mpi_size( &dhm->P );

exit:
#if defined(POLARSSL_PEM_PARSE_C)
    pem_free( &pem );
#endif
    if( ret != 0 )
        dhm_free( dhm );

    return( ret );
}

#if defined(POLARSSL_FS_IO)
/*
 * Load all data from a file into a given buffer.
 */
static int load_file( const char *path, unsigned char **buf, size_t *n )
{
    FILE *f;
    long size;

    if( ( f = fopen( path, "rb" ) ) == NULL )
        return( POLARSSL_ERR_DHM_FILE_IO_ERROR );

    fseek( f, 0, SEEK_END );
    if( ( size = ftell( f ) ) == -1 )
    {
        fclose( f );
        return( POLARSSL_ERR_DHM_FILE_IO_ERROR );
    }
    fseek( f, 0, SEEK_SET );

    *n = (size_t) size;

    if( *n + 1 == 0 ||
        ( *buf = (unsigned char *) polarssl_malloc( *n + 1 ) ) == NULL )
    {
        fclose( f );
        return( POLARSSL_ERR_DHM_MALLOC_FAILED );
    }

    if( fread( *buf, 1, *n, f ) != *n )
    {
        fclose( f );
        polarssl_free( *buf );
        return( POLARSSL_ERR_DHM_FILE_IO_ERROR );
    }

    fclose( f );

    (*buf)[*n] = '\0';

    return( 0 );
}

/*
 * Load and parse DHM parameters
 */
int dhm_parse_dhmfile( dhm_context *dhm, const char *path )
{
    int ret;
    size_t n;
    unsigned char *buf;

    if( ( ret = load_file( path, &buf, &n ) ) != 0 )
        return( ret );

    ret = dhm_parse_dhm( dhm, buf, n );

    polarssl_zeroize( buf, n + 1 );
    polarssl_free( buf );

    return( ret );
}
#endif /* POLARSSL_FS_IO */
#endif /* POLARSSL_ASN1_PARSE_C */

#if defined(POLARSSL_SELF_TEST)

#include "polarssl/certs.h"

/*
 * Checkup routine
 */
int dhm_self_test( int verbose )
{
#if defined(POLARSSL_CERTS_C)
    int ret;
    dhm_context dhm;

    dhm_init( &dhm );

    if( verbose != 0 )
        polarssl_printf( "  DHM parameter load: " );

    if( ( ret = dhm_parse_dhm( &dhm, (const unsigned char *) test_dhm_params,
                               strlen( test_dhm_params ) ) ) != 0 )
    {
        if( verbose != 0 )
            polarssl_printf( "failed\n" );

        ret = 1;
        goto exit;
    }

    if( verbose != 0 )
        polarssl_printf( "passed\n\n" );

exit:
    dhm_free( &dhm );

    return( ret );
#else
    if( verbose != 0 )
        polarssl_printf( "  DHM parameter load: skipped\n" );

    return( 0 );
#endif /* POLARSSL_CERTS_C */
}

#endif /* POLARSSL_SELF_TEST */

#endif /* POLARSSL_DHM_C */
