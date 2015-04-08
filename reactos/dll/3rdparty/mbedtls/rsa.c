/*
 *  The RSA public-key cryptosystem
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
 *  RSA was designed by Ron Rivest, Adi Shamir and Len Adleman.
 *
 *  http://theory.lcs.mit.edu/~rivest/rsapaper.pdf
 *  http://www.cacr.math.uwaterloo.ca/hac/about/chap8.pdf
 */

#if !defined(POLARSSL_CONFIG_FILE)
#include "polarssl/config.h"
#else
#include POLARSSL_CONFIG_FILE
#endif

#if defined(POLARSSL_RSA_C)

#include "polarssl/rsa.h"
#include "polarssl/oid.h"

#if defined(POLARSSL_PKCS1_V21)
#include "polarssl/md.h"
#endif

#include <stdlib.h>
#include <stdio.h>

#if defined(POLARSSL_PLATFORM_C)
#include "polarssl/platform.h"
#else
#define polarssl_printf printf
#endif

/*
 * Initialize an RSA context
 */
void rsa_init( rsa_context *ctx,
               int padding,
               int hash_id )
{
    memset( ctx, 0, sizeof( rsa_context ) );

    rsa_set_padding( ctx, padding, hash_id );

#if defined(POLARSSL_THREADING_C)
    polarssl_mutex_init( &ctx->mutex );
#endif
}

/*
 * Set padding for an existing RSA context
 */
void rsa_set_padding( rsa_context *ctx, int padding, int hash_id )
{
    ctx->padding = padding;
    ctx->hash_id = hash_id;
}

#if defined(POLARSSL_GENPRIME)

/*
 * Generate an RSA keypair
 */
int rsa_gen_key( rsa_context *ctx,
                 int (*f_rng)(void *, unsigned char *, size_t),
                 void *p_rng,
                 unsigned int nbits, int exponent )
{
    int ret;
    mpi P1, Q1, H, G;

    if( f_rng == NULL || nbits < 128 || exponent < 3 )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    mpi_init( &P1 ); mpi_init( &Q1 ); mpi_init( &H ); mpi_init( &G );

    /*
     * find primes P and Q with Q < P so that:
     * GCD( E, (P-1)*(Q-1) ) == 1
     */
    MPI_CHK( mpi_lset( &ctx->E, exponent ) );

    do
    {
        MPI_CHK( mpi_gen_prime( &ctx->P, ( nbits + 1 ) >> 1, 0,
                                f_rng, p_rng ) );

        MPI_CHK( mpi_gen_prime( &ctx->Q, ( nbits + 1 ) >> 1, 0,
                                f_rng, p_rng ) );

        if( mpi_cmp_mpi( &ctx->P, &ctx->Q ) < 0 )
            mpi_swap( &ctx->P, &ctx->Q );

        if( mpi_cmp_mpi( &ctx->P, &ctx->Q ) == 0 )
            continue;

        MPI_CHK( mpi_mul_mpi( &ctx->N, &ctx->P, &ctx->Q ) );
        if( mpi_msb( &ctx->N ) != nbits )
            continue;

        MPI_CHK( mpi_sub_int( &P1, &ctx->P, 1 ) );
        MPI_CHK( mpi_sub_int( &Q1, &ctx->Q, 1 ) );
        MPI_CHK( mpi_mul_mpi( &H, &P1, &Q1 ) );
        MPI_CHK( mpi_gcd( &G, &ctx->E, &H  ) );
    }
    while( mpi_cmp_int( &G, 1 ) != 0 );

    /*
     * D  = E^-1 mod ((P-1)*(Q-1))
     * DP = D mod (P - 1)
     * DQ = D mod (Q - 1)
     * QP = Q^-1 mod P
     */
    MPI_CHK( mpi_inv_mod( &ctx->D , &ctx->E, &H  ) );
    MPI_CHK( mpi_mod_mpi( &ctx->DP, &ctx->D, &P1 ) );
    MPI_CHK( mpi_mod_mpi( &ctx->DQ, &ctx->D, &Q1 ) );
    MPI_CHK( mpi_inv_mod( &ctx->QP, &ctx->Q, &ctx->P ) );

    ctx->len = ( mpi_msb( &ctx->N ) + 7 ) >> 3;

cleanup:

    mpi_free( &P1 ); mpi_free( &Q1 ); mpi_free( &H ); mpi_free( &G );

    if( ret != 0 )
    {
        rsa_free( ctx );
        return( POLARSSL_ERR_RSA_KEY_GEN_FAILED + ret );
    }

    return( 0 );
}

#endif /* POLARSSL_GENPRIME */

/*
 * Check a public RSA key
 */
int rsa_check_pubkey( const rsa_context *ctx )
{
    if( !ctx->N.p || !ctx->E.p )
        return( POLARSSL_ERR_RSA_KEY_CHECK_FAILED );

    if( ( ctx->N.p[0] & 1 ) == 0 ||
        ( ctx->E.p[0] & 1 ) == 0 )
        return( POLARSSL_ERR_RSA_KEY_CHECK_FAILED );

    if( mpi_msb( &ctx->N ) < 128 ||
        mpi_msb( &ctx->N ) > POLARSSL_MPI_MAX_BITS )
        return( POLARSSL_ERR_RSA_KEY_CHECK_FAILED );

    if( mpi_msb( &ctx->E ) < 2 ||
        mpi_cmp_mpi( &ctx->E, &ctx->N ) >= 0 )
        return( POLARSSL_ERR_RSA_KEY_CHECK_FAILED );

    return( 0 );
}

/*
 * Check a private RSA key
 */
