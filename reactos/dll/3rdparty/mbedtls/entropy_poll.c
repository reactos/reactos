/*
 *  Platform-specific and custom entropy polling functions
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

#if defined(POLARSSL_ENTROPY_C)

#include "polarssl/entropy.h"
#include "polarssl/entropy_poll.h"

#if defined(POLARSSL_TIMING_C)
#include "polarssl/timing.h"
#endif
#if defined(POLARSSL_HAVEGE_C)
#include "polarssl/havege.h"
#endif

#if !defined(POLARSSL_NO_PLATFORM_ENTROPY)
#if defined(_WIN32) && !defined(EFIX64) && !defined(EFI32)

#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0400
#endif
#include <windows.h>
#include <wincrypt.h>

int platform_entropy_poll( void *data, unsigned char *output, size_t len,
                           size_t *olen )
{
    HCRYPTPROV provider;
    ((void) data);
    *olen = 0;

    if( CryptAcquireContext( &provider, NULL, NULL,
                              PROV_RSA_FULL, CRYPT_VERIFYCONTEXT ) == FALSE )
    {
        return( POLARSSL_ERR_ENTROPY_SOURCE_FAILED );
    }

    if( CryptGenRandom( provider, (DWORD) len, output ) == FALSE )
        return( POLARSSL_ERR_ENTROPY_SOURCE_FAILED );

    CryptReleaseContext( provider, 0 );
    *olen = len;

    return( 0 );
}
#else /* _WIN32 && !EFIX64 && !EFI32 */

/*
 * Test for Linux getrandom() support.
 * Since there is no wrapper in the libc yet, use the generic syscall wrapper
 * available in GNU libc and compatible libc's (eg uClibc).
 */
#if defined(__linux__) && defined(__GLIBC__)
#include <linux/version.h>
#include <unistd.h>
#include <sys/syscall.h>
#if defined(SYS_getrandom)
#define HAVE_GETRANDOM
static int getrandom_wrapper( void *buf, size_t buflen, unsigned int flags )
{
    return( syscall( SYS_getrandom, buf, buflen, flags ) );
}
#endif /* SYS_getrandom */
#endif /* __linux__ */

#if defined(HAVE_GETRANDOM)

#include <errno.h>

int platform_entropy_poll( void *data,
                           unsigned char *output, size_t len, size_t *olen )
{
    int ret;
    ((void) data);

    if( ( ret = getrandom_wrapper( output, len, 0 ) ) < 0 )
        return( POLARSSL_ERR_ENTROPY_SOURCE_FAILED );

    *olen = ret;
    return( 0 );
}

#else /* HAVE_GETRANDOM */

#include <stdio.h>

int platform_entropy_poll( void *data,
                           unsigned char *output, size_t len, size_t *olen )
{
    FILE *file;
    size_t ret;
    ((void) data);

    *olen = 0;

    file = fopen( "/dev/urandom", "rb" );
    if( file == NULL )
        return( POLARSSL_ERR_ENTROPY_SOURCE_FAILED );

    ret = fread( output, 1, len, file );
    if( ret != len )
    {
        fclose( file );
        return( POLARSSL_ERR_ENTROPY_SOURCE_FAILED );
    }

    fclose( file );
    *olen = len;

    return( 0 );
}
#endif /* HAVE_GETRANDOM */
#endif /* _WIN32 && !EFIX64 && !EFI32 */
#endif /* !POLARSSL_NO_PLATFORM_ENTROPY */

#if defined(POLARSSL_TIMING_C)
int hardclock_poll( void *data,
                    unsigned char *output, size_t len, size_t *olen )
{
    unsigned long timer = hardclock();
    ((void) data);
    *olen = 0;

    if( len < sizeof(unsigned long) )
        return( 0 );

    memcpy( output, &timer, sizeof(unsigned long) );
    *olen = sizeof(unsigned long);

    return( 0 );
}
#endif /* POLARSSL_TIMING_C */

#if defined(POLARSSL_HAVEGE_C)
int havege_poll( void *data,
                 unsigned char *output, size_t len, size_t *olen )
{
    havege_state *hs = (havege_state *) data;
    *olen = 0;

    if( havege_random( hs, output, len ) != 0 )
        return( POLARSSL_ERR_ENTROPY_SOURCE_FAILED );

    *olen = len;

    return( 0 );
}
#endif /* POLARSSL_HAVEGE_C */

#endif /* POLARSSL_ENTROPY_C */
