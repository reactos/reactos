/*
 *  Public Key abstraction layer: wrapper functions
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

#if defined(POLARSSL_PK_C)

#include "polarssl/pk_wrap.h"

/* Even if RSA not activated, for the sake of RSA-alt */
#include "polarssl/rsa.h"

#if defined(POLARSSL_ECP_C)
#include "polarssl/ecp.h"
#endif

#if defined(POLARSSL_ECDSA_C)
#include "polarssl/ecdsa.h"
#endif

#if defined(POLARSSL_PLATFORM_C)
#include "polarssl/platform.h"
#else
#include <stdlib.h>
#define polarssl_malloc     malloc
#define polarssl_free       free
#endif

/* Implementation that should never be optimized out by the compiler */
static void polarssl_zeroize( void *v, size_t n ) {
    volatile unsigned char *p = v; while( n-- ) *p++ = 0;
}

#if defined(POLARSSL_RSA_C)
static int rsa_can_do( pk_type_t type )
{
    return( type == POLARSSL_PK_RSA ||
            type == POLARSSL_PK_RSASSA_PSS );
}

static size_t rsa_get_size( const void *ctx )
{
    return( 8 * ((const rsa_context *) ctx)->len );
}

static int rsa_verify_wrap( void *ctx, md_type_t md_alg,
                   const unsigned char *hash, size_t hash_len,
                   const unsigned char *sig, size_t sig_len )
{
    int ret;

    if( sig_len < ((rsa_context *) ctx)->len )
        return( POLARSSL_ERR_RSA_VERIFY_FAILED );

    if( ( ret = rsa_pkcs1_verify( (rsa_context *) ctx, NULL, NULL,
                                  RSA_PUBLIC, md_alg,
                                  (unsigned int) hash_len, hash, sig ) ) != 0 )
        return( ret );

    if( sig_len > ((rsa_context *) ctx)->len )
        return( POLARSSL_ERR_PK_SIG_LEN_MISMATCH );

    return( 0 );
}

static int rsa_sign_wrap( void *ctx, md_type_t md_alg,
                   const unsigned char *hash, size_t hash_len,
                   unsigned char *sig, size_t *sig_len,
                   int (*f_rng)(void *, unsigned char *, size_t), void *p_rng )
{
    *sig_len = ((rsa_context *) ctx)->len;

    return( rsa_pkcs1_sign( (rsa_context *) ctx, f_rng, p_rng, RSA_PRIVATE,
                md_alg, (unsigned int) hash_len, hash, sig ) );
}

static int rsa_decrypt_wrap( void *ctx,
                    const unsigned char *input, size_t ilen,
                    unsigned char *output, size_t *olen, size_t osize,
                    int (*f_rng)(void *, unsigned char *, size_t), void *p_rng )
{
    if( ilen != ((rsa_context *) ctx)->len )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    return( rsa_pkcs1_decrypt( (rsa_context *) ctx, f_rng, p_rng,
                RSA_PRIVATE, olen, input, output, osize ) );
}

static int rsa_encrypt_wrap( void *ctx,
                    const unsigned char *input, size_t ilen,
                    unsigned char *output, size_t *olen, size_t osize,
                    int (*f_rng)(void *, unsigned char *, size_t), void *p_rng )
{
    *olen = ((rsa_context *) ctx)->len;

    if( *olen > osize )
        return( POLARSSL_ERR_RSA_OUTPUT_TOO_LARGE );

    return( rsa_pkcs1_encrypt( (rsa_context *) ctx,
                f_rng, p_rng, RSA_PUBLIC, ilen, input, output ) );
}

static int rsa_check_pair_wrap( const void *pub, const void *prv )
{
    return( rsa_check_pub_priv( (const rsa_context *) pub,
                                (const rsa_context *) prv ) );
}

static void *rsa_alloc_wrap( void )
{
    void *ctx = polarssl_malloc( sizeof( rsa_context ) );

    if( ctx != NULL )
        rsa_init( (rsa_context *) ctx, 0, 0 );

    return( ctx );
}

