/*
 *  Digital Signature Algorithm (DSA)
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_DSA_C)

#include "mbedtls/dsa.h"
#include "mbedtls/asn1write.h"
#include "mbedtls/platform_util.h"

#include <string.h>

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdlib.h>
#include <stdio.h>
#define mbedtls_printf     printf
#define mbedtls_calloc    calloc
#define mbedtls_free       free
#endif

#if !defined(MBEDTLS_DSA_ALT)

#define DSA_VALIDATE_RET( cond )    \
    MBEDTLS_INTERNAL_VALIDATE_RET( cond, MBEDTLS_ERR_DSA_BAD_INPUT_DATA )
#define DSA_VALIDATE( cond )        \
    MBEDTLS_INTERNAL_VALIDATE( cond )

/*
 * Initialize context
 */
void mbedtls_dsa_init( mbedtls_dsa_context *ctx )
{
    DSA_VALIDATE( ctx != NULL );
    memset( ctx, 0, sizeof( mbedtls_dsa_context ) );

    mbedtls_mpi_init( &ctx->P );
    mbedtls_mpi_init( &ctx->Q );
    mbedtls_mpi_init( &ctx->G );
    mbedtls_mpi_init( &ctx->Y );
    mbedtls_mpi_init( &ctx->X );
}

/*
 * Free context
 */
void mbedtls_dsa_free( mbedtls_dsa_context *ctx )
{
    if( ctx == NULL )
        return;

    mbedtls_mpi_free( &ctx->P );
    mbedtls_mpi_free( &ctx->Q );
    mbedtls_mpi_free( &ctx->G );
    mbedtls_mpi_free( &ctx->Y );
    mbedtls_mpi_free( &ctx->X );

    mbedtls_platform_zeroize( ctx, sizeof( mbedtls_dsa_context ) );
}

/*
 * Set group parameters
 */
int mbedtls_dsa_set_group( mbedtls_dsa_context *ctx, const mbedtls_mpi *P, const mbedtls_mpi *Q, const mbedtls_mpi *G )
{
    int ret;
    DSA_VALIDATE_RET( ctx != NULL );
    DSA_VALIDATE_RET( P != NULL );
    DSA_VALIDATE_RET( Q != NULL );
    DSA_VALIDATE_RET( G != NULL );

    if( ( ret = mbedtls_mpi_copy( &ctx->P, P ) ) != 0 )
    {
        return( MBEDTLS_ERR_DSA_BAD_INPUT_DATA + ret );
    }

    if( ( ret = mbedtls_mpi_copy( &ctx->Q, Q ) ) != 0 )
    {
        return( MBEDTLS_ERR_DSA_BAD_INPUT_DATA + ret );
    }

    if( ( ret = mbedtls_mpi_copy( &ctx->G, G ) ) != 0 )
    {
        return( MBEDTLS_ERR_DSA_BAD_INPUT_DATA + ret );
    }

    if (!ctx->len) ctx->len = mbedtls_mpi_size( &ctx->P );

    return( 0 );
}

/** 
 * Compute a power of two
 *  @param a  The integer to store the power in
 *  @param n  The power of two you want to store (a = 2^n)
 *  @return 0 on success
 */
int mpi_2expt(mbedtls_mpi *a, int n)
{
    int ret;

    DSA_VALIDATE_RET( a != NULL );

    MBEDTLS_MPI_CHK( mbedtls_mpi_lset(a, 0) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_set_bit(a, n, 1) );

cleanup:
    return ret;
}

/*
 * Generate keypair
 */
