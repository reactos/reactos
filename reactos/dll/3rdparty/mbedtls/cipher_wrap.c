/**
 * \file cipher_wrap.c
 *
 * \brief Generic cipher wrapper for mbed TLS
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

#if defined(POLARSSL_CIPHER_C)

#include "polarssl/cipher_wrap.h"

#if defined(POLARSSL_AES_C)
#include "polarssl/aes.h"
#endif

#if defined(POLARSSL_ARC4_C)
#include "polarssl/arc4.h"
#endif

#if defined(POLARSSL_CAMELLIA_C)
#include "polarssl/camellia.h"
#endif

#if defined(POLARSSL_DES_C)
#include "polarssl/des.h"
#endif

#if defined(POLARSSL_BLOWFISH_C)
#include "polarssl/blowfish.h"
#endif

#if defined(POLARSSL_GCM_C)
#include "polarssl/gcm.h"
#endif

#if defined(POLARSSL_CCM_C)
#include "polarssl/ccm.h"
#endif

#if defined(POLARSSL_PLATFORM_C)
#include "polarssl/platform.h"
#else
#define polarssl_malloc     malloc
#define polarssl_free       free
#endif

#include <stdlib.h>

#if defined(POLARSSL_GCM_C)
/* shared by all GCM ciphers */
static void *gcm_ctx_alloc( void )
{
    return polarssl_malloc( sizeof( gcm_context ) );
}

static void gcm_ctx_free( void *ctx )
{
    gcm_free( ctx );
    polarssl_free( ctx );
}
#endif /* POLARSSL_GCM_C */

#if defined(POLARSSL_CCM_C)
/* shared by all CCM ciphers */
static void *ccm_ctx_alloc( void )
{
    return polarssl_malloc( sizeof( ccm_context ) );
}

static void ccm_ctx_free( void *ctx )
{
    ccm_free( ctx );
    polarssl_free( ctx );
}
#endif /* POLARSSL_CCM_C */

#if defined(POLARSSL_AES_C)

static int aes_crypt_ecb_wrap( void *ctx, operation_t operation,
        const unsigned char *input, unsigned char *output )
{
    return aes_crypt_ecb( (aes_context *) ctx, operation, input, output );
}

static int aes_crypt_cbc_wrap( void *ctx, operation_t operation, size_t length,
        unsigned char *iv, const unsigned char *input, unsigned char *output )
{
#if defined(POLARSSL_CIPHER_MODE_CBC)
    return aes_crypt_cbc( (aes_context *) ctx, operation, length, iv, input,
                          output );
#else
    ((void) ctx);
    ((void) operation);
    ((void) length);
    ((void) iv);
    ((void) input);
    ((void) output);

    return( POLARSSL_ERR_CIPHER_FEATURE_UNAVAILABLE );
#endif /* POLARSSL_CIPHER_MODE_CBC */
}

static int aes_crypt_cfb128_wrap( void *ctx, operation_t operation,
        size_t length, size_t *iv_off, unsigned char *iv,
        const unsigned char *input, unsigned char *output )
{
#if defined(POLARSSL_CIPHER_MODE_CFB)
    return aes_crypt_cfb128( (aes_context *) ctx, operation, length, iv_off, iv,
                             input, output );
#else
    ((void) ctx);
    ((void) operation);
    ((void) length);
    ((void) iv_off);
    ((void) iv);
    ((void) input);
    ((void) output);

    return( POLARSSL_ERR_CIPHER_FEATURE_UNAVAILABLE );
#endif /* POLARSSL_CIPHER_MODE_CFB */
}

static int aes_crypt_ctr_wrap( void *ctx, size_t length, size_t *nc_off,
        unsigned char *nonce_counter, unsigned char *stream_block,
        const unsigned char *input, unsigned char *output )
{
#if defined(POLARSSL_CIPHER_MODE_CTR)
    return aes_crypt_ctr( (aes_context *) ctx, length, nc_off, nonce_counter,
                          stream_block, input, output );
#else
    ((void) ctx);
    ((void) length);
    ((void) nc_off);
    ((void) nonce_counter);
    ((void) stream_block);
    ((void) input);
    ((void) output);

    return( POLARSSL_ERR_CIPHER_FEATURE_UNAVAILABLE );
#endif /* POLARSSL_CIPHER_MODE_CTR */
}

static int aes_setkey_dec_wrap( void *ctx, const unsigned char *key,
                                unsigned int key_length )
{
    return aes_setkey_dec( (aes_context *) ctx, key, key_length );
}

static int aes_setkey_enc_wrap( void *ctx, const unsigned char *key,
                                unsigned int key_length )
{
    return aes_setkey_enc( (aes_context *) ctx, key, key_length );
}

static void * aes_ctx_alloc( void )
{
    aes_context *aes = (aes_context *) polarssl_malloc( sizeof( aes_context ) );

    if( aes == NULL )
        return( NULL );

    aes_init( aes );

    return( aes );
}

static void aes_ctx_free( void *ctx )
{
    aes_free( (aes_context *) ctx );
    polarssl_free( ctx );
}

const cipher_base_t aes_info = {
    POLARSSL_CIPHER_ID_AES,
    aes_crypt_ecb_wrap,
    aes_crypt_cbc_wrap,
    aes_crypt_cfb128_wrap,
    aes_crypt_ctr_wrap,
    NULL,
    aes_setkey_enc_wrap,
    aes_setkey_dec_wrap,
    aes_ctx_alloc,
    aes_ctx_free
};

const cipher_info_t aes_128_ecb_info = {
    POLARSSL_CIPHER_AES_128_ECB,
    POLARSSL_MODE_ECB,
    128,
    "AES-128-ECB",
    16,
    0,
    16,
    &aes_info
};