static void rsa_free_wrap( void *ctx )
{
    rsa_free( (rsa_context *) ctx );
    polarssl_free( ctx );
}

static void rsa_debug( const void *ctx, pk_debug_item *items )
{
    items->type = POLARSSL_PK_DEBUG_MPI;
    items->name = "rsa.N";
    items->value = &( ((rsa_context *) ctx)->N );

    items++;

    items->type = POLARSSL_PK_DEBUG_MPI;
    items->name = "rsa.E";
    items->value = &( ((rsa_context *) ctx)->E );
}

const pk_info_t rsa_info = {
    POLARSSL_PK_RSA,
    "RSA",
    rsa_get_size,
    rsa_can_do,
    rsa_verify_wrap,
    rsa_sign_wrap,
    rsa_decrypt_wrap,
    rsa_encrypt_wrap,
    rsa_check_pair_wrap,
    rsa_alloc_wrap,
    rsa_free_wrap,
    rsa_debug,
};
#endif /* POLARSSL_RSA_C */

#if defined(POLARSSL_ECP_C)
/*
 * Generic EC key
 */
static int eckey_can_do( pk_type_t type )
{
    return( type == POLARSSL_PK_ECKEY ||
            type == POLARSSL_PK_ECKEY_DH ||
            type == POLARSSL_PK_ECDSA );
}

static size_t eckey_get_size( const void *ctx )
{
    return( ((ecp_keypair *) ctx)->grp.pbits );
}

#if defined(POLARSSL_ECDSA_C)
/* Forward declarations */
static int ecdsa_verify_wrap( void *ctx, md_type_t md_alg,
                       const unsigned char *hash, size_t hash_len,
                       const unsigned char *sig, size_t sig_len );

static int ecdsa_sign_wrap( void *ctx, md_type_t md_alg,
                   const unsigned char *hash, size_t hash_len,
                   unsigned char *sig, size_t *sig_len,
                   int (*f_rng)(void *, unsigned char *, size_t), void *p_rng );

static int eckey_verify_wrap( void *ctx, md_type_t md_alg,
                       const unsigned char *hash, size_t hash_len,
                       const unsigned char *sig, size_t sig_len )
{
    int ret;
    ecdsa_context ecdsa;

    ecdsa_init( &ecdsa );

    if( ( ret = ecdsa_from_keypair( &ecdsa, ctx ) ) == 0 )
        ret = ecdsa_verify_wrap( &ecdsa, md_alg, hash, hash_len, sig, sig_len );

    ecdsa_free( &ecdsa );

    return( ret );
}

static int eckey_sign_wrap( void *ctx, md_type_t md_alg,
                   const unsigned char *hash, size_t hash_len,
                   unsigned char *sig, size_t *sig_len,
                   int (*f_rng)(void *, unsigned char *, size_t), void *p_rng )
{
    int ret;
    ecdsa_context ecdsa;

    ecdsa_init( &ecdsa );

    if( ( ret = ecdsa_from_keypair( &ecdsa, ctx ) ) == 0 )
        ret = ecdsa_sign_wrap( &ecdsa, md_alg, hash, hash_len, sig, sig_len,
                               f_rng, p_rng );

    ecdsa_free( &ecdsa );

    return( ret );
}

#endif /* POLARSSL_ECDSA_C */

static int eckey_check_pair( const void *pub, const void *prv )
{
    return( ecp_check_pub_priv( (const ecp_keypair *) pub,
                                (const ecp_keypair *) prv ) );
}

static void *eckey_alloc_wrap( void )
{
    void *ctx = polarssl_malloc( sizeof( ecp_keypair ) );

    if( ctx != NULL )
        ecp_keypair_init( ctx );

    return( ctx );
}

static void eckey_free_wrap( void *ctx )
{
    ecp_keypair_free( (ecp_keypair *) ctx );
    polarssl_free( ctx );
}

