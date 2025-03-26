/*
 *  PKCS#12 Personal Information Exchange Syntax
 *
 *  Copyright The Mbed TLS Contributors
 *  SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 *
 *  This file is provided under the Apache License 2.0, or the
 *  GNU General Public License v2.0 or later.
 *
 *  **********
 *  Apache License 2.0:
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  **********
 *
 *  **********
 *  GNU General Public License v2.0 or later:
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
 *
 *  **********
 */
/*
 *  The PKCS #12 Personal Information Exchange Syntax Standard v1.1
 *
 *  http://www.rsa.com/rsalabs/pkcs/files/h11301-wp-pkcs-12v1-1-personal-information-exchange-syntax.pdf
 *  ftp://ftp.rsasecurity.com/pub/pkcs/pkcs-12/pkcs-12v1-1.asn
 */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_PKCS12_C)

#include "mbedtls/pkcs12.h"
#include "mbedtls/asn1.h"
#include "mbedtls/cipher.h"
#include "mbedtls/platform_util.h"

#include <string.h>

#if defined(MBEDTLS_ARC4_C)
#include "mbedtls/arc4.h"
#endif

#if defined(MBEDTLS_DES_C)
#include "mbedtls/des.h"
#endif

#if defined(MBEDTLS_ASN1_PARSE_C)

static int pkcs12_parse_pbe_params( mbedtls_asn1_buf *params,
                                    mbedtls_asn1_buf *salt, int *iterations )
{
    int ret;
    unsigned char **p = &params->p;
    const unsigned char *end = params->p + params->len;

    /*
     *  pkcs-12PbeParams ::= SEQUENCE {
     *    salt          OCTET STRING,
     *    iterations    INTEGER
     *  }
     *
     */
    if( params->tag != ( MBEDTLS_ASN1_CONSTRUCTED | MBEDTLS_ASN1_SEQUENCE ) )
        return( MBEDTLS_ERR_PKCS12_PBE_INVALID_FORMAT +
                MBEDTLS_ERR_ASN1_UNEXPECTED_TAG );

    if( ( ret = mbedtls_asn1_get_tag( p, end, &salt->len, MBEDTLS_ASN1_OCTET_STRING ) ) != 0 )
        return( MBEDTLS_ERR_PKCS12_PBE_INVALID_FORMAT + ret );

    salt->p = *p;
    *p += salt->len;

    if( ( ret = mbedtls_asn1_get_int( p, end, iterations ) ) != 0 )
        return( MBEDTLS_ERR_PKCS12_PBE_INVALID_FORMAT + ret );

    if( *p != end )
        return( MBEDTLS_ERR_PKCS12_PBE_INVALID_FORMAT +
                MBEDTLS_ERR_ASN1_LENGTH_MISMATCH );

    return( 0 );
}

#define PKCS12_MAX_PWDLEN 128

static int pkcs12_pbe_derive_key_iv( mbedtls_asn1_buf *pbe_params, mbedtls_md_type_t md_type,
                                     const unsigned char *pwd,  size_t pwdlen,
                                     unsigned char *key, size_t keylen,
                                     unsigned char *iv,  size_t ivlen )
{
    int ret, iterations = 0;
    mbedtls_asn1_buf salt;
    size_t i;
    unsigned char unipwd[PKCS12_MAX_PWDLEN * 2 + 2];

    if( pwdlen > PKCS12_MAX_PWDLEN )
        return( MBEDTLS_ERR_PKCS12_BAD_INPUT_DATA );

    memset( &salt, 0, sizeof(mbedtls_asn1_buf) );
    memset( &unipwd, 0, sizeof(unipwd) );

    if( ( ret = pkcs12_parse_pbe_params( pbe_params, &salt,
                                         &iterations ) ) != 0 )
        return( ret );

    for( i = 0; i < pwdlen; i++ )
        unipwd[i * 2 + 1] = pwd[i];

    if( ( ret = mbedtls_pkcs12_derivation( key, keylen, unipwd, pwdlen * 2 + 2,
                                   salt.p, salt.len, md_type,
                                   MBEDTLS_PKCS12_DERIVE_KEY, iterations ) ) != 0 )
    {
        return( ret );
    }

    if( iv == NULL || ivlen == 0 )
        return( 0 );

    if( ( ret = mbedtls_pkcs12_derivation( iv, ivlen, unipwd, pwdlen * 2 + 2,
                                   salt.p, salt.len, md_type,
                                   MBEDTLS_PKCS12_DERIVE_IV, iterations ) ) != 0 )
    {
        return( ret );
    }
    return( 0 );
}