const cipher_info_t aes_192_ecb_info = {
    POLARSSL_CIPHER_AES_192_ECB,
    POLARSSL_MODE_ECB,
    192,
    "AES-192-ECB",
    16,
    0,
    16,
    &aes_info
};

const cipher_info_t aes_256_ecb_info = {
    POLARSSL_CIPHER_AES_256_ECB,
    POLARSSL_MODE_ECB,
    256,
    "AES-256-ECB",
    16,
    0,
    16,
    &aes_info
};

#if defined(POLARSSL_CIPHER_MODE_CBC)
const cipher_info_t aes_128_cbc_info = {
    POLARSSL_CIPHER_AES_128_CBC,
    POLARSSL_MODE_CBC,
    128,
    "AES-128-CBC",
    16,
    0,
    16,
    &aes_info
};

const cipher_info_t aes_192_cbc_info = {
    POLARSSL_CIPHER_AES_192_CBC,
    POLARSSL_MODE_CBC,
    192,
    "AES-192-CBC",
    16,
    0,
    16,
    &aes_info
};

const cipher_info_t aes_256_cbc_info = {
    POLARSSL_CIPHER_AES_256_CBC,
    POLARSSL_MODE_CBC,
    256,
    "AES-256-CBC",
    16,
    0,
    16,
    &aes_info
};
#endif /* POLARSSL_CIPHER_MODE_CBC */

#if defined(POLARSSL_CIPHER_MODE_CFB)
const cipher_info_t aes_128_cfb128_info = {
    POLARSSL_CIPHER_AES_128_CFB128,
    POLARSSL_MODE_CFB,
    128,
    "AES-128-CFB128",
    16,
    0,
    16,
    &aes_info
};

const cipher_info_t aes_192_cfb128_info = {
    POLARSSL_CIPHER_AES_192_CFB128,
    POLARSSL_MODE_CFB,
    192,
    "AES-192-CFB128",
    16,
    0,
    16,
    &aes_info
};

const cipher_info_t aes_256_cfb128_info = {
    POLARSSL_CIPHER_AES_256_CFB128,
    POLARSSL_MODE_CFB,
    256,
    "AES-256-CFB128",
    16,
    0,
    16,
    &aes_info
};
#endif /* POLARSSL_CIPHER_MODE_CFB */

#if defined(POLARSSL_CIPHER_MODE_CTR)
const cipher_info_t aes_128_ctr_info = {
    POLARSSL_CIPHER_AES_128_CTR,
    POLARSSL_MODE_CTR,
    128,
    "AES-128-CTR",
    16,
    0,
    16,
    &aes_info
};

const cipher_info_t aes_192_ctr_info = {
    POLARSSL_CIPHER_AES_192_CTR,
    POLARSSL_MODE_CTR,
    192,
    "AES-192-CTR",
    16,
    0,
    16,
    &aes_info
};

const cipher_info_t aes_256_ctr_info = {
    POLARSSL_CIPHER_AES_256_CTR,
    POLARSSL_MODE_CTR,
    256,
    "AES-256-CTR",
    16,
    0,
    16,
    &aes_info
};
#endif /* POLARSSL_CIPHER_MODE_CTR */

#if defined(POLARSSL_GCM_C)
static int gcm_aes_setkey_wrap( void *ctx, const unsigned char *key,
                                unsigned int key_length )
{
    return gcm_init( (gcm_context *) ctx, POLARSSL_CIPHER_ID_AES,
                     key, key_length );
}

const cipher_base_t gcm_aes_info = {
    POLARSSL_CIPHER_ID_AES,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    gcm_aes_setkey_wrap,
    gcm_aes_setkey_wrap,
    gcm_ctx_alloc,
    gcm_ctx_free,
};

const cipher_info_t aes_128_gcm_info = {
    POLARSSL_CIPHER_AES_128_GCM,
    POLARSSL_MODE_GCM,
    128,
    "AES-128-GCM",
    12,
    POLARSSL_CIPHER_VARIABLE_IV_LEN,
    16,
    &gcm_aes_info
};

const cipher_info_t aes_192_gcm_info = {
    POLARSSL_CIPHER_AES_192_GCM,
    POLARSSL_MODE_GCM,
    192,
    "AES-192-GCM",
    12,
    POLARSSL_CIPHER_VARIABLE_IV_LEN,
    16,
    &gcm_aes_info
};

const cipher_info_t aes_256_gcm_info = {
    POLARSSL_CIPHER_AES_256_GCM,
    POLARSSL_MODE_GCM,
    256,
    "AES-256-GCM",
    12,
    POLARSSL_CIPHER_VARIABLE_IV_LEN,
    16,
    &gcm_aes_info
};
#endif /* POLARSSL_GCM_C */

#if defined(POLARSSL_CCM_C)
static int ccm_aes_setkey_wrap( void *ctx, const unsigned char *key,
                                unsigned int key_length )
{
    return ccm_init( (ccm_context *) ctx, POLARSSL_CIPHER_ID_AES,
                     key, key_length );
}

const cipher_base_t ccm_aes_info = {
    POLARSSL_CIPHER_ID_AES,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    ccm_aes_setkey_wrap,
    ccm_aes_setkey_wrap,
    ccm_ctx_alloc,
    ccm_ctx_free,
};

const cipher_info_t aes_128_ccm_info = {
    POLARSSL_CIPHER_AES_128_CCM,
    POLARSSL_MODE_CCM,
    128,
    "AES-128-CCM",
    12,
    POLARSSL_CIPHER_VARIABLE_IV_LEN,
    16,
    &ccm_aes_info
};

