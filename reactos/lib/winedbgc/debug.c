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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <ntddk.h>
#include <wine/debugtools.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "porting.h"
/*
#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "wine/debug.h"
#include "wine/library.h"
#include "wine/unicode.h"
 */
struct dll
{
    struct dll   *next;        /* linked list of dlls */
    struct dll   *prev;
    char * const *channels;    /* array of channels */
    int           nb_channels; /* number of channels in array */
};

static struct dll *first_dll;

struct debug_option
{
    struct debug_option *next;       /* next option in list */
    unsigned char        set;        /* bits to set */
    unsigned char        clear;      /* bits to clear */
    char                 name[14];   /* channel name, or empty for "all" */
};

static struct debug_option *first_option;
static struct debug_option *last_option;

static const char * const debug_classes[] = { "fixme", "err", "warn", "trace" };

static int cmp_name( const void *p1, const void *p2 )
{
    const char *name = p1;
    const char * const *chan = p2;
    return strcmp( name, *chan + 1 );
}

/* apply a debug option to the channels of a given dll */
static void apply_option( struct dll *dll, const struct debug_option *opt )
{
    if (opt->name[0])
    {
        char **dbch = bsearch( opt->name, dll->channels, dll->nb_channels,
                               sizeof(*dll->channels), cmp_name );
        if (dbch) **dbch = (**dbch & ~opt->clear) | opt->set;
    }
    else /* all */
    {
        int i;
        for (i = 0; i < dll->nb_channels; i++)
            dll->channels[i][0] = (dll->channels[i][0] & ~opt->clear) | opt->set;
    }
}

/* register a new set of channels for a dll */
void *__wine_dbg_register( char * const *channels, int nb )
{
    struct debug_option *opt = first_option;
    struct dll *dll = malloc( sizeof(*dll) );
    if (dll)
    {
        dll->channels = channels;
        dll->nb_channels = nb;
        dll->prev = NULL;
        if ((dll->next = first_dll)) dll->next->prev = dll;
        first_dll = dll;

        /* apply existing options to this dll */
        while (opt)
        {
            apply_option( dll, opt );
            opt = opt->next;
        }
    }
    return dll;
}


/* unregister a set of channels; must pass the pointer obtained from wine_dbg_register */
void __wine_dbg_unregister( void *channel )
{
    struct dll *dll = channel;
    if (dll)
    {
        if (dll->next) dll->next->prev = dll->prev;
        if (dll->prev) dll->prev->next = dll->next;
        else first_dll = dll->next;
        free( dll );
    }
}


/* add a new debug option at the end of the option list */
void wine_dbg_add_option( const char *name, unsigned char set, unsigned char clear )
{
    struct dll *dll = first_dll;
    struct debug_option *opt;

    if (!(opt = malloc( sizeof(*opt) ))) return;
    opt->next  = NULL;
    opt->set   = set;
    opt->clear = clear;
    strncpy( opt->name, name, sizeof(opt->name) );
    opt->name[sizeof(opt->name)-1] = 0;
    if (last_option) last_option->next = opt;
    else first_option = opt;
    last_option = opt;

    /* apply option to all existing dlls */
    while (dll)
    {
        apply_option( dll, opt );
        dll = dll->next;
    }
}

/* parse a set of debugging option specifications and add them to the option list */
int wine_dbg_parse_options( const char *str )
{
    char *p, *opt, *next, *options;
    int i, errors = 0;

    if (!(options = strdup(str))) return -1;
    for (opt = options; opt; opt = next)
    {
        unsigned char set = 0, clear = 0;

        if ((next = strchr( opt, ',' ))) *next++ = 0;

        p = opt + strcspn( opt, "+-" );
        if (!p[0] || !p[1])  /* bad option, skip it */
        {
            errors++;
            continue;
        }

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
            {
                errors++;
                continue;
            }
        }
        else
        {
            if (*p == '+') set = ~0;
            else clear = ~0;
        }
        p++;
        if (!strcmp( p, "all" )) p = "";  /* empty string means all */
        wine_dbg_add_option( p, set, clear );
    }
    free( options );
    return errors;
}

#ifdef __WINE__

/* varargs wrapper for __wine_dbg_vprintf */
int wine_dbg_printf( const char *format, ... )
{
    int ret;
    va_list valist;

    va_start(valist, format);
    ret = __wine_dbg_vprintf( format, valist );
    va_end(valist);
    return ret;
}


/* varargs wrapper for __wine_dbg_vlog */
int wine_dbg_log( int cls, const char *channel, const char *func, const char *format, ... )
{
    int ret;
    va_list valist;

    va_start(valist, format);
    ret = __wine_dbg_vlog( cls, channel, func, format, valist );
    va_end(valist);
    return ret;
}