int rsa_check_privkey( const rsa_context *ctx )
{
    int ret;
    mpi PQ, DE, P1, Q1, H, I, G, G2, L1, L2, DP, DQ, QP;

    if( ( ret = rsa_check_pubkey( ctx ) ) != 0 )
        return( ret );

    if( !ctx->P.p || !ctx->Q.p || !ctx->D.p )
        return( POLARSSL_ERR_RSA_KEY_CHECK_FAILED );

    mpi_init( &PQ ); mpi_init( &DE ); mpi_init( &P1 ); mpi_init( &Q1 );
    mpi_init( &H  ); mpi_init( &I  ); mpi_init( &G  ); mpi_init( &G2 );
    mpi_init( &L1 ); mpi_init( &L2 ); mpi_init( &DP ); mpi_init( &DQ );
    mpi_init( &QP );

    MPI_CHK( mpi_mul_mpi( &PQ, &ctx->P, &ctx->Q ) );
    MPI_CHK( mpi_mul_mpi( &DE, &ctx->D, &ctx->E ) );
    MPI_CHK( mpi_sub_int( &P1, &ctx->P, 1 ) );
    MPI_CHK( mpi_sub_int( &Q1, &ctx->Q, 1 ) );
    MPI_CHK( mpi_mul_mpi( &H, &P1, &Q1 ) );
    MPI_CHK( mpi_gcd( &G, &ctx->E, &H  ) );

    MPI_CHK( mpi_gcd( &G2, &P1, &Q1 ) );
    MPI_CHK( mpi_div_mpi( &L1, &L2, &H, &G2 ) );
    MPI_CHK( mpi_mod_mpi( &I, &DE, &L1  ) );

    MPI_CHK( mpi_mod_mpi( &DP, &ctx->D, &P1 ) );
    MPI_CHK( mpi_mod_mpi( &DQ, &ctx->D, &Q1 ) );
    MPI_CHK( mpi_inv_mod( &QP, &ctx->Q, &ctx->P ) );
    /*
     * Check for a valid PKCS1v2 private key
     */
    if( mpi_cmp_mpi( &PQ, &ctx->N ) != 0 ||
        mpi_cmp_mpi( &DP, &ctx->DP ) != 0 ||
        mpi_cmp_mpi( &DQ, &ctx->DQ ) != 0 ||
        mpi_cmp_mpi( &QP, &ctx->QP ) != 0 ||
        mpi_cmp_int( &L2, 0 ) != 0 ||
        mpi_cmp_int( &I, 1 ) != 0 ||
        mpi_cmp_int( &G, 1 ) != 0 )
    {
        ret = POLARSSL_ERR_RSA_KEY_CHECK_FAILED;
    }

cleanup:
    mpi_free( &PQ ); mpi_free( &DE ); mpi_free( &P1 ); mpi_free( &Q1 );
    mpi_free( &H  ); mpi_free( &I  ); mpi_free( &G  ); mpi_free( &G2 );
    mpi_free( &L1 ); mpi_free( &L2 ); mpi_free( &DP ); mpi_free( &DQ );
    mpi_free( &QP );

    if( ret == POLARSSL_ERR_RSA_KEY_CHECK_FAILED )
        return( ret );

    if( ret != 0 )
        return( POLARSSL_ERR_RSA_KEY_CHECK_FAILED + ret );

    return( 0 );
}

/*
 * Check if contexts holding a public and private key match
 */
int rsa_check_pub_priv( const rsa_context *pub, const rsa_context *prv )
{
    if( rsa_check_pubkey( pub ) != 0 ||
        rsa_check_privkey( prv ) != 0 )
    {
        return( POLARSSL_ERR_RSA_KEY_CHECK_FAILED );
    }

    if( mpi_cmp_mpi( &pub->N, &prv->N ) != 0 ||
        mpi_cmp_mpi( &pub->E, &prv->E ) != 0 )
    {
        return( POLARSSL_ERR_RSA_KEY_CHECK_FAILED );
    }

    return( 0 );
}

/*
 * Do an RSA public key operation
 */
int rsa_public( rsa_context *ctx,
                const unsigned char *input,
                unsigned char *output )
{
    int ret;
    size_t olen;
    mpi T;

    mpi_init( &T );

    MPI_CHK( mpi_read_binary( &T, input, ctx->len ) );

    if( mpi_cmp_mpi( &T, &ctx->N ) >= 0 )
    {
        mpi_free( &T );
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );
    }

    olen = ctx->len;
    MPI_CHK( mpi_exp_mod( &T, &T, &ctx->E, &ctx->N, &ctx->RN ) );
    MPI_CHK( mpi_write_binary( &T, output, olen ) );

cleanup:

    mpi_free( &T );

    if( ret != 0 )
        return( POLARSSL_ERR_RSA_PUBLIC_FAILED + ret );

    return( 0 );
}

/*
 * Generate or update blinding values, see section 10 of:
 *  KOCHER, Paul C. Timing attacks on implementations of Diffie-Hellman, RSA,
 *  DSS, and other systems. In : Advances in Cryptology—CRYPTO’96. Springer
 *  Berlin Heidelberg, 1996. p. 104-113.
 */
static int rsa_prepare_blinding( rsa_context *ctx, mpi *Vi, mpi *Vf,
                 int (*f_rng)(void *, unsigned char *, size_t), void *p_rng )
{
    int ret, count = 0;

#if defined(POLARSSL_THREADING_C)
    polarssl_mutex_lock( &ctx->mutex );
#endif

    if( ctx->Vf.p != NULL )
    {
        /* We already have blinding values, just update them by squaring */
        MPI_CHK( mpi_mul_mpi( &ctx->Vi, &ctx->Vi, &ctx->Vi ) );
        MPI_CHK( mpi_mod_mpi( &ctx->Vi, &ctx->Vi, &ctx->N ) );
        MPI_CHK( mpi_mul_mpi( &ctx->Vf, &ctx->Vf, &ctx->Vf ) );
        MPI_CHK( mpi_mod_mpi( &ctx->Vf, &ctx->Vf, &ctx->N ) );

        goto done;
    }

    /* Unblinding value: Vf = random number, invertible mod N */
    do {
        if( count++ > 10 )
            return( POLARSSL_ERR_RSA_RNG_FAILED );

        MPI_CHK( mpi_fill_random( &ctx->Vf, ctx->len - 1, f_rng, p_rng ) );
        MPI_CHK( mpi_gcd( &ctx->Vi, &ctx->Vf, &ctx->N ) );
    } while( mpi_cmp_int( &ctx->Vi, 1 ) != 0 );

    /* Blinding value: Vi =  Vf^(-e) mod N */
    MPI_CHK( mpi_inv_mod( &ctx->Vi, &ctx->Vf, &ctx->N ) );
    MPI_CHK( mpi_exp_mod( &ctx->Vi, &ctx->Vi, &ctx->E, &ctx->N, &ctx->RN ) );

done:
    if( Vi != &ctx->Vi )
    {
        MPI_CHK( mpi_copy( Vi, &ctx->Vi ) );
        MPI_CHK( mpi_copy( Vf, &ctx->Vf ) );
    }

cleanup:
#if defined(POLARSSL_THREADING_C)
    polarssl_mutex_unlock( &ctx->mutex );
#endif

    return( ret );
}

/*
 * Do an RSA private key operation
 */
int rsa_private( rsa_context *ctx,
                 int (*f_rng)(void *, unsigned char *, size_t),
                 void *p_rng,
                 const unsigned char *input,
                 unsigned char *output )
{
    int ret;
    size_t olen;
    mpi T, T1, T2;
    mpi *Vi, *Vf;

    /*
     * When using the Chinese Remainder Theorem, we use blinding values.
     * Without threading, we just read them directly from the context,
     * otherwise we make a local copy in order to reduce locking contention.
     */
#if defined(POLARSSL_THREADING_C)
    mpi Vi_copy, Vf_copy;

    mpi_init( &Vi_copy ); mpi_init( &Vf_copy );
    Vi = &Vi_copy;
    Vf = &Vf_copy;
#else
    Vi = &ctx->Vi;
    Vf = &ctx->Vf;
#endif

    mpi_init( &T ); mpi_init( &T1 ); mpi_init( &T2 );

    MPI_CHK( mpi_read_binary( &T, input, ctx->len ) );
    if( mpi_cmp_mpi( &T, &ctx->N ) >= 0 )
    {
        mpi_free( &T );
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );
    }

    if( f_rng != NULL )
    {
        /*
         * Blinding
         * T = T * Vi mod N
         */
        MPI_CHK( rsa_prepare_blinding( ctx, Vi, Vf, f_rng, p_rng ) );
        MPI_CHK( mpi_mul_mpi( &T, &T, Vi ) );
        MPI_CHK( mpi_mod_mpi( &T, &T, &ctx->N ) );
    }