int mbedtls_dsa_genkey( mbedtls_dsa_context *ctx, size_t group_size, size_t modulus_size,
                int (*f_rng)(void *, unsigned char *, size_t), void *p_rng )
{
    int ret;
    unsigned long L, N, n, outbytes, seedbytes, counter, j, i;
    int mr_tests_q, mr_tests_p, found_p, found_q;
    mbedtls_mpi t2L1, t2N1, t2seedlen, U, U1, W, W1, X, q, q1, p, p1, e, h, h1, g, c, t2q, seedinc, seedinc1;
    unsigned char *wbuf, *sbuf, digest[MBEDTLS_DSA_MAX_GROUP];
    const mbedtls_md_info_t * md_info;

    DSA_VALIDATE_RET( ctx != NULL );
    DSA_VALIDATE_RET( f_rng != NULL );

    /* check size */
    DSA_VALIDATE_RET( group_size <= MBEDTLS_DSA_MAX_GROUP );
    DSA_VALIDATE_RET( group_size >= 1 );
    DSA_VALIDATE_RET( group_size < modulus_size );
    DSA_VALIDATE_RET( modulus_size <= MBEDTLS_DSA_MAX_MODULUS );

#if !defined(MBEDTLS_SHA512_C) && !defined(MBEDTLS_SHA256_C)
    return MBEDTLS_ERR_DSA_BAD_INPUT_DATA;
#endif

#if defined(MBEDTLS_SHA512_C)
    md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA512);
#else
    md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
