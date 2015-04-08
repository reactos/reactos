/*
 *  Privacy Enhanced Mail (PEM) decoding
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

#if defined(POLARSSL_PEM_PARSE_C) || defined(POLARSSL_PEM_WRITE_C)
#include "polarssl/pem.h"
#include "polarssl/base64.h"
#include "polarssl/des.h"
#include "polarssl/aes.h"
#include "polarssl/md5.h"
#include "polarssl/cipher.h"

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

#if defined(POLARSSL_PEM_PARSE_C)
void pem_init( pem_context *ctx )
{
    memset( ctx, 0, sizeof( pem_context ) );
}

#if defined(POLARSSL_MD5_C) && defined(POLARSSL_CIPHER_MODE_CBC) &&         \
    ( defined(POLARSSL_DES_C) || defined(POLARSSL_AES_C) )
/*
 * Read a 16-byte hex string and convert it to binary
 */
static int pem_get_iv( const unsigned char *s, unsigned char *iv,
                       size_t iv_len )
{
    size_t i, j, k;

    memset( iv, 0, iv_len );

    for( i = 0; i < iv_len * 2; i++, s++ )
    {
        if( *s >= '0' && *s <= '9' ) j = *s - '0'; else
        if( *s >= 'A' && *s <= 'F' ) j = *s - '7'; else
        if( *s >= 'a' && *s <= 'f' ) j = *s - 'W'; else
            return( POLARSSL_ERR_PEM_INVALID_ENC_IV );

        k = ( ( i & 1 ) != 0 ) ? j : j << 4;

        iv[i >> 1] = (unsigned char)( iv[i >> 1] | k );
    }

    return( 0 );
}

static void pem_pbkdf1( unsigned char *key, size_t keylen,
                        unsigned char *iv,
                        const unsigned char *pwd, size_t pwdlen )
{
    md5_context md5_ctx;
    unsigned char md5sum[16];
    size_t use_len;

    md5_init( &md5_ctx );

    /*
     * key[ 0..15] = MD5(pwd || IV)
     */
    md5_starts( &md5_ctx );
    md5_update( &md5_ctx, pwd, pwdlen );
    md5_update( &md5_ctx, iv,  8 );
    md5_finish( &md5_ctx, md5sum );

    if( keylen <= 16 )
    {
        memcpy( key, md5sum, keylen );

        md5_free( &md5_ctx );
        polarssl_zeroize( md5sum, 16 );
        return;
    }

    memcpy( key, md5sum, 16 );

    /*
     * key[16..23] = MD5(key[ 0..15] || pwd || IV])
     */
    md5_starts( &md5_ctx );
    md5_update( &md5_ctx, md5sum,  16 );
    md5_update( &md5_ctx, pwd, pwdlen );
    md5_update( &md5_ctx, iv,  8 );
    md5_finish( &md5_ctx, md5sum );

    use_len = 16;
    if( keylen < 32 )
        use_len = keylen - 16;

    memcpy( key + 16, md5sum, use_len );

    md5_free( &md5_ctx );
    polarssl_zeroize( md5sum, 16 );
}

#if defined(POLARSSL_DES_C)
/*
 * Decrypt with DES-CBC, using PBKDF1 for key derivation
 */
static void pem_des_decrypt( unsigned char des_iv[8],
                               unsigned char *buf, size_t buflen,
                               const unsigned char *pwd, size_t pwdlen )
{
    des_context des_ctx;
    unsigned char des_key[8];

    des_init( &des_ctx );

    pem_pbkdf1( des_key, 8, des_iv, pwd, pwdlen );

    des_setkey_dec( &des_ctx, des_key );
    des_crypt_cbc( &des_ctx, DES_DECRYPT, buflen,
                     des_iv, buf, buf );

    des_free( &des_ctx );
    polarssl_zeroize( des_key, 8 );
}