#if defined(POLARSSL_RSA_NO_CRT)
    MPI_CHK( mpi_exp_mod( &T, &T, &ctx->D, &ctx->N, &ctx->RN ) );
#else
    /*
     * faster decryption using the CRT
     *
     * T1 = input ^ dP mod P
     * T2 = input ^ dQ mod Q
     */
    MPI_CHK( mpi_exp_mod( &T1, &T, &ctx->DP, &ctx->P, &ctx->RP ) );
    MPI_CHK( mpi_exp_mod( &T2, &T, &ctx->DQ, &ctx->Q, &ctx->RQ ) );

    /*
     * T = (T1 - T2) * (Q^-1 mod P) mod P
     */
    MPI_CHK( mpi_sub_mpi( &T, &T1, &T2 ) );
    MPI_CHK( mpi_mul_mpi( &T1, &T, &ctx->QP ) );
    MPI_CHK( mpi_mod_mpi( &T, &T1, &ctx->P ) );

    /*
     * T = T2 + T * Q
     */
    MPI_CHK( mpi_mul_mpi( &T1, &T, &ctx->Q ) );
    MPI_CHK( mpi_add_mpi( &T, &T2, &T1 ) );
#endif /* POLARSSL_RSA_NO_CRT */

    if( f_rng != NULL )
    {
        /*
         * Unblind
         * T = T * Vf mod N
         */
        MPI_CHK( mpi_mul_mpi( &T, &T, Vf ) );
        MPI_CHK( mpi_mod_mpi( &T, &T, &ctx->N ) );
    }

    olen = ctx->len;
    MPI_CHK( mpi_write_binary( &T, output, olen ) );

cleanup:
    mpi_free( &T ); mpi_free( &T1 ); mpi_free( &T2 );
#if defined(POLARSSL_THREADING_C)
    mpi_free( &Vi_copy ); mpi_free( &Vf_copy );
#endif

    if( ret != 0 )
        return( POLARSSL_ERR_RSA_PRIVATE_FAILED + ret );

    return( 0 );
}

#if defined(POLARSSL_PKCS1_V21)
/**
 * Generate and apply the MGF1 operation (from PKCS#1 v2.1) to a buffer.
 *
 * \param dst       buffer to mask
 * \param dlen      length of destination buffer
 * \param src       source of the mask generation
 * \param slen      length of the source buffer
 * \param md_ctx    message digest context to use
 */
static void mgf_mask( unsigned char *dst, size_t dlen, unsigned char *src,
                      size_t slen, md_context_t *md_ctx )
{
    unsigned char mask[POLARSSL_MD_MAX_SIZE];
    unsigned char counter[4];
    unsigned char *p;
    unsigned int hlen;
    size_t i, use_len;

    memset( mask, 0, POLARSSL_MD_MAX_SIZE );
    memset( counter, 0, 4 );

    hlen = md_ctx->md_info->size;

    // Generate and apply dbMask
    //
    p = dst;

    while( dlen > 0 )
    {
        use_len = hlen;
        if( dlen < hlen )
            use_len = dlen;

        md_starts( md_ctx );
        md_update( md_ctx, src, slen );
        md_update( md_ctx, counter, 4 );
        md_finish( md_ctx, mask );

        for( i = 0; i < use_len; ++i )
            *p++ ^= mask[i];

        counter[3]++;

        dlen -= use_len;
    }
}
#endif /* POLARSSL_PKCS1_V21 */

#if defined(POLARSSL_PKCS1_V21)
/*
 * Implementation of the PKCS#1 v2.1 RSAES-OAEP-ENCRYPT function
 */
int rsa_rsaes_oaep_encrypt( rsa_context *ctx,
                            int (*f_rng)(void *, unsigned char *, size_t),
                            void *p_rng,
                            int mode,
                            const unsigned char *label, size_t label_len,
                            size_t ilen,
                            const unsigned char *input,
                            unsigned char *output )
{
    size_t olen;
    int ret;
    unsigned char *p = output;
    unsigned int hlen;
    const md_info_t *md_info;
    md_context_t md_ctx;

    if( mode == RSA_PRIVATE && ctx->padding != RSA_PKCS_V21 )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    if( f_rng == NULL )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    md_info = md_info_from_type( ctx->hash_id );
    if( md_info == NULL )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    olen = ctx->len;
    hlen = md_get_size( md_info );

    if( olen < ilen + 2 * hlen + 2 )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    memset( output, 0, olen );

    *p++ = 0;

    // Generate a random octet string seed
    //
    if( ( ret = f_rng( p_rng, p, hlen ) ) != 0 )
        return( POLARSSL_ERR_RSA_RNG_FAILED + ret );

    p += hlen;

    // Construct DB
    //
    md( md_info, label, label_len, p );
    p += hlen;
    p += olen - 2 * hlen - 2 - ilen;
    *p++ = 1;
    memcpy( p, input, ilen );

    md_init( &md_ctx );
    md_init_ctx( &md_ctx, md_info );

    // maskedDB: Apply dbMask to DB
    //
    mgf_mask( output + hlen + 1, olen - hlen - 1, output + 1, hlen,
               &md_ctx );

    // maskedSeed: Apply seedMask to seed
    //
    mgf_mask( output + 1, hlen, output + hlen + 1, olen - hlen - 1,
               &md_ctx );

    md_free( &md_ctx );

    return( ( mode == RSA_PUBLIC )
            ? rsa_public(  ctx, output, output )
            : rsa_private( ctx, f_rng, p_rng, output, output ) );
}
#endif /* POLARSSL_PKCS1_V21 */

#if defined(POLARSSL_PKCS1_V15)
/*
 * Implementation of the PKCS#1 v2.1 RSAES-PKCS1-V1_5-ENCRYPT function
 */
