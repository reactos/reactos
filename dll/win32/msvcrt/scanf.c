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

#include <conio.h>
#include <stdarg.h>
#include <limits.h>
#include <math.h>

#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "msvcrt.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);

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

/* vfscanf_l */
#undef WIDE_SCANF
#undef CONSOLE
#undef STRING
#undef SECURE
#include "scanf.h"

/* vfscanf_s_l */
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

/* vsscanf_l */
#undef WIDE_SCANF
#undef CONSOLE
#define STRING 1
#undef SECURE
#include "scanf.h"

/* vsscanf_s_l */
#define SECURE 1
#include "scanf.h"

/* vsnscanf_l */
#undef SECURE
#define STRING_LEN 1
#include "scanf.h"

/* vsnscanf_s_l */
#define SECURE
#include "scanf.h"
#undef STRING_LEN

/* vswscanf_l */
#define WIDE_SCANF 1
#undef CONSOLE
#define STRING 1
#undef SECURE
#include "scanf.h"

/* vsnwscanf_l */
#define STRING_LEN 1
#include "scanf.h"

/* vsnwscanf_s_l */
#define SECURE 1
#include "scanf.h"
#undef STRING_LEN

/* vswscanf_s_l */
#define SECURE 1
#include "scanf.h"

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
int WINAPIV fscanf(FILE *file, const char *format, ...)
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
int WINAPIV _fscanf_l(FILE *file, const char *format,
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
int WINAPIV fscanf_s(FILE *file, const char *format, ...)
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
int WINAPIV _fscanf_s_l(FILE *file, const char *format,
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
int WINAPIV scanf(const char *format, ...)
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
int WINAPIV _scanf_l(const char *format, _locale_t locale, ...)
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
int WINAPIV scanf_s(const char *format, ...)
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
int WINAPIV _scanf_s_l(const char *format, _locale_t locale, ...)
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
int WINAPIV fwscanf(FILE *file, const wchar_t *format, ...)
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
int WINAPIV _fwscanf_l(FILE *file, const wchar_t *format,
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
int WINAPIV fwscanf_s(FILE *file, const wchar_t *format, ...)
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
int WINAPIV _fwscanf_s_l(FILE *file, const wchar_t *format,
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
int WINAPIV wscanf(const wchar_t *format, ...)
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
int WINAPIV _wscanf_l(const wchar_t *format,
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
int WINAPIV wscanf_s(const wchar_t *format, ...)
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
int WINAPIV _wscanf_s_l(const wchar_t *format,
        _locale_t locale, ...)
{
    va_list valist;
    int res;

    va_start(valist, locale);
    res = vfwscanf_s_l(stdin, format, locale, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		sscanf (MSVCRT.@)
 */
int WINAPIV sscanf(const char *str, const char *format, ...)
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
int WINAPIV _sscanf_l(const char *str, const char *format,
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
int WINAPIV sscanf_s(const char *str, const char *format, ...)
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
int WINAPIV _sscanf_s_l(const char *str, const char *format,
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
int WINAPIV swscanf(const wchar_t *str, const wchar_t *format, ...)
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
int WINAPIV _swscanf_l(const wchar_t *str, const wchar_t *format,
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
int WINAPIV swscanf_s(const wchar_t *str, const wchar_t *format, ...)
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
int WINAPIV _swscanf_s_l(const wchar_t *str, const wchar_t *format,
        _locale_t locale, ...)
{
    va_list valist;
    int res;

    va_start(valist, locale);
    res = vswscanf_s_l(str, format, locale, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_cscanf (MSVCRT.@)
 */
int WINAPIV _cscanf(const char *format, ...)
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
int WINAPIV _cscanf_l(const char *format, _locale_t locale, ...)
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
int WINAPIV _cscanf_s(const char *format, ...)
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
int WINAPIV _cscanf_s_l(const char *format, _locale_t locale, ...)
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
int WINAPIV _cwscanf(const wchar_t *format, ...)
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
int WINAPIV _cwscanf_l(const wchar_t *format, _locale_t locale, ...)
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
int WINAPIV _cwscanf_s(const wchar_t *format, ...)
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
int WINAPIV _cwscanf_s_l(const wchar_t *format, _locale_t locale, ...)
{
    va_list valist;
    int res;

    va_start(valist, locale);
    res = vcwscanf_s_l(format, locale, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_snscanf (MSVCRT.@)
 */
int WINAPIV _snscanf(const char *input, size_t length, const char *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vsnscanf_l(input, length, format, NULL, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_snscanf_l (MSVCRT.@)
 */
int WINAPIV _snscanf_l(const char *input, size_t length,
        const char *format, _locale_t locale, ...)
{
    va_list valist;
    int res;

    va_start(valist, locale);
    res = vsnscanf_l(input, length, format, locale, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_snscanf_s (MSVCRT.@)
 */
int WINAPIV _snscanf_s(const char *input, size_t length, const char *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vsnscanf_s_l(input, length, format, NULL, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_snscanf_s_l (MSVCRT.@)
 */
int WINAPIV _snscanf_s_l(const char *input, size_t length,
        const char *format, _locale_t locale, ...)
{
    va_list valist;
    int res;

    va_start(valist, locale);
    res = vsnscanf_s_l(input, length, format, locale, valist);
    va_end(valist);
    return res;
}


/*********************************************************************
 *              __stdio_common_vsscanf (UCRTBASE.@)
 */
int CDECL __stdio_common_vsscanf(unsigned __int64 options,
                                       const char *input, size_t length,
                                       const char *format,
                                       _locale_t locale,
                                       va_list valist)
{
    /* LEGACY_WIDE_SPECIFIERS only has got an effect on the wide
     * scanf. LEGACY_MSVCRT_COMPATIBILITY affects parsing of nan/inf,
     * but parsing of those isn't implemented at all yet. */
    if (options & ~UCRTBASE_SCANF_MASK)
        FIXME("options %#I64x not handled\n", options);
    if (options & _CRT_INTERNAL_SCANF_SECURECRT)
        return vsnscanf_s_l(input, length, format, locale, valist);
    else
        return vsnscanf_l(input, length, format, locale, valist);
}

/*********************************************************************
 *              __stdio_common_vswscanf (UCRTBASE.@)
 */
int CDECL __stdio_common_vswscanf(unsigned __int64 options,
                                        const wchar_t *input, size_t length,
                                        const wchar_t *format,
                                        _locale_t locale,
                                        va_list valist)
{
    /* LEGACY_WIDE_SPECIFIERS only has got an effect on the wide
     * scanf. LEGACY_MSVCRT_COMPATIBILITY affects parsing of nan/inf,
     * but parsing of those isn't implemented at all yet. */
    if (options & ~UCRTBASE_SCANF_MASK)
        FIXME("options %#I64x not handled\n", options);
    if (options & _CRT_INTERNAL_SCANF_SECURECRT)
        return vsnwscanf_s_l(input, length, format, locale, valist);
    else
        return vsnwscanf_l(input, length, format, locale, valist);
}

/*********************************************************************
 *              __stdio_common_vfscanf (UCRTBASE.@)
 */
int CDECL __stdio_common_vfscanf(unsigned __int64 options,
                                       FILE *file,
                                       const char *format,
                                       _locale_t locale,
                                       va_list valist)
{
    if (options & ~(_CRT_INTERNAL_SCANF_SECURECRT | _CRT_INTERNAL_SCANF_LEGACY_WIDE_SPECIFIERS))
        FIXME("options %#I64x not handled\n", options);
    if (options & _CRT_INTERNAL_SCANF_SECURECRT)
        return vfscanf_s_l(file, format, locale, valist);
    else
        return vfscanf_l(file, format, locale, valist);
}

/*********************************************************************
 *              __stdio_common_vfwscanf (UCRTBASE.@)
 */
int CDECL __stdio_common_vfwscanf(unsigned __int64 options,
                                        FILE *file,
                                        const wchar_t *format,
                                        _locale_t locale,
                                        va_list valist)
{
    if (options & ~_CRT_INTERNAL_SCANF_SECURECRT)
        FIXME("options %#I64x not handled\n", options);
    if (options & _CRT_INTERNAL_SCANF_SECURECRT)
        return vfwscanf_s_l(file, format, locale, valist);
    else
        return vfwscanf_l(file, format, locale, valist);
}

/*********************************************************************
 *		_snwscanf (MSVCRT.@)
 */
int WINAPIV _snwscanf(wchar_t *input, size_t length,
        const wchar_t *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vsnwscanf_l(input, length, format, NULL, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_snwscanf_l (MSVCRT.@)
 */
int WINAPIV _snwscanf_l(wchar_t *input, size_t length,
        const wchar_t *format, _locale_t locale, ...)
{
    va_list valist;
    int res;

    va_start(valist, locale);
    res = vsnwscanf_l(input, length, format, locale, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_snwscanf_s (MSVCRT.@)
 */
int WINAPIV _snwscanf_s(wchar_t *input, size_t length,
        const wchar_t *format, ...)
{
    va_list valist;
    int res;

    va_start(valist, format);
    res = vsnwscanf_s_l(input, length, format, NULL, valist);
    va_end(valist);
    return res;
}

/*********************************************************************
 *		_snscanf_s_l (MSVCRT.@)
 */
int WINAPIV _snwscanf_s_l(wchar_t *input, size_t length,
        const wchar_t *format, _locale_t locale, ...)
{
    va_list valist;
    int res;

    va_start(valist, locale);
    res = vsnwscanf_s_l(input, length, format, locale, valist);
    va_end(valist);
    return res;
}

#if _MSVCR_VER==120

/*********************************************************************
 *		vsscanf (MSVCRT120.@)
 */
int CDECL MSVCRT_vsscanf(const char *buffer, const char *format, va_list valist)
{
    if (!MSVCRT_CHECK_PMT(buffer != NULL && format != NULL)) return -1;

    return vsscanf_l(buffer, format, NULL, valist);
}

/*********************************************************************
 *		vswscanf (MSVCRT120.@)
 */
int CDECL vswscanf(const wchar_t *buffer, const wchar_t *format, va_list valist)
{
    if (!MSVCRT_CHECK_PMT(buffer != NULL && format != NULL)) return -1;

    return vswscanf_l(buffer, format, NULL, valist);
}

#endif /* _MSVCR_VER>=120 */