#endif

    seedbytes = group_size;
    L = modulus_size * 8;
    N = group_size * 8;
    if (N > mbedtls_md_get_size(md_info) * 8) return MBEDTLS_ERR_DSA_BAD_INPUT_DATA; /* group_size too big */

    outbytes = mbedtls_md_get_size(md_info);
    n = ((L + outbytes*8 - 1) / (outbytes*8)) - 1;

    if ((wbuf = mbedtls_calloc(1, (n+1)*outbytes)) == NULL) { ret = MBEDTLS_ERR_MPI_ALLOC_FAILED; goto cleanup3; }
    if ((sbuf = mbedtls_calloc(1, seedbytes)) == NULL)      { ret = MBEDTLS_ERR_MPI_ALLOC_FAILED; goto cleanup2; }

    mbedtls_mpi_init( &t2L1 );
    mbedtls_mpi_init( &t2N1 );
    mbedtls_mpi_init( &t2seedlen );
    mbedtls_mpi_init( &U );
    mbedtls_mpi_init( &U1 );
    mbedtls_mpi_init( &W );
    mbedtls_mpi_init( &W1 );
    mbedtls_mpi_init( &X );
    mbedtls_mpi_init( &q );
    mbedtls_mpi_init( &q1 );
    mbedtls_mpi_init( &p );
    mbedtls_mpi_init( &p1 );
    mbedtls_mpi_init( &e );
    mbedtls_mpi_init( &h );
    mbedtls_mpi_init( &h1 );
    mbedtls_mpi_init( &g );
    mbedtls_mpi_init( &c );
    mbedtls_mpi_init( &t2q );
    mbedtls_mpi_init( &seedinc );
    mbedtls_mpi_init( &seedinc1 );

    /* M-R tests (without Lucas test) according FIPS-186-4 - Appendix C.3 - table C.1 */
    if      (L <= 1024) { mr_tests_p = 40; }
    else if (L <= 2048) { mr_tests_p = 56; }
    else                { mr_tests_p = 64; }

    if      (N <= 160)  { mr_tests_q = 40; }
    else if (N <= 224)  { mr_tests_q = 56; }
    else                { mr_tests_q = 64; }

    /* t2L1 = 2^(L-1) */
    MBEDTLS_MPI_CHK( mpi_2expt(&t2L1, L-1) );
    /* t2N1 = 2^(N-1) */
    MBEDTLS_MPI_CHK( mpi_2expt(&t2N1, N-1) );
    /* t2seedlen = 2^seedlen */
    MBEDTLS_MPI_CHK( mpi_2expt(&t2seedlen, seedbytes*8) );

    /* FIPS-186-4 A.1.1.2 Generation of the Probable Primes p and q Using an Approved Hash Function
    *
    * L = The desired length of the prime p (in bits e.g. L = 1024)
    * N = The desired length of the prime q (in bits e.g. N = 160)
    * seedlen = The desired bit length of the domain parameter seed; seedlen shallbe equal to or greater than N
    * outlen  = The bit length of Hash function
    *
    * 1.  Check that the (L, N)
    * 2.  If (seedlen <N), then return INVALID.
    * 3.  n = ceil(L / outlen) - 1
    * 4.  b = L- 1 - (n * outlen)
    * 5.  domain_parameter_seed = an arbitrary sequence of seedlen bits
    * 6.  U = Hash (domain_parameter_seed) mod 2^(N-1)
    * 7.  q = 2^(N-1) + U + 1 - (U mod 2)
    * 8.  Test whether or not q is prime as specified in Appendix C.3
    * 9.  If qis not a prime, then go to step 5.
    * 10. offset = 1
    * 11. For counter = 0 to (4L- 1) do {
    *       For j=0 to n do {
    *         Vj = Hash ((domain_parameter_seed+ offset + j) mod 2^seedlen
    *       }
    *       W = V0 + (V1 *2^outlen) + ... + (Vn-1 * 2^((n-1) * outlen)) + ((Vn mod 2^b) * 2^(n * outlen))
    *       X = W + 2^(L-1)           Comment: 0 <= W < 2^(L-1); hence 2^(L-1) <= X < 2^L
    *       c = X mod 2*q
    *       p = X - (c - 1)           Comment: p ~ 1 (mod 2*q)
    *       If (p >= 2^(L-1)) {
    *         Test whether or not p is prime as specified in Appendix C.3.
    *         If p is determined to be prime, then return VALID and the values of p, qand (optionally) the values of domain_parameter_seed and counter
    *       }
    *       offset = offset + n + 1   Comment: Increment offset
    *     }
    */
    for(found_p=0; !found_p;) {
        /* q */
        for(found_q=0; !found_q;) {
            f_rng( p_rng, sbuf, seedbytes );
            i = outbytes;
            MBEDTLS_MPI_CHK( mbedtls_md(md_info, sbuf, seedbytes, digest) );
            MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary(&U, digest, outbytes) );
            MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi(&U1, &U, &t2N1) );
            MBEDTLS_MPI_CHK( mbedtls_mpi_add_mpi(&q, &t2N1, &U1) );
            if (mbedtls_mpi_get_bit(&q, 0) == 0)
            {
                MBEDTLS_MPI_CHK( mbedtls_mpi_copy(&q, &q1) );
                MBEDTLS_MPI_CHK( mbedtls_mpi_add_int(&q, &q1, 1) );
            }
            if( mbedtls_mpi_is_prime_ext(&q, mr_tests_q, f_rng, p_rng) == 0) found_q = 1;
        }
        /* p */
        MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary(&seedinc, sbuf, seedbytes) );
        MBEDTLS_MPI_CHK( mbedtls_mpi_add_mpi(&t2q, &q, &q) );
        for(counter=0; counter < 4*L && !found_p; counter++) {
            for(j=0; j<=n; j++) {
                MBEDTLS_MPI_CHK( mbedtls_mpi_copy(&seedinc1, &seedinc) );
                MBEDTLS_MPI_CHK( mbedtls_mpi_add_int(&seedinc, &seedinc1, 1) );
                MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi(&seedinc1, &seedinc, &t2seedlen) );
                MBEDTLS_MPI_CHK( mbedtls_mpi_copy(&seedinc, &seedinc1) );
                /* seedinc = (seedinc+1) % 2^seed_bitlen */
                i = mbedtls_mpi_size(&seedinc);
                if (i > seedbytes) { ret = MBEDTLS_ERR_DSA_BAD_INPUT_DATA; goto cleanup; }
                mbedtls_platform_zeroize(sbuf, seedbytes);
                MBEDTLS_MPI_CHK( mbedtls_mpi_write_binary(&seedinc, sbuf + seedbytes-i, i) );
                i = outbytes;
                MBEDTLS_MPI_CHK( mbedtls_md(md_info, sbuf, seedbytes, wbuf+(n-j)*outbytes) );
            }
            MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary(&W, wbuf, (n+1)*outbytes) );
            MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi(&W1, &W, &t2L1) );
            MBEDTLS_MPI_CHK( mbedtls_mpi_add_mpi(&X, &W1, &t2L1) );
            MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi(&c, &X, &t2q) );
            MBEDTLS_MPI_CHK( mbedtls_mpi_sub_int(&p1, &c, 1) );
            MBEDTLS_MPI_CHK( mbedtls_mpi_sub_mpi(&p, &X, &p1) );
            if (mbedtls_mpi_cmp_mpi(&p, &t2L1) >= 0) {
                /* p >= 2^(L-1) */
                if( mbedtls_mpi_is_prime_ext(&p, mr_tests_p, f_rng, p_rng) == 0 ) found_p = 1;
            }
        }
    }

    /* FIPS-186-4 A.2.1 Unverifiable Generation of the Generator g
    * 1. e = (p - 1)/q
    * 2. h = any integer satisfying: 1 < h < (p - 1)
    *    h could be obtained from a random number generator or from a counter that changes after each use
    * 3. g = h^e mod p
    * 4. if (g == 1), then go to step 2.
    *
    */

    /* e = (p - 1)/q */
    MBEDTLS_MPI_CHK( mbedtls_mpi_sub_int(&p1, &p, 1) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_div_mpi(&e, &c, &p1, &q) );

    i = mbedtls_mpi_size(&p);
    do {
        /* h is random and 1 < h < (p-1) */
        do {
            MBEDTLS_MPI_CHK( mbedtls_mpi_fill_random(&h, i, f_rng, p_rng) );
        } while (mbedtls_mpi_cmp_mpi(&h, &p) >= 0 || mbedtls_mpi_cmp_int(&h, 2) <= 0);
        MBEDTLS_MPI_CHK( mbedtls_mpi_sub_int(&h1, &h, 1) );
        /* g = h^e mod p */
        MBEDTLS_MPI_CHK( mbedtls_mpi_exp_mod(&g, &h1, &e, &p, NULL) );
    } while (mbedtls_mpi_cmp_int(&g, 1) == 0);

    MBEDTLS_MPI_CHK( mbedtls_mpi_copy(&ctx->P, &p) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_copy(&ctx->Q, &q) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_copy(&ctx->G, &g) );

    MBEDTLS_MPI_CHK( mbedtls_mpi_sub_int(&q1, &q, 1) );

    /* private key x should be from range: 1 <= x <= q-1 (see FIPS 186-4 B.1.2) */
    do {
        MBEDTLS_MPI_CHK( mbedtls_mpi_fill_random( &ctx->X, mbedtls_mpi_size( &q1 ), f_rng, p_rng ) );
    } while ( mbedtls_mpi_cmp_int( &ctx->X, 1 ) < 0 || mbedtls_mpi_cmp_mpi( &ctx->X, &q1 ) > 0 );

    /* compute y = g^x mod p */
    MBEDTLS_MPI_CHK( mbedtls_mpi_exp_mod( &ctx->Y, &ctx->G, &ctx->X, &ctx->P, NULL ) );

