/*
 * wsprintf functions
 *
 * Copyright 1996 Alexandre Julliard
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
 *
 * NOTE:
 * This code is duplicated in shlwapi. If you change something here make sure
 * to change it in shlwapi too.
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS User32
 * PURPOSE:          [w]sprintf functions
 * FILE:             lib/user32/sprintf.c
 * PROGRAMER:        Steven Edwards 
 * REVISION HISTORY: 2003/07/13 Merged from wine user/wsprintf.c
 * NOTES:            Adapted from Wine
 */

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>

#define WPRINTF_LEFTALIGN   0x0001  /* Align output on the left ('-' prefix) */
#define WPRINTF_PREFIX_HEX  0x0002  /* Prefix hex with 0x ('#' prefix) */
#define WPRINTF_ZEROPAD     0x0004  /* Pad with zeros ('0' prefix) */
#define WPRINTF_LONG        0x0008  /* Long arg ('l' prefix) */
#define WPRINTF_SHORT       0x0010  /* Short arg ('h' prefix) */
#define WPRINTF_UPPER_HEX   0x0020  /* Upper-case hex ('X' specifier) */
#define WPRINTF_WIDE        0x0040  /* Wide arg ('w' prefix) */

typedef enum
{
    WPR_UNKNOWN,
    WPR_CHAR,
    WPR_WCHAR,
    WPR_STRING,
    WPR_WSTRING,
    WPR_SIGNED,
    WPR_UNSIGNED,
    WPR_HEXA
} WPRINTF_TYPE;

typedef struct
{
    UINT         flags;
    UINT         width;
    UINT         precision;
    WPRINTF_TYPE   type;
} WPRINTF_FORMAT;

typedef union {
    WCHAR   wchar_view;
    CHAR    char_view;
    LPCSTR  lpcstr_view;
    LPCWSTR lpcwstr_view;
    INT     int_view;
} WPRINTF_DATA;

static const CHAR null_stringA[] = "(null)";
static const WCHAR null_stringW[] = { '(', 'n', 'u', 'l', 'l', ')', 0 };

/***********************************************************************
 *           WPRINTF_ParseFormatA
 *
 * Parse a format specification. A format specification has the form:
 *
 * [-][#][0][width][.precision]type
 *
 * Return value is the length of the format specification in characters.
 */
static INT WPRINTF_ParseFormatA( LPCSTR format, WPRINTF_FORMAT *res )
{
    LPCSTR p = format;

    res->flags = 0;
    res->width = 0;
    res->precision = 0;
    if (*p == '-') { res->flags |= WPRINTF_LEFTALIGN; p++; }
    if (*p == '#') { res->flags |= WPRINTF_PREFIX_HEX; p++; }
    if (*p == '0') { res->flags |= WPRINTF_ZEROPAD; p++; }
    while ((*p >= '0') && (*p <= '9'))  /* width field */
    {
        res->width = res->width * 10 + *p - '0';
        p++;
    }
    if (*p == '.')  /* precision field */
    {
        p++;
        while ((*p >= '0') && (*p <= '9'))
        {
            res->precision = res->precision * 10 + *p - '0';
            p++;
        }
    }
    if (*p == 'l') { res->flags |= WPRINTF_LONG; p++; }
    else if (*p == 'h') { res->flags |= WPRINTF_SHORT; p++; }
    else if (*p == 'w') { res->flags |= WPRINTF_WIDE; p++; }
    switch(*p)
    {
    case 'c':
        res->type = (res->flags & WPRINTF_LONG) ? WPR_WCHAR : WPR_CHAR;
        break;
    case 'C':
        res->type = (res->flags & WPRINTF_SHORT) ? WPR_CHAR : WPR_WCHAR;
        break;
    case 'd':
    case 'i':
        res->type = WPR_SIGNED;
        break;
    case 's':
        res->type = (res->flags & (WPRINTF_LONG |WPRINTF_WIDE)) ? WPR_WSTRING : WPR_STRING;
        break;
    case 'S':
        res->type = (res->flags & (WPRINTF_SHORT|WPRINTF_WIDE)) ? WPR_STRING : WPR_WSTRING;
        break;
    case 'u':
        res->type = WPR_UNSIGNED;
        break;
    case 'X':
        res->flags |= WPRINTF_UPPER_HEX;
        /* fall through */
    case 'x':
        res->type = WPR_HEXA;
        break;
    default: /* unknown format char */
        res->type = WPR_UNKNOWN;
        p--;  /* print format as normal char */
        break;
    }
    return (INT)(p - format) + 1;
}