int rsa_rsaes_pkcs1_v15_encrypt( rsa_context *ctx,
                                 int (*f_rng)(void *, unsigned char *, size_t),
                                 void *p_rng,
                                 int mode, size_t ilen,
                                 const unsigned char *input,
                                 unsigned char *output )
{
    size_t nb_pad, olen;
    int ret;
    unsigned char *p = output;

    if( mode == RSA_PRIVATE && ctx->padding != RSA_PKCS_V15 )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    if( f_rng == NULL )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    olen = ctx->len;

    if( olen < ilen + 11 )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    nb_pad = olen - 3 - ilen;

    *p++ = 0;
    if( mode == RSA_PUBLIC )
    {
        *p++ = RSA_CRYPT;

        while( nb_pad-- > 0 )
        {
            int rng_dl = 100;

            do {
                ret = f_rng( p_rng, p, 1 );
            } while( *p == 0 && --rng_dl && ret == 0 );

            // Check if RNG failed to generate data
            //
            if( rng_dl == 0 || ret != 0 )
                return( POLARSSL_ERR_RSA_RNG_FAILED + ret );

            p++;
        }
    }
    else
    {
        *p++ = RSA_SIGN;

        while( nb_pad-- > 0 )
            *p++ = 0xFF;
    }

    *p++ = 0;
    memcpy( p, input, ilen );

    return( ( mode == RSA_PUBLIC )
            ? rsa_public(  ctx, output, output )
            : rsa_private( ctx, f_rng, p_rng, output, output ) );
}
#endif /* POLARSSL_PKCS1_V15 */

/*
 * Add the message padding, then do an RSA operation
 */
int rsa_pkcs1_encrypt( rsa_context *ctx,
                       int (*f_rng)(void *, unsigned char *, size_t),
                       void *p_rng,
                       int mode, size_t ilen,
                       const unsigned char *input,
                       unsigned char *output )
{
    switch( ctx->padding )
    {
#if defined(POLARSSL_PKCS1_V15)
        case RSA_PKCS_V15:
            return rsa_rsaes_pkcs1_v15_encrypt( ctx, f_rng, p_rng, mode, ilen,
                                                input, output );
#endif

#if defined(POLARSSL_PKCS1_V21)
        case RSA_PKCS_V21:
            return rsa_rsaes_oaep_encrypt( ctx, f_rng, p_rng, mode, NULL, 0,
                                           ilen, input, output );
#endif

        default:
            return( POLARSSL_ERR_RSA_INVALID_PADDING );
    }
}

#if defined(POLARSSL_PKCS1_V21)
/*
 * Implementation of the PKCS#1 v2.1 RSAES-OAEP-DECRYPT function
 */
int rsa_rsaes_oaep_decrypt( rsa_context *ctx,
                            int (*f_rng)(void *, unsigned char *, size_t),
                            void *p_rng,
                            int mode,
                            const unsigned char *label, size_t label_len,
                            size_t *olen,
                            const unsigned char *input,
                            unsigned char *output,
                            size_t output_max_len )
{
    int ret;
    size_t ilen, i, pad_len;
    unsigned char *p, bad, pad_done;
    unsigned char buf[POLARSSL_MPI_MAX_SIZE];
    unsigned char lhash[POLARSSL_MD_MAX_SIZE];
    unsigned int hlen;
    const md_info_t *md_info;
    md_context_t md_ctx;

    /*
     * Parameters sanity checks
     */
    if( mode == RSA_PRIVATE && ctx->padding != RSA_PKCS_V21 )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    ilen = ctx->len;

    if( ilen < 16 || ilen > sizeof( buf ) )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    md_info = md_info_from_type( ctx->hash_id );
    if( md_info == NULL )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    /*
     * RSA operation
     */
    ret = ( mode == RSA_PUBLIC )
          ? rsa_public(  ctx, input, buf )
          : rsa_private( ctx, f_rng, p_rng, input, buf );

    if( ret != 0 )
        return( ret );

    /*
     * Unmask data and generate lHash
     */
    hlen = md_get_size( md_info );

    md_init( &md_ctx );
    md_init_ctx( &md_ctx, md_info );

    /* Generate lHash */
    md( md_info, label, label_len, lhash );

    /* seed: Apply seedMask to maskedSeed */
    mgf_mask( buf + 1, hlen, buf + hlen + 1, ilen - hlen - 1,
               &md_ctx );

    /* DB: Apply dbMask to maskedDB */
    mgf_mask( buf + hlen + 1, ilen - hlen - 1, buf + 1, hlen,
               &md_ctx );

    md_free( &md_ctx );

    /*
     * Check contents, in "constant-time"
     */
    p = buf;
    bad = 0;

    bad |= *p++; /* First byte must be 0 */

    p += hlen; /* Skip seed */

    /* Check lHash */
    for( i = 0; i < hlen; i++ )
        bad |= lhash[i] ^ *p++;

    /* Get zero-padding len, but always read till end of buffer
     * (minus one, for the 01 byte) */
    pad_len = 0;
    pad_done = 0;
    for( i = 0; i < ilen - 2 * hlen - 2; i++ )
    {
        pad_done |= p[i];
        pad_len += ( pad_done == 0 );
    }

    p += pad_len;
    bad |= *p++ ^ 0x01;

    /*
     * The only information "leaked" is whether the padding was correct or not
     * (eg, no data is copied if it was not correct). This meets the
     * recommendations in PKCS#1 v2.2: an opponent cannot distinguish between
     * the different error conditions.
     */
    if( bad != 0 )
        return( POLARSSL_ERR_RSA_INVALID_PADDING );

    if( ilen - ( p - buf ) > output_max_len )
        return( POLARSSL_ERR_RSA_OUTPUT_TOO_LARGE );

    *olen = ilen - (p - buf);
    memcpy( output, p, *olen );

    return( 0 );
}
#endif /* POLARSSL_PKCS1_V21 */

#if defined(POLARSSL_PKCS1_V15)
/*
 * Implementation of the PKCS#1 v2.1 RSAES-PKCS1-V1_5-DECRYPT function
 */
int rsa_rsaes_pkcs1_v15_decrypt( rsa_context *ctx,
                                 int (*f_rng)(void *, unsigned char *, size_t),
                                 void *p_rng,
                                 int mode, size_t *olen,
                                 const unsigned char *input,
                                 unsigned char *output,
                                 size_t output_max_len)
{
    int ret;
    size_t ilen, pad_count = 0, i;
    unsigned char *p, bad, pad_done = 0;
    unsigned char buf[POLARSSL_MPI_MAX_SIZE];

    if( mode == RSA_PRIVATE && ctx->padding != RSA_PKCS_V15 )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    ilen = ctx->len;

    if( ilen < 16 || ilen > sizeof( buf ) )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    ret = ( mode == RSA_PUBLIC )
          ? rsa_public(  ctx, input, buf )
          : rsa_private( ctx, f_rng, p_rng, input, buf );

    if( ret != 0 )
        return( ret );

    p = buf;
    bad = 0;

    /*
     * Check and get padding len in "constant-time"
     */
    bad |= *p++; /* First byte must be 0 */

    /* This test does not depend on secret data */
    if( mode == RSA_PRIVATE )
    {
        bad |= *p++ ^ RSA_CRYPT;

        /* Get padding len, but always read till end of buffer
         * (minus one, for the 00 byte) */
        for( i = 0; i < ilen - 3; i++ )
        {
            pad_done |= ( p[i] == 0 );
            pad_count += ( pad_done == 0 );
        }

        p += pad_count;
        bad |= *p++; /* Must be zero */
    }
    else
    {
        bad |= *p++ ^ RSA_SIGN;

        /* Get padding len, but always read till end of buffer
         * (minus one, for the 00 byte) */
        for( i = 0; i < ilen - 3; i++ )
        {
            pad_done |= ( p[i] != 0xFF );
            pad_count += ( pad_done == 0 );
        }

        p += pad_count;
        bad |= *p++; /* Must be zero */
    }

    if( bad )
        return( POLARSSL_ERR_RSA_INVALID_PADDING );

    if( ilen - ( p - buf ) > output_max_len )
        return( POLARSSL_ERR_RSA_OUTPUT_TOO_LARGE );

    *olen = ilen - (p - buf);
    memcpy( output, p, *olen );

    return( 0 );
}
#endif /* POLARSSL_PKCS1_V15 */