cleanup:
    mbedtls_mpi_free( &t2L1 );
    mbedtls_mpi_free( &t2N1 );
    mbedtls_mpi_free( &t2seedlen );
    mbedtls_mpi_free( &U );
    mbedtls_mpi_free( &U1 );
    mbedtls_mpi_free( &W );
    mbedtls_mpi_free( &W1 );
    mbedtls_mpi_free( &X );
    mbedtls_mpi_free( &q );
    mbedtls_mpi_free( &q1 );
    mbedtls_mpi_free( &p );
    mbedtls_mpi_free( &p1 );
    mbedtls_mpi_free( &e );
    mbedtls_mpi_free( &h );
    mbedtls_mpi_free( &h1 );
    mbedtls_mpi_free( &g );
    mbedtls_mpi_free( &c );
    mbedtls_mpi_free( &t2q );
    mbedtls_mpi_free( &seedinc );
    mbedtls_mpi_free( &seedinc1 );
    mbedtls_free(sbuf);
cleanup2:
    mbedtls_free(wbuf);
cleanup3:
    return( ret );
}

/*
 * Check group parameters
 */
int mbedtls_dsa_check_pqg( const mbedtls_mpi *P, const mbedtls_mpi *Q, const mbedtls_mpi *G )
{
    int ret = 0;
    mbedtls_mpi P1, T;

    DSA_VALIDATE_RET( P != NULL );
    DSA_VALIDATE_RET( Q != NULL );
    DSA_VALIDATE_RET( G != NULL );

    mbedtls_mpi_init( &P1 );
    mbedtls_mpi_init( &T );

    if( mbedtls_mpi_cmp_int( P, 0 ) == 0 )
    {
        ret = MBEDTLS_ERR_DSA_BAD_INPUT_DATA;
        goto cleanup;
    }

    if( mbedtls_mpi_cmp_int( Q, 0 ) == 0 )
    {
        ret = MBEDTLS_ERR_DSA_BAD_INPUT_DATA;
        goto cleanup;
    }

    if( mbedtls_mpi_cmp_int( G, 0 ) == 0 )
    {
        ret = MBEDTLS_ERR_DSA_BAD_INPUT_DATA;
        goto cleanup;
    }

    /* Check if P-1 is a multiple of Q */
    MBEDTLS_MPI_CHK( mbedtls_mpi_sub_int( &P1, P, 1 ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( &T, &P1, Q ) );
    if( mbedtls_mpi_cmp_int( &T, 0 ) != 0 )
    {
        ret = MBEDTLS_ERR_DSA_BAD_INPUT_DATA;
        goto cleanup;
    }

    /* Check if 1 < G < P */
    if( mbedtls_mpi_cmp_int( G, 1 ) <= 0 || mbedtls_mpi_cmp_mpi( G, P ) >= 0 )
    {
        ret = MBEDTLS_ERR_DSA_BAD_INPUT_DATA;
        goto cleanup;
    }

    /* Check if G^Q mod P = 1 */
    MBEDTLS_MPI_CHK( mbedtls_mpi_exp_mod( &T, G, Q, P, NULL ) );
    if( mbedtls_mpi_cmp_int( &T, 1 ) != 0 )
    {
        ret = MBEDTLS_ERR_DSA_BAD_INPUT_DATA;
        goto cleanup;
    }

cleanup:
    mbedtls_mpi_free( &P1 );
    mbedtls_mpi_free( &T );

    return( ret );
}

/*
 * Check public key
 */
int mbedtls_dsa_check_pubkey( const mbedtls_dsa_context *ctx )
{
    int ret = 0;
    mbedtls_mpi T;

    DSA_VALIDATE_RET( ctx != NULL );

    /* Y must be in [2, P-1] */
    if( mbedtls_mpi_cmp_int( &ctx->Y, 2 ) < 0 ||
        mbedtls_mpi_cmp_mpi( &ctx->Y, &ctx->P ) >= 0 )
    {
        return( MBEDTLS_ERR_DSA_BAD_INPUT_DATA );
    }

    mbedtls_mpi_init( &T );

    /* Y^Q mod P = 1 */
    MBEDTLS_MPI_CHK( mbedtls_mpi_exp_mod( &T, &ctx->Y, &ctx->Q, &ctx->P, NULL ) );
    if( mbedtls_mpi_cmp_int( &T, 1 ) != 0 )
    {
        ret = MBEDTLS_ERR_DSA_BAD_INPUT_DATA;
    }

cleanup:
    mbedtls_mpi_free( &T );
    return( ret );
}

/*
 * Check private key
 */
int mbedtls_dsa_check_privkey( const mbedtls_dsa_context *ctx )
{
    int ret = 0;
    mbedtls_mpi T;

    DSA_VALIDATE_RET( ctx != NULL );

    /* X must be in ]0, Q[ */
    if( mbedtls_mpi_cmp_int( &ctx->X, 0 ) <= 0 ||
        mbedtls_mpi_cmp_mpi( &ctx->X, &ctx->Q ) >= 0 )
    {
        return( MBEDTLS_ERR_DSA_BAD_INPUT_DATA );
    }

    mbedtls_mpi_init( &T );

    /* Y = G^X mod P */
    MBEDTLS_MPI_CHK( mbedtls_mpi_exp_mod( &T, &ctx->G, &ctx->X, &ctx->P, NULL ) );
    if( mbedtls_mpi_cmp_mpi( &T, &ctx->Y ) != 0 )
    {
        ret = MBEDTLS_ERR_DSA_BAD_INPUT_DATA;
    }

cleanup:
    mbedtls_mpi_free( &T );
    return( ret );
}

/*
 * Derive public key from private key
 */
int mbedtls_dsa_pubkey_from_privkey( mbedtls_dsa_context *ctx )
{
    int ret = 0;

    DSA_VALIDATE_RET( ctx != NULL );

    /* X must be in ]0, Q[ */
    if( mbedtls_mpi_cmp_int( &ctx->X, 0 ) <= 0 ||
        mbedtls_mpi_cmp_mpi( &ctx->X, &ctx->Q ) >= 0 )
    {
        return( MBEDTLS_ERR_DSA_BAD_INPUT_DATA );
    }

    /* Y = G^X mod P */
    MBEDTLS_MPI_CHK( mbedtls_mpi_exp_mod( &ctx->Y, &ctx->G, &ctx->X, &ctx->P, NULL ) );

cleanup:
    return( ret );
}

/*
 * Convert a signature (given by context) to ASN.1
 */
static int dsa_signature_to_asn1( const mbedtls_mpi *r, const mbedtls_mpi *s,
                                    unsigned char *sig, size_t *slen )
{
    int ret;
    unsigned char buf[MBEDTLS_DSA_ASN_SIGNATURE_MAX_LEN];
    unsigned char *p = buf + sizeof( buf );
    size_t len = 0;

    MBEDTLS_ASN1_CHK_ADD( len, mbedtls_asn1_write_mpi( &p, buf, s ) );
    MBEDTLS_ASN1_CHK_ADD( len, mbedtls_asn1_write_mpi( &p, buf, r ) );

    MBEDTLS_ASN1_CHK_ADD( len, mbedtls_asn1_write_len( &p, buf, len ) );
    MBEDTLS_ASN1_CHK_ADD( len, mbedtls_asn1_write_tag( &p, buf,
                                       MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) );

    memcpy( sig, p, len );
    *slen = len;

    return( 0 );
}

/*
 * Compute and write signature
 */
int mbedtls_dsa_sign( mbedtls_dsa_context *ctx,
                      const unsigned char *hash, size_t hlen,
                      unsigned char *sig, size_t *slen,
                      int (*f_rng)(void *, unsigned char *, size_t),
                      void *p_rng )
{
    int ret;
    mbedtls_mpi k, r, rp, kq, gcd, s, xr, xrh, xrhkq, h;
    size_t hashlen, qlen;

    DSA_VALIDATE_RET( ctx != NULL );
    DSA_VALIDATE_RET( hash != NULL );
    DSA_VALIDATE_RET( sig != NULL );
    DSA_VALIDATE_RET( slen != NULL );
    DSA_VALIDATE_RET( f_rng != NULL );

    mbedtls_mpi_init( &k );
    mbedtls_mpi_init( &r );
    mbedtls_mpi_init( &rp );
    mbedtls_mpi_init( &kq );
    mbedtls_mpi_init( &gcd );
    mbedtls_mpi_init( &s );
    mbedtls_mpi_init( &xr );
    mbedtls_mpi_init( &xrh );
    mbedtls_mpi_init( &xrhkq );
    mbedtls_mpi_init( &h );

    /* FIPS 186-4 4.7: use leftmost min(bitlen(q), bitlen(hash)) bits of 'hash' */
    qlen = mbedtls_mpi_size( &ctx->Q );
    hashlen = hlen;
    if (hashlen > qlen) hashlen = qlen;

    MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary( &h, hash, hashlen ) );

retry:
    do
    {
        /* gen random k */
        if( mbedtls_mpi_fill_random( &k, qlen, f_rng, p_rng ) != 0 ) goto cleanup;

        /* k should be from range: 1 <= k <= q-1 (see FIPS 186-4 B.2.2) */
        if( mbedtls_mpi_cmp_int( &k, 0 ) <= 0 || mbedtls_mpi_cmp_mpi( &k, &ctx->Q ) >= 0 ) goto retry;

        /* test gcd */
        if( mbedtls_mpi_gcd( &gcd, &k, &ctx->Q ) != 0 ) goto cleanup;

    } while( mbedtls_mpi_cmp_int( &gcd, 1 ) != 0 );

    /* now find 1/k mod q */
    MBEDTLS_MPI_CHK( mbedtls_mpi_inv_mod( &kq, &k, &ctx->Q ) );

    /* now find r = g^k mod p mod q */
    MBEDTLS_MPI_CHK( mbedtls_mpi_exp_mod( &rp, &ctx->G, &k, &ctx->P, NULL ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( &r, &rp, &ctx->Q ) );

    /* now find s = (hash + xr)/k mod q */
    MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi( &xr, &ctx->X, &r ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_add_mpi( &xrh, &xr, &h ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi( &xrhkq, &xrh, &kq ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( &s, &xrhkq, &ctx->Q ) );

    MBEDTLS_MPI_CHK( dsa_signature_to_asn1( &r, &s, sig, slen ) );

cleanup:
    mbedtls_mpi_free( &k );
    mbedtls_mpi_free( &r );
    mbedtls_mpi_init( &rp );
    mbedtls_mpi_init( &kq );
    mbedtls_mpi_init( &gcd );
    mbedtls_mpi_free( &s );
    mbedtls_mpi_init( &xr );
    mbedtls_mpi_init( &xrh );
    mbedtls_mpi_init( &xrhkq );
    mbedtls_mpi_free( &h );

    return( ret );
}

/*
 * Read and check signature
 */
int mbedtls_dsa_verify( mbedtls_dsa_context *ctx,
                        const unsigned char *hash, size_t hlen,
                        const unsigned char *sig, size_t slen )
{
    int ret;
    unsigned char *p = (unsigned char *) sig;
    const unsigned char *end = sig + slen;
    size_t len, hashlen, qlen;
    mbedtls_mpi r, s, h, w, hw, rw, u1, u2, gu1, yu2, u12, u12p, v, _rr;

    DSA_VALIDATE_RET( ctx != NULL );
    DSA_VALIDATE_RET( hash != NULL );
    DSA_VALIDATE_RET( sig != NULL );

    mbedtls_mpi_init( &r );
    mbedtls_mpi_init( &s );
    mbedtls_mpi_init( &h );
    mbedtls_mpi_init( &w );
    mbedtls_mpi_init( &hw );
    mbedtls_mpi_init( &rw );
    mbedtls_mpi_init( &u1 );
    mbedtls_mpi_init( &u2 );
    mbedtls_mpi_init( &gu1 );
    mbedtls_mpi_init( &yu2 );
    mbedtls_mpi_init( &u12 );
    mbedtls_mpi_init( &u12p );
    mbedtls_mpi_init( &v );
    mbedtls_mpi_init( &_rr );

    if( ( ret = mbedtls_asn1_get_tag( &p, end, &len,
                    MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) ) != 0 )
    {
        ret = MBEDTLS_ERR_DSA_BAD_INPUT_DATA;
        goto cleanup;
    }

    if( p + len != end )
    {
        ret = MBEDTLS_ERR_DSA_BAD_INPUT_DATA;
        goto cleanup;
    }

    if( ( ret = mbedtls_asn1_get_mpi( &p, end, &r ) ) != 0 ||
        ( ret = mbedtls_asn1_get_mpi( &p, end, &s ) ) != 0 )
    {
        ret = MBEDTLS_ERR_DSA_BAD_INPUT_DATA;
        goto cleanup;
    }

    if( mbedtls_mpi_cmp_int( &r, 0 ) <= 0 || mbedtls_mpi_cmp_mpi( &r, &ctx->Q ) >= 0 ||
        mbedtls_mpi_cmp_int( &s, 0 ) <= 0 || mbedtls_mpi_cmp_mpi( &s, &ctx->Q ) >= 0 )
    {
        ret = MBEDTLS_ERR_DSA_VERIFY_FAILED;
        goto cleanup;
    }

    /* FIPS 186-4 4.7: use leftmost min(bitlen(q), bitlen(hash)) bits of 'hash' */
    qlen = mbedtls_mpi_size( &ctx->Q );
    hashlen = hlen;
    if (hashlen > qlen) hashlen = qlen;

    MBEDTLS_MPI_CHK( mbedtls_mpi_read_binary( &h, hash, hashlen ) );

    /* w = 1/s mod q */
    MBEDTLS_MPI_CHK( mbedtls_mpi_inv_mod( &w, &s, &ctx->Q ) );

    /* u1 = h*w mod q */
    MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi( &hw, &h, &w ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( &u1, &hw, &ctx->Q ) );

    /* u2 = r*w mod q */
    MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi( &rw, &r, &w ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( &u2, &rw, &ctx->Q ) );

    /* v = g^u1 * y^u2 mod p mod q */
    MBEDTLS_MPI_CHK( mbedtls_mpi_exp_mod( &gu1, &ctx->G, &u1, &ctx->P, &_rr ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_exp_mod( &yu2, &ctx->Y, &u2, &ctx->P, &_rr ) );

    MBEDTLS_MPI_CHK( mbedtls_mpi_mul_mpi( &u12, &gu1, &yu2 ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( &u12p, &u12, &ctx->P ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_mod_mpi( &v, &u12p, &ctx->Q ) );

    /* if r = v then it is verified */
    if( mbedtls_mpi_cmp_mpi( &v, &r ) != 0 )
    {
        ret = MBEDTLS_ERR_DSA_VERIFY_FAILED;
        goto cleanup;
    }

cleanup:
    mbedtls_mpi_free( &r );
    mbedtls_mpi_free( &s );
    mbedtls_mpi_free( &h );
    mbedtls_mpi_free( &w );
    mbedtls_mpi_free( &u1 );
    mbedtls_mpi_free( &u2 );
    mbedtls_mpi_init( &gu1 );
    mbedtls_mpi_init( &yu2 );
    mbedtls_mpi_init( &u12 );
    mbedtls_mpi_init( &u12p );
    mbedtls_mpi_free( &v );
    mbedtls_mpi_free( &_rr );

    return( ret );
}

#endif /* MBEDTLS_DSA_ALT */

#if defined(MBEDTLS_SELF_TEST)

static const char *dsa_test_p_hex = "F7E75FDC469067FFDC4E847C51F452DF27303F51D8F2E3E444924563740179E1";
static const char *dsa_test_q_hex = "87E85B349454331564214218435B420E53368DB1";
static const char *dsa_test_g_hex = "F7E75FDC469067FFDC4E847C51F452DF27303F51D8F2E3E444924563740179E1";
static const char *dsa_test_x_hex = "20B4822143298349213489213498213498213948";
static const char *dsa_test_y_hex = "154285095685091850198501985091850918509185091850918509185091850918509185091850918509185091850918509185091850918509185091850918509185";
static const char *dsa_test_hash_hex = "A9993E364706816ABA3E25717850C26C9CD0D89D";

/*
 * Checkup routine
 */
int mbedtls_dsa_self_test( int verbose )
{
    int ret;
    mbedtls_dsa_context dsa;
    unsigned char sig[MBEDTLS_DSA_ASN_SIGNATURE_MAX_LEN];
    size_t slen;
    unsigned char hash[20];

    mbedtls_dsa_init( &dsa );

    if( verbose != 0 )
        mbedtls_printf( "  DSA key generation: " );

    MBEDTLS_MPI_CHK( mbedtls_mpi_read_string( &dsa.P, 16, dsa_test_p_hex ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_read_string( &dsa.Q, 16, dsa_test_q_hex ) );
    MBEDTLS_MPI_CHK( mbedtls_mpi_read_string( &dsa.G, 16, dsa_test_g_hex ) );
    dsa.len = mbedtls_mpi_size( &dsa.P );

    if( ( ret = mbedtls_dsa_genkey( &dsa, NULL, NULL ) ) != 0 )
    {
        if( verbose != 0 )
            mbedtls_printf( "failed\n" );

        ret = 1;
        goto exit;
    }

    if( verbose != 0 )
        mbedtls_printf( "passed\n  DSA signature: " );

    memcpy( hash, dsa_test_hash_hex, 20 );

    if( ( ret = mbedtls_dsa_write_signature( &dsa, MBEDTLS_MD_SHA1, hash, 20, sig, &slen, NULL, NULL ) ) != 0 )
    {
        if( verbose != 0 )
            mbedtls_printf( "failed\n" );

        ret = 1;
        goto exit;
    }

    if( verbose != 0 )
        mbedtls_printf( "passed\n  DSA verification: " );

    if( ( ret = mbedtls_dsa_read_signature( &dsa, hash, 20, sig, slen ) ) != 0 )
    {
        if( verbose != 0 )
            mbedtls_printf( "failed\n" );

        ret = 1;
        goto exit;
    }

    if( verbose != 0 )
        mbedtls_printf( "passed\n\n" );

exit:
    mbedtls_dsa_free( &dsa );

    return( ret );
}

#endif /* MBEDTLS_SELF_TEST */

#endif /* MBEDTLS_DSA_C */