const cipher_info_t aes_192_ccm_info = {
    POLARSSL_CIPHER_AES_192_CCM,
    POLARSSL_MODE_CCM,
    192,
    "AES-192-CCM",
    12,
    POLARSSL_CIPHER_VARIABLE_IV_LEN,
    16,
    &ccm_aes_info
};

const cipher_info_t aes_256_ccm_info = {
    POLARSSL_CIPHER_AES_256_CCM,
    POLARSSL_MODE_CCM,
    256,
    "AES-256-CCM",
    12,
    POLARSSL_CIPHER_VARIABLE_IV_LEN,
    16,
    &ccm_aes_info
};
#endif /* POLARSSL_CCM_C */

#endif /* POLARSSL_AES_C */

#if defined(POLARSSL_CAMELLIA_C)

static int camellia_crypt_ecb_wrap( void *ctx, operation_t operation,
        const unsigned char *input, unsigned char *output )
{
    return camellia_crypt_ecb( (camellia_context *) ctx, operation, input,
                               output );
}

static int camellia_crypt_cbc_wrap( void *ctx, operation_t operation,
        size_t length, unsigned char *iv,
        const unsigned char *input, unsigned char *output )
{
#if defined(POLARSSL_CIPHER_MODE_CBC)
    return camellia_crypt_cbc( (camellia_context *) ctx, operation, length, iv,
                               input, output );
#else
    ((void) ctx);
    ((void) operation);
    ((void) length);
    ((void) iv);
    ((void) input);
    ((void) output);

    return( POLARSSL_ERR_CIPHER_FEATURE_UNAVAILABLE );
#endif /* POLARSSL_CIPHER_MODE_CBC */
}

static int camellia_crypt_cfb128_wrap( void *ctx, operation_t operation,
        size_t length, size_t *iv_off, unsigned char *iv,
        const unsigned char *input, unsigned char *output )
{
#if defined(POLARSSL_CIPHER_MODE_CFB)
    return camellia_crypt_cfb128( (camellia_context *) ctx, operation, length,
                                  iv_off, iv, input, output );
#else
    ((void) ctx);
    ((void) operation);
    ((void) length);
    ((void) iv_off);
    ((void) iv);
    ((void) input);
    ((void) output);

    return( POLARSSL_ERR_CIPHER_FEATURE_UNAVAILABLE );
#endif /* POLARSSL_CIPHER_MODE_CFB */
}

static int camellia_crypt_ctr_wrap( void *ctx, size_t length, size_t *nc_off,
        unsigned char *nonce_counter, unsigned char *stream_block,
        const unsigned char *input, unsigned char *output )
{
#if defined(POLARSSL_CIPHER_MODE_CTR)
    return camellia_crypt_ctr( (camellia_context *) ctx, length, nc_off,
                               nonce_counter, stream_block, input, output );
#else
    ((void) ctx);
    ((void) length);
    ((void) nc_off);
    ((void) nonce_counter);
    ((void) stream_block);
    ((void) input);
    ((void) output);

    return( POLARSSL_ERR_CIPHER_FEATURE_UNAVAILABLE );
#endif /* POLARSSL_CIPHER_MODE_CTR */
}

static int camellia_setkey_dec_wrap( void *ctx, const unsigned char *key,
                                     unsigned int key_length )
{
    return camellia_setkey_dec( (camellia_context *) ctx, key, key_length );
}

static int camellia_setkey_enc_wrap( void *ctx, const unsigned char *key,
                                     unsigned int key_length )
{
    return camellia_setkey_enc( (camellia_context *) ctx, key, key_length );
}

static void * camellia_ctx_alloc( void )
{
    camellia_context *ctx;
    ctx = (camellia_context *) polarssl_malloc( sizeof( camellia_context ) );

    if( ctx == NULL )
        return( NULL );

    camellia_init( ctx );

    return( ctx );
}

static void camellia_ctx_free( void *ctx )
{
    camellia_free( (camellia_context *) ctx );
    polarssl_free( ctx );
}

const cipher_base_t camellia_info = {
    POLARSSL_CIPHER_ID_CAMELLIA,
    camellia_crypt_ecb_wrap,
    camellia_crypt_cbc_wrap,
    camellia_crypt_cfb128_wrap,
    camellia_crypt_ctr_wrap,
    NULL,
    camellia_setkey_enc_wrap,
    camellia_setkey_dec_wrap,
    camellia_ctx_alloc,
    camellia_ctx_free
};

const cipher_info_t camellia_128_ecb_info = {
    POLARSSL_CIPHER_CAMELLIA_128_ECB,
    POLARSSL_MODE_ECB,
    128,
    "CAMELLIA-128-ECB",
    16,
    0,
    16,
    &camellia_info
};

const cipher_info_t camellia_192_ecb_info = {
    POLARSSL_CIPHER_CAMELLIA_192_ECB,
    POLARSSL_MODE_ECB,
    192,
    "CAMELLIA-192-ECB",
    16,
    0,
    16,
    &camellia_info
};

const cipher_info_t camellia_256_ecb_info = {
    POLARSSL_CIPHER_CAMELLIA_256_ECB,
    POLARSSL_MODE_ECB,
    256,
    "CAMELLIA-256-ECB",
    16,
    0,
    16,
    &camellia_info
};

#if defined(POLARSSL_CIPHER_MODE_CBC)
const cipher_info_t camellia_128_cbc_info = {
    POLARSSL_CIPHER_CAMELLIA_128_CBC,
    POLARSSL_MODE_CBC,
    128,
    "CAMELLIA-128-CBC",
    16,
    0,
    16,
    &camellia_info
};