/*
 * Do an RSA operation, then remove the message padding
 */
int rsa_pkcs1_decrypt( rsa_context *ctx,
                       int (*f_rng)(void *, unsigned char *, size_t),
                       void *p_rng,
                       int mode, size_t *olen,
                       const unsigned char *input,
                       unsigned char *output,
                       size_t output_max_len)
{
    switch( ctx->padding )
    {
#if defined(POLARSSL_PKCS1_V15)
        case RSA_PKCS_V15:
            return rsa_rsaes_pkcs1_v15_decrypt( ctx, f_rng, p_rng, mode, olen,
                                                input, output, output_max_len );
#endif

#if defined(POLARSSL_PKCS1_V21)
        case RSA_PKCS_V21:
            return rsa_rsaes_oaep_decrypt( ctx, f_rng, p_rng, mode, NULL, 0,
                                           olen, input, output,
                                           output_max_len );
#endif

        default:
            return( POLARSSL_ERR_RSA_INVALID_PADDING );
    }
}

#if defined(POLARSSL_PKCS1_V21)
/*
 * Implementation of the PKCS#1 v2.1 RSASSA-PSS-SIGN function
 */
int rsa_rsassa_pss_sign( rsa_context *ctx,
                         int (*f_rng)(void *, unsigned char *, size_t),
                         void *p_rng,
                         int mode,
                         md_type_t md_alg,
                         unsigned int hashlen,
                         const unsigned char *hash,
                         unsigned char *sig )
{
    size_t olen;
    unsigned char *p = sig;
    unsigned char salt[POLARSSL_MD_MAX_SIZE];
    unsigned int slen, hlen, offset = 0;
    int ret;
    size_t msb;
    const md_info_t *md_info;
    md_context_t md_ctx;

    if( mode == RSA_PRIVATE && ctx->padding != RSA_PKCS_V21 )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    if( f_rng == NULL )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    olen = ctx->len;

    if( md_alg != POLARSSL_MD_NONE )
    {
        // Gather length of hash to sign
        //
        md_info = md_info_from_type( md_alg );
        if( md_info == NULL )
            return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

        hashlen = md_get_size( md_info );
    }

    md_info = md_info_from_type( ctx->hash_id );
    if( md_info == NULL )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    hlen = md_get_size( md_info );
    slen = hlen;

    if( olen < hlen + slen + 2 )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    memset( sig, 0, olen );

    // Generate salt of length slen
    //
    if( ( ret = f_rng( p_rng, salt, slen ) ) != 0 )
        return( POLARSSL_ERR_RSA_RNG_FAILED + ret );

    // Note: EMSA-PSS encoding is over the length of N - 1 bits
    //
    msb = mpi_msb( &ctx->N ) - 1;
    p += olen - hlen * 2 - 2;
    *p++ = 0x01;
    memcpy( p, salt, slen );
    p += slen;

    md_init( &md_ctx );
    md_init_ctx( &md_ctx, md_info );

    // Generate H = Hash( M' )
    //
    md_starts( &md_ctx );
    md_update( &md_ctx, p, 8 );
    md_update( &md_ctx, hash, hashlen );
    md_update( &md_ctx, salt, slen );
    md_finish( &md_ctx, p );

    // Compensate for boundary condition when applying mask
    //
    if( msb % 8 == 0 )
        offset = 1;

    // maskedDB: Apply dbMask to DB
    //
    mgf_mask( sig + offset, olen - hlen - 1 - offset, p, hlen, &md_ctx );

    md_free( &md_ctx );

    msb = mpi_msb( &ctx->N ) - 1;
    sig[0] &= 0xFF >> ( olen * 8 - msb );

    p += hlen;
    *p++ = 0xBC;

    return( ( mode == RSA_PUBLIC )
            ? rsa_public(  ctx, sig, sig )
            : rsa_private( ctx, f_rng, p_rng, sig, sig ) );
}
#endif /* POLARSSL_PKCS1_V21 */

#if defined(POLARSSL_PKCS1_V15)
/*
 * Implementation of the PKCS#1 v2.1 RSASSA-PKCS1-V1_5-SIGN function
 */
/*
 * Do an RSA operation to sign the message digest
 */
int rsa_rsassa_pkcs1_v15_sign( rsa_context *ctx,
                               int (*f_rng)(void *, unsigned char *, size_t),
                               void *p_rng,
                               int mode,
                               md_type_t md_alg,
                               unsigned int hashlen,
                               const unsigned char *hash,
                               unsigned char *sig )
{
    size_t nb_pad, olen, oid_size = 0;
    unsigned char *p = sig;
    const char *oid = NULL;

    if( mode == RSA_PRIVATE && ctx->padding != RSA_PKCS_V15 )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    olen = ctx->len;
    nb_pad = olen - 3;

    if( md_alg != POLARSSL_MD_NONE )
    {
        const md_info_t *md_info = md_info_from_type( md_alg );
        if( md_info == NULL )
            return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

        if( oid_get_oid_by_md( md_alg, &oid, &oid_size ) != 0 )
            return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

        nb_pad -= 10 + oid_size;

        hashlen = md_get_size( md_info );
    }

    nb_pad -= hashlen;

    if( ( nb_pad < 8 ) || ( nb_pad > olen ) )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    *p++ = 0;
    *p++ = RSA_SIGN;
    memset( p, 0xFF, nb_pad );
    p += nb_pad;
    *p++ = 0;

    if( md_alg == POLARSSL_MD_NONE )
    {
        memcpy( p, hash, hashlen );
    }
    else
    {
        /*
         * DigestInfo ::= SEQUENCE {
         *   digestAlgorithm DigestAlgorithmIdentifier,
         *   digest Digest }
         *
         * DigestAlgorithmIdentifier ::= AlgorithmIdentifier
         *
         * Digest ::= OCTET STRING
         */
        *p++ = ASN1_SEQUENCE | ASN1_CONSTRUCTED;
        *p++ = (unsigned char) ( 0x08 + oid_size + hashlen );
        *p++ = ASN1_SEQUENCE | ASN1_CONSTRUCTED;
        *p++ = (unsigned char) ( 0x04 + oid_size );
        *p++ = ASN1_OID;
        *p++ = oid_size & 0xFF;
        memcpy( p, oid, oid_size );
        p += oid_size;
        *p++ = ASN1_NULL;
        *p++ = 0x00;
        *p++ = ASN1_OCTET_STRING;
        *p++ = hashlen;
        memcpy( p, hash, hashlen );
    }

    return( ( mode == RSA_PUBLIC )
            ? rsa_public(  ctx, sig, sig )
            : rsa_private( ctx, f_rng, p_rng, sig, sig ) );
}
#endif /* POLARSSL_PKCS1_V15 */