/***********************************************************************
 *           WPRINTF_ParseFormatW
 *
 * Parse a format specification. A format specification has the form:
 *
 * [-][#][0][width][.precision]type
 *
 * Return value is the length of the format specification in characters.
 */
static INT WPRINTF_ParseFormatW( LPCWSTR format, WPRINTF_FORMAT *res )
{
    LPCWSTR p = format;

    res->flags = 0;
    res->width = 0;
    res->precision = 0;
    if (*p == '-') { res->flags |= WPRINTF_LEFTALIGN; p++; }
    if (*p == '#') { res->flags |= WPRINTF_PREFIX_HEX; p++; }
    if (*p == '0') { res->flags |= WPRINTF_ZEROPAD; p++; }
    while ((*p >= '0') && (*p <= '9'))  /* width field */
    {
        res->width = res->width * 10 + *p - '0';
        p++;
    }
    if (*p == '.')  /* precision field */
    {
        p++;
        while ((*p >= '0') && (*p <= '9'))
        {
            res->precision = res->precision * 10 + *p - '0';
            p++;
        }
    }
    if (*p == 'l') { res->flags |= WPRINTF_LONG; p++; }
    else if (*p == 'h') { res->flags |= WPRINTF_SHORT; p++; }
    else if (*p == 'w') { res->flags |= WPRINTF_WIDE; p++; }
    switch((CHAR)*p)
    {
    case 'c':
        res->type = (res->flags & WPRINTF_SHORT) ? WPR_CHAR : WPR_WCHAR;
        break;
    case 'C':
        res->type = (res->flags & WPRINTF_LONG) ? WPR_WCHAR : WPR_CHAR;
        break;
    case 'd':
    case 'i':
        res->type = WPR_SIGNED;
        break;
    case 's':
        res->type = ((res->flags & WPRINTF_SHORT) && !(res->flags & WPRINTF_WIDE)) ? WPR_STRING : WPR_WSTRING;
        break;
    case 'S':
        res->type = (res->flags & (WPRINTF_LONG|WPRINTF_WIDE)) ? WPR_WSTRING : WPR_STRING;
        break;
    case 'u':
        res->type = WPR_UNSIGNED;
        break;
    case 'X':
        res->flags |= WPRINTF_UPPER_HEX;
        /* fall through */
    case 'x':
        res->type = WPR_HEXA;
        break;
    default:
        res->type = WPR_UNKNOWN;
        p--;  /* print format as normal char */
        break;
    }
    return (INT)(p - format) + 1;
}


/***********************************************************************
 *           WPRINTF_GetLen
 */
static UINT WPRINTF_GetLen( WPRINTF_FORMAT *format, WPRINTF_DATA *arg,
                              LPSTR number, UINT maxlen )
{
    UINT len;

    if (format->flags & WPRINTF_LEFTALIGN) format->flags &= ~WPRINTF_ZEROPAD;
    if (format->width > maxlen) format->width = maxlen;
    switch(format->type)
    {
    case WPR_CHAR:
    case WPR_WCHAR:
        return (format->precision = 1);
    case WPR_STRING:
        if (!arg->lpcstr_view) arg->lpcstr_view = null_stringA;
        for (len = 0; !format->precision || (len < format->precision); len++)
            if (!*(arg->lpcstr_view + len)) break;
        if (len > maxlen) len = maxlen;
        return (format->precision = len);
    case WPR_WSTRING:
        if (!arg->lpcwstr_view) arg->lpcwstr_view = null_stringW;
        for (len = 0; !format->precision || (len < format->precision); len++)
            if (!*(arg->lpcwstr_view + len)) break;
        if (len > maxlen) len = maxlen;
        return (format->precision = len);
    case WPR_SIGNED:
        len = sprintf( number, "%d", arg->int_view );
        break;
    case WPR_UNSIGNED:
        len = sprintf( number, "%u", (UINT)arg->int_view );
        break;
    case WPR_HEXA:
        len = sprintf( number,
                       (format->flags & WPRINTF_UPPER_HEX) ? "%X" : "%x",
                       (UINT)arg->int_view);
        break;
    default:
        return 0;
    }
    if (len > maxlen) len = maxlen;
    if (format->precision < len) format->precision = len;
    if (format->precision > maxlen) format->precision = maxlen;
    if ((format->flags & WPRINTF_ZEROPAD) && (format->width > format->precision))
        format->precision = format->width;
    if (format->flags & WPRINTF_PREFIX_HEX) len += 2;
    return len;
}