/*
 * Decrypt with 3DES-CBC, using PBKDF1 for key derivation
 */
static void pem_des3_decrypt( unsigned char des3_iv[8],
                               unsigned char *buf, size_t buflen,
                               const unsigned char *pwd, size_t pwdlen )
{
    des3_context des3_ctx;
    unsigned char des3_key[24];

    des3_init( &des3_ctx );

    pem_pbkdf1( des3_key, 24, des3_iv, pwd, pwdlen );

    des3_set3key_dec( &des3_ctx, des3_key );
    des3_crypt_cbc( &des3_ctx, DES_DECRYPT, buflen,
                     des3_iv, buf, buf );

    des3_free( &des3_ctx );
    polarssl_zeroize( des3_key, 24 );
}
#endif /* POLARSSL_DES_C */

#if defined(POLARSSL_AES_C)
/*
 * Decrypt with AES-XXX-CBC, using PBKDF1 for key derivation
 */
static void pem_aes_decrypt( unsigned char aes_iv[16], unsigned int keylen,
                               unsigned char *buf, size_t buflen,
                               const unsigned char *pwd, size_t pwdlen )
{
    aes_context aes_ctx;
    unsigned char aes_key[32];

    aes_init( &aes_ctx );

    pem_pbkdf1( aes_key, keylen, aes_iv, pwd, pwdlen );

    aes_setkey_dec( &aes_ctx, aes_key, keylen * 8 );
    aes_crypt_cbc( &aes_ctx, AES_DECRYPT, buflen,
                     aes_iv, buf, buf );

    aes_free( &aes_ctx );
    polarssl_zeroize( aes_key, keylen );
}
#endif /* POLARSSL_AES_C */

#endif /* POLARSSL_MD5_C && POLARSSL_CIPHER_MODE_CBC &&
          ( POLARSSL_AES_C || POLARSSL_DES_C ) */

