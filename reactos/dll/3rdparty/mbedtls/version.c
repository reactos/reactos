/*
 *  Version information
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

#if defined(POLARSSL_VERSION_C)

#include "polarssl/version.h"
#include <string.h>

const char version[] = POLARSSL_VERSION_STRING;

unsigned int version_get_number()
{
    return( POLARSSL_VERSION_NUMBER );
}

void version_get_string( char *string )
{
    memcpy( string, POLARSSL_VERSION_STRING,
            sizeof( POLARSSL_VERSION_STRING ) );
}

void version_get_string_full( char *string )
{
    memcpy( string, POLARSSL_VERSION_STRING_FULL,
            sizeof( POLARSSL_VERSION_STRING_FULL ) );
}

#endif /* POLARSSL_VERSION_C */