const cipher_info_t camellia_192_cbc_info = {
    POLARSSL_CIPHER_CAMELLIA_192_CBC,
    POLARSSL_MODE_CBC,
    192,
    "CAMELLIA-192-CBC",
    16,
    0,
    16,
    &camellia_info
};

const cipher_info_t camellia_256_cbc_info = {
    POLARSSL_CIPHER_CAMELLIA_256_CBC,
    POLARSSL_MODE_CBC,
    256,
    "CAMELLIA-256-CBC",
    16,
    0,
    16,
    &camellia_info
};
#endif /* POLARSSL_CIPHER_MODE_CBC */

#if defined(POLARSSL_CIPHER_MODE_CFB)
const cipher_info_t camellia_128_cfb128_info = {
    POLARSSL_CIPHER_CAMELLIA_128_CFB128,
    POLARSSL_MODE_CFB,
    128,
    "CAMELLIA-128-CFB128",
    16,
    0,
    16,
    &camellia_info
};

const cipher_info_t camellia_192_cfb128_info = {
    POLARSSL_CIPHER_CAMELLIA_192_CFB128,
    POLARSSL_MODE_CFB,
    192,
    "CAMELLIA-192-CFB128",
    16,
    0,
    16,
    &camellia_info
};

const cipher_info_t camellia_256_cfb128_info = {
    POLARSSL_CIPHER_CAMELLIA_256_CFB128,
    POLARSSL_MODE_CFB,
    256,
    "CAMELLIA-256-CFB128",
    16,
    0,
    16,
    &camellia_info
};
#endif /* POLARSSL_CIPHER_MODE_CFB */

#if defined(POLARSSL_CIPHER_MODE_CTR)
const cipher_info_t camellia_128_ctr_info = {
    POLARSSL_CIPHER_CAMELLIA_128_CTR,
    POLARSSL_MODE_CTR,
    128,
    "CAMELLIA-128-CTR",
    16,
    0,
    16,
    &camellia_info
};

const cipher_info_t camellia_192_ctr_info = {
    POLARSSL_CIPHER_CAMELLIA_192_CTR,
    POLARSSL_MODE_CTR,
    192,
    "CAMELLIA-192-CTR",
    16,
    0,
    16,
    &camellia_info
};

const cipher_info_t camellia_256_ctr_info = {
    POLARSSL_CIPHER_CAMELLIA_256_CTR,
    POLARSSL_MODE_CTR,
    256,
    "CAMELLIA-256-CTR",
    16,
    0,
    16,
    &camellia_info
};
#endif /* POLARSSL_CIPHER_MODE_CTR */

#if defined(POLARSSL_GCM_C)
static int gcm_camellia_setkey_wrap( void *ctx, const unsigned char *key,
                                     unsigned int key_length )
{
    return gcm_init( (gcm_context *) ctx, POLARSSL_CIPHER_ID_CAMELLIA,
                     key, key_length );
}

const cipher_base_t gcm_camellia_info = {
    POLARSSL_CIPHER_ID_CAMELLIA,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    gcm_camellia_setkey_wrap,
    gcm_camellia_setkey_wrap,
    gcm_ctx_alloc,
    gcm_ctx_free,
};

const cipher_info_t camellia_128_gcm_info = {
    POLARSSL_CIPHER_CAMELLIA_128_GCM,
    POLARSSL_MODE_GCM,
    128,
    "CAMELLIA-128-GCM",
    12,
    POLARSSL_CIPHER_VARIABLE_IV_LEN,
    16,
    &gcm_camellia_info
};

const cipher_info_t camellia_192_gcm_info = {
    POLARSSL_CIPHER_CAMELLIA_192_GCM,
    POLARSSL_MODE_GCM,
    192,
    "CAMELLIA-192-GCM",
    12,
    POLARSSL_CIPHER_VARIABLE_IV_LEN,
    16,
    &gcm_camellia_info
};

const cipher_info_t camellia_256_gcm_info = {
    POLARSSL_CIPHER_CAMELLIA_256_GCM,
    POLARSSL_MODE_GCM,
    256,
    "CAMELLIA-256-GCM",
    12,
    POLARSSL_CIPHER_VARIABLE_IV_LEN,
    16,
    &gcm_camellia_info
};
#endif /* POLARSSL_GCM_C */

#if defined(POLARSSL_CCM_C)
static int ccm_camellia_setkey_wrap( void *ctx, const unsigned char *key,
                                     unsigned int key_length )
{
    return ccm_init( (ccm_context *) ctx, POLARSSL_CIPHER_ID_CAMELLIA,
                     key, key_length );
}

const cipher_base_t ccm_camellia_info = {
    POLARSSL_CIPHER_ID_CAMELLIA,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    ccm_camellia_setkey_wrap,
    ccm_camellia_setkey_wrap,
    ccm_ctx_alloc,
    ccm_ctx_free,
};

const cipher_info_t camellia_128_ccm_info = {
    POLARSSL_CIPHER_CAMELLIA_128_CCM,
    POLARSSL_MODE_CCM,
    128,
    "CAMELLIA-128-CCM",
    12,
    POLARSSL_CIPHER_VARIABLE_IV_LEN,
    16,
    &ccm_camellia_info
};

const cipher_info_t camellia_192_ccm_info = {
    POLARSSL_CIPHER_CAMELLIA_192_CCM,
    POLARSSL_MODE_CCM,
    192,
    "CAMELLIA-192-CCM",
    12,
    POLARSSL_CIPHER_VARIABLE_IV_LEN,
    16,
    &ccm_camellia_info
};

