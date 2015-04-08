/**
 * \file camellia.h
 *
 * \brief Camellia block cipher
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
#ifndef POLARSSL_CAMELLIA_H
#define POLARSSL_CAMELLIA_H

#if !defined(POLARSSL_CONFIG_FILE)
#include "config.h"
#else
#include POLARSSL_CONFIG_FILE
#endif

#include <string.h>

#if defined(_MSC_VER) && !defined(EFIX64) && !defined(EFI32)
#include <basetsd.h>
typedef UINT32 uint32_t;
#else
#include <inttypes.h>
#endif

#define CAMELLIA_ENCRYPT     1
#define CAMELLIA_DECRYPT     0

#define POLARSSL_ERR_CAMELLIA_INVALID_KEY_LENGTH           -0x0024  /**< Invalid key length. */
#define POLARSSL_ERR_CAMELLIA_INVALID_INPUT_LENGTH         -0x0026  /**< Invalid data input length. */

#if !defined(POLARSSL_CAMELLIA_ALT)
// Regular implementation
//

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief          CAMELLIA context structure
 */
typedef struct
{
    int nr;                     /*!<  number of rounds  */
    uint32_t rk[68];            /*!<  CAMELLIA round keys    */
}
camellia_context;

/**
 * \brief          Initialize CAMELLIA context
 *
 * \param ctx      CAMELLIA context to be initialized
 */
void camellia_init( camellia_context *ctx );

/**
 * \brief          Clear CAMELLIA context
 *
 * \param ctx      CAMELLIA context to be cleared
 */
void camellia_free( camellia_context *ctx );

/**
 * \brief          CAMELLIA key schedule (encryption)
 *
 * \param ctx      CAMELLIA context to be initialized
 * \param key      encryption key
 * \param keysize  must be 128, 192 or 256
 *
 * \return         0 if successful, or POLARSSL_ERR_CAMELLIA_INVALID_KEY_LENGTH
 */
int camellia_setkey_enc( camellia_context *ctx, const unsigned char *key,
                         unsigned int keysize );

/**
 * \brief          CAMELLIA key schedule (decryption)
 *
 * \param ctx      CAMELLIA context to be initialized
 * \param key      decryption key
 * \param keysize  must be 128, 192 or 256
 *
 * \return         0 if successful, or POLARSSL_ERR_CAMELLIA_INVALID_KEY_LENGTH
 */
int camellia_setkey_dec( camellia_context *ctx, const unsigned char *key,
                         unsigned int keysize );

/**
 * \brief          CAMELLIA-ECB block encryption/decryption
 *
 * \param ctx      CAMELLIA context
 * \param mode     CAMELLIA_ENCRYPT or CAMELLIA_DECRYPT
 * \param input    16-byte input block
 * \param output   16-byte output block
 *
 * \return         0 if successful
 */
int camellia_crypt_ecb( camellia_context *ctx,
                    int mode,
                    const unsigned char input[16],
                    unsigned char output[16] );

#if defined(POLARSSL_CIPHER_MODE_CBC)
/**
 * \brief          CAMELLIA-CBC buffer encryption/decryption
 *                 Length should be a multiple of the block
 *                 size (16 bytes)
 *
 * \note           Upon exit, the content of the IV is updated so that you can
 *                 call the function same function again on the following
 *                 block(s) of data and get the same result as if it was
 *                 encrypted in one call. This allows a "streaming" usage.
 *                 If on the other hand you need to retain the contents of the
 *                 IV, you should either save it manually or use the cipher
 *                 module instead.
 *
 * \param ctx      CAMELLIA context
 * \param mode     CAMELLIA_ENCRYPT or CAMELLIA_DECRYPT
 * \param length   length of the input data
 * \param iv       initialization vector (updated after use)
 * \param input    buffer holding the input data
 * \param output   buffer holding the output data
 *
 * \return         0 if successful, or
 *                 POLARSSL_ERR_CAMELLIA_INVALID_INPUT_LENGTH
 */
int camellia_crypt_cbc( camellia_context *ctx,
                    int mode,
                    size_t length,
                    unsigned char iv[16],
                    const unsigned char *input,
                    unsigned char *output );
#endif /* POLARSSL_CIPHER_MODE_CBC */

#if defined(POLARSSL_CIPHER_MODE_CFB)
/**
 * \brief          CAMELLIA-CFB128 buffer encryption/decryption
 *
 * Note: Due to the nature of CFB you should use the same key schedule for
 * both encryption and decryption. So a context initialized with
 * camellia_setkey_enc() for both CAMELLIA_ENCRYPT and CAMELLIE_DECRYPT.
 *
 * \note           Upon exit, the content of the IV is updated so that you can
 *                 call the function same function again on the following
 *                 block(s) of data and get the same result as if it was
 *                 encrypted in one call. This allows a "streaming" usage.
 *                 If on the other hand you need to retain the contents of the
 *                 IV, you should either save it manually or use the cipher
 *                 module instead.
 *
 * \param ctx      CAMELLIA context
 * \param mode     CAMELLIA_ENCRYPT or CAMELLIA_DECRYPT
 * \param length   length of the input data
 * \param iv_off   offset in IV (updated after use)
 * \param iv       initialization vector (updated after use)
 * \param input    buffer holding the input data
 * \param output   buffer holding the output data
 *
 * \return         0 if successful, or
 *                 POLARSSL_ERR_CAMELLIA_INVALID_INPUT_LENGTH
 */
int camellia_crypt_cfb128( camellia_context *ctx,
                       int mode,
                       size_t length,
                       size_t *iv_off,
                       unsigned char iv[16],
                       const unsigned char *input,
                       unsigned char *output );
#endif /* POLARSSL_CIPHER_MODE_CFB */

#if defined(POLARSSL_CIPHER_MODE_CTR)
/**
 * \brief               CAMELLIA-CTR buffer encryption/decryption
 *
 * Warning: You have to keep the maximum use of your counter in mind!
 *
 * Note: Due to the nature of CTR you should use the same key schedule for
 * both encryption and decryption. So a context initialized with
 * camellia_setkey_enc() for both CAMELLIA_ENCRYPT and CAMELLIA_DECRYPT.
 *
 * \param ctx           CAMELLIA context
 * \param length        The length of the data
 * \param nc_off        The offset in the current stream_block (for resuming
 *                      within current cipher stream). The offset pointer to
 *                      should be 0 at the start of a stream.
 * \param nonce_counter The 128-bit nonce and counter.
 * \param stream_block  The saved stream-block for resuming. Is overwritten
 *                      by the function.
 * \param input         The input data stream
 * \param output        The output data stream
 *
 * \return         0 if successful
 */
int camellia_crypt_ctr( camellia_context *ctx,
                       size_t length,
                       size_t *nc_off,
                       unsigned char nonce_counter[16],
                       unsigned char stream_block[16],
                       const unsigned char *input,
                       unsigned char *output );
#endif /* POLARSSL_CIPHER_MODE_CTR */

#ifdef __cplusplus
}
#endif

#else  /* POLARSSL_CAMELLIA_ALT */
#include "camellia_alt.h"
#endif /* POLARSSL_CAMELLIA_ALT */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief          Checkup routine
 *
 * \return         0 if successful, or 1 if the test failed
 */
int camellia_self_test( int verbose );

#ifdef __cplusplus
}
#endif

#endif /* camellia.h */