/***********************************************************************
 *           wvsnprintfA   (internal)
 */
static INT wvsnprintfA( LPSTR buffer, UINT maxlen, LPCSTR spec, va_list args )
{
    WPRINTF_FORMAT format;
    LPSTR p = buffer;
    UINT i, len, sign;
    CHAR number[20];
    WPRINTF_DATA argData;

    //TRACE("%p %u %s\n", buffer, maxlen, debugstr_a(spec));

    while (*spec && (maxlen > 1))
    {
        if (*spec != '%') { *p++ = *spec++; maxlen--; continue; }
        spec++;
        if (*spec == '%') { *p++ = *spec++; maxlen--; continue; }
        spec += WPRINTF_ParseFormatA( spec, &format );

        switch(format.type)
        {
        case WPR_WCHAR:
            argData.wchar_view = (WCHAR)va_arg( args, int );
            break;
        case WPR_CHAR:
            argData.char_view = (CHAR)va_arg( args, int );
            break;
        case WPR_STRING:
            argData.lpcstr_view = va_arg( args, LPCSTR );
            break;
        case WPR_WSTRING:
            argData.lpcwstr_view = va_arg( args, LPCWSTR );
            break;
        case WPR_HEXA:
        case WPR_SIGNED:
        case WPR_UNSIGNED:
            argData.int_view = va_arg( args, INT );
            break;
        default:
            argData.wchar_view = 0;
            break;
        }

        len = WPRINTF_GetLen( &format, &argData, number, maxlen - 1 );
        sign = 0;
        if (!(format.flags & WPRINTF_LEFTALIGN))
            for (i = format.precision; i < format.width; i++, maxlen--)
                *p++ = ' ';
        switch(format.type)
        {
        case WPR_WCHAR:
            *p++ = argData.wchar_view;
            break;
        case WPR_CHAR:
            *p++ = argData.char_view;
            break;
        case WPR_STRING:
            memcpy( p, argData.lpcstr_view, len );
            p += len;
            break;
        case WPR_WSTRING:
            {
                LPCWSTR ptr = argData.lpcwstr_view;
                for (i = 0; i < len; i++) *p++ = (CHAR)*ptr++;
            }
            break;
        case WPR_HEXA:
            if ((format.flags & WPRINTF_PREFIX_HEX) && (maxlen > 3))
            {
                *p++ = '0';
                *p++ = (format.flags & WPRINTF_UPPER_HEX) ? 'X' : 'x';
                maxlen -= 2;
                len -= 2;
            }
            /* fall through */
        case WPR_SIGNED:
            /* Transfer the sign now, just in case it will be zero-padded*/
            if (number[0] == '-')
            {
                *p++ = '-';
                sign = 1;
            }
            /* fall through */
        case WPR_UNSIGNED:
            for (i = len; i < format.precision; i++, maxlen--) *p++ = '0';
            memcpy( p, number + sign, len - sign  );
            p += len - sign;
            break;
        case WPR_UNKNOWN:
            continue;
        }
        if (format.flags & WPRINTF_LEFTALIGN)
            for (i = format.precision; i < format.width; i++, maxlen--)
                *p++ = ' ';
        maxlen -= len;
    }
    *p = 0;
    //TRACE("%s\n",debugstr_a(buffer));
    return (maxlen > 1) ? (INT)(p - buffer) : -1;
}


/***********************************************************************
 *           wvsnprintfW   (internal)
 */