const cipher_info_t camellia_256_ccm_info = {
    POLARSSL_CIPHER_CAMELLIA_256_CCM,
    POLARSSL_MODE_CCM,
    256,
    "CAMELLIA-256-CCM",
    12,
    POLARSSL_CIPHER_VARIABLE_IV_LEN,
    16,
    &ccm_camellia_info
};
#endif /* POLARSSL_CCM_C */

#endif /* POLARSSL_CAMELLIA_C */

#if defined(POLARSSL_DES_C)

static int des_crypt_ecb_wrap( void *ctx, operation_t operation,
        const unsigned char *input, unsigned char *output )
{
    ((void) operation);
    return des_crypt_ecb( (des_context *) ctx, input, output );
}

static int des3_crypt_ecb_wrap( void *ctx, operation_t operation,
        const unsigned char *input, unsigned char *output )
{
    ((void) operation);
    return des3_crypt_ecb( (des3_context *) ctx, input, output );
}

static int des_crypt_cbc_wrap( void *ctx, operation_t operation, size_t length,
        unsigned char *iv, const unsigned char *input, unsigned char *output )
{
#if defined(POLARSSL_CIPHER_MODE_CBC)
    return des_crypt_cbc( (des_context *) ctx, operation, length, iv, input,
                          output );
#else
    ((void) ctx);
    ((void) operation);
    ((void) length);
    ((void) iv);
    ((void) input);
    ((void) output);

    return( POLARSSL_ERR_CIPHER_FEATURE_UNAVAILABLE );
#endif /* POLARSSL_CIPHER_MODE_CBC */
}

static int des3_crypt_cbc_wrap( void *ctx, operation_t operation, size_t length,
        unsigned char *iv, const unsigned char *input, unsigned char *output )
{
#if defined(POLARSSL_CIPHER_MODE_CBC)
    return des3_crypt_cbc( (des3_context *) ctx, operation, length, iv, input,
                           output );
#else
    ((void) ctx);
    ((void) operation);
    ((void) length);
    ((void) iv);
    ((void) input);
    ((void) output);

    return( POLARSSL_ERR_CIPHER_FEATURE_UNAVAILABLE );
#endif /* POLARSSL_CIPHER_MODE_CBC */
}

static int des_setkey_dec_wrap( void *ctx, const unsigned char *key,
                                unsigned int key_length )
{
    ((void) key_length);

    return des_setkey_dec( (des_context *) ctx, key );
}

static int des_setkey_enc_wrap( void *ctx, const unsigned char *key,
                                unsigned int key_length )
{
    ((void) key_length);

    return des_setkey_enc( (des_context *) ctx, key );
}

static int des3_set2key_dec_wrap( void *ctx, const unsigned char *key,
                                  unsigned int key_length )
{
    ((void) key_length);

    return des3_set2key_dec( (des3_context *) ctx, key );
}

static int des3_set2key_enc_wrap( void *ctx, const unsigned char *key,
                                  unsigned int key_length )
{
    ((void) key_length);

    return des3_set2key_enc( (des3_context *) ctx, key );
}

static int des3_set3key_dec_wrap( void *ctx, const unsigned char *key,
                                  unsigned int key_length )
{
    ((void) key_length);

    return des3_set3key_dec( (des3_context *) ctx, key );
}

static int des3_set3key_enc_wrap( void *ctx, const unsigned char *key,
                                  unsigned int key_length )
{
    ((void) key_length);

    return des3_set3key_enc( (des3_context *) ctx, key );
}

static void * des_ctx_alloc( void )
{
    des_context *des = (des_context *) polarssl_malloc( sizeof( des_context ) );

    if( des == NULL )
        return( NULL );

    des_init( des );

    return( des );
}

static void des_ctx_free( void *ctx )
{
    des_free( (des_context *) ctx );
    polarssl_free( ctx );
}

static void * des3_ctx_alloc( void )
{
    des3_context *des3;
    des3 = (des3_context *) polarssl_malloc( sizeof( des3_context ) );

    if( des3 == NULL )
        return( NULL );

    des3_init( des3 );

    return( des3 );
}

static void des3_ctx_free( void *ctx )
{
    des3_free( (des3_context *) ctx );
    polarssl_free( ctx );
}

const cipher_base_t des_info = {
    POLARSSL_CIPHER_ID_DES,
    des_crypt_ecb_wrap,
    des_crypt_cbc_wrap,
    NULL,
    NULL,
    NULL,
    des_setkey_enc_wrap,
    des_setkey_dec_wrap,
    des_ctx_alloc,
    des_ctx_free
};

const cipher_info_t des_ecb_info = {
    POLARSSL_CIPHER_DES_ECB,
    POLARSSL_MODE_ECB,
    POLARSSL_KEY_LENGTH_DES,
    "DES-ECB",
    8,
    0,
    8,
    &des_info
};

#if defined(POLARSSL_CIPHER_MODE_CBC)
const cipher_info_t des_cbc_info = {
    POLARSSL_CIPHER_DES_CBC,
    POLARSSL_MODE_CBC,
    POLARSSL_KEY_LENGTH_DES,
    "DES-CBC",
    8,
    0,
    8,
    &des_info
};
#endif /* POLARSSL_CIPHER_MODE_CBC */

const cipher_base_t des_ede_info = {
    POLARSSL_CIPHER_ID_DES,
    des3_crypt_ecb_wrap,
    des3_crypt_cbc_wrap,
    NULL,
    NULL,
    NULL,
    des3_set2key_enc_wrap,
    des3_set2key_dec_wrap,
    des3_ctx_alloc,
    des3_ctx_free
};