int pem_read_buffer( pem_context *ctx, const char *header, const char *footer,
                     const unsigned char *data, const unsigned char *pwd,
                     size_t pwdlen, size_t *use_len )
{
    int ret, enc;
    size_t len;
    unsigned char *buf;
    const unsigned char *s1, *s2, *end;
#if defined(POLARSSL_MD5_C) && defined(POLARSSL_CIPHER_MODE_CBC) &&         \
    ( defined(POLARSSL_DES_C) || defined(POLARSSL_AES_C) )
    unsigned char pem_iv[16];
    cipher_type_t enc_alg = POLARSSL_CIPHER_NONE;
#else
    ((void) pwd);
    ((void) pwdlen);
#endif /* POLARSSL_MD5_C && POLARSSL_CIPHER_MODE_CBC &&
          ( POLARSSL_AES_C || POLARSSL_DES_C ) */

    if( ctx == NULL )
        return( POLARSSL_ERR_PEM_BAD_INPUT_DATA );

    s1 = (unsigned char *) strstr( (const char *) data, header );

    if( s1 == NULL )
        return( POLARSSL_ERR_PEM_NO_HEADER_FOOTER_PRESENT );

    s2 = (unsigned char *) strstr( (const char *) data, footer );

    if( s2 == NULL || s2 <= s1 )
        return( POLARSSL_ERR_PEM_NO_HEADER_FOOTER_PRESENT );

    s1 += strlen( header );
    if( *s1 == '\r' ) s1++;
    if( *s1 == '\n' ) s1++;
    else return( POLARSSL_ERR_PEM_NO_HEADER_FOOTER_PRESENT );

    end = s2;
    end += strlen( footer );
    if( *end == '\r' ) end++;
    if( *end == '\n' ) end++;
    *use_len = end - data;

    enc = 0;

    if( memcmp( s1, "Proc-Type: 4,ENCRYPTED", 22 ) == 0 )
    {
#if defined(POLARSSL_MD5_C) && defined(POLARSSL_CIPHER_MODE_CBC) &&         \
    ( defined(POLARSSL_DES_C) || defined(POLARSSL_AES_C) )
        enc++;

        s1 += 22;
        if( *s1 == '\r' ) s1++;
        if( *s1 == '\n' ) s1++;
        else return( POLARSSL_ERR_PEM_INVALID_DATA );


#if defined(POLARSSL_DES_C)
        if( memcmp( s1, "DEK-Info: DES-EDE3-CBC,", 23 ) == 0 )
        {
            enc_alg = POLARSSL_CIPHER_DES_EDE3_CBC;

            s1 += 23;
            if( pem_get_iv( s1, pem_iv, 8 ) != 0 )
                return( POLARSSL_ERR_PEM_INVALID_ENC_IV );

            s1 += 16;
        }
        else if( memcmp( s1, "DEK-Info: DES-CBC,", 18 ) == 0 )
        {
            enc_alg = POLARSSL_CIPHER_DES_CBC;

            s1 += 18;
            if( pem_get_iv( s1, pem_iv, 8) != 0 )
                return( POLARSSL_ERR_PEM_INVALID_ENC_IV );

            s1 += 16;
        }
#endif /* POLARSSL_DES_C */

#if defined(POLARSSL_AES_C)
        if( memcmp( s1, "DEK-Info: AES-", 14 ) == 0 )
        {
            if( memcmp( s1, "DEK-Info: AES-128-CBC,", 22 ) == 0 )
                enc_alg = POLARSSL_CIPHER_AES_128_CBC;
            else if( memcmp( s1, "DEK-Info: AES-192-CBC,", 22 ) == 0 )
                enc_alg = POLARSSL_CIPHER_AES_192_CBC;
            else if( memcmp( s1, "DEK-Info: AES-256-CBC,", 22 ) == 0 )
                enc_alg = POLARSSL_CIPHER_AES_256_CBC;
            else
                return( POLARSSL_ERR_PEM_UNKNOWN_ENC_ALG );

            s1 += 22;
            if( pem_get_iv( s1, pem_iv, 16 ) != 0 )
                return( POLARSSL_ERR_PEM_INVALID_ENC_IV );

            s1 += 32;
        }
#endif /* POLARSSL_AES_C */

        if( enc_alg == POLARSSL_CIPHER_NONE )
            return( POLARSSL_ERR_PEM_UNKNOWN_ENC_ALG );

        if( *s1 == '\r' ) s1++;
        if( *s1 == '\n' ) s1++;
        else return( POLARSSL_ERR_PEM_INVALID_DATA );
#else
        return( POLARSSL_ERR_PEM_FEATURE_UNAVAILABLE );
#endif /* POLARSSL_MD5_C && POLARSSL_CIPHER_MODE_CBC &&
          ( POLARSSL_AES_C || POLARSSL_DES_C ) */
    }

    len = 0;
    ret = base64_decode( NULL, &len, s1, s2 - s1 );

    if( ret == POLARSSL_ERR_BASE64_INVALID_CHARACTER )
        return( POLARSSL_ERR_PEM_INVALID_DATA + ret );

    if( ( buf = (unsigned char *) polarssl_malloc( len ) ) == NULL )
        return( POLARSSL_ERR_PEM_MALLOC_FAILED );

    if( ( ret = base64_decode( buf, &len, s1, s2 - s1 ) ) != 0 )
    {
        polarssl_free( buf );
        return( POLARSSL_ERR_PEM_INVALID_DATA + ret );
    }

    if( enc != 0 )
    {
#if defined(POLARSSL_MD5_C) && defined(POLARSSL_CIPHER_MODE_CBC) &&         \
    ( defined(POLARSSL_DES_C) || defined(POLARSSL_AES_C) )
        if( pwd == NULL )
        {
            polarssl_free( buf );
            return( POLARSSL_ERR_PEM_PASSWORD_REQUIRED );
        }

#if defined(POLARSSL_DES_C)
        if( enc_alg == POLARSSL_CIPHER_DES_EDE3_CBC )
            pem_des3_decrypt( pem_iv, buf, len, pwd, pwdlen );
        else if( enc_alg == POLARSSL_CIPHER_DES_CBC )
            pem_des_decrypt( pem_iv, buf, len, pwd, pwdlen );
#endif /* POLARSSL_DES_C */

#if defined(POLARSSL_AES_C)
        if( enc_alg == POLARSSL_CIPHER_AES_128_CBC )
            pem_aes_decrypt( pem_iv, 16, buf, len, pwd, pwdlen );
        else if( enc_alg == POLARSSL_CIPHER_AES_192_CBC )
            pem_aes_decrypt( pem_iv, 24, buf, len, pwd, pwdlen );
        else if( enc_alg == POLARSSL_CIPHER_AES_256_CBC )
            pem_aes_decrypt( pem_iv, 32, buf, len, pwd, pwdlen );
#endif /* POLARSSL_AES_C */

        /*
         * The result will be ASN.1 starting with a SEQUENCE tag, with 1 to 3
         * length bytes (allow 4 to be sure) in all known use cases.
         *
         * Use that as heurisitic to try detecting password mismatchs.
         */
        if( len <= 2 || buf[0] != 0x30 || buf[1] > 0x83 )
        {
            polarssl_free( buf );
            return( POLARSSL_ERR_PEM_PASSWORD_MISMATCH );
        }
#else
        polarssl_free( buf );
        return( POLARSSL_ERR_PEM_FEATURE_UNAVAILABLE );
#endif /* POLARSSL_MD5_C && POLARSSL_CIPHER_MODE_CBC &&
          ( POLARSSL_AES_C || POLARSSL_DES_C ) */
    }

    ctx->buf = buf;
    ctx->buflen = len;

    return( 0 );
}

