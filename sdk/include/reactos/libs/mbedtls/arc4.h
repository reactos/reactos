/**
 * \file arc4.h
 *
 * \brief The ARCFOUR stream cipher
 *
 * \warning   ARC4 is considered a weak cipher and its use constitutes a
 *            security risk. We recommend considering stronger ciphers instead.
 */
/*
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
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
 *
 */
#ifndef MBEDTLS_ARC4_H
#define MBEDTLS_ARC4_H

#if !defined(MBEDTLS_CONFIG_FILE)
#include "config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include <stddef.h>

#define MBEDTLS_ERR_ARC4_HW_ACCEL_FAILED                  -0x0019  /**< ARC4 hardware accelerator failed. */

#if !defined(MBEDTLS_ARC4_ALT)
// Regular implementation
//

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief     ARC4 context structure
 *
 * \warning   ARC4 is considered a weak cipher and its use constitutes a
 *            security risk. We recommend considering stronger ciphers instead.
 *
 */
typedef struct
{
    int x;                      /*!< permutation index */
    int y;                      /*!< permutation index */
    unsigned char m[256];       /*!< permutation table */
}
mbedtls_arc4_context;

/**
 * \brief          Initialize ARC4 context
 *
 * \param ctx      ARC4 context to be initialized
 *
 * \warning        ARC4 is considered a weak cipher and its use constitutes a
 *                 security risk. We recommend considering stronger ciphers
 *                 instead.
 *
 */
void mbedtls_arc4_init( mbedtls_arc4_context *ctx );

/**
 * \brief          Clear ARC4 context
 *
 * \param ctx      ARC4 context to be cleared
 *
 * \warning        ARC4 is considered a weak cipher and its use constitutes a
 *                 security risk. We recommend considering stronger ciphers
 *                 instead.
 *
 */
void mbedtls_arc4_free( mbedtls_arc4_context *ctx );

/**
 * \brief          ARC4 key schedule
 *
 * \param ctx      ARC4 context to be setup
 * \param key      the secret key
 * \param keylen   length of the key, in bytes
 *
 * \warning        ARC4 is considered a weak cipher and its use constitutes a
 *                 security risk. We recommend considering stronger ciphers
 *                 instead.
 *
 */
void mbedtls_arc4_setup( mbedtls_arc4_context *ctx, const unsigned char *key,
                 unsigned int keylen );

/**
 * \brief          ARC4 cipher function
 *
 * \param ctx      ARC4 context
 * \param length   length of the input data
 * \param input    buffer holding the input data
 * \param output   buffer for the output data
 *
 * \return         0 if successful
 *
 * \warning        ARC4 is considered a weak cipher and its use constitutes a
 *                 security risk. We recommend considering stronger ciphers
 *                 instead.
 *
 */
int mbedtls_arc4_crypt( mbedtls_arc4_context *ctx, size_t length, const unsigned char *input,
                unsigned char *output );

#ifdef __cplusplus
}
#endif

#else  /* MBEDTLS_ARC4_ALT */
#include "arc4_alt.h"
#endif /* MBEDTLS_ARC4_ALT */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief          Checkup routine
 *
 * \return         0 if successful, or 1 if the test failed
 *
 * \warning        ARC4 is considered a weak cipher and its use constitutes a
 *                 security risk. We recommend considering stronger ciphers
 *                 instead.
 *
 */
int mbedtls_arc4_self_test( int verbose );

#ifdef __cplusplus
}
#endif

#endif /* arc4.h */
