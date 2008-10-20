/*
 * Management of the debugging channels
 *
 * Copyright 2000 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "wine/config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <excpt.h>

#define WIN32_NO_STATUS
#include "wine/debug.h"
#include "wine/library.h"

#include <rtlfuncs.h>
#include <cmfuncs.h>

ULONG
__cdecl
DbgPrint(
    IN PCCH  Format,
    IN ...
);

static const char * const debug_classes[] = { "fixme", "err", "warn", "trace" };

#define MAX_DEBUG_OPTIONS 256

static unsigned char default_flags = (1 << __WINE_DBCL_ERR) | (1 << __WINE_DBCL_FIXME);
static int nb_debug_options = -1;
static struct __wine_debug_channel debug_options[MAX_DEBUG_OPTIONS];

static struct __wine_debug_functions funcs;

static void debug_init(void);

static int __cdecl cmp_name( const void *p1, const void *p2 )
{
    const char *name = p1;
    const struct __wine_debug_channel *chan = p2;
    return strcmp( name, chan->name );
}

/* get the flags to use for a given channel, possibly setting them too in case of lazy init */
unsigned char __wine_dbg_get_channel_flags( struct __wine_debug_channel *channel )
{
    if (nb_debug_options == -1) debug_init();

    if (nb_debug_options)
    {
        struct __wine_debug_channel *opt = bsearch( channel->name, debug_options, nb_debug_options,
                                                    sizeof(debug_options[0]), cmp_name );
        if (opt) return opt->flags;
    }
    /* no option for this channel */
    if (channel->flags & (1 << __WINE_DBCL_INIT)) channel->flags = default_flags;
    return default_flags;
}

/* set the flags to use for a given channel; return 0 if the channel is not available to set */
int __wine_dbg_set_channel_flags( struct __wine_debug_channel *channel,
                                  unsigned char set, unsigned char clear )
{
    if (nb_debug_options == -1) debug_init();

    if (nb_debug_options)
    {
        struct __wine_debug_channel *opt = bsearch( channel->name, debug_options, nb_debug_options,
                                                    sizeof(debug_options[0]), cmp_name );
        if (opt)
        {
            opt->flags = (opt->flags & ~clear) | set;
            return 1;
        }
    }
    return 0;
}

/* add a new debug option at the end of the option list */
static void add_option( const char *name, unsigned char set, unsigned char clear )
{
    int min = 0, max = nb_debug_options - 1, pos, res;

    if (!name[0])  /* "all" option */
    {
        default_flags = (default_flags & ~clear) | set;
        return;
    }
    if (strlen(name) >= sizeof(debug_options[0].name)) return;

    while (min <= max)
    {
        pos = (min + max) / 2;
        res = strcmp( name, debug_options[pos].name );
        if (!res)
        {
            debug_options[pos].flags = (debug_options[pos].flags & ~clear) | set;
            return;
        }
        if (res < 0) max = pos - 1;
        else min = pos + 1;
    }
    if (nb_debug_options >= MAX_DEBUG_OPTIONS) return;

    pos = min;
    if (pos < nb_debug_options) memmove( &debug_options[pos + 1], &debug_options[pos],
                                         (nb_debug_options - pos) * sizeof(debug_options[0]) );
    strcpy( debug_options[pos].name, name );
    debug_options[pos].flags = (default_flags & ~clear) | set;
    nb_debug_options++;
}

/* parse a set of debugging option specifications and add them to the option list */
static void parse_options( const char *str )
{
    char *opt, *next, *options;
    unsigned int i;

    if (!(options = _strdup(str))) return;
    for (opt = options; opt; opt = next)
    {
        const char *p;
        unsigned char set = 0, clear = 0;

        if ((next = strchr( opt, ',' ))) *next++ = 0;

        p = opt + strcspn( opt, "+-" );
        if (!p[0]) p = opt;  /* assume it's a debug channel name */

        if (p > opt)
        {
            for (i = 0; i < sizeof(debug_classes)/sizeof(debug_classes[0]); i++)
            {
                int len = strlen(debug_classes[i]);
                if (len != (p - opt)) continue;
                if (!memcmp( opt, debug_classes[i], len ))  /* found it */
                {
                    if (*p == '+') set |= 1 << i;
                    else clear |= 1 << i;
                    break;
                }
            }
            if (i == sizeof(debug_classes)/sizeof(debug_classes[0])) /* bad class name, skip it */
                continue;
        }
        else
        {
            if (*p == '-') clear = ~0;
            else set = ~0;
        }
        if (*p == '+' || *p == '-') p++;
        if (!p[0]) continue;

        if (!strcmp( p, "all" ))
            default_flags = (default_flags & ~clear) | set;
        else
            add_option( p, set, clear );
    }
    free( options );
}