/*
 * Do an RSA operation to sign the message digest
 */
int rsa_pkcs1_sign( rsa_context *ctx,
                    int (*f_rng)(void *, unsigned char *, size_t),
                    void *p_rng,
                    int mode,
                    md_type_t md_alg,
                    unsigned int hashlen,
                    const unsigned char *hash,
                    unsigned char *sig )
{
    switch( ctx->padding )
    {
#if defined(POLARSSL_PKCS1_V15)
        case RSA_PKCS_V15:
            return rsa_rsassa_pkcs1_v15_sign( ctx, f_rng, p_rng, mode, md_alg,
                                              hashlen, hash, sig );
#endif

#if defined(POLARSSL_PKCS1_V21)
        case RSA_PKCS_V21:
            return rsa_rsassa_pss_sign( ctx, f_rng, p_rng, mode, md_alg,
                                        hashlen, hash, sig );
#endif

        default:
            return( POLARSSL_ERR_RSA_INVALID_PADDING );
    }
}

#if defined(POLARSSL_PKCS1_V21)
/*
 * Implementation of the PKCS#1 v2.1 RSASSA-PSS-VERIFY function
 */
int rsa_rsassa_pss_verify_ext( rsa_context *ctx,
                               int (*f_rng)(void *, unsigned char *, size_t),
                               void *p_rng,
                               int mode,
                               md_type_t md_alg,
                               unsigned int hashlen,
                               const unsigned char *hash,
                               md_type_t mgf1_hash_id,
                               int expected_salt_len,
                               const unsigned char *sig )
{
    int ret;
    size_t siglen;
    unsigned char *p;
    unsigned char buf[POLARSSL_MPI_MAX_SIZE];
    unsigned char result[POLARSSL_MD_MAX_SIZE];
    unsigned char zeros[8];
    unsigned int hlen;
    size_t slen, msb;
    const md_info_t *md_info;
    md_context_t md_ctx;

    if( mode == RSA_PRIVATE && ctx->padding != RSA_PKCS_V21 )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    siglen = ctx->len;

    if( siglen < 16 || siglen > sizeof( buf ) )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    ret = ( mode == RSA_PUBLIC )
          ? rsa_public(  ctx, sig, buf )
          : rsa_private( ctx, f_rng, p_rng, sig, buf );

    if( ret != 0 )
        return( ret );

    p = buf;

    if( buf[siglen - 1] != 0xBC )
        return( POLARSSL_ERR_RSA_INVALID_PADDING );

    if( md_alg != POLARSSL_MD_NONE )
    {
        // Gather length of hash to sign
        //
        md_info = md_info_from_type( md_alg );
        if( md_info == NULL )
            return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

        hashlen = md_get_size( md_info );
    }

    md_info = md_info_from_type( mgf1_hash_id );
    if( md_info == NULL )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    hlen = md_get_size( md_info );
    slen = siglen - hlen - 1; /* Currently length of salt + padding */

    memset( zeros, 0, 8 );

    // Note: EMSA-PSS verification is over the length of N - 1 bits
    //
    msb = mpi_msb( &ctx->N ) - 1;

    // Compensate for boundary condition when applying mask
    //
    if( msb % 8 == 0 )
    {
        p++;
        siglen -= 1;
    }
    if( buf[0] >> ( 8 - siglen * 8 + msb ) )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    md_init( &md_ctx );
    md_init_ctx( &md_ctx, md_info );

    mgf_mask( p, siglen - hlen - 1, p + siglen - hlen - 1, hlen, &md_ctx );

    buf[0] &= 0xFF >> ( siglen * 8 - msb );

    while( p < buf + siglen && *p == 0 )
        p++;

    if( p == buf + siglen ||
        *p++ != 0x01 )
    {
        md_free( &md_ctx );
        return( POLARSSL_ERR_RSA_INVALID_PADDING );
    }

    /* Actual salt len */
    slen -= p - buf;

    if( expected_salt_len != RSA_SALT_LEN_ANY &&
        slen != (size_t) expected_salt_len )
    {
        md_free( &md_ctx );
        return( POLARSSL_ERR_RSA_INVALID_PADDING );
    }

    // Generate H = Hash( M' )
    //
    md_starts( &md_ctx );
    md_update( &md_ctx, zeros, 8 );
    md_update( &md_ctx, hash, hashlen );
    md_update( &md_ctx, p, slen );
    md_finish( &md_ctx, result );

    md_free( &md_ctx );

    if( memcmp( p + slen, result, hlen ) == 0 )
        return( 0 );
    else
        return( POLARSSL_ERR_RSA_VERIFY_FAILED );
}

/*
 * Simplified PKCS#1 v2.1 RSASSA-PSS-VERIFY function
 */
int rsa_rsassa_pss_verify( rsa_context *ctx,
                           int (*f_rng)(void *, unsigned char *, size_t),
                           void *p_rng,
                           int mode,
                           md_type_t md_alg,
                           unsigned int hashlen,
                           const unsigned char *hash,
                           const unsigned char *sig )
{
    md_type_t mgf1_hash_id = ( ctx->hash_id != POLARSSL_MD_NONE )
                             ? (md_type_t) ctx->hash_id
                             : md_alg;

    return( rsa_rsassa_pss_verify_ext( ctx, f_rng, p_rng, mode,
                                       md_alg, hashlen, hash,
                                       mgf1_hash_id, RSA_SALT_LEN_ANY,
                                       sig ) );

}
#endif /* POLARSSL_PKCS1_V21 */

#if defined(POLARSSL_PKCS1_V15)
/*
 * Implementation of the PKCS#1 v2.1 RSASSA-PKCS1-v1_5-VERIFY function
 */
