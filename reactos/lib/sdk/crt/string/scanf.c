/*
 * general implementation of scanf used by scanf, sscanf, fscanf,
 * _cscanf, wscanf, swscanf and fwscanf
 *
 * Copyright 1996,1998 Marcus Meissner
 * Copyright 1996 Jukka Iivonen
 * Copyright 1997,2000 Uwe Bonnes
 * Copyright 2000 Jon Griffiths
 * Copyright 2002 Daniel Gudbjartsson
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

#include <precomp.h>
#include <ctype.h>
#include <locale.h>

#ifdef _LIBCNT_
_locale_t get_locale(void){ return NULL; }
#else
_locale_t get_locale(void); //defined in locale.c
#endif 

// HACK for LIBCNT
#ifndef debugstr_a
#define debugstr_a
#endif

//extern FILE _iob[];

/* helper function for *scanf.  Returns the value of character c in the
 * given base, or -1 if the given character is not a digit of the base.
 */
static int char2digit(char c, int base) {
    if ((c>='0') && (c<='9') && (c<='0'+base-1)) return (c-'0');
    if (base<=10) return -1;
    if ((c>='A') && (c<='Z') && (c<='A'+base-11)) return (c-'A'+10);
    if ((c>='a') && (c<='z') && (c<='a'+base-11)) return (c-'a'+10);
    return -1;
}

/* helper function for *wscanf.  Returns the value of character c in the
 * given base, or -1 if the given character is not a digit of the base.
 */
static int wchar2digit(wchar_t c, int base) {
    if ((c>='0') && (c<='9') && (c<='0'+base-1)) return (c-'0');
    if (base<=10) return -1;
    if ((c>='A') && (c<='Z') && (c<='A'+base-11)) return (c-'A'+10);
    if ((c>='a') && (c<='z') && (c<='a'+base-11)) return (c-'a'+10);
    return -1;
}

#ifndef _LIBCNT_
/* vfscanf_l */
#undef WIDE_SCANF
#undef CONSOLE
#undef STRING
#undef SECURE
#include "scanf.h"

/* vfscanf_l */
#define SECURE 1
#include "scanf.h"

/* vfwscanf_l */
#define WIDE_SCANF 1
#undef CONSOLE
#undef STRING
#undef SECURE
#include "scanf.h"

/* vfwscanf_s_l */
#define SECURE 1
#include "scanf.h"
#endif

/* vsscanf_l */
#undef WIDE_SCANF
#undef CONSOLE
#define STRING 1
#undef SECURE
#include "scanf.h"

/* vsscanf_s_l */
#define SECURE 1
#include "scanf.h"

/* vswscanf_l */
#define WIDE_SCANF 1
#undef CONSOLE
#define STRING 1
#undef SECURE
#include "scanf.h"

/* vswscanf_s_l */
#define SECURE 1
#include "scanf.h"

#ifndef _LIBCNT_
/* vcscanf_l */
#undef WIDE_SCANF
#define CONSOLE 1
#undef STRING
#undef SECURE
#include "scanf.h"

/* vcscanf_s_l */
#define SECURE 1
#include "scanf.h"

/* vcwscanf_l */
#define WIDE_SCANF 1
#define CONSOLE 1
#undef STRING
#undef SECURE
#include "scanf.h"

/* vcwscanf_s_l */
#define SECURE 1
#include "scanf.h"

/*********************************************************************
 *		fscanf (MSVCRT.@)
 */