/* initialize all options at startup */
static void debug_init(void)
{
    char *wine_debug;
    DWORD dwLength;
    /* GetEnvironmentVariableA will change LastError! */
    DWORD LastError = GetLastError(); 

    if (nb_debug_options != -1) return;  /* already initialized */
    nb_debug_options = 0;

    dwLength = GetEnvironmentVariableA("DEBUGCHANNEL", NULL, 0);
    if (dwLength)
    {
        wine_debug = malloc(dwLength);
        if (wine_debug)
        {
            if (GetEnvironmentVariableA("DEBUGCHANNEL", wine_debug, dwLength) < dwLength)
                parse_options(wine_debug);
            free(wine_debug);
        }
    }
    SetLastError(LastError);
}

/* varargs wrapper for funcs.dbg_vprintf */
int wine_dbg_printf( const char *format, ... )
{
    int ret;
    va_list valist;

    va_start(valist, format);
    ret = funcs.dbg_vprintf( format, valist );
    va_end(valist);
    return ret;
}

/* printf with temp buffer allocation */
const char *wine_dbg_sprintf( const char *format, ... )
{
    static const int max_size = 200;
    char *ret;
    int len;
    va_list valist;

    va_start(valist, format);
    ret = funcs.get_temp_buffer( max_size );
    len = vsnprintf( ret, max_size, format, valist );
    if (len == -1 || len >= max_size) ret[max_size-1] = 0;
    else funcs.release_temp_buffer( ret, len + 1 );
    va_end(valist);
    return ret;
}


/* varargs wrapper for funcs.dbg_vlog */
int wine_dbg_log( enum __wine_debug_class cls, struct __wine_debug_channel *channel,
                  const char *file, const char *func, const int line, const char *format, ... )
{
    int ret;
    va_list valist;

    if (!(__wine_dbg_get_channel_flags( channel ) & (1 << cls))) return -1;

    va_start(valist, format);
    ret = funcs.dbg_vlog( cls, channel, file, func, line, format, valist );
    va_end(valist);
    return ret;
}


/* allocate some tmp string space */
/* FIXME: this is not 100% thread-safe */
static char *get_temp_buffer( size_t size )
{
    static char *list[32];
    static long pos;
    char *ret;
    int idx;

    idx = InterlockedExchangeAdd( &pos, 1 ) % (sizeof(list)/sizeof(list[0]));
    if ((ret = realloc( list[idx], size ))) list[idx] = ret;
    return ret;
}


/* release unused part of the buffer */
static void release_temp_buffer( char *buffer, size_t size )
{
    /* don't bother doing anything */
}


/* default implementation of wine_dbgstr_an */
static const char *default_dbgstr_an( const char *str, int n )
{
    static const char hex[16] = "0123456789abcdef";
    char *dst, *res;
    size_t size;

    if (!((ULONG_PTR)str >> 16))
    {
        if (!str) return "(null)";
        res = funcs.get_temp_buffer( 6 );
        sprintf( res, "#%04x", LOWORD(str) );
        return res;
    }
    if (n == -1) n = strlen(str);
    if (n < 0) n = 0;
    size = 10 + min( 300, n * 4 );
    dst = res = funcs.get_temp_buffer( size );
    *dst++ = '"';
    while (n-- > 0 && dst <= res + size - 9)
    {
        unsigned char c = *str++;
        switch (c)
        {
        case '\n': *dst++ = '\\'; *dst++ = 'n'; break;
        case '\r': *dst++ = '\\'; *dst++ = 'r'; break;
        case '\t': *dst++ = '\\'; *dst++ = 't'; break;
        case '"':  *dst++ = '\\'; *dst++ = '"'; break;
        case '\\': *dst++ = '\\'; *dst++ = '\\'; break;
        default:
            if (c >= ' ' && c <= 126)
                *dst++ = c;
            else
            {
                *dst++ = '\\';
                *dst++ = 'x';
                *dst++ = hex[(c >> 4) & 0x0f];
                *dst++ = hex[c & 0x0f];
            }
        }
    }
    *dst++ = '"';
    if (n > 0)
    {
        *dst++ = '.';
        *dst++ = '.';
        *dst++ = '.';
    }
    *dst++ = 0;
    funcs.release_temp_buffer( res, dst - res );
    return res;
}


