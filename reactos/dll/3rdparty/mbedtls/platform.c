/*
 *  Platform abstraction layer
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

#if defined(POLARSSL_PLATFORM_C)

#include "polarssl/platform.h"

#if defined(POLARSSL_PLATFORM_MEMORY)
#if !defined(POLARSSL_PLATFORM_STD_MALLOC)
static void *platform_malloc_uninit( size_t len )
{
    ((void) len);
    return( NULL );
}

#define POLARSSL_PLATFORM_STD_MALLOC   platform_malloc_uninit
#endif /* !POLARSSL_PLATFORM_STD_MALLOC */

#if !defined(POLARSSL_PLATFORM_STD_FREE)
static void platform_free_uninit( void *ptr )
{
    ((void) ptr);
}

#define POLARSSL_PLATFORM_STD_FREE     platform_free_uninit
#endif /* !POLARSSL_PLATFORM_STD_FREE */

void * (*polarssl_malloc)( size_t ) = POLARSSL_PLATFORM_STD_MALLOC;
void (*polarssl_free)( void * )     = POLARSSL_PLATFORM_STD_FREE;

int platform_set_malloc_free( void * (*malloc_func)( size_t ),
                              void (*free_func)( void * ) )
{
    polarssl_malloc = malloc_func;
    polarssl_free = free_func;
    return( 0 );
}
#endif /* POLARSSL_PLATFORM_MEMORY */

#if defined(POLARSSL_PLATFORM_PRINTF_ALT)
#if !defined(POLARSSL_PLATFORM_STD_PRINTF)
/*
 * Make dummy function to prevent NULL pointer dereferences
 */
static int platform_printf_uninit( const char *format, ... )
{
    ((void) format);
    return( 0 );
}

#define POLARSSL_PLATFORM_STD_PRINTF    platform_printf_uninit
#endif /* !POLARSSL_PLATFORM_STD_PRINTF */

int (*polarssl_printf)( const char *, ... ) = POLARSSL_PLATFORM_STD_PRINTF;

int platform_set_printf( int (*printf_func)( const char *, ... ) )
{
    polarssl_printf = printf_func;
    return( 0 );
}
#endif /* POLARSSL_PLATFORM_PRINTF_ALT */

#if defined(POLARSSL_PLATFORM_FPRINTF_ALT)
#if !defined(POLARSSL_PLATFORM_STD_FPRINTF)
/*
 * Make dummy function to prevent NULL pointer dereferences
 */
static int platform_fprintf_uninit( FILE *stream, const char *format, ... )
{
    ((void) stream);
    ((void) format);
    return( 0 );
}

#define POLARSSL_PLATFORM_STD_FPRINTF   platform_fprintf_uninit
#endif /* !POLARSSL_PLATFORM_STD_FPRINTF */

int (*polarssl_fprintf)( FILE *, const char *, ... ) =
                                        POLARSSL_PLATFORM_STD_FPRINTF;

int platform_set_fprintf( int (*fprintf_func)( FILE *, const char *, ... ) )
{
    polarssl_fprintf = fprintf_func;
    return( 0 );
}
#endif /* POLARSSL_PLATFORM_FPRINTF_ALT */

#endif /* POLARSSL_PLATFORM_C */
