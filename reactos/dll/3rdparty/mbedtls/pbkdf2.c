/**
 * \file pbkdf2.c
 *
 * \brief Password-Based Key Derivation Function 2 (from PKCS#5)
 *        DEPRECATED: Use pkcs5.c instead
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
 * PBKDF2 is part of PKCS#5
 *
 * http://tools.ietf.org/html/rfc2898 (Specification)
 * http://tools.ietf.org/html/rfc6070 (Test vectors)
 */

#if !defined(POLARSSL_CONFIG_FILE)
#include "polarssl/config.h"
#else
#include POLARSSL_CONFIG_FILE
#endif

#if defined(POLARSSL_PBKDF2_C)

#include "polarssl/pbkdf2.h"
#include "polarssl/pkcs5.h"

int pbkdf2_hmac( md_context_t *ctx, const unsigned char *password, size_t plen,
                 const unsigned char *salt, size_t slen,
                 unsigned int iteration_count,
                 uint32_t key_length, unsigned char *output )
{
    return pkcs5_pbkdf2_hmac( ctx, password, plen, salt, slen, iteration_count,
                              key_length, output );
}

#if defined(POLARSSL_SELF_TEST)
int pbkdf2_self_test( int verbose )
{
    return pkcs5_self_test( verbose );
}
#endif /* POLARSSL_SELF_TEST */

#endif /* POLARSSL_PBKDF2_C */
