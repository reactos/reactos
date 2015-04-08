/**
 * \file pkcs5.c
 *
 * \brief PKCS#5 functions
 *
 * \author Mathias Olsson <mathias@kompetensum.com>
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
 * PKCS#5 includes PBKDF2 and more
 *
 * http://tools.ietf.org/html/rfc2898 (Specification)
 * http://tools.ietf.org/html/rfc6070 (Test vectors)
 */

#if !defined(POLARSSL_CONFIG_FILE)
#include "polarssl/config.h"
#else
#include POLARSSL_CONFIG_FILE
#endif

#if defined(POLARSSL_PKCS5_C)

#include "polarssl/pkcs5.h"
#include "polarssl/asn1.h"
#include "polarssl/cipher.h"
#include "polarssl/oid.h"

#if defined(POLARSSL_PLATFORM_C)
#include "polarssl/platform.h"
#else
#define polarssl_printf printf
#endif

static int pkcs5_parse_pbkdf2_params( const asn1_buf *params,
                                      asn1_buf *salt, int *iterations,
                                      int *keylen, md_type_t *md_type )
{
    int ret;
    asn1_buf prf_alg_oid;
    unsigned char *p = params->p;
    const unsigned char *end = params->p + params->len;

    if( params->tag != ( ASN1_CONSTRUCTED | ASN1_SEQUENCE ) )
        return( POLARSSL_ERR_PKCS5_INVALID_FORMAT +
                POLARSSL_ERR_ASN1_UNEXPECTED_TAG );
    /*
     *  PBKDF2-params ::= SEQUENCE {
     *    salt              OCTET STRING,
     *    iterationCount    INTEGER,
     *    keyLength         INTEGER OPTIONAL
     *    prf               AlgorithmIdentifier DEFAULT algid-hmacWithSHA1
     *  }
     *
     */
    if( ( ret = asn1_get_tag( &p, end, &salt->len, ASN1_OCTET_STRING ) ) != 0 )
        return( POLARSSL_ERR_PKCS5_INVALID_FORMAT + ret );

    salt->p = p;
    p += salt->len;

    if( ( ret = asn1_get_int( &p, end, iterations ) ) != 0 )
        return( POLARSSL_ERR_PKCS5_INVALID_FORMAT + ret );

    if( p == end )
        return( 0 );

    if( ( ret = asn1_get_int( &p, end, keylen ) ) != 0 )
    {
        if( ret != POLARSSL_ERR_ASN1_UNEXPECTED_TAG )
            return( POLARSSL_ERR_PKCS5_INVALID_FORMAT + ret );
    }

    if( p == end )
        return( 0 );

    if( ( ret = asn1_get_alg_null( &p, end, &prf_alg_oid ) ) != 0 )
        return( POLARSSL_ERR_PKCS5_INVALID_FORMAT + ret );

    if( !OID_CMP( OID_HMAC_SHA1, &prf_alg_oid ) )
        return( POLARSSL_ERR_PKCS5_FEATURE_UNAVAILABLE );

    *md_type = POLARSSL_MD_SHA1;

    if( p != end )
        return( POLARSSL_ERR_PKCS5_INVALID_FORMAT +
                POLARSSL_ERR_ASN1_LENGTH_MISMATCH );

    return( 0 );
}