const cipher_info_t des_ede_ecb_info = {
    POLARSSL_CIPHER_DES_EDE_ECB,
    POLARSSL_MODE_ECB,
    POLARSSL_KEY_LENGTH_DES_EDE,
    "DES-EDE-ECB",
    8,
    0,
    8,
    &des_ede_info
};

#if defined(POLARSSL_CIPHER_MODE_CBC)
const cipher_info_t des_ede_cbc_info = {
    POLARSSL_CIPHER_DES_EDE_CBC,
    POLARSSL_MODE_CBC,
    POLARSSL_KEY_LENGTH_DES_EDE,
    "DES-EDE-CBC",
    8,
    0,
    8,
    &des_ede_info
};
#endif /* POLARSSL_CIPHER_MODE_CBC */

const cipher_base_t des_ede3_info = {
    POLARSSL_CIPHER_ID_DES,
    des3_crypt_ecb_wrap,
    des3_crypt_cbc_wrap,
    NULL,
    NULL,
    NULL,
    des3_set3key_enc_wrap,
    des3_set3key_dec_wrap,
    des3_ctx_alloc,
    des3_ctx_free
};

const cipher_info_t des_ede3_ecb_info = {
    POLARSSL_CIPHER_DES_EDE3_ECB,
    POLARSSL_MODE_ECB,
    POLARSSL_KEY_LENGTH_DES_EDE3,
    "DES-EDE3-ECB",
    8,
    0,
    8,
    &des_ede3_info
};
#if defined(POLARSSL_CIPHER_MODE_CBC)
const cipher_info_t des_ede3_cbc_info = {
    POLARSSL_CIPHER_DES_EDE3_CBC,
    POLARSSL_MODE_CBC,
    POLARSSL_KEY_LENGTH_DES_EDE3,
    "DES-EDE3-CBC",
    8,
    0,
    8,
    &des_ede3_info
};
#endif /* POLARSSL_CIPHER_MODE_CBC */
#endif /* POLARSSL_DES_C */

#if defined(POLARSSL_BLOWFISH_C)

static int blowfish_crypt_ecb_wrap( void *ctx, operation_t operation,
        const unsigned char *input, unsigned char *output )
{
    return blowfish_crypt_ecb( (blowfish_context *) ctx, operation, input,
                               output );
}

static int blowfish_crypt_cbc_wrap( void *ctx, operation_t operation,
        size_t length, unsigned char *iv, const unsigned char *input,
        unsigned char *output )
{
#if defined(POLARSSL_CIPHER_MODE_CBC)
    return blowfish_crypt_cbc( (blowfish_context *) ctx, operation, length, iv,
                               input, output );
#else
    ((void) ctx);
    ((void) operation);
    ((void) length);
    ((void) iv);
    ((void) input);
    ((void) output);

    return( POLARSSL_ERR_CIPHER_FEATURE_UNAVAILABLE );
#endif /* POLARSSL_CIPHER_MODE_CBC */
}

static int blowfish_crypt_cfb64_wrap( void *ctx, operation_t operation,
        size_t length, size_t *iv_off, unsigned char *iv,
        const unsigned char *input, unsigned char *output )
{
#if defined(POLARSSL_CIPHER_MODE_CFB)
    return blowfish_crypt_cfb64( (blowfish_context *) ctx, operation, length,
                                 iv_off, iv, input, output );
#else
    ((void) ctx);
    ((void) operation);
    ((void) length);
    ((void) iv_off);
    ((void) iv);
    ((void) input);
    ((void) output);

    return( POLARSSL_ERR_CIPHER_FEATURE_UNAVAILABLE );
#endif /* POLARSSL_CIPHER_MODE_CFB */
}

static int blowfish_crypt_ctr_wrap( void *ctx, size_t length, size_t *nc_off,
        unsigned char *nonce_counter, unsigned char *stream_block,
        const unsigned char *input, unsigned char *output )
{
#if defined(POLARSSL_CIPHER_MODE_CTR)
    return blowfish_crypt_ctr( (blowfish_context *) ctx, length, nc_off,
                               nonce_counter, stream_block, input, output );
#else
    ((void) ctx);
    ((void) length);
    ((void) nc_off);
    ((void) nonce_counter);
    ((void) stream_block);
    ((void) input);
    ((void) output);

    return( POLARSSL_ERR_CIPHER_FEATURE_UNAVAILABLE );
#endif /* POLARSSL_CIPHER_MODE_CTR */
}

static int blowfish_setkey_wrap( void *ctx, const unsigned char *key,
                                 unsigned int key_length )
{
    return blowfish_setkey( (blowfish_context *) ctx, key, key_length );
}

static void * blowfish_ctx_alloc( void )
{
    blowfish_context *ctx;
    ctx = (blowfish_context *) polarssl_malloc( sizeof( blowfish_context ) );

    if( ctx == NULL )
        return( NULL );

    blowfish_init( ctx );

    return( ctx );
}

static void blowfish_ctx_free( void *ctx )
{
    blowfish_free( (blowfish_context *) ctx );
    polarssl_free( ctx );
}

const cipher_base_t blowfish_info = {
    POLARSSL_CIPHER_ID_BLOWFISH,
    blowfish_crypt_ecb_wrap,
    blowfish_crypt_cbc_wrap,
    blowfish_crypt_cfb64_wrap,
    blowfish_crypt_ctr_wrap,
    NULL,
    blowfish_setkey_wrap,
    blowfish_setkey_wrap,
    blowfish_ctx_alloc,
    blowfish_ctx_free
};

const cipher_info_t blowfish_ecb_info = {
    POLARSSL_CIPHER_BLOWFISH_ECB,
    POLARSSL_MODE_ECB,
    128,
    "BLOWFISH-ECB",
    8,
    POLARSSL_CIPHER_VARIABLE_KEY_LEN,
    8,
    &blowfish_info
};