/* default implementation of wine_dbgstr_wn */
static const char *default_dbgstr_wn( const WCHAR *str, int n )
{
    char *dst, *res;
    size_t size;

    if (!((ULONG_PTR)str >> 16))
    {
        if (!str) return "(null)";
        res = funcs.get_temp_buffer( 6 );
        sprintf( res, "#%04x", LOWORD(str) );
        return res;
    }
    if (n == -1)
    {
        const WCHAR *end = str;
        while (*end) end++;
        n = end - str;
    }
    if (n < 0) n = 0;
    size = 12 + min( 300, n * 5 );
    dst = res = funcs.get_temp_buffer( n * 5 + 7 );
    *dst++ = 'L';
    *dst++ = '"';
    while (n-- > 0 && dst <= res + size - 10)
    {
        WCHAR c = *str++;
        switch (c)
        {
        case '\n': *dst++ = '\\'; *dst++ = 'n'; break;
        case '\r': *dst++ = '\\'; *dst++ = 'r'; break;
        case '\t': *dst++ = '\\'; *dst++ = 't'; break;
        case '"':  *dst++ = '\\'; *dst++ = '"'; break;
        case '\\': *dst++ = '\\'; *dst++ = '\\'; break;
        default:
            if (c >= ' ' && c <= 126)
                *dst++ = c;
            else
            {
                *dst++ = '\\';
                sprintf(dst,"%04x",c);
                dst+=4;
            }
        }
    }
    *dst++ = '"';
    if (n > 0)
    {
        *dst++ = '.';
        *dst++ = '.';
        *dst++ = '.';
    }
    *dst++ = 0;
    funcs.release_temp_buffer( res, dst - res );
    return res;
}


/* default implementation of wine_dbg_vprintf */
static int default_dbg_vprintf( const char *format, va_list args )
{
    char buffer[512];
    vsnprintf( buffer, sizeof(buffer), format, args );
    buffer[sizeof(buffer) - 1] = '\0';
    return DbgPrint( "%s", buffer );
}


/* default implementation of wine_dbg_vlog */
static int default_dbg_vlog( enum __wine_debug_class cls, struct __wine_debug_channel *channel,
                             const char *file, const char *func, const int line, const char *format, va_list args )
{
    int ret = 0;

    if (cls < sizeof(debug_classes)/sizeof(debug_classes[0]))
        ret += wine_dbg_printf( "%s:", debug_classes[cls] );
    ret += wine_dbg_printf ( "(%s:%d) ", file, line );
    if (format)
        ret += funcs.dbg_vprintf( format, args );
    return ret;
}

/* wrappers to use the function pointers */

const char *wine_dbgstr_an( const char * s, int n )
{
    return funcs.dbgstr_an(s, n);
}

const char *wine_dbgstr_wn( const WCHAR *s, int n )
{
    return funcs.dbgstr_wn(s, n);
}

void __wine_dbg_set_functions( const struct __wine_debug_functions *new_funcs,
                               struct __wine_debug_functions *old_funcs, size_t size )
{
    if (old_funcs) memcpy( old_funcs, &funcs, min(sizeof(funcs),size) );
    if (new_funcs) memcpy( &funcs, new_funcs, min(sizeof(funcs),size) );
}

static struct __wine_debug_functions funcs =
{
    get_temp_buffer,
    release_temp_buffer,
    default_dbgstr_an,
    default_dbgstr_wn,
    default_dbg_vprintf,
    default_dbg_vlog
};
