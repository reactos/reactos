/*
 *  Version information
 *
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
 */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_VERSION_C)

#include "mbedtls/version.h"
#include <string.h>

unsigned int mbedtls_version_get_number()
{
    return( MBEDTLS_VERSION_NUMBER );
}

void mbedtls_version_get_string( char *string )
{
    memcpy( string, MBEDTLS_VERSION_STRING,
            sizeof( MBEDTLS_VERSION_STRING ) );
}

void mbedtls_version_get_string_full( char *string )
{
    memcpy( string, MBEDTLS_VERSION_STRING_FULL,
            sizeof( MBEDTLS_VERSION_STRING_FULL ) );
}

#endif /* MBEDTLS_VERSION_C */