int fscanf(FILE *file, const char *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vfscanf_l(file, format, NULL, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_fscanf_l (MSVCRT.@)
 */
int CDECL _fscanf_l(FILE *file, const char *format,
        _locale_t locale, ...)
{
    va_list valist;
    int res;

    va_start(valist, locale);
    res = vfscanf_l(file, format, locale, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		fscanf_s (MSVCRT.@)
 */
int CDECL fscanf_s(FILE *file, const char *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vfscanf_s_l(file, format, NULL, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_fscanf_s_l (MSVCRT.@)
 */
int CDECL _fscanf_s_l(FILE *file, const char *format,
        _locale_t locale, ...)
{
    va_list valist;
    int res;

    va_start(valist, locale);
    res = vfscanf_s_l(file, format, locale, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		scanf (MSVCRT.@)
 */
int CDECL scanf(const char *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vfscanf_l(stdin, format, NULL, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_scanf_l (MSVCRT.@)
 */
int CDECL _scanf_l(const char *format, _locale_t locale, ...)
{
    va_list valist;
    int res;

    va_start(valist, locale);
    res = vfscanf_l(stdin, format, locale, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		scanf_s (MSVCRT.@)
 */
int CDECL scanf_s(const char *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vfscanf_s_l(stdin, format, NULL, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_scanf_s_l (MSVCRT.@)
 */
int CDECL _scanf_s_l(const char *format, _locale_t locale, ...)
{
    va_list valist;
    int res;

    va_start(valist, locale);
    res = vfscanf_s_l(stdin, format, locale, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		fwscanf (MSVCRT.@)
 */
int fwscanf(FILE *file, const wchar_t *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vfwscanf_l(file, format, NULL, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_fwscanf_l (MSVCRT.@)
 */
int CDECL _fwscanf_l(FILE *file, const wchar_t *format,
        _locale_t locale, ...)
{
    va_list valist;
    int res;

    va_start(valist, locale);
    res = vfwscanf_l(file, format, locale, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		fwscanf_s (MSVCRT.@)
 */
int CDECL fwscanf_s(FILE *file, const wchar_t *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vfwscanf_s_l(file, format, NULL, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_fwscanf_s_l (MSVCRT.@)
 */
int CDECL _fwscanf_s_l(FILE *file, const wchar_t *format,
        _locale_t locale, ...)
{
    va_list valist;
    int res;

    va_start(valist, locale);
    res = vfwscanf_s_l(file, format, locale, valist);
    va_end(valist);
    return res;
}


/*********************************************************************
 *		wscanf (MSVCRT.@)
 */
int CDECL wscanf(const wchar_t *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vfwscanf_l(stdin, format, NULL, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_wscanf_l (MSVCRT.@)
 */
int CDECL _wscanf_l(const wchar_t *format,
        _locale_t locale, ...)
{
    va_list valist;
    int res;

    va_start(valist, locale);
    res = vfwscanf_l(stdin, format, locale, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		wscanf_s (MSVCRT.@)
 */
int CDECL wscanf_s(const wchar_t *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vfwscanf_s_l(stdin, format, NULL, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_wscanf_s_l (MSVCRT.@)
 */
int CDECL _wscanf_s_l(const wchar_t *format,
        _locale_t locale, ...)
{
    va_list valist;
    int res;

    va_start(valist, locale);
    res = vfwscanf_s_l(stdin, format, locale, valist);
    va_end(valist);
    return res;
}
#endif

/*********************************************************************
 *		sscanf (MSVCRT.@)
 */
int CDECL sscanf(const char *str, const char *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vsscanf_l(str, format, NULL, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_sscanf_l (MSVCRT.@)
 */
int CDECL _sscanf_l(const char *str, const char *format,
        _locale_t locale, ...)
{
    va_list valist;
    int res;

    va_start(valist, locale);
    res = vsscanf_l(str, format, locale, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		sscanf_s (MSVCRT.@)
 */
int CDECL sscanf_s(const char *str, const char *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vsscanf_s_l(str, format, NULL, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_sscanf_s_l (MSVCRT.@)
 */
int CDECL _sscanf_s_l(const char *str, const char *format,
        _locale_t locale, ...)
{
    va_list valist;
    int res;

    va_start(valist, locale);
    res = vsscanf_s_l(str, format, locale, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		swscanf (MSVCRT.@)
 */
int CDECL swscanf(const wchar_t *str, const wchar_t *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vswscanf_l(str, format, NULL, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_swscanf_l (MSVCRT.@)
 */
int CDECL _swscanf_l(const wchar_t *str, const wchar_t *format,
        _locale_t locale, ...)
{
    va_list valist;
    int res;

    va_start(valist, locale);
    res = vswscanf_l(str, format, locale, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		swscanf_s (MSVCRT.@)
 */
int CDECL swscanf_s(const wchar_t *str, const wchar_t *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vswscanf_s_l(str, format, NULL, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_swscanf_s_l (MSVCRT.@)
 */
int CDECL _swscanf_s_l(const wchar_t *str, const wchar_t *format,
        _locale_t locale, ...)
{
    va_list valist;
    int res;

    va_start(valist, locale);
    res = vswscanf_s_l(str, format, locale, valist);
    va_end(valist);
    return res;
}

#ifndef _LIBCNT_
/*********************************************************************
 *		_cscanf (MSVCRT.@)
 */
int CDECL _cscanf(const char *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vcscanf_l(format, NULL, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_cscanf_l (MSVCRT.@)
 */
int CDECL _cscanf_l(const char *format, _locale_t locale, ...)
{
    va_list valist;
    int res;

    va_start(valist, locale);
    res = vcscanf_l(format, locale, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_cscanf_s (MSVCRT.@)
 */
int CDECL _cscanf_s(const char *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vcscanf_s_l(format, NULL, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_cscanf_s_l (MSVCRT.@)
 */
int CDECL _cscanf_s_l(const char *format, _locale_t locale, ...)
{
    va_list valist;
    int res;

    va_start(valist, locale);
    res = vcscanf_s_l(format, locale, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_cwscanf (MSVCRT.@)
 */
int CDECL _cwscanf(const wchar_t *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vcwscanf_l(format, NULL, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_cwscanf_l (MSVCRT.@)
 */
int CDECL _cwscanf_l(const wchar_t *format, _locale_t locale, ...)
{
    va_list valist;
    int res;

    va_start(valist, locale);
    res = vcwscanf_l(format, locale, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_cwscanf_s (MSVCRT.@)
 */
int CDECL _cwscanf_s(const wchar_t *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vcwscanf_s_l(format, NULL, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_cwscanf_s_l (MSVCRT.@)
 */
int CDECL _cwscanf_s_l(const wchar_t *format, _locale_t locale, ...)
{
    va_list valist;
    int res;

    va_start(valist, locale);
    res = vcwscanf_s_l(format, locale, valist);
    va_end(valist);
    return res;
}
#endif