static void eckey_debug( const void *ctx, pk_debug_item *items )
{
    items->type = POLARSSL_PK_DEBUG_ECP;
    items->name = "eckey.Q";
    items->value = &( ((ecp_keypair *) ctx)->Q );
}

const pk_info_t eckey_info = {
    POLARSSL_PK_ECKEY,
    "EC",
    eckey_get_size,
    eckey_can_do,
#if defined(POLARSSL_ECDSA_C)
    eckey_verify_wrap,
    eckey_sign_wrap,
#else
    NULL,
    NULL,
#endif
    NULL,
    NULL,
    eckey_check_pair,
    eckey_alloc_wrap,
    eckey_free_wrap,
    eckey_debug,
};

/*
 * EC key restricted to ECDH
 */
static int eckeydh_can_do( pk_type_t type )
{
    return( type == POLARSSL_PK_ECKEY ||
            type == POLARSSL_PK_ECKEY_DH );
}

const pk_info_t eckeydh_info = {
    POLARSSL_PK_ECKEY_DH,
    "EC_DH",
    eckey_get_size,         /* Same underlying key structure */
    eckeydh_can_do,
    NULL,
    NULL,
    NULL,
    NULL,
    eckey_check_pair,
    eckey_alloc_wrap,       /* Same underlying key structure */
    eckey_free_wrap,        /* Same underlying key structure */
    eckey_debug,            /* Same underlying key structure */
};
#endif /* POLARSSL_ECP_C */

#if defined(POLARSSL_ECDSA_C)
static int ecdsa_can_do( pk_type_t type )
{
    return( type == POLARSSL_PK_ECDSA );
}

static int ecdsa_verify_wrap( void *ctx, md_type_t md_alg,
                       const unsigned char *hash, size_t hash_len,
                       const unsigned char *sig, size_t sig_len )
{
    int ret;
    ((void) md_alg);

    ret = ecdsa_read_signature( (ecdsa_context *) ctx,
                                hash, hash_len, sig, sig_len );

    if( ret == POLARSSL_ERR_ECP_SIG_LEN_MISMATCH )
        return( POLARSSL_ERR_PK_SIG_LEN_MISMATCH );

    return( ret );
}

static int ecdsa_sign_wrap( void *ctx, md_type_t md_alg,
                   const unsigned char *hash, size_t hash_len,
                   unsigned char *sig, size_t *sig_len,
                   int (*f_rng)(void *, unsigned char *, size_t), void *p_rng )
{
    /* Use deterministic ECDSA by default if available */
#if defined(POLARSSL_ECDSA_DETERMINISTIC)
    ((void) f_rng);
    ((void) p_rng);

    return( ecdsa_write_signature_det( (ecdsa_context *) ctx,
                hash, hash_len, sig, sig_len, md_alg ) );
#else
    ((void) md_alg);

    return( ecdsa_write_signature( (ecdsa_context *) ctx,
                hash, hash_len, sig, sig_len, f_rng, p_rng ) );
#endif /* POLARSSL_ECDSA_DETERMINISTIC */
}

static void *ecdsa_alloc_wrap( void )
{
    void *ctx = polarssl_malloc( sizeof( ecdsa_context ) );

    if( ctx != NULL )
        ecdsa_init( (ecdsa_context *) ctx );

    return( ctx );
}

static void ecdsa_free_wrap( void *ctx )
{
    ecdsa_free( (ecdsa_context *) ctx );
    polarssl_free( ctx );
}

const pk_info_t ecdsa_info = {
    POLARSSL_PK_ECDSA,
    "ECDSA",
    eckey_get_size,     /* Compatible key structures */
    ecdsa_can_do,
    ecdsa_verify_wrap,
    ecdsa_sign_wrap,
    NULL,
    NULL,
    eckey_check_pair,   /* Compatible key structures */
    ecdsa_alloc_wrap,
    ecdsa_free_wrap,
    eckey_debug,        /* Compatible key structures */
};
#endif /* POLARSSL_ECDSA_C */