#undef PKCS12_MAX_PWDLEN

int mbedtls_pkcs12_pbe_sha1_rc4_128( mbedtls_asn1_buf *pbe_params, int mode,
                             const unsigned char *pwd,  size_t pwdlen,
                             const unsigned char *data, size_t len,
                             unsigned char *output )
{
#if !defined(MBEDTLS_ARC4_C)
    ((void) pbe_params);
    ((void) mode);
    ((void) pwd);
    ((void) pwdlen);
    ((void) data);
    ((void) len);
    ((void) output);
    return( MBEDTLS_ERR_PKCS12_FEATURE_UNAVAILABLE );
#else
    int ret;
    unsigned char key[16];
    mbedtls_arc4_context ctx;
    ((void) mode);

    mbedtls_arc4_init( &ctx );

    if( ( ret = pkcs12_pbe_derive_key_iv( pbe_params, MBEDTLS_MD_SHA1,
                                          pwd, pwdlen,
                                          key, 16, NULL, 0 ) ) != 0 )
    {
        return( ret );
    }

    mbedtls_arc4_setup( &ctx, key, 16 );
    if( ( ret = mbedtls_arc4_crypt( &ctx, len, data, output ) ) != 0 )
        goto exit;

exit:
    mbedtls_platform_zeroize( key, sizeof( key ) );
    mbedtls_arc4_free( &ctx );

    return( ret );
#endif /* MBEDTLS_ARC4_C */
}

int mbedtls_pkcs12_pbe( mbedtls_asn1_buf *pbe_params, int mode,
                mbedtls_cipher_type_t cipher_type, mbedtls_md_type_t md_type,
                const unsigned char *pwd,  size_t pwdlen,
                const unsigned char *data, size_t len,
                unsigned char *output )
{
    int ret, keylen = 0;
    unsigned char key[32];
    unsigned char iv[16];
    const mbedtls_cipher_info_t *cipher_info;
    mbedtls_cipher_context_t cipher_ctx;
    size_t olen = 0;

    cipher_info = mbedtls_cipher_info_from_type( cipher_type );
    if( cipher_info == NULL )
        return( MBEDTLS_ERR_PKCS12_FEATURE_UNAVAILABLE );

    keylen = cipher_info->key_bitlen / 8;

    if( ( ret = pkcs12_pbe_derive_key_iv( pbe_params, md_type, pwd, pwdlen,
                                          key, keylen,
                                          iv, cipher_info->iv_size ) ) != 0 )
    {
        return( ret );
    }

    mbedtls_cipher_init( &cipher_ctx );

    if( ( ret = mbedtls_cipher_setup( &cipher_ctx, cipher_info ) ) != 0 )
        goto exit;

    if( ( ret = mbedtls_cipher_setkey( &cipher_ctx, key, 8 * keylen, (mbedtls_operation_t) mode ) ) != 0 )
        goto exit;

    if( ( ret = mbedtls_cipher_set_iv( &cipher_ctx, iv, cipher_info->iv_size ) ) != 0 )
        goto exit;

    if( ( ret = mbedtls_cipher_reset( &cipher_ctx ) ) != 0 )
        goto exit;

    if( ( ret = mbedtls_cipher_update( &cipher_ctx, data, len,
                                output, &olen ) ) != 0 )
    {
        goto exit;
    }

    if( ( ret = mbedtls_cipher_finish( &cipher_ctx, output + olen, &olen ) ) != 0 )
        ret = MBEDTLS_ERR_PKCS12_PASSWORD_MISMATCH;

exit:
    mbedtls_platform_zeroize( key, sizeof( key ) );
    mbedtls_platform_zeroize( iv,  sizeof( iv  ) );
    mbedtls_cipher_free( &cipher_ctx );

    return( ret );
}

#endif /* MBEDTLS_ASN1_PARSE_C */