#endif /*__WINE__*/

/* allocate some tmp string space */
/* FIXME: this is not 100% thread-safe */
static char *get_tmp_space( int size )
{
    static char *list[32];
    static long pos;
    char *ret;
    int idx;

    idx = interlocked_xchg_add( &pos, 1 ) % (sizeof(list)/sizeof(list[0]));
    if ((ret = realloc( list[idx], size ))) list[idx] = ret;
    return ret;
}


/* default implementation of wine_dbgstr_an */
static const char *default_dbgstr_an( const char *str, int n )
{
    char *dst, *res;

    if (!HIWORD(str))
    {
        if (!str) return "(null)";
        res = get_tmp_space( 6 );
        sprintf( res, "#%04x", LOWORD(str) );
        return res;
    }
    if (n == -1) n = strlen(str);
    if (n < 0) n = 0;
    else if (n > 200) n = 200;
    dst = res = get_tmp_space( n * 4 + 6 );
    *dst++ = '"';
    while (n-- > 0)
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
                *dst++ = '0' + ((c >> 6) & 7);
                *dst++ = '0' + ((c >> 3) & 7);
                *dst++ = '0' + ((c >> 0) & 7);
            }
        }
    }
    *dst++ = '"';
    if (*str)
    {
        *dst++ = '.';
        *dst++ = '.';
        *dst++ = '.';
    }
    *dst = 0;
    return res;
}


/* default implementation of wine_dbgstr_wn */
static const char *default_dbgstr_wn( const WCHAR *str, int n )
{
    char *dst, *res;

    if (!HIWORD(str))
    {
        if (!str) return "(null)";
        res = get_tmp_space( 6 );
        sprintf( res, "#%04x", LOWORD(str) );
        return res;
    }
    if (n == -1) n = strlenW(str);
    if (n < 0) n = 0;
    else if (n > 200) n = 200;
    dst = res = get_tmp_space( n * 5 + 7 );
    *dst++ = 'L';
    *dst++ = '"';
    while (n-- > 0)
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
    if (*str)
    {
        *dst++ = '.';
        *dst++ = '.';
        *dst++ = '.';
    }
    *dst = 0;
    return res;
}


/* default implementation of wine_dbgstr_guid */
static const char *default_dbgstr_guid( const struct _GUID *id )
{
    char *str;

    if (!id) return "(null)";
    if (!((int)id >> 16))
    {
        str = get_tmp_space( 12 );
        sprintf( str, "<guid-0x%04x>", (int)id & 0xffff );
    }
    else
    {
        str = get_tmp_space( 40 );
        sprintf( str, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                 id->Data1, id->Data2, id->Data3,
                 id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
                 id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7] );
    }
    return str;
}


/* default implementation of wine_dbg_vprintf */
static int default_dbg_vprintf( const char *format, va_list args )
{
    return vfprintf( stderr, format, args );
}


/* default implementation of wine_dbg_vlog */
static int default_dbg_vlog( int cls, const char *channel, const char *func,
                             const char *format, va_list args )
{
    int ret = 0;

    if (cls < sizeof(debug_classes)/sizeof(debug_classes[0]))
        ret += wine_dbg_printf( "%s:%s:%s ", debug_classes[cls], channel + 1, func );
    if (format)
        ret += __wine_dbg_vprintf( format, args );
    return ret;
}


/* exported function pointers so that debugging functions can be redirected at run-time */

const char * (*__wine_dbgstr_an)( const char * s, int n ) = default_dbgstr_an;
const char * (*__wine_dbgstr_wn)( const WCHAR *s, int n ) = default_dbgstr_wn;
const char * (*__wine_dbgstr_guid)( const struct _GUID *id ) = default_dbgstr_guid;
int (*__wine_dbg_vprintf)( const char *format, va_list args ) = default_dbg_vprintf;
int (*__wine_dbg_vlog)( int cls, const char *channel, const char *function,
                        const char *format, va_list args ) = default_dbg_vlog;

/* wrappers to use the function pointers */

#ifdef __WINE__

const char *wine_dbgstr_guid( const struct _GUID *id )
{
    return __wine_dbgstr_guid(id);
}

const char *wine_dbgstr_an( const char * s, int n )
{
    return __wine_dbgstr_an(s, n);
}

const char *wine_dbgstr_wn( const WCHAR *s, int n )
{
    return __wine_dbgstr_wn(s, n);
}

const char *wine_dbgstr_a( const char *s )
{
    return __wine_dbgstr_an( s, -1 );
}

const char *wine_dbgstr_w( const WCHAR *s )
{
    return __wine_dbgstr_wn( s, -1 );
}

#endif /*__WINE__*/
