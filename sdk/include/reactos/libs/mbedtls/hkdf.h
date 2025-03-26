/**
 * \file hkdf.h
 *
 * \brief   This file contains the HKDF interface.
 *
 *          The HMAC-based Extract-and-Expand Key Derivation Function (HKDF) is
 *          specified by RFC 5869.
 */
/*
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
#ifndef MBEDTLS_HKDF_H
#define MBEDTLS_HKDF_H

#if !defined(MBEDTLS_CONFIG_FILE)
#include "config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include "md.h"

/**
 *  \name HKDF Error codes
 *  \{
 */
#define MBEDTLS_ERR_HKDF_BAD_INPUT_DATA  -0x5F80  /**< Bad input parameters to function. */
/* \} name */

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  \brief  This is the HMAC-based Extract-and-Expand Key Derivation Function
 *          (HKDF).
 *
 *  \param  md        A hash function; md.size denotes the length of the hash
 *                    function output in bytes.
 *  \param  salt      An optional salt value (a non-secret random value);
 *                    if the salt is not provided, a string of all zeros of
 *                    md.size length is used as the salt.
 *  \param  salt_len  The length in bytes of the optional \p salt.
 *  \param  ikm       The input keying material.
 *  \param  ikm_len   The length in bytes of \p ikm.
 *  \param  info      An optional context and application specific information
 *                    string. This can be a zero-length string.
 *  \param  info_len  The length of \p info in bytes.
 *  \param  okm       The output keying material of \p okm_len bytes.
 *  \param  okm_len   The length of the output keying material in bytes. This
 *                    must be less than or equal to 255 * md.size bytes.
 *
 *  \return 0 on success.
 *  \return #MBEDTLS_ERR_HKDF_BAD_INPUT_DATA when the parameters are invalid.
 *  \return An MBEDTLS_ERR_MD_* error for errors returned from the underlying
 *          MD layer.
 */
int mbedtls_hkdf( const mbedtls_md_info_t *md, const unsigned char *salt,
                  size_t salt_len, const unsigned char *ikm, size_t ikm_len,
                  const unsigned char *info, size_t info_len,
                  unsigned char *okm, size_t okm_len );

/**
 *  \brief  Take the input keying material \p ikm and extract from it a
 *          fixed-length pseudorandom key \p prk.
 *
 *  \warning    This function should only be used if the security of it has been
 *              studied and established in that particular context (eg. TLS 1.3
 *              key schedule). For standard HKDF security guarantees use
 *              \c mbedtls_hkdf instead.
 *
 *  \param       md        A hash function; md.size denotes the length of the
 *                         hash function output in bytes.
 *  \param       salt      An optional salt value (a non-secret random value);
 *                         if the salt is not provided, a string of all zeros
 *                         of md.size length is used as the salt.
 *  \param       salt_len  The length in bytes of the optional \p salt.
 *  \param       ikm       The input keying material.
 *  \param       ikm_len   The length in bytes of \p ikm.
 *  \param[out]  prk       A pseudorandom key of at least md.size bytes.
 *
 *  \return 0 on success.
 *  \return #MBEDTLS_ERR_HKDF_BAD_INPUT_DATA when the parameters are invalid.
 *  \return An MBEDTLS_ERR_MD_* error for errors returned from the underlying
 *          MD layer.
 */
int mbedtls_hkdf_extract( const mbedtls_md_info_t *md,
                          const unsigned char *salt, size_t salt_len,
                          const unsigned char *ikm, size_t ikm_len,
                          unsigned char *prk );

/**
 *  \brief  Expand the supplied \p prk into several additional pseudorandom
 *          keys, which is the output of the HKDF.
 *
 *  \warning    This function should only be used if the security of it has been
 *              studied and established in that particular context (eg. TLS 1.3
 *              key schedule). For standard HKDF security guarantees use
 *              \c mbedtls_hkdf instead.
 *
 *  \param  md        A hash function; md.size denotes the length of the hash
 *                    function output in bytes.
 *  \param  prk       A pseudorandom key of at least md.size bytes. \p prk is
 *                    usually the output from the HKDF extract step.
 *  \param  prk_len   The length in bytes of \p prk.
 *  \param  info      An optional context and application specific information
 *                    string. This can be a zero-length string.
 *  \param  info_len  The length of \p info in bytes.
 *  \param  okm       The output keying material of \p okm_len bytes.
 *  \param  okm_len   The length of the output keying material in bytes. This
 *                    must be less than or equal to 255 * md.size bytes.
 *
 *  \return 0 on success.
 *  \return #MBEDTLS_ERR_HKDF_BAD_INPUT_DATA when the parameters are invalid.
 *  \return An MBEDTLS_ERR_MD_* error for errors returned from the underlying
 *          MD layer.
 */
int mbedtls_hkdf_expand( const mbedtls_md_info_t *md, const unsigned char *prk,
                         size_t prk_len, const unsigned char *info,
                         size_t info_len, unsigned char *okm, size_t okm_len );

#ifdef __cplusplus
}
#endif

#endif /* hkdf.h */