static void pkcs12_fill_buffer( unsigned char *data, size_t data_len,
                                const unsigned char *filler, size_t fill_len )
{
    unsigned char *p = data;
    size_t use_len;

    while( data_len > 0 )
    {
        use_len = ( data_len > fill_len ) ? fill_len : data_len;
        memcpy( p, filler, use_len );
        p += use_len;
        data_len -= use_len;
    }
}

int mbedtls_pkcs12_derivation( unsigned char *data, size_t datalen,
                       const unsigned char *pwd, size_t pwdlen,
                       const unsigned char *salt, size_t saltlen,
                       mbedtls_md_type_t md_type, int id, int iterations )
{
    int ret;
    unsigned int j;

    unsigned char diversifier[128];
    unsigned char salt_block[128], pwd_block[128], hash_block[128];
    unsigned char hash_output[MBEDTLS_MD_MAX_SIZE];
    unsigned char *p;
    unsigned char c;

    size_t hlen, use_len, v, i;

    const mbedtls_md_info_t *md_info;
    mbedtls_md_context_t md_ctx;

    // This version only allows max of 64 bytes of password or salt
    if( datalen > 128 || pwdlen > 64 || saltlen > 64 )
        return( MBEDTLS_ERR_PKCS12_BAD_INPUT_DATA );

    md_info = mbedtls_md_info_from_type( md_type );
    if( md_info == NULL )
        return( MBEDTLS_ERR_PKCS12_FEATURE_UNAVAILABLE );

    mbedtls_md_init( &md_ctx );

    if( ( ret = mbedtls_md_setup( &md_ctx, md_info, 0 ) ) != 0 )
        return( ret );
    hlen = mbedtls_md_get_size( md_info );

    if( hlen <= 32 )
        v = 64;
    else
        v = 128;

    memset( diversifier, (unsigned char) id, v );

    pkcs12_fill_buffer( salt_block, v, salt, saltlen );
    pkcs12_fill_buffer( pwd_block,  v, pwd,  pwdlen  );

    p = data;
    while( datalen > 0 )
    {
        // Calculate hash( diversifier || salt_block || pwd_block )
        if( ( ret = mbedtls_md_starts( &md_ctx ) ) != 0 )
            goto exit;

        if( ( ret = mbedtls_md_update( &md_ctx, diversifier, v ) ) != 0 )
            goto exit;

        if( ( ret = mbedtls_md_update( &md_ctx, salt_block, v ) ) != 0 )
            goto exit;

        if( ( ret = mbedtls_md_update( &md_ctx, pwd_block, v ) ) != 0 )
            goto exit;

        if( ( ret = mbedtls_md_finish( &md_ctx, hash_output ) ) != 0 )
            goto exit;

        // Perform remaining ( iterations - 1 ) recursive hash calculations
        for( i = 1; i < (size_t) iterations; i++ )
        {
            if( ( ret = mbedtls_md( md_info, hash_output, hlen, hash_output ) ) != 0 )
                goto exit;
        }

        use_len = ( datalen > hlen ) ? hlen : datalen;
        memcpy( p, hash_output, use_len );
        datalen -= use_len;
        p += use_len;

        if( datalen == 0 )
            break;

        // Concatenating copies of hash_output into hash_block (B)
        pkcs12_fill_buffer( hash_block, v, hash_output, hlen );

        // B += 1
        for( i = v; i > 0; i-- )
            if( ++hash_block[i - 1] != 0 )
                break;

        // salt_block += B
        c = 0;
        for( i = v; i > 0; i-- )
        {
            j = salt_block[i - 1] + hash_block[i - 1] + c;
            c = (unsigned char) (j >> 8);
            salt_block[i - 1] = j & 0xFF;
        }

        // pwd_block  += B
        c = 0;
        for( i = v; i > 0; i-- )
        {
            j = pwd_block[i - 1] + hash_block[i - 1] + c;
            c = (unsigned char) (j >> 8);
            pwd_block[i - 1] = j & 0xFF;
        }
    }

    ret = 0;

exit:
    mbedtls_platform_zeroize( salt_block, sizeof( salt_block ) );
    mbedtls_platform_zeroize( pwd_block, sizeof( pwd_block ) );
    mbedtls_platform_zeroize( hash_block, sizeof( hash_block ) );
    mbedtls_platform_zeroize( hash_output, sizeof( hash_output ) );

    mbedtls_md_free( &md_ctx );

    return( ret );
}

#endif /* MBEDTLS_PKCS12_C */