int pkcs5_pbes2( asn1_buf *pbe_params, int mode,
                 const unsigned char *pwd,  size_t pwdlen,
                 const unsigned char *data, size_t datalen,
                 unsigned char *output )
{
    int ret, iterations = 0, keylen = 0;
    unsigned char *p, *end;
    asn1_buf kdf_alg_oid, enc_scheme_oid, kdf_alg_params, enc_scheme_params;
    asn1_buf salt;
    md_type_t md_type = POLARSSL_MD_SHA1;
    unsigned char key[32], iv[32];
    size_t olen = 0;
    const md_info_t *md_info;
    const cipher_info_t *cipher_info;
    md_context_t md_ctx;
    cipher_type_t cipher_alg;
    cipher_context_t cipher_ctx;

    p = pbe_params->p;
    end = p + pbe_params->len;

    /*
     *  PBES2-params ::= SEQUENCE {
     *    keyDerivationFunc AlgorithmIdentifier {{PBES2-KDFs}},
     *    encryptionScheme AlgorithmIdentifier {{PBES2-Encs}}
     *  }
     */
    if( pbe_params->tag != ( ASN1_CONSTRUCTED | ASN1_SEQUENCE ) )
        return( POLARSSL_ERR_PKCS5_INVALID_FORMAT +
                POLARSSL_ERR_ASN1_UNEXPECTED_TAG );

    if( ( ret = asn1_get_alg( &p, end, &kdf_alg_oid, &kdf_alg_params ) ) != 0 )
        return( POLARSSL_ERR_PKCS5_INVALID_FORMAT + ret );

    // Only PBKDF2 supported at the moment
    //
    if( !OID_CMP( OID_PKCS5_PBKDF2, &kdf_alg_oid ) )
        return( POLARSSL_ERR_PKCS5_FEATURE_UNAVAILABLE );

    if( ( ret = pkcs5_parse_pbkdf2_params( &kdf_alg_params,
                                           &salt, &iterations, &keylen,
                                           &md_type ) ) != 0 )
    {
        return( ret );
    }

    md_info = md_info_from_type( md_type );
    if( md_info == NULL )
        return( POLARSSL_ERR_PKCS5_FEATURE_UNAVAILABLE );

    if( ( ret = asn1_get_alg( &p, end, &enc_scheme_oid,
                              &enc_scheme_params ) ) != 0 )
    {
        return( POLARSSL_ERR_PKCS5_INVALID_FORMAT + ret );
    }

    if( oid_get_cipher_alg( &enc_scheme_oid, &cipher_alg ) != 0 )
        return( POLARSSL_ERR_PKCS5_FEATURE_UNAVAILABLE );

    cipher_info = cipher_info_from_type( cipher_alg );
    if( cipher_info == NULL )
        return( POLARSSL_ERR_PKCS5_FEATURE_UNAVAILABLE );

    /*
     * The value of keylen from pkcs5_parse_pbkdf2_params() is ignored
     * since it is optional and we don't know if it was set or not
     */
    keylen = cipher_info->key_length / 8;

    if( enc_scheme_params.tag != ASN1_OCTET_STRING ||
        enc_scheme_params.len != cipher_info->iv_size )
    {
        return( POLARSSL_ERR_PKCS5_INVALID_FORMAT );
    }

    md_init( &md_ctx );
    cipher_init( &cipher_ctx );

    memcpy( iv, enc_scheme_params.p, enc_scheme_params.len );

    if( ( ret = md_init_ctx( &md_ctx, md_info ) ) != 0 )
        goto exit;

    if( ( ret = pkcs5_pbkdf2_hmac( &md_ctx, pwd, pwdlen, salt.p, salt.len,
                                   iterations, keylen, key ) ) != 0 )
    {
        goto exit;
    }

    if( ( ret = cipher_init_ctx( &cipher_ctx, cipher_info ) ) != 0 )
        goto exit;

    if( ( ret = cipher_setkey( &cipher_ctx, key, 8 * keylen, mode ) ) != 0 )
        goto exit;

    if( ( ret = cipher_crypt( &cipher_ctx, iv, enc_scheme_params.len,
                              data, datalen, output, &olen ) ) != 0 )
        ret = POLARSSL_ERR_PKCS5_PASSWORD_MISMATCH;

exit:
    md_free( &md_ctx );
    cipher_free( &cipher_ctx );

    return( ret );
}

int pkcs5_pbkdf2_hmac( md_context_t *ctx, const unsigned char *password,
                       size_t plen, const unsigned char *salt, size_t slen,
                       unsigned int iteration_count,
                       uint32_t key_length, unsigned char *output )
{
    int ret, j;
    unsigned int i;
    unsigned char md1[POLARSSL_MD_MAX_SIZE];
    unsigned char work[POLARSSL_MD_MAX_SIZE];
    unsigned char md_size = md_get_size( ctx->md_info );
    size_t use_len;
    unsigned char *out_p = output;
    unsigned char counter[4];

    memset( counter, 0, 4 );
    counter[3] = 1;

    if( iteration_count > 0xFFFFFFFF )
        return( POLARSSL_ERR_PKCS5_BAD_INPUT_DATA );

    while( key_length )
    {
        // U1 ends up in work
        //
        if( ( ret = md_hmac_starts( ctx, password, plen ) ) != 0 )
            return( ret );

        if( ( ret = md_hmac_update( ctx, salt, slen ) ) != 0 )
            return( ret );

        if( ( ret = md_hmac_update( ctx, counter, 4 ) ) != 0 )
            return( ret );

        if( ( ret = md_hmac_finish( ctx, work ) ) != 0 )
            return( ret );

        memcpy( md1, work, md_size );

        for( i = 1; i < iteration_count; i++ )
        {
            // U2 ends up in md1
            //
            if( ( ret = md_hmac_starts( ctx, password, plen ) ) != 0 )
                return( ret );

            if( ( ret = md_hmac_update( ctx, md1, md_size ) ) != 0 )
                return( ret );

            if( ( ret = md_hmac_finish( ctx, md1 ) ) != 0 )
                return( ret );

            // U1 xor U2
            //
            for( j = 0; j < md_size; j++ )
                work[j] ^= md1[j];
        }

        use_len = ( key_length < md_size ) ? key_length : md_size;
        memcpy( out_p, work, use_len );

        key_length -= (uint32_t) use_len;
        out_p += use_len;

        for( i = 4; i > 0; i-- )
            if( ++counter[i - 1] != 0 )
                break;
    }

    return( 0 );
}