#if defined(POLARSSL_CIPHER_MODE_CBC)
const cipher_info_t blowfish_cbc_info = {
    POLARSSL_CIPHER_BLOWFISH_CBC,
    POLARSSL_MODE_CBC,
    128,
    "BLOWFISH-CBC",
    8,
    POLARSSL_CIPHER_VARIABLE_KEY_LEN,
    8,
    &blowfish_info
};
#endif /* POLARSSL_CIPHER_MODE_CBC */

#if defined(POLARSSL_CIPHER_MODE_CFB)
const cipher_info_t blowfish_cfb64_info = {
    POLARSSL_CIPHER_BLOWFISH_CFB64,
    POLARSSL_MODE_CFB,
    128,
    "BLOWFISH-CFB64",
    8,
    POLARSSL_CIPHER_VARIABLE_KEY_LEN,
    8,
    &blowfish_info
};
#endif /* POLARSSL_CIPHER_MODE_CFB */

#if defined(POLARSSL_CIPHER_MODE_CTR)
const cipher_info_t blowfish_ctr_info = {
    POLARSSL_CIPHER_BLOWFISH_CTR,
    POLARSSL_MODE_CTR,
    128,
    "BLOWFISH-CTR",
    8,
    POLARSSL_CIPHER_VARIABLE_KEY_LEN,
    8,
    &blowfish_info
};
#endif /* POLARSSL_CIPHER_MODE_CTR */
#endif /* POLARSSL_BLOWFISH_C */

#if defined(POLARSSL_ARC4_C)
static int arc4_crypt_stream_wrap( void *ctx, size_t length,
                                   const unsigned char *input,
                                   unsigned char *output )
{
    return( arc4_crypt( (arc4_context *) ctx, length, input, output ) );
}

static int arc4_setkey_wrap( void *ctx, const unsigned char *key,
                             unsigned int key_length )
{
    /* we get key_length in bits, arc4 expects it in bytes */
    if( key_length % 8 != 0 )
        return( POLARSSL_ERR_CIPHER_BAD_INPUT_DATA );

    arc4_setup( (arc4_context *) ctx, key, key_length / 8 );
    return( 0 );
}

static void * arc4_ctx_alloc( void )
{
    arc4_context *ctx;
    ctx = (arc4_context *) polarssl_malloc( sizeof( arc4_context ) );

    if( ctx == NULL )
        return( NULL );

    arc4_init( ctx );

    return( ctx );
}

static void arc4_ctx_free( void *ctx )
{
    arc4_free( (arc4_context *) ctx );
    polarssl_free( ctx );
}

const cipher_base_t arc4_base_info = {
    POLARSSL_CIPHER_ID_ARC4,
    NULL,
    NULL,
    NULL,
    NULL,
    arc4_crypt_stream_wrap,
    arc4_setkey_wrap,
    arc4_setkey_wrap,
    arc4_ctx_alloc,
    arc4_ctx_free
};

const cipher_info_t arc4_128_info = {
    POLARSSL_CIPHER_ARC4_128,
    POLARSSL_MODE_STREAM,
    128,
    "ARC4-128",
    0,
    0,
    1,
    &arc4_base_info
};
#endif /* POLARSSL_ARC4_C */

#if defined(POLARSSL_CIPHER_NULL_CIPHER)
static int null_crypt_stream( void *ctx, size_t length,
                              const unsigned char *input,
                              unsigned char *output )
{
    ((void) ctx);
    memmove( output, input, length );
    return( 0 );
}

static int null_setkey( void *ctx, const unsigned char *key,
                        unsigned int key_length )
{
    ((void) ctx);
    ((void) key);
    ((void) key_length);

    return( 0 );
}

static void * null_ctx_alloc( void )
{
    return( (void *) 1 );
}

static void null_ctx_free( void *ctx )
{
    ((void) ctx);
}

const cipher_base_t null_base_info = {
    POLARSSL_CIPHER_ID_NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    null_crypt_stream,
    null_setkey,
    null_setkey,
    null_ctx_alloc,
    null_ctx_free
};

const cipher_info_t null_cipher_info = {
    POLARSSL_CIPHER_NULL,
    POLARSSL_MODE_STREAM,
    0,
    "NULL",
    0,
    0,
    1,
    &null_base_info
};
#endif /* defined(POLARSSL_CIPHER_NULL_CIPHER) */