/*
 * Support for alternative RSA-private implementations
 */

static int rsa_alt_can_do( pk_type_t type )
{
    return( type == POLARSSL_PK_RSA );
}

static size_t rsa_alt_get_size( const void *ctx )
{
    const rsa_alt_context *rsa_alt = (const rsa_alt_context *) ctx;

    return( 8 * rsa_alt->key_len_func( rsa_alt->key ) );
}

static int rsa_alt_sign_wrap( void *ctx, md_type_t md_alg,
                   const unsigned char *hash, size_t hash_len,
                   unsigned char *sig, size_t *sig_len,
                   int (*f_rng)(void *, unsigned char *, size_t), void *p_rng )
{
    rsa_alt_context *rsa_alt = (rsa_alt_context *) ctx;

    *sig_len = rsa_alt->key_len_func( rsa_alt->key );

    return( rsa_alt->sign_func( rsa_alt->key, f_rng, p_rng, RSA_PRIVATE,
                md_alg, (unsigned int) hash_len, hash, sig ) );
}

static int rsa_alt_decrypt_wrap( void *ctx,
                    const unsigned char *input, size_t ilen,
                    unsigned char *output, size_t *olen, size_t osize,
                    int (*f_rng)(void *, unsigned char *, size_t), void *p_rng )
{
    rsa_alt_context *rsa_alt = (rsa_alt_context *) ctx;

    ((void) f_rng);
    ((void) p_rng);

    if( ilen != rsa_alt->key_len_func( rsa_alt->key ) )
        return( POLARSSL_ERR_RSA_BAD_INPUT_DATA );

    return( rsa_alt->decrypt_func( rsa_alt->key,
                RSA_PRIVATE, olen, input, output, osize ) );
}

#if defined(POLARSSL_RSA_C)
static int rsa_alt_check_pair( const void *pub, const void *prv )
{
    unsigned char sig[POLARSSL_MPI_MAX_SIZE];
    unsigned char hash[32];
    size_t sig_len = 0;
    int ret;

    if( rsa_alt_get_size( prv ) != rsa_get_size( pub ) )
        return( POLARSSL_ERR_RSA_KEY_CHECK_FAILED );

    memset( hash, 0x2a, sizeof( hash ) );

    if( ( ret = rsa_alt_sign_wrap( (void *) prv, POLARSSL_MD_NONE,
                                   hash, sizeof( hash ),
                                   sig, &sig_len, NULL, NULL ) ) != 0 )
    {
        return( ret );
    }

    if( rsa_verify_wrap( (void *) pub, POLARSSL_MD_NONE,
                         hash, sizeof( hash ), sig, sig_len ) != 0 )
    {
        return( POLARSSL_ERR_RSA_KEY_CHECK_FAILED );
    }

    return( 0 );
}
#endif /* POLARSSL_RSA_C */

static void *rsa_alt_alloc_wrap( void )
{
    void *ctx = polarssl_malloc( sizeof( rsa_alt_context ) );

    if( ctx != NULL )
        memset( ctx, 0, sizeof( rsa_alt_context ) );

    return( ctx );
}

static void rsa_alt_free_wrap( void *ctx )
{
    polarssl_zeroize( ctx, sizeof( rsa_alt_context ) );
    polarssl_free( ctx );
}

const pk_info_t rsa_alt_info = {
    POLARSSL_PK_RSA_ALT,
    "RSA-alt",
    rsa_alt_get_size,
    rsa_alt_can_do,
    NULL,
    rsa_alt_sign_wrap,
    rsa_alt_decrypt_wrap,
    NULL,
#if defined(POLARSSL_RSA_C)
    rsa_alt_check_pair,
#else
    NULL,
#endif
    rsa_alt_alloc_wrap,
    rsa_alt_free_wrap,
    NULL,
};

#endif /* POLARSSL_PK_C */