int rsa_rsassa_pkcs1_v15_verify( rsa_context *ctx,
                                 int (*f_rng)(void *, unsigned char *, size_t),
                                 void *p_rng,
                                 int mode,
                                 md_type_t md_alg,
                                 unsigned int hashlen,
                                 const unsigned char *hash,
                                 const unsigned char *sig )
{
    int ret;
    size_t len, siglen, asn1_len;
    unsigned char *p, *end;
    unsigned char buf[POLARSSL_MPI_MAX_SIZE];
    md_type_t msg_md_alg;
    const md_info_t *md_info;
    asn1_buf oid;

    if( mode == RSA_PRIVATE && ctx->padding != RSA_PKCS_V15 )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    siglen = ctx->len;

    if( siglen < 16 || siglen > sizeof( buf ) )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    ret = ( mode == RSA_PUBLIC )
          ? rsa_public(  ctx, sig, buf )
          : rsa_private( ctx, f_rng, p_rng, sig, buf );

    if( ret != 0 )
        return( ret );

    p = buf;

    if( *p++ != 0 || *p++ != RSA_SIGN )
        return( POLARSSL_ERR_RSA_INVALID_PADDING );

    while( *p != 0 )
    {
        if( p >= buf + siglen - 1 || *p != 0xFF )
            return( POLARSSL_ERR_RSA_INVALID_PADDING );
        p++;
    }
    p++;

    len = siglen - ( p - buf );

    if( len == hashlen && md_alg == POLARSSL_MD_NONE )
    {
        if( memcmp( p, hash, hashlen ) == 0 )
            return( 0 );
        else
            return( POLARSSL_ERR_RSA_VERIFY_FAILED );
    }

    md_info = md_info_from_type( md_alg );
    if( md_info == NULL )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );
    hashlen = md_get_size( md_info );

    end = p + len;

    // Parse the ASN.1 structure inside the PKCS#1 v1.5 structure
    //
    if( ( ret = asn1_get_tag( &p, end, &asn1_len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
        return( POLARSSL_ERR_RSA_VERIFY_FAILED );

    if( asn1_len + 2 != len )
        return( POLARSSL_ERR_RSA_VERIFY_FAILED );

    if( ( ret = asn1_get_tag( &p, end, &asn1_len,
            ASN1_CONSTRUCTED | ASN1_SEQUENCE ) ) != 0 )
        return( POLARSSL_ERR_RSA_VERIFY_FAILED );

    if( asn1_len + 6 + hashlen != len )
        return( POLARSSL_ERR_RSA_VERIFY_FAILED );

    if( ( ret = asn1_get_tag( &p, end, &oid.len, ASN1_OID ) ) != 0 )
        return( POLARSSL_ERR_RSA_VERIFY_FAILED );

    oid.p = p;
    p += oid.len;

    if( oid_get_md_alg( &oid, &msg_md_alg ) != 0 )
        return( POLARSSL_ERR_RSA_VERIFY_FAILED );

    if( md_alg != msg_md_alg )
        return( POLARSSL_ERR_RSA_VERIFY_FAILED );

    /*
     * assume the algorithm parameters must be NULL
     */
    if( ( ret = asn1_get_tag( &p, end, &asn1_len, ASN1_NULL ) ) != 0 )
        return( POLARSSL_ERR_RSA_VERIFY_FAILED );

    if( ( ret = asn1_get_tag( &p, end, &asn1_len, ASN1_OCTET_STRING ) ) != 0 )
        return( POLARSSL_ERR_RSA_VERIFY_FAILED );

    if( asn1_len != hashlen )
        return( POLARSSL_ERR_RSA_VERIFY_FAILED );

    if( memcmp( p, hash, hashlen ) != 0 )
        return( POLARSSL_ERR_RSA_VERIFY_FAILED );

    p += hashlen;

    if( p != end )
        return( POLARSSL_ERR_RSA_VERIFY_FAILED );

    return( 0 );
}
#endif /* POLARSSL_PKCS1_V15 */

/*
 * Do an RSA operation and check the message digest
 */
int rsa_pkcs1_verify( rsa_context *ctx,
                      int (*f_rng)(void *, unsigned char *, size_t),
                      void *p_rng,
                      int mode,
                      md_type_t md_alg,
                      unsigned int hashlen,
                      const unsigned char *hash,
                      const unsigned char *sig )
{
    switch( ctx->padding )
    {
#if defined(POLARSSL_PKCS1_V15)
        case RSA_PKCS_V15:
            return rsa_rsassa_pkcs1_v15_verify( ctx, f_rng, p_rng, mode, md_alg,
                                                hashlen, hash, sig );
#endif

#if defined(POLARSSL_PKCS1_V21)
        case RSA_PKCS_V21:
            return rsa_rsassa_pss_verify( ctx, f_rng, p_rng, mode, md_alg,
                                          hashlen, hash, sig );
#endif

        default:
            return( POLARSSL_ERR_RSA_INVALID_PADDING );
    }
}

/*
 * Copy the components of an RSA key
 */
int rsa_copy( rsa_context *dst, const rsa_context *src )
{
    int ret;

    dst->ver = src->ver;
    dst->len = src->len;

    MPI_CHK( mpi_copy( &dst->N, &src->N ) );
    MPI_CHK( mpi_copy( &dst->E, &src->E ) );

    MPI_CHK( mpi_copy( &dst->D, &src->D ) );
    MPI_CHK( mpi_copy( &dst->P, &src->P ) );
    MPI_CHK( mpi_copy( &dst->Q, &src->Q ) );
    MPI_CHK( mpi_copy( &dst->DP, &src->DP ) );
    MPI_CHK( mpi_copy( &dst->DQ, &src->DQ ) );
    MPI_CHK( mpi_copy( &dst->QP, &src->QP ) );

    MPI_CHK( mpi_copy( &dst->RN, &src->RN ) );
    MPI_CHK( mpi_copy( &dst->RP, &src->RP ) );
    MPI_CHK( mpi_copy( &dst->RQ, &src->RQ ) );

    MPI_CHK( mpi_copy( &dst->Vi, &src->Vi ) );
    MPI_CHK( mpi_copy( &dst->Vf, &src->Vf ) );

    dst->padding = src->padding;
    dst->hash_id = src->hash_id;

cleanup:
    if( ret != 0 )
        rsa_free( dst );

    return( ret );
}

/*
 * Free the components of an RSA key
 */
void rsa_free( rsa_context *ctx )
{
    mpi_free( &ctx->Vi ); mpi_free( &ctx->Vf );
    mpi_free( &ctx->RQ ); mpi_free( &ctx->RP ); mpi_free( &ctx->RN );
    mpi_free( &ctx->QP ); mpi_free( &ctx->DQ ); mpi_free( &ctx->DP );
    mpi_free( &ctx->Q  ); mpi_free( &ctx->P  ); mpi_free( &ctx->D );
    mpi_free( &ctx->E  ); mpi_free( &ctx->N  );

#if defined(POLARSSL_THREADING_C)
    polarssl_mutex_free( &ctx->mutex );
#endif
}

#if defined(POLARSSL_SELF_TEST)

#include "polarssl/sha1.h"

/*
 * Example RSA-1024 keypair, for test purposes
 */
#define KEY_LEN 128

#define RSA_N   "9292758453063D803DD603D5E777D788" \
                "8ED1D5BF35786190FA2F23EBC0848AEA" \
                "DDA92CA6C3D80B32C4D109BE0F36D6AE" \
                "7130B9CED7ACDF54CFC7555AC14EEBAB" \
                "93A89813FBF3C4F8066D2D800F7C38A8" \
                "1AE31942917403FF4946B0A83D3D3E05" \
                "EE57C6F5F5606FB5D4BC6CD34EE0801A" \
                "5E94BB77B07507233A0BC7BAC8F90F79"

#define RSA_E   "10001"

#define RSA_D   "24BF6185468786FDD303083D25E64EFC" \
                "66CA472BC44D253102F8B4A9D3BFA750" \
                "91386C0077937FE33FA3252D28855837" \
                "AE1B484A8A9A45F7EE8C0C634F99E8CD" \
                "DF79C5CE07EE72C7F123142198164234" \
                "CABB724CF78B8173B9F880FC86322407" \
                "AF1FEDFDDE2BEB674CA15F3E81A1521E" \
                "071513A1E85B5DFA031F21ECAE91A34D"

#define RSA_P   "C36D0EB7FCD285223CFB5AABA5BDA3D8" \
                "2C01CAD19EA484A87EA4377637E75500" \
                "FCB2005C5C7DD6EC4AC023CDA285D796" \
                "C3D9E75E1EFC42488BB4F1D13AC30A57"

#define RSA_Q   "C000DF51A7C77AE8D7C7370C1FF55B69" \
                "E211C2B9E5DB1ED0BF61D0D9899620F4" \
                "910E4168387E3C30AA1E00C339A79508" \
                "8452DD96A9A5EA5D9DCA68DA636032AF"

#define RSA_DP  "C1ACF567564274FB07A0BBAD5D26E298" \
                "3C94D22288ACD763FD8E5600ED4A702D" \
                "F84198A5F06C2E72236AE490C93F07F8" \
                "3CC559CD27BC2D1CA488811730BB5725"

#define RSA_DQ  "4959CBF6F8FEF750AEE6977C155579C7" \
                "D8AAEA56749EA28623272E4F7D0592AF" \
                "7C1F1313CAC9471B5C523BFE592F517B" \
                "407A1BD76C164B93DA2D32A383E58357"

#define RSA_QP  "9AE7FBC99546432DF71896FC239EADAE" \
                "F38D18D2B2F0E2DD275AA977E2BF4411" \
                "F5A3B2A5D33605AEBBCCBA7FEB9F2D2F" \
                "A74206CEC169D74BF5A8C50D6F48EA08"

#define PT_LEN  24
#define RSA_PT  "\xAA\xBB\xCC\x03\x02\x01\x00\xFF\xFF\xFF\xFF\xFF" \
                "\x11\x22\x33\x0A\x0B\x0C\xCC\xDD\xDD\xDD\xDD\xDD"

#if defined(POLARSSL_PKCS1_V15)
static int myrand( void *rng_state, unsigned char *output, size_t len )
{
#if !defined(__OpenBSD__)
    size_t i;

    if( rng_state != NULL )
        rng_state  = NULL;

    for( i = 0; i < len; ++i )
        output[i] = rand();
#else
    if( rng_state != NULL )
        rng_state = NULL;

    arc4random_buf( output, len );
#endif /* !OpenBSD */

    return( 0 );
}
#endif /* POLARSSL_PKCS1_V15 */

/*
 * Checkup routine
 */
int rsa_self_test( int verbose )
{
    int ret = 0;
#if defined(POLARSSL_PKCS1_V15)
    size_t len;
    rsa_context rsa;
    unsigned char rsa_plaintext[PT_LEN];
    unsigned char rsa_decrypted[PT_LEN];
    unsigned char rsa_ciphertext[KEY_LEN];
#if defined(POLARSSL_SHA1_C)
    unsigned char sha1sum[20];
#endif

    rsa_init( &rsa, RSA_PKCS_V15, 0 );

    rsa.len = KEY_LEN;
    MPI_CHK( mpi_read_string( &rsa.N , 16, RSA_N  ) );
    MPI_CHK( mpi_read_string( &rsa.E , 16, RSA_E  ) );
    MPI_CHK( mpi_read_string( &rsa.D , 16, RSA_D  ) );
    MPI_CHK( mpi_read_string( &rsa.P , 16, RSA_P  ) );
    MPI_CHK( mpi_read_string( &rsa.Q , 16, RSA_Q  ) );
    MPI_CHK( mpi_read_string( &rsa.DP, 16, RSA_DP ) );
    MPI_CHK( mpi_read_string( &rsa.DQ, 16, RSA_DQ ) );
    MPI_CHK( mpi_read_string( &rsa.QP, 16, RSA_QP ) );

    if( verbose != 0 )
        polarssl_printf( "  RSA key validation: " );

    if( rsa_check_pubkey(  &rsa ) != 0 ||
        rsa_check_privkey( &rsa ) != 0 )
    {
        if( verbose != 0 )
            polarssl_printf( "failed\n" );

        return( 1 );
    }

    if( verbose != 0 )
        polarssl_printf( "passed\n  PKCS#1 encryption : " );

    memcpy( rsa_plaintext, RSA_PT, PT_LEN );

    if( rsa_pkcs1_encrypt( &rsa, myrand, NULL, RSA_PUBLIC, PT_LEN,
                           rsa_plaintext, rsa_ciphertext ) != 0 )
    {
        if( verbose != 0 )
            polarssl_printf( "failed\n" );

        return( 1 );
    }

    if( verbose != 0 )
        polarssl_printf( "passed\n  PKCS#1 decryption : " );

    if( rsa_pkcs1_decrypt( &rsa, myrand, NULL, RSA_PRIVATE, &len,
                           rsa_ciphertext, rsa_decrypted,
                           sizeof(rsa_decrypted) ) != 0 )
    {
        if( verbose != 0 )
            polarssl_printf( "failed\n" );

        return( 1 );
    }

    if( memcmp( rsa_decrypted, rsa_plaintext, len ) != 0 )
    {
        if( verbose != 0 )
            polarssl_printf( "failed\n" );

        return( 1 );
    }

#if defined(POLARSSL_SHA1_C)
    if( verbose != 0 )
        polarssl_printf( "passed\n  PKCS#1 data sign  : " );

    sha1( rsa_plaintext, PT_LEN, sha1sum );

    if( rsa_pkcs1_sign( &rsa, myrand, NULL, RSA_PRIVATE, POLARSSL_MD_SHA1, 0,
                        sha1sum, rsa_ciphertext ) != 0 )
    {
        if( verbose != 0 )
            polarssl_printf( "failed\n" );

        return( 1 );
    }

    if( verbose != 0 )
        polarssl_printf( "passed\n  PKCS#1 sig. verify: " );

    if( rsa_pkcs1_verify( &rsa, NULL, NULL, RSA_PUBLIC, POLARSSL_MD_SHA1, 0,
                          sha1sum, rsa_ciphertext ) != 0 )
    {
        if( verbose != 0 )
            polarssl_printf( "failed\n" );

        return( 1 );
    }

    if( verbose != 0 )
        polarssl_printf( "passed\n\n" );
#endif /* POLARSSL_SHA1_C */

cleanup:
    rsa_free( &rsa );
#else /* POLARSSL_PKCS1_V15 */
    ((void) verbose);
#endif /* POLARSSL_PKCS1_V15 */
    return( ret );
}

#endif /* POLARSSL_SELF_TEST */

#endif /* POLARSSL_RSA_C */