const cipher_definition_t cipher_definitions[] =
{
#if defined(POLARSSL_AES_C)
    { POLARSSL_CIPHER_AES_128_ECB,          &aes_128_ecb_info },
    { POLARSSL_CIPHER_AES_192_ECB,          &aes_192_ecb_info },
    { POLARSSL_CIPHER_AES_256_ECB,          &aes_256_ecb_info },
#if defined(POLARSSL_CIPHER_MODE_CBC)
    { POLARSSL_CIPHER_AES_128_CBC,          &aes_128_cbc_info },
    { POLARSSL_CIPHER_AES_192_CBC,          &aes_192_cbc_info },
    { POLARSSL_CIPHER_AES_256_CBC,          &aes_256_cbc_info },
#endif
#if defined(POLARSSL_CIPHER_MODE_CFB)
    { POLARSSL_CIPHER_AES_128_CFB128,       &aes_128_cfb128_info },
    { POLARSSL_CIPHER_AES_192_CFB128,       &aes_192_cfb128_info },
    { POLARSSL_CIPHER_AES_256_CFB128,       &aes_256_cfb128_info },
#endif
#if defined(POLARSSL_CIPHER_MODE_CTR)
    { POLARSSL_CIPHER_AES_128_CTR,          &aes_128_ctr_info },
    { POLARSSL_CIPHER_AES_192_CTR,          &aes_192_ctr_info },
    { POLARSSL_CIPHER_AES_256_CTR,          &aes_256_ctr_info },
#endif
#if defined(POLARSSL_GCM_C)
    { POLARSSL_CIPHER_AES_128_GCM,          &aes_128_gcm_info },
    { POLARSSL_CIPHER_AES_192_GCM,          &aes_192_gcm_info },
    { POLARSSL_CIPHER_AES_256_GCM,          &aes_256_gcm_info },
#endif
#if defined(POLARSSL_CCM_C)
    { POLARSSL_CIPHER_AES_128_CCM,          &aes_128_ccm_info },
    { POLARSSL_CIPHER_AES_192_CCM,          &aes_192_ccm_info },
    { POLARSSL_CIPHER_AES_256_CCM,          &aes_256_ccm_info },
#endif
#endif /* POLARSSL_AES_C */

#if defined(POLARSSL_ARC4_C)
    { POLARSSL_CIPHER_ARC4_128,             &arc4_128_info },
#endif

#if defined(POLARSSL_BLOWFISH_C)
    { POLARSSL_CIPHER_BLOWFISH_ECB,         &blowfish_ecb_info },
#if defined(POLARSSL_CIPHER_MODE_CBC)
    { POLARSSL_CIPHER_BLOWFISH_CBC,         &blowfish_cbc_info },
#endif
#if defined(POLARSSL_CIPHER_MODE_CFB)
    { POLARSSL_CIPHER_BLOWFISH_CFB64,       &blowfish_cfb64_info },
#endif
#if defined(POLARSSL_CIPHER_MODE_CTR)
    { POLARSSL_CIPHER_BLOWFISH_CTR,         &blowfish_ctr_info },
#endif
#endif /* POLARSSL_BLOWFISH_C */

#if defined(POLARSSL_CAMELLIA_C)
    { POLARSSL_CIPHER_CAMELLIA_128_ECB,     &camellia_128_ecb_info },
    { POLARSSL_CIPHER_CAMELLIA_192_ECB,     &camellia_192_ecb_info },
    { POLARSSL_CIPHER_CAMELLIA_256_ECB,     &camellia_256_ecb_info },
#if defined(POLARSSL_CIPHER_MODE_CBC)
    { POLARSSL_CIPHER_CAMELLIA_128_CBC,     &camellia_128_cbc_info },
    { POLARSSL_CIPHER_CAMELLIA_192_CBC,     &camellia_192_cbc_info },
    { POLARSSL_CIPHER_CAMELLIA_256_CBC,     &camellia_256_cbc_info },
#endif
#if defined(POLARSSL_CIPHER_MODE_CFB)
    { POLARSSL_CIPHER_CAMELLIA_128_CFB128,  &camellia_128_cfb128_info },
    { POLARSSL_CIPHER_CAMELLIA_192_CFB128,  &camellia_192_cfb128_info },
    { POLARSSL_CIPHER_CAMELLIA_256_CFB128,  &camellia_256_cfb128_info },
#endif
#if defined(POLARSSL_CIPHER_MODE_CTR)
    { POLARSSL_CIPHER_CAMELLIA_128_CTR,     &camellia_128_ctr_info },
    { POLARSSL_CIPHER_CAMELLIA_192_CTR,     &camellia_192_ctr_info },
    { POLARSSL_CIPHER_CAMELLIA_256_CTR,     &camellia_256_ctr_info },
#endif
#if defined(POLARSSL_GCM_C)
    { POLARSSL_CIPHER_CAMELLIA_128_GCM,     &camellia_128_gcm_info },
    { POLARSSL_CIPHER_CAMELLIA_192_GCM,     &camellia_192_gcm_info },
    { POLARSSL_CIPHER_CAMELLIA_256_GCM,     &camellia_256_gcm_info },
#endif
#if defined(POLARSSL_CCM_C)
    { POLARSSL_CIPHER_CAMELLIA_128_CCM,     &camellia_128_ccm_info },
    { POLARSSL_CIPHER_CAMELLIA_192_CCM,     &camellia_192_ccm_info },
    { POLARSSL_CIPHER_CAMELLIA_256_CCM,     &camellia_256_ccm_info },
#endif
#endif /* POLARSSL_CAMELLIA_C */

#if defined(POLARSSL_DES_C)
    { POLARSSL_CIPHER_DES_ECB,              &des_ecb_info },
    { POLARSSL_CIPHER_DES_EDE_ECB,          &des_ede_ecb_info },
    { POLARSSL_CIPHER_DES_EDE3_ECB,         &des_ede3_ecb_info },
#if defined(POLARSSL_CIPHER_MODE_CBC)
    { POLARSSL_CIPHER_DES_CBC,              &des_cbc_info },
    { POLARSSL_CIPHER_DES_EDE_CBC,          &des_ede_cbc_info },
    { POLARSSL_CIPHER_DES_EDE3_CBC,         &des_ede3_cbc_info },
#endif
#endif /* POLARSSL_DES_C */

#if defined(POLARSSL_CIPHER_NULL_CIPHER)
    { POLARSSL_CIPHER_NULL,                 &null_cipher_info },
#endif /* POLARSSL_CIPHER_NULL_CIPHER */

    { 0, NULL }
};

#define NUM_CIPHERS sizeof cipher_definitions / sizeof cipher_definitions[0]
int supported_ciphers[NUM_CIPHERS];

#endif /* POLARSSL_CIPHER_C */