static INT wvsnprintfW( LPWSTR buffer, UINT maxlen, LPCWSTR spec, va_list args )
{
    WPRINTF_FORMAT format;
    LPWSTR p = buffer;
    UINT i, len, sign;
    CHAR number[20];
    WPRINTF_DATA argData;

    //TRACE("%p %u %s\n", buffer, maxlen, debugstr_w(spec));

    while (*spec && (maxlen > 1))
    {
        if (*spec != '%') { *p++ = *spec++; maxlen--; continue; }
        spec++;
        if (*spec == '%') { *p++ = *spec++; maxlen--; continue; }
        spec += WPRINTF_ParseFormatW( spec, &format );

        switch(format.type)
        {
        case WPR_WCHAR:
            argData.wchar_view = (WCHAR)va_arg( args, int );
            break;
        case WPR_CHAR:
            argData.char_view = (CHAR)va_arg( args, int );
            break;
        case WPR_STRING:
            argData.lpcstr_view = va_arg( args, LPCSTR );
            break;
        case WPR_WSTRING:
            argData.lpcwstr_view = va_arg( args, LPCWSTR );
            break;
        case WPR_HEXA:
        case WPR_SIGNED:
        case WPR_UNSIGNED:
            argData.int_view = va_arg( args, INT );
            break;
        default:
            argData.wchar_view = 0;
            break;
        }

        len = WPRINTF_GetLen( &format, &argData, number, maxlen - 1 );
        sign = 0;
        if (!(format.flags & WPRINTF_LEFTALIGN))
            for (i = format.precision; i < format.width; i++, maxlen--)
                *p++ = ' ';
        switch(format.type)
        {
        case WPR_WCHAR:
            *p++ = argData.wchar_view;
            break;
        case WPR_CHAR:
            *p++ = argData.char_view;
            break;
        case WPR_STRING:
            {
                LPCSTR ptr = argData.lpcstr_view;
                for (i = 0; i < len; i++) *p++ = (WCHAR)*ptr++;
            }
            break;
        case WPR_WSTRING:
            if (len) memcpy( p, argData.lpcwstr_view, len * sizeof(WCHAR) );
            p += len;
            break;
        case WPR_HEXA:
            if ((format.flags & WPRINTF_PREFIX_HEX) && (maxlen > 3))
            {
                *p++ = '0';
                *p++ = (format.flags & WPRINTF_UPPER_HEX) ? 'X' : 'x';
                maxlen -= 2;
                len -= 2;
            }
            /* fall through */
        case WPR_SIGNED:
            /* Transfer the sign now, just in case it will be zero-padded*/
            if (number[0] == '-')
            {
                *p++ = '-';
                sign = 1;
            }
            /* fall through */
        case WPR_UNSIGNED:
            for (i = len; i < format.precision; i++, maxlen--) *p++ = '0';
            for (i = sign; i < len; i++) *p++ = (WCHAR)number[i];
            break;
        case WPR_UNKNOWN:
            continue;
        }
        if (format.flags & WPRINTF_LEFTALIGN)
            for (i = format.precision; i < format.width; i++, maxlen--)
                *p++ = ' ';
        maxlen -= len;
    }
    *p = 0;
    //TRACE("%s\n",debugstr_w(buffer));
    return (maxlen > 1) ? (INT)(p - buffer) : -1;
}

/***********************************************************************
 *           wvsprintfA   (USER32.@)
 * @implemented
 */
INT STDCALL wvsprintfA( LPSTR buffer, LPCSTR spec, va_list args )
{
    INT res = wvsnprintfA( buffer, 1024, spec, args );
    return ( res == -1 ) ? 1024 : res;
}


/***********************************************************************
 *           wvsprintfW   (USER32.@)
 * @implemented
 */
INT STDCALL wvsprintfW( LPWSTR buffer, LPCWSTR spec, va_list args )
{
    INT res = wvsnprintfW( buffer, 1024, spec, args );
    return ( res == -1 ) ? 1024 : res;
}

/***********************************************************************
 *           wsprintfA   (USER32.@)
 * @implemented
 */
INT CDECL wsprintfA( LPSTR buffer, LPCSTR spec, ... )
{
    va_list valist;
    INT res;

    va_start( valist, spec );
    res = wvsnprintfA( buffer, 1024, spec, valist );
    va_end( valist );
    return ( res == -1 ) ? 1024 : res;
}

/***********************************************************************
 *           wsprintfW   (USER32.@)
 * @implemented
 */
INT CDECL wsprintfW( LPWSTR buffer, LPCWSTR spec, ... )
{
    va_list valist;
    INT res;

    va_start( valist, spec );
    res = wvsnprintfW( buffer, 1024, spec, valist );
    va_end( valist );
    return ( res == -1 ) ? 1024 : res;
}
