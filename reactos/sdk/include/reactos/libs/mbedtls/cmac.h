/**
 * \file cmac.h
 *
 * \brief Cipher-based Message Authentication Code (CMAC) Mode for
 *        Authentication
 *
 *  Copyright (C) 2015-2016, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: GPL-2.0
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
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */
#ifndef MBEDTLS_CMAC_H
#define MBEDTLS_CMAC_H

#include "mbedtls/cipher.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MBEDTLS_AES_BLOCK_SIZE          16
#define MBEDTLS_DES3_BLOCK_SIZE         8

#if defined(MBEDTLS_AES_C)
#define MBEDTLS_CIPHER_BLKSIZE_MAX      16  /* longest used by CMAC is AES */
#else
#define MBEDTLS_CIPHER_BLKSIZE_MAX      8   /* longest used by CMAC is 3DES */
#endif

/**
 * CMAC context structure - Contains internal state information only
 */
struct mbedtls_cmac_context_t
{
    /** Internal state of the CMAC algorithm  */
    unsigned char       state[MBEDTLS_CIPHER_BLKSIZE_MAX];

    /** Unprocessed data - either data that was not block aligned and is still
     *  pending to be processed, or the final block */
    unsigned char       unprocessed_block[MBEDTLS_CIPHER_BLKSIZE_MAX];

    /** Length of data pending to be processed */
    size_t              unprocessed_len;
};

/**
 * \brief               Set the CMAC key and prepare to authenticate the input
 *                      data.
 *                      Should be called with an initialised cipher context.
 *
 * \param ctx           Cipher context
 * \param key           CMAC key
 * \param keybits       length of the CMAC key in bits
 *                      (must be acceptable by the cipher)
 *
 * \return              0 if successful, or a cipher specific error code
 */
int mbedtls_cipher_cmac_starts( mbedtls_cipher_context_t *ctx,
                                const unsigned char *key, size_t keybits );

/**
 * \brief               Generic CMAC process buffer.
 *                      Called between mbedtls_cipher_cmac_starts() or
 *                      mbedtls_cipher_cmac_reset() and
 *                      mbedtls_cipher_cmac_finish().
 *                      May be called repeatedly.
 *
 * \param ctx           CMAC context
 * \param input         buffer holding the  data
 * \param ilen          length of the input data
 *
 * \returns             0 on success, MBEDTLS_ERR_MD_BAD_INPUT_DATA if parameter
 *                      verification fails.
 */
int mbedtls_cipher_cmac_update( mbedtls_cipher_context_t *ctx,
                                const unsigned char *input, size_t ilen );

/**
 * \brief               Output CMAC.
 *                      Called after mbedtls_cipher_cmac_update().
 *                      Usually followed by mbedtls_cipher_cmac_reset(), then
 *                      mbedtls_cipher_cmac_starts(), or mbedtls_cipher_free().
 *
 * \param ctx           CMAC context
 * \param output        Generic CMAC checksum result
 *
 * \returns             0 on success, MBEDTLS_ERR_MD_BAD_INPUT_DATA if parameter
 *                      verification fails.
 */
int mbedtls_cipher_cmac_finish( mbedtls_cipher_context_t *ctx,
                                unsigned char *output );

/**
 * \brief               Prepare to authenticate a new message with the same key.
 *                      Called after mbedtls_cipher_cmac_finish() and before
 *                      mbedtls_cipher_cmac_update().
 *
 * \param ctx           CMAC context to be reset
 *
 * \returns             0 on success, MBEDTLS_ERR_MD_BAD_INPUT_DATA if parameter
 *                      verification fails.
 */
int mbedtls_cipher_cmac_reset( mbedtls_cipher_context_t *ctx );

/**
 * \brief               Output = Generic_CMAC( hmac key, input buffer )
 *
 * \param cipher_info   message digest info
 * \param key           CMAC key
 * \param keylen        length of the CMAC key in bits
 * \param input         buffer holding the  data
 * \param ilen          length of the input data
 * \param output        Generic CMAC-result
 *
 * \returns             0 on success, MBEDTLS_ERR_MD_BAD_INPUT_DATA if parameter
 *                      verification fails.
 */
int mbedtls_cipher_cmac( const mbedtls_cipher_info_t *cipher_info,
                         const unsigned char *key, size_t keylen,
                         const unsigned char *input, size_t ilen,
                         unsigned char *output );

#if defined(MBEDTLS_AES_C)
/**
 * \brief           AES-CMAC-128-PRF
 *                  Implementation of (AES-CMAC-PRF-128), as defined in RFC 4615
 *
 * \param key       PRF key
 * \param key_len   PRF key length in bytes
 * \param input     buffer holding the input data
 * \param in_len    length of the input data in bytes
 * \param output    buffer holding the generated pseudorandom output (16 bytes)
 *
 * \return          0 if successful
 */
int mbedtls_aes_cmac_prf_128( const unsigned char *key, size_t key_len,
                              const unsigned char *input, size_t in_len,
                              unsigned char output[16] );
#endif /* MBEDTLS_AES_C */

#if defined(MBEDTLS_SELF_TEST) && ( defined(MBEDTLS_AES_C) || defined(MBEDTLS_DES_C) )
/**
 * \brief          Checkup routine
 *
 * \return         0 if successful, or 1 if the test failed
 */
int mbedtls_cmac_self_test( int verbose );
#endif /* MBEDTLS_SELF_TEST && ( MBEDTLS_AES_C || MBEDTLS_DES_C ) */

#ifdef __cplusplus
}
#endif

#endif /* MBEDTLS_CMAC_H */
