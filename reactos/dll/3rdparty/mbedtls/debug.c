/*
 *  Debugging routines
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

#if defined(POLARSSL_DEBUG_C)

#include "polarssl/debug.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#if defined(_MSC_VER) && !defined(EFIX64) && !defined(EFI32)
#if !defined  snprintf
#define  snprintf  _snprintf
#endif

#if !defined vsnprintf
#define vsnprintf _vsnprintf
#endif
#endif /* _MSC_VER */

static int debug_log_mode = POLARSSL_DEBUG_DFL_MODE;
static int debug_threshold = 0;

void debug_set_log_mode( int log_mode )
{
    debug_log_mode = log_mode;
}

void debug_set_threshold( int threshold )
{
    debug_threshold = threshold;
}

char *debug_fmt( const char *format, ... )
{
    va_list argp;
    static char str[512];
    int maxlen = sizeof( str ) - 1;

    va_start( argp, format );
    vsnprintf( str, maxlen, format, argp );
    va_end( argp );

    str[maxlen] = '\0';
    return( str );
}

void debug_print_msg( const ssl_context *ssl, int level,
                      const char *file, int line, const char *text )
{
    char str[512];
    int maxlen = sizeof( str ) - 1;

    if( ssl->f_dbg == NULL || level > debug_threshold )
        return;

    if( debug_log_mode == POLARSSL_DEBUG_LOG_RAW )
    {
        ssl->f_dbg( ssl->p_dbg, level, text );
        return;
    }

    snprintf( str, maxlen, "%s(%04d): %s\n", file, line, text );
    str[maxlen] = '\0';
    ssl->f_dbg( ssl->p_dbg, level, str );
}

void debug_print_ret( const ssl_context *ssl, int level,
                      const char *file, int line,
                      const char *text, int ret )
{
    char str[512];
    int maxlen = sizeof( str ) - 1;
    size_t idx = 0;

    if( ssl->f_dbg == NULL || level > debug_threshold )
        return;

    if( debug_log_mode == POLARSSL_DEBUG_LOG_FULL )
        idx = snprintf( str, maxlen, "%s(%04d): ", file, line );

    snprintf( str + idx, maxlen - idx, "%s() returned %d (-0x%04x)\n",
              text, ret, -ret );

    str[maxlen] = '\0';
    ssl->f_dbg( ssl->p_dbg, level, str );
}

void debug_print_buf( const ssl_context *ssl, int level,
                      const char *file, int line, const char *text,
                      unsigned char *buf, size_t len )
{
    char str[512];
    char txt[17];
    size_t i, maxlen = sizeof( str ) - 1, idx = 0;

    if( ssl->f_dbg == NULL || level > debug_threshold )
        return;

    if( debug_log_mode == POLARSSL_DEBUG_LOG_FULL )
        idx = snprintf( str, maxlen, "%s(%04d): ", file, line );

    snprintf( str + idx, maxlen - idx, "dumping '%s' (%u bytes)\n",
              text, (unsigned int) len );

    str[maxlen] = '\0';
    ssl->f_dbg( ssl->p_dbg, level, str );

    idx = 0;
    memset( txt, 0, sizeof( txt ) );
    for( i = 0; i < len; i++ )
    {
        if( i >= 4096 )
            break;

        if( i % 16 == 0 )
        {
            if( i > 0 )
            {
                snprintf( str + idx, maxlen - idx, "  %s\n", txt );
                ssl->f_dbg( ssl->p_dbg, level, str );

                idx = 0;
                memset( txt, 0, sizeof( txt ) );
            }

            if( debug_log_mode == POLARSSL_DEBUG_LOG_FULL )
                idx = snprintf( str, maxlen, "%s(%04d): ", file, line );

            idx += snprintf( str + idx, maxlen - idx, "%04x: ",
                             (unsigned int) i );

        }

        idx += snprintf( str + idx, maxlen - idx, " %02x",
                         (unsigned int) buf[i] );
        txt[i % 16] = ( buf[i] > 31 && buf[i] < 127 ) ? buf[i] : '.' ;
    }

    if( len > 0 )
    {
        for( /* i = i */; i % 16 != 0; i++ )
            idx += snprintf( str + idx, maxlen - idx, "   " );

        snprintf( str + idx, maxlen - idx, "  %s\n", txt );
        ssl->f_dbg( ssl->p_dbg, level, str );
    }
}

#if defined(POLARSSL_ECP_C)
void debug_print_ecp( const ssl_context *ssl, int level,
                      const char *file, int line,
                      const char *text, const ecp_point *X )
{
    char str[512];
    int maxlen = sizeof( str ) - 1;

    if( ssl->f_dbg == NULL || level > debug_threshold )
        return;

    snprintf( str, maxlen, "%s(X)", text );
    str[maxlen] = '\0';
    debug_print_mpi( ssl, level, file, line, str, &X->X );

    snprintf( str, maxlen, "%s(Y)", text );
    str[maxlen] = '\0';
    debug_print_mpi( ssl, level, file, line, str, &X->Y );
}
#endif /* POLARSSL_ECP_C */

