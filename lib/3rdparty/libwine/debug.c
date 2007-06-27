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

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include <wine/config.h>
#include <wine/port.h>
#include <wine/debug.h>

/* ---------------------------------------------------------------------- */

static CRITICAL_SECTION WineDebugCS;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &WineDebugCS,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
    0, 0, { 0, 0 }
};
static CRITICAL_SECTION WineDebugCS = { &critsect_debug, -1, 0, 0, 0, 0 };
static DWORD WineDebugTlsIndex = TLS_OUT_OF_INDEXES;

/* ---------------------------------------------------------------------- */

struct debug_info
{
    char *str_pos;       /* current position in strings buffer */
    char *out_pos;       /* current position in output buffer */
    char  strings[1024]; /* buffer for temporary strings */
    char  output[1024];  /* current output line */
};

static struct debug_info tmp = { tmp.strings, tmp.output };

/* get the debug info pointer for the current thread */
static inline struct debug_info *get_info(void)
{
    struct debug_info *info;

    if (WineDebugTlsIndex == TLS_OUT_OF_INDEXES)
    {
        EnterCriticalSection(&WineDebugCS);
        if (WineDebugTlsIndex == TLS_OUT_OF_INDEXES)
        {
            DWORD NewTlsIndex = TlsAlloc();
            if (NewTlsIndex == TLS_OUT_OF_INDEXES)
            {
               LeaveCriticalSection(&WineDebugCS);
               return &tmp;
            }
            info = HeapAlloc(GetProcessHeap(), 0, sizeof(*info));
            if (!info)
            {
               LeaveCriticalSection(&WineDebugCS);
               TlsFree(NewTlsIndex);
               return &tmp;
            }
            info->str_pos = info->strings;
            info->out_pos = info->output;
            TlsSetValue(NewTlsIndex, info);
            WineDebugTlsIndex = NewTlsIndex;
        }
        LeaveCriticalSection(&WineDebugCS);
    }

    return TlsGetValue(WineDebugTlsIndex);
}

/* allocate some tmp space for a string */
static void *gimme1(int n)
{
    struct debug_info *info = get_info();
    char *res = info->str_pos;

    if (res + n >= &info->strings[sizeof(info->strings)]) res = info->strings;
    info->str_pos = res + n;
    return res;
}

/* release extra space that we requested in gimme1() */
static inline void release(void *ptr)
{
    struct debug_info *info;
    if (WineDebugTlsIndex == TLS_OUT_OF_INDEXES)
        info = &tmp;
    else
        info = TlsGetValue(WineDebugTlsIndex);
    info->str_pos = ptr;
}

/***********************************************************************
 *              wine_dbgstr_an
 */
const char *wine_dbgstr_an(const char *src, int n)
{
    char *dst, *res;

    if (!HIWORD(src))
    {
        if (!src) return "(null)";
        res = gimme1(6);
        sprintf(res, "#%04x", (WORD)(DWORD)(src) );
        return res;
    }
    if (n < 0) n = 0;
    else if (n > 200) n = 200;
    dst = res = gimme1 (n * 4 + 6);
    *dst++ = '"';
    while (n-- > 0 && *src)
    {
        unsigned char c = *src++;
        switch (c)
        {
        case '\n': *dst++ = '\\'; *dst++ = 'n'; break;
        case '\r': *dst++ = '\\'; *dst++ = 'r'; break;
        case '\t': *dst++ = '\\'; *dst++ = 't'; break;
        case '"': *dst++ = '\\'; *dst++ = '"'; break;
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
    if (*src)
    {
        *dst++ = '.';
        *dst++ = '.';
        *dst++ = '.';
    }
    *dst++ = '\0';
    release( dst );
    return res;
}

/***********************************************************************
 *              wine_dbgstr_wn
 */
const char *wine_dbgstr_wn(const WCHAR *src, int n)
{
    char *dst, *res;

    if (!HIWORD(src))
    {
        if (!src) return "(null)";
        res = gimme1(6);
        sprintf(res, "#%04x", (WORD)(DWORD)(src) );
        return res;
    }
    if (n < 0) n = 0;
    else if (n > 200) n = 200;
    dst = res = gimme1(n * 5 + 7);
    *dst++ = 'L';
    *dst++ = '"';
    while (n-- > 0 && *src)
    {
        WCHAR c = *src++;
        switch (c)
        {
        case '\n': *dst++ = '\\'; *dst++ = 'n'; break;
        case '\r': *dst++ = '\\'; *dst++ = 'r'; break;
        case '\t': *dst++ = '\\'; *dst++ = 't'; break;
        case '"': *dst++ = '\\'; *dst++ = '"'; break;
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
    if (*src)
    {
        *dst++ = '.';
        *dst++ = '.';
        *dst++ = '.';
    }
    *dst++ = '\0';
    release(dst);
    return res;
}

const char *wine_dbgstr_longlong( unsigned long long ll )
{
    if (ll >> 32) return wine_dbg_sprintf( "%lx%08lx", (unsigned long)(ll >> 32), (unsigned long)ll );
    else return wine_dbg_sprintf( "%lx", (unsigned long)ll );
}

/* varargs wrapper for __wine_dbg_vsprintf */
const char *wine_dbg_sprintf( const char *format, ... )
{
    char* buffer = gimme1(1024);
    va_list ap;

    va_start(ap, format);
    release(buffer+vsnprintf(buffer, 1024, format, ap));
    va_end(ap);

    return buffer;
}

const char *wine_dbgstr_w( const WCHAR *s )
{
    return wine_dbgstr_wn( s, -1 );
}