#if defined(POLARSSL_SELF_TEST)

#if !defined(POLARSSL_SHA1_C)
int pkcs5_self_test( int verbose )
{
    if( verbose != 0 )
        polarssl_printf( "  PBKDF2 (SHA1): skipped\n\n" );

    return( 0 );
}
#else

#include <stdio.h>

#define MAX_TESTS   6

size_t plen[MAX_TESTS] =
    { 8, 8, 8, 8, 24, 9 };

unsigned char password[MAX_TESTS][32] =
{
    "password",
    "password",
    "password",
    "password",
    "passwordPASSWORDpassword",
    "pass\0word",
};

size_t slen[MAX_TESTS] =
    { 4, 4, 4, 4, 36, 5 };

unsigned char salt[MAX_TESTS][40] =
{
    "salt",
    "salt",
    "salt",
    "salt",
    "saltSALTsaltSALTsaltSALTsaltSALTsalt",
    "sa\0lt",
};

uint32_t it_cnt[MAX_TESTS] =
    { 1, 2, 4096, 16777216, 4096, 4096 };

uint32_t key_len[MAX_TESTS] =
    { 20, 20, 20, 20, 25, 16 };


unsigned char result_key[MAX_TESTS][32] =
{
    { 0x0c, 0x60, 0xc8, 0x0f, 0x96, 0x1f, 0x0e, 0x71,
      0xf3, 0xa9, 0xb5, 0x24, 0xaf, 0x60, 0x12, 0x06,
      0x2f, 0xe0, 0x37, 0xa6 },
    { 0xea, 0x6c, 0x01, 0x4d, 0xc7, 0x2d, 0x6f, 0x8c,
      0xcd, 0x1e, 0xd9, 0x2a, 0xce, 0x1d, 0x41, 0xf0,
      0xd8, 0xde, 0x89, 0x57 },
    { 0x4b, 0x00, 0x79, 0x01, 0xb7, 0x65, 0x48, 0x9a,
      0xbe, 0xad, 0x49, 0xd9, 0x26, 0xf7, 0x21, 0xd0,
      0x65, 0xa4, 0x29, 0xc1 },
    { 0xee, 0xfe, 0x3d, 0x61, 0xcd, 0x4d, 0xa4, 0xe4,
      0xe9, 0x94, 0x5b, 0x3d, 0x6b, 0xa2, 0x15, 0x8c,
      0x26, 0x34, 0xe9, 0x84 },
    { 0x3d, 0x2e, 0xec, 0x4f, 0xe4, 0x1c, 0x84, 0x9b,
      0x80, 0xc8, 0xd8, 0x36, 0x62, 0xc0, 0xe4, 0x4a,
      0x8b, 0x29, 0x1a, 0x96, 0x4c, 0xf2, 0xf0, 0x70,
      0x38 },
    { 0x56, 0xfa, 0x6a, 0xa7, 0x55, 0x48, 0x09, 0x9d,
      0xcc, 0x37, 0xd7, 0xf0, 0x34, 0x25, 0xe0, 0xc3 },
};

int pkcs5_self_test( int verbose )
{
    md_context_t sha1_ctx;
    const md_info_t *info_sha1;
    int ret, i;
    unsigned char key[64];

    md_init( &sha1_ctx );

    info_sha1 = md_info_from_type( POLARSSL_MD_SHA1 );
    if( info_sha1 == NULL )
    {
        ret = 1;
        goto exit;
    }

    if( ( ret = md_init_ctx( &sha1_ctx, info_sha1 ) ) != 0 )
    {
        ret = 1;
        goto exit;
    }

    if( verbose != 0 )
        polarssl_printf( "  PBKDF2 note: test #3 may be slow!\n" );

    for( i = 0; i < MAX_TESTS; i++ )
    {
        if( verbose != 0 )
            polarssl_printf( "  PBKDF2 (SHA1) #%d: ", i );

        ret = pkcs5_pbkdf2_hmac( &sha1_ctx, password[i], plen[i], salt[i],
                                  slen[i], it_cnt[i], key_len[i], key );
        if( ret != 0 ||
            memcmp( result_key[i], key, key_len[i] ) != 0 )
        {
            if( verbose != 0 )
                polarssl_printf( "failed\n" );

            ret = 1;
            goto exit;
        }

        if( verbose != 0 )
            polarssl_printf( "passed\n" );
    }

    polarssl_printf( "\n" );

exit:
    md_free( &sha1_ctx );

    return( ret );
}
#endif /* POLARSSL_SHA1_C */

#endif /* POLARSSL_SELF_TEST */

#endif /* POLARSSL_PKCS5_C */