#if defined(POLARSSL_BIGNUM_C)
void debug_print_mpi( const ssl_context *ssl, int level,
                      const char *file, int line,
                      const char *text, const mpi *X )
{
    char str[512];
    int j, k, maxlen = sizeof( str ) - 1, zeros = 1;
    size_t i, n, idx = 0;

    if( ssl->f_dbg == NULL || X == NULL || level > debug_threshold )
        return;

    for( n = X->n - 1; n > 0; n-- )
        if( X->p[n] != 0 )
            break;

    for( j = ( sizeof(t_uint) << 3 ) - 1; j >= 0; j-- )
        if( ( ( X->p[n] >> j ) & 1 ) != 0 )
            break;

    if( debug_log_mode == POLARSSL_DEBUG_LOG_FULL )
        idx = snprintf( str, maxlen, "%s(%04d): ", file, line );

    snprintf( str + idx, maxlen - idx, "value of '%s' (%d bits) is:\n",
              text, (int) ( ( n * ( sizeof(t_uint) << 3 ) ) + j + 1 ) );

    str[maxlen] = '\0';
    ssl->f_dbg( ssl->p_dbg, level, str );

    idx = 0;
    for( i = n + 1, j = 0; i > 0; i-- )
    {
        if( zeros && X->p[i - 1] == 0 )
            continue;

        for( k = sizeof( t_uint ) - 1; k >= 0; k-- )
        {
            if( zeros && ( ( X->p[i - 1] >> ( k << 3 ) ) & 0xFF ) == 0 )
                continue;
            else
                zeros = 0;

            if( j % 16 == 0 )
            {
                if( j > 0 )
                {
                    snprintf( str + idx, maxlen - idx, "\n" );
                    ssl->f_dbg( ssl->p_dbg, level, str );
                    idx = 0;
                }

                if( debug_log_mode == POLARSSL_DEBUG_LOG_FULL )
                    idx = snprintf( str, maxlen, "%s(%04d): ", file, line );
            }

            idx += snprintf( str + idx, maxlen - idx, " %02x", (unsigned int)
                             ( X->p[i - 1] >> ( k << 3 ) ) & 0xFF );

            j++;
        }

    }

    if( zeros == 1 )
    {
        if( debug_log_mode == POLARSSL_DEBUG_LOG_FULL )
        {
            idx = snprintf( str, maxlen, "%s(%04d): ", file, line );

        }
        idx += snprintf( str + idx, maxlen - idx, " 00" );
    }

    snprintf( str + idx, maxlen - idx, "\n" );
    ssl->f_dbg( ssl->p_dbg, level, str );
}
#endif /* POLARSSL_BIGNUM_C */

#if defined(POLARSSL_X509_CRT_PARSE_C)
static void debug_print_pk( const ssl_context *ssl, int level,
                            const char *file, int line,
                            const char *text, const pk_context *pk )
{
    size_t i;
    pk_debug_item items[POLARSSL_PK_DEBUG_MAX_ITEMS];
    char name[16];

    memset( items, 0, sizeof( items ) );

    if( pk_debug( pk, items ) != 0 )
    {
        debug_print_msg( ssl, level, file, line, "invalid PK context" );
        return;
    }

    for( i = 0; i < POLARSSL_PK_DEBUG_MAX_ITEMS; i++ )
    {
        if( items[i].type == POLARSSL_PK_DEBUG_NONE )
            return;

        snprintf( name, sizeof( name ), "%s%s", text, items[i].name );
        name[sizeof( name ) - 1] = '\0';

        if( items[i].type == POLARSSL_PK_DEBUG_MPI )
            debug_print_mpi( ssl, level, file, line, name, items[i].value );
        else
#if defined(POLARSSL_ECP_C)
        if( items[i].type == POLARSSL_PK_DEBUG_ECP )
            debug_print_ecp( ssl, level, file, line, name, items[i].value );
        else
#endif
            debug_print_msg( ssl, level, file, line, "should not happen" );
    }
}

void debug_print_crt( const ssl_context *ssl, int level,
                      const char *file, int line,
                      const char *text, const x509_crt *crt )
{
    char str[1024], prefix[64];
    int i = 0, maxlen = sizeof( prefix ) - 1, idx = 0;

    if( ssl->f_dbg == NULL || crt == NULL || level > debug_threshold )
        return;

    if( debug_log_mode == POLARSSL_DEBUG_LOG_FULL )
    {
        snprintf( prefix, maxlen, "%s(%04d): ", file, line );
        prefix[maxlen] = '\0';
    }
    else
        prefix[0] = '\0';

    maxlen = sizeof( str ) - 1;

    while( crt != NULL )
    {
        char buf[1024];
        x509_crt_info( buf, sizeof( buf ) - 1, prefix, crt );

        if( debug_log_mode == POLARSSL_DEBUG_LOG_FULL )
            idx = snprintf( str, maxlen, "%s(%04d): ", file, line );

        snprintf( str + idx, maxlen - idx, "%s #%d:\n%s",
                  text, ++i, buf );

        str[maxlen] = '\0';
        ssl->f_dbg( ssl->p_dbg, level, str );

        debug_print_pk( ssl, level, file, line, "crt->", &crt->pk );

        crt = crt->next;
    }
}
#endif /* POLARSSL_X509_CRT_PARSE_C */

#endif /* POLARSSL_DEBUG_C */