void pem_free( pem_context *ctx )
{
    polarssl_free( ctx->buf );
    polarssl_free( ctx->info );

    polarssl_zeroize( ctx, sizeof( pem_context ) );
}
#endif /* POLARSSL_PEM_PARSE_C */

#if defined(POLARSSL_PEM_WRITE_C)
int pem_write_buffer( const char *header, const char *footer,
                      const unsigned char *der_data, size_t der_len,
                      unsigned char *buf, size_t buf_len, size_t *olen )
{
    int ret;
    unsigned char *encode_buf, *c, *p = buf;
    size_t len = 0, use_len = 0, add_len = 0;

    base64_encode( NULL, &use_len, der_data, der_len );
    add_len = strlen( header ) + strlen( footer ) + ( use_len / 64 ) + 1;

    if( use_len + add_len > buf_len )
    {
        *olen = use_len + add_len;
        return( POLARSSL_ERR_BASE64_BUFFER_TOO_SMALL );
    }

    if( ( encode_buf = polarssl_malloc( use_len ) ) == NULL )
        return( POLARSSL_ERR_PEM_MALLOC_FAILED );

    if( ( ret = base64_encode( encode_buf, &use_len, der_data,
                               der_len ) ) != 0 )
    {
        polarssl_free( encode_buf );
        return( ret );
    }

    memcpy( p, header, strlen( header ) );
    p += strlen( header );
    c = encode_buf;

    while( use_len )
    {
        len = ( use_len > 64 ) ? 64 : use_len;
        memcpy( p, c, len );
        use_len -= len;
        p += len;
        c += len;
        *p++ = '\n';
    }

    memcpy( p, footer, strlen( footer ) );
    p += strlen( footer );

    *p++ = '\0';
    *olen = p - buf;

    polarssl_free( encode_buf );
    return( 0 );
}
#endif /* POLARSSL_PEM_WRITE_C */
#endif /* POLARSSL_PEM_PARSE_C || POLARSSL_PEM_WRITE_C */
